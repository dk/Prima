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
/*  System dependent widget management (unix, x11)         */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Application.h"

#define SORT(a,b)       ({ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }})
#define REVERT(a)       ({ XX-> size. y - (a) - 1; })

Bool
apc_widget_map_points( Handle self, Bool toScreen, int n, Point *p)
{
   Point d = {0,0};
   
   while ( self && (self != application)) {
      DEFXX;
      Point origin;
      if ( XX-> parentHandle) {
         XWindow dummy;
         XTranslateCoordinates( DISP, X_WINDOW, guts. root, 0, XX-> size.y-1, &origin.x, &origin.y, &dummy);
         origin. y = guts. displaySize. y - origin. y;
         self = application;
      } else {
         origin = XX-> origin;
         self = XX-> flags. clip_owner ? PWidget(self)-> owner : application;
      }
      d. x += origin. x;
      d. y += origin. y;
   }
   
   if ( !toScreen) {
      d. x = -d. x;
      d. y = -d. y;
   }
   
   while (n--) {
      p[n]. x += d. x;
      p[n]. y += d. y;
   }
   return true;
}

ApiHandle
apc_widget_get_parent_handle( Handle self)
{
   return X(self)-> parentHandle;
}

Handle
apc_widget_get_z_order( Handle self, int zOrderId)
{
   XWindow root, parent, *children;
   unsigned int count;
   int i, inc;
   Handle ret = nilHandle;

   if ( !( PComponent(self)-> owner))
      return self;

   switch ( zOrderId) {
   case zoFirst:
      i = 1;
      inc = -1;
      break;
   case zoLast:
      i  = 1;
      inc = 1;
      break;
   case zoNext:
      i = 0;
      inc = -1;
      break;
   case zoPrev:
      i = 0;
      inc = 1;
      break;
   default:
      return nilHandle;
   }

   if ( XQueryTree( DISP, PComponent(PComponent(self)-> owner)-> handle, 
      &root, &parent, &children, &count) == 0)
         return nilHandle;

   if ( count == 0) goto EXIT;

   if ( i == 0) {
      int found = -1;
      for ( i = 0; i < count; i++) {
         if ( children[ i] == X_WINDOW) {
            found = i;
            break;
         }   
      }   
      if ( found < 0) { // if !clipOwner
         ret = self;
         goto EXIT; 
      }   
      i = found + inc;
      if ( i < 0 || i >= count) goto EXIT; // last in line
   } else
      i = ( zOrderId == zoFirst) ? count - 1 : 0;
   
   while ( 1) {
      Handle who = ( Handle) hash_fetch( guts. windows, (void*)&(children[i]), sizeof(X_WINDOW));
      if ( who) {
         ret = who;
         break;
      }   
      i += inc;
      if ( i < 0 || i >= count) break;
   }   

EXIT:   
   if ( children) XFree( children);
   return ret;
}   

typedef struct _RecreateData {
  Point    pos;
  Point    size;
  Point    virtSize;
  Bool     enabled;
  Bool     visible;
  Bool     focused;
  Bool     capture;
} RecreateData, *PRecreateData;

static void
get_view_ex( Handle self, PRecreateData p)
{
  p-> capture   = apc_widget_is_captured( self);
  p-> pos       = apc_widget_get_pos( self);
  p-> size      = apc_widget_get_size( self);
  p-> virtSize  = PWidget(self)-> virtualSize;
  p-> enabled   = apc_widget_is_enabled( self);
  p-> focused   = apc_widget_is_focused( self);
  p-> visible   = apc_widget_is_visible( self);
}


static void
set_view_ex( Handle self, PRecreateData p)
{
  DEFXX; 
  XX-> origin. x = XX-> origin. y = XX-> size. x = XX-> size. y = INT_MAX; // force reset
  apc_widget_set_visible( self, false);
  apc_widget_set_pos( self, p-> pos. x, p-> pos. y);
  apc_widget_set_size( self, p-> size. x, p-> size. y);
  PWidget(self)-> virtualSize = p-> virtSize;
  apc_widget_set_enabled( self, p-> enabled);
  apc_widget_set_visible( self, p-> visible);
  if ( p-> focused) {
     if ( guts. focused == self) guts. focused = nilHandle;
     apc_widget_set_focused( self);
  }   
  if ( p-> capture) apc_widget_set_capture( self, 1, nilHandle);
  XClearWindow( DISP, X_WINDOW);
}

