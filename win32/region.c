#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "win32\win32guts.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

Bool
apc_region_create( Handle self, PRegionRec rec)
{
	return true;
}

Bool
apc_region_destroy( Handle self)
{
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

Box
apc_region_get_box( Handle self)
{
	return (Box){0,0,0,0};
}

Box
apc_gp_get_region_box( Handle self)
{
	DEFXX;
	Box box = {0,0,0,0};
	return box;
}

ApiHandle
apc_region_get_handle( Handle self)
{
	return NULL;
}

#ifdef __cplusplus
}
#endif
