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
/*
 * System dependent image routines (unix, x11)
 */

#include "unix/guts.h"
#include "Image.h"
#include "Icon.h"
#include "img_conv.h"

#define REVERT(a)	({ XX-> size. y - (a) - 1; })
#define SHIFT(a,b)	({ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; })
// #undef USE_MITSHM

typedef struct _PrimaXImage
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
} PrimaXImage;

#define get_ximage_data(xim)            ((xim)->data_alias)
#define get_ximage_bytes_per_line(xim)  ((xim)->bytes_per_line_alias)

static PrimaXImage*
prepare_ximage( int width, int height, Bool bitmap)
{
   PrimaXImage *i;

   i = malloc( sizeof( PrimaXImage));
   if (!i) return nil;
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
                                i-> image-> bytes_per_line * height,
                                IPC_CREAT | 0777);
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
         XDestroyImage( i-> image);
         shmdt( i-> xmem. shmaddr);
         shmctl( i-> xmem. shmid, IPC_RMID, 0);
         goto normal_way;
      }
      XSync(DISP,false);
      shmctl( i-> xmem. shmid, IPC_RMID, 0);
      i-> data_alias = i-> image-> data;
      i-> shm = true;
      return i;
   }
normal_way:
#endif
   i-> bytes_per_line_alias = (( width * (bitmap ? 1 : guts.depth) + 31) / 32) * 4;
   i-> data_alias = malloc( height * i-> bytes_per_line_alias);
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
free_ximage( PrimaXImage *i) /* internal */
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
   if ( i-> ref_cnt > 0) {
      i-> can_free = true;
      return true;
   }
   return free_ximage( i);
}

static Bool
destroy_one_ximage( PrimaXImage *i, int nothing1, void *nothing2, void *nothing3)
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
   PrimaXImage *i;

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
put_ximage( XDrawable win, GC gc, PrimaXImage *i, int src_x, int src_y, int dst_x, int dst_y, int width, int height)
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
   XX-> flags. is_image = true;
   return true;
}

Bool
apc_image_destroy( Handle self)
{
   DEFXX;
   if ( XX-> image_cache) {
      destroy_ximage( XX-> image_cache);
      XX-> image_cache = nil;
   }
   if ( XX-> icon_cache) {
      destroy_ximage( XX-> icon_cache);
      XX-> icon_cache = nil;
   }
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

   if ( XX-> image_cache) {
      destroy_ximage( XX-> image_cache);
      XX-> image_cache = nil;
   }
   if ( XX-> icon_cache) {
      destroy_ximage( XX-> icon_cache);
      XX-> icon_cache = nil;
   }
   XX-> size. x = img-> w;
   XX-> size. y = img-> h;
   return true;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
    DOLBUG( "apc_dbm_create()\n");
    return false;
}

Bool
apc_dbm_destroy( Handle self)
{
   DOLBUG( "apc_dbm_destroy()\n");
   return true;
}

void
prima_copy_xybitmap( unsigned char *data, const unsigned char *idata, int w, int h, int ls, int ils)
{
   static Bool initialized = false;
   static unsigned char bits[256];
   int y;
   register int x;

   if ( guts.bit_order == MSBFirst) {
      for ( y = h-1; y >= 0; y--) {
	 memcpy( ls*(h-y-1)+data, idata+y*ils, ls);
      }
   } else {
      if (!initialized) {
	 unsigned int i, j;
	 int k;
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
      for ( y = h-1; y >= 0; y--) {
	 register const unsigned char *s = idata+y*ils;
	 register unsigned char *t = ls*(h-y-1)+data;
	 for ( x = 0; x < (w+7)/8; x++) {
	    *t++ = bits[*s++];
	 }
      }
   }
}

static void
create_image_cache_1_to_1( PImage img, Handle drawable, Bool icon)
{
   PDrawableSysData IMG = X((Handle)img);
   unsigned char *data;
   int ls;
   int h = img-> h, w = img-> w;
   int ils;
   unsigned char *idata;
   PrimaXImage *ximage;
   PrimaXImage **cache, **icon_cache;
   Bool bit_cache = X(drawable)->flags.is_image && (PImage(drawable)->type & imBPP) == 1;

   if ( bit_cache) {
      cache = &IMG-> image_bit_cache;
      icon_cache = &IMG-> icon_bit_cache;
   } else {
      cache = &IMG-> image_cache;
      icon_cache = &IMG-> icon_cache;
   }

   if ( icon) {
      ils = PIcon(img)->maskLine;
      idata = PIcon(img)->mask;
   } else {
      ils = img-> lineSize;
      idata = img-> data;
   }

   ximage = prepare_ximage( w, h, true);
   if (!ximage) croak( "create_image_cache_1_to_1(): error creating ximage");
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);
   prima_copy_xybitmap( data, idata, w, h, ls, ils);

   if (icon) {
      *icon_cache = ximage;
   } else {
      *cache = ximage;
      (bit_cache ? IMG-> bitmap_bit_back : IMG-> bitmap_back) =
	 *prima_allocate_color(drawable,
                               ARGB(img->palette[0].r,img->palette[0].g,img->palette[0].b));
      (bit_cache ? IMG-> bitmap_bit_fore : IMG-> bitmap_fore) =
         *prima_allocate_color(drawable,
                               ARGB(img->palette[1].r,img->palette[1].g,img->palette[1].b));
   }
}

