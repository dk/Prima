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
      guts.xshmattach_failed = false;
      XSetErrorHandler(shm_ignore_errors);
      if ( XShmAttach(DISP, &i->xmem) == 0) {
         XCHECKPOINT;
bad_xshm_attach:
         XSetErrorHandler(guts.main_error_handler);
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
   i-> image = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
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

Bool
prima_free_ximage( PrimaXImage *i) 
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
   hash_first_that( guts.ximages, destroy_one_ximage, nil, nil, nil);
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
prima_put_ximage( XDrawable win, GC gc, PrimaXImage *i, int src_x, int src_y, int dst_x, int dst_y, int width, int height)
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
    DEFXX;
    PImage img = PImage( self);
    XX-> gdrawable = XCreatePixmap( DISP, guts. root, 1, 1, 
       ((img-> type & imBPP) == 1) ? 1 : guts. depth);
    XCHECKPOINT;
    prima_prepare_drawable_for_painting( self);
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
   XX->gdrawable        = XCreatePixmap( DISP, guts. root, XX->size. x, XX->size. y,
                                         monochrome ? 1 : guts.depth);
   if (XX-> gdrawable == None) return false;
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
   while ( dataSize--)
      *(data++) = mirrored_bits[*data];
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
      cache-> icon      = ximage;
   } else {
      cache-> image     = ximage;
      cache-> back      = *prima_allocate_color( cache->bitmap ? nilHandle : application, ARGB(img->palette[0].r,img->palette[0].g,img->palette[0].b));
      cache-> fore      = *prima_allocate_color( cache->bitmap ? nilHandle : application, ARGB(img->palette[1].r,img->palette[1].g,img->palette[1].b));
   }
   return true;
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

