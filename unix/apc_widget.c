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

#define SORT(a,b)       ({ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }})
#define REVERT(a)       ({ XX-> size. y - (a) - 1; })

Point
apc_widget_client_to_screen( Handle self, Point p)
{
   XWindow cld;
   DEFXX;

   p. y = X( self)-> size. y - p. y - 1;
   if ( !XTranslateCoordinates( DISP, XX->drawable, RootWindow( DISP, SCREEN),
				p. x, p. y, &p. x, &p. y, &cld)) {
      croak( "apc_widget_client_to_screen(): XTranslateCoordinates() failed");
   }
   XCHECKPOINT;
   p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
   return p;
}

Bool
apc_widget_create( Handle self, Handle owner, Bool sync_paint,
		   Bool clip_owner, Bool transparent)
{
   XSetWindowAttributes attrs;
   XWindow parent;
   XWindow old;
   Handle real_owner;
   DEFXX;

   /* Transparency is ignored for now */

   if ( !clip_owner) {
      parent = RootWindow( DISP, SCREEN);
      real_owner = application;
   } else if ( owner == application) {
      parent = RootWindow( DISP, SCREEN);
      real_owner = application;
   } else {
      parent = PWidget( owner)-> handle;
      real_owner = owner;
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

   XX-> flags. clip_owner = clip_owner;
   XX-> flags. sync_paint = sync_paint;
   XX-> flags. do_size_hints = false;
   XX-> flags. no_size = true;
   XX-> flags. process_configure_notify = false;

   XX-> owner = real_owner;
   XX-> size = (Point){0,0};
   XX-> known_size = (Point){APC_BAD_SIZE,APC_BAD_SIZE};
   XX-> origin = (Point){0,0};
   XX-> known_origin = (Point){APC_BAD_ORIGIN,APC_BAD_ORIGIN};
   apc_component_fullname_changed_notify( self);

   DOLBUG( "&&&&&&&&&&& window created: %s &&&&&&&&&&&\n", PWidget( self)-> name);

   prima_send_create_event( X_WINDOW);
   return true;
}

Bool
apc_widget_begin_paint( Handle self, Bool inside_on_paint)
{
   prima_no_cursor( self);
   prima_prepare_drawable_for_painting( self);
   return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
   DOLBUG( "apc_widget_begin_paint_info()\n");
   return false;
}

Bool
apc_widget_destroy( Handle self)
{
   DEFXX;
   free(XX-> dashes);
   XX-> dashes = nil;
   XX-> ndashes = 0;
   free(XX->paint_dashes);
   XX-> paint_dashes = nil;
   XX-> paint_ndashes = 0;
   if ( X_WINDOW) {
      XCHECKPOINT;
      XDestroyWindow( DISP, X_WINDOW);
      XCHECKPOINT;
      hash_delete( guts.windows, (void*)&X_WINDOW, sizeof(X_WINDOW), false);
      X_WINDOW = nilHandle;
   }
   return true;
}

PFont
apc_widget_default_font( PFont f)
{
   return apc_font_default( f);
}

Bool
apc_widget_end_paint( Handle self)
{
   prima_cleanup_drawable_after_painting( self);
   prima_update_cursor( self);
   return true;
}

Bool
apc_widget_end_paint_info( Handle self)
{
   DOLBUG( "apc_widget_end_paint_info()\n");
   return true;
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   return X(self)-> flags. clip_owner;
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
   return X_WINDOW;
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
   return X(self)-> flags. sync_paint;
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
   DEFXX;

   if ( XX-> flags. mapped
	&& XGetWindowAttributes( DISP, XX->drawable, &attrs)
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

Bool
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
      ev. window = XX->drawable;
      ev. count = 0;
      ev. x = r. x;
      ev. y = r. y;
      ev. width = r. width;
      ev. height = r. height;
      XX-> exposed_rect = r;

      if ( !XX-> flags. sync_paint) {
	 XSendEvent( DISP, XX->drawable, false, 0, (XEvent*)&ev);
	 XCHECKPOINT;
      }
   } else {
      prima_rect_union( &XX-> exposed_rect, &r);
   }

   XUnionRectWithRegion( &r, XX-> region, XX-> region);
   if ( XX-> flags. sync_paint) {
      apc_widget_update( self);
   }
   return true;
}

Point
apc_widget_screen_to_client( Handle self, Point p)
{
   XWindow cld;
   DEFXX;

   p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
   if ( !XTranslateCoordinates( DISP, RootWindow( DISP, SCREEN),
				XX->drawable, p. x, p. y,
				&p. x, &p. y, &cld)) {
      croak( "apc_widget_screen_to_client(): XTranslateCoordinates() failed");
   }
   XCHECKPOINT;
   p. y = XX-> size. y - p. y - 1;
   return p;
}

Bool
apc_widget_scroll( Handle self, int horiz, int vert,
		   Rect *r, Bool withChildren)
{
   DEFXX;
   int src_x, src_y, w, h, dst_x, dst_y;
   XRectangle xr, ir;
   Region invalid, reg;
   PPaintList pl;

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
   if (XX->scroll_rect. left != 0 || XX-> scroll_rect. right != 0) {
      Region region;
      Rect rect;
      XRectangle cpa;
     
      rect = XX-> scroll_rect;
      xr. x = rect. left;
      xr. y = REVERT( rect. top) + 1;
      xr. width = rect. right - rect. left;
      xr. height = rect. top - rect. bottom;
      region = XCreateRegion();
      XUnionRectWithRegion( &xr, region, region);
      XSetRegion( DISP, XX-> gc, region);
      XCHECKPOINT;
      XDestroyRegion( region);
      cpa. x = src_x;
      cpa. y = src_y;
      cpa. width = w;
      cpa. height = h;
      prima_rect_intersect( &cpa, &xr);
      dst_x += -src_x + cpa. x;
      dst_y += -src_y + cpa. y;
      src_x = cpa. x;
      src_y = cpa. y;
      w = cpa. width;
      h = cpa. height;
   }

   xr. x = src_x;
   xr. y = src_y;
   xr. width = w;
   xr. height = h;
   ir = xr;
   invalid = XCreateRegion();
   XUnionRectWithRegion( &xr, invalid, invalid);
   xr. x = dst_x;
   xr. y = dst_y;
   reg = XCreateRegion();
   XUnionRectWithRegion( &xr, reg, reg);
   if ( horiz == 0) {
      if ( vert < 0)
	 ir. height = -vert;
      else {
	 ir. y += ir. height - vert;
	 ir. height = vert;
      }
   } else if ( vert == 0) {
      if ( horiz < 0)
	 ir. width = -horiz;
      else {
	 ir. x += ir. width - horiz;
	 ir. width = horiz;
      }
   }
   XUnionRectWithRegion( &ir, invalid, invalid);
   XSubtractRegion( invalid, reg, invalid);
   XDestroyRegion( reg);

   XCopyArea( DISP, XX-> drawable, XX-> drawable, XX-> gc,
	      src_x, src_y, w, h, dst_x, dst_y);
   XCHECKPOINT;

   prima_release_gc( XX);

   if ( !XX-> region) {
      XX-> region = XCreateRegion();
      XX-> exposed_rect = ir;
   } else {
      XX-> exposed_rect. x += horiz;
      XX-> exposed_rect. y -= vert;
      prima_rect_union( &XX-> exposed_rect, &ir);
      XOffsetRegion( XX-> region, horiz, -vert);
   }
   XUnionRegion( XX-> region, invalid, XX-> region);
   XDestroyRegion( invalid);

   pl = guts. paint_list;
   while ( pl) {
      if ( pl-> obj == self)
	 return true;
      pl = pl-> next;
   }
   pl = malloc( sizeof( PaintList));
   if (!pl) croak( "no memory");
   pl-> obj = self;
   pl-> next = guts. paint_list;
   guts. paint_list = pl;
   return true;
}

Bool
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
   return true;
}

