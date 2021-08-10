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
	sys lineWidth = 1.0;
	return true;
}

Bool
apc_gp_done( Handle self)
{
	objCheck false;
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
			if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
		} else if ( is_apt( aptWinPS)) {
			if ( self == application)
				dc_free();
			else {
				if ( !ReleaseDC(( HWND) var handle,  sys ps)) apiErr;
			}
		}
	}
	if ( sys linePatternLen  > sizeof(sys linePattern)) free( sys linePattern);
	stylus_free( sys stylusResource, false); /* XXX check this */
	stylus_gp_free( sys stylusGPResource, false);
	if ( sys linePatternLen  > sizeof(sys linePattern)) free( sys linePattern);
	font_free( sys fontResource, false);
	if ( sys p256) free( sys p256);
	sys bm = NULL;
	sys pal = NULL;
	sys ps = NULL;
	sys bm = NULL;
	sys p256 = NULL;
	sys fontResource = NULL;
	sys stylusResource = NULL;
	sys stylusGPResource = NULL;
	sys linePattern = NULL;
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

static Bool
gp_Arc(
	Handle self,
	int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
	int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2,
	double angleStart, double angleEnd
) {
	if ( nXRadial1 == nXRadial2 && nYRadial1 == nYRadial2 && fabs(angleStart - angleEnd) < 360 ) {
		Bool ret;
		HGDIOBJ old;

		old = SelectObject( sys ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		adjust_line_end_int( nXRadial1, nYRadial1, &nXRadial2, &nYRadial2);
		MoveToEx( sys ps, nXRadial1, nYRadial1, NULL);
		ret = LineTo( sys ps, nXRadial2, nYRadial2);
		DeleteObject(SelectObject( sys ps, old) );
		return ret;
	} else {
		return Arc(
			sys ps,
			nLeftRect,  nTopRect,  nRightRect,  nBottomRect,
			nXRadial1,  nYRadial1,  nXRadial2,  nYRadial2
		);
	}
}

static Bool
gp_Chord(
  Handle self,
  int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
  int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2,
  double angleStart, double angleEnd, Bool filled
) {
	if (
		(abs(nXRadial1 - nXRadial2) < 2) &&
		(abs(nYRadial1 == nYRadial2) < 2) &&
		(fabs(angleStart - angleEnd) < 360 )
	) {
		Bool ret;
		HGDIOBJ old;

		if ( filled ) {
			nXRadial2--;
			nYRadial2--;
		}

		old = SelectObject( sys ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		adjust_line_end_int( nXRadial1, nYRadial1, &nXRadial2, &nYRadial2);
		MoveToEx( sys ps, nXRadial1, nYRadial1, NULL);
		ret = LineTo( sys ps, nXRadial2, nYRadial2);
		DeleteObject(SelectObject( sys ps, old) );
		return ret;
	} else {
		return Chord(
			sys ps,
			nLeftRect,  nTopRect,  nRightRect,  nBottomRect,
			nXRadial1,  nYRadial1,  nXRadial2,  nYRadial2
		);
	}
}

static Bool
gp_Pie(
	Handle self,
	int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
	int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2,
	double angleStart, double angleEnd, Bool filled
) {
	if ( nXRadial1 == nXRadial2 && nYRadial1 == nYRadial2 && fabs(angleStart - angleEnd) < 360 ) {
		int cx, cy;
		Bool ret;
		HGDIOBJ old;

		old = SelectObject( sys ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		cx  = ( nLeftRect + nRightRect ) / 2;
		cy  = ( nTopRect  + nBottomRect ) / 2;
		adjust_line_end_int( cx, cy, &nXRadial1, &nYRadial1);
		if ( filled ) {
			if ( nXRadial2 > cx ) nXRadial1 = nXRadial2;
			if ( nYRadial2 > cy ) nYRadial1 = nYRadial2;
		}
		MoveToEx( sys ps, cx, cy, NULL);
		ret = LineTo( sys ps, nXRadial1, nYRadial1);
		DeleteObject(SelectObject( sys ps, old) );
		return ret;
	} else {
		return Pie(
			sys ps,
			nLeftRect,  nTopRect,  nRightRect,  nBottomRect,
			nXRadial1,  nYRadial1,  nXRadial2,  nYRadial2
		);
	}
}


#define GRAD 57.29577951

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

#define ELLIPSE_RECT (int)(x - ( dX - 1) / 2), (int)(y - dY / 2), (int)(x + dX / 2 + 1), (int)(y + (dY - 1) / 2 + 1)
#define ELLIPSE_RECT_SUPERINCLUSIVE (int)(x - ( dX - 1) / 2), (int)(y - dY / 2), (int)(x + dX / 2 + 2), (int)(y + (dY - 1) / 2 + 2)
#define ARC_COMPLETE (int)(x + dX / 2 + 1), y, (int)(x + dX / 2 + 1), y
#define ARC_ANGLED   (int)(x + cos( angleStart / GRAD) * dX / 2 + 0.5), (int)(y - sin( angleStart / GRAD) * dY / 2 + 0.5), \
							(int)(x + cos( angleEnd / GRAD) * dX / 2 + 0.5),   (int)(y - sin( angleEnd / GRAD) * dY / 2 + 0.5)
#define ARC_ANGLED_SUPERINCLUSIVE   (int)(x + cos( angleStart / GRAD) * dX / 2 + 0.5), (int)(y - sin( angleStart / GRAD) * dY / 2 + 0.5), \
							(int)(x + cos( angleEnd / GRAD) * dX / 2 + 1.5),   (int)(y - sin( angleEnd / GRAD) * dY / 2 + 1.5)

#define EMULATE_OPAQUE_LINE \
(sys stylus. pen. lopnStyle == PS_USERSTYLE && sys currentROP2 == ropCopyPut)
#define STYLUS_USE_OPAQUE_LINE \
	sys stylusFlags &=~ stbPen;\
	if ( !sys opaquePen) sys opaquePen = CreatePen( PS_SOLID, sys stylus.pen.lopnWidth.x, sys lbs[1]);\
	SelectObject(sys ps, sys opaquePen);\
	if (sys currentROP != R2_COPYPEN) SetROP2( sys ps, R2_COPYPEN)
#define STYLUS_RESTORE_OPAQUE_LINE if (sys currentROP != R2_COPYPEN) SetROP2( sys ps, sys currentROP)

Bool
apc_gp_arc( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{ objCheck false; {
	int compl, needf;
	HDC ps = sys ps;

	y = sys lastSize. y - y - 1;
	compl = arc_completion( &angleStart, &angleEnd, &needf);

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		if ( compl )
			Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
		else
			gp_Arc( self, ELLIPSE_RECT, ARC_ANGLED, angleStart, angleEnd);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( ps);
	while( compl--)
		Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
	if ( !needf) return true;
	if ( !gp_Arc( self, ELLIPSE_RECT, ARC_ANGLED, angleStart, angleEnd)) apiErrRet;
	return true;
}}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
	HDC     ps = sys ps;
	HGDIOBJ old;
	Bool ok;

	check_swap( x1, x2);
	check_swap( y1, y2);

	ok = true;
	old = SelectObject( ps, hPenHollow);
	STYLUS_USE_BRUSH( ps);
	if ( !( ok = Rectangle( ps, x1, sys lastSize. y - y2 - 1, x2 + 2, sys lastSize. y - y1 + 1))) apiErr;
	SelectObject( ps, old);
	return ok;
}}

Bool
apc_gp_bars( Handle self, int nr, Rect *rr)
{objCheck false;{
	HDC     ps = sys ps;
	HGDIOBJ old = SelectObject( ps, hPenHollow);
	Bool ok = true;
	int i, y = sys lastSize. y;
	STYLUS_USE_BRUSH( ps);
	for ( i = 0; i < nr; i++, rr++) {
		Rect xr = *rr;
		check_swap( xr.left, xr.right);
		check_swap( xr.bottom, xr.top);
		if ( !( ok = Rectangle( ps, xr.left, y - xr.top - 1, xr.left + 2, y - xr.bottom + 1))) {
			apiErr;
			break;
		}
	}
	SelectObject( ps, old);
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
		x2 = sys lastSize. x - 1;
		y2 = sys lastSize. y - 1;
	}
	check_swap( x1, x2);
	check_swap( y1, y2);
	w = x2 - x1 + 1;
	h = y2 - y1 + 1;
	y1 = sys lastSize. y - y2 - 1;

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
	HGDIOBJ  oldp = SelectObject( ps, hPenHollow);
	LOGBRUSH ers;
	HGDIOBJ  oldh;
	ers. lbStyle = PS_SOLID;
	ers. lbColor = sys lbs[ 1];
	ers. lbHatch = 0;
	oldh = CreateBrushIndirect( &ers);
	oldh = SelectObject( ps, oldh);
	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
		x1 = y1 = 0;
		x2 = sys lastSize. x - 1;
		y2 = sys lastSize. y - 1;
	}
	check_swap( x1, x2);
	check_swap( y1, y2);
	if ( !( ok = Rectangle( sys ps, x1, sys lastSize. y - y2 - 1, x2 + 2, sys lastSize. y - y1 + 1))) apiErr;
	SelectObject( ps, oldp);
	DeleteObject( SelectObject( ps, oldh));
	return ok;
}}

