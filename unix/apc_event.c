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

/***********************************************************/
/*                                                         */
/*  System dependent event management (unix, x11)          */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

static Handle
xw2h( XWindow win)
/*
    tries to map X window to Prima's native handle
 */
{
   return (Handle)hash_fetch( guts.windows, (void*)&win, sizeof(win));
}

extern Bool appDead;

static void
handle_key_event( Handle self, XKeyEvent *ev, Event *e, Bool release)
{
   /* if e-> cmd is unset on exit, the event will not be passed to the system-independent part */
   char str_buf[ 256];
   KeySym keysym;
   U32 keycode;
   int str_len;

   str_len = XLookupString( ev, str_buf, 256, &keysym, nil);

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
      if ( str_len == 1)
	 keycode = (U8)*str_buf;
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

void
handle_event( XEvent *ev, XEvent *next_event)
{
   XWindow win, w2;
   Handle self, h2;
   Bool wasSent;
   Bool disabled;
   Event e, secondary;
   PDrawableSysData selfxx;
   XButtonEvent *bev;

   if ( appDead)
      return;

   (void)h2;(void)self;(void)w2;(void)win;(void)xw2h;

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
   if ( next_event 
	&& next_event-> type == ev-> type 
	&& ev-> type == MotionNotify
	&& win == next_event-> xany. window)
   {
      guts. skipped_events++;
      return;
   }

   self = xw2h( win);
   if (!self)
      return;
   e. gen. source = self;
   XX = X(self);
   disabled = !XX-> flags. enabled;

   wasSent = ev-> xany. send_event;

   switch ( ev-> type) {
   case KeyPress: {
      if (disabled) return;
      handle_key_event( self, &ev-> xkey, &e, false);
      break;
   }
   case KeyRelease: {
      if (disabled) return;
      handle_key_event( self, &ev-> xkey, &e, true);
      break;
   }
   case ButtonPress: {
      if (disabled) return;
      bev = &ev-> xbutton;
      e. cmd = cmMouseDown;
     ButtonEvent:
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
      e. pos. mod = 0;
      if ( e. cmd == cmMouseDown
	   && (( guts. mouse_wheel_up != 0 && bev-> button == guts. mouse_wheel_up)
	       || ( guts. mouse_wheel_down != 0 && bev-> button == guts. mouse_wheel_down)))
      {
	 secondary. cmd = cmMouseWheel;
	 secondary. pos. where. x = e. pos. where. x;
	 secondary. pos. where. y = e. pos. where. y;
	 secondary. pos. mod = e. pos. mod;
	 secondary. pos. button = bev-> button == guts. mouse_wheel_up ? WHEEL_DELTA : -WHEEL_DELTA;
      }
      break;
   }
   case ButtonRelease: {
      if (disabled) return;
      bev = &ev-> xbutton;
      e. cmd = cmMouseUp;
      goto ButtonEvent;
   }
   case MotionNotify: {
      if (disabled) return;
      e. cmd = cmMouseMove;
      e. pos. where. x = ev-> xmotion. x;
      e. pos. where. y = XX-> size. y - ev-> xmotion. y - 1;
      break;
   }
   case EnterNotify: {
      if (disabled) {
	 return;
      }
      e. cmd = cmMouseEnter;
     CrossingEvent:
      e. pos. where. x = ev-> xcrossing. x;
      e. pos. where. y = XX-> size. y - ev-> xcrossing. y - 1;
      break;
   }
   case LeaveNotify: {
      if (disabled) {
	 return;
      }
      e. cmd = cmMouseLeave;
      goto CrossingEvent;
   }
   case FocusIn: {
      switch ( ev-> xfocus. detail) {
      case NotifyVirtual:
      case NotifyNonlinearVirtual: /* XXX ? */
      case NotifyPointer:
      case NotifyPointerRoot: /* XXX ? */
      case NotifyDetailNone: /* XXX ? */
	 return;
      }
      XX-> flags. focused = 1;
      guts. focused = self;
      e. cmd = cmReceiveFocus;
      DOLBUG( "~~~~~~~~~ receive focus to %s, mode: %d, sent: %d, detail: %d\n",
	      PWidget(self)-> name,
	      ev-> xfocus. mode,
	      ev-> xfocus. send_event,
	      ev-> xfocus. detail);
      break;
   }
   case FocusOut: {
      switch ( ev-> xfocus. detail) {
      case NotifyVirtual:
      case NotifyNonlinearVirtual: /* XXX ? */
      case NotifyPointer:
      case NotifyPointerRoot: /* XXX ? */
      case NotifyDetailNone: /* XXX ? */
	 return;
      }
      XX-> flags. focused = 0;
      guts. focused = nilHandle;
      DOLBUG( "~~~~~~~~~ release focus of %s, mode: %d, sent: %d, detail: %d\n",
	      PWidget(self)-> name,
	      ev-> xfocus. mode,
	      ev-> xfocus. send_event,
	      ev-> xfocus. detail);
      e. cmd = cmReleaseFocus;
      break;
   }
   case KeymapNotify: {
      if (disabled) return;
      break;
   }
   case Expose: {
      XRectangle r;
      PPaintList pl;

      if ( !wasSent) {
	 r. x = ev-> xexpose. x;
	 r. y = ev-> xexpose. y;
	 r. width = ev-> xexpose. width;
	 r. height = ev-> xexpose. height;
	 if ( !XX-> region) {
	    XX-> region = XCreateRegion();
	 }
	 XUnionRectWithRegion( &r, XX-> region, XX-> region);
      }

      if ( ev-> xexpose. count)
	 return;

      pl = guts. paint_list;
      while ( pl) {
	 if ( pl-> obj == self)
	    return;
	 pl = pl-> next;
      }
      pl = malloc( sizeof( PaintList));
      if (!pl) croak( "no memory");
      pl-> obj = self;
      pl-> next = guts. paint_list;
      guts. paint_list = pl;
      return;
   }
   case GraphicsExpose: {
      DOLBUG( "********* Graphics Exposing ********* %s (%d,%d) *********\n",
	      PWidget(self)-> text, X(self)-> size. x, X(self)-> size. y);
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
      break;
   }
   case MapNotify: {
      XX-> flags. mapped = true;
      break;
   }
   case MapRequest: {
      break;
   }

   case ReparentNotify: {
      XWindow p = ev-> xreparent. parent;
      if ( !xw2h( p))
	 XX-> realParent = p;
      DOLBUG( "ReparentNotify\n");
      return;
   }

   case ConfigureNotify: {
      XConfigureEvent *cev = &ev-> xconfigure;
      Point oldSize = XX-> knownSize;
      Point oldOrigin = XX-> knownOrigin;
      Bool sizeChanged;

      if ( XX-> flags. noSize && oldSize.x == oldSize.y && oldSize. x == APC_BAD_SIZE) {
	 XX-> knownSize. x = XX-> size. x;
	 XX-> knownSize. y = XX-> size. y;
	 XX-> knownOrigin. x = XX-> origin. x;
	 XX-> knownOrigin. y = XX-> origin. y;
	 return;
      }

/*       if ( oldSize.x == oldSize.y && oldSize.x == APC_BAD_SIZE) */
/* 	 oldSize = XX-> size; */

/*       if ( oldOrigin.x == oldOrigin.y && oldOrigin.x == APC_BAD_ORIGIN) */
/* 	 oldOrigin = XX-> origin; */

      XX-> size. x = cev-> width;
      XX-> size. y = cev-> height;
      sizeChanged = XX-> flags. noSize
	 || XX-> size. x != XX-> knownSize. x
	 || XX-> size. y != XX-> knownSize. y;

      if ( XX-> realParent && !wasSent) {
	 XWindow cld;
	 if ( !XTranslateCoordinates( DISP, XX-> realParent, XX-> parent, 
				      cev-> x, cev-> y, 
				      &cev-> x, &cev-> y, &cld)) {
	     /* I don't expect this error to occur, ever */
	    croak( "APC_EVENT internal error at line %d", __LINE__);
	 }
	 XCHECKPOINT;
      }
      XX-> origin. x = cev-> x;
      XX-> origin. y = X(XX-> owner)-> size. y - XX-> size. y - cev-> y;

      if ( XX-> flags. noSize
	   || XX-> origin. x != XX-> knownOrigin. x
	   || XX-> origin. y != XX-> knownOrigin. y) {
	 /* move notification */
	 e. cmd = cmMove;
	 e. gen. P = oldOrigin;
	 CComponent( self)-> message( self, &e);
      }

      XX-> flags. noSize = false;

      if ( sizeChanged) {
	 /* size notification */
	 e. gen. source = self;
	 e. cmd = cmSize;
	 e. gen. R. left = oldSize. x;
	 e. gen. R. bottom = oldSize. y;
	 e. gen. P = XX-> size;
	 e. gen. R. right = XX-> size. x;
	 e. gen. R. top = XX-> size. y;
	 {
	    int count = PWidget( self)-> widgets. count;
	    Handle *selves = malloc( count * sizeof( Handle));
	    int i, stage;

	    memcpy( selves, PWidget( self)-> widgets. items, count * sizeof( Handle));
	    for ( i = 0; i < count; i++) {
	       PWidget child = PWidget( selves[i]);

	       if ( X(selves[i])-> flags. clipOwner && (child-> growMode & gmDontCare) == 0) {
		  stage = child-> stage;
		  child-> stage = csFrozen;
		  apc_widget_set_pos( selves[i], X(selves[i])-> origin. x, X(selves[i])-> origin. y);
		  child-> stage = stage;
	       }
	    }
	    free( selves);
	 }
	 DOLBUG( "old size of %s: %dx%d, new size: %dx%d\n", 
		  PComponent( self)-> name, 
		  oldSize. x, oldSize. y, 
		  XX-> size. x, XX-> size. y);
	 CComponent( self)-> message( self, &e);
      }

      XX-> knownOrigin = XX-> origin;
      XX-> knownSize = XX-> size;

      return;
   }
   case ConfigureRequest: {
      break;
   }
   case GravityNotify: {
      DOLBUG( "!!!!!!!! GravityNotify: %08lx  %08lx -- %d %d !!!!!!!!\n",
	      ev-> xgravity. window,
	      ev-> xgravity. event,
	      ev-> xgravity. x,
	      ev-> xgravity. y);
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
      char *c;
      DOLBUG( "!!!!!!!! PropertyNotify: %s !!!!!!!!\n",
	      c = XGetAtomName( DISP, ev-> xproperty. atom));
      if (c) XFree(c);
      break;
   }
   case SelectionClear: {
      break;
   }
   case SelectionRequest: {
      break;
   }
   case SelectionNotify: {
      break;
   }
   case ColormapNotify: {
      break;
   }
   case ClientMessage: {
      if ( guts. wm_translate_event)
	 guts. wm_translate_event( self, ev, &e);
      break;
   }
   case MappingNotify: {
      break;
   }
   }

   if ( e. cmd) {
      guts. handled_events++;
      CComponent( self)-> message( self, &e);
      if ( secondary. cmd) {
	 CComponent( self)-> message( self, &secondary);
      }
   } else {
      /* Unhandled event, just do nothing */
      DOLBUG( "*** event %u to %s ***\n", ev-> type, PWidget( self)-> name);
      guts. unhandled_events++;
   }
}
