#include <stdlib.h>
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define _DEBUG
#define DEBUG if(1)printf
#else
#define DEBUG if(0)printf
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif


Bool
img_region_foreach(
	PRegionRec region, 
	int dstX, int dstY, int dstW, int dstH,
	RegionCallbackFunc callback, void * param
) {
	Box * r;
	int j, right, top;
	if ( region == NULL )
		return callback( dstX, dstY, dstW, dstH, param);
	right = dstX + dstW;
	top   = dstY + dstH;
	r = region-> boxes;
	for ( j = 0; j < region-> n_boxes; j++, r++) {
		int xx = r->x;
		int yy = r->y;
		int ww = r->width;
		int hh = r->height;
		if ( xx + ww > right ) ww = right - xx;
		if ( yy + hh > top   ) hh = top   - yy;
		if ( xx < dstX ) {
			ww -= dstX - xx;
			xx = dstX;
		}
		if ( yy < dstY ) {
			hh -= dstY - yy;
			yy = dstY;
		}
		if ( xx + ww >= dstX && yy + hh >= dstY && ww > 0 && hh > 0 )
			if ( !callback( xx, yy, ww, hh, param ))
				return false;
	}
	return true;
}

PRegionRec
img_region_alloc(PRegionRec old_region, int n_size)
{
	PRegionRec ret = NULL;
	ssize_t size = sizeof(RegionRec) + n_size * sizeof(Box);
	if ( old_region ) {
		if ( n_size <= old_region->size )
			return old_region;
		if (( ret = realloc(old_region, size)) == NULL)
			return NULL;
	} else {
		if (( ret = malloc(size)) == NULL)
			return NULL;
		bzero(ret, sizeof(RegionRec));
	}
	ret->boxes = (Box*) (((Byte*)ret) + sizeof(RegionRec));
	ret->size  = n_size;
	return ret;
}

PRegionRec
img_region_extend(PRegionRec region, int x, int y, int width, int height)
{
	Box *r;

	if ( !region ) {
		if ( !( region = img_region_new( 32 )))
			return NULL;
	}

	if ( region-> size == region-> n_boxes ) {
		PRegionRec old = region;
		if ( !( region = img_region_alloc( old, region-> size * 3 ))) {
			free( old );
			return NULL;
		}
	}

	r = region->boxes + region->n_boxes;
	r->x = x;
	r->y = y;
	r->width = width;
	r->height = height;
	region-> n_boxes++;
	DEBUG("%d: + %d %d %d %d\n", region->n_boxes, x, y, width, height);
	return region;
}

Box
img_region_box(PRegionRec region)
{
	int i, n = 0;
	Box ret, *curr = NULL;
	Rect r;

	if ( region != NULL && region-> n_boxes > 0 ) {
		n        = region-> n_boxes;
		curr     = region->boxes;
		r.left   = curr->x;
		r.bottom = curr->y;
		r.right  = curr->x + curr->width;
		r.top    = curr->y + curr->height;
		curr++;
	} else
		bzero(&r, sizeof(r));

	for ( i = 1; i < n; i++, curr++) {
		int right = curr->x + curr->width, top = curr->y + curr->height;
		if ( curr-> x < r.left)   r.left   = curr->x;
		if ( curr-> y < r.bottom) r.bottom = curr->y;
		if ( right    > r.right)  r.right  = right;
		if ( top      > r.top  )  r.top    = top;
	}
	ret.x      = r.left;
	ret.y      = r.bottom;
	ret.width  = r.right - r.left;
	ret.height = r.top   - r.bottom;
	return ret;
}

Bool
img_point_in_region( int x, int y, PRegionRec region)
{
	int i;
	Box * b;
	for ( i = 0, b = region->boxes; i < region->n_boxes; i++, b++) {
		if ( x >= b->x && y >= b->y && x < b->x + b->width && y < b->y + b->height)
			return true;
	}
	return false;
}

