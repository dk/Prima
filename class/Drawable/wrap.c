#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "Drawable_private.h"

#ifdef WITH_LIBTHAI
#include <thai/thwchar.h>
#include <thai/thwbrk.h>
#define LIBTHAI_MAX_TEXT_BREAKS 2048
#define dLIBTHAI(tb) int libthai_buf[LIBTHAI_MAX_TEXT_BREAKS];tb.word_breaks=libthai_buf
#else
#define dLIBTHAI(tb)
#endif

#ifdef __cplusplus
extern "C" {
#endif

static PFontABC
find_abc_in_list_cache( PList p, int base )
{
	int i;
	for ( i = 0; i < p-> count; i += 2)
		if (( unsigned int) p-> items[ i] == base)
			return ( PFontABC) p-> items[i + 1];
	return NULL;
}

static Bool
fill_abc_list_cache( PList * cache, int base, PFontABC abc)
{
	PList p;
	if ( *cache == NULL )
		*cache = plist_create( 8, 8);
	if (( p = *cache) == NULL)
		return false;
	list_add( p, ( Handle) base);
	list_add( p, ( Handle) abc);
	return true;
}

PFontABC
Drawable_call_get_font_abc( Handle self, unsigned int from, unsigned int to, int flags)
{
	PFontABC abc;

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_abc( self, from, to, flags);
		if ( !abc) return NULL;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		dmARGS;
		dmCHECK(NULL);
		dmENTER(NULL);
		abc = apc_gp_get_font_abc( self, from, to, flags);
		dmLEAVE;
		if ( !abc) return NULL;
	} else {
		SV * sv;
		int len = to - from + 1;
		if ( !( abc = malloc(len * sizeof( FontABC)))) return NULL;
		sv = my-> get_font_abc( self, from, to, flags);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > len * 3) n = len * 3;
			n = ( n / 3) * 3;
			if ( n < len) memset( abc, 0, len * sizeof( FontABC));
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
			memset( abc, 0, len * sizeof( FontABC));
		sv_free( sv);
	}

	return abc;
}

static PFontABC
call_get_font_abc_base( Handle self, unsigned int base, int flags)
{
	return Drawable_call_get_font_abc( self, base * 256, base * 256 + 255, flags);
}

static PFontABC
query_abc_range( Handle self, TextWrapRec * t, unsigned int base)
{
	PFontABC abc;

	if ( t-> utf8_text) {
		if ( 
			*(t-> unicode) && 
			(( abc = find_abc_in_list_cache( *(t->unicode), base)) != NULL)
		)
			return abc;
	} else if (*( t-> ascii))
		return *(t-> ascii);

	if ( !( abc = call_get_font_abc_base(self, base, t-> utf8_text ? toUTF8 : 0)))
		return NULL;

	if ( t-> utf8_text) {
		if ( !fill_abc_list_cache(t->unicode, base, abc)) {
			free( abc);
			return NULL;
		}
	} else
		*(t-> ascii) = abc;

	return abc;
}

static Bool
precalc_abc_buffer( PFontABC src, float * width, PFontABC dest)
{
	int i;
	if ( !src || !dest) return false;
	for ( i = 0; i < 256; i++) {
		width[i] = src[i]. a + src[i]. b + src[i]. c;
		dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
		dest[i]. b = src[i]. b;
		dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
	}
	return true;
}

static Bool
precalc_ac_buffer( PFontABC src, PFontABC dest)
{
	int i;
	if ( !dest) return false;
	for ( i = 0; i < 256; i++) {
		dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
		dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
	}
	return true;
}

static Bool
fill_font_ranges( Handle self )
{
	if ( Drawable_get_font_ranges == my->get_font_ranges ) {
		dmCHECK(false);
		if ( !var-> font_abc_glyphs_ranges ) {
			if ( !( var-> font_abc_glyphs_ranges = apc_gp_get_font_ranges(self, &var->font_abc_glyphs_n_ranges)))
				return false;
		}
	} else {
		SV * sv;
		void * array;
		Bool do_free;
		sv = my-> get_font_ranges( self);
		array = prima_read_array( sv, "get_font_ranges", 'i', 1, -1, -1, &var->font_abc_glyphs_n_ranges, &do_free);
		if ( !array ) {
			sv_free(sv);
			return false;
		}
		if ( do_free ) {
			var-> font_abc_glyphs_ranges = array;
		} else {
			int size = var->font_abc_glyphs_n_ranges * sizeof(int);
			if ( !( var-> font_abc_glyphs_ranges = malloc(size))) {
				warn("Not enough memory");
				sv_free(sv);
				return false;
			}
			memcpy( var-> font_abc_glyphs_ranges, array, size );
			free(array);
		}
		sv_free(sv);
	}
	return true;
}

