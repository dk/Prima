#include "win32\win32guts.h"
#include "Window.h"
#include "Application.h"
#include "Menu.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

WinGuts guts;
Bool    loggerDead   = false;
DWORD   rc;
PHash   stylusMan    = nilHandle; // pen & brush manager
PHash   fontMan      = nilHandle; // font manager
PHash   patMan       = nilHandle; // pattern resource manager
PHash   menuMan      = nilHandle; // HMENU manager
PHash   imageMan     = nilHandle; // HBITMAP manager
HPEN    hPenHollow;
HBRUSH  hBrushHollow;
PatResource hPatHollow;
DIBMONOBRUSH bmiHatch = {
   { sizeof( BITMAPINFOHEADER), 8, 8, 1, 1, BI_RGB, 0, 0, 0, 2, 2},
   {{0,0,0,0}, {0,0,0,0}}
};
int     FONTSTRUCSIZE, FONTSTRUCSIZE2;
Handle lastMouseOver = nilHandle;
MusClkRec musClk = {0};

void set_platform_data( HINSTANCE inst, int cmd)
{
   memset( &guts, 0, sizeof( guts));
   guts. cmdShow = cmd;
   guts. instance = inst;
}

extern void start_logger( void);

Bool
window_subsystem_init()
{
   WNDCLASS  wc;
   HDC dc;
   start_logger();

   guts. errorMode = SetErrorMode( SEM_FAILCRITICALERRORS);

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc   = ( WNDPROC) generic_app_handler;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)NULL;
   wc.lpszClassName = "GenericApp";
   RegisterClass( &wc);

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc   = ( WNDPROC) generic_frame_handler;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)NULL;
   wc.lpszClassName = "GenericFrame";
   RegisterClass( &wc);

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc   = ( WNDPROC) generic_view_handler;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = NULL; // LoadCursor( NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)NULL;
   wc.lpszClassName = "Generic";
   RegisterClass( &wc);

   stylusMan = hash_create();
   fontMan   = hash_create();
   patMan    = hash_create();
   menuMan   = hash_create();
   imageMan  = hash_create();
   {
      LOGBRUSH b = { BS_HOLLOW, 0, 0};
      Font f;
      hPenHollow        = CreatePen( PS_NULL, 0, 0);
      hBrushHollow      = CreateBrushIndirect( &b);
      hPatHollow. style = PS_NULL;
      hPatHollow. styleCount = 0;
      hPatHollow. dotsPtr = nil;
      FONTSTRUCSIZE    = (char *)(&(f. name)) - (char *)(&f);
      FONTSTRUCSIZE2   = (char *)(&(f. name)) - (char *)(&f. style);
   }

   dc = dc_alloc();
   guts. displayResolution. x = GetDeviceCaps( dc, LOGPIXELSX);
   guts. displayResolution. y = GetDeviceCaps( dc, LOGPIXELSY);
   {
      LOGFONT lf;

      // GetObject( GetCurrentObject( dc, OBJ_FONT), sizeof( LOGFONT), &lf);
      // font_logfont2font( &lf, &guts. windowFont, &guts. displayResolution);

      memset( &guts. windowFont, 0, sizeof( Font));
      strcpy( guts. windowFont. name, DEFAULT_WIDGET_FONT);
      guts. windowFont. size  = DEFAULT_WIDGET_FONT_SIZE;
      guts. windowFont. width = guts. windowFont. height = C_NUMERIC_UNDEF;
      apc_font_pick( nilHandle, &guts. windowFont, &guts. windowFont);

      guts. ncmData. cbSize = sizeof( NONCLIENTMETRICS);
      SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS),
         ( PVOID) &guts. ncmData, 0);
      font_logfont2font( &guts. ncmData. lfMenuFont, &guts. menuFont, &guts. displayResolution);
      font_logfont2font( &guts. ncmData. lfMessageFont, &guts. msgFont, &guts. displayResolution);
      font_logfont2font( &guts. ncmData. lfCaptionFont, &guts. capFont, &guts. displayResolution);
   }
   create_font_hash();

   guts. displayBMInfo. bmiHeader. biBitCount = 0;
   guts. displayBMInfo. bmiHeader. biSize = sizeof( BITMAPINFO);
   GetDIBits( dc, GetCurrentObject( dc, OBJ_BITMAP), 0, 0, NULL, &guts. displayBMInfo, DIB_PAL_COLORS);

   dc_free();
   guts. insertMode = true;
   guts. iconSizeSmall. x = GetSystemMetrics( SM_CXSMICON);
   guts. iconSizeSmall. y = GetSystemMetrics( SM_CYSMICON);
   guts. iconSizeLarge. x = GetSystemMetrics( SM_CXICON);
   guts. iconSizeLarge. y = GetSystemMetrics( SM_CYICON);
   guts. pointerSize. x   = GetSystemMetrics( SM_CXCURSOR);
   guts. pointerSize. y   = GetSystemMetrics( SM_CYCURSOR);
   list_create( &guts. transp, 8, 8);

   // selecting locale layout, more or less latin-like
   {
      char buf[ KL_NAMELENGTH] = "";
      char * langs[]   = {"0409","0405","040A","040B","040E","040F","0413","0414","041D"};
      HKL current      = GetKeyboardLayout( 0);
      int i, j, size   = GetKeyboardLayoutList( 0, nil);
      HKL * kl         = malloc( sizeof( HKL) * size);

      guts. keyLayout = nil;
      GetKeyboardLayoutList( size, kl);
      for ( i = 0; i < size; i++) {
         ActivateKeyboardLayout( kl[ i], 0);
         GetKeyboardLayoutName( buf);
         for ( j = 0; j < ( sizeof( langs) / sizeof( char*)); j++) {
            if ( strncmp( buf + 4, langs[ j], 4) == 0) {
               guts. keyLayout = kl[ i];
               goto found;
            }
         }
      }
   found:;
      ActivateKeyboardLayout( current, 0);
      free( kl);
   }
   guts. currentKeyState = guts. keyState;
   return true;
}

