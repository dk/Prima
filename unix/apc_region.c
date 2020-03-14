#include "unix/guts.h"
#include "Icon.h"

#define pREGION GET_REGION(self)->region
#define pAPERTURE GET_REGION(self)->aperture

Region
prima_region_create( Handle mask)
{
	unsigned long w, h, x, y, size = 256, count = 0;
	Region	  rgn = None;
	Byte	   * idata;
	XRectangle * current, * rdata;
	Bool	  set = 0;

	if ( !mask)
		return None;

	w = PImage( mask)-> w;
	h = PImage( mask)-> h;
	/*
		XUnionRegion is actually SLOWER than the image scan -
		- uncomment if this is wrong
	if ( X( mask)-> cached_region) {
		rgn = XCreateRegion();
		XUnionRegion( rgn, X( mask)-> cached_region, rgn);
		return rgn;
	}
	*/

	idata  = PImage( mask)-> data + PImage( mask)-> dataSize - PImage( mask)-> lineSize;

	rdata = ( XRectangle*) malloc( size * sizeof( XRectangle));
	if ( !rdata) {
		warn("Not enough memory");
		return None;
	}

	count = 0;
	current = rdata;
	current--;

	for ( y = 0; y < h; y++) {
		for ( x = 0; x < w; x++) {
			if ( idata[ x >> 3] == 0) {
				x += 7;
				continue;
			}
			if ( idata[ x >> 3] & ( 1 << ( 7 - ( x & 7)))) {
				if ( set && current-> y == y && current-> x + current-> width == x)
					current-> width++;
				else {
					set = 1;
					if ( count >= size) {
						void * xrdata = realloc( rdata, ( size *= 3) * sizeof( XRectangle));
						if ( !xrdata) {
							warn("Not enough memory");
							free( rdata);
							return None;
						}
						rdata = xrdata;
						current = rdata;
						current += count - 1;
					}
					count++;
					current++;
					current-> x   = x;
					current-> y   = y;
					current-> width  = 1;
					current-> height = 1;
				}
			}
		}
		idata -= PImage( mask)-> lineSize;
	}

	if ( set) {
		rgn = XCreateRegion();
		for ( x = 0, current = rdata; x < count; x++, current++)
			XUnionRectWithRegion( current, rgn, rgn);

		/*
		X( mask)-> cached_region = XCreateRegion();
		XUnionRegion( X( mask)-> cached_region, rgn, X( mask)-> cached_region);
		*/
	}
	free( rdata);

	return rgn;
}

static Bool
rgn_empty(Handle self)
{
	XRectangle xr;
	pREGION = XCreateRegion();
	xr. x = 0;
	xr. y = 0;
	xr. width  = 1;
	xr. height = 1;
	XUnionRectWithRegion( &xr, pREGION, pREGION);
	XXorRegion( pREGION, pREGION, pREGION);
	pAPERTURE = 0;
	return true;
}

static Bool
rgn_rect(Handle self, int count, Box * r)
{
	int i, aperture;
	Box * rr;
	pREGION = XCreateRegion();

	aperture = r->y + r->height;
	for ( i = 0, rr = r; i < count; i++, rr++) {
		if ( aperture < rr->y + rr->height)
			aperture = rr->y + rr->height;
	}

	for ( i = 0; i < count; i++, r++) {
		XRectangle xr;
		xr. x = r-> x;
		xr. y = aperture - r->y - r->height;
		xr. width  = r-> width;
		xr. height = r-> height;
		XUnionRectWithRegion( &xr, pREGION, pREGION);
	}
	pAPERTURE = aperture;
	return true;
}

