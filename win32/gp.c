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

#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include <gbm.h>
#include "Window.h"
#include "Img.h"
#include "Icon.h"
#include "DeviceBitmap.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

void
apc_gp_init( Handle self)
{
   objCheck;
   sys lineWidth = 1;
   sys res = guts. displayResolution;
}

void
apc_gp_done( Handle self)
{
   objCheck;
   if ( sys bm)
      if ( !DeleteObject( sys bm)) apiErr;
   if ( sys pal)
      if ( !DeleteObject( sys pal)) apiErr;
   if ( sys ps)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !EndPaint(( HWND) var handle, sys ps)) apiErr;
      } else if ( is_apt( aptWinPS)) {
         if ( self == application)
             dc_free();
         else {
             if ( !ReleaseDC(( HWND) var handle,  sys ps)) apiErr;
         }
      }
   }
   font_free( sys fontResource, false);
   if ( sys p256) free( sys p256);
   sys bm = sys pal = sys ps = sys bm = sys p256 = nilHandle;
   sys fontResource = nil;
   free( sys charTable);
   free( sys charTable2);
}

static __INLINE__ void
adjust_line_end( int  x1, int  y1, int * x2, int * y2, Bool forth)
{
   if ( forth) {
      if ( x1 == *x2)
         ( y1 < *y2) ? ( *y2)++ : ( *y2)--;
      else if ( y1 == *y2)
         ( x1 < *x2) ? ( *x2)++ : ( *x2)--;
      else
      {
         //     Zinc Application Framework - W_GDIDSP.CPP COPYRIGHT (C) 1990-1998
         long tan = ( *y2 - y1) * 1000L / ( *x2 - x1);
         if ( tan < 1000 && tan > -1000) {
            int dx = *x2 - x1;
            ( dx > 0) ? dx++ : dx--;
            *x2 = x1 + dx;
            *y2 = (int)( y1 + dx * tan / 1000L);
         } else {
            int dy = *y2 - y1;
            ( dy > 0) ? dy++ : dy--;
            *x2 = (int)( x1 + dy * 1000L / tan);
            *y2 = y1 + dy;
         }
         // eo Zinc
      }
   } else {
      if ( x1 == *x2)
         ( y1 < *y2) ? ( *y2)-- : ( *y2)++;
      else if ( y1 == *y2)
         ( x1 < *x2) ? ( *x2)-- : ( *x2)++;
      else
      {
         //     Zinc Application Framework - W_GDIDSP.CPP COPYRIGHT (C) 1990-1998
         long tan = ( *y2 - y1) * 1000L / ( *x2 - x1);
         if ( tan < 1000 && tan > -1000) {
            int dx = *x2 - x1;
            ( dx > 0) ? dx-- : dx++;
            *x2 = x1 + dx;
            *y2 = (int)( y1 + dx * tan / 1000L);
         } else {
            int dy = *y2 - y1;
            ( dy > 0) ? dy-- : dy++;
            *x2 = (int)( x1 + dy * 1000L / tan);
            *y2 = y1 + dy;
         }
         // eo Zinc
      }
   }
}

#define GRAD 57.29577951

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

void
apc_gp_arc( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{ objCheck; {
   HDC     ps = sys ps;
   STYLUS_USE_PEN( ps);
   y = sys lastSize. y - y;
   // SetArcDirection( ps, AD_COUNTERCLOCKWISE);
   if (( angleEnd > angleStart + 360) && (((int)( angleEnd - angleStart) / 360) % 2) == 1)
       Arc( ps, x - radX, y - radY, x + radX, y + radY, x + radX, y, x + radX, y);
   if ( !Arc(
       ps, x - radX, y - radY - 1, x + radX + 1, y + radY,
       x + cos( angleStart / GRAD) * radX, y - sin( angleStart / GRAD) * radY,
       x + cos( angleEnd / GRAD) * radX,   y - sin( angleEnd / GRAD) * radY
   )) apiErr;
}}

void
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{objCheck;{
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hPenHollow);
   STYLUS_USE_BRUSH( ps);
   check_swap( x1, x2);
   check_swap( y1, y2);
   if ( !Rectangle( sys ps, x1, sys lastSize. y - y2 - 1, x2 + 2, sys lastSize. y - y1 + 1)) apiErr;
   SelectObject( ps, old);
}}

void
apc_gp_clear( Handle self)
{objCheck;{
   HDC      ps   = sys ps;
   HGDIOBJ  oldp = SelectObject( ps, hPenHollow);
   LOGBRUSH ers  = { PS_SOLID, apc_gp_get_back_color( self), 0};
   HGDIOBJ  oldh = CreateBrushIndirect( &ers);
   int      oldrop = GetROP2( ps);
   if ( oldrop != R2_COPYPEN) SetROP2( ps, R2_COPYPEN);
   oldh = SelectObject( ps, oldh);
   if ( !Rectangle( sys ps, 0, 0, sys lastSize. x + 1, sys lastSize. y + 1)) apiErr;
   if ( oldrop != R2_COPYPEN) SetROP2( ps, oldrop);
   SelectObject( ps, oldp);
   DeleteObject( SelectObject( ps, oldh));
}}

void
apc_gp_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{objCheck;{
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hBrushHollow);
   STYLUS_USE_PEN( ps);
   y = sys lastSize. y - y;
   if (( angleEnd > angleStart + 360) && (((int)( angleEnd - angleStart) / 360) % 2) == 1) {
      // SetArcDirection( ps, AD_COUNTERCLOCKWISE);
      Arc( ps, x - radX, y - radY - 1, x + radX + 1, y + radY, x + radX + 1, y, x + radX + 1, y);
   }
   if ( !Chord(
       ps, x - radX, y - radY - 1, x + radX + 1, y + radY,
       x + cos( angleStart / GRAD) * radX, y - sin( angleStart / GRAD) * radY,
       x + cos( angleEnd / GRAD) * radX,   y - sin( angleEnd / GRAD) * radY
   )) apiErr;
   SelectObject( ps, old);
}}

void
apc_gp_draw_poly( Handle self, int numPts, Point * points)
{objCheck;{
   int i, dy = sys lastSize. y;
   for ( i = 0; i < numPts; i++)  points[ i]. y = dy - points[ i]. y - 1;
   if ( points[ 0]. x != points[ numPts - 1].x || points[ 0]. y != points[ numPts - 1].y)
      adjust_line_end( points[ numPts - 2].x, points[ numPts - 2].y, &points[ numPts - 1].x, &points[ numPts - 1].y, true);
   STYLUS_USE_PEN( sys ps);
   Polyline( sys ps, ( POINT*) points, numPts);
}}

void
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{objCheck;{
   int i, dy = sys lastSize. y;
   DWORD * pts = malloc( sizeof( DWORD) * numPts);
   for ( i = 0; i < numPts; i++)  {
      points[ i]. y = dy - points[ i]. y - 1;
      pts[ i] = 2;
      if ( i & 1)
         adjust_line_end( points[ i - 1].x, points[ i - 1].y, &points[ i].x, &points[ i].y, true);
   }
   STYLUS_USE_PEN( sys ps);
   PolyPolyline( sys ps, ( POINT*) points, pts, numPts/2);
   free( pts);
}}

