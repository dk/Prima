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
#include <stdio.h>
#define INCL_GPI
#include "os2/os2guts.h"
#include "Application.h"
#include "Component.h"
#include "Menu.h"
#include "Widget.h"
#include "Window.h"
#include <float.h>
#include <signal.h>

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( v->handle, QW_PARENT) : v->handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)


OS2Guts guts;
ERRORID rc            = 0;
Handle lastMouseOver = nilHandle;
extern Handle hwnd_to_view( HWND win);
extern Bool single_color_notify ( Handle self, Handle child, void * color);
extern Bool font_notify ( Handle self, Handle child, void * font);


void sigh ( int sig);

static unsigned long dllModHandle = 0;

unsigned long _DLL_InitTerm( unsigned long modhandle, unsigned long flag)
{
   dllModHandle = flag ? 0 : modhandle;
   return 1;
}

Bool
window_subsystem_init( char * error_buf)
{
   ULONG minor;
   PPIB  ppib;
   PTIB  ptib;

   DosGetInfoBlocks( &ptib, &ppib);

   DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, &minor, sizeof( minor));
   memset( &guts, 0, sizeof( guts));
   guts. pid = ppib-> pib_ulpid;

   guts. appTypePM = 1;
   ppib-> pib_ultype = 3; /* nasty hack - but that way stdout and PM can coexist */

   if ( !( guts. anchor = WinInitialize( 0))) {
      sprintf( error_buf, "WinIntialize(0) error");
      return false;
   }
   if ( !( guts. queue = WinCreateMsgQueue( guts. anchor, minor <= 30 ? 256 : 0))) {
      rc = WinGetLastError(guts. anchor);
      WinTerminate( guts. anchor);
      if ( rc == 0x81051) {
         sprintf( error_buf, "WinCreateMsgQueue: not a PM program");
         guts. appTypePM = 0;
      } else {
         sprintf( error_buf, "WinCreateMsgQueue: error %03x", (int)rc);
      }
      return false;
   }
   _fpreset();
   create_font_hash();

   { // make Prima.dll accessible for other modules
      char buf[2048];
      DosQueryModuleName(dllModHandle, 2048, buf);
      apc_dl_export( buf);
   }

   if ( !WinRegisterClass( guts. anchor,
                     "GeNeRiC",
                     generic_view_handler,
                     CS_MOVENOTIFY,
                     4)) apiErrRet;
   {
      ULONG cps;
      guts. codePage = 0;
      rc = DosQueryCp( sizeof( guts. codePage), &guts. codePage, &cps);
      if ( rc != 0 && rc != 473 /*ERROR_CPLIST_TOO_SMALL*/) {
         sprintf( error_buf, "DosQueryCp: error %03x", (int)rc);
         return false;
      }
   }
   WinGetLastError( guts. anchor);
   {
      HDC   dc = DevOpenDC( guts. anchor, OD_MEMORY, "*", 0, nil, nilHandle);
      SIZEL s = {0, 0};
      ULONG lFmts[ 256];
      int i;

      if ( dc == nilHandle) apiErrRet;
      if ( !( guts. ps = GpiCreatePS( guts. anchor, dc, &s, PU_PELS | GPIT_MICRO | GPIA_ASSOC))) {
         rc = WinGetLastError(guts. anchor);
         sprintf( error_buf, "GpiCreatePS: error %03x", (int)rc);
         DevCloseDC( dc);
         return false;
      }
      if ( !GpiCreateLogColorTable( guts. ps, 0, LCOLF_RGB, 0, 0, nil)) {
         rc = WinGetLastError(guts. anchor);
         sprintf( error_buf, "GpiCreateLogColorTable: error %03x", (int)rc);
         GpiDestroyPS( guts. ps);
         DevCloseDC( dc);
         return false;
      }
      GpiQueryCharBox( guts. ps, &guts. defFontBox);
      guts. fontId = 0;
      if ( !( guts. fontHash = create_fontid_hash())) {
         sprintf( error_buf, "no memory");
         GpiDestroyPS( guts. ps);
         DevCloseDC( dc);
         return false;
      }
      DevQueryCaps( dc, CAPS_HORIZONTAL_FONT_RES, 1, ( PLONG) &guts. displayResolution. x);
      DevQueryCaps( dc, CAPS_VERTICAL_FONT_RES, 1, ( PLONG) &guts. displayResolution. y);
      DevQueryCaps( dc, CAPS_BITMAP_FORMATS, 1, ( PLONG) &guts. bmfCount);
      WinGetLastError( guts. anchor);
      if ( !GpiQueryDeviceBitmapFormats( guts. ps, 2 * guts. bmfCount, lFmts)) {
         rc = WinGetLastError(guts. anchor);
         sprintf( error_buf, "GpiQueryDeviceBitmapFormats: error %03x", (int)rc);
         GpiDestroyPS( guts. ps);
         DevCloseDC( dc);
         return false;
      };
      if ( !( guts. bmf = malloc( guts. bmfCount * sizeof( int) * 2))) {
         sprintf( error_buf, "no memory");
         GpiDestroyPS( guts. ps);
         DevCloseDC( dc);
         return false;
      }
      for ( i = 0; i < guts. bmfCount * 2; i++) guts. bmf[ i] = lFmts[ i];
      gp_get_font( guts. ps, &guts. sysDefFont, guts. displayResolution);
      apc_font_pick( nilHandle, &guts. sysDefFont, &guts. sysDefFont);
      guts. monoBitsOk = ( WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN) == 350 && guts. bmf[ 1] == 4);
   }
   list_create( &guts. transp, 8, 8);
   list_create( &guts. psList, 8, 8);
   list_create( &guts. winPsList, 8, 8);
   list_create( &guts. eventHooks, 1, 1);
   list_create( &guts. files, 8, 8);
   guts. appLock = 0;
   guts. pointerLock = 0;

   signal( SIGSEGV, sigh);
   signal( SIGTERM, sigh);
   signal( SIGFPE , sigh);

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

