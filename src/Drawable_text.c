#include "apricot.h"
#include "Drawable.h"

#ifdef __cplusplus
extern "C" {
#endif


#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)
#define var (( PDrawable) self)

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER(fail)     if ( !inPaint) if ( !my-> begin_paint_info( self)) return (fail)
#define gpLEAVE           if ( !inPaint) my-> end_paint_info( self)

Bool
Drawable_text_out( Handle self, SV * text, int x, int y)
{
	Bool ok;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
		ok = apc_gp_text_out( self, c_text, x, y, dlen, utf8 ? toUTF8 : 0);
		if ( !ok) perl_error();
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		int n_glyphs;
		void * glyphs;
		Bool do_free;
	
		if ( !( glyphs = prima_read_array(
			text, "Drawable::text_out",
			's', 1, 1, -1, &n_glyphs, &do_free)))
			return false;

		ok = apc_gp_text_out( self, (char*)glyphs, x, y, n_glyphs, toGlyphs);
		if ( !ok) perl_error();
		if ( do_free ) free(glyphs);
	} else {
		SV * ret = sv_call_perl(text, "text_out", "<Hii", self, x, y);
		ok = ret && SvTRUE(ret);
	}
	return ok;
}

SV*
Drawable_text_shape( Handle self, SV * text, HV * profile)
{
	dPROFILE;
	gpARGS;
	SV * ret = nilSV;
	STRLEN dlen;
	int size, new_size, flags = 0;
	uint16_t * glyphs;
	int16_t * char_offsets;
	Bool with_offsets;
	char * c_text;
	SV * sv_glyphs, *sv_offsets;

	if ( SvROK(text)) {
		SV * ref = newRV_noinc((SV*) profile);
		gpENTER(nilSV);
		ret = sv_call_perl(text, "text_shape", "<HSS", self, text, ref);
		gpLEAVE;
		sv_free(ref);
		return ret;
	}

	gpENTER(nilSV);

	c_text = SvPV( text, dlen);
	if (prima_is_utf8_sv( text)) {
		flags |= toUTF8;
		dlen = prima_utf8_length(c_text);
	}

	with_offsets = pexist(map) ? pget_B(map) : false;
	if ( pexist(rtl)      && pget_B(rtl))      flags |= toRTL;
	if ( pexist(override) && pget_B(override)) flags |= toOverride;

	/* MSDN, on ScriptShape: A reasonable value is (1.5 * cChars + 16) */
	size = dlen * 2 + 16;
	sv_glyphs = prima_array_new(size * 2);
	glyphs = (uint16_t*) prima_array_get_storage(sv_glyphs);
	if ( with_offsets ) {
		sv_offsets = prima_array_new(size * 2);
		char_offsets = (int16_t*) prima_array_get_storage(sv_offsets);
	} else {
		char_offsets = NULL;
		sv_offsets = NULL;
	}

	new_size = apc_gp_text_shape(self,
		pexist(language) ? pget_c(language) : NULL,
		c_text, dlen, flags,
		size, glyphs, char_offsets);
	gpLEAVE;

	if ( new_size < 0 ) {
		sv_free(sv_glyphs);
		if ( sv_offsets ) sv_free(sv_offsets);
		return (new_size == -2) ? newSViv(0) : nilSV;
	}

	if (new_size < size ) {
		prima_array_truncate( sv_glyphs, new_size * 2);
		if ( sv_offsets ) prima_array_truncate( sv_offsets, new_size * 2);
	}

	sv_glyphs = prima_array_tie( sv_glyphs, 2, "s" );
	if ( !with_offsets ) return sv_glyphs;

	sv_offsets = prima_array_tie( sv_offsets, 2, "s");
	ret = newSVsv(call_perl(self, "new_glyph_obj", "<SS", sv_glyphs, sv_offsets));

	sv_free(sv_glyphs);
	sv_free(sv_offsets);

	return ret;
}

