local @ImageStats double[7];

object Prima::Image( Prima::Drawable)
{
	# variables
	int            type;              # imXXXX
	int            lineSize;          # rounded to 32-bit boundary
	int            dataSize;          #  == lineSize * h
	int            conversion;        # ictXXXX
	int            scaling;           # istXXXX
	unsigned char *data;
	ImageStats     stats;
	int            statsCache;
	Point          resolution;
	int            eventMask1;
	int            eventMask2;
	PRegionRec     regionData;
	int            updateLock;
	int            extraROP;
	void          *loading_session;
	void          *saving_session;

	property Bool   antialias;
	property Rect   clipRect;
	property int    conversion;
	property SV *   data;
	property int    lineSize;
	property SV *   palette;
	property SV *   pixel( int x, int y);
	property Bool   preserveType;
	property Handle region;
	property Point  resolution;
	property int    rop;
	property int    scaling;
	property Point  size;
	property double stats( int index);
	property int    type;

	# methods
	method UV     add_notification( char * name, SV * subroutine, Handle referer = NULL_HANDLE, int index = -1);
	method Bool   bar(double x1, double y1, double x2, double y2);
	method Bool   bar_alpha(int alpha, int x1=-1, int y1=-1, int x2=-1, int y2=-1);
	method Bool   bars(SV * rects);
	method Handle bitmap();
	c_only Bool   begin_font_query();
	method Bool   begin_paint();
	method Bool   begin_paint_info();
	method Bool   can_draw_alpha();
	static SV   * codecs( SV * dummy, int codecID = -1);
	method Handle convert_to_icon( int maskType, SV * mask_fill );
	method Bool   clear(double x1=-1, double y1=-1, double x2=-1, double y2=-1);
	c_only void   create_empty( int width, int height, int type);
	method void   done();
	method Handle dup();
	c_only void   end_font_query();
	method void   end_paint();
	method void   end_paint_info();
	method Handle extract( int x, int y, int width, int height);
	method Bool   fillpoly( SV * podoubles);
	method Bool   flood_fill( int x, int y, Color color, Bool singleBorder = 1);
	static Font*  font_match( SV * dummy, Font * source, Font * dest, Bool pick = true);
	method SV*    fonts( char * name = "", char * encoding = "");
	method SV*    font_encodings();
	method int    get_bpp();
	method Font   get_font();
	method SV   * get_handle();
	method Color  get_nearest_color( Color color);
	method Bool   graphic_context_push();
	method Bool   graphic_context_pop();
	c_only void   handle_event( PEvent event);
	method void   init( HV * profile);
	method Bool   line(double x1, double y1, double x2, double y2);
	method Bool   lines( SV * points);
	weird  PList  load( SV * className, SV *filename, HV * profile);
	c_only void   make_empty();
	method void   map( Color color);
	c_only Bool   matrix_transform( Matrix matrix, ColorPixel fill, Point *aperture);
	method void   mirror( Bool vertically );
	import SV *   notification_types();
	method Bool   polyline( SV * points);
	import SV *   profile_default ();
	method void   premultiply_alpha(SV * alpha);
	method Bool   put_image_indirect( Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop = ropDefault);
	method void   remove_notification( UV id);
	method Bool   rotate( double degrees, SV * fill = NULL_SV );
	method Bool   rectangle(double x1, double y1, double x2, double y2);
	method void   resample( double srcLo, double srcHi, double dstLo, double dstHi);
	weird  int    save( SV *className, SV *filename, HV * profile);
	method void   set( HV * profile);
	method void   set_font( Font font);
	c_only void   reset( int type, RGBColor * palette, int palSize);
	c_only void   stretch( int width, int height);
	method Bool   text_out( SV * text, int x, int y, int from = 0, int len = -1);
	method SV*    text_shape( SV * text, HV * profile);
	method Bool   transform( HV * options);
	c_only void   update_change();

	c_only Bool   assert_drawing_mode( int mode );
	c_only void   begin_preserve_type( PImagePreserveTypeRec save);
	c_only void   end_preserve_type( PImagePreserveTypeRec save);
	c_only void   restore_font( SaveFont *f);
	c_only Bool   switch_font( SaveFont *f, uint16_t fid);
}
