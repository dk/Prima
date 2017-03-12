#include "unix/guts.h"
#include "Icon.h"

#define REGION GET_REGION(self)->region
#define HEIGHT GET_REGION(self)->height

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
	REGION = XCreateRegion();
	xr. x = 0;
	xr. y = 0;
	xr. width  = 1;
	xr. height = 1;
	XUnionRectWithRegion( &xr, REGION, REGION);
	XXorRegion( REGION, REGION, REGION);
	HEIGHT = 0;
	return true;
}

static Bool
rgn_rect(Handle self, Box r)
{
	XRectangle xr;
	REGION = XCreateRegion();
	xr. x = r. x;
	xr. y = 0;
	xr. width  = r. width;
	xr. height = r. height;
	XUnionRectWithRegion( &xr, REGION, REGION);
	HEIGHT = r.y + r.height;
	return true;
}

static Bool
rgn_polygon(Handle self, PolygonRegionRec * r)
{
	int i, max;
	XPoint * xp;

	if ( !( xp = malloc( sizeof(XPoint) * r->n_points ))) {
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
	REGION = XPolygonRegion( xp, r->n_points, r-> winding ? WindingRule : EvenOddRule );

	free( xp );
	return true;
}

static Bool
rgn_image(Handle self, Handle image)
{
	REGION = prima_region_create(image);

	if ( !REGION )
		REGION = XCreateRegion();
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
	if ( REGION ) {
		XDestroyRegion(REGION);
		REGION = NULL;
	}
	return true;
}

ApiHandle
apc_region_get_handle( Handle self)
{
	return (ApiHandle) REGION;
}

Bool
apc_region_offset( Handle self, int dx, int dy)
{
	XOffsetRegion(REGION, dx, -dy);
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
		if ( REGION ) XDestroyRegion( REGION );
		REGION = XCreateRegion();
		XUnionRegion( REGION, r2->region, REGION);
		HEIGHT = r2-> height;
		return true;
	}

	d = HEIGHT - r2-> height;
	if ( d > 0 ) 
		XOffsetRegion( r2-> region, 0, d);
	else
		XOffsetRegion( REGION, 0, -d);

	switch (rgnop) {
	case rgnopIntersect:
		XIntersectRegion( REGION, r2->region, REGION);
		break;
	case rgnopUnion:
		XUnionRegion( REGION, r2->region, REGION);
		break;
	case rgnopXor:
		XXorRegion( REGION, r2->region, REGION);
		break;
	case rgnopDiff:
		XSubtractRegion( REGION, r2->region, REGION);
		break;
	default:
		ok = false;
	}
	if ( d > 0 ) 
		XOffsetRegion( r2-> region, 0, -d);
	else
		HEIGHT = r2-> height;

	return ok;
}

Bool
apc_region_point_inside( Handle self, Point p)
{
	return XPointInRegion( REGION, p.x, HEIGHT - p.y - 1);
}

int
apc_region_rect_inside( Handle self, Rect r)
{
	int res = XRectInRegion(
		REGION,
		r. left, HEIGHT - r. bottom - 1,
		r. right - r.left + 1, r.top - r.bottom + 1
	);
	switch (res) {
	case RectangleIn:   return rgnInside;
	case RectanglePart: return rgnPartially;
	default:            return rgnOutside;
	}
}

Bool
apc_region_equals( Handle self, Handle other_region)
{
	return XEqualRegion( REGION, GET_REGION(other_region)->region);
}

Bool
apc_region_is_empty( Handle self)
{
	return XEmptyRegion( REGION );
}


Box
apc_region_get_box( Handle self)
{
	Box box;
	XRectangle xr;
	XClipBox( REGION, &xr);
	box. x      = xr. x;
	box. y      = HEIGHT - xr. height - xr.y;
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
	PImage img;
	PRegionSysData r;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if (rgn == nilHandle) {
		Rect r;
		r. left   = 0;
		r. bottom = 0;
		r. right  = XX-> size. x - 1;
		r. top    = XX-> size. y - 1;
		return apc_gp_set_clip_rect( self, r);
	}

	r = GET_REGION(rgn);

	XClipBox( r-> region, &XX-> clip_rect);
	XX-> clip_rect. y += XX-> size. y - r-> height;
	XX-> clip_mask_extent. x = XX-> clip_rect. width;
	XX-> clip_mask_extent. y = XX-> clip_rect. height;
	if ( XX-> clip_rect. width == 0 || XX-> clip_rect. height == 0) {
		Rect r;
		r. left   = -1;
		r. bottom = -1;
		r. right  = -1;
		r. top    = -1;
		return apc_gp_set_clip_rect( self, r);
	}

	region = XCreateRegion();
	XUnionRegion( region, r-> region, region);
	/* offset region if drawable is buffered */
	XOffsetRegion( region, XX-> btransform. x, XX-> size.y - r-> height - XX-> btransform. y);
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
	Bool bitmap, layered;
	GC gc;
	XGCValues gcv;

	if ( !XF_IN_PAINT(XX)) return false;

	if ( !rgn) 
		return XX-> clip_mask_extent. x != 0 && XX-> clip_mask_extent. y != 0;
		
	if ( XX-> clip_mask_extent. x == 0 || XX-> clip_mask_extent. y == 0)
		return false;

	bitmap  = XT_IS_BITMAP(XX) ? true : false;
	layered = XF_LAYERED(XX) ? true : false;

	CIcon( rgn)-> create_empty_icon(
		rgn, XX-> clip_mask_extent. x, XX-> clip_mask_extent. y, 
		layered ? 24 : (bitmap ? imBW : guts. qdepth),
		layered ? 8 : 1
	);
	CIcon( rgn)-> begin_paint( rgn);
	XCHECKPOINT;

	gcv. graphics_exposures = false;
	gcv. fill_style = FillSolid;
	gcv. foreground = (layered || bitmap) ? 0xffffffff : guts.monochromeMap[1];
	gcv. clip_y_origin = XX-> clip_mask_extent. y - XX-> size. y;
	gc = XCreateGC( DISP, XX->gdrawable, GCGraphicsExposures|GCFillStyle|GCForeground|GCClipYOrigin, &gcv);
	XCopyGC( DISP, XX->gc, GCClipMask, gc);
	XFillRectangle( DISP, X(rgn)-> gdrawable, gc, 0, 0, XX-> clip_mask_extent.x, XX-> clip_mask_extent.y);
	XFreeGC( DISP, gc);
	XCHECKPOINT;

	CIcon( rgn)-> end_paint( rgn);
	if ( !bitmap || layered) CIcon( rgn)-> set_type( rgn, imBW);
	XCHECKPOINT;

	return true;
}

Box
apc_gp_get_region_box( Handle self)
{
	DEFXX;
	Box box = {0,0,0,0};

	if ( !XF_IN_PAINT(XX)) return box;
	if ( XX-> clip_mask_extent. x == 0 || XX-> clip_mask_extent. y == 0)
		return box;
	
	box. x      = XX-> clip_rect. x;
	box. y      = XX-> size. y - XX-> clip_mask_extent. y - XX-> clip_rect. y;
	box. width  = XX-> clip_mask_extent. x;
	box. height = XX-> clip_mask_extent. y;

	return box;
}

