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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "Window.h"
#include "img_api.h"
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

Bool image_screenable( Handle image, Handle screen, int * bitCount)
{
   PImage i = ( PImage) image;
   if (( i-> type & ( imRealNumber | imComplexNumber | imTrigComplexNumber)) ||
       ( i-> type == imLong || i-> type == imShort)) {
      if ( bitCount) *bitCount = 8;
      return false;
   }

   if ( i-> type == imRGB) {

      if ( !screen)
         return true;

      if ( dsys( screen) options. aptCompatiblePS || !dsys( screen) ps) {
         int bpp = guts. displayBMInfo. bmiHeader. biBitCount *
                   guts. displayBMInfo. bmiHeader. biPlanes;
         if ( bpp) {
            if ( bitCount) {
               *bitCount = bpp;
               if ( *bitCount < 4) *bitCount = 1;
               else if ( *bitCount < 8) *bitCount = 4;
               else return true;
            }
            return false;
         }
      } else {
         if ( dsys( screen) bpp == 0) {
            if ( !dsys(screen) ps) {
               *bitCount = 1;
               return false;
            }
            dsys( screen) bpp = GetDeviceCaps( dsys(screen) ps, BITSPIXEL);
         }
         if ( dsys( screen) bpp <= 8) {
            *bitCount = dsys( screen) bpp;
            if ( *bitCount < 4) *bitCount = 1;
            else if ( *bitCount < 8) *bitCount = 4;
            return false;
         }
      }
   }
   return true;
}


Handle image_enscreen( Handle image, Handle screen)
{
   PImage i = ( PImage) image;
   int lower;
   if ( !image_screenable( image, screen, &lower))
   {
      Handle j = i-> self-> dup( image);
      if ( i-> type == imRGB) {
         ((( PImage) j)-> self)->set_type( j, lower);
      } else {
         ((( PImage) j)-> self)->resample( j,
            ((( PImage) j)-> self)->stats( j, false, isRangeLo, 0),
            ((( PImage) j)-> self)->stats( j, false, isRangeHi, 0),
            0, 255
         );
         ((( PImage) j)-> self)->set_type( j, lower | imGrayScale);
      }
      return j;
   } else
      return image;
}

BITMAPINFO * image_get_binfo( Handle self, XBITMAPINFO * bi)
{
   int i;
   PImage       image = ( PImage) self;
   int          nColors;
   int          bitCount, lower;

   if ( is_apt( aptDeviceBitmap))
   {
      memcpy( bi, &guts. displayBMInfo, sizeof( BITMAPINFO));
      if ((( PDeviceBitmap) self)-> monochrome) {
         bi-> bmiHeader. biPlanes = bi-> bmiHeader. biBitCount = 1;
         bi-> bmiHeader. biClrUsed = bi-> bmiHeader. biClrImportant = 2;
      } else if ( bi-> bmiHeader. biBitCount <= 8) {
         nColors = 1 << ( bi-> bmiHeader. biBitCount * bi-> bmiHeader. biPlanes);
         if ( sys pal) {
            GetPaletteEntries( sys pal, 0, nColors, ( LPPALETTEENTRY) &bi-> bmiColors);
            bi-> bmiHeader. biClrUsed = bi-> bmiHeader. biClrImportant = nColors;
         } else
            bi-> bmiHeader. biClrUsed = bi-> bmiHeader. biClrImportant = 0;
      } else
         bi-> bmiHeader. biClrUsed = bi-> bmiHeader. biClrImportant = 0;
      return ( BITMAPINFO *) bi;
   }


   if ( image_screenable( self, nilHandle, &lower)) {
      nColors  = (( 1 << ( image-> type & imBPP)) & 0x1ff);
      bitCount = image-> type & imBPP;
   } else {
      nColors  = ( 1 << lower) & 0x1ff;
      bitCount = lower;
   }

   if ( nColors > image-> palSize) nColors = image-> palSize;
   memset( bi, 0, sizeof( BITMAPINFOHEADER) + nColors * sizeof( RGBQUAD));
   bi-> bmiHeader. biSize          = sizeof( BITMAPINFOHEADER); // - sizeof( RGBQUAD);
   bi-> bmiHeader. biWidth         = image-> w;
   bi-> bmiHeader. biHeight        = image-> h;
   bi-> bmiHeader. biPlanes        = 1;
   bi-> bmiHeader. biBitCount      = bitCount;
   bi-> bmiHeader. biCompression   = BI_RGB;
   bi-> bmiHeader. biClrUsed       = nColors;
   bi-> bmiHeader. biClrImportant  = nColors;

   for ( i = 0; i < nColors; i++)
   {
      bi-> bmiColors[ i]. rgbRed    = image-> palette[ i]. r;
      bi-> bmiColors[ i]. rgbGreen  = image-> palette[ i]. g;
      bi-> bmiColors[ i]. rgbBlue   = image-> palette[ i]. b;
   }
   return ( BITMAPINFO *) bi;
}



