#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void PlotFunc( PImage src, PImage dst, int x, int y, int xFrom, int yFrom, int xLen, int yLen, void *blt);

typedef struct {
	PImage src, dst;
	PlotFunc *func;
	Rect r;
	void* blt;
} PlotStruct;

static Bool
plot_glyph( int x, int y, int w, int h, PlotStruct * ptr)
{
	int ox, oy;
	if (
		ptr->r.left   >= x + w ||
		ptr->r.right  < x      ||
		ptr->r.bottom >= y + h ||
		ptr->r.top    < y
	)
		return true;

	if ( ptr->r.right >= x + w )
		w += x - ptr->r.left + 1;
	else
		w = ptr->r.right - ptr->r.left + 1;
	if ( ptr->r.left <= x ) {
		ox = x - ptr->r.left;
		w -= ox;
	} else {
		ox = 0;
		x = ptr->r.left;
	}

	if ( ptr->r.top >= y + h )
		h += y - ptr->r.bottom + 1;
	else
		h = ptr->r.top - ptr->r.bottom + 1;
	if ( ptr->r.bottom <= y ) {
		oy = y - ptr->r.bottom;
		h -= oy;
	} else {
		oy = 0;
		y = ptr->r.bottom;
	}

	ptr->func( ptr->src, ptr->dst, x, y, ox, oy, w, h, ptr->blt);

	return true;
}

static void
plot_1bit( PImage src, PImage dst, int x, int y, int xFrom, int yFrom, int xLen, int yLen, void *blt)
{
	int i;
	Byte *s, *d;
	for (
		i = 0, s = src->data + yFrom * src->lineSize, d = dst->data + y * dst-> lineSize;
		i < yLen;
		i++, s += src->lineSize, d += dst->lineSize
	)
		bc_mono_put( s, xFrom, xLen, d, x, (BitBltProc*)blt);
}

void
img_plot_glyph( Handle self, PImage glyph, int x, int y, PImgPaintContext ctx)
{
	PImage i = (PImage) self;

	PlotStruct rec = {
		glyph, i, NULL,
		{ x, y, x + glyph->w - 1, y + glyph->h - 1 }
	};

	if (glyph->type != i->type )
		return;

	switch (i->type) {
	case imbpp1:
	case imBW:
		rec.func = plot_1bit;
		rec.blt  = img_find_blt_proc(ctx->rop);
		break;
	default:
		warn("Not implemented");
		return;
	}

	img_region_foreach( ctx->region,
		x, y, glyph->w, glyph->h,
		(RegionCallbackFunc*)plot_glyph, &rec
	);
}

#ifdef __cplusplus
}
#endif

