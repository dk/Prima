#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Window.h"
#include "Icon.h"
#include "Application.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle


Bool
cursor_update( Handle self)
{
	if ( !is_apt( aptFocused))
		return true;
	DestroyCaret();
	if ( is_apt( aptCursorVis)) {
		if ( !CreateCaret(( HWND) var handle, nil, sys cursorSize. x, sys cursorSize. y)) apiErrRet;
		if ( !SetCaretPos( sys cursorPos. x, sys lastSize. y - sys cursorPos. y - sys cursorSize. y)) apiErrRet;
		if ( !ShowCaret(( HWND) var handle)) apiErrRet;
	}
	return true;
}

Bool
apc_cursor_set_pos( Handle self, int x, int y)
{
	objCheck false;
	if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);
	sys cursorPos. x = x;
	sys cursorPos. y = y;
	return cursor_update( self);
}

Bool
apc_cursor_set_size( Handle self, int x, int y)
{
	objCheck false;
	if ( !hwnd_check_limits( x, y, false)) apcErrRet( errInvParams);
	sys cursorSize. x = x;
	sys cursorSize. y = y;
	return cursor_update( self);
}

Bool
apc_cursor_set_visible( Handle self, Bool visible)
{
	objCheck false;
	apt_assign( aptCursorVis, visible);
	return cursor_update( self);
}

Point
apc_cursor_get_pos( Handle self)
{
	Point p = {0,0};
	objCheck p;
	return sys cursorPos;
}

Point
apc_cursor_get_size( Handle self)
{
	Point p = {0,0};
	objCheck p;
	return sys cursorSize;
}

Bool
apc_cursor_get_visible( Handle self)
{
	objCheck false;
	return is_apt( aptCursorVis);
}

Point
apc_pointer_get_hot_spot( Handle self)
{
	Point         r = {0,0};
	ICONINFO      ii;
	objCheck r;
	if ( !GetIconInfo( sys pointer, &ii))
		apiErr
	else {
		r. x = ii. xHotspot;
		r. y = guts. pointerSize. y - ii. yHotspot - 1;
		DeleteObject( ii. hbmMask);
		DeleteObject( ii. hbmColor);
	}
	return r;
}

Point
apc_pointer_get_pos( Handle self)
{
	Point p = {0,0};
	RECT r;
	objCheck p;
	if ( !GetCursorPos(( POINT*) &p)) apiErr;
	GetWindowRect( HWND_DESKTOP, &r);
	p. y = r. bottom - p. y - 1;
	return p;
}

int
apc_pointer_get_shape( Handle self)
{
	objCheck 0;
	return sys pointerId;
}

Point
apc_pointer_get_size( Handle self)
{
	return guts. pointerSize;
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
	ICONINFO  ii;
	PIcon     i = ( PIcon) icon;
	HDC dc;
	XBITMAPINFO bi = {{
		sizeof( BITMAPINFOHEADER), 0, 0, 1, 1
	}};

	bi. bmiHeader. biWidth = guts. pointerSize. x;
	bi. bmiHeader. biHeight = guts. pointerSize. y;

	if ( icon == nilHandle)
		apcErrRet( errInvParams);
	objCheck false;
	dobjCheck( icon) false;
	if ( !GetIconInfo( sys pointer, &ii))
		apiErrRet;
	i-> self-> create_empty( icon, guts. pointerSize. x, guts. pointerSize. y, 1);
	if (!( dc = dc_alloc())) return false;
	if ( ii. hbmColor) {
		HDC ops = dsys( icon) ps;
		HBITMAP obm = dsys( icon) bm;
		dsys( icon) ps = dc;
		dsys( icon) bm = ii. hbmColor;
		image_query_bits( icon, false);
		if ( !GetDIBits( dc, ii. hbmMask, 0, i-> h, i-> mask, ( BITMAPINFO*) &bi, DIB_RGB_COLORS)) apiErr;
		dsys( icon) ps = ops;
		dsys( icon) bm = obm;
	} else {
		bi. bmiHeader. biHeight *= 2;
		if ( !GetDIBits( dc, ii. hbmMask, 0, i-> h, i-> data, ( BITMAPINFO*) &bi, DIB_RGB_COLORS)) apiErr;
		if ( !GetDIBits( dc, ii. hbmMask, i-> h, i-> h, i-> mask, ( BITMAPINFO*) &bi, DIB_RGB_COLORS)) apiErr;
	}
	dc_free();
	DeleteObject( ii. hbmMask);
	DeleteObject( ii. hbmColor);
	return true;
}

Bool
apc_pointer_get_visible( Handle self)
{
	objCheck false;
	return !guts. pointerInvisible;
}