void
bm_put_zs( HBITMAP hbm, int x, int y, int z)
{
   HDC dc = dc_alloc();
   HDC xdc = CreateCompatibleDC( 0);
   BITMAP bitmap;
   int cx, cy;


   SelectObject( xdc, hbm);
   GetObject( hbm, sizeof( BITMAP), ( LPSTR) &bitmap);

   cx = bitmap. bmWidth;
   cy = bitmap. bmHeight;

   StretchBlt( dc, x, y, z * cx, z * cy, xdc, 0, 0, cx, cy, SRCCOPY);

   DeleteDC( xdc);
   dc_free();
}




HBITMAP
image_make_bitmap_handle( Handle img, HPALETTE pal)
{
   HBITMAP bm;
   XBITMAPINFO xbi;
   BITMAPINFO * bi = image_get_binfo( img, &xbi);
   HPALETTE old = nil, xpal = pal;
   HWND foc = GetFocus();
   HDC dc = GetDC( foc);

   if ( !dc)
      apiErr;

   if ( xpal == nil)
      xpal = image_make_bitmap_palette( img);

   if ( xpal) {
      old = SelectPalette( dc, xpal, 1);
      RealizePalette( dc);        // m$ suxx !!!
   }

   // if (((( PImage) img)-> type & imBPP) != 1)
   if ((( PImage) img)-> type != imBW)
      bm = CreateDIBitmap( dc, &bi-> bmiHeader, CBM_INIT,
        (( PImage) img)-> data, bi, DIB_RGB_COLORS);
   else {
      bm = CreateBitmap( bi-> bmiHeader. biWidth, bi-> bmiHeader. biHeight, 1, 1, NULL);
      SetDIBits( dc, bm, 0, bi-> bmiHeader. biHeight, (( PImage) img)-> data, bi, DIB_RGB_COLORS);
   }

   if ( !bm) {
      apiErr;
      if ( old) {
         SelectPalette( dc, old, 1);
         RealizePalette( dc);
      }
      if ( xpal != pal)
         DeleteObject( xpal);
      ReleaseDC( foc, dc);
      return nil;
   }

   if ( old) {
      SelectPalette( dc, old, 1);
      RealizePalette( dc);
   }

   if ( xpal != pal)
      DeleteObject( xpal);

   ReleaseDC( foc, dc);

   return bm;
}

HPALETTE
image_make_bitmap_palette( Handle img)
{
   PDrawable i    = ( PDrawable) img;
   int j, nColors = i-> palSize;
   XLOGPALETTE lp;
   HPALETTE r;
   RGBColor  dest[ 256];
   PRGBColor logp = i-> palette;

   lp. palVersion = 0x300;
   lp. palNumEntries = nColors;

   if ( nColors == 0) return nil;

   if ( !dsys(img)p256) {
      if ( nColors > 256) {
         dsys(img)p256 = ( PXLOGPALETTE) malloc( sizeof( XLOGPALETTE));
         cm_squeeze_palette( i-> palette, nColors, dest, 256);
         nColors = lp. palNumEntries = 256;
         logp = dest;
      }

      for ( j = 0; j < nColors; j++) {
         lp. palPalEntry[ j]. peRed    = logp[ j]. r;
         lp. palPalEntry[ j]. peGreen  = logp[ j]. g;
         lp. palPalEntry[ j]. peBlue   = logp[ j]. b;
         lp. palPalEntry[ j]. peFlags  = 0;
      }

      if ( dsys(img)p256) memcpy( dsys(img)p256, &lp, sizeof( XLOGPALETTE));
      if ( !( r = CreatePalette(( LOGPALETTE*) &lp))) apiErrRet;
   } else {
      if ( !( r = CreatePalette(( LOGPALETTE*) dsys(img)p256))) apiErrRet;
   }
   return r;
}