int
Drawable_get_text_width( Handle self, SV * text, int flags)
{
	gpARGS;
	int res;

	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		} else
			flags &= ~toUTF8;
		gpENTER(0);
		res = apc_gp_get_text_width( self, c_text, dlen, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		int n_glyphs;
		void * glyphs;
		Bool do_free;

		gpENTER(0);
		if ( !( glyphs = prima_read_array(
			text, "Drawable::get_text_width",
			's', 1, 1, -1, &n_glyphs, &do_free))) {
			gpLEAVE;
			return false;
		}

		flags &= ~toUTF8;
		res = apc_gp_get_text_width( self, (char*) glyphs, n_glyphs, flags | toGlyphs );
		gpLEAVE;
		if ( do_free ) free(glyphs);
	} else {
		SV * ret;
		gpENTER(0);
		ret = sv_call_perl(text, "get_text_width", "<Hi", self, flags);
		gpLEAVE;
		res = (ret && SvOK(ret)) ? SvIV(ret) : 0;
	}

	return res;
}

SV *
Drawable_get_text_box( Handle self, SV * text, int flags )
{
	gpARGS;
	Point * p;
	AV * av;
	int i;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);

		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		} else
			flags &= ~toUTF8;

		gpENTER( newRV_noinc(( SV *) newAV()));
		p = apc_gp_get_text_box( self, c_text, dlen, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		int n_glyphs;
		void * glyphs;
		Bool do_free;
	
		gpENTER( newRV_noinc(( SV *) newAV()));
		if ( !( glyphs = prima_read_array(
			text, "Drawable::get_text_box",
			's', 1, 1, -1, &n_glyphs, &do_free))) {
			gpLEAVE;
			return false;
		}

		flags &= ~toUTF8;
		p = apc_gp_get_text_box( self, (char*) glyphs, n_glyphs, flags | toGlyphs );
		gpLEAVE;

		if ( do_free ) free(glyphs);
	} else {
		SV * ret;
		gpENTER( newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "get_text_box", "<H", self));
		gpLEAVE;
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

static PFontABC
call_get_font_abc( Handle self, unsigned int base, int flags)
{
	PFontABC abc;

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, flags);
		if ( !abc) return NULL;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		gpARGS;
		gpENTER(NULL);
		abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, flags);
		gpLEAVE;
		if ( !abc) return NULL;
	} else {
		SV * sv;
		if ( !( abc = malloc( 256 * sizeof( FontABC)))) return NULL;
		sv = my-> get_font_abc( self, base * 256, base * 256 + 255, flags);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > 256 * 3) n = 256 * 3;
			n = ( n / 3) * 3;
			if ( n < 256) memset( abc, 0, 256 * sizeof( FontABC));
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
			memset( abc, 0, 256 * sizeof( FontABC));
		sv_free( sv);
	}

	return abc;
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

	if ( !( abc = call_get_font_abc(self, base, t-> utf8_text ? toUTF8 : 0)))
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
	if ( !dest) return false;
	for ( i = 0; i < 256; i++) {
		width[i] = src[i]. a + src[i]. b + src[i]. c;
		dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
		dest[i]. b = src[i]. b;
		dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
	}
	return true;
}

static Bool
add_wrapped_text(
	TextWrapRec * t, 
	int start, int utfstart, int end, int utfend,
	int tildeIndex, int * tildePos, int * tildeLPos, int * tildeLine,
	char *** array, int * size
) {
	int l = end - start;
	char *c = NULL;

	if (!( t-> options & twReturnChunks)) {
		if ( !( c = allocs( l + 1)))
			return false;
		memcpy( c, t-> text + start, l);
		c[l] = 0;
	}

	if ( tildeIndex >= 0 && tildeIndex >= start && tildeIndex < end) {
		*tildeLine = t-> t_line = t-> count;
		*tildePos = *tildeLPos = tildeIndex - start;
		if ( tildeIndex == end - 1) {
			t-> t_line++;
			tildeLPos = 0;
		}
	}

	if ( t-> count == *size) {
		char ** n;
		if ( !( n = (char **) realloc( *array, *size *= 2)))
			return false;
		*array = n;
	}

	if ( t-> options & twReturnChunks) {
		(*array)[ t-> count++] = INT2PTR(char*,utfstart);
		(*array)[ t-> count++] = INT2PTR(char*,utfend - utfstart);
	} else
		(*array)[ t-> count++] = c;

	return true;
}

