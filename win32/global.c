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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#include "win32\win32guts.h"
#include "Window.h"
#include "Application.h"
#include "Menu.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

// #define FONT_CHECK

WinGuts guts;
DWORD   rc;
PHash   stylusMan    = nil; // pen & brush manager
PHash   fontMan      = nil; // font manager
PHash   patMan       = nil; // pattern resource manager
PHash   menuMan      = nil; // HMENU manager
PHash   imageMan     = nil; // HBITMAP manager
PHash   regnodeMan   = nil; // cache for apc_widget_user_profile
HPEN    hPenHollow;
HBRUSH  hBrushHollow;
PatResource hPatHollow;
DIBMONOBRUSH bmiHatch = {
   { sizeof( BITMAPINFOHEADER), 8, 8, 1, 1, BI_RGB, 0, 0, 0, 2, 2},
   {{0,0,0,0}, {0,0,0,0}}
};
int     FONTSTRUCSIZE;
Handle lastMouseOver = nilHandle;
MusClkRec musClk = {0};
char * keyLayouts[]   = {  "0409", "0403", "0405", "0406", "0407",
      "0807","0809","080A","080C","0C0C","100C","0810","0814","0816",
      "040A","040B","040C","040E","040F","0410","0413","0414","0415","0416",
      "0417","0418","041A","041D"
};
WCHAR lastDeadKey = 0;


BOOL APIENTRY
DllMain( HINSTANCE hInstance, DWORD reason, LPVOID reserved)
{
    if ( reason == DLL_PROCESS_ATTACH) {
       memset( &guts, 0, sizeof( guts));
       guts. instance = hInstance;
       guts. cmdShow  = SW_SHOWDEFAULT;
    }
    return TRUE;
}

