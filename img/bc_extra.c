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
 *
 */

#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extra bitstroke convertors

static void
memcpy_bitconvproc( Byte * src, Byte * dest, int count)
{
   memcpy( dest, src, count);
}


void
ibc_repad( Byte * source, Byte * dest, int srcLineSize, int dstLineSize, int srcDataSize, int dstDataSize, int srcBpp, int dstBpp, void * convProc)
{
   int sb  = srcLineSize / srcBpp;
   int db  = dstLineSize / dstBpp;
   int bsc = sb > db ? db : sb;
   int sh  = srcDataSize / srcLineSize;
   int dh  = dstDataSize / dstLineSize;
   int  h  = sh > dh ? dh : sh;
   if ( convProc == nil) {
      convProc = memcpy_bitconvproc;
      srcBpp = dstBpp = 1;
   }   

   for ( ; h > 0; h--, source += srcLineSize, dest += dstLineSize)
      (( PSimpleConvProc) convProc)( source, dest, bsc);

   sb = srcDataSize % srcLineSize / srcBpp;
   db = dstDataSize % dstLineSize / dstBpp;
   (( PSimpleConvProc) convProc)( source, dest, sb > db ? db : sb);
}

void
bc_rgb_rgbi( register Byte * source, register Byte * dest, register int count)
{
   while ( count--)
   {
      *dest++ = *source++;
      *dest++ = *source++;
      *dest++ = *source++;
      *dest++ = 0;
   }
}

void
bc_rgbi_rgb( register Byte * source, register Byte * dest, register int count)
{
   while ( count--)
   {
      *dest++ = *source++;
      *dest++ = *source++;
      *dest++ = *source++;
      source++;
   }
}


void
bc_rgb_irgb( register Byte * source, register Byte * dest, register int count)
{
   while ( count--)
   {
      *dest++ = 0;
      *dest++ = *source++;
      *dest++ = *source++;
      *dest++ = *source++;
   }
}

void
bc_irgb_rgb( register Byte * source, register Byte * dest, register int count)
{
   while ( count--)
   {
      source++;
      *dest++ = *source++;
      *dest++ = *source++;
      *dest++ = *source++;
   }
}


#ifdef __cplusplus
}
#endif

