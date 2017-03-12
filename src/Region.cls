use Types;
object Prima::Region( Prima::Component)
{
	method   Box       box() => apc_region_get_box;
	method   Bool      combine(Handle other_region, int rgnop = rgnopCopy);
	method   void      done();
	method   SV      * get_handle();
	method   Bool      equals(Handle other_region);
	method   void      init( HV * profile);
	method   Bool      is_empty() => apc_region_is_empty;
	method   Bool      offset(int dx, int dy) => apc_region_offset;
	method   Bool      point_inside(Point p) => apc_region_point_inside;
	method   int       rect_inside(Rect p) => apc_region_rect_inside;
}
