#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILL_PATTERN_SIZE 8
#define BLT_BUFSIZE ((MAX_SIZEOF_PIXEL * FILL_PATTERN_SIZE * FILL_PATTERN_SIZE) * 2)

static void
multiply( Byte * src, Byte * alpha, int alpha_step, Byte * dst, int bytes)
{
	while (bytes--) {
		*(dst++) = *(src++) * *alpha / 255.0 + .5;
		alpha += alpha_step;
	}
}

typedef struct {
	int bpp, als, dls, step, pat_x_offset;
	Byte * dst, *dstMask, *pattern_buf, *adbuf;
	Bool use_dst_alpha, solid;
	Byte src_alpha, dst_alpha_mul;
	PImgPaintContext ctx;
	BlendFunc * blend1, * blend2;
} ImgBarAlphaCallbackRec;

static Bool
img_bar_alpha_single_opaque( int x, int y, int w, int h, ImgBarAlphaCallbackRec * ptr)
{
	int i;
	const int bpp = ptr->bpp;
	const int blt_bytes = w * bpp;
	const int dls = ptr->dls;
	const int als = ptr->als;
	const Byte * d = ptr->dst + y * ptr->dls + x * bpp;
	Byte * a = (als > 0) ? ptr->dstMask + y * als + x : NULL;
	int blt_step = (blt_bytes > ptr->step) ? ptr->step : blt_bytes;
	Byte * pat_ptr;

	if (!ptr->solid && (( ptr-> pat_x_offset % FILL_PATTERN_SIZE ) != (x % FILL_PATTERN_SIZE))) {
		int dx = (x % FILL_PATTERN_SIZE) - ( ptr-> pat_x_offset % FILL_PATTERN_SIZE );
		if ( dx < 0 ) dx += FILL_PATTERN_SIZE;
		pat_ptr = ptr->pattern_buf + dx * bpp;
		if ( blt_step + FILL_PATTERN_SIZE * bpp > BLT_BUFSIZE )
			blt_step -= FILL_PATTERN_SIZE * bpp;
	} else
		pat_ptr = ptr->pattern_buf;

	for ( i = 0; i < h; i++) {
		Byte *adbuf_ptr;

		int bytes = blt_bytes;
		Byte *d_ptr = (Byte *)d;
		Byte *s_ptr = pat_ptr + ((y + i) % FILL_PATTERN_SIZE) * ptr->step;

		if ( !ptr->use_dst_alpha ) {
			adbuf_ptr = ptr->adbuf;
			img_fill_alpha_buf( adbuf_ptr, (Byte*)a, w, bpp);
		} else
			adbuf_ptr = ptr->adbuf;

		while ( bytes > 0 ) {
			ptr->blend1(
				s_ptr, 1,
				&ptr->src_alpha, 0,
				d_ptr,
				adbuf_ptr, ptr->use_dst_alpha ? 0 : 1,
				( bytes > blt_step ) ? blt_step : bytes);
			bytes -= blt_step;
			d_ptr += blt_step;
		}
		d += dls;

		if ( a ) {
			if ( ptr->dst_alpha_mul < 255 )
				multiply( a, &ptr->dst_alpha_mul, 0, a, w);
			ptr->blend2(
				&ptr->src_alpha, 0,
				&ptr->src_alpha, 0,
				(Byte*)a,
				a, ptr->use_dst_alpha ? 0 : 1,
				w
			);
			a += als;
		}
	}
	return true;
}

static Bool
img_bar_alpha_single_transparent( int x, int y, int w, int h, ImgBarAlphaCallbackRec * ptr)
{
	int i, j;
	const int bpp = ptr->bpp;
	const int blt_bytes = w * bpp;
	const int dls = ptr->dls;
	const int als = ptr->als;
	const Byte * d = ptr->dst + y * ptr->dls + x * bpp;
	Byte * a = (als > 0) ? ptr->dstMask + y * als + x : NULL;

	for ( i = 0; i < h; i++) {
		unsigned int pat;
		Byte *d_ptr, *a_ptr, *adbuf_ptr, *adbuf_ptr2;
		pat = (unsigned int) ptr->ctx->pattern[(i + FILL_PATTERN_SIZE - ptr->ctx->patternOffset. y) % FILL_PATTERN_SIZE];
		if ( pat == 0 ) goto NEXT_LINE;
		pat = (((pat << 8) | pat) >> ((ptr->ctx->patternOffset. x + 8 - (x % 8)) % FILL_PATTERN_SIZE)) & 0xff;

		if ( !ptr->use_dst_alpha ) {
			adbuf_ptr = ptr->adbuf;
			img_fill_alpha_buf( adbuf_ptr, (Byte*)a, w, bpp);
		} else
			adbuf_ptr = ptr->adbuf;

		if ( pat == 0xff && bpp == 1) {
			ptr->blend1(
				ptr->ctx->color, 0,
				&ptr->src_alpha, 0,
				(Byte*)d,
				adbuf_ptr, ptr->use_dst_alpha ? 0 : 1,
				blt_bytes);
			if ( a ) {
				if ( ptr->dst_alpha_mul < 255 )
					multiply( a, &ptr->dst_alpha_mul, 0, a, w);
				ptr->blend2(
					&ptr->src_alpha, 0,
					&ptr->src_alpha, 0,
					(Byte*)a,
					a, ptr->use_dst_alpha ? 0 : 1,
					w
				);
			}
			goto NEXT_LINE;
		}

		for (
			j = 0, d_ptr = (Byte*)d, a_ptr = (Byte*)a, adbuf_ptr2 = adbuf_ptr;
			j < w;
			j++
		) {
			if ( pat & (0x80 >> (j % 8)) ) {
				ptr->blend1(
					ptr->ctx->color, 0,
					&ptr->src_alpha, 0,
					d_ptr,
					adbuf_ptr2, ptr->use_dst_alpha ? 0 : 1,
					bpp);
				if ( a ) ptr->blend2(
					&ptr->src_alpha, 0,
					&ptr->src_alpha, 0,
					a_ptr,
					a_ptr, ptr->use_dst_alpha ? 0 : 1,
					1
				);
			}
			d_ptr += bpp;
			if ( a ) a_ptr++;
			if ( !ptr-> use_dst_alpha ) adbuf_ptr2++;
		}

	NEXT_LINE:
		d += dls;
		if ( a ) a += als;
	}
	return true;
}

