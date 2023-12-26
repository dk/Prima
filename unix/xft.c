/********************************/
/*                               */
/*  Xft - client-side X11 fonts  */
/*                               */
/*********************************/


/*

USAGE
-----
To use specific Xft fonts, set Prima font names in your X resource
database in fontconfig formats - for example, Palatino-12. Consult
'man fontconfig' for the syntax, but be aware that Prima uses only
size, weight, and name fields.

IMPLEMENTATION NOTES
--------------------

This implementations doesn't work with non-scalable Xft fonts,
since their rasterization capabilities are currently ( June 2003) very limited -
no scaling and no rotation. Plus, the selection of the non-scalable fonts is
a one big something, and I believe that one in apc_font.c is enough.

The following Xft/fontconfig problems, if fixed in th next versions, need to be
taken into the consideration:
- no font/glyph data - internal leading, underscore size/position,
	no strikeout size/position, average width.
- no raster operations
- no glyph reference point shift
- no on-the-fly antialiasing toggle
- no text background painting ( only rectangles )
- no text strikeout / underline drawing routines
- produces xlib failures for large polygons - answered to be a Xrender bug;
	the X error handler catches this now.

TO DO
-----
- The full set of raster operations - not supported by xft ( stupid )
- apc_show_message - probably never will be implemented though
- Investigate if ICONV can be replaced by native perl's ENCODE interface
- Under some circumstances Xft decides to put antialiasing aside, for
	example, on the paletted displays. Check, if some heuristics that would
	govern whether Xft is to be used there, are needed.

*/

#include "unix/guts.h"

#ifdef USE_XFT

#ifndef WITH_FONTCONFIG
#error "panic: WITH_FONTCONFIG is not defined"
#endif
#ifndef WITH_FREETYPE
#error "panic: WITH_FREETYPE is not defined"
#endif

#ifdef WITH_HARFBUZZ
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#endif

#define MY_MATRIX (PDrawable(self)->current_state.matrix)

#define XFTdebug(...) if (pguts->debug & DEBUG_FONTS) prima_debug2("xf", __VA_ARGS__)

#define MAX_GLYPH_SIZE (guts.limits.request_length / 256)

typedef struct {
	Font font;
	XftFont *orig, *xft_font;
	XftFont *orig_base, *xft_base_font;
	uint32_t fid;
	uint16_t *fonts;
	Matrix matrix;
	float width_factor;
} FontContext;

static void
font_context_init( FontContext * fc, Font * font, uint16_t * fonts, XftFont * orig, XftFont * base, float width_factor, Matrix matrix)
{
	bzero(fc, sizeof(FontContext));
	fc->font      = *font;
	fc->orig      = fc->xft_font = orig;
	fc->orig_base = fc->xft_base_font = base;
	fc->fid       = 0;
	fc->fonts     = fonts;
	fc->width_factor = width_factor;
	COPY_MATRIX(matrix, fc->matrix);
}

static void
font_context_next( FontContext * fc )
{
	PCachedFont kf;
	Font *_src, src, dst;
	uint16_t nfid;

	if ( !fc-> fonts ) return;

	nfid = *(fc->fonts++);
	if ( nfid == fc-> fid ) return;

	fc->fid = nfid;
	if ( nfid == 0 ) {
		fc->xft_font = fc->orig;
		fc->xft_base_font = fc->orig_base;
		return;
	}

	if ( !( _src = prima_font_mapper_get_font(nfid)))
		return;

	src = *_src;
	dst = fc->font;
#define CP(x) src.x = dst.x; src.undef.x = 0;
	CP(size)
	CP(direction)
#undef CP

	if ( !( kf = prima_font_pick( &src, fc->matrix, &dst, FONTKEY_XFT)))
		return;
	fc-> xft_font = kf-> xft;

	if ( !fc->orig_base )
		return;
	if ( IS_ZERO(fc->font.direction) && prima_matrix_is_translated_only(fc->matrix))
		fc-> xft_base_font = fc->xft_font;
	else {
		dst.direction = 0.0;
		if ( !( kf = prima_font_pick( &dst, fc->matrix, NULL, FONTKEY_XFT)))
			return;
		fc-> xft_base_font = kf-> xft;
	}
}

void
prima_xft_init(void)
{
	if ( !apc_fetch_resource( "Prima", "", "UseXFT", "usexft",
				NULL_HANDLE, frUnix_int, &guts.use_xft))
		guts. use_xft = 1;
	if ( guts. use_xft && !XftInit(0))
		guts. use_xft = 0;
	if ( !guts. use_xft) return;

	/* After this point guts.use_xft must never be altered */
	XFTdebug("XFT ok");
}

void
prima_xft_done(void)
{
}

void
prima_xft_build_key( PFontKey key, PFont f, Matrix matrix, Bool bySize)
{
	key-> height    = bySize ? -f-> size * FONT_SIZE_GRANULARITY : f-> height;
	key-> width     = f-> width;
	key-> style     = f-> style;
	key-> pitch     = f-> pitch;
	key-> vector    = (f-> vector == fvBitmap) ? 0 : 1;
	key-> direction = ROUND2INT(f-> direction);
	if ( matrix ) {
		/* don't mix matrix and direction to avoid sin/cos calls here */
		int i;
		for ( i = 0; i < 4; i++) key->matrix[i] = ROUND2INT( matrix[i] );
	} else {
		key->matrix[0] = key->matrix[3] = ROUND_DIRECTION;
	}
	strcpy( key-> name, f-> name);
}

static void
build_mismatch_key( PFontKey key, PFont font)
{
	bzero(key, sizeof(FontKey));
	key->type = FONTKEY_XFT;
	prima_xft_build_key(key, font, NULL, true);
	key->direction = 0;
	key->width = 0;
}

static PCachedFont
try_size( PFont font, double size)
{
	PCachedFont ret;
	Font f = *font;
	if ( size <= 0.0 )
		return NULL;
	bzero( &f.undef, sizeof(f.undef));
	f. undef. height = f. undef. width = 1;
	f. width = f.height = 0;
	f. size = size;
	f. direction = 0.0;
	guts.debug_indent++;
	ret = prima_font_pick( &f, NULL, NULL, FONTKEY_XFT);
	guts.debug_indent--;
	return ret;
}

/* Bug #128146: libXft might use another shared libfontconfig library, as
 * observed on a badly configured macos, and thus FcPattern* returned by it
 * can crash everything */
static FcPattern *
my_XftFontMatch(Display        *dpy,
              int               screen,
              _Xconst FcPattern *pattern,
              FcResult          *result)
{
	FcPattern   *new;
	FcPattern   *match;
	if ( !( new = FcPatternDuplicate (pattern)))
		return NULL;
	FcConfigSubstitute (NULL, new, FcMatchPattern);
	if (dpy) XftDefaultSubstitute (dpy, screen, new);
	match = FcFontMatch (NULL, new, result);
	FcPatternDestroy (new);
	return match;
}

