#include "apricot.h"
#include "Drawable.h"
#include "Icon.h"
#include "Region.h"
#include <Drawable.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)
#define var (( PDrawable) self)

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER(fail)     if ( !inPaint) if ( !my-> begin_paint_info( self)) return (fail)
#define gpLEAVE           if ( !inPaint) my-> end_paint_info( self)

void
Drawable_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	apc_gp_init( self);
	var-> w = var-> h = 0;
	my-> set_color        ( self, pget_i ( color));
	my-> set_backColor    ( self, pget_i ( backColor));
	my-> set_fillMode     ( self, pget_i ( fillMode));
	my-> set_fillPattern  ( self, pget_sv( fillPattern));
	my-> set_lineEnd      ( self, pget_i ( lineEnd));
	my-> set_lineJoin     ( self, pget_i ( lineJoin));
	my-> set_linePattern  ( self, pget_sv( linePattern));
	my-> set_lineWidth    ( self, pget_i ( lineWidth));
	my-> set_miterLimit   ( self, pget_i ( miterLimit));
	my-> set_region       ( self, pget_H ( region));
	my-> set_rop          ( self, pget_i ( rop));
	my-> set_rop2         ( self, pget_i ( rop2));
	my-> set_textOpaque   ( self, pget_B ( textOpaque));
	my-> set_textOutBaseline( self, pget_B ( textOutBaseline));
	if ( pexist( translate))
	{
		AV * av;
		Point tr;
		SV ** holder, *sv;
		
		sv = pget_sv( translate);
		if ( sv && SvOK(sv) && SvROK(sv) && SvTYPE(av = (AV*)SvRV(sv)) == SVt_PVAV && av_len(av) == 1) {
			tr.x = tr.y = 0;
			holder = av_fetch( av, 0, 0);
			if ( holder) tr.x = SvIV( *holder); else warn("Array panic on 'translate'");
			holder = av_fetch( av, 1, 0);
			if ( holder) tr.y = SvIV( *holder); else warn("Array panic on 'translate'");
			my-> set_translate( self, tr);
		} else
			warn("Array panic on 'translate'");

		sv = pget_sv( fillPatternOffset);
		if ( sv && SvOK(sv) && SvROK(sv) && SvTYPE(av = (AV*)SvRV(sv)) == SVt_PVAV && av_len(av) == 1) {
			tr.x = tr.y = 0;
			holder = av_fetch( av, 0, 0);
			if ( holder) tr.x = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
			holder = av_fetch( av, 1, 0);
			if ( holder) tr.y = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
			my-> set_fillPatternOffset( self, tr );
		} else
			warn("Array panic on 'fillPatternOffset'");
	}
	SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::init");
	my-> set_font( self, Font_buffer);
	my-> set_palette( self, pget_sv( palette));
	CORE_INIT_TRANSIENT(Drawable);
}

static void
clear_font_abc_caches( Handle self)
{
	PList u;
	if (( u = var-> font_abc_unicode)) {
		int i;
		for ( i = 0; i < u-> count; i += 2)
			free(( void*) u-> items[ i + 1]);
		plist_destroy( u);
		var-> font_abc_unicode = nil;
	}
	if ( var-> font_abc_ascii) {
		free( var-> font_abc_ascii);
		var-> font_abc_ascii = nil;
	}
}

void
Drawable_done( Handle self)
{
	clear_font_abc_caches( self);
	apc_gp_done( self);
	inherited done( self);
}

void
Drawable_cleanup( Handle self)
{
	if ( is_opt( optInDrawInfo))
		my-> end_paint_info( self);
	if ( is_opt( optInDraw))
		my-> end_paint( self);
	inherited cleanup( self);
}

Bool
Drawable_begin_paint( Handle self)
{
	if ( var-> stage > csFrozen) return false;
	if ( is_opt( optInDrawInfo)) my-> end_paint_info( self);
	opt_set( optInDraw);
	return true;
}

void
Drawable_end_paint( Handle self)
{
	clear_font_abc_caches( self);
	opt_clear( optInDraw);
}

Bool
Drawable_begin_paint_info( Handle self)
{
	if ( var-> stage > csFrozen) return false;
	if ( is_opt( optInDraw))     return true;
	if ( is_opt( optInDrawInfo)) return false;
	opt_set( optInDrawInfo);
	return true;
}

void
Drawable_end_paint_info( Handle self)
{
	clear_font_abc_caches( self);
	opt_clear( optInDrawInfo);
}

void
Drawable_set( Handle self, HV * profile)
{
	dPROFILE;
	if ( pexist( font))
	{
		SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::set");
		my-> set_font( self, Font_buffer);
		pdelete( font);
	}
	if ( pexist( translate))
	{
		AV * av = ( AV *) SvRV( pget_sv( translate));
		Point tr = {0,0};
		SV ** holder = av_fetch( av, 0, 0);
		if ( holder) tr.x = SvIV( *holder); else warn("Array panic on 'translate'");
		holder = av_fetch( av, 1, 0);
		if ( holder) tr.y = SvIV( *holder); else warn("Array panic on 'translate'");
		my-> set_translate( self, tr);
		pdelete( translate);
	}
	if ( pexist( width) && pexist( height)) {
		Point size;
		size. x = pget_i( width);
		size. y = pget_i( height);
		my-> set_size( self, size);
		pdelete( width);
		pdelete( height);
	}
	if ( pexist( fillPatternOffset))
	{
		AV * av = ( AV *) SvRV( pget_sv( fillPatternOffset));
		Point fpo = {0,0};
		SV ** holder = av_fetch( av, 0, 0);
		if ( holder) fpo.x = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
		holder = av_fetch( av, 1, 0);
		if ( holder) fpo.y = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
		my-> set_fillPatternOffset( self, fpo);
		pdelete( fillPatternOffset);
	}
	inherited set( self, profile);
}


Font *
Drawable_font_match( char * dummy, Font * source, Font * dest, Bool pick)
{
	if ( pick)
		apc_font_pick( nilHandle, source, dest);
	else
		Drawable_font_add( nilHandle, source, dest);
	return dest;
}

