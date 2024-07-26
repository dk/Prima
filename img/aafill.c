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

#define _DEBUG_MIXDOWN 0

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
				register int x2 = p[1].x;
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
	int             y_curr, y_lim, y_scan, x, dx, saved_x, y;
	unsigned int    maplen, curr_count;
	Bool            map_is_dirty, first_call;
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
	clip.bottom *= AAY;
	clip.right  = (clip.right + 1) * AAX - 1; /* convert to exclusive-inclusive coordinates */
	clip.top    = (clip.top   + 1) * AAY - 1;
	if ( !intersect(&aa_extents, &clip)) {
		free( pXpts );
		return -1;
	}

	ctx->x       = aa_extents.left   / AAX;
	xmax_px      = (aa_extents.right - 1) / AAX;
	ctx->dx      = ctx->x * AAX;
	ctx->maplen  = xmax_px - ctx->x + 1;

	DEBUG("EXTENTS %d-%d/%d-%d = %d-%d = %d\n", aa_extents.left, aa_extents.right, aa_extents.bottom, aa_extents.top, ctx->x, xmax_px , ctx->maplen);

	ctx->block = poly_poly2points(pXpts, n_pts, rule, &clip);
	free( pXpts );
	if ( ctx->block == NULL )
		return 0;

	ctx->y_curr                = aa_extents.bottom;
	ctx->y_lim                 = ctx->y_curr + AAY - 1;
	ctx->y_scan                = ctx->y_curr;
	ctx->y                     = (ctx->y_scan >> AAY_SHIFT) - 1;
	bzero( &ctx->scanline_ptr, sizeof(ctx->scanline_ptr));
	ctx->scanline_ptr.block    = ctx->block;
	ctx->scanline_ptr.point[0] = ctx->block->pts;
	ctx->curr_count            = ctx->block->size;
	ctx->curr_point            = ctx->block->pts;
	ctx->saved_x               = -1;
	ctx->first_call            = true;

	return 1;
}

static Bool
aafill_next_scanline( PAAFillRec ctx, Byte *map)
{
	int delta = 0;

	if (ctx->curr_count == 0 && !ctx->map_is_dirty )
		return false;

	ctx->y++;

	ctx->map_is_dirty = false;
	if ( map ) bzero( map, ctx->maplen );
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

		if ( map ) map[ ctx-> saved_x ] = 1;
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
				if ( map )
					fill( ctx->x, ctx->y_curr, map, ctx->maplen, &ctx->scanline_ptr);
				ctx->saved_x = x;
				return true;
			}

			ctx->y_scan = p-> y;
			scanline = p-> y - ctx->y_curr;
			ctx->scanline_ptr.point[scanline] = p;
		}

		DEBUG("SET.%d(%d-%d.%d) @ %d\n", (int)(p - ctx->block->pts), p->x, !delta, p->y, x);
		if ( map && x >= 0 && x < ctx->maplen ) map[x] = 1;
		ctx->map_is_dirty = true;
		ctx->curr_point++;
		ctx->curr_count--;
	}

	if ( ctx->map_is_dirty ) {
		ctx->map_is_dirty = false;
		if ( map )
			fill( ctx->x, ctx->y_curr, map, ctx->maplen, &ctx->scanline_ptr);
		return true;
	}

	return false;
}

static void
aafill_done(PAAFillRec ctx)
{
	free( ctx-> block );
}

static Bool
aafill_inplace( Handle self, NPoint *pts, int n_pts, int rule)
{
	Byte *map;
	Rect clip;
	AAFillRec aa;

	if (PImage(self)->type != imByte)
		return false;

	clip.left  = clip.bottom = 0;
	clip.right = PImage(self)->w - 1;
	clip.top   = PImage(self)->h - 1;

	switch (aafill_init( pts, n_pts, rule, clip, &aa)) {
		case -1: return false;
		case  0: return true ;
	}
	map = PImage(self)->data + PImage(self)->lineSize * (aa.y + 1) + aa.x;

	while ( aafill_next_scanline( &aa, map)) 
		map += PImage(self)->lineSize;
	aafill_done(&aa);
	return true;
}

#define FILL_PATTERN_SIZE sizeof(FillPattern)