Bool
apc_gp_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
	Bool ok = true;
	HDC     ps = sys ps;
	HGDIOBJ old = SelectObject( ps, hBrushHollow);
	int compl, needf;
	compl = arc_completion( &angleStart, &angleEnd, &needf);
	y = sys lastSize. y - y - 1;

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		if ( compl ) Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
		gp_Chord( self, ELLIPSE_RECT, ARC_ANGLED, angleStart, angleEnd, false);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( ps);
	while( compl--)
		Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
	if ( needf) {
		if ( !( ok = gp_Chord( self, ELLIPSE_RECT, ARC_ANGLED, angleStart, angleEnd, false))) apiErr;
	}
	SelectObject( ps, old);
	return ok;
}}

Bool
apc_gp_draw_poly( Handle self, int numPts, Point * points)
{objCheck false;{
	int i, dy = sys lastSize. y;
	POINT *p;
	Bool ok;

	if ((p = malloc( sizeof(POINT) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i]. x = points[i]. x;
		p[i]. y = dy - points[i]. y - 1;
	}
	if ( p[0]. x != p[numPts - 1].x || p[0]. y != p[numPts - 1].y || numPts == 2)
		adjust_line_end_LONG( p[numPts - 2].x, p[numPts - 2].y, &p[numPts - 1].x, &p[numPts - 1].y);

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		Polyline( sys ps, p, numPts);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( sys ps);
	if ( !(ok = Polyline( sys ps, p, numPts))) apiErr;

	free(p);

	return ok;
}}

Bool
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{objCheck false;{
	Bool ok = true;
	int i, dy = sys lastSize. y;
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
		p[i]. x = points[i]. x;
		p[i]. y = dy - points[i]. y - 1;
		pts[ i] = 2;
		if ( i & 1)
			adjust_line_end_LONG( p[i - 1].x, p[i - 1].y, &p[i].x, &p[i].y);
	}

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		PolyPolyline( sys ps, p, pts, numPts/2);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( sys ps);
	if ( !( ok = PolyPolyline( sys ps, p, pts, numPts/2))) apiErr;
	free( pts);
	free( p);
	return ok;
}}

