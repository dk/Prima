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
/*  System dependent widget management (unix, x11)         */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

Point
apc_widget_client_to_screen( Handle self, Point p)
{
   XWindow cld;

   p. y = X( self)-> size. y - p. y - 1;
   if ( !XTranslateCoordinates( DISP, X_WINDOW, RootWindow( DISP, SCREEN),
				p. x, p. y, &p. x, &p. y, &cld)) {
      croak( "apc_widget_client_to_screen(): XTranslateCoordinates() failed");
   }
   XCHECKPOINT;
   p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
   return p;
}

Bool
apc_widget_create( Handle self, Handle owner, Bool syncPaint,
		   Bool clipOwner, Bool transparent)
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
      parent = PWidget( owner)-> handle;
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
      attrs. override_redirect = true;
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
   }
   /* otherwise do nothing? */
   XX-> parent = parent;
   XX-> drawable = X_WINDOW;

   XX-> flags. clipOwner = clipOwner;
   XX-> flags. syncPaint = syncPaint;
   XX-> flags. doSizeHints = false;
   XX-> flags. noSize = true;

   XX-> owner = realOwner;
   XX-> size = (Point){0,0};
   XX-> knownSize = (Point){APC_BAD_SIZE,APC_BAD_SIZE};
   XX-> origin = (Point){0,0};
   XX-> knownOrigin = (Point){APC_BAD_ORIGIN,APC_BAD_ORIGIN};
   apc_component_fullname_changed_notify( self);

   DOLBUG( "&&&&&&&&&&& window created: %s &&&&&&&&&&&\n", PWidget( self)-> name);

   return true;
}

#define VIRGIN_GC_MASK (GCLineWidth|GCBackground|GCForeground|GCFunction|GCClipMask)

Bool
apc_widget_begin_paint( Handle self, Bool insideOnPaint)
{
   DEFXX;
   unsigned long mask = VIRGIN_GC_MASK;

   XX-> paintRop = XX-> rop;
   XX-> savedFont = PDrawable( self)-> font;
   XX-> fore = XX-> savedFore;
   XX-> back = XX-> savedBack;
   XX-> flags. zeroLine = XX-> flags. savedZeroLine;
   XX-> gcv. clip_mask = None;
   XX-> gtransform = XX-> transform;

   prima_get_gc( XX);
   XChangeGC( DISP, XX-> gc, mask, &XX-> gcv);
   XCHECKPOINT;

   if ( XX-> region) {
      XSetRegion( DISP, XX-> gc, XX-> region);
      XX-> stale_region = XX-> region;
      XX-> region = nil;
   }

   XX-> flags. paint = true;

   if ( !XX-> flags. reloadFont && XX-> font && XX-> font-> id) {
      XSetFont( DISP, XX-> gc, XX-> font-> id);
      XCHECKPOINT;
   } else {
      apc_gp_set_font( self, &PDrawable( self)-> font);
      XX-> flags. reloadFont = false;
   }
   return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
   DOLBUG( "apc_widget_begin_paint_info()\n");
   return false;
}

void
apc_widget_destroy( Handle self)
{
   if ( X_WINDOW) {
      XCHECKPOINT;
      XDestroyWindow( DISP, X_WINDOW);
      XCHECKPOINT;
      hash_delete( guts.windows, (void*)&X_WINDOW, sizeof(X_WINDOW), false);
      X_WINDOW = nilHandle;
   }
}

PFont
apc_widget_default_font( PFont f)
{
   return apc_font_default( f);
}

void
apc_widget_end_paint( Handle self)
{
   DEFXX;
   prima_release_gc(XX);
   XX-> flags. paint = false;
   if ( XX-> flags. reloadFont) {
      PDrawable( self)-> font = XX-> savedFont;
   }
   if ( XX-> stale_region) {
      XDestroyRegion( XX-> stale_region);
      XX-> stale_region = nil;
   }
}