#if _DEBUG_MIXDOWN
#define DEBUG_MIXDOWN(a,b,c) debug_mixdown(a,b,c)
static void debug_mixdown(char prefix, Byte *p, int z)
{
	printf("%c ", prefix);
	while (z--) {
		printf("%02x", *(p++));
	}
	printf("\n");
}
#else
#define DEBUG_MIXDOWN(a,b,c)
#endif


typedef struct {
	Byte src_alpha, dst_alpha, bpp, color_src_increment;
	Byte *fg, *bg;
	unsigned int pixels, bytes;
	int x, y;
	BlendFunc *blend1, *blend2;
	Byte *color_src, *color_pat, *color_mask, *color_alpha, *color_dst, *color_target;
	Byte *icon_src, *icon_dst, *icon_mask, *icon_alpha, *icon_target;
	Byte *tile_buffer;
} RenderContext, *PRenderContext;

typedef void RenderFunc(PIcon i, PImgPaintContext ctx, PRenderContext render);
typedef RenderFunc *PRenderFunc;

#define dRENDER(x) static void x(PIcon i, PImgPaintContext ctx, PRenderContext render)

static void
fill_gray_pattern( PImgPaintContext ctx, Byte *dst, Byte fg, Byte bg)
{
	Byte i, *ppat = ctx->pattern;
	for ( i = 0; i < FILL_PATTERN_SIZE; i++) {
		Byte *xdst = dst;
		register Byte pat = *(ppat++);
		*(dst++) = (pat & 0x80) ? fg : bg;
		*(dst++) = (pat & 0x40) ? fg : bg;
		*(dst++) = (pat & 0x20) ? fg : bg;
		*(dst++) = (pat & 0x10) ? fg : bg;
		*(dst++) = (pat & 0x08) ? fg : bg;
		*(dst++) = (pat & 0x04) ? fg : bg;
		*(dst++) = (pat & 0x02) ? fg : bg;
		*(dst++) = (pat & 0x01) ? fg : bg;
		memcpy( dst, xdst, FILL_PATTERN_SIZE);
		dst += FILL_PATTERN_SIZE;
	}
}

static void
render_copy_source(PIcon i, PImgPaintContext ctx, PRenderContext render)
{
	memcpy( render->color_dst, render->color_target, render->bytes);
	if ( render->icon_dst)
		memcpy( render->icon_dst, render->icon_target, render->pixels);
}

dRENDER(render_apply_transparent_pattern)
{
	Byte *pat = render->color_pat;
	register Byte *p   = render->icon_alpha;
	unsigned int bytes = render->pixels;

	pat += ((render->y + FILL_PATTERN_SIZE - ctx->patternOffset.y) % FILL_PATTERN_SIZE) * FILL_PATTERN_SIZE * 2;
	pat +=  (render->x + FILL_PATTERN_SIZE - ctx->patternOffset.x) % FILL_PATTERN_SIZE;

	while ( bytes > 0 ) {
		register Byte *s = pat;
		register int n = (bytes > FILL_PATTERN_SIZE) ? FILL_PATTERN_SIZE : bytes;
		bytes -= n;
		while (n--) *(p++) &= *(s++);
	}
}

static void
render_solid_rgb_init( PImgPaintContext ctx, PRenderContext render)
{
	unsigned int n = render->pixels;
	register Byte* line  = render->color_src;
	register Byte* color = ctx->color;
	if ( n > 12 ) {
		Byte stencil[12];
		stencil[0] = stencil[3] = stencil[6] = stencil[ 9] = color[0];
		stencil[1] = stencil[4] = stencil[7] = stencil[10] = color[1];
		stencil[2] = stencil[5] = stencil[8] = stencil[11] = color[2];
		while ( n > 4 ) {
			memcpy( line, stencil, 12 );
			line += 12;
			n -= 4;
		}
	}
	while ( n-- ) {
		*(line++) = color[0];
		*(line++) = color[1];
		*(line++) = color[2];
	}
}

