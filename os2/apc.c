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
#include "DeviceBitmap.h"
#include "Component.h"
#include "Widget.h"
#include "Menu.h"
#include "Window.h"
#include "Img.h"
#include "Image.h"
#include "Icon.h"
#include "Timer.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)

typedef struct _BInfo
{
   unsigned long structLength;
   unsigned long w;
   unsigned long h;
   unsigned short planes;  // always 1
   unsigned short bpp;
   RGB2 palette[ 0];
} BInfo, *PBInfo;

typedef struct _BInfo2
{
   unsigned long structLength;
   unsigned long w;
   unsigned long h;
   unsigned short planes;  // always 1
   unsigned short bpp;
   RGB2 palette[ 0x100];
} BInfo2, *PBInfo2;


// ----- Application
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

void
apc_application_destroy( Handle self)
{
   if ( WinIsWindow( guts. anchor, var handle)) {
      if ( !WinStopTimer( guts. anchor, var handle, TID_USERMAX - 1)) apiErr;
      if ( !WinDestroyWindow( WinQueryWindow( var handle, QW_PARENT))) apiErr;
   }
   if ( !loggerDead) WinPostMsg( guts. logger, WM_QUIT, MPVOID, MPVOID);
   WinPostQueueMsg( guts. queue, WM_QUIT, MPVOID, MPVOID);
   loggerDead = true;
}

void
apc_application_close( Handle self)
{
   if ( !WinPostMsg( guts. logger, WM_CLOSE, 0, 0)) apiErr;
}

static void
save_gp_attrs( Handle self)
{
   if ( sys psd == nil) sys psd = malloc( sizeof( PaintSaveData));
   apc_gp_set_text_opaque( self, sys textOpaque);
   apc_gp_set_line_width( self, sys lineWidth);
   apc_gp_set_line_end( self, sys lineEnd);
   apc_gp_set_line_pattern( self, sys linePattern);
   apc_gp_set_rop( self, sys rop);
   apc_gp_set_rop2( self, sys rop2);
   apc_gp_set_transform( self, sys transform. x, sys transform. y);
   apc_gp_set_fill_pattern( self, sys fillPattern2);
   sys psd-> font        = var font;
   sys psd-> lineWidth   = sys lineWidth;
   sys psd-> lineEnd     = sys lineEnd;
   sys psd-> linePattern = sys linePattern;
   sys psd-> rop         = sys rop;
   sys psd-> rop2        = sys rop2;
   sys psd-> transform   = sys transform;
   sys psd-> textOpaque  = sys textOpaque;
}

static void
restore_gp_attrs( Handle self)
{
   if ( sys psd == nil) return;
   var font        = sys psd-> font;
   sys lineWidth   = sys psd-> lineWidth;
   sys lineEnd     = sys psd-> lineEnd;
   sys linePattern = sys psd-> linePattern;
   sys rop         = sys psd-> rop;
   sys rop2        = sys psd-> rop2;
   sys transform   = sys psd-> transform;
   sys textOpaque  = sys psd-> textOpaque;
   free( sys psd);
   sys psd = nil;
}

Bool
apc_application_begin_paint ( Handle self)
{
   if ( !( sys ps = WinGetScreenPS( HWND_DESKTOP)))
        apiErrRet;
   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil))
        apiErr;
   list_add( &guts. winPsList, sys ps);
   sys fontId = 0;
   sys fontHash = create_fontid_hash();
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   apc_gp_set_color( self, var self-> get_color( self));
   apc_gp_set_back_color( self, var self-> get_back_color( self));
   apt_clear( aptFontExists);
   apc_gp_set_font( self, &var font);
   save_gp_attrs( self);
   return true;
}

void
apc_application_end_paint( Handle self)
{
   if ( !WinReleasePS( sys ps)) apiErr;
   list_delete( &guts. winPsList, sys ps);
   if ( sys fillBitmap)
      if ( !GpiDeleteBitmap( sys fillBitmap)) apiErr;
   sys ps = sys fillBitmap = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptCompatiblePS);
   apt_clear( aptFontExists);
   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   restore_gp_attrs( self);
}


void
apc_application_go( Handle self)
{
   QMSG message;

   while ( WinGetMsg( guts. anchor, &message, 0, 0, 0))
   {
      if ( message .msg == WM_TERMINATE) break;
      WinDispatchMsg( guts. anchor, &message);
      kill_zombies();
   }
   if ( application) Object_destroy( application);
}

void
apc_application_yield( void)
{
   QMSG message;

   while ( WinPeekMsg( guts. anchor, &message, 0, 0, 0, PM_REMOVE))
   {
      switch( message. msg)
      {
        case WM_QUIT:
        case WM_TERMINATE:
           if ( !loggerDead && message. hwnd)
              WinSendMsg( message. hwnd, WM_CLOSE, MPVOID, MPVOID);
           break;
        default:
           WinDispatchMsg( guts. anchor, &message);
           kill_zombies();
      }
   }
}


Point
apc_application_get_size( Handle self)
{
   return ( Point) {
      WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN),
      WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN)
   };
}

static void
lock( Bool lock)
{
   if ( lock)
   {
      if ( guts. appLock++ == 0) WinLockWindowUpdate( HWND_DESKTOP, HWND_DESKTOP);
   }
   else
   {
      if ( --guts. appLock == 0) WinLockWindowUpdate( HWND_DESKTOP, nilHandle);
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

int
apc_application_get_os_info( char * system, char * release, char * vendor, char * arch)
{
   ULONG si[ 2];
   int version, subVersion;
   DosQuerySysInfo( QSV_VERSION_MAJOR, QSV_VERSION_MINOR, &si, sizeof( si));
   version    = ( si[ 1] >= 30) ? ( si[ 1] / 10) : ( si[ 0] / 10);
   subVersion = ( si[ 1] < 30)  ? si[ 1] : 0;
   if ( system ) strcpy( system, "OS/2");
   if ( vendor ) strcpy( vendor, "IBM");
   if ( arch   ) strcpy( arch  , "i386");
   if ( release) sprintf( release, "%d.%d", version, subVersion);
   return apcOS2;
}

int
apc_application_get_gui_info( char * description)
{
   strcpy( description, "Presentation Manager");
   return guiPM;
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
apc_application_get_view_from_point( Handle self, Point point)
{
   return hwnd_to_view( WinWindowFromPoint( HWND_DESKTOP, ( PPOINTL) &point, 1));
}


// COMPONENT

void
apc_component_create( Handle self)
{
   PComponent c = ( PComponent) self;
   PDrawableData d = c-> sysData;

   if ( d) return;
   d = malloc( sizeof( DrawableData));
   memset( d, 0, sizeof( DrawableData));
   c-> sysData = d;
   return;
}

void
apc_component_destroy( Handle self)
{
   PComponent    c = ( PComponent) self;
   PDrawableData d = c-> sysData;
   var handle = nilHandle;
   if ( d == nil) return;
   free( d);
   c-> sysData = nil;
}


// COMPONENT END

typedef struct _ViewProfile {
  ColorSet colors;
  Point    pos;
  Point    size;
  Bool     enabled;
  Bool     visible;
  Bool     focused;
  Bool     capture;
} ViewProfile, *PViewProfile;

static void
get_view_ex( Handle self, PViewProfile p)
{
  int i;
  p-> capture   = apc_widget_is_captured( self);
  for ( i = 0; i <= ciMaxId; i++) p-> colors[ i] = apc_widget_get_color( self, i);
  p-> pos       = apc_widget_get_pos( self);
  p-> size      = apc_widget_get_size( self);
  p-> enabled   = apc_widget_is_enabled( self);
  p-> focused   = apc_widget_is_focused( self);
  p-> visible   = apc_widget_is_visible( self);
}

static void
set_view_ex( Handle self, PViewProfile p)
{
  int i;
  HWND wnd = var handle;
  apc_widget_set_visible( self, false);
  for ( i = 0; i <= ciMaxId; i++) apc_widget_set_color( self, p-> colors[i], i);
  apc_widget_set_font( self, &var font);
  apc_widget_set_pos( self, p-> pos. x, p-> pos. y);
  apc_widget_set_size( self, p-> size. x, p-> size. y);
  apc_widget_set_enabled( self, p-> enabled);
  if ( p-> focused) apc_widget_set_focused( self);
  apc_widget_set_visible( self, p-> visible);
  if ( p-> capture) apc_widget_set_capture( self, 1);
  if ( sys timeDefs) for ( i = 0; i < sys timeDefsCount; i++)
     if ( sys timeDefs[ i]. item)
     {
        Handle xs   = ( Handle) sys timeDefs[ i]. item;
        Handle self = xs;
        if ( sys s. timer. active)
           if ( !WinStartTimer ( guts. anchor, wnd, var handle, sys s. timer. timeout)) apiErr;
     }
   if ( !WinInvalidateRect ( var handle, nil, true)) apiErr;
}

// ----- WINDOW

static Bool repost_msgs( PostMsg * msg, Handle self)
{
   WinPostMsg( var handle, WM_POSTAL, ( MPARAM) self, ( MPARAM) msg);
   return false;
}

static Bool
create_group( Handle self, Handle owner, Bool syncPaint, Bool clipOwner, PSZ className, ULONG style)
{
   HWND ret;
   HWND old = HANDLE;
   HWND behind = HWND_TOP;
   HWND ownerView  = (( PWidget) owner)-> handle;
   HWND parentView = (( PDrawableData)(( PComponent) owner)-> sysData)-> parent;
   int  count = 0;
   Bool reset = false;
   Handle * list = nil;

   if ( HANDLE)
   {
      if (( DHANDLE( owner) == sys owner) && ( clipOwner == is_apt( aptClipOwner)))
      {
         char buf[256];
         behind = WinQueryWindow( HANDLE, QW_PREV);
         if ( behind == nilHandle) behind = HWND_TOP;
         WinQueryWindowText( behind, 256, buf);
      }
      var stage = csAxEvents;
      if ( kind_of( self, CWidget))
      {
         PWidget g = ( PWidget) self;
         list  = g-> controls. items;
         count = g-> controls. count;
      }
      var self-> end_paint_info( self);
      var self-> end_paint( self);
      reset = true;
   }

   switch (( int) className)
   {
     case WC_FRAME:
       {
          Handle frame;
          if ( !clipOwner) parentView = HWND_DESKTOP;
          if ( !( frame = WinCreateStdWindow( parentView, 0, &style, "GeNeRiC",
            "", WS_CLIPCHILDREN
            | ( syncPaint ? WS_SYNCPAINT : 0)
            , nilHandle, 0, &ret)))
               apiErrRet;
          WinSetWindowULong( frame, QWL_USER, ( ApiHandle) WinSubclassWindow( frame, ( PFNWP) generic_frame_handler));
          if ( !WinSetWindowPos( frame, behind, 0, 0, 0, 0, SWP_ZORDER | SWP_NOADJUST))
             apiErr;
          if ( firstModal) if ( !WinSetWindowPos( frame, DHANDLE(firstModal),0,0,0,0,SWP_ZORDER))
             apiErr;
       }
       break;
    case WC_CUSTOM:
       if ( !clipOwner) parentView = HWND_DESKTOP;
       if ( !( ret = WinCreateWindow( parentView, "GeNeRiC", "",
          style |  WS_CLIPCHILDREN
          | WS_CLIPSIBLINGS
          | ( syncPaint ? WS_SYNCPAINT : 0)
          , 0, 0, 0, 0, ownerView,
          behind, 0, nil, nil)))
            apiErrRet;
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
      for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
      if ( kind_of( self, CWindow))
      {
         PAbstractMenu menu = ( PAbstractMenu)((( PWindow) self)-> menu);
         if ( menu) menu-> self-> set_selected(( Handle) menu, true);
      }
      if ( !WinDestroyWindow( old))
         apiErr;
      if ( var postList) list_first_that( var postList, repost_msgs, ( void*)self);
   }
   WinPostMsg( ret, WM_PRIMA_CREATE, 0, 0);
   return true;
}


Bool
apc_window_create( Handle self, Handle owner, Bool syncPaint, int borderIcons, int borderStyle, Bool taskList, int windowState)
{
   Bool reset = false;
   ViewProfile vprf;
   int oStage = var stage;
   WindowData ws;
   Rect maxRS;
   ULONG frameFlags = FCF_NOBYTEALIGN
      | (( borderIcons &  biSystemMenu) ? FCF_SYSMENU    : 0)
      | (( borderIcons &  biMinimize)   ? FCF_MINBUTTON  : 0)
      | (( borderIcons &  biMaximize)   ? FCF_MAXBUTTON  : 0)
      | (( borderIcons &  biTitleBar)   ? FCF_TITLEBAR   : 0)
      | (( borderStyle == bsSizeable)   ? FCF_SIZEBORDER : 0)
      | (( borderStyle == bsSingle  )   ? FCF_BORDER     : 0)
      | (( borderStyle == bsDialog  )   ? FCF_DLGBORDER  : 0)
      | (  taskList                     ? FCF_TASKLIST   : 0)
   ;

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
      get_view_ex( self, &vprf);
      ws = sys s. window;
      maxRS. left   = WinQueryWindowUShort( h, QWS_XRESTORE);
      maxRS. bottom = WinQueryWindowUShort( h, QWS_YRESTORE);
      maxRS. right  = WinQueryWindowUShort( h, QWS_CXRESTORE);
      maxRS. top    = WinQueryWindowUShort( h, QWS_CYRESTORE);
      reset = true;
   }
   if ( reset || ( var handle == nilHandle))
      if ( !create_group( self, owner, syncPaint, 0, WC_FRAME, frameFlags)) {
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
      set_view_ex( self, &vprf);
      sys s. window = ws;
      WinSetWindowUShort( h, QWS_XRESTORE,  maxRS. left);
      WinSetWindowUShort( h, QWS_YRESTORE,  maxRS. bottom);
      WinSetWindowUShort( h, QWS_CXRESTORE, maxRS. right);
      WinSetWindowUShort( h, QWS_CYRESTORE, maxRS. top);
      var stage = oStage;
   }
   else
      sys s. window. modalResult = -1;
   apc_window_set_caption( self, var text);
   lock( false);
   return apcError == 0;
}

Bool
apc_window_close( Handle self)
{
   if ( !WinPostMsg( HANDLE, WM_CLOSE, 0, 0)) apiErrRet;
   return true;
}

void
apc_window_activate( Handle self)
{
   if ( self)
      if ( !WinSetActiveWindow( HWND_DESKTOP, HANDLE)) apiErr;
}

Bool
apc_window_is_active( Handle self)
{
   return WinQueryActiveWindow( HWND_DESKTOP) == HANDLE;
}

Point
frame2client( Handle self, Point p, Bool f2c)
{
   RECTL r = {0,0,p.x,p.y};
   WinCalcFrameRect( HANDLE, &r, f2c);
   return ( Point){ r. xRight - r. xLeft,  r. yTop - r. yBottom};
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

Point
apc_window_get_client_pos( Handle self)
{
   SWP  swp;
   Point d = apc_sys_get_window_borders( sys s. window. borderStyle);
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

int
apc_window_get_border_icons ( Handle self)
{
   return sys s. window. borderIcons;
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
apc_window_get_task_listed( Handle self)
{
   return is_apt( aptTaskList);
}

int
apc_window_get_border_style ( Handle self)
{
   return sys s. window. borderStyle;
}

void
apc_window_set_caption( Handle self, char * caption)
{
   if ( caption == nil || strlen( caption) == 0) caption = " ";
   if ( !WinSetWindowText( HANDLE, caption)) apiErr;
}

void
apc_window_set_client_size  ( Handle self, int x, int y)
{
   if ( sys s. window. state == wsMinimized)
   {
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
         if ( !WinSetWindowUShort( h, QWS_CXRESTORE, p. x)) apiErr;
         if ( !WinSetWindowUShort( h, QWS_CYRESTORE, p. y)) apiErr;
      } else
         if ( !WinSetWindowPos( HANDLE, 0, 0, 0, p. x, p. y, SWP_SIZE)) apiErr;
   }
}

void
apc_window_set_client_pos  ( Handle self, int x, int y)
{
   Point d = apc_sys_get_window_borders( sys s. window. borderStyle);
   if ( sys s. window. state == wsMinimized)
   {
      sys s. window. hiddenPos = ( Point){x - d.x, y - d.y};
   } else {
      HWND h = HANDLE;
      if ( var stage == csConstructing && sys s. window. state != wsNormal)
      {
         if ( !WinSetWindowUShort( h, QWS_XRESTORE, x - d.x)) apiErr;
         if ( !WinSetWindowUShort( h, QWS_YRESTORE, y - d.y)) apiErr;
      } else
         if ( !WinSetWindowPos( h, 0, x - d.x, y - d.y, 0, 0, SWP_MOVE)) apiErr;
   }
}

void
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
      int i, lock = sys lockState;
      while ( sys lockState) apc_widget_unlock( self);
      if ( !WinSetWindowPos( HANDLE, 0, 0, 0, 0, 0, fl)) apiErr;
      for ( i = 0; i < lock; i++) apc_widget_lock( self);
      sys s. window. state = state;
   }
}

static HPOINTER make_pointer_handle( Handle self, Handle icon, Point hotSpot);


void
apc_window_set_icon( Handle self, Handle icon)
{
   HPOINTER p = icon ? make_pointer_handle( self, icon, ( Point){0,0}) : nilHandle;
   WinSendMsg( HANDLE, WM_SETICON, MPFROMP( p), 0);
}

Bool
apc_window_get_icon( Handle self, Handle icon)
{
   HPOINTER p = ( HPOINTER) WinSendMsg( HANDLE, WM_QUERYICON, 0, 0);
   HPOINTER save = sys pointer;
   Bool ret;
   if ( p    == nilHandle) apcErrRet( errInvWindowIcon);
   if ( icon == nilHandle) apcErrRet( errInvParams);
   sys pointer = p;
   ret = apc_pointer_get_bitmap( self, icon);
   sys pointer = save;
   return ret;
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
   if ( wMenu) WinDestroyWindow( wMenu);
   if ( menu)
      (( PComponent) menu)-> handle = add_item( HANDLE, menu, (( PMenu) menu)-> tree);
   if ( var stage <= csNormal)
   {
      WinSendMsg( HANDLE, WM_UPDATEFRAME, MPFROMLONG( FCF_MENU), 0);
      var self-> set_size( self, size.x, size. y);
   }
   WinSetFocus( HWND_DESKTOP, focus);
   return apcError == 0;
}


extern Bool dlgActive;
extern Bool focusAction;

int
apc_window_execute ( Handle self)
{
   SWP swp;
   int ret = cmCancel;
   int fl = SWP_ZORDER;
   Handle top = dlgModal;
   HWND  lastFoc;

   apcErrClear;
   // checking whether given window is already in modal state
   if ( firstModal)
   {
      Handle w = firstModal;
      while ( w)
      {
         if ( w == self) return cmCancel;
         w = dsys( w) s. window. nextModal;
      }
   }
   if ( top) {
      protect_object( top);
      lastFoc = WinQueryWindowULong( DHANDLE( top), QWL_HWNDFOCUSSAVE);
      if ( !lastFoc) lastFoc = DHANDLE( top);
      if ( !WinSetWindowULong( DHANDLE( top), QWL_HWNDFOCUSSAVE, nilHandle))
         apiErr;
   }

   // deactivating possible auto modal actions
   focusAction = 1;
   dlgActive = 1;
   dlgModal = nilHandle;
   if ( top) WinSendMsg( DHANDLE( top), WM_ACTIVATE, 0, 0);
   sys s. window. modalResult = cmCancel;
   // checking min/hidden position
   WinQueryWindowPos( HANDLE, &swp);
   if ( swp. fl & ( SWP_HIDE | SWP_MINIMIZE))
   {
      fl |= (( swp. fl & SWP_HIDE)     ? SWP_SHOW    : 0) |
            (( swp. fl & SWP_MINIMIZE) ? SWP_RESTORE : 0);
   }

   // bringing window to top
   if ( !WinSetWindowPos( HANDLE, HWND_TOP, 0, 0, 0, 0, fl | SWP_ACTIVATE))
      apiErr;

   // if ( top) WinFocusChange( HWND_DESKTOP, lastFoc, FC_NOSETACTIVE | FC_NOLOSEACTIVE);
   // intercepting chain
   dlgModal = top;
   if ( firstModal) dsys( dlgModal) s. window. nextModal = self;
      else firstModal = self;
   dlgModal = self;
   // posting initial message
   if ( !WinPostMsg( HANDLE, WM_DLGENTERMODAL, 0, 0))
      apiErr;
   focusAction = 0;
   // message loop
   {
      QMSG message;
      while ( WinGetMsg( guts. anchor, &message, 0, 0, 0))
      {
         if ( message. msg == WM_BREAKMSGLOOP || message. msg == WM_TERMINATE)
            break;
         WinDispatchMsg( guts. anchor, &message);
         kill_zombies();
      }
      if ( message. msg == WM_QUIT || message. msg == WM_TERMINATE)
         WinPostMsg( 0, message. msg, message. mp1, message. mp2);
   }
   // hiding window
   focusAction = 1;
   dlgModal = nilHandle;
   if ( var handle)
   {
      if ( !WinSetWindowPos( HANDLE, 0, 0, 0, 0, 0, SWP_HIDE))
         apiErr;
      sys s. window. nextModal = nilHandle;
      ret = sys s. window. modalResult;
      sys s. window. modalResult = -1;       // means in non-modal state
   }
   // releasing chain
   if ( self == firstModal) firstModal = nilHandle;
   // old dialog exists, bring it to top
   focusAction = 0;
   dlgModal = top;
   if ( top && (( PWidget) top)-> handle)
   {
      dsys( top) s. window. nextModal = nilHandle;
      if ( !WinSetWindowPos( DHANDLE( top), HWND_TOP, 0, 0, 0, 0, SWP_ZORDER | SWP_ACTIVATE))
         apiErr;
      if ( lastFoc) if ( !WinSetFocus( HWND_DESKTOP, lastFoc)) apiErr;
   }
   else
   {
      Handle h = firstModal;
      while ( h && dsys( h) s. window. nextModal) h = dsys( h) s. window. nextModal;
      if ( h)
      {
         dlgModal = h;
         if ( !WinSetWindowPos( DHANDLE( h), HWND_TOP, 0, 0, 0, 0, SWP_ZORDER | SWP_ACTIVATE))
            apiErr;
      }
      else
      {
         Handle h = nilHandle;
         HWND z;
         dlgModal = nilHandle;
         z = WinQueryWindow( HWND_DESKTOP, QW_TOP);
         while ( z)
         {
            h = hwnd_to_view( z);
            if (h)
            {
               SWP swp;
               WinQueryWindowPos( z, &swp);
               if (( swp. fl & (SWP_HIDE|SWP_MINIMIZE)) == 0)
                  break;
            }
            z = WinQueryWindow( z, QW_NEXT);
         }
         if ( h)
            if ( !WinSetWindowPos( z, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER | SWP_ACTIVATE)) apiErr;
      }
   }
   if ( top) unprotect_object( top);

   return ret;
}

void
apc_window_end_modal( Handle self, int cmd)
{
   if ( !self || self != dlgModal) return;
   sys s. window. modalResult = cmd;
   if ( !WinPostMsg( 0, WM_BREAKMSGLOOP, 0, 0)) apiErr;
}

// WINDOW END

#define stdDisabled  SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW
#define stdHilite    SYSCLR_HILITEFOREGROUND, SYSCLR_HILITEBACKGROUND
#define std3d        SYSCLR_BUTTONLIGHT     , SYSCLR_BUTTONDARK

static int buttonScheme[] = {
   SYSCLR_MENUTEXT, SYSCLR_BUTTONMIDDLE,
   SYSCLR_MENUTEXT, SYSCLR_BUTTONMIDDLE,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_BUTTONMIDDLE,
   std3d
};
static int sliderScheme[] = {
   SYSCLR_WINDOWTEXT,       SYSCLR_SCROLLBAR,
   SYSCLR_WINDOWSTATICTEXT, SYSCLR_SCROLLBAR,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW,
   std3d
};

static int dialogScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_DIALOGBACKGROUND,
   SYSCLR_ACTIVETITLETEXT, SYSCLR_ACTIVETITLETEXTBGND,
   SYSCLR_INACTIVETITLETEXT, SYSCLR_INACTIVETITLETEXTBGND,
   std3d
};
static int staticScheme[] = {
   SYSCLR_WINDOWSTATICTEXT, SYSCLR_WINDOW,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW,
   std3d
};
static int editScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_ENTRYFIELD,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_ENTRYFIELD,
   std3d
};
static int menuScheme[] = {
   SYSCLR_MENUTEXT, SYSCLR_MENU,
   SYSCLR_MENUHILITE, SYSCLR_MENUHILITEBGND,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_MENU,
   std3d
};

