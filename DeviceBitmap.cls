object DeviceBitmap( Drawable)
{
   Bool monochrome;
   method Bool   begin_paint();
   method Bool   begin_paint_info();
   method void   done();
   method void   end_paint();
   method Handle icon();
   method Handle image();
   method Bool   get_monochrome();
   method void   init( HV * profile);
}
