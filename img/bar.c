#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILL_PATTERN_SIZE 8
#define BLT_BUFSIZE ((MAX_SIZEOF_PIXEL * FILL_PATTERN_SIZE * FILL_PATTERN_SIZE) * 2)

typedef struct {
	int bpp, als, dls, step, pat_x_offset;
	Byte * dst, *dstMask, *pattern_buf, *adbuf;
	Bool use_dst_alpha, solid;
	Byte src_alpha;
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
	const Byte * a = (als > 0) ? ptr->dstMask + y * als + x : NULL;
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
	const Byte * a = (als > 0) ? ptr->dstMask + y * als + x : NULL;

	for ( i = 0; i < h; i++) {
		unsigned int pat;
		Byte *d_ptr, *a_ptr, *adbuf_ptr, *adbuf_ptr2;
		pat = (unsigned int) ptr->ctx->pattern[(i + ptr->ctx->patternOffset. y) % FILL_PATTERN_SIZE];
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
			if ( a ) ptr->blend2(
				&ptr->src_alpha, 0,
				&ptr->src_alpha, 0,
				(Byte*)a,
				a, ptr->use_dst_alpha ? 0 : 1,
				w
			);
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
	Byte blt_buffer[BLT_BUFSIZE], *adbuf;
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

	/* premultiply colors */
	for ( j = 0; j < bpp; j++) {
		ctx->color[j] = (float)(ctx->color[j] * src_alpha) / 255.0 + .5;
		ctx->backColor[j] = (float)(ctx->backColor[j] * src_alpha) / 255.0 + .5;
	}

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
				pat = (unsigned int) ctx->pattern[(j + ctx->patternOffset. y) % FILL_PATTERN_SIZE];
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
	PImage dest;
	PImgPaintContext ctx;
	BitBltProc* blt;
	int src_x, src_y;
	unsigned int src_stride, dst_stride;
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
	for (
		dy = y - offset.y;
		dy < Y2;
		dy += th
	)
	for (
		dx = x - offset.x;
		dx < X2;
		dx += tw
	) {
		int x1 = dx, y1 = dy, x2 = dx + tw - 1, y2 = dy + th - 1;
		tx->src_x = tx->src_y = 0;
		if ( x1 < 0 ) {
			tx->src_x -= x1;
			x1 = 0;
		}
		if ( y1 < 0 ) {
			tx->src_y -= y1;
			y1 = 0;
		}
		if ( x2 >= X2 ) x2 = X2 - 1;
		if ( y2 >= Y2 ) y2 = Y2 - 1;
		if ( x2 < 0 || y2 < 0 || x1 > w || y1 > h || x2 < x1 || y2 < y1 )
			continue;

		tx->src = tile->data + tx->src_y * tx->src_stride;
		if ( !img_region_foreach( region,
			x1, y1, x2 - x1 + 1, y2 - y1 + 1,
			(RegionCallbackFunc*) tiler, tx))
			return false;
	}

	return true;
}

static Bool
put1( int x, int y, int w, int h, TileCallbackRec* tx)
{
	int i;
	Byte * src = tx->src;
	Byte * dst = tx->dst + y * tx->dst_stride;
	for ( i = tx->src_y; i < h; i++) {
		bc_mono_put( src, tx->src_x, w, dst, x, tx-> blt);
		src += tx->src_stride;
		dst += tx->dst_stride;
	}
	return true;
}

/* strictly imBW stipple */

static int
rop1_direct(int rop)
{
	switch(rop) {
	case ropBlackness:
	case ropNotOr:
	case ropNotSrcAnd:
	case ropNotPut: return ropNotSrcAnd;
	case ropNotDestAnd:
	case ropInvert:
	case ropXorPut:
	case ropNotAnd: return ropXorPut;
	case ropAndPut:
	case ropNotXor:
	case ropNoOper:
	case ropNotSrcOr: return ropNoOper;
	case ropCopyPut:
	case ropNotDestOr:
	case ropOrPut:
	case ropWhiteness: return ropOrPut;
	}
	return ropNoOper;
}

static int
rop1_inverse(int rop)
{
	switch(rop) {
	case ropBlackness: return ropNotSrcAnd;
	case ropNotOr: return ropXorPut;
	case ropNotSrcAnd: return ropNoOper;
	case ropNotPut: return ropOrPut;
	case ropNotDestAnd: return ropNotSrcAnd;
	case ropInvert: return ropXorPut;
	case ropXorPut: return ropNoOper;
	case ropNotAnd: return ropOrPut;
	case ropAndPut: return ropNotSrcAnd;
	case ropNotXor: return ropXorPut;
	case ropNoOper: return ropNoOper;
	case ropNotSrcOr: return ropOrPut;
	case ropCopyPut: return ropNotSrcAnd;
	case ropNotDestOr: return ropXorPut;
	case ropOrPut: return ropNoOper;
	case ropWhiteness: return ropOrPut;
	}
	return ropNoOper;
}

Bool
img_bar_stipple( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	PImage i = (PImage) dest;
	TileCallbackRec tx = { (PImage)dest, ctx };

	if (( i->type & imBPP ) == 1) {
		/* special case */
		int rop = ctx->transparent ?
			(( ctx->color[0] > 0 ) ? rop1_direct(ctx->rop) : rop1_inverse(ctx->rop)) :
			rop_1bit_transform( ctx->color[0] > 0, ctx->backColor[0] > 0, ctx-> rop)
			;
		tx.blt = img_find_blt_proc(rop);
		return tile( x, y, w, h, put1, &tx);
	} else {
		/* general case */
		if ( ctx->transparent ) {
		} else {
		}
	}
	return false;
}

Bool
img_bar_tile( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	return false;
}

Bool
img_bar_stipple_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	return false;
}

Bool
img_bar_tile_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx)
{
	return false;
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
		if ( PImage(ctx->tile)->type == imBW && !kind_of(ctx->tile, CIcon)) {
			if ( ctx-> rop & ropConstantAlpha )
				return img_bar_stipple_alpha( dest, x, y, w, h, ctx);
			else
				return img_bar_stipple( dest, x, y, w, h, ctx);
		} else {
			if ( ctx-> rop & ropConstantAlpha )
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
			static int rop1[16] = {
				ropNotOr, ropXorPut, ropInvert, ropNotOr,
				ropNotSrcAnd, ropXorPut, ropNotSrcAnd, ropXorPut,
				ropNotOr, ropNotOr, ropNotSrcAnd, ropInvert,
				ropInvert, ropXorPut, ropNotSrcAnd, ropInvert
			};
			static int rop2[16] = {
				ropNotDestAnd, ropNoOper, ropNotDestAnd, ropInvert,
				ropNotSrcOr, ropNotXor, ropAndPut, ropAndPut,
				ropXorPut, ropNotAnd, ropNoOper, ropNotAnd,
				ropXorPut, ropNotSrcOr, ropNotXor, ropInvert
			};
			int rop = ctx->rop;
			FILL(backColor,0x00);
			ctx->rop = rop1[rop];
			ctx->transparent = false;
			img_bar( dest, x, y, w, h, ctx);
			FILL(backColor,0xff);
			ctx->rop = rop2[rop];
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
			pat = (unsigned int) ctx->pattern[(j + ctx->patternOffset. y) % FILL_PATTERN_SIZE];
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
			for ( k = 0; k < FILL_PATTERN_SIZE; k++)
				matrix[k] = *((pat & (0x80 >> k)) ? ctx->color : ctx->backColor);
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

