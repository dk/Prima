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
#include "Menu.h"
#include "Image.h"
#include "Window.h"
#include "Application.h"

//#undef DEFXX
//#define Y(obj)          ((PMenuSysData)(PComponent((obj))-> sysData))
//#define DEFXX           PMenuSysData selfxx = Y(self)
#define TREE            (PAbstractMenu(self)->tree)


static PMenuWindow
get_menu_window( Handle self, XWindow xw)
{
   DEFMM;
   PMenuWindow w = XX-> w;
   while (w && w->w != xw)
      w = w-> next;
   return w;
}


/*
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
         | ButtonMotionMask
         | KeymapStateMask
         | ExposureMask
         | VisibilityChangeMask
         | StructureNotifyMask
         | FocusChangeMask
         | PropertyChangeMask
         | ColormapChangeMask
         | OwnerGrabButtonMask;
      attrs. override_redirect = true;
      attrs. do_not_propagate_mask = attrs. event_mask;
      w->w = XCreateWindow( DISP, guts. root,
                            0, 0, 1, 1, 0, CopyFromParent,
                            InputOutput, CopyFromParent,
                            0
                            | CWOverrideRedirect
                            | CWEventMask
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
*/


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

   if ( w-> first < 0) {
      for ( i = 0; i < w->num; i++) 
         if ( w-> um[i].pixmap)
            XFreePixmap( DISP, w-> um[i].pixmap);
   }
   free( w-> um);
   w-> um = nil;
   w-> num = 0;
}

static XCharStruct * 
char_struct( XFontStruct * xs, int c) 
{
    XCharStruct * xc;
    if ( !xs-> per_char) {
       xc = &xs-> min_bounds;
    } else if (c < xs-> min_char_or_byte2 || c > xs-> max_char_or_byte2) {
       xc = xs-> per_char + xs-> default_char - xs-> min_char_or_byte2; 
    } else
       xc = xs-> per_char + c - xs-> min_char_or_byte2;
    return xc;
}   

#define MENU_XOFFSET 5
#define MENU_CHECK_XOFFSET 10
#define VMENU_YMIN 6
#define VMENU_SUBMENU_WCOEFF  4

static void
update_menu_window( PMenuSysData XX, PMenuWindow w)
{
   int x = MENU_XOFFSET * 2, y = VMENU_YMIN, startx;
   Bool vertical = w != &XX-> wstatic;
   PMenuItemReg m = w->m;
   PUnixMenuItem ix;
   
   free_unix_items( w);
   w-> num = item_count( w);
   ix = w-> um = malloc( sizeof( UnixMenuItem) * w-> num);
   bzero( w-> um, sizeof( UnixMenuItem) * w-> num);

   if ( vertical) // submenu space 
      x += char_struct( XX-> font-> fs, ' ')-> width * VMENU_SUBMENU_WCOEFF;
   startx = x;
   
   while ( m) {
      if ( m-> divider) {
         if ( vertical) ix-> height = MENU_ITEM_GAP * 2;
      } else {
         int l = 0;
         if ( m-> text) {
            int i, ntildas = 0;
            char * t = m-> text;
            ix-> height = MENU_ITEM_GAP * 2 + XX-> font-> font. height;
            for ( i = 0; t[i]; i++) {
               if ( t[i] == '~' && t[i+1]) {
                  ntildas++;
                  if ( t[i+1] == '~') 
                     i++;
               }   
            }   
            ix-> width += startx + XTextWidth( XX-> font-> fs, m-> text, i);
            if ( ntildas)
               ix-> width -= char_struct( XX-> font-> fs, '~')-> width * ntildas; 
         } else if ( m-> bitmap) {
            /* XXX dynamicColors */
            Handle h = CImage( m-> bitmap)-> bitmap( m-> bitmap);
            ix-> pixmap = X(h)-> gdrawable;
            X(h)-> gdrawable = XCreatePixmap( DISP, guts. root, 1, 1, 1);
            Object_destroy( h);
            ix-> height += PImage( m-> bitmap)-> h + MENU_ITEM_GAP * 2;
            ix-> width  += PImage( m-> bitmap)-> w + startx;
         }
         if ( m-> accel && ( l = strlen( m-> accel))) 
             ix-> accel_width = XTextWidth( XX-> font-> fs, m-> accel, l);
         if ( ix-> accel_width + ix-> width > x) x = ix-> accel_width + ix-> width;
      }
      y += ix-> height;
      m = m-> next;
      ix++;
   }
   
   if ( vertical) {
      w-> sz.x = x;
      w-> sz.y = y;
      XResizeWindow( DISP, w-> w, x, y);
   }
}