Bool
Drawable_font_add( Handle self, Font * source, Font * dest)
{
	Bool useHeight = !source-> undef. height;
	Bool useWidth  = !source-> undef. width;
	Bool useSize   = !source-> undef. size;
	Bool usePitch  = !source-> undef. pitch;
	Bool useStyle  = !source-> undef. style;
	Bool useDir    = !source-> undef. direction;
	Bool useName   = !source-> undef. name;
	Bool useVec    = !source-> undef. vector;
	Bool useEnc    = !source-> undef. encoding;

	/* assignning values */
	if ( dest != source) {
		dest-> undef = source-> undef;
		if ( useHeight) dest-> height    = source-> height;
		if ( useWidth ) dest-> width     = source-> width;
		if ( useDir   ) dest-> direction = source-> direction;
		if ( useStyle ) dest-> style     = source-> style;
		if ( usePitch ) dest-> pitch     = source-> pitch;
		if ( useSize  ) dest-> size      = source-> size;
		if ( useVec   ) dest-> vector    = source-> vector;
		if ( useName  ) strcpy( dest-> name, source-> name);
		if ( useEnc   ) strcpy( dest-> encoding, source-> encoding);
	}

	/* nulling dependencies */
	if ( !useHeight && useSize)
		dest-> height = 0;
	if ( !useWidth && ( usePitch || useHeight || useName || useSize || useDir || useStyle))
		dest-> width = 0;
	if ( !usePitch && ( useStyle || useName || useDir || useWidth))
		dest-> pitch = fpDefault;
	if ( useHeight)
		dest-> size = 0;
	if ( !useHeight && !useSize && ( dest-> height <= 0 || dest-> height > 16383))
		useSize = 1;

	/* validating entries */
	if ( dest-> height <= 0) dest-> height = 1;
		else if ( dest-> height > 16383 ) dest-> height = 16383;
	if ( dest-> width  <  0) dest-> width  = 1;
		else if ( dest-> width  > 16383 ) dest-> width  = 16383;
	if ( dest-> size   <= 0) dest-> size   = 1;
		else if ( dest-> size   > 16383 ) dest-> size   = 16383;
	if ( dest-> name[0] == 0)
		strcpy( dest-> name, "Default");
	if ( dest-> undef.pitch || dest-> pitch < fpDefault || dest-> pitch > fpFixed)
		dest-> pitch = fpDefault;
	if ( dest-> undef. direction )
		dest-> direction = 0;
	if ( dest-> undef. style )
		dest-> style = 0;
	if ( dest-> undef. vector || dest-> vector < fvBitmap || dest-> vector > fvDefault)
		dest-> vector = fvDefault;
	memset(&dest->undef, 0, sizeof(dest->undef));

	return useSize && !useHeight;
}


int
Drawable_get_paint_state( Handle self)
{
	if ( is_opt( optInDraw))
		return psEnabled;
	else if ( is_opt( optInDrawInfo))
		return psInformation;
	else
		return psDisabled;
}

int
Drawable_get_bpp( Handle self)
{
	gpARGS;
	int ret;
	gpENTER(0);
	ret = apc_gp_get_bpp( self);
	gpLEAVE;
	return ret;
}

SV *
Drawable_linePattern( Handle self, Bool set, SV * pattern)
{
	if ( set) {
		STRLEN len;
		unsigned char *pat = ( unsigned char *) SvPV( pattern, len);
		if ( len > 255) len = 255;
		apc_gp_set_line_pattern( self, pat, len);
	} else {
		unsigned char ret[ 256];
		int len = apc_gp_get_line_pattern( self, ret);
		return newSVpvn((char*) ret, len);
	}
	return nilSV;
}

Color
Drawable_get_nearest_color( Handle self, Color color)
{
	gpARGS;
	gpENTER(clInvalid);
	color = apc_gp_get_nearest_color( self, color);
	gpLEAVE;
	return color;
}

Point
Drawable_resolution( Handle self, Bool set, Point resolution)
{
	if ( set)
		croak("Attempt to write read-only property %s", "Drawable::resolution");
	return apc_gp_get_resolution( self);
}

SV *
Drawable_get_physical_palette( Handle self)
{
	gpARGS;
	int i, nCol;
	AV * av = newAV();
	PRGBColor r;

	gpENTER(newRV_noinc(( SV *) av));
	r = apc_gp_get_physical_palette( self, &nCol);
	gpLEAVE;

	for ( i = 0; i < nCol; i++) {
		av_push( av, newSViv( r[ i].b));
		av_push( av, newSViv( r[ i].g));
		av_push( av, newSViv( r[ i].r));
	}
	free( r);
	return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_font_abcdef( Handle self, int first, int last, Bool unicode, PFontABC (*func)(Handle, int, int, Bool))
{
	int i;
	AV * av;
	PFontABC abc;

	if ( first < 0) first = 0;
	if ( last  < 0) last  = 255;
	if ( !unicode) {
		if ( first > 255) first = 255;
		if ( last  > 255) last  = 255;
	}

	if ( first > last)
		abc = nil;
	else {
		gpARGS;
		gpENTER( newRV_noinc(( SV *) newAV()));
		abc = func( self, first, last, unicode );
		gpLEAVE;
	}

	av = newAV();
	if ( abc != nil) {
		for ( i = 0; i <= last - first; i++) {
			av_push( av, newSVnv( abc[ i]. a));
			av_push( av, newSVnv( abc[ i]. b));
			av_push( av, newSVnv( abc[ i]. c));
		}
		free( abc);
	}
	return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_font_abc( Handle self, int first, int last, Bool unicode)
{
	return Drawable_get_font_abcdef( self, first, last, unicode, apc_gp_get_font_abc);
}

SV *
Drawable_get_font_def( Handle self, int first, int last, Bool unicode)
{
	return Drawable_get_font_abcdef( self, first, last, unicode, apc_gp_get_font_def);
}

SV *
Drawable_get_font_ranges( Handle self)
{
	int count = 0;
	unsigned long * ret;
	AV * av = newAV();
	gpARGS;

	gpENTER( newRV_noinc(( SV *) av));
	ret = apc_gp_get_font_ranges( self, &count);
	gpLEAVE;
	if ( ret) {
		int i;
		for ( i = 0; i < count; i++)
			av_push( av, newSViv( ret[i]));
		free( ret);
	}
	return newRV_noinc(( SV *) av);
}


SV *
Drawable_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_gp_get_handle( self));
	return newSVpv( buf, 0);
}