PCachedFont
prima_xft_match( Font *font, Matrix matrix, Bool by_size, PCachedFont cf)
{
	FcPattern *request, *match;
	FcResult res = FcResultNoMatch;
	const char *encoding;
	XftFont * xf;
	FontKey key;
	Font requested_font = *font;
	PCachedFont kf_base = NULL;
	int base_width = 1, exact_pixel_size = 0;
	double pixel_size = font->height;
	Bool transformation_needed;

	if ( !guts. use_xft) return NULL;

	if ( guts. xft_disable_large_fonts > 0) {
		/* xft is unable to deal with large polygon requests.
			we must cut the large fonts here, before Xlib croaks */
		if (
			( by_size && ( font->size >= MAX_GLYPH_SIZE)) ||
			(!by_size && ( font->height >= MAX_GLYPH_SIZE / 72.0 * guts. resolution. y))
		)
			return NULL;
	}

	/* see if the font is not present in xft - the hashed negative matches are stored with width=height=0 */
	build_mismatch_key(&key, font);
	XFTdebug("want %gx%d.%s.%s.%s/%s^%g.%d", by_size ? -font->size : font->height, key.width, _F_DEBUG_STYLE(key.style), _F_DEBUG_PITCH(key.pitch), key.name, font->encoding, ROUGHLY(font->direction), font->vector);
	if ( hash_fetch( guts.fc_mismatch, &key, sizeof( FontKey))) {
		XFTdebug("refuse");
		return NULL;
	}

	/* convert encoding */
	encoding = prima_fc_find_encoding( font->encoding);
	if ( strcmp( encoding, font->encoding ) != 0 ) {
		/* xft has no such encoding, pass it back */
		if ( prima_corefont_encoding( font->encoding) || !guts. xft_priority)
			return NULL;
	}

	/* see if the non-transformed font exists */
	transformation_needed = font->width != 0 || !IS_ZERO(font-> direction) || ( matrix && !prima_matrix_is_translated_only(matrix));
	if ( transformation_needed ) {
		Font f = *font;
		f.direction = 0.0;
		f.width     = 0;
		f.undef.width = 1;
		XFTdebug("try non-transformed font");
		guts.debug_indent++;
		kf_base = prima_font_pick( &f, NULL, NULL, FONTKEY_XFT);
		guts.debug_indent--;
		if ( !kf_base ) {
			XFTdebug("cannot, bail out");
			return NULL;
		}
		*cf   = *kf_base;
		*font = f;
		font-> direction  = requested_font.direction;
		font-> width      = requested_font.width;

		base_width = kf_base-> font.width;
		if ( FcPatternGetDouble( kf_base-> xft-> pattern, FC_PIXEL_SIZE, 0, &pixel_size) == FcResultMatch) {
			exact_pixel_size = 1;
			XFTdebug("existing base font %x %gx0 suggests exact_pixel_size %g base_width %d", kf_base->xft, 
				(double) key.height / ((key.height > 0) ? 1 : FONT_SIZE_GRANULARITY),
				pixel_size, base_width);
		}
	}

	/* create FcPattern request */
	if ( !( request = FcPatternCreate()))
		return NULL;
	if ( strcmp( font-> name, "Default") != 0)
		FcPatternAddString( request, FC_FAMILY, ( FcChar8*) font-> name);
	if ( by_size) {
		FcPatternAddDouble( request, FC_SIZE, font-> size);
		XFTdebug("FC_SIZE = %.1f", font->size);
	} else {
		FcPatternAddDouble( request, FC_PIXEL_SIZE, pixel_size);
		XFTdebug("FC_PIXEL_SIZE = %g", pixel_size);
	}
	FcPatternAddInteger( request, FC_SPACING,
		(font-> pitch == fpFixed && guts.force_fc_monospace_emulation) ? FC_MONO : FC_PROPORTIONAL);

	FcPatternAddInteger( request, FC_SLANT, ( font-> style & fsItalic) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
	FcPatternAddInteger( request, FC_WEIGHT,
		( font-> style & fsBold) ? FC_WEIGHT_BOLD :
		( font-> style & fsThin) ? FC_WEIGHT_THIN : FC_WEIGHT_NORMAL);
	FcPatternAddBool( request, FC_SCALABLE, font-> vector > fvBitmap );
	if ( transformation_needed ) {
		FcMatrix mat;
		FcMatrixInit(&mat);
		if ( font-> width != 0) {
			FcMatrixScale( &mat, ( double) font-> width / base_width, 1);
			XFTdebug("FcMatrixScale %d/%d=%g", font->width, base_width, ( double) font-> width / base_width);
		}
		if ( !IS_ZERO(requested_font. direction)) {
			FcMatrixRotate( &mat,
				cos(font->direction * 3.14159265358 / 180.0),
				sin(font->direction * 3.14159265358 / 180.0)
			);
		}
		if ( matrix ) {
			FcMatrix result;
			FcMatrix m = { matrix[0], matrix[2], matrix[1], matrix[3] };
			FcMatrixMultiply( &result, &mat, &m);
			XFTdebug("FcMatrixMutiply %g %g %g %g", matrix[0], matrix[1], matrix[2], matrix[3]);
			mat = result;
		}
		FcPatternAddMatrix( request, FC_MATRIX, &mat);
	}

	/* match best font - must return something useful; the match is statically allocated */
	match = my_XftFontMatch( DISP, SCREEN, request, &res);
	if ( !match) {
		XFTdebug("XftFontMatch error");
		FcPatternDestroy( request);
		return NULL;
	}
	/* if (pguts->debug & DEBUG_FONTS) { FcPatternPrint(match); } */
	FcPatternDestroy( request);

	/* convert Ubuntu[pitch=fpFixed] to Ubuntu Mono because the freetype-synthesized mono looks ugly */
	{
		int fcam;
		Font suggested_font;
		fcam = prima_fc_suggest_pitch(font, &suggested_font, match);
		if ( fcam > 0 ) {
			PCachedFont kf;
			if (( kf = prima_font_pick( &suggested_font, matrix, NULL, FONTKEY_XFT)) != NULL ) {
				*font = suggested_font;
				prima_fc_end_suggestion(fcam);
				FcPatternDestroy(match);
				return kf;
			};
			prima_fc_end_suggestion(fcam);
		}
	}

	if ( !prima_fc_encoding_is_supported(requested_font.encoding, match)) {
		FcPatternDestroy( match);
		return NULL;
	}

	/* Check if the matched font is scalable -- see comments in the beginning
		of the file about non-scalable fonts in Xft */
	if ( requested_font. vector > fvBitmap ) {
		FcBool scalable;
		if (( FcPatternGetBool( match, FC_SCALABLE, 0, &scalable) == FcResultMatch) && !scalable) {
			build_mismatch_key(&key, font);
			hash_store( guts.fc_mismatch, &key, sizeof( FontKey), (void*)1);
			XFTdebug("refuse bitmapped font");
			FcPatternDestroy( match);
			return NULL;
		}
	}


	/* check name match */
	{
		FcChar8 * s = NULL;
		FcPatternGetString( match, FC_FAMILY, 0, &s);
		if ( !s || strcmp(( const char*) s, requested_font.name) != 0) {
			int i, n = guts. n_fonts;
			PFontInfo info = guts.font_info;

			if ( !guts. xft_priority) {
				XFTdebug("name mismatch");
			NAME_MISMATCH:
				build_mismatch_key( &key, &requested_font);
				hash_store( guts.fc_mismatch, &key, sizeof( FontKey), (void*)1);
				FcPatternDestroy( match);
				return NULL;
			}

			/* check if core has cached face name */
			bzero( &key, sizeof(key));
			key.type = FONTKEY_CORE;
			prima_corefont_build_key( &key, &requested_font, by_size);
			if ( hash_fetch( guts.font_hash, &key, sizeof( FontKey))) {
				XFTdebug("pass to cached core");
				goto NAME_MISMATCH;
			}

			/* check if core has non-cached face name */
			for ( i = 0; i < n; i++) {
				if (
					info[i]. flags. disabled ||
					!info[i].flags.name ||
					(strcmp( info[i].font.name, requested_font.name) != 0)
				) continue;
				XFTdebug("pass to core");
				goto NAME_MISMATCH;
			}
		}
	}

	/* strangely enough, if the match is used after XftFontOpenPattern, it is destroyed */
	if (!kf_base)
		prima_fc_pattern2font( match, font);

	/* load the font */
	if ( !( xf = XftFontOpenPattern( DISP, match))) {
		build_mismatch_key( &key, &requested_font);
		hash_store( guts.fc_mismatch, &key, sizeof( FontKey), (void*)1);
		XFTdebug("XftFontOpenPattern error");
		FcPatternDestroy( match);
		return NULL;
	}
	XFTdebug("load font %x", xf);

	cf-> xft       = xf;
	cf-> xft_base  = kf_base ? kf_base-> xft : xf;
	cf-> xft_width_factor = (transformation_needed && font->width > 0.0) ? (float) font->width / base_width : 1.0;
	if ( !kf_base) {
		font->internalLeading = xf-> height - font->size * guts. resolution. y / 72.0 + 0.5;
		if ( !by_size && !exact_pixel_size) {
			/* Try to locate the corresponding size and
			the correct height - FC_PIXEL_SIZE is not correct most probably
			multiply size by X to address pixel-wise heights correctly.
			*/
			HeightGuessStack hgs;
			int h;
			double sz, last_sz = -1;
			PCachedFont guessed_font;

			sz = font-> size * (double) font-> height / (double) xf->height;
			XFTdebug("need to figure the corresponding size - try %g first...", sz);
			if (( guessed_font = try_size( font, sz)) != NULL ) {
				h = guessed_font-> font.height;
				XFTdebug("got height = %d", h);
				if ( h != requested_font. height) {
					XFTdebug("not good enough, try approximation from size %g", sz);
					prima_font_init_try_height( &hgs, requested_font. height, sz * FONT_SIZE_GRANULARITY + .5);
					while ( 1) {
						last_sz = sz;
						sz = prima_font_try_height( &hgs, h) / FONT_SIZE_GRANULARITY;
						if ( sz < 0) break;
						if ( !( guessed_font = try_size( font, sz)))
							break;
						h = guessed_font-> font.height;
						XFTdebug("size %.1f got us %d pixels", sz, h);
						if ( h == 0 || h == font-> height)
							break;
					}
				}
				if ( sz < 0)
					sz = last_sz;
				if ( sz > 0) {
					font-> size = FONT_SIZE_ROUND(sz);
					XFTdebug("found size: %.1f", font->size);
				}
			} else {
				XFTdebug("found nothing");
			}

			if ( guessed_font && requested_font. height != guessed_font-> font.height) {
				cf->xft = xf  = guessed_font->xft;
				cf->xft_base  = guessed_font->xft_base;
				font-> height = guessed_font->font.height;
				XFTdebug("redirect to font %x", xf);
			}
			XFTdebug("guessed size %g", font->size);
		} else {
			font-> height = xf-> height;
			XFTdebug("set height: %d", font->height);
		}
		font-> maximalWidth = xf-> max_advance_width;
		/* XXX FT_Face reports very strange values */
		font->underlinePosition  = -xf-> descent;
		font->underlineThickness = (font->height > 16) ? font->height / 16 : 1;

		if ( font->pitch != fpFixed) {
			/* XXX 
			detail the width to conform a bit more to win32 definition of font width -
			I believe though that the whole concept of width needs reworking because win32
			has one idea what width is, both when picking up and when detailing font info,
			while freetype simple doesn't use either
			*/
			FcChar32 c;
			int num = 0, sum = 0;
			XftFont *x = kf_base ? kf_base->xft : xf;
			for ( c = 63; c < 126; c += 4 ) {
				XGlyphInfo g;
				FT_UInt ix;
				if (( ix = XftCharIndex( DISP, x, c)) == 0)
					continue;
				XftGlyphExtents( DISP, x, &ix, 1, &g);
				if ( g.xOff > 0 && g.xOff <= x-> max_advance_width ) {
					sum += g.xOff;
					num++;
				} else
					XFTdebug("!! font %s returns bad XftGlyphExtents", font->name);
			}

			font->width = (num > 10) ? ((float) sum / num + .5) : font->maximalWidth;
			XFTdebug("set width: %d based off %d samples", font->width, num);
		} else
			font->width = font->maximalWidth;
	} else
		font-> width = ( requested_font.width > 0 ) ? requested_font.width : base_width;

	font-> descent = cf-> xft_base-> descent;
	font-> ascent  = cf-> xft_base-> ascent;
	if ( font-> descent + font-> ascent != font-> height ) {
		XFTdebug("descent %d + ascent %d != height %d, fixing", font-> descent, font-> ascent, font-> height );
		font-> ascent = font-> height - font-> descent;
	}

	return cf;
}

static FcChar32 *
xft_text2ucs4( const unsigned char * text, int len, Bool utf8, uint32_t * map8)
{
	FcChar32 *ret, *r;
	if ( utf8) {
		unsigned int charlen, bytelen = strlen(( const char *) text);
		(void)bytelen;

		if ( len < 0) len = prima_utf8_length(( char*) text, -1);
		if ( !( r = ret = malloc( len * sizeof( FcChar32)))) return NULL;
		while ( len--) {
			*(r++) = prima_utf8_uvchr((char *) text, bytelen, &charlen);
			text += charlen;
			bytelen -= charlen;
			if ( charlen == 0 ) break;
		}
	} else {
		int i;
		if ( len < 0) len = strlen(( char*) text);
		if ( !( ret = malloc( len * sizeof( FcChar32)))) return NULL;
		for ( i = 0; i < len; i++)
			ret[i] = ( text[i] < 128) ? text[i] : map8[ text[i] - 128];
	}
	return ret;
}

/*
x11 has problems with text_out strings that are wider than
64K pixel - it wraps the coordinates and produces mess. This hack is
although ugly, but is better that X11 default behavior, and
at least can be excused by the fact that all GP spaces have
their geometrical limits.
*/
static int
check_width(PCachedFont self, int len)
{
	int div;
	div = 65535L / (self-> font. maximalWidth ? self-> font. maximalWidth : 1);
	if ( div <= 0) div = 1;
	if ( len > div) len = div;
	return len;
}

#define UPDATE_OVERHANGS(_len,_flags,wf)                                          \
	if ( i == 0) {                                                            \
		if (( _flags & toAddOverhangs ) && glyph. x > 0) ret += glyph. x; \
		if ( overhangs) overhangs-> x = (glyph.x > 0) ? wf * glyph. x + .5 : 0;\
	}                                                                         \
	if ( i == _len - 1) {                                                     \
		int c = glyph. xOff - glyph. width + glyph. x;                    \
		if ( (_flags & toAddOverhangs) && c < 0) ret -= c;                \
		if ( overhangs) overhangs-> y = (c < 0) ? -c *wf + .5 : 0;        \
	}

int
prima_xft_get_text_width(
	PCachedFont self, const char * text, int len, int flags,
	uint32_t * map8, Point * overhangs
) {
	int i, ret = 0, bytelen = 0;
	XftFont * font = self-> xft_base;

	if ( overhangs) overhangs-> x = overhangs-> y = 0;
	if ( flags & toUTF8 ) bytelen = strlen(text);
	len = check_width(self, len);

	for ( i = 0; i < len; i++) {
		FcChar32 c;
		FT_UInt ft_index;
		XGlyphInfo glyph;
		if ( flags & toUTF8) {
			unsigned int charlen;
			c = ( FcChar32) prima_utf8_uvchr(text, bytelen, &charlen);
			text += charlen;
			bytelen -= charlen;
			if ( charlen == 0 ) break;
		} else if ( ((Byte*)text)[i] > 127) {
			c = map8[ ((Byte*)text)[i] - 128];
		} else
			c = text[i];
		ft_index = XftCharIndex( DISP, font, c);
		XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
		ret += glyph. xOff * self->xft_width_factor + .5;
		if ( (flags & toAddOverhangs ) || overhangs) {
			UPDATE_OVERHANGS(len,flags, self->xft_width_factor)
		}
	}
	return ret;
}

int
prima_xft_get_glyphs_width( Handle self, PCachedFont selfxx, PGlyphsOutRec t, Point * overhangs)
{
	int i, ret = 0;
	FontContext fc;

	font_context_init(&fc, &selfxx->font, t->fonts, selfxx->xft_base, NULL, selfxx-> xft_width_factor, MY_MATRIX);
	if ( overhangs) overhangs-> x = overhangs-> y = 0;

	t->len = check_width(selfxx, t->len);
	for ( i = 0; i < t->len; i++) {
		FT_UInt ft_index;
		XGlyphInfo glyph;
		ft_index = t->glyphs[i];
		font_context_next(&fc);
		XftGlyphExtents( DISP, fc.xft_font, &ft_index, 1, &glyph);
		ret += glyph. xOff * fc.width_factor + .5;
		if ( (t->flags & toAddOverhangs ) || overhangs) {
			UPDATE_OVERHANGS(t->len,t->flags,fc.width_factor)
		}
	}
	return ret;
}

Point *
prima_xft_get_text_box( Handle self, const char * text, int len, int flags)
{
	DEFXX;
	Point ovx;
	return prima_get_text_box(self, ovx, 
		prima_xft_get_text_width( XX-> font, text, len, flags, XX-> fc_map8, &ovx)
	);
}

Point *
prima_xft_get_glyphs_box( Handle self, PGlyphsOutRec t)
{
	DEFXX;
	Point ovx;
	return prima_get_text_box(self, ovx,
		prima_xft_get_glyphs_width(self, XX-> font, t, &ovx)
	);
}

#define SORT(a,b)       { int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)       (XX-> size. y - (a) - 1)
#define SHIFT(a,b)      { (a) += XX-> btransform. x; (b) += XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)

/* emulate win32 clearing off alpha bits on any Gdi operation */

#define EMULATE_ALPHA_CLEARING 0

static void
XftDrawGlyph_layered( PDrawableSysData selfxx, 
	_Xconst XftColor *color, XftFont * font, 
	int x, int y, _Xconst FT_UInt glyph
) {
	XftColor black;
	XGlyphInfo extents;

	XftGlyphExtents( DISP, font, &glyph, 1, &extents);

	if (
		!XX-> xft_shadow_pixmap ||
		extents. width  > XX-> xft_shadow_extentions.x ||
		extents. height > XX-> xft_shadow_extentions.y
	) {
		int w, h;
		if ( XX-> xft_shadow_pixmap ) {
			XFreePixmap( DISP, XX-> xft_shadow_pixmap);
			XftDrawDestroy( XX-> xft_shadow_drawable);
			XX-> xft_shadow_pixmap   = 0;
			XX-> xft_shadow_drawable = NULL;
		}
		w = extents. width * 4;
		h = extents. height * 4;
		if ( w < 32 ) w = 32;
		if ( h < 32 ) h = 32;
		XX-> xft_shadow_extentions. x = w;
		XX-> xft_shadow_extentions. y = h;
		XX-> xft_shadow_pixmap   = XCreatePixmap( DISP, guts.root, w, h, 1);
		XX-> xft_shadow_drawable = XftDrawCreateBitmap( DISP, XX-> xft_shadow_pixmap );
	}

	if ( !XX-> xft_shadow_gc ) {
		XGCValues gcv;
		gcv. foreground = 1;
		XX-> xft_shadow_gc = XCreateGC( DISP, XX-> xft_shadow_pixmap, GCForeground, &gcv);
	}

	XFillRectangle( DISP, XX-> xft_shadow_pixmap, XX-> xft_shadow_gc,
		0, 0, XX-> xft_shadow_extentions.x, XX-> xft_shadow_extentions.y);

	black.color.red   =
	black.color.green =
	black.color.blue  =
	black.pixel       = 0x1;
	black.color.alpha = 0x0;
	XftDrawGlyphs( XX-> xft_shadow_drawable, &black, font, extents.x, extents.y, &glyph, 1);
	XftDrawGlyphs( XX-> xft_drawable, color, font, x, y, &glyph, 1);

	XCopyPlane( DISP, XX-> xft_shadow_pixmap, XX-> gdrawable, XX-> gc, 0, 0, extents.width, extents.height,
		x - extents.x, y - extents.y, 1);
}

/* When plotting rotated fonts, xft does not account for the accumulated
	round-off error, and thus the text line is shown at different angle
	than requested. We track this and align the reference point when it
	deviates from the ideal line */
static void
xft_draw_glyphs( Handle self, PDrawableSysData selfxx,
		_Xconst XftColor *color, int x, int y,
		_Xconst FcChar32 *string, int len,
		PGlyphsOutRec t)
{
	XGCValues old_gcv, gcv;
	int i, ox, oy;
	double shift;
	FontContext fc;
	uint16_t * advances  = t ? t->advances : NULL;
	int16_t  * positions = t ? t->positions : NULL;
	Bool straight = IS_ZERO(XX-> font-> font. direction) && prima_matrix_is_translated_only(MY_MATRIX);

	font_context_init(&fc, &XX->font->font, 
		t ? t->fonts : NULL, 
		XX->font->xft, advances ? NULL : XX->font->xft_base,
		XX->font->xft_width_factor,
		MY_MATRIX);

	ox = x;
	oy = y;
	shift = 0.0;
	if ( XX-> flags. layered && EMULATE_ALPHA_CLEARING) {
		FT_UInt ft_index;
		/* prepare xrender */
		XftDrawGlyphs( XX-> xft_drawable, color, XX->font->xft, x, y, &ft_index, 0);

		XGetGCValues( DISP, XX-> gc, GCFunction|GCBackground|GCForeground|GCPlaneMask, &old_gcv);
		gcv. foreground = 0xffffffff;
		gcv. background = 0x00000000;
		gcv. function   = GXand;
		gcv. plane_mask = guts. argb_bits. alpha_mask;
		XChangeGC( DISP, XX-> gc, GCFunction|GCBackground|GCForeground|GCPlaneMask, &gcv);
	}

	if (t) len = t->len;
	for ( i = 0; i < len; i++) {
		int rx, ry;
		double cx, cy, dx = 0, dy = 0;
		FT_UInt ft_index;
		XGlyphInfo glyph;

		if ( t ) {
			ft_index = t->glyphs[i];
			font_context_next(&fc);
			if ( advances ) {
				shift += *(advances++);
				dx = *(positions++);
				dy = *(positions++);
				if ( !straight )
					prima_matrix_apply( XX-> fc_font_matrix, &dx, &dy);
			} else
				goto CHECK_EXTENTS;
		} else {
			ft_index = XftCharIndex( DISP, fc.xft_font, string[i]);
		CHECK_EXTENTS:
			XftGlyphExtents( DISP, fc.xft_base_font, &ft_index, 1, &glyph);
			shift += fc.width_factor * glyph. xOff;
		}
		if ( straight ) {
			cx = ox + shift;
			cy = oy;
		} else {
			double ax = shift, ay = 0;
			prima_matrix_apply( XX-> fc_font_matrix, &ax, &ay);
			cx = ox + ax;
			cy = oy - ay;
		}
		dx += x;
		dy += y;
		rx = (dx > 0) ? dx + .5 : dx - .5;
		ry = (dy > 0) ? dy + .5 : dy - .5;
		if ( XX-> flags. layered && EMULATE_ALPHA_CLEARING)
			XftDrawGlyph_layered( XX, color, fc.xft_font, rx, ry, ft_index);
		else
			XftDrawGlyphs( XX-> xft_drawable, color, fc.xft_font, rx, ry, &ft_index, 1);
		x = (cx > 0) ? cx + .5 : cx - .5;
		y = (cy > 0) ? cy + .5 : cy - .5;
	}

	if ( XX-> flags. layered && EMULATE_ALPHA_CLEARING)
		XChangeGC( DISP, XX-> gc, GCFunction|GCBackground|GCForeground|GCPlaneMask, &old_gcv);
}

static void
my_XftDrawString32( Handle self, PDrawableSysData selfxx,
		_Xconst XftColor *color, int x, int y,
		_Xconst FcChar32 *string, int len)
{
	if ( IS_ZERO(XX-> font-> font. direction) && prima_matrix_is_translated_only(MY_MATRIX) && !XX-> flags. layered )
		XftDrawString32( XX-> xft_drawable, color, XX-> font-> xft, x, y, string, len);
	else
		xft_draw_glyphs( self, XX, color, x, y, string, len, NULL);
}

static int
filter_unsupported_rops( PDrawableSysData selfxx, int rop, XftColor * xftcolor )
{
	/* filter out unsupported rops */
	switch ( rop) {
	case ropBlackness:
		xftcolor->color.red   =
		xftcolor->color.green =
		xftcolor->color.blue  =
		xftcolor->pixel       = 0;
		rop = ropCopyPut;
		break;
	case ropWhiteness:
		xftcolor->color.red   =
		xftcolor->color.green =
		xftcolor->color.blue  = 0xffff;
		xftcolor->pixel       = 0xffffffff;
		rop = ropCopyPut;
		break;
	case ropXorPut:
	case ropOrPut:
	case ropNotSrcAnd:
	case ropNotSrcXor:
	case ropNotSrcOr:
	case ropAndPut:
		xftcolor->color.red   = COLOR_R16(XX->fore.color);
		xftcolor->color.green = COLOR_G16(XX->fore.color);
		xftcolor->color.blue  = COLOR_B16(XX->fore.color);
		xftcolor->pixel       = XX-> fore. primary;
		break;
	case ropNotPut:
		xftcolor->color.red   = COLOR_R16(~XX->fore.color);
		xftcolor->color.green = COLOR_G16(~XX->fore.color);
		xftcolor->color.blue  = COLOR_B16(~XX->fore.color);
		xftcolor->pixel       = ~XX-> fore. primary;
		rop = ropCopyPut;
		break;
	default:
		xftcolor->color.red   = COLOR_R16(XX->fore.color);
		xftcolor->color.green = COLOR_G16(XX->fore.color);
		xftcolor->color.blue  = COLOR_B16(XX->fore.color);
		xftcolor->pixel       = XX-> fore. primary;
		rop = ropCopyPut;
	}

	return rop;
}

static void
setup_alpha(PDrawableSysData selfxx, XftColor * xftcolor, XftFont ** font)
{
	if ( XX-> flags. layered || !XX->type.bitmap) {
		if ( selfxx->flags.antialias ) {
			float div = 65535.0 / (float)(xftcolor->color.alpha = (selfxx->alpha << 8));
			xftcolor->color.red   = (float) xftcolor->color.red   / div;
			xftcolor->color.green = (float) xftcolor->color.green / div;
			xftcolor->color.blue  = (float) xftcolor->color.blue  / div;
		} else
			xftcolor->color.alpha = 0xffff;
	} else {
		xftcolor->color.alpha = (
			(
				xftcolor->color.red/3 + 
				xftcolor->color.green/3 + 
				xftcolor->color.blue/3
			) > (0xff00 / 2)
		) ?
			0xffff :
			0
			;
	}
}

/* emulate over- and understriking */
static void
overstrike( Handle self, int x, int y, Point *ovx, int advance)
{
	DEFXX;
	int lw = 1;
	int d  = PDrawable(self)-> font. underlinePosition;
	int t  = PDrawable(self)-> font. underlineThickness;
	int x1, y1, x2, y2;

	XSetFillStyle( DISP, XX-> gc, FillSolid);
	if ( !XX-> flags. brush_fore) {
		XSetForeground( DISP, XX-> gc, XX-> fore. primary);
		XX-> flags. brush_fore = 1;
	}

	if ( ovx->x < 0 ) ovx->x = 0;
	if ( ovx->y < 0 ) ovx->y = 0;
	advance += ovx->y;

	if ( lw != t ) {
		XGCValues gcv;
		lw = gcv.line_width = t;
		gcv.cap_style = CapNotLast;
		XChangeGC( DISP, XX-> gc, GCLineWidth | GCCapStyle, &gcv);
	}

	if ( PDrawable( self)-> font. style & fsUnderlined) {
		x1 = ovx->x;
		x2 = advance + 1;
		y2 = d;
		y1 = -y2;
		prima_matrix_apply_int_to_int( XX->fc_font_matrix, &x1, &y1);
		prima_matrix_apply_int_to_int( XX->fc_font_matrix, &x2, &y2);
		x1 = x - x1;
		y1 = y - y1;
		x2 = x + x2;
		y2 = y + y2;
		XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
	}

	if ( PDrawable( self)-> font. style & fsStruckOut) {
		x1 = ovx->x;
		x2 = advance + 1;
		y2 = (XX-> font-> font.ascent - XX-> font-> font.internalLeading)/3;
		y1 = -y2;
		prima_matrix_apply_int_to_int( XX->fc_font_matrix, &x1, &y1);
		prima_matrix_apply_int_to_int( XX->fc_font_matrix, &x2, &y2);
		x1 = x - x1;
		y1 = y - y1;
		x2 = x + x2;
		y2 = y + y2;
		XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
	}

	if ( lw != 1 ) {
		XGCValues gcv;
		gcv.line_width = 1;
		XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
	}
}

static void
allocate_xftdraw_surface(PDrawableSysData selfxx)
{
	if ( !XX-> xft_drawable) {
		if ( XX-> type. bitmap)
			XX-> xft_drawable = XftDrawCreateBitmap( DISP, XX-> gdrawable );
		else
			XX-> xft_drawable = XftDrawCreate( DISP,
				XX-> gdrawable, XX->visual->visual, XX->colormap
			);
		XftDrawSetSubwindowMode( XX-> xft_drawable,
			XX-> flags.clip_by_children ? ClipByChildren : IncludeInferiors);
		XCHECKPOINT;
	}
	if ( !XX-> flags. xft_clip) {
		XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
		XX-> flags. xft_clip = 1;
	}
}

typedef struct {
	int x, y, dx, dy, width, height;
	Pixmap canvas;
	GC gc;
} TextBlit;

static Bool
open_text_blit(Handle self, int x, int y, int dx, int rop, TextBlit * tb)
{
	DEFXX;
	XGCValues gcv;
	int i;
	Rect rc;
	Point p[4], offset;

	bzero( &rc, sizeof(rc));
	tb-> x   = x;
	tb-> y   = y;
	tb-> dx  = dx;
	tb-> dy  = XX-> font-> font. height;

	offset. x = offset. y = 0;
	p[0].x = p[2].x = 0;
	p[0].y = p[1].y = 0;
	p[1].x = p[3].x = tb->dx;
	p[2].y = p[3].y = tb->dy;
	rc. left = rc. right = rc. top = rc. bottom = 0;
	for ( i = 1; i < 4; i++) {
		int x = p[i].x, y = p[i].y;
		prima_matrix_apply_int_to_int( XX->fc_font_matrix, &x, &y);
		if ( rc. left    > x) rc. left   = x;
		if ( rc. right   < x) rc. right  = x;
		if ( rc. bottom  > y) rc. bottom = y;
		if ( rc. top     < y) rc. top    = y;
	}
	tb->width  = rc. right  - rc. left   + 1;
	tb->height = rc. top    - rc. bottom + 1;

	tb->canvas = XCreatePixmap( DISP, guts. root,
		tb->width, tb->height,
		XX-> type. bitmap ? 1 : guts. depth);
	if ( !tb->canvas)
		return false;

	tb->dx = -rc. left;
	tb->dy = -rc. bottom;
	tb->gc = XCreateGC( DISP, tb->canvas, 0, &gcv);
	switch ( rop) {
	case ropAndPut:
	case ropNotSrcXor:
	case ropNotSrcOr:
		XSetForeground( DISP, tb->gc, 0xffffffff);
		break;
	default:
		XSetForeground( DISP, tb->gc, 0x0);
		break;
	}
	XFillRectangle( DISP, tb->canvas, tb->gc, 0, 0, tb->width, tb->height);
	XftDrawChange( XX-> xft_drawable, tb->canvas);
	if ( XX-> flags. xft_clip)
		XftDrawSetClip( XX-> xft_drawable, 0);

	return true;
}

static void
close_text_blit(PDrawableSysData selfxx, TextBlit * tb)
{
	XftDrawChange( XX-> xft_drawable, XX-> gdrawable);
	if ( XX-> flags. xft_clip)
		XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
	XCHECKPOINT;
	XCopyArea( DISP,
		tb->canvas, XX-> gdrawable, XX-> gc,
		0, 0, tb->width, tb->height,
		tb->x - tb->dx, REVERT( tb->y - tb->dy + tb->height)
	);
	XFreeGC( DISP, tb->gc);
	XFreePixmap( DISP, tb->canvas);
}

/*
	If you're guided here by something like

		X Error: BadLength (poly request too large or internal Xlib length error),

	then you probably need to know that your X server is out of date.
	The error is caused by a Xrender bug, and you have the following options:
	- update your X server
	- set guts.xft_disable_large_fonts to 1, which would explicitly tell Prima not to wait
	for Xlib to bark on large polygons
	- set guts.xft_disable_large_fonts to 1 and decrease MAX_GLYPH_SIZE, if the former
	option is not enough
*/

Bool
prima_xft_text_out( Handle self, const char * text, int x, int y, int len, int flags)
{
	DEFXX;
	FcChar32 *ucs4;
	XftColor xftcolor;
	XftFont *font = XX-> font-> xft;
	int rop = XX-> rop;
	Point baseline;

	if ( len == 0) return true;
	len = check_width( XX->font, len );
	rop = filter_unsupported_rops( XX, rop, &xftcolor );
	setup_alpha( XX, &xftcolor, &font );

	/* paint background if opaque */
	if ( XX-> flags. opaque) {
		Point * p = prima_xft_get_text_box( self, text, len, flags);
		prima_paint_text_background( self, p, x, y );
		free( p);
	}

	SHIFT(x,y);
	RANGE2(x,y);

	/* xft doesn't allow shifting glyph reference point - need to adjust manually */
	baseline.x = 0;
	baseline.y = PDrawable(self)-> font. descent;
	prima_matrix_apply_int_to_int( XX-> fc_font_matrix, &baseline.x, &baseline.y);
	if ( !XX-> flags. base_line) {
		x += baseline.x;
		y += baseline.y;
	}

	/* convert text string to unicode */
	if ( !( ucs4 = xft_text2ucs4(( unsigned char*) text, len, flags & toUTF8, X( self)-> fc_map8)))
		return false;

	allocate_xftdraw_surface(XX);
	if ( rop != ropCopyPut) {
		/* emulate rops by blitting the text */
		int dx;
		TextBlit tb;
		dx = prima_xft_get_text_width( XX-> font, text, len,
			flags | toAddOverhangs, X(self)-> fc_map8, NULL);
		if (!open_text_blit(self, x - baseline.x, y - baseline.y, dx, rop, &tb))
			goto COPY_PUT;
		my_XftDrawString32( self, XX, &xftcolor,
			tb.dx + baseline.x, tb.height - tb.dy - baseline.y,
			ucs4, len);
		close_text_blit(XX, &tb);
	} else {
	COPY_PUT:
		my_XftDrawString32( self, XX, &xftcolor, x, REVERT( y) + 1, ucs4, len);
	}
	free( ucs4);
	XCHECKPOINT; 

	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {
		Point ovx;
		int l = prima_xft_get_text_width( XX-> font, text, len, flags | toAddOverhangs, X(self)-> fc_map8, &ovx);
		overstrike(self, x, y, &ovx, l - ovx.x - ovx.y - 1);
	}
	XFLUSH;

	return true;
}

Bool
prima_xft_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y)
{
	DEFXX;
	XftColor xftcolor;
	XftFont *font = XX-> font-> xft;
	int rop = XX-> rop;
	Point baseline;

	t-> flags |= toAddOverhangs; /* for overstriking etc */

	if ( t->len == 0) return true;
	t->len = check_width( XX->font, t->len );
	rop = filter_unsupported_rops( XX, rop, &xftcolor );
	setup_alpha( XX, &xftcolor, &font );

	/* paint background if opaque */
	if ( XX-> flags. opaque) {
		Point * p = prima_xft_get_glyphs_box( self, t);
		prima_paint_text_background( self, p, x, y );
		free( p);
	}

	SHIFT(x,y);
	RANGE2(x,y);
	/* xft doesn't allow shifting glyph reference point - need to adjust manually */
	baseline.x = 0;
	baseline.y = XX-> font-> font. descent;
	prima_matrix_apply_int_to_int( XX-> fc_font_matrix, &baseline.x, &baseline.y);
	if ( !XX-> flags. base_line) {
		x += baseline.x;
		y += baseline.y;
	}

	allocate_xftdraw_surface(XX);
	if ( rop != ropCopyPut) {
		/* emulate rops by blitting the text */
		int dx;
		TextBlit tb;
		dx = prima_xft_get_glyphs_width( self, XX-> font, t, NULL);
		if (!open_text_blit(self, x - baseline.x, y - baseline.y, dx, rop, &tb))
			goto COPY_PUT;
		xft_draw_glyphs(self, XX, &xftcolor,
			tb.dx + baseline.x, tb.height - tb.dy - baseline.y,
			NULL, 0, t);
		close_text_blit(XX, &tb);
	} else {
	COPY_PUT:
		xft_draw_glyphs(self, XX, &xftcolor, x, REVERT(y) + 1, NULL, 0, t);
	}
	XCHECKPOINT;

	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {
		Point ovx;
		t-> flags |= toAddOverhangs;
		overstrike(self, x, y, &ovx, prima_xft_get_glyphs_width(
			self, XX-> font, t, &ovx) - 1);
	}
	XFLUSH;

	return true;
}