void
apc_widget_end_paint_info( Handle self)
{
   DOLBUG( "apc_widget_end_paint_info()\n");
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   return X(self)-> flags. clipOwner;
}

Rect
apc_widget_get_clip_rect( Handle self)
{
   DOLBUG( "apc_widget_get_clip_rect()\n");
   return (Rect){0,0,0,0};
}

Color
apc_widget_get_color( Handle self, int index)
{
   return X(self)-> colors[ index];
}

Bool
apc_widget_get_first_click( Handle self)
{
   DOLBUG( "apc_widget_get_first_click()\n");
   return false;
}

Handle
apc_widget_get_focused( void)
{
   return guts. focused;
}

ApiHandle
apc_widget_get_handle( Handle self)
{
   DOLBUG( "apc_widget_get_handle()\n");
   return nilHandle;
}

Rect
apc_widget_get_invalid_rect( Handle self)
{
   DOLBUG( "apc_widget_get_invalid_rect()\n");
   return (Rect){0,0,0,0};
}

Point
apc_widget_get_pos( Handle self)
{
   return X(self)-> origin;
}

Bool
apc_widget_get_shape( Handle self, Handle mask)
{
   return false;
}

Point
apc_widget_get_size( Handle self)
{
   return X(self)-> size;
}

Bool
apc_widget_get_sync_paint( Handle self)
{
   return X(self)-> flags. syncPaint;
}

Bool
apc_widget_get_transparent( Handle self)
{
   DOLBUG( "apc_widget_get_transparent()\n");
   return false;
}

Bool
apc_widget_is_captured( Handle self)
{
   return X(self)-> flags. grab;
}

Bool
apc_widget_is_enabled( Handle self)
{
   return X(self)-> flags. enabled;
}

Bool
apc_widget_is_exposed( Handle self)
{
   return X(self)-> flags. exposed;
}

Bool
apc_widget_is_focused( Handle self)
{
   return X(self)-> flags. focused;
}

Bool
apc_widget_is_responsive( Handle self)
{
   DOLBUG( "apc_widget_is_responsive()\n");
   return false;
}

Bool
apc_widget_is_showing( Handle self)
{
   XWindowAttributes attrs;

   if ( X(self)-> flags. mapped
	&& XGetWindowAttributes( DISP, X_WINDOW, &attrs)
	&& attrs. map_state == IsViewable)
      return true;
   else
      return false;
}

Bool
apc_widget_is_visible( Handle self)
{
   DOLBUG("apc_widget_is_visible(%s): %d\n", PWidget( self)-> name, X(self)-> flags. mapped);
   return X(self)-> flags. mapped;
}

void
apc_widget_invalidate_rect( Handle self, Rect *rect)
{
   XRectangle r;
   XExposeEvent ev;
   DEFXX;

   if ( rect) {
      r. x = rect-> left;
      r. width = rect-> right - rect-> left;
      r. y = XX-> size. y - rect-> top;
      r. height = rect-> top - rect-> bottom;
   } else {
      r. x = 0;
      r. width = XX-> size. x;
      r. y = 0;
      r. height = XX-> size. y;
   }

   if ( !XX-> region) {
      XX-> region = XCreateRegion();
      ev. type = Expose;
      ev. window = X_WINDOW;
      ev. count = 0;
      ev. x = r. x;
      ev. y = r. y;
      ev. width = r. width;
      ev. height = r. height;

      if ( !XX-> flags. syncPaint) {
	 XSendEvent( DISP, X_WINDOW, false, 0, (XEvent*)&ev);
	 XCHECKPOINT;
      }
   }

   XUnionRectWithRegion( &r, XX-> region, XX-> region);
   if ( XX-> flags. syncPaint) {
      apc_widget_update( self);
   }
}

Point
apc_widget_screen_to_client( Handle self, Point p)
{
   XWindow cld;

   p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
   if ( !XTranslateCoordinates( DISP, RootWindow( DISP, SCREEN),
				X_WINDOW, p. x, p. y,
				&p. x, &p. y, &cld)) {
      croak( "apc_widget_screen_to_client(): XTranslateCoordinates() failed");
   }
   XCHECKPOINT;
   p. y = X( self)-> size. y - p. y - 1;
   return p;
}

