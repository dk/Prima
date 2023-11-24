#ifndef _IMG_IMG_CONV_H_
#define _IMG_IMG_CONV_H_
#include "guts.h"
#include "Image.h"
#include <sys/types.h>
#include <limits.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
#include <fcntl.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef void SimpleConvProc( Byte * srcData, Byte * dstData, int count);
typedef SimpleConvProc *PSimpleConvProc;

typedef struct {
	ColorPixel color;
	ColorPixel backColor;
	int rop;
	Bool transparent;
	FillPattern pattern;
	Point patternOffset;
	Handle tile;
	unsigned char * linePattern;
	PRegionRec region;
} ImgPaintContext, *PImgPaintContext;

typedef void BitBltProc( Byte * src, Byte * dst, int count);
typedef BitBltProc *PBitBltProc;

#define dBLEND_FUNC(name) void name( \
	const Byte * src, const Byte src_inc, \
	const Byte * src_a, const Byte src_a_inc,\
	Byte * dst, \
	const Byte * dst_a, const Byte dst_a_inc,\
	int bytes)

typedef dBLEND_FUNC(BlendFunc);

/* initializer routine */
extern void init_image_support(void);

/* image basic routines */
#define LINE_SIZE(width,type) (((( width ) * (( type ) & imBPP) + 31) / 32) * 4)
#define EFFECTIVE_LINE_SIZE(width,type) (\
	 ((type&imBPP) == 1) ? (((width) + 7) / 8)  : \
	(((type&imBPP) == 4) ? (((width) + 1) / 2) : \
	((width)*(type&imBPP)/8)) \
)

extern Bool ic_stretch( int type, Byte * srcData, int srcW, int srcH, Byte * dstData, int w, int h, int scaling, char * error);
extern int  ic_stretch_suggest_type( int type, int scaling );
extern void ic_type_convert( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * palSize, Bool palSize_only);
extern Bool itype_supported( int type);
extern Bool itype_importable( int type, int *newtype, void **from_proc, void **to_proc);
extern Bool iconvtype_supported( int conv);
extern Byte rop_1bit_transform(Byte fore, Byte back, Byte rop);

/* palette routines */
extern void cm_init_colormap( void);
extern void cm_reverse_palette( PRGBColor source, PRGBColor dest, int colors);
extern void cm_squeeze_palette( PRGBColor source, int srcColors, PRGBColor dest, int destColors);
extern Byte cm_nearest_color( RGBColor color, int palSize, PRGBColor palette);
extern void cm_fill_colorref( PRGBColor fromPalette, int fromColorCount, PRGBColor toPalette, int toColorCount, Byte * colorref);
extern void cm_colorref_4to8( Byte * src16, Byte * dst256 );
extern U16* cm_study_palette( RGBColor * palette, int pal_size);
extern Bool cm_optimized_palette( Byte * data, int lineSize, int width, int height, RGBColor * palette, int * max_pal_size);
extern void cm_reduce_palette4( Byte * srcData, int srcLine, int width, int height, RGBColor * srcPalette, int srcPalSize, RGBColor * dstPalette, int * dstPalSize);
extern void cm_reduce_palette8( Byte * srcData, int srcLine, int width, int height, RGBColor * srcPalette, int srcPalSize, RGBColor * dstPalette, int * dstPalSize);

