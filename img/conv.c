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
/* Created by Anton Berezin <tobez@plab.ku.dk> */
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif


#define var (( PImage) self)

#define minimum_ByteValue 0
#define maximum_ByteValue 255
#define minimum_ShortValue INT16_MIN
#define maximum_ShortValue INT16_MAX
#define minimum_LongValue INT32_MIN
#define maximum_LongValue INT32_MAX

#define macro_asis(SourceType,DestType)                                        \
void ic_##SourceType##_##DestType( Handle self,                           \
      Byte *dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)   \
{                                                                              \
   SourceType *src = (SourceType*) var->data;                                   \
   DestType *dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType *s = src;                                                     \
      DestType *d = dst;                                                       \
      SourceType *stop = s + width;                                            \
      while ( s != stop) *d++ = (DestType)*s++;                                \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
   memcpy( dstPal, map_RGB_gray, 256 * sizeof( RGBColor));                     \
}

#define macro_asis_toint(SourceType,DestType)                                        \
void ic_##SourceType##_##DestType( Handle self,                           \
      Byte *dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)   \
{                                                                              \
   SourceType *src = (SourceType*) var->data;                                   \
   DestType *dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType *s = src;                                                     \
      DestType *d = dst;                                                       \
      SourceType *stop = s + width;                                            \
      while ( s != stop) *d++ = (DestType)(*s++ + 0.5);                        \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
   memcpy( dstPal, map_RGB_gray, 256 * sizeof( RGBColor));                     \
}

#define macro_asis_complex(SourceType,DestType)                                        \
void ic_##SourceType##_##DestType##_complex( Handle self,                           \
      Byte *dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)  \
{                                                                              \
   SourceType *src = (SourceType*) var->data;                                   \
   DestType *dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType *s = src;                                                     \
      DestType *d = dst;                                                       \
      SourceType *stop = s + width;                                            \
      while ( s != stop) { *d++ = (DestType)*s++; *d++ = 0; }                  \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
   memcpy( dstPal, map_RGB_gray, 256 * sizeof( RGBColor));                     \
}

#define macro_asis_revcomplex(SourceType,DestType)                       \
void ic_##SourceType##_complex_##DestType( Handle self,                           \
      Byte *dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)         \
{                                                                              \
   SourceType *src = (SourceType*) var->data;                                   \
   DestType *dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType *s = src;                                                     \
      DestType *d = dst;                                                       \
      SourceType *stop = s + width*2;                                          \
      while ( s != stop) { *d++ = (DestType)*s++; s++; }                        \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
   memcpy( dstPal, map_RGB_gray, 256 * sizeof( RGBColor));                     \
}

#define macro_asis_revcomplex_toint(SourceType,DestType)                       \
void ic_##SourceType##_complex_##DestType( Handle self,                           \
      Byte *dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)         \
{                                                                              \
   SourceType *src = (SourceType*) var->data;                                   \
   DestType *dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType *s = src;                                                     \
      DestType *d = dst;                                                       \
      SourceType *stop = s + width*2;                                          \
      while ( s != stop) { *d++ = (DestType)(*s++ + 0.5); s++; }                \
      src = (SourceType*)(((Byte*)src) + srcLine);                            \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
   memcpy( dstPal, map_RGB_gray, 256 * sizeof( RGBColor));                     \
}