void
render_opaque_rgb_pattern_init( PImgPaintContext ctx, Byte *dst)
{
	Byte i, *ppat = ctx->pattern;
	for ( i = 0; i < FILL_PATTERN_SIZE; i++) {
		Byte *xdst = dst;
		register Byte pat = *(ppat++), *color;
#define APPLY(x) \
	color = (pat & x) ? ctx->color : ctx->backColor; \
	*(dst++) = color[0];  \
	*(dst++) = color[1];  \
	*(dst++) = color[2]
		APPLY(0x80);
		APPLY(0x40);
		APPLY(0x20);
		APPLY(0x10);
		APPLY(0x08);
		APPLY(0x04);
		APPLY(0x02);
		APPLY(0x01);
#undef APPLY
		memcpy( dst, xdst, FILL_PATTERN_SIZE * 3);
		dst += FILL_PATTERN_SIZE * 3;
	}
}

dRENDER(render_opaque_pattern)
{
	unsigned int bytes = render->bytes;
	Byte *pat = render->color_pat, *s = render->color_src;
	int sz = FILL_PATTERN_SIZE * render->bpp;

	pat += ((render->y + FILL_PATTERN_SIZE - ctx->patternOffset.y) % FILL_PATTERN_SIZE) * sz * 2;
	pat += ((render->x + FILL_PATTERN_SIZE - ctx->patternOffset.x) % FILL_PATTERN_SIZE) * render->bpp;

	while ( bytes > 0 ) {
		int n = (bytes > sz) ? sz : bytes;
		memcpy( s, pat, n);
		s     += n;
		bytes -= n;
	}
}

static void
render_apply_1bit_mask(PImgPaintContext ctx, PRenderContext render, Byte *pat, unsigned int stride)
{
	PImage stipple = (PImage) ctx->tile;
	Byte   colorref[2] = {255,0};
	unsigned int x, n;
	register Byte *d = render->icon_alpha;

	pat += ((render->y + stipple->h - ctx->patternOffset.y) % stipple->h) * stride;
	bc_mono_byte_cr( pat, render->tile_buffer, stipple->w, colorref);

	x = (render->x + stipple->w - ctx->patternOffset.x) % stipple->w;
	n = render->pixels;

	while ( n > 0 ) {
		register Byte *s = render->tile_buffer + x;
		register unsigned int bytes = stipple-> w - x;
		if ( bytes > n ) bytes = n;
		n -= bytes;
		while (bytes--) *d++ &= *s++;
		x = 0;
	}
}

dRENDER(render_apply_stipple)
{
	PImage stipple = (PImage) ctx->tile;
	render_apply_1bit_mask(ctx, render, stipple->data, stipple->lineSize);
}

dRENDER(render_apply_stipple_mask)
{
	PIcon stipple = (PIcon) ctx->tile;
	render_apply_1bit_mask(ctx, render, stipple->mask, stipple->maskLine);
}

dRENDER(render_apply_tile_mask)
{
	PIcon stipple = (PIcon) ctx->tile;
	unsigned int x, n;
	Byte *d = render->icon_alpha, *pat = stipple->mask;

	pat += ((render->y + stipple->h - ctx->patternOffset.y) % stipple->h) * stipple->maskLine;

	x = (render->x + stipple->w - ctx->patternOffset.x) % stipple->w;
	n = render->pixels;

	while ( n > 0 ) {
		Byte *s = pat + x;
		unsigned int bytes = stipple-> w - x;
		if ( bytes > n ) bytes = n;
		n -= bytes;
		x = 0;
		//memcpy( d, s, bytes);
		img_multiply_alpha( s, d, 1, d, bytes);
		d += bytes;
	}
}

static void
render_generic_tile(PImgPaintContext ctx, PRenderContext render, Byte *pattern)
{
	unsigned int x, p, w;
	Byte *dst = render->color_src, bpp = render->bpp;

	w = PImage(ctx->tile)-> w;
	x = (render->x + w - ctx->patternOffset.x) % w;
	p = render->pixels;

	while ( p > 0 ) {
		unsigned int pixels = w - x;
		if ( pixels > p ) pixels = p;
		p      -= pixels;
		pixels *= bpp;
		memcpy( dst, pattern + x * bpp, pixels);
		dst    += pixels;
		x       = 0;
	}
}

