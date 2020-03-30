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

	use_path = GetROP2( sys ps) != R2_COPYPEN;
	if ( use_path ) {
		STYLUS_USE_BRUSH( ps);
		BeginPath(ps);
	} else {
		STYLUS_USE_TEXT( ps);
		if ( opa != bk) SetBkMode( ps, opa);
	}

	ok = ( flags & toUTF8 ) ?
		TextOutW( ps, x, sys lastSize. y - y, ( U16*)text, len) :
		TextOutA( ps, x, sys lastSize. y - y, text, len);
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

Bool
apc_gp_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y)
{objCheck false;{
	Bool ok = true;
	HDC ps = sys ps;
	int bk  = GetBkMode( ps);
	int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
	Bool use_path;

	if ( t->len > 8192 ) t->len = 8192;
	use_path = GetROP2( sys ps) != R2_COPYPEN;
	if ( use_path ) {
		STYLUS_USE_BRUSH( ps);
		BeginPath(ps);
	} else {
		STYLUS_USE_TEXT( ps);
		if ( opa != bk) SetBkMode( ps, opa);
	}

	if ( t-> advances ) {
		#define SZ 1024
		INT dx[SZ], n, *pdx, *pdx2;
		uint16_t *goffsets = t->positions, *advances = t->advances;
		n = t-> len * 2;
		if ( n > SZ) {
			if ( !( pdx = malloc(sizeof(INT) * n)))
				pdx = dx;
		} else
			pdx = dx;
		pdx2 = pdx;
		n /= 2;
		while ( n-- > 0 ) {
			*(pdx2++) = *(goffsets++) + *(advances++);
			*(pdx2++) = *(goffsets++);
		}
		ok = ExtTextOutW(ps, x, sys lastSize. y - y, ETO_GLYPH_INDEX | ETO_PDY, NULL, (LPCWSTR) t->glyphs, t->len, pdx);
		if ( pdx != dx ) free(pdx);
		#undef SZ
	} else
		ok = ExtTextOutW(ps, x, sys lastSize. y - y, ETO_GLYPH_INDEX, NULL, (LPCWSTR) t->glyphs, t->len, NULL);
	if ( !ok ) apiErr;

	if ( use_path ) {
		EndPath(ps);
		FillPath(ps);
	} else {
		if ( opa != bk) SetBkMode( ps, bk);
	}

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

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{objCheck nil;{
	DWORD i, j, size;
	GLYPHSET * gs;
	unsigned long * ret;
	WCRANGE *src;

	*count = 0;
	if (( size = GetFontUnicodeRanges( sys ps, NULL )) == 0) 
		return NULL;
	if (!( gs = malloc(size)))
		apiErrRet;
	gs-> cbThis = size;
	if ( GetFontUnicodeRanges( sys ps, gs ) == 0) {
		free(gs);
		apiErrRet;
	}
	if ( !( ret = malloc(sizeof(unsigned long) * 2 * gs->cRanges))) {
		free(gs);
		return NULL;
	}
	for ( i = j = 0, src = gs-> ranges; i < gs->cRanges; i++, src++) {
		ret[j++] = src->wcLow;
		ret[j++] = src->wcLow + src->cGlyphs - 1;
	}
	*count = j;

	free(gs);
	return ret;
}}

unsigned char *
apc_gp_get_font_languages( Handle self)
{objCheck nil;{
	return NULL;
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
	flags &= ~toGlyphs;
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

int
apc_gp_get_glyphs_width( Handle self, PGlyphsOutRec t)
{
	return gp_get_text_width( self, (const char*)t->glyphs, t->len, t->flags | toGlyphs);
}

static void
gp_get_text_box( Handle self, const char * text, int len, int flags, Point * pt)
{
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
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len, int flags)
{objCheck nil;{
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return nil;

	memset( pt, 0, sizeof( Point) * 5);

	flags &= ~toGlyphs;
	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) {
			free( pt);
			return nil;
		}
		len = mb_len;
	}
	gp_get_text_box(self, text, len, flags, pt);
	if ( flags & toUTF8 ) free(( char*) text);
	return pt;
}}

Point *
apc_gp_get_glyphs_box( Handle self, PGlyphsOutRec t)
{objCheck nil;{
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return nil;

	memset( pt, 0, sizeof( Point) * 5);
	gp_get_text_box(self, (const char*)t->glyphs, t->len, toGlyphs, pt);
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
