#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "win32\win32guts.h"
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
#define HEIGHT GET_REGION(self)->height

HRGN
region_create( Handle mask)
{
	LONG w, h, x, y, size = 256;
	HRGN    rgn = NULL;
	Byte    * idata;
	RGNDATA * rdata = nil;
	RECT    * current;
	Bool      set = 0;

	if ( !mask)
		return NULL;

	dobjCheck( mask) NULL;

	w = PImage( mask)-> w;
	h = PImage( mask)-> h;
	if ( dsys( mask) s. image. imgCachedRegion) {
		rgn = CreateRectRgn(0,0,0,0);
		CombineRgn( rgn, dsys( mask) s. image. imgCachedRegion, nil, RGN_COPY);
		return rgn;
	}

	idata  = PImage( mask)-> data + PImage( mask)-> dataSize - PImage( mask)-> lineSize;

	rdata = ( RGNDATA*) malloc( sizeof( RGNDATAHEADER) + size * sizeof( RECT));
	if ( !rdata) {
		warn("Not enough memory");
		return NULL;
	}

	rdata-> rdh. nCount = 0;
	current = ( RECT * ) &( rdata-> Buffer);
	current--;

	for ( y = 0; y < h; y++) {
		for ( x = 0; x < w; x++) {
			if ( idata[ x >> 3] == 0) {
				x += 7;
				continue;
			}
			if ( idata[ x >> 3] & ( 1 << ( 7 - ( x & 7)))) {
				if ( set && current-> top == y && current-> right == x)
					current-> right++;
				else {
					set = 1;
					if ( rdata-> rdh. nCount >= size) {
						void * xrdata = ( RGNDATA *) realloc( rdata, sizeof( RGNDATAHEADER) + ( size *= 3) * sizeof( RECT));
						if ( !xrdata) {
							free( rdata);
							return NULL;
						}
						rdata = xrdata;
						current = ( RECT * ) &( rdata-> Buffer);
						current += rdata-> rdh. nCount - 1;
					}
					rdata-> rdh. nCount++;
					current++;
					current-> left   = x;
					current-> top    = y;
					current-> right  = x + 1;
					current-> bottom = y + 1;
				}
			}
		}
		idata -= PImage( mask)-> lineSize;
	}


	if ( set) {
		rdata-> rdh. dwSize          = sizeof( RGNDATAHEADER);
		rdata-> rdh. iType           = RDH_RECTANGLES;
		rdata-> rdh. nRgnSize        = rdata-> rdh. nCount * sizeof( RECT);
		rdata-> rdh. rcBound. left   = 0;
		rdata-> rdh. rcBound. top    = 0;
		rdata-> rdh. rcBound. right  = h;
		rdata-> rdh. rcBound. bottom = w;

		if ( !( rgn = ExtCreateRegion( NULL,
			sizeof( RGNDATAHEADER) + ( rdata-> rdh. nCount * sizeof( RECT)), rdata))) {
			apcErr( 900);
		}

		dsys( mask) s. image. imgCachedRegion = CreateRectRgn(0,0,0,0);
		CombineRgn( dsys( mask) s. image. imgCachedRegion, rgn, nil, RGN_COPY);
	} else {
		dsys( mask) s. image. imgCachedRegion = rgn = CreateRectRgn(0,0,0,0);
	}
	free( rdata);

	return rgn;
}


static Bool
rgn_empty(Handle self)
{
	REGION = CreateRectRgn(0,0,0,0);
	HEIGHT = 0;
	return true;
}

static Bool
rgn_rect(Handle self, Box r)
{
	REGION = CreateRectRgn(r.x, 0, r.width+r.x, r.height);
	HEIGHT = r.y + r.height;
	return true;
}

static Bool
rgn_polygon(Handle self, PolygonRegionRec * r)
{
	int i, max;
	POINT * xp;

	if ( !( xp = malloc( sizeof(POINT) * r->n_points ))) {
		warn("Not enough memory");
		return false;
	}

	for ( i = 0, max = 0; i < r->n_points; i++) {
		if ( max < r->points[i].y)
			max = r->points[i].y;
	}
	for ( i = 0; i < r->n_points; i++) {
		xp[i].x = r->points[i].x;
		xp[i].y = max - r->points[i].y;
	}

	HEIGHT = max;
	REGION = CreatePolygonRgn( xp, r->n_points, r-> winding ? WINDING : ALTERNATE );

	free( xp );
	return true;
}

