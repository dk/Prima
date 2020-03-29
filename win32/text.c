#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include <usp10.h>
#include "guts.h"
#include "win32\win32guts.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

#define GRAD 57.29577951

Bool
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, int flags )
{objCheck false;{
	Bool ok = true;
	HDC ps = sys ps;
	int bk  = GetBkMode( ps);
	int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
	Bool use_path;

	if ( flags & toGlyphs ) {
		if ( len > 8192 )
			len = 8192;
		flags &= ~toUTF8;
	} else {
		int div = 32768L / (var font. maximalWidth ? var font. maximalWidth : 1);
		if ( div <= 0) div = 1;
		/* Win32 has problems with text_out strings that are wider than
		32K pixel - it doesn't plot the string at all. This hack is
		although ugly, but is better that Win32 default behaviour, and
		at least can be excused by the fact that all GP spaces have
		their geometrical limits. */
		if ( len > div) len = div;

		if ( flags & toUTF8 ) {
			int mb_len;
			if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) return false;
			len = mb_len;
		}
	}

	use_path = GetROP2( sys ps) != R2_COPYPEN;
	if ( use_path ) {
		STYLUS_USE_BRUSH( ps);
		BeginPath(ps);
	} else {
		STYLUS_USE_TEXT( ps);
		if ( opa != bk) SetBkMode( ps, opa);
	}

	if ( flags & toGlyphs ) {
		ok = ExtTextOutW(ps, x, sys lastSize. y - y, ETO_GLYPH_INDEX, NULL, (LPCWSTR) text, len, NULL);
	} else {
		ok = ( flags & toUTF8 ) ?
			TextOutW( ps, x, sys lastSize. y - y, ( U16*)text, len) :
			TextOutA( ps, x, sys lastSize. y - y, text, len);
	}
	if ( !ok ) apiErr;

	if ( use_path ) {
		EndPath(ps);
		FillPath(ps);
	} else {
		if ( opa != bk) SetBkMode( ps, bk);
	}

	if ( flags & toUTF8 ) free(( char *) text);
	return ok;
}}


/* Shaping code was borrowed from pangowin32-shape.c by Red Hat Software and Alexander Larsson */

static WORD
make_langid(const char *lang)
{
#define CASE(t,p,s) if (strcmp(lang, t) == 0) return MAKELANGID (LANG_##p, SUBLANG_##p##_##s)
#define CASEN(t,p) if (strcmp(lang, t) == 0) return MAKELANGID (LANG_##p, SUBLANG_NEUTRAL)

/* Languages that most probably don't affect Uniscribe have been
* left out. Uniscribe is documented to use
* SCRIPT_CONTROL::uDefaultLanguage only to select digit shapes, so
* just leave languages with own digits.
*/

	CASEN ("ar", ARABIC);
	CASEN ("hy", ARMENIAN);
	CASEN ("as", ASSAMESE);
	CASEN ("az", AZERI);
	CASEN ("bn", BENGALI);
	CASE ("zh-tw", CHINESE, TRADITIONAL);
	CASE ("zh-cn", CHINESE, SIMPLIFIED);
	CASE ("zh-hk", CHINESE, HONGKONG);
	CASE ("zh-sg", CHINESE, SINGAPORE);
	CASE ("zh-mo", CHINESE, MACAU);
	CASEN ("dib", DIVEHI);
	CASEN ("fa", FARSI);
	CASEN ("ka", GEORGIAN);
	CASEN ("gu", GUJARATI);
	CASEN ("he", HEBREW);
	CASEN ("hi", HINDI);
	CASEN ("ja", JAPANESE);
	CASEN ("kn", KANNADA);
	CASE ("ks-in", KASHMIRI, INDIA);
	CASEN ("ks", KASHMIRI);
	CASEN ("kk", KAZAK);
	CASEN ("kok", KONKANI);
	CASEN ("ko", KOREAN);
	CASEN ("ky", KYRGYZ);
	CASEN ("ml", MALAYALAM);
	CASEN ("mni", MANIPURI);
	CASEN ("mr", MARATHI);
	CASEN ("mn", MONGOLIAN);
	CASE ("ne-in", NEPALI, INDIA);
	CASEN ("ne", NEPALI);
	CASEN ("or", ORIYA);
	CASEN ("pa", PUNJABI);
	CASEN ("sa", SANSKRIT);
	CASEN ("sd", SINDHI);
	CASEN ("syr", SYRIAC);
	CASEN ("ta", TAMIL);
	CASEN ("tt", TATAR);
	CASEN ("te", TELUGU);
	CASEN ("th", THAI);
	CASE ("ur-pk", URDU, PAKISTAN);
	CASE ("ur-in", URDU, INDIA);
	CASEN ("ur", URDU);
	CASEN ("uz", UZBEK);

#undef CASE
#undef CASEN

	return MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);
}

static WORD
langid(const char *lang)
{
#define LANGBUF 5
	static char last_lang[LANGBUF] = "";
	static WORD last_langid = MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);

	if ( strncmp( lang, last_lang, LANGBUF) == 0) return last_langid;
	last_langid = make_langid(lang);
	strncpy( last_lang, lang, LANGBUF);
	return last_langid;
#undef LANGBUF
}

#pragma pack(1)
typedef struct {
	HFONT font;
	DWORD script;
} ScriptCacheKey;
#pragma pack()