int
Drawable_height( Handle self, Bool set, int height)
{
	Point p = my-> get_size( self);
	if ( !set)
		return p. y;
	p. y = height;
	my-> set_size( self, p);
	return height;
}

Point
Drawable_size ( Handle self, Bool set, Point size)
{
	if ( set)
		croak("Attempt to write read-only property %s", "Drawable::size");
	size. x = var-> w;
	size. y = var-> h;
	return size;
}

int
Drawable_width( Handle self, Bool set, int width)
{
	Point p = my-> get_size( self);
	if ( !set)
		return p. x;
	p. x = width;
	my-> set_size( self, p);
	return width;
}

Bool
Drawable_put_image_indirect( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
	Bool ok;
	if ( image == nilHandle) return false;
	if ( xLen == xDestLen && yLen == yDestLen)
		ok = apc_gp_put_image( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
	else
		ok = apc_gp_stretch_image( self, image, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
	if ( !ok) perl_error();
	return ok;
}

Bool
Drawable_text_out( Handle self, SV * text, int x, int y)
{
	Bool ok;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
		ok = apc_gp_text_out( self, c_text, x, y, dlen, utf8);
		if ( !ok) perl_error();
	} else {
		SV * ret = sv_call_perl(text, "text_out", "<Hii", self, x, y);
		ok = ret && SvTRUE(ret);
	}
	return ok;
}

static Bool
read_polypoints( Handle self, SV * points, char * procName, int min, Bool (*procPtr)(Handle,int,Point*))
{
	int count;
	Point * p;
	Bool ret = false;
	if (( p = (Point*) prima_read_array( points, procName, true, 2, min, -1, &count)) != NULL) {
		ret = procPtr( self, count, p);
		if ( !ret) perl_error();
		free(p);
	}
	return ret;
}

#define DEF_LINE_PROCESSOR(name,func) Bool \
Drawable_##name( Handle self, SV * points)\
{\
	return read_polypoints( self, points, "Drawable::" #name, 2, func);\
}

DEF_LINE_PROCESSOR(polyline, apc_gp_draw_poly)
DEF_LINE_PROCESSOR(lines, apc_gp_draw_poly2)
DEF_LINE_PROCESSOR(fillpoly, apc_gp_fill_poly)

Bool
Drawable_bars( Handle self, SV * rects)
{
	int count;
	Rect * p;
	Bool ret = false;
	if (( p = prima_read_array( rects, "Drawable::bars", true, 4, 0, -1, &count)) != NULL) {
		ret = apc_gp_bars( self, count, p);
		if ( !ret) perl_error();
		free( p);
	}
	return ret;
}

SV *
Drawable_render_glyph( Handle self, int index, HV * profile)
{
	int flags, *buffer, count;
	SV * ret;
	dPROFILE;
	gpARGS;
	gpENTER(nilSV);

	flags = ggoUseHints;
	if ( pexist(glyph)   && pget_B(glyph))   flags |= ggoGlyphIndex;
	if ( pexist(hints)   && !pget_B(hints))  flags &= ~ggoUseHints;
	if ( pexist(unicode) && pget_B(unicode)) flags |= ggoUnicode;
	count = apc_gp_get_glyph_outline( self, index, flags, &buffer);
	hv_clear(profile); /* old gencls bork */
	gpLEAVE;

	if ( count == 0 ) return nilSV;
	ret = prima_array_new(sizeof(int) * count);
	memcpy( prima_array_get_storage(ret), buffer, sizeof(int) * count);
	free( buffer );
	return prima_array_tie( ret, sizeof(int), "i");
}

/*

Render B-spline

thanks to :

Marcel Steinbeck for tinyspline.c
Thibaut Séguy for bspline.js
Carl-Wilhelm Reinhold de Boor for the rendering algorithm

*/

/* fill default set of knots for curve */
static double *
default_knots( int n_points, int degree, Bool clamped )
{
	double * ret, * ptr;
	int i, n_knots, order;

	order = degree + 1;
	n_knots = n_points + order;
	if ( !( ret = malloc( sizeof(double) * n_knots )))
		return NULL;
	ptr = ret;

	if ( clamped ) {
		int slope = n_knots - order * 2;
		double fac = 1.0 / (n_knots - 2 * degree - 1);
		for ( i = 0; i < order; i++, ptr++) *ptr = 0.0;
		for ( i = 0; i < slope; i++, ptr++) *ptr = fac * (i + 1);
		for ( i = 0; i < order; i++, ptr++) *ptr = 1.0;
	} else {
		for ( i = 0; i < n_knots; i++, ptr++) *ptr = (double) i;
	}

	return ret;
}

/* render single point on a spline with de Boor's algorithm */
static Bool
render_point(
	double t,
	int degree, int n_points, int dimensions, double * v,
	double * knots, int * last_found_knot, Point * result
) {
	double lo, hi;
	int l, i, n_knots = n_points + degree + 1, s, found = false;

	lo = knots[degree];
	hi = knots[n_knots - degree - 1];
	t = t * ( hi - lo ) + lo;

	/* find which pair of knots t belongs to */
	s = ( *last_found_knot < 0 ) ? degree : *last_found_knot;
	for ( ; s < n_points; s++ ) {
		if ( t >= knots[s] && t <= knots[s + 1] ) {
			*last_found_knot = s;
			found = true;
			break;
		}
	}
	if ( !found ) {
		warn("badly formed knot vector: outside curve definition");
		return false;
	}

	for ( l = 1; l <= degree + 1; l++) {
		for ( i = s; i > s - degree - 1 + l; i-- ) {
			int j, ix;
			double dkt, a, a_hat;
			dkt = knots[i + degree + 1 - l] - knots[i];
			if ( dkt == 0.0 ) {
				warn("badly formed knot vector: not increasing");
				return false;
			}
			a = ( t - knots[i] ) / dkt;
			a_hat = 1.0 - a;
			ix = i * 3;
      			/* interpolate each component */
			for ( j = 0; j < dimensions; j++, ix++)
				v[ix] = a_hat * v[ix - 3] + a * v[ix];
		}
	}

	/* convert back to cartesian and return */
	s *= 3;
	if ( dimensions == 3 ) {
		double f;
		f = v[s] / v[s+2];
		result-> x = ( f < 0 ) ? (f - .5) : (f + .5);
		f = v[s+1] / v[s+2];
		result-> y = ( f < 0 ) ? (f - .5) : (f + .5);
	} else {
		result-> x = (v[s] < 0) ? (v[s] - .5) : (v[s] + .5);
		result-> y = (v[s+1] < 0) ? (v[s+1] - .5) : (v[s+1] + .5);
	}

	return true;
}

static int
tangent_detect( Point * a, Point * b)
{
	register int x = a->x - b->x;
	register int y = a->y - b->y;
	if ( x == 0 ) {
		if ( y ==  0) return 0;
		if ( y == -1) return 1;
		if ( y ==  1) return 2;
	} else if ( y == 0 ) {
		if ( x == -1) return 3;
		if ( x ==  1) return 4;
	} else if ( x < 2 && x > -2 && y < 2 && y > -2 )
		return (x * 10) + y;
	return -1;
}

static void
tangent_apply( int tangent,  Point * b)
{
	switch(tangent) {
	case 1:
		b->y++;
		break;
	case 2:
		b->y--;
		break;
	case 3:
		b->x++;
		break;
	case 4:
		b->x--;
		break;
	case 10 - 1:
		b->x--;	
		b->y++;	
		break;
	case 10 + 1:
		b->x--;	
		b->y--;	
		break;
	case - 10 - 1:
		b->x++;	
		b->y++;	
		break;
	case - 10 + 1:
		b->x++;	
		b->y--;	
		break;
	}
}

static Bool
cut_corner( int t2, int t1, Point * p2, Point * p1)
{
	switch (t2 * 10 + t1) {
	/*
	 >xxa    xx is tangent 3
  	    ab>  aa is tangent 1
	         bb is something not 1

        corner is detected when (aa) is exactly 1px high,
	and is cut so that (ab) becomes (b)
	*/
	case 13: case 14: return p2->y + 1 == p1->y;
	case 23: case 24: return p2->y - 1 == p1->y;
	case 31: case 32: return p2->x + 1 == p1->x;
	case 41: case 42: return p2->x - 1 == p1->x;
	}
	return false;
}

SV *
Drawable_render_spline( SV * obj, SV * points, HV * profile)
{
	dPROFILE;
	NPoint *p, *pp;
	Point *rendered, *storage;
	SV *ret;
	Bool ok, closed;
	int i, j, degree, precision, n_points, final_size, k, dim, n_add_points, temp_size,
		tangent, last_tangent;
	double *knots, *weights, t, dt, *weighted, *temp;

	knots = weights = weighted = NULL;
	p = NULL;
	ret = NULL;
	ok = false;

	/* parse input */
	if ( pexist( degree )) {
		degree = pget_i(degree);
		if ( degree < 2 ) {
			warn("degree must be at least 2");
			goto EXIT;
		}
	} else
		degree = 2;

	if ( pexist( precision )) {
		precision = pget_i(precision);
		if ( precision < 2 || precision > 1024 ) {
			warn("precision must be at least 2 and max 1024");
			goto EXIT;
		}
	} else
		precision = 24;

	p = (NPoint*) prima_read_array( points, "Drawable::render_spline", false, 2, degree + 1, -1, &n_points);
	if ( !p) goto EXIT;

	/* closed curve will need at least one extra point and unclamped default knot set */
	if ( pexist( closed )) {
		SV * sv = pget_sv(closed);
		if ( SvTYPE(sv) == SVt_NULL ) goto DETECT_SHAPE;
		closed = SvTRUE(sv);
	}
	else {
	DETECT_SHAPE:
		closed = p[0].x == p[n_points-1].x && p[0].y == p[n_points-1].y;
	}
	n_add_points = closed ? degree - 1 : 0;
	n_points += n_add_points;

	if ( pexist( knots )) {
		knots = (double*) prima_read_array( pget_sv(knots), "knots", false, 1,
			n_points + degree + 1, n_points + degree + 1, NULL);
		if (!knots) goto EXIT;
	} else
		knots = default_knots(n_points, degree, !closed);

	if ( pexist( weights )) {
		weights = (double*) prima_read_array(pget_sv(weights), "weights", false, 1,
			n_points, n_points, NULL);
		if (!weights) goto EXIT;
		dim = 3;
	} else {
		weights = NULL; /* all ones */
		dim = 2;
	}

	/* allocate result storage */
	precision *= n_points - n_add_points;
	ret = prima_array_new(( precision + 1) * sizeof(Point) );

	temp_size = sizeof(double) * 3 * n_points;
	if ( !(weighted = malloc( 2 * temp_size ))) {
		warn("not enough memory");
		goto EXIT;
	}

	/* convert to weighted points */
	for ( i = j = 0, pp = p; i < n_points; i++, pp++) {
		register double w = weights ? weights[i] : 1.0;
		weighted[j++] = pp->x * w;
		weighted[j++] = pp->y * w;
		weighted[j++] = w;
		if ( i == n_points - n_add_points - 1 ) pp = p;
	}
	temp = weighted + 3 * n_points;

	/* render */
	final_size = 0;
	rendered = storage = (Point*) prima_array_get_storage(ret);
	k = -1;
	last_tangent = -1;
	for ( i = 0, t = 0.0, dt = 1.0 / precision; i < precision - 1; i++, t += dt) {
		memcpy( temp, weighted, temp_size);
		if (!render_point(t, degree, n_points, dim, temp, knots, &k, rendered))
			goto EXIT;
		if ( i > 0 ) {
			/* primitive line detection */
			tangent = tangent_detect( rendered-1, rendered);
			if ( tangent == 0 ) continue;
			if ( i > 1 && tangent > 0 && tangent == last_tangent) {
				tangent_apply( tangent, rendered-1);
				continue;
			} else if ( cut_corner(last_tangent, tangent, rendered-2, rendered-1)) {
			/* primitive corner detection - convert 4-connectivity into 8- */
				*(rendered-1) = *rendered;
				tangent = -1;
				rendered--;
				final_size--;
				continue;
			} else
				last_tangent = tangent;
		}
		final_size++;
		rendered++;
	}
	memcpy( temp, weighted, temp_size);
	if ( !render_point(1.0, degree, n_points, dim, temp, knots, &k, rendered))
		goto EXIT;
	final_size++;
	rendered++;

	/* looks good */
	ok = true;
	if ( closed ) {
		final_size++;
		*rendered = storage[0];
		rendered++;
	}
	if ( final_size == 1 ) {
		final_size = 2;
		storage[1] = storage[0];
	}
	prima_array_truncate( ret, final_size * sizeof( Point) );

EXIT:
	hv_clear(profile); /* old gencls bork */
	if (weighted) free(weighted);
	if (p)        free(p);
	if (knots)    free(knots);
	if (weights)  free(weights);
	if ( ok ) {
		return prima_array_tie( ret, sizeof(int), "i");
	} else {
		if (ret)  sv_free(ret);
		return newRV_noinc(( SV *) newAV());
	}
}

int
Drawable_get_text_width( Handle self, SV * text, Bool addOverhang)
{
	gpARGS;
	int res;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
		gpENTER(0);
		res = apc_gp_get_text_width( self, c_text, dlen, addOverhang, utf8);
		gpLEAVE;
	} else {
		SV * ret;
		gpENTER(0);
		ret = sv_call_perl(text, "get_text_width", "<Hi", self, addOverhang);
		gpLEAVE;
		res = (ret && SvOK(ret)) ? SvIV(ret) : 0;
	}
	return res;
}

SV *
Drawable_get_text_box( Handle self, SV * text)
{
	gpARGS;
	Point * p;
	AV * av;
	int i;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
		gpENTER( newRV_noinc(( SV *) newAV()));
		p = apc_gp_get_text_box( self, c_text, dlen, utf8);
		gpLEAVE;

		av = newAV();
		if ( p) {
			for ( i = 0; i < 5; i++) {
				av_push( av, newSViv( p[ i]. x));
				av_push( av, newSViv( p[ i]. y));
			};
			free( p);
		}
		return newRV_noinc(( SV *) av);
	} else {
		SV * ret;
		gpENTER( newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "get_text_box", "<H", self));
		gpLEAVE;
		return ret;
	}
}