Bool freePS( HPS ps, void * dummy)    { WinEndPaint ( ps); return false; }
Bool freeWinPS( HPS ps, void * dummy) { WinReleasePS( ps); return false; }

void
window_subsystem_cleanup( void)
{
   list_first_that( &guts. psList, freePS, nil);
   list_first_that( &guts. winPsList, freeWinPS, nil);
   guts. psList. count = 0;
   guts. winPsList. count = 0;
   while ( guts. appLock > 0) apc_application_unlock( application);
   while ( guts. pointerLock < 0) {
      WinShowPointer(  HWND_DESKTOP, 1);
      guts. pointerLock++;
   }

   {
      QMSG msg;
      for(;;) {
         if ( WinPeekMsg( guts. anchor, &msg, 0, WM_QUIT, WM_QUIT, PM_REMOVE)) continue;
         if ( WinPeekMsg( guts. anchor, &msg, 0, WM_CLOSE, WM_CLOSE, PM_REMOVE)) continue;
         break;
      }
   }
}

void
sigh( int sig)
{
    if ( sig == SIGSEGV || sig == SIGTERM || sig == SIGFPE) {
       list_first_that( &guts. psList, freePS, nil);
       list_first_that( &guts. winPsList, freeWinPS, nil);
    }
    signal( sig, SIG_ACK);
}

