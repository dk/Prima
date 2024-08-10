/*****************************************/
/*                                       */
/*  XRender-based graphics               */
/*                                       */
/*****************************************/

#include "unix/guts.h"
#include "Image.h"

#ifdef HAVE_X11_EXTENSIONS_XRENDER_H

#define SORT(a,b)	{ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)	(XX-> size. y - (a) - 1)
#define SHIFT(a,b)	{ (a) += XX-> btransform. x; (b) += XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)

/*
https://gitlab.freedesktop.org/xorg/lib/libxrender/-/issues/1:
tesselation in xrender is horribly broken, but for now I'll give it a go to at least not throw a coredump

source is from https://github.com/freedesktop/xorg-libXrender/blob/master/src/Poly.c, (c) Keith Packard

*/
typedef struct {
	Picture   picture;
	Pixmap    pixmap;
	GC        gc;
	XGCValues gcv;
} Pen;

static Pen pen;

#ifdef NEED_X11_EXTENSIONS_XRENDER_H
/* piece of Xrender guts */
typedef struct _XExtDisplayInfo {
	struct _XExtDisplayInfo *next;
	Display *display;
	XExtCodes *codes;
	XPointer data;
} XExtDisplayInfo;

extern XExtDisplayInfo *
XRenderFindDisplay (Display *dpy);
#endif


Bool
prima_init_xrender_subsystem(char * error_buf, Bool disable_argb32)
{
	int i, count;
	XRenderPictFormat *f;
	XVisualInfo template, *list;
	XRenderPictureAttributes xrp_attr;

	{
		int dummy;
		if ( XRenderQueryExtension( DISP, &dummy, &dummy))
			guts. render_extension = true;
	}

	if ( !guts. render_extension ) return true;

#ifdef NEED_X11_EXTENSIONS_XRENDER_H
	{ /* snatch error code from xrender guts */
		XExtDisplayInfo *info = XRenderFindDisplay( DISP);
		if ( info && info-> codes)
			guts. xft_xrender_major_opcode = info-> codes-> major_opcode;
	}
#endif

	template. depth = 32; /* XXX should try non-32 bit alpha'ed visuals */
	list = XGetVisualInfo( DISP, VisualDepthMask, &template, &count);
	for ( i = 0; i < count; i++) {
		if (!(f = XRenderFindVisualFormat(DISP, list[i].visual))) continue;
		if (f->direct.alphaMask == 0) continue;
		guts. argb_visual = list[i];
		guts. argb_bits. red_mask   = f->direct. redMask   << f->direct. red;
		guts. argb_bits. green_mask = f->direct. greenMask << f->direct. green;
		guts. argb_bits. blue_mask  = f->direct. blueMask  << f->direct. blue;
		guts. argb_bits. alpha_mask = f->direct. alphaMask << f->direct. alpha;
		prima_find_color_mask_range( guts. argb_bits. red_mask,   &guts. argb_bits. red_shift,   &guts. argb_bits. red_range);
		prima_find_color_mask_range( guts. argb_bits. green_mask, &guts. argb_bits. green_shift, &guts. argb_bits. green_range);
		prima_find_color_mask_range( guts. argb_bits. blue_mask,  &guts. argb_bits. blue_shift,  &guts. argb_bits. blue_range);
		prima_find_color_mask_range( guts. argb_bits. alpha_mask, &guts. argb_bits. alpha_shift, &guts. argb_bits. alpha_range);
		guts. xrender_argb32_format = f;
		guts. argb_depth = f-> depth;
		Pdebug("selected visual 0x%x for ARGB operations", list[i].visualid);
		break;
	}
	if ( list) XFree( list);

	/* find compat format for putting regular pixmaps */
	if (
		!(guts. xrender_display_format = XRenderFindVisualFormat(DISP, guts.visual.visual))
	) {
		guts. xrender_argb32_format = NULL;
		guts. argb_visual. visual = NULL;
		guts. argb_depth = 0;
		return true;
	}

	if ( disable_argb32 ) {
		guts. argb_visual. visual = NULL;
	}

	if ( guts. argb_visual. visual ) {
		guts. argbColormap = XCreateColormap( DISP, guts. root, guts. argb_visual. visual, AllocNone);
		guts. render_supports_argb32 = true;
	} else {
		Pdebug("no ARGB visual found");
	}

	if ( !( guts. xrender_a8_format = XRenderFindStandardFormat(DISP, PictStandardA8))) {
		Pdebug("no A8 visual found");
		guts. render_extension = false;
		return false;
	}
	if ( !( guts. xrender_a1_format = XRenderFindStandardFormat(DISP, PictStandardA1))) {
		Pdebug("no A1 visual found");
		guts. render_extension = false;
		return false;
	}

	pen.pixmap      = XCreatePixmap( DISP, guts.root, 8, 8,
		guts.render_supports_argb32 ? guts.argb_depth : guts.depth);
	pen.gc          = XCreateGC( DISP, pen.pixmap, GCGraphicsExposures, &pen.gcv);
	xrp_attr.repeat = RepeatNormal;
	pen.picture     = XRenderCreatePicture( DISP, pen.pixmap,
		guts.render_supports_argb32 ? guts.xrender_argb32_format : guts.xrender_display_format,
		CPRepeat, &xrp_attr);
	pen.gcv.graphics_exposures = false;
	guts.xrender_pen_dirty = true;

	return true;
}