static int
find_tilde_position( TextWrapRec * t )
{	
	int i, tildeIndex = -100;

	for ( i = 0; i < t-> textLen - 1; i++)
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

	return tildeIndex;
}

static void
fill_tilde_properties(Handle self, TextWrapRec * t, int tildePos, int tildeOffset)
{
	UV uv;
	int base;
	PFontABC abcc;
	float start, end;

	t-> t_char = t-> text + tildePos + 1;
	if ( t-> utf8_text) {
		STRLEN len;
		prima_utf8_uvchr(t-> t_char, t-> textLen - tildePos - 1, &len);
		if ( len == 0 ) return;
	} else
		uv = *(t->t_char);

	abcc = query_abc_range( self, t, base = uv / 256) + (uv & 0xff);
	start = tildeOffset;
	end   = start + abcc-> b - 1.0;
	if ( abcc-> a < 0.0 ) {
		start += abcc->a;
		end += abcc->a;
	}
	t-> t_start = start + .5 * (( start < 0 ) ? -1 : 1);
	t-> t_end   = end   + .5 * (( end   < 0 ) ? -1 : 1);
}

static Bool
expand_tabs( TextWrapRec * t, char ** strings)
{
	int i;

	for ( i = 0; i < t-> count; i++) {
		int tabs = 0, len = 0;
		char *substr = strings[ i], *n;

		while (*substr) {
			if ( *substr == '\t') tabs++;
			substr++;
			len++;
		}
		if ( tabs == 0) continue;
	
		if ( !( n = allocs( len + tabs * t-> tabIndent + 1)))
			return false;

		len = 0;
		substr = strings[ i];
		while ( *substr) {
			if ( *substr == '\t') {
				int j = t-> tabIndent;
				while ( j--) n[ len++] = ' ';
			} else
				n[ len++] = *substr;
			substr++;
		}

		free( strings[ i]);
		n[len] = 0;
		strings[i] = n;
	}

	return true;
}