void
apc_gp_ellipse( Handle self, int x, int y, int radX, int radY)
{objCheck;{
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hBrushHollow);
   STYLUS_USE_PEN( ps);
   y = sys lastSize. y - y;
   if ( !Ellipse( ps, x - radX, y - radY - 1, x + radX + 1, y + radY)) apiErr;
   SelectObject( ps, old);
}}

void
apc_gp_fill_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{objCheck;{
   HDC     ps = sys ps;
   HGDIOBJ old;
   Bool    comp = stylus_complex( &sys stylus, ps);
   y = sys lastSize. y - y;
   STYLUS_USE_BRUSH( ps);
   if ( comp) {
      old  = SelectObject( ps, hPenHollow);
      if ( !Chord(
          ps, x - radX, y - radY - 1, x + radX + 2, y + radY + 1,
          x + cos( angleStart / GRAD) * radX,   y - sin( angleStart / GRAD) * radY,
          x + cos( angleEnd / GRAD) * radX + 1, y - sin( angleEnd / GRAD) * radY + 1
      )) apiErr;
   } else {
      old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. pen. lopnColor));
      if ( !Chord(
          ps, x - radX, y - radY - 1, x + radX + 1, y + radY,
          x + cos( angleStart / GRAD) * radX,   y - sin( angleStart / GRAD) * radY,
          x + cos( angleEnd / GRAD) * radX + 1, y - sin( angleEnd / GRAD) * radY + 1
      )) apiErr;
   }
   SelectObject( ps, old);
   if ( !comp) DeleteObject( old);
}}


void
apc_gp_fill_ellipse( Handle self, int x, int y, int radX, int radY)
{objCheck;{
   HDC     ps  = sys ps;
   HGDIOBJ old;
   Bool    comp = stylus_complex( &sys stylus, ps);
   STYLUS_USE_BRUSH( ps);
   y = sys lastSize. y - y;
   if ( comp) {
      old  = SelectObject( ps, hPenHollow);
      if ( !Ellipse( ps, x - radX, y - radY - 1, x + radX + 2, y + radY + 1)) apiErr;
   } else {
      old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. pen. lopnColor));
      if ( !Ellipse( ps, x - radX, y - radY - 1, x + radX + 1, y + radY)) apiErr;
   }
   old = SelectObject( ps, old);
   if ( !comp) DeleteObject( old);
}}

static int ctx_R22R4[] = {
  R2_COPYPEN      ,  SRCCOPY          ,
  R2_XORPEN       ,  SRCINVERT        ,
  R2_MASKPEN      ,  SRCAND           ,
  R2_MERGEPEN     ,  SRCPAINT         ,
  R2_NOTCOPYPEN   ,  NOTSRCCOPY       ,
  R2_MASKPENNOT   ,  SRCERASE         ,
  R2_MERGEPENNOT  ,  0x00DD0228       ,
  R2_MASKNOTPEN   ,  0x00220326       ,
  R2_MERGENOTPEN  ,  MERGEPAINT       ,
  R2_NOTXORPEN    ,  0x00990066       ,
  R2_NOTMASKPEN   ,  0x007700E6       ,
  R2_NOTMERGEPEN  ,  NOTSRCERASE      ,
  R2_NOP          ,  0x00AA0029       ,
  R2_BLACK        ,  BLACKNESS        ,
  R2_WHITE        ,  WHITENESS        ,
  R2_NOT          ,  DSTINVERT        ,
  endCtx
};

void
apc_gp_fill_poly( Handle self, int numPts, Point * points)
{objCheck;{
   HDC     ps = sys ps;
   int i,  dy = sys lastSize. y;

   for ( i = 0; i < numPts; i++) points[ i]. y = dy - points[ i]. y - 1;

   if ( !stylus_complex( &sys stylus, ps)) {
      HPEN old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. pen. lopnColor));
      STYLUS_USE_BRUSH( ps);
      Polygon( ps, ( POINT *) points, numPts);
      DeleteObject( SelectObject( ps, old));
   } else {
      int j, dx    = sys lastSize. x;
      int rop      = ctx_remap_def( GetROP2( ps), ctx_R22R4, true, SRCCOPY);
      Point bound  = {0,0};
      Point trans  = {dx,dy};
      HBITMAP bmMask, bmSrc, bmJ;
      HDC dc;
      HGDIOBJ old1, old2;
      Bool db = is_apt( aptDeviceBitmap) || is_apt( aptBitmap);
      for ( i = 0; i < numPts; i++)  {
         if ( points[ i]. x > bound. x) bound. x = points[ i]. x;
         if ( points[ i]. y > bound. y) bound. y = points[ i]. y;
         if ( points[ i] .x < trans. x) trans. x = points[ i]. x;
         if ( points[ i] .y < trans. y) trans. y = points[ i]. y;
      }
      if (( trans. x == dx) || ( trans. y == dy)) return;
      if ( bound. x > dx) bound. x = dx;
      if ( bound. y > dy) bound. y = dy;
      for ( i = 0; i < numPts; i++)  {
         points[ i]. x -= trans. x;
         points[ i]. y -= trans. y;
      }
      bound. x -= trans. x - 1;
      bound. y -= trans. y - 1;

      if ( !( dc  = dc_compat_alloc( ps))) {
         apiErr;
         return;
      }
      if ( db) ps = dc_alloc(); // fact that if dest ps is memory dc, CCB will result mono-bitmap
      if ( !( bmSrc  = CreateCompatibleBitmap( ps, bound. x, bound. y))) {
         apiErr;
         if ( db) dc_free();
         dc_compat_free();
         return;
      }
      if ( db) {
         dc_free();
         ps = sys ps;
      }
      if ( !( bmMask = CreateBitmap( bound. x, bound. y, 1, 1, nil))) {
         apiErr;
         dc_compat_free();
         return;
      }
      bmJ = SelectObject( dc, bmSrc);
      old1 = SelectObject( dc, sys stylusResource-> hbrush);
      old2 = SelectObject( dc, hPenHollow);
      Rectangle( dc, 0, 0, bound. x + 1, bound. y + 1);
      SelectObject( dc, old2);
      SelectObject( dc, old1);
      SelectObject( dc, bmMask);
      SetROP2( dc, R2_WHITE);
      Rectangle( dc, 0, 0, bound. x, bound. y);
      SetROP2( dc, R2_BLACK);
      if ( !Polygon( dc, ( POINT *) points, numPts)) apiErr;
      SelectObject( dc, bmSrc);
      if ( !MaskBlt( ps, trans. x, trans. y, bound. x, bound. y, dc, 0, 0, bmMask, 0, 0,
               MAKEROP4( 0x00AA0029, rop))) apiErr;
      SelectObject( dc, bmJ);
      dc_compat_free();
      DeleteObject( bmMask);
      DeleteObject( bmSrc);
   }
}}

