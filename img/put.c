#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif


static Bool
img_put_alpha( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop, PRegionRec region);

typedef struct {
	int srcX;
	int srcY;
	int bits;
	int bytes;
	int srcLS;
	int dstLS;
	int dX;
	int dY;
	Byte * src;
	Byte * dst;
	Byte * colorref;
	PBitBltProc proc;
} ImgPutCallbackRec;

static Bool
img_put_single( int x, int y, int w, int h, ImgPutCallbackRec * ptr)
{
	int i, count;
	Byte *dptr, *sptr;

	sptr  = ptr->src + ptr->srcLS * (ptr->dY + y);
	dptr  = ptr->dst + ptr->dstLS * y;

	switch ( ptr-> bits ) {
	case 1:
		for ( i = 0; i < h; i++, sptr += ptr->srcLS, dptr += ptr->dstLS)
			bc_mono_put( sptr, ptr->dX + x, w, dptr, x, ptr->proc);
		break;
	case 4:
		for ( i = 0; i < h; i++, sptr += ptr->srcLS, dptr += ptr->dstLS)
			bc_nibble_put( sptr, ptr->dX + x, w, dptr, x, ptr->proc, ptr->colorref);
		break;
	case 8:
		if ( ptr-> colorref ) {
			sptr  += ptr->bytes * (ptr->dX + x);
			dptr  += ptr->bytes * x;
			for ( i = 0; i < h; i++, sptr += ptr->srcLS, dptr += ptr->dstLS)
				bc_byte_put( sptr, dptr, w, ptr->proc, ptr->colorref);
			break;
		}
		/* fall through */
	default:
		sptr  += ptr->bytes * (ptr->dX + x);
		dptr  += ptr->bytes * x;
		count = w * ptr->bytes;
		for ( i = 0; i < h; i++, sptr += ptr->srcLS, dptr += ptr->dstLS)
			ptr->proc( sptr, dptr, count);
	}

	return true;
}

static void
bitblt_move( Byte * src, Byte * dst, int count)
{
	memmove( dst, src, count);
}

