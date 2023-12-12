#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
bc_mono_colormask_on_byte(
	Byte * src, unsigned int offset,
	Byte * dst, unsigned int _bits, unsigned int bytes,
	Byte *color, BitBltProc *blt
) {
	register Byte B, shift;
	Byte curr = 0;
	register int bits = _bits;

	src += offset >> 3;
	shift = offset & 7;
	B = *src++ << shift;

	while ( bits > 0 ) {
		if ( curr == 0 ) {
			/* skip pixels */
			while ( bits > 0 ) {
				if ( B == 0 ) {
					register Byte skip = 8 - shift;
					B     = *src++;
					dst  += bytes * skip;
					bits -= skip;
					shift = 0;
				} else if ( B & 0x80) {
					goto FLIP;
				} else {
					if ( shift++ == 7 ) {
						shift = 0;
						B     = *src++;
					}
					dst  += bytes;
					bits--;
					B <<= 1;
				}
			}
		} else {
			/* plot pixels */
			while ( bits > 0 ) {
				Byte buf[256];
				register Byte *pbuf = buf;
				register unsigned int n = (sizeof(buf) > bits * bytes) ? bits * bytes : sizeof(buf), m = 0;
				while (n > bytes - 1) {
					if (( B & 0x80 ) == 0) {
						if ( m > 0 ) {
							blt(buf, dst, m);
							dst += m;
						}
						goto FLIP;
					}
					if ( shift++ == 7 ) {
						shift = 0;
						B     = *src++;
					} else
						B <<= 1;
					bits--;

					switch ( bytes ) {
					case 4:
						*((uint32_t*)pbuf) = *((uint32_t*)color);
						pbuf += 4;
						break;
					case 3:
						*pbuf++ = color[0];
						*pbuf++ = color[1];
						*pbuf++ = color[2];
						break;
					case 2:
						*((uint16_t*)pbuf) = *((uint16_t*)color);
						pbuf += 2;
						break;
					case 1:
						*pbuf++ = color[0];
						break;
					default:
						memcpy( pbuf, color, bytes);
						pbuf += bytes;
					}
					m += bytes;
					n -= bytes;

				}
				if ( m > 0 ) {
					blt(buf, dst, m);
					dst += m;
				}
			}
		}
	FLIP:
		curr = !curr;
	}
}

static void
bc_mono_colormask_on_nibble( Byte * src, unsigned int src_offset, Byte * dst, int dst_offset, unsigned int nibbles, Byte acolor, BitBltProc *blt)
{
	Byte buf[256], color = (acolor & 0xf0) | (acolor << 4);
	while ( nibbles > 0 ) {
		unsigned int
			pixels = (nibbles > sizeof(buf)) ? sizeof(buf) : nibbles,
			head = dst_offset & 1,
			tail;
		Byte *dd = dst + (dst_offset >> 1);
		if ( head ) {
			tail = 1;
			if ( ((pixels + 1) & 1) && pixels > 1 )
				pixels--; /* align to byte boundary on the next iteration */
		} else {
			tail = pixels & 1;
		}
		bc_nibble_byte( dd, buf, pixels + tail);
		bc_mono_colormask_on_byte( src, src_offset, buf + head, pixels, 1, &color, blt);
		bc_byte_nibble_cr( buf, dd, pixels + tail, map_stdcolorref);
		src_offset += pixels;
		dst_offset += pixels;
		nibbles    -= pixels;
	}
}


typedef struct PlotStruct PlotStruct;
typedef void PlotFunc( int x, int y, int xFrom, int yFrom, int xLen, int yLen, PlotStruct *p);

struct PlotStruct {
	PImage src_image, dst_image;
	Byte *src, *dst, *mask;
	unsigned int src_stride, dst_stride, mask_stride;
	Byte *color;
	Rect r;

	unsigned int bpp, bytes;
	PlotFunc *func;
	BitBltProc* blt;

	BlendFunc  *blend1, *blend2;
	Bool        use_dst_alpha, is_icon;
	Byte        src_alpha, dst_alpha;
};