void
image_set_cache( Handle from, Handle self)
{
   if ( sys pal == nil)
      sys pal = image_make_bitmap_palette( from);
   if ( sys bm == nil) {
      sys bm  = image_make_bitmap_handle( from, sys pal);
      if ( !is_apt( aptDeviceBitmap))
         hash_store( imageMan, &self, sizeof( self), (void*)self);
   }
}

void
image_destroy_cache( Handle self)
{
   if ( sys bm) {
      if ( !DeleteObject( sys bm)) apiErr;
      hash_delete( imageMan, &self, sizeof( self), false);
      sys bm = nil;
   }
   if ( sys pal) {
      if ( !DeleteObject( sys pal)) apiErr;
      sys pal = nil;
   }
   if ( sys s. imgCachedRegion) {
      if ( !DeleteObject( sys s. imgCachedRegion)) apiErr;
      sys s. imgCachedRegion = nil;
   }
}

void
image_query_bits( Handle self, Bool forceNewImage)
{
   PImage i = ( PImage) self;
   XBITMAPINFO xbi;
   BITMAPINFO * bi;
   XBITMAPINFO xbi2;
   int  newBits;
   HDC  ops;
   BITMAP bitmap;

   if ( forceNewImage) {
      ops = sys ps;
      if ( !ops) sys ps = dc_alloc();
   }

   if ( !GetObject( sys bm, sizeof( BITMAP), ( LPSTR) &bitmap)) {
      apiErr;
      return;
      // if GetObject fails to get even BITMAP, there will be no good in farther run for sure.
   }

   if (( bitmap. bmPlanes == 1) && (
          ( bitmap. bmBitsPixel == 1) ||
          ( bitmap. bmBitsPixel == 4) ||
          ( bitmap. bmBitsPixel == 8) ||
          ( bitmap. bmBitsPixel == 24)
      ))
      newBits = bitmap. bmBitsPixel;
   else {
      newBits = ( bitmap. bmBitsPixel <= 4) ? 4 :
                (( bitmap. bmBitsPixel <= 8) ? 8 : 24);
   }


   if ( forceNewImage) {
      i-> self-> create_empty( self, bitmap. bmWidth, bitmap. bmHeight, newBits);
   } else {
      if (( newBits != ( i-> type & imBPP)) || (( i-> type & ~imBPP) != 0))
         i-> self-> create_empty( self, i-> w, i-> h, newBits);
   }

   bi = image_get_binfo( self, &xbi);

   if ( !GetDIBits( sys ps, sys bm, 0, i-> h, i-> data, bi, DIB_RGB_COLORS)) apiErr;

   if (( i-> type & imBPP) < 24) {
      int j, nColors = 1 << ( i-> type & imBPP);
      for ( j = 0; j < nColors; j++) {
         i-> palette[ j]. r = xbi. bmiColors[ j]. rgbRed;
         i-> palette[ j]. g = xbi. bmiColors[ j]. rgbGreen;
         i-> palette[ j]. b = xbi. bmiColors[ j]. rgbBlue;
      }
   }

   if ( forceNewImage) {
      if ( !ops) {
         dc_free();
      }
      sys ps = ops;
   }
}


Bool
apc_image_create( Handle self)
{
   objCheck false;
   // apt_set( aptBitmap);
   image_destroy_cache( self);
   sys lastSize. x = var w;
   sys lastSize. y = var h;
   return true;
}

Bool
apc_image_destroy( Handle self)
{
   objCheck false;
   image_destroy_cache( self);
   return true;
}

Bool
apc_image_begin_paint( Handle self)
{
   apcErrClear;
   objCheck false;
   if ( !( sys ps = CreateCompatibleDC( 0))) apiErrRet;
   if ( sys bm == nil) {
      Handle deja  = image_enscreen( self, self);
      image_set_cache( deja, self);
      if ( deja != self) Object_destroy( deja);
   }
   sys stockBM = SelectObject( sys ps, sys bm);
   if ( !sys pal)
      sys pal = image_make_bitmap_palette( self);
   hwnd_enter_paint( self);
   if (( PImage( self)-> type & imBPP) == imbpp1) sys bpp = 1;
   if ( sys pal) {
      SelectPalette( sys ps, sys pal, 0);
      RealizePalette( sys ps);
   }
   return true;
}