Bool
apc_widget_create( Handle self, Handle owner, Bool sync_paint,
		   Bool clip_owner, Bool transparent, ApiHandle parentHandle)
{
   DEFXX;
   Bool reset;
   Handle real_owner;
   XWindow parent, old = X_WINDOW;
   XSetWindowAttributes attrs;
   Handle * list = PWidget( self)-> widgets. items;
   int count = PWidget( self)-> widgets. count;
   RecreateData vprf;

   reset = ( old != nilHandle) && ( 
      ( clip_owner != XX-> flags. clip_owner) ||
      ( parentHandle != XX-> parent)
   );

   if ( reset) {
      int i;
      CWidget( self)-> end_paint_info( self);
      CWidget( self)-> end_paint( self);
      for( i = 0; i < count; i++)
         get_view_ex( list[ i], ( RecreateData*)( X( list[ i])-> recreateData = malloc( sizeof( RecreateData))));
      
      if ( XX-> recreateData) {
         memcpy( &vprf, XX-> recreateData, sizeof( vprf));
         free( XX-> recreateData);
         XX-> recreateData = nil;
      } else
         get_view_ex( self, &vprf);
   }   

   /* Transparency is ignored for now */

   XX-> type.drawable = true;
   XX-> type.widget = true;
   if ( !clip_owner || ( owner == application)) {
      parent = guts. root;
      real_owner = application;
   } else {
      parent = PWidget( owner)-> handle;
      real_owner = owner;
   }
   
   if ( parentHandle)
      parent = parentHandle;
   XX-> parentHandle = parentHandle;

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
   attrs. override_redirect = true;
   attrs. do_not_propagate_mask = attrs. event_mask;
   attrs. win_gravity = ( clip_owner && ( owner != application)) 
      ? SouthWestGravity : NorthWestGravity;
   X_WINDOW = XCreateWindow( DISP, parent,
	                     0, 0, 1, 1, 0, CopyFromParent,
	                     InputOutput, CopyFromParent,
	                     0
	                     /* | CWBackPixmap */
	                     /* | CWBackPixel */
	                     /* | CWBorderPixmap */
	                     /* | CWBorderPixel */
	                     /* | CWBitGravity */
	                     | CWWinGravity
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

   if ( reset) {
      hash_delete( guts.windows, &old, sizeof(X_WINDOW), false);
   } else {
      XX-> size = (Point){0,0};
   }   
   hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);

   XX-> real_parent = XX-> parent = parent;
   XX-> gdrawable = XX-> udrawable = X_WINDOW;

   XX-> flags. clip_owner = clip_owner;
   XX-> flags. sync_paint = sync_paint;
   XX-> flags. process_configure_notify = false;
   
   XX-> above = nilHandle;
   XX-> owner = real_owner;

   apc_component_fullname_changed_notify( self);
   
   if ( reset) {
      int i, stage = PWidget(self)-> stage;
      Handle oldOwner = PWidget(self)-> owner;  PWidget(self)-> owner = owner;
      PWidget(self)-> stage = csFrozen;
      if ( XX-> flags. paint_pending) {
         TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
         XX-> flags. paint_pending = false;
      }
      XX-> flags. falsely_hidden = 0;
      set_view_ex( self, &vprf);
      XCHECKPOINT;
      PWidget(self)-> owner = oldOwner;
      for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
      XDestroyWindow( DISP, old);
      XCHECKPOINT;
      if ( XX-> pointer_id == crUser)
         XDefineCursor( DISP, XX-> udrawable, XX-> user_pointer);
      else
         apc_pointer_set_shape( self, XX-> pointer_id);
      PWidget(self)-> stage = stage;
   } else
      prima_send_create_event( X_WINDOW);

   return true;
}

Bool
apc_widget_begin_paint( Handle self, Bool inside_on_paint)
{
   if ( guts. dynamicColors && inside_on_paint) prima_palette_free( self, false);
   prima_no_cursor( self);
   prima_prepare_drawable_for_painting( self);
   return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
   prima_no_cursor( self);
   prima_prepare_drawable_for_painting( self);
   return true;
}

