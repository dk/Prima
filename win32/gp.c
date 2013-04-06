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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include "Window.h"
#include "Icon.h"
#include "DeviceBitmap.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

Bool
apc_gp_init( Handle self)
{
   objCheck false;
   sys lineWidth = 1;
   sys res = guts. displayResolution;
   return true;
}

Bool
apc_gp_done( Handle self)
{
   objCheck false;
   if ( sys bm)
      if ( !DeleteObject( sys bm)) apiErr;
   if ( sys pal)
      if ( !DeleteObject( sys pal)) apiErr;
   if ( sys ps)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
      } else if ( is_apt( aptWinPS)) {
         if ( self == application)
             dc_free();
         else {
             if ( !ReleaseDC(( HWND) var handle,  sys ps)) apiErr;
         }
      }
   }
   if ( sys linePatternLen  > 3) free( sys linePattern);

   if ( sys linePatternLen2 > 3) free( sys linePattern2);
   font_free( sys fontResource, false);
   if ( sys p256) free( sys p256);
   sys bm = nil;
   sys pal = nil;
   sys ps = nil;
   sys bm = nil;
   sys p256 = nil;
   sys fontResource = nil;
   sys linePattern = nil;
   return true;
}

void
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

#define ELLIPSE_RECT (int)(x - ( dX - 1) / 2), (int)(y - dY / 2), (int)(x + dX / 2 + 1), (int)(y + (dY - 1) / 2 + 1)
#define ELLIPSE_RECT_SUPERINCLUSIVE (int)(x - ( dX - 1) / 2), (int)(y - dY / 2), (int)(x + dX / 2 + 2), (int)(y + (dY - 1) / 2 + 2)
#define ARC_COMPLETE (int)(x + dX / 2 + 1), y, (int)(x + dX / 2 + 1), y
#define ARC_ANGLED   (int)(x + cos( angleStart / GRAD) * dX / 2 + 0.5), (int)(y - sin( angleStart / GRAD) * dY / 2 + 0.5), \
                     (int)(x + cos( angleEnd / GRAD) * dX / 2 + 0.5),   (int)(y - sin( angleEnd / GRAD) * dY / 2 + 0.5)
#define ARC_ANGLED_SUPERINCLUSIVE   (int)(x + cos( angleStart / GRAD) * dX / 2 + 0.5), (int)(y - sin( angleStart / GRAD) * dY / 2 + 0.5), \
                     (int)(x + cos( angleEnd / GRAD) * dX / 2 + 1.5),   (int)(y - sin( angleEnd / GRAD) * dY / 2 + 1.5)


Bool
apc_gp_arc( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{ objCheck false; {
   int compl, needf, drawState = 0;
   Bool erratic = erratic_line( self);
   HDC     ps = sys ps;
   STYLUS_USE_PEN( ps);
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   y = sys lastSize. y - y - 1;
   while( compl--) {
      if ( erratic)
         drawState = gp_arc( self, x, y, dX, dY, 0, 360, drawState);
      else
         Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
   }
   if ( !needf) return true;
   if ( erratic)
      gp_arc( self, x, y, dX, dY, angleStart, angleEnd, drawState);
   else
      if ( !Arc( ps, ELLIPSE_RECT, ARC_ANGLED)) apiErrRet;
   return true;
}}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hPenHollow);
   Bool ok = true;
   STYLUS_USE_BRUSH( ps);
   check_swap( x1, x2);
   check_swap( y1, y2);
   if ( !( ok = Rectangle( ps, x1, sys lastSize. y - y2 - 1, x2 + 2, sys lastSize. y - y1 + 1))) apiErr;
   SelectObject( ps, old);
   return ok;
}}

Bool
apc_gp_clear( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
   Bool     ok = true;
   HDC      ps   = sys ps;
   HGDIOBJ  oldp = SelectObject( ps, hPenHollow);
   LOGBRUSH ers;
   HGDIOBJ  oldh;
   ers. lbStyle = PS_SOLID;
   ers. lbColor = sys lbs[ 1];
   ers. lbHatch = 0;
   oldh = CreateBrushIndirect( &ers);
   oldh = SelectObject( ps, oldh);
   if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
      x1 = y1 = 0;
      x2 = sys lastSize. x - 1;
      y2 = sys lastSize. y - 1;
   }
   check_swap( x1, x2);
   check_swap( y1, y2);
   if ( !( ok = Rectangle( sys ps, x1, sys lastSize. y - y2 - 1, x2 + 2, sys lastSize. y - y1 + 1))) apiErr;
   SelectObject( ps, oldp);
   DeleteObject( SelectObject( ps, oldh));
   return ok;
}}

Bool
apc_gp_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
   Bool ok = true;
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hBrushHollow);
   int compl, needf, drawState = 0;
   Bool erratic = erratic_line( self);
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   STYLUS_USE_PEN( ps);
   y = sys lastSize. y - y - 1;
   while( compl--) {
      if ( erratic)
         drawState = gp_arc( self, x, y,  dX, dY, 0, 360, drawState);
      else
         Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
   }
   if ( needf) {
      if ( erratic) {
         drawState = gp_arc( self, x, y,  dX, dY, angleStart, angleEnd, drawState);
         gp_line( self, ARC_ANGLED, drawState);
      } else
         if ( !( ok = Chord( ps, ELLIPSE_RECT, ARC_ANGLED))) apiErr;
   }
   SelectObject( ps, old);
   return ok;
}}

Bool
apc_gp_draw_poly( Handle self, int numPts, Point * points)
{objCheck false;{
   int i, dy = sys lastSize. y;
   for ( i = 0; i < numPts; i++)  points[ i]. y = dy - points[ i]. y - 1;
   if ( points[ 0]. x != points[ numPts - 1].x || points[ 0]. y != points[ numPts - 1].y)
      adjust_line_end( points[ numPts - 2].x, points[ numPts - 2].y, &points[ numPts - 1].x, &points[ numPts - 1].y, true);
   STYLUS_USE_PEN( sys ps);
   if ( erratic_line( self)) {
      int draw = 0;
      for ( i = 0; i < numPts - 1; i++)
         draw = gp_line( self, points[i].x, points[i].y, points[i+1].x, points[i+1].y, draw);
   } else {
      if ( !Polyline( sys ps, ( POINT*) points, numPts)) apiErrRet;
   }
   return true;
}}

Bool
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{objCheck false;{
   Bool ok = true;
   int i, dy = sys lastSize. y;
   DWORD * pts = ( DWORD *) malloc( sizeof( DWORD) * numPts);
   if ( !pts) return false;

   for ( i = 0; i < numPts; i++)  {
      points[ i]. y = dy - points[ i]. y - 1;
      pts[ i] = 2;
      if ( i & 1)
         adjust_line_end( points[ i - 1].x, points[ i - 1].y, &points[ i].x, &points[ i].y, true);
   }
   STYLUS_USE_PEN( sys ps);
   if ( erratic_line( self)) {
      for ( i = 0; i < numPts; i++)  {
         if ( i & 1)
            gp_line( self, points[ i - 1].x, points[ i - 1].y, points[ i].x, points[ i].y, 0);
      }
   } else
      if ( !( ok = PolyPolyline( sys ps, ( POINT*) points, pts, numPts/2))) apiErr;
   free( pts);
   return ok;
}}


