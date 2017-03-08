#include "unix/guts.h"
#include "Image.h"

#define GET_REGION(obj) ((PUnixSysData)(PComponent((obj))-> sysData))->region
#define REGION GET_REGION(self)

static Bool
rgn_empty(Handle self)
{
    return true;
}

static Bool
rgn_rects(Handle self, RectRegionRec * r)
{
    int i;
    Rect * pr;
    for ( i = 0, pr = r->rects; i < r->n_rects; i++, pr++) {
        XRectangle xr;
        xr. x = pr-> left;
        xr. width  = pr-> right - pr-> left;
        xr. height = pr-> top - pr-> bottom;
		region = XCreateRegion();
		XUnionRectWithRegion( &xr, region, region);
    }
    return true;
}

static Bool
rgn_ellipse(Handle self, EllipticRegionRec * r)
{
    return true;
}

static Bool
rgn_polyline(Handle self, PolylineRegionRec * r)
{
    return true;
}

static Bool
rgn_image(Handle self, ImageRegionRec * r)
{
    return true;
}

Bool
apc_region_create( Handle self, PRegionRec rec)
{
    Bool ok;

    REGION = XCreateRegion();

    switch( rec-> type ) {
    case rgnEmpty:    ok = rgn_empty(self);
    case rgnRects:    ok = rgn_rects(self, &rec->rects);
    case rgnEllipse:  ok = rgn_ellipse(self, &rec->elliptic);
    case rgnPolyline: ok = rgn_polyline(self, &rec->polyline);
    case rgnImage:    ok = rgn_image(self, &rec->image);
    default:          ok = false;
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
