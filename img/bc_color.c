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

#define EDIFF_OP_RGB \
if ( b > 255) { eb -= ( b - 255); b = 255; } else { eb = 0; } \
if ( g > 255) { eg -= ( g - 255); g = 255; } else { eg = 0; } \
if ( r > 255) { er -= ( r - 255); r = 255; } else { er = 0; }



/* Bitstroke convertors */
/* Mono */
/* 1-> 16 */
void
bc_mono_nibble( register Byte * source, register Byte * dest, register int count)
{
   register Byte tailsize = count & 7;
   dest    += (count - 1) >> 1;
   count    = count >> 3;
   source  += count;

   if ( tailsize)
   {
      register Byte tail = (*source) >> ( 8 - tailsize);
      if ( tailsize & 1)
      {
         tailsize++;
         tail <<= 1;
      }
      while( tailsize)
      {
         *dest-- = ( tail & 1) | (( tail & 2) << 3);
         tail >>= 2;
         tailsize -= 2;
      }
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *dest-- = ( c & 1) | (( c & 2) << 3);  c >>= 2;
      *dest-- = ( c & 1) | (( c & 2) << 3);  c >>= 2;
      *dest-- = ( c & 1) | (( c & 2) << 3);  c >>= 2;
      *dest-- = ( c & 1) | (( c & 2) << 3);
   }
}

/* 1-> mapped 16 */
void
bc_mono_nibble_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref)
{
   register Byte tailsize = count & 7;
   dest    += (count - 1) >> 1;
   count    = count >> 3;
   source  += count;

   if ( tailsize)
   {
      register Byte tail = (*source) >> ( 8 - tailsize);
      if ( tailsize & 1)
      {
         tailsize++;
         tail <<= 1;
      }
      while( tailsize)
      {
         *dest-- = colorref[ tail & 1] | ( colorref[( tail & 2) >> 1] << 4);
         tail >>= 2;
         tailsize -= 2;
      }
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *dest-- = colorref[ c & 1] | ( colorref[( c & 2) >> 1] << 4); c >>= 2;
      *dest-- = colorref[ c & 1] | ( colorref[( c & 2) >> 1] << 4); c >>= 2;
      *dest-- = colorref[ c & 1] | ( colorref[( c & 2) >> 1] << 4); c >>= 2;
      *dest-- = colorref[ c & 1] | ( colorref[( c & 2) >> 1] << 4);
   }
}

/*  1 -> 256 */
void
bc_mono_byte( register Byte * source, register Byte * dest, register int count)
{
   register Byte tailsize = count & 7;
   dest    += count - 1;
   count    = count >> 3;
   source  += count;
   if ( tailsize)
   {
      register Byte tail = (*source) >> ( 8 - tailsize);
      while( tailsize--)
      {
         *dest-- = tail & 1;
         tail >>= 1;
      }
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *dest-- = c & 1;      c >>= 1;
      *dest-- = c & 1;      c >>= 1;
      *dest-- = c & 1;      c >>= 1;
      *dest-- = c & 1;      c >>= 1;
      *dest-- = c & 1;      c >>= 1;
      *dest-- = c & 1;      c >>= 1;
      *dest-- = c & 1;
      *dest-- = c >> 1;
   }
}

/*  1 -> mapped 256 */
void
bc_mono_byte_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref)
{
   register Byte tailsize = count & 7;
   dest    += count - 1;
   count    = count >> 3;
   source  += count;
   if ( tailsize)
   {
      register Byte tail = (*source) >> ( 8 - tailsize);
      while( tailsize--)
      {
         *dest-- = colorref[ tail & 1];
         tail >>= 1;
      }
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *dest-- = colorref[ c & 1];      c >>= 1;
      *dest-- = colorref[ c & 1];      c >>= 1;
      *dest-- = colorref[ c & 1];      c >>= 1;
      *dest-- = colorref[ c & 1];      c >>= 1;
      *dest-- = colorref[ c & 1];      c >>= 1;
      *dest-- = colorref[ c & 1];      c >>= 1;
      *dest-- = colorref[ c & 1];
      *dest-- = colorref[ c >> 1];
   }
}


