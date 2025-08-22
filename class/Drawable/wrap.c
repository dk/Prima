#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "private/Drawable.h"

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

static void*
find_in_cache( PList p, int base )
{
	int i;
	if ( p == NULL )
		return NULL;
	for ( i = 0; i < p-> count; i += 2)
		if (( unsigned int) p-> items[ i] == base)
			return ( void*) p-> items[i + 1];
	return NULL;
}

static Bool
fill_cache( PList * cache, int base, void *entry)
{
	PList p;
	if ( *cache == NULL )
		*cache = plist_create( 8, 8);
	if (( p = *cache) == NULL)
		return false;
	list_add( p, ( Handle) base);
	list_add( p, ( Handle) entry);
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

/*
Fill the 2-int array meaning:

(-1,-1) - glyph doesn't intersect the descent line
(0,-1)  - glyph intersects the line wholly XXX
(A,B)   - glyph intersects the line from A to B

*/

static Bool
check_intersections( Point a, Point b, int y0, int y1, int *x_margins)
{
	int i;
	warn("? %d.%d - %d.%d x (%d %d)\n", a.x, a.y, b.x, b.y, y0, y1);
	if ( a.y > b.y ) {
		Point s = a;
		a = b;
		b = s;
	}

	if ( a.y > y1 || b.y < y0 ) {
		return false;
	} else if ( a.y >= y0 && b.y <= y1 ) {
		int min = ( a.x < b.x ) ? a.x : b.x;
		int max = ( a.x < b.x ) ? b.x : a.x;

		for ( i = 0; i < ((min == max) ? 1 : 2); i++) {
			int x = (i == 0) ? min : max;
			if ( x < 0 ) x = 0;
			if ( x_margins[1] >= 0 ) {
				if ( x_margins[0] < 0 || x_margins[0] > x)
					x_margins[0] = x;
				if ( x_margins[1] < x)
					x_margins[1] = x;
			} else if ( x_margins[0] < 0 )
				x_margins[0] = x_margins[1] = x;
		}
		warn("x> %d %d\n", x_margins[0], x_margins[1]);

		/* mark whole line */
		//x_margins[0] = 0;
		//x_margins[1] = -1;
		return false;
	}

	for ( i = 0; i < ((y0 == y1) ? 1 : 2); i++) {
		int y = (i == 0) ? y0 : y1;

		if ( a.y < y && b.y >= y ) {
			int x = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
			if ( x < 0 ) x = 0;
			if ( x_margins[1] >= 0 ) {
				if ( x_margins[0] < 0 || x_margins[0] > x)
					x_margins[0] = x;
				if ( x_margins[1] < x)
					x_margins[1] = x;
			} else if ( x_margins[0] < 0 )
				x_margins[0] = x_margins[1] = x;
		}
	}
	warn("y> %d %d\n", x_margins[0], x_margins[1]);
	return false;
}

int*
Drawable_call_get_glyph_descents( Handle self, unsigned int from, unsigned int to)
{
	dmARGS;
	int i, j, breakout, len = to - from + 1, extra_margin;
	int *descents, y0, y1;
	PFontABC abc;
	Bool want_dm_leave = false;

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_def( self, from, to, toGlyphs);
		if ( !abc) return NULL;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		dmCHECK(NULL);
		dmENTER(NULL);
		abc = apc_gp_get_font_def( self, from, to, toGlyphs);
		if ( !abc) {
			dmLEAVE;
			return NULL;
		}
		want_dm_leave = true;
	} else {
		SV * sv;
		if ( !( abc = malloc(len * sizeof( FontABC)))) return NULL;
		sv = my-> get_font_def( self, from, to, toGlyphs);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > len * 3) n = len * 3;
			n = ( n / 3) * 3;
			if ( n < len) memset( abc, 0, len * sizeof( FontABC));
			for ( i = 0; i < n; i += 3) {
				SV ** holder = av_fetch( av, i, 0);
				if ( holder) abc[j].a = ( float) SvNV( *holder);
				j++;
			}
		} else
			memset( abc, 0, len * sizeof( FontABC));
		sv_free( sv);
	}


	{
		int ut = (var->font.underlineThickness <= 0) ?
			1 :
			var->font.underlineThickness;
		int up = (var->font.underlinePosition < 0) ?
			var->font.underlinePosition :
			-(var->font.descent - ut/2 - 1);
		breakout = var->font.descent + up + ut;

		extra_margin = ( ut > 2 ) ? ut / 2 : 1;
		y0 = up - extra_margin;
		y1 = up + extra_margin + ut - 1;

		if ( y0 >= 0 ) y0 = -1;
		if ( y1 >= 0 ) y1 = -1;
		warn("d=%d up=%d ut=%d b=%d/ y=%d %d\n", var->font.descent, up, ut, breakout, y0, y1);
	}


	if (!( descents = malloc(sizeof(int) * len * 2))) {
		free(abc);
		if ( want_dm_leave ) dmLEAVE;
		return NULL;
	}
	for ( i = j = 0; i < len; i++, j += 2)
		descents[j] = descents[j + 1] = -1;

	/* query glyph outlines, check intersections with the underline */
	for ( i = j = 0; i < len; i++, j += 2) {
		warn("GLY %d a=%g\n", i, abc[i].a);
		if ( abc[i].a >= breakout )
			continue;

		if ( !self || my-> render_glyph == Drawable_render_glyph) {
			int k, count, *buffer = NULL, *p;
			Point anchor = {0,0}, curr_point = {0,0}, next_point;
			Bool close_shape = false, got_anchor = false, got_curr_point = false;
			count = apc_gp_get_glyph_outline( self, from + i, ggoGlyphIndex, &buffer);

			if ( count < 0 || !buffer) {
				descents[j] = 0;
				continue;
			}

#define POINT(x) ((x) / 64.0 + .5)
			for ( k = 0, p = buffer; k < count;) {
				int cmd      = *(p++);
				int n_subcmd = *(p++);
				int *p_sub, sub;
				k += n_subcmd * 2 + 2;
				warn("cmd %d\n", n_subcmd);
				switch ( cmd ) {
				case ggoMove:
					anchor.x    = POINT(p[0]);
					anchor.y    = POINT(p[1]);
					if ( got_curr_point && check_intersections( curr_point, anchor, y0, y1, descents + j))
						goto STOP;
					close_shape = false;
					got_anchor  = true;
					break;
				case ggoLine:
				case ggoConic:
				case ggoCubic:
					next_point.x = POINT(p[0]);
					next_point.y = POINT(p[1]);
					if ( got_curr_point )
						if ( check_intersections( curr_point, next_point, y0, y1, descents + j))
							goto STOP;
					for ( sub = 1, p_sub = p + 1; sub < n_subcmd; sub++) {
						curr_point   = next_point;
						next_point.x = POINT(*(p_sub++));
						next_point.y = POINT(*(p_sub++));
						if ( check_intersections( curr_point, next_point, y0, y1, descents + j))
							goto STOP;
					}
					break;
				default:
					p += n_subcmd * 2;
					continue;
				}

				p += n_subcmd * 2;
				curr_point.x   = POINT(p[-2]);
				curr_point.y   = POINT(p[-1]);
				got_curr_point = true;
				close_shape    = true;
			}
#undef POINT

			if ( close_shape && got_anchor )
				check_intersections( curr_point, anchor, y0, y1, descents + j);

		STOP:
			warn("%d => %d %d\n", i, descents[j], descents[j+1]);
			if ( buffer )
				free(buffer);
		} else {
			croak("unimplemented");
		}
	}

	free(abc);

	/* if not the whole glyph, extend the X parts by half of the line thickness */
	for ( i = j = 0; i < len; i++, j += 2) {
		if ( descents[j] >= 0 && descents[j+1] >= 0) {
			descents[j]   -= extra_margin;
			if ( descents[j] < 0 )
				descents[j] = 0;

			descents[j+1] += extra_margin;
		}
	}

	if ( want_dm_leave ) dmLEAVE;
	return descents;
}

