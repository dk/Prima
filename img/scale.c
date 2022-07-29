#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define var (( PImage) self)
#define my  ((( PImage) self)-> self)

#define BS_BYTEIMPACT_ROP(type,op,expr)               \
void bs_##type##_##op(                                \
	type * src_data, type * dst_data,             \
	int w, int x, int absx, long step             \
){                                                    \
	Fixed count = {0};                            \
	int   last = 0;                               \
	int   i, k;                                   \
	int   j    = ( x == absx) ? 0 : ( absx - 1);  \
	int   inc  = ( x == absx) ? 1 : -1;           \
	dst_data[k=j] = src_data[0];                  \
	j += inc;                                     \
	for ( i = 0; i < w; i++) {                    \
		if ( count.i.i > last) {              \
			dst_data[k=j] = src_data[i];  \
			j += inc;                     \
			last = count.i.i;             \
		}                                     \
		count.l += step;                      \
		expr;                                 \
	}                                             \
}

#define BS_BYTEIMPACT(type)                           \
	BS_BYTEIMPACT_ROP(type,in,)

#define BS_BYTEIMPACT_INT(type)                       \
	BS_BYTEIMPACT_ROP(type,in,)                   \
	BS_BYTEIMPACT_ROP(type,and,dst_data[k]&=src_data[i]) \
	BS_BYTEIMPACT_ROP(type,or,dst_data[k]|=src_data[i])

#define BS_BYTEEXPAND( type)                          \
void bs_##type##_out(                                 \
	type * src_data, type * dst_data,             \
	int w, int x, int absx, long step             \
){                                                    \
	Fixed count = {0};                            \
	int   i;                                      \
	int   j    = ( x == absx) ? 0 : ( absx - 1);  \
	int   inc  = ( x == absx) ? 1 : -1;           \
	int   last = 0;                               \
	for ( i = 0; i < absx; i++) {                 \
		if ( count.i.i > last) {              \
			last = count.i.i;             \
			src_data++;                   \
		}                                     \
		count.l += step;                      \
		dst_data[j] = *src_data;              \
		j += inc;                             \
	}                                             \
}

BS_BYTEEXPAND( uint8_t)
BS_BYTEEXPAND( int16_t)
BS_BYTEEXPAND( RGBColor)
BS_BYTEEXPAND( int32_t)
BS_BYTEEXPAND( float)
BS_BYTEEXPAND( double)
BS_BYTEEXPAND( Complex)
BS_BYTEEXPAND( DComplex)

BS_BYTEIMPACT_INT( uint8_t)
BS_BYTEIMPACT_INT( int16_t)
BS_BYTEIMPACT_INT( int32_t)
BS_BYTEIMPACT( RGBColor)
BS_BYTEIMPACT_ROP(RGBColor,and,dst_data[k].r&=src_data[i].r;dst_data[k].g&=src_data[i].g;dst_data[k].b&=src_data[i].b)
BS_BYTEIMPACT_ROP(RGBColor,or, dst_data[k].r|=src_data[i].r;dst_data[k].g|=src_data[i].g;dst_data[k].b|=src_data[i].b)
BS_BYTEIMPACT( float)
BS_BYTEIMPACT( double)
BS_BYTEIMPACT( Complex)
BS_BYTEIMPACT( DComplex)


