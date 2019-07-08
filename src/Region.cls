use Types;
object Prima::Region( Prima::Component)
{
	PRegionRec rects;
	method   Box       box() => apc_region_get_box;
	method   Bool      combine(Handle other_region, int rgnop = rgnopCopy);
	method   void      done();
	method   SV      * get_handle();
	method   SV      * get_boxes();
	method   Bool      equals(Handle other_region);
	method   void      init( HV * profile);
	method   Bool      is_empty() => apc_region_is_empty;
	method   Bool      offset(int dx, int dy);
	method   Bool      point_inside(Point p) => apc_region_point_inside;
	method   int       rect_inside(Rect p) => apc_region_rect_inside;
	c_only   PRegionRec update_change(Bool disown);
	c_only   Handle    create_from_data( PRegionRec data );
}