Bool
window_subsystem_init( char * error_buf)
{
   WNDCLASSW wc;
   HDC dc;
   HBITMAP hbm;

   guts. version  = GetVersion();
   if ( IS_WIN95) {
      OSVERSIONINFO os = { sizeof( OSVERSIONINFO)};
      GetVersionEx( &os);
      if ((os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
         ((os.dwMajorVersion > 4) ||
         ((os.dwMajorVersion == 4) && (os.dwMinorVersion > 0))))
         guts. is98 = 1;
   }
   guts. mainThreadId = GetCurrentThreadId();
   guts. errorMode = SetErrorMode( SEM_FAILCRITICALERRORS);
   guts. desktopWindow = GetDesktopWindow();

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc   = ( WNDPROC) generic_app_handler;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)NULL;
   wc.lpszClassName = L"GenericApp";
   RegisterClassW( &wc);

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc   = ( WNDPROC) generic_frame_handler;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)NULL;
   wc.lpszClassName = L"GenericFrame";
   RegisterClassW( &wc);

   memset( &wc, 0, sizeof( wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc   = ( WNDPROC) generic_view_handler;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = guts. instance;
   wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
   wc.hCursor       = NULL; // LoadCursor( NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)NULL;
   wc.lpszClassName = L"Generic";
   RegisterClassW( &wc);

   stylusMan  = hash_create();
   fontMan    = hash_create();
   patMan     = hash_create();
   menuMan    = hash_create();
   imageMan   = hash_create();
   regnodeMan = hash_create();
   create_font_hash();
   {
      LOGBRUSH b = { BS_HOLLOW, 0, 0};
      Font f;
      hPenHollow        = CreatePen( PS_NULL, 0, 0);
      hBrushHollow      = CreateBrushIndirect( &b);
      hPatHollow. dotsCount = 0;
      hPatHollow. dotsPtr   = nil;
      FONTSTRUCSIZE    = (char *)(&(f. name)) - (char *)(&f);
   }

   if (!( dc = dc_alloc())) return false; 
   guts. displayResolution. x = GetDeviceCaps( dc, LOGPIXELSX);
   guts. displayResolution. y = GetDeviceCaps( dc, LOGPIXELSY);
   {
      LOGFONT lf;
      HFONT   sfont;

      // getting most common font name
      memset( &lf, 0, sizeof( lf));
      lf. lfCharSet        = OEM_CHARSET;
      lf. lfOutPrecision   = OUT_DEFAULT_PRECIS;
      lf. lfClipPrecision  = CLIP_DEFAULT_PRECIS;
      lf. lfQuality        = PROOF_QUALITY;
      lf. lfPitchAndFamily = DEFAULT_PITCH;
      sfont = SelectObject( dc, CreateFontIndirect( &lf));
      GetTextFace( dc, 256, guts. defaultSystemFont);

      // getting common fixed font name
      lf. lfHeight = 320;
      lf. lfPitchAndFamily = FIXED_PITCH;
      DeleteObject( SelectObject( dc, CreateFontIndirect( &lf)));
      GetTextFace( dc, 256, guts. defaultFixedFont);

      // getting common variable font name
      lf. lfPitchAndFamily = VARIABLE_PITCH;
      DeleteObject( SelectObject( dc, CreateFontIndirect( &lf)));
      GetTextFace( dc, 256, guts. defaultVariableFont);
      DeleteObject( SelectObject( dc, sfont));

      // getting system font presets
      memset( &guts. windowFont, 0, sizeof( Font));
      strcpy( guts. windowFont. name, DEFAULT_WIDGET_FONT);
      guts. windowFont. size  = DEFAULT_WIDGET_FONT_SIZE;
      guts. windowFont. width = guts. windowFont. height = C_NUMERIC_UNDEF;
#ifdef FONT_CHECK
      guts. windowFont. size = 12;
#endif
      apc_font_pick( nilHandle, &guts. windowFont, &guts. windowFont);

      guts. ncmData. cbSize = sizeof( NONCLIENTMETRICS);
      SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS),
         ( PVOID) &guts. ncmData, 0);
      font_logfont2font( &guts. ncmData. lfMenuFont, &guts. menuFont, &guts. displayResolution);
      font_logfont2font( &guts. ncmData. lfMessageFont, &guts. msgFont, &guts. displayResolution);
      font_logfont2font( &guts. ncmData. lfCaptionFont, &guts. capFont, &guts. displayResolution);
   }

   memset( &guts. displayBMInfo, 0, sizeof( guts. displayBMInfo));
   guts. displayBMInfo. bmiHeader. biSize = sizeof( BITMAPINFO);
   if ( !( hbm = GetCurrentObject( dc, OBJ_BITMAP))) {
      apiErr;
      dc_free();
      return false;
   }

   if ( !GetDIBits( dc, hbm, 0, 0, NULL, &guts. displayBMInfo, DIB_PAL_COLORS)) {
      guts. displayBMInfo. bmiHeader. biBitCount = GetDeviceCaps( dc, BITSPIXEL);
      guts. displayBMInfo. bmiHeader. biPlanes   = GetDeviceCaps( dc, PLANES);
   }

   dc_free();
   guts. insertMode = true;
   guts. iconSizeSmall. x = GetSystemMetrics( SM_CXSMICON);
   guts. iconSizeSmall. y = GetSystemMetrics( SM_CYSMICON);
   guts. iconSizeLarge. x = GetSystemMetrics( SM_CXICON);
   guts. iconSizeLarge. y = GetSystemMetrics( SM_CYICON);
   guts. pointerSize. x   = GetSystemMetrics( SM_CXCURSOR);
   guts. pointerSize. y   = GetSystemMetrics( SM_CYCURSOR);
   list_create( &guts. transp, 8, 8);
   list_create( &guts. files, 8, 8);
   list_create( &guts. sockets, 8, 8);
   list_create( &guts. eventHooks, 1, 1);

   // selecting locale layout, more or less latin-like

   {
      char buf[ KL_NAMELENGTH * 2] = "";
      HKL current      = GetKeyboardLayout( 0);
      int i, j, size   = GetKeyboardLayoutList( 0, nil);
      HKL * kl         = ( HKL *) malloc( sizeof( HKL) * size);

      guts. keyLayout = nil;
      if ( !GetKeyboardLayoutName( buf)) apiErr;
      for ( j = 0; j < ( sizeof( keyLayouts) / sizeof( char*)); j++) {
         if ( strncmp( buf + 4, keyLayouts[ j], 4) == 0) {
            guts. keyLayout = current;
            goto found_1;
         }
      }

      if ( kl) {
         GetKeyboardLayoutList( size, kl);
         for ( i = 0; i < size; i++) {
            ActivateKeyboardLayout( kl[ i], 0);
            if ( !GetKeyboardLayoutName( buf)) apiErr;
            for ( j = 0; j < ( sizeof( keyLayouts) / sizeof( char*)); j++) {
               if ( strncmp( buf + 4, keyLayouts[ j], 4) == 0) {
                  guts. keyLayout = kl[ i];
                  goto found_2;
               }
            }
         }
      found_2:;
         ActivateKeyboardLayout( current, 0);
      }
   found_1:;
      free( kl);
   }
   guts. currentKeyState = guts. keyState;
   memset( guts. emptyKeyState, 0, sizeof( guts. emptyKeyState));
   guts. smDblClk. x = GetSystemMetrics( SM_CXDOUBLECLK);
   guts. smDblClk. y = GetSystemMetrics( SM_CYDOUBLECLK);

   return true;
}

Bool
window_subsystem_get_options( int * argc, char *** argv)
{
   *argc = 0;
   return true;
}

Bool
window_subsystem_set_option( char * option, char * value)
{
   return false;
}