/*  1 -> gray */
void
bc_mono_graybyte( register Byte * source, register Byte * dest, register int count, register PRGBColor palette)
{
   register Byte tailsize = count & 7;
   dest    += count - 1;
   count    = count >> 3;
   source  += count;
   if ( tailsize)
   {
      register Byte tail = (*source) >> ( 8 - tailsize);
      while( tailsize--)
      {
         register RGBColor r = palette[ tail & 1];
         *dest-- = map_RGB_gray[ r.r + r.g + r.b];
         tail >>= 1;
      }
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      register RGBColor r;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c & 1]; *dest-- = map_RGB_gray[ r.r + r.g + r.b]; c >>= 1;
      r = palette[ c];     *dest-- = map_RGB_gray[ r.r + r.g + r.b];
   }
}


/*  1 -> rgb */
void
bc_mono_rgb( register Byte * source, Byte * dest, register int count, register PRGBColor palette)
{
   register Byte tailsize   = count & 7;
   register PRGBColor rdest = ( PRGBColor) dest;
   rdest   += count - 1;
   count    = count >> 3;
   source  += count;
   if ( tailsize)
   {
      register Byte tail = (*source) >> ( 8 - tailsize);
      while( tailsize--)
      {
         *rdest-- = palette[ tail & 1];
         tail >>= 1;
      }
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *rdest-- = palette[ c & 1];      c >>= 1;
      *rdest-- = palette[ c & 1];      c >>= 1;
      *rdest-- = palette[ c & 1];      c >>= 1;
      *rdest-- = palette[ c & 1];      c >>= 1;
      *rdest-- = palette[ c & 1];      c >>= 1;
      *rdest-- = palette[ c & 1];      c >>= 1;
      *rdest-- = palette[ c & 1];
      *rdest-- = palette[ c >> 1];
   }
}


/*  Nibble */
/* 16-> 1 */
void
bc_nibble_mono_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref)
{
   register int count8 = count >> 3;
   while ( count8--)
   {
      register Byte c;
      register Byte d;
      c = *source++;  d  = ( colorref[ c & 0xF] << 6) | ( colorref[ c >> 4] << 7);
      c = *source++;  d |= ( colorref[ c & 0xF] << 4) | ( colorref[ c >> 4] << 5);
      c = *source++;  d |= ( colorref[ c & 0xF] << 2) | ( colorref[ c >> 4] << 3);
      c = *source++;  *dest++ = d | ( colorref[ c & 0xF] << 1) | colorref[ c >> 4];
   }
   count &= 7;
   if ( count)
   {
      register Byte d = 0;
      register Byte s = 7;
      count = ( count >> 1) + ( count & 1);
      while ( count--)
      {
         register Byte c = *source++;
         d |= colorref[ c >> 4 ] << s--;
         d |= colorref[ c & 0xF] << s--;
      }
      *dest = d;
   }
}

/* 16-> 1, halftone */
void
bc_nibble_mono_ht( register Byte * source, register Byte * dest, register int count, register PRGBColor palette, int lineSeqNo)
{
#define n64cmp1 (( r = palette[ c >> 4], (( map_RGB_gray[r.r+r.g+r.b] >> 2) > map_halftone8x8_64[ index++]))?1:0)
#define n64cmp2 (( r = palette[ c & 15], (( map_RGB_gray[r.r+r.g+r.b] >> 2) > map_halftone8x8_64[ index++]))?1:0)
   register int count8 = count >> 3;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   while ( count8--)
   {
      register Byte  index = lineSeqNo;
      register Byte  c;
      register Byte  dst;
      register RGBColor r;
      c = *source++; dst   = n64cmp1 << 7; dst |= n64cmp2 << 6;
      c = *source++; dst  |= n64cmp1 << 5; dst |= n64cmp2 << 4;
      c = *source++; dst  |= n64cmp1 << 3; dst |= n64cmp2 << 2;
      c = *source++; dst  |= n64cmp1 << 1; *dest++ = dst | n64cmp2;
   }
   count &= 7;
   if ( count)
   {
      Byte index = lineSeqNo;
      register Byte d = 0;
      register Byte s = 7;
      count = ( count >> 1) + ( count & 1);
      while ( count--)
      {
         register Byte c = *source++;
         register RGBColor r;
         d |= n64cmp1 << s--;
         d |= n64cmp2 << s--;
      }
      *dest = d;
   }
}