void
apc_gp_fill_sector( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{objCheck;{
   HDC     ps = sys ps;
   HGDIOBJ old;
   int newY    = sys lastSize. y - y;
   POINT   pts[ 3] = {
      { x + cos( angleEnd / GRAD) * radX,   newY - sin( angleEnd / GRAD) * radY},
      { x + cos( angleStart / GRAD) * radX, newY - sin( angleStart / GRAD) * radY},
   };
   Bool    comp = stylus_complex( &sys stylus, ps);
   STYLUS_USE_BRUSH( ps);
   y = newY;
   // SetArcDirection( ps, AD_COUNTERCLOCKWISE);
   if ( comp) {
      old = SelectObject( ps, hPenHollow);
      if ( !Pie(
          ps, x - radX, y - radY - 1, x + radX + 2, y + radY + 1,
          pts[ 1]. x, pts[ 1]. y,
          pts[ 0]. x, pts[ 0]. y
      )) apiErr;
   } else {
      old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. pen. lopnColor));
      if ( !Pie(
          ps, x - radX, y - radY - 1, x + radX + 1, y + radY,
          pts[ 1]. x, pts[ 1]. y,
          pts[ 0]. x, pts[ 0]. y
      )) apiErr;
   }
   SelectObject( ps, old);
   if ( !comp) DeleteObject( old);
}}

void
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{objCheck;{
   HDC ps = sys ps;
   STYLUS_USE_BRUSH( ps);
   ExtFloodFill( ps, x, sys lastSize. y - y - 1, remap_color( borderColor, true),
      singleBorder ? FLOODFILLSURFACE : FLOODFILLBORDER);
}}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{objCheck 0;{
   COLORREF c = GetPixel( sys ps, x, sys lastSize. y - y - 1);
   if ( c == CLR_INVALID) return clInvalid;
   return remap_color(( Color) c, false);
}}

ApiHandle
apc_gp_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}

void
apc_gp_line( Handle self, int x1, int y1, int x2, int y2)
{objCheck;{
   HDC ps = sys ps;
   adjust_line_end( x1, y1, &x2, &y2, true);
   STYLUS_USE_PEN( ps);
   MoveToEx( ps, x1, sys lastSize. y - y1 - 1, nil);
   LineTo(   ps, x2, sys lastSize. y - y2 - 1);
}}

void
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   apc_gp_stretch_image ( self, image, x, y, xFrom, yFrom, xLen, yLen, xLen, yLen, rop);
}

void
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{objCheck;{
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hBrushHollow);
   STYLUS_USE_PEN( ps);
   check_swap( x1, x2);
   check_swap( y1, y2);
   if ( !Rectangle( sys ps, x1, sys lastSize. y - y1, x2 + 1, sys lastSize. y - y2 - 1)) apiErr;
   SelectObject( ps, old);
}}

void
apc_gp_sector( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{objCheck;{
   HDC     ps = sys ps;
   int     newY = sys lastSize. y - y;
   POINT   pts[ 2] = {
      { x + cos( angleEnd / GRAD) * radX,   newY - sin( angleEnd / GRAD) * radY},
      { x + cos( angleStart / GRAD) * radX, newY - sin( angleStart / GRAD) * radY},
   };
   HGDIOBJ old = SelectObject( ps, hBrushHollow);
   STYLUS_USE_PEN( ps);
   y = newY;
   // SetArcDirection( ps, AD_COUNTERCLOCKWISE);
   if (( angleEnd > angleStart + 360) && (((int)( angleEnd - angleStart) / 360) % 2) == 1)
       Arc( ps, x - radX, y - radY - 1, x + radX + 1, y + radY, x + radX + 1, y, x + radX + 1, y);
   if ( !Pie(
       ps, x - radX, y - radY - 1, x + radX + 1, y + radY,
       pts[ 1]. x, pts[ 1]. y,
       pts[ 0]. x, pts[ 0]. y
   )) apiErr;
   SelectObject( ps, old);
}}

void
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
   objCheck;
   SetPixelV( sys ps, x, sys lastSize. y - y - 1, remap_color( color, true));
}


static int ctx_rop2R4[] = {
  ropCopyPut      ,  SRCCOPY          ,
  ropXorPut       ,  SRCINVERT        ,
  ropAndPut       ,  SRCAND           ,
  ropOrPut        ,  SRCPAINT         ,
  ropNotPut       ,  NOTSRCCOPY       ,
  ropNotDestAnd   ,  SRCERASE         ,
  ropNotDestOr    ,  0x00DD0228       ,
  ropNotSrcAnd    ,  0x00220326       ,
  ropNotSrcOr     ,  MERGEPAINT       ,
  ropNotXor       ,  0x00990066       ,
  ropNotAnd       ,  0x007700E6       ,
  ropNotOr        ,  NOTSRCERASE      ,
  ropNoOper       ,  0x00AA0029       ,
  ropBlackness    ,  BLACKNESS        ,
  ropWhiteness    ,  WHITENESS        ,
  ropInvert       ,  DSTINVERT        ,
  endCtx
};


static void dc2screen( HDC dc, Handle self)
{
   HDC xdc = dc_alloc();
   if ( !BitBlt( xdc, 0, 0, var w, var h, dc, 0, 0, SRCCOPY)) apiErr;
   dc_free();
}