unsigned long *
prima_xft_get_font_ranges( Handle self, int * count)
{
	return prima_fc_get_font_ranges(X(self)-> font-> xft-> charset, count);
}

char *
prima_xft_get_font_languages( Handle self)
{
	FcPattern *pat;
	if ( !(pat = X(self)-> font-> xft-> pattern))
		return NULL;
	return prima_fc_get_font_languages(pat);
}

PFontABC
prima_xft_get_font_abc( Handle self, int firstChar, int lastChar, int flags)
{
	PFontABC abc;
	int i, len = lastChar - firstChar + 1;
	XftFont *font = X(self)-> font-> xft_base;

	if ( !( abc = malloc( sizeof( FontABC) * len)))
		return NULL;

	for ( i = 0; i < len; i++) {
		FT_UInt ft_index;
		XGlyphInfo glyph;
		if ( flags & toGlyphs ) {
			ft_index = i + firstChar;
		} else {
			FcChar32 c = i + firstChar;
			if ( !(flags & toUnicode) && c > 128)
				c = X(self)-> fc_map8[ c - 128];
			ft_index = XftCharIndex( DISP, font, c);
		}
		XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
		abc[i]. a = -glyph. x;
		abc[i]. b = glyph. width;
		abc[i]. c = glyph. xOff - glyph. width + glyph. x;
	}

	return abc;
}

