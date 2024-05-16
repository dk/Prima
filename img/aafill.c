#include <stdlib.h>
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AAX       8
#define AAY       8
#define AAX_SHIFT 3
#define AAY_SHIFT 3

typedef struct {
	PolyPointBlock *block;
	Point *point;
} ScanlinePtr;

/* advance AAY pointers until x, subsample AA pixel value if needed */
static Byte
moveto( ScanlinePtr* ptrs, int x, Bool subsample_last_pixel)
{
	int i, x_from;
	Byte y_collector = 0;

	x *= AAX;
	x_from = x - AAX;

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
			Point          *last = b->pts + b->size - 2;

			while ( p != last ) {
				if ( y != p->y ) {
					ptrs->point = NULL;
					ptrs->block = NULL;
					goto NEXT;
				}

				if ( p[1].x < x ) {
					p += 2;
					continue;
				}

				if ( subsample_last_pixel) {
					register int x1 = p->x;
					register int x2 = p[1].x;
					if ( x1 <= x && x2 >= x_from ) {
						if ( x1 < x_from ) x1 = x_from;
						if ( x2 > x      ) x2 = x;
						x_collector += x2 - x1 + 1;
						printf(":%d[%d]: %d %d\n", y, i, x1, x2);
					}
				}

				ptrs->point = p;
				goto NEXT;
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

	return y_collector;
}

/*

Convert scanline where 1s are edge crossings are and 0s are either filled or unfilled pixels,
into a properly subsampled AA scanline

*/
static void
fill( int startx, int y, Byte *map, unsigned int maplen, ScanlinePtr* ptrs)
{
	Byte *scanned;
	unsigned int dx;

	/* advance after the first edge or quit if there are none */
	if (( scanned = (Byte*) memchr( map, 1, maplen )) == NULL)
		return;
	dx  = scanned - map;
	map = scanned;
	maplen -= dx;
	startx += dx;

	while ( 1 ) {
		unsigned int advance;
		Byte replicator;
		/* this is a beginning or an end to an edge, we don't care which
		because in the AxA square there can be both types. What's more important
		that this pixel value needs always to be calculated */
		*(map++) = moveto( ptrs, startx++, true );
		printf("L=%02x\n", map[-1]);
		if ( ++dx >= maplen-- ) return;
		if ( *map ) continue; /* next pixel is also intersected by an edge, so just do that again */

		/* last edge? quit early */
		if (( scanned = (Byte*) memchr( map, 1, maplen)) == NULL)
			return;
		advance = scanned - map; /* that's how many pixels we can replicate! */
		dx     += advance;
		maplen -= advance;

		/* now, the next pixel is not intersected by any edge, so whatever its value going to
		be, this pixel and its neighbours will share it until the edge end. This is the whole 
		point of no-AA optimization, in order to not subsample identical values. */
		replicator = moveto( ptrs, startx + 1, true );
		printf("R=%02x\n", replicator);
		if ( replicator > 0 )
			memset( map, replicator, advance );
		if ( advance > 1 )
			moveto( ptrs, startx + advance - 1, false);
		startx += advance;
	}
}

typedef struct {
	PImage i;
	PImgPaintContext ctx;
	BlendFunc *blend1, *blend2;
} FillRec;

typedef void FillProc( Byte *scanline, unsigned int w, int src_x, int y, FillRec *fr);
typedef FillProc *PFillProc;

static void
fill_solid_color_byte( Byte *scanline, unsigned int w, int src_x, int y, FillRec *fr)
{
	Byte *dst = fr->i->data + y * fr->i->lineSize + src_x;
	Byte dummy_alpha = 255;
	fr->blend1(
		scanline, 1,
		&dummy_alpha, 0,
		dst,
		&dummy_alpha, 0,
		w
	);
}

/* call fill procedure over the parts that are to be painted, skip the others */
static void
execute( FillRec *fill_rec, FillProc * fill_proc, Byte *scanline, int w, int src_x, int y)
{
	{
		int z = w;
		Byte *p = scanline;
		while (z--) {
			printf("%02x ", *(p++));
		}
		printf("\n");
	}
	/* XXX check boundaries here or relegate to a common cliprect routine? */
	while ( w > 0 ) {
		if ( *scanline ) {
			Byte *next;
			if (( next = memchr( scanline, 0, w )) == NULL) {
				fill_proc( scanline, w, src_x, y, fill_rec);
				return;
			} else {
				register unsigned int dx = next - scanline;
				fill_proc( scanline, dx, src_x, y, fill_rec);
				scanline += dx;
				src_x    += dx;
				w        -= dx;
			}
		} else {
			scanline++;
			src_x++;
			w--;
		}
	}
}

Bool
img_aafill( Handle self, NPoint *pts, int n_pts, int rule, PImgPaintContext ctx)
{
	int y_curr, y_lim, y_scan;
	int xmin_aa, xmax_aa, ymin_aa, ymax_aa, xmin_px, xmax_px, ymin_px, ymax_px;
	unsigned int maplen;
	Bool ok = false;
	Point *pXpts = NULL;
	PolyPointBlock *first, *curr;
	Byte *map = NULL;
	ScanlinePtr scanline_ptr[AAY];
	FillRec fill_rec;
	FillProc *fill_proc;

	(void) ymin_px;
	(void) ymax_px;

	if (n_pts < 2)
		return false;

	fill_rec.i   = (PImage) self;
	fill_rec.ctx = ctx;
	if ( !img_find_blend_proc(ctx->rop, &fill_rec.blend1, &fill_rec.blend2))
		img_find_blend_proc(ropSrcOver, &fill_rec.blend1, &fill_rec.blend2);
	fill_proc = fill_solid_color_byte;

	if (( pXpts = malloc(sizeof(Point) * n_pts)) == NULL)
		return false;
	{
		register int     n   = n_pts;
		register Point  *dst = pXpts;
		register NPoint *src = pts;
		xmin_aa = xmax_aa = src-> x * AAX + .5;
		ymin_aa = ymax_aa = src-> y * AAX + .5;
		while (n--) {
			register int x;
			x = src-> x * AAX + .5;
			if ( xmin_aa > x ) xmin_aa = x;
			if ( xmax_aa < x ) xmax_aa = x;
			dst-> x = x;
			x = src-> y * AAY + .5;
			if ( ymin_aa > x ) ymin_aa = x;
			if ( ymax_aa < x ) ymax_aa = x;
			dst-> y = x;
			dst++;
			src++;
		}
		xmin_px = xmin_aa / AAX;
		xmax_px = xmax_aa / AAX;
		ymin_px = ymin_aa / AAY;
		ymax_px = ymax_aa / AAY;
		maplen = xmax_px - xmin_px + 1;
	}

	if (( first = curr = poly_poly2points(pXpts, n_pts, rule)) == NULL )
		goto FAIL;
	free( pXpts );
	pXpts = NULL;

	if (( map = malloc( maplen )) == NULL)
		goto FAIL;

	y_curr = ymin_aa;
	y_lim  = y_curr + AAY - 1;
	y_scan = y_curr;
	bzero( map, maplen );
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
					execute( &fill_rec, fill_proc, map, maplen, xmin_px, y_curr >> AAY_SHIFT);
					bzero( map, maplen );
					bzero( scanline_ptr, sizeof(scanline_ptr));
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
			map[ ( p-> x - xmin_aa ) >> AAX_SHIFT ] = 1;
			p++;
		}
		curr = curr->next;
	}
	if ( (( ymax_aa - ymin_aa + 1 ) % AAY) != 0 ) {
		fill( xmin_px, y_curr, map, maplen, scanline_ptr);
		execute( &fill_rec, fill_proc, map, maplen, xmin_px, y_curr >> AAY_SHIFT);
	}

	ok = true;
FAIL:
	if ( map )
		free( map );
	if ( pXpts ) 
		free( pXpts );
	if ( first )
		poly_free_blocks( first );
	return ok;
}

#ifdef __cplusplus
}
#endif
