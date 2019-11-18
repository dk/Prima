#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void BitBltProc( Byte * src, Byte * dst, int count);
typedef BitBltProc *PBitBltProc;

static void
bitblt_copy( Byte * src, Byte * dst, int count)
{
	memcpy( dst, src, count);
}

static void
bitblt_move( Byte * src, Byte * dst, int count)
{
	memmove( dst, src, count);
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

static PBitBltProc
find_blt_proc( int rop )
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
	default:
		proc = bitblt_copy;
	}
	return proc;
}

static Bool
img_put_alpha( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop, PBoxRegionRec region);

typedef struct {
	int srcX;
	int srcY;
	int bpp;
	int srcLS;
	int dstLS;
	int dX;
	int dY;
	Byte * src;
	Byte * dst;
	PBitBltProc proc;
} ImgPutCallbackRec;

static Bool
img_put_single( int x, int y, int w, int h, ImgPutCallbackRec * ptr)
{
	int i, count;
	Byte *dptr, *sptr;
	sptr  = ptr->src + ptr->srcLS * (ptr->dY + y) + ptr->bpp * (ptr->dX + x);
	dptr  = ptr->dst + ptr->dstLS * y + ptr->bpp * x;
	count = w * ptr->bpp;
	for ( i = 0; i < h; i++, sptr += ptr->srcLS, dptr += ptr->dstLS)
		ptr->proc( sptr, dptr, count);
	return true;
}

Bool
img_put( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop, PBoxRegionRec region)
{
	Point srcSz, dstSz;
	int asrcW, asrcH;
	Bool newObject = false;

	if ( dest == nilHandle || src == nilHandle) return false;
	if ( rop == ropNoOper) return false;

	if ( kind_of( src, CIcon)) {
		/* since src is always treated as read-only,
			employ a nasty hack here, re-assigning
			all mask values to data */
		Byte * data  = PImage( src)-> data;
		int dataSize = PImage( src)-> dataSize;
		int lineSize = PImage( src)-> lineSize;
		int palSize  = PImage( src)-> palSize;
		int type     = PImage( src)-> type;
		void *self   = PImage( src)-> self;
		RGBColor palette[2];

		if ( PIcon(src)-> maskType != imbpp1) {
			if ( PIcon(src)-> maskType != imbpp8) croak("panic: bad icon mask type");
			return img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region);
		}

		PImage( src)-> self     =  CImage;
		PIcon( src)-> data = PIcon( src)-> mask;
		PImage( src)-> lineSize =  PIcon( src)-> maskLine;
		PImage( src)-> dataSize =  PIcon( src)-> maskSize;
		memcpy( palette, PImage( src)-> palette, 6);
		memcpy( PImage( src)-> palette, stdmono_palette, 6);


		PImage( src)-> type     =  imBW;
		PImage( src)-> palSize  = 2;
		img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropAndPut, region);
		rop = ropXorPut;
		memcpy( PImage( src)-> palette, palette, 6);

		PImage( src)-> self     = self;
		PImage( src)-> type     = type;
		PImage( src)-> data     = data;
		PImage( src)-> lineSize = lineSize;
		PImage( src)-> dataSize = dataSize;
		PImage( src)-> palSize  = palSize;
	} else if ( rop == ropAlphaCopy ) {
		Bool ok;
		Image dummy;
		PIcon i;
		if ( !kind_of( dest, CIcon )) return false;
		if ( PImage(src)-> type != imByte ) {
			Handle dup = CImage(src)->dup(src);
			CImage(dup)->set_type(src, imByte);
			ok = img_put( dest, dup, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region);
			Object_destroy(dup);
			return ok;
		}
		if ( PIcon(dest)-> maskType != imbpp8) {
			CIcon(dest)-> set_maskType(dest, imbpp8);
			ok = img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region);
			if ( PIcon(dest)-> options. optPreserveType )
				CIcon(dest)-> set_maskType(dest, imbpp1);
			return ok;
		}

		i = (PIcon) dest;
		img_fill_dummy( &dummy, i-> w, i-> h, imByte, i-> mask, NULL);
		return img_put((Handle)&dummy, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropCopyPut, region);
	} else if ( rop & ropConstantAlpha )
		return img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region);

	srcSz. x = PImage(src)-> w;
	srcSz. y = PImage(src)-> h;
	dstSz. x = PImage(dest)-> w;
	dstSz. y = PImage(dest)-> h;

	if ( dstW < 0) {
		dstW = abs( dstW);
		srcW = -srcW;
	}
	if ( dstH < 0) {
		dstH = abs( dstH);
		srcH = -srcH;
	}

	asrcW = abs( srcW);
	asrcH = abs( srcH);

	if (
		srcX >= srcSz. x || srcX + srcW <= 0 ||
		srcY >= srcSz. y || srcY + srcH <= 0 ||
		dstX >= dstSz. x || dstX + dstW <= 0 ||
		dstY >= dstSz. y || dstY + dstH <= 0
	)
		return true;

	/* check if we can do it without expensive scalings and extractions */
	if (
		( srcW == dstW) && ( srcH == dstH) &&
		( srcX >= 0) && ( srcY >= 0) && ( srcX + srcW <= srcSz. x) && ( srcY + srcH <= srcSz. y)
	)
		goto NOSCALE;

	if ( srcX != 0 || srcY != 0 || asrcW != srcSz. x || asrcH != srcSz. y) {
	/* extract source rectangle */
		Handle x;
		int ssx = srcX, ssy = srcY, ssw = asrcW, ssh = asrcH;
		if ( ssx < 0) {
			ssw += ssx;
			ssx = 0;
		}
		if ( ssy < 0) {
			ssh += ssy;
			ssy = 0;
		}
		x = CImage( src)-> extract( src, ssx, ssy, ssw, ssh);
		if ( !x) return false;

		if ( srcX < 0 || srcY < 0 || srcX + asrcW >= srcSz. x || srcY + asrcH > srcSz. y) {
			HV * profile;
			Handle dx;
			int dsx = 0, dsy = 0, dsw = PImage(x)-> w, dsh = PImage(x)-> h, type = PImage( dest)-> type;

			if ( asrcW != srcW || asrcH != srcH) { /* reverse before application */
				CImage( x)-> stretch( x, srcW, srcH);
				srcW = asrcW;
				srcH = asrcH;
				if ( PImage(x)-> w != asrcW || PImage(x)-> h != asrcH) {
					Object_destroy( x);
					return true;
				}
			}

			if (( type & imBPP) < 8) type = imbpp8;

			profile = newHV();
			pset_i( type,        type);
			pset_i( width,       asrcW);
			pset_i( height,      asrcH);
			pset_i( conversion,  PImage( src)-> conversion);
			dx = Object_create( "Prima::Image", profile);
			sv_free((SV*)profile);
			if ( !dx) {
				Object_destroy( x);
				return false;
			}
			if ( PImage( dx)-> palSize > 0) {
				PImage( dx)-> palSize = PImage( x)-> palSize;
				memcpy( PImage( dx)-> palette, PImage( x)-> palette, 768);
			}
			memset( PImage( dx)-> data, 0, PImage( dx)-> dataSize);

			if ( srcX < 0) dsx = asrcW - dsw;
			if ( srcY < 0) dsy = asrcH - dsh;
			img_put( dx, x, dsx, dsy, 0, 0, dsw, dsh, dsw, dsh, ropCopyPut, region);
			Object_destroy( x);
			x = dx;
		}

		src = x;
		newObject = true;
		srcX = srcY = 0;
	}

	if ( srcW != dstW || srcH != dstH) {
		/* stretch & reverse */
		if ( !newObject) {
			src = CImage( src)-> dup( src);
			if ( !src) goto EXIT;
			newObject = true;
		}
		if ( srcW != asrcW) {
			dstW = -dstW;
			srcW = asrcW;
		}
		if ( srcH != asrcH) {
			dstH = -dstH;
			srcH = asrcH;
		}
		CImage(src)-> stretch( src, dstW, dstH);
		if ( PImage(src)-> w != dstW || PImage(src)-> h != dstH) goto EXIT;
		dstW = abs( dstW);
		dstH = abs( dstH);
	}

