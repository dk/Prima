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

typedef struct {
	HDC dc;
	int i, len, stop;
	Bool fixed_pitch, orig_fixed_pitch;
	Handle self;
	HFONT orig, saved, curr;
	PDCFont nondefault_font;
	uint32_t nondefault_fid;
	uint16_t *fonts;
} FontContext;

static void
font_context_init( FontContext * fc, Handle self, PGlyphsOutRec t)
{
	bzero(fc, sizeof(FontContext));
	fc->self  = self;
	fc->orig  = sys dc_font->hfont;
	fc->fonts = t->fonts;
	fc->len   = t->len;
	fc->dc    = sys ps;
	fc->fixed_pitch = fc->orig_fixed_pitch = (sys tmPitchAndFamily & TMPF_FIXED_PITCH) ? 0 : 1;
}

static void
font_context_done( FontContext * fc )
{
	if ( fc-> nondefault_font )
		font_free(fc-> nondefault_font, false);
	if ( fc-> saved )
		SelectObject(fc->dc, fc->saved);
}

static void
font_context_rewind( FontContext * fc, int index )
{
	fc-> i = index;
	fc-> stop = fc-> i >= fc-> len;
}

static int
font_context_next( FontContext * fc )
{
	Font *_src, src, dst;
	uint16_t nfid;
	int start, len;
	HFONT hfont, selected;
	Bool fixed_pitch;

	if ( fc-> stop ) return 0;

	if ( !fc-> fonts ) {
		start = fc-> i;
		fc-> i = fc-> len;
		fc-> stop = true;
		return fc-> i - start;
	}

	for (
		len = 0, start = fc->i, nfid = fc->fonts[fc->i];
		fc->i <= fc->len;
		fc->i++
	) {
		if (
			fc->i >= fc->len || fc->fonts[fc->i] != nfid
		) {
			len = fc->i - start;
			break;
		}
	}
	fc-> stop = fc-> i >= fc-> len;
	if ( len == 0 ) return 0;

	if ( nfid == 0 ) {
		hfont       = fc->orig;
		fixed_pitch = fc->orig_fixed_pitch;
	} else if ( nfid == fc->nondefault_fid ) {
		hfont       = fc->nondefault_font->hfont;
		fixed_pitch = fc->nondefault_font->font.pitch == fpFixed;
	} else if ( !( _src = prima_font_mapper_get_font(nfid))) {
		hfont       = fc->orig;
		fixed_pitch = fc->orig_fixed_pitch;
	} else {
		dst = (( PWidget) fc->self)-> font;
		src = *_src;
#define CP(x) src.x = dst.x; src.undef.x = 0;
		CP(size)
		CP(direction)
#undef CP
		src.direction = dst.direction;
		apc_font_pick(fc->self, &src, &dst);
		if ( strcmp(src.name, dst.name) == 0) {
			if ( fc-> nondefault_font )
				font_free(fc-> nondefault_font, false);
			fc->nondefault_font = font_alloc(&dst);
			fc->nondefault_fid  = nfid;
			hfont               = fc->nondefault_font->hfont;
			fixed_pitch         = fc->nondefault_font->font.pitch == fpFixed;
		} else {
			hfont               = fc->orig;
			fixed_pitch         = fc->orig_fixed_pitch;
		}
	}

	fc->curr = hfont;
	fc->fixed_pitch = fixed_pitch;
	selected = SelectObject(fc->dc, fc->curr);
	if ( !fc->saved ) fc->saved = selected;

	return len;
}

/*
	emulate underscore and strikeout because ExtTextOutW with ETO_PDY underlines each glyph separately .
	Also, Gdip ignores viewport, specify params in parallel
*/
static void
underscore_font( Handle self, int x1, int y1, int x2, int y2, int width, Bool use_alpha)
{
	HDC dc = sys ps;
	GpPen * gppen = NULL;
	NPoint cs = CDrawable(self)->trig_cache(self);

	if ( var font. style & (fsUnderlined|fsStruckOut)) {
		int line_width = 1;
		COLORREF fg = sys rq_pen.logpen.lopnColor;
		line_width = var font. underlineThickness;
		if ( use_alpha ) {
			gppen = stylus_gp_get_pen(line_width,
				( sys alpha << 24) |
				 (fg >> 16) |
				 (fg & 0xff00) |
				((fg & 0xff) << 16)
			);
		} else {
			SelectObject( dc, stylus_get_pen(PS_SOLID, line_width, fg));
			STYLUS_FREE_PEN;
		}
	}

	if ( var font. style & fsUnderlined ) {
		int i, Y = 0;
		Point pt[2];

		if ( !is_apt( aptTextOutBaseline))
			Y -= var font. descent;
		Y -= var font. underlinePosition;

		pt[0].x = 0;
		pt[0].y = -Y;
		pt[1].x = width;
		pt[1].y = -Y;
		if ( var font. direction != 0) {
			for ( i = 0; i < 2; i++) {
				float x = pt[i].x * cs.y - pt[i].y * cs.x;
				float y = pt[i].x * cs.x + pt[i].y * cs.y;
				pt[i].x = x + (( x > 0) ? 0.5 : -0.5);
				pt[i].y = y + (( y > 0) ? 0.5 : -0.5);
			}
		}

		if ( use_alpha ) {
			GdipDrawLineI(sys graphics, gppen, x2 + pt[0].x, y2 - pt[0].y, x2 + pt[1].x, y2 - pt[1].y);
		} else {
			MoveToEx( dc, x1 + pt[0].x, y1 - pt[0].y, NULL);
			LineTo( dc, x1 + pt[1].x, y1 - pt[1].y);
		}
	}

	if ( var font. style & fsStruckOut ) {
		int i, Y = 0;
		Point pt[2];

		if ( !is_apt( aptTextOutBaseline))
			Y -= var font. descent;
		if (sys otmsStrikeoutSize > 0)
			Y -= sys otmsStrikeoutPosition;
		else
			Y -= (var font. ascent - var font. internalLeading) / 3;

		pt[0].x = 0;
		pt[0].y = -Y;
		pt[1].x = width;
		pt[1].y = -Y;
		if ( var font. direction != 0) {
			for ( i = 0; i < 2; i++) {
				float x = pt[i].x * cs.y - pt[i].y * cs.x;
				float y = pt[i].x * cs.x + pt[i].y * cs.y;
				pt[i].x = x + (( x > 0) ? 0.5 : -0.5);
				pt[i].y = y + (( y > 0) ? 0.5 : -0.5);
			}
		}

		if ( use_alpha ) {
			GdipDrawLineI(sys graphics, gppen, x2 + pt[0].x, y2 - pt[0].y, x2 + pt[1].x, y2 - pt[1].y);
		} else {
			MoveToEx( dc, x1 + pt[0].x, y1 - pt[0].y, NULL);
			LineTo( dc, x1 + pt[1].x, y1 - pt[1].y);
		}
	}
}

void
gp_get_text_widths( Handle self, const char* text, int len, int flags, ABC * extents)
{
	SIZE  sz;
	int   div, offset = 0, ret = 0;

	objCheck;
	memset(extents, 0, sizeof(ABC));
	if ( len == 0) return;

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
	extents->abcB = ret;

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
			extents->abcA = abc[0].abcA;
			extents->abcC = abc[1].abcC;
		}
	}
}