void
apc_widget_scroll( Handle self, int horiz, int vert,
		   Rect *r, Bool withChildren)
{
   DEFXX;
   int src_x, src_y, w, h, dst_x, dst_y;

   if ( r) {
      src_x = r-> left;
      src_y = XX-> size. y - r-> top;
      w = r-> right - src_x;
      h = r-> top - r-> bottom;
   } else {
      src_x = 0;
      src_y = 0;
      w = XX-> size. x;
      h = XX-> size. y;
   }
   dst_x = src_x + horiz;
   dst_y = src_y - vert;

   prima_get_gc( XX);
   XX-> gcv. clip_mask = None;
   XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &XX-> gcv);
   XCHECKPOINT;

   XCopyArea( DISP, XX-> drawable, XX-> drawable, XX-> gc,
	      src_x, src_y, w, h, dst_x, dst_y);
   XCHECKPOINT;

   prima_release_gc( XX);
}

void
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
   int r;
   XWindow confine_to = None;
   DEFXX;

   if ( capture) {

      if ( confineTo && PWidget(confineTo)-> handle)
	 confine_to = PWidget(confineTo)-> handle;

      r = XGrabPointer( DISP, X_WINDOW, false, 0
			| ButtonPressMask
			| ButtonReleaseMask
			| PointerMotionMask
			| ButtonMotionMask, GrabModeAsync, GrabModeAsync,
			None, None, CurrentTime);
      XCHECKPOINT;
      if ( r != GrabSuccess) {
	 /* XXX do something sensible here */
	 warn( "apc_widget_set_capture(): unsuccessful grab");
      } else {
	 XX-> flags. grab = true;
      }
   } else if ( !capture) {
      XUngrabPointer( DISP, CurrentTime);
      XCHECKPOINT;
      XX-> flags. grab = false;
   }
}

void
apc_widget_set_clip_rect( Handle self, Rect clipRect)
{
   DOLBUG( "apc_widget_set_clip_rect()\n");
}

void
apc_widget_set_color( Handle self, Color color, int index)
{
   X(self)-> colors[ index] = color;
   if ( index == ciFore)
      apc_gp_set_color( self, color);
   else if ( index == ciBack)
      apc_gp_set_back_color( self, color);
}

void
apc_widget_set_enabled( Handle self, Bool enable)
{
   X(self)-> flags. enabled = enable;
   DOLBUG( "apc_widget_set_enabled( %d) of %s\n", enable, PWidget(self)->name);
}

void
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   DOLBUG( "apc_widget_set_first_click()\n");
}

void
apc_widget_set_focused( Handle self)
{
   if ( apc_widget_is_showing( self)) {
      DOLBUG( "~~~~~~~~~ Setting focus to %s\n", PWidget( self)-> name);
      XSetInputFocus( DISP, X_WINDOW, RevertToParent, CurrentTime);
      XCHECKPOINT;
   } else {
      DOLBUG( "~~~~~~~~~~~~~~~~~~ cannot set focus ~~~~~~~~~~~~~~~~~\n");
   }
}

void
apc_widget_set_font( Handle self, PFont font)
{
   Event ev = {cmFontChanged};

   apc_gp_set_font( self, font);

   ev. gen. source = self;
   CWidget(self)-> message( self, &ev);
}

void
apc_widget_set_palette( Handle self)
{
   DOLBUG( "apc_widget_set_palette()\n");
}

void
apc_widget_set_pos( Handle self, int x, int y)
{
   DEFXX;

   XX-> origin = (Point){x,y};  /* XXX ? */
   y = X(XX-> owner)-> size. y - XX-> size.y - y;
   XMoveWindow( DISP, X_WINDOW, x, y);
   DOLBUG( "XMoveWindow: widget (%s) move to (%d,%d)\n", PWidget(self)-> name, x, y);
   XCHECKPOINT;
}

