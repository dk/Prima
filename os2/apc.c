/*-
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
/* Created by:
         Dmitry Karasik <dk@plab.ku.dk>
         Anton Berezin  <tobez@plab.ku.dk>
 */
/* apc.c --- apc/ api for os/2 */
#include <limits.h>
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#define INCL_VIO
#define INCL_KBD
#include <float.h>
#include <io.h>
#include <fcntl.h>
#include "os2/os2guts.h"
#include "Menu.h"
#include "File.h"
#include "Window.h"
#include "Image.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)

// Application

Bool
apc_application_begin_paint ( Handle self)
{
   if ( !( sys ps = WinGetScreenPS( HWND_DESKTOP)))
        apiErrRet;
   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil))
        apiErr;
   list_add( &guts. winPsList, sys ps);
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   hwnd_enter_paint( self);
   return true;
}

Bool
apc_application_begin_paint_info( Handle self)
{
   Bool ok = apc_application_begin_paint( self);
   if ( ok) {
      HPS ps = sys ps;
      RECTL r = {0,0,0,0};
      HRGN old, rgn = GpiCreateRegion( ps, 1, &r);
      GpiSetClipRegion( ps, rgn, &old);
      GpiDestroyRegion( ps, old);
   }
   return ok;
}

Bool
apc_application_create( Handle self)
{
   ULONG style = 0;
   HWND client;
   sys className = WC_APPLICATION;

   if (( var handle = WinCreateStdWindow( HWND_DESKTOP, 0, &style, "GeNeRiC", "", 0, nilHandle, 0, &client)) == nilHandle)
      apiErrRet;
   sys parent  = HWND_DESKTOP;
   sys owner   = HWND_DESKTOP;
   WinSetWindowULong( var handle, QWL_USER, self);
   WinSetWindowULong( client, QWL_USER, self);
   WinPostMsg( var handle, WM_PRIMA_CREATE, 0, 0);
   WinStartTimer( guts. anchor, client, TID_USERMAX - 1, 100);
   var handle = client;
   apt_set( aptEnabled);
   return true;
}

Bool
apc_application_destroy( Handle self)
{
   if ( WinIsWindow( guts. anchor, var handle)) {
      if ( !WinStopTimer( guts. anchor, var handle, TID_USERMAX - 1)) apiErr;
      if ( !WinDestroyWindow( WinQueryWindow( var handle, QW_PARENT))) apiErr;
   }
   WinPostQueueMsg( guts. queue, WM_QUIT, MPVOID, MPVOID);
   return true;
}

Bool
apc_application_close( Handle self)
{
   return WinPostMsg( HANDLE, WM_QUIT, 0, 0);
}

Bool
apc_application_end_paint( Handle self)
{
   Bool ok = true;
   hwnd_leave_paint( self);
   list_delete( &guts. winPsList, sys ps);
   if ( !( ok = WinReleasePS( sys ps))) apiErr;
   sys ps = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
   return ok;
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
      strncpy( description, "Presentation Manager", len);
      description[len-1] = 0;
   }
   return guiPM;
}

/* XXX !palette */
Bool
apc_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
   HBITMAP bm;
   HPS ps, ps2;
   HDC dc;
   PBITMAPINFO2 bmInfo;
   POINTL pt [3] = {
      {0, 0},
      {xLen - 1, yLen - 1},
      {x, y},
   };
   int type = guts. bmf[ 1];

   if ( image == nilHandle) apcErrRet( errInvParams);
   dobjCheck( image) false;

   apcErrClear;

   if ( type > 8) type = 24;
   image_begin_query( type, &type);
   CImage( image)-> create_empty( image, xLen, yLen, type);

   bm = bitmap_make_ps( image, &ps, &dc, &bmInfo, cbScreen);
   if ( bm == nilHandle)
      return false;


   free( bmInfo);
   if ( GpiSetBitmap( ps, bm) == HBM_ERROR) apiErr;
   ps2 = WinGetScreenPS( HWND_DESKTOP);

   if ( GpiBitBlt( ps, ps2, 3, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) apiErr;

   WinReleasePS( ps2);

   image_query( image, ps);

   GpiSetBitmap( ps, nilHandle);
   GpiDeleteBitmap( bm);
   GpiDestroyPS( ps);
   DevCloseDC( dc);
   return true;
}

Handle
hwnd_to_view( HWND win)
{
   Handle h;
   int i = 2;
   while (i--) {
      if ( !WinIsWindow( guts. anchor, win)) return nilHandle;
      if ( WinQueryAnchorBlock( win) != guts. anchor) return nilHandle;
      h = WinQueryWindowULong( win, QWL_USER);
      if ( !h) return nilHandle;
      if ( WinQueryWindowPtr( win, QWP_PFNWP) == generic_view_handler) return h;
      win = WinWindowFromID( win, FID_CLIENT);
   }
   return nilHandle;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
   return hwnd_to_view(( HWND) apiHandle);
}

Rect
apc_application_get_indents( Handle self)
{
   Rect r = {0,0,0,0};
   return r;
}

int
apc_application_get_os_info( char *system, int slen,
                             char *release, int rlen,
                             char *vendor, int vlen,
                             char *arch, int alen)
{
   ULONG si[ 2];
   int version, subVersion;
   DosQuerySysInfo( QSV_VERSION_MAJOR, QSV_VERSION_MINOR, &si, sizeof( si));
   version    = ( si[ 1] >= 30) ? ( si[ 1] / 10) : ( si[ 0] / 10);
   subVersion = ( si[ 1] < 30)  ? si[ 1] : 0;
   if ( system) {
      strncpy( system, "OS/2", slen);
      system[slen-1] = 0;
   }
   if ( vendor) {
      strncpy( vendor, "IBM", vlen);
      vendor[vlen-1] = 0;
   }
   if ( arch) {
      strncpy( arch, "i386", alen);
      arch[alen-1] = 0;
   }
   if ( release) {
      snprintf( release, rlen, "%d.%d", version, subVersion);
   }
   return apcOs2;
}

Point
apc_application_get_size( Handle self)
{
   return ( Point) {
      WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN),
      WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN)
   };
}

Handle
apc_application_get_widget_from_point( Handle self, Point point)
{
   return hwnd_to_view( WinWindowFromPoint( HWND_DESKTOP, ( PPOINTL) &point, 1));
}