/* bitstroke conversion routines */
extern void bc_mono_nibble( register Byte * source, register Byte * dest, register int count);
extern void bc_mono_nibble_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref);
extern void bc_mono_byte( register Byte * source, register Byte * dest, register int count);
extern void bc_mono_byte_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref);
extern void bc_mono_graybyte( register Byte * source, register Byte * dest, register int count, register PRGBColor palette);
extern void bc_mono_rgb( register Byte * source, Byte * dest, register int count, register PRGBColor palette);
extern void bc_nibble_mono_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref);
extern void bc_nibble_mono_ht( register Byte * source, register Byte * dest, register int count, register PRGBColor palette, int lineSeqNo);
extern void bc_nibble_mono_ed( Byte * source, Byte * dest, int count, PRGBColor palette, int * err_buf);
extern void bc_nibble_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref);
extern void bc_nibble_nibble_ht( register Byte * source, register Byte * dest, register int count, register PRGBColor palette, int lineSeqNo);
extern void bc_nibble_nibble_ed( Byte * source, Byte * dest, int count, PRGBColor palette, int * err_buf);
extern void bc_nibble_byte( register Byte * source, register Byte * dest, register int count);
extern void bc_nibble_graybyte( register Byte * source, register Byte * dest, register int count, register PRGBColor palette);
extern void bc_nibble_byte_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref);
extern void bc_nibble_rgb( register Byte * source, Byte * dest, register int count, register PRGBColor palette);
extern void bc_byte_mono_cr( register Byte * source, Byte * dest, register int count, register Byte * colorref);
extern void bc_byte_mono_ht( register Byte * source, register Byte * dest, register int count, PRGBColor palette, int lineSeqNo);
extern void bc_byte_mono_ed( Byte * source, Byte * dest, int count, PRGBColor palette, int * err_buf);
extern void bc_byte_nibble_cr( register Byte * source, Byte * dest, register int count, register Byte * colorref);
extern void bc_byte_nibble_ht( register Byte * source, Byte * dest, register int count, register PRGBColor palette, int lineSeqNo);
extern void bc_byte_nibble_ed( Byte * source, Byte * dest, int count, PRGBColor palette, int * err_buf);
extern void bc_byte_byte_ht( register Byte * source, Byte * dest, register int count, register PRGBColor palette, int lineSeqNo);
extern void bc_byte_byte_ed( Byte * source, Byte * dest, int count, PRGBColor palette, int * err_buf);
extern void bc_byte_cr( register Byte * source, register Byte * dest, register int count, register Byte * colorref);
extern void bc_byte_op( Byte * source, Byte * dest, int count, U16 * tree, PRGBColor src_palette, PRGBColor dst_palette, int * err_buf);
extern void bc_byte_nop( Byte * source, Byte * dest, int count, U16 * tree, PRGBColor src_palette, PRGBColor dst_palette);
extern void bc_byte_graybyte( register Byte * source, register Byte * dest, register int count, register PRGBColor palette);
extern void bc_byte_rgb( register Byte * source, Byte * dest, register int count, register PRGBColor palette);
extern void bc_graybyte_mono( register Byte * source, register Byte * dest, register int count);
extern void bc_graybyte_mono_ht( register Byte * source, register Byte * dest, register int count, int lineSeqNo);
extern void bc_graybyte_nibble( register Byte * source, Byte * dest, register int count);
extern void bc_graybyte_nibble_ht( register Byte * source, Byte * dest, register int count, int lineSeqNo);
extern void bc_graybyte_nibble_ed( Byte * source, Byte * dest, int count, int * err_buf);
extern void bc_graybyte_rgb( register Byte * source, Byte * dest, register int count);
extern void bc_rgb_mono_ht( register Byte * source, register Byte * dest, register int count, int lineSeqNo);
extern void bc_rgb_mono_ed( Byte * source, Byte * dest, int count, int * err_buf);
extern Byte rgb_color_to_16( register Byte b, register Byte g, register Byte r);
extern void bc_rgb_nibble( register Byte *source, Byte *dest, int count);
extern void bc_rgb_nibble_ht( register Byte * source, Byte * dest, register int count, int lineSeqNo);
extern void bc_rgb_nibble_ed( Byte * source, Byte * dest, int count, int * err_buf);
extern void bc_rgb_byte( Byte * source, register Byte * dest, register int count);
extern void bc_rgb_byte_ht( Byte * source, register Byte * dest, register int count, int lineSeqNo);
extern void bc_rgb_byte_ed( Byte * source, Byte * dest, int count, int * err_buf);
extern void bc_rgb_byte_op( RGBColor * src, Byte * dest, int count, U16 * tree, RGBColor * palette, int * err_buf);
extern void bc_rgb_byte_nop( RGBColor * src, Byte * dest, int count, U16 * tree, RGBColor * palette);
extern void bc_rgb_graybyte( Byte * source, register Byte * dest, register int count);
extern void bc_mono_Short( Byte * source, Byte * dest, register unsigned int count, Short fore, Short back);
extern void bc_mono_Long( Byte * source, Byte * dest, register unsigned int count, Long fore, Long back);
extern void bc_mono_float( Byte * source, Byte * dest, register unsigned int count, float fore, float back);
extern void bc_mono_double( Byte * source, Byte * dest, register unsigned int count, double fore, double back);

/* bitstroke stretching types */

typedef void StretchProc( void * srcData, void * dstData, int w, int x, int absx, long step);
typedef StretchProc *PStretchProc;

#if !defined(sgi) || defined(__GNUC__)
#pragma pack(1)
#endif
typedef union _Fixed {
	int32_t l;
#if (BYTEORDER==0x4321) || (BYTEORDER==0x87654321)
	struct {
		int16_t  i;
		uint16_t f;
	} i;
#else
	struct {
		uint16_t f;
		int16_t  i;
	} i;
#endif
} Fixed;
#if !defined(sgi) || defined(__GNUC__)
#pragma pack()
#endif