static PFontABC
query_abc_range( Handle self, TextWrapRec * t, unsigned int base)
{
	PFontABC abc;

	if ( t-> utf8_text) {
		if (
			t->unicode &&
			(( abc = (PFontABC) find_in_cache( *(t->unicode), base)) != NULL)
		)
			return abc;
	} else if (*( t-> ascii))
		return *(t-> ascii);

	if ( !( abc = Drawable_call_get_font_abc( self, base * 256, base * 256 + 255, t-> utf8_text ? toUTF8 : 0)))
		return NULL;

	if ( t-> utf8_text) {
		if ( !fill_cache(t->unicode, base, abc)) {
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

#define FIDGID2BASE(fid,gid) (((fid) << 8) + ((gid) >> 8))
#define GID2OFFSET(gid)      ((gid) & 0xff)
#define BASE_FROM(base)      ((base & 0xff) * 256)
#define BASE_TO(base)        (BASE_FROM(base) + 255)

static PFontABC
query_abc_glyph_range( Handle self, int base)
{
	PFontABC abcs = NULL;

	abcs = (PFontABC) find_in_cache( var->font_abc_glyphs, base);
	if ( abcs == NULL) {
		abcs = Drawable_call_get_font_abc( self, BASE_FROM(base), BASE_TO(base), toGlyphs);
		if ( abcs == NULL)
			return NULL;
		if ( !fill_cache(&(var->font_abc_glyphs), base, abcs)) {
			free(abcs);
			return NULL;
		}
	}

	return abcs;
}

static int*
query_descent_range( Handle self, int base)
{
	int* ret;

	ret = (int*) find_in_cache( var->glyph_descents, base);
	if ( ret == NULL) {
		ret = Drawable_call_get_glyph_descents( self, BASE_FROM(base), BASE_TO(base));
		if ( ret == NULL)
			return NULL;
		if ( !fill_cache(&(var->glyph_descents), base, ret)) {
			free(ret);
			return NULL;
		}
	}

	return ret;
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
	uint16_t fid;
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
wrap_load_glyphs_abc(Handle self, uint16_t fid, uint16_t gid, WrapRec * w, GlyphWrapRec *g)
{
	PFontABC labc;
	int base = FIDGID2BASE(fid,gid);
	if ( base == w-> base)
		return true;
	my->switch_font( self, &g->savefont, fid);
	w-> base = base;
	if ( !(labc = query_abc_glyph_range( self, w->base)))
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

#define LOAD_ABC(fid,gid) \
	if ( !wrap_load_glyphs_abc(self, fid, gid, &wr, gw)) \
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
		uint32_t uv, uv0, last_uv = 0, last_fid = 0;
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
				if ( gw->fonts ) last_fid = gw->fonts[index + j];
				uv = gw->glyphs[index + j];
				if ( j == 0 ) uv0 = uv;
				if (!gw-> advances || reassign_w) /* do not query ABC unnecessarily if advances are there */
					LOAD_ABC(last_fid, last_uv);
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
			LOAD_ABC(last_fid, last_uv);
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
	my->save_font(self, &t.savefont);
	c = my->do_text_wrap( self, NULL, &t, NULL);
	my->restore_font(self, &t.savefont);
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

	my->save_font(self, &gw.savefont);
	c = my->do_text_wrap( self, &tw, &gw, log2vis + from);
	my->restore_font(self, &gw.savefont);
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

static NPoint*
render_underline(Handle self, int x, int y, GlyphsOutRec *t, int * n_points)
{
	dmARGS;
	int x0, y0, i, base;
	FontABC *abc = NULL, last_abc = {0,0,0};
	int *descents = NULL, ut;
	Bool ok = true;
	Bool line_is_on;
	NPoint c, *ret;
	float half_widths[2];
	SaveFont savefont;

	x0 = x;
	y0 = y;
	x = y = 0;

	if ( var-> font. direction != 0)
		c = my->trig_cache(self);
	else
		c.x = c.y = 0;

	{
		Bool text_out_baseline;
		ut = (var->font.underlineThickness <= 0) ?
			1 :
			var->font.underlineThickness;
		int up = (var->font.underlinePosition < 0) ?
			-var->font.underlinePosition :
			(var->font.descent - 1);
		y -= up + ((ut > 1) ? (ut / 2) : 0);

		text_out_baseline = ( my-> textOutBaseline == Drawable_textOutBaseline) ?
			apc_gp_get_text_out_baseline(self) :
			my-> get_textOutBaseline(self);
		if ( !text_out_baseline )
			y += var->font.descent;

		if ( ut > 1 ) {
			for (i = 0; i < 2; i++) {
				NRect box;
				int j;
				j = ( i == 0 ) ? leiArrowTail : leiArrowHead;
				j   = Drawable_resolve_line_end_index(&var->current_state, j);
				box = Drawable_line_end_box( &var->current_state, j);
				half_widths[i] = box.right * ut;
			}

			half_widths[1] *= -1.0;
		} else
			half_widths[0] = half_widths[1] = 0.0;
	}

	*n_points = 0;
	dmCHECK(NULL);
	dmENTER(NULL);
	if (( ret = malloc(sizeof(NPoint) * 6 * t->len)) == NULL) {
		dmLEAVE;
		return NULL;
	}

	line_is_on = false;
	base = -1;

#define ADD_POINT(hw) {                                        \
	float xx = x - hw;                                     \
	float this_a = ( abc[o].a < 0 ) ? -abc[o].a   : 0;     \
	float last_c = (last_abc.c < 0) ? -last_abc.c : 0;     \
	warn("x %g/%d a %g c %g hw %g = %g\n", xx, hw, this_a, last_c, half_widths[hw], half_widths[hw]+xx);\
	xx -= this_a + last_c;                                 \
	xx += half_widths[hw];                                 \
	ret[ *n_points    ].x = xx;                            \
	ret[ (*n_points)++].y = y;                             \
}
#define LAST_X ret[(*n_points) - 1].x
#define CHECK_POINT                                            \
	if ( *n_points > 1 && LAST_X - ret[(*n_points)-2].x <= ut ) { \
		warn("skipsie!\n");\
		*n_points -= 2;\
	}

	if ( t-> fonts )
		my->save_font(self, &savefont);
	for ( i = 0; i < t->len; i++) {
		int o = GID2OFFSET(t->glyphs[i]), from, to;
		int b = FIDGID2BASE(t->fonts ? t->fonts[i] : 0, t->glyphs[i]);
		if ( base != b) {
			base = b;
			if ( t->fonts )
				my-> switch_font(self, &savefont, t->fonts[i]);
			abc      = query_abc_glyph_range( self, base);
			descents = query_descent_range  ( self, base);
			if (abc == NULL || descents == NULL) {
				ok = false;
				break;
			}
		}

		from = descents[o * 2];
		to   = descents[o * 2 + 1];
		warn("C %d (%g %g %g = %g) => %d %d\n", t->glyphs[i], abc[o].a, abc[o].b, abc[o].c, abc[o].a+abc[o].b+abc[o].c, from, to);

		if ( from < 0 ) {
			if ( !line_is_on ) {
				/* start the line */
				ADD_POINT(0)
				CHECK_POINT;
				line_is_on = true;
			}
			/* else continue the existing line */
		} else if ( to < 0 ) {
			if ( line_is_on ) {
				/* stop the line */
				ADD_POINT(1)
				CHECK_POINT;
				line_is_on = false;
			}
			/* else continue skipping line */
		} else if ( from == 0 ) {
			/* line wanted after a skip */
			if ( line_is_on ) {
				ADD_POINT(1)
				CHECK_POINT;
				warn("a0: %g\n", LAST_X);
			} else
				line_is_on = true;
			ADD_POINT(0)
			LAST_X += to + 1;
			warn("b: %g\n", LAST_X);
			CHECK_POINT;
		} else if ( to + 1 >= abc[o].a + abc[o].b + abc[o].c ) {
			/* line wanted but not until the end */
			if ( line_is_on ) {
				line_is_on = false;
			} else {
				ADD_POINT(0)
				warn("d0: %g\n", LAST_X);
				CHECK_POINT;
			}
			ADD_POINT(1)
			LAST_X += from - 1;
			CHECK_POINT;
			warn("d1: %g\n", LAST_X);
		} else {
			if ( !line_is_on ) {
				ADD_POINT(0)
				warn("f0: %g\n", LAST_X);
				CHECK_POINT;
				line_is_on = true;
			}

			ADD_POINT(1);
			LAST_X += from - 1;
			warn("f1: %g\n", LAST_X);
			CHECK_POINT;

			ADD_POINT(0)
			LAST_X += to + 1;
			warn("f2: %g\n", LAST_X);
			CHECK_POINT;
		}

		x += abc[o].a + abc[o].b + abc[o].c;
		last_abc = abc[o];
	}
	if ( t-> fonts )
		my->restore_font(self, &savefont);
	dmLEAVE;

	if ( !ok ) {
		free(ret);
		ret = NULL;
		*n_points = 0;
	}

	if ((( *n_points ) % 2) == 1 ) {
		int o = t->glyphs[t->len - 1] & 0xff;
		warn("FINO\n");
		ADD_POINT(1)
		CHECK_POINT;
	}

#undef ADD_POINT
#undef LAST_X
#undef CHECK_POINT

	{
		NPoint *p = ret;
		for ( i = 0; i < *n_points; i++, p += 2)
			if ( p[0].x > p[1].x ) {
				NPoint s = *p;
				*p = p[1];
				p[1] = s;
			}
	}

	if ( var-> font. direction != 0) {
		NPoint *p = ret;
		for ( i = 0; i < *n_points; i++, p++) {
			NPoint n = *p;
			p->x = n.x * c.y - n.y * c.x + x0;
			p->y = n.x * c.x + n.y * c.y + y0;
		}
	} else {
		NPoint *p = ret;
		for ( i = 0; i < *n_points; i++, p++) {
			p-> x += x0;
			p-> y += y0;
		}
	}

	return ret;
}

SV *
Drawable_render_underline(Handle self, SV * glyphs, int x, int y)
{
	AV * av;
	NPoint *np, *npi;
	int i, n_points;
	GlyphsOutRec t;
	if (!Drawable_read_glyphs(&t, glyphs, 0, "Drawable::render_underline"))
		return NULL_SV;
	np = render_underline(self, x, y, &t, &n_points);
	av = newAV();
	for ( i = 0, npi = np; i < n_points; i++, npi++) {
		if ( var-> antialias ) {
			av_push(av, newSVnv(npi->x));
			av_push(av, newSVnv(npi->y));
		} else {
			int z;
			z = (npi->x > 0) ? (npi->x + .5) : (npi->x - 0.5);
			av_push(av, newSViv(z));
			z = (npi->y > 0) ? (npi->y + .5) : (npi->y - 0.5);
			av_push(av, newSViv(z));
		}
	}
	free(np);
	return newRV_noinc(( SV *) av);
}

#ifdef __cplusplus
}
#endif
