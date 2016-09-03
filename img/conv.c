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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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

#define macro_int_int(SourceType,MidType,DestType)                             \
void rs_##SourceType##_##DestType( Handle self,                                \
	Byte * dstData, int dstType,                                              \
	double srcLo, double srcHi, double dstLo, double dstHi)                   \
{                                                                              \
	SourceType *src = (SourceType*) var->data;                                   \
	DestType *dst = (DestType*) dstData;                                        \
	int y;                                                                      \
	MidType aNumerator      = dstHi - dstLo;                                       \
	MidType bNumerator      = dstLo * srcHi - dstHi * srcLo;                       \
	MidType denominator     = srcHi - srcLo;                                       \
	int  width = var->w;                                                         \
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
		MidType v;                                                                  \
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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
	int srcLine = LINE_SIZE(width,var->type);                                   \
	int dstLine = LINE_SIZE(width,dstType);                                      \
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
		SourceType v;                                                            \
		while ( s != stop)                                                       \
		{                                                                        \
			v = a**s+++b;                                                         \
			v = (v<minimum_##DestType##Value) ? minimum_##DestType##Value :       \
				((v>maximum_##DestType##Value) ? maximum_##DestType##Value : v);  \
			*d++ = v + .5;                                                        \
		}                                                                        \
		src = (SourceType*)(((Byte*)src) + srcLine);                             \
		dst = (DestType*)(((Byte*)dst) + dstLine);                               \
	}                                                                           \
}

macro_int_int( Byte, int, Byte)
macro_int_int( Short, long, Short)
macro_int_int( Long, int64_t, Long)
macro_float_float__int_float( float, float)
macro_float_float__int_float( double, double)

macro_int_int( Short, long, Byte)
macro_int_int( Long, int64_t, Byte)
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
