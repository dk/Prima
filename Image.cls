local @ImageStats double[6];

object Image( Drawable)
{
   # variables
   int            type;              # imXXXX
   int            lineSize;          # rounded to 32-bit boundary
   int            dataSize;          #  == lineSize * h
   int            conversion;        # ictXXXX
   unsigned char *data;
   int            status;
   ImageStats     stats;
   int            statsCache;

   # methods
   method Handle bitmap();
   method Bool   begin_paint();
   method Bool   begin_paint_info();
   c_only void   create_empty( int width, int height, int type);
   method void   done();
   method Handle dup();
   method void   end_paint();
   method void   end_paint_info();
   method Handle extract( int x, int y, int width, int height);
   method int    get_bpp();
   method int    get_conversion();
   method SV   * get_data();
   method SV   * get_handle();
   method char * get_status_string();
   method SV   * get_palette();
   method Color  get_pixel(int x, int y);
   method Bool   get_preserve_type();
   method Bool   get_h_scaling();
   method Bool   get_v_scaling();
   method double get_stats( int index);
   method int    get_type();
   method void   init( HV * profile);
   method Bool   load( char *filename, int index = -1);
   c_only void   make_empty();
   import SV *   profile_default ();
   method void   resample( double srcLo, double srcHi, double dstLo, double dstHi);
   method Bool   save( char *filename, HV *options);
   method void   set( HV * profile);
   method void   set_conversion( int conversion);
   method void   set_data( SV * data);
   method void   set_height( int height);
   method void   set_palette( SV * palette);
   method void   set_pixel(int x, int y, Color color);
   method void   set_preserve_type( Bool preserveType);
   method void   set_h_scaling( Bool scaling);
   method void   set_v_scaling( Bool scaling);
   method void   set_size( int width, int height);
   method void   set_stats( double value, int index);
   import void   set_type( int type);
   method void   set_width( int width);
   c_only void   reset( int type, SV * palette);
   c_only void   stretch( int width, int height);
   c_only void   update_change();
}
