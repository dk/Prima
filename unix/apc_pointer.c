/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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

#include "unix/guts.h"
#include "Icon.h"

static int
cursor_map[] = {
   /* crArrow           => */   XC_left_ptr,
   /* crText            => */   XC_xterm,
   /* crWait            => */   XC_watch,
   /* crSize            => */   XC_sizing,
   /* crMove            => */   XC_fleur,
   /* crSizeWest        => */   XC_left_side,
   /* crSizeEast        => */   XC_right_side,
   /* crSizeNE          => */   XC_sb_h_double_arrow,
   /* crSizeNorth       => */   XC_top_side,
   /* crSizeSouth       => */   XC_bottom_side,
   /* crSizeNS          => */   XC_sb_v_double_arrow,
   /* crSizeNW          => */   XC_top_left_corner,
   /* crSizeSE          => */   XC_bottom_right_corner,
   /* crSizeNE          => */   XC_top_right_corner,
   /* crSizeSW          => */   XC_bottom_left_corner,
   /* crInvalid         => */   XC_X_cursor,
};

Cursor
predefined_cursors[] = {
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None,
   None
};

static int
get_cursor( Handle self, Pixmap *source, Pixmap *mask, Point *hot_spot, Cursor *cursor)
{
   int id = X(self)-> pointer_id;

   while ( self && ( id = X(self)-> pointer_id) == crDefault)
      self = PWidget(self)-> owner;
   if ( id == crDefault) {
      id = crArrow;
   } else if ( id == crUser) {
      if (source)       *source   = X(self)-> user_p_source;
      if (mask)         *mask     = X(self)-> user_p_mask;
      if (hot_spot)     *hot_spot = X(self)-> pointer_hot_spot;
      if (cursor)       *cursor   = X(self)-> user_pointer;
   }

   return id;
}

static Bool
load_pointer_font( void)
{
   if ( !guts.pointer_font)
      guts.pointer_font = XLoadQueryFont( DISP, "cursor");
   if ( !guts.pointer_font) {
      warn( "Cannot load cursor font");
      return false;
   }
   return true;
}

Point
apc_pointer_get_hot_spot( Handle self)
{
   Point hot_spot;
   int idx;
   int id = get_cursor(self, nil, nil, &hot_spot, nil);
   XFontStruct *fs;
   XCharStruct *cs;
   Point ret = {0,0};

   if ( id < crDefault || id > crUser)  return ret;
   if ( id == crUser)                   return hot_spot;
   if ( !load_pointer_font())           return ret;

   idx = cursor_map[id];
   fs = guts.pointer_font;
   if ( !fs-> per_char)
      cs = &fs-> min_bounds;
   else if ( idx < fs-> min_char_or_byte2 || idx > fs-> max_char_or_byte2) {
      int default_char = fs-> default_char;
      if ( default_char < fs-> min_char_or_byte2 || default_char > fs-> max_char_or_byte2)
        default_char = fs-> min_char_or_byte2;
      cs = fs-> per_char + default_char - fs-> min_char_or_byte2;
   } else
      cs = fs-> per_char + idx - fs-> min_char_or_byte2;
   ret. x = -cs->lbearing;
   ret. y = guts.cursor_height - cs->ascent;
   if ( ret. x < 0) ret. x = 0;
   if ( ret. y < 0) ret. y = 0;
   if ( ret. x >= guts. cursor_width)  ret. x = guts. cursor_width  - 1;
   if ( ret. y >= guts. cursor_height) ret. y = guts. cursor_height - 1;
   return ret;
}

Point
apc_pointer_get_pos( Handle self)
{
   Point p;
   XWindow root, child;
   int x, y;
   unsigned int mask;

   if ( !XQueryPointer( DISP, guts. root,
			&root, &child, &p. x, &p. y,
			&x, &y, &mask)) 
      return guts. displaySize;
   p. y = guts. displaySize. y - p. y - 1;
   return p;
}

int
apc_pointer_get_shape( Handle self)
{
   return X(self)->pointer_id;
}

