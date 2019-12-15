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

static Bool transform_fail(void) {
	warn("Image.rotate/transform: transformation results in invalid image");
	return false;
}

/* Transform 4 corners to next shearing, then calculate integral pixels it occupies.
This is needed to calculate the least enclosed rect for a sheared (and, ultimately, rotated) image */
static Bool
apply_shear( Point * points, float func, int w, int h, int index, Point * out_min, Point * out_dim)
{
	int i, min_i;
	Point * p, min_p;
	float max_shift, min, max, center, tmp[4];

	max_shift = (func >= 0) ? 0 : func * ((index ? w : h) - 1);

	/* apply shearing */
	max = 0;
	for ( i = 0, p = points; i < 4; i++, p++) {
		float n = index ?
			p->y + p->x * func :
			p->x + p->y * func;
		n -= max_shift;
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
		int n = (tmp[i] > center) ? (int)ceil(tmp[i]) : (int)(floor(tmp[i]));
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

	fill_dimensions( points, min_p, out_min, out_dim);
	return true;
}

/* Transform 4 corners to scaling shearing, then calculate integral pixels it occupies. */
static Bool
apply_scale( Point * points, float sx, float sy, Point * out_min, Point * out_dim)
{
	int i;
	Point * p, min_p;
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
		p->x = (tmp[i].x > center.x) ? (int)ceil(tmp[i].x + sx) - 1 : (int)(floor(tmp[i].x));
		p->y = (tmp[i].y > center.y) ? (int)ceil(tmp[i].y + sy) - 1 : (int)(floor(tmp[i].y));
		if ( p->x <= -16383 || p->x >= 16384 || p->y <= -16383 || p->y >= 16384 )
			return transform_fail();
		if (i == 0 || min_p.x > p->x)
			min_p.x = p->x;
		if (i == 0 || min_p.y > p->y)
			min_p.y = p->y;
	}

	fill_dimensions( points, min_p, out_min, out_dim);
	return true;
}

static Bool
apply_rotate90( Point * points, int h, Point * out_min, Point * out_dim)
{
	int i;
	Point * p, min_p = {0,0};
	for ( i = 0, p = points; i < 4; i++, p++) {
		int x = h - p->y;
		p-> y = p-> x;
		p-> x = x;
	}
	fill_dimensions( points, min_p, out_min, out_dim);
	return true;
}

static Bool
create_tmp_image( PImage template, int channels, PImage target, Point size)
{
	img_fill_dummy( target, size.x, size.y, template->type, NULL, template->palette);

	if (target->dataSize == 0)
		croak("rotate/transform panic: interim image (%d,%d) is NULL", size.x, size.y);
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
static void                                                       \
shear_x_scanline_ ## type(                                        \
	void * _src, int channels, int src_w,                     \
	void * _dst, int dst_w,                                   \
	int delta, float sf, Bool reverse                         \
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
		leftover[c] = 0;                                  \
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
static void                                                               \
shear_y_scanline_ ## type(                                                \
	void * _src, int channels, int src_w, int src_h, int src_stride,  \
	void * _dst, int dst_w, int dst_h, int dst_stride,                \
	int delta, float sf                                               \
) {                                                                       \
	int y, c, ny;                                                     \
	float leftover[3];                                                \
	type * src = (type*)_src, * dst = ( type* ) _dst;                 \
	pixel_interim_type new_pixel;                                     \
	                                                                  \
	for ( c = 0; c < channels; c++)                                   \
		leftover[c] = 0;                                          \
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
	int delta, float sf, Bool reverse);

typedef void ShearYFunc(
	void * _src, int channels, int src_w, int src_h, int src_stride,
	void * _dst, int dst_w, int dst_h, int dst_stride,
	int delta, float sf
);

#define FIND_SHEAR_FUNC(letter) \
	case imByte   : shear_func = shear_## letter ##_scanline_Byte;   break;\
	case imShort  : shear_func = shear_## letter ##_scanline_Short;  break;\
	case imLong   : shear_func = shear_## letter ##_scanline_Long;   break;\
	case imFloat  : shear_func = shear_## letter ##_scanline_float;  break;\
	case imDouble : shear_func = shear_## letter ##_scanline_double; break;

static void
shear_x( PImage src, int channels, PImage dst, float func, FilterFunc filter, int dx, Bool apply_180)
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
		float sk = ( func > 0 ) ? func * y : (-func) * (h - y - 1);
		int    si = (int) floor(sk);
		shear_func(
			src_scanline + dsrc * y, channels, w,
			dst_scanline + ddst * y, dw,
			si + dx, filter(sk - si), apply_180
		);
	}
}

