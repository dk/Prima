#include "unix/guts.h"

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  System dependent event management (unix, x11)          */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

static Handle
xw2h( XWindow win)
/*
    tries to map X window to Prima's native handle
 */
{
   return (Handle)hash_fetch( guts.windows, (void*)&win, sizeof(win));
}

extern Bool appDead;

void
handle_event( XEvent *ev, XEvent *next_event)
{
   XWindow win, w2;
   Handle self, h2;
   Bool wasSent;
   Bool disabled;
   Event e;
   PDrawableSysData selfxx;
   XButtonEvent *bev;

   if ( appDead)
      return;

   (void)h2;(void)self;(void)w2;(void)win;(void)xw2h;

   memset( &e, 0, sizeof( e));

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
      break;
   }
   case KeyRelease: {
      if (disabled) return;
      break;
   }
   case ButtonPress: {
      if (disabled) return;
      bev = &ev-> xbutton;
      e. cmd = cmMouseDown;
     ButtonEvent:
      switch (bev-> button) {
      case Button1:
	 e. pos. button = mbLeft;
	 break;
      case Button2:
	 e. pos. button = mbMiddle;
	 break;
      case Button3:
	 e. pos. button = mbRight;
	 break;
      default:
	 e. pos. button = 0;
      }
      e. pos. where. x = bev-> x;
      e. pos. where. y = XX-> size. y - bev-> y - 1;
      e. pos. mod = 0;
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

      if ( ev-> xexpose. count && !XX-> flags. syncPaint) {
	 return;
      }

      e. cmd = cmPaint;
      break;
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
   } else {
      /* Unhandled event, just do nothing */
      DOLBUG( "*** event %u to %s ***\n", ev-> type, PWidget( self)-> name);
      guts. unhandled_events++;
   }
}
