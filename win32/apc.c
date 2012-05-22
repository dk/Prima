/*
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
 *
 * $Id$
 */
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#include "win32\win32guts.h"
#include <commdlg.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "File.h"
#include "Menu.h"
#include "Image.h"
#include "Window.h"
#include "Application.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

#define WinShowWindow(WND) SetWindowPos( WND, nil, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
#define WinHideWindow(WND) SetWindowPos( WND, nil, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW);
#define apc_widget_redraw(self)  apc_widget_invalidate_rect(self,nil)

Bool
apc_application_begin_paint ( Handle self)
{
   objCheck false;
   apcErrClear;
   if ( !( sys ps = dc_alloc())) apiErrRet;
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   hwnd_enter_paint( self);
   if (( sys pal = palette_create( self))) {
      SelectPalette( sys ps, sys pal, 0);
      RealizePalette( sys ps);
   }
   return true;
}


Bool
apc_application_begin_paint_info( Handle self)
{
   Bool ok = apc_application_begin_paint( self);
   objCheck false;
   if ( ok) {
      HRGN rgn = CreateRectRgn( 0, 0, 0, 0);
      SelectClipRgn( sys ps, rgn);
      DeleteObject( rgn);
   }
   return ok;
}


Bool
apc_application_create( Handle self)
{
   HWND h;
   RECT r;
   MSG msg;
   const WCHAR wnull = 0;

   objCheck false;

   // make sure that no leftover messages, esp.WM_QUIT, are floating around
   while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE)); 

   if ( !( h = CreateWindowExW( 0, const_char2wchar("GenericApp"), &wnull, 0, 0, 0, 0, 0,
          nil, nil, guts. instance, nil))) apiErrRet;
   sys handle = h;
   sys parent = sys owner = HWND_DESKTOP;
   SetWindowLongPtr( sys handle, GWLP_USERDATA, self);
   PostMessage( sys handle, WM_PRIMA_CREATE, 0, 0);
   sys className = WC_APPLICATION;
   // if ( !SetTimer( h, TID_USERMAX, 100, nil)) apiErr;
   GetClientRect( h, &r);
   if ( !( var handle = ( Handle) CreateWindowExW( 0,  const_char2wchar("Generic"), &wnull, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        0, 0, r. right - r. left, r. bottom - r. top, h, nil,
        guts. instance, nil))) apiErrRet;
   SetWindowLongPtr(( HWND) var handle, GWLP_USERDATA, self);
   apt_set( aptEnabled);
   sys lastSize = apc_application_get_size( self);
   return true;
}

Bool
apc_application_close( Handle self)
{
   PostQuitMessage(0);
   return true;
}

Bool
apc_application_destroy( Handle self)
{
   objCheck false;
   SetWindowLongPtr( sys handle, GWLP_USERDATA, 0);
   if ( IsWindow( sys handle))  {
      if ( guts. mouseTimer) {
          guts. mouseTimer = 0;
          if ( !KillTimer( sys handle, TID_USERMAX)) apiErr;
      }
      if ( !DestroyWindow( sys handle)) apiErr;
   }
   free( sys timeDefs);
   PostThreadMessage( guts. mainThreadId, WM_TERMINATE, 0, 0);
   PostQuitMessage(0);
   return true;
}

Bool
apc_application_end_paint( Handle self)
{
   apcErrClear;
   objCheck false;
   hwnd_leave_paint( self);
   if ( sys pal) DeleteObject( sys pal);
   dc_free();
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
   sys pal = nil;
   sys ps = nil;
   return true;
}

Bool
apc_application_end_paint_info( Handle self)
{
   return apc_application_end_paint( self);
}


int
apc_application_get_gui_info( char * description, int len)
{
   if ( description) {
      strncpy( description, "Windows", len);
      description[len-1] = 0;
   }
   return guiWindows;
}

Bool
apc_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
   HBITMAP bm, bm2;
   HDC dc, dc2;
   XLOGPALETTE lpg;
   HPALETTE hp, hp2, hp3;
   if ( image == nilHandle) apcErrRet( errInvParams);
   dobjCheck( image) false;


   apcErrClear;
   if (!( dc  = dc_alloc())) return false;
   lpg. palNumEntries = GetSystemPaletteEntries( dc, 0, 256, lpg. palPalEntry);
   lpg. palVersion = 0x300;

   hp  = CreatePalette(( LOGPALETTE*)&lpg);
   dc2 = CreateCompatibleDC( dc);
   if ( !dc2) {
      DeleteObject( hp);
      dc_free();
      return false;
   }
   hp2 = SelectPalette( dc2, hp, 0);
   RealizePalette( dc2);
   hp3 = SelectPalette( dc, hp, 1);

   bm  = CreateCompatibleBitmap( dc, xLen, yLen);
   if ( !bm) {
      SelectPalette( dc, hp3, 1);
      SelectPalette( dc2, hp2, 1);
      DeleteObject( hp);
      dc_free();
      return false;
   }
   bm2 = SelectObject( dc2, bm);
   BitBlt( dc2, 0, 0, xLen, yLen, dc, x, sys lastSize.y - y - yLen, SRCCOPY);
   SelectObject( dc2, bm2);
   SelectPalette( dc2, hp2, 1);
   DeleteObject( hp);
   DeleteDC( dc2);

   bm2 = dsys(image)bm;
   dsys(image)bm = bm;
   image_query_bits( image, true);
   dsys(image)bm = bm2;
   DeleteObject( bm);
   SelectPalette( dc, hp3, 1);
   dc_free();

   apc_image_update_change( image);

   return true;
}


Handle
hwnd_to_view( HWND win)
{
   Handle h;
   LONG_PTR ll;
   if (( !win) || ( !IsWindow( win)))
      return nilHandle;
   if ( GetWindowThreadProcessId( win, nil) != guts. mainThreadId)
      return nilHandle;
   h = GetWindowLongPtr( win, GWLP_USERDATA);
   if ( !h) return nilHandle;
   ll = GetWindowLongPtr( win, GWLP_WNDPROC);
   if (
       ( ll == ( LONG_PTR) generic_view_handler) ||
       ( ll == ( LONG_PTR) generic_app_handler) ||
       ( ll == ( LONG_PTR) generic_frame_handler)
      ) return h;

   if ( SendMessage( win, WM_HASMATE, 0, ( LPARAM) &h) == HASMATE_MAGIC)
      return h;
   return nilHandle;
}


