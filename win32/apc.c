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
#include "guts.h"
#include "Menu.h"
#include "Image.h"
#include "Window.h"
#include "Application.h"

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
   hwnd_enter_paint( self);
   if ( sys pal = palette_create( self)) {
      SelectPalette( sys ps, sys pal, 0);
      RealizePalette( sys ps);
   }
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
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
   objCheck false;
   if ( !( h = CreateWindowEx( 0, "GenericApp", "", 0, 0, 0, 0, 0,
          nilHandle, nilHandle, guts. instance, nilHandle))) apiErrRet;
   sys handle = h;
   sys parent = sys owner = HWND_DESKTOP;
   SetWindowLong( sys handle, GWL_USERDATA, self);
   PostMessage( sys handle, WM_PRIMA_CREATE, 0, 0);
   sys className = WC_APPLICATION;
   if ( !SetTimer( h, TID_USERMAX, 100, nil)) apiErr;
   GetClientRect( h, &r);
   if ( !( var handle = ( Handle) CreateWindowEx( 0,  "Generic", "", WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
        0, 0, r. right - r. left, r. bottom - r. top, h, nilHandle,
        guts. instance, nil))) apiErrRet;
   SetWindowLong(( HWND) var handle, GWL_USERDATA, self);
   apt_set( aptEnabled);
   sys lastSize = apc_application_get_size( self);
   return true;
}

void
apc_application_close( Handle self)
{
   if ( guts. logger) {
      if ( !PostMessage( guts. logger, WM_CLOSE, 0, 0)) apiErr;
   } else
      PostQuitMessage(0);
}

void
apc_application_destroy( Handle self)
{
   objCheck;
   if ( IsWindow( sys handle))  {
      if ( !KillTimer( sys handle, TID_USERMAX)) apiErr;
      if ( !DestroyWindow( sys handle)) apiErr;
   }
   free( sys timeDefs);
   if ( !loggerDead)
      PostMessage( guts. logger, WM_QUIT, 0, 0);
   PostThreadMessage( guts. mainThreadId, WM_TERMINATE, 0, 0);
   loggerDead = true;
   if ( !guts. logger)
      PostQuitMessage(0);
}

void
apc_application_end_paint( Handle self)
{
   apcErrClear;
   objCheck;
   hwnd_leave_paint( self);
   if ( sys pal) DeleteObject( sys pal);
   dc_free();
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
   sys pal = sys ps = nil;
}

void
apc_application_end_paint_info( Handle self)
{
   apc_application_end_paint( self);
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
   dc  = dc_alloc();
   lpg. palNumEntries = GetSystemPaletteEntries( dc, 0, 256, lpg. palPalEntry);
   lpg. palVersion = 0x300;

   hp  = CreatePalette(( LOGPALETTE*)&lpg);
   dc2 = CreateCompatibleDC( dc);
   hp2 = SelectPalette( dc2, hp, 0);
   RealizePalette( dc2);
   hp3 = SelectPalette( dc, hp, 1);

   bm  = CreateCompatibleBitmap( dc, xLen, yLen);
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

   return true;
}


