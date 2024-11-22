/* DirectWrite-based fonts */

#define INITGUID
#include "win32\win32guts.h"

#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_3.h>

#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle


static Bool                    dw_ok       = false;

static ID2D1Factory           *d2d_factory = NULL;
static IDWriteFactory4        *factory     = NULL;
static IDWriteFontCollection1 *collection  = NULL;
static IDWriteGdiInterop      *gdi         = NULL;

Bool
dwrite_font_init(void)
{
	HRESULT hr;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, (REFIID) &IID_ID2D1Factory, NULL, (void**) &d2d_factory);
	if ( hr != S_OK )
		apiHErrRet(hr);

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory4, (IUnknown**) &factory);
	if ( hr != S_OK )
		apiHErrRet(hr);

	hr = factory->lpVtbl->IDWriteFactory3_GetSystemFontCollection( factory, 1, &collection, 1 );
	if ( hr != S_OK )
		apiHErrRet(hr);

	hr = factory->lpVtbl->GetGdiInterop( factory, &gdi );
	if ( hr != S_OK )
		apiHErrRet(hr);

	dw_ok = true;
	return true;
}

void
dwrite_font_done(void)
{
	if ( gdi        ) gdi        ->lpVtbl->Release(gdi);
	if ( collection ) collection ->lpVtbl->Release(collection);
	if ( factory    ) factory    ->lpVtbl->Release(factory);
	if ( d2d_factory) d2d_factory->lpVtbl->Base.Release((IUnknown*)d2d_factory);

	d2d_factory = NULL;
	factory     = NULL;
	collection  = NULL;

	dw_ok = false;
}

Bool
dwrite_logfont_colored( LOGFONTW *lf )
{
	HRESULT hr;
	IDWriteFont2 *font;
	Bool ok = false;

	if (!dw_ok) return ok;

	hr = gdi->lpVtbl->CreateFontFromLOGFONT(gdi, lf, (IDWriteFont**) &font);
	if ( hr != S_OK )
		apiHErrRet(hr);

	if ( font->lpVtbl->IsColorFont(font))
		ok = true;

	font->lpVtbl->Release(font);
	return ok;
}

static IDWriteColorGlyphRunEnumerator1*
get_enumerator(Handle self, PDCFont dc, PGlyphsOutRec t, int x, int y)
{
	int i, j, n;
	HRESULT hr;
	DWRITE_GLYPH_RUN run;
	Byte *buf;
	FLOAT               advances[256], *p_advances;
	DWRITE_GLYPH_OFFSET offsets[256], *p_offsets;
	D2D1_POINT_2F offset = {x, y};
	IDWriteColorGlyphRunEnumerator1 *enumerator;

	if (!dw_ok)
		return NULL;
	if ( dc-> dw_checked && !dc->dw_colorface )
		return NULL;

	if ( !dc->dw_checked ) {
		HRESULT hr;
		hr = gdi->lpVtbl->CreateFontFaceFromHdc(gdi, sys ps, (IDWriteFontFace**) &dc-> dw_colorface);
		dc->dw_checked = true;
		if ( hr != S_OK )
			return dc-> dw_colorface = NULL;
	}
	n = t->len;
	if ( n > 256 && (t->advances || t->positions)) {
		if ( !( buf = malloc((sizeof(FLOAT) + sizeof(DWRITE_GLYPH_OFFSET)) * n)))
			return NULL;
		p_advances = (FLOAT*) buf;
		p_offsets  = (DWRITE_GLYPH_OFFSET*) (buf + sizeof(FLOAT) * n);
	} else {
		p_advances = advances;
		p_offsets  = offsets;
		buf        = NULL;
	}

	if ( t-> advances ) {
		for ( i = 0; i < n; i++)
			p_advances[i] = t->advances[i];
	} else
		p_advances = NULL;

	if ( t-> positions ) {
		for ( i = j = 0; i < n; i++) {
			offsets[i].advanceOffset  = t->positions[j++];
			offsets[i].ascenderOffset = t->positions[j++];
		}
	} else
		p_offsets = NULL;

	run.fontFace      = ( IDWriteFontFace *) dc->dw_colorface;
	run.fontEmSize    = var font.size;
	run.glyphCount    = t->len;
	run.glyphIndices  = t->glyphs;
	run.glyphAdvances = p_advances;
	run.glyphOffsets  = p_offsets;
	run.isSideways    = 0;
	run.bidiLevel     = 0;

	hr = factory->lpVtbl->IDWriteFactory4_TranslateColorGlyphRun(
		factory, offset, &run, NULL,
		DWRITE_GLYPH_IMAGE_FORMATS_COLR,
		DWRITE_MEASURING_MODE_NATURAL,
		NULL, 0,
		&enumerator
	);

	if ( buf )
		free(buf);

	if ( hr == DWRITE_E_NOCOLOR )
		return NULL;
	if ( hr != S_OK ) {
		apiHErr(hr);
		return NULL;
	}

	return enumerator;
}