Bool
apc_gp_ellipse( Handle self, int x, int y, int dX, int dY)
{objCheck false;{
	Bool    ok = true;
	HDC     ps = sys ps;
	HGDIOBJ old = SelectObject( ps, hBrushHollow);

	y = sys lastSize. y - y - 1;

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		Ellipse( ps, ELLIPSE_RECT);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( ps);
	if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
	SelectObject( ps, old);
	return ok;
}}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
	Bool ok = true;
	HDC     ps = sys ps;
	HGDIOBJ old;
	Bool   comp;
	int compl, needf;

	compl = arc_completion( &angleStart, &angleEnd, &needf);
	comp = ((sys psFillMode & fmOverlay) == 0) || stylus_complex( &sys stylus, ps);
	y = sys lastSize. y - y - 1;
	STYLUS_USE_BRUSH( ps);

	if ( comp) {
		old  = SelectObject( ps, hPenHollow);
		while ( compl--)
			if ( !( ok = Ellipse( ps, ELLIPSE_RECT_SUPERINCLUSIVE))) apiErr;
		if ( !( ok = !needf || gp_Chord(
			self, ELLIPSE_RECT_SUPERINCLUSIVE, ARC_ANGLED_SUPERINCLUSIVE, angleStart, angleEnd, true
		))) apiErr;
	} else {
		old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		while ( compl--)
			if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
		if ( !( ok = !needf || gp_Chord(
			self, ELLIPSE_RECT, ARC_ANGLED_SUPERINCLUSIVE, angleStart, angleEnd, true
		))) apiErr;
	}
	old = SelectObject( ps, old);
	if ( !comp) DeleteObject( old);
	return ok;
}}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y, int dX, int dY)
{objCheck false;{
	Bool ok = true;
	HDC     ps  = sys ps;
	HGDIOBJ old;
	Bool    comp = ((sys psFillMode & fmOverlay) == 0) || stylus_complex( &sys stylus, ps);
	STYLUS_USE_BRUSH( ps);
	y = sys lastSize. y - y - 1;
	if ( comp) {
		old  = SelectObject( ps, hPenHollow);
		if ( !( ok = Ellipse( ps, ELLIPSE_RECT_SUPERINCLUSIVE))) apiErr;
	} else {
		old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
	}
	old = SelectObject( ps, old);
	if ( !comp) DeleteObject( old);
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
	int i,  dy = sys lastSize. y;
	POINT *p;

	if ((p = malloc( sizeof(POINT) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i]. x = points[i]. x;
		p[i]. y = dy - points[i]. y - 1;
	}
	if ( numPts == 2 )
		adjust_line_end_LONG( p[0].x, p[0].y, &p[1].x, &p[1].y);

	if (( sys psFillMode & fmOverlay) == 0) {
		HGDIOBJ old = SelectObject( ps, hPenHollow);
		STYLUS_USE_BRUSH( ps);
		if ( !( ok = Polygon( ps, p, numPts))) apiErr;
		SelectObject( ps, old);
	} else if ( !stylus_complex( &sys stylus, ps)) {
		HPEN old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		STYLUS_USE_BRUSH( ps);
		if ( !( ok = Polygon( ps, p, numPts))) apiErr;
		DeleteObject( SelectObject( ps, old));
	} else {
		int dx    = sys lastSize. x;
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
			if ( p[ i]. x > bound. x) bound. x = p[ i]. x;
			if ( p[ i]. y > bound. y) bound. y = p[ i]. y;
			if ( p[ i] .x < trans. x) trans. x = p[ i]. x;
			if ( p[ i] .y < trans. y) trans. y = p[ i]. y;
		}
		if (( trans. x == dx) || ( trans. y == dy)) {
			free(p);
			return false;
		}
		if ( bound. x > dx) bound. x = dx;
		if ( bound. y > dy) bound. y = dy;
		for ( i = 0; i < numPts; i++)  {
			p[ i]. x -= trans. x;
			p[ i]. y -= trans. y;
		}
		bound. x -= trans. x - 1;
		bound. y -= trans. y - 1;

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
		old1 = SelectObject( dc, sys stylusResource-> hbrush);
		oldelta = SelectObject( dc, hPenHollow);
		Rectangle( dc, 0, 0, bound. x + 1, bound. y + 1);
		SelectObject( dc, oldelta);
		SelectObject( dc, old1);
		SelectObject( dc, bmMask);
		SetROP2( dc, R2_WHITE);
		Rectangle( dc, 0, 0, bound. x, bound. y);
		SetROP2( dc, R2_BLACK);
		if ( !( ok = Polygon( dc, p, numPts))) apiErr;
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
apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points)
{
	int i;
	double dy = sys lastSize. y;
	GpPointF *p;
	objCheck false;

	if ((p = malloc( sizeof(GpPointF) * numPts)) == NULL)
		return false;

	for ( i = 0; i < numPts; i++)  {
		p[i].X = points[i].x;
		p[i].Y = dy - points[i].y - 1.0;
	}

	STYLUS_USE_GP_BRUSH;
	GPCALL GdipFillPolygon( sys graphics, sys stylusGPResource-> brush, p, numPts,
		((sys psFillMode & fmWinding) == fmAlternate) ? ALTERNATE : WINDING);
	apiGPErrCheckRet(false);

	return true;
}

Bool
apc_gp_fill_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
	Bool ok = true;
	HDC     ps = sys ps;
	HGDIOBJ old;
	int newY  = sys lastSize. y - y - 1;
	POINT   pts[ 3];
	Bool comp;
	int compl, needf;

	compl = arc_completion( &angleStart, &angleEnd, &needf);
	comp = ((sys psFillMode & fmOverlay) == 0) || stylus_complex( &sys stylus, ps);

	pts[ 0]. x = x + cos( angleEnd / GRAD) * dX / 2 + 0.5;
	pts[ 0]. y = newY - sin( angleEnd / GRAD) * dY / 2 + 0.5;
	pts[ 1]. x = x + cos( angleStart / GRAD) * dX / 2 + 0.5;
	pts[ 1]. y = newY - sin( angleStart / GRAD) * dY / 2 + 0.5;

	STYLUS_USE_BRUSH( ps);
	y = newY;
	if ( comp) {
		old = SelectObject( ps, hPenHollow);
		while ( compl--)
			if ( !( ok = Ellipse( ps, ELLIPSE_RECT_SUPERINCLUSIVE))) apiErr;
		if ( !( ok = !needf || gp_Pie(
			self, ELLIPSE_RECT_SUPERINCLUSIVE,
			pts[ 1]. x, pts[ 1]. y,
			pts[ 0]. x, pts[ 0]. y,
			angleStart, angleEnd, true
		))) apiErr;
	} else {
		old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
		while ( compl--)
			if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
		if ( !( ok = !needf || gp_Pie(
			self, ELLIPSE_RECT,
			pts[ 1]. x, pts[ 1]. y,
			pts[ 0]. x, pts[ 0]. y,
			angleStart, angleEnd, true
		))) apiErr;
	}
	old = SelectObject( ps, old);
	if ( !comp) DeleteObject( old);
	return ok;
}}