Bool
apc_gp_ellipse( Handle self, int x, int y, int dX, int dY)
{objCheck false;{
   Bool    ok = true;
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hBrushHollow);

   STYLUS_USE_PEN( ps);
   y = sys lastSize. y - y - 1;
   if ( erratic_line( self))
      gp_arc( self, x, y, dX, dY, 0, 360, 0);
   else {
      // if ( !( ok = Ellipse( ps, x - dX - 0.5, y - dY + 0.5, x + dX + 1.0, y + dY + 1.5))) apiErr;
      if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
   }
   SelectObject( ps, old);
   return ok;
}}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
   Bool ok = true;
   HDC     ps = sys ps;
   HGDIOBJ old;
   Bool   comp;
   int compl, needf;

   compl = arc_completion( &angleStart, &angleEnd, &needf);
   comp = stylus_complex( &sys stylus, ps);
   y = sys lastSize. y - y - 1;
   STYLUS_USE_BRUSH( ps);
   if ( comp) {
      old  = SelectObject( ps, hPenHollow);
      while ( compl--)
         if ( !( ok = Ellipse( ps, ELLIPSE_RECT_SUPERINCLUSIVE))) apiErr;
      if ( !( ok = !needf || Chord(
          ps, ELLIPSE_RECT_SUPERINCLUSIVE, ARC_ANGLED_SUPERINCLUSIVE
      ))) apiErr;
   } else {
      old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
      while ( compl--)
         if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
      if ( !( ok = !needf || Chord(
          ps, ELLIPSE_RECT, ARC_ANGLED_SUPERINCLUSIVE
      ))) apiErr;
   }
   old = SelectObject( ps, old);
   if ( !comp) DeleteObject( old);
   return ok;
}}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y, int dX, int dY)
{objCheck false;{
   Bool ok = true;
   HDC     ps  = sys ps;
   HGDIOBJ old;
   Bool    comp = stylus_complex( &sys stylus, ps);
   STYLUS_USE_BRUSH( ps);
   y = sys lastSize. y - y - 1;
   if ( comp) {
      old  = SelectObject( ps, hPenHollow);
      if ( !( ok = Ellipse( ps, ELLIPSE_RECT_SUPERINCLUSIVE))) apiErr;
   } else {
      old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
      if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
   }
   old = SelectObject( ps, old);
   if ( !comp) DeleteObject( old);
   return ok;
}}

static Handle ctx_R22R4[] = {
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

Bool
apc_gp_fill_poly( Handle self, int numPts, Point * points)
{Bool ok = true; objCheck false;{
   HDC     ps = sys ps;
   int i,  dy = sys lastSize. y;

   for ( i = 0; i < numPts; i++) points[ i]. y = dy - points[ i]. y - 1;

   if ( !stylus_complex( &sys stylus, ps)) {
      HPEN old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
      STYLUS_USE_BRUSH( ps);
      if ( !( ok = Polygon( ps, ( POINT *) points, numPts))) apiErr;
      DeleteObject( SelectObject( ps, old));
   } else {
      int dx    = sys lastSize. x;
      int rop      = ctx_remap_def( GetROP2( ps), ctx_R22R4, true, SRCCOPY);
      Point bound  = {0,0};
      Point trans;
      HBITMAP bmMask, bmSrc, bmJ;
      HDC dc;
      HGDIOBJ old1, oldelta;
      Bool db = is_apt( aptDeviceBitmap) || is_apt( aptBitmap);
      trans. x = dx;
      trans. y = dy;
      for ( i = 0; i < numPts; i++)  {
         if ( points[ i]. x > bound. x) bound. x = points[ i]. x;
         if ( points[ i]. y > bound. y) bound. y = points[ i]. y;
         if ( points[ i] .x < trans. x) trans. x = points[ i]. x;
         if ( points[ i] .y < trans. y) trans. y = points[ i]. y;
      }
      if (( trans. x == dx) || ( trans. y == dy)) return false;
      if ( bound. x > dx) bound. x = dx;
      if ( bound. y > dy) bound. y = dy;
      for ( i = 0; i < numPts; i++)  {
         points[ i]. x -= trans. x;
         points[ i]. y -= trans. y;
      }
      bound. x -= trans. x - 1;
      bound. y -= trans. y - 1;

      if ( !( dc  = dc_compat_alloc( ps))) apiErrRet;
      if ( db) {
         if ( !( ps = dc_alloc())) { // fact that if dest ps is memory dc, CCB will result mono-bitmap
            dc_compat_free();
            return false;
         }
      }
      if ( !( bmSrc  = CreateCompatibleBitmap( ps, bound. x, bound. y))) {
         apiErr;
         if ( db) dc_free();
         dc_compat_free();
         return false;
      }
      if ( db) {
         dc_free();
         ps = sys ps;
      }
      if ( !( bmMask = CreateBitmap( bound. x, bound. y, 1, 1, nil))) {
         apiErr;
         dc_compat_free();
         return false;
      }
      bmJ = SelectObject( dc, bmSrc);
      old1 = SelectObject( dc, sys stylusResource-> hbrush);
      oldelta = SelectObject( dc, hPenHollow);
      Rectangle( dc, 0, 0, bound. x + 1, bound. y + 1);
      SelectObject( dc, oldelta);
      SelectObject( dc, old1);
      SelectObject( dc, bmMask);
      SetROP2( dc, R2_WHITE);
      Rectangle( dc, 0, 0, bound. x, bound. y);
      SetROP2( dc, R2_BLACK);
      if ( !( ok = Polygon( dc, ( POINT *) points, numPts))) apiErr;
      SelectObject( dc, bmSrc);
      if ( !( ok &= MaskBlt( ps, trans. x, trans. y, bound. x, bound. y, dc, 0, 0, bmMask, 0, 0,
               MAKEROP4( 0x00AA0029, rop)))) apiErr;
      SelectObject( dc, bmJ);
      dc_compat_free();
      DeleteObject( bmMask);
      DeleteObject( bmSrc);
   }
}return ok;}

Bool
apc_gp_fill_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
   Bool ok = true;
   HDC     ps = sys ps;
   HGDIOBJ old;
   int newY  = sys lastSize. y - y - 1;
   POINT   pts[ 3];
   Bool comp;
   int compl, needf;

   compl = arc_completion( &angleStart, &angleEnd, &needf);
   comp = stylus_complex( &sys stylus, ps);

   pts[ 0]. x = x + cos( angleEnd / GRAD) * dX / 2 + 0.5;
   pts[ 0]. y = newY - sin( angleEnd / GRAD) * dY / 2 + 0.5;
   pts[ 1]. x = x + cos( angleStart / GRAD) * dX / 2 + 0.5;
   pts[ 1]. y = newY - sin( angleStart / GRAD) * dY / 2 + 0.5;

   STYLUS_USE_BRUSH( ps);
   y = newY;
   if ( comp) {
      old = SelectObject( ps, hPenHollow);
      while ( compl--)
         if ( !( ok = Ellipse( ps, ELLIPSE_RECT_SUPERINCLUSIVE))) apiErr;
      if ( !( ok = !needf || Pie(
          ps, ELLIPSE_RECT_SUPERINCLUSIVE,
          pts[ 1]. x, pts[ 1]. y,
          pts[ 0]. x, pts[ 0]. y
      ))) apiErr;
   } else {
      old = SelectObject( ps, CreatePen( PS_SOLID, 1, sys stylus. brush. lb. lbColor));
      while ( compl--)
         if ( !( ok = Ellipse( ps, ELLIPSE_RECT))) apiErr;
      if ( !( ok = !needf || Pie(
          ps, ELLIPSE_RECT,
          pts[ 1]. x, pts[ 1]. y,
          pts[ 0]. x, pts[ 0]. y
      ))) apiErr;
   }
   old = SelectObject( ps, old);
   if ( !comp) DeleteObject( old);
   return ok;
}}