static void
gp_get_polyfont_widths( Handle self, const PGlyphsOutRec t, int flags, ABC * extents)
{
	int div, len, offset = 0;
	FontContext fc;

	objCheck;
	memset(extents, 0, sizeof(ABC));
	if ( t->len == 0) return;

	font_context_init(&fc, self, t);

	if ( t->advances ) {
		int i;
		ABC abc;
		for ( i = 0; i < t->len; i++)
			extents->abcB += t->advances[i];
		GetCharABCWidthsI( sys ps, t->glyphs[0], 1, NULL, &abc);
		extents->abcA = abc.abcA;
		if ( t->fonts[0] != t->fonts[t->len - 1] ) {
			font_context_rewind(&fc, t->len - 1);
			font_context_next(&fc);
		}
		GetCharABCWidthsI( sys ps, t->glyphs[t->len-1], 1, NULL, &abc);
		extents->abcC = abc.abcC;
		font_context_done(&fc);
		return;
	}

	/* width more that 32K returned incorrectly by Win32 core */
	if (( div = 32768L / ( var font. maximalWidth ? var font. maximalWidth : 1)) == 0)
		div = 1;

	while (( len = font_context_next(&fc)) > 0 ) {
		ABC abc;
		SIZE sz;
		int local_offset = offset;
		while ( local_offset < len) {
			int chunk_len = ( local_offset + div > len) ? ( len - local_offset) : div;
			if ( !GetTextExtentPointI( sys ps, ( WCHAR*) t->glyphs + local_offset, chunk_len, &sz)) apiErr;
			local_offset += div;
			extents-> abcB += sz.cx;
		}
		if ( offset == 0 ) {
			GetCharABCWidthsI( sys ps, t->glyphs[0], 1, NULL, &abc);
			extents->abcA = abc.abcA;
		} else if ( fc.stop ) {
			GetCharABCWidthsI( sys ps, t->glyphs[len-1], 1, NULL, &abc);
			extents->abcC = abc.abcC;
		}
		offset += len;
	}
	font_context_done(&fc);
}

static int
gp_get_text_width( Handle self, const char* text, int len, int flags)
{
	ABC abc;
	gp_get_text_widths(self,text,len,flags,&abc);
	if ( flags & toAddOverhangs ) {
		if ( abc.abcA < 0) abc.abcB -= abc.abcA;
		if ( abc.abcC < 0) abc.abcB -= abc.abcC;
	}
	return abc.abcB;
}

static int
gp_get_glyphs_width( Handle self, PGlyphsOutRec t, int flags)
{
	ABC abc;
	if ( t-> fonts )
		gp_get_polyfont_widths(self,t,flags,&abc);
	else
		gp_get_text_widths( self, (const char*)t->glyphs, t->len, t->flags | toGlyphs, &abc);
	if ( abc.abcA < 0) abc.abcB -= abc.abcA;
	if ( abc.abcC < 0) abc.abcB -= abc.abcC;
	return abc.abcB;
}

static void
paint_text_background( Handle self, const char * text, int len, int flags)
{
	Point p[5];
	ABC abc;
	uint32_t *palette;
	POINT pp[5];
	HGDIOBJ  o1, o2;
	int rop;

	palette = sys alpha_arena_palette;
	sys alpha_arena_palette = NULL;

	if ( flags & toGlyphs) {
		PGlyphsOutRec t = (PGlyphsOutRec) text;
		if ( t-> fonts )
			gp_get_polyfont_widths(self,t,toAddOverhangs,&abc);
		else
			gp_get_text_widths( self, (const char*)t->glyphs, t->len, t->flags | toGlyphs | toAddOverhangs, &abc);
	} else {
		gp_get_text_widths(self, text, len, flags | toAddOverhangs, &abc);
	}

	gp_get_text_box(self, &abc, p);
	pp[0].x =  p[0].x;
	pp[0].y = -p[0].y;
	pp[1].x =  p[1].x;
	pp[1].y = -p[1].y;
	pp[2].x =  p[3].x;
	pp[2].y = -p[3].y;
	pp[3].x =  p[2].x;
	pp[3].y = -p[2].y;
	pp[4].x =  p[0].x;
	pp[4].y = -p[0].y;

	rop = GetROP2(sys ps);
	o1 = SelectObject( sys ps, std_hollow_pen);
	if ( rop != R2_COPYPEN )
		SetROP2( sys ps, R2_COPYPEN );
	o2 = SelectObject( sys ps, stylus_get_solid_brush(sys bg));
	Polygon( sys ps, pp, 5 );
	SelectObject( sys ps, o1 );
	SelectObject( sys ps, o2 );
	if ( rop != R2_COPYPEN )
		SetROP2( sys ps, rop );

	sys alpha_arena_palette = palette;
}


Bool
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, int flags )
{objCheck false;{
	Bool ok = true;
	HDC ps = sys ps;
	int bk  = GetBkMode( ps);
	int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
	Bool use_path, use_alpha;

	int div = 32768L / (var font. maximalWidth ? var font. maximalWidth : 1);
	if ( div <= 0) div = 1;
	/* Win32 has problems with text_out strings that are wider than
	32K pixel - it doesn't plot the string at all. This hack is
	although ugly, but is better that Win32 default behavior, and
	at least can be excused by the fact that all GP spaces have
	their geometrical limits. */
	if ( len > div) len = div;

	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) return false;
		len = mb_len;
	}

	select_world_transform(self, true);
	SHIFT_XY(x, y);
	SetViewportOrgEx( sys ps, x, y, NULL );

	use_alpha = sys alpha < 255;
	use_path = (GetROP2( sys ps) != R2_COPYPEN) && !use_alpha;
	if ( use_path ) {
		STYLUS_USE_BRUSH;
		BeginPath(ps);
	} else if ( !use_alpha ) {
		STYLUS_USE_TEXT;
		if ( opa != bk) SetBkMode( ps, opa);
	}

	if ( use_alpha ) {
		if ( is_apt( aptTextOpaque))
			paint_text_background(self, (char*)text, len, flags & toUTF8);
		ok = aa_text_out( self, 0, 0, (void*)text, len, flags & toUTF8);
	} else {
		ok = ( flags & toUTF8 ) ?
			TextOutW( ps, 0, 0, ( U16*)text, len) :
			TextOutA( ps, 0, 0, text, len);
		if ( !ok ) apiErr;
	}

	if ( var font. style & (fsUnderlined | fsStruckOut))
		underscore_font( self, 0, 0, x, y, gp_get_text_width( self, text, len, flags), use_alpha);

	if ( use_path ) {
		EndPath(ps);
		FillPath(ps);
	} else if ( !use_alpha ) {
		if ( opa != bk) SetBkMode( ps, bk);
	}

	if ( flags & toUTF8 ) free(( char *) text);
	return ok;
}}