#define UINT16_PRECISION (1L<<(8*sizeof(uint16_t)))

/* bitstroke stretching routines */
extern void bs_mono_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step);
extern void bs_nibble_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step);
extern void bs_uint8_t_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step);
extern void bs_int16_t_in( int16_t * srcData, int16_t * dstData, int w, int x, int absx, long step);
extern void bs_RGBColor_in( RGBColor * srcData, RGBColor * dstData, int w, int x, int absx, long step);
extern void bs_int32_t_in( int32_t * srcData, int32_t * dstData, int w, int x, int absx, long step);
extern void bs_float_in( float * srcData, float * dstData, int w, int x, int absx, long step);
extern void bs_double_in( double * srcData, double * dstData, int w, int x, int absx, long step);
extern void bs_Complex_in( Complex * srcData, Complex * dstData, int w, int x, int absx, long step);
extern void bs_DComplex_in( DComplex * srcData, DComplex * dstData, int w, int x, int absx, long step);
extern void bs_mono_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step);
extern void bs_nibble_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step);
extern void bs_uint8_t_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step);
extern void bs_int16_t_out( int16_t * srcData, int16_t * dstData, int w, int x, int absx, long step);
extern void bs_RGBColor_out( RGBColor * srcData, RGBColor * dstData, int w, int x, int absx, long step);
extern void bs_int32_t_out( int32_t * srcData, int32_t * dstData, int w, int x, int absx, long step);
extern void bs_float_out( float * srcData, float * dstData, int w, int x, int absx, long step);
extern void bs_double_out( double * srcData, double * dstData, int w, int x, int absx, long step);
extern void bs_Complex_out( Complex * srcData, Complex * dstData, int w, int x, int absx, long step);
extern void bs_DComplex_out( DComplex * srcData, DComplex * dstData, int w, int x, int absx, long step);

/* bitstroke copy routines */
extern void bc_byte_put( Byte * source, Byte * dest, unsigned int count, BitBltProc * blt, Byte * colorref);
extern void bc_nibble_copy( Byte * source, Byte * dest, unsigned int from, unsigned int width);
extern void bc_nibble_put( Byte * source, unsigned int from, unsigned int width, Byte * dest, unsigned int to, BitBltProc * blt, Byte * colorref8to4);
extern void bc_mono_copy( Byte * source, Byte * dest, unsigned int from, unsigned int width);
extern void bc_mono_put( Byte * source, unsigned int from, unsigned int width, Byte * dest, unsigned int to, BitBltProc * blt);

/* image conversion routines */
#define BC(from,to,conv) void ic_##from##_##to##_ict##conv( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)
#define BC2(from,to) void ic_##from##_##to( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)

extern BC(mono,mono,None);
extern BC(mono,mono,Optimized);
extern BC(mono,nibble,None);
extern BC(mono,byte,None);
extern BC(mono,graybyte,None);
extern BC(mono,rgb,None);
extern BC(nibble,mono,None);
extern BC(nibble,mono,Ordered);
extern BC(nibble,mono,ErrorDiffusion);
extern BC(nibble,mono,Optimized);
extern BC(nibble,nibble,None);
extern BC(nibble,nibble,Posterization);
extern BC(nibble,nibble,Ordered);
extern BC(nibble,nibble,ErrorDiffusion);
extern BC(nibble,nibble,Optimized);
extern BC(nibble,byte,None);
extern BC(nibble,graybyte,None);
extern BC(nibble,rgb,None);
extern BC(byte,mono,None);
extern BC(byte,mono,Ordered);
extern BC(byte,mono,ErrorDiffusion);
extern BC(byte,mono,Optimized);
extern BC(byte,nibble,None);
extern BC(byte,nibble,Posterization);
extern BC(byte,nibble,Ordered);
extern BC(byte,nibble,ErrorDiffusion);
extern BC(byte,nibble,Optimized);
extern BC(byte,byte,None);
extern BC(byte,byte,Ordered);
extern BC(byte,byte,Posterization);
extern BC(byte,byte,ErrorDiffusion);
extern BC(byte,graybyte,None);
extern BC(byte,rgb,None);
extern BC(graybyte,mono,None);
extern BC(graybyte,mono,Ordered);
extern BC(graybyte,mono,ErrorDiffusion);
extern BC(graybyte,nibble,None);
extern BC(graybyte,nibble,Ordered);
extern BC(graybyte,nibble,ErrorDiffusion);
extern BC(graybyte,rgb,None);
extern BC(rgb,mono,None);
extern BC(rgb,mono,Posterization);
extern BC(rgb,mono,Ordered);
extern BC(rgb,mono,ErrorDiffusion);
extern BC(rgb,mono,Optimized);
extern BC(rgb,nibble,None);
extern BC(rgb,nibble,Posterization);
extern BC(rgb,nibble,Ordered);
extern BC(rgb,nibble,ErrorDiffusion);
extern BC(rgb,nibble,Optimized);
extern BC(rgb,byte,None);
extern BC(rgb,byte,Posterization);
extern BC(rgb,byte,Ordered);
extern BC(rgb,byte,ErrorDiffusion);
extern BC(rgb,byte,Optimized);
extern BC(rgb,graybyte,None);
extern BC(byte,byte,Optimized);
extern BC(nibble,nibble,Optimized);

