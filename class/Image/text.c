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
		apc_gp_set_font( self, &var-> font);
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
plot_glyphs( Handle self, PGlyphsOutRec t, int x, int y )
{
	unsigned int    i, i2, flags;
	Point           o;
	int             advance      = 0;
	Bool            restore_font = false;
	uint32_t        last_font    = 0;
	Bool            straight     = (var->font.direction == 0.0) && prima_matrix_is_translated_only(VAR_MATRIX);
	Matrix          matrix;
	ImgPaintContext ctx;

	flags = ggoUseHints;
	if (((var->type & imBPP) == 1) || !var->antialias)
		flags |= ggoMonochrome;

	if ( !apc_gp_get_text_out_baseline( self))
		y += var-> font. descent;
	o.x = x;
	o.y = y;

	bzero(&ctx, sizeof(ImgPaintContext));
	Image_color2pixel( self, my->get_color(self), ctx.color);
	ctx.rop = my->effective_rop(self, var->extraROP);
	ctx.region = var-> regionData;

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
		int default_advance = 0;

		if ( t->fonts && t->fonts[i] != last_font ) {
			if (t->fonts[i] != 0) {
				Font src = PASSIVE_FONT(t->fonts[i])->font, dst;
				dst = var->font;
				src.size = dst.size;
				src.undef.size = 0;
				apc_font_pick( self, &src, &dst);
				if ( strcmp(dst.name, src.name) != 0 )
					continue;
				apc_gp_set_font( self, &dst);
				last_font = t->fonts[i];
				restore_font = true;
			} else {
				apc_gp_set_font( self, &var->font);
				last_font = 0;
				restore_font = true;
			}
		}

		if ( !( arena = apc_font_get_glyph_bitmap(
			self, t->glyphs[i], flags,
			&offset, &size,
			t->advances ? NULL : &default_advance
		)))
			return false;

		glyph.left   = o.x + offset.x;
		glyph.bottom = o.y + offset.y;
		if ( t->positions ) {
			glyph.left   += t->positions[i2 + 0];
			glyph.bottom += t->positions[i2 + 1];
		}
		glyph.right  = glyph.left   + size.x - 1;
		glyph.top    = glyph.bottom + size.y - 1;
		if (
			glyph.left   <  var->w &&
			glyph.right  >= 0      &&
			glyph.bottom <  var->h &&
			glyph.top    >= 0      &&
			size.x       >  0      &&
			size.y       >  0
		) {
			Image src_dummy;
			img_fill_dummy( &src_dummy, size.x, size.y,
				imGrayScale | ((flags & ggoMonochrome) ? 1 : 8),
				arena, NULL);
			img_plot_glyph( self, &src_dummy, glyph.left, glyph.bottom, &ctx);
		}
		free(arena);

		advance += t->advances ? t->advances[i]: default_advance;
		o.x = x + advance;
		if ( !straight ) {
			o.y = y;
			prima_matrix_apply_int_to_int( matrix, &o.x, &o.y);
		}
	}

	if ( restore_font )
		apc_gp_set_font( self, &var-> font);

	return true;
}

Bool
Image_text_out( Handle self, SV * text, int x, int y, int from, int len)
{
	Bool ok;
	if (opt_InPaint)
		return inherited text_out(self, text, x, y, from, len);

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

#ifdef __cplusplus
}
#endif
