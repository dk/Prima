#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
rotate270( PImage i, Byte * new_data, int new_line_size)
{
	int y;
	int w	= i->w;
	int pixel_size	  = (i-> type & imBPP) / 8;
	int tail = i-> lineSize - w * pixel_size  ;
	Byte * src = i->data, * dst0;

	switch (i->type & imBPP) {
	case 8:
		dst0 = new_data + i->w * new_line_size;
		for ( y = 0; y < i-> h; y++) {
			register int x = w;
			register Byte * dst = dst0++;
			while (x--)
				*(dst -= new_line_size) = *src++;
			src += tail;
		}
		return;

	default:
		dst0 = new_data + ( i-> w - 1) * new_line_size;
		new_line_size += pixel_size;
		for ( y = 0; y < i-> h; y++) {
			register int x = w;
			register Byte * dst = dst0;
			while (x--) {
				register int b = pixel_size;
				while ( b--)
					*dst++ = *src++;
				dst -= new_line_size;
			}
			src += tail;
			dst0 += pixel_size;
		}
		return ;
	}
}

static void
rotate180( PImage i, Byte * new_data)
{
	int y, bs2;
	int w	       = i->w;
	int pixel_size = (i-> type & imBPP) / 8;
	int tail       = i-> lineSize - w * pixel_size	;
	Byte * src = i->data, *dst = new_data + i-> h * i-> lineSize - tail - pixel_size;

	switch (i->type & imBPP) {
	case 8:
		for ( y = 0; y < i-> h; y++) {
			register int x = w;
			while (x--)
				*dst-- = *src++;
			src += tail;
			dst -= tail;
		}
		return;

	default:
		bs2 = pixel_size + pixel_size;
		for ( y = 0; y < i-> h; y++) {
			register int x = w;
			while (x--) {
				register int b = pixel_size;
				while ( b--)
					*dst++ = *src++;
				dst -= bs2;
			}
			src += tail;
			dst -= tail;
		}
		return ;
	}
}

static void
rotate90( PImage i, Byte * new_data, int new_line_size)
{
	int y;
	int w	= i->w;
	int pixel_size	  = (i-> type & imBPP) / 8;
	int tail = i-> lineSize - w * pixel_size;
	Byte * src = i->data, *dst0;

	switch (i->type & imBPP) {
	case 8:
		dst0 = new_data + i->h - new_line_size - 1;
		for ( y = 0; y < i-> h; y++) {
			register int x = w;
			register Byte * dst = dst0--;
			while (x--)
				*(dst += new_line_size) = *(src++);
			src += tail;
		}
		return;

	default:
		dst0 = new_data + (i->h - 1) * pixel_size;
		new_line_size -= pixel_size;
		for ( y = 0; y < i-> h; y++) {
			register int x = w;
			register Byte * dst = dst0;
			while (x--) {
				register int b = pixel_size;
				while ( b--)
					*(dst++) = *(src++);
				dst += new_line_size;
			}
			src += tail;
			dst0 -= pixel_size;
		}
		return ;
	}
}

void
img_integral_rotate( Handle self, Byte * new_data, int new_line_size, int degrees)
{
	PImage i = ( PImage ) self;

	if (( i-> type & imBPP) < 8 )
		croak("Not implemented");

	switch ( degrees ) {
	case 90:
		rotate90(i, new_data, new_line_size);
		break;
	case 180:
		rotate180(i, new_data);
		break;
	case 270:
		rotate270(i, new_data, new_line_size);
		break;
	}
}

Bool
img_mirror_raw( int type, int w, int h, Byte * data, Bool vertically)
{
	int y;
	int ls = LINE_SIZE(w, type);
	register Byte swap;

	if ( vertically ) {
		Byte * src = data, *dst = data + ( h - 1 ) * ls, *p, *q;
		h /= 2;
		for ( y = 0; y < h; y++, src += ls, dst -= ls ) {
			register int t = ls;
			p = src;
			q = dst;
			while ( t-- ) {
				swap = *q;
				*(q++) = *p;
				*(p++) = swap;
			}
		}
	} else {
		int x, pixel_size = (type & imBPP) / 8, last_pixel = (w - 1) * pixel_size, w2 = w / 2;
		switch (type & imBPP) {
		case 1:
		case 4:
			 return false;
		case 8:
			for ( y = 0; y < h; y++, data += ls ) {
				Byte *p = data, *q = data + last_pixel;
				register int t = w2;
				while ( t-- ) {
					swap = *q;
					*(q--) = *p;
					*(p++) = swap;
				}
			}
			break;
		default:
			for ( y = 0; y < h; y++, data += ls ) {
				Byte *p = data, *q = data + last_pixel;
				for ( x = 0; x < w2; x++, q -= pixel_size * 2) {
					register int t = pixel_size;
					while (t--) {
						swap = *q;
						*(q++) = *p;
						*(p++) = swap;
					}
				}
			}
		}
	}
	return true;
}