/*

It seems that Windows decidedly doesn't shape combining characters, and
possibly doesn't kerning/ligatures in general for fixed width fonts. The two
functions, fix_combiners_pdx and fix_combiners_advances try to fix for this.

It isn't clear whether this is Windows, or mono fonts in general, because
Courier New doesn't do that under x11/fontconfig, either, hinting at a very
special GSUB/GPOS setup there. This causes the addition of
MAPPER_FLAGS_COMBINING_SUPPORTED flag so that polyfont handler doesn't try to
run combiners on these fonts

*/
static void
fix_combiners_pdx( Handle self, PGlyphsOutRec t, INT *pdx)
{
	int i;
	for ( i = 0; i < t->len; ) {
		int j, cluster_length, cluster_start;
		int first_glyph_width;

		cluster_start = i++;
		for ( cluster_length = 1; i < t->len; ) {
			if ( t->advances[i] > 0 )
				break;
			cluster_length++;
			i++;
		}

		if ( cluster_length == 1 )
			continue;

		first_glyph_width = t->advances[cluster_start];
		for ( j = 1; j < cluster_length; j++)
			pdx[(cluster_start + j - 1) * 2] -= first_glyph_width * j;
		pdx[(cluster_start + j - 1) * 2] += first_glyph_width * (j - 1);
	}
}

static Bool
gp_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y, int * text_advance, Bool fixed_pitch)
{
	Bool ok;
	if ( t-> advances ) {
		#define SZ 1024
		INT dx[SZ], i, n, *pdx;
		int16_t *goffsets = t->positions;
		uint16_t *advances = t->advances;

		if ( text_advance ) *text_advance = 0;

		n = t-> len * 2;
		if ( n > SZ) {
			if ( !( pdx = malloc(sizeof(INT) * n)))
				pdx = dx;
		} else
			pdx = dx;

		for ( i = 0; i < n; i += 2) {
			int gx     = *(goffsets++);
			int gy     = *(goffsets++);
			int adv    = *(advances++);
			pdx[i]     = adv;
			pdx[i + 1] = 0;
			if ( text_advance )
				*text_advance += adv;

			if ( i == 0 ) {
				x += gx;
				y -= gy;
			} else {
				pdx[i - 2] += gx;
				pdx[i - 1] += gy;
			}
			pdx[i]     -= gx;
			pdx[i + 1] -= gy;
		}

		if ( fixed_pitch )
			fix_combiners_pdx(self, t, pdx);
		ok = ExtTextOutW(sys ps, x, y, ETO_GLYPH_INDEX | ETO_PDY, NULL, (LPCWSTR) t->glyphs, t->len, pdx);
		if ( pdx != dx ) free(pdx);
		#undef SZ
	} else {
		ok = ExtTextOutW(sys ps, x, y, ETO_GLYPH_INDEX, NULL, (LPCWSTR) t->glyphs, t->len, NULL);
		if ( text_advance ) {
			SIZE sz;
			if ( !GetTextExtentPointI( sys ps, (WCHAR*) t->glyphs, t->len, &sz)) apiErr;
			*text_advance += sz.cx;
		}
	}
	if ( !ok ) apiErr;
	return ok;
}