static Bool
rgn_polygon(Handle self, PolygonRegionRec * r)
{
	int i, max, open, xp_points;
	XPoint * xp;

	open =
		r->points[r->n_points-1].x != r->points[0].x ||
		r->points[r->n_points-1].y != r->points[0].y;
	xp_points = r->n_points + (open ? 1 : 0);

	if ( !( xp = malloc( sizeof(XPoint) * xp_points ))) {
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

	pAPERTURE = max;
	pREGION = XPolygonRegion( xp, r->n_points, 
		((r-> fill_mode & fmWinding) == fmAlternate) ? EvenOddRule : WindingRule);

	if (( r->fill_mode & fmOverlay) == 0) goto NO_OVERLAY;

	/* superimpose polyline points using Bresenham
	because x11 regions are as broken as filled shapes */
	for ( i = 0; i < xp_points-1; i++) {
		int curr_maj, curr_min, to_maj, delta_maj, delta_min;
		int delta_y, delta_x;
		int dir = 0, d, d_inc1, d_inc2;
		int inc_maj, inc_min;
		int x, y, acc_x = 0, acc_y = INT_MIN, ox;
		XPoint a = xp[i], b = xp[i+1];
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

		x = INT_MIN;
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
					XRectangle xr;
					xr. y = acc_y;
					xr. height = 1;
					if (ox < acc_x) {
						xr.x = ox;
						xr.width = acc_x - ox + 1;
					} else {
						xr.x = acc_x;
						xr.width = ox - acc_x + 1;
					}
					XUnionRectWithRegion( &xr, pREGION, pREGION);
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
			XRectangle xr;
			xr. y = acc_y;
			xr. height = 1;
			if (x < acc_x) {
				xr.x = x;
				xr.width = acc_x - x + 1;
			} else {
				xr.x = acc_x;
				xr.width = x - acc_x + 1;
			}
			XUnionRectWithRegion( &xr, pREGION, pREGION);
		}
	}

NO_OVERLAY:
	free( xp );
	return true;
}

static Bool
rgn_image(Handle self, Handle image)
{
	pREGION = prima_region_create(image);

	if ( !pREGION )
		pREGION = XCreateRegion();
	else
		pAPERTURE = PImage(image)->h;
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
	if ( pREGION ) {
		XDestroyRegion(pREGION);
		pREGION = NULL;
	}
	return true;
}

ApiHandle
apc_region_get_handle( Handle self)
{
	return (ApiHandle) pREGION;
}

Bool
apc_region_offset( Handle self, int dx, int dy)
{
	XOffsetRegion(pREGION, dx, -dy);
	return true;
}

Bool
apc_region_combine( Handle self, Handle other_region, int rgnop)
{
	PRegionSysData r2;
	int d;
	Bool ok = true;

	r2 = GET_REGION(other_region);

	if ( rgnop == rgnopCopy ) {
		if ( pREGION ) XDestroyRegion( pREGION );
		pREGION = XCreateRegion();
		XUnionRegion( pREGION, r2->region, pREGION);
		pAPERTURE = r2-> aperture;
		return true;
	}

	d = pAPERTURE - r2-> aperture;
	if ( d > 0 )
		XOffsetRegion( r2-> region, 0, d);
	else
		XOffsetRegion( pREGION, 0, -d);

	switch (rgnop) {
	case rgnopIntersect:
		XIntersectRegion( pREGION, r2->region, pREGION);
		break;
	case rgnopUnion:
		XUnionRegion( pREGION, r2->region, pREGION);
		break;
	case rgnopXor:
		XXorRegion( pREGION, r2->region, pREGION);
		break;
	case rgnopDiff:
		XSubtractRegion( pREGION, r2->region, pREGION);
		break;
	default:
		ok = false;
	}
	if ( d > 0 )
		XOffsetRegion( r2-> region, 0, -d);
	else
		pAPERTURE = r2-> aperture;

	return ok;
}

Bool
apc_region_point_inside( Handle self, Point p)
{
	return XPointInRegion( pREGION, p.x, pAPERTURE - p.y - 1);
}

int
apc_region_rect_inside( Handle self, Rect r)
{
	int res = XRectInRegion(
		pREGION,
		r. left, pAPERTURE - r. bottom - 1,
		r. right - r.left + 1, r.top - r.bottom + 1
	);
	switch (res) {
	case RectangleIn:   return rgnInside;
	case RectanglePart: return rgnPartially;
	default:	    return rgnOutside;
	}
}

Bool
apc_region_equals( Handle self, Handle other_region)
{
	return XEqualRegion( pREGION, GET_REGION(other_region)->region);
}

Bool
apc_region_is_empty( Handle self)
{
	return XEmptyRegion( pREGION );
}

Box
apc_region_get_box( Handle self)
{
	Box box;
	XRectangle xr;
	XClipBox( pREGION, &xr);
	box. x	    = xr. x;
	box. y	    = pAPERTURE - xr. height - xr.y;
	box. width  = xr. width;
	box. height = xr. height;
	return box;
}

#define REVERT(a)	(XX-> size. y - (a) - 1)
#define SORT(a,b)	{ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}