Bool
img_put(
	Handle dest, Handle src,
	int dstX, int dstY, int srcX, int srcY,
	int dstW, int dstH, int srcW, int srcH,
	int rop,
	PRegionRec region, Byte * color
) {
	Point srcSz, dstSz;
	int asrcW, asrcH;
	Bool newObject = false, retval = true, use_colorref = false;
	Byte colorref[256];

	if ( dest == NULL_HANDLE || src == NULL_HANDLE) return false;
	if ( rop == ropNoOper) return false;

	if ( kind_of( src, CIcon)) {
		Image dummy;
		PIcon s = PIcon(src);
		if ( s-> maskType != imbpp1) {
			if ( s-> maskType != imbpp8) croak("panic: bad icon mask type");
			return img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region);
		}
		img_fill_dummy( &dummy, s-> w, s-> h, imBW, s-> mask, stdmono_palette);
		img_put( dest, (Handle) &dummy, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropAndPut, region, color);
		rop = ropXorPut;
	} else if ( rop == ropAlphaCopy ) {
		Bool ok;
		Image dummy;
		PIcon i;
		if ( !kind_of( dest, CIcon )) return false;
		if ( PImage(src)-> type != imByte ) {
			Handle dup = CImage(src)->dup(src);
			CImage(dup)->set_type(src, imByte);
			ok = img_put( dest, dup, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region, color);
			Object_destroy(dup);
			return ok;
		}
		if ( PIcon(dest)-> maskType != imbpp8) {
			CIcon(dest)-> set_maskType(dest, imbpp8);
			ok = img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region, color);
			if ( PIcon(dest)-> options. optPreserveType )
				CIcon(dest)-> set_maskType(dest, imbpp1);
			return ok;
		}

		i = (PIcon) dest;
		img_fill_dummy( &dummy, i-> w, i-> h, imByte, i-> mask, std256gray_palette);
		return img_put((Handle)&dummy, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropCopyPut, region, color);
	} else if ( color != NULL ) {
		Bool ok;
		Icon dummy;
		PImage s = PImage(src);
		Byte * bits;
		ImgPaintContext ctx;
		if ( s-> type != imByte ) {
			Handle dup = CImage(src)->dup(src);
			CImage(dup)->set_type(dup, imByte);
			ok = img_put( dest, dup, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region, color);
			Object_destroy(dup);
			return ok;
		}
		bzero( &ctx, sizeof(ctx));
		memcpy( ctx.color, color, MAX_SIZEOF_PIXEL);
		if ( PImage(dest)-> type & imGrayScale ) {
			unsigned size = LINE_SIZE(s->w, imByte) * s->h;
			if ( !( bits = malloc( size )))
				return false;
			img_resample_colors(dest, imbpp8, &ctx);
			memset(bits, ctx.color[0], size );
			img_fill_dummy((PImage) &dummy, s-> w, s-> h, imByte, bits, std256gray_palette);
		} else {
			Byte * line;
			int w, linesize = LINE_SIZE(s-> w, imRGB);
			if ( !( bits = malloc( linesize * s->h )))
				return false;
			line = bits;
			img_resample_colors(dest, imbpp24, &ctx);
			for ( w = 0; w < s-> w; w++ ) {
				*(line++) = ctx.color[0];
				*(line++) = ctx.color[1];
				*(line++) = ctx.color[2];
			}
			for ( w = 1, line = bits + linesize; w < s-> h; w++, line += linesize)
				memcpy( line, bits, linesize);
			img_fill_dummy((PImage) &dummy, s-> w, s-> h, imRGB, bits, NULL);
		}
		dummy. self     = CIcon;
		dummy. mask     = s-> data;
		dummy. maskLine = s-> lineSize;
		dummy. maskSize = s-> dataSize;
		dummy. maskType = imbpp8;
		rop &= ~(ropAlphaCopy | ropConstantColor);
		ok = img_put(dest, (Handle)&dummy, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region, NULL);
		free(bits);
		return ok;
	} else if ( rop & ropConstantAlpha ) {
		return img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop, region);
	}

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
			retval = img_put( dx, x, dsx, dsy, 0, 0, dsw, dsh, dsw, dsh, ropCopyPut, region, color);
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
	if (( PImage(src)-> type & imBPP ) != ( PImage( dest)-> type & imBPP )) {
		if ( !newObject) {
			src = CImage( src)-> dup( src);
			if ( !src) goto EXIT;
			newObject = true;
		}
		PImage(src)-> self-> reset( src, PImage(dest)-> type, PImage(dest)->palette, PImage(dest)->palSize);
	}

	if (( PImage( dest)-> type & imBPP) == 1) {
		PImage i  = (PImage) dest;
		PImage s  = (PImage) src;
		Byte fore = cm_nearest_color(s->palette[1], i->palSize, i->palette);
		Byte back = cm_nearest_color(s->palette[0], i->palSize, i->palette);
		rop = rop_1bit_transform( fore, back, rop );
	} else if (( PImage( dest)-> type & imBPP) <= 8 ) {
		/* equalize palette */
		if ( rop == ropCopyPut && (
			PImage(dest)->palSize != PImage(src)->palSize ||
			memcmp( PImage(dest)->palette, PImage(src)->palette, PImage(dest)->palSize * 3) != 0
		)) {
			cm_fill_colorref(
				PImage( src)-> palette, PImage( src)-> palSize,
				PImage( dest)-> palette, PImage( dest)-> palSize,
				colorref);
			use_colorref = true;
			if (( PImage( dest)-> type & imBPP) == 4 )
				cm_colorref_4to8( colorref, colorref );
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
		int bpp = PImage( dest)-> type & imBPP;
		ImgPutCallbackRec rec = {
			/* srcX  */ srcX,
			/* srcY  */ srcY,
			/* bits */  bpp,
			/* bytes */ bpp / 8,
			/* srcLS */ PImage( src)-> lineSize,
			/* dstLS */ PImage( dest)-> lineSize,
			/* dX    */ srcX - dstX,
			/* dY    */ srcY - dstY,
			/* src   */ PImage( src )-> data,
			/* dst   */ PImage( dest )-> data,
			/* cr    */ use_colorref ? colorref : NULL,
			/* proc  */ img_find_blt_proc(rop)
		};
		if ( rop == ropCopyPut && dest == src) /* incredible */
			rec.proc = bitblt_move;
		img_region_foreach( region,
			dstX, dstY, dstW, dstH,
			(RegionCallbackFunc*)img_put_single, &rec
		);
	}

EXIT:
	if ( newObject) Object_destroy( src);

	return retval;
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
	Byte src_alpha_mul;
	Byte dst_alpha_mul;
	Byte * asbuf;
	Byte * adbuf;
	BlendFunc * blend1, * blend2;
} ImgPutAlphaCallbackRec;

static void
multiply( Byte * src, Byte * alpha, int alpha_step, Byte * dst, int bytes)
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
			img_fill_alpha_buf( asbuf_ptr, m_ptr, w, bpp);
			if ( ptr-> src_alpha_mul < 255 )
				multiply( asbuf_ptr, &ptr->src_alpha_mul, 0, asbuf_ptr, bytes);
		} else
			asbuf_ptr = ptr->asbuf;

		if ( !ptr->use_dst_alpha ) {
			adbuf_ptr = ptr->adbuf + bytes * OMP_THREAD_NUM;
			img_fill_alpha_buf( adbuf_ptr, a_ptr, w, bpp);
			if ( ptr-> dst_alpha_mul < 255 )
				multiply( adbuf_ptr, &ptr->dst_alpha_mul, 0, adbuf_ptr, bytes);
		} else
			adbuf_ptr = ptr->adbuf;

		ptr->blend1(
			s_ptr, 1,
			asbuf_ptr, ptr->use_src_alpha ? 0 : 1,
			d_ptr,
			adbuf_ptr, ptr->use_dst_alpha ? 0 : 1,
			bytes);
		if (a == NULL)
			continue;