static PFontABC
query_abc_range_glyphs( Handle self, GlyphWrapRec * t, unsigned int base)
{
	PFontABC abc;

	if ( 
		*(t-> cache) && 
		(( abc = find_abc_in_list_cache( *(t->cache), base)) != NULL)
	)
		return abc;

	if ( !( abc = call_get_font_abc_base(self, base, toGlyphs)))
		return NULL;
	if ( t->fonts) {
		/* different fonts case */
		Byte * fa;
		PassiveFontEntry *pfe;
		int i;
		uint32_t from, to;
		unsigned int page;
		char * key;
		SaveFont savefont;
		Byte used_fonts[MAX_CHARACTERS / 8], filled_entries[256 / 8];

		from = base * 256;
		to   = from + 255;
		page = from >> FONTMAPPER_VECTOR_BASE;
		bzero(used_fonts, sizeof(used_fonts));
		bzero(filled_entries, sizeof(filled_entries));
		used_fonts[0] = 0x01; /* fid = 0 */
		key = Drawable_font_key(var->font.name, var->font.style);
		i = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));
		my->save_font(self, &savefont);
		if ( i > 0 ) {
			/* copy ranges from subst table */
			pfe = PASSIVE_FONT(i);
			if ( !pfe-> ranges_queried )
				Drawable_query_ranges(pfe);
			if ( pfe-> vectors.count <= page ) goto NO_FONT_ABC; /* should be there, or some error */
			/* page covers the 256 range in whole */
			fa = (Byte *) pfe-> vectors.items[ page ];
			if ( fa ) {
				i = from & FONTMAPPER_VECTOR_MASK;
				memcpy( filled_entries, fa + i, 256 / 8);
			}
		} else {
			/* query the range and fill the cache */
			unsigned long * ranges;
			if ( !fill_font_ranges(self))
				goto NO_FONT_ABC;
			ranges = var-> font_abc_glyphs_ranges;
			for ( i = 0; i < var->font_abc_glyphs_n_ranges; i += 2, ranges += 2 ) {
				int j;
				if ( ranges[0] > to || ranges[1] < from )
					continue;
				for ( j = ranges[0]; j <= ranges[1]; j++) {
					if ( j >= from && j <= to )
						filled_entries[(j - from) >> 3] |= 1 << ((j - from) & 7);
				}
			}
		}

		for ( i = 0; i < t->n_glyphs; i++) {
			PFontABC abc2;
			uint16_t fid = t->fonts[i];
			uint32_t uv;
			if ( used_fonts[fid >> 3] & ( 1 << (fid & 7)))
				continue;
			used_fonts[fid >> 3] |= 1 << (fid & 7);

			pfe = PASSIVE_FONT(fid);
			if ( !my->switch_font(self, &savefont, fid))
				continue;

			if ( !pfe-> ranges_queried )
				Drawable_query_ranges(pfe);
			if ( pfe-> vectors.count <= page )
				continue;

			if ( !( abc2 = Drawable_call_get_font_abc( self, from, to, toGlyphs)))
				continue;

			fa = (Byte *) pfe-> vectors.items[ page ];
			if ( !fa ) continue;
			for ( uv = from; uv <= to; uv++) {
				unsigned int bit = uv & FONTMAPPER_VECTOR_MASK;
				if (( fa[bit >> 3] & (1 << (bit & 7))) == 0) continue;
				if ((filled_entries[(uv - from) >> 3] & (1 << ((uv - from) & 7))) != 0) continue;
				filled_entries[(uv - from) >> 3] |= 1 << ((uv - from) & 7);
				abc[uv - from] = abc2[uv - from];
			}

			free(abc2);
		}
		my->restore_font( self, &savefont );
	}
NO_FONT_ABC:

	if ( !fill_abc_list_cache(t->cache, base, abc)) {
		free( abc);
		return NULL;
	}

	return abc;
}

static int
find_tilde_position( TextWrapRec * t )
{
	int i, tildeIndex = -100;

	for ( i = 0; i < t-> textLen - 1; i++) {
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
	}

	return tildeIndex;
}

static void
fill_tilde_properties(Handle self, TextWrapRec * t, int tildeIndex, int tildePos, int tildeCharPos, int tildeOffset)
{
	UV uv;
	int base;
	PFontABC abcc;
	float start, end;

	t-> t_bytepos = tildePos;
	t-> t_pos  = tildeCharPos + (( t->options & twCollapseTilde ) ? 0 : 1);
	t-> t_char = t-> text + tildeIndex + 1;
	if ( t-> utf8_text) {
		unsigned int len;
		uv = prima_utf8_uvchr_end(t-> t_char, t->text + t-> textLen, &len);
		if ( len == 0 ) return;
	} else
		uv = *(t->t_char);

	abcc = query_abc_range( self, t, base = uv / 256) + (uv & 0xff);
	start = tildeOffset;
	end   = start + ((abcc->a < 0) ? 0 : abcc->a) + abcc-> b + ((abcc->c < 0) ? 0 : abcc->c) - 1.0;
	if ( abcc-> a < 0.0 ) {
		start += abcc->a;
		end += abcc->a;
	}
	t-> t_start = start + .5 * (( start < 0 ) ? -1 : 1);
	t-> t_end   = end   + .5 * (( end   < 0 ) ? -1 : 1);
}

#ifdef WITH_LIBTHAI
static void
text_get_libthai_breaks( TextWrapRec* t)
{
	unsigned int charlen;
	char * utf8, thbuf[1024 * sizeof(thwchar_t)];
	int target_len_xchars, src_len_bytes, got_thai_chars = 0;
	thwchar_t *u32;
	semistatic_t pbuf;

	if ( prima_guts.use_libthai == 1 ) {
		ThBrk * t;
		if (!( t = th_brk_new(NULL))) {
			warn("libthai error, disabling");
			prima_guts.use_libthai = 0;
		} else {
			prima_guts.use_libthai = 2;
			th_brk_delete(t);
		}
	}

	utf8              = t-> text;
	target_len_xchars = t-> utf8_textLen;
	src_len_bytes     = t-> textLen;
	semistatic_init(&pbuf, &thbuf, sizeof(thwchar_t), 1024);

	if ( !semistatic_expand(&pbuf, t->utf8_textLen + 1)) {
		warn("Not enough memory");
		goto FAIL;
	}
	u32 = (thwchar_t*) pbuf.heap;
	while ( target_len_xchars--) {
		UV u = (thwchar_t) prima_utf8_uvchr(utf8, src_len_bytes, &charlen);
		*(u32++) = u;
		if ( u >= 0xE00 && u <= 0xE7F ) /* thai code block */
			got_thai_chars = 1;
		utf8 += charlen;
		src_len_bytes -= charlen;
		if ( src_len_bytes <= 0 || charlen == 0) break;
	}
	if ( !got_thai_chars )
		goto FAIL;
	*u32 = 0;

	t-> n_word_breaks = th_brk_wc_find_breaks(NULL, (thwchar_t*)pbuf.heap, t->word_breaks, LIBTHAI_MAX_TEXT_BREAKS);
#ifdef _DEBUG
	if ( t-> n_word_breaks ) {
		int i;
		printf("thai breaks:");
		for ( i = 0; i < t-> n_word_breaks; i++) printf(" %d", t->word_breaks[i]);
		printf("\n");
	}
#endif
FAIL:
	semistatic_done(&pbuf);
}
#endif