Bool
apc_widget_destroy( Handle self)
{
   DEFXX;

   if ( guts. focused == self)
      guts. focused = nilHandle;
   XX-> flags.modal = false;
   if ( XX-> flags. paint_pending) {
      TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
      XX-> flags. paint_pending = false;
   }
   if ( XX-> recreateData) free( XX-> recreateData);
   free(XX-> dashes);
   XX-> dashes = nil;
   XX-> ndashes = 0;
   free(XX->paint_dashes);
   XX-> paint_dashes = nil;
   XX-> paint_ndashes = 0;
   if ( X_WINDOW) {
      if ( guts. grab_redirect == X_WINDOW) {
         XUngrabPointer( DISP, CurrentTime);
         guts. grab_redirect = nilHandle;
      }   
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
   prima_cleanup_drawable_after_painting( self);
   prima_update_cursor( self);
   return true;
}

Bool
apc_widget_get_clip_owner( Handle self)
{
   return X(self)-> flags. clip_owner;
}

Color
apc_widget_get_color( Handle self, int index)
{
   return X(self)-> colors[ index];
}

Bool
apc_widget_get_first_click( Handle self)
{
   return X(self)-> flags. first_click ? true : false;
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
   DEFXX;
   XRectangle r;
   if ( !XX-> region)
      return (Rect){0,0,0,0};
   XClipBox( XX-> region, &r);
   return (Rect){r.x, XX-> size.y - r.height - r.y, r.x + r.width, XX-> size.y - r.y};
}

Point
apc_widget_get_pos( Handle self)
{
   DEFXX;
   XWindow r;
   int x, y, w, h, d, b;

   if ( XX-> type. window) {
      Rect rc;
      Point p = apc_window_get_client_pos( self);
      prima_get_frame_info( self, &rc);
      p. x -= rc. left;
      p. y -= rc. bottom;
      return p;
   }   
      
   if ( XX-> parentHandle == nilHandle)
      return PWidget(self)-> pos;
   
   XGetGeometry( DISP, X_WINDOW, &r, &x, &y, &w, &h, &b, &d);
   XTranslateCoordinates( DISP, XX-> parentHandle, guts. root, x, y, &x, &y, &r);
   y = DisplayHeight( DISP, SCREEN) - y - w; 
   return ( Point) { x, y};    
}

Bool
apc_widget_get_shape( Handle self, Handle mask)
{
   return false;
}

Point
apc_widget_get_size( Handle self)
{
   DEFXX;
   if ( XX-> type. window) {
      Rect rc;
      Point p = apc_window_get_client_size( self);
      prima_get_frame_info( self, &rc);
      p. x += rc. left + rc. right;
      p. y += rc. bottom + rc. top;
      return p;
   }   
   
   return XX-> size;
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
   return X(self)-> flags. grab ? true : false;
}

Bool
apc_widget_is_enabled( Handle self)
{
   return XF_ENABLED(X(self)) ? true : false;
}

Bool
apc_widget_is_exposed( Handle self)
{
   return X(self)-> flags. exposed ? true : false;
}

Bool
apc_widget_is_focused( Handle self)
{
   return guts. focused == self;
}

Bool
apc_widget_is_responsive( Handle self)
{
   Bool ena = true;
   while ( ena && self != application) {
      ena  = XF_ENABLED(X(self)) ? true : false;
      self = PWidget(self)-> owner;
   }
   return ena;
}

Bool
apc_widget_is_showing( Handle self)
{
   XWindowAttributes attrs;
   DEFXX;

   if ( XX && XX-> flags. mapped
	&& XGetWindowAttributes( DISP, XX->udrawable, &attrs)
	&& attrs. map_state == IsViewable)
      return true;
   else
      return false;
}

Bool
apc_widget_is_visible( Handle self)
{
   return X(self)-> flags. want_visible ? true : false;
}

Bool
apc_widget_invalidate_rect( Handle self, Rect *rect)
{
   XRectangle r;
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
      if ( !XX-> flags. paint_pending) {
         TAILQ_INSERT_TAIL( &guts.paintq, XX, paintq_link);
         XX-> flags. paint_pending = true;
      }
   }

   XUnionRectWithRegion( &r, XX-> region, XX-> region);
   if ( XX-> flags. sync_paint) {
      apc_widget_update( self);
   }
   return true;
}

