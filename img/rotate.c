#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
rotate270( PImage i, Byte * new_data, int new_line_size)
{
	int y;
	int w = i->w;
	int pixel_size = (i-> type & imBPP) / 8;
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
	int w          = i->w;
	int pixel_size = (i-> type & imBPP) / 8;
	int tail       = i-> lineSize - w * pixel_size        ;
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
	int w = i->w;
	int pixel_size = (i-> type & imBPP) / 8;
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

static void
fill_dimensions( Point * points, Point min_p, Point * out_min, Point * out_dim)
{
	int i;
	Point max_p, *p;

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
		p->x -= min_p.x;
		p->y -= min_p.y;
	}
}

static void
point_swap( Point *a, Point *b)
{
	Point p0 = *a;
	*a = *b;
	*b = p0;
}

static Bool transform_fail(void) {
	warn("Image.rotate/transform: transformation results in invalid image");
	return false;
}

/* Transform 4 corners to next shearing, then calculate integral pixels it occupies.
This is needed to calculate the least enclosed rect for a sheared (and, ultimately, rotated) image */
static Bool
apply_shear( Point * points, float func_mul, float func_add, int w, int h, int index, Point * out_min, Point * out_dim, Point *aperture)
{
	int i, min_i;
	Point * p, min_p, p0 = points[0];
	float max_shift, min = 0, max = 0, center, tmp[4];

	max_shift = (func_mul >= 0) ? 0 : func_mul * ((index ? w : h) - 1);

	/* apply shearing */
	max = 0;
	for ( i = 0, p = points; i < 4; i++, p++) {
		float n = index ?
			p->y + p->x * func_mul :
			p->x + p->y * func_mul;
		n += func_add - max_shift;
		if ( n <= -16383 || n >= 16384 )
			return transform_fail();
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
		int n = (tmp[i] > center) ? (int)ceilf(tmp[i]) : (int)(floorf(tmp[i]));
		if ( index )
			p->y = n;
		else
			p->x = n;
		if (i == 0 || min_i > n)
			min_i = n;
	}

	if ( index ) {
		min_p.x = 0;
		min_p.y = min_i;
	} else {
		min_p.x = min_i;
		min_p.y = 0;
	}

	if ( aperture != NULL ) {
		int n0 = (int)(floorf(tmp[0])) - min_i;
		if (index)
			aperture-> y -= n0 - p0.y;
		else
			aperture-> x -= n0 - p0.x;
	}

	fill_dimensions( points, min_p, out_min, out_dim);
	return true;
}

/* Transform 4 corners to scaling shearing, then calculate integral pixels it occupies. */
static Bool
apply_scale( Point * points, float sx, float sy, Point * out_min, Point * out_dim, Point *aperture)
{
	int i;
	Point * p, min_p, p0 = points[0];
	NPoint min, max, center, tmp[4];

	/* apply shearing */
	max.x = max.y = min.x = min.y = 0.0;
	for ( i = 0, p = points; i < 4; i++, p++) {
		tmp[i].x = p->x * sx;
		tmp[i].y = p->y * sy;
		if ( i == 0 ) {
			min = max = tmp[0];
		} else {
			if ( min.x > tmp[i].x ) min.x = tmp[i].x;
			if ( min.y > tmp[i].y ) min.y = tmp[i].y;
			if ( max.x < tmp[i].x ) max.x = tmp[i].x;
			if ( max.x < tmp[i].x ) max.y = tmp[i].y;
		}
	}
	center.x = (max.x + min.x) / 2;
	center.y = (max.y + min.y) / 2;

	/* convert to integers */
	sx = fabs(sx);
	sy = fabs(sy);
	min_p.x = min_p.y = 0;
	for ( i = 0, p = points; i < 4; i++, p++) {
		p->x = (tmp[i].x > center.x) ? (int)ceilf(tmp[i].x + sx) - 1 : (int)(floorf(tmp[i].x));
		p->y = (tmp[i].y > center.y) ? (int)ceilf(tmp[i].y + sy) - 1 : (int)(floorf(tmp[i].y));
		if ( p->x <= -16383 || p->x >= 16384 || p->y <= -16383 || p->y >= 16384 )
			return transform_fail();
		if (i == 0 || min_p.x > p->x)
			min_p.x = p->x;
		if (i == 0 || min_p.y > p->y)
			min_p.y = p->y;
	}

	if ( sx < 0 ) {
		point_swap( points + 0, points + 1 );
		point_swap( points + 2, points + 3 );
	}
	if ( sy < 0 ) {
		point_swap( points + 0, points + 3 );
		point_swap( points + 1, points + 2 );
	}
	if ( aperture != NULL ) {
		aperture->x += p0.x - points[0].x + min_p.x;
		aperture->y += p0.y - points[0].y + min_p.y;
	}

	fill_dimensions( points, min_p, out_min, out_dim);

	return true;
}