void
apc_gp_stretch_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{objCheck;{
   PIcon  i    = ( PIcon) image;
   Handle deja = image;
   int    ly   = sys lastSize. y;
   HDC    xdc  = sys ps;
   HPALETTE p1, p2 = nil, p3;
   HBITMAP  b1;
   HDC dc;
   DWORD theRop;
   Bool db, dcmono;

   COLORREF oFore, oBack;

   dobjCheck(image);
   db = dsys( image) options. aptDeviceBitmap || i-> options. optInDraw;

   // Determinig whether we have bitmap adapted for output or it's just bare bits
   if ( db) {
      if (
          ( guts. displayBMInfo. bmiHeader. biBitCount == 8) &&
          ( !is_apt( aptCompatiblePS)) &&
          ( is_apt( aptWinPS) || is_apt( aptBitmap))
         )
      {
         // There is a big uncertainity about 8-bit BitBlt's, based on fact that
         // memory DCs aren't really compatible with screen DCs, - correct results
         // could be guaranteed either when you Blt source DC = CreateCompatibleDC( dest DC),
         // not when source = CCDC( thirdDC), dest = CCDC( thirdDC) - that's right, I mean it!
         // or just SetDIBits; - latter is slow, stupid, memory-greedy - but surely can be trusted.
         Handle img      = ( Handle) create_object("Prima::Image", "");
         HDC adc         = dsys( image) ps;
         HBITMAP abitmap = dsys( image) bm;
         dsys( img) ps   = dsys( image) ps;
         dsys( img) bm   = dsys( image) bm;
         image_query_bits( img, true);
         dsys( img) ps   = adc;
         dsys( img) bm   = abitmap;
         apc_gp_stretch_image( self, img, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
         Object_destroy( img);
         return;
      }

      dc = dsys( image) ps;
   } else {
      if ( is_apt( aptCompatiblePS))
         dc = CreateCompatibleDC( xdc);
      else
         dc = nil;
      if ( dc) {
         if ( dsys( image) bm == nil) {
            image_destroy_cache( image); // if palette still exists
            deja = image_enscreen( image, self);
            image_set_cache( deja, image);
         }
      } else
         deja = image_enscreen( image, self);
   }

   // actions for mono images
   dcmono = dsys( image) options. aptDeviceBitmap ?
      (( PDeviceBitmap) image)-> monochrome : (
         i-> options. optInDraw ?
           ( dsys( image) bpp == 1) :
           (( i-> type & imBPP) == 1)
      );

   if ( dcmono) {
      PRGBColor pal = i-> palette;
      oFore  = GetTextColor( sys ps);
      oBack  = GetBkColor( sys ps);
      SetTextColor( sys ps,
         i-> palSize > 1
         ? RGB( pal[0].r, pal[0].g, pal[0].b)
         : RGB( 0, 0, 0));
      SetBkColor( sys ps,
         i-> palSize > 2
         ? RGB( pal[1].r, pal[1].g, pal[1].b)
         : RGB( 0xff, 0xff, 0xff));
   }

   // if image is actually icon, drawing and-mask
   if ( kind_of( deja, CIcon)) {
      XBITMAPINFO xbi = {
         { sizeof( BITMAPINFOHEADER), i-> w, i-> h, 1, 1, BI_RGB, 0, 0, 0, 2, 2},
         { {0,0,0,0}, {255,255,255,0}}
      };
      StretchDIBits(
         xdc, x, ly - y - yDestLen, xDestLen, yDestLen, xFrom, yFrom, xLen, yLen,
         i-> mask, ( BITMAPINFO*) &xbi, DIB_RGB_COLORS, SRCAND
      );
      theRop = SRCINVERT;
   } else {
      theRop = ctx_remap_def( rop, ctx_rop2R4, true, SRCCOPY);
   }

   // saving bitmap and palette ( if available) to both dc and xdc.
   p3 = dsys( image) pal;

   if ( p3) {
      if ( db)
         p1 = nil;
      else {
         if ( dc)
            p1 = SelectPalette( dc, p3, 0);
         else
            p1 = nil;
      }
   }

   if ( db)
      b1 = nil;
   else {
      if ( dc)
         b1 = SelectObject( dc, dsys( image) bm);
      else
         b1 = nil;
   }

   if ( p3) {
      p2 = SelectPalette( xdc, p3, 1);
      RealizePalette( xdc);
   }

   //
   if ( dc) {
      if ( !StretchBlt( xdc, x, ly - y - yDestLen, xDestLen, yDestLen, dc,
            xFrom, i-> h - yFrom - yLen, xLen, yLen, theRop)) apiErr;
   } else {
      XBITMAPINFO xbi;
      BITMAPINFO * bi = image_get_binfo( deja, &xbi);
      if ( !StretchDIBits( xdc, x, ly - y - yDestLen, xDestLen, yDestLen,
            xFrom, yFrom,
            xLen, yLen, ((PImage) deja)-> data, bi,
            DIB_RGB_COLORS, theRop)) apiErr;
   }

   // restoring gdiobjects back
   if ( p3) {
      if ( p1) SelectPalette( dc,  p1, 1);
   }
   if ( p2) SelectPalette( xdc, p2, 1);
   if ( b1) SelectObject( dc, b1);

   if ( dcmono) {
      SetTextColor( sys ps, oFore);
      SetBkColor( sys ps, oBack);
   }

   if ( !db) {
      if ( dc) DeleteDC( dc);
      if ( deja != image) Object_destroy( deja);
   }
}}

void
apc_gp_text_out( Handle self, const char * text, int x, int y, int len)
{objCheck;{
   HDC ps = sys ps;
   int bk  = GetBkMode( ps);
   int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
   STYLUS_USE_TEXT( ps);
   if ( opa != bk) SetBkMode( ps, opa);

   if ( !TextOut( ps, x, sys lastSize. y - y, text, len)) apiErr;
   if ( opa != bk) SetBkMode( ps, bk);
}}


// This procedure is about to compensate morbid Win95 behavior when
// calling GetCharABCWidthsFloat() - it returns ERROR_CALL_NOT_IMPLEMENTED. psst.
static BOOL
gp_GetCharABCWidthsFloat( HDC dc, UINT iFirstChar, UINT iLastChar, LPABCFLOAT lpABCF)
{
   UINT i;
   INT fb[ 256];
   TEXTMETRIC tm;


   if ( IS_NT)
      return GetCharABCWidthsFloat( dc, iFirstChar, iLastChar, lpABCF);

   if ( iFirstChar > iLastChar)  // checking bound as far as we can
      return FALSE;

   if ( !GetTextMetrics( dc, &tm)) // determining font
      return FALSE;

   if ( !GetCharWidth( dc, iFirstChar, iLastChar, fb)) // checking full widths
      return FALSE;

   if ( tm. tmPitchAndFamily & TMPF_TRUETYPE) {
      ABC abc[ 256];
      int charExtra;

      if ( !GetCharABCWidths( dc, iFirstChar, iLastChar, abc))
         apiErrRet;

      charExtra = fb[0] - tm. tmOverhang - ( abc[0].abcA + abc[0].abcB + abc[0].abcC);
      if ( charExtra < 0) charExtra = 0;

      for ( i = 0; i <= iLastChar - iFirstChar; i++) {
         lpABCF[i]. abcfA = abc[ i]. abcA;
         lpABCF[i]. abcfB = abc[ i]. abcB + charExtra;
         lpABCF[i]. abcfC = abc[ i]. abcC;
      }
   } else {
      memset( lpABCF, 0, sizeof( ABCFLOAT) * ( iLastChar - iFirstChar + 1));

      for ( i = 0; i <= iLastChar - iFirstChar; i++) {
         lpABCF[i]. abcfA = tm. tmOverhang;
         lpABCF[i]. abcfB = fb[ i] - tm. tmOverhang;
         lpABCF[i]. abcfC = -tm. tmOverhang;
      }
   }
   return TRUE;
}


static char **
gp_text_wrap( Handle self, TextWrapRec * t)
{
    int start = 0, i, lSize = 16;
    float * ct     = sys charTable;
    ABCFLOAT * ct2 = sys charTable2;
    float w = 0, inc;
    char ** ret    = malloc( sizeof( char*) * lSize);
    Bool wasTab    = 0;
    Bool doWidthBreak = t-> width >= 0;
    int tildeIndex = -100, tildeLPos, tildeLine, tildePos;
    unsigned char * text    = ( unsigned char*) t-> text;

// macro for adding string/chunk into result table
#define lAdd(end) {                                       \
   int l = end - start;                                   \
   char * c;                                              \
   if (!( t-> options & twReturnChunks)) {                \
      c = malloc( l + 1);                                 \
      memcpy( c, &text[start], l);                        \
      c[ l] = 0;                                          \
   }                                                      \
   if ( tildeIndex >= 0 && tildeIndex >= start &&         \
        tildeIndex < end)                                 \
   {                                                      \
      tildeLine = t-> t_line = t-> count;                 \
      tildePos = tildeLPos   = tildeIndex - start;        \
      if ( tildeIndex == end - 1) {                       \
         t-> t_line++;                                    \
         tildeLPos = 0;                                   \
      }                                                   \
   }                                                      \
   if ( t-> count == lSize) {                             \
      char ** n = malloc( sizeof( char*) * ( lSize + 16));\
      memcpy( n, ret, sizeof( char*) * lSize);            \
      lSize += 16;                                        \
      free( ret);                                         \
      ret = n;                                            \
   }                                                      \
   if ( t-> options & twReturnChunks) {                   \
      ret[ t-> count++] = (char*) start;                  \
      ret[ t-> count++] = (char*) l;                      \
   } else                                                 \
      ret[ t-> count++] = c;                              \
   start += l;                                            \
}

// determining ~ character location
    if ( t-> options & twCalcMnemonic) {
       for ( i = 0; i < t-> textLen - 1; i++)
          if ( text[ i] == '~') {
             char c = text[ i + 1];
             if ( c == '~' || c < ' ') {
                i++;
                continue;
             } else {
                tildeIndex = i;
                break;
             }
          }
    }
// scanning line accumulating widths and breaking if necessary
    t-> count = 0;
    w = ct2[ text[ 0]]. abcfA;
    for ( i = 0; i < t-> textLen; i++)
    {
       float winc;

       switch ( text[ i])
       {
          case '\t':
             if (!( t-> options & twCalcTabs)) goto _default;
             if ( t-> options & twSpaceBreak)
             {
                lAdd( i); start++;
                w = ct2[ text[ i + 1]]. abcfA;
                continue;
             }
             winc = ct[' '] * t-> tabIndent;
             inc  = ct2[' ']. abcfC;
             wasTab = true;
             break;
          case '\n':
             if (!( t-> options & twNewLineBreak)) goto _default;
             lAdd( i); start++; w = ct2[ text[ i + 1]]. abcfA;
             continue;
          case ' ':
             if (!( t-> options & twSpaceBreak)) goto _default;
             lAdd( i); start++; w = ct2[ text[ i + 1]]. abcfA;
             continue;
          case '~':
             if ( i == tildeIndex) {
                inc = winc = 0;
                break;
             }
          _default: default:
             winc = ct[ text[ i]];
             inc  = ct2[ text[ i]]. abcfC;
       }
       if ( doWidthBreak && w + winc + inc > t-> width)
       {
          if (( i - start == 0) || (( i - 1 == tildeIndex) && ( i - start == 1))) {
            // case when even single char could not fit in given width.
             if ( t-> options & twBreakSingle)
             {
                // do not return anything in this case
                int j;
                if (!( t-> options & twReturnChunks)) {
                   for ( j = 0; j < t-> count; j++) free( ret[ j]);
                   ret[ 0] = malloc(1);
                   ret[ 0][ 0] = 0;
                }
                t-> count = 0;
                return ret;
             } else
                // or fit this character
                lAdd( i + 1);
          } else {
             unsigned char * c;
             lAdd( i);
             if ( t-> options & twWordBreak)
             {
                // checking whether break was at word boundary
                unsigned char rc = text[ i];
                int len;
                if ( t-> options & twReturnChunks)
                {
                   c   = &text[ (int)ret[ t-> count - 2]];
                   len = (int) ret[ t-> count - 1];
                }
                else  {
                  c = ret[ t-> count - 1];
                  len = strlen( c);
                }
                if ( rc != ' ' && rc != '\t' && rc != '\n') {
                   // determining whether this line could be split
                   int j;
                   Bool ok = false;
                   for ( j = len; j >= 0; j--)
                      if ( c[ j] == ' ' || c[ j] == '\n' || c[ j] == '\t') {
                         ok = true;
                         break;
                      }
                   if ( ok)
                   {
                      start -= len - j - 1;
                      if ( t-> options & twReturnChunks)
                         (int) ret[ t-> count - 1] = j;
                      else
                         c[ j] = 0;
                      i -= len - j - 1;
                   }
                }
             }
             i--; // repeat again
          }
          w = 0;
          continue;
       } else
          w += winc;
    }
// adding or skipping last line
    if ( t-> textLen - start > 0 || t-> count == 0) lAdd( t-> textLen);
// expanding tabs
    if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab)
    {
       for ( i = 0; i < t-> count; i++)
       {
          int tabs = 0, len = 0;
          char *substr = ret[ i], *n;
          while (*substr) { if ( *substr == '\t') tabs++; substr++; len++; }
          if ( tabs == 0) continue;
          n = malloc( len + tabs * t-> tabIndent + 1);
          substr = ret[ i];
          len = 0;
          while ( *substr)
          {
             if ( *substr == '\t')
             {
                int j = t-> tabIndent;
                while ( j--) n[ len++] = ' ';
             } else
                n[ len++] = *substr;
             substr++;
          }
          free( ret[ i]);
          n[ len] = 0;
          ret[ i] = n;
       }
    }