Handle
hwnd_to_view( HWND win)
{
   Handle h;
   LONG ll;
   if (( !win) || ( !IsWindow( win)))
      return nilHandle;
   if ( GetWindowThreadProcessId( win, nil) != guts. mainThreadId)
      return nilHandle;
   h = GetWindowLong( win, GWL_USERDATA);
   if ( !h) return nilHandle;
   ll = GetWindowLong( win, GWL_WNDPROC);
   if (
       ( ll == ( LONG) generic_view_handler) ||
       ( ll == ( LONG) generic_app_handler) ||
       ( ll == ( LONG) generic_frame_handler)
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
      switch ( si. wProcessorArchitecture) {
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
process_msg( MSG * msg)
{
   switch ( msg-> message)
   {
      case WM_TERMINATE:
      case WM_QUIT:
         return false;
      case WM_BREAKMSGLOOP:
         return true;
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
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
   }
   // absolutely unneeded syscall, we don't use CHAR messages, but -
   // Mustdie 95 and Mustdie 98 switches kbd lamps inside TranslateMessage()
   if ( IS_WIN95) TranslateMessage( msg);
   DispatchMessage( msg);
   kill_zombies();
   return true;
}


void
apc_application_go( Handle self)
{
   MSG msg;
   objCheck;
   while ( GetMessage( &msg, NULL, 0, 0) && process_msg( &msg));
   if ( application) Object_destroy( application);
}


static void
lock( Bool lock)
{
   if ( lock)
   {
      if ( guts. appLock++ == 0) LockWindowUpdate( HWND_DESKTOP);
   }
   else
   {
      if ( --guts. appLock == 0) LockWindowUpdate( nilHandle);
   }
}

void
apc_application_lock( Handle self)
{
  lock( true);
}

void
apc_application_unlock( Handle self)
{
  lock( false);
}

void
apc_application_yield()
{
   MSG msg;
   while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
      if ( !process_msg( &msg))
         PostThreadMessage( guts. mainThreadId, appDead ? WM_QUIT : WM_TERMINATE, 0, 0);
}

Handle
apc_application_get_view_from_point( Handle self, Point point)
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
   return ( Handle) GetWindowLong( p, GWL_USERDATA);
}

// Component
void
apc_component_create( Handle self)
{
   PComponent c = ( PComponent) self;
   PDrawableData d = c-> sysData;

   objCheck;

   if ( d) return;
   d = malloc( sizeof( DrawableData));
   memset( d, 0, sizeof( DrawableData));
   c-> sysData = d;
}

void
apc_component_destroy( Handle self)
{
   PComponent    c = ( PComponent) self;
   PDrawableData d = c-> sysData;
   objCheck;
   var handle = nilHandle;
   if ( d == nil) return;
   free( d);
   c-> sysData = nil;
}

void
apc_component_fullname_changed_notify( Handle self)
{
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
  p-> capture   = apc_widget_is_captured( self);
  for ( i = 0; i <= ciMaxId; i++) p-> colors[ i] = apc_widget_get_color( self, i);
  p-> pos       = apc_widget_get_pos( self);
  p-> size      = apc_widget_get_size( self);
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
  apc_widget_set_pos( self, p-> pos. x, p-> pos. y);
  apc_widget_set_size( self, p-> size. x, p-> size. y);
  var virtualSize = p-> virtSize;
  apc_widget_set_enabled( self, p-> enabled);
  if ( p-> focused) apc_widget_set_focused( self);
  apc_widget_set_visible( self, p-> visible);
  if ( p-> capture) apc_widget_set_capture( self, 1, p-> capture);
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
              ViewProfile * vprf)
{
   HWND ret;
   HWND old        = HANDLE;
   HWND behind     = HWND_TOP;
   HWND ownerView  = ( HWND) (( PWidget) owner)-> handle;
   HWND parentView = (( PDrawableData)(( PComponent) owner)-> sysData)-> parent;
   int  count = 0;
   Bool reset = false;
   Handle * list = nil;
   int oStage = var stage;

   if ( HANDLE)
   {
      int i;
      if (( DHANDLE( owner) == sys owner) && ( clipOwner == is_apt( aptClipOwner)))
      {
         char buf[256];
         behind = GetWindow( HANDLE, GW_HWNDPREV);
         if ( behind == nilHandle) behind = HWND_TOP;
         GetWindowText( behind, buf, 256);
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
         get_view_ex( list[ i], ( ViewProfile*) dsys( list[ i]) recreateData = malloc( sizeof( ViewProfile)));
   }

   switch (( int) className)
   {
     case WC_FRAME:
       {
          HWND frame;
          RECT r;
          int  rcp[4] = {0,0,0,0};
          if ( !clipOwner) parentView = HWND_DESKTOP;
          if ( !taskListed && ( parentView == HWND_DESKTOP || parentView == nilHandle))
              parentView = DHANDLE( application);
          if ( !usePos)  rcp[0] = rcp[1] = CW_USEDEFAULT;
          if ( !useSize) rcp[2] = rcp[3] = CW_USEDEFAULT;
          if ( !( frame = CreateWindowEx( exstyle, "GenericFrame", "",
                style | WS_CLIPCHILDREN,
                rcp[0], rcp[1], rcp[2], rcp[3],
                parentView, nilHandle, guts. instance, nil)))
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
          if ( !( ret = CreateWindowEx( 0,  "Generic", "",
                WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                0, 0, r. right - r. left, r. bottom - r. top, frame, nilHandle,
                guts. instance, nil)))
             apiErr;
          sys lastSize. x = r. right  - r. left;
          sys lastSize. y = r. bottom - r. top;
          sys handle = frame;
          SetWindowLong( frame, GWL_USERDATA, ( LONG) self);
       }
       break;
    case WC_CUSTOM:
       if ( !clipOwner || owner == application) {
            style &= ~WS_CHILD;
            style |= WS_POPUP;
            exstyle |= WS_EX_TOOLWINDOW;
       }
       if ( !( ret = CreateWindowEx( exstyle,  "Generic", "",
             style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0,
             parentView, nilHandle, guts. instance, nil)))
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
   SetWindowLong( ret, GWL_USERDATA, ( LONG) self);

   if ( reset)
   {
      int i;
      apc_widget_set_pos( self, vprf-> pos. x, vprf-> pos. y);
      apc_widget_set_size( self, vprf-> size. x, vprf-> size. y);
      for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
      if ( sys className == WC_FRAME)
      {
         SetMenu( HANDLE, GetMenu( old));
         SetMenu( old, NULL);
      }
      if ( !DestroyWindow( old))
         apiErr;
      if ( var postList) list_first_that( var postList, ( PListProc) repost_msgs, ( void*)self);
   }
   PostMessage( ret, WM_PRIMA_CREATE, 0, 0);
   return true;
}

// Window
Bool
apc_window_create( Handle self, Handle owner, Bool syncPaint, Bool clipOwner, int borderIcons,
                   int borderStyle, Bool taskList, int windowState, Bool usePos, Bool useSize)
{
  Bool reset = false;
  ViewProfile vprf;
  int oStage = var stage;
  WindowData ws;
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
  if ( !kind_of( self, CWidget)) apcErrRet( errInvObject);
  apcErrClear;
  if (( var handle != nilHandle) && (
       ( DHANDLE( owner) != sys owner)
    || ( borderStyle != sys s. window. borderStyle)
    || ( borderIcons != sys s. window. borderIcons)
    || ( taskList    != is_apt( aptTaskList))
    || ( clipOwner   != is_apt( aptClipOwner))
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
     reset = true;
  }
  lock( true);

  if ( reset || ( var handle == nilHandle))
     if ( !create_group( self, owner, syncPaint, clipOwner,
           taskList, WC_FRAME, style, exstyle, usePos, useSize, &vprf)) {
        lock( false);
        return false;
     }
  ws. borderStyle = sys s. window. borderStyle = borderStyle;
  ws. borderIcons = sys s. window. borderIcons = borderIcons;
  ws. state       = sys s. window. state       = windowState;
  apt_assign( aptSyncPaint, syncPaint);
  apt_assign( aptTaskList,  taskList);
  if ( reset)
  {
     apc_window_set_window_state( self, windowState);
     set_view_ex( self, &vprf);
     sys s. window = ws;
     if ( !SetWindowPlacement( HANDLE, &wp)) apiErr;
     var stage = oStage;
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
  apc_window_set_caption( self, var text);
  lock( false);
  return apcError == 0;
}

void
apc_window_activate( Handle self)
{
   if ( self) {
      HWND w;
      objCheck;
      w = HANDLE;
      SetForegroundWindow( w); // no reasonable error description here,
      SetActiveWindow( w);     // long live M$DN :E
   }
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
   if ( icon == nilHandle) return p != nilHandle;

   save = sys pointer;
   sys pointer = p;
   ret = apc_pointer_get_bitmap( self, icon);
   sys pointer = save;
   return ret;
}


static int;
map_tildas( char * buf, int len)
{
   int j, newLen;
   if ( len < 0) len = strlen( buf);
   newLen = len;
   for ( j = 0; j <= len; j++) {
      if ( buf[ j] == '~') {
          if ( buf[ j + 1] == '~')
             j++;
          else if ( buf[ j + 1])
             buf[ j] = '&';
          continue;
      } else if ( buf[ j] == '&') {
         memmove( &buf[ j + 1], &buf[ j], ++len - j);
         j++;
         newLen++;
         continue;
      }
   }
   return newLen;
}


static HWND
add_item( Bool menuType, Handle menu, PMenuItemReg i)
{
    MENUITEMINFO menuItem;
    HWND m;
    PMenuWndData mwd;
    PMenuItemReg first;

    if ( i == nil) return nilHandle;

    if ( menuType)
       m = CreateMenu();
    else
       m = CreatePopupMenu();

    if ( !m) {
       apiErr;
       return nilHandle;
    }
    mwd = malloc( sizeof( MenuWndData));
    mwd-> menu = menu;
    first      = i;
    hash_store( menuMan, &m, sizeof( void*), mwd);

    while ( i != nil)
    {
       memset( &menuItem, 0, sizeof( menuItem));
       menuItem. cbSize   = sizeof( menuItem);
       menuItem. fMask    = MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
       menuItem. fType    = 0;
       menuItem. fType   |= ( i-> divider    ) ? MFT_SEPARATOR    : 0;
       menuItem. fType   |= ( i-> bitmap     ) ? MFT_BITMAP       : 0;
       menuItem. fType   |= ( i-> text       ) ? MFT_STRING       : 0;
       menuItem. fType   |= ( i-> rightAdjust) ? MFT_RIGHTJUSTIFY : 0;
       menuItem. fState   = 0;
       menuItem. fState  |= ( i-> checked )    ? MFS_CHECKED      : 0;
       menuItem. fState  |= ( i-> disabled)    ? MFS_GRAYED       : 0;
       menuItem. wID      = i-> id + MENU_ID_AUTOSTART;
       menuItem. hSubMenu = add_item( menuType, menu, i-> down);
       if (!( i-> divider && i-> rightAdjust)) {
          if ( i-> text) {
             char buf [ 1024];
             strcpy( buf, i-> text);
             if ( i-> accel) snprintf( buf, 1024, "%s\t%s", buf, i-> accel);
             map_tildas( buf, strlen( i-> text));
             menuItem. dwTypeData = ( LPTSTR) buf;
          } else if ( i-> bitmap)
             menuItem. dwTypeData = image_make_bitmap_handle( i-> bitmap, nil);
          InsertMenuItem( m, -1, true, &menuItem);
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

void
apc_window_set_caption( Handle self, const char * caption)
{
   objCheck;
   if ( !SetWindowText( HANDLE, caption)) apiErr;
}

void
apc_window_set_client_pos( Handle self, int x, int y)
{
   Point delta = get_window_borders( sys s. window. borderStyle);
   RECT  r;
   Handle parent = var self-> get_parent( self);
   Point sz = CWidget( parent)-> get_size( parent);


   objCheck;
   if ( var stage == csConstructing && apc_window_get_window_state( self) != wsNormal) {
      WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
      if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
      w. rcNormalPosition. top    += sz. y - y + delta. y - w. rcNormalPosition. bottom;
      w. rcNormalPosition. bottom  = sz. y - y + delta. y;
      w. rcNormalPosition. right  += x - delta. x - w. rcNormalPosition. left;
      w. rcNormalPosition. left    = x - delta. x;
      w. flags   = 0;
      w. showCmd  = 0;
      if ( !SetWindowPlacement( HANDLE, &w)) apiErr;
   } else {
      if ( !GetWindowRect( HANDLE, &r)) apiErr;
      x -= delta. x;
      y  = sz. y - ( r. bottom - r. top) - y + delta. y;
      SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
   }
}

void
apc_window_set_client_size( Handle self, int x, int y)
{
   RECT r, c, c2;
   HWND h;
   int  ws = apc_window_get_window_state( self);


   objCheck;
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
      w. showCmd = 0;
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
         SWP_NOZORDER | SWP_NOACTIVATE);
      sys sizeLockLevel--;
   }
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
   Point size;
   objCheck false;
   apcErrClear;
   size = var self-> get_size( self);
   SetMenu( HANDLE, menu ? ( HMENU) (( PComponent) menu)-> handle : nilHandle);
   DrawMenuBar( HANDLE);
   var self-> set_size( self, size.x, size. y);
   return apcError == 0;
}


void
apc_window_set_icon( Handle self, Handle icon)
{
   HICON i;
   objCheck;
   i = icon ? image_make_icon_handle( icon, guts. iconSizeLarge, nil, false) : nil;
   i = ( HICON) SendMessage( HANDLE, WM_SETICON, ICON_BIG, ( LPARAM) i);
   if ( i) DestroyIcon( i);
   if ( icon && guts. logger != NULL &&
        ((guts. loggerIcon == NULL) ||
         (guts. loggerIconSupplier == self))
      ) {
      guts. loggerIcon = image_make_icon_handle( icon, guts. iconSizeLarge, nil, false);
      guts. loggerIconSupplier = self;
      i = ( HICON) SendMessage( guts. logger, WM_SETICON, ICON_BIG, ( LPARAM) guts. loggerIcon);
      if ( i) DestroyIcon( i);
   }
}

void
apc_window_set_window_state( Handle self, int state)
{
   int  fl = -1;
   objCheck;
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
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insertBefore)
{
   HWND wnd;
   HWND active = GetActiveWindow();

   objCheck false;
   wnd = HANDLE;
   if ( sys className != WC_FRAME) { apcErr( errInvParams); return false; }

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
         if ( msg. message == WM_BREAKMSGLOOP) {
            if ( msg. lParam == self)
               break;
            else
               continue;
         }
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

void
apc_window_end_modal( Handle self)
{
   HWND wnd;
   objCheck;
   wnd = HANDLE;
   if ( PWindow( self)-> modal == mtExclusive) {
      if ( self == (( PApplication) application)-> topExclModal)
         PostThreadMessage( guts. mainThreadId, WM_BREAKMSGLOOP, 0, self);
   }
   guts. focSysDisabled = 1;
   CWindow( self)-> exec_leave_proc( self);
   WinHideWindow( wnd);
   objCheck;
   apc_widget_set_enabled( self, 0);
   objCheck;
   if ( application) {
      Handle who = Application_popup_modal( application);
      if ( !who && var owner)
         CWidget( var owner)-> set_selected( var owner, 1);
         // SetFocus( DHANDLE( var owner));
   }
   guts. focSysDisabled = 0;
}

// View management
Point
apc_widget_client_to_screen( Handle self, Point p)
{
   POINT pt;
   Point sz = ((( PWidget) self)-> self)-> get_size( self);
   Point appSz = apc_application_get_size( application);
   objCheck sz;
   pt. x = p. x;
   pt. y = sz. y - p. y;
   MapWindowPoints(( HWND) var handle, nilHandle, &pt, 1);
   p. x = pt. x;
   p. y = appSz. y - pt. y;
   return p;
}

Bool
apc_widget_create( Handle self, Handle owner, Bool syncPaint, Bool clipOwner, Bool transparent)
{
   Bool reset = false;
   ViewProfile vprf;
   int oStage = var stage;
   int exstyle;

   objCheck false;
   dobjCheck( owner) false;

   if ( !kind_of( self, CWidget)) apcErr( errInvObject);
   apcErrClear;

   exstyle = 0;
   if (( var handle != nilHandle) &&
         (( DHANDLE( owner) != sys owner)                 ||
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
      create_group( self, owner, syncPaint, clipOwner, 0, WC_CUSTOM, WS_CHILD, exstyle, 1, 1, &vprf);
   if ( reset)
   {
      set_view_ex( self, &vprf);
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


      if ( !( dc = CreateCompatibleDC( sys ps))) apiErr;

      if ( sys pal = palette_create( self)) {
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
         apc_gp_set_transform( self, -r. left, -r. top);
         sys transform2. x = r. left;
         sys transform2. y = r. top;
      } else
         apiErr;
   } else {
      if ( sys pal = palette_create( self)) {
         sys stockPalette = SelectPalette( sys ps, sys pal, 1);
         RealizePalette( sys ps);
      }
   }
   hwnd_enter_paint( self);

   if ( useRPDraw) {
      HDC dc;
      Handle owner = var owner;
      Point tr = dsys(owner)transform2;
      Point ed = apc_widget_get_pos( self);

      CWidget( owner)-> begin_paint( owner);
      dc = dsys( owner) ps;
      dsys( owner) ps = sys ps;
      dsys(owner) transform2. x += ed. x;
      dsys(owner) transform2. y += ed. y;
      apc_gp_set_transform( owner, 0, 0);

      CWidget( owner)-> notify( owner, "sH", "Paint", owner);
      dsys(owner)transform2 = tr;
      apc_gp_set_transform( owner, 0, 0);
      dsys( owner) ps = dc;
      CWidget( owner)-> end_paint( owner);
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
   sys pal = palette_create( self);
   rgn = CreateRectRgn( 0, 0, 0, 0);
   SelectClipRgn( sys ps, rgn);
   DeleteObject( rgn);
   return true;
}


void
apc_widget_destroy( Handle self)
{
   objCheck;
   if ( sys pointer2) {
      if ( sys pointer2 == sys pointer) SetCursor( NULL); // un-use resource first
      if ( !DestroyCursor( sys pointer2)) apiErr;
   }
   if ( sys recreateData) free( sys recreateData);
   free( sys timeDefs);
   if ( var handle == nilHandle) return;

   if ( sys className == WC_FRAME)
      guts. topWindows--;

   if ( !DestroyWindow( HANDLE)) apiErr;
   if ( self == guts. loggerIconSupplier) {
      if ( guts. logger) {
         HICON i = ( HICON) SendMessage( guts. logger, WM_SETICON, ICON_BIG, ( LPARAM) NULL);
         DestroyIcon( i);
      }
      guts. loggerIconSupplier = NULL;
      guts. loggerIcon = NULL;
   }
}

PFont
apc_widget_default_font( PFont copyTo)
{
   *copyTo = guts. windowFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}

void
apc_widget_end_paint( Handle self)
{
   objCheck;
   if ( is_opt( optBuffered)) {
      if ( sys bm != nilHandle) {
         if ( !SetViewportOrgEx( sys ps, 0, 0, nil)) apiErr;
         if ( !BitBlt( sys ps2, sys transform2. x, sys transform2. y, var w, var h, sys ps, 0, 0, SRCCOPY)) apiErr;
         if ( sys stockBM)
            SelectObject( sys ps, sys stockBM);
         DeleteObject( sys bm);
      }
      if ( sys pal) {
         SelectPalette( sys ps2, sys pal2, 1);
         SelectPalette( sys ps, sys stockPalette, 1);
         DeleteObject( sys pal);
         sys pal = sys pal2 = nil;
      }

      DeleteDC( sys ps);
      sys ps = sys ps2;
      sys bm = sys ps2 = sys stockBM = nil;
   }

   hwnd_leave_paint( self);

   if ( sys pal)
      DeleteObject( sys pal);

   if ( sys ps != nilHandle)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
      } else if ( is_apt( aptWinPS))
         if ( !ReleaseDC(( HWND) var handle, sys ps)) apiErr;
   }
   sys ps = sys pal = sys pal2 = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptWM_PAINT);
   apt_clear( aptCompatiblePS);
}

void
apc_widget_end_paint_info( Handle self)
{
   objCheck;
   hwnd_leave_paint( self);
   sys pal = nil;
   if ( !ReleaseDC(( HWND) var handle, sys ps)) apiErr;
   sys ps = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   objCheck false;
   return is_apt( aptClipOwner);
}

Rect
apc_widget_get_clip_rect( Handle self)
{
   Rect r = {0,0,0,0};
   RECT * c;
   objCheck r;
   c = sys pClipRect;
   if ( c) {
      int ly    = sys lastSize. y;
      r. left   = c-> left;
      r. right  = c-> right;
      r. top    = ly - c-> top;
      r. bottom = ly - c-> bottom;
   } else
      r. left = r. right = r. top = r. bottom = 0;
   return r;
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

   dc = dc_compat_alloc(0);
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


void
apc_widget_invalidate_rect( Handle self, Rect * rect)
{
   PRECT pRect = rect ? map_Rect( self, rect) : nil;
   objCheck;
   if ( !InvalidateRect (( HWND) var handle, pRect, false)) apiErr;
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
   objCheck;
   process_transparents( self);
}

Point
apc_widget_screen_to_client( Handle self, Point p)
{
   POINT pt = {0,0};
   Point sz = ((( PWidget) self)-> self)-> get_size( self);
   Point appSz = apc_application_get_size( application);
   objCheck p;
   pt. x = p. x;
   pt. y = appSz. y - p. y;
   MapWindowPoints( nilHandle, ( HWND) var handle, &pt, 1);
   p. x = pt. x;
   p. y = sz. y - pt. y;
   return p;
}

void
apc_widget_scroll( Handle self, int horiz, int vert, Rect * r, Bool scrollChildren)
{
   PRECT pRect = r ? map_Rect( self, r) : nil;
   objCheck;
   HideCaret(( HWND) var handle);
   if ( !ScrollWindowEx(( HWND) var handle,
      horiz, -vert, pRect, sys pClipRect, NULL, NULL,
      SW_INVALIDATE | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
   )) apiErr;
   objCheck;
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
   ShowCaret(( HWND) var handle);
}

void
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
   objCheck;
   if ( capture) {
      SetCapture(( HWND) var handle);
      if ( confineTo) {
         RECT r;
         GetWindowRect(( HWND) PComponent( confineTo)-> handle, &r);
         if ( !ClipCursor( &r)) apiErr;
      }
   } else {
      if ( !ReleaseCapture()) apiErr;
      if ( !ClipCursor( NULL)) apiErr;
   }
}

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

void
apc_widget_set_clip_rect( Handle self, Rect c)
{
   int ly;
   RECT * cr;

   objCheck;

   // inclusive-inclusive
   ly = sys lastSize. y;
   cr = &sys clipRect;

   check_swap( c. bottom, c. top);
   check_swap( c. left, c. right);
   cr-> left   = c. left;
   cr-> right  = c. right;
   cr-> top    = ly - c. top;
   cr-> bottom = ly - c. bottom;

   sys pClipRect = ( c. top == 0 && c. bottom == 0 && c. left == 0 && c. right == 0) ?
      nil : cr;
}

void
apc_widget_set_color( Handle self, Color color, int index)
{
   Event ev = {cmColorChanged};
   objCheck;
   sys viewColors[ index] = color;

   ev. gen. source = self;
   ev. gen. i      = index;
   var self-> message( self, &ev);
}

void
apc_widget_set_enabled( Handle self, Bool enable)
{
   objCheck;
   apt_assign( aptEnabled, enable);
   if (( sys className == WC_FRAME) || ( var owner == application))
      EnableWindow( HANDLE, enable);
   else
      SendMessage( HANDLE, WM_ENABLE, ( WPARAM) enable, 0);
}

void
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   objCheck;
   apt_assign( aptFirstClick, firstClick);
}

void
apc_widget_set_focused( Handle self)
{
   if ( self && ( self != Application_map_focus( application, self)))
      return;
   guts. focSysGranted++;
   SetFocus( self ? (( HWND) var handle) : nil);
   guts. focSysGranted--;
}

void
apc_widget_set_font( Handle self, PFont font)
{
   Event ev = {cmFontChanged};
   objCheck;
   ev. gen. source = self;
   var self-> message( self, &ev);
}

void
apc_widget_set_palette( Handle self)
{
   objCheck;
   if ( sys p256) {
      free( sys p256);
      sys p256 = nil;
   }
   if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
      palette_change( self);
}

void
apc_widget_set_pos( Handle self, int x, int y)
{
   Handle parent;
   Point sz;
   RECT r;

   objCheck;

   parent = is_apt( aptClipOwner) ? var owner : application;
   sz = ((( PWidget) parent)-> self)-> get_size( parent);

   if ( sys className == WC_FRAME) {
      HWND h = HANDLE;
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
         if ( !GetWindowPlacement( h, &w)) apiErr;
         w. rcNormalPosition. top    += sz. y - y - w. rcNormalPosition. bottom;
         w. rcNormalPosition. bottom  = sz. y - y;
         w. rcNormalPosition. right  += x - w. rcNormalPosition. left;
         w. rcNormalPosition. left    = x;
         w. flags = w. showCmd = 0;
         if ( !SetWindowPlacement( h, &w)) apiErr;
         return;
      }
   }
   if ( !GetWindowRect( HANDLE, &r)) apiErr;
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
   y = sz. y - y - r. bottom + r. top;
   if ( !SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
}

void
apc_widget_set_size( Handle self, int width, int height)
{
   RECT r;
   HWND h;
   objCheck;
   h = HANDLE;
   if ( sys className == WC_FRAME) {
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
         if ( !GetWindowPlacement( h, &w)) apiErr;
         if ( width  < 0) width = 0;
         if ( height < 0) height = 0;
         w. rcNormalPosition. top    = w. rcNormalPosition. bottom - height;
         w. rcNormalPosition. right  = width + w. rcNormalPosition. left;
         w. flags = w. showCmd = 0;
         if ( !SetWindowPlacement( h, &w)) apiErr;
         return;
      }
   }
   if ( !GetWindowRect( h, &r)) apiErr;
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
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
      SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
   if ( sys className != WC_FRAME) sys sizeLockLevel--;
}



void
apc_widget_set_shape( Handle self, Handle mask)
{
   HRGN rgn = nilHandle;
   objCheck;

   rgn = region_create( mask);
   if ( !rgn) {
      SetWindowRgn( HANDLE, nil, true);
      return;
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
      apiErr;
}


void
apc_widget_set_tab_order( Handle self, int tabOrder)
{
}

void
apc_widget_set_visible( Handle self, Bool show)
{
   objCheck;
   if ( !SetWindowPos( HANDLE, nilHandle, 0, 0, 0, 0,
        ( show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
   objCheck;
   if ( !is_apt( aptClipOwner)) {
      InvalidateRect(( HWND) var handle, nil, false);
      objCheck;
      process_transparents( self);
   }
}

void
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
   HWND opt = ( top) ? HWND_TOP : HWND_BOTTOM;
   objCheck;
   if ( behind != nilHandle) {
      dobjCheck( behind);
      opt = DHANDLE( behind);
   }
   if ( !SetWindowPos( HANDLE, opt, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)) apiErr;
   // unexpected stupig Windows bug'o'feature - when only window is available to a thread,
   // bring_to_front does not do anything! - so fixing
  // if (( sys className == WC_FRAME) && ( opt == HWND_TOP) && ( guts. topWindows < 2))
  //    apc_window_activate( self);
}

void
apc_widget_update( Handle self)
{
   objCheck;
   if ( !UpdateWindow(( HWND) var handle)) apiErr;
}

void
apc_widget_validate_rect( Handle self, Rect rect)
{
   objCheck;
   if ( !ValidateRect (( HWND) var handle, map_Rect( self, &rect))) apiErr;
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
   apc_menu_destroy( self);
   var handle  = ( Handle) add_item( true, self, (( PMenu) self)-> tree);
   return var handle != nilHandle;
}

static Bool clear_menus( PMenuWndData item, int keyLen, void * key, void * params)
{
   if ( item-> menu == ( Handle) params)
      hash_delete( menuMan, key, keyLen, true);
   return false;
}

void
apc_menu_destroy( Handle self)
{
   if ( var handle) {
      objCheck;
      if ( IsMenu(( HMENU) var handle) && !DestroyMenu(( HMENU) var handle)) apiErr;
      hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
   }
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

void
apc_menu_set_color( Handle self, Color color, int index)
{
}

void
apc_menu_set_font( Handle self, PFont font)
{
}

void
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
   PWindow owner;
   Point size;
   Bool resize;
   objCheck;
   dobjCheck( var owner);
   if ( resize = kind_of( var owner, CWindow) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self)) {
      owner = ( PWindow) var owner;
      size = owner-> self-> get_size( var owner);
   }

   // since GetMenuItemInfo does not work in NT, do not free menuMan entries here,
   // they'll be freed in apc_menu_destroy anyway.

   DeleteMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   DrawMenuBar( DHANDLE( var owner));
   if ( resize)
      owner-> self-> set_size( var owner, size.x, size. y);
}

void
apc_menu_item_set_accel( Handle self, PMenuItemReg m, const char * accel)
{
   MENUITEMINFO mii = {sizeof( MENUITEMINFO)};
   char buf [ 1024];
   UINT flags;

   if ( !var handle) return;
   objCheck;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) {
      apiErr;
      return;
   }

   if ( flags & MF_BITMAP)
      flags = ( flags & ~MF_BITMAP) | MF_STRING;

   snprintf( buf, 1024, "%s\t%s", m-> text, accel);
   map_tildas( buf, strlen( m-> text));

   if ( !ModifyMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                    m-> id + MENU_ID_AUTOSTART, buf))
      apiErr;
}

void
apc_menu_item_set_check( Handle self, PMenuItemReg m, Bool check)
{
   if ( !var handle) return;
   objCheck;
   CheckMenuItem(( HMENU) var handle,
      m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( check ? MF_CHECKED : MF_UNCHECKED));
}

void
apc_menu_item_set_enabled( Handle self, PMenuItemReg m, Bool enabled)
{
   if ( !var handle) return;
   objCheck;
   EnableMenuItem(( HMENU) var handle,
      m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( enabled ? MF_ENABLED : MF_GRAYED));
}

void
apc_menu_item_set_key( Handle self, PMenuItemReg m, int key)
{
}

void
apc_menu_item_set_text( Handle self, PMenuItemReg m, const char * text)
{
   MENUITEMINFO mii = {sizeof( MENUITEMINFO)};
   char buf [ 1024];
   UINT flags;

   if ( !var handle) return;
   objCheck;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) {
      apiErr;
      return;
   }
   if ( flags & MF_BITMAP)
      flags = ( flags & ~MF_BITMAP) | MF_STRING;

   if ( m-> accel)
      snprintf( buf, 1024, "%s\t%s", text, m-> accel);
   else
      strncpy( buf, text, 1024);
   map_tildas( buf, strlen( text));

   ModifyMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                    m-> id + MENU_ID_AUTOSTART, buf);
}

void
apc_menu_item_set_image( Handle self, PMenuItemReg m, Handle image)
{
   MENUITEMINFO mii = {sizeof( MENUITEMINFO)};
   UINT flags;

   if ( !var handle) return;
   objCheck;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) {
      apiErr;
      return;
   }
   flags |= MF_BITMAP;

   ModifyMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                    m-> id + MENU_ID_AUTOSTART, image_make_bitmap_handle( image, nil));
}


ApiHandle
apc_menu_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) var handle;
}

