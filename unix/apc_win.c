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
/*  System dependent window management (unix, x11)         */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Window.h"
#include "Application.h"

Bool
apc_window_create( Handle self, Handle owner, Bool sync_paint, int border_icons,
		   int border_style, Bool task_list,
		   int window_state, Bool use_origin, Bool use_size)
{
   DEFXX;
   Handle real_owner;
   XSizeHints hints;
   XSetWindowAttributes attrs;
   XWindow parent = guts. root;

   if ( X_WINDOW)  // recreate request - can't change a thing ... 
      return true; 

   /* create */
   attrs. event_mask = 0
      | KeyPressMask
      | KeyReleaseMask
      | ButtonPressMask
      | ButtonReleaseMask
      | EnterWindowMask
      | LeaveWindowMask
      | PointerMotionMask
      /* | PointerMotionHintMask */
      /* | Button1MotionMask */
      /* | Button2MotionMask */
      /* | Button3MotionMask */
      /* | Button4MotionMask */
      /* | Button5MotionMask */
      | ButtonMotionMask
      | KeymapStateMask
      | ExposureMask
      | VisibilityChangeMask
      | StructureNotifyMask
      /* | ResizeRedirectMask */
      /* | SubstructureNotifyMask */
      /* | SubstructureRedirectMask */
      | FocusChangeMask
      | PropertyChangeMask
      | ColormapChangeMask
      | OwnerGrabButtonMask;
   attrs. override_redirect = false;
   attrs. do_not_propagate_mask = attrs. event_mask;
   X_WINDOW = XCreateWindow( DISP, parent,
	                     0, 0, 1, 1, 0, CopyFromParent,
	                     InputOutput, CopyFromParent,
	                     0
	                     /* | CWBackPixmap */
	                     /* | CWBackPixel */
	                     /* | CWBorderPixmap */
	                     /* | CWBorderPixel */
	                     /* | CWBitGravity */
	                     /* | CWWinGravity */
	                     /* | CWBackingStore */
	                     /* | CWBackingPlanes */
	                     /* | CWBackingPixel */
	                     | CWOverrideRedirect
	                     /* | CWSaveUnder */
	                     | CWEventMask
	                     /* | CWDontPropagate */
	                     /* | CWColormap */
	                     /* | CWCursor */
	                     , &attrs);
   if (!X_WINDOW) return false;
   XCHECKPOINT;
   hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);
   XCHECKPOINT;
   XX-> flags. iconic = ( window_state == wsMinimized) ? 1 : 0;
   guts. wm_create_window( self, X_WINDOW);
   XCHECKPOINT;

   XX-> type.drawable = true;
   XX-> type.widget = true;
   XX-> type.window = true;

   real_owner = application;
   XX-> real_parent = XX-> parent = parent;
   XX-> udrawable = XX-> gdrawable = X_WINDOW;

   XX-> flags. clip_owner = false;
   XX-> flags. sync_paint = sync_paint;
   XX-> flags. process_configure_notify = true;

   XX-> above = nilHandle;
   XX-> owner = real_owner;
   apc_component_fullname_changed_notify( self);
   prima_send_create_event( X_WINDOW);

   // setting initial size
   XX-> size = guts. displaySize;
   if ( window_state != wsMaximized) {
      XX-> zoomRect. right = XX-> size. x;
      XX-> zoomRect. top   = XX-> size. y;
      XX-> size. x /= 3;
      XX-> size. y /= 3;
   } else
      XX-> flags. zoomed = 1;
   XX-> origin = ( Point) {0,0};
   
   bzero( &hints, sizeof( XSizeHints));
   hints. flags  = PBaseSize;
   hints. width  = hints. base_width  = XX-> size. x;
   hints. height = hints. base_height = XX-> size. y;
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XResizeWindow( DISP, X_WINDOW, XX-> size. x, XX-> size. y); 

   prima_send_cmSize( self, (Point){0,0});
   
   return true;
}

Bool
apc_window_activate( Handle self)
{
   if ( X(self)-> flags. iconic || X(self)-> flags. withdrawn) return true;
   XRaiseWindow( DISP, X_WINDOW);
   apc_application_yield();
   XSetInputFocus( DISP, X_WINDOW, RevertToParent, CurrentTime);
   XCHECKPOINT;
   apc_application_yield();
   return true;
}

Bool
apc_window_is_active( Handle self)
{
   return apc_window_get_active() == self;
   return false;
}

Bool
apc_window_close( Handle self)
{
   return prima_simple_message( self, cmClose, true);
}

