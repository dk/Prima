#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
bitblt_copy( Byte * src, Byte * dst, int count)
{
	memcpy( dst, src, count);
}

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

static void
bitblt_xor( Byte * src, Byte * dst, int count)
{
	while ( count--) *(dst++) ^= *(src++);
}

static void
bitblt_not( Byte * src, Byte * dst, int count)
{
	while ( count--) *(dst++) = ~(*(src++));
}

static void
bitblt_notdstand( Byte * src, Byte * dst, int count)
{
	while ( count--) {
		*dst = ~(*dst) & (*(src++));
		dst++;
	}
}

static void
bitblt_notdstor( Byte * src, Byte * dst, int count)
{
	while ( count--) {
		*dst = ~(*dst) | (*(src++));
		dst++;
	}
}

static void
bitblt_notsrcand( Byte * src, Byte * dst, int count)
{
	while ( count--) *(dst++) &= ~(*(src++));
}

static void
bitblt_notsrcor( Byte * src, Byte * dst, int count)
{
	while ( count--) *(dst++) |= ~(*(src++));
}

static void
bitblt_notxor( Byte * src, Byte * dst, int count)
{
	while ( count--) {
		*dst = ~( *(src++) ^ (*dst));
		dst++;
	}
}

static void
bitblt_notand( Byte * src, Byte * dst, int count)
{
	while ( count--) {
		*dst = ~( *(src++) & (*dst));
		dst++;
	}
}

static void
bitblt_notor( Byte * src, Byte * dst, int count)
{
	while ( count--) {
		*dst = ~( *(src++) | (*dst));
		dst++;
	}
}

static void
bitblt_black( Byte * src, Byte * dst, int count)
{
	memset( dst, 0, count);
}

static void
bitblt_white( Byte * src, Byte * dst, int count)
{
	memset( dst, 0xff, count);
}

static void
bitblt_invert( Byte * src, Byte * dst, int count)
{
	while ( count--) {
		*dst = ~(*dst);
		dst++;
	}
}

static void
bitblt_nooper( Byte * src, Byte * dst, int count)
{
}

PBitBltProc
img_find_blt_proc( int rop )
{
	PBitBltProc proc = NULL;
	switch ( rop) {
	case ropCopyPut:
		proc = bitblt_copy;
		break;
	case ropAndPut:
		proc = bitblt_and;
		break;
	case ropOrPut:
		proc = bitblt_or;
		break;
	case ropXorPut:
		proc = bitblt_xor;
		break;
	case ropNotPut:
		proc = bitblt_not;
		break;
	case ropNotDestAnd:
		proc = bitblt_notdstand;
		break;
	case ropNotDestOr:
		proc = bitblt_notdstor;
		break;
	case ropNotSrcAnd:
		proc = bitblt_notsrcand;
		break;
	case ropNotSrcOr:
		proc = bitblt_notsrcor;
		break;
	case ropNotXor:
		proc = bitblt_notxor;
		break;
	case ropNotAnd:
		proc = bitblt_notand;
		break;
	case ropNotOr:
		proc = bitblt_notor;
		break;
	case ropBlackness:
		proc = bitblt_black;
		break;
	case ropWhiteness:
		proc = bitblt_white;
		break;
	case ropInvert:
		proc = bitblt_invert;
		break;
	case ropNoOper:
		proc = bitblt_nooper;
		break;
	default:
		proc = bitblt_copy;
	}
	return proc;
}


#define dVAL(x) register int32_t s = x
#define STORE \
	*dst++ = ( s > 255 ) ? 255 : s;\
	src += src_inc;\
	src_a += src_a_inc;\
	dst_a += dst_a_inc
#define BLEND_LOOP while(bytes-- > 0)

#define UP(x) ((int32_t)(x) << 8 )
#define DOWN(expr) (((expr) + 127) >> 8)

#define dBLEND_FUNCx(name,expr) \
static dBLEND_FUNC(name) \
{ \
	BLEND_LOOP {\
		dVAL(DOWN(expr));\
		STORE;\
	}\
}

#define S (*src)
#define D (*dst)
#define SA (*src_a)
#define DA (*dst_a)
#define INVSA (255 - *src_a)
#define INVDA (255 - *dst_a)

/* sss */
static dBLEND_FUNC(blend_src_copy)
{
	if ( src_inc )
		memcpy( dst, src, bytes);
	else
		memset( dst, *src, bytes);
}

/* ddd */
static dBLEND_FUNC(blend_dst_copy)
{
}

/* 0 */
static dBLEND_FUNC(blend_clear)
{
	memset( dst, 0, bytes);
}