void
apc_menu_update( Handle self, PMenuItemReg oldBranch, PMenuItemReg newBranch)
{
   objCheck;
   dobjCheck( var owner);

   if ( kind_of( var owner, CWindow) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self)) {
      HMENU h = GetMenu( DHANDLE( var owner));
      PWindow owner = ( PWindow) var owner;
      Point size = owner-> self-> get_size( var owner);
      if ( h) DestroyMenu( h);
      hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
      var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
      SetMenu( DHANDLE( var owner), self ? ( HMENU) var handle : nilHandle);
      DrawMenuBar( DHANDLE( var owner));
      owner-> self-> set_size( var owner, size.x, size. y);
   } else {
      if ( var handle)
         DestroyMenu(( HMENU) var handle);
      hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
      var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
   }
}

Bool
apc_popup_create( Handle self, Handle owner)
{
   objCheck false;
   dobjCheck( owner) false;
   sys owner = DHANDLE( owner);
   apc_menu_destroy( self);
   var handle  = ( Handle) add_item( false, self, (( PMenu) self)-> tree);
   sys className = WC_MENU;
   return var handle != nilHandle;
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
   objCheck false;

   y = dsys( var owner) lastSize. y - y;
   owner = ( HWND)(( PComponent) var owner)-> handle;
   if ( var owner != application) {
      POINT pt = {x,y};
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

   if ( !TrackPopupMenuEx(
      ( HMENU) var handle, TPM_LEFTBUTTON|TPM_LEFTALIGN|TPM_RIGHTBUTTON,
       x, y, owner, anchor ? &tpm : NULL)
      ) apiErrRet;
   return true;
}


int ctx_kb2VK[] = {
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
   endCtx
};

int ctx_kb2VK2[] = {
   kbBackspace   ,   VK_BACK           ,
   kbTab         ,   VK_TAB            ,
   kbEsc         ,   VK_ESCAPE         ,
   kbSpace       ,   VK_SPACE          ,
   kbEnter       ,   VK_RETURN         ,
   endCtx
};

int ctx_kb2VK3[] = {
   kbAltL        ,   kbAltR            ,
   kbShiftL      ,   kbShiftR          ,
   kbCtrlL       ,   kbCtrlR           ,
   endCtx
};


static int ctx_hmp2HELP[] =
{
   hmpMain,          HELP_CONTENTS,
   hmpContents,      HELP_CONTENTS,
   hmpExtra,         HELP_INDEX,
   endCtx
};

Bool
apc_help_open_topic( Handle self, long command)
{
   UINT hm = ctx_remap_def( command, ctx_hmp2HELP, true, HELP_CONTEXT);
   DWORD data = ( hm == HELP_CONTEXT) ? ( DWORD) command : 0;
   dobjCheck( application) false;
   return WinHelp( DHANDLE( application), PApplication(application)-> helpFile, hm, data);
}

void
apc_help_close( Handle self)
{
   dobjCheck( application);
   WinHelp( DHANDLE( application), PApplication(application)-> helpFile, HELP_QUIT, 0);
}

void
apc_help_set_file( Handle self, const char * helpFile)
{
}


typedef LRESULT (*ApiMessageSender)(HWND hwnd, UINT msg, WPARAM mp1, LPARAM mp2);
void
apc_message( Handle self, PEvent ev, Bool post)
{
   ULONG msg;
   USHORT mp1s = 0;
   ApiMessageSender sender = post ? (ApiMessageSender) PostMessage : (ApiMessageSender) SendMessage;
   objCheck;
   switch ( ev-> cmd)
   {
       case cmPost:
          sender(( HWND) var handle, WM_POSTAL, ( WPARAM) ev-> gen. H, ( LPARAM) ev-> gen. p);
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
                kp = malloc( sizeof( KeyPacket));
                kp-> mp1 = mp1;
                kp-> mp2 = mp2;
                kp-> msg = msg;
                kp-> wnd = ( HWND) var handle;
                kp-> mod = ev-> pos. mod;
                PostMessage( 0, WM_KEYPACKET, 0, ( LPARAM) kp);
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
                      return;
                } else {
                   SHORT c = VkKeyScan( ev-> key. code);
                   if ( c == -1) {
                      HKL kl = guts. keyLayout ? guts. keyLayout : GetKeyboardLayout( 0);
                      c = VkKeyScanEx( ev-> key. code, kl);
                      if ( c == -1) return;
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
                    if ( mp1 == 0) return;
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
                kp = malloc( sizeof( KeyPacket));
                kp-> mp1 = mp1;
                kp-> mp2 = mp2;
                kp-> msg = msg;
                kp-> wnd = HANDLE;
                kp-> mod = ev-> key. mod;
                PostMessage( 0, WM_KEYPACKET, 0, ( LPARAM) kp);
             } else {
                BYTE * mod = mod_select( ev-> key. mod);
                SendMessage( HANDLE, msg, mp1, mp2);
                mod_free( mod);
             }
          }
          break;
   }
}


