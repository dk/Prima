/*
XXX todo:
export underscore position and size
check memory leaks

*/
#include "unix/guts.h"

#ifdef USE_FONTQUERY

#define FQdebug(...) if (pguts->debug & DEBUG_FONTS) prima_debug2("fq", __VA_ARGS__)

#define MY_MATRIX (PDrawable(self)->current_state.matrix)

void
prima_fq_build_key( PFontKey key, PFont f, Bool by_size)
{
	key-> style     = f-> style;
	key-> pitch     = f-> pitch;
	strcpy( key-> name, f-> name);
}

static Bool
fq_load(FcPattern *pattern, PCachedFont kf)
{
	int         id;
	FcChar8	    *filename;

	switch (FcPatternGetString(pattern, FC_FILE, 0, &filename)) {
	case FcResultNoMatch:
		if (FcPatternGetFTFace (pattern, FC_FT_FACE, 0, &kf->ft_face) != FcResultMatch || !&kf->ft_face)
			return false;
		break;
	case FcResultMatch:
		break;
	default:
		return false;
	}

	switch (FcPatternGetInteger (pattern, FC_INDEX, 0, &id)) {
	case FcResultNoMatch:
		id = 0;
		break;
	case FcResultMatch:
		break;
	default:
		return false;
	}

	if (!kf->ft_face && !( kf->ft_face = prima_ft_lock_face((char*) filename, id)))
		return false;

	/*
	 * Get the set of Unicode codepoints covered by the font.
	 * If the incoming pattern doesn't provide this data, go
	 * off and compute it.  Yes, this is expensive, but it's
	 * required to map Unicode to glyph indices.
	 */
	if (FcPatternGetCharSet (pattern, FC_CHARSET, 0, &kf->fc_charset) == FcResultMatch)
		kf->fc_charset = FcCharSetCopy(kf->fc_charset);
	else
		kf->fc_charset = FcFreeTypeCharSet(kf->ft_face, FcConfigGetBlanks (NULL));

	return true;
}

static void
build_mismatch_key( PFontKey key, PFont font)
{
	bzero(key, sizeof(FontKey));
	key->type = FONTKEY_FREETYPE;
	prima_xft_build_key(key, font, NULL, true);
	key->direction = 0;
	key->width = 0;
}

static void
fill_synthetic_fields( FT_Face f, PFont font, Bool by_size)
{
	float mul;

	if ( f-> units_per_EM == 0 ) {
		/* .Apple Color Emoji UI does this -- a SVG font? */
		if ( by_size )
			font->height = font->size;
		else
			font->size = font->height;

		font->ascent          = font->height;
		font->descent         = 0;
		font->internalLeading = 0;
		font->maximalWidth    = font->height;
		font->width           = font->height;
		font->externalLeading = 0;
		font->xDeviceRes      = font->yDeviceRes = 72;
		font->underlinePosition  = -1;
		font->underlineThickness = (font->height > 16) ? font->height / 16 : 1;
		return;
	}

	if (by_size) {
		mul = font->size / f-> units_per_EM;
		font->height = f-> height * mul + .5;
		FQdebug("set height: %d", font->height);
	} else {
		mul = (float) font->height / f-> height;
		font->size   = f-> units_per_EM * mul;
		FQdebug("set size: %g", font->size);
	}

	font->ascent          = f-> ascender * mul + .5;
	font->descent         = font->height - font->ascent;
	font->internalLeading = (f-> height - f-> units_per_EM) * mul + .5;
	font->maximalWidth    = f-> max_advance_width * mul + .5;
	font->width           = font->height; /* XXX bitmap fonts? */
	font->externalLeading = (f-> bbox.yMax - f-> ascender ) * mul + .5;
	font->xDeviceRes      = font->yDeviceRes = 72;
	/* XXX FT_Face reports very strange values */
	font->underlinePosition  = -font-> descent;
	font->underlineThickness = (font->height > 16) ? font->height / 16 : 1;
}

void
prima_fq_apply_synthetic_fields(PCachedFont kf, PFont source, PFont dest)
{
	int width = (source->undef.width || source->width == 0) ? 0 : source->width;
	dest->height = source->height;
	dest->size   = source->size;
	fill_synthetic_fields( kf->ft_face, dest, source->undef.height);
	if ( width > 0 ) dest->width = width;
}