/* 16-> 1, error diffusion */
void
bc_nibble_mono_ed( register Byte * source, register Byte * dest, register int count, register PRGBColor palette)
{
#define en1(xd) (( y = palette[(xd)], (sum = y.r + y.b + y.g + e) > 383)?1:0)
#define en2 e = sum - (( sum > 383) ? 765 : 0)
   
   int sum, e = 0;
   register int count8 = count >> 3;
   while ( count8--)
   {
      Byte  c;
      Byte  dst;
      RGBColor y;
      
      c = *source++; dst  = en1(c >> 4) << 7; en2; dst |= en1(c & 0xf) << 6; en2;
      c = *source++; dst |= en1(c >> 4) << 5; en2; dst |= en1(c & 0xf) << 4; en2;
      c = *source++; dst |= en1(c >> 4) << 3; en2; dst |= en1(c & 0xf) << 2; en2;
      c = *source++; dst |= en1(c >> 4) << 1; en2; *dest++ = dst | en1(c & 0xf); en2;
   }
   count &= 7;
   if ( count)
   {
      Byte d = 0, s = 7;
      count = ( count >> 1) + ( count & 1);
      while ( count--)
      {
         Byte c = *source++;
         RGBColor y;
         d |= en1( c >> 4) << s--;
         en2;
         d |= en1( c & 0xf) << s--;
         en2;
      }
      *dest = d;
   }
}   

/* map 16 */
void
bc_nibble_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref)
{
   count  =  ( count >> 1) + ( count & 1);
   source += count - 1;
   dest   += count - 1;
   while ( count--)
   {
      register Byte c = *source--;
      *dest-- = colorref[ c & 0xF] | ( colorref[ c >> 4] << 4);
   }
}

/*  16 -> 256 */
void
bc_nibble_byte( register Byte * source, register Byte * dest, register int count)
{
   register Byte tail = count & 1;
   dest   += count - 1;
   count  =  count >> 1;
   source += count;

   if ( tail) *dest-- = (*source) >> 4;
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *dest-- = c & 0xF;
      *dest-- = c >> 4;
   }
}

/*  16 -> gray */
void
bc_nibble_graybyte( register Byte * source, register Byte * dest, register int count, register PRGBColor palette)
{
   register Byte tail = count & 1;
   dest   += count - 1;
   count  =  count >> 1;
   source += count;

   if ( tail)
   {
      register RGBColor r = palette[ (*source) >> 4];
      *dest-- = map_RGB_gray[ r.r + r.g + r.b];
   }
   source--;
   while( count--)
   {
      register Byte c = *source--;
      register RGBColor r = palette[ c & 0xF];
      *dest-- = map_RGB_gray[ r.r + r.g + r.b];
      r = palette[ c >> 4];
      *dest-- = map_RGB_gray[ r.r + r.g + r.b];
   }
}


/* 16 -> mapped 256 */
void
bc_nibble_byte_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref)
{
   register Byte tail = count & 1;
   dest   += count - 1;
   count  =  count >> 1;
   source += count;

   if ( tail) *dest-- = colorref[ (*source) >> 4];
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *dest-- = colorref[ c & 0xF];
      *dest-- = colorref[ c >> 4];
   }
}


/* 16-> rgb */
void
bc_nibble_rgb( register Byte * source, Byte * dest, register int count, register PRGBColor palette)
{
   register Byte tail = count & 1;
   register PRGBColor rdest = ( PRGBColor) dest;
   rdest  += count - 1;
   count  =  count >> 1;
   source += count;

   if ( tail) *rdest-- = palette[ (*source) >> 4];
   source--;
   while( count--)
   {
      register Byte c = *source--;
      *rdest-- = palette[ c & 0xF];
      *rdest-- = palette[ c >> 4];
   }
}

