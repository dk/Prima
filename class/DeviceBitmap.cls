object Prima::DeviceBitmap( Prima::Drawable)
{
	int type;
	property int type;
	property int maskPixel(int x, int y);
	method Bool   begin_paint();
	method Bool   begin_paint_info();
	method void   done();
	method void   end_paint();
	method int    get_effective_rop(int rop);
	method Handle icon();
	method Handle image();
	method SV *   get_handle();
	method int    get_paint_state();
	method void   init( HV * profile);
}