void
bs_mono_in( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   last   = 0;
	register int i, j;
	register U16 xd, xs;

	if ( x == absx) {
		xd = (( xs = src_data[0]) >> 7) & 1;
		j = 1;
		for ( i = 0; i < w; i++) {
			if (( i & 7) == 0) xs = src_data[ i >> 3];
			xs <<= 1;
			if ( count.i.i > last) {
				if (( j & 7) == 0) dst_data[ ( j - 1) >> 3] = xd;
				xd <<= 1;
				xd |= ( xs >> 8) & 1;
				j++;
				last = count.i.i;
			}
			count.l += step;
		}
		i = j & 7;
		dst_data[( j - 1) >> 3] = xd << ( i ? ( 8 - i) : 0);
	} else {
		j = absx - 1;
		xd = ( xs = src_data[ j >> 3]) & 0x80;
		for ( i = 0; i < w; i++) {
			if (( i & 7) == 0) xs = src_data[ i >> 3];
			xs <<= 1;
			if ( count.i.i > last) {
				if (( j & 7) == 0) dst_data[ ( j + 1) >> 3] = xd;
				xd >>= 1;
				xd |= ( xs >> 1) & 0x80;
				j--;
				last = count.i.i;
			}
			count.l += step;
		}
		dst_data[( j + 1) >> 3] = xd;
	}
}

#define BIT(x) (1 << (7 - (x & 7)))

void
bs_mono_and( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   last = 0;
	int   i, k;
	int   j    = (x == absx) ? 0 : (absx - 1);
	int   inc  = (x == absx) ? 1 : -1;
	k = j;
	dst_data[j >> 3] = src_data[0] & 0x80;
	j += inc;
	for ( i = 0; i < w; i++) {
		if ( count.i.i > last) {
			k = j;
			if ( src_data[i >> 3] & BIT(i))
				dst_data[j >> 3] |= BIT(j);
			else
				dst_data[j >> 3] &= ~BIT(j);
			j += inc;
			last = count.i.i;
		} else if (( src_data[i >> 3] & BIT(i)) == 0)
			dst_data[k >> 3] &= ~BIT(k);
		count.l += step;
	}
}

void
bs_mono_or( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   last = 0;
	int   i, k;
	int   j    = (x == absx) ? 0 : (absx - 1);
	int   inc  = (x == absx) ? 1 : -1;
	k = j;
	dst_data[j >> 3] = src_data[0] & 0x80;
	j += inc;
	for ( i = 0; i < w; i++) {
		if ( count.i.i > last) {
			k = j;
			if ( src_data[i >> 3] & BIT(i))
				dst_data[j >> 3] |= BIT(j);
			else
				dst_data[j >> 3] &= ~BIT(j);
			j += inc;
			last = count.i.i;
		} else if ( src_data[i >> 3] & BIT(i))
			dst_data[k >> 3] |= BIT(k);
		count.l += step;
	}
}

#undef BIT

void
bs_mono_out( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed    count = {step/2};
	register int i, j = 0;
	register U16 xd = 0, xs;
	int last = 0;

	if ( x == absx) {
		xs = src_data[0];
		for ( i = 0; i < absx; i++) {
			if ( count.i.i > last) {
				last = count.i.i;
				xs <<= 1;
				j++;
				if (( j & 7) == 0) xs = src_data[ j >> 3];
			}
			count.l += step;
			xd <<= 1;
			xd |= ( xs >> 7) & 1;
			if ((( i + 1) & 7) == 0) dst_data[ i >> 3] = xd;
		}
		if ( i & 7) dst_data[ i >> 3] = xd << ( 8 - ( i & 7));
		/* dst_data[ i >> 3] = xd << (( i & 7) ? ( 8 - ( i & 7)) : 0); */
	} else {
		register int k = absx;
		xs = src_data[ j >> 3];
		for ( i = 0; i < absx; i++) {
			if ( count.i.i > last) {
				last = count.i.i;
				xs <<= 1;
				j++;
				if (( j & 7) == 0) xs = src_data[ j >> 3];
			}
			count.l += step;
			xd >>= 1;
			xd |= xs & 0x80;
			k--;
			if (( k & 7) == 0) dst_data[( k + 1) >> 3] = xd;
		}
		dst_data[( k + 0) >> 3] = xd;
	}
}

/* nibble stretching functions are requiring *dst_data filled with zeros */