static int scrollScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_SCROLLBAR,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_FIELDBACKGROUND,
   std3d
};

static int windowScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_WINDOW,
   SYSCLR_ACTIVETITLETEXT, SYSCLR_ACTIVETITLETEXTBGND,
   SYSCLR_INACTIVETITLETEXT, SYSCLR_INACTIVETITLETEXTBGND,
   std3d
};
static int customScheme[] = {
   SYSCLR_OUTPUTTEXT, SYSCLR_WINDOW,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW,
   std3d
};

static int ctx_wc2SCHEME[] =
{
   wcButton    , ( int) &buttonScheme,
   wcCheckBox  , ( int) &buttonScheme,
   wcRadio     , ( int) &buttonScheme,
   wcDialog    , ( int) &dialogScheme,
   wcSlider    , ( int) &sliderScheme,
   wcLabel     , ( int) &staticScheme,
   wcInputLine , ( int) &editScheme,
   wcEdit      , ( int) &editScheme,
   wcListBox   , ( int) &editScheme,
   wcCombo     , ( int) &editScheme,
   wcMenu      , ( int) &menuScheme,
   wcPopup     , ( int) &menuScheme,
   wcScrollBar , ( int) &scrollScheme,
   wcWindow    , ( int) &windowScheme,
   wcWidget    , ( int) &customScheme,
   endCtx
};


static long
remap_color( HPS ps, long clr, Bool toSystem)
{
   long c = clr;
   if ( toSystem && c < 0)
   {
      int * scheme = ( int *) ctx_remap_def( clr & wcMask, ctx_wc2SCHEME, true, ( int) &customScheme);
      if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
      c = WinQuerySysColor( HWND_DESKTOP, scheme[ clr - 1], 0);
   }
   return c;
}

// VIEW

Point
apc_widget_client_to_screen   ( Handle self, Point p)
{
   if ( !WinMapWindowPoints( var handle, HWND_DESKTOP, ( PPOINTL) &p, 1))
      apiErr;
   return p;
}

#define need_view_recreate    (( DHANDLE( owner) != sys owner)        \
                             || ( syncPaint != is_apt( aptSyncPaint)) \
                             || ( clipOwner != is_apt( aptClipOwner)) \
                             )

Bool
apc_widget_create( Handle self, Handle owner, Bool syncPaint, Bool clipOwner, Bool transparent)
{
   Bool reset = false;
   ViewProfile vprf;
   int oStage = var stage;
   if ( !kind_of( self, CWidget)) apcErr( errInvObject);
   apcErrClear;
   if (( var handle != nilHandle) && need_view_recreate )
   {
      get_view_ex( self, &vprf);
      reset = true;
   }
   if ( reset || ( var handle == nilHandle)) create_group( self, owner, syncPaint, clipOwner, WC_CUSTOM, 0);
   if ( reset)
   {
      set_view_ex( self, &vprf);
      var stage = oStage;
   }

   if ( is_apt( aptTransparent) != transparent && !reset) apc_widget_repaint( self);
   apt_assign( aptTransparent, transparent);
   if ( reset) apc_widget_repaint( self);
   return apcError == 0;
}

#define fgBitmap 0
#define fgVector 1

static Bool recursiveFF = 0;
static FONTMETRICS fmtx;

FIXED float2fixed( double f)
{
   double d1, d2;
   d2 = modf( f, &d1);
   return MAKEFIXED( d1, d2 * 65536);
}

double fixed2float( FIXED f)
{
   double r;
   r = FIXEDINT( f) + FIXEDFRAC( f) / 65536.0;
   return r;
}

static int
font_font2gp_internal( PFont font, Point res, Bool forceSize)
{
   int i, count, resId, resValue, vecId, widValue, heiValue, ascent, descent;
   PFONTMETRICS fm;
   Bool useWidth       = font-> width != 0;
   Bool useDirection   = font-> direction   != 0;
   Bool usePitch       = ( font-> pitch & fpPitchMask) != fpDefault;
   Bool usePrecis      = ( font-> pitch & fpPrecisionMask) != fpDontCare;
   #define gotta( w, result){font-> width  = w;       \
                         free( fm);                \
                         font-> ascent = ascent;   \
                         font-> descent = descent; \
                         return result;}
   #define ascents( fontmtx)               \
       ascent = fontmtx.  lMaxAscender;    \
       descent = fontmtx.  lMaxDescender;


   i = 0;
   // querying font for given name
   {
      SIZEF sz;
      if ( !GpiQueryCharBox( guts. ps, &sz)) apiErr;
      if ( !GpiSetCharBox( guts. ps, &guts. defFontBox)) apiErr;
      count = GpiQueryFonts( guts. ps, QF_PUBLIC, font->name, ( PLONG)&i, sizeof( FONTMETRICS), nil);
      if ( count < 0) apiErrRet;
      fm = malloc( count * sizeof( FONTMETRICS));
      if ( GpiQueryFonts( guts. ps, QF_PUBLIC, font->name, ( PLONG)&count, sizeof( FONTMETRICS), fm) < 0)
        apiErrRet;
      if ( !GpiSetCharBox( guts. ps, &sz)) apiErr;
   }

   resId = vecId = -1;
   resValue = widValue = heiValue = INT_MAX;
   // cycling for appropriate value
   for ( i = 0; i < count; i++)
   {
      PFONTMETRICS f = &fm[ i];

      if ( usePitch)
      {
         int fpitch = ( f-> fsType & FM_TYPE_FIXED) ? fpFixed : fpVariable;
         if ( fpitch != ( font-> pitch & fpPitchMask)) continue;
      }

      if ( f-> fsDefn & FM_DEFN_OUTLINE)
         vecId = i;
      else {
         long scr = ( f-> sXDeviceRes - res. x) * ( f-> sXDeviceRes - res. x) +
                    ( f-> sYDeviceRes - res. y) * ( f-> sYDeviceRes - res. y);
         if ( scr < resValue && ( !forceSize || ( f-> sNominalPointSize == font-> size * 10))) {
            resId    = i;
            resValue = scr;
            heiValue = ( f-> lMaxBaselineExt - font-> height) * ( f-> lMaxBaselineExt - font-> height);
            if ( useWidth)
               widValue = ( f-> lAveCharWidth - font-> width) * ( f-> lAveCharWidth - font-> width);
         } else if ( !forceSize && scr == resValue) {
            long hei = ( f-> lMaxBaselineExt - font-> height) * ( f-> lMaxBaselineExt - font-> height);
            if ( hei < heiValue) {
               resId    = i;
               heiValue = hei;
               if ( useWidth)
                  widValue = ( f-> lAveCharWidth - font-> width) * ( f-> lAveCharWidth - font-> width);
            } else if ( useWidth && ( hei == heiValue)) {
               long wid = ( f-> lAveCharWidth - font-> width) * ( f-> lAveCharWidth - font-> width);
               if ( wid < widValue) {
                  resId    = i;
                  widValue = wid;
               }
            }
         }
      } // end else
   } // end loop

 // is font aspectable?
   if ( useDirection) {
      if ( vecId < 0) {
         if ( !usePitch && ( resId >= 0))
            font-> pitch = ( fm[ resId]. fsType & FM_TYPE_FIXED) ? fpFixed : fpVariable;
         usePitch = 1; // if not aspectable, force it.
      }
      resId = -1;
   }

   // have resolution ok?
   if (
         resId >= 0 &&                                  // if resolution match found
         resValue < 16 &&                               // and difference is no more than 4 points
         ( !usePrecis ||                                // and no precision specified
            (( font-> pitch & fpPrecisionMask) == fpRaster)     // or raster precision preferred
         ) &&
         ( forceSize ||                                 // enough for size pickup -
            ( heiValue < 16 &&                          // but if picking for height,
               ( !useWidth ||                           // 4 pts y-difference is ok, and enough
                  ( widValue == 0 && heiValue == 0)     // if no width pickup specified, else
               )                                        // same 4 pts x-diff check.
            )
         )
      )
   {
      font-> height = fm[ resId]. lMaxBaselineExt;
      ascents( fm[ resId]);
      memcpy( &fmtx, &fm[ resId], sizeof( fmtx));
      font-> size   = fm[ resId]. sNominalPointSize / 10;
      gotta( fm[ resId]. lAveCharWidth, fgBitmap);
   }
   // or vector font - for any purpose?

   if ( vecId >= 0 && ( !usePrecis || ( font-> pitch & fpPrecisionMask) == fpVector))
   {
      if ( forceSize) {
         int selSize = fm[ vecId]. sNominalPointSize;
         // font-> height = fm[ vecId]. lMaxBaselineExt * font-> size * 10.0 / selSize + 0.5;
         font-> height = font-> size * res. y / 72.0 + 0.5;
         ascent  = fm[ vecId]. lMaxAscender  * font-> size * 10.0 / selSize + 0.5;
         descent = fm[ vecId]. lMaxDescender * font-> size * 10.0 / selSize + 0.5;
         memcpy( &fmtx, &fm[ vecId], sizeof( fmtx));
         #define fmtx_adj( field) fmtx. field = fmtx. field * font-> size * 10 / selSize + 0.5
         fmtx_adj( lAveCharWidth    );
         fmtx_adj( lMaxBaselineExt  );
         fmtx_adj( lMaxAscender     );
         fmtx_adj( lLowerCaseAscent );
         fmtx_adj( lMaxDescender    );
         fmtx_adj( lLowerCaseDescent);
         fmtx_adj( lExternalLeading );
         fmtx_adj( lInternalLeading );
         fmtx_adj( lMaxCharInc      );
         #undef fmtx_adj
         // gotta( fm[ vecId]. lMaxCharInc * font-> size * 10.0 / selSize + 0.5, fgVector);
      } else {
         #define mult font-> height / fm[ vecId]. lMaxBaselineExt + 0.5
         ascent  = fm[ vecId]. lMaxAscender  * mult;
         descent = fm[ vecId]. lMaxDescender * mult;
         font-> size = font-> height * 72.0 / res. y + 0.5;
         memcpy( &fmtx, &fm[ vecId], sizeof( fmtx));
         #define fmtx_adj( field) fmtx. field = fmtx. field * mult
         fmtx_adj( lAveCharWidth    );
         fmtx_adj( lMaxBaselineExt  );
         fmtx_adj( lMaxAscender     );
         fmtx_adj( lLowerCaseAscent );
         fmtx_adj( lMaxDescender    );
         fmtx_adj( lLowerCaseDescent);
         fmtx_adj( lExternalLeading );
         fmtx_adj( lInternalLeading );
         fmtx_adj( lMaxCharInc      );
         #undef fmtx_adj
         #undef mult
      }
      if ( useWidth)
         gotta( font-> width, fgVector)
      else
         gotta( font-> height, fgVector)
   }

   // no need in this
   free( fm);  fm = nil;

   // if strict font not found, use subplacing
   if ( usePitch)
   {
       int ogp  = font-> pitch & fpPitchMask;
       int ret;
       int ogpp = font-> pitch & fpPrecisionMask;
       strcpy( font-> name, (( font-> pitch & fpPitchMask) == fpFixed) ?
        DEFAULT_FIXED_FONT : DEFAULT_VARIABLE_FONT);
       font-> pitch = fpDefault | ogpp;
       ret = font_font2gp_internal( font, res, forceSize);
       if ( ogp == fpFixed && ( strcmp( font-> name, DEFAULT_FIXED_FONT) != 0))
       {
          strcpy( font-> name, DEFAULT_SYSTEM_FONT);
          font-> pitch = fpDefault | ogpp;
          ret = font_font2gp_internal( font, res, forceSize);
       }
       return ret;
   }

   // Font not found. So use general representation for height and width
   strcpy( font-> name, DEFAULT_FONT_NAME);
   if ( recursiveFF == 0)
   {
      // trying to catch default font witch correct values
      int r;
      recursiveFF++;
      r = font_font2gp_internal( font, res, forceSize);
      recursiveFF--;
      return r;
   } else {
      // if not succeeded, to avoid recursive call use "wild guess".
      // This could be achieved if system does not have "Helv" font
      *font = guts. sysDefFont;
      font-> pitch = fpDefault;
      return font_font2gp_internal( font, res, forceSize);
   }
   return fgBitmap;
}

static int
font_font2gp( PFont font, Point res, Bool forceSize)
{
   int vectored;
   Font key;
   Bool addSizeEntry;

   font-> resolution = res. y * 0x10000 + res. x;
   if ( get_font_from_hash( font, &vectored, &fmtx, forceSize))
      return vectored;
   memcpy( &key, font, sizeof( Font));
   vectored = font_font2gp_internal( font, res, forceSize);
   font-> resolution = res. y * 0x10000 + res. x;
   if ( forceSize) {
      key. height = font-> height;
      key. width  = font-> width;
      addSizeEntry = true;
   } else {
      key. size  = font-> size;
      addSizeEntry = vectored ? (( key. height == key. width)  || ( key. width == 0)) : true;
   }
   add_font_to_hash( &key, font, vectored, &fmtx, addSizeEntry);
   return vectored;
}

static void
font_pp2gp( char * ppFontNameSize, PFont font)
{
   char * p = strchr( ppFontNameSize, '.');
   int i;

   if ( p)
   {
      font-> size = atoi( ppFontNameSize);
      p++;
   } else {
      font-> size = DEFAULT_FONT_SIZE;
      p = DEFAULT_FONT_NAME;
   }
   font-> height      = font-> size * guts. displayResolution. y / 72.0 + 0.5;
   font-> width       = 0;
   font-> pitch       = fpDefault;
   font-> style       = fsNormal;
   font-> direction   = 0;
   strcpy( font-> name, p);
   p = font-> name;
   for ( i = strlen( p) - 1; i >= 0; i--)
   {
      if ( p[ i] == '.')
      {
         if ( strcmp( "Italic",     &p[ i + 1]) == 0) font-> style |= fsItalic;
         if ( strcmp( "Underscore", &p[ i + 1]) == 0) font-> style |= fsUnderlined;
         if ( strcmp( "Strikeout",  &p[ i + 1]) == 0) font-> style |= fsStruckOut;
         if ( strcmp( "Bold",       &p[ i + 1]) == 0) font-> style |= fsBold;
         if ( strcmp( "Outline",    &p[ i + 1]) == 0) font-> style |= fsOutline;
         p[ i] = 0;
      }
   }
}

static void
font_gp2pp( PFont font, char * buf)
{
//    int size = font-> height * 72.0 / guts. displayResolution. y + 0.5;
   sprintf ( buf, "%d.%s%s%s%s%s%s", font-> size, font-> name,
      ( font-> style & fsItalic)     ? ".Italic"     : "",
      ( font-> style & fsBold)       ? ".Bold"       : "",
      ( font-> style & fsStruckOut)  ? ".Strikeout"  : "",
      ( font-> style & fsUnderlined) ? ".Underlined" : "",
      ( font-> style & fsOutline)    ? ".Outline"    : ""
   );
}

static int
font_style( PFONTMETRICS fm)
{
   int style = ftNormal; // 0
   if ( fm-> fsSelection & FM_SEL_ITALIC)     style |= fsItalic;
   if ( fm-> fsSelection & FM_SEL_UNDERSCORE) style |= fsUnderlined;
   if ( fm-> fsSelection & FM_SEL_STRIKEOUT)  style |= fsStruckOut;
   if ( fm-> fsSelection & FM_SEL_BOLD)       style |= fsBold;
   if ( fm-> fsSelection & FM_SEL_OUTLINE)    style |= fsOutline;
   return style;
}

static void
convert_fontmetrics( PFONTMETRICS m, PFontMetric f)
{
   strcpy( f-> family,   m-> szFamilyname);
   if ( m-> fsType & FM_TYPE_FACETRUNC)
      WinQueryAtomName( WinQuerySystemAtomTable(), m-> FaceNameAtom, f-> facename, 256);
   else
      strcpy( f-> facename, m-> szFacename);
   f-> nominalSize        = m-> sNominalPointSize / 10;
// f-> width              = m-> lAveCharWidth    ;
   f-> width              = m-> lMaxCharInc      ;
   f-> height             = m-> lMaxBaselineExt  ;
   f-> style              = font_style( m)       ;
   f-> pitch              =
      (( m-> fsType & FM_TYPE_FIXED   ) ? fpFixed  : fpVariable) |
      (( m-> fsDefn & FM_DEFN_OUTLINE ) ? fpVector : fpRaster);
   f-> weight             = m-> usWeightClass    ;
   f-> maxAscent          = m-> lMaxAscender     ;
   f-> lcAscent           = m-> lLowerCaseAscent ;
   f-> maxDescent         = m-> lMaxDescender    ;
   f-> lcDescent          = m-> lLowerCaseDescent;
// f-> averageWidth       = m-> lAveCharWidth    ;
   f-> averageWidth       = m-> lMaxCharInc      ;
   f-> internalLeading    = m-> lInternalLeading ;
   f-> externalLeading    = m-> lExternalLeading ;
   f-> xDeviceRes         = m-> sXDeviceRes      ;
   f-> yDeviceRes         = m-> sYDeviceRes      ;
   f-> firstChar          = m-> sFirstChar       ;
   f-> lastChar           = m-> sLastChar        ;
   f-> breakChar          = m-> sBreakChar       ;
   f-> defaultChar        = m-> sDefaultChar     ;
}

PFontMetric
apc_fonts( char * facename, int * retCount)
{
   PFontMetric fmtx;
   PFONTMETRICS fm;
   int i = 0, j, count;
   apcErrClear;
   count = GpiQueryFonts( guts. ps, QF_PUBLIC, facename, ( PLONG)&i, sizeof( FONTMETRICS), nil);
   if ( count < 0) { apiErr; return nil; }
   fm    = malloc( count * sizeof( FONTMETRICS));
   if ( GpiQueryFonts( guts. ps, QF_PUBLIC, facename, ( PLONG)&count, sizeof( FONTMETRICS), fm) < 0) {
       apiErr;
       return nil;
   }
   if ( facename == nil)
   {
      int     rc = 0;
      char ** table = malloc( count * sizeof( char*));
      for ( i = 0; i < count; i++)
      {
         Bool found = false;
         for ( j = 0; j < rc; j++)
         {
            // if ( strcmp( fm[ i].szFamilyname, table[ j]) == 0)
            if ( strcmp( fm[ i].szFacename, table[ j]) == 0)
            {
               found = true;
               fm[ i].szFacename[0] = 0;
               break;
            }
         }
         if ( !found) table[ rc++] = fm[ i].szFacename;
      }
      free( table);
      *retCount = rc;
   } else *retCount = count;
   fmtx = malloc( *retCount * sizeof( FontMetric));

   for ( i = 0, j = 0; i < count; i++)
   {
      if ( fm[ i]. szFacename[0] == 0) continue;
      convert_fontmetrics( &fm[ i], &fmtx[ j]);
      j++;
   }
   free( fm);
   return fmtx;
}

PFontMetric
apc_font_metrics( Handle self, PFont font, PFontMetric metrics)
{
   apcErrClear;
   font_font2gp( font, sys res, false);
   convert_fontmetrics( &fmtx, metrics);
   return metrics;
}

void
apc_font_pick( Handle self, PFont source, PFont dest)
{
   font_font2gp( dest, self ? sys res : guts. displayResolution,
      Drawable_font_add( self, source, dest));
}

