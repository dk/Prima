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
/*
 * System dependent image routines (unix, x11)
 */

#include "unix/guts.h"
#include "Image.h"
#include "Icon.h"
#include "DeviceBitmap.h"
#include "img_conv.h"

#define REVERT(a)	({ XX-> size. y - (a) - 1; })
#define SHIFT(a,b)	({ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; })
/* Multiple evaluation macro! */
#define REVERSE_BYTES_32(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))

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
typedef U8 ColorComponent;

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

struct PrimaXImage
{
   Bool shm;
   Bool can_free;
   int ref_cnt;
   void *data_alias;
   int bytes_per_line_alias;
   XImage *image;
#ifdef USE_MITSHM
   XShmSegmentInfo xmem;
#endif
};

#define get_ximage_data(xim)            ((xim)->data_alias)
#define get_ximage_bytes_per_line(xim)  ((xim)->bytes_per_line_alias)

static struct PrimaXImage*
prepare_ximage( int width, int height, Bool bitmap)
{
   struct PrimaXImage *i;
   int extra_bytes;
  
   switch ( guts.idepth) {
   case 16:     extra_bytes = 1;        break;
   case 24:     extra_bytes = 5;        break;
   case 32:     extra_bytes = 7;        break;
   default:     extra_bytes = 0;
   }

   i = malloc( sizeof( struct PrimaXImage));
   if (!i) return nil;
   bzero( i, sizeof( struct PrimaXImage));

#ifdef USE_MITSHM
   if ( guts. local_connection && guts. shared_image_extension && !bitmap) {
      i-> image = XShmCreateImage( DISP, DefaultVisual( DISP, SCREEN),
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
         XDestroyImage( i-> image);
         shmctl( i-> xmem. shmid, IPC_RMID, 0);
         goto normal_way;
      }
      i-> xmem. readOnly = false;
      if ( XShmAttach( DISP, &i->xmem) == 0) {
         XCHECKPOINT;
         XDestroyImage( i-> image);
         shmdt( i-> xmem. shmaddr);
         shmctl( i-> xmem. shmid, IPC_RMID, 0);
         goto normal_way;
      }
      XCHECKPOINT;
      XSync(DISP,false);
      XCHECKPOINT;
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
      free(i);
      return nil;
   }
   i-> image = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
                             bitmap ? 1 : guts.depth,
                             bitmap ? XYBitmap : ZPixmap,
                             0, i-> data_alias,
                             width, height, 32, i-> bytes_per_line_alias);
   XCHECKPOINT;
   if ( !i-> image) {
      free( i-> data_alias);
      free( i);
      return nil;
   }
   return i;
}