void
window_subsystem_done()
{
   list_destroy( &guts. files);

   if ( guts. socketMutex) {
      // appDead must be TRUE for this moment!
      appDead = true;
      CloseHandle( guts. socketMutex);
   }

   list_destroy( &guts. eventHooks);
   list_destroy( &guts. sockets);
   list_destroy( &guts. transp);
   destroy_font_hash();

   font_clean();
   stylus_clean();
   hash_destroy( imageMan,   false);
   hash_destroy( menuMan,    false);
   hash_destroy( patMan,     true);
   hash_destroy( fontMan,    true);
   hash_destroy( stylusMan,  true);
   hash_destroy( regnodeMan, false);
   DeleteObject( hPenHollow);
   DeleteObject( hBrushHollow);
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

Bool
apc_register_hook( int hookType, void * hookProc)
{
   if ( hookType != HOOK_EVENT_LOOP) return false;
   list_add( &guts. eventHooks, ( Handle) hookProc);
   return true;
}   

Bool
apc_deregister_hook( int hookType, void * hookProc)
{
   if ( hookType != HOOK_EVENT_LOOP) return false;
   list_delete( &guts. eventHooks, ( Handle) hookProc); 
   return true;
}   

Bool
apc_register_event( void * sysMessage)
{
   UINT i;
   for ( i = 0; i < WM_LAST_USER_MESSAGE - WM_FIRST_USER_MESSAGE; i++) {
      if (( guts. msgMask[ i >> 3] & (1 << (i & 7))) == 0) {
         guts. msgMask[ i >> 3] |= 1 << (i & 7);
         *(( UINT*) sysMessage) = WM_FIRST_USER_MESSAGE + i; 
         return true; 
      }   
   }   
   return false;
}   

Bool
apc_deregister_event( void * sysMessage)
{
   UINT i = *((UINT*) sysMessage);
   if (( guts. msgMask[ i >> 3] & (1 << (i & 7))) == 0) 
      return false;
   guts. msgMask[ i >> 3] &= ~(1 << (i & 7));
   return true;
}   


static char err_buf[ 256] = "";
char * err_msg( DWORD errId, char * buffer)
{
   LPVOID lpMsgBuf;
   int len;
   if ( buffer == nil) buffer = err_buf;
   FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errId,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      ( LPTSTR) &lpMsgBuf, 0, NULL);
   if ( lpMsgBuf)
      strncpy( buffer, ( const char *) lpMsgBuf, 256);
   else
      buffer[0] = 0;
   buffer[ 255] = 0;
   LocalFree( lpMsgBuf);

   /* chomp! */
   len = strlen(buffer);
   while ( len > 0) {
      len--;
      if ( buffer[len] != '\xD' && buffer[len] != '\xA' && buffer[len] != '.')
         break;
      buffer[len] = 0;
   }

   return buffer;
}

char *
apc_last_error(void)
{
   return err_buf;
}

static Bool move_back( PWidget self, PWidget child, int * delta)
{
   RECT r;
   int oStage = child-> stage;

   if ( !dsys( child) options. aptClipOwner) return false;

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
   self = GetWindowLongPtr( client, GWLP_USERDATA);
   v = (PComponent) hwnd_to_view( who);
   while (v && ( Handle) v != application)
   {
      if ( (Handle)v == self) return true;
      v = ( PComponent) ( v-> owner);
   }
   return false;
}

extern Handle ctx_kb2VK[];
extern Handle ctx_kb2VK2[];
extern Handle ctx_kb2VK3[];

static Bool
find_oid( PAbstractMenu menu, PMenuItemReg m, int id)
{
   return m-> down && ( m-> down-> id == id);
}

Handle ctx_deadkeys[] = {
   0x5E, 0x302, // Circumflex accent
   0x60, 0x300, // Grave accent
   0xA8, 0x308, // Diaeresis
   0xB4, 0x301, // Acute accent
   0xB8, 0x327, // Cedilla
   endCtx
};

static void
zorder_sync( Handle self, HWND me, LPWINDOWPOS lp)
{
   if ( lp-> hwndInsertAfter == HWND_TOP ||
        lp-> hwndInsertAfter == HWND_NOTOPMOST ||
        lp-> hwndInsertAfter == HWND_TOPMOST) {
       me = GetNextWindow( me, GW_HWNDPREV);
       if ( me)
          PostMessage( me, WM_ZORDERSYNC, 0, 0);
   } else if ( lp-> hwndInsertAfter == HWND_BOTTOM) {
      me = GetNextWindow( me, GW_HWNDNEXT);
      if ( me)
         PostMessage( me, WM_ZORDERSYNC, 0, 0);
   }
}