void
apc_widget_set_shape( Handle self, Handle mask)
{
   return;
}

void
apc_widget_set_size( Handle self, int width, int height)
{
   DEFXX;
   int y;
   PWidget widg = PWidget( self);

   widg-> virtualSize = (Point){width,height};

   width = width > 0
      ? ( width >= widg-> sizeMin. x
	  ? ( width <= widg-> sizeMax. x
	      ? width
	      : widg-> sizeMax. x)
	  : widg-> sizeMin. x)
      : 1;
   height = height > 0
      ? ( height >= widg-> sizeMin. y
	  ? ( height <= widg-> sizeMax. y
	      ? height
	      : widg-> sizeMax. y)
	  : widg-> sizeMin. y)
      : 1;

   XX-> size = (Point){width, height};  /* XXX ? */
   y = X(XX-> owner)-> size. y - height - XX-> origin. y;
   XMoveResizeWindow( DISP, X_WINDOW, XX-> origin. x, y, width, height);
   DOLBUG( "widget (%s) size to (%d,%d) - (%d,%d)\n", PWidget(self)-> name, XX-> origin. x, y, width, height);
   XCHECKPOINT;
}

void
apc_widget_set_tab_order( Handle self, int tabOrder)
{
   DOLBUG( "apc_widget_set_tab_order()\n");
}

void
apc_widget_set_visible( Handle self, Bool show)
{
   DEFXX;
   Bool flush_n_wait = false;

   DOLBUG( "apc_widget_set_visible( %d) of %s\n", show, PWidget(self)->name);
   XX-> flags. mapped = show;
   if ( show) {
      if ( XX-> flags. doSizeHints) {
	 XSizeHints hints;
	 int width, height;

	 bzero( &hints, sizeof( XSizeHints));
	 hints. flags = PMinSize | PBaseSize;
	 hints. min_width = PWidget(self)-> sizeMin. x;
	 hints. min_height = PWidget(self)-> sizeMin. y;
	 hints. base_width = DisplayWidth( DISP, SCREEN) / 3;
	 hints. base_height = DisplayHeight( DISP, SCREEN) / 3;
	 width = hints. base_width < hints. min_width ? hints. min_width : hints. base_width;
	 height = hints. base_height < hints. min_height ? hints. min_height : hints. base_height;
	 hints. width = width;
	 hints. height = height;
	 XResizeWindow( DISP, X_WINDOW, width, height);
	 XSetWMNormalHints( DISP, X_WINDOW, &hints);
	 XCHECKPOINT;
	 XX-> flags. doSizeHints = false;
	 flush_n_wait = true;
      }

      XMapWindow( DISP, X_WINDOW);
      // XMapRaised( DISP, X_WINDOW);
      XRaiseWindow( DISP, X_WINDOW);
      XFlush( DISP);
      if ( flush_n_wait) {
	 XWindowAttributes attrs;
/* 	 XSizeHints hints; */
/* 	 long suppl; */
/* 	 while (!XGetWMNormalHints( DISP, X_WINDOW, &hints, &suppl)) { */
	 attrs. map_state = IsUnmapped;
	 while( attrs. map_state == IsUnmapped) {
	    XGetWindowAttributes( DISP, X_WINDOW, &attrs);
	    usleep( 10);
	 }
	 apc_application_yield();
      }
   } else {
      XUnmapWindow( DISP, X_WINDOW);
   }
   XCHECKPOINT;
}

void
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
   DOLBUG( "apc_widget_set_z_order()\n");
}

void
apc_widget_update( Handle self)
{
   Event e;
   if ( X(self)-> region) {
      e. cmd = cmPaint;
      CComponent( self)-> message( self, &e);
   }
}

void
apc_widget_validate_rect( Handle self, Rect rect)
{
   DOLBUG( "apc_widget_validate_rect()\n");
}