static Bool
win32_shaper( Handle self, PTextShapeRec t)
{
	Bool ok = false;
	HRESULT hr;
	WCHAR * wtext = NULL;
	uint16_t * out_clusters;
	int i, item, item_step, nitems, wlen;
	SCRIPT_CONTROL control;
	SCRIPT_ITEM *items = NULL;
	WORD *clusters = NULL;
	SCRIPT_VISATTR *visuals = NULL;
	int *advances = NULL;
	GOFFSET *goffsets = NULL;
	unsigned int * surrogate_map = NULL, first_surrogate_pair = 0;

	if ((items = malloc(sizeof(SCRIPT_ITEM) * (t-> len + 1))) == NULL)
		goto EXIT;
	if ((clusters = malloc(sizeof(WORD) * t-> n_glyphs_max)) == NULL)
		goto EXIT;
	if ((visuals = malloc(sizeof(SCRIPT_VISATTR) * t-> n_glyphs_max)) == NULL)
		goto EXIT;
	if ((wtext = malloc(sizeof(WCHAR) * 2 * t->len)) == NULL)
		goto EXIT;
	if ( t->flags & toPositions) {
		if ((advances = malloc(sizeof(int) * t->n_glyphs_max)) == NULL)
			goto EXIT;
		if ((goffsets = malloc(sizeof(GOFFSET) * t->n_glyphs_max)) == NULL)
			goto EXIT;
	}

	/* convert UTF-32 to UTF-16, mark surrogates */
	{
		int i;
		unsigned int index = 0, *curr = NULL;
		uint32_t *src;
		WCHAR *dst;
		for ( i = wlen = 0, src = t->text, dst = wtext; i < t->len; i++, index++) {
			uint32_t c = *src++;
			if ( c >= 0x10000 && c <= 0x10FFFF ) {
				c -= 0x10000;
				*(dst++) = 0xd800 + (c & 0x3ff);
				*(dst++) = 0xdc00 + (c >> 10);
				if ( !surrogate_map ) {
					first_surrogate_pair = i;
					surrogate_map = malloc(sizeof(unsigned int*) * (t-> len - first_surrogate_pair));
					if ( !surrogate_map ) goto EXIT;
					curr = surrogate_map;
				}
				*(curr++) = index;
				*(curr++) = index;
				wlen += 2;
			} else {
				if (( c >= 0xD800 && c <= 0xDFFF ) || ( c > 0x10FFFF )) c = 0;
				if ( surrogate_map )
					*(curr++) = index;
				*(dst++) = c;
				wlen++;
			}
		}
	}

	bzero(&control, sizeof(control));
	control.uDefaultLanguage = t->language ?
		langid(t->language) :
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	if ((hr = ScriptItemize(wtext, wlen, t->n_glyphs_max, &control, NULL, items, &nitems)) != S_OK) {
		apiHErr(hr);
		goto EXIT;
	}
	//printf("Itemize: %d\n", nitems);

	if ( t->flags & toRTL ) {
		item = nitems - 1;
		item_step = -1;
	} else {
		item = 0;
		item_step = 1;
	}

	for (
		i = 0, out_clusters = t-> clusters;
		i < nitems;
		i++, item += item_step
	) {
		int j, itemlen, last_char, nglyphs;
		int last_glyph;

		SCRIPT_CACHE * script_cache;
		ScriptCacheKey key = { sys fontResource->hfont, items[item].a.eScript };
		if (( script_cache = ( SCRIPT_CACHE*) hash_fetch( scriptCacheMan, &key, sizeof(key))) == NULL) {
			if ((script_cache = malloc(sizeof(SCRIPT_CACHE))) == NULL)
				goto EXIT;
			*script_cache = NULL;
			hash_store( scriptCacheMan, &key, sizeof(key), script_cache);
		}

		itemlen = items[item+1].iCharPos - items[item].iCharPos;
		items[item].a.fRTL = (t->flags & toRTL) ? 1 : 0;
		//printf("shape(%d @ %d) len %d %s\n", item, items[item].iCharPos, itemlen, items[item].a.fRTL ? "RTL" : "LTR");
		if (( hr = ScriptShape(
			sys ps, script_cache,
			wtext + items[item].iCharPos, itemlen, t->n_glyphs_max,
			&items[item].a,
			t->glyphs + t->n_glyphs, clusters, visuals,
			&nglyphs
		)) != S_OK) {
			apiHErr(hr);
			goto EXIT;
		}
#ifdef _DEBUG
		{
			int i;
			printf("shape input %d: ", item);
			for ( i = 0; i < itemlen; i++) {
				printf("%x ", *(wtext + items[item].iCharPos + i));
			}
			printf("\n");
			printf("shape output: ");
			for ( i = 0; i < nglyphs; i++) {
				printf("%d(%x) ", clusters[i], t->glyphs[t->n_glyphs + i]);
			}
			printf("\n");
		}
#endif

		if (items[item].a.fRTL) {
			for ( j = last_char = 0, last_glyph = nglyphs - 1; j < itemlen; j++) {
				int k, rlen = 1;
				WORD curr_glyph = clusters[j];
				last_char = j;
				for ( k = j + 1; k < itemlen; k++) {
					if ( clusters[k] == curr_glyph )
						rlen++;
					else
						break;
				}
				for ( ; last_glyph >= curr_glyph; last_glyph--) 
					out_clusters[last_glyph] = j + items[item].iCharPos;
				j += rlen - 1;
			}
			for ( ; last_glyph >= 0; last_glyph--)
				out_clusters[last_glyph] = last_char + items[item].iCharPos;
		} else {
			for ( j = last_char = last_glyph = 0; j < itemlen; j++) {
				int k, rlen = 1;
				WORD curr_glyph = clusters[j];
				last_char = j;
				for ( k = j + 1; k < itemlen; k++) {
					if ( clusters[k] == curr_glyph )
						rlen++;
					else
						break;
				}
				for ( ; last_glyph <= curr_glyph; last_glyph++) 
					out_clusters[last_glyph] = j + items[item].iCharPos;
				j += rlen - 1;
			}
			for ( ; last_glyph < nglyphs; last_glyph++)
				out_clusters[last_glyph] = last_char + items[item].iCharPos;
		}
#ifdef _DEBUG
		{
			int i;
			printf("clusters: ");
			for ( i = 0; i < nglyphs; i++) {
				printf("%d ", out_clusters[i]);
			}
			printf("\n");
		}
#endif

		/* map from utf16 */
		if ( surrogate_map ) {
			int k, loops = nglyphs;
			uint16_t * out_glyphs = t-> glyphs + t-> n_glyphs;
			for ( j = k = 0; j < loops; j++) {
				if ( k < j) {
					out_glyphs[k] = out_glyphs[j];
				}
				if ( surrogate_map && out_clusters[j] >= first_surrogate_pair) {
					out_clusters[k] = surrogate_map[out_clusters[j] - first_surrogate_pair];
					if ( j & 1 ) {
						k++;
						nglyphs--;
					}
				} else {
					out_clusters[k] = out_clusters[j];
					k++;
				}
			}
		}

		if ( t-> flags & toPositions) {
			ABC abc;
			GOFFSET * i_g;
			int * i_a;
			uint16_t * o_g, *o_a;
			printf("copying %d positions\n", nglyphs);
			if (( hr = ScriptPlace(sys ps, script_cache, t->glyphs + t->n_glyphs, nglyphs,
			       visuals, &items[item].a,
			       advances, goffsets, &abc)) != S_OK
			) {
				apiHErr(hr);
				goto EXIT;
			}
			for (
				i = 0,
				i_a = advances,    i_g = goffsets,
				o_a = t->advances, o_g = t->positions;
				i < nglyphs;
				i++
			) {
				*(o_a++) = *(i_a++);
				*(o_g++) = i_g->dv;
				*(o_g++) = i_g->du;
				i_g++;
			}
			t-> advances  += nglyphs;
			t-> positions += nglyphs * 2;
		}

		t-> n_glyphs += nglyphs;
		out_clusters += nglyphs;
	}

	ok = true;

EXIT:
	if ( surrogate_map  ) free(surrogate_map);
	if ( goffsets ) free(goffsets);
	if ( advances ) free(advances);
	if ( clusters ) free(clusters);
	if ( visuals  ) free(visuals);
	if ( items    ) free(items);
	if ( wtext    ) free(wtext);
	return ok;
}