#define macro_int_int(SourceType,DestType)                                     \
void rs_##SourceType##_##DestType( Handle self,                                \
     Byte * dstData, int dstType,                                              \
     double srcLo, double srcHi, double dstLo, double dstHi)                   \
{                                                                              \
   SourceType *src = (SourceType*) var->data;                                   \
   DestType *dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   long aNumerator      = dstHi - dstLo;                                       \
   long bNumerator      = dstLo * srcHi - dstHi * srcLo;                       \
   long denominator     = srcHi - srcLo;                                       \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   if ( denominator == 0 || dstHi == dstLo)                                    \
   {                                                                           \
      DestType v = (dstLo<minimum_##DestType##Value) ? minimum_##DestType##Value : \
          ((dstLo>maximum_##DestType##Value) ? maximum_##DestType##Value : dstLo); \
      for ( y = 0; y < var->h; y++)                                             \
      {                                                                        \
         DestType *d = dst;                                                    \
         DestType *stop = d + width;                                           \
         while ( d != stop) *d++=v;                                            \
         dst = (DestType*)(((Byte*)dst) + dstLine);                            \
      }                                                                        \
      return;                                                                  \
   }                                                                           \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType *s = src;                                                     \
      DestType *d = dst;                                                       \
      SourceType *stop = s + width;                                            \
      long v;                                                                  \
      while ( s != stop)                                                       \
      {                                                                        \
         v = (aNumerator**s+++bNumerator)/denominator;                         \
         v = (v<minimum_##DestType##Value) ? minimum_##DestType##Value :       \
             ((v>maximum_##DestType##Value) ? maximum_##DestType##Value : v);  \
         *d++ = v;                                                             \
      }                                                                        \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
}

#define macro_float_float__int_float(SourceType,DestType)                      \
void rs_##SourceType##_##DestType( Handle self,                                \
     Byte * dstData,  int dstType,                                             \
     double srcLo, double srcHi, double dstLo, double dstHi)                   \
{                                                                              \
   SourceType* src = (SourceType*) var->data;                                   \
   DestType* dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   double a, b;                                                                \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   if ( srcHi == srcLo || dstHi == dstLo)                                      \
   {                                                                           \
      for ( y = 0; y < var->h; y++)                                             \
      {                                                                        \
         DestType *d = dst;                                                    \
         DestType *stop = d + width;                                           \
         while ( d != stop) *d++=dstLo;                                        \
         dst = (DestType*)(((Byte*)dst) + dstLine);                            \
      }                                                                        \
      return;                                                                  \
   }                                                                           \
   a = ((double)dstHi - (double)dstLo) / ((double)srcHi - (double)srcLo);      \
   b = ((double)dstLo*(double)srcHi -                                          \
      (double)dstHi*(double)srcLo)/((double)srcHi-(double)srcLo);              \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType* s = src;                                                     \
      DestType* d = dst;                                                       \
      SourceType* stop = s + width;                                            \
      while ( s != stop) *d++=a**s+++b;                                        \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
}

#define macro_float_int(SourceType,DestType)                                   \
void rs_##SourceType##_##DestType( Handle self,                                \
     Byte * dstData, int dstType,                                              \
     double srcLo, double srcHi, double dstLo, double dstHi)                   \
{                                                                              \
   SourceType* src = (SourceType*) var->data;                                   \
   DestType* dst = (DestType*) dstData;                                        \
   int y;                                                                      \
   double a, b;                                                                \
   int  width = var->w;                                                         \
   int srcLine = (( width * ( var->type & imBPP) + 31) / 32) * 4;               \
   int dstLine = (( width * ( dstType & imBPP) + 31) / 32) * 4;                \
   if ( srcHi == srcLo || dstHi == dstLo)                                      \
   {                                                                           \
      DestType v = (dstLo<minimum_##DestType##Value) ? minimum_##DestType##Value : \
          ((dstLo>maximum_##DestType##Value) ? maximum_##DestType##Value : dstLo) \
          + 0.5;                                                                \
      for ( y = 0; y < var->h; y++)                                             \
      {                                                                        \
         DestType *d = dst;                                                    \
         DestType *stop = d + width;                                           \
         while ( d != stop) *d++=v;                                            \
         dst = (DestType*)(((Byte*)dst) + dstLine);                            \
      }                                                                        \
      return;                                                                  \
   }                                                                           \
   a = ((double)dstHi - (double)dstLo) / ((double)srcHi - (double)srcLo);      \
   b = ((double)dstLo*(double)srcHi -                                          \
      (double)dstHi*(double)srcLo)/((double)srcHi-(double)srcLo);              \
   for ( y = 0; y < var->h; y++)                                                \
   {                                                                           \
      SourceType* s = src;                                                     \
      DestType* d = dst;                                                       \
      SourceType* stop = s + width;                                            \
      long v;                                                                  \
      while ( s != stop)                                                       \
      {                                                                        \
         v = a**s+++b;                                                         \
         v = (v<minimum_##DestType##Value) ? minimum_##DestType##Value :       \
             ((v>maximum_##DestType##Value) ? maximum_##DestType##Value : v);  \
         *d++ = v;                                                             \
      }                                                                        \
      src = (SourceType*)(((Byte*)src) + srcLine);                             \
      dst = (DestType*)(((Byte*)dst) + dstLine);                               \
   }                                                                           \
}

macro_int_int( Byte, Byte)
macro_int_int( Short, Short)
macro_int_int( Long, Long)
macro_float_float__int_float( float, float)
macro_float_float__int_float( double, double)

macro_int_int( Short, Byte)
macro_int_int( Long, Byte)
macro_float_int(float, Byte)
macro_float_int(double, Byte)

macro_asis(Byte,Short)
macro_asis(Byte,Long)
macro_asis(Byte,float)
macro_asis(Byte,double)
macro_asis(Short,Byte)
macro_asis(Short,Long)
macro_asis(Short,float)
macro_asis(Short,double)
macro_asis(Long,Byte)
macro_asis(Long,Short)
macro_asis(Long,float)
macro_asis(Long,double)
macro_asis_toint(float,Byte)
macro_asis_toint(float,Short)
macro_asis_toint(float,Long)
macro_asis(float,double)
macro_asis_toint(double,Byte)
macro_asis_toint(double,Short)
macro_asis_toint(double,Long)
macro_asis(double,float)

macro_asis_complex(Byte,float)
macro_asis_complex(Byte,double)
macro_asis_complex(Short,float)
macro_asis_complex(Short,double)   
macro_asis_complex(Long,float)   
macro_asis_complex(Long,double)   
macro_asis_complex(float,float)   
macro_asis_complex(float,double)   
macro_asis_complex(double,float)   
macro_asis_complex(double,double)   

macro_asis_revcomplex(double,double)
macro_asis_revcomplex(double,float)
macro_asis_revcomplex_toint(double,Long)
macro_asis_revcomplex_toint(double,Short)
macro_asis_revcomplex_toint(double,Byte)
macro_asis_revcomplex(float,double)
macro_asis_revcomplex(float,float)
macro_asis_revcomplex_toint(float,Long)
macro_asis_revcomplex_toint(float,Short)
macro_asis_revcomplex_toint(float,Byte)
   
#ifdef __cplusplus
}
#endif
