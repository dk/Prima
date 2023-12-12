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
	if ( opt_InPaint )
		return false;
	if ( is_opt(optInFontQuery) )
		return true;
	if ( !apc_font_begin_query( self ))
		return false;
	opt_set(optInFontQuery);
	apc_font_pick( self, &var->font, NULL );
	apc_gp_set_font( self, &var->font );
	return true;
}

void
Image_end_font_query( Handle self )
{
	apc_font_end_query( self );
	opt_clear(optInFontQuery);
}

Font *
Image_font_match( SV * dummy, Font * source, Font * dest, Bool pick)
{
	if ( pick) {
		Handle self;
		if ( dummy && SvOK(dummy) && (self = gimme_the_mate(dummy)) && kind_of(self, CImage)) {
			my-> begin_font_query(self);
			apc_font_pick( NULL_HANDLE, source, dest);
		}
	} else
		Drawable_font_add( NULL_HANDLE, source, dest);

	return dest;
}

Font
Image_get_font( Handle self)
{
	if ( !is_opt(optInFontQuery))
		my-> begin_font_query(self);

	return var-> font;
}

void
Image_set_font( Handle self, Font font)
{
	if (
		!is_opt(optInFontQuery) &&        /* special case during inherited init             */
		var->transient_class == CComponent /* to not enter for every image creation          */
	) {                                       /* until at least some font operations are needed */
		Drawable_font_add( self, &font, &var->font);
		return;
	}

	if ( !is_opt(optInFontQuery))
		my-> begin_font_query(self);

	inherited set_font(self, font);
}