void
window_subsystem_done( void)
{
   HDC dc = GpiQueryDevice( guts. ps);

   if ( guts. socketMutex) {
      // appDead must be TRUE for this moment!
      appDead = true;
      DosCloseMutexSem( guts. socketMutex);
   }

   
   list_destroy( &guts. files);
   list_destroy( &guts. eventHooks);
   list_destroy( &guts. transp);
   list_first_that( &guts. psList, freePS, nil);
   list_first_that( &guts. winPsList, freeWinPS, nil);
   list_destroy( &guts. psList);
   list_destroy( &guts. winPsList);
   free( guts. bmf);
   GpiDestroyPS( guts. ps);
   guts. fontId = 0;
   destroy_fontid_hash( guts. fontHash);
   DevCloseDC( dc);
   destroy_font_hash();
   WinDestroyMsgQueue( guts. queue);
   WinTerminate( guts. anchor);
   guts. anchor = guts. queue = nilHandle;
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


static Bool
local_wnd( HWND who, HWND client)
{
   HWND parent = WinQueryWindow( who, QW_PARENT);
   HWND owner  = WinQueryWindow( who, QW_OWNER);
   HWND frame  = WinQueryWindow( client, QW_PARENT);
   PComponent v;
   Handle self;
   if ( who == client || parent == client || owner == client || parent == frame || owner == frame)
      return true;
   self = WinQueryWindowULong( client, QWL_USER);
   v = (PComponent)hwnd_to_view( who);
   if (!v && WinQueryWindowPtr( who, QWP_PFNWP) == generic_menu_handler)
      v = (PComponent)(( PMenuWndData) WinQueryWindowULong( who, QWL_USER))-> menu;
   while (v && ( Handle) v != application)
   {
      if ((Handle)v == self) return true;
      ( Handle) v = v-> owner;
   }
   return false;
}


MRESULT EXPENTRY
generic_view_handler( HWND w, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   Handle view;
   PWidget v;
   PWidget orgv;
   Event ev;
   MRESULT toReturn = 0;
   ULONG   orgMsg;
   int i, orgCmd;
   Handle self;
   PFNWP fnwp;
   Bool hiStage = false;

#define flow(cm)  ev.cmd = (cm); ( PWidget) v = ctrl;  ( PWidget) view  = ctrl;
#define flowb(cm) flow(cm) break;

   view = WinQueryWindowULong( w, QWL_USER);
   if (( view == nilHandle) || appDead)
      return WinDefWindowProc( w, msg, mp1, mp2);

   for ( i = 0; i < guts. eventHooks. count; i++) {
      QMSG ms = { w, msg, mp1, mp2, 0};
      if ((( PrimaHookProc *)( guts. eventHooks. items[i]))((void*) &ms))
         return 0;
   }


   // Here we (fortunately) have all we need to parse events
   v = ( PWidget) view;
   self = view;
   ( ApiHandle) fnwp = sys handle2;
   orgv = v;
   orgMsg  = msg;
   memset( &ev, 0, sizeof (ev));
   ev. gen. source = view;
   switch ( msg)
   {
      case WM_BUTTON1DBLCLK:
        ev. pos. button = mbLeft;
        goto _GenEvDoubleC;
      case WM_BUTTON2DBLCLK:
        ev. pos. button = mbRight;
        goto _GenEvDoubleC;
      case WM_BUTTON3DBLCLK:
        ev. pos. button = mbMiddle;
        goto _GenEvDoubleC;
      case WM_BUTTON1CLICK:
        ev. pos. button = mbLeft;
        goto _GenEvClick;
      case WM_BUTTON2CLICK:
        ev. pos. button = mbRight;
        goto _GenEvClick;
      case WM_BUTTON3CLICK:
        ev. pos. button = mbMiddle;
        goto _GenEvClick;
      case WM_BUTTON1UP:
        ev. pos. button = mbLeft;
        goto _GenEvUpC;
      case WM_BUTTON2UP:
        ev. pos. button = mbRight;
        goto _GenEvUpC;
      case WM_BUTTON3UP:
        ev. pos. button = mbMiddle;
        goto _GenEvUpC;
      case WM_BUTTON1DOWN:
        ev. pos. button = mbLeft;
        goto _GenEvDnC;
      case WM_BUTTON2DOWN:
        ev. pos. button = mbRight;
        goto _GenEvDnC;
      case WM_BUTTON3DOWN:
        ev. pos. button = mbMiddle;
        goto _GenEvDnC;
      case WM_MOUSEMOVE:
        ev. cmd = cmMouseMove;
        goto _GenEvC;
      case WM_MOUSEENTER:
        ev. cmd = cmMouseEnter;
        goto _GenEvC;
      case WM_MOUSELEAVE:
        ev. cmd = cmMouseLeave;
        goto _GenEvC;

      _GenEvDoubleC:
       ev. pos. dblclk = TRUE;
      _GenEvClick:
       ev. cmd = cmMouseClick;
       goto _GenEvC;
      _GenEvUpC:
       ev. cmd = cmMouseUp;
       goto _GenEvC;
      _GenEvDnC:
       ev. cmd = cmMouseDown;
       goto _GenEvC;

      _GenEvC:
        if ((short)SHORT1FROMMP( mp2) == HT_DISCARD) return 0;
        if ( !is_apt( aptEnabled)) {
           if (( ev. cmd != cmMouseMove) &&
               ( ev. cmd != cmMouseEnter) &&
               ( ev. cmd != cmMouseLeave))
           {
              if ( ev. cmd == cmMouseDown || (ev. cmd == cmMouseClick && ev. pos. dblclk))
                 WinAlarm( HWND_DESKTOP, WA_WARNING);
              return 0;
           }
        }

        if ( ev.cmd == cmMouseMove && self != lastMouseOver) {
           Handle old = lastMouseOver;
           lastMouseOver = self;
           if ( old) WinSendMsg((( PWidget) old)-> handle, WM_MOUSELEAVE, mp1, mp2);
           WinSendMsg( w, WM_MOUSEENTER, mp1, mp2);
        } else if ( ev. cmd == cmMouseDown && !is_apt( aptFirstClick)) {
            Handle x = self;
            while ( dsys(x) className != WC_FRAME && ( x != application)) x = (( PWidget) x)-> owner;
            // possible bug with toplevel non-windows
            if ( x != application && !local_wnd( WinQueryActiveWindow( HWND_DESKTOP), (( PWidget) x)-> handle)) {
                ev. cmd = 0; // yes, we abandon mousedown but we should force selection:
                if ((( PApplication) application)-> hintUnder == self) v-> self-> set_hintVisible( self, 0);
                if (( v-> options. optSelectable) && ( v-> selectingButtons & ev. pos. button))
                   apc_widget_set_focused( self);
            }
        }
        ev. pos. where. x = (short)SHORT1FROMMP( mp1);
        ev. pos. where. y = (short)SHORT2FROMMP( mp1);
        if  ( SHORT2FROMMP( mp2) & KC_SHIFT) { ev. pos. mod |= kmShift; }
        if  ( SHORT2FROMMP( mp2) & KC_ALT  ) { ev. pos. mod |= kmAlt;   }
        if  ( SHORT2FROMMP( mp2) & KC_CTRL ) { ev. pos. mod |= kmCtrl;  }
        break;
      case WM_ACTIVATEMENU:
         {
            hiStage = true;
            ev. cmd    = cmMenu;
            ev. gen. H = ( Handle) mp1;
            ev. gen. i = ( int)    mp2;
         }
         break;
      case WM_ADJUSTWINDOWPOS:
        if ( sys className != WC_FRAME)
        {
          int fl = (( PSWP) mp1) -> fl;
          if (fl & ( SWP_SIZE | SWP_MOVE | SWP_MAXIMIZE | SWP_RESTORE | SWP_HIDE)) {
             ev. cmd = cmCalcBounds;
             ev. gen. R. left   = (( PSWP) mp1) -> x;
             ev. gen. R. bottom = (( PSWP) mp1) -> y;
             ev. gen. R. right  = (( PSWP) mp1) -> cx;
             ev. gen. R. top    = (( PSWP) mp1) -> cy;
          }
        }
        break;
      case WM_CALCVALIDRECTS:
        if ( v-> self-> custom_paint( self))
           return MPFROMLONG( CVR_REDRAW);
        else
           return MPFROMLONG( CVR_ALIGNLEFT | CVR_ALIGNBOTTOM);
      case WM_CHAR:
        if ( apc_widget_is_responsive( self))
        {
           ev. cmd = ( SHORT1FROMMP( mp1) & KC_KEYUP) ? cmKeyUp : cmKeyDown;
           ev. key. code   = SHORT1FROMMP( mp2) & 0xFF;
           ev. key. key    = ctx_remap_def( SHORT2FROMMP( mp2), ctx_kb2VK, false, kbNoKey);
           ev. key. repeat = CHAR3FROMMP( mp1);
           ev. key. mod    = 0;
           if  ( SHORT1FROMMP( mp1) & KC_SHIFT) { ev. key. mod |= kmShift; }
           if  ( SHORT1FROMMP( mp1) & KC_ALT  ) { ev. key. mod |= kmAlt;   }
           if  ( SHORT1FROMMP( mp1) & KC_CTRL ) {
              ev. key. mod |= kmCtrl;
              if ( isalpha( ev. key. code))
                  ev. key. code = toupper( ev. key. code) - '@';
           }
           if  ( SHORT1FROMMP( mp1) & KC_DEADKEY) { ev. key. mod |= kmDeadKey; }
        }
        break;
      case WM_CLOSE:
         ev. cmd = cmClose;
         break;
      case WM_COMMAND:
         {
            switch ( SHORT1FROMMP(  mp2))
            {
            case CMDSRC_MENU:
               if ( SHORT1FROMMP( mp1) <= MENU_ID_AUTOSTART)
               {
                  HWND active = WinQueryFocus( HWND_DESKTOP);
                  if ( active != nilHandle) WinSendMsg ( active, SHORT1FROMMP( mp1), 0, 0);
               }
               break;
            } // end source case
         }
         break;
      case WM_CONTEXTMENU :
        ev. cmd       = cmPopup;
        ev. gen. B    = !SHORT2FROMMP( mp2);
        // mouse event
        ev. gen. P. x = SHORT1FROMMP( mp1);
        ev. gen. P. y = SHORT2FROMMP( mp1);
        break;
      case WM_DESTROY:
        if ( v-> stage <= csNormal)
        {
           v-> handle = nilHandle;  // tell apc not to kill this HWND
           WinSetWindowULong( w, QWL_USER, 0);
           Object_destroy(( Handle) v);
        }
        break;
      case WM_ENABLE:
         ev. cmd = SHORT1FROMMP( mp1) ? cmEnable : cmDisable;
         hiStage = true;
         break;
      case WM_FOCUSCHANGE:
         if ( !guts. focSysDisabled && !guts. focSysGranted) {
            Handle hf = Application_map_focus( application, self);
            if ( hf && hf != self) {
               WinPostMsg( w, WM_FORCEFOCUS, ( MPARAM) (( PWidget) hf)-> handle, ( MPARAM) hf);
               return ( MPARAM) 0;
            }
         }
         break;
      case WM_FORCEFOCUS:
         if ( mp2 && WinIsWindow( guts. anchor, ( HWND) mp1))
            ((( PWidget) mp2)-> self)-> set_selected(( Handle) mp2, 1);
         return 0;
      case WM_FONTCHANGED:
         ev. cmd = cmFontChanged;
         break;
      case WM_HELP:
         if ( apc_widget_is_responsive( self)) {
            ev. cmd      = cmKeyDown;
            ev. key. key = kbF1;
         }
         break;
      case WM_MENUCOMMAND:
         ev. gen. i = SHORT1FROMMP( mp1);
         ev. gen. H = ev. gen. source = LONGFROMMP( mp2);
         ev. cmd = cmMenuCmd;
         break;
      case WM_MENUSELECT:
         {
            PMenuWndData menu = ( PMenuWndData) WinQueryWindowULong(( HWND) mp2, QWL_USER);
            if ( menu && SHORT2FROMMP( mp1) && ( SHORT1FROMMP( mp1) > MENU_ID_AUTOSTART))
            {
               if (( ev. gen. source = menu-> menu) != nilHandle)
                  WinPostMsg( w, WM_MENUCOMMAND, mp1, MPFROMLONG( menu-> menu));
            }
         }
         break;
      case WM_MOVE:
         ev. cmd = cmMove;
         ev. gen. P = v-> self-> get_origin( self);
         if ( ev. gen. P. x == v-> pos. x && ev. gen. P. y == v-> pos. y) ev. cmd = 0;
         break;
      case WM_REPAINT:
         ev. cmd = cmRepaint;
         break;
      case WM_PAINT:
        {
           PWidget vv = ( PWidget)( v-> owner);
           Bool ok = true;
           if ( v-> stage == csNormal) {
              while ( vv) {
                 if ( vv-> stage < csNormal) {
                    ok = false;
                    break;
                 }
                 vv = ( PWidget) vv-> owner;
              }
           }
           if ( !ok) {
              WinPostMsg( w, WM_REPAINT, 0, 0);
              break;
           }

           ev. cmd = cmPaint;
           switch (( int) sys className)
           {
             case WC_FRAME:
                break;
             case WC_CUSTOM:
                if ( v-> stage == csNormal && list_index_of( &guts. transp, self) >= 0) return 0;
                break;
             default:
               ev. cmd = 0;
           }
           break;
        }
      case WM_PRESPARAMMENU:
         ev. cmd = LONGFROMMP( mp1);
         ev. gen. source = ( Handle) mp2;
         if ( ev. cmd == cmFontChanged) hiStage = true;
         break;
      case WM_MENUCOLORCHANGED:
         ev. cmd = cmColorChanged;
         ev. gen. source = ( Handle) mp2;
         ev. gen. i = LONGFROMMP( mp1);
         hiStage = true;
         break;
      case WM_PRESPARAMCHANGED:
       if ( v-> stage == csNormal)
         switch ( LONGFROMMP( mp1)) {
             case PP_FOREGROUNDCOLOR:
             case PP_FOREGROUNDCOLORINDEX:
                ev. gen. i = ciFore;
                opt_clear( optOwnerColor);
                goto SETCMD;
             case PP_BACKGROUNDCOLOR:
             case PP_BACKGROUNDCOLORINDEX:
                ev. gen. i = ciBack;
                opt_clear( optOwnerBackColor);
                goto SETCMD;
             case PP_HILITEFOREGROUNDCOLOR:
             case PP_HILITEFOREGROUNDCOLORINDEX:
                ev. gen. i = ciHiliteText;
                goto SETCMD;
             case PP_HILITEBACKGROUNDCOLOR:
             case PP_HILITEBACKGROUNDCOLORINDEX:
                ev. gen. i = ciHilite;
                goto SETCMD;
             case PP_DISABLEDFOREGROUNDCOLOR:
             case PP_DISABLEDFOREGROUNDCOLORINDEX:
                ev. gen. i = ciDisabledText;
                goto SETCMD;
             case PP_DISABLEDBACKGROUNDCOLOR:
             case PP_DISABLEDBACKGROUNDCOLORINDEX:
                ev. gen. i = ciDisabled;
             SETCMD:
                {
                   SingleColor s = { apc_widget_get_color( self, ev. gen. i), ev. gen. i};
                   v-> self-> first_that( self, single_color_notify, &s);
                }
                if ( ev. gen. i == ciFore) opt_clear( optOwnerColor); else
                if ( ev. gen. i == ciBack) opt_clear( optOwnerBackColor);
                ev. cmd = cmColorChanged;
                hiStage = true;
                break;
             case PP_FONTNAMESIZE:
             case PP_FONTHANDLE:
                view_get_font( view, &v-> font);
                v-> self-> first_that( self, font_notify, &v-> font);
                opt_clear( optOwnerFont);
                ev. cmd = cmFontChanged;
                hiStage = true;
                break;
         }
         break;
      case WM_PRIMA_CREATE:
         ev. cmd = cmSetup;
         break;
      case WM_SETFOCUS:
         ev. cmd = SHORT1FROMMP( mp2) ? cmReceiveFocus : cmReleaseFocus;
         ev. gen. source = LONGFROMMP( mp1);
         hiStage = true;
         apt_assign( aptFocused, ev. cmd == cmReceiveFocus);
         cursor_update( self);
         WinShowCursor( w, is_apt( aptFocused) && is_apt( aptCursorVis));
         break;
      case WM_SHOW:
        if ( list_index_of( &guts. transp, self) < 0)
        {
           if ( v-> stage <= csNormal) ev. cmd = SHORT1FROMMP( mp1) ? cmShow : cmHide;
           hiStage = true;
           apt_assign( aptVisible, SHORT1FROMMP( mp1));
        }
        break;
      case WM_TIMER:
        {
           int id = SHORT1FROMMP( mp1) - 1;
           if ( id + 1 == TID_USERMAX-1)
           // application local timer
           {
             POINTL p;
             HWND wp;

             // checking for WM_MOUSEENTER
             if ( lastMouseOver && !WinQueryCapture( HWND_DESKTOP))
             {
                WinQueryPointerPos( HWND_DESKTOP, &p);
                wp = WinWindowFromPoint( HWND_DESKTOP, &p, true);
                if ( wp != (( PWidget) lastMouseOver)-> handle)
                {
                   HWND old = (( PWidget) lastMouseOver)-> handle;
                   Handle s;
                   lastMouseOver = nilHandle;
                   WinSendMsg( old, WM_MOUSELEAVE, 0, 0);
                   s = hwnd_to_view( wp);
                   if (s && (( PWidget) s)-> handle == wp)
                   {
                      WinMapWindowPoints( HWND_DESKTOP, wp, &p, 1);
                      WinSendMsg( wp, WM_MOUSEENTER, MPFROM2SHORT( p. x, p. y), 0);
                      lastMouseOver = s;
                   }
                }
             }
             return 0;
           }
           if ( id >= 0 && id < sys timeDefsCount) ev. gen. H = ( Handle) sys timeDefs[ id]. item;
           if ( ev. gen. H)
           {
               v = ( PWidget)view = ev. gen. H;
               ev. cmd = cmTimer;
           }
        }
        break;
      case WM_POSTAL:
         ev. cmd    = cmPost;
         ev. gen. H = ( Handle) mp1;
         ev. gen. p = ( void *) mp2;
         break;
      case WM_WINDOWPOSCHANGED:
         if (( v-> stage <= csNormal) && ( sys className != WC_FRAME))
         {
            PSWP New = PVOIDFROMMP( mp1);
            PSWP Old = New + 1;
            int fl = New-> fl;
            if ( fl & SWP_ZORDER)
               WinPostMsg( w, WM_ZORDERSYNC, 0, 0);
            if ( fl & SWP_MOVE)
            {
               ev. cmd = cmMove;
               ev. gen . P. x = New-> x;
               ev. gen . P. y = New-> y;
               if ( fl & (SWP_SIZE|SWP_RESTORE|SWP_MAXIMIZE))
               {
                  New->fl &= ~SWP_MOVE;
                  WinSendMsg( w, WM_WINDOWPOSCHANGED, mp1, mp2);
                  New->fl |= SWP_MOVE;
               }
               if ( ev. gen. P. x == v-> pos. x && ev. gen. P. y == v-> pos. y) ev. cmd = 0;
            } else if ( fl & (SWP_SIZE|SWP_RESTORE|SWP_MAXIMIZE)) {
               ev. cmd = cmSize;
               ev. gen. R. right  = ev. gen . P. x = New-> cx;
               ev. gen. R. top    = ev. gen . P. y = New-> cy;
               ev. gen. R. left   = Old-> cx;
               ev. gen. R. bottom = Old-> cy;
               if (( sys sizeLockLevel == 0) && ( v-> stage <= csNormal))
                  v-> virtualSize = ev. gen. P;
            }
            if ( is_apt( aptTransparent)) WinInvalidateRect( w, nil, false);
         }
         break;
      case WM_ZORDERSYNC:
         ev. cmd = cmZOrderChanged;
         break;
   }

   if ( hiStage && WinIsWindow( guts. anchor, w))
      toReturn = fnwp ? fnwp( w, orgMsg, mp1, mp2) :
                        WinDefWindowProc( w, msg, mp1, mp2);

   orgCmd = ev. cmd;
   if ( ev. cmd) v-> self-> message( view, &ev); else ev. cmd = orgMsg;

   if ( v-> stage <= csDestroying)
   switch ( orgMsg)
   {
      case WM_ADJUSTWINDOWPOS:
         if ( ev. cmd) {
            (( PSWP) mp1) -> x  =  ev. gen. R. left;
            (( PSWP) mp1) -> y  =  ev. gen. R. bottom;
            (( PSWP) mp1) -> cx =  ev. gen. R. right;
            (( PSWP) mp1) -> cy =  ev. gen. R. top;
         }
         break;
      case WM_BUTTON1DOWN: case WM_BUTTON1UP: case WM_BUTTON1CLICK: case WM_BUTTON1DBLCLK:
      case WM_BUTTON2DOWN: case WM_BUTTON2UP: case WM_BUTTON2CLICK: case WM_BUTTON2DBLCLK:
      case WM_BUTTON3DOWN: case WM_BUTTON3UP: case WM_BUTTON3CLICK: case WM_BUTTON3DBLCLK:
         return ( MRESULT)1;
      case WM_CHAR:
         if ( ev. cmd == 0 || WinIsWindowEnabled( w))
         {
            // this is to avoid default send to owner hwnd
            return ( MRESULT)1;
         }
         break;
      case WM_CLOSE:
        if ( ev. cmd)
        {
           if ( sys className == WC_FRAME && PWindow(self)->modal) {
              CWindow(self)-> cancel( self);
              return 0;
           }
           else
             Object_destroy(( Handle) v);
           break;
        } else {
           return 0;
        }
      case WM_HELP:
         if ( !ev. cmd)
           WinSendMsg( w, WM_VIEWHELP, 0, 0);
         break;
      case WM_COMMAND:
          return toReturn;
      case WM_PRESPARAMCHANGED:
        WinInvalidateRect ( w, nil, true);
        break;
      case WM_MOUSEMOVE:
        if ( WinIsWindowEnabled( w)) WinSetPointer( HWND_DESKTOP, sys pointer);
        return MRFROMLONG(1);
   }

   if ( ev. cmd && !hiStage)
   {
      if ( !WinIsWindow( guts. anchor, w)) return 0;
      toReturn = fnwp ? fnwp( w, orgMsg, mp1, mp2) :
                        WinDefWindowProc( w, msg, mp1, mp2);
   }
   return toReturn;
}

MRESULT EXPENTRY
generic_frame_handler( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   HWND  client = WinWindowFromID( win, FID_CLIENT);
   PFNWP fProc  = ( PFNWP) WinQueryWindowULong( win, QWL_USER);
   Event ev;
   Handle self = WinQueryWindowULong( client, QWL_USER);
   PWidget v = ( PWidget) self;
   MPARAM toReturn = 0;
   Bool hiStage = false;
   Point dPoint;
   int i;

   if ( !self)
      return fProc ? fProc( win, msg, mp1, mp2) : WinDefWindowProc( win, msg, mp1, mp2);

   for ( i = 0; i < guts. eventHooks. count; i++) {
      QMSG ms = { win, msg, mp1, mp2, 0};
      if ((( PrimaHookProc *)( guts. eventHooks. items[i]))((void*) &ms))
         return 0;
   }


   memset( &ev, 0, sizeof (ev));
   ev. gen. source = self;

   switch ( msg)
   {
      case WM_ACTIVATE:
         ev. cmd = SHORT1FROMMP(mp1) ? cmActivate : cmDeactivate;
         break;
      case WM_ADJUSTWINDOWPOS:
      {
         int fl = (( PSWP) mp1) -> fl;

         if ( fl & SWP_ZORDER) {
            Handle hf = Application_map_focus( application, self);
            if ( hf && hf != self)
               (( PSWP) mp1)-> fl &= ~SWP_ZORDER;
         }

         if (fl & ( SWP_SIZE | SWP_MAXIMIZE | SWP_RESTORE))
         // if (fl & ( SWP_SIZE | SWP_MAXIMIZE))
         {
            Point p;
            ev. cmd = cmCalcBounds;
            ev. gen. R. left   = (( PSWP) mp1) -> x;
            ev. gen. R. bottom = (( PSWP) mp1) -> y;
            p = frame2client( self, ( Point){
               (( PSWP) mp1) -> cx,
               (( PSWP) mp1) -> cy
            }, true);
            dPoint = ( Point){
               (( PSWP) mp1) -> cx - p. x,
               (( PSWP) mp1) -> cy - p. y
            };
            ev. gen. R. right = p. x;
            ev. gen. R. top   = p. y;
         }
         if ( fl & SWP_MINIMIZE)
         {
            sys s. window. lastFrameSize  = ( Point){(( PSWP) mp1) -> cx, (( PSWP) mp1) -> cy};
            sys s. window. lastClientSize = frame2client( self,
               sys s. window. lastFrameSize, true );
         }
      }
      break;
      case WM_CHAR:
         WinSendMsg( v-> handle, msg, mp1, mp2);
         break;
      case WM_HITTEST:
         if ( !guts. focSysDisabled && ( Application_map_focus( application, self) != self))
            return ( MRESULT) HT_ERROR;
         break;
      case WM_DLGENTERMODAL:
         WinSendMsg( win, WM_ACTIVATE, MPFROMLONG( 1), 0);
         ev. cmd = cmExecute;
         break;
      case WM_FOCUSCHANGE:
         if ( !guts. focSysDisabled && !guts. focSysGranted) {
            if ( SHORT1FROMMP( mp2)) {
               Handle hf = Application_map_focus( application, self);
               if ( hf && hf != self) {
                  WinPostMsg( win, WM_FORCEFOCUS, ( MPARAM) (( PWidget) hf)-> handle, ( MPARAM) hf);
                  return ( MPARAM) 0;
               }
            } else if ( PWindow( self)-> modal) {
               PID xpid;
               TID xtid;
               WinQueryWindowProcess(( HWND) mp1, &xpid, &xtid);
               if ( xpid != guts. pid) return 0;
            }
            hiStage = true;
         }
         break;
      case WM_WINDOWPOSCHANGED:
      {
          PSWP new = PVOIDFROMMP( mp1);
          PSWP old = new + 1;
          #define wspush(d)                        \
          if ( v)                                  \
          {                                        \
             Event xev;                            \
             xev. cmd        = cmWindowState;      \
             xev. gen.source = self;               \
             xev. gen. i     = d;                  \
             v-> self-> message( self, &xev);      \
          }

          if ( new-> fl & SWP_ZORDER)
              WinPostMsg( client, WM_ZORDERSYNC, 0, 0);

          if ( new-> fl & ( SWP_SIZE | SWP_MAXIMIZE | SWP_RESTORE | SWP_HIDE))
          {
             Point pOld = frame2client( self, ( Point){old->cx, old->cy}, true);
             Point pNew = frame2client( self, ( Point){new->cx, new->cy}, true);
             ev. gen. R. left   = pOld. x;
             ev. gen. R. bottom = pOld. y;
             ev. gen. P. x = ev. gen. R. right = pNew. x;
             ev. gen. P. y = ev. gen. R. top   = pNew. y;
             ev. cmd = cmSize;
             if (( sys sizeLockLevel == 0) && ( v-> stage <= csNormal))
                v-> virtualSize = ev. gen. P;
             hiStage = true;
          }
          if (( new-> fl & SWP_RESTORE) && ( sys s. window. state == wsMinimized))
          {
              Point p = frame2client( self, sys s. window. hiddenSize, false);
              wspush( wsNormal);
              sys s. window. state = wsNormal;
              WinSetWindowPos( win, 0,
                sys s. window. hiddenPos. x, sys s. window. hiddenPos. y,
                p.x, p.y, SWP_SIZE|SWP_MOVE
              );
          }
          if (( new-> fl & SWP_RESTORE) && ( sys s. window. state == wsMaximized))
          {
             wspush( wsNormal);
             sys s. window. state = wsNormal;
          }
          if (( new-> fl & SWP_MINIMIZE) && !( old-> fl & SWP_MINIMIZE))
          {
             sys s. window. hiddenPos.  x = old-> x;
             sys s. window. hiddenPos.  y = old-> y;
             sys s. window. hiddenSize = sys s. window. lastClientSize;
             wspush( wsMinimized);
             sys s. window. state = wsMinimized;
          }
          if (( new-> fl & SWP_MAXIMIZE) && !( old-> fl & SWP_MAXIMIZE))
          {
             wspush( wsMaximized);
             sys s. window. state = wsMaximized;
          }
          if ( new-> fl & ( SWP_SHOW | SWP_HIDE))
             WinSendMsg( client, WM_SHOW, MPFROMLONG((new-> fl & SWP_SHOW)!=0), 0);
      }
      break;
   }

   if ( hiStage)
      toReturn = fProc ? fProc( win, msg, mp1, mp2) : WinDefWindowProc( win, msg, mp1, mp2);

   if (( ev. cmd != 0) && ( v != nil))
   {
      v-> self-> message( self, &ev);
   }

   if ( v-> stage <= csDestroying)
   switch ( msg)
   {
      case WM_ADJUSTWINDOWPOS:
         if ( ev. cmd)
         {
            (( PSWP) mp1)-> x  = ev. gen. R. left;
            (( PSWP) mp1)-> y  = ev. gen. R. bottom;
            (( PSWP) mp1)-> cx = ev. gen. R. right + dPoint. x;
            (( PSWP) mp1)-> cy = ev. gen. R. top   + dPoint. y;
          }
          break;
      case WM_FOCUSCHANGE:
          break;
      case WM_WINDOWPOSCHANGED:
          {
             PSWP new = PVOIDFROMMP( mp1);
             if ( is_apt( aptSyncPaint) && ( new-> fl & ( SWP_MAXIMIZE|SWP_RESTORE)))
               apc_widget_invalidate_rect( self, nil);
          }
          break;
   }
   if ( !WinIsWindow( guts. anchor, win)) return 0;
   if ( !hiStage)
      toReturn = fProc ? fProc( win, msg, mp1, mp2) : WinDefWindowProc( win, msg, mp1, mp2);
   return toReturn;
}

static Bool
locate_accel( PAbstractMenu menu, PMenuItemReg m, int * key)
{
   return menu-> self-> translate_accel(( Handle) menu, m-> text) == *key;
}

static Bool
find_oid( PAbstractMenu menu, PMenuItemReg m, int id)
{
   return m-> down && ( m-> down-> id == id);
}


MRESULT EXPENTRY
generic_menu_handler( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   PMenuWndData md = ( PMenuWndData) WinQueryWindowULong( win, QWL_USER);
   MPARAM ret;
   if (( md  == nil) || ( md-> menu == nilHandle)) return WinDefWindowProc( win, msg, mp1, mp2);
   if ( !appDead)
   switch ( msg)
   {
   case WM_CHAR:
      if (( SHORT1FROMMP( mp1) & KC_KEYUP) == 0)
      {
         int code, key, flags = SHORT1FROMMP( mp1), mod = 0;
         code = SHORT1FROMMP( mp2);
         key  = ctx_remap_def( SHORT2FROMMP( mp2), ctx_kb2VK, false, kbNoKey);
         if  ( flags & KC_SHIFT) { mod |= kmShift; }
         if  ( flags & KC_ALT  ) { mod |= kmAlt;   }
         if  ( flags & KC_CTRL ) { mod |= kmCtrl;  }
         switch ( SHORT2FROMMP( mp2))
         {
            case VK_ESC: case VK_SPACE: case VK_END:  case VK_HOME:  case VK_LEFT:
            case VK_UP:  case VK_RIGHT: case VK_DOWN: case VK_ENTER:
               break;
            default:
            {
               PAbstractMenu menu = ( PAbstractMenu)( md-> menu);
               PWidget owner = ( PWidget)( menu-> owner);
               int xkey = menu-> self-> translate_key(( Handle) menu, key, code, mod);
               if ( menu-> self-> first_that(( Handle) menu, locate_accel, &xkey, false))
                  break; // suspect that menu shortcut found. Do not touch it.
               if ( menu-> self-> sub_call_key(( Handle) menu, xkey)    ||
                    owner-> self-> process_accel(( Handle) owner, xkey))
               {
                  WinSendMsg( menu-> handle, MM_ENDMENUMODE, MPFROMLONG( true), 0);
                  return ( MPARAM)1;
               }
            }
         }
      }
      break;
   case WM_DESTROY:
      {
         PFNWP f = md-> fnwp;
         WinSetWindowULong( win, QWL_USER, 0);
         free( md);
         if ( !WinIsWindow( guts. anchor, win)) return 0;
         return f ? f( win, msg, mp1, mp2) : WinDefWindowProc( win, msg, mp1, mp2);
      }
      break;
   case WM_INITMENU:
      {
         PMenuItemReg m;
         PMenuWndData mwd = ( PMenuWndData) WinQueryWindowULong(( HWND) mp2, QWL_USER);
         if ( mwd) {
            PComponent owner = ( PComponent)(( PComponent)( md-> menu))-> owner;
            m = AbstractMenu_first_that( mwd-> menu, find_oid, (void*)mwd->id, true);
            if ( m)
               WinSendMsg( owner-> handle, WM_ACTIVATEMENU, MPFROMLONG( md-> menu), ( MPARAM) m-> id);
            else
               WinSendMsg( owner-> handle, WM_ACTIVATEMENU, MPFROMLONG( md-> menu), ( MPARAM) 0);
         }
      }
      break;
   case WM_PRESPARAMCHANGED:
      switch ( LONGFROMMP( mp1))
      {
         int i;
         case PP_MENUFOREGROUNDCOLOR:
         case PP_MENUFOREGROUNDCOLORINDEX:
            i = ciFore; goto SETCMD;
         case PP_MENUBACKGROUNDCOLOR:
         case PP_MENUBACKGROUNDCOLORINDEX:
            i = ciBack; goto SETCMD;
         case PP_MENUHILITEFGNDCOLOR:
         case PP_MENUHILITEFGNDCOLORINDEX:
            i = ciHiliteText; goto SETCMD;
         case PP_MENUHILITEBGNDCOLOR:
         case PP_MENUHILITEBGNDCOLORINDEX:
            i = ciHilite; goto SETCMD;
         case PP_MENUDISABLEDFGNDCOLOR:
         case PP_MENUDISABLEDFGNDCOLORINDEX:
            i = ciDisabledText; goto SETCMD;
         case PP_MENUDISABLEDBGNDCOLOR:
         case PP_MENUDISABLEDBGNDCOLORINDEX:
            i = ciDisabled;
         SETCMD:
            {
               PComponent owner = ( PComponent)(( PComponent)( md-> menu))-> owner;
               WinSendMsg( owner-> handle,
                  WM_MENUCOLORCHANGED, MPFROMSHORT(i), MPFROMLONG( md-> menu));
            }
            break;
         case PP_FONTNAMESIZE:
         case PP_FONTHANDLE:
            {
               PComponent owner = ( PComponent)(( PComponent)( md-> menu))-> owner;
               WinSendMsg( owner-> handle,
                  WM_PRESPARAMMENU, MPFROMLONG( cmFontChanged), MPFROMLONG( md-> menu));
            }
            break;
      }
      break;
   }
   if ( !WinIsWindow( guts. anchor, win)) return 0;
   ret = md-> fnwp ? md-> fnwp( win, msg, mp1, mp2) : WinDefWindowProc( win, msg, mp1, mp2);
   return ret;
}
