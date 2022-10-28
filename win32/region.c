#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PComponent) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle
#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

#define GET_REGION(obj) (&(dsys(obj)s.region))
#define REGION GET_REGION(self)->region
#define APERTURE GET_REGION(self)->aperture

Bool
apc_region_create( Handle self, PRegionRec rec)
{
	int i, h, count;
	Box *rr, *r;

	if ( rec == NULL ) {
		REGION = CreateRectRgn(0,0,0,0);
		APERTURE = 0;
		return true;
	}

	r = rec->boxes;
	count = rec->n_boxes;

	h = r->y + r->height;
	for ( i = 0, rr = r; i < count; i++, rr++) {
		if ( h < rr->y + rr->height)
			h = rr->y + rr->height;
	}
	REGION = CreateRectRgn(r->x, h - r->y - r->height, r->width+r->x, h - r->y);
	APERTURE = h;
	for ( i = 1, r++; i < count; i++, r++) {
		HRGN reg = CreateRectRgn(r->x, h - r->y - r->height, r->width+r->x, h - r->y);
		CombineRgn( REGION, REGION, reg, RGN_OR);
		DeleteObject(reg);
	}
	return true;
}

Bool
apc_region_destroy( Handle self)
{
	if (REGION) {
		DeleteObject(REGION);
		REGION = NULL;
	}
	return true;
}

Bool
apc_region_offset( Handle self, int dx, int dy)
{
	OffsetRgn(REGION, dx, -dy);
	return false;
}

static Handle ctx_rgnop2RGN[] = {
	rgnopCopy,        RGN_COPY,
	rgnopUnion,       RGN_OR,
	rgnopIntersect,   RGN_AND,
	rgnopXor,         RGN_XOR,
	rgnopDiff,        RGN_DIFF,
	endCtx
};

Bool
apc_region_combine( Handle self, Handle other_region, int rgnop)
{
	RegionData * r2;
	int d;
	Bool ok;

	r2 = GET_REGION(other_region);

	if ( rgnop == rgnopCopy ) {
		ok = CombineRgn( REGION, r2->region, NULL, RGN_COPY);
		APERTURE = r2-> aperture;
		return ok;
	}

	d = APERTURE - r2-> aperture;
	if ( d > 0 )
		OffsetRgn( r2-> region, 0, d);
	else
		OffsetRgn( REGION, 0, -d);

	rgnop = ctx_remap_def( rgnop, ctx_rgnop2RGN, true, RGN_COPY);
	ok = CombineRgn( REGION, REGION, r2->region, rgnop);

	if ( d > 0 )
		OffsetRgn( r2-> region, 0, -d);
	else
		APERTURE = r2-> aperture;

	return ok;
}

Bool
apc_region_point_inside( Handle self, Point p)
{
	return PtInRegion( REGION, p.x, APERTURE - p.y - 1);
}

int
apc_region_rect_inside( Handle self, Rect r)
{
	RECT q;
	HRGN t1, t2;
	int ret;

	q. left   = r. left;
	q. top    = APERTURE - r. bottom;
	q. right  = r. right + 1;
	q. bottom = APERTURE - r. top - 1;
	if ( RectInRegion( REGION, &q) == 0 ) return rgnOutside;

	t1 = CreateRectRgnIndirect(&q);
	t2 = CreateRectRgnIndirect(&q);

	CombineRgn( t2, REGION, t1, RGN_AND);
	ret = EqualRgn( t1, t2 ) ? rgnInside : rgnPartially;

	DeleteObject(t2);
	DeleteObject(t1);
	return ret;
}

Bool
apc_region_equals( Handle self, Handle other_region)
{
	return EqualRgn( REGION, GET_REGION(other_region)->region);
}

Bool
apc_region_is_empty( Handle self)
{
	RECT xr;
	return GetRgnBox( REGION, &xr) == NULLREGION;
}

Box
apc_region_get_box( Handle self)
{
	Box box;
	RECT xr;
	if ( GetRgnBox( REGION, &xr) == NULLREGION) {
		memset(&box, 0, sizeof(box));
		return box;
	}
	box. x      = xr. left;
	box. y      = APERTURE - xr. bottom;
	box. width  = xr. right - xr. left;
	box. height = xr. bottom - xr. top;
	return box;
}

ApiHandle
apc_region_get_handle( Handle self)
{
	return (ApiHandle) REGION;
}

