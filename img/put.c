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
img_put_alpha( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop);

Bool 
img_put( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop)
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
		Byte *p;

		if ( PIcon(src)-> maskType != imbpp1) {
			if ( PIcon(src)-> maskType != imbpp8) croak("panic: bad icon mask type");
			return img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop);
		}

		PImage( src)-> self     =  CImage;
		PIcon( src)-> data = PIcon( src)-> mask;
		PImage( src)-> lineSize =  PIcon( src)-> maskLine;
		PImage( src)-> dataSize =  PIcon( src)-> maskSize;
		memcpy( palette, PImage( src)-> palette, 6);
		memcpy( PImage( src)-> palette, stdmono_palette, 6);


		PImage( src)-> type     =  imBW;
		PImage( src)-> palSize  = 2;
		img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropAndPut);
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
			ok = img_put( dest, dup, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop);
			Object_destroy(dup);
			return ok;
		}
		if ( PIcon(dest)-> maskType != imbpp8) {
			CIcon(dest)-> set_maskType(dest, imbpp8);
			ok = img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop);
			if ( PIcon(dest)-> options. optPreserveType )
				CIcon(dest)-> set_maskType(dest, imbpp1);
			return ok;
		}

		i = (PIcon) dest;
		img_fill_dummy( &dummy, i-> w, i-> h, imByte, i-> mask, NULL);
		return img_put((Handle)&dummy, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropCopyPut);
	} else if ( rop & ropConstantAlpha )
		return img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop);
	
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

	if ( srcX >= srcSz. x || srcX + srcW <= 0 ||
		srcY >= srcSz. y || srcY + srcH <= 0 ||
		dstX >= dstSz. x || dstX + dstW <= 0 ||
		dstY >= dstSz. y || dstY + dstH <= 0)
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
			img_put( dx, x, dsx, dsy, 0, 0, dsw, dsh, dsw, dsh, ropCopyPut);
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
			img_put( b8, src, dstX, dstY, 0, 0, dstW, dstH, PImage(src)-> w, PImage(src)-> h, rop);
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
			img_put( dest, src, dstX, dstY, 0, 0, dstW, dstH, PImage(src)-> w, PImage(src)-> h, rop);
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
		int  y, dyd, dys, count, pix;
		Byte *dptr, *sptr;
		PBitBltProc proc = find_blt_proc(rop);

		pix = ( PImage( dest)-> type & imBPP ) / 8;
		dyd = PImage( dest)-> lineSize;
		dys = PImage( src)-> lineSize;
		sptr = PImage( src )-> data + dys * srcY + pix * srcX;
		dptr = PImage( dest)-> data + dyd * dstY + pix * dstX;
		count = dstW * pix;

		if ( proc == bitblt_copy && dest == src) /* incredible */
			proc = bitblt_move;
		
		for ( y = 0; y < dstH; y++, sptr += dys, dptr += dyd) 
			proc( sptr, dptr, count);
	}

EXIT:
	if ( newObject) Object_destroy( src);

	return true;
}