void
prima_handle_menu_event( XEvent *ev, XWindow win, Handle self)
{
   switch ( ev-> type) {
   case Expose:
       {
          DEFMM;
          PMenuWindow w;
          PUnixMenuItem ix;
          PMenuItemReg m;
          GC gc = guts. menugc;
          int mx, my;
          Bool vertical;
          int sz = 1024, l, i, x, y;
          char *s;
          char *t;
          int down_space;
          int right, last = -1;
          XX-> paint_pending = false;
          if ( XX-> wstatic. w == win) {
             w = XX-> w;
             vertical = false;
          } else {
             if ( !( w = get_menu_window( self, win))) return;
             vertical = true;
          }
          right = vertical ? 0 : w-> right;
          m  = w-> m;
          mx = w-> sz.x - 1;
          my = w-> sz.y - 1;
          ix = w-> um;
          XSetFont( DISP, gc, XX-> font-> id);
          XSetForeground( DISP, gc, XX->c[ciBack]);
          XSetBackground( DISP, gc, XX->c[ciBack]);
          if ( vertical) {
             XFillRectangle( DISP, win, gc, 2, 2, mx-1, my-1);
             XSetForeground( DISP, gc, XX->c[ciDark3DColor]);
             XDrawLine( DISP, win, gc, 0, 0, 0, my);
             XDrawLine( DISP, win, gc, 0, 0, mx-1, 0);
             XDrawLine( DISP, win, gc, mx-1, my-1, 2, my-1);
             XDrawLine( DISP, win, gc, mx-1, my-1, mx-1, 1);
             XSetForeground( DISP, gc, guts. monochromeMap[0]);
             XDrawLine( DISP, win, gc, mx, my, 1, my);
             XDrawLine( DISP, win, gc, mx, my, mx, 0);
             XSetForeground( DISP, gc, XX->c[ciLight3DColor]);
             XDrawLine( DISP, win, gc, 1, 1, 1, my-1);
             XDrawLine( DISP, win, gc, 1, 1, mx-2, 1);
          } else
             XFillRectangle( DISP, win, gc, 0, 0, w-> sz.x, w-> sz.y);
          XSetForeground( DISP, gc, XX->c[ciFore]);
          y = vertical ? MENU_ITEM_GAP - 1 : - 1 - MENU_ITEM_GAP;
          x = MENU_XOFFSET;
          s = malloc( sz);
          while ( m) {
             int deltaY = 0;
             if ( last >= w-> last) {
                deltaY = MENU_ITEM_GAP + XX-> font-> font. height;
                XDrawString( DISP, win, gc, x+MENU_XOFFSET+MENU_CHECK_XOFFSET+1, y+deltaY+1, ">>", 2);   
                break;
             }
             if ( m-> divider) {
                if ( vertical) {
                   y += MENU_ITEM_GAP - 1;
                   XSetForeground( DISP, gc, XX->c[ciDark3DColor]);
                   XDrawLine( DISP, win, gc, 1, y, mx-1, y);
                   y++;
                   XSetForeground( DISP, gc, XX->c[ciLight3DColor]);
                   XDrawLine( DISP, win, gc, 1, y, mx-1, y);
                   y += MENU_ITEM_GAP;
                   XSetForeground( DISP, gc, XX->c[ciFore]);
                } else if ( right > 0) {
                   x += right;
                   right = 0;
                }
             } else if ( m-> text) {
                XFontStruct * xs = XX-> font-> fs;
                int lineStart = -1, lineEnd = 0, haveDash = 0;

                deltaY = MENU_ITEM_GAP + XX-> font-> font. height;
                t = m-> text;
                
                for (;;) {
                   l = 0; i = 0;
                   while ( l < sz && t[i]) {
                      if (t[i] == '~' && t[i+1]) {
                         if ( t[i+1] == '~') {
                            s[l++] = '~'; i += 2;
                         } else {
                            if ( !haveDash) {
                               haveDash = 1;
                               lineEnd = lineStart + char_struct( xs, t[i+1])-> width;
                            }
                            i++;
                         }   
                      } else {
                         if ( !haveDash) {
                            XCharStruct * cs = char_struct( xs, t[i]);
                            if ( lineStart < 0)
                               lineStart = ( cs-> lbearing < 0) ? - cs-> lbearing : 0;
                            lineStart += cs-> width;
                         }   
                         s[l++] = t[i++];
                      }
                   }
                   if ( t[i]) {
                      free(s); s = malloc( sz *= 2);
                   } else
                      break;
                }
                if ( m-> disabled) {
                   XSetForeground( DISP, gc, XX->c[ciLight3DColor]); 
                   XDrawString( DISP, win, gc, x+MENU_XOFFSET+MENU_CHECK_XOFFSET+1, y+deltaY+1, s, l);   
                   XSetForeground( DISP, gc, XX->c[ciDisabledText]); 
                }   
                XDrawString( DISP, win, gc, x+MENU_XOFFSET+MENU_CHECK_XOFFSET, deltaY+y, s, l);
                if ( haveDash) {
                   if ( m-> disabled) {
                       XSetForeground( DISP, gc, XX->c[ciLight3DColor]); 
                       XDrawLine( DISP, win, gc, x+MENU_XOFFSET+MENU_CHECK_XOFFSET+lineStart+1, deltaY+y+xs->max_bounds.descent-1+1, 
                          x+MENU_XOFFSET+MENU_CHECK_XOFFSET+lineEnd+1, deltaY+y+xs->max_bounds.descent-1+1);
                       XSetForeground( DISP, gc, XX->c[ciDisabledText]); 
                   }   
                   XDrawLine( DISP, win, gc, x+MENU_XOFFSET+MENU_CHECK_XOFFSET+lineStart, deltaY+y+xs->max_bounds.descent-1, 
                      x+MENU_XOFFSET+MENU_CHECK_XOFFSET+lineEnd, deltaY+y+xs->max_bounds.descent-1);
                }  
                if ( !vertical) x += ix-> width + MENU_XOFFSET + MENU_CHECK_XOFFSET;
             } else if ( m-> bitmap) {
             }

             down_space = char_struct( XX-> font-> fs, ' ')-> width * VMENU_SUBMENU_WCOEFF;
             if ( m-> accel) {
                int zx = vertical ? 
                   mx - MENU_XOFFSET - down_space - ix-> accel_width : 
                   x + MENU_XOFFSET;
                int zy = vertical ? 
                   y + ( deltaY + ix-> height) / 2 : 
                   y + deltaY;
                l = strlen( m-> accel);
                if ( m-> disabled) {
                   XSetForeground( DISP, gc, XX->c[ciLight3DColor]); 
                   XDrawString( DISP, win, gc, zx + 1, zy + 1, m-> accel, l);
                   XSetForeground( DISP, gc, XX->c[ciDisabledText]); 
                }   
                XDrawString( DISP, win, gc, zx, zy, m-> accel, l);
                if ( !vertical)
                   x += ix-> accel_width + MENU_XOFFSET;
             }  

             if ( vertical && m-> down) {
                int ave    = XX-> font-> font. height * 0.4;
                int center = y - deltaY / 2;
                XPoint p[3] = {
                   { mx - MENU_XOFFSET - down_space/2, center},
                   { mx - ave - MENU_XOFFSET - down_space/2, center - ave * 0.6},
                   { mx - ave - MENU_XOFFSET - down_space/2, center + ave * 0.6 + 1}
                };
                if ( m-> disabled) {
                   int i;
                   XSetForeground( DISP, gc, XX->c[ciLight3DColor]); 
                   for ( i = 0; i < 3; i++) { 
                      p[i].x++;
                      p[i].y++;
                   }   
                   XFillPolygon( DISP, win, gc, p, 3, Nonconvex, CoordModeOrigin);   
                   for ( i = 0; i < 3; i++) { 
                      p[i].x--;
                      p[i].y--;
                   }   
                   XSetForeground( DISP, gc, XX->c[ciDisabledText]); 
                }   
                XFillPolygon( DISP, win, gc, p, 3, Nonconvex, CoordModeOrigin);
             }  
             if ( m-> checked) {
                int bottom = y - deltaY * 0.2;
                XGCValues gcv;
                gcv. line_width = 3;
                XChangeGC( DISP, gc, GCLineWidth, &gcv); 
                if ( m-> disabled) {
                   XSetForeground( DISP, gc, XX->c[ciLight3DColor]); 
                   XDrawLine( DISP, win, gc, x + 1 + 1 , y - deltaY / 2 + 1, x + MENU_XOFFSET - 2 + 1, bottom -+1);
                   XDrawLine( DISP, win, gc, x + MENU_XOFFSET - 2 + 1, bottom + 1, x + MENU_CHECK_XOFFSET + 1, y - deltaY * 0.65 + 1);
                   XSetForeground( DISP, gc, XX->c[ciDisabledText]); 
                }    
                XDrawLine( DISP, win, gc, x + 1, y - deltaY / 2, x + MENU_XOFFSET - 2, bottom);
                XDrawLine( DISP, win, gc, x + MENU_XOFFSET - 2, bottom, x + MENU_CHECK_XOFFSET, y - deltaY * 0.65);
                gcv. line_width = 1;
                XChangeGC( DISP, gc, GCLineWidth, &gcv); 
             } 

             if ( vertical) {
                y += MENU_ITEM_GAP;
                deltaY += MENU_ITEM_GAP;
             }
             
             if ( m-> disabled) 
                XSetForeground( DISP, gc, XX->c[ciFore]);  
             m = m-> next;
             ix++;
             last++;
          }
          free(s);
       }
       break;
   case ConfigureNotify:
       {
          DEFMM;
          if ( XX-> wstatic. w == win) {
             PMenuWindow  w = XX-> w;
             PMenuItemReg m;
             PUnixMenuItem ix;
             int x = MENU_XOFFSET * 2;
             int stage = 0;
             w-> sz. x = ev-> xconfigure. width;
             w-> sz. y = ev-> xconfigure. height;
AGAIN:             
             w-> last = -1;
             m = w-> m;
             ix = w-> um;
             while ( m) { 
                int dx = 0;
                if ( !m-> divider) {
                   dx += MENU_XOFFSET + MENU_CHECK_XOFFSET + ix-> width; 
                   if ( m-> accel) 
                      dx += MENU_XOFFSET + ix-> accel_width;
                }
                if ( x + dx >= w-> sz.x) {
                   if ( stage == 0) { // now we are sure that >> should be drawn - check again
                      x = MENU_XOFFSET * 3 + XTextWidth( XX-> font-> fs, ">>", 2);
                      stage++;
                      goto AGAIN;
                   } 
                   break;
                }
                x += dx;
                w-> last++;
                m = m-> next;
                ix++; 
             }
             m = w-> m;
             ix = w-> um;
             w-> right = 0;
             if ( w-> last >= w-> num - 1) {
                Bool hit = false;
                x = MENU_XOFFSET;
                while ( m) {
                   if ( m-> divider) {
                      hit = true;
                      break;
                   } else {
                      x += MENU_XOFFSET + MENU_CHECK_XOFFSET + ix-> width; 
                      if ( m-> accel) 
                         x += MENU_XOFFSET + ix-> accel_width;
                   }
                   m = m-> next; 
                   ix++;
                }
                if ( hit) {
                   w-> right = MENU_XOFFSET;
                   while ( m) {
                      if ( !m-> divider) {
                         w-> right += MENU_XOFFSET + MENU_CHECK_XOFFSET + ix-> width; 
                         if ( m-> accel) 
                            w-> right += MENU_XOFFSET + ix-> accel_width;
                      }
                      m = m-> next;
                      ix++;
                   }
                   w-> right = w-> sz.x - w-> right - x;
                }
             }
          }
       }
       break;
   }
}