#define BLEND_ALPHA(src,step)\
	ptr->blend2(\
		src,step,\
		src,step,\
		(Byte*)a_ptr,\
		a_ptr, ptr->use_dst_alpha ? 0 : 1,\
		w)

		if ( ptr->dst_alpha_mul < 255 )
			multiply( a_ptr, &ptr->dst_alpha_mul, 0, a_ptr, w);
		if ( ptr-> src_alpha_mul < 255 ) {
			if ( bpp == 3 ) /* reuse the old values from multiply() otherwise */
				multiply( m_ptr, &ptr->src_alpha_mul, 0, asbuf_ptr, w);
			BLEND_ALPHA(asbuf_ptr,1);
		} else if ( ptr->use_src_alpha )
			BLEND_ALPHA(ptr->asbuf,0);
		else
			BLEND_ALPHA(m_ptr,1);
#undef BLEND_ALPHA
	}
	return true;
}

/*
	This is basically a lightweight pixman_image_composite() .
	Converts images to either 8 or 24 bits before processing
*/
static Bool
img_put_alpha( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop, PRegionRec region)
{
	int bpp, bytes, mls, als, xrop;
	unsigned int src_alpha = 0, dst_alpha = 0;
	Bool use_src_alpha = false, use_dst_alpha = false, no_scale;
	Byte *asbuf, *adbuf, src_alpha_mul = 255, dst_alpha_mul = 255;

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
	rop &= ropPorterDuffMask;
	if ( rop > ropMaxPDFunc || rop < 0 ) return false;


	/* align types and geometry - can only operate over imByte and imRGB */
	if (
		srcX >= PImage(src)->w  || srcX + srcW <= 0 ||
		srcY >= PImage(src)->h  || srcY + srcH <= 0 ||
		dstX >= PImage(dest)->w || dstX + dstW <= 0 ||
		dstY >= PImage(dest)->h || dstY + dstH <= 0
	)
		return true;
	no_scale =
		( srcW == dstW) && ( srcH == dstH) &&
		( srcX >= 0) && ( srcY >= 0) && ( srcX + srcW <= PImage(src)-> w) && ( srcY + srcH <= PImage(src)-> h)
		;

	bpp = (( PImage(src)->type & imGrayScale) && ( PImage(dest)->type & imGrayScale)) ? imByte : imRGB;

	/* adjust source type */
	if (PImage(src)-> type != bpp || !no_scale ) {
		Bool ok;
		Handle dup;

		dup = ( srcW != PImage(src)-> w || srcH != PImage(src)-> h) ?
			CImage(src)-> extract( src, srcX, srcY, srcW, srcH ) :
			CImage(src)-> dup(src);

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
	if ( PImage(src)->type != PImage(dest)->type || PImage(src)-> type != bpp)
		croak("panic: assert failed for img_put_alpha: %s", "types");

	bpp = ( bpp == imByte ) ? 1 : 3;
	if ( kind_of(src, CIcon)) {
		mls = PIcon(src)-> maskLine;
		if ( PIcon(src)-> maskType != imbpp8)
			croak("panic: assert failed for img_put_alpha: %s=%d", "src mask type", PIcon(src)->maskType);
		if ( use_src_alpha )
			src_alpha_mul = src_alpha;
		use_src_alpha = false;
	} else {
		mls = 0;
	}

	if ( kind_of(dest, CIcon)) {
		als = PIcon(dest)-> maskLine;
		if ( PIcon(dest)-> maskType != imbpp8)
			croak("panic: assert failed for img_put_alpha: %s=%d", "dst mask type", PIcon(dest)->maskType);
		if ( use_dst_alpha )
			dst_alpha_mul = dst_alpha;
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

	if ( use_src_alpha ) asbuf[0] = src_alpha;
	if ( use_dst_alpha ) adbuf[0] = dst_alpha;

	/* adjust final rectangles */
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
			/* src_alpha_mul */ src_alpha_mul,
			/* dst_alpha_mul */ dst_alpha_mul,
			/* asbuf         */ asbuf,
			/* adbuf         */ adbuf,
		};
		img_find_blend_proc(rop, &rec.blend1, &rec.blend2);
		img_region_foreach( region,
			dstX, dstY, dstW, dstH,
			(RegionCallbackFunc*)img_put_alpha_single, &rec
		);
	};

	/* cleanup */
	free(adbuf);
	free(asbuf);

	return true;
}

#ifdef __cplusplus
}
#endif