static Bool
process_msg( QMSG * msg)
{
   switch( msg-> msg) {
   case WM_TERMINATE:
   case WM_QUIT:
      return false;
   case WM_SOCKET:
      {
         int i;
         int socket = ( int) msg-> mp2;
         for ( i = 0; i < guts. files. count; i++) {
            Handle self = guts. files. items[ i];
            if (( sys s. file == socket) &&
                ( PFile( self)-> eventMask & (int) msg-> mp1)) {
               Event ev;
               ev. cmd = ((int) msg-> mp1 == feRead) ? cmFileRead :
                         (( (int) msg-> mp1 == feWrite) ? cmFileWrite : cmFileException);
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
   case WM_CROAK:
      if ( msg-> mp1)
         croak(( char *) msg-> mp2);
      else
         warn(( char *) msg-> mp2);
      return true;

   }
   return true;
}

Bool
apc_application_go( Handle self)
{
   QMSG message;

   while ( WinGetMsg( guts. anchor, &message, 0, 0, 0))
   {
      if ( !process_msg( &message)) break;
      WinDispatchMsg( guts. anchor, &message);
      kill_zombies();
   }
   if ( application) Object_destroy( application);
   return true;
}

static Bool
lock( Bool lock)
{
   if ( lock)
   {
      if ( guts. appLock++ == 0) return WinLockWindowUpdate( HWND_DESKTOP, HWND_DESKTOP);
   }
   else
   {
      if ( --guts. appLock == 0) return WinLockWindowUpdate( HWND_DESKTOP, nilHandle);
   }
   return true;
}


Bool
apc_application_lock( Handle self)
{
  return lock( true);
}

Bool
apc_application_unlock( Handle self)
{
  return lock( false);
}

Bool
apc_application_sync( void)
{
   return true;
}

Bool
apc_application_yield( void)
{
   QMSG message;

   while ( WinPeekMsg( guts. anchor, &message, 0, 0, 0, PM_REMOVE))
   {
      if ( !process_msg( &message)) {
         if ( message. hwnd)
            WinSendMsg( message. hwnd, WM_CLOSE, MPVOID, MPVOID);
         break;
      }
      WinDispatchMsg( guts. anchor, &message);
      kill_zombies();
   }
   return true;
}

// Component

Bool
apc_component_create( Handle self)
{
   PComponent c = ( PComponent) self;
   PDrawableData d = c-> sysData;

   if ( d) return false;
   if ( !( d = malloc( sizeof( DrawableData))))
      return false;
   memset( d, 0, sizeof( DrawableData));
   c-> sysData = d;
   return true;
}

Bool
apc_component_destroy( Handle self)
{
   PComponent    c = ( PComponent) self;
   PDrawableData d = c-> sysData;
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

// End Component

typedef struct _ViewProfile {
  ColorSet colors;
  Point    pos;
  Point    size;
  Point    virtSize;
  Bool     enabled;
  Bool     visible;
  Bool     focused;
  Bool     capture;
} ViewProfile, *PViewProfile;

static void
get_view_ex( Handle self, PViewProfile p)
{
  int i;
  if ( !p) return;
  p-> capture   = apc_widget_is_captured( self);
  for ( i = 0; i <= ciMaxId; i++) p-> colors[ i] = apc_widget_get_color( self, i);
  p-> pos       = apc_widget_get_pos( self);
  p-> size      = apc_widget_get_size( self);
  p-> virtSize  = var virtualSize;
  p-> enabled   = apc_widget_is_enabled( self);
  p-> focused   = apc_widget_is_focused( self);
  p-> visible   = apc_widget_is_visible( self);
}

void
process_transparents( Handle self)
{
   int i;
   SWP swp, rswp;
   RECTL mr, r, dr;
   objCheck;
   WinQueryWindowPos(( HWND) var handle, &swp);
   mr. xLeft   = 0;
   mr. yBottom = 0;
   mr. xRight  = swp. cx;
   mr. yTop    = swp. cy;
   for ( i = 0; i < var widgets. count; i++) {
      HWND xwnd;
      Handle x = var widgets. items[ i];
      dobjCheck(x);
      xwnd = DHANDLE(x);
      if (( dsys(x) options & aptTransparent) && WinIsWindowVisible( xwnd)) {
         WinQueryWindowPos( xwnd, &rswp);
         r. xLeft   = rswp. x;
         r. yBottom = rswp. y;
         r. xRight  = rswp. x + rswp. cx;
         r. yTop    = rswp. y + rswp. cy;
         WinIntersectRect( guts. anchor, &dr, &r, &mr);
         if ( !WinIsRectEmpty( guts. anchor, &dr)) {
            WinInvalidateRect( xwnd, nil, false);
         }
      }
   }
}


static void
set_view_ex( Handle self, PViewProfile p)
{
  int i;
  HWND wnd = var handle;
  apc_widget_set_visible( self, false);
  for ( i = 0; i <= ciMaxId; i++) apc_widget_set_color( self, p-> colors[i], i);
  apc_widget_set_font( self, &var font);
  apc_widget_set_rect( self, p-> pos. x, p-> pos. y, p-> size. x, p-> size. y);
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
           if ( !WinStartTimer ( guts. anchor, wnd, var handle, sys s. timer. timeout)) apiErr;
     }
   if ( !WinInvalidateRect ( var handle, nil, true)) apiErr;
   process_transparents( self);
}

// Window

static Bool repost_msgs( PostMsg * msg, Handle self)
{
   WinPostMsg( var handle, WM_POSTAL, ( MPARAM) self, ( MPARAM) msg);
   return false;
}

static Bool
create_group( Handle self, Handle owner, Bool syncPaint, Bool clipOwner,
              PSZ className, ULONG style, ViewProfile * vprf, HWND parentHandle)
{
   HWND ret;
   HWND old = HANDLE;
   HWND behind = HWND_TOP;
   HWND ownerView  = (( PWidget) owner)-> handle;
   HWND parentView = (( PDrawableData)(( PComponent) owner)-> sysData)-> parent;
   int  i, count = 0;
   Bool reset = false;
   Handle * list = nil;
   int oStage = var stage;

   if ( HANDLE)
   {
      if (( DHANDLE( owner) == sys owner) && ( clipOwner == is_apt( aptClipOwner)))
      {
         behind = WinQueryWindow( HANDLE, QW_PREV);
         if ( behind == nilHandle) behind = HWND_TOP;
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
          Handle frame;
          parentView = HWND_DESKTOP;
          if ( !( frame = WinCreateStdWindow( parentView, 0, &style, "GeNeRiC",
            "", WS_CLIPCHILDREN
            | ( syncPaint ? WS_SYNCPAINT : 0)
            , nilHandle, 0, &ret)))
               apiErrRet;
          WinSetWindowULong( frame, QWL_USER, ( ApiHandle) WinSubclassWindow( frame, ( PFNWP) generic_frame_handler));
          if ( !WinSetWindowPos( frame, behind, 0, 0, 0, 0, SWP_ZORDER | SWP_NOADJUST))
             apiErr;
          if ((( PApplication) application)-> topExclModal &&
              ((( PApplication) application)-> topExclModal != self)
             )
             if ( !WinSetWindowPos( frame, DHANDLE((( PApplication) application)-> topExclModal),
                   0,0,0,0, SWP_ZORDER))
                apiErr;
       }
       break;
    case WC_CUSTOM:
       if ( !clipOwner) parentView = HWND_DESKTOP;
       if ( parentHandle) parentView = parentHandle;
       if ( !( ret = WinCreateWindow( parentView, "GeNeRiC", "",
          style |  WS_CLIPCHILDREN
          | WS_CLIPSIBLINGS
          | ( syncPaint ? WS_SYNCPAINT : 0)
          , 0, 0, 0, 0, ownerView,
          behind, 0, nil, nil)))
            apiErrRet;
       sys parentHandle = parentHandle;
       break;
   }
   apt_assign( aptClipOwner, clipOwner);
   apt_assign( aptSyncPaint, syncPaint);
   apt_set( aptEnabled);
   sys className = className;
   sys parent  = ret;
   var handle  = ret;
   sys owner   = ownerView;
   if ( !reset)
      sys pointer = WinQuerySysPointer( HWND_DESKTOP, SPTR_ARROW, false);
   WinSetWindowULong( ret, QWL_USER, ( ULONG) self);

   if ( reset)
   {
      int i;
      Handle oldOwner = owner; var owner = owner;
      apc_widget_set_rect( self, vprf-> pos. x, vprf-> pos. y, vprf-> size.x, vprf-> size.y);
      var owner = oldOwner;
      for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
      if ( sys className == WC_FRAME)
      {
         PAbstractMenu menu = ( PAbstractMenu)((( PWindow) self)-> menu);
         if ( menu) menu-> self-> set_selected(( Handle) menu, true);
      }
      if ( !WinDestroyWindow( old))
         apiErr;
      if ( var postList) list_first_that( var postList, repost_msgs, ( void*)self);
   }
   WinPostMsg( ret, WM_PRIMA_CREATE, 0, 0);
   var stage = oStage;
   return true;
}

Bool
apc_window_create( Handle self, Handle owner, Bool syncPaint, int borderIcons,
                   int borderStyle, Bool taskList, int windowState, 
		   int on_top, Bool usePos, Bool useSize)
{
   Bool reset = false;
   ViewProfile vprf;
   int oStage = var stage;
   WindowData ws;
   Rect maxRS;
   ULONG frameFlags = FCF_NOBYTEALIGN
      | (( borderIcons &  biSystemMenu) ? FCF_SYSMENU       : 0)
      | (( borderIcons &  biMinimize)   ? FCF_MINBUTTON     : 0)
      | (( borderIcons &  biMaximize)   ? FCF_MAXBUTTON     : 0)
      | (( borderIcons &  biTitleBar)   ? FCF_TITLEBAR      : 0)
      | (( borderStyle == bsSizeable)   ? FCF_SIZEBORDER    : 0)
      | (( borderStyle == bsSingle  )   ? FCF_BORDER        : 0)
      | (( borderStyle == bsDialog  )   ? FCF_DLGBORDER     : 0)
      | (  taskList                     ? FCF_TASKLIST      : 0)
      | (( !usePos && !useSize && windowState != wsMaximized) ? FCF_SHELLPOSITION : 0)
   ;

   objCheck false;
   dobjCheck( owner) false;

   if ( !kind_of( self, CWidget)) apcErrRet( errInvObject);
   apcErrClear;
   lock( true);
   if (( var handle != nilHandle) && (
        ( DHANDLE( owner) != sys owner)
     || ( borderStyle != sys s. window. borderStyle)
     || ( borderIcons != sys s. window. borderIcons)
     || ( syncPaint   != is_apt( aptSyncPaint))
     || ( taskList    != is_apt( aptTaskList))
   ))
   {
      HWND h = HANDLE;
      apc_window_set_window_state( self, windowState);
     // prevent cmSize/cmWindowStage message loss if recreate goes with WS_XXX change.
     if ( sys recreateData) {
        memcpy( &vprf, sys recreateData, sizeof( vprf));
        free( sys recreateData);
        sys recreateData = nil;
     } else
        get_view_ex( self, &vprf);
      ws = sys s. window;
      maxRS. left   = WinQueryWindowUShort( h, QWS_XRESTORE);
      maxRS. bottom = WinQueryWindowUShort( h, QWS_YRESTORE);
      maxRS. right  = WinQueryWindowUShort( h, QWS_CXRESTORE);
      maxRS. top    = WinQueryWindowUShort( h, QWS_CYRESTORE);
      frameFlags   &= ~FCF_SHELLPOSITION; // prevent using that flag on recreate
      reset = true;
   }
   if ( reset || ( var handle == nilHandle))
      if ( !create_group( self, owner, syncPaint, false, WC_FRAME, frameFlags, &vprf, nilHandle)) {
         lock( false);
         return false;
      }

   ws. borderStyle = sys s. window. borderStyle = borderStyle;
   ws. borderIcons = sys s. window. borderIcons = borderIcons;
   apt_assign( aptSyncPaint, syncPaint);
   apt_assign( aptTaskList,  taskList);
   apc_window_set_window_state( self, windowState);
   if ( reset)
   {
      HWND h = HANDLE;
      Handle oldOwner = var owner; var owner = oldOwner;
      set_view_ex( self, &vprf);
      var owner = oldOwner;
      sys s. window = ws;
      WinSetWindowUShort( h, QWS_XRESTORE,  maxRS. left);
      WinSetWindowUShort( h, QWS_YRESTORE,  maxRS. bottom);
      WinSetWindowUShort( h, QWS_CXRESTORE, maxRS. right);
      WinSetWindowUShort( h, QWS_CYRESTORE, maxRS. top);
      var stage = oStage;
   }
   else
      sys s. window. modalResult = -1;
   apc_window_set_caption( self, var text, is_opt( optUTF8_text));
   lock( false);

   return true;
}

Bool
apc_window_activate( Handle self)
{
   Bool ok = false;
   if ( self) {
      if ( self != Application_map_focus( application, self))
         return false;
      if ( !( ok = WinSetActiveWindow( HWND_DESKTOP, HANDLE))) apiErr;
   }
   return ok;
}

Bool
apc_window_is_active( Handle self)
{
   return WinQueryActiveWindow( HWND_DESKTOP) == HANDLE;
}

Handle
apc_window_get_active( void)
{
   Handle self = hwnd_to_view( WinQueryActiveWindow( HWND_DESKTOP));
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
   if ( !WinPostMsg( HANDLE, WM_CLOSE, 0, 0)) apiErrRet;
   return true;
}

int
apc_window_get_border_icons ( Handle self)
{
   return sys s. window. borderIcons;
}

int
apc_window_get_border_style ( Handle self)
{
   return sys s. window. borderStyle;
}

Point
frame2client( Handle self, Point p, Bool f2c)
{
   RECTL r = {0,0,p.x,p.y};
   WinCalcFrameRect( HANDLE, &r, f2c);
   return ( Point){ r. xRight - r. xLeft,  r. yTop - r. yBottom};
}

static Point
get_window_borders( int borderStyle)
{
   Point ret = { 0, 0};
   switch ( borderStyle)
   {
      case bsSizeable:
         ret. x = WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER);
         ret. y = WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER);
         break;
      case bsSingle:
         ret. x = WinQuerySysValue( HWND_DESKTOP, SV_CXBORDER);
         ret. y = WinQuerySysValue( HWND_DESKTOP, SV_CYBORDER);
         break;
      case bsDialog:
         ret. x = WinQuerySysValue( HWND_DESKTOP, SV_CXDLGFRAME);
         ret. y = WinQuerySysValue( HWND_DESKTOP, SV_CYDLGFRAME);
         break;
   }
   return ret;
}


Point
apc_window_get_client_pos( Handle self)
{
   SWP  swp;
   Point d = get_window_borders( sys s. window. borderStyle);
   if ( sys s. window. state == wsMinimized)
   {
      d. x += sys s. window. hiddenPos. x;
      d. y += sys s. window. hiddenPos. y;
   }
   else {
      if ( !WinQueryWindowPos ( HANDLE, &swp)) apiErr;
      d. x += swp. x;
      d. y += swp. y;
   }
   return d;
}


Point
apc_window_get_client_size( Handle self)
{
   SWP  swp;
   if ( sys s. window. state == wsMinimized)
      return sys s. window. hiddenSize;
   else
      if ( !WinQueryWindowPos ( var handle, &swp)) apiErr;
   return ( Point){ swp. cx, swp. cy};
}

Bool
apc_window_get_icon( Handle self, Handle icon)
{
   HPOINTER p, save;
   Bool ret;

   p = ( HPOINTER) WinSendMsg( HANDLE, WM_QUERYICON, 0, 0);
   if ( icon == nilHandle) return p != nilHandle;

   save = sys pointer;
   sys pointer = p;
   ret = apc_pointer_get_bitmap( self, icon);
   sys pointer = save;
   return ret;
}

Bool
apc_window_get_task_listed( Handle self)
{
   return is_apt( aptTaskList);
}

Bool
apc_window_get_on_top( Handle self)
{
   return false; /* XXX */
}

int
apc_window_get_window_state( Handle self)
{
   SWP s;
   if ( !WinQueryWindowPos( HANDLE, &s)) apiErr;
   if ( s. fl & SWP_MAXIMIZE) return wsMaximized;
   if ( s. fl & SWP_MINIMIZE) return wsMinimized;
   return wsNormal;
}

Bool
apc_window_set_caption( Handle self, const char * caption, Bool utf8)
{
   Bool ok;
   if ( caption == nil || strlen( caption) == 0) caption = " ";
   if ( !( ok = WinSetWindowText( HANDLE, caption))) apiErr;
   return ok;
}

Bool
apc_window_set_client_size( Handle self, int x, int y)
{
   Bool ok = true;
   if ( sys s. window. state == wsMinimized)
   {
      var virtualSize = ( Point){x, y};
      if ( x < 0) x = 0;
      if ( y < 0) y = 0;
      sys s. window. lastFrameSize. x += sys s. window. lastFrameSize. x - sys s. window. lastClientSize. x + x;
      sys s. window. lastFrameSize. y += sys s. window. lastFrameSize. y - sys s. window. lastClientSize. y + y;
      sys s. window. hiddenSize = sys s. window. lastClientSize = ( Point){x, y};
   } else {
      Point p = frame2client( self, ( Point){x, y}, false);
      if ( var stage == csConstructing && sys s. window. state != wsNormal)
      {
         HWND h = HANDLE;
         if ( !( ok = WinSetWindowUShort( h, QWS_CXRESTORE, p. x))) apiErr;
         if ( !( ok &= WinSetWindowUShort( h, QWS_CYRESTORE, p. y))) apiErr;
         var virtualSize = ( Point){x, y};
      } else {
         sys sizeLockLevel++;
         var virtualSize = ( Point){x, y};
         if ( !( ok = WinSetWindowPos( HANDLE, 0, 0, 0, p. x, p. y, SWP_SIZE))) apiErr;
         sys sizeLockLevel--;
      }
   }
   return ok;
}

Bool
apc_window_set_client_rect( Handle self, int x, int y, int width, int height)
{
   Bool ok = true;
   Point d = get_window_borders( sys s. window. borderStyle);
   if ( sys s. window. state == wsMinimized)
   {
      var virtualSize = ( Point){width, height};
      if ( width < 0) width = 0;
      if ( height < 0) height = 0;
      sys s. window. lastFrameSize. x += sys s. window. lastFrameSize. x - sys s. window. lastClientSize. x + width;
      sys s. window. lastFrameSize. y += sys s. window. lastFrameSize. y - sys s. window. lastClientSize. y + height;
      sys s. window. hiddenPos = ( Point){x - d.x, y - d.y};
      sys s. window. hiddenSize = sys s. window. lastClientSize = ( Point){width, height};
   } else {
      Point p = frame2client( self, ( Point){width, height}, false);
      if ( var stage == csConstructing && sys s. window. state != wsNormal)
      {
         HWND h = HANDLE;
         if ( !( ok = WinSetWindowUShort( h, QWS_CXRESTORE, p. x))) apiErr;
         if ( !( ok &= WinSetWindowUShort( h, QWS_CYRESTORE, p. y))) apiErr;
         if ( !( ok &= WinSetWindowUShort( h, QWS_XRESTORE, x - d.x))) apiErr;
         if ( !( ok &= WinSetWindowUShort( h, QWS_YRESTORE, y - d.y))) apiErr;
         var virtualSize = ( Point){width, height};
      } else {
         sys sizeLockLevel++;
         var virtualSize = ( Point){width, height};
         if ( !( ok = WinSetWindowPos( HANDLE, 0, x - d.x, y - d.y, p. x, p. y, SWP_SIZE|SWP_MOVE))) apiErr;
         sys sizeLockLevel--;
      }
   }
   return ok;
}


Bool
apc_window_set_client_pos( Handle self, int x, int y)
{
   Bool ok = true;
   Point d = get_window_borders( sys s. window. borderStyle);
   if ( sys s. window. state == wsMinimized)
   {
      sys s. window. hiddenPos = ( Point){x - d.x, y - d.y};
   } else {
      HWND h = HANDLE;
      if ( var stage == csConstructing && sys s. window. state != wsNormal)
      {
         if ( !( ok = WinSetWindowUShort( h, QWS_XRESTORE, x - d.x))) apiErr;
         if ( !( ok &= WinSetWindowUShort( h, QWS_YRESTORE, y - d.y))) apiErr;
      } else {
         if ( !( ok = WinSetWindowPos( h, 0, x - d.x, y - d.y, 0, 0, SWP_MOVE))) apiErr;
      }
   }
   return ok;
}

static HWND add_item( HWND w, Handle menu, PMenuItemReg i);

Bool
apc_window_set_menu( Handle self, Handle menu)
{
   HWND focus = WinQueryFocus( HWND_DESKTOP);
   Point size = var self-> get_size( self);
   HWND wMenu = WinWindowFromID( HANDLE, FID_MENU);

   if ( menu) (( PComponent) menu)-> handle = nilHandle;
   apcErrClear;

   if ( wMenu)
      WinDestroyWindow( wMenu);
   if ( menu)
      (( PComponent) menu)-> handle = add_item( HANDLE, menu, (( PMenu) menu)-> tree);
   if ( var stage <= csNormal)
   {
      WinSendMsg( HANDLE, WM_UPDATEFRAME, MPFROMLONG( FCF_MENU), 0);
      var self-> set_size( self, size);
   }
   WinSetFocus( HWND_DESKTOP, focus);
   return apcError == 0;
}

Bool
apc_window_set_icon( Handle self, Handle icon)
{
   HPOINTER p = icon ? pointer_make_handle( self, icon, ( Point){0,0}) : nilHandle;
   WinSendMsg( HANDLE, WM_SETICON, MPFROMP( p), 0);
   return false;
}

Bool
apc_window_set_window_state( Handle self, int state)
{
   int  fl = -1;
   switch ( state)
   {
      case wsMaximized: fl = SWP_MAXIMIZE; break;
      case wsMinimized: fl = SWP_MINIMIZE; break;
      case wsNormal   : fl = SWP_RESTORE ; break;
   }
   if ( fl > 0)
   {
      if ( !WinSetWindowPos( HANDLE, 0, 0, 0, 0, 0, fl)) apiErr;
      sys s. window. state = state;
   }
   return false;
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insertBefore)
{
   HWND wnd;
   HWND active = WinQueryActiveWindow( HWND_DESKTOP);

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
   apc_widget_set_visible( self, 1);
   if ( sys s. window. state == wsMinimized)
      WinSetWindowPos( wnd, nilHandle, 0, 0, 0, 0, SWP_RESTORE);
   if ( !insertBefore) {
      WinSetActiveWindow( HWND_DESKTOP, wnd);
      CWidget( self)-> set_selected( self, 1);
   } else {
      HWND zorder;
      dobjCheck( insertBefore) false;
      zorder = WinQueryWindow( DHANDLE( insertBefore), QW_NEXT);
      if ( !zorder) zorder = HWND_BOTTOM;
      WinSetWindowPos( wnd, zorder, 0, 0, 0, 0, SWP_ZORDER);
      if ( active)
         WinSetActiveWindow( HWND_DESKTOP, active);
   }
   objCheck false;
   WinPostMsg( wnd, WM_DLGENTERMODAL, ( MPARAM) 1, ( MPARAM) 0);
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
      QMSG message;
      while ( WinGetMsg( guts. anchor, &message, 0, 0, 0))
      {
         if ( message. msg == WM_TERMINATE)
            break;
         WinDispatchMsg( guts. anchor, &message);
         kill_zombies();
         if ( self && !(( PWindow) self)-> modal)
            break;
      }
      if ( message. msg == WM_QUIT || message. msg == WM_TERMINATE)
         WinPostMsg( 0, message. msg, message. mp1, message. mp2);
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
   WinShowWindow( wnd, 0);
   objCheck false;
   apc_widget_set_enabled( self, 0);
   objCheck false;
   if ( application) {
      Handle who = Application_popup_modal( application);
      if ( sys s. window. oldActive)
         WinSetActiveWindow( HWND_DESKTOP, sys s. window. oldActive);
      if ( !who && var owner) {
         CWidget( var owner)-> set_selected( var owner, 1);
      }
      if (( who = sys s. window. oldFoc)) {
         if ( PWidget( who)-> stage == csNormal)
            CWidget( who)-> set_focused( who, 1);
         unprotect_object( who);
      }
   }
   guts. focSysDisabled = 0;
   return true;
}

// Widget

Bool
apc_widget_map_points( Handle self, Bool toScreen, int count, Point * points)
{
   Bool ok;

   if ( self == application) return true;

   if ( !( ok = WinMapWindowPoints(
       toScreen ? var handle : HWND_DESKTOP,
       toScreen ? HWND_DESKTOP : var handle,
       ( PPOINTL) points, count))) apiErr;
   return ok;
}


Color
apc_widget_map_color( Handle self, Color color)
{
   if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0)) color |= var widgetClass;
   return remap_color( NULLHANDLE, color, true);
}