static Bool
apply_rotate90( Point * points, int h, Point * out_min, Point * out_dim, Point * aperture)
{
	int i;
	Point * p, min_p = {0,0}, p0 = points[0];
	for ( i = 0, p = points; i < 4; i++, p++) {
		int x = h - p->y;
		p-> y = p-> x;
		p-> x = x;
	}

	aperture->x += p0.x - points[0].x;
	aperture->y += p0.y - points[0].y;
	fill_dimensions( points, min_p, out_min, out_dim);
	memmove( points + 1, points + 0, sizeof(Point) * 3);
	points[3] = p0;
	return true;
}

static void
reduce_image_to_channels( int src_type, int *dst_type, int *dst_channels)
{
	if ( src_type == imRGB)  {
		if ( dst_channels) *dst_channels = 3;
		if ( dst_type)     *dst_type = imByte;
	}
	else if ( src_type & (imComplexNumber | imTrigComplexNumber) ) {
		int bpp   = src_type & imBPP;
		int flags = src_type & ~(imBPP | imComplexNumber | imTrigComplexNumber);
		if ( dst_channels) *dst_channels  = 2;
		if ( dst_type)     *dst_type      = (bpp / 2) | flags | imRealNumber;
	} else {
		if ( dst_channels) *dst_channels = 1;
		if ( dst_type)     *dst_type     = src_type;
	}
}

static Bool
create_tmp_image( PImage template, int channels, PImage target, Point size, ColorPixel fill)
{
	img_fill_dummy( target, size.x, size.y, template->type, NULL, template->palette);

	if (target->dataSize == 0)
		croak("rotate/transform panic: interim image (%d,%d) is NULL", size.x, size.y);
	if (!(target->data = malloc( target->dataSize))) {
		warn("not enough memory: %d bytes", target->dataSize);
		return false;
	}
	bzero( target->data, target->dataSize );

	if ( channels == 1 && ((template->type & imBPP) == 8)) {
		memset( target->data, fill[0], target->dataSize );
	} else {
		Byte *data;
		int y, bpp = (template->type & imBPP) / 8;
		for ( y = 0, data = target->data; y < size.x; y++, data += bpp )
			memcpy( data, fill, bpp);
		for ( y = 1, data = target->data + target->lineSize; y < size.y; y ++, data += target-> lineSize )
			memcpy( data, target->data, target-> lineSize );
	}

	reduce_image_to_channels( template->type, &target->type, NULL);
	target->w *= channels;

	return true;
}

#define SHEAR_X_FUNCTION(type,pixel_interim_type) \
static void                                                       \
shear_x_scanline_ ## type(                                        \
	void * _src, int channels, int src_w,                     \
	void * _dst, int dst_w,                                   \
	int delta, float sf, float * fill, Bool reverse           \
) {                                                               \
	int x, c, nx, dsrc;                                       \
	float leftover[3];                                        \
	type *src = (type*)_src, *dst = (type*) _dst;             \
	pixel_interim_type new_pixel;                             \
	                                                          \
	if ( reverse ) {                                          \
		dsrc = channels * 2;                              \
		src  += (src_w - 1) * channels;                   \
	} else                                                    \
		dsrc = 0;                                         \
	                                                          \
	for ( c = 0; c < channels; c++)                           \
		leftover[c] = fill[c] * (1.0 - sf);               \
	                                                          \
	dst += delta * channels;                                  \
	for (                                                     \
		x = 0, nx = delta;                                \
		x < src_w;                                        \
		x++, nx++, src -= dsrc                            \
	) {                                                       \
		for ( c = 0; c < channels; c++, src++) {          \
			float to_transfer = ((float)*src) * sf;

#define SHEAR_X_LOOP \
			float to_leave    = *src - new_pixel + leftover[c]; \
			if ( nx >= dst_w ) return;

#define CLAMP(minval,maxval) \
			if ( new_pixel < minval ) new_pixel = minval; \
			if ( new_pixel > maxval ) new_pixel = maxval;

#define SHEAR_X_LOOP_END \
			if ( nx >= 0)                 \
				*(dst++) = new_pixel; \
			else                          \
				dst++;                \
			leftover[c] = to_leave;       \
		}                                     \
	}
#define SHEAR_X_TAIL \
	if ( nx >= 0 && nx < dst_w) \
		for ( c = 0; c < channels; c++) {

#define SHEAR_X_FUNCTION_END \
			*(dst++) = new_pixel;  \
		}                              \
}

