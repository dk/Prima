#include "img_conv.h"
#include "Image.h"

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
		/* skip pixels */
		if ( curr == 0 ) {
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
				register unsigned int n = (sizeof(buf) > bits) ? bits : sizeof(buf), m = 0;
				while (n-- > bytes - 1) {
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
	Byte *src, *dst;
	unsigned int src_stride, dst_stride;
	Byte *color;
	Rect r;

	unsigned int bpp, bytes;
	PlotFunc *func;
	BitBltProc* blt;
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

void
img_plot_glyph( Handle self, PImage glyph, int x, int y, PImgPaintContext ctx)
{
	PImage i = (PImage) self;
	int w, h;
	Bool mono = (glyph->type & imBPP) == 1;

	PlotStruct rec = {
		glyph, i,
		glyph->data, i->data,
		glyph->lineSize, i->lineSize,
		ctx->color,
		{ x, y, x + glyph->w - 1, y + glyph->h - 1 }
	};

	if (glyph-> w == 0 || glyph-> h == 0)
		return;

	if ( ctx->rop <= ropNoOper ) {
		if ( !mono ) {
			warn("img_plot_glyph: cannot use bitwise rops with antialiased glyphs");
			return;
		}
		rec.blt   = img_find_blt_proc(ctx->rop);
		rec.func  = plot_rop;
		rec.bpp   = i->type & imBPP;
		rec.bytes = rec.bpp / 8;
	} else {
		warn("img_plot_glyph: target type=%x rop=%x is not implemented", i->type, ctx->rop);
	}

	w = glyph->w;
	h = glyph->h;
	if ( x + w > i->w ) w = i->w - x - 1;
	if ( y + h > i->h ) h = i->h - y - 1;

	img_region_foreach( ctx->region,
		x, y, w, h,
		(RegionCallbackFunc*)plot_glyph, &rec
	);
}

#ifdef __cplusplus
}
#endif