Rect
apc_gp_get_clip_rect( Handle self)
{
	RECT r;
	Rect rr = {0,0,0,0};
	objCheck rr;
	if ( !is_opt( optInDraw) || !sys ps) return rr;
	if ( !GetClipBox( sys ps, &r)) apiErr;
	if ( IsRectEmpty( &r)) return rr;
	rr. left   = r. left;
	rr. right  = r. right - 1;
	rr. bottom = sys last_size. y - r. bottom;
	rr. top    = sys last_size. y - r. top    - 1;

	rr. left   += sys transform2. x;
	rr. right  += sys transform2. x;
	rr. top    -= sys transform2. y;
	rr. bottom -= sys transform2. y;

	return rr;
}

Bool
apc_gp_set_clip_rect( Handle self, Rect c)
{
	HRGN rgn;
	objCheck false;
	if ( !is_opt( optInDraw) || !sys ps) return true;
	// inclusive-exclusive
	c. left   -= sys transform2. x;
	c. right  -= sys transform2. x;
	c. top    += sys transform2. y;
	c. bottom += sys transform2. y;
	check_swap( c. top, c. bottom);
	check_swap( c. left, c. right);
	if ( !( rgn = CreateRectRgn(
		c. left,  sys last_size. y - c. top,
		c. right + 1, sys last_size. y - c. bottom - 1))
		) apiErrRet;
	if ( is_apt(aptLayeredPaint) && sys layered_parent_region )
		CombineRgn( rgn, rgn, sys layered_parent_region, RGN_AND);
	if ( !SelectClipRgn( sys ps, rgn)) apiErr;
	if ( sys graphics ) {
		GPCALL GdipSetClipHrgn(sys graphics, rgn, CombineModeReplace);
		apiGPErrCheck;
	}
	if ( !DeleteObject( rgn)) apiErr;
	return true;
}

Bool
apc_gp_get_region( Handle self, Handle mask)
{
	HRGN rgn;
	RECT rect;
	int res;

	objCheck false;
	if ( !is_opt( optInDraw) || !sys ps) return false;

	if ( !mask ) {
		rgn = CreateRectRgn(0,0,0,0);
		res = GetClipRgn( sys ps, rgn );
		DeleteObject(rgn);
		return res > 0;
	}

	rgn = GET_REGION(mask)-> region;
	res = GetClipRgn( sys ps, rgn );
	if ( res <= 0)        // error or just no region
		return false;
	GetRgnBox(rgn, &rect);
	OffsetRgn( rgn, sys transform2. x, sys transform2. y - rect.top);
	GET_REGION(mask)-> aperture = sys last_size. y - rect.top;
	return true;
}

Bool
apc_gp_set_region( Handle self, Handle region)
{
	HRGN rgn = NULL;
	objCheck false;

	if ( !is_opt( optInDraw) || !sys ps) return true;

	if ( !region) {
		SelectClipRgn( sys ps, NULL);
		return true;
	}
	rgn = CreateRectRgn(0,0,0,0);
	CombineRgn(rgn, GET_REGION(region)->region, NULL, RGN_COPY);
	OffsetRgn( rgn, -sys transform2. x, -sys transform2. y);
	OffsetRgn( rgn, 0, sys last_size.y - GET_REGION(region)->aperture);
	if ( is_apt(aptLayeredPaint) && sys layered_parent_region )
		CombineRgn( rgn, rgn, sys layered_parent_region, RGN_AND);
	SelectClipRgn( sys ps, rgn);
	if ( sys graphics ) {
		GPCALL GdipSetClipHrgn(sys graphics, rgn, CombineModeReplace);
		apiGPErrCheck;
	}
	DeleteObject( rgn);
	return true;
}

PRegionRec
apc_region_copy_rects( Handle self)
{
	int i, aperture, size;
	PRegionRec ret;
	Box *dst;
	RECT *src;
	RGNDATA *rgndata;

	size = GetRegionData( REGION, 0, NULL);
	if ( !( rgndata = malloc(size))) {
		warn("Not enough memory\n");
		return NULL;
	}
	size = GetRegionData( REGION, size, rgndata);
	if ( size == 0) return NULL;

	if ( !( ret = img_region_new( rgndata-> rdh. nCount )))
		return NULL;

	ret-> n_boxes = rgndata->rdh. nCount;
	src = (RECT*) &(rgndata->Buffer);
	dst = ret-> boxes;
	aperture = APERTURE;
	for ( i = 0; i < ret->n_boxes; i++, src++, dst++) {
		dst-> x = src-> left;
		dst-> y = aperture - src-> bottom;
		dst-> width  = src-> right - src->left;
		dst-> height = src-> bottom - src->top;
		/* printf("%d: %ld %ld %ld %ld => %d %d %d %d\n", aperture, src->left, src->top, src->right, src->bottom, dst->x, dst->y, dst-> width, dst->height); */
	}
	free(rgndata);

	return ret;
}

#ifdef __cplusplus
}
#endif