Bool
apc_image_begin_paint_info( Handle self)
{
   apcErrClear;
   objCheck false;
   if ( !( sys ps = CreateCompatibleDC( 0))) apiErrRet;
   if ( !sys pal) sys pal = image_make_bitmap_palette( self);
   if ( sys pal) SelectPalette( sys ps, sys pal, 0);
   hwnd_enter_paint( self);
   if (( PImage( self)-> type & imBPP) == imbpp1) sys bpp = 1;
   return true;
}


Bool
apc_image_end_paint( Handle self)
{
   BITMAPINFO * bi;
   apcErrClear;
   objCheck false;

   image_query_bits( self, false);
   hwnd_leave_paint( self);
   if ( sys stockBM)
      SelectObject( sys ps, sys stockBM);
   DeleteDC( sys ps);
   sys stockBM = sys ps = nil;
   return apcError == errOk;
}

Bool
apc_image_end_paint_info( Handle self)
{
   objCheck false;
   apcErrClear;
   hwnd_leave_paint( self);
   DeleteDC( sys ps);
   sys ps = nil;
   return apcError == errOk;
}


Bool
apc_image_update_change( Handle self)
{
   objCheck false;
   image_destroy_cache( self);
   sys lastSize. x = var w;
   sys lastSize. y = var h;
   return true;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
   HDC dc;
   Bool palc = 0;

   objCheck false;
   apcErrClear;
   apt_set( aptBitmap);
   apt_set( aptDeviceBitmap);
   apt_set( aptCompatiblePS);

   if ( !( sys ps = CreateCompatibleDC( 0))) apiErrRet;
   sys lastSize. x = var w;
   sys lastSize. y = var h;

   if ( monochrome)
      sys bm = CreateBitmap( var w, var h, 1, 1, nil);
   else {
      if ( sys pal = palette_create( self)) {
         sys stockPalette = SelectPalette( sys ps, sys pal, 1);
         RealizePalette( sys ps);
         palc = 1;
      }
      sys bm = CreateCompatibleBitmap( dc_alloc(), var w, var h);
      if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
         apt_clear( aptCompatiblePS);
   }

   if ( !sys bm) {
      apiErr;
      if ( !monochrome) dc_free();
      if ( palc) {
         SelectPalette( sys ps, sys stockPalette, 1);
         DeleteObject( sys stockPalette);
         sys stockPalette = nil;
      }
      DeleteDC( sys ps);
      return false;
   }
   if ( !monochrome) dc_free();

   sys stockBM = SelectObject( sys ps, sys bm);

   hwnd_enter_paint( self);
   if ( monochrome) sys bpp = 1;

   hash_store( imageMan, &self, sizeof( self), (void*)self);
   return true;
}

void
dbm_recreate( Handle self)
{
   HBITMAP bm, stock;
   HDC dc, dca;
   HPALETTE p, std = nil;
   if ((( PDeviceBitmap) self)-> monochrome) return;

   if ( !( dc = CreateCompatibleDC( 0))) {
      apiErr;
      return;
   }
   dca = dc_alloc();

   if ( sys pal) {
      p = SelectPalette( dc, sys pal, 1);
      RealizePalette( dc);
   }

   if ( !( bm = CreateCompatibleBitmap( dca, var w, var h))) {
      DeleteDC( dc);
      dc_free();
      apiErr;
      return;
   }
   stock = SelectObject( dc, bm);

   BitBlt( dc, 0, 0, var w, var h, sys ps, 0, 0, SRCCOPY);

   if ( sys pal) {
      SelectPalette( sys ps, sys stockPalette, 1);
      sys stockPalette = p;
   } else
      sys stockPalette = GetCurrentObject( dc, OBJ_PAL);

   if ( sys stockBM)
      SelectObject( sys ps, sys stockBM);
   DeleteObject( sys bm);
   DeleteDC( sys ps);

   sys ps = dc;
   sys bm = bm;
   sys stockBM = stock;
   dc_free();
}