// removing ~ and determining it's location
    if ( tildeIndex >= 0 && !(t-> options & twReturnChunks))
    {
        SIZE sz;
        ABCFLOAT ch;
        unsigned char * l = ret[ tildeLine];
        t-> t_char = l[ tildePos+1];
        if ( t-> options & twCollapseTilde)
           memcpy( &l[ tildePos], &l[ tildePos+1], strlen( l) - tildePos);
        l = ret[ t-> t_line];
        if ( !GetTextExtentPoint32( sys ps, l, tildeLPos, &sz)) apiErr;
        if (( sys tmPitchAndFamily & TMPF_TRUETYPE) == 0) sz. cx -= sys tmOverhang;
        if ( !gp_GetCharABCWidthsFloat( sys ps, l[ tildeLPos], l[ tildeLPos], &ch)) apiErr;

        t-> t_start = sz. cx;
        t-> t_end   = sz. cx + ch. abcfB + ch. abcfA + ch. abcfC;
    } else {
        t-> t_start = t-> t_end = t-> t_line = -1;
    }
    return ret;
}

char **
apc_gp_text_wrap( Handle self, TextWrapRec * t)
{objCheck nil;{
   if ( sys charTable == nil) {
      int i;
      float    * f1;
      ABCFLOAT * f2;
      f2 = sys charTable2  = malloc( 256 * sizeof( ABCFLOAT));
      f1 = sys charTable   = malloc( 256 * sizeof( float));
      if ( !gp_GetCharABCWidthsFloat( sys ps, 0, 255, f2)) apiErr;
      for ( i = 0; i < 256; i++) {
         f1[i] = f2[i]. abcfA + f2[i]. abcfB + f2[i]. abcfC;
         f2[i]. abcfC = ( f2[i]. abcfC < 0) ? -f2[i]. abcfC : 0;
         f2[i]. abcfA = ( f2[i]. abcfA < 0) ? -f2[i]. abcfA : 0;
      }
   }
   gp_text_wrap( self, t);
}}