static void
create_rgb_to_16_lut( int ncolors, const PRGBColor pal, U16 *lut)
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
calc_shifts_rgb_to_24( unsigned long mask,
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

static void
create_rgb_to_24_lut( int ncolors, const PRGBColor pal, unsigned long *lut)
{
   Visual *v = DefaultVisual( DISP, SCREEN);
   unsigned long rmask, gmask, bmask;
   int rrsh, grsh, brsh, rlsh, glsh, blsh;
   int i;

   calc_shifts_rgb_to_24( rmask = v-> red_mask, &rrsh, &rlsh);
   calc_shifts_rgb_to_24( gmask = v-> green_mask, &grsh, &glsh);
   calc_shifts_rgb_to_24( bmask = v-> blue_mask, &brsh, &blsh);
   for ( i = 0; i < ncolors; i++) {
      lut[i] = 0;
      lut[i] |=
	 (((pal[i]. r >> rrsh) << rlsh) & rmask);
      lut[i] |=
	 (((pal[i]. g >> grsh) << glsh) & gmask);
      lut[i] |=
	 (((pal[i]. b >> brsh) << blsh) & bmask);
   }
}

static void
create_image_cache_4_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   U32 lut[ 256];
   U16 lut1[ 16];
   unsigned char *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   unsigned i;
   PrimaXImage *ximage;

   create_rgb_to_16_lut( 16, img-> palette, lut1);
   for ( i = 0; i < 256; i++) {
      lut[i] = ((U32)lut1[(i & 0xf0) >> 4]) | (((U32)lut1[(i & 0x0f) >> 0]) << 16);
   }
   ximage = prepare_ximage( w, h, false);
   if ( !ximage) croak( "create_image_cache_4_to_16(): error creating ximage");
   ls = get_ximage_bytes_per_line( ximage);
   data = get_ximage_data( ximage);
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U32 *d = (U32*)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> image_cache = ximage;
}

static void
create_image_cache_8_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   U16 lut[ 256];
   U16 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   PrimaXImage *ximage;

   create_rgb_to_16_lut( img-> palSize, img-> palette, lut);

   ximage = prepare_ximage( w, h, false);
   if ( !ximage) croak( "create_image_cache_8_to_16(): error creating ximage");
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U16 *d = (U16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> image_cache = ximage;
}

static void
create_image_cache_8_to_24( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   unsigned long lut[ 256];
   U32 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   PrimaXImage *ximage;

   create_rgb_to_24_lut( img-> palSize, img-> palette, lut);

   ximage = prepare_ximage( w, h, false);
   if ( !ximage) croak( "create_image_cache_8_to_24(): error creating ximage");
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U32 *d = (U32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
         ((unsigned char *)d)--;
      }
   }

   IMG-> image_cache = ximage;
}

