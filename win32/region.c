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
#define APERTURE GET_REGION(self)->aperture

HRGN
region_create( Handle mask)
{
	LONG w, h, x, y, size = 256;
	HRGN    rgn = NULL;
	Byte    * idata;
	RGNDATA * rdata = NULL;
	RECT    * current;
	Bool      set = 0;

	if ( !mask)
		return NULL;

	dobjCheck( mask) NULL;

	w = PImage( mask)-> w;
	h = PImage( mask)-> h;
	if ( dsys( mask) s. image. imgCachedRegion) {
		rgn = CreateRectRgn(0,0,0,0);
		CombineRgn( rgn, dsys( mask) s. image. imgCachedRegion, NULL, RGN_COPY);
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
		CombineRgn( dsys( mask) s. image. imgCachedRegion, rgn, NULL, RGN_COPY);
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
	APERTURE = 0;
	return true;
}

static Bool
rgn_rect(Handle self, int count, Box * r)
{
	int i, h;
	Box * rr;
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

static Bool
rgn_polygon(Handle self, PolygonRegionRec * r)
{
	int i, max, open, xp_points;
	POINT * xp;

	open =
		r->points[r->n_points-1].x != r->points[0].x ||
		r->points[r->n_points-1].y != r->points[0].y;
	xp_points = r->n_points + (open ? 1 : 0);

	if ( !( xp = malloc( sizeof(POINT) * xp_points ))) {
		warn("Not enough memory");
		return false;
	}

	for ( i = 0, max = 0; i < r->n_points; i++) {
		if ( max < r->points[i].y)
			max = r->points[i].y;
	}
	max++;
	for ( i = 0; i < r->n_points; i++) {
		xp[i].x = r->points[i].x;
		xp[i].y = max - r->points[i].y - 1;
	}
	if ( open ) {
		xp[i].x = r->points[0].x;
		xp[i].y = max - r->points[0].y - 1;
	}

	APERTURE = max;
	REGION = CreatePolygonRgn( xp, r->n_points, 
		((r-> fill_mode & fmWinding) == fmAlternate) ? ALTERNATE : WINDING);
	if (( r->fill_mode & fmOverlay) == 0) goto NO_OVERLAY;

	/* superimpose polyline points using Bresenham
	because windows regions are as broken as filled shapes */
	for ( i = 0; i < xp_points-1; i++) {
		int curr_maj, curr_min, to_maj, delta_maj, delta_min;
		int delta_y, delta_x;
		int dir = 0, d, d_inc1, d_inc2;
		int inc_maj, inc_min;
		int x, y, acc_x = 0, acc_y = INT_MIN, ox;
		POINT a = xp[i], b = xp[i+1];
		delta_y = b.y - a.y;
		delta_x = b.x - a.x;
		if (abs(delta_y) > abs(delta_x)) dir = 1;

		if (dir) {
			curr_maj = a.y;
			curr_min = a.x;
			to_maj = b.y;
			delta_maj = delta_y;
			delta_min = delta_x;
		} else {
			curr_maj = a.x;
			curr_min = a.y;
			to_maj = b.x;
			delta_maj = delta_x;
			delta_min = delta_y;
		}

		if (delta_maj != 0)
			inc_maj = (abs(delta_maj)==delta_maj ? 1 : -1);
		else
			inc_maj = 0;

		if (delta_min != 0)
			inc_min = (abs(delta_min)==delta_min ? 1 : -1);
		else
			inc_min = 0;

		delta_maj = abs(delta_maj);
		delta_min = abs(delta_min);

		d      = (delta_min << 1) - delta_maj;
		d_inc1 = (delta_min << 1);
		d_inc2 = ((delta_min - delta_maj) << 1);

		while(1) {
			ox = x;
			if (dir) {
				x = curr_min;
				y = curr_maj;
			} else {
				x = curr_maj;
				y = curr_min;
			}
			if ( acc_y != y ) {
				if ( acc_y > INT_MIN) {
					HRGN reg;
					int x1, x2;
					if (ox < acc_x) {
						x1 = ox;
						x2 = acc_x;
					} else {
						x1 = acc_x;
						x2 = ox;
					}
					reg = CreateRectRgn(x1, acc_y, x2+1, acc_y + 1);
					CombineRgn( REGION, REGION, reg, RGN_OR);
					DeleteObject(reg);
				}
				acc_x = x;
				acc_y = y;
			}

			if (curr_maj == to_maj) break;
			curr_maj += inc_maj;
			if (d < 0) {
				d += d_inc1;
			} else {
				d += d_inc2;
				curr_min += inc_min;
			}
		}
		if ( acc_y > INT_MIN) {
			HRGN reg;
			int x1, x2;
			if (x < acc_x) {
				x1 = x;
				x2 = acc_x;
			} else {
				x1 = acc_x;
				x2 = x;
			}
			reg = CreateRectRgn(x1, acc_y, x2+1, acc_y + 1);
			CombineRgn( REGION, REGION, reg, RGN_OR);
			DeleteObject(reg);
		}
	}

NO_OVERLAY:
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
		APERTURE = PImage(image)->h;
	return true;
}

Bool
apc_region_create( Handle self, PRegionRec rec)
{
	switch( rec-> type ) {
	case rgnEmpty:
		return rgn_empty(self);
	case rgnRectangle:
		return rgn_rect(self, rec->data.box.n_boxes, rec->data.box.boxes);
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
	if ( !( rgn = CreateRectRgn(
		c. left,  sys lastSize. y - c. top,
		c. right + 1, sys lastSize. y - c. bottom - 1))
		) apiErrRet;
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
	GET_REGION(mask)-> aperture = sys lastSize. y - rect.top;
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
	OffsetRgn( rgn, 0, sys lastSize.y - GET_REGION(region)->aperture);
	if ( is_apt(aptLayeredPaint) && sys layeredParentRegion )
		CombineRgn( rgn, rgn, sys layeredParentRegion, RGN_AND);
	SelectClipRgn( sys ps, rgn);
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

	ret = malloc(sizeof(RegionRec) + sizeof(Box) * rgndata-> rdh. nCount );
	if ( ret == NULL ) {
		free(ret);
		warn("Not enough memory\n");
		return NULL;
	}

	ret-> type = rgnRectangle;
	ret-> data. box. n_boxes = rgndata->rdh. nCount;
	src = (RECT*) &(rgndata->Buffer);
	dst = ret-> data. box. boxes = (Box*) (((Byte*)ret) + sizeof(RegionRec));
	aperture = APERTURE;
	for ( i = 0; i < ret->data. box. n_boxes; i++, src++, dst++) {
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
