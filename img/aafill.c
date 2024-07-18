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
	DEBUG("SKIPTO %d\n", x);
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
				register int x2 = p[1].x - 1; /* see comment below about fmOutline, the same reason for -1 */
				if ( x1 <= x_to && x2 >= x_from ) {
					if ( x1 < x_from ) x1 = x_from;
					if ( x2 > x_to   ) x2 = x_to;
					x_collector += x2 - x1 + 1;
					DEBUG(":%d[%d]: %d %d = %d\n", y, i, x1, x2, x_collector);
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
	DEBUG("<< %d > %02x\n", y_collector, (y_collector > 0) ? (y_collector << AA_RES_SHIFT) - 1 : 0);

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
	Byte *oldmap = map;
	unsigned int oldmaplen = maplen;
	{
		int z = maplen;
		Byte *p = map;
		printf("< ");
		while (z--) {
			printf("%02x ", *(p++));
		}
		printf(":%d\n", maplen);
	}
#endif

#ifdef _DEBUG
#define BAILOUT goto BAIL
#else
#define BAILOUT return
#endif

	/* advance after the first edge or quit if there are none */
	if (( scanned = (Byte*) memchr( map, 1, maplen )) == NULL)
		BAILOUT;
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
		DEBUG("Y:%d.%d L=%02x (%d)\n", startx - 1, y, map[-1], maplen);
		if ( --maplen <= 0 ) BAILOUT;
		if ( *map ) continue; /* next pixel is also intersected by an edge, so just do that again */

		/* last edge? quit early */
		if (( scanned = (Byte*) memchr( map, 1, maplen)) == NULL)
			BAILOUT;
		advance = scanned - map; /* that's how many pixels we can replicate! */
		maplen -= advance;

		/* now, the next pixel is not intersected by any edge, so whatever its value going to
		be, this pixel and its neighbours will share it until the edge end. This is the whole 
		point of no-AA optimization, in order to not subsample identical values. */
		replicator = skipto( scan, startx + 1, true );
		DEBUG("Y:%d R=%02x x %d\n", y, replicator, advance);
		if ( replicator > 0 )
			memset( map, replicator, advance );
		if ( advance > 1 )
			skipto( scan, startx + advance - 1, false);
		startx += advance;
		map    += advance;
	}

#ifdef _DEBUG
	BAIL: {
		int z = oldmaplen;
		Byte *p = oldmap;
		printf("> ");
		while (z--) {
			printf("%02x ", *(p++));
		}
		printf(":%d\n", oldmaplen);
	}
#endif
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

typedef struct {
	int             y_curr, y_lim, y_scan, xmin_px, dx, saved_x, y;
	unsigned int    maplen, curr_count;
	Bool            map_is_dirty;
	PolyPointBlock *block;
	ScanlinePtr     scanline_ptr;
	Point          *curr_point;
} AAFillRec, *PAAFillRec;

static int
aafill_init( NPoint *pts, int n_pts, int rule, Rect clip, PAAFillRec ctx)
{
	int    xmax_px;
	Point *pXpts = NULL;
	Rect   aa_extents;

	if (n_pts < 2)
		return -1;

	if (( pXpts = prepare_points_and_clip( pts, n_pts, &aa_extents)) == NULL)
		return 0;

	clip.left   *= AAX;
	clip.right  = (clip.right + 1 ) * AAX - 1;
	clip.bottom *= AAY;
	clip.top    = (clip.top + 1 ) * AAY - 1;
	if ( !intersect(&aa_extents, &clip)) {
		free( pXpts );
		return -1;
	}

	ctx->xmin_px = aa_extents.left   / AAX;
	xmax_px      = aa_extents.right  / AAX;
	ctx->dx      = ctx->xmin_px * AAX;
	ctx->maplen  = xmax_px - ctx->xmin_px + 1;
	DEBUG("EXTENTS %d-%d/%d-%d = %d-%d = %d\n", aa_extents.left, aa_extents.right, aa_extents.bottom, aa_extents.top, ctx->xmin_px, xmax_px , ctx->maplen);

	ctx->block = poly_poly2points(pXpts, n_pts, rule, &clip);
	free( pXpts );
	if ( ctx->block == NULL )
		return 0;

	ctx->y_curr                = aa_extents.bottom;
	ctx->y_lim                 = ctx->y_curr + AAY - 1;
	ctx->y_scan                = ctx->y_curr;
	ctx->y                     = ctx->y_scan >> AAY_SHIFT;
	bzero( &ctx->scanline_ptr, sizeof(ctx->scanline_ptr));
	ctx->scanline_ptr.block    = ctx->block;
	ctx->scanline_ptr.point[0] = ctx->block->pts;
	ctx->curr_count            = ctx->block->size;
	ctx->curr_point            = ctx->block->pts;
	ctx->saved_x               = -1;

	return 1;
}

