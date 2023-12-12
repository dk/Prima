use Types;

object Prima::Drawable ( Prima::Component)
{
	Font         font;
	int          w;
	int          h;
	PRGBColor    palette;       	       # logical palette
	int          palSize;               # logical palette size
	Bool         antialias;
	int          alpha;
	Handle       fillPatternImage;

	DrawablePaintState current_state;
	DrawablePaintState saved_state;

	property     int           alpha;
	property     Bool          antialias;
	property     Color         backColor;
	property     Color         color;
	property     Rect          clipRect;
	property     int           fillMode;
	property     SV *          fillPattern;
	property     Point         fillPatternOffset;
	property     int           height;
	property     SV *          lineEnd;
	property     SV *          lineEndIndex(int index);
	property     int           lineJoin;
	property     SV *          linePattern;
	property     double        lineWidth;
	property     double        miterLimit;
	property     SV *          palette;
	property     SV *          pixel( int x, int y);
	property     Handle        region;
	property     Point         resolution;
	property     int           rop;
	property     int           rop2;
	property     Point         size;
	property     Bool          textOpaque;
	property     Bool          textOutBaseline;
	property     int           width;

	# globals
	method void  init( HV * profile);
	method void  done();
	method void  cleanup();
	method Bool  begin_paint();
	method Bool  begin_paint_info();
	method void  end_paint();
	method void  end_paint_info();
	static Font* font_match( char * dummy, Font * source, Font * dest, Bool pick = true);
	c_only Bool  font_add( Font * source, Font * dest);
	import SV *  profile_default();

	method void  set( HV * profile);
	method void  set_font( Font font);
	method void  set_matrix( SV * matrix);

	method Bool  can_draw_alpha() => apc_gp_can_draw_alpha;
	method int   get_bpp();
	method int   get_effective_rop(int rop);
	method Font  get_font();
	method SV *  get_font_abc( int first = -1, int last = -1, int flags = 0);
	method SV *  get_font_def( int first = -1, int last = -1, int flags = 0);
	method SV *  get_font_languages();
	method SV *  get_font_ranges();
	method SV *  get_handle();
	method SV *  get_matrix();
	method Color get_nearest_color( Color color);
	method int   get_paint_state();
	method SV *  get_physical_palette();
	method int   get_text_width( SV * text, int flags = 0, int from = 0, int len = -1);
	method SV *  get_text_box( SV * text, int from = 0, int len = -1);
	method SV *  render_glyph( int index, HV * profile);
	static SV *  render_polyline( SV * obj, SV * points, HV * profile);
	import Handle render_pattern( SV * obj, SV * pattern, HV * profile);
	static SV *  render_spline( SV * obj, SV * points, HV * profile);
	method Bool  graphic_context_push();
	method Bool  graphic_context_pop();

	# operators
	method Bool  arc( double x, double y, double dX, double dY, double startAngle, double endAngle);
	method Bool  bar(double x1, double y1, double x2, double y2);
	method Bool  bar_alpha(int alpha, int x1=-1, int y1=-1, int x2=-1, int y2=-1)        => apc_gp_alpha;
	method Bool  bars(SV * rects);
	method Bool  chord( double x, double y, double dX, double dY, double startAngle, double endAngle);
	method Bool  clear(double x1=-1, double y1=-1, double x2=-1, double y2=-1);
	method Bool  ellipse(double x, double y, double dX, double dY);
	method Bool  fill_chord( double x, double y, double dX, double dY, double startAngle, double endAngle);
	method Bool  fill_ellipse(double x, double y, double dX, double dY);
	method Bool  fillpoly( SV * points);
	method Bool  fill_sector( double x, double y, double dX, double dY, double startAngle, double endAngle);
	method Bool  flood_fill( int x, int y, Color color, Bool singleBorder = 1) => apc_gp_flood_fill;
	method Bool  line(double x1, double y1, double x2, double y2);
	method Bool  lines( SV * points);
	method Bool  polyline( SV * points);
	method Bool  put_image_indirect( Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop = ropDefault);
	method Bool  rectangle(double x1, double y1, double x2, double y2);
	method Bool  sector( double x, double y, double dX, double dY, double startAngle, double endAngle);
	method Bool  text_out( SV * text, int x, int y, int from = 0, int len = -1);
	method SV*   text_shape( SV * text, HV * profile);
	method SV*   text_wrap( SV * text, int width, int options = twDefault, int tabIndent = 8, int from = 0, int len = -1, SV * glyphs = NULL_SV);

	# font_abc caches
	PFontABC font_abc_ascii;
	PList    font_abc_unicode;
	PList    font_abc_glyphs;
	unsigned long * font_abc_glyphs_ranges;
	int             font_abc_glyphs_n_ranges;

	# misc stuff
	NPoint   trig_cache;
	c_only   NPoint trig_cache();
	c_only   void   calculate_text_box( int advance, Bool text_out_baseline, Point overhangs, Point * pt);
	c_only   int*   do_text_wrap( TextWrapRec * tw, GlyphWrapRec * gw, uint16_t * log2vis);
	c_only   Bool   assert_drawing_mode( int mode );

}