static void
text_init_wrap_rec( Handle self, SV * text, int width, int options, int tabIndent, int from, int len, TextWrapRec * t)
{
	STRLEN tlen;

	t-> text      = SvPV( text, tlen);
	t-> utf8_text = prima_is_utf8_sv( text);
	if ( t-> utf8_text) {
		t-> utf8_textLen = prima_utf8_length( t-> text, tlen);
		if (( t-> utf8_textLen = Drawable_check_length(from, len, t-> utf8_textLen)) == 0)
			from = 0;
		t-> text = Drawable_hop_text(t->text, true, from);
		t-> textLen = utf8_hop(( U8*) t-> text, t-> utf8_textLen) - (U8*) t-> text;
	} else {
		if ((tlen = Drawable_check_length(from, len, tlen)) == 0)
			from = 0;
		t-> text = Drawable_hop_text(t->text, false, from);
		t-> utf8_textLen = t-> textLen = tlen;
	}

	t-> width     = width;
	t-> tabIndent = ( tabIndent < 0) ? 0 : tabIndent;
	t-> options   = options;
	t-> ascii     = &var-> font_abc_ascii;
	t-> unicode   = &var-> font_abc_unicode;
	t-> t_char    = NULL;
	t-> t_start   = C_NUMERIC_UNDEF;
	t-> t_end     = C_NUMERIC_UNDEF;
	t-> t_line    = C_NUMERIC_UNDEF;
	t-> t_pos     = C_NUMERIC_UNDEF;
	t-> t_bytepos = C_NUMERIC_UNDEF;
	t-> count     = 0;
	t-> n_word_breaks = 0;

#ifdef WITH_LIBTHAI
	if ( prima_guts.use_libthai && t-> utf8_text && (t->options & twWordBreak))
		text_get_libthai_breaks(t);
#endif
}

static SV*
mnemonic2sv(TextWrapRec * t)
{
	HV * profile = newHV();
	if ( t-> t_char) {
		STRLEN len = t-> utf8_text ? utf8_hop(( U8*) t-> t_char, 1) - ( U8*) t-> t_char : 1;
		SV * sv_char = newSVpv( t-> t_char, len);
		pset_sv_noinc( tildeChar, sv_char);
		if ( t->utf8_text) SvUTF8_on( sv_char);
		if ( t->t_start != C_NUMERIC_UNDEF) pset_i( tildeStart, t->t_start);
		if ( t->t_end   != C_NUMERIC_UNDEF) pset_i( tildeEnd,   t->t_end);
		if ( t->t_line  != C_NUMERIC_UNDEF) pset_i( tildeLine,  t->t_line);
		if ( t->t_pos   != C_NUMERIC_UNDEF) pset_i( tildePos,   t->t_pos);
	}
	return newRV_noinc(( SV *) profile);
}

static SV*
first_line2sv(int * c, int count )
{
	int rlen = ( c && count > 0) ? c[3] : 0;
	return newSViv( rlen);
}

static SV*
chunks2sv(Handle self, int from, int * c, int count)
{
	int i;
	AV * av = newAV();

	for ( i = 0; i < count; i += 4) {
		av_push( av, newSViv(from + c[i+2]) );
		av_push( av, newSViv(c[i+3]) );
	}

	return (SV*)av;
}

static SV*
textout2sv(Handle self, int * c, TextWrapRec * t)
{
	int i, line;
	char buf[256];
	semistatic_t pbuf;
	AV * av = newAV();

	if ( t-> options & twExpandTabs )
		semistatic_init(&pbuf, &buf, 1, 256);

	for ( i = line = 0; i < t-> count; i += 4, line++) {
		SV * sv;
		if ( t-> options & twExpandTabs ) {
			int j, nt, len = c[i+1], sz;
			char *src, *dst;
			for ( j = nt = 0, src = t->text + c[i]; j < len; j++, src++ )
				if ( *src == '\t' )
					nt++;
			if ( nt == 0 ) goto AS_IS;

			sz = len + nt * (t->tabIndent - 1) + 1;
			if ( !semistatic_expand(&pbuf, sz)) {
				warn("Not enough memory");
				sv_free((SV*) av);
				return NULL_SV;
			}
			for ( j = 0, src = t->text + c[i], dst = (char*)pbuf.heap; j < len; j++, src++ ) {
				if ( *src == '\t') {
					int k;
					for ( k = 0; k < t->tabIndent; k++ )
						*(dst++) = ' ';
				}
				else
					*(dst++) = *src;
			}
			sv = newSVpvn((char*) pbuf.heap, sz - 1 );
		} else {
		AS_IS:
			sv = newSVpvn( t->text + c[i], c[i+1]);
		}
		if (( t-> options & twCollapseTilde) && ( line == t-> t_line) && t-> t_char) {
			STRLEN tlen;
			char * pv = SvPV( sv, tlen );
			memmove( pv + t-> t_bytepos, pv + t-> t_bytepos + 1, tlen - t-> t_bytepos - 1);
			pv[tlen] = 0;
			SvCUR_set(sv, tlen-1);
			SvPOK_only(sv);
		}

		if ( t->utf8_text) SvUTF8_on( sv);
		av_push( av, sv );
	}

	if ( t-> options & twExpandTabs )
		semistatic_done(&pbuf);

	return (SV*)av;
}