/* returns rect in X coordinates BUT without menuHeight deviation */
void
prima_gp_get_clip_rect( Handle self, XRectangle *cr, Bool for_internal_paints)
{
	DEFXX;
	XRectangle r;

	cr-> x = 0;
	cr-> y = 0;
	cr-> width = XX-> size.x;
	cr-> height = XX-> size.y;
	if ( XF_IN_PAINT(XX) && XX-> paint_region) {
		XClipBox( XX-> paint_region, &r);
		prima_rect_intersect( cr, &r);
	}
	if ( XX-> clip_rect. x != 0
		|| XX-> clip_rect. y != 0
		|| XX-> clip_rect. width != XX-> size.x
		|| XX-> clip_rect. height != XX-> size.y) {
		prima_rect_intersect( cr, &XX-> clip_rect);
	}

	if ( for_internal_paints) {
		cr-> x += XX-> btransform. x;
		cr-> y -= XX-> btransform. y;
	}
}

Rect
apc_gp_get_clip_rect( Handle self)
{
	DEFXX;
	XRectangle cr;
	Rect r;

	prima_gp_get_clip_rect( self, &cr, 0);
	r. left = cr. x;
	r. top = XX-> size. y - cr. y - 1;
	r. bottom = r. top - cr. height + 1;
	r. right = cr. x + cr. width - 1;
	return r;
}


Bool
apc_gp_set_clip_rect( Handle self, Rect clipRect)
{
	DEFXX;
	Region region;
	XRectangle r;

	if ( !XF_IN_PAINT(XX))
		return false;

	SORT( clipRect. left, clipRect. right);
	SORT( clipRect. bottom, clipRect. top);
	r. x = clipRect. left;
	r. y = REVERT( clipRect. top);
	r. width = clipRect. right - clipRect. left+1;
	r. height = clipRect. top - clipRect. bottom+1;
	XX-> clip_rect = r;
	XX-> clip_mask_extent. x = r. width;
	XX-> clip_mask_extent. y = r. height;
	region = XCreateRegion();
	XUnionRectWithRegion( &r, region, region);
	if ( XX-> paint_region)
		XIntersectRegion( region, XX-> paint_region, region);
	if ( XX-> btransform. x != 0 || XX-> btransform. y != 0) {
		XOffsetRegion( region, XX-> btransform. x, -XX-> btransform. y);
	}
	XSetRegion( DISP, XX-> gc, region);
	if ( XX-> flags. kill_current_region)
		XDestroyRegion( XX-> current_region);
	XX-> flags. kill_current_region = 1;
	XX-> current_region = region;
	XX-> flags. xft_clip = 0;
#ifdef USE_XFT
	if ( XX-> xft_drawable) prima_xft_update_region( self);
#endif
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	if ( XX-> argb_picture ) XRenderSetPictureClipRegion(DISP, XX->argb_picture, region);
#endif
	return true;
}

Bool
apc_gp_set_region( Handle self, Handle rgn)
{
	DEFXX;
	Region region;
	PRegionSysData r;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if (rgn == nilHandle) {
		Rect r;
		r. left   = 0;
		r. bottom = 0;
		r. right  = XX-> size. x - 1;
		r. top	  = XX-> size. y - 1;
		return apc_gp_set_clip_rect( self, r);
	}

	r = GET_REGION(rgn);

	XClipBox( r-> region, &XX-> clip_rect);
	XX-> clip_rect. y += XX-> size. y - r-> aperture;
	XX-> clip_mask_extent. x = XX-> clip_rect. width;
	XX-> clip_mask_extent. y = XX-> clip_rect. height;
	if ( XX-> clip_rect. width == 0 || XX-> clip_rect. height == 0) {
		Rect r;
		r. left   = -1;
		r. bottom = -1;
		r. right  = -1;
		r. top	  = -1;
		return apc_gp_set_clip_rect( self, r);
	}

	region = XCreateRegion();
	XUnionRegion( region, r-> region, region);
	/* offset region if drawable is buffered */
	XOffsetRegion( region, XX-> btransform. x, XX-> size.y - r-> aperture - XX-> btransform. y);
	/* otherwise ( and only otherwise ), and if there's a
		X11 clipping, intersect the region with it. X11 clipping
		must not mix with the buffer clipping */
	if (( !XX-> udrawable || XX-> udrawable == XX-> gdrawable) &&
		XX-> paint_region)
		XIntersectRegion( region, XX-> paint_region, region);
	XSetRegion( DISP, XX-> gc, region);
	if ( XX-> flags. kill_current_region)
		XDestroyRegion( XX-> current_region);
	XX-> flags. kill_current_region = 1;
	XX-> current_region = region;
	XX-> flags. xft_clip = 0;
#ifdef USE_XFT
	if ( XX-> xft_drawable) prima_xft_update_region( self);
#endif
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	if ( XX-> argb_picture ) XRenderSetPictureClipRegion(DISP, XX->argb_picture, region);
#endif
	return true;
}

