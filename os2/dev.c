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
/* apc.c --- apc/ api for os/2 */
#include <limits.h>
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#include "os2/os2guts.h"
#include "Image.h"
#include "DeviceBitmap.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)


HBITMAP
bitmap_make_ps( Handle img, HPS * hps, HDC * hdc, PBITMAPINFO2 * bm, int createBitmap)
{
   SIZEL sizl;
   HBITMAP ret = nilHandle;
   *bm  = nil;
   *hps = nilHandle;
   *hdc = nilHandle;

   sizl . cx = (( PDrawable) img)-> w;
   sizl . cy = (( PDrawable) img)-> h;

   if ( createBitmap && ( sizl. cx == 0 || sizl. cy == 0)) return false;

   *hdc = DevOpenDC ( guts. anchor, OD_MEMORY, "*", 0, nil, nilHandle );
   if ( *hdc == nilHandle ) { apiErr; return nilHandle;}

   *hps = GpiCreatePS ( guts. anchor, *hdc, &sizl,
                       PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC ) ;
   if ( *hps == nilHandle)
   {
      apiErr;
      DevCloseDC( *hdc);
      *hdc = nilHandle;
      return nilHandle;
   }
   if ( !GpiCreateLogColorTable( *hps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;

   if ( createBitmap) {
      Handle deja = nilHandle;
      PImage image;

      if ( createBitmap == cbImage) {
         deja  = cmono_enscreen( img);
         image = ( PImage) deja;
         if (( *bm  = get_binfo( deja)))
            ret = GpiCreateBitmap( *hps, ( PBITMAPINFOHEADER2) *bm, CBM_INIT, image-> data, *bm);
      } else {
         if (( *bm = get_screen_binfo())) {
            if ( createBitmap == cbMonoScreen) (*bm)-> cPlanes = (*bm)-> cBitCount = 1;
            (*bm)-> cx = (( PDrawable) img)-> w;
            (*bm)-> cy = (( PDrawable) img)-> h;
            ret = GpiCreateBitmap( *hps, ( PBITMAPINFOHEADER2) *bm, 0, nil, nilHandle);
         }
      }
      if ( ret == nilHandle)
      {
         apiErr;
         GpiDestroyPS( *hps);
         DevCloseDC( *hdc);
         free( *bm);
         *bm  = nil;
         *hps = *hdc = nilHandle;
      }
      if ( createBitmap == cbImage && deja != img) Object_destroy( deja);
      if ( ret == nilHandle) return nilHandle;
   } else
      ret = ( HBITMAP) 1;
   if ( GpiSetBitmap( *hps, nilHandle) == HBM_ERROR) apiErr;
   return ret;
}

Handle
bitmap_make_handle( Handle img)
{
    HDC hdc;
    HPS hps;
    PBITMAPINFO2 bmInfo;
    HBITMAP bm = bitmap_make_ps( img, &hps, &hdc, &bmInfo, cbImage);
    if ( bm == nilHandle) return nilHandle;
    if ( !GpiDestroyPS( hps)) apiErr;
    if ( !DevCloseDC( hdc)) apiErr;
    free( bmInfo);
    return ( Handle) bm;
}

Bool
image_begin_query( int primType, int * typeToConvert)
{
  Bool found = false;
  int i;
  for ( i = 0; i < guts. bmfCount; i++)
  {
     if ( primType == guts. bmf[ i * 2 + 1])
     {
        found = true;
        break;
     }
  }
  if ( !found)
  {
     if (( guts. bmf[ 1] == 15 || guts. bmf[ 1] == 16) &&
         ( primType == imbpp24)
        )
     found = true;
  }
  if ( found)
     *typeToConvert = primType;
   else
     *typeToConvert = ( guts. bmf[ 1] == 15 || guts. bmf[ 1] == 16) ?
       imbpp24 : guts. bmf[ 1];
  return !found;
}

PBITMAPINFO2
get_binfo( Handle self)
{
   int i;
   PImage       image = ( PImage) self;
   int          nColors;
   PBITMAPINFO2 bi;

   if ( is_apt( aptDeviceBitmap)) {
       if (( bi = get_screen_binfo())) {
          bi-> cx = image-> w;
          bi-> cy = image-> h;
          if ((( PDeviceBitmap) self)-> monochrome) bi-> cPlanes = bi-> cBitCount = 1;
       }
       return bi;
   }

   nColors = (( 1 << ( image-> type & imBPP)) & 0x1ff);
   bi      = malloc( sizeof ( BITMAPINFO2) + nColors * sizeof( RGB2));
   if ( !bi) return nil;
   memset( bi, 0, sizeof( BITMAPINFO2) + nColors * sizeof( RGB2));
   bi-> cbFix           = sizeof( BITMAPINFO2) - sizeof( RGB2);
   bi-> cx              = image-> w;
   bi-> cy              = image-> h;
   bi-> cPlanes         = 1;
   bi-> cBitCount       = image-> type & imBPP;
   bi-> ulCompression   = BCA_UNCOMP;
   bi-> cclrUsed        = nColors;
   bi-> cclrImportant   = nColors;
   bi-> usUnits         = BRU_METRIC;
   bi-> usRecording     = BRA_BOTTOMUP;
   bi-> usRendering     = BRH_NOTHALFTONED;
   bi-> ulColorEncoding = BCE_RGB;
   for ( i = 0; i < nColors; i++)
   {
      bi-> argbColor[ i]. bRed    = image-> palette[ i]. r;
      bi-> argbColor[ i]. bGreen  = image-> palette[ i]. g;
      bi-> argbColor[ i]. bBlue   = image-> palette[ i]. b;
   }
   if ( guts. monoBitsOk && bi-> cBitCount == 1)
   {
      bi-> argbColor[ 0]. bRed = bi-> argbColor[ 0]. bBlue = bi-> argbColor[ 0]. bGreen = 0xFF;
      bi-> argbColor[ 1]. bRed = bi-> argbColor[ 1]. bBlue = bi-> argbColor[ 1]. bGreen = 0x00;
   }
   return bi;
}

PBITMAPINFO2
get_screen_binfo( void)
{
    PBITMAPINFO2 br      = malloc( sizeof ( BITMAPINFO2) + 256 * sizeof( RGB2));
    if ( !br) return nil;

    memset( br, 0, sizeof ( BITMAPINFO2) + 256 * sizeof( RGB2));
    br-> cbFix           = sizeof( BITMAPINFO2) - sizeof( RGB2);
    br-> cPlanes         = guts. bmf[ 0];
    br-> cBitCount       = guts. bmf[ 1];
    br-> ulCompression   = BCA_UNCOMP;
    br-> usRecording     = BRA_BOTTOMUP;
    br-> usRendering     = BRH_NOTHALFTONED;
    br-> ulColorEncoding = BCE_RGB;
    return br;
}

Bool
screenable( Handle image)
{
   PImage i = ( PImage) image;
   if (( i-> type & ( imRealNumber | imComplexNumber | imTrigComplexNumber)) ||
       ( i-> type == imLong || i-> type == imShort)) return false;
   return true;
}

Handle
enscreen( Handle image)
{
   PImage i = ( PImage) image;
   if ( !screenable( image))
   {
      Handle j = i-> self-> dup( image);
      ((( PImage) j)-> self)->resample( j,
         ((( PImage) j)-> self)-> stats( j, false, isRangeLo, 0),
         ((( PImage) j)-> self)-> stats( j, false, isRangeHi, 0),
         0, 255
      );
      ((( PImage) j)-> self)->set_type( j, imByte);
      return j;
   } else
      return image;
}

Handle
cmono_enscreen( Handle image)
{
   PImage i = ( PImage) image;
   if ( !screenable( image))
   {
      return enscreen( image);
   } else
      if ((( i-> type & imBPP) == imbpp1) &&
          (
            ( ARGB( i-> palette[0].r, i-> palette[0].g, i-> palette[0].b) != clBlack) ||
            ( ARGB( i-> palette[1].r, i-> palette[1].g, i-> palette[1].b) != clWhite)
          ))
      {
         Handle j = i-> self-> dup( image);
         ((( PImage) j)-> self)->set_type( j, im16);
         return j;
      } else
      return image;
}

void
image_query( Handle self, HPS ps)
{
   PBITMAPINFO2 bi;
   PImage image = ( PImage) self;
   int i, nColors;

   if ( is_apt( aptDeviceBitmap)) {
      apcErr(0x82002/* bitmap in use */);
      return;
   }

   if ( !( bi = get_binfo( self))) return;
   nColors = (( 1 << ( image-> type & imBPP)) & 0x1ff);
   if ( GpiQueryBitmapBits( ps, 0, image-> h, image-> data, ( PBITMAPINFO2) bi) < 0) apiErr;
   for ( i = 0; i < nColors; i++)
   {
     image-> palette[ i]. r = bi-> argbColor[ i]. bRed;
     image-> palette[ i]. g = bi-> argbColor[ i]. bGreen;
     image-> palette[ i]. b = bi-> argbColor[ i]. bBlue;
   }
   free( bi);
}

void
bitmap_make_cache( Handle from, Handle self)
{
    HDC hdc;
    HPS hps;
    PBITMAPINFO2 bi, br;
    HBITMAP bm = bitmap_make_ps( from, &hps, &hdc, &bi, cbImage);

    if ( sys bmInfo) free( sys bmInfo);
    if ( sys bmRaw)  free( sys bmRaw);
    sys bmInfo = nil;
    sys bmRaw  = nil;

    if ( bm == nilHandle) goto EXIT;

    if ( !( br = get_screen_binfo())) goto EXIT;
    if ( !( sys bmRaw = malloc((( br-> cBitCount * bi-> cx + 31) / 32) * br-> cPlanes * 4 * bi-> cy))) {
       free( sys bmRaw);
       goto EXIT;
    }
    sys bmInfo = br;
    if ( GpiSetBitmap( hps, bm) == HBM_ERROR) apiErr;
    if ( GpiQueryBitmapBits( hps, 0, bi-> cy, sys bmRaw, ( PBITMAPINFO2) br) < 0)
       apiErr;
    if ( GpiSetBitmap( hps, nilHandle) == HBM_ERROR) apiErr;
    if ( !GpiDeleteBitmap( bm)) apiErr;
    if ( !GpiDestroyPS( hps))   apiErr;
    if ( !DevCloseDC( hdc))     apiErr;
EXIT:;
    free( bi);
}

static void
image_destroy_cache( Handle self)
{
   if ( sys bmRaw)  free( sys bmRaw);
   if ( sys bmInfo) free( sys bmInfo);
   sys bmRaw =  nil;
   sys bmInfo = nil;
}

void
image_set_cache( Handle from, Handle self)
{
   if ( sys bmRaw == nil)
      bitmap_make_cache( from, self);
}

// Image
Bool
apc_image_create( Handle self)
{
   image_destroy_cache( self);
   return true;
}

Bool
apc_image_update_change( Handle self)
{
   image_destroy_cache( self);
   return true;
}

Bool
apc_image_destroy( Handle self)
{
   image_destroy_cache( self);
   return true;
}

#define putbmp(x,y,bm) {                                          \
  POINTL point = {x,y};                                               \
  HPS display = WinGetScreenPS( HWND_DESKTOP);                    \
  WinDrawBitmap(display,bm,nil,&point,CLR_WHITE,CLR_BLACK,DBM_NORMAL);\
  WinReleasePS( display);                                         \
}

/* XXX palette operations */
Bool
apc_image_begin_paint( Handle self)
{
   PBITMAPINFO2 bi;
   HBITMAP cached = sys bm;
   HBITMAP ret;
   Handle deja = self;
   Bool retmonoback = false;

   if ( PImage( self)-> w == 0 || PImage( self)-> h == 0) return false;

   apcErrClear;
   sys bm = nilHandle;
   image_destroy_cache( self);
   sys bm = cached;

   // creating HDC and HPS
   ret = bitmap_make_ps( self, &sys ps, &sys dc, &bi, cbNoBitmap);
   if ( ret == nilHandle) {
      image_destroy_cache( self);
      return false;
   }

   // upgrade to color image, if necessary
   if ( PImage( self)-> type == imbpp1) {
      int type = guts. bmf[1];
      image_begin_query( type, &type);
      if ( type > 8) type = 24;
      if ( type != imbpp1) {
          opt_clear( optInDraw);
          CImage( self)-> set_type( self, type);
          opt_set( optInDraw);
          retmonoback = true;
      }
   }

   // creating bitmap ( or use cached)
   if ( sys bm == nilHandle) {
      Handle deja  = enscreen( self);
      if (( bi = get_binfo( deja)))
         sys bm = GpiCreateBitmap( sys ps, ( PBITMAPINFOHEADER2) bi, 0, nil, nil);
      if ( sys bm == nilHandle ) {
         apiErr;
         GpiDestroyPS( sys ps);
         sys ps = nilHandle;
         DevCloseDC( sys dc);
         free( bi);
         if ( deja != self) Object_destroy( deja);
         if ( retmonoback) {
            opt_clear( optInDraw);
            CImage( self)-> set_type( self, imbpp1);
            opt_set( optInDraw);
         }
         return false;
      }
   }
   // binding them together
   if ( GpiSetBitmap( sys ps, sys bm) == HBM_ERROR) apiErr;
   // setting bits
   if ( cached == nilHandle) {
      PImage i = ( PImage) deja;
      if ( GpiSetBitmapBits( sys ps, 0, i-> h, i-> data, bi) < 0) apiErr;
      if ( deja != self) Object_destroy( deja);
      free( bi);
   }

   if ( retmonoback) {
      opt_clear( optInDraw);
      CImage( self)-> set_type( self, imbpp1);
      opt_set( optInDraw);
   }

   hwnd_enter_paint( self);
   if (( PImage( self)-> type & imBPP) == imbpp1) sys bpp = 1;
   return true;
}

/* XXX */
Bool
apc_image_begin_paint_info( Handle self)
{
   PBITMAPINFO2 bi;
   HBITMAP ret;

   apcErrClear;
   // creating HDC and HPS
   ret = bitmap_make_ps( self, &sys ps, &sys dc, &bi, cbNoBitmap);
   if ( ret == nilHandle) {
      image_destroy_cache( self);
      return false;
   }
   hwnd_enter_paint( self);
   if (( PImage( self)-> type & imBPP) == imbpp1) sys bpp = 1;
   return true;
}


/* XXX palette operations */
Bool
apc_image_end_paint( Handle self)
{
   PImage image = ( PImage) self;
   apcErrClear;
   {
      int type, oldType = image-> type & imBPP;
      if ( image-> type == imbpp1) {
         type = guts. bmf[1];
         image_begin_query( type, &type);
         if ( type > 8) type = 24;
         image-> self-> create_empty( self, image-> w, image-> h, type);
      } else
         if ( image_begin_query( oldType, &type))
            image-> self-> create_empty( self, image-> w, image-> h, type);
      image_query( self, sys ps);
   }

   hwnd_leave_paint( self);

   if ( GpiSetBitmap( sys ps, nilHandle) == HBM_ERROR) apiErr;
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
   image_destroy_cache( self);
   sys ps = nilHandle;
   return true;
}

/* XXX */
Bool
apc_image_end_paint_info( Handle self)
{
   apcErrClear;
   hwnd_leave_paint( self);
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
   image_destroy_cache( self);
   sys ps = nilHandle;
   return apcError == errOk;
}

ApiHandle
apc_image_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}

// Image end
// DBM

/* XXX palette operations */
Bool
apc_dbm_create( Handle self, Bool monochrome)
{
   PBITMAPINFO2 bi;
   HBITMAP ret;

   apcErrClear;
   sys bm = nilHandle;
   apt_set( aptDeviceBitmap);

   // creating HDC and HPS
   ret = bitmap_make_ps( self, &sys ps, &sys dc, &bi, monochrome ? cbMonoScreen : cbScreen);
   if ( ret == nilHandle) return false;
   free( bi);
   if ( GpiSetBitmap( sys ps, ret) == HBM_ERROR) apiErrRet;
   sys bm = ret;
   hwnd_enter_paint( self);
   if ( monochrome) sys bpp = 1;
   return true;
}

/* XXX palette operations */
Bool
apc_dbm_destroy( Handle self)
{
   apcErrClear;
   hwnd_leave_paint( self);
   if ( GpiSetBitmap( sys ps, nilHandle) == HBM_ERROR) apiErr;
   if ( !GpiDeleteBitmap( sys bm)) apiErr;
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
   sys ps = sys dc = sys bm = nilHandle;
   return true;
}

ApiHandle
apc_dbm_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}



// DBM end