static Bool
img_bar_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	int bpp, als;
	unsigned int src_alpha = 0xff, dst_alpha = 0;
	Bool use_dst_alpha = false, solid;
	Byte blt_buffer[BLT_BUFSIZE], *adbuf, dst_alpha_mul = 255;
	int j, k, blt_bytes, blt_step = -1;

	if ( ctx->transparent && (memcmp( ctx->pattern, fillPatterns[fpEmpty], sizeof(FillPattern)) == 0))
		return true;

	/* align types and geometry - can only operate over imByte and imRGB */
	bpp = ( PImage(dest)->type & imGrayScale) ? imByte : imRGB;
	if (PImage(dest)-> type != bpp || ( kind_of( dest, CIcon) && PIcon(dest)->maskType != imbpp8 )) {
		Bool icon = kind_of(dest, CIcon), ok;
		int type = PImage(dest)->type;
		int mask = icon ? PIcon(dest)->maskType : 0;

		if ( type != bpp ) {
			img_resample_colors( dest, bpp, ctx );
			CIcon(dest)-> set_type( dest, bpp );
		}
		if ( icon && mask != imbpp8 )
			CIcon(dest)-> set_maskType( dest, imbpp8 );
		ok = img_bar_alpha( dest, x, y, w, h, ctx);
		if ( PImage(dest)-> options. optPreserveType ) {
			if ( type != bpp )
				CImage(dest)-> set_type( dest, type );
			if ( icon && mask != imbpp8 )
				CIcon(dest)-> set_maskType( dest, mask );
		}
		return ok;
	}

	/* differentiate between per-pixel alpha and a global value */
	if ( ctx->rop & ropSrcAlpha )
		src_alpha = (ctx->rop >> ropSrcAlphaShift) & 0xff;
	if ( ctx->rop & ropDstAlpha ) {
		use_dst_alpha = true;
		dst_alpha = (ctx->rop >> ropDstAlphaShift) & 0xff;
	}
	ctx->rop &= ropPorterDuffMask;
	if ( ctx->rop > ropMaxPDFunc || ctx->rop < 0 ) ctx->rop = ropSrcOver;

	/* assign pointers */
	bpp = ( bpp == imByte ) ? 1 : 3;
	if ( kind_of(dest, CIcon)) {
		als = PIcon(dest)-> maskLine;
		if ( PIcon(dest)-> maskType != imbpp8)
			croak("panic: assert failed for img_bar_alpha: %s", "dst mask type");
		if ( use_dst_alpha )
			dst_alpha_mul = dst_alpha;
		use_dst_alpha = false;
	} else {
		als = 0;
	}

	if ( !use_dst_alpha && als == 0) {
		use_dst_alpha = true;
		dst_alpha = 0xff;
	}
	if ( !(adbuf = malloc(use_dst_alpha ? 1 : (bpp * w)))) {
		warn("not enough memory");
		return false;
	}
	if ( use_dst_alpha ) adbuf[0] = dst_alpha;

	solid = (memcmp( ctx->pattern, fillPatterns[fpSolid], sizeof(FillPattern)) == 0);
	if ( solid || !ctx->transparent ) {
		/* render a (minimum) 8x8xPIXEL matrix with pattern, then
		replicate it over blt_buffer as much as possible, to streamline
		byte operations */
		blt_bytes = w * bpp;
		if ( blt_bytes < FILL_PATTERN_SIZE * bpp ) blt_bytes = FILL_PATTERN_SIZE * bpp;
		blt_bytes *= FILL_PATTERN_SIZE;
		blt_step = ((blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes) / FILL_PATTERN_SIZE;
		if ( bpp > 1 )
			blt_step = (blt_step / bpp / FILL_PATTERN_SIZE) * bpp * FILL_PATTERN_SIZE;
		for ( j = 0; j < FILL_PATTERN_SIZE; j++) {
			unsigned int pat, strip_size;
			Byte matrix[MAX_SIZEOF_PIXEL * FILL_PATTERN_SIZE], *buffer;
			if ( solid ) {
				pat = 0xff;
			} else {
				pat = (unsigned int) ctx->pattern[(j + FILL_PATTERN_SIZE - ctx->patternOffset. y) % FILL_PATTERN_SIZE];
				pat = (((pat << 8) | pat) >> ((ctx->patternOffset. x + 8 - (x % 8)) % FILL_PATTERN_SIZE)) & 0xff;
			}
			buffer = blt_buffer + j * blt_step;
			if ( bpp == 1 ) {
				strip_size = FILL_PATTERN_SIZE;
				for ( k = 0; k < FILL_PATTERN_SIZE; k++)
					matrix[k] = *((pat & (0x80 >> k)) ? ctx->color : ctx->backColor);
			} else {
				strip_size = FILL_PATTERN_SIZE * bpp;
				for ( k = 0; k < FILL_PATTERN_SIZE; k++) {
					Byte * color = (pat & (0x80 >> k)) ? ctx->color : ctx->backColor;
					memcpy( matrix + k * bpp, color, bpp);
				}
			}
			if ( strip_size > 1 ) {
				Byte * buf = buffer; 
				for ( k = 0; k < blt_step / strip_size; k++, buf += strip_size)
					memcpy( buf, matrix, strip_size);
				if ( blt_step % strip_size != 0)
					memcpy( buf, matrix, blt_step % strip_size);
			}
		}

		/*
		printf("bpp:%d step:%d\n", bpp, blt_step);
		for ( j = 0; j < 8; j++) {
			printf("%d: ", j);
			for ( k = 0; k < blt_step; k++) {
				printf("%02x", blt_buffer[ j * blt_step + k]);
			}
			printf("\n");
		} 
		*/
	}

	/* select function */
	{
		ImgBarAlphaCallbackRec rec = {
			/* bpp           */ bpp,
			/* als           */ als,
			/* dls           */ PImage(dest)-> lineSize,
			/* step          */ blt_step,
			/* pat_x_offset  */ x,
			/* dst           */ PImage(dest)->data,
			/* dstMask       */ (als > 0) ? PIcon(dest)->mask : NULL,
			/* pattern_buf   */ blt_buffer,
			/* adbuf         */ adbuf,
			/* use_dst_alpha */ use_dst_alpha,
			/* solid         */ solid,
			/* src_alpha     */ src_alpha,
			/* dst_alpha_mul */ dst_alpha_mul,
			/* ctx           */ ctx
		};
		img_find_blend_proc(ctx->rop, &rec.blend1, &rec.blend2);
		img_region_foreach( ctx->region, x, y, w, h,
			( RegionCallbackFunc *)((solid || !ctx->transparent) ?
				img_bar_alpha_single_opaque : img_bar_alpha_single_transparent),
			&rec
		);
	};

	free(adbuf);

	return true;
}