int
apc_application_get_os_info( char *system, int slen,
                             char *release, int rlen,
                             char *vendor, int vlen,
                             char *arch, int alen)
{
   SYSTEM_INFO si;
   OSVERSIONINFO os = { sizeof( OSVERSIONINFO)};
   DWORD  version;
   GetSystemInfo( &si);
   version = GetVersion();
   GetVersionEx( &os);
   if ( system) {
      if ( IS_NT) {
         strncpy( system, "Windows NT", slen);
      } else if ( IS_WIN95) {
         if ((os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
             ((os.dwMajorVersion > 4) ||
              ((os.dwMajorVersion == 4) && (os.dwMinorVersion > 0)))) {
            strncpy( system, "Windows 98", slen);
         } else {
            strncpy( system, "Windows 95", slen);
         }
      } else {
         strncpy( system, "Windows", slen);
      }
      system[ slen-1] = 0;
   }
   if ( vendor) {
      strncpy( vendor, "Microsoft", vlen);
      vendor[ vlen-1] = 0;
   }
   if ( arch) {
      char * pb = "Unknown";
#if defined( __BORLANDC__) && ! ( defined( __cplusplus) || defined( _ANONYMOUS_STRUCT))
      switch ( si. u. s. wProcessorArchitecture) {
#else
      switch ( si. wProcessorArchitecture) {
#endif
      case PROCESSOR_ARCHITECTURE_INTEL :   pb = "i386";  break;
      case PROCESSOR_ARCHITECTURE_MIPS  :   pb = "MIPS";  break;
      case PROCESSOR_ARCHITECTURE_ALPHA :   pb = "Alpha"; break;
      case PROCESSOR_ARCHITECTURE_PPC   :   pb = "PPC";   break;
      }
      strncpy( arch, pb, alen);
      arch[ alen-1] = 0;
   }
   if ( release)
      snprintf( release, rlen, "%d.%d",
                LOBYTE( LOWORD( version)),
                HIBYTE( LOWORD( version)));
   return apcWin32;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
   return hwnd_to_view(( HWND) apiHandle);
}

Rect
apc_application_get_indents( Handle self)
{
	Point size;
	UINT rc;
	Rect ret = {0,0,0,0};
	APPBARDATA d;
	
	size = apc_application_get_size( self);

	memset( &d, 0, sizeof(d));
	d. cbSize = sizeof(d);
	rc = SHAppBarMessage( ABM_GETSTATE, &d);
	if (( rc & ABS_AUTOHIDE) == 0) {
		memset( &d, 0, sizeof(d));
		d. cbSize = sizeof(d);
		rc = SHAppBarMessage( ABM_GETTASKBARPOS, &d);
		switch ( d. uEdge) {
		case ABE_TOP:
			ret. top = d. rc. bottom;
			if ( ret. top < 0) ret. top = 0;
			break;
		case ABE_BOTTOM:
			ret. bottom = size. y - d. rc. top;
			if ( ret. bottom < 0) ret. bottom = 0;
			break;
		case ABE_RIGHT:
			ret. right = size. x - d. rc. left;
			if ( ret. right < 0) ret. right = 0;
			break;
		case ABE_LEFT:
			ret. left = d. rc. right;
			if ( ret. left < 0) ret. left = 0;
			break;
		}
	}
	
	return ret;
}

Point
apc_application_get_size( Handle self)
{
   RECT  r;
   Point ret = {0,0};
   objCheck ret;
   GetWindowRect( HWND_DESKTOP, &r);
   ret. x = r. right;
   ret. y = r. bottom;
   return ret;
}

static Bool
files_rehash( Handle self, void * dummy)
{
   CFile( self)-> is_active( self, true);
   return false;
}

static Bool
process_msg( MSG * msg)
{
   Bool postpone_msg_translation = false;
   switch ( msg-> message)
   {
   case WM_TERMINATE:
   case WM_QUIT:
      return false;
   case WM_CROAK:
      if ( msg-> wParam)
         croak(( char *) msg-> lParam);
      else
         warn(( char *) msg-> lParam);
      return true;
   case WM_SYSKEYDOWN:
      /*
	 If Prima handles an Alt-Key combination that is also handled by a menu
	 in TranslateMessage(), we need to prevent the message from being
	 processed by the menu, by setting guts.dont_xlate_message flag.
       */
      postpone_msg_translation = true;
   case WM_SYSKEYUP:
   case WM_KEYDOWN:
   case WM_KEYUP:
      GetKeyboardState( guts. keyState);
      break;
   case WM_KEYPACKET:
      {
         KeyPacket * kp = ( KeyPacket *) msg-> lParam;
         BYTE * mod = mod_select( kp-> mod);
         SendMessage( kp-> wnd, kp-> msg, kp-> mp1, kp-> mp2);
         mod_free( mod);
      }
      break;
   case WM_LBUTTONDOWN:  musClk. emsg = WM_LBUTTONUP; goto MUS1;
   case WM_MBUTTONDOWN:  musClk. emsg = WM_MBUTTONUP; goto MUS1;
   case WM_RBUTTONDOWN:  musClk. emsg = WM_RBUTTONUP; goto MUS1;
   MUS1:
      musClk. pending = 1;
      musClk. msg     = *msg;
      musClk. msg. wParam &=  MK_CONTROL|MK_SHIFT;
      break;
   case WM_LBUTTONUP:   musClk. msg. message = WM_LMOUSECLICK; goto MUS2;
   case WM_MBUTTONUP:   musClk. msg. message = WM_MMOUSECLICK; goto MUS2;
   case WM_RBUTTONUP:   musClk. msg. message = WM_RMOUSECLICK; goto MUS2;
   MUS2:
      if ( musClk. pending &&
           ( musClk. emsg         == msg-> message) &&
           ( musClk. msg. hwnd    == msg-> hwnd)    &&
           ( musClk. msg. wParam  == ( msg-> wParam & ( MK_CONTROL|MK_SHIFT))) &&
           ( abs( musClk. msg. time  - msg-> time) < 200)
         )
         PostMessage( msg-> hwnd, musClk. msg. message, msg-> wParam, msg-> lParam);
      musClk. pending = 0;
      break;
   case WM_LBUTTONDBLCLK:
   case WM_MBUTTONDBLCLK:
   case WM_RBUTTONDBLCLK:
      musClk. pending = 0;
      break;
   case WM_SOCKET:
      {
         int i;
         SOCKETHANDLE socket = ( SOCKETHANDLE) msg-> lParam;
         for ( i = 0; i < guts. sockets. count; i++) {
            Handle self = guts. sockets. items[ i];
            if (( sys s. file. object == socket) &&
                ( PFile( self)-> eventMask & msg-> wParam)) {
               Event ev;
               ev. cmd = ( msg-> wParam == feRead) ? cmFileRead :
                         (( msg-> wParam == feWrite) ? cmFileWrite : cmFileException);
               CComponent( self)-> message( self, &ev);
               break;
            }
         }
         guts. socketPostSync = 0; // clear semaphore
      }
      return true;
   case WM_SOCKET_REHASH:
      socket_rehash();
      guts. socketPostSync = 0; // clear semaphore
      return true;
   case WM_FILE:
      if ( msg-> wParam == 0) {
         int i;

         if ( guts. files. count == 0) return true;

         list_first_that( &guts. files, files_rehash, nil);
         for ( i = 0; i < guts. files. count; i++) {
            Handle self = guts. files. items[i];
            if ( PFile( self)-> eventMask & feRead)
               PostMessage( NULL, WM_FILE, feRead, ( LPARAM) self);
            if ( PFile( self)-> eventMask & feWrite)
               PostMessage( NULL, WM_FILE, feWrite, ( LPARAM) self);
         }
         PostMessage( NULL, WM_FILE, 0, 0);
      } else {
         int i;
         Handle self = nilHandle;
         for ( i = 0; i < guts. files. count; i++)
            if (( guts. files. items[i] == ( Handle) msg-> lParam) &&
                ( PFile(guts. files. items[i])-> eventMask & msg-> wParam)) {
               self = ( Handle) msg-> lParam;
               break;
            }
         if ( self) {
            Event ev;
            ev. cmd = ( msg-> wParam == feRead) ? cmFileRead : cmFileWrite;
            CComponent( self)-> message( self, &ev);
         }
      }
      return true;
   }
   if ( !postpone_msg_translation) 
      TranslateMessage( msg);
   DispatchMessage( msg);
   if ( postpone_msg_translation) { 
      if ( guts. dont_xlate_message)
         guts. dont_xlate_message = false;
      else
         TranslateMessage( msg);
   }
   kill_zombies();
   return true;
}


Bool
apc_application_go( Handle self)
{
   MSG msg;
   objCheck false;

   while ( GetMessage( &msg, NULL, 0, 0) && process_msg( &msg));

   if ( application) Object_destroy( application);
   return true;
}


static Bool
HWND_lock( Bool lock)
{
   if ( lock)
   {
      if ( guts. appLock++ == 0) return LockWindowUpdate( HWND_DESKTOP);
   }
   else
   {
      if ( --guts. appLock == 0) return LockWindowUpdate( nil);
   }
   return true;
}

Bool
apc_application_lock( Handle self)
{
   return HWND_lock( true);
}

Bool
apc_application_unlock( Handle self)
{
   return HWND_lock( false);
}

Bool
apc_application_sync( void)
{
   return true;
}

Bool
apc_application_yield()
{
   MSG msg;
   while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
      if ( !process_msg( &msg)) {
         PostThreadMessage( guts. mainThreadId, appDead ? WM_QUIT : WM_TERMINATE, 0, 0);
	 break;
      }
   return true;
}

Handle
apc_application_get_widget_from_point( Handle self, Point point)
{
   DWORD pid, tid;
   POINT pt;
   HWND  p;

   objCheck nilHandle;
   pt.x = point. x;
   pt.y = sys lastSize. y - point. y - 1;
   p    =  WindowFromPoint( pt);

   if ( p) {
      POINT xp = pt;
      MapWindowPoints( HWND_DESKTOP, p, &xp, 1);
      p = ChildWindowFromPointEx( p, xp, CWP_SKIPINVISIBLE);
   } else
      p = ChildWindowFromPointEx( HWND_DESKTOP, pt, CWP_SKIPINVISIBLE);

   if ( !p) return nilHandle;
   if ( !( tid = GetWindowThreadProcessId( p, &pid))) apiErr;
   if ( tid != guts. mainThreadId) return nilHandle;
   return ( Handle) GetWindowLongPtr( p, GWLP_USERDATA);
}

// Component
Bool
apc_component_create( Handle self)
{
   PComponent c = ( PComponent) self;
   PDrawableData d = ( PDrawableData) c-> sysData;

   objCheck false;

   if ( d) return false;
   d = ( PDrawableData) malloc( sizeof( DrawableData));
   if ( !d) return false;
   memset( d, 0, sizeof( DrawableData));
   c-> sysData = d;
   return true;
}

Bool
apc_component_destroy( Handle self)
{
   PComponent    c = ( PComponent) self;
   PDrawableData d = ( PDrawableData) c-> sysData;
   objCheck false;
   var handle = nilHandle;
   if ( d == nil) return false;
   free( d);
   c-> sysData = nil;
   return true;
}

Bool
apc_component_fullname_changed_notify( Handle self)
{
   return true;
}


void
process_transparents( Handle self)
{
   int i;
   RECT mr;
   objCheck;
   GetWindowRect(( HWND) var handle, &mr);
   for ( i = 0; i < var widgets. count; i++) {
      HWND xwnd;
      Handle x = var widgets. items[ i];
      dobjCheck(x);
      xwnd = DHANDLE(x);
      if ( dsys(x) options. aptTransparent && IsWindowVisible( xwnd)) {
         RECT r, dr;
         GetWindowRect( xwnd, &r);
         IntersectRect( &dr, &r, &mr);
         if ( !IsRectEmpty( &dr))
            InvalidateRect( xwnd, nil, false);
      }
   }
}

typedef struct _ViewProfile {
  ColorSet colors;
  Point    pos;
  Point    size;
  Point    virtSize;
  Bool     enabled;
  Bool     visible;
  Bool     focused;
  Handle   capture;
} ViewProfile, *PViewProfile;


static void
get_view_ex( Handle self, PViewProfile p)
{
  int i;
  if ( !p) return;
  p-> capture   = apc_widget_is_captured( self);
  for ( i = 0; i <= ciMaxId; i++) p-> colors[ i] = apc_widget_get_color( self, i);
  if ( sys className == WC_FRAME) {
     p-> pos       = apc_window_get_client_pos( self);
     p-> size      = apc_window_get_client_size( self);
  } else {
     p-> pos       = apc_widget_get_pos( self);
     p-> size      = apc_widget_get_size( self);
  }
  p-> virtSize  = var virtualSize;
  p-> enabled   = apc_widget_is_enabled( self);
  p-> focused   = apc_widget_is_focused( self);
  p-> visible   = apc_widget_is_visible( self);
}


static void
set_view_ex( Handle self, PViewProfile p)
{
  int i;
  HWND wnd = ( HWND) var handle;
  apc_widget_set_visible( self, false);
  for ( i = 0; i <= ciMaxId; i++) apc_widget_set_color( self, p-> colors[i], i);
  apc_widget_set_font( self, &var font);
  if ( sys className == WC_FRAME) {
     apc_window_set_client_rect( self, p-> pos. x, p-> pos. y, p-> size.x, p-> size.y);
  } else {
     apc_widget_set_rect( self, p-> pos. x, p-> pos. y, p-> size.x, p-> size.y);
  }
  var virtualSize = p-> virtSize;
  apc_widget_set_enabled( self, p-> enabled);
  if ( p-> focused) apc_widget_set_focused( self);
  apc_widget_set_visible( self, p-> visible);
  if ( p-> capture) apc_widget_set_capture( self, 1, nilHandle);
  if ( sys timeDefs) for ( i = 0; i < sys timeDefsCount; i++)
     if ( sys timeDefs[ i]. item)
     {
        Handle xs   = ( Handle) sys timeDefs[ i]. item;
        Handle self = xs;
        if ( is_opt( optActive))
           if ( !SetTimer( wnd, var handle, sys s. timer. timeout, nil)) {
              opt_clear( optActive);
              apiErr;
           }
     }
   if ( !InvalidateRect(( HWND) var handle, nil, false)) apiErr;
   process_transparents( self);
}

static Bool repost_msgs( PostMsg * msg, Handle self)
{
   PostMessage(( HWND) var handle, WM_POSTAL, ( WPARAM) self, ( LPARAM) msg);
   return false;
}

static Bool
create_group( Handle self, Handle owner, Bool syncPaint, Bool clipOwner,
              Bool taskListed, int className, DWORD style, DWORD exstyle,
              Bool usePos, Bool useSize,
              ViewProfile * vprf, HWND parentHandle)
{
   HWND ret = nil;
   HWND old        = HANDLE;
   HWND behind     = HWND_TOP;
   HWND ownerView  = ( HWND) (( PWidget) owner)-> handle;
   HWND parentView = (( PDrawableData)(( PComponent) owner)-> sysData)-> parent;
   int  count = 0;
   Bool reset = false;
   Handle * list = nil;
   const WCHAR wnull = 0;

   if ( HANDLE)
   {
      int i;
      if (( DHANDLE( owner) == sys owner) && ( clipOwner == is_apt( aptClipOwner)))
      {
         behind = GetWindow( HANDLE, GW_HWNDPREV);
         if ( behind == nil) behind = HWND_TOP;
      }
      var stage = csAxEvents;
      if ( kind_of( self, CWidget))
      {
         PWidget g = ( PWidget) self;
         list  = g-> widgets. items;
         count = g-> widgets. count;
      }
      var self-> end_paint_info( self);
      var self-> end_paint( self);
      reset = true;
      for( i = 0; i < count; i++)
         get_view_ex( list[ i], ( ViewProfile*)( dsys( list[ i]) recreateData = malloc( sizeof( ViewProfile))));
   }

   switch (( int) className)
   {
     case WC_FRAME:
       {
          HWND frame;
          RECT r;
          int  rcp[4] = {0,0,0,0};
          if ( !clipOwner) parentView = HWND_DESKTOP;
          if ( !taskListed && ( parentView == HWND_DESKTOP || parentView == nil))
              parentView = DHANDLE( application);
          if ( !usePos)  rcp[0] = rcp[1] = CW_USEDEFAULT;
          if ( !useSize) rcp[2] = rcp[3] = CW_USEDEFAULT;
          if ( !( frame = CreateWindowExW( exstyle, const_char2wchar("GenericFrame"), &wnull,
                style | WS_CLIPCHILDREN,
                rcp[0], rcp[1], rcp[2], rcp[3],
                parentView, nil, guts. instance, nil)))
             apiErrRet;
          if ( !SetWindowPos( frame, behind, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
             apiErr;
          if ((( PApplication) application)-> topExclModal &&
              ((( PApplication) application)-> topExclModal != self)
             )
             if ( !SetWindowPos( frame, DHANDLE((( PApplication) application)-> topExclModal),
                   0,0,0,0,SWP_NOMOVE | SWP_NOSIZE  | SWP_NOACTIVATE))
                apiErr;
          GetClientRect( frame, &r);
          if ( !( ret = CreateWindowExW( 0,  const_char2wchar("Generic"), &wnull,
                WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                0, 0, r. right - r. left, r. bottom - r. top, frame, nil,
                guts. instance, nil)))
             apiErr;
          sys lastSize. x = r. right  - r. left;
          sys yOverride = sys lastSize. y = r. bottom - r. top;
          sys handle = frame;
          SetWindowLongPtr( frame, GWLP_USERDATA, ( LONG) self);
       }
       break;
    case WC_CUSTOM:
       if ( !parentHandle && ( !clipOwner || owner == application)) {
            style &= ~WS_CHILD;
            style |= WS_POPUP;
            exstyle |= WS_EX_TOOLWINDOW;
       }
       if ( parentHandle) parentView = parentHandle;
       sys parentHandle = parentHandle;

       if ( !( ret = CreateWindowExW( exstyle,  (LPCWSTR) const_char2wchar("Generic"), &wnull,
             style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0,
             parentView, nil, guts. instance, nil)))
          apiErrRet;
       if ( !SetWindowPos( ret, behind, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
          apiErr;
       sys handle = ret;
       break;
   }

   apt_assign( aptClipOwner, clipOwner);
   apt_assign( aptSyncPaint, syncPaint);
   apt_set( aptEnabled);
   sys className = className;
   sys parent  = ret;
   var handle  = ( Handle) ret;
   sys owner   = ownerView;
   if ( !reset)
      sys pointer = LoadCursor( guts. instance, IDC_ARROW);
   SetWindowLongPtr( ret, GWLP_USERDATA, ( LONG) self);

   if ( reset)
   {
      int i;
      Handle oldOwner = var owner; var owner = owner;
      if ( sys className == WC_FRAME) {
         apc_window_set_client_rect( self, vprf-> pos. x, vprf-> pos. y, vprf-> size. x, vprf-> size. y);
      } else {
         apc_widget_set_rect( self, vprf-> pos. x, vprf-> pos. y, vprf-> size. x, vprf-> size. y);
      }
      var owner = oldOwner;
      for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
      if ( sys className == WC_FRAME)
      {
         SetMenu( HANDLE, GetMenu( old));
         SetMenu( old, NULL);
      }
      if ( !DestroyWindow( old))
         apiErr;
      if ( var postList) list_first_that( var postList, repost_msgs, ( void*)self);
   }
   PostMessage( ret, WM_PRIMA_CREATE, 0, 0);

   
   if ( !reset) {
      /* set manually cmMove and cmSize when windows are configured automatically */
      if ( !usePos) {
         Event e;
         Point sz = apc_window_get_client_pos( self);
         memset( &e, 0, sizeof(e));
         e. gen. source = self;
         e. cmd = cmMove;
         e. gen. P. x = sz. x;
         e. gen. P. y = sz. y;
         CComponent(self)->message( self, &e);
         if ( PObject( self)-> stage == csDead) return false; 
      }
      if ( !useSize) {
         Event e;
         Point sz = apc_window_get_client_size( self);
         memset( &e, 0, sizeof(e));
         e. gen. source = self;
         e. cmd = cmSize;
         e. gen. P. x = e. gen. R. right = sz. x;
         e. gen. P. y = e. gen. R. top = sz. y;
         CComponent(self)->message( self, &e);
         if ( PObject( self)-> stage == csDead) return false; 
      }
   }
   return true;
}

// Window
Bool
apc_window_create( Handle self, Handle owner, Bool syncPaint, int borderIcons,
                   int borderStyle, Bool taskList, int windowState, 
		   int on_top, Bool usePos, Bool useSize)
{
  Bool reset = false;
  ViewProfile vprf;
  int oStage = var stage;
  WindowData ws;
  HICON icon = (HICON) nilHandle;
  WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
  DWORD style = WS_CLIPCHILDREN | WS_OVERLAPPED
     | (( borderIcons &  biSystemMenu) ? WS_SYSMENU     : 0)
     | (( borderIcons &  biMinimize)   ? WS_MINIMIZEBOX : 0)
     | (( borderIcons &  biMaximize)   ? WS_MAXIMIZEBOX : 0)
     | (( borderIcons &  biTitleBar)   ? 0              : WS_POPUP)
     | (( borderStyle == bsSizeable)   ? WS_THICKFRAME  : 0)
     | (( borderStyle == bsSingle  )   ? WS_BORDER      : 0)
     | (( borderStyle == bsDialog  )   ? WS_BORDER      : 0)
     | (( windowState == wsMinimized)  ? WS_MINIMIZE    : 0)
     | (( windowState == wsMaximized)  ? WS_MAXIMIZE    : 0)
  ;
  DWORD exstyle = 0
     | (( borderStyle == bsDialog  )   ? WS_EX_DLGMODALFRAME : 0)
//   | (  taskList                     ? 0                   : WS_EX_TOOLWINDOW)
  ;

  objCheck false;
  dobjCheck( owner) false;

  if ( !kind_of( self, CWidget)) apcErrRet( errInvObject);
  apcErrClear;
  if (( var handle != nilHandle) && (
       ( DHANDLE( owner) != sys owner)
    || ( borderStyle != sys s. window. borderStyle)
    || ( borderIcons != sys s. window. borderIcons)
    || ( taskList    != is_apt( aptTaskList))
  ))
  {
     apc_window_set_window_state( self, windowState);
     // prevent cmSize/cmWindowStage message loss if recreate goes with WS_XXX change.
     if ( sys recreateData) {
        memcpy( &vprf, sys recreateData, sizeof( vprf));
        free( sys recreateData);
        sys recreateData = nil;
     } else
        get_view_ex( self, &vprf);
     ws = sys s. window;
     if ( !GetWindowPlacement( HANDLE, &wp)) apiErr;
     usePos = useSize = 1; // prevent using shell-position flags for recreate
     icon = ( HICON) SendMessage( HANDLE, WM_GETICON, ICON_BIG, 0);
     reset = true;
  }
  HWND_lock( true);

  if ( reset || ( var handle == nilHandle))
     if ( !create_group( self, owner, syncPaint, false,
           taskList, WC_FRAME, style, exstyle, usePos, useSize, &vprf, NULL)) {
	if ( on_top >= 0) {
	   apt_assign( aptOnTop, on_top);
	   if ( on_top > 0)
	      SetWindowPos( sys handle, HWND_TOPMOST, 0, 0, 0, 0, 
	         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
        HWND_lock( false);
        return false;
     }
  ws. borderStyle = sys s. window. borderStyle = borderStyle;
  ws. borderIcons = sys s. window. borderIcons = borderIcons;
  ws. state       = sys s. window. state       = windowState;
  apt_assign( aptSyncPaint, syncPaint);
  apt_assign( aptTaskList,  taskList);
  if ( usePos) apt_set( aptWinPosDetermined);
  if ( reset)
  {
     Handle oldOwner = var owner; 
     
     var owner = owner;
     apc_window_set_window_state( self, windowState);
     var owner = oldOwner;
     set_view_ex( self, &vprf);
     sys s. window = ws;
     if ( windowState != wsMaximized)
        GetWindowRect( HANDLE, &wp. rcNormalPosition);
     if ( !SetWindowPlacement( HANDLE, &wp)) apiErr;
     var stage = oStage;
     if ( icon) SendMessage( HANDLE, WM_SETICON, ICON_BIG, ( LPARAM) icon);
  }
  else {
//   WINDOWPLACEMENT wp = {sizeof( WINDOWPLACEMENT)};
//   if ( windowState != wsNormal) {
//      if ( !GetWindowPlacement( HANDLE, &wp)) apiErr;
//      wp. showCmd = ( windowState == wsMinimized) ? SW_MINIMIZE : SW_SHOWMAXIMIZED;
//      if ( !SetWindowPlacement( HANDLE, &wp)) apiErr;
//  }
    guts. topWindows++;
  }
  if ( on_top >= 0) {
     apt_assign( aptOnTop, on_top);
     if ( on_top > 0)
        SetWindowPos( sys handle, HWND_TOPMOST, 0, 0, 0, 0, 
           SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }
  apc_window_set_caption( self, var text, is_opt( optUTF8_text));
  HWND_lock( false);
  return apcError == 0;
}

Bool
apc_window_activate( Handle self)
{
   if ( self) {
      HWND w;
      objCheck false;
      w = HANDLE;
      SetForegroundWindow( w); // no reasonable error description here,
      SetActiveWindow( w);     // long live M$DN :E
      return true;
   }
   return false;
}

Bool
apc_window_is_active( Handle self)
{
   return apc_window_get_active() == self;
}

Handle
apc_window_get_active( void)
{
   Handle self = hwnd_to_view( GetActiveWindow());
   if ( !self)
      return nilHandle;
   if ( sys className == WC_FRAME)
      return self;
   else if ( !is_apt( aptClipOwner)) {
      return hwnd_top_level( self);
   } else
      return nilHandle;
}

Bool
apc_window_close( Handle self)
{
   objCheck true;
   if ( !PostMessage( HANDLE, WM_CLOSE, 0, 0)) apiErrRet;
   return true;
}

int
apc_window_get_border_icons( Handle self)
{
   objCheck 0;
   return sys s. window. borderIcons;
}

int
apc_window_get_border_style( Handle self)
{
   objCheck 0;
   return sys s. window. borderStyle;
}

ApiHandle
apc_window_get_client_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) var handle;
}

Point
apc_window_get_client_pos( Handle self)
{
   Point delta = get_window_borders( sys s. window. borderStyle);
   Handle parent = var self-> get_parent( self);
   Point p = {0,0}, sz   = CWidget( parent)-> get_size( parent);
   RECT  r;

   objCheck p;
   if ( apc_window_get_window_state( self) == wsMinimized) {
      WINDOWPLACEMENT w = { sizeof( WINDOWPLACEMENT)};
      if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
      p. x = w. rcNormalPosition. left + delta. x;
      p. y = sz. y - w. rcNormalPosition. bottom + delta. y;
   } else {
      GetWindowRect( HANDLE, &r);
      p. x = r. left + delta. x;
      p. y = sz. y - r. bottom + delta. y;
   }
   return p;
}

Point
apc_window_get_client_size( Handle self)
{
   RECT r;
   Point p = { 0,0};
   objCheck p;
   if ( apc_window_get_window_state( self) == wsMinimized) {
      // cannot acquire client extension at this time. Using euristic calculations.
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      Point delta = get_window_borders( sys s. window. borderStyle);
      int   menuY  = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
      int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
                     GetSystemMetrics( SM_CYCAPTION) : 0;
      if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
      p. x = w. rcNormalPosition. right  - w. rcNormalPosition. left - delta. x * 2;
      p. y = w. rcNormalPosition. bottom - w. rcNormalPosition. top  - delta. y * 2
         - menuY - titleY;
   } else {
      GetWindowRect(( HWND) var handle, &r);
      p. x = r. right - r. left;
      p. y = r. bottom - r. top;
   }
   return p;
}

Bool
apc_window_get_icon( Handle self, Handle icon)
{
   HICON    p;
   HCURSOR  save;
   Bool ret;

   objCheck false;
   p = ( HICON) SendMessage( HANDLE, WM_GETICON, ICON_BIG, 0);
   if ( icon == nilHandle) return p != nil;

   save = sys pointer;
   sys pointer = p;
   ret = apc_pointer_get_bitmap( self, icon);
   sys pointer = save;
   return ret;
}

static void
map_tildas( WCHAR * buf, int len)
{
   int j;
   for ( j = 0; j < len; j++) {
      if ( buf[ j] == '~') {
          if ( buf[ j + 1] == '~')
             j++;
          else if ( buf[ j + 1])
             buf[ j] = '&';
          continue;
      } else if ( buf[ j] == '&') {
         memmove( buf + j + 1, buf + j, (len - j - 1) * sizeof( WCHAR));
         j++;
         continue;
      }
   }
   buf[len - 1] = 0;
}

static WCHAR *
map_text_accel( PMenuItemReg i)
{
   char * c;
   int l1, l2 = 0, amps = 0;
   WCHAR * buf;

   l1 = 1 + (( i-> flags. utf8_text && HAS_WCHAR) ? prima_utf8_length( i-> text) : strlen( i-> text));
   c = i-> text;  while (*c++) if ( *c == '&') amps++;
   if ( i-> accel) {
      l2 = 1 + (( i-> flags. utf8_accel && HAS_WCHAR) ? prima_utf8_length( i-> accel) : strlen( i-> accel));
      c = i-> accel; while (*c++) if ( *c == '&') amps++;
   }
   buf = malloc( sizeof( WCHAR) * ( l1 + l2 + amps));
   if ( !buf) return nil;
   memset( buf, 0, sizeof( WCHAR) * ( l1 + l2 + amps));
   
   if ( i-> flags. utf8_text && HAS_WCHAR) 
      utf8_to_wchar( i-> text, buf, strlen(i->text), l1 - 1);
   else
      MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, i-> text, l1 - 1, buf, l1 * 2 - 2);
   if ( i-> accel) {
      buf[l1 - 1] = '\t';
      if ( i-> flags. utf8_accel && HAS_WCHAR) 
         utf8_to_wchar( i-> accel, buf + l1, strlen(i->accel), l2);
      else
         MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, i-> accel, l2 - 1, buf + l1, l2 * 2 - 2);
   }
   map_tildas( buf, l1 + l2 + amps);
   if ( !HAS_WCHAR)
      wchar2char(( char*) buf, buf, l1 + l2 + amps); 
   return buf;
}

static HMENU
add_item( Bool menuType, Handle menu, PMenuItemReg i)
{
    MENUITEMINFOW menuItem;
    HMENU m;
    PMenuWndData mwd;
    PMenuItemReg first;

    if ( i == nil) return nil;

    if ( menuType)
       m = CreateMenu();
    else
       m = CreatePopupMenu();

    if ( !m) {
       apiErr;
       return nil;
    }
    mwd = ( PMenuWndData) malloc( sizeof( MenuWndData));
    if ( !mwd) {
       DestroyMenu( m);
       return nil;  
    }
    mwd-> menu = menu;
    first      = i;
    hash_store( menuMan, &m, sizeof( void*), mwd);

    while ( i != nil)
    {
       memset( &menuItem, 0, sizeof( menuItem));
       menuItem. cbSize   = sizeof( menuItem);
       menuItem. fMask    = MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
       menuItem. fType    = 0;
       menuItem. fType   |= ( i-> flags. divider    ) ? MFT_SEPARATOR    : 0;
       menuItem. fType   |= ( i-> bitmap     ) ? MFT_BITMAP       : 0;
       menuItem. fType   |= ( i-> text       ) ? MFT_STRING       : 0;
       menuItem. fType   |= ( i-> flags. rightAdjust) ? MFT_RIGHTJUSTIFY : 0;
       menuItem. fState   = 0;
       menuItem. fState  |= ( i-> flags. checked )    ? MFS_CHECKED      : 0;
       menuItem. fState  |= ( i-> flags. disabled)    ? MFS_GRAYED       : 0;
       menuItem. wID      = i-> id + MENU_ID_AUTOSTART;
       menuItem. hSubMenu = add_item( menuType, menu, i-> down);
       if (!( i-> flags. divider && i-> flags. rightAdjust)) {
          if ( i-> text) {
             menuItem. dwTypeData = map_text_accel( i);
          } else if ( i-> bitmap && PObject( i-> bitmap)-> stage < csDead)
             menuItem. dwTypeData = ( LPWSTR) image_make_bitmap_handle( i-> bitmap, nil);
          if ( HAS_WCHAR)
             InsertMenuItemW( m, -1, true, &menuItem);
          else
             InsertMenuItemA( m, -1, true, ( LPMENUITEMINFOA) &menuItem);
          if ( i-> text && menuItem. dwTypeData) free( menuItem. dwTypeData);
       }
       menuItem. dwItemData = menu;
       i = i-> next;
    }
    mwd-> id = first-> id;
    return m;
}


int
apc_window_get_window_state( Handle self)
{
   WINDOWPLACEMENT s = {sizeof( WINDOWPLACEMENT)};
   objCheck wsNormal;
   if ( !GetWindowPlacement( HANDLE, &s)) apiErr;
   if ( s. showCmd == SW_SHOWMAXIMIZED) return wsMaximized;
   if ( s. showCmd == SW_SHOWMINIMIZED) return wsMinimized;
   return wsNormal;
}

Bool
apc_window_get_task_listed( Handle self)
{
   objCheck false;
   return is_apt( aptTaskList);
}

Bool
apc_window_get_on_top( Handle self)
{
   objCheck false;
   return is_apt( aptOnTop);
}

Bool
apc_window_set_caption( Handle self, const char * caption, Bool utf8)
{
   objCheck false;
   if ( HAS_WCHAR && utf8) {
      WCHAR * c = alloc_utf8_to_wchar( caption, -1);
      if ( !( rc = SetWindowTextW( HANDLE, c))) apiErr;
      free( c);
   } else {
      if ( !( rc = SetWindowTextA( HANDLE, caption))) apiErr;
   }
   return rc == 0;
}

Bool
apc_window_set_client_pos( Handle self, int x, int y)
{
   Point delta = get_window_borders( sys s. window. borderStyle);
   RECT  r;
   Handle parent = var self-> get_parent( self);
   Point sz = CWidget( parent)-> get_size( parent);

   objCheck false;
   if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);
   apt_set( aptWinPosDetermined);

   if ( var stage == csConstructing && apc_window_get_window_state( self) != wsNormal) {
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
      w. rcNormalPosition. top    += sz. y - y + delta. y - w. rcNormalPosition. bottom;
      w. rcNormalPosition. bottom  = sz. y - y + delta. y;
      w. rcNormalPosition. right  += x - delta. x - w. rcNormalPosition. left;
      w. rcNormalPosition. left    = x - delta. x;
      w. flags   = 0;
      if ( !SetWindowPlacement( HANDLE, &w)) apiErr;
   } else {
      if ( !GetWindowRect( HANDLE, &r)) apiErr;
      x -= delta. x;
      y  = sz. y - ( r. bottom - r. top) - y + delta. y;
      SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
   }
   return true;
}

Bool
apc_window_set_client_size( Handle self, int x, int y)
{
   RECT r, c, c2;
   HWND h;
   int  ws = apc_window_get_window_state( self);

   objCheck false;
   if ( !hwnd_check_limits( x, y, false)) apcErrRet( errInvParams);

   h = HANDLE;
   if (( var stage == csConstructing && ws != wsNormal) || ws == wsMinimized) {
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      Point delta = get_window_borders( sys s. window. borderStyle);

      var virtualSize. x = x;
      var virtualSize. y = y;
      if ( x < 0) x = 0;
      if ( y < 0) y = 0;
      if ( !GetWindowPlacement( h, &w)) apiErr;
      if ( !GetWindowRect( h, &c2)) apiErr;
      if ( ws == wsMaximized) {
         if ( !GetClientRect( h, &c)) apiErr;
      }
      else {
         // cannot acquire client extension at this time. Using euristic calculations.
         int  menuY = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
         int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
                         GetSystemMetrics( SM_CYCAPTION) : 0;
         c = c2;
         c. right  -= delta. x * 2;
         c. bottom -= delta. y * 2 + menuY + titleY;
      }
      w. rcNormalPosition. top    = w. rcNormalPosition. bottom - y - ( c2. bottom - c2. top - c. bottom + c. top);
      w. rcNormalPosition. right  = x + ( c2. right - c2. left - c. right + c. left) + w. rcNormalPosition. left;
      w. flags   = 0;
      if ( !SetWindowPlacement( h, &w)) apiErr;
   } else {
      if ( !GetWindowRect( h, &r)) apiErr;
      if ( !GetClientRect( h, &c)) apiErr;
      sys sizeLockLevel++;
      var virtualSize. x = x;
      var virtualSize. y = y;
      if ( x < 0) x = 0;
      if ( y < 0) y = 0;
      SetWindowPos( h, 0,
         r. left,
         r. top - y + ( c. bottom - c. top),
         x + r. right  - r. left - c. right + c. left,
         y + r. bottom - r. top  - c. bottom + c. top,
         SWP_NOZORDER | SWP_NOACTIVATE | 
            ( is_apt( aptWinPosDetermined) ? 0 : SWP_NOMOVE)
         );
      sys sizeLockLevel--;
   }
   return true;
}

Bool
apc_window_set_client_rect( Handle self, int x, int y, int width, int height)
{
   RECT r, c, c2;
   HWND h;
   int  ws = apc_window_get_window_state( self);
   Point delta = get_window_borders( sys s. window. borderStyle);
   Handle parent = var self-> get_parent( self);
   Point sz = CWidget( parent)-> get_size( parent);

   objCheck false;
   if ( !hwnd_check_limits( x, y, false)) apcErrRet( errInvParams);
   if ( !hwnd_check_limits( width, height, false)) apcErrRet( errInvParams);
   apt_set( aptWinPosDetermined);

   h = HANDLE;
   if (( var stage == csConstructing && ws != wsNormal) || ws == wsMinimized) {
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      Point delta = get_window_borders( sys s. window. borderStyle);

      var virtualSize. x = width;
      var virtualSize. y = height;
      if ( width < 0) width = 0;
      if ( height < 0) height = 0;
      if ( !GetWindowPlacement( h, &w)) apiErr;
      if ( !GetWindowRect( h, &c2)) apiErr;
      if ( ws == wsMaximized) {
         if ( !GetClientRect( h, &c)) apiErr;
      }
      else {
         // cannot acquire client extension at this time. Using euristic calculations.
         int  menuY = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
         int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
                         GetSystemMetrics( SM_CYCAPTION) : 0;
         c = c2;
         c. right  -= delta. x * 2;
         c. bottom -= delta. y * 2 + menuY + titleY;
      }
      w. rcNormalPosition. bottom = sz. y - y + delta. y;
      w. rcNormalPosition. left   = x - delta. x;
      w. rcNormalPosition. top    = w. rcNormalPosition. bottom - height - ( c2. bottom - c2. top - c. bottom + c. top);
      w. rcNormalPosition. right  = width + ( c2. right - c2. left - c. right + c. left) + w. rcNormalPosition. left;
      w. flags   = 0;
      if ( !SetWindowPlacement( h, &w)) apiErr;
   } else {
      if ( !GetWindowRect( h, &r)) apiErr;
      if ( !GetClientRect( h, &c)) apiErr;
      sys sizeLockLevel++;
      x -= delta. x;
      y  = sz. y - y - height - ( r. bottom - r. top  - c. bottom + c. top) + delta. y;
      var virtualSize. x = width;
      var virtualSize. y = height;
      if ( width < 0) width = 0;
      if ( height < 0) height = 0;
      SetWindowPos( h, 0,
         x, y,
         width + r. right  - r. left - c. right + c. left,
         height + r. bottom - r. top  - c. bottom + c. top,
         SWP_NOZORDER | SWP_NOACTIVATE);
      sys sizeLockLevel--;
   }
   return true;
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
   Point size;
   objCheck false;
   apcErrClear;
   size = var self-> get_size( self);
   SetMenu( HANDLE, menu ? ( HMENU) (( PComponent) menu)-> handle : nil);
   DrawMenuBar( HANDLE);
   if ( apc_window_get_window_state( self) == wsNormal)
       var self-> set_size( self, size);
   return apcError == 0;
}


Bool
apc_window_set_icon( Handle self, Handle icon)
{
   HICON i;
   objCheck false;
   i = icon ? image_make_icon_handle( icon, guts. iconSizeLarge, nil, false) : nil;
   i = ( HICON) SendMessage( HANDLE, WM_SETICON, ICON_BIG, ( LPARAM) i);
   if ( i) DestroyIcon( i);
   return true;
}

Bool
apc_window_set_window_state( Handle self, int state)
{
   int  fl = -1;
   objCheck false;
   switch ( state)
   {
      case wsMaximized: fl = SW_SHOWMAXIMIZED; break;
      case wsMinimized: fl = SW_MINIMIZE; break;
      case wsNormal   : fl = SW_SHOWNORMAL; break;
   }
   if ( fl > 0)
   {
      ShowWindow( HANDLE, fl);
      sys s. window. state = state;
   }
   return true;
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insertBefore)
{
   HWND wnd;
   HWND active = GetActiveWindow();

   objCheck false;
   wnd = HANDLE;
   if ( sys className != WC_FRAME) { apcErr( errInvParams); return false; }

   sys s. window. oldFoc = apc_widget_get_focused();
   if ( sys s. window. oldFoc) protect_object( sys s. window. oldFoc);
   sys s. window. oldActive = active;

   // setting window up
   guts. focSysDisabled = 1;
   CWindow( self)-> exec_enter_proc( self, shared, insertBefore);
   apc_widget_set_enabled( self, 1);
   SetWindowPos( wnd, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
   if ( sys s. window. state == wsMinimized)
      ShowWindow( wnd, SW_RESTORE);
   if ( !insertBefore) {
      SetActiveWindow( wnd);
      SetForegroundWindow( wnd);
   } else {
      HWND zorder;
      dobjCheck( insertBefore) false;
      zorder = GetWindow( DHANDLE( insertBefore), GW_HWNDNEXT);
      if ( !zorder) zorder = HWND_BOTTOM;
      SetWindowPos( wnd, zorder, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
      if ( active)
         SetActiveWindow( active);
   }
   objCheck false;
   PostMessage( wnd, WM_DLGENTERMODAL, 1, 0);
   guts. focSysDisabled = 0;
   return true;
}

Bool
apc_window_execute( Handle self, Handle insertBefore)
{
   objCheck false;
   if ( !window_start_modal( self, false, insertBefore))
      return false;
   // message loop
   {
      MSG msg;
      while ( GetMessage( &msg, NULL, 0, 0)) {
         if ( !process_msg( &msg)) {
             if ( !appDead)
                PostThreadMessage( guts. mainThreadId, WM_TERMINATE, 0, 0);
             break;
         }
         if ( self && !(( PWindow) self)-> modal)
            break;
      }
   }
   // !!note - at this point object may be unaccessible (except var area only).
   return true;
}

Bool
apc_window_execute_shared( Handle self, Handle insertBefore)
{
   objCheck false;
   return window_start_modal( self, true, insertBefore);
}

Bool
apc_window_end_modal( Handle self)
{
   HWND wnd;
   objCheck false;
   wnd = HANDLE;
   guts. focSysDisabled = 1;
   CWindow( self)-> exec_leave_proc( self);
   WinHideWindow( wnd);
   objCheck false;
   apc_widget_set_enabled( self, 0);
   objCheck false;
   if ( application) {
      Handle who = Application_popup_modal( application);
      if ( sys s. window. oldActive)
         SetActiveWindow( sys s. window. oldActive);
      if ( !who && var owner)
         CWidget( var owner)-> set_selected( var owner, 1);
      if (( who = sys s. window. oldFoc)) {
         if ( PWidget( who)-> stage == csNormal)
            CWidget( who)-> set_focused( who, 1);
         unprotect_object( who);
      }
   }
   guts. focSysDisabled = 0;
   return true;
}

// View management
Bool
apc_widget_map_points( Handle self, Bool toScreen, int count, Point * p)
{
   int i;
   Point sz = ((( PWidget) self)-> self)-> get_size( self);
   Point appSz = apc_application_get_size( application);

   if ( self == application)
      return true;

   objCheck false;

   if ( toScreen) {
      for ( i = 0; i < count; i++)
         p[i]. y = sz. y - p[i]. y;
      MapWindowPoints(( HWND) var handle, nil, ( POINT * ) p, count);
      for ( i = 0; i < count; i++)
         p[i]. y = appSz. y - p[i].y;
   } else {
      for ( i = 0; i < count; i++)
         p[i]. y = appSz. y - p[i]. y;
      MapWindowPoints( nil, ( HWND) var handle, ( POINT * ) p, count);
      for ( i = 0; i < count; i++)
         p[i]. y = sz. y - p[i]. y;
   }
   return true;
}

Color
apc_widget_map_color( Handle self, Color color)
{
   if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0)) color |= var widgetClass;
   return remap_color( remap_color( color, true), false);  
}   

Bool
apc_widget_create( Handle self, Handle owner, Bool syncPaint, Bool clipOwner, Bool transparent, ApiHandle parentHandle)
{
   Bool reset = false;
   ViewProfile vprf;
   int oStage = var stage;
   int exstyle;

   objCheck false;
   dobjCheck( owner) false;

   if ( !kind_of( self, CWidget)) apcErr( errInvObject);
   apcErrClear;

   if ( parentHandle && !IsWindow(( HWND) parentHandle))
      return false;

   exstyle = 0;
   if (( var handle != nilHandle) &&
         (( DHANDLE( owner) != sys owner)                 ||
         (( HWND) parentHandle != sys parentHandle)       ||
          ( clipOwner       != is_apt( aptClipOwner))
      ))
   {
      if ( sys recreateData) {
         memcpy( &vprf, sys recreateData, sizeof( vprf));
         free( sys recreateData);
         sys recreateData = nil;
      } else
         get_view_ex( self, &vprf);
      reset = true;
   }
   if ( reset || ( var handle == nilHandle))
      create_group( self, owner, syncPaint, clipOwner, 0, WC_CUSTOM,
         WS_CHILD, exstyle, 1, 1, &vprf, ( HWND) parentHandle);
   apt_set( aptWinPosDetermined); 
   if ( reset)
   {
      Handle oldOwner = var owner; var owner = owner;
      set_view_ex( self, &vprf);
      var owner = oldOwner;
      var stage = oStage;
   }
   if ( is_apt( aptTransparent) != transparent && !reset) apc_widget_redraw( self);
   apt_assign( aptTransparent, transparent);
   if ( reset) apc_widget_redraw( self);
   return apcError == 0;
}

Bool
apc_widget_begin_paint( Handle self, Bool insideOnPaint)
{
   Bool useRPDraw = false;
   objCheck false;
   apcErrClear;

   if ( is_apt( aptTransparent))
   {
      if ( IsWindowVisible(( HWND) var handle)) {
         HWND parent = GetParent( HANDLE);
         if ( !parent) {
            MSG  msg;
            list_add( &guts. transp, self);
            WinHideWindow(( HWND) var handle);
            if ( parent) UpdateWindow( parent);
            while ( PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
               DispatchMessage( &msg);
            if ( !parent) Sleep( 1);
            WinShowWindow(( HWND) var handle);
            UpdateWindow(( HWND) var handle);
            list_delete( &guts. transp, self);
         } else
            useRPDraw = true;
      }
   }

   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   apt_assign( aptWM_PAINT, insideOnPaint);

   sys transform2. x = 0;
   sys transform2. y = 0;

   if ( insideOnPaint) {
      if ( !( sys ps = BeginPaint(( HWND) var handle, &sys paintStruc))) apiErrRet;
   } else {
      if ( !( sys ps = GetDC(( HWND) var handle))) apiErrRet;
   }

   if ( is_opt( optBuffered) && insideOnPaint) {
      RECT r;
      HBITMAP bm;
      HDC dc;

      GetClipBox( sys ps, &r);
      var w = r. right  - r. left;
      var h = r. bottom - r. top;

      if ( var w == 0 || var h == 0) {
         if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
         apt_clear( aptWinPS);
         apt_clear( aptWM_PAINT);
         apt_clear( aptCompatiblePS);
         sys ps = nil;
         return false;
      }

      if ( !( dc = CreateCompatibleDC( sys ps))) apiErr;

      if ( sys pal) {
         sys stockPalette = SelectPalette( dc, sys pal, 1);
         RealizePalette( dc);
         sys pal2 = SelectPalette( sys ps, sys pal, 1);
         RealizePalette( sys ps);
      }

      if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
         apt_clear( aptCompatiblePS);

      bm = CreateCompatibleBitmap( sys ps, var w, var h);

      if ( bm) {
         sys ps2 = sys ps;
         sys ps  = dc;
         sys stockBM = SelectObject( dc, bm);
         sys bm = bm;
         SetBrushOrgEx( dc, -r. left, -r. top, NULL);
         apc_gp_set_transform( self, -r. left, -r. top);
         sys transform2. x = r. left;
         sys transform2. y = r. top;
         apt_set( aptBitmap);
      } else
         apiErr;
   } else {
      if ( sys pal) {
         sys stockPalette = SelectPalette( sys ps, sys pal, 1);
         RealizePalette( sys ps);
      }
   }

   if ( useRPDraw) {
      HDC dc;
      HGDIOBJ o_pen, o_brush, o_font, o_pal, o_extpen;
      Handle owner = var owner;
      Point tr = dsys(owner)transform2;
      Point ed = apc_widget_get_pos( self);
      Point sz = apc_widget_get_size( self);
      Point so = CWidget(owner)-> get_size( owner);

      CWidget( owner)-> begin_paint( owner);
      dc = dsys( owner) ps;
      dsys( owner) ps = sys ps;
      dsys(owner) transform2. x += ed. x;
      dsys(owner) transform2. y += so. y - sz. y - ed. y;
      apc_gp_set_transform( owner, 0, 0);
      apc_gp_set_text_out_baseline( owner, dsys(owner) options. aptTextOutBaseline);

      SelectObject( sys ps, o_pen    = GetCurrentObject( dc, OBJ_PEN));
      SelectObject( sys ps, o_brush  = GetCurrentObject( dc, OBJ_BRUSH));
      SelectObject( sys ps, o_font   = GetCurrentObject( dc, OBJ_FONT));
      SelectObject( sys ps, o_pal    = GetCurrentObject( dc, OBJ_PAL));
      SelectObject( sys ps, o_extpen = GetCurrentObject( dc, OBJ_EXTPEN));

      CWidget( owner)-> notify( owner, "sH", "Paint", owner);

      SelectObject( sys ps, o_pen);
      SelectObject( sys ps, o_brush);
      SelectObject( sys ps, o_font);
      SelectObject( sys ps, o_pal);
      SelectObject( sys ps, o_extpen);

      dsys(owner)transform2 = tr;
      apc_gp_set_transform( owner, 0, 0);
      dsys( owner) ps = dc;
      CWidget( owner)-> end_paint( owner);
   }

   hwnd_enter_paint( self);

   if ( useRPDraw) {
      apc_gp_set_transform( self, sys transform. x, sys transform. y);
   }

   return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
   HRGN rgn;
   objCheck false;
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   sys transform2. x = 0;
   sys transform2. y = 0;
   if ( !( sys ps = GetDC(( HWND) var handle))) apiErrRet;
   hwnd_enter_paint( self);
   rgn = CreateRectRgn( 0, 0, 0, 0);
   SelectClipRgn( sys ps, rgn);
   DeleteObject( rgn);
   return true;
}


Bool
apc_widget_destroy( Handle self)
{
   objCheck false;
   SetWindowLongPtr( HANDLE, GWLP_USERDATA, 0);
   if ( sys pointer2) {
      if ( sys pointer2 == sys pointer) SetCursor( NULL); // un-use resource first
      if ( !DestroyCursor( sys pointer2)) apiErr;
   }
   if ( sys recreateData) free( sys recreateData);
   if ( self == lastMouseOver) lastMouseOver = nilHandle;
   free( sys timeDefs);
   if ( var handle == nilHandle) return true;

   if ( sys className == WC_FRAME)
      guts. topWindows--;

   if ( !DestroyWindow( HANDLE)) apiErrRet;
   return true;
}

PFont
apc_widget_default_font( PFont copyTo)
{
   *copyTo = guts. windowFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}

Bool
apc_widget_end_paint( Handle self)
{
   objCheck false;
   if ( is_opt( optBuffered)) {
      apt_clear( aptBitmap);
      if ( sys bm != nil) {
         if ( !SetViewportOrgEx( sys ps, 0, 0, nil)) apiErr;
         if ( !BitBlt( sys ps2, sys transform2. x, sys transform2. y, var w, var h, sys ps, 0, 0, SRCCOPY)) apiErr;
         if ( sys stockBM)
            SelectObject( sys ps, sys stockBM);
         DeleteObject( sys bm);
      }

      if ( sys pal) {
         SelectPalette( sys ps2, sys pal2, 1);
         SelectPalette( sys ps, sys stockPalette, 1);
         RealizePalette( sys ps2);
         sys pal2 = nil;
      }

      DeleteDC( sys ps);
      sys ps = sys ps2;
      sys bm = nil;
      sys ps2 = nil;
      sys stockBM = nil;
   }

   hwnd_leave_paint( self);

   if ( sys ps != nil) {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
      } else if ( is_apt( aptWinPS))
         if ( !ReleaseDC(( HWND) var handle, sys ps)) apiErr;
   }
   sys ps = nil;
   sys pal2 = nil;
   apt_clear( aptWinPS);
   apt_clear( aptWM_PAINT);
   apt_clear( aptCompatiblePS);
   return true;
}

Bool
apc_widget_end_paint_info( Handle self)
{
   Bool ok = true;
   objCheck false;
   hwnd_leave_paint( self);
   if ( !( ok = ReleaseDC(( HWND) var handle, sys ps))) apiErr;
   sys ps = nil;
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
   return ok;
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   objCheck false;
   return is_apt( aptClipOwner);
}

Color
apc_widget_get_color( Handle self, int index)
{
   objCheck clInvalid;
   return sys viewColors[ index];
}

Bool
apc_widget_get_first_click( Handle self)
{
   objCheck false;
   return is_apt( aptFirstClick);
}

Handle
apc_widget_get_focused()
{
   return hwnd_to_view( GetFocus());
}

static PRECT map_Rect( Handle self, Rect * rect)
{
   RECT  __rectangle;
   objCheck ( PRECT) rect;
   GetWindowRect(( HWND) var handle, &__rectangle);
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT) &__rectangle, 2);
   rect-> bottom = ( __rectangle. bottom - __rectangle. top) - rect-> bottom;
   rect-> top    = ( __rectangle. bottom - __rectangle. top) - rect-> top;
// that is due the difference in field placement between Rect and RECT structures
   if ( rect-> bottom > rect-> top) {
      LONG i = rect-> bottom;
      rect-> bottom = rect-> top;
      rect-> top    = i;
   }
   return ( PRECT) rect;
}

static Rect * map_RECT( Handle self, RECT * rect)
{
   RECT __rectangle;
   objCheck ( Rect*)rect;
   GetWindowRect(( HWND) var handle, &__rectangle);
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&__rectangle, 2);
   rect-> bottom = ( __rectangle. bottom - __rectangle. top) - rect-> bottom;
   rect-> top    = ( __rectangle. bottom - __rectangle. top) - rect-> top;
// that is due the difference in field placement between Rect and RECT structures
   if ( rect-> bottom < rect-> top) {
      LONG i = rect-> bottom;
      rect-> bottom = rect-> top;
      rect-> top    = i;
   }
   return ( Rect *) rect;
}

ApiHandle
apc_widget_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) HANDLE;
}

