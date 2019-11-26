#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define var (( PImage) self)
#define my  ((( PImage) self)-> self)

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
	Fixed    count = {step/2};
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

#define STEP(x) ((double)x * (double)(UINT16_PRECISION))

void
ic_stretch_box( int type, Byte * srcData, int srcW, int srcH, Byte * dstData, int w, int h, Bool xStretch, Bool yStretch)
{
	int  absh = h < 0 ? -h : h;
	int  absw = w < 0 ? -w : w;
	int  srcLine = LINE_SIZE(srcW, type);
	int  dstLine = LINE_SIZE(absw, type);

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
			dstLine = -dstLine;
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
			dstLine = -dstLine;
		}
		if ( absh < srcH)
		{
			ystep. l = STEP( absh / srcH );
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
			ystep. l = STEP( srcH / absh);
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
	if ( srcW < absw || srcH < absh || ( type & imBPP) == imNibble)
		memset( dstData, 0, dstLine * absh);
	if ( absw < srcW)
		xstep. l = STEP( absw / srcW);
	else
		xstep. l = STEP( srcW / absw);
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
			dstLine = -dstLine;
		}
		for ( i = 0; i < yMin; i++, srcData += srcLine, dstData += dstLine)
			proc( srcData, dstData, srcW, w, absw, xstep.l);
		return;
	}