static PFontABC
query_abc_range( Handle self, TextWrapRec * t, unsigned int base)
{
	PFontABC abc;

	/* find if present in cache */
	if ( t-> utf8_text) {
		if ( *(t-> unicode)) {
			int i;
			PList p;
			if (( p = *(t-> unicode)))
				for ( i = 0; i < p-> count; i += 2)
					if (( unsigned int) p-> items[ i] == base)
						return ( PFontABC) p-> items[i + 1];
		}
	} else
		if ( *( t-> ascii)) return *(t-> ascii);

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text);
		if ( !abc) return nil;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		gpARGS;
		gpENTER(nil);
		abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text);
		gpLEAVE;
		if ( !abc) return nil;
	} else {
		SV * sv;
		if ( !( abc = malloc( 256 * sizeof( FontABC)))) return nil;
		sv = my-> get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > 256 * 3) n = 256 * 3;
			n = ( n / 3) * 3;
			if ( n < 256) memset( abc, 0, 256 * sizeof( FontABC));
			for ( i = 0; i < n; i += 3) {
				SV ** holder = av_fetch( av, i, 0);
				if ( holder) abc[j]. a = ( float) SvNV( *holder);
				holder = av_fetch( av, i + 1, 0);
				if ( holder) abc[j]. b = ( float) SvNV( *holder);
				holder = av_fetch( av, i + 2, 0);
				if ( holder) abc[j]. c = ( float) SvNV( *holder);
				j++;
			}
		} else
			memset( abc, 0, 256 * sizeof( FontABC));
		sv_free( sv);
	}

	/* store in cache */
	if ( t-> utf8_text) {
		PList p;
		if ( !*(t-> unicode))
			*(t-> unicode) = plist_create( 8, 8);
		if (( p = *(t-> unicode))) {
			list_add( p, ( Handle) base);
			list_add( p, ( Handle) abc);
		} else {
			free( abc);
			return nil;
		}
	} else
		*(t-> ascii) = abc;

	return abc;
}