#define need_view_recreate    (( DHANDLE( owner) != sys owner)        \
                             || ( syncPaint != is_apt( aptSyncPaint)) \
                             || ( clipOwner != is_apt( aptClipOwner)) \
                             || (( HWND) parentHandle != sys parentHandle) \
                             )

Bool
apc_widget_create( Handle self, Handle owner, Bool syncPaint, Bool clipOwner, Bool transparent, ApiHandle parentHandle)
{
   Bool reset = false;
   ViewProfile vprf;
   int oStage = var stage;

   objCheck false;
   dobjCheck( owner) false;

   if ( !kind_of( self, CWidget)) apcErr( errInvObject);
   apcErrClear;

   if ( parentHandle && !WinIsWindow( guts. anchor, ( HWND) parentHandle))
      return false;

   if (( var handle != nilHandle) && need_view_recreate )
   {
      if ( sys recreateData) {
         memcpy( &vprf, sys recreateData, sizeof( vprf));
         free( sys recreateData);
         sys recreateData = nil;
      } else
         get_view_ex( self, &vprf);
      reset = true;
   }
   if ( reset || ( var handle == nilHandle)) {
      if ( !create_group( self, owner, syncPaint, clipOwner, WC_CUSTOM, 0, &vprf, parentHandle))
         return false;
   }
   if ( reset)
   {
      set_view_ex( self, &vprf);
      var stage = oStage;
   }

   if (( is_apt( aptTransparent) != transparent) && !reset) apc_widget_invalidate_rect( self, nil);
   apt_assign( aptTransparent, transparent);
   if ( reset) apc_widget_invalidate_rect( self, nil);
   return true;
}

