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

Bool
apc_gp_init( Handle self)
{
	objCheck false;
	return true;
}

Bool
apc_gp_done( Handle self)
{
	objCheck false;
	cleanup_gc_stack(self, 1);
	aa_free_arena(self, 0);
	if ( sys bm)
		if ( !DeleteObject( sys bm)) apiErr;
	if ( sys pal)
		if ( !DeleteObject( sys pal)) apiErr;
	if ( sys graphics) {
		GdipDeleteGraphics(sys graphics);
		sys graphics = NULL;
	}
	if ( sys ps) {
		if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
			if ( !EndPaint(( HWND) var handle, &sys paint_struct)) apiErr;
		} else if ( is_apt( aptWinPS)) {
			if ( self == prima_guts.application)
				dc_free();
			else {
				if ( !ReleaseDC(( HWND) var handle,  sys ps)) apiErr;
			}
		}
	}
	if ( sys line_pattern_len  > sizeof(sys line_pattern)) free( sys line_pattern);
	stylus_release( self );
	font_free( sys dc_font, false);
	if ( sys p256) free( sys p256);
	sys bm = NULL;
	sys pal = NULL;
	sys ps = NULL;
	sys bm = NULL;
	sys p256 = NULL;
	sys dc_font = NULL;
	sys line_pattern = NULL;
	return true;
}

#define ADJUST_LINE_END_DEF(typ)                                   \
void                                                               \
adjust_line_end_##typ                                              \
( typ x1, typ y1, typ *x2, typ *y2)                                \
{                                                                  \
	if ( x1 == *x2)                                            \
		( y1 < *y2) ? ( *y2)++ : ( *y2)--;                 \
	else if ( y1 == *y2)                                       \
		( x1 < *x2) ? ( *x2)++ : ( *x2)--;                 \
	else {                                                     \
		long tan = ( *y2 - y1) * 1000L / ( *x2 - x1);      \
		if ( tan < 1000 && tan > -1000) {                  \
			typ dx = *x2 - x1;                         \
			( dx > 0) ? dx++ : dx--;                   \
			*x2 = x1 + dx;                             \
			*y2 = (typ)( y1 + dx * tan / 1000L);       \
		} else {                                           \
			typ dy = *y2 - y1;                         \
			( dy > 0) ? dy++ : dy--;                   \
			*x2 = (typ)( x1 + dy * 1000L / tan);       \
			*y2 = y1 + dy;                             \
		}                                                  \
	}                                                          \
}                                                                  \

ADJUST_LINE_END_DEF(int);
ADJUST_LINE_END_DEF(LONG);

#define GRAD 57.29577951

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

#define EMULATE_OPAQUE_LINE \
(sys rq_pen.logpen.lopnStyle == PS_USERSTYLE && sys rop2 == ropCopyPut)
#define STYLUS_USE_OPAQUE_LINE \
	SelectObject(sys ps, stylus_get_pen(PS_SOLID, sys rq_pen.logpen.lopnWidth.x, sys rq_brush.back_color));\
	STYLUS_FREE_PEN;\
	if (sys rop != R2_COPYPEN) SetROP2( sys ps, R2_COPYPEN)
#define STYLUS_RESTORE_OPAQUE_LINE if (sys rop != R2_COPYPEN) SetROP2( sys ps, sys rop)

/* emulate transparent mono patterns with xor/and stippling */
static Bool
make_brush(Handle self, int* mix)
{
	switch (*mix) {
	case 0: {
		(*mix)++;
		if (
			sys rop != R2_COPYPEN ||
			sys rop2 != ropNoOper ||
			sys rq_brush.logbrush.lbStyle != BS_DIBPATTERNPT || (
				var fillPatternImage &&
				PObject(var fillPatternImage)->stage == csNormal &&
				PImage(var fillPatternImage)->type != imBW &&
				dsys(var fillPatternImage)options.aptIcon
			)
		) {
			STYLUS_USE_BRUSH;
			return true;
		}
		if ( !apc_gp_push(self, NULL, NULL, 0))
			return false;
		apc_gp_set_rop( self, ropAndPut );
		apc_gp_set_color( self, 0);
		apc_gp_set_back_color( self, 0xffffff);

		STYLUS_USE_BRUSH;
		(*mix) = 10;
		return true;
	}
	case 1:
		return false;
	case 10:
		(*mix)++;
		if ( !apc_gp_pop( self, NULL ))
			return false;
		if ( !apc_gp_push(self, NULL, NULL, 0))
			return false;
		apc_gp_set_rop( self, ropXorPut );
		apc_gp_set_back_color( self, 0);
		STYLUS_USE_BRUSH;
		return true;
	case 11:
		apc_gp_pop( self, NULL );
		return false;
	default:
		return false;
	}
}

Bool
apc_gp_bars( Handle self, int nr, Rect *rr)
{objCheck false;{
	HDC     ps = sys ps;
	Bool ok = true;
	int i;
	int mix = 0;

	SelectObject( ps, std_hollow_pen);
	STYLUS_FREE_PEN;

	while ( make_brush(self, &mix)) {
		Rect *r = rr;
		for ( i = 0; i < nr; i++, r++) {
			Rect xr = *r;
			check_swap( xr.left, xr.right);
			check_swap( xr.bottom, xr.top);
			SHIFT_Y( xr.bottom);
			SHIFT_Y( xr.top);
			if ( !( ok = Rectangle( ps, xr.left, xr.top, xr.right + 1, xr.bottom + 1))) {
				apiErr;
				break;
			}
		}
	}
	return ok;
}}

