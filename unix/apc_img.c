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
/*
 * System dependent image routines (unix, x11)
 */

#include "unix/guts.h"
#include "Image.h"
#include "Icon.h"
#include "DeviceBitmap.h"

#define REVERT(a)	( XX-> size. y - (a) - 1 )
#define SHIFT(a,b)	{ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; }
/* Multiple evaluation macro! */
#define REVERSE_BYTES_32(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))
#define REVERSE_BYTES_16(x) ((((x)&0xff)<<8 ) | (((x)&0xff00)>>8))

#define ByteBits                8
#define ByteMask                0xff
#define ByteValues              256
#define LOWER_BYTE(x)           ((x)&ByteMask)
#define ColorComponentMask      ByteMask
#define LSNibble                0x0f
#define LSNibbleShift           0
#define MSNibble                0xf0
#define MSNibbleShift           4
#define NPalEntries4            16
#define NPalEntries8            256

typedef U8 Pixel8;
typedef unsigned long XPixel;

typedef uint16_t Pixel16;

typedef struct
{
   Pixel16 a;
   Pixel16 b;
} Duplet16;

typedef struct
{
   ColorComponent a0, a1, a2;
} Pixel24;

typedef struct
{
   ColorComponent a0, a1, a2;
   ColorComponent b0, b1, b2;
} Duplet24;

typedef uint32_t Pixel32;

typedef struct
{
   Pixel32 a;
   Pixel32 b;
} Duplet32;


#define get_ximage_data(xim)            ((xim)->data_alias)
#define get_ximage_bytes_per_line(xim)  ((xim)->bytes_per_line_alias)

#ifdef USE_MITSHM
static int
shm_ignore_errors(Display *d, XErrorEvent *ev)
{
   guts.xshmattach_failed = true;
   return 0;
}
#endif

PrimaXImage*
prima_prepare_ximage( int width, int height, Bool bitmap)
{
   PrimaXImage *i;
   int extra_bytes;

   if (width == 0 || height == 0) return false;
  
   switch ( guts.idepth) {
   case 16:     extra_bytes = 1;        break;
   case 24:     extra_bytes = 5;        break;
   case 32:     extra_bytes = 7;        break;
   default:     extra_bytes = 0;
   }

   i = malloc( sizeof( PrimaXImage));
   if (!i) {
      warn("No enough memory");
      return nil;
   }   
   bzero( i, sizeof( PrimaXImage));

#ifdef USE_MITSHM
   if ( guts. shared_image_extension && !bitmap) {
      i-> image = XShmCreateImage( DISP, VISUAL, 
                                   bitmap ? 1 : guts.depth,
                                   bitmap ? XYBitmap : ZPixmap,
                                   nil, &i->xmem, width, height);
      XCHECKPOINT;
      if ( !i-> image) goto normal_way;
      i-> bytes_per_line_alias = i-> image-> bytes_per_line;
      i-> xmem. shmid = shmget( IPC_PRIVATE,
                                i-> image-> bytes_per_line * height + extra_bytes,
                                IPC_CREAT | 0666);
      if ( i-> xmem. shmid < 0) {
         XDestroyImage( i-> image);
         goto normal_way;
      }
      i-> xmem. shmaddr = i-> image-> data = shmat( i-> xmem. shmid, 0, 0);
      if ( i-> xmem. shmaddr == (void*)-1 || i-> xmem. shmaddr == nil) {
         i-> image-> data = nil;
         XDestroyImage( i-> image);
         shmctl( i-> xmem. shmid, IPC_RMID, 0);
         goto normal_way;
      }
      i-> xmem. readOnly = false;
      guts.xshmattach_failed = false;
      XSetErrorHandler(shm_ignore_errors);
      if ( XShmAttach(DISP, &i->xmem) == 0) {
         XCHECKPOINT;
bad_xshm_attach:
         XSetErrorHandler(guts.main_error_handler);
         i-> image-> data = nil;
         XDestroyImage( i-> image);
         shmdt( i-> xmem. shmaddr);
         shmctl( i-> xmem. shmid, IPC_RMID, 0);
         goto normal_way;
      }
      XCHECKPOINT;
      XSync(DISP,false);
      XCHECKPOINT;
      if (guts.xshmattach_failed)       goto bad_xshm_attach;
      shmctl( i-> xmem. shmid, IPC_RMID, 0);
      i-> data_alias = i-> image-> data;
      i-> shm = true;
      return i;
   }
normal_way:
#endif
   i-> bytes_per_line_alias = (( width * (bitmap ? 1 : guts.idepth) + 31) / 32) * 4;
   i-> data_alias = malloc( height * i-> bytes_per_line_alias + extra_bytes);
   if (!i-> data_alias) {
      warn("No enough memory");
      free(i);
      return nil;
   }
   i-> image = XCreateImage( DISP, VISUAL, 
                             bitmap ? 1 : guts.depth,
                             bitmap ? XYBitmap : ZPixmap,
                             0, i-> data_alias,
                             width, height, 32, i-> bytes_per_line_alias);
   XCHECKPOINT;
   if ( !i-> image) {
      warn("XCreateImage(%d,%d) error", width, height);
      free( i-> data_alias);
      free( i);
      return nil;
   }
   return i;
}

void
prima_XDestroyImage( XImage * i)
{
   if ( i) {
      if ( i-> data) {
         free( i-> data);
         i-> data = nil;
      }
      ((*((i)->f.destroy_image))((i)));
   }
}

Bool
prima_free_ximage( PrimaXImage *i) 
{
   if (!i) return true;
#ifdef USE_MITSHM
   if ( i-> shm) {
      XShmDetach( DISP, &i-> xmem);
      i-> image-> data = nil;
      XDestroyImage( i-> image);
      shmdt( i-> xmem. shmaddr);
      free(i);
      return true;
   }
#endif
   XDestroyImage( i-> image);
   free(i);
   return true;
}

static Bool
destroy_ximage( PrimaXImage *i)
{
   if ( !i) return true;
   if ( i-> ref_cnt > 0) {
      i-> can_free = true;
      return true;
   }
   return prima_free_ximage( i);
}

static Bool
destroy_one_ximage( PrimaXImage *i, int nothing1, void *nothing2, void *nothing3)
{
   prima_free_ximage( i);
   return false;
}

void
prima_gc_ximages( void )
{
   if ( !guts.ximages) return;
   hash_first_that( guts.ximages, (void*)destroy_one_ximage, nil, nil, nil);
}

void
prima_ximage_event( XEvent *eve) /* to be called from apc_event's handle_event */
{
#ifdef USE_MITSHM
   XShmCompletionEvent *ev = (XShmCompletionEvent*)eve;
   PrimaXImage *i;

   if ( eve && eve-> type == guts. shared_image_completion_event) {
      i = hash_fetch( guts.ximages, (void*)&ev->shmseg, sizeof(ev->shmseg));
      if ( i) {
         i-> ref_cnt--;
         if ( i-> ref_cnt <= 0) {
            hash_delete( guts.ximages, (void*)&ev->shmseg, sizeof(ev->shmseg), false);
            if ( i-> can_free)
               prima_free_ximage( i);
         }
      }
   }
#endif
}

void
prima_put_ximage( 
	XDrawable win, GC gc, PrimaXImage *i, 
	int src_x, int src_y, int dst_x, int dst_y, int width, int height
) {
   if ( src_x < 0) {
      width += src_x;
      dst_x -= src_x;
      src_x = 0;
      if ( width <= 0) return;
   }
#ifdef USE_MITSHM
   if ( i-> shm) {
      if ( src_y + height > i-> image-> height)
         height = i-> image-> height - src_y;
      if ( i-> ref_cnt < 0)
         i-> ref_cnt = 0;
      i-> ref_cnt++;
      if ( i-> ref_cnt == 1)
         hash_store( guts.ximages, &i->xmem.shmseg, sizeof(i->xmem.shmseg), i);
      XShmPutImage( DISP, win, gc, i-> image, src_x, src_y, dst_x, dst_y, width, height, true);
      XFlush(DISP);
      return;
   }
#endif
   XPutImage( DISP, win, gc, i-> image, src_x, src_y, dst_x, dst_y, width, height);
   XCHECKPOINT;
}


/* image & bitmaps */
Bool
apc_image_create( Handle self)
{
   DEFXX;
   XX-> type.image = true;
   XX-> type.icon = !!kind_of(self, CIcon);
   XX-> type.drawable = true;
   XX-> image_cache. type = CACHE_AUTODETECT;
   XX->size. x                  = PImage(self)-> w;
   XX->size. y                  = PImage(self)-> h;
   return true;
}

static void
clear_caches( Handle self)
{
   DEFXX;

   prima_palette_free( self, false);
   destroy_ximage( XX-> image_cache. icon);
   destroy_ximage( XX-> image_cache. image);
   XX-> image_cache. icon      = nil;
   XX-> image_cache. image     = nil;
}

Bool
apc_image_destroy( Handle self)
{
   clear_caches( self);
   return true;
}

ApiHandle
apc_image_get_handle( Handle self)
{
   return (ApiHandle) X(self)-> gdrawable;
}