char **
Drawable_do_text_wrap( Handle self, TextWrapRec * t)
{
	char **ret = NULL;
	int bufsize = 128;

	float width[256];
	FontABC abc[256];
	unsigned int base = 0x10000000;

	int
		start = 0, utf_start = 0,
		split_start = -1, split_end = -1, 
		i, utf_p,
		utf_split = -1;
	float w = 0, inc = 0, initial_overhang = 0;
	Bool wasTab = 0, reassign_w = 1;
	Bool doWidthBreak = t-> width >= 0;
	int tildeIndex = -100, tildeLPos = 0, tildeLine = 0, tildePos = 0, tildeOffset = 0;
	int spaceWidth = 0, spaceC = 0, spaceOK = 0;

#define ADD(end, utfend)                                          \
	if ( !add_wrapped_text( t,                                \
		start, utf_start, end, utfend,                    \
		tildeIndex,                                       \
		&tildePos, &tildeLPos, &tildeLine,                \
		&ret, &bufsize))                                    \
			return ret;                               \
	start     = end;                                          \
	utf_start = utfend;                                       \
	if ( t-> options & twReturnFirstLineLength) return ret;

#define ADVANCE               \
	split_start = p;      \
	split_end   = i;      \
	utf_split   = utf_p;

#define NEW_LINE              \
	start = i;            \
	utf_start++;          \
	reassign_w = 1

	t-> count = 0;
	if (!( ret = allocn( char*, bufsize))) return NULL;

	/* determining ~ character location */
	if ( t-> options & twCalcMnemonic)
		tildeIndex = find_tilde_position(t);

	/* process UV chars */
	for ( i = 0, utf_p = 0; i < t-> textLen; utf_p++) {
		UV uv;
		float winc;
		int p = i;

		if ( t-> utf8_text) {
			STRLEN len;
			uv = prima_utf8_uvchr(t->text + i, t-> textLen, &len);
			i += len;
			if ( len == 0 )
				break;
		} else
			uv = (( unsigned char *)(t-> text))[i++];

		if ( uv / 256 != base)
			if ( !precalc_abc_buffer( query_abc_range( self, t, base = uv / 256), width, abc))
				return ret;

		if ( reassign_w) {
			w = initial_overhang = abc[ uv & 0xff]. a;
			reassign_w = 0;
		}

		switch ( uv ) {
		case '\t':
			ADVANCE;	
			if (!( t-> options & twCalcTabs))
				goto _default;

			if ( t-> options & twSpaceBreak) {
				ADD( p, utf_p);
				NEW_LINE;
				continue;
			}

			if ( !spaceOK) {
				PFontABC s;
				if ( !( s = query_abc_range( self, t, 0))) return ret;
				spaceOK    = 1;
				spaceC     = (s[' '].c < 0) ? - s[' ']. c : 0;
				spaceWidth = (s[' '].a + s[' '].b + s[' '].c) * t-> tabIndent;
			}
			winc   = spaceWidth;
			inc    = spaceC;
			wasTab = true;
			break;

		case '\n':
		case 0x2028:
		case 0x2029:
			ADVANCE;	
			if (!( t-> options & twNewLineBreak))
				goto _default;
			ADD( p, utf_p);
			NEW_LINE;
			continue;

		case ' ':
			ADVANCE;	
			if (!( t-> options & twSpaceBreak))
				goto _default;
			ADD( p, utf_p);
			NEW_LINE;
			continue;

		case '~':
			if ( p == tildeIndex ) {
				tildeOffset = w - initial_overhang;
				inc = winc = 0;
				break;
			}

		_default: default:
			winc = width[uv & 0xff];
			inc  = abc[uv & 0xff].c;
		}

		/* add next char */
		if ( !doWidthBreak || w + winc + inc <= t-> width) {
			w += winc;
			continue;
		}

		if (
			( p == start) || 
			(( p == start - 1) && ( p - 1 == tildeIndex))
		) {
			/* case when even single char cannot be fit in  */
			if ( t-> options & twBreakSingle) {
				/* do not return anything in this case */
				int j;
				if (!( t-> options & twReturnChunks)) {
					for ( j = 0; j < t-> count; j++) free( ret[ j]);
					ret[0] = duplicate_string("");
				}
				t-> count = 0;
				return ret;
			}

			/* or push this character disregarding the width */
			ADD(i, utf_p + 1);
		} else {
			/* normal break condition */
			if ( t-> options & twWordBreak) {
				/* checking if break was at word boundary */
				if ( start <= split_start) {
					ADD(split_start, utf_split);
					i = start = split_end;
					utf_start = utf_split + 1;
					utf_p = utf_split;
					w = 0;
					continue;
				} else if ( t-> options & twBreakSingle) {
					/* cannot be split, return nothing */
					int j;
					if (!( t-> options & twReturnChunks)) {
						for ( j = 0; j < t-> count; j++)
							free( ret[ j]);
						ret[0] = duplicate_string("");
					}
					t-> count = 0;
					return ret;
				}
			}

			/* repeat again */
			ADD(p, utf_p);
			i = start = p;
			utf_start = utf_p;
			utf_p--;
		}
		w = 0;
	}

	/* adding or skipping last line */
	if ( t-> textLen - start > 0 || t-> count == 0)
		ADD(t-> textLen, t-> utf8_textLen);

	/* remove ~ and fill its location */
	t-> t_start = t-> t_end = t-> t_line = C_NUMERIC_UNDEF;
	if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
		if ( t-> options & twCollapseTilde) {
			char * l = ret[tildeLine];
			memmove( l + tildePos, l + tildePos + 1, strlen(l) - tildePos);
		}
		fill_tilde_properties(self, t, tildePos, tildeOffset);
	}

	if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab)
		expand_tabs( t, ret);

	return ret;
}
#undef ADD