PRegionRec
img_region_mask( Handle mask)
{
	unsigned long w, h, x, y, count = 0;
	Byte	   * idata;
	Box * current;
	PRegionRec rdata;
	Bool	  set = 0;

	if ( !mask)
		return NULL;

	w = PImage( mask)-> w;
	h = PImage( mask)-> h;
	idata  = PImage( mask)-> data;

	if ( !( rdata = img_region_new(256)))
		return NULL;

	count = 0;
	current = rdata-> boxes;
	current--;

	for ( y = 0; y < h; y++) {
		for ( x = 0; x < w; x++) {
			if ( idata[ x >> 3] == 0) {
				x += 7;
				continue;
			}
			if ( idata[ x >> 3] & ( 1 << ( 7 - ( x & 7)))) {
				if ( set && current-> y == y && current-> x + current-> width == x)
					current-> width++;
				else {
					PRegionRec xrdata;
					set = 1;
					if ( !( xrdata = img_region_extend( rdata, x, y, 1, 1)))
						return NULL;
					if ( xrdata != rdata ) {
						rdata = xrdata;
						current = rdata->boxes;
						current += count - 1;
					}
					count++;
					current++;
				}
			}
		}
		idata += PImage( mask)-> lineSize;
	}

	return rdata;
}

void
img_region_offset( PRegionRec region, int dx, int dy)
{
	int j;
	Box *r;
	if ( region == NULL )
		return;
	for ( j = 0, r = region->boxes; j < region-> n_boxes; j++, r++) {
		r-> x += dx;
		r-> y += dy;
	}
}

static int
region_cmp( const void * a, const void * b)
{
	if ( ((PBox)a)->y > ((PBox)b)->y)
		return 1;
	else if ( ((PBox)a)->y < ((PBox)b)->y)
		return -1;
	if ( ((PBox)a)->x > ((PBox)b)->x)
		return 1;
	else if ( ((PBox)a)->x < ((PBox)b)->x)
		return -1;
	else
		return 0;
}

void
img_region_sort( PRegionRec region )
{
	if ( region->flags & rgnSorted ) 
		return;
	qsort( region-> boxes, region-> n_boxes, sizeof(Box), region_cmp);
	region->flags |= rgnSorted;
}

/*
In the list, each entry is 2 indexes to prev and next box that cross the current scanline.
Each new scanline updates the list by rearranging these indexes
*/
PRegionScanlineIterator
img_region_iterate_scanline( PRegionRec region )
{
	PRegionScanlineIterator ret;

	if ( region == NULL || region->n_boxes == 0 )
		return NULL;

	if ( !( ret = malloc( sizeof(RegionScanlineIterator) + sizeof(unsigned int) * 2 * region->n_boxes))) {
		warn("no memory");
		return NULL;
	}

	ret->region     = region;
	ret->y          = region->boxes[0].y - 1;
	ret->null_index = region->n_boxes;
	ret->head       = ret->null_index;
	ret->current    = 0;

	img_region_sort(region);
	DEBUG("ITER(%d) FROM %d\n", region->n_boxes, ret->y);

	return ret;
}

static void
insert_box( PRegionScanlineIterator i, unsigned int where, unsigned int who)
{
	if ( where == i->head ) {
		i->list[who  * 2    ] = i->null_index; /* prev */
		i->list[who  * 2 + 1] = i->head;       /* next */
		i->head                = who;
	} else if ( where == i->null_index ) {
		unsigned int w = i->head;
		while (1) {
			unsigned int next = i->list[ w * 2 + 1 ];
			if ( next != i-> null_index ) {
				w = next;
				continue;
			}
			i->list[w    * 2 + 1] = who;
			i->list[who  * 2    ] = w;
			i->list[who  * 2 + 1] = i->null_index;
			break;
		}
	} else {
		unsigned int prev = i->list[where * 2];
		i->list[prev * 2 + 1] = who;
		i->list[who  * 2    ] = prev;
		i->list[who  * 2 + 1] = where;
		i->list[where * 2   ] = who;
	}
#ifdef _DEBUG
	{
		int x;
		x = i->head;
		printf("ins(%d %d)(%d %d):", i->region->n_boxes, i->y, where, who);
		while ( x != i->null_index ) {
			printf(" %d", x);
			x = i->list[ x * 2 + 1 ];
		}
		printf("\n");
	}
#endif
}