void
prima_end_menu(void)
{
}

Bool
apc_menu_create( Handle self, Handle owner)
{
   DEFMM;
   int i;
   apc_menu_destroy( self);
   XX-> type.menu = true;
   XX-> w         = &XX-> wstatic;
   XX-> w-> self  = self; 
   XX-> w-> m     = TREE;
   XX-> w-> first = 0;
   for ( i = 0; i <= ciMaxId; i++)
      XX-> c[i] = prima_allocate_color( 
          nilHandle, 
          prima_map_color( PWindow(owner)-> menuColor[i]), 
          nil);
   apc_menu_set_font( self, &PWindow(owner)-> menuFont);
   return true;
}

Bool
apc_menu_destroy( Handle self)
{
   if ( guts. currentMenu == self) prima_end_menu();
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
   Color c;
   if ( index < 0 || index > ciMaxId) return clInvalid;
   c = M(self)-> c[index];
   if ( guts. palSize > 0) 
       return guts. palette[c]. composite;
   return
      ((((c & guts. visual. blue_mask)  >> guts. blue_shift) << 8) >> guts. blue_range) |
     (((((c & guts. visual. green_mask) >> guts. green_shift) << 8) >> guts. green_range) << 8) |
     (((((c & guts. visual. red_mask)   >> guts. red_shift)   << 8) >> guts. red_range) << 16);
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
   DEFMM;
   if ( !XX-> font)
      return apc_font_default( font);
   memcpy( font, &XX-> font-> font, sizeof( Font));
   return font;
}