LRESULT CALLBACK generic_view_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
   LRESULT ret = 0;
   Handle  self   = GetWindowLongPtr( win, GWLP_USERDATA);
   PWidget v      = ( PWidget) self;
   UINT    orgMsg = msg;
   Event   ev;
   Bool    hiStage   = false;
   int     i, orgCmd;
   Bool    message_result = true;

   if ( !self || appDead)
      return DefWindowProcW( win, msg, mp1, mp2);

   for ( i = 0; i < guts. eventHooks. count; i++) {
      if ((( PrimaHookProc *)( guts. eventHooks. items[i]))((void*) &msg))
         return 0;
   }
   
   memset( &ev, 0, sizeof (ev));
   ev. gen. source = self;

   switch ( msg) {
   case WM_NCACTIVATE:
        // if activation or deactivation is concerned with declipped window ( e.g.self),
        // notify its top level frame so that it will have the chance to redraw itself correspondingly
        if ( is_declipped_child( self) && !Widget_is_child( hwnd_to_view(( HWND) mp2), hwnd_top_level( self))) {
           Handle x = hwnd_top_level( self);
           if ( x) SendMessage( DHANDLE( x), WM_NCACTIVATE, mp1, mp2);
        }   
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
   case WM_COMMAND:
      if (( HIWORD( mp1) == 0 /* menu source */) && ( mp2 == 0)) {
          if ( LOWORD( mp1) <= MENU_ID_AUTOSTART) {
             HWND active = GetFocus();
             if ( active != nil) SendMessage( active, LOWORD( mp1), 0, 0);
          } else if ( sys lastMenu) {
             PAbstractMenu a = ( PAbstractMenu) sys lastMenu;
             if ( a-> stage <= csNormal)
                a-> self-> sub_call_id(( Handle) a, LOWORD( mp1) - MENU_ID_AUTOSTART);
          }
      }
      break;
   case WM_CONTEXTMENU:
      {
         POINT a;
	 a. x = ( short)LOWORD( mp2);
	 a. y = ( short)HIWORD( mp2);
         ev. cmd       = cmPopup;
         // mouse event
         ev. gen. B    = ( GetKeyState( VK_LBUTTON) < 0) | ( GetKeyState( VK_RBUTTON) < 0);
         if ( !ev. gen. B && GetSystemMetrics( SM_MOUSEPRESENT))
            GetCursorPos(( POINT*) &a);

         MapWindowPoints( NULL, win, &a, 1);
         ev. gen. P. x = a. x;
         ev. gen. P. y = sys lastSize. y - a. y - 1;
      }
      break;
   case WM_ENABLE:
      ev. cmd = mp1 ? cmEnable : cmDisable;
      hiStage = true;
      break;
   case WM_ERASEBKGND:
      return 1;
   case WM_FORCEFOCUS:
      if ( mp2)
         ((( PWidget) mp2)-> self)-> set_selected(( Handle) mp2, 1);
      return 0;
   case WM_HASMATE:
      *(( Handle*) mp2) = self;
      return HASMATE_MAGIC;
   case WM_IME_CHAR:
      if ( apc_widget_is_responsive( self)) {
         ev. cmd = cmKeyDown;
         ev. key. mod  = kmUnicode;
         ev. key. key  = kbNoKey;
         ev. key. code = mp1;
      }
      break;
   case WM_SYSKEYDOWN:
   case WM_SYSKEYUP:
      if ( mp2 & ( 1 << 29)) ev. key. mod = kmAlt;
   case WM_KEYDOWN:
   case WM_KEYUP:
      if ( apc_widget_is_responsive( self)) {
          BYTE * keyState;
          Bool up = ( msg == WM_KEYUP) || ( msg == WM_SYSKEYUP);
          Bool extended = mp2 & ( 1 << 24);
          UINT scan = ( HIWORD( mp2) & 0xFF) | ( up ? 0x80000000 : 0);
          int deadPollCount = 0;
          HKL kl = GetKeyboardLayout(0);

          // basic assignments
          ev. cmd = up ? cmKeyUp : cmKeyDown;
          ev. key. key    = ctx_remap_def( mp1, ctx_kb2VK, false, kbNoKey);
          ev. key. code   = mp1;
          ev. key. repeat = mp2 & 0x000000FF;

          // VK validations
          if ( extended) {
             int ks = ev. key. key;
             ev. key. key = ctx_remap_def( ks, ctx_kb2VK3, true, ks);
             if ( ev. key. key != ks)
                extended = false; // avoid (Ctrl|Alt)R+KeyPad combinations
          } else if ( mp1 >= VK_NUMPAD0 && mp1 <= VK_DIVIDE)
             extended = true; // include numpads

          ev. key. mod   = 0 |
             ( extended ? kmKeyPad : 0) |
             (( GetKeyState( VK_SHIFT)   < 0) ? kmShift : 0) |
             (( GetKeyState( VK_CONTROL) < 0) ? kmCtrl  : 0) |
             (( GetKeyState( VK_MENU)    < 0) ? kmAlt   : 0);

          keyState = guts. keyState;
AGAIN:             
          if ( PApplication(application)-> wantUnicodeInput) {
             WCHAR keys[ 2];
             // unicode mapping
             switch ( ToUnicodeEx( mp1, scan, keyState, keys, 2, 0, kl)) {
             case 1: // char
	     	if ( lastDeadKey ) {
 		   WCHAR wcBuffer[3];
 		   WCHAR out[3];
 		   wcBuffer[0] = keys[0];
 		   wcBuffer[1] = lastDeadKey;
 		   wcBuffer[2] = '\0';
 		   if ( FoldStringW(MAP_PRECOMPOSED, (LPWSTR) wcBuffer, 3, (LPWSTR) out, 3) )
 		      keys[0] = out[0];
		}
                if ( !deadPollCount && ( GetKeyState( VK_MENU) < 0) && ( GetKeyState( VK_SHIFT) >= 0)) {
                   WCHAR keys2[2];
                   if (( ToUnicodeEx( mp1, scan, guts. emptyKeyState, keys2, 2, 0, kl) == 1) &&
                       ( keys2[0] != keys[0])) {
                       /* example - (AltGr+2) == '@' on danish keyboard.
                          this hack is to tell whether the key without mods
                          will give same character code ...  */
                       ev. key. mod &= ~(kmAlt|kmCtrl|kmShift);
                   }
                }   
                if (!up) lastDeadKey = 0;
                break;
             case 2: { // dead key
                   lastDeadKey = ctx_remap_def( keys[0], ctx_deadkeys, true, keys[0]);
                   keys[ 0] = 0;
               	   ev. key. mod |= kmDeadKey;
                }
                break;
             case 0: // virtual key
                if ( deadPollCount == 0) {
                /* can't have character code - maybe fish out without mods? */
                   keyState = guts. emptyKeyState;
                   deadPollCount = 1;
                   goto AGAIN;
                } else {
                /* same meaning without mods, no code anyway */
                   keys[ 0] = 0;
                }
                if (!up) lastDeadKey = 0;
                break;
             default:
               	ev. key. mod |= kmDeadKey;
                if (!up) lastDeadKey = 0;
             }
             ev. key. code = keys[ 0];
	     ev. key. mod |= kmUnicode;
          } else {
             BYTE keys[ 4];
             switch ( ToAsciiEx( mp1, scan, keyState, (LPWORD) keys, 0, kl)) {
             case 1: // char
	     	if ( lastDeadKey ) {
 		   BYTE cBuffer[3];
 		   BYTE out[3];
 		   cBuffer[0] = keys[0];
 		   cBuffer[1] = lastDeadKey;
 		   cBuffer[2] = '\0';
 		   if ( FoldStringA(MAP_PRECOMPOSED, (LPSTR) cBuffer, 3, (LPSTR) out, 3) )
 		      keys[0] = out[0];
		}
                if ( !deadPollCount && ( GetKeyState( VK_MENU) < 0) && ( GetKeyState( VK_SHIFT) >= 0)) {
                   BYTE keys2[4];
                   if (( ToAsciiEx( mp1, scan, guts. emptyKeyState, (LPWORD) keys2, 0, kl) == 1) &&
                       ( keys2[0] != keys[0])) {
                       /* example - (AltGr+2) == '@' on danish keyboard.
                          this hack is to tell whether the key without mods
                          will give same character code ...  */
                       ev. key. mod &= ~(kmAlt|kmCtrl|kmShift);
                   }
                }   
                break;
             case 2: { // dead key
                   lastDeadKey = keys[0];
                   keys[ 0] = 0;
               	   ev. key. mod |= kmDeadKey;
                }
                break;
             case 0: // virtual key
                if ( deadPollCount == 0) {
                /* can't have character code - maybe fish out without mods? */
                   keyState = guts. emptyKeyState;
                   deadPollCount = 1;
                   goto AGAIN;
                } else {
                /* same meaning without mods, no code anyway */
                   keys[ 0] = 0;
                }   
                if (!up) lastDeadKey = 0;
                break;
             default:
                ev. key. mod |= kmDeadKey;
                if (!up) lastDeadKey = 0;
             }
             ev. key. code = keys[ 0];
          }

          // simulated key codes
          if ( ev. key. key == kbTab && ( ev. key. mod & kmShift))
             ev. key. key = kbBackTab;
          
          if ( ev. key. code >= 'A' && ev. key. code <= 'z' && ev. key. mod & kmCtrl) {
             ev. key. code = toupper(ev. key. code & 0xFF) - '@';
             if (!( ev. key. mod & kmShift)) ev. key. code = tolower( ev. key. code);
          }
      }
      break;
   case WM_INITMENUPOPUP:
      if ( HIWORD( mp2)) break; // do not use system popup
   case WM_INITMENU:
      {
         PMenuWndData mwd = ( PMenuWndData) hash_fetch( menuMan, &mp1, sizeof( void*));
         PMenuItemReg m = nil;
         sys lastMenu = mwd ? mwd-> menu : nilHandle;
         if ( mwd && mwd-> menu && ( PAbstractMenu(mwd-> menu)->stage <= csNormal)) {
            m = ( PMenuItemReg) AbstractMenu_first_that( mwd-> menu, find_oid, INT2PTR(void*,mwd->id), true);
            hiStage    = true;
            ev. cmd    = cmMenu;
            ev. gen. H = mwd-> menu;
            ev. gen. i = m ? m-> id : 0;
         }
         if (( msg == WM_INITMENUPOPUP) && ( m == nil))
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
   case WM_MOUSEWHEEL:
      {
         POINT p;
	 p. x = (short)LOWORD( mp2);
	 p. y = (short)HIWORD( mp2);
         ev. cmd         = cmMouseWheel;
         ev. pos. button = ( short) HIWORD( mp1);
         MapWindowPoints( nil, win, &p, 1);
         ev. pos. where. x = p. x;
         ev. pos. where. y = sys lastSize. y - p. y - 1;
      }
      goto MB_MAIN_NOPOS;
   case WM_MOUSEMOVE:
      ev. cmd = cmMouseMove;
      if ( self != lastMouseOver) {
         Handle old = lastMouseOver;
         lastMouseOver = self;
         if ( old && ( PWidget( old)-> stage == csNormal))
            SendMessage(( HWND)(( PWidget) old)-> handle, WM_MOUSEEXIT, mp1, mp2);
         SendMessage( win, WM_MOUSEENTER, mp1, mp2);
         if ( !guts. mouseTimer) {
            guts. mouseTimer = 1;
            if ( !SetTimer( dsys(application)handle, TID_USERMAX, 100, nil)) apiErr;
         }
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
         if ( ev. cmd == cmMouseDown || (ev. cmd == cmMouseClick && ev. pos. dblclk))
            MessageBeep( MB_OK);
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
             if ((( PApplication) application)-> hintUnder == self) v-> self-> set_hintVisible( self, 0);
             if (( v-> options. optSelectable) && ( v-> selectingButtons & ev. pos. button))
                apc_widget_set_focused( self);
         }
      }
      ev. pos. where. x = (short)LOWORD( mp2);
      ev. pos. where. y = sys lastSize. y - (short)HIWORD( mp2) - 1;
   MB_MAIN_NOPOS:
      ev. pos. mod      = 0 |
        (( mp1 & MK_CONTROL ) ? kmCtrl   : 0) |
        (( mp1 & MK_SHIFT   ) ? kmShift  : 0) |
        (( GetKeyState( VK_MENU) < 0) ? kmAlt : 0) |
	apc_pointer_get_state(self)
      ;
      break;
   case WM_MENUCHAR:
       {
          int key;
          ev. key. key    = ctx_remap_def( mp1, ctx_kb2VK2, false, kbNoKey);
          ev. key. code   = mp1;
          ev. key. mod   |=
             (( GetKeyState( VK_SHIFT)   < 0) ? kmShift : 0) |
             (( GetKeyState( VK_CONTROL) < 0) ? kmCtrl  : 0) |
             (( GetKeyState( VK_MENU)    < 0) ? kmAlt   : 0);
          if (( ev. key. mod & kmCtrl) && ( ev. key. code <= 'z'))
             ev. key. code += 'A' - 1;
          key = CAbstractMenu-> translate_key( nilHandle, ev. key. code, ev. key. key, ev. key. mod);
          if ( v-> self-> process_accel( self, key))
             return MAKELONG( 0, MNC_CLOSE);
       }
       break;
   case WM_SYNCMOVE:
       {
          Handle parent = v-> self-> get_parent(( Handle) v);
          if ( parent) {
             Point pos  = var self-> get_origin( self);
             ev. cmd    = cmMove;
             ev. gen. P = pos;
             if ( pos. x == var pos. x && pos. y == var pos. y) ev. cmd = 0;
          }
       }
       break;
   case WM_MOVE:
      {
          Handle parent = v-> self-> get_parent(( Handle) v);
          if ( parent) {
             Point sz = CWidget(parent)-> get_size( parent);
             ev. cmd = cmMove;
             ev. gen . P. x = ( short) LOWORD( mp2);
             ev. gen . P. y = sz. y - ( short) HIWORD( mp2) - sys yOverride;
             if ( is_apt( aptTransparent))
                InvalidateRect( win, nil, false);
          }
      }
      break;
   case WM_NCHITTEST:
      if ( guts. focSysDialog) return HTERROR;
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
      if (( HWND) mp1 != win) {
         Handle mp = hwnd_to_view(( HWND) mp1);
         if ( mp && ( hwnd_top_level( mp) == hwnd_top_level( self)))
            return 0;
         palette_change( self);
      }
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
      if ( guts. focSysDialog) return 1;
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
      sys lastSize. x    = ev. gen. R. right  = ev. gen . P. x = ( short) LOWORD( mp2);
      sys lastSize. y    = ev. gen. R. top    = ev. gen . P. y = ( short) HIWORD( mp2);
      if ( ev. gen. R. top != ev. gen. R. bottom) {
         int delta = ev. gen. R. top - ev. gen. R. bottom;
         Widget_first_that( self, move_back, &delta);
         if ( is_apt( aptFocused)) cursor_update(( Handle) self);
      }
      if ( sys sizeLockLevel == 0 && var stage <= csNormal)
         var virtualSize = sys lastSize;
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
      {
         LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
         if ( sys className == WC_CUSTOM) {
            if (( l-> flags & SWP_NOSIZE) == 0) {
               ev. cmd = cmCalcBounds;
               ev. gen. R. right = l-> cx;
               ev. gen. R. top   = l-> cy;
            }
         }
         if (( l-> flags & SWP_NOZORDER) == 0)
            zorder_sync( self, win, l);
      }
      break;
   case WM_WINDOWPOSCHANGED:
      {
          LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
          if (( l-> flags & SWP_NOZORDER) == 0)
             PostMessage( win, WM_ZORDERSYNC, 0, 0);
          if (( l-> flags & SWP_NOSIZE) == 0) {
             sys yOverride = l-> cy;
             SendMessage( win, WM_SYNCMOVE, 0, 0);
          }
          if ( l-> flags & SWP_HIDEWINDOW) SendMessage( win, WM_SETVISIBLE, 0, 0);
          if ( l-> flags & SWP_SHOWWINDOW) SendMessage( win, WM_SETVISIBLE, 1, 0);
      }
      break;
   case WM_ZORDERSYNC:
      ev. cmd = cmZOrderChanged;
      break;
   }

   if ( hiStage)
      ret = DefWindowProcW( win, msg, mp1, mp2);

   orgCmd = ev. cmd;
   if ( ev. cmd) 
      message_result = v-> self-> message( self, &ev); 
   else 
      ev. cmd = orgMsg;

   if ( v-> stage > csNormal) orgMsg = 0; // protect us from dead body

   switch ( orgMsg) {
   case WM_DESTROY:
      v-> handle = nilHandle;       // tell apc not to kill this HWND
      SetWindowLongPtr( win, GWLP_USERDATA, 0);
      Object_destroy(( Handle) v);
      break;
   case WM_PAINT:
      return 0;
   case WM_SYSKEYDOWN:
       if ( !message_result)
	  guts. dont_xlate_message = true;
       break;
   case WM_SYSKEYUP:
       // ev. cmd = 1; // forced call DefWindowProc superseded for test reasons
       break;
   case WM_MOUSEMOVE:
      if ( is_apt( aptEnabled)) SetCursor( sys pointer);
      break;
   case WM_MOUSEWHEEL:
      return ( LRESULT)1;
   case WM_WINDOWPOSCHANGING:
       {
          LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
          if ( sys className == WC_CUSTOM) {
             if (( l-> flags & SWP_NOSIZE) == 0) {
                int dy = l-> cy - ev. gen. R. top;
                l-> cx = ev. gen. R. right;
                l-> cy = ev. gen. R. top;
                l-> y += dy;
             }
             return false;
          }
          if (( l-> flags & SWP_NOZORDER) == 0)
             zorder_sync( self, win, l);
       }
       break;
   }

   if ( ev. cmd && !hiStage)
      ret = DefWindowProcW( win, msg, mp1, mp2);

   return ret;
}