Bool
apc_widget_set_clip_rect( Handle self, Rect rect)
{
   SORT( rect. left, rect. right);
   SORT( rect. bottom, rect. top);
   X(self)-> scroll_rect = rect;
   return true;
}

Bool
apc_widget_set_color( Handle self, Color color, int index)
{
   X(self)-> colors[ index] = color;
   if ( index == ciFore)
      apc_gp_set_color( self, color);
   else if ( index == ciBack)
      apc_gp_set_back_color( self, color);
   return true;
}

Bool
apc_widget_set_enabled( Handle self, Bool enable)
{
   X(self)-> flags. enabled = enable;
   DOLBUG( "apc_widget_set_enabled( %d) of %s\n", enable, PWidget(self)->name);
   return true;
}

Bool
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   DOLBUG( "apc_widget_set_first_click()\n");
   return true;
}

Bool
apc_widget_set_focused( Handle self)
{
   if ( apc_widget_is_showing( self)) {
      XSetInputFocus( DISP, X_WINDOW, RevertToParent, CurrentTime);
      XCHECKPOINT;
      return true;
   } else {
      return false;
   }
}

Bool
apc_widget_set_font( Handle self, PFont font)
{
   Event ev = {cmFontChanged};

   apc_gp_set_font( self, font);

   ev. gen. source = self;
   CWidget(self)-> message( self, &ev);
   return true;
}

