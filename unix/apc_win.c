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
   Event e;
   Handle real_owner;
   XSetWindowAttributes attrs;
   XWindow parent = RootWindow( DISP, SCREEN);

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
   guts. wm_create_window( self, X_WINDOW);
   XCHECKPOINT;

   XX-> type.drawable = true;
   XX-> type.widget = true;
   XX-> type.window = true;

   real_owner = application;
   XX-> parent = parent;
   XX-> udrawable = XX-> gdrawable = X_WINDOW;

   XX-> flags. clip_owner = false;
   XX-> flags. sync_paint = sync_paint;
   XX-> flags. do_size_hints = true;
   XX-> flags. no_size = true;
   XX-> flags. process_configure_notify = true;

   XX-> owner = real_owner;
   XX-> size. x = DisplayWidth( DISP, SCREEN) / 3;
   XX-> size. y = DisplayHeight( DISP, SCREEN) / 3;
   XX-> known_size = (Point){APC_BAD_SIZE,APC_BAD_SIZE};
   XX-> origin = (Point){0,0};
   XX-> known_origin = (Point){APC_BAD_ORIGIN,APC_BAD_ORIGIN};
   apc_component_fullname_changed_notify( self);
   prima_send_create_event( X_WINDOW);

   bzero( &e, sizeof(e));
   e. gen. source = self;
   e. cmd = cmSize;
   e. gen. P = XX-> size;
   e. gen. R. right = XX-> size. x;
   e. gen. R. top = XX-> size. y;
   apc_message( self, &e, false);

   return true;
}

Bool
apc_window_activate( Handle self)
{
   XWindow frame;

   /* fprintf( stderr, "activate %s\n", PComponent(self)->name); */
   frame = prima_find_frame_window( PWidget( self)-> handle);
   if ( frame) XRaiseWindow( DISP, frame);
   {
      XWindow windoze = apc_widget_is_showing( self) ? X_WINDOW : None;
      /* if (windoze == None) */
         /* fprintf( stderr, "FUCHIK %s\n", PComponent(self)->name); */
      /* else */
         /* fprintf( stderr, "PUCHIK %s: %08lx => %08lx\n", */
                  /* PComponent(self)->name, PWidget(self)->handle, windoze); */
      XSetInputFocus( DISP, windoze, RevertToParent, CurrentTime);
   }
   return true;
}

Bool
apc_window_is_active( Handle self)
{
   DOLBUG( "apc_window_is_active()\n");
   return false;
}

Bool
apc_window_close( Handle self)
{
   DOLBUG( "apc_window_close()\n");
   return false;
}

Handle
apc_window_get_active( void)
{
   DOLBUG( "apc_window_get_active()\n");
   return nilHandle;
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
   if (0) /*MMM*/
   fprintf( stderr, "%s: getcpos: %d, %d\n",
            PComponent(self)->name, X(self)->origin.x, X(self)->origin.y);
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
   DOLBUG( "apc_window_get_window_state()\n");
   return 0;
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

   if (( p = prima_find_frame_window( X_WINDOW)) == None) {
      return false;
   } else if ( p == X_WINDOW) {
      r-> left = r-> bottom = r-> right = r-> top = 0;
   } else {
      if ( !XTranslateCoordinates( DISP, X_WINDOW, p, 0, 0, &r->left, &r->top, &dummy))
         croak( "error in XTranslateCoordinates()");
      if ( !XGetGeometry( DISP, p, &dummy, &px, &py, &pw, &ph, &pb, &pd))
         croak( "error in XGetGeometry()");
      r->right = pw - XX->size.x - r-> left + pb;
      r->left += pb;
      r->bottom = ph - XX->size.y - r-> top + pb;
      r->top += pb;
   }
   return true;
}

Bool
apc_window_set_client_pos( Handle self, int x, int y)
{
   DEFXX;
   XSizeHints hints;

   bzero( &hints, sizeof( XSizeHints));

   XX-> origin = (Point){x,y}; 
   y = X(XX-> owner)-> size. y - XX-> size.y - y;
   hints. flags = USPosition | PMinSize | PMaxSize;
   hints. x = x;
   hints. y = y;
   hints. min_width = PWidget(self)-> sizeMin. x;
   hints. min_height = PWidget(self)-> sizeMin. y;
   hints. max_width = PWidget(self)-> sizeMax. x;
   hints. max_height = PWidget(self)-> sizeMax. y;
   XX-> flags. do_size_hints = false;

   XMoveWindow( DISP, X_WINDOW, x, y);
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XCHECKPOINT;
   return true;
}

Bool
apc_window_set_client_size( Handle self, int width, int height)
{
   DEFXX;
   int y;
   XSizeHints hints;
   PWidget widg = PWidget( self);
   Event e;

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

   bzero( &e, sizeof(e));
   e. gen. source = self;
   e. cmd = cmSize;
   e. gen. R. left = XX-> size. x;
   e. gen. R. bottom = XX-> size. y;
   e. gen. P = XX-> size = (Point){width,height}; /* XXX */
   e. gen. R. right = XX-> size. x;
   e. gen. R. top = XX-> size. y;
   y = X(XX-> owner)-> size. y - height - XX-> origin. y;

   hints. flags = USPosition | USSize | PMinSize | PMaxSize;
   hints. x = XX-> origin. x;
   hints. y = y;
   hints. width = width;
   hints. height = height;
   hints. min_width = widg-> sizeMin. x;
   hints. min_height = widg-> sizeMin. y;
   hints. max_width = widg-> sizeMax. x;
   hints. max_height = widg-> sizeMax. y;
   XX-> flags. do_size_hints = false;

   XMoveResizeWindow( DISP, X_WINDOW, XX-> origin. x, y, width, height);
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XCHECKPOINT;

   {
      int i, y = X(XX-> owner)-> size. y, count = PWidget( self)-> widgets. count;
      for ( i = 0; i < count; i++) {
	 PWidget child = PWidget( PWidget( self)-> widgets. items[i]);
         XMoveWindow( DISP, child-> handle, X(child)-> origin.x, y - X(child)-> size.y - X(child)-> origin.y);
      }
   }
   apc_message( self, &e, false);

   DOLBUG( "window size to (%d,%d(%d)) - (%d,%d)\n", XX-> origin. x, XX-> origin. y, y, width, height);
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

Bool
apc_window_set_window_state( Handle self, int state)
{
   DOLBUG( "apc_window_set_window_state()\n");
   return true;
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insert_before)
{
   DEFXX;

   if (( XX-> preexec_focus = apc_widget_get_focused()))
      protect_object( XX-> preexec_focus);
   CWindow( self)-> exec_enter_proc( self, shared, insert_before);
   apc_widget_set_enabled( self, true);
   apc_widget_set_visible( self, true);
   prima_simple_message( self, cmExecute, true);
   guts. modal_count++;
   return true;
}

Bool
apc_window_execute( Handle self, Handle insert_before)
{
   if ( !window_start_modal( self, false, insert_before))
      return false;
   if (!application) return false;

   X(self)-> flags.modal = true;
   protect_object( self);

   XNoOp( DISP);
   XFlush( DISP);

   while ( prima_one_loop_round( true) && X(self) && X(self)-> flags.modal)
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
   if ( win-> modal == mtExclusive)
      XX-> flags.modal = false;
   CWindow( self)-> exec_leave_proc( self);
   apc_widget_set_visible( self, false);
   apc_widget_set_enabled( self, 0);
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
