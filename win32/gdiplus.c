#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Window.h"
#include "Icon.h"
#include "DeviceBitmap.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

static Bool
create_gdip_surface(Handle self)
{
	HRGN rgn;

	/* force null region as it lingers in GDI+ space somehow */
	rgn = CreateRectRgn(0,0,0,0);
	if ( GetClipRgn( sys ps, rgn ) > 0 )
		SelectClipRgn( sys ps, NULL);
	else
		rgn = NULL;

	GPCALL GdipCreateFromHDC(sys ps, &sys graphics);
	apiGPErrCheckRet(false);

	if ( rgn ) {
		SelectClipRgn( sys ps, rgn);
		GPCALL GdipSetClipHrgn(sys graphics, rgn, CombineModeReplace);
		apiGPErrCheck;
		DeleteObject( rgn);
	}

	return true;
}

#define CREATE_GDIP_SURFACE (( sys ps && sys graphics == NULL ) ? create_gdip_surface(self) : true)

Bool
apc_gp_set_alpha( Handle self, int alpha)
{
	if (
		( is_apt(aptDeviceBitmap) && ((PDeviceBitmap)self)->type == dbtBitmap) ||
		( is_apt(aptImage)        && ((PImage)self)-> type == imBW )
	)
		alpha = 255;

	sys alpha = alpha;

	if ( alpha < 255 ) {
		if ( !CREATE_GDIP_SURFACE) {
			sys alpha = 255;
			return false;
		}
	}

	if ( sys ps && sys graphics )
		STYLUS_FREE_GP_BRUSH;

	if ( sys alphaArenaPalette ) {
		free(sys alphaArenaPalette);
		sys alphaArenaPalette = NULL;
	}

	return true;
}

Bool
apc_gp_set_antialias( Handle self, Bool aa)
{
	if ( aa ) {
		if (
			( is_apt(aptDeviceBitmap) && ((PDeviceBitmap)self)->type == dbtBitmap) ||
			( is_apt(aptImage)        && ((PImage)self)-> type == imBW )
		)
			return false;
		if ( !CREATE_GDIP_SURFACE)
			return false;
		apt_set(aptGDIPlus);
		GdipSetSmoothingMode(sys graphics, SmoothingModeAntiAlias);
		GdipSetPixelOffsetMode(sys graphics, PixelOffsetModeHalf);
	} else {
		apt_clear(aptGDIPlus);
		GdipSetSmoothingMode(sys graphics, SmoothingModeNone);
		GdipSetPixelOffsetMode(sys graphics, PixelOffsetModeNone);
	}
	return true;
}

int
apc_gp_get_alpha( Handle self)
{
	return sys alpha;
}

Bool
apc_gp_get_antialias( Handle self)
{
	return is_apt(aptGDIPlus);
}

Bool
apc_gp_aa_bar( Handle self, double x1, double y1, double x2, double y2)
{
	double dy = sys lastSize. y;
	Point t = sys gp_transform;
	objCheck false;

	x2 -= x1 - 1;
	y2 -= y1 - 1;
	x1 += t.x;
	y1 = t.y + dy - y1 - y2;

	STYLUS_USE_GP_BRUSH;
	if ( !CURRENT_GP_BRUSH ) return false;
	GPCALL GdipFillRectangle(
		sys graphics,
		CURRENT_GP_BRUSH,
		x1, y1, x2, y2
	);
	apiGPErrCheckRet(false);

	return true;
}