PFont
apc_font_default( PFont font)
{
   *font = guts. sysDefFont;
   font-> pitch = fpDefault;
   return font;
}

PFont
apc_widget_default_font ( PFont font)
{
   char buf  [256];
   apcErrClear;
   PrfQueryProfileString( HINI_USERPROFILE, "PM_SystemFonts", "WindowText", DEFAULT_FONT_FACE, buf, 255);
   font_pp2gp( buf, font);
   font_font2gp( font, guts. displayResolution, true);
   return font;
}

Bool
apc_widget_is_captured( Handle self)
{
   return WinQueryCapture( HWND_DESKTOP) == HANDLE;
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
   return clLightRed;
}

void
apc_widget_set_capture( Handle self, Bool capture)
{
   if ( !WinSetCapture( HWND_DESKTOP, capture ? var handle : nilHandle)) apiErr;
}

void
apc_widget_set_color( Handle self, Color color, int index)
{
   int idx;
   int oStage = var stage;
   Event ev = {cmColorChanged};

   color = remap_color( nilHandle, color, true);
   if ( index == ciLight3DColor) { sys l3dc = color; return; }
   if ( index == ciDark3DColor)  { sys d3dc = color; return; }
   idx = ctx_remap_end( index, ctx_ci2PP, true);
   if ( idx == endCtx) return;

   var stage = csAxEvents;
   if ( !WinSetPresParam( var handle, idx, sizeof( color), &color)) apiErr;
   var stage = oStage;

   ev. gen. source = self;
   ev. gen. i      = index;
   var self-> message( self, &ev);
}

void
apc_widget_set_pos( Handle self, int x, int y)
{
   if ( kind_of( self, CWindow))
   {
      HWND h = HANDLE;
      if ( sys s. window. state == wsMinimized) sys s. window. hiddenPos = ( Point){ x, y};
      if ( var stage == csConstructing && sys s. window. state != wsNormal)
      {
         if ( !WinSetWindowUShort( h, QWS_XRESTORE, x)) apiErr;
         if ( !WinSetWindowUShort( h, QWS_YRESTORE, y)) apiErr;
      } else
         if ( !WinSetWindowPos( h, 0, x, y, 0, 0, SWP_MOVE)) apiErr;
   } else
      if ( !WinSetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_MOVE)) apiErr;
}

void
apc_widget_set_size( Handle self, int width, int height)
{
   if ( kind_of( self, CWindow) &&  ( sys s. window. state == wsMinimized))
   {
      sys s. window. lastClientSize. x += sys s. window. lastClientSize. x - sys s. window. lastFrameSize. x + width;
      sys s. window. lastClientSize. y += sys s. window. lastClientSize. y - sys s. window. lastFrameSize. y + height;
      sys s. window. lastClientSize = sys s. window. hiddenSize = ( Point){ width, height};
   }
   else
      if ( !WinSetWindowPos( HANDLE, 0, 0, 0, width, height, SWP_SIZE)) apiErr;
}

void
apc_widget_set_visible( Handle self, Bool show)
{
   if ( sys lockState)
   {
      apt_assign( aptLockVisState, show);
      return;
   }
   if ( !WinSetWindowPos( HANDLE, 0, 0, 0, 0, 0, show ? SWP_SHOW : SWP_HIDE)) apiErr;
   if ( !is_apt( aptClipOwner)) WinInvalidateRect ( var handle, nil, true);
}

void
apc_widget_destroy( Handle self)
{
   if ( sys pointer2) if ( !WinDestroyPointer( sys pointer2)) apiErr;
   free( sys timeDefs);
   if ( var handle == nilHandle) return;
   if (sys className == WC_FRAME && firstModal) {
      Handle h = firstModal;
      Handle h1;
      if ( firstModal == self)
         firstModal = dsys( self) s. window. nextModal;
      else
         while (h) {
            h1 = dsys( h) s. window. nextModal;
            if ( h1 == self) {
               dsys( h) s. window. nextModal = dsys( h1) s. window. nextModal;
               break;
            }
            h = h1;
         }
   }
   if ( !WinDestroyWindow( HANDLE)) apiErr;
}

PFont
view_get_font ( Handle self, PFont font)
{
   char buf  [256];
   if ( WinQueryPresParam( var handle, PP_FONTNAMESIZE, 0, nil, 256,
        &buf, QPF_NOINHERIT) && strchr( buf, '.'))
   {
      font_pp2gp( buf, font);
      font_font2gp( font, sys res, true);
   }
      else apc_widget_default_font( font);
   return font;
}


Bool
apc_widget_get_first_click ( Handle self)
{
   return is_apt( aptFirstClick);
}

Point
apc_widget_get_pos( Handle self)
{
   SWP  swp;
   if ( !WinQueryWindowPos ( HANDLE, &swp)) apiErr;
   if ( swp. fl & SWP_MINIMIZE && kind_of( self, CWindow)) return sys s. window. hiddenPos;
   return ( Point){ swp. x, swp. y};
}


Bool
apc_widget_get_transparent( Handle self)
{
   return is_apt( aptTransparent);
}

Bool
apc_widget_get_sync_paint( Handle self)
{
  return is_apt( aptSyncPaint);
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   return is_apt( aptClipOwner);
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

Rect
apc_widget_get_invalid_rect( Handle self)
{
   Rect r;
   Bool ok = ( Bool) WinQueryUpdateRect( var handle, ( PRECTL) &r);
   return ok ? r : ( Rect){0,0,0,0};
}

Bool
apc_widget_is_enabled ( Handle self)
{
   return is_apt( aptEnabled);
// return WinIsWindowEnabled( HANDLE);
}

Bool
apc_widget_is_responsive ( Handle self)
{
   Bool ena = true;
   while ( ena && self != application) {
      // ena  = WinIsWindowEnabled( HANDLE);
      ena  = is_apt( aptEnabled);
      self = var owner;
   }
   return ena;
}

Bool
apc_widget_is_visible ( Handle self)
{
   return is_apt( aptVisible);
}

Bool
apc_widget_is_focused ( Handle self)
{
   return is_apt( aptFocused);
}

Handle
apc_widget_get_focused( void)
{
   return hwnd_to_view( WinQueryFocus( HWND_DESKTOP));
}

void
apc_widget_repaint( Handle self)
{
   if ( !WinInvalidateRect( var handle, nil, true)) apiErr;
}

void
apc_widget_update( Handle self)
{
   if ( !WinUpdateWindow( var handle)) apiErr;
}

void
apc_widget_invalidate_rect( Handle self, Rect rect)
{
   if ( !WinInvalidateRect ( var handle, ( PRECTL)&rect, true)) apiErr;
}

void
apc_widget_lock ( Handle self)
{
   if ( sys lockState++ == 0)
   {
      apt_assign( aptLockVisState, ( WinQueryWindowULong( HANDLE, QWL_STYLE) & WS_VISIBLE) == WS_VISIBLE);
      if ( !WinLockWindowUpdate( HWND_DESKTOP, HANDLE)) apiErr;
   }
}

void
apc_widget_unlock ( Handle self)
{
   if ( --sys lockState == 0)
   {
      if ( !WinLockWindowUpdate( HWND_DESKTOP, nilHandle)) apiErr;
      if ( !WinSetWindowPos( HANDLE, 0, 0, 0, 0, 0, is_apt( aptLockVisState) ? SWP_SHOW : SWP_HIDE)) apiErr;
   }
}

void
apc_widget_validate_rect ( Handle self, Rect rect)
{
   if ( !WinValidateRect( var handle, ( PRECTL)&rect, true)) apiErr;
}

void
apc_widget_set_enabled ( Handle self, Bool enable)
{
   apt_assign( aptEnabled, enable);
   if (( sys className == WC_FRAME) || ( var owner == application) || !is_apt( aptClipOwner)) {
      if ( !WinEnableWindow( HANDLE, enable)) apiErr;
   } else
      WinSendMsg( HANDLE, WM_ENABLE, ( MPARAM) enable, 0);
}


void
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   apt_assign( aptFirstClick, firstClick);
}

void
apc_widget_set_focused ( Handle self)
{
   if ( self)
     WinFocusChange( HWND_DESKTOP, var handle, is_apt( aptClipOwner) ? 0
         : FC_NOLOSEACTIVE);
   else
     WinFocusChange( HWND_DESKTOP, HWND_DESKTOP, 0);
}

void
apc_widget_set_font ( Handle self, PFont font)
{
   char buf [256];
   int oStage = var stage;
   if (( var handle == nilHandle) || ( font == nilHandle)) return;
   font_gp2pp( font, buf);
   var stage = csAxEvents;
   if ( !WinSetPresParam( var handle, PP_FONTNAMESIZE, ( ULONG) strlen( buf)+1, ( PVOID) &buf)) apiErr;
   var stage = oStage;
   WinSendMsg( var handle, WM_FONTCHANGED, 0, 0);
}

void
apc_widget_set_tab_order( Handle self, int tabOrder)
{
}

#define cbNoBitmap     0
#define cbScreen       1
#define cbMonoScreen   2
#define cbImage        3

static PBITMAPINFO2 get_screen_binfo( void);
static HBITMAP make_bitmap_ps( Handle img, HPS * hps, HDC * hdc, PBITMAPINFO2 * bm, int createBitmap);

Bool
apc_widget_begin_paint( Handle self, Bool insideOnPaint)
{
   Bool presetHPS = ( sys ps != nilHandle);
   apcErrClear;
   if ( is_apt( aptTransparent))
   {
      SWP swp;
      WinQueryWindowPos( var handle, &swp);
      if (( swp. fl & SWP_HIDE) == 0)
      {
         HWND parent = dsys((( PWidget) self)-> owner) parent;
         list_add( &guts. transp, self);
         WinSetWindowPos( var handle, 0, 0, 0, 0, 0, SWP_HIDE);
         WinUpdateWindow( parent);
         if ( parent == HWND_DESKTOP) DosSleep( 1);
         WinSetWindowPos( var handle, 0, 0, 0, 0, 0, SWP_SHOW);
         list_delete( &guts. transp, self);
      }
   }
   apt_set( aptWinPS);
   apt_set( aptCompatiblePS);
   apt_assign( aptWM_PAINT, insideOnPaint);

   sys fontId     = 0;
   sys fontHash   = create_fontid_hash();
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
      bm = make_bitmap_ps( self, &hps, &hdc, &bmInfo, cbScreen);
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
   apc_gp_set_color( self, apc_widget_get_color( self, ciFore));
   apc_gp_set_back_color( self, apc_widget_get_color( self, ciBack));
   apt_assign( aptFontExists, presetHPS);
   if ( presetHPS)
   {
      Bool vectored;
      SIZEF sz;
      apc_gp_get_font( self, &var font);
      vectored = fmtx. fsDefn & FM_DEFN_OUTLINE;
      sys fontId = GpiQueryCharSet( sys ps);
      if ( !GpiQueryCharBox( sys ps, &sz)) apiErr;
      var font. resolution = sys res. y * 0x10000 + sys res. x;
      add_fontid_to_hash( sys fontHash, sys fontId, &var font, &sz, vectored);
   } else {
      apc_gp_set_font( self, &var font);
   }
   save_gp_attrs( self);
   return true;
}

void
apc_widget_end_paint( Handle self)
{
   if ( is_opt( optBuffered) && ( sys bm != nilHandle)) {
      POINTL pt [4] = {
         { sys transform2. x, sys transform2. y},
         { sys transform2. x + var w - 1, sys transform2. y + var h - 1},
         {0, 0}, { var w, var h}
      };
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

   if ( sys ps != nilHandle)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT))
      {
         list_delete( &guts. psList, sys ps);
         if ( !WinEndPaint( sys ps)) apiErr;
      }
      else if ( is_apt( aptWinPS))
      {
         list_delete( &guts. winPsList, sys ps);
         if ( !WinReleasePS( sys ps)) apiErr;
      }
   }
   if ( sys fillBitmap)
      if ( !GpiDeleteBitmap( sys fillBitmap)) apiErr;
   sys ps = sys fillBitmap = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptWM_PAINT);
   apt_clear( aptCompatiblePS);
   apt_clear( aptFontExists);
   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   restore_gp_attrs( self);
}

Point
apc_widget_screen_to_client   ( Handle self, Point p)
{
   if ( !WinMapWindowPoints( HWND_DESKTOP, var handle, ( PPOINTL) &p, 1)) apiErr;
   return p;
}

void
apc_widget_scroll ( Handle self, int horiz, int vert, Bool scrollChildren)
{
   WinShowCursor( var handle, false);
   if ( !WinScrollWindow( HANDLE, horiz, vert, nil, nil, nilHandle, nil,
                    SW_INVALIDATERGN | ( scrollChildren ? SW_SCROLLCHILDREN : 0))) apiErr;
   WinShowCursor( var handle, true);
}

void
apc_widget_scroll_rect( Handle self, int horiz, int vert, Rect r, Bool scrollChildren)
{
   WinShowCursor( var handle, false);
   if ( !WinScrollWindow( HANDLE, horiz, vert, (PRECTL)&r, nil, nilHandle, nil,
                    SW_INVALIDATERGN | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
                  )) apiErr;
   WinShowCursor( var handle, true);
}

void
apc_widget_set_z_order ( Handle self, Handle behind, Bool top)
{
   HWND opt = ( top) ? HWND_TOP : HWND_BOTTOM;
   if ( behind != nilHandle) opt = (( PWidget) behind)-> handle;
   if ( !WinSetWindowPos( var handle, opt, 0, 0, 0, 0, SWP_ZORDER)) apiErr;
}

// VIEW END
// IMAGE

static PBITMAPINFO2
get_binfo( Handle self)
{
   int i;
   PImage       image = ( PImage) self;
   int          nColors;
   PBITMAPINFO2 bi;

   if ( is_apt( aptDeviceBitmap)) {
       bi = get_screen_binfo();
       bi-> cx = image-> w;
       bi-> cy = image-> h;
       if ((( PDeviceBitmap) self)-> monochrome) bi-> cPlanes = bi-> cBitCount = 1;
       return bi;
   }

   nColors = (( 1 << ( image-> type & imBPP)) & 0x1ff);
   bi      = malloc( sizeof ( BITMAPINFO2) + nColors * sizeof( RGB2));
   memset( bi, 0, sizeof( BITMAPINFO2) + nColors * sizeof( RGB2));
   bi-> cbFix           = sizeof( BITMAPINFO2) - sizeof( RGB2);
   bi-> cx              = image-> w;
   bi-> cy              = image-> h;
   bi-> cPlanes         = 1;
   bi-> cBitCount       = image-> type & imBPP;
   bi-> ulCompression   = BCA_UNCOMP;
   bi-> cclrUsed        = nColors;
   bi-> cclrImportant   = nColors;
   bi-> usUnits         = BRU_METRIC;
   bi-> usRecording     = BRA_BOTTOMUP;
   bi-> usRendering     = BRH_NOTHALFTONED;
   bi-> ulColorEncoding = BCE_RGB;
   for ( i = 0; i < nColors; i++)
   {
      bi-> argbColor[ i]. bRed    = image-> palette[ i]. r;
      bi-> argbColor[ i]. bGreen  = image-> palette[ i]. g;
      bi-> argbColor[ i]. bBlue   = image-> palette[ i]. b;
   }
   if ( guts. monoBitsOk && bi-> cBitCount == 1)
   {
      bi-> argbColor[ 0]. bRed = bi-> argbColor[ 0]. bBlue = bi-> argbColor[ 0]. bGreen = 0xFF;
      bi-> argbColor[ 1]. bRed = bi-> argbColor[ 1]. bBlue = bi-> argbColor[ 1]. bGreen = 0x00;
   }
   return bi;
}

static PBITMAPINFO2
get_screen_binfo( void)
{
    PBITMAPINFO2 br      = malloc( sizeof ( BITMAPINFO2) + 256 * sizeof( RGB2));
    memset( br, 0, sizeof ( BITMAPINFO2) + 256 * sizeof( RGB2));
    br-> cbFix           = sizeof( BITMAPINFO2) - sizeof( RGB2);
    br-> cPlanes         = guts. bmf[ 0];
    br-> cBitCount       = guts. bmf[ 1];
    br-> ulCompression   = BCA_UNCOMP;
    br-> usRecording     = BRA_BOTTOMUP;
    br-> usRendering     = BRH_NOTHALFTONED;
    br-> ulColorEncoding = BCE_RGB;
    return br;
}

static Bool screenable( Handle image)
{
   PImage i = ( PImage) image;
   if (( i-> type & ( imRealNumber | imComplexNumber | imTrigComplexNumber)) ||
       ( i-> type == imLong || i-> type == imShort)) return false;
   return true;
}

static Handle enscreen( Handle image)
{
   PImage i = ( PImage) image;
   if ( !screenable( image))
   {
      Handle j = i-> self-> dup( image);
      ((( PImage) j)-> self)->resample( j,
         ((( PImage) j)-> self)->get_stats( j, isRangeLo),
         ((( PImage) j)-> self)->get_stats( j, isRangeHi),
         0, 255
      );
      ((( PImage) j)-> self)->set_type( j, imByte);
      return j;
   } else
      return image;
}

static Handle cmono_enscreen( Handle image)
{
   PImage i = ( PImage) image;
   if ( !screenable( image))
   {
      return enscreen( image);
   } else
      if ((( i-> type & imBPP) == imbpp1) &&
          (
            ( ARGB( i-> palette[0].r, i-> palette[0].g, i-> palette[0].b) != clBlack) ||
            ( ARGB( i-> palette[1].r, i-> palette[1].g, i-> palette[1].b) != clWhite)
          ))
      {
         Handle j = i-> self-> dup( image);
         ((( PImage) j)-> self)->set_type( j, im16);
         return j;
      } else
      return image;
}

