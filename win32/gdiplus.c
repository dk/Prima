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

	if ( sys alpha_arena_palette ) {
		free(sys alpha_arena_palette);
		sys alpha_arena_palette = NULL;
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
apc_gp_aa_bars( Handle self, int nr, NRect *rr)
{
	double dy = sys last_size. y;
	Point t = sys transform2;
	objCheck false;

	select_world_transform(self, false);

	if (( is_apt(aptDeviceBitmap) && ((PDeviceBitmap)self)->type == dbtBitmap)) {
		Bool ok;
		PImage pt;
		Rect *bars;
		if ( sys alpha < 127 ) return true;

		if ( !( bars = prima_array_convert( nr * 4, rr, 'd', NULL, 'i')))
			return false;

		/* emulate transparency on a bitmap by xor/and */
		if (
			sys rop2 == ropNoOper &&
			sys rq_brush.logbrush.lbStyle == BS_DIBPATTERNPT && (
				(( pt = (PImage) var fillPatternImage ) == NULL ) ||
				(
					( pt-> type == imBW ) &&
					!dsys(pt)options.aptIcon
				)
			)
		) {
			Color fg, bg;
			int rop;
			rop = apc_gp_get_rop(self);
			fg = apc_gp_get_color(self);
			bg = apc_gp_get_back_color(self);
			apc_gp_set_color(self, 0);
			apc_gp_set_back_color(self, 0xffffff);
			apc_gp_set_rop(self, ropAndPut);
			ok = apc_gp_bars(self, nr, bars);
			apc_gp_set_rop(self, ropXor);
			apc_gp_set_color(self, fg);
			apc_gp_set_back_color(self, 0);
			ok |= apc_gp_bars(self, nr, bars);
			apc_gp_set_rop(self, rop);
			apc_gp_set_back_color(self, bg);
		} else {
			ok = apc_gp_bars(self, nr, bars);
		}

		free(bars);
		return ok;
	}

	STYLUS_USE_GP_BRUSH;
	if ( !CURRENT_GP_BRUSH ) return false;

	if ( nr > 1 ) {
		int i;
		RectF *rects, *rri;
		NRect *rrn;
		if ( !( rects = malloc(sizeof(RectF) * nr)))
			return false;
		for ( i = 0, rrn = rr, rri = rects; i < nr; i++) {
			rri->X      = rrn->left - t.x;
			rri->Y      = dy - rrn->top - t.y - 1.0;
			rri->Width  = rrn->left - rrn-> right  + 1.0;
			rri->Height = rrn->top  - rrn-> bottom + 1.0;
		}

		GPCALL GdipFillRectangles(
			sys graphics,
			CURRENT_GP_BRUSH,
			rects, nr
		);

		free(rects);
	} else {
		GPCALL GdipFillRectangle(
			sys graphics,
			CURRENT_GP_BRUSH,
			rr->left     - t.x,
			dy - rr->top - t.y      - 1.0,
			rr->right - rr-> left   + 1.0,
			rr->top   - rr-> bottom + 1.0
		);
	}

	apiGPErrCheckRet(false);
	return true;
}

Bool
apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points)
{
	int i;
	double dy = sys last_size. y;
	Point t = sys transform2;
	GpPointF *p;
	objCheck false;

	select_world_transform(self, false);

	if (( is_apt(aptDeviceBitmap) && ((PDeviceBitmap)self)->type == dbtBitmap)) {
		Bool ok;
		Point *p;
		PImage pt;
		if ( sys alpha < 127 ) return true;

		if ( !( p = prima_array_convert(( numPts + 1 ) * 2, points, 'd', NULL, 'i')))
			return false;

		/* emulate transparency on a bitmap by xor/and */
		if (
			sys rop2 == ropNoOper &&
			sys rq_brush.logbrush.lbStyle == BS_DIBPATTERNPT && (
				(( pt = (PImage) var fillPatternImage ) == NULL) ||
				(
					( pt-> type == imBW ) &&
					!dsys(pt)options.aptIcon
				)
			)
		) {
			Bool ok;
			Color fg, bg;
			int rop = apc_gp_get_rop(self);
			fg = apc_gp_get_color(self);
			bg = apc_gp_get_back_color(self);
			apc_gp_set_color(self, 0);
			apc_gp_set_back_color(self, 0xffffff);
			apc_gp_set_rop(self, ropAndPut);
			ok = apc_gp_fill_poly( self, numPts, p );
			apc_gp_set_rop(self, ropXor);
			apc_gp_set_color(self, fg);
			apc_gp_set_back_color(self, 0);
			ok |= apc_gp_fill_poly( self, numPts, p );
			apc_gp_set_rop(self, rop);
			apc_gp_set_back_color(self, bg);
			return ok;
		} else
			ok = apc_gp_fill_poly( self, numPts, p );
		free(p);
		return ok;
	}

	if ((p = malloc( sizeof(GpPointF) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i].X = points[i].x - t.x;
		p[i].Y = dy - points[i].y - t.y;
	}

	STYLUS_USE_GP_BRUSH;
	if ( !CURRENT_GP_BRUSH ) return false;
	GPCALL GdipFillPolygon(
		sys graphics,
		CURRENT_GP_BRUSH,
		p, numPts,
		((sys fill_mode & fmWinding) == fmAlternate) ?
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
	if ( !for_reuse && sys alpha_arena_palette ) {
		free(sys alpha_arena_palette);
		sys alpha_arena_palette = NULL;
	}

	if ( !sys alpha_arena_dc)
		return;

	if (sys alpha_arena_stock_font)
		SelectObject( sys alpha_arena_dc, sys alpha_arena_stock_font);
	SelectObject( sys alpha_arena_dc, sys alpha_arena_stock_bitmap);
	DeleteObject( sys alpha_arena_bitmap );
	sys alpha_arena_bitmap = NULL;
	sys alpha_arena_stock_font = NULL;
	sys alpha_arena_stock_bitmap = NULL;
	if ( !for_reuse ) {
		DeleteDC(sys alpha_arena_dc);
		sys alpha_arena_dc = NULL;
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

	if ( w < sys alpha_arena_size.x || h < sys alpha_arena_size.y || !sys alpha_arena_dc ) {
		HDC dc;
		if ( sys alpha_arena_bitmap ) {
			aa_free_arena(self, true);
		} else {
			if (!( sys alpha_arena_dc = CreateCompatibleDC(0)))
				return false;
			SetTextAlign(sys alpha_arena_dc, TA_BOTTOM);
			SetBkMode( sys alpha_arena_dc, TRANSPARENT);
			SetTextColor( sys alpha_arena_dc, 0xffffff);
		}

		sys alpha_arena_size.x = sys alpha_arena_size.y = w * 4;

		dc = dc_alloc();
		if ( !( sys alpha_arena_bitmap = image_create_argb_dib_section(
			dc, sys alpha_arena_size.x, sys alpha_arena_size.y, &sys alpha_arena_ptr
		))) {
			dc_free();
			return false;
		}
		dc_free();

		sys alpha_arena_stock_bitmap = SelectObject( sys alpha_arena_dc, sys alpha_arena_bitmap);
		sys alpha_arena_stock_font = NULL;
		sys alpha_arena_font_changed = true;
	}

	if ( sys alpha_arena_font_changed || !sys alpha_arena_stock_font) {
		HFONT f;
		f = SelectObject( sys alpha_arena_dc, sys dc_font-> hfont );
		if ( !sys alpha_arena_stock_font )
			sys alpha_arena_stock_font = f;
		sys alpha_arena_font_changed = false;
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
	p = sys alpha_arena_ptr;
	palette = sys alpha_arena_palette;
	for (
		i = maxy = maxx = 0,
			minx = sys alpha_arena_size.x - 1,
			miny = sys alpha_arena_size.y - 1;
		i < sys alpha_arena_size.y;
		i++
	) {
		Bool match = false;
		for ( j = 0; j < sys alpha_arena_size.x; j++, p++) {
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
	sz = sys alpha_arena_size;
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
		m = sys alpha_arena_size.y - maxy - 1;
		n = sys alpha_arena_size.y - miny - 1;
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
		sys alpha_arena_dc, minx, miny,           sz.x, sz.y,
		bf);
}

/* precalculate alpha map */
Bool
aa_fill_palette(Handle self)
{
	int i,j,r,g,b;
	COLORREF fg = sys rq_pen.logpen.lopnColor;

	if ( sys alpha_arena_palette )
		return true;

	if ( !( sys alpha_arena_palette = malloc(4 * 256 * 3)))
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
		sys alpha_arena_palette[j++] = a;
		sys alpha_arena_palette[j++] = a;
		sys alpha_arena_palette[j++] = a;
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
		memset(sys alpha_arena_ptr, 0, sys alpha_arena_size.x * sys alpha_arena_size.y * 4);
		if ( wide ) {
			if (!GetCharABCWidthsFloatW( sys ps, ((WCHAR*)text)[i], ((WCHAR*)text)[i], &abc)) apiErrRet;
		} else {
			if (!GetCharABCWidthsFloatA( sys ps, ((char *)text)[i], ((char *)text)[i], &abc)) apiErrRet;
		}
		if ( wide ) {
			if (!TextOutW( sys alpha_arena_dc, sys alpha_arena_size.x/2, sys alpha_arena_size.y/2, ((WCHAR*)text) + i, 1)) apiErrRet;
		} else {
			if (!TextOutA( sys alpha_arena_dc, sys alpha_arena_size.x/2, sys alpha_arena_size.y/2, ((char *)text) + i, 1)) apiErrRet;
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

	f = SelectObject( sys alpha_arena_dc, font );
	if ( !sys alpha_arena_stock_font )
		sys alpha_arena_stock_font = f;
	sys alpha_arena_font_changed = false;

	for ( i = 0; i < t->len; i++) {
		ABCFLOAT abc, *pabc;
		int adv, dx, dy;
		memset(sys alpha_arena_ptr, 0, sys alpha_arena_size.x * sys alpha_arena_size.y * 4);
		if ( !ExtTextOutW(sys alpha_arena_dc,
			sys alpha_arena_size.x/2, sys alpha_arena_size.y/2,
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