Bool
apc_widget_scroll( Handle self, int horiz, int vert,
		   Rect *confine, Rect *clip, Bool withChildren)
{
   DEFXX;
   int src_x, src_y, w, h, dst_x, dst_y;
   XRectangle r;
   Region invalid, reg;

   prima_no_cursor( self);
   prima_get_gc( XX);
   XX-> gcv. clip_mask = None;
   XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &XX-> gcv);
   XCHECKPOINT;
   
   if ( confine) {
      src_x = confine-> left;
      src_y = XX-> size. y - confine-> top;
      w = confine-> right - src_x;
      h = confine-> top - confine-> bottom;
   } else {
      src_x = 0;
      src_y = 0;
      w = XX-> size. x;
      h = XX-> size. y;
   }

   dst_x = src_x + horiz;
   dst_y = src_y - vert;

   if (clip) {
      XRectangle cpa;
     
      r. x = clip-> left;
      r. y = REVERT( clip-> top) + 1;
      r. width = clip-> right - clip-> left;
      r. height = clip-> top - clip-> bottom;
      reg = XCreateRegion();
      XUnionRectWithRegion( &r, reg, reg);
      XSetRegion( DISP, XX-> gc, reg);
      XCHECKPOINT;
      XDestroyRegion( reg);
      cpa. x = src_x;
      cpa. y = src_y;
      cpa. width = w;
      cpa. height = h;
      prima_rect_intersect( &cpa, &r);
      dst_x += -src_x + cpa. x;
      dst_y += -src_y + cpa. y;
      src_x = cpa. x;
      src_y = cpa. y;
      w = cpa. width;
      h = cpa. height;
   }

   XCopyArea( DISP, XX-> udrawable, XX-> udrawable, XX-> gc,
	      src_x, src_y, w, h, dst_x, dst_y);
   prima_release_gc( XX);
   XCHECKPOINT;

   r. x = src_x;
   r. y = src_y;
   r. width = w;
   r. height = h;
   invalid = XCreateRegion();
   XUnionRectWithRegion( &r, invalid, invalid);

   if ( XX-> region) {
      reg = XCreateRegion();
      XUnionRegion( XX-> region, reg, reg);
      XIntersectRegion( reg, invalid, reg);
      XSubtractRegion( XX-> region, reg, XX-> region);
      XOffsetRegion( reg, horiz, -vert);
      XUnionRegion( XX-> region, reg, XX-> region);
      XDestroyRegion( reg);
   } else {
      XX-> region = XCreateRegion();
   }

   r. x = dst_x;
   r. y = dst_y;
   reg = XCreateRegion();
   XUnionRectWithRegion( &r, reg, reg);
   XSubtractRegion( invalid, reg, invalid);
   XDestroyRegion( reg);
   XUnionRegion( XX-> region, invalid, XX-> region);
   XDestroyRegion( invalid);
   if ( !XX-> flags. paint_pending) {
      TAILQ_INSERT_TAIL( &guts.paintq, XX, paintq_link);
      XX-> flags. paint_pending = true;
   }
   return true;
}

Bool
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
   int r;
   XWindow confine_to = None;
   DEFXX;

   if ( capture) {
      XWindow z = X_WINDOW;
      if ( confineTo && PWidget(confineTo)-> handle)
	 confine_to = PWidget(confineTo)-> handle;
AGAIN:      
      r = XGrabPointer( DISP, z, false, 0
			| ButtonPressMask
			| ButtonReleaseMask
			| PointerMotionMask
			| ButtonMotionMask, GrabModeAsync, GrabModeAsync,
			confine_to, None, CurrentTime);
      XCHECKPOINT;
      if ( r != GrabSuccess) {
         XWindow root = guts. root, rx;
         if (( r == GrabNotViewable) && ( root != z)) {
            XTranslateCoordinates( DISP, z, guts. root, 0, 0, 
                &guts. grab_translate_mouse.x, &guts. grab_translate_mouse.y, &rx);
            guts. grab_redirect = z;
            z = root;
            goto AGAIN;
         }  
         guts. grab_redirect = nilHandle;
         return false;
      } else {
	 XX-> flags. grab = true;
      }
   } else {
      guts. grab_redirect = nilHandle;
      XUngrabPointer( DISP, CurrentTime);
      XCHECKPOINT;
      XX-> flags. grab = false;
   }
   return true;
}