void
window_subsystem_done()
{
   list_destroy( &guts. transp);
   destroy_font_hash();
   hash_destroy( imageMan, false);
   hash_destroy( menuMan,  false);
   hash_destroy( patMan,   true);
   hash_destroy( fontMan,  true);
   hash_destroy( stylusMan, true);
   DeleteObject( hPenHollow);
   DeleteObject( hBrushHollow);

   DestroyWindow( guts. logger);
   loggerDead          = TRUE;
   guts. logger        = NULL;
   guts. loggerListBox = NULL;
   SetErrorMode( guts. errorMode);
}

void
window_subsystem_cleanup()
{
   while ( guts. appLock > 0) apc_application_unlock( application);
   while ( guts. pointerLock < 0) {
      ShowCursor( 1);
      guts. pointerLock++;
   }
}

static char buf[ 256];
char * err_msg( DWORD errId)
{
   LPVOID lpMsgBuf;
   FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errId,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      ( LPTSTR) &lpMsgBuf, 0, NULL);
   strncpy( buf, lpMsgBuf, 256);
   buf[ 256] = 0;
   LocalFree( lpMsgBuf);
   return buf;
}


static Bool move_back( PWidget self, PWidget child, int * delta)
{
   RECT r;
   int oStage = child-> stage;

   if (( child-> growMode & gfDontCare) || !dsys( child) options. aptClipOwner)
      return false;

   child-> stage = csFrozen;
   GetWindowRect( DHANDLE( child), &r);
   if ( dsys( child) options. aptClipOwner)
      MapWindowPoints( NULL, ( HWND) self-> handle, ( LPPOINT)&r, 2);
   SetWindowPos( DHANDLE( child), 0, r. left, r. top + *delta, 0, 0,
      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
   child-> stage = oStage;

   return false;
}


static Bool
local_wnd( HWND who, HWND client)
{
   PComponent v;
   Handle self;
   if ( who == client)
      return true;
   self = GetWindowLong( client, GWL_USERDATA);
   v = (PComponent) hwnd_to_view( who);
   while (v && ( Handle) v != application)
   {
      if ((Handle)v == self) return true;
      ( Handle) v = v-> owner;
   }
   return false;
}

extern int ctx_kb2VK[];
extern int ctx_kb2VK2[];

static Bool
find_oid( PAbstractMenu menu, PMenuItemReg m, int id)
{
   return m-> down && ( m-> down-> id == id);
}


LRESULT CALLBACK generic_view_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
   LRESULT ret = 0;
   Handle  self   = GetWindowLong( win, GWL_USERDATA);
   PWidget v      = ( PWidget) self;
   UINT    orgMsg = msg;
   Event   ev;
   Bool    hiStage   = false;
   int     orgCmd;

   if ( !self || appDead)
      return DefWindowProc( win, msg, mp1, mp2);

   memset( &ev, 0, sizeof (ev));
   ev. gen. source = self;

   switch ( msg) {
   case WM_NCACTIVATE:
        // if activation or deactivation is concerned with declipped window ( e.g.self),
        // notify its top level frame so that it will have the chance to redraw itself correspondingly
        if ( is_declipped_child( self) && !Widget_is_child( hwnd_to_view(( HWND) mp2), hwnd_top_level( self)))
           SendMessage( DHANDLE( hwnd_top_level( self)), WM_NCACTIVATE, mp1, mp2);
        break;
   case WM_MOUSEACTIVATE:
       // if pointing to non-active frame, but its declipped child is active at the moment,
       // cancel activation - it could produce unwilling focus changes
       if ( sys className == WC_FRAME) {
          Handle x = hwnd_to_view( GetActiveWindow());
          if ( is_declipped_child(x) && Widget_is_child( x, self))
             return MA_NOACTIVATE;
       }
       break;
   case WM_CLOSE:
      if ( sys className != WC_FRAME)
         return 0;
      break;
   case WM_COLORCHANGED:
      ev. cmd = cmColorChanged;
      ev. gen. i = ( int) mp1;
      break;
   case WM_COMMAND:
      if (( HIWORD( mp1) == 0 /* menu source */) && ( mp2 == 0)) {
          if ( LOWORD( mp1) <= MENU_ID_AUTOSTART) {
             HWND active = GetFocus();
             if ( active != nil) SendMessage( active, LOWORD( mp1), 0, 0);
          } else if ( sys lastMenu) {
             PAbstractMenu a = ( PAbstractMenu) sys lastMenu;
             a-> self-> sub_call_id(( Handle) a, LOWORD( mp1) - MENU_ID_AUTOSTART);
          }
      }
      break;
   case WM_CONTEXTMENU:
      ev. cmd       = cmPopup;
      ev. gen. B    = ( GetKeyState( VK_LBUTTON) < 0) | ( GetKeyState( VK_RBUTTON) < 0);
      // mouse event
      ev. gen. P. x = LOWORD( mp2);
      ev. gen. P. y = sys lastSize. y - HIWORD( mp2);
      break;
   case WM_ENABLE:
      ev. cmd = mp1 ? cmEnable : cmDisable;
      hiStage = true;
      break;
   case WM_ERASEBKGND:
      return 1;
#ifdef DEBUG
   case WM_EXTERNAL:
      switch (( int) mp1) {
      case 0:
         return 0x080174;
      case 1:
         if ( apc_clipboard_open()) {
            if ( apc_clipboard_has_format( cfText)) {
               int len;
               char * c = ( char*)apc_clipboard_get_data( cfText, &len);
               eval( c);
               free( c);
            }
            apc_clipboard_close();
         }
         return 1;
      }
      return 0;
#endif
   case WM_FONTCHANGED:
      ev. cmd = cmFontChanged;
      break;
   case WM_FORCEFOCUS:
      if ( mp2)
         ((( PWidget) mp2)-> self)-> set_selected(( Handle) mp2, 1);
      return 0;
   // case WM_SYSCHAR:return 1;
   case WM_SYSKEYUP:
   case WM_SYSKEYDOWN:
      if ( mp2 & ( 1 << 29)) ev. key. mod = kbAlt;
   case WM_KEYDOWN:
   case WM_KEYUP:
      if ( apc_widget_is_responsive( self)) {
          Bool up = ( msg == WM_KEYUP) || ( msg == WM_SYSKEYUP);
          ev. cmd = up ? cmKeyUp : cmKeyDown;
          ev. key. key    = ctx_remap_def( mp1, ctx_kb2VK, false, kbNoKey);
          ev. key. code   = mp1;
          ev. key. repeat = mp2 & 0x000000FF;
          ev. key. mod   |=
             (( GetKeyState( VK_SHIFT)   < 0) ? kbShift : 0) |
             (( GetKeyState( VK_CONTROL) < 0) ? kbCtrl  : 0) |
             (( GetKeyState( VK_MENU)    < 0) ? kbAlt   : 0);
          if ( ev. key. key == kbNoKey) {
             if ( mp1 < ' ') {
                ev. key. code = 0;   // bare mod key
                if ( mp1 != VK_MENU && mp1 != VK_SHIFT && mp1 != VK_CONTROL)
                   return 1;         // other mod key, not handled
             } else { // trying to map key to ascii...
                WORD keys[ 2];
                UINT scan = ( HIWORD( mp2) & 0xFF) | ( up ? 0x80000000 : 0);
                if ( ev. key. mod & kbCtrl) {
                   // non-alphanumeric keys - such as /\|?., etc - with kbCtrl gives weird results
                   BYTE octl  = guts. currentKeyState[ VK_CONTROL];
                   HKL  kl    = guts. keyLayout ? guts. keyLayout : GetKeyboardLayout( 0);
                   guts. currentKeyState[ VK_CONTROL] = 0;
                   if ( ToAsciiEx( mp1, scan, guts. keyState, keys, 0, kl) != 1)
                      keys[ 0] = mp1; // fails to translate key, so mp1 is best match
                   guts. currentKeyState[ VK_CONTROL] = octl;
                } else { // character translation candidate
                   if ( ToAsciiEx( mp1, scan, guts. currentKeyState, keys, 0, GetKeyboardLayout( 0)) != 1)
                     return 1;
                }
                ev. key. code = keys[ 0] & 0xFF;
             }
          } else
             if ( ev. key. key == kbTab && ( ev. key. mod & kbShift))
                ev. key. key = kbShiftTab;
      }
      break;
   case WM_INITMENUPOPUP:
      if ( HIWORD( mp2)) break; // do not use system popup
   case WM_INITMENU:
      {
         PMenuWndData mwd = ( PMenuWndData) hash_fetch( menuMan, &mp1, sizeof( void*));
         PMenuItemReg m;
         sys lastMenu = mwd ? mwd-> menu : nilHandle;
         if ( mwd) {
            m = AbstractMenu_first_that( mwd-> menu, find_oid, (void*)mwd->id, true);
            hiStage    = true;
            ev. cmd    = cmMenu;
            ev. gen. H = mwd-> menu;
            ev. gen. p = m ? m-> variable : "";
         }
         if (( msg == WM_INITMENUPOPUP) && ( m == nilHandle))
            ev. cmd = 0;
      }
      break;
   case WM_KILLFOCUS:
      if (( HWND) mp1 != win) {
        ev. cmd = cmReleaseFocus;
        hiStage = true;
        apt_assign( aptFocused, 0);
        DestroyCaret();
      }
      break;
   case WM_LBUTTONDOWN:
      ev. pos. button = mbLeft;
      goto MB_DOWN;
   case WM_RBUTTONDOWN:
      ev. pos. button = mbRight;
      goto MB_DOWN;
   case WM_MBUTTONDOWN:
      ev. pos. button = mbMiddle;
      goto MB_DOWN;
   case WM_LBUTTONUP:
      ev. pos. button = mbLeft;
      goto MB_UP;
   case WM_RBUTTONUP:
      ev. pos. button = mbRight;
      goto MB_UP;
   case WM_MBUTTONUP:
      ev. pos. button = mbMiddle;
      goto MB_UP;
   case WM_LBUTTONDBLCLK:
      ev. pos. button = mbLeft;
      goto MB_DBLCLK;
   case WM_RBUTTONDBLCLK:
      ev. pos. button = mbRight;
      goto MB_DBLCLK;
   case WM_MBUTTONDBLCLK:
      ev. pos. button = mbMiddle;
      goto MB_DBLCLK;
   case WM_LMOUSECLICK:
      ev. pos. button = mbLeft;
      goto MB_CLICK;
   case WM_RMOUSECLICK:
      ev. pos. button = mbRight;
      goto MB_CLICK;
   case WM_MMOUSECLICK:
      ev. pos. button = mbMiddle;
      goto MB_CLICK;
   case WM_MOUSEMOVE:
      ev. cmd = cmMouseMove;
      if ( self != lastMouseOver) {
         Handle old = lastMouseOver;
         lastMouseOver = self;
         if ( old)
            SendMessage(( HWND)(( PWidget) old)-> handle, WM_MOUSEEXIT, mp1, mp2);
         SendMessage( win, WM_MOUSEENTER, mp1, mp2);
      }
      goto MB_MAIN;
   case WM_MOUSEENTER:
      ev. cmd = cmMouseEnter;
      goto MB_MAIN;
   case WM_MOUSEEXIT:
      ev. cmd = cmMouseLeave;
      goto MB_MAIN;
   MB_DOWN:
      ev. cmd = cmMouseDown;
      goto MB_MAINACT;
   MB_UP:
      ev. cmd = cmMouseUp;
      goto MB_MAINACT;
   MB_DBLCLK:
      ev. pos. dblclk = 1;
   MB_CLICK:
      ev. cmd = cmMouseClick;
      goto MB_MAINACT;
   MB_MAINACT:
      if ( !is_apt( aptEnabled) || !apc_widget_is_responsive( self))
      {
         if ( ev. cmd != cmMouseUp) MessageBeep( MB_OK);
         return 0;
      }
      goto MB_MAIN;
   MB_MAIN:
      if ( ev. cmd == cmMouseDown && !is_apt( aptFirstClick)) {
         Handle x = self;
         while ( dsys(x) className != WC_FRAME && ( x != application)) x = (( PWidget) x)-> owner;
         if ( x != application && !local_wnd( GetActiveWindow(), DHANDLE( x)))
         {
             ev. cmd = 0; // yes, we abandon mousedown but we should force selection:
             if ((( PApplication) application)-> hintUnder == self) v-> self-> set_hint_visible( self, 0);
             if (( v-> options. optSelectable) && ( v-> selectingButtons & ev. pos. button))
                apc_widget_set_focused( self);
         }
      }
      ev. pos. where. x = (short)LOWORD( mp2);
      ev. pos. where. y = sys lastSize. y - (short)HIWORD( mp2) - 1;
      ev. pos. mod      = 0 |
        (( mp1 & MK_CONTROL) ? kbCtrl  : 0) |
        (( mp1 & MK_SHIFT  ) ? kbShift : 0) |
        (( GetKeyState( VK_MENU) < 0) ? kbAlt : 0)
      ;
      break;
   case WM_MENUCHAR:
       {
          int key;
          ev. key. key    = ctx_remap_def( mp1, ctx_kb2VK2, false, kbNoKey);
          ev. key. code   = mp1;
          ev. key. mod   |=
             (( GetKeyState( VK_SHIFT)   < 0) ? kbShift : 0) |
             (( GetKeyState( VK_CONTROL) < 0) ? kbCtrl  : 0) |
             (( GetKeyState( VK_MENU)    < 0) ? kbAlt   : 0);
          if (( ev. key. mod & kbCtrl) && ( ev. key. code <= 'z'))
             ev. key. code += 'A' - 1;
          key = CAbstractMenu-> translate_key( nilHandle, ev. key. code, ev. key. key, ev. key. mod);
          if ( v-> self-> process_accel( self, key))
             return MAKELONG( 0, MNC_CLOSE);
       }
       break;
   case WM_SYNCMOVE:
       {
          RECT r;
          Handle parent = v-> self-> get_parent(( Handle) v);
          Point sz   = CWidget(parent)-> get_size( parent);
          Point pos  = var self-> get_pos( self);
          ev. cmd    = cmMove;
          ev. gen. P = pos;
          if ( pos. x == var pos. x && pos. y == var pos. y) ev. cmd == 0;
       }
       break;
   case WM_MOVE:
      {
          Handle parent = v-> self-> get_parent(( Handle) v);
          Point sz = CWidget(parent)-> get_size( parent);
          ev. cmd = cmMove;
          ev. gen . P. x = ( SHORT) LOWORD( mp2);
          ev. gen . P. y = sz. y - ( int) HIWORD( mp2) - sys yOverride;
          if ( is_apt( aptTransparent)) InvalidateRect( win, nil, false);
      }
      break;
   case WM_NCHITTEST:
      // dlg protect code - protecting from user actions
      if ( !guts. focSysDisabled && ( Application_map_focus( application, self) != self))
         return HTERROR;
      break;
   case WM_PAINT:
		ev. cmd = cmPaint;
      if (
           ( sys className == WC_CUSTOM) &&
           ( var stage == csNormal) &&
           ( list_index_of( &guts. transp, self) >= 0)
         )
         return 0;
      break;
   case WM_QUERYNEWPALETTE:
      return palette_change( self);
   case WM_PALETTECHANGED:
      if (( HWND) mp1 != win)
         palette_change( self);
      break;
   case WM_POSTAL:
      ev. cmd    = cmPost;
      ev. gen. H = ( Handle) mp1;
      ev. gen. p = ( void *) mp2;
      break;
   case WM_PRIMA_CREATE:
      ev. cmd = cmSetup;
      break;
   case WM_SETFOCUS:
      // dlg protect code - general case
      if ( !guts. focSysDisabled && !guts. focSysGranted) {
         Handle hf = Application_map_focus( application, self);
         if ( hf != self) {
            PostMessage( win, WM_FORCEFOCUS, 0, ( LPARAM) hf);
            return 1;
         }
      }
      if (( HWND) mp1 != win) {
         ev. cmd = cmReceiveFocus;
         hiStage = true;
         apt_assign( aptFocused, 1);
         cursor_update( self);
      }
      break;
   case WM_SETVISIBLE:
     if ( list_index_of( &guts. transp, self) < 0) {
        if ( v-> stage <= csNormal) ev. cmd = mp1 ? cmShow : cmHide;
        hiStage = true;
        apt_assign( aptVisible, mp1);
     }
     break;
   case WM_SIZE:
      ev. cmd = cmSize;
      ev. gen. R. left   = sys lastSize. x;
      ev. gen. R. bottom = sys lastSize. y;
      sys lastSize. x    = ev. gen. R. right  = ev. gen . P. x = LOWORD( mp2);
      sys lastSize. y    = ev. gen. R. top    = ev. gen . P. y = HIWORD( mp2);
      if ( ev. gen. R. top != ev. gen. R. bottom)
      {
         int delta = ev. gen. R. top - ev. gen. R. bottom;
         Widget_first_that( self, move_back, &delta);
         if ( is_apt( aptFocused)) cursor_update(( Handle) self);
      }
      break;
   case WM_TIMER:
      {
         int id = mp1 - 1;
         if ( id >= 0 && id < sys timeDefsCount) ev. gen. H = ( Handle) sys timeDefs[ id]. item;
         if ( ev. gen. H) {
             v = ( PWidget)( self = ev. gen. H);
             ev. cmd = cmTimer;
         }
      }
      break;
   case WM_WINDOWPOSCHANGING:
      if ( sys className == WC_CUSTOM) {
         LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
         if (( l-> flags & SWP_NOSIZE) == 0) {
            ev. cmd = cmCalcBounds;
            ev. gen. R. right = l-> cx;
            ev. gen. R. top   = l-> cy;
         }
      }
      break;
   case WM_WINDOWPOSCHANGED:
      {
          LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
          if (( l-> flags & SWP_NOZORDER) == 0)
             ev. cmd = cmZOrderChanged;
          if (( l-> flags & SWP_NOSIZE) == 0) {
             sys yOverride = l-> cy;
             SendMessage( win, WM_SYNCMOVE, 0, 0);
          }
          if ( l-> flags & SWP_HIDEWINDOW) SendMessage( win, WM_SETVISIBLE, 0, 0);
          if ( l-> flags & SWP_SHOWWINDOW) SendMessage( win, WM_SETVISIBLE, 1, 0);
      }
      break;
   }

   if ( hiStage)
      ret = DefWindowProc( win, msg, mp1, mp2);

   orgCmd = ev. cmd;
   if ( ev. cmd) v-> self-> message( self, &ev); else ev. cmd = orgMsg;

   switch ( orgMsg) {
   case WM_DESTROY:
      if ( v-> stage <= csNormal) {
         v-> handle = nilHandle;       // tell apc not to kill this HWND
         Object_destroy(( Handle) v);
      }
      break;
   case WM_PAINT:
      return 0;
   case WM_LBUTTONDBLCLK: case WM_LBUTTONUP: case WM_LBUTTONDOWN:
   case WM_MBUTTONDBLCLK: case WM_MBUTTONUP: case WM_MBUTTONDOWN:
   case WM_RBUTTONDBLCLK: case WM_RBUTTONUP: case WM_RBUTTONDOWN:
   case WM_KEYDOWN:       case WM_KEYUP:
       if ( ev. cmd == 0)
       {
          return ( LRESULT)1;
       }
       break;
   case WM_SYSKEYDOWN:
   case WM_SYSKEYUP:
       ev. cmd = 1; // forcing DefWindowProc to be called
       break;
   case WM_MOUSEMOVE:
      if ( is_apt( aptEnabled)) SetCursor( sys pointer);
      break;
   case WM_WINDOWPOSCHANGING:
       if ( sys className == WC_CUSTOM) {
          LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
          if (( l-> flags & SWP_NOSIZE) == 0) {
             int dy = l-> cy - ev. gen. R. top;
             l-> cx = ev. gen. R. right;
             l-> cy = ev. gen. R. top;
             l-> y += dy;
          }
          return false;
       }
       break;
   }

   if ( ev. cmd && !hiStage)
      ret = DefWindowProc( win, msg, mp1, mp2);

   return ret;
}