void bs_nibble_in( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   last = 0;
	int   i;
	int   j    = (x == absx) ? 0 : ( absx - 1);
	int   inc  = (x == absx) ? 1 : -1;
	dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ 0] >> 4) : ( src_data[ 0] & 0xF0);
	j += inc;
	for ( i = 0; i < w; i++) {
		if ( count.i.i > last) {
			if ( i & 1)
				dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ i >> 1] & 0x0F) : ( src_data[ i >> 1] << 4);
			else
				dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ i >> 1] >> 4) : ( src_data[ i >> 1] & 0xF0);
			j += inc;
			last = count.i.i;
		}
		count.l += step;
	}
}

void bs_nibble_and( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   last = 0;
	int   i, k;
	int   j    = (x == absx) ? 0 : ( absx - 1);
	int   inc  = (x == absx) ? 1 : -1;
	k = j;
	dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ 0] >> 4) : ( src_data[ 0] & 0xF0);
	j += inc;
	for ( i = 0; i < w; i++) {
		if ( count.i.i > last) {
			k = j;
			if ( i & 1)
				dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ i >> 1] & 0x0F) : ( src_data[ i >> 1] << 4);
			else
				dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ i >> 1] >> 4) : ( src_data[ i >> 1] & 0xF0);
			j += inc;
			last = count.i.i;
		} else {
			if ( i & 1)
				dst_data[ k >> 1] &= ( k & 1) ? ((src_data[ i >> 1] & 0x0F) | 0xF0) : ((src_data[ i >> 1] << 4) | 0x0F);
			else
				dst_data[ k >> 1] &= ( k & 1) ? ((src_data[ i >> 1] >> 4) | 0xF0) : (( src_data[ i >> 1] & 0xF0) | 0x0F);
		}
		count.l += step;
	}
}

void bs_nibble_or( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   last = 0;
	int   i, k;
	int   j    = (x == absx) ? 0 : ( absx - 1);
	int   inc  = (x == absx) ? 1 : -1;
	k = j;
	dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ 0] >> 4) : ( src_data[ 0] & 0xF0);
	j += inc;
	for ( i = 0; i < w; i++) {
		if ( count.i.i > last) {
			k = j;
			if ( i & 1)
				dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ i >> 1] & 0x0F) : ( src_data[ i >> 1] << 4);
			else
				dst_data[ j >> 1] |= ( j & 1) ? ( src_data[ i >> 1] >> 4) : ( src_data[ i >> 1] & 0xF0);
			j += inc;
			last = count.i.i;
		} else {
			if ( i & 1)
				dst_data[ k >> 1] |= ( k & 1) ? ( src_data[ i >> 1] & 0x0F) : ( src_data[ i >> 1] << 4);
			else
				dst_data[ k >> 1] |= ( k & 1) ? ( src_data[ i >> 1] >> 4) : ( src_data[ i >> 1] & 0xF0);
		}
		count.l += step;
	}
}

void bs_nibble_out( uint8_t * src_data, uint8_t * dst_data, int w, int x, int absx, long step)
{
	Fixed count = {0};
	int   i, k = 0;
	int   j    = (x == absx) ? 0 : ( absx - 1);
	int   inc  = (x == absx) ? 1 : -1;
	int   last = 0;
	for ( i = 0; i < absx; i++) {
		if ( count.i.i > last) {
			if (( k & 1) == 1) src_data++;
			k++;
			last = count.i.i;
		}
		count.l += step;
		if ( k & 1)
			dst_data[ j >> 1] |= ( j & 1) ? ( *src_data & 0x0F) : ( *src_data << 4);
		else
			dst_data[ j >> 1] |= ( j & 1) ? ( *src_data >> 4) : ( *src_data & 0xF0);
		j += inc;
	}
}

#define STEP(x) ((double)x * (double)(UINT16_PRECISION))

typedef struct {
	int type;
	PStretchProc in, and, or, out;
} stretch_t;