Bool
img_region_next_scanline(PRegionScanlineIterator i)
{
	PRegionRec r  = i->region;
	int     oy = i->y, ny = i->y + 1;
	unsigned int next;

	DEBUG("NEXT FROM(%d): %d->%d @ %d\n", r->n_boxes, oy, ny, i->current);

	/* advance the boxes in the linked list, remove if needed */
	next = i->head;
	while ( next != i->null_index ) {
		Box *b = r->boxes + next;
		if ( b->y + b->height - 1 == oy ) {
			/* leave old box */
			unsigned int j    = next * 2;
			unsigned int prev = i->list[j];
			DEBUG("OLD BOX(%d): [%d] %d -> %d\n", r->n_boxes, prev, next, i->list[j+1]);
			next = i->list[j+1];
			if ( prev == i-> null_index )
				i->head = next;
			else
				i->list[prev * 2 + 1] = next;
			if ( next != i-> null_index )
				i->list[next * 2] = prev;
		} else
			next = i->list[ next * 2 + 1 ];
	}

	/* scan for new boxes to enter he linked list */
	while ( 1 ) {
		Box * b;
		if ( i->current >= r->n_boxes ) {
			if ( i->head == i->null_index ) {
				DEBUG("STOP(%d)\n", r->n_boxes);
				return false;
			}
			i->y++;
			DEBUG("EOL2(%d) > %d\n", r->n_boxes, i->y);
			return true;
		}

		b = r->boxes + i->current;
		if ( b->y == ny ) {
			/* insert new box sorted by X */
			Bool inserted = false;

			next = i->head;
			while ( next != i-> null_index ) {
				if ( b->x > r->boxes[next].x ) {
					next = i->list[ next * 2 + 1 ];
					continue;
				} else {
					insert_box(i, next, i->current);
					inserted = true;
					break;
				}
			}
			if ( !inserted )
				insert_box(i, next, i->current);
			DEBUG("INSERT BOX(%d) %d: %d %d %d %d\n", r->n_boxes, i->current, b->x, b->y, b->width, b->height);
		} else if ( b->y > oy ) {
			i->y++;
			DEBUG("EOL(%d) > %d, curr=%d (%d %d %d %d)\n", r->n_boxes, i->y, i->current, b->x, b->y, b->width, b->height);
			return true;
		}
		i->current++;
		b++;
		if (i->current < r->n_boxes) {
			DEBUG("CURR(%d) :%d @ (%d %d %d %d)\n", r->n_boxes, i->current, b->x, b->y, b->width, b->height);
		} else {
			DEBUG("CURR(%d) :%d @ EOL\n", r->n_boxes, i->current);
		}
	}
}

#define SCAN_PTR(i)   ptr##i
#define dSCAN_ITER(i) unsigned int SCAN_PTR(i) = i->head
#define SCAN_COND(i)  (SCAN_PTR(i) != i-> null_index)
#define SCAN_NEXT(i)  SCAN_PTR(i) = i->list[ SCAN_PTR(i) * 2 + 1 ]
#define SCAN_CURR(i)  (i->region->boxes + SCAN_PTR(i))

void
img_region_fill_scanline_map(PRegionScanlineIterator i, Byte *map, int map_offset, int map_width)
{
	dSCAN_ITER(i);

	bzero(map, map_width);
	while ( SCAN_COND(i)) {
		Box *b = SCAN_CURR(i);
		int l  = b->x - map_offset;
		int r  = l + b->width + 1;
		if ( l < map_width && r > 0 ) {
			if ( l < 0 ) l = 0;
			if ( r > map_width ) r = map_width + 1;
			if ( r - l > 1 ) memset( map + l, 0xff, r - l - 1);
		}
		SCAN_NEXT(i);
	}
}

static PRegionRec
rgn_copy( PRegionRec r )
{
	PRegionRec ret;
	if ( r == NULL )
		return NULL;
	if ( !( ret = img_region_new( r->n_boxes )))
		return NULL;
	memcpy( ret->boxes, r->boxes, r->n_boxes * sizeof(Box));
	ret->n_boxes = r->n_boxes;
	return ret;
}

static Bool
copy_scanline( PRegionScanlineIterator i, PRegionRec *ret)
{
	PRegionRec r2;
	dSCAN_ITER(i);
	while (SCAN_COND(i)) {
		Box *b = SCAN_CURR(i);
		r2 = img_region_extend(*ret, b->x, i->y, b->width, 1);
		SCAN_NEXT(i);
		if ( r2 == NULL )
			return false;
		*ret = r2;
	}
	return true;
}