LRESULT CALLBACK generic_frame_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
   LRESULT ret = 0;
   Handle  self   = GetWindowLong( win, GWL_USERDATA);
   PWidget   v      = ( PWidget) self;
   UINT    orgMsg = msg;
   Event   ev;
   Bool    hiStage   = false;
   RECT    rectum;
   int     orgCmd;

   if ( !self)
      return DefWindowProc( win, msg, mp1, mp2);

   memset( &ev, 0, sizeof (ev));
   ev. gen. source = self;

   switch ( msg) {
   case WM_ACTIVATE:
       // dlg protect code - protecting from window activation
       if ( LOWORD( mp1) && !guts. focSysDisabled) {
          Handle hf = Application_map_focus( application, self);
          if ( hf != self) {
             guts. focSysDisabled = 1;
             Application_popup_modal( application);
             PostMessage( win, msg, 0, 0);
             guts. focSysDisabled = 0;
             return 1;
          }
       }
       ev. cmd = ( LOWORD( mp1) != WA_INACTIVE) ? cmActivate : cmDeactivate;
       hiStage = true;
       break;
   case WM_CLOSE:
       ev. cmd = cmClose;
       break;
   case WM_COMMAND:
   // case WM_MENUSELECT:
   case WM_INITMENUPOPUP:
   case WM_INITMENU:
   case WM_MENUCHAR:
   case WM_KEYDOWN:
   case WM_KEYUP:
   case WM_SETVISIBLE:
   case WM_ENABLE:
   case WM_FORCEFOCUS:
   case WM_QUERYNEWPALETTE:
      return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
   case WM_PALETTECHANGED:
      if (( HWND) mp1 == win) return 0;
      return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
   case WM_SYSKEYDOWN:
   case WM_SYSKEYUP:
       if ( generic_view_handler(( HWND) v-> handle, msg, mp1, mp2) == 1)
          return 1;
       else
          hiStage = true;
       break;
   case WM_DLGENTERMODAL:
       ev. cmd = mp1 ? cmExecute : cmEndModal;
       break;
   case WM_ERASEBKGND:
       return 1;
   case WM_NCACTIVATE:
      if (( mp1 == 0) && ( mp2 != 0)) {
         Handle x = hwnd_to_view(( HWND) mp2);
         if ( is_declipped_child( x) && Widget_is_child( x, self))
            return 1;
      }
      // dlg protect code - protecting from window activation
      if ( mp1 && !guts. focSysDisabled) {
         Handle hf = Application_map_focus( application, self);
         if ( hf != self) {
            guts. focSysDisabled = 1;
            Application_popup_modal( application);
            PostMessage( win, msg, 0, 0);
            guts. focSysDisabled = 0;
            return 1;
         }
      }
      break;
   case WM_NCHITTEST:
      // dlg protect code - protecting from user actions
      if ( !guts. focSysDisabled && ( Application_map_focus( application, self) != self))
         return HTERROR;
      break;
   case WM_SETFOCUS:
       // dlg protect code - general case
       if ( !guts. focSysDisabled && !guts. focSysGranted) {
          Handle hf = Application_map_focus( application, self);
          if ( hf != self) {
             PostMessage( win, WM_FORCEFOCUS, 0, ( LPARAM) hf);
             return 1;
          }
       }

       // This code is about to protect frame events when set_selected would
       // grant SetFocus() call to another frame.
       {
          Handle x  = var self-> get_selectee( self);
          Handle w  = x;
          Bool hasCO = false;
          while ( w && w != self) {
             if ( !dsys( w) options. aptClipOwner) {
                hasCO = true;
                break;
             }
             w = (( PWidget) w)-> owner;
          }
          if ( !hasCO)
             var self-> set_selected( self, true);
          // else we do not select any widget, but still have a chance to resize frame :)
       }
       break;
   case WM_SIZE:
       {
          RECT r;
          int state;
          Bool doWSChange = false;
          // SetWindowPos(( HWND) var handle, 0, 0, 0, LOWORD( mp2), HIWORD( mp2), SWP_NOZORDER);
          if (( int) mp1 == SIZE_RESTORED) {
             state = wsNormal;
             if ( sys s. window. state != state) doWSChange = true;
          } else if (( int) mp1 == SIZE_MAXIMIZED) {
             state = wsMaximized;
             doWSChange = true;
          } else if (( int) mp1 == SIZE_MINIMIZED) {
             state = wsMinimized;
             doWSChange = true;
          }
          if ( doWSChange) {
             ev. gen. i = sys s. window. state = state;
             ev. cmd = cmWindowState;
          }
       }
       break;
   case WM_SYNCMOVE:
       {
          RECT r;
          Handle parent = v-> self-> get_parent(( Handle) v);
          Point sz   = CWidget(parent)-> get_size( parent);
          Point pos  = var self-> get_pos( self);
          ev. cmd    = cmMove;
          ev. gen. P = pos;
          if ( pos. x == var pos. x && pos. y == var pos. y) ev. cmd == 0;
       }
       break;
   case WM_MOVE:
       {
          Handle parent = v-> self-> get_parent(( Handle) v);
          Point sz = CWidget(parent)-> get_size( parent);
          ev. cmd = cmMove;
          ev. gen . P. x = ( SHORT) LOWORD( mp2);
          ev. gen . P. y = sz. y - ( SHORT) HIWORD( mp2) - sys yOverride;
       }
       break;
   case WM_SYSCHAR:return 1;
   case WM_TIMER:
      if ( mp1 == TID_USERMAX)
      // application local timer
      {
        POINT p;
        HWND wp;
        if ( lastMouseOver && !GetCapture())
        {
           HWND desktop = HWND_DESKTOP;
           GetCursorPos( &p);
           wp = WindowFromPoint( p);
           if ( wp) {
              POINT xp = p;
              MapWindowPoints( desktop, wp, &xp, 1);
              wp = ChildWindowFromPoint( wp, xp);
           } else
              wp = ChildWindowFromPoint( wp, p);
           if ( wp != ( HWND)(( PWidget) lastMouseOver)-> handle)
           {
              HWND old = ( HWND)(( PWidget) lastMouseOver)-> handle;
              Handle s;
              lastMouseOver = nilHandle;
              SendMessage( old, WM_MOUSEEXIT, 0, 0);
              s = hwnd_to_view( wp);
              if ( s && ( HWND)(( PWidget) s)-> handle == wp)
              {
                 MapWindowPoints( desktop, wp, &p, 1);
                 SendMessage( wp, WM_MOUSEENTER, 0, MAKELPARAM( p. x, p. y));
                 lastMouseOver = s;
              }
           }
        }
        return 0;
      }
      break;
   case WM_GETMINMAXINFO:
      {
         LPMINMAXINFO l = ( LPMINMAXINFO) mp2;
         Point min = var self-> get_size_min( self);
         Point max = var self-> get_size_max( self);
         l-> ptMinTrackSize. x = min. x;
         l-> ptMinTrackSize. y = min. y;
         l-> ptMaxTrackSize. x = max. x;
         l-> ptMaxTrackSize. y = max. y;
      }
      break;
   case WM_WINDOWPOSCHANGED:
       {
           LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
           if (( l-> flags & SWP_NOZORDER) == 0)
              ev. cmd = cmZOrderChanged;
           if (( l-> flags & SWP_NOSIZE) == 0) {
              RECT r;
              GetClientRect( win, &r);
              sys yOverride = r. bottom + r. top;
              SendMessage( win, WM_SYNCMOVE, 0, 0);
           }
           if ( l-> flags & SWP_HIDEWINDOW) SendMessage( win, WM_SETVISIBLE, 0, 0);
           if ( l-> flags & SWP_SHOWWINDOW) SendMessage( win, WM_SETVISIBLE, 1, 0);
           {
              RECT r;
              GetClientRect( win, &r);
              SetWindowPos(( HWND) var handle, 0, 0, 0, r. right, r.bottom, SWP_NOZORDER);
           }
       }
       break;
   }

   if ( hiStage)
      ret = DefWindowProc( win, msg, mp1, mp2);

   orgCmd = ev. cmd;
   if ( ev. cmd) v-> self-> message( self, &ev); else ev. cmd = orgMsg;

   switch ( orgMsg) {
   case WM_CLOSE:
      if ( ev. cmd) {
         if ( sys className == WC_FRAME && PWindow(self)->modal) {
           Window_cancel( self);
           return 0;
         } else
           Object_destroy(( Handle) v);
         break;
      } else
         return 0;
   }

   if ( ev. cmd && !hiStage)
      ret = DefWindowProc( win, msg, mp1, mp2);
   return ret;
}