static Bool
precalc_abc_buffer( PFontABC src, float * width, PFontABC dest)
{
	int i;
	if ( !dest) return false;
	for ( i = 0; i < 256; i++) {
		width[i] = src[i]. a + src[i]. b + src[i]. c;
		dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
		dest[i]. b = src[i]. b;
		dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
	}
	return true;
}

static Bool
add_wrapped_text( TextWrapRec * t, int start, int utfstart, int end, int utfend,
						int tildeIndex, int * tildePos, int * tildeLPos, int * tildeLine,
						char *** lArray, int * lSize)
{
	int l = end - start;
	char *c = nil;
	if (!( t-> options & twReturnChunks)) {
		if ( !( c = allocs( l + 1))) return false;
		memcpy( c, t-> text + start, l);
		c[ l] = 0;
	}
	if ( tildeIndex >= 0 && tildeIndex >= start && tildeIndex < end) {
		*tildeLine = t-> t_line = t-> count;
		*tildePos = *tildeLPos = tildeIndex - start;
		if ( tildeIndex == end - 1) {
			t-> t_line++;
			tildeLPos = 0;
		}
	}
	if ( t-> count == *lSize) {
		char ** n = allocn( char*, *lSize + 16);
		if ( !n) return false;
		memcpy( n, *lArray, sizeof( char*) * (*lSize));
		*lSize += 16;
		free( *lArray);
		*lArray = n;
	}
	if ( t-> options & twReturnChunks) {
		(*lArray)[ t-> count++] = INT2PTR(char*,utfstart);
		(*lArray)[ t-> count++] = INT2PTR(char*,utfend - utfstart);
	} else
		(*lArray)[ t-> count++] = c;
	return true;
}

