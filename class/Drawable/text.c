#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "private/Drawable.h"

#ifdef __cplusplus
extern "C" {
#endif

static void*
read_subarray( AV * av, int index,
	int length_expected, unsigned int * plen, char * letter_expected,
	const char * caller, const char * subarray
) {
	SV ** holder;
	void * ref;
	size_t length;
	char * letter;
	holder = av_fetch(av, index, 0);

	if (
		!holder ||
		!*holder ||
		!SvOK(*holder)
	) {
		warn("invalid subarray #%d (%s) passed to %s", index, subarray, caller);
		return NULL;
	}

	if ( !prima_array_parse( *holder, &ref, &length, &letter)) {
		warn("invalid subarray #%d (%s) passed to %s: %s", index, subarray, caller, "not a Prima::array");
		return NULL;
	}

	if (*letter != *letter_expected) {
		warn("invalid subarray #%d (%s/%c) passed to %s: %s", index, subarray, *letter, caller, "not a Prima::array of 16-bit integers");
		return NULL;
	}

	if ( length_expected >= 0 && length < length_expected ) {
		warn("invalid subarray #%d (%s) passed to %s: length must be at least %d", index, subarray, caller, length_expected);
		return NULL;
	}
	if ( plen ) *plen = length;
	return ref;
}

Bool
Drawable_read_glyphs( PGlyphsOutRec t, SV * text, Bool indexes_required, const char * caller)
{
	int len;
	AV* av;
	SV ** holder;

	if ( !SvROK(text) || SvTYPE(SvRV(text)) != SVt_PVAV)
		return false;

	bzero(t, sizeof(GlyphsOutRec));
	/* assuming caller checked for SvTYPE( SvRV( text)) == SVt_PVAV */

	av  = (AV*) SvRV(text);

	/* is this just a single glyphstr? */
	if ( SvTIED_mg(( SV*) av, PERL_MAGIC_tied )) {
		void * ref;
		size_t length;
		char * letter;

		if ( indexes_required ) {
			warn("%s requires glyphstr with indexes", caller);
			return false;
		}

		if ( !prima_array_parse( text, &ref, &length, &letter) || *letter != 'S') {
			warn("invalid glyphstr passed to %s: %s", caller, "not a Prima::array");
			return false;
		}

		t-> glyphs = ref;
		t-> len    = length;
		t-> text_len = 0;
		return true;
	}

	len = av_len(av) + 1;
	if ( len > 5 ) len = 5; /* we don't need more */
	if ( len < 1 || len != 5 ) {
		warn("malformed glyphs array in %s", caller);
		return false;
	}

	if ( !( t-> glyphs = read_subarray( av, 0, -1, &t->len, "S", caller, "glyphs")))
		return false;
	if ( t->len == 0 )
		return true;

	holder = av_fetch(av, 4, 0);
	if ( holder && *holder && !SvOK(*holder))
		goto NO_FONTS;
	if ( !( t-> fonts = read_subarray( av, 4, t->len, NULL, "S", caller, "fonts")))
		return false;

NO_FONTS:
	holder = av_fetch(av, 2, 0);
	if ( holder && *holder && !SvOK(*holder))
		goto NO_ADVANCES;
	if ( !( t-> advances = read_subarray( av, 2, t->len, NULL, "S", caller, "advances")))
		return false;
	if ( !( t-> positions = read_subarray( av, 3, t->len * 2, NULL, "s", caller, "positions")))
		return false;

NO_ADVANCES:
	if ( !( t-> indexes = read_subarray( av, 1, t->len + 1, NULL, "S", caller, "indexes")))
		return false;
	t-> text_len = t-> indexes[t->len];

	return true;
}

int
Drawable_check_length( int from, int len, int real_len )
{
	if ( len < 0 ) len = real_len;
	if ( from < 0 ) return 0;
	if ( from + len > real_len ) len = real_len - from;
	if ( len <= 0 ) return 0;
	return len;
}

char * 
Drawable_hop_text(char * start, Bool utf8, int from)
{
	if ( !utf8 ) return start + from;
	while ( from-- ) start = (char*)utf8_hop((U8*)start, 1);
	return start;
}

void 
Drawable_hop_glyphs(GlyphsOutRec * t, int from, int len)
{
	if ( from == 0 && len == t->len ) return;

	t->len      = len;
	t->glyphs  += from;

	if ( t-> indexes ) {
		int i, max_index = 0, next_index = t->indexes[t->len];
		t->indexes += from;
		for ( i = 0; i <= len; i++ ) {
			int ix = t->indexes[i] & ~toRTL;
			if ( max_index < ix ) max_index = ix;
		}
		for ( i = 0; i <= t->len; i++ ) {
			int ix = t->indexes[i] & ~toRTL;
			if (ix > max_index && ix < next_index) next_index = ix;
		}
		t->text_len = next_index;
	}

	if ( t->advances ) {
		t->advances  += from;
		t->positions += from * 2;
	}

	if ( t-> fonts )
		t-> fonts    += from;
}

Bool
Drawable_text_out( Handle self, SV * text, int x, int y, int from, int len)
{
	Bool ok;
	if ( !opt_InPaint) return false;

	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		gpCHECK(false);
		if ( utf8) dlen = prima_utf8_length(c_text, dlen);
		if ((len = Drawable_check_length(from,len,dlen)) == 0)
			return true;
		c_text = Drawable_hop_text(c_text, utf8, from);
		prima_matrix_apply_int_to_int(VAR_MATRIX, &x, &y);
		ok = apc_gp_text_out( self, c_text, x, y, len, utf8 ? toUTF8 : 0);
		if ( !ok) perl_error();
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		gpCHECK(false);
		if (!Drawable_read_glyphs(&t, text, 0, "Drawable::text_out"))
			return false;
		if (t.len == 0)
			return true;
		if (( len = Drawable_check_length(from,len,t.len)) == 0)
			return true;
		Drawable_hop_glyphs(&t, from, len);
		prima_matrix_apply_int_to_int(VAR_MATRIX, &x, &y);
		ok = apc_gp_glyphs_out( self, &t, x, y);
		if ( !ok) perl_error();
	} else {
		SV * ret = sv_call_perl(text, "text_out", "<Hiiii", self, x, y, from, len);
		ok = ret && SvTRUE(ret);
	}
	return ok;
}

