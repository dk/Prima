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
	int             y_curr, y_lim, y_scan, x, dx, saved_x, y;
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

	ctx->x       = aa_extents.left   / AAX;
	xmax_px      = aa_extents.right  / AAX;
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
				fill( ctx->x, ctx->y_curr, map, ctx->maplen, &ctx->scanline_ptr);
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
		fill( ctx->x, ctx->y_curr, map, ctx->maplen, &ctx->scanline_ptr);
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
	map = PImage(self)->data + PImage(self)->lineSize * aa.y + aa.x;

	while ( aafill_next_scanline( &aa, map)) 
		map += PImage(self)->lineSize;
	aafill_done(&aa);
	return true;
}

#define FILL_PATTERN_SIZE sizeof(FillPattern)

typedef struct {
	Byte alpha, bpp;
	BlendFunc *blend1, *blend2;
	Byte *fg, *bg;
	Byte *dst;
	unsigned int pixels, bytes;
	int x, y;
} RenderContext, *PRenderContext;

typedef int   RenderGetSize(PImgPaintContext ctx, PRenderContext render);
typedef Bool  RenderInit   (PImgPaintContext ctx, PRenderContext render, void *self);
typedef Byte* RenderFunc   (PImgPaintContext ctx, PRenderContext render, void *self, Byte* src);
typedef void  RenderDone   (void *self);
typedef struct {
	RenderGetSize *GetSize;
	RenderInit    *Init;
	RenderFunc    *Render;
	RenderDone    *Done;
	void          *self;
	unsigned int   offset;
} RenderObject, *PRenderObject;

static int
render_acquire_map_get_size( PImgPaintContext ctx, PRenderContext render)
{
	return render-> pixels;
}

static RenderObject obj_render_acquire_map = {
	render_acquire_map_get_size
};

static Byte*
render_apply_alpha(PImgPaintContext ctx, PRenderContext render, void *self, Byte* src)
{
	register Byte *p        = src;
	register Byte alpha     = render->alpha;
	register unsigned int n = render->pixels;
	while (n--) {
		*p = (alpha * *p) / 255.0 + .5;
		p++;
	}
	return src;
}

static RenderObject obj_render_apply_alpha = {
	NULL,
	NULL,
	render_apply_alpha
};

static Byte*
render_solid_gray(PImgPaintContext ctx, PRenderContext render, void *self, Byte *src)
{
	Byte dummy = 0;
	render->blend1( render->fg, 0, src, 1, render->dst, &dummy, 0, render->pixels);
	return render->dst;
}

static RenderObject obj_render_blend_solid_gray = {
	NULL,
	NULL,
	render_solid_gray
};

static int
render_apply_transparent_pattern_get_size( PImgPaintContext ctx, PRenderContext render)
{
	return FILL_PATTERN_SIZE * 8 * 2;
}