void
img_mirror( Handle self, Bool vertically)
{
	PImage i = ( PImage ) self;
	if ( !img_mirror_raw( i->type, i->w, i->h, i->data, vertically))
		croak("not implemented");
}

#define PI 3.14159265358979323846264338327950288419716939937510
#define RAD (180.0 / PI)

/* Transform 4 corners to next shearing, then calculate integral pixels it occupies.
This is needed to calculate the least enclosed rect for a sheared (and, ultimately, rotated) image */
static void
apply_shear( Point * points, double func, int w, int h, int index, Point * out_min, Point * out_dim)
{
	int i, min_i;
	Point * p, max_p;
	double max_shift, min, max, center, tmp[4];
	
	max_shift = (func >= 0) ? 0 : func * ((index ? w : h) - 1);

	/* apply shearing */
	max = 0;
	for ( i = 0, p = points; i < 4; i++, p++) {
		double n = index ?
			p->y + p->x * func :
			p->x + p->y * func;
		n -= max_shift;
		tmp[i] = n;
		if ( i == 0 ) {
			min = max = n;
		} else {
			if ( min > n ) min = n;
			if ( max < n ) max = n;
		}
	}
	center = (max + min) / 2;

	/* convert to integers */
	min_i = 0;
	for ( i = 0, p = points; i < 4; i++, p++) {
		int n = (tmp[i] > center) ? (int)ceil(tmp[i]) : (int)(floor(tmp[i]));
		if ( index )
			p->y = n;
		else
			p->x = n;
		if (i == 0 || min_i > n)
			min_i = n;
	}

	/* calculate dimensions */
	*out_min = max_p = points[0];
	for ( i = 1, p = points + 1; i < 4; i++, p++) {
		if ( out_min->x > p->x ) out_min->x = p->x;
		if ( out_min->y > p->y ) out_min->y = p->y;
		if ( max_p.x < p-> x) max_p.x = p->x;
		if ( max_p.y < p-> y) max_p.y = p->y;
	}
	out_dim->x = max_p.x - out_min->x + 1;
	out_dim->y = max_p.y - out_min->y + 1;

	/* exclude empty scanlines */
	for ( i = 0, p = points; i < 4; i++, p++) {
		if ( index )
			p->y -= min_i;
		else
			p->x -= min_i;
	}
}

static Bool
create_tmp_image( PImage template, int channels, PImage target, Point size)
{
	img_fill_dummy( target, size.x, size.y, template->type, NULL, template->palette);

	if (!(target->data = malloc( target->dataSize))) {
		warn("not enough memory: %d bytes", target->dataSize);
		return false;
	}
	bzero( target->data, target->dataSize );

	switch ( channels ) {
	case 2:
		target->type &= ~(imComplexNumber | imTrigComplexNumber);
		break;
	case 3:
		target->type = imByte;
		break;
	}
	target->w *= channels;

	return true;
}

#define SHEAR_X_FUNCTION(type,pixel_interim_type) \
static void							  \
shear_x_scanline_ ## type(					  \
	void * _src, int channels, int src_w,			  \
	void * _dst, int dst_w,					  \
	int delta, double sf, Bool reverse			  \
) {								  \
	int x, c, nx, dsrc;					  \
	double leftover[3];					  \
	type *src = (type*)_src, *dst = (type*) _dst;		  \
	pixel_interim_type new_pixel;						\
								  \
	if ( reverse ) {					  \
		dsrc = -1;					  \
		src  += (src_w - 1) * channels + channels - 1;	  \
	} else							  \
		dsrc = 1;					  \
								  \
	for ( c = 0; c < channels; c++)				  \
		leftover[c] = 0;				  \
								  \
	dst += delta * channels;				  \
	for (							  \
		x = 0, nx = delta;				  \
		x < src_w;					  \
		x++, nx++					  \
	) {							  \
		for ( c = 0; c < channels; c++, src += dsrc) {	  \
			double to_transfer = ((double)*src) * sf;

#define SHEAR_X_LOOP \
			double to_leave    = *src - new_pixel + leftover[c]; \
			if ( nx >= dst_w ) return;

#define CLAMP(minval,maxval) \
			if ( new_pixel < minval ) new_pixel = minval; \
			if ( new_pixel > maxval ) new_pixel = maxval;

#define SHEAR_X_LOOP_END \
			if ( nx >= 0)		      \
				*(dst++) = new_pixel; \
			else			      \
				dst++;		      \
			leftover[c] = to_leave;       \
		}				      \
	}
#define SHEAR_X_TAIL \
	if ( nx >= 0 && nx < dst_w) \
		for ( c = 0; c < channels; c++) {

#define SHEAR_X_FUNCTION_END \
			*(dst++) = new_pixel; \
		}			      \
}