static Bool
plot_glyph( int x, int y, int w, int h, PlotStruct * ptr)
{
	int ox, oy;
	if (
		ptr->r.left   >= x + w ||
		ptr->r.right  < x      ||
		ptr->r.bottom >= y + h ||
		ptr->r.top    < y
	)
		return true;

	if ( ptr->r.right >= x + w )
		w += x - ptr->r.left + 1;
	else
		w = ptr->r.right - ptr->r.left + 1;
	if ( ptr->r.left <= x ) {
		ox = x - ptr->r.left;
		w -= ox;
	} else {
		ox = 0;
		x = ptr->r.left;
	}
	if ( x < 0 ) {
		ox -= x;
		w += x;
		x = 0;
	}
	if ( w <= 0 || ox >= ptr->src_image->w )
		return true;

	if ( ptr->r.top >= y + h )
		h += y - ptr->r.bottom + 1;
	else
		h = ptr->r.top - ptr->r.bottom + 1;
	if ( ptr->r.bottom <= y ) {
		oy = y - ptr->r.bottom;
		h -= oy;
	} else {
		oy = 0;
		y = ptr->r.bottom;
	}
	if ( y < 0 ) {
		oy -= y;
		h += y;
		y = 0;
	}
	if ( h <= 0 || oy >= ptr->src_image->h )
		return true;

	ptr->func( x, y, ox, oy, w, h, ptr);

	return true;
}

static void
plot_rop( int x, int y, int xFrom, int yFrom, int xLen, int yLen, PlotStruct *p)
{
	int i;
	Byte *s, *d;
	for (
		i = 0, s = p->src + yFrom * p->src_stride, d = p->dst + y * p->dst_stride;
		i < yLen;
		i++, s += p->src_stride, d += p->dst_stride
	) {
		switch ( p-> bpp ) {
		case 1:
			bc_mono_put( s, xFrom, xLen, d, x, p->blt);
			break;
		case 4:
			bc_mono_colormask_on_nibble( s, xFrom, d, x, xLen, p->color[0], p->blt);
			break;
		default:
			bc_mono_colormask_on_byte( s, xFrom, d + x * p->bytes, xLen, p->bytes, p->color, p->blt);
		}
	}
}

static void
plot_blend( int x, int y, int xFrom, int yFrom, int xLen, int yLen, PlotStruct *p)
{
	int i;
	Byte *s, *d, *m;
	Bool
		use_mbuf  = p->blend2 && p->is_icon,
		fill_mbuf = use_mbuf && p->bytes == 3;
	Byte buf1[768], buf2[768], buf3[256], sa = p->src_alpha;

	for (
		i = 0,
			s = p->src + yFrom * p->src_stride + xFrom,
			d = p->dst + y * p->dst_stride + x * p->bytes,
			m = p->use_dst_alpha ? &p->dst_alpha : p->mask + y * p->mask_stride + x
			;
		i < yLen;
		i++, s += p->src_stride, d += p->dst_stride
	) {
		int pixels = xLen, bytes = p->bytes;
		Bool curr = 0;
		register Byte *ss = s, *dd = d, *mm = m;
		while ( pixels > 0 ) {
			if ( curr == 0 ) {
				/* skip pixels */
				while ( pixels > 0 ) {
					if ( *ss > 0 )
						goto FLIP;
					dd += bytes;
					pixels--;
					ss++;
					if ( !p->use_dst_alpha ) mm++;
				}
			} else {
				/* plot pixels */
#define BLEND \
	if ( px1 > 0 ) {                                               \
		p->blend1( buf1, 1, buf2, 1, dd, mm, 0, px1);          \
		if ( use_mbuf )                                        \
			p->blend2( mmbuf, 1, mmbuf, 1, mm, mm, 0, px2);\
		dd += px1;                                             \
		if ( !p->use_dst_alpha ) mm += px2;                    \
	}
				while ( pixels > 0 ) {
					register Byte
						*xbuf  = buf1,
						*abuf  = buf2,
						*mbuf  = fill_mbuf ? buf3 : buf2;
					Byte
						*mmbuf = mbuf;
					int
						n = (sizeof(buf2) > pixels) ? pixels : sizeof(buf2),
						px1 = 0,
						px2 = 0;
					while (n-- > 0) {
						if ( *ss == 0 ) {
							BLEND;
							goto FLIP;
						}
						if ( bytes == 3 ) {
							register Byte sss = *ss;
							*xbuf++ = p->color[0] * sss / 255;
							*xbuf++ = p->color[1] * sss / 255;
							*xbuf++ = p->color[2] * sss / 255;
							if ( sa < 255 )
								sss = sss * sa / 255;
							*abuf++ = sss;
							*abuf++ = sss;
							*abuf++ = sss;
							if ( fill_mbuf ) *mbuf++ = sss;
						} else {
							register Byte sss = *ss;
							*xbuf++ = p->color[0] * sss / 255;
							*abuf++ = ( sa < 255 ) ? sss * sa / 255 : sss;
						}
						px1 += bytes;
						px2 ++;
						n   -= bytes;
						pixels--;
						ss++;
					}
					BLEND;
				}
			}
		FLIP:
			curr = !curr;
		}
#undef BLEND

		if ( !p->use_dst_alpha )
			m += p->mask_stride;
	}
}