ApiHandle
apc_widget_get_parent_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys parentHandle;
}


Rect
apc_widget_get_invalid_rect( Handle self)
{
   Rect r  = {0,0,0,0};
   objCheck r;
   if ( GetUpdateRect(( HWND) var handle, ( PRECT) &r, false))
      return *( map_RECT( self, ( PRECT) &r));
   return r;
}

Point
apc_widget_get_pos( Handle self)
{
   RECT  r;
   Point p, sz = {0,0};
   Handle parent;
   objCheck sz;
   parent = is_apt( aptClipOwner) ? var owner : application;
   sz = ((( PWidget) parent)-> self)-> get_size( parent);
   if ( sys className == WC_FRAME && apc_window_get_window_state( self) == wsMinimized) {
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
      p. x = w. rcNormalPosition. left;
      p. y = sz. y - w. rcNormalPosition. bottom;
   } else {
      GetWindowRect( HANDLE, &r);
      if ( is_apt( aptClipOwner) && ( var owner != application))
         MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
      p. x = r. left;
      p. y = sz. y - r. bottom;
   }
   return p;
}

Point
apc_widget_get_size( Handle self)
{
   RECT r;
   Point p = {0,0};
   objCheck p;
   if ( sys className == WC_FRAME && apc_window_get_window_state( self) == wsMinimized) {
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
      p. x = w. rcNormalPosition. right  - w. rcNormalPosition. left;
      p. y = w. rcNormalPosition. bottom - w. rcNormalPosition. top;
   } else {
      GetWindowRect( HANDLE, &r);
      if ( is_apt( aptClipOwner) && ( var owner != application))
         MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
      p. x = r. right - r. left;
      p. y = r. bottom - r. top;
   }
   return p;
}