NOSCALE:

	if (( PImage( dest)-> type & imBPP) < 8) {
		PImage i = ( PImage) dest;
		int type = i-> type;
		if (rop != ropCopyPut || i-> conversion == ictNone) {
			Handle b8 = i-> self-> dup( dest);
			PImage j  = ( PImage) b8;
			int mask  = (1 << (type & imBPP)) - 1;
			int sz;
			Byte *dj, *di;
			Byte colorref[256];
			j-> self-> reset( b8, imbpp8, nil, 0);
			sz = j-> dataSize;
			dj = j-> data;
			/* change 0/1 to 0x000/0xfff for correct masking */
			while ( sz--) {
				if ( *dj == mask) *dj = 0xff;
				dj++;
			}
			img_put( b8, src, dstX, dstY, 0, 0, dstW, dstH, PImage(src)-> w, PImage(src)-> h, rop, region);
			for ( sz = 0; sz < 256; sz++) colorref[sz] = ( sz > mask) ? mask : sz;
			dj = j-> data;
			di = i-> data;

			for ( sz = 0; sz < i-> h; sz++, dj += j-> lineSize, di += i-> lineSize) {
				if (( type & imBPP) == 1)
					bc_byte_mono_cr( dj, di, i-> w, colorref);
				else
					bc_byte_nibble_cr( dj, di, i-> w, colorref);
			}
			Object_destroy( b8);
		} else {
			int conv = i-> conversion;
			i-> conversion = PImage( src)-> conversion;
			i-> self-> reset( dest, imbpp8, nil, 0);
			img_put( dest, src, dstX, dstY, 0, 0, dstW, dstH, PImage(src)-> w, PImage(src)-> h, rop, region);
			i-> self-> reset( dest, type, nil, 0);
			i-> conversion = conv;
		}
		goto EXIT;
	}

	if ( PImage( dest)-> type != PImage( src)-> type) {
		int type = PImage( src)-> type & imBPP;
		int mask = (1 << type) - 1;
		/* equalize type */
		if ( !newObject) {
			src = CImage( src)-> dup( src);
			if ( !src) goto EXIT;
			newObject = true;
		}
		CImage( src)-> reset( src, PImage( dest)-> type, nil, 0);
		if ( type < 8 && rop != ropCopyPut) {
			/* change 0/1 to 0x000/0xfff for correct masking */
			int sz   = PImage( src)-> dataSize;
			Byte * d = PImage( src)-> data;
			while ( sz--) {
				if ( *d == mask) *d = 0xff;
				d++;
			}
			memset( PImage( src)-> palette + 255, 0xff, sizeof(RGBColor));
		}
	}

	if ( PImage( dest)-> type == imbpp8) {
		/* equalize palette */
		Byte colorref[256], *s;
		int sz, i = PImage( src)-> dataSize;
		if ( !newObject) {
			src = CImage( src)-> dup( src);
			if ( !src) goto EXIT;
			newObject = true;
		}
		cm_fill_colorref(
			PImage( src)-> palette, PImage( src)-> palSize,
			PImage( dest)-> palette, PImage( dest)-> palSize,
			colorref);
		s = PImage( src)-> data;
		/* identity transform for padded ( 1->xfff, see above ) pixels */
		for ( sz = PImage( src)-> palSize; sz < 256; sz++)
			colorref[sz] = sz;
		while ( i--) {
			*s = colorref[ *s];
			s++;
		}
	}

	if ( dstX < 0 || dstY < 0 || dstX + dstW >= dstSz. x || dstY + dstH >= dstSz. y) {
		/* adjust destination rectangle */
		if ( dstX < 0) {
			dstW += dstX;
			srcX -= dstX;
			dstX = 0;
		}
		if ( dstY < 0) {
			dstH += dstY;
			srcY -= dstY;
			dstY = 0;
		}
		if ( dstX + dstW > dstSz. x)
			dstW = dstSz. x - dstX;
		if ( dstY + dstH > dstSz. y)
			dstH = dstSz. y - dstY;
	}

	/* checks done, do put_image */
	{
		ImgPutCallbackRec rec = {
			/* srcX  */ srcX,
			/* srcY  */ srcY,
			/* bpp   */ ( PImage( dest)-> type & imBPP ) / 8,
			/* srcLS */ PImage( src)-> lineSize,
			/* dstLS */ PImage( dest)-> lineSize,
			/* dX    */ srcX - dstX,
			/* dY    */ srcY - dstY,
			/* src   */ PImage( src )-> data,
			/* dst   */ PImage( dest )-> data,
			/* proc  */ find_blt_proc(rop)
		};
		if ( rec.proc == bitblt_copy && dest == src) /* incredible */
			rec.proc = bitblt_move;
		img_region_foreach( region,
			dstX, dstY, dstW, dstH,
			(RegionCallbackFunc*)img_put_single, &rec
		);
	}