static Bool
add_scanline( int x, int y, int w, PRegionRec* ret)
{
	PRegionRec r2 = *ret;
	if ( r2->n_boxes > 0 ) {
		int x2;
		Box *last = r2->boxes + r2->n_boxes - 1;
		x2 = last->x + last->width;
		if ( last->y == y && x2 >= x ) {
			/* merge */
			DEBUG("merge %d %d %d %d + %d %d = %d ", last->x, last->y, last->width, last->height, x, w, last->x);
			if ( x2 < x + w)
				last-> width = x + w - last->x;
			DEBUG("%d\n", last->width);
			return true;
		}
	}
	/* add */
	if ( !( r2 = img_region_extend(*ret,x,y,w,1)))
		return false;
	if ( r2 != *ret ) *ret = r2;
	return true;
}

static Bool
union_scanlines( PRegionScanlineIterator i1, PRegionScanlineIterator i2, PRegionRec *ret)
{
	dSCAN_ITER(i1);
	dSCAN_ITER(i2);

	while ( 1 ) {
		int x, w;
		if ( SCAN_COND(i1)) {
			Box *b1 = SCAN_CURR(i1);
			if ( SCAN_COND(i2)) {
				Box *b2 = SCAN_CURR(i2);
				if ( b1->x + b1->width < b2->x - 1 ) {
					/* case 1, b1 is fully before b2 */
					DEBUG("case 1.1\n");
					x = b1->x;
					w = b1->width;
					SCAN_NEXT(i1);
				} else if ( b2->x + b2->width < b1-> x - 1 ) {
					/* case 2, b2 is fully before b1 */
					DEBUG("case 2.1\n");
					x = b2->x;
					w = b2->width;
					SCAN_NEXT(i2);
				} else {
					/* merge */
					int r;
					x = MIN(b1->x, b2->x);
					r = MAX(b1->x + b1->width, b2->x + b2->width);
					w = r - x;
					DEBUG("case 3 (%d %d / %d %d) = (%d %d)\n", b1->x, b1->x + b1->width, b2->x, b2->x + b2->width, x, w);
					SCAN_NEXT(i1);
					SCAN_NEXT(i2);
				}
			} else {
				/* tail of b1 */
				DEBUG("case 1.2\n");
				x = b1->x;
				w = b1->width;
				SCAN_NEXT(i1);
			}
		} else if ( SCAN_COND(i2)) {
			/* tail of b2 */
			Box *b2 = SCAN_CURR(i2);
			DEBUG("case 2.2\n");
			x = b2->x;
			w = b2->width;
			SCAN_NEXT(i2);
		} else
			break;

		if (!add_scanline(x,i1->y,w,ret))
			return false;
	}


	return true;
}

static Bool
intersect_scanlines( PRegionScanlineIterator i1, PRegionScanlineIterator i2, PRegionRec *ret)
{
	dSCAN_ITER(i1);
	dSCAN_ITER(i2);

	while ( 1 ) {
		int x, w;
		if ( SCAN_COND(i1)) {
			Box *b1 = SCAN_CURR(i1);
			if ( SCAN_COND(i2)) {
				Box *b2 = SCAN_CURR(i2);
				if ( b1->x + b1->width < b2->x - 1 ) {
					/* case 1, b1 is fully before b2 */
					DEBUG("case 1.1\n");
					SCAN_NEXT(i1);
					continue;
				} else if ( b2->x + b2->width < b1-> x - 1 ) {
					/* case 2, b2 is fully before b1 */
					DEBUG("case 2.1\n");
					SCAN_NEXT(i2);
					continue;
				} else {
					int r1, r2;
					/* case 3, one ends before another */
					x = MAX(b1->x, b2->x);
					r1 = b1->x + b1->width;
					r2 = b2->x + b2->width;
					w = MIN(r1,r2) - x;
					if ( r1 <= r2 )
						SCAN_NEXT(i1);
					if ( r1 >= r2 )
						SCAN_NEXT(i2);
					DEBUG("case 3 (%d %d / %d %d) = (%d %d)\n", b1->x, r1, b2->x, r2, x, w);
				}
			} else {
				/* tail of b1 */
				DEBUG("case 1.2\n");
				SCAN_NEXT(i1);
				continue;
			}
		} else if ( SCAN_COND(i2)) {
			/* tail of b2 */
			DEBUG("case 2.2\n");
			SCAN_NEXT(i2);
			continue;
		} else
			break;

		if (!add_scanline(x,i1->y,w,ret))
			return false;
	}

	return true;
}