Bool
apc_dbm_destroy( Handle self)
{
   apcErrClear;
   hash_delete( imageMan, &self, sizeof( self), false);
   objCheck false;

   hwnd_leave_paint( self);

   if ( sys pal)
      DeleteObject( sys pal);
   if ( sys stockBM)
     SelectObject( sys ps, sys stockBM);
   DeleteObject( sys bm);
   DeleteDC( sys ps);
   sys pal = sys stockBM = sys ps = sys bm = nil;
   return true;
}

HICON
image_make_icon_handle( Handle img, Point size, Point * hotSpot, Bool forPointer)
{
   PIcon i = ( PIcon) img;
   HICON    r;
   ICONINFO ii;
   int   j, bpp = i-> type & imBPP;
   Bool  noSZ   = i-> w != size. x || i-> h != size. y;
   Bool  noBPP  = bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24;
   HDC dc;
   XBITMAPINFO bi;
   Bool notAnIcon = !kind_of( img, CIcon);

   ii. fIcon = hotSpot ? false : true;
   ii. xHotspot = hotSpot ? hotSpot-> x : 0;
   ii. yHotspot = hotSpot ? hotSpot-> y : 0;

   if ( noSZ || noBPP)
      i = ( PIcon)( i-> self-> dup( img));

   if ( IS_WIN95 && forPointer && ( guts. displayBMInfo. bmiHeader. biBitCount <= 4)) {
      i-> self-> set_conversion(( Handle) i, ictNone);
   } else
      forPointer = false;

   if ( noSZ || noBPP) {
      if ( noSZ)
         i-> self-> set_size(( Handle) i, size);
      if ( noBPP)
         i-> self-> set_type(( Handle) i,
             ( bpp < 4) ? 1 :
             (( bpp < 8) ? 4 :
             (( bpp < 24) ? 8 : 24))
      );
   }

   dc = dc_alloc();
   image_get_binfo(( Handle)i, &bi);

  // if ( 0) {
   if ( IS_NT) {
      
      if ( !( ii. hbmColor = CreateDIBitmap( dc, &bi. bmiHeader, CBM_INIT,
          i-> data, ( BITMAPINFO*) &bi, DIB_RGB_COLORS))) apiErr;
      bi. bmiHeader. biBitCount = bi. bmiHeader. biPlanes = 1;
      bi. bmiColors[ 0]. rgbRed = bi. bmiColors[ 0]. rgbGreen = bi. bmiColors[ 0]. rgbBlue = 0;
      bi. bmiColors[ 1]. rgbRed = bi. bmiColors[ 1]. rgbGreen = bi. bmiColors[ 1]. rgbBlue = 255;

      if ( !( ii. hbmMask  = CreateDIBitmap( dc, &bi. bmiHeader, CBM_INIT,
         notAnIcon ? NULL : i-> mask, ( BITMAPINFO*) &bi, DIB_RGB_COLORS))) apiErr;
   } else {
// Moronious and "macaronious" code for Win95 -
// since CreateIconIndirect gives results so weird,
// we use following sequence.
      int mSize = i-> maskSize / i-> h;
      Byte *data = ( Byte*)malloc( mSize), *b1 = i-> mask, *b2 = i-> mask + mSize*(i-> h - 1);
      Byte * mask;

// reverting bits vertically - it's not HBITMAP, just bare bits
      while ( b1 < b2) {
         memcpy( data, b1,   mSize);
         memcpy( b1,   b2,   mSize);
         memcpy( b2,   data, mSize);
         b1 += mSize;
         b2 -= mSize;
      }
      free( data);

      if ( notAnIcon)      
         mask = i-> mask;
      else {
         int sz = (( i-> w + 31) / 32) * 4; 
         mask = ( Byte*)malloc( sz);
         memset( mask, 0xff, sz);
      }   
// creating icon by old 3.1 method - we need that for correct AND-mask,
// don't care other pointer properties
      r = forPointer ?
         CreateCursor( guts. instance, ii. xHotspot, ii. yHotspot,
            size.x, size.y, mask, i-> data) :
         CreateIcon( guts. instance, size.x, size.y, 1, i-> type & imBPP,
            mask, i-> data);
      if ( notAnIcon) free( mask);
      if ( !r) {
         if (( Handle) i != img) Object_destroy(( Handle) i);
         apiErrRet;
      }
      GetIconInfo( r, &ii);
      DeleteObject( ii. hbmColor);

      if ( !( ii. hbmColor = CreateDIBitmap( dc, &bi. bmiHeader, CBM_INIT,
          i-> data, ( BITMAPINFO*) &bi, DIB_RGB_COLORS))) apiErr;
      DestroyIcon( r);
   }
   dc_free();

   if ( !( r = CreateIconIndirect( &ii))) apiErr;

   DeleteObject( ii. hbmColor);
   DeleteObject( ii. hbmMask);
   if (( Handle) i != img) Object_destroy(( Handle) i);
   return r;
}

