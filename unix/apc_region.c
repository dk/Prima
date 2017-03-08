#include "unix/guts.h"
#include "Image.h"

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