dRENDER(render_opaque_gray_stipple)
{
	PImage stipple = (PImage) ctx->tile;
	Byte      *pat = stipple->data, colorref[2] = {render->fg[0],render->bg[0]};

	pat += ((render->y + stipple->h - ctx->patternOffset.y) % stipple->h) * stipple->lineSize;
	bc_mono_byte_cr( pat, render->tile_buffer, stipple->w, colorref);
	render_generic_tile(ctx, render, render->tile_buffer);
}

dRENDER(render_opaque_rgb_stipple)
{
	PImage stipple = (PImage) ctx->tile;
	Byte      *pat = stipple->data;
	RGBColor colorref[2];

	memcpy( &colorref[0], render->fg, 3);
	memcpy( &colorref[1], render->bg, 3);
	pat += ((render->y + stipple->h - ctx->patternOffset.y) % stipple->h) * stipple->lineSize;
	bc_mono_rgb( pat, render->tile_buffer, stipple->w, colorref);

	render_generic_tile(ctx, render, render->tile_buffer);
}

dRENDER(render_save_alpha)
{
	memcpy( render->icon_src, render->icon_alpha, render->pixels);
}

dRENDER(render_apply_constant_alpha_to_icon_src)
{
	img_multiply_alpha( render->icon_alpha, &render->src_alpha, 0, render->icon_src, render->bytes);
}

dRENDER(render_tile)
{
	PImage stipple = (PImage) ctx->tile;
	Byte      *pat = stipple->data;
	pat += ((render->y + stipple->h - ctx->patternOffset.y) % stipple->h) * stipple->lineSize;
	render_generic_tile(ctx, render, pat);
}

static void
mixdown( Byte *src, Byte *mask, Byte *dst, unsigned int bytes)
{
	while (bytes-- > 0) {
		if (*mask++)
			*dst = *src;
		dst++;
		src++;
	}
}

dRENDER(render_mixdown)
{
	DEBUG_MIXDOWN('C', render->color_src, (render->color_src_increment == 1) ? render->bytes : 1);

	if ( render->icon_mask != render->icon_alpha )
		memcpy( render->icon_mask, render->icon_alpha, render->pixels); /* alpha may become 0 but mask should not */
	if ( render->src_alpha < 255 )
		img_multiply_alpha( render->icon_alpha, &render->src_alpha, 0, render->icon_alpha, render->pixels);
	if ( render->bpp == 3 ) {
		bc_graybyte_rgb( render->icon_mask,  render->color_mask,  render->pixels);
		if ( render->color_mask != render->color_alpha )
			bc_graybyte_rgb( render->icon_alpha, render->color_alpha, render->pixels);
	}
	DEBUG_MIXDOWN('A', render->color_alpha, render->bytes);
	DEBUG_MIXDOWN('+', render->color_dst, render->bytes);
	render->blend1(
		render->color_src, render->color_src_increment,
		render->color_alpha, 1,
		render->color_dst,
		render->icon_target ? render->icon_target : &render->dst_alpha,
		render->icon_target ? 1 : 0,
		render->bytes);

	DEBUG_MIXDOWN('=', render->color_dst, render->bytes);
	if ( render->color_target == render->color_dst ) {
		render->color_dst    += i->lineSize;
		render->color_target += i->lineSize;
	} else {
		DEBUG_MIXDOWN('*', render->color_mask, render->bytes);
		mixdown( render->color_dst, render->color_mask, render->color_target, render->bytes );
		DEBUG_MIXDOWN('=', render->color_dst, render->bytes);
		render->color_target += i->lineSize;
	}

	if ( render->icon_target ) {
		DEBUG_MIXDOWN('M', render->icon_src,   render->pixels);
		DEBUG_MIXDOWN('A', render->icon_alpha, render->pixels);
		DEBUG_MIXDOWN('+', render->icon_dst,   render->pixels);
		render->blend2(
			render->icon_src,    1,
			render->icon_alpha,  1,
			render->icon_dst,
			render->icon_target, 1,
			render->pixels
		);
		DEBUG_MIXDOWN('=', render->icon_dst, render->pixels);

		if ( render->icon_target == render->icon_dst ) {
			render->icon_target += i->maskLine;
			render->icon_dst    += i->maskLine;
		} else {
			DEBUG_MIXDOWN('*', render->icon_mask, render->pixels);
			mixdown( render->icon_dst, render->icon_mask, render->icon_target, render->pixels );
			DEBUG_MIXDOWN('=', render->icon_dst, render->pixels);
			render->icon_target += i->lineSize;
		}
	}
}