SHEAR_X_FUNCTION(Byte,short int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_X_LOOP
	CLAMP(0,255)
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + fill[c] * sf + 0.5;
	CLAMP(0,255)
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(Short,int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_X_LOOP
	CLAMP(INT16_MIN,INT16_MAX)
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + fill[c] * sf + 0.5;
	CLAMP(INT16_MIN,INT16_MAX)
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(Long,int64_t)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_X_LOOP
	CLAMP(INT32_MIN,INT32_MAX)
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + fill[c] * sf + 0.5;
	CLAMP(INT32_MIN,INT32_MAX)
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(float,float)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_X_LOOP
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + fill[c] * sf ;
SHEAR_X_FUNCTION_END

SHEAR_X_FUNCTION(double,double)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_X_LOOP
	SHEAR_X_LOOP_END
	SHEAR_X_TAIL
	new_pixel = leftover[c] + fill[c] * sf;
SHEAR_X_FUNCTION_END

#define SHEAR_Y_FUNCTION(type,pixel_interim_type) \
static void                                                               \
shear_y_scanline_ ## type(                                                \
	void * _src, int channels, int src_w, int src_h, int src_stride,  \
	void * _dst, int dst_w, int dst_h, int dst_stride,                \
	int delta, float sf, float * fill                                 \
) {                                                                       \
	int y, c, ny;                                                     \
	float leftover[3];                                                \
	type * src = (type*)_src, * dst = ( type* ) _dst;                 \
	pixel_interim_type new_pixel;                                     \
	                                                                  \
	for ( c = 0; c < channels; c++)                                   \
		leftover[c] = fill[c] * ( 1 - sf );                       \
	                                                                  \
	dst = (type*) (((Byte*) dst) + delta * dst_stride);               \
	                                                                  \
	for ( y = 0, ny = delta; y < src_h; y++, ny++) {                  \
		for ( c = 0; c < channels; c++) {                         \
			type   pixel           = src[c];                  \
			float to_transfer = ((float)pixel) * sf;

#define SHEAR_Y_LOOP \
			float to_leave    = pixel - new_pixel + leftover[c];\
			if ( ny >= dst_h) return;

#define SHEAR_Y_LOOP_END(type) \
			if ( ny >= 0 ) dst[c] = new_pixel;                \
			leftover[c] = to_leave;                           \
		}                                                         \
		src = (type*)(((Byte*)src) + src_stride);                 \
		dst = (type*)(((Byte*)dst) + dst_stride);                 \
	}

#define SHEAR_Y_TAIL \
	if ( ny >= 0 && ny < dst_h)                                       \
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
	new_pixel = leftover[c] + fill[c] * sf + 0.5;
	CLAMP(0,255)
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(Short,int)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_Y_LOOP
	CLAMP(INT16_MIN,INT16_MAX)
	SHEAR_Y_LOOP_END(Short)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + fill[c] * sf + 0.5;
	CLAMP(INT16_MIN,INT16_MAX)
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(Long,int64_t)
	new_pixel = leftover[c] + to_transfer + 0.5;
	SHEAR_Y_LOOP
	CLAMP(INT32_MIN,INT32_MAX)
	SHEAR_Y_LOOP_END(Long)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + fill[c] * sf + 0.5;
	CLAMP(INT32_MIN,INT32_MAX)
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(float,float)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_Y_LOOP
	SHEAR_Y_LOOP_END(float)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + fill[c] * sf ;
SHEAR_Y_FUNCTION_END

SHEAR_Y_FUNCTION(double,double)
	new_pixel = leftover[c] + to_transfer;
	SHEAR_Y_LOOP
	SHEAR_Y_LOOP_END(double)
	SHEAR_Y_TAIL
	new_pixel = leftover[c] + fill[c] * sf;
SHEAR_Y_FUNCTION_END

typedef void ShearXFunc(
	void * src, int channels, int src_w,
	void * dst, int dst_w,
	int delta, float sf, float * fill, Bool reverse);

typedef void ShearYFunc(
	void * _src, int channels, int src_w, int src_h, int src_stride,
	void * _dst, int dst_w, int dst_h, int dst_stride,
	int delta, float sf, float * fill
);

#define FIND_SHEAR_FUNC(letter) \
	case imByte   : shear_func = shear_## letter ##_scanline_Byte;   break;\
	case imShort  : shear_func = shear_## letter ##_scanline_Short;  break;\
	case imLong   : shear_func = shear_## letter ##_scanline_Long;   break;\
	case imFloat  : shear_func = shear_## letter ##_scanline_float;  break;\
	case imDouble : shear_func = shear_## letter ##_scanline_double; break;

static void
shear_x( PImage src, int channels, PImage dst, float func_mul, float func_add, FilterFunc filter, int dx, float* fill, Bool apply_180)
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
		float sk = func_add + (( func_mul > 0 ) ? func_mul * y : (-func_mul) * (h - y - 1));
		int   si = (int) floorf(sk);
		shear_func(
			src_scanline + dsrc * y, channels, w,
			dst_scanline + ddst * y, dw,
			si + dx, filter(sk - si), fill, apply_180
		);
	}
}

static void
shear_y( PImage src, int channels, PImage dst, float func_mul, float func_add, FilterFunc filter, int dy, float *fill)
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
		float sk = func_add + (( func_mul > 0 ) ? func_mul * x : (-func_mul) * (w - x - 1));
		int    si = (int) floorf(sk);
		shear_func(
			src_scanline + dscanline * x, channels, w, h, src->lineSize,
			dst_scanline + dscanline * x, dw, dst->h, dst->lineSize,
			si + dy, filter(sk - si), fill);
	}
}

