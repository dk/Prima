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
rgn_ellipse(Handle self, Point d)
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
		ok = rgn_rect(self, rec->data.rectangle);
		break;
	case rgnEllipse:
		ok = rgn_ellipse(self, rec->data.ellipse);
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