Handle
apc_window_get_active( void)
{
   Handle x = guts. focused;
   while ( x && !X(x)-> type. window) x = PWidget(x)-> owner;
   return x;
}

int
apc_window_get_border_icons( Handle self)
{
   DOLBUG( "apc_window_get_border_icons()\n");
   return 0;
}

int
apc_window_get_border_style( Handle self)
{
   DOLBUG( "apc_window_get_border_style()\n");
   return 0;
}

Point
apc_window_get_client_pos( Handle self)
{
   return X(self)-> origin;
}

Point
apc_window_get_client_size( Handle self)
{
   return X(self)-> size;
}

Bool
apc_window_get_icon( Handle self, Handle icon)
{
   DOLBUG( "apc_window_get_icon()\n");
   return false;
}

int
apc_window_get_window_state( Handle self)
{
   return (X(self)-> flags. iconic != 0) ? wsMinimized :
     ((X(self)-> flags. zoomed != 0) ? wsMaximized : wsNormal);
}

Bool
apc_window_get_task_listed( Handle self)
{
   DOLBUG( "apc_window_get_task_listed()\n");
   return false;
}

Bool
apc_window_set_caption( Handle self, const char *caption)
{
   XStoreName( DISP, X_WINDOW, caption);
   XSetIconName( DISP, X_WINDOW, caption);
   return true;
}

XWindow
prima_find_frame_window( XWindow w)
{
   XWindow r, p, *c;
   int nc;

   if ( w == None)
      return None;
   while ( XQueryTree( DISP, w, &r, &p, &c, &nc)) {
      if (c)
         XFree(c);
      if ( p == r)
         return w;
      w = p;
   }
   return None;
}

Bool
prima_get_frame_info( Handle self, PRect r)
{
   DEFXX;
   XWindow p, dummy;
   int px, py;
   unsigned int pw, ph, pb, pd;

   bzero( r, sizeof( Rect));
   if (( p = prima_find_frame_window( X_WINDOW)) == None) {
      return false;
   } else if ( p != X_WINDOW) 
      if ( !XTranslateCoordinates( DISP, X_WINDOW, p, 0, 0, &r-> left, &r-> bottom, &dummy))
         warn( "error in XTranslateCoordinates()");
      if ( !XGetGeometry( DISP, p, &dummy, &px, &py, &pw, &ph, &pb, &pd)) {
         warn( "error in XGetGeometry()");
      r-> right = pw - r-> left  - XX-> size. x;
      r-> top   = ph - r-> right - XX-> size. y;
   }
   return true;
}

Bool
apc_window_set_client_pos( Handle self, int x, int y)
{
   DEFXX;
   XSizeHints hints;
   Point p = XX-> origin;

   bzero( &hints, sizeof( XSizeHints));
   
   XX-> origin = (Point){x,y};
   y = X(XX-> owner)-> size. y - XX-> size.y - y;
   hints. flags = USPosition;
   hints. x = x;
   hints. y = y;
   XMoveWindow( DISP, X_WINDOW, x, y);
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XCHECKPOINT;
   if ( XX-> origin.x != p. x || XX-> origin. y != p. y) {
      Event e;
      bzero( &e, sizeof( e));
      e. cmd = cmMove;
      e. gen. source = self;
      e. gen. P = XX-> origin;
      apc_message( self, &e, false);
   }
   return true;
}

Bool
apc_window_set_client_size( Handle self, int width, int height)
{
   DEFXX;
   XSizeHints hints;
   PWidget widg = PWidget( self);
   Point sz = XX-> size;

   bzero( &hints, sizeof( XSizeHints));

   widg-> virtualSize = (Point){width,height};
   width = ( width > 0)
      ? (( width >= widg-> sizeMin. x)
	  ? (( width <= widg-> sizeMax. x)
              ? width 
	      : widg-> sizeMax. x)
	  : widg-> sizeMin. x)
      : 1; 
   height = ( height > 0)
      ? (( height >= widg-> sizeMin. y)
	  ? (( height <= widg-> sizeMax. y)
	      ? height
	      : widg-> sizeMax. y)
	  : widg-> sizeMin. y)
      : 1;

   XX-> size = (Point){width,height}; /* XXX */

   hints. flags = USPosition | USSize;
   hints. x = XX-> origin. x;
   hints. y = X(XX-> owner)-> size. y - XX-> size.y - XX-> origin. y;
   hints. width = width;
   hints. height = height;
   XMoveResizeWindow( DISP, X_WINDOW, hints. x, hints. y, width, height);
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XCHECKPOINT;
   prima_send_cmSize( self, sz);
   return true;
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
   DOLBUG( "apc_window_set_menu()\n");
   return false;
}

