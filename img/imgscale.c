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

#define BS_BYTEIMPACT( type)                                                        \
void bs_##type##_in( type * srcData, type * dstData, int w, int x, int absx, long step)  \
{                                                                                   \
   Fixed count = {0};                                                               \
   int   last = 0;                                                                  \
   int   i;                                                                         \
   int   j    = ( x == absx) ? 0 : ( absx - 1);                                     \
   int   inc  = ( x == absx) ? 1 : -1;                                              \
   dstData[j] = srcData[0];                                                         \
   j += inc;                                                                        \
   for ( i = 0; i < w; i++)                                                         \
   {                                                                                \
      if ( count.i.i > last)                                                        \
      {                                                                             \
         dstData[j] = srcData[i];                                                   \
         j += inc;                                                                  \
         last = count. i. i;                                                        \
      }                                                                             \
      count. l += step;                                                             \
   }                                                                                \
}

#define BS_BYTEEXPAND( type)                                                        \
void bs_##type##_out( type * srcData, type * dstData, int w, int x, int absx, long step) \
{                                                                                   \
   Fixed count = {0};                                                               \
   int   i;                                                                         \
   int   j    = ( x == absx) ? 0 : ( absx - 1);                                     \
   int   inc  = ( x == absx) ? 1 : -1;                                              \
   int   last = 0;                                                                  \
   for ( i = 0; i < absx; i++)                                                      \
   {                                                                                \
      if ( count. i. i > last)                                                      \
      {                                                                             \
         last = count. i. i;                                                        \
         srcData++;                                                                 \
      }                                                                             \
      count. l += step;                                                             \
      dstData[j] = *srcData;                                                        \
      j += inc;                                                                     \
   }                                                                                \
}

BS_BYTEEXPAND( uint8_t)
BS_BYTEEXPAND( int16_t)
BS_BYTEEXPAND( RGBColor)
BS_BYTEEXPAND( int32_t)
BS_BYTEEXPAND( float)
BS_BYTEEXPAND( double)
BS_BYTEEXPAND( Complex)
BS_BYTEEXPAND( DComplex)

BS_BYTEIMPACT( uint8_t)
BS_BYTEIMPACT( int16_t)
BS_BYTEIMPACT( RGBColor)
BS_BYTEIMPACT( int32_t)
BS_BYTEIMPACT( float)
BS_BYTEIMPACT( double)
BS_BYTEIMPACT( Complex)
BS_BYTEIMPACT( DComplex)


void
bs_mono_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed count = {0};
   int   last   = 0;
   register int i, j;
   register U16 xd, xs;

   if ( x == absx)
   {
      xd = (( xs = srcData[0]) >> 7) & 1;
      j = 1;
      for ( i = 0; i < w; i++)
      {
         if (( i & 7) == 0) xs = srcData[ i >> 3];
         xs <<= 1;
         if ( count.i.i > last)
         {
            if (( j & 7) == 0) dstData[ ( j - 1) >> 3] = xd;
            xd <<= 1;
            xd |= ( xs >> 8) & 1;
            j++;
            last = count. i. i;
         }
         count. l += step;
      }
      i = j & 7;
      dstData[( j - 1) >> 3] = xd << ( i ? ( 8 - i) : 0);
   } else {
      j = absx - 1;
      xd = ( xs = srcData[ j >> 3]) & 0x80;
      for ( i = 0; i < w; i++)
      {
         if (( i & 7) == 0) xs = srcData[ i >> 3];
         xs <<= 1;
         if ( count.i.i > last)
         {
            if (( j & 7) == 0) dstData[ ( j + 1) >> 3] = xd;
            xd >>= 1;
            xd |= ( xs >> 1) & 0x80;
            j--;
            last = count. i. i;
         }
         count. l += step;
      }
      dstData[( j + 1) >> 3] = xd;
   }
}

