#include <img_conv.h>
#define var (( PImage) self)

#define minimum_ByteValue 0
#define maximum_ByteValue UCHAR_MAX
#define minimum_shortValue SHRT_MIN
#define maximum_shortValue SHRT_MAX
#define minimum_longValue LONG_MIN
#define maximum_longValue LONG_MAX

#define macro_asis(SourceType,DestType)                                        \
void ic_##SourceType##_##DestType( Handle self,                           \
      Byte *dstData, PRGBColor dstPal, int dstType)                            \
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

// macro_int_int( Byte, short)
// macro_int_int( Byte, long)
// macro_int_int( short, long)
// macro_int_int( short, Byte)
// macro_int_int( long, short)
// macro_int_int( long, Byte)
// macro_float_float__int_float( float, double)
// macro_float_float__int_float( double, float)
// macro_float_float__int_float( Byte, float)
// macro_float_float__int_float( Byte, double)
// macro_float_float__int_float( short, float)
// macro_float_float__int_float( short, double)
// macro_float_float__int_float( long, float)
// macro_float_float__int_float( long, double)
// macro_float_int(float, Byte)
// macro_float_int(float, short)
// macro_float_int(float, long)
// macro_float_int(double, Byte)
// macro_float_int(double, short)
// macro_float_int(double, long)

macro_int_int( Byte, Byte)
macro_int_int( short, short)
macro_int_int( long, long)
macro_float_float__int_float( float, float)
macro_float_float__int_float( double, double)

macro_int_int( short, Byte)
macro_int_int( long, Byte)
macro_float_int(float, Byte)
macro_float_int(double, Byte)

macro_asis(Byte,short)
macro_asis(Byte,long)
macro_asis(Byte,float)
macro_asis(Byte,double)
macro_asis(short,Byte)
macro_asis(short,long)
macro_asis(short,float)
macro_asis(short,double)
macro_asis(long,Byte)
macro_asis(long,short)
macro_asis(long,float)
macro_asis(long,double)
macro_asis(float,Byte)
macro_asis(float,short)
macro_asis(float,long)
macro_asis(float,double)
macro_asis(double,Byte)
macro_asis(double,short)
macro_asis(double,long)
macro_asis(double,float)