Bool
apc_gp_alpha( Handle self, int alpha, int x1, int y1, int x2, int y2)
{objCheck false;{
	Bool ok;
	HDC dc, buf_dc;
	HBITMAP buf_bm, old_bm;
	unsigned int dst_lw, y, w, h;
	Byte *dst;

	if ( !(
		(is_apt(aptDeviceBitmap) && ((PDeviceBitmap)self)->type == dbtLayered) ||
		is_apt(aptLayered) ||
		( apc_widget_get_layered_request(self) && apc_widget_surface_is_layered(self))
	))
	return false;

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
		x1 = y1 = 0;
		x2 = sys last_size. x - 1;
		y2 = sys last_size. y - 1;
	}
	check_swap( x1, x2);
	check_swap( y1, y2);
	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	y1 = y2;
	SHIFT_Y(y1);

	dc     = GetDC(NULL);
	buf_dc = CreateCompatibleDC(dc);
	if ( !(buf_bm = image_create_argb_dib_section(dc, w, h, (uint32_t**) &dst))) {
		DeleteDC(buf_dc);
		ReleaseDC(NULL, dc);
		return false;
	}

	old_bm = SelectObject(buf_dc, buf_bm);
	if ( !( ok = BitBlt( buf_dc, 0, 0, w, h, sys ps, x1, y1, SRCCOPY))) {
		apiErr;
		goto EXIT;
	}

	dst_lw = w * 4;
	for ( y = 0; y < h; y++, dst += dst_lw ) {
		register Byte *dd = dst + 3;
		register unsigned int ww = w;
		while (ww--) {
			*dd = alpha;
			dd += 4;
		}
	}

	if ( !( ok = BitBlt( sys ps, x1, y1, w, h, buf_dc, 0, 0, SRCCOPY)))
		apiErr;

EXIT:
	SelectObject(buf_dc, old_bm);
	DeleteDC(buf_dc);
	DeleteObject(buf_bm);
	ReleaseDC(NULL, dc);

	return ok;
}}

Bool
apc_gp_can_draw_alpha( Handle self)
{
	objCheck false;
	if ( is_apt(aptDeviceBitmap))
		return ((PDeviceBitmap)self)->type != dbtBitmap;
	else if ( is_apt( aptImage ))
		return ((PImage)self)-> type != imBW;
	else
		return true;
}

Bool
apc_gp_clear( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
	Bool     ok = true;
	HDC      ps   = sys ps;
	HGDIOBJ  o1, o2;

	o1 = SelectObject( ps, std_hollow_pen);
	o2 = SelectObject( ps, stylus_get_solid_brush(sys bg));

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
		x1 = y1 = 0;
		x2 = sys last_size. x - 1;
		y2 = sys last_size. y - 1;
	}
	check_swap( x1, x2);
	check_swap( y1, y2);
	SHIFT_Y(y1);
	SHIFT_Y(y2);
	if ( !( ok = Rectangle( sys ps, x1, y2, x2 + 1, y1 + 1)))
		apiErr;

	SelectObject( ps, o1 );
	SelectObject( ps, o2 );

	return ok;
}}