void 
img_bar( Handle dest, int x, int y, int w, int h, int rop, void * color)
{
	PImage i     = (PImage) dest;
	Byte * data  = i-> data;
	int dataSize = i-> dataSize; 
	int lineSize = i-> lineSize; 
	int palSize  = i-> palSize; 
	int type     = i-> type;
	int pixSize  = (type & imBPP) / 8;
	PBitBltProc proc;
#define BLT_BUFSIZE 1024
	Byte blt_buffer[BLT_BUFSIZE];
	int j, k, offset, blt_bytes, blt_step;
	Byte filler, lmask, rmask, *p, *q;

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
	if ( w <= 0 || h <= 0 ) return;

	switch ( type & imBPP) {
	case imbpp1:
		filler = (*((Byte*)color)) ? 255 : 0;
		blt_bytes = (( x + w - 1) >> 3) - (x >> 3) + 1;
		blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes;
		memset( blt_buffer, filler, blt_step);
		lmask = ( x & 7 ) ? 255 << ( 8 - x & 7) : 0;
		rmask = (( x + w) & 7 ) ? 255 >> ((x + w) & 7) : 0;
		offset = x >> 3;
		break;
	case imbpp4:
		filler = (*((Byte*)color)) * 17;
		blt_bytes = (( x + w - 1) >> 1) - (x >> 1) + 1;
		blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes;
		memset( blt_buffer, filler, blt_step);
		lmask = ( x & 1 )       ? 0xf0 : 0;
		rmask = (( x + w) & 1 ) ? 0x0f : 0;
		offset = x >> 1;
		break;
	case imbpp8:
		filler = (*((Byte*)color));
		blt_bytes = w;
		blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes;
		memset( blt_buffer, filler, blt_step);
		lmask = rmask = 0;
		offset = x;
		break;
	default:
		blt_bytes = w * pixSize;
		blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE / pixSize : w;
		for ( j = 0, p = blt_buffer; j < blt_step; j ++ )
			for ( k = 0, q = (Byte*) color; k < pixSize; k++ )
				*(p++) = *(q++);
		blt_step *= pixSize;
		lmask = rmask = 0;
		offset = x * pixSize;
	}

	data += lineSize * y + offset;
	proc = find_blt_proc(rop);

#define DEBUG_ 1
#ifdef DEBUG
	warn("%d/%d, blt_bytes:%d, blt_step:%d, lmask:%02x, rmask:%02x, buf:%02x%02x%02x%02x\n", 
		x, w, blt_bytes, blt_step, lmask, rmask, blt_buffer[0], blt_buffer[1], blt_buffer[2], blt_buffer[3]);
#endif	

	for ( j = 0; j < h; j++) {
		int bytes = blt_bytes;
		Byte lsave = *data, rsave = data[blt_bytes - 1], *p = data;
		while ( bytes > 0 ) {
			proc( blt_buffer, p, ( bytes > blt_step ) ? blt_step : bytes );
			bytes -= blt_step;
			p += blt_step;
		}
		if ( lmask ) *data = (lsave & lmask) | (*data & ~lmask);
		if ( rmask ) data[blt_bytes-1] = (rsave & rmask) | (data[blt_bytes-1] & ~rmask);
		data += lineSize;
	}
}

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

#define dBLEND_FUNC(name) void name( const Byte * src, const Byte * src_a, Byte * dst, const Byte * dst_a, int bytes )

typedef dBLEND_FUNC(BlendFunc);

