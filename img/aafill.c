#include <stdlib.h>
#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AAX          8
#define AAY          8
#define AAX_SHIFT    3
#define AAY_SHIFT    3
#define AA_RES_SHIFT 2 /* log2(256/(AAX*AAY)) */

#if 0
#define _DEBUG
#define DEBUG if(1)printf
#else
#define DEBUG if(0)printf
#endif

typedef struct {
	PolyPointBlock *block;
	Point *point[AAY];
} ScanlinePtr;

/* advance AAY pointers until x, subsample AA pixel value if needed */
static Byte
skipto( ScanlinePtr* scan, int x, Bool subsample_last_pixel)
{
	int i, x_from, x_to;
	unsigned int y_collector = 0;
	PolyPointBlock *b = scan->block;

	x *= AAX;
	if ( subsample_last_pixel ) {
		x_from = x - AAX;
		x_to   = x - 1;
	}

	for ( i = 0; i < AAY; i++) {
		int    y;
		Byte   x_collector = 0;
		Point *p = scan->point[i], *last;

		if ( p != NULL ) {
			y    = p->y;
			last = b->pts + b->size;
		} else
			continue;

		while ( p != last ) {
			if ( y != p->y ) {
				scan->point[i] = NULL;
				goto NEXT;
			}

			if ( subsample_last_pixel) {
				register int x1 = p->x;
				register int x2 = p[1].x;
				if ( x1 <= x_to && x2 >= x_from ) {
					if ( x1 < x_from ) x1 = x_from;
					if ( x2 > x_to   ) x2 = x_to;
					x_collector += x2 - x1 + 1;
					DEBUG(":%d[%d]: %d %d\n", y, i, x1, x2);
				}
			}

			if ( p[1].x < x ) {
				DEBUG("%d.%d: %d-%d =\n", x, i, p->x, p[1].x);
				p += 2;
			} else {
				scan->point[i] = p;
				goto NEXT;
			}
		}
		scan->point[i] = NULL;

	NEXT:
		y_collector += x_collector;
	}

	return (y_collector > 0) ? (y_collector << AA_RES_SHIFT) - 1 : 0;
}

/*

Convert scanline where 1s are edge crossings are and 0s are either filled or unfilled pixels,
into a properly subsampled AA scanline

*/
static void
fill( int startx, int y, Byte *map, unsigned int maplen, ScanlinePtr* scan)
{
	Byte *scanned;
	unsigned int advance;

#ifdef _DEBUG
	{
		int z = maplen;
		Byte *p = map;
		while (z--) {
			printf("%02x ", *(p++));
		}
		printf(":%d\n", maplen);
	}
#endif

	/* advance after the first edge or quit if there are none */
	if (( scanned = (Byte*) memchr( map, 1, maplen )) == NULL)
		return;
	advance  = scanned - map;
	map      = scanned;
	maplen  -= advance;
	startx  += advance;

	while ( 1 ) {
		unsigned int advance;
		Byte replicator;
		/* this is a beginning or an end to an edge, we don't care which
		because in the AxA square there can be both types. What's more important
		that this pixel value needs always to be calculated */
		*(map++) = skipto( scan, ++startx, true );
		DEBUG("Y:%d L=%02x (%d)\n", y, map[-1], maplen);
		if ( --maplen <= 0 ) return;
		if ( *map ) continue; /* next pixel is also intersected by an edge, so just do that again */

		/* last edge? quit early */
		if (( scanned = (Byte*) memchr( map, 1, maplen)) == NULL)
			return;
		advance = scanned - map; /* that's how many pixels we can replicate! */
		maplen -= advance;

		/* now, the next pixel is not intersected by any edge, so whatever its value going to
		be, this pixel and its neighbours will share it until the edge end. This is the whole 
		point of no-AA optimization, in order to not subsample identical values. */
		replicator = skipto( scan, startx + 1, true );
		DEBUG("Y:%d R=%02x\n", y, replicator);
		if ( replicator > 0 )
			memset( map, replicator, advance );
		if ( advance > 1 )
			skipto( scan, startx + advance - 1, false);
		startx += advance;
		map    += advance;
	}
}

static Bool
intersect( PRect src, PRect clip)
{
	if (
		src->right  <  clip->left   ||
		src->top    <  clip->bottom ||
		src->left   >  clip->right  ||
		src->bottom >  clip->top
	)
		return false;

	if ( src->left   < clip->left   ) src->left   = clip->left;
	if ( src->bottom < clip->bottom ) src->bottom = clip->bottom;
	if ( src->right  > clip->right  ) src->right  = clip->right;
	if ( src->top    > clip->top    ) src->top    = clip->top;

	return true;
}