Bool
apc_image_begin_paint_info( Handle self)
{
    DEFXX;
    PImage img = PImage( self);
    Bool bitmap = (img-> type == imBW) || ( guts. idepth == 1);
    if ( !DISP) return false;
    XX-> gdrawable = XCreatePixmap( DISP, guts. root, 1, 1, 
       bitmap ? 1 : guts. depth);
    XCHECKPOINT;
    XX-> type.pixmap = !bitmap;
    XX-> type.bitmap = !!bitmap;
    prima_prepare_drawable_for_painting( self, false);
    XX-> size. x = 1;
    XX-> size. y = 1;
    return true;
}

Bool
apc_image_end_paint_info( Handle self)
{
   DEFXX;
   prima_cleanup_drawable_after_painting( self);
   if ( XX-> gdrawable) {
      XFreePixmap( DISP, XX-> gdrawable);
      XCHECKPOINT;
      XX-> gdrawable = 0;
   }
   XX-> size. x = PImage( self)-> w;
   XX-> size. y = PImage( self)-> h;
   return true;
}

Bool
apc_image_update_change( Handle self)
{
   DEFXX;
   PImage img = PImage( self);

   clear_caches( self);

   XX-> size. x = img-> w;
   XX-> size. y = img-> h;
   if ( guts. depth > 1)
      XX-> type.pixmap = (img-> type == imBW) ? 0 : 1;
   else
      XX-> type.pixmap = 0;
   XX-> type.bitmap = !!XX-> type.pixmap;
   if ( XX-> cached_region) {
      XDestroyRegion( XX-> cached_region);
      XX-> cached_region = nil;
   }
   return true;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
   DEFXX;
   if ( !DISP) return false;
   if ( guts. idepth == 1) monochrome = true;
   XX-> type.bitmap = !!monochrome;
   XX-> type.pixmap = !monochrome;
   XX-> type.dbm = true;
   XX-> type.drawable = true;
   XX->size. x          = ((PDeviceBitmap)(self))-> w;
   XX->size. y          = ((PDeviceBitmap)(self))-> h;
   if ( XX-> size.x == 0) XX-> size.x = 1;
   if ( XX-> size.y == 0) XX-> size.y = 1;
   XX->gdrawable        = XCreatePixmap( DISP, guts. root, XX->size. x, XX->size. y,
                                         monochrome ? 1 : guts.depth);
   if (XX-> gdrawable == None) return false;
   XCHECKPOINT;
   prima_prepare_drawable_for_painting( self, false);
   return true;
}

Bool
apc_dbm_destroy( Handle self)
{
   DEFXX;
   if ( XX->gdrawable) {
      prima_cleanup_drawable_after_painting( self);
      XFreePixmap( DISP, XX->gdrawable);
      XX-> gdrawable = None;
   }
   return true;
}

ApiHandle
apc_dbm_get_handle( Handle self)
{
   return (ApiHandle) X(self)-> gdrawable;
}

static Byte*
mirror_bits( void)
{
   static Bool initialized = false;
   static Byte bits[256];
   unsigned int i, j;
   int k;

   if (!initialized) {
      for ( i = 0; i < 256; i++) {
         bits[i] = 0;
         j = i;
         for ( k = 0; k < 8; k++) {
            bits[i] <<= 1;
            if ( j & 0x1)
               bits[i] |= 1;
            j >>= 1;
         }
      }
      initialized = true;
   }

   return bits;
}

void
prima_copy_xybitmap( unsigned char *data, const unsigned char *idata, int w, int h, int ls, int ils)
{
   int y;
   register int x;
   Byte *mirrored_bits;

   /* XXX: MSB/LSB */
   if ( guts.bit_order == MSBFirst) {
      for ( y = h-1; y >= 0; y--) {
	 memcpy( ls*(h-y-1)+data, idata+y*ils, ls);
      }
   } else {
      mirrored_bits = mirror_bits();
      w = ( w + 7) / 8;
      for ( y = h-1; y >= 0; y--) {
	 register const unsigned char *s = idata+y*ils;
	 register unsigned char *t = ls*(h-y-1)+data;
	 for ( x = 0; x < w; x++) {
	    *t++ = mirrored_bits[*s++];
	 }
      }
   }
}

void
prima_mirror_bytes( unsigned char *data, int dataSize)
{
   Byte *mirrored_bits = mirror_bits();
   while ( dataSize--) {
      *data = mirrored_bits[*data];
      data++;
   }
}

static Bool
create_cache1_1( Image *img, ImageCache *cache, Bool for_icon)
{
   unsigned char *data;
   int ls;
   int h = img-> h, w = img-> w;
   int ils;
   unsigned char *idata;
   PrimaXImage *ximage;

   if ( for_icon) {
      ils = PIcon(img)->maskLine;
      idata = PIcon(img)->mask;
   } else {
      ils = img-> lineSize;
      idata = img-> data;
   }

   ximage = prima_prepare_ximage( w, h, true);
   if (!ximage) return false;
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);
   prima_copy_xybitmap( data, idata, w, h, ls, ils);

   if ( for_icon) {
      cache-> icon  = ximage;
   } else {
      cache-> image = ximage;
   }
   return true;
}

static void
create_rgb_to_8_lut( int ncolors, const PRGBColor pal, Pixel8 *lut)
{
   int i;
   for ( i = 0; i < ncolors; i++) 
      lut[i] = 
            (((pal[i].r << guts. red_range  ) >> 8) << guts.   red_shift) |
            (((pal[i].g << guts. green_range) >> 8) << guts. green_shift) |
            (((pal[i].b << guts. blue_range ) >> 8) << guts.  blue_shift);
}

static void
create_rgb_to_16_lut( int ncolors, const PRGBColor pal, Pixel16 *lut)
{
   int i;
   for ( i = 0; i < ncolors; i++) 
      lut[i] = 
            (((pal[i].r << guts. red_range  ) >> 8) << guts.   red_shift) |
            (((pal[i].g << guts. green_range) >> 8) << guts. green_shift) |
            (((pal[i].b << guts. blue_range ) >> 8) << guts.  blue_shift);
   if ( guts.machine_byte_order != guts.byte_order) 
      for ( i = 0; i < ncolors; i++) 
         lut[i] = REVERSE_BYTES_16(lut[i]);
}

static int *
rank_rgb_shifts( void)
{
   static int shift[3];
   static Bool shift_unknown = true;

   if ( shift_unknown) {
      int xchg;
      shift[0] = guts. red_shift;
      shift[1] = guts. green_shift;
      if ( shift[1] < shift[0]) {
         xchg = shift[0];
         shift[0] = shift[1];
         shift[1] = xchg;
      }
      shift[2] = guts. blue_shift;
      if ( shift[2] < shift[0]) {
         xchg = shift[2];
         shift[2] = shift[1];
         shift[1] = shift[0];
         shift[0] = xchg;
      } else if ( shift[2] < shift[1]) {
         xchg = shift[1];
         shift[1] = shift[2];
         shift[2] = xchg;
      }

      shift_unknown = false;
   }

   return shift;
}

static void
create_rgb_to_xpixel_lut( int ncolors, const PRGBColor pal, XPixel *lut)
{
   int i;
   for ( i = 0; i < ncolors; i++) 
      lut[i] = 
            (((pal[i].r << guts. red_range  ) >> 8) << guts.   red_shift) |
            (((pal[i].g << guts. green_range) >> 8) << guts. green_shift) |
            (((pal[i].b << guts. blue_range ) >> 8) << guts.  blue_shift);
   if ( guts.machine_byte_order != guts.byte_order) 
      for ( i = 0; i < ncolors; i++) 
         lut[i] = REVERSE_BYTES_32(lut[i]);
}

static Bool
create_cache4_8( Image *img, ImageCache *cache)
{
   static Bool init = false;
   static unsigned char lut1[ NPalEntries8];
   static unsigned char lut2[ NPalEntries8];
   unsigned char *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w, ww = (w >> 1) + (w & 1);
   unsigned i;

   if ( !init) {
      init = true;
      for ( i = 0; i < NPalEntries8; i++) {
         lut1[i] = ((i & MSNibble) >> MSNibbleShift);
         lut2[i] = ((i & LSNibble) >> LSNibbleShift);
      }  
   }

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register unsigned char *d = (unsigned char*)(ls*(h-y-1)+data);
      for ( x = 0; x < ww; x++) {
	 *d++ = lut1[line[x]];
	 *d++ = lut2[line[x]];
      }
   }
   return true;
}