char **
Drawable_do_text_wrap( Handle self, TextWrapRec * t)
{
	unsigned int base = 0x10000000;
	float width[256];
	FontABC abc[256];
	int start = 0, utf_start = 0, split_start = -1, split_end = -1, i, utf_p, utf_split = -1;
	float w = 0, inc = 0;
	char **ret;
	Bool wasTab = 0, reassign_w = 1;
	Bool doWidthBreak = t-> width >= 0;
	int tildeIndex = -100, tildeLPos = 0, tildeLine = 0, tildePos = 0, tildeOffset = 0, lSize = 16;
	int spaceWidth = 0, spaceC = 0, spaceOK = 0;

#define lAdd(end, utfend) if(1) { \
	if ( !add_wrapped_text( t, start, utf_start, end, utfend, tildeIndex, \
		&tildePos, &tildeLPos, &tildeLine, &ret, &lSize)) return ret;\
	start = end; \
	utf_start = utfend; \
	if (( t-> options & twReturnFirstLineLength) == twReturnFirstLineLength) return ret; }

	t-> count = 0;
	if (!( ret = allocn( char*, lSize))) return nil;

	/* determining ~ character location */
	if ( t-> options & twCalcMnemonic)
		for ( i = 0; i < t-> textLen - 1; i++)
			if ( t-> text[ i] == '~') {
				unsigned char c = t-> text[ i + 1];
				if ( c == '~' || c < ' ') {
					i++;
					continue;
				} else {
					tildeIndex = i;
					break;
				}
			}


	/* process UV chars */
	for ( i = 0, utf_p = 0; i < t-> textLen; utf_p++) {
		UV uv;
		float winc;
		int p = i;

		if ( t-> utf8_text) {
			STRLEN len;
#if PERL_PATCHLEVEL >= 16
			uv = utf8_to_uvchr_buf(( U8*) t-> text + i, ( U8*) t-> text + t-> textLen, &len);
#else
			uv = utf8_to_uvchr(( U8*) t-> text + i, &len);
#endif
			i += len;
			if ( len == 0 ) break;
		} else
			uv = (( unsigned char *)(t-> text))[i++];

		if ( uv / 256 != base)
			if ( !precalc_abc_buffer( query_abc_range( self, t, base = uv / 256), width, abc))
				return ret;
		if ( reassign_w) w = abc[ uv & 0xff]. a;
		reassign_w = 0;

		switch ( uv ) {
		case '\t':
			split_start = p; split_end = i; utf_split = utf_p;
			if (!( t-> options & twCalcTabs)) goto _default;
			if ( t-> options & twSpaceBreak) {
				lAdd( p, utf_p);
				start = i;
				utf_start++;
				reassign_w = 1;
				continue;
			}
			if ( !spaceOK) {
				PFontABC s = query_abc_range( self, t, 0);
				if ( !s) return ret;
				spaceWidth = (s[' '].a + s[' '].b + s[' '].c) * t-> tabIndent;
				spaceC     = (s[' '].c < 0) ? - s[' ']. c : 0;
				spaceOK = 1;
			}
			winc = spaceWidth;
			inc  = spaceC;
			wasTab = true;
			break;
		case '\n':
		case 0x2028:
		case 0x2029:
			split_start = p; split_end = i; utf_split = utf_p;
			if (!( t-> options & twNewLineBreak)) goto _default;
			lAdd( p, utf_p);
			start = i;
			utf_start++;
			reassign_w = 1;
			continue;
		case ' ':
			split_start = p; split_end = i; utf_split = utf_p;
			if (!( t-> options & twSpaceBreak)) goto _default;
			lAdd( p, utf_p);
			start = i;
			utf_start++;
			reassign_w = 1;
			continue;
		case '~':
			if ( p == tildeIndex ) {
				tildeOffset = w;
				inc = winc = 0;
				break;
			}
		_default: default:
			winc = width[ uv & 0xff];
			inc  = abc[ uv & 0xff]. c;
		}

		if ( doWidthBreak && w + winc + inc > t-> width) {
			if (( p == start) || (( p == start - 1) && ( p - 1 == tildeIndex))) {
			/* case when even single char cannot be fit in  */
				if ( t-> options & twBreakSingle) {
					/* do not return anything in this case */
					int j;
					if (!( t-> options & twReturnChunks)) {
						for ( j = 0; j < t-> count; j++) free( ret[ j]);
						ret[ 0] = duplicate_string("");
					}
					t-> count = 0;
					return ret;
				}
				/* or push this character disregarding the width */
				lAdd( i, utf_p + 1);
			} else { /* normal break condition */
				/* checking if break was at word boundary */
				if ( t-> options & twWordBreak) {
					if ( start <= split_start) {
						lAdd( split_start, utf_split );
						i = start = split_end;
						utf_start = utf_split + 1;
						utf_p = utf_split;
						w = 0;
						continue;
					} else if ( t-> options & twBreakSingle) {
						/* cannot be split, return nothing */
						int j;
						if (!( t-> options & twReturnChunks)) {
							for ( j = 0; j < t-> count; j++) free( ret[ j]);
							ret[ 0] = duplicate_string("");
						}
						t-> count = 0;
						return ret;
					}
				}
				/* repeat again */
				lAdd( p, utf_p );
				i = start = p;
				utf_start = utf_p;
				utf_p--;
			}
			w = 0;
			continue;
		} else
			w += winc;
	}

	/* adding or skipping last line */
	if ( t-> textLen - start > 0 || t-> count == 0) lAdd( t-> textLen, t-> utf8_textLen);

	/* removing ~ and determining it's location */
	if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
		PFontABC abc;
		char *l = ret[ tildeLine];
		t-> t_char = t-> text + tildePos + 1;
		if ( t-> options & twCollapseTilde)
			memmove( l + tildePos, l + tildePos + 1, strlen( l) - tildePos);
		abc = query_abc_range( self, t, 0) + '~';
		w = tildeOffset;
		t-> t_start = w - 1;
		t-> t_end   = w + abc->a + abc->b + abc->c;
	} else {
		t-> t_start = t-> t_end = t-> t_line = C_NUMERIC_UNDEF;
	}

	/* expanding tabs */
	if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab) {
		for ( i = 0; i < t-> count; i++) {
			int tabs = 0, len = 0;
			char *substr = ret[ i], *n;
			while (*substr) {
				if ( *substr == '\t') tabs++;
				substr++;
				len++;
			}
			if ( tabs == 0) continue;
			if ( !( n = allocs( len + tabs * t-> tabIndent + 1)))
				return ret;
			len = 0;
			substr = ret[ i];
			while ( *substr) {
				if ( *substr == '\t') {
					int j = t-> tabIndent;
					while ( j--) n[ len++] = ' ';
				} else
					n[ len++] = *substr;
				substr++;
			}
			free( ret[ i]);
			n[ len] = 0;
			ret[ i] = n;
		}
	}

	return ret;
}