PFontABC
prima_xft_get_font_def( Handle self, int firstChar, int lastChar, int flags)
{
	PFontABC abc;
	int i, len = lastChar - firstChar + 1;
	XftFont *font = X(self)-> font-> xft_base;

	if ( !( abc = malloc( sizeof( FontABC) * len)))
		return NULL;

	for ( i = 0; i < len; i++) {
		FT_UInt ft_index;
		XGlyphInfo glyph;
		if ( flags & toGlyphs ) {
			ft_index = i + firstChar;
		} else {
			FcChar32 c = i + firstChar;
			if ( !(flags & toUnicode) && c > 128)
				c = X(self)-> fc_map8[ c - 128];
			ft_index = XftCharIndex( DISP, font, c);
		}
		XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
		abc[i]. a = X(self)-> font-> font. descent - glyph. height + glyph. y; /* XXX yOff ? */
		abc[i]. b = glyph. height;
		abc[i]. c = X(self)-> font-> font. ascent - glyph. y;
	}

	return abc;
}

Bool
prima_xft_parse( char * ppFontNameSize, Font * font)
{
	FcPattern * p = FcNameParse(( FcChar8*) ppFontNameSize);
	Font f, def = guts. default_font;
	const char *encoding;


	bzero( &f, sizeof( Font));
	f. undef. height = f. undef. width = f. undef. size = f. undef. vector = f. undef. pitch = 1;
	prima_fc_pattern2font( p, &f);
	f. undef. width = 1;

	if ((encoding = prima_fc_pattern2encoding(p)) != NULL)
		strcpy(f.encoding, encoding);

	if ( !prima_font_pick( &f, NULL, &def, FONTKEY_XFT)) return false;
	*font = def;
	XFTdebug( "parsed ok: %g.%s", def.size, def.name);
	return true;
}