Bool
apc_gp_draw_poly( Handle self, int numPts, Point * points)
{objCheck false;{
	int i;
	POINT *p;
	Bool ok;

	if ((p = malloc( sizeof(POINT) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i].x = points[i].x;
		p[i].y = points[i].y;
		SHIFT_Y(p[i].y);
	}
	if ( p[0]. x != p[numPts - 1].x || p[0]. y != p[numPts - 1].y || numPts == 2)
		adjust_line_end_LONG( p[numPts - 2].x, p[numPts - 2].y, &p[numPts - 1].x, &p[numPts - 1].y);

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		Polyline( sys ps, p, numPts);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN;
	if ( !(ok = Polyline( sys ps, p, numPts))) apiErr;

	free(p);

	return ok;
}}

Bool
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{objCheck false;{
	Bool ok = true;
	int i;
	DWORD * pts;
	POINT * p;

	pts = ( DWORD *) malloc( sizeof( DWORD) * numPts);
	if ( !pts) return false;
	p = malloc( sizeof(POINT) * numPts);
	if ( !p ) {
		free(pts);
		return false;
	}

	for ( i = 0; i < numPts; i++)  {
		p[i].x = points[i].x;
		p[i].y = SHIFT_Y(points[i].y);
		pts[i] = 2;
		if ( i & 1)
			adjust_line_end_LONG( p[i - 1].x, p[i - 1].y, &p[i].x, &p[i].y);
	}

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		PolyPolyline( sys ps, p, pts, numPts/2);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN;
	if ( !( ok = PolyPolyline( sys ps, p, pts, numPts/2))) apiErr;
	free( pts);
	free( p);
	return ok;
}}

static Handle ctx_R22R4[] = {
	R2_COPYPEN      ,  SRCCOPY          ,
	R2_XORPEN       ,  SRCINVERT        ,
	R2_MASKPEN      ,  SRCAND           ,
	R2_MERGEPEN     ,  SRCPAINT         ,
	R2_NOTCOPYPEN   ,  NOTSRCCOPY       ,
	R2_MASKPENNOT   ,  SRCERASE         ,
	R2_MERGEPENNOT  ,  0x00DD0228       ,
	R2_MASKNOTPEN   ,  0x00220326       ,
	R2_MERGENOTPEN  ,  MERGEPAINT       ,
	R2_NOTXORPEN    ,  0x00990066       ,
	R2_NOTMASKPEN   ,  0x007700E6       ,
	R2_NOTMERGEPEN  ,  NOTSRCERASE      ,
	R2_NOP          ,  0x00AA0029       ,
	R2_BLACK        ,  BLACKNESS        ,
	R2_WHITE        ,  WHITENESS        ,
	R2_NOT          ,  DSTINVERT        ,
	endCtx
};

Bool
apc_gp_fill_poly( Handle self, int numPts, Point * points)
{Bool ok = true; objCheck false;{
	HDC     ps = sys ps;
	int i;
	POINT *p;
	int mix = 0;

	if ((p = malloc( sizeof(POINT) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i].x = points[i].x;
		p[i].y = SHIFT_Y(points[i].y);
	}
	if ( numPts == 2 )
		adjust_line_end_LONG( p[0].x, p[0].y, &p[1].x, &p[1].y);

	if (( sys fill_mode & fmOverlay) == 0) {
		SelectObject( ps, std_hollow_pen);
		while ( make_brush(self, &mix)) {
			if ( !( ok = Polygon( ps, p, numPts))) apiErr;
		}
		STYLUS_FREE_PEN;
	} else if ( !stylus_is_complex(self)) {
		SelectObject( ps, stylus_get_pen( PS_SOLID, 1, sys rq_brush.logbrush.lbColor));
		while ( make_brush(self, &mix)) {
			if ( !( ok = Polygon( ps, p, numPts))) apiErr;
		}
		STYLUS_FREE_PEN;
	} else {
		int dx       = sys last_size.x;
		int dy       = sys last_size.y;
		int rop      = ctx_remap_def( GetROP2( ps), ctx_R22R4, true, SRCCOPY);
		Point bound  = {0,0};
		Point trans;
		HBITMAP bmMask, bmSrc, bmJ;
		HDC dc;
		HGDIOBJ old1, oldelta;
		Bool db = is_apt( aptDeviceBitmap) || is_apt( aptBitmap);
		trans. x = dx;
		trans. y = dy;
		for ( i = 0; i < numPts; i++)  {
			if ( p[i].x > bound.x) bound.x = p[i].x;
			if ( p[i].y > bound.y) bound.y = p[i].y;
			if ( p[i].x < trans.x) trans.x = p[i].x;
			if ( p[i].y < trans.y) trans.y = p[i].y;
		}
		if (( trans. x == dx) || ( trans. y == dy)) {
			free(p);
			return false;
		}
		if ( bound. x > dx) bound. x = dx;
		if ( bound. y > dy) bound. y = dy;
		for ( i = 0; i < numPts; i++)  {
			p[i].x -= trans.x;
			p[i].y -= trans.y;
		}
		bound.x -= trans.x - 1;
		bound.y -= trans.y - 1;

		if ( !( dc  = dc_compat_alloc( ps))) apiErrRet;
		if ( db) {
			if ( !( ps = dc_alloc())) { // fact that if dest ps is memory dc, CCB will result mono-bitmap
				dc_compat_free();
				free(p);
				return false;
			}
		}
		if ( !( bmSrc  = CreateCompatibleBitmap( ps, bound. x, bound. y))) {
			apiErr;
			if ( db) dc_free();
			dc_compat_free();
			free(p);
			return false;
		}
		if ( db) {
			dc_free();
			ps = sys ps;
		}
		if ( !( bmMask = CreateBitmap( bound. x, bound. y, 1, 1, NULL))) {
			apiErr;
			dc_compat_free();
			free(p);
			return false;
		}
		bmJ = SelectObject( dc, bmSrc);
		oldelta = SelectObject( dc, std_hollow_pen);

		STYLUS_USE_BRUSH;
		old1 = SelectObject(dc, CURRENT_BRUSH);
		Rectangle( dc, 0, 0, bound. x, bound. y);
		SelectObject( dc, oldelta);
		SelectObject( dc, old1);
		SelectObject( dc, bmMask);
		SetROP2( dc, R2_WHITE);
		Rectangle( dc, 0, 0, bound. x, bound. y);
		if (
			sys rop == R2_COPYPEN &&
			sys rop2 == ropNoOper &&
			sys rq_brush.logbrush.lbStyle == BS_DIBPATTERNPT && (
				!var fillPatternImage || (
					PObject(var fillPatternImage)->stage == csNormal &&
					PImage(var fillPatternImage)->type == imBW &&
					!dsys(var fillPatternImage)options.aptIcon
				)
			)
		) {
			HDC savedc = sys ps;
			sys ps = dc;
			apc_gp_push( self, NULL, NULL, 0 );
			apc_gp_set_color( self, 0xffffff );
			apc_gp_set_back_color( self, 0 );
			apc_gp_set_rop( self, ropCopyPut );
			STYLUS_USE_BRUSH;
			SelectObject( dc, std_hollow_pen);
			if ( !( ok = Polygon( dc, p, numPts))) apiErr;
			apc_gp_pop( self, NULL );
			sys ps = savedc;
		} else {
			SetROP2( dc, R2_BLACK);
			if ( !( ok = Polygon( dc, p, numPts))) apiErr;
		}
		SelectObject( dc, bmSrc);
		if ( !( ok &= MaskBlt( ps, trans. x, trans. y, bound. x, bound. y, dc, 0, 0, bmMask, 0, 0,
					MAKEROP4( 0x00AA0029, rop)))) apiErr;
		SelectObject( dc, bmJ);
		dc_compat_free();
		DeleteObject( bmMask);
		DeleteObject( bmSrc);
	}
	free(p);
}return ok;}

Bool
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{objCheck false;{
	HDC ps = sys ps;
	STYLUS_USE_BRUSH;
	SHIFT_Y(y);
	if ( !ExtFloodFill( ps, x, y, remap_color( borderColor, true),
		singleBorder ? FLOODFILLSURFACE : FLOODFILLBORDER)) apiErrRet;
	return true;
}}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{objCheck clInvalid;{
	COLORREF c;
	SHIFT_Y(y);
	c = GetPixel( sys ps, x, y);
	if ( c == CLR_INVALID) return clInvalid;
	return remap_color(( Color) c, false);
}}

ApiHandle
apc_gp_get_handle( Handle self)
{
	objCheck 0;
	return ( ApiHandle) sys ps;
}

Bool
apc_gp_line( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
	HDC ps = sys ps;

	adjust_line_end_int( x1, y1, &x2, &y2);
	SHIFT_Y(y1);
	SHIFT_Y(y2);

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		MoveToEx( ps, x1, y1, NULL);
		LineTo( ps, x2, y2);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN;

	MoveToEx( ps, x1, y1, NULL);
	if ( !LineTo( ps, x2, y2)) apiErrRet;

	return true;
}}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
	return apc_gp_stretch_image ( self, image, x, y, xFrom, yFrom, xLen, yLen, xLen, yLen, rop);
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
	Bool ok = true;
	HDC     ps = sys ps;
	HGDIOBJ old;

	check_swap( x1, x2);
	check_swap( y1, y2);

	old = SelectObject( ps, std_hollow_brush);
	if ( sys rq_pen.logpen.lopnWidth.x > 1 &&
		(sys rq_pen.logpen.lopnWidth.x % 2) == 0
		) {
		/* change up-winding to down-winding */
		y1--;
		y2--;
	}
	SHIFT_Y(y1);
	SHIFT_Y(y2);

	if ( EMULATE_OPAQUE_LINE ) {
		STYLUS_USE_OPAQUE_LINE;
		Rectangle( sys ps, x1, y1, x2, y2);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN;
	if ( !( ok = Rectangle( sys ps, x1, y1, x2, y2))) apiErr;
	SelectObject( ps, old );

	return ok;
}}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
	objCheck false;
	SHIFT_Y(y);
	SetPixelV( sys ps, x, y, remap_color( color, true));
	return true;
}