typedef struct {
	int * storage;
	unsigned int bufsize, base, options;
	int width, limit, utf8_limit;
	float widths[256];
	FontABC abcs[256];
	struct {
		int start, utf8_start;
		int end;
		int split_start, utf8_split_start;
		int split_end, utf8_split_end;
		int p, utf8_p;
	} curr, prev;
	Bool do_width_break, first_only;
	int tilde_index, tilde_line, tilde_pos, tilde_char_pos, tilde_offset;
} WrapRec;

static Bool
wrap_init( WrapRec * w, TextWrapRec * tw, GlyphWrapRec * gw)
{
	bzero( w, sizeof(WrapRec));
	w-> width            = tw ? tw->width        : gw->width;
	w-> options          = tw ? tw->options      : gw->options;
	w-> limit            = tw ? tw->textLen      : gw->n_glyphs;
	w-> utf8_limit       = tw ? tw->utf8_textLen : gw->n_glyphs;

	w-> do_width_break   = w->width >= 0;
	w-> first_only       = (w->options & twReturnFirstLineLength) == twReturnFirstLineLength;

	w-> base             = 0x10000000;

	w-> curr.split_start = w-> curr.split_end = w-> curr.utf8_split_start = w-> curr.utf8_split_end = -1;
	w-> prev             = w-> curr;

	w-> bufsize          = 128;
	if (!( w-> storage = allocn( int, w-> bufsize))) return false;

	w-> tilde_index      = -100;

	return true;
}

static Bool
wrap_add_entry( WrapRec * w, TextWrapRec * tw, GlyphWrapRec * gw, int end, int utf_end )
{
	int *count = tw ? &tw->count : &gw->count;
	if ( *count == w-> bufsize) {
		int * n;
		if ( !( n = (int *)realloc( w->storage, sizeof(unsigned int) * (w->bufsize *= 2))))
			return false;
		w->storage = n;
	}
#ifdef _DEBUG
	printf("add %d - %d\n", w-> curr. utf8_start, end);
#endif

	if (
		tw && 
		w-> tilde_index >= 0 &&
		w-> tilde_index >= w-> curr.start &&
		w-> tilde_index < end
	) {
		char
			*line     = tw-> text + w-> curr.start,
			*tilde_at = tw->text + w-> tilde_index;
		w-> tilde_char_pos = 0;
		while ( line < tilde_at ) {
			line = (char*)utf8_hop((U8*)line, 1);
			w-> tilde_char_pos++;
		}
		w-> tilde_line = tw-> t_line = *count / 4;
		w-> tilde_pos  = w->tilde_index - w->curr.start;
		if ( w-> tilde_index == end - 1) tw-> t_line++;
	}

	w-> storage[(*count)++] = w-> curr.start;
	w-> storage[(*count)++] = end - w-> curr.start;
	w-> storage[(*count)++] = w-> curr.utf8_start;
	w-> storage[(*count)++] = utf_end - w-> curr.utf8_start;
	if ( tw && gw ) gw-> count = tw-> count;

	w-> curr.start      = end;
	w-> curr.utf8_start = utf_end;

	return !w-> first_only;
}

#define wrap_new_word(w,len,utflen)               \
	w.curr.split_start      = w.curr.p;       \
	w.curr.split_end        = w.curr.p + len; \
	w.curr.utf8_split_start = w.curr.utf8_p;  \
	w.curr.utf8_split_end   = w.curr.utf8_p + utflen

#define wrap_fetch_uvchr(w, tw, len)              \
	((len = 1) && tw->utf8_text) ?            \
		prima_utf8_uvchr_end(             \
			tw->text + w.curr.p,      \
			tw->text + tw-> textLen,  \
			&len                      \
		) :                               \
		((unsigned char*)(tw-> text))[w.curr.p]

#define wrap_step_ptr(w,len,utflen)               \
	w.curr.p += len;                          \
	w.curr.utf8_p += utflen

static Bool
wrap_load_glyphs_abc(uint32_t uv, WrapRec * w, Handle self, GlyphWrapRec *g)
{
	PFontABC labc;
	if ( uv / 256 == w-> base)
		return true;
	w-> base = uv / 256;
	if ( !(labc = query_abc_range_glyphs( self, g, w->base)))
		return false;
	if ( g-> advances )
		precalc_ac_buffer(labc, w->abcs);
	else
		precalc_abc_buffer(labc, w->widths, w->abcs);
	return true;
}

/* a very quick check, if possible, if glyphstr fits */

static SV*
glyphs_fit_quickcheck(Handle self, SV * glyphs, int width, int options, TextWrapRec *tw, GlyphsOutRec *g)
{
	AV * av;
	if (!(
		(g->len == 0) ||
		(g->advances && ( width >= Drawable_get_glyphs_width(self, g, true)))
	))
		return NULL;

	if (( options & twReturnFirstLineLength) == twReturnFirstLineLength)
		return newSViv(tw ? tw-> utf8_textLen : g->len);

	av = newAV();
	if ( options & twReturnChunks) {
		av_push( av, newSViv(0));
		av_push( av, newSViv(tw ? tw-> utf8_textLen : g->len));
	} else if ( !tw || options & twReturnGlyphs ) {
		av_push( av, newSVsv(sv_call_perl(glyphs, "clone", "<S", glyphs)));
	} else {
		SV * sv = newSVpv( tw-> text, tw-> textLen );
		if ( tw->utf8_text) SvUTF8_on( sv);
		av_push( av, sv );
	}
	return newRV_noinc(( SV *) av);
}