Bool
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{objCheck false;{
	HDC ps = sys ps;
	STYLUS_USE_BRUSH( ps);
	if ( !ExtFloodFill( ps, x, sys lastSize. y - y - 1, remap_color( borderColor, true),
		singleBorder ? FLOODFILLSURFACE : FLOODFILLBORDER)) apiErrRet;
	return true;
}}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{objCheck clInvalid;{
	COLORREF c = GetPixel( sys ps, x, sys lastSize. y - y - 1);
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
	y1 = sys lastSize. y - y1 - 1;
	y2 = sys lastSize. y - y2 - 1;

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		MoveToEx( ps, x1, y1, NULL);
		LineTo( ps, x2, y2);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( ps);

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

	old = SelectObject( ps, hBrushHollow);
	if ( sys stylus. pen. lopnWidth. x > 1 &&
		(sys stylus. pen. lopnWidth. x % 2) == 0
		) {
		/* change up-winding to down-winding */
		y1--;
		y2--;
	}

	if ( EMULATE_OPAQUE_LINE ) {
		STYLUS_USE_OPAQUE_LINE;
		Rectangle( sys ps, x1, sys lastSize. y - y1, x2 + 1, sys lastSize. y - y2 - 1);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( ps);
	if ( !( ok = Rectangle( sys ps, x1, sys lastSize. y - y1, x2 + 1, sys lastSize. y - y2 - 1))) apiErr;
	SelectObject( ps, old);
	return ok;
}}

Bool
apc_gp_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
	Bool ok = true;
	HDC     ps = sys ps;
	int compl, needf, newY = sys lastSize. y - y - 1;
	POINT   pts[ 2];
	HGDIOBJ old;

	compl = arc_completion( &angleStart, &angleEnd, &needf);
	old = SelectObject( ps, hBrushHollow);
	pts[ 0]. x = x + cos( angleEnd / GRAD) * dX / 2 + 0.5;
	pts[ 0]. y = newY - sin( angleEnd / GRAD) * dY / 2 + 0.5;
	pts[ 1]. x = x + cos( angleStart / GRAD) * dX / 2 + 0.5;
	pts[ 1]. y = newY - sin( angleStart / GRAD) * dY / 2 + 0.5;
	y = newY;

	if (EMULATE_OPAQUE_LINE) {
		STYLUS_USE_OPAQUE_LINE;
		if ( compl ) Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
		gp_Pie(
			self, ELLIPSE_RECT,
			pts[ 1]. x, pts[ 1]. y,
			pts[ 0]. x, pts[ 0]. y,
			angleStart, angleEnd, false
		);
		STYLUS_RESTORE_OPAQUE_LINE;
	}

	STYLUS_USE_PEN( ps);

	while( compl--)
		Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
	if ( needf) {
		if ( !( ok = gp_Pie(
			self, ELLIPSE_RECT,
			pts[ 1]. x, pts[ 1]. y,
			pts[ 0]. x, pts[ 0]. y,
			angleStart, angleEnd, false
		))) apiErr;
	}

	SelectObject( ps, old);
	return ok;
}}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
	objCheck false;
	SetPixelV( sys ps, x, sys lastSize. y - y - 1, remap_color( color, true));
	return true;
}