static Bool
create_cache4_8( Image *img, ImageCache *cache)
{
   unsigned char lut1[ NPalEntries8];
   unsigned char lut2[ NPalEntries8];
   unsigned char *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   unsigned i;

   for ( i = 0; i < NPalEntries8; i++) {
      lut1[i] = ((i & MSNibble) >> MSNibbleShift);
      lut2[i] = ((i & LSNibble) >> LSNibbleShift);
   }   
   

   cache->image = prima_prepare_ximage( w, h, false);
   if ( !cache->image) return false;
   ls = get_ximage_bytes_per_line( cache->image);
   data = get_ximage_data( cache->image);
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register unsigned char *d = (unsigned char*)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
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
   lls = ls > img-> lineSize ? img-> lineSize : ls;

   for ( y = h-1; y >= 0; y--) 
      memcpy( data + ls * (h - y - 1), img-> data + y*img-> lineSize,  lls);
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
  
static int
get_bpp_depth( int depth)
{
   if ( depth == 1) return 1; else
   if ( depth <= 4) return 4; else
   if ( depth <= 8) return 8; else
   return 24;
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
   case 8:      return create_cache_equal( img, cache);
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

ImageCache*
prima_create_image_cache( PImage img, Handle drawable)
{
   PDrawableSysData IMG = X((Handle)img);
   int target_bpp       = get_bpp( drawable);
   ImageCache *cache    = get_cache((Handle)img, drawable);
   Bool ret;

   if ( img-> palette == nil) {
      warn( "UAI_014: image has no palette");
      return false;
   }
   
   if ( XT_IS_ICON(IMG) && cache-> icon == nil) {
      if ( !create_cache1_1( img, cache, true))
         return nil;
   }   
   if ( cache-> image == nil) {
      switch ( img-> type & imBPP) {
      case 1:   ret = create_cache1( img, cache, target_bpp); break;
      case 4:   ret = create_cache4( img, cache, target_bpp); break;
      case 8:   ret = create_cache8( img, cache, target_bpp); break;
      case 24:  ret = create_cache24(img, cache, target_bpp); break;
      default:  
         warn( "UAI_015: unsupported image type");
         return nil;
      }
      if ( !ret) return nil;
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
   if ( !cache) return false;
   p1 = XCreatePixmap( DISP, guts. root, icon-> w, icon-> h, 1);
   p2 = XCreatePixmap( DISP, guts. root, icon-> w, icon-> h, 1);
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
   XSetForeground( DISP, XX-> gc, 1);
   XSetBackground( DISP, XX-> gc, 0);
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
   if ( !cache) return false;
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
         XSetForeground( DISP, XX-> gc, 0xffffffff);
         XSetBackground( DISP, XX-> gc, 0x00000000);
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
   XX-> gdrawable = XCreatePixmap( DISP, guts. root, img-> w, img-> h,
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
                                 int *rev_bit_count,
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
   *rev_bit_count = bc;
   for ( i = 0; i <= *mask2; i++) {
      lut[i] = i << bc;
   }
}

static RGBLUTEntry lut[3];

RGBLUTEntry * 
prima_rgblut( void)
{
   static Bool initialize = true;
   if (initialize) {
      Visual *v = DefaultVisual( DISP, SCREEN);

      calc_masks_and_lut_16or32_to_24( v-> red_mask,   &lut[0]. mask, &lut[0]. revMask, &lut[0]. shift, &lut[0]. revShift, lut[0]. lut);
      calc_masks_and_lut_16or32_to_24( v-> green_mask, &lut[1]. mask, &lut[1]. revMask, &lut[1]. shift, &lut[1]. revShift, lut[1]. lut);
      calc_masks_and_lut_16or32_to_24( v-> blue_mask,  &lut[2]. mask, &lut[2]. revMask, &lut[2]. shift, &lut[2]. revShift, lut[2]. lut);
      
      initialize = false;
   }   
   return lut;
}   

static void
convert_16_to_24( XImage *i, PImage img)
{
   static ColorComponent * lur, * lub, * lug;
   static Bool initialize = true;
   static unsigned long rm1, bm1, gm1;
   static int rbc, bbc, gbc;
   int y, x, h, w;
   Pixel16 *d;
   Pixel24 *line;

   if ( initialize) {
      RGBLUTEntry * r = prima_rgblut();
      rm1 = r[0]. mask;  gm1 = r[1]. mask;  bm1 = r[2]. mask;
      rbc = r[0]. shift; gbc = r[1]. shift; bbc = r[2]. shift;
      lur = r[0]. lut;   lug = r[1]. lut;   lub = r[2]. lut;
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
   static ColorComponent * lur, * lub, *lug;
   static Bool initialize = true;
   static unsigned long rm1, bm1, gm1;
   static int rbc, bbc, gbc;
   int y, x, h, w;
   Pixel32 *d, dd;
   Pixel24 *line;

   if ( initialize) {
      RGBLUTEntry * r = prima_rgblut();
      rm1 = r[0]. mask;  gm1 = r[1]. mask;  bm1 = r[2]. mask;
      rbc = r[0]. shift; gbc = r[1]. shift; bbc = r[2]. shift;
      lur = r[0]. lut;   lug = r[1]. lut;   lub = r[2]. lut;
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

Bool
prima_query_image( Handle self, XImage * i)
{
   PImage img = PImage( self);
   int target_depth = (( img-> type & imBPP) == 1) ? 1 : get_bpp_depth( guts. idepth);

   if (( img-> type & imBPP) != target_depth) 
      CImage( self)-> create_empty( self, img-> w, img-> h, target_depth);

   if ( target_depth == 1) {
      prima_copy_xybitmap( img-> data, i-> data, img-> w, img-> h, img-> lineSize, i-> bytes_per_line);
   } else {
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
            return false;
         }
      } else {
         /* just copy with care */
      }
   }
   return true;
}   
   

static void
slurp_image( Handle self, Pixmap px)
{
   XImage *i = nil;
   PImage img = PImage( self);

   if ( !px) return;

   if (( img-> type & imBPP) == 1)
      i = XGetImage( DISP, px, 0, 0, img-> w, img-> h, 1, XYPixmap);
   else
      i = XGetImage( DISP, px, 0, 0, img-> w, img-> h, AllPlanes, ZPixmap);
   XCHECKPOINT;
   if ( i) {
      Bool res = prima_query_image( self, i);
      XDestroyImage( i);
      if ( !res) 
         warn( "UAI_017: unsupported depths combination");
   } else 
      warn("Error querying image");  
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

#define ABS(x) (((x)<0)?(-(x)):(x))

static Bool mbsInitialized = false;
static Byte set_bits[ByteValues];
static Byte clear_bits[ByteValues];

static void mbs_init_bits()
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

BS_BYTEEXPAND( Pixel8);
BS_BYTEEXPAND( Pixel16);
BS_BYTEEXPAND( Pixel24);
BS_BYTEEXPAND( Pixel32);

BS_BYTEIMPACT( Pixel8);
BS_BYTEIMPACT( Pixel16);
BS_BYTEIMPACT( Pixel24);
BS_BYTEIMPACT( Pixel32);


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
   *clipstart = cstart;
   *clipsize  = cend - cstart;
}

typedef void mStretchProc( void * srcData, void * dstData, Bool xreverse, 
   int targetwidth, Fixed step, Fixed count, int first, int last, int targetLineSize);


static PrimaXImage *
do_stretch( Handle self, PImage img, PrimaXImage *cache,
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

   prima_gp_get_clip_rect( self, &cr);
   xclipstart = cr. x - dst_x;
   xclipsize = cr. width;
   yclipstart = cr. y - dst_y;
   yclipsize = cr. height;
   bpp = ( cache-> image-> format == XYBitmap) ? 1 : guts. idepth;
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
      mStretchProc * proc = nil;
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
   unsigned long f = 0, b = 0;
   PrimaXImage *stretch;
   ImageCache *cache;
   int func, ofunc;
   XGCValues gcv;
   int x, y, w, h;

   if ( XT_IS_DBM(X(image))) {
      DOLBUG( "apc_gp_stretch_image");
      return false;
   }   

   if ( img-> options. optInDrawInfo) return false;

   cache = prima_create_image_cache( img, self);
   if ( !cache) return false;
   if ( src_h < 0) {
      src_h = -src_h;
      dst_h = -dst_h;
   }   
   if ( src_w < 0) {
      src_w = -src_w;
      dst_w = -dst_w;
   }   
   
   if ( src_w != dst_w || src_h != dst_h) {

      SHIFT( dst_x, dst_y);
      dst_y = XX->size.y - dst_y - ABS(dst_h);
      src_y = img-> h - src_y - ABS(src_h);

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
               XSetForeground( DISP, XX-> gc, 0xffffffff);
               XSetBackground( DISP, XX-> gc, 0x00000000);
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


Bool
apc_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
   DEFXX;
   Bool inPaint = opt_InPaint, ret = false;
   XImage * i;
   
   if ( !image || PObject(image)-> stage == csDead) return false;

   /* rect validation - questionable but without it the request may be fatal ( by BadMatch) */
   if ( x < 0) x = 0;
   if ( y < 0) y = 0;
   if ( x + xLen > XX-> size. x) xLen = XX-> size. x - x;
   if ( y + yLen > XX-> size. y) yLen = XX-> size. y - y;
   if ( xLen <= 0 || yLen <= 0) return false;
   
   if ( !inPaint) apc_application_begin_paint( self);

   CImage( image)-> create_empty( image, xLen, yLen, get_bpp_depth( guts. idepth));
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