/* Byte */
/* 256-> 1 */
void
bc_byte_mono_cr( register Byte * source, Byte * dest, register int count, register Byte * colorref)
{
   register int count8 = count >> 3;
   while ( count8--)
   {
      register Byte c = colorref[ *source++] << 7;
      c |= colorref[ *source++] << 6;
      c |= colorref[ *source++] << 5;
      c |= colorref[ *source++] << 4;
      c |= colorref[ *source++] << 3;
      c |= colorref[ *source++] << 2;
      c |= colorref[ *source++] << 1;
      *dest++ = c | colorref[ *source++];
   }
   count &= 7;
   if ( count)
   {
      register Byte c = 0;
      register Byte s = 7;
      while ( count--) c |= colorref[ *source++] << s--;
      *dest = c;
   }
}

/* byte-> mono, halftoned */
void
bc_byte_mono_ht( register Byte * source, register Byte * dest, register int count, PRGBColor palette, int lineSeqNo)
{
#define b64cmp  (( r = palette[ *source++], (( map_RGB_gray[r.r+r.g+r.b] >> 2) > map_halftone8x8_64[ index++]))?1:0)
   int count8 = count & 7;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   count >>= 3;
   while ( count--)
   {
      register Byte  index = lineSeqNo;
      register Byte  dst;
      register RGBColor r;
      dst  = b64cmp << 7;
      dst |= b64cmp << 6;
      dst |= b64cmp << 5;
      dst |= b64cmp << 4;
      dst |= b64cmp << 3;
      dst |= b64cmp << 2;
      dst |= b64cmp << 1;
      *dest++ = dst | b64cmp;
   }
   if ( count8)
   {
      register Byte     index = lineSeqNo;
      register Byte     dst = 0;
      register Byte     i = 7;
      register RGBColor r;
      count = count8;
      while( count--) dst |= b64cmp << i--;
      *dest = dst;
   }
}

/* byte-> mono, halftoned */
void
bc_byte_mono_ed( register Byte * source, register Byte * dest, register int count, PRGBColor palette)
{
#define egb1 (( y = palette[*source++], (sum = y.r + y.b + y.g + e) > 383)?1:0)
#define egb2 e = sum - (( sum > 383) ? 765 : 0)
   int sum, e = 0, count8 = count & 7;
   count >>= 3;
   while ( count--)
   {
      Byte  dst;
      RGBColor y;
      dst  = egb1 << 7; egb2;
      dst |= egb1 << 6; egb2;
      dst |= egb1 << 5; egb2;
      dst |= egb1 << 4; egb2;
      dst |= egb1 << 3; egb2;
      dst |= egb1 << 2; egb2;
      dst |= egb1 << 1; egb2;
      *dest++ = dst | egb1; egb2;
   }
   if ( count8)
   {
      Byte     dst = 0;
      Byte     i = 7;
      RGBColor y;
      count = count8;
      while( count--) {
         dst |= egb1 << i--;
         egb2;
      }   
      *dest = dst;
   }
   
}   

/* 256-> 16 */
void
bc_byte_nibble_cr( register Byte * source, Byte * dest, register int count, register Byte * colorref)
{
   Byte tail = count & 1;
   count = count >> 1;
   while ( count--)
   {
      register Byte c = colorref[ *source++] << 4;
      *dest++     = c | colorref[ *source++];
   }
   if ( tail) *dest = colorref[ *source] << 4;
}

/* 256-> 16 cubic halftoned */
void
bc_byte_nibble_ht( register Byte * source, Byte * dest, register int count, register PRGBColor palette, int lineSeqNo)
{
#define b8cmp (                                      \
                (((( r. b+1) >> 2) > cmp))      +  \
                (((( r. g+1) >> 2) > cmp) << 1) +  \
                (((( r. r+1) >> 2) > cmp) << 2)    \
               )
   Byte tail = count & 1;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   count = count >> 1;
   while ( count--)
   {
      register Byte index = lineSeqNo + (( count & 3) << 1);
      register Byte dst;
      register RGBColor r;
      register Byte cmp;

      r = palette[ *source++];
      cmp = map_halftone8x8_64[ index++];
      dst = b8cmp << 4;
      r = palette[ *source++];
      cmp = map_halftone8x8_64[ index];
      *dest++ = dst + b8cmp;
   }
   if ( tail)
   {
      register RGBColor r = palette[ *source];
      register Byte cmp   = map_halftone8x8_64[ lineSeqNo + 1];
      *dest = b8cmp << 4;
   }
}

