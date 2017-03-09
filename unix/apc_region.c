#include "unix/guts.h"
#include "Image.h"

#define REGION GET_REGION(self)->region
#define TOP GET_REGION(self)->top

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
	if ( !rdata) return None;

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
	return true;
}

static Bool
rgn_rect(Handle self, Rect2 r)
{
	XRectangle xr;
	xr. x = r. x;
	xr. y = 0;
	xr. width  = r. width;
	xr. height = r. height;
	XUnionRectWithRegion( &xr, REGION, REGION);
	TOP = r.height + r.y;
	return true;
}

static Bool
rgn_ellipse(Handle self, Rect2 ellipse)
{
	return true;
}

static Bool
rgn_polyline(Handle self, PolylineRegionRec * r)
{
	return true;
}

static Bool
rgn_image(Handle self, Handle image)
{
	return true;
}

Bool
apc_region_create( Handle self, PRegionRec rec)
{
	Bool ok;
	
	REGION = XCreateRegion();
	
	switch( rec-> type ) {
	case rgnEmpty:
		ok = rgn_empty(self);
		break;
	case rgnRectangle:
		ok = rgn_rect(self, rec->data.box);
		break;
	case rgnEllipse:
		ok = rgn_ellipse(self, rec->data.box);
		break;
	case rgnPolyline:
		ok = rgn_polyline(self, &rec->data.polyline);
		break;
	case rgnImage:
		ok = rgn_image(self, rec->data.image);
		break;
	default:
		ok = false;
	}
	
	if ( !ok ) {
		XDestroyRegion(REGION);
		REGION = NULL;
	}

	return true;
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

Bool
apc_region_offset( Handle self, int dx, int dy)
{
	return false;
}

Bool
apc_region_combine( Handle self, Handle other_region, int rgnop)
{
	PRegionSysData r2;
	int d;
	
	r2 = GET_REGION(other_region);
	d = TOP - r2-> top;

	switch (rgnop) {
	case rgnopCopy:
		break;
	case rgnopIntersect:
		break;
	case rgnopUnion:
		if ( d > 0 ) {
			XOffsetRegion( r2-> region, 0, d);
			XUnionRegion( REGION, r2->region, REGION);
			XOffsetRegion( r2-> region, 0, -d);
		} else {
			XOffsetRegion( REGION, 0, -d);
			XUnionRegion( REGION, r2->region, REGION);
			TOP = r2-> top;
		}
		break;
	case rgnopXor:
		break;
	case rgnopDiff:
		break;
	}
	return false;
}

Bool
apc_region_point_inside( Handle self, Point p)
{
	return false;
}

int
apc_region_rect_inside( Handle self, Rect r)
{
	return false;
}

Bool
apc_region_equals( Handle self, Handle other_region)
{
	return false;
}

Rect
apc_region_get_box( Handle self)
{
	return (Rect){0,0,0,0};
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
	EMPTY:
		r. left   = 0;
		r. bottom = 0;
		r. right  = XX-> size. x;
		r. top    = XX-> size. y;
		return apc_gp_set_clip_rect( self, r);
	}

	r = GET_REGION(rgn);

	XClipBox( r-> region, &XX-> clip_rect);
	XX-> clip_rect. y = REVERT( r-> top - XX-> clip_rect. y);
	XX-> clip_mask_extent. x = XX-> clip_rect. width;
	XX-> clip_mask_extent. y = XX-> clip_rect. height;
	if ( XX-> clip_rect. width == 0 || XX-> clip_rect. height == 0) goto EMPTY;

	region = XCreateRegion();
	XUnionRegion( region, r-> region, region);
	/* offset region if drawable is buffered */
	XOffsetRegion( region, XX-> btransform. x, XX-> size.y - r-> top - XX-> btransform. y);
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
	int depth;

	if ( !XF_IN_PAINT(XX)) return false;

	if ( !rgn) 
		return XX-> clip_mask_extent. x != 0 && XX-> clip_mask_extent. y != 0;
		
	if ( XX-> clip_mask_extent. x == 0 || XX-> clip_mask_extent. y == 0)
		return false;

	XSetClipOrigin( DISP, XX-> gc, 0, 0);

	depth = XT_IS_BITMAP(XX) ? 1 : guts. qdepth;
	CImage( rgn)-> create_empty( rgn, XX-> clip_mask_extent. x, XX-> clip_mask_extent. y, depth);
	CImage( rgn)-> begin_paint( rgn);
	XCHECKPOINT;
	XSetForeground( DISP, XX-> gc, ( depth == 1) ? 1 : guts. monochromeMap[1]);
	XFillRectangle( DISP, X(rgn)-> gdrawable, XX-> gc, 0, 0, XX-> clip_mask_extent.x + 1, XX-> clip_mask_extent.y + 1);
	XCHECKPOINT;
	XX-> flags. brush_fore = 0;
	CImage( rgn)-> end_paint( rgn);
	XCHECKPOINT;
	if ( depth != 1) CImage( rgn)-> set_type( rgn, imBW);

	XSetClipOrigin( DISP, XX-> gc, XX-> btransform.x, 
		- XX-> btransform. y + XX-> size. y - XX-> clip_mask_extent.y);
	return true;
}