extern BC2(Byte,Short);
extern BC2(Byte,Long);
extern BC2(Byte,float);
extern BC2(Byte,double);
extern BC2(Short,Byte);
extern BC2(Short,Long);
extern BC2(Short,float);
extern BC2(Short,double);
extern BC2(Long,Byte);
extern BC2(Long,Short);
extern BC2(Long,float);
extern BC2(Long,double);
extern BC2(float,Byte);
extern BC2(float,Short);
extern BC2(float,Long);
extern BC2(float,double);
extern BC2(double,Byte);
extern BC2(double,Short);
extern BC2(double,Long);
extern BC2(double,float);

extern BC2(Byte,float_complex);
extern BC2(Byte,double_complex);
extern BC2(Short,float_complex);
extern BC2(Short,double_complex);
extern BC2(Long,float_complex);
extern BC2(Long,double_complex);
extern BC2(float,float_complex);
extern BC2(float,double_complex);
extern BC2(double,float_complex);
extern BC2(double,double_complex);

extern BC2(double_complex,double);
extern BC2(double_complex,float);
extern BC2(double_complex,Long);
extern BC2(double_complex,Short);
extern BC2(double_complex,Byte);
extern BC2(float_complex,double);
extern BC2(float_complex,float);
extern BC2(float_complex,Long);
extern BC2(float_complex,Short);
extern BC2(float_complex,Byte);