/* general case */
	if ( h < 0)
	{
		dstData += dstLine * ( absh - 1);
		dstLine = -dstLine;
	}
	if ( absh < srcH)
	{
		int j = 0;
		ystep. l = STEP( absh / srcH);
		proc( srcData, dstData, srcW, w, absw, xstep.l);
		dstData += dstLine;
		j++;
		for ( i = 0; i < srcH; i++)
		{
			if ( count. i.i > last)
			{
				proc( srcData, dstData, srcW, w, absw, xstep.l);
				dstData += dstLine;
				last = count.i.i;
				j++;
			}
			count. l += ystep. l;
			srcData += srcLine;
		}
	} else {
		ystep. l = STEP( srcH / absh);
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

/* Resizing with filters - stolen from ImageMagick MagickCore/resize.c */

#define PI  3.14159265358979323846264338327950288419716939937510
#define PI2 1.57079632679489661923132169163975144209858469968755

static double
filter_sinc_fast(const double x)
{
/*
	Approximations of the sinc function sin(pi x)/(pi x) over the interval
	[-4,4] constructed by Nicolas Robidoux and Chantal Racette with funding
	from the Natural Sciences and Engineering Research Council of Canada.

	Although the approximations are polynomials (for low order of
	approximation) and quotients of polynomials (for higher order of
	approximation) and consequently are similar in form to Taylor polynomials /
	Pade approximants, the approximations are computed with a completely
	different technique.

	Summary: These approximations are "the best" in terms of bang (accuracy)
	for the buck (flops). More specifically: Among the polynomial quotients
	that can be computed using a fixed number of flops (with a given "+ - * /
	budget"), the chosen polynomial quotient is the one closest to the
	approximated function with respect to maximum absolute relative error over
	the given interval.

	The Remez algorithm, as implemented in the boost library's minimax package,
	is the key to the construction: http://www.boost.org/doc/libs/1_36_0/libs/
	math/doc/sf_and_dist/html/math_toolkit/backgrounders/remez.html

	If outside of the interval of approximation, use the standard trig formula.
*/
	if (x > 4.0)
	{
		const double alpha=(double) (PI*x);
		return(sin((double) alpha)/alpha);
	}
	{
		/*
			The approximations only depend on x^2 (sinc is an even function).
		*/
		const double xx = x*x;
		/*
			Maximum absolute relative error 6.3e-6 < 1/2^17.
		*/
		const double c0 = 0.173610016489197553621906385078711564924e-2L;
		const double c1 = -0.384186115075660162081071290162149315834e-3L;
		const double c2 = 0.393684603287860108352720146121813443561e-4L;
		const double c3 = -0.248947210682259168029030370205389323899e-5L;
		const double c4 = 0.107791837839662283066379987646635416692e-6L;
		const double c5 = -0.324874073895735800961260474028013982211e-8L;
		const double c6 = 0.628155216606695311524920882748052490116e-10L;
		const double c7 = -0.586110644039348333520104379959307242711e-12L;
		const double p =
			c0+xx*(c1+xx*(c2+xx*(c3+xx*(c4+xx*(c5+xx*(c6+xx*c7))))));
		return((xx-1.0)*(xx-4.0)*(xx-9.0)*(xx-16.0)*p);
	}
}

static double filter_triangle(const double x)
{
/*
	1st order (linear) B-Spline, bilinear interpolation, Tent 1D filter, or
	a Bartlett 2D Cone filter.
*/
	if (x < 1.0)
		return(1.0-x);
	return(0.0);
}


static double
filter_quadratic(const double x)
{
/*
	2rd order (quadratic) B-Spline approximation of Gaussian.
*/
	if (x < 0.5)
		return (0.75-x*x);
	if (x < 1.5)
		return (0.5*(x-1.5)*(x-1.5));
	return(0.0);
}

/*
Cubic Filters using B,C determined values:
	Mitchell-Netravali  B = 1/3 C = 1/3  "Balanced" cubic spline filter
	Catmull-Rom         B = 0   C = 1/2  Interpolatory and exact on linears
	Spline              B = 1   C = 0    B-Spline Gaussian approximation
	Hermite             B = 0   C = 0    B-Spline interpolator

See paper by Mitchell and Netravali, Reconstruction Filters in Computer
Graphics Computer Graphics, Volume 22, Number 4, August 1988
http://www.cs.utexas.edu/users/fussell/courses/cs384g/lectures/mitchell/
Mitchell.pdf.

Coefficents are determined from B,C values:
	P0 = (  6 - 2*B       )/6 = coeff[0]
	P1 =         0
	P2 = (-18 +12*B + 6*C )/6 = coeff[1]
	P3 = ( 12 - 9*B - 6*C )/6 = coeff[2]
	Q0 = (      8*B +24*C )/6 = coeff[3]
	Q1 = (    -12*B -48*C )/6 = coeff[4]
	Q2 = (      6*B +30*C )/6 = coeff[5]
	Q3 = (    - 1*B - 6*C )/6 = coeff[6]

which are used to define the filter:

	P0 + P1*x + P2*x^2 + P3*x^3      0 <= x < 1
	Q0 + Q1*x + Q2*x^2 + Q3*x^3      1 <= x < 2

which ensures function is continuous in value and derivative (slope).
*/
static double
filter_cubic_spline0(const double x)
{
	if (x < 1.0) return 1+x*(x*(-3.0+x*2));
	return 0.0;
}

static double
filter_cubic_spline1(const double x)
{
	if (x < 1.0)
		return(2.0/3.0+x*(x*(-1.0+x/2.0)));
	if (x < 2.0)
		return(4.0/3.0+x*(-2.0+x*(1.0-x/6.0)));
	return(0.0);
}

static double
filter_gaussian(const double x)
{
/* Gaussian with a sigma = 1/2 */
	return exp((double)(x*x*-2.0));
}


FilterRec ist_filters[] = {
	{ istTriangle,  filter_triangle,      1.0 },
	{ istQuadratic, filter_quadratic,     1.5 },
	{ istSinc,      filter_sinc_fast,     4.0 },
	{ istHermite,   filter_cubic_spline0, 1.0 },
	{ istCubic,     filter_cubic_spline1, 2.0 },
	{ istGaussian,  filter_gaussian,      2.0 },
	{ 0, NULL, 0.0 }
};

static int
fill_contributions( FilterRec * filter, double * contributions, int * start, int offset, double factor, int max, double support, Bool as_fixed )
{
	double bisect, density;
	int n, stop;

	bisect = (double) (offset + 0.5) / factor;
	*start  = bisect - support + 0.5;
	if ( *start < 0 ) *start = 0;
	stop   = bisect + support + 0.5;
	if ( stop > max ) stop = max;

	density = 0.0;
	for (n = 0; n < (stop-*start); n++) {
		contributions[n] = filter->filter(fabs((double) (*start+n)-bisect+0.5));
		density += contributions[n];
	}

	if ( density != 0.0 && density != 1.0 ) {
		int i;
		for ( i = 0; i < n; i++) contributions[i] /= density;
	}

	if ( as_fixed && n > 0 ) {
		int i    = (sizeof(Fixed) > sizeof(double)) ? (n - 1) : 0;
		int to   = (sizeof(Fixed) > sizeof(double)) ? -1 : n;
		int incr = (sizeof(Fixed) > sizeof(double)) ? -1 : 1;
		for ( ; i != to; i += incr )
			((Fixed*)(contributions))[i].l = contributions[i] * 65536.0 + .5;
	}

	return n;
}

#define STRETCH_HORIZONTAL_OPEN(type) \
static void \
stretch_horizontal_##type(  \
		FilterRec * filter, double scale, double * contribution_storage, double support,  \
		int channels, Byte * src_data, int src_w, int src_h,  \
		Byte * dst_data, int dst_w, int dst_h, double x_factor, int contribution_chunk \
) { \
	int x, src_line_size, dst_line_size; \
	src_line_size = LINE_SIZE(src_w * channels, 8*sizeof(type)); \
	dst_line_size = LINE_SIZE(dst_w * channels, 8*sizeof(type));\
	if ( src_w == dst_w && src_h == dst_h ) { \
		memcpy( dst_data, src_data, dst_line_size * dst_h);\
		return;\
	}

#define STRETCH_HORIZONTAL_LOOP(type,pixel_init,pixel_access,as_fixed,contrib,roundoff) \
	for (x = 0; x < dst_w; x++) { \
		double * contributions = (double*) (((Byte*)contribution_storage) + contribution_chunk * OMP_THREAD_NUM); \
		int y, c, start, n = fill_contributions( filter, contributions, &start, x, x_factor, src_w, support, as_fixed ); \
		Byte * src = src_data + start * channels * sizeof(type); \
		Byte * dst = dst_data + x     * channels * sizeof(type); \
		for ( c = 0; c < channels; c++, src += sizeof(type), dst += sizeof(type)) { \
			Byte *src_y = src, *dst_y = dst; \
			for ( y = 0; y < dst_h; y++, src_y += src_line_size, dst_y += dst_line_size ) { \
				register int j; \
				pixel_init;\
				register type *src_j = (type*)src_y; \
				for ( j = 0; j < n; j++, src_j += channels) \
					pixel_access += (contrib * *src_j roundoff);

#define STRETCH_PUTBACK(type) \
				*((type*)(dst_y))
#define STRETCH_HORIZONTAL_CLOSE(type) STRETCH_PUTBACK(type) = pixel; }}}}

#define STRETCH_VERTICAL_OPEN(type) \
static void \
stretch_vertical_##type(  \
		FilterRec * filter, double scale, double * contribution_storage, double support,  \
		Byte * src_data, int src_w, int src_h,  \
		Byte * dst_data, int dst_w, int dst_h, double y_factor, int contribution_chunk \
) { \
	int y, src_line_size, dst_line_size; \
	src_line_size = LINE_SIZE(src_w, 8*sizeof(type)); \
	dst_line_size = LINE_SIZE(dst_w, 8*sizeof(type));\
	if ( src_w == dst_w && src_h == dst_h ) { \
		memcpy( dst_data, src_data, dst_line_size * dst_h);\
		return;\
	}

#define STRETCH_VERTICAL_LOOP(type,pixel_init,pixel_access,as_fixed,contrib,roundoff) \
	for ( y = 0; y < dst_h; y++) { \
		Byte *src_y, *dst_y; \
		double * contributions = (double*) (((Byte*)contribution_storage) + contribution_chunk * OMP_THREAD_NUM); \
		int x, start, n = fill_contributions( filter, contributions, &start, y, y_factor, src_h, support, as_fixed ); \
		src_y = src_data + start * src_line_size; \
		dst_y = dst_data + y     * dst_line_size; \
		for ( x = 0; x < dst_w; x++, src_y += sizeof(type), dst_y += sizeof(type)) { \
			int j; \
			pixel_init; \
			Byte * src = src_y;\
			for ( j = 0; j < n; j++, src += src_line_size) \
				pixel_access += (contrib * *((type*)(src)) roundoff);

#define STRETCH_VERTICAL_CLOSE(type) STRETCH_PUTBACK(type) = pixel; }}}

#define STRETCH_FIXED_BYTE_CLAMP \
	if ( pixel.i.i < 0 ) pixel.i.i = 0;\
	if ( pixel.i.i > 255 ) pixel.i.i = 255;\

STRETCH_HORIZONTAL_OPEN(Byte)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	STRETCH_HORIZONTAL_LOOP(Byte,Fixed pixel = {0},pixel.l,1,((Fixed*)contributions)[j].l,)
	STRETCH_FIXED_BYTE_CLAMP
	STRETCH_PUTBACK(Byte)=pixel.i.i;}}}
}

STRETCH_VERTICAL_OPEN(Byte)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	STRETCH_VERTICAL_LOOP(Byte,Fixed pixel = {0},pixel.l,1,((Fixed*)contributions)[j].l,)
	STRETCH_FIXED_BYTE_CLAMP
	STRETCH_PUTBACK(Byte)=pixel.i.i;}}
}