/* 256-> 16 cubic, error diffusion */
void
bc_byte_nibble_ed( register Byte * source, Byte * dest, register int count, register PRGBColor palette)
{
   int er = 0, eg = 0, eb = 0;
   int r, g, b;
   
   Byte tail = count & 1;
   count = count >> 1;
   while ( count--)
   {
      Byte dst;
      RGBColor y;
      y = palette[ *source++]; r = y.r + er; g = y.g + eg; b = y.b + eb; 
      EDIFF_OP_RGB
      dst = (( r > 127) * 4 + (g > 127) * 2 + (b > 127)) << 4;
      er += r - (( r > 127) ? 255 : 0);
      eg += g - (( g > 127) ? 255 : 0);
      eb += b - (( b > 127) ? 255 : 0);
      
      y = palette[ *source++]; r = y.r + er; g = y.g + eg; b = y.b + eb; 
      EDIFF_OP_RGB
      *dest++ = dst + (( r > 127) * 4 + (g > 127) * 2 + (b > 127));
      er += r - (( r > 127) ? 255 : 0);
      eg += g - (( g > 127) ? 255 : 0);
      eb += b - (( b > 127) ? 255 : 0);
   }
   if ( tail)
   {
      RGBColor y = palette[ *source++]; r = y.r + er; g = y.g + eg; b = y.b + eb; 
      EDIFF_OP_RGB
      *dest = (( r > 127) * 4 + (g > 127) * 2 + (b > 127))<<4;
   }
}

/* map 256 */
void
bc_byte_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref)
{
   dest   += count - 1;
   source += count - 1;
   while ( count--) *dest-- = colorref[ *source--];
}

/* 256-> gray */
void
bc_byte_graybyte( register Byte * source, register Byte * dest, register int count, register PRGBColor palette)
{
   while ( count--)
   {
      register RGBColor r = palette[ *source++];
      *dest++ = map_RGB_gray[ r .r + r. g + r. b];
   }
}

/* 256-> rgb */
void
bc_byte_rgb( register Byte * source, Byte * dest, register int count, register PRGBColor palette)
{
   register PRGBColor rdest = ( PRGBColor) dest;
   rdest  += count - 1;
   source += count - 1;
   while ( count--) *rdest-- = palette[ *source--];
}

/* Gray Byte */
/* gray-> mono, halftoned */
void
bc_graybyte_mono_ht( register Byte * source, register Byte * dest, register int count, int lineSeqNo)
{
#define gb64cmp  (((*source+++1) >> 2) > map_halftone8x8_64[ index++])
   int count8 = count & 7;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   count >>= 3;
   while ( count--)
   {
      register Byte  index = lineSeqNo;
      register Byte  dst;
      dst  = gb64cmp << 7;
      dst |= gb64cmp << 6;
      dst |= gb64cmp << 5;
      dst |= gb64cmp << 4;
      dst |= gb64cmp << 3;
      dst |= gb64cmp << 2;
      dst |= gb64cmp << 1;
      *dest++ = dst | gb64cmp;
   }
   if ( count8)
   {
      register Byte  index = lineSeqNo;
      register Byte  dst = 0;
      register Byte  i = 7;
      count = count8;
      while( count--) dst |= gb64cmp << i--;
      *dest = dst;
   }
}

