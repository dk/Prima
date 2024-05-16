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

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

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

/* compress region vertically */
static PRegionRec
compress_region( PRegionRec region)
{
	int i, n;
	Box *prev, *curr;
	for (
		i = 1, n = region-> n_boxes, prev = region->boxes, curr = prev + 1;
		i < n;
		i++, curr++
	) {
		if (
			curr->x == prev->x &&
			curr->y == prev->y + prev->height &&
			curr->width == prev->width
		) {
			prev->height += curr->height;
			region-> n_boxes--;
		} else {
			if ( curr - prev > 1 ) {
				memmove( prev + 1, curr, sizeof(Box) * (n - i));
				curr = prev + 1;
			}
			prev = curr;
		}
	}
	return region;
}

static PRegionRec
points2region( PolyPointBlock *first, int outline)
{
	register Box  *rects;
	register Point *pts;
	register int i;
	PolyPointBlock *curr;
	int numRects;
	PRegionRec reg;

	numRects = 0;
	curr = first;
	while (curr != NULL) {
		numRects += curr->size;
		curr = curr->next;
	}
	if ( !( reg = img_region_new(numRects * 2 + outline)))
		return NULL;

	rects = reg->boxes - 1;
	numRects = 0;
	curr = first;
	while ( curr != NULL ) {
		/* the loop uses 2 points per iteration */
		i = curr->size >> 1;
		for (pts = curr->pts; i--; pts += 2) {
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
			if ( rects-> width < 0 ) {
					rects->x += rects->width;
				rects->width = -rects->width;
			}
			if ( rects-> height < 0 ) {
				rects->y += rects->height;
				rects->height = -rects->height;
			}
			DEBUG("insert x=%d y=%d w=%d h=%d\n", rects->x, rects->y, rects->width, rects->height);
		}
		curr = curr->next;
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

	if (( pt_block = poly_poly2points(pts, count, rule)) == NULL )
		return NULL;
	region = points2region(pt_block, outline);
	poly_free_blocks( pt_block );

	if (outline) {
		PRegionRec new = superimpose_outline(region, pts, count);
		if (new) region = new;
	}
	return compress_region(region);
}

#ifdef __cplusplus
}
#endif