static double default_filter(const double x) { return 1.0 - x; }

static FilterFunc*
find_filter( int scaling )
{
	int n;
	FilterFunc *filter = default_filter;
	if ( scaling > istTriangle ) {
		Bool found = 0;
		for ( n = 0; ; n++) {
			if ( ist_filters[n]. id == 0 ) break;
			if ( ist_filters[n]. id == scaling ) {
				filter = ist_filters[n].filter;
				found = true;
				break;
			}
		}
		if ( !found )
			warn( "no appropriate scaling filter found");
	}
	return filter;
}

/* convert pixels of all types to floats, as resampling is in the float space */
static void
fix_ffills( int type, int channels, ColorPixel fill, float * ffill )
{
	Byte n, *src = fill, bpp = ( type & imBPP ) / 8;
	for ( n = 0; n < channels; n++) {
		if ( channels == 2 ) {
			switch ( type & imBPP ) {
			case sizeof(float)  * 8:
				ffill[n] = *((float*)(src));
				break;
			case sizeof(double) * 8:
				ffill[n] = *((double*)(src));
				break;
			default:
				croak("panic: cannot convert pixel type %x to float", type);
			}
		} else {
			switch ( type ) {
			case imByte:
				ffill[n] = *((Byte*)(src));
				break;
			case imShort:
				ffill[n] = *((Short*)(src));
				break;
			case imLong:
				ffill[n] = *((Long*)(src));
				break;
			case imFloat:
				ffill[n] = *((float*)(src));
				break;
			case imDouble:
				ffill[n] = *((double*)(src));
				break;
			default:
				croak("panic: cannot convert pixel type %x to float", type);
			}
		}
		src += bpp;
	}
}


/* Fast rotation by Paeth algorithm. Accepts grayscale images with bpp >= 8, and 24 bpp RGBs */
Bool
img_generic_rotate( Handle self, float degrees, PImage output, ColorPixel fill, NPoint delta, Point *aperture)
{
	Bool apply_180 = false;
	Image *i = (PImage)self, s0, s1, s2;
	float sin1, tan2;
	Point p[4], s1dim, s2dim, s3dim, s1min, s2min, s3min;
	int type, channels;
	FilterFunc *filter = find_filter(i->scaling);
	float ffill[3] = {0,0,0};
	Point _aperture;

	while (degrees < 0.0  ) degrees += 360.0;
	while (degrees > 360.0) degrees -= 360.0;

	if ( aperture == NULL ) aperture = &_aperture;
	aperture->x = aperture->y = 0;

	reduce_image_to_channels( i->type, &type, &channels);
	fix_ffills( type, channels, fill, ffill );

	if ( degrees < 270.0 && degrees > 90.0 ) {
		aperture->x -= i->w;
		aperture->y -= i->h;

		degrees -= 180.0;
		apply_180 = true;
	}

	degrees /= (float) RAD;
	sin1 = sin(degrees);
	tan2 = -tan(degrees/2);

	bzero(&p, sizeof(p));
	p[1].x = p[2].x = i->w - 1;
	p[2].y = p[3].y = i->h - 1;
	if ( apply_180 )
		point_swap( &p[0], &p[2]);

	if ( !(
		apply_shear(p, tan2, 0,       i->w,    i-> h,   0, &s1min, &s1dim, aperture) &&
		apply_shear(p, sin1, delta.y, s1dim.x, s1dim.y, 1, &s2min, &s2dim, aperture) &&
		apply_shear(p, tan2, delta.x, s2dim.x, s2dim.y, 0, &s3min, &s3dim, aperture)
	))
		return false;

	if ( !create_tmp_image(i, channels, &s1, s1dim, fill))
		return false;
	img_fill_dummy( &s0, i->w * channels, i->h, s1.type, i->data, i->palette);
	shear_x(&s0, channels, &s1, tan2, 0.0, filter, 0, ffill, apply_180);
	if ( !create_tmp_image(i, channels, &s2, s2dim, fill)) {
		free(s1.data);
		return false;
	}
	shear_y(&s1, channels, &s2, sin1, delta.y, filter, -s2min.y, ffill);
	free(s1.data);

	s3dim.x++; /* double shearing by x can result in 2 extra pixels, not just 1 */
	if ( !create_tmp_image(i, channels, output, s3dim, fill)) {
		free(s1.data);
		return false;
	}
	shear_x(&s2, channels, output, tan2, delta.x, filter, -s3min.x, ffill, false);
	free(s2.data);

	output-> w /= channels;
	output-> type = i->type;

	return true;
}