PFontABC
apc_gp_get_font_abc( Handle self)
{objCheck nil;{
   int i;
   ABCFLOAT f2[ 256];
   PFontABC f1 = malloc( 256 * sizeof( FontABC));
   if ( !gp_GetCharABCWidthsFloat( sys ps, 0, 255, f2)) apiErr;
   for ( i = 0; i < 256; i++) {
      f1[i].a = f2[i].abcfA;
      f1[i].b = f2[i].abcfB;
      f1[i].c = f2[i].abcfC;
   }
   return f1;
}}

// gpi settings
Color
apc_gp_get_back_color( Handle self)
{
   objCheck 0;
   return remap_color( sys lbs[1], false);
}

int
apc_gp_get_bpp( Handle self)
{
   objCheck 0;
   return sys bpp;
}

Color
apc_gp_get_color( Handle self)
{
   objCheck 0;
   return remap_color( sys stylus. pen. lopnColor, false);
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   RECT r;
   Rect rr = {0,0,0,0};
   objCheck rr;
   if ( !GetClipBox( sys ps, &r)) apiErr;
   if ( IsRectEmpty( &r)) return rr;
   rr. left   = r. left;
   rr. right  = r. right - 1;
   rr. bottom = sys lastSize. y - r. bottom;
   rr. top    = sys lastSize. y - r. top    - 1;
// rr. left   += sys transform2. x;
// rr. right  += sys transform2. x;
// rr. top    -= sys transform2. y;
// rr. bottom -= sys transform2. y;
   return rr;
}

apc_gp_get_line_end( Handle self)
{
   objCheck 0;
   return sys lineEnd;
}

int
apc_gp_get_line_width( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys lineWidth;
   return sys stylus. pen. lopnWidth. x;
}

static int ctx_lp2PS[] = {
    lpSolid,          PS_SOLID             ,
    lpNull,           PS_NULL              ,
    lpShortDash,      PS_DOT               ,
    lpDash,           PS_DASH              ,
    lpDashDot,        PS_DASHDOT           ,
    lpDashDotDot,     PS_DASHDOTDOT        ,
    endCtx
};

int
apc_gp_get_line_pattern( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys linePattern;
   return ( sys stylus. pen. lopnStyle == PS_USERSTYLE) ?
      sys stylus. extPen. patResource-> style :
      ctx_remap_def( sys stylus. pen. lopnStyle, ctx_lp2PS, false, lpSolid);
}

Color
apc_gp_get_nearest_color( Handle self, Color color)
{
#define quit return remap_color( GetNearestColor( sys ps, clr), false)

   XLOGPALETTE lpLoc, lpGlob;
   int locIdx, globIdx, cdiff;
   long clrGlob, clr = remap_color( color, true);
   objCheck 0;

   if ( !sys pal || ( sys bpp > 8)) quit;
   lpLoc. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lpLoc. palPalEntry);
   if ( lpLoc. palNumEntries == 0)  quit;
   lpGlob. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lpGlob. palPalEntry);
   if ( lpGlob. palNumEntries == 0) quit;

   locIdx = palette_match_color( &lpLoc, clr, &cdiff);
   if ( cdiff >= COLOR_TOLERANCE)   quit;

   clrGlob = ARGB(
      lpLoc. palPalEntry[ locIdx]. peBlue,
      lpLoc. palPalEntry[ locIdx]. peGreen,
      lpLoc. palPalEntry[ locIdx]. peRed
   );
   globIdx = palette_match_color( &lpGlob, clrGlob, &cdiff);
   if ( cdiff >= COLOR_TOLERANCE)   quit;
   return ARGB(
      lpGlob. palPalEntry[ globIdx]. peRed,
      lpGlob. palPalEntry[ globIdx]. peGreen,
      lpGlob. palPalEntry[ globIdx]. peBlue
   );
#undef quit
}

PRGBColor
apc_gp_get_physical_palette( Handle self, int * color)
{
   XLOGPALETTE lpGlob;
   int i, nCol;
   PRGBColor r;

   *color = 0;
   objCheck nil;

   if (( GetDeviceCaps( sys ps, RASTERCAPS) & RC_PALETTE) == 0)
      return nil;

   nCol = GetDeviceCaps( sys ps, NUMCOLORS);
   if ( nCol <= 0 || nCol > 256)
      return nil;


   if ( sys pal && ( nCol > 16)) {
      XLOGPALETTE lp;
      int i, lpCount = 0;
      int map[ 256];

      lp. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lp. palPalEntry);
      lpGlob. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lpGlob. palPalEntry);

      for ( i = 0; i < lp. palNumEntries; i++) {
         long clr = ARGB(
           lp. palPalEntry[ i]. peBlue,
           lp. palPalEntry[ i]. peGreen,
           lp. palPalEntry[ i]. peRed
         );
         int j, cdiff;
         int idx = palette_match_color( &lpGlob, clr, &cdiff);
         Bool hasmatch = 0;

         if ( cdiff >= COLOR_TOLERANCE) continue;

         if ( idx < 10 || idx > 245) continue;

         for ( j = 0; j < lpCount; j++)
            if ( map[ j] == idx) {
               hasmatch = 1;
               break;
            }
         if ( hasmatch) continue;
         map[ lpCount++] = idx;
      }

      for ( i = 0; i < lpCount; i++)
         lp. palPalEntry[ i] = lpGlob. palPalEntry[ map[ i]];
      for ( i = 0; i < lpCount; i++)
         lpGlob. palPalEntry[ i + nCol] = lp. palPalEntry[ i];
      *color = nCol + lpCount;
   } else {
      *color = GetSystemPaletteEntries( sys ps, 0, 256, lpGlob. palPalEntry);
      if (( nCol == 20) && ( *color == 256))
         *color = 20;
   }

   if ( nCol == 20) {
      int i;
      for ( i = 0; i < 10; i++)
         lpGlob. palPalEntry[ i + 10] = lpGlob. palPalEntry[ 255 - i];
   }

   r = malloc( sizeof( RGBColor) * *color);
   for ( i = 0; i < *color; i++) {
      r[i].r = lpGlob. palPalEntry[i]. peRed;
      r[i].g = lpGlob. palPalEntry[i]. peGreen;
      r[i].b = lpGlob. palPalEntry[i]. peBlue;
   }
   return r;
}

