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
is_rect( Point *pts, int Count, int outline, Box *single)
{
    if (((Count == 4) ||
	 ((Count == 5) && (pts[4].x == pts[0].x) && (pts[4].y == pts[0].y))) &&
	(((pts[0].y == pts[1].y) &&
	  (pts[1].x == pts[2].x) &&
	  (pts[2].y == pts[3].y) &&
	  (pts[3].x == pts[0].x)) ||
	 ((pts[0].x == pts[1].x) &&
	  (pts[1].y == pts[2].y) &&
	  (pts[2].x == pts[3].x) &&
	  (pts[3].y == pts[0].y)))
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

	/* superimpose polyline points using Bresenham
	because regions are as broken as filled shapes */
	for ( i = 0; i < count - 1; i++) {
		int curr_maj, curr_min, to_maj, delta_maj, delta_min;
		int delta_y, delta_x;
		int dir = 0, d, d_inc1, d_inc2;
		int inc_maj, inc_min;
		int x, y, acc_x = 0, acc_y = INT_MIN, ox;
		Point a = pts[i], b = pts[i+1];
		delta_y = b.y - a.y;
		delta_x = b.x - a.x;
		if (abs(delta_y) > abs(delta_x)) dir = 1;
		DEBUG("edge %d.%d-%d.%d\n", a.x,a.y,b.x,b.y);

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
				if ( acc_y > INT_MIN) {
					int xx, width;
					if (ox < acc_x) {
						xx = ox;
						width = acc_x - ox + 1;
					} else {
						xx = acc_x;
						width = ox - acc_x + 1;
					}
					if (!( region = union_hline( region, scanline2box, xx, acc_y, width)))
						goto EXIT;
				}
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
		if ( acc_y > INT_MIN) {
			int xx, width;
			if (x < acc_x) {
				xx = x;
				width = acc_x - x + 1;
			} else {
				xx = acc_x;
				width = x - acc_x + 1;
			}
			if (!( region = union_hline( region, scanline2box, xx, acc_y, width)))
				goto EXIT;
		}
	}

EXIT:
	free(scanline2box);
	return region;
}

/*

The code below is based on the libX11 region implementation

Differences:
  - extents are not calculated
  - recognizes fmOverlay
  - 4-point rectangular polyline is recognized as a RECT but contrary to the
    X11 implementation is NOT a special case, and returns inclusive-exclusive
    coordinates when fmOverlay is unset.

*/

/************************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

#define LARGE_COORDINATE 1000000
#define SMALL_COORDINATE -LARGE_COORDINATE

#undef MAXSHORT
#undef MINSHORT
#define MAXSHORT 32767
#define MINSHORT -MAXSHORT

/*
 * number of points to buffer before sending them off
 * to scanlines() :  Must be an even number
 */
#define NUMPTSTOBUFFER 200

/*
 * used to allocate buffers for points and link
 * the buffers together
 */
typedef struct _POINTBLOCK {
    Point pts[NUMPTSTOBUFFER];
    struct _POINTBLOCK *next;
} POINTBLOCK;

/*
 *     This file contains a few macros to help track
 *     the edge of a filled object.  The object is assumed
 *     to be filled in scanline order, and thus the
 *     algorithm used is an extension of Bresenham's line
 *     drawing algorithm which assumes that y is always the
 *     major axis.
 *     Since these pieces of code are the same for any filled shape,
 *     it is more convenient to gather the library in one
 *     place, but since these pieces of code are also in
 *     the inner loops of output primitives, procedure call
 *     overhead is out of the question.
 *     See the author for a derivation if needed.
 */


/*
 *  In scan converting polygons, we want to choose those pixels
 *  which are inside the polygon.  Thus, we add .5 to the starting
 *  x coordinate for both left and right edges.  Now we choose the
 *  first pixel which is inside the pgon for the left edge and the
 *  first pixel which is outside the pgon for the right edge.
 *  Draw the left pixel, but not the right.
 *
 *  How to add .5 to the starting x coordinate:
 *      If the edge is moving to the right, then subtract dy from the
 *  error term from the general form of the algorithm.
 *      If the edge is moving to the left, then add dy to the error term.
 *
 *  The reason for the difference between edges moving to the left
 *  and edges moving to the right is simple:  If an edge is moving
 *  to the right, then we want the algorithm to flip immediately.
 *  If it is moving to the left, then we don't want it to flip until
 *  we traverse an entire pixel.
 */
#define BRESINITPGON(dy, x1, x2, xStart, d, m, m1, incr1, incr2) { \
    int dx;      /* local storage */ \
\
    /* \
     *  if the edge is horizontal, then it is ignored \
     *  and assumed not to be processed.  Otherwise, do this stuff. \
     */ \
    if ((dy) != 0) { \
        xStart = (x1); \
        dx = (x2) - xStart; \
        if (dx < 0) { \
            m = dx / (dy); \
            m1 = m - 1; \
            incr1 = -2 * dx + 2 * (dy) * m1; \
            incr2 = -2 * dx + 2 * (dy) * m; \
            d = 2 * m * (dy) - 2 * dx - 2 * (dy); \
        } else { \
            m = dx / (dy); \
            m1 = m + 1; \
            incr1 = 2 * dx - 2 * (dy) * m1; \
            incr2 = 2 * dx - 2 * (dy) * m; \
            d = -2 * m * (dy) + 2 * dx; \
        } \
	DEBUG("x start = %d incr1=%d incr2=%d m=%d m1=%d\n", x1, incr1, incr2, m, m1);\
    } \
}