typedef struct {
	GlyphsOutRec g;
	Bool     has_color;
	COLORREF colorref;
	Point    baseline;
	int      format;
} GlyphColorRec;

static GlyphColorRec*
run2glyphs( const DWRITE_COLOR_GLYPH_RUN1 *run)
{
	GlyphColorRec *t;
	Byte *ptr;
	unsigned int size;

	size =
		sizeof(GlyphColorRec) +
		sizeof(uint16_t) * run->glyphRun.glyphCount * (
			1 +
			(run->glyphRun.glyphAdvances ? 1 : 0) +
			(run->glyphRun.glyphOffsets  ? 2 : 0)
		);
	if ( !( ptr = malloc(size)))
		return NULL;
	t = (GlyphColorRec*) ptr;

	memset( t, 0, size);
	t-> g.len     = run-> glyphRun.glyphCount;
	t-> format    = run-> glyphImageFormat;
	ptr += sizeof(GlyphColorRec);

	t->baseline.x = run->baselineOriginX;
	t->baseline.y = run->baselineOriginY;
	if ( run-> paletteIndex != 65535 ) {
		t->colorref =
			 (int) (run->runColor.r * 255.0 + .5)          |
			((int) (run->runColor.g * 255.0 + .5)) * 256   |
			((int) (run->runColor.b * 255.0 + .5)) * 65536;
		t->has_color = true;
	}

	t->g.glyphs = (uint16_t*) ptr;
	memcpy( t->g.glyphs, run->glyphRun.glyphIndices, sizeof(uint16_t) * t->g.len);
	ptr += sizeof(uint16_t) * t->g.len;

	if ( run->glyphRun.glyphAdvances) {
		int i;
		uint16_t *a;
		int16_t *o;

		a = t->g.advances  = t->g.glyphs + t->g.len;
		for ( i = 0; i < run->glyphRun.glyphCount; i++)
			*(a++) = (run->glyphRun.glyphAdvances[i] > 65535.0) ?
				65535 :
				(run->glyphRun.glyphAdvances[i] + .5);
		ptr += sizeof(uint16_t) * t->g.len;

		if ( run->glyphRun.glyphOffsets ) {
			o = t->g.positions = (int16_t*)t->g.advances + t->g.len;
			for ( i = 0; i < run->glyphRun.glyphCount; i++) {
				*(o++) =
					(run->glyphRun.glyphOffsets[i].advanceOffset > -32767.0) ? (
						(run->glyphRun.glyphOffsets[i].advanceOffset > 32767.0) ? 32767 :
							(run->glyphRun.glyphOffsets[i].advanceOffset + .5)
					) : -32767;
				*(o++) =
					(run->glyphRun.glyphOffsets[i].ascenderOffset > -32767.0) ? (
						(run->glyphRun.glyphOffsets[i].ascenderOffset > 32767.0) ? 32767 :
							(run->glyphRun.glyphOffsets[i].ascenderOffset + .5)
					) : -32767;
			}
			ptr += sizeof(int16_t) * 2 * t->g.len;
		}
	}

	return t;
}

static PList
enumerate_colorrun(IDWriteColorGlyphRunEnumerator1 *enumerator, unsigned int seed_glyphs)
{
	PList l;

	if ( seed_glyphs < 16 ) seed_glyphs = 16;
	seed_glyphs *= 2;
	if ( !( l = plist_create( seed_glyphs, seed_glyphs )))
		return NULL;

	while (1) {
		BOOL ok;
		HRESULT hr;
		const DWRITE_COLOR_GLYPH_RUN1 *run;
		GlyphColorRec *t;

		if ( enumerator->lpVtbl->IDWriteColorGlyphRunEnumerator1_GetCurrentRun( enumerator, &run) != S_OK )
			break;
		if (( t = run2glyphs( run )) != NULL )
			list_add(l, (Handle) t);
		hr = enumerator->lpVtbl->MoveNext( enumerator, &ok);
		if ( hr != S_OK ) {
			apiHErr(hr);
			break;
		}
	}

	if ( l-> count == 0 ) {
		plist_destroy(l);
		return NULL;
	}

	return l;
}