Bool
apc_widget_begin_paint( Handle self, Bool insideOnPaint)
{
   Bool useRPDraw = false;
   Bool presetHPS = ( sys ps != nilHandle);
   apcErrClear;
   if ( is_apt( aptTransparent))
   {
      SWP swp;
      WinQueryWindowPos( var handle, &swp);
      if (( swp. fl & SWP_HIDE) == 0)
      {
         HWND os2parent = WinQueryWindow( HANDLE, QW_PARENT);
         if ( os2parent == nilHandle || os2parent == HWND_DESKTOP) {
            HWND parent = dsys((( PWidget) self)-> owner) parent;
            list_add( &guts. transp, self);
            WinSetWindowPos( var handle, 0, 0, 0, 0, 0, SWP_HIDE);
            WinUpdateWindow( parent);
            if ( parent == HWND_DESKTOP) DosSleep( 1);
            WinSetWindowPos( var handle, 0, 0, 0, 0, 0, SWP_SHOW);
            list_delete( &guts. transp, self);
         } else
            useRPDraw = true;
      }
   }
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   apt_assign( aptWM_PAINT, insideOnPaint);
   sys transform2 = ( Point){0, 0};

   if ( insideOnPaint)
   {
      if ( !presetHPS)
      {
         if ( !( sys ps = WinBeginPaint( var handle, nilHandle, nil))) apiErrRet;
         list_add( &guts. psList, sys ps);
      }
   } else {
      if ( !( sys ps = WinGetPS( var handle))) apiErrRet;
      list_add( &guts. winPsList, sys ps);
   }

   if ( is_opt( optBuffered)) {
      HDC hdc;
      HPS hps;
      PBITMAPINFO2 bmInfo;
      HBITMAP bm;
      SWP swp;
      Rect cr = apc_gp_get_clip_rect( self);
      WinQueryWindowPos( var handle, &swp);

      var w = cr. right - cr. left   + 1;
      var h = cr. top   - cr. bottom + 1;

      if ( var w == 0 || var h == 0) {
         if ( !WinEndPaint( sys ps)) apiErr;
         apt_clear( aptWinPS);
         apt_clear( aptWM_PAINT);
         apt_clear( aptCompatiblePS);
         sys ps = nilHandle;
         return false;
      }

      bm = bitmap_make_ps( self, &hps, &hdc, &bmInfo, cbScreen);
      if ( bm != nilHandle) {
         sys ps2 = sys ps;
         sys ps  = hps;
         sys dc2 = sys dc;
         sys dc = hdc;
         sys bm = bm;
         if ( GpiSetBitmap( hps, bm) == HBM_ERROR) apiErr;
         apc_gp_set_transform( self, -cr. left, -cr. bottom);
         sys transform2 = ( Point) {cr. left, cr. bottom};
         free( bmInfo);
      }
   }

   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;
   hwnd_enter_paint( self);

   if ( useRPDraw) {
      HPS ps;
      Handle owner = var owner;
      Point tr = dsys(owner)transform2;
      Point ed = apc_widget_get_pos( self);
      Color f, b, f1, b1;


      CWidget(owner)-> begin_paint( owner);
      f  = apc_gp_get_color( owner);
      b  = apc_gp_get_back_color( owner);
      f1 = apc_gp_get_color( self);
      b1 = apc_gp_get_back_color( self);

      ps = dsys( owner) ps;
      dsys(owner) ps = sys ps;
      dsys(owner) transform2. x += ed. x;
      dsys(owner) transform2. y += ed. y;
      apc_gp_set_transform( owner, 0, 0);
      apc_gp_set_color( owner, f);
      apc_gp_set_back_color( owner, b);

      CWidget( owner)-> notify( owner, "sH", "Paint", owner);
      apc_gp_set_color( owner, f1);
      apc_gp_set_back_color( owner, b1);

      dsys(owner)transform2 = tr;
      apc_gp_set_transform( owner, 0, 0);
      dsys(owner) ps = ps;
      CWidget( owner)-> end_paint( owner);
      apc_gp_set_transform( self, sys transform. x, sys transform. y);
   }

   return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
   RECTL r = {0,0,0,0};
   HRGN old, rgn;
   HPS ps;
   objCheck false;
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   sys transform2. x = 0;
   sys transform2. y = 0;
   if ( !( ps = sys ps = WinGetPS( var handle))) apiErrRet;
   list_add( &guts. winPsList, sys ps);
   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;
   hwnd_enter_paint( self);
   rgn = GpiCreateRegion( ps, 1, &r);
   GpiSetClipRegion( ps, rgn, &old);
   GpiDestroyRegion( ps, old);
   return true;
}