#define BRESINCRPGON(d, minval, m, m1, incr1, incr2) { \
    if (m1 > 0) { \
        if (d > 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } else {\
        if (d >= 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } \
}


/*
 *     This structure contains all of the information needed
 *     to run the bresenham algorithm.
 *     The variables may be hardcoded into the declarations
 *     instead of using this structure to make use of
 *     register declarations.
 */
typedef struct {
    int minor_axis;	/* minor axis        */
    int d;		/* decision variable */
    int m, m1;		/* slope and slope+1 */
    int incr1, incr2;	/* error increments */
} BRESINFO;


#define BRESINITPGONSTRUCT(dmaj, min1, min2, bres) \
	BRESINITPGON(dmaj, min1, min2, bres.minor_axis, bres.d, \
                     bres.m, bres.m1, bres.incr1, bres.incr2)

#define BRESINCRPGONSTRUCT(bres) \
        BRESINCRPGON(bres.d, bres.minor_axis, bres.m, bres.m1, bres.incr1, bres.incr2)



/*
 *     These are the data structures needed to scan
 *     convert regions.  Two different scan conversion
 *     methods are available -- the even-odd method, and
 *     the winding number method.
 *     The even-odd rule states that a point is inside
 *     the polygon if a ray drawn from that point in any
 *     direction will pass through an odd number of
 *     path segments.
 *     By the winding number rule, a point is decided
 *     to be inside the polygon if a ray drawn from that
 *     point in any direction passes through a different
 *     number of clockwise and counter-clockwise path
 *     segments.
 *
 *     These data structures are adapted somewhat from
 *     the algorithm in (Foley/Van Dam) for scan converting
 *     polygons.
 *     The basic algorithm is to start at the top (smallest y)
 *     of the polygon, stepping down to the bottom of
 *     the polygon by incrementing the y coordinate.  We
 *     keep a list of edges which the current scanline crosses,
 *     sorted by x.  This list is called the Active Edge Table (AET)
 *     As we change the y-coordinate, we update each entry in
 *     in the active edge table to reflect the edges new xcoord.
 *     This list must be sorted at each scanline in case
 *     two edges intersect.
 *     We also keep a data structure known as the Edge Table (ET),
 *     which keeps track of all the edges which the current
 *     scanline has not yet reached.  The ET is basically a
 *     list of ScanLineList structures containing a list of
 *     edges which are entered at a given scanline.  There is one
 *     ScanLineList per scanline at which an edge is entered.
 *     When we enter a new edge, we move it from the ET to the AET.
 *
 *     From the AET, we can implement the even-odd rule as in
 *     (Foley/Van Dam).
 *     The winding number rule is a little trickier.  We also
 *     keep the EdgeTableEntries in the AET linked by the
 *     nextWETE (winding EdgeTableEntry) link.  This allows
 *     the edges to be linked just as before for updating
 *     purposes, but only uses the edges linked by the nextWETE
 *     link as edges representing spans of the polygon to
 *     drawn (as with the even-odd rule).
 */

/*
 * for the winding number rule
 */
#define CLOCKWISE          1
#define COUNTERCLOCKWISE  -1

typedef struct _EdgeTableEntry {
     int ymax;             /* ycoord at which we exit this edge. */
     BRESINFO bres;        /* Bresenham info to run the edge     */
     struct _EdgeTableEntry *next;       /* next in the list     */
     struct _EdgeTableEntry *back;       /* for insertion sort   */
     struct _EdgeTableEntry *nextWETE;   /* for winding num rule */
     int ClockWise;        /* flag for winding number rule       */
#ifdef _DEBUG
     Point p1, p2;
#endif
} EdgeTableEntry;


typedef struct _ScanLineList{
     int scanline;              /* the scanline represented */
     EdgeTableEntry *edgelist;  /* header node              */
     struct _ScanLineList *next;  /* next in the list       */
} ScanLineList;


typedef struct {
     int ymax;                 /* ymax for the polygon     */
     int ymin;                 /* ymin for the polygon     */
     ScanLineList scanlines;   /* header node              */
} EdgeTable;


/*
 * Here is a struct to help with storage allocation
 * so we can allocate a big chunk at a time, and then take
 * pieces from this heap when we need to.
 */
#define SLLSPERBLOCK 25

typedef struct _ScanLineListBlock {
     ScanLineList SLLs[SLLSPERBLOCK];
     struct _ScanLineListBlock *next;
} ScanLineListBlock;

#ifdef _DEBUG
#define EDGE_DEBUG(p) p->p1.x,p->p1.y,p->p2.x,p->p2.y
#else
#define EDGE_DEBUG(p) 0,0,0,0
#endif



/*
 *
 *     a few macros for the inner loops of the fill code where
 *     performance considerations don't allow a procedure call.
 *
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The winding number rule is in effect, so we must notify
 *     the caller when the edge has been removed so he
 *     can reorder the Winding Active Edge Table.
 */
#define EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET) { \
   if (pAET->ymax == y) {          /* leaving this edge */ \
      DEBUG("v edge exit\n");\
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      fixWAET = 1; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres); \
      DEBUG("| edge update x=%d\n", pAET->bres.minor_axis);\
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}


