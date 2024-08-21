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
				rec-> color, 1, &rec->src_alpha, 0,
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
		if ( !img_find_blend_proc( rop, &rec->blend1, &rec->blend2 )) {
			warn("line: blending rop expected");
			return false;
		}
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
	ImgSegmentedLineRec rec;
	RegionRec dummy_region, *save_region;
	Box dummy_region_box, *pbox;
	Point* pp;
	Rect  enclosure;
	Bool closed;
	ImagePreserveTypeRec p;

	if ( ctx->rop == ropNoOper || n_points <= 1) return true;

	CImage(dest)->begin_preserve_type(dest, &p);
	switch ( hline_init( &rec.h, dest, ctx, "img_polyline")) {
	case HLINE_INIT_RETRY: {
		Bool ok;
		ok = img_polyline( dest, n_points, points, ctx);
		CImage(dest)->end_preserve_type(dest, &p);
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
	save_region = ctx->region;
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
		a.x = pp[0].x;
		a.y = pp[0].y;
		b.x = pp[1].x;
		b.y = pp[1].y;
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
	ctx->region = save_region;
	return true;
}

static NPolyPolyline*
nppl_alloc( NPolyPolyline *old, Bool use_lj_hints, unsigned int new_size)
{
	NPolyPolyline *p;
	unsigned int sz1 = sizeof(NPoint) * new_size;
	unsigned int sz  = sizeof(NPolyPolyline) + sz1;

	if ( use_lj_hints ) sz += sizeof(Byte) * new_size;

	if ( old == NULL ) {
		if ( !( p = malloc(sz)))
			return NULL;
		bzero( p, sz );
	} else {
		int old_size = old->size;
		NPolyPolyline *prev = old->prev;
		/*
		printf("ALLOC ? (%p,%p) <- %p -> (%p,%p)\n",
			old->prev, old->prev ? old->prev->next : NULL,
			old,
			old->next ? old->next->prev : NULL, old->next);
		*/
		if ( new_size < old_size )
			return old;
		if ( !( p = realloc( old, sz )))
			return NULL;
		if (prev)
			prev->next = p;
		if (p->next)
			p->next->prev = p;
		if ( use_lj_hints )
			memmove( p-> buf + sz1, p-> buf + sizeof(NPoint) * old_size, sizeof(Byte) * old_size);
		/*
		printf("ALLOC ! (%p,%p) <- %p -> (%p,%p)\n",
			p->prev, p->prev ? p->prev->next : NULL,
			p,
			p->next ? p->next->prev : NULL, p->next);
		*/
	}
	p->size = new_size;
	p->points = (NPoint*) p->buf;
	if ( use_lj_hints )
		p->lj_hints = p-> buf + sz1;
	p->theta = -1000000.0;
	return p;
}

static void
fill_tangent(NPolyPolyline *p, NPoint prev, NPoint next)
{
	int div = 0;
	double theta = 0.0;
/*
when drawing a line where pattern produces a single point - either
because line pattern has a 1-pixel segment, or a last stroke can
only fix a single pixel - there is a problem with proper calculation
of tangent when widening such a point to a shape. Normally the widening
algorithm is capable of finding out a tangent for plotting line ends that
are rotated correspondingly to the tangent, but for a single point this
doesn't work. So we help it by filling the tangent for such single points
(and only for single points)
*/
	/*  printf("theta? %g.%g - %g.%g - %g.%g\n",
		prev.x, prev.y,
		p->points[0].x, p->points[0].y,
		next.x, next.y); */
	if ( p->points[0].x != prev.x || p->points[0].y != prev.y ) {
		theta += atan2( p->points[0].y - prev.y, p->points[0].x - prev.x);
		div++;
	}
	if ( p->points[0].x != next.x || p->points[0].y != next.y ) {
		theta += atan2( next.y - p->points[0].y, next.x - p->points[0].x);
		div++;
	}
	p->theta = (div > 0) ? theta / div : 0.0;
	/* printf("THETA = %g (%d)\n", p->theta * 360 / 3.14, div); */
}

NPolyPolyline*
img_polyline2patterns( NPoint * points, int n_points, Byte *lj_hints, double line_width, unsigned char * line_pattern, Bool integer_precision)
{
	NPolyPolyline *dst = NULL, *curr = NULL;
	int i, pattern_len;
	float pattern_buf[256], *pattern, sqrt_table[1024];
	semistatic_t pattern_array;
	Bool ok = false, closed, strokecolor, new_point, new_stroke, black, joiner;
	int step;
	float advance, strokelen, pixlen, draw, plotted;
	double dx, dy;
	NPoint a, b, a1, b1, r, last_a, last_b;
	Bool lj_override_flag = false;

	if (integer_precision)
		bzero(sqrt_table, sizeof(sqrt_table));

	/*
	convert line pattern with respect to the line width.
	lpNull results in an empty line, and lpSolid is same as points by definition
	*/
	if (( pattern_len = strlen((char*) line_pattern)) < 2)
		return NULL;
	semistatic_init(&pattern_array, &pattern_buf, sizeof(float), sizeof(pattern_buf) / sizeof(float));
	if ( !semistatic_expand(&pattern_array, pattern_len)) {
		warn("Not enough memory");
		goto EXIT;
	}
	if ( line_width < 1.0) line_width = 1.0;
	pattern = (float*)pattern_array.heap;
	for ( i = 0; i < pattern_len; i++, pattern++)
		*pattern = 1.0 + line_width * ( line_pattern[i] - 1 );
	pattern = (float*)pattern_array.heap;

	closed = points[0].x == points[n_points-1].x && points[0].y == points[n_points-1].y;
	i = step = 0;
	strokecolor = joiner = false;
	new_point = new_stroke = true;
	advance = strokelen = 0.0;
	last_a = a = points[0];
	last_b = b = points[1];
	/* these are not needed, but hush the compiler warning */
	a1.x = a1.y = r.x = r.y = 0;
	plotted = false;
	pixlen = 0.0;

	while ( 1 ) {
		float next_seg_advance;

		/* open next segment */
		if ( advance <= 0.0 && new_stroke ) {
			strokecolor = !strokecolor;
			strokelen   = pattern[step++];
			if ( step >= pattern_len ) step = 0;
			joiner = 0;
			if ( strokecolor ) {
				NPolyPolyline*p;
				if ( !( p = nppl_alloc(NULL, lj_hints != NULL, 32)))
					goto EXIT;
				/* printf("new segment %g.%g %g.%g / %g.%g %g.%g\n", a.x, a.y, b.x, b.y, last_a.x, last_a.y, last_b.x, last_b.y); */
				if ( curr != NULL ) {
					if ( curr-> n_points == 1 )
						fill_tangent(curr, last_a, last_b);
					curr->next = p;
					p->prev = curr;
					curr = p;
				} else
					curr = dst = p;
				/* printf("NEW (%p) <- %p [%p]\n", curr->prev, curr, dst); */
				last_a = a;
				last_b = b;
			}
		}

		/* advance to new point */
		if ( new_point ) {
			double dl;
			if ( lj_hints)
				lj_override_flag = lj_hints[i];
			a = points[i++];
			if ( i >= n_points ) break;
			b = points[i];
			dx = b.x - a.x;
			dy = b.y - a.y;
			dl = dx * dx + dy * dy;
			if ( integer_precision && dl < 1024 ) {
				int ix = dl + .5;
				if ( ix > 0 && sqrt_table[ix] == 0.0 )
					sqrt_table[ix] = sqrtf(ix);
				pixlen = sqrt_table[ix];
			} else
				pixlen = sqrtf(dl);

			if (pixlen > 0.0) {
				r.x = dx / pixlen;
				r.y = dy / pixlen;
			} else
				r.x = r.y = 1.0;
			if ( integer_precision )
				pixlen = floor( pixlen + .5 );
			if (
				( i == n_points - 1 && !closed ) ||
				( pixlen == 0.0 )
			)
				pixlen += 1.0;
			else {
				b.x -= r.x;
				b.y -= r.y;
			}
			a1 = a;
			b1 = b;
			/* printf("np %g.%g - %g.%g\n", a.x, a.y, b.x, b.y); */
			plotted = 0.0;
			if ( joiner && advance == 0.0 && curr && curr-> n_points > 0 )
				curr->n_points--;
			joiner = false;
		}

		/* do we draw? */
		if ( advance > 0.0 ) {
			draw  = advance;
			black = false;
		} else {
			draw  = strokelen;
			black = strokecolor;
		}
		next_seg_advance = black ? line_width - 1.0 : 1.0;

#define ADD_POINT_ENTRY(xx)                                                \
	if ( curr->n_points == 0 ||                                        \
		curr->points[curr->n_points-1].x != xx.x ||                \
		curr->points[curr->n_points-1].y != xx.y) {                \
		curr->points[curr->n_points] = xx;                         \
		if ( lj_hints)                                             \
			curr->lj_hints[curr->n_points] = lj_override_flag; \
		curr->n_points++;                                          \
	}

#define ADD_POINT(aa,bb) \
	if ( black && curr ) { /* curr should be definitely non-NULL by now */ \
		if ( curr->n_points > curr-> size - 2) {                       \
			Bool change_dst = curr == dst;                         \
			if ( !( curr = nppl_alloc(curr, lj_hints != NULL, curr->size * 2)))  \
				goto EXIT;                                     \
			if (change_dst) dst = curr;                            \
		}                                                              \
		ADD_POINT_ENTRY(aa);                                           \
		ADD_POINT_ENTRY(bb);                                           \
	}

		if ( draw < pixlen ) {
			/* normal line segment, ends before to pattern segment */
			plotted += draw;
			if ( draw > 1.0 ) {
				b1.x = (plotted - 1.0) * r.x + a.x;
				b1.y = (plotted - 1.0) * r.y + a.y;
			} else
				b1 = a1;
			ADD_POINT(a1,b1);
			pixlen -= draw;
			advance += (advance > 0.0) ? -draw : next_seg_advance;
			a1.x = b1.x + r.x;
			a1.y = b1.y + r.y;
			new_point = false;
			new_stroke = true;
		} else if ( draw == pixlen ) {
			/* exact match that ends line by pattern end */
			ADD_POINT(a1,b);
			new_stroke = new_point = true;
			advance += (advance > 0.0) ? -draw : next_seg_advance;
			joiner = black;
		} else if ( black && draw == 1.0 && pixlen <= 0 ) {
			/* skip tail */
			new_stroke = new_point = true;
			advance = next_seg_advance;
		} else {
			/* also normal line end, ends after pattern segment, join and continue */
			ADD_POINT(a1,b);
			new_point = true;
			new_stroke = false;
			if ( advance > 0 )
				advance -= pixlen;
			else {
				strokelen -= pixlen;
				joiner = black;
			}
		}
#undef ADD_POINT
	}
	if ( curr-> n_points == 1 )
		fill_tangent(curr, last_a, last_b);

	/* finalize */
	if ( curr && curr-> n_points == 0) {
		NPolyPolyline *p = curr->prev;
		free(curr);
		curr = p;
		if ( p ) p->next = NULL;
		if ( dst == curr ) dst = NULL;
	}

	/* merge the tail with the head if they collide */
	if ( closed && pattern[0] > 1.0 && strokelen > 1.0 && curr != dst ) {
		NPolyPolyline *p = dst;
		dst = dst->next;
		if ( p->n_points > curr-> size - curr-> n_points) {
			if ( !( curr = nppl_alloc(curr, lj_hints != NULL, curr->size + p-> n_points)))
				goto EXIT;
		}
		memcpy( curr-> points + curr->n_points, p-> points, p->n_points * sizeof(NPoint));
		if ( curr-> lj_hints)
			memcpy( curr-> lj_hints + curr-> n_points, p-> lj_hints, p->n_points * sizeof(Byte));
		curr-> n_points += p-> n_points;
		free( p );
	}
	ok = true;

/*
	{
		int n = 0;
		NPolyPolyline* p = dst;
		if ( !p ) printf("#0: [NULL]\n");
		while (p) {
			int i;
			printf("#%d: ", n++);
			for ( i = 0; i < p->n_points; i++) printf("%g.%g ", p->points[i].x, p->points[i].y);
			p = p->next;
			printf("\n");
		}
	}
*/

EXIT:
	if ( !ok ) {
		while (dst) {
			NPolyPolyline* p = dst->next;
			free(dst);
			dst = p;
		}
		dst = NULL;
	}
	semistatic_done(&pattern_array);
	return dst;
}

#ifdef __cplusplus
}
#endif
