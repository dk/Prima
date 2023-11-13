#include "apricot.h"
#include "guts.h"
#include "Image.h"
#include "Drawable_private.h"
#include "Image_private.h"
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

Bool
Image_begin_font_query( Handle self )
{
	Bool ok;
	if ( opt_InPaint )
	if ( is_opt(optInFontQuery) )
		return true;
	ok = apc_font_begin_query( self );
	if ( ok ) {
		opt_set(optInFontQuery);
		apc_font_set_font( self, &var-> font);
	}
	return ok;
}

void
Image_end_font_query( Handle self )
{
	apc_font_end_query( self );
	opt_clear(optInFontQuery);
}

static PTextShapeRec
prepare_simple_shaping_input( char * text, unsigned int len, Bool utf8)
{
	PTextShapeRec s;
	Byte *buf;
	int size;

	size = sizeof( TextShapeRec );
	while ( (size % 4) > 0 ) size++; /* align to 4 bytes */

	if ( !( buf = malloc(size + 14 * len))) /* 2 bytes per * (glyphs + indexes + advances + 2 * positions) + 4 bytes * len */
		return NULL;

	s = (PTextShapeRec) buf;
	bzero(s, size + 14 * len);
	s->len       = len;
	s->n_glyphs  = s->n_glyphs_max = len;
	s->glyphs    = (uint16_t*)(buf + size);
	s->indexes   = s-> glyphs + len;
	s->advances  = s-> glyphs + len * 2;
	s->positions = (int16_t*) (s-> glyphs + len * 3);
	s->text      = (uint32_t*) (s-> glyphs + len * 5);
	if ( utf8 ) {
		uint32_t *dst = s->text;
		while ( len > 0 ) {
			STRLEN charlen;
			UV uv = prima_utf8_uvchr(text, len, &charlen);
			if ( uv > 0x10FFFF ) uv = 0x10FFFF;
			*(dst++) = uv;
			if ( charlen == 0 ) break;
			text  += charlen;
			len   -= charlen;
		}
	} else {
		uint32_t *dst = s->text;
		while (len-- > 0)
			*(dst++) = *((unsigned char*) text++);
	}

	return s;
}

static Bool
plot_glyph( Handle self, Byte *arena, Point size, int x, int y, PImgPaintContext ctx)
{
	Image src_dummy;

	img_fill_dummy( &src_dummy, size.x, size.y,
		imGrayScale | (((var->type & imBPP) == 1) ? 1 : 8),
		arena, NULL
	);

	if (
		( var->type & imBPP) != 1 &&
		( var->type & imBPP) != 4 &&
		( var->type & imBPP) != 8 && (
			( var->type & imGrayScale) != 0 ||
			var->type == imRGB
		)
	) {
		/* convert from 8 bit to other non-palette-matching depths */
		Byte *arena2;
		RGBColor dummy_palette;
		int dummy_palsize = 0;

		if ( !( arena2 = malloc(LINE_SIZE(size.x, var->type) * size.y))) {
			warn("Not enough memory");
			return false;
		}
		ic_type_convert((Handle)&src_dummy, arena2, &dummy_palette, var->type, &dummy_palsize, false);
		img_fill_dummy( &src_dummy, size.x, size.y, var->type, arena2, NULL);
	} else if ( var->type == 8 && ctx-> rop == ropCopyPut ) {
		/* convert from 8 bit to 8 paletted bits - reuse memory */
		/* XXX */
	} else if ( (var->type &imBPP) == 4 ) {
		/* convert from 8 bit to 4 bits - reuse memory */
		RGBColor dummy_palette;
		int dummy_palsize = 0;
		ic_type_convert((Handle)&src_dummy, arena, &dummy_palette, var->type, &dummy_palsize, false);
		img_fill_dummy( &src_dummy, size.x, size.y, var->type, arena, NULL);
		if ( var->type == 4 && ctx-> rop == ropCopyPut ) {
			/* XXX */
		}
	}

	img_plot_glyph( self, &src_dummy, x, y, ctx);

	return true;
}