Bool
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{objCheck false;{
   HDC ps = sys ps;
   STYLUS_USE_BRUSH( ps);
   if ( !ExtFloodFill( ps, x, sys lastSize. y - y - 1, remap_color( borderColor, true),
      singleBorder ? FLOODFILLSURFACE : FLOODFILLBORDER)) apiErrRet;
   return true;
}}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{objCheck clInvalid;{
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


Bool
apc_gp_line( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
   HDC ps = sys ps;

   STYLUS_USE_PEN( ps);

   adjust_line_end( x1, y1, &x2, &y2, true);
   y1 = sys lastSize. y - y1 - 1;
   y2 = sys lastSize. y - y2 - 1;
   if ( erratic_line( self))
      gp_line( self, x1, y1, x2, y2, 0);
   else {
      MoveToEx( ps, x1, y1, nil);
      if ( !LineTo( ps, x2, y2)) apiErrRet;
   }
   return true;
}}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   return apc_gp_stretch_image ( self, image, x, y, xFrom, yFrom, xLen, yLen, xLen, yLen, rop);
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{objCheck false;{
   Bool ok = true;
   HDC     ps = sys ps;
   HGDIOBJ old = SelectObject( ps, hBrushHollow);
   STYLUS_USE_PEN( ps);
   check_swap( x1, x2);
   if ( erratic_line( self)) {
      int draw;
      y1 = sys lastSize. y - y1 - 1;
      y2 = sys lastSize. y - y2 - 1;
      check_swap( y1, y2);
      draw = gp_line( self, x2, y1, x1 + 1, y1, 0);
      draw = gp_line( self, x1, y1, x1, y2 - 1, draw);
      draw = gp_line( self, x1, y2, x2 - 1, y2, draw);
      gp_line( self, x2, y2, x2, y1 + 1, draw);
   } else {
      check_swap( y1, y2);
      if ( sys stylus. pen. lopnWidth. x > 1 &&
           (sys stylus. pen. lopnWidth. x % 2) == 0
	 ) {
	 /* change up-winding to down-winding */
	 y1--;
	 y2--;
      }
      if ( !( ok = Rectangle( sys ps, x1, sys lastSize. y - y1, x2 + 1, sys lastSize. y - y2 - 1))) apiErr;
   }
   SelectObject( ps, old);
   return ok;
}}

Bool
apc_gp_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{objCheck false;{
   Bool ok = true;
   HDC     ps = sys ps;
   int compl, needf, newY = sys lastSize. y - y - 1, drawState = 0;
   Bool erratic = erratic_line( self);
   POINT   pts[ 2];
   HGDIOBJ old;

   compl = arc_completion( &angleStart, &angleEnd, &needf);
   old = SelectObject( ps, hBrushHollow);
   pts[ 0]. x = x + cos( angleEnd / GRAD) * dX / 2 + 0.5;
   pts[ 0]. y = newY - sin( angleEnd / GRAD) * dY / 2 + 0.5;
   pts[ 1]. x = x + cos( angleStart / GRAD) * dX / 2 + 0.5;
   pts[ 1]. y = newY - sin( angleStart / GRAD) * dY / 2 + 0.5;
   STYLUS_USE_PEN( ps);
   y = newY;

   while( compl--) {
      if ( erratic)
         drawState = gp_arc( self, x, y, dX, dY, 0, 360, drawState);
      else
         Arc( ps, ELLIPSE_RECT, ARC_COMPLETE);
   }
   if ( needf) {
      if ( erratic) {
         drawState = gp_arc( self, x, y, dX, dY, angleStart, angleEnd, drawState);
         drawState = gp_line( self, pts[ 1]. x, pts[ 1]. y, x, y, drawState);
         gp_line( self, x, y, pts[ 0]. x, pts[ 0]. y, drawState);
      } else
         if ( !( ok = Pie(
             ps, ELLIPSE_RECT,
             pts[ 1]. x, pts[ 1]. y,
             pts[ 0]. x, pts[ 0]. y
         ))) apiErr;
   }

   SelectObject( ps, old);
   return ok;
}}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
   objCheck false;
   SetPixelV( sys ps, x, sys lastSize. y - y - 1, remap_color( color, true));
   return true;
}