/* image resampling routines */
extern void rs_Byte_Byte( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_Short_Short( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_Long_Long( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_float_float( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_double_double( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_Short_Byte( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_Long_Byte( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_float_Byte( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);
extern void rs_double_Byte( Handle self, Byte * dstData, int dstType, double srcLo, double srcHi, double dstLo, double dstHi);

/* extra converters */
extern void bc_irgb_rgb( Byte * source, Byte * dest, int count);
extern void bc_ibgr_rgb( Byte * source, Byte * dest, int count);
extern void bc_bgri_rgb( Byte * source, Byte * dest, int count);
extern void bc_rgbi_rgb( Byte * source, Byte * dest, int count);
extern void bc_rgb_irgb( Byte * source, Byte * dest, int count);
extern void bc_rgb_rgbi( Byte * source, Byte * dest, int count);
extern void bc_rgb_ibgr( Byte * source, Byte * dest, int count);
extern void bc_rgb_bgri( Byte * source, Byte * dest, int count);
extern void bc_rgba_rgb_a( Byte * rgba_source, Byte * rgb_dest, Byte * a_dest, int count);
extern void bc_rgba_bgr_a( Byte * rgba_source, Byte * bgr_dest, Byte * a_dest, int count);
extern void bc_rgb_a_rgba( Byte * rgb_source, Byte * a_source, Byte * rgba_dest, int count);
extern void bc_bgr_a_rgba( Byte * bgr_source, Byte * a_source, Byte * rgba_dest, int count);


/* remove pixels based on a mask */
extern void bc_a8mask_nibble( register Byte *mask, register Byte *pixels, unsigned int width);
extern void bc_a8mask_multibyte( register Byte *mask, register Byte *pixels, unsigned int width, int pixel_width);

/* misc */

extern void ibc_repad( Byte * source, Byte * dest, int srcLineSize, int dstLineSize, int srcDataSize, int dstDataSize, int srcBPP, int dstBPP, void * bit_conv_proc, Bool reverse);
extern void img_fill_dummy( PImage dummy, int w, int h, int type, Byte * data, RGBColor * palette);
extern Bool img_put( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop, PRegionRec region, Byte * color);
extern Bool img_bar( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx);
extern void img_integral_rotate( Handle self, Byte * new_data, int new_line_size, int degrees);
extern Bool img_generic_rotate( Handle self, float degrees, PImage output, ColorPixel fill, NPoint delta, Point *aperture);
extern Bool img_2d_transform( Handle self, Matrix matrix, ColorPixel fill, PImage output, Point *aperture);
extern void img_mirror( Handle self, Bool vertically);
extern Bool img_mirror_raw( int type, int w, int h, Byte * data, Bool vertically);
extern void img_premultiply_alpha_constant( Handle self, int alpha);
extern void img_premultiply_alpha_map( Handle self, Handle alpha);
extern Bool img_polyline( Handle dest, int n_points, Point * points, PImgPaintContext ctx);
extern Bool img_flood_fill( Handle self, int x, int y, ColorPixel color, Bool single_border, PImgPaintContext ctx);
extern PBitBltProc img_find_blt_proc( int rop );
extern Bool img_find_blend_proc( int rop, BlendFunc ** blend1, BlendFunc ** blend2 );
extern Bool img_resample_colors( Handle dest, int bpp, PImgPaintContext ctx);
extern void img_fill_alpha_buf( Byte * dst, Byte * src, int width, int bpp);
extern void img_plot_glyph( Handle self, PImage glyph, int x, int y, PImgPaintContext ctx);

typedef struct _NPolyPolyline {
	unsigned int n_points, size;
	Byte *lj_hints;
	double theta; /* tangent for single-point polylines */
	struct _NPolyPolyline *next, *prev;
	NPoint *points;
	Byte buf[1];
} NPolyPolyline;

extern NPolyPolyline* img_polyline2patterns(
	NPoint * points, int n_points, Byte * lj_hints,
	double line_width, unsigned char * line_pattern,
	Bool integer_precision
);

/* regions */
typedef Bool RegionCallbackFunc( int x, int y, int w, int h, void * param);

extern Box img_region_box(PRegionRec region);
#define img_region_new(sz) img_region_alloc(NULL,sz)
extern PRegionRec img_region_alloc(PRegionRec old_region, int n_size);
extern PRegionRec img_region_extend(PRegionRec region, int x, int y, int width, int height);
extern Bool img_region_foreach(
	PRegionRec region, 
	int x, int y, int w, int h,
	RegionCallbackFunc *cb, void *param
);
extern Bool img_point_in_region( int x, int y, PRegionRec region);
extern PRegionRec img_region_polygon( Point *pts, int count, int rule);
extern PRegionRec img_region_mask( Handle mask);
extern void img_region_offset( PRegionRec region, int dx, int dy);

/* istXXX function */
typedef double FilterFunc( const double x );
typedef struct {
	unsigned int id;
	FilterFunc * filter;
	double support;
} FilterRec;
extern FilterRec ist_filters[];

/* internal maps */
extern Byte     map_stdcolorref    [ 256];
extern Byte     div51              [ 256];
extern Byte     div51f             [ 256];
extern Byte     div17              [ 256];
extern Byte     mod51              [ 256];
extern int8_t   mod51f             [ 256];
extern Byte     mod17mul3          [ 256];
extern RGBColor cubic_palette      [ 256];
extern RGBColor cubic_palette8     [   8];
extern RGBColor cubic_palette16    [  16];
extern RGBColor stdmono_palette    [   2];
extern RGBColor std16gray_palette  [  16];
extern RGBColor std256gray_palette [ 256];
extern Byte     map_halftone8x8_51 [  64];
extern Byte     map_halftone8x8_64 [  64];


/* internal macros */

#define dBCARGS                                                   \
	int i;                                                         \
	int width = var->w, height = var->h;                           \
	int srcType = var->type;                                       \
	int srcLine = LINE_SIZE(width,srcType);                        \
	int dstLine = LINE_SIZE(width,dstType);                        \
	Byte * srcData = var->data;                                    \
	Byte colorref[ 256]

#define dBCLOOP \
	Byte * srcDataLoop = srcData + i * srcLine;\
	Byte * dstDataLoop = dstData + i * dstLine

#if defined (__BORLANDC__)
#define BCWARN
#else
#define BCWARN                                                   \
	(void)srcType; (void)srcLine; (void)dstLine;                  \
	(void)srcData; (void)colorref; (void)i;
#endif


#define BCCONV srcData, dstData, width
#define BCCONVLOOP srcDataLoop, dstDataLoop, width
#define BCINCR srcData += srcLine; dstData += dstLine

#define map_RGB_gray ((Byte*)std256gray_palette)

#define PAL_FREE   0x8000
#define PAL_REF    0x4000
#define CELL_SIZE  64

#ifdef __cplusplus
}
#endif

#endif
