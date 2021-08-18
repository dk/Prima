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

#define CHECK_GP(ret) \
	if ( !is_opt(optSystemDrawable)) { \
		warn("This method is not available because %s is not a system Drawable object. You need to implement your own (ref:%d)", my->className, __LINE__);\
		return ret; \
	}

void
Drawable_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	apc_gp_init( self);
	var-> w = var-> h = 0;
	my-> set_alpha        ( self, pget_i ( alpha));
	my-> set_antialias    ( self, pget_B ( antialias));
	my-> set_color        ( self, pget_i ( color));
	my-> set_backColor    ( self, pget_i ( backColor));
	my-> set_fillMode     ( self, pget_i ( fillMode));
	my-> set_fillPattern  ( self, pget_sv( fillPattern));
	my-> set_lineEnd      ( self, pget_i ( lineEnd));
	my-> set_lineJoin     ( self, pget_i ( lineJoin));
	my-> set_linePattern  ( self, pget_sv( linePattern));
	my-> set_lineWidth    ( self, pget_f ( lineWidth));
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
	if (( u = var-> font_abc_glyphs)) {
		int i;
		for ( i = 0; i < u-> count; i += 2)
			free(( void*) u-> items[ i + 1]);
		plist_destroy( u);
		var-> font_abc_glyphs = NULL;
	}
	if (( u = var-> font_abc_unicode)) {
		int i;
		for ( i = 0; i < u-> count; i += 2)
			free(( void*) u-> items[ i + 1]);
		plist_destroy( u);
		var-> font_abc_unicode = NULL;
	}
	if ( var-> font_abc_ascii) {
		free( var-> font_abc_ascii);
		var-> font_abc_ascii = NULL;
	}
	if ( var-> font_abc_glyphs_ranges ) {
		free(var-> font_abc_glyphs_ranges);
		var-> font_abc_glyphs_ranges = NULL;
		var-> font_abc_glyphs_n_ranges = 0;
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
		apc_font_pick( NULL_HANDLE, source, dest);
	else
		Drawable_font_add( NULL_HANDLE, source, dest);
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
		if ( useName  ) {
			strcpy( dest-> name, source-> name);
			dest->is_utf8.name = source->is_utf8.name;
		}
		if ( useEnc   ) {
			strcpy( dest-> encoding, source-> encoding);
			dest->is_utf8.encoding = source->is_utf8.encoding;
		}
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
	if ( dest-> name[0] == 0) {
		strcpy( dest-> name, "Default");
		dest->is_utf8.name = false;
	}
	if ( dest-> undef.pitch || dest-> pitch < fpDefault || dest-> pitch > fpFixed)
		dest-> pitch = fpDefault;
	if ( dest-> undef. direction )
		dest-> direction = 0;
	if ( dest-> undef. style )
		dest-> style = 0;
	if ( dest-> undef. vector || dest-> vector < fvBitmap || dest-> vector > fvDefault)
		dest-> vector = fvDefault;
	if ( dest-> undef. encoding )
		dest-> encoding[0] = 0;
	memset(&dest->undef, 0, sizeof(dest->undef));

	return useSize && !useHeight;
}

