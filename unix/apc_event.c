/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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

/***********************************************************/
/*                                                         */
/*  System dependent event management (unix, x11)          */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "AbstractMenu.h"
#include "Application.h"
#include "Window.h"
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

void
prima_send_create_event( XWindow win)
{
   XClientMessageEvent ev;

   bzero( &ev, sizeof(ev));
   ev. type = ClientMessage;
   ev. display = DISP;
   ev. window = win;
   ev. message_type = guts. create_event;
   ev. format = 32;
   ev. data. l[0] = 0;
   XSendEvent( DISP, win, false, 0, (XEvent*)&ev);
   XCHECKPOINT;
}

Handle
prima_xw2h( XWindow win)
/*
    tries to map X window to Prima's native handle
 */
{
   Handle self;
   self = (Handle)hash_fetch( guts.windows, (void*)&win, sizeof(win));
   if (!self)
      self = (Handle)hash_fetch( guts.menu_windows, (void*)&win, sizeof(win));
   return self;
}

extern Bool appDead;

static void
handle_key_event( Handle self, XKeyEvent *ev, Event *e, Bool release)
{
   /* if e-> cmd is unset on exit, the event will not be passed to the system-independent part */
   char str_buf[ 256];
   KeySym keysym, keysym2;
   U32 keycode;
   int str_len;

   str_len = XLookupString( ev, str_buf, 256, &keysym, nil);
   /*
   fprintf( stderr, "keysym: %08lu 0: %08lu, 1: %08lu, 2: %08lu, 3: %08lu\n", keysym,
            XLookupKeysym(ev,0),
            XLookupKeysym(ev,1),
            XLookupKeysym(ev,2),
            XLookupKeysym(ev,3));
    */

   switch (keysym) {
   /* virtual keys-modifiers */
   case XK_Shift_L:		keycode = kbShiftL;		break;
   case XK_Shift_R:		keycode = kbShiftR;		break;
   case XK_Control_L:		keycode = kbCtrlL;		break;
   case XK_Control_R:		keycode = kbCtrlR;		break;
   case XK_Alt_L:		keycode = kbAltL;		break;
   case XK_Alt_R:		keycode = kbAltR;		break;
   case XK_Meta_L:		keycode = kbMetaL;		break;
   case XK_Meta_R:		keycode = kbMetaR;		break;
   case XK_Super_L:		keycode = kbSuperL;		break;
   case XK_Super_R:		keycode = kbSuperR;		break;
   case XK_Hyper_L:		keycode = kbHyperL;		break;
   case XK_Hyper_R:		keycode = kbHyperR;		break;
   case XK_Caps_Lock:		keycode = kbCapsLock;		break;
   case XK_Num_Lock:		keycode = kbNumLock;		break;
   case XK_Scroll_Lock:		keycode = kbScrollLock;		break;
   case XK_Shift_Lock:		keycode = kbShiftLock;		break;
   /* virtual keys with charcode */
   case XK_BackSpace:		keycode = kbBackspace;		break;
   case XK_Tab:			keycode = kbTab;		break;
   case XK_KP_Tab:		keycode = kbKPTab;		break;
   case XK_Linefeed:		keycode = kbLinefeed;		break;
   case XK_Return:		keycode = kbEnter;		break;
   case XK_KP_Enter:		keycode = kbKPEnter;		break;
   case XK_Escape:		keycode = kbEscape;		break;
   case XK_KP_Space:		keycode = kbKPSpace;		break;
   case XK_KP_Equal:		keycode = kbKPEqual;		break;
   case XK_KP_Multiply:		keycode = kbKPMultiply;		break;
   case XK_KP_Add:		keycode = kbKPAdd;		break;
   case XK_KP_Separator:	keycode = kbKPSeparator;	break;
   case XK_KP_Subtract:		keycode = kbKPSubtract;		break;
   case XK_KP_Decimal:		keycode = kbKPDecimal;		break;
   case XK_KP_Divide:		keycode = kbKPDivide;		break;
   case XK_KP_0:		keycode = kbKP0;		break;
   case XK_KP_1:		keycode = kbKP1;		break;
   case XK_KP_2:		keycode = kbKP2;		break;
   case XK_KP_3:		keycode = kbKP3;		break;
   case XK_KP_4:		keycode = kbKP4;		break;
   case XK_KP_5:		keycode = kbKP5;		break;
   case XK_KP_6:		keycode = kbKP6;		break;
   case XK_KP_7:		keycode = kbKP7;		break;
   case XK_KP_8:		keycode = kbKP8;		break;
   case XK_KP_9:		keycode = kbKP9;		break;
   /* Other virtual keys */
   case XK_Clear:		keycode = kbClear;		break;
   case XK_Pause:		keycode = kbPause;		break;
   case XK_Sys_Req:		keycode = kbSysReq;		break;
   case XK_Delete:		keycode = kbDelete;		break;
   case XK_KP_Delete:		keycode = kbKPDelete;		break;
   case XK_Home:		keycode = kbHome;		break;
   case XK_KP_Home:		keycode = kbKPHome;		break;
   case XK_Left:		keycode = kbLeft;		break;
   case XK_KP_Left:		keycode = kbKPLeft;		break;
   case XK_Up:			keycode = kbUp;			break;
   case XK_KP_Up:		keycode = kbKPUp;		break;
   case XK_Right:		keycode = kbRight;		break;
   case XK_KP_Right:		keycode = kbKPRight;		break;
   case XK_Down:		keycode = kbDown;		break;
   case XK_KP_Down:		keycode = kbKPDown;		break;
   case XK_Prior:		keycode = kbPrior;		break;
   case XK_KP_Prior:		keycode = kbKPPrior;		break;
   case XK_Next:		keycode = kbNext;		break;
   case XK_KP_Next:		keycode = kbKPNext;		break;
   case XK_End:			keycode = kbEnd;		break;
   case XK_KP_End:		keycode = kbKPEnd;		break;
   case XK_Begin:		keycode = kbBegin;		break;
   case XK_KP_Begin:		keycode = kbKPBegin;		break;
   case XK_Select:		keycode = kbSelect;		break;
   case XK_Print:		keycode = kbPrint;		break;
   case XK_Execute:		keycode = kbExecute;		break;
   case XK_Insert:		keycode = kbInsert;		break;
   case XK_KP_Insert:		keycode = kbKPInsert;		break;
   case XK_Undo:		keycode = kbUndo;		break;
   case XK_Redo:		keycode = kbRedo;		break;
   case XK_Menu:		keycode = kbMenu;		break;
   case XK_Find:		keycode = kbFind;		break;
   case XK_Cancel:		keycode = kbCancel;		break;
   case XK_Help:		keycode = kbHelp;		break;
   case XK_Break:		keycode = kbBreak;		break;
   case XK_ISO_Left_Tab:	keycode = kbBackTab;		break;
   /* Virtual function keys */
   case XK_F1:			keycode = kbF1;			break;
   case XK_KP_F1:		keycode = kbKPF1;		break;
   case XK_F2:			keycode = kbF2;			break;
   case XK_KP_F2:		keycode = kbKPF2;		break;
   case XK_F3:			keycode = kbF3;			break;
   case XK_KP_F3:		keycode = kbKPF3;		break;
   case XK_F4:			keycode = kbF4;			break;
   case XK_KP_F4:		keycode = kbKPF4;		break;
   case XK_F5:			keycode = kbF5;			break;
   case XK_F6:			keycode = kbF6;			break;
   case XK_F7:			keycode = kbF7;			break;
   case XK_F8:			keycode = kbF8;			break;
   case XK_F9:			keycode = kbF9;			break;
   case XK_F10:			keycode = kbF10;		break;
   case XK_F11:			keycode = kbF11;		break;
   case XK_F12:			keycode = kbF12;		break;
   case XK_F13:			keycode = kbF13;		break;
   case XK_F14:			keycode = kbF14;		break;
   case XK_F15:			keycode = kbF15;		break;
   case XK_F16:			keycode = kbF16;		break;
   case XK_F17:			keycode = kbF17;		break;
   case XK_F18:			keycode = kbF18;		break;
   case XK_F19:			keycode = kbF19;		break;
   case XK_F20:			keycode = kbF20;		break;
   case XK_F21:			keycode = kbF21;		break;
   case XK_F22:			keycode = kbF22;		break;
   case XK_F23:			keycode = kbF23;		break;
   case XK_F24:			keycode = kbF24;		break;
   case XK_F25:			keycode = kbF25;		break;
   case XK_F26:			keycode = kbF26;		break;
   case XK_F27:			keycode = kbF27;		break;
   case XK_F28:			keycode = kbF28;		break;
   case XK_F29:			keycode = kbF29;		break;
   case XK_F30:			keycode = kbF30;		break;
   default:			keycode = kbNoKey;
   }

   if ( str_len == 1 && keycode == kbNoKey && *str_buf == ' ')
      keycode = kbSpace;
   if ( keycode == kbNoKey) {
      if ( keysym <= 0x0000007f && !isalpha(keysym & 0x000000ff))
	 keycode = keysym & 0x000000ff;
      else if (( ev-> state & ControlMask) && keysym > 0x0000007f
               && (keysym2 = XLookupKeysym( ev, 0)) <= 0x0000007f
               && isalpha(keysym2 & 0x0000007f))
         keycode = toupper(keysym2 & 0x0000007f) - '@';
      else if ( str_len == 1)
	 keycode = (uint8_t)*str_buf;
      else if ( keysym < 0xFD00)
	 keycode = keysym & 0x000000ff;
      else
	 return; /* don't generate an event */
   }
   if (( keycode & kbCodeCharMask) == kbCodeCharMask)
      keycode |= (keycode >> 8) & 0xFF;
   e-> cmd = release ? cmKeyUp : cmKeyDown;
   e-> key. key = keycode & kbCodeMask;
   if ( !e-> key. key)		e-> key. key = kbNoKey;
   e-> key. code = keycode & kbCharMask;
   e-> key. mod = keycode & kbModMask;
   e-> key. repeat = 1;
   /* ShiftMask LockMask ControlMask Mod1Mask Mod2Mask Mod3Mask Mod4Mask Mod5Mask */
   if ( ev-> state & ShiftMask)		e-> key. mod |= kmShift;
   if ( ev-> state & ControlMask)	e-> key. mod |= kmCtrl;
   if ( ev-> state & Mod1Mask)		e-> key. mod |= kmAlt;
}