Handle
apc_widget_get_z_order( Handle self, int zOrderId)
{
   Handle h;
   HWND   w;
   UINT   cmd1, cmd2;
   objCheck nilHandle;

   switch ( zOrderId) {
   case zoFirst:
      cmd1 = GW_HWNDFIRST;
      cmd2 = GW_HWNDNEXT;
      break;
   case zoLast:
      cmd1 = GW_HWNDLAST;
      cmd2 = GW_HWNDPREV;
      break;
   case zoNext:
      cmd1 = cmd2 = GW_HWNDNEXT;
      break;
   case zoPrev:
      cmd1 = cmd2 = GW_HWNDPREV;
      break;
   default:
      apcErr( errInvParams);
      return nilHandle;
   }

   w = GetWindow( HANDLE, cmd1);
   if ( !w) return nilHandle;
   h = hwnd_to_view( w);
   while ( h == nilHandle) {
      w = GetWindow( w, cmd2);
      if ( !w) return nilHandle;
      h = hwnd_to_view( w);
   }

   return h;
}


Bool
apc_widget_get_shape( Handle self, Handle mask)
{
   HRGN rgn;
   int res;
   HBITMAP bm, bmSave;
   HBRUSH  brSave;
   HDC dc;
   XBITMAPINFO xbi;
   BITMAPINFO * bi;

   objCheck false;
   rgn = CreateRectRgn(0,0,0,0);

   res = GetWindowRgn( HANDLE, rgn);
   if ( res == ERROR) {
      DeleteObject( rgn);
      return false;
   }
   if ( !mask) {
      DeleteObject( rgn);
      return true;
   }

   CImage( mask)-> create_empty( mask, sys extraBounds. x, sys extraBounds. y, imBW);

   if (!( dc = dc_compat_alloc(0))) return true;
   if ( !( bm = CreateBitmap( PImage( mask)-> w, PImage( mask)-> h, 1, 1, nil))) {
      dc_compat_free();
      return true;
   }

   bmSave = SelectObject( dc, bm);
   brSave = SelectObject( dc, CreateSolidBrush( RGB(0,0,0)));
   Rectangle( dc, 0, 0, PImage( mask)-> w, PImage( mask)-> h);
   DeleteObject( SelectObject( dc, CreateSolidBrush( RGB( 255, 255, 255))));
   SetViewportOrgEx( dc, -sys extraPos. x, -sys extraPos. y, NULL);
   PaintRgn( dc, rgn);
   DeleteObject( SelectObject( dc, brSave));

   bi = image_get_binfo( mask, &xbi);
   if ( !GetDIBits( dc, bm, 0, PImage( mask)-> h, PImage( mask)-> data, bi, DIB_RGB_COLORS)) apiErr;
   SelectObject( dc, bmSave);
   DeleteObject( bm);
   dc_compat_free();

   DeleteObject( rgn);

   return true;
}