static stretch_t stretch_procs[] = {
#define SDECL(a,b) ((PStretchProc)bs_##a##_##b)
#define STRETCH_STD(type) SDECL(type,in),SDECL(type,in), SDECL(type,in),SDECL(type,out)
#define STRETCH_INT(type) SDECL(type,in),SDECL(type,and),SDECL(type,or),SDECL(type,out)
	{imMono,               STRETCH_INT(mono)     },
	{imBW,                 STRETCH_INT(mono)     },
	{imNibble,             STRETCH_INT(nibble)   },
	{imNibble|imGrayScale, STRETCH_INT(nibble)   },
	{imByte,               STRETCH_INT(uint8_t)  },
	{im256,                STRETCH_INT(uint8_t)  },
	{imRGB,                STRETCH_INT(RGBColor) },
	{imRGB|imGrayScale,    STRETCH_INT(RGBColor) },
	{imShort,              STRETCH_INT(int16_t)  },
	{imLong,               STRETCH_INT(int32_t)  },
	{imFloat,              STRETCH_STD(float)    },
	{imDouble,             STRETCH_STD(double)   },
	{imComplex,            STRETCH_STD(Complex)  },
	{imDComplex,           STRETCH_STD(DComplex) },
	{imTrigComplex,        STRETCH_STD(Complex)  },
	{imTrigDComplex,       STRETCH_STD(DComplex) }
#undef STRETCH_STD
#undef STRETCH_INT
#undef SDECL
};

static void
bitblt_or( Byte * src, Byte * dst, int count)
{
	while ( count--) *(dst++) |= *(src++);
}

static void
bitblt_and( Byte * src, Byte * dst, int count)
{
	while ( count--) *(dst++) &= *(src++);
}