EXIT:
	if ( newObject) Object_destroy( src);

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

#define FILL_PATTERN_SIZE 8
#define BLT_BUFSIZE ((MAX_SIZEOF_PIXEL * FILL_PATTERN_SIZE * (FILL_PATTERN_SIZE / 8) * 2 * 4))

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

	blt_step = (blt_bytes > ptr->step) ? ptr->step : blt_bytes;
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
				if ( blt_step + FILL_PATTERN_SIZE / 2 > BLT_BUFSIZE )
					blt_step -= FILL_PATTERN_SIZE / 2;
			} else
				pat_ptr = ptr->buf;
			break;
		default:
			pat_ptr = ptr->buf + dx * ptr->bpp / 8;
			if ( blt_step + FILL_PATTERN_SIZE * ptr->count > BLT_BUFSIZE )
				blt_step -= FILL_PATTERN_SIZE * ptr->count;
		}
	} else
		pat_ptr = ptr->buf;

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

static Bool
img_bar_alpha( Handle dest, int x, int y, int w, int h, PImgPaintContext ctx);

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
			/* proc         */ find_blt_proc(ctx->rop),
		};
		img_region_foreach( ctx->region,
			x, y, w, h,
			(RegionCallbackFunc*)img_bar_single, &rec
		);
	}

	return true;
}

/* reformat color values to imByte/imRGB */
static Bool
resample_colors( Handle dest, int bpp, PImgPaintContext ctx)
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
		*((Color*)ctx->color)     = ARGB(fg.r,fg.g,fg.b);
		*((Color*)ctx->backColor) = ARGB(bg.r,bg.g,bg.b);
	}
	return true;
}

#define VISIBILITY_NONE       0
#define VISIBILITY_CLIPPED    1
#define VISIBILITY_UNSURE     2
#define VISIBILITY_CLEAR      3

static void
fill_alpha_buf( Byte * dst, Byte * src, int width, int bpp)
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

#define dBLEND_FUNC(name) void name( \
	const Byte * src, const Byte src_inc, \
	const Byte * src_a, const Byte src_a_inc,\
	Byte * dst, \
	const Byte * dst_a, const Byte dst_a_inc,\
	int bytes)

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

typedef dBLEND_FUNC(BlendFunc);

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

dBLEND_FUNCx(blend_src_over,  UP(S) + UP(D) * INVSA / 255)
dBLEND_FUNCx(blend_xor,      (UP(S) * INVDA + UP(D) * INVSA) / 255)
dBLEND_FUNCx(blend_dst_over,  UP(D) + UP(S) * INVDA / 255)
dBLEND_FUNCx(blend_src_in,    UP(S) * DA / 255)
dBLEND_FUNCx(blend_dst_in,    UP(D) * SA / 255)
dBLEND_FUNCx(blend_src_out,   UP(S) * INVDA / 255)
dBLEND_FUNCx(blend_dst_out,   UP(D) * INVSA / 255)
dBLEND_FUNCx(blend_src_atop, (UP(S) * DA + UP(D) * INVSA) / 255)
dBLEND_FUNCx(blend_dst_atop, (UP(D) * SA + UP(S) * INVDA) / 255)

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
	blend_src_over,
	blend_xor,
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

static void
find_blend_proc( int rop, BlendFunc ** blend1, BlendFunc ** blend2 )
{
	*blend1 = blend_functions[rop];
	*blend2 = (rop >= ropMultiply) ? blend_functions[ropScreen] : blend_functions[rop];
}

typedef struct {
	PIcon       i;
	PBitBltProc proc;
	BlendFunc  *blend1, *blend2;
	int         bpp, bytes, optimized_stride;
	PImgPaintContext ctx;
	Byte        *color;

	Bool        use_dst_alpha, is_icon;
	Byte        src_alpha,dst_alpha;
} ImgHLineRec;

static void
setpixel( ImgHLineRec* rec, int x, int y)
{
	switch ( rec->bpp ) {
	case 1: {
		Byte * dst = rec->i->data + rec->i->lineSize * y + x / 8, src = *dst;
		Byte shift = 7 - (x & 7);
		src = (src >> shift) & 1;
		rec->proc( rec->color, &src, 1);
		if ( src & 1 )
			*dst |= 1 << shift;
		else
			*dst &= ~(1 << shift);
		break;
	}
	case 4: {
		Byte * dst = rec->i->data + rec->i->lineSize * y + x / 2, src = *dst, tmp = *dst;
		if ( x & 1 ) {
			rec->proc( rec->color, &src, 1);
			*dst = (tmp & 0xf0) | (src & 0x0f);
		} else {
			src >>= 4;
			rec->proc( rec->color, &src, 1);
			*dst = (tmp & 0x0f) | (src << 4);
		}
		break;
	}
	case 8:
		if ( rec->proc)
			rec->proc( rec->color, rec->i->data + rec->i->lineSize * y + x, 1);
		else
			rec->blend1(
				rec-> color, 0, &rec->src_alpha, 0,
				rec->i->data + rec->i->lineSize * y + x,
				rec->use_dst_alpha ? 
					&rec->dst_alpha : (rec->i->mask + rec->i->maskLine * y + x), 0,
				1
			);
		break;
	default:
		if ( rec->proc)
			rec->proc( rec->color, rec->i->data + rec->i->lineSize * y + x * rec->bytes, rec->bytes);
		else
			rec->blend1(
				rec-> color, 0, &rec->src_alpha, 0,
				rec->i->data + rec->i->lineSize * y + x * rec->bytes,
				rec->use_dst_alpha ? 
					&rec->dst_alpha : (rec->i->mask + rec->i->maskLine * y + x), 0,
				rec->bytes
			);
	}

	if ( rec->blend2 && rec->is_icon ) {
		Byte * a = rec->i->mask + rec->i->maskLine * y + x;
		rec->blend2( &rec->src_alpha, 0, &rec->src_alpha, 0, (Byte*)a, a, 0, 1);
	}
}

