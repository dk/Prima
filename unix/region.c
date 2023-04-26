#include "unix/guts.h"
#include "Icon.h"

#define pREGION GET_REGION(self)->region
#define pAPERTURE GET_REGION(self)->aperture

Bool
apc_region_create( Handle self, PRegionRec rec)
{
	int i, aperture, count;
	Box *rr, *r;
	pREGION = XCreateRegion();

	r = rec ? rec->boxes : NULL;
	count = rec ? rec->n_boxes : 0;

	if ( r ) {
		aperture = r->y + r->height;
		for ( i = 0, rr = r; i < count; i++, rr++) {
			if ( aperture < rr->y + rr->height)
				aperture = rr->y + rr->height;
		}
	} else
		aperture = 0;

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
	CLIP_ARGB_PICTURE(XX->argb_picture, region);
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

	if (rgn == NULL_HANDLE) {
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
	CLIP_ARGB_PICTURE(XX->argb_picture, region);
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
	PRegionRec rgnrec2;
	Region rgn2 = (Region) 0;

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

	prima_copy_1bit_ximage( pi.data, i, false);
	XDestroyImage( i);

	rgnrec2 = img_region_mask(( Handle) &pi);
	free(pi.data);
	if ( rgnrec2) {
		int i, count = rgnrec2->n_boxes, aperture = pi.h;
		Box *r = rgnrec2->boxes;
		rgn2 = XCreateRegion();
		for ( i = 0; i < count; i++, r++) {
			XRectangle xr;
			xr. x = r-> x;
			xr. y = aperture - r->y - r->height;
			xr. width  = r-> width;
			xr. height = r-> height;
			XUnionRectWithRegion( &xr, rgn2, rgn2);
		}
		free(rgnrec2);
	}

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
	if ( !( ret = img_region_new( region->numRects )))
		return NULL;

	ret-> n_boxes = region-> numRects;
	src = region->rects;
	dst = ret-> boxes;
	aperture = pAPERTURE;
	for ( i = 0; i < ret->n_boxes; i++, src++, dst++) {
		dst-> x = src-> x1;
		dst-> y = aperture - src-> y2;
		dst-> width  = src-> x2 - src->x1;
		dst-> height = src-> y2 - src->y1;
		/* printf("%d: %d %d %d %d => %d %d %d %d\n", aperture, src->x1, src->y1, src->x2, src->y2, dst->x, dst->y, dst-> width, dst->height);  */
	}

	return ret;
}