PCachedFont
prima_fq_match( PFont font, Bool by_size, PCachedFont kf)
{
	FcPattern *request, *match;
	FcResult res = FcResultNoMatch;
	const char *encoding;
	FontKey key;
	double request_size = by_size ? font->size : font->height;
	int width;

	/* see if the font is not present - the hashed negative matches
			are stored with width=0, as the width alterations are derived */
	/* see if the font is not present in xft - the hashed negative matches are stored with width=height=0 */
	build_mismatch_key(&key, font);
	FQdebug("want %gx%d.%s.%s.%s/%s^%g.%d", by_size ? -font->size : font->height, key.width, _F_DEBUG_STYLE(key.style), _F_DEBUG_PITCH(key.pitch), key.name, font->encoding, ROUGHLY(font->direction), font->vector);
	if ( hash_fetch( guts.fc_mismatch, &key, sizeof( FontKey))) {
		FQdebug("refuse");
		return NULL;
	}

	encoding = prima_fc_find_encoding( font->encoding);

	/* create FcPattern request */
	if ( !( request = FcPatternCreate()))
		return NULL;
	if ( strcmp( font-> name, "Default") != 0)
		FcPatternAddString( request, FC_FAMILY, ( FcChar8*) font-> name);
	FcPatternAddInteger( request, FC_SPACING,
		(font-> pitch == fpFixed && guts.force_fc_monospace_emulation) ? FC_MONO : FC_PROPORTIONAL);
	FcPatternAddInteger( request, FC_SLANT, ( font-> style & fsItalic) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
	FcPatternAddInteger( request, FC_WEIGHT,
		( font-> style & fsBold) ? FC_WEIGHT_BOLD :
		( font-> style & fsThin) ? FC_WEIGHT_THIN : FC_WEIGHT_NORMAL);
	FcPatternAddBool( request, FC_SCALABLE, font-> vector > fvBitmap );

	/* match best font - must return something useful; the match is statically allocated */
	FcConfigSubstitute (NULL, request, FcMatchPattern);
	if ( !( match = FcFontMatch (NULL, request, &res))) {
		FQdebug("FcFontMatch error");
		FcPatternDestroy( request);
		return NULL;
	}
	FcPatternDestroy( request);

	/* convert Ubuntu[pitch=fpFixed] to Ubuntu Mono because the freetype-synthesized mono looks ugly */
	{
		int fcam;
		Font suggested_font;
		fcam = prima_fc_suggest_pitch(font, &suggested_font, match);
		if ( fcam > 0 ) {
			PCachedFont kff;
			if (( kff = prima_font_pick( &suggested_font, NULL, NULL, FONTKEY_FREETYPE)) != NULL ) {
				*font = suggested_font;
				prima_fc_end_suggestion(fcam);
				FcPatternDestroy(match);
				return kff;
			};
			prima_fc_end_suggestion(fcam);
		}
	}

	if ( !prima_fc_encoding_is_supported(font->encoding, match)) {
		FcPatternDestroy( match);
		return NULL;
	}

	/* Check if the matched font is scalable */
	if ( font-> vector > fvBitmap ) {
		FcBool scalable;
		if (( FcPatternGetBool( match, FC_SCALABLE, 0, &scalable) == FcResultMatch) && !scalable) {
			build_mismatch_key(&key, font);
			hash_store( guts.fc_mismatch, &key, sizeof( FontKey), (void*)1);
			FQdebug("refuse bitmapped font");
			FcPatternDestroy( match);
			return NULL;
		}
	}

	/* load the font */
	if ( !fq_load(match, kf)) {
		build_mismatch_key( &key, font);
		hash_store( guts.fc_mismatch, &key, sizeof( FontKey), (void*)1);
		FQdebug("error loading font");
		FcPatternDestroy( match);
		return NULL;
	}

	FQdebug("loaded ok");
	width = (font->undef.width || font->width == 0) ? 0 : font->width;
	prima_fc_pattern2font( match, font);
	if ( font->undef.size && font->undef.height) {
		/* .Apple Color Emoji UI does this -- a SVG font? */
		font->size = font-> height = request_size;
	}
	if ( width > 0 )
		font->width = width;
	fill_synthetic_fields( kf->ft_face, font, by_size);
	strcpy( font->encoding, encoding);
	return kf;
}

typedef struct {
	Font font;
	FT_Face orig, current;
	uint32_t fid;
	uint16_t *fonts;
} FontContext;

static void
font_context_init( FontContext * fc, Font * font, uint16_t * fonts, FT_Face orig)
{
	bzero(fc, sizeof(FontContext));
	fc->font      = *font;
	fc->orig      = fc->current = orig;
	fc->fid       = 0;
	fc->fonts     = fonts;
}

static void
font_context_next( FontContext * fc)
{
	Font *_src, src, dst;
	uint16_t nfid;
	PCachedFont kf;

	if ( !fc-> fonts ) return;

	nfid = *(fc->fonts++);
	if ( nfid == fc-> fid ) return;

	fc->fid = nfid;
	if ( nfid == 0 ) {
		fc->current = fc->orig;
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

	Drawable_font_add( NULL_HANDLE, &src, &dst);
	if ( !( kf = prima_font_pick( &src, NULL, &dst, FONTKEY_FREETYPE)))
		return;
	fc-> current = kf-> ft_face;
	FT_Set_Pixel_Sizes( fc->current, fc->font.size+.5, fc->font.size +.5 );
}

Bool
prima_fq_begin_query(Handle self)
{
	return true;
}

void
prima_fq_free_cached_font(PCachedFont f)
{
	prima_ft_unlock_face(f->ft_face);
	FcCharSetDestroy(f->fc_charset);
}

Bool
prima_fq_set_font( Handle self, PCachedFont kf)
{
	DEFXX;
	FT_Matrix ft_matrix;
	PFont font = &PDrawable(self)->font;

	prima_fc_set_font( self, font );

	FT_Set_Pixel_Sizes( kf->ft_face, font->size+.5, font->size+.5);

	ft_matrix.xx = XX->fc_font_matrix[0] * 0x10000L;
	ft_matrix.yx = XX->fc_font_matrix[1] * 0x10000L;
	ft_matrix.xy = XX->fc_font_matrix[2] * 0x10000L;
	ft_matrix.yy = XX->fc_font_matrix[3] * 0x10000L;
	FT_Set_Transform( kf->ft_face, &ft_matrix, NULL );

	return true;
}

#define UPDATE_OVERHANGS(_len,_flags)                                                    \
	if ( i == 0) {                                                                   \
		if (( _flags & toAddOverhangs ) && g-> bitmap_left < 0)                  \
			ret -= g->bitmap_left;                                           \
		if ( overhangs)                                                          \
			overhangs-> x = (g->bitmap_left < 0) ? -g->bitmap_left : 0;      \
	}                                                                                \
	if ( i == _len - 1) {                                                            \
		int c = FT266_to_short(g->advance.x - g->metrics.width) - g->bitmap_left;\
		if ( (_flags & toAddOverhangs) && c < 0) ret -= c;                       \
		if ( overhangs) overhangs-> y = (c < 0) ? -c : 0;                        \
	}

#define FT266_TRUNC(x)    ((x) >> 6)
#define FT266_ROUND(x)    (((x)+32) & -64)
#define FT266_to_short(x) ((short)(FT266_TRUNC(FT266_ROUND(x))))

unsigned long *
prima_fq_get_font_ranges( Handle self, int * count)
{
	return prima_fc_get_font_ranges(X(self)-> font-> fc_charset, count);
}

char *
prima_fq_get_font_languages( Handle self)
{
	DEFXX;
	FcPattern *pat, *match;
	char *ret = NULL;
	PFont f = &XX->font->font;
	FcResult res;

	if ( !( pat = FcPatternCreate())) return NULL;

	if ( strcmp( f->name, "Default") != 0)
		FcPatternAddString( pat, FC_FAMILY, ( FcChar8*) f-> name);
	FcPatternAddInteger( pat, FC_SPACING, (f->pitch == fpFixed) ? FC_MONO : FC_PROPORTIONAL);
	FcPatternAddInteger( pat, FC_SLANT,   (f->style & fsItalic) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
	FcPatternAddBool   ( pat, FC_SCALABLE, f->vector > fvBitmap );
	FcPatternAddInteger( pat, FC_WEIGHT,
		( f->style & fsBold) ? FC_WEIGHT_BOLD :
		( f->style & fsThin) ? FC_WEIGHT_THIN : FC_WEIGHT_NORMAL);

	if (( match = FcFontMatch (NULL, pat, &res)) != NULL) {
		ret = prima_fc_get_font_languages(match);
		FcPatternDestroy(match);
	}
	FcPatternDestroy(pat);

	return ret;
}

PFontABC
prima_fq_get_font_abc( Handle self, int firstChar, int lastChar, int flags)
{
	DEFXX;
	PFontABC abc;
	int i, len = lastChar - firstChar + 1;
	FT_Face ft_face = XX->font->ft_face;

	if ( !( abc = malloc( sizeof( FontABC) * len)))
		return NULL;

	for ( i = 0; i < len; i++) {
		FT_UInt ft_index;
		FT_GlyphSlot g;
		if ( flags & toGlyphs ) {
			ft_index = i + firstChar;
		} else {
			FcChar32 c = i + firstChar;
			if ( !(flags & toUnicode) && c > 128)
				c = X(self)-> fc_map8[ c - 128];
			ft_index = FcFreeTypeCharIndex(ft_face, c);
		}
		if ( FT_Load_Glyph( ft_face, ft_index, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_NO_BITMAP))
			continue;
		g = ft_face->glyph;
		abc[i]. a = g->bitmap_left;
		abc[i]. b = FT266_to_short(g->metrics.width);
		abc[i]. c = FT266_to_short(g->advance.x - g->metrics.width) - g->bitmap_left;
	}

	return abc;
}

PFontABC
prima_fq_get_font_def( Handle self, int firstChar, int lastChar, int flags)
{
	DEFXX;
	PFontABC abc;
	int i, len = lastChar - firstChar + 1;
	FT_Face ft_face = XX->font->ft_face;

	if ( !( abc = malloc( sizeof( FontABC) * len)))
		return NULL;

	for ( i = 0; i < len; i++) {
		FT_UInt ft_index;
		FT_GlyphSlot g;
		if ( flags & toGlyphs ) {
			ft_index = i + firstChar;
		} else {
			FcChar32 c = i + firstChar;
			if ( !(flags & toUnicode) && c > 128)
				c = X(self)-> fc_map8[ c - 128];
			ft_index = FcFreeTypeCharIndex(ft_face, c);
		}
		if ( FT_Load_Glyph( ft_face, ft_index, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_NO_BITMAP))
			continue;
		g = ft_face->glyph;
		abc[i].a = X(self)->font->font.descent - FT266_to_short(g->metrics.height) + g->bitmap_top;
		abc[i].b = FT266_to_short(g->metrics.height);
		abc[i].c = X(self)->font->font.ascent - g->bitmap_top;
	}

	return abc;
}

int
prima_fq_get_text_width( Handle self, const char * text, int len, int flags, Point * overhangs)
{
	DEFXX;
	FT_Face ft_face = XX->font->ft_face;
	int i, ret = 0, bytelen = 0;

	if ( overhangs) overhangs-> x = overhangs-> y = 0;
	if ( flags & toUTF8 ) bytelen = strlen(text);

	for ( i = 0; i < len; i++) {
		FcChar32 c;
		FT_GlyphSlot g;
		FT_UInt ft_index;
		if ( flags & toUTF8) {
			unsigned int charlen;
			c = ( FcChar32) prima_utf8_uvchr(text, bytelen, &charlen);
			text += charlen;
			bytelen -= charlen;
			if ( charlen == 0 ) break;
		} else if ( ((Byte*)text)[i] > 127) {
			c = XX->fc_map8[ ((Byte*)text)[i] - 128];
		} else
			c = text[i];
		ft_index = FcFreeTypeCharIndex(ft_face, c);
		if ( FT_Load_Glyph( ft_face, ft_index, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_NO_BITMAP))
			continue;
		g = ft_face->glyph;
		ret += FT266_to_short(g->advance.x);
		if ( (flags & toAddOverhangs ) || overhangs) {
			UPDATE_OVERHANGS(len,flags)
		}
	}
	return ret;
}

int
prima_fq_get_glyphs_width( Handle self, PGlyphsOutRec t, Point * overhangs)
{
	DEFXX;
	int i, ret = 0;
	FontContext fc;

	font_context_init(&fc, &XX->font->font, t->fonts, XX->font->ft_face);
	if ( overhangs) overhangs-> x = overhangs-> y = 0;

	for ( i = 0; i < t->len; i++) {
		FT_GlyphSlot g;
		FT_UInt ft_index = t->glyphs[i];
		font_context_next(&fc);
		if ( FT_Load_Glyph( fc.current, ft_index, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_NO_BITMAP))
			continue;
		g = fc.current->glyph;
		ret += FT266_to_short(g->advance.x);
		if ( (t->flags & toAddOverhangs ) || overhangs) {
			UPDATE_OVERHANGS(t->len,t->flags)
		}
	}
	return ret;
}

Point *
prima_fq_get_text_box( Handle self, const char * text, int len, int flags)
{
	Point ovx;
	return prima_get_text_box(self, ovx,
		prima_fq_get_text_width( self, text, len, flags, &ovx)
	);
}

Point *
prima_fq_get_glyphs_box( Handle self, PGlyphsOutRec t)
{
	Point ovx;
	return prima_get_text_box(self, ovx,
		prima_fq_get_glyphs_width(self, t, &ovx)
	);
}

int
prima_fq_get_glyph_outline( Handle self, unsigned int index, unsigned int flags, int ** buffer)
{
	DEFXX;
	FcChar32 c;
	FT_UInt ft_index;
	FT_Int32 ft_flags = FT_LOAD_NO_BITMAP |
		(( flags & ggoUseHints ) ? 0 : FT_LOAD_NO_HINTING);

	c = ((flags & (ggoUnicode|ggoGlyphIndex)) || index <= 128) ? index : XX-> fc_map8[index - 128];
	ft_index = (flags & ggoGlyphIndex) ? c : FcFreeTypeCharIndex(XX->font->ft_face, c);

	return prima_ft_get_glyph_outline( XX->font->ft_face, ft_index, ft_flags, buffer);
}

Byte*
prima_fq_get_glyph_bitmap( Handle self, uint16_t index, unsigned int flags, PPoint offset, PPoint size, int *advance)
{
	FT_Int32 ft_flags =
		FT_LOAD_RENDER |
		(( flags & ggoUseHints )   ? 0 : FT_LOAD_NO_HINTING) |
		(( flags & ggoMonochrome ) ? FT_LOAD_MONOCHROME : 0)
		;
	return prima_ft_get_glyph_bitmap(X(self)->font->ft_face, index, ft_flags, offset, size, advance);
}

unsigned long *
prima_fq_mapper_query_ranges(PFont font, int * count, unsigned int * flags)
{
	PCachedFont kf;
	unsigned long * ranges;
	Font f = *font;

	if ( f.undef.size && f.undef.height ) {
		f.undef.size = 0;
		f.size = 12.0;
	}
	if ( !( kf = prima_font_pick( &f, NULL, NULL, FONTKEY_FREETYPE))) {
		*count = 0;
		return NULL;
	}

	*flags = 0 | MAPPER_FLAGS_SYNTHETIC_PITCH;
	ranges = prima_fc_get_font_ranges( kf->fc_charset, count);
	if ( prima_ft_combining_supported(kf-> ft_face))
		*flags |= MAPPER_FLAGS_COMBINING_SUPPORTED;

	return ranges;
}

Bool
prima_fq_text_shaper( Handle self, PTextShapeRec r, uint32_t * map8)
{
	DEFXX;
        int i;
	uint16_t *glyphs;
	uint32_t *text;
	FT_Face ft_face = XX->font->ft_face;

        for (
		i = 0, glyphs = r->glyphs, text = r->text;
		i < r->len;
		i++
	) {
		uint32_t c = *(text++);
		if ( map8 && c > 128 ) c = map8[c];
               	*(glyphs++) = FcFreeTypeCharIndex(ft_face, c);
	}
	r-> n_glyphs = r->len;

	if ( r-> advances ) {
		uint16_t *advances;
		for (
			i = 0, glyphs = r->glyphs, advances = r->advances;
			i < r-> len;
			i++, glyphs++, advances++
		) {
			FT_UInt ft_index = *glyphs;
			if ( FT_Load_Glyph( ft_face, ft_index, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_NO_BITMAP))
				*advances = 0;
			else
				*advances = FT266_to_short(ft_face->glyph->advance.x);
		}
		bzero(r->positions, r->len * 2 * sizeof(int16_t));
	}

        return true;
}

Bool
prima_fq_text_shaper_bytes( Handle self, PTextShapeRec r)
{
	return prima_fq_text_shaper( self, r, X(self)-> fc_map8);
}

Bool
prima_fq_text_shaper_ident( Handle self, PTextShapeRec r)
{
	return prima_fq_text_shaper( self, r, NULL);
}

Bool
prima_fq_text_shaper_harfbuzz( Handle self, PTextShapeRec r)
{
	return prima_ft_text_shaper_harfbuzz(X(self)->font->ft_face, r);
}

#endif
