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
/*  System dependent window management (unix, x11)         */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

Bool
apc_window_create( Handle self, Handle owner, Bool syncPaint,
		   Bool clipOwner, int borderIcons,
		   int borderStyle, Bool taskList,
		   int windowState, Bool useOrigin, Bool useSize)
{
   XSetWindowAttributes attrs;
   XWindow parent;
   XWindow old;
   Handle realOwner;
   DEFXX;

   /* Transparency is ignored for now */

   if ( !clipOwner) {
      parent = RootWindow( DISP, SCREEN);
      realOwner = application;
   } else if ( owner == application) {
      parent = RootWindow( DISP, SCREEN);
      realOwner = application;
   } else {
      parent = RootWindow( DISP, SCREEN);    /* XXX for now  :-(  */
      realOwner = owner;
   }

   old = X_WINDOW;
   if ( old && XX-> parent != parent) {
      /* reparent */
   } else if ( !old) {
      /* create */
      attrs. event_mask = 0
	 | KeyPressMask
	 | KeyReleaseMask
	 | ButtonPressMask
	 | ButtonReleaseMask
	 | EnterWindowMask
	 | LeaveWindowMask
	 /* | PointerMotionMask */
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
      if (!X_WINDOW)
	 return false;
      XCHECKPOINT;
      hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);
      XCHECKPOINT;
      guts. wm_create_window( self, X_WINDOW);
      XCHECKPOINT;
   } else {
      /* XXX otherwise do nothing? */
   }

   XX-> parent = parent;
   XX-> drawable = X_WINDOW;

   XX-> flags. clipOwner = clipOwner;
   XX-> flags. syncPaint = syncPaint;
   XX-> flags. doSizeHints = true;
   XX-> flags. noSize = true;

   XX-> owner = realOwner;
   XX-> size = (Point){0,0};
   XX-> knownSize = (Point){APC_BAD_SIZE,APC_BAD_SIZE};
   XX-> origin = (Point){0,0};
   XX-> knownOrigin = (Point){APC_BAD_ORIGIN,APC_BAD_ORIGIN};
   apc_component_fullname_changed_notify( self);

   return true;
}

void
apc_window_activate( Handle self)
{
   DOLBUG( "apc_window_activate()\n");
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

void
apc_window_set_caption( Handle self, const char *caption)
{
   XStoreName( DISP, X_WINDOW, caption);
}

void
apc_window_set_client_pos( Handle self, int x, int y)
{
   DEFXX;
   XSizeHints hints;

   bzero( &hints, sizeof( XSizeHints));

   XX-> origin = (Point){x,y}; /* XXX ? */
   y = X(XX-> owner)-> size. y - XX-> size.y - y;

   hints. flags = USPosition | PMinSize;
   hints. x = x;
   hints. y = y;
   hints. min_width = PWidget(self)-> sizeMin. x;
   hints. min_height = PWidget(self)-> sizeMin. y;
   XX-> flags. doSizeHints = false;
       
   XMoveWindow( DISP, X_WINDOW, x, y);
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XCHECKPOINT;

   DOLBUG( "window move to (%d,%d(%d))\n", x, XX-> origin. y, y);
}

void
apc_window_set_client_size( Handle self, int width, int height)
{
   DEFXX;
   int y;
   XSizeHints hints;

   bzero( &hints, sizeof( XSizeHints));

   XX-> size = (Point){width, height}; /* XXX ? */
   y = X(XX-> owner)-> size. y - height - XX-> origin. y;

   hints. flags = USPosition | USSize | PMinSize;
   hints. x = XX-> origin. x;
   hints. y = y;
   hints. width = width;
   hints. height = height;
   hints. min_width = PWidget(self)-> sizeMin. x;
   hints. min_height = PWidget(self)-> sizeMin. y;
   XX-> flags. doSizeHints = false;

   XMoveResizeWindow( DISP, X_WINDOW, XX-> origin. x, y, width, height);
   XSetWMNormalHints( DISP, X_WINDOW, &hints);
   XCHECKPOINT;

   DOLBUG( "window size to (%d,%d(%d)) - (%d,%d)\n", XX-> origin. x, XX-> origin. y, y, width, height);
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
   DOLBUG( "apc_window_set_menu()\n");
   return false;
}

void
apc_window_set_icon( Handle self, Handle icon)
{
   DOLBUG( "apc_window_set_icon()\n");
}

void
apc_window_set_window_state( Handle self, int state)
{
   DOLBUG( "apc_window_set_window_state()\n");
}

Bool
apc_window_execute( Handle self, Handle insertBefore)
{
   DOLBUG( "apc_window_execute()\n");
   return false;
}

Bool
apc_window_execute_shared( Handle self, Handle insertBefore)
{
   DOLBUG( "apc_window_execute_shared()\n");
   return false;
}

void
apc_window_end_modal( Handle self)
{
   DOLBUG( "apc_window_end_modal()\n");
}