int
Drawable_get_bpp( Handle self)
{
	gpARGS;
	int ret;
	CHECK_GP(0);
	gpENTER(0);
	ret = apc_gp_get_bpp( self);
	gpLEAVE;
	return ret;
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

Color
Drawable_get_nearest_color( Handle self, Color color)
{
	gpARGS;
	CHECK_GP(0);
	gpENTER(clInvalid);
	color = apc_gp_get_nearest_color( self, color);
	gpLEAVE;
	return color;
}

SV *
Drawable_get_physical_palette( Handle self)
{
	gpARGS;
	int i, nCol;
	AV * av = newAV();
	PRGBColor r;

	CHECK_GP(NULL_SV);
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
Drawable_get_font_abcdef( Handle self, int first, int last, int flags, PFontABC (*func)(Handle, int, int, int))
{
	int i;
	AV * av;
	PFontABC abc;

	if ( first < 0) first = 0;
	if ( last  < 0) last  = 255;

	if ( flags & toGlyphs )
		flags &= ~toUTF8;
	else if ( !(flags & toUTF8)) {
		if ( first > 255) first = 255;
		if ( last  > 255) last  = 255;
	}

	if ( first > last)
		abc = NULL;
	else {
		gpARGS;
		gpENTER( newRV_noinc(( SV *) newAV()));
		abc = func( self, first, last, flags );
		gpLEAVE;
	}

	av = newAV();
	if ( abc != NULL) {
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
Drawable_get_font_abc( Handle self, int first, int last, int flags)
{
	CHECK_GP(NULL_SV);
	return Drawable_get_font_abcdef( self, first, last, flags, apc_gp_get_font_abc);
}

SV *
Drawable_get_font_def( Handle self, int first, int last, int flags)
{
	CHECK_GP(NULL_SV);
	return Drawable_get_font_abcdef( self, first, last, flags, apc_gp_get_font_def);
}

SV *
Drawable_get_font_languages( Handle self)
{
	char *buf, *p;
	AV * av = newAV();
	gpARGS;

	CHECK_GP(NULL_SV);
	gpENTER( newRV_noinc(( SV *) av));
	p = buf = apc_gp_get_font_languages( self);
	gpLEAVE;
	if (p) {
		while (*p) {
			int len = strlen(p);
			av_push(av, newSVpv(p, len));
			p += len + 1;
		}
		free(buf);
	}
	return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_font_ranges( Handle self)
{
	int count = 0;
	unsigned long * ret;
	AV * av = newAV();
	gpARGS;

	CHECK_GP(NULL_SV);
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
	CHECK_GP(NULL_SV);
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
Drawable_resolution( Handle self, Bool set, Point resolution)
{
	CHECK_GP(resolution);
	if ( set)
		croak("Attempt to write read-only property %s", "Drawable::resolution");
	return apc_gp_get_resolution( self);
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
	CHECK_GP(false);
	if ( image == NULL_HANDLE) return false;
	if ( !(PObject(image)-> options.optSystemDrawable)) {
		warn("This method is not available on this class because it is not a system Drawable object. You need to implement your own");
		return false;
	}
	if ( xLen == xDestLen && yLen == yDestLen)
		ok = apc_gp_put_image( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
	else
		ok = apc_gp_stretch_image( self, image, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
	if ( !ok) perl_error();
	return ok;
}

static Bool
primitive( Handle self, Bool fill, char * method, ...)
{
	Bool r;
	SV * ret;
	char format[256];
	va_list args;
	va_start( args, method);
	ENTER;
	SAVETMPS;
	strcpy(format, "<");
	strncat(format, method, 255);
	ret = call_perl_indirect( self, fill ? "fill_aa_primitive" : "stroke_aa_primitive", format, true, false, args);
	va_end( args);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}

#define IS_AA (var->antialias || (var->alpha < 255))
#define TRUNC(x) x=trunc(x)
#define TRUNC2(x,y) {TRUNC(x);TRUNC(y);}
#define TRUNC4(x,y,z,t) {TRUNC(x);TRUNC(y);TRUNC(z);TRUNC(t);}


Bool
Drawable_arc( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	while ( startAngle > endAngle ) endAngle += 360.0;
	return IS_AA ?
		primitive( self, 0, "snnnnnn", "arc", x, y, dX-1, dY-1, startAngle, endAngle) :
		apc_gp_arc(self, x, y, dX, dY, startAngle, endAngle);
}

Bool
Drawable_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnnnn", "chord", x, y, dX-1, dY-1, startAngle, endAngle) :
		apc_gp_chord(self, x, y, dX, dY, startAngle, endAngle);
}

Bool
Drawable_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnn", "ellipse", x, y, dX-1, dY-1) :
		apc_gp_ellipse(self, x, y, dX, dY);
}

Bool
Drawable_bar( Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);

	if ( !var->antialias ) TRUNC4(x1,y1,x2,y2);

	if (IS_AA) {
		NPoint r[5] = { {x1,y1}, {x2,y1}, {x2,y2}, {x1,y2}, {x1,y1} };
		return apc_gp_aa_fill_poly( self, 5, r);
	} else
		return apc_gp_bar(self, x1, y1, x2, y2);
}

Bool
Drawable_bars( Handle self, SV * rects)
{
	int count;
	Rect * p;
	Bool ret = false, do_free;
	CHECK_GP(false);
	if (( p = prima_read_array( rects, "Drawable::bars",
		IS_AA ? 'd' : 'i',
		4, 0, -1, &count, &do_free)) == NULL)
		return false;

	if ( IS_AA ) {
		int i;
		NRect *r;
		for ( i = 0, r = (NRect*)p; i < count; i++, r++) {
			NPoint xr[5] = {
				{r->left,r->bottom},
				{r->left,r->top},
				{r->right,r->top},
				{r->right,r->bottom},
				{r->left,r->bottom}
			};
			if ( !var->antialias) {
				int j;
				for ( j = 0; j < 5; j++)
					TRUNC2(xr[j].x,xr[j].y);
			}
			if ( !( ret = apc_gp_aa_fill_poly( self, 5, xr)))
				break;
		}
	} else
		ret = apc_gp_bars( self, count, p);
	if ( !ret) perl_error();
	if ( do_free ) free( p);
	return ret;
}

Bool
Drawable_clear( Handle self, double x1, double y1, double x2, double y2)
{
	Bool full;
	CHECK_GP(false);

	full = x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0;
	if ( !var->antialias ) TRUNC4(x1,y1,x2,y2);
	if ( !full && IS_AA) {
		Bool ok;
		Color color;
		FillPattern fp;
		NPoint r[5] = { {x1,y1}, {x2,y1}, {x2,y2}, {x1,y2}, {x1,y1} };
		color = apc_gp_get_color(self);
		memcpy(&fp, apc_gp_get_fill_pattern(self), sizeof(FillPattern));
		apc_gp_set_color(self, apc_gp_get_back_color(self));
		apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
		ok = apc_gp_aa_fill_poly( self, 5, r);
		apc_gp_set_fill_pattern(self, fp);
		apc_gp_set_color(self, color);
		return ok;
	} else return apc_gp_clear(self,
		x1,y1,x2,y2
	);
}

Bool
Drawable_fill_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	if (IS_AA) {
		return primitive( self, 1, "snnnnnn", "chord", x, y, dX, dY, startAngle, endAngle);
	} else return apc_gp_fill_chord(self,
		x, y, dX, dY, startAngle, endAngle
	);
}

Bool
Drawable_fill_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	if (IS_AA) {
		return primitive( self, 1, "snnnn", "ellipse", x, y, dX, dY);
	} else return apc_gp_fill_ellipse(self,
		x, y, dX, dY
	);
}

Bool
Drawable_fill_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	if (IS_AA) {
		return primitive( self, 1, "snnnnnn", "sector", x, y, dX, dY, startAngle, endAngle);
	} else return apc_gp_fill_sector(self,
		x, y, dX, dY, startAngle, endAngle
	);
}

Bool
Drawable_fillpoly(Handle self, SV * points)
{
	int count;
	void *p;
	Bool ret = false;
	Bool do_free = true;
	CHECK_GP(false);

	if (( p = prima_read_array(
		points, "fillpoly",
		IS_AA ? 'd' : 'i',
		2, 2, -1, &count, 
		(var->alpha < 255 && !var->antialias) ? NULL : &do_free
	)) == NULL)
		return false;

	if ( var->alpha < 255 && !var->antialias ) {
		int i;
		NPoint *pp = (NPoint*)p;
		for ( i = 0; i < count; i++, pp++) TRUNC2(pp->x,pp->y);
	}

	ret = IS_AA ?
		apc_gp_aa_fill_poly( self, count, (NPoint*) p) :
		apc_gp_fill_poly( self, count, (Point*) p);
	if ( !ret) perl_error();
	if ( do_free ) free(p);

	return ret;
}

Bool
Drawable_line(Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);
	if (IS_AA)
		return primitive( self, 0, "snnnn", "line", x1, y1, x2, y2);
	else return apc_gp_line(self,
		x1, y1, x2, y2
	);
}

static Bool
read_polypoints( Handle self, SV * points, char * procName, int min, Bool (*procPtr)(Handle,int,Point*))
{
	int count;
	Point * p;
	Bool ret = false;
	Bool do_free;
	if (( p = (Point*) prima_read_array( points, procName, 'i', 2, min, -1, &count, &do_free)) != NULL) {
		ret = procPtr( self, count, p);
		if ( !ret) perl_error();
		if ( do_free ) free(p);
	}
	return ret;
}

Bool
Drawable_lines(Handle self, SV * lines)
{
	CHECK_GP(false);

	if (IS_AA)
		return primitive( self, 0, "sS", "lines", lines);
	else
		return read_polypoints( self, lines, "Drawable::lines", 2, apc_gp_draw_poly2);
}

Bool
Drawable_polyline(Handle self, SV * lines)
{
	CHECK_GP(false);

	if (IS_AA)
		return primitive( self, 0, "sS", "line", lines);
	else
		return read_polypoints( self, lines, "Drawable::polyline", 2, apc_gp_draw_poly);
}

Bool
Drawable_rectangle( Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnn", "rectangle", x1,y1,x2,y2) :
		apc_gp_rectangle(self, x1, y1, x2, y2);
}

SV *
Drawable_render_glyph( Handle self, int index, HV * profile)
{
	int flags, *buffer, count;
	SV * ret;
	dPROFILE;
	gpARGS;
	CHECK_GP(NULL_SV);
	gpENTER(NULL_SV);

	flags = ggoUseHints;
	if ( pexist(glyph)   && pget_B(glyph))   flags |= ggoGlyphIndex;
	if ( pexist(hints)   && !pget_B(hints))  flags &= ~ggoUseHints;
	if ( pexist(unicode) && pget_B(unicode)) flags |= ggoUnicode;
	count = apc_gp_get_glyph_outline( self, index, flags, &buffer);
	hv_clear(profile); /* old gencls bork */
	gpLEAVE;

	if ( count < 0 ) return NULL_SV;
	ret = prima_array_new(sizeof(int) * count);
	memcpy( prima_array_get_storage(ret), buffer, sizeof(int) * count);
	if ( buffer ) free( buffer );
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
	double * knots, int * last_found_knot, Point * result, NPoint * nresult
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
		if ( result )
			result-> x = ( f < 0 ) ? (f - .5) : (f + .5);
		else
			nresult-> x = f;
		f = v[s+1] / v[s+2];
		if ( result )
			result-> y = ( f < 0 ) ? (f - .5) : (f + .5);
		else
			nresult-> y = f;
	} else if ( result ) {
		result-> x = (v[s] < 0) ? (v[s] - .5) : (v[s] + .5);
		result-> y = (v[s+1] < 0) ? (v[s+1] - .5) : (v[s+1] + .5);
	} else {
		nresult-> x = v[s];
		nresult-> y = v[s+1];
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
	NPoint *nrendered, *nstorage;
	SV *ret;
	Bool ok, closed, as_integer;
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

	as_integer = pexist(integer) ? pget_B( integer ) : true;

	p = (NPoint*) prima_read_array( points, "Drawable::render_spline", 'd', 2, degree + 1, -1, &n_points, NULL);
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
		knots = (double*) prima_read_array( pget_sv(knots), "knots", 'd', 1,
			n_points + degree + 1, n_points + degree + 1, NULL, NULL);
		if (!knots) goto EXIT;
	} else
		knots = default_knots(n_points, degree, !closed);

	if ( pexist( weights )) {
		weights = (double*) prima_read_array(pget_sv(weights), "weights", 'd', 1,
			n_points, n_points, NULL, NULL);
		if (!weights) goto EXIT;
		dim = 3;
	} else {
		weights = NULL; /* all ones */
		dim = 2;
	}

	/* allocate result storage */
	precision *= n_points - n_add_points;
	ret = prima_array_new(( precision + 1) * (as_integer ? sizeof(Point) : sizeof(NPoint)) );

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
	if ( as_integer ) {
		nrendered = nstorage = NULL;
		rendered  = storage  = (Point*) prima_array_get_storage(ret);
	} else {
		rendered  = storage  = NULL;
		nrendered = nstorage = (NPoint*) prima_array_get_storage(ret);
	}
	k = -1;
	last_tangent = -1;
	for ( i = 0, t = 0.0, dt = 1.0 / precision; i < precision - 1; i++, t += dt) {
		memcpy( temp, weighted, temp_size);
		if (!render_point(t, degree, n_points, dim, temp, knots, &k, rendered, nrendered))
			goto EXIT;
		if ( as_integer && i > 0 ) {
			/* primitive line detection */
			tangent = tangent_detect( rendered-1, rendered);
			if ( tangent == 0 ) continue;
			if ( final_size > 1 && tangent > 0 && tangent == last_tangent) {
				tangent_apply( tangent, rendered-1);
				continue;
			} else if ( final_size > 1 && cut_corner(last_tangent, tangent, rendered-2, rendered-1)) {
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
		if ( as_integer )
			rendered++;
		else
			nrendered++;
	}
	memcpy( temp, weighted, temp_size);
	if ( !render_point(1.0, degree, n_points, dim, temp, knots, &k, rendered, nrendered))
		goto EXIT;
	final_size++;
	rendered++;
	nrendered++;

	/* looks good */
	ok = true;
	if ( closed ) {
		final_size++;
		if ( as_integer ) {
			*rendered = storage[0];
			rendered++;
		} else {
			*nrendered = nstorage[0];
			nrendered++;
		}
	}
	if ( final_size == 1 ) {
		final_size = 2;
		if ( as_integer )
			storage[1] = storage[0];
		else
			nstorage[1] = nstorage[0];
	}
	prima_array_truncate( ret, final_size * (as_integer ? sizeof( Point) : sizeof(NPoint)) );

EXIT:
	hv_clear(profile); /* old gencls bork */
	if (weighted) free(weighted);
	if (p)        free(p);
	if (knots)    free(knots);
	if (weights)  free(weights);
	if ( ok ) {
		return prima_array_tie( ret,
			as_integer ? sizeof(int) : sizeof(double),
			as_integer ? "i" : "d"
		);
	} else {
		if (ret)  sv_free(ret);
		return newRV_noinc(( SV *) newAV());
	}
}

SV *
Drawable_render_polyline( SV * obj, SV * points, HV * profile)
{
	dPROFILE;
	int count;
	Bool free_input = false, free_buffer = false, as_integer = false;
	double *input = NULL, *buffer = NULL, box[4];
	SV * ret;
	void * storage;

	if (( input = (double*) prima_read_array( points, "render_polyline", 'd', 2, 1, -1, &count, &free_input)) == NULL)
		goto FAIL;

	if ( pexist(integer)) as_integer = pget_B(integer);

	if ( pexist(matrix) ) {
		int i;
		double *src, *dst, *cmatrix;
		if (( cmatrix = (double*) prima_read_array(
			pget_sv(matrix),
			"render_polyline.matrix", 'd', 1, 6, 6, NULL, NULL)
		) == NULL) 
			goto FAIL;
		if ( !( buffer = malloc(sizeof(double) * 2 * count))) {
			free(cmatrix);
			warn("Not enough memory");
			goto FAIL;
		}
		free_buffer = true;

		for ( i = 0, src = input, dst = buffer; i < count; i++) {
			double x,y;
			x = *(src++);
			y = *(src++);
			*(dst++) = cmatrix[0] * x + cmatrix[2] * y + cmatrix[4];
			*(dst++) = cmatrix[1] * x + cmatrix[3] * y + cmatrix[5];
		}
		free(cmatrix);
	} else {
		buffer = input;
		free_buffer = false;
	}

	if ( pexist(box) && pget_B(box)) {
		int i;
		double *src;
		box[0] = box[2] = buffer[0];
		box[1] = box[3] = buffer[1];
		for ( i = 1, src = buffer + 2; i < count; i++) {
			double x,y;
			x = *(src++);
			y = *(src++);
			if ( box[0] > x ) box[0] = x;
			if ( box[1] > y ) box[1] = y;
			if ( box[2] < x ) box[2] = x;
			if ( box[3] < y ) box[3] = y;
		}
		box[2] -= box[0] - 1;
		box[3] -= box[1] - 1;
		if ( as_integer ) {
			box[0] = floor(box[0]);
			box[1] = floor(box[1]);
		}
		if ( free_buffer ) free(buffer);
		free_buffer = false;
		buffer = box;
		count  = 2;
	}

	ret = prima_array_new(count * 2 * (as_integer ? sizeof(int) : sizeof(double)));
	storage = prima_array_get_storage(ret);
	if ( as_integer ) {
		int i, *dst;
		double *src;
		for ( i = 0, src = buffer, dst = (int*)storage; i < count; i++) {
			register double x;
			x = *(src++);
			*(dst++) = x + ((x < 0) ? -.5 : +.5);
			x = *(src++);
			*(dst++) = x + ((x < 0) ? -.5 : +.5);
		}
	} else
		memcpy(storage, buffer, count * 2 * sizeof(double));

	if ( free_buffer ) free( buffer );
	if ( free_input ) free(input);
	hv_clear(profile); /* old gencls bork */

	return prima_array_tie( ret,
		as_integer ? sizeof(int) : sizeof(double),
		as_integer ? "i" : "d");

FAIL:
	if ( free_buffer ) free( buffer );
	if ( free_input ) free(input);
	hv_clear(profile); /* old gencls bork */
	return NULL_SV;
}

PRGBColor
prima_read_palette( int * palSize, SV * palette)
{
	AV * av;
	int i, count;
	Byte * buf;

	if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV)) {
		*palSize = 0;
		return NULL;
	}
	av = (AV *) SvRV( palette);
	count = av_len( av) + 1;
	*palSize = count / 3;
	count = *palSize * 3;
	if ( count == 0) return NULL;

	if ( !( buf = allocb( count))) return NULL;

	for ( i = 0; i < count; i++)
	{
		SV **itemHolder = av_fetch( av, i, 0);
		if ( itemHolder == NULL)
			return ( PRGBColor) buf;
		buf[ i] = SvIV( *itemHolder);
	}

	return ( PRGBColor) buf;
}