static void
advance_pointers(PIcon i, PRenderContext render)
{
	if ( render->color_target == render->color_dst ) {
		render->color_dst    += i->lineSize;
		render->color_target += i->lineSize;
	}
	if ( render->icon_target && render->icon_target == render->icon_dst ) {
		render->icon_target += i->maskLine;
		render->icon_dst    += i->maskLine;
	}
}

static Bool
conversion_needed( Handle self, NPoint *pts, int n_pts, int rule, PImgPaintContext ctx, Bool *ok)
{
	PIcon i = (PIcon) self;
	int bpp = ( i->type & imGrayScale) ? imByte : imRGB;
	Bool is_icon = kind_of(self, CIcon);
	ImagePreserveTypeRec p;

	if (i-> type != bpp || ( is_icon && i->maskType != imbpp8 )) {
		i->self-> begin_preserve_type( self, &p );
		if ( i->type != bpp ) {
			img_resample_colors( self, bpp, ctx );
			i->self-> set_type( self, bpp );
		}
		if ( is_icon && i->maskType != imbpp8 )
			i->self->set_maskType( self, imbpp8 );
		*ok = img_aafill( self, pts, n_pts, rule, ctx);
		i-> self-> end_preserve_type( self, &p );
		return true;
	}

	if ( ctx->tile != NULL_HANDLE ) {
		PIcon tile = (PIcon) ctx->tile;
		Bool tile_is_icon = kind_of( ctx->tile, CIcon );

		/* can only do mono non-icon tiles or those that match bpp and type */
		if (
			tile->type != i->type &&
			(tile_is_icon || tile->type != imBW)
		) {
			CImage(ctx->tile)-> begin_preserve_type( ctx->tile, &p );
			CImage(ctx->tile)-> set_type( ctx->tile, i->type );
			*ok = img_aafill( self, pts, n_pts, rule, ctx);
			CImage(ctx->tile)-> end_preserve_type( ctx->tile, &p );
			return true;
		}
	}

	return false;
}