static Byte
rop_direct( int rop )
{
	switch (rop) {
	case ropBlackness  : return ropNotSrcAnd ;
	case ropNotOr      : return ropXorPut    ;
	case ropNotSrcAnd  : return ropNoOper    ;
	case ropNotPut     : return ropOrPut     ;
	case ropNotDestAnd : return ropNotSrcAnd ;
	case ropInvert     : return ropXorPut    ;
	case ropXorPut     : return ropNoOper    ;
	case ropNotAnd     : return ropOrPut     ;
	case ropAndPut     : return ropNotSrcAnd ;
	case ropNotXor     : return ropXorPut    ;
	case ropNoOper     : return ropNoOper    ;
	case ropNotSrcOr   : return ropOrPut     ;
	case ropCopyPut    : return ropNotSrcAnd ;
	case ropNotDestOr  : return ropXorPut    ;
	case ropOrPut      : return ropNoOper    ;
	case ropWhiteness  : return ropOrPut     ;
	}
	return rop;
}

static Byte
rop_inverse( int rop )
{
	switch (rop) {
	case ropBlackness  : return ropOrPut     ;
	case ropNotOr      : return ropNoOper    ;
	case ropNotSrcAnd  : return ropXorPut    ;
	case ropNotPut     : return ropNotSrcAnd ;
	case ropNotDestAnd : return ropOrPut     ;
	case ropInvert     : return ropNoOper    ;
	case ropXorPut     : return ropXorPut    ;
	case ropNotAnd     : return ropNotSrcAnd ;
	case ropAndPut     : return ropOrPut     ;
	case ropNotXor     : return ropNoOper    ;
	case ropNoOper     : return ropXorPut    ;
	case ropNotSrcOr   : return ropNotSrcAnd ;
	case ropCopyPut    : return ropOrPut     ;
	case ropNotDestOr  : return ropNoOper    ;
	case ropOrPut      : return ropXorPut    ;
	case ropWhiteness  : return ropNotSrcAnd ;
	}
	return rop;
}

static Bool
plot_glyphs( Handle self, PGlyphsOutRec t, int x, int y )
{
	unsigned int    i, i2, flags;
	Point           o            = {x, y};
	int             advance      = 0;
	Bool            restore_font = false;
	uint32_t        last_font    = 0;
	Bool            straight     = (var->font.direction == 0.0) && prima_matrix_is_translated_only(VAR_MATRIX);
	Matrix          matrix;
	ImgPaintContext ctx;
	Bool            ok           = true;

	flags = ggoUseHints |
		(((var->type & imBPP) == 1) ? ggoMonochrome : 0)
		;

	bzero(&ctx, sizeof(ImgPaintContext));
	Image_color2pixel( self, my->get_color(self),     ctx.color);
	Image_color2pixel( self, my->get_backColor(self), ctx.backColor);
	ctx.rop = var-> extraROP;
	if ( var->alpha < 255 ) {
		ctx.rop &= ~(0xff << ropSrcAlphaShift);
		ctx.rop |= ropSrcAlpha | ( var->alpha << ropSrcAlphaShift );
	}
	ctx.region = var-> regionData;
	if ((var->type & imBPP) == 1) {
		if (ctx.rop >= ropSrcOver )
			ctx.rop = ropCopyPut;
		ctx.rop = ctx.color[0] ? rop_inverse(ctx.rop) : rop_direct(ctx.rop);
	}

	if ( !straight ) {
		if ( var->font.direction != 0.0 ) {
			Matrix m2;
			double s = sin( var-> font.direction / 57.29577951);
			double c = cos( var-> font.direction / 57.29577951);
			m2[0] = c;
			m2[1] = s;
			m2[2] = -s;
			m2[3] = c;
			prima_matrix_multiply( VAR_MATRIX, m2, matrix );
			matrix[4] = matrix[5] = 0.0;
		} else
			COPY_MATRIX_WITHOUT_TRANSLATION( VAR_MATRIX, matrix);
		o.x += VAR_MATRIX[4];
		o.y += VAR_MATRIX[5];
	}

	for ( i = i2 = 0; i < t->len; i++, i2 += 2) {
		Byte *arena;
		Point offset, size;
		Rect glyph;
		int default_advance;

		if ( t->fonts && t->fonts[i] != last_font ) {
			if (t->fonts[i] != 0) {
				Font src = PASSIVE_FONT(t->fonts[i])->font, dst;
				dst = var->font;
				src.size = dst.size;
				src.undef.size = 0;
				apc_font_pick( self, &src, &dst);
				if ( strcmp(dst.name, src.name) != 0 )
					continue;
				apc_font_set_font( self, &dst);
				last_font = t->fonts[i];
				restore_font = true;
			} else {
				apc_font_set_font( self, &var->font);
				last_font = 0;
				restore_font = true;
			}
		}

		if ( !( arena = apc_font_get_glyph_bitmap( self, t->glyphs[i], flags, &offset, &size, &default_advance)))
			return false;

		glyph.left   = o.x - offset.x;
		glyph.bottom = o.y - offset.y;
		if ( t->positions ) {
			glyph.left   += t->positions[i2 + 0];
			glyph.bottom += t->positions[i2 + 1];
		}
		glyph.right  = glyph.left   + size.x - 1;
		glyph.top    = glyph.bottom + size.y - 1;
		if (
			glyph.left < var->w &&
			glyph.right >= 0 &&
			glyph.bottom < var->h &&
			glyph.top    >= 0
		)
			ok = plot_glyph(self, arena, size, glyph.left, glyph.bottom, &ctx);
		free(arena);
		if ( !ok )
			break;

		advance += t->advances ? t->advances[i]: default_advance;
		o.x = x + advance;
		if ( !straight ) {
			o.y = y;
			prima_matrix_apply_int_to_int( matrix, &o.x, &o.y);
		}
	}

	if ( restore_font )
		apc_font_set_font( self, &var-> font);

	return ok;
}