// gpi settings
Color
apc_gp_get_back_color( Handle self)
{
	objCheck 0;
	return remap_color( sys bg, false);
}

int
apc_gp_get_bpp( Handle self)
{
	objCheck 0;
	return sys bpp;
}

Color
apc_gp_get_color( Handle self)
{
	objCheck 0;
	return remap_color( sys ps ? sys rq_pen.logpen.lopnColor : sys fg, false);
}

int
apc_gp_get_fill_mode( Handle self)
{
	objCheck 0;
	return sys fill_mode;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
	objCheck NULL;
	return &sys fill_pattern;
}

Point
apc_gp_get_fill_pattern_offset( Handle self)
{
	Point p = {0,0};
	objCheck p;
	return sys fill_pattern_offset;
}

int
apc_gp_get_line_pattern( Handle self, unsigned char * buffer)
{
	objCheck 0;
	if ( !sys ps) {
		memcpy(
			( char *) buffer,
			(char*)(( sys line_pattern_len > sizeof(sys line_pattern)) ? sys line_pattern : (Byte*)(&sys line_pattern)),
			sys line_pattern_len
		);
		return sys line_pattern_len;
	}

	switch ( sys rq_pen.logpen.lopnStyle) {
	case PS_NULL:
		strcpy(( char *) buffer, "");
		return 0;
	case PS_USERSTYLE: {
		int i;
		int len = sys rq_pen.line_pattern->count;
		if ( len > 255) len = 255;
		for ( i = 0; i < len; i++) {
			buffer[i] = sys rq_pen.line_pattern->ptr[i];
			if ( i & 1 )
				buffer[i]--;
			else
				buffer[i]++;
		}
		return len;
	}
	default:
		strcpy(( char *) buffer, "\1");
		return 1;
	}
}