static void
img_region_compress(PRegionRec r)
{
	int i, y0, y1, y0_start, y0_end, null_y, skipped = 0;
	Box *b0 = r->boxes, *b1 = r->boxes;

	if ( r->n_boxes < 2 )
		return;

	img_region_sort(r);

	y1 = b0->y;
	null_y = y0 = y1 - 1;
	if ( null_y >= 0 ) null_y = -1; /* for debug */
	y0_start = 0;
	y0_end   = -1;

	for ( i = 0; i < r->n_boxes; i++, b1++) {
		DEBUG("? b0=%d %d %d %d / b1=%d %d %d %d / y0=%d y1=%d s=%d e=%d\n",
			b0->x, b0->y, b0->width, b0->height,
			b1->x, b1->y, b1->width, b1->height,
			y0, y1, y0_start, y0_end
			);
		if ( b1-> y == y1 ) {
			while ( b0->x <= b1->x && y0_start <= y0_end ) {
				y0_start++;
			DEBUG("?? b0=%d %d %d %d / b1=%d %d %d %d / y0=%d y1=%d s=%d e=%d\n",
				b0->x, b0->y, b0->width, b0->height,
				b1->x, b1->y, b1->width, b1->height,
				y0, y1, y0_start, y0_end
				);
				if ( b0->x == b1->x && b0->width == b1->width && b0->y != null_y ) {
					DEBUG("merge %p %d %d %d %d + %p %d %d %d %d = ", 
						b0, b0->x, b0->y, b0->width, b0->height,
						b1, b1->x, b1->y, b1->width, b1->height
						);
					b1->y = b0->y;
					b1->height = b0->height + 1;
					b0->y = null_y;
					b0++;
					skipped++;
					DEBUG("%d %d %d %d\n", b1->x, b1->y, b1->width, b1->height);
					break;
				}
				b0++;
			}
		} else if ( b1-> y == y1 + 1 ) {
			y0++;
			y1++;
			y0_start = y0_end + 1;
			y0_end   = i - 1;
			b0       = r->boxes + y0_start;
			i--;
			b1--;
			DEBUG("adjacent scan y0=%d s=%d e=%d\n", y0, y0_start, y0_end);
		} else if ( b1-> y > y1 ) {
			y0_start = i;
			y0_end = i;
			y0 = b1->y;
			y1 = y0 + 1;
			b0 = b1;
			DEBUG("new scan y0=%d s=%d e=%d\n", y0, y0_start, y0_end);
		} else {
			y0_end++;
		}

	}

	if ( skipped == 0 )
		return;

	b0 = r->boxes;
	b1 = r->boxes + r->n_boxes - 1;
	r->n_boxes -= skipped;
	r->flags &= ~rgnSorted;
	if ( r->n_boxes == 0 )
		return;

	while (b0 < b1) {
		if ( b1->y == null_y )
			b1--;
		else if ( b0->y == null_y )
			*b0++ = *b1--;
		else
			b0++;
	}
}