PTextShapeFunc
apc_gp_get_text_shaper( Handle self, Bool * glyph_mapper_only)
{
	if ( !( sys tmPitchAndFamily & TMPF_TRUETYPE))
		return NULL;
	*glyph_mapper_only = false;
	return win32_shaper;
}

#define TM(field) to->field = from->field
void
textmetric_c2w( LPTEXTMETRICA from, LPTEXTMETRICW to)
{
	TM(tmHeight);
	TM(tmAscent);
	TM(tmDescent);
	TM(tmInternalLeading);
	TM(tmExternalLeading);
	TM(tmAveCharWidth);
	TM(tmMaxCharWidth);
	TM(tmWeight);
	TM(tmOverhang);
	TM(tmDigitizedAspectX);
	TM(tmDigitizedAspectY);
	TM(tmFirstChar);
	TM(tmLastChar);
	TM(tmDefaultChar);
	TM(tmBreakChar);
	TM(tmItalic);
	TM(tmUnderlined);
	TM(tmStruckOut);
	TM(tmPitchAndFamily);
	TM(tmCharSet);
}
#undef TM

PFontABC
apc_gp_get_font_abc( Handle self, int first, int last, int flags)
{objCheck NULL;{
	int i;
	PFontABC  f1;
	ABCFLOAT *f2 = NULL;
	ABC *f3 = NULL;

	if ( flags & toGlyphs ) {
		if ( !(f3 = ( ABC*) malloc(( last - first + 1) * sizeof( ABC))))
			return NULL;
	} else {
		if ( !( f2 = ( ABCFLOAT*) malloc(( last - first + 1) * sizeof( ABCFLOAT))))
			return NULL;
	}

	if ( !( f1 = ( PFontABC) malloc(( last - first + 1) * sizeof( FontABC)))) {
		if ( f2 ) free(f2);
		if ( f3 ) free(f3);
		return NULL;
	}


	if ( flags & toGlyphs ) {
		if (!GetCharABCWidthsI( sys ps, first, last - first + 1, NULL, f3)) apiErr;
	} else if ( flags & toUnicode ) {
		if (!GetCharABCWidthsFloatW( sys ps, first, last, f2)) apiErr;
	} else {
		if (!GetCharABCWidthsFloatA( sys ps, first, last, f2)) apiErr;
	}

	for ( i = 0; i <= last - first; i++) {
		f1[i].a = f2 ? f2[i].abcfA : f3[i].abcA;
		f1[i].b = f2 ? f2[i].abcfB : f3[i].abcB;
		f1[i].c = f2 ? f2[i].abcfC : f3[i].abcC;
	}

	if ( f2) free( f2);
	if ( f3) free( f3);
	return f1;
}}

/* extract vertical data from a bitmap */
Bool
gp_get_font_def_bitmap( Handle self, int first, int last, int flags, PFontABC abc)
{
	Bool ret = true;
	Font font;
	int i, j, h, w, lineSize;
	HBITMAP bm, oldBM;
	BITMAPINFO bi;
	HDC dc;
	LOGFONT logfont;
	HFONT hfont, oldFont;
	Byte * glyph, * empty;

	/* don't to need exact dimension, just to fit the glyph is enough */
	w = var font. maximalWidth * 2;
	h = var font. height;
	lineSize = (( w + 31) / 32) * 4;
	w = lineSize * 8;
	if ( !( glyph = malloc( lineSize * ( h + 1 ))))
		return false;
	empty = glyph + lineSize * h;
	memset( empty, 0xff, lineSize);

	if ( !( dc = CreateCompatibleDC( NULL ))) {
		free( glyph );
		return false;
	}
	if ( !( bm = CreateBitmap( w, h, 1, 1, NULL))) {
		free( glyph );
		return false;
	}

	bi. bmiHeader. biSize         = sizeof( bi. bmiHeader);
	bi. bmiHeader. biPlanes       = 1;
	bi. bmiHeader. biBitCount     = 1;
	bi. bmiHeader. biSizeImage    = lineSize * h;
	bi. bmiHeader. biWidth        = w;
	bi. bmiHeader. biHeight       = h;
	bi. bmiHeader. biCompression  = BI_RGB;
	bi. bmiHeader. biClrUsed      = 2;
	bi. bmiHeader. biClrImportant = 2;

	oldBM    = SelectObject( dc, bm );

	font = var font;
	font. direction = 0;
	font_font2logfont( &font, &logfont);
	hfont = CreateFontIndirect( &logfont);
	oldFont = SelectObject( dc, hfont );

	memset( abc, 0, sizeof(FontABC) * (last - first + 1));
	for ( i = 0; i <= last - first; i++) {
		Rectangle( dc, -1, -1, w+2, h+2 );
		if ( flags & toGlyphs ) {
			WCHAR ch = first + i;
			ExtTextOutW( dc, var font. maximalWidth, 0, ETO_GLYPH_INDEX, NULL, &ch, 1, NULL);
		} else if ( flags & toUnicode ) {
			WCHAR ch = first + i;
			TextOutW( dc, var font. maximalWidth, 0, &ch, 1);
		} else {
			CHAR ch = first + i;
			TextOutA( dc, var font. maximalWidth, 0, &ch, 1);
		}

		if ( !GetDIBits( dc, bm, 0, h, glyph, &bi, DIB_RGB_COLORS)) {
			ret = false;
			break;
		}

/*
		for ( j = 0; j < h; j++) {
			int k, l;
			for ( k = 0; k < lineSize; k++) {
				Byte * p = glyph + j * lineSize + k;
				printf(".");
				for ( l = 0; l < 8; l++) {
					int z = (*p) & ( 1 << (7-l) );
					printf("%s", z ? "*" : " ");
				}
			}
			printf("\n");
		}
*/

		for ( j = 0; j < h; j++) {
			if ( memcmp( glyph + j * lineSize, empty, lineSize) != 0 ) {
				abc[i]. a = j;
				break;
			}
		}
		for ( j = h - 1; j >= 0; j--) {
			if ( memcmp( glyph + j * lineSize, empty, lineSize) != 0 ) {
				abc[i]. c = h - j - 1;
				break;
			}
		}

		if ( abc[i]. a != 0 || abc[i].c != 0)
			abc[i]. b = h - abc[i]. a - abc[i]. c;
	}

	SelectObject( dc, oldFont);
	SelectObject( dc, oldBM );
	DeleteObject( hfont );
	DeleteObject( bm );
	DeleteDC( dc );
	free( glyph );
	return ret;
}