static void
hline( ImgHLineRec *rec, int x, int n, int y)
{
	switch ( rec->bpp) {
	case 8:
	case 24: {
		/* optimized multipixel set */
		int wn;
		int w = rec->bytes, stride = rec->optimized_stride;
		Byte * dst = rec->i->data + rec->i->lineSize * y + x * w;
		Byte * mask = ( rec->blend1 && !rec->use_dst_alpha) ?
			(rec->i->mask + rec->i->maskLine * y + x) : NULL;
		for ( wn = w * n; wn > 0; wn -= stride, dst += stride) {
			int dw = ( wn >= stride ) ? stride : wn;
			if ( rec->proc )
				rec->proc( rec->color, dst, dw);
			else {
				Byte mask_buf[MAX_SIZEOF_PIXEL];
				if ( mask ) {
					int bp = rec->bpp / 8;
					int dm = dw / bp;
					fill_alpha_buf( mask_buf, mask, dm, bp);
					rec->blend2(
						&rec->src_alpha, 0,
						&rec->src_alpha, 0,
						mask, mask, 1, dm);
					mask += dm;
				}
				rec->blend1( rec->color, 1,
					&rec->src_alpha, 0,
					dst,
					mask ? mask_buf : &rec->dst_alpha,
					rec->use_dst_alpha ? 0 : 1,
					dw);
			}
		}
		return;
	}
	default: {
		int i;
		for ( i = 0; i < n; i++, x++) setpixel(rec, x, y);
	}}
}

#define HLINE_INIT_OK    0
#define HLINE_INIT_FAIL  1
#define HLINE_INIT_RETRY 2

/* prepare to draw horizontal lines with alpha etc using single solid color */
static int
hline_init( ImgHLineRec * rec, Handle dest, PImgPaintContext ctx, char * method)
{
	int i;

	/* misc */
	rec->ctx     = ctx;
	rec->i       = (PIcon) dest;
	rec->bpp     = rec->i->type & imBPP;
	rec->bytes   = rec->bpp / 8;

	/* deal with alpha request */
	if ( ctx-> rop & ropConstantAlpha ) {
		int j, rop = ctx->rop;
		/* differentiate between per-pixel alpha and a global value */
		if ( ctx->rop & ropSrcAlpha )
			rec->src_alpha = (rop >> ropSrcAlphaShift) & 0xff;
		else
			rec->src_alpha = 0xff;
		if ( rop & ropDstAlpha ) {
			rec->use_dst_alpha = true;
			rec->dst_alpha = (rop >> ropDstAlphaShift) & 0xff;
		}
		rop &= ropPorterDuffMask;
		if ( rop > ropMaxPDFunc || rop < 0 ) return false;
		find_blend_proc( rop, &rec->blend1, &rec->blend2 );
		rec->is_icon = kind_of( dest, CIcon );

		/* align types and geometry - can only operate over imByte and imRGB */
		int bpp = ( PImage(dest)->type & imGrayScale) ? imByte : imRGB;
		if (PImage(dest)-> type != bpp || ( rec->is_icon && PIcon(dest)->maskType != imbpp8 )) {
			int type = PImage(dest)->type;
			int mask = rec->is_icon ? PIcon(dest)->maskType : 0;

			if ( type != bpp ) {
				resample_colors( dest, bpp, ctx );
				CIcon(dest)-> set_type( dest, bpp );
				if ( PImage(dest)->type != bpp)
					return HLINE_INIT_FAIL;
			}
			if ( rec->is_icon && mask != imbpp8 ) {
				CIcon(dest)-> set_maskType( dest, imbpp8 );
				if ( PIcon(dest)->maskType != imbpp8)
					return HLINE_INIT_FAIL;
			}
			return HLINE_INIT_RETRY;
		}

		if ( rec->is_icon ) {
			if ( PIcon(dest)-> maskType != imbpp8)
				croak("panic: assert failed for %s: %s", method, "dst mask type");
			rec->use_dst_alpha = false;
		} else if ( !rec->use_dst_alpha ) {
			rec->use_dst_alpha = true;
			rec->dst_alpha = 0xff;
		}
		rec->proc = NULL;

		/* premultiply colors */
		for ( j = 0; j < bpp / 8; j++) {
			ctx->color[j] = (float)(ctx->color[j] * 255.0) / rec->src_alpha + .5;
			ctx->backColor[j] = (float)(ctx->backColor[j] * 255.0) / rec->src_alpha + .5;
		}
	} else {
		rec->blend1 = rec->blend2 = NULL;
		rec->proc = find_blt_proc(ctx->rop);
	}

	rec->color   = ctx->color;
	
	/* colors; optimize 8 and 24 pixels for horizontal line memcpy */
	switch ( rec->bpp ) {
	case 8:
		memset( ctx->color + 1, ctx->color[0], MAX_SIZEOF_PIXEL - 1);
		rec->optimized_stride = MAX_SIZEOF_PIXEL;
		break;
	case 24: 
		for ( i = 1; i < MAX_SIZEOF_PIXEL / 3; i++)
			memcpy( ctx->color + i * 3, ctx->color, 3);
		rec->optimized_stride = (MAX_SIZEOF_PIXEL / 3) * 3;
	}

	return HLINE_INIT_OK;
}

typedef struct {
	ImgHLineRec h;
	Bool        solid, segment_is_fg, skip_pixel;
	int         current_segment, segment_offset, n_segments;
} ImgSegmentedLineRec;

static void
segmented_hline( ImgSegmentedLineRec *rec, int x1, int x2, int y, int visibility)
{
	int n  = abs(x2 - x1) + 1;
	int dx = (x1 < x2) ? 1 : -1;
	/* printf("(%d,%d)->%d %d\n", x1, y, x2,visibility); */
	if ( rec->skip_pixel ) {
		rec->skip_pixel = false;
		if ( n-- == 1 ) return;
		x1 += dx;
	}
	if ( rec->solid) {
		if ( visibility == VISIBILITY_CLEAR ) {
			if ( x2 < x1 )
				hline( &rec->h, x2, x1 - x2 + 1, y);
			else
				hline( &rec->h, x1, x2 - x1 + 1, y);
		} else {
			/* VISIBILITY_NONE is not reaching here */
			int i;
			for ( i = 0; i < n; i++, x1 += dx)
				if ( img_point_in_region(x1, y, rec->h.ctx->region))
					setpixel(&rec->h, x1, y);
		}
	} else {
		int i;
		for ( i = 0; i < n; i++, x1 += dx) {
			/* calculate color */
			rec->h.color = rec->segment_is_fg ?
				rec->h.ctx->color : 
				( rec->h.ctx->transparent ? NULL : rec->h.ctx->backColor )
				;
			if ( ++rec->segment_offset >= rec->h.ctx->linePattern[rec->current_segment]) {
				rec->segment_offset = 0;
				if ( ++rec->current_segment >= rec->n_segments ) {
					rec->current_segment = 0;
					rec->segment_is_fg = true;
				} else {
					rec->segment_is_fg = !rec->segment_is_fg;
				}
			}

			/* put pixel */
			if (
				(visibility > VISIBILITY_NONE) &&
				(rec->h.color != NULL) &&
				(
					(visibility == VISIBILITY_CLEAR) || 
					img_point_in_region(x1, y, rec->h.ctx->region)
				)
			)
				setpixel(&rec->h, x1, y);
		}
	}
}