static PRegionRec
rgn_apply( PRegionRec rgn1, PRegionRec rgn2, int rop )
{
	PRegionScanlineIterator i1 = NULL, i2 = NULL;

	Bool ok = false;
	PRegionRec ret = NULL;

#define SCAN_COPY(i) if (rop == rgnopUnion && !copy_scanline(i, &ret)) goto EXIT

	if ( !( ret = img_region_new( rgn1-> n_boxes + rgn2-> n_boxes )))
		goto EXIT;
	if ( !( i1 = img_region_iterate_scanline(rgn1)))
		goto EXIT;
	if ( !( i2 = img_region_iterate_scanline(rgn2)))
		goto EXIT;
	DEBUG("ENTER %s: %d %d\n", (rop == rgnopUnion) ? "UNION" : "INTERSECT", i1->y, i2->y);

	/* if bottom parts are unequal, copy them */
	while ( i1->y < i2->y ) {
		if ( !img_region_next_scanline(i1))
			break;
		DEBUG("%s 1: %d %d\n", (rop == rgnopUnion) ? "COPY" : "SKIP", i1->y, i2->y);
		SCAN_COPY(i1);
	}
	while ( i2->y < i1->y ) {
		if ( !img_region_next_scanline(i2))
			break;
		SCAN_COPY(i2);
	}

	/* run parts that intersect by Y */
	while ( 1 ) {
		/* do they end here? */
		if ( !img_region_next_scanline(i1))
			break;
		if ( !img_region_next_scanline(i2)) {
			SCAN_COPY(i1);
			break;
		}
		/* are they empty? */
		{
			dSCAN_ITER(i1);
			dSCAN_ITER(i2);
			if ( !SCAN_COND(i1) || !SCAN_COND(i2)) {
				if ( SCAN_COND(i1)) {
					SCAN_COPY(i1);
				} else if ( SCAN_COND(i2)) {
					SCAN_COPY(i2);
				}
				continue;
			}
		}

		/* union them now */
		if ( rop == rgnopUnion ) {
			if ( !union_scanlines(i1, i2, &ret))
				goto EXIT;
		} else {
			if ( !intersect_scanlines(i1, i2, &ret))
				goto EXIT;
		}
	}

	/* again, if the top parts are unequal, copy them too */
	while ( img_region_next_scanline(i1)) {
		SCAN_COPY(i1);
	}
	while ( img_region_next_scanline(i2)) {
		SCAN_COPY(i2);
	}

	ok = true;

EXIT:

	if (ok)
		img_region_compress(ret);
	if ( !ok && ret ) {
		free(ret);
		ret = NULL;
	}
	if (i1) free(i1);
	if (i2) free(i2);
	return ret;

#undef SCAN_COPY
}

PRegionRec
img_region_combine( PRegionRec rgn1, PRegionRec rgn2, int rop)
{
	if (rgn1) img_region_sort(rgn1);
	if (rgn2) img_region_sort(rgn2);
	switch ( rop ) {
	case rgnopCopy:
		return rgn_copy(rgn1);
	case rgnopUnion:
		if ( rgn1 == NULL )
			return rgn_copy(rgn2);
		else if ( rgn2 == NULL )
			return rgn_copy(rgn1);
		return rgn_apply(rgn1, rgn2, rgnopUnion);
	case rgnopIntersect:
		if ( rgn1 == NULL || rgn2 == NULL )
			return NULL;
		return rgn_apply(rgn1, rgn2, rgnopIntersect);
	default:
		warn("img_region_combine(rop=%d) is unimplmented", rop);
		return NULL;
	}
}

/* special case, a polyline that is a single scanline - alternating fill is not handled (should it at all?) */
static Bool
is_hline( Point *pts, int count, Box *hliner)
{
	int i;
	hliner->x      = pts[0].x;
	hliner->y      = pts[0].y;
	hliner->height = 1;
	hliner->width  = 1;
	for ( i = 1, pts++; i < count; i++, pts++) {
		if ( pts->y != hliner->y ) return false;
		if ( pts->x < hliner->x ) {
			hliner-> width += hliner->x - pts->x;
			hliner->x = pts->x;
		} else if ( pts->x >= hliner->x + hliner->width ) {
			hliner->width += pts->x - hliner->x - hliner->width + 1;
		}
	}

	return true;
}

static Bool
is_rect( Point *pts, int count, int outline, Box *single)
{
	if (
		(
			(count == 4) || (
				count == 5           &&
				pts[4].x == pts[0].x &&
				pts[4].y == pts[0].y
			)
		) && (
			(
				pts[0].y == pts[1].y &&
				pts[1].x == pts[2].x &&
				pts[2].y == pts[3].y &&
				pts[3].x == pts[0].x
			) || (
				pts[0].x == pts[1].x &&
				pts[1].y == pts[2].y &&
				pts[2].x == pts[3].x &&
				pts[3].y == pts[0].y)
		)
	) {
		int x2, y2;
		single->x = MIN(pts[0].x, pts[2].x);
		single->y = MIN(pts[0].y, pts[2].y);
		x2 = MAX(pts[0].x, pts[2].x);
		y2 = MAX(pts[0].y, pts[2].y);
		if ( !outline ) {
			x2--;
			y2--;
		}
		single->width  = x2 - single->x + 1;
		single->height = y2 - single->y + 1;
		return true;
	}

	return false;
}

