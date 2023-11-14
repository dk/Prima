#include "apricot.h"
#include "Drawable.h"
#include "Drawable_private.h"

#ifdef __cplusplus
extern "C" {
#endif

void
Drawable_clear_font_abc_caches( Handle self)
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

	/* assigning values */
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
		dmARGS;
		dmENTER( newRV_noinc(( SV *) newAV()));
		abc = func( self, first, last, flags );
		dmLEAVE;
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
	dmCHECK(NULL_SV);
	return Drawable_get_font_abcdef( self, first, last, flags, apc_gp_get_font_abc);
}

SV *
Drawable_get_font_def( Handle self, int first, int last, int flags)
{
	dmCHECK(NULL_SV);
	return Drawable_get_font_abcdef( self, first, last, flags, apc_gp_get_font_def);
}

SV *
Drawable_get_font_languages( Handle self)
{
	char *buf, *p;
	AV * av = newAV();
	dmARGS;

	dmCHECK(NULL_SV);
	dmENTER( newRV_noinc(( SV *) av));
	p = buf = apc_gp_get_font_languages( self);
	dmLEAVE;
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
	dmARGS;

	dmCHECK(NULL_SV);
	dmENTER( newRV_noinc(( SV *) av));
	ret = apc_gp_get_font_ranges( self, &count);
	dmLEAVE;
	if ( ret) {
		int i;
		for ( i = 0; i < count; i++)
			av_push( av, newSViv( ret[i]));
		free( ret);
	}
	return newRV_noinc(( SV *) av);
}

Font
Drawable_get_font( Handle self)
{
	return var-> font;
}

void
Drawable_set_font( Handle self, Font font)
{
	Drawable_clear_font_abc_caches( self);
	apc_font_pick( self, &font, &var-> font);
	apc_gp_set_font( self, &var-> font);
}


#ifdef __cplusplus
}
#endif