void
prima_xft_update_region( Handle self)
{
	DEFXX;
	if ( XX-> xft_drawable) {
		XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
		XX-> flags. xft_clip = 1;
	}
}

void
prima_xft_gp_destroy( Handle self )
{
	DEFXX;
	if ( XX-> xft_drawable) {
		XftDrawDestroy( XX-> xft_drawable);
		XX-> xft_drawable = NULL;
	}
	if ( XX-> xft_shadow_drawable) {
		XftDrawDestroy( XX-> xft_shadow_drawable);
		XX-> xft_shadow_drawable = NULL;
	}
	if ( XX-> xft_shadow_pixmap) {
		XFreePixmap( DISP, XX-> xft_shadow_pixmap);
		XX-> xft_shadow_pixmap = 0;
	}
	if ( XX-> xft_shadow_gc) {
		XFreeGC( DISP, XX-> xft_shadow_gc);
		XX-> xft_shadow_gc = 0;
	}
}

int
prima_xft_get_glyph_outline( Handle self, unsigned int index, unsigned int flags, int ** buffer)
{
	DEFXX;
	int ret;
	FcChar32 c;
	FT_UInt ft_index;
	FT_Face ft_face;
	FT_Int32 ft_flags = FT_LOAD_NO_BITMAP |
		(( flags & ggoUseHints ) ? 0 : FT_LOAD_NO_HINTING);

	if ( !( ft_face = XftLockFace( XX->font->xft)))
		return -1;

	c = ((flags & (ggoUnicode|ggoGlyphIndex)) || index <= 128) ? index : XX-> fc_map8[index - 128];
	ft_index = (flags & ggoGlyphIndex) ? c : XftCharIndex( DISP, XX->font->xft, c);

	ret = prima_ft_get_glyph_outline( ft_face, ft_index, ft_flags, buffer);

	XftUnlockFace(XX->font->xft);

	return ret;
}