Bool
apc_gp_get_region( Handle self, Handle mask)
{
   HRGN rgn;
   int res;
   HBITMAP bm, bmSave;
   HBRUSH  brSave;
   HDC dc;
   XBITMAPINFO xbi;
   BITMAPINFO * bi;
   RECT clipEx;

   objCheck false;
   if ( !is_opt( optInDraw) || !sys ps) return false;

   rgn = CreateRectRgn(0,0,0,0);

   res = GetClipRgn( sys ps, rgn);
   if ( res <= 0) {        // error or just no region
      DeleteObject( rgn);
      return false;
   }
   if ( !mask) {
      DeleteObject( rgn);
      return true;
   }

   GetClipBox( sys ps, &clipEx);
   OffsetRgn( rgn, sys transform2. x, sys transform2. y);
   OffsetRgn( rgn, 0,  clipEx. bottom - clipEx. top - sys lastSize.y);

   CImage( mask)-> create_empty( mask, clipEx. right, sys lastSize.y - clipEx. top, imBW);

   dc = dc_compat_alloc(0);
   if ( !( bm = CreateBitmap( PImage( mask)-> w, PImage( mask)-> h, 1, 1, nil))) {
      dc_compat_free();
      return true;
   }

   bmSave = SelectObject( dc, bm);
   brSave = SelectObject( dc, CreateSolidBrush( RGB(0,0,0)));
   Rectangle( dc, 0, 0, PImage( mask)-> w, PImage( mask)-> h);
   DeleteObject( SelectObject( dc, CreateSolidBrush( RGB( 255, 255, 255))));
   PaintRgn( dc, rgn);
   DeleteObject( SelectObject( dc, brSave));

   bi = image_get_binfo( mask, &xbi);
   if ( !GetDIBits( dc, bm, 0, PImage( mask)-> h, PImage( mask)-> data, bi, DIB_RGB_COLORS)) apiErr;
   SelectObject( dc, bmSave);
   DeleteObject( bm);
   dc_compat_free();

   DeleteObject( rgn);

   return true;
}


Point
apc_gp_get_resolution( Handle self)
{
   Point p = guts. displayResolution;
   if ( !self) return p;
   objCheck p;
   return sys res;
}

static int ctx_rop2R2[] = {
   ropCopyPut       , R2_COPYPEN      ,
   ropXorPut        , R2_XORPEN       ,
   ropAndPut        , R2_MASKPEN      ,
   ropOrPut         , R2_MERGEPEN     ,
   ropNotPut        , R2_NOTCOPYPEN   ,
   ropNotDestAnd    , R2_MASKPENNOT   ,
   ropNotDestOr     , R2_MERGEPENNOT  ,
   ropNotSrcAnd     , R2_MASKNOTPEN   ,
   ropNotSrcOr      , R2_MERGENOTPEN  ,
   ropNotXor        , R2_NOTXORPEN    ,
   ropNotAnd        , R2_NOTMASKPEN   ,
   ropNotOr         , R2_NOTMERGEPEN  ,
   ropNoOper        , R2_NOP          ,
   ropBlackness     , R2_BLACK        ,
   ropWhiteness     , R2_WHITE        ,
   ropInvert        , R2_NOT          ,
   endCtx
};


int
apc_gp_get_rop( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys rop;
   return ctx_remap_def( GetROP2( sys ps), ctx_rop2R2, false, ropCopyPut);
}

int
apc_gp_get_rop2( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys rop2;
   return ( GetBkMode( sys ps) == OPAQUE) ? ropCopyPut : ropNoOper;
}

Bool
apc_gp_get_text_out_baseline( Handle self)
{
   objCheck 0;
   return is_apt( aptTextOutBaseline);
}


int
apc_gp_get_text_width( Handle self, const char* text, int len, Bool addOverhang)
{
   SIZE  sz;
   objCheck 0;
   if ( len == 0) return 0;
   if ( !GetTextExtentPoint32( sys ps, text, len, &sz)) apiErr;

   if ( addOverhang) {
      if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
         ABC abc[2];
         GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
         GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
         if ( abc[0]. abcA < 0) sz. cx -= abc[0]. abcA;
         if ( abc[1]. abcC < 0) sz. cx -= abc[1]. abcC;
      } else if ( IS_NT)
         sz. cx += sys tmOverhang;
   } else if ( !IS_NT)
      sz. cx -= sys tmOverhang;
   return sz. cx;
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len)
{objCheck nil;{
   SIZE  sz;
   Point * pt = malloc( sizeof( Point) * 5);

   if ( !GetTextExtentPoint32( sys ps, text, len, &sz)) apiErr;
   memset( pt, 0, sizeof( Point) * 5);

   pt[0].y = pt[2]. y = sz. cy - - var font. descent;
   pt[1].y = pt[3]. y = - var font. descent;
   pt[4].y = 0;
   pt[3].x = pt[2]. x = pt[4]. x = sz. cx;
   if ( len > 0 && !IS_NT) pt[4].x -= sys tmOverhang;

   if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
      ABC abc[2];
      GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
      GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
      if ( abc[0]. abcA < 0) {
         pt[0].x += abc[0]. abcA;
         pt[1].x += abc[0]. abcA;
      }
      if ( abc[1]. abcC < 0) {
         pt[2].x -= abc[1]. abcC;
         pt[3].x -= abc[1]. abcC;
      }
   }

   if ( var font. direction != 0) {
      int i;
      float s = sin( var font. direction / ( 10 * GRAD));
      float c = cos( var font. direction / ( 10 * GRAD));
      for ( i = 0; i < 5; i++) {
         int x = pt[i]. x;
         int y = pt[i]. y;
         pt[i]. x = x * c - y * s;
         pt[i]. y = x * s + y * c;
      }
   }
   return pt;
}}

Point
apc_gp_get_transform( Handle self)
{
   Point p = {0,0};
   objCheck p;
   if ( !sys ps) return sys transform;
   if ( !GetViewportOrgEx( sys ps, (POINT*)&p)) apiErr;
   p. y = -p. y;
   p. x += sys transform2. x;
   p. y += sys transform2. y;
   return p;
}

Bool
apc_gp_get_text_opaque( Handle self)
{
   objCheck 0;
   return is_apt( aptTextOpaque);
}

#define pal_ok ((sys bpp <= 8) && ( sys pal))

void
apc_gp_set_back_color( Handle self, Color color)
{
   long clr = remap_color( color, true);
   objCheck;
   if ( sys ps) {
      PStylus s = & sys stylus;
      if ( pal_ok) clr = palette_match( self, clr);
      if ( SetBkColor( sys ps, clr) == CLR_INVALID) apiErr;
      s-> brush. backColor = color;
      if ( s-> brush. lb. lbStyle == BS_DIBPATTERNPT)
         stylus_change( self);
   }
   sys lbs[1] = clr;
}

void
apc_gp_set_clip_rect( Handle self, Rect c)
{
   HRGN rgn;
   objCheck;
   if ( !is_opt( optInDraw) || !sys ps) return;
   // inclusive-exclusive
   c. left   -= sys transform2. x;
   c. right  -= sys transform2. x;
   c. top    += sys transform2. y;
   c. bottom += sys transform2. y;
   check_swap( c. top, c. bottom);
   check_swap( c. left, c. right);
   if ( !( rgn = CreateRectRgn( c. left,  sys lastSize. y - c. top,
                                c. right + 1, sys lastSize. y - c. bottom - 1))) {
      apiErr;
      return;
   }
   if ( !SelectClipRgn( sys ps, rgn)) apiErr;
   if ( !DeleteObject( rgn)) apiErr;
}