Bool
img_polyline( Handle dest, int n_points, Point * points, PImgPaintContext ctx)
{
	PIcon i = (PIcon) dest;
	int j;
	int type     = i->type;
	int maskType = kind_of(dest, CIcon) ? i->maskType : 0;
	ImgSegmentedLineRec rec;
	BoxRegionRec dummy_region;
	Box dummy_region_box, *pbox;
	Point* pp;
	Rect  enclosure;
	Bool closed;

	if ( ctx->rop == ropNoOper || n_points <= 1) return true;

	switch ( hline_init( &rec.h, dest, ctx, "img_polyline")) {
	case HLINE_INIT_RETRY: {
		Bool ok;
		ok = img_polyline( dest, n_points, points, ctx);
		if ( i-> options. optPreserveType ) {
			if ( type != i->type )
				CImage(dest)-> set_type( dest, type );
			if ( maskType != 0 && maskType != i->maskType )
				CIcon(dest)-> set_maskType( dest, maskType );
		}
		return ok;
	}
	case HLINE_INIT_FAIL:
		return false;
	}

	rec.solid   = (strcmp((const char*)ctx->linePattern, (const char*)lpSolid) == 0);
	if ( *(ctx->linePattern) == 0) {
		if ( ctx->transparent ) return true;
		rec.solid = true;
		memcpy( ctx->color, ctx->backColor, MAX_SIZEOF_PIXEL);
	}

	if ( rec.solid )
		rec.h.color = ctx->color;

	/* patterns */
	rec.n_segments       = strlen(( const char*) ctx->linePattern );
	rec.current_segment  = 0;
	rec.segment_offset   = 0;
	rec.segment_is_fg    = 1;
	if ( ctx->region == NULL ) {
		dummy_region.n_boxes = 1;
		dummy_region.boxes = &dummy_region_box;
		dummy_region_box.x = 0;
		dummy_region_box.y = 0;
		dummy_region_box.width  = i->w;
		dummy_region_box.height = i->h;
		ctx->region = &dummy_region;
	}
	enclosure.left   = ctx->region->boxes[0].x;
	enclosure.bottom = ctx->region->boxes[0].y;
	enclosure.right  = ctx->region->boxes[0].x + ctx->region->boxes[0].width  - 1;
	enclosure.top    = ctx->region->boxes[0].y + ctx->region->boxes[0].height - 1;
	for ( j = 1, pbox = ctx->region->boxes + 1; j < ctx->region->n_boxes; j++, pbox++) {
		int right = pbox->x + pbox->width - 1;
		int top   = pbox->y + pbox->height - 1;
		if ( enclosure.left   > pbox->x ) enclosure.left   = pbox->x;
		if ( enclosure.bottom > pbox->y ) enclosure.bottom = pbox->y;
		if ( enclosure.right  < right   ) enclosure.right  = right;
		if ( enclosure.top    < top     ) enclosure.top    = top;
	}

	closed  = points[0].x == points[n_points-1].x && points[0].y == points[n_points-1].y && n_points > 2;
	for ( j = 0, pp = points; j < n_points - 1; j++, pp++) {
		/* calculate clipping: -1 invisible, 0 definitely clipped, 1 possibly clipped */
		int visibility; 
		int curr_maj, curr_min, to_maj, delta_maj, delta_min;
		int delta_y, delta_x;
		int dir = 0, d, d_inc1, d_inc2;
		int inc_maj, inc_min;
		int x, y, acc_x = 0, acc_y = INT_MIN, ox;
		Point a, b;

		/* printf("* p(%d): (%d,%d)-(%d,%d)\n", j, pp[0].x, pp[0].y, pp[1].x, pp[1].y); */
		a.x = pp[0].x + ctx->translate.x;
		a.y = pp[0].y + ctx->translate.y;
		b.x = pp[1].x + ctx->translate.x;
		b.y = pp[1].y + ctx->translate.y;
		if (a.x == b.x && a.y == b.y && n_points > 2) continue;

		if (
			( a.x < enclosure.left   && b.x < enclosure.left) ||
			( a.x > enclosure.right  && b.x > enclosure.right) ||
			( a.y < enclosure.bottom && b.y < enclosure.bottom) ||
			( a.y > enclosure.top    && b.y > enclosure.top)
		) {
			visibility = VISIBILITY_NONE;
			if ( rec.solid ) continue;
		} else if (
			a.x >= enclosure.left   && b.x >= enclosure.left &&
			a.x <= enclosure.right  && b.x <= enclosure.right &&
			a.y >= enclosure.bottom && b.y >= enclosure.bottom &&
			a.y <= enclosure.top    && b.y <= enclosure.top
		) {
			if ( ctx->region->n_boxes > 1) {
				int i,n;
				Box *e;
				visibility = VISIBILITY_CLIPPED;
				for (
					i = 0, e = ctx->region->boxes, n = ctx->region->n_boxes; 
					i < n; i++, e++
				) {
					int r = e->x + e->width;
					int t = e->y + e->height;
					if (
						a.x >= e->x && a.y >= e->y && a.x < r && a.y < t &&
						b.x >= e->x && b.y >= e->y && b.x < r && b.y < t
					) {
						visibility = VISIBILITY_CLEAR;
						break;
					}
				}
			} else {
				visibility = VISIBILITY_CLEAR;
			}
		} else {
			visibility = VISIBILITY_CLIPPED;
		}

/* 
   Bresenham line plotting, (c) LiloHuang @ 2008, kenwu@cpan.org 
   http://cpansearch.perl.org/src/KENWU/Algorithm-Line-Bresenham-C-0.1/Line/Bresenham/C/C.xs
 */
 		rec.skip_pixel = closed || (j > 0);
		delta_y = b.y - a.y;
		delta_x = b.x - a.x;
		if (abs(delta_y) > abs(delta_x)) dir = 1;
		
		if (dir) {
			curr_maj = a.y;
			curr_min = a.x;
			to_maj = b.y;
			delta_maj = delta_y;
			delta_min = delta_x;
		} else {
			curr_maj = a.x;
			curr_min = a.y;
			to_maj = b.x;
			delta_maj = delta_x;
			delta_min = delta_y;   
		}

		if (delta_maj != 0)
			inc_maj = (abs(delta_maj)==delta_maj ? 1 : -1);
		else
			inc_maj = 0;
		
		if (delta_min != 0)
			inc_min = (abs(delta_min)==delta_min ? 1 : -1);
		else
			inc_min = 0;
		
		delta_maj = abs(delta_maj);
		delta_min = abs(delta_min);
		
		d      = (delta_min << 1) - delta_maj;
		d_inc1 = (delta_min << 1);
		d_inc2 = ((delta_min - delta_maj) << 1);

		while(1) {
			ox = x;
			if (dir) {
				x = curr_min;	 
				y = curr_maj;
			} else {
				x = curr_maj;	 
				y = curr_min;
			}
			if ( acc_y != y ) {
				if ( acc_y > INT_MIN) 
					segmented_hline( &rec, acc_x, ox, acc_y, visibility);
				acc_x = x;
				acc_y = y;
			}
	
			if (curr_maj == to_maj) break;
			curr_maj += inc_maj;
			if (d < 0) {
				d += d_inc1;
			} else {
				d += d_inc2;
				curr_min += inc_min;
			}
		}
		if ( acc_y > INT_MIN)
			segmented_hline( &rec, acc_x, x, acc_y, visibility);
	}
	return true;
}