Color
apc_gp_get_nearest_color( Handle self, Color color)
{
#define quit return remap_color( GetNearestColor( sys ps, clr), false)

	XLOGPALETTE lpLoc, lpGlob;
	int locIdx, globIdx, cdiff;
	long clrGlob, clr = remap_color( color, true);
	objCheck 0;

	if ( !sys pal || ( sys bpp > 8)) quit;
	lpLoc. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lpLoc. palPalEntry);
	if ( lpLoc. palNumEntries == 0)  quit;
	lpGlob. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lpGlob. palPalEntry);
	if ( lpGlob. palNumEntries == 0) quit;

	locIdx = palette_match_color( &lpLoc, clr, &cdiff);
	if ( cdiff >= COLOR_TOLERANCE)   quit;

	clrGlob = ARGB(
		lpLoc. palPalEntry[ locIdx]. peBlue,
		lpLoc. palPalEntry[ locIdx]. peGreen,
		lpLoc. palPalEntry[ locIdx]. peRed
	);
	globIdx = palette_match_color( &lpGlob, clrGlob, &cdiff);
	if ( cdiff >= COLOR_TOLERANCE)   quit;
	return ARGB(
		lpGlob. palPalEntry[ globIdx]. peRed,
		lpGlob. palPalEntry[ globIdx]. peGreen,
		lpGlob. palPalEntry[ globIdx]. peBlue
	);
#undef quit
}

PRGBColor
apc_gp_get_physical_palette( Handle self, int * color)
{
	XLOGPALETTE lpGlob;
	int i, nCol;
	PRGBColor r;

	*color = 0;
	objCheck NULL;

	if (( GetDeviceCaps( sys ps, RASTERCAPS) & RC_PALETTE) == 0)
		return NULL;

	nCol = GetDeviceCaps( sys ps, NUMCOLORS);
	if ( nCol <= 0 || nCol > 256)
		return NULL;


	if ( sys pal && ( nCol > 16)) {
		XLOGPALETTE lp;
		int i, lpCount = 0;
		int map[ 256];

		lp. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lp. palPalEntry);
		lpGlob. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lpGlob. palPalEntry);

		for ( i = 0; i < lp. palNumEntries; i++) {
			long clr = ARGB(
			lp. palPalEntry[ i]. peBlue,
			lp. palPalEntry[ i]. peGreen,
			lp. palPalEntry[ i]. peRed
			);
			int j, cdiff;
			int idx = palette_match_color( &lpGlob, clr, &cdiff);
			Bool hasmatch = 0;

			if ( cdiff >= COLOR_TOLERANCE) continue;

			if ( idx < 10 || idx > 245) continue;

			for ( j = 0; j < lpCount; j++)
				if ( map[ j] == idx) {
					hasmatch = 1;
					break;
				}
			if ( hasmatch) continue;
			map[ lpCount++] = idx;
		}

		for ( i = 0; i < lpCount; i++)
			lp. palPalEntry[ i] = lpGlob. palPalEntry[ map[ i]];
		for ( i = 0; i < lpCount; i++)
			lpGlob. palPalEntry[ i + nCol] = lp. palPalEntry[ i];
		*color = nCol + lpCount;
	} else {
		*color = GetSystemPaletteEntries( sys ps, 0, 256, lpGlob. palPalEntry);
		if (( nCol == 20) && ( *color == 256))
			*color = 20;
	}

	if ( nCol == 20) {
		int i;
		for ( i = 0; i < 10; i++)
			lpGlob. palPalEntry[ i + 10] = lpGlob. palPalEntry[ 255 - i];
	}

	r = ( PRGBColor) malloc( sizeof( RGBColor) * *color);
	if ( !r) return NULL;

	for ( i = 0; i < *color; i++) {
		r[i].r = lpGlob. palPalEntry[i]. peRed;
		r[i].g = lpGlob. palPalEntry[i]. peGreen;
		r[i].b = lpGlob. palPalEntry[i]. peBlue;
	}
	return r;
}

Point
apc_gp_get_resolution( Handle self)
{
	Point p = guts. display_resolution;
	if ( !self) return p;
	objCheck p;
	return is_apt( aptPrinter ) ? sys res : p;
}

static Handle ctx_rop2R2[] = {
	ropCopyPut       , R2_COPYPEN      ,
	ropXorPut        , R2_XORPEN       ,
	ropAndPut        , R2_MASKPEN      ,
	ropOrPut         , R2_MERGEPEN     ,
	ropNotPut        , R2_NOTCOPYPEN   ,
	ropNotDestAnd    , R2_MASKPENNOT   ,
	ropNotDestOr     , R2_MERGEPENNOT  ,
	ropNotSrcAnd     , R2_MASKNOTPEN   ,
	ropNotSrcOr      , R2_MERGENOTPEN  ,
	ropNotXor        , R2_NOTXORPEN    ,
	ropNotAnd        , R2_NOTMASKPEN   ,
	ropNotOr         , R2_NOTMERGEPEN  ,
	ropNoOper        , R2_NOP          ,
	ropBlackness     , R2_BLACK        ,
	ropWhiteness     , R2_WHITE        ,
	ropInvert        , R2_NOT          ,
	endCtx
};

int
apc_gp_get_rop( Handle self)
{
	objCheck 0;
	if ( !sys ps) return sys rop;
	return ctx_remap_def( GetROP2( sys ps), ctx_rop2R2, false, ropCopyPut);
}

int
apc_gp_get_rop2( Handle self)
{
	objCheck 0;
	if ( !sys ps) return sys rop2;
	return ( GetBkMode( sys ps) == OPAQUE) ? ropCopyPut : ropNoOper;
}

#define pal_ok ((sys bpp <= 8) && ( sys pal))