static void
create_image_cache_8_to_32( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   unsigned long lut[ 256];
   U32 *data;
   int x, y;
   int ls;
   int h = img-> h, w = img-> w;
   PrimaXImage *ximage;

   create_rgb_to_24_lut( img-> palSize, img-> palette, lut);

   ximage = prepare_ximage( w, h, false);
   if ( !ximage) croak( "create_image_cache_8_to_32(): error creating ximage");
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U32 *d = (U32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> image_cache = ximage;
}

static void
create_image_cache_24_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   static U16 lur[256], lub[256], lug[256];
   static Bool initialize = true;
   U16 *data;
   int x, y;
   int i;
   RGBColor pal[256];
   int ls;
   int h = img-> h, w = img-> w;
   PrimaXImage *ximage;

   if ( initialize) {
      for ( i = 0; i < 256; i++) {
	 pal[i]. r = i; pal[i]. g = 0; pal[i]. b = 0;
      }
      create_rgb_to_16_lut( 256, pal, lur);
      for ( i = 0; i < 256; i++) {
	 pal[i]. r = 0; pal[i]. g = i; pal[i]. b = 0;
      }
      create_rgb_to_16_lut( 256, pal, lug);
      for ( i = 0; i < 256; i++) {
	 pal[i]. r = 0; pal[i]. g = 0; pal[i]. b = i;
      }
      create_rgb_to_16_lut( 256, pal, lub);
      initialize = false;
   }

   ximage = prepare_ximage( w, h, false);
   if ( !ximage) croak( "create_image_cache_24_to_16(): error creating ximage");
   ls = get_ximage_bytes_per_line(ximage);
   data = get_ximage_data(ximage);

   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U16 *d = (U16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lub[line[0]] | lug[line[1]] | lur[line[2]];
	 line += 3;
      }
   }

   IMG-> image_cache = ximage;
}


void
prima_create_image_cache( PImage img, Handle drawable, Bool icon)
{
   PDrawableSysData IMG = X((Handle)img);
   Bool bit_cache = X(drawable)->flags.is_image && (PImage(drawable)->type & imBPP) == 1;

   if ( bit_cache) {
      if ( !IMG-> image_bit_cache) {
         if ( !img-> palette) {
            croak( "create_image_cache(): no palette, ouch!");
         }
         switch (img-> type & imBPP) {
         case 1:
            create_image_cache_1_to_1( img, drawable, false);
            break;
         default:
            croak( "create_image_cache(): unsupported BPP for drawing on bitmaps");
         }
      }
      if ( icon && !IMG-> icon_bit_cache) {
         create_image_cache_1_to_1( img, drawable, true);
      }
   } else {
      if ( !IMG-> image_cache) {
         if ( !img-> palette) {
            croak( "create_image_cache(): no palette, ouch!");
         }
         switch (img-> type & imBPP) {
         case 1:
            create_image_cache_1_to_1( img, drawable, false);
            break;
         case 4:
            switch (guts.depth) {
            case 16:
               create_image_cache_4_to_16( img);
               break;
            default:
               croak( "create_image_cache(): unsupported screen depth for 4-bit images");
            }
            break;
         case 8:
            switch (guts.depth) {
            case 16:
               create_image_cache_8_to_16( img);
               break;
            case 24:
               create_image_cache_8_to_24( img);
               break;
            case 32:
               create_image_cache_8_to_32( img);
               break;
            default:
               croak( "create_image_cache(): unsupported screen depth for 8-bit images");
            }
            break;
         case 24:
            switch (guts.depth) {
            case 16:
               create_image_cache_24_to_16( img);
               break;
            case 24:
               /* create_image_cache_24_to_24( img); */
               break;
            default:
               croak( "create_image_cache(): unsupported screen depth for 24-bit images");
            }
            break;
         default:
            croak( "create_image_cache(): unsupported BPP");
         }
      }
      if ( icon && !IMG-> icon_cache) {
         create_image_cache_1_to_1( img, drawable, true);
      }
   }
}

