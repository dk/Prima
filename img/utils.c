#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

void
img_fill_dummy( PImage dummy, int w, int h, int type, Byte * data, RGBColor * palette)
{
	bzero( dummy, sizeof(Image));
	dummy-> self     = CImage;
	dummy-> w        = w;
	dummy-> h        = h;
	dummy-> type     = type;
	dummy-> data     = data;
	dummy-> lineSize = LINE_SIZE(w, type);
	dummy-> dataSize = dummy-> lineSize * h;
	dummy-> palette  = palette;
	dummy-> updateLock = true; /* just in case */

	if ( type == imRGB ) {
		dummy-> palSize = 0;
	} else if ( type & ( imRealNumber|imComplexNumber|imTrigComplexNumber)) {
		dummy-> palSize = 256;
	} else {
		dummy-> palSize = 1 << (type & imBPP);
	}
}

/* reformat color values to imByte/imRGB */
Bool
img_resample_colors( Handle dest, int bpp, PImgPaintContext ctx)
{
	RGBColor fg, bg;
	int type = PImage(dest)->type;
	int rbpp = type & imBPP;
	if (rbpp <= 8 ) {
		fg = PImage(dest)->palette[*(ctx->color)];
		bg = PImage(dest)->palette[*(ctx->backColor)];
	} else switch ( type ) {
	case imRGB:
		fg.b = ctx->color[0];
		fg.g = ctx->color[1];
		fg.r = ctx->color[2];
		bg.b = ctx->backColor[0];
		bg.g = ctx->backColor[1];
		bg.r = ctx->backColor[2];
		break;
	case imShort:
		fg.b = fg.g = fg.r = *((Short*)(ctx->color));
		bg.b = bg.g = bg.r = *((Short*)(ctx->backColor));
		break;
	case imLong:
		fg.b = fg.g = fg.r = *((Long*)(ctx->color));
		bg.b = bg.g = bg.r = *((Long*)(ctx->backColor));
		break;
	case imFloat: case imComplex: case imTrigComplex:
		fg.b = fg.g = fg.r = *((float*)(ctx->color));
		bg.b = bg.g = bg.r = *((float*)(ctx->backColor));
		break;
	case imDouble: case imDComplex: case imTrigDComplex:
		fg.b = fg.g = fg.r = *((double*)(ctx->color));
		bg.b = bg.g = bg.r = *((double*)(ctx->backColor));
		break;
	default:
		return false;
	}
	if ( bpp == imByte ) {
		*(ctx->color)     = (fg.r + fg.g + fg.b) / 3;
		*(ctx->backColor) = (bg.r + bg.g + bg.b) / 3;
	} else {
                ctx->color[0] = fg.b;
                ctx->color[1] = fg.g;
                ctx->color[2] = fg.r;
                ctx->backColor[0] = bg.b;
                ctx->backColor[1] = bg.g;
                ctx->backColor[2] = bg.r;
	}
	return true;
}

/* alpha stuff */
void
img_premultiply_alpha_constant( Handle self, int alpha)
{
	Byte * data;
	int i, j, pixels;
	Image dummy;
	if ( PImage(self)-> type == imByte ) {
		pixels = 1;
	} else if ( PImage(self)-> type == imRGB ) {
		pixels = 3;
	} else if (( PImage(self)-> type & imBPP) <= 8 ) {
		img_fill_dummy( &dummy, PImage(self)->palSize * 3, 1, imByte, NULL, NULL);
		self = (Handle) &dummy;
		pixels = 1;
	} else {
		croak("Not implemented");
	}

	data = PImage(self)-> data;
	for ( i = 0; i < PImage(self)-> h; i++) {
		register Byte *d = data, k;
		for ( j = 0; j < PImage(self)-> w; j++ ) {
			for ( k = 0; k < pixels; k++, d++)
				*d = (alpha * *d) / 255.0 + .5;
		}
		data += PImage(self)-> lineSize;
	}
}

void
img_premultiply_alpha_map( Handle self, Handle alpha)
{
	Byte * data, * mask;
	int i, pixels;
	if ( PImage(self)-> type == imByte ) {
		pixels = 1;
	} else if ( PImage(self)-> type == imRGB ) {
		pixels = 3;
	} else {
		croak("Not implemented");
	}

	if ( PImage(alpha)-> type != imByte )
		croak("Not implemented");

	data = PImage(self)-> data;
	mask = PImage(alpha)-> data;
	for ( i = 0; i < PImage(self)-> h; i++) {
		int j;
		register Byte *d = data, *m = mask, k;
		for ( j = 0; j < PImage(self)-> w; j++ ) {
			register uint16_t alpha = *m++;
			for ( k = 0; k < pixels; k++, d++)
				*d = (alpha * *d) / 255.0 + .5;
		}
		data += PImage(self)-> lineSize;
		mask += PImage(alpha)-> lineSize;
	}
}

void
img_fill_alpha_buf( Byte * dst, Byte * src, int width, int bpp)
{
	register int x = width;
	if ( bpp == 3 ) {
		while (x-- > 0) {
			register Byte a = *src++;
			*dst++ = a;
			*dst++ = a;
			*dst++ = a;
		}
	} else
		memcpy( dst, src, width * bpp);
}

void
img_multiply_alpha( Byte * src, Byte * alpha, int alpha_step, Byte * dst, int bytes)
{
	if ( alpha_step == 0 ) {
		while (bytes--)
			*(dst++) = *(src++) * *(alpha  ) / 255.0 + .5;
	} else {
		while (bytes--)
			*(dst++) = *(src++) * *(alpha++) / 255.0 + .5;
	}
}

#ifdef __cplusplus
}
#endif