int
Drawable_get_glyphs_width( Handle self, PGlyphsOutRec t, Bool add_overhangs)
{
	int i, ret;
	uint16_t * advances = t->advances;

	for ( i = ret = 0; i < t-> len; i++)
		ret += *(advances++);

	if ( add_overhangs ) {
		PFontABC abc;
		uint16_t glyph1 = t->glyphs[0], glyph2 = t->glyphs[t->len - 1];

		abc = Drawable_call_get_font_abc( self, glyph1, glyph1, toGlyphs);
		if ( !abc ) return ret;
		ret += ( abc->a < 0 ) ? (-abc->a + .5) : 0;

		if ( glyph1 != glyph2 ) {
			free(abc);
			abc = Drawable_call_get_font_abc( self, glyph2, glyph2, toGlyphs);
			if ( !abc ) return ret;
		}
		ret += ( abc->c < 0 ) ? (-abc->c + .5) : 0;
		free(abc);
	}

	return ret;
}


int
Drawable_get_text_width( Handle self, SV * text, int flags, int from, int len)
{
	dmARGS;
	int res;

	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		dmCHECK(0);
		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		} else
			flags &= ~toUTF8;
		if (( len = Drawable_check_length(from,len,dlen)) == 0)
			return 0;
		c_text = Drawable_hop_text(c_text, flags & toUTF8, from);
		dmENTER(0);
		res = apc_gp_get_text_width( self, c_text, len, flags);
		dmLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		dmCHECK(0);
		if (!Drawable_read_glyphs(&t, text, 0, "Drawable::get_text_width"))
			return false;
		if (t.len == 0)
			return true;
		if (( len = Drawable_check_length(from,len,t.len)) == 0)
			return 0;
		Drawable_hop_glyphs(&t, from, len);
		if (t.advances)
			return Drawable_get_glyphs_width(self, &t, flags & toAddOverhangs);
		dmENTER(0);
		res = apc_gp_get_glyphs_width( self, &t, flags);
		dmLEAVE;
	} else {
		SV * ret;
		dmENTER(0);
		ret = sv_call_perl(text, "get_text_width", "<Hiii", self, flags, from, len);
		dmLEAVE;
		res = (ret && SvOK(ret)) ? SvIV(ret) : 0;
	}

	return res;
}

NPoint
Drawable_trig_cache( Handle self )
{
	if ( !is_opt(optFontTrigCache)) {
		if ( var-> font. direction == 0.0 ) {
			var->trig_cache.x = 0.0;
			var->trig_cache.y = 1.0;
		} else {
#define GRAD 57.29577951
			var->trig_cache.x = sin( var-> font. direction / GRAD);
			var->trig_cache.y = cos( var-> font. direction / GRAD);
#undef GRAD
		}
		opt_set(optFontTrigCache);
	}
	return var->trig_cache;
}