static PRegionRec
rect_region( Box * box)
{
	PRegionRec reg;
	if ( !( reg = img_region_new(1)))
		return NULL;
	reg->n_boxes= 1;
	reg->boxes[0] = *box;
	return reg;
}

static void
populate_scanline2box( PRegionRec region, int *scanline2box )
{
	int i,j,ly,ymin;
	Box *box;

	ymin = region-> boxes[0].y;
	DEBUG("scanlines:");
	for (
		i = j = 0, ly = ymin - 1, box = region->boxes;
		i < region->n_boxes;
		i++, box++
	) {
		if ( box->y != ly ) {
			scanline2box[j++] = i;
			DEBUG("%d=%d ", j-1, i);
		}
		ly = box->y;
	}
	DEBUG("\n");
}

static PRegionRec
add_hline( PRegionRec region, int *scanline2box, int x, int y, int width )
{
	int ymax, ymin;
	if ( region-> n_boxes == 0 ) {
		scanline2box[0] = 0;
		DEBUG("new rgn\n");
		return img_region_extend(region, x, y, width, 1);
	}
	ymin = region-> boxes[0].y;
	ymax = region-> boxes[region-> n_boxes - 1].y;

	/* no holes, better return intact */
	if ( y != ymin - 1 && y != ymax + 1)
		return region;

	if ( y == ymin - 1 ) {
		Box* box;
		/* stuff before */
		if ( !( region = img_region_extend(region, 0, 0, 0, 0)))
			return NULL;
		box = region->boxes;
		memmove( box + 1, box, (region-> n_boxes - 1) * sizeof(Box));
		box-> x      = x;
		box-> y      = y;
		box-> width  = width;
		box-> height = 1;
		populate_scanline2box(region, scanline2box);
		DEBUG("stuff before\n");
		return region;
	} else {
		/* stuff after */
		scanline2box[ ymax - ymin + 1] = region->n_boxes;
		DEBUG("stuff after; scanline(%d) = %d\n", ymax-ymin+1, region->n_boxes);
		return img_region_extend(region, x, y, width, 1);
	}
}

static PRegionRec
union_hline( PRegionRec region, int *scanline2box, int x, int y, int width )
{
	int i, ymin, ymax, box_offset;
	Box *box;
	DEBUG("add %d %d %d\n", x, y, width);
	if ( region-> n_boxes == 0 )
		return add_hline( region, scanline2box, x, y, width );
	ymin = region-> boxes[0].y;
	ymax = region-> boxes[region-> n_boxes - 1].y;
	if ( y < ymin || y > ymax )
		return add_hline( region, scanline2box, x, y, width );

	/* expand hline, if any, strictly by 1 pixel left or right */
	box_offset = scanline2box[y - ymin];
	box = region->boxes + box_offset;
	for ( i = box_offset; i < region-> n_boxes && box->y == y; i++, box++) {
		int r1 = box-> x + box-> width;
		int r2 = x + width;
		if ( x >= box->x && x <= r1 + 1 ) {
			if ( r2 > r1 ) {
				DEBUG("add right: w %d -> %d\n", box->width, box-> width + r2 - r1);
				box->width += r2 - r1;
			}
			return region;
		} else if ( x < box->x && r2 >= box-> x - 1) {
			DEBUG("add left: %d %d -> ", box->x, box->width);
			if ( x < box->x )
				box->x = x;
			if ( r2 < r1 ) r2 = r1;
			box->width = r2 - box->x;
			DEBUG("%d %d\n", box->x, box->width);
			return region;
		}
	}

	/* need to insert a rectangle and recalculate the scanline2box */
	DEBUG("insert\n");
	if ( !( region = img_region_alloc( region, region->size * 2)))
		return NULL;
	box = region->boxes + box_offset;
	memmove( box + 1, box, sizeof(Box) * (region->n_boxes - box_offset));
	box->x = x;
	box->y = y;
	box->width  = width;
	box->height = 1;
	region->n_boxes++;
	populate_scanline2box(region, scanline2box);

	return region;
}