static Bool
free_ximage( struct PrimaXImage *i) /* internal */
{
   if (!i) return true;
#ifdef USE_MITSHM
   if ( i-> shm) {
      XShmDetach( DISP, &i-> xmem);
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
destroy_ximage( struct PrimaXImage *i)
{
   if ( !i) return true;
   if ( i-> ref_cnt > 0) {
      i-> can_free = true;
      return true;
   }
   return free_ximage( i);
}

static Bool
destroy_one_ximage( struct PrimaXImage *i, int nothing1, void *nothing2, void *nothing3)
{
   free_ximage( i);
   return false;
}

void
prima_gc_ximages( void )
{
   if ( !guts.ximages) return;
   hash_first_that( guts.ximages, destroy_one_ximage, nil, nil, nil);
}

void
prima_ximage_event( XEvent *eve) /* to be called from apc_event's handle_event */
{
#ifdef USE_MITSHM
   XShmCompletionEvent *ev = (XShmCompletionEvent*)eve;
   struct PrimaXImage *i;

   if ( eve && eve-> type == guts. shared_image_completion_event) {
      i = hash_fetch( guts.ximages, (void*)&ev->shmseg, sizeof(ev->shmseg));
      if ( i) {
         i-> ref_cnt--;
         if ( i-> ref_cnt <= 0) {
            hash_delete( guts.ximages, (void*)&ev->shmseg, sizeof(ev->shmseg), false);
            if ( i-> can_free)
               free_ximage( i);
         }
      }
   }
#endif
}

void
prima_put_ximage( XDrawable win, GC gc, struct PrimaXImage *i, int src_x, int src_y, int dst_x, int dst_y, int width, int height)
{
#ifdef USE_MITSHM
   if ( i-> shm) {
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
   XX->bitmap_cache. bitmap     = true;
   XX->screen_cache. bitmap     = false;
   XX->size. x                  = PImage(self)-> w;
   XX->size. y                  = PImage(self)-> h;
   return true;
}

static void
clear_caches( Handle self)
{
   DEFXX;

   destroy_ximage( XX-> bitmap_cache. icon);
   destroy_ximage( XX-> bitmap_cache. image);
   destroy_ximage( XX-> screen_cache. icon);
   destroy_ximage( XX-> screen_cache. image);
   XX-> bitmap_cache. icon      = nil;
   XX-> bitmap_cache. image     = nil;
   XX-> screen_cache. icon      = nil;
   XX-> screen_cache. image     = nil;
}

Bool
apc_image_destroy( Handle self)
{
   clear_caches( self);
   return true;
}

Bool
apc_image_begin_paint_info( Handle self)
{
    DOLBUG( "apc_image_begin_paint_info()\n");
    return false;
}

Bool
apc_image_end_paint_info( Handle self)
{
   DOLBUG( "apc_image_end_paint_info()\n");
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
   XX-> type.pixmap = (img-> type & imBPP) != 1;
   XX-> type.bitmap = (img-> type & imBPP) == 1;
   return true;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
   DEFXX;
   XX-> type.bitmap = !!monochrome;
   XX-> type.pixmap = !monochrome;
   XX-> type.dbm = true;
   XX-> type.drawable = true;
   XX->size. x          = ((PDeviceBitmap)(self))-> w;
   XX->size. y          = ((PDeviceBitmap)(self))-> h;
   XX->gdrawable        = XCreatePixmap( DISP, RootWindow( DISP, SCREEN), XX->size. x, XX->size. y,
                                         monochrome ? 1 : guts.depth);
   if (XX-> gdrawable == None)
      croak( "UAI_001: create pixmap error");
   XCHECKPOINT;
   prima_prepare_drawable_for_painting( self);
   return true;
}

Bool
apc_dbm_destroy( Handle self)
{
   DEFXX;
   prima_cleanup_drawable_after_painting( self);
   if ( XX->gdrawable)
      XFreePixmap( DISP, XX->gdrawable);
   return true;
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
      for ( y = h-1; y >= 0; y--) {
	 register const unsigned char *s = idata+y*ils;
	 register unsigned char *t = ls*(h-y-1)+data;
	 for ( x = 0; x < (w+7)/8; x++) {
	    *t++ = mirrored_bits[*s++];
	 }
      }
   }
}

static void
create_cache1_1( Image *img, ImageCache *cache, Bool for_icon)
{
   unsigned char *data;
   int ls;
   int h = img-> h, w = img-> w;
   int ils;
   unsigned char *idata;
   struct PrimaXImage *ximage;

   if ( for_icon) {
      ils = PIcon(img)->maskLine;
      idata = PIcon(img)->mask;
   } else {
      ils = img-> lineSize;
      idata = img-> data;
   }

   ximage = prepare_ximage( w, h, true);
   if (!ximage) croak( "UAI_002: error creating ximage");
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);
   prima_copy_xybitmap( data, idata, w, h, ls, ils);

   if ( for_icon) {
      cache-> icon      = ximage;
   } else {
      cache-> image     = ximage;
      cache-> back      = *prima_allocate_color( cache->bitmap ? nilHandle : application, ARGB(img->palette[0].r,img->palette[0].g,img->palette[0].b));
      cache-> fore      = *prima_allocate_color( cache->bitmap ? nilHandle : application, ARGB(img->palette[1].r,img->palette[1].g,img->palette[1].b));
   }
}

static void
create_rgb_to_16_lut( int ncolors, const PRGBColor pal, Pixel16 *lut)
{
   /* XXX make this 3,2,5,11-independent */
   Visual *v = DefaultVisual( DISP, SCREEN);
   unsigned long red_mask, green_mask, blue_mask;
   int i;

   red_mask = v-> red_mask;
   green_mask = v-> green_mask;
   blue_mask = v-> blue_mask;
   for ( i = 0; i < ncolors; i++) {
      lut[i] = 0;
      lut[i] |=
	 (((pal[i]. r >> 3) << 11) & red_mask) & 0xffff;
      lut[i] |=
	 (((pal[i]. g >> 2) << 5) & green_mask) & 0xffff;
      lut[i] |=
	 ((pal[i]. b >> 3) & blue_mask) & 0xffff;
   }
}

static void
calc_shifts_rgb_to_xpixel( unsigned long mask,
                           int *right,
                           int *left)
{
   int bc, l;

   l = 0;
   while (( mask & 1) == 0) { l++; mask >>= 1; }
   bc = 0;
   while ( mask) { bc++; mask >>= 1; }
   *right = 8-bc;
   *left = l;
}

static int *
rank_rgb_shifts( void)
{
   static int shift[3];
   static Bool shift_unknown = true;
   Visual *v;
   unsigned long m;
   int xchg;

   if ( shift_unknown) {
      v = DefaultVisual( DISP, SCREEN);

      m = v-> red_mask;
      shift[0] = 0;
      while ((m & 1) == 0) { shift[0]++; m >>= 1; }
      m = v-> green_mask;
      shift[1] = 0;
      while ((m & 1) == 0) { shift[1]++; m >>= 1; }
      if ( shift[1] < shift[0]) {
         xchg = shift[0];
         shift[0] = shift[1];
         shift[1] = xchg;
      }
      m = v-> blue_mask;
      shift[2] = 0;
      while ((m & 1) == 0) { shift[2]++; m >>= 1; }
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
   Visual *v = DefaultVisual( DISP, SCREEN);
   unsigned long rmask, gmask, bmask;
   int rrsh, grsh, brsh, rlsh, glsh, blsh;
   int i;

   calc_shifts_rgb_to_xpixel( rmask = v-> red_mask, &rrsh, &rlsh);
   calc_shifts_rgb_to_xpixel( gmask = v-> green_mask, &grsh, &glsh);
   calc_shifts_rgb_to_xpixel( bmask = v-> blue_mask, &brsh, &blsh);

   for ( i = 0; i < ncolors; i++) {
      lut[i] = 0;
      lut[i] |=
	 (((pal[i]. r >> rrsh) << rlsh) & rmask);
      lut[i] |=
	 (((pal[i]. g >> grsh) << glsh) & gmask);
      lut[i] |=
	 (((pal[i]. b >> brsh) << blsh) & bmask);
   }

   if ( guts.machine_byte_order != guts.byte_order) {
      for ( i = 0; i < ncolors; i++) {
         lut[i] = REVERSE_BYTES_32(lut[i]);
      }
   }
}

static void
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
   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_003: error creating ximage");
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Duplet16 *d = (Duplet16*)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }
}