void
Drawable_calculate_text_box( Handle self, int advance, Bool text_out_baseline, Point ovx, Point * pt)
{
	if ( ovx.x > 0 ) ovx.x = 0;
	if ( ovx.y > 0 ) ovx.y = 0;

	pt[0].y = pt[2]. y = var-> font. ascent - 1;
	pt[1].y = pt[3]. y = - var-> font. descent;
	pt[4].y = 0;
	pt[4].x = advance;
	pt[3].x = pt[2]. x = advance - ovx.y;
	pt[0]. x = pt[1].x = ovx.x;

	if ( !text_out_baseline ) {
		int i = 4, d = var->font. descent;
		while ( i--) pt[i]. y += d;
	}

	if ( var-> font. direction != 0) {
		int i;
		NPoint c = my->trig_cache(self);
		for ( i = 0; i < 5; i++) {
			float x = pt[i]. x * c.y - pt[i]. y * c.x;
			float y = pt[i]. x * c.x + pt[i]. y * c.y;
			pt[i].x = x + (( x > 0) ? 0.5 : -0.5);
			pt[i].y = y + (( y > 0) ? 0.5 : -0.5);
		}
	}
}

void
Drawable_get_glyphs_box( Handle self, PGlyphsOutRec t, Point * pt)
{
	int l;
	Bool text_out_baseline;
	Point ovx = {0,0};
	SaveFont savefont;
	FontABC *abc;

	text_out_baseline = ( my-> textOutBaseline == Drawable_textOutBaseline) ?
		apc_gp_get_text_out_baseline(self) :
		my-> get_textOutBaseline(self);
	l = Drawable_get_glyphs_width(self, t, false);

	if ( t->fonts )
		my->save_font(self, &savefont);

	if ( t-> fonts )
		my->switch_font( self, &savefont, t-> fonts[0]);

	if (( abc = Drawable_call_get_font_abc( self, t->glyphs[0], t->glyphs[0], toGlyphs)) != NULL) {
		int l = t->len - 1;
		ovx.x = abc->a;
		if ( t->glyphs[0] == t->glyphs[l] && ( !t->fonts || t->fonts[0] == t->fonts[l])) {
			ovx.y = abc->c;
			free(abc);
		} else {
			free(abc);
			if (( abc = Drawable_call_get_font_abc( self, t->glyphs[l], t->glyphs[l], toGlyphs)) != NULL) {
				ovx.y = abc->c;
				free(abc);
			}
		}
	}

	if ( t->fonts )
		my->restore_font(self, &savefont);

	Drawable_calculate_text_box(self, l, text_out_baseline, ovx, pt);
}

SV *
Drawable_get_text_box( Handle self, SV * text, int from, int len )
{
	dmARGS;
	Point * p;
	AV * av;
	int i, flags = 0;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		dmCHECK(NULL_SV);

		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		}
		if ((len = Drawable_check_length(from,len,dlen)) == 0)
			return newRV_noinc(( SV *) newAV());
		c_text = Drawable_hop_text(c_text, flags & toUTF8, from);
		dmENTER( newRV_noinc(( SV *) newAV()));
		p = apc_gp_get_text_box( self, c_text, len, flags);
		dmLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		dmCHECK(NULL_SV);
		if (!Drawable_read_glyphs(&t, text, 0, "Drawable::get_text_box"))
			return newRV_noinc(( SV *) newAV());
		if (( len = Drawable_check_length(from,len,t.len)) == 0)
			return newRV_noinc(( SV *) newAV());
		Drawable_hop_glyphs(&t, from, len);
		if (t.advances) {
			if (!( p = malloc( sizeof(Point) * 5 )))
				return newRV_noinc(( SV *) newAV());
			Drawable_get_glyphs_box(self, &t, p);
		} else {
			dmENTER( newRV_noinc(( SV *) newAV()));
			p = apc_gp_get_glyphs_box( self, &t);
			dmLEAVE;
		}
	} else {
		SV * ret;
		dmENTER( newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "get_text_box", "<Hii", self, from, len ));
		dmLEAVE;
		return ret;
	}

	av = newAV();
	if ( p) {
		for ( i = 0; i < 5; i++) {
			av_push( av, newSViv( p[ i]. x));
			av_push( av, newSViv( p[ i]. y));
		};
		free( p);
	}
	return newRV_noinc(( SV *) av);
}

#ifdef __cplusplus
}
#endif