typedef struct {
	int bpp;
	int count;
	int ls;
	int step;
	int pat_x_offset;
	Bool solid;
	Byte * data;
	Byte * buf;
	PBitBltProc proc;
} ImgBarCallbackRec;

static Bool
img_bar_single( int x, int y, int w, int h, ImgBarCallbackRec * ptr)
{
	int j, blt_bytes, blt_step, offset;
	Byte lmask, rmask;
	Byte * data, *pat_ptr;

	switch ( ptr->bpp ) {
	case 1:
		blt_bytes = (( x + w - 1) >> 3) - (x >> 3) + 1;
		lmask = ( x & 7 ) ? 255 << ( 8 - (x & 7)) : 0;
		rmask = (( x + w) & 7 ) ? 255 >> ((x + w) & 7) : 0;
		offset = x >> 3;
		break;
	case 4:
		blt_bytes = (( x + w - 1) >> 1) - (x >> 1) + 1;
		lmask = ( x & 1 )       ? 0xf0 : 0;
		rmask = (( x + w) & 1 ) ? 0x0f : 0;
		offset = x >> 1;
		break;
	case 8:
		blt_bytes = w;
		lmask = rmask = 0;
		offset = x;
		break;
	default:
		blt_bytes = w * ptr->count;
		lmask = rmask = 0;
		offset = x * ptr->count;
	}

	blt_step = ptr->step;
	if (!ptr->solid && (( ptr-> pat_x_offset % FILL_PATTERN_SIZE ) != (x % FILL_PATTERN_SIZE))) {
		int dx = (x % FILL_PATTERN_SIZE) - ( ptr-> pat_x_offset % FILL_PATTERN_SIZE );
		if ( dx < 0 ) dx += FILL_PATTERN_SIZE;

		switch ( ptr->bpp ) {
		case 1:
			pat_ptr = ptr->buf;
			break;
		case 4:
			if ( dx > 1 ) {
				pat_ptr = ptr->buf + dx / 2;
				if ( dx > 0 || blt_step + FILL_PATTERN_SIZE / 2 > BLT_BUFSIZE )
					blt_step -= FILL_PATTERN_SIZE / 2;
			} else
				pat_ptr = ptr->buf;
			break;
		default:
			pat_ptr = ptr->buf + dx * ptr->bpp / 8;
			if ( dx > 0 || blt_step + FILL_PATTERN_SIZE * ptr->count > BLT_BUFSIZE )
				blt_step -= FILL_PATTERN_SIZE * ptr->count;
		}
	} else {
		pat_ptr = ptr->buf;
	}

	if (blt_bytes < blt_step) blt_step = blt_bytes;

	data = ptr->data + ptr->ls * y + offset;

	for ( j = 0; j < h; j++) {
		int bytes = blt_bytes;
		Byte lsave = *data, rsave = data[blt_bytes - 1], *p = data;
		Byte * src = pat_ptr + ((y + j) % FILL_PATTERN_SIZE) * ptr->step;
		while ( bytes > 0 ) {
			ptr->proc( src, p, ( bytes > blt_step ) ? blt_step : bytes );
			bytes -= blt_step;
			p += blt_step;
		}
		if ( lmask ) *data = (lsave & lmask) | (*data & ~lmask);
		if ( rmask ) data[blt_bytes-1] = (rsave & rmask) | (data[blt_bytes-1] & ~rmask);
		data += ptr->ls;
	}
	return true;
}

typedef struct {
	/* common stuff */
	PImage dest;
	PImgPaintContext ctx;

	/* non-alpha */
	Byte *colormap;
	BitBltProc* blt;

	/* alpha */
	int bpp;
	int src_mask_stride;
	int dst_mask_stride;
	Byte * src_mask;
	Byte * dst_mask;
	Bool use_src_alpha;
	Bool use_dst_alpha;
	Byte src_alpha_mul;
	Byte dst_alpha_mul;
	Byte * asbuf;
	Byte * adbuf;
	BlendFunc * blend1, * blend2;

	/* do not fill this */
	int src_x, src_y, orig_x, orig_y;
	unsigned int src_stride, dst_stride, bytes;
	Byte *src, *dst;

} TileCallbackRec;

typedef Bool TileCallbackFunc( int x, int y, int w, int h, TileCallbackRec* param);

static Bool
tile( int x, int y, int w, int h, TileCallbackFunc *tiler, TileCallbackRec* tx)
{
	PImage dest          = (PImage) tx->dest;
	PImage tile          = (PImage) tx->ctx->tile;
	Point offset         = tx->ctx->patternOffset;
	PBoxRegionRec region = tx->ctx->region;
	int dx, dy, tw = tile->w, th = tile->h, X2 = w + x, Y2 = h + y;

	tx->src_stride       = PImage(tile)-> lineSize;
	tx->dst_stride       = dest-> lineSize;
	tx->dst              = dest-> data;
	tx->bytes            = (dest->type & imBPP) / 8;
	for (
		dy = y - th + offset.y;
		dy < Y2;
		dy += th
	)
	for (
		dx = x - tw + offset.x;
		dx < X2;
		dx += tw
	) {
		int x1 = dx, y1 = dy, x2 = dx + tw - 1, y2 = dy + th - 1;
		tx->src_x = tx->src_y = 0;
		if ( x1 < x ) {
			tx->src_x += x - x1;
			x1 = x;
		}
		if ( y1 < y ) {
			tx->src_y += y - y1;
			y1 = y;
		}
		if ( x2 >= X2 ) x2 = X2 - 1;
		if ( y2 >= Y2 ) y2 = Y2 - 1;
		if ( x2 < x || y2 < y || x1 > w || y1 > h || x2 < x1 || y2 < y1 )
			continue;

		tx->src = tile->data + tx->src_y * tx->src_stride;
		tx->orig_x = x1;
		tx->orig_y = y1;
		if ( !img_region_foreach( region,
			x1, y1, x2 - x1 + 1, y2 - y1 + 1,
			(RegionCallbackFunc*) tiler, tx))
			return false;
	}

	return true;
}