static Bool
integral_rotate( Handle self, int degrees, PImage output)
{
	PImage i = (PImage) self;
	int w, h;
	if ( degrees != 180 ) {
		w = i->h;
		h = i->w;
	} else {
		w = i->w;
		h = i->h;
	}

	img_fill_dummy( output, w, h, i->type, NULL, i->palette);
	if (!(output->data = malloc( output->dataSize))) {
		warn("not enough memory: %d bytes", output->dataSize);
		return false;
	}
	img_integral_rotate( self, output->data, output->lineSize, degrees);
	return true;
}

static Bool
scale( Handle self, double mx, double my, PImage output)
{
	PImage i = (PImage) self;
	NPoint sz   = { mx * i->w, my * i->h};
	char errbuf[256];
	int w, h;
 	w = (sz.x < 0) ? (sz.x - 0.5) : (sz.x + 0.5);
	h = (sz.y < 0) ? (sz.y - 0.5) : (sz.y + 0.5);

	img_fill_dummy( output, abs(w), abs(h), i->type, NULL, i->palette);
	if (!(output->data = malloc( output->dataSize))) {
		warn("not enough memory: %d bytes", output->dataSize);
		return false;
	}

	if ( !ic_stretch( i->type,
		i->data, i->w, i->h,
		output->data, w, h,
		(i->scaling < istTriangle) ? istBox : i->scaling,
		errbuf
	)) {
		free(output->data);
		warn("%s", errbuf);
		return false;
	}

	return true;
}

/* max image size is 16K, so best precision we need is 1/32 K */
static void
roundoff(double *m, int count)
{
	while ( count-- ) {
		*m = floorf((*m) * 32768.0 + 0.5) / 32768.0;
		m++;
	}
}

/* very special case for rotation and scaling */
static int
check_rotated_case( Handle self, Matrix matrix, PImage output, ColorPixel fill, Point *aperture)
{
	Image *i, src;
	Bool ok;
	double mx = 1.0, my = 1.0, angle = 0.0;
	int fixed_angle = -1;

	if ( matrix[4] != 0.0 || matrix[5] != 0.0 )
		return -1;

	if ( matrix[0] == 0.0 && matrix[3] == 0.0 ) {
		if ( matrix[1] < 0 && matrix[2] > 0 ) {
			fixed_angle = 270;
			mx = -matrix[1];
			my = matrix[2];
		} else {
			fixed_angle = 90;
			mx = matrix[1];
			my = -matrix[2];
		}
	} else if ( matrix[1] == 0.0 && matrix[2] == 0.0 ) {
		if ( matrix[0] < 0 && matrix[3] < 0 ) {
			mx = -matrix[0];
			my = -matrix[3];
			fixed_angle = 180;
		} else {
			mx = matrix[0];
			my = matrix[3];
			fixed_angle = 0;
		}
	}

	if ( fixed_angle < 0 ) {
		double angles[2], mcos, msin, angle1, angle2, m[4];
		angles[0] = RAD * (angle1 = atan2(matrix[1], matrix[0]));
		angles[1] = RAD * (angle2 = atan2(-matrix[2], matrix[3]));
		roundoff(angles,2);
		if ( fabs(angles[0] - angles[1]) > 0.001 )
			return -1;

		mcos = cos(angle1);
		msin = sin(angle1);
		m[0] = ( mcos != 0.0 ) ?  matrix[0] / mcos : 1.0;
		m[1] = ( msin != 0.0 ) ?  matrix[1] / msin : 1.0;
		m[2] = ( msin != 0.0 ) ? -matrix[2] / msin : 1.0;
		m[3] = ( mcos != 0.0 ) ?  matrix[3] / mcos : 1.0;
		roundoff(m, 4);
		if ( m[0] != m[1] || m[2] != m[3])
			return -1;

		mx = m[0];
		my = m[2];
		angle = angle1;
	}

	i = (PImage) self;
	img_fill_dummy( &src, i->w, i->h, i->type, i->data, i->palette);

	if ( mx != 1.0 || my != 1.0 ) {
		Image tmp;
		if ( !scale(( Handle) &src, mx, my, &tmp))
			return false;
		if ( mx > 0 )
			aperture-> x *= mx;
		else
			aperture-> x -= tmp.w;
		if ( my > 0 )
			aperture-> y *= my;
		else
			aperture-> y -= tmp.h;
		if ( fixed_angle == 0 ) {
			*output = tmp;
			return true;
		}
		src = tmp;
	}

	switch (fixed_angle) {
	case 0:
		img_fill_dummy( output, 0, 0, 0, NULL, NULL);
		return true;
	case 90:
		aperture-> x -= src.h;
		if ( mx < 0 ) {
			aperture-> y -= src.w;
			aperture-> x += src.w;
		}
		if ( my < 0 ) {
			aperture-> y += src.h;
			aperture-> x += src.h;
		}
		ok = integral_rotate((Handle) &src, 90, output);
		break;
	case 180:
		aperture-> x -= src.w;
		aperture-> y -= src.h;
		ok = integral_rotate((Handle) &src, 180, output);
		break;
	case 270:
		aperture-> y -= src.w;
		ok = integral_rotate((Handle) &src, 270, output);
		break;
	default: {
		NPoint delta;
		Point extra_aperture;

		delta.x = matrix[4];
		delta.y = matrix[5];
		ok = img_generic_rotate((Handle) &src, angle * RAD, output, fill, delta, &extra_aperture);
		aperture->x += extra_aperture.x;
		aperture->y += extra_aperture.y;
	}}

	if ( src.data != i->data)
		free(src.data);

	return ok;
}