SV*
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent)
{
	gpARGS;
	TextWrapRec t;
	Bool retChunks;
	char** c;
	int i;
	AV * av;
	STRLEN tlen;
	int returnFirstLine = ( options & twReturnFirstLineLength) == twReturnFirstLineLength;

	if ( SvROK( text )) {
		SV * ret;
		gpENTER( returnFirstLine ? newSViv(0) : newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "text_wrap", "<Hiii", self, width, options, tabIndent));
		gpLEAVE;
		return ret;
	}

	t. text      = SvPV( text, tlen);
	t. utf8_text = prima_is_utf8_sv( text);
	if ( t. utf8_text) {
		t. utf8_textLen = prima_utf8_length( t. text);
		t. textLen = utf8_hop(( U8*) t. text, t. utf8_textLen) - (U8*) t. text;
	} else {
		t. utf8_textLen = t. textLen = tlen;
	}
	t. width     = ( width < 0) ? 0 : width;
	t. tabIndent = ( tabIndent < 0) ? 0 : tabIndent;
	t. options   = options;
	retChunks    = t. options & twReturnChunks;
	t. ascii     = &var-> font_abc_ascii;
	t. unicode   = &var-> font_abc_unicode;
	t. t_char    = nil;

	gpENTER( returnFirstLine ? newSViv(0) : newRV_noinc(( SV *) newAV()));
	c = Drawable_do_text_wrap( self, &t);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		IV rlen = 0;
		if ( c) {
			if ( t. count > 0) rlen = PTR2IV(c[1]);
			free( c);
		}
		return newSViv( rlen);
	}

	if ( !c) return nilSV;

	av = newAV();
	for ( i = 0; i < t. count; i++) {
		SV * sv = retChunks ? newSViv( PTR2IV(c[i])) : newSVpv( c[ i], 0);
		if ( !retChunks) {
			if ( t. utf8_text) SvUTF8_on( sv);
			free( c[i]);
		}
		av_push( av, sv);
	}

	free( c);

	if  ( t. options & ( twCalcMnemonic | twCollapseTilde)) {
		HV * profile = newHV();
		SV * sv_char;
		if ( t. t_char) {
			STRLEN len = t. utf8_text ? utf8_hop(( U8*) t. t_char, 1) - ( U8*) t. t_char : 1;
			sv_char = newSVpv( t. t_char, len);
			if ( t. utf8_text) SvUTF8_on( sv_char);
			if ( t. t_start != C_NUMERIC_UNDEF) pset_i( tildeStart, t. t_start);
			if ( t. t_end   != C_NUMERIC_UNDEF) pset_i( tildeEnd,   t. t_end);
			if ( t. t_line  != C_NUMERIC_UNDEF) pset_i( tildeLine,  t. t_line);
		} else {
			sv_char = newSVsv( nilSV);
			pset_sv( tildeStart, nilSV);
			pset_sv( tildeEnd,   nilSV);
			pset_sv( tildeLine,  nilSV);
		}
		pset_sv_noinc( tildeChar, sv_char);
		av_push( av, newRV_noinc(( SV *) profile));
	}

	return newRV_noinc(( SV *) av);
}


PRGBColor
prima_read_palette( int * palSize, SV * palette)
{
	AV * av;
	int i, count;
	Byte * buf;

	if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV)) {
		*palSize = 0;
		return nil;
	}
	av = (AV *) SvRV( palette);
	count = av_len( av) + 1;
	*palSize = count / 3;
	count = *palSize * 3;
	if ( count == 0) return nil;

	if ( !( buf = allocb( count))) return nil;

	for ( i = 0; i < count; i++)
	{
		SV **itemHolder = av_fetch( av, i, 0);
		if ( itemHolder == nil)
			return ( PRGBColor) buf;
		buf[ i] = SvIV( *itemHolder);
	}

	return ( PRGBColor) buf;
}

Color
Drawable_backColor( Handle self, Bool set, Color color)
{
	if (!set) return apc_gp_get_back_color( self);
	apc_gp_set_back_color( self, color);
	return color;
}

Color
Drawable_color( Handle self, Bool set, Color color)
{
	if (!set) return apc_gp_get_color( self);
	apc_gp_set_color( self, color);
	return color;
}

Rect
Drawable_clipRect( Handle self, Bool set, Rect clipRect)
{
	if ( !set)
		return apc_gp_get_clip_rect( self);
	apc_gp_set_clip_rect( self, clipRect);
	return clipRect;
}

int
Drawable_fillMode( Handle self, Bool set, int fillMode)
{
	if (!set) return apc_gp_get_fill_mode( self);
	apc_gp_set_fill_mode( self, fillMode);
	return fillMode;
}