Bool
ic_stretch_box( int type, Byte * src_data, int src_w, int src_h, Byte * dst_data, int w, int h, int scaling, char * error)
{
	int  abs_h = h < 0 ? -h : h;
	int  abs_w = w < 0 ? -w : w;
	int  src_line = LINE_SIZE(src_w, type);
	int  dst_line = LINE_SIZE(abs_w, type);
	Bool x_stretch, y_stretch;

	Fixed xstep, ystep, count;
	int last = 0;
	int i;
	int y_min = ( src_h > abs_h) ? abs_h : src_h;
	PStretchProc xproc = NULL;
	PBitBltProc yproc = NULL;
	Byte *src_last = NULL, impl_buf[1024];
	semistatic_t pimpl_buf;

	if ( scaling < istAND ) {
		x_stretch = scaling & istBoxX;
		y_stretch = scaling & istBoxY;
	} else
		x_stretch = y_stretch = 1;
	if ( w == src_w) x_stretch = false;
	if ( h == src_h) y_stretch = false;

	/* transfer */
	if ( !x_stretch && !y_stretch && ( w > 0)) {
		int y;
		int x_min = (( type & imBPP) < 8) ?
			( src_line > dst_line) ? dst_line : src_line :
			(((( src_w > abs_w) ? abs_w : src_w) * ( type & imBPP)) / 8);
		if ( src_w < w || src_h < abs_h) memset( dst_data, 0, dst_line * abs_h);
		if ( h < 0)
		{
			dst_data += dst_line * ( y_min - 1);
			dst_line = -dst_line;
		}
		for ( y = 0; y < y_min; y++, src_data += src_line, dst_data += dst_line)
			memcpy( dst_data, src_data, x_min);
		return true;
	}

	/* define processors */
	if ( y_stretch && abs_h < src_h && (scaling == istAND || scaling == istOR))
		yproc = (scaling == istAND) ? bitblt_and : bitblt_or;
	for ( i = 0; i < sizeof(stretch_procs)/sizeof(stretch_t); i++) {
		if ( stretch_procs[i].type != type ) continue;
		if ( src_w > abs_w) {
			if ( scaling == istAND )
				xproc = stretch_procs[i].and;
			else if ( scaling == istOR )
				xproc = stretch_procs[i].or;
			else
				xproc = stretch_procs[i].in;
			if ( xproc == stretch_procs[i].in )
				yproc = NULL;
		} else
			xproc = stretch_procs[i].out;
		break;
	}
	if ( xproc == NULL ) {
		strlcpy(error, "Cannot stretch this image type", 255);
		return false;
	}


/* y-only stretch case */
	if ( !x_stretch && y_stretch && ( w > 0)) {
		int x_min = (( type & imBPP) < 8) ?
			( src_line > dst_line) ? dst_line : src_line :
			(((( src_w > abs_w) ? abs_w : src_w) * ( type & imBPP)) / 8);
		Byte *last_dst_data;
		count.l = 0;
		if ( src_w < w) memset( dst_data, 0, dst_line * abs_h);
		if ( h < 0) {
			dst_data += dst_line * ( abs_h - 1);
			dst_line = -dst_line;
		}
		if ( abs_h < src_h) {
			ystep.l = STEP( abs_h / src_h );
			memcpy( last_dst_data = dst_data, src_data, x_min);
			dst_data += dst_line;
			for ( i = 0; i < src_h; i++) {
				if ( count.i.i > last) {
					memcpy( last_dst_data = dst_data, src_data, x_min);
					dst_data += dst_line;
					last = count.i.i;
				} else if ( yproc && i > 0 )
					yproc( src_data, last_dst_data, x_min );
				count.l += ystep.l;
				src_data += src_line;
			}
		} else {
			ystep.l = STEP( src_h / abs_h);
			for ( i = 0; i < abs_h; i++) {
				if ( count.i.i > last) {
					src_data += src_line;
					last = count.i.i;
				}
				count.l += ystep.l;
				memcpy( dst_data, src_data, x_min);
				dst_data  += dst_line;
			}
		}
		return true;
	}

/* general actions for x-scaling */
	count.l = 0;
	if ( src_w < abs_w || src_h < abs_h || ( type & imBPP) == imNibble)
		memset( dst_data, 0, dst_line * abs_h);
	if ( abs_w < src_w)
		xstep. l = STEP( abs_w / src_w);
	else
		xstep. l = STEP( src_w / abs_w);

/* no vertical stretch case */
	if ( !y_stretch || ( src_h == -h)) {
		if ( h < 0) {
			dst_data += dst_line * ( y_min - 1);
			dst_line = -dst_line;
		}
		for ( i = 0; i < y_min; i++, src_data += src_line, dst_data += dst_line)
			xproc( src_data, dst_data, src_w, w, abs_w, xstep.l);
		return true;
	}

/* general case */
	if ( yproc ) {
		semistatic_init(&pimpl_buf, &impl_buf, 1, 1024);
		if ( !semistatic_expand(&pimpl_buf, dst_line)) {
			strlcpy(error, "Not enough memory", 255);
			return false;
		}
	}

	if ( h < 0) {
		dst_data += dst_line * ( abs_h - 1);
		dst_line = -dst_line;
	}
	if ( abs_h < src_h) {
		int j = 0;
		Byte * last_dst_data;
		ystep.l = STEP( abs_h / src_h);
		xproc( src_data, last_dst_data = dst_data, src_w, w, abs_w, xstep.l);
		dst_data += dst_line;
		j++;
		for ( i = 0; i < src_h; i++) {
			if ( count.i.i > last) {
				xproc( src_data, last_dst_data = dst_data, src_w, w, abs_w, xstep.l);
				dst_data += dst_line;
				last = count.i.i;
				j++;
			} else if ( yproc && i > 0 ) {
				bzero(pimpl_buf.heap, dst_line);
				xproc( src_data , pimpl_buf.heap, src_w, w, abs_w, xstep.l);
				yproc( pimpl_buf.heap, last_dst_data, dst_line);
			}
			count.l += ystep.l;
			src_data += src_line;
		}
	} else {
		ystep.l = STEP( src_h / abs_h);
		for ( i = 0; i < abs_h; i++) {
			if ( count.i.i > last) {
				src_data += src_line;
				last = count.i.i;
			}
			count.l += ystep.l;
			if ( src_last == src_data) {
				memcpy( dst_data, dst_data - dst_line, dst_line < 0 ? -dst_line : dst_line);
			} else {
				xproc( src_data, dst_data, src_w, w, abs_w, xstep.l);
				src_last = src_data;
			}
			dst_data += dst_line;
		}
	}

	if ( yproc )
		semistatic_done(&pimpl_buf);

	return true;
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
stretch_filtered( int type, Byte * old_data, int old_w, int old_h, Byte * new_data, int w, int h, int scaling, char * error )
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
		strlcpy( error, "no appropriate scaling filter found", 255);
		return false;
	}
	if ( w <= 0 || h <= 0) {
		strlcpy(error, "image dimensions must be positive", 255);
		return false;
	}

	switch (type) {
	case imMono:
	case imBW:
	case imNibble:
	case im256:
	case imNibble | imGrayScale:
		strlcpy(error, "type not supported", 255);
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
		old_w *= 3;
	}
	if ( channels == 2 ) {
		w *= 2;
		old_w *= 2;
		type = (( type & imBPP ) / 2) | imGrayScale | imRealNumber;
	}

	/* allocate space for semi-filtered and target data */
	factor_x = (double) w / (double) old_w;
	factor_y = (double) h / (double) old_h;
	if (factor_x > factor_y) {
		fw = w;
		fh = old_h;
	} else {
		fw = old_w;
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
		old_data, old_w / channels, old_h, filter_data, fw / channels, fh, factor_x, support_size)