Point
apc_pointer_get_size( Handle self)
{
   Point p;
   p.x = guts.cursor_width;
   p.y = guts.cursor_height;
   return p;
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
   XImage *im;
   int id;
   Pixmap p1 = None, p2 = None;
   Bool free_pixmap = true;
   GC gc;
   XGCValues gcv;
   char c;
   int w = guts.cursor_width, h = guts.cursor_height;

   id = get_cursor( self, &p1, &p2, nil, nil);
   if ( id < crDefault || id > crUser)  return false;
   if ( id == crUser) {
      if ( !p1 || !p2) {
         warn( "User pointer inconsistency");
         return false;
      }
      free_pixmap = false;
   } else {
      XFontStruct *fs;
      XCharStruct *cs;
      int idx = cursor_map[id];

      if ( !load_pointer_font()) return false;
      fs = guts.pointer_font;
      if ( !fs-> per_char)
         cs = &fs-> min_bounds;
      else if ( idx < fs-> min_char_or_byte2 || idx > fs-> max_char_or_byte2) {
         int default_char = fs-> default_char;
         if ( default_char < fs-> min_char_or_byte2 || default_char > fs-> max_char_or_byte2)
            default_char = fs-> min_char_or_byte2;
         cs = fs-> per_char + default_char - fs-> min_char_or_byte2;
      } else
         cs = fs-> per_char + idx - fs-> min_char_or_byte2;
      
      p1 = XCreatePixmap( DISP, guts. root, w, h, 1);
      p2 = XCreatePixmap( DISP, guts. root, w, h, 1);
      gcv. background = 1;
      gcv. foreground = 0;
      gcv. font = guts.pointer_font-> fid;
      gc = XCreateGC( DISP, p1, GCBackground | GCForeground | GCFont, &gcv);
      XFillRectangle( DISP, p1, gc, 0, 0, w, h);
      gcv. background = 0;
      gcv. foreground = 1;
      XChangeGC( DISP, gc, GCBackground | GCForeground, &gcv);
      XFillRectangle( DISP, p2, gc, 0, 0, w, h);
      XDrawString( DISP, p1, gc, -cs-> lbearing, cs-> ascent, (c = (char)(idx+1), &c), 1);
      gcv. background = 1;
      gcv. foreground = 0;
      XChangeGC( DISP, gc, GCBackground | GCForeground, &gcv);
      XDrawString( DISP, p2, gc, -cs-> lbearing, cs-> ascent, (c = (char)(idx+1), &c), 1);
      XDrawString( DISP, p1, gc, -cs-> lbearing, cs-> ascent, (c = (char)idx, &c), 1);
      XFreeGC( DISP, gc);
   }
   CIcon(icon)-> create_empty( icon, w, h, imBW);
   im = XGetImage( DISP, p1, 0, 0, w, h, 1, XYPixmap);
   prima_copy_xybitmap( PIcon(icon)-> data, (Byte*)im-> data,
                        PIcon(icon)-> w, PIcon(icon)-> h,
                        PIcon(icon)-> lineSize, im-> bytes_per_line);
   XDestroyImage( im);
   im = XGetImage( DISP, p2, 0, 0, w, h, 1, XYPixmap);
   prima_copy_xybitmap( PIcon(icon)-> mask, (Byte*)im-> data,
                        PIcon(icon)-> w, PIcon(icon)-> h,
                        PIcon(icon)-> maskLine, im-> bytes_per_line);
  if ( id == crUser) {
     int i;
     Byte * mask = PIcon(icon)-> mask;
     for ( i = 0; i < PIcon(icon)-> maskSize; i++) 
        mask[i] = ~mask[i];
   }   
   XDestroyImage( im);
   if ( free_pixmap) {
      XFreePixmap( DISP, p1);
      XFreePixmap( DISP, p2);
   }
   return true;
}

Bool
apc_pointer_get_visible( Handle self)
{
   return guts. pointer_invisible_count == 0;
}