Bool
apc_window_set_icon( Handle self, Handle icon)
{
   DOLBUG( "apc_window_set_icon()\n");
   return true;
}

static void
apc_widget_set_rect( Handle self, int x, int y, int szx, int szy)
{
    XSizeHints hints;
    hints. flags = USPosition | USSize;
    hints. x = x;
    hints. y = y;
    hints. width  = szx;
    hints. height = szy;
    XMoveResizeWindow( DISP, X_WINDOW, hints. x, hints. y, hints. width, hints. height);
    XSetWMNormalHints( DISP, X_WINDOW, &hints);
}   

Bool
apc_window_set_window_state( Handle self, int state)
{
   DEFXX;
   Event e;
   XWMHints * wh;
   if ( state != wsMinimized && state != wsNormal && state != wsMaximized) return false; 

   switch ( state) {
   case wsMinimized:
       if ( XX-> flags. iconic) return false;
       break;
   case wsMaximized:
       break;
   case wsNormal:
       if ( !XX-> flags. iconic && !XX-> flags. zoomed) return false;
       break;
   default:
       return false;
   }   
   
   wh = XGetWMHints( DISP, X_WINDOW);
   if ( !wh) {
      warn("Error querying XGetWMHints");
      return false;
   }
   
   if ( state == wsMaximized && !XX-> flags. zoomed) {
      XX-> zoomRect = ( Rect) {XX-> origin.x, XX-> origin.y, XX-> size.x, XX-> size.y};
      XX-> flags. zoomed = 1;
      apc_widget_set_rect( self, 0, 0, guts. displaySize.x, guts. displaySize.y);
   }

   if ( !XX-> flags. withdrawn) {
      if ( state == wsMinimized)
         XIconifyWindow( DISP, X_WINDOW, SCREEN);
      else {
         XMapWindow( DISP, X_WINDOW);
      }   
   }     
   XX-> flags. iconic = ( state == wsMinimized) ? 1 : 0;
   
   if ( XX-> flags. zoomed && state != wsMaximized) {
      XX-> flags. zoomed = 0;
      apc_widget_set_rect( self, XX-> zoomRect. left, XX-> zoomRect. bottom, 
         XX-> zoomRect. right, XX-> zoomRect. top);
   }   
   XFree( wh); 
   
   bzero( &e, sizeof(e));
   e. gen. source = self;
   e. cmd = cmWindowState;
   e. gen. i = state;
   apc_message( self, &e, false);
   return true;
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insert_before)
{
   DEFXX;
   if (( XX-> preexec_focus = apc_widget_get_focused()))
      protect_object( XX-> preexec_focus);
   CWindow( self)-> exec_enter_proc( self, shared, insert_before);
   XSetTransientForHint( DISP, X_WINDOW, PWindow(PWindow(self)-> owner)-> handle);
   apc_widget_set_enabled( self, true);
   apc_widget_set_visible( self, true);
   apc_window_activate( self);
   prima_simple_message( self, cmExecute, true);
   guts. modal_count++;
   return true;
}

Bool
apc_window_execute( Handle self, Handle insert_before)
{
   X(self)-> flags.modal = true;
   if ( !window_start_modal( self, false, insert_before))
      return false;
   if (!application) return false;

   protect_object( self);

   XSync( DISP, false);
   while ( prima_one_loop_round( true, true) && X(self) && X(self)-> flags.modal)
      ;
   unprotect_object( self);
   return true;
}

Bool
apc_window_execute_shared( Handle self, Handle insert_before)
{
   return window_start_modal( self, true, insert_before);
}

Bool
apc_window_end_modal( Handle self)
{
   PWindow win = PWindow(self);
   Handle modal, oldfoc;
   DEFXX;
   XX-> flags.modal = false;
   XSetTransientForHint( DISP, X_WINDOW, nilHandle);
   CWindow( self)-> exec_leave_proc( self);
   apc_widget_set_visible( self, false);
   if ( application) {
      modal = CApplication(application)->popup_modal( application);
      if ( !modal && win->owner)
         CWidget( win->owner)-> set_selected( win->owner, true);
      if (( oldfoc = XX-> preexec_focus)) {
         if ( PWidget( oldfoc)-> stage == csNormal)
            CWidget( oldfoc)-> set_focused( oldfoc, true);
         unprotect_object( oldfoc);
      }
   }
   if ( guts. modal_count > 0)
      guts. modal_count--;
   return true;
}