void
bs_mono_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed    count = {0};
   register int i, j = 0;
   register U16 xd = 0, xs;
   int last = 0;

   if ( x == absx)
   {
      xs = srcData[0];
      for ( i = 0; i < absx; i++)
      {
         if ( count. i. i > last)
         {
            last = count.i.i;
            xs <<= 1;
            j++;
            if (( j & 7) == 0) xs = srcData[ j >> 3];
         }
         count.l += step;
         xd <<= 1;
         xd |= ( xs >> 7) & 1;
         if ((( i + 1) & 7) == 0) dstData[ i >> 3] = xd;
      }
      if ( i & 7) dstData[ i >> 3] = xd << ( 8 - ( i & 7));
      /* dstData[ i >> 3] = xd << (( i & 7) ? ( 8 - ( i & 7)) : 0); */
   } else {
      register int k = absx;
      xs = srcData[ j >> 3];
      for ( i = 0; i < absx; i++)
      {
         if ( count. i. i > last)
         {
            last = count.i.i;
            xs <<= 1;
            j++;
            if (( j & 7) == 0) xs = srcData[ j >> 3];
         }
         count.l += step;
         xd >>= 1;
         xd |= xs & 0x80;
         k--;
         if (( k & 7) == 0) dstData[( k + 1) >> 3] = xd;
      }
      dstData[( k + 0) >> 3] = xd;
   }
}

/* nibble stretching functions are requiring *dstData filled with zeros */

void bs_nibble_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed count = {0};
   int   last = 0;
   int   i;
   int   j    = ( x == absx) ? 0 : ( absx - 1);
   int   inc  = ( x == absx) ? 1 : -1;
   dstData[ j >> 1] |= ( j & 1) ? ( srcData[ 0] >> 4) : ( srcData[ 0] & 0xF0);
   j += inc;
   for ( i = 0; i < w; i++)
   {
      if ( count.i.i > last)
      {
         if ( i & 1)
            dstData[ j >> 1] |= ( j & 1) ? ( srcData[ i >> 1] & 0x0F) : ( srcData[ i >> 1] << 4);
         else
            dstData[ j >> 1] |= ( j & 1) ? ( srcData[ i >> 1] >> 4) : ( srcData[ i >> 1] & 0xF0);
         j += inc;
         last = count. i. i;
      }
      count. l += step;
   }
}

void bs_nibble_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed count = {0};
   int   i, k = 0;
   int   j    = ( x == absx) ? 0 : ( absx - 1);
   int   inc  = ( x == absx) ? 1 : -1;
   int   last = 0;
   for ( i = 0; i < absx; i++)
   {
      if ( count. i. i > last)
      {
         if (( k & 1) == 1) srcData++;
         k++;
         last = count. i. i;
      }
      count. l += step;
      if ( k & 1)
         dstData[ j >> 1] |= ( j & 1) ? ( *srcData & 0x0F) : ( *srcData << 4);
      else
         dstData[ j >> 1] |= ( j & 1) ? ( *srcData >> 4) : ( *srcData & 0xF0);
      j += inc;
   }
}