Bool
apc_widget_destroy( Handle self)
{
   if ( sys pointer2) {
      if ( sys pointer2 == sys pointer) WinSetPointer( HWND_DESKTOP, ( HPOINTER) NULL); // un-use resource first
      if ( !WinDestroyPointer( sys pointer2)) apiErr;
   }
   free( sys recreateData);
   free( sys timeDefs);
   if ( self == lastMouseOver) lastMouseOver = nilHandle;
   if ( var handle == nilHandle) return false;
   if ( !WinDestroyWindow( HANDLE)) apiErr;
   return true;
}

PFont
apc_widget_default_font ( PFont font)
{
   char buf  [256];
   apcErrClear;
   PrfQueryProfileString( HINI_USERPROFILE, "PM_SystemFonts", "WindowText", DEFAULT_FONT_FACE, buf, 255);
   font_pp2gp( buf, font);
   apc_font_pick( nilHandle, font, font);
   return font;
}

Bool
apc_widget_end_paint( Handle self)
{
   if ( is_opt( optBuffered) && ( sys bm != nilHandle)) {
      POINTL pt [4] = {
         { sys transform2. x, sys transform2. y},
         { sys transform2. x + var w - 1, sys transform2. y + var h - 1},
         {0, 0}, { var w, var h}
      };

      if ( sys fillBitmap) {
         if ( !GpiSetPatternSet( sys ps, LCID_DEFAULT)) apiErr;
         if ( !GpiDeleteSetId( sys ps, 3)) apiErr;
         if ( !GpiDeleteBitmap( sys fillBitmap)) apiErr;
         sys fillBitmap = nilHandle;
      }

      GpiSetBitmap( sys ps, nilHandle);
      if ( GpiWCBitBlt( sys ps2, sys bm, 4, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) apiErr;
      GpiDeleteBitmap( sys bm);
      GpiDestroyPS( sys ps);
      DevCloseDC( sys dc);
      sys ps = sys ps2;
      sys dc = sys dc2;
      sys ps2 = sys dc2 = nilHandle;
      sys bm = nilHandle;
   }

   hwnd_leave_paint( self);
   if ( sys ps != nilHandle)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT))
      {
         if ( !WinEndPaint( sys ps)) apiErr;
         list_delete( &guts. psList, sys ps);
      }
      else if ( is_apt( aptWinPS))
      {
         if ( !WinReleasePS( sys ps)) apiErr;
         list_delete( &guts. winPsList, sys ps);
      }
   }
   sys ps = nilHandle;
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
   if ( !WinReleasePS( sys ps)) apiErr;
   list_delete( &guts. winPsList, sys ps);
   sys ps = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
   return ok;
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   return is_apt( aptClipOwner);
}

static int ctx_ci2PP[] =
{
   ciNormalText     , PP_FOREGROUNDCOLOR,
   ciNormal         , PP_BACKGROUNDCOLOR,
   ciHiliteText     , PP_HILITEFOREGROUNDCOLOR,
   ciHilite         , PP_HILITEBACKGROUNDCOLOR,
   ciDisabledText   , PP_DISABLEDFOREGROUNDCOLOR,
   ciDisabled       , PP_DISABLEDBACKGROUNDCOLOR,
   ciLight3DColor   , PP_BORDERCOLOR,
   ciDark3DColor    , PP_BORDERDARKCOLOR,
   endCtx,
};


Color
apc_widget_get_color( Handle self, int index)
{
   long p = 0;
   if ( index == ciLight3DColor) return sys l3dc;
   if ( index == ciDark3DColor)  return sys d3dc;
   if ( WinQueryPresParam(
      var handle,
      ctx_remap( index, ctx_ci2PP, true),
      0, nil, sizeof( long), &p, QPF_NOINHERIT))
   return p;
   apiErr;
   return clInvalid;
}

Bool
apc_widget_get_first_click ( Handle self)
{
   return is_apt( aptFirstClick);
}

Handle
apc_widget_get_focused( void)
{
   return hwnd_to_view( WinQueryFocus( HWND_DESKTOP));
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
   Rect r;
   Bool ok = ( Bool) WinQueryUpdateRect( var handle, ( PRECTL) &r);
   return ok ? r : ( Rect){0,0,0,0};
}

Point
apc_widget_get_pos( Handle self)
{
   SWP  swp;
   if ( swp. fl & SWP_MINIMIZE && ( sys className == WC_FRAME)) return sys s. window. hiddenPos;
   if ( !WinQueryWindowPos( HANDLE, &swp)) apiErr;
   return ( Point){ swp. x, swp. y};
}

Point
apc_widget_get_size( Handle self)
{
   SWP  swp;
   if ( !WinQueryWindowPos ( HANDLE, &swp)) apiErr;
   if (( swp. fl & SWP_MINIMIZE) && kind_of( self, CWindow))
      return sys s. window. lastFrameSize;
   return ( Point) { swp. cx, swp. cy};
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
      cmd1 = QW_TOP;
      cmd2 = QW_NEXT;
      break;
   case zoLast:
      cmd1 = QW_BOTTOM;
      cmd2 = QW_PREV;
      break;
   case zoNext:
      cmd1 = cmd2 = QW_NEXT;
      break;
   case zoPrev:
      cmd1 = cmd2 = QW_PREV;
      break;
   default:
      apcErr( errInvParams);
      return nilHandle;
   }

   w = WinQueryWindow( HANDLE, cmd1);
   if ( !w) return nilHandle;
   h = hwnd_to_view( w);
   while ( h == nilHandle) {
      w = WinQueryWindow( w, cmd2);
      if ( !w) return nilHandle;
      h = hwnd_to_view( w);
   }

   return h;
}

/* XXX */
Bool
apc_widget_get_shape( Handle self, Handle mask)
{
   return false;
}

Bool
apc_widget_get_sync_paint( Handle self)
{
  return is_apt( aptSyncPaint);
}

Bool
apc_widget_get_transparent( Handle self)
{
   return is_apt( aptTransparent);
}

Bool
apc_widget_is_captured( Handle self)
{
   return WinQueryCapture( HWND_DESKTOP) == HANDLE;
}

Bool
apc_widget_is_enabled ( Handle self)
{
   return is_apt( aptEnabled);
}

Bool
apc_widget_is_responsive ( Handle self)
{
   Bool ena = true;
   while ( ena && self != application) {
      ena  = is_apt( aptEnabled);
      self = var owner;
   }
   return ena;
}