static Bool
create_cache4_16( Image *img, ImageCache *cache)
{
   Duplet16 lut[ NPalEntries8];
   Pixel16 lut1[ NPalEntries4];
   unsigned char *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   unsigned i;

   create_rgb_to_16_lut( NPalEntries4, img-> palette, lut1);
   for ( i = 0; i < NPalEntries8; i++) {
      lut[i]. a = lut1[(i & MSNibble) >> MSNibbleShift];
      lut[i]. b = lut1[(i & LSNibble) >> LSNibbleShift];
   }
   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Duplet16 *d = (Duplet16*)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache4_24( Image *img, ImageCache *cache)
{
   Duplet24 lut[ NPalEntries8];
   XPixel lut1[ NPalEntries4];
   unsigned char *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   unsigned i;
   int *shift = rank_rgb_shifts();

   create_rgb_to_xpixel_lut( NPalEntries4, img-> palette, lut1);
   for ( i = 0; i < NPalEntries8; i++) {
      lut[i]. a0 = (ColorComponent)((lut1[(i & MSNibble) >> MSNibbleShift] >> shift[0]) & ColorComponentMask);
      lut[i]. a1 = (ColorComponent)((lut1[(i & MSNibble) >> MSNibbleShift] >> shift[1]) & ColorComponentMask);
      lut[i]. a2 = (ColorComponent)((lut1[(i & MSNibble) >> MSNibbleShift] >> shift[2]) & ColorComponentMask);
      lut[i]. b0 = (ColorComponent)((lut1[(i & LSNibble) >> LSNibbleShift] >> shift[0]) & ColorComponentMask);
      lut[i]. b1 = (ColorComponent)((lut1[(i & LSNibble) >> LSNibbleShift] >> shift[1]) & ColorComponentMask);
      lut[i]. b2 = (ColorComponent)((lut1[(i & LSNibble) >> LSNibbleShift] >> shift[2]) & ColorComponentMask);
   }

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Duplet24 *d = (Duplet24 *)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache4_32( Image *img, ImageCache *cache)
{
   Duplet32 lut[ NPalEntries8];
   XPixel lut1[ NPalEntries4];
   unsigned char *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   unsigned i;

   create_rgb_to_xpixel_lut( NPalEntries4, img-> palette, lut1);
   for ( i = 0; i < NPalEntries8; i++) {
      lut[i]. a = lut1[(i & MSNibble) >> MSNibbleShift];
      lut[i]. b = lut1[(i & LSNibble) >> LSNibbleShift];
   }

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Duplet32 *d = (Duplet32 *)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache_equal( Image *img, ImageCache *cache)
{
   unsigned char *data;
   int y, ls, lls, h = img-> h;
   cache->image = prima_prepare_ximage( img-> w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);
   lls = (ls > img-> lineSize) ? img-> lineSize : ls;

   for ( y = h-1; y >= 0; y--) 
      memcpy( data + ls * (h - y - 1), img-> data + y*img-> lineSize,  lls);
   return true;
}

static Bool
create_cache8_8_tc( Image *img, ImageCache *cache)
{
   Pixel8 lut[ NPalEntries8];
   Pixel8 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;

   create_rgb_to_8_lut( img-> palSize, img-> palette, lut);

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel8 *d = (Pixel8*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache8_16( Image *img, ImageCache *cache)
{
   Pixel16 lut[ NPalEntries8];
   Pixel16 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;

   create_rgb_to_16_lut( img-> palSize, img-> palette, lut);

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel16 *d = (Pixel16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache8_24( Image *img, ImageCache *cache)
{
   Pixel24 lut[ NPalEntries8];
   XPixel lut1[ NPalEntries8];
   Pixel24 *data;
   int i;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   int *shift = rank_rgb_shifts();

   create_rgb_to_xpixel_lut( img-> palSize, img-> palette, lut1);
   for ( i = 0; i < NPalEntries8; i++) {
      lut[i]. a0 = (ColorComponent)((lut1[i] >> shift[0]) & ColorComponentMask);
      lut[i]. a1 = (ColorComponent)((lut1[i] >> shift[1]) & ColorComponentMask);
      lut[i]. a2 = (ColorComponent)((lut1[i] >> shift[2]) & ColorComponentMask);
   }

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel24 *d = (Pixel24*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache8_32( Image *img, ImageCache *cache)
{
   XPixel lut[ NPalEntries8];
   Pixel32 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;

   create_rgb_to_xpixel_lut( img-> palSize, img-> palette, lut);

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
   return true;
}

static Bool
create_cache24_16( Image *img, ImageCache *cache)
{
   static Pixel16 lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8];
   static Bool initialize = true;
   U16 *data;
   int x, y;
   int i;
   RGBColor pal[NPalEntries8];
   int ls;
   int h = img-> h, w = img-> w;

   if ( initialize) {
      for ( i = 0; i < NPalEntries8; i++) {
	 pal[i]. r = i; pal[i]. g = 0; pal[i]. b = 0;
      }
      create_rgb_to_16_lut( NPalEntries8, pal, lur);
      for ( i = 0; i < NPalEntries8; i++) {
	 pal[i]. r = 0; pal[i]. g = i; pal[i]. b = 0;
      }
      create_rgb_to_16_lut( NPalEntries8, pal, lug);
      for ( i = 0; i < NPalEntries8; i++) {
	 pal[i]. r = 0; pal[i]. g = 0; pal[i]. b = i;
      }
      create_rgb_to_16_lut( NPalEntries8, pal, lub);
      initialize = false;
   }

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register Pixel24 *line = (Pixel24*)(img-> data + y*img-> lineSize);
      register Pixel16 *d = (Pixel16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lub[line->a0] | lug[line->a1] | lur[line->a2];
	 line++;
      }
   }
   return true;
}

static Bool
create_cache24_32( Image *img, ImageCache *cache)
{
   static XPixel lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8];
   static Bool initialize = true;
   RGBColor pal[NPalEntries8];
   Pixel32 *data;
   int x, y;
   int i, ls;
   int h = img-> h, w = img-> w;

   if ( initialize) {
      for ( i = 0; i < NPalEntries8; i++) {
	 pal[i]. r = i; pal[i]. g = 0; pal[i]. b = 0;
      }
      create_rgb_to_xpixel_lut( NPalEntries8, pal, lur);
      for ( i = 0; i < NPalEntries8; i++) {
	 pal[i]. r = 0; pal[i]. g = i; pal[i]. b = 0;
      }
      create_rgb_to_xpixel_lut( NPalEntries8, pal, lug);
      for ( i = 0; i < NPalEntries8; i++) {
	 pal[i]. r = 0; pal[i]. g = 0; pal[i]. b = i;
      }
      create_rgb_to_xpixel_lut( NPalEntries8, pal, lub);
      initialize = false;
   }

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register Pixel24 *line = (Pixel24*)(img-> data + y*img-> lineSize);
      register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lub[line->a0] | lug[line->a1] | lur[line->a2];
	 line++;
      }
   }
   return true;
}

static Bool
create_cache1( Image* img, ImageCache *cache, int bpp)
{
   return create_cache1_1( img, cache, false);
}

static Bool
create_cache4( Image* img, ImageCache *cache, int bpp)
{
   switch (bpp) {
   case  8:     return create_cache4_8( img, cache);
   case 16:     return create_cache4_16( img, cache);
   case 24:     return create_cache4_24( img, cache);
   case 32:     return create_cache4_32( img, cache);
   default:     warn( "UAI_011: unsupported image conversion: %d => %d", 4, bpp);
   }             
   return false;
}

static Bool
create_cache8( Image* img, ImageCache *cache, int bpp)
{
   switch (bpp) {
   case 8:      
      return ( guts. visualClass == TrueColor || guts. visualClass == DirectColor) ?
               create_cache8_8_tc( img, cache) :
               create_cache_equal( img, cache);
   case 16:     return create_cache8_16( img, cache);
   case 24:     return create_cache8_24( img, cache);
   case 32:     return create_cache8_32( img, cache);
   default:     warn( "UAI_012: unsupported image conversion: %d => %d", 8, bpp);
   }
   return false;
}

static Bool
create_cache24( Image* img, ImageCache *cache, int bpp)
{
   switch (bpp) {
   case 16:     return create_cache24_16( img, cache); break;
   case 32:     return create_cache24_32( img, cache); break;
   default:     warn( "UAI_013: unsupported image conversion: %d => %d", 24, bpp);
   }
   return false;
}

static void
cache_remap_8( Image*img, ImageCache* cache)
{
   int sz = img-> h * cache-> image-> bytes_per_line_alias;
   Byte * p = cache-> image-> data_alias;
   while ( sz--) {
      *p = guts. mappingPlace[ *p];
      p++;
   }
}

static void
cache_remap_4( Image*img, ImageCache* cache)
{
   int sz = img-> h * cache-> image-> bytes_per_line_alias;
   Byte * p = cache-> image-> data_alias;
   while ( sz--) {
      *p = 
         guts. mappingPlace[(*p) & 0xf] |
        (guts. mappingPlace[((*p) & 0xf0) >> 4] << 4);
      p++;
   }
}

static void
cache_remap_1( Image*img, ImageCache* cache)
{
   int sz = img-> h * cache-> image-> bytes_per_line_alias;
   Byte * p = cache-> image-> data_alias;
   if ( guts. mappingPlace[0] == guts. mappingPlace[1]) 
      memset( p, (guts. mappingPlace[0] == 0) ? 0 : 0xff, sz);
   else if ( guts. mappingPlace[0] != 0)  
      while ( sz--) {
         *p = ~(*p);
         p++;
      }
}


ImageCache*
prima_create_image_cache( PImage img, Handle drawable, int type)
{
   PDrawableSysData IMG = X((Handle)img);
   int target_bpp;
   ImageCache *cache    = &X((Handle)img)-> image_cache;
   Bool ret;
   Handle dup = nilHandle;
   PImage pass = img;

   if ( img-> w == 0 || img-> h == 0) return nil;

   if ( img-> palette == nil) {
      warn( "UAI_014: image has no palette");
      return nil;
   }

   switch ( type) {
   case CACHE_AUTODETECT:
      type = ( drawable == nilHandle || X(drawable) == nil ||
         XT_IS_BITMAP(X(drawable)) || ( guts. idepth == 1)) ?
         CACHE_BITMAP : CACHE_PIXMAP;
      break;
   case CACHE_PIXMAP:
      if ( guts. idepth == 1) type = CACHE_BITMAP;
      break;
   case CACHE_LOW_RES:
      if ( !guts. dynamicColors) type = CACHE_PIXMAP; 
      if ( guts. idepth == 1) type = CACHE_BITMAP;
      break;
   }

   if ( drawable == nilHandle) drawable = application;

   target_bpp = ( type == CACHE_BITMAP) ? 1 : guts. idepth;
   
   if ( XT_IS_ICON(IMG)) {
      if ( cache-> icon == nil) {
         if ( !create_cache1_1( img, cache, true)) {
            return nil;
         }
      }
   } else
      cache-> icon = nil;

   if ( cache-> image != nil) {
      if ( cache-> type == type) return cache;
      destroy_ximage( cache-> image);
      cache-> image = nil;
   }

   if (( img-> type & ( imRealNumber | imComplexNumber | imTrigComplexNumber)) ||
       ( img-> type == imLong || img-> type == imShort)) {
      if ( !dup) {
         if (!(dup = img-> self-> dup(( Handle) img)))
            return nil;
      }
      pass = ( PImage) dup;
      pass-> self->resample(( Handle) pass,
         pass-> self->stats(( Handle) pass, false, isRangeLo, 0),
         pass-> self->stats(( Handle) pass, false, isRangeHi, 0),
         0, 255
      );
      pass-> self-> set_type(( Handle) pass, imByte);
   }
   
   if ( target_bpp <= 8 && img-> type != imBW) {
      int bpp, colors = 0;
      RGBColor palbuf[256], *palptr = nil;
      if ( !dup) {
         if (!(dup = img-> self-> dup(( Handle) img)))
            return nil;
      }
      pass = ( PImage) dup;
      if ( target_bpp <= 1) bpp = imbpp1; else
      if ( target_bpp <= 4) bpp = imbpp4; else bpp = imbpp8;

      if ( guts. palSize > 0 && target_bpp > 1) {
         int i, maxRank = RANK_FREE;
         if ( type == CACHE_LOW_RES) maxRank = RANK_LOCKED;
         for ( i = 0; i < guts. palSize; i++) {
            if ( guts. palette[i]. rank <= maxRank) continue;
            palbuf[colors]. r = guts. palette[i]. r;
            palbuf[colors]. g = guts. palette[i]. g;
            palbuf[colors]. b = guts. palette[i]. b;
            colors++;
            if ( colors > 255) break;
         }
         palptr = palbuf;
      }
      pass-> self-> reset( dup, bpp, palptr, colors);
   } 
     
   switch ( pass-> type & imBPP) {
   case 1:   ret = create_cache1( pass, cache, target_bpp); break;
   case 4:   ret = create_cache4( pass, cache, target_bpp); break;
   case 8:   ret = create_cache8( pass, cache, target_bpp); break;
   case 24:  ret = create_cache24(pass, cache, target_bpp); break;
   default:  
      warn( "UAI_015: unsupported image type");
      return nil;
   }
   if ( !ret) {
      if ( dup) Object_destroy(dup);
      return nil;
   }

   if (( guts. palSize > 0) && (( pass-> type & imBPP) != 24)) {
      int i, maxRank = RANK_FREE;
      Byte * p = X((Handle)img)-> palette;
      if ( type == CACHE_LOW_RES) {
         drawable = application;
         maxRank = RANK_LOCKED; 
      }
               
      for ( i = 0; i < pass-> palSize; i++) {
         int j = guts. mappingPlace[i] = prima_color_find( nilHandle,
            RGB_COMPOSITE( 
              pass-> palette[i].r,
              pass-> palette[i].g,
              pass-> palette[i].b
            ), -1, nil, maxRank);

         if ( p && ( prima_lpal_get( p, j) == RANK_FREE)) 
             prima_color_add_ref(( Handle) img, j, RANK_LOCKED);
      }
      for ( i = pass-> palSize; i < 256; i++) guts. mappingPlace[i] = 0;
      
      switch(target_bpp){
      case 8: if ((pass-> type & imBPP) != 1) cache_remap_8( img, cache); break;
      case 4: if ((pass-> type & imBPP) != 1) cache_remap_4( img, cache); break;
      case 1: cache_remap_1( img, cache); break;
      default: warn("UAI_019: palette is not supported");
      }
   }

   if ( dup) Object_destroy(dup);
   cache-> type = type;
   return cache;
}

Bool
prima_create_icon_pixmaps( Handle self, Pixmap *xor, Pixmap *and)
{
   Pixmap p1, p2;
   PIcon icon = PIcon(self);
   ImageCache *cache;
   GC gc;
   XGCValues gcv;
   
   cache = prima_create_image_cache((PImage)icon, nilHandle, CACHE_BITMAP);
   if ( !cache) return false;
   p1 = XCreatePixmap( DISP, guts. root, icon-> w, icon-> h, 1);
   p2 = XCreatePixmap( DISP, guts. root, icon-> w, icon-> h, 1);
   XCHECKPOINT;
   if ( p1 == None || p2 == None) {
      if (p1 != None) XFreePixmap( DISP, p1);
      if (p2 != None) XFreePixmap( DISP, p2);
      return false;
   }
   gcv. graphics_exposures = false;
   gc = XCreateGC( DISP, p1, GCGraphicsExposures, &gcv);
   XSetForeground( DISP, gc, 0);
   XSetBackground( DISP, gc, 1);
   prima_put_ximage( p2, gc, cache->icon, 0, 0, 0, 0, icon-> w, icon-> h);
   XSetForeground( DISP, gc, 1);
   XSetBackground( DISP, gc, 0);
   prima_put_ximage( p1, gc, cache->image, 0, 0, 0, 0, icon-> w, icon-> h);
   XFreeGC( DISP, gc);
   *xor = p1;
   *and = p2;
   return true;
}

static Bool
put_pixmap( Handle self, Handle pixmap, int dst_x, int dst_y, int src_x, int src_y, int w, int h, int rop)
{
   DEFXX;
   PDrawableSysData YY = X(pixmap);
   XGCValues gcv;
   int func;

   /* XXX currently can X-fail in several cases */
   SHIFT( dst_x, dst_y);

   if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) 
      warn( "UAI_016: error querying GC values");
   XCHECKPOINT;
   func = prima_rop_map( rop);
   if ( func != gcv. function)
      XSetFunction( DISP, XX-> gc, func);

   XCopyArea( DISP, YY-> gdrawable, XX-> gdrawable, XX-> gc,
              src_x, YY->size.y - src_y - h,
              w, h,
              dst_x, XX->size.y - dst_y - h);

   if ( func != gcv. function)
      XSetFunction( DISP, XX-> gc, gcv. function);
   XCHECKPOINT;
   XFLUSH;
   return true;
}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   DEFXX;
   PImage img = PImage( image);
   int func, ofunc;
   XGCValues gcv;
   ImageCache *cache = nil;
   Bool mono, icon = false, tempResult = false;
   PrimaXImage * result = nil;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if ( xFrom >= img-> w || yFrom >= img-> h) return false;
   if ( xFrom + xLen > img-> w) xLen = img-> w - xFrom;
   if ( yFrom + yLen > img-> h) yLen = img-> h - yFrom;
   if ( xLen <= 0 || yLen <= 0) return false;
  
   if ( XT_IS_DBM(X(image)) || PObject( image)-> options. optInDraw) {
      HV * profile;
      XImage * i;
      if (
           ( XT_IS_BITMAP(X(image)) && ( XT_IS_BITMAP(X(self)) || ( guts. depth == 1))) ||
           ( XT_IS_PIXMAP(X(image)) && ( XT_IS_PIXMAP(X(self)) || XT_IS_WIDGET(X(self)))) 
         )
         return put_pixmap( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);

      if ( XT_IS_BITMAP(X(image))) {
         int j, ls;
         Byte * src, * dst;
         mono = true;
         if ( !( i = XGetImage( DISP, X(image)-> gdrawable, 
             xFrom, img-> h - yFrom - yLen, xLen, yLen, 1, XYPixmap)))
            return false;
         if ( !( result = prima_prepare_ximage( xLen, yLen, true))) {
            XDestroyImage( i);
            return false;
         }
         xFrom = 0;
         yFrom = img-> h - yLen;
         tempResult = true;
         ls = ( i-> bytes_per_line > result-> bytes_per_line_alias) ? 
            result-> bytes_per_line_alias : i-> bytes_per_line;
         src = ( unsigned char *) i-> data;
         dst = result-> data_alias;
         for ( j = 0; j < yLen; j++, src += i-> bytes_per_line, dst += result-> bytes_per_line_alias) 
            memcpy( dst, src, ls);
         XCHECKPOINT;
         XDestroyImage( i);
      } else {
         Handle obj;
         Bool ret = false;
         if ( !( i = XGetImage( DISP, X(image)-> gdrawable, 
                xFrom, img-> h - yFrom - yLen, xLen, yLen, AllPlanes, ZPixmap)))
            return false;
         obj = Object_create("Prima::Image", profile = newHV());
         CImage( obj)-> create_empty( obj, xLen, yLen, guts. qdepth);
         if ( prima_query_image( obj, i)) {
            CImage( obj)-> set_type( obj, imBW);
            ret = apc_gp_put_image( self, obj, x, y, 0, 0, xLen, yLen, rop);
         }
         sv_free((SV*) profile);
         Object_destroy( obj);
         XCHECKPOINT;
         XDestroyImage( i);
         return ret;
      }
   } else {
      mono = (( img-> type & imBPP) == 1) || ( guts. idepth == 1);
      icon = XT_IS_ICON(X(image));
      cache = prima_create_image_cache( img, self, CACHE_AUTODETECT);
      if ( !cache) return false;
      result = cache-> image;
   }
   
   if ( guts. dynamicColors) {
      int i;
      for ( i = 0; i < guts. palSize; i++) 
         if (( wlpal_get( image, i) == RANK_FREE) &&
             ( wlpal_get( self,  i) != RANK_FREE))
            prima_color_add_ref( self, i, RANK_LOCKED);
   }
   SHIFT( x, y);
   if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) 
      warn( "UAI_016: error querying GC values");
   ofunc = gcv. function;
   if ( icon) {
      func = GXand;
      if ( XT_IS_BITMAP(X(self))) {
         XSetForeground( DISP, XX-> gc, 1);
         XSetBackground( DISP, XX-> gc, 0);
      } else {
         XSetForeground( DISP, XX-> gc, 0xFFFFFFFF);
         XSetBackground( DISP, XX-> gc, 0x00000000);
      }
      XX-> flags. brush_fore = 0;
      XX-> flags. brush_back = 0;
      
      if ( func != ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
      prima_put_ximage( XX-> gdrawable, XX-> gc, cache->icon,
                        xFrom, img-> h - yFrom - yLen,
                        x, REVERT(y) - yLen + 1, xLen, yLen);
      func = GXxor;
      if ( func == ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
   } else {
      func = prima_rop_map( rop);
   }

   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, func);
   
   if ( XT_IS_BITMAP(X(self))) {
      XSetForeground( DISP, XX-> gc, 1);
      XSetBackground( DISP, XX-> gc, 0);
      XX-> flags. brush_fore = XX-> flags. brush_back = 0;
   } else if ( mono) {
      unsigned long fore, back;
      if ( XT_IS_DBM(X(image))) {
         fore = XX-> back. primary;
         back = XX-> fore. primary;
      } else {
         if ( guts. palSize > 0) {
            fore = prima_color_find( self,
                RGB_COMPOSITE( img-> palette[1].r, img-> palette[1].g, img-> palette[1].b),
                -1, nil, RANK_NORMAL);
            back = prima_color_find( self,
                RGB_COMPOSITE( img-> palette[0].r, img-> palette[0].g, img-> palette[0].b),
                -1, nil, RANK_NORMAL);
         } else {
            fore = 
               (((img-> palette[1].r << guts. red_range  ) >> 8) << guts.   red_shift) |
               (((img-> palette[1].g << guts. green_range) >> 8) << guts. green_shift) |
               (((img-> palette[1].b << guts. blue_range ) >> 8) << guts.  blue_shift);
            back = 
               (((img-> palette[0].r << guts. red_range  ) >> 8) << guts.   red_shift) |
               (((img-> palette[0].g << guts. green_range) >> 8) << guts. green_shift) |
               (((img-> palette[0].b << guts. blue_range ) >> 8) << guts.  blue_shift);
         }
      }
         
      XSetForeground( DISP, XX-> gc, fore);
      XSetBackground( DISP, XX-> gc, back);
      XX-> flags. brush_fore = XX-> flags. brush_back = 0;
   }
   prima_put_ximage( XX-> gdrawable, XX-> gc, result,
                     xFrom, img-> h - yFrom - yLen,
                     x, REVERT(y) - yLen + 1, xLen, yLen);
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, ofunc);

   if ( tempResult)
      prima_free_ximage( result);
   XFLUSH;
   return true;
}

Bool
apc_image_begin_paint( Handle self)
{
   DEFXX;
   PImage img = PImage( self);
   int icon = XX-> type. icon;
   Bool bitmap = (img-> type  == imBW) || ( guts. idepth == 1);

   if ( !DISP) return false;
   if (img-> w == 0 || img-> h == 0) return false;

   XX-> gdrawable = XCreatePixmap( DISP, guts. root, img-> w, img-> h,
                                   bitmap ? 1 : guts. depth);
   XX-> type.pixmap = !bitmap;
   XX-> type.bitmap = !!bitmap;
   XCHECKPOINT;
   XX-> type. icon = 0;
   prima_prepare_drawable_for_painting( self, false);
   PObject( self)-> options. optInDraw = 0;
   apc_gp_put_image( self, self, 0, 0, 0, 0, img-> w, img-> h, ropCopyPut);
   /*                ^^^^^ ^^^^    :-)))  */
   PObject( self)-> options. optInDraw = 1;
   XX-> type. icon = icon;
   return true;
}

static void
convert_16_to_24( XImage *i, PImage img)
{
   int y, x, h, w;
   Pixel16 *d;
   register Pixel24 *line;

   /*
      Compensate less than 8-bit true-color memory layout depth converted into
      real 8 bit, a bit slower but more error-prone in general sense. Although
      Prima::gp-problems advises not to check against 0xffffff as white, since
      white is 0xf8f8f8 on 15-bit displays for example, it is not practical to
      use this check fro example when a RGB image is to be converted into a
      low-palette image with RGB(0xff,0xff,0xff) expected and desirable palette
      slot value.
    */

   int rmax = 0xff & ( 0xff << ( 8 - guts. red_range));
   int gmax = 0xff & ( 0xff << ( 8 - guts. green_range));
   int bmax = 0xff & ( 0xff << ( 8 - guts. blue_range));
   if ( rmax == 0 ) rmax = 0xff;
   if ( gmax == 0 ) gmax = 0xff;
   if ( bmax == 0 ) bmax = 0xff;

   h = img-> h; w = img-> w;
   for ( y = 0; y < h; y++) {
      d = (Pixel16 *)(i-> data + (h-y-1)*i-> bytes_per_line);
      line = (Pixel24*)(img-> data + y*img-> lineSize);
      if ( guts.machine_byte_order != guts.byte_order) {
         for ( x = 0; x < w; x++) {
            register Pixel16 dd = REVERSE_BYTES_16(*d);
            line-> a0 = (((dd & guts. visual. blue_mask)  >> guts. blue_shift) << 8) >> guts. blue_range; 
            line-> a1 = (((dd & guts. visual. green_mask) >> guts. green_shift) << 8) >> guts. green_range;
            line-> a2 = (((dd & guts. visual. red_mask)   >> guts. red_shift) << 8) >> guts. red_range;
            if ( line-> a0 == bmax) line-> a0 = 0xff;
            if ( line-> a1 == gmax) line-> a1 = 0xff;
            if ( line-> a2 == rmax) line-> a2 = 0xff;
            line++; d++;
         }
      } else {
         for ( x = 0; x < w; x++) {
            line-> a0 = (((*d & guts. visual. blue_mask)  >> guts. blue_shift) << 8) >> guts. blue_range; 
            line-> a1 = (((*d & guts. visual. green_mask) >> guts. green_shift) << 8) >> guts. green_range;
            line-> a2 = (((*d & guts. visual. red_mask)   >> guts. red_shift) << 8) >> guts. red_range;
            if ( line-> a0 == bmax) line-> a0 = 0xff;
            if ( line-> a1 == gmax) line-> a1 = 0xff;
            if ( line-> a2 == rmax) line-> a2 = 0xff;
            line++; d++;
         }
      }
   }
}

static void
convert_32_to_24( XImage *i, PImage img)
{
   int y, x, h, w;
   Pixel32 *d, dd;
   Pixel24 *line;

   h = img-> h; w = img-> w;
   if ( guts.machine_byte_order != guts.byte_order) {
      for ( y = 0; y < h; y++) {
         d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
         line = (Pixel24*)(img-> data + y*img-> lineSize);
         for ( x = 0; x < w; x++) {
            dd = REVERSE_BYTES_32(*d);
            line-> a0 = (((dd & guts. visual. blue_mask)  >> guts. blue_shift) << 8) >> guts. blue_range; 
            line-> a1 = (((dd & guts. visual. green_mask) >> guts. green_shift) << 8) >> guts. green_range;
            line-> a2 = (((dd & guts. visual. red_mask)   >> guts. red_shift) << 8) >> guts. red_range;
            d++; line++;
         }
      }
   } else {
      for ( y = 0; y < h; y++) {
         d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
         line = (Pixel24*)(img-> data + y*img-> lineSize);
         for ( x = 0; x < w; x++) {
            line-> a0 = (((*d & guts. visual. blue_mask)  >> guts. blue_shift) << 8) >> guts. blue_range; 
            line-> a1 = (((*d & guts. visual. green_mask) >> guts. green_shift) << 8) >> guts. green_range;
            line-> a2 = (((*d & guts. visual. red_mask)   >> guts. red_shift) << 8) >> guts. red_range;
            d++; line++;
         }
      }
   }
}

static void
convert_equal_paletted( XImage *i, PImage img)
{
   int y, w, h;
   Pixel8 *d, *line;
   XColor xc[256];
   
   h = img-> h;
   w = ( img-> lineSize < i-> bytes_per_line) ? img-> lineSize : i-> bytes_per_line;
   d = ( Pixel8*)(i-> data + (h-1) * i-> bytes_per_line);
   line = (Pixel8*)img-> data;
   bzero( line, img-> dataSize);
   for ( y = 0; y < h; y++) {
      memcpy( line, d, img-> w);
      d -= i-> bytes_per_line;
      line += img-> lineSize;
   }
   for ( y = 0; y < 256; y++) guts. mappingPlace[y] = -1;
   for ( y = 0; y < img-> dataSize; y++)
      guts. mappingPlace[ img-> data[y]] = 0;
   for ( y = 0; y < guts. palSize; y++) xc[y]. pixel = y;
   XQueryColors( DISP, guts. defaultColormap, xc, guts. palSize);
   
/* XXX if ( guts. bit_order != MSBFirst) prima_mirror_bytes( img-> data, img-> dataSize); */

   img-> palSize = 0;
   for ( y = 0; y < 256; y++) 
      if ( guts. mappingPlace[y] == 0) {
         img-> palette[img-> palSize]. r = xc[y].red/256;
         img-> palette[img-> palSize]. g = xc[y].green/256;
         img-> palette[img-> palSize]. b = xc[y].blue/256;
         guts. mappingPlace[y] = img-> palSize++;
      }

   for ( y = 0; y < img-> dataSize; y++)
      img-> data[y] = guts. mappingPlace[ img-> data[y]]; 
}

Bool
prima_query_image( Handle self, XImage * i)
{
   PImage img = PImage( self);
   int target_depth = ( img-> type == imBW) ? 1 : guts. qdepth;

   if (( img-> type & imBPP) != target_depth) 
      CImage( self)-> create_empty( self, img-> w, img-> h, target_depth);
   
   X(self)-> size. x = img-> w;
   X(self)-> size. y = img-> h;

   if ( target_depth == 1) {
      prima_copy_xybitmap( img-> data, (Byte*)i-> data, img-> w, img-> h, img-> lineSize, i-> bytes_per_line);
   } else {
     switch ( guts. idepth) {
     case 8:
        switch ( target_depth) {
        case 4:
           CImage( self)-> create_empty( self, img-> w, img-> h, 8);
        case 8:
           convert_equal_paletted( i, img);
           break;
        default: goto slurp_image_unsupported_depth;
        }
        break;
     case 16:
        switch ( target_depth) {
        case 24:
           convert_16_to_24( i, img);
           break;
        default: goto slurp_image_unsupported_depth;
        }
        break;
     case 32:
        switch ( target_depth) {
        case 24:
           convert_32_to_24( i, img);
           break;
        default: goto slurp_image_unsupported_depth;
        }
        break;
slurp_image_unsupported_depth:
     default:
        warn("UAI_023: unsupported backing image conversion from %d to %d\n", guts.idepth, target_depth);
        return false;
     }
   }
   return true;
}   
   
Bool
prima_std_query_image( Handle self, Pixmap px)
{
   XImage * i;
   Bool mono = PImage(self)-> type == imBW || guts. depth == 1;
   Bool ret;
   if (!( i = XGetImage( DISP, px, 0, 0, 
       PImage(self)-> w, PImage( self)-> h, 
       mono ? 1 : AllPlanes, mono ? XYPixmap : ZPixmap)))
      return false;
   XCHECKPOINT;
   ret = prima_query_image( self, i);
   XDestroyImage( i);
   return ret;
}

Pixmap
prima_std_pixmap( Handle self, int type)
{
   Pixmap px;
   XGCValues gcv;
   GC gc;
   PImage img = ( PImage) self;
   unsigned long fore, back;

   ImageCache * xi = prima_create_image_cache(( PImage) self, nilHandle, type);
   if ( !xi) return nilHandle;
   
   px = XCreatePixmap( DISP, guts. root, img-> w, img-> h, 
       ( type == CACHE_BITMAP) ? 1 : guts. depth);
   if ( !px) return nilHandle;
   
   gcv. graphics_exposures = false;
   gc = XCreateGC( DISP, guts. root, GCGraphicsExposures, &gcv);
   if ( guts. palSize > 0) {
      fore = prima_color_find( self,
          RGB_COMPOSITE( img-> palette[1].r, img-> palette[1].g, img-> palette[1].b),
          -1, nil, RANK_NORMAL);
      back = prima_color_find( self,
          RGB_COMPOSITE( img-> palette[0].r, img-> palette[0].g, img-> palette[0].b),
          -1, nil, RANK_NORMAL);
   } else {
      fore = 
         (((img-> palette[1].r << guts. red_range  ) >> 8) << guts.   red_shift) |
         (((img-> palette[1].g << guts. green_range) >> 8) << guts. green_shift) |
         (((img-> palette[1].b << guts. blue_range ) >> 8) << guts.  blue_shift);
      back = 
         (((img-> palette[0].r << guts. red_range  ) >> 8) << guts.   red_shift) |
         (((img-> palette[0].g << guts. green_range) >> 8) << guts. green_shift) |
         (((img-> palette[0].b << guts. blue_range ) >> 8) << guts.  blue_shift);
   }
      
   XSetForeground( DISP, gc, fore);
   XSetBackground( DISP, gc, back);
   prima_put_ximage( px, gc, xi->image, 0, 0, 0, 0, img-> w, img-> h);
   XFreeGC( DISP, gc);
   return px;
}

Bool
apc_image_end_paint( Handle self)
{
   DEFXX;
   prima_std_query_image( self, XX-> gdrawable);
   prima_cleanup_drawable_after_painting( self);
   if ( XX-> gdrawable) {
      XFreePixmap( DISP, XX-> gdrawable);
      XCHECKPOINT;
      XX-> gdrawable = 0;
   }
   clear_caches( self);
   return true;
}

/*
   static void
   reverse_buffer_bytes_32( void *buffer, size_t bufsize)
   {
      U32 *buf = buffer;

      if ( bufsize % 4) croak( "UAI_018: expect bufsize % 4 == 0");
      bufsize /= 4;

      while ( bufsize) {
         *buf = REVERSE_BYTES_32(*buf);
         buf++;
         bufsize--;
      }
   }
*/

typedef struct {
   Fixed count;
   Fixed step;
   int source;
   int last;
} StretchSeed;

#define ABS(x) (((x)<0)?(-(x)):(x))

static Bool mbsInitialized = false;
static Byte set_bits[ByteValues];
static Byte clear_bits[ByteValues];

static void mbs_init_bits(void)
{
   if ( !mbsInitialized) {
      int i;
      if ( guts.bit_order == MSBFirst) {
         Byte * mirrored_bits = mirror_bits();
         for ( i = 0; i < ByteValues; i++) {
            set_bits[i]   = mirrored_bits[1 << (i%ByteBits)];
            clear_bits[i] = ~mirrored_bits[(1 << (i%ByteBits))];
         }
      } else {
         for ( i = 0; i < ByteValues; i++) {
            set_bits[i]   = 1 << (i%ByteBits);
            clear_bits[i] = ~(1 << (i%ByteBits));
         }
      }
      mbsInitialized = true;
   }   
} 

static void 
mbs_mono_in( Byte * srcData, Byte * dstData, Bool xreverse, 
    int targetwidth, Fixed step, Fixed count,int first, int last, int targetLineSize )
{
    int x   = xreverse ? targetwidth - 1 : 0;
    int inc = xreverse ? -1 : 1;
    if ( srcData[first/ByteBits] & set_bits[LOWER_BYTE(first)])
       dstData[x/ByteBits] |= set_bits[LOWER_BYTE(x)];
    else
       dstData[x/ByteBits] &= clear_bits[LOWER_BYTE(x)];
    
    x += inc;
    targetwidth--;
    while ( targetwidth) {
       if ( count.i.i > last) {
           if ( srcData[first/ByteBits] & set_bits[LOWER_BYTE(first)])
              dstData[x/ByteBits] |= set_bits[LOWER_BYTE(x)];
           else
              dstData[x/ByteBits] &= clear_bits[LOWER_BYTE(x)];
           x += inc;
           last = count.i.i;
           targetwidth--;
       }
       count.l += step.l;
       first++;
    }
}   

static void 
mbs_mono_out( Byte * srcData, Byte * dstData, Bool xreverse,   
    int targetwidth, Fixed step, Fixed count,int first, int last, int targetLineSize)
{                                                                       
   int x   = xreverse ? ( targetwidth - 1) : 0;
   int inc = xreverse ? -1 : 1;
   while ( targetwidth) {
      if ( count.i.i > last) {
         first++;
         last = count.i.i;
      }
      count.l += step.l;
      if ( srcData[first/ByteBits] & set_bits[LOWER_BYTE(first)])
         dstData[x/ByteBits] |= set_bits[LOWER_BYTE(x)];
      else
         dstData[x/ByteBits] &= clear_bits[LOWER_BYTE(x)];
      x += inc;
      targetwidth--;
   }   
}   


#define BS_BYTEEXPAND( type)                                                        \
static void mbs_##type##_out( type * srcData, type * dstData, Bool xreverse,    \
    int targetwidth, Fixed step, Fixed count,int first, int last, int targetLineSize)\
{                                                                       \
   int x   = xreverse ? ( targetwidth - 1) : 0;                         \
   int inc = xreverse ? -1 : 1;                                         \
   while ( targetwidth) {                                               \
      if ( count.i.i > last) {                                          \
         first++;                                                       \
         last = count.i.i;                                              \
      }                                                                 \
      count.l += step.l;                                                \
      dstData[x] = srcData[first];                                      \
      x += inc;                                                         \
      targetwidth--;                                                    \
   }                                                                    \
}   