Byte*
prima_xft_get_glyph_bitmap( Handle self, uint16_t index, unsigned int flags, PPoint offset, PPoint size, int *advance)
{
	DEFXX;
	Byte *ret;
	FT_Face face;
	FT_Int32 ft_flags =
		FT_LOAD_RENDER |
		(( flags & ggoUseHints )   ? 0 : FT_LOAD_NO_HINTING) |
		(( flags & ggoMonochrome ) ? FT_LOAD_MONOCHROME : 0)
		;
	if ( !( face = XftLockFace( XX->font->xft)))
		return NULL;
	ret = prima_ft_get_glyph_bitmap(face, index, ft_flags, offset, size, advance);
	XftUnlockFace(XX->font->xft);
	return ret;
}

unsigned long *
prima_xft_mapper_query_ranges(PFont font, int * count, unsigned int * flags)
{
	PCachedFont kf;
	unsigned long * ranges;

	if ( !( kf = prima_font_pick( font, NULL, NULL, FONTKEY_XFT))) {
		*count = 0;
		return NULL;
	}

	*flags = 0 | MAPPER_FLAGS_SYNTHETIC_PITCH;
	ranges = prima_fc_get_font_ranges( kf->xft->charset, count);

#ifdef WITH_HARFBUZZ
{
	FT_Face face;
	if ( !( face = XftLockFace( kf->xft)))
		return ranges;
	if ( prima_ft_combining_supported(face))
		*flags |= MAPPER_FLAGS_COMBINING_SUPPORTED;
	XftUnlockFace(kf->xft);
}
#endif

	return ranges;
}