Bool
apc_widget_set_color( Handle self, Color color, int i)
{
   Event e = {cmColorChanged};

   X(self)-> colors[ i] = color;
   if ( i == ciFore)
      apc_gp_set_color( self, color);
   else if ( i == ciBack)
      apc_gp_set_back_color( self, color);

   bzero( &e, sizeof(e));
   e. gen. source = self;
   e. gen. i      = i;
   apc_message( self, &e, false);

   return true;
}

Bool
apc_widget_set_enabled( Handle self, Bool enable)
{
   DEFXX;

   if ( enable == XF_ENABLED(XX)) return true;
   XF_ENABLED(XX) = enable;
   prima_simple_message(self, enable ? cmEnable : cmDisable, false);
   return true;
}

Bool
apc_widget_set_first_click( Handle self, Bool firstClick)
{
   X(self)-> flags. first_click = firstClick ? 1 : 0;
   return true;
}

Bool
apc_widget_set_focused( Handle self)
{
   XWindow focus = None;
   if ( guts. message_boxes) return false;
   if ( self && ( self != CApplication( application)-> map_focus( application, self)))
      return false;
   if ( self) {
      if (XT_IS_WINDOW(X(self))) return true; /* already done in activate() */
      focus = X_WINDOW;
   }
   XSetInputFocus( DISP, focus, RevertToParent, CurrentTime);
   XCHECKPOINT;
   apc_application_yield();
   return true;
}

Bool
apc_widget_set_font( Handle self, PFont font)
{
   apc_gp_set_font( self, font);
   prima_simple_message( self, cmFontChanged, false);
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

   if ( XX-> type. window) {
      Rect rc;
      prima_get_frame_info( self, &rc);
      return apc_window_set_client_pos( self, x + rc. left, y + rc. bottom);
   }   
   
   if ( XX-> parentHandle == nilHandle && x == XX-> origin.x && y == XX-> origin. y)
      return true;
   bzero( &e, sizeof( e));
   e. cmd = cmMove;
   e. gen. source = self;
   XX-> origin = e. gen. P = (Point){x,y};
   y = X(XX-> owner)-> size. y - XX-> size.y - y;
   if ( XX-> parentHandle) {
      XWindow cld;
      XTranslateCoordinates( DISP, PWidget(XX-> owner)-> handle, XX-> parentHandle, x, y, &x, &y, &cld);
   } 
   XMoveWindow( DISP, X_WINDOW, x, y);
   XCHECKPOINT;
   apc_message( self, &e, false);
   return true;
}

Bool
apc_widget_set_shape( Handle self, Handle mask)
{
   return true;
}

void
prima_send_cmSize( Handle self, Point oldSize)
{
   DEFXX;
   Event e;

   bzero( &e, sizeof(e));
   e. gen. source = self;
   e. cmd = cmSize;
   e. gen. R. left = oldSize. x;
   e. gen. R. bottom = oldSize. y;
   e. gen. P. x = e. gen. R. right = XX-> size. x;
   e. gen. P. y = e. gen. R. top = XX-> size. y;
   {
      int i, y = XX-> size. y, count = PWidget( self)-> widgets. count;
      for ( i = 0; i < count; i++) {
	 PWidget child = PWidget( PWidget( self)-> widgets. items[i]);
         if ((( PWidget(child)-> growMode & gmDontCare) == 0) &&
             ( X(child)-> flags. clip_owner || ( child-> owner == application)))
            XMoveWindow( DISP, child-> handle, X(child)-> origin.x, y - X(child)-> size.y - X(child)-> origin. y);
      }
   }
   apc_message( self, &e, false);
}   