/* strictly imBW stipple */

static int rop1_direct[16] = {
	ropOrPut,ropXorPut,ropNoOper,ropOrPut,
	ropNotSrcAnd,ropXorPut,ropNotSrcAnd,ropXorPut,
	ropOrPut,ropOrPut,ropNotSrcAnd,ropNoOper,
	ropNoOper,ropXorPut,ropNotSrcAnd,ropNoOper
};

static int rop1_inverse[16] = {
	ropNotSrcAnd,ropNoOper,ropNotSrcAnd,ropNoOper,
	ropOrPut,ropXorPut,ropNotSrcAnd,ropNotSrcAnd,
	ropXorPut,ropOrPut,ropNoOper,ropOrPut,
	ropXorPut,ropOrPut,ropXorPut,ropNoOper
};

static int ropX_step1[16] = {
	ropNotOr, ropXorPut, ropInvert, ropNotOr,
	ropNotSrcAnd, ropXorPut, ropNotSrcAnd, ropXorPut,
	ropNotOr, ropNotOr, ropNotSrcAnd, ropInvert,
	ropInvert, ropXorPut, ropNotSrcAnd, ropInvert
};
static int ropX_step2[16] = {
	ropNotDestAnd, ropNoOper, ropNotDestAnd, ropInvert,
	ropNotSrcOr, ropNotXor, ropAndPut, ropAndPut,
	ropXorPut, ropNotAnd, ropNoOper, ropNotAnd,
	ropXorPut, ropNotSrcOr, ropNotXor, ropInvert
};

static Bool
put1( int x, int y, int w, int h, TileCallbackRec* tx)
{
	int i;
	Byte * src = tx->src + ( y - tx->orig_y ) * tx->src_stride;
	Byte * dst = tx->dst + y * tx->dst_stride;
	for ( i = 0; i < h; i++) {
		bc_mono_put( src, tx->src_x + x - tx->orig_x, w, dst, x, tx-> blt);
		src += tx->src_stride;
		dst += tx->dst_stride;
	}
	return true;
}


static Bool
put4( int x, int y, int w, int h, TileCallbackRec* tx)
{
	int i;
	Byte * src = tx->src + ( y - tx->orig_y ) * tx->src_stride;
	Byte * dst = tx->dst + y * tx->dst_stride;
	for ( i = 0; i < h; i++) {
		bc_nibble_put( src, tx->src_x + x - tx->orig_x, w, dst, x, tx-> blt, tx->colormap);
		src += tx->src_stride;
		dst += tx->dst_stride;
	}
	return true;
}

static Bool
put8x( int x, int y, int w, int h, TileCallbackRec* tx)
{
	int i;
	Byte * src = tx->src + ( y - tx->orig_y ) * tx->src_stride + ( tx->src_x + x - tx->orig_x ) * tx->bytes;
	Byte * dst = tx->dst + y * tx->dst_stride + x * tx->bytes;
	w *= tx-> bytes;
	for ( i = 0; i < h; i++) {
		if ( tx->colormap )
			bc_byte_put( src, dst, w, tx->blt, tx->colormap);
		else
			tx->blt(src, dst, w);
		src += tx->src_stride;
		dst += tx->dst_stride;
	}
	return true;
}

typedef void MonoExpandFunc( Byte * source, Byte * dest, register unsigned int count, Byte * fore, Byte * back);

#define MONO_EXPANDER(type)                   \
static void                                   \
expand_##type(                                \
	Byte * source, Byte * dest,           \
	register unsigned int count,          \
	Byte * fore, Byte * back)             \
{                                             \
	bc_mono_##type(                       \
		source, dest, count,          \
		*((type*)fore), *((type*)back)\
	);                                    \
}

MONO_EXPANDER(float);
MONO_EXPANDER(double);
MONO_EXPANDER(Short);
MONO_EXPANDER(Long);

static void
expand_rgb(
	Byte * source, Byte * dest,
	register unsigned int count,
	Byte * fore, Byte * back)
{
	RGBColor pal[2] = {*((PRGBColor)back),*((PRGBColor)fore)};
	bc_mono_rgb( source, dest, count, pal);
}

static void
fill_mef( MonoExpandFunc * mef, Handle tile, Handle expanded_tile, Byte *fore, Byte *back)
{
	Byte * src = PImage(tile)->data;
	Byte * dst = PImage(expanded_tile)->data;
	unsigned int src_stride = PImage(tile)->lineSize;
	unsigned int dst_stride = PImage(expanded_tile)->lineSize;
	unsigned int y;
	for ( y = 0; y < PImage(tile)->h; y++) {
		mef( src, dst, PImage(tile)->w, fore, back);
		src += src_stride;
		dst += dst_stride;
	}
}

/* special case */
static Bool
img_bar_stipple_1bit( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	int rop;
	TileCallbackRec tx;

	bzero(&tx, sizeof(tx));
	tx.dest = (PImage)dest;
	tx.ctx  = ctx;
	rop     = ctx->transparent ?
		(( ctx->color[0] > 0 ) ? rop1_direct[ctx->rop] : rop1_inverse[ctx->rop]) :
		rop_1bit_transform( ctx->color[0] > 0, ctx->backColor[0] > 0, ctx-> rop)
		;
	tx.blt  = img_find_blt_proc(rop);
	return tile( x, y, w, h, put1, &tx);
}

