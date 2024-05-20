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
		fr->ctx->color, 0,
		scanline, 1,
		dst,
		&dummy_alpha, 0,
		w
	);
}

/* call fill procedure over the parts that are to be painted, skip the others */
static void
execute( FillRec *fill_rec, FillProc * fill_proc, Byte *scanline, int w, int src_x, int y)
{
#ifdef _DEBUG
	{
		int z = w;
		Byte *p = scanline;
		printf("%d: ", y);
		while (z--) {
			printf("%02x ", *(p++));
		}
		printf("\n");
	}
#endif

	/* don't check boundaries as this is delegated to a common cliprect routine */
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

static Bool
skip_to_y( ScanlinePtr *ptr, int y)
{
	while ( ptr-> block ) {
		Point *last = ptr->block->pts + ptr->block->size;
		while ( ptr->point != last ) {
			if ( ptr->point->y >= y )
				return ptr->point->y == y;
			ptr->point += 2;
		}
		if (( ptr-> block = ptr->block->next) == NULL) {
			ptr-> point = NULL;
			return false;
		}
		ptr-> point = ptr->block->pts;
	}
	return false;
}

static void
mask( ScanlinePtr *ptr, Byte *map, unsigned int maplen, int offset )
{
	int x = offset, y = ptr->point->y;

	DEBUG("BLANKING %d %d\n", y, offset);
	while ( ptr-> block ) {
		Point *last = ptr->block->pts + ptr->block->size;
		while ( ptr->point != last ) {
			register Point *p = ptr->point;
			if ( p->y > y )
				goto STOP;
			if ( x < p->x ) {
				bzero( map + x - offset, p->x - x );
				DEBUG("%d: BLANK %d - %d\n", y, x, p->x - 1);
			}
			x = p[1].x + 1;
			ptr->point += 2;
		}
		if (( ptr-> block = ptr->block->next) == NULL) {
			ptr-> point = NULL;
			break;
		}
		ptr-> point = ptr->block->pts;
	}
STOP:
	if ( maplen - x + offset > 0 )
		bzero( map + x - offset, maplen - x + offset);
	DEBUG("%d: BLANY %d - %d\n", y, x, maplen - x + offset);
}

static Bool
intersect_1box_region( RegionRec *region, Rect *clip)
{
	Rect r2;
	r2.left   = region-> boxes[0].x;
	r2.bottom = region-> boxes[0].y;
	r2.right  = r2.left   + region-> boxes[0].width  - 1;
	r2.top    = r2.bottom + region-> boxes[0].height - 1;
	if ( !intersect(clip, &r2))
		return false;
	DEBUG("BOX1: %d %d %d %d\n",
		clip->left, clip->bottom, clip->right, clip->top);
	return true;
}

Bool
img_aafill( Handle self, NPoint *pts, int n_pts, int rule, PImgPaintContext ctx)
{
	int y_curr, y_lim, y_scan, xmin_px, xmax_px;
	unsigned int maplen;
	Bool ok = false, map_is_dirty;
	Point *pXpts = NULL;
	PolyPointBlock *first = NULL, *curr = NULL, *complex_clip = NULL;
	Byte *map = NULL;
	ScanlinePtr scanline_ptr[AAY], clip_ptr = { NULL, NULL };
	FillRec fill_rec;
	FillProc *fill_proc;
	Rect clip, aa_extents;
	int bpp;

	if (n_pts < 2)
		return false;

	fill_rec.i   = (PImage) self;
	fill_rec.ctx = ctx;
	if ( ctx->rop < ropMinPDFunc || ctx->rop > ropMaxPDFunc )
		ctx->rop = ropSrcOver;
	if ( ctx-> rop == ropDstCopy)
		return true;
	if ( !img_find_blend_proc(ctx->rop, &fill_rec.blend1, &fill_rec.blend2))
		return false;
	if ( ctx->transparent && (memcmp( ctx->pattern, fillPatterns[fpEmpty], sizeof(FillPattern)) == 0))
		return true;

	bpp = ( PImage(self)->type & imGrayScale) ? imByte : imRGB;
	if (PImage(self)-> type != bpp || ( kind_of( self, CIcon) && PIcon(self)->maskType != imbpp8 )) {
		Bool ok;
		ImagePreserveTypeRec p;
		CImage(self)-> begin_preserve_type( self, &p );
		if ( PImage(self)->type != bpp ) {
			img_resample_colors( self, bpp, ctx );
			CIcon(self)-> set_type( self, bpp );
		}
		if ( kind_of(self, CIcon) && PIcon(self)->maskType != imbpp8 )
			CIcon(self)-> set_maskType( self, imbpp8 );
		ok = img_aafill( self, pts, n_pts, rule, ctx);
		CImage(self)-> end_preserve_type( self, &p );
		return ok;
	}

	fill_proc = fill_solid_color_byte;

	clip.left  = clip.bottom = 0;
	clip.right = fill_rec.i->w - 1;
	clip.top   = fill_rec.i->h - 1;
	if ( ctx->region ) {
		switch ( ctx-> region-> n_boxes ) {
		case 0:
			return true;
		case 1:
			if (!intersect_1box_region( ctx-> region, &clip))
				return true;
		default:
			if (( complex_clip = poly_region2points( ctx->region, &clip)) == NULL)
				goto FAIL;
			DEBUG("COMPLEX REGION\n");
		}
	}

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

	if (( map = malloc(maplen)) == NULL)
		goto FAIL;

	y_curr = aa_extents.bottom;
	y_lim  = y_curr + AAY - 1;
	y_scan = y_curr;
	bzero( map, maplen );
	map_is_dirty = false;
	bzero( scanline_ptr, sizeof(scanline_ptr));
	scanline_ptr[0].block = curr;
	scanline_ptr[0].point = curr->pts;
	if ( complex_clip ) {
		clip_ptr.block = complex_clip;
		clip_ptr.point = complex_clip->pts;
		skip_to_y( &clip_ptr, y_curr >> AAY_SHIFT );
	}

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
					if ( complex_clip ) {
						if ( skip_to_y( &clip_ptr, y_curr >> AAY_SHIFT ))
							mask( &clip_ptr, map, maplen, xmin_px );
						else
							goto SKIP1;
					}
					execute( &fill_rec, fill_proc, map, maplen, xmin_px, y_curr >> AAY_SHIFT);
				SKIP1:
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
			DEBUG("SET.%ld(%d.%d) @ %d\n", p - curr->pts, p->x, p->y, (p->x - aa_extents.left) >> AAX_SHIFT);
			map[ ( p-> x - aa_extents.left ) >> AAX_SHIFT ] = 1;
			map_is_dirty = true;
			p++;
		}
		curr = curr->next;
	}
	if ( map_is_dirty ) {
		fill( xmin_px, y_curr, map, maplen, scanline_ptr);
		if ( complex_clip ) {
			if ( skip_to_y( &clip_ptr, y_curr >> AAY_SHIFT ))
				mask( &clip_ptr, map, maplen, xmin_px );
			else
				goto SKIP2;
		}
		execute( &fill_rec, fill_proc, map, maplen, xmin_px, y_curr >> AAY_SHIFT);
	SKIP2:;
	}

	ok = true;
FAIL:
	if ( map )
		free( map );
	if ( pXpts ) 
		free( pXpts );
	if ( first )
		poly_free_blocks( first );
	if ( complex_clip )
		poly_free_blocks( complex_clip );
	return ok;
}

#ifdef __cplusplus
}
#endif