PFontABC
apc_gp_get_font_def( Handle self, int first, int last, int flags)
{objCheck nil;{
	int i;
	DWORD ret;
	PFontABC f1;
	MAT2 gmat = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
	GLYPHMETRICS g;

	f1 = ( PFontABC) malloc(( last - first + 1) * sizeof( FontABC));
	if ( !f1) return nil;

	for ( i = 0; i <= last - first; i++) {
		memset(&g, 0, sizeof(g));
		if ( flags & toGlyphs ) {
			ret = GetGlyphOutlineW(sys ps, i + first, GGO_METRICS | GGO_GLYPH_INDEX, &g, sizeof(g), NULL, &gmat);
		} else if ( flags & toUnicode ) {
			ret = GetGlyphOutlineW(sys ps, i + first, GGO_METRICS, &g, sizeof(g), NULL, &gmat);
		} else {
			ret = GetGlyphOutlineA(sys ps, i + first, GGO_METRICS, &g, sizeof(g), NULL, &gmat);
		}
		if ( ret == GDI_ERROR ) {
			if ( !gp_get_font_def_bitmap( self, first, last, flags, f1 )) {
				free( f1 );
				return nil;
			}
			return f1;
		}
		f1[i]. a = var font. descent + g.gmptGlyphOrigin. y - g.gmBlackBoxY; /* XXX g.gmCellIncY ? */
		f1[i]. b = g.gmBlackBoxY;
		f1[i]. c = var font.ascent - g.gmptGlyphOrigin. y;
	}

	return f1;
}}

static Handle ipa_extensions_ranges[] = {
	0x000250 , 0x0002AF, // 4 IPA Extensions
	0x001D00 , 0x001D7F, //    Phonetic Extensions
	0x001D80 , 0x001DBF, //    Phonetic Extensions Supplement
};

static Handle spacing_modifier_letters_ranges[] = {
	0x0002B0 , 0x0002FF, // 5 Spacing Modifier Letters
	0x00A700 , 0x00A71F, //    Modifier Tone Letters
};

static Handle combining_diacritical_marks_ranges[] = {
	0x000300 , 0x00036F, // 6 Combining Diacritical Marks
	0x001DC0 , 0x001DFF, //    Combining Diacritical Marks Supplement
};

static Handle cyrillic_ranges[] = {
	0x000400 , 0x0004FF, // 9 Cyrillic
	0x000500 , 0x00052F, //    Cyrillic Supplement
	0x002DE0 , 0x002DFF, //    Cyrillic Extended-A
	0x00A640 , 0x00A69F, //    Cyrillic Extended-B
};

static Handle arabic_ranges[] = {
	0x000600 , 0x0006FF, // 13 Arabic
	0x000750 , 0x00077F, //    Arabic Supplement
};

static Handle georgian_ranges[] = {
	0x0010A0 , 0x0010FF, // 26 Georgian
	0x002D00 , 0x002D2F, //    Georgian Supplement
};

static Handle latin_extended_additional_ranges[] = {
	0x001E00 , 0x001EFF, // 29 Latin Extended Additional
	0x002C60 , 0x002C7F, //    Latin Extended-C
	0x00A720 , 0x00A7FF, //    Latin Extended-D
};

static Handle general_punctuation_ranges[] = {
	0x002000 , 0x00206F, // 31 General Punctuation
	0x002E00 , 0x002E7F, //    Supplemental Punctuation
};

static Handle arrows_ranges[] = {
	0x002190 , 0x0021FF, // 37 Arrows
	0x0027F0 , 0x0027FF, //    Supplemental Arrows-A
	0x002900 , 0x00297F, //    Supplemental Arrows-B
	0x002B00 , 0x002BFF, //    Miscellaneous Symbols and Arrows
};

static Handle mathematical_operators_ranges[] = {
	0x002200 , 0x0022FF, // 38 Mathematical Operators
	0x0027C0 , 0x0027EF, //    Miscellaneous Mathematical Symbols-A
	0x002980 , 0x0029FF, //    Miscellaneous Mathematical Symbols-B
	0x002A00 , 0x002AFF, //    Supplemental Mathematical Operators
};

static Handle katakana_ranges[] = {
	0x0030A0 , 0x0030FF, // 50 Katakana
	0x0031F0 , 0x0031FF, //    Katakana Phonetic Extensions
};