static Bool
img_bar_stipple_generic( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	PImage i = (PImage) dest;
	TileCallbackRec tx;
	Byte colormap[256];
	Handle orig_tile = ctx->tile;
	MonoExpandFunc *mef = NULL;
	Bool ok;
	PImage t;
	TileCallbackFunc *tiler;

	bzero(&tx, sizeof(tx));
	tx.dest = (PImage)dest;
	tx.ctx  = ctx;

	if (( ctx->tile = CImage(ctx->tile)->dup(ctx->tile)) == NULL_HANDLE)
		return false;
	t = (PImage)ctx->tile;

	if ( t->type != imBW ) {
		warn("panic: bad tile type");
		return false;
	}
	t->type = imbpp1;

	t->palSize = 2;
	switch ( i-> type ) {
	case 4:
	case 4 | imGrayScale:
	case 8:
	case 8 | imGrayScale:
		t->palette[0] = i->palette[ ctx->backColor[0] ];
		t->palette[1] = i->palette[ ctx->color[0] ];
		CImage(t)->reset((Handle)t, i->type, i->palette, i->palSize);
		break;
	case 24:
		memcpy(t->palette + 1, ctx->color, 3);
		if ( ctx->transparent ) {
			bzero(t->palette, 3);
			mef = expand_rgb;
		} else
			memcpy(t->palette, ctx->backColor, 3);
		CImage(t)->reset((Handle)t, i->type, NULL, 0);
		break;
	case imFloat :
	case imDouble:
	case imShort :
	case imLong  :
		CImage(t)->reset((Handle)t, i->type, NULL, 0);
		break;
	default:
		Object_destroy((Handle) t);
		ctx-> tile = NULL_HANDLE;
		warn("Stippling for image type %x is not implemented yet", i->type);
		return false;
	}

	switch ( i-> type ) {
	case imFloat : mef = expand_float  ; break;
	case imDouble: mef = expand_double ; break;
	case imShort : mef = expand_Short  ; break;
	case imLong  : mef = expand_Long   ; break;
	}
	if (mef && i-> type != 24 ) { /* 24 is already converted */
		if ( ctx->transparent ) {
			Byte zero[MAX_SIZEOF_PIXEL];
			bzero(zero, MAX_SIZEOF_PIXEL);
			fill_mef( mef, orig_tile, ctx->tile, ctx->color, zero );
		} else
			fill_mef( mef, orig_tile, ctx->tile, ctx->color, ctx->backColor );
	}

	tiler = (( i-> type & imBPP ) == 4) ? put4 : put8x;

	if ( ctx->transparent ) {
		int k;
		/* see explanation in the similar code in apc_bar */
		for ( k = 0; k < 256; k++) colormap[k] = k;
		if (( i-> type & imBPP ) == 4) {
			colormap[ ctx->backColor[0] ] = 0;
			cm_colorref_4to8(colormap, colormap);
			tx.colormap = colormap;
		} else if (( i-> type & imBPP ) == 8) {
			colormap[ ctx->backColor[0] ] = 0;
			tx.colormap = colormap;
		}
		tx.blt = img_find_blt_proc(ropX_step1[ctx->rop]);
		if ( !( ok = tile( x, y, w, h, tiler, &tx)))
			goto FAIL;

		if (( i-> type & imBPP ) == 4) {
			colormap[ ctx->backColor[0] ] = 15;
			cm_colorref_4to8(colormap, colormap);
			tx.colormap = colormap;
		} else if (( i-> type & imBPP ) == 8) {
			colormap[ ctx->backColor[0] ] = 255;
			tx.colormap = colormap;
		} else if ( mef ) {
			Byte ones[MAX_SIZEOF_PIXEL];
			memset(ones, 0xff, MAX_SIZEOF_PIXEL);
			fill_mef( mef, orig_tile, ctx->tile, ctx->color, ones );
		} else {
			warn("panic: no encoder");
		}
		ctx->rop = ropX_step2[ctx->rop];
	}

	tx.blt = img_find_blt_proc(ctx->rop);
	ok = tile( x, y, w, h, tiler, &tx);

FAIL:
	Object_destroy((Handle) t);
	ctx-> tile = NULL_HANDLE;

	return ok;
}

static Bool
alpha_tiler( int x, int y, int w, int h, TileCallbackRec* ptr)
{
	int i;
	const int bpp = ptr->bpp;
	int bytes     = w * bpp;
	const Byte *s =
		ptr->src      + (y - ptr->orig_y) * ptr->src_stride + (ptr->src_x + x - ptr->orig_x) * bpp;
	const Byte *m = (ptr->src_mask_stride > 0) ?
		ptr->src_mask + (ptr->src_y + y - ptr->orig_y) * ptr->src_mask_stride + (ptr->src_x + x - ptr->orig_x)
		: NULL;
	Byte *d = ptr->dst + y * ptr->dst_stride + x * bpp;
	Byte *a = (ptr->dst_mask_stride > 0) ? ptr->dst_mask + y * ptr->dst_mask_stride + x : NULL;

	for ( i = 0; i < h; i++) {
		if ( !ptr->use_dst_alpha ) {
			img_fill_alpha_buf( ptr->adbuf, a, w, bpp);
			if ( ptr-> dst_alpha_mul < 255 )
				multiply( ptr->adbuf, &ptr->dst_alpha_mul, 0, ptr->adbuf, bytes);
		}

		/*
		printf("%02x%02x/%02x%02x + %02x%02x/%02x", s[0], s[1],
			ptr->use_src_alpha ? ptr->src_alpha_mul : m[0],
			ptr->use_src_alpha ? ptr->src_alpha_mul : m[1],
			d[0], d[1], ptr->adbuf[0]);
		*/
		ptr->blend1(
			s, 1,
			ptr->use_src_alpha ? &ptr->src_alpha_mul : m,
			ptr->use_src_alpha ? 0 : 1,
			d,
			ptr->adbuf,
			ptr->use_dst_alpha ? 0 : 1,
			bytes);
		/*
			printf("=> %02x%02x\n", d[0], d[1]);
		*/

		if (a != NULL) {
			if ( ptr->dst_alpha_mul < 255 )
				multiply( a, &ptr->dst_alpha_mul, 0, a, w);
			ptr->blend2(
				ptr->use_src_alpha ? &ptr->src_alpha_mul : m,
				ptr->use_src_alpha ? 0 : 1,
				ptr->use_src_alpha ? &ptr->src_alpha_mul : m,
				ptr->use_src_alpha ? 0 : 1,
				(Byte*)a,
				a, ptr->use_dst_alpha ? 0 : 1,
				w);
		}

		s += ptr->src_stride;
		d += ptr->dst_stride;
		if ( m ) m += ptr->src_mask_stride;
		if ( a ) a += ptr->dst_mask_stride;
	}
	return true;
}

static Bool
img_bar_tile_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx);