dBLEND_FUNCx(blend_blend,    UP(S) + UP(D) * INVSA / 255)

dBLEND_FUNCx(blend_src_over, (UP(S) * SA    + UP(D) * INVSA) / 255)
dBLEND_FUNCx(blend_xor,      (UP(S) * INVDA + UP(D) * INVSA) / 255)
dBLEND_FUNCx(blend_dst_over, (UP(D) * DA    + UP(S) * INVDA) / 255)
dBLEND_FUNCx(blend_src_in,    UP(S) * DA    / 255)
dBLEND_FUNCx(blend_dst_in,    UP(D) * SA    / 255)
dBLEND_FUNCx(blend_src_out,   UP(S) * INVDA / 255)
dBLEND_FUNCx(blend_dst_out,   UP(D) * INVSA / 255)
dBLEND_FUNCx(blend_src_atop, (UP(S) * DA    + UP(D) * INVSA) / 255)
dBLEND_FUNCx(blend_dst_atop, (UP(D) * SA    + UP(S) * INVDA) / 255)

/* sss + ddd */
static dBLEND_FUNC(blend_add)
{
	BLEND_LOOP {
		dVAL(S + D);
		STORE;
	}
}

/* SEPARABLE(S * D) */
dBLEND_FUNCx(blend_multiply, (UP(D) * (S + INVSA) + UP(S) * INVDA) / 255)

/* SEPARABLE(D * SA + S * DA - S * D) */
dBLEND_FUNCx(blend_screen,   (UP(S) * 255 + UP(D) * (255 - S)) / 255)

#define SEPARABLE(f) (UP(S) * INVDA + UP(D) * INVSA + (f))/255
dBLEND_FUNCx(blend_overlay, SEPARABLE(
	(2 * D < DA) ?
		(2 * UP(D) * S) :
		(UP(SA) * DA - UP(2) * (DA - D) * (SA - S)) 
	))

static dBLEND_FUNC(blend_darken)
{
	BLEND_LOOP {
		register int32_t ss = UP(S) * DA;
		register int32_t dd = UP(D) * SA;
		dVAL(DOWN(SEPARABLE((ss > dd) ? dd : ss)));
		STORE;
	}
}

static dBLEND_FUNC(blend_lighten)
{
	BLEND_LOOP {
		register int32_t ss = UP(S) * DA;
		register int32_t dd = UP(D) * SA;
		dVAL(DOWN(SEPARABLE((ss > dd) ? ss : dd)));
		STORE;
	}
}

static dBLEND_FUNC(blend_color_dodge)
{
	BLEND_LOOP {
		register int32_t s;
		if ( S >= SA ) {
			s = D ? UP(SA) * DA : 0;
		} else {
			register int32_t dodge = D * SA / (SA - S);
			s = UP(SA) * ((DA < dodge) ? DA : dodge);
		}
		s = DOWN(SEPARABLE(s));
		STORE;
	}
}

static dBLEND_FUNC(blend_color_burn)
{
	BLEND_LOOP {
		register int32_t s;
		if ( S == 0 ) {
			s = (D < DA) ? 0 : UP(SA) * DA;
		} else {
			register int32_t burn = (DA - D) * SA / S;
			s = (DA < burn) ? 0 : UP(SA) * (DA - burn);
		}
		s = DOWN(SEPARABLE(s));
		STORE;
	}
}

dBLEND_FUNCx(blend_hard_light, SEPARABLE(
	(2 * S < SA) ?
		(2 * UP(D) * S) :
		(UP(SA) * DA - UP(2) * (DA - D) * (SA - S)) 
	))

static dBLEND_FUNC(blend_soft_light)
{
	BLEND_LOOP {
		register int32_t s;
		if ( 2 * S < SA ) {
			s = DA ? D * (UP(SA) - UP(DA - D) * (SA - 2 * S) / DA ) : 0;
		} else if (DA == 0) {
			s = 0;
		} else if (4 * D <= DA) {
			s = D * (UP(SA) + (2 * S - SA) * ((UP(16) * D / DA - UP(12)) * D / DA + UP(3)));
		} else {
			s = 256 * (D * SA + (sqrt(D * DA) - D) * (2 * SA - S));
		}
		s = DOWN(SEPARABLE(s));
		STORE;
	}
}

static dBLEND_FUNC(blend_difference)
{
	BLEND_LOOP {
		dVAL(UP(D) * SA - UP(S) * DA);
		if ( s < 0 ) s = -s;
		s = DOWN(SEPARABLE(s));
		STORE;
	}
}