#define BS_BYTEIMPACT( type)                                                        \
static void mbs_##type##_in( type * srcData, type * dstData, Bool xreverse,    \
    int targetwidth, Fixed step, Fixed count, int first, int last, int targetLineSize)    \
{                                                                       \
    int x   = xreverse ? targetwidth - 1 : 0;                           \
    int inc = xreverse ? -1 : 1;                                        \
    dstData[x] = srcData[first];                                        \
    x += inc;                                                           \
    targetwidth--;                                                      \
    while ( targetwidth) {                                              \
       if ( count.i.i > last) {                                         \
           dstData[x] = srcData[first];                                 \
           x += inc;                                                    \
           last = count.i.i;                                            \
           targetwidth--;                                               \
       }                                                                \
       count.l += step.l;                                               \
       first++;                                                         \
    }                                                                   \
}   

BS_BYTEEXPAND( Pixel8)
BS_BYTEEXPAND( Pixel16)
BS_BYTEEXPAND( Pixel24)
BS_BYTEEXPAND( Pixel32)

BS_BYTEIMPACT( Pixel8)
BS_BYTEIMPACT( Pixel16)
BS_BYTEIMPACT( Pixel24)
BS_BYTEIMPACT( Pixel32)


static void mbs_copy( Byte * srcData, Byte * dstData, Bool xreverse,  
    int targetwidth, Fixed step, Fixed count, int first, int last, int targetLineSize)
{
   memcpy( dstData, srcData, targetLineSize);
}   