static HBITMAP
make_bitmap_ps( Handle img, HPS * hps, HDC * hdc, PBITMAPINFO2 * bm, int createBitmap)
{
   SIZEL sizl;
   HBITMAP ret;
   *bm  = nil;
   *hps = nilHandle;
   *hdc = DevOpenDC ( guts. anchor, OD_MEMORY, "*", 0, nil, nilHandle );
   if ( *hdc == nilHandle ) { apiErr; return nilHandle;}

   sizl . cx = (( PDrawable) img)-> w;
   sizl . cy = (( PDrawable) img)-> h;
   *hps = GpiCreatePS ( guts. anchor, *hdc, &sizl,
                       PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC ) ;
   if ( *hps == nilHandle)
   {
      apiErr;
      DevCloseDC( *hdc);
      *hdc = nilHandle;
      return nilHandle;
   }
   if ( !GpiCreateLogColorTable( *hps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;

   if ( createBitmap) {
      Handle deja;
      PImage image;
      if ( createBitmap == cbImage) {
         deja  = cmono_enscreen( img);
         image = ( PImage) deja;
         *bm   = get_binfo( deja);
         ret = GpiCreateBitmap( *hps, ( PBITMAPINFOHEADER2) *bm, CBM_INIT, image-> data, *bm);
      } else {
         *bm = get_screen_binfo();
         if ( createBitmap == cbMonoScreen) (*bm)-> cPlanes = (*bm)-> cBitCount = 1;
         (*bm)-> cx = (( PDrawable) img)-> w;
         (*bm)-> cy = (( PDrawable) img)-> h;
         ret = GpiCreateBitmap( *hps, ( PBITMAPINFOHEADER2) *bm, 0, nil, nilHandle);
      }
      if ( ret == nilHandle)
      {
         apiErr;
         GpiDestroyPS( *hps);
         DevCloseDC( *hdc);
         free( *bm);
         *bm  = nil;
         *hps = *hdc = nilHandle;
      }
      if ( createBitmap == cbImage && deja != img) Object_destroy( deja);
      if ( ret == nilHandle) return nilHandle;
   } else
      ret = ( HBITMAP) 1;
   if ( GpiSetBitmap( *hps, nilHandle) == HBM_ERROR) apiErr;
   return ret;
}

static Handle
make_bitmap_handle( Handle img)
{
    HDC hdc;
    HPS hps;
    PBITMAPINFO2 bmInfo;
    HBITMAP bm = make_bitmap_ps( img, &hps, &hdc, &bmInfo, cbImage);
    if ( bm == nilHandle) return nilHandle;
    if ( !GpiDestroyPS( hps)) apiErr;
    if ( !DevCloseDC( hdc)) apiErr;
    free( bmInfo);
    return ( Handle) bm;
}

static HPOINTER
make_pointer_handle( Handle self, Handle icon, Point hotSpot)
{
    HBITMAP m, b = nilHandle;
    BInfo2 bm;
    PIcon i = ( PIcon) icon;
    POINTERINFO pi = { true, hotSpot. x, hotSpot. y};
    Point ptSize;
    HPOINTER ret = nilHandle;

    // create mono bitmap
    unsigned char * mbuf;
    if ( kind_of( icon, CIcon))
    {
       mbuf = malloc( i-> maskSize * 2);
       memset( &mbuf[ 0], 0xFF, i-> maskSize);
       memcpy( &mbuf[ i-> maskSize], i-> mask, i-> maskSize);
    } else {
       int maskSize = (( i-> w + 31) / 32) * 4 * i-> h;
       mbuf = malloc( maskSize * 2);
       memset( mbuf, 0xFF, maskSize * 2);
    }
    if ((( i-> type & imBPP) != imMono) ||
         ( ARGB( i-> palette[0].r, i-> palette[0].g, i-> palette[0].b) != clBlack) ||
         ( ARGB( i-> palette[1].r, i-> palette[1].g, i-> palette[1].b) != clWhite)
       )
       b = ( HBITMAP) make_bitmap_handle( icon);
    else
    {
       int j;
       for ( j = 0; j < i-> dataSize; j++) mbuf[ j] = ~i-> data[ j];
    }

    memset( &bm, 0, sizeof ( bm));
    bm. structLength = sizeof ( BInfo);
    bm. w            = i-> w;
    bm. h            = i-> h * 2;
    bm. bpp          = 1;
    bm. planes       = 1;
    bm. palette[1].bRed = bm. palette[1].bGreen = bm. palette[1].bBlue = 0;
    bm. palette[0].bRed = bm. palette[0].bGreen = bm. palette[0].bBlue = 0xFF;
    m = GpiCreateBitmap( guts. ps, ( PBITMAPINFOHEADER2) &bm, CBM_INIT, mbuf, ( PBITMAPINFO2) &bm);
    free( mbuf);
    if ( m == nilHandle) {
       apiErr;
       return nilHandle;
    }
    // scaling to pointer size
    ptSize = ( Point){
       WinQuerySysValue( HWND_DESKTOP, SV_CXPOINTER),
       WinQuerySysValue( HWND_DESKTOP, SV_CYPOINTER)
    };
    if ( i-> w != ptSize. x || i-> h != ptSize. y)
    {
       HBITMAP old, sm, sb = nilHandle;
       BITMAPINFOHEADER2 bi;
       POINTL pt [4] = {
         { 0, 0}, { ptSize. x - 1, ptSize. y - 1},
         { 0, 0}, { i->w, i->h}
       };
       if ( b)
       {
          // stretch color bits
          bi. cbFix = sizeof( bi);
          if ( !GpiQueryBitmapInfoHeader( b, &bi)) apiErr;
          bi. cx = ptSize. x;
          bi. cy = ptSize. y;
          sb = GpiCreateBitmap( guts. ps, &bi, 0, nil, nil);
          if ( sb == nilHandle) {
             apiErr;
             GpiDeleteBitmap( m);
             return nilHandle;
          }
          old = GpiSetBitmap( guts. ps, sb);
          if ( old == HBM_ERROR) apiErr;
          if ( GpiWCBitBlt( guts. ps, b, 4, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) {
             apiErr;
             GpiDeleteBitmap( m);
             return nilHandle;
          }
       } else {
          old = GpiSetBitmap( guts. ps, nilHandle);
          if ( old == HBM_ERROR) apiErr;
       }
       // stretch mono bits
       bi. cbFix = sizeof( bi);
       if ( !GpiQueryBitmapInfoHeader( m, &bi)) {
          apiErr;
          GpiDeleteBitmap( m);
          return nilHandle;
       }
       bi. cx = ptSize. x;
       bi. cy = ptSize. y * 2;
       pt[ 1]. y = ptSize. y * 2 - 1;
       pt[ 3]. y = i-> h * 2,
       sm = GpiCreateBitmap( guts. ps, &bi, 0, nil, nil);
       if ( sm == nilHandle) {
          apiErr;
          GpiDeleteBitmap( m);
          return nilHandle;
       }
       if ( GpiSetBitmap( guts. ps, sm) == HBM_ERROR) apiErr;
       if ( GpiWCBitBlt( guts. ps, m, 4, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) {
          apiErr;
          GpiSetBitmap( guts. ps, nilHandle);
          GpiDeleteBitmap( sm);
          GpiDeleteBitmap( m);
          return nilHandle;
       }
       // restore old bitmap
       if ( GpiSetBitmap( guts. ps, old) == HBM_ERROR) apiErr;
       if ( b) if ( !GpiDeleteBitmap( b)) apiErr;
       if ( !GpiDeleteBitmap( m)) apiErr;
       m = sm;
       b = sb;
    }
    pi. hbmPointer = pi. hbmMiniPointer = m;
    pi. hbmColor   = pi. hbmMiniColor   = b;
    // create pointer
    ret = WinCreatePointerIndirect( HWND_DESKTOP, &pi);
    if ( !ret) {
       apiErr;
       if ( b) GpiDeleteBitmap( b);
       GpiDeleteBitmap( m);
       return nilHandle;
    }
    if ( b) GpiDeleteBitmap( b);
    GpiDeleteBitmap( m);
    return ret;
}

#ifdef USE_GPIDRAWBITS
static void
make_bitmap_cache( Handle from, Handle self)
{
    HDC hdc;
    HPS hps;
    PBITMAPINFO2 bi, br;
    HBITMAP bm = make_bitmap_ps( from, &hps, &hdc, &bi, cbImage);
    if ( bm == nilHandle) return;

    if ( sys bmInfo) free( sys bmInfo);
    if ( sys bmRaw)  free( sys bmRaw);

    br = get_screen_binfo();
    sys bmRaw = malloc((( br-> cBitCount * bi-> cx + 31) / 32) * br-> cPlanes * 4 * bi-> cy);
    sys bmInfo = br;
    if ( GpiSetBitmap( hps, bm) == HBM_ERROR) apiErr;
    if ( GpiQueryBitmapBits( hps, 0, bi-> cy, sys bmRaw, ( PBITMAPINFO2) br) < 0)
       apiErr;
    if ( GpiSetBitmap( hps, nilHandle) == HBM_ERROR) apiErr;
    if ( !GpiDeleteBitmap( bm)) apiErr;
    if ( !GpiDestroyPS( hps))   apiErr;
    if ( !DevCloseDC( hdc))     apiErr;
    free( bi);
}
#endif

static void
image_destroy_cache( Handle self)
{
#ifdef USE_GPIDRAWBITS
   if ( sys bmRaw)  free( sys bmRaw);
   if ( sys bmInfo) free( sys bmInfo);
   sys bmRaw =  nil;
   sys bmInfo = nil;
#else
   if ( sys bm) if ( !GpiDeleteBitmap( sys bm)) apiErr;
   sys bm = nilHandle;
#endif
}

static void
image_set_cache( Handle from, Handle self)
{
#ifdef USE_GPIDRAWBITS
   if ( sys bmRaw == nil)    make_bitmap_cache( from, self);
#else
   if ( sys bm == nilHandle) sys bm = ( HBITMAP) make_bitmap_handle( from);
#endif
}

Bool
apc_image_create( Handle self)
{
   image_destroy_cache( self);
   return true;
}

void
apc_image_update_change( Handle self)
{
   image_destroy_cache( self);
}

void
apc_image_destroy( Handle self)
{
   image_destroy_cache( self);
}

static Bool
image_begin_query( int primType, int * typeToConvert)
{
  Bool found = false;
  int i;
  for ( i = 0; i < guts. bmfCount; i++)
  {
     if ( primType == guts. bmf[ i * 2 + 1])
     {
        found = true;
        break;
     }
  }
  if ( !found)
  {
     if (( guts. bmf[ 1] == 15 || guts. bmf[ 1] == 16) &&
         ( primType == imbpp24)
        )
     found = true;
  }
  if ( found)
     *typeToConvert = primType;
   else
     *typeToConvert = ( guts. bmf[ 1] == 15 || guts. bmf[ 1] == 16) ?
       imbpp24 : guts. bmf[ 1];
  return !found;
}

#define putbmp(x,y,bm) {                                          \
  POINTL point = {x,y};                                               \
  HPS display = WinGetScreenPS( HWND_DESKTOP);                    \
  WinDrawBitmap(display,bm,nil,&point,CLR_WHITE,CLR_BLACK,DBM_NORMAL);\
  WinReleasePS( display);                                         \
}

Bool
apc_image_begin_paint( Handle self)
{
   PBITMAPINFO2 bi;
   HBITMAP cached = sys bm;
   HBITMAP ret;
   Handle deja = self;

   apcErrClear;
   sys bm = nilHandle;
   apc_image_end_paint( self);
   sys bm = cached;

   // creating HDC and HPS
   ret = make_bitmap_ps( self, &sys ps, &sys dc, &bi, cbNoBitmap);
   if ( ret == nilHandle) {
      image_destroy_cache( self);
      return false;
   }
   // creating bitmap ( or use cached)
   if ( sys bm == nilHandle) {
      Handle deja  = enscreen( self);
      bi           = get_binfo( deja);
      sys bm       = GpiCreateBitmap( sys ps, ( PBITMAPINFOHEADER2) bi, 0, nil, nil);
      if ( sys bm == nilHandle ) {
         apiErr;
         GpiDestroyPS( sys ps);
         sys ps = nilHandle;
         DevCloseDC( sys dc);
         free( bi);
         if ( deja != self) Object_destroy( deja);
         return false;
      }
   }
   // binding them together
   if ( GpiSetBitmap( sys ps, sys bm) == HBM_ERROR) apiErr;
   // setting bits
   if ( cached == nilHandle) {
      PImage i = ( PImage) deja;
      if ( GpiSetBitmapBits( sys ps, 0, i-> h, i-> data, bi) < 0) apiErr;
      if ( deja != self) Object_destroy( deja);
      free( bi);
   }

   apt_clear( aptFontExists);
   sys fontId = 0;
   sys fontHash = create_fontid_hash();
   apc_gp_set_color( self, sys lbs[ 0]);
   apc_gp_set_back_color( self, sys lbs[ 1]);
   apc_gp_set_font( self, &var font);
   save_gp_attrs( self);
   return true;
}


static void
image_query( Handle self, HPS ps)
{
   PBITMAPINFO2 bi;
   PImage image = ( PImage) self;
   int i, nColors;

   if ( is_apt( aptDeviceBitmap)) {
      apcErr(0x82002/* bitmap in use */);
      return;
   }

   bi = get_binfo( self);
   nColors = (( 1 << ( image-> type & imBPP)) & 0x1ff);
   if ( GpiQueryBitmapBits( ps, 0, image-> h, image-> data, ( PBITMAPINFO2) bi) < 0) apiErr;
   for ( i = 0; i < nColors; i++)
   {
     image-> palette[ i]. r = bi-> argbColor[ i]. bRed;
     image-> palette[ i]. g = bi-> argbColor[ i]. bGreen;
     image-> palette[ i]. b = bi-> argbColor[ i]. bBlue;
   }
   free( bi);
}

void
apc_image_end_paint( Handle self)
{
   PImage image = ( PImage) self;

   if ( image == nil) return;

   apcErrClear;
   if (( sys bm == nilHandle) || ( sys ps == nilHandle))
   {
      // destroying gpi guts if something's incorrect ( or free cache)
      if ( sys ps != nilHandle)
      {
         GpiSetBitmap( sys ps, nilHandle);
         GpiDestroyPS( sys ps);
      }
      image_destroy_cache( self);
      sys ps = nilHandle;
      return;
   }

   {
      int type, oldType = image-> type & imBPP;
      if ( image_begin_query( oldType, &type))
        image-> self-> create_empty( self, image-> w, image-> h, type);
      image_query( self, sys ps);
   }
   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   if ( sys fillBitmap)
     if ( !GpiDeleteBitmap( sys fillBitmap)) apiErr;
   sys fillBitmap = nilHandle;


   restore_gp_attrs( self);
   if ( GpiSetBitmap( sys ps, nilHandle) == HBM_ERROR) apiErr;
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
#ifdef USE_GPIDRAWBITS
   image_destroy_cache( self);
#endif
   sys ps = nilHandle;
   return;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
   PBITMAPINFO2 bi;
   HBITMAP ret;

   apcErrClear;
   sys bm = nilHandle;
   apt_set( aptDeviceBitmap);

   // creating HDC and HPS
   ret = make_bitmap_ps( self, &sys ps, &sys dc, &bi, monochrome ? cbMonoScreen : cbScreen);
   if ( ret == nilHandle) return false;
   free( bi);
   if ( GpiSetBitmap( sys ps, ret) == HBM_ERROR) apiErrRet;
   sys bm = ret;

   apt_clear( aptFontExists);
   sys fontId = 0;
   sys fontHash = create_fontid_hash();
   apc_gp_set_color( self, sys lbs[ 0]);
   apc_gp_set_back_color( self, sys lbs[ 1]);
   apc_gp_set_font( self, &var font);
   save_gp_attrs( self);
   return true;
}

void
apc_dbm_destroy( Handle self)
{
   apcErrClear;
   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   restore_gp_attrs( self);
   if ( GpiSetBitmap( sys ps, nilHandle) == HBM_ERROR) apiErr;
   if ( !GpiDeleteBitmap( sys bm)) apiErr;
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
   sys ps = sys dc = sys bm = nilHandle;
}


// IMAGE END
// GP

void
apc_gp_init( Handle self)
{
   apt_clear( aptFontExists);
   sys res = guts. displayResolution;
}

void
apc_gp_done( Handle self)
{
#ifdef USE_GPIDRAWBITS
   free( sys bmRaw); sys bmRaw = nil;
   free( sys bmInfo); sys bmInfo = nil;
#endif
   if ( sys bm)
      if ( !GpiDeleteBitmap( sys bm)) apiErr;
   if ( sys ps)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !WinEndPaint( sys ps)) apiErr;
      } else if ( is_apt( aptWinPS)) {
         if ( !WinReleasePS( sys ps)) apiErr;
      } else
         GpiSetBitmap( sys ps, nilHandle); // bitmapped PS
   }
   sys ps = sys bm = nilHandle;
   apt_clear( aptFontExists);
   free( sys charTable);
   free( sys saveFont);
}

// gpi various fixes

#define apc_gp_move( ps, x, y) {                                             \
                                  POINTL ___p = {x, y};                      \
                                  if ( !GpiMove( ps, &___p)) apiErr;         \
                               }

#define apc_gp_fix             if ( sys lineWidth > 0) {                     \
                                  if ( !GpiBeginPath( sys ps, 1)) apiErr;    \
                               }

#define apc_gp_fix_end         if ( sys lineWidth > 0) {                     \
                                  if ( !GpiEndPath( sys ps)) apiErr;         \
                                  if ( !GpiModifyPath( sys ps, 1, MPATH_STROKE)) apiErr;    \
                                  if ( GpiFillPath( sys ps, 1, FPATH_WINDING) == GPI_ERROR) \
                                      apiErr;                                \
                                }

void                                                     /* no fix */
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   POINTL p;
   apc_gp_move( sys ps, x1, y1);
   p. x = x2; p. y = y2;
   if ( GpiBox( sys ps, DRO_FILL, &p, 0, 0) == GPI_ERROR) apiErr;
}

void
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
   POINTL p;
   apc_gp_move( sys ps, x1, y1);
   p. x = x2; p. y = y2;
   apc_gp_fix;
   if ( GpiBox( sys ps, DRO_OUTLINE, &p, 0, 0) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
}

void                                                        /* no fix */
apc_gp_fill_ellipse( Handle self, int x, int y, int Rx, int Ry)
{
   ARCPARAMS arc;

   arc. lP = Rx;
   arc. lQ = Ry;
   arc. lR = 0;
   arc. lS = 0;
   if ( !GpiSetArcParams( sys ps, &arc)) apiErr;
   apc_gp_move( sys ps, x, y);
   if ( GpiFullArc( sys ps, DRO_FILL, MAKEFIXED(1, 0)) == GPI_ERROR) apiErr;
}

void
apc_gp_ellipse( Handle self, int x, int y, int Rx, int Ry)
{
   ARCPARAMS arc;

   arc. lP = Rx;
   arc. lQ = Ry;
   arc. lR = 0;
   arc. lS = 0;
   if ( !GpiSetArcParams( sys ps, &arc)) apiErr;
   apc_gp_move( sys ps, x, y);
   apc_gp_fix;
   if ( GpiFullArc( sys ps, DRO_OUTLINE, MAKEFIXED(1, 0)) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
}

void
apc_gp_draw_poly( Handle self, int numPts, Point * points)
{
   if ( numPts <= 1) return;
   apc_gp_move( sys ps, points[0].x, points[0].y);
   apc_gp_fix;
   if ( GpiPolyLine( sys ps, numPts - 1, ( PPOINTL)&points[1]) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
}

void
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{
   if ( numPts <= 1) return;
   apc_gp_move( sys ps, points[0].x, points[0].y);
   apc_gp_fix;
   if ( GpiPolyLineDisjoint( sys ps, numPts - 1, ( PPOINTL)&points[1]) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
}


void
apc_gp_fill_poly( Handle self, int numPts, Point * points)
{
   if ( numPts <= 1) return;
   apc_gp_move( sys ps, points[0].x, points[0].y);
   if ( !GpiBeginPath( sys ps, 1)) apiErr;
   if ( GpiPolyLine( sys ps, numPts - 1, ( PPOINTL)&points[1]) == GPI_ERROR) apiErr;
   if ( !GpiEndPath( sys ps)) apiErr;
   if ( GpiFillPath( sys ps, 1, FPATH_ALTERNATE) == GPI_ERROR) apiErr;
}

#define gp_arc_set {               \
  ARCPARAMS arc;                   \
  arc. lP = radX;                  \
  arc. lQ = radY;                  \
  arc. lR = 0;                     \
  arc. lS = 0;                     \
  if ( !GpiSetArcParams( sys ps, &arc)) apiErr;  \
  if ( angleStart > angleEnd)      \
  {                                \
     double a = angleEnd;          \
     angleEnd = angleStart;        \
     angleStart = a;               \
  }                                \
}

void
apc_gp_arc ( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
  POINTL ptl = { x, y};
  LONG lType = GpiQueryLineType( sys ps);
  gp_arc_set;
  if ( !GpiSetLineType ( sys ps, LINETYPE_INVISIBLE)) apiErr;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), MAKEFIXED(0, 0)) == GPI_ERROR) apiErr;
  if ( !GpiSetLineType ( sys ps, lType)) apiErr;
  apc_gp_fix;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  apc_gp_fix_end;
}

void
apc_gp_chord ( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
  POINTL ptl = { x, y};
  LONG lType = GpiQueryLineType( sys ps);
  gp_arc_set;
  if ( !GpiSetLineType ( sys ps, LINETYPE_INVISIBLE)) apiErr;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  if ( !GpiSetLineType ( sys ps, lType)) apiErr;
  apc_gp_fix;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  apc_gp_fix_end;
}

void
apc_gp_fill_chord ( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
  POINTL ptl = { x, y};
  LONG lType = GpiQueryLineType( sys ps);
  gp_arc_set;
  if ( !GpiSetLineType ( sys ps, LINETYPE_INVISIBLE)) apiErr;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  if ( !GpiSetLineType ( sys ps, lType)) apiErr;
  if ( !GpiBeginPath( sys ps, 1)) apiErr;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  if ( !GpiEndPath( sys ps)) apiErr;
  if ( GpiFillPath( sys ps, 1, FPATH_ALTERNATE) == GPI_ERROR) apiErr;
}

void
apc_gp_sector ( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
  POINTL ptl = { x, y};
  gp_arc_set;
  if ( !GpiMove( sys ps, &ptl)) apiErr;
  apc_gp_fix;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  if ( GpiLine( sys ps, &ptl) == GPI_ERROR) apiErr;
  apc_gp_fix_end;
}

void
apc_gp_fill_sector ( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
  POINTL ptl = { x, y};
  gp_arc_set;
  if ( !GpiBeginPath( sys ps, 1)) apiErr;
  if ( !GpiMove( sys ps, &ptl)) apiErr;
  if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 1, 0), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
  if ( GpiLine( sys ps, &ptl) == GPI_ERROR) apiErr;
  if ( !GpiEndPath( sys ps)) apiErr;
  if ( GpiFillPath( sys ps, 1, FPATH_ALTERNATE) == GPI_ERROR) apiErr;
}


void
apc_gp_clear( Handle self)                                 /* no fix */
{
   if ( !GpiErase( sys ps)) apiErr;
}

void apc_gp_line ( Handle self, int x1, int y1, int x2, int y2)
{
   POINTL p = {x2, y2};
   apc_gp_move( sys ps, x1, y1);
   apc_gp_fix;
   if ( GpiLine( sys ps, &p) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
}

void                                                       /* no fix */
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{
   apc_gp_move( sys ps, x, y);
   if ( GpiFloodFill( sys ps, singleBorder ? FF_SURFACE : FF_BOUNDARY, remap_color ( sys ps, borderColor, true)) == GPI_ERROR) apiErr;
}

#define gp_font_check     if ( !is_apt( aptFontExists)) {          \
                             apt_set( aptFontExists);              \
                             apc_gp_set_font( self, &sys font);    \
                          }


void                                                        /* no fix */
apc_gp_text_out( Handle self, char * text, int x, int y, int len)
{
   POINTL p = {x, y};
   POINTL pt[ TXTBOX_COUNT];
   Bool   opaque = sys textOpaque;

   gp_font_check;
   if ( len > 512) apc_gp_move( sys ps, 0, 0);
   while ( len > 0)
   {
      int drawLen = ( len > 512) ? 512 : len;
      if ( opaque || len > 0)
         if ( GpiQueryTextBox( sys ps, drawLen, text, TXTBOX_COUNT, pt) == GPI_ERROR) apiErr;
      if ( opaque) {
         LONG c = GpiQueryColor( sys ps);
         int i;
         POINTL z = pt[2];
         pt[2] = pt[3];
         pt[3] = z;
         for ( i = 0; i < 5; i++) { pt[i].x+=x; pt[i].y+=y;}
         GpiSetColor( sys ps, GpiQueryBackColor( sys ps));
         apc_gp_fill_poly( self, 4, ( Point*) pt);
         GpiSetColor( sys ps, c);
      }
      if ( GpiCharStringAt ( sys ps, &p, drawLen, text) == GPI_ERROR) apiErr;
      len -= 512;
      if ( len <= 0) return;
      p. x += pt[ TXTBOX_CONCAT]. x;
      text += 512;
   }
}

void                                                         /* no fix */
apc_gp_set_pixel ( Handle self, int x, int y, Color color)
{
  POINTL p = {x, y};
  COLOR c = GpiQueryColor ( sys ps);
  GpiSetColor ( sys ps, remap_color( sys ps, color, true));
  if ( !GpiSetPel ( sys ps, &p)) apiErr;
  GpiSetColor( sys ps, c);
}

Color
apc_gp_get_pixel ( Handle self, int x, int y)
{
   POINTL p = {x, y};
   return remap_color( sys ps, GpiQueryPel ( sys ps, &p), false);
}