static void
glyph_init_wrap_rec( Handle self, int width, int options, int offset, GlyphsOutRec *g, GlyphWrapRec *t)
{
	t->offset    = offset;
	t->n_glyphs  = g->len;
	t->glyphs    = g->glyphs;
	t->indexes   = g->indexes;
	t->advances  = g->advances;
	t->positions = g->positions;
	t->fonts     = g->fonts;
	t->width     = width;
	t->text_len  = g->text_len;
	t->options   = options;
	t->cache     = &var-> font_abc_glyphs;
	t->count     = 0;
}

static SV*
glyphout2sv(Handle self, int * c, GlyphsOutRec *g, TextWrapRec *tw, GlyphWrapRec *gw, uint16_t* log2vis)
{
#define STATIC_BUF_SIZE 1024
	int i, line;
	AV * av;
	int j,
		mul[5] = { 1, 1, 1, 2, 1 },
		extras[5] = {0, 1, 0, 0, 0},
		got_tab = 0;
	uint16_t *payload[5] = { g->glyphs, g->indexes, g->advances, (uint16_t*)g->positions, g->fonts };
	uint16_t buf1[STATIC_BUF_SIZE];
	uint32_t buf2[STATIC_BUF_SIZE];
	semistatic_t sbuf, tbuf;

	av = newAV();

	semistatic_init(&sbuf, &buf1, sizeof(uint16_t), STATIC_BUF_SIZE);

	if ( tw != NULL && (tw->options & twExpandTabs)) {
		int k, l;
		char *text;
		semistatic_init(&tbuf, &buf2, sizeof(uint32_t), STATIC_BUF_SIZE);
		for (
			k = 0, l = 0, text = tw->text;
			k < tw-> utf8_textLen;
			k++, l++
		) {
			uint32_t uv;
			unsigned int len;
			if (tw->utf8_text) {
				uv = prima_utf8_uvchr_end(text, tw->text + tw-> textLen, &len);
				if ( len < 1 ) break;
				text += len;
			} else {
				uv = *(text++);
			}
			if ( !semistatic_push(tbuf,uint32_t,uv))
				goto FAIL;
			if ( uv == '\t')
				got_tab = 1;
		}
	}

	for ( i = 2, line = 0; i < gw->count; i += 4, line++) {
		SV *sv_payload[5];
		uint16_t *dest[5];
		int first_char = c[i] + gw->offset, last_char = first_char + c[i + 1];

		sbuf.count = 0;

		if ( tw && gw->indexes) {
			/* copy subset by text and use indexes */
			for ( j = 0; j < g->len; j++) {
				int ix = g->indexes[j] & ~toRTL;
				if ( ix < first_char || ix >= last_char ) continue;
				if (
					tw &&
					( tw-> options & twCollapseTilde) &&
					tw-> t_char &&
					line == tw-> t_line &&
					ix == tw-> t_pos
				)
					continue;
				if ( !semistatic_push(sbuf,uint16_t,j))
					goto FAIL;
			}
		} else {
			/* copy as is */
			for ( j = first_char; j < last_char; j++)
				if ( !semistatic_push(sbuf,uint16_t,j))
					goto FAIL;
		}

		for ( j = 0; j < 5; j++) {
			SV * sv;
			uint16_t *dst, k;
			if ( payload[j] == NULL ) {
				sv_payload[j] = NULL_SV;
				continue;
			}
			sv  = prima_array_new(sizeof(uint16_t) * (sbuf.count * mul[j] + extras[j]));
			dest[j] = dst = (uint16_t*)prima_array_get_storage(sv);
			if ( mul[j] == 1 ) {
				for ( k = 0; k < sbuf.count; k++)
					*(dst++) = payload[j][semistatic_at(sbuf,uint16_t,k)];
				if ( j == 1 )
					*dst = payload[j][g->len];
			} else {
				for ( k = 0; k < sbuf.count; k++) {
					int ix = semistatic_at(sbuf,uint16_t,k) * 2;
					*(dst++) = payload[j][ix];
					*(dst++) = payload[j][ix+1];
				}
			}

			if ( j == 2 && got_tab ) {
				uint16_t *advances = dest[2], *indexes = dest[1];
				for ( k = 0; k < sbuf.count; k++)
					if ( semistatic_at(tbuf, uint32_t, indexes[k] & ~toRTL) == '\t' )
						advances[ k ] *= tw-> tabIndent;
			}
			sv_payload[j] = sv_2mortal(prima_array_tie( sv, sizeof(uint16_t), (j == 3) ? "s" : "S"));
		}

		av_push( av, newSVsv(
			call_perl(self, "new_glyph_obj", "<SSSSS",
				sv_payload[0],
				sv_payload[1],
				sv_payload[2],
				sv_payload[3],
				sv_payload[4]
			)
		));
	}

	semistatic_done(&sbuf);
	return (SV*)av;
#undef STATIC_BUF_SIZE

FAIL:
	semistatic_done(&sbuf);
	sv_free((SV*)av);
	return NULL_SV;
}