Bool
apc_gp_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y)
{objCheck false;{
	Bool ok = true;
	HDC ps = sys ps;
	int xx, yy, savelen;
	int bk  = GetBkMode( ps);
	int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
	Bool use_path, use_alpha;
	FontContext fc;
	float fxx, fyy;
	NPoint cs = CDrawable(self)->trig_cache(self);

	select_world_transform(self, true);
	SHIFT_XY(x,y);
	SetViewportOrgEx( sys ps, x, y, NULL );

	if ( t->len > 8192 ) t->len = 8192;
	use_path = GetROP2( sys ps) != R2_COPYPEN;
	use_alpha = sys alpha < 255;
	if ( use_path ) {
		STYLUS_USE_BRUSH;
		BeginPath(ps);
	} else if ( use_alpha ) {
		if ( is_apt( aptTextOpaque))
			paint_text_background(self, (char*) t, 0, toGlyphs);
	} else {
		STYLUS_USE_TEXT;
		if ( opa != bk) SetBkMode( ps, opa);
	}

	fxx = xx = 0;
	fyy = yy = 0;
	savelen = t->len;
	font_context_init(&fc, self, t);
	while (( t-> len = font_context_next(&fc)) > 0 ) {
		int advance = 0;
		if ( !( ok = use_alpha ?
				aa_glyphs_out(self, t, xx, yy, fc.stop ? NULL : &advance, fc.curr) :
				gp_glyphs_out(self, t, xx, yy, fc.stop ? NULL : &advance, fc.fixed_pitch)
			))
			break;
		if ( !fc.stop ) {
			fxx += (float)advance * cs.y;
			fyy -= (float)advance * cs.x;
			xx = fxx + ((fxx < 0) ? -.5 : +.5);
			yy = fyy + ((fyy < 0) ? -.5 : +.5);

			t->glyphs    += t->len;
			if ( t-> advances ) {
				t->advances  += t->len;
				t->positions += t->len * 2;
			}
		}
	}
	font_context_done(&fc);
	t->len = savelen;

	if ( var font. style & (fsUnderlined | fsStruckOut))
		underscore_font( self, 0, yy, x, y + yy, gp_get_glyphs_width( self, t, 0), use_alpha);

	if ( use_path ) {
		EndPath(ps);
		FillPath(ps);
	} else if ( !use_alpha ) {
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
	static char last_lang[LANGBUF + 1] = "";
	static WORD last_langid = MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);

	if ( strncmp( lang, last_lang, LANGBUF) == 0) return last_langid;
	last_langid = make_langid(lang);
	strlcpy( last_lang, lang, LANGBUF);
	return last_langid;
#undef LANGBUF
}

#pragma pack(1)
typedef struct {
	HFONT font;
	DWORD script;
} ScriptCacheKey;
#pragma pack()


/* convert UTF-32 to UTF-16, mark surrogates */
static Bool
build_wtext( PTextShapeRec t,
	WCHAR * wtext, unsigned int * wlen,
	unsigned int * first_surrogate_pair, unsigned int ** surrogate_map)
{
	int i;
	unsigned int index = 0, *curr = NULL;
	uint32_t *src;
	WCHAR *dst;

	for ( i = *wlen = 0, src = t->text, dst = wtext; i < t->len; i++, index++) {
		uint32_t c = *src++;
		if ( c >= 0x10000 && c <= 0x10FFFF ) {
			c -= 0x10000;
			*(dst++) = 0xd800 + (c >> 10);
			*(dst++) = 0xdc00 + (c & 0x3ff);
			if ( !*surrogate_map ) {
				*first_surrogate_pair = i;
				*surrogate_map = malloc(sizeof(unsigned int*) * (t-> len - i) * 2);
				if ( !*surrogate_map ) return false;
				curr = *surrogate_map;
			}
			*(curr++) = index;
			*(curr++) = index;
			*wlen += 2;
		} else {
			if (( c >= 0xD800 && c <= 0xDFFF ) || ( c > 0x10FFFF )) c = 0;
			if ( *surrogate_map )
				*(curr++) = index;
			*(dst++) = c;
			(*wlen)++;
		}
	}

	return true;
}

static unsigned int
fill_null_glyphs( 
	PTextShapeRec t, 
	unsigned int char_pos, unsigned int itemlen, 
	unsigned int * surrogate_map, unsigned int first_surrogate_pair,
	uint16_t advance
) {
	int i, nglyphs;
	if ( surrogate_map ) {
		int p1 = char_pos;
		int p2 = p1 + itemlen - 1;
		if ( p1 >= first_surrogate_pair ) p1 = surrogate_map[p1 - first_surrogate_pair];
		if ( p2 >= first_surrogate_pair ) p2 = surrogate_map[p2 - first_surrogate_pair];
		nglyphs = p2 - p1 + 1;
	} else
		nglyphs = itemlen;
#ifdef _DEBUG
	printf("%d null glyphs, indexes: %x\n", nglyphs, t->indexes + t->n_glyphs);
#endif
	bzero( t->glyphs + t->n_glyphs, sizeof(uint16_t) * nglyphs);
	for ( i = 0; i < nglyphs; i++)
		t->indexes[ t->n_glyphs + i ] = i + char_pos;
	if ( t->advances ) {
		for ( i = 0; i < nglyphs; i++)
			t->advances[t-> n_glyphs + i] = advance;
		bzero( t->positions + t->n_glyphs * 2, sizeof(uint16_t) * nglyphs * 2);
	}
	t-> n_glyphs += nglyphs;
	return nglyphs;
}

static void
convert_indexes( Bool rtl, unsigned int char_pos, unsigned int itemlen, unsigned int nglyphs, WORD * indexes, uint16_t * out_indexes)
{
	int j, last_glyph, last_char;
	if (rtl) {
		for ( j = last_char = 0, last_glyph = nglyphs - 1; j < itemlen; j++) {
			int k, textlen = 1, glyphlen = last_glyph + 1;
			WORD curr_glyph = indexes[j];
			last_char = j;
			for ( k = j + 1; k < itemlen; k++) {
				if ( indexes[k] == curr_glyph )
					textlen++;
				else {
					glyphlen = curr_glyph - indexes[k];
					break;
				}
			}
			for ( k = 0; k < glyphlen; k++)
				out_indexes[last_glyph--] = j + char_pos;
			j += textlen - 1;
		}
	} else {
		for ( j = last_char = last_glyph = 0; j < itemlen; j++) {
			int k, textlen = 1, glyphlen = nglyphs - last_glyph;
			WORD curr_glyph = indexes[j];
			last_char = j;
			for ( k = j + 1; k < itemlen; k++) {
				if ( indexes[k] == curr_glyph )
					textlen++;
				else {
					glyphlen = indexes[k] - curr_glyph;
					break;
				}
			}
			for (k = 0; k < glyphlen; k++)
				out_indexes[last_glyph++] = j + char_pos;
			j += textlen - 1;
		}
	}
}

/* see explanation in fix_combiners_pdx */
static void
fix_combiners_advances( Handle self, PTextShapeRec t, int nglyphs)
{
	int i;
	for ( i = 0; i < nglyphs; ) {
		int j, cluster_length, cluster_glyph_start;
		int cluster_text_start = t->indexes[t->n_glyphs + i];

		cluster_glyph_start = i++;
		for ( cluster_length = 1; i < nglyphs; ) {
			if ( t->indexes[t->n_glyphs + i] != cluster_text_start )
				break;
			cluster_length++;
			i++;
		}

		if ( cluster_length == 1 )
			continue;

		for ( j = 1; j < cluster_length; j++) {
			uint32_t c = t->text[t->n_glyphs + cluster_text_start + j];
			if ( c < 0x300 || c > 0x36f )
				continue;
			t->advances[t->n_glyphs + cluster_glyph_start + j] = 0;
		}
	}
}

static Bool
win32_unicode_shaper( Handle self, PTextShapeRec t)
{
	Bool ok = false;
	HRESULT hr;
	WCHAR * wtext = NULL;
	uint16_t * out_indexes, default_advance = 0;
	int i, item, item_step, nitems;
	SCRIPT_CONTROL control;
	SCRIPT_ITEM *items = NULL;
	WORD *indexes = NULL;
	SCRIPT_VISATTR *visuals = NULL;
	int *advances = NULL;
	GOFFSET *goffsets = NULL;
	unsigned int * surrogate_map = NULL, first_surrogate_pair = 0, wlen;

	if ((items = malloc(sizeof(SCRIPT_ITEM) * (t-> len + 1))) == NULL)
		goto EXIT;
	if ((indexes = malloc(sizeof(WORD) * t-> n_glyphs_max)) == NULL)
		goto EXIT;
	if ((visuals = malloc(sizeof(SCRIPT_VISATTR) * t-> n_glyphs_max)) == NULL)
		goto EXIT;
	if ((wtext = malloc(sizeof(WCHAR) * 2 * t->len)) == NULL)
		goto EXIT;
	if ((advances = malloc(sizeof(int) * t->n_glyphs_max)) == NULL)
		goto EXIT;
	if ((goffsets = malloc(sizeof(GOFFSET) * t->n_glyphs_max)) == NULL)
		goto EXIT;

	build_wtext( t, wtext, &wlen, &first_surrogate_pair, &surrogate_map);

	bzero(&control, sizeof(control));
	control.uDefaultLanguage = t->language ?
		langid(t->language) :
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	if ((hr = ScriptItemize(wtext, wlen, t->n_glyphs_max, &control, NULL, items, &nitems)) != S_OK) {
		apiHErr(hr);
		goto EXIT;
	}
#ifdef _DEBUG
	printf("itemizer: %d\n", nitems);
#endif

	if ( t->flags & toRTL ) {
		item = nitems - 1;
		item_step = -1;
	} else {
		item = 0;
		item_step = 1;
	}

	for (
		i = 0, out_indexes = t-> indexes;
		i < nitems;
		i++, item += item_step
	) {
		int j, itemlen, nglyphs;

		SCRIPT_CACHE * script_cache;
		ScriptCacheKey key = { sys dc_font->hfont, items[item].a.eScript };
		if (( script_cache = ( SCRIPT_CACHE*) hash_fetch( mgr_scripts, &key, sizeof(key))) == NULL) {
			if ((script_cache = malloc(sizeof(SCRIPT_CACHE))) == NULL)
				goto EXIT;
			*script_cache = NULL;
			hash_store( mgr_scripts, &key, sizeof(key), script_cache);
		}

		itemlen = items[item+1].iCharPos - items[item].iCharPos;
		items[item].a.fRTL = (t->flags & toRTL) ? 1 : 0;
		//printf("shape(%d @ %d) len %d %s\n", item, items[item].iCharPos, itemlen, items[item].a.fRTL ? "RTL" : "LTR");
		if (( hr = ScriptShape(
			sys ps, script_cache,
			wtext + items[item].iCharPos, itemlen, t->n_glyphs_max,
			&items[item].a,
			t->glyphs + t->n_glyphs, indexes, visuals,
			&nglyphs
		)) != S_OK) {
			if ( hr == USP_E_SCRIPT_NOT_IN_FONT) {
#ifdef _DEBUG
				printf("USP_E_SCRIPT_NOT_IN_FONT\n");
#endif
				if ( default_advance == 0 ) {
					ABC abc;
					if ( GetCharABCWidthsI( sys ps, 0, 1, NULL, &abc)) 
						default_advance = abc.abcA + abc.abcB + abc.abcC;
					if ( default_advance <= 0 )
						default_advance = 1;
				}
				out_indexes += fill_null_glyphs(t, items[item].iCharPos, itemlen, surrogate_map, first_surrogate_pair, default_advance);
				continue;
			}
			apiHErr(hr);
			goto EXIT;
		}
		convert_indexes( items[item].a.fRTL, items[item].iCharPos, itemlen, nglyphs, indexes, out_indexes);
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
				printf("%d(%x) ", indexes[i], t->glyphs[t->n_glyphs + i]);
			}
			printf("\n");
			printf("indexes: ");
			for ( i = 0; i < nglyphs; i++) {
				printf("%d ", out_indexes[i]);
			}
			printf("\n");
		}
#endif

		/* map from utf16 */
		if ( surrogate_map ) {
			int k;
			uint16_t * out_glyphs = t-> glyphs + t-> n_glyphs;
			for ( j = k = 0; j < nglyphs; j++, k++) {
				if ( k < j)
					out_glyphs[k] = out_glyphs[j];
				if ( out_indexes[j] >= first_surrogate_pair)
					out_indexes[k] = surrogate_map[out_indexes[j] - first_surrogate_pair];
				else if ( k < j )
					out_indexes[k] = out_indexes[j];
			}
		}

		if ( t-> advances ) {
			GOFFSET * i_g;
			int * i_a, i;
			ABC abc;
			int16_t * o_g;
			uint16_t *o_a;

			if (( hr = ScriptPlace(sys ps, script_cache, t->glyphs + t->n_glyphs, nglyphs,
			       visuals, &items[item].a,
			       advances, goffsets, &abc)) != S_OK
			) {
				apiHErr(hr);
				goto EXIT;
			}
			for (
				i = 0,
				i_a = advances,
				i_g = goffsets,
				o_a = t->advances  + t-> n_glyphs,
				o_g = t->positions + t-> n_glyphs * 2;
				i < nglyphs;
				i++
			) {
				*(o_a++) = *(i_a++);
				*(o_g++) = i_g->du;
				*(o_g++) = i_g->dv;
				i_g++;
			}
			if ( !(sys tmPitchAndFamily & TMPF_FIXED_PITCH ))
				fix_combiners_advances(self, t, nglyphs);
		}

		t-> n_glyphs += nglyphs;
		out_indexes += nglyphs;
	}

	ok = true;