SHEAR_X_FUNCTION(Byte,short int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_X_LOOP
	CLAMP(0,255)
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + 0.5;
	CLAMP(0,255)
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(Short,int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_X_LOOP
	CLAMP(INT16_MIN,INT16_MAX)
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + 0.5;
	CLAMP(INT16_MIN,INT16_MAX)
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(Long,int64_t)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_X_LOOP
	CLAMP(INT32_MIN,INT32_MAX)
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + 0.5;
	CLAMP(INT32_MIN,INT32_MAX)
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(float,float)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_X_LOOP
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c];
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(double,double)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_X_LOOP
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c];
SHEAR_X_FUNCTION_END

#define SHEAR_Y_FUNCTION(type,pixel_interim_type) \
static void								  \
shear_y_scanline_ ## type(						  \
	void * _src, int channels, int src_w, int src_h, int src_stride,  \
	void * _dst, int dst_w, int dst_h, int dst_stride,		  \
	int delta, double sf						  \
) {									  \
	int y, c, ny;							  \
	double leftover[3];						  \
	type * src = (type*)_src, * dst = ( type* ) _dst;		  \
	pixel_interim_type new_pixel;					  \
									  \
	for ( c = 0; c < channels; c++)					  \
		leftover[c] = 0;					  \
									  \
	dst = (type*) (((Byte*) dst) + delta * dst_stride);		  \
									  \
	for ( y = 0, ny = delta; y < src_h; y++, ny++) {		  \
		for ( c = 0; c < channels; c++) {			  \
			type   pixel	   = src[c];			  \
			double to_transfer = ((double)pixel) * sf;

#define SHEAR_Y_LOOP \
			double to_leave    = pixel - new_pixel + leftover[c];\
			if ( ny >= dst_h) return;

#define SHEAR_Y_LOOP_END(type) \
			if ( ny >= 0 ) dst[c] = new_pixel;		    \
			leftover[c] = to_leave;				    \
		}							    \
		src = (type*)(((Byte*)src) + src_stride);		    \
		dst = (type*)(((Byte*)dst) + dst_stride);		    \
	}

#define SHEAR_Y_TAIL \
	if ( ny >= 0 && ny < dst_h)					   \
		for ( c = 0; c < channels; c++) {

#define SHEAR_Y_FUNCTION_END \
			dst[c] = new_pixel; \
		} \
}
									     