Bool
Image_text_out( Handle self, SV * text, int x, int y, int from, int len)
{
	Bool ok;
	if ( !my-> begin_font_query(self) ) return false;

	if ( !SvROK( text )) {
		GlyphsOutRec t;
		TextShapeRec *s;
		STRLEN dlen;
		PTextShapeFunc shaper;
		char * c_text = SvPV( text, dlen);
		Bool   utf8   = prima_is_utf8_sv( text);
		int    type;

		if ( utf8) dlen = prima_utf8_length(c_text, dlen);
		if ((len = Drawable_check_length(from,len,dlen)) == 0)
			return true;
		c_text = Drawable_hop_text(c_text, utf8, from);
		prima_matrix_apply_int_to_int(VAR_MATRIX, &x, &y);

		type = utf8 ? tsGlyphs : tsBytes;
		if ( !( shaper = apc_font_get_text_shaper( self, &type)))
			return false;
		if ( !( s = prepare_simple_shaping_input(c_text, len, utf8)))
			return false;
		if ( !(ok = shaper(self, s))) {
			perl_error();
			goto FAIL;
		}

		bzero(&t, sizeof(t));
		t.len = t.text_len = len;
		t.glyphs    = s->glyphs;
		t.indexes   = s->indexes;
		t.advances  = s->advances;
		t.positions = s->positions;
		ok = plot_glyphs( self, &t, x, y );
	FAIL:
		free(s);
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!Drawable_read_glyphs(&t, text, 0, "Drawable::text_out"))
			return false;
		if (t.len == 0)
			return true;
		if (( len = Drawable_check_length(from,len,t.len)) == 0)
			return true;
		Drawable_hop_glyphs(&t, from, len);
		prima_matrix_apply_int_to_int(VAR_MATRIX, &x, &y);
		ok = plot_glyphs( self, &t, x, y );
	} else {
		SV * ret = sv_call_perl(text, "text_out", "<Hiiii", self, x, y, from, len);
		ok = ret && SvTRUE(ret);
	}
	return ok;
}

void
Image_set_font( Handle self, Font font)
{
	if ( opt_InPaint )
		return inherited set_font(self, font);
	Drawable_clear_font_abc_caches( self);
	apc_font_pick( self, &font, &var-> font);
	if ( is_opt(optInFontQuery))
		apc_font_set_font( self, &var-> font);
}

#ifdef __cplusplus
}
#endif