/* gray-> mono, error diffusion */
void
bc_graybyte_mono_ed( register Byte * source, register Byte * dest, register int count)
{
#define edgb1 (((sum = (*source++) + e)) > 127)
#define edgb2 e = sum - (( sum > 127) ? 255 : 0)
   int sum, e = 0;
   int count8 = count & 7;
   count >>= 3;
   while ( count--)
   {
      Byte  dst;
      dst  = edgb1 << 7; edgb2;
      dst |= edgb1 << 6; edgb2;
      dst |= edgb1 << 5; edgb2;
      dst |= edgb1 << 4; edgb2;
      dst |= edgb1 << 3; edgb2;
      dst |= edgb1 << 2; edgb2;
      dst |= edgb1 << 1; edgb2;
      *dest++ = dst | edgb1; edgb2;
   }
   if ( count8)
   {
      register Byte  dst = 0;
      register Byte  i = 7;
      count = count8;
      while( count--) {
         dst |= edgb1 << i--; 
         edgb2;
      }   
      *dest = dst;
   }
   
}   

/* gray -> 16 gray */
void
bc_graybyte_nibble_ht( register Byte * source, Byte * dest, register int count, int lineSeqNo)
{
#define gb16cmp ( div17[c] + (( mod17mul3[c]) > cmp))
   Byte tail = count & 1;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   count = count >> 1;
   while ( count--)
   {
      register short c;
      register Byte index = lineSeqNo + (( count & 3) << 1);
      register Byte dst;
      register Byte cmp;
      c = *source++;
      cmp = map_halftone8x8_51[ index++];
      dst = gb16cmp << 4;
      c = *source++;
      cmp = map_halftone8x8_51[ index];
      *dest++ = dst + gb16cmp;
   }
   if ( tail)
   {
      register short c = *source;
      register Byte cmp = map_halftone8x8_51[ lineSeqNo + 1];
      *dest = gb16cmp << 4;
   }
}

/* gray -> 16 gray, error diffusion */
void
bc_graybyte_nibble_ed( register Byte * source, Byte * dest, register int count)
{
   int e = 0;
   Byte tail = count & 1;
   count = count >> 1;
   while ( count--)
   {
      Byte dst;
      int s = ( *source++) + e;
      if ( s > 255) { e -= ( s - 255); s = 255; } else { e = 0; }
      dst = div17[s] << 4;
      e += s % 17;
      s = ( *source++) + e;
      if ( s > 255) { e -= ( s - 255); s = 255; } else { e = 0; }
      *dest++ = dst + div17[s];
      e += s % 17;
   }   
   if ( tail) {
      int s = ( *source++) + e;
      if ( s > 255) { e -= ( s - 255); s = 255; } else { e = 0; }
      *dest++ = div17[s] << 4;
   }   
}   

/* gray-> rgb */
void
bc_graybyte_rgb( register Byte * source, Byte * dest, register int count)
{
   register PRGBColor rdest = ( PRGBColor) dest;
   rdest  += count - 1;
   source += count - 1;
   while ( count--)
   {
      register Byte  c = *source--;
      register RGBColor r;
      r. r = c;
      r. b = c;
      r. g = c;
      *rdest-- = r;
   }
}

/* RGB */

/* rgb -> gray */
void
bc_rgb_graybyte( Byte * source, register Byte * dest, register int count)
{
   register PRGBColor rsource = ( PRGBColor) source;
   while ( count--)
   {
      register RGBColor r = *rsource++;
      *dest++ = map_RGB_gray[ r .r + r. g + r. b];
   }
}

/* rgb-> mono, halftoned */
void
bc_rgb_mono_ht( register Byte * source, register Byte * dest, register int count, int lineSeqNo)
{
#define tc64cmp  (( source+=3, ( map_RGB_gray[ source[-1] + source[-2] + source[-3]] >> 2) > map_halftone8x8_64[ index++])?1:0)
   int count8 = count & 7;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   count >>= 3;
   while ( count--)
   {
      register Byte  index = lineSeqNo;
      register Byte  dst;
      dst  = tc64cmp << 7;
      dst |= tc64cmp << 6;
      dst |= tc64cmp << 5;
      dst |= tc64cmp << 4;
      dst |= tc64cmp << 3;
      dst |= tc64cmp << 2;
      dst |= tc64cmp << 1;
      *dest++  = dst | tc64cmp;
   }
   if ( count8)
   {
      register Byte  index = lineSeqNo;
      register Byte  dst = 0;
      register Byte  i = 7;
      count = count8;
      while( count--) dst |=  tc64cmp << i--;
      *dest = dst;
   }
}