typedef struct {
	int dX;
	int dY;
	int bpp;
	int sls;
	int dls;
	int mls;
	int als;
	Byte * src;
	Byte * dst;
	Byte * srcMask;
	Byte * dstMask;
	Bool use_src_alpha;
	Bool use_dst_alpha;
	Byte * pmsbuf;
	Byte * asbuf;
	Byte * adbuf;
	BlendFunc * blend1, * blend2;
} ImgPutAlphaCallbackRec;

static void
premultiply( Byte * src, Byte * alpha, int alpha_step, Byte * dst, int bytes)
{
	while (bytes--) {
		*(dst++) = *(src++) * *alpha / 255.0 + .5;
		alpha += alpha_step;
	}
}

static Bool
img_put_alpha_single( int x, int y, int w, int h, ImgPutAlphaCallbackRec * ptr)
{
	int i;
	const int bpp = ptr->bpp;
	int bytes = w * bpp;
	const int sls = ptr->sls;
	const int dls = ptr->dls;
	const int mls = ptr->mls;
	const int als = ptr->als;
	const Byte * s = ptr->src + (ptr->dY + y) * ptr->sls + (ptr->dX + x) * bpp;
	const Byte * d = ptr->dst + y * ptr->dls + x * bpp;
	const Byte * m = ( mls > 0) ? ptr->srcMask + (ptr->dY + y) * mls + (ptr->dX + x) : NULL;
	const Byte * a = ( als > 0) ? ptr->dstMask + y * als + x : NULL;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < h; i++) {
		Byte *asbuf_ptr, *adbuf_ptr, *s_ptr, *m_ptr, *d_ptr, *a_ptr;

		s_ptr = (Byte*)s + sls * i;
		d_ptr = (Byte*)d + dls * i;
		m_ptr = m ? (Byte*)m + mls * i : NULL;
		a_ptr = a ? (Byte*)a + als * i : NULL;

		if ( !ptr->use_src_alpha ) {
			asbuf_ptr = ptr->asbuf + bytes * OMP_THREAD_NUM;
			fill_alpha_buf( asbuf_ptr, m_ptr, w, bpp);
		} else
			asbuf_ptr = ptr->asbuf;

		if ( !ptr->use_dst_alpha ) {
			adbuf_ptr = ptr->adbuf + bytes * OMP_THREAD_NUM;
			fill_alpha_buf( adbuf_ptr, a_ptr, w, bpp);
		} else
			adbuf_ptr = ptr->adbuf;

		if ( ptr-> pmsbuf ) {
			Byte *pmsbuf = ptr-> pmsbuf + bytes * OMP_THREAD_NUM;
			premultiply( s_ptr, asbuf_ptr, ptr->use_src_alpha ? 0 : 1, pmsbuf, bytes);
			s_ptr = pmsbuf;
		} 
		ptr->blend1(
			s_ptr, 1,
			asbuf_ptr, ptr->use_src_alpha ? 0 : 1,
			d_ptr,
			adbuf_ptr, ptr->use_dst_alpha ? 0 : 1,
			bytes);
		if (a) {
			if ( ptr->use_src_alpha )
				ptr->blend2(
					ptr->asbuf, 0,
					ptr->asbuf, 0,
					(Byte*)a_ptr,
					a_ptr, ptr->use_dst_alpha ? 0 : 1,
					w);
			else
				ptr->blend2(
					m_ptr, 1,
					m_ptr, 1,
					(Byte*)a_ptr,
					a_ptr, ptr->use_dst_alpha ? 0 : 1,
					w);
		}
	}
	return true;
}