static void
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

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_004: error creating ximage");
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Duplet24 *d = (Duplet24 *)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }
}

static void
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

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_005: error creating ximage");
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Duplet32 *d = (Duplet32 *)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }
}

static void
create_cache8_16( Image *img, ImageCache *cache)
{
   Pixel16 lut[ NPalEntries8];
   Pixel16 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;

   create_rgb_to_16_lut( img-> palSize, img-> palette, lut);

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_006: error creating ximage");
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel16 *d = (Pixel16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
}

static void
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

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_007: error creating ximage");
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel24 *d = (Pixel24*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
}

static void
create_cache8_32( Image *img, ImageCache *cache)
{
   XPixel lut[ NPalEntries8];
   Pixel32 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;

   create_rgb_to_xpixel_lut( img-> palSize, img-> palette, lut);

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_008: error creating ximage");
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }
}

static void
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

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_009: error creating ximage");
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
}

static void
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

   cache->image = prepare_ximage( w, h, false);
   if ( !cache->image) croak( "UAI_010: error creating ximage");
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
}

static int
get_bpp( Handle self)
{
   if ( self == nilHandle || X(self) == nil)
      return 1; /* XXX */
   else if ( XT_IS_BITMAP(X(self)))
      return 1;
   else
      return guts. idepth;
}

static ImageCache*
get_cache( Handle self, Handle drawable)
{
   DEFXX;
   if ( drawable == nilHandle)
      return & XX-> bitmap_cache;
   else if ( XT_IS_BITMAP(X(drawable)))
      return & XX->bitmap_cache;
   else
      return & XX->screen_cache;
}

static void
create_cache1( Image* img, ImageCache *cache, int bpp)
{
   create_cache1_1( img, cache, false);
}

static void
create_cache4( Image* img, ImageCache *cache, int bpp)
{
   switch (bpp) {
   case 16:     create_cache4_16( img, cache);  break;
   case 24:     create_cache4_24( img, cache);  break;
   case 32:     create_cache4_32( img, cache);  break;
   default:     croak( "UAI_011: unsupported image conversion: %d => %d", 4, bpp);
   }
}

static void
create_cache8( Image* img, ImageCache *cache, int bpp)
{
   switch (bpp) {
   case 16:     create_cache8_16( img, cache);  break;
   case 24:     create_cache8_24( img, cache);  break;
   case 32:     create_cache8_32( img, cache);  break;
   default:     croak( "UAI_012: unsupported image conversion: %d => %d", 8, bpp);
   }
}

static void
create_cache24( Image* img, ImageCache *cache, int bpp)
{
   switch (bpp) {
   case 16:     create_cache24_16( img, cache); break;
   case 32:     create_cache24_32( img, cache); break;
   default:     croak( "UAI_013: unsupported image conversion: %d => %d", 24, bpp);
   }
}

ImageCache*
prima_create_image_cache( PImage img, Handle drawable)
{
   PDrawableSysData IMG = X((Handle)img);
   int target_bpp       = get_bpp( drawable);
   ImageCache *cache    = get_cache((Handle)img, drawable);

   if ( img-> palette == nil)
      croak( "UAI_014: image has no palette");
   if ( XT_IS_ICON(IMG) && cache-> icon == nil)
      create_cache1_1( img, cache, true);
   if ( cache-> image == nil) {
      switch ( img-> type & imBPP) {
      case 1:   create_cache1( img, cache, target_bpp); break;
      case 4:   create_cache4( img, cache, target_bpp); break;
      case 8:   create_cache8( img, cache, target_bpp); break;
      case 24:  create_cache24(img, cache, target_bpp); break;
      default:  croak( "UAI_015: unsupported image type");
      }
   }
   return cache;
}