static Bool
input_disabled( PDrawableSysData XX, Bool ignore_horizon)
{
   Handle horizon = application;

   if ( guts. message_boxes) return true;
   if ( guts. modal_count > 0 && !ignore_horizon)
      horizon = CApplication(application)-> map_focus( application, XX-> self);
   while (XX->self && XX-> self != horizon && XX-> self != application) {
      if (!XF_ENABLED(XX)) return true;
      XX = X(PWidget(XX->self)->owner);
   }
   return XX->self && XX-> self != horizon;
}

static Bool
no_input( PDrawableSysData XX, Bool ignore_horizon, Bool beep)
{
   if ( input_disabled(XX, ignore_horizon)) {
      if ( beep) {
         apc_beep( mbWarning);
      }
      return true;
   }
   return false;
}

/*
static char * xevdefs[] = { "0", "1"
,"KeyPress" ,"KeyRelease" ,"ButtonPress" ,"ButtonRelease" ,"MotionNotify" ,"EnterNotify"
,"LeaveNotify" ,"FocusIn" ,"FocusOut" ,"KeymapNotify" ,"Expose" ,"GraphicsExpose"
,"NoExpose" ,"VisibilityNotify" ,"CreateNotify" ,"DestroyNotify" ,"UnmapNotify"
,"MapNotify" ,"MapRequest" ,"ReparentNotify" ,"ConfigureNotify" ,"ConfigureRequest"
,"GravityNotify" ,"ResizeRequest" ,"CirculateNotify" ,"CirculateRequest" ,"PropertyNotify"
,"SelectionClear" ,"SelectionRequest" ,"SelectionNotify" ,"ColormapNotify" ,"ClientMessage"
,"MappingNotify"};
*/

