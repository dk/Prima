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
	Point *point;
} ScanlinePtr;

/* advance AAY pointers until x, subsample AA pixel value if needed */
static Byte
skipto( ScanlinePtr* ptrs, int x, Bool subsample_last_pixel)
{
	int i, x_from, x_to;
	unsigned int y_collector = 0;

	x *= AAX;
	if ( subsample_last_pixel ) {
		x_from = x - AAX;
		x_to   = x - 1;
	}

	for ( i = 0; i < AAY; i++, ptrs++) {
		int y;
		Byte x_collector = 0;

		if ( ptrs-> point )
			y = ptrs->point->y;
		else
			continue;

		while (1) {
			Point          *p    = ptrs->point;
			PolyPointBlock *b    = ptrs->block;
			Point          *last = b->pts + b->size - 0;

			while ( p != last ) {
				if ( y != p->y ) {
					ptrs->point = NULL;
					ptrs->block = NULL;
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
					ptrs->point = p;
					goto NEXT;
				}
			}

			if (( ptrs->block = b = b->next) == NULL) {
				ptrs->point = NULL;
				break;
			}

			ptrs->point = b->pts;
		}

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
fill( int startx, int y, Byte *map, unsigned int maplen, ScanlinePtr* ptrs)
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
		*(map++) = skipto( ptrs, ++startx, true );
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
		replicator = skipto( ptrs, startx + 1, true );
		DEBUG("Y:%d R=%02x\n", y, replicator);
		if ( replicator > 0 )
			memset( map, replicator, advance );
		if ( advance > 1 )
			skipto( ptrs, startx + advance - 1, false);
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
	PolyPointBlock *first = NULL, *curr = NULL;
	Byte *map = NULL;
	ScanlinePtr scanline_ptr[AAY];
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

	if (( first = curr = poly_poly2points(pXpts, n_pts, rule, &clip)) == NULL )
		goto FAIL;
	free( pXpts );
	pXpts = NULL;

	y_curr = aa_extents.bottom;
	map    = PImage(self)->data + PImage(self)->lineSize * ( y_curr >> AAY_SHIFT ) + xmin_px;
	y_lim  = y_curr + AAY - 1;
	y_scan = y_curr;
	bzero( map, maplen );
	map_is_dirty = false;
	bzero( scanline_ptr, sizeof(scanline_ptr));
	scanline_ptr[0].block = curr;
	scanline_ptr[0].point = curr->pts;

	/*

	Traverse list of rendered point pairs, do the following:

	 - In the byte map, mark bytes that are crossing an edge. Those
	 will mark where the AA calculation must be done and where it
	 will be safe to reuse a previously calculated value (see more in fill())

	 - For each 0..AAY-1 scanlines, remember where each scanline started.

	*/

	while ( curr != NULL ) {
		Point *p = curr->pts;
		int n = curr->size;
		while (n--) {
			if ( p-> y != y_scan ) {
				register int scanline;
				if ( p-> y > y_lim ) {
					fill( xmin_px, y_curr, map, maplen, scanline_ptr);
					map += PImage(self)->lineSize;
					bzero( map, maplen );
					bzero( scanline_ptr, sizeof(scanline_ptr));
					map_is_dirty = false;
					while ( p->y > y_lim ) {
						y_lim  += AAY;
						y_curr += AAY;
					}
				}

				y_scan = p-> y;
				scanline = p-> y - y_curr;
				scanline_ptr[scanline].block = curr;
				scanline_ptr[scanline].point = p;
			}
			DEBUG("SET.%d(%d.%d) @ %d\n", (int)(p - curr->pts), p->x, p->y, (p->x - aa_extents.left) >> AAX_SHIFT);
			map[ ( p-> x - aa_extents.left ) >> AAX_SHIFT ] = 1;
			map_is_dirty = true;
			p++;
		}
		curr = curr->next;
	}
	if ( map_is_dirty )
		fill( xmin_px, y_curr, map, maplen, scanline_ptr);

	ok = true;
FAIL:
	if ( pXpts ) 
		free( pXpts );
	if ( first )
		poly_free_blocks( first );
	return ok;
}

#ifdef __cplusplus
}
#endif