Bool
apc_gp_get_region( Handle self, Handle rgn)
{
	DEFXX;
	GC gc;
	XGCValues gcv;
	int w, h;
	Pixmap pixmap;
	XImage * i;
	Image pi;
	Region rgn2;

	if ( !XF_IN_PAINT(XX)) return false;

	if ( !rgn)
		return XX-> clip_mask_extent. x != 0 && XX-> clip_mask_extent. y != 0;

	if ( XX-> clip_mask_extent. x == 0 || XX-> clip_mask_extent. y == 0)
		return false;

	w = XX-> clip_mask_extent. x;
	h = XX-> clip_mask_extent. y;

	pixmap = XCreatePixmap( DISP, guts.root, w, h,
		XF_LAYERED(XX) ? guts. argb_depth :
		( XT_IS_BITMAP(XX) ? 1 : guts. depth )
	);
	XCHECKPOINT;

	gcv. graphics_exposures = false;
	gcv. fill_style = FillSolid;
	gcv. foreground = 0;
	gcv. clip_y_origin = -XX-> clip_rect. y + XX-> btransform. y;
	gcv. clip_x_origin = -XX-> clip_rect. x - XX-> btransform. x;
	XCHECKPOINT;
	gc = XCreateGC( DISP, pixmap, GCGraphicsExposures|GCFillStyle|GCForeground|GCClipXOrigin|GCClipYOrigin, &gcv);
	XFillRectangle( DISP, pixmap, gc, 0, 0, w, h);
	XSetForeground( DISP, gc, 1 );
	XCopyGC( DISP, XX->gc, GCClipMask, gc);
	XFillRectangle( DISP, pixmap, gc, 0, 0, w, h);
	XFreeGC( DISP, gc);
	XCHECKPOINT;

	i = XGetImage( DISP, pixmap, 0, 0, w, h, 1, XYPixmap);
	XFreePixmap( DISP, pixmap);
	if ( !i ) {
		warn("Cannot query image");
		return false;
	}

	img_fill_dummy( &pi, w, h, imBW | imGrayScale, NULL, NULL);
	if ( !( pi.data = malloc(pi.lineSize * h))) {
		XDestroyImage( i);
		warn("Not enough memory");
		return false;
	}

	prima_copy_xybitmap( pi.data, (Byte*)i-> data, w, h, pi.lineSize, i-> bytes_per_line);
	XDestroyImage( i);

	rgn2 = prima_region_create((Handle) &pi);
	free(pi.data);

	if ( GET_REGION(rgn)-> region )
		XDestroyRegion(  GET_REGION(rgn)-> region );
	if ( rgn2 ) {
		XOffsetRegion( rgn2, XX-> clip_rect.x, 0);
		GET_REGION(rgn)-> aperture = XX-> size.y - XX-> clip_rect.y;
		GET_REGION(rgn)-> region = rgn2;
	} else {
		GET_REGION(rgn)-> aperture = 0;
		GET_REGION(rgn)-> region = XCreateRegion();
	}
	return true;
}

PRegionRec
apc_region_copy_rects( Handle self)
{
	int i, aperture;
	PRegionRec ret;
	Box *dst;
	BoxRec *src;
	REGION *region;

	region = (REGION*) pREGION;
	ret = malloc(sizeof(RegionRec) + sizeof(Box) * region->numRects);
	if ( ret == NULL ) {
		warn("Not enough memory\n");
		return NULL;
	}

	ret-> type = rgnRectangle;
	ret-> data. box. n_boxes = region-> numRects;
	src = region->rects;
	dst = ret-> data. box. boxes = (Box*) (((Byte*)ret) + sizeof(RegionRec));
	aperture = pAPERTURE;
	for ( i = 0; i < ret->data. box. n_boxes; i++, src++, dst++) {
		dst-> x = src-> x1;
		dst-> y = aperture - src-> y2;
		dst-> width  = src-> x2 - src->x1;
		dst-> height = src-> y2 - src->y1;
/*		printf("%d: %d %d %d %d => %d %d %d %d\n", aperture, src->x1, src->y1, src->x2, src->y2, dst->x, dst->y, dst-> width, dst->height); */
	}

	return ret;
}