dBLEND_FUNCx(blend_exclusion, SEPARABLE( UP(S) * (DA - 2 * D) + UP(D) * SA ))

static BlendFunc* blend_functions[] = {
	blend_blend,
	blend_xor,
	blend_src_over,
	blend_dst_over,
	blend_src_copy,
	blend_dst_copy,
	blend_clear,
	blend_src_in,
	blend_dst_in,
	blend_src_out,
	blend_dst_out,
	blend_src_atop,
	blend_dst_atop,
	blend_add,
	blend_multiply,
	blend_screen,
	blend_dst_copy,
	blend_overlay,
	blend_darken,
	blend_lighten,
	blend_color_dodge,
	blend_color_burn,
	blend_hard_light,
	blend_soft_light,
	blend_difference,
	blend_exclusion
};

void
img_find_blend_proc( int rop, BlendFunc ** blend1, BlendFunc ** blend2 )
{
	*blend1 = blend_functions[rop];
	*blend2 = (rop >= ropMultiply) ? blend_functions[ropScreen] : blend_functions[rop];
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
	if ( PImage(self)-> type == imByte ) {
		pixels = 1;
	} else if ( PImage(self)-> type == imRGB ) {
		pixels = 3;
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

Byte
rop_1bit_transform(Byte fore, Byte back, Byte rop)
{
	/*
	Special case with current foreground and background colors for 1-bit bitmaps/pixmaps, see also
	L<pod/Prima/Drawable.pod | Monochrome bitmaps>.

	Raster ops can be identified by a fingerprint.  For example, Or's is 14
	and Noop's is 10:

        0 | 0 =    0                      0 | 0 =    0
        0 | 1 =   1                       0 | 1 =   1
        1 | 0 =  1                        1 | 0 =  0
        1 | 1 = 1                         1 | 1 = 1
        ---     ----                      ---     ----
                1110 = 14                         1010 = 10

	when this special case uses not actual 0s and 1s, but bit values of
	foreground and background color instead, the resulting operation can
	still be expressed in rops, but these needs to be adjusted. Let's
	consider a case where both colors are 0, and rop = OrPut:

        0 | 0 =    0
        0 | 1 =   1
        0 | 0 =  0
        0 | 1 = 1
        ---     ----
                1010 = 10

	this means that in these conditions, Or (as well as Xor and AndInverted) becomes Noop.

	*/
	if ( fore == 0 && back == 0 ) {
		switch( rop) {
			case ropAndPut:
			case ropNotDestAnd:
			case ropBlackness:
			case ropCopyPut:       return ropBlackness;
			case ropNotXor:
			case ropInvert:
			case ropNotOr:
			case ropNotDestOr:     return ropInvert;
			case ropNotSrcAnd:
			case ropNoOper:
			case ropOrPut:
			case ropXorPut:        return ropNoOper;
			case ropNotAnd:
			case ropNotPut:
			case ropNotSrcOr:
			case ropWhiteness:     return ropWhiteness;
		}
	} else if ( fore == 0 && back == 1 ) {
		switch( rop) {
			case ropAndPut:        return ropNotSrcAnd;
			case ropNotSrcAnd:     return ropAndPut;
			case ropNotDestAnd:    return ropNotOr;
			case ropBlackness:     return ropBlackness;
			case ropCopyPut:       return ropNotPut;
			case ropNotPut:        return ropCopyPut;
			case ropNotXor:        return ropXorPut;
			case ropInvert:        return ropInvert;
			case ropNotAnd:        return ropNotDestOr;
			case ropNoOper:        return ropNoOper;
			case ropNotOr:         return ropNotDestAnd;
			case ropOrPut:         return ropNotSrcOr;
			case ropNotSrcOr:      return ropOrPut;
			case ropNotDestOr:     return ropNotAnd;
			case ropWhiteness:     return ropWhiteness;
			case ropXorPut:        return ropNotXor;
		}
	} else if ( fore == 1 && back == 1 ) {
		switch( rop) {
			case ropAndPut:
			case ropNotSrcOr:
			case ropNotXor:
			case ropNoOper:        return ropNoOper;
			case ropNotSrcAnd:
			case ropBlackness:
			case ropNotPut:
			case ropNotOr:         return ropBlackness;
			case ropInvert:
			case ropNotAnd:
			case ropNotDestAnd:
			case ropXorPut:        return ropInvert;
			case ropOrPut:
			case ropNotDestOr:
			case ropWhiteness:
			case ropCopyPut:       return ropWhiteness;
		}
	}
	return rop;
}

#ifdef __cplusplus
}
#endif