static Handle bopomofo_ranges[] = {
	0x003100 , 0x00312F, // 51 Bopomofo
	0x0031A0 , 0x0031BF, //    Bopomofo Extended
};

static Handle cjk_radicals_supplement_ranges[] = {
	0x002E80 , 0x002EFF, // 59 CJK Radicals Supplement
	0x002F00 , 0x002FDF, //    Kangxi Radicals
	0x002FF0 , 0x002FFF, //    Ideographic Description Characters
	0x003190 , 0x00319F, //    Kanbun
	0x003400 , 0x004DBF, //    CJK Unified Ideographs Extension A
	0x004E00 , 0x009FFF, //    CJK Unified Ideographs
	0x020000 , 0x02A6DF, //    CJK Unified Ideographs Extension B
};

static Handle cjk_strokes_ranges[] = {
	0x0031C0 , 0x0031EF, // 61 CJK Strokes
	0x00F900 , 0x00FAFF, //    CJK Compatibility Ideographs
	0x02F800 , 0x02FA1F, //    CJK Compatibility Ideographs Supplement
};

static Handle vertical_forms_ranges[] = {
	0x00FE10 , 0x00FE1F, // 65 Vertical Forms
	0x00FE30 , 0x00FE4F, //    CJK Compatibility Forms
};

static Handle ethiopic_ranges[] = {
	0x001200 , 0x00137F, // 75 Ethiopic
	0x001380 , 0x00139F, //    Ethiopic Supplement
	0x002D80 , 0x002DDF, //    Ethiopic Extended
};

static Handle khmer_ranges[] = {
	0x001780 , 0x0017FF, // 80 Khmer
	0x0019E0 , 0x0019FF, //    Khmer Symbols
};

static Handle yi_syllables_ranges[] = {
	0x00A000 , 0x00A48F, // 83 Yi Syllables
	0x00A490 , 0x00A4CF, //    Yi Radicals
};

static Handle tagalog_ranges[] = {
	0x001700 , 0x00171F, // 84 Tagalog
	0x001720 , 0x00173F, //    Hanunoo
	0x001740 , 0x00175F, //    Buhid
	0x001760 , 0x00177F, //    Tagbanwa
};

static Handle byzantine_musical_symbols_ranges[] = {
	0x01D000 , 0x01D0FF, // 88 Byzantine Musical Symbols
	0x01D100 , 0x01D1FF, //    Musical Symbols
	0x01D200 , 0x01D24F, //    Ancient Greek Musical Notation
};

static Handle private_use__plane_15__ranges[] = {
	0x0FF000 , 0x0FFFFD, // 90 Private Use (plane 15)
	0x100000 , 0x10FFFD, //    Private Use (plane 16)
};

static Handle variation_selectors_ranges[] = {
	0x00FE00 , 0x00FE0F, // 91 Variation Selectors
	0x0E0100 , 0x0E01EF, //    Variation Selectors Supplement
};

static Handle linear_b_syllabary_ranges[] = {
	0x010000 , 0x01007F, // 101 Linear B Syllabary
	0x010080 , 0x0100FF, //    Linear B Ideograms
	0x010100 , 0x01013F, //    Aegean Numbers
};

static Handle cuneiform_ranges[] = {
	0x012000 , 0x0123FF, // 110 Cuneiform
	0x012400 , 0x01247F, //    Cuneiform Numbers and Punctuation
};

static Handle lycian_ranges[] = {
	0x010280 , 0x01029F, // 121 Lycian
	0x0102A0 , 0x0102DF, //    Carian
	0x010920 , 0x01093F, //    Lydian
};

static Handle mahjong_tiles_ranges[] = {
	0x01F000 , 0x01F02F, // 122 Mahjong Tiles
	0x01F030 , 0x01F09F, //    Domino Tiles
};

#define SUBRANGES 123

