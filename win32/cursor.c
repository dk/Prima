#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include "Window.h"
#include "Icon.h"
#include "Application.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle


void
cursor_update( Handle self)
{
   if ( !is_apt( aptFocused))
      return;
   DestroyCaret();
   if ( is_apt( aptCursorVis)) {
      if ( !CreateCaret(( HWND) var handle, nil, sys cursorSize. x, sys cursorSize. y)) apiErr;
      if ( !SetCaretPos( sys cursorPos. x, sys lastSize. y - sys cursorPos. y - sys cursorSize. y)) apiErr;
      if ( !ShowCaret(( HWND) var handle)) apiErr;
   }
}

void
apc_cursor_set_pos( Handle self, int x, int y)
{
   sys cursorPos. x = x;
   sys cursorPos. y = y;
   cursor_update( self);
}

void
apc_cursor_set_size( Handle self, int x, int y)
{
   sys cursorSize. x = x;
   sys cursorSize. y = y;
   cursor_update( self);
}

void
apc_cursor_set_visible( Handle self, Bool visible)
{
   apt_assign( aptCursorVis, visible);
   cursor_update( self);
}

Point
apc_cursor_get_pos( Handle self)
{
   return sys cursorPos;
}

Point
apc_cursor_get_size( Handle self)
{
   return sys cursorSize;
}

Bool
apc_cursor_get_visible( Handle self)
{
   return is_apt( aptCursorVis);
}

Point
apc_pointer_get_hot_spot( Handle self)
{
   Point         r = {0,0};
   ICONINFO      ii;
   if ( !GetIconInfo( sys pointer, &ii))
      apiErr
   else {
      r. x = ii. xHotspot;
      r. y = ii. yHotspot;
   }
   return r;
}

Point
apc_pointer_get_pos( Handle self)
{
   Point p;
   RECT r;
   if ( !GetCursorPos(( POINT*) &p)) apiErr;
   GetWindowRect( HWND_DESKTOP, &r);
   p. y = r. bottom - p. y - 1;
   return p;
}

int
apc_pointer_get_shape( Handle self)
{
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
   int j;
   HDC dc;
   XBITMAPINFO bi = {{
      sizeof( BITMAPINFOHEADER), guts. pointerSize. x, guts. pointerSize. y, 1, 1
   }};

   if ( icon == nilHandle)
      apcErrRet( errInvParams);
   if ( !GetIconInfo( sys pointer, &ii))
      apiErrRet;
   i-> self-> create_empty( icon, guts. pointerSize. x, guts. pointerSize. y, 1);
   dc = dc_alloc();
   if ( ii. hbmColor) {
      HDC ops = dsys( icon) ps;
      HDC obm = dsys( icon) bm;
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
   for ( j = 0; j < i-> maskSize; j++)  i-> mask[ j] = ~i-> mask[ j];
}

Bool
apc_pointer_get_visible( Handle self)
{
   return is_apt( aptPointerVis);
}

void
apc_pointer_set_pos( Handle self, int x, int y)
{
   RECT r;
   GetWindowRect( HWND_DESKTOP, &r);
   if ( !SetCursorPos( x, r. bottom - y - 1)) apiErr;
}

int ctx_cr2IDC[] =
{
   crArrow,    ( int) IDC_ARROW,
   crText,     ( int) IDC_IBEAM,
   crWait,     ( int) IDC_WAIT,
   crSize,     ( int) IDC_SIZEALL,
   crMove,     ( int) IDC_SIZEALL,
   crSizeWE,   ( int) IDC_SIZEWE,
   crSizeNS,   ( int) IDC_SIZENS,
   crSizeNWSE, ( int) IDC_SIZENWSE,
   crSizeNESW, ( int) IDC_SIZENESW,
   crInvalid,  ( int) IDC_NO,
   endCtx
};


void
apc_pointer_set_shape( Handle self, int sysPtrId)
{
   HCURSOR user = sys pointer2;
   if ( sysPtrId < crDefault || sysPtrId > crUser) return;
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
      ctx_remap_def( sysPtrId, ctx_cr2IDC, true, ( int)IDC_ARROW)));

   if ( var stage == csNormal)
      if ( !SetCursor( sys pointer)) apiErr;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
   if ( sys pointer2)
      if ( !DestroyCursor( sys pointer2)) apiErr;
   apcErrClear;
   sys pointer2 = icon ? image_make_icon_handle( icon, guts. pointerSize, &hotSpot) : nilHandle;
   if ( apcError) return false;
   if ( sys pointerId == crUser)
   {
      sys pointer = sys pointer2;
      if ( var stage == csNormal)
        if ( !SetCursor( sys pointer)) apiErrRet;
   }
   return true;
}

void
apc_pointer_set_visible( Handle self, Bool visible)
{
   if ( var stage == csNormal) {
      apt_assign( aptPointerVis, visible);
      ShowCursor( visible);
      guts. pointerLock += visible ? 1 : -1;
   }
}

int
apc_pointer_get_state( Handle self)
{
   return
      (( GetKeyState( VK_LBUTTON) < 0) ? mbLeft     : 0) |
      (( GetKeyState( VK_RBUTTON) < 0) ? mbRight    : 0) |
      (( GetKeyState( VK_MBUTTON) < 0) ? mbMiddle   : 0);
}