static int ctx_rop2ROP[] = {
   ropCopyPut,      ROP_SRCCOPY,
   ropOrPut,        ROP_SRCPAINT,
   ropAndPut,       ROP_SRCAND,
   ropXorPut,       ROP_SRCINVERT,
   ropNotPut,       ROP_NOTSRCCOPY,
   ropNotDestAnd,   ROP_NOTSRCERASE,
   ropErase,        ROP_NOTSRCERASE,
   ropAndPattern,   ROP_MERGECOPY,
   ropNotSrcOr,     ROP_MERGEPAINT,
   ropPattern,      ROP_PATCOPY,
   ropNotSrcOrPat , ROP_PATPAINT,
   ropXorPattern,   ROP_PATINVERT,
   ropInvert,       ROP_DSTINVERT,
   ropBlackness,    ROP_ZERO,
   ropWhiteness,    ROP_ONE,
   endCtx
};

void                                                                /* no fix */
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
  apc_gp_stretch_image ( self, image, x, y, xFrom, yFrom, xLen, yLen, xLen, yLen, rop);
}

void                                                                 /* no fix */
apc_gp_stretch_image ( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   PBITMAPINFO2 b;
   POINTL pt [4];
   PIcon i = ( PIcon) image;
   Color saveFore;
   Color saveBack;
   Handle deja = enscreen( image);

   b = get_binfo( deja);
   // clipping & positioning rectangles
   pt [0]. x = x;                    // dest from
   pt [0]. y = y;
   pt [1]. x = x + xDestLen - 1;     // dest to
   pt [1]. y = y + yDestLen - 1;
   pt [2]. x = xFrom;                // src from
   pt [2]. y = yFrom;
   pt [3]. x = xFrom + xLen;         // src to
   pt [3]. y = yFrom + yLen;

   if ( kind_of( deja, CIcon))
   {
      BInfo2 mono = {
        sizeof ( BInfo),
        i-> w,
        i-> h,
        1, 1,
        {{ 0xFF, 0xFF, 0xFF, 0}, { 0, 0, 0, 0}}
      };
      saveFore = GpiQueryColor( sys ps);
      saveBack = GpiQueryBackColor( sys ps);
      GpiSetBackColor( sys ps, guts. monoBitsOk ? CLR_WHITE : CLR_BLACK);
      GpiSetColor( sys ps, guts. monoBitsOk ? CLR_BLACK : CLR_WHITE);
      if ( GpiDrawBits( sys ps, i-> mask, ( PBITMAPINFO2) &mono, 4, pt, ROP_SRCAND, BBO_IGNORE) == GPI_ERROR) apiErr;
      if ( b-> cBitCount == imMono)
      {
         GpiSetColor( sys ps, ARGB( i-> palette[ 1]. r, i-> palette[ 1]. g, i-> palette[ 1]. b));
         GpiSetBackColor( sys ps, CLR_BLACK);
      }
#ifdef USE_GPIDRAWBITS
      if ( is_apt( aptCompatiblePS)) {
         image_set_cache( deja, image);
         if ( GpiDrawBits( sys ps, dsys( image) bmRaw, dsys( image) bmInfo, 4, pt, ROP_SRCINVERT, BBO_IGNORE) == GPI_ERROR) apiErr;
      } else {
         HBITMAP bm = ( dsys( image) bm == nilHandle) ? (( HBITMAP) make_bitmap_handle( deja)) : ( dsys( image) bm);
         if ( bm)
                if ( GpiWCBitBlt( sys ps, bm, 4, pt, ROP_SRCINVERT, BBO_IGNORE) == GPI_ERROR) apiErr;
         if ( bm && bm != dsys( image) bm) GpiDeleteBitmap( bm);
      }
#else
      image_set_cache( deja, image);
      if ( dsys( image) bm)
         if ( GpiWCBitBlt( sys ps, dsys( image) bm, 4, pt, ROP_SRCINVERT, BBO_IGNORE) == GPI_ERROR) apiErr;
#endif
      GpiSetColor( sys ps, saveFore);
      GpiSetBackColor( sys ps, saveBack);
   } else {
      Bool db = ( dsys( image) options & aptDeviceBitmap) ? 1 : 0;
      if ( b-> cBitCount == imMono)
      {
         long cr[ 2];
         cr[0] = db ? 0xFFFFFF :
            remap_color( sys ps, ARGB( i-> palette[ 1]. r, i-> palette[ 1]. g, i-> palette[ 1]. b), true);
         cr[1] = db ? 0x000000 :
            remap_color( sys ps, ARGB( i-> palette[ 0]. r, i-> palette[ 0]. g, i-> palette[ 0]. b), true);
         if ( cr[1] == clBlack)
         {
            long tmp = cr[0];
            cr[0] = cr[1];
            cr[1] = tmp;
         }
         saveFore = GpiQueryColor( sys ps);
         saveBack = GpiQueryBackColor( sys ps);
         GpiSetColor( sys ps, cr[1]);
         GpiSetBackColor( sys ps, cr[0]);
      }
      if ( db) {
         if ( GpiWCBitBlt( sys ps, dsys( image) bm, 4, pt, ctx_remap( rop, ctx_rop2ROP, true), BBO_IGNORE) == GPI_ERROR) apiErr;
      } else {
#ifdef USE_GPIDRAWBITS
         if ( is_apt( aptCompatiblePS)) {
            image_set_cache( deja, image);
            if ( GpiDrawBits( sys ps, dsys( image) bmRaw, dsys( image) bmInfo, 4, pt, ctx_remap( rop, ctx_rop2ROP, true), BBO_IGNORE) == GPI_ERROR) apiErr;
         } else {
            HBITMAP bm = ( dsys( image) bm == nilHandle) ? (( HBITMAP) make_bitmap_handle( deja)) : ( dsys( image) bm);
            if ( bm)
                if ( GpiWCBitBlt( sys ps, bm, 4, pt, ctx_remap( rop, ctx_rop2ROP, true), BBO_IGNORE) == GPI_ERROR) apiErr;
            if ( bm && bm != dsys( image) bm) GpiDeleteBitmap( bm);
         }
#else
         image_set_cache( deja, image);
         if ( dsys( image) bm)
                if ( GpiWCBitBlt( sys ps, dsys( image) bm, 4, pt, ctx_remap( rop, ctx_rop2ROP, true), BBO_IGNORE) == GPI_ERROR) apiErr;
#endif
      }
      if ( !guts. monoBitsOk && b-> cBitCount == imMono)
      {
         GpiSetColor( sys ps, saveFore);
         GpiSetBackColor( sys ps, saveBack);
      }
   }
   free( b);
   if ( deja != image) Object_destroy( deja);
}


// GP SET+GET

void
apc_gp_set_transform( Handle self, int x, int y)
{
   MATRIXLF m = { MAKEFIXED( 1, 0), 0, 0, 0, MAKEFIXED( 1, 0), 0,
      x - sys transform2. x, y - sys transform2. y};
   if ( !sys ps)
   {
      sys transform = ( Point){ x, y};
      return;
   }
   if ( !GpiSetDefaultViewMatrix( sys ps, 8, &m, TRANSFORM_REPLACE)) apiErr;
}

Point
apc_gp_get_transform( Handle self)
{
   MATRIXLF m = {0,0,0,0,0,0,0,0};
   if ( !sys ps) return sys transform;
   if ( !GpiQueryDefaultViewMatrix( sys ps, 8, &m)) apiErr;
   return ( Point) { m. lM31 + sys transform2. x, m. lM32 + sys transform2. y};
}

void
apc_gp_set_color( Handle self, Color color)
{
   COLOR clr = remap_color( sys ps, color, true);
   if ( !sys ps) sys lbs[0] = color; else
   if ( !GpiSetColor( sys ps, clr)) apiErr;
}

void
apc_gp_set_clip_rect( Handle self, Rect clipRect)
{
   HRGN  rgn, old;
   if ( !is_opt( optInDraw) || !sys ps) return;
   clipRect. left   -= sys transform2. x;
   clipRect. right  -= sys transform2. x;
   clipRect. top    -= sys transform2. y;
   clipRect. bottom -= sys transform2. y;
   if ( !( rgn = GpiCreateRegion( sys ps, 1, ( PRECTL) &clipRect))) apiErr;
   if ( GpiSetClipRegion( sys ps, rgn, &old) == RGN_ERROR) apiErr;
   if ( old) if ( !GpiDestroyRegion( sys ps, old)) apiErr;
}

void
apc_gp_set_back_color( Handle self, Color color)
{
   COLOR clr = remap_color( sys ps, color, true);
   if ( !sys ps) sys lbs[1] = color; else
   if ( !GpiSetBackColor( sys ps, clr)) apiErr;
}


static int ctx_rop2FM[] = {
   ropCopyPut      , FM_DEFAULT,
   ropCopyPut      , FM_OVERPAINT,
   ropXorPut       , FM_XOR,
   ropAndPut       , FM_AND,
   ropOrPut        , FM_OR,
   ropNotPut       , FM_NOTCOPYSRC,
   ropNotDestOr    , FM_MERGESRCNOT,
   ropNotSrcAnd    , FM_MASKSRCNOT,
   ropNotSrcOr     , FM_MERGENOTSRC,
   ropNotXor       , FM_NOTXORSRC,
   ropNotAnd       , FM_NOTMASKSRC,
   ropNotOr        , FM_NOTMERGESRC,
   ropNoOper       , FM_LEAVEALONE,
   ropBlackness    , FM_ZERO,
   ropWhiteness    , FM_ONE,
   ropInvert       , FM_INVERT,
   endCtx
};

static int ctx_rop2BM[] = {
   ropCopyPut      , BM_OVERPAINT,
   ropXorPut       , BM_XOR,
   ropOrPut        , BM_OR,
   ropNoOper       , BM_LEAVEALONE,
   ropNoOper       , BM_DEFAULT,
   ropSrcLeave     , BM_SRCTRANSPARENT,
   ropDestLeave    , BM_DESTTRANSPARENT,
   endCtx
};


// centering around PRIM_LINE settings
int
apc_gp_get_rop ( Handle self)
{
   if ( !sys ps) return sys rop;
   return ctx_remap( GpiQueryMix( sys ps), ctx_rop2FM, false);
}

void
apc_gp_set_rop ( Handle self, int rop)
{
   if ( !sys ps) { sys rop = rop; return; }
   if ( !GpiSetMix( sys ps, ctx_remap( rop, ctx_rop2FM, true))) apiErr;
}

int
apc_gp_get_rop2 ( Handle self)
{
   if ( !sys ps) return sys rop2;
   return ctx_remap( GpiQueryBackMix( sys ps), ctx_rop2BM, false);
}

void
apc_gp_set_rop2 ( Handle self, int rop)
{
   if ( !sys ps) { sys rop2 = rop; return; }
   if ( !GpiSetBackMix( sys ps, ctx_remap( rop, ctx_rop2BM, true))) apiErr;
}

Color
apc_gp_get_color( Handle self)
{
   if ( !sys ps) return sys lbs[0];
   return remap_color( sys ps, GpiQueryColor( sys ps), false);
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   Rect r = {0,0,0,0};
   if ( !is_opt( optInDraw))
   {
      Point size = var self-> get_size( self);
      r. right = size. x;
      r. top   = size. y;
      return r;
   }
   if ( GpiQueryClipBox( sys ps, ( PRECTL)&r) == RGN_ERROR) apiErr;
   r. left   += sys transform2. x;
   r. right  += sys transform2. x;
   r. top    += sys transform2. y;
   r. bottom += sys transform2. y;
   return r;
}

Color
apc_gp_get_back_color( Handle self)
{
   if ( !sys ps) return sys lbs[1];
   return remap_color( sys ps, GpiQueryBackColor( sys ps), false);
}

void
apc_gp_set_line_width( Handle self, int lineWidth)
{
   sys lineWidth = lineWidth;
   if ( !sys ps) return;
   if ( lineWidth == 0) {
      if ( !GpiSetLineWidth( sys ps, MAKEFIXED( lineWidth, 0))) apiErr;
   } else {
      if ( !GpiSetLineWidthGeom( sys ps, lineWidth)) apiErr;
   }
}

int
apc_gp_get_line_width( Handle self)
{
   return sys lineWidth;
}

void
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
  sys textOpaque = opaque;
}

Bool
apc_gp_get_text_opaque( Handle self)
{
   return sys textOpaque;
}

static int ctx_ps2LINETYPE[] = {
    lpDot,            LINETYPE_DOT               ,
    lpShortDash,      LINETYPE_SHORTDASH         ,
    lpDash,           LINETYPE_SHORTDASH         ,
    lpDashDot,        LINETYPE_DASHDOT           ,
    lpDotDot,         LINETYPE_DOUBLEDOT         ,
    lpLongDash,       LINETYPE_LONGDASH          ,
    lpDashDotDot,     LINETYPE_DASHDOUBLEDOT     ,
    lpSolid,          LINETYPE_SOLID             ,
    lpNull,           LINETYPE_INVISIBLE         ,
    endCtx
};

void
apc_gp_set_line_pattern ( Handle self, int pattern)
{
   if ( !sys ps) { sys linePattern = pattern; return; }
   if ( !GpiSetLineType( sys ps, ctx_remap_def( pattern, ctx_ps2LINETYPE, true, LINETYPE_DEFAULT))) apiErr;
}

int
apc_gp_get_line_pattern ( Handle self)
{
   if ( !sys ps) return sys linePattern;
   return ctx_remap_def( GpiQueryLineType( sys ps), ctx_ps2LINETYPE, false, lpSolid);
}


static int ctx_le2LINEEND [] = {
   leFlat      ,  LINEEND_FLAT    ,
   leSquare    ,  LINEEND_SQUARE  ,
   leRound     ,  LINEEND_ROUND   ,
   endCtx
};

void
apc_gp_set_line_end ( Handle self, int lineEnd)
{
   if ( !sys ps) { sys lineEnd = lineEnd; return; }
   if ( !GpiSetLineEnd( sys ps, ctx_remap_def( lineEnd, ctx_le2LINEEND, true, LINEEND_DEFAULT))) apiErr;
}

int
apc_gp_get_line_end ( Handle self)
{
   if ( !sys ps) return sys lineEnd;
   return ctx_remap_def( GpiQueryLineEnd( sys ps), ctx_le2LINEEND, false, leFlat);
}

#define GRAD 572.9577951


static void
gp_set_font_extra( Handle self, HPS ps, int fontId, PSIZEF sz, Bool vectored, PFont font)
{
   GRADIENTL g;
   if ( !GpiSetCharSet( ps, fontId)) apiErr;
   if ( vectored) {
      Bool isExtra = is_apt( aptExtraFont);
      if ( !GpiSetCharBox( ps, sz)) apiErr;

      if ( font-> direction != 0) {
         g = ( GRADIENTL) { ( long) ( cos( font-> direction / GRAD) * 1000) , ( long) ( sin( font-> direction / GRAD) * 1000)};
         if ( !GpiSetCharAngle( ps, &g)) apiErr;
         apt_set( aptExtraFont);
      } else if ( isExtra) {
         g = ( GRADIENTL) { 0, 0};
         if ( !GpiSetCharAngle( ps, &g)) apiErr;
      }
   }
}

void
apc_gp_set_font( Handle self, PFont font)
{
   // very simple & uneffective gpi font management;
   // does not caches fonts, uses only one local ID = 1.
   FATTRS f = {
      sizeof ( FATTRS),
      0,
      0,                   // does not force match
      "",                  // facename
      0,                   // default registry
      guts. codePage,      // current code page
      0,                   // height
      0,                   // width
      0,
      FATTR_FONTUSE_TRANSFORMABLE
   };
   Bool vectored;
   SIZEF sz;
   int fontId;

   if ( &sys font != font) sys font = *font;
   if ( !is_apt( aptFontExists)) return;

   apcErrClear;
   if ( sys fontHash)
   {
      int fontId;
      font-> resolution = sys res.y * 0x10000 + sys res. x;
      fontId = get_fontid_from_hash( sys fontHash, font, &sz, (int*)&vectored);
      if ( fontId)
      {
         gp_set_font_extra( self, sys ps, fontId, &sz, vectored, font);
         return;
      }
   }

   vectored = font_font2gp( font, sys res, false);
   if ( apcError != 0) return;

   if ( vectored) {
      sz = ( SIZEF)
      {
         MAKEFIXED( font-> width,  0),
         MAKEFIXED( font-> height, 0)
//         float2fixed( font-> size * font-> aspect * sys res. x / 72.0),
//         float2fixed( font-> size * sys res. y / 72.0)
      };
   } else {
      f. lAveCharWidth   = font-> width;
      f. lMaxBaselineExt = font-> height;
      f. fsFontUse = FATTR_FONTUSE_NOMIX;
   }
   if ( font-> style & fsItalic    ) f. fsSelection |= FATTR_SEL_ITALIC;
   if ( font-> style & fsUnderlined) f. fsSelection |= FATTR_SEL_UNDERSCORE;
   if ( font-> style & fsStruckOut ) f. fsSelection |= FATTR_SEL_STRIKEOUT;
   if ( font-> style & fsBold      ) f. fsSelection |= FATTR_SEL_BOLD;
   if ( font-> style & fsOutline   ) f. fsSelection |= FATTR_SEL_OUTLINE;

   memcpy( f. szFacename, font-> name, FACESIZE);
   if ( sys fontHash) {
      (sys fontId)++;
      if ( sys fontId > 254) sys fontId = 1;
      fontId = sys fontId;
   } else
      fontId = 1;

   if ( !GpiDeleteSetId( sys ps, fontId)) {
      rc = WinGetLastError( guts. anchor);
      if ( LOUSHORT(rc) != 0x2103 /*PMERR_SETID_NOT_FOUND*/) apiAltErr( rc);
   }
   if ( GpiCreateLogFont( sys ps, nil, fontId, &f) == GPI_ERROR) apiErr;

   gp_set_font_extra( self, sys ps, fontId, &sz, vectored, font);

   if ( sys fontHash) {
      font-> resolution = sys res. y * 0x10000 + sys res. x;
      add_fontid_to_hash( sys fontHash, sys fontId, font, &sz, vectored);
   }
}


PFont
gp_get_font( HPS ps, PFont font, Point res)
{
   Bool vector;
   if ( !GpiQueryFontMetrics( ps, sizeof( fmtx), &fmtx)) { apiErr; return font; }
   vector = fmtx. fsDefn & FM_DEFN_OUTLINE;
   font-> direction   = 0;
   if ( vector)
   {
      SIZEF sz;
      GRADIENTL g;
      GpiQueryCharBox( ps, &sz);
      font-> width   = FIXEDINT( sz. cx);
      font-> height  = FIXEDINT( sz. cy);
      font-> size    = fixed2float( sz. cy) * 72.0 / res. y;
//      float x       = fixed2float( sz. cx) * 72.0 / res. x;
//      float y       = fixed2float( sz. cy) * 72.0 / res. y;
//      font-> size   = y + 0.5;
//      font-> aspect = x / y;
      GpiQueryCharAngle( ps, &g);
      if ( g. x != 0 || g. y != 0) font-> direction   = atan2( g. y, g. x) * GRAD;
   } else {
      font-> height = fmtx. lMaxBaselineExt;
      font-> width  = fmtx. lMaxCharInc;
      font-> size   = fmtx. sNominalPointSize / 10;
   }
   font-> pitch  =
      (( fmtx. fsType & FM_TYPE_FIXED   ) ? fpFixed  : fpVariable) |
      (( fmtx. fsDefn & FM_DEFN_OUTLINE ) ? fpVector : fpRaster);
   if ( fmtx. fsType & FM_TYPE_FACETRUNC)
      WinQueryAtomName( WinQuerySystemAtomTable(), fmtx. FaceNameAtom, font-> name, 256);
   else
      strcpy ( font-> name, fmtx. szFacename);
   font-> ascent  = fmtx. lMaxAscender;
   font-> descent = fmtx. lMaxDescender;
   font-> resolution = res. y * 0x10000 + res. x;
   return font;
}

PFont
apc_gp_get_font( Handle self, PFont font)
{
   if ( !is_apt( aptFontExists))
   {
      *font = sys font;
      return font;
   }
   font-> style  = sys font. style;
   return gp_get_font( sys ps, font, sys res);
}

// these are pure actions for requesting some standalone font metrics from any drawable,
// no matter whether they are in or not in paint state ( e.g exists PS or not).
static void
gp_enter_font( Handle self, Font * requiredFont, Font * cache)
{
   // if D(rawable) not in paint state, use guts. ps ( need a ps, anyway)
   // and set up some font-related fields
   if ( !is_opt( optInDraw))
   {
      sys ps = guts. ps;
      sys fontId = guts. fontId;                    //  for font - hashing
      sys fontHash = guts. fontHash;                //
      GpiSetCharSet( guts. ps, LCID_DEFAULT);
      if ( requiredFont == nil) apc_gp_set_font( self, &var font); // and font itself
   }
   // if requesting standalone font, current font should be saved.
   if ( requiredFont) {
      Font res = var font;
      // But this don't needed when D not in paint state, therefore,
      // font changes will be skipped. Otherwise, we'll need
      // Font cache to save current one.
      if ( is_opt( optInDraw)) {
         if ( is_apt( aptFontExists))
            gp_get_font( self, cache, sys res);
         else {
            *cache = sys font;
            apt_set( aptFontExists);
         }
      } else
         apt_set( aptFontExists);
      // so, picking and setting font on PS
      apc_font_pick( self, requiredFont, &res);
      apc_gp_set_font( self, &res);
   } else
      gp_font_check;
}

static void
gp_leave_font( Handle self, Font * requiredFont, Font * cache)
{
   // if font was saved, so, it should be restored.
   if ( requiredFont && is_opt( optInDraw)) apc_gp_set_font( self, cache);
   // and that middle-state, if D was not painting, should be set to previous
   // by nulling related fields
   if ( !is_opt( optInDraw)) {
      sys ps = nilHandle;
      sys fontId = 0;
      sys fontHash = nil;
      apt_clear( aptFontExists);
   }
}

int
apc_gp_get_text_width ( Handle self, char * text, int len, Bool addOverhang, Font *font)
{
   POINTL pt[ TXTBOX_COUNT];
   Font x;
   gp_enter_font( self, font, &x);
   apc_gp_move( sys ps,0,0);
   if ( !GpiQueryTextBox( sys ps, len, text, TXTBOX_COUNT, pt)) apiErr;
   gp_leave_font( self, font, &x);
   return pt[ TXTBOX_CONCAT]. x + ( addOverhang ? pt[ TXTBOX_BOTTOMLEFT]. x : 0);
}