/*
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The even-odd rule is in effect.
 */
#define EVALUATEEDGEEVENODD(pAET, pPrevAET, y) { \
   if (pAET->ymax == y) {          /* leaving this edge */ \
      DEBUG("v edge exit\n");\
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres); \
      DEBUG("| edge update x=%d\n", pAET->bres.minor_axis);\
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}

/*
 *     InsertEdgeInET
 *
 *     Insert the given edge into the edge table.
 *     First we must find the correct bucket in the
 *     Edge table, then find the right slot in the
 *     bucket.  Finally, we can insert it.
 *
 */
static void
InsertEdgeInET(
    EdgeTable *ET,
    EdgeTableEntry *ETE,
    int scanline,
    ScanLineListBlock **SLLBlock,
    int *iSLLBlock)
{
    register EdgeTableEntry *start, *prev;
    register ScanLineList *pSLL, *pPrevSLL;
    ScanLineListBlock *tmpSLLBlock;

    /*
     * find the right bucket to put the edge into
     */
    pPrevSLL = &ET->scanlines;
    pSLL = pPrevSLL->next;
    while (pSLL && (pSLL->scanline < scanline))
    {
        pPrevSLL = pSLL;
        pSLL = pSLL->next;
    }

    /*
     * reassign pSLL (pointer to ScanLineList) if necessary
     */
    if ((!pSLL) || (pSLL->scanline > scanline))
    {
        if (*iSLLBlock > SLLSPERBLOCK-1)
        {
            tmpSLLBlock = malloc(sizeof(ScanLineListBlock));
            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = (ScanLineListBlock *)NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }
        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        pSLL->edgelist = (EdgeTableEntry *)NULL;
        pPrevSLL->next = pSLL;
    }
    pSLL->scanline = scanline;

    /*
     * now insert the edge in the right bucket
     */
    prev = (EdgeTableEntry *)NULL;
    start = pSLL->edgelist;
    while (start && (start->bres.minor_axis < ETE->bres.minor_axis))
    {
        prev = start;
        start = start->next;
    }
    ETE->next = start;

    if (prev) {
        prev->next = ETE;
    }
    else {
        pSLL->edgelist = ETE;
    }
}

