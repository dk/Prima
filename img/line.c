#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VISIBILITY_NONE       0
#define VISIBILITY_CLIPPED    1
#define VISIBILITY_UNSURE     2
#define VISIBILITY_CLEAR      3

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
					img_fill_alpha_buf( mask_buf, mask, dm, bp);
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
		int rop = ctx->rop;
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
		img_find_blend_proc( rop, &rec->blend1, &rec->blend2 );
		rec->is_icon = kind_of( dest, CIcon );

		/* align types and geometry - can only operate over imByte and imRGB */
		int bpp = ( PImage(dest)->type & imGrayScale) ? imByte : imRGB;
		if (PImage(dest)-> type != bpp || ( rec->is_icon && PIcon(dest)->maskType != imbpp8 )) {
			int type = PImage(dest)->type;
			int mask = rec->is_icon ? PIcon(dest)->maskType : 0;

			if ( type != bpp ) {
				img_resample_colors( dest, bpp, ctx );
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
	} else {
		rec->blend1 = rec->blend2 = NULL;
		rec->proc = img_find_blt_proc(ctx->rop);
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
	RegionRec dummy_region;
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

		x = INT_MIN;
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

#ifdef __cplusplus
}
#endif