Bool
apc_pointer_set_pos( Handle self, int x, int y)
{
   XEvent ev;
   if ( !XWarpPointer( DISP, None, guts. root, 
      0, 0, guts. displaySize.x, guts. displaySize.y, x, guts. displaySize.y - y - 1))
      return false;
   XCHECKPOINT;
   XSync( DISP, false);
   while ( XCheckMaskEvent( DISP, PointerMotionMask|EnterWindowMask|LeaveWindowMask, &ev))
      prima_handle_event( &ev, nil);
   return true;   
}

Bool
apc_pointer_set_shape( Handle self, int id)
{
   DEFXX;
   Cursor uc = None;

   if ( id < crDefault || id > crUser)  return false;
   XX-> pointer_id = id;
   id = get_cursor( self, nil, nil, nil, &uc);
   if ( id == crUser) {
      if ( uc != None || ( uc = XX-> user_pointer) != None) {
         if ( self != application) {
            if ( guts. pointer_invisible_count < 0) {
               if ( !XX-> flags. pointer_obscured) {
                  XDefineCursor( DISP, XX-> udrawable, prima_null_pointer());   
                  XX-> flags. pointer_obscured = 1;
               }   
            } else {   
               XDefineCursor( DISP, XX-> udrawable, uc);
               XX-> flags. pointer_obscured = 0;
            }
            XCHECKPOINT;
         }
      } else
         id = crArrow;
   }
   if ( id != crUser) {
      if ( predefined_cursors[id] == None) {
         predefined_cursors[id] =
            XCreateFontCursor( DISP, cursor_map[id]);
         XCHECKPOINT;
      }
      XX-> actual_pointer = predefined_cursors[id];
      if ( self != application) {
         if ( guts. pointer_invisible_count < 0) {
            if ( !XX-> flags. pointer_obscured) {
               XDefineCursor( DISP, XX-> udrawable, prima_null_pointer());   
               XX-> flags. pointer_obscured = 1;
            }   
         } else {   
            XDefineCursor( DISP, XX-> udrawable, predefined_cursors[id]);
            XX-> flags. pointer_obscured = 0;
         }
         XCHECKPOINT;
      }
   }
   XFlush( DISP);
   if ( guts. grab_widget)
       apc_widget_set_capture( guts. grab_widget, true, guts. grab_confine);
   return true;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hot_spot)
{
   DEFXX;
   Handle cursor;
   PIcon c;

   if ( XX-> user_pointer != None) {
      XFreeCursor( DISP, XX-> user_pointer);
      XX-> user_pointer = None;
   }
   if ( XX-> user_p_source != None) {
      XFreePixmap( DISP, XX-> user_p_source);
      XX-> user_p_source = None;
   }
   if ( XX-> user_p_mask != None) {
      XFreePixmap( DISP, XX-> user_p_mask);
      XX-> user_p_mask = None;
   }
   if ( icon != nilHandle) {
      Bool noSZ  = PIcon(icon)-> w != guts.cursor_width || PIcon(icon)-> h != guts.cursor_height;
      Bool noBPP = (PIcon(icon)-> type & imBPP) != 1;
      XColor xcb, xcw;
      if ( noSZ || noBPP) {
         cursor = CIcon(icon)->dup(icon);
         c = PIcon(cursor);
         if ( cursor == nilHandle) {
            warn( "Error duping user cursor");
            return false;
         }
         if ( noSZ) {
            CIcon(cursor)-> stretch( cursor, guts.cursor_width, guts.cursor_height);
            if ( c-> w != guts.cursor_width || c-> h != guts.cursor_height) {
               warn( "Error stretching user cursor");
               Object_destroy( cursor);
               return false;
            }
         }   
         if ( noBPP) {
            CIcon(cursor)-> set_type( cursor, imMono);
            if ((c-> type & imBPP) != 1) {
               warn( "Error black-n-whiting user cursor");
               Object_destroy( cursor);
               return false;
            }
         }
      } else
         cursor = icon;
      if ( !prima_create_icon_pixmaps( cursor, &XX-> user_p_source, &XX-> user_p_mask)) {
         warn( "Error creating user cursor pixmaps");
         if ( noSZ || noBPP)
            Object_destroy( cursor);
         return false;
      }
      if ( noSZ || noBPP)
         Object_destroy( cursor);
      if ( hot_spot. x < 0) hot_spot. x = 0;
      if ( hot_spot. y < 0) hot_spot. y = 0;
      if ( hot_spot. x >= guts. cursor_width)  hot_spot. x = guts. cursor_width  - 1;
      if ( hot_spot. y >= guts. cursor_height) hot_spot. y = guts. cursor_height - 1;
      XX-> pointer_hot_spot = hot_spot;
      xcb. red = xcb. green = xcb. blue = 0; 
      xcw. red = xcw. green = xcw. blue = 0xFFFF; 
      xcb. pixel = guts. monochromeMap[0];
      xcw. pixel = guts. monochromeMap[1];
      xcb. flags = xcw. flags = DoRed | DoGreen | DoBlue;
      XX-> user_pointer = XCreatePixmapCursor( DISP, XX-> user_p_source,
          XX-> user_p_mask, &xcw, &xcb, 
          hot_spot. x, guts.cursor_height - hot_spot. y);
      if ( XX-> user_pointer == None) {
         warn( "error creating cursor from pixmaps");
         return false;
      }
      if ( XX-> pointer_id == crUser && self != application) {
         if ( guts. pointer_invisible_count < 0) {
            if ( !XX-> flags. pointer_obscured) {
               XDefineCursor( DISP, XX-> udrawable, prima_null_pointer());
               XX-> flags. pointer_obscured = 1;
            }   
         } else {   
            XDefineCursor( DISP, XX-> udrawable, XX-> user_pointer);
            XX-> flags. pointer_obscured = 0;
         }
         XCHECKPOINT;
      }      
   }
   XFlush( DISP);
   if ( guts. grab_widget)
       apc_widget_set_capture( guts. grab_widget, true, guts. grab_confine);
   return true;
}