Bool
prima_xft_text_shaper( Handle self, PTextShapeRec r, uint32_t * map8)
{
        int i;
	uint16_t *glyphs;
	uint32_t *text;
	XftFont *font = X(self)->font->xft_base;

	if ( !X(self)-> font-> xft)
		return prima_text_shaper_core_text(self, r);

        for ( 
		i = 0, glyphs = r->glyphs, text = r->text;
		i < r->len; 
		i++
	) {
		uint32_t c = *(text++);
		if ( map8 && c > 128 ) c = map8[c];
               	*(glyphs++) = XftCharIndex(DISP, font, c);
	}
	r-> n_glyphs = r->len;

	if ( r-> advances ) {
		uint16_t *advances;
		for (
			i = 0, glyphs = r->glyphs, advances = r->advances;
			i < r-> len;
			i++, glyphs++, advances++
		) {
			XGlyphInfo glyph;
			FT_UInt ft_index = *glyphs;
			XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
			*advances = glyph.xOff;
		}
		bzero(r->positions, r->len * 2 * sizeof(int16_t));
	}

        return true;
}

Bool
prima_xft_text_shaper_bytes( Handle self, PTextShapeRec r)
{
	return prima_xft_text_shaper( self, r, X(self)-> fc_map8);
}

Bool
prima_xft_text_shaper_ident( Handle self, PTextShapeRec r)
{
	return prima_xft_text_shaper( self, r, NULL);
}