/* rgb-> mono, error diffusion */
void
bc_rgb_mono_ed( Byte * source, register Byte * dest, register int count)
{
#define ed1r ((sum = ((*source++) + (*source++) + (*source++) + e)) > 383)
#define ed2r e = sum - (( sum > 383) ? 765 : 0)
   int sum, e = 0;
   int count8 = count & 7;

   count >>= 3;
   while ( count--) {
      Byte dst;
      dst =  ed1r << 7; ed2r;
      dst |= ed1r << 6; ed2r;
      dst |= ed1r << 5; ed2r;
      dst |= ed1r << 4; ed2r;
      dst |= ed1r << 3; ed2r;
      dst |= ed1r << 2; ed2r;
      dst |= ed1r << 1; ed2r;
      *dest++ = dst | ed1r; ed2r;
   }   
   if ( count8) {
      Byte  dst = 0;
      Byte  i = 7;
      count = count8;
      while( count--) {
         dst |= ed1r << i--;
         ed2r;
      }   
      *dest = dst;
   }   
}   

/* rgb -> nibble, no halftoning */
Byte
rgb_color_to_16( register Byte b, register Byte g, register Byte r)
{
   /* 1 == 255 */
   /* 2/3 == 170 */
   /* 1/2 == 128 */
   /* 1/3 == 85 */
   /* 0 == 0 */
   int rg, dist = 384;
   Byte code = 0;
   Byte mask = 8;

   rg = r+g;
   if ( rg-b > 128 ) code |= 1;
   if ((int)r - (int)g + (int)b > 128 ) code |= 2;
   if ((int)g + (int)b - (int)r > 128 ) code |= 4;
   if ( code == 0)
   {
      dist = 128;
      mask = 7;
   }
   else if ( code == 7)
   {
      code = 8;
      dist = 640;
      mask = 7;
   }
   if ( rg+b > dist) code |= mask;
   return code;
}

void
bc_rgb_nibble( register Byte *source, Byte *dest, int count)
{
   Byte tail = count & 1;
   register Byte *stop = source + (count >> 1)*6;
   while ( source != stop)
   {
      *dest++ = (rgb_color_to_16(source[0],source[1],source[2]) << 4) |
                 rgb_color_to_16(source[3],source[4],source[5]);
      source += 6;
   }
   if ( tail)
      *dest = rgb_color_to_16(source[0],source[1],source[2]) << 4;
}

/* rgb-> 8 halftoned */
void
bc_rgb_nibble_ht( register Byte * source, Byte * dest, register int count, int lineSeqNo)
{
#define tc8cmp  ( source+=3,                          \
                 (((( source[-3]+1) >>2) > cmp))      +  \
                 (((( source[-2]+1) >>2) > cmp) << 1) +  \
                 (((( source[-1]+1) >>2) > cmp) << 2)    \
                )
   Byte tail = count & 1;
   lineSeqNo = ( lineSeqNo & 7) << 3;
   count = count >> 1;
   while ( count--)
   {
      register Byte index = lineSeqNo + (( count & 3) << 1);
      register Byte dst;
      register Byte cmp;
      cmp = map_halftone8x8_64[ index++];
      dst = tc8cmp << 4;
      cmp = map_halftone8x8_64[ index];
      *dest++ = dst + tc8cmp;
   }
   if ( tail)
   {
      register Byte cmp  = map_halftone8x8_64[ lineSeqNo + 1];
      *dest = tc8cmp << 4;
   }
}