Bool
apc_widget_is_focused ( Handle self)
{
   return is_apt( aptFocused);
}

Bool
apc_widget_is_visible( Handle self)
{
   return is_apt( aptVisible);
}

Bool
apc_widget_is_showing( Handle self)
{
   return WinIsWindowVisible( HANDLE);
}

Bool
apc_widget_is_exposed( Handle self)
{
   return WinIsWindowVisible( HANDLE);
}

Bool
apc_widget_invalidate_rect( Handle self, Rect * rect)
{
   if ( !WinInvalidateRect ( var handle, ( PRECTL) rect, false)) apiErrRet;
   objCheck false;
   process_transparents( self);
   return true;
}

Bool
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
   if ( !WinSetCapture( HWND_DESKTOP, capture ? var handle : nilHandle)) apiErrRet;
   return true;
}

Bool
apc_widget_scroll( Handle self, int horiz, int vert, Rect *r, Rect *cr, Bool scrollChildren)
{
   WinShowCursor( var handle, false);
   if ( !WinScrollWindow( var handle, horiz, vert, (PRECTL)r, (PRECTL)cr, nilHandle, nil,
                    SW_INVALIDATERGN | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
                  )) apiErr;
   return cursor_update( self);
}

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

Bool
apc_widget_set_color( Handle self, Color color, int index)
{
   int idx;
   int oStage = var stage;
   Event ev = {cmColorChanged};

   color = remap_color( nilHandle, color, true);
   if ( index == ciLight3DColor) { sys l3dc = color; return true; }
   if ( index == ciDark3DColor)  { sys d3dc = color; return true; }
   idx = ctx_remap_end( index, ctx_ci2PP, true);
   if ( idx == endCtx) return false;

   var stage = csAxEvents;
   if ( !WinSetPresParam( var handle, idx, sizeof( color), &color)) apiErr;
   var stage = oStage;

   ev. gen. source = self;
   ev. gen. i      = index;
   var self-> message( self, &ev);
   return true;
}

Bool
apc_widget_set_enabled( Handle self, Bool enable)
{
   apt_assign( aptEnabled, enable);
   if (( sys className == WC_FRAME) || ( var owner == application) || !is_apt( aptClipOwner)) {
      if ( !WinEnableWindow( HANDLE, enable)) apiErrRet;
   } else
      WinSendMsg( HANDLE, WM_ENABLE, ( MPARAM) enable, 0);
   return true;
}


Bool
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   apt_assign( aptFirstClick, firstClick);
   return true;
}

Bool
apc_widget_set_focused( Handle self)
{
   if ( self && ( self != Application_map_focus( application, self)))
      return false;
   guts. focSysGranted++;
   if ( self)
      WinFocusChange( HWND_DESKTOP, var handle, is_apt( aptClipOwner) ? 0
          : FC_NOLOSEACTIVE);
   else
      WinFocusChange( HWND_DESKTOP, HWND_DESKTOP, 0);
   guts. focSysGranted--;
   return true;
}

Bool
apc_widget_set_font( Handle self, PFont font)
{
   char buf [256];
   int oStage = var stage;
   if (( var handle == nilHandle) || ( font == nil)) return false;
   font_gp2pp( font, buf);
   var stage = csAxEvents;
   if ( !WinSetPresParam( var handle, PP_FONTNAMESIZE, ( ULONG) strlen( buf)+1, ( PVOID) &buf)) apiErr;
   var stage = oStage;
   WinSendMsg( var handle, WM_FONTCHANGED, 0, 0);

   return true;
}

/* XXX */
Bool
apc_widget_set_palette( Handle self)
{
   return false;
}

Bool
apc_widget_set_pos( Handle self, int x, int y)
{
   HWND h = HANDLE;
   SWP swp;
   if ( sys className == WC_FRAME)
   {
      Bool ret = 0;
      if ( sys s. window. state == wsMinimized) {
         sys s. window. hiddenPos = ( Point){ x, y};
         ret = 1;
      }
      if ( var stage == csConstructing && sys s. window. state != wsNormal)
      {
         if ( !WinSetWindowUShort( h, QWS_XRESTORE, x)) apiErr;
         if ( !WinSetWindowUShort( h, QWS_YRESTORE, y)) apiErrRet;
         return true;
      }
      if ( ret) return true;
   }
   if ( sys parentHandle) {
      POINTL ppos = { x, y};
      WinMapWindowPoints( HWND_DESKTOP, sys parentHandle, &ppos, 1);
      x = ppos. x;
      y = ppos. y;
   } else {
      WinQueryWindowPos( h, &swp);
      if ( x == swp. x && y == swp. y) return true;
   }
   if ( !WinSetWindowPos( h, 0, x, y, 0, 0, SWP_MOVE)) apiErrRet;
   return true;
}

Bool
apc_widget_set_size( Handle self, int width, int height)
{
   if (
        ( sys className == WC_FRAME) && (
           ( var stage == csConstructing && sys s. window. state != wsNormal) ||
           ( sys s. window. state == wsMinimized)
        )
      )
   {
      HWND h = HANDLE;
      if ( !WinSetWindowUShort( h, QWS_CXRESTORE, width)) apiErr;
      if ( !WinSetWindowUShort( h, QWS_CYRESTORE, height)) apiErrRet;
      sys s. window. lastClientSize. x += sys s. window. lastClientSize. x - sys s. window. lastFrameSize. x + width;
      sys s. window. lastClientSize. y += sys s. window. lastClientSize. y - sys s. window. lastFrameSize. y + height;
      sys s. window. lastFrameSize = ( Point){ width, height};
      var virtualSize = sys s. window. hiddenSize = sys s. window. lastClientSize;
   }
   else {
      sys sizeLockLevel++;
      var virtualSize = ( Point) {width, height};
      if ( !WinSetWindowPos( HANDLE, 0, 0, 0, width, height, SWP_SIZE)) apiErr;
      sys sizeLockLevel--;
   }
   return true;
}

Bool
apc_widget_set_rect( Handle self, int x, int y, int width, int height)
{
   if (
        ( sys className == WC_FRAME) && (
           ( var stage == csConstructing && sys s. window. state != wsNormal) ||
           ( sys s. window. state == wsMinimized)
        )
      )
   {
      HWND h = HANDLE;
      if ( sys s. window. state == wsMinimized)
         sys s. window. hiddenPos = ( Point){ x, y};
      if ( !WinSetWindowUShort( h, QWS_XRESTORE, x)) apiErr;
      if ( !WinSetWindowUShort( h, QWS_YRESTORE, y)) apiErrRet;
      if ( !WinSetWindowUShort( h, QWS_CXRESTORE, width)) apiErr;
      if ( !WinSetWindowUShort( h, QWS_CYRESTORE, height)) apiErrRet;
      sys s. window. lastClientSize. x += sys s. window. lastClientSize. x - sys s. window. lastFrameSize. x + width;
      sys s. window. lastClientSize. y += sys s. window. lastClientSize. y - sys s. window. lastFrameSize. y + height;
      sys s. window. lastFrameSize = ( Point){ width, height};
      var virtualSize = sys s. window. hiddenSize = sys s. window. lastClientSize;
   }
   else {
      if ( sys parentHandle) {
         POINTL ppos = { x, y};
         WinMapWindowPoints( HWND_DESKTOP, sys parentHandle, &ppos, 1);
         x = ppos. x;
         y = ppos. y;
      }
      sys sizeLockLevel++;
      var virtualSize = ( Point) {width, height};
      if ( !WinSetWindowPos( HANDLE, 0, x, y, width, height, SWP_SIZE|SWP_MOVE)) apiErr;
      sys sizeLockLevel--;
   }
   return true;
}

Bool
apc_widget_set_size_bounds( Handle self, Point min, Point max)
{
   return true;
}

/* XXX */
Bool
apc_widget_set_shape( Handle self, Handle mask)
{
   return false;
}

Bool
apc_widget_set_visible( Handle self, Bool show)
{
   HWND h = HANDLE;
   if ( show == WinIsWindowVisible( h)) return true;
   if ( !WinSetWindowPos( h, 0, 0, 0, 0, 0, show ? SWP_SHOW : SWP_HIDE)) apiErrRet;
   if ( !is_apt( aptClipOwner)) {
      WinInvalidateRect(( HWND) var handle, nil, false);
      objCheck false;
      process_transparents( self);
   }
   return true;
}

Bool
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
   HWND opt = ( top) ? HWND_TOP : HWND_BOTTOM;
   if ( behind != nilHandle) {
      dobjCheck( behind) false;
      opt = (( PWidget) behind)-> handle;
   }
   if ( !WinSetWindowPos( var handle, opt, 0, 0, 0, 0, SWP_ZORDER)) apiErrRet;
   return true;
}