static void
shear_y( PImage src, int channels, PImage dst, float func, FilterFunc filter, int dy)
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
		float sk = ( func > 0 ) ? func * x : (-func) * (w - x - 1);
		int    si = (int) floor(sk);
		shear_func(
			src_scanline + dscanline * x, channels, w, h, src->lineSize,
			dst_scanline + dscanline * x, dw, dst->h, dst->lineSize,
			si + dy, filter(sk - si));
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


/* Fast rotation by Paeth algortihm. Accepts grayscale images with bpp >= 8, and 24 bpp RGBs */
Bool
img_generic_rotate( Handle self, float degrees, PImage dummy)
{
	Bool apply_180 = false;
	Image *i = (PImage)self, s0, s1, s2;
	float sin1, tan2;
	Point p[4], s1dim, s2dim, s3dim, s1min, s2min, s3min;
	int channels;
	FilterFunc *filter = find_filter(i->scaling);

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

	if ( !(
		apply_shear(p, tan2, i->w, i-> h, 0, &s1min, &s1dim) &&
		apply_shear(p, sin1, s1dim.x, s1dim.y, 1, &s2min, &s2dim) &&
		apply_shear(p, tan2, s2dim.x, s2dim.y, 0, &s3min, &s3dim)
	))
		return false;

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

static Bool
integral_rotate( Handle self, int degrees, PImage dummy)
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

	img_fill_dummy( dummy, w, h, i->type, NULL, i->palette);
	if (!(dummy->data = malloc( dummy->dataSize))) {
		warn("not enough memory: %d bytes", dummy->dataSize);
		return false;
	}
	img_integral_rotate( self, dummy->data, dummy->lineSize, degrees);
	return true;
}

/* max image size is 16K, so best precision we need is 1/32 K */
static void
roundoff(float * matrix, int count)
{
	while ( count-- ) 
		*matrix = floor(*matrix * 32768.0 + 0.5) / 32768.0;
}

/* very special case for rotation */
static int
check_rotated_case( Handle self, float *matrix, PImage dummy)
{
	if ( matrix[0] == matrix[3] && matrix[1] == -matrix[2] ) {
		float angle = acos(matrix[0]);
		float sin1  = sin( angle );
		roundoff( &sin1, 1);
		if ( sin1 == matrix[1] ) {
			float cos1 = matrix[0];
			if ( cos1 == 0.0 ) {
				if ( sin1 == 1.0 ) 
					return integral_rotate(self, 90, dummy);
				else if ( sin1 == -1.0 )
					return integral_rotate(self, 270, dummy);
			} else if ( cos1 == 1.0 && sin1 == 0.0 ) {
				img_fill_dummy( dummy, 0, 0, 0, NULL, NULL);
				return true;
			} else if ( cos1 == -1.0 && sin1 == 0.0 ) 
				return integral_rotate(self, 180, dummy);

			return img_generic_rotate( self, angle * RAD, dummy);
		}
	}
	return -1;
}

#define SHEAR_X 0
#define SHEAR_Y 1
#define SCALE_X 2
#define SCALE_Y 3
#define MAX_LDU_COEFF 3
typedef float LDUCoeff[MAX_LDU_COEFF+1];

#define MAX_STEPS 4
#define STEP_SHEAR_X   0
#define STEP_SHEAR_Y   1
#define STEP_SCALE     2
#define STEP_ROTATE_90 3

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
3) rotate90, shear x, scale, shear y
4) rotate90, shear y, scale, shear x

where it is up to the consumer to skip individual steps if these are no-ops.

That is of course a valid question whether #3 and #4 are generally more optimal
to implement that way rather than just using a direct pixel resampling with
direct 2D transformation, but that's yet to be measured.

[1] https://en.wikipedia.org/wiki/Block_LU_decomposition