EXIT:
	if ( surrogate_map  ) free(surrogate_map);
	if ( goffsets ) free(goffsets);
	if ( advances ) free(advances);
	if ( indexes )  free(indexes);
	if ( visuals  ) free(visuals);
	if ( items    ) free(items);
	if ( wtext    ) free(wtext);
	return ok;
}

static Bool
win32_mapper( Handle self, PTextShapeRec t, Bool unicode)
{
	int i, len = t->len;
	uint32_t *src = t-> text;
	uint16_t *glyphs = t->glyphs;
	INT buf[8192];
	DWORD ret;

	if ( len > 8192 ) len = 8192;

	if ( unicode ) {
		WCHAR *dst = (WCHAR*) buf;
		for ( i = 0; i < t->len; i++)
			*(dst++) = *(src++);
		ret = GetGlyphIndicesW( sys ps, (LPCWSTR)buf, t->len, t->glyphs, GGI_MARK_NONEXISTING_GLYPHS);
	} else {
		char *dst = (char*) buf;
		for ( i = 0; i < t->len; i++)
			*(dst++) = *(src++);
		ret = GetGlyphIndicesA( sys ps, (LPCSTR)buf, t->len, t->glyphs, GGI_MARK_NONEXISTING_GLYPHS);
	}
	if ( ret == GDI_ERROR)
		apiErrRet;
	t-> n_glyphs = ret;
	for ( i = 0; i < t->n_glyphs; i++, glyphs++)
		if (*glyphs == 0xffff) *glyphs = 0;

	if ( t->advances ) {
		INT *widths = buf;
		uint16_t *advances = t->advances;
		bzero(t->positions, t->n_glyphs * sizeof(int16_t) * 2);
		if ( GetCharWidthI(sys ps, 0, t->n_glyphs, t->glyphs, buf) == 0)
			apiErrRet;
		for ( i = 0; i < t->n_glyphs; i++, widths++)
			*(advances++) = (*widths >= 0) ? *widths : 0;
	}

	return true;
}

static Bool
win32_byte_mapper( Handle self, PTextShapeRec t)
{
	return win32_mapper(self, t, false);
}

static Bool
win32_unicode_mapper( Handle self, PTextShapeRec t)
{
	return win32_mapper(self, t, true);
}

PTextShapeFunc
apc_font_get_text_shaper( Handle self, int * type)
{
	if ( *type == tsBytes ) {
		*type = (sys tmPitchAndFamily & (TMPF_TRUETYPE|TMPF_OPENTYPE)) ?
			tsGlyphs :
			tsNone;
		return win32_byte_mapper;
	} else if ( *type == tsGlyphs ) {
		*type = (sys tmPitchAndFamily & (TMPF_TRUETYPE|TMPF_OPENTYPE)) ?
			tsGlyphs :
			tsNone;
		return win32_unicode_mapper;
	} else {
		*type = (sys tmPitchAndFamily & (TMPF_TRUETYPE|TMPF_OPENTYPE)) ?
			tsFull :
			tsNone;
		return win32_unicode_shaper;
	}
}

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
	LOGFONTW logfont;
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
	hfont = CreateFontIndirectW( &logfont);
	oldFont = SelectObject( dc, hfont );

	memset( abc, 0, sizeof(FontABC) * (last - first + 1));
	for ( i = 0; i <= last - first; i++) {
		Rectangle( dc, -1, -1, w+1, h+1 );
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
{objCheck NULL;{
	int i;
	DWORD ret;
	PFontABC f1;
	MAT2 gmat = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
	GLYPHMETRICS g;

	f1 = ( PFontABC) malloc(( last - first + 1) * sizeof( FontABC));
	if ( !f1) return NULL;

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
				return NULL;
			}
			return f1;
		}
		f1[i]. a = var font. descent + g.gmptGlyphOrigin. y - g.gmBlackBoxY; /* XXX g.gmCellIncY ? */
		f1[i]. b = g.gmBlackBoxY;
		f1[i]. c = var font.ascent - g.gmptGlyphOrigin. y;
	}

	return f1;
}}

/*
 get_opentype_cmap1213_font_ranges is based on the following:

 wine:  dlls/dwrite/opentype.c
 	Copyright 2014 Aric Stewart for CodeWeavers

 pango: pangowin32.c (
 	Copyright (C) 1999 Red Hat Software
 	Copyright (C) 2000 Tor Lillqvist
 	Copyright (C) 2001 Alexander Larsson

 Thank you!
*/