// gpi settings
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

Color
apc_gp_get_back_color( Handle self)
{
	objCheck 0;
	return remap_color( sys lbs[1], false);
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
	return remap_color( sys ps ? sys stylus. pen. lopnColor : sys lbs[0], false);
}

int
apc_gp_get_fill_mode( Handle self)
{
	objCheck 0;
	return sys ps ? sys psFillMode : sys fillMode;
}

static Handle ctx_le2PS_ENDCAP[] = {
	leRound,          PS_ENDCAP_ROUND             ,
	leSquare,         PS_ENDCAP_SQUARE            ,
	leFlat,           PS_ENDCAP_FLAT              ,
	endCtx
};

int
apc_gp_get_line_end( Handle self)
{
	objCheck 0;
	if ( !sys ps) return sys lineEnd;
	return ctx_remap_def( sys stylus. extPen. lineEnd, ctx_le2PS_ENDCAP, false, leRound);
}

static Handle ctx_lj2PS_JOIN[] = {
	ljRound,          PS_JOIN_ROUND             ,
	ljBevel,          PS_JOIN_BEVEL             ,
	ljMiter,          PS_JOIN_MITER             ,
	endCtx
};

int
apc_gp_get_line_join( Handle self)
{
	objCheck 0;
	if ( !sys ps) return sys lineJoin;
	return ctx_remap_def( sys stylus. extPen. lineJoin, ctx_lj2PS_JOIN, false, ljRound);
}