Bool
apc_widget_get_sync_paint( Handle self)
{
   objCheck false;
   return is_apt( aptSyncPaint);
}

Bool
apc_widget_get_transparent( Handle self)
{
   objCheck false;
   return is_apt( aptTransparent);
}

Bool
apc_widget_is_captured( Handle self)
{
   objCheck false;
   return GetCapture() == ( HWND) var handle;
}

Bool
apc_widget_is_enabled( Handle self)
{
   objCheck false;
   return is_apt( aptEnabled);
   // return IsWindowEnabled( HANDLE);
}

Bool
apc_widget_is_responsive( Handle self)
{
   Bool ena = true;
   objCheck false;
   while ( ena && self != application) {
      // ena  = IsWindowEnabled( HANDLE);
      ena  = is_apt( aptEnabled);
      self = var owner;
   }
   return ena;
}

Bool
apc_widget_is_focused( Handle self)
{
   objCheck false;
   return is_apt( aptFocused);
}

Bool
apc_widget_is_visible( Handle self)
{
   objCheck false;
   return ( GetWindowLong(HANDLE, GWL_STYLE) & WS_VISIBLE) ? 1 : 0;
}

Bool
apc_widget_is_showing( Handle self)
{
   objCheck false;
   return IsWindowVisible( HANDLE);
}

Bool
apc_widget_is_exposed( Handle self)
{
   HWND h;
   HRGN rgnSave = NULL;
   HRGN rgn     = NULL;
   int  rgnSaveType, rgnType;

   objCheck false;

   h = ( HWND) var handle;
   if ( !IsWindowVisible( h)) return false;

   rgnSave = CreateRectRgn(0,0,0,0);
   rgn     = CreateRectRgn(0,0,0,0);
   rgnSaveType = GetUpdateRgn( h, rgnSave, FALSE);
   rgnSaveType = ( rgnSaveType == COMPLEXREGION || rgnSaveType == SIMPLEREGION);

   if ( rgnSaveType) ValidateRect( h, NULL);

   InvalidateRect( h, NULL, false);
   rgnType = GetUpdateRgn( h, rgn, FALSE);
   ValidateRect( h, NULL);

   if ( rgnSaveType) InvalidateRgn( h, rgnSave, FALSE);
   DeleteObject( rgnSave);
   DeleteObject( rgn);
   return ( rgnType == COMPLEXREGION || rgnType == SIMPLEREGION);
}


Bool
apc_widget_invalidate_rect( Handle self, Rect * rect)
{
   PRECT pRect = rect ? map_Rect( self, rect) : nil;
   objCheck false;
   if ( !InvalidateRect (( HWND) var handle, pRect, false)) apiErr;
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
   objCheck false;
   process_transparents( self);
   return true;
}