#define MAKE_TT_TABLE_NAME(c1, c2, c3, c4) \
   (((DWORD)c4) << 24 | ((DWORD)c3) << 16 | ((DWORD)c2) << 8 | ((DWORD)c1))
#define CMAP (MAKE_TT_TABLE_NAME('c','m','a','p'))
#define CMAP_HEADER_SIZE 4
#define ENCODING_TABLE_SIZE 8
#define BE16(x) (((x&0xff00)>>8)|((x&0xff)<<8))
#define BE32(x) (((x&0xff000000)>>24)|((x&0xff0000)>>8)|((x&0xff00)<<8)|((x&0xff)<<24))

#pragma pack(1)
struct cmap_encoding_subtable
{
	WORD platform_id;
	WORD encoding_id;
	DWORD offset;
};
#pragma pack()

unsigned long *
get_opentype_cmap1213_font_ranges( HDC ps, int * count)
{
	static const uint16_t encodings[][2] = {
		{ 3, 0 }, /* MS Symbol encoding is preferred. */
		{ 3, 10 },
		{ 0, 6 },
		{ 0, 4 },
		{ 3, 1 },
		{ 0, 3 },
		{ 0, 2 },
		{ 0, 1 },
		{ 0, 0 },
	};

	uint16_t i, j, n_tables, format;
	uint32_t cmap_size, offset, n_groups, *groups;
	unsigned long * ret = NULL;
	struct cmap_encoding_subtable *table, *found_record;
	uint8_t *cmap = NULL;

	cmap_size = GetFontData(ps, CMAP, 0, NULL, 0);
	if ( cmap_size == 0 || cmap_size == GDI_ERROR)
		goto FAIL;
	if ( !( cmap = malloc(cmap_size))) {
		warn("Not enough memory");
		goto FAIL;
	}
	if (GetFontData(ps, CMAP, 0, cmap, cmap_size) != cmap_size)
		goto FAIL;
#define READ16(v,offset) \
	if (offset + 2 < cmap_size) \
		v = BE16( *((uint16_t*)(cmap + offset)) ); \
		else goto FAIL
#define READ32(v,offset) \
	if (offset + 4 < cmap_size) \
		v = BE32( *((uint32_t*)(cmap + offset)) ); \
		else goto FAIL
#define READPTR(v,offset,size) \
	if (offset + size <= cmap_size) \
		v = (void*)(cmap + offset); \
		else goto FAIL

	READ16(n_tables, 2);
	READPTR(table, CMAP_HEADER_SIZE, ENCODING_TABLE_SIZE * n_tables);
	for (i = 0, found_record = NULL; i < sizeof(encodings)/sizeof(uint16_t)/2; i++) {
		struct cmap_encoding_subtable *t;
		uint16_t *enc = (uint16_t*)( encodings + i );
		for ( j = 0, t = table; j < n_tables; j++, t++)
			if ( enc[0] == BE16(t->platform_id) && enc[1] == BE16(t->encoding_id)) {
				found_record = t;
				goto STOP;
			}
	}
	goto FAIL;
STOP:

	offset = BE32(found_record->offset);
	READ16(format, offset);
	/* don't implement logic for BMP planes as GetFontUnicodeRanges can retrieve it just fine */
	if ( format != 12 && format != 13 )
		return NULL;

	READ32(n_groups, offset + 12);
	READPTR(groups, offset + 16, n_groups * 3 * 4);

	if ( !( ret = malloc(n_groups * sizeof(unsigned long) * 2))) {
		warn("Not enough memory");
		goto FAIL;
	}
	for ( i = 0; i < n_groups; i++) {
		ret[(*count)++] = BE32(groups[i * 3]);
		ret[(*count)++] = BE32(groups[i * 3 + 1]);
	}

	free(cmap);
	return ret;

FAIL:
	if ( cmap ) free(cmap);
	if ( ret ) free(ret);
	*count = 0;
	return NULL;
}

#undef READPTR
#undef READ16
#undef READ32
#undef BE16
#undef BE32
#undef CMAP
#undef CMAP_HEADER_SIZE
#undef ENCODING_TABLE_SIZE
#undef MAKE_TT_TABLE_NAME