#define COLOR_CHANGE_FREE_BRUSH ( \
	( \
		var fillPatternImage && \
		PObject(var fillPatternImage)->stage == csNormal \
	) ? \
		((PImage(var fillPatternImage)->type == imBW) && !dsys(var fillPatternImage)options.aptIcon) : \
		true \
)

Bool
apc_gp_set_back_color( Handle self, Color color)
{
	long clr = remap_color( color, true);
	objCheck false;
	if ( sys ps) {
		if ( pal_ok) clr = palette_match( self, clr);
		if ( SetBkColor( sys ps, clr) == CLR_INVALID) apiErr;
		sys rq_brush.back_color = clr;
		if (
			sys rq_brush.logbrush.lbStyle == BS_DIBPATTERNPT &&
			COLOR_CHANGE_FREE_BRUSH
		)
			STYLUS_FREE_BRUSH;
	}
	sys bg = clr;
	return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
	long clr = remap_color( color, true);
	objCheck false;
	sys fg = clr;
	if ( sys ps) {
		if ( pal_ok) clr = palette_match( self, clr);
		sys rq_pen.logpen.lopnColor   = ( COLORREF) clr;
		sys rq_brush.logbrush.lbColor = ( COLORREF) (( sys rq_brush.logbrush.lbStyle == BS_DIBPATTERNPT) ? 0 : clr);
		sys rq_brush.color            = ( COLORREF) clr;
		STYLUS_FREE_PEN;
		STYLUS_FREE_GP_PEN;
		STYLUS_FREE_TEXT;
		if ( COLOR_CHANGE_FREE_BRUSH ) {
			STYLUS_FREE_BRUSH;
			STYLUS_FREE_GP_BRUSH;
		}
		if ( sys alpha_arena_palette ) {
			free(sys alpha_arena_palette);
			sys alpha_arena_palette = NULL;
		}
	}
	return true;
}

Bool
apc_gp_set_fill_mode( Handle self, int fill_mode)
{
	objCheck false;
	sys fill_mode = fill_mode;
	if ( sys ps)
		SetPolyFillMode( sys ps, ((fill_mode & fmWinding) == fmAlternate) ? ALTERNATE : WINDING);
	return true;
}

static Point
apply_fill_pattern_offset( Handle self )
{
	Point o = sys fill_pattern_offset;
	int w, h;
	if ( var fillPatternImage ) {
		if ( PObject(var fillPatternImage)->stage != csNormal ) {
			o.x = o.y = 0;
			return o;
		}
		Handle i = PDrawable(self)->fillPatternImage;
		h = PDrawable(i)-> h;
		w = PDrawable(i)-> w;
	} else {
		h = 8;
		w = 8;
	}

	o.y = sys last_size.y - o.y;
	o.x -= sys transform2.x;
	o.y -= sys transform2.y;
	while (o.x < 0) o.x += w;
	while (o.y < 0) o.y += h;
	o.x %= w;
	o.y %= h;

	return o;
}

Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
	objCheck false;
{
	HDC ps     = sys ps;
	PRQBrush b = & sys rq_brush;
	uint32_t *p1 = ( uint32_t*) pattern;
	uint32_t *p2 = p1 + 1;

	memcpy( &sys fill_pattern, pattern, sizeof( FillPattern));
	if ( !ps) return true;

	if (( *p1 == 0) && ( *p2 == 0)) {
		b-> logbrush.lbStyle = BS_SOLID;
		b-> logbrush.lbColor = GetBkColor( ps);
		b-> logbrush.lbHatch = 0;
		b-> back_color       = 0;
		memset( b-> fill_pattern, 0, sizeof(FillPattern) );
	} else if (( *p1 == 0xFFFFFFFF) && ( *p2 == 0xFFFFFFFF)) {
		b-> logbrush.lbStyle = BS_SOLID;
		b-> logbrush.lbColor = sys rq_pen.logpen.lopnColor;
		b-> logbrush.lbHatch = 0;
		b-> back_color       = 0;
		memset( b-> fill_pattern, 0, sizeof(FillPattern) );
	} else {
		Point offset;
		b-> logbrush.lbStyle = BS_DIBPATTERNPT;
		b-> logbrush.lbColor = DIB_RGB_COLORS;
		b-> logbrush.lbHatch = (LONG_PTR) 0;
		b-> back_color       = GetBkColor( ps);
		memcpy( b-> fill_pattern, pattern, sizeof( FillPattern));
		offset = apply_fill_pattern_offset(self);
		if ( CURRENT_GP_BRUSH != NULL ) {
			GdipResetTextureTransform(CURRENT_GP_BRUSH);
			GdipTranslateTextureTransform(CURRENT_GP_BRUSH,offset.x,offset.y,MatrixOrderPrepend);
		}
	}
	STYLUS_FREE_BRUSH;
	STYLUS_FREE_GP_BRUSH;

	return true;
}}

Bool
apc_gp_set_fill_image( Handle self, Handle image)
{
	objCheck false;
	if ( !sys ps || !image)
		return false;
	sys rq_brush.back_color = GetBkColor(sys ps);
	sys rq_brush.logbrush.lbStyle = BS_DIBPATTERNPT; /* for stylus_is_complex */
	STYLUS_FREE_BRUSH;
	STYLUS_FREE_GP_BRUSH;
	return true;
}