static Bool
aafill_next_scanline( PAAFillRec ctx, Byte *map)
{
	int delta = 0;

	if (ctx->curr_count == 0 && !ctx->map_is_dirty )
		return false;

	ctx->map_is_dirty = false;
	bzero( map, ctx->maplen );
	if ( ctx->curr_point != ctx->block->pts)
		bzero( ctx->scanline_ptr.point, sizeof(ctx->scanline_ptr.point));

	if ( ctx-> saved_x >= 0 ) {
		int scanline;
		register Point *p = ctx->curr_point;

		while ( p->y > ctx->y_lim ) {
			ctx->y_lim  += AAY;
			ctx->y_curr += AAY;
		}
		ctx->y_scan = p->y;

		scanline = p->y - ctx->y_curr;
		ctx->scanline_ptr.point[scanline] = p;
		DEBUG("SET.%d(%d-0.%d) @ %d\n", (int)(p - ctx->block->pts), p->x, p->y, ctx->saved_x);

		map[ ctx-> saved_x ] = 1;
		ctx-> saved_x = -1;

		ctx-> map_is_dirty = true;
		ctx-> curr_point++;
		ctx-> curr_count--;
		delta = 1;
	}

	/*

	Traverse list of rendered point pairs, do the following:

	 - In the byte map, mark bytes that are crossing an edge. Those
	 will mark where the AA calculation must be done and where it
	 will be safe to reuse a previously calculated value (see more in fill())

	 - For each 0..AAY-1 scanlines, remember where each scanline started.

	*/

	while (ctx->curr_count > 0) {
		register int x;
		register Point *p = ctx->curr_point;

		if ( ctx->curr_count > 1 && p->x == p[1].x && p->y == p[1].y ) {
			if ( ctx-> curr_count == 1 ) {
				ctx-> curr_count = 0;
				break;
			}
			ctx->curr_point += 2;
			ctx->curr_count -= 2;
			continue;
		}

		if ( p-> y != ctx->y_scan ) delta = 0;
		x = (p->x - delta - ctx->dx) >> AAX_SHIFT;
		delta = !delta; /* last pixel of a line end, used by fmOutline but not by AA fills */

		if ( p-> y != ctx->y_scan ) {
			register int scanline;
			if ( p-> y > ctx->y_lim ) {
				fill( ctx->xmin_px, ctx->y_curr, map, ctx->maplen, &ctx->scanline_ptr);
				ctx->saved_x = x;
				ctx->y++;
				return true;
			}

			ctx->y_scan = p-> y;
			scanline = p-> y - ctx->y_curr;
			ctx->scanline_ptr.point[scanline] = p;
		}

		DEBUG("SET.%d(%d-%d.%d) @ %d\n", (int)(p - ctx->block->pts), p->x, !delta, p->y, x);
		map[x] = 1;
		ctx->map_is_dirty = true;
		ctx->curr_point++;
		ctx->curr_count--;
	}

	if ( ctx->map_is_dirty ) {
		ctx->map_is_dirty = false;
		fill( ctx->xmin_px, ctx->y_curr, map, ctx->maplen, &ctx->scanline_ptr);
		ctx->y++;
		return true;
	}

	return false;
}

static void
aafill_done(PAAFillRec ctx)
{
	free( ctx-> block );
}

Bool
img_aafill( Handle self, NPoint *pts, int n_pts, int rule, PImgPaintContext ctx)
{
	Byte *dst, *map;
	Rect clip;
	AAFillRec aa;
	Bool ok = false;
	Byte alpha;
	BlendFunc *blend;

	if (PImage(self)->type != imByte)
		return false;

	clip.left  = clip.bottom = 0;
	clip.right = PImage(self)->w - 1;
	clip.top   = PImage(self)->h - 1;

	switch (aafill_init( pts, n_pts, rule, clip, &aa)) {
		case -1: return false;
		case  0: return true ;
	}
	dst = PImage(self)->data + PImage(self)->lineSize * aa.y + aa.xmin_px;

	if ( ctx != NULL ) {
		int rop;
		if ( !( map = malloc( aa.maplen ))) {
			warn("img_aafill: no memory");
			goto EXIT;
		}
		rop   = ctx->rop & ropPorterDuffMask;
		alpha = (ctx-> rop & ropSrcAlpha) ? (( ctx->rop >> ropSrcAlphaShift ) & 0xff ) : 255;
		if ( !img_find_blend_proc( rop, &blend, NULL)) {
			warn("img_aafill: blend not supported");
			goto EXIT;
		}
	} else {
		map = dst;
		alpha = 255;
	}

	while ( aafill_next_scanline( &aa, map)) {
		if ( ctx == NULL ) {
			map += PImage(self)->lineSize;
		} else {
			Byte dummy;
			if ( alpha != 255 ) {
				register Byte *p = map;
				register unsigned int n = aa.maplen;
				while (n--) {
					*p = (alpha * *p) / 255.0 + .5;
					p++;
				}
			}
			blend( ctx->color, 0, map, 1, dst, &dummy, 0, aa.maplen);
			dst += PImage(self)->lineSize;
		}
	}

	if ( ctx != NULL )
		free(map);

	ok = true;

EXIT:
	aafill_done(&aa);
	return ok;
}

#ifdef __cplusplus
}
#endif
