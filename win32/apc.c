#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Menu.h"
#include "Window.h"
#include "Application.h"


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

#define WinShowWindow(WND) SetWindowPos( WND, nil, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
#define WinHideWindow(WND) SetWindowPos( WND, nil, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW);

Bool
apc_application_begin_paint ( Handle self)
{
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
   if ( !PostMessage( guts. logger, WM_CLOSE, 0, 0)) apiErr;
}

void
apc_application_destroy( Handle self)
{
   if ( IsWindow( sys handle))  {
      if ( !KillTimer( sys handle, TID_USERMAX)) apiErr;
      if ( !DestroyWindow( sys handle)) apiErr;
   }
   if ( !loggerDead)
      PostMessage( guts. logger, WM_QUIT, 0, 0);
   PostThreadMessage( guts. mainThreadId, WM_TERMINATE, 0, 0);
   loggerDead = true;
}

void
apc_application_end_paint   ( Handle self)
{
   apcErrClear;
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
apc_application_get_gui_info( char * description)
{
   strcpy( description, "Windows");
   return guiWindows;
}

Handle
hwnd_to_view( HWND win)
{
   Handle h;
   LONG ll;
   int i = 2;
   while (i--) {
      if (( !win) || ( !IsWindow( win))) return nilHandle;
      if ( GetWindowThreadProcessId( win, nil) != guts. mainThreadId) return nilHandle;
      h = GetWindowLong( win, GWL_USERDATA);
      if ( !h) return nilHandle;
      ll = GetWindowLong( win, GWL_WNDPROC);
      if (
          ( ll == ( LONG) generic_view_handler) ||
          ( ll == ( LONG) generic_app_handler) ||
          ( ll == ( LONG) generic_frame_handler)
         ) return h;
   }
   return nilHandle;
}


int
apc_application_get_os_info( char * system, char * release, char * vendor, char * arch)
{
   SYSTEM_INFO si;
   OSVERSIONINFO os = { sizeof( OSVERSIONINFO)};
   DWORD  version;
   GetSystemInfo( &si);
   version = GetVersion();
   GetVersionEx( &os);
   if ( system ) {
      if ( IS_NT)    strcpy( system, "Windows NT"); else
      if ( IS_WIN95) {
         if ((os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            ((os.dwMajorVersion > 4) ||
            ((os.dwMajorVersion == 4) && (os.dwMinorVersion > 0))))
            strcpy( system, "Windows 98");
         else
            strcpy( system, "Windows 95");
      }
         strcpy( system, "Windows");
   }
   if ( vendor )
      strcpy( vendor, "Microsoft");
   if ( arch   ) {
      char * pb = "Unknown";
      switch ( si. wProcessorArchitecture) {
      case PROCESSOR_ARCHITECTURE_INTEL :   pb = "i386";  break;
      case PROCESSOR_ARCHITECTURE_MIPS  :   pb = "MIPS";  break;
      case PROCESSOR_ARCHITECTURE_ALPHA :   pb = "Alpha"; break;
      case PROCESSOR_ARCHITECTURE_PPC   :   pb = "PPC";   break;
      }
      strcpy( arch, pb);
   }
   if ( release) sprintf( release, "%d.%d",
      LOBYTE( LOWORD( version)),
      HIBYTE( LOWORD( version))
   );
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
   Point ret;
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
              ( musClk. msg. pt. x   == msg-> pt. x)   &&
              ( musClk. msg. pt. y   == msg-> pt. y)   &&
              ( musClk. msg. wParam  == ( msg-> wParam & ( MK_CONTROL|MK_SHIFT)))  &&
              ( musClk. msg. lParam  == msg-> lParam)
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
   DispatchMessage( msg);
   kill_zombies();
   stylus_clean();
   return true;
}


void
apc_application_go( Handle self)
{
   MSG msg;
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
   POINT pt = {point. x, sys lastSize. y - point. y - 1};
   HWND  p =  WindowFromPoint( pt);
   if ( p) {
      POINT xp = pt;
      MapWindowPoints( HWND_DESKTOP, p, &xp, 1);
      p = ChildWindowFromPoint( p, xp);
   } else
      p = ChildWindowFromPoint( HWND_DESKTOP, pt);

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
   var handle = nilHandle;
   if ( d == nil) return;
   free( d);
   c-> sysData = nil;
}


static void
process_transparents( Handle self)
{
   int i;
   RECT mr;
   GetWindowRect(( HWND) var handle, &mr);
again:
   for ( i = 0; i < var widgets. count; i++) {
      Handle x = var widgets. items[ i];
      if ( dsys(x) options. aptTransparent && IsWindowVisible( DHANDLE( x))) {
         RECT r, dr;
         GetWindowRect( DHANDLE( x), &r);
         IntersectRect( &dr, &r, &mr);
         if ( !IsRectEmpty( &dr))
            InvalidateRect( DHANDLE( x), nil, false);
      }
   }
   if ( self != application) {
      self = application;
      goto again;
   }
}

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
  HWND wnd = ( HWND) var handle;
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
           if ( !SetTimer( wnd, var handle, sys s. timer. timeout, nil)) apiErr;
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
          if ( !clipOwner) parentView = HWND_DESKTOP;
          if ( !taskListed && ( parentView == HWND_DESKTOP || parentView == nilHandle))
              parentView = DHANDLE( application);
          if ( !( frame = CreateWindowEx( exstyle, "GenericFrame", "", style, 0, 0, 0, 0,
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
          if ( !( ret = CreateWindowEx( 0,  "Generic", "", WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                0, 0, r. right - r. left, r. bottom - r. top, frame, nilHandle,
                guts. instance, nil)))
             apiErr;
          sys handle = frame;
          SetWindowLong( frame, GWL_USERDATA, ( LONG) self);
       }
       break;
    case WC_CUSTOM:
       if ( !clipOwner || owner == application) {
            // parentView = nil;
            style &= ~WS_CHILD;
            style |= WS_POPUP;
            // style |= WS_SYSMENU | WS_DLGFRAME | WS_THICKFRAME;
            exstyle |= WS_EX_TOOLWINDOW;
       }
       if ( !( ret = CreateWindowEx( exstyle,  "Generic", "", style, 0, 0, 0, 0,
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
         PAbstractMenu menu = ( PAbstractMenu)((( PWindow) self)-> menu);
         if ( menu) menu-> self-> set_selected(( Handle) menu, true);
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
                   int borderStyle, Bool taskList, int windowState)
{
  Bool reset = false;
  ViewProfile vprf;
  int oStage = var stage;
  WindowData ws;
  WINDOWPLACEMENT wp;
  DWORD style = WS_CLIPCHILDREN | WS_OVERLAPPED
     | (( borderIcons &  biSystemMenu) ? WS_SYSMENU     : 0)
     | (( borderIcons &  biMinimize)   ? WS_MINIMIZEBOX : 0)
     | (( borderIcons &  biMaximize)   ? WS_MAXIMIZEBOX : 0)
     | (( borderIcons &  biTitleBar)   ? 0              : WS_POPUP)
     | (( borderStyle == bsSizeable)   ? WS_THICKFRAME  : 0)
     | (( borderStyle == bsSingle  )   ? WS_BORDER      : 0)
     | (( borderStyle == bsDialog  )   ? WS_BORDER      : 0)
//   | (  taskList                     ? FCF_TASKLIST   : 0)
  ;
  DWORD exstyle = 0
     | (( borderStyle == bsDialog  )   ? WS_EX_DLGMODALFRAME : 0)
//   | (  taskList                     ? 0                   : WS_EX_TOOLWINDOW)
  ;

  if ( !kind_of( self, CWidget)) apcErrRet( errInvObject);
  apcErrClear;
  lock( true);
  if (( var handle != nilHandle) && (
       ( DHANDLE( owner) != sys owner)
    || ( borderStyle != sys s. window. borderStyle)
    || ( borderIcons != sys s. window. borderIcons)
    || ( taskList    != is_apt( aptTaskList))
    || ( clipOwner   != is_apt( aptClipOwner))
  ))
  {
     if ( sys recreateData) {
        memcpy( &vprf, sys recreateData, sizeof( vprf));
        free( sys recreateData);
        sys recreateData = nil;
     } else
        get_view_ex( self, &vprf);
     ws = sys s. window;
     wp. length = sizeof( wp);
     GetWindowPlacement( HANDLE, &wp);
     reset = true;
  }


  if ( reset || ( var handle == nilHandle))
     if ( !create_group( self, owner, syncPaint, clipOwner, taskList, WC_FRAME, style, exstyle, &vprf)) {
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
     SetWindowPlacement( HANDLE, &wp);
     var stage = oStage;
  }
  else {
     WINDOWPLACEMENT wp;
     if ( windowState != wsNormal) {
        GetWindowPlacement( HANDLE, &wp);
        wp. showCmd = ( windowState == wsMinimized) ? SW_MINIMIZE : SW_SHOWMAXIMIZED;
        SetWindowPlacement( HANDLE, &wp);
     }
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
      HWND w = HANDLE;
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
apc_window_close( Handle self) {
   if ( !PostMessage( HANDLE, WM_CLOSE, 0, 0)) apiErrRet;
   return true;
}

int
apc_window_get_border_icons( Handle self)
{
   return sys s. window. borderIcons;
}

int
apc_window_get_border_style( Handle self)
{
   return sys s. window. borderStyle;
}

Point
apc_window_get_client_pos( Handle self)
{
   Point delta = apc_sys_get_window_borders( sys s. window. borderStyle);
   Handle parent = var self-> get_parent( self);
   Point p, sz   = CWidget( parent)-> get_size( parent);
   RECT  r;

   if ( apc_window_get_window_state( self) == wsMinimized) {
      WINDOWPLACEMENT w;
      w. length = sizeof( w);
      GetWindowPlacement( HANDLE, &w);
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
   Point p;
   if ( apc_window_get_window_state( self) == wsMinimized) {
      // cannot acquire client extension at this time. Using euristic calculations.
      WINDOWPLACEMENT w;
      Point delta = apc_sys_get_window_borders( sys s. window. borderStyle);
      int   menuY  = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
      int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
                     GetSystemMetrics( SM_CYCAPTION) : 0;
      w. length = sizeof( w);
      GetWindowPlacement( HANDLE, &w);
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
   HICON    p = ( HICON) SendMessage( HANDLE, WM_GETICON, ICON_BIG, 0);
   HCURSOR  save = sys pointer;
   Bool ret;
   if ( p    == nilHandle) apcErrRet( errInvWindowIcon);
   if ( icon == nilHandle) apcErrRet( errInvParams);
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
   WINDOWPLACEMENT s;
   s. length = sizeof( s);
   if ( !GetWindowPlacement( HANDLE, &s)) apiErr;
   if ( s. showCmd == SW_SHOWMAXIMIZED) return wsMaximized;
   if ( s. showCmd == SW_SHOWMINIMIZED) return wsMinimized;
   return wsNormal;
}

Bool
apc_window_get_task_listed( Handle self)
{
   return is_apt( aptTaskList);
}

void
apc_window_set_caption( Handle self, char * caption)
{
   if ( !SetWindowText( HANDLE, caption)) apiErr;
}

void
apc_window_set_client_pos( Handle self, int x, int y)
{
   Point delta = apc_sys_get_window_borders( sys s. window. borderStyle);
   RECT  r;
   Handle parent = var self-> get_parent( self);
   Point sz = CWidget( parent)-> get_size( parent);


   if ( var stage == csConstructing && apc_window_get_window_state( self) != wsNormal) {
      WINDOWPLACEMENT w;
      w. length = sizeof( w);
      GetWindowPlacement( HANDLE, &w);
      w. rcNormalPosition. top    += sz. y - y + delta. y - w. rcNormalPosition. bottom;
      w. rcNormalPosition. bottom  = sz. y - y + delta. y;
      w. rcNormalPosition. right  += x - delta. x - w. rcNormalPosition. left;
      w. rcNormalPosition. left    = x - delta. x;
      w. flags   = 0;
      w. showCmd  = 0;
      SetWindowPlacement( HANDLE, &w);
   } else {
      GetWindowRect( HANDLE, &r);
      x -= delta. x;
      y  = sz. y - ( r. bottom - r. top) - y + delta. y;
      SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
   }
}

void
apc_window_set_client_size( Handle self, int x, int y)
{
   RECT r, c, c2;
   int  ws = apc_window_get_window_state( self);
   if ( x < 0) x = 0;
   if ( y < 0) y = 0;

   if (( var stage == csConstructing && ws != wsNormal) || ws == wsMinimized) {
   // if (( var stage == csConstructing) || ws == wsMinimized) {
      WINDOWPLACEMENT w;
      Point delta = apc_sys_get_window_borders( sys s. window. borderStyle);

      w. length = sizeof( w);
      GetWindowPlacement( HANDLE, &w);
      GetWindowRect( HANDLE, &c2);
      if ( ws == wsMaximized) {
         GetClientRect( HANDLE, &c);
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
      SetWindowPlacement( HANDLE, &w);
   } else {
      GetWindowRect( HANDLE, &r);
      GetClientRect( HANDLE, &c);
      SetWindowPos ( HANDLE, 0,
         r. left,
         r. top - y + ( c. bottom - c. top),
         x + r. right  - r. left - c. right + c. left,
         y + r. bottom - r. top  - c. bottom + c. top,
         SWP_NOZORDER | SWP_NOACTIVATE);
   }
}


Bool
apc_window_set_menu( Handle self, Handle menu)
{
   Point size  = var self-> get_size( self);
   HMENU wMenu = GetMenu( HANDLE);
   if ( menu)
      (( PComponent) menu)-> handle = nilHandle;
   apcErrClear;
   if ( wMenu) DestroyMenu( wMenu);
   if ( menu)
      (( PComponent) menu)-> handle = ( Handle) add_item( true, menu, (( PMenu) menu)-> tree);
   SetMenu( HANDLE, menu ? ( HMENU) (( PComponent) menu)-> handle : nilHandle);
   DrawMenuBar( HANDLE);
   var self-> set_size( self, size.x, size. y);
   return apcError == 0;
}


void
apc_window_set_icon( Handle self, Handle icon)
{
   HICON i;
   i = icon ? image_make_icon_handle( icon, guts. iconSizeLarge, nil) : nil;
   i = ( HICON) SendMessage( HANDLE, WM_SETICON, ICON_BIG, ( LPARAM) i);
   if ( i) DestroyIcon( i);
}

void
apc_window_set_window_state( Handle self, int state)
{
   int  fl = -1;
   switch ( state)
   {
      case wsMaximized: fl = SW_SHOWMAXIMIZED; break;
      case wsMinimized: fl = SW_MINIMIZE; break;
      case wsNormal   : fl = SW_RESTORE ; break;
   }
   if ( fl > 0)
   {
      int i, lock = sys lockState;
      while ( sys lockState) apc_widget_unlock( self);
      ShowWindow( HANDLE, fl);
      for ( i = 0; i < lock; i++) apc_widget_lock( self);
      sys s. window. state = state;
   }
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insertBefore)
{
   HWND wnd = HANDLE;
   HWND active = GetActiveWindow();

   if ( sys className != WC_FRAME) { apcErr( errInvParams); return false; }

   // setting window up
   guts. focSysDisabled = 1;
   ((( PWindow) self)-> self)-> exec_enter_proc( self, shared, insertBefore);
   SetWindowPos( wnd, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
   if ( sys s. window. state == wsMinimized)
      ShowWindow( wnd, SW_RESTORE);
   if ( !insertBefore) {
      SetActiveWindow( wnd);
      SetForegroundWindow( wnd);
   } else {
      HWND zorder = GetWindow( DHANDLE( insertBefore), GW_HWNDNEXT);
      if ( !zorder) zorder = HWND_BOTTOM;
      SetWindowPos( wnd, zorder, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
      if ( active)
         SetActiveWindow( active);
   }
   PostMessage( wnd, WM_DLGENTERMODAL, 1, 0);
   guts. focSysDisabled = 0;
}

Bool
apc_window_execute( Handle self, Handle insertBefore)
{
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
   return window_start_modal( self, true, insertBefore);
}

void
apc_window_end_modal( Handle self)
{
   HWND wnd = HANDLE;
   if ((( PWindow) self)-> modal == mtExclusive) {
      if ( self == (( PApplication) application)-> topExclModal)
         PostThreadMessage( guts. mainThreadId, WM_BREAKMSGLOOP, 0, self);
   }
   guts. focSysDisabled = 1;
   WinHideWindow( wnd);
   ((( PWindow) self)-> self)-> exec_leave_proc( self);
   if ( application) Application_popup_modal( application);
   guts. focSysDisabled = 0;
}

// View management
Point
apc_widget_client_to_screen( Handle self, Point p)
{
   POINT pt;
   Point sz = ((( PWidget) self)-> self)-> get_size( self);
   Point appSz = apc_application_get_size( application);
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
      create_group( self, owner, syncPaint, clipOwner, 0, WC_CUSTOM, WS_CHILD, exstyle, &vprf);
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

Bool
apc_widget_begin_paint( Handle self, Bool insideOnPaint)
{
   apcErrClear;

   if ( is_apt( aptTransparent))
   {
      if ( IsWindowVisible(( HWND) var handle))
      {
         HWND parent = dsys((( PWidget) self)-> owner) parent;
         MSG  msg;
         list_add( &guts. transp, self);
         WinHideWindow(( HWND) var handle);
         UpdateWindow( parent);
         while ( PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
            DispatchMessage( &msg);
         if ( parent == HWND_DESKTOP) Sleep( 1);
         WinShowWindow(( HWND) var handle);
         list_delete( &guts. transp, self);
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

   if ( is_opt( optBuffered)) {
      RECT r;
      HBITMAP bm;
      HDC dc;
      HPALETTE oldPal;

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
         if ( !SelectObject( dc, bm)) apiErr;
         sys bm = bm;
         apc_gp_set_transform( self, -r. left, -r. top);
         sys transform2. x = r. left;
         sys transform2. y = r. top;
      } else
         apiErr;
   }
   hwnd_enter_paint( self);

   return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
   HRGN rgn;
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


void
apc_widget_destroy( Handle self)
{
   if ( sys pointer2) if ( !DestroyCursor( sys pointer2)) apiErr;
   if ( sys recreateData) free( sys recreateData);
   free( sys timeDefs);
   if ( var handle == nilHandle) return;

   if ( sys className == WC_FRAME)
      guts. topWindows--;

   if ( !DestroyWindow( HANDLE)) apiErr;
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
   if ( is_opt( optBuffered) && ( sys bm != nilHandle)) {
      if ( !SetViewportOrgEx( sys ps, 0, 0, nil)) apiErr;
      if ( !BitBlt( sys ps2, sys transform2. x, sys transform2. y, var w, var h, sys ps, 0, 0, SRCCOPY)) apiErr;
      DeleteObject( sys bm);
      if ( sys pal) {
         SelectPalette( sys ps2, sys pal2, 1);
         SelectPalette( sys ps, sys stockPalette, 1);
         DeleteObject( sys pal);
         sys pal = sys pal2 = nil;
      }

      DeleteDC( sys ps);
      sys ps = sys ps2;
      sys bm = sys ps2 = nil;
   }

   hwnd_leave_paint( self);

   if ( sys ps != nilHandle)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
      } else if ( is_apt( aptWinPS))
         if ( !ReleaseDC(( HWND) var handle, sys ps)) apiErr;
   }
   sys ps = sys pal = nilHandle;
   apt_clear( aptWinPS);
   apt_clear( aptWM_PAINT);
   apt_clear( aptCompatiblePS);
}

void
apc_widget_end_paint_info( Handle self)
{
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
   return is_apt( aptClipOwner);
}

Rect
apc_widget_get_clip_rect( Handle self)
{
   Rect r;
   RECT * c = sys pClipRect;
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
   return sys viewColors[ index];
}

Bool
apc_widget_get_first_click( Handle self)
{
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
   return ( ApiHandle) HANDLE;
}

Rect
apc_widget_get_invalid_rect( Handle self)
{
   Rect r  = {0,0,0,0};
   if ( GetUpdateRect(( HWND) var handle, ( PRECT) &r, false))
      return *( map_RECT( self, ( PRECT) &r));
   return r;
}

Point
apc_widget_get_pos( Handle self)
{
   RECT  r;
   Point p, sz;
   Handle parent = is_apt( aptClipOwner) ? var owner : application;
   sz = ((( PWidget) parent)-> self)-> get_size( parent);
   if ( sys className == WC_FRAME && apc_window_get_window_state( self) == wsMinimized) {
      WINDOWPLACEMENT w;
      w. length = sizeof( w);
      GetWindowPlacement( HANDLE, &w);
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
   Point p;
   if ( sys className == WC_FRAME && apc_window_get_window_state( self) == wsMinimized) {
      WINDOWPLACEMENT w;
      w. length = sizeof( w);
      GetWindowPlacement( HANDLE, &w);
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
   return GetCapture() == ( HWND) var handle;
}

Bool
apc_widget_is_enabled( Handle self)
{
   return is_apt( aptEnabled);
   // return IsWindowEnabled( HANDLE);
}

Bool
apc_widget_is_responsive( Handle self)
{
   Bool ena = true;
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
   return is_apt( aptFocused);
}

Bool
apc_widget_is_visible( Handle self)
{
   return IsWindowVisible( HANDLE);
}

void
apc_widget_invalidate_rect( Handle self, Rect rect)
{
   if ( sys lockState == 0) {
      if ( !InvalidateRect (( HWND) var handle, map_Rect( self, &rect), false)) apiErr;
      if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
      process_transparents( self);
   }
}

void
apc_widget_lock( Handle self)
{
   if ( sys lockState++ == 0) {
      apt_assign( aptLockVisState, ( GetWindowLong( HANDLE, GWL_STYLE) & WS_VISIBLE) == WS_VISIBLE);
      if ( !LockWindowUpdate( HANDLE)) apiErr;
   }
}

static Bool
repaint_all( Handle owner, Handle self, void * dummy)
{
   if ( !is_apt( aptTransparent)) {
      if ( !InvalidateRect(( HWND) var handle, nil, false)) apiErr;
      if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
      var self-> first_that( self, repaint_all, nil);
   }
   process_transparents( self);
   return false;
}

void
apc_widget_repaint( Handle self)
{
   if ( sys lockState == 0) {
      if ( !InvalidateRect(( HWND) var handle, nil, false)) apiErr;
      if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
      var self-> first_that( self, repaint_all, nil);
      process_transparents( self);
   }
}

Point
apc_widget_screen_to_client( Handle self, Point p)
{
   POINT pt;
   Point sz = ((( PWidget) self)-> self)-> get_size( self);
   Point appSz = apc_application_get_size( application);
   pt. x = p. x;
   pt. y = appSz. y - p. y;
   MapWindowPoints( nilHandle, ( HWND) var handle, &pt, 1);
   p. x = pt. x;
   p. y = sz. y - pt. y;
   return p;
}

void
apc_widget_scroll( Handle self, int horiz, int vert, Bool scrollChildren)
{
   HideCaret(( HWND) var handle);
   if ( !ScrollWindowEx(( HWND) var handle,
      horiz, -vert, NULL, sys pClipRect, NULL, NULL,
      SW_INVALIDATE | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
   )) apiErr;
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
   ShowCaret(( HWND) var handle);
}

void
apc_widget_scroll_rect( Handle self, int horiz, int vert, Rect r, Bool scrollChildren)
{
   HideCaret(( HWND) var handle);
   if ( !ScrollWindowEx(( HWND) var handle,
      horiz, -vert, map_Rect( self, &r), sys pClipRect, NULL, NULL,
      SW_INVALIDATE | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
   )) apiErr;
   ShowCaret(( HWND) var handle);
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
}

void
apc_widget_set_capture( Handle self, Bool capture)
{
   if ( capture)
      SetCapture(( HWND) var handle);
   else
      if ( !ReleaseCapture()) apiErr;
}

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

void
apc_widget_set_clip_rect( Handle self, Rect c)
{
   int ly = sys lastSize. y;
   RECT * cr = &sys clipRect;
   // inclusive-inclusive

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
   sys viewColors[ index] = color;
   SendMessage(( HWND) var handle, WM_COLORCHANGED, index, 0);
}

void
apc_widget_set_enabled( Handle self, Bool enable)
{
   apt_assign( aptEnabled, enable);
   if (( sys className == WC_FRAME) || ( var owner == application))
      EnableWindow( HANDLE, enable);
}

void
apc_widget_set_first_click( Handle self, Bool firstClick)
{
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
   SendMessage(( HWND) var handle, WM_FONTCHANGED, 0, 0);
}

void
apc_widget_set_palette( Handle self)
{
   if ( guts. displayBMInfo. bmiHeader. biBitCount <= 8)
      palette_change( self);
}

void
apc_widget_set_pos( Handle self, int x, int y)
{
   Handle parent = is_apt( aptClipOwner) ? var owner : application;
   Point sz = ((( PWidget) parent)-> self)-> get_size( parent);
   RECT r;

   if ( sys className == WC_FRAME) {
      HWND h = HANDLE;
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w;
         w. length = sizeof( w);
         GetWindowPlacement( h, &w);
         w. rcNormalPosition. top    += sz. y - y - w. rcNormalPosition. bottom;
         w. rcNormalPosition. bottom  = sz. y - y;
         w. rcNormalPosition. right  += x - w. rcNormalPosition. left;
         w. rcNormalPosition. left    = x;
         w. flags = w. showCmd = 0;
         if ( !SetWindowPlacement( h, &w)) apiErr;
         return;
      }
   }
   GetWindowRect( HANDLE, &r);
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
   y = sz. y - y - r. bottom + r. top;
   if ( !SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
}

void
apc_widget_set_size( Handle self, int width, int height)
{
   RECT c, r;
   if ( width  < 0) width = 0;
   if ( height < 0) height = 0;
   if ( sys className == WC_FRAME) {
      HWND h = HANDLE;
      int  ws = apc_window_get_window_state( self);
      if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
         WINDOWPLACEMENT w;
         w. length = sizeof( w);
         GetWindowPlacement( h, &w);
         w. rcNormalPosition. top    = w. rcNormalPosition. bottom - height;
         w. rcNormalPosition. right  = width + w. rcNormalPosition. left;
         w. flags = w. showCmd = 0;
         if ( !SetWindowPlacement( h, &w)) apiErr;
         return;
      }
   }
   GetWindowRect( HANDLE, &r);
   if ( is_apt( aptClipOwner) && ( var owner != application))
      MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
   GetClientRect( HANDLE, &c);
   if ( !SetWindowPos( HANDLE, 0,
      r. left,
      r. top - height + ( c. bottom - c. top),
      width  + r. right  - r. left - c. right + c. left,
      height + r. bottom - r. top  - c. bottom + c. top,
      SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
}

void
apc_widget_set_tab_order( Handle self, int tabOrder)
{
}

void
apc_widget_set_visible( Handle self, Bool show)
{
   if ( sys lockState)
   {
      apt_assign( aptLockVisState, show);
      return;
   }
   if ( !SetWindowPos( HANDLE, nilHandle, 0, 0, 0, 0,
        ( show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
   if ( !is_apt( aptClipOwner)) {
      InvalidateRect(( HWND) var handle, nil, false);
      process_transparents( self);
   }
}

void
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
   HWND opt = ( top) ? HWND_TOP : HWND_BOTTOM;
   if ( behind != nilHandle) opt = DHANDLE( behind);
   if ( !SetWindowPos( HANDLE, opt, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)) apiErr;
   // unexpected stupig Windows bug'o'feature - when only window is available to a thread,
   // bring_to_front does not do anything! - so fixing
  // if (( sys className == WC_FRAME) && ( opt == HWND_TOP) && ( guts. topWindows < 2))
  //    apc_window_activate( self);
}

void
apc_widget_unlock( Handle self)
{
   if ( --sys lockState == 0) {
      if ( !LockWindowUpdate( nilHandle)) apiErr;
      if ( !SetWindowPos( HANDLE, 0, 0, 0, 0, 0,
            ( is_apt( aptLockVisState) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) |
             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErr;
   }
}

void
apc_widget_update( Handle self)
{
   if ( !UpdateWindow(( HWND) var handle)) apiErr;
}

void
apc_widget_validate_rect( Handle self, Rect rect)
{

   if ( !ValidateRect (( HWND) var handle, map_Rect( self, &rect))) apiErr;
}

// View attributes

int
apc_kbd_get_state( Handle self)
{
  return
      (( GetKeyState( VK_MENU)    < 0) ? kbAlt      : 0) |
      (( GetKeyState( VK_CONTROL) < 0) ? kbCtrl     : 0) |
      (( GetKeyState( VK_SHIFT)   < 0) ? kbShift    : 0);
}

Bool
apc_menu_create( Handle self, Handle owner)
{
   sys className = WC_MENU;
   sys owner     = DHANDLE( owner);
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
      if ( IsWindow(( HWND) var handle) && !DestroyMenu(( HMENU) var handle)) apiErr;
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
apc_menu_item_set_accel( Handle self, PMenuItemReg m, char * accel)
{
   MENUITEMINFO mii = {sizeof( MENUITEMINFO)};
   char buf [ 1024];
   UINT flags;

   if ( !var handle) return;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) {
      apiErr;
      return;
   }

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
   CheckMenuItem(( HMENU) var handle,
      m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( check ? MF_CHECKED : MF_UNCHECKED));
}

void
apc_menu_item_set_enabled( Handle self, PMenuItemReg m, Bool enabled)
{
   if ( !var handle) return;
   EnableMenuItem(( HMENU) var handle,
      m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( enabled ? MF_ENABLED : MF_GRAYED));
}

void
apc_menu_item_set_key( Handle self, PMenuItemReg m, int key)
{
}

void
apc_menu_item_set_text( Handle self, PMenuItemReg m, char * text)
{
   MENUITEMINFO mii = {sizeof( MENUITEMINFO)};
   char buf [ 1024];
   UINT flags;

   if ( !var handle) return;
   flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
   if ( flags == 0xFFFFFFFF) {
      apiErr;
      return;
   }

   if ( m-> accel)
      snprintf( buf, 1024, "%s\t%s", text, m-> accel);
   else
      strncpy( buf, text, 1024);
   map_tildas( buf, strlen( text));

   ModifyMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
                    m-> id + MENU_ID_AUTOSTART, buf);
}

Bool
apc_popup_create( Handle self, Handle owner)
{
   sys owner = DHANDLE( owner);
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
apc_popup( Handle self, int x, int y)
{
   if ( !TrackPopupMenuEx(
      ( HMENU) var handle, TPM_LEFTBUTTON|TPM_LEFTALIGN|TPM_RIGHTBUTTON,
       x, dsys( var owner) lastSize. y - y, ( HWND)(( PComponent) var owner)-> handle, NULL)
      ) apiErrRet;
   return true;
}


int ctx_kb2VK[] = {
   kbNoKey       ,   0                 ,
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
   return WinHelp( DHANDLE( application), PApplication(application)-> helpFile, hm, data);
}

void
apc_help_close( Handle self)
{
   WinHelp( DHANDLE( application), PApplication(application)-> helpFile, HELP_QUIT, 0);
}

void
apc_help_set_file( Handle self, char * helpFile)
{
}


typedef LRESULT (*ApiMessageSender)(HWND hwnd, UINT msg, WPARAM mp1, LPARAM mp2);
void
apc_message( Handle self, PEvent ev, Bool post)
{
   ULONG msg;
   ApiMessageSender sender = post ? (ApiMessageSender) PostMessage : (ApiMessageSender) SendMessage;
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
             WPARAM mp1 = 0 |
               (( ev-> pos. mod & kbShift) ? MK_SHIFT   : 0) |
               (( ev-> pos. mod & kbCtrl ) ? MK_CONTROL : 0);
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
                if (( GetKeyState( VK_MENU) < 0) ^ (( ev-> pos. mod & kbAlt) != 0))
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
             Bool specF10 = ( ev-> key. key == kbF10) && !( ev-> key. mod & kbAlt);
             // constructing mp1
             if ( ev-> key. key == kbNoKey) {
                if ( ev-> key. code == 0) {
                   if ( ev-> key. mod & kbAlt   ) mp1 = VK_MENU;    else
                   if ( ev-> key. mod & kbShift ) mp1 = VK_SHIFT;   else
                   if ( ev-> key. mod & kbCtrl  ) mp1 = VK_CONTROL; else
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
                      (( c & 1) ? kbShift : 0) |
                      (( c & 2) ? kbCtrl  : 0) |
                      (( c & 4) ? kbAlt   : 0);
                }
             } else {
                 if ( ev-> key. key == kbShiftTab) {
                    mp1  = VK_TAB;
                    ev-> key. mod |= kbShift;
                 } else {
                    mp1 = ctx_remap( ev-> key. key, ctx_kb2VK, true);
                    if ( mp1 == 0) return;
                 }
                 scan = MapVirtualKey( mp1, 0);
             }

             // constructing msg
             msg = ( ev-> key. mod & kbAlt) ? (
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


void  apc_show_message( char * message)
{
   MessageBox( NULL, message, "Prima", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
}

Point
apc_sys_get_autoscroll_rate()
{
   Point pt = { 200, 50}; // cannot find actual values .(
   return pt;
}

int
apc_sys_get_cursor_width()
{
   GetSystemMetrics( SM_CXBORDER);
}

Bool
apc_sys_get_insert_mode()
{
   return guts. insertMode;
}

Point
apc_sys_get_scrollbar_metrics()
{
   Point p = {
      GetSystemMetrics( SM_CXHSCROLL),
      GetSystemMetrics( SM_CYVSCROLL)
   };
   return p;
}

void
apc_sys_set_insert_mode( Bool insMode)
{
   guts. insertMode = insMode;
}

Point
apc_sys_get_window_borders( int borderStyle)
{
   Point ret = { 0, 0};
   switch ( borderStyle)
   {
      case bsSizeable:
         ret. x = GetSystemMetrics( SM_CXFRAME);
         ret. y = GetSystemMetrics( SM_CYFRAME);
         break;
      case bsSingle:
         ret. x = GetSystemMetrics( SM_CXBORDER);
         ret. y = GetSystemMetrics( SM_CYBORDER);
         break;
      case bsDialog:
         ret. x = GetSystemMetrics( SM_CXDLGFRAME);
         ret. y = GetSystemMetrics( SM_CYDLGFRAME);
         break;
   }
   return ret;
}

int
apc_sys_get_value( int sysValue)
{
   HKEY hKey;
   DWORD valSize = 256, valType = REG_SZ;
   char buf[ 256] = "";

   switch ( sysValue) {
   case svYMenu          :
       return guts. ncmData. iMenuHeight;
   case svYTitleBar      :
       return guts. ncmData. iCaptionHeight;
   case svMousePresent   :
       return GetSystemMetrics( SM_MOUSEPRESENT);
   case svMouseButtons   :
       return GetSystemMetrics( SM_CMOUSEBUTTONS);
   case svSubmenuDelay   :
       RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey);
       RegQueryValueEx( hKey, "MenuShowDelay", nil, &valType, buf, &valSize);
       return atol( buf);
   case svFullDrag       :
       RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey);
       RegQueryValueEx( hKey, "DragFullWindows", nil, &valType, buf, &valSize);
       return atol( buf);
   default:
      apcErr( errInvParams);
   }
   return 0;
}

PFont
apc_sys_get_msg_font( PFont copyTo)
{
   *copyTo = guts. msgFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}

PFont
apc_sys_get_caption_font( PFont copyTo)
{
   *copyTo = guts. capFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}


// etc
static int ctx_mb2MB[] =
{
   mbError       , MB_ICONHAND,
   mbQuestion    , MB_ICONQUESTION,
   mbInformation , MB_ICONASTERISK,
   mbWarning     , MB_ICONEXCLAMATION,
   endCtx
};


void
apc_beep( int style)
{
   MessageBeep( ctx_remap_def( style, ctx_mb2MB, true, MB_OK));
}

void
apc_beep_tone( int freq, int duration)
{
   Beep( freq, duration);
}

char *
apc_system_action( char * params)
{
   if ( stricmp( params, "wait.before.quit") == 0) { waitBeforeQuit = 1; }
   return 0;
}

void
apc_query_drives_map( char *firstDrive, char *map)
{
   char *m = map;
   int beg;
   DWORD driveMap;
   int i;

   strcpy( map, "");

   beg = toupper( *firstDrive);
   if (( beg < 'A') || ( beg > 'Z') || ( firstDrive[1] != ':'))
      return;

   beg -= 'A';

   if ( !( driveMap = GetLogicalDrives()))
      apiErr;
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

static int ctx_dt2DRIVE[] =
{
   dtUnknown  , 0               ,
   dtNone     , 1               ,
   dtFloppy   , DRIVE_REMOVABLE ,
   dtHDD      , DRIVE_FIXED     ,
   dtNetwork  , DRIVE_REMOTE    ,
   dtCDROM    , DRIVE_CDROM     ,
   dtMemory   , DRIVE_RAMDISK   ,
   endCtx
};

int
apc_query_drive_type( char *drive)
{
   return ctx_remap_def( GetDriveType( drive), ctx_dt2DRIVE, false, dtNone);
}

static char userName[ 1024];

char *
apc_get_user_name()
{
   DWORD maxSize = 1024;
   if ( !GetUserName( userName, &maxSize)) apiErr;
   return userName;
}


void *apc_dlopen(char *path, int mode)
{
   (void) mode;
   return LoadLibrary( path);
}

void *dlsym(void *dll, char *symbol)
{
   return GetProcAddress((HMODULE)dll, symbol);
}

static char dlerror_description[256];

char *dlerror(void)
{
   snprintf( dlerror_description, 256, "dlerror: %08x", GetLastError());
   return dlerror_description;
}