static Bool
rgn_image(Handle self, Handle image)
{
	REGION = region_create(image);

	if ( !REGION )
		REGION = CreateRectRgn(0,0,0,0);
	else
		HEIGHT = PImage(image)->h;
	return true;
}

Bool
apc_region_create( Handle self, PRegionRec rec)
{
	switch( rec-> type ) {
	case rgnEmpty:
		return rgn_empty(self);
	case rgnRectangle:
		return rgn_rect(self, rec->data.box);
	case rgnPolygon:
		return rgn_polygon(self, &rec->data.polygon);
	case rgnImage:
		return rgn_image(self, rec->data.image);
	default:
		return false;
	}
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
		HEIGHT = r2-> height;
		return ok;
	}

	d = HEIGHT - r2-> height;
	if ( d > 0 )
		OffsetRgn( r2-> region, 0, d);
	else
		OffsetRgn( REGION, 0, -d);

	rgnop = ctx_remap_def( rgnop, ctx_rgnop2RGN, true, RGN_COPY);
	ok = CombineRgn( REGION, REGION, r2->region, rgnop);

	if ( d > 0 )
		OffsetRgn( r2-> region, 0, -d);
	else
		HEIGHT = r2-> height;

	return ok;
}

Bool
apc_region_point_inside( Handle self, Point p)
{
	return PtInRegion( REGION, p.x, HEIGHT - p.y - 1);
}

int
apc_region_rect_inside( Handle self, Rect r)
{
	RECT q;
	HRGN t1, t2;
	int ret;

	q. left   = r. left;
	q. top    = HEIGHT - r. bottom;
	q. right  = r. right + 1;
	q. bottom = HEIGHT - r. top - 1;
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
	box. y      = HEIGHT - xr. bottom;
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
	Point p;
	Rect rr = {0,0,0,0};
	objCheck rr;
	if ( !is_opt( optInDraw) || !sys ps) return rr;
	if ( !GetClipBox( sys ps, &r)) apiErr;
	if ( IsRectEmpty( &r)) return rr;
	rr. left   = r. left;
	rr. right  = r. right - 1;
	rr. bottom = sys lastSize. y - r. bottom;
	rr. top    = sys lastSize. y - r. top    - 1;

	p = apc_gp_get_transform(self);
	rr. left   += p.x;
	rr. right  += p.x;
	rr. top    += p.y;
	rr. bottom += p.y;

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
	if ( !( rgn = CreateRectRgn( c. left,  sys lastSize. y - c. top,
										c. right + 1, sys lastSize. y - c. bottom - 1))) apiErrRet;
	if ( is_apt(aptLayeredPaint) && sys layeredParentRegion )
		CombineRgn( rgn, rgn, sys layeredParentRegion, RGN_AND);
	if ( !SelectClipRgn( sys ps, rgn)) apiErr;
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
	GET_REGION(mask)-> height = sys lastSize. y - rect.top;
	return true;
}

Bool
apc_gp_set_region( Handle self, Handle region)
{
	HRGN rgn = nil;
	objCheck false;

	if ( !is_opt( optInDraw) || !sys ps) return true;

	if ( !region) {
		SelectClipRgn( sys ps, nil);
		return true;
	}
	rgn = CreateRectRgn(0,0,0,0);
	CombineRgn(rgn, GET_REGION(region)->region, NULL, RGN_COPY);
	OffsetRgn( rgn, -sys transform2. x, -sys transform2. y);
	OffsetRgn( rgn, 0, sys lastSize.y - GET_REGION(region)->height);
	if ( is_apt(aptLayeredPaint) && sys layeredParentRegion )
		CombineRgn( rgn, rgn, sys layeredParentRegion, RGN_AND);
	SelectClipRgn( sys ps, rgn);
	DeleteObject( rgn);
	return true;
}


#ifdef __cplusplus
}
#endif