Bool
apc_widget_scroll( Handle self, int horiz, int vert, Rect * r, Rect *cr, Bool scrollChildren)
{
   PRECT pRect = r ? map_Rect( self, r) : nil;
   PRECT pClipRect = cr ? map_Rect( self, cr) : nil;
   Point sz = apc_widget_get_size( self);
   objCheck false;

   HideCaret(( HWND) var handle);

   if ( pClipRect) {
      if ( pClipRect-> left < 0) pClipRect-> left = 0;
      if ( pClipRect-> top  < 0) pClipRect-> top = 0;
      if ( pClipRect-> right  > sz. x) pClipRect-> right = sz. x;
      if ( pClipRect-> bottom > sz. y) pClipRect-> bottom = sz. y;
   }

   if ( pRect) {
      if ( pRect-> left < -sz. x * 2) pRect-> left = -sz. x * 2;
      if ( pRect-> top  < -sz. y * 2) pRect-> top = -sz. y * 2;
      if ( pRect-> right  > sz. x * 2) pRect-> right = sz. x * 2;
      if ( pRect-> bottom > sz. y * 2) pRect-> bottom = sz. y * 2;
   }

   if ( horiz > sz. x || horiz < -sz. x || vert > sz. y || vert < -sz. y) {
      if ( pRect && pClipRect) {
         RECT rc;
         UnionRect( &rc, (RECT*)pRect, (RECT*)pClipRect);
         InvalidateRect(( HWND) var handle, &rc, false);
      } else 
         InvalidateRect(( HWND) var handle, pRect ? pRect : pClipRect, false);
   } else {
      if ( !ScrollWindowEx(( HWND) var handle,
         horiz, -vert, pRect, pClipRect, NULL, NULL,
         SW_INVALIDATE | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
      )) {
         ShowCaret(( HWND) var handle);
         apiErr;
      }
   }
   objCheck false;
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
   return ShowCaret(( HWND) var handle);
}

Bool
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
   objCheck false;
   if ( capture) {
      SetCapture(( HWND) var handle);
      if ( confineTo) {
         RECT r;
         GetWindowRect(( HWND) PComponent( confineTo)-> handle, &r);
         if ( !ClipCursor( &r)) apiErrRet;
      }
   } else {
      if ( !ReleaseCapture()) apiErrRet;
      if ( !ClipCursor( NULL)) apiErrRet;
   }
   return true;
}

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

Bool
apc_widget_set_color( Handle self, Color color, int index)
{
   Event ev = {cmColorChanged};
   objCheck false;
   sys viewColors[ index] = color;

   ev. gen. source = self;
   ev. gen. i      = index;
   var self-> message( self, &ev);
   return true;
}

Bool
apc_widget_set_enabled( Handle self, Bool enable)
{
   objCheck false;
   apt_assign( aptEnabled, enable);
   if (( sys className == WC_FRAME) || ( var owner == application))
      EnableWindow( HANDLE, enable);
   else
      SendMessage( HANDLE, WM_ENABLE, ( WPARAM) enable, 0);
   return true;
}

Bool
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   objCheck false;
   apt_assign( aptFirstClick, firstClick);
   return true;
}

Bool
apc_widget_set_focused( Handle self)
{
   if ( self && ( self != Application_map_focus( application, self)))
      return false;
   guts. focSysGranted++;
   SetFocus( self ? (( HWND) var handle) : nil);
   guts. focSysGranted--;
   return true;
}

Bool
apc_widget_set_font( Handle self, PFont font)
{
   Event ev = {cmFontChanged};
   objCheck false;
   ev. gen. source = self;
   var self-> message( self, &ev);
   return true;
}

Bool
apc_widget_set_palette( Handle self)
{
   objCheck false;
   apc_gp_set_palette( self);
   if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
      palette_change( self);
   return true;
}

Bool
apc_widget_set_pos( Handle self, int x, int y)
{
   Handle parent;
   Point sz;
   RECT r;

   objCheck false;
   if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);

   parent = is_apt( aptClipOwner) ? var owner : application;
   sz = ((( PWidget) parent)-> self)-> get_size( parent);
   apt_set( aptWinPosDetermined);

   if ( sys className == WC_FRAME) {
      HWND h = HANDLE;
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
         if ( !GetWindowPlacement( h, &w)) apiErrRet;
         w. rcNormalPosition. top    += sz. y - y - w. rcNormalPosition. bottom;
         w. rcNormalPosition. bottom  = sz. y - y;
         w. rcNormalPosition. right  += x - w. rcNormalPosition. left;
         w. rcNormalPosition. left    = x;
         w. flags = 0;
         if ( !SetWindowPlacement( h, &w)) apiErrRet;
         return true;
      }
   }
   if ( !GetWindowRect( HANDLE, &r)) apiErrRet;
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
   if ( sys parentHandle) {
      POINT ppos;
      ppos. x = x;
      ppos. y = dsys( application) lastSize. y - y;
      MapWindowPoints( NULL, sys parentHandle, ( LPPOINT)&ppos, 1);
      GetWindowRect( sys parentHandle, &r);
      x = ppos. x;
      y = ppos. y;
   } else
      y = sz. y - y - r. bottom + r. top;

   if ( !SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErrRet;
   return true;
}

Bool
apc_widget_set_size( Handle self, int width, int height)
{
   RECT r;
   HWND h;
   objCheck false;

   if ( !hwnd_check_limits( width, height, false)) apcErrRet( errInvParams);
   h = HANDLE;
   if ( sys className == WC_FRAME) {
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
         if ( !GetWindowPlacement( h, &w)) apiErrRet;
         if ( width  < 0) width = 0;
         if ( height < 0) height = 0;
         w. rcNormalPosition. top    = w. rcNormalPosition. bottom - height;
         w. rcNormalPosition. right  = width + w. rcNormalPosition. left;
         w. flags = 0;
         if ( !SetWindowPlacement( h, &w)) apiErrRet;
         return true;
      }
   }
   if ( !GetWindowRect( h, &r)) apiErrRet;
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
   if ( sys parentHandle)
      MapWindowPoints( NULL, sys parentHandle, ( LPPOINT)&r, 2);

   if ( sys className != WC_FRAME) {
      sys sizeLockLevel++;
      var virtualSize. x = width;
      var virtualSize. y = height;
   }
   if ( height < 0) height = 0;
   if ( width  < 0) width  = 0;
   if ( !SetWindowPos( h, 0,
      r. left, r. bottom - height,
      width, height,
      SWP_NOZORDER | SWP_NOACTIVATE | 
         ( is_apt( aptWinPosDetermined) ? 0 : SWP_NOMOVE)
      )) apiErrRet;
   if ( sys className != WC_FRAME) sys sizeLockLevel--;
   return true;
}
  
Bool
apc_widget_set_rect( Handle self, int x, int y, int width, int height)
{
   RECT r;
   HWND h;
   Handle parent;
   Point sz;
   objCheck false;

   if ( !hwnd_check_limits( width, height, false)) apcErrRet( errInvParams);
   if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);

   parent = is_apt( aptClipOwner) ? var owner : application;
   sz = ((( PWidget) parent)-> self)-> get_size( parent);
   apt_set( aptWinPosDetermined);

   h = HANDLE;
   if ( sys className == WC_FRAME) {
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
         if ( !GetWindowPlacement( h, &w)) apiErrRet;
         if ( width  < 0) width = 0;
         if ( height < 0) height = 0;
         w. rcNormalPosition. left    = x;
         w. rcNormalPosition. bottom  = sz. y - y;
         w. rcNormalPosition. right   = x + width;
         w. rcNormalPosition. top     = sz. y - y - height;
         w. flags = 0;
         if ( !SetWindowPlacement( h, &w)) apiErrRet;
         return true;
      }
   }
   if ( !GetWindowRect( h, &r)) apiErrRet;
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);

   if ( sys className != WC_FRAME) {
      sys sizeLockLevel++;
      var virtualSize. x = width;
      var virtualSize. y = height;
   }
   if ( height < 0) height = 0;
   if ( width  < 0) width  = 0;
   if ( sys parentHandle) {
      POINT ppos;
      ppos. x = x;
      ppos. y = dsys( application) lastSize. y - y;
      MapWindowPoints( NULL, sys parentHandle, ( LPPOINT)&ppos, 1);
      GetWindowRect( sys parentHandle, &r);
      x = ppos. x;
      y = ppos. y;
   } else
      y = sz. y - y - height;
   
   if ( !SetWindowPos( h, 0, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE)) 
      apiErrRet;
   if ( sys className != WC_FRAME) sys sizeLockLevel--;
   return true;
}


Bool
apc_widget_set_size_bounds( Handle self, Point min, Point max)
{
   return true;
}   

Bool
apc_widget_set_shape( Handle self, Handle mask)
{
   HRGN rgn = nil;
   objCheck false;

   rgn = region_create( mask);
   if ( !rgn) {
      SetWindowRgn( HANDLE, nil, true);
      return true;
   }

   sys extraBounds. x = PImage( mask)-> w;
   sys extraBounds. y = PImage( mask)-> h;
   if ( sys className == WC_FRAME) {
      Point delta = get_window_borders( sys s. window. borderStyle);
      Point sz    = apc_widget_get_size( self);
      Point p     = sys extraBounds;
      HRGN  r1, r2;
      OffsetRgn( rgn, delta.x, sz. y - p.y - delta.y);
      sys extraPos. x = delta.x;
      sys extraPos. y = sz. y - p.y - delta.y;
      r1 = CreateRectRgn( 0, 0, 8192, 8192);
      r2 = CreateRectRgn( delta. x, sz. y - delta. y - p.y,
         delta.x + p.x + 1, sz. y - delta. y + 1);
      CombineRgn( r1, r1, r2, RGN_XOR);
      CombineRgn( rgn, rgn, r1, RGN_OR);
      DeleteObject( r1);
      DeleteObject( r2);
   } else
      sys extraPos. x = sys extraPos. y = 0;

   if ( !SetWindowRgn( HANDLE, rgn, true))
      apiErrRet;
   return true;
}

Bool
apc_widget_set_visible( Handle self, Bool show)
{
   objCheck false;
   if ( !SetWindowPos( HANDLE, nil, 0, 0, 0, 0,
        ( show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)
        | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErrRet;
   objCheck false;
   if ( !is_apt( aptClipOwner)) {
      InvalidateRect(( HWND) var handle, nil, false);
      objCheck false;
      process_transparents( self);
   }
   return true;
}

Bool
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
   HWND opt = ( top) ? HWND_TOP : HWND_BOTTOM;
   objCheck false;
   if ( behind != nilHandle) {
      dobjCheck( behind) false;
      opt = DHANDLE( behind);
   }
   if ( !SetWindowPos( HANDLE, opt, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)) apiErrRet;
   return true;
}

Bool
apc_widget_update( Handle self)
{
   objCheck false;
   if ( !UpdateWindow(( HWND) var handle)) apiErrRet;
   return true;
}

Bool
apc_widget_validate_rect( Handle self, Rect rect)
{
   objCheck false;
   if ( !ValidateRect (( HWND) var handle, map_Rect( self, &rect))) apiErrRet;
   return true;
}

// View attributes

int
apc_kbd_get_state( Handle self)
{
  return
      (( GetKeyState( VK_MENU)    < 0) ? kmAlt      : 0) |
      (( GetKeyState( VK_CONTROL) < 0) ? kmCtrl     : 0) |
      (( GetKeyState( VK_SHIFT)   < 0) ? kmShift    : 0);
}

Bool
apc_menu_create( Handle self, Handle owner)
{
   objCheck false;
   dobjCheck( owner) false;
   sys className = WC_MENU;
   sys owner     = DHANDLE( owner);
   return true;
}

static Bool clear_menus( PMenuWndData item, int keyLen, void * key, void * params)
{
   if ( item-> menu == ( Handle) params)
      hash_delete( menuMan, key, keyLen, true);
   return false;
}

Bool
apc_menu_destroy( Handle self)
{
   if ( var handle) {
      objCheck false;
      hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
      if ( IsMenu(( HMENU) var handle) && !DestroyMenu(( HMENU) var handle)) apiErrRet;
      return true;
   }
   return false;
}

PFont
apc_menu_default_font( PFont copyTo)
{
   *copyTo = guts. menuFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}

Color
apc_menu_get_color( Handle self, int index)
{
   return remap_color( index | var widgetClass, true);
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
   return apc_menu_default_font( font);
}

Bool
apc_menu_set_color( Handle self, Color color, int index)
{
   return true;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
   return true;
}

Bool
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
   PWindow owner = nil;
   Point size;
   Bool resize;
   objCheck false;
   dobjCheck( var owner) false;
   if (( resize = kind_of( var owner, CWindow) &&
        kind_of( self, CMenu) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self))) {
      owner = ( PWindow) var owner;
      size = owner-> self-> get_size( var owner);
   }

   // since GetMenuItemInfo does not work in NT, do not free menuMan entries here,
   // they'll be freed in apc_menu_destroy anyway.

   DeleteMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   DrawMenuBar( DHANDLE( var owner));
   if ( resize && apc_window_get_window_state( var owner) == wsNormal)
      owner-> self-> set_size( var owner, size);
   return true;
}

Bool
apc_menu_item_set_accel( Handle self, PMenuItemReg m)
{
   UINT flags;
   WCHAR * buf;

   if ( !var handle) return false;
   objCheck false;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) return false;

   if ( flags & MF_BITMAP)
      flags = ( flags & ~MF_BITMAP) | MF_STRING;

   apcErrClear;
   buf = map_text_accel( m);
   if ( HAS_WCHAR) {
      if ( !ModifyMenuW(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                       m-> id + MENU_ID_AUTOSTART, buf)) 
         apiErr;
   } else {
      if ( !ModifyMenuA(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                       m-> id + MENU_ID_AUTOSTART, ( char *) buf)) 
         apiErr;
   }
   free( buf);
   return rc == errOk;
}

Bool
apc_menu_item_set_check( Handle self, PMenuItemReg m)
{
   DWORD res;
   if ( !var handle) return false;
   objCheck false;
   res = CheckMenuItem(( HMENU) var handle,
      m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( m-> flags. checked ? MF_CHECKED : MF_UNCHECKED));
   return res != 0xFFFFFFFF;
}