Bool
apc_pointer_set_visible( Handle self, Bool visible)
{
   /* maintaining hide/show count */
   if ( visible) {
      if ( guts. pointer_invisible_count == 0) 
         return true;
      if ( ++guts. pointer_invisible_count < 0)
         return true;
   } else {
      if ( guts. pointer_invisible_count-- < 0)
         return true;
   }
 
   /* setting pointer for widget under cursor */
   {
      Point p    = apc_pointer_get_pos( application);
      Handle wij = apc_application_get_widget_from_point( application, p);
      if ( wij) {
         X(wij)-> flags. pointer_obscured = (visible ? 0 : 1);
         XDefineCursor( DISP, X(wij)-> udrawable, 
            visible ? (( X(wij)-> pointer_id == crUser) ? 
                         X(wij)-> user_pointer : X(wij)-> actual_pointer) 
                    : prima_null_pointer());  
      }   
   }   
   XFlush( DISP);
   if ( guts. grab_widget)
       apc_widget_set_capture( guts. grab_widget, true, guts. grab_confine);
   return true;
}

Cursor
prima_null_pointer( void)
{
   if ( guts. null_pointer == nilHandle) {
      Handle nullc = ( Handle) create_object( "Prima::Icon", "", nil);
      PIcon  n = ( PIcon) nullc;
      Pixmap xor, and;
      XColor xc;      
      if ( nullc == nilHandle) {
         warn("Error creating icon object");
         return nilHandle;
      }   
      n-> self-> create_empty( nullc, 16, 16, imBW);
      memset( n-> mask, 0xFF, n-> maskSize);
      if ( !prima_create_icon_pixmaps( nullc, &xor, &and)) {
         warn( "Error creating null cursor pixmaps"); 
         Object_destroy( nullc);
         return nilHandle;
      }  
      Object_destroy( nullc);
      xc. red = xc. green = xc. blue = 0;
      xc. pixel = guts. monochromeMap[0];
      xc. flags = DoRed | DoGreen | DoBlue;
      guts. null_pointer = XCreatePixmapCursor( DISP, xor, and, &xc, &xc, 0, 0);                                      
      XCHECKPOINT;
      XFreePixmap( DISP, xor);
      XFreePixmap( DISP, and);
      if ( !guts. null_pointer) {
         warn( "Error creating null cursor from pixmaps");
         return nilHandle;
      }   
   }
   return guts. null_pointer;
}
