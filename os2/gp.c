/*
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
/* apc.c --- apc/ api for os/2 */
#include <limits.h>
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#include "os2/os2guts.h"
#include "DeviceBitmap.h"
#include "Icon.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)

Bool
apc_gp_init( Handle self)
{
   sys res = guts. displayResolution;
   return true;
}

Bool
apc_gp_done( Handle self)
{
   free( sys bmRaw); sys bmRaw = nil;
   free( sys bmInfo); sys bmInfo = nil;
   if ( sys linePatternLen > 3) free( sys linePattern);
   if ( sys bm)
      if ( !GpiDeleteBitmap( sys bm)) apiErr;
   if ( sys ps)
   {
      if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
         if ( !WinEndPaint( sys ps)) apiErr;
      } else if ( is_apt( aptWinPS)) {
         if ( !WinReleasePS( sys ps)) apiErr;
      } else
         GpiSetBitmap( sys ps, nilHandle); // bitmapped PS
   }
   sys ps = sys bm = nilHandle;
   return true;
}

// gpi various fixes

#define apc_gp_move( ps, x, y) {                                             \
                                  POINTL ___p = {x, y};                      \
                                  if ( !GpiMove( ps, &___p)) apiErr;         \
                               }

#define apc_gp_fix             if ( sys lineWidth > 0 || sys linePatternLen < 2) { \
                                  if ( !GpiBeginPath( sys ps, 1)) apiErr;\
                               }


#define apc_gp_fix_end         if ( sys lineWidth > 0 || sys linePatternLen < 2) {\
                                  if ( !GpiEndPath( sys ps)) apiErr;         \
                                  if ( !GpiModifyPath( sys ps, 1, MPATH_STROKE)) apiErr;    \
                                  if ( GpiFillPath( sys ps, 1, FPATH_WINDING) == GPI_ERROR); \
                               }

#define gp_arc_set {               \
  ARCPARAMS arc;                   \
  arc. lP = dX;                    \
  arc. lQ = dY;                    \
  arc. lR = 0;                     \
  arc. lS = 0;                     \
  if ( !GpiSetArcParams( sys ps, &arc)) apiErr;  \
  if ( angleStart > angleEnd)      \
  {                                \
     double a = angleEnd;          \
     angleEnd = angleStart;        \
     angleStart = a;               \
  }                                \
}


Bool
apc_gp_arc ( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   POINTL ptl = { x, y};
   LONG lType = GpiQueryLineType( sys ps);
   gp_arc_set;
   if ( !GpiSetLineType ( sys ps, LINETYPE_INVISIBLE)) apiErr;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), MAKEFIXED(0, 0)) == GPI_ERROR) apiErr;
   if ( !GpiSetLineType ( sys ps, lType)) apiErr;
   apc_gp_fix;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool                                                     /* no fix */
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   POINTL p;
   apc_gp_move( sys ps, x1, y1);
   p. x = x2; p. y = y2;
   if ( GpiBox( sys ps, DRO_FILL, &p, 0, 0) == GPI_ERROR) apiErrRet;
   return true;
}

Bool
apc_gp_clear( Handle self, int x1, int y1, int x2, int y2)      /* no fix */
{
   POINTL p;
   HPS ps = sys ps;
   LONG patternId, pattern, color;
   if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
      SIZEL sz;
      GpiQueryPS( ps, &sz);
      x1 = y1 = 0;
      x2 = sz. cx - 1;
      y2 = sz. cy - 1;
   }
   apc_gp_move( ps, x1, y1);
   p. x = x2; p. y = y2;
   patternId = GpiQueryPatternSet( ps);
   if ( patternId != 0) {
      GpiSetPatternSet( ps, 0);
   }
   pattern = GpiQueryPattern( ps);
   if ( pattern != PATSYM_SOLID) GpiSetPattern( ps, PATSYM_SOLID);
   color = GpiQueryColor( ps);
   GpiSetColor( ps, GpiQueryBackColor( ps));

   if ( GpiBox( ps, DRO_FILL, &p, 0, 0) == GPI_ERROR) apiErrRet;

   GpiSetColor( ps, color);
   if ( pattern   != PATSYM_SOLID) GpiSetPattern( ps, pattern);
   if ( patternId != 0) GpiSetPatternSet( ps, patternId);
   return true;
}