Bool
apc_menu_item_set_enabled( Handle self, PMenuItemReg m)
{
   DWORD res;
   if ( !var handle) return false;
   objCheck false;
   res = EnableMenuItem(( HMENU) var handle,
      m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( m-> flags. disabled ? MF_GRAYED : MF_ENABLED));
   DrawMenuBar( DHANDLE( var owner));
   return res != 0xFFFFFFFF;
}

Bool
apc_menu_item_set_key( Handle self, PMenuItemReg m)
{
   return true;
}

Bool
apc_menu_item_set_text( Handle self, PMenuItemReg m)
{
   WCHAR * buf;
   UINT flags;

   if ( !var handle) return false;
   objCheck false;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) return false;

   if ( flags & MF_BITMAP)
      flags = ( flags & ~MF_BITMAP) | MF_STRING;

   apcErrClear;
   buf = map_text_accel( m);
   if ( HAS_WCHAR) {
      if ( !ModifyMenuW(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                    m-> id + MENU_ID_AUTOSTART, (WCHAR*) buf)) apiErr;
   } else {
      if ( !ModifyMenuA(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                    m-> id + MENU_ID_AUTOSTART, (char*) buf)) apiErr;
   }
   free( buf);
   return rc == errOk;
}

Bool
apc_menu_item_set_image( Handle self, PMenuItemReg m)
{
   UINT flags;

   if ( !var handle) return false;
   objCheck false;
   dobjCheck( m-> bitmap) false;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) return false;

   flags |= MF_BITMAP;

   if ( HAS_WCHAR) {
      if ( !ModifyMenuW(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                       m-> id + MENU_ID_AUTOSTART, 
                       ( LPCWSTR) image_make_bitmap_handle( m-> bitmap, nil))) apiErrRet;
   } else {
      if ( !ModifyMenuA(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                       m-> id + MENU_ID_AUTOSTART, 
                       ( LPCSTR) image_make_bitmap_handle( m-> bitmap, nil))) apiErrRet;
   }
   return true;
}


ApiHandle
apc_menu_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) var handle;
}

Bool
apc_menu_update( Handle self, PMenuItemReg oldBranch, PMenuItemReg newBranch)
{
   objCheck false;
   dobjCheck( var owner) false;

   if ( kind_of( var owner, CWindow) &&
        kind_of( self, CMenu) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self)) {
      HMENU h = GetMenu( DHANDLE( var owner));
      PWindow owner = ( PWindow) var owner;
      Point size = owner-> self-> get_size( var owner);
      if ( h) DestroyMenu( h);
      hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
      var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
      SetMenu( DHANDLE( var owner), self ? ( HMENU) var handle : nil);
      DrawMenuBar( DHANDLE( var owner));
      if ( apc_window_get_window_state( var owner) == wsNormal)
         owner-> self-> set_size( var owner, size);
   } else {
      if ( var handle)
         DestroyMenu(( HMENU) var handle);
      hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
      var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
   }
   return true;
}

Bool
apc_popup_create( Handle self, Handle owner)
{
   objCheck false;
   dobjCheck( owner) false;
   sys owner = DHANDLE( owner);
   sys className = WC_MENU;
   return true;
}

PFont
apc_popup_default_font( PFont font)
{
   return apc_menu_default_font( font);
}

Bool
apc_popup( Handle self, int x, int y, Rect * anchor)
{
   TPMPARAMS tpm;
   HWND owner;
   Bool ret = true;
   objCheck false;

   if ( guts. popupActive) return false;

   y = dsys( var owner) lastSize. y - y;
   owner = ( HWND)(( PComponent) var owner)-> handle;
   if ( var owner != application) {
      POINT pt;
      pt. x = x;
      pt. y = y;
      if ( !MapWindowPoints( owner, nil, &pt, 1)) apiErr;
      x = pt.x;
      y = pt.y;
   }
   if ( anchor &&
        anchor-> left   &&
        anchor-> right  &&
        anchor-> top    &&
        anchor-> bottom
      )
   {
      RECT r;
      GetWindowRect( owner, &r);
      tpm. cbSize = sizeof( tpm);
      tpm. rcExclude. left   = anchor-> left;
      tpm. rcExclude. right  = anchor-> right;
      tpm. rcExclude. top    = r. bottom - r. top - anchor-> top;
      tpm. rcExclude. bottom = r. bottom - r. top - anchor-> bottom;
      if ( !MapWindowPoints( owner, nil, ( PPOINT) &tpm. rcExclude, 2)) apiErr;
   } else
      anchor = nil;

   guts. popupActive = 1;
   ret = TrackPopupMenuEx(
      ( HMENU) var handle, TPM_LEFTBUTTON|TPM_LEFTALIGN|TPM_RIGHTBUTTON,
       x, y, owner, anchor ? &tpm : NULL);
   guts. popupActive = 0;
   return ret;
}


Handle ctx_kb2VK[] = {
   kbNoKey       ,   0                 ,
   kbAltL        ,   VK_MENU           ,
   kbAltR        ,   VK_RMENU          ,
   kbCtrlL       ,   VK_CONTROL        ,
   kbCtrlR       ,   VK_RCONTROL       ,
   kbShiftL      ,   VK_SHIFT          ,
   kbShiftR      ,   VK_RSHIFT         ,
   kbBackspace   ,   VK_BACK           ,
   kbTab         ,   VK_TAB            ,
   kbPause       ,   VK_PAUSE          ,
   kbEsc         ,   VK_ESCAPE         ,
   kbSpace       ,   VK_SPACE          ,
   kbPgUp        ,   VK_PRIOR          ,
   kbPgDn        ,   VK_NEXT           ,
   kbEnd         ,   VK_END            ,
   kbHome        ,   VK_HOME           ,
   kbLeft        ,   VK_LEFT           ,
   kbUp          ,   VK_UP             ,
   kbRight       ,   VK_RIGHT          ,
   kbDown        ,   VK_DOWN           ,
   kbPrintScr    ,   VK_PRINT          ,
   kbInsert      ,   VK_INSERT         ,
   kbDelete      ,   VK_DELETE         ,
   kbEnter       ,   VK_RETURN         ,
   kbF1          ,   VK_F1             ,
   kbF2          ,   VK_F2             ,
   kbF3          ,   VK_F3             ,
   kbF4          ,   VK_F4             ,
   kbF5          ,   VK_F5             ,
   kbF6          ,   VK_F6             ,
   kbF7          ,   VK_F7             ,
   kbF8          ,   VK_F8             ,
   kbF9          ,   VK_F9             ,
   kbF10         ,   VK_F10            ,
   kbF11         ,   VK_F11            ,
   kbF12         ,   VK_F12            ,
   kbF13         ,   VK_F13            ,
   kbF14         ,   VK_F14            ,
   kbF15         ,   VK_F15            ,
   kbF16         ,   VK_F16            ,
   kbNumLock     ,   VK_NUMLOCK        ,
   kbScrollLock  ,   VK_SCROLL         ,
   kbCapsLock    ,   VK_CAPITAL        ,
   kbClear       ,   VK_CLEAR          ,
   kbSelect      ,   VK_SELECT         ,
   kbExecute     ,   VK_EXECUTE        ,
   kbSysRq       ,   VK_SNAPSHOT       ,
   endCtx
};

Handle ctx_kb2VK2[] = {
   kbBackspace   ,   VK_BACK           ,
   kbTab         ,   VK_TAB            ,
   kbEsc         ,   VK_ESCAPE         ,
   kbSpace       ,   VK_SPACE          ,
   kbEnter       ,   VK_RETURN         ,
   endCtx
};

Handle ctx_kb2VK3[] = {
   kbAltL        ,   kbAltR            ,
   kbShiftL      ,   kbShiftR          ,
   kbCtrlL       ,   kbCtrlR           ,
   endCtx
};

Bool
apc_message( Handle self, PEvent ev, Bool post)
{
   ULONG msg;
   USHORT mp1s = 0;
   objCheck false;
   switch ( ev-> cmd)
   {
       case cmPost:
          if (post)
             PostMessage(( HWND) var handle, WM_POSTAL, ( WPARAM) ev-> gen. H, ( LPARAM) ev-> gen. p);
	  else
             SendMessage(( HWND) var handle, WM_POSTAL, ( WPARAM) ev-> gen. H, ( LPARAM) ev-> gen. p);
          break;
       case cmMouseMove:
          msg = WM_MOUSEMOVE;
          goto general;
       case cmMouseUp:
          if ( ev-> pos. button & mbMiddle) msg = WM_MBUTTONUP; else
          if ( ev-> pos. button & mbRight)  msg = WM_RBUTTONUP; else
          msg = WM_LBUTTONUP;
          goto general;
       case cmMouseDown:
          if ( ev-> pos. button & mbMiddle) msg = WM_MBUTTONDOWN; else
          if ( ev-> pos. button & mbRight)  msg = WM_RBUTTONDOWN; else
          msg = WM_LBUTTONDOWN;
          goto general;
       case cmMouseWheel:
          msg  = WM_MOUSEWHEEL;
          mp1s = ( SHORT) ev-> pos. button;
          goto general;
       case cmMouseClick:
          if ( ev-> pos. dblclk)
          {
             if ( ev-> pos. button & mbMiddle) msg = WM_MBUTTONDBLCLK; else
             if ( ev-> pos. button & mbRight)  msg = WM_RBUTTONDBLCLK; else
             msg = WM_LBUTTONDBLCLK;
          } else {
             Event newEvent = *ev;
             if ( ev-> pos. button & mbMiddle) msg = WM_MMOUSECLICK; else
             if ( ev-> pos. button & mbRight)  msg = WM_RMOUSECLICK; else
             msg = WM_LMOUSECLICK;
             newEvent. cmd = cmMouseDown;
             apc_message( self, &newEvent, post);
             newEvent. cmd = cmMouseUp;
             apc_message( self, &newEvent, post);
          }
       general:
          {
             LPARAM mp2 = MAKELPARAM( ev-> pos. where. x, sys lastSize. y - ev-> pos. where. y - 1);
             WPARAM mp1 = mp1s |
               (( ev-> pos. mod & kmShift) ? MK_SHIFT   : 0) |
               (( ev-> pos. mod & kmCtrl ) ? MK_CONTROL : 0);
             if ( post) {
                KeyPacket * kp;
                kp = ( KeyPacket *) malloc( sizeof( KeyPacket));
                if ( kp) {
                   kp-> mp1 = mp1;
                   kp-> mp2 = mp2;
                   kp-> msg = msg;
                   kp-> wnd = ( HWND) var handle;
                   kp-> mod = ev-> pos. mod;
                   PostMessage( 0, WM_KEYPACKET, 0, ( LPARAM) kp);
                }
             } else {
                BYTE * mod = nil;
                if (( GetKeyState( VK_MENU) < 0) ^ (( ev-> pos. mod & kmAlt) != 0))
                   mod = mod_select( ev-> pos. mod);
                SendMessage(( HWND) var handle, msg, mp1, mp2);
                if ( mod) mod_free( mod);
             }
          }
          break;
       case cmKeyDown:
       case cmKeyUp:
          {
             WPARAM mp1;
             LPARAM mp2;
             int scan = 0;
             UINT msg;
             Bool specF10 = ( ev-> key. key == kbF10) && !( ev-> key. mod & kmAlt);
             // constructing mp1
             if ( ev-> key. key == kbNoKey) {
                if ( ev-> key. code == 0) {
                   if ( ev-> key. mod & kmAlt   ) mp1 = VK_MENU;    else
                   if ( ev-> key. mod & kmShift ) mp1 = VK_SHIFT;   else
                   if ( ev-> key. mod & kmCtrl  ) mp1 = VK_CONTROL; else
                      return false;
                } else {
                   SHORT c = VkKeyScan(( CHAR ) ev-> key. code);
                   if ( c == -1) {
                      HKL kl = guts. keyLayout ? guts. keyLayout : GetKeyboardLayout( 0);
                      c = VkKeyScanEx(( CHAR) ev-> key. code, kl);
                      if ( c == -1) return false;
                      scan = MapVirtualKeyEx( LOBYTE( c), 0, kl);
                   } else {
                      scan = MapVirtualKey( LOBYTE( c), 0);
                   }
                   mp1 = LOBYTE( c);
                   c = HIBYTE( c);
                   ev-> key. mod |=
                      (( c & 1) ? kmShift : 0) |
                      (( c & 2) ? kmCtrl  : 0) |
                      (( c & 4) ? kmAlt   : 0);
                }
             } else {
                 if ( ev-> key. key == kbBackTab) {
                    mp1  = VK_TAB;
                    ev-> key. mod |= kmShift;
                 } else {
                    mp1 = ctx_remap( ev-> key. key, ctx_kb2VK, true);
                    if ( mp1 == 0) return false;
                 }
                 scan = MapVirtualKey( mp1, 0);
             }

             // constructing msg
             msg = ( ev-> key. mod & kmAlt) ? (
               ( ev-> cmd == cmKeyUp) ? WM_SYSKEYUP : WM_SYSKEYDOWN
             ) : (
               ( ev-> cmd == cmKeyUp) ? WM_KEYUP : WM_KEYDOWN
             );

             // constructing mp2
             mp2 = MAKELPARAM( ev-> key. repeat,  scan);
             switch ( msg) {
                  case WM_KEYDOWN:
                     mp2 |= 0x00000000;
                     break;
                  case WM_SYSKEYDOWN:
                     mp2 |= 0x20000000;
                     break;
                  case WM_KEYUP:
                     mp2 |= 0xC0000000;
                     break;
                  case WM_SYSKEYUP:
                     mp2 |= 0xE0000000;
                     break;
             }

             if ( specF10)
                msg = ( ev-> cmd == cmKeyUp) ? WM_SYSKEYUP : WM_SYSKEYDOWN;

             if ( post) {
                KeyPacket * kp;
                kp = ( KeyPacket *) malloc( sizeof( KeyPacket));
                if ( kp) {
                   kp-> mp1 = mp1;
                   kp-> mp2 = mp2;
                   kp-> msg = msg;
                   kp-> wnd = HANDLE;
                   kp-> mod = ev-> key. mod;
                   PostMessage( 0, WM_KEYPACKET, 0, ( LPARAM) kp);
                }
             } else {
                BYTE * mod = mod_select( ev-> key. mod);
                SendMessage( HANDLE, msg, mp1, mp2);
                mod_free( mod);
             }
          }
          break;
   default:
          return false;
   }
   return true;
}