static Handle ctx_rop2R4[] = {
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

/*
static void dc2screen( HDC dc, Handle self)
{
   HDC xdc = dc_alloc();
   if ( !BitBlt( xdc, 0, 0, var w, var h, dc, 0, 0, SRCCOPY)) apiErr;
   dc_free();
}
*/

Bool
apc_gp_stretch_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{objCheck false;{
   PIcon  i    = ( PIcon) image;
   Handle deja = image;
   int    ly   = sys lastSize. y;
   HDC    xdc  = sys ps;
   HPALETTE p1, p2 = nil, p3;
   HBITMAP  b1;
   HDC dc;
   DWORD theRop;
   Bool ok = true, db, dcmono = false;
   POINT tr = {0, 0};

   COLORREF oFore = 0, oBack = 0;

   dobjCheck(image) false;
   db = dsys( image) options. aptDeviceBitmap || i-> options. optInDraw;

   if ( xLen == 0 || yLen == 0) return false;

   // Determinig whether we have bitmap adapted for output or it's just bare bits
   if ( db) {
      if (
          ( guts. displayBMInfo. bmiHeader. biBitCount == 8) &&
          ( !is_apt( aptCompatiblePS)) &&
          // ( is_apt( aptWinPS) || is_apt( aptBitmap))
          is_apt( aptBitmap)
         )
      {
         // There is a big uncertainity about 8-bit BitBlt's, based on fact that
         // memory DCs aren't really compatible with screen DCs, - correct results
         // could be guaranteed only when you Blt source DC = CreateCompatibleDC( dest DC),
         // not when source = CCDC( thirdDC), dest = CCDC( thirdDC) - that's right, I mean it!
         // or just SetDIBits; - latter is slow, stupid, memory-greedy - but surely can be trusted.
         Bool ok;
         Handle img      = ( Handle) create_object("Prima::Image", "");
         HDC adc         = dsys( image) ps;
         HBITMAP abitmap = dsys( image) bm;
         dsys( img) ps   = dsys( image) ps;
         dsys( img) bm   = dsys( image) bm;
         image_query_bits( img, true);
         dsys( img) ps   = adc;
         dsys( img) bm   = abitmap;
         ok = apc_gp_stretch_image( self, img, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
         Object_destroy( img);
         return ok;
      }

      dc = dsys( image) ps;
      SetViewportOrgEx( dc, 0, 0, &tr);
   } else {
      if ( is_apt( aptCompatiblePS))
         dc = CreateCompatibleDC( xdc);
      else
         dc = nil;
      if ( dc) {
         if ( dsys( image) bm == nil) {
            image_destroy_cache( image); // if palette still exists
            deja = image_enscreen( image, self);
            if ( !image_set_cache( deja, image)) {
	       // we're low on memory, reverting to StretchDIBits
	       DeleteDC( dc);
	       dc = nil;
	    }
         }
      } else
         deja = image_enscreen( image, self);
   }

   // actions for mono images
   if ( dsys( image) options. aptDeviceBitmap) {
      if ((( PDeviceBitmap) image)-> monochrome)
         STYLUS_USE_TEXT( sys ps);
   } else
      dcmono = i-> options. optInDraw ?  ( dsys( image) bpp == 1) : (( i-> type & imBPP) == 1);

   if ( dcmono) {
      PRGBColor pal = i-> palette;
      oFore  = GetTextColor( sys ps);
      oBack  = GetBkColor( sys ps);
      SetTextColor( sys ps,
         i-> palSize > 0
         ? RGB( pal[0].r, pal[0].g, pal[0].b)
         : RGB( 0, 0, 0));
      SetBkColor( sys ps,
         i-> palSize > 1
         ? RGB( pal[1].r, pal[1].g, pal[1].b)
         : RGB( 0xff, 0xff, 0xff));
   }

   // Winsloth 95 is morbid, but 98 is yet formidable!
   if ( IS_WIN95) {
      if ( xLen + xFrom > i-> w) {
         xDestLen = xDestLen * ( i-> w - xFrom) / xLen;
         xLen = i-> w - xFrom;
      }
      if ( yFrom < 0) {
         y -= yFrom * yDestLen / yLen;
         yDestLen += yFrom * yDestLen / yLen;
         yLen  += yFrom;
         yFrom = 0;
      }
   }

   // if image is actually icon, drawing and-mask
   if ( kind_of( deja, CIcon)) {
      XBITMAPINFO xbi = {
         { sizeof( BITMAPINFOHEADER), 0, 0, 1, 1, BI_RGB, 0, 0, 0, 2, 2},
         { {0,0,0,0}, {255,255,255,0}}
      };
      xbi. bmiHeader. biWidth = i-> w;
      xbi. bmiHeader. biHeight = i-> h;
      if ( StretchDIBits(
         xdc, x, ly - y - yDestLen, xDestLen, yDestLen, xFrom, yFrom, xLen, yLen,
         i-> mask, ( BITMAPINFO*) &xbi, DIB_RGB_COLORS, SRCAND
         ) == GDI_ERROR) {
         ok = false;
         apiErr;
      }
      theRop = SRCINVERT;
   } else {
      theRop = ctx_remap_def( rop, ctx_rop2R4, true, SRCCOPY);
   }

   // saving bitmap and palette ( if available) to both dc and xdc.
   p3 = dsys( image) pal;

   if ( p3 && !db && dc) 
      p1 = SelectPalette( dc, p3, 0);
   else
      p1 = nil;

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
            xFrom, i-> h - yFrom - yLen, xLen, yLen, theRop)) {
	 ok = false;
         apiErr;
      }
   } else {
      XBITMAPINFO xbi;
      BITMAPINFO * bi = image_get_binfo( deja, &xbi);
      if ( bi-> bmiHeader. biClrUsed > 0)
         bi-> bmiHeader. biClrUsed = bi-> bmiHeader. biClrImportant = PImage(deja)-> palSize;
      if ( StretchDIBits( xdc, x, ly - y - yDestLen, xDestLen, yDestLen,
            xFrom, yFrom,
            xLen, yLen, ((PImage) deja)-> data, bi,
            DIB_RGB_COLORS, theRop) == GDI_ERROR) {
         ok = false;
         apiErr;
      }
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

   if ( dc && tr. x != 0 && tr. y != 0) 
      SetViewportOrgEx( dc, tr. x, tr. y, NULL);

   if ( !db) {
      if ( dc) DeleteDC( dc);
      if ( deja != image) Object_destroy( deja);
   }

   return ok;
}}

Bool
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, Bool utf8 )
{objCheck false;{
   Bool ok = true;
   HDC ps = sys ps;
   int bk  = GetBkMode( ps);
   int opa = is_apt( aptTextOpaque) ? OPAQUE : TRANSPARENT;
   int div = 32768L / (var font. maximalWidth ? var font. maximalWidth : 1);
   if ( div <= 0) div = 1;

   /* Win32 has problems with text_out strings that are wider than
      32K pixel - it doesn't plot the string at all. This hack is
      although ugly, but is better that Win32 default behaviour, and
      at least can be excused by the fact that all GP spaces have
      their geometrical limits. */
   if ( len > div) len = div;

   if ( !HAS_WCHAR) utf8 = false;

   if ( utf8) {
      int mb_len;
      if ( !( text = ( char *) alloc_utf8_to_wchar( text, len, &mb_len))) return false;
      len = mb_len;
   }      

   if ( GetROP2( sys ps) != R2_COPYPEN) {
      STYLUS_USE_BRUSH( ps);
      BeginPath(ps);
      if ( utf8) 
         TextOutW( ps, x, sys lastSize. y - y, ( U16*)text, len);
      else
         TextOutA( ps, x, sys lastSize. y - y, text, len);
      EndPath(ps);
      FillPath(ps);
   } else {
      STYLUS_USE_TEXT( ps);
      if ( opa != bk) SetBkMode( ps, opa);

      if ( utf8) {
         if ( !( ok = TextOutW( ps, x, sys lastSize. y - y, ( U16*)text, len))) apiErr;
      } else {
         if ( !( ok = TextOutA( ps, x, sys lastSize. y - y, text, len))) apiErr;
      }
      if ( opa != bk) SetBkMode( ps, bk);
   }
   if ( utf8) free(( char *) text);
   return ok;
}}


// This procedure is about to compensate morbid Win95 behavior when
// calling GetCharABCWidthsFloat() - it returns ERROR_CALL_NOT_IMPLEMENTED. psst.
static BOOL
gp_GetCharABCWidthsFloat( HDC dc, UINT iFirstChar, UINT iLastChar, LPABCFLOAT lpABCF, Bool unicode)
{
   UINT i, d = iLastChar - iFirstChar + 1;
   TEXTMETRICW tm;

   if ( IS_NT)
      return unicode ? 
         GetCharABCWidthsFloatW( dc, iFirstChar, iLastChar, lpABCF) :
         GetCharABCWidthsFloatA( dc, iFirstChar, iLastChar, lpABCF);

   if ( iFirstChar > iLastChar)  // checking bound as far as we can
      return FALSE;

   if ( !HAS_WCHAR) unicode = false;
   
   if ( !gp_GetTextMetrics( dc, &tm)) // determining font
      return FALSE;

   if ( tm. tmPitchAndFamily & TMPF_TRUETYPE) {
      ABC *abc;
      int charExtra;
      INT fb;
      
      if ( unicode) {
         if ( !GetCharWidthW( dc, iFirstChar, iFirstChar, &fb))
            return FALSE;
      } else {
         if ( !GetCharWidthA( dc, iFirstChar, iFirstChar, &fb))
            return FALSE;
      }

      if ( !( abc = malloc( d * sizeof( ABC)))) 
         return FALSE;

      if ( unicode) {
         if ( !GetCharABCWidthsW( dc, iFirstChar, iLastChar, abc))
            apiErrRet;
      } else {
         if ( !GetCharABCWidthsA( dc, iFirstChar, iLastChar, abc))
            apiErrRet;
      }

      charExtra = fb - tm. tmOverhang - ( abc[0].abcA + abc[0].abcB + abc[0].abcC);
      if ( charExtra < 0) charExtra = 0;

      for ( i = 0; i <= iLastChar - iFirstChar; i++) {
         lpABCF[i]. abcfA = abc[ i]. abcA;
         lpABCF[i]. abcfB = abc[ i]. abcB + charExtra;
         lpABCF[i]. abcfC = abc[ i]. abcC;
      }

      free( abc);
   } else {
      INT * fb;

      if ( !( fb = malloc( d * sizeof( INT)))) 
         return FALSE;

      if ( unicode) {
         if ( !GetCharWidthW( dc, iFirstChar, iLastChar, fb)) {// checking full widths
            free( fb);
            return FALSE;
         }
      } else {
         if ( !GetCharWidthA( dc, iFirstChar, iLastChar, fb)) {// checking full widths
            free( fb);
            return FALSE;
         }
      }

      memset( lpABCF, 0, sizeof( ABCFLOAT) * ( iLastChar - iFirstChar + 1));

      for ( i = 0; i <= iLastChar - iFirstChar; i++) {
         lpABCF[i]. abcfA = tm. tmOverhang;
         lpABCF[i]. abcfB = fb[ i] - tm. tmOverhang;
         lpABCF[i]. abcfC = -tm. tmOverhang;
      }

      free( fb);
   }
   
   return TRUE;
}