ApiHandle
apc_image_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}

ApiHandle
apc_dbm_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}

ApiHandle
apc_prn_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}


Bool
apc_prn_create( Handle self) {
   objCheck false;
//   sys lastSize. x = var w;
//   sys lastSize. y = var h;
   return true;
}

static void ppi_create( LPPRINTER_INFO_2 dest, LPPRINTER_INFO_2 source)
{
#define SZCPY(field) if ( source-> field) strcpy( dest-> field = ( char*)malloc( strlen( source-> field) + 1), source-> field)
   memcpy( dest, source, sizeof( PRINTER_INFO_2));
   SZCPY( pPrinterName);
   SZCPY( pServerName);
   SZCPY( pShareName);
   SZCPY( pPortName);
   SZCPY( pDriverName);
   SZCPY( pComment);
   SZCPY( pLocation);
   SZCPY( pSepFile);
   SZCPY( pPrintProcessor);
   SZCPY( pDatatype);
   SZCPY( pParameters);
   if ( source-> pDevMode)
   {
      int sz = source-> pDevMode-> dmSize + source-> pDevMode-> dmDriverExtra;
      memcpy( dest-> pDevMode = ( LPDEVMODE) malloc( sz), source-> pDevMode, sz);
   }
}


static void ppi_destroy( LPPRINTER_INFO_2 ppi)
{
   if ( !ppi) return;
   free( ppi-> pPrinterName);
   free( ppi-> pServerName);
   free( ppi-> pShareName);
   free( ppi-> pPortName);
   free( ppi-> pDriverName);
   free( ppi-> pComment);
   free( ppi-> pLocation);
   free( ppi-> pSepFile);
   free( ppi-> pPrintProcessor);
   free( ppi-> pDatatype);
   free( ppi-> pParameters);
   free( ppi-> pDevMode);
   memset( ppi, 0, sizeof( PRINTER_INFO_2));
}


Bool
apc_prn_destroy( Handle self)
{
   ppi_destroy( &sys s. prn. ppi);
   return true;
}


static int
prn_query( Handle self, const char * printer, LPPRINTER_INFO_2 info)
{
   DWORD returned, needed;
   LPPRINTER_INFO_2 ppi, useThis = nil;
   int i;
   Bool useDefault = ( printer == nil || strlen( printer) == 0);
   char * device;

   EnumPrinters( PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL, nil,
         2, nil, 0, &needed, &returned);

   ppi = ( LPPRINTER_INFO_2) malloc( needed + 4);
   if ( !EnumPrinters( PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL, nil,
         2, ( LPBYTE) ppi, needed, &needed, &returned)) {
      apiErr;
      free( ppi);
      return 0;
   }

   if ( returned == 0) {
      apcErr( errNoPrinters);
      free( ppi);
      return 0;
   }

   device = apc_prn_get_default( self);

   for ( i = 0; i < returned; i++)
   {
      if ( useDefault && device && ( strcmp( device, ppi[ i]. pPrinterName) == 0))
      {
         useThis = &ppi[ i];
         break;
      }
      if ( !useDefault && ( strcmp( printer, ppi[ i]. pPrinterName) == 0))
      {
         useThis = &ppi[ i];
         break;
      }
   }
   if ( useDefault && useThis == nil) useThis = ppi;
   if ( useThis) ppi_create( info, useThis);
   if ( !useThis) apcErr( errInvPrinter);
   free( ppi);
   return useThis ? 1 : -2;
}