/*
 *     CreateEdgeTable
 *
 *     This routine creates the edge table for
 *     scan converting polygons.
 *     The Edge Table (ET) looks like:
 *
 *    EdgeTable
 *     --------
 *    |  ymax  |        ScanLineLists
 *    |scanline|-->------------>-------------->...
 *     --------   |scanline|   |scanline|
 *                |edgelist|   |edgelist|
 *                ---------    ---------
 *                    |             |
 *                    |             |
 *                    V             V
 *              list of ETEs   list of ETEs
 *
 *     where ETE is an EdgeTableEntry data structure,
 *     and there is one ScanLineList per scanline at
 *     which an edge is initially entered.
 *
 */

static void
CreateETandAET(
    register int count,
    register Point *pts,
    EdgeTable *ET,
    EdgeTableEntry *AET,
    register EdgeTableEntry *pETEs,
    ScanLineListBlock   *pSLLBlock)
{
    register Point *top, *bottom;
    register Point *PrevPt, *CurrPt;
    int iSLLBlock = 0;
    int dy;

    if (count < 2)  return;

    /*
     *  initialize the Active Edge Table
     */
    AET->next = (EdgeTableEntry *)NULL;
    AET->back = (EdgeTableEntry *)NULL;
    AET->nextWETE = (EdgeTableEntry *)NULL;
    AET->bres.minor_axis = SMALL_COORDINATE;

    /*
     *  initialize the Edge Table.
     */
    ET->scanlines.next = (ScanLineList *)NULL;
    ET->ymax = SMALL_COORDINATE;
    ET->ymin = LARGE_COORDINATE;
    pSLLBlock->next = (ScanLineListBlock *)NULL;

    PrevPt = &pts[count-1];

    /*
     *  for each vertex in the array of points.
     *  In this loop we are dealing with two vertices at
     *  a time -- these make up one edge of the polygon.
     */
    while (count--)
    {
        CurrPt = pts++;

        /*
         *  find out which point is above and which is below.
         */
	DEBUG("new edge %d.%d-%d.%d\n", PrevPt->x, PrevPt->y, CurrPt->x, CurrPt->y);
#ifdef _DEBUG
	pETEs->p1 = *PrevPt;
	pETEs->p2 = *CurrPt;
#endif
        if (PrevPt->y > CurrPt->y)
        {
            bottom = PrevPt, top = CurrPt;
            pETEs->ClockWise = 0;
        }
        else
        {
            bottom = CurrPt, top = PrevPt;
            pETEs->ClockWise = 1;
        }

        /*
         * don't add horizontal edges to the Edge table.
         */
        if (bottom->y != top->y)
        {
            pETEs->ymax = bottom->y - 1;  /* -1 so we don't get last scanline */

            /*
             *  initialize integer edge algorithm
             */
            dy = bottom->y - top->y;
            BRESINITPGONSTRUCT(dy, top->x, bottom->x, pETEs->bres);

            InsertEdgeInET(ET, pETEs, top->y, &pSLLBlock, &iSLLBlock);

	    if (PrevPt->y > ET->ymax)
		ET->ymax = PrevPt->y;
	    if (PrevPt->y < ET->ymin)
		ET->ymin = PrevPt->y;
            pETEs++;
        }

        PrevPt = CurrPt;
    }
}