static SV*
string_wrap( Handle self,SV * text, int width, int options, int tabIndent)
{
	gpARGS;
	TextWrapRec t;
	char** c;
	int i;
	AV * av;
	STRLEN tlen;

	t. text      = SvPV( text, tlen);
	t. utf8_text = prima_is_utf8_sv( text);
	if ( t. utf8_text) {
		t. utf8_textLen = prima_utf8_length( t. text);
		t. textLen = utf8_hop(( U8*) t. text, t. utf8_textLen) - (U8*) t. text;
	} else {
		t. utf8_textLen = t. textLen = tlen;
	}
	t. width     = ( width < 0) ? 0 : width;
	t. tabIndent = ( tabIndent < 0) ? 0 : tabIndent;
	t. options   = options;
	t. ascii     = &var-> font_abc_ascii;
	t. unicode   = &var-> font_abc_unicode;
	t. t_char    = NULL;

	gpENTER( (options & twReturnFirstLineLength) ? newSViv(0) : newRV_noinc(( SV *) newAV()));
	c = Drawable_do_text_wrap( self, &t);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		IV rlen = 0;
		if ( c) {
			if ( t. count > 0) rlen = PTR2IV(c[1]);
			free( c);
		}
		return newSViv( rlen);
	}

	if ( !c) return nilSV;

	av = newAV();
	for ( i = 0; i < t. count; i++) {
		SV * sv = (t. options & twReturnChunks) ?
			newSViv( PTR2IV(c[i])) :
			newSVpv( c[ i], 0);
		if ( !(t. options & twReturnChunks)) {
			if ( t. utf8_text) SvUTF8_on( sv);
			free( c[i]);
		}
		av_push( av, sv);
	}

	free( c);

	if  ( t. options & ( twCalcMnemonic | twCollapseTilde)) {
		HV * profile = newHV();
		SV * sv_char;
		if ( t. t_char) {
			STRLEN len = t. utf8_text ? utf8_hop(( U8*) t. t_char, 1) - ( U8*) t. t_char : 1;
			sv_char = newSVpv( t. t_char, len);
			if ( t. utf8_text) SvUTF8_on( sv_char);
			if ( t. t_start != C_NUMERIC_UNDEF) pset_i( tildeStart, t. t_start);
			if ( t. t_end   != C_NUMERIC_UNDEF) pset_i( tildeEnd,   t. t_end);
			if ( t. t_line  != C_NUMERIC_UNDEF) pset_i( tildeLine,  t. t_line);
		} else {
			sv_char = newSVsv( nilSV);
			pset_sv( tildeStart, nilSV);
			pset_sv( tildeEnd,   nilSV);
			pset_sv( tildeLine,  nilSV);
		}
		pset_sv_noinc( tildeChar, sv_char);
		av_push( av, newRV_noinc(( SV *) profile));
	}

	return newRV_noinc(( SV *) av);
}

typedef struct {
	uint16_t * glyphs;   /* glyphset to be wrapped */
	int        n_glyphs; /* glyphset length in words */
	int        width;    /* width to wrap with */
	int        options;  /* twXXX constants */
	int        count;    /* count of lines returned */
	PList    * cache;
} GlyphWrapRec;

static Bool
add_wrapped_glyphs( GlyphWrapRec * t, unsigned int start, unsigned int end, unsigned int ** array, int * size)
{
	if ( t-> count == *size) {
		unsigned int * n;
		if ( !( n = (unsigned int *)realloc( *array, sizeof(unsigned int) * (*size *= 2))))
			return false;
		*array = n;
	}

	(*array)[t-> count++] = start;
	(*array)[t-> count++] = end - start;

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

	if ( !( abc = call_get_font_abc(self, base, toGlyphs)))
		return NULL;

	if ( !fill_abc_list_cache(t->cache, base, abc)) {
		free( abc);
		return NULL;
	}

	return abc;
}