SHEAR_Y_FUNCTION(Byte,short int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_Y_LOOP
	CLAMP(0,255)
	SHEAR_Y_LOOP_END(Byte)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + 0.5;
	CLAMP(0,255)
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(Short,int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_Y_LOOP
	CLAMP(INT16_MIN,INT16_MAX)
	SHEAR_Y_LOOP_END(Short)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + 0.5;
	CLAMP(INT16_MIN,INT16_MAX)
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(Long,int64_t)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_Y_LOOP
	CLAMP(INT32_MIN,INT32_MAX)
	SHEAR_Y_LOOP_END(Long)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + 0.5;
	CLAMP(INT32_MIN,INT32_MAX)
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(float,float)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_Y_LOOP
	SHEAR_Y_LOOP_END(float)
	SHEAR_Y_TAIL
	new_pixel = leftover[c];
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(double,double)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_Y_LOOP
	SHEAR_Y_LOOP_END(double)
	SHEAR_Y_TAIL
	new_pixel = leftover[c];
SHEAR_Y_FUNCTION_END

typedef void ShearXFunc(
	void * src, int channels, int src_w,
	void * dst, int dst_w,
	int delta, double sf, Bool reverse);

typedef void ShearYFunc(
	void * _src, int channels, int src_w, int src_h, int src_stride,
	void * _dst, int dst_w, int dst_h, int dst_stride,
	int delta, double sf
);

#define FIND_SHEAR_FUNC(letter) \
	case imByte   : shear_func = shear_## letter ##_scanline_Byte;	 break;\
	case imShort  : shear_func = shear_## letter ##_scanline_Short;  break;\
	case imLong   : shear_func = shear_## letter ##_scanline_Long;	 break;\
	case imFloat  : shear_func = shear_## letter ##_scanline_float;  break;\
	case imDouble : shear_func = shear_## letter ##_scanline_double; break;

static void
shear_x( PImage src, int channels, PImage dst, double func, FilterFunc filter, int dx, Bool apply_180)
{
	int w, dw, h, y, lim_y, dsrc, ddst;
	Byte * src_scanline, * dst_scanline;
	ShearXFunc * shear_func;

	w  = src->w / channels;
	dw = dst->w / channels;
	h  = src->h;

	if ( apply_180 ) {
		src_scanline = src->data + src->lineSize * ( h - 1 );
		dsrc = -src->lineSize;
	} else {
		src_scanline = src->data;
		dsrc = src->lineSize;
	}
	dst_scanline = dst->data;
	ddst = dst->lineSize;

	switch ( src->type ) {
	FIND_SHEAR_FUNC(x)
	default:
		croak("panic: wrong type to rotate:%x", src->type);
	}

	lim_y = ( h < dst->h ) ? h : dst->h;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( y = 0; y < lim_y; y++) {
		double sk = ( func > 0 ) ? func * y : (-func) * (h - y - 1);
		int    si = (int) floor(sk);
		shear_func(
			src_scanline + dsrc * y, channels, w, 
			dst_scanline + ddst * y, dw, 
			si + dx, filter(sk - si), apply_180
		);
	}
}

static void
shear_y( PImage src, int channels, PImage dst, double func, FilterFunc filter, int dy)
{
	int x, w, h, dw, lim_x, dscanline;
	Byte * src_scanline, * dst_scanline;
	ShearYFunc * shear_func;
	
	w  = src->w / channels;
	dw = dst->w / channels;
	h  = src->h;

	src_scanline = src->data;
	dst_scanline = dst->data;
	dscanline = channels * (src->type & imBPP) / 8;

	switch ( src->type ) {
	FIND_SHEAR_FUNC(y)
	default:
		croak("panic: wrong type to rotate:%x", src->type);
	}

	lim_x = ( w < dst-> w ) ? w : dst->w;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( x = 0; x < lim_x; x++) {
		double sk = ( func > 0 ) ? func * x : (-func) * (w - x - 1);
		int    si = (int) floor(sk);
		shear_func(
			src_scanline + dscanline * x, channels, w, h, src->lineSize,
			dst_scanline + dscanline * x, dw, dst->h, dst->lineSize,
			si + dy, filter(sk - si));
	}
}

static double default_filter(const double x) { return 1.0 - x; }

/* Fast rotation by Paeth algortihm. Accepts grayscale images with bpp >= 8, and 24 bpp RGBs */
Bool 
img_generic_rotate( Handle self, double degrees, PImage dummy)
{
	Bool apply_180 = false;
	Image *i = (PImage)self, s0, s1, s2;
	double sin1, tan2;
	Point p[4], s1dim, s2dim, s3dim, s1min, s2min, s3min;
	int n, channels;
	FilterFunc *filter = default_filter;

	if ( i-> scaling > istTriangle ) {
		Bool found = 0;
		for ( n = 0; ; n++) {
			if ( ist_filters[n]. id == 0 ) break;
			if ( ist_filters[n]. id == i->scaling ) {
				filter = ist_filters[n].filter;
				found = true;
				break;
			}
		}
		if ( !found )
			warn( "no appropriate scaling filter found");
	}

	if ( i->type == imRGB) 
		channels = 3;
	else if ( i->type & (imComplexNumber | imTrigComplexNumber) )
		channels = 2;
	else
		channels = 1;

	if ( degrees < 270 && degrees > 90 ) {
		degrees -= 180;
		apply_180 = true;
	}

	degrees /= RAD;
	sin1 = sin(degrees);
	tan2 = -tan(degrees/2);

	bzero(&p, sizeof(p));
	p[1].x = p[2].x = i->w - 1;
	p[2].y = p[3].y = i->h - 1;
	
	apply_shear(p, tan2, i->w, i-> h, 0, &s1min, &s1dim);
	apply_shear(p, sin1, s1dim.x, s1dim.y, 1, &s2min, &s2dim);
	apply_shear(p, tan2, s2dim.x, s2dim.y, 0, &s3min, &s3dim);

	if ( !create_tmp_image(i, channels, &s1, s1dim))
		return false;
	img_fill_dummy( &s0, i->w * channels, i->h, s1.type, i->data, i->palette);
	shear_x(&s0, channels, &s1, tan2, filter, 0, apply_180);
	if ( !create_tmp_image(i, channels, &s2, s2dim)) {
		free(s1.data);
		return false;
	}
	shear_y(&s1, channels, &s2, sin1, filter, -s2min.y);
	free(s1.data);

	s3dim.x++; /* double shearing by x can result in 2 extra pixels, not just 1 */
	if ( !create_tmp_image(i, channels, dummy, s3dim)) {
		free(s1.data);
		return false;
	}
	shear_x(&s2, channels, dummy, tan2, filter, -s3min.x,false);
	free(s2.data);

	dummy-> w /= channels;
	dummy-> type = i->type;

	return true;
}

#ifdef __cplusplus
}
#endif
 