#define TM(field) to->field = from->field
void
textmetric_c2w( LPTEXTMETRICA from, LPTEXTMETRICW to)
{
    TM(tmHeight); TM(tmAscent); TM(tmDescent); TM(tmInternalLeading);
    TM(tmExternalLeading); TM(tmAveCharWidth); TM(tmMaxCharWidth);
    TM(tmWeight); TM(tmOverhang); TM(tmDigitizedAspectX);
    TM(tmDigitizedAspectY); TM(tmFirstChar); TM(tmLastChar);
    TM(tmDefaultChar); TM(tmBreakChar); TM(tmItalic); TM(tmUnderlined);
    TM(tmStruckOut); TM(tmPitchAndFamily); TM(tmCharSet);
}
#undef TM
                

BOOL
gp_GetTextMetrics( HDC dc, LPTEXTMETRICW tm)
{
   BOOL ret;
   TEXTMETRICA a;
   if ( HAS_WCHAR)
      return GetTextMetricsW( dc, tm);
   ret = GetTextMetricsA( dc, &a);
   textmetric_c2w( &a, tm);
   return ret;
}

PFontABC
apc_gp_get_font_abc( Handle self, int first, int last, Bool unicode)
{objCheck nil;{
   int i;
   ABCFLOAT *f2;
   PFontABC  f1;

   f1 = ( PFontABC) malloc(( last - first + 1) * sizeof( FontABC));
   if ( !f1) return nil;
   f2 = ( ABCFLOAT*) malloc(( last - first + 1) * sizeof( ABCFLOAT));
   if ( !f2) {
      free( f1);
      return nil;
   }

   if ( !gp_GetCharABCWidthsFloat( sys ps, first, last, f2, unicode)) apiErr;
   for ( i = 0; i <= last - first; i++) {
      f1[i].a = f2[i].abcfA;
      f1[i].b = f2[i].abcfB;
      f1[i].c = f2[i].abcfC;
   }
   free( f2);
   return f1;
}}

static Handle ipa_ranges[] = {
  0x0250 , 0x02AF,	// 4  IPA Extensions
  0x1D00 , 0x1D7F,  //    Phonetic Extensions
  0x1D80 , 0x1DBF,  //    Phonetic Extensions Supplement
};

static Handle bopomoto_ranges[] = {
  0x3100 , 0x312f,  // 51 Bopomofo  
  0x31a0 , 0x31bf,  //    Extended Bopomofo 
};

static Handle cjk_ranges[] = {
  0x4e00 , 0x9fff,  // 59 CJK Unified Ideographs
  0x2e80 , 0x2eff,  //    CJK Radicals Supplement
  0x2f00 , 0x2fdf,  //    Kangxi Radicals
  0x2ff0 , 0x2fff,  //    Ideographic Description
  0x3400 , 0x4dbf,  //    CJK Unified Ideograph Extension A 
};  

static Handle unicode_subranges[ 84 * 2] = {
  0x0020 , 0x007e,  // 0  Basic Latin 
  0x0080 , 0x00ff,  // 1  Latin-1 Supplement 
  0x0100 , 0x017f,  // 2  Latin Extended-A 
  0x0180 , 0x024f,  // 3  Latin Extended-B 
        3, (Handle) &ipa_ranges,
  0x02b0 , 0x02ff,  // 5  Spacing Modifier Letters 
  0x0300 , 0x036f,  // 6  Combining Diacritical Marks 
  0x0370 , 0x03ff,  // 7  Basic Greek 
       0 ,      0,  // 8  Reserved  
  0x0400 , 0x04ff,  // 9  Cyrillic 
  0x0530 , 0x058f,  // 10 Armenian 
  0x0590 , 0x05ff,  // 11 Basic Hebrew 
       0 ,      0,  // 12 Reserved  
  0x0600 , 0x06ff,  // 13 Basic Arabic 
       0 ,      0,  // 14 Reserved  
  0x0900 , 0x097f,  // 15 Devanagari 
  0x0980 , 0x09ff,  // 16 Bengali 
  0x0a00 , 0x0a7f,  // 17 Gurmukhi 
  0x0a80 , 0x0aff,  // 18 Gujarati 
  0x0b00 , 0x0b7f,  // 19 Oriya 
  0x0b80 , 0x0bff,  // 20 Tamil 
  0x0c00 , 0x0c7f,  // 21 Telugu 
  0x0c80 , 0x0cff,  // 22 Kannada 
  0x0d00 , 0x0d7f,  // 23 Malayalam 
  0x0e00 , 0x0e7f,  // 24 Thai 
  0x0e80 , 0x0eff,  // 25 Lao 
  0x10a0 , 0x10ff,  // 26 Basic Georgian 
       0 ,      0,  // 27 Reserved  
  0x1100 , 0x11ff,  // 28 Hangul Jamo 
  0x1e00 , 0x1eff,  // 29 Latin Extended Additional 
  0x1f00 , 0x1fff,  // 30 Greek Extended 
  0x2000 , 0x206f,  // 31 General Punctuation 
  0x2070 , 0x209f,  // 32 Subscripts and Superscripts 
  0x20a0 , 0x20cf,  // 33 Currency Symbols 
  0x20d0 , 0x20ff,  // 34 Combining Diacritical Marks for Symbols 
  0x2100 , 0x214f,  // 35 Letter-like Symbols 
  0x2150 , 0x218f,  // 36 Number Forms 
  0x2190 , 0x21ff,  // 37 Arrows 
  0x2200 , 0x22ff,  // 38 Mathematical Operators 
  0x2300 , 0x23ff,  // 39 Miscellaneous Technical 
  0x2400 , 0x243f,  // 40 Control Pictures 
  0x2440 , 0x245f,  // 41 Optical Character Recognition 
  0x2460 , 0x24ff,  // 42 Enclosed Alphanumerics 
  0x2500 , 0x257f,  // 43 Box Drawing 
  0x2580 , 0x259f,  // 44 Block Elements 
  0x25a0 , 0x25ff,  // 45 Geometric Shapes 
  0x2600 , 0x26ff,  // 46 Miscellaneous Symbols 
  0x2700 , 0x27bf,  // 47 Dingbats 
  0x3000 , 0x303f,  // 48 Chinese, Japanese, and Korean (CJK) Symbols and Punctuation 
  0x3040 , 0x309f,  // 49 Hiragana 
  0x30a0 , 0x30ff,  // 50 Katakana 
       2,  (Handle) &bopomoto_ranges,
  0x3130 , 0x318f,  // 52 Hangul Compatibility Jamo 
  0x3190 , 0x319f,  // 53 CJK Miscellaneous 
  0x3200 , 0x32ff,  // 54 Enclosed CJK Letters and Months 
  0x3300 , 0x33ff,  // 55 CJK Compatibility 
  0xac00 , 0xd7a3,  // 56 Hangul 
  0xd800 , 0xdfff,  // 57 Surrogates
       0 ,      0,  // 58 Reserved  
       5,  (Handle) &cjk_ranges,
  0xe000 , 0xf8ff,  // 60 Private Use Area 
  0xf900 , 0xfaff,  // 61 CJK Compatibility Ideographs 
  0xfb00 , 0xfb4f,  // 62 Alphabetic Presentation Forms 
  0xfb50 , 0xfdff,  // 63 Arabic Presentation Forms-A 
  0xfe20 , 0xfe2f,  // 64 Combining Half Marks 
  0xfe30 , 0xfe4f,  // 65 CJK Compatibility Forms 
  0xfe50 , 0xfe6f,  // 66 Small Form Variants 
  0xfe70 , 0xfefe,  // 67 Arabic Presentation Forms-B 
  0xff00 , 0xffef,  // 68 Halfwidth and Fullwidth Forms 
  0xfff0 , 0xfffd,  // 69 Specials 
  0x0f00 , 0x0fcf,  // 70 Tibetan 
  0x0700 , 0x074f,  // 71 Syriac 
  0x0780 , 0x07bf,  // 72 Thaana 
  0x0d80 , 0x0dff,  // 73 Sinhala 
  0x1000 , 0x109f,  // 74 Myanmar 
  0x1200 , 0x12bf,  // 75 Ethiopic 
  0x13a0 , 0x13ff,  // 76 Cherokee 
  0x1400 , 0x14df,  // 77 Canadian Aboriginal Syllabics 
  0x1680 , 0x169f,  // 78 Ogham 
  0x16a0 , 0x16ff,  // 79 Runic 
  0x1780 , 0x17ff,  // 80 Khmer 
  0x1800 , 0x18af,  // 81 Mongolian 
  0x2800 , 0x28ff,  // 82 Braille 
  0xa000 , 0xa48c   // 83 YI, Yi Radicals 
};