static void
stretch_calculate_seed( int ssize, int tsize,
                        int *clipstart, int *clipsize,
                        StretchSeed *seed)
/*
   ARGUMENTS:

   INP  int ssize       specifies source 1d size
   INP  int tsize       specifies target size
   I/O  int *clipstart  on input, desired start of clipped stretched image
                        on output, adjusted value
   I/O  int *clipsize   on input, desired size of clipped region
                        on output, adjusted value
   OUT  StretchSeed *seed
                        calculated seed values, should be passed without
                        modification to the actual stretch routine
 */
{
   Fixed count;
   Fixed step;
   int last;
   int asize;
   int cstart;
   int cend;
   int s;
   int t;
   int dt;

   count. l = 0;
   s = 0;
   asize  =  ABS(tsize);
   cstart = *clipstart;
   cend   =  cstart + *clipsize;
   t      =  0;
   dt     =  1;
   if ( cstart < 0)          cstart = 0;
   if ( cend > asize)        cend = asize;
   
   if ( asize < ssize) {
      step. l = (double) asize / ssize * UINT16_PRECISION;
      last    = -1;
      while ( t != cend) {
         if ( count.i.i > last) {
            last = count.i.i;
            if ( t == cstart) {
               seed-> count  = count;
               seed-> step   = step;
               seed-> source = s;
               seed-> last   = last;
            }
            t += dt;
         }
         count.l += step.l;
         s++;
      }
   } else {
      step. l = (double) ssize / asize * UINT16_PRECISION;
      last    = 0;
      while ( t != cend) {
         if ( count.i.i > last) {
            s++;
            last = count.i.i;
         }
         if ( t == cstart) {
            seed-> count  = count;
            seed-> step   = step;
            seed-> source = s;
            seed-> last   = last;
         }
         count.l += step.l;
         t += dt;
      }
   }
   *clipstart = cstart;
   *clipsize  = cend - cstart;
}