Bool
prima_create_icon_pixmaps( Handle self, Pixmap *xor, Pixmap *and)
{
   DEFXX;
   Pixmap p1, p2;
   PIcon icon = PIcon(self);
   ImageCache *cache;

   cache = prima_create_image_cache((PImage)icon, nilHandle);
   p1 = XCreatePixmap( DISP, RootWindow( DISP, SCREEN), icon-> w, icon-> h, 1);
   p2 = XCreatePixmap( DISP, RootWindow( DISP, SCREEN), icon-> w, icon-> h, 1);
   XCHECKPOINT;
   if ( p1 == None || p2 == None) {
      if (p1 != None) XFreePixmap( DISP, p1);
      if (p2 != None) XFreePixmap( DISP, p2);
      return false;
   }
   XX-> gdrawable = p1;
   prima_prepare_drawable_for_painting( self);
   XSetForeground( DISP, XX-> gc, 0);
   XSetBackground( DISP, XX-> gc, 1);
   prima_put_ximage( p2, XX-> gc, cache->icon,
                     0, 0, 0, 0, icon-> w, icon-> h);
   prima_put_ximage( p1, XX-> gc, cache->image,
                     0, 0, 0, 0, icon-> w, icon-> h);
   prima_cleanup_drawable_after_painting( self);
   XX-> gdrawable = None;
   *xor = p1;
   *and = p2;
   return true;
}

static Bool
put_pixmap( Handle self, Handle pixmap, int dst_x, int dst_y, int src_x, int src_y, int w, int h, int rop)
{
   DEFXX;
   PDrawableSysData YY = X(pixmap);

   /* XXX currently can X-fail in several cases */
   /* XXX rop support */
   SHIFT( dst_x, dst_y);

   XCHECKPOINT;
   XCopyArea( DISP, YY-> gdrawable, XX-> gdrawable, XX-> gc,
              src_x, YY->size.y - src_y - h,
              w, h,
              dst_x, REVERT(dst_y) - h + 1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   DEFXX;
   PImage img = PImage( image);
   unsigned long f = 0, b = 0;
   int func, ofunc;
   XGCValues gcv;
   ImageCache *cache;

   if ( XT_IS_DBM(X(image)))
      return put_pixmap( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
   cache = prima_create_image_cache( img, self);
   SHIFT( x, y);
   if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) {
      warn( "UAI_016: error querying GC values");
   }
   ofunc = gcv. function;
   if ( cache-> icon) {
      func = GXand;
      f = XX-> fore. pixel;
      b = XX-> back. pixel;
      if ( cache-> bitmap) {
         XSetForeground( DISP, XX-> gc, 1);
         XSetBackground( DISP, XX-> gc, 0);
      } else {
         XSetForeground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
      }
      if ( func != ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
      prima_put_ximage( XX-> gdrawable, XX-> gc, cache->icon,
                        xFrom, img-> h - yFrom - yLen,
                        x, REVERT(y) - yLen + 1, xLen, yLen);
      XSetForeground( DISP, XX-> gc, f);
      XSetBackground( DISP, XX-> gc, b);
      func = GXxor;
      if ( func == ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
   } else {
      func = prima_rop_map( rop);
   }
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, func);
   if (( img-> type & imBPP) == 1) {
      f = XX-> fore. pixel;
      b = XX-> back. pixel;
      XSetForeground( DISP, XX-> gc, cache->fore. pixel);
      XSetBackground( DISP, XX-> gc, cache->back. pixel);
      XCHECKPOINT;
   }
   prima_put_ximage( XX-> gdrawable, XX-> gc, cache->image,
                     xFrom, img-> h - yFrom - yLen,
                     x, REVERT(y) - yLen + 1, xLen, yLen);
   if (( img-> type & imBPP) == 1) {
      XSetForeground( DISP, XX-> gc, f);
      XSetBackground( DISP, XX-> gc, b);
      XCHECKPOINT;
   }
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, ofunc);
   return true;
}

Bool
apc_image_begin_paint( Handle self)
{
   DEFXX;
   PImage img = PImage( self);
   Bool bitmap = (img-> type & imBPP) == 1;

   XX-> gdrawable = XCreatePixmap( DISP, RootWindow( DISP, SCREEN), img-> w, img-> h,
                                   bitmap ? 1 : guts. depth);
   XCHECKPOINT;
   prima_prepare_drawable_for_painting( self);
   apc_gp_put_image( self, self, 0, 0, 0, 0, img-> w, img-> h, ropCopyPut);
   /*                ^^^^^ ^^^^    :-)))  */
   return true;
}

static void
calc_masks_and_lut_16or32_to_24( unsigned long mask,
                                 unsigned long *mask1,
                                 unsigned long *mask2,
                                 int *bit_count,
                                 ColorComponent *lut)
{
   unsigned i;
   unsigned long m;
   int bc;

   *mask2 = mask;
   *bit_count = 0;
   while (( *mask2 & 1) == 0) { (*bit_count)++; *mask2 >>= 1; }
   m = *mask2;
   bc = 0;
   while ( m) { bc++; m >>= 1; }
   bc = 8 - bc;
   *mask1 = mask;
   for ( i = 0; i <= *mask2; i++) {
      lut[i] = i << bc;
   }
}

static void
convert_16_to_24( XImage *i, PImage img)
{
   /* XXX is ``static'' reliable here?? */
   static ColorComponent lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8];
   static Bool initialize = true;
   static unsigned long rm1, bm1, gm1, rm2, bm2, gm2;
   static int rbc, bbc, gbc;
   int y, x, h, w;
   Pixel16 *d;
   Pixel24 *line;

   if ( initialize) {
      Visual *v = DefaultVisual( DISP, SCREEN);

      calc_masks_and_lut_16or32_to_24( v-> red_mask, &rm1, &rm2, &rbc, lur);
      calc_masks_and_lut_16or32_to_24( v-> green_mask, &gm1, &gm2, &gbc, lug);
      calc_masks_and_lut_16or32_to_24( v-> blue_mask, &bm1, &bm2, &bbc, lub);

      initialize = false;
   }

   h = img-> h; w = img-> w;
   for ( y = 0; y < h; y++) {
      d = (Pixel16 *)(i-> data + (h-y-1)*i-> bytes_per_line);
      line = (Pixel24*)(img-> data + y*img-> lineSize);
      for ( x = 0; x < w; x++) {
         line-> a0 = lub[(*d & bm1) >> bbc];
	 line-> a1 = lug[(*d & gm1) >> gbc];
	 line-> a2 = lur[(*d & rm1) >> rbc];
	 d++; line++;
      }
   }
}

