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
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif


#define var (( PImage) self)

#define dEDIFF_ARGS \
   int * err_buf
#define EDIFF_INIT  \
    if (!(err_buf = malloc(( width + 2) * 3 * sizeof( int)))) return;\
    memset( err_buf, 0,( width + 2) * 3 * sizeof( int));
#define EDIFF_DONE free(err_buf)
#define EDIFF_CONV err_buf

#define BCPARMS      self, dstData, dstPal, dstType, dstPalSize, palSize_only
#define FILL_PALETTE(_pal,_palsize,_maxpalsize,_colorref)\
   fill_palette(self,palSize_only,dstPal,dstPalSize,_pal,_palsize,_maxpalsize,_colorref)

/* Mono */

static void
fill_palette( Handle self, Bool palSize_only, RGBColor * dstPal, int * dstPalSize, 
              RGBColor * fillPalette, int fillPalSize, int maxPalSize, Byte * colorref)
{
   Bool do_colormap = 1;
   if ( palSize_only) { 
      if ( var-> palSize > *dstPalSize)
         cm_squeeze_palette( var-> palette, var-> palSize, dstPal, *dstPalSize);
      else if ( *dstPalSize > fillPalSize + var-> palSize) {
         memcpy( dstPal, var-> palette, var-> palSize * sizeof(RGBColor));
         memcpy( dstPal + var-> palSize, fillPalette, fillPalSize * sizeof(RGBColor));
         memset( dstPal + var-> palSize + fillPalSize, 0, (*dstPalSize - fillPalSize - var-> palSize) * sizeof(RGBColor));
         do_colormap = 0;
      } else {
         memcpy( dstPal, var-> palette, var-> palSize * sizeof(RGBColor));
         cm_squeeze_palette( fillPalette, fillPalSize, dstPal + var-> palSize, *dstPalSize - var-> palSize);
         do_colormap = 0;
      }
   } else if ( *dstPalSize != 0) {
      if ( *dstPalSize > maxPalSize) 
         cm_squeeze_palette( dstPal, *dstPalSize, dstPal, *dstPalSize = maxPalSize);
   } else if ( var-> palSize > maxPalSize) {
      cm_squeeze_palette( var-> palette, var-> palSize, dstPal, *dstPalSize = maxPalSize);
   } else {
      memcpy( dstPal, var-> palette, (*dstPalSize = var-> palSize) * sizeof(RGBColor));
      do_colormap = 0;
   }
   if ( colorref) {
      if ( do_colormap)
         cm_fill_colorref( var->palette, var-> palSize, dstPal, *dstPalSize, colorref);
      else
         memcpy( colorref, map_stdcolorref, 256);
   }
}

BC( mono, mono, None)
{
   int j, ws, mask;
   dBCARGS;
   BCWARN;

   if ( palSize_only || *dstPalSize == 0) 
      memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof( RGBColor));
   
   if ((
         (var->palette[0].r + var->palette[0].g + var->palette[0].b) >
         (var->palette[1].r + var->palette[1].g + var->palette[1].b)
       ) == (
         (dstPal[0].r + dstPal[0].g + dstPal[0].b) >
         (dstPal[1].r + dstPal[1].g + dstPal[1].b) 
       )) {
      if ( dstData != var-> data)
         memcpy( dstData, var-> data, var-> dataSize);
   } else {
      /* preserve off-width zeros */
      ws = width >> 3;
      if ((width & 7) == 0) {
         ws--;
         mask = 0xff;
      } else
         mask = (0xff00 >> (width & 7)) & 0xff;
      for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
         for ( j = 0; j < ws; j++) dstData[j] =~ srcData[j];
         dstData[ws] = (~srcData[j]) & mask;
      }
   }
}

BC( mono, mono, Optimized)
{
   dBCARGS;
   U16 * tree;
   Byte * buf;
   dEDIFF_ARGS;
   BCWARN;

   FILL_PALETTE( stdmono_palette, 2, 2, nil);

   if ( !( buf = malloc( width))) goto FAIL;
   
   EDIFF_INIT;
   if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_mono_byte( srcData, buf, width); 
      bc_byte_op( buf, buf, width, tree, var-> palette, dstPal, EDIFF_CONV);
      bc_byte_mono_cr( buf, dstData, width, map_stdcolorref); 
   }
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_mono_mono_ictNone(BCPARMS);
}