PrinterInfo*
apc_prn_enumerate( Handle self, int * count)
{
   DWORD returned, needed;
   LPPRINTER_INFO_2 ppi;
   PPrinterInfo list;
   char *printer;
   int i;

   *count = 0;
   objCheck nil;

   EnumPrinters( PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL, nil, 2,
         nil, 0, &needed, &returned);

   ppi = ( LPPRINTER_INFO_2) malloc( needed + 4);
   if ( !EnumPrinters( PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL, nil, 2,
         ( LPBYTE) ppi, needed, &needed, &returned)) {
      apiErr;
      free( ppi);
      return nil;
   }

   if ( returned == 0) {
      apcErr( errNoPrinters);
      free( ppi);
      return nil;
   }

   printer = apc_prn_get_default( self);

   list = ( PPrinterInfo) malloc( returned * sizeof( PrinterInfo));
   for ( i = 0; i < returned; i++)
   {
      strncpy( list[ i]. name,   ppi[ i]. pPrinterName, 255);   list[ i]. name[ 255]   = 0;
      strncpy( list[ i]. device, ppi[ i]. pPortName, 255);      list[ i]. device[ 255] = 0;
      list[ i]. defaultPrinter = (( printer != nil) && ( strcmp( printer, list[ i]. name) == 0));
   }
   *count = returned;
   free( ppi);
   return list;
}

static HDC prn_info_dc( Handle self)
{
   LPPRINTER_INFO_2 ppi = &sys s. prn. ppi;
   HDC ret = CreateIC( ppi-> pDriverName, ppi-> pPrinterName, ppi-> pPortName, ppi-> pDevMode);
   if ( !ret) apiErr;
   return ret;
}

Bool
apc_prn_select( Handle self, const char* printer)
{
   int rc;
   PRINTER_INFO_2 ppi;
   HDC dc;

   objCheck false;

   rc = prn_query( self, printer, &ppi);
   if ( rc > 0)
   {
      ppi_destroy( &sys s. prn. ppi);
      memcpy( &sys s. prn. ppi, &ppi, sizeof( ppi));
   } else
      return false;

   if ( !( dc = prn_info_dc( self))) return false;
   sys res.      x = ( float) GetDeviceCaps( dc, LOGPIXELSX);
   sys res.      y = ( float) GetDeviceCaps( dc, LOGPIXELSY);
   sys lastSize. x = GetDeviceCaps( dc, HORZRES);
   sys lastSize. y = GetDeviceCaps( dc, VERTRES);
   if ( !DeleteDC( dc)) apiErr;
   return true;
}

char *
apc_prn_get_selected( Handle self)
{
   objCheck "";
   return sys s. prn. ppi. pPrinterName;
}

Point
apc_prn_get_size( Handle self)
{
   Point p = {0,0};
   objCheck p;
   return sys lastSize;
}

Point
apc_prn_get_resolution( Handle self)
{
   Point p = {0,0};
   objCheck p;
   return sys res;
}

char *
apc_prn_get_default( Handle self)
{
   objCheck "";

   GetProfileString("windows", "device", ",,,", sys s. prn. defPrnBuf, 255);
   if (( sys s. prn. device = strtok( sys s. prn. defPrnBuf, (const char *) ","))
            && ( sys s. prn. driver = strtok((char *) NULL,
               (const char *) ", "))
            && ( sys s. prn. port = strtok ((char *) NULL,
               (const char *) ", "))) {

   } else
      sys s. prn. device = sys s. prn. driver = sys s. prn. port = nil;

   return sys s. prn. device;
}