static Point*
prepare_points_and_clip( NPoint *pts, unsigned int n_pts, Rect *aa_extents)
{
	Point *pXpts;

	if (( pXpts = malloc(sizeof(Point) * n_pts)) == NULL)
		return NULL;

	{
		register int     n   = n_pts;
		register Point  *dst = pXpts;
		register NPoint *src = pts;
		aa_extents->left = aa_extents->right = src-> x * AAX + .5;
		aa_extents->bottom = aa_extents->top = src-> y * AAX + .5;
		while (n--) {
			register int x;

			x = src-> x * AAX + .5;
			if ( aa_extents->left   > x ) aa_extents->left   = x;
			if ( aa_extents->right  < x ) aa_extents->right  = x;
			dst-> x = x;

			x = src-> y * AAY + .5;
			if ( aa_extents->bottom > x ) aa_extents->bottom = x;
			if ( aa_extents->top    < x ) aa_extents->top    = x;
			dst-> y = x;

			dst++;
			src++;
		}
	}

	return pXpts;
}

Bool
img_aafill( Handle self, NPoint *pts, int n_pts, int rule)
{
	int y_curr, y_lim, y_scan, xmin_px, xmax_px;
	unsigned int maplen;
	Bool ok = false, map_is_dirty;
	Point *pXpts = NULL;
	PolyPointBlock *block = NULL;
	Byte *map = NULL;
	ScanlinePtr scanline_ptr;
	Rect clip, aa_extents;

	if (n_pts < 2)
		return false;
	if (PImage(self)->type != imByte)
		return false;

	clip.left  = clip.bottom = 0;
	clip.right = PImage(self)->w - 1;
	clip.top   = PImage(self)->h - 1;

	if (( pXpts = prepare_points_and_clip( pts, n_pts, &aa_extents)) == NULL)
		return false;
	clip.left   *= AAX;
	clip.right  = (clip.right + 1 ) * AAX - 1;
	clip.bottom *= AAY;
	clip.top    = (clip.top + 1 ) * AAY - 1;
	if ( !intersect(&aa_extents, &clip)) {
		ok = true;
		goto FAIL;
	}

	xmin_px = aa_extents.left   / AAX;
	xmax_px = aa_extents.right  / AAX;
	maplen  = xmax_px - xmin_px + 1;
	DEBUG("EXTENTS %d-%d/%d-%d = %d-%d = %d\n", aa_extents.left, aa_extents.right, aa_extents.bottom, aa_extents.top, xmin_px, xmax_px , maplen);

	if (( block = poly_poly2points(pXpts, n_pts, rule, &clip)) == NULL )
		goto FAIL;
	free( pXpts );
	pXpts = NULL;

	y_curr = aa_extents.bottom;
	map    = PImage(self)->data + PImage(self)->lineSize * ( y_curr >> AAY_SHIFT ) + xmin_px;
	y_lim  = y_curr + AAY - 1;
	y_scan = y_curr;
	bzero( map, maplen );
	map_is_dirty = false;
	bzero( &scanline_ptr, sizeof(scanline_ptr));
	scanline_ptr.block    = block;
	scanline_ptr.point[0] = block->pts;

	/*

	Traverse list of rendered point pairs, do the following:

	 - In the byte map, mark bytes that are crossing an edge. Those
	 will mark where the AA calculation must be done and where it
	 will be safe to reuse a previously calculated value (see more in fill())

	 - For each 0..AAY-1 scanlines, remember where each scanline started.

	*/

	{
		Point *p = block->pts;
		int n = block->size, delta = 0;
		while (n--) {
			register int x = (p->x - delta - aa_extents.left) >> AAX_SHIFT;
			delta = !delta; /* last pixel of a line end, used by fmOutline but not by AA fills */
			if ( p-> y != y_scan ) {
				register int scanline;
				if ( p-> y > y_lim ) {
					fill( xmin_px, y_curr, map, maplen, &scanline_ptr);
					map += PImage(self)->lineSize;
					bzero( map, maplen );
					bzero( scanline_ptr.point, sizeof(scanline_ptr.point));
					map_is_dirty = false;
					while ( p->y > y_lim ) {
						y_lim  += AAY;
						y_curr += AAY;
					}
				}

				y_scan = p-> y;
				scanline = p-> y - y_curr;
				scanline_ptr.point[scanline] = p;
			}
			DEBUG("SET.%d(%d-%d.%d) @ %d\n", (int)(p - block->pts), p->x, delta, p->y, x);
			map[x] = 1;
			map_is_dirty = true;
			p++;
		}
	}
	if ( map_is_dirty )
		fill( xmin_px, y_curr, map, maplen, &scanline_ptr);

	ok = true;
FAIL:
	if ( pXpts )
		free( pXpts );
	if ( block )
		free( block );
	return ok;
}

#ifdef __cplusplus
}
#endif