Bool
Drawable_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnnnn", "sector", x, y, dX-1, dY-1, startAngle, endAngle) :
		apc_gp_sector(self, x, y, dX, dY, startAngle, endAngle);
}

/* Properties */

int
Drawable_alpha( Handle self, Bool set, int alpha)
{
	if (!set) return apc_gp_get_alpha( self);
	if ( alpha < 0 ) alpha = 0;
	if ( alpha > 255 ) alpha = 255;
	apc_gp_set_alpha( self, alpha);
	return var->alpha = apc_gp_get_alpha(self);
}

Bool
Drawable_antialias( Handle self, Bool set, Bool aa)
{
	if (set) apc_gp_set_antialias( self, aa );
	return var->antialias = apc_gp_get_antialias( self );
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

SV *
Drawable_fillPattern( Handle self, Bool set, SV * svpattern)
{
	int i;
	if ( !set) {
		AV * av;
		FillPattern * fp = apc_gp_get_fill_pattern( self);
		if ( !fp) return NULL_SV;
		av = newAV();
		for ( i = 0; i < 8; i++) av_push( av, newSViv(( int) (*fp)[i]));
		return newRV_noinc(( SV *) av);
	} else {
		if ( SvROK( svpattern) && ( SvTYPE( SvRV( svpattern)) == SVt_PVAV)) {
			FillPattern fp;
			AV * av = ( AV *) SvRV( svpattern);
			if ( av_len( av) != 7) {
				warn("Illegal fillPattern passed to Drawable::fillPattern");
				return NULL_SV;
			}
			for ( i = 0; i < 8; i++) {
				SV ** holder = av_fetch( av, i, 0);
				if ( !holder) {
					warn("Array panic on Drawable::fillPattern");
					return NULL_SV;
				}
				fp[ i] = SvIV( *holder);
			}
			apc_gp_set_fill_pattern( self, fp);
		} else {
			int id = SvIV( svpattern);
			if (( id < 0) || ( id > fpMaxId)) {
				warn("fillPattern index out of range passed to Drawable::fillPattern");
				return NULL_SV;
			}
			apc_gp_set_fill_pattern( self, fillPatterns[ id]);
		}
	}
	return NULL_SV;
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
	return NULL_SV;
}


double
Drawable_lineWidth( Handle self, Bool set, double lineWidth)
{
	if (!set) return apc_gp_get_line_width( self);
	if ( lineWidth < 0.0 ) lineWidth = 0.0;
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
	if ( var-> stage > csFrozen) return NULL_SV;
	colors = var-> palSize;
	if ( set) {
		free( var-> palette);
		var-> palette = prima_read_palette( &var-> palSize, palette);
		if ( colors == 0 && var-> palSize == 0) return NULL_SV; /* do not bother apc */
		apc_gp_set_palette( self);
	} else {
		AV * av = newAV();
		int i;
		Byte * pal = ( Byte*) var-> palette;
		for ( i = 0; i < colors * 3; i++) av_push( av, newSViv( pal[ i]));
		return newRV_noinc(( SV *) av);
	}
	return NULL_SV;
}

SV *
Drawable_pixel( Handle self, Bool set, int x, int y, SV * color)
{
	CHECK_GP(0);
	if (!set)
		return newSViv( apc_gp_get_pixel( self, x, y));
	apc_gp_set_pixel( self, x, y, SvIV( color));
	return NULL_SV;
}

Handle
Drawable_region( Handle self, Bool set, Handle mask)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( !is_opt(optSystemDrawable))
		return NULL_HANDLE;

	if ( set) {
		if ( mask && kind_of( mask, CRegion)) {
			apc_gp_set_region( self, mask);
			return NULL_HANDLE;
		}

		if ( mask && !kind_of( mask, CImage)) {
			warn("Illegal object reference passed to Drawable::region");
			return NULL_HANDLE;
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
			apc_gp_set_region(self, NULL_HANDLE);

	} else if ( apc_gp_get_region( self, NULL_HANDLE)) {
		HV * profile = newHV();
		Handle i = Object_create( "Prima::Region", profile);
		sv_free(( SV *) profile);
		apc_gp_get_region( self, i);
		--SvREFCNT( SvRV((( PAnyObject) i)-> mate));
		return i;
	}

	return NULL_HANDLE;
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

#ifdef __cplusplus
}
#endif