Point *
apc_gp_get_text_box( Handle self, char * text, int len, Font *font)
{
   POINTL * pt = malloc( sizeof( POINTL) * TXTBOX_COUNT);
   Font x;
   gp_enter_font( self, font, &x);
   apc_gp_move(sys ps,0,0);
   if ( !GpiQueryTextBox( sys ps, len, text, TXTBOX_COUNT, pt)) apiErr;
   gp_leave_font( self, font, &x);
   return ( Point *) pt;
}


static char **
gp_text_wrap( Handle self, TextWrapRec * t)
{
    int start = 0, i, lSize = 16;
    float * ct     = sys charTable;
    float w=0, inc = ct[ 256];
    char ** ret    = malloc( sizeof( char*) * lSize);
    Bool wasTab    = 0;
    Bool doWidthBreak = t-> width >= 0;
    int tildeIndex = -100, tildeLPos, tildeLine, tildePos;
    unsigned char * text    = ( unsigned char*) t-> text;
    POINTL pt[ TXTBOX_COUNT];

// macro for adding string/chunk into result table
#define lAdd(end) {                                       \
   int l = end - start;                                   \
   char * c;                                              \
   if (!( t-> options & twReturnChunks)) {                \
      c = malloc( l + 1);                                 \
      memcpy( c, &text[start], l);                        \
      c[ l] = 0;                                          \
   }                                                      \
   if ( tildeIndex >= 0 && tildeIndex >= start &&         \
        tildeIndex < end)                                 \
   {                                                      \
      tildeLine = t-> t_line = t-> count;                 \
      tildePos = tildeLPos   = tildeIndex - start;        \
      if ( tildeIndex == end - 1) {                       \
         t-> t_line++;                                    \
         tildeLPos = 0;                                   \
      }                                                   \
   }                                                      \
   if ( t-> count == lSize) {                             \
      char ** n = malloc( sizeof( char*) * ( lSize + 16));\
      memcpy( n, ret, sizeof( char*) * lSize);            \
      lSize += 16;                                        \
      free( ret);                                         \
      ret = n;                                            \
   }                                                      \
   if ( t-> options & twReturnChunks) {                   \
      ret[ t-> count++] = (char*) start;                  \
      ret[ t-> count++] = (char*) l;                      \
   } else                                                 \
      ret[ t-> count++] = c;                              \
   start += l;                                            \
}

// determining ~ character location
    if ( t-> options & twCalcMnemonic) {
       for ( i = 0; i < t-> textLen - 1; i++)
          if ( text[ i] == '~') {
             char c = text[ i + 1];
             if ( c == '~' || c < ' ') {
                i++;
                continue;
             } else {
                tildeIndex = i;
                break;
             }
          }
    }
// scanning line accumulating widths and breaking if necessary
    t-> count = 0;
    for ( i = 0; i < t-> textLen; i++)
    {
       float winc;

       switch ( text[ i])
       {
          case '\t':
             if (!( t-> options & twCalcTabs)) goto _default;
             if ( t-> options & twSpaceBreak)
             {
                lAdd( i); start++; w = 0;
                continue;
             }
             winc = ct[' '] * t-> tabIndent;
             wasTab = true;
             break;
          case '\n':
             if (!( t-> options & twNewLineBreak)) goto _default;
             lAdd( i); start++; w = 0;
             continue;
          case ' ':
             if (!( t-> options & twSpaceBreak)) goto _default;
             lAdd( i); start++; w = 0;
             continue;
          case '~':
             if ( i == tildeIndex) {
                winc = 0;
                break;
             }
          _default: default:
             winc = ct[ text[ i]];
       }
       if ( doWidthBreak && w + winc + inc > t-> width)
       {
          if (( i - start == 0) || (( i - 1 == tildeIndex) && ( i - start == 1))) {
            // case when even single char could not fit in given width.
             if ( t-> options & twBreakSingle)
             {
                // do not return anything in this case
                int j;
                if (!( t-> options & twReturnChunks)) {
                   for ( j = 0; j < t-> count; j++) free( ret[ j]);
                   ret[ 0] = malloc(1);
                   ret[ 0][ 0] = 0;
                }
                t-> count = 0;
                return ret;
             } else
                // or fit this character
                lAdd( i + 1);
          } else {
             char * c;
             lAdd( i);
             if ( t-> options & twWordBreak)
             {
                // checking whether break was at word boundary
                char rc = text[ i];
                int len;
                if ( t-> options & twReturnChunks)
                {
                   c   = &text[ (int)ret[ t-> count - 2]];
                   len = (int) ret[ t-> count - 1];
                }
                else  {
                  c = ret[ t-> count - 1];
                  len = strlen( c);
                }
                if ( rc != ' ' && rc != '\t' && rc != '\n') {
                   // determining whether this line could be split
                   int j;
                   Bool ok = false;
                   for ( j = len; j >= 0; j--)
                      if ( c[ j] == ' ' || c[ j] == '\n' || c[ j] == '\t') {
                         ok = true;
                         break;
                      }
                   if ( ok)
                   {
                      start -= len - j - 1;
                      if ( t-> options & twReturnChunks)
                         (int) ret[ t-> count - 1] = j;
                      else
                         c[ j] = 0;
                      i -= len - j - 1;
                   }
                }
             }
             i--; // repeat again
          }
          w = 0;
          continue;
       } else
          w += winc;
    }
// adding or skipping last line
    if ( t-> textLen - start > 0 || t-> count == 0) lAdd( t-> textLen);
// expanding tabs
    if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab)
    {
       for ( i = 0; i < t-> count; i++)
       {
          int tabs = 0, len = 0;
          char *substr = ret[ i], *n;
          while (*substr) { if ( *substr == '\t') tabs++; substr++; len++; }
          if ( tabs == 0) continue;
          n = malloc( len + tabs * t-> tabIndent + 1);
          substr = ret[ i];
          len = 0;
          while ( *substr)
          {
             if ( *substr == '\t')
             {
                int j = t-> tabIndent;
                while ( j--) n[ len++] = ' ';
             } else
                n[ len++] = *substr;
             substr++;
          }
          free( ret[ i]);
          n[ len] = 0;
          ret[ i] = n;
       }
    }
// removing ~ and determining it's location
    if ( tildeIndex >= 0 && !(t-> options & twReturnChunks))
    {
        int a, b, c;
        char * l   = ret[ tildeLine];
        t-> t_char = l[ tildePos+1];
        if ( t-> options & twCollapseTilde)
           memcpy( &l[ tildePos], &l[ tildePos+1], strlen( l) - tildePos);
        l = ret[ t-> t_line];
        apc_gp_move( sys ps, 0, 0);
        if ( !GpiQueryTextBox( sys ps, tildeLPos, l, TXTBOX_COUNT, pt)) apiErr;
        a = pt[ TXTBOX_CONCAT]. x;
        apc_gp_move( sys ps, 0, 0);
        if ( !GpiQueryTextBox( sys ps, tildeLPos + 1, l, TXTBOX_COUNT, pt)) apiErr;
        b = pt[ TXTBOX_CONCAT]. x;
        apc_gp_move( sys ps, 0, 0);
        if ( !GpiQueryTextBox( sys ps, 1, &l[ tildeLPos], TXTBOX_COUNT, pt)) apiErr;
        c = pt[ TXTBOX_CONCAT]. x;
        t-> t_start = b - c + 1;
        t-> t_end   = b + b - a - c - 1;
    } else {
        t-> t_start = t-> t_end = t-> t_line = -1;
    }
    return ret;
}

char **
apc_gp_text_wrap( Handle self, TextWrapRec * t)
{
   switch( t-> options & ( twBegin|twEnd|twProcess))
   {
// starting actions
      case twBegin:
      {
         int i;
         FONTMETRICS fm;
         PLONG ct = malloc( 257 * sizeof( LONG));
         if ( t-> font) sys saveFont = malloc( sizeof( Font));
         gp_enter_font( self, t-> font, sys saveFont);

         sys charTable = malloc( 257 * sizeof( float));
         memset( ct, 0, 256 * sizeof( LONG));
         if ( !GpiQueryFontMetrics ( sys ps, sizeof( fm), &fm)) apiErr;
         if ( fm. fsDefn & FM_DEFN_OUTLINE)
         {
            SIZEF szOld, szNew = {MAKEFIXED(fm. sXDeviceRes,0), MAKEFIXED(fm. sYDeviceRes,0)};
            float fOld,  fNew  = fixed2float( szNew. cx);
            POINTL pt[ TXTBOX_COUNT];

            if ( !GpiQueryCharBox( sys ps, &szOld)) apiErr;
            fOld = fixed2float( szOld. cx);
            if ( !GpiSetCharBox( sys ps, &szNew)) apiErr;
            if ( !GpiQueryWidthTable( sys ps, 0, 256, ct)) apiErr;

            apc_gp_move( sys ps, 0, 0);
            if ( !GpiQueryTextBox( sys ps, 1, "W", TXTBOX_COUNT, pt)) apiErr;
            ct[ 256] = pt[ TXTBOX_BOTTOMLEFT]. x;
            if ( ct[ 256] < 0) ct[ 256] = 0;
            for ( i = 0; i < 257; i++) sys charTable[ i] = ct[ i] * fOld / fNew;
            if ( !GpiSetCharBox( sys ps, &szOld)) apiErr;
         } else {
            if ( !GpiQueryWidthTable( sys ps, 0, 256, ct)) apiErr;
            for ( i = 0; i < 256; i++) sys charTable[ i] = ct[ i];
            sys charTable[ 256] = 0;
         }
         free( ct);
         return nil;
      }
// ending actions
      case twEnd:
      {
         gp_leave_font( self, sys saveFont, sys saveFont);
         free( sys saveFont);
         sys saveFont = nil;
         free( sys charTable);
         sys charTable = nil;
         return nil;
      }
      case twProcess:
         return gp_text_wrap( self, t);
      default:
         return nil;
   }
}

Point
apc_gp_get_resolution( Handle self)
{
   return sys res;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
   return &sys fillPattern;
}

void
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   long *p1 = ( long*) pattern;
   long *p2 = p1 + 1;
   if ( !sys ps)
   {
      memcpy( &sys fillPattern2, pattern, sizeof( FillPattern));
      return;
   }
   memcpy( &sys fillPattern, pattern, sizeof( FillPattern));
   if ( !GpiSetPatternSet( sys ps, 0)) apiErr;
   if (( *p1 == 0) && ( *p2 == 0)) GpiSetPattern( sys ps, PATSYM_BLANK); else
   if (( *p1 == 0xFFFFFFFF) && ( *p2 == 0xFFFFFFFF)) GpiSetPattern( sys ps, PATSYM_SOLID); else
   {
      BInfo2 bm = { sizeof( BInfo), 8, 8, 1, 1, {{ 0, 0, 0, 0}, { 0xFF, 0xFF, 0xFF, 0}}};
      HBITMAP b;
      ULONG core [8];
      int i;
      for ( i = 0; i < 8; i++) core[ i] = pattern[ i];
      if ( sys fillBitmap) {
         GpiDeleteBitmap( sys fillBitmap);
         sys fillBitmap = nilHandle;
      }
      b = GpiCreateBitmap( sys ps, ( PBITMAPINFOHEADER2) &bm, CBM_INIT, (PBYTE) &core, ( PBITMAPINFO2) &bm);
      if ( b != nilHandle) {
         GpiSetBitmapId( sys ps, b, 3);
         GpiSetPatternSet( sys ps, 3);
         sys fillBitmap = b;
      } else apiErr;
   }
}

// GP END
// MENU



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
    md-> menu = menu;
    WinSetWindowULong( m, QWL_USER, ( ULONG) md);
    md-> fnwp = WinSubclassWindow( m, ( PFNWP) generic_menu_handler);
    first = i;

    while ( i != nil)
    {
       menuItem. iPosition = MIT_END;
       menuItem. afStyle = 0;
       menuItem. afStyle |= ( i-> divider    ) ? MIS_SEPARATOR       : 0;
       menuItem. afStyle |= ( i-> down       ) ? MIS_SUBMENU         : 0;
       menuItem. afStyle |= ( i-> bitmap     ) ? MIS_BITMAP          : 0;
       menuItem. afStyle |= ( i-> text       ) ? MIS_TEXT            : 0;
       menuItem. afStyle |= ( i-> rightAdjust) ? MIS_BUTTONSEPARATOR : 0;
       menuItem. afAttribute = 0;
       menuItem. afAttribute |= ( i-> checked)  ? MIA_CHECKED        : 0;
       menuItem. afAttribute |= ( i-> disabled) ? MIA_DISABLED       : 0;
       menuItem. id    = i-> id + MENU_ID_AUTOSTART + 1;
       menuItem. hItem = ( i-> bitmap) ? make_bitmap_handle( i-> bitmap) : 0;
       menuItem. hwndSubMenu = add_item( m, menu, i-> down);
       if (!( i-> divider && i-> rightAdjust))
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
   sys className = WC_MENU;
   sys owner = DHANDLE( owner);
   sys res   = guts. displayResolution;
   return true;
}

void
apc_menu_destroy( Handle self)
{
   if ( var handle && WinIsWindow( guts. anchor, var handle))
      if ( !WinDestroyWindow( var handle)) apiErr;
}

PFont
apc_menu_default_font( PFont font)
{
   char buf  [256];
   PrfQueryProfileString( HINI_USERPROFILE, "PM_SystemFonts", "Menus", DEFAULT_FONT_FACE, buf, 255);
   font_pp2gp( buf, font);
   font_font2gp( font, guts. displayResolution, true);
   return font;
}

PFont
apc_menu_get_font ( Handle self, PFont font)
{
   char buf  [256];
   if ( WinQueryPresParam( var handle, PP_FONTNAMESIZE, 0, nil, 256,
        &buf, QPF_NOINHERIT) && strchr( buf, '.'))
      {
         font_pp2gp( buf, font);
         font_font2gp( font, sys res, true);
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
     p : clLightRed;
}

void
apc_menu_set_color( Handle self, Color color, int index)
{
   if ( index == ciLight3DColor) return;
   if ( index == ciDark3DColor)  return;
   index = ctx_remap_end( index, ctx_ci2PP_MENU, true);
   if (( var handle == nilHandle) || ( index == endCtx)) return;
   color = remap_color( nilHandle, color, true);
   if ( !WinSetPresParam( var handle, index, sizeof( color), &color)) apiErr;
}

void
apc_menu_set_font( Handle self, PFont font)
{
   char buf [256];
   if (( var handle == nilHandle) || ( font == nilHandle)) return;
   font_gp2pp( font, buf);
   if ( !WinSetPresParam( var handle, PP_FONTNAMESIZE, ( ULONG) strlen( buf)+1, ( PVOID) &buf)) apiErr;
}

void
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
   PWindow owner;
   Point size;
   Bool resize;

   if (!var handle) return;
   resize = kind_of( var owner, CWindow) &&
        var stage <= csNormal &&
        ((( PAbstractMenu) self)-> self)-> get_selected( self);
   if ( resize)
   {
      owner = ( PWindow) var owner;
      size = owner-> self-> get_size( var owner);
   }
   WinSendMsg( var handle, MM_DELETEITEM, MPFROM2SHORT( m->id, true), 0);
   if ( resize) owner-> self-> set_size( var owner, size.x, size. y);
}

void
apc_menu_item_set_check ( Handle self, PMenuItemReg m, Bool check)
{
   if (!var handle) return;
   WinSendMsg( var handle,
      MM_SETITEMATTR, MPFROM2SHORT( m->id, true),
      MPFROM2SHORT( MIA_CHECKED, check ? MIA_CHECKED : false));
}

Bool
apc_menu_item_get_check ( Handle self, int sysId)
{
   if (!var handle) return false;
   return ( LONGFROMMP( WinSendMsg(
      var handle, MM_QUERYITEMATTR,
      MPFROM2SHORT( sysId, true), MPFROMLONG( MIA_CHECKED)))
      ) != 0;
}

Bool
apc_menu_item_get_enabled ( Handle self, int sysId)
{
if (!var handle) return false;
   return  LONGFROMMP( WinSendMsg(
      var handle, MM_QUERYITEMATTR,
      MPFROM2SHORT( sysId, true), MPFROMLONG( MIA_DISABLED))) == 0;
}

char *
apc_menu_item_get_text ( Handle self, int sysId, char * buf)
{
   if ( var handle)
     WinSendMsg( var handle, MM_QUERYITEMTEXT, MPFROM2SHORT( sysId, 256), ( MPARAM) buf);
   else buf[0] = 0;
   return buf;
}

void
apc_menu_item_set_enabled( Handle self, PMenuItemReg m, Bool enabled)
{
   if (!var handle) return;
   WinSendMsg( var handle,
      MM_SETITEMATTR, MPFROM2SHORT( m->id, true),
      MPFROM2SHORT( MIA_DISABLED, enabled ? false : MIA_DISABLED));
}

void
apc_menu_item_set_text( Handle self, PMenuItemReg m, char * text)
{
   char buf [ 1024];
   char * t = m-> accel ? buf : text;
   if (!var handle) return;
   if ( m-> accel) snprintf( buf, 1024, "%s\t%s", text, m-> accel);
   WinSendMsg( var handle, MM_SETITEMTEXT, MPFROM2SHORT( m->id, true), ( MPARAM) t);
}

void
apc_menu_item_set_accel( Handle self, PMenuItemReg m, char * accel)
{
   char buf [ 1024];
   char * t = accel ? buf : m->text;
   if (!var handle) return;
   if ( accel) snprintf( buf, 1024, "%s\t%s", m->text, accel);
   WinSendMsg( var handle, MM_SETITEMTEXT, MPFROM2SHORT( m->id, true), ( MPARAM) t);
}

void apc_menu_item_set_key( Handle self, PMenuItemReg m, int key) {}

Bool
apc_popup_create ( Handle self, Handle owner)
{
   sys owner = DHANDLE( owner);
   var handle  = add_item( HWND_OBJECT, self, (( PMenu) self)-> tree);
   sys className = WC_MENU;
   sys res   = guts. displayResolution;
   return var handle != nilHandle;
}

PFont
apc_popup_default_font( PFont font)
{
   return apc_menu_default_font( font);
}

Bool
apc_popup( Handle self, int x, int y)
{
   HWND h  = (( PWidget) var owner)-> handle;
   Bool rc = WinPopupMenu( h, h, var handle, x, y, 0,
      PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_MOUSEBUTTON3);
   if ( !rc) apiErr;
   return rc;
}


// MENU END
// TIMER

static int
add_timer( Handle timerObject, Handle self)
{
   PItemRegRec pTime;
   int i;
   if ( timerObject == nilHandle) {
      apcErr( errInvObject);
      return -1;
   }

   if ( sys timeDefs) for ( i = 0; i < sys timeDefsCount; i++)
     if ( sys timeDefs[ i]. item == nil)
     {
        sys timeDefs[ i]. item = ( void*) timerObject;
        return i + 1;
     }

   pTime = malloc (( sys timeDefsCount + 1) * sizeof( ItemRegRec));
   if ( sys timeDefs) {
      memcpy( pTime, sys timeDefs, sys timeDefsCount* sizeof( ItemRegRec));
      free( sys timeDefs);
   }
   sys timeDefs = pTime;
   pTime += sys timeDefsCount++;
   pTime-> item = ( void*) timerObject;
   return sys timeDefsCount;
}

static void
remove_timer( Handle timerObject, Handle self)
{
   int i;
   PItemRegRec list = sys timeDefs;
   for ( i = 0; i < sys timeDefsCount; i++)
   {
      if (( Handle)( list-> item) == timerObject)
      {
          list-> item = nilHandle;
          break;
      }
      list++;
   }
}

Bool
apc_timer_create ( Handle self, Handle owner, int timeout)
{
   if (( DHANDLE( owner) != sys owner) && ( var handle != nilHandle))
   {
      if ( !WinStopTimer( guts. anchor, (( PWidget) owner)-> handle, var handle)) apiErr;
      remove_timer( self, var owner);
   }
   sys owner = DHANDLE( owner);
   if ( !( var handle = add_timer( self, owner))) return false;
   sys s. timer. timeout = timeout;
   sys s. timer. active = 0;
   return true;
}

void
apc_timer_destroy ( Handle self)
{
   if ( var handle && WinIsWindow( guts. anchor, (( PWidget) var owner)-> handle))
   {
      if ( !WinStopTimer( guts. anchor, (( PWidget) var owner)-> handle, var handle)) apiErr;
      remove_timer( self, var owner);
   }
}

int
apc_timer_get_timeout( Handle self)
{
   return sys s. timer. timeout;
}

void
apc_timer_set_timeout( Handle self, int timeout)
{
   if ( sys s. timer. active)
      if ( !WinStartTimer ( guts. anchor, (( PWidget) var owner)-> handle, var handle, timeout)) apiErr;
   sys s. timer. timeout = timeout;
}

Bool
apc_timer_start( Handle self)
{
   if ( WinStartTimer ( guts. anchor, (( PWidget) var owner)-> handle, var handle, sys s. timer. timeout)) {
      sys s. timer. active = true;
      return true;
   }
   apiErr;
   return false;
}

void
apc_timer_stop( Handle self)
{
   WinStopTimer( guts. anchor, (( PWidget) var owner)-> handle, var handle);
   sys s. timer. active = false;
}