BC( mono, nibble, None)
{
   dBCARGS;
   BCWARN;
   FILL_PALETTE( stdmono_palette, 2, 16, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_mono_nibble_cr( BCCONV, colorref);
}

BC( mono, byte, None)
{
   dBCARGS;
   BCWARN;
   FILL_PALETTE( stdmono_palette, 2, 256, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_mono_byte_cr( BCCONV, colorref);
}

BC( mono, graybyte, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_mono_graybyte( BCCONV, var->palette);
}

BC( mono, rgb, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_mono_rgb( BCCONV, var->palette);
}

/* Nibble */

BC( nibble, mono, None)
{
   dBCARGS;
   BCWARN;
   FILL_PALETTE( stdmono_palette, 2, 2, colorref);
   cm_fill_colorref( var->palette, var-> palSize, dstPal, *dstPalSize, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_nibble_mono_cr( BCCONV, colorref);
}

BC( nibble, mono, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_nibble_mono_ht( BCCONV, var->palette, i);
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( nibble, mono, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_nibble_mono_ed( BCCONV, var->palette, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( nibble, mono, Optimized)
{
   dBCARGS;
   U16 * tree;
   Byte * buf;
   dEDIFF_ARGS;
   BCWARN;

   FILL_PALETTE( stdmono_palette, 2, 2, nil);
   if ( !( buf = malloc( width))) goto FAIL;
   EDIFF_INIT;
   if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_nibble_byte( srcData, buf, width);
      bc_byte_op( buf, buf, width, tree, var-> palette, dstPal, EDIFF_CONV);
      bc_byte_mono_cr( buf, dstData, width, map_stdcolorref);
   }
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_nibble_mono_ictErrorDiffusion(BCPARMS);
} 

BC( nibble, nibble, None)
{
   dBCARGS;
   int j, w = (width >> 1) + (width & 1);
   BCWARN;
   FILL_PALETTE( cubic_palette16, 16, 16, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      for ( j = 0; j < w; j++)
         dstData[j] = (colorref[srcData[j] >> 4] << 4) | colorref[srcData[j] & 0xf]; 
   }
}

BC( nibble, nibble, Optimized)
{
   dBCARGS;
   U16 * tree;
   Byte * buf;
   dEDIFF_ARGS;
   BCWARN;

   FILL_PALETTE( cubic_palette16, 16, 16, nil);

   if ( !( buf = malloc( width))) goto FAIL;
   
   EDIFF_INIT;
   if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_nibble_byte( srcData, buf, width); 
      bc_byte_op( buf, buf, width, tree, var-> palette, dstPal, EDIFF_CONV);
      bc_byte_nibble_cr( buf, dstData, width, map_stdcolorref); 
   }
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_nibble_nibble_ictNone(BCPARMS);
}

BC( nibble, byte, None)
{
   dBCARGS;
   BCWARN;
   FILL_PALETTE( cubic_palette, 216, 256, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_nibble_byte_cr( BCCONV, colorref);
}

BC( nibble, graybyte, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_nibble_graybyte( BCCONV, var->palette);
}

BC( nibble, rgb, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_nibble_rgb( BCCONV, var->palette);
}

/* Byte */
BC( byte, mono, None)
{
   dBCARGS;
   BCWARN;
   FILL_PALETTE( stdmono_palette, 2, 2, colorref);
   cm_fill_colorref( var->palette, var-> palSize, dstPal, *dstPalSize, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_mono_cr( BCCONV, colorref);
}

BC( byte, mono, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_mono_ht( BCCONV, var->palette, i);
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( byte, mono, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_mono_ed( BCCONV, var->palette, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( byte, mono, Optimized)
{
   dBCARGS;
   U16 * tree;
   Byte * buf;
   dEDIFF_ARGS;
   BCWARN;

   FILL_PALETTE( stdmono_palette, 2, 2, nil);
   if ( !( buf = malloc( width))) goto FAIL;
   EDIFF_INIT;
   if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_byte_op( srcData, buf, width, tree, var-> palette, dstPal, EDIFF_CONV);
      bc_byte_mono_cr( buf, dstData, width, map_stdcolorref);
   }
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_byte_mono_ictErrorDiffusion(BCPARMS);
} 

BC( byte, nibble, None)
{
   dBCARGS;
   BCWARN;
   FILL_PALETTE( cubic_palette16, 16, 16, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_nibble_cr( BCCONV, colorref);
}

BC( byte, nibble, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_nibble_ht( BCCONV, var->palette, i);
   memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( byte, nibble, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_nibble_ed( BCCONV, var->palette, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( byte, nibble, Optimized)
{
   dBCARGS;
   U16 * tree;
   Byte * buf, hist[256];
   RGBColor new_palette[256];
   int j, new_pal_size = 0;
   dEDIFF_ARGS;
   BCWARN;

   if ( *dstPalSize == 0 || palSize_only) {
      int lim = palSize_only ? *dstPalSize : 16;
      memset( hist, 0, sizeof( hist));
      for ( i = 0; i < height; i++) {
         Byte * d = srcData + srcLine * i;
         for ( j = 0; j < width; j++, d++) 
            if ( hist[*d] == 0) {
               hist[*d] = 1;
               new_palette[new_pal_size++] = var-> palette[*d];
               if ( new_pal_size == 256) goto END_HIST_LOOP;
            }
      }
   END_HIST_LOOP:
      if ( new_pal_size > lim) {
         cm_squeeze_palette( new_palette, new_pal_size, new_palette, lim);
         new_pal_size = lim;
      }
   } else
      memcpy( new_palette, dstPal, ( new_pal_size = *dstPalSize) * sizeof(RGBColor));

   if ( !( buf = malloc( width))) goto FAIL;
   EDIFF_INIT;
   if (!( tree = cm_study_palette( new_palette, new_pal_size))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_byte_op( srcData, buf, width, tree, var-> palette, new_palette, EDIFF_CONV);
      bc_byte_nibble_cr( buf, dstData, width, map_stdcolorref);
   }
   memcpy( dstPal, new_palette, new_pal_size * sizeof(RGBColor));
   *dstPalSize = new_pal_size;
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_byte_nibble_ictErrorDiffusion(BCPARMS);
} 

BC( byte, byte, None)
{
   dBCARGS;
   int j;
   BCWARN;
   FILL_PALETTE( cubic_palette, 216, 256, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      for ( j = 0; j < width; j++)
         dstData[j] = colorref[srcData[j]];
   }
}

BC( byte, byte, Optimized)
{
   dBCARGS;
   U16 * tree;
   dEDIFF_ARGS;
   BCWARN;

   FILL_PALETTE( cubic_palette, 216, 256, nil);

   EDIFF_INIT;
   if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
      EDIFF_DONE;
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) 
      bc_byte_op( srcData, dstData, width, tree, var-> palette, dstPal, EDIFF_CONV);
   free( tree);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_byte_byte_ictNone(BCPARMS);
}

BC( byte, graybyte, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_graybyte( BCCONV, var->palette);
}

BC( byte, rgb, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_rgb( BCCONV, var->palette);
}

/* Graybyte */
BC( graybyte, mono, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_graybyte_mono_ht( BCCONV, i);
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( graybyte, mono, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_byte_mono_ed( BCCONV, std256gray_palette, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( graybyte, nibble, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_graybyte_nibble_ht( BCCONV, i);
   memcpy( dstPal, std16gray_palette, sizeof( std16gray_palette));
   *dstPalSize = 16;
}

BC( graybyte, nibble, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_graybyte_nibble_ed( BCCONV, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, std16gray_palette, sizeof( std16gray_palette));
   *dstPalSize = 16;
}

BC( graybyte, rgb, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_graybyte_rgb( BCCONV);
}

/* RGB */
BC( rgb, mono, None)
{
   dBCARGS;
   Byte * convBuf = allocb( width);
   BCWARN;
   if ( !convBuf) return;
   cm_fill_colorref(( PRGBColor) map_RGB_gray, 256, stdmono_palette, 2, colorref);
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
   {
      bc_rgb_graybyte( srcData, convBuf, width);
      bc_byte_mono_cr( convBuf, dstData, width, colorref);
   }
   free( convBuf);
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( rgb, mono, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_mono_ht( BCCONV, i);
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( rgb, mono, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_mono_ed( BCCONV, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( rgb, mono, Optimized)
{
   dBCARGS;
   Byte * buf;
   U16 * tree;
   dEDIFF_ARGS;
   BCWARN;

   if ( palSize_only) goto FAIL;
   if ( !( buf = malloc( width))) goto FAIL;
   EDIFF_INIT;
   if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_rgb_byte_op(( RGBColor *) srcData, buf, width, tree, dstPal, EDIFF_CONV);
      bc_byte_mono_cr( buf, dstData, width, map_stdcolorref);
   }
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;

FAIL:   
   ic_rgb_mono_ictErrorDiffusion(BCPARMS);
}

BC( rgb, nibble, None)
{
   dBCARGS;
   BCWARN;
   memcpy( dstPal, cubic_palette16, sizeof( cubic_palette16));
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_nibble(BCCONV);
   *dstPalSize = 16;
}

BC( rgb, nibble, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_nibble_ht( BCCONV, i);
   memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( rgb, nibble, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_nibble_ed( BCCONV, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( rgb, nibble, Optimized)
{
   dBCARGS;
   U16 * tree;
   Byte * buf;
   RGBColor new_palette[16];
   int new_pal_size = 16;
   dEDIFF_ARGS;
   BCWARN;

   if ( *dstPalSize == 0 || palSize_only) {
      if ( palSize_only) new_pal_size = *dstPalSize;
      if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size)) 
         goto FAIL;
   } else
      memcpy( new_palette, dstPal, ( new_pal_size = *dstPalSize) * sizeof(RGBColor));

   if ( !( buf = malloc( width))) goto FAIL;
   EDIFF_INIT;
   if (!( tree = cm_study_palette( new_palette, new_pal_size))) {
      EDIFF_DONE;
      free( buf);
      goto FAIL;
   }
   memcpy( dstPal, new_palette, new_pal_size * 3);
   *dstPalSize = new_pal_size;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
      bc_rgb_byte_op(( RGBColor *) srcData, buf, width, tree, dstPal, EDIFF_CONV);
      bc_byte_nibble_cr( buf, dstData, width, map_stdcolorref);
   }
   free( tree);
   free( buf);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_rgb_nibble_ictErrorDiffusion(BCPARMS);
} 

BC( rgb, byte, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_byte( BCCONV);
   memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( rgb, byte, Ordered)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_byte_ht( BCCONV, i);
   memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( rgb, byte, ErrorDiffusion)
{
   dBCARGS;
   dEDIFF_ARGS;
   BCWARN;
   EDIFF_INIT;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_byte_ed( BCCONV, EDIFF_CONV);
   EDIFF_DONE;
   memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}   

BC( rgb, byte, Optimized)
{
   dBCARGS;
   U16 * tree;
   RGBColor new_palette[768];
   int new_pal_size = 256;
   dEDIFF_ARGS;
   BCWARN;
   if ( *dstPalSize == 0 || palSize_only) {
      if ( palSize_only) new_pal_size = *dstPalSize;
      if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size)) 
         goto FAIL;
   } else
      memcpy( new_palette, dstPal, ( new_pal_size = *dstPalSize) * sizeof(RGBColor));

   EDIFF_INIT;
   if (!( tree = cm_study_palette( new_palette, new_pal_size))) {
      EDIFF_DONE;
      goto FAIL;
   }
   memcpy( dstPal, new_palette, new_pal_size * 3);
   *dstPalSize = new_pal_size;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_byte_op(( RGBColor *) srcData, dstData, width, tree, dstPal, EDIFF_CONV);
   free( tree);
   EDIFF_DONE;
   return;
   
FAIL:  
   ic_rgb_byte_ictErrorDiffusion(BCPARMS);
} 

BC( rgb, graybyte, None)
{
   dBCARGS;
   BCWARN;
   for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine)
      bc_rgb_graybyte( BCCONV);
}

#ifdef __cplusplus
}
#endif