Bool
apc_widget_set_palette( Handle self)
{
   DOLBUG( "apc_widget_set_palette()\n");
   return true;
}

Bool
apc_widget_set_pos( Handle self, int x, int y)
{
   DEFXX;
   Event e;

   if ( x == XX-> origin. x && y == XX-> origin. y)
      return true;
   bzero( &e, sizeof( e));
   e. cmd = cmMove;
   e. gen. source = self;
   e. gen. P = XX-> origin;
   XX-> origin = (Point){x,y};  /* XXX ? */
   y = X(XX-> owner)-> size. y - XX-> size.y - y;
   XMoveWindow( DISP, X_WINDOW, x, y);
   DOLBUG( "XMoveWindow: widget (%s) move to (%d,%d)\n", PWidget(self)-> name, x, y);
   XCHECKPOINT;
   CComponent( self)-> message( self, &e);
   return true;
}

Bool
apc_widget_set_shape( Handle self, Handle mask)
{
   return true;
}

Bool
apc_widget_set_size( Handle self, int width, int height)
{
   DEFXX;
   int y;
   PWidget widg = PWidget( self);
   Event e;

   bzero( &e, sizeof( e));
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
   
   if ( XX-> size. x == width && XX-> size. y == height) {
      return true;
   }

   e. gen. source = self;
   e. cmd = cmSize;
   e. gen. R. left = XX-> size. x;
   e. gen. R. bottom = XX-> size. y;
   e. gen. P = XX-> size = (Point){width,height};
   e. gen. R. right = XX-> size. x;
   e. gen. R. top = XX-> size. y;
   y = X(XX-> owner)-> size. y - height - XX-> origin. y;
   XMoveResizeWindow( DISP, X_WINDOW, XX-> origin. x, y, width, height);
   DOLBUG( "widget (%s) size to (%d,%d) - (%d,%d) |old size %d,%d|\n",
	    PWidget(self)-> name, XX-> origin. x, y, width, height,
	    e. gen. R. left, e. gen. R. bottom);
   XCHECKPOINT;
   if (1){
      int count = PWidget( self)-> widgets. count;
      Handle *selves = malloc( count * sizeof( Handle));
      int i, stage;

      memcpy( selves, PWidget( self)-> widgets. items, count * sizeof( Handle));
      for ( i = 0; i < count; i++) {
	 PWidget child = PWidget( selves[i]);

	 if ( X(selves[i])-> flags. clip_owner && (child-> growMode & gmDontCare) == 0) {
	    stage = child-> stage;
	    child-> stage = csFrozen;
	    apc_widget_set_pos( selves[i], X(selves[i])-> origin. x, X(selves[i])-> origin. y);
	    child-> stage = stage;
	 }
      }
      free( selves);
   }
   CComponent( self)-> message( self, &e);
   return true;
}

Bool
apc_widget_set_visible( Handle self, Bool show)
{
   DEFXX;
   Bool flush_n_wait = false;

   if (!XX) return false;
   XX-> flags. mapped = show;
   if ( show) {
      if ( XX-> flags. do_size_hints) {
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
	 XX-> flags. do_size_hints = false;
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
   return true;
}

Bool
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
   XWindow windoze[2];

   /* top does not matter if behind is non-nil */
   if (behind) {
      windoze[0] = PComponent(behind)->handle;
      windoze[1] = X_WINDOW;
      XRestackWindows( DISP, windoze, 2);
      XCHECKPOINT;
   } else if (top) {
      XRaiseWindow( DISP, X_WINDOW);
      XCHECKPOINT;
   } else {
      XLowerWindow( DISP, X_WINDOW);
      XCHECKPOINT;
   }
   return true;
}

Bool
apc_widget_update( Handle self)
{
   Event e;
   if ( X(self)-> region) {
      e. cmd = cmPaint;
      CComponent( self)-> message( self, &e);
   }
   return true;
}

Bool
apc_widget_validate_rect( Handle self, Rect rect)
{
   DOLBUG( "apc_widget_validate_rect()\n");
   return true;
}