static uint16_t *
fill_log2vis(GlyphsOutRec *g, int from)
{
	int i;
	uint16_t *l2v, *ix = g->indexes, last = 0;
	if (( l2v = malloc( sizeof(uint16_t) * g->text_len)) == NULL)
		return NULL;
	if ( ix ) {
		memset(l2v, 0xff, sizeof(uint16_t) * g->text_len);
		for ( i = 0; i < g->len; i++) {
			int v = ix[i] & ~toRTL;
			if ( l2v[v] > i ) l2v[v] = i;
		}
		for ( i = 0; i < g->text_len; i++) {
			if ( l2v[i] != 0xffff )
				last = l2v[i];
			else
				l2v[i] = last;
		}
	} else {
		for ( i = 0; i < g->text_len; i++)
			l2v[i] = i;
	}

	return l2v;
}

int *
Drawable_do_text_wrap( Handle self, TextWrapRec * tw, GlyphWrapRec * gw, uint16_t * log2vis)
{
	WrapRec wr;
	float w = 0, initial_overhang = 0;
	Bool reassign_w = 0;
	int space_width = -1, space_c = 0, curr_word_break_idx = -1;

	if ( !wrap_init(&wr, tw, gw))
		return NULL;
	if ( tw && tw-> n_word_breaks )
		curr_word_break_idx = 0;

	/* determining ~ character location */
	if ( wr.options & twCalcMnemonic)
		wr.tilde_index = find_tilde_position(tw);

#define ADD(ptr) \
	if ( !wrap_add_entry( &wr, tw, gw, wr.curr.ptr, wr.curr.utf8_##ptr))  \
		return wr.storage

#define LOAD_ABC(x) \
	if ( !wrap_load_glyphs_abc(x, &wr, self, gw)) \
		return wr.storage

#define RETURN_EMPTY if (1) {             \
	if ( gw ) gw-> count = 0;         \
	if ( tw ) tw-> count = 0;         \
	return wr.storage;                \
}

	while ( wr.curr.p < wr.limit ) {
		float dw, c;
		unsigned int j, nc, ng, wmul = 1;
		unsigned int len = 1;
		uint32_t uv, uv0, last_uv = 0;
		uint16_t index;

		wr.prev = wr.curr;

		/* nc: codepoints in the cluster */
		if ( log2vis ) {
			unsigned int v, cmp;
			for (
				nc = 1, v = wr.curr.utf8_p + 1, index = log2vis[wr.curr.utf8_p];
				v < tw->utf8_textLen;
				v++
			) {
				if ( log2vis[v] != index ) break;
				nc++;
			}
			for (
				ng = 1,
					v = log2vis[wr.curr.utf8_p] + 1,
					cmp = gw->indexes[log2vis[wr.curr.utf8_p]] & ~toRTL;
				v < gw->n_glyphs;
				v++
			) {
				if (( gw->indexes[v] & ~toRTL ) != cmp ) break;
				ng++;
			}
		} else {
			ng    = 1;
			nc    = 1;
			index = wr.curr.utf8_p;
		}

#define wrap_fetch_uvchr(w, tw, len)              \
	((len = 1) && tw->utf8_text) ?            \
		prima_utf8_uvchr_end(             \
			tw->text + w.curr.p,      \
			tw->text + tw-> textLen,  \
			&len                      \
		) :                               \
		((unsigned char*)(tw-> text))[w.curr.p]
		uv = tw ? wrap_fetch_uvchr(wr,tw,len) : 0;
		if ( len < 1 ) break;

		if ( curr_word_break_idx >= 0 && tw->word_breaks[curr_word_break_idx] == wr.curr.utf8_p) {
			if ( ++curr_word_break_idx >= tw->n_word_breaks)
				curr_word_break_idx = -1;
			wrap_new_word(wr,0,0);
			goto NON_BREAKER;
		}

		if ( !tw || nc > 1 )
			goto NON_BREAKER;

		switch ( uv ) {
		case '\n':
		case 0x2028:
		case 0x2029:
			wrap_new_word(wr,len,1);
			if (!( wr.options & twNewLineBreak))
				goto NON_BREAKER;
			break;

		case ' ':
			wrap_new_word(wr,len,1);
			if (!( wr.options & twSpaceBreak))
				goto NON_BREAKER;
			break;
		case '\t':
			wrap_new_word(wr,len,1);
			if ( wr.options & twCalcTabs)
				wmul = tw->tabIndent;
			if (!( wr.options & twSpaceBreak))
				goto NON_BREAKER;
			if ( space_width < 0 ) {
				PFontABC s;
				if ( !( s = query_abc_range( self, tw, 0)))
					return wr.storage;
				space_c     = (s[' '].c < 0) ? - s[' ']. c : 0;
				space_width = (s[' '].a + s[' '].b + s[' '].c) * tw-> tabIndent;
			}
			dw   = space_width;
			c    = space_c;
			goto PREDEFINED_WIDTH;
		case '~':
			if ( wr.curr.p == wr.tilde_index ) {
				wr.tilde_offset = w - initial_overhang;
				dw = c = 0;
				goto PREDEFINED_WIDTH;
			}
			goto NON_BREAKER;
		default:
			goto NON_BREAKER;
		}

		ADD(p);
		wrap_step_ptr(wr, len, 1);
		wr.curr.start      = wr.curr.p;
		wr.curr.utf8_start = wr.curr.utf8_p;
		reassign_w = 1;
		continue;
	NON_BREAKER:

		/* calculate widths */
		dw = c = 0;
		if ( gw ) {
			for ( j = 0, uv = uv0 = 0; j < ng; j++) {
				last_uv = uv;
				uv = gw->glyphs[index + j];
				if ( j == 0 ) uv0 = uv;
				if (!gw-> advances || reassign_w) /* do not query ABC unnecessarily if advances are there */
					LOAD_ABC(last_uv);
				dw += (gw->advances ? gw->advances[index + j] : wr.widths[uv & 0xff]) * wmul;
				if ( j == nc - 1 && !gw-> advances)
					c = wr.abcs[uv & 0xff].c;
				if ( reassign_w) {
					w = initial_overhang = wr.abcs[uv & 0xff].a;
					reassign_w = 0;
				}
			}
		} else {
			if ( uv / 256 != wr.base)
				if ( !precalc_abc_buffer( query_abc_range( self, tw, wr.base = uv / 256), wr.widths, wr.abcs))
					return wr.storage;
			uv0 = uv;
			dw  = wr.widths[uv & 0xff];
			c   = wr.abcs[uv & 0xff].c;

		}
		if ( reassign_w) {
			w = initial_overhang = wr.abcs[uv0 & 0xff].a;
			reassign_w = 0;
		}
	PREDEFINED_WIDTH:

		/* advance text pointers */
		if ( tw ) {
			for ( j = 0; j < nc; j++) {
				wrap_fetch_uvchr(wr,tw,len);
				if ( len < 1 ) break;
				wrap_step_ptr(wr, len, 1);
			}
		} else {
			wrap_step_ptr(wr, 1, 1);
		}

#ifdef _DEBUG
		printf("i:%d/%d nc:%d ng:%d w:%f dw:%f c:%f index:%d uv:%x\n",  wr.curr.p, wr.curr.utf8_p, nc, ng, w, dw, c, index, uv0);
#endif
		if ( !wr.do_width_break || (w + dw + c <= wr.width)) {
			w += dw;
			continue;
		}

		if ( gw && gw-> advances && wr.prev.p > wr.curr.start ) {
			/* this glyph is clearly out of bounds, but it could be that the previous was too.

			The reason behind this complication is that fetching every glyphs A/C metrics under libXft,
			on probably under win32, requires the whole glyph to be fetched. This hiccups if the string
			or font size are so unfortunate that glyphs are being discarded often. But since C is used only
			to check whether the last glyph hangs over the limit or not, we don't query C until necessary.
			The complication is that we need to step back if the previous glyph's C was big enough to make it
			not fit either.

			The effect can be seen when selecting with mouse chinese text in podview in Prima/Drawable/Glyphs -
			when each glyph is queried, it might take several seconds for each redraw.
			*/
			LOAD_ABC(last_uv);
			if ( w + wr.abcs[last_uv & 0xff].c > wr.width ) /* ... and it is */
				wr.curr = wr.prev;
		}

		if ( wr.prev.p == wr.curr.start) {
			/* case when even single char cannot be fit in  */
			if ( wr.options & twBreakSingle) RETURN_EMPTY;
			/* or push this character disregarding the width */
			ADD(p);
			reassign_w = 1;
		} else {
			/* normal break condition */
			if ( wr.options & twWordBreak) {
				if (
					/* checking if break was at word boundary */
					(wr.curr.start < wr.curr.split_start) ||
					/* or add at least the breaker char itself */
					(wr.curr.start == wr.curr.split_start && wr.curr.split_start < wr.curr.split_end)
				) {
					ADD(split_start);
					wr.curr.p      = wr.curr.start      = wr.curr.split_end;
					wr.curr.utf8_p = wr.curr.utf8_start = wr.curr.utf8_split_end;
					w = 0;
					reassign_w = 1;
					continue;
				} else if ( wr.options & twBreakSingle) {
					/* cannot be split */
					RETURN_EMPTY;
				}
			}

			/* repeat again */
			wr.curr = wr.prev;
			ADD(p);
			reassign_w = 1;
		}
		w = 0;
	}

	/* adding or skipping last line */
	if (
		wr.limit - wr.curr.start > 0 ||
		( tw && tw->count == 0) ||
		( gw && gw->count == 0)
	) {
		wr.curr.p      = wr.limit;
		wr.curr.utf8_p = wr.utf8_limit;
		ADD(p);
	}

	/* fill ~ location */
	if (tw && wr.tilde_index >= 0 && !(wr.options & twReturnChunks))
		fill_tilde_properties(self, tw, wr.tilde_index, wr.tilde_pos, wr.tilde_char_pos, wr.tilde_offset);

	return wr.storage;
}
#undef ADD
#undef LOAD_ABC
#undef RETURN_EMPTY

static SV*
string_wrap( Handle self,SV * text, int width, int options, int tabIndent, int from, int len)
{
	dmARGS;
	TextWrapRec t;
	int * c;
	SV *ret;
	dLIBTHAI(t);

	if ( options & twReturnGlyphs ) {
		warn("Drawable.text_wrap only can use tw::ReturnGlyphs if glyphs are supplied");
		options &= ~twReturnGlyphs;
	}

	text_init_wrap_rec( self, text, width, options, tabIndent, from, len, &t);
	dmENTER(NULL_SV);
	c = my->do_text_wrap( self, &t, NULL, NULL);
	dmLEAVE;
	t.t_pos += from;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength)
		ret = first_line2sv(c, t.count);
	else if ( !c)
		return NULL_SV;
	else if ( options & twReturnChunks ) {
		SV * sv = chunks2sv(self, from, c, t.count);
		ret = ( sv == NULL_SV ) ? NULL_SV : newRV_noinc(sv);
	} else {
		SV * av = textout2sv(self, c, &t);
		if ( av != NULL_SV ) {
			if  (t.options & ( twCalcMnemonic | twCollapseTilde))
				av_push((AV*) av, mnemonic2sv(&t));
			ret = newRV_noinc(av);
		} else
			ret = av;
	}
	free( c);

	return ret;
}