// #define DEBUG 1

#if DEBUG
static char *pipeline_opnames[] = { "shear x", "shear y", "scale", "rotate 90" };
#endif

#define MAX_STEPS 5 /* see below */

typedef struct {
	int cmd;
	float p1, p2;
} ImgOp;

typedef struct {
	int n_steps;
	ImgOp steps[MAX_STEPS];
} ImgOpPipeline;

#define STEP_SHEAR_X     0
#define STEP_SHEAR_Y     1
#define STEP_SCALE       2
#define STEP_ROTATE_90   3

static void
add_op( ImgOpPipeline * p, int cmd, float p1, float p2)
{
	ImgOp * io = p->steps + p->n_steps;
	if ( p->n_steps > MAX_STEPS ) croak("panic: ImgOp pipeline overflow");
	if ( p1 == -0.0 ) p1 = 0.0;
	if ( p2 == -0.0 ) p2 = 0.0;
	io->cmd = cmd;
	io->p1  = p1;
	io->p2  = p2;
	p->n_steps++;
}

/*

Based on block LDU decomposition [1]

|A B| = |1   0| * |A      0| * |1 B/A|
|C D|   |C/A 1|   |0 D-CB/A|   |0   1|

a matrix can be split into a product of lower, diagonal, and upper matrices.
Except, naturally, cases where A=0, but these cases can be decomposed to
UDL:

|A B| = |1 B/D| * |A-BC/D 0| * |1   0|
|C D|   |0   1|   |0      D|   |C/D 1|

Those with both A and D = 0 can be rotated 90 by
multiplying to

|0 -1|
|1  0|

and sent back to the LDU/UDL, while cases with B and/or C = 0 are just a
degenerate scalings, resulting in images with X=0 and/or Y=0.

L and U steps are shearings (that can be implemented very effectively), but
they have their own corner cases where either A or D are close to 0 and thus
interim images can be huge. Even where such images can be created, the
accumulated errors make the resulting image look bad. Therefore select_ldu()
below makes a guess, what if we apply a rotate90 transformation first, and see
if these huge shearings disappear. So the ldu/select_ldu chain can produce the
following op-lists:

0) scale
1) shear x, scale, shear y
2) shear y, scale, shear x
3) shear x, scale, shear y, rotate90
4) shear y, scale, shear x, rotate90

(MAX_STEPS is thus 4 plus one extra shear for dx/dy, if any)

where it is up to the consumer to skip individual steps if these are no-ops.

That is of course a valid question whether #3 and #4 are generally more optimal
to implement that way rather than just using a direct pixel resampling with
direct 2D transformation, but that's yet to be measured.

[1] https://en.wikipedia.org/wiki/Block_LU_decomposition

*/
static void
ldu( Matrix matrix, ImgOpPipeline *iop)
{
	Matrix local_matrix;

	memset( iop, 0, sizeof(ImgOpPipeline));
	if ( matrix[0] == 0.0 ) {
		if (matrix[3] == 0.0) {
			if ( matrix[2] != 0.0 && matrix[1] != 0.0 ) {
				/* scaling and then 90-rotation (0,-1,1,0) */
				local_matrix[0] =  matrix[1];
				local_matrix[1] = -matrix[0];
				local_matrix[2] =  matrix[3];
				local_matrix[3] = -matrix[2];
				matrix = local_matrix;
				goto UDL;
			} else {
				/* degenerate scaling */
				add_op(iop, STEP_SCALE, matrix[3], matrix[2]);
			}
		} else {
		UDL:
			add_op( iop, STEP_SHEAR_Y,   matrix[1] / matrix[3],                         0.0);
			add_op( iop, STEP_SCALE,     matrix[0] - matrix[1] * matrix[2] / matrix[3], matrix[3]);
			add_op( iop, STEP_SHEAR_X,   matrix[2] / matrix[3],                         0.0);
			add_op( iop, STEP_ROTATE_90, 0.0,                                           0.0);
		}
	} else {
		add_op( iop, STEP_SHEAR_X,   matrix[2] / matrix[0], 0.0);
		add_op( iop, STEP_SCALE,     matrix[0],             matrix[3] - matrix[1] * matrix[2] / matrix[0]);
		add_op( iop, STEP_SHEAR_Y,   matrix[1] / matrix[0], 0.0);
	}
}

