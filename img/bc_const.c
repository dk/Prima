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

#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define map_RGB_gray ((Byte*)std256gray_palette)

Byte     map_stdcolorref    [ 256];
Byte     div51              [ 256];
Byte     div17              [ 256];
Byte     mod51              [ 256];
Byte     mod17mul3          [ 256];
RGBColor cubic_palette      [ 256];
RGBColor cubic_palette8     [   8];
RGBColor cubic_palette16    [  16] =
{
   {0,0,0}, {0,128,128}, {128,0,128}, {0,0,255},
   {128,128,0}, {0,255,0}, {255,0,0}, {85,85,85},
   {170,170,170},{0,255,255},{255,0,255},{128,128,255},
   {255,255,0},{128,255,128},{255,128,128},{255,255,255}
};
RGBColor stdmono_palette    [   2] = {{ 0, 0, 0}, { 0xFF, 0xFF, 0xFF}};
RGBColor std16gray_palette  [  16];
RGBColor std256gray_palette [ 256];
Byte     map_halftone8x8_51 [  64] = {
    0, 38,  9, 47,  2, 40, 11, 50,
   25, 12, 35, 22, 27, 15, 37, 24,
    6, 44,  3, 41,  8, 47,  5, 43,
   31, 19, 28, 15, 34, 21, 31, 18,
    1, 39, 11, 49,  0, 39, 10, 48,
   27, 14, 36, 23, 26, 13, 35, 23,
    7, 46,  4, 43,  7, 45,  3, 42,
   33, 20, 30, 17, 32, 19, 29, 16
};
Byte     map_halftone8x8_64 [  64] = {
    0, 47, 12, 59,  3, 50, 15, 62,
   32, 16, 43, 28, 34, 19, 46, 31,
    8, 55,  4, 51, 11, 58,  7, 54,
   39, 24, 35, 20, 42, 27, 38, 23,
    2, 49, 14, 61,  1, 48, 13, 60,
   33, 18, 45, 30, 32, 17, 44, 29,
   10, 57,  6, 53,  9, 56,  5, 52,
   41, 26, 37, 22, 40, 25, 36, 21
};

void
cm_init_colormap( void)
{
   int i;
   for ( i = 0; i < 256; i++)
   {
      map_RGB_gray[ i * 3] = map_RGB_gray[ i * 3 + 1] = map_RGB_gray[ i * 3 + 2] = i;
      map_stdcolorref[ i] = i;
      div51[ i] = i / 51;
      div17[ i] = i / 17;
      mod51[ i] = i % 51;
      mod17mul3[ i] = ( i % 17) * 3;
   }
   for ( i = 0; i < 16; i++)
      std16gray_palette[ i]. r = std16gray_palette[ i]. g = std16gray_palette[ i]. b = i * 17;
   {
      int b, g, r;
      for ( b = 0; b < 6; b++) for ( g = 0; g < 6; g++) for ( r = 0; r < 6; r++)
      {
/*       cubic_palette[ b + g * 6 + r * 36] =( RGBColor) { b * 51, g * 51, r *
 *       51 }; */
	 int idx = b + g * 6 + r * 36;
	 cubic_palette[ idx]. b = b * 51;
	 cubic_palette[ idx]. g = g * 51;
	 cubic_palette[ idx]. r = r * 51;
      }
   }
   {
      int b, g, r;
      for ( b = 0; b < 2; b++) for ( g = 0; g < 2; g++) for ( r = 0; r < 2; r++)
      {
/*       cubic_palette8[ b + g * 2 + r * 4] = ( RGBColor) { b * 255, g * 255,
 *       r * 255 }; */
	 int idx = b + g * 2 + r * 4;
	 cubic_palette8[ idx]. b = b * 255;
	 cubic_palette8[ idx]. g = g * 255;
	 cubic_palette8[ idx]. r = r * 255;
      }
   }
}

void
cm_reverse_palette( PRGBColor source, PRGBColor dest, int colors)
{
   while( colors--)
   {
      register Byte r = source[0].r;
      register Byte b = source[0].b;
      register Byte g = source[0].g;
      dest[0].r = b;
      dest[0].b = r;
      dest[0].g = g;
      source++;
      dest++;
   }
}

void
cm_squeeze_palette( PRGBColor source, int srcColors, PRGBColor dest, int destColors)
{
   if (( srcColors == 0) || ( destColors == 0)) return;
   if ( srcColors <= destColors)
      memcpy( dest, source, srcColors * sizeof( RGBColor));
   else
   {
      int tolerance = 0;
      int colors    = srcColors;

      PRGBColor buf = allocn( RGBColor, srcColors);
      if (!buf) return;
      memcpy( buf, source, srcColors * sizeof( RGBColor));
      while (1)
      {
         int i;
         int tt2 = tolerance*tolerance;
	
         for ( i = 0; i < colors - 1; i++)
         {
	    register int r = buf[i]. r;
	    register int g = buf[i]. g;
	    register int b = buf[i]. b;
            int j;
            register PRGBColor next = buf + i + 1;
	
            for ( j = i + 1; j < colors; j++)
            {
               if (( ( next-> r - r)*( next-> r - r) +
                     ( next-> g - g)*( next-> g - g) +
                     ( next-> b - b)*( next-> b - b)) <= tt2)
               {
                  buf[ j] = buf[ --colors];
                  if ( colors <= destColors) goto Enough;
               }
               next++;
            }
         }
	 tolerance += 2;
      }
Enough:
      memcpy( dest, buf, destColors * sizeof( RGBColor));
      free( buf);
   }
}

Byte
cm_nearest_color( RGBColor color, int palSize, PRGBColor palette)
{
   int diff = INT_MAX, cdiff = 0;
   Byte ret = 0;
   while( palSize--)
   {
      int dr=abs( (int)color. r - (int)palette[ palSize]. r),
          dg=abs( (int)color. g - (int)palette[ palSize]. g),
          db=abs( (int)color. b - (int)palette[ palSize]. b);
      cdiff=dr*dr+dg*dg+db*db;
      if ( cdiff < diff)
      {
         ret = palSize;
         diff = cdiff;
         if ( cdiff == 0) break;
      }
   }
   return ret;
}

void
cm_fill_colorref( PRGBColor fromPalette, int fromColorCount, PRGBColor toPalette, int toColorCount, Byte * colorref)
{
   while( fromColorCount--)
      colorref[ fromColorCount] =
         cm_nearest_color( fromPalette[ fromColorCount], toColorCount, toPalette);
}


#ifdef __cplusplus
}
#endif