#define VERTICAL(type)   stretch_vertical_##type  ( \
		filter, scale_y, contributions, support_y, \
		filter_data, fw, fh, new_data, w, h, factor_y, support_size )
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
		old_data, old_w, old_h, filter_data, fw, fh, factor_y, support_size)
#define HORIZONTAL(type) stretch_horizontal_##type( \
		filter, scale_x, contributions, support_x, channels, \
		filter_data, fw / channels, fh, new_data, w / channels, h, factor_x, support_size)
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
ic_stretch_filtered( int type, Byte * old_data, int old_w, int old_h, Byte * new_data, int w, int h, int scaling, char * error )
{
	int abs_w, abs_h;
	Bool mirror_x, mirror_y;

	abs_w = abs(w);
	abs_h = abs(h);
	mirror_x = w < 0;
	mirror_y = h < 0;

	/* if it's cheaper to mirror before the conversion, do it */
	if ( mirror_y && old_h < abs_h ) {
		img_mirror_raw( type, old_w, old_h, old_data, 1 );
		mirror_y = 0;
	}
	if ( mirror_x && old_w < abs_w) {
		img_mirror_raw( type, old_w, old_h, old_data, 0 );
		mirror_x = 0;
	}

	if ( !stretch_filtered( type, old_data, old_w, old_h, new_data, w, h, scaling, error ))
		return false;

	if ( mirror_x ) img_mirror_raw( type, w, h, new_data, 0 );
	if ( mirror_y ) img_mirror_raw( type, w, h, new_data, 1 );

	return true;
}

int
ic_stretch_suggest_type( int type, int scaling )
{
	if ( scaling <= istOR )
		return type;

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
ic_stretch( int type, Byte * src_data, int src_w, int src_h, Byte * dst_data, int w, int h, int scaling, char * error)
{
	return ( scaling <= istOR ) ?
		ic_stretch_box( type, src_data, src_w, src_h, dst_data, w, h, scaling, error):
		ic_stretch_filtered( type, src_data, src_w, src_h, dst_data, w, h, scaling, error );
}

#ifdef __cplusplus
}
#endif