#define STRETCH_CLAMP(min,max) \
	if ( pixel < min ) pixel = min;\
	if ( pixel > max ) pixel = max;

STRETCH_HORIZONTAL_OPEN(Short)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_HORIZONTAL_LOOP(Short,register long pixel = 0,pixel,0,contributions[j],+.5)
STRETCH_CLAMP(INT16_MIN,INT16_MAX)
STRETCH_HORIZONTAL_CLOSE(Short)

STRETCH_VERTICAL_OPEN(Short)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_VERTICAL_LOOP(Short,register long pixel = 0,pixel,0,contributions[j],+.5)
STRETCH_CLAMP(INT16_MIN,INT16_MAX)
STRETCH_VERTICAL_CLOSE(Short)

STRETCH_HORIZONTAL_OPEN(Long)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_HORIZONTAL_LOOP(Long,register int64_t pixel = 0,pixel,0,contributions[j],+.5)
STRETCH_CLAMP(INT32_MIN,INT32_MAX)
STRETCH_HORIZONTAL_CLOSE(Long)

STRETCH_VERTICAL_OPEN(Long)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_VERTICAL_LOOP(Long,register int64_t pixel = 0,pixel,0,contributions[j],+.5)
STRETCH_CLAMP(INT32_MIN,INT32_MAX)
STRETCH_VERTICAL_CLOSE(Long)