static void
convert_32_to_24( XImage *i, PImage img)
{
   /* XXX is ``static'' reliable here?? */
   static ColorComponent lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8];
   static Bool initialize = true;
   static unsigned long rm1, bm1, gm1, rm2, bm2, gm2;
   static int rbc, bbc, gbc;
   int y, x, h, w;
   Pixel32 *d, dd;
   Pixel24 *line;

   if ( initialize) {
      Visual *v = DefaultVisual( DISP, SCREEN);

      calc_masks_and_lut_16or32_to_24( v-> red_mask, &rm1, &rm2, &rbc, lur);
      calc_masks_and_lut_16or32_to_24( v-> green_mask, &gm1, &gm2, &gbc, lug);
      calc_masks_and_lut_16or32_to_24( v-> blue_mask, &bm1, &bm2, &bbc, lub);

      initialize = false;
   }

   h = img-> h; w = img-> w;
   if ( guts.machine_byte_order != guts.byte_order) {
      for ( y = 0; y < h; y++) {
         d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
         line = (Pixel24*)(img-> data + y*img-> lineSize);
         for ( x = 0; x < w; x++) {
            dd = REVERSE_BYTES_32(*d);
            line-> a0 = lub[(dd & bm1) >> bbc];
            line-> a1 = lug[(dd & gm1) >> gbc];
            line-> a2 = lur[(dd & rm1) >> rbc];
            d++; line++;
         }
      }
   } else {
      for ( y = 0; y < h; y++) {
         d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
         line = (Pixel24*)(img-> data + y*img-> lineSize);
         for ( x = 0; x < w; x++) {
            line-> a0 = lub[(*d & bm1) >> bbc];
            line-> a1 = lug[(*d & gm1) >> gbc];
            line-> a2 = lur[(*d & rm1) >> rbc];
            d++; line++;
         }
      }
   }
}

static void
slurp_image( Handle self, Pixmap px)
{
   int target_depth;
   XImage *i = nil;
   PImage img = PImage( self);

   if (( img-> type & imBPP) == 1) {
      if ( px) {
         i = XGetImage( DISP, px, 0, 0, img-> w, img-> h, 1, XYPixmap);
         XCHECKPOINT;
         prima_copy_xybitmap( img-> data, i-> data, img-> w, img-> h, img-> lineSize, i-> bytes_per_line);
      }
   } else {
      if ( px) {
         i = XGetImage( DISP, px, 0, 0, img-> w, img-> h, AllPlanes, ZPixmap);
         XCHECKPOINT;

         target_depth = guts. idepth;
         if ( target_depth == 16 || target_depth == 32)
            target_depth = 24;
         if (( img-> type & imBPP) != target_depth) {
            CImage( self)-> create_empty( self, img-> w, img-> h, target_depth);
         }
         if ( guts. idepth != target_depth) {
            switch ( guts. idepth) {
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
               XDestroyImage( i);
               croak( "UAI_017: unsupported depths combination");
            }
         } else {
            /* just copy with care */
         }
      }
   }
   if (i) XDestroyImage(i);
}