static SV*
glyphs_wrap( Handle self, SV * text, int width, int options, int from, int len)
{
	dmARGS;
	GlyphWrapRec t;
	int * c;
	GlyphsOutRec g;
	SV *qt, *ret;

	if (!Drawable_read_glyphs(&g, text, 1, "Drawable::text_wrap"))
		return NULL_SV;
	if ((len = Drawable_check_length(from, len, g.len)) == 0)
		from = 0;
	Drawable_hop_glyphs(&g, from, len);
	if (( qt = glyphs_fit_quickcheck(self, text, width, options, NULL, &g)) != NULL)
		return qt;
	glyph_init_wrap_rec( self, width, options, 0, &g, &t);
	if (options & (twExpandTabs|twCollapseTilde|twCalcMnemonic|twCalcTabs|twWordBreak))
		warn("Drawable::text_wrap(glyphs) does not accept tw::ExpandTabs,tw::CollapseTilde,tw::CalcMnemonic,tw::CalcTabs,tw::WordBreak");

	dmENTER(NULL_SV);
	c = my->do_text_wrap( self, NULL, &t, NULL);
	dmLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength)
		ret = first_line2sv(c, t.count);
	else if ( !c)
		return NULL_SV;
	else if ( options & twReturnChunks ) {
		SV * sv = chunks2sv(self, from, c, t.count);
		ret = (sv == NULL_SV) ? NULL_SV : newRV_noinc(sv);
	} else {
		SV * sv = glyphout2sv(self, c, &g, NULL, &t, NULL);
		ret = (sv == NULL_SV) ? NULL_SV : newRV_noinc(sv);
	}
	free( c);

	return ret;
}

