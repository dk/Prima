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
/*  System dependent menu routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "AbstractMenu.h"
#include "Image.h"
#include "Application.h"

#undef DEFXX
#define Y(obj)          ((PMenuSysData)(PComponent((obj))-> sysData))
#define DEFXX           PMenuSysData selfxx = Y(self)
#define TREE            (PAbstractMenu(self)->tree)

static PMenuWindow
get_menu_window( Handle self, XWindow xw)
{
   DEFXX;
   PMenuWindow w = XX-> w;
   while (w && w->w != xw)
      w = w-> next;
   return w;
}

static PMenuWindow
get_window( Handle self, PMenuSysData XX, PMenuItemReg m)
{
   PMenuWindow w = XX-> w;
   XSetWindowAttributes attrs;
   
   while (w != nil && w-> m != m)
      w = w-> next;
   if (!w) {
      w = malloc( sizeof( MenuWindow));
      if (!w) croak( "get menu window: no memory");
      bzero(w, sizeof( MenuWindow));
      w->self = self;
      w->m = m;
      w->sz.x = -1;
      w->sz.y = -1;
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
      w->w = XCreateWindow( DISP, RootWindow(DISP,SCREEN),
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
      if (!w->w) {
         free(w);
         return nil;
      }
      XCHECKPOINT;
      hash_store( guts.menu_windows, &w->w, sizeof(w->w), (void*)self);
      w->next = XX->w;
      w->prev = nil;
      if (XX->w) XX->w->prev = w;
      XX->w = w;
   }
   return w;
}

static int
item_count( PMenuWindow w)
{
   int i = 0;
   PMenuItemReg m = w->m;

   while (m) {
      i++; m=m->next;
   }
   return i;
}

static void
free_unix_items( PMenuWindow w)
{
   int i;

   for ( i = 0; i < w->num; i++) {
      free( w->um[i]. text);
   }
   free( w-> um);
   w-> um = nil;
   w-> num = 0;
}

static void
update_menu_window( PMenuSysData XX, PMenuWindow w)
{
   int x = 9, y = 6, xx;
   PMenuItemReg m = w->m;
   int sz = 1024, l, i, k;
   char *s = malloc( sz);
   char *t;
   
   free_unix_items( w);
   w-> num = item_count( w);
   w-> um = malloc( sizeof( UnixMenuItem) * w-> num);
   bzero( w-> um, sizeof( UnixMenuItem) * w-> num);
   
   while ( m) {
      if ( m-> divider) {
         y += 8;
      } else if ( m-> text) {
         y += 9 + XX-> font-> font. height;
         t = m-> text;
         for (;;) {
            l = 0; i = 0;
            while ( l < sz && t[i]) {
               if (t[i] == '\t') {
                  if ( l + 8 >= sz)
                     break;
                  for (k=0; k < 8; k++) {
                     s[l++] = ' ';
                  }
                  i++;
               } else if (t[i] == '~') {
                  if ( t[i+1] == '~') {
                     s[l++] = '~'; i += 2;
                  } else
                     i++;
               } else {
                  s[l++] = t[i++];
               }
            }
            if ( t[i]) {
               free(s); s = malloc( sz *= 2);
            } else
               break;
         }
         xx = 9+12+12 + XTextWidth( XX-> font-> fs, s, l);
         if ( xx > x) x = xx;
      } else if ( m-> bitmap) {
      } else {
         free(s);
         croak( "update menu window: fatal internal inconsistency");
      }
      m = m-> next;
   }
   
   w-> sz.x = x;
   w-> sz.y = y;
   XResizeWindow( DISP, w->w, x, y);
   free(s);
}

void
prima_handle_menu_event( XEvent *ev, XWindow win, Handle self)
{
   DEFXX;
   PMenuWindow w;
   GC gc;
   int mx, my, y;
   PMenuItemReg m;
   int sz = 1024, l, i, k;
   char *s;
   char *t;

   switch ( ev-> type) {
   case Expose:
      w = get_menu_window( self, win);
      if (w) {
         m = w->m;
         gc = guts.menugc;
         mx = w->sz.x-1;
         my = w->sz.y-1;
         XSetForeground( DISP, gc, XX->c[ciBack].pixel);
         XFillRectangle( DISP, win, gc, 0, 0, mx, my);
         XSetForeground( DISP, gc, XX->c[ciDark3DColor].pixel);
         XDrawLine( DISP, win, gc, 0, 0, 0, my);
         XDrawLine( DISP, win, gc, 0, 0, mx-1, 0);
         XSetForeground( DISP, gc, BlackPixel( DISP, SCREEN));
         XDrawLine( DISP, win, gc, mx, my, 1, my);
         XDrawLine( DISP, win, gc, mx, my, mx, 0);
         XSetForeground( DISP, gc, XX->c[ciLight3DColor].pixel);
         XDrawLine( DISP, win, gc, 1, 1, 1, my-1);
         XDrawLine( DISP, win, gc, 1, 1, mx-2, 1);
         XSetForeground( DISP, gc, XX->c[ciDark3DColor].pixel);
         XDrawLine( DISP, win, gc, mx-1, my-1, 2, my-1);
         XDrawLine( DISP, win, gc, mx-1, my-1, mx-1, 1);
         XSetForeground( DISP, gc, XX->c[ciFore].pixel);
         XSetBackground( DISP, gc, XX->c[ciBack].pixel);
         XSetFont( DISP, gc, XX-> font-> id);

         y = 3;
         s = malloc( sz);
         while ( m) {
            if ( m-> divider) {
               y += 3;
               XSetForeground( DISP, gc, XX->c[ciDark3DColor].pixel);
               XDrawLine( DISP, win, gc, 1, y, mx-1, y);
               y++;
               XSetForeground( DISP, gc, XX->c[ciLight3DColor].pixel);
               XDrawLine( DISP, win, gc, 1, y, mx-1, y);
               y += 4;
               XSetForeground( DISP, gc, XX->c[ciFore].pixel);
            } else if ( m-> text) {
               y += 4 + XX-> font-> font. height;
               t = m-> text;
               for (;;) {
                  l = 0; i = 0;
                  while ( l < sz && t[i]) {
                     if (t[i] == '\t') {
                        if ( l + 8 >= sz)
                           break;
                        for (k=0; k < 8; k++) {
                           s[l++] = ' ';
                        }
                        i++;
                     } else if (t[i] == '~') {
                        if ( t[i+1] == '~') {
                           s[l++] = '~'; i += 2;
                        } else
                           i++;
                     } else {
                        s[l++] = t[i++];
                     }
                  }
                  if ( t[i]) {
                     free(s); s = malloc( sz *= 2);
                  } else
                     break;
               }
               XDrawString( DISP, win, gc, 5+12, y, s, l);
               y += 5;
            } else if ( m-> bitmap) {
            } else {
               croak( "update menu window: fatal internal inconsistency");
            }
            m = m-> next;
         }
      }
      break;
   }
}

Bool
apc_menu_create( Handle self, Handle owner)
{
   X(self)-> type.menu = true;
   return true;
}

Bool
apc_menu_destroy( Handle self)
{
   return true;
}

PFont
apc_menu_default_font( PFont f)
{
   return apc_font_default( f);
}

Color
apc_menu_get_color( Handle self, int index)
{
   return 0;
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
   return nil;
}

Bool
apc_menu_set_color( Handle self, Color color, int i)
{
   DEFXX;

   if ( i >= 0 && i <= ciMaxId) {
      XX-> c[i] = *prima_allocate_color( self, color);
      return true;
   } else {
      return false;
   }
}

/* apc_menu_set_font is in apc_font.c */