static void
fill_gray_pattern( PImgPaintContext ctx, Byte *dst, Byte fg, Byte bg)
{
	Byte i, *ppat = ctx->pattern;
	ctx->patternOffset.y %= FILL_PATTERN_SIZE;
	ctx->patternOffset.x %= FILL_PATTERN_SIZE;
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

static Bool
render_apply_transparent_pattern_init( PImgPaintContext ctx, PRenderContext render, void *self)
{
	fill_gray_pattern( ctx, (Byte*) self, 0xff, 0);
	return true;
}

static Byte*
render_apply_transparent_pattern(PImgPaintContext ctx, PRenderContext render, void *self, Byte *src)
{
	Byte *pat = (Byte*) self;
	register Byte *p = src;
	unsigned int bytes = render->pixels;

	pat += ((render->y + FILL_PATTERN_SIZE - ctx->patternOffset.y) % FILL_PATTERN_SIZE) * FILL_PATTERN_SIZE * 2;
	pat +=  (render->x + FILL_PATTERN_SIZE - ctx->patternOffset.x) % FILL_PATTERN_SIZE;

	while ( bytes > 0 ) {
		register Byte *s = pat;
		register int n = (bytes > FILL_PATTERN_SIZE) ? FILL_PATTERN_SIZE : bytes;
		bytes -= n;
		while (n--) *(p++) &= *(s++);

	}

	return src;
}

static RenderObject obj_render_apply_transparent_pattern = {
	render_apply_transparent_pattern_get_size,
	render_apply_transparent_pattern_init,
	render_apply_transparent_pattern
};

static Bool
render_opaque_gray_pattern_init( PImgPaintContext ctx, PRenderContext render, void *self)
{
	fill_gray_pattern( ctx, (Byte*) self, render->fg[0], render->bg[0]);
	return true;
}

static Byte*
render_opaque_pattern(PImgPaintContext ctx, PRenderContext render, void *self, Byte *src)
{
	unsigned int bytes = render->bytes;
	Byte *pat = (Byte*) self, dummy = 0, *s = src, *d = render->dst;
	int sz = FILL_PATTERN_SIZE * render->bpp;

	pat += ((render->y + FILL_PATTERN_SIZE - ctx->patternOffset.y) % FILL_PATTERN_SIZE) * sz * 2;
	pat += ((render->x + FILL_PATTERN_SIZE - ctx->patternOffset.x) % FILL_PATTERN_SIZE) * render->bpp;

	while ( bytes > 0 ) {
		int n = (bytes > sz) ? sz : bytes;
		render->blend1(pat, 1, s, 1, d, &dummy, 0, n);
		s     += n;
		d     += n;
		bytes -= n;
	}

	return src;
}

static RenderObject obj_render_opaque_gray_pattern = {
	render_apply_transparent_pattern_get_size,
	render_opaque_gray_pattern_init,
	render_opaque_pattern
};

static int
render_map_to_rgb_get_size( PImgPaintContext ctx, PRenderContext render)
{
	return render->bytes;
}

static Byte*
render_map_to_rgb(PImgPaintContext ctx, PRenderContext render, void *self, Byte *src)
{
	bc_graybyte_rgb( src, (Byte*) self, render->pixels);
	return (Byte*) self;
}

static RenderObject obj_render_map_to_rgb = {
	render_map_to_rgb_get_size,
	NULL,
	render_map_to_rgb
};

static int
render_solid_rgb_get_size( PImgPaintContext ctx, PRenderContext render)
{
	return render->bytes;
}

static Bool
render_solid_rgb_init( PImgPaintContext ctx, PRenderContext render, void *self)
{
	unsigned int n = render->pixels;
	register Byte* line  = (Byte*) self;
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
	return true;
}

static Byte*
render_solid_rgb(PImgPaintContext ctx, PRenderContext render, void *self, Byte *src)
{
	Byte dummy = 0;
	render->blend1((Byte*) self, 1, src, 1, render->dst, &dummy, 0, render->bytes);
	return render->dst;
}

static RenderObject obj_render_blend_solid_rgb = {
	render_solid_rgb_get_size,
	render_solid_rgb_init,
	render_solid_rgb
};

static int
render_opaque_rgb_pattern_get_size( PImgPaintContext ctx, PRenderContext render)
{
	return FILL_PATTERN_SIZE * 8 * 2 * 3;
}

static Bool
render_opaque_rgb_pattern_init( PImgPaintContext ctx, PRenderContext render, void *self)
{
	Byte i, *ppat = ctx->pattern, *dst = (Byte*) self;
	ctx->patternOffset.y %= FILL_PATTERN_SIZE;
	ctx->patternOffset.x %= FILL_PATTERN_SIZE;
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
	return true;
}

static RenderObject obj_render_opaque_rgb_pattern = {
	render_opaque_rgb_pattern_get_size,
	render_opaque_rgb_pattern_init,
	render_opaque_pattern
};

static void
render_done( PRenderObject *pipeline, int n_renders)
{
	int i;
	Byte *arena = pipeline[0]->self;
	for ( i = 0; i < n_renders; i++) {
		RenderDone *f = pipeline[i]->Done;
		if (f && pipeline[i]->self)
			f(pipeline[i]->self);
		pipeline[i]->self = NULL;
	}
	if (arena)
		free(arena);
}

static void*
render_init( PRenderObject *pipeline, int n_renders, PImgPaintContext ctx, PRenderContext render)
{
	int i;
	Byte *arena, *ptr;
	unsigned int arena_size = 0;

	for ( i = 0; i < n_renders; i++) {
		unsigned int sz;
		RenderGetSize *f = pipeline[i]->GetSize;
		if ( f ) {
			sz = f(ctx, render);
			sz = ((sz / sizeof(void*)) + (sz % sizeof(void*))) * sizeof(void*);
			pipeline[i]->offset = sz;
			arena_size += sz;
		} else
			pipeline[i]->offset = 0;
		pipeline[i]->self = NULL;
	}

	if ( !( arena = malloc( arena_size ))) {
		warn("no memory");
		return NULL;
	}

	ptr = arena;
	for ( i = 0; i < n_renders; i++) {
		RenderInit *f = pipeline[i]->Init;
		if (f && !f(ctx, render, ptr)) {
			render_done(pipeline, i - 1);
			return NULL;
		}
		pipeline[i]->self = ptr;
		ptr += pipeline[i]->offset;
	}

	return arena;
}

static void
render_run( PRenderObject *pipeline, int n_renders, PImgPaintContext ctx, PRenderContext render)
{
	int i;
	Byte *ptr = pipeline[0]->self;
	for ( i = 0; i < n_renders; i++) {
		RenderFunc *f = pipeline[i]->Render;
		if (f)
			ptr = f(ctx, render, pipeline[i]->self, ptr);
	}
}

Bool
img_aafill( Handle self, NPoint *pts, int n_pts, int rule, PImgPaintContext ctx)
{
	Rect clip;
	AAFillRec aa;
	Bool ok = false, pattern, dual_pattern;
	int rop, n_renders = 0;
	RenderContext render;
	PRenderObject pipeline[6];

	if ( ctx == NULL )
		return aafill_inplace(self, pts, n_pts, rule);

	if ( PImage(self)->type != imByte && PImage(self)->type != imRGB )
		return false;
	render.bpp = ( PImage(self)->type & imBPP ) / 8;

	render.fg  = ctx->color;
	render.bg  = ctx->backColor;
	if ( memcmp(ctx->pattern, fillPatterns[fpEmpty], sizeof(FillPattern)) == 0) {
		if ( !ctx->transparent )
			return true;
		render.fg = render.bg;
		pattern = false;
	} else
		pattern = memcmp(ctx->pattern, fillPatterns[fpSolid], sizeof(FillPattern)) != 0;
	dual_pattern = pattern ? !ctx->transparent : false;

	clip.left  = clip.bottom = 0;
	clip.right = PImage(self)->w - 1;
	clip.top   = PImage(self)->h - 1;
	switch (aafill_init( pts, n_pts, rule, clip, &aa)) {
		case -1: return false;
		case  0: return true ;
	}

	rop          = ctx->rop & ropPorterDuffMask;
	render.alpha = (ctx-> rop & ropSrcAlpha) ? (( ctx->rop >> ropSrcAlphaShift ) & 0xff ) : 255;
	if ( !img_find_blend_proc( rop, &render.blend1, &render.blend2)) {
		warn("img_aafill: blend not supported");
		goto EXIT;
	}
	render.x      = aa.x;
	render.dst    = PImage(self)->data + PImage(self)->lineSize * aa.y + aa.x * render.bpp;
	render.pixels = aa.maplen;
	render.bytes  = render.pixels * render.bpp;

	pipeline[n_renders++] = &obj_render_acquire_map;
	if ( render.alpha != 255 )
		pipeline[n_renders++] = &obj_render_apply_alpha;
	if ( !dual_pattern && pattern )
		pipeline[n_renders++] = &obj_render_apply_transparent_pattern;
	if ( render.bpp == 3 ) {
		pipeline[n_renders++] = &obj_render_map_to_rgb;
		if ( dual_pattern )
			pipeline[n_renders++] = &obj_render_opaque_rgb_pattern;
		else
			pipeline[n_renders++] = &obj_render_blend_solid_rgb;
	} else {
		if ( dual_pattern )
			pipeline[n_renders++] = &obj_render_opaque_gray_pattern;
		else
			pipeline[n_renders++] = &obj_render_blend_solid_gray;
	}

	if ( !render_init(pipeline, n_renders, ctx, &render))
		goto EXIT;

	while ( aafill_next_scanline( &aa, pipeline[0]->self)) {
		render.y = aa.y;
		render_run(pipeline, n_renders, ctx, &render);
		render.dst += PImage(self)->lineSize;
	}

	render_done(pipeline, n_renders);
	ok = true;

EXIT:
	aafill_done(&aa);
	return ok;
}

#ifdef __cplusplus
}
#endif