#if DEBUG
static void
debug_pipeline( ImgOpPipeline * iop)
{
	int i;
	ImgOp *io = iop->steps;
	for ( i = 0; i < iop->n_steps; i++, io++) {
		printf("%d: %s (%g %g)\n", i, pipeline_opnames[io->cmd], io->p1, io->p2);
	}
}
#endif

/* check two ldu variants, with and without prerequisite rotation90.
Select the one that has less scale/shear factor to avoid distortions and interim image being too big */
static void
select_ldu( Matrix matrix, ImgOpPipeline *iop)
{
	int select_first = true;
	ImgOpPipeline p1, p2;

	ldu(matrix, &p1);
#if DEBUG
	printf("LDU1:\n");
	debug_pipeline(&p1);
#endif
	if ( p1.steps[p1.n_steps - 1].cmd != STEP_ROTATE_90 ) {
		Matrix m2 = { matrix[1], -matrix[0], matrix[3], -matrix[2] };
		ldu(m2, &p2);
		if ( p2.steps[p2.n_steps - 1].cmd != STEP_ROTATE_90) {
			int i;
			ImgOp *io;
			float v, max1 = 0.0, max2 = 0.0;
			for ( i = 0, io = p1.steps; i < p1.n_steps; i++, io++) {
				if ( io->cmd != STEP_SHEAR_X && io->cmd != STEP_SHEAR_Y )
					continue;
				v = fabs(io->p1);
				if ( max1 < v ) max1 = v;
				v = fabs(io->p2);
				if ( max1 < v ) max1 = v;
			}
			for ( i = 0, io = p2.steps; i < p2.n_steps; i++, io++) {
				if ( io->cmd != STEP_SHEAR_X && io->cmd != STEP_SHEAR_Y )
					continue;
				v = fabs(io->p1);
				if ( max2 < v ) max2 = v;
				v = fabs(io->p2);
				if ( max2 < v ) max2 = v;
			}
			if ( max2 < max1 ) select_first = false;
		}
		add_op( &p2, STEP_ROTATE_90, 0.0, 0.0);
#if DEBUG
		printf("LDU2:\n");
		debug_pipeline(&p2);
#endif
	}
#if DEBUG
	printf("use LDU%d\n", select_first ? 1 : 2);
#endif

	*iop = select_first ? p1 : p2;
}

/*
Add offsetting  by adding 1 or 2 extra shear steps with SHEAR_XY_ADD.
*/
static void
add_offsetting( float mx, float my, ImgOpPipeline *iop)
{
	Bool need_x = mx != 0.0, need_y = my != 0.0;

	/* fix existing last shearing, if any */
	if ( iop->n_steps > 0 ) {
		if ( iop->steps[iop->n_steps - 1].cmd == STEP_SHEAR_X && need_x ) {
			iop->steps[iop->n_steps - 1].p2 = mx;
			need_x = false;
		} else if ( iop->steps[iop->n_steps - 1].cmd == STEP_SHEAR_Y && need_y ) {
			iop->steps[iop->n_steps - 1].p2 = my;
			need_y = false;
		} else if ( iop->steps[iop->n_steps - 1].cmd == STEP_ROTATE_90 && iop->n_steps > 1 ) {
			if ( iop->steps[iop->n_steps - 2].cmd == STEP_SHEAR_X && need_y ) {
				iop->steps[iop->n_steps - 2].p2 = my;
				need_y = false;
			} else if ( iop->steps[iop->n_steps - 2].cmd == STEP_SHEAR_Y && need_x ) {
				iop->steps[iop->n_steps - 2].p2 = mx;
				need_x = false;
			}
		}
	}

	/* add pure offsetting steps */
	if ( need_x )
		add_op( iop, STEP_SHEAR_X, 0.0, mx);
	if ( need_y )
		add_op( iop, STEP_SHEAR_Y, 0.0, my);
}

/* Generic 2D transform applied through LDU decomposition as series of shears and/or scaling.
   Does not fare well with inputs that create interim images that are too large, f.ex.
   rotation to angles near 90,270. So it detects rotations to cover for at least these cases,
   and additionally checks whether 90/180/270 integral rotation can be applied. */