typedef void mStretchProc( void * srcData, void * dstData, Bool xreverse, 
   int targetwidth, Fixed step, Fixed count, int first, int last, int targetLineSize);


static PrimaXImage *
do_stretch( Handle self, PrimaXImage *cache,
            int src_x, int src_y, int src_w, int src_h,
            int dst_x, int dst_y, int dst_w, int dst_h,
            int *x, int *y, int *w, int *h)
{
   Byte *data;
   PrimaXImage *stretch;
   StretchSeed xseed, yseed;
   XRectangle cr;
   int bpp;
   int sls;
   int tls;
   int xclipstart, xclipsize;
   int yclipstart, yclipsize;

   prima_gp_get_clip_rect( self, &cr, 1);
   xclipstart = cr. x - dst_x;
   xclipsize = cr. width;
   yclipstart = cr. y - dst_y;
   yclipsize = cr. height;
   bpp = ( cache-> image-> format == XYBitmap || cache-> image-> format == XYPixmap) ? 1 : guts. idepth;
   if ( xclipstart + xclipsize <= 0 || yclipstart + yclipsize <= 0) return nil;
   stretch_calculate_seed( src_w, dst_w, &xclipstart, &xclipsize, &xseed);
   stretch_calculate_seed( src_h, dst_h, &yclipstart, &yclipsize, &yseed);
   if ( xclipsize <= 0 || yclipsize <= 0) return nil;
   stretch = prima_prepare_ximage( xclipsize, yclipsize, bpp == 1);
   if ( !stretch) return false;

   tls = get_ximage_bytes_per_line( stretch);
   sls = get_ximage_bytes_per_line( cache);
   data = get_ximage_data( stretch);
   
   {
      Byte * last_source = nil;
      Byte * srcData = ( Byte*) get_ximage_data(cache) + ( src_y + yseed. source) * sls;
      Byte * dstData = data;
      Bool   xshrink = dst_w < 0 ? -dst_w < src_w : dst_w < src_w;
      Bool   yshrink = dst_h < 0 ? -dst_h < src_h : dst_h < src_h;
      mStretchProc * proc = ( mStretchProc*) nil;
      int targetwidth  = xclipsize;
      int targetheight = yclipsize;
      int copyBytes = tls > sls ? sls : tls;
      switch ( bpp) {
      case 1:
          srcData += src_x / ByteBits;
          xseed. source += src_x % ByteBits;
          bzero( dstData, targetheight * tls);
          mbs_init_bits();
          proc = ( mStretchProc * )( xshrink ? mbs_mono_in : mbs_mono_out);
          break;
      case 8: 
          srcData += src_x * sizeof( Pixel8);
          proc = ( mStretchProc * )( xshrink ? mbs_Pixel8_in : mbs_Pixel8_out);
          if ( dst_w == src_w) proc = ( mStretchProc *) mbs_copy;
          break;          
      case 16: 
          srcData += src_x * sizeof( Pixel16);
          proc = ( mStretchProc * )( xshrink ? mbs_Pixel16_in : mbs_Pixel16_out);
          if ( dst_w == src_w) proc = ( mStretchProc *) mbs_copy;
          break;
      case 24: 
          srcData += src_x * sizeof( Pixel24); 
          proc = ( mStretchProc * )( xshrink ? mbs_Pixel24_in : mbs_Pixel24_out);
          if ( dst_w == src_w) proc = ( mStretchProc *) mbs_copy;
          break;
      case 32:    
          srcData += src_x * sizeof( Pixel32); 
          proc = ( mStretchProc * )( xshrink ? mbs_Pixel32_in : mbs_Pixel32_out);
          if ( dst_w == src_w) proc = ( mStretchProc *) mbs_copy;
          break;
      default:
          warn( "UAI_020: %d-bit stretch is not yet implemented", bpp); 
          prima_free_ximage( stretch);
          return nil;
      }   
      
      if ( dst_h < 0) {
         dstData += ( yclipsize - 1) * tls;
         tls = -tls;
      }   

      if ( yshrink) {
         proc( srcData, dstData, dst_w < 0, targetwidth, 
            xseed.step, xseed.count, xseed.source, xseed.last, copyBytes);
         dstData += tls;
         targetheight--;
         while ( targetheight) {
            if ( yseed.count.i.i > yseed.last) {  
               proc( srcData, dstData, dst_w < 0, targetwidth, 
                     xseed.step, xseed.count, xseed.source, xseed.last, copyBytes);
               dstData += tls;
               yseed. last = yseed.count.i.i;
               targetheight--;
            }   
            yseed.count.l += yseed.step.l;
            srcData += sls;
         }   
      } else {
         while ( targetheight) {
            if ( yseed.count.i.i > yseed.last) {
               srcData += sls;
               yseed.last = yseed.count.i.i;
            }   
            yseed.count.l += yseed.step.l;
            if ( last_source == srcData) 
               memcpy( dstData, dstData - tls, ABS(tls)); 
            else {
               last_source = srcData;
               proc( srcData, dstData, dst_w < 0, targetwidth, 
                     xseed.step, xseed.count, xseed.source, xseed. last, copyBytes);
            }   
            dstData += tls;
            targetheight--;
         }   
      }   
   }   
   *x = dst_x + xclipstart;
   *y = dst_y + yclipstart;
   *w = xclipsize;
   *h = yclipsize;
   return stretch;
}