Bool
apc_widget_set_size( Handle self, int width, int height)
{
   DEFXX;
   Point sz = XX-> size;
   PWidget widg = PWidget( self);
   int x, y;

   if ( XX-> type. window) {
      Rect rc;
      prima_get_frame_info( self, &rc);
      return apc_window_set_client_size( self, width - rc. left - rc. right, height - rc. bottom - rc. top);
   }   
   
   widg-> virtualSize = (Point){width,height};

   width = ( width > 0)
      ? (( width >= widg-> sizeMin. x)
	  ? (( width <= widg-> sizeMax. x)
	      ? width
	      : widg-> sizeMax. x)
	  : widg-> sizeMin. x)
      : 0;

   height = ( height > 0)
      ? (( height >= widg-> sizeMin. y)
	  ? (( height <= widg-> sizeMax. y)
	      ? height
	      : widg-> sizeMax. y)
	  : widg-> sizeMin. y)
      : 0;
   
   if ( XX-> parentHandle == nilHandle && XX-> size. x == width && XX-> size. y == height)
      return true;

   XX-> size. x = width;
   XX-> size. y = height;

   x = XX-> origin. x;
   y = X(XX-> owner)-> size. y - XX-> size.y - XX-> origin. y;
   if ( XX-> parentHandle) {
      XWindow cld;
      XTranslateCoordinates( DISP, PWidget(XX-> owner)-> handle, XX-> parentHandle, x, y, &x, &y, &cld);
   } 
   if ( width != 0 && height != 0) {
      XMoveResizeWindow( DISP, X_WINDOW, x, y, width, height);
      if ( XX-> flags. falsely_hidden) {
         if ( XX-> flags. want_visible) XMapWindow( DISP, X_WINDOW);
         XX-> flags. falsely_hidden = 0;
      }   
   } else {
      if ( XX-> flags. want_visible) XUnmapWindow( DISP, X_WINDOW);  
      XMoveResizeWindow( DISP, X_WINDOW, x, y, 1, 1);
      XX-> flags. falsely_hidden = 1;
   }   
   prima_send_cmSize( self, sz);
   return true;
}

Bool
apc_widget_set_size_bounds( Handle self, Point min, Point max)
{
   DEFXX;
   if ( XX-> type. window) { 
      XSizeHints hints;
      bzero( &hints, sizeof( hints));
      hints. flags = PMinSize | PMaxSize;
      if ( XX-> flags. sizeable) {
         hints. min_width  = min. x;
         hints. min_height = min. y;
         hints. max_width  = max. x;
         hints. max_height = max. y;
      } else {   
         hints. min_width  = XX-> size. x;
         hints. min_height = XX-> size. y;
         hints. max_width  = XX-> size. x;
         hints. max_height = XX-> size. y;
      }
      XSetWMNormalHints( DISP, X_WINDOW, &hints);
      XCHECKPOINT;
   }
   return true;
}   

Bool
apc_widget_set_visible( Handle self, Bool show)
{
   DEFXX;
   int oldShow;
   if ( XX-> type. window)
      return apc_window_set_visible( self, show);
   
   oldShow = XX-> flags. want_visible ? 1 : 0;
   XX-> flags. want_visible = show;
   if ( !XX-> flags. falsely_hidden) {
      if ( show)
         XMapWindow( DISP, X_WINDOW);
      else
         XUnmapWindow( DISP, X_WINDOW);
      XCHECKPOINT;
   }
   if ( oldShow != ( show ? 1 : 0))
      prima_simple_message(self, show ? cmShow : cmHide, false);
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

   if ( X(self)-> type. window) 
      prima_wm_sync( self, ConfigureNotify);
   else 
      prima_simple_message( self, cmZOrderChanged, false);
   return true;
}

Bool
apc_widget_update( Handle self)
{
   DEFXX;

   if ( XX-> region) {
      if ( XX-> flags. paint_pending) {
         TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
         XX-> flags. paint_pending = false;
      }
      prima_simple_message( self, cmPaint, false);
   }
   return true;
}

Bool
apc_widget_validate_rect( Handle self, Rect rect)
{
   XRectangle r;
   DEFXX;
   Region rgn;

   r. x = rect. left;
   r. width = rect. right - rect. left;
   r. y = XX-> size. y - rect. top;
   r. height = rect. top - rect. bottom;

   if ( !XX-> region) 
      return true;
   
   if ( !( rgn = XCreateRegion())) 
      return false;
   
   XUnionRectWithRegion( &r, rgn, rgn);
   XSubtractRegion( XX-> region, rgn, XX-> region);
   XDestroyRegion( rgn);

   if ( XEmptyRegion( XX-> region)) {
      if ( XX-> flags. paint_pending) {
         TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
         XX-> flags. paint_pending = false;
      }
      XDestroyRegion( XX-> region);
      XX-> region = nil;
   }   
   return true;
}