static Bool kill_img_cache( Handle self, int keyLen, void * key, void * killDBM)
{
   if ( is_apt( aptDeviceBitmap)) {
      if ( killDBM) dbm_recreate( self);
   } else
      image_destroy_cache( self);
   return false;
}


LRESULT CALLBACK generic_app_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
   switch ( msg) {
      case WM_DISPLAYCHANGE:
         {
            HDC dc = dc_alloc();
            int oldBPP = guts. displayBMInfo. bmiHeader. biBitCount;
            guts. displayBMInfo. bmiHeader. biBitCount = 0;
            guts. displayBMInfo. bmiHeader. biSize = sizeof( BITMAPINFO);
            GetDIBits( dc, GetCurrentObject( dc, OBJ_BITMAP), 0, 0, NULL, &guts. displayBMInfo, DIB_PAL_COLORS);
            dsys( application) lastSize. x = LOWORD( mp2);
            dsys( application) lastSize. y = HIWORD( mp2);
            if ( oldBPP != guts. displayBMInfo. bmiHeader. biBitCount)
               hash_first_that( imageMan, kill_img_cache, (void*)1, nil, nil);
            dc_free();
         }
         break;
      case WM_FONTCHANGE:
         destroy_font_hash();
         break;
      case WM_COMPACTING:
         destroy_font_hash();
         hash_first_that( imageMan, kill_img_cache, nil, nil, nil);
         break;
   }
   return generic_frame_handler( win, msg, mp1, mp2);
}