unsigned long *
get_font_ranges( HDC ps, int * count)
{
	DWORD i, j, size;
	GLYPHSET * gs;
	unsigned long * ret;
	WCRANGE *src;

	*count = 0;
	ret = get_opentype_cmap1213_font_ranges( ps, count );
	if ( ret != NULL ) return ret;

	if (( size = GetFontUnicodeRanges( ps, NULL )) == 0)
		return NULL;
	if (!( gs = malloc(size)))
		apiErrRet;
	bzero(gs, size);
	gs-> cbThis = size;
	if ( GetFontUnicodeRanges( ps, gs ) == 0) {
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
}

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{
	objCheck NULL;
	return get_font_ranges(sys ps, count);
}

unsigned long *
apc_gp_get_mapper_ranges(PFont font, int * count, unsigned int * flags)
{
	HDC dc;
	unsigned long * ret;
	char name[256];
	LOGFONTW logfont;
	HFONT hfont, hstock;

	*count = 0;

	strlcpy(name, font->name, 256);
	apc_font_pick( NULL_HANDLE, font, NULL);
	if ( strcmp( font->name, name ) != 0 ) 
		return NULL;

	font_font2logfont( font, &logfont);
	logfont.lfHeight = 0;
	logfont.lfWidth  = 0;
	if ( !( hfont = CreateFontIndirectW( &logfont))) {
		apiErr;
		return NULL;
	}

	if ( !( dc = dc_alloc())) {
		DeleteObject(hfont);
		return NULL;
	}

	hstock = SelectObject(dc, hfont);
	ret = get_font_ranges(dc, count);

	*flags = MAPPER_FLAGS_COMBINING_SUPPORTED;

	SelectObject(dc, hstock);
	dc_free();
	DeleteObject(hfont);

	return ret;
}

static char *
single_lang(const char * lang1)
{
	char * m;
	int l = strlen(lang1) + 1;
	m = malloc( l + 3 + 1 );
	strcpy( m, "en" );
	strcpy( m + 3, lang1 );
	m[l + 3] = 0;
	return m;
}

typedef struct {
	DWORD v[4];
	const char * str;
} LangId;

static LangId languages[] = {
	{
		{0x00000001,0x00000000,0x00000000,0x00000000},
		"fj ho ia ie io kj kwm ms ng nr om rn rw sn so ss st sw ts uz xh za zu"
	},{
		{0x00000003,0x00000000,0x00000000,0x00000000},
		"aa an ay bi br ch da de en es eu fil fo fur fy gd gl gv ht id is it jv lb li mg nb nds nl nn no oc pap-an pap-aw pt rm sc sg sma smj sq su sv tl vo wa yap"
	},{
		{0x00000007,0x00000000,0x00000000,0x00000000},
		"af ca co crh cs csb et fi fr hsb hu kl ku-tr mt na nso pl se sk smn tk tn tr vot wen wo"
	},{
		{0x20000007,0x00000000,0x00000000,0x00000000},
		"cy ga gn"
	},{
		{0x0000000f,0x00000000,0x00000000,0x00000000},
		"ro"
	},{
		{0x0000001f,0x00000000,0x00000000,0x00000000},
		"az-az sms"
	},{
		{0x0000005f,0x00000000,0x00000000,0x00000000},
		"ee ln"
	},{
		{0x2000005f,0x00000000,0x00000000,0x00000000},
		"ak fat tw"
	},{
		{0x0000006f,0x00000000,0x00000000,0x00000000},
		"nv"
	},{
		{0x2000004f,0x00000000,0x00000000,0x00000000},
		"vi yo"
	},{
		{0x0000020f,0x00000000,0x00000000,0x00000000},
		"mo"
	},{
		{0x00000027,0x00000000,0x00000000,0x00000000},
		"ty"
	},{
		{0x20000003,0x00000000,0x00000000,0x00000000},
		"ast"
	},{
		{0x00000023,0x00000000,0x00000000,0x00000000},
		"qu quz"
	},{
		{0x00000043,0x00000000,0x00000000,0x00000000},
		"shs"
	},{
		{0x20000043,0x00000000,0x00000000,0x00000000},
		"bin"
	},{
		{0x00000005,0x00000000,0x00000000,0x00000000},
		"bs eo hr ki la lg lt lv mh ny sl"
	},{
		{0x20000005,0x00000000,0x00000000,0x00000000},
		"mi"
	},{
		{0x0000000d,0x00000000,0x00000000,0x00000000},
		"kw"
	},{
		{0x0000001d,0x00000000,0x00000000,0x00000000},
		"bm ff"
	},{
		{0x2000001d,0x00000000,0x00000000,0x00000000},
		"ber-dz kab"
	},{
		{0x00000025,0x00000000,0x00000000,0x00000000},
		"haw"
	},{
		{0x00000205,0x00000000,0x00000000,0x00000000},
		"sh"
	},{
		{0x20000001,0x00000000,0x00000000,0x00000000},
		"ig ve"
	},{
		{0x00000009,0x00000000,0x00000000,0x00000000},
		"kr"
	},{
		{0x00000019,0x00000000,0x00000000,0x00000000},
		"ha sco"
	},{
		{0x00000021,0x00000000,0x00000000,0x00000000},
		"sm to"
	},{
		{0x20000041,0x00000000,0x00000000,0x00000000},
		"hz"
	},{
		{0x00000400,0x00000000,0x00000000,0x00000000},
		"hy"
	},{
		{0x00000800,0x00000000,0x00000000,0x00000000},
		"he yi"
	},{
		{0x00002000,0x00000000,0x00000000,0x00000000},
		"ar az-ir fa ks ku-iq ku-ir lah ota pa-pk pes prs ps-af ps-pk sd ug ur"
	},{
		{0x00004000,0x00000000,0x00000000,0x00000000},
		"nqo"
	},{
		{0x00008000,0x00000000,0x00000000,0x00000000},
		"bh bho brx doi hi hne kok mai mr ne sa sat"
	},{
		{0x00018000,0x00000000,0x00000000,0x00000000},
		"mni"
	},{
		{0x00010000,0x00000000,0x00000000,0x00000000},
		"as bn"
	},{
		{0x00020000,0x00000000,0x00000000,0x00000000},
		"pa"
	},{
		{0x00040000,0x00000000,0x00000000,0x00000000},
		"gu"
	},{
		{0x00080000,0x00000000,0x00000000,0x00000000},
		"or"
	},{
		{0x00000204,0x00000000,0x00000000,0x00000000},
		"cv"
	},{
		{0x00100000,0x00000000,0x00000000,0x00000000},
		"ta"
	},{
		{0x00200000,0x00000000,0x00000000,0x00000000},
		"te"
	},{
		{0x00400000,0x00000000,0x00000000,0x00000000},
		"kn"
	},{
		{0x00800000,0x00000000,0x00000000,0x00000000},
		"ml"
	},{
		{0x01000000,0x00000000,0x00000000,0x00000000},
		"th"
	},{
		{0x02000000,0x00000000,0x00000000,0x00000000},
		"lo"
	},{
		{0x04000000,0x00000000,0x00000000,0x00000000},
		"ka"
	},{
		{0x00000000,0x08070000,0x00000000,0x00000000},
		"ja"
	},{
		{0x00000000,0x08010000,0x00000000,0x00000000},
		"zh-hk zh-mo"
	},{
		{0x00000020,0x08000000,0x00000000,0x00000000},
		"zh-cn zh-sg"
	},{
		{0x00000000,0x01100000,0x00000000,0x00000000},
		"ko"
	},{
		{0x00000000,0x28000000,0x00000000,0x00000000},
		"zh-tw"
	},{
		{0x00000080,0x00000000,0x00000000,0x00000000},
		"el"
	},{
		{0x00000000,0x00000000,0x00000040,0x00000000},
		"bo dz"
	},{
		{0x00000000,0x00000000,0x00000080,0x00000000},
		"syr"
	},{
		{0x00000000,0x00000000,0x00000100,0x00000000},
		"dv"
	},{
		{0x00000000,0x00000000,0x00000200,0x00000000},
		"si"
	},{
		{0x00000000,0x00000000,0x00000400,0x00000000},
		"my"
	},{
		{0x00000000,0x00000000,0x00000800,0x00000000},
		"am byn gez sid ti-er ti-et tig wal"
	},{
		{0x00000000,0x00000000,0x00001000,0x00000000},
		"chr"
	},{
		{0x00000000,0x00000000,0x00002000,0x00000000},
		"iu"
	},{
		{0x00000000,0x00000000,0x00010000,0x00000000},
		"km"
	},{
		{0x00000000,0x00000000,0x00020000,0x00000000},
		"mn-cn"
	},{
		{0x00000000,0x00000000,0x00080000,0x00000000},
		"ii"
	},{
		{0x00000200,0x00000000,0x00000000,0x00000000},
		"ab av ba be bg bua ce chm cu ik kaa kk ku-am kum kv ky lez mk mn-mn os ru sah sel sr tg tt tyv uk"
	},{
		{0x00000000,0x00000000,0x00000000,0x00000004},
		"ber-ma"
	}
};

char *
apc_gp_get_font_languages( Handle self)
{objCheck NULL;{
	int i, size;
	char * ret, * p;
	FONTSIGNATURE f;
	LangId *lang;

	memset( &f, 0, sizeof(f));
	i = GetTextCharsetInfo( sys ps, &f, 0);
	if ( i == DEFAULT_CHARSET)
		apiErrRet;

	if ( f. fsUsb[0] == 0 && f. fsUsb[1] == 0 && f. fsUsb[2] == 0 && f. fsUsb[3] == 0) {
		switch( i ) {
		case SYMBOL_CHARSET      : return NULL;
		case SHIFTJIS_CHARSET    : return single_lang("ja");
		case HANGEUL_CHARSET     :
		case GB2312_CHARSET      :
		case CHINESEBIG5_CHARSET : return single_lang("zh");
#ifdef JOHAB_CHARSET
		case GREEK_CHARSET       : return single_lang("el");
		case HEBREW_CHARSET      : return single_lang("he");
		case ARABIC_CHARSET      : return single_lang("ar");
		case VIETNAMESE_CHARSET  : return single_lang("vi");
		case THAI_CHARSET        : return single_lang("th");
		case RUSSIAN_CHARSET     : return single_lang("ru");
#endif
		}
		return single_lang("");
	}

	size = 1024;
	if ( !( p = ret = malloc( size )))
		return NULL;
	for ( i = 0, lang = languages; i < sizeof(languages)/sizeof(LangId); i++, lang++) {
		DWORD *a = f.fsUsb;
		DWORD *b = lang->v;
		if (
			((a[0] & b[0]) == b[0]) &&
			((a[1] & b[1]) == b[1]) &&
			((a[2] & b[2]) == b[2]) &&
			((a[3] & b[3]) == b[3])
		) {
			int len = strlen(lang->str) + 1;
			if ( p - ret + len + 1 > size ) {
				char * p2;
				size *= 2;
				if ( !( p2 = realloc(p, size))) {
					free(ret);
					return NULL;
				}
				p   = p2 + (p - ret);
				ret = p2;
			}
			strcpy( p, lang->str );
			p += len;
		}
	}
	*p = 0;

	while ( p > ret ) {
		if (*p == ' ') *p = 0;
		p--;
	}

	return ret;
}}

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
	return gp_get_glyphs_width( self, t, t->flags);
}