static Handle unicode_subranges[SUBRANGES * 2] = {
	0x000000 , 0x00007F, // 0 Basic Latin
	0x000080 , 0x0000FF, // 1 Latin-1 Supplement
	0x000100 , 0x00017F, // 2 Latin Extended-A
	0x000180 , 0x00024F, // 3 Latin Extended-B
	3        , (Handle) &ipa_extensions_ranges, // 4 
	2        , (Handle) &spacing_modifier_letters_ranges, // 5 
	2        , (Handle) &combining_diacritical_marks_ranges, // 6 
	0x000370 , 0x0003FF, // 7 Greek and Coptic
	0x002C80 , 0x002CFF, // 8 Coptic
	4        , (Handle) &cyrillic_ranges, // 9 
	0x000530 , 0x00058F, // 10 Armenian
	0x000590 , 0x0005FF, // 11 Hebrew
	0x00A500 , 0x00A63F, // 12 Vai
	2        , (Handle) &arabic_ranges, // 13 
	0x0007C0 , 0x0007FF, // 14 NKo
	0x000900 , 0x00097F, // 15 Devanagari
	0x000980 , 0x0009FF, // 16 Bangla
	0x000A00 , 0x000A7F, // 17 Gurmukhi
	0x000A80 , 0x000AFF, // 18 Gujarati
	0x000B00 , 0x000B7F, // 19 Odia
	0x000B80 , 0x000BFF, // 20 Tamil
	0x000C00 , 0x000C7F, // 21 Telugu
	0x000C80 , 0x000CFF, // 22 Kannada
	0x000D00 , 0x000D7F, // 23 Malayalam
	0x000E00 , 0x000E7F, // 24 Thai
	0x000E80 , 0x000EFF, // 25 Lao
	2        , (Handle) &georgian_ranges, // 26 
	0x001B00 , 0x001B7F, // 27 Balinese
	0x001100 , 0x0011FF, // 28 Hangul Jamo
	3        , (Handle) &latin_extended_additional_ranges, // 29 
	0x001F00 , 0x001FFF, // 30 Greek Extended
	2        , (Handle) &general_punctuation_ranges, // 31 
	0x002070 , 0x00209F, // 32 Superscripts And Subscripts
	0x0020A0 , 0x0020CF, // 33 Currency Symbols
	0x0020D0 , 0x0020FF, // 34 Combining Diacritical Marks For Symbols
	0x002100 , 0x00214F, // 35 Letterlike Symbols
	0x002150 , 0x00218F, // 36 Number Forms
	4        , (Handle) &arrows_ranges, // 37 
	4        , (Handle) &mathematical_operators_ranges, // 38 
	0x002300 , 0x0023FF, // 39 Miscellaneous Technical
	0x002400 , 0x00243F, // 40 Control Pictures
	0x002440 , 0x00245F, // 41 Optical Character Recognition
	0x002460 , 0x0024FF, // 42 Enclosed Alphanumerics
	0x002500 , 0x00257F, // 43 Box Drawing
	0x002580 , 0x00259F, // 44 Block Elements
	0x0025A0 , 0x0025FF, // 45 Geometric Shapes
	0x002600 , 0x0026FF, // 46 Miscellaneous Symbols
	0x002700 , 0x0027BF, // 47 Dingbats
	0x003000 , 0x00303F, // 48 CJK Symbols And Punctuation
	0x003040 , 0x00309F, // 49 Hiragana
	2        , (Handle) &katakana_ranges, // 50 
	2        , (Handle) &bopomofo_ranges, // 51 
	0x003130 , 0x00318F, // 52 Hangul Compatibility Jamo
	0x00A840 , 0x00A87F, // 53 Phags-pa
	0x003200 , 0x0032FF, // 54 Enclosed CJK Letters And Months
	0x003300 , 0x0033FF, // 55 CJK Compatibility
	0x00AC00 , 0x00D7AF, // 56 Hangul Syllables
	0x00D800 , 0x00DFFF, // 57 Non-Plane 0. Note that setting this bit implies that there is at least one supplementary code point beyond the Basic Multilingual Plane (BMP) that is supported by this font. See Surrogates and Supplementary Characters.
	0x010900 , 0x01091F, // 58 Phoenician
	7        , (Handle) &cjk_radicals_supplement_ranges, // 59 
	0x00E000 , 0x00F8FF, // 60 Private Use Area
	3        , (Handle) &cjk_strokes_ranges, // 61 
	0x00FB00 , 0x00FB4F, // 62 Alphabetic Presentation Forms
	0x00FB50 , 0x00FDFF, // 63 Arabic Presentation Forms-A
	0x00FE20 , 0x00FE2F, // 64 Combining Half Marks
	2        , (Handle) &vertical_forms_ranges, // 65 
	0x00FE50 , 0x00FE6F, // 66 Small Form Variants
	0x00FE70 , 0x00FEFF, // 67 Arabic Presentation Forms-B
	0x00FF00 , 0x00FFEF, // 68 Halfwidth And Fullwidth Forms
	0x00FFF0 , 0x00FFFF, // 69 Specials
	0x000F00 , 0x000FFF, // 70 Tibetan
	0x000700 , 0x00074F, // 71 Syriac
	0x000780 , 0x0007BF, // 72 Thaana
	0x000D80 , 0x000DFF, // 73 Sinhala
	0x001000 , 0x00109F, // 74 Myanmar
	3        , (Handle) &ethiopic_ranges, // 75 
	0x0013A0 , 0x0013FF, // 76 Cherokee
	0x001400 , 0x00167F, // 77 Unified Canadian Aboriginal Syllabics
	0x001680 , 0x00169F, // 78 Ogham
	0x0016A0 , 0x0016FF, // 79 Runic
	2        , (Handle) &khmer_ranges, // 80 
	0x001800 , 0x0018AF, // 81 Mongolian
	0x002800 , 0x0028FF, // 82 Braille Patterns
	2        , (Handle) &yi_syllables_ranges, // 83 
	4        , (Handle) &tagalog_ranges, // 84 
	0x010300 , 0x01032F, // 85 Old Italic
	0x010330 , 0x01034F, // 86 Gothic
	0x010400 , 0x01044F, // 87 Deseret
	3        , (Handle) &byzantine_musical_symbols_ranges, // 88 
	0x01D400 , 0x01D7FF, // 89 Mathematical Alphanumeric Symbols
	2        , (Handle) &private_use__plane_15__ranges, // 90 
	2        , (Handle) &variation_selectors_ranges, // 91 
	0x0E0000 , 0x0E007F, // 92 Tags
	0x001900 , 0x00194F, // 93 Limbu
	0x001950 , 0x00197F, // 94 Tai Le
	0x001980 , 0x0019DF, // 95 New Tai Lue
	0x001A00 , 0x001A1F, // 96 Buginese
	0x002C00 , 0x002C5F, // 97 Glagolitic
	0x002D30 , 0x002D7F, // 98 Tifinagh
	0x004DC0 , 0x004DFF, // 99 Yijing Hexagram Symbols
	0x00A800 , 0x00A82F, // 100 Syloti Nagri
	3        , (Handle) &linear_b_syllabary_ranges, // 101 
	0x010140 , 0x01018F, // 102 Ancient Greek Numbers
	0x010380 , 0x01039F, // 103 Ugaritic
	0x0103A0 , 0x0103DF, // 104 Old Persian
	0x010450 , 0x01047F, // 105 Shavian
	0x010480 , 0x0104AF, // 106 Osmanya
	0x010800 , 0x01083F, // 107 Cypriot Syllabary
	0x010A00 , 0x010A5F, // 108 Kharoshthi
	0x01D300 , 0x01D35F, // 109 Tai Xuan Jing Symbols
	2        , (Handle) &cuneiform_ranges, // 110 
	0x01D360 , 0x01D37F, // 111 Counting Rod Numerals
	0x001B80 , 0x001BBF, // 112 Sundanese
	0x001C00 , 0x001C4F, // 113 Lepcha
	0x001C50 , 0x001C7F, // 114 Ol Chiki
	0x00A880 , 0x00A8DF, // 115 Saurashtra
	0x00A900 , 0x00A92F, // 116 Kayah Li
	0x00A930 , 0x00A95F, // 117 Rejang
	0x00AA00 , 0x00AA5F, // 118 Cham
	0x010190 , 0x0101CF, // 119 Ancient Symbols
	0x0101D0 , 0x0101FF, // 120 Phaistos Disc
	3        , (Handle) &lycian_ranges, // 121 
	2        , (Handle) &mahjong_tiles_ranges, // 122 
};