LRESULT CALLBACK generic_frame_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
   LRESULT ret = 0;
   Handle  self   = GetWindowLongPtr( win, GWLP_USERDATA);
   PWidget   v    = ( PWidget) self;
   UINT    orgMsg = msg;
   Event   ev;
   Bool    hiStage   = false;
   int     i, orgCmd;

   if ( !self)
      return DefWindowProcW( win, msg, mp1, mp2);

   for ( i = 0; i < guts. eventHooks. count; i++) {
      MSG ms = { win, msg, mp1, mp2, 0};
      if ((( PrimaHookProc *)( guts. eventHooks. items[i]))((void*) &ms))
         return 0;
   }    

   memset( &ev, 0, sizeof (ev));
   ev. gen. source = self;

   switch ( msg) {
   case WM_ACTIVATE:
       if ( guts. focSysDialog) return 1;
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
   case WM_MOUSEWHEEL:
   case WM_ZORDERSYNC:
      return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
   case WM_QUERYNEWPALETTE:
      return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
   case WM_PALETTECHANGED:
      if (( HWND) mp1 == win) return 0;
      return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
   case WM_SYSKEYDOWN:
   case WM_SYSKEYUP:
       if ( generic_view_handler(( HWND) v-> handle, msg, mp1, mp2) == 0)
          return 0;
       hiStage = true;
       break;
   case WM_DLGENTERMODAL:
       ev. cmd = mp1 ? cmExecute : cmEndModal;
       break;
   case WM_ERASEBKGND:
       return 1;
   case WM_NCACTIVATE:
      if ( guts. focSysDialog) return 1;
       
      if (( mp1 == 0) && ( mp2 != 0)) {
         Handle x = hwnd_to_view(( HWND) mp2);
         if ( is_declipped_child( x) && Widget_is_child( x, self)) {
            return 1;
         }   
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
      if ( guts. focSysDialog) return HTERROR;
      // dlg protect code - protecting from user actions
      if ( !guts. focSysDisabled) {
         Handle foc = Application_map_focus( application, self);
         if ( foc != self) {
            return ( foc == apc_window_get_active()) ? HTERROR : HTCLIENT;
         }   
      }
      break;
   case WM_SETFOCUS:
       if ( guts. focSysDialog) return 1;
      
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
          Bool hasCO = w == self;
          while ( w && w != self) {
             if ( !dsys( w) options. aptClipOwner) {
                hasCO = true;
                break;
             }
             w = (( PWidget) w)-> owner;
          }
          if ( !hasCO) {
             var self-> set_selected( self, true);
          }   
          // else we do not select any widget, but still have a chance to resize frame :)
       }
       break;
   case WM_SIZE:
       {
          int state = wsNormal;
          Bool doWSChange = false;
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
          Handle parent = v-> self-> get_parent(( Handle) v);
          if ( parent) {
             Point pos  = var self-> get_origin( self);
             ev. cmd    = cmMove;
             ev. gen. P = pos;
             if ( pos. x == var pos. x && pos. y == var pos. y) ev. cmd = 0;
          }
       }
       break;
   case WM_MOVE:
       {
          Handle parent = v-> self-> get_parent(( Handle) v);
          if ( parent) {
             Point sz = CWidget(parent)-> get_size( parent);
             ev. cmd = cmMove;
             ev. gen . P. x = ( short) LOWORD( mp2);
             ev. gen . P. y = sz. y - ( short) HIWORD( mp2) - sys yOverride;
          }
       }
       break;
// case WM_SYSCHAR:return 1;
   case WM_TIMER:
      if ( mp1 == TID_USERMAX)
      // application local timer
      {
        POINT p;
        HWND wp;
        if ( lastMouseOver && !GetCapture() && ( PObject( lastMouseOver)-> stage == csNormal))
        {
           HWND desktop = HWND_DESKTOP;
           GetCursorPos( &p);
           wp = WindowFromPoint( p);
           if ( wp) {
              POINT xp = p;
              MapWindowPoints( desktop, wp, &xp, 1);
              wp = ChildWindowFromPointEx( wp, xp, CWP_SKIPINVISIBLE);
           } else
              wp = ChildWindowFromPointEx( wp, p, CWP_SKIPINVISIBLE);
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
              } else if ( guts. mouseTimer) {
                 guts. mouseTimer = 0;
                 if ( !KillTimer( dsys(application)handle, TID_USERMAX)) apiErr;
              }
           }
        }
        return 0;
      }
      break;
   case WM_GETMINMAXINFO:
      {
         LPMINMAXINFO l = ( LPMINMAXINFO) mp2;
         Point min = var self-> get_sizeMin( self);
         Point max = var self-> get_sizeMax( self);
         Point bor = get_window_borders( sys s. window. borderStyle);
         int   dy  = 0 +
            (( sys s. window. borderIcons & biTitleBar) ? GetSystemMetrics( SM_CYCAPTION) : 0) +
            ( PWindow(self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0);
         l-> ptMinTrackSize. x = min. x + bor.x * 2;
         l-> ptMinTrackSize. y = min. y + bor.y * 2 + dy;
         l-> ptMaxTrackSize. x = max. x + bor.x * 2;
         l-> ptMaxTrackSize. y = max. y + bor.y * 2 + dy;
      }
      break;
   case WM_WINDOWPOSCHANGED:
       {
           LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
           if (( l-> flags & SWP_NOZORDER) == 0)
              PostMessage( win, WM_ZORDERSYNC, 0, 0);
           if (( l-> flags & SWP_NOSIZE) == 0) {
              RECT r;
              GetClientRect( win, &r);
              sys yOverride = r. bottom - r. top;
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
      ret = DefWindowProcW( win, msg, mp1, mp2);

   orgCmd = ev. cmd;
   if ( ev. cmd) v-> self-> message( self, &ev); else ev. cmd = orgMsg;

   if ( var stage == csDead) orgMsg = 0;

   switch ( orgMsg) {
   case WM_CLOSE:
      if ( ev. cmd) {
         if ( sys className == WC_FRAME && PWindow(self)->modal) {
           CWindow( self)-> cancel( self);
           return 0;
         } else {
           SetWindowLongPtr( win, GWLP_USERDATA, 0);
           Object_destroy(( Handle) v);
         }
         break;
      } else
         return 0;
   }

   if ( ev. cmd && !hiStage)
      ret = DefWindowProcW( win, msg, mp1, mp2);
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
            HBITMAP hbm;

            if ( dc) {
               guts. displayBMInfo. bmiHeader. biBitCount = 0;
               guts. displayBMInfo. bmiHeader. biSize = sizeof( BITMAPINFO);
               if ( !( hbm = GetCurrentObject( dc, OBJ_BITMAP))) apiErr;

               if ( !GetDIBits( dc, hbm, 0, 0, NULL, &guts. displayBMInfo, DIB_PAL_COLORS)) {
                  guts. displayBMInfo. bmiHeader. biBitCount = ( int) mp1;
                  guts. displayBMInfo. bmiHeader. biPlanes   = GetDeviceCaps( dc, PLANES);
               };
            }
            dsys( application) lastSize. x = ( short) LOWORD( mp2);
            dsys( application) lastSize. y = ( short) HIWORD( mp2);
            if ( dc) {
               if ( oldBPP != guts. displayBMInfo. bmiHeader. biBitCount)
                  hash_first_that( imageMan, kill_img_cache, (void*)1, nil, nil);
               dc_free();
            }
         }
         break;
      case WM_FONTCHANGE:
         destroy_font_hash();
         break;
      case WM_COMPACTING:
         stylus_clean();
         font_clean();
         destroy_font_hash();
         hash_first_that( imageMan, kill_img_cache, nil, nil, nil);
         hash_destroy( regnodeMan, false);
         regnodeMan = hash_create();
         break;
      case WM_QUERYNEWPALETTE:
      case WM_PALETTECHANGED:
         return 0;
   }
   return generic_frame_handler( win, msg, mp1, mp2);
}

#ifdef __cplusplus
}
#endif