static SV*
string_glyphs_wrap( Handle self, SV * text, int width, int options, int tabIndent, int from, int len, SV * glyphs)
{
	dmARGS;
	SV *qt, *ret, *av = NULL;
	GlyphsOutRec g;
	TextWrapRec tw;
	GlyphWrapRec gw;
	int *c;
	uint16_t *log2vis = NULL;
	dLIBTHAI(tw);

	if ( !SvROK(glyphs) || SvTYPE( SvRV(glyphs)) != SVt_PVAV ) {
		warn("Drawable::text_wrap: not a glyph array passed");
		return NULL_SV;
	}
	if (!Drawable_read_glyphs(&g, glyphs, 1, "Drawable::text_wrap"))
		return NULL_SV;
	text_init_wrap_rec( self, text, width, options, tabIndent, 0, -1, &tw);
	if ( g.text_len != tw.utf8_textLen) {
		warn("Drawable::text_wrap: text and glyphstr don't match");
		return NULL_SV;
	}
	if ( from != 0 || len != -1 )
		text_init_wrap_rec( self, text, width, options, tabIndent, from, len, &tw);

	if (
		from == 0 && len == -1 &&
		!( options & (twCalcTabs|twExpandTabs|twSpaceBreak|twNewLineBreak|twCalcMnemonic|twCollapseTilde))
	) {
		if (( qt = glyphs_fit_quickcheck(self, glyphs, width, options, &tw, &g)) != NULL)
			return qt;
	}

	dmENTER(NULL_SV);

	glyph_init_wrap_rec( self, width, options, from, &g, &gw);
	if ( g.indexes ) {
		/* log2vis needs to address the whole string */
		if ( !( log2vis = fill_log2vis(&g, from))) {
			dmLEAVE;
			warn("not enough memory");
			return NULL_SV;
		}
	}

	c = my->do_text_wrap( self, &tw, &gw, log2vis + from);
	dmLEAVE;
	tw.t_pos += from;

	if (( options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		ret = first_line2sv(c, gw.count);
	} else if ( !c ) {
		ret = NULL_SV;
	} else if ( options & twReturnGlyphs ) {
		av = glyphout2sv(self, c, &g, &tw, &gw, log2vis + from );
		ret = ( av == NULL_SV ) ? NULL_SV : newRV_noinc(av);
	} else if ( options & twReturnChunks ) {
		SV * sv = chunks2sv(self, from, c, gw.count);
		ret = ( sv == NULL_SV ) ? NULL_SV : newRV_noinc(sv);
	} else {
		av = textout2sv(self, c, &tw);
		ret = ( av == NULL_SV ) ? NULL_SV : newRV_noinc(av);
	}

	if  (
		(tw.options & ( twCalcMnemonic | twCollapseTilde)) &&
		av &&
		SvTYPE(av) == SVt_PVAV
	)
		av_push((AV*) av, mnemonic2sv(&tw));

	if ( log2vis) free(log2vis);
	free( c);

	return ret;
}

SV*
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent, int from, int len, SV * glyphs)
{
	if ( width < 0 ) width = INT_MAX;
	if ( SvOK(glyphs)) {
		return string_glyphs_wrap(self, text, width, options, tabIndent, from, len, glyphs);
	} else if ( !SvROK( text )) {
		return string_wrap(self, text, width, options, tabIndent, from, len);
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		return glyphs_wrap(self, text, width, options, from, len);
	} else {
		SV * ret;
		dmARGS;
		dmENTER(
			(( options & twReturnFirstLineLength) == twReturnFirstLineLength) ?
				newSViv(0) : newRV_noinc(( SV *) newAV())
		);
		ret = newSVsv(sv_call_perl(text, "text_wrap", "<Hiiiii", self, width, options, tabIndent, from, len));
		dmLEAVE;
		return ret;
	}
}

#ifdef __cplusplus
}
#endif
