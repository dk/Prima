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
	method UV     add_notification( char * name, SV * subroutine, Handle referer = nilHandle, int index = -1);
	method Bool   arc( int x, int y, int dX, int dY, double startAngle, double endAngle);
	method Bool   bar(int x1, int y1, int x2, int y2);
	method Bool   bars(SV * rects);
	method Handle bitmap();
	method Bool   begin_paint();
	method Bool   begin_paint_info();
	method Bool   chord( int x, int y, int dX, int dY, double startAngle, double endAngle);
	static SV   * codecs( SV * dummy, int codecID = -1);
	method Bool   clear(int x1=-1, int y1=-1, int x2=-1, int y2=-1);
	c_only void   create_empty( int width, int height, int type);
	method void   done();
	method Handle dup();
	method Bool   ellipse(int x, int y, int dX, int dY);
	method void   end_paint();
	method void   end_paint_info();
	method Handle extract( int x, int y, int width, int height);
	method Bool   fill_chord( int x, int y, int dX, int dY, double startAngle, double endAngle);
	method Bool   fill_ellipse(int x, int y, int dX, int dY);
	method Bool   fillpoly( SV * points);
	method Bool   fill_sector( int x, int y, int dX, int dY, double startAngle, double endAngle);
	method Bool   flood_fill( int x, int y, Color color, Bool singleBorder = 1);
	method int    get_bpp();
	method SV   * get_handle();
	method Color  get_nearest_color( Color color);
	c_only void   handle_event( PEvent event);
	method void   init( HV * profile);
	method Bool   line(int x1, int y1, int x2, int y2);
	method Bool   lines( SV * points);
	weird  PList  load( SV * className, char *filename, HV * profile);
	c_only void   make_empty();
	method void   map( Color color);
	method void   mirror( Bool vertically );
	import SV *   notification_types();
	method Bool   polyline( SV * points);
	import SV *   profile_default ();
	method void   premultiply_alpha(SV * alpha);
	method Bool   put_image_indirect( Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop);
	method void   remove_notification( UV id);
	method Bool   rotate( double degrees );
	method Bool   rectangle(int x1, int y1, int x2, int y2);
	method void   resample( double srcLo, double srcHi, double dstLo, double dstHi);
	weird  int    save( SV *className, char *filename, HV * profile);
	method Bool   sector( int x, int y, int dX, int dY, double startAngle, double endAngle);
	method void   set( HV * profile);
	c_only void   reset( int type, RGBColor * palette, int palSize);
	c_only void   stretch( int width, int height);
	method Bool   transform( double a, double b, double c, double d );
	c_only void   update_change();
}