/*
	This is basically a lightweight pixman_image_composite() .
	Converts images to either 8 or 24 bits before processing
*/
static Bool
img_put_alpha( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop, PBoxRegionRec region)
{
	int bpp, bytes, mls, als, xrop;
	unsigned int src_alpha = 0, dst_alpha = 0;
	Bool use_src_alpha = false, use_dst_alpha = false, use_pms = false;
	Byte *asbuf, *adbuf, *pmsbuf = NULL;

	xrop = rop;

	/* differentiate between per-pixel alpha and a global value */
	if ( rop & ropSrcAlpha ) {
		use_src_alpha = true;
		src_alpha = (rop >> ropSrcAlphaShift) & 0xff;
	}
	if ( rop & ropDstAlpha ) {
		use_dst_alpha = true;
		dst_alpha = (rop >> ropDstAlphaShift) & 0xff;
	}
	if ( rop & ropPremultiply )
		use_pms = true;
	rop &= ropPorterDuffMask;
	if ( rop > ropMaxPDFunc || rop < 0 ) return false;

	/* align types and geometry - can only operate over imByte and imRGB */
	bpp = (( PImage(src)->type & imGrayScale) && ( PImage(dest)->type & imGrayScale)) ? imByte : imRGB;

	/* adjust rectangles */
	if ( dstX < 0 || dstY < 0 || dstX + dstW >= PImage(dest)-> w || dstY + dstH >= PImage(dest)-> h) {
		if ( dstX < 0) {
			dstW += dstX;
			srcX -= dstX;
			dstX = 0;
		}
		if ( dstY < 0) {
			dstH += dstY;
			srcY -= dstY;
			dstY = 0;
		}
		if ( dstX + dstW > PImage(dest)-> w)
			dstW = PImage(dest)-> w - dstX;
		if ( dstY + dstH > PImage(dest)-> h)
			dstH = PImage(dest)-> h - dstY;
	}
	if ( srcX + srcW > PImage(src)-> w)
		srcW = PImage(src)-> w - srcX;
	if ( srcY + srcH > PImage(src)-> h)
		srcH = PImage(src)-> h - srcY;
	if ( srcH <= 0 || srcW <= 0 )
		return false;

	/* adjust source type */
	if (PImage(src)-> type != bpp || srcW != dstW || srcH != dstH ) {
		Bool ok;
		Handle dup;

		if ( srcW != PImage(src)-> w || srcH != PImage(src)-> h)
			dup = CImage(src)-> extract( src, srcX, srcY, srcW, srcH );
		else
			dup = CImage(src)-> dup(src);

		if ( srcW != dstW || srcH != dstH )
			CImage(dup)->stretch( dup, dstW, dstH );
		if ( PImage( dup )-> type != bpp )
			CImage(dup)-> set_type( dup, bpp);

		ok = img_put_alpha( dest, dup, dstX, dstY, 0, 0, dstW, dstH, dstW, dstH, xrop, region);

		Object_destroy(dup);
		return ok;
	}

	/* adjust destination type */
	if (PImage(dest)-> type != bpp || ( kind_of( dest, CIcon) && PIcon(dest)->maskType != imbpp8 )) {
		Bool ok;
		Bool icon = kind_of(dest, CIcon);
		int type = PImage(dest)->type;
		int mask = icon ? PIcon(dest)->maskType : 0;

		if ( type != bpp )
			CIcon(dest)-> set_type( dest, bpp );
		if ( icon && mask != imbpp8 )
			CIcon(dest)-> set_maskType( dest, imbpp8 );
		ok = img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, xrop, region);
		if ( PImage(dest)-> options. optPreserveType ) {
			if ( type != bpp )
				CImage(dest)-> set_type( dest, type );
			if ( icon && mask != imbpp8 )
				CIcon(dest)-> set_maskType( dest, mask );
		}
		return ok;
	}

	/* assign pointers */
	if ( srcW != dstW || srcH != dstH ||
		PImage(src)->type != PImage(dest)->type || PImage(src)-> type != bpp)
		croak("panic: assert failed for img_put_alpha: %s", "types and geometry");

	bpp = ( bpp == imByte ) ? 1 : 3;
	if ( kind_of(src, CIcon)) {
		mls = PIcon(src)-> maskLine;
		if ( PIcon(src)-> maskType != imbpp8)
			croak("panic: assert failed for img_put_alpha: %s", "src mask type");
		use_src_alpha = false;
	} else {
		mls = 0;
	}

	if ( kind_of(dest, CIcon)) {
		als = PIcon(dest)-> maskLine;
		if ( PIcon(dest)-> maskType != imbpp8)
			croak("panic: assert failed for img_put_alpha: %s", "dst mask type");
		use_dst_alpha = false;
	} else {
		als = 0;
	}

	if ( !use_src_alpha && mls == 0) {
		use_src_alpha = true;
		src_alpha = 0xff;
	}
	if ( !use_dst_alpha && als == 0) {
		use_dst_alpha = true;
		dst_alpha = 0xff;
	}
	if ( use_pms && !use_src_alpha && mls == 0 )
		use_pms = false;

	/* make buffers */
	bytes = dstW * bpp;
	if ( !(asbuf = malloc(use_src_alpha ? 1 : (bytes * OMP_MAX_THREADS)))) {
		warn("not enough memory");
		return false;
	}
	if ( !(adbuf = malloc(use_dst_alpha ? 1 : (bytes * OMP_MAX_THREADS)))) {
		free(asbuf);
		warn("not enough memory");
		return false;
	}
	if ( use_pms ) {
		if ( !(pmsbuf = malloc( bytes * OMP_MAX_THREADS))) {
			free(adbuf);
			free(asbuf);
			warn("not enough memory");
			return false;
		}
	}

	if ( use_src_alpha ) asbuf[0] = src_alpha;
	if ( use_dst_alpha ) adbuf[0] = dst_alpha;

	/* select function */
	{
		ImgPutAlphaCallbackRec rec = {
			/* dX            */ srcX - dstX,
			/* dY            */ srcY - dstY,
			/* bpp           */ bpp,
			/* sls           */ PImage(src )-> lineSize,
			/* dls           */ PImage(dest)-> lineSize,
			/* mls           */ mls,
			/* als           */ als,
			/* src           */ PImage(src )->data,
			/* dst           */ PImage(dest)->data,
			/* srcMask       */ (mls > 0) ? PIcon(src )->mask : NULL,
			/* dstMask       */ (als > 0) ? PIcon(dest)->mask : NULL,
			/* use_src_alpha */ use_src_alpha,
			/* use_dst_alpha */ use_dst_alpha,
			/* pmsbuf        */ pmsbuf,
			/* asbuf         */ asbuf,
			/* adbuf         */ adbuf,
		};
		find_blend_proc(rop, &rec.blend1, &rec.blend2);
		img_region_foreach( region,
			dstX, dstY, dstW, dstH,
			(RegionCallbackFunc*)img_put_alpha_single, &rec
		);
	};

	/* cleanup */
	free(adbuf);
	free(asbuf);
	if (pmsbuf) free(pmsbuf);

	return true;
}

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
			fill_alpha_buf( adbuf_ptr, (Byte*)a, w, bpp);
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
			fill_alpha_buf( adbuf_ptr, (Byte*)a, w, bpp);
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
			resample_colors( dest, bpp, ctx );
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
		ctx->color[j] = (float)(ctx->color[j] * 255.0) / src_alpha + .5;
		ctx->backColor[j] = (float)(ctx->backColor[j] * 255.0) / src_alpha + .5;
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
		find_blend_proc(ctx->rop, &rec.blend1, &rec.blend2);
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
	PIcon         i;
	Rect          clip;
	int           y, bpp, bytes;
	Byte   *      color;
	Bool          single_border;
	int           first;
	PList  *      lists;
	PBoxRegionRec new_region;
	int           new_region_size;
} FillSession;