STRETCH_HORIZONTAL_OPEN(float)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_HORIZONTAL_LOOP(float,register double pixel = 0,pixel,0,contributions[j],)
STRETCH_HORIZONTAL_CLOSE(float)

STRETCH_VERTICAL_OPEN(float)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_VERTICAL_LOOP(float,register double pixel = 0,pixel,0,contributions[j],)
STRETCH_VERTICAL_CLOSE(float)

STRETCH_HORIZONTAL_OPEN(double)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_HORIZONTAL_LOOP(double,register double pixel = 0,pixel,0,contributions[j],)
STRETCH_HORIZONTAL_CLOSE(double)

STRETCH_VERTICAL_OPEN(double)
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
STRETCH_VERTICAL_LOOP(double,register double pixel = 0,pixel,0,contributions[j],)
STRETCH_VERTICAL_CLOSE(double)

static Bool
stretch_filtered( int type, Byte * oldData, int oldW, int oldH, Byte * newData, int w, int h, int scaling, char * error )
{
	int channels, fw, fh, flw, i, support_size;
	double factor_x, factor_y, scale_x, scale_y, *contributions, support_x, support_y ;
	Byte * filter_data;
	FilterRec * filter = NULL;

	for ( i = 0; ; i++) {
                if ( ist_filters[i]. id == 0 ) break;
		if ( ist_filters[i]. id == scaling ) {
			filter = &ist_filters[i];
			break;
		}
	}
	if ( !filter ) {
		strncpy( error, "no appropriate scaling filter found", 255);
		return false;
	}
	if ( w <= 0 || h <= 0) {
		strncpy(error, "image dimensions must be positive", 255);
		return false;
	}

	switch (type) {
	case imMono:
	case imBW:
	case imNibble:
	case im256:
	case imNibble | imGrayScale:
		strncpy(error, "type not supported", 255);
		return false;
	case imRGB:
		channels = 3;
		break;
	case imByte:
		channels = 1;
		break;
	case imComplex:
	case imDComplex:
	case imTrigComplex:
	case imTrigDComplex:
		channels = 2;
		break;
	default:
		channels = 1;
	}

	/* convert to multi-channel structures */
	if ( type == imRGB ) {
		type = imByte;
		w *= 3;
		oldW *= 3;
	}
	if ( channels == 2 ) {
		w *= 2;
		oldW *= 2;
		type = (( type & imBPP ) / 2) | imGrayScale | imRealNumber;
	}

	/* allocate space for semi-filtered and target data */
	factor_x = (double) w / (double) oldW;
	factor_y = (double) h / (double) oldH;
	if (factor_x > factor_y) {
		fw = w;
		fh = oldH;
	} else {
		fw = oldW;
		fh = h;
	}
	flw = LINE_SIZE( fw, type);

	if ( !( filter_data = malloc( flw * fh ))) {
		snprintf(error, 255, "not enough memory: %d bytes", flw * fh);
		return false;
	}

	scale_x = 1.0 / factor_x;
	if ( scale_x < 1.0 ) scale_x = 1.0;
	scale_y = 1.0 / factor_y;
	if ( scale_y < 1.0 ) scale_y = 1.0;
	support_x = scale_x * filter-> support;
	support_y = scale_y * filter-> support;
	scale_x = 1.0 / scale_x;
	scale_y = 1.0 / scale_y;
	/* Support too small even for nearest neighbour: Reduce to point sampling.  */
	if (support_x < 0.5) support_x = (double) 0.5;
	if (support_y < 0.5) support_y = (double) 0.5;
	support_size = (int)(2.0 * (( support_x < support_y ) ? support_y : support_x) * 3.0);
	support_size *= (sizeof(Fixed) > sizeof(double)) ? sizeof(Fixed) : sizeof(double);
	if (!(contributions = malloc(support_size * OMP_MAX_THREADS))) {
		free( filter_data );
		snprintf(error, 255, "not enough memory: %d bytes", support_size);
		return false;
	}

	/* stretch */
	if (factor_x > factor_y) {
#define HORIZONTAL(type) stretch_horizontal_##type( \
		filter, scale_x, contributions, support_x, channels, \
		oldData, oldW / channels, oldH, filter_data, fw / channels, fh, factor_x, support_size)
#define VERTICAL(type)   stretch_vertical_##type  ( \
		filter, scale_y, contributions, support_y, \
		filter_data, fw, fh, newData, w, h, factor_y, support_size )