/* sss + (ddd * (255 - as)) / 255 */
static dBLEND_FUNC(blend_src_over)
{
	while ( bytes-- > 0 ) {
		register int32_t s = 
				((int32_t)(*src++) << 8 ) +  
				((int32_t)(*dst) << 8) * (255 - *src_a++) / 255 
				+ 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
}

/* (sss * (255 - ad) + ddd * (255 - as)) / 255 */
static dBLEND_FUNC(blend_xor)
{
	while ( bytes-- > 0 ) {
		register int32_t s = (
				((int32_t)(*src++) << 8) * (255 - *dst_a++) + 
				((int32_t)(*dst)   << 8) * (255 - *src_a++)
			) / 255 + 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
}

/* sss * (255 - ad) / 255 + ddd */
static dBLEND_FUNC(blend_dst_over)
{
	while ( bytes-- > 0 ) {
		register int32_t s = 
				((int32_t)(*dst) << 8 ) +  
				((int32_t)(*src++) << 8) * (255 - *dst_a++) / 255 
				+ 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
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

/* sss * ad / 255 */
static dBLEND_FUNC(blend_src_in)
{
	while ( bytes-- > 0 ) {
		register int32_t s = (((int32_t)(*src++) << 8) * *dst_a++) / 255 + 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
}

/* ddd * as / 255 */
static dBLEND_FUNC(blend_dst_in)
{
	while ( bytes-- > 0 ) { 
		register int32_t d = (((int32_t)(*dst) << 8) * *src_a++) / 255 + 127;
		d >>= 8;
		*dst++ = ( d > 255 ) ? 255 : d;
	}
}

/* sss * (255 - ad) / 255 */
static dBLEND_FUNC(blend_src_out)
{
	while ( bytes-- > 0 ) {
		register int32_t s = (((int32_t)(*src++) << 8) * ( 255 - *dst_a++)) / 255 + 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
}

/* ddd * (255 - as) / 255 */
static dBLEND_FUNC(blend_dst_out)
{
	while ( bytes-- > 0 ) {
		register int32_t d = (((int32_t)(*dst) << 8) * ( 255 - *src_a++)) / 255 + 127;
		d >>= 8;
		*dst++ = ( d > 255 ) ? 255 : d;
	}
}

/* (sss * ad + ddd * (255 - as)) / 255 */
static dBLEND_FUNC(blend_src_atop)
{
	while ( bytes-- > 0 ) {
		register int32_t s = (
			((int32_t)(*src++) << 8) * *dst_a++ + 
			((int32_t)(*dst) << 8) * (255 - *src_a++)
		) / 255 + 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
}

/* (sss * (255 - ad) + ddd * as) / 255 */
static dBLEND_FUNC(blend_dst_atop)
{
	while ( bytes-- > 0 ) {
		register int32_t s = (
			((int32_t)(*src++) << 8) * ( 255 - *dst_a++) + 
			((int32_t)(*dst) << 8) * *src_a++
		) / 255 + 127;
		s >>= 8;
		*dst++ = ( s > 255 ) ? 255 : s;
	}
}

/* sss */
static dBLEND_FUNC(blend_src_copy)
{
	memcpy( dst, src, bytes);
}

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
	blend_dst_atop
};

/* 
	This is basically a lightweight pixman_image_composite() .
	Converts images to either 8 or 24 bits before processing
*/
static Bool 
img_put_alpha( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop)
{
	int bpp, bytes, sls, dls, mls, als, x, y, px;
	Byte *s, *d, *m, *a; 
	unsigned int src_alpha = 0, dst_alpha = 0;
	Bool use_src_alpha = false, use_dst_alpha = false;
	Byte *asbuf, *adbuf;
	BlendFunc * blend_func;

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
	if ( rop > ropDstAtop || rop < 0 ) return false;

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

		ok = img_put_alpha( dest, dup, dstX, dstY, 0, 0, dstW, dstH, dstW, srcH, rop);

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
		ok = img_put_alpha( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, rop);
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
	sls = PImage(src)-> lineSize;
	dls = PImage(dest)-> lineSize;
	s = PImage(src )-> data + srcY * sls + srcX * bpp;
	d = PImage(dest)-> data + dstY * dls + dstX * bpp;

	if ( kind_of(src, CIcon)) {
		mls = PIcon(src)-> maskLine;
		m   = PIcon(src)-> mask + srcY * mls + srcX;
		if ( PIcon(src)-> maskType != imbpp8)
		croak("panic: assert failed for img_put_alpha: %s", "src mask type");
		use_src_alpha = false;
	} else {
		m   = NULL;
		mls = 0;
	}

	if ( kind_of(dest, CIcon)) {
		als = PIcon(dest)-> maskLine;
		a   = PIcon(dest)-> mask + dstY * als + dstX;
		if ( PIcon(dest)-> maskType != imbpp8)
		croak("panic: assert failed for img_put_alpha: %s", "dst mask type");
		use_dst_alpha = false;
	} else {
		a   = NULL;
		als = 0;
	}

	if ( !use_src_alpha && !m) {
		use_src_alpha = true;
		src_alpha = 0xff;
	}
	if ( !use_dst_alpha && !a) {
		use_dst_alpha = true;
		dst_alpha = 0xff;
	}

	/* make buffers for alpha */
	bytes = dstW * bpp;
	if ( !(asbuf = malloc(bytes * (use_src_alpha ? 1 : OMP_MAX_THREADS)))) {
		warn("not enough memory");
		return false;
	}
	if ( !(adbuf = malloc(bytes * (use_dst_alpha ? 1 : OMP_MAX_THREADS)))) {
		free(asbuf);
		warn("not enough memory");
		return false;
	}

	if ( use_src_alpha )
		for ( x = 0; x < bytes; x++) asbuf[x] = src_alpha;
	if ( use_dst_alpha )
		for ( x = 0; x < bytes; x++) adbuf[x] = dst_alpha;

	/* select function */
	blend_func = blend_functions[rop];

	/* blend */
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( y = 0; y < dstH; y++) {
		Byte *asbuf_ptr, *adbuf_ptr, *s_ptr, *m_ptr, *d_ptr, *a_ptr;

		s_ptr = s + sls * y;
		d_ptr = d + dls * y;
		if (m) m_ptr = m + mls * y;
		if (a) a_ptr = a + als * y;

		if ( !use_src_alpha ) {
			asbuf_ptr = asbuf + bytes * OMP_THREAD_NUM;
			fill_alpha_buf( asbuf_ptr, m_ptr, dstW, bpp);
		} else
			asbuf_ptr = asbuf;

		if ( !use_dst_alpha ) {
			adbuf_ptr = adbuf + bytes * OMP_THREAD_NUM;
			fill_alpha_buf( adbuf, a_ptr, dstW, bpp);
		} else
			adbuf_ptr = adbuf;
		
		blend_func( s_ptr, asbuf_ptr, d_ptr, adbuf_ptr, bytes);
		if (a) {
			if ( use_src_alpha )
				blend_func( asbuf, asbuf, a_ptr, a_ptr, dstW);
			else
				blend_func( m_ptr, m_ptr, a_ptr, a_ptr, dstW);
		}
	}

	/* cleanup */
	free(adbuf);
	free(asbuf);

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

void img_premultiply_alpha_map( Handle self, Handle alpha)
{
	Byte * data, * mask;
	int i, j, pixels;
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