static Handle ctx_CHARSET2index[] = {
  // SHIFTJIS_CHARSET   ??
  HANGEUL_CHARSET     , 28, 
  // GB2312_CHARSET     ?? 
  // CHINESEBIG5_CHARSET ??
#ifdef JOHAB_CHARSET
  GREEK_CHARSET       , 7,
  HEBREW_CHARSET      , 11,
  ARABIC_CHARSET      , 13,
  // TURKISH_CHARSET    ??
  // VIETNAMESE_CHARSET ??
  THAI_CHARSET        , 24,
  EASTEUROPE_CHARSET  , 3,
  RUSSIAN_CHARSET     , 9,
  // MAC_CHARSET        ??
  BALTIC_CHARSET      , 2,
#endif  
  endCtx
};

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{objCheck nil;{
   int i;
   unsigned long * ret;
   FONTSIGNATURE f;

   if ( !HAS_WCHAR) {
      TEXTMETRIC tm;
      GetTextMetrics( sys ps, &tm);
      if ( !( ret = malloc( sizeof( unsigned long) * 2)))
        return nil;
      ret[0] = tm. tmFirstChar;
      ret[1] = tm. tmLastChar;
      *count = 2;
      return ret;
   }
   
   memset( &f, 0, sizeof(f));
   i = GetTextCharsetInfo( sys ps, &f, 0);
   if ( i == DEFAULT_CHARSET)
      apiErrRet;

   if ( f. fsUsb[0] == 0 && f. fsUsb[1] == 0 && f. fsUsb[2] == 0 && f. fsUsb[3] == 0) {
      int index = ctx_remap_end( i, ctx_CHARSET2index, true);
      TEXTMETRICW tm;
      if ( !( ret = malloc( sizeof( unsigned long) * 4)))
        return nil;
      gp_GetTextMetrics( sys ps, &tm);
      if ( index == endCtx) {
         ret[0] = 0x20;
         ret[1] = 0xff;
         *count = 2;
      } else {
         *count = 0;
         if ( index > 0) {
            ret[(*count)++] = unicode_subranges[ 0];
            ret[(*count)++] = unicode_subranges[ 1];
         }
         ret[(*count)++] = unicode_subranges[ index * 2];
         ret[(*count)++] = unicode_subranges[ index * 2 + 1];
      }
      if ( ret[0] > tm. tmFirstChar) ret[0] = tm.tmFirstChar;
      return ret;
   }

   for ( i = 0; i < 84; i++) 
      if ( f. fsUsb[ i / 32] & ( 1 << (i % 32))) {
         Handle x = unicode_subranges[i * 2];
         if ( x < 0x20)
            (*count) += x * 2;
         else
            (*count) += 2;
      }

   if ( !( ret = malloc( sizeof( long) * (*count))))
      return nil;
   
   *count = 0;
   for ( i = 0; i < 84; i++) 
      if ( f. fsUsb[ i / 32] & ( 1 << (i % 32))) {
         Handle x = unicode_subranges[i * 2];
         if ( x < 0x20) {
            Handle * z = ( Handle *) unicode_subranges[i * 2 + 1];
            while ( x--) {
               ret[ (*count)++] = *(z++);
               ret[ (*count)++] = *(z++);
            }
         } else {
            ret[ (*count)++] = x;
            ret[ (*count)++] = unicode_subranges[i * 2 + 1];
         }
      }

   return ret;
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
   return remap_color( sys ps ? sys stylus. pen. lopnColor : sys lbs[0], false);
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   RECT r;
   Rect rr = {0,0,0,0};
   objCheck rr;
   if ( !is_opt( optInDraw) || !sys ps) return rr;
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

Bool
apc_gp_get_fill_winding( Handle self)
{
   objCheck 0;
   if ( ! sys ps) return sys fillWinding;
   return GetPolyFillMode( sys ps) == WINDING;
}

static Handle ctx_le2PS_ENDCAP[] = {
    leRound,          PS_ENDCAP_ROUND             ,
    leSquare,         PS_ENDCAP_SQUARE            ,
    leFlat,           PS_ENDCAP_FLAT              ,
    endCtx
};

int
apc_gp_get_line_end( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys lineEnd;
   return ctx_remap_def( sys stylus. extPen. lineEnd, ctx_le2PS_ENDCAP, false, leRound);
}

static Handle ctx_lj2PS_JOIN[] = {
    ljRound,          PS_JOIN_ROUND             ,
    ljBevel,          PS_JOIN_BEVEL             ,
    ljMiter,          PS_JOIN_MITER             ,
    endCtx
};

int
apc_gp_get_line_join( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys lineJoin;
   return ctx_remap_def( sys stylus. extPen. lineJoin, ctx_lj2PS_JOIN, false, ljRound);
}

int
apc_gp_get_line_width( Handle self)
{
   objCheck 0;
   if ( !sys ps) return sys lineWidth;
   return sys stylus. pen. lopnWidth. x;
}

int
apc_gp_get_line_pattern( Handle self, unsigned char * buffer)
{
   objCheck 0;
   if ( !sys ps) {
      strcpy(( char *) buffer, (char*)(( sys linePatternLen > 3) ? sys linePattern : (Byte*)(&sys linePattern)));
      return sys linePatternLen;
   }

   switch ( sys stylus. pen. lopnStyle) {
   case PS_NULL:
       strcpy(( char *) buffer, "");
       return 0;
   case PS_DASH:
       strcpy(( char *) buffer, psDash);
       return 2;
   case PS_DOT:
       strcpy(( char *) buffer, psDot);
       return 2;
   case PS_DASHDOT:
       strcpy(( char *) buffer, psDashDot);
       return 4;
   case PS_DASHDOTDOT:
       strcpy(( char *) buffer, psDashDotDot);
       return 6;
   case PS_USERSTYLE:
       {
          int i;
          int len = sys stylus. extPen. patResource-> dotsCount;
          if ( len > 255) len = 255;
          for ( i = 0; i < len; i++)
             buffer[ i] = sys stylus. extPen. patResource-> dots[ i];
          return len;
       }
   default:
       strcpy(( char *) buffer, "\1");
       return 1;
   }
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

   r = ( PRGBColor) malloc( sizeof( RGBColor) * *color);
   if ( !r) return nil;

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

   if ( !( dc = dc_compat_alloc(0))) return true;
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

static Handle ctx_rop2R2[] = {
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


static int
gp_get_text_width( Handle self, const char* text, int len, Bool addOverhang, Bool wide)
{
   SIZE  sz;
   int   div, offset = 0, ret = 0;
   
   objCheck 0;
   if ( len == 0) return 0;

   if ( !HAS_WCHAR) wide = false;

   /* width more that 32K returned incorrectly by Win32 core */
   if (( div = 32768L / ( var font. maximalWidth ? var font. maximalWidth : 1)) == 0)
      div = 1;

   while ( offset < len) {
      int chunk_len = ( offset + div > len) ? ( len - offset) : div;
      if ( wide) {
         if ( !GetTextExtentPoint32W( sys ps, ( WCHAR*) text + offset, chunk_len, &sz)) apiErr;
      } else {
         if ( !GetTextExtentPoint32( sys ps, text + offset, chunk_len, &sz)) apiErr;
      }
      ret += sz. cx;
      if ( !IS_NT && offset > 0) ret -= sys tmOverhang;
      offset += div;
   }
   
   if ( addOverhang) {
      if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
         ABC abc[2];
         if ( wide) {
            GetCharABCWidthsW( sys ps, *((WCHAR*)text), *((WCHAR*)text), &abc[0]);
            GetCharABCWidthsW( sys ps, *((WCHAR*)text + len - 1), *((WCHAR*)text + len - 1), &abc[1]);
         } else {
            GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
            GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
         }
         if ( abc[0]. abcA < 0) ret -= abc[0]. abcA;
         if ( abc[1]. abcC < 0) ret -= abc[1]. abcC;
      } else if ( IS_NT)
         ret += sys tmOverhang;
   } else if ( !IS_NT) 
      ret -= sys tmOverhang;

   return ret;
}

int
apc_gp_get_text_width( Handle self, const char* text, int len, Bool addOverhang, Bool utf8)
{
   int ret;
   if ( !HAS_WCHAR) utf8 = false;
   if ( utf8) {
      int mb_len;
      if ( !( text = ( char *) alloc_utf8_to_wchar( text, len, &mb_len))) return 0;
      len = mb_len;
   }      
   ret = gp_get_text_width( self, text, len, addOverhang, utf8);
   if ( utf8)
      free(( char*) text);
   return ret;
}   

Point *
apc_gp_get_text_box( Handle self, const char* text, int len, Bool utf8)
{objCheck nil;{
   Point * pt = ( Point *) malloc( sizeof( Point) * 5);
   if ( !pt) return nil;

   if ( !HAS_WCHAR) utf8 = false;
   memset( pt, 0, sizeof( Point) * 5);

   if ( utf8) {
      int mb_len;
      if ( !( text = ( char *) alloc_utf8_to_wchar( text, len, &mb_len))) {
         free( pt);
         return nil;
      }
      len = mb_len;
   }

   pt[0].y = pt[2]. y = var font. ascent - 1;
   pt[1].y = pt[3]. y = - var font. descent;
   pt[4].y = pt[0]. x = pt[1].x = 0;
   pt[3].x = pt[2]. x = pt[4].x = gp_get_text_width( self, text, len, false, utf8);
   if ( len > 0 && !IS_NT) {
      pt[2].x += sys tmOverhang;
      pt[3].x += sys tmOverhang;
   }

   if ( !is_apt( aptTextOutBaseline)) {
      int i = 4, d = var font. descent;
      while ( i--) pt[ i]. y += d;
   }

   if ( sys tmPitchAndFamily & TMPF_TRUETYPE) {
      ABC abc[2];
      if ( utf8) {
         GetCharABCWidthsW( sys ps, *((WCHAR*)text), *((WCHAR*)text), &abc[0]);
         GetCharABCWidthsW( sys ps, *((WCHAR*)text + len - 1), *((WCHAR*)text + len - 1), &abc[1]);
      } else {
         GetCharABCWidths( sys ps, text[ 0    ], text[ 0    ], &abc[0]);
         GetCharABCWidths( sys ps, text[ len-1], text[ len-1], &abc[1]);
      }
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
      float s = sin( var font. direction / GRAD);
      float c = cos( var font. direction / GRAD);
      for ( i = 0; i < 5; i++) {
         float x = pt[i]. x * c - pt[i]. y * s;
         float y = pt[i]. x * s + pt[i]. y * c;
         pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
         pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
      }
   }

   if ( utf8) free(( char*) text);
   
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
   p. y -= sys transform2. y;
   return p;
}

Bool
apc_gp_get_text_opaque( Handle self)
{
   objCheck false;
   return is_apt( aptTextOpaque);
}

#define pal_ok ((sys bpp <= 8) && ( sys pal))

Bool
apc_gp_set_back_color( Handle self, Color color)
{
   long clr = remap_color( color, true);
   objCheck false;
   if ( sys ps) {
      PStylus s = & sys stylus;
      if ( pal_ok) clr = palette_match( self, clr);
      if ( SetBkColor( sys ps, clr) == CLR_INVALID) apiErr;
      s-> brush. backColor = clr;
      if ( s-> brush. lb. lbStyle == BS_DIBPATTERNPT)
         stylus_change( self);
   }
   sys lbs[1] = clr;
   return true;
}

Bool
apc_gp_set_clip_rect( Handle self, Rect c)
{
   HRGN rgn;
   objCheck false;
   if ( !is_opt( optInDraw) || !sys ps) return true;
   // inclusive-exclusive
   c. left   -= sys transform2. x;
   c. right  -= sys transform2. x;
   c. top    += sys transform2. y;
   c. bottom += sys transform2. y;
   check_swap( c. top, c. bottom);
   check_swap( c. left, c. right);
   if ( !( rgn = CreateRectRgn( c. left,  sys lastSize. y - c. top,
                                c. right + 1, sys lastSize. y - c. bottom - 1))) apiErrRet;
   if ( !SelectClipRgn( sys ps, rgn)) apiErr;
   if ( !DeleteObject( rgn)) apiErr;
   return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
   long clr = remap_color( color, true);
   objCheck false;
   if ( !sys ps)
      sys lbs[0] = clr;
   else {
      PStylus s = & sys stylus;
      if ( pal_ok) clr = palette_match( self, clr);
      s-> pen. lopnColor = ( COLORREF) clr;
      if ( s-> brush. lb. lbStyle != BS_DIBPATTERNPT) s-> brush. lb. lbColor = ( COLORREF) clr;
      stylus_change( self);
   }
   return true;
}

Bool
apc_gp_set_fill_winding( Handle self, Bool fillWinding)
{
   objCheck false;
   if ( sys ps) 
      SetPolyFillMode( sys ps, fillWinding ? WINDING : ALTERNATE);
   else
      sys fillWinding = fillWinding;
   return true;
}

Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   objCheck false;
{
   HDC ps    = sys ps;
   PStylus s = & sys stylus;
   long *p1 = ( long*) pattern;
   long *p2 = p1 + 1;
   if ( !ps) {
      memcpy( &sys fillPattern2, pattern, sizeof( FillPattern));
      return true;
   }
   memcpy( &sys fillPattern, pattern, sizeof( FillPattern));
   if (( *p1 == 0) && ( *p2 == 0)) {
      s-> brush. lb. lbStyle = BS_SOLID;
      s-> brush. lb. lbColor = GetBkColor( ps);
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
      s-> brush. lb. lbHatch = ( LONG_PTR) &bmiHatch;
      s-> brush. backColor   = GetBkColor( ps);
      memcpy( s-> brush. pattern, pattern, sizeof( FillPattern));
   }
   stylus_change( self);
   return true;
}}

Bool
apc_gp_set_font( Handle self, PFont font)
{
   TEXTMETRICW tm;
   objCheck false;
   if ( !sys ps) return true;
   font_change( self, font);
   gp_GetTextMetrics( sys ps, &tm);
   sys tmOverhang       = tm. tmOverhang;
   sys tmPitchAndFamily = tm. tmPitchAndFamily;
   return true;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
   objCheck nil;
   return sys ps ? &sys fillPattern : &sys fillPattern2;
}

Bool
apc_gp_set_line_end( Handle self, int lineEnd)
{
   objCheck false;
   if ( !sys ps) sys lineEnd = lineEnd; else {
      PStylus s         = &sys stylus;
      PEXTPEN ep        = &s-> extPen;
      ep-> lineEnd      = ctx_remap_def( lineEnd, ctx_le2PS_ENDCAP, true, PS_ENDCAP_ROUND);
      if (( ep-> actual  = stylus_extpenned( s, 0 & exsLineEnd)))
         ep-> style = stylus_get_extpen_style( s);
      stylus_change( self);
   }
   return true;
}

Bool
apc_gp_set_line_join( Handle self, int lineJoin)
{
   objCheck false;
   if ( !sys ps) sys lineJoin = lineJoin; else {
      PStylus s         = &sys stylus;
      PEXTPEN ep        = &s-> extPen;
      ep-> lineJoin     = ctx_remap_def( lineJoin, ctx_lj2PS_JOIN, true, PS_JOIN_ROUND);
      if (( ep-> actual = stylus_extpenned( s, 0 & exsLineJoin)))
         ep-> style = stylus_get_extpen_style( s);
      stylus_change( self);
   }
   return true;
}

Bool
apc_gp_set_line_width( Handle self, int lineWidth)
{
   objCheck false;
   if ( !sys ps) sys lineWidth = lineWidth; else {
      PStylus s = &sys stylus;
      if ( lineWidth < 0 || lineWidth > 8192) lineWidth = 0;
      s-> pen. lopnWidth. x = lineWidth;
      stylus_change( self);
   }
   return true;
}

Bool
apc_gp_set_line_pattern( Handle self, unsigned char * pattern, int len)
{
   objCheck false;
   if ( !sys ps) {
      if ( sys linePatternLen > 3)
         free( sys linePattern);
      if ( len > 3) {
         sys linePattern = ( unsigned char *) malloc( len);
         if ( !sys linePattern) {
            sys linePatternLen = 0;
            return false;
         }
         memcpy( sys linePattern, pattern, len);
      } else
         memcpy( &sys linePattern, pattern, len);
      sys linePatternLen = len;
   } else {
      PStylus s           = &sys stylus;
      PEXTPEN ep          = &s-> extPen;

      if ( IS_WIN95) {
         if ( sys linePatternLen2 > 3)
            free( sys linePattern2);
         if ( len > 3) {
            sys linePattern2 = ( unsigned char *) malloc( len);
            if ( !sys linePattern2) {
               sys linePatternLen2 = 0;
               return false;
            }
            memcpy( sys linePattern2, pattern, len);
         } else
            memcpy( &sys linePattern2, pattern, len);
         sys linePatternLen2 = len;
      }

      s-> pen. lopnStyle  = patres_user( pattern, len);
      if (( ep-> actual    = stylus_extpenned( s, 0 & exsLinePattern))) {
         ep-> style       = stylus_get_extpen_style( s);
         ep-> patResource = ( s-> pen. lopnStyle == PS_USERSTYLE) ?
            patres_fetch( pattern, len) : &hPatHollow;
      } else
         ep-> patResource = &hPatHollow;
      stylus_change( self);
   }
   return true;
}

Bool
apc_gp_set_palette( Handle self)
{
   HPALETTE pal;

   objCheck false;
   if ( sys p256) {
      free( sys p256);
      sys p256 = nil;
   }

   pal = palette_create( self);
   if ( sys ps) {
      if ( pal)
         SelectPalette( sys ps, pal, 0);
      else
         SelectPalette( sys ps, sys stockPalette, 1);
      RealizePalette( sys ps);
   }
   if ( sys pal) DeleteObject( sys pal);
   sys pal = pal;
   return true;
}

Bool
apc_gp_set_region( Handle self, Handle mask)
{
   HRGN rgn = nil;
   objCheck false;

   if ( !is_opt( optInDraw) || !sys ps) return true;

   rgn = region_create( mask);
   if ( !rgn) {
      SelectClipRgn( sys ps, nil);
      return true;
   }
   OffsetRgn( rgn, -sys transform2. x, -sys transform2. y);
   OffsetRgn( rgn, 0, sys lastSize.y - PImage(mask)-> h);
   SelectClipRgn( sys ps, rgn);
   DeleteObject( rgn);
   return true;
}

Bool
apc_gp_set_rop( Handle self, int rop)
{
   objCheck false;
   if ( !sys ps) { sys rop = rop; return true; }
   if ( !SetROP2( sys ps, ctx_remap_def( rop, ctx_rop2R2, true, R2_COPYPEN))) apiErr;
   return true;
}

Bool
apc_gp_set_rop2( Handle self, int rop)
{
   objCheck false;
   if ( !sys ps) { sys rop2 = rop; return true; }
   if ( rop != ropCopyPut) rop = ropNoOper;
   if ( !SetBkMode( sys ps, ( rop == ropCopyPut) ? OPAQUE : TRANSPARENT)) apiErr;
   return true;
}

Bool
apc_gp_set_transform( Handle self, int x, int y)
{
   objCheck false;
   if ( !sys ps) {
      sys transform. x = x;
      sys transform. y = y;
      return true;
   }
   if ( !SetViewportOrgEx( sys ps, x - sys transform2. x, - ( y + sys transform2. y), nil)) apiErr;
   return true;
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
   objCheck false;
   apt_assign( aptTextOpaque, opaque);
   return true;
}


Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
   objCheck false;
   apt_assign( aptTextOutBaseline, baseline);
   if ( sys ps) SetTextAlign( sys ps, baseline ? TA_BASELINE : TA_BOTTOM);
   return true;
}

#ifdef __cplusplus
}
#endif