int
Drawable_lineEnd( Handle self, Bool set, int lineEnd)
{
	if (!set) return apc_gp_get_line_end( self);
	apc_gp_set_line_end( self, lineEnd);
	return lineEnd;
}

int
Drawable_lineJoin( Handle self, Bool set, int lineJoin)
{
	if (!set) return apc_gp_get_line_join( self);
	apc_gp_set_line_join( self, lineJoin);
	return lineJoin;
}

int
Drawable_lineWidth( Handle self, Bool set, int lineWidth)
{
	if (!set) return apc_gp_get_line_width( self);
	if ( lineWidth < 0 ) lineWidth = 0;
	apc_gp_set_line_width( self, lineWidth);
	return lineWidth;
}

double
Drawable_miterLimit( Handle self, Bool set, double miterLimit)
{
	if (!set) return apc_gp_get_miter_limit( self);
	apc_gp_set_miter_limit( self, miterLimit);
	return miterLimit;
}

SV *
Drawable_palette( Handle self, Bool set, SV * palette)
{
	int colors;
	if ( var-> stage > csFrozen) return nilSV;
	colors = var-> palSize;
	if ( set) {
		free( var-> palette);
		var-> palette = prima_read_palette( &var-> palSize, palette);
		if ( colors == 0 && var-> palSize == 0) return nilSV; /* do not bother apc */
		apc_gp_set_palette( self);
	} else {
		AV * av = newAV();
		int i;
		Byte * pal = ( Byte*) var-> palette;
		for ( i = 0; i < colors * 3; i++) av_push( av, newSViv( pal[ i]));
		return newRV_noinc(( SV *) av);
	}
	return nilSV;
}

SV *
Drawable_pixel( Handle self, Bool set, int x, int y, SV * color)
{
	if (!set)
		return newSViv( apc_gp_get_pixel( self, x, y));
	apc_gp_set_pixel( self, x, y, SvIV( color));
	return nilSV;
}

Handle
Drawable_region( Handle self, Bool set, Handle mask)
{
	if ( var-> stage > csFrozen) return nilHandle;

	if ( set) {
		if ( mask && kind_of( mask, CRegion)) {
			apc_gp_set_region( self, mask);
			return nilHandle;
		}

		if ( mask && !kind_of( mask, CImage)) {
			warn("Illegal object reference passed to Drawable::region");
			return nilHandle;
		}

		if ( mask ) {
			Handle region;
			HV * profile = newHV();

			pset_H( image, mask );
			region = Object_create("Prima::Region", profile);
			sv_free(( SV *) profile);

			apc_gp_set_region(self, region);
			Object_destroy(region);

		} else
			apc_gp_set_region(self, nilHandle);

	} else if ( apc_gp_get_region( self, nilHandle)) {
		HV * profile = newHV();
		Handle i = Object_create( "Prima::Region", profile);
		sv_free(( SV *) profile);
		apc_gp_get_region( self, i);
		--SvREFCNT( SvRV((( PAnyObject) i)-> mate));
		return i;
	}

	return nilHandle;
}

int
Drawable_rop( Handle self, Bool set, int rop)
{
	if (!set) return apc_gp_get_rop( self);
	apc_gp_set_rop( self, rop);
	return rop;
}

int
Drawable_rop2( Handle self, Bool set, int rop2)
{
	if (!set) return apc_gp_get_rop2( self);
	apc_gp_set_rop2( self, rop2);
	return rop2;
}

Bool
Drawable_textOpaque( Handle self, Bool set, Bool opaque)
{
	if (!set) return apc_gp_get_text_opaque( self);
	apc_gp_set_text_opaque( self, opaque);
	return opaque;
}

Bool
Drawable_textOutBaseline( Handle self, Bool set, Bool textOutBaseline)
{
	if (!set) return apc_gp_get_text_out_baseline( self);
	apc_gp_set_text_out_baseline( self, textOutBaseline);
	return textOutBaseline;
}

Point
Drawable_translate( Handle self, Bool set, Point translate)
{
	if (!set) return apc_gp_get_transform( self);
	apc_gp_set_transform( self, translate. x, translate. y);
	return translate;
}

SV *
Drawable_fillPattern( Handle self, Bool set, SV * svpattern)
{
	int i;
	if ( !set) {
		AV * av;
		FillPattern * fp = apc_gp_get_fill_pattern( self);
		if ( !fp) return nilSV;
		av = newAV();
		for ( i = 0; i < 8; i++) av_push( av, newSViv(( int) (*fp)[i]));
		return newRV_noinc(( SV *) av);
	} else {
		if ( SvROK( svpattern) && ( SvTYPE( SvRV( svpattern)) == SVt_PVAV)) {
			FillPattern fp;
			AV * av = ( AV *) SvRV( svpattern);
			if ( av_len( av) != 7) {
				warn("Illegal fillPattern passed to Drawable::fillPattern");
				return nilSV;
			}
			for ( i = 0; i < 8; i++) {
				SV ** holder = av_fetch( av, i, 0);
				if ( !holder) {
					warn("Array panic on Drawable::fillPattern");
					return nilSV;
				}
				fp[ i] = SvIV( *holder);
			}
			apc_gp_set_fill_pattern( self, fp);
		} else {
			int id = SvIV( svpattern);
			if (( id < 0) || ( id > fpMaxId)) {
				warn("fillPattern index out of range passed to Drawable::fillPattern");
				return nilSV;
			}
			apc_gp_set_fill_pattern( self, fillPatterns[ id]);
		}
	}
	return nilSV;
}

Point
Drawable_fillPatternOffset( Handle self, Bool set, Point fpo)
{
	if (!set) return apc_gp_get_fill_pattern_offset( self);
	fpo. x %= 8;
	fpo. y %= 8;
	apc_gp_set_fill_pattern_offset( self, fpo);
	return fpo;
}

Font
Drawable_get_font( Handle self)
{
	return var-> font;
}

void
Drawable_set_font( Handle self, Font font)
{
	clear_font_abc_caches( self);
	apc_font_pick( self, &font, &var-> font);
	apc_gp_set_font( self, &var-> font);
}


#ifdef __cplusplus
}
#endif