float
apc_gp_get_line_width( Handle self)
{
	objCheck 0;
	return sys ps ? sys stylus.extPen.lineWidth : sys lineWidth;
}

int
apc_gp_get_line_pattern( Handle self, unsigned char * buffer)
{
	objCheck 0;
	if ( !sys ps) {
		memcpy(
			( char *) buffer,
			(char*)(( sys linePatternLen > sizeof(sys linePattern)) ? sys linePattern : (Byte*)(&sys linePattern)),
			sys linePatternLen
		);
		return sys linePatternLen;
	}

	switch ( sys stylus. pen. lopnStyle) {
	case PS_NULL:
		strcpy(( char *) buffer, "");
		return 0;
	case PS_DASH:
		strcpy(( char *) buffer, psDash);
		return 2;
	case PS_DOT:
		strcpy(( char *) buffer, psDot);
		return 2;
	case PS_DASHDOT:
		strcpy(( char *) buffer, psDashDot);
		return 4;
	case PS_DASHDOTDOT:
		strcpy(( char *) buffer, psDashDotDot);
		return 6;
	case PS_USERSTYLE:
		{
			int i;
			int len = sys stylus. extPen. patResource-> dotsCount;
			if ( len > 255) len = 255;
			for ( i = 0; i < len; i++)
				buffer[ i] = sys stylus. extPen. patResource-> dots[ i];
			return len;
		}
	default:
		strcpy(( char *) buffer, "\1");
		return 1;
	}
}