*/
static void
ldu( float *matrix, LDUCoeff c, int * steps, int * n_steps)
{
	float local_matrix[4];
	if ( matrix[0] == 0.0 ) {
		if (matrix[3] == 0.0) {
			if ( matrix[2] != 0.0 && matrix[1] != 0.0 ) {
				/* scaling preceded by 90-rotation (0,-1,1,0) */
				steps[(*n_steps)++] = STEP_ROTATE_90;
				local_matrix[0] =  matrix[1];
				local_matrix[1] = -matrix[0];
				local_matrix[2] =  matrix[3];
				local_matrix[3] = -matrix[2];
				matrix = local_matrix;
				goto UDL;
			} else {
				/* degenerate scaling */
				steps[(*n_steps)++] = STEP_SCALE;
				c[SCALE_X] = matrix[3];
				c[SCALE_Y] = matrix[2];
				c[SHEAR_X] = c[SHEAR_Y] = 0.0;
			}
		} else {
		UDL:
			steps[(*n_steps)++] = STEP_SHEAR_Y;
			steps[(*n_steps)++] = STEP_SCALE;
			steps[(*n_steps)++] = STEP_SHEAR_X;
			c[SHEAR_X] = matrix[2] / matrix[3];
			c[SHEAR_Y] = matrix[1] / matrix[3];
			c[SCALE_X] = matrix[0] - matrix[1] * matrix[2] / matrix[3];
			c[SCALE_Y] = matrix[3];
		}
	} else {
		steps[(*n_steps)++] = STEP_SHEAR_X;
		steps[(*n_steps)++] = STEP_SCALE;
		steps[(*n_steps)++] = STEP_SHEAR_Y;
		c[SHEAR_X] = matrix[2] / matrix[0];
		c[SHEAR_Y] = matrix[1] / matrix[0];
		c[SCALE_X] = matrix[0];
		c[SCALE_Y] = matrix[3] - matrix[1] * matrix[2] / matrix[0];
	}
	if ( c[SHEAR_X] == -0.0 ) c[SHEAR_X] = 0.0;
	if ( c[SHEAR_Y] == -0.0 ) c[SHEAR_Y] = 0.0;
	if ( c[SCALE_X] == -0.0 ) c[SCALE_X] = 0.0;
	if ( c[SCALE_Y] == -0.0 ) c[SCALE_Y] = 0.0;
}

/* check two ldu variants, with and without prerequisite rotation90.
Select the one that has less scale/shear factor to avoid distortions and interim image being too big */
static void
select_ldu( float *matrix, LDUCoeff c, int * steps, int * n_steps)
{
	int select_first = true;
	int steps1[MAX_STEPS+1], steps2[MAX_STEPS+1], n_steps1 = 0, n_steps2 = 0;
	LDUCoeff c1, c2;

	memset( steps, 0, 16);

	ldu(matrix, c1, steps1, &n_steps1);
	/*
	 printf("LDU1: %d steps [%d %d %d %d], shear: %g %g, scale: %g %g\n", n_steps1, 
		 steps1[0], steps1[1], steps1[2], steps1[3],
		 c1[SHEAR_X], c1[SHEAR_Y], c1[SCALE_X], c1[SCALE_Y]); 
	*/
	if ( steps1[0] != STEP_ROTATE_90 ) {
		int i;
		float max1 = 0.0, max2 = 0.0;
		float m2[4] = { matrix[1], -matrix[0], matrix[3], -matrix[2] };
		steps2[n_steps2++] = STEP_ROTATE_90;
		ldu(m2, c2, steps2, &n_steps2);
		/*
		 printf("LDU2: %d steps [%d %d %d %d], shear: %g %g, scale: %g %g\n", n_steps2, 
			 steps2[0], steps2[1], steps2[2], steps2[3],
			 c2[SHEAR_X], c2[SHEAR_Y], c2[SCALE_X], c2[SCALE_Y]); 
		*/
		if ( steps2[1] != STEP_ROTATE_90) {
			for ( i = 0; i <= MAX_LDU_COEFF; i++) {
				float v1 = fabs(c1[i]), v2 = fabs(c2[i]);
				if ( max1 < v1 ) max1 = v1;
				if ( max2 < v2 ) max2 = v2;
			}
			if ( max2 < max1 ) select_first = false;
		}
	}
	/* printf("use LDU%d\n", select_first ? 1 : 2); */

	if ( select_first ) {
		memcpy( c, c1, sizeof(float) * ( MAX_LDU_COEFF + 1));
		memcpy( steps, steps1, sizeof(int) * (*n_steps = n_steps1));
	} else {
		memcpy( c, c2, sizeof(float) * ( MAX_LDU_COEFF + 1));
		memcpy( steps, steps2, sizeof(int) * (*n_steps = n_steps2));
	}
}