Bool
apc_pointer_set_pos( Handle self, int x, int y)
{
	RECT r;
	if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);
	GetWindowRect( HWND_DESKTOP, &r);
	if ( !SetCursorPos( x, r. bottom - y - 1)) apiErrRet;
	return true;
}

Handle ctx_cr2IDC[] =
{
	crArrow,     ( Handle) IDC_ARROW,
	crText,      ( Handle) IDC_IBEAM,
	crWait,      ( Handle) IDC_WAIT,
	crSize,      ( Handle) IDC_SIZEALL,
	crMove,      ( Handle) IDC_SIZEALL,
	crSizeWest,  ( Handle) IDC_SIZEWE,
	crSizeEast,  ( Handle) IDC_SIZEWE,
	crSizeWE,    ( Handle) IDC_SIZEWE,
	crSizeNorth, ( Handle) IDC_SIZENS,
	crSizeSouth, ( Handle) IDC_SIZENS,
	crSizeNS,    ( Handle) IDC_SIZENS,
	crSizeNW,    ( Handle) IDC_SIZENWSE,
	crSizeSE,    ( Handle) IDC_SIZENWSE,
	crSizeNE,    ( Handle) IDC_SIZENESW,
	crSizeSW,    ( Handle) IDC_SIZENESW,
	crInvalid,   ( Handle) IDC_NO,
        crDragNone,  ( Handle) IDC_NO,
        crDragCopy,  ( Handle) IDC_HAND, 
        crDragMove,  ( Handle) IDC_HAND, 
        crDragLink,  ( Handle) IDC_HAND, 
        crDragAsk ,  ( Handle) IDC_HAND, 
	endCtx
};

static Bool
direct_pointer_change( Handle self)
{
	Point p;
	if ( var stage != csNormal || !IsWindowVisible( HANDLE)) return false;
	p = apc_pointer_get_pos( application);
	return self == apc_application_get_widget_from_point( application, p);
}

Bool
apc_pointer_set_shape( Handle self, int sysPtrId)
{
	HCURSOR user;

	objCheck false;
	user = sys pointer2;
	if ( sysPtrId < crDefault || sysPtrId > crUser) return false;
	sys pointerId = sysPtrId;
	if ( sysPtrId == crDefault)
	{
		Handle owner = var owner;
		while( owner)
		{
			sysPtrId = dsys( owner) pointerId;
			if ( sysPtrId != crDefault) break;
			owner = (( PComponent) owner)-> owner;
		}
		if ( sysPtrId == crDefault) sysPtrId = crArrow;
		if ( sysPtrId == crUser) user = dsys( owner) pointer2;
	}
	sys pointer = ( sysPtrId == crUser) ? user :
		LoadCursor( NULL, MAKEINTRESOURCE(
		ctx_remap_def( sysPtrId, ctx_cr2IDC, true, ( Handle)IDC_ARROW)));

	if ( direct_pointer_change( self))
		SetCursor( sys pointer);
	return true;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
	Bool direct;
	HCURSOR cursor;
	objCheck false;

	apcErrClear;
	direct = direct_pointer_change( self);
	hotSpot. y = guts. pointerSize. y - hotSpot. y - 1;
	if (icon) {
		Point sz = { PImage(icon)->w, PImage(icon)-> h };
		cursor = image_make_icon_handle( icon, sz, &hotSpot);
		if ( apcError) return false;
	} else
		cursor = nil;

	if ( sys pointer2) {
		if ( direct) SetCursor( NULL);
		if ( !DestroyCursor( sys pointer2)) apiErr;
	}
	sys pointer2 = cursor;

	if ( sys pointerId == crUser)
	{
		sys pointer = sys pointer2;
		if ( direct) SetCursor( sys pointer);
	}
	return true;
}

Bool
apc_pointer_set_visible( Handle self, Bool visible)
{
	if ( var stage == csNormal) {
		guts. pointerInvisible = !visible;
		ShowCursor( visible);
		guts. pointerLock += visible ? 1 : -1;
	}
	return true;
}

int
apc_pointer_get_state( Handle self)
{
	return
		(( GetKeyState( VK_LBUTTON)  < 0) ? mbLeft     : 0) |
		(( GetKeyState( VK_RBUTTON)  < 0) ? mbRight    : 0) |
		(( GetKeyState( VK_MBUTTON)  < 0) ? mbMiddle   : 0) |
		(( GetKeyState( VK_XBUTTON1) < 0) ? mb4        : 0) |
		(( GetKeyState( VK_XBUTTON2) < 0) ? mb5        : 0);
}

#ifdef __cplusplus
}
#endif