void
prima_done_xrender_subsystem()
{
	if ( !guts. render_extension ) return;

	if ( guts. argbColormap )
		XFreeColormap( DISP, guts. argbColormap );
	XRenderFreePicture( DISP, pen.picture);
	XFreePixmap( DISP, pen.pixmap );
	XFreeGC( DISP, pen.gc);
	XCHECKPOINT;
}

Picture
prima_render_create_picture(XDrawable drawable, int depth)
{
	XRenderPictFormat *xf;
	switch (depth) {
	case 1:
		xf = guts.xrender_a1_format;
		break;
	case 32:
		if ( guts. render_supports_argb32) {
			xf = guts.xrender_argb32_format;
			break;
		}
	default:
		xf = guts.xrender_display_format;
	}
	return XRenderCreatePicture( DISP, drawable, xf, 0, NULL);
}

static void
pen_update(Handle self)
{
	DEFXX;
	int flags =
		GCTileStipXOrigin | GCTileStipYOrigin |
		GCForeground      | GCFillStyle       |
		0;
	int alpha = XX->alpha, red, green, blue;
	Color fore, back;

	switch ( XX-> rop) {
	case ropNotPut:
		fore = ~XX->fore.color;
		back = ~XX->back.color;
		break;
	case ropBlackness:
		fore = back = 0x000000;
		break;
	case ropWhiteness:
		fore = back = 0xffffff;
		break;
	case ropNoOper:
		fore = back = 0x000000;
		alpha = 0;
		break;
	default:
		fore = XX->fore.color;
		back = XX->back.color;
	}

#define COMP(src,c) \
	if ( guts.render_supports_argb32) { \
		c = ((float) src) / 255.0 * (float)alpha + .5; \
		c &= 0xff; \
		c = ((c << guts.argb_bits.c##_range) >> 8) << guts.argb_bits.c##_shift; \
	} else \
		c = ((src << guts.screen_bits.c##_range) >> 8) << guts.screen_bits.c##_shift; \

	COMP(COLOR_R(fore),red);
	COMP(COLOR_G(fore),green);
	COMP(COLOR_B(fore),blue);
	pen.gcv.foreground = red | green | blue;

	COMP(COLOR_R(back),red);
	COMP(COLOR_G(back),green);
	COMP(COLOR_B(back),blue);
	pen.gcv.background = red | green | blue;
#undef COMP

	if (guts.render_supports_argb32) {
		alpha = ((alpha << guts.argb_bits.alpha_range) >> 8) << guts.argb_bits.alpha_shift;
		pen.gcv.foreground |= alpha;
		pen.gcv.background |= alpha;
	}

	prima_get_fill_pattern_offsets(self, &pen.gcv.ts_x_origin, &pen.gcv.ts_y_origin);
	pen.gcv.stipple = prima_get_hatch( &XX-> fill_pattern);
	if ( pen.gcv.stipple ) {
		if ( XX-> rop2 == ropNoOper )
			pen.gcv.background = 0x00000000;
		pen.gcv.fill_style = FillOpaqueStippled;
		flags |= GCStipple | GCBackground;
	} else
		pen.gcv.fill_style = FillSolid;
	XChangeGC( DISP, pen.gc, flags, &pen.gcv);

	XFillRectangle( DISP, pen.pixmap, pen.gc, 0, 0, 8, 8);
	guts.xrender_pen_dirty = false;
}

static Point
get_pixmap_size( Pixmap px )
{
	XWindow _w;
	int _i;
	unsigned int w, h, _u;
	Point ret;
	XGetGeometry( DISP, px, &_w, &_i, &_i, &w, &h, &_u, &_u);
	ret.x = w;
	ret.y = h;
	return ret;
}

static void
pen_create_tile(Handle self, Pixmap tile)
{
	DEFXX;
	XRenderPictureAttributes xrp_attr;
	xrp_attr.repeat = RepeatNormal;

	XX-> fp_render_picture = XRenderCreatePicture(
		DISP, tile,
		guts.render_supports_argb32 ? guts.xrender_argb32_format : guts.xrender_display_format,
		CPRepeat, &xrp_attr
	);
}

static void
pen_create_stipple(Handle self, Pixmap stipple)
{
	DEFXX;
	XRenderPictureAttributes xrp_attr;
	GC gc;
	XGCValues gcv;
	Point sz;

	sz = get_pixmap_size( stipple );

	if ( guts. render_supports_argb32 ) {
		if ( !( XX-> fp_render_pen = XCreatePixmap( DISP, guts.root, sz.x, sz.y, guts.argb_depth)))
			return;
		gcv.foreground = DEV_RGBA(&guts.argb_bits,
			COLOR_R(XX->fore.color) * XX->alpha / 255,
			COLOR_G(XX->fore.color) * XX->alpha / 255,
			COLOR_B(XX->fore.color) * XX->alpha / 255,
			XX->alpha);
		gcv.background = DEV_RGBA(&guts.argb_bits,
			COLOR_R(XX->back.color) * XX->alpha / 255,
			COLOR_G(XX->back.color) * XX->alpha / 255,
			COLOR_B(XX->back.color) * XX->alpha / 255,
			(XX->rop2 == ropNoOper) ? 0 : XX->alpha);
	} else {
		if ( !( XX-> fp_render_pen = XCreatePixmap( DISP, guts.root, sz.x, sz.y, guts.depth)))
			return;
		gcv.foreground = DEV_RGB(&guts.screen_bits,
			COLOR_R(XX->fore.color),
			COLOR_G(XX->fore.color),
			COLOR_B(XX->fore.color)
		);
		gcv.background = DEV_RGB(&guts.screen_bits,
			COLOR_R(XX->back.color),
			COLOR_G(XX->back.color),
			COLOR_B(XX->back.color)
		);
	}

	if ( !( gc = XCreateGC(DISP, XX-> fp_render_pen, GCForeground|GCBackground, &gcv))) {
		XFreePixmap( DISP, XX-> fp_render_pen );
		XX-> fp_render_pen = 0;
		return;
	}

	XCopyPlane( DISP, stipple, XX-> fp_render_pen, gc, 0, 0, sz.x, sz.y, 0, 0, 1);
	XFreeGC(DISP, gc);

	xrp_attr.repeat = RepeatNormal;
	if ( !( XX-> fp_render_picture = XRenderCreatePicture( DISP, XX-> fp_render_pen,
		guts.render_supports_argb32 ? guts.xrender_argb32_format : guts.xrender_display_format,
		CPRepeat, &xrp_attr
	))) {
		XFreePixmap(DISP, XX-> fp_render_pen);
		XX-> fp_render_pen = 0;
		return;
	}
}

void
prima_render_cleanup_stipples(Handle self)
{
	DEFXX;
	if ( XX-> fp_render_picture ) {
		XRenderFreePicture( DISP, XX-> fp_render_picture);
		XX-> fp_render_picture = 0;
	}
	if ( XX-> fp_render_pen ) {
		XFreePixmap(DISP, XX-> fp_render_pen);
		XX-> fp_render_pen = 0;
	}
}

Picture
prima_pen_picture( Handle self)
{
	DEFXX;
	if ( guts.xrender_pen_dirty ) {
		prima_render_cleanup_stipples(self);
		if ( XX-> fp_stipple || XX-> fp_tile ) {
			if ( XX-> fp_tile )
				pen_create_tile(self, XX-> fp_tile);
			else
				pen_create_stipple(self, XX-> fp_stipple);
			guts.xrender_pen_dirty = false;
		} else
			pen_update(self);
	}
	return XX-> fp_render_picture ? XX-> fp_render_picture : pen.picture;
}

Bool
apc_gp_aa_bars( Handle self, int nr, NRect *rr)
{
	DEFXX;
	int ok = true, i;
	XTrapezoid *xt0, *xt1;
	XRectangle *xr0, *xr1;
	int nt = 0, nx = 0;
	Byte *arena;
	Bool may_use_rects, may_use_traps;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( XT_IS_BITMAP(XX)) {
		if ( XX->alpha < 0x7f ) return true;

		if ( nr == 1 ) {
			Rect r = {rr[0].left + .5, rr[0].bottom + .5, rr[0].right + .5, rr[0].top + .5};
			return apc_gp_bars(self, 1, &r);
		} else {
			Rect *r;
			if (( r = prima_array_convert( nr * 4, rr, 'd', NULL, 'i')))
				return false;
			ok = apc_gp_bars(self, nr, r);
			free(r);
			return ok;
		}
	}

	may_use_rects = !(
		XX-> fp_stipple ||
		XX-> fp_tile    ||
		prima_get_hatch( &XX-> fill_pattern)
	);
	may_use_traps = XX->flags.antialias || !may_use_rects;

	if ( !( arena = malloc(
		(
			(may_use_traps ? sizeof(XTrapezoid) : 0) +
			(may_use_rects ? sizeof(XRectangle) : 0)
		) * nr
	)))
		return false;
	xt0 = (XTrapezoid*) arena;
	xr0 = (XRectangle*) (arena + (may_use_traps ? sizeof(XTrapezoid) : 0) * nr);

	for (i = 0, xt1 = xt0, xr1 = xr0; i < nr; i++, rr++) {
		double x1, y1, x2, y2;
		int    ix1, iy1, ix2, iy2;
		x1 = rr->left  + XX-> btransform.x;
		y1 = REVERT(rr->bottom + XX-> btransform. y) + 1;
		x2 = rr->right + XX-> btransform.x + 1;
		y2 = REVERT(rr->top + XX-> btransform. y);
		RANGE2(x1, y1);
		RANGE2(x2, y2);
		ix1 = floor( x1 + .5 );
		iy1 = floor( y1 + .5 );
		ix2 = floor( x2 + .5 );
		iy2 = floor( y2 + .5 );
		if (
			may_use_rects && (
				!may_use_traps || (
					(double) ix1 == x1 &&
					(double) iy1 == y1 &&
					(double) ix2 == x2 &&
					(double) iy2 == y2
				)
			)
		) {
			xr1->x = x1;
			xr1->y = y2;
			xr1->width = x2 - x1;
			xr1->height = y1 - y2;
			xr1++;
			nx++;
		} else {
			xt1->left.p1.x  = xt1->left.p2.x  = XDoubleToFixed(x1);
			xt1->right.p1.x = xt1->right.p2.x = XDoubleToFixed(x2);
			xt1->top        = xt1->left.p2.y  = xt1->right.p2.y = XDoubleToFixed(y2);
			xt1->bottom     = xt1->left.p1.y  = xt1->right.p1.y = XDoubleToFixed(y1);
			xt1++;
			nt++;
		}
	}

	if ( nt > 0 ) {
		Picture pen = prima_pen_picture(self);
		XRenderPictFormat *format = XX->flags.antialias ? guts.xrender_a8_format : guts.xrender_a1_format;
		XRenderCompositeTrapezoids(DISP, PictOpOver, pen, XX->argb_picture, format, 0, 0, xt0, nt);
	}

	if ( nx > 0 && XX->rop != ropNoOper ) {
		Color c;
		XRenderColor xc;
		switch ( XX-> rop) {
		case ropNotPut:
			c = ~XX->fore.color;
			break;
		case ropBlackness:
			c = 0x000000;
		case ropWhiteness:
			c = 0xffffff;
		default:
			c = XX->fore.color;
		}
		xc.red   = COLOR_R16(c) * XX->alpha / 255;
		xc.green = COLOR_G16(c) * XX->alpha / 255;
		xc.blue  = COLOR_B16(c) * XX->alpha / 255;
		xc.alpha = XX->alpha << 8;
		XRenderFillRectangles(DISP, PictOpOver, XX->argb_picture, &xc, xr0, nx);
	}

	free(arena);

	XRENDER_SYNC_NEEDED;
	XCHECKPOINT;
	return ok;
}

Bool
apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points)
{
	XPointDouble *p;
	int i, ok;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( XT_IS_BITMAP(XX)) {
		Point *p;
		if ( XX->alpha < 0x7f ) return true;
		if ( !( p = prima_array_convert( numPts * 2, points, 'd', NULL, 'i')))
			return false;
		ok = apc_gp_fill_poly( self, numPts, p );
		free(p);
		return ok;
	}

	if ( !( p = malloc(( numPts + 1) * sizeof( XPointDouble)))) return false;

	for ( i = 0; i < numPts; i++) {
		p[i].x = points[i]. x + XX-> btransform. x;
		p[i].y = REVERT(points[i]. y + XX-> btransform. y - 1.0);
		RANGE2(p[i].x, p[i].y);
	}
	p[numPts].x = points[0]. x + XX-> btransform. x;
	p[numPts].y = REVERT(points[0]. y + XX-> btransform. y - 1.0);
	RANGE2(p[numPts].x, p[numPts].y);

	ok = my_XRenderCompositeDoublePoly(
		DISP, PictOpOver, prima_pen_picture(self), XX->argb_picture,
		XX->flags.antialias ? guts.xrender_a8_format : guts.xrender_a1_format,
		0, 0, 0, 0, p, numPts,
		((XX->fill_mode & fmWinding) == fmAlternate) ? EvenOddRule : WindingRule
	);
	free( p);

	XRENDER_SYNC_NEEDED;
	XCHECKPOINT;
	return ok;
}

/*
 *
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

//#define EDGE_DEBUG 1

typedef struct _Edge Edge;

struct _Edge {
    XLineFixed	edge;
    double	current_x;
    Bool	clockWise;
    Edge	*next, *prev;
};

static int
CompareEdge (const void *o1, const void *o2)
{
    const Edge	*e1 = o1, *e2 = o2;

    return e1->edge.p1.y - e2->edge.p1.y;
}

static double
XRenderComputeX (XLineFixed *line, XFixed y)
{
    double  dx = XFixedToDouble(line->p2.x - line->p1.x);
    double  ex = XFixedToDouble(y - line->p1.y) * (double) dx;
    double  dy = XFixedToDouble(line->p2.y - line->p1.y);
#if EDGE_DEBUG
    printf("f(%g:%g - %g:%g, %g) = %g\n",
    	line->p1.x/65536.0,
    	line->p1.y/65536.0,
    	line->p2.x/65536.0,
    	line->p2.y/65536.0,
	y/65536.0,
	XFixedToDouble(line->p1.x) + ex / dy
    );
#endif
    return XFixedToDouble(line->p1.x) + ex / dy;
}

static double
XRenderComputeInverseSlope (XLineFixed *l)
{
    return (XFixedToDouble (l->p2.x - l->p1.x) /
	    XFixedToDouble (l->p2.y - l->p1.y));
}

static double
XRenderComputeXIntercept (XLineFixed *l, double inverse_slope)
{
    return XFixedToDouble (l->p1.x) - inverse_slope * XFixedToDouble (l->p1.y);
}

static XFixed
XRenderComputeIntersect (XLineFixed *l1, XLineFixed *l2)
{
    /*
     * x = m1y + b1
     * x = m2y + b2
     * m1y + b1 = m2y + b2
     * y * (m1 - m2) = b2 - b1
     * y = (b2 - b1) / (m1 - m2)
     */
    double  m1 = XRenderComputeInverseSlope (l1);
    double  b1 = XRenderComputeXIntercept (l1, m1);
    double  m2 = XRenderComputeInverseSlope (l2);
    double  b2 = XRenderComputeXIntercept (l2, m2);
    if ( m1 == m2 ) return XDoubleToFixed(32766); /* lines do not intersect */

    return XDoubleToFixed ((b2 - b1) / (m1 - m2));
}

static int
XRenderComputeTrapezoids (Edge		*edges,
			  int		nedges,
			  int		winding,
			  XTrapezoid	*traps,
			  int           maxtraps,
			  int           *ntraps
			  )
{
    int		inactive, ok = 1;
    Edge	*active;
    Edge	*e, *en, *next;
    XFixed	y, next_y, intersect;

    *ntraps = 0;
    qsort (edges, nedges, sizeof (Edge), CompareEdge);
#if EDGE_DEBUG
    {
    	int i;
	for ( i = 0; i < nedges; i++) {
	Edge *e = edges + i;
		printf("edge %d:%d %-6.2g:%-6.2g - %-6.2g:%-6.2g\n", i, e->clockWise,
		e->edge.p1.x / 65536.0,
		e->edge.p1.y / 65536.0,
		e->edge.p2.x / 65536.0,
		e->edge.p2.y / 65536.0
		);
	}
    }
#endif

    y = edges[0].edge.p1.y;
    active = NULL;
    inactive = 0;
    while (ok && (active || inactive < nedges))
    {
	/* insert new active edges into list */
	while (inactive < nedges)
	{
	    e = &edges[inactive];
	    if (e->edge.p1.y > y)
		break;
	    /* move this edge into the active list */
	    inactive++;
	    e->next = active;
	    e->prev = NULL;
	    if (active)
		active->prev = e;
	    active = e;
	}

	/* compute x coordinates along this group */
	for (e = active; e; e = e->next)
	    e->current_x = XRenderComputeX (&e->edge, y);

	/* sort active list */
	for (e = active; e; e = next)
	{
	    next = e->next;
#if EDGE_DEBUG
	    if ( !next ) continue;
	    printf("cmp %d vs %d", e - edges+1, e->next - edges+1);
#endif
	    /*
	     * Find one later in the list that belongs before the
	     * current one
	     */
	    for (en = next; en; en = en->next)
	    {
#if EDGE_DEBUG
	        printf(" (vs %d r%f %f)", en-edges+1,
			en->current_x, e->current_x
		);
#endif
		if (en->current_x < e->current_x ||
		    (en->current_x == e->current_x &&
		     en->edge.p2.x < e->edge.p2.x))
		{
		    /*
		     * insert en before e
		     *
		     * extract en
		     */
		    en->prev->next = en->next;
		    if (en->next)
			en->next->prev = en->prev;
		    /*
		     * insert en
		     */
		    if (e->prev)
			e->prev->next = en;
		    else
			active = en;
		    en->prev = e->prev;
		    e->prev = en;
		    en->next = e;
		    /*
		     * start over at en
		     */
		    next = en;
#if EDGE_DEBUG
		    printf("- %d is before %d ", 1+en-edges, 1+e-edges);
#endif
		    break;
		}
	    }
#if EDGE_DEBUG
	    printf("\n");
#endif
	}
#if EDGE_DEBUG
	printf ("y: %6.3g => ",  y / 65536.0);
	for (e = active; e; e = e->next)
	{
	    printf (" %d:%6.3g/%6.3g", 1+e-edges, e->current_x, e-> edge.p2.x / 65536.0);
	}
	printf ("\n");
#endif
	/* find next inflection point */
	next_y = active->edge.p2.y;
	for (e = active; e; e = en)
	{
	    if (e->edge.p2.y < next_y)
		next_y = e->edge.p2.y;
	    en = e->next;
	    /* check intersect */
	    if (en && e->edge.p2.x > en->edge.p2.x)
	    {
		intersect = XRenderComputeIntersect (&e->edge, &e->next->edge);
		/* make sure this point is below the actual intersection */
		intersect = intersect + 1;
		if (intersect < next_y && intersect > y)
		    next_y = intersect;
	    }
	}
	/* check next inactive point */
	if (inactive < nedges && edges[inactive].edge.p1.y < next_y)
	    next_y = edges[inactive].edge.p1.y;

	if ( y == next_y ) {
	    /* emergency brake #1 */
	    ok = 0;
	    break;
	}

	/* walk the list generating trapezoids */
	for (e = active; e && (en = e->next); e = en->next)
	{
	    traps->top = y;
	    traps->bottom = next_y;
	    traps->left = e->edge;
	    if ( winding == WindingRule ) {
	       int cw = (e->clockWise ? 1 : -1) + (en->clockWise ? 1 : -1);
#if EDGE_DEBUG
	       printf("* %d:%d %d:%d\n", e->clockWise, 1+e-edges, en->clockWise, 1+en-edges);
#endif
	       while (en && cw != 0) {
                   en = en->next;
#if EDGE_DEBUG
	           printf("+ %d:%d\n", e, en->clockWise, 1+en-edges);
#endif
	           cw += en->clockWise ? 1 : -1;
	       }
	       traps->right = en->edge;
	       if ( !en ) {
	          ok = 0;
		  break;
	       }
	    }
	    traps->right = en->edge;
#if EDGE_DEBUG
	    printf("%strap %g:%d - %g:%d\n", (winding == WindingRule) ? "*" : "", y/65536.0, 1+e-edges, next_y/65536.0, 1+en-edges);
#endif
	    traps++;
	    (*ntraps)++;
	    if ( --maxtraps <= 0 ) {
	        /* emergency brake #2 */
	        ok = 0;
	    	break;
	    }
	}

	y = next_y;

	/* delete inactive edges from list */
	for (e = active; e; e = next)
	{
	    next = e->next;
	    if (e->edge.p2.y <= y)
	    {
		if (e->prev)
		    e->prev->next = e->next;
		else
		    active = e->next;
		if (e->next)
		    e->next->prev = e->prev;
	    }
	}
    }
    return ok;
}

int
my_XRenderCompositeDoublePoly (Display		    *dpy,
			    int			    op,
			    Picture		    src,
			    Picture		    dst,
			    _Xconst XRenderPictFormat	*maskFormat,
			    int			    xSrc,
			    int			    ySrc,
			    int			    xDst,
			    int			    yDst,
			    _Xconst XPointDouble    *fpoints,
			    int			    npoints,
			    int			    winding)
{
    Edge	    *edges;
    XTrapezoid	    *traps;
    int		    i, ok = 0, nedges, ntraps;
    XFixed	    x, y, prevx = 0, prevy = 0, firstx = 0, firsty = 0;
    XFixed	    top = 0, bottom = 0;	/* GCCism */

    edges = (Edge *) malloc (npoints * sizeof (Edge) +
			      ((npoints * npoints + 1) * sizeof (XTrapezoid)));
    if (!edges) {
        warn("XRenderCompositeDoublePoly: not enough memory (%d vertices, %ld bytes)\n",
		npoints, npoints * sizeof (Edge) + ((npoints * npoints + 1) * sizeof (XTrapezoid)));
	return 0;
    }
    traps = (XTrapezoid *) (edges + npoints);
    nedges = 0;
    for (i = 0; i <= npoints; i++)
    {
	if (i == npoints)
	{
	    x = firstx;
	    y = firsty;
	}
	else
	{
	    x = XDoubleToFixed (fpoints[i].x);
	    y = XDoubleToFixed (fpoints[i].y);
	}
	if (i)
	{
	    if (y < top)
		top = y;
	    else if (y > bottom)
		bottom = y;
	    if (prevy < y)
	    {
		edges[nedges].edge.p1.x = prevx;
		edges[nedges].edge.p1.y = prevy;
		edges[nedges].edge.p2.x = x;
		edges[nedges].edge.p2.y = y;
		edges[nedges].clockWise = True;
		nedges++;
	    }
	    else if (prevy > y)
	    {
		edges[nedges].edge.p1.x = x;
		edges[nedges].edge.p1.y = y;
		edges[nedges].edge.p2.x = prevx;
		edges[nedges].edge.p2.y = prevy;
		edges[nedges].clockWise = False;
		nedges++;
	    }
	    /* drop horizontal edges */
	}
	else
	{
	    top = y;
	    bottom = y;
	    firstx = x;
	    firsty = y;
	}
	prevx = x;
	prevy = y;
    }

    ok = XRenderComputeTrapezoids (edges, nedges, winding, traps, npoints * npoints + 1, &ntraps);
    /* XXX adjust xSrc/xDst */
    if ( ok )
       XRenderCompositeTrapezoids (dpy, op, src, dst, maskFormat, xSrc, ySrc, traps, ntraps);
    free (edges);
    return ok;
}

#else

Bool prima_init_xrender_subsystem(char * error_buf, Bool disable_argb32) { return true; }
void prima_done_xrender_subsystem(void)                             { }
Bool apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points) { return false; }
Bool apc_gp_aa_bars( Handle self, int nr, NRect *rr)                { return false; }

#endif