/*
 *     loadAET
 *
 *     This routine moves EdgeTableEntries from the
 *     EdgeTable into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */

static void
loadAET(
    register EdgeTableEntry *AET,
    register EdgeTableEntry *ETEs)
{
    register EdgeTableEntry *pPrevAET;
    register EdgeTableEntry *tmp;
#ifdef _DEBUG
    EdgeTableEntry *saveAET = AET;
#endif

    pPrevAET = AET;
    AET = AET->next;
    while (ETEs)
    {
        while (AET && (AET->bres.minor_axis < ETEs->bres.minor_axis))
        {
            pPrevAET = AET;
            AET = AET->next;
        }
        tmp = ETEs->next;
        ETEs->next = AET;
        if (AET)
            AET->back = ETEs;
        ETEs->back = pPrevAET;
        pPrevAET->next = ETEs;
        pPrevAET = ETEs;

	DEBUG("^ edge %d.%d-%d.%d enter\n", EDGE_DEBUG(ETEs));
        ETEs = tmp;
    }
#ifdef _DEBUG
    DEBUG("active edges: ");
    AET = saveAET->next;
    while ( AET ) {
       DEBUG("x=%d(%d.%d-%d.%d) ", AET->bres.minor_axis, EDGE_DEBUG(AET));
       AET = AET->next;
    }
    DEBUG("\n");
#endif
}

/*
 *     computeWAET
 *
 *     This routine links the AET by the
 *     nextWETE (winding EdgeTableEntry) link for
 *     use by the winding number rule.  The final
 *     Active Edge Table (AET) might look something
 *     like:
 *
 *     AET
 *     ----------  ---------   ---------
 *     |ymax    |  |ymax    |  |ymax    |
 *     | ...    |  |...     |  |...     |
 *     |next    |->|next    |->|next    |->...
 *     |nextWETE|  |nextWETE|  |nextWETE|
 *     ---------   ---------   ^--------
 *         |                   |       |
 *         V------------------->       V---> ...
 *
 */
static void
computeWAET(
    register EdgeTableEntry *AET)
{
    register EdgeTableEntry *pWETE;
    register int inside = 1;
    register int isInside = 0;

    AET->nextWETE = (EdgeTableEntry *)NULL;
    pWETE = AET;
    AET = AET->next;
    while (AET)
    {
        if (AET->ClockWise)
            isInside++;
        else
            isInside--;

        if ((!inside && !isInside) ||
            ( inside &&  isInside))
        {
            pWETE->nextWETE = AET;
            pWETE = AET;
            inside = !inside;
        }
        AET = AET->next;
    }
    pWETE->nextWETE = (EdgeTableEntry *)NULL;
}

/*
 *     InsertionSort
 *
 *     Just a simple insertion sort using
 *     pointers and back pointers to sort the Active
 *     Edge Table.
 *
 */