// TIMER END
// MESSAGE
typedef MRESULT (*ApiMessageSender)(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void
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
             if ( ev-> pos. mod & kbShift) mod |= KC_SHIFT;
             if ( ev-> pos. mod & kbAlt  ) mod |= KC_ALT;
             if ( ev-> pos. mod & kbCtrl ) mod |= KC_CTRL;
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
             if ( ev-> key. mod & kbShift) m1s1         |= KC_SHIFT;
             if ( ev-> key. mod & kbAlt  ) m1s1         |= KC_ALT;
             if ( ev-> key. mod & kbCtrl ) m1s1         |= KC_CTRL;
             if ( ev-> key. code) m1s1                  |= KC_CHAR;
             if (( ev-> key. key != kbNoKey) || ev-> key. mod)  m1s1 |= KC_VIRTUALKEY;
             if ( ev-> key. mod & ~kbShift && ev-> key. code)  m1s1 &= ~KC_CHAR;
             m1c1 = ev-> key. repeat;
             sender( HANDLE, WM_CHAR, MPFROMSH2CH( m1s1, m1c1, m1c2), mp2);
          }
          break;
   }
}

void
apc_show_message( char * message)
{
   char title [256] = {0};
   if ( guts. anchor && guts. queue) {
      if ( WinQueryTaskTitle( 0, title, 256) != 0) apiErr;
      if ( WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, message, title, 0, MB_OK | MB_MOVEABLE) == MBID_ERROR) apiErr;
   } else {
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
      VioWrtCharStrAtt( "PRIMA", 5, 0, 37, &attr, 0);
      for ( i = 0; i < 4; i++) { DosBeep( 4500, 40); DosSleep( 80); }
      attr = 0x20;
      VioWrtNAttr( &attr, 80, 0, 0, 0);
      KbdCharIn( &key, IO_WAIT, 0);
      VioEndPopUp(0);
   }
}

// MESSAGE END

int ctx_kb2VK[] =
{
   kbNoKey       ,   0                 ,
   kbBackspace   ,   VK_BACKSPACE      ,
   kbTab         ,   VK_TAB            ,
   kbShiftTab    ,   VK_BACKTAB        ,
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
   endCtx
};

// CURSOR

void
cursor_update( Handle self)
{
   WinDestroyCursor( var handle);
   if ( is_apt( aptCursorVis) && is_apt( aptFocused))
   {
      if ( !WinCreateCursor( var handle,
         sys cursorPos. x, sys cursorPos. y,
         sys cursorSize. x, sys cursorSize. y,
         CURSOR_SOLID | CURSOR_FLASH, nil)) apiErr;
      if ( !WinShowCursor( var handle, is_apt( aptCursorVis))) apiErr;
   }
}

void
apc_cursor_set_pos( Handle self, int x, int y)
{
   sys cursorPos = ( Point){ x, y};
   cursor_update( self);
}

void
apc_cursor_set_size( Handle self, int x, int y)
{
   sys cursorSize = ( Point){ x, y};
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

// CURSOR END

// POINTER
Point
apc_pointer_get_pos( Handle self)
{
   POINTL p;
   WinQueryPointerPos( HWND_DESKTOP, &p);
   return ( Point){ p.x, p.y};
}

void
apc_pointer_set_pos( Handle self, int x, int y)
{
   WinSetPointerPos( HWND_DESKTOP, x, y);
}

Point
apc_pointer_get_hot_spot( Handle self)
{
   POINTERINFO p;
   if ( !WinQueryPointerInfo( sys pointer, &p)) apiErr;
   return ( Point){ p. xHotspot, p. yHotspot};
}

Point
apc_pointer_get_size( Handle self)
{
   return ( Point) {
      WinQuerySysValue( HWND_DESKTOP, SV_CXPOINTER),
      WinQuerySysValue( HWND_DESKTOP, SV_CYPOINTER)
   };
}

int
apc_pointer_get_shape( Handle self)
{
   return sys pointerId;
}

Bool
apc_pointer_get_visible( Handle self)
{
   return is_apt( aptPointerVis);
}

int ctx_cr2SPTR[] =
{
   crArrow,    SPTR_ARROW,
   crText,     SPTR_TEXT,
   crWait,     SPTR_WAIT,
   crSize,     SPTR_MOVE,
   crMove,     SPTR_MOVE,
   crSizeWE,   SPTR_SIZEWE,
   crSizeNS,   SPTR_SIZENS,
   crSizeNWSE, SPTR_SIZENWSE,
   crSizeNESW, SPTR_SIZENESW,
   crInvalid,  SPTR_ILLEGAL,
   endCtx
};


void
apc_pointer_set_shape( Handle self, int sysPtrId)
{
   HPOINTER user = sys pointer2;
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
      WinQuerySysPointer( HWND_DESKTOP,
      ctx_remap_def( sysPtrId, ctx_cr2SPTR, true, SPTR_ARROW),
      false);
   if ( var stage == csNormal) if ( !WinSetPointer( HWND_DESKTOP, sys pointer)) apiErr;
}

void
apc_pointer_set_visible( Handle self, Bool visible)
{
   if ( var stage == csNormal)
   {
      apt_assign( aptPointerVis, visible);
      if (!WinShowPointer( HWND_DESKTOP, visible)) apiErr;
      guts. pointerLock += visible ? 1 : -1;
   }
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
   int j;
   BInfo2 bi;
   HBITMAP bDup = nilHandle, bSave;
   POINTERINFO p;
   BITMAPINFOHEADER bh;
   PIcon i = ( PIcon) icon;
   POINTL pt[4] = {{0,0},{0,0},{0,0},{0,0}};
   Bool ret = true;

   if (( icon == nilHandle) || (!screenable( icon))) apcErrRet( errInvParams);
   if ( WinQueryPointerInfo( sys pointer, &p) == 0) apiErrRet;
   self = icon;
   memset( &bi, 0, sizeof( bi));
   bi. structLength = sizeof( BInfo);
   bi. planes = 1;
   bi. bpp    = 1;

   // querying bitmap characterictics
   if ( p. hbmColor)
   {
      // pointer have a color bitmap
      if ( !GpiQueryBitmapParameters( p. hbmColor, &bh)) apiErrRet;
      // creating storage
      if (bh. cBitCount > 8) bh. cBitCount = 24;
      image_begin_query( bh. cBitCount, ( int *)&bh. cBitCount);
      i-> self-> create_empty( icon, bh. cx, bh. cy, bh. cBitCount);
      // reading color map
      apcErrClear;
      bSave = GpiSetBitmap( guts. ps, p. hbmColor);
      if ( bSave == HBM_ERROR) apiErrRet;
      image_query( icon, guts. ps);
      if ( apcError) return false;
      // creating color duplicate
      bDup = make_bitmap_handle( icon);
      if ( apcError) return false;
      // query mono XOR map to xor the color duplicate
      if ( GpiSetBitmap( guts. ps, p. hbmPointer) == HBM_ERROR) apiErr;
      if ( GpiQueryBitmapBits( guts. ps, 0, i-> h, (PBYTE)i-> mask, ( PBITMAPINFO2)&bi) < 0) {
         apiErr;
         GpiSetBitmap( guts. ps, bSave);
         GpiDeleteBitmap( bDup);
         return false;
      }
      if ( GpiSetBitmap( guts. ps, bDup) == HBM_ERROR) apiErr;
      pt [1]. x = i-> w - 1;     // dest to
      pt [1]. y = i-> h - 1;
      pt [3]. x = i-> w;         // src to
      pt [3]. y = i-> h;
      GpiSetColor( guts. ps, CLR_WHITE);
      GpiSetBackColor( guts. ps, CLR_BLACK);
      if ( GpiDrawBits( guts. ps, i-> mask, ( PBITMAPINFO2) &bi, 4, pt, ROP_SRCINVERT, BBO_IGNORE) == GPI_ERROR) {
         apiErr;
         ret = false;
      }
      image_query( icon, guts. ps);
      if ( apcError) ret = false;
   } else {
      // pointer does not have color bitmap
      if ( !GpiQueryBitmapParameters( p. hbmPointer, &bh)) apiErrRet;
      if (bh. cBitCount > 8) bh. cBitCount = 24;
      image_begin_query( bh. cBitCount, ( int *)&bh. cBitCount);
      i-> self-> create_empty( icon, bh. cx, bh. cy / 2, bh. cBitCount);
      bSave = GpiSetBitmap( guts. ps, p. hbmPointer);
      if ( bSave == HBM_ERROR) apiErr;
      if ( GpiQueryBitmapBits( guts. ps, 0, i-> h, (PBYTE)i-> data, ( PBITMAPINFO2)&bi) < 0)
      {
         apiErr;
         GpiSetBitmap( guts. ps, p. hbmPointer);
         return false;
      }
   }

   // query mono AND map
   if ( GpiSetBitmap( guts. ps, p. hbmPointer) == HBM_ERROR) apiErr;
   if ( bDup) if ( !GpiDeleteBitmap( bDup)) apiErr;
   if ( GpiQueryBitmapBits( guts. ps, i-> h, i-> h, (PBYTE)i-> mask, ( PBITMAPINFO2)&bi) < 0) ret = false;
   for ( j = 0; j < i-> maskSize; j++) i-> mask[ j] = ~i-> mask[ j];
   // end up
   if ( GpiSetBitmap( guts. ps, bSave) == HBM_ERROR) apiErrRet;
   return ret;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
   if ( sys pointer2) if ( !WinDestroyPointer( sys pointer2)) apiErr;
   apcErrClear;
   sys pointer2 = icon ? make_pointer_handle( self, icon, hotSpot) : nilHandle;
   if ( apcError) return false;
   if ( sys pointerId == crUser)
   {
      sys pointer = sys pointer2;
      if ( var stage == csNormal)
        if ( !WinSetPointer( HWND_DESKTOP, sys pointer)) apiErrRet;
   }
   return true;
}

int
apc_pointer_get_state( Handle self)
{
   return
   (( WinGetKeyState( HWND_DESKTOP, VK_BUTTON1) & 0x8000) ? mbLeft   : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_BUTTON2) & 0x8000) ? mbRight  : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_BUTTON3) & 0x8000) ? mbMiddle : 0);
}

int
apc_kbd_get_state( Handle self)
{
   return
   (( WinGetKeyState( HWND_DESKTOP, VK_ALT    ) & 0x8000) ? kbAlt    : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_CTRL   ) & 0x8000) ? kbCtrl   : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_SHIFT  ) & 0x8000) ? kbShift  : 0);
}

// POINTER END

// SYS
static int ctx_mb2WA[] =
{
   mbError       , WA_ERROR,
   mbQuestion    , WA_NOTE,
   mbInformation , WA_NOTE,
   mbWarning     , WA_WARNING,
   endCtx
};

Point
apc_sys_get_autoscroll_rate( void)
{
   return ( Point){
      WinQuerySysValue( HWND_DESKTOP, SV_FIRSTSCROLLRATE),
      WinQuerySysValue( HWND_DESKTOP, SV_SCROLLRATE)
   };
}

int
apc_sys_get_cursor_width( void)
{
  return WinQuerySysValue( HWND_DESKTOP, SV_CXBORDER);
}

Bool
apc_sys_get_insert_mode( void)
{
   return WinQuerySysValue( HWND_DESKTOP, SV_INSERTMODE);
}

void
apc_sys_set_insert_mode( Bool insMode)
{
   WinSetSysValue( HWND_DESKTOP, SV_INSERTMODE, insMode);
}

Point
apc_sys_get_scrollbar_metrics( void)
{
  return ( Point) {
    WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL),
    WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL)
  };
}

Point
apc_sys_get_window_borders( int borderStyle)
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


void
apc_beep( int style)
{
   WinAlarm( HWND_DESKTOP, ctx_remap_def( style, ctx_mb2WA, true, WA_WARNING));
}

void
apc_beep_tone( int freq, int duration)
{
   DosBeep( freq, duration);
}

char *
apc_system_action( char * params)
{
   char *ret = nil;

   if ( strnicmp( params, "logger.", 7) == 0)
   {
      if ( stricmp( params + 7, "pos.get") == 0)
      {
         SWP winPos;
         HWND handle;

         ret = malloc( 2048);
         handle = WinQueryWindow( guts. logger, QW_PARENT);
         WinQueryWindowPos( handle, &winPos);

         if ( winPos.fl & SWP_MINIMIZE)
            sprintf( ret, "MIN %d %d %d %d",
                     WinQueryWindowUShort( handle, QWS_XRESTORE),
                     WinQueryWindowUShort( handle, QWS_YRESTORE),
                     WinQueryWindowUShort( handle, QWS_CXRESTORE),
                     WinQueryWindowUShort( handle, QWS_CYRESTORE));
         else if ( winPos.fl & SWP_MAXIMIZE)
            sprintf( ret, "MAX %d %d %d %d",
                     WinQueryWindowUShort( handle, QWS_XRESTORE),
                     WinQueryWindowUShort( handle, QWS_YRESTORE),
                     WinQueryWindowUShort( handle, QWS_CXRESTORE),
                     WinQueryWindowUShort( handle, QWS_CYRESTORE));
         else if ( !WinIsWindowVisible( handle))
            sprintf( ret, "HIDDEN %ld %ld %ld %ld", winPos. x, winPos. y, winPos. cx, winPos. cy);
         else
            sprintf( ret, "NORM %ld %ld %ld %ld", winPos. x, winPos. y, winPos. cx, winPos. cy);
      } else if ( strnicmp( params + 7, "pos.set", 7) == 0) {
         int i;
         long x, y, cx, cy;
         char cmd[ 256];
         HWND win = WinQueryWindow( guts. logger, QW_PARENT);

         if ( strlen( params) > 270) return ret;   // do nothing
         i = sscanf( params + 14, "%s %ld %ld %ld %ld", cmd, &x, &y, &cx, &cy);
         if ( i != 5) return ret;
         if ( stricmp( cmd, "NORM") == 0)
            WinSetWindowPos( win, HWND_TOP, x, y, cx, cy, SWP_SIZE|SWP_MOVE|SWP_SHOW);
         else if ( stricmp( cmd, "HIDDEN") == 0)
            WinSetWindowPos( win, HWND_TOP, x, y, cx, cy, SWP_SIZE|SWP_MOVE|SWP_HIDE);
         else if ( stricmp( cmd, "MIN") == 0) {
            WinSetWindowPos( win, HWND_TOP, 0, 0, 0, 0, SWP_MINIMIZE|SWP_SHOW);
            WinSetWindowUShort( win, QWS_CXRESTORE, cx);
            WinSetWindowUShort( win, QWS_XRESTORE, x);
            WinSetWindowUShort( win, QWS_CYRESTORE, cy);
            WinSetWindowUShort( win, QWS_YRESTORE, y);
         } else if ( stricmp( cmd, "MAX") == 0) {
            WinSetWindowPos( win, HWND_TOP, 0, 0, 0, 0, SWP_MAXIMIZE|SWP_SHOW);
            WinSetWindowUShort( win, QWS_CXRESTORE, cx);
            WinSetWindowUShort( win, QWS_XRESTORE, x);
            WinSetWindowUShort( win, QWS_CYRESTORE, cy);
            WinSetWindowUShort( win, QWS_YRESTORE, y);
         }
      } else if ( strnicmp( params + 7, "clean", 5) == 0) {
         WinSendMsg( guts. loggerListBox, LM_DELETEALL, 0, 0);
      }
   }
   else if ( stricmp( params, "wait.before.quit") == 0) { waitBeforeQuit = 1; }
   else if ( stricmp( params, "sigsegv") == 0)
   {
      char *t = 0;
      int   i = t[100];
      (void)i;
   }
   return ret;
}

void
apc_query_drives_map( char *firstDrive, char *map)
{
   char *m = map;
   int beg;
   ULONG curDrive, driveMap;
   int i;

   strcpy( map, "");

   beg = toupper( *firstDrive);
   if (( beg < 'A') || ( beg > 'Z') || ( firstDrive[1] != ':'))
      return;

   beg -= 'A';

   DosQueryCurrentDisk( &curDrive, &driveMap);
   for ( i = beg; i < 26; i++)
   {
      if ((driveMap << ( 31 - i)) >> 31)
      {
         *m++ = i + 'A';
         *m++ = ':';
         *m++ = ' ';
      }
   }

   *m = '\0';
   return;
}

Bool is_cd( int num) {
   ULONG actionStatus;
   HFILE cdDriver;
   APIRET rc;
   struct {
      short n;
      short i;
   } data;
   ULONG dataLen;

   rc = DosOpen( "\\DEV\\CD-ROM2$", &cdDriver, &actionStatus, 0, FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE, 0);
   if (rc) return false;

   dataLen = 4;
   rc = DosDevIOCtl( cdDriver, 0x82, 0x60, 0, 0, 0, &data, 4, &dataLen);
   DosClose( cdDriver);
   if (rc) return false;

   return ( data. i <= num && num <= data. i + data. n - 1);
}

int apc_query_drive_type( char *drive)
{
#define BADRC(func) if ((func)!=NO_ERROR) return dtNone;
   char root[ 16];
   int num;
   Bool removable;

   if (( toupper( *drive) < 'A') || ( toupper( *drive) > 'Z') || ( drive[1] != ':'))
      return -1;
   root[0] = toupper( drive[0]);
   root[1] = drive[1];
   root[2] = '\\';
   root[3] = '\0';
   num = root[0] - 'A';

   // Check whether this drive exists
   {
      ULONG curDrive, driveMap;
      BADRC(DosQueryCurrentDisk( &curDrive, &driveMap));
      if (!((driveMap << ( 31 - num)) >> 31)) return dtNone;
   }

   // Check whether it is a CD-ROM
   if ( is_cd( num)) return dtCDROM;

   // Classify by types and ``removableness''
   {
      BIOSPARAMETERBLOCK bpb;
      struct
      {
         char cmd;
         char unit;
      } params = { 0, num};
      ULONG parLen = sizeof( params);
      ULONG bpbLen = sizeof( bpb);

      memset( &bpb, 0, sizeof( bpb));
      bpb. bDeviceType = 5;
      params. cmd = 0; params. unit = num;
      BADRC( DosDevIOCtl( -1, IOCTL_DISK, DSK_GETDEVICEPARAMS,
                          &params, sizeof( params), &parLen,
                          &bpb, sizeof( bpb), &bpbLen));
      removable = !(bpb. fsDeviceAttr & 1);
      switch ( bpb. bDeviceType) {
         case 0:  // various stupid floppies
         case 1:
         case 2:
         case 3:
         case 4:
         case 9:
            if (removable) return dtFloppy;
            break;
         case 5: {
            char buf[ sizeof( FSQBUFFER2) + (3*CCHMAXPATH)] = {0};
            ULONG cbBuf = sizeof( buf);
            PFSQBUFFER2 fs = (void*)buf;
            char *fsname;
            root[2] = '\0';
            if ( DosQueryFSAttach( root, 0, FSAIL_QUERYNAME, (void *)buf, &cbBuf) != NO_ERROR)
               return dtUnknown;
            if (fs-> iType != FSAT_REMOTEDRV)
               return dtHDD;
            fsname = fs->szName + fs->cbName + 1;
            if ( strcmp( fsname, "RAMFS") == 0)
               return dtMemory;
            return dtNetwork;
            } break;
         case 7:  // ``other'' :-)
            if (removable && ( bpb. usBytesPerSector * bpb. cSectors == 512*1440 ||
                               bpb. usBytesPerSector * bpb. cSectors == 512*2880 ||
                               bpb. usBytesPerSector * bpb. cSectors == 512*5760))
               return dtFloppy;
            break;
      }
   }

   return dtUnknown;
#undef BADRC
}

static const char * NETBIOS_username( void)
{
    char *username;
    char errmod[256];
    static unsigned short ( *Net32WkstaGetInfo) ( const unsigned char * pszServer,
                                           unsigned long         sLevel,
                                           unsigned char       * pbBuffer,
                                           unsigned long         cbBuffer,
                                           unsigned long       * pcbTotalAvail);
    APIRET rc;
    ULONG avail;
    HMODULE hNETAPI;
    static Bool badModule;
    static char *buf = nil;

    if (!Net32WkstaGetInfo && !badModule) {
        rc=DosLoadModule( errmod, 256, "NETAPI32", &hNETAPI);
        if (rc!=0) {
            badModule = true;
            return NULL;
        }
        rc=DosQueryProcAddr( hNETAPI, 0, "Net32WkstaGetInfo", ( PFN*) &Net32WkstaGetInfo);
        if (rc!=0) {
            badModule = true;
            DosFreeModule(hNETAPI);
            return NULL;
        }
    }
    free( buf); buf = nil;
    rc = Net32WkstaGetInfo(NULL, 1, NULL, 0, &avail);
    buf = ( char*) malloc( avail);
    rc = Net32WkstaGetInfo( NULL, 1, buf, avail, &avail);
    username = *( char**) ( buf+14);
    return ( rc==0 && username[ 0] ? username : NULL);
}

static const char * UNIXstyle_username( void)
{
    const char *username = getenv( "USER");
    if ( !username) {
        username = getenv( "LOGNAME");
    }
    return username;
}

char * apc_get_user_name( void)
{
    const char *username=NETBIOS_username();
    if (!username) {
        username=UNIXstyle_username();
    }
    return ( username ? (char *)username : "");
}
// SYSTEM END

// CLIPBOARD
Bool apc_clipboard_create( void) { return true; }
void apc_clipboard_destroy( void) {}

Bool
apc_clipboard_open( void)
{
   Bool ok;
   Bool oad = appDead;
   appDead = true;
   ok = WinOpenClipbrd( guts. anchor);
   appDead = oad;
   if ( !ok) apiErr;
   return ok;
}

void
apc_clipboard_close( void)
{
   if ( !WinCloseClipbrd( guts. anchor)) apiErr;
}

void
apc_clipboard_clear( void)
{
   if ( !WinEmptyClipbrd( guts. anchor)) apiErr;
}


static long cf2CF( long id)
{
   if ( id == cfText)   return CF_TEXT;
   if ( id == cfBitmap) return CF_BITMAP;
   return id - cfCustom;
}

Bool
apc_clipboard_has_format( long id)
{
   ULONG flags;
   return WinQueryClipbrdFmtInfo( guts. anchor, cf2CF( id), &flags);
}