Bool
apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points)
{
	int i;
	double dy = sys lastSize. y;
	Point t = sys gp_transform;
	GpPointF *p;
	objCheck false;

	if ((p = malloc( sizeof(GpPointF) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i].X = t.x + points[i].x;
		p[i].Y = t.y + dy - points[i].y;
	}

	STYLUS_USE_GP_BRUSH;
	if ( !CURRENT_GP_BRUSH ) return false;
	GPCALL GdipFillPolygon(
		sys graphics,
		CURRENT_GP_BRUSH,
		p, numPts,
		((sys psFillMode & fmWinding) == fmAlternate) ?
			FillModeAlternate : FillModeWinding
	);
	apiGPErrCheckRet(false);

	return true;
}

/* emulate alpha text */

#define GRAD 57.29577951

void
aa_free_arena(Handle self, Bool for_reuse)
{
	if ( !for_reuse && sys alphaArenaPalette ) {
		free(sys alphaArenaPalette);
		sys alphaArenaPalette = NULL;
	}

	if ( !sys alphaArenaDC)
		return;

	if (sys alphaArenaStockFont)
		SelectObject( sys alphaArenaDC, sys alphaArenaStockFont);
	SelectObject( sys alphaArenaDC, sys alphaArenaStockBitmap);
	DeleteObject( sys alphaArenaBitmap );
	sys alphaArenaBitmap = NULL;
	sys alphaArenaStockFont = NULL;
	sys alphaArenaStockBitmap = NULL;
	if ( !for_reuse ) {
		DeleteDC(sys alphaArenaDC);
		sys alphaArenaDC = NULL;
	}

}

static Bool
aa_make_arena(Handle self)
{
	int w,h;
	w = var font. maximalWidth;
	h = var font. height;
	if ( w < h ) w = h;

	if ( var font. direction != 0) {
		if ( sys font_sin == sys font_cos && sys font_sin == 0.0 ) {
			sys font_sin = sin( var font. direction / GRAD);
			sys font_cos = cos( var font. direction / GRAD);
		}
	}

	if ( w < sys alphaArenaSize.x || h < sys alphaArenaSize.y || !sys alphaArenaDC ) {
		HDC dc;
		if ( sys alphaArenaBitmap ) {
			aa_free_arena(self, true);
		} else {
			if (!( sys alphaArenaDC = CreateCompatibleDC(0)))
				return false;
			SetTextAlign(sys alphaArenaDC, TA_BOTTOM);
			SetBkMode( sys alphaArenaDC, TRANSPARENT);
			SetTextColor( sys alphaArenaDC, 0xffffff);
		}

		sys alphaArenaSize.x = sys alphaArenaSize.y = w * 4;

		dc = dc_alloc();
		if ( !( sys alphaArenaBitmap = image_create_argb_dib_section(
			dc, sys alphaArenaSize.x, sys alphaArenaSize.y, &sys alphaArenaPtr
		))) {
			dc_free();
			return false;
		}
		dc_free();

		sys alphaArenaStockBitmap = SelectObject( sys alphaArenaDC, sys alphaArenaBitmap);
		sys alphaArenaStockFont = NULL;
		sys alphaArenaFontChanged = true;
	}

	if ( sys alphaArenaFontChanged || !sys alphaArenaStockFont) {
		HFONT f;
		f = SelectObject( sys alphaArenaDC, sys dc_font-> hfont );
		if ( !sys alphaArenaStockFont )
			sys alphaArenaStockFont = f;
		sys alphaArenaFontChanged = false;
	}

	return true;
}

static Bool
aa_render( Handle self, int x, int y, NPoint* delta, ABCFLOAT * abc, int advance, int dx, int dy)
{
	BLENDFUNCTION bf;
	float xx, yy, shift;
	register uint32_t * p, * palette;
	int i, j, miny, maxy, maxx, minx;
	Point sz;

	/* replace white to our color + alpha, calculate minimal affected box */
	p = sys alphaArenaPtr;
	palette = sys alphaArenaPalette;
	for (
		i = maxy = maxx = 0,
			minx = sys alphaArenaSize.x - 1,
			miny = sys alphaArenaSize.y - 1;
		i < sys alphaArenaSize.y;
		i++
	) {
		Bool match = false;
		for ( j = 0; j < sys alphaArenaSize.x; j++, p++) {
			if (1 || *p != 0) {
				register Byte *argb = (Byte*) p;
				*p = palette[argb[0] + argb[1] + argb[2]];
				if ( minx > j ) minx = j;
				if ( maxx < j ) maxx = j;
				match = true;
			}
		}
		if ( match ) {
			if ( miny > i ) miny = i;
			if ( maxy < i ) maxy = i;
		}
	}
	if ( maxy < miny ) return true;

	/* calculate advance for the next glyph and the position, if needed, for this one */
	if ( advance >= 0 ) {
		shift = advance;
		if ( var font.direction != 0 ) {
			xx = (dx * sys font_cos) - (dy * sys font_sin);
			yy = (dx * sys font_sin) + (dy * sys font_cos);
		} else {
			xx = dx;
			yy = dy;
		}
	} else {
		shift = abc->abcfA + abc->abcfB + abc->abcfC;
		xx = yy = 0.0;
	}
	x += round(delta->x + xx);
	y += round(delta->y - yy);

	if ( var font.direction != 0 ) {
		delta->x += shift * sys font_cos;
		delta->y -= shift * sys font_sin;
	} else {
		delta->x += shift;
	}

	/* calculate the aperture */
	sz = sys alphaArenaSize;
	x -= sz.x/2;
	y -= sz.y/2;
	if ( is_apt( aptTextOutBaseline)) {
		if ( var font. direction != 0 ) {
			float d = var font. descent;
			x += d * sys font_sin + .5;
			y += d * sys font_cos + .5;
		} else
			y += var font. descent;
	}

	/* minimize the blit rectangle */
	{
		int m, n;
		m = sys alphaArenaSize.y - maxy - 1;
		n = sys alphaArenaSize.y - miny - 1;
		miny = m;
		maxy = n;
	}
	sz.x = maxx - minx + 1;
	sz.y = maxy - miny + 1;

	/* blend */
	bf.BlendOp             = AC_SRC_OVER;
	bf.BlendFlags          = 0;
	bf.SourceConstantAlpha = 0xff;
	bf.AlphaFormat         = AC_SRC_ALPHA;
	return AlphaBlend(
		sys ps,           x + minx, y + miny,   sz.x, sz.y,
		sys alphaArenaDC, minx, miny,           sz.x, sz.y,
		bf);
}

/* precalculate alpha map */
Bool
aa_fill_palette(Handle self)
{
	int i,j,r,g,b;
	COLORREF fg = sys rq_pen.logpen.lopnColor;

	if ( sys alphaArenaPalette )
		return true;

	if ( !( sys alphaArenaPalette = malloc(4 * 256 * 3)))
		return false;

	b = (fg >> 16) & 0xff;
	g = (fg & 0xff00) >> 8;
	r =  fg & 0xff;

	for ( i = j = 0; i < 256; i++) {
		uint32_t a = sys alpha * i / 255;
		a =
			(a << 24)              |
			((r * a / 255 ) << 16) |
			((g * a / 255 ) << 8)  |
			( b * a / 255 )
			;
		sys alphaArenaPalette[j++] = a;
		sys alphaArenaPalette[j++] = a;
		sys alphaArenaPalette[j++] = a;
	}

	return true;
}

Bool
aa_text_out( Handle self, int x, int y, void * text, int len, Bool wide)
{
	int i;
	NPoint delta = { 0, 0 };

	if ( !(aa_fill_palette(self) && aa_make_arena(self)))
		return false;

	for ( i = 0; i < len; i++) {
		ABCFLOAT abc;
		memset(sys alphaArenaPtr, 0, sys alphaArenaSize.x * sys alphaArenaSize.y * 4);
		if ( wide ) {
			if (!GetCharABCWidthsFloatW( sys ps, ((WCHAR*)text)[i], ((WCHAR*)text)[i], &abc)) apiErrRet;
		} else {
			if (!GetCharABCWidthsFloatA( sys ps, ((char *)text)[i], ((char *)text)[i], &abc)) apiErrRet;
		}
		if ( wide ) {
			if (!TextOutW( sys alphaArenaDC, sys alphaArenaSize.x/2, sys alphaArenaSize.y/2, ((WCHAR*)text) + i, 1)) apiErrRet;
		} else {
			if (!TextOutA( sys alphaArenaDC, sys alphaArenaSize.x/2, sys alphaArenaSize.y/2, ((char *)text) + i, 1)) apiErrRet;
		}
		if ( !aa_render(self, x, y, &delta, &abc, -1, 0, 0))
			return false;
	}
	return true;
}

Bool
aa_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y, int * text_advance, HFONT font)
{
	int i;
	NPoint delta = { 0, 0 };
	uint16_t *advances = t->advances;
	int16_t *positions = t->positions;
	HFONT f;

	if ( !(aa_fill_palette(self) && aa_make_arena(self)))
		return false;

	if ( text_advance )
		*text_advance = 0;

	f = SelectObject( sys alphaArenaDC, font );
	if ( !sys alphaArenaStockFont )
		sys alphaArenaStockFont = f;
	sys alphaArenaFontChanged = false;

	for ( i = 0; i < t->len; i++) {
		ABCFLOAT abc, *pabc;
		int adv, dx, dy;
		memset(sys alphaArenaPtr, 0, sys alphaArenaSize.x * sys alphaArenaSize.y * 4);
		if ( !ExtTextOutW(sys alphaArenaDC,
			sys alphaArenaSize.x/2, sys alphaArenaSize.y/2,
			ETO_GLYPH_INDEX, NULL, (LPCWSTR)(t->glyphs) + i, 1, 
			NULL
		))
			apiErrRet;

		if ( advances ) {
			adv = *(advances++);
			if ( text_advance )
				*text_advance += adv;
			pabc = NULL;
			dx = *(positions++);
			dy = *(positions++);
		} else {
			ABC abci;
			adv = -1;
			pabc = &abc;
			dx = dy = 0;
			if (!GetCharABCWidthsI( sys ps, ((WCHAR*)(t->glyphs))[i], 1, NULL, &abci)) apiErr;
			if ( text_advance )
				*text_advance += abci.abcA + abci.abcB + abci.abcC;
			abc.abcfA = abci.abcA;
			abc.abcfB = abci.abcB;
			abc.abcfC = abci.abcC;
		}
		if ( !aa_render(self, x, y, &delta, pabc, adv, dx, dy))
			return false;
	}

	return true;
}

#ifdef __cplusplus
}
#endif