static PTextShapeRec
prepare_simple_shaping_input( char * text, unsigned int bytelen, unsigned int len, Bool utf8)
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
			unsigned int charlen;
			UV uv = prima_utf8_uvchr(text, bytelen, &charlen);
			if ( uv > 0x10FFFF ) uv = 0x10FFFF;
			*(dst++) = uv;
			if ( charlen == 0 ) break;
			text += charlen;
			bytelen -= charlen;
			len--;
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
	int             dy = 0;
	int             advance      = 0;
	Bool            straight     = (var->font.direction == 0.0) && prima_matrix_is_translated_only(VAR_MATRIX);
	Matrix          matrix;
	ImgPaintContext ctx;
	Color           color;
	Bool            gp_save = false;
	SaveFont        savefont;

	flags = ggoUseHints;
	if (((var->type & imBPP) == 1) || !var->antialias)
		flags |= ggoMonochrome;

	if ( !(flags & ggoMonochrome) && var->type != imByte && var->type != imRGB) {
		ImagePreserveTypeRec p;
		my-> begin_preserve_type( self, &p);
		my-> reset( self, (var->type & imGrayScale) ? imByte : imRGB, NULL, 0);
		if ( is_opt( optPreserveType)) {
			Bool ok = plot_glyphs( self, t, x, y );
			my-> end_preserve_type( self, &p);
			return ok;
		}
	}

	bzero(&ctx, sizeof(ImgPaintContext));
	ctx.region = var-> regionData;

	ctx.rop = var-> extraROP;
	color = my->get_color(self);
	if ( flags & ggoMonochrome) {
		if ( ctx.rop > ropWhiteness )
			ctx.rop = ropCopyPut;
	} else if ( ctx.rop >= ropMinPDFunc && ctx.rop <= ropMaxPDFunc ) {
		ctx.rop &= ~(0xff << ropSrcAlphaShift);
		ctx.rop |= ropSrcAlpha | ( var-> alpha << ropSrcAlphaShift );
		/* make sure blending is used */
		int a = (ctx.rop & ropSrcAlpha) ? (ctx.rop >> ropSrcAlphaShift ) & 0xff : 0xff;
		a = var-> alpha * a / 255;
		ctx.rop &= ~(0xff << ropSrcAlphaShift);
		ctx.rop |= ropSrcAlpha | ( a << ropSrcAlphaShift );
	} else if ( ctx.rop <= ropWhiteness ) {
		switch (ctx.rop) {
		case ropCopyPut:
			break;
		case ropNoOper:
			return true;
		case ropBlackness:
			color = 0;
			break;
		case ropWhiteness:
			color = 0xffffff;
			break;
		default:
			return false;
		}
		ctx.rop = ropBlend | ropSrcAlpha | (var-> alpha << ropSrcAlphaShift);
	}

	if ( !apc_gp_get_text_out_baseline( self)) {
		dy = var-> font. descent;
		y += dy;
	}

	if ( !straight ) {
		if ( var->font.direction != 0.0 ) {
			Matrix m2;
			NPoint c = my->trig_cache(self);
			m2[0] =  c.y;
			m2[1] =  c.x;
			m2[2] = -c.x;
			m2[3] =  c.y;
			prima_matrix_multiply( VAR_MATRIX, m2, matrix );
		} else
			COPY_MATRIX( VAR_MATRIX, matrix);
	} else
		prima_matrix_set_identity(matrix);
	o.x = matrix[4] += x;
	o.y = matrix[5] += y;
	x = y = 0;

	if ( my->get_textOpaque(self)) {
		Point p[5];
		Color bc;
		double d = var->font.direction;
		var->font.direction = 0;
		Drawable_get_glyphs_box(self, t, p);
		var->font.direction = d;

		bc = my->get_backColor(self);
		if ( !( flags & ggoMonochrome))
			bc = Image_premultiply_color(self, ctx.rop, bc);
		if ( p[0].x == p[1].x && p[0].y == p[2].y && straight) {
			Image_color2pixel( self, bc, ctx.color);
			memset(&ctx.pattern, 0xff, sizeof(FillPattern));
			img_bar( self, o.x + p[1].x, o.y + p[1].y - dy, p[2].x - p[1].x + 1, p[2].y - p[1].y + 1, &ctx);
		} else {
			SV * sv1  = prima_array_new(sizeof(Point) * 4);
			SV * sv2  = sv_2mortal( prima_array_tie(sv1, sizeof(p[0].x), "i"));
			Point *p2 = (Point*) SvPV(sv1, PL_na);
			p2[0].x = p[0].x;
			p2[0].y = p[0].y - dy;
			p2[1].x = p[1].x;
			p2[1].y = p[1].y - dy;
			p2[2].x = p[3].x;
			p2[2].y = p[3].y - dy;
			p2[3].x = p[2].x;
			p2[3].y = p[2].y - dy;
			prima_matrix_apply2_int_to_int( matrix, p2, p2, 4);
			if ( !gp_save) {
				gp_save = my-> graphic_context_push(self);
				prima_matrix_set_identity(VAR_MATRIX);
				apc_gp_set_text_matrix( self, VAR_MATRIX);
			}
			apc_gp_set_fill_pattern( self, fillPatterns[fpSolid]);
			my-> set_rop(self, ropCopyPut);
			my-> set_color(self, bc);
			if ( flags & ggoMonochrome)
				my-> set_antialias( self, false );
			Image_draw_primitive( self, 1, "sS", "line", sv2 );
		}
	}

	if ( !( flags & ggoMonochrome))
		color = Image_premultiply_color(self, ctx.rop, color);
	Image_color2pixel( self, color, ctx.color);

	Drawable_save_font( self, &savefont );
	for ( i = i2 = 0; i < t->len; i++, i2 += 2) {
		Byte *arena;
		Point offset, size;
		Rect glyph;
		int default_advance = 0;

		if ( t->fonts )
			if ( !Drawable_switch_font(self, &savefont, t->fonts[i]))
				continue;

		if ( !( arena = apc_font_get_glyph_bitmap(
			self, t->glyphs[i], flags,
			&offset, &size,
			t->advances ? NULL : &default_advance
		)))
			return false;

		glyph.left   = o.x + offset.x;
		glyph.bottom = o.y + offset.y;
		if ( t->positions ) {
			/* XXX -- it this correct? */
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
		} else {
			o.x += matrix[4];
			o.y = y + matrix[5];
		}
	}
	Drawable_restore_font( self, &savefont );

	if ( var-> font.style & (fsUnderlined|fsStruckOut) ) {
		Bool use_1px =
			var-> font.underlineThickness <= 1 && (
				(flags & ggoMonochrome) ||
				straight ||
				!my->get_antialias(self)
			)
		;

		if ( use_1px ) {
			ctx.linePattern = lpSolid;
		} else {
			if ( !gp_save ) {
				gp_save = my-> graphic_context_push(self);
				prima_matrix_set_identity(VAR_MATRIX);
				apc_gp_set_text_matrix( self, VAR_MATRIX);
			}
			my-> set_color(self, color);
			my-> set_lineWidth( self, var->font.underlineThickness );
			my-> set_lineEnd( self, sv_2mortal(newSViv(leRound)) );
			apc_gp_set_line_pattern(self, lpSolid, strlen((char*) lpSolid));
			if ( flags & ggoMonochrome)
				my-> set_antialias( self, false );
		}

		if ( var-> font.style & fsStruckOut ) {
			Point poly[2];
			poly[0].x = x;
			poly[1].x = x + advance;
			poly[0].y = poly[1].y = y + (var-> font.ascent - var-> font.internalLeading) / 3;
			prima_matrix_apply2_int_to_int( matrix, poly, poly, 2);
			if ( use_1px)
				img_polyline(self, 2, poly, &ctx);
			else
				Image_draw_primitive( self, 0, "siiii", "line", poly[0].x, poly[0].y, poly[1].x, poly[1].y);
		}

		if ( var-> font.style & fsUnderlined ) {
			Point poly[2];
			poly[0].x = x;
			poly[1].x = x + advance;
			poly[0].y = poly[1].y = y + var-> font.underlinePosition;
			prima_matrix_apply2_int_to_int( matrix, poly, poly, 2);
			if ( use_1px) {
				img_polyline(self, 2, poly, &ctx);
			} else {
				Image_draw_primitive( self, 0, "siiii", "line", poly[0].x, poly[0].y, poly[1].x, poly[1].y);
			}
		}

	}

	if ( gp_save )
		my-> graphic_context_pop(self);

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
		STRLEN dlen, bytelen;
		PTextShapeFunc shaper;
		char * c_text = SvPV( text, bytelen);
		Bool   utf8   = prima_is_utf8_sv( text);
		int    type;

		dlen = utf8 ? prima_utf8_length(c_text, bytelen) : bytelen;
		if ((len = Drawable_check_length(from,len,dlen)) == 0)
			return true;
		c_text = Drawable_hop_text(c_text, utf8, from);
		prima_matrix_apply_int_to_int(VAR_MATRIX, &x, &y);

		type = utf8 ? tsGlyphs : tsBytes;
		if ( !( shaper = apc_font_get_text_shaper( self, &type)))
			return false;
		if ( !( s = prepare_simple_shaping_input(c_text, bytelen, len, utf8)))
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

SV*
Image_text_shape( Handle self, SV * text_sv, HV * profile)
{
	if (!opt_InPaint && !my-> begin_font_query(self) )
		return NULL_SV;

	return inherited text_shape(self, text_sv, profile);
}

SV*
Image_fonts( Handle self, char * name, char * encoding)
{
	if (!opt_InPaint && !my-> begin_font_query(self) )
		return NULL_SV;
	return Application_fonts( self, name, encoding);
}

SV*
Image_font_encodings( Handle self)
{
	if (!opt_InPaint && !my-> begin_font_query(self) )
		return NULL_SV;
	return Application_font_encodings( self);
}

#ifdef __cplusplus
}
#endif