Bool
prima_create_icon_pixmaps( Handle self, Pixmap *xor, Pixmap *and)
{
   DEFXX;
   Pixmap p1, p2;
   PIcon icon = PIcon(self);

   prima_create_image_cache((PImage)icon, self, true);
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
   put_ximage( p2, XX-> gc, X(self)-> icon_bit_cache,
               0, 0, 0, 0, icon-> w, icon-> h);
   put_ximage( p1, XX-> gc, X(self)-> image_bit_cache,
               0, 0, 0, 0, icon-> w, icon-> h);
   prima_cleanup_drawable_after_painting( self);
   XX-> gdrawable = None;
   *xor = p1;
   *and = p2;
   return true;
}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   DEFXX;
   PDrawableSysData IMG = X(image);
   PImage img = PImage( image);
   unsigned long f = 0, b = 0;
   Bool icon = kind_of((Handle)img, CIcon);
   int func, ofunc;
   XGCValues gcv;
   Bool bit_cache = XX->flags.is_image && (PImage(self)->type & imBPP) == 1;

   /* 1) XXX - rop - correct support! */
   /* 2) XXX - Shared Mem Image Extension! */
   prima_create_image_cache( img, self, icon);
   SHIFT( x, y);
   if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) {
      warn( "apc_gp_put_image(): XGetGCValues() error");
   }
   ofunc = gcv. function;
   if ( icon) {
      func = GXand;
      f = XX-> fore. pixel;
      b = XX-> back. pixel;
      if ( bit_cache) {
         XSetForeground( DISP, XX-> gc, 1);
         XSetBackground( DISP, XX-> gc, 0);
      } else {
         XSetForeground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
      }
      if ( func != ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
      put_ximage( XX-> gdrawable, XX-> gc, bit_cache ? IMG-> icon_bit_cache : IMG-> icon_cache,
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
      XSetForeground( DISP, XX-> gc, IMG-> bitmap_fore. pixel);
      XSetBackground( DISP, XX-> gc, IMG-> bitmap_back. pixel);
      XCHECKPOINT;
   }
   put_ximage( XX-> gdrawable, XX-> gc, bit_cache ? IMG-> image_bit_cache : IMG-> image_cache,
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
calc_masks_and_lut_16_to_24( unsigned long mask,
			     unsigned long *mask1,
			     unsigned long *mask2,
			     int *bit_count,
			     unsigned char *lut)
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
   static unsigned char lur[256], lub[256], lug[256];  /* is ``static'' reliable here?? */
   static Bool initialize = true;
   static unsigned long rm1, bm1, gm1, rm2, bm2, gm2;
   static int rbc, bbc, gbc;
   int y, x, h, w;
   U16 *d;
   unsigned char *line;

   if ( initialize) {
      Visual *v = DefaultVisual( DISP, SCREEN);

      calc_masks_and_lut_16_to_24( v-> red_mask, &rm1, &rm2, &rbc, lur);
      calc_masks_and_lut_16_to_24( v-> green_mask, &gm1, &gm2, &gbc, lug);
      calc_masks_and_lut_16_to_24( v-> blue_mask, &bm1, &bm2, &bbc, lub);

      initialize = false;
   }

   h = img-> h; w = img-> w;
   for ( y = 0; y < h; y++) {
      d = (U16 *)(i-> data + (h-y-1)*i-> bytes_per_line);
      line = img-> data + y*img-> lineSize;
      for ( x = 0; x < w; x++) {
	 *line++ = lub[(*d & bm1) >> bbc];
	 *line++ = lug[(*d & gm1) >> gbc];
	 *line++ = lur[(*d & rm1) >> rbc];
	 d++;
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

         target_depth = guts. depth;
         if ( target_depth == 16)
            target_depth = 24;
         if (( img-> type & imBPP) != target_depth) {
            CImage( self)-> create_empty( self, img-> w, img-> h, target_depth);
         }
         if ( guts. depth != target_depth) {
            switch ( guts. depth) {
            case 16:
               switch ( target_depth) {
               case 24:
                  convert_16_to_24( i, img);
                  break;
               default: goto slurp_image_unsupported_depth;
               }
               break;
slurp_image_unsupported_depth:
            default:
               XDestroyImage( i);
               croak( "slurp_image(): unsupported depth-target depth combination");
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

extern void
ic_stretch( Handle, Byte *, int, int, Bool, Bool);

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
           uint16_t *source, int source_line_size,
           uint16_t *target, int targetwidth, int targetheight,
           int target_line_size)
{
   croak( "stretch_1: not implemented yet");
}

static void
stretch_16( const StretchSeed *xseed, const StretchSeed *yseed,
            Bool xreverse, Bool yreverse,
            Bool xshrink, Bool yshrink,
            uint16_t *source, int source_line_size,
            uint16_t *target, int targetwidth, int targetheight,
            int target_line_size)
{
   Fixed xcount, ycount;
   Fixed xstep, ystep;
   int xlast, ylast;
   int xfirst, yfirst;
   int x;
   uint16_t *last_source = nil;

   ycount = yseed-> count;
   ystep  = yseed-> step;
   ylast  = yseed-> last;
   yfirst = yseed-> source;

   source = (uint16_t*)((Byte*)source + yfirst * source_line_size);
   if (yreverse) {
      target = (uint16_t*)((Byte*)target + (targetheight-1)*target_line_size);
      target_line_size = - target_line_size;
   }

   if ( yshrink) {
      while ( targetheight) {
      }
   } else {
      while ( targetheight) {
         if ( ycount.i.i > ylast) {
            source = (uint16_t*)((Byte*)source + source_line_size);
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
         target = (uint16_t*)((Byte*)target + target_line_size);
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
   } else {
      asize  =  tsize;
      cstart = *clipstart;
      cend   =  cstart + *clipsize;
      t      =  0;
      dt     =  1;
      if ( cstart < 0)          cstart = 0;
      if ( cend > asize)        cend = asize;
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

Bool
apc_gp_stretch_image( Handle self, Handle image,
		      int x, int y, int xFrom, int yFrom,
		      int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   DEFXX;
   PImage img = PImage( image);
   PDrawableSysData IMG = X(image);
   // unsigned long f = 0, b = 0;
   XRectangle cr;
   StretchSeed xseed, yseed;
   int xclipstart, xclipsize;
   int yclipstart, yclipsize;
   Byte *data;
   int tls;
   int sls;
   PrimaXImage *stretch, *image_cache;
   Bool bit_cache;

   if ( xDestLen == xLen && yDestLen == yLen) {
      apc_gp_put_image( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
      return true;
   }

   /* 1) XXX - rop - correct support! */
   /* 2) XXX - Shared Mem Image Extension! */
   SHIFT( x, y);
   y = XX->size.y - y - yDestLen;
   yFrom = img-> h - yFrom - yLen;

   prima_gp_get_clip_rect( self, &cr);
   xclipstart = cr. x - x;
   xclipsize = cr. width;
   yclipstart = cr. y - y;
   yclipsize = cr. height;

   if ( xclipstart + xclipsize <= 0 || yclipstart + yclipsize <= 0) return true;
   stretch_calculate_seed( xLen, xDestLen, &xclipstart, &xclipsize, guts. depth, &xseed);
   stretch_calculate_seed( yLen, yDestLen, &yclipstart, &yclipsize, 8, &yseed);
   if ( xclipsize <= 0 || yclipsize <= 0) return true;

   stretch = prepare_ximage( xclipsize, yclipsize, false);
   if ( !stretch) croak( "apc_gp_stretch_image(): error creating ximage");

   prima_create_image_cache( img, self, false);
   bit_cache = XX->flags.is_image && (PImage(self)->type & imBPP) == 1;
   image_cache = bit_cache ? IMG-> image_bit_cache : IMG-> image_cache;

   tls = get_ximage_bytes_per_line(stretch);
   sls = get_ximage_bytes_per_line( image_cache);
   data = get_ximage_data(stretch);

   switch ( img-> type & imBPP) {
   case 1:
      stretch_1( &xseed, &yseed,
                 xDestLen < 0, yDestLen < 0,
                 xDestLen < 0 ? -xDestLen < xLen : xDestLen < xLen,
                 yDestLen < 0 ? -yDestLen < yLen : yDestLen < yLen,
                 (void*)(get_ximage_data(image_cache) + yFrom*sls + xFrom*2), sls,
                 (void*)data, xclipsize, yclipsize, tls);
      break;
   default:
      switch ( bit_cache ? 1 : guts.depth) {
      case 16:
         stretch_16( &xseed, &yseed,
                     xDestLen < 0, yDestLen < 0,
                     xDestLen < 0 ? -xDestLen < xLen : xDestLen < xLen,
                     yDestLen < 0 ? -yDestLen < yLen : yDestLen < yLen,
                     (void*)(get_ximage_data(image_cache) + yFrom*sls + xFrom*2), sls,
                     (void*)data, xclipsize, yclipsize, tls);
         break;
      default:
         croak( "apc_gp_stretch_image(): internal error 16N16");
      }
   }
   put_ximage( XX-> gdrawable, XX-> gc, stretch,
	      0, 0, x+xclipstart, y + yclipstart, xclipsize, yclipsize);
   destroy_ximage( stretch);
   XCHECKPOINT;
   return true;
}