Bool
apc_gp_set_fill_pattern_offset( Handle self, Point offset)
{
	objCheck false;
	sys fill_pattern_offset = offset;
	if ( sys ps) {
		offset = apply_fill_pattern_offset(self);
		SetBrushOrgEx( sys ps, offset.x, offset.y, NULL);
		if ( CURRENT_GP_BRUSH != NULL ) {
			GdipResetTextureTransform(CURRENT_GP_BRUSH);
			GdipTranslateTextureTransform(CURRENT_GP_BRUSH,offset.x,offset.y,MatrixOrderPrepend);
		}
	}
	return true;
}

Bool
apc_gp_set_font( Handle self, PFont font)
{
	OUTLINETEXTMETRICW otm;
	objCheck false;
	if ( !sys ps) return true;
	font_change( self, font);

	if ( GetOutlineTextMetricsW(sys ps, sizeof(otm), &otm)) {
		sys tmOverhang             = otm.otmTextMetrics.tmOverhang;
		sys tmPitchAndFamily       = otm.otmTextMetrics.tmPitchAndFamily;
		sys otmsStrikeoutSize      = otm.otmsStrikeoutSize;
		sys otmsStrikeoutPosition  = otm.otmsStrikeoutPosition;
		sys otmsUnderscoreSize     = otm.otmsUnderscoreSize;
		sys otmsUnderscorePosition = otm.otmsUnderscorePosition;
	} else {
		TEXTMETRICW tm;
		GetTextMetricsW( sys ps, &tm);
		sys tmOverhang        = tm.tmOverhang;
		sys tmPitchAndFamily  = tm.tmPitchAndFamily;
		sys otmsStrikeoutSize = sys otmsStrikeoutPosition = sys otmsUnderscoreSize = sys otmsUnderscorePosition = -1;
	}

	sys font_sin = sys font_cos = 0.0;

	return true;
}

Bool
apc_gp_set_line_pattern( Handle self, unsigned char * pattern, int len)
{
	objCheck false;
	if ( !sys ps) {
		if ( sys line_pattern_len > sizeof(sys line_pattern))
			free( sys line_pattern);
		if ( len > sizeof(sys line_pattern)) {
			sys line_pattern = ( unsigned char *) malloc( len);
			if ( !sys line_pattern) {
				sys line_pattern_len = 0;
				return false;
			}
			memcpy( sys line_pattern, pattern, len);
		} else
			memcpy( &sys line_pattern, pattern, len);
		sys line_pattern_len = len;
	} else {
		sys rq_pen.logpen.lopnStyle  = patres_user( pattern, len);
		sys rq_pen.line_pattern = &std_hollow_line_pattern;
		if (( sys rq_pen.geometric = stylus_is_geometric(self))) {
			sys rq_pen.style        = sys rq_pen.logpen.lopnStyle | PS_GEOMETRIC;
			sys rq_pen.line_pattern = patres_fetch( pattern, len);
		}
		STYLUS_FREE_PEN;
		STYLUS_FREE_GP_PEN;
	}
	return true;
}

Bool
apc_gp_set_palette( Handle self)
{
	HPALETTE pal;

	objCheck false;
	if ( sys p256) {
		free( sys p256);
		sys p256 = NULL;
	}

	pal = palette_create( self);
	if ( sys ps) {
		if ( pal)
			SelectPalette( sys ps, pal, 0);
		else
			SelectPalette( sys ps, sys stock_palette, 1);
		RealizePalette( sys ps);
	}
	if ( sys pal) DeleteObject( sys pal);
	sys pal = pal;
	return true;
}

Bool
apc_gp_set_rop( Handle self, int rop)
{
	objCheck false;
	if ( !sys ps) { sys rop = rop; return true; }
	sys rop = ctx_remap_def( rop, ctx_rop2R2, true, R2_COPYPEN);
	if ( !SetROP2( sys ps, sys rop)) apiErr;
	return true;
}

Bool
apc_gp_set_rop2( Handle self, int rop)
{
	objCheck false;
	if ( !sys ps) { sys rop2 = rop; return true; }
	if ( rop != ropCopyPut) rop = ropNoOper;
	sys rop2 = rop;
	if ( !SetBkMode( sys ps, ( rop == ropCopyPut) ? OPAQUE : TRANSPARENT)) apiErr;
	STYLUS_FREE_GP_BRUSH;
	return true;
}