Bool
apc_prn_setup( Handle self)
{
   void * lph;
   LONG sz, ret;
   DEVMODE * dm;
   HWND who = GetActiveWindow();
   HDC dc;

   objCheck false;
   if ( !OpenPrinter( sys s. prn. ppi. pPrinterName, &lph, nil))
      apiErrRet;
   sz = DocumentProperties( nil, lph, sys s. prn. ppi. pPrinterName, nil, nil, 0);
   if ( sz <= 0) {
      apiErr;
      ClosePrinter( lph);
      return false;
   }
   dm  = ( DEVMODE * ) malloc( sz);

   sys s. prn. ppi. pDevMode-> dmFields = -1;
   ret = DocumentProperties( hwnd_to_view( who) ? who : nil, lph, sys s. prn. ppi. pPrinterName,
       dm, sys s. prn. ppi. pDevMode, DM_IN_BUFFER|DM_IN_PROMPT|DM_OUT_BUFFER);
   ClosePrinter( lph);
   if ( ret != IDOK) {
      free( dm);
      return false;
   }
   free( sys s. prn. ppi. pDevMode);
   sys s. prn. ppi. pDevMode = dm;

   if ( !( dc = prn_info_dc( self))) return false;
   sys res.      x = ( float) GetDeviceCaps( dc, LOGPIXELSX);
   sys res.      y = ( float) GetDeviceCaps( dc, LOGPIXELSY);
   sys lastSize. x = GetDeviceCaps( dc, HORZRES);
   sys lastSize. y = GetDeviceCaps( dc, VERTRES);
   if ( !DeleteDC( dc)) apiErr;

   return true;
}

#define apiPrnErr       {                  \
   rc = GetLastError();                    \
   if ( rc != 1223 /* ERROR_CANCELLED */)  \
      apiAltErr( rc)                       \
   else                                    \
      apcErr( errUserCancelled);           \
}

Bool
apc_prn_begin_doc( Handle self, const char* docName)
{
   LPPRINTER_INFO_2 ppi = &sys s. prn. ppi;
   DOCINFO doc;
   doc. cbSize = sizeof( DOCINFO);
   doc. lpszDocName = docName;
   doc. lpszOutput = nil;
   doc. lpszDatatype = nil;
   doc. fwType = 0;

   objCheck false;
   if ( !( sys ps = CreateDC( ppi-> pDriverName, ppi-> pPrinterName, ppi-> pPortName, ppi-> pDevMode)))
      apiErrRet;

   if ( StartDoc( sys ps, &doc) <= 0) {
      apiPrnErr;
      DeleteDC( sys ps);
      sys ps = nil;
      return false;
   }
   if ( StartPage( sys ps) <= 0) {
      apiPrnErr;
      DeleteDC( sys ps);
      sys ps = nil;
      return false;
   }

   hwnd_enter_paint( self);
   if ( sys pal = palette_create( self)) {
      SelectPalette( sys ps, sys pal, 0);
      RealizePalette( sys ps);
   }
   return true;
}

Bool
apc_prn_begin_paint_info( Handle self)
{
   LPPRINTER_INFO_2 ppi = &sys s. prn. ppi;
   DOCINFO doc = { sizeof( DOCINFO), "", nil, nil, 0};

   objCheck false;
   if ( !( sys ps = CreateDC( ppi-> pDriverName, ppi-> pPrinterName, ppi-> pPortName, ppi-> pDevMode)))
      apiErrRet;

   hwnd_enter_paint( self);
   sys pal = palette_create( self);
   return true;
}


Bool
apc_prn_end_doc( Handle self)
{
   apcErrClear;

   objCheck false;
   if ( EndPage( sys ps) < 0) apiPrnErr;
   if ( EndDoc ( sys ps) < 0) apiPrnErr;

   hwnd_leave_paint( self);
   if ( sys pal) DeleteObject( sys pal);
   DeleteDC( sys ps);
   sys pal = sys ps = nil;
   return apcError == errOk;
}

Bool
apc_prn_end_paint_info( Handle self)
{
   apcErrClear;
   objCheck false;
   hwnd_leave_paint( self);
   DeleteDC( sys ps);
   sys ps = nil;
   return apcError == errOk;
}

Bool
apc_prn_new_page( Handle self)
{
   apcErrClear;
   objCheck false;
   if ( EndPage( sys ps) < 0) apiPrnErr;
   if ( StartPage( sys ps) < 0) apiPrnErr;
   return apcError == errOk;
}

Bool
apc_prn_abort_doc( Handle self)
{
   objCheck false;
   if ( AbortDoc( sys ps) < 0) apiPrnErr;
   hwnd_leave_paint( self);
   if ( sys pal) DeleteObject( sys pal);
   DeleteDC( sys ps);
   sys pal = sys ps = nil;
   return apcError == errOk;
}

#ifdef __cplusplus
}
#endif