static unsigned int *
Drawable_do_glyphs_wrap( Handle self, GlyphWrapRec * t)
{
	unsigned int *ret = NULL;
	int size = 128;

	float width[256];
	FontABC abc[256];
	unsigned int base = 0x10000000;

	unsigned int start = 0, i;
	float w = 0, inc = 0, initial_overhang = 0;
	Bool reassign_w = 1;
	Bool doWidthBreak = t-> width >= 0;

#define ADD(end)                                                 \
	if ( !add_wrapped_glyphs( t, start, end, &ret, &size))   \
		return ret;                                      \
	start = end;                                             \
	if (t-> options & twReturnFirstLineLength)               \
		return ret;

	t-> count = 0;
	if (!( ret = allocn( unsigned int, size))) return NULL;

	/* process glyphs */
	for ( i = 0; i < t-> n_glyphs; i++) {
		uint16_t uv;
		float winc;
		int p = i;

		uv = t-> glyphs[i];
		if ( uv / 256 != base)
			if ( !precalc_abc_buffer( query_abc_range_glyphs( self, t, base = uv / 256), width, abc))
				return ret;
		if ( reassign_w) {
			w = initial_overhang = abc[ uv & 0xff]. a;
			reassign_w = 0;
		}
		winc = width[ uv & 0xff];
		inc  = abc[ uv & 0xff]. c;

		if ( doWidthBreak && w + winc + inc > t-> width) {
			w += winc;
			continue;
		}

		if (( p == start) || ( p == start - 1)) {
			/* case when even single char cannot be fit in  */
			if ( t-> options & twBreakSingle) {
				/* do not return anything in this case */
				t-> count = 0;
				return ret;
			}
			/* or push this character disregarding the width */
			ADD(i);
		} else { /* normal break condition */
			ADD(p);
			i = start = p;
		}

		w = 0;
	}

	/* adding or skipping last line */
	if ( t-> n_glyphs - start > 0 || t-> count == 0)
		ADD( t-> n_glyphs);

	return ret;
}
#undef ADD

static SV*
glyphs_wrap( Handle self, SV * text, int width, int options)
{
	gpARGS;
	GlyphWrapRec t;
	AV * av;
	int i;
	unsigned int * c;
	Bool do_free;

	t. width     = ( width < 0) ? 0 : width;
	t. options   = options;
	t. cache     = &var-> font_abc_glyphs;

	gpENTER((options & twReturnFirstLineLength) ? newSViv(0) : newRV_noinc(( SV *) newAV()));

	if ( !( t.glyphs = prima_read_array(
		text, "Drawable::text_wrap",
		's', 1, 1, -1, &t.n_glyphs, &do_free))) {
		gpLEAVE;
		return nilSV;
	}

	c = Drawable_do_glyphs_wrap( self, &t);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		IV rlen = 0;
		if ( c) {
			if ( t. count > 0) rlen = c[1];
			free( c);
		}
		if ( do_free ) free( t. glyphs );
		return newSViv( rlen);
	}

	if ( !c) {
		if ( do_free ) free( t. glyphs );
		return nilSV;
	}

	av = newAV();
	if ( options & twReturnChunks) {
		for ( i = 0; i < t.count; i++)
			av_push( av, newSViv( c[i]));
	} else {
		for ( i = 0; i < t. count; i += 2) {
			SV * sv;
			unsigned int offset = c[i], size = c[i + 1] * 2;
			sv = prima_array_new(size);
			memcpy( prima_array_get_storage(sv), t.glyphs + offset, size);
			av_push( av, prima_array_tie( sv, 2, "s"));
		}
	}
	free( c);

	if ( do_free ) free( t. glyphs );
	return newRV_noinc(( SV *) av);
}

SV*
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent)
{

	if ( !SvROK( text )) {
		return string_wrap(self, text, width, options, tabIndent);
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		return glyphs_wrap(self, text, width, options);
	} else {
		SV * ret;
		gpARGS;
		gpENTER((options & twReturnFirstLineLength) ? newSViv(0) : newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "text_wrap", "<Hiii", self, width, options, tabIndent));
		gpLEAVE;
		return ret;
	}
}

#ifdef __cplusplus
}
#endif