char *
apc_system_action( const char * params)
{
   switch ( *params) {
   case 'w':
      if ( strcmp( params, "wait.before.quit") == 0) {
         waitBeforeQuit = 1;
      } else if ( strncmp( params, "win32.DrawFocusRect ", 20) == 0) {
         RECT r;
         HWND win;
         Handle self;
         int i = sscanf( params + 20, "%lu %d %d %d %d", &win, &r.left, &r.bottom, &r.right, &r.top);

         if ( i != 5 || !( self = hwnd_to_view( win))) {
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
            log_write( "%08x: %s", h, GetWindowText( h, b, 255) ? b : "NULL");
         } else {
            log_write( "? No foc");
         }
      } else if (strncmp( params, "win32.WNetGetUser", 17) == 0) {
         char connection[ 1024];
         char user[ 1024];
         DWORD len = 1024;
         int i = sscanf( params + 18, "%s", connection);
         if ( i != 1) {
            warn( "Bad parameters to sysaction win32.WNetGetUser");
            return 0;
         }
         if ( WNetGetUser( connection, user, &len) != NO_ERROR)
            return 0;
         return strcpy( malloc( strlen( user) + 1), user);
      } else
         goto DEFAULT;
      break;
   DEFAULT:
   default:
      warn( "Unknown sysaction \"%s\"", params);
   }
   return 0;
}