void *
apc_clipboard_get_data( long id, int * length)
{
   id = cf2CF( id);
   switch( id)
   {
      case CF_BITMAP:
         {
             PImage image      = ( PImage) *length;
             Handle self       = ( Handle) image;
             HBITMAP b = WinQueryClipbrdData( guts. anchor, id);
             HBITMAP b2, bsave;
             BITMAPINFOHEADER bh;
             PBITMAPINFO2 bi;
             POINTL pt [4] = {{0,0},{0,0},{0,0},{0,0}};
             if ( b == nilHandle) {
                apcErr( errInvClipboardData);
                return nil;
             }
             if ( !GpiQueryBitmapParameters( b, &bh)) apiErr;
             if (bh. cBitCount > 8) bh. cBitCount = 24;
             image_begin_query( bh. cBitCount, ( int *)&bh. cBitCount);
             image-> self-> create_empty( self, bh. cx, bh. cy, bh. cBitCount);
             pt[ 1]. x = pt[ 3]. x = bh. cx;
             pt[ 1]. y = pt[ 3]. y = bh. cy;
             pt[ 1]. x--;
             pt[ 1]. y--;
             bi = get_binfo( self);
             b2 = GpiCreateBitmap( guts. ps, ( PBITMAPINFOHEADER2)bi, 0, nil, nil);
             if ( b2 == nilHandle) { apiErr; return nil; }
             free( bi);
             bsave = GpiSetBitmap( guts. ps, b2);
             if ( bsave == HBM_ERROR) apiErr;
             if ( GpiWCBitBlt( guts. ps, b, 4, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) apiErr;
             image_query( self, guts. ps);
             if ( GpiSetBitmap( guts. ps, bsave) == HBM_ERROR) apiErr;
             if ( !GpiDeleteBitmap( b2)) apiErr;
             return (void*)*length;
         }
         break;
      case CF_TEXT:
         {
             char * ptr = (char*) WinQueryClipbrdData( guts. anchor, id);
             char * ret;
             int i, len = *length = strlen( ptr) + 1;
             if ( ptr == nil) {
                apcErr( errInvClipboardData);
                return nil;
             }
             ret = malloc( *length);
             strcpy( ret, ptr);
             for ( i = 0; i < len - 1; i++)
                if ( ret[ i] == '\r') {
                   memcpy( ret + i, ret + i + 1, len - i + 1);
                   len--;
                }
             return ret;
         }
         break;
      default:
         {
            char * ptr = (char*) WinQueryClipbrdData( guts. anchor, id);
            void * ret;
            if ( ptr == nil) {
               apcErr( errInvClipboardData);
               return nil;
            }
            *length = *(( int*) ptr);
            ptr += sizeof( int);
            ret = malloc( *length);
            memcpy( ret, ptr, *length);
            return ret;
         }
   }
   return nil;
}

Bool
apc_clipboard_set_data( long id, void * data, int length)
{
   id = cf2CF( id);
   if ( data == nil)
   {
      if ( !WinEmptyClipbrd( guts. anchor)) apiErr;
      return true;
   }
   switch ( id)
   {
      case CF_BITMAP:
         {
            HBITMAP b = ( HBITMAP) make_bitmap_handle(( Handle) data );
            if ( b == nilHandle) apiErrRet;
            if ( !WinSetClipbrdData( guts. anchor, b, id, CFI_HANDLE)) apiErrRet;
         }
         break;
      case CF_TEXT:
         {
             void * ptr;
             rc = DosAllocSharedMem( &ptr, nil, length, PAG_WRITE|PAG_COMMIT|OBJ_GIVEABLE);
             if ( rc != 0) { apiAltErr( rc); return false; };
             memcpy( ptr, data, length);
             if ( !WinSetClipbrdData( guts. anchor, (ULONG)ptr, id, CFI_POINTER)) apiErrRet;
         }
         break;
      default:
         {
             char * ptr;
             rc = DosAllocSharedMem(( PPVOID) &ptr, nil, length+sizeof(int), PAG_WRITE|PAG_COMMIT|OBJ_GIVEABLE);
             if ( rc != 0) { apiAltErr( rc); return false; };
             memcpy( ptr+sizeof(int), data, length);
             memcpy( ptr, &length, sizeof(int));
             if ( !WinSetClipbrdData( guts. anchor, (ULONG)ptr, id, CFI_POINTER)) apiErrRet;
         }
    }
    return false;
}

long
apc_clipboard_register_format( char * format)
{
   ATOM atom;
   char buf[ 1024];
   snprintf( buf, 1024, "Apc private %s", format);
   atom = WinAddAtom( WinQuerySystemAtomTable(), buf) + cfCustom;
   if ( atom == 0) apiErr;
   return atom;
}

void
apc_clipboard_deregister_format( long id)
{
   WinDeleteAtom( WinQuerySystemAtomTable(), (ATOM)( id - cfCustom));
}

// CLIPBOARD END
// PRINTER

static void ppi_create( PRQINFO3 * dest, PRQINFO3 * source)
{
#define SZCPY(field) if ( source-> field) strcpy( dest-> field = malloc( strlen( source-> field) + 1), source-> field)
   memcpy( dest, source, sizeof( PRQINFO3));
   SZCPY( pszName);
   SZCPY( pszSepFile);
   SZCPY( pszPrProc);
   SZCPY( pszParms);
   SZCPY( pszComment);
   SZCPY( pszPrinters);
   SZCPY( pszDriverName);
   if ( source-> pDriverData)
   {
      dest-> pDriverData = malloc( source-> pDriverData-> cb);
      memcpy( dest-> pDriverData, source-> pDriverData, source-> pDriverData-> cb);
   }
}


static void ppi_destroy( PRQINFO3 * ppi)
{
   if ( !ppi) return;
   free( ppi-> pszName);
   free( ppi-> pszSepFile);
   free( ppi-> pszPrProc);
   free( ppi-> pszParms);
   free( ppi-> pszComment);
   free( ppi-> pszPrinters);
   free( ppi-> pszDriverName);
   free( ppi-> pDriverData);
   memset( ppi, 0, sizeof( PRQINFO3));
}


static int
prn_query( char * printer, PRQINFO3 * info)
{
   ULONG returned, total, needed;
   SPLERR sprc = SplEnumQueue( nil, 3, nil, 0, &returned, &total, &needed, nil);
   PRQINFO3 *ppi, *useThis = nil;
   int i;
   Bool useDefault = ( printer == nil || strlen( printer) == 0);

   if (( sprc != ERROR_MORE_DATA) && ( sprc != NO_ERROR)) {
      apiAltErr( sprc);
      return -1;
   }
   if ( total == 0) return 0;
   ppi  = malloc( needed);
   sprc = SplEnumQueue( nil, 3, ppi, needed, &returned, &total, &needed, nil);
   if ( sprc != 0) {
      apiAltErr( sprc);
      free( ppi);
      return -1;
   }
   for ( i = 0; i < returned; i++)
   {
      if ( useDefault && ( ppi[ i]. fsType & PRQ3_TYPE_APPDEFAULT))
      {
         useThis = &ppi[ i];
         break;
      }
      if ( !useDefault && ( strcmp( printer, ppi[ i]. pszComment) == 0))
      {
         useThis = &ppi[ i];
         break;
      }
   }
   if ( useDefault && useThis == nil) useThis = ppi;
   if ( useThis) ppi_create( info, useThis);
   if ( !useThis) apcErr( errInvPrinter);
   free( ppi);
   return useThis ? 1 : -2;
}

PrinterInfo*
apc_prn_enumerate( Handle self, int * count)
{
   ULONG returned, total, needed;
   SPLERR sprc = SplEnumQueue( nil, 3, nil, 0, &returned, &total, &needed, nil);
   PRQINFO3 * ppi;
   PPrinterInfo list;
   int i;

   *count = 0;

   if (( sprc != ERROR_MORE_DATA) && ( sprc != NO_ERROR)) {
      apiAltErr( sprc);
      return nil;
   }
   if ( total == 0) {
      apcErr( errNoPrinters);
      return nil;
   }
   ppi = malloc( needed);
   sprc = SplEnumQueue( nil, 3, ppi, needed, &returned, &total, &needed, nil);
   if ( sprc != 0) {
      apiAltErr( sprc);
      free( ppi);
      return nil;
   }

   list = malloc( returned * sizeof( PrinterInfo));
   for ( i = 0; i < returned; i++)
   {
      strncpy( list[ i]. name, ppi[ i]. pszComment, 255);       list[ i]. name[ 255]   = 0;
      strncpy( list[ i]. device, ppi[ i]. pszDriverName, 255);  list[ i]. device[ 255] = 0;
      list[ i]. defaultPrinter = ppi[ i]. fsType & PRQ3_TYPE_APPDEFAULT;
   }
   *count = returned;
   free( ppi);
   return list;
}

static HDC prn_info_dc( Handle self)
{
   char * c;
   DEVOPENSTRUC dev;
   char buf[ 1024];
   HDC ret;

   memset( &dev, 0, sizeof( dev));
   dev. pszLogAddress = sys s. prn. ppi. pszName;
   strcpy( buf, sys s. prn. ppi. pszDriverName);
   c = strchr( buf, '.');
   if ( c) *c = '\0';
   dev. pszDriverName = buf;
   dev. pdriv         = sys s. prn. ppi. pDriverData;
   dev. pszDataType   = nil;
   dev. pszComment    = sys s. prn. ppi. pszComment;
   if ( strlen( sys s. prn. ppi. pszPrProc)) dev. pszQueueProcName   = sys s. prn. ppi. pszPrProc;
   if ( strlen( sys s. prn. ppi. pszParms))  dev. pszQueueProcParams = sys s. prn. ppi. pszParms;
   dev .pszSpoolerParams = nil;
   dev .pszNetworkParams = nil;
   ret = DevOpenDC( guts. anchor, OD_INFO, "*", 3, (PDEVOPENDATA)&dev, nilHandle);
   if ( ret == nilHandle) apiErr;
   return ret;
}

Bool
apc_prn_create( Handle self)
{
   return true;
}

Bool
apc_prn_select( Handle self, char * printer)
{
   int rc;
   PRQINFO3 ppi;
   HDC dc;
   rc = prn_query( printer, &ppi);
   if ( rc > 0)
   {
      ppi_destroy( &sys s. prn. ppi);
      memcpy( &sys s. prn. ppi, &ppi, sizeof( ppi));
   } else
      return false;

   if ( !( dc = prn_info_dc( self))) return false;
   if ( !DevQueryCaps( dc, CAPS_HORIZONTAL_FONT_RES, 1, ( PLONG) &sys res. x)) apiErrRet;
   if ( !DevQueryCaps( dc, CAPS_VERTICAL_FONT_RES, 1, ( PLONG) &sys res. y)) apiErrRet;
   if ( !DevCloseDC( dc)) apiErr;
   return true;
}

char*
apc_prn_get_selected( Handle self)
{
   return sys s. prn. ppi. pszComment;
}

char *
apc_prn_get_default( Handle self)
{
   PRQINFO3 p;
   int rc = prn_query( nil, &p);
   if ( rc <= 0) return "";
   free( sys s. prn. defaultPrn);
   sys s. prn. defaultPrn = malloc( strlen(( char*) p. pszComment) + 1);
   strcpy( sys s. prn. defaultPrn, ( char*) p. pszComment);
   ppi_destroy( &p);
   return sys s. prn. defaultPrn;
}

Point
apc_prn_get_size( Handle self)
{
   HDC dc;
   PHCINFO hc;
   LONG i, forms;
   Point toRet = {0,0};

   if ( is_opt( optInDraw)) return ( Point){ sys s. prn. size. cx, sys s. prn. size. cy};
   if ( !( dc = prn_info_dc( self))) return toRet;
   forms = DevQueryHardcopyCaps( dc, 0, 0, nil);
   if ( forms == DQHC_ERROR) {
      apiErr;
      return toRet;
   }
   hc = malloc( forms * sizeof( HCINFO));
   if ( DevQueryHardcopyCaps( dc, 0, forms, hc) == DQHC_ERROR) {
      apiErr;
      return toRet;
   }
   if ( !DevCloseDC( dc)) apiErr;
   for ( i = 0; i < forms; i++)
   {
      if ( hc[ i]. flAttributes & HCAPS_CURRENT)
      {
         toRet = ( Point){ hc[i]. xPels, hc[i]. yPels};
         break;
      }
   }
   if ( toRet.x == 0 && toRet. y == 0 && forms != 0)
      toRet = ( Point) { hc[0]. xPels, hc[0]. yPels};
   free( hc);
   return toRet;
}


void
apc_prn_destroy( Handle self)
{
   ppi_destroy( &sys s. prn. ppi);
   if ( sys ps) if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( sys dc) if ( !DevCloseDC( sys dc)) apiErr;
   free( sys s. prn. defaultPrn);
   sys dc = sys ps = nilHandle;
}

Bool
apc_prn_begin_doc( Handle self, char * docName)
{
   LONG out = 0;
   char * c;
   DEVOPENSTRUC dev;
   char buf[ 1024];
   char bufcm[ 1024];
   LONG res;

   memset( &dev, 0, sizeof( dev));
   dev. pszLogAddress = sys s. prn. ppi. pszName;
   strcpy( buf, sys s. prn. ppi. pszDriverName);
   c = strchr( buf, '.');
   if ( c) *c = '\0';
   dev. pszDriverName = buf;
   dev. pdriv         = sys s. prn. ppi. pDriverData;
   dev. pszDataType   = "PM_Q_STD";
   strcpy( bufcm, application ? (( PComponent) application)-> name : "APC");
   dev. pszComment    = bufcm;

   if ( strlen( sys s. prn. ppi. pszPrProc)) dev. pszQueueProcName   = sys s. prn. ppi. pszPrProc;
   if ( strlen( sys s. prn. ppi. pszParms))  dev. pszQueueProcParams = sys s. prn. ppi. pszParms;
   dev .pszSpoolerParams = nil;
   dev .pszNetworkParams = nil;
   sys dc = DevOpenDC( guts. anchor, OD_QUEUED, "*", 9, (PDEVOPENDATA)&dev, nilHandle);
   if ( !sys dc) apiErrRet;
   DevQueryCaps( sys dc, CAPS_WIDTH,  1, &sys s. prn. size. cx );
   DevQueryCaps( sys dc, CAPS_HEIGHT, 1, &sys s. prn. size. cy );
   sys ps = GpiCreatePS( guts. anchor, sys dc, &sys s. prn. size,
      PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
   if ( !sys ps)
   {
      apiErr;
      DevCloseDC( sys dc);
      sys dc = nilHandle;
      return false;
   }
   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;
   res = DevEscape( sys dc, DEVESC_STARTDOC, strlen( docName), docName, &out, nil);
   if ( res != DEV_OK)
   {
      apiAltErr( res);
      GpiDestroyPS( sys ps);
      DevCloseDC( sys dc);
      sys dc = sys ps = nilHandle;
      return false;
   }
   sys fontId = 0;
   sys fontHash = create_fontid_hash();
   apt_clear( aptFontExists);
   apc_gp_set_color( self, sys lbs[ 0]);
   apc_gp_set_back_color( self, sys lbs[ 1]);
   apc_gp_set_font( self, &var font);
   save_gp_attrs( self);
   return true;
}

void
apc_prn_end_doc( Handle self)
{
   LONG out = 0;
   LONG res;
   res = DevEscape( sys dc, DEVESC_ENDDOC, 0, nil, &out, nil);
   if ( res != DEV_OK) {
      apiAltErr( res);
      DevEscape( sys dc, DEVESC_ABORTDOC, 0, nil, &out, nil);
   }
   GpiDestroyPS( sys ps);
   DevCloseDC( sys dc);
   sys dc = sys ps = nilHandle;
   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   restore_gp_attrs( self);
}

void
apc_prn_new_page( Handle self)
{
   LONG out = 0;
   LONG res = DEV_ERROR;
   if ( sys dc) res = DevEscape( sys dc, DEVESC_NEWFRAME, 0, nil, &out, nil);
   if ( res != DEV_OK) apiAltErr( res);
}

void
apc_prn_abort_doc( Handle self)
{
   LONG out = 0;
   LONG res = DevEscape( sys dc, DEVESC_ABORTDOC, 0, nil, &out, nil);
   if ( res != DEV_OK) apiAltErr( res);
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
   sys dc = sys ps = nilHandle;
   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   restore_gp_attrs( self);
}

Bool
apc_prn_setup( Handle self)
{
   char *c;
   char drv[ 1024];
   char prns[ 1024];
   LONG sprc;
   HDC  dc;

   strcpy( prns, sys s. prn. ppi. pszPrinters);
   c = strchr( prns, ',');
   if ( c) *c = '\0';
   strcpy( drv, sys s. prn. ppi. pszDriverName);
   c = strchr( drv, '.');
   if ( c) {
     *c = '\0';
     c++;
   }

   sprc = DevPostDeviceModes( guts. anchor, nil, drv,
      c, prns, DPDM_POSTJOBPROP);
   if ( sprc == DPDM_ERROR) apiErrRet;
   if ( sprc == DPDM_NONE) apcErrRet( errNoPrnSettableOptions);
   if ( sprc <= 0) apiErrRet;
   if ( sprc > sys s. prn. ppi. pDriverData-> cb) apiErrRet;
   if ( DevPostDeviceModes( guts. anchor,
      sys s. prn. ppi. pDriverData,
      drv,
      c, prns,
      DPDM_POSTJOBPROP
   ) != DEV_OK) apiErrRet;

   if ( !( dc = prn_info_dc( self))) return false;
   if ( !DevQueryCaps( dc, CAPS_HORIZONTAL_FONT_RES, 1, ( PLONG) &sys res. x)) apiErrRet;
   if ( !DevQueryCaps( dc, CAPS_VERTICAL_FONT_RES, 1, ( PLONG) &sys res. y)) apiErrRet;
   if ( !DevCloseDC( dc)) apiErr;

   return true;
}

Point
apc_prn_get_resolution( Handle self)
{
   return sys res;
}

// PRINTER END
// HELP

static Bool recreateHelp = false;

static Bool
help_create( void) {
   HELPINIT h;
   HWND     hi;
   int      f;
   char *   fName = (( PApplication) application)-> helpFile;
   char     buf[ 512 + 0x6B];

   f = open( fName, O_RDONLY);
   if ( f == -1) apcErrRet( errApcError);
   read( f, buf, sizeof( buf));
   close( f);
   if ( strncmp( buf, "HSP", 3) != 0) apcErrRet( errApcError);
   buf[ sizeof( buf) - 1] = 0;

   memset( &h, 0, sizeof( h));
   h. cb = sizeof( h);
   (int)h. phtHelpTable = 1;
   h. pszHelpLibraryName = fName;
   h. pszHelpWindowTitle = &buf[ 0x6B];
   hi = WinCreateHelpInstance( guts. anchor, &h);
   if ( !hi) apiErrRet;
   guts. helpWnd = hi;
// if ( !WinAssociateHelpInstance( hi, DHANDLE( application))) apiErr;
   return true;
}

static int ctx_hmp2HM[] =
{
   hmpMain,          HM_HELP_CONTENTS,
   hmpContents,      HM_HELP_CONTENTS,
   hmpExtra,         HM_HELP_INDEX,
   endCtx
};

Bool
apc_help_open_topic( Handle self, long command)
{
   int  rc;
   long hm;
   MPARAM hm1, hm2;
   if ( guts. helpWnd == nilHandle || recreateHelp) {
      if ( recreateHelp) apc_help_close( self);
      if ( !help_create()) return false;
   }
   hm  = ctx_remap_def( command, ctx_hmp2HM, true, HM_DISPLAY_HELP);
   hm1 = ( hm == HM_DISPLAY_HELP) ? MPFROMLONG( command)        : 0;
   hm2 = ( hm == HM_DISPLAY_HELP) ? MPFROMSHORT( HM_RESOURCEID) : 0;
   rc = ( int) WinSendMsg( guts. helpWnd, hm, hm1, hm2);
   if ( rc != 0) {
      apiAltErr( rc);
      return false;
   }
   return true;
}

void
apc_help_close( Handle self)
{
   if ( guts. helpWnd) {
//    WinAssociateHelpInstance( guts. helpWnd, nilHandle);
      WinDestroyHelpInstance( guts. helpWnd);
      guts. helpWnd = nilHandle;
   }
   recreateHelp  = false;
}

void
apc_help_set_file( Handle self, char * helpFile)
{
   recreateHelp = true;
}

extern void *dlopen( char *path, int mode);

void *
apc_dlopen(char *path, int mode)
{
   void *RETVAL;
   APIRET rc;

   RETVAL = dlopen(path, mode) ;

   if ( RETVAL) {
      PSZ oldLibPath = NULL, newLibPath;
      ULONG bSize = 4096;
      int done = 0;

      do {
         char *p1, *p2;

         rc = DosAllocMem( ( PPVOID)&oldLibPath, bSize, PAG_COMMIT | PAG_READ | PAG_WRITE);
         if ( rc != NO_ERROR) {
             break;
         }
         rc = DosQueryExtLIBPATH( oldLibPath, BEGIN_LIBPATH);
         if ( rc != NO_ERROR) {
            DosFreeMem( oldLibPath);
            if ( rc == ERROR_INSUFFICIENT_BUFFER) {
               bSize += 4096;
               continue;
            }
            break;
         }
         newLibPath = ( PSZ)malloc( strlen( oldLibPath) + strlen( path) + 1);
         if ( newLibPath == NULL) {
            DosFreeMem( oldLibPath);
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
         }
         strcpy( newLibPath, path);
         p1 = strrchr( newLibPath, '/');
         p2 = strrchr( newLibPath, '\\');
         if ( p1 == NULL) {
            p1 = p2;
         }
         else if ( p1 < p2) {
            p1 = p2;
         }
         if ( p1 != NULL) {
            *p1++ = ';';
            *p1 = 0;
            strcat( newLibPath, oldLibPath);
            rc = DosSetExtLIBPATH( newLibPath, BEGIN_LIBPATH);
         }
         free( newLibPath);
         DosFreeMem( oldLibPath);
         done = 1;
      } while ( ! done);
   }

   return ( rc == NO_ERROR ? RETVAL : NULL);
}

// HELP END
// APC END