Bool
apc_menu_set_color( Handle self, Color color, int i)
{
   DEFMM;
   if ( i < 0 || i > ciMaxId) return false;
   XX-> c[i] = prima_allocate_color( nilHandle, prima_map_color( color), nil);
   if ( XX-> type.menu && X_WINDOW && !XX-> paint_pending) {
      XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
      XX-> paint_pending = true;
   }
   return true;
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
   DEFMM;
   apc_menu_destroy( self);
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
   /*
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
   if ( !XTranslateCoordinates( DISP, owner->udrawable, guts. root,
                                x, y, &x, &y, &dummy)) {
      croak( "apc_popup(): XTranslateCoordinates() failed");
   }
   XMoveWindow( DISP, w->w, x, y);
   XMapWindow( DISP, w->w);
   XRaiseWindow( DISP, w->w);
   XFlush( DISP);
   XCHECKPOINT;
   */
   return false;
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
   DEFXX;
   int y = XX-> menuHeight;
   if ( XX-> menuHeight > 0) {
      PMenu m = ( PMenu) PWindow( self)-> menu;
      PMenuWindow xw, w = M(m)-> w;
      while ( w) {
         xw = w-> next;
         hash_delete( guts. menu_windows, &w-> w, sizeof( w-> w), false);
         XDestroyWindow( DISP, w-> w);
         free_unix_items( w);
         if ( w != &M(m)-> wstatic) free( w);
         w = xw;
      }
      if ( m-> handle == guts. currentMenu) guts. currentMenu = nilHandle;
      M(m)-> w-> w = m-> handle = nilHandle;
      M(m)-> paint_pending = false;
      y = 0;
   }

   if ( menu) {
      PMenu m = ( PMenu) menu;
      XEvent ev;
      XSetWindowAttributes attrs;
      attrs. event_mask =           KeyPressMask | ButtonPressMask  | ButtonReleaseMask
         | EnterWindowMask     | LeaveWindowMask | ButtonMotionMask | ExposureMask
         | StructureNotifyMask | FocusChangeMask | OwnerGrabButtonMask;
      attrs. do_not_propagate_mask = attrs. event_mask;
      attrs. win_gravity = NorthWestGravity;
      y = PWindow(self)-> menuFont. height + MENU_ITEM_GAP * 2;
      M(m)-> w-> w = m-> handle = XCreateWindow( DISP, X_WINDOW, 
         0, 0, 1, 1, 0, CopyFromParent, 
         InputOutput, CopyFromParent, CWWinGravity| CWEventMask, &attrs);
      hash_store( guts. menu_windows, &m-> handle, sizeof( m-> handle), m);
      XResizeWindow( DISP, m-> handle, XX-> size.x, y);
      XMapWindow( DISP, m-> handle);
      M(m)-> paint_pending = true;
      update_menu_window(M(m), M(m)-> w);
      ev. type = ConfigureNotify;
      ev. xconfigure. width  = XX-> size.x;
      ev. xconfigure. height = y;
      prima_handle_menu_event( &ev, ( XWindow) m-> handle, menu);
   }
   prima_window_reset_menu( self, y);
   return true;
}