static Byte
rop_black( int rop )
{
	switch (rop) {
	case ropNotSrcAnd  :
	case ropXorPut     :
	case ropOrPut      : return ropNoOper;
	case ropBlackness  :
	case ropNotDestAnd :
	case ropAndPut     :
	case ropCopyPut    : return ropNotSrcAnd;
	case ropNotPut     :
	case ropNotAnd     :
	case ropNotSrcOr   :
	case ropWhiteness  : return ropOrPut;
	case ropNotOr      :
	case ropInvert     :
	case ropNotXor     :
	case ropNotDestOr  : return ropXorPut;
	}
	return rop;
}

static Byte
rop_white( int rop )
{
	switch (rop) {
	case ropAndPut     :
	case ropNotXor     :
	case ropNotSrcOr   : return ropNoOper;
	case ropBlackness  :
	case ropNotOr      :
	case ropNotPut     : return ropNotSrcAnd;
	case ropCopyPut    :
	case ropNotDestOr  :
	case ropWhiteness  : return ropOrPut;
	case ropNotDestAnd :
	case ropInvert     :
	case ropNotAnd     : return ropXorPut;
	}
	return rop;
}


Bool
img_plot_glyph( Handle self, PImage glyph, int x, int y, PImgPaintContext ctx)
{
	PImage i = (PImage) self;
	int w, h, rop = ctx->rop;
	Bool mono = (glyph->type & imBPP) == 1;

	PlotStruct rec = {
		glyph, i,
		glyph->data, i->data, NULL,
		glyph->lineSize, i->lineSize, 0,
		ctx->color,
		{ x, y, x + glyph->w - 1, y + glyph->h - 1 }
	};

	if (mono) {
		if (rop >= ropWhiteness )
			rop = ropCopyPut;
		if ((i->type & imBPP) == 1)
			rop = ctx->color[0] ? rop_white(rop) : rop_black(rop);
	}

	if ( rop == ropNoOper) {
		return true;
	} else if ( rop <= ropWhiteness ) {
		if ( !mono ) {
			warn("img_plot_glyph: cannot use raster operations with antialiased text");
			return false;
		}
		rec.blt   = img_find_blt_proc(rop);
		rec.func  = plot_rop;
		rec.bpp   = i->type & imBPP;
		rec.bytes = rec.bpp / 8;
	} else if ( i-> type == imByte || i-> type == imRGB ) {
		if ( mono ) {
			warn("img_plot_glyph: cannot use blending with non-antialiased text");
			return false;
		}

		/* differentiate between per-pixel alpha and a global value */
		if ( rop & ropSrcAlpha )
			rec.src_alpha = (rop >> ropSrcAlphaShift) & 0xff;
		else
			rec.src_alpha = 0xff;
		if ( rop & ropDstAlpha ) {
			rec.use_dst_alpha = true;
			rec.dst_alpha = (rop >> ropDstAlphaShift) & 0xff;
		} else
			rec.use_dst_alpha = false;
		rop &= ropPorterDuffMask;
		if ( !img_find_blend_proc(rop, &rec.blend1, &rec.blend2)) {
			warn("img_plot_glyph: blending rop expected");
			return false;
		}

		rec.is_icon = kind_of( self, CIcon );
		if ( rec.is_icon ) {
			if ((PIcon(self)-> maskType != imbpp8) && !rec.use_dst_alpha) {
				warn("img_plot_glyph: cannot use antialiased text on 1-bit-mask icons");
				return false;
			}
			rec.mask        = PIcon(self)->mask;
			rec.mask_stride = PIcon(self)->maskLine;
		} else if ( !rec.use_dst_alpha ) {
			rec.use_dst_alpha = true;
			rec.dst_alpha = 0xff;
		}
		rec.func  = plot_blend;
		rec.bpp   = i->type & imBPP;
		rec.bytes = rec.bpp / 8;
	} else {
		warn("img_plot_glyph: cannot use blending on target type=%x", i->type);
		return false;
	}

	w = glyph->w;
	h = glyph->h;
	if ( x + w > i->w ) w = i->w - x - 1;
	if ( y + h > i->h ) h = i->h - y - 1;
	if ( w <= 0 || h <= 0 ) return true;

	img_region_foreach( ctx->region,
		x, y, w, h,
		(RegionCallbackFunc*)plot_glyph, &rec
	);

	return true;
}

#ifdef __cplusplus
}
#endif