Bool
img_aafill( Handle self, NPoint *pts, int n_pts, int rule, PImgPaintContext ctx)
{
	PIcon i = (PIcon) self, tile;
	Rect clip;
	AAFillRec aa;
	Bool ok = false, pattern, dual_pattern, is_icon, tile_is_icon;
	int rop, n_renders = 0;
	RenderContext render;
	PRenderFunc pipeline[10];
	Byte *arena = NULL, *rgn_map = NULL;
	PRegionScanlineIterator rgn = NULL;
	PBitBltProc rgn_puncher = NULL;

	if ( ctx == NULL )
		return aafill_inplace(self, pts, n_pts, rule);

	/* align types and geometry - can only operate over imByte and imRGB and 8-bit icons */
	if ( conversion_needed(self, pts, n_pts, rule, ctx, &ok))
		return ok;

	is_icon = kind_of(self, CIcon);

	bzero(&render, sizeof(render));
	render.bpp = ( i->type & imBPP ) / 8;
	render.fg  = ctx->color;
	render.bg  = ctx->backColor;
	if ( ctx->tile == NULL_HANDLE ) {
		if ( memcmp(ctx->pattern, fillPatterns[fpEmpty], sizeof(FillPattern)) == 0) {
			if ( !ctx->transparent )
				return true;
			render.fg = render.bg;
			pattern = false;
		} else
			pattern = memcmp(ctx->pattern, fillPatterns[fpSolid], sizeof(FillPattern)) != 0;
		dual_pattern = pattern ? !ctx->transparent : false;
		ctx->patternOffset.y %= FILL_PATTERN_SIZE;
		ctx->patternOffset.x %= FILL_PATTERN_SIZE;
		tile_is_icon = false;
		tile = NULL;
	} else {
		tile = (PIcon) ctx->tile;
		tile_is_icon = kind_of( ctx->tile, CIcon );
		ctx->patternOffset.y %= PImage(ctx->tile)->h;
		ctx->patternOffset.x %= PImage(ctx->tile)->w;
		dual_pattern = pattern = false;
	}

	clip.left  = clip.bottom = 0;
	clip.right = i->w - 1;
	clip.top   = i->h - 1;
	switch (aafill_init( pts, n_pts, rule, clip, &aa)) {
		case -1: return false;
		case  0: return true ;
	}

	if ( ctx->region != NULL ) {
		if (( rgn = img_region_iterate_scanline( ctx->region )) == NULL)
			goto EXIT;
		rgn_puncher = img_find_blt_proc(ropAndPut);
	}

	rop                 = ctx->rop & ropPorterDuffMask;
	if ( !img_find_blend_proc( rop, &render.blend1, &render.blend2)) {
		warn("img_aafill: blend not supported");
		goto EXIT;
	}
	render.x            = aa.x;
	render.y            = aa.y;
	render.pixels       = aa.maplen;
	render.bytes        = render.pixels * render.bpp;
	render.color_target = i->data + i->lineSize * (aa.y + 1) + aa.x * render.bpp;
	render.src_alpha    = (ctx->rop & ropSrcAlpha) ? ((ctx->rop >> ropSrcAlphaShift) & 0xff) : 0xff;
	render.dst_alpha    = (ctx->rop & ropDstAlpha) ? ((ctx->rop >> ropDstAlphaShift) & 0xff) : 0xff;
	if ( is_icon )
		render.icon_target = i->mask + i->maskLine * (aa.y + 1) + aa.x;

	{
#define ROUND_MEM(x) sizeof(void*) * ((x / sizeof(void*)) + ((x % sizeof(void*)) ? 1 : 0))
		unsigned int sz_color, sz_icon, sz_pat, sz_tile, sz_rgn;
		int need_extra_mask = (is_icon && render.bpp == 3) ? 1 : 0;
		Byte *last;
		sz_color = ROUND_MEM(render.bytes);
		sz_icon  = is_icon ? ROUND_MEM(render.pixels) : 0;
		sz_pat   = pattern ? ROUND_MEM(FILL_PATTERN_SIZE * FILL_PATTERN_SIZE * render.bpp * 2) : 0;
		sz_rgn   = ctx->region ? render.pixels : 0;
		sz_tile  = (
				ctx->tile != NULL_HANDLE && (
					(tile_is_icon && (tile->maskType == 1)) ||
					tile->type == imBW
				)
			) ? ROUND_MEM(tile-> w * render.bpp * 8) : 0;

#undef ROUND_MEM
		if ( !( last = arena = malloc(
			sz_color * 4 +
			sz_icon  * (2 + 2 * need_extra_mask) +
			sz_pat +
			sz_rgn +
			sz_tile
		))) {
			warn("not enough memory");
			goto EXIT;
		}
		last = (render.color_src   = last) + sz_color;
		last = (render.color_dst   = last) + sz_color;
		last = (render.color_mask  = last) + sz_color;
		last = (render.color_alpha = last) + sz_color;
		if ( sz_icon > 0 ) {
			last = (render.icon_src  = last) + sz_icon;
			last = (render.icon_dst  = last) + sz_icon;
		}
		if ( sz_pat > 0 )
			last = (render.color_pat = last) + sz_pat;
		if ( sz_rgn > 0 )
			last = (rgn_map = last) + sz_pat;
		if ( sz_tile > 0 )
			last = (render.tile_buffer = last) + sz_tile;

		if (need_extra_mask) {
			last = (render.icon_mask   = last) + sz_icon;
			last = (render.icon_alpha  = last) + sz_icon;
		} else {
			render.icon_mask  = render.color_mask;
			render.icon_alpha = render.color_alpha;
		}
	}

	if ( pattern ) {
		if (dual_pattern) {
			if ( render.bpp == 3 )
				render_opaque_rgb_pattern_init(ctx, render.color_pat);
			else
				fill_gray_pattern( ctx, render.color_pat, render.fg[0], render.bg[0]);
		} else
			fill_gray_pattern( ctx, render.color_pat, 0xff, 0);
	}

#define PUSH(x) pipeline[n_renders++] = &x

	if ( rop == ropSrcOver ) {
		/* it is special because SrcOver with alpha pixel=0 is same as mask pixel=0, so
		rendering over a copy of source pixels and consequential masking can be
		effectively avoided. */
		render.color_dst  = render.color_target;
		if ( render.icon_target )
			render.icon_dst  = render.icon_target;
		if ( render.src_alpha == 255 ) {
			render.color_mask = render.color_alpha;
			render.icon_mask  = render.icon_alpha;
		}
	} else
		PUSH(render_copy_source);

	if ( ctx->tile ) {
		if ( tile_is_icon ) {
			if (tile->maskType == 1)
				PUSH(render_apply_stipple_mask);
			else
				PUSH(render_apply_tile_mask);
		} else if ( tile->type == imBW && ctx->transparent)
			PUSH(render_apply_stipple);
	}

	render.color_src_increment = 1;
	if ( render.bpp == 3 ) {
		Bool lift_alpha = false;
		if ( ctx->tile) {
			if ( tile->type == imBW && !tile_is_icon) {
				if ( ctx->transparent )
					render_solid_rgb_init(ctx,&render);
				else
					PUSH(render_opaque_rgb_stipple);
			} else {
				PUSH(render_tile);
				if ( tile_is_icon && tile->maskType == 8 && is_icon ) {
					PUSH(render_save_alpha);
					lift_alpha = true;
				}
			}
		} else if ( dual_pattern )
			PUSH(render_opaque_pattern);
		else {
			if (pattern)
				PUSH(render_apply_transparent_pattern);
			render_solid_rgb_init(ctx,&render);
		}
		if ( is_icon && !lift_alpha )
			PUSH(render_apply_constant_alpha_to_icon_src);
	} else {
		Bool lift_alpha = false;
		if ( ctx->tile) {
			if ( tile->type == imBW && !tile_is_icon) {
				if ( ctx->transparent ) {
					render.color_src_increment = 0;
					render.color_src[0] = render.fg[0];
				} else
					PUSH(render_opaque_gray_stipple);
			} else {
				PUSH(render_tile);
				if ( tile_is_icon && tile->maskType == 8 && is_icon ) {
					PUSH(render_save_alpha);
					lift_alpha = true;
				}
			}
		} else if ( dual_pattern )
			PUSH(render_opaque_pattern);
		else {
			if (pattern)
				PUSH(render_apply_transparent_pattern);
			render.color_src_increment = 0;
			render.color_src[0] = render.fg[0];
		}
		if ( is_icon && !lift_alpha )
			render.icon_src = render.icon_alpha;
	}

	PUSH(render_mixdown);
#undef PUSH

	if ( rgn ) {
		while ( rgn->y < aa.y ) {
			if ( !img_region_next_scanline(rgn)) {
				ok = true;
				goto EXIT;
			}
		}
	}

	while ( 1 ) {
		int i;
		Bool fast_skip = false;
		if ( rgn ) {
			if (rgn->y <= aa.y ) {
				if (!img_region_next_scanline(rgn))
					break;
				img_region_fill_scanline_map(rgn, rgn_map, render.x, render.pixels);
				DEBUG_MIXDOWN('R', rgn_map, render.pixels);
				if (memchr(rgn_map, 0xff, render.pixels) == NULL)
					fast_skip = true;
			} else
				fast_skip = true;
		}
		if ( !aafill_next_scanline( &aa, fast_skip ? NULL : render.icon_alpha))
			break;
		if ( fast_skip ) {
			advance_pointers((PIcon)self, &render);
			continue;
		}
		if ( rgn )
			rgn_puncher(rgn_map, render.icon_alpha, render.pixels);
		render.y = aa.y;
		for ( i = 0; i < n_renders; i++)
			pipeline[i]((PIcon)self,ctx,&render);
	}

	ok = true;

EXIT:
	if ( arena ) free(arena);
	if ( rgn )   free(rgn);
	aafill_done(&aa);
	return ok;
}

#ifdef __cplusplus
}
#endif