static Handle ctx_CHARSET2index[] = {
	// SHIFTJIS_CHARSET   ??
	HANGEUL_CHARSET     , 28,
	// GB2312_CHARSET     ??
	// CHINESEBIG5_CHARSET ??
#ifdef JOHAB_CHARSET
	GREEK_CHARSET       , 7,
	HEBREW_CHARSET      , 11,
	ARABIC_CHARSET      , 13,
	// TURKISH_CHARSET    ??
	// VIETNAMESE_CHARSET ??
	THAI_CHARSET        , 24,
	EASTEUROPE_CHARSET  , 3,
	RUSSIAN_CHARSET     , 9,
	// MAC_CHARSET        ??
	BALTIC_CHARSET      , 2,
#endif
	endCtx
};

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{objCheck nil;{
	int i;
	unsigned long * ret;
	FONTSIGNATURE f;

	memset( &f, 0, sizeof(f));
	i = GetTextCharsetInfo( sys ps, &f, 0);
	if ( i == DEFAULT_CHARSET)
		apiErrRet;

	if ( f. fsUsb[0] == 0 && f. fsUsb[1] == 0 && f. fsUsb[2] == 0 && f. fsUsb[3] == 0) {
		int index = ctx_remap_end( i, ctx_CHARSET2index, true);
		TEXTMETRICW tm;
		if ( !( ret = malloc( sizeof( unsigned long) * 4)))
			return nil;
		GetTextMetricsW( sys ps, &tm);
		if ( index == endCtx) {
			ret[0] = 0x20;
			ret[1] = 0xff;
			*count = 2;
		} else {
			*count = 0;
			if ( index > 0) {
				ret[(*count)++] = unicode_subranges[ 0];
				ret[(*count)++] = unicode_subranges[ 1];
			}
			ret[(*count)++] = unicode_subranges[ index * 2];
			ret[(*count)++] = unicode_subranges[ index * 2 + 1];
		}
		if ( ret[0] > tm. tmFirstChar) ret[0] = tm.tmFirstChar;
		return ret;
	}

	for ( i = 0; i < SUBRANGES; i++)
		if ( f. fsUsb[ i / 32] & ( 1 << (i % 32))) {
			Handle x = unicode_subranges[i * 2];
			if ( x < 0x20)
				(*count) += x * 2;
			else
				(*count) += 2;
		}

	if ( !( ret = malloc( sizeof( long) * (*count))))
		return nil;

	*count = 0;
	for ( i = 0; i < SUBRANGES; i++)
		if ( f. fsUsb[ i / 32] & ( 1 << (i % 32))) {
			Handle x = unicode_subranges[i * 2];
			if ( x < 0x20) {
				Handle * z = ( Handle *) unicode_subranges[i * 2 + 1];
				while ( x--) {
					ret[ (*count)++] = *(z++);
					ret[ (*count)++] = *(z++);
				}
			} else {
				ret[ (*count)++] = x;
				ret[ (*count)++] = unicode_subranges[i * 2 + 1];
			}
		}

	return ret;
}}


static int
gp_get_text_width( Handle self, const char* text, int len, int flags)
{
	SIZE  sz;
	int   div, offset = 0, ret = 0;

	objCheck 0;
	if ( len == 0) return 0;

	/* width more that 32K returned incorrectly by Win32 core */
	if (( div = 32768L / ( var font. maximalWidth ? var font. maximalWidth : 1)) == 0)
		div = 1;

	while ( offset < len) {
		int chunk_len = ( offset + div > len) ? ( len - offset) : div;
		if ( flags & toGlyphs) {
			if ( !GetTextExtentPointI( sys ps, ( WCHAR*) text + offset, chunk_len, &sz)) apiErr;
		} else if ( flags & toUnicode) {
			if ( !GetTextExtentPoint32W( sys ps, ( WCHAR*) text + offset, chunk_len, &sz)) apiErr;
		} else {
			if ( !GetTextExtentPoint32( sys ps, text + offset, chunk_len, &sz)) apiErr;
		}
		ret += sz. cx;
		offset += div;
	}

	if ( flags & toAddOverhangs) {
		if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
			ABC abc[2];
			if ( flags & toGlyphs) {
				WCHAR * t = (WCHAR*) text;
				GetCharABCWidthsI( sys ps, *t, 1, NULL, &abc[0]);
				GetCharABCWidthsI( sys ps, t[len-1], 1, NULL, &abc[1]);
			} else if ( flags & toUnicode) {
				WCHAR * t = (WCHAR*) text;
				if ( guts. utf8_prepend_0x202D ) {
					/* the 1st character is 0x202D, skip it */
					t++;
					len--;
				}
				GetCharABCWidthsW( sys ps, *t, *t, &abc[0]);
				GetCharABCWidthsW( sys ps, t[len-1], t[len-1], &abc[1]);
			} else {
				GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
				GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
			}
			if ( abc[0]. abcA < 0) ret -= abc[0]. abcA;
			if ( abc[1]. abcC < 0) ret -= abc[1]. abcC;
		}
	}

	return ret;
}