static int
InsertionSort(
    register EdgeTableEntry *AET)
{
    register EdgeTableEntry *pETEchase;
    register EdgeTableEntry *pETEinsert;
    register EdgeTableEntry *pETEchaseBackTMP;
    register int changed = 0;

    AET = AET->next;
    while (AET)
    {
        pETEinsert = AET;
        pETEchase = AET;
        while (pETEchase->back->bres.minor_axis > AET->bres.minor_axis)
            pETEchase = pETEchase->back;

        AET = AET->next;
        if (pETEchase != pETEinsert)
        {
            pETEchaseBackTMP = pETEchase->back;
            pETEinsert->back->next = AET;
            if (AET)
                AET->back = pETEinsert->back;
            pETEinsert->next = pETEchase;
            pETEchase->back->next = pETEinsert;
            pETEchase->back = pETEinsert;
            pETEinsert->back = pETEchaseBackTMP;
            changed = 1;
        }
    }
    return(changed);
}

/*
 *     Clean up our act.
 */
static void
FreeStorage(
    register ScanLineListBlock   *pSLLBlock)
{
    register ScanLineListBlock   *tmpSLLBlock;

    while (pSLLBlock)
    {
        tmpSLLBlock = pSLLBlock->next;
        free(pSLLBlock);
        pSLLBlock = tmpSLLBlock;
    }
}

/*
 *     Create an array of rectangles from a list of points.
 *     If indeed these things (POINTS, RECTS) are the same,
 *     then this proc is still needed, because it allocates
 *     storage for the array, which was allocated on the
 *     stack by the calling procedure.
 *
 */
