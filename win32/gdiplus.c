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
	GPCALL GdipCreateFromHDC(sys ps, &sys graphics);
	apiGPErrCheckRet(false);

	if ( !is_apt(aptRegionIsEmpty)) {
		HRGN rgn;
		rgn = CreateRectRgn(0,0,0,0);
		GetClipRgn( sys ps, rgn );
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
		stylus_change( self);

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
		p[i].Y = t.y + dy - points[i].y - 1.0;
	}

	STYLUS_USE_GP_BRUSH;
	GPCALL GdipFillPolygon(
		sys graphics,
		sys stylusGPResource-> brush,
		p, numPts,
		((sys psFillMode & fmWinding) == fmAlternate) ?
			FillModeAlternate : FillModeWinding
	);
	apiGPErrCheckRet(false);

	return true;
}

#define GRAD 57.29577951

void
aa_free_arena(Handle self, Bool for_reuse)
{
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
		f = SelectObject( sys alphaArenaDC, sys fontResource-> hfont );
		if ( !sys alphaArenaStockFont )
			sys alphaArenaStockFont = f;
		sys alphaArenaFontChanged = false;
	}

	return true;
}

typedef struct {
	uint32_t full;
	int a,r,g,b;
} AAColor;

static Bool
aa_render( Handle self, int x, int y, NPoint* delta, ABCFLOAT * abc, int advance, int dx, int dy, AAColor * color)
{
	BLENDFUNCTION bf;
	float xx, yy, shift;
	register uint32_t * p;
	int i, j, miny, maxy, maxx, minx;
	Point sz;

	/* replace white to our color + alpha, calculate minimal affected box */
	p = sys alphaArenaPtr;
	for (
		i = maxy = maxx = 0,
			minx = sys alphaArenaSize.x - 1,
			miny = sys alphaArenaSize.y - 1;
		i < sys alphaArenaSize.y;
		i++
	) {
		for ( j = 0; j < sys alphaArenaSize.x; j++, p++) {
			if (*p != 0) {
				Byte *argb = (Byte*)p;
				int alpha = argb[0] + argb[1] + argb[2];
				if ( alpha == 255 * 3 ) {
					*p = color->full;
				} else {
					register int ma = alpha * color->a / (255 * 3);
					*p =
						(ma << 24) |
						((color->r * ma / 255 ) << 16) |
						((color->g * ma / 255 ) << 8) |
						( color->b * ma / 255 )
						;
				}
				if ( miny > i ) miny = i;
				if ( maxy < i ) maxy = i;
				if ( minx > j ) minx = j;
				if ( maxx < j ) maxx = j;
			}
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

void
aa_color(Handle self, AAColor * color)
{
	int r,g,b;
	float a;
	PStylus s = & sys stylus;

#define COMP(c) \
	c = ((float) c) * a + .5; \
	c &= 0xff

	a = (float) sys alpha / 255.0;
	color-> b = b = (s->pen.lopnColor >> 16) & 0xff;
	color-> g = g = (s->pen.lopnColor & 0xff00) >> 8;
	color-> r = r = s->pen.lopnColor & 0xff;
	COMP(r);
	COMP(g);
	COMP(b);
	color-> a    = sys alpha;
	color-> full = (sys alpha << 24) | (r << 16) | (g << 8) | b;
}

Bool
aa_text_out( Handle self, int x, int y, void * text, int len, Bool wide)
{
	int i;
	NPoint delta = { 0, 0 };
	AAColor color;

	aa_color(self, &color);
	if ( !aa_make_arena(self))
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
		if ( !aa_render(self, x, y, &delta, &abc, -1, 0, 0, &color))
			return false;
	}
	return true;
}

Bool
aa_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y, int * text_advance)
{
	return false;
}

#ifdef __cplusplus
}
#endif