Bool
apc_widget_update( Handle self)
{
   if ( !WinUpdateWindow( var handle)) apiErrRet;
   return true;
}

Bool
apc_widget_validate_rect ( Handle self, Rect rect)
{
   if ( !WinValidateRect( var handle, ( PRECTL)&rect, true)) apiErrRet;
   return true;
}


// View attributes

// Keyboard

int ctx_kb2VK[] =
{
   kbNoKey       ,   0                 ,
   kbBackspace   ,   VK_BACKSPACE      ,
   kbTab         ,   VK_TAB            ,
   kbBackTab     ,   VK_BACKTAB        ,
   kbPause       ,   VK_PAUSE          ,
   kbEsc         ,   VK_ESC            ,
   kbSpace       ,   VK_SPACE          ,
   kbPgUp        ,   VK_PAGEUP         ,
   kbPgDn        ,   VK_PAGEDOWN       ,
   kbEnd         ,   VK_END            ,
   kbHome        ,   VK_HOME           ,
   kbLeft        ,   VK_LEFT           ,
   kbUp          ,   VK_UP             ,
   kbRight       ,   VK_RIGHT          ,
   kbDown        ,   VK_DOWN           ,
   kbPrintScr    ,   VK_PRINTSCRN      ,
   kbInsert      ,   VK_INSERT         ,
   kbDelete      ,   VK_DELETE         ,
   kbEnter       ,   VK_ENTER          ,
   kbEnter       ,   VK_NEWLINE        ,
   kbSysRq       ,   VK_SYSRQ          ,
   kbScrollLock  ,   VK_SCRLLOCK       ,
   kbNumLock     ,   VK_NUMLOCK        ,
   kbCapsLock    ,   VK_CAPSLOCK       ,
   kbShiftL      ,   VK_SHIFT          ,
   kbCtrlL       ,   VK_CTRL           ,
   kbAltL        ,   VK_ALT            ,
   kbAltR        ,   VK_ALTGRAF        ,
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
   kbF17         ,   VK_F17            ,
   kbF18         ,   VK_F18            ,
   kbF19         ,   VK_F19            ,
   kbF20         ,   VK_F20            ,
   kbF21         ,   VK_F21            ,
   kbF22         ,   VK_F22            ,
   kbF23         ,   VK_F23            ,
   kbF24         ,   VK_F24            ,
   endCtx
};

int
apc_kbd_get_state( Handle self)
{
   return
   (( WinGetKeyState( HWND_DESKTOP, VK_ALT    ) & 0x8000) ? kmAlt    : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_CTRL   ) & 0x8000) ? kmCtrl   : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_SHIFT  ) & 0x8000) ? kmShift  : 0);
}

// Menus

static HWND
add_item( HWND w, Handle menu, PMenuItemReg i)
{
    MENUITEM menuItem;
    HWND m;
    PMenuWndData md;
    PMenuItemReg first;

    if ( i == nil) return nilHandle;
    if ( !( m = WinCreateMenu( w, nil))) {
       apiErr;
       return nilHandle;
    }

    md = malloc( sizeof( MenuWndData));
    if ( !md) {
       WinDestroyWindow( m);
       return nilHandle;
    }
    md-> menu = menu;
    WinSetWindowULong( m, QWL_USER, ( ULONG) md);
    md-> fnwp = WinSubclassWindow( m, ( PFNWP) generic_menu_handler);
    first = i;

    while ( i != nil)
    {
       menuItem. iPosition = MIT_END;
       menuItem. afStyle = 0;
       menuItem. afStyle |= ( i-> flags. divider) ? MIS_SEPARATOR       : 0;
       menuItem. afStyle |= ( i-> down       ) ? MIS_SUBMENU         : 0;
       menuItem. afStyle |= ( i-> bitmap     ) ? MIS_BITMAP          : 0;
       menuItem. afStyle |= ( i-> text       ) ? MIS_TEXT            : 0;
       menuItem. afStyle |= ( i-> flags. rightAdjust) ? MIS_BUTTONSEPARATOR : 0;
       menuItem. afAttribute = 0;
       menuItem. afAttribute |= ( i-> flags. checked)  ? MIA_CHECKED        : 0;
       menuItem. afAttribute |= ( i-> flags. disabled) ? MIA_DISABLED       : 0;
       menuItem. id    = i-> id + MENU_ID_AUTOSTART + 1;
       menuItem. hItem = ( i-> bitmap && PObject( i-> bitmap)-> stage < csDead) ?
          bitmap_make_handle( i-> bitmap) : 0;
       menuItem. hwndSubMenu = add_item( m, menu, i-> down);
       if (!( i-> flags. divider && i-> flags. rightAdjust))
       {
          char buf [ 1024];
          char * t = i-> accel ? buf : i-> text;
          if ( i-> accel) snprintf( buf, 1024, "%s\t%s", i-> text, i-> accel);
          WinSendMsg( m, MM_INSERTITEM, MPFROMP( &menuItem), t);
          if ( i-> variable) i-> id = menuItem. id;
       }
       i = i-> next;
    }
    md-> id = first-> id;
    return m;
}

Bool
apc_menu_create ( Handle self, Handle owner)
{
   objCheck false;
   dobjCheck( owner) false;
   sys className = WC_MENU;
   sys owner = DHANDLE( owner);
   return true;
}

Bool
apc_menu_destroy( Handle self)
{
   if ( var handle && WinIsWindow( guts. anchor, var handle))
      if ( !WinDestroyWindow( var handle)) apiErrRet;
   return true;
}

PFont
apc_menu_default_font( PFont font)
{
   char buf  [256];
   PrfQueryProfileString( HINI_USERPROFILE, "PM_SystemFonts", "Menus", DEFAULT_FONT_FACE, buf, 255);
   font_pp2gp( buf, font);
   apc_font_pick( nilHandle, font, font);
   return font;
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
   char buf  [256];
   if ( WinQueryPresParam( var handle, PP_FONTNAMESIZE, 0, nil, 256,
        &buf, QPF_NOINHERIT) && strchr( buf, '.'))
      {
         font_pp2gp( buf, font);
         apc_font_pick( nilHandle, font, font);
      } else apc_menu_default_font( font);
   return font;
}

static int ctx_ci2PP_MENU[] =
{
   ciNormalText     , PP_MENUFOREGROUNDCOLOR,
   ciNormal         , PP_MENUBACKGROUNDCOLOR,
   ciHiliteText     , PP_MENUHILITEFGNDCOLOR,
   ciHilite         , PP_MENUHILITEBGNDCOLOR,
   ciDisabledText   , PP_MENUDISABLEDFGNDCOLOR,
   ciDisabled       , PP_MENUDISABLEDBGNDCOLOR,
   endCtx,
};

Color
apc_menu_get_color( Handle self, int index)
{
   long p = 0;
   if ( index == ciLight3DColor) return WinQuerySysColor( HWND_DESKTOP, SYSCLR_BUTTONLIGHT, 0);
   if ( index == ciDark3DColor)  return WinQuerySysColor( HWND_DESKTOP, SYSCLR_BUTTONDARK, 0);
   return WinQueryPresParam( var handle, ctx_remap( index, ctx_ci2PP_MENU, true), 0, nil, sizeof( long), &p, QPF_NOINHERIT) ?
     p : clInvalid;
}

Bool
apc_menu_set_color( Handle self, Color color, int index)
{
   if ( index == ciLight3DColor) return true;
   if ( index == ciDark3DColor)  return true;
   index = ctx_remap_end( index, ctx_ci2PP_MENU, true);
   if (( var handle == nilHandle) || ( index == endCtx)) return false;
   color = remap_color( nilHandle, color, true);
   if ( !WinSetPresParam( var handle, index, sizeof( color), &color)) apiErrRet;
   return true;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
   char buf [256];
   if (( var handle == nilHandle) || ( font == nilHandle)) return false;
   font_gp2pp( font, buf);
   if ( !WinSetPresParam( var handle, PP_FONTNAMESIZE, ( ULONG) strlen( buf)+1, ( PVOID) &buf)) apiErrRet;
   return true;
}

Bool
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
   PWindow owner = nil;
   Point size;
   Bool resize;

   if (!var handle) return false;
   objCheck false;
   dobjCheck( var owner) false;

   resize = kind_of( var owner, CWindow) &&
            kind_of( self, CMenu) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self);
   if ( resize)
   {
      owner = ( PWindow) var owner;
      size = owner-> self-> get_size( var owner);
   }
   WinSendMsg( var handle, MM_DELETEITEM, MPFROM2SHORT( m->id, true), 0);
   if ( resize) owner-> self-> set_size( var owner, size);
   return true;
}

