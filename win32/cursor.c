/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "Window.h"
#include "Icon.h"
#include "Application.h"

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
   int j;
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
   DeleteObject( ii. hbmMask);
   DeleteObject( ii. hbmColor);
   return true;
}

Bool
apc_pointer_get_visible( Handle self)
{
   objCheck false;
   return is_apt( aptPointerVis);
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

int ctx_cr2IDC[] =
{
   crArrow,     ( int) IDC_ARROW,
   crText,      ( int) IDC_IBEAM,
   crWait,      ( int) IDC_WAIT,
   crSize,      ( int) IDC_SIZEALL,
   crMove,      ( int) IDC_SIZEALL,
   crSizeWest,  ( int) IDC_SIZEWE,
   crSizeEast,  ( int) IDC_SIZEWE,
   crSizeWE,    ( int) IDC_SIZEWE,
   crSizeNorth, ( int) IDC_SIZENS,
   crSizeSouth, ( int) IDC_SIZENS,
   crSizeNS,    ( int) IDC_SIZENS,
   crSizeNW,    ( int) IDC_SIZENWSE,
   crSizeSE,    ( int) IDC_SIZENWSE,
   crSizeNE,    ( int) IDC_SIZENESW,
   crSizeSW,    ( int) IDC_SIZENESW,
   crInvalid,   ( int) IDC_NO,
   endCtx
};

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
      ctx_remap_def( sysPtrId, ctx_cr2IDC, true, ( int)IDC_ARROW)));

   if ( var stage == csNormal) SetCursor( sys pointer);
   return true;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
   objCheck false;
   if ( sys pointer2) {
      SetCursor( NULL);
      if ( !DestroyCursor( sys pointer2)) apiErr;
   }
   apcErrClear;
   hotSpot. y = guts. pointerSize. y - hotSpot. y - 1;
   sys pointer2 = icon ? image_make_icon_handle( icon, guts. pointerSize, &hotSpot, true) : nilHandle;
   if ( apcError) return false;
   if ( sys pointerId == crUser)
   {
      sys pointer = sys pointer2;
      if ( var stage == csNormal)
         SetCursor( sys pointer);
   }
   return true;
}

Bool
apc_pointer_set_visible( Handle self, Bool visible)
{
   if ( var stage == csNormal) {
      apt_assign( aptPointerVis, visible);
      ShowCursor( visible);
      guts. pointerLock += visible ? 1 : -1;
   }
   return true;
}

int
apc_pointer_get_state( Handle self)
{
   return
      (( GetKeyState( VK_LBUTTON) < 0) ? mbLeft     : 0) |
      (( GetKeyState( VK_RBUTTON) < 0) ? mbRight    : 0) |
      (( GetKeyState( VK_MBUTTON) < 0) ? mbMiddle   : 0);
}