Bool
apc_menu_update( Handle self, PMenuItemReg oldBranch, PMenuItemReg newBranch)
{
   return true;
}

Bool
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
   return true;
}

Bool
apc_menu_item_set_accel( Handle self, PMenuItemReg m, const char * accel)
{
   return true;
}

Bool
apc_menu_item_set_check( Handle self, PMenuItemReg m, Bool check)
{
   return true;
}

Bool
apc_menu_item_set_enabled( Handle self, PMenuItemReg m, Bool enabled)
{
   return true;
}

Bool
apc_menu_item_set_image( Handle self, PMenuItemReg m, Handle image)
{
   return true;
}

Bool
apc_menu_item_set_key( Handle self, PMenuItemReg m, int key)
{
   return true;
}

Bool
apc_menu_item_set_text( Handle self, PMenuItemReg m, const char * text)
{
   return true;
}

ApiHandle
apc_menu_get_handle( Handle self)
{
   return nilHandle;
}

Bool
apc_popup_create( Handle self, Handle owner)
{
   DEFXX;
   XX-> type.menu = true;
   XX-> type.popup = true;
   return true;
}

PFont
apc_popup_default_font( PFont f)
{
   return apc_font_default( f);
}

Bool
apc_popup( Handle self, int x, int y, Rect *anchor)
{
   DEFXX;
   PMenuItemReg m;
   PMenuWindow w;
   XWindow dummy;
   PDrawableSysData owner;
   

   if (!(m=TREE)) return false;
   if (!(w = get_window(self,XX,m))) return false;
   if (w->sz.x <= 0) update_menu_window(XX,w);
   fprintf( stderr, "calculated size is %d, %d\n", w->sz.x, w->sz.y);
   owner = X(PComponent(self)->owner);
   y = owner->size.y - w->sz.y - y;
   if ( !XTranslateCoordinates( DISP, owner->udrawable, RootWindow( DISP, SCREEN),
                                x, y, &x, &y, &dummy)) {
      croak( "apc_popup(): XTranslateCoordinates() failed");
   }
   XMoveWindow( DISP, w->w, x, y);
   XMapWindow( DISP, w->w);
   XRaiseWindow( DISP, w->w);
   XFlush( DISP);
   XCHECKPOINT;
   return false;
}