/* fix the tile to be either imByte or imRGB [ / imbpp8 ] */
static Bool
img_bar_tile_alpha_fix_src( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx, Bool src_is_icon, int bpp)
{
	Bool ok;
	PIcon t;
	if (( ctx->tile = CImage(ctx->tile)->dup(ctx->tile)) == NULL_HANDLE)
		return false;
	t = (PIcon) ctx->tile;
	if ((t->type & imBPP) != (bpp & imBPP))
		CIcon(ctx->tile)->set_type(ctx->tile, bpp);
	if (src_is_icon && t-> maskType != imbpp8)
		CIcon(ctx->tile)->set_maskType(ctx->tile, imbpp8);

	ok = img_bar_tile_alpha( dest, x, y, w, h, ctx);
	Object_destroy( ctx-> tile);
	ctx-> tile = NULL_HANDLE;
	return ok;
}

/* align types and geometry - can only operate over imByte and imRGB, and imbpp8 mask */
static Bool
img_bar_tile_alpha_fix_dest( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx, Bool dst_is_icon, int bpp)
{
	Bool ok;
	int type = PIcon(dest)->type;
	int mask = dst_is_icon ? PIcon(dest)->maskType : 0;
	if ( type != bpp )
		CIcon(dest)-> set_type( dest, bpp );
	if ( dst_is_icon && mask != imbpp8 )
		CIcon(dest)-> set_maskType( dest, imbpp8 );
	ok = img_bar_tile_alpha( dest, x, y, w, h, ctx);
	if ( PIcon(dest)-> options. optPreserveType ) {
		if ( type != bpp )
			CImage(dest)-> set_type( dest, type );
		if ( dst_is_icon && mask != imbpp8 )
			CIcon(dest)-> set_maskType( dest, mask );
	}
	return ok;
}

/* respect ropSrcAlpha for icons, adjust mask buffer */
static Byte*
img_fill_alpha( Handle tile, int bpp, int src_alpha )
{
	PIcon t = (PIcon) tile;
	register Byte *src, *dst, *res;
	register unsigned int len;

	if (( res = malloc( bpp * t-> maskSize )) == NULL)
		return NULL;

	src = t->mask;
	len = t->maskSize;
	dst = res;

	if ( bpp == 3 && src_alpha < 255 ) {
		while (len--) {
			register Byte a;
			a = (int)(*(src++)) * (int)src_alpha / 255.0 + .5;
			*(dst++) = a;
			*(dst++) = a;
			*(dst++) = a;
		}
	} else if ( bpp == 3 ) {
		while (len--) {
			register Byte a;
			a = *(src++);
			*(dst++) = a;
			*(dst++) = a;
			*(dst++) = a;
		}
	} else {
		while (len--)
			*(dst++) = (int)(*(src++)) * (int)src_alpha / 255.0 + .5;
	}

	return res;
}

static Bool
img_bar_tile_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	TileCallbackRec tx;
	PIcon i = (PIcon) dest;
	PIcon t = (PIcon) ctx->tile;
	unsigned int bpp, als, mls, bytes;
	unsigned int src_alpha = 0xff, dst_alpha = 0xff;
	Bool ok = false, src_is_icon, dst_is_icon, use_src_alpha = false, use_dst_alpha = false;
	Byte *asbuf = NULL, *adbuf = NULL, *ambuf = NULL;

	src_is_icon = kind_of( ctx-> tile, CIcon );
	dst_is_icon = kind_of( dest, CIcon );

	/* align types and geometry - can only operate over imByte and imRGB, and imbpp8 mask */
	bpp = ( i->type & imGrayScale) ? imByte : imRGB;
	if ( i->type != bpp || ( dst_is_icon && i->maskType != imbpp8 ))
		return img_bar_tile_alpha_fix_dest( dest, x, y, w, h, ctx, dst_is_icon, bpp);

	/* fix the tile */
	if (
		(t->type & imBPP) != (bpp & imBPP) ||
		( src_is_icon && t->maskType != imbpp8 )
	)
		return img_bar_tile_alpha_fix_src( dest, x, y, w, h, ctx, src_is_icon, bpp);

	bpp = ( bpp == imByte ) ? 1 : 3;

	/* differentiate between per-pixel alpha and a global value */
	if (ctx->rop & ropSrcAlpha)
		src_alpha = (ctx->rop >> ropSrcAlphaShift) & 0xff;
	if (ctx->rop & ropDstAlpha)
		dst_alpha = (ctx->rop >> ropDstAlphaShift) & 0xff;

	if ( src_is_icon ) {
		mls = t-> maskLine;
		if ( t-> maskType != imbpp8)
			croak("panic: assert failed for img_put_alpha: %s", "src mask type");
	} else {
		mls = 0;
		use_src_alpha = true;
	}

	if ( dst_is_icon ) {
		als = i-> maskLine;
		if ( i-> maskType != imbpp8)
			croak("panic: assert failed for img_bar_tile_alpha: %s", "dst mask type");
	} else {
		als = 0;
		use_dst_alpha = true;
	}

	/* respect ropSrcAlpha for icons, adjust mask buffer */
	if ( src_is_icon && ( bpp == 3 || src_alpha < 255 )) {
		if (( ambuf = img_fill_alpha( ctx->tile, bpp, src_alpha)) == NULL)
			goto FAIL;
		mls *= bpp;
	}

	ctx->rop &= ropPorterDuffMask;
	if ( ctx->rop > ropMaxPDFunc || ctx->rop < 0 ) ctx->rop = ropSrcOver;

	/* make buffers */
	bytes = w * bpp;
	if ( !(adbuf = malloc(use_dst_alpha ? 1 : bytes))) {
		warn("not enough memory");
		goto FAIL;
	}
	if ( use_dst_alpha ) adbuf[0] = dst_alpha;

	/* run */
	bzero(&tx, sizeof(tx));
	tx.dest            = (PImage)dest;
	tx.ctx             = ctx;
	tx.bpp             = bpp;
	tx.src_mask_stride = mls;
	tx.dst_mask_stride = als;
	tx.src_mask        = (mls > 0) ? ( ambuf ? ambuf : t->mask ) : NULL;
	tx.dst_mask        = (als > 0) ? i->mask : NULL;
	tx.use_src_alpha   = use_src_alpha;
	tx.use_dst_alpha   = use_dst_alpha;
	tx.src_alpha_mul   = src_alpha;
	tx.dst_alpha_mul   = dst_alpha;
	tx.asbuf           = asbuf;
	tx.adbuf           = adbuf;
	img_find_blend_proc(ctx->rop, &tx.blend1, &tx.blend2);
	ok = tile(x, y, w, h, alpha_tiler, &tx);

FAIL:
	if ( ambuf) free(ambuf);
	if ( adbuf) free(adbuf);
	if ( asbuf) free(asbuf);

	return ok;
}