/* Generic 2D transform applied through LDU decomposition as series of shears and/or scaling.
   Does not fare well with inputs that create interim images that are too large, f.ex.
   rotation to angles near 90,270. So it detects rotations to cover for at least these cases,
   and addionally checks whether 90/180/270 integral rotation can be applied. */

Bool
img_2d_transform( Handle self, float *matrix, PImage dummy)
{
	int n_steps = 0, applied_steps = 0, steps[MAX_STEPS], step, n, type, channels;
	Point p[4], dimensions[MAX_STEPS+1], offsets[MAX_STEPS];
	Image *i = (PImage)self, tmp_images[MAX_STEPS+1];
	FilterFunc *filter = find_filter(i->scaling);
	LDUCoeff c;

	roundoff(matrix, 4);
	if ((n = check_rotated_case(self, matrix, dummy)) >= 0)
		return n;
	select_ldu(matrix, c, steps, &n_steps);
	for ( n = 0; n <= MAX_LDU_COEFF; n++)
		if ( fabs(c[n]) > 8192.0) {
			warn("Image.tranform: input matrix is not supported");
			return false;
		}

	type = i->type;
	if ( i->type == imRGB)  {
		channels = 3;
		type = imByte;
	}
	else if ( i->type & (imComplexNumber | imTrigComplexNumber) ) {
		channels = 2;
		type &= ~(imComplexNumber | imTrigComplexNumber);
	} else
		channels = 1;

	bzero(&p, sizeof(p));
	p[1].x = p[2].x = i->w - 1;
	p[2].y = p[3].y = i->h - 1;

	dimensions[0].x  = i-> w;
	dimensions[0].y  = i-> h;
	for ( step = 0; step < n_steps; step++) {
		switch ( steps[step]) {
		case STEP_SHEAR_X:
			if ( c[SHEAR_X] == 0.0 ) goto SKIP;
			if ( !apply_shear(p, c[SHEAR_X], 
				dimensions[step].x, dimensions[step].y, 0, 
				&offsets[step], &dimensions[step+1]))
				return false;
			applied_steps++;
			break;
		case STEP_SHEAR_Y:
			if ( c[SHEAR_Y] == 0.0 ) goto SKIP;
			if ( !apply_shear(p, c[SHEAR_Y],
				dimensions[step].x, dimensions[step].y, 1,
				&offsets[step], &dimensions[step+1]))
				return false;
			applied_steps++;
			break;
		case STEP_SCALE:
			if ( c[SCALE_X] == 1.0 && c[SCALE_Y] == 1.0 ) goto SKIP;
			if ( !apply_scale(p, c[SCALE_X], c[SCALE_Y], &offsets[step], &dimensions[step+1]))
				return false;
			applied_steps++;
			break;
		case STEP_ROTATE_90:
			if ( !apply_rotate90(p, dimensions[step].y, &offsets[step], &dimensions[step+1]))
				return false;
			applied_steps++;
			break;
		}

		continue;

	SKIP:
		switch ( step ) {
		case 0: steps[0] = steps[1]; /* fallthrough */
		case 1: steps[1] = steps[2];
		case 2: steps[2] = steps[3];
		}
		step--;
		n_steps--;
	}

	if ( applied_steps == 0 ) {
		img_fill_dummy( dummy, 0, 0, 0, NULL, NULL);
		return true;
	}

	img_fill_dummy( &tmp_images[0], i->w * channels, i->h, type, i->data, i->palette);
	for ( step = 0; step < n_steps; step++) {
		if ( !create_tmp_image(i, channels, &tmp_images[step+1], dimensions[step+1])) {
			if ( step > 0 )
				free(tmp_images[step].data);
			return false;
		}
		switch ( steps[step]) {
		case STEP_SHEAR_X:
			shear_x(&tmp_images[step], channels, &tmp_images[step+1], c[SHEAR_X], filter, -offsets[step].x, 0);
			break;
		case STEP_SHEAR_Y:
			shear_y(&tmp_images[step], channels, &tmp_images[step+1], c[SHEAR_Y], filter, -offsets[step].y);
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
			int mx = (c[SCALE_X] < 0) ? -1 : 1;
			int my = (c[SCALE_Y] < 0) ? -1 : 1;
			if ( !ic_stretch( i->type,
				src->data, src->w / channels, src->h,
				dst->data, dst->w * mx / channels, dst->h * my,
				i->scaling, errbuf)) {
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

	*dummy = tmp_images[n_steps];
	dummy-> w /= channels;
	dummy-> type = i->type;
	return true;
}

#ifdef __cplusplus
}
#endif
 