Bool
img_2d_transform( Handle self, Matrix matrix, ColorPixel fill, PImage output, Point *aperture)
{
	int applied_steps = 0, n, step, type, channels;
	Point p[4], dimensions[MAX_STEPS+1], offsets[MAX_STEPS];
	Image *i = (PImage)self, tmp_images[MAX_STEPS+1];
	FilterFunc *filter = find_filter(i->scaling);
	ImgOpPipeline iop;
	ImgOp *io;
	float ffill[3];
	Point _aperture;

	if ( aperture == NULL ) aperture = &_aperture;
	aperture->x = floor(matrix[4]);
	aperture->y = floor(matrix[5]);
	matrix[4] -= (double) aperture->x;
	matrix[5] -= (double) aperture->y;
	roundoff((double*) matrix, 6);

	if ((n = check_rotated_case(self, matrix, output, fill, aperture)) >= 0)
		return n;

	memset( &iop, 0, sizeof(iop));
	if (matrix[0] != 1.0 || matrix[1] != 0.0 || matrix[2] != 0.0 || matrix[3] != 1.0)
		select_ldu(matrix, &iop);
	if ( matrix[4] != 0.0 || matrix[5] != 0.0 ) 
		add_offsetting(matrix[4], matrix[5], &iop);

	for ( n = 0; n < iop.n_steps; n++ )
		if ( fabs(iop.steps[n].p1) > 8192.0 || fabs(iop.steps[n].p2) > 8192.0) {
			warn("Image.transform: input matrix is not supported");
			return false;
		}

	reduce_image_to_channels( i->type, &type, &channels);
	fix_ffills( type, channels, fill, ffill );

	bzero(&p, sizeof(p));
	p[1].x = p[2].x = i->w - 1;
	p[2].y = p[3].y = i->h - 1;

	dimensions[0].x  = i-> w;
	dimensions[0].y  = i-> h;
	for ( step = 0, io = iop.steps; step < iop.n_steps;) {
		switch ( io-> cmd) {
		case STEP_SHEAR_X:
			if ( io->p1 == 0.0 && io-> p2 == 0.0 ) goto SKIP;

			if ( !apply_shear(p, io->p1, io->p2,
				dimensions[step].x, dimensions[step].y, 0,
				&offsets[step], &dimensions[step+1], aperture))
				return false;
			applied_steps++;
			break;
		case STEP_SHEAR_Y:
			if ( io->p1 == 0.0 && io-> p2 == 0.0 ) goto SKIP;

			if ( !apply_shear(p, io->p1, io->p2,
				dimensions[step].x, dimensions[step].y, 1,
				&offsets[step], &dimensions[step+1], aperture))
				return false;
			applied_steps++;
			break;
		case STEP_SCALE:
			if ( io->p1 == 1.0 && io-> p2 == 1.0 ) goto SKIP;

			if ( !apply_scale(p, io->p1, io->p2, &offsets[step], &dimensions[step+1], aperture))
				return false;
			applied_steps++;
			break;
		case STEP_ROTATE_90:
			if ( !apply_rotate90(p, dimensions[step].y, &offsets[step], &dimensions[step+1], aperture))
				return false;
			applied_steps++;
			break;
		}
#if DEBUG
		printf("dim(%d,%s,%g,%g) = %d %d\n", step, pipeline_opnames[io->cmd], io->p1, io->p2, dimensions[step+1].x,dimensions[step+1].y);
#endif
		io++;
		step++;
		continue;

	SKIP:
		if ( step < iop.n_steps - 1)
			memmove( io, io + 1, (iop.n_steps - step - 1) * sizeof(ImgOp) );
		iop.n_steps--;
	}

	if ( applied_steps == 0 ) {
		img_fill_dummy( output, 0, 0, 0, NULL, NULL);
		return true;
	}

	img_fill_dummy( &tmp_images[0], i->w * channels, i->h, type, i->data, i->palette);
	for ( step = 0, io = iop.steps; step < iop.n_steps; step++, io++) {
		if ( !create_tmp_image(i, channels, &tmp_images[step+1], dimensions[step+1], fill)) {
			if ( step > 0 )
				free(tmp_images[step].data);
			return false;
		}
		switch ( io-> cmd ) {
		case STEP_SHEAR_X:
			shear_x(&tmp_images[step], channels, &tmp_images[step+1], io->p1, io->p2, filter, -offsets[step].x, ffill, 0);
			break;
		case STEP_SHEAR_Y:
			shear_y(&tmp_images[step], channels, &tmp_images[step+1], io->p1, io->p2, filter, -offsets[step].y, ffill);
			break;
		case STEP_ROTATE_90: {
			PImage src = &tmp_images[step];
			PImage dst = &tmp_images[step+1];
			src-> w   /= channels;
			src-> type = i->type;
			rotate90(src, dst->data, dst->lineSize);
			break;
		}
		case STEP_SCALE: {
			char errbuf[256];
			Image *src = &tmp_images[step], *dst = &tmp_images[step + 1];
			int mx = (io->p1 < 0) ? -1 : 1;
			int my = (io->p2 < 0) ? -1 : 1;
			if ( !ic_stretch( i->type,
				src->data, src->w / channels, src->h,
				dst->data, dst->w * mx / channels, dst->h * my,
				(i->scaling < istTriangle) ? istBox : i->scaling,
				errbuf
			)) {
				if ( step > 0 )
					free(tmp_images[step].data);
				free(tmp_images[step + 1].data);
				warn("%s", errbuf);
				return false;
			}
			break;
		}}

		if ( step > 0 )
			free(tmp_images[step].data);
	}

	*output = tmp_images[iop.n_steps];
	output-> w /= channels;
	output-> type = i->type;
	return true;
}

#ifdef __cplusplus
}
#endif
 