Bool
apc_gp_stretch_image( Handle self, Handle image,
		      int dst_x, int dst_y, int src_x, int src_y,
		      int dst_w, int dst_h, int src_w, int src_h, int rop)
{
   DEFXX;
   PImage img = PImage( image);
   PrimaXImage *stretch;
   ImageCache *cache = nil;
   int func, ofunc;
   XGCValues gcv;
   int x, y, w, h;
   Bool mono, icon = false, tempResult = false;
   PrimaXImage * result;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if ( src_h < 0) {
      src_h = -src_h;
      dst_h = -dst_h;
   }   
   if ( src_w < 0) {
      src_w = -src_w;
      dst_w = -dst_w;
   }   
   if ( abs(src_x) >= img-> w) return false;
   if ( abs(src_y) >= img-> h) return false;
   if ( src_w == 0 || src_h == 0) return false;
   if ( src_x < 0) {
      dst_x -= src_x * dst_w / src_w;
      dst_w += src_x * dst_w / src_w;
      src_w += src_x;
      src_x = 0;
   }
   if ( src_y < 0) {
      dst_y -= src_y * dst_h / src_h;
      dst_h += src_y * dst_h / src_h;
      src_h += src_y;
      src_y = 0;
   }
   if ( src_x + src_w > img-> w) {
      dst_w = (img-> w - src_x) * dst_w / src_w;
      src_w = img-> w - src_x;
   }
   if ( src_y + src_h > img-> h) {
      dst_h = (img-> h - src_y) * dst_h / src_h;
      src_h = img-> h - src_y;
   }
   if ( src_w == 0 || src_h == 0) return false;
   if ( XT_IS_DBM(X(image)) || PObject( image)-> options. optInDraw) {
      XImage * i;
      if ( XT_IS_PIXMAP(X(image)) && XT_IS_BITMAP(X(self))) {
         Handle obj;
         HV * profile;
         if ( !( i = XGetImage( DISP, X(image)-> gdrawable, 
                src_x, img-> h - src_y - src_h, src_w, src_h, 
                AllPlanes, ZPixmap)))
            return false;
         obj = Object_create("Prima::Image", profile = newHV());
         CImage( obj)-> create_empty( obj, src_w, src_h, guts. qdepth);
         if ( prima_query_image( obj, i)) 
             apc_gp_stretch_image( self, obj, dst_x, dst_y, 0, 0, dst_w, dst_h, src_w, src_h, rop);
         sv_free((SV*) profile);
         Object_destroy( obj);
         return true;
      } else {
         mono = XT_IS_BITMAP(X(image));
         if ( !( i = XGetImage( DISP, X(image)-> gdrawable, 
                src_x, img-> h - src_y - src_h, src_w, src_h, 
                mono ? 1 : AllPlanes, mono ? XYPixmap : ZPixmap)))
             return false;
         tempResult = true;  
         src_x = 0;
         src_y = img-> h - ABS( src_h);
         if ( !( result = malloc( sizeof( PrimaXImage)))) {
            XDestroyImage( i);
            return false;
         }
         bzero( result, sizeof( PrimaXImage));
         result-> image = i;
         result-> data_alias = i-> data; 
         result-> bytes_per_line_alias = i-> bytes_per_line; 
      }
   } else {
      mono = (( img-> type & imBPP) == 1) || ( guts. idepth == 1);
      cache = prima_create_image_cache( img, self, CACHE_AUTODETECT);
      if ( !cache) return false;
      result = cache-> image;
      icon   = cache-> icon != nil;
   }


   if ( guts. dynamicColors) {
      int i;
      for ( i = 0; i < guts. palSize; i++) 
         if (( wlpal_get( image, i) == RANK_FREE) &&
             ( wlpal_get( self,  i) != RANK_FREE))
            prima_color_add_ref( self, i, RANK_LOCKED);
   }
  
   SHIFT( dst_x, dst_y);
   dst_y = XX->size.y - dst_y - ABS(dst_h);
   src_y = img-> h - src_y - ABS(src_h);

   if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) 
      warn( "UAI_022: error querying GC values");
   ofunc = gcv. function;

   if ( icon) {
      func = GXxor;
      stretch = do_stretch(self, cache-> icon,
                           src_x, src_y, src_w, src_h,
                           dst_x, dst_y, dst_w, dst_h,
                           &x, &y, &w, &h
                          );
      if ( stretch != nil) {
         func = GXand;
         if ( XT_IS_BITMAP(X(self))) {
            XSetForeground( DISP, XX-> gc, 1);
            XSetBackground( DISP, XX-> gc, 0);
         } else {
            XSetForeground( DISP, XX-> gc, 0xFFFFFFFF);
            XSetBackground( DISP, XX-> gc, 0x00000000);
         }
         XX-> flags. brush_fore = 0;
         XX-> flags. brush_back = 0;
         if ( func != ofunc)
            XSetFunction( DISP, XX-> gc, func);
         XCHECKPOINT;
         prima_put_ximage( XX-> gdrawable, XX-> gc, stretch, 0, 0, x, y, w, h);
         func = GXxor;
         if ( func == ofunc)
            XSetFunction( DISP, XX-> gc, func);
         XCHECKPOINT;
         destroy_ximage( stretch);
         XCHECKPOINT;
      }
   } else
      func = prima_rop_map( rop);

   stretch = do_stretch(self, result,
                        src_x, src_y, src_w, src_h,
                        dst_x, dst_y, dst_w, dst_h,
                        &x, &y, &w, &h
                       );
   if ( stretch == nil) goto EXIT; /* nothing to draw */
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, func);
   if ( XT_IS_BITMAP(X(self))) {
      XSetForeground( DISP, XX-> gc, 1);
      XSetBackground( DISP, XX-> gc, 0);
      XX-> flags. brush_fore = XX-> flags. brush_back = 0;
   } else if ( mono) {
      unsigned long fore, back;
      if ( XT_IS_DBM(X(image))) {
         fore = XX-> back. primary;
         back = XX-> fore. primary;
      } else {
         if ( guts. palSize > 0) {
            fore = prima_color_find( self, 
                RGB_COMPOSITE( img-> palette[1].r, img-> palette[1].g, img-> palette[1].b),
                -1, nil, RANK_NORMAL);
            back = prima_color_find( self,
                RGB_COMPOSITE( img-> palette[0].r, img-> palette[0].g, img-> palette[0].b),
                -1, nil, RANK_NORMAL);
         } else {
            fore = 
               (((img-> palette[1].r << guts. red_range  ) >> 8) << guts.   red_shift) |
               (((img-> palette[1].g << guts. green_range) >> 8) << guts. green_shift) |
               (((img-> palette[1].b << guts. blue_range ) >> 8) << guts.  blue_shift);
            back = 
               (((img-> palette[0].r << guts. red_range  ) >> 8) << guts.   red_shift) |
               (((img-> palette[0].g << guts. green_range) >> 8) << guts. green_shift) |
               (((img-> palette[0].b << guts. blue_range ) >> 8) << guts.  blue_shift);
         }
      }
         
      XSetForeground( DISP, XX-> gc, fore);
      XSetBackground( DISP, XX-> gc, back);
      XX-> flags. brush_fore = XX-> flags. brush_back = 0;
   }
   prima_put_ximage( XX-> gdrawable, XX-> gc, stretch, 0, 0, x, y, w, h);
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, ofunc);
   destroy_ximage( stretch);
   XCHECKPOINT;
   XFLUSH;