Bool
apc_image_end_paint( Handle self)
{
   DEFXX;
   slurp_image( self, XX-> gdrawable);
   prima_cleanup_drawable_after_painting( self);
   if ( XX-> gdrawable) {
      XFreePixmap( DISP, XX-> gdrawable);
      XCHECKPOINT;
      XX-> gdrawable = 0;
   }
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

static void
stretch_1( const StretchSeed *xseed, const StretchSeed *yseed,
           Bool xreverse, Bool yreverse,
           Bool xshrink, Bool yshrink,
           Byte *source, int source_line_size,
           Byte *target, int targetwidth, int targetheight,
           int target_line_size)
{
   Fixed xcount, ycount;
   Fixed xstep, ystep;
   int xlast, ylast;
   int xfirst, yfirst;
   unsigned x, i;
   Byte *last_source = nil;
   static Bool initialize = true;
   static Byte set_bits[ByteValues], clear_bits[ByteValues], *bits = set_bits;
   void *trg;
   int th, tls;

   trg = target;
   th = targetheight;
   tls = target_line_size;

   if ( initialize) {
      if ( guts.bit_order == MSBFirst) {
         Byte *mirrored_bits = mirror_bits();
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
      initialize = false;
   }
   ycount = yseed-> count;
   ystep  = yseed-> step;
   ylast  = yseed-> last;
   yfirst = yseed-> source;

   source = source + yfirst * source_line_size;
   bzero( target, targetheight*target_line_size);
   if (yreverse) {
      target = target + (targetheight-1)*target_line_size;
      target_line_size = - target_line_size;
   }

   if ( yshrink) {
      while ( targetheight) {
      }
   } else {
      while ( targetheight) {
         if ( ycount.i.i > ylast) {
            source += source_line_size;
            ylast = ycount.i.i;
         }
         ycount.l += ystep.l;
         if ( last_source == source) {
            memcpy( target, target - target_line_size, target_line_size < 0 ? - target_line_size : target_line_size);
         } else {
            last_source = source;
            xcount = xseed-> count;
            xstep  = xseed-> step;
            xlast  = xseed-> last;
            xfirst = xseed-> source;

            if ( xshrink) {
            } else {
               x = 0;
               while ( x < targetwidth) {
                  if ( xcount.i.i > xlast) {
                     xfirst++;
                     xlast = xcount.i.i;
                  }
                  xcount.l += xstep.l;
                  if ( source[xfirst/ByteBits] & bits[LOWER_BYTE(xfirst)])
                     target[x/ByteBits] |= set_bits[LOWER_BYTE(x)];
                  else
                     target[x/ByteBits] &= clear_bits[LOWER_BYTE(x)];
                  x++;
               }
            }
         }
         target += target_line_size;
         targetheight--;
      }
   }
}

static void
stretch_16( const StretchSeed *xseed, const StretchSeed *yseed,
            Bool xreverse, Bool yreverse,
            Bool xshrink, Bool yshrink,
            Pixel16 *source, int source_line_size,
            Pixel16 *target, int targetwidth, int targetheight,
            int target_line_size)
{
   Fixed xcount, ycount;
   Fixed xstep, ystep;
   int xlast, ylast;
   int xfirst, yfirst;
   int x;
   Pixel16 *last_source = nil;

   ycount = yseed-> count;
   ystep  = yseed-> step;
   ylast  = yseed-> last;
   yfirst = yseed-> source;

   source = (Pixel16*)((Byte*)source + yfirst * source_line_size);
   if (yreverse) {
      target = (Pixel16*)((Byte*)target + (targetheight-1)*target_line_size);
      target_line_size = - target_line_size;
   }

   if ( yshrink) {
      while ( targetheight) {
      }
   } else {
      while ( targetheight) {
         if ( ycount.i.i > ylast) {
            source = (Pixel16*)((Byte*)source + source_line_size);
            ylast = ycount.i.i;
         }
         ycount.l += ystep.l;
         if ( last_source == source) {
            memcpy( target, (Byte*)target - target_line_size, target_line_size < 0 ? - target_line_size : target_line_size);
         } else {
            last_source = source;
            xcount = xseed-> count;
            xstep  = xseed-> step;
            xlast  = xseed-> last;
            xfirst = xseed-> source;

            if ( xshrink) {
            } else {
               x = 0;
               while ( x < targetwidth) {
                  if ( xcount.i.i > xlast) {
                     xfirst++;
                     xlast = xcount.i.i;
                  }
                  xcount.l += xstep.l;
                  target[x++] = source[xfirst];
               }
            }
         }
         target = (Pixel16*)((Byte*)target + target_line_size);
         targetheight--;
      }
   }
}

static void
stretch_24( const StretchSeed *xseed, const StretchSeed *yseed,
            Bool xreverse, Bool yreverse,
            Bool xshrink, Bool yshrink,
            Pixel24 *source, int source_line_size,
            Pixel24 *target, int targetwidth, int targetheight,
            int target_line_size)
{
   Fixed xcount, ycount;
   Fixed xstep, ystep;
   int xlast, ylast;
   int xfirst, yfirst;
   int x;
   Pixel24 *last_source = nil;

   ycount = yseed-> count;
   ystep  = yseed-> step;
   ylast  = yseed-> last;
   yfirst = yseed-> source;

   source = (Pixel24*)((Byte*)source + yfirst * source_line_size);
   if (yreverse) {
      target = (Pixel24*)((Byte*)target + (targetheight-1)*target_line_size);
      target_line_size = - target_line_size;
   }

   if ( yshrink) {
      while ( targetheight) {
      }
   } else {
      while ( targetheight) {
         if ( ycount.i.i > ylast) {
            source = (Pixel24*)((Byte*)source + source_line_size);
            ylast = ycount.i.i;
         }
         ycount.l += ystep.l;
         if ( last_source == source) {
            memcpy( target, (Byte*)target - target_line_size, target_line_size < 0 ? - target_line_size : target_line_size);
         } else {
            last_source = source;
            xcount = xseed-> count;
            xstep  = xseed-> step;
            xlast  = xseed-> last;
            xfirst = xseed-> source;

            if ( xshrink) {
            } else {
               x = 0;
               while ( x < targetwidth) {
                  if ( xcount.i.i > xlast) {
                     xfirst++;
                     xlast = xcount.i.i;
                  }
                  xcount.l += xstep.l;
                  target[x++] = source[xfirst];
               }
            }
         }
         target = (Pixel24*)((Byte*)target + target_line_size);
         targetheight--;
      }
   }
}

static void
stretch_32( const StretchSeed *xseed, const StretchSeed *yseed,
            Bool xreverse, Bool yreverse,
            Bool xshrink, Bool yshrink,
            Pixel32 *source, int source_line_size,
            Pixel32 *target, int targetwidth, int targetheight,
            int target_line_size)
{
   Fixed xcount, ycount;
   Fixed xstep, ystep;
   int xlast, ylast;
   int xfirst, yfirst;
   int x;
   Pixel32 *last_source = nil;

   ycount = yseed-> count;
   ystep  = yseed-> step;
   ylast  = yseed-> last;
   yfirst = yseed-> source;

   source = (Pixel32*)((Byte*)source + yfirst * source_line_size);
   if (yreverse) {
      target = (Pixel32*)((Byte*)target + (targetheight-1)*target_line_size);
      target_line_size = - target_line_size;
   }

   if ( yshrink) {
      while ( targetheight) {
      }
   } else {
      while ( targetheight) {
         if ( ycount.i.i > ylast) {
            source = (Pixel32*)((Byte*)source + source_line_size);
            ylast = ycount.i.i;
         }
         ycount.l += ystep.l;
         if ( last_source == source) {
            memcpy( target, (Byte*)target - target_line_size, target_line_size < 0 ? - target_line_size : target_line_size);
         } else {
            last_source = source;
            xcount = xseed-> count;
            xstep  = xseed-> step;
            xlast  = xseed-> last;
            xfirst = xseed-> source;

            if ( xshrink) {
            } else {
               x = 0;
               while ( x < targetwidth) {
                  if ( xcount.i.i > xlast) {
                     xfirst++;
                     xlast = xcount.i.i;
                  }
                  xcount.l += xstep.l;
                  target[x++] = source[xfirst];
               }
            }
         }
         target = (Pixel32*)((Byte*)target + target_line_size);
         targetheight--;
      }
   }
}

static void
stretch_calculate_seed( int ssize, int tsize,
                        int *clipstart, int *clipsize,
                        int bpp,
                        StretchSeed *seed)
/*
   ARGUMENTS:

   INP  int ssize       specifies source 1d size
   INP  int tsize       specifies target size
   I/O  int *clipstart  on input, desired start of clipped stretched image
                        on output, adjusted value
   I/O  int *clipsize   on input, desired size of clipped region
                        on output, adjusted value
   INP  int bpp         bits per pixel value; used for fine adjustments
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
   if ( tsize < 0) {
      asize  = -tsize;
      cend   = *clipstart;
      cstart =  cend + *clipsize;
      t      =  asize - 1;
      dt     = -1;
      if ( cend < 0)            cend = 0;
      if ( cstart > asize)      cstart = asize;
      if ( bpp == 1) {
         cend &= ~7;
         cstart += 7;  cstart &= ~7;
      }
   } else {
      asize  =  tsize;
      cstart = *clipstart;
      cend   =  cstart + *clipsize;
      t      =  0;
      dt     =  1;
      if ( cstart < 0)          cstart = 0;
      if ( cend > asize)        cend = asize;
      if ( bpp == 1) {
         cstart &= ~7;
         cend += 7;  cend &= ~7;
      }
   }
   if ( asize < ssize) {
      step. l = (double) asize / ssize * 0x10000;
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
      step. l = (double) ssize / asize * 0x10000;
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
   if ( tsize < 0) {
      *clipstart = cend;
      *clipsize  = cstart - cend;
   } else {
      *clipstart = cstart;
      *clipsize  = cend - cstart;
   }
}

static struct PrimaXImage *
do_stretch( Handle self, PImage img, struct PrimaXImage *cache,
            int src_x, int src_y, int src_w, int src_h,
            int dst_x, int dst_y, int dst_w, int dst_h,
            int *x, int *y, int *w, int *h)
{
   Byte *data;
   struct PrimaXImage *stretch;
   StretchSeed xseed, yseed;
   XRectangle cr;
   int bpp;
   int sls;
   int tls;
   int xclipstart, xclipsize;
   int yclipstart, yclipsize;

   prima_gp_get_clip_rect( self, &cr);
   xclipstart = cr. x - dst_x;
   xclipsize = cr. width;
   yclipstart = cr. y - dst_y;
   yclipsize = cr. height;

   bpp = ( cache-> image-> format == XYBitmap) ? 1 : guts. idepth;

   if ( xclipstart + xclipsize <= 0 || yclipstart + yclipsize <= 0) return nil;
   stretch_calculate_seed( src_w, dst_w, &xclipstart, &xclipsize, bpp, &xseed);
   stretch_calculate_seed( src_h, dst_h, &yclipstart, &yclipsize, 0, &yseed);
   if ( xclipsize <= 0 || yclipsize <= 0) return nil;

   stretch = prepare_ximage( xclipsize, yclipsize, bpp == 1);
   if ( !stretch) croak( "UAI_019: error creating ximage");

   tls = get_ximage_bytes_per_line( stretch);
   sls = get_ximage_bytes_per_line( cache);
   data = get_ximage_data( stretch);

   switch ( bpp) {
   case 1:
      stretch_1( &xseed, &yseed,
                 dst_w < 0, dst_h < 0,
                 dst_w < 0 ? -dst_w < src_w : dst_w < src_w,
                 dst_w < 0 ? -dst_w < src_w : dst_w < src_w,
                 (get_ximage_data(cache) + src_y*sls + src_x/ByteBits), sls,
                 data, xclipsize, yclipsize, tls);
      break;
   case 16:
      stretch_16( &xseed, &yseed,
                  dst_w < 0, dst_h < 0,
                  dst_w < 0 ? -dst_w < src_w : dst_w < src_w,
                  dst_h < 0 ? -dst_h < src_h : dst_h < src_h,
                  (void*)(get_ximage_data(cache) + src_y*sls + src_x*sizeof(Pixel16)), sls,
                  (void*)data, xclipsize, yclipsize, tls);
      break;
   case 24:
      stretch_24( &xseed, &yseed,
                  dst_w < 0, dst_h < 0,
                  dst_w < 0 ? -dst_w < src_w : dst_w < src_w,
                  dst_h < 0 ? -dst_h < src_h : dst_h < src_h,
                  (void*)(get_ximage_data(cache) + src_y*sls + src_x*sizeof(Pixel24)), sls,
                  (void*)data, xclipsize, yclipsize, tls);
      break;
   case 32:
      stretch_32( &xseed, &yseed,
                  dst_w < 0, dst_h < 0,
                  dst_w < 0 ? -dst_w < src_w : dst_w < src_w,
                  dst_h < 0 ? -dst_h < src_h : dst_h < src_h,
                  (void*)(get_ximage_data(cache) + src_y*sls + src_x*sizeof(Pixel32)), sls,
                  (void*)data, xclipsize, yclipsize, tls);
      break;
   default:
      croak( "UAI_020: %d-bit stretch is not yet implemented", bpp);
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
   unsigned long f = 0, b = 0;
   struct PrimaXImage *stretch;
   ImageCache *cache;
   int func, ofunc;
   XGCValues gcv;
   int x, y, w, h;

   if ( XT_IS_DBM(X(image)))
      croak( "UAI_021: not implemented");

   cache = prima_create_image_cache( img, self);
   if ( src_w != dst_w || src_h != dst_h) {

      SHIFT( dst_x, dst_y);
      dst_y = XX->size.y - dst_y - dst_h;
      src_y = img-> h - src_y - src_h;

      if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) {
         warn( "UAI_022: error querying GC values");
      }
      ofunc = gcv. function;

      if ( cache-> icon) {
         func = GXxor;
         stretch = do_stretch(self, img, cache-> icon,
                              src_x, src_y, src_w, src_h,
                              dst_x, dst_y, dst_w, dst_h,
                              &x, &y, &w, &h
                             );
         if ( stretch != nil) {
            func = GXand;
            f = XX-> fore. pixel;
            b = XX-> back. pixel;
            if ( cache-> bitmap) {
               XSetForeground( DISP, XX-> gc, 1);
               XSetBackground( DISP, XX-> gc, 0);
            } else {
               XSetForeground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
               XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
            }
            if ( func != ofunc)
               XSetFunction( DISP, XX-> gc, func);
            XCHECKPOINT;
            prima_put_ximage( XX-> gdrawable, XX-> gc, stretch, 0, 0, x, y, w, h);
            XSetForeground( DISP, XX-> gc, f);
            XSetBackground( DISP, XX-> gc, b);
            func = GXxor;
            if ( func == ofunc)
               XSetFunction( DISP, XX-> gc, func);
            XCHECKPOINT;
            destroy_ximage( stretch);
            XCHECKPOINT;
         }
      } else
         func = prima_rop_map( rop);

      stretch = do_stretch(self, img, cache-> image,
                           src_x, src_y, src_w, src_h,
                           dst_x, dst_y, dst_w, dst_h,
                           &x, &y, &w, &h
                          );
      if ( stretch == nil) return true; /* nothing to draw */
      if ( func != ofunc)
         XSetFunction( DISP, XX-> gc, func);

      if (( img-> type & imBPP) == 1) {
         f = XX-> fore. pixel;
         b = XX-> back. pixel;
         XSetForeground( DISP, XX-> gc, cache-> fore. pixel);
         XSetBackground( DISP, XX-> gc, cache-> back. pixel);
         XCHECKPOINT;
      }
      prima_put_ximage( XX-> gdrawable, XX-> gc, stretch, 0, 0, x, y, w, h);
      if (( img-> type & imBPP) == 1) {
         XSetForeground( DISP, XX-> gc, f);
         XSetBackground( DISP, XX-> gc, b);
         XCHECKPOINT;
      }
      if ( func != ofunc)
         XSetFunction( DISP, XX-> gc, ofunc);
      destroy_ximage( stretch);
      XCHECKPOINT;
   } else
      return apc_gp_put_image( self, image, dst_x, dst_y, src_x, src_y, src_w, src_h, rop);
   return true;
}