void
gp_get_text_box( Handle self, ABC * abc, Point * pt)
{
	Point ovx = { abc->abcA, abc->abcC };
	Drawable_calculate_text_box( self, abc->abcB, is_apt(aptTextOutBaseline), ovx, pt);
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len, int flags)
{objCheck NULL;{
	ABC abc;
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return NULL;

	memset( pt, 0, sizeof( Point) * 5);

	flags &= ~toGlyphs;
	if ( flags & toUTF8 ) {
		int mb_len;
		if ( !( text = ( char *) guts.alloc_utf8_to_wchar_visual( text, len, &mb_len))) {
			free( pt);
			return NULL;
		}
		len = mb_len;
	}
	gp_get_text_widths(self, text, len, flags | toAddOverhangs, &abc);
	gp_get_text_box(self, &abc, pt);
	if ( flags & toUTF8 ) free(( char*) text);
	return pt;
}}

Point *
apc_gp_get_glyphs_box( Handle self, PGlyphsOutRec t)
{objCheck NULL;{
	ABC abc;
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return NULL;

	memset( pt, 0, sizeof( Point) * 5);
	if ( t-> fonts )
		gp_get_polyfont_widths(self,t,toAddOverhangs,&abc);
	else
		gp_get_text_widths( self, (const char*)t->glyphs, t->len, t->flags | toGlyphs | toAddOverhangs, &abc);
	gp_get_text_box(self, &abc, pt);

	return pt;
}}

int
apc_gp_get_glyph_outline( Handle self, unsigned int index, unsigned int flags, int ** buffer)
{
	int offset, gdi_size, r_size, *r_buf, *r_ptr;
	Byte * gdi_buf;
	GLYPHMETRICS gm;
	MAT2 matrix;
	UINT format;

	*buffer = NULL;
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
		return gdi_size;
	}

	if (( gdi_buf = malloc(gdi_size)) == NULL ) {
		warn("Not enough memory");
		return -1;
	}

	if (
		( (flags & (ggoUnicode | ggoGlyphIndex) ) ?
			GetGlyphOutlineW(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix) :
			GetGlyphOutlineA(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix)
		) == GDI_ERROR
	) {
		apiErr;
		free(gdi_buf);
		return -1;
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
		return -1;
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
apc_gp_set_text_matrix( Handle self, Matrix matrix)
{
	Bool old_want_world_transform, new_want_world_transform;
	objCheck 0;

	old_want_world_transform = is_apt(aptWantWorldTransform);
	new_want_world_transform = !prima_matrix_is_identity( matrix );
	if ( old_want_world_transform && !new_want_world_transform ) {
		XFORM xf = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
		SetViewportOrgEx( sys ps, 0, 0, NULL );
		SetWorldTransform( sys ps, &xf );
		apt_set( aptCachedWorldTransform );
	} else {
		apt_clear( aptCachedWorldTransform );
	}

	apt_assign( aptWantWorldTransform, new_want_world_transform);

	return true;
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

Bool
apc_font_begin_query( Handle self )
{
	if ( !( sys ps = CreateCompatibleDC( NULL )))
		return false;
	return true;
}

void
apc_font_end_query( Handle self )
{
	DeleteDC( sys ps );
	sys ps = 0;
}

Byte*
apc_font_get_glyph_bitmap( Handle self, uint16_t index, unsigned int flags, PPoint offset, PPoint size, int *advance)
{
	int gdi_size;
	Byte * gdi_buf;
	GLYPHMETRICS gm;
	MAT2 matrix;
	UINT format;
	Matrix *m = &var current_state.matrix;

	memset(&matrix, 0, sizeof(matrix));
#define FLOAT2FIXED(field,val) \
	matrix.field.value = ((val) < 0) ? ((short)(val)) - 1 : ((short)(val));\
	matrix.field.fract = ((val) - matrix.field.value) * 65535
	FLOAT2FIXED(eM11, (*m)[0]);
	FLOAT2FIXED(eM12, -(*m)[1]);
	FLOAT2FIXED(eM21, (*m)[2]);
	FLOAT2FIXED(eM22, -(*m)[3]);
#undef FLOAT2FIXED
	select_world_transform(self, false);

	format = GGO_GLYPH_INDEX |
		(( flags & ggoMonochrome ) ? GGO_BITMAP : GGO_GRAY8_BITMAP) |
		(( flags & ggoUseHints )   ? 0          : GGO_UNHINTED)
		;
	if (( gdi_size = GetGlyphOutlineW(sys ps, index, format, &gm, 0, NULL, &matrix)) < 0 )
		return NULL;
	if (( gdi_buf = malloc(gdi_size)) == NULL ) {
		warn("Not enough memory");
		return NULL;
	}

	if ( GetGlyphOutlineW(sys ps, index, format, &gm, gdi_size, gdi_buf, &matrix) == GDI_ERROR) {
		apiErr;
		free(gdi_buf);
		return NULL;
	}

	size-> x   = gm.gmBlackBoxX;
	size-> y   = gm.gmBlackBoxY;
	offset-> x = gm.gmptGlyphOrigin.x;
	offset-> y =-gm.gmptGlyphOrigin.y;
	if ( gdi_size == 0 )
		size-> x = size-> y = 0;
	if ( advance ) {
		ABC x;
		if (GetCharABCWidthsI( sys ps, index, 1, NULL, &x)) {
			*advance = x.abcA + x.abcB + x.abcC;
		} else {
			*advance = gm.gmCellIncX;
			apiErr;
		}
	}

	if ( !( flags & ggoMonochrome)) {
		register Byte* buf = gdi_buf;
		register unsigned int bytes = size->y * (( size->x * 8 + 31) / 32) * 4;
		while (bytes--) {
			*buf = *buf * 3.984 + .5; /* 255/64 */
			buf++;
		}
	}

	return gdi_buf;
}

#ifdef __cplusplus
}
#endif