/* rgb-> 8 cubic, error diffusion */
void
bc_rgb_nibble_ed( Byte * source, register Byte * dest, register int count)
{
   int er = 0, eg = 0, eb = 0;
   Byte tail = count & 1;
   count = count >> 1;
   while ( count--) {
      int b = (*source++) + eb;
      int g = (*source++) + eg;
      int r = (*source++) + er;
      Byte dst;
      EDIFF_OP_RGB
      dst = (( r > 127) * 4 + (g > 127) * 2 + (b > 127)) << 4;
      er += r - (( r > 127) ? 255 : 0);
      eg += g - (( g > 127) ? 255 : 0);
      eb += b - (( b > 127) ? 255 : 0);
      b = (*source++) + eb;
      g = (*source++) + eg;
      r = (*source++) + er;
      EDIFF_OP_RGB
      *dest++ = dst + (( r > 127) * 4 + (g > 127) * 2 + (b > 127));
      er += r - (( r > 127) ? 255 : 0);
      eg += g - (( g > 127) ? 255 : 0);
      eb += b - (( b > 127) ? 255 : 0);
   }   
   if ( tail) {
      int b = (*source++) + eb;
      int g = (*source++) + eg;
      int r = (*source++) + er;
      EDIFF_OP_RGB
      *dest++ = (( r > 127) * 4 + (g > 127) * 2 + (b > 127)) << 4;
   }   
}   

/* rgb-> 256 cubic */
void
bc_rgb_byte( Byte * source, register Byte * dest, register int count)
{
   while ( count--)
   {
      register Byte dst = ( div51 [ *source++]);
      dst += ( div51[ *source++]) * 6;
      *dest++ = dst + div51[ *source++] * 36;
   }
}


/* rgb-> 256 cubic, halftoned */
void
bc_rgb_byte_ht( Byte * source, register Byte * dest, register int count, int lineSeqNo)
{
   lineSeqNo = ( lineSeqNo & 7) << 3;
   while ( count--)
   {
      register Byte cmp = map_halftone8x8_51[( count & 7) + lineSeqNo];
      register Byte src;
      register Byte dst;
      src = *source++;
      dst =  ( div51[ src] + ( mod51[ src] > cmp));
      src = *source++;
      dst += ( div51[ src] + ( mod51[ src] > cmp)) * 6;
      src = *source++;
      dst += ( div51[ src] + ( mod51[ src] > cmp)) * 36;
      *dest++ = dst;
   }
}

/* rgb-> 256 cubic, error diffusion */
void
bc_rgb_byte_ed( Byte * source, register Byte * dest, register int count)
{
   int er = 0, eg = 0, eb = 0;
   while ( count--) {
      int b = (*source++) + eb;
      int g = (*source++) + eg;
      int r = (*source++) + er;
      EDIFF_OP_RGB
      (*dest++) = div51[r] * 36 + div51[g] * 6 + div51[b];
      er += mod51[ r];
      eg += mod51[ g];
      eb += mod51[ b];
   }   
}   
   
/* bitstroke copiers */
void
bc_nibble_copy( Byte * source, Byte * dest, unsigned int from, unsigned int width)
{
   if ( from & 1) {
      register Byte a;
      register int byteLim = (( width - 1) >> 1) + (( width - 1) & 1);
      source += from >> 1;
      a = *source++;
      while ( byteLim--) {
         register Byte b = *source++;
         *dest++ = ( a << 4) | ( b >> 4);
         a = b;
      }
      if ( width & 1) *dest++ = a << 4;
   } else
      memcpy( dest, source + ( from >> 1), ( width >> 1) + ( width & 1));
}

void
bc_mono_copy( Byte * source, Byte * dest, unsigned int from, unsigned int width)
{
   if (( from & 7) != 0) {
      register Byte a;
      short    lShift = from & 7;
      short    rShift = 8 - lShift;
      register int toLim  = ( width >> 3) + ((( width & 7) > 0) ? 1 : 0);
      Byte * froLim = source + (( from + width) >> 3) + (((( from + width) & 7) > 0) ? 1 : 0);
      source += from >> 3;
      a = *source++;
      while( toLim--) {
         register Byte b = ( source == froLim) ? 0 : *source++;
         *dest++ = ( a << lShift) | ( b >> rShift);
         a = b;
      }
   } else
      memcpy( dest, source + ( from >> 3), ( width >> 3) + (( width & 7) > 0 ? 1 : 0));
}

#ifdef __cplusplus
}
#endif