void
ic_stretch( int type, Byte * srcData, int srcW, int srcH, Byte * dstData, int w, int h, Bool xStretch, Bool yStretch)
{
   int  absh = h < 0 ? -h : h;
   int  absw = w < 0 ? -w : w;
   int  srcLine = (( srcW *  ( type & imBPP) + 31) / 32) * 4;
   int  dstLine = (( absw  * ( type & imBPP) + 31) / 32) * 4;
   Fixed xstep, ystep, count;
   int last = 0;
   int i;
   int yMin = ( srcH > absh) ? absh : srcH;
   PStretchProc proc = ( PStretchProc) nil;
   Byte *srcLast = nil;

   if ( w == srcW) xStretch = false;
   if ( h == srcH) yStretch = false;
/* transfer case */
   if ( !xStretch && !yStretch && ( w > 0))
   {
      int y;
      int xMin = (( type & imBPP) < 8) ? 
                  ( srcLine > dstLine) ? dstLine : srcLine :
                  (((( srcW > absw) ? absw : srcW) * ( type & imBPP)) / 8);
      if ( srcW < w || srcH < absh) memset( dstData, 0, dstLine * absh);
      if ( h < 0)
      {
         dstData += dstLine * ( yMin - 1);
         dstLine =- dstLine;
      }
      for ( y = 0; y < yMin; y++, srcData += srcLine, dstData += dstLine) 
         memcpy( dstData, srcData, xMin);
      return;
   }

/* y-only stretch case */
   if ( !xStretch && yStretch && ( w > 0))
   {
      int xMin = (( type & imBPP) < 8) ? 
         ( srcLine > dstLine) ? dstLine : srcLine :
         (((( srcW > absw) ? absw : srcW) * ( type & imBPP)) / 8);
      count. l = 0;
      if ( srcW < w) memset( dstData, 0, dstLine * absh);
      if ( h < 0)
      {
         dstData += dstLine * ( absh - 1);
         dstLine =- dstLine;
      }
      if ( absh < srcH)
      {
         ystep. l = (double) absh / srcH * UINT16_PRECISION;
         memcpy( dstData, srcData, xMin);
         dstData += dstLine;
         for ( i = 0; i < srcH; i++)
         {
            if ( count. i.i > last)
            {
               memcpy( dstData, srcData, xMin);
               dstData += dstLine;
               last = count.i.i;
            }
            count. l += ystep. l;
            srcData += srcLine;
         }
      } else {
         ystep. l = (double) srcH / absh * UINT16_PRECISION;
         for ( i = 0; i < absh; i++)
         {
            if ( count.i.i > last)
            {
               srcData += srcLine;
               last = count.i.i;
            }
            count. l += ystep. l;
            memcpy( dstData, srcData, xMin);
            dstData  += dstLine;
         }
      }
      return;
   }

/* general actions for x-scaling */
   count. l = 0;
   count. l = 0;
   if ( srcW < absw || srcH < absh || ( type & imBPP) == imNibble)
      memset( dstData, 0, dstLine * absh);
   if ( absw < srcW)
      xstep. l = (double) absw / srcW * UINT16_PRECISION;
   else
      xstep. l = (double) srcW / absw * UINT16_PRECISION;
   switch( type)
   {
      case imMono:     case imBW:
         proc = ( PStretchProc)(( srcW > absw) ? bs_mono_in : bs_mono_out);  break;
      case imNibble:   case imNibble|imGrayScale:
         proc = ( PStretchProc)(( srcW > absw) ? bs_nibble_in : bs_nibble_out);     break;
      case imByte:     case im256:
         proc = ( PStretchProc)(( srcW > absw) ? bs_uint8_t_in : bs_uint8_t_out);   break;
      case imRGB:      case imRGB|imGrayScale:
         proc = ( PStretchProc)(( srcW > absw) ? bs_RGBColor_in : bs_RGBColor_out); break;
      case imShort:
         proc = ( PStretchProc)(( srcW > absw) ? bs_int16_t_in : bs_int16_t_out);   break;
      case imLong:
         proc = ( PStretchProc)(( srcW > absw) ? bs_int32_t_in : bs_int32_t_out);   break;
      case imFloat:
         proc = ( PStretchProc)(( srcW > absw) ? bs_float_in : bs_float_out);       break;
      case imDouble:
         proc = ( PStretchProc)(( srcW > absw) ? bs_double_in : bs_double_out);     break;
      case imComplex:  case imTrigComplex:
         proc = ( PStretchProc)(( srcW > absw) ? bs_Complex_in : bs_Complex_out);   break;
      case imDComplex: case imTrigDComplex:
         proc = ( PStretchProc)(( srcW > absw) ? bs_DComplex_in : bs_DComplex_out); break;
      default:
         return;
   }

/* no vertical stretch case */
   if ( !yStretch || ( srcH == -h))
   {
      if ( h < 0)
      {
         dstData += dstLine * ( yMin - 1);
         dstLine =- dstLine;
      }
      for ( i = 0; i < srcH; i++, srcData += srcLine, dstData += dstLine)
         proc( srcData, dstData, srcW, w, absw, xstep.l);
      return;
   }

/* general case */
   if ( h < 0)
   {
      dstData += dstLine * ( absh - 1);
      dstLine =- dstLine;
   }
   if ( absh < srcH)
   {
      ystep. l = (double) absh / srcH * UINT16_PRECISION;
      proc( srcData, dstData, srcW, w, absw, xstep.l);
      dstData += dstLine;
      for ( i = 0; i < srcH; i++)
      {
         if ( count. i.i > last)
         {
            proc( srcData, dstData, srcW, w, absw, xstep.l);
            dstData += dstLine;
            last = count.i.i;
         }
         count. l += ystep. l;
         srcData += srcLine;
      }
   } else {
      ystep. l = (double) srcH / absh * UINT16_PRECISION;
      for ( i = 0; i < absh; i++)
      {
         if ( count.i.i > last)
         {
            srcData += srcLine;
            last = count.i.i;
         }
         count. l += ystep. l;
         if ( srcLast == srcData) {
            memcpy( dstData, dstData - dstLine, dstLine < 0 ? -dstLine : dstLine);
         } else {
            proc( srcData, dstData, srcW, w, absw, xstep.l);
            srcLast = srcData;
         }
         dstData += dstLine;
      }
   }
}

#ifdef __cplusplus
}
#endif