Bool
img_bar_tile( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	PImage i = (PImage) dest;
	PImage t = (PImage)ctx->tile;
	TileCallbackRec tx;
	Byte colormap[256];
	Handle orig_tile = ctx->tile;
	Bool ok;
	TileCallbackFunc *tiler;

	if (kind_of(ctx->tile, CIcon)) {
		Image dummy;
		PIcon s = (PIcon) ctx->tile;
		if ( s-> maskType != imbpp1) {
			if ( s-> maskType != imbpp8) croak("panic: bad icon mask type");
			return img_bar_tile_alpha( dest, x, y, w, h, ctx);
		}
		img_fill_dummy( &dummy, s-> w, s-> h, imBW, s-> mask, stdmono_palette);
		ctx->rop  = ropAndPut;
		ctx->tile = (Handle) &dummy;
		img_bar_tile( dest, x, y, w, h, ctx);
		ctx->rop  = ropXorPut;
		ctx->tile = orig_tile;
	}

	bzero(&tx, sizeof(tx));
	tx.dest = (PImage)dest;
	tx.ctx  = ctx;

	if (( t->type & imBPP ) != ( i->type & imBPP )) {
		if (( ctx->tile = CImage(ctx->tile)->dup(ctx->tile)) == NULL_HANDLE)
			return false;
		CImage(ctx->tile)->reset(ctx-> tile, i->type, i->palette, i->palSize);
		t = (PImage) ctx->tile;
	}

	switch (i-> type & imBPP ) {
	case 1:
		tiler = put1;
		break;
	case 4:
		tiler = put4;
		break;
	default:
		tiler = put8x;
	}

	if (
		i->palSize != t->palSize ||
		memcmp( t->palette, i->palette, i->palSize * 3) != 0
	) {
		cm_fill_colorref(
			t-> palette, t-> palSize,
			i-> palette, i-> palSize,
			colormap);
		if (( PImage( dest)-> type & imBPP) == 4 )
			cm_colorref_4to8( colormap, colormap );
		tx.colormap = colormap;
	}

	tx.blt = img_find_blt_proc(ctx->rop);
	ok = tile( x, y, w, h, tiler, &tx);

	if ( ctx->tile != orig_tile ) {
		Object_destroy(ctx->tile);
		ctx->tile = NULL_HANDLE;
	}
	return ok;
}

Bool
img_bar_stipple_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	Bool ok;
	Handle tt = ctx->tile;
	PIcon t;
	PImage i;
	int bpp;

	i = (PImage) dest;

	/* create alpha channel */
	if ( ctx->transparent ) {
		Byte *src, *dst;
		int masklen;

		t = (PIcon) ctx->tile;
		ctx->tile = (Handle) create_object("Prima::Icon", "iiiii",
			"width",       t->w,
			"height",      t->h,
			"type",        imBW,
			"maskType",    1,
			"autoMasking", amNone
		);
		if ( ctx->tile == NULL_HANDLE)
			return false;
		t = (PIcon) ctx->tile;

		memcpy( t->data, PImage(tt)->data, t->dataSize);
		src     = PImage(tt)->data;
		dst     = t->mask;
		masklen = t->dataSize;
		while (masklen--) *(dst++) = ~*(src++);
		t->self->set_maskType(ctx->tile, imbpp8);

	} else {
		ctx->tile = CImage(ctx->tile)->dup(ctx->tile);
		if ( ctx->tile == NULL_HANDLE)
			return false;
		t = (PIcon) ctx->tile;
	}

	/* apply colors to pixels */
	bpp = ( i->type & imGrayScale) ? imByte : imRGB;
	if ( i->type != bpp )
		img_resample_colors( dest, bpp, ctx );

	if ( bpp == imByte ) {
		t->type = imbpp1;
		memset(t->palette + 1, ctx->color[0], 3);
		memset(t->palette, ctx->backColor[0], 3);
		t->self->set_type(ctx->tile, imByte);
	} else {
		memcpy(t->palette + 1, ctx->color, 3);
		memcpy(t->palette, ctx->backColor, 3);
		t->self->set_type(ctx->tile, imRGB);
	}

	ok = img_bar_tile_alpha( dest, x, y, w, h, ctx);

	Object_destroy(ctx->tile);
	ctx->tile = NULL_HANDLE;
	return ok;
}