void    apc_gp_set_color( Handle self, Color color)
{
   long clr = remap_color( color, true);
   objCheck;
   if ( !sys ps)
      sys lbs[0] = clr;
   else {
      PStylus s = & sys stylus;
      if ( pal_ok) clr = palette_match( self, clr);
      s-> pen. lopnColor = ( COLORREF) clr;
      if ( s-> brush. lb. lbStyle != BS_DIBPATTERNPT) s-> brush. lb. lbColor = ( COLORREF) clr;
      stylus_change( self);
   }
}

void
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   objCheck;
{
   HDC ps    = sys ps;
   PStylus s = & sys stylus;
   long *p1 = ( long*) pattern;
   long *p2 = p1 + 1;
   if ( !ps) {
      memcpy( &sys fillPattern2, pattern, sizeof( FillPattern));
      return;
   }
   memcpy( &sys fillPattern, pattern, sizeof( FillPattern));
   if (( *p1 == 0) && ( *p2 == 0)) {
      s-> brush. lb. lbStyle = BS_NULL;
      s-> brush. lb. lbColor = s-> pen. lopnColor;
      s-> brush. lb. lbHatch = 0;
      s-> brush. backColor   = 0;
      memset( s-> brush. pattern, 0, sizeof( s-> brush. pattern));
   } else if (( *p1 == 0xFFFFFFFF) && ( *p2 == 0xFFFFFFFF)) {
      s-> brush. lb. lbStyle = BS_SOLID;
      s-> brush. lb. lbColor = s-> pen. lopnColor;
      s-> brush. lb. lbHatch = 0;
      s-> brush. backColor   = 0;
      memset( s-> brush. pattern, 0, sizeof( s-> brush. pattern));
   } else {
      s-> brush. lb. lbStyle = BS_DIBPATTERNPT;
      s-> brush. lb. lbColor = DIB_RGB_COLORS;
      s-> brush. lb. lbHatch = ( LONG) &bmiHatch;
      s-> brush. backColor   = GetBkColor( ps);
      memcpy( s-> brush. pattern, pattern, sizeof( FillPattern));
   }
   stylus_change( self);
}}

void
apc_gp_set_font( Handle self, PFont font)
{
   TEXTMETRIC tm;
   objCheck;
   if ( !sys ps)
      return;
   font_change( self, font);
   GetTextMetrics( sys ps, &tm);
   sys tmOverhang       = tm. tmOverhang;
   sys tmPitchAndFamily = tm. tmPitchAndFamily;

   free( sys charTable);
   free( sys charTable2);
   (void*)sys charTable = (void*)sys charTable2 = nil;
}

FillPattern * apc_gp_get_fill_pattern( Handle self)
{
   objCheck nil;
   return &sys fillPattern;
}

static int ctx_le2PS_ENDCAP[] = {
    leRound,          PS_ENDCAP_ROUND             ,
    leSquare,         PS_ENDCAP_SQUARE            ,
    leFlat,           PS_ENDCAP_FLAT              ,
    endCtx
};

void
apc_gp_set_line_end( Handle self, int lineEnd)
{
   objCheck;
   if ( !sys ps) sys lineEnd = lineEnd; else {
      PStylus s         = &sys stylus;
      PEXTPEN ep        = &s-> extPen;
      ep-> lineEnd      = ctx_remap_def( lineEnd, ctx_le2PS_ENDCAP, true, PS_ENDCAP_ROUND);
      if ( ep-> actual  = stylus_extpenned( s, 0 & exsLineEnd))
         ep-> style = stylus_get_extpen_style( s);
      stylus_change( self);
   }
}

void
apc_gp_set_line_width( Handle self, int lineWidth)
{
   objCheck;
   if ( !sys ps) sys lineWidth = lineWidth; else {
      PStylus s = &sys stylus;
      if ( lineWidth < 0 || lineWidth > 8192) lineWidth = 0;
      s-> pen. lopnWidth. x = lineWidth;
      stylus_change( self);
   }
}

void
apc_gp_set_line_pattern( Handle self, int pattern)
{
   objCheck;
   if ( !sys ps) sys linePattern = pattern; else {
      PStylus s           = &sys stylus;
      PEXTPEN ep          = &s-> extPen;
      s-> pen. lopnStyle  = ctx_remap_def( pattern, ctx_lp2PS, true, PS_USERSTYLE);
      if ( ep-> actual    = stylus_extpenned( s, 0 & exsLinePattern)) {
         ep-> style       = stylus_get_extpen_style( s);
         ep-> patResource = ( s-> pen. lopnStyle == PS_USERSTYLE) ? patres_fetch( pattern) : &hPatHollow;
      } else
         ep-> patResource = &hPatHollow;
      stylus_change( self);
   }
}

void
apc_gp_set_palette( Handle self)
{
   HPALETTE pal;

   objCheck;
   if ( sys p256) {
      free( sys p256);
      sys p256 = nil;
   }

   if ( !sys ps) return;
   pal = palette_create( self);
   if ( pal)
      SelectPalette( sys ps, pal, 0);
   else
      SelectPalette( sys ps, sys stockPalette, 1);
   RealizePalette( sys ps);
   if ( sys pal) DeleteObject( sys pal);
   sys pal = pal;
}

void
apc_gp_set_region( Handle self, Handle mask)
{
   HRGN rgn = nilHandle;
   objCheck;

   if ( !is_opt( optInDraw) || !sys ps) return;

   rgn = region_create( mask);
   if ( !rgn) {
      SelectClipRgn( sys ps, nil);
      return;
   }
   OffsetRgn( rgn, -sys transform2. x, -sys transform2. y);
   OffsetRgn( rgn, 0, sys lastSize.y - PImage(mask)-> h);
   SelectClipRgn( sys ps, rgn);
   DeleteObject( rgn);
}

void
apc_gp_set_rop( Handle self, int rop)
{
   objCheck;
   if ( !sys ps) { sys rop = rop; return; }
   if ( !SetROP2( sys ps, ctx_remap_def( rop, ctx_rop2R2, true, R2_COPYPEN))) apiErr;
}

void
apc_gp_set_rop2( Handle self, int rop)
{
   objCheck;
   if ( rop != ropCopyPut) rop = ropNoOper;
   if ( !sys ps) { sys rop2 = rop; return; }
   if ( !SetBkMode( sys ps, ( rop == ropCopyPut) ? OPAQUE : TRANSPARENT)) apiErr;
}

void
apc_gp_set_transform( Handle self, int x, int y)
{
   objCheck;
   if ( !sys ps) {
      sys transform. x = x;
      sys transform. y = y;
      return;
   }
   if ( !SetViewportOrgEx( sys ps, x - sys transform2. x, - ( y + sys transform2. y), nil)) apiErr;
}

void
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
   objCheck;
   apt_assign( aptTextOpaque, opaque);
}


void
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
   objCheck;
   apt_assign( aptTextOutBaseline, baseline);
   if ( sys ps) SetTextAlign( sys ps, baseline ? TA_BASELINE : TA_BOTTOM);
}

