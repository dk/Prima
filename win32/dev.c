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

Bool image_screenable( Handle image, Handle screen, int * bitCount)
{
   PImage i = ( PImage) image;
   if (( i-> type & ( imRealNumber | imComplexNumber | imTrigComplexNumber)) ||
       ( i-> type == imLong || i-> type == imShort)) {
      if ( bitCount) *bitCount = 8;
      return false;
   }

   if ( i-> type == imRGB) {
      if ( !screen || dsys( screen) options. aptCompatiblePS || !dsys( screen) ps) {
         if ( guts. displayBMInfo. bmiHeader. biBitCount <= 8) {
            if ( bitCount) {
               *bitCount = guts. displayBMInfo. bmiHeader. biBitCount;
               if ( *bitCount < 4) *bitCount = 1;
               else if ( *bitCount < 8) *bitCount = 4;
            }
            return false;
         }
      } else {
         int bc = GetDeviceCaps( dsys( screen) ps, BITSPIXEL);
         if ( bc <= 8) {
            if ( bitCount) {
               *bitCount = bc;
               if ( *bitCount < 4) *bitCount = 1;
               else if ( *bitCount < 8) *bitCount = 4;
            }
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
            ((( PImage) j)-> self)->get_stats( j, isRangeLo),
            ((( PImage) j)-> self)->get_stats( j, isRangeHi),
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
         nColors = 1 << bi-> bmiHeader. biBitCount;
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
      nColors  = 1 << lower;
      bitCount = lower;
   }

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

   if ( !( bm = CreateDIBitmap( dc, &bi-> bmiHeader, CBM_INIT,
        (( PImage) img)-> data, bi, DIB_RGB_COLORS))) apiErr;

   if ( old) {
      SelectPalette( dc, old, 1);
      RealizePalette( dc);
   }

   if ( xpal != pal)
      DeleteObject( xpal);

   return bm;
}

HPALETTE
image_make_bitmap_palette( Handle img)
{
   PDrawable i    = ( PDrawable) img;
   int j, nColors = i-> palSize;
   XLOGPALETTE lp = { 0x300, nColors};
   HPALETTE r;
   RGBColor  dest[ 256];
   PRGBColor logp = i-> palette;
   if ( nColors == 0) return nil;

   if ( nColors > 256) {
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
   if ( !( r = CreatePalette(( LOGPALETTE*) &lp))) apiErrRet;
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
   }
   if ( sys pal)
      if ( !DeleteObject( sys pal)) apiErr;
   sys bm = sys pal = nilHandle;
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

   if ( forceNewImage) {
      ops = sys ps;
      if ( !ops) sys ps = dc_alloc();
   }

   xbi2. bmiHeader. biBitCount = 0;
   xbi2. bmiHeader. biSize     = sizeof( BITMAPINFO);
   if ( !GetDIBits( sys ps, sys bm, 0, 0, nil, ( BITMAPINFO *) &xbi2, DIB_RGB_COLORS)) apiErr;

   if (( xbi2. bmiHeader. biPlanes == 1) && (
          ( xbi2. bmiHeader. biBitCount == 1) ||
          ( xbi2. bmiHeader. biBitCount == 4) ||
          ( xbi2. bmiHeader. biBitCount == 8) ||
          ( xbi2. bmiHeader. biBitCount == 24)
      ))
      newBits = xbi2. bmiHeader. biBitCount;
   else {
      newBits = ( xbi2. bmiHeader. biBitCount <= 4) ? 4 :
                (( xbi2. bmiHeader. biBitCount <= 8) ? 8 : 24);
   }

   if ( forceNewImage) {
      i-> self-> create_empty( self, xbi2. bmiHeader. biWidth, xbi2. bmiHeader. biHeight, newBits);
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
      sys ps = ops;
      if ( !ops) dc_free();
   }
}


Bool
apc_image_create( Handle self)
{
   apt_set( aptBitmap);
   image_destroy_cache( self);
   sys lastSize. x = var w;
   sys lastSize. y = var h;
   return true;
}

void
apc_image_destroy( Handle self)
{
   image_destroy_cache( self);
}

Bool
apc_image_begin_paint( Handle self)
{
   apcErrClear;
   if ( !( sys ps = CreateCompatibleDC( 0))) apiErrRet;
   if ( sys bm == nilHandle) {
      Handle deja  = image_enscreen( self, self);
      image_set_cache( deja, self);
      if ( deja != self) Object_destroy( deja);
   }
   sys stockBM = SelectObject( sys ps, sys bm);
   if ( !sys pal)
      sys pal = image_make_bitmap_palette( self);
   hwnd_enter_paint( self);
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
   if ( !( sys ps = CreateCompatibleDC( 0))) apiErrRet;
   if ( sys pal) SelectPalette( sys ps, sys pal, 0);
   hwnd_enter_paint( self);
   return true;
}


void
apc_image_end_paint( Handle self)
{
   BITMAPINFO * bi;
   apcErrClear;

   image_query_bits( self, false);
   hwnd_leave_paint( self);
   if ( sys stockBM)
      SelectObject( sys ps, sys stockBM);
   DeleteDC( sys ps);
   sys stockBM = sys ps = nil;
}

void
apc_image_end_paint_info( Handle self)
{
   apcErrClear;
   hwnd_leave_paint( self);
   DeleteDC( sys ps);
   sys ps = nil;
}


void
apc_image_update_change( Handle self)
{
   image_destroy_cache( self);
   sys lastSize. x = var w;
   sys lastSize. y = var h;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
   HDC dc;
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
      }
      sys bm = CreateCompatibleBitmap( dc_alloc(), var w, var h);
      if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
         apt_clear( aptCompatiblePS);
   }

   if ( !sys bm) {
      apiErr;
      if ( !monochrome) dc_free();
      DeleteDC( sys ps);
      return false;
   }
   if ( !monochrome) dc_free();

   sys stockBM = SelectObject( sys ps, sys bm);

   hwnd_enter_paint( self);

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

void
apc_dbm_destroy( Handle self)
{
   apcErrClear;
   hash_delete( imageMan, &self, sizeof( self), false);

   hwnd_leave_paint( self);

   if ( sys pal)
      DeleteObject( sys pal);
   if ( sys stockBM)
     SelectObject( sys ps, sys stockBM);
   DeleteObject( sys bm);
   DeleteDC( sys ps);
   sys pal = sys stockBM = sys ps = sys bm = nil;
}

/*
static void
bm_put_zs( HBITMAP hbm, int x, int y, int z)
{
   HDC dc = dc_alloc();
   HDC xdc = CreateCompatibleDC( 0);
   BITMAPINFO bm;
   int cx, cy;


   bm. bmiHeader. biBitCount = 0;
   bm. bmiHeader. biSize = sizeof( BITMAPINFO);
   SelectObject( xdc, hbm);
   GetDIBits( xdc, hbm, 0, 0, NULL, &bm, DIB_PAL_COLORS);
   cx = bm. bmiHeader. biWidth;
   cy = bm. bmiHeader. biHeight;

   StretchBlt( dc, x, y, z * cx, z * cy, xdc, 0, 0, cx, cy, SRCCOPY);

   DeleteDC( xdc);
   dc_free();
}
*/


HICON
image_make_icon_handle( Handle img, Point size, Point * hotSpot)
{
   PIcon i = ( PIcon) img;
   HICON    r;
   ICONINFO ii = { hotSpot ? false : true, hotSpot ? hotSpot-> x : 0, hotSpot ? hotSpot-> y : 0};
   int   j, bpp = i-> type & imBPP;
   Bool  noSZ   = i-> w != size. x || i-> h != size. y;
   Bool  noBPP  = bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24;
   HDC dc;
   XBITMAPINFO bi;

   ( Handle) i = i-> self-> dup( img);
   if ( noSZ || noBPP) {
      if ( noSZ)
         i-> self-> set_size(( Handle) i, size. x, size. y);
      if ( noBPP)
         i-> self-> set_type(( Handle) i,
             ( bpp < 4) ? 1 :
             (( bpp < 8) ? 4 :
             (( bpp < 24) ? 8 : 24))
      );
   }

   for ( j = 0; j < i-> maskSize; j++) i-> mask[ j] = ~i-> mask[ j];

   dc = dc_alloc();
   image_get_binfo(( Handle)i, &bi);
   if ( !( ii. hbmColor = CreateDIBitmap( dc, &bi. bmiHeader, CBM_INIT,
       i-> data, ( BITMAPINFO*) &bi, DIB_RGB_COLORS))) apiErr;
   bi. bmiHeader. biBitCount = 1;
   bi. bmiColors[ 0]. rgbRed = bi. bmiColors[ 0]. rgbGreen = bi. bmiColors[ 0]. rgbBlue = 0;
   bi. bmiColors[ 1]. rgbRed = bi. bmiColors[ 1]. rgbGreen = bi. bmiColors[ 1]. rgbBlue = 255;

   if ( !( ii. hbmMask  = CreateDIBitmap( dc, &bi. bmiHeader, CBM_INIT,
       i-> mask, ( BITMAPINFO*) &bi, DIB_RGB_COLORS))) apiErr;
   dc_free();
   if ( !( r = CreateIconIndirect( &ii))) apiErr;

   DeleteObject( ii. hbmColor);
   DeleteObject( ii. hbmMask);
   Object_destroy(( Handle) i);
   return r;
}


Bool
apc_prn_create( Handle self) {
//   sys lastSize. x = var w;
//   sys lastSize. y = var h;
   return true;
}

static void ppi_create( LPPRINTER_INFO_2 dest, LPPRINTER_INFO_2 source)
{
#define SZCPY(field) if ( source-> field) strcpy( dest-> field = malloc( strlen( source-> field) + 1), source-> field)
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
      memcpy( dest-> pDevMode = malloc( sz), source-> pDevMode, sz);
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


void
apc_prn_destroy( Handle self)
{

}


static int
prn_query( Handle self, char * printer, LPPRINTER_INFO_2 info)
{
   DWORD returned, needed;
   LPPRINTER_INFO_2 ppi, useThis = nil;
   int i;
   Bool useDefault = ( printer == nil || strlen( printer) == 0);
   char * device;

   EnumPrinters( PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL, nil,
         2, nil, 0, &needed, &returned);

   ppi = malloc( needed + 4);
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

   EnumPrinters( PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL, nil, 2,
         nil, 0, &needed, &returned);

   ppi = malloc( needed + 4);
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

   list = malloc( returned * sizeof( PrinterInfo));
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
apc_prn_select( Handle self, char * printer)
{
   int rc;
   PRINTER_INFO_2 ppi;
   HDC dc;
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
   return sys s. prn. ppi. pPrinterName;
}

Point
apc_prn_get_size( Handle self)
{
   return sys lastSize;
}

Point
apc_prn_get_resolution( Handle self)
{
   return sys res;
}

char *
apc_prn_get_default( Handle self)
{
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

   if ( !OpenPrinter( sys s. prn. device, &lph, nil))
      apiErrRet;
   sz = DocumentProperties( nil, lph, sys s. prn. device, nil, nil, 0);
   if ( sz <= 0) {
      apiErr;
      ClosePrinter( lph);
      return false;
   }
   dm  = malloc( sz);

   sys s. prn. ppi. pDevMode-> dmFields = -1;
   ret = DocumentProperties( hwnd_to_view( who) ? who : nil, lph, sys s. prn. device,
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
apc_prn_begin_doc( Handle self, char * docName)
{
   LPPRINTER_INFO_2 ppi = &sys s. prn. ppi;
   DOCINFO doc = { sizeof( DOCINFO), docName, nil, nil, 0};

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

   if ( !( sys ps = CreateDC( ppi-> pDriverName, ppi-> pPrinterName, ppi-> pPortName, ppi-> pDevMode)))
      apiErrRet;

   hwnd_enter_paint( self);
   return true;
}


void
apc_prn_end_doc( Handle self)
{
   apcErrClear;

   if ( EndPage( sys ps) < 0) apiPrnErr;
   if ( EndDoc( sys ps) < 0) apiPrnErr;

   hwnd_leave_paint( self);
   if ( sys pal) DeleteObject( sys pal);
   DeleteDC( sys ps);
   sys pal = sys ps = nil;
}

void
apc_prn_end_paint_info( Handle self)
{
   apcErrClear;
   hwnd_leave_paint( self);
   DeleteDC( sys ps);
   sys ps = nil;
}

void
apc_prn_new_page( Handle self)
{
   if ( EndPage( sys ps) < 0) apiPrnErr;
   if ( StartPage( sys ps) < 0) apiPrnErr;
}

void
apc_prn_abort_doc( Handle self)
{
   if ( AbortDoc( sys ps) < 0) apiPrnErr;

   hwnd_leave_paint( self);
   if ( sys pal) DeleteObject( sys pal);
   DeleteDC( sys ps);
   sys pal = sys ps = nil;
}