#define HANDLE_TYPE(type,name) \
		case name: \
			HORIZONTAL(type);\
			VERTICAL(type);\
			break
		switch ( type ) {
			HANDLE_TYPE(Byte, imByte);
			HANDLE_TYPE(Short, imShort);
			HANDLE_TYPE(Long, imLong);
			HANDLE_TYPE(float, imFloat);
			HANDLE_TYPE(double, imDouble);
		default:
			croak("panic: bad image type: %x", type);
		}
#undef HORIZONTAL
#undef VERTICAL
#undef HANDLE_TYPE
	} else {
#define VERTICAL(type)   stretch_vertical_##type  ( \
		filter, scale_y, contributions, support_y, \
		oldData, oldW, oldH, filter_data, fw, fh, factor_y, support_size)
#define HORIZONTAL(type) stretch_horizontal_##type( \
		filter, scale_x, contributions, support_x, channels, \
		filter_data, fw / channels, fh, newData, w / channels, h, factor_x, support_size)
#define HANDLE_TYPE(type,name) \
		case name: \
			VERTICAL(type);\
			HORIZONTAL(type);\
			break
		switch ( type ) {
			HANDLE_TYPE(Byte, imByte);
			HANDLE_TYPE(Short, imShort);
			HANDLE_TYPE(Long, imLong);
			HANDLE_TYPE(float, imFloat);
			HANDLE_TYPE(double, imDouble);
		default:
			croak("panic: bad image type: %x", type);
		}
#undef HORIZONTAL
#undef VERTICAL
#undef HANDLE_TYPE
	}
	free( contributions );
	free( filter_data );
	return true;
}

Bool
ic_stretch_filtered( int type, Byte * oldData, int oldW, int oldH, Byte * newData, int w, int h, int scaling, char * error )
{
	int absw, absh;
	Bool mirror_x, mirror_y;

	absw = abs(w);
	absh = abs(h);
	mirror_x = w < 0;
	mirror_y = h < 0;

	/* if it's cheaper to mirror before the conversion, do it */
	if ( mirror_y && oldH < absh ) {
		img_mirror_raw( type, oldW, oldH, oldData, 1 );
		mirror_y = 0;
	}
	if ( mirror_x && oldW < absw) {
		img_mirror_raw( type, oldW, oldH, oldData, 0 );
		mirror_x = 0;
	}

	if ( !stretch_filtered( type, oldData, oldW, oldH, newData, w, h, scaling, error ))
		return false;

	if ( mirror_x ) img_mirror_raw( type, w, h, newData, 0 );
	if ( mirror_y ) img_mirror_raw( type, w, h, newData, 1 );

	return true;
}

int
ic_stretch_suggest_type( int type, int scaling )
{
	if ( scaling <= istBox ) return type;

	switch (type) {
	case imMono:
	case imNibble:
	case im256:
	case imRGB:
		return imRGB;
	case imBW:
	case imNibble | imGrayScale:
	case imByte:
		return imByte;
		break;
	default:
		return type;
	}
}

Bool
ic_stretch( int type, Byte * srcData, int srcW, int srcH, Byte * dstData, int w, int h, int scaling, char * error)
{
	if ( scaling <= istBox ) {
		ic_stretch_box( type, srcData, srcW, srcH, dstData, w, h, scaling & istBoxX, scaling & istBoxY);
		return true;
	} else {
		return ic_stretch_filtered( type, srcData, srcW, srcH, dstData, w, h, scaling, error );
	}
}

#ifdef __cplusplus
}
#endif