Bool
apc_gp_push(Handle self, GCStorageFunction * destructor, void * user_data, unsigned int user_data_size)
{
	int size;
	PPaintState state;

	if ( !sys gc_stack ) {
		if ( !( sys gc_stack = plist_create(4,4))) return false;
	}
	size = sizeof(PaintState) + user_data_size;
	if ( !( state = malloc(size))) return false;
	if ( list_add( sys gc_stack, (Handle) state) < 0) return false;

	bzero(state, sizeof(PaintState));
	state->user_data = state->user_data_buf;
	memcpy( state-> user_data, user_data, user_data_size);
	state->user_data_size = user_data_size;
	state->user_destructor = destructor;

	state->in_paint = (sys ps != 0);

	if ( sys ps ) {
		int i;
		if ( !( SaveDC(sys ps))) {
			list_delete_at( sys gc_stack, sys gc_stack->count - 1);
			free(state);
			return false;
		}
		memcpy( state->paint.dc_obj, sys current_dc_obj, sizeof(sys current_dc_obj));
		state->paint.stylus_flags     = sys stylus_flags;
		state->paint.dc_font          = sys dc_font;
		for ( i = 0; i < DCO_COUNT; i++)
			if ( state->paint.dc_obj[i] )
				state->paint.dc_obj[i]->refcnt++;
		state->paint.font_sin  = sys font_sin;
		state->paint.font_cos  = sys font_cos;

		state->paint.rq_pen      = sys rq_pen;
		state->paint.rq_brush    = sys rq_brush;
	} else {
		state->common.line_pattern_len = sys line_pattern_len;
		if ( state->common.line_pattern_len > sizeof(sys line_pattern)) {
			if (( state->common.line_pattern = malloc(state->common.line_pattern_len)) != NULL)
				memcpy( state->common.line_pattern, sys line_pattern, state->common.line_pattern_len);
			else
				state->common.line_pattern_len = 0;
		} else
			state->common.line_pattern = sys line_pattern;
		state->nonpaint.palette = palette_create(self);
	}

	memcpy( state->common.fill_pattern, sys fill_pattern, sizeof(FillPattern));
	state->common.fill_pattern_offset = sys fill_pattern_offset;
	state->common.fill_mode     = sys fill_mode;
	state->common.rop           = sys rop;
	state->common.rop2          = sys rop2;
	state->common.antialias     = is_apt(aptGDIPlus);
	state->common.alpha         = sys alpha;
	state->common.fg            = sys fg;
	state->common.bg            = sys bg;
	state->common.font          = var font;
	state->common.text_out_baseline = is_apt( aptTextOutBaseline);
	state->common.text_opaque   = is_apt( aptTextOpaque);
	if ( var fillPatternImage )
		protect_object( state->fill_image = var fillPatternImage );
	return true;
}

Bool
apc_gp_pop( Handle self, void * user_data)
{
	PPaintState state;

	if ( !sys gc_stack ) return false;
	if ( sys gc_stack-> count <= 0 ) return false;
	if ( !( state = ( PPaintState) list_at( sys gc_stack, sys gc_stack-> count - 1))) return false;
	list_delete_at( sys gc_stack, sys gc_stack->count - 1);

	if ( user_data )
		memcpy( user_data, state->user_data, state->user_data_size);

	if ( state-> in_paint ) {
		stylus_release(self);
		RestoreDC( sys ps, -1);
		memcpy( sys current_dc_obj, state->paint.dc_obj, sizeof(sys current_dc_obj));
		sys stylus_flags        = state-> paint.stylus_flags;
		sys dc_font             = state-> paint.dc_font;
		sys font_sin            = state-> paint.font_sin;
		sys font_cos            = state-> paint.font_cos;
		sys rq_pen              = state-> paint.rq_pen;
		sys rq_brush            = state-> paint.rq_brush;

		sys pal                 = GetCurrentObject(sys ps, OBJ_PAL);

		if (sys graphics) {
			HRGN rgn;
			int res;
			rgn = CreateRectRgn(0,0,0,0);
			res = GetClipRgn( sys ps, rgn );
			if ( res <= 0 ) {
				if ( res < 0 ) apiErr;
				DeleteObject(rgn);
				rgn = CreateRectRgn(0,0,sys last_size.x,sys last_size.y);
			}
			GPCALL GdipSetClipHrgn(sys graphics, rgn, CombineModeReplace);
			apiGPErrCheck;
			DeleteObject(rgn);
		}
		if ( CURRENT_GP_BRUSH != NULL ) {
			POINT offset;
			GetBrushOrgEx( sys ps, &offset);
			GdipResetTextureTransform(CURRENT_GP_BRUSH);
			GdipTranslateTextureTransform(CURRENT_GP_BRUSH,offset.x,offset.y,MatrixOrderPrepend);
		}
	} else {
		if ( sys line_pattern_len > sizeof(sys line_pattern)) 
			free(sys line_pattern);
		sys line_pattern_len = state->common.line_pattern_len;
		sys line_pattern    = state->common.line_pattern;
		if ( sys pal ) DeleteObject( sys pal);
		sys pal            = state-> nonpaint.palette;
	}

	memcpy( sys fill_pattern, state->common.fill_pattern, sizeof(FillPattern));
	sys fill_pattern_offset = state->common.fill_pattern_offset;
	sys fill_mode           = state->common.fill_mode;
	sys rop                 = state->common.rop;
	sys rop2                = state->common.rop2;

	sys alpha  = state->common.alpha;
	apc_gp_set_antialias( self, state->common.antialias );
	var font   = state->common.font;
	sys fg     = state->common.fg;
	sys bg     = state->common.bg;
	if (var fillPatternImage)
		unprotect_object(var fillPatternImage);
	var fillPatternImage = state-> fill_image;
	apt_assign(aptTextOutBaseline, state->common.text_out_baseline);
	apt_assign(aptTextOpaque,      state->common.text_opaque);
	if ( sys alpha_arena_palette ) {
		free(sys alpha_arena_palette);
		sys alpha_arena_palette = NULL;
	}

	free(state);

	return true;
}

#ifdef __cplusplus
}
#endif