void
prima_handle_event( XEvent *ev, XEvent *next_event)
{
   XWindow win;
   Handle self;
   Bool was_sent;
   Event e, secondary;
   PDrawableSysData selfxx;
   XButtonEvent *bev;
   int cmd;

   XCHECKPOINT;
   if ( ev-> type == guts. shared_image_completion_event) {
      prima_ximage_event( ev);
      return;
   }


   if ( guts. message_boxes) {
      struct MsgDlg * md = guts. message_boxes;
      XWindow win = ev-> xany. window;
      while ( md) {
         if ( md-> w == win) {
            prima_msgdlg_event( ev, md);
            return;
         }   
         md = md-> next;
      }   
   }   

   if ( appDead)
      return;

   bzero( &e, sizeof( e));
   bzero( &secondary, sizeof( secondary));

   /* Get a window, including special cases */
   switch ( ev-> type) {
   case ConfigureNotify:
      win = ev-> xconfigure. window;
      break;
   case ReparentNotify:
      win = ev-> xreparent. window;
      break;
   case MapNotify:
      win = ev-> xmap. window;
      break;
   case UnmapNotify:
      win = ev-> xunmap. window;
      break;
   default:
      win = ev-> xany. window;
   }

   /* possibly skip this event */
   if ( next_event) {
      if (next_event-> type == ev-> type
          && ev-> type == MotionNotify
          && win == next_event-> xany. window) {
         guts. skipped_events++;
         return;
      } else if ( ev-> type == KeyRelease
                  && next_event-> type == KeyPress
                  && ev-> xkey. time == next_event-> xkey. time
                  && ev-> xkey. display == next_event-> xkey. display
                  && ev-> xkey. window == next_event-> xkey. window
                  && ev-> xkey. root == next_event-> xkey. root
                  && ev-> xkey. subwindow == next_event-> xkey. subwindow
                  && ev-> xkey. x == next_event-> xkey. x
                  && ev-> xkey. y == next_event-> xkey. y
                  && ev-> xkey. state == next_event-> xkey. state
                  && ev-> xkey. keycode == next_event-> xkey. keycode) {
         guts. skipped_events++;
         return;
      }
   }

   if ( win == guts. root && guts. grab_redirect) 
      win = guts. grab_redirect;

   self = prima_xw2h( win);
   /*
   printf("%d:%s of ", ev-> type, ((ev-> type >= LASTEvent) ? "?" : xevdefs[ev-> type]));
   printf( self ? "%s\n" : "%08x\n", self ? PWidget(self)-> name : self);
   */
   if (!self)
      return;
   if ( XT_IS_MENU(X(self))) {
      prima_handle_menu_event( ev, win, self);
      return;
   }
   e. gen. source = self;
   secondary. gen. source = self;
   XX = X(self);

   was_sent = ev-> xany. send_event;

   switch ( ev-> type) {
   case KeyPress: {
      guts. last_time = ev-> xkey. time;
      if (no_input(XX, false, true)) return;
      handle_key_event( self, &ev-> xkey, &e, false);
      break;
   }
   case KeyRelease: {
      guts. last_time = ev-> xkey. time;
      if (no_input(XX, false, false)) return;
      handle_key_event( self, &ev-> xkey, &e, true);
      break;
   }
   case ButtonPress: {
      guts. last_time = ev-> xbutton. time;
      if (no_input(XX, false, true)) return;
      bev = &ev-> xbutton;
      e. cmd = cmMouseDown;
     ButtonEvent:
      if ( ev-> xany. window == guts. root && guts. grab_redirect) {
         bev-> x -= guts. grab_translate_mouse. x;
         bev-> y -= guts. grab_translate_mouse. y;
      }      
      switch (bev-> button) {
      case Button1:
	 e. pos. button = mb1;
	 break;
      case Button2:
	 e. pos. button = mb2;
	 break;
      case Button3:
	 e. pos. button = mb3;
	 break;
      case Button4:
	 e. pos. button = mb4;
	 break;
      case Button5:
	 e. pos. button = mb5;
	 break;
      case Button6:
	 e. pos. button = mb6;
	 break;
      case Button7:
	 e. pos. button = mb7;
	 break;
      case Button8:
	 e. pos. button = mb8;
	 break;
      default:
	 return;
      }
      e. pos. where. x = bev-> x;
      e. pos. where. y = XX-> size. y - bev-> y - 1;
      if ( bev-> state & ShiftMask)     e.pos.mod |= kmShift;
      if ( bev-> state & ControlMask)   e.pos.mod |= kmCtrl;
      if ( bev-> state & Mod1Mask)      e.pos.mod |= kmAlt;
      if ( bev-> state & Button1Mask)   e.pos.mod |= mb1;
      if ( bev-> state & Button2Mask)   e.pos.mod |= mb2;
      if ( bev-> state & Button3Mask)   e.pos.mod |= mb3;
      if ( bev-> state & Button4Mask)   e.pos.mod |= mb4;
      if ( bev-> state & Button5Mask)   e.pos.mod |= mb5;
      if ( bev-> state & Button6Mask)   e.pos.mod |= mb6;
      if ( bev-> state & Button7Mask)   e.pos.mod |= mb7;
      if ( e. cmd == cmMouseDown
	   && (( guts. mouse_wheel_up != 0 && bev-> button == guts. mouse_wheel_up)
	       || ( guts. mouse_wheel_down != 0 && bev-> button == guts. mouse_wheel_down)))
      {
	 secondary. cmd = cmMouseWheel;
	 secondary. pos. where. x = e. pos. where. x;
	 secondary. pos. where. y = e. pos. where. y;
	 secondary. pos. mod = e. pos. mod;
	 secondary. pos. button = bev-> button == guts. mouse_wheel_up ? WHEEL_DELTA : -WHEEL_DELTA;
      } else if ( e. cmd == cmMouseDown && e. pos. button == mbRight) {
	 secondary. cmd = cmPopup;
         secondary. gen. B = true;
         secondary. gen. P. x = e. pos. where. x;
         secondary. gen. P. y = e. pos. where. y;
      } else if ( e. cmd == cmMouseUp &&
                  guts.last_button_event.type == ButtonPress &&
                  bev-> window == guts.last_button_event.window &&
                  bev-> button == guts.last_button_event.button &&
                  bev-> time - guts.last_button_event.time <= guts.click_time_frame) {
	 secondary. cmd = cmMouseClick;
	 secondary. pos. where. x = e. pos. where. x;
	 secondary. pos. where. y = e. pos. where. y;
	 secondary. pos. mod = e. pos. mod;
	 secondary. pos. button = e. pos. button;
         if (guts.last_click.window == bev->window &&
             guts.last_click.button == bev->button &&
             guts.last_button_event.time - guts.last_click.time <=
                guts.double_click_time_frame) {
            bzero( &guts.last_click, sizeof(guts.last_click));
            secondary.pos.dblclk = true;
         } else {
            memcpy( &guts.last_click, bev, sizeof(guts.last_click));
         }
      }
      memcpy( &guts.last_button_event, bev, sizeof(*bev));
      
      if ( e. cmd == cmMouseDown && !XX-> flags. first_click) {
         Handle x = self, f = guts. focused ? guts. focused : application;
         while ( !X(x)-> type. window && ( x != application)) x = (( PWidget) x)-> owner;
         while ( !X(f)-> type. window && ( f != application)) f = (( PWidget) f)-> owner;
         if ( x != f) {
            e. cmd = 0;
            if ((( PApplication) application)-> hintUnder == self) 
               CWidget(self)-> set_hintVisible( self, 0);
            if (( PWidget(self)-> options. optSelectable) && ( PWidget(self)-> selectingButtons & e. pos. button))
               apc_widget_set_focused( self);
         }
      }
      break;
   }
   case ButtonRelease: {
      guts. last_time = ev-> xbutton. time;
      if (no_input(XX, false, false)) return;
      bev = &ev-> xbutton;
      e. cmd = cmMouseUp;
      goto ButtonEvent;
   }
   case MotionNotify: {
      guts. last_time = ev-> xmotion. time;
      if (no_input(XX, true, false)) return;
      e. cmd = cmMouseMove;
      if ( ev-> xany. window == guts. root && guts. grab_redirect) {
         ev-> xmotion. x -= guts. grab_translate_mouse. x;
         ev-> xmotion. y -= guts. grab_translate_mouse. y;
      }      
      e. pos. where. x = ev-> xmotion. x;
      e. pos. where. y = XX-> size. y - ev-> xmotion. y - 1;
      if ( ev-> xmotion. state & ShiftMask)     e.pos.mod |= kmShift;
      if ( ev-> xmotion. state & ControlMask)   e.pos.mod |= kmCtrl;
      if ( ev-> xmotion. state & Mod1Mask)      e.pos.mod |= kmAlt;
      if ( ev-> xmotion. state & Button1Mask)   e.pos.mod |= mb1;
      if ( ev-> xmotion. state & Button2Mask)   e.pos.mod |= mb2;
      if ( ev-> xmotion. state & Button3Mask)   e.pos.mod |= mb3;
      if ( ev-> xmotion. state & Button4Mask)   e.pos.mod |= mb4;
      if ( ev-> xmotion. state & Button5Mask)   e.pos.mod |= mb5;
      if ( ev-> xmotion. state & Button6Mask)   e.pos.mod |= mb6;
      if ( ev-> xmotion. state & Button7Mask)   e.pos.mod |= mb7;
      break;
   }
   case EnterNotify: {
      if (( guts. pointer_invisible_count == 0) && XX-> flags. pointer_obscured) {
         XX-> flags. pointer_obscured = 0; 
         XDefineCursor( DISP, XX-> udrawable, 
           ( XX-> pointer_id == crUser) ? XX-> user_pointer : 
           XX-> actual_pointer);
      } else if (( guts. pointer_invisible_count < 0) && !XX-> flags. pointer_obscured) { 
         XX-> flags. pointer_obscured = 1;
         XDefineCursor( DISP, XX-> udrawable, guts. null_pointer);
      }   
      guts. last_time = ev-> xcrossing. time;
      if (no_input(XX, true, false)) {
	 return;
      }
      e. cmd = cmMouseEnter;
     CrossingEvent:
      if ( ev-> xcrossing. subwindow != None) return;
      e. pos. where. x = ev-> xcrossing. x;
      e. pos. where. y = XX-> size. y - ev-> xcrossing. y - 1;
      break;
   }
   case LeaveNotify: {
      guts. last_time = ev-> xcrossing. time;
      if (no_input(XX, true, false)) {
	 return;
      }
      e. cmd = cmMouseLeave;
      goto CrossingEvent;
   }
   case FocusIn: {
      Handle frame = self;
      switch ( ev-> xfocus. detail) {
      case NotifyVirtual:
      case NotifyPointer:
      case NotifyPointerRoot: 
      case NotifyDetailNone: 
	 return;
      case NotifyNonlinearVirtual: 
         if (!XT_IS_WINDOW(XX)) return;
         break;
      }

      if ( guts. message_boxes) {
         struct MsgDlg * md = guts. message_boxes;
         while ( md) {
            XSetInputFocus( DISP, md-> w, RevertToNone, CurrentTime); 
            md = md-> next;
         }   
         return;
      }   

      if (!XT_IS_WINDOW(XX))
         frame = CApplication(application)-> top_frame( application, self);
      if ( CApplication(application)-> map_focus( application, frame) != frame) {
         CApplication(application)-> popup_modal( application);
         break;
      }

      if ( XT_IS_WINDOW(XX)) {
	 e. cmd = cmActivate;
	 CComponent( self)-> message( self, &e);
         if (( PObject( self)-> stage == csDead) ||
             ( ev-> xfocus. detail == NotifyNonlinearVirtual)) return;
      }
      
      if ( guts. focused) prima_no_cursor( guts. focused);
      guts. focused = self;
      prima_update_cursor( guts. focused);
      e. cmd = cmReceiveFocus;
      break;
   }
   case FocusOut: {
      switch ( ev-> xfocus. detail) {
      case NotifyVirtual:
      case NotifyPointer:
      case NotifyPointerRoot: 
      case NotifyDetailNone: 
	 return;
      case NotifyNonlinearVirtual: 
         if (!XT_IS_WINDOW(XX)) return;
         break;
      }
      
      if ( XT_IS_WINDOW(XX)) {
	 e. cmd = cmDeactivate;
	 CComponent( self)-> message( self, &e);
         if (( PObject( self)-> stage == csDead) ||
             ( ev-> xfocus. detail == NotifyNonlinearVirtual)) return;
      }
      
      if ( guts. focused) prima_no_cursor( guts. focused);
      if ( self == guts. focused) guts. focused = nilHandle;
      e. cmd = cmReleaseFocus;
      break;
   }
   case KeymapNotify: {
      break;
   }
   case Expose: {
      XRectangle r;
      if ( !was_sent) {
	 r. x = ev-> xexpose. x;
	 r. y = ev-> xexpose. y;
	 r. width = ev-> xexpose. width;
	 r. height = ev-> xexpose. height;
	 if ( !XX-> region)
	    XX-> region = XCreateRegion();
	 XUnionRectWithRegion( &r, XX-> region, XX-> region);
      }

      if ( ev-> xexpose. count == 0 && !XX-> flags. paint_pending) {
         TAILQ_INSERT_TAIL( &guts.paintq, XX, paintq_link);
         XX-> flags. paint_pending = true;
      }
      return;
   }
   case GraphicsExpose: {
      break;
   }
   case NoExpose: {
      break;
   }
   case VisibilityNotify: {
      XX-> flags. exposed = ( ev-> xvisibility. state != VisibilityFullyObscured);
      break;
   }
   case CreateNotify: {
      break;
   }
   case DestroyNotify: {
      break;
   }
   case UnmapNotify: {
      XX-> flags. mapped = false;
      if ( XX-> type. window) {
         e. cmd = cmHide;
         if ( !XX-> flags. iconic) {
            secondary. cmd = cmWindowState;
            secondary. gen. i = wsMinimized;
            XX-> flags. iconic = 1;
         }   
      }   
      break;
   }
   case MapNotify: {
      XX-> flags. mapped = true;
      if ( XX-> type. window) {
         e. cmd = cmShow;
         if ( XX-> flags. iconic) {
            secondary. cmd = cmWindowState;
            secondary. gen. i = XX-> flags. zoomed ? wsMaximized : wsNormal;
            XX-> flags. iconic = 0;
         }   
         if ( XX-> flags. withdrawn)
            XX-> flags. withdrawn = 0;
      }   
      break;
   }
   case MapRequest: {
      break;
   }

   case ReparentNotify: {
      XWindow p = ev-> xreparent. parent;
      if ( !prima_xw2h( p)) 
	 XX-> real_parent = p;
      return;
   }

   case ConfigureNotify: {
      XConfigureEvent *cev = &ev-> xconfigure;
      Bool size_changed, pos_changed;
      Point old_size, old_pos;

      if ( cev-> above != XX-> above) {
	 /* z-order notification */
	 e. cmd = cmZOrderChanged;
	 CComponent( self)-> message( self, &e);
         if ( PObject( self)-> stage == csDead) return; 
      }   
      
      if ( !XX-> flags. process_configure_notify)
	 return;

      /* if ( XX-> real_parent != XX-> parent && !was_sent) {
	 XWindow cld;
	 XTranslateCoordinates( DISP, XX-> real_parent, XX-> parent,
            cev-> x, cev-> y, &cev-> x, &cev-> y, &cld);
	 XCHECKPOINT;
      } */
      size_changed = ( cev-> width != XX-> size. x) || ( cev-> height != XX-> size. y);
      pos_changed  = ( cev-> x != XX-> origin. x) || 
                     ( XX-> origin. y != X(XX-> owner)-> size. y - XX-> size. y - cev-> y);
      old_size  = XX-> size;
      old_pos   = XX-> origin;
      XX-> size   = ( Point) { cev-> width, cev-> height};
      XX-> origin = ( Point) { cev-> x, X(XX-> owner)-> size. y - XX-> size. y - cev-> y };
      
      if ( pos_changed) {
	 e. cmd = cmMove;
	 e. gen. P = XX-> origin;
	 CComponent( self)-> message( self, &e);
         if ( PObject( self)-> stage == csDead) return; 
      }

      if ( size_changed) {
         prima_send_cmSize( self, old_size);
         if ( PObject( self)-> stage == csDead) return; 
      }

      if ( size_changed && XX-> flags. want_visible) {
         int qx = guts. displaySize.x * 3 / 4, qy = guts. displaySize.y * 3 / 4;
         if ( !XX-> flags. zoomed) {
            if ( XX-> size. x > qx && XX-> size. y > qy) {
               e. cmd = cmWindowState;
               e. gen. i = wsMaximized;
               XX-> zoomRect = (Rect){old_pos.x, old_pos.y, old_size.x, old_size.y};
               XX-> flags. zoomed = 1;
            }   
         } else {
            if ( old_size.x > XX-> size.x && old_size.y > XX-> size.y) {
               e. cmd = cmWindowState;
               e. gen. i = wsNormal;
               XX-> flags. zoomed = 0;
            } else  
               XX-> zoomRect = (Rect){XX-> origin.x, XX-> origin.y, XX-> size.x, XX-> size.y};
         }   
         if ( e. cmd) CComponent( self)-> message( self, &e);
         if ( PObject( self)-> stage == csDead) return; 
      }   
      return;
   }
   case ConfigureRequest: {
      break;
   }
   case GravityNotify: {
      break;
   }
   case ResizeRequest: {
      break;
   }
   case CirculateNotify: {
      break;
   }
   case CirculateRequest: {
      break;
   }
   case PropertyNotify: {
      /* char *c;
      guts. last_time = ev-> xproperty. time;
      DOLBUG( "!!!!!!!! PropertyNotify: %s !!!!!!!!\n",
	      c = XGetAtomName( DISP, ev-> xproperty. atom));
      if (c) XFree(c); */
      break;
   }
   case SelectionClear: {
      guts. last_time = ev-> xselectionclear. time;
      break;
   }
   case SelectionRequest: {
      guts. last_time = ev-> xselectionrequest. time;
      break;
   }
   case SelectionNotify: {
      guts. last_time = ev-> xselection. time;
      break;
   }
   case ColormapNotify: {
      break;
   }
   case ClientMessage: {
      if ( ev-> xclient. message_type == guts. create_event) {
	 e. cmd = cmSetup;
      } else if ( guts. wm_translate_event) {
	 guts. wm_translate_event( self, ev, &e);
      }
      break;
   }
   case MappingNotify: {
      break;
   }
   }

   if ( e. cmd) {
      guts. handled_events++;
      cmd = e. cmd;
      CComponent( self)-> message( self, &e);
      if ( PObject( self)-> stage == csDead) return; 
      if ( e. cmd && cmd == cmClose) {
         if ( XX-> type. window && PWindow(self)-> modal)
            CWindow(self)-> cancel( self);
         else
            Object_destroy( self);
      }
      if ( secondary. cmd) {
	 CComponent( self)-> message( self, &secondary);
         if ( PObject( self)-> stage == csDead) return; 
      }
   } else {
      /* Unhandled event, do nothing */
      guts. unhandled_events++;
   }
}
