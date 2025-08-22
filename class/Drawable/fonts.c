#include "apricot.h"
#include "Drawable.h"
#include "private/Drawable.h"

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
	if (( u = var-> glyph_descents)) {
		int i;
		for ( i = 0; i < u-> count; i += 2)
			free(( void*) u-> items[ i + 1]);
		plist_destroy( u);
		var-> glyph_descents = NULL;
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
	if ( var->underline_info ) {
		free(var->underline_info);
		var->underline_info = NULL;
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
#define SRC(x)  (!source->undef.x)
#define DST(x)  (!dest->undef.x)
#define COPY(x) if (SRC(x)) { dest->x = source->x; dest->undef.x = 0; }

	/* assigning values */
	if ( dest != source) {
		COPY(height);
		COPY(width);
		COPY(direction);
		COPY(style);
		COPY(pitch);
		COPY(size);
		COPY(vector);
		if (SRC(name)) {
			strcpy( dest-> name, source-> name);
			dest->is_utf8.name = source->is_utf8.name;
			dest->undef.name = 0;
		}
		if ( SRC(encoding)) {
			strcpy( dest-> encoding, source-> encoding);
			dest->is_utf8.encoding = source->is_utf8.encoding;
			dest->undef.encoding = 0;
		}
	}

	/* resolving ambiguities */
	if ( SRC(height))
		dest->undef.size   = 1;
	else if (SRC(size))
		dest->undef.height = 1;
	if ( DST(size) && DST(height))
		dest->undef.size   = 1;

	if ( !SRC(width) && (DST(pitch) || DST(height) || DST(name) || DST(size) || DST(direction) || DST(style)))
		dest->undef.width  = 1;
	if ( !SRC(pitch) && (DST(name) || DST(size) || DST(direction) || DST(style)))
		dest->undef.pitch  = 1;

	/* syncing undef flags with values that are treated as undefs in the apc */
	/* size,height,style,direction, and encoding don't have speclly treated values */
	if ( dest-> pitch == fpDefault)
		dest->undef.pitch  = 1;
	if ( dest-> vector == fvDefault)
		dest->undef.vector = 1;
	if ( dest-> width == 0)
		dest->undef.width  = 1;
	if ( dest-> name[0] == 0 || strcmp(dest->name, "Default") == 0 ) {
		dest->undef.name   = 1;
	}

	/* ... and in the other direction */
	if ( !DST(size))
		dest->size         = 0;
	if ( !DST(height))
		dest->height       = 0;
	if ( !DST(direction))
		dest->direction    = 0;
	if ( !DST(style))
		dest->style        = 0;
	if ( !DST(pitch))
		dest->pitch        = fpDefault;
	if ( !DST(vector))
		dest->vector       = fvDefault;
	if ( !DST(width))
		dest->width        = 0;
	if ( !DST(name)) {
		strcpy( dest-> name, "Default");
		dest->is_utf8.name = false;
	}
	if ( !DST(encoding))
		dest->encoding[0]  = 0;

	/* validating the good entries */
#define CLAMP(x) if ( DST(x) && dest->x <= 0) dest->x = 1; else if ( dest->x > 16383 ) dest->x = 16383
	CLAMP(size);
	CLAMP(height);
	CLAMP(width);
#undef CLAMP

	if (DST(size))
		dest-> size = FONT_SIZE_ROUND(dest->size);

	return dest->undef.height;
#undef SRC
#undef DST
#undef COPY
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
	opt_clear(optFontTrigCache);
}


#ifdef __cplusplus
}
#endif