Bool
apc_menu_item_set_accel( Handle self, PMenuItemReg m)
{
   char buf [ 1024];
   char * t = m-> accel ? buf : m->text;
   if (!var handle) return false;
   if ( m-> accel) snprintf( buf, 1024, "%s\t%s", m->text, m-> accel);
   WinSendMsg( var handle, MM_SETITEMTEXT, MPFROM2SHORT( m->id, true), ( MPARAM) t);
   return true;
}


Bool
apc_menu_item_set_check ( Handle self, PMenuItemReg m)
{
   if (!var handle) return false;
   WinSendMsg( var handle,
      MM_SETITEMATTR, MPFROM2SHORT( m->id, true),
      MPFROM2SHORT( MIA_CHECKED, m-> flags. checked ? MIA_CHECKED : false));
   return true;
}

Bool
apc_menu_item_set_enabled( Handle self, PMenuItemReg m)
{
   if (!var handle) return false;
   WinSendMsg( var handle,
      MM_SETITEMATTR, MPFROM2SHORT( m->id, true),
      MPFROM2SHORT( MIA_DISABLED, m-> flags. disabled ? MIA_DISABLED : false));
   return true;
}

Bool
apc_menu_item_set_text( Handle self, PMenuItemReg m)
{
   char buf [ 1024];
   char * t = m-> accel ? buf : m-> text;

   if (!var handle) return false;
   if ( m-> accel) snprintf( buf, 1024, "%s\t%s", m-> text, m-> accel);

   WinSendMsg( var handle, MM_SETITEMTEXT, MPFROM2SHORT( m->id, true), ( MPARAM) t);
   return true;
}

Bool
apc_menu_item_set_key( Handle self, PMenuItemReg m)
{
   return true;
}

/* XXX - fails to set MIS_BITMAP from MIS_TEXT */
Bool
apc_menu_item_set_image( Handle self, PMenuItemReg m)
{
   MENUITEM mi;
   if ( PObject( m-> bitmap)-> stage == csDead) return false;
   if ( !var handle) return false;
   if ( !WinSendMsg( var handle, MM_QUERYITEM, MPFROM2SHORT( m-> id, true), &mi)) apiErr;
   mi. afStyle = ( mi. afStyle & ~MIS_TEXT) | MIS_BITMAP;
   mi. hItem   = m-> bitmap ? bitmap_make_handle( m-> bitmap) : 0;
   if ( !WinSendMsg( var handle, MM_SETITEM, MPFROM2SHORT( m-> id, true), &mi)) apiErr;
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
   HWND att;
   char buf[256];
   int  qppOk = 0;
   objCheck false;
   dobjCheck( var owner) false;

   att = kind_of( self, CMenu) ? DHANDLE( var owner) : HWND_OBJECT;
   if ( kind_of( var owner, CWindow) &&
        kind_of( self, CMenu) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self)) {
      Point size = CWindow( var owner)-> get_size( var owner);
      if ( var handle) {
         if ( !WinQueryPresParam( var handle, PP_FONTNAMESIZE, 0, nil, 255, &buf, QPF_NOINHERIT))
           apiErr
         else
           qppOk = 1;
         WinDestroyWindow( var handle);
      }
      var handle = ( Handle) add_item( att, self, (( PMenu) self)-> tree);
      if ( qppOk && !WinSetPresParam( var handle, PP_FONTNAMESIZE, ( ULONG) strlen( buf) + 1, ( PVOID) &buf))
         apiErr;
      WinSendMsg( DHANDLE( var owner), WM_UPDATEFRAME, MPFROMLONG( FCF_MENU), 0);
      CWindow( var owner)-> set_size( var owner, size);
   } else {
      if ( var handle)
         WinDestroyWindow( var handle);
      var handle = ( Handle) add_item( att, self, (( PMenu) self)-> tree);
   }
   return true;
}


Bool
apc_popup_create ( Handle self, Handle owner)
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
   HWND h  = (( PWidget) var owner)-> handle;
   Bool rc = WinPopupMenu( h, h, var handle, x, y, 0,
      PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_MOUSEBUTTON3);
   if ( !rc) apiErr;
   return rc;
}

// Menus end

// Messages
typedef MRESULT (*ApiMessageSender)(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
Bool
apc_message( Handle self, PEvent ev, Bool post)
{
   ULONG msg;
   ApiMessageSender sender = post ? (ApiMessageSender) WinPostMsg : (ApiMessageSender) WinSendMsg;
   switch ( ev-> cmd)
   {
       case cmPost:
          sender( var handle, WM_POSTAL, ( MPARAM) ev-> gen. H, ( MPARAM) ev-> gen. p);
          break;
       case cmMouseMove:
          msg = WM_MOUSEMOVE;
          goto general;
       case cmMouseUp:
          if ( ev-> pos. button & mbMiddle) msg = WM_BUTTON3UP; else
          if ( ev-> pos. button & mbRight)  msg = WM_BUTTON2UP; else
          msg = WM_BUTTON1UP;
          goto general;
       case cmMouseDown:
          if ( ev-> pos. button & mbMiddle) msg = WM_BUTTON3DOWN; else
          if ( ev-> pos. button & mbRight)  msg = WM_BUTTON2DOWN; else
          msg = WM_BUTTON1DOWN;
          goto general;
       case cmMouseClick:
          if ( ev-> pos. dblclk)
          {
             if ( ev-> pos. button & mbMiddle) msg = WM_BUTTON3DBLCLK; else
             if ( ev-> pos. button & mbRight)  msg = WM_BUTTON2DBLCLK; else
             msg = WM_BUTTON1DBLCLK;
          } else {
             if ( ev-> pos. button & mbMiddle) msg = WM_BUTTON3CLICK; else
             if ( ev-> pos. button & mbRight)  msg = WM_BUTTON2CLICK; else
             msg = WM_BUTTON1CLICK;
          }
       general:
          {
             MPARAM mp2, mp1 = MPFROM2SHORT( ev-> pos. where. x, ev-> pos. where. y);
             short mod = 0;
             if ( ev-> pos. mod & kmShift) mod |= KC_SHIFT;
             if ( ev-> pos. mod & kmAlt  ) mod |= KC_ALT;
             if ( ev-> pos. mod & kmCtrl ) mod |= KC_CTRL;
             mp2 = MPFROM2SHORT( HT_NORMAL, mod);
             sender( HANDLE, msg, mp1, mp2);
          }
          break;
       case cmKeyDown:
       case cmKeyUp:
          {
             short m1s1=0;
             CHAR m1c1 = 1, m1c2 = 0;
             MPARAM mp2 = MPFROM2SHORT( ev-> key. code,
                ctx_remap( ev-> key. key, ctx_kb2VK, true));
             if ( ev-> cmd == cmKeyUp) m1s1             |= KC_KEYUP;
             if ( ev-> key. mod & kmShift) m1s1         |= KC_SHIFT;
             if ( ev-> key. mod & kmAlt  ) m1s1         |= KC_ALT;
             if ( ev-> key. mod & kmCtrl ) m1s1         |= KC_CTRL;
             if ( ev-> key. code) m1s1                  |= KC_CHAR;
             if (( ev-> key. key != kbNoKey) || ev-> key. mod)  m1s1 |= KC_VIRTUALKEY;
             if ( ev-> key. mod & ~kmShift && ev-> key. code)  m1s1 &= ~KC_CHAR;
             m1c1 = ev-> key. repeat;
             sender( HANDLE, WM_CHAR, MPFROMSH2CH( m1s1, m1c1, m1c2), mp2);
          }
          break;
   }
   return true;
}

Bool
apc_show_message( const char * message, Bool utf8)
{
   char title [256] = {0};
   if ( guts. anchor && guts. queue) {
      if ( WinQueryTaskTitle( 0, title, 256) != 0) apiErr;
      if ( WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, message, title, 0, MB_OK | MB_MOVEABLE) == MBID_ERROR) apiErrRet;
      return true;
   } else if ( guts. appTypePM) {
      int        i;
      USHORT     vioOptions = VP_WAIT | VP_OPAQUE;
      KBDKEYINFO key;
      BYTE       attr     = 0x07;
      BYTE       fill[ 2] = {' ', attr};

      VioPopUp( &vioOptions, 0);
      for ( i = 1; i < 25; i++) VioWrtNCell( fill, 80, i, 0, 0);
      fill[ 1] = 0x38;
      VioWrtNCell( fill, 80, 0, 0, 0);
      VioWrtCharStrAtt( message, strlen( message), 12, (80 - strlen( message)) / 2, &attr, 0);
      attr = fill[1];
      VioWrtCharStrAtt( "Prima", 5, 0, 37, &attr, 0);
      for ( i = 0; i < 4; i++) { DosBeep( 4500, 40); DosSleep( 80); }
      attr = 0x20;
      VioWrtNAttr( &attr, 80, 0, 0, 0);
      KbdCharIn( &key, IO_WAIT, 0);
      VioEndPopUp(0);
   } else {
      fprintf( stderr, "%s\n", message);
   }
   return true;
}

// Messages end