Bool
apc_gp_chord( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
   POINTL ptl = { x, y};
   LONG lType = GpiQueryLineType( sys ps);
   gp_arc_set;
   if ( !GpiSetLineType ( sys ps, LINETYPE_INVISIBLE)) apiErr;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   if ( !GpiSetLineType ( sys ps, lType)) apiErr;
   apc_gp_fix;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool
apc_gp_draw_poly( Handle self, int numPts, Point * points)
{
   apc_gp_move( sys ps, points[0].x, points[0].y);
   apc_gp_fix;
   if ( GpiPolyLine( sys ps, numPts - 1, ( PPOINTL)&points[1]) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{
   apc_gp_move( sys ps, points[0].x, points[0].y);
   apc_gp_fix;
   if ( GpiPolyLineDisjoint( sys ps, numPts, ( PPOINTL) points) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool
apc_gp_ellipse( Handle self, int x, int y, int dX, int dY)
{
   ARCPARAMS arc;

   arc. lP = dX;
   arc. lQ = dY;
   arc. lR = 0;
   arc. lS = 0;
   if ( !GpiSetArcParams( sys ps, &arc)) apiErr;
   apc_gp_move( sys ps, x, y);
   apc_gp_fix;
   if ( GpiFullArc( sys ps, DRO_OUTLINE, MAKEFIXED(0, 0x8000)) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool
apc_gp_fill_chord ( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
   POINTL ptl = { x, y};
   LONG lType = GpiQueryLineType( sys ps);
   gp_arc_set;
   if ( !GpiSetLineType ( sys ps, LINETYPE_INVISIBLE)) apiErr;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   if ( !GpiSetLineType ( sys ps, lType)) apiErr;
   if ( !GpiBeginPath( sys ps, 1)) apiErr;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   if ( !GpiEndPath( sys ps)) apiErr;
   if ( GpiFillPath( sys ps, 1, sys fillWinding ? FPATH_WINDING : FPATH_ALTERNATE) == GPI_ERROR) apiErrRet;
   return true;
}

Bool                                                        /* no fix */
apc_gp_fill_ellipse( Handle self, int x, int y, int dX, int dY)
{
   ARCPARAMS arc;

   arc. lP = dX;
   arc. lQ = dY;
   arc. lR = 0;
   arc. lS = 0;
   if ( !GpiSetArcParams( sys ps, &arc)) apiErr;
   apc_gp_move( sys ps, x, y);
   if ( GpiFullArc( sys ps, DRO_FILL, MAKEFIXED(0, 0x8000)) == GPI_ERROR) apiErrRet;
   return true;
}

Bool
apc_gp_fill_poly( Handle self, int numPts, Point * points)
{
   apc_gp_move( sys ps, points[0].x, points[0].y);
   if ( !GpiBeginPath( sys ps, 1)) apiErr;
   if ( GpiPolyLine( sys ps, numPts - 1, ( PPOINTL)&points[1]) == GPI_ERROR) apiErr;
   if ( !GpiEndPath( sys ps)) apiErr;
   if ( GpiFillPath( sys ps, 1, sys fillWinding ? FPATH_WINDING : FPATH_ALTERNATE) == GPI_ERROR) apiErrRet;
   return true;
}

Bool
apc_gp_fill_sector ( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
   POINTL ptl = { x, y};
   gp_arc_set;
   if ( !GpiBeginPath( sys ps, 1)) apiErr;
   if ( !GpiMove( sys ps, &ptl)) apiErr;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   if ( GpiLine( sys ps, &ptl) == GPI_ERROR) apiErr;
   if ( !GpiEndPath( sys ps)) apiErr;
   if ( GpiFillPath( sys ps, 1, sys fillWinding ? FPATH_WINDING : FPATH_ALTERNATE) == GPI_ERROR) apiErrRet;
   return true;
}

Bool                                                       /* no fix */
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{
   apc_gp_move( sys ps, x, y);
   if ( GpiFloodFill( sys ps, singleBorder ? FF_SURFACE : FF_BOUNDARY,
      remap_color ( sys ps, borderColor, true)) == GPI_ERROR) apiErrRet;
   return true;
}

Color
apc_gp_get_pixel ( Handle self, int x, int y)
{
   POINTL p = {x, y};
   LONG ret = GpiQueryPel ( sys ps, &p);
   if ( ret == GPI_ALTERROR) {
      apiErr;
      return clInvalid;
   }
   return remap_color( sys ps, ret, false);
}

ApiHandle
apc_gp_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}

Bool
apc_gp_line ( Handle self, int x1, int y1, int x2, int y2)
{
   POINTL p = {x2, y2};
   apc_gp_move( sys ps, x1, y1);
   apc_gp_fix;
   if ( GpiLine( sys ps, &p) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

static int ctx_rop2ROP[] = {
   ropCopyPut,      ROP_SRCCOPY,
   ropAndPut,       ROP_SRCAND,
   ropBlackness,    ROP_ZERO,
   ropInvert,       ROP_DSTINVERT,
   ropNoOper,       0xAA,
   ropNotAnd,       0x77,
   ropNotDestAnd,   ROP_SRCERASE,
   ropNotDestOr,    0xDD,
   ropNotOr,        ROP_NOTSRCERASE,
   ropNotPut,       ROP_NOTSRCCOPY,
   ropNotSrcAnd,    0x22,
   ropNotSrcOr,     ROP_MERGEPAINT,
   ropNotXor,       0x99,
   ropOrPut,        ROP_SRCPAINT,
   ropWhiteness,    ROP_ONE,
   ropXorPut,       ROP_SRCINVERT,
   endCtx
};

static int ctx_rop2FM[] = {
   ropCopyPut      , FM_OVERPAINT,
   ropCopyPut      , FM_DEFAULT,
   ropOrPut        , FM_OR,
   ropXorPut       , FM_XOR,
   ropNoOper       , FM_LEAVEALONE,
   ropAndPut       , FM_AND,
   ropNotSrcAnd    , FM_SUBTRACT,
   ropNotDestAnd   , FM_MASKSRCNOT,
   ropBlackness    , FM_ZERO,
   ropNotOr        , FM_NOTMERGESRC,
   ropNotXor       , FM_NOTXORSRC,
   ropInvert       , FM_INVERT,
   ropNotDestOr    , FM_MERGESRCNOT,
   ropNotPut       , FM_NOTCOPYSRC,
   ropNotSrcOr     , FM_MERGENOTSRC,
   ropNotAnd       , FM_NOTMASKSRC,
   ropWhiteness    , FM_ONE,
   endCtx
};

static int ctx_rop2BM[] = {
   ropCopyPut      , BM_OVERPAINT,
   ropXorPut       , BM_XOR,
   ropOrPut        , BM_OR,
   ropNoOper       , BM_LEAVEALONE,
   ropNoOper       , BM_DEFAULT,
   endCtx
};

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   return apc_gp_stretch_image ( self, image, x, y, xFrom, yFrom, xLen, yLen, xLen, yLen, rop);
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
   POINTL p;
   apc_gp_move( sys ps, x1, y1);
   p. x = x2; p. y = y2;
   apc_gp_fix;
   if ( GpiBox( sys ps, DRO_OUTLINE, &p, 0, 0) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool
apc_gp_sector ( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
   POINTL ptl = { x, y};
   gp_arc_set;
   if ( !GpiMove( sys ps, &ptl)) apiErr;
   apc_gp_fix;
   if ( GpiPartialArc ( sys ps, &ptl, MAKEFIXED( 0, 0x8000), float2fixed(angleStart), float2fixed(angleEnd-angleStart)) == GPI_ERROR) apiErr;
   if ( GpiLine( sys ps, &ptl) == GPI_ERROR) apiErr;
   apc_gp_fix_end;
   return true;
}

Bool                                                         /* no fix */
apc_gp_set_pixel ( Handle self, int x, int y, Color color)
{
   POINTL p = {x, y};
   LONG c = GpiQueryColor ( sys ps);
   GpiSetColor( sys ps, remap_color( sys ps, color, true));
   if ( GpiSetPel( sys ps, &p) == GPI_ERROR) apiErr;
   GpiSetColor( sys ps, c);
   return true;
}

/* XXX palette operations */
Bool
apc_gp_stretch_image ( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   PBITMAPINFO2 b;
   POINTL pt [4];
   HPS ps = sys ps;
   PIcon i = ( PIcon) image;
   Color saveFore = 0, saveBack = 0;
   Handle deja = enscreen( image);
   Bool useSave = 0;
   Bool db = ( dsys( image) options & aptDeviceBitmap) ? 1 : 0;
   LONG lrop;

   if (!(b = get_binfo( deja))) return false;

   // clipping & positioning rectangles
   pt [0]. x = x;                    // dest from
   pt [0]. y = y;
   pt [1]. x = x + xDestLen - 1;     // dest to
   pt [1]. y = y + yDestLen - 1;
   pt [2]. x = xFrom;                // src from
   pt [2]. y = yFrom;
   pt [3]. x = xFrom + xLen;         // src to
   pt [3]. y = yFrom + yLen;

   if ( kind_of( deja, CIcon)) {
      BInfo2 mono = {
        sizeof ( BInfo),
        i-> w,
        i-> h,
        1, 1,
        {{ 0xFF, 0xFF, 0xFF, 0}, { 0, 0, 0, 0}}
      };
      useSave = 1;
      saveFore = GpiQueryColor( ps);
      saveBack = GpiQueryBackColor( ps);
      GpiSetColor( ps, guts. monoBitsOk ? CLR_WHITE : CLR_BLACK);
      GpiSetBackColor( ps, guts. monoBitsOk ? CLR_BLACK : CLR_WHITE);
      if ( GpiDrawBits( ps, i-> mask, ( PBITMAPINFO2) &mono, 4, pt, ROP_SRCAND, BBO_IGNORE) == GPI_ERROR) apiErr;
      lrop = ROP_SRCINVERT;
   } else
      lrop = ctx_remap( rop, ctx_rop2ROP, true);

   if ( b-> cBitCount == imMono) {
      if ( db) {
         useSave = 1;
         saveFore = GpiQueryColor( ps);
         saveBack = GpiQueryBackColor( ps);
         GpiSetColor( ps, saveBack);
         GpiSetBackColor( ps, saveFore);
      } else {
         long cr[ 2];
         cr[0] = remap_color( ps, ARGB( i-> palette[ 1]. r, i-> palette[ 1]. g, i-> palette[ 1]. b), true);
         cr[1] = remap_color( ps, ARGB( i-> palette[ 0]. r, i-> palette[ 0]. g, i-> palette[ 0]. b), true);
         if ( cr[1] == clBlack)
         {
            long tmp = cr[0];
            cr[0] = cr[1];
            cr[1] = tmp;
         }
         useSave = 1;
         saveFore = GpiQueryColor( ps);
         saveBack = GpiQueryBackColor( ps);
         GpiSetColor( ps, cr[1]);
         GpiSetBackColor( ps, cr[0]);
      }
   }
   if ( db) {
      HBITMAP bmSave = GpiSetBitmap( dsys( image) ps, nilHandle); // avoid PMERR_BITMAP_IN_USE
      if ( GpiWCBitBlt( ps, dsys( image) bm, 4, pt, lrop, BBO_IGNORE) == GPI_ERROR) apiErr;
      GpiSetBitmap( dsys( image) ps, bmSave);
   } else {
      if ( is_apt( aptCompatiblePS)) {
         image_set_cache( deja, image);
         if ( dsys( image) bmRaw)
            if ( GpiDrawBits( ps, dsys( image) bmRaw, dsys( image) bmInfo, 4, pt, lrop, BBO_IGNORE) == GPI_ERROR) apiErr;
      } else {
         HBITMAP bm = ( dsys( image) bm == nilHandle) ? (( HBITMAP) bitmap_make_handle( deja)) : ( dsys( image) bm);
         if ( bm)
             if ( GpiWCBitBlt( sys ps, bm, 4, pt, lrop, BBO_IGNORE) == GPI_ERROR) apiErr;
         if ( bm && bm != dsys( image) bm) GpiDeleteBitmap( bm);
      }
   }

   if ( useSave)
   {
      GpiSetColor( ps, saveFore);
      GpiSetBackColor( ps, saveBack);
   }

   free( b);
   if ( deja != image) Object_destroy( deja);

   return true;
}


Bool                                                        /* no fix */
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, Bool utf8)
{
   POINTL p = {x, y};
   POINTL pt[ TXTBOX_COUNT];
   Bool   opaque = is_apt( aptTextOpaque);

   if ( !is_apt( aptTextOutBaseline)) {
      p. y += var font. descent;
      y += var font. descent;
   }
   if ( len > 512) apc_gp_move( sys ps, 0, 0);
   while ( len > 0)
   {
      int drawLen = ( len > 512) ? 512 : len;
      if ( opaque || len > 0)
         if ( GpiQueryTextBox( sys ps, drawLen, ( char *) text, TXTBOX_COUNT, pt) == GPI_ERROR) apiErr;
      if ( opaque) {
         LONG c = GpiQueryColor( sys ps);
         int i;
         POINTL z = pt[2];
         pt[2] = pt[3];
         pt[3] = z;
         for ( i = 0; i < 5; i++) { pt[i].x+=p. x; pt[i].y+=p. y;}
         pt[1]. y = pt[2]. y = p. y - var font. descent;
         pt[0]. y = pt[3]. y = pt[1]. y + var font. height;
         GpiSetColor( sys ps, GpiQueryBackColor( sys ps));
         apc_gp_fill_poly( self, 4, ( Point*) pt);
         GpiSetColor( sys ps, c);
      }
      if ( GpiCharStringAt ( sys ps, &p, drawLen, text) == GPI_ERROR) apiErr;
      len -= 512;
      if ( len <= 0) return true;
      p. x += pt[ TXTBOX_CONCAT]. x;
      text += 512;
   }
   return true;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
   return sys ps ? &sys fillPattern : &sys fillPattern2;
}

/* XXX check for correct results !!! */
PFontABC
apc_gp_get_font_abc( Handle self, int first, int last, Bool unicode)
{
   int i, x = last - first + 1;
   LONG f2[ 256];
   PFontABC f1;
   FONTMETRICS fm;
   HPS ps = sys ps;

   if ( x > 256 || x <= 0) return nil;

   if ( !GpiQueryFontMetrics ( ps, sizeof( fm), &fm)) {
      apiErr;
      return nil;
   }

   f1 = malloc( x * sizeof( FontABC));
   if ( !f1) return nil;

   if (( fm. fsDefn & FM_DEFN_OUTLINE) == 0) {
      if ( !GpiQueryWidthTable( ps, first, x, f2)) apiErr;
      for ( i = 0; i < x; i++) {
         f1[i].a = f1[i].c = 0;
         f1[i].b = f2[i];
      }
      return f1;
   }


   {
      SIZEF szOld, szNew = {MAKEFIXED(fm. sXDeviceRes,0), MAKEFIXED(fm. sYDeviceRes,0)};
      float fOld,  fNew  = fixed2float( szNew. cx);
      double ratio;
      POINTL pt[ TXTBOX_COUNT];

      if ( !GpiQueryCharBox( ps, &szOld)) apiErr;
      fOld = fixed2float( szOld. cx);
      if ( !GpiSetCharBox( ps, &szNew)) apiErr;
      apc_gp_move( ps, 0, 0);
      if ( !GpiQueryWidthTable( ps, first, x, f2)) apiErr;
      if ( !GpiQueryTextBox( ps, 1, "W", TXTBOX_COUNT, pt)) apiErr;
      if ( !GpiSetCharBox( ps, &szOld)) apiErr;

      ratio = fOld / fNew;
      for ( i = 0; i < x; i++) {
         f1[ i]. a = pt[ TXTBOX_BOTTOMLEFT]. x * ratio;
         f1[ i]. b = ratio * f2[i];
         f1[ i]. c = 0;
      }
   }
   return f1;
}

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{
   FONTMETRICS fm;
   unsigned long * ret = malloc( sizeof( unsigned long) * 2);
   if ( !ret) return nil;
   if ( !GpiQueryFontMetrics( sys ps, sizeof( fm), &fm)) {
       free( ret);
       apiErr;
       return nil;
   }
   *count = 2;
   ret[0] = fm. sFirstChar;
   ret[1] = fm. sLastChar;
   if ( ret[0] > 255) ret[0] = 255;
   if ( ret[1] > 255) ret[1] = 255;
   return ret;
}

Color
apc_gp_get_back_color( Handle self)
{
   if ( !sys ps) return sys lbs[1];
   return remap_color( sys ps, GpiQueryBackColor( sys ps), false);
}

int
apc_gp_get_bpp( Handle self)
{
   return sys bpp;
}

Color
apc_gp_get_color( Handle self)
{
   if ( !sys ps) return sys lbs[0];
   return remap_color( sys ps, GpiQueryColor( sys ps), false);
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   Rect r = {0,0,0,0};
   if ( !is_opt( optInDraw)) {
      Point size = var self-> get_size( self);
      r. right = size. x;
      r. top   = size. y;
      return r;
   }
   if ( GpiQueryClipBox( sys ps, ( PRECTL)&r) == RGN_ERROR) apiErr;
   return r;
}

static int ctx_le2LINEEND [] = {
   leFlat      ,  LINEEND_FLAT    ,
   leSquare    ,  LINEEND_SQUARE  ,
   leRound     ,  LINEEND_ROUND   ,
   endCtx
};

Bool
apc_gp_get_fill_winding ( Handle self)
{
   return sys fillWinding;
}

int
apc_gp_get_line_end ( Handle self)
{
   if ( !sys ps) return sys lineEnd;
   return ctx_remap_def( GpiQueryLineEnd( sys ps), ctx_le2LINEEND, false, leRound);
}

static int ctx_lj2LINEJOIN [] = {
   ljRound     ,  LINEJOIN_ROUND  ,
   ljBevel     ,  LINEJOIN_BEVEL  ,
   ljMiter     ,  LINEJOIN_MITRE  ,
   endCtx
};

int
apc_gp_get_line_join ( Handle self)
{
   if ( !sys ps) return sys lineJoin;
   return ctx_remap_def( GpiQueryLineJoin( sys ps), ctx_lj2LINEJOIN, false, ljRound);
}

int
apc_gp_get_line_width( Handle self)
{
   return sys lineWidth;
}

int
apc_gp_get_line_pattern( Handle self, unsigned char * buffer)
{
   objCheck 0;
   strcpy( buffer, ( sys linePatternLen > 3) ? sys linePattern : ( unsigned char*)&sys linePattern);
   return sys linePatternLen;
}

/* XXX palette! */
Color
apc_gp_get_nearest_color( Handle self, Color color)
{
   HPS ps = sys ps;
   return remap_color( ps, GpiQueryNearestColor( ps, 0, remap_color( ps, color, true)), false);
}

/* XXX palette! */
PRGBColor
apc_gp_get_physical_palette( Handle self, int * color)
{
   HPAL pal;
   LONG count;
   PRGBColor ret;
   PULONG buf;
   int i;

   *color = 0;
   pal = GpiQueryPalette( sys ps);
   if ( !pal) return nil;
   count = GpiQueryPaletteInfo( pal, sys ps, 0, 0, 0, nil);
   if ( count == PAL_ERROR) { apiErr; return nil; };
   if ( count == 0) return nil;
   if ( !( buf = ( PULONG) malloc( sizeof( LONG) * count))) return nil;
   if ( !( ret = ( PRGBColor) malloc( sizeof( RGBColor) * count))) {
      free( buf);
      return nil;
   }
   if ( GpiQueryPaletteInfo( pal, sys ps, 0, 0, count, buf) == PAL_ERROR) {
      apiErr;
      free( buf);
      free( ret);
      return nil;
   }
   *color = count;
   for ( i = 0; i < count; i++) {
      ret[ i]. b = buf[ i] & 0xFF;
      ret[ i]. g = ( buf[ i] >> 8)  & 0xFF;
      ret[ i]. r = ( buf[ i] >> 16) & 0xFF;
   }
   free( buf);
   return ret;
}

/* XXX */
Bool
apc_gp_get_region( Handle self, Handle mask)
{
   HRGN rgn, orgn;
   RECTL clipEx;
   POINTL offset;

   objCheck false;
   if ( !is_opt( optInDraw) || !sys ps) return false;

   rgn = GpiQueryClipRegion( sys ps);
   if ( !rgn || rgn == HRGN_ERROR)          // error or just no region
      return false;
   if ( !mask)
      return true;

   GpiQueryClipBox( sys ps, &clipEx);

   GpiSetClipRegion( sys ps, nilHandle, &orgn);

   offset = ( POINTL) { sys transform2. x, sys transform2. y};
   GpiOffsetRegion( sys ps, rgn, &offset);
   CImage( mask)-> create_empty( mask, clipEx. xRight, clipEx. yTop, imBW);
   CImage( mask)-> begin_paint( mask);
   CImage( mask)-> set_backColor( mask, 0);
   CImage( mask)-> clear( mask, 0, 0, clipEx. xRight, clipEx. yTop);
   CImage( mask)-> set_color( mask, 0xFFFFFF);
   CImage( mask)-> set_backColor( mask, 0xFFFFFF);
   if ( GpiPaintRegion( dsys( mask) ps, rgn) == GPI_ERROR) apiErr;
   offset = ( POINTL) { -sys transform2. x, -sys transform2. y};
   GpiOffsetRegion( sys ps, rgn, &offset);

   CImage( mask)-> end_paint( mask);

   GpiSetClipRegion( sys ps, rgn, &orgn);

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

int
apc_gp_get_rop ( Handle self)
{
   if ( !sys ps) return sys rop;
   return ctx_remap( GpiQueryMix( sys ps), ctx_rop2FM, false);
}

int
apc_gp_get_rop2 ( Handle self)
{
   if ( !sys ps) return sys rop2;
   return ctx_remap( GpiQueryBackMix( sys ps), ctx_rop2BM, false);
}

Bool
apc_gp_get_text_out_baseline( Handle self)
{
   objCheck 0;
   return is_apt( aptTextOutBaseline);
}

/* XXX - check correct results ! */
int
apc_gp_get_text_width ( Handle self, const char* text, int len, Bool addOverhang, Bool utf8 )
{
   POINTL pt[ TXTBOX_COUNT];
   HPS ps = sys ps;
   apc_gp_move( sys ps, 0, 0);
   if ( var font. direction != 0) {
      GRADIENTL g = {1,0};
      if ( !GpiSetCharAngle( ps, &g)) apiErr;
   }
   if ( !GpiQueryTextBox( ps, len, ( char*)text, TXTBOX_COUNT, pt)) apiErr;
   if ( var font. direction != 0) {
      GRADIENTL g = ( GRADIENTL) { ( long) ( cos( var font. direction / GRAD) * 10000) , ( long) ( sin( var font. direction / GRAD) * 10000)};
      if ( !GpiSetCharAngle( ps, &g)) apiErr;
   }
   return pt[ TXTBOX_CONCAT]. x + ( addOverhang ? pt[ TXTBOX_BOTTOMLEFT]. x : 0);
}

/* XXX - check correct results ! */
Point *
apc_gp_get_text_box( Handle self, const char* text, int len, Bool utf8)
{
   POINTL * pt = malloc( sizeof( POINTL) * TXTBOX_COUNT);
   if ( !pt) return nil;
   apc_gp_move( sys ps, 0, 0);
   if ( !GpiQueryTextBox( sys ps, len, (char*) text, TXTBOX_COUNT, pt)) apiErr;
   if ( !is_apt( aptTextOutBaseline)) {
      int i = 4, d = var font. descent;
      while ( i--) pt[ i]. y += d;
   }
   return ( Point *) pt;
}

Bool
apc_gp_get_text_opaque( Handle self)
{
   return is_apt( aptTextOpaque);
}

Point
apc_gp_get_transform( Handle self)
{
   MATRIXLF m = {0,0,0,0,0,0,0,0};
   if ( !sys ps) return sys transform;
   if ( !GpiQueryDefaultViewMatrix( sys ps, 8, &m)) apiErr;
   return ( Point) { m. lM31 + sys transform2. x, m. lM32 + sys transform2. y};
}

Bool
apc_gp_set_back_color( Handle self, Color color)
{
   LONG clr = remap_color( sys ps, color, true);
   if ( !sys ps) sys lbs[1] = color; else
   if ( !GpiSetBackColor( sys ps, clr)) apiErrRet;
   return true;
}

Bool
apc_gp_set_clip_rect( Handle self, Rect clipRect)
{
   HRGN  rgn, old;
   if ( !is_opt( optInDraw) || !sys ps) return false;
   clipRect. left   -= sys transform2. x;
   clipRect. right  -= sys transform2. x - 1;
   clipRect. top    -= sys transform2. y - 1;
   clipRect. bottom -= sys transform2. y;
   if ( !( rgn = GpiCreateRegion( sys ps, 1, ( PRECTL) &clipRect))) apiErr;
   if ( GpiSetClipRegion( sys ps, rgn, &old) == RGN_ERROR) apiErr;
   if ( old) if ( !GpiDestroyRegion( sys ps, old)) apiErr;
   return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
   LONG clr = remap_color( sys ps, color, true);
   if ( !sys ps) sys lbs[0] = color; else
   if ( !GpiSetColor( sys ps, clr)) apiErrRet;
   return true;
}


Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   int i, coop;
   ULONG pat = PATSYM_ERROR;
   if ( !sys ps)
   {
      memcpy( &sys fillPattern2, pattern, sizeof( FillPattern));
      return true;
   }
   memcpy( &sys fillPattern, pattern, sizeof( FillPattern));
   if ( !GpiSetPatternSet( sys ps, 0)) apiErr;

   if ( pattern[0] == 0) {
      coop = 0;
      for ( i = 1; i < 8; i++) coop |= pattern[i];
      if ( coop == 0) pat = PATSYM_BLANK;
   } else if ( pattern[0] == 0xFF) {
      coop = 0xFF;
      for ( i = 1; i < 8; i++) coop &= pattern[i];
      if ( coop == 0xFF) pat = PATSYM_SOLID;
   }

   if ( pat != PATSYM_ERROR)
      GpiSetPattern( sys ps, pat);
   else
   {
      BInfo2 bm = { sizeof( BInfo), 8, 8, 1, 1, {{ 0, 0, 0, 0}, { 0xFF, 0xFF, 0xFF, 0}}};
      HBITMAP b;
      ULONG core [8];
      int i;
      for ( i = 0; i < 8; i++) core[ i] = pattern[ i];
      if ( sys fillBitmap) {
         GpiSetPatternSet( sys ps, LCID_DEFAULT);
         GpiDeleteSetId( sys ps, 3);
         GpiDeleteBitmap( sys fillBitmap);
         sys fillBitmap = nilHandle;
      }
      b = GpiCreateBitmap( sys ps, ( PBITMAPINFOHEADER2) &bm, CBM_INIT, (PBYTE) &core, ( PBITMAPINFO2) &bm);
      if ( b != nilHandle) {
         GpiSetBitmapId( sys ps, b, 3);
         GpiSetPatternSet( sys ps, 3);
         sys fillBitmap = b;
      } else apiErrRet;
   }
   return true;
}

static void
gp_set_font_extra( Handle self, HPS ps, int fontId, PSIZEF sz, Bool vectored, PFont font)
{
   if ( !GpiSetCharSet( ps, fontId)) apiErr;
   if ( vectored) {
      GRADIENTL g;
      if ( !GpiSetCharBox( ps, sz)) apiErr;
      if ( font-> direction != 0)
         g = ( GRADIENTL) { ( long) ( cos( font-> direction / GRAD) * 10000) , ( long) ( sin( font-> direction / GRAD) * 10000)};
      else
         g = ( GRADIENTL) { 1, 0};
      if ( !GpiSetCharAngle( ps, &g)) apiErr;
   }
}

/* XXX check very carefully!!! */
Bool
apc_gp_set_font( Handle self, PFont font)
{
   // very simple & uneffective gpi font management;
   // does not caches fonts, uses only one local ID = 1.
   USHORT cp = font_enc2cp( font-> encoding);
   FATTRS f = {
      sizeof ( FATTRS),
      0,
      0,                   // does not force match
      "",                  // facename
      0,                   // default registry
      ( cp == 65535) ? guts. codePage : cp, // the selected or the default codepage
      0,                   // height
      0,                   // width
      0,
      FATTR_FONTUSE_TRANSFORMABLE
   };
   int vectored;
   SIZEF sz;
   int fontId;

   if ( !sys ps) return true;

   apcErrClear;
   if ( sys fontHash)
   {
      int fontId;
      font-> resolution = sys res.y * 0x10000 + sys res. x;
      /* XXX check whether hash data are relevant */
      fontId = get_fontid_from_hash( sys fontHash, font, &sz, &vectored);
      if ( fontId)
      {
         gp_set_font_extra( self, sys ps, fontId, &sz, vectored, font);
         return true;
      }
   }

   vectored = font_font2gp( font, sys res, false);
   if ( apcError != 0) return false;

   if ( vectored) {
      sz = ( SIZEF)
      {
         MAKEFIXED( font-> width,  0),
         MAKEFIXED( font-> height, 0)
      };
   } else {
      f. lAveCharWidth   = font-> width;
      f. lMaxBaselineExt = font-> height;
      f. fsFontUse = FATTR_FONTUSE_NOMIX;
   }
   if ( font-> style & fsItalic    ) f. fsSelection |= FATTR_SEL_ITALIC;
   if ( font-> style & fsUnderlined) f. fsSelection |= FATTR_SEL_UNDERSCORE;
   if ( font-> style & fsStruckOut ) f. fsSelection |= FATTR_SEL_STRIKEOUT;
   if ( font-> style & fsBold      ) f. fsSelection |= FATTR_SEL_BOLD;
   if ( font-> style & fsOutline   ) f. fsSelection |= FATTR_SEL_OUTLINE;
   memcpy( f. szFacename, font-> name, FACESIZE);
   if ( sys fontHash) {
      (sys fontId)++;
      if ( sys fontId > 254) sys fontId = 1;
      fontId = sys fontId;
   } else
      fontId = 1;

   if ( !GpiDeleteSetId( sys ps, fontId)) {
      rc = WinGetLastError( guts. anchor);
      if ( LOUSHORT(rc) != 0x2103 /*PMERR_SETID_NOT_FOUND*/) apiAltErr( rc);
   }
   if ( GpiCreateLogFont( sys ps, nil, fontId, &f) == GPI_ERROR) apiErr;

   gp_set_font_extra( self, sys ps, fontId, &sz, vectored, font);

   if ( sys fontHash) {
      font-> resolution = sys res. y * 0x10000 + sys res. x;
      add_fontid_to_hash( sys fontHash, sys fontId, font, &sz, vectored);
   }
   return true;
}

Bool
apc_gp_set_fill_winding( Handle self, Bool fillWinding)
{
   sys fillWinding = fillWinding;
   return true;
}

Bool
apc_gp_set_line_end ( Handle self, int lineEnd)
{
   if ( !sys ps) { sys lineEnd = lineEnd; return true; }
   if ( !GpiSetLineEnd( sys ps, ctx_remap_def( lineEnd, ctx_le2LINEEND, true, LINEEND_DEFAULT)))
      apiErrRet;
   return true;
}

Bool
apc_gp_set_line_join( Handle self, int lineJoin)
{
   if ( !sys ps) { sys lineJoin = lineJoin; return true; }
   if ( !GpiSetLineJoin( sys ps, ctx_remap_def( lineJoin, ctx_lj2LINEJOIN, true, LINEJOIN_DEFAULT)))
      apiErrRet;
   return true;
}

Bool
apc_gp_set_line_width( Handle self, int lineWidth)
{
   sys lineWidth = lineWidth;
   if ( !sys ps) return true;
   if ( lineWidth == 0) {
      if ( !GpiSetLineWidthGeom( sys ps, 1)) apiErrRet;
      if ( !GpiSetLineWidth( sys ps, LINEWIDTH_NORMAL)) apiErrRet;
   } else {
      if ( !GpiSetLineWidthGeom( sys ps, lineWidth)) apiErrRet;
   }
   return true;
}

Bool
apc_gp_set_line_pattern( Handle self, unsigned char * pattern, int len)
{
   objCheck false;
   if ( sys linePatternLen > 3)
      free( sys linePattern);
   if ( len > 3) {
      if ( !( sys linePattern = malloc( len))) {
         sys linePatternLen = 0;
         return false;
      }
      memcpy( sys linePattern, pattern, len);
   } else
      memcpy( &(sys linePattern), pattern, len);
   sys linePatternLen = len;

   if ( sys ps) {
      LONG pat;
      switch ( len) {
      case 0:
         pat = LINETYPE_INVISIBLE;
         break;
      case 1:
         pat = pattern[0] ? LINETYPE_SOLID : LINETYPE_INVISIBLE;
         break;
      case 2:
         if (( memcmp( pattern, lpDotDot, 2) == 0) || ( memcmp( pattern, lpDot, 2) == 0)) {
             pat = LINETYPE_DOT;
             break;
         } else
         if (( memcmp( pattern, psDash, 2) == 0) || ( memcmp( pattern, lpDash, 2) == 0)) {
             pat = LINETYPE_LONGDASH;
             break;
         }
         pat = LINETYPE_SHORTDASH;
         break;
      case 4:
         pat = LINETYPE_DASHDOT;
         break;
      case 5:
      case 6:
         pat = LINETYPE_DASHDOUBLEDOT;
         break;
      default:
         pat = LINETYPE_SOLID;
      }
      if ( !GpiSetLineType( sys ps, pat)) apiErrRet;
   }
   return true;
}


/* XXX */
Bool
apc_gp_set_palette( Handle self)
{
   return false;
}

/* XXX */
Bool
apc_gp_set_region( Handle self, Handle mask)
{
   HRGN rgn = nilHandle;
   HRGN oldrgn;
   POINTL offset;
   objCheck false;

   if ( !is_opt( optInDraw) || !sys ps) return true;

   rgn = region_create( self, mask);
   if ( !rgn) {
      if ( GpiSetClipRegion( sys ps, nilHandle, &oldrgn) == HRGN_ERROR) apiErrRet;
      if ( oldrgn) GpiDestroyRegion( sys ps, oldrgn);
      return true;
   }
   offset = ( POINTL) { -sys transform2. x, -sys transform2. y };
   GpiOffsetRegion( sys ps, rgn, &offset);
   if ( GpiSetClipRegion( sys ps, rgn, &oldrgn) == HRGN_ERROR) apiErrRet;
   if ( oldrgn) GpiDestroyRegion( sys ps, oldrgn);
   return true;
}

Bool
apc_gp_set_rop ( Handle self, int rop)
{
   if ( !sys ps) { sys rop = rop; return true; }
   if ( !GpiSetMix( sys ps, ctx_remap( rop, ctx_rop2FM, true))) apiErrRet;
   return true;
}

Bool
apc_gp_set_rop2 ( Handle self, int rop)
{
   if ( !sys ps) { sys rop2 = rop; return true; }
   if ( !GpiSetBackMix( sys ps, ctx_remap( rop, ctx_rop2BM, true))) apiErrRet;
   return true;
}

Bool
apc_gp_set_transform( Handle self, int x, int y)
{
   MATRIXLF m = { MAKEFIXED( 1, 0), 0, 0, 0, MAKEFIXED( 1, 0), 0,
      x - sys transform2. x, y - sys transform2. y};
   if ( !sys ps)
   {
      sys transform = ( Point){ x, y};
      return true;
   }
   if ( !GpiSetDefaultViewMatrix( sys ps, 8, &m, TRANSFORM_REPLACE)) apiErrRet;
   return true;
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
   apt_assign( aptTextOpaque, opaque);
   return true;
}

Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
   apt_assign( aptTextOutBaseline, baseline);
   return true;
}