Bool
prima_xft_text_shaper_harfbuzz( Handle self, PTextShapeRec r)
{
	DEFXX;
	Bool ok;
	FT_Face face;

	if ( !X(self)-> font-> xft)
		return prima_text_shaper_core_text(self, r);

	if ( !( face = XftLockFace( XX->font->xft)))
		return -1;
	ok = prima_ft_text_shaper_harfbuzz(face, r);
	XftUnlockFace(XX->font->xft);

	return ok;
}

static Bool
kill_lists( void * f, int keyLen, void * key, void * dummy)
{
	plist_destroy((PList) f);
	return false;
}

void
prima_xft_init_font_substitution(void)
{
	int i;
	FcFontSet * s;
	FcPattern   *pat, **ppat;
	FcObjectSet *os;
	PHash core_fonts = hash_create();
	PFontInfo info;

#define REG(x) if ( guts.x##_font.name[0]) \
	prima_font_mapper_save_font(guts.x##_font.name, guts.x##_font.style)
	REG(default);
	REG(default_widget);
	REG(default_msg);
	REG(default_caption);
	REG(default_menu);
#undef REG

	for ( i = 0, info = guts. font_info; i < guts. n_fonts; i++, info++) {
		PList list;
		int len = strlen(info->font.name);
		list = (PList) hash_fetch( core_fonts, info-> font.name, len);
		if ( !list ) {
			list = plist_create(32, 32);
			hash_store( core_fonts, info-> font.name, len, (void*) list);
		}
		list_add( list, (Handle) i);
	}


	pat = FcPatternCreate();
	FcPatternAddBool( pat, FC_SCALABLE, 1);
	os = FcObjectSetBuild( FC_FAMILY, (void*) 0);
	s = FcFontList( 0, pat, os);
	FcObjectSetDestroy( os);
	FcPatternDestroy( pat);

	if ( !s) return;

	ppat = s-> fonts;
	for ( i = 0; i < s->nfont; i++, ppat++) {
		int j;
		FcChar8 * s;
		PList list;
		char lower[512], *llower = lower, *lupper;

		if ( FcPatternGetString(*ppat, FC_FAMILY, 0, &s) != FcResultMatch)
			continue;

		/* disable the corresponding core font */
		lupper = (char*) s;
		while ( *lupper && (lupper - (char*)s) < 512 )
			*(llower++) = tolower((int)*(lupper++));
		*llower = 0;
		if (( list = (PList) hash_fetch(core_fonts, lower, strlen(lower))) != NULL) {
			for (j = 0; j < list->count; j++) {
				PFontInfo info = guts.font_info + (int) list->items[j];
				info->flags.disabled = 1;
			}
		}
	}

	FcFontSetDestroy(s);

	hash_first_that( core_fonts, (void*)kill_lists, NULL, NULL, NULL);
	hash_destroy( core_fonts, false);
}

#else
#warning Required Xft version 2.1.0 or higher; fontconfig version 2.0.1 or higher. To compile without Xft, re-run 'perl Makefile.PL WITH_XFT=0'
#endif /* USE_XFT */
