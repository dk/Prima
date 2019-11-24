#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
rotate270( PImage i, Byte * new_data, int new_line_size)
{
	int y;
	int w   = i->w;
	int pixel_size    = (i-> type & imBPP) / 8;
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
	int tail       = i-> lineSize - w * pixel_size  ;
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
	int w   = i->w;
	int pixel_size    = (i-> type & imBPP) / 8;
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
img_rotate( Handle self, Byte * new_data, int new_line_size, int degrees)
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

#ifdef __cplusplus
}
#endif