static PRegionRec
superimpose_outline( PRegionRec region, Point *pts, int count)
{
	int i, ymin, ymax, n_scanlines, *scanline2box;

	/* fill quick access table. It might be extended with new scanlines by y, but never
	with new entries by x */
	if ( region-> n_boxes > 0 ) {
		ymin = region-> boxes[0].y;
		ymax = region-> boxes[region-> n_boxes - 1].y;
		n_scanlines = ymax - ymin + 1;
		DEBUG("ymin %d ymax %d\n",ymin, ymax);
	} else {
		n_scanlines = 0;
		ymin = ymax = 0;
	}
	/* space for 1-2 extra scanlines (1 up, 1 down), but never more, most probably will be needed */
	if ( !( scanline2box = malloc((n_scanlines + 2) * sizeof(int))))
		return region;
	populate_scanline2box(region, scanline2box);

	/* superimpose either horizontal segments or individual vertexes as single pixels */
	for ( i = 0; i < count; i++) {
		Point a = pts[i], b = pts[(i == count - 1) ? 0 : i + 1];
		if ( a.y == b.y ) {
			if ( a.x > b.x ) {
				int z = a.x;
				a.x = b.x;
				b.x = z;
			}
			DEBUG("edge %d.%d-%d.%d\n", a.x,a.y,b.x,b.y);
			if (!( region = union_hline( region, scanline2box, a.x, a.y, b.x - a.x + 1)))
				goto EXIT;
		} else {
			DEBUG("vertex %d.%d\n", a.x,a.y);
			if (!( region = union_hline( region, scanline2box, a.x, a.y, 1)))
				goto EXIT;
		}
	}

EXIT:
	free(scanline2box);
	return region;
}

static PRegionRec
points2region( PolyPointBlock *block, int outline)
{
	register Box  *rects;
	register Point *pts;
	register int i;
	int numRects;
	PRegionRec reg;

	numRects = block->size;
	if ( !( reg = img_region_new(numRects * 2 + outline)))
		return NULL;

	rects = reg->boxes - 1;
	numRects = 0;

	/* the loop uses 2 points per iteration */
	i = block->size >> 1;
	for (pts = block->pts; i--; pts += 2) {
		DEBUG("i=%d %d.%d - %d.%d\n", i, pts->x, pts->y, pts[1].x, pts[1].y);
		if (numRects &&
			pts->x == rects->x &&
			pts->y == rects->y + rects->height - 1 &&
			pts[1].x == rects->x + rects->width - 1 &&
			(
				numRects == 1 ||
				rects[-1].y != rects->y
			) && (
				i && pts[2].y > pts[1].y
			)
		) {
			rects->height = pts[1].y - rects->y + 1;
			DEBUG("update x=%d y=%d w=%d h=%d\n", rects->x, rects->y, rects->width, rects->height);
			continue;
		}
		numRects++;
		rects++;
		rects->x = pts->x;
		rects->y = pts->y;
		rects->width  = pts[1].x - pts-> x + outline;
		rects->height = pts[1].y - pts-> y + 1;
		if ( rects-> width < 0  ) {
			rects->x     +=  rects-> width;
			rects->width  = -rects-> width;
		}
		if ( rects-> height < 0 ) {
			rects->y     +=  rects-> height;
			rects->height = -rects-> height;
		}
		DEBUG("insert x=%d y=%d w=%d h=%d\n", rects->x, rects->y, rects->width, rects->height);
	}

	reg->n_boxes = numRects;

	return reg;
}

PRegionRec
img_region_polygon( Point *pts, int count, int rule)
{
	PRegionRec region;
	PolyPointBlock * pt_block;
	Bool outline;
	Box single;

	outline = (rule & fmOverlay) ? 1 : 0;

	if (count < 2) {
		DEBUG("not enough points");
		return img_region_new(0);
	}

	if (is_hline( pts, count, &single)) {
		if ( !outline ) {
			DEBUG("got invisible hline");
			return img_region_new(0);
		}
		DEBUG("got hline %d %d %d %d\n", single.x, single.y, single.width, single.height);
		return rect_region(&single);
	}

	if (is_rect( pts, count, outline, &single)) {
		DEBUG("got rect %d %d %d %d\n", single.x, single.y, single.width, single.height);
		return rect_region(&single);
	}

	if (( pt_block = poly_poly2points(pts, count, rule & fmWinding, NULL)) == NULL )
		return NULL;
	region = points2region(pt_block, outline);
	free( pt_block );

	if (outline) {
		PRegionRec new = superimpose_outline(region, pts, count);
		if (new) region = new;
	}
	img_region_compress(region);
	return region;
}

#ifdef __cplusplus
}
#endif