static Bool
is_aa_colr( Handle self, PList l)
{
	int i;
	Bool has_no_advances = false, has_colr = false;

	if ( sys alpha == 255 ) return false;

	for ( i = 0; i < l->count; i++) {
		GlyphColorRec *t = (GlyphColorRec*) l->items[i];
		if (t-> format == DWRITE_GLYPH_IMAGE_FORMATS_COLR)
			has_colr = true;
		if (!t->g.advances)
			has_no_advances = true;
	}

	return has_colr && !has_no_advances;
}

static void
draw_colorglyphs( Handle self, HFONT hfont, PList l )
{
	int i;

	for ( i = 0; i < l->count; i++) {
		GlyphColorRec *t   = (GlyphColorRec*) l->items[i];

		switch (t-> format) {
		case 0:
			STYLUS_USE_TEXT;
			if ( sys alpha < 255 )
				text_aa_glyphs_out( self, &t->g, t->baseline.x, t->baseline.y, NULL, hfont);
			else
				text_gp_glyphs_out( self, &t->g, t->baseline.x, t->baseline.y, 0);
			break;
		case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
			if ( t->has_color ) {
				SetTextColor( sys ps, t->colorref );
				STYLUS_FREE_TEXT;
			} else
				STYLUS_USE_TEXT;

			text_gp_glyphs_out( self, &t->g, t->baseline.x, t->baseline.y, 0);
			break;
		}
	}
}

static void
draw_aa_colr( Handle self, HFONT hfont, PList l, int xx, int yy )
{
	NPoint delta = {0,0};
	int i, j, x = MININT;

	if ( !text_aa_init(self, hfont, false))
		return;

	while ( 1 ) {
		int next_x = x, advance = -1;
		Point offset = {0,0};

		for ( i = 0; i < l->count; i++) {
			GlyphColorRec *t = (GlyphColorRec*) l->items[i];
			int dx = t->baseline.x;
			for ( j = 0; j < t->g.len; j++) {
				if ( dx > x && (next_x == x || dx < next_x))
					next_x = dx;
				dx += t->g.advances[j];
			}
		}
		if ( next_x == x )
			break;
		x = next_x;

		text_aa_begin_render(self, false);
		for ( i = 0; i < l->count; i++) {
			GlyphColorRec *t = (GlyphColorRec*) l->items[i];
			int dx = t->baseline.x, idx = -1;
			for ( j = 0; j < t->g.len; j++) {
				uint16_t a = t->g.advances[j];
				if ( dx == next_x ) {
					if ( advance < 0 ) {
						advance = a;
						if ( t->g.positions ){
							int16_t *p = t->g.positions + j * 2;
							offset.x = *(p++);
							offset.y = *(p++);
						}
					}
					idx = j;
					break;
				} else
					dx += a;
			}
			if ( idx < 0 )
				continue;

			SetTextColor( sys alpha_arena_dc, t->has_color ? t->colorref : sys rq_pen.logpen.lopnColor);
			text_aa_render( self, t->g.glyphs[idx]);
		}

		if ( !text_aa_end_render(self, xx, yy, &delta, NULL, advance, offset.x, offset.y, false))
			break;
	}
}

Bool
dwrite_color_text_out(Handle self, PDCFont dc, PGlyphsOutRec t, int x, int y)
{
	PList l;
	IDWriteColorGlyphRunEnumerator1 *enumerator;

	if ( !dw_ok ) return false;

	if ( !( enumerator = get_enumerator(self, dc, t, x, y)))
		return false;
	l = enumerate_colorrun( enumerator, t->len );
	enumerator->lpVtbl->Release(enumerator);
	if ( !l )
		return false;

	if ( is_aa_colr( self, l ))
		draw_aa_colr( self, dc->hfont, l, x, y );
	else
		draw_colorglyphs( self, dc->hfont, l );

	list_delete_all( l, true );
	plist_destroy( l );

	return true;
}

void
dwrite_free_face(void *face)
{
	((IDWriteFontFace*)face)->lpVtbl->Release((IDWriteFontFace*)face);
}

#ifdef __cplusplus
}
#endif