static Bool
fs_get_pixel( FillSession * fs, int x, int y)
{
	Byte * data;


	if ( x < fs-> clip. left || x > fs-> clip. right || y < fs-> clip. bottom || y > fs-> clip. top)
		return false;

	if ( fs-> lists[ y - fs-> first]) {
		PList l = fs-> lists[ y - fs-> first];
		int i;
		for ( i = 0; i < l-> count; i+=2) {
			if (((int) l-> items[i+1] >= x) && ((int)l->items[i] <= x))
				return false;
		}
	}

	data = fs->i->data + fs->i->lineSize * y;

	switch( fs-> bpp) {
	case 1: {
		Byte xz = *(data + (x >> 3));
		Byte v  = ( xz & ( 0x80 >> ( x & 7)) ? 1 : 0);
		return fs-> single_border ?
			( v == *(fs-> color)) : ( v != *(fs-> color));
	}
	case 4: {
		Byte xz = *(data + (x >> 1));
		Byte v  = (x & 1) ? ( xz & 0xF) : ( xz >> 4);
		return fs-> single_border ?
			( v == *(fs-> color)) : ( v != *(fs-> color));
	}
	case 8:
		return fs-> single_border ?
			( *(fs-> color) == *(data + x) ):
			( *(fs-> color) != *(data + x) );
	case 16:
		return fs-> single_border ?
			( *((uint16_t*)(fs-> color)) == *((uint16_t*)data + x) ) :
			( *((uint16_t*)(fs-> color)) != *((uint16_t*)data + x) );
	case 32:
		return fs-> single_border ?
			( *((uint32_t*)(fs-> color)) == *((uint32_t*)data + x) ) :
			( *((uint32_t*)(fs-> color)) != *((uint32_t*)data + x) );
	default: {
		return fs-> single_border ?
			( memcmp(data + x * fs->bytes, fs->color, fs->bytes) == 0) :
			( memcmp(data + x * fs->bytes, fs->color, fs->bytes) != 0);
	}}
}

static void
fs_hline( FillSession * fs, int x1, int y, int x2)
{
	y -= fs-> first;
	if ( fs-> lists[y] == NULL)
		fs-> lists[y] = plist_create( 32, 128);
	list_add( fs-> lists[y], ( Handle) x1);
	list_add( fs-> lists[y], ( Handle) x2);
}

static int
fs_fill( FillSession * fs, int sx, int sy, int d, int pxl, int pxr)
{
	int x, xr = sx;
	while ( sx > fs-> clip. left  && fs_get_pixel( fs, sx - 1, sy)) sx--;
	while ( xr < fs-> clip. right && fs_get_pixel( fs, xr + 1, sy)) xr++;
	fs_hline( fs, sx, sy, xr);

	if ( sy + d >= fs-> clip. bottom && sy + d <= fs-> clip. top) {
		x = sx;
		while ( x <= xr) {
			if ( fs_get_pixel( fs, x, sy + d))
				x = fs_fill( fs, x, sy + d, d, sx, xr);
			x++;
		}
	} 

	if ( sy - d >= fs-> clip. bottom && sy - d <= fs-> clip. top) {
		x = sx;
		while ( x < pxl) {
			if ( fs_get_pixel( fs, x, sy - d))
				x = fs_fill( fs, x, sy - d, -d, sx, xr);
			x++;
		}
		x = pxr;
		while ( x <= xr) {
			if ( fs_get_pixel( fs, x, sy - d))
				x = fs_fill( fs, x, sy - d, -d, sx, xr);
			x++;
		}
	}
	return xr;
}

static Bool
fs_intersect( int x1, int y, int w, int h, FillSession * fs)
{
	PList l;
	Handle * items;
	int i, j, x2;

	x2 = x1 + w - 1;
	for ( i = 0; i < h; i++) 
		if (( l = fs-> lists[y + i - fs->first]) != NULL )
			for ( j = 0, items = l->items; j < l-> count; j+=2) {
				Box * box;
				int left  = (int) (*(items++));
				int right = (int) (*(items++));
				if ( left < x1 )
					left = x1;
				if ( right > x2)
					right = x2;
				if ( left > right )
					continue;
				if ( fs-> new_region-> n_boxes >= fs-> new_region_size ) {
					PBoxRegionRec n;
					fs-> new_region_size *= 2;
					if ( !( n = img_region_alloc( fs-> new_region, fs-> new_region_size)))
						return false;
					fs-> new_region = n;
				}
				box = fs-> new_region-> boxes + fs-> new_region-> n_boxes;
				box-> x      = left;
				box-> y      = y + i;
				box-> width  = right - left + 1;
				box-> height = 1;
				fs-> new_region-> n_boxes++;
			}

	return true;
}

Bool
img_flood_fill( Handle self, int x, int y, ColorPixel color, Bool single_border, PImgPaintContext ctx)
{
	Bool ok = true;
	Box box;
	FillSession fs;
	
	fs.i             = ( PIcon ) self;
	fs.color         = color;
	fs.bpp           = fs.i->type & imBPP;
	fs.bytes         = fs.bpp / 8;
	fs.single_border = single_border;

	if ( ctx-> region ) {
		Box box = img_region_box( ctx-> region );
		fs. clip. left   = box. x;
		fs. clip. bottom = box. y;
		fs. clip. right  = box. x + box. width - 1;
		fs. clip. top    = box. y + box. height - 1;
	} else {
		fs. clip. left   = 0;
		fs. clip. bottom = 0;
		fs. clip. right  = fs.i->w - 1;
		fs. clip. top    = fs.i->h - 1;
	}

	fs. new_region_size = (fs. clip. top - fs. clip. bottom + 1) * 4;
	if ( !( fs. new_region = img_region_alloc(NULL, fs. new_region_size)))
		return false;

	fs. first = fs. clip. bottom;
	if ( !( fs. lists = malloc(( fs. clip. top - fs. clip. bottom + 1) * sizeof( void*)))) {
		free( fs. new_region );
		return false;
	}
	bzero( fs. lists, ( fs. clip. top - fs. clip. bottom + 1) * sizeof( void*));

	if ( fs_get_pixel( &fs, x, y)) {
		fs_fill( &fs, x, y, -1, x, x);
		ok = img_region_foreach( ctx->region,
			0, 0, fs.i->w, fs.i->h,
			(RegionCallbackFunc*)fs_intersect, &fs
		);
	}

	for ( x = 0; x < fs. clip. bottom - fs. clip. top + 1; x++)
		if ( fs. lists[x])
			plist_destroy( fs.lists[x]);
	free( fs. lists);

	if ( ok ) {
		ctx-> region = fs. new_region;
		box = img_region_box( ctx-> region );
		ok = img_bar( self, box.x, box.y, box.width, box.height, ctx);
	}

	free( fs. new_region);

	return ok;
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

void img_premultiply_alpha_map( Handle self, Handle alpha)
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

#ifdef __cplusplus
}
#endif