static PRegionRec
PtsToRegion(
    register int numFullPtBlocks,
    register int iCurPtBlock,
    POINTBLOCK *FirstPtBlock,
    int outline)
{
    register Box  *rects;
    register Point *pts;
    register POINTBLOCK *CurPtBlock;
    register int i;
    register int numRects;
    PRegionRec reg;

    numRects = ((numFullPtBlocks * NUMPTSTOBUFFER) + iCurPtBlock) >> 1;

    if ( !( reg = img_region_new(numRects + outline)))
       return NULL;

    CurPtBlock = FirstPtBlock;
    rects = reg->boxes - 1;
    numRects = 0;

    for ( ; numFullPtBlocks >= 0; numFullPtBlocks--) {
	/* the loop uses 2 points per iteration */
	i = NUMPTSTOBUFFER >> 1;
	if (!numFullPtBlocks)
	    i = iCurPtBlock >> 1;
	DEBUG("numFullPtBlocks=%d i=%d\n", numFullPtBlocks, i);
	for (pts = CurPtBlock->pts; i--; pts += 2) {
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
	CurPtBlock = CurPtBlock->next;
    }

    reg->n_boxes = numRects;

    return reg;
}

/*

 1. Add the intersection point
 3. Send out the buffer if filled

 */

#define ADD_POINT                                     \
   pts->x = pAET->bres.minor_axis,  pts->y = y;       \
   DEBUG("+p %d %d\n", pts->x, pts->y);               \
   pts++, iPts++;                                     \
   if (iPts == NUMPTSTOBUFFER) {                      \
       tmpPtBlock = malloc(sizeof(POINTBLOCK));       \
       curPtBlock->next = tmpPtBlock;                 \
       curPtBlock = tmpPtBlock;                       \
       pts = curPtBlock->pts;                         \
       numFullPtBlocks++;    iPts = 0;                \
   }

/*
 *     polytoregion
 *
 *     Scan converts a polygon by returning a run-length
 *     encoding of the resultant bitmap -- the run-length
 *     encoding is in the form of an array of rectangles.
 */
PRegionRec
img_region_polygon(
    Point     *Pts,		     /* the pts                 */
    int       Count,                 /* number of pts           */
    int	rule)			     /* winding rule */
{
    PRegionRec region;
    register EdgeTableEntry *pAET;   /* Active Edge Table       */
    register int y;                  /* current scanline        */
    register int iPts = 0;           /* number of pts in buffer */
    register EdgeTableEntry *pWETE;  /* Winding Edge Table Entry*/
    register ScanLineList *pSLL;     /* current scanLineList    */
    register Point *pts;             /* output buffer           */
    EdgeTableEntry *pPrevAET;        /* ptr to previous AET     */
    EdgeTable ET;                    /* header node for ET      */
    EdgeTableEntry AET;              /* header node for AET     */
    EdgeTableEntry *pETEs;           /* EdgeTableEntries pool   */
    ScanLineListBlock SLLBlock;      /* header for scanlinelist */
    int fixWAET = 0;
    POINTBLOCK FirstPtBlock, *curPtBlock; /* PtBlock buffers    */
    POINTBLOCK *tmpPtBlock;
    int numFullPtBlocks = 0;
    int outline;
    Box single;

    outline = (rule & fmOverlay) ? 1 : 0;
    rule    = (rule & fmWinding) ? 0 : 1;
    DEBUG("init: %s %s\n", rule ? "alt" : "wind", outline ? "outl" : "raw");

    pts = Pts;

    if (Count < 2) {
        DEBUG("not enought points");
    	return img_region_new(0);
    }

    if (is_hline( Pts, Count, &single)) {
	if ( !outline ) {
           DEBUG("got invisible hline");
    	   return img_region_new(0);
	}
        DEBUG("got hline %d %d %d %d\n", single.x, single.y, single.width, single.height);
    	return rect_region(&single);
    }

    if (is_rect( Pts, Count, outline, &single)) {
        DEBUG("got rect %d %d %d %d\n", single.x, single.y, single.width, single.height);
    	return rect_region(&single);
    }

    if (! (pETEs = malloc(Count * sizeof(EdgeTableEntry))))
	return NULL;

    pts = FirstPtBlock.pts;
    CreateETandAET(Count, Pts, &ET, &AET, pETEs, &SLLBlock);
    pSLL = ET.scanlines.next;
    curPtBlock = &FirstPtBlock;

    if (rule == 1) {
        /*
         *  for each scanline
         */
	DEBUG("for %d <=> %d\n", ET.ymin, ET.ymax);
        for (y = ET.ymin; y < ET.ymax; y++) {
	    DEBUG("== %d ==\n", y);
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL != NULL && y == pSLL->scanline) {
                loadAET(&AET, pSLL->edgelist);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;

            /*
             *  for each active edge
             */
            while (pAET) {
		ADD_POINT;
                EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
            }
            (void) InsertionSort(&AET);
        }
    }
    else {
        /*
         *  for each scanline
         */
	DEBUG("for %d <=> %d\n", ET.ymin, ET.ymax);
        for (y = ET.ymin; y < ET.ymax; y++) {
	    DEBUG("== %d ==\n", y);
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL != NULL && y == pSLL->scanline) {
                loadAET(&AET, pSLL->edgelist);
                computeWAET(&AET);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;
            pWETE = pAET;

            /*
             *  for each active edge
             */
            while (pAET) {
                /*
                 *  add to the buffer only those edges that
                 *  are in the Winding active edge table.
                 */
                if (pWETE == pAET) {
		    ADD_POINT;
                    pWETE = pWETE->nextWETE;
                }
                EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
            }

            /*
             *  recompute the winding active edge table if
             *  we just resorted or have exited an edge.
             */
            if (InsertionSort(&AET) || fixWAET) {
                computeWAET(&AET);
                fixWAET = 0;
            }
        }
    }
    FreeStorage(SLLBlock.next);
    region = PtsToRegion(numFullPtBlocks, iPts, &FirstPtBlock, outline);
    for (curPtBlock = FirstPtBlock.next; --numFullPtBlocks >= 0;) {
	tmpPtBlock = curPtBlock->next;
	free(curPtBlock);
	curPtBlock = tmpPtBlock;
    }
    free(pETEs);
    if (outline) {
    	PRegionRec new = superimpose_outline(region, Pts, Count);
	if (new) region = new;
    }
    return(region);
}

#ifdef __cplusplus
}
#endif