Bool
img_bar( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	PImage i     = (PImage) dest;
	int pixSize  = (i->type & imBPP) / 8;
	Byte blt_buffer[BLT_BUFSIZE];
	int j, k, blt_bytes, blt_step;
	Bool solid;

	/* check boundaries */
	if ( ctx->rop == ropNoOper) return true;

	if ( x < 0 ) {
		w += x;
		x = 0;
	}
	if ( y < 0 ) {
		h += y;
		y = 0;
	}
	if ( x + w > i->w ) w = i->w - x;
	if ( y + h > i->h ) h = i->h - y;
	if ( w <= 0 || h <= 0 ) return true;

	if ( ctx-> tile ) {
		int W = PImage(ctx->tile)->w;
		int H = PImage(ctx->tile)->h;
		while ( ctx->patternOffset.x < 0 ) ctx-> patternOffset.x += W;
		while ( ctx->patternOffset.y < 0 ) ctx-> patternOffset.y += H;
		ctx-> patternOffset.x %= W;
		ctx-> patternOffset.y %= H;

		if ( PImage(ctx->tile)->type == imBW && !kind_of(ctx->tile, CIcon)) {
			if ( ctx-> rop & ropConstantAlpha)
				return img_bar_stipple_alpha( dest, x, y, w, h, ctx);
			else if (( i->type & imBPP ) == 1)
				return img_bar_stipple_1bit( dest, x, y, w, h, ctx);
			else
				return img_bar_stipple_generic( dest, x, y, w, h, ctx);
		} else {
			if ( ctx-> rop & ropConstantAlpha)
				return img_bar_tile_alpha( dest, x, y, w, h, ctx);
			else
				return img_bar_tile( dest, x, y, w, h, ctx);
		}
	}

	while ( ctx->patternOffset.x < 0 ) ctx-> patternOffset.x += FILL_PATTERN_SIZE;
	while ( ctx->patternOffset.y < 0 ) ctx-> patternOffset.y += FILL_PATTERN_SIZE;

	if ( ctx-> rop & ropConstantAlpha )
		return img_bar_alpha(dest, x, y, w, h, ctx);

	if ( memcmp( ctx->pattern, fillPatterns[fpSolid], sizeof(FillPattern)) == 0) {
		/* do nothing */
	} else if (memcmp( ctx->pattern, fillPatterns[fpEmpty], sizeof(FillPattern)) == 0) {
		if ( ctx->transparent ) return true;
		/* still do nothing */
	} else if ( ctx->transparent ) {
	/* transparent stippling: if rop is simple enough, adjust parameters to
	execute it as another rop with adjusted input. Otherwise make it into
	two-step operation, such as CopyPut stippling is famously executed by
	And and Xor rops */
		#define FILL(who,val) memset( ctx->who, val, MAX_SIZEOF_PIXEL)
		switch ( ctx-> rop ) {
		case ropBlackness:
			FILL(color,0x00);
			FILL(backColor,0xff);
			ctx->rop = ropAndPut;
			break;
		case ropWhiteness:
			FILL(color,0xff);
			FILL(backColor,0x00);
			ctx->rop = ropOrPut;
			break;
		case ropInvert:
			FILL(color,0xff);
			FILL(backColor,0x00);
			ctx->rop = ropXorPut;
			break;
		case ropNotSrcAnd:
		case ropXorPut:
			FILL(backColor,0x00);
			break;
		default: {
			int rop = ctx->rop;
			FILL(backColor,0x00);
			ctx->rop = ropX_step1[rop];
			ctx->transparent = false;
			img_bar( dest, x, y, w, h, ctx);
			FILL(backColor,0xff);
			ctx->rop = ropX_step2[rop];
			break;
		}}
	}

	/* render a 8x8xPIXEL matrix with pattern, then horizontally
	replicate it over blt_buffer as much as possible, to streamline
	byte operations */
	switch ( i->type & imBPP) {
	case imbpp1:
		blt_bytes = (( x + w - 1) >> 3) - (x >> 3) + 1;
		if ( blt_bytes < FILL_PATTERN_SIZE ) blt_bytes = FILL_PATTERN_SIZE;
		break;
	case imbpp4:
		blt_bytes = (( x + w - 1) >> 1) - (x >> 1) + 1;
		if ( blt_bytes < FILL_PATTERN_SIZE / 2 ) blt_bytes = FILL_PATTERN_SIZE / 2;
		break;
	default:
		blt_bytes = w * pixSize;
		if ( blt_bytes < FILL_PATTERN_SIZE * pixSize ) blt_bytes = FILL_PATTERN_SIZE * pixSize;
	}
	blt_bytes *= FILL_PATTERN_SIZE;
	blt_step = ((blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes) / FILL_PATTERN_SIZE;
	if ( pixSize > 1 )
		blt_step = (blt_step / pixSize / FILL_PATTERN_SIZE) * pixSize * FILL_PATTERN_SIZE;
	solid = (memcmp( ctx->pattern, fillPatterns[fpSolid], sizeof(FillPattern)) == 0);
	for ( j = 0; j < FILL_PATTERN_SIZE; j++) {
		unsigned int pat, strip_size;
		Byte matrix[MAX_SIZEOF_PIXEL * FILL_PATTERN_SIZE], *buffer;
		if ( solid ) {
			pat = 0xff;
		} else {
			pat = (unsigned int) ctx->pattern[(j + FILL_PATTERN_SIZE - ctx->patternOffset. y) % FILL_PATTERN_SIZE];
			pat = (((pat << 8) | pat) >> ((ctx->patternOffset. x + 8 - (x % 8)) % FILL_PATTERN_SIZE)) & 0xff;
		}
		buffer = blt_buffer + j * blt_step;
		switch ( i->type & imBPP) {
		case 1:
			strip_size = 1;
			matrix[0] = ctx->color[0] ? 
				(ctx->backColor[0] ? 0xff : pat) :
				(ctx->backColor[0] ? ~pat : 0);
			memset( buffer, matrix[0], blt_step);
			break;
		case 4: 
			strip_size = FILL_PATTERN_SIZE / 2;
			for ( k = 0; k < FILL_PATTERN_SIZE; ) {
				Byte c1 = *((pat & (0x80 >> k++)) ? ctx->color : ctx->backColor);
				Byte c2 = *((pat & (0x80 >> k++)) ? ctx->color : ctx->backColor);
				matrix[ (k / 2) - 1] = (c1 << 4) | (c2 & 0xf);
			}
			break;
		case 8: 
			strip_size = FILL_PATTERN_SIZE;
			for ( k = 0; k < FILL_PATTERN_SIZE; k++) {
				matrix[k] = *((pat & (0x80 >> k)) ? ctx->color : ctx->backColor);
			}
			break;
		default: 
			strip_size = FILL_PATTERN_SIZE * pixSize;
			for ( k = 0; k < FILL_PATTERN_SIZE; k++) {
				Byte * color = (pat & (0x80 >> k)) ? ctx->color : ctx->backColor;
				memcpy( matrix + k * pixSize, color, pixSize);
			}
		}
		if ( strip_size > 1 ) {
			Byte * buf = buffer; 
			for ( k = 0; k < blt_step / strip_size; k++, buf += strip_size)
				memcpy( buf, matrix, strip_size);
			if ( blt_step % strip_size != 0)
				memcpy( buf, matrix, blt_step % strip_size);
		}
	}
	/*
	printf("pxs:%d step:%d\n", pixSize, blt_step);
	for ( j = 0; j < 8; j++) {
		printf("%d: ", j);
		for ( k = 0; k < blt_step; k++) {
			printf("%02x", blt_buffer[ j * blt_step + k]);
		}
		printf("\n");
	} */
	{
		ImgBarCallbackRec rec = {
			/* bpp          */ (i->type & imBPP),
			/* count        */ (i->type & imBPP) / 8,
			/* ls           */ i->lineSize,
			/* step         */ blt_step,
			/* pat_x_offset */ x,
			/* solid        */ solid,
			/* data         */ i->data,
			/* buf          */ blt_buffer,
			/* proc         */ img_find_blt_proc(ctx->rop),
		};
		img_region_foreach( ctx->region,
			x, y, w, h,
			(RegionCallbackFunc*)img_bar_single, &rec
		);
	}

	return true;
}



#ifdef __cplusplus
}
#endif