int
apc_gp_get_text_width( Handle self, const char* text, int len, int flags)
{
	int ret;
	if ( flags & toGlyphs ) flags &= ~toUTF8;
	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) return 0;
		len = mb_len;
	}
	ret = gp_get_text_width( self, text, len, flags);
	if ( flags & toUTF8)
		free(( char*) text);
	return ret;
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len, int flags)
{objCheck nil;{
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return nil;

	memset( pt, 0, sizeof( Point) * 5);

	if ( flags & toGlyphs ) flags &= ~toUTF8;

	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) {
			free( pt);
			return nil;
		}
		len = mb_len;
	}

	pt[0].y = pt[2]. y = var font. ascent - 1;
	pt[1].y = pt[3]. y = - var font. descent;
	pt[4].y = pt[0]. x = pt[1].x = 0;
	pt[3].x = pt[2]. x = pt[4].x = gp_get_text_width( self, text, len, flags );

	if ( !is_apt( aptTextOutBaseline)) {
		int i = 4, d = var font. descent;
		while ( i--) pt[ i]. y += d;
	}

	if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
		ABC abc[2];
		if ( flags & toGlyphs ) {
			WCHAR * t = (WCHAR*) text;
			GetCharABCWidthsI( sys ps, *t, 1, NULL, &abc[0]);
			GetCharABCWidthsI( sys ps, t[len-1], 1, NULL, &abc[1]);
		} else if ( flags & toUTF8 ) {
			WCHAR * t = (WCHAR*) text;
			if ( guts. utf8_prepend_0x202D ) {
				/* the 1st character is 0x202D, skip it */
				t++;
				len--;
			}
			GetCharABCWidthsW( sys ps, *t, *t, &abc[0]);
			GetCharABCWidthsW( sys ps, t[len-1], t[len-1], &abc[1]);
		} else {
			GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
			GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
		}
		if ( abc[0]. abcA < 0) {
			pt[0].x += abc[0]. abcA;
			pt[1].x += abc[0]. abcA;
		}
		if ( abc[1]. abcC < 0) {
			pt[2].x -= abc[1]. abcC;
			pt[3].x -= abc[1]. abcC;
		}
	}

	if ( var font. direction != 0) {
		int i;
		float s = sin( var font. direction / GRAD);
		float c = cos( var font. direction / GRAD);
		for ( i = 0; i < 5; i++) {
			float x = pt[i]. x * c - pt[i]. y * s;
			float y = pt[i]. x * s + pt[i]. y * c;
			pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
			pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
		}
	}

	if ( flags & toUTF8 ) free(( char*) text);

	return pt;
}}

int
apc_gp_get_glyph_outline( Handle self, int index, int flags, int ** buffer)
{
	int offset, gdi_size, r_size, *r_buf, *r_ptr;
	Byte * gdi_buf;
	GLYPHMETRICS gm;
	MAT2 matrix;
	UINT format;

	memset(&matrix, 0, sizeof(matrix));
	matrix.eM11.value = matrix.eM22.value = 1;

	format = GGO_NATIVE;
	if ( flags & ggoGlyphIndex )       format |= GGO_GLYPH_INDEX;
	if (( flags & ggoUseHints ) == 0 ) format |= GGO_UNHINTED;

	gdi_size = (flags & (ggoUnicode | ggoGlyphIndex)) ?
		GetGlyphOutlineW(sys ps, index, format, &gm, 0, NULL, &matrix) :
		GetGlyphOutlineA(sys ps, index, format, &gm, 0, NULL, &matrix);
	if ( gdi_size <= 0 ) {
		if ( gdi_size < 0 ) apiErr;
		return 0;
	}

	if (( gdi_buf = malloc(gdi_size)) == NULL ) {
		warn("Not enough memory");
		return 0;
	}

	if (
		( (flags & (ggoUnicode | ggoGlyphIndex) ) ?
			GetGlyphOutlineW(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix) :
			GetGlyphOutlineA(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix)
		) == GDI_ERROR
	) {
		apiErr;
		free(gdi_buf);
		return 0;
	}

	offset = 0;
	r_size = 0;
	while ( offset < gdi_size ) {
		TTPOLYGONHEADER * h = ( TTPOLYGONHEADER*) (gdi_buf + offset);
		unsigned int curve_offset = sizeof(TTPOLYGONHEADER);
		r_size += 2 /* cmd=ggoMove */ + 2 /* x, y */;
		while ( curve_offset < h->cb ) {
			TTPOLYCURVE * c = (TTPOLYCURVE*) (gdi_buf + offset + curve_offset);
			curve_offset += sizeof(WORD) * 2 + c->cpfx * sizeof(POINTFX);
			r_size += 2 /* cmd */ + c->cpfx * 2;
		}
		offset += h->cb;
	}
	if (( r_buf = malloc(r_size * sizeof(int))) == NULL ) {
		warn("Not enough memory");
		free(gdi_buf);
		return 0;
	}
	r_ptr = r_buf;

	offset = 0;
#define PTX(x) (x.value * 64 + x.fract / (0x10000 / 64))
	while ( offset < gdi_size ) {
		TTPOLYGONHEADER * h = ( TTPOLYGONHEADER*) (gdi_buf + offset);
		unsigned int curve_offset = sizeof(TTPOLYGONHEADER);
		*(r_ptr++) = ggoMove;
		*(r_ptr++) = 1;
		*(r_ptr++) = PTX(h->pfxStart.x);
		*(r_ptr++) = PTX(h->pfxStart.y);
		while ( curve_offset < h->cb ) {
			int i;
			TTPOLYCURVE * c = (TTPOLYCURVE*) (gdi_buf + offset + curve_offset);
			switch ( c-> wType ) {
			case TT_PRIM_LINE:
				*(r_ptr++) = ggoLine;
				break;
			case TT_PRIM_QSPLINE:
				*(r_ptr++) = ggoConic;
				break;
			case TT_PRIM_CSPLINE:
				*(r_ptr++) = ggoCubic;
				break;
			default:
				warn("Unknown constant TT_PRIM_%d\n", c->wType);
				free(gdi_buf);
				free(r_buf);
				return 0;
			}
			*(r_ptr++) = c-> cpfx;
			for ( i = 0; i < c-> cpfx; i++) {
				*(r_ptr++) = PTX(c->apfx[i].x);
				*(r_ptr++) = PTX(c->apfx[i].y);
			}
			curve_offset += sizeof(WORD) * 2 + c->cpfx * sizeof(POINTFX);
		}
		offset += h->cb;
	}
	free(gdi_buf);
	*buffer = r_buf;
	return r_size;
}

Bool
apc_gp_get_text_out_baseline( Handle self)
{
	objCheck 0;
	return is_apt( aptTextOutBaseline);
}

Bool
apc_gp_get_text_opaque( Handle self)
{
	objCheck false;
	return is_apt( aptTextOpaque);
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
	objCheck false;
	apt_assign( aptTextOpaque, opaque);
	return true;
}


Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
	objCheck false;
	apt_assign( aptTextOutBaseline, baseline);
	if ( sys ps) SetTextAlign( sys ps, baseline ? TA_BASELINE : TA_BOTTOM);
	return true;
}

#ifdef __cplusplus
}
#endif