/* convert explorer-string format (asciiz,asciiz,...,0) into
   backslash-escaped string. spaces and backslashes are escaped */
static char *
duplicate_zz_string( const char * c)
{
   int sz = 1;
   char * d = ( char *) c, * ret;
   while ( d[0] || d[1]) {
      if ( *d == ' ' || *d == '\\') sz++;
      sz++;
      d++;
   }
   if ( !( ret = d = malloc( sz))) return nil;
   while ( c[0] || c[1]) {
      if ( !*c) {
	 *d++ = ' ';
	 c++;
	 continue;
      }
      if ( *c == ' ' || *c == '\\') 
	 *d++ = '\\';
      *d++ = *c++;
   }
   *d++ = 0;
   return ret;
}

/* performs non-standard windows open file function */
static char *
win32_openfile( const char * params)
{
   static OPENFILENAME o;
   static Bool initialized = false;
   static char filter[2048] = "";
   static char defext[32] = "";
   static char directory[2048] = "";
   static char title[256] = "";
   static char filters[20480];

   if ( !initialized) {
      memset( &o, 0, sizeof(o));
      o. lStructSize = sizeof(o);
      o. nMaxFile = 20479;
      o. nMaxCustFilter = 2047;
      initialized = true;
   }

   if ( strncmp( params, "filters=", 8) == 0) {
      params += 8;
      if ( strcmp( params, "NULL") == 0) {
         o. lpstrFilter = NULL;
      } else {
	 /* copy \0\0-terminated string */
	 char * fb = filters;
	 int fbsz = 20477;
	 while (( params[0] || params[1]) && fbsz--) 
	    *fb++ = *params++;
	 *fb = *params;
         o. lpstrFilter = filters;
      }
   } else if ( strncmp( params, "directory", 9) == 0) {
      params += 9;
      if ( *params == '=') {
	 if ( strcmp( params, "NULL") == 0) {
	    o. lpstrInitialDir = NULL;
	 } else {
	    strncpy( directory, params, 2047);
	    o. lpstrInitialDir = directory;
	 }
      } else {
	 return duplicate_string( directory);
      }
   } else if ( strncmp( params, "title=", 6) == 0) {
      params += 6;
      if ( strcmp( params, "NULL") == 0) {
         o. lpstrTitle = NULL;
      } else {
	 strncpy( title, params, 255);
         o. lpstrTitle = title;
      }
   } else if ( strncmp( params, "defext=", 7) == 0) {
      params += 7;
      if ( strcmp( params, "NULL") == 0) {
         o. lpstrDefExt = NULL;
      } else {
	 strncpy( defext, params, 31);
         o. lpstrDefExt = defext;
      }
   } else if ( strncmp( params, "filter=", 7) == 0) {
      params += 7;
      if ( strcmp( params, "NULL") == 0) {
         o. lpstrCustomFilter = NULL;
      } else if ( strcmp( params, "DEFAULT") == 0) {
         o. lpstrCustomFilter = filter;
      } else {
	 strncpy( filter, params, 2047);
         o. lpstrCustomFilter = filter;
      }
   } else if ( strncmp( params, "filterindex", 11) == 0) {
      params += 11;
      if ( *params == '=') {
	 int fi = 0;
	 sscanf( params + 1, "%d", &fi);
	 o. nFilterIndex = fi;
      } else {
	 char buf[25];
	 sprintf( buf, "%d", (int) o. nFilterIndex);
	 return duplicate_string( buf);
      }
   } else if ( strncmp( params, "flags=", 6) == 0) {
      params += 6;
      o. Flags = 0;
      while ( *params) {
         char * cp = ( char *) params, pp;
	 while ( *cp && *cp != ',') *cp++;
	 pp = *cp;
	 *cp = 0;
	 if ( stricmp( params, "READONLY") == 0) o. Flags |=              OFN_READONLY; else
	 if ( stricmp( params, "OVERWRITEPROMPT") == 0) o. Flags |=       OFN_OVERWRITEPROMPT; else
	 if ( stricmp( params, "HIDEREADONLY") == 0) o. Flags |=          OFN_HIDEREADONLY; else
	 if ( stricmp( params, "NOCHANGEDIR") == 0) o. Flags |=           OFN_NOCHANGEDIR; else
	 if ( stricmp( params, "SHOWHELP") == 0) o. Flags |=              OFN_SHOWHELP; else
	 if ( stricmp( params, "NOVALIDATE") == 0) o. Flags |=            OFN_NOVALIDATE; else
	 if ( stricmp( params, "ALLOWMULTISELECT") == 0) o. Flags |=      OFN_ALLOWMULTISELECT; else
	 if ( stricmp( params, "EXTENSIONDIFFERENT") == 0) o. Flags |=    OFN_EXTENSIONDIFFERENT; else
	 if ( stricmp( params, "PATHMUSTEXIST") == 0) o. Flags |=         OFN_PATHMUSTEXIST; else
	 if ( stricmp( params, "FILEMUSTEXIST") == 0) o. Flags |=         OFN_FILEMUSTEXIST; else
	 if ( stricmp( params, "CREATEPROMPT") == 0) o. Flags |=          OFN_CREATEPROMPT; else
	 if ( stricmp( params, "SHAREAWARE") == 0) o. Flags |=            OFN_SHAREAWARE; else
	 if ( stricmp( params, "NOREADONLYRETURN") == 0) o. Flags |=      OFN_NOREADONLYRETURN; else
	 if ( stricmp( params, "NOTESTFILECREATE") == 0) o. Flags |=      OFN_NOTESTFILECREATE; else
	 if ( stricmp( params, "NONETWORKBUTTON") == 0) o. Flags |=       OFN_NONETWORKBUTTON; else
	 if ( stricmp( params, "NOLONGNAMES") == 0) o. Flags |=           OFN_NOLONGNAMES; else
#ifndef OFN_EXPLORER
#define OFN_EXPLORER 0
#define OFN_NODEREFERENCELINKS 0
#define OFN_LONGNAMES 0
#endif	    
	 if ( stricmp( params, "EXPLORER") == 0) o. Flags |=              OFN_EXPLORER; else
	 if ( stricmp( params, "NODEREFERENCELINKS") == 0) o. Flags |=    OFN_NODEREFERENCELINKS; else
	 if ( stricmp( params, "LONGNAMES") == 0) o. Flags |=             OFN_LONGNAMES; else
         warn("win32.OpenFile: Unknown constant OFN_%s", params);
         params = cp + 1;
         if ( !pp) break;
      }
   } else if (( strncmp( params, "open", 4) == 0) || 
              ( strncmp( params, "save", 4) == 0)) {
      Bool ret;
      char filename[20480] = "";

      guts. focSysDialog = 1;
      o. lpstrFile = filename;
      ret = (strncmp( params, "open", 4) == 0) ? 
         GetOpenFileName( &o) :
         GetSaveFileName( &o);
      if ( ret == 0) {
         DWORD error;
         error = CommDlgExtendedError();
         if ( error != 0) {
            warn("win32.OpenFile: Get%sFileName error %d at line %d at %s\n", 
        	 (strncmp( params, "open", 4) == 0) ? "Open" : "Save",
                 error,
        	 __LINE__, __FILE__
            );
         }
      }
      guts. focSysDialog = 0;
      if ( !ret) return 0;
      strncpy( directory, o. lpstrFile, o. nFileOffset);
      if ( o. Flags & OFN_ALLOWMULTISELECT) {
         if ( o. Flags & OFN_EXPLORER)
	    return duplicate_zz_string( o. lpstrFile + o. nFileOffset );
	 else
	    return duplicate_string( o. lpstrFile + o. nFileOffset );
      }
      return duplicate_string( o. lpstrFile);
   } else {
      warn("win32.OpenFile: Unknown function %s", params);
   }
   
   return 0;
}

static BOOL CALLBACK
find_console( HWND w, LPARAM ptr)
{
   DWORD pid;
   char buf[256];
   DWORD tid = GetWindowThreadProcessId( w, &pid);
   if ( tid != guts. mainThreadId) return TRUE;
   if ( GetClassName( w, buf, 255) == 0) return TRUE;
   if ( strcmp( buf, "ConsoleWindowClass") != 0) return TRUE;
   *(HWND*)(ptr) = w;
   return FALSE;
}

char *
apc_system_action( const char * params)
{
   switch ( *params) {
   case 'b':
#define STR "browser"
      if ( strncmp( params, STR, strlen( STR)) == 0) {
#undef STR
         HKEY k;
         DWORD valSize = MAX_PATH, valType = REG_SZ, res;
         char buf[ MAX_PATH] = "";
         RegOpenKeyEx( HKEY_CLASSES_ROOT, "http\\shell\\open\\command", 0, KEY_READ, &k);
         res = RegQueryValueEx( k, nil, nil, &valType, ( LPBYTE)buf, &valSize);
         RegCloseKey( k);
         if ( res != ERROR_SUCCESS) return nil;
         return duplicate_string( buf);
      }
      break;
   case 'w':
      if ( strcmp( params, "wait.before.quit") == 0) {
         waitBeforeQuit = 1;
      } else if ( strncmp( params, "win32.DrawFocusRect ", 20) == 0) {
         RECT r;
         Handle win;
         Handle self;
         int i = sscanf( params + 20, "%lu %ld %ld %ld %ld", &win, &r.left, &r.bottom, &r.right, &r.top);

         if ( i != 5 || !( self = hwnd_to_view(( HWND) win))) {
            warn( "Bad parameters to sysaction win32.DrawFocusRect");
            return 0;
         }
         if ( !opt_InPaint) return 0;
         r. bottom = sys lastSize. y - r. bottom;
         r. top    = sys lastSize. y - r. top;
         DrawFocusRect( sys ps, &r);
      } else if (strcmp( params, "win.printfocusedwindow") == 0) {
         HWND h = GetFocus();
         if ( h) {
            char b[ 256];
            DOLBUG( "%08x: %s", h, GetWindowText( h, b, 255) ? b : "NULL");
         } else {
            DOLBUG( "? No foc");
         }
      } else if (strncmp( params, "win32.WNetGetUser", 17) == 0) {
         char connection[ 1024];
         char user[ 1024], *c;
         DWORD len = 1024;
         int i = sscanf( params + 18, "%s", connection);
         if ( i != 1) {
            warn( "Bad parameters to sysaction win32.WNetGetUser");
            return 0;
         }
         if ( WNetGetUser( connection, user, &len) != NO_ERROR)
            return 0;
         c = ( char *) malloc( strlen( user) + 1);
         return c ? strcpy( c , user) : 0;
      } else if ( strncmp( params, "win32.SetVersion", 16) == 0) {
         const char * ver = params + 17;
         while ( *ver && ( *ver == ' '  || *ver == '\t')) ver++;

         if ( stricmp( ver, "NT") == 0) {
            guts. version = 0;
            guts. is98 = 0;
         } else if (( stricmp( ver, "95") == 0) || ( stricmp( ver, "mustdie") == 0)) {
            guts. version = 0x80000095;
            guts. is98 = 0;
         } else if ( stricmp( ver, "98") == 0) {
            guts. version = 0x80000098;
            guts. is98 = 1;
         } else {
            warn( "Are you nuts? Microsoft itself afraid of %s yet!", ver);
         }
      } else if ( strncmp( params, "win32.ConsoleWindow", 19) == 0) {
         params += 19;

         if ( !guts. console) {
            EnumWindows((WNDENUMPROC) find_console, (LPARAM) &guts. console);
            if ( strcmp( params, " exists") == 0) return 0;
            if ( !guts. console) {
                warn("No associated console window found");
                return 0;
            }
         }

         if ( strcmp( params, " exists") == 0) {
           char * p = ( char *) malloc(12);
           if ( p) sprintf( p, "0x%08lx", ( Handle) guts. console);
           return p;
         } else
         if ( strcmp( params, " hide") == 0)     { ShowWindow( guts. console, SW_HIDE); } else
         if ( strcmp( params, " show") == 0)     { ShowWindow( guts. console, SW_SHOW); } else
         if ( strcmp( params, " minimize") == 0) { ShowWindow( guts. console, SW_MINIMIZE); } else
         if ( strcmp( params, " maximize") == 0) { ShowWindow( guts. console, SW_SHOWMAXIMIZED ); } else
         if ( strcmp( params, " restore") == 0)  { ShowWindow( guts. console, SW_RESTORE); } else
         if ( strcmp( params, " topmost") == 0)  { SetWindowPos( guts. console, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE); } else
         if ( strcmp( params, " notopmost") == 0)  { SetWindowPos( guts. console, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE); } else
         if ( strncmp( params, " text", 5) == 0)  {
            char * p = (char*)params + 5;
            while ( *p == ' ') p++;
            if ( *p) {
               SetWindowText( guts. console, p);
            } else {
               int lc = GetWindowTextLength( guts. console);
               p = (char*)malloc( lc + 2);
               if ( p) GetWindowText( guts. console, p, lc+1);
               return p;
            }
         } else {
            warn( "Bad parameters '%s' to sysaction win32.ConsoleWindow", params);
            return 0;
         }
      } else if ( strncmp( params, "win32.OpenFile.", 15) == 0) {
	 params += 15;
	 return win32_openfile( params);
      } else
         goto DEFAULT;
      break;
   DEFAULT:
   default:
      warn( "Unknown sysaction \"%s\"", params);
   }
   return 0;
}

#ifdef __cplusplus
}
#endif