float
apc_gp_get_miter_limit( Handle self)
{
	FLOAT ml;
	objCheck 0;
	if ( !sys ps) return sys miterLimit;
	if (! GetMiterLimit( sys ps, &ml)) return 0;
	return (float)ml;
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
	Point p = guts. displayResolution;
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

Point
apc_gp_get_transform( Handle self)
{
	Point p = {0,0};
	objCheck p;
	if ( !sys ps) return sys transform;
	if ( !GetViewportOrgEx( sys ps, (POINT*)&p)) apiErr;
	p. y = -p. y;
	p. x += sys transform2. x;
	p. y -= sys transform2. y;
	return p;
}

#define pal_ok ((sys bpp <= 8) && ( sys pal))

Bool
apc_gp_set_alpha( Handle self, int alpha)
{
	sys alpha = alpha;
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
		if ( sys ps && sys graphics == NULL ) {
			GPCALL GdipCreateFromHDC(sys ps, &sys graphics);
			apiGPErrCheckRet(false);
		}
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

Bool
apc_gp_set_back_color( Handle self, Color color)
{
	long clr = remap_color( color, true);
	objCheck false;
	if ( sys ps) {
		PStylus s = & sys stylus;
		if ( pal_ok) clr = palette_match( self, clr);
		if ( SetBkColor( sys ps, clr) == CLR_INVALID) apiErr;
		s-> brush. backColor = clr;
		if ( s-> brush. lb. lbStyle == BS_DIBPATTERNPT)
			stylus_change( self);
		if ( sys opaquePen ) {
			sys stylusFlags &=~ stbPen;
			SelectObject( sys ps, sys stockPen);
			DeleteObject( sys opaquePen );
			sys opaquePen = NULL;
		}
	}
	sys lbs[1] = clr;
	return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
	long clr = remap_color( color, true);
	objCheck false;
	sys lbs[0] = clr;
	if ( sys ps) {
		PStylus s = & sys stylus;
		if ( pal_ok) clr = palette_match( self, clr);
		s-> pen. lopnColor = ( COLORREF) clr;
		if ( s-> brush. lb. lbStyle != BS_DIBPATTERNPT) s-> brush. lb. lbColor = ( COLORREF) clr;
		stylus_change( self);
	}
	return true;
}

Bool
apc_gp_set_fill_mode( Handle self, int fillMode)
{
	objCheck false;
	if ( sys ps) {
		SetPolyFillMode( sys ps, ((fillMode & fmWinding) == fmAlternate) ? ALTERNATE : WINDING);
		sys psFillMode = fillMode;
	} else
		sys fillMode = fillMode;
	return true;
}

Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
	objCheck false;
{
	HDC ps    = sys ps;
	PStylus s = & sys stylus;
	uint32_t *p1 = ( uint32_t*) pattern;
	uint32_t *p2 = p1 + 1;
	if ( !ps) {
		memcpy( &sys fillPattern2, pattern, sizeof( FillPattern));
		return true;
	}
	memcpy( &sys fillPattern, pattern, sizeof( FillPattern));
	if (( *p1 == 0) && ( *p2 == 0)) {
		s-> brush. lb. lbStyle = BS_SOLID;
		s-> brush. lb. lbColor = GetBkColor( ps);
		s-> brush. lb. lbHatch = 0;
		s-> brush. backColor   = 0;
		memset( s-> brush. pattern, 0, sizeof( s-> brush. pattern));
	} else if (( *p1 == 0xFFFFFFFF) && ( *p2 == 0xFFFFFFFF)) {
		s-> brush. lb. lbStyle = BS_SOLID;
		s-> brush. lb. lbColor = s-> pen. lopnColor;
		s-> brush. lb. lbHatch = 0;
		s-> brush. backColor   = 0;
		memset( s-> brush. pattern, 0, sizeof( s-> brush. pattern));
	} else {
		s-> brush. lb. lbStyle = BS_DIBPATTERNPT;
		s-> brush. lb. lbColor = DIB_RGB_COLORS;
		s-> brush. lb. lbHatch = ( LONG_PTR) &bmiHatch;
		s-> brush. backColor   = GetBkColor( ps);
		memcpy( s-> brush. pattern, pattern, sizeof( FillPattern));
	}
	stylus_change( self);
	return true;
}}

Bool
apc_gp_set_fill_pattern_offset( Handle self, Point offset)
{
	objCheck false;
	if ( sys ps)
		SetBrushOrgEx( sys ps, offset.x, 8 - offset.y, NULL);
	else
		sys fillPatternOffset = offset;
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

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
	objCheck NULL;
	return sys ps ? &sys fillPattern : &sys fillPattern2;
}

Point
apc_gp_get_fill_pattern_offset( Handle self)
{
	Point p = {0,0};
	POINT wp;
	objCheck p;
	if ( !sys ps)
		return sys fillPatternOffset;
	GetBrushOrgEx( sys ps, &wp);
	p. x = wp. x;
	p. y = 8 - wp. y;
	return p;
}

Bool
apc_gp_set_line_end( Handle self, int lineEnd)
{
	objCheck false;
	if ( !sys ps) {
		sys lineEnd = lineEnd;
	} else {
		PStylus s         = &sys stylus;
		PEXTPEN ep        = &s-> extPen;
		ep-> lineEnd      = ctx_remap_def( lineEnd, ctx_le2PS_ENDCAP, true, PS_ENDCAP_ROUND);
		if (( ep-> actual  = stylus_extpenned( s)))
			ep-> style = stylus_get_extpen_style( s);
		stylus_change( self);
	}
	return true;
}

Bool
apc_gp_set_line_join( Handle self, int lineJoin)
{
	objCheck false;
	if ( !sys ps) {
		sys lineJoin = lineJoin;
	} else {
		PStylus s         = &sys stylus;
		PEXTPEN ep        = &s-> extPen;
		ep-> lineJoin     = ctx_remap_def( lineJoin, ctx_lj2PS_JOIN, true, PS_JOIN_ROUND);
		if (( ep-> actual = stylus_extpenned( s)))
			ep-> style = stylus_get_extpen_style( s);
		stylus_change( self);
	}
	return true;
}

Bool
apc_gp_set_line_width( Handle self, float lineWidth)
{
	objCheck false;
	if ( !sys ps) {
		sys lineWidth = lineWidth;
	} else {
		PStylus s = &sys stylus;
		PEXTPEN ep        = &s-> extPen;
		if ( lineWidth < 0.0 || lineWidth > 8192.0) lineWidth = 0.0;
		s-> pen. lopnWidth. x = lineWidth + .5;
		ep-> lineWidth = lineWidth;
		if (( ep-> actual = stylus_extpenned( s)))
			ep-> style = stylus_get_extpen_style( s);
		stylus_change( self);
	}
	return true;
}

Bool
apc_gp_set_line_pattern( Handle self, unsigned char * pattern, int len)
{
	objCheck false;
	if ( !sys ps) {
		if ( sys linePatternLen > sizeof(sys linePattern))
			free( sys linePattern);
		if ( len > sizeof(sys linePattern)) {
			sys linePattern = ( unsigned char *) malloc( len);
			if ( !sys linePattern) {
				sys linePatternLen = 0;
				return false;
			}
			memcpy( sys linePattern, pattern, len);
		} else
			memcpy( &sys linePattern, pattern, len);
		sys linePatternLen = len;
	} else {
		PStylus s           = &sys stylus;
		PEXTPEN ep          = &s-> extPen;

		s-> pen. lopnStyle  = patres_user( pattern, len);
		if (( ep-> actual    = stylus_extpenned( s))) {
			ep-> style       = stylus_get_extpen_style( s);
			ep-> patResource = ( s-> pen. lopnStyle == PS_USERSTYLE) ?
				patres_fetch( pattern, len) : &hPatHollow;
		} else
			ep-> patResource = &hPatHollow;
		stylus_change( self);
	}
	return true;
}

Bool
apc_gp_set_miter_limit( Handle self, float miter_limit)
{
	objCheck false;
	if ( !sys ps) {
		sys miterLimit = miter_limit;
		return true;
	} else 
		return SetMiterLimit( sys ps, (FLOAT) miter_limit, NULL);
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
			SelectPalette( sys ps, sys stockPalette, 1);
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
	sys currentROP = ctx_remap_def( rop, ctx_rop2R2, true, R2_COPYPEN);
	if ( !SetROP2( sys ps, sys currentROP)) apiErr;
	return true;
}

Bool
apc_gp_set_rop2( Handle self, int rop)
{
	objCheck false;
	if ( !sys ps) { sys rop2 = rop; return true; }
	if ( rop != ropCopyPut) rop = ropNoOper;
	sys currentROP2 = rop;
	if ( !SetBkMode( sys ps, ( rop == ropCopyPut) ? OPAQUE : TRANSPARENT)) apiErr;
	return true;
}

Bool
apc_gp_set_transform( Handle self, int x, int y)
{
	objCheck false;
	if ( !sys ps) {
		sys transform. x = x;
		sys transform. y = y;
		return true;
	}
	if ( !SetViewportOrgEx( sys ps, x - sys transform2. x, - ( y + sys transform2. y), NULL)) apiErr;
	return true;
}

#ifdef __cplusplus
}
#endif