EXIT:   
   if ( tempResult)
      prima_free_ximage( result);
   return true;
}

Bool
apc_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
   DEFXX;
   Bool inPaint = opt_InPaint, ret = false;
   XImage * i;
   
   if ( !image || PObject(image)-> stage == csDead) return false;
   
   XFLUSH;

   /* rect validation - questionable but without it the request may be fatal ( by BadMatch) */
   if ( x < 0) x = 0;
   if ( y < 0) y = 0;
   if ( x + xLen > XX-> size. x) xLen = XX-> size. x - x;
   if ( y + yLen > XX-> size. y) yLen = XX-> size. y - y;
   if ( xLen <= 0 || yLen <= 0) return false;
   
   if ( !inPaint) apc_application_begin_paint( self);

   CImage( image)-> create_empty( image, xLen, yLen, guts. qdepth);
   if ( guts. idepth == 1)
      i = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y - y - yLen, xLen, yLen, 1, XYPixmap);
   else
      i = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y - y - yLen, xLen, yLen, AllPlanes, ZPixmap);
   XCHECKPOINT;

   if ( i) {
      if ( !( ret = prima_query_image( image, i))) 
         warn("UAI_017: unsupported depths combination");
      XDestroyImage( i);
   }
   
   if ( !inPaint) apc_application_end_paint( self);
   return ret;
}   

