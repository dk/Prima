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
#define SHIFT(a,b)	{ (a) += XX-> gtransform. x + XX-> btransform. x; \
									(b) += XX-> gtransform. y + XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)

/*
https://gitlab.freedesktop.org/xorg/lib/libxrender/-/issues/1:
tesselation in xrender is horribly broken, but for now I'll give it a go to at least not throw a coredump

source is from https://github.com/freedesktop/xorg-libXrender/blob/master/src/Poly.c, (c) Keith Packard

*/
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
			    int			    winding);

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
prima_init_xrender_subsystem(char * error_buf)
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
		Pdebug("selected visual 0x%x for ARGB operations\n", list[i].visualid);
		break;
	}
	if ( list) XFree( list);

	/* find compat format for putting regular pixmaps */
	if (!(guts. xrender_display_format = XRenderFindVisualFormat(DISP, guts.visual.visual))) {
		guts. xrender_argb32_format = NULL;
		guts. argb_visual. visual = NULL;
		guts. argb_depth = 0;
	}

	if ( guts. argb_visual. visual )
		guts. argbColormap = XCreateColormap( DISP, guts. root, guts. argb_visual. visual, AllocNone);
	else {
		Pdebug("no ARGB visual found\n");
		guts. render_extension = false;
		return true;
	}

	guts. xrender_a8_format = XRenderFindStandardFormat(DISP, PictStandardA8);
	guts. xrender_a1_format = XRenderFindStandardFormat(DISP, PictStandardA1);

	pen.pixmap      = XCreatePixmap( DISP, guts.root, 8, 8, guts.argb_depth);
	pen.gcv.graphics_exposures = false;
	pen.gc          = XCreateGC( DISP, pen.pixmap, GCGraphicsExposures, &pen.gcv);
	xrp_attr.repeat = RepeatNormal;
	pen.picture     = XRenderCreatePicture( DISP, pen.pixmap, guts.xrender_argb32_format, CPRepeat, &xrp_attr);
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
		xf = guts.xrender_argb32_format;
		break;
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
	int alpha = XX->paint_alpha, red, green, blue;

	switch ( XX-> paint_rop) {
	case ropNotPut:
		pen.gcv.foreground = ~XX-> fore.primary;
		pen.gcv.background = ~XX-> back.primary;
		break;
	case ropBlackness:
		pen.gcv.foreground = 0x000000;
		pen.gcv.background = 0x000000;
		break;
	case ropWhiteness:
		pen.gcv.foreground = 0xffffff;
		pen.gcv.background = 0xffffff;
		break;
	case ropNoOper:
		pen.gcv.foreground = 0x000000;
		pen.gcv.background = 0x000000;
		alpha = 0;
		break;
	default:
		pen.gcv.foreground = XX-> fore.primary;
		pen.gcv.background = XX-> back.primary;
	}

#define COMP(src,c) \
	c = ((float) src) / 255.0 * (float)alpha + .5; \
	c &= 0xff; \
	c = ((c << guts.argb_bits.c##_range) >> 8) << guts.argb_bits.c##_shift;

	COMP(COLOR_R(XX->fore.color),red);
	COMP(COLOR_G(XX->fore.color),green);
	COMP(COLOR_B(XX->fore.color),blue);
	pen.gcv.foreground = red | green | blue;

	COMP(COLOR_R(XX->back.color),red);
	COMP(COLOR_G(XX->back.color),green);
	COMP(COLOR_B(XX->back.color),blue);
	pen.gcv.background = red | green | blue;
#undef COMP

	alpha = ((alpha << guts.argb_bits.alpha_range) >> 8) << guts.argb_bits.alpha_shift;
	pen.gcv.foreground |= alpha;
	pen.gcv.background |= alpha;

	pen.gcv.ts_x_origin = XX-> fill_pattern_offset.x;
	pen.gcv.ts_y_origin = XX-> fill_pattern_offset.y;
	pen.gcv.stipple = prima_get_hatch( &XX-> fill_pattern);
	if ( pen.gcv.stipple ) {
		if ( XX-> paint_rop2 == ropNoOper )
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
	XRenderPictFormat *xf;
	XRenderPictureAttributes xrp_attr;
	xrp_attr.repeat = RepeatNormal;
	if ( XF_LAYERED(XX))
		xf = guts.xrender_argb32_format;
	else
		xf = guts.xrender_display_format;

	if ( PDrawable(self)-> fillPatternImage && !X(PDrawable(self)-> fillPatternImage)->type.icon ) {
		GC gc;
		XGCValues gcv;

		gcv.foreground = ((XX->paint_alpha << guts. argb_bits. alpha_range) >> 8) << guts. argb_bits. alpha_shift;
		if ( ( gc = XCreateGC(DISP, tile, GCForeground, &gcv))) {
			Point sz = get_pixmap_size( tile );
			XSetPlaneMask( DISP, gc, guts.argb_bits.alpha_mask);
			XFillRectangle( DISP, tile, gc, 0, 0, sz.x, sz.y);
			XFreeGC( DISP, gc);
			XFLUSH;
		}
	}

	XX-> fp_render_picture = XRenderCreatePicture( DISP, tile, xf, CPRepeat, &xrp_attr);
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
	if ( !( XX-> fp_render_pen = XCreatePixmap( DISP, guts.root, sz.x, sz.y, guts.argb_depth)))
		return;

	gcv.foreground = COLOR2DEV_RGBA( &guts.argb_bits, XX->fore.color, 255 );
	gcv.background = COLOR2DEV_RGBA( &guts.argb_bits, XX->back.color, ( XX->paint_rop2 == ropNoOper) ? 0 : 255 );
	if ( !( gc = XCreateGC(DISP, XX-> fp_render_pen, GCForeground|GCBackground, &gcv))) {
		XFreePixmap( DISP, XX-> fp_render_pen );
		XX-> fp_render_pen = 0;
		return;
	}

	XCopyPlane( DISP, stipple, XX-> fp_render_pen, gc, 0, 0, sz.x, sz.y, 0, 0, 1);

	XFreeGC(DISP, gc);

	xrp_attr.repeat = RepeatNormal;
	if ( !( XX-> fp_render_picture = XRenderCreatePicture( DISP, XX-> fp_render_pen, guts.xrender_argb32_format, CPRepeat, &xrp_attr))) {
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

static Picture
pen_picture( Handle self)
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
apc_gp_aa_bar( Handle self, double x1, double y1, double x2, double y2)
{
	XPointDouble p[5];
	int ok;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( XT_IS_BITMAP(XX)) {
		if ( XX->paint_alpha < 0x7f ) return true;
		if ( XX-> paint_rop2 == ropNoOper )
			XX-> flags.use_stipple_transparency = 1;
		ok = apc_gp_bar(self, x1 + .5, y1 + .5, x2 + .5, y2 + .5);
		XX-> flags.use_stipple_transparency = 0;
 		return ok;
	}

	x1 += XX-> gtransform. x + XX-> btransform. x;
	y1 = REVERT(y1 + XX-> gtransform. y + XX-> btransform. y) + 1;
	x2 += XX-> gtransform. x + XX-> btransform. x + 1;
	y2 = REVERT(y2 + XX-> gtransform. y + XX-> btransform. y);
	RANGE2(x2, y2);
	p[0].x = x1;
	p[0].y = y1;
	p[1].x = x2;
	p[1].y = y1;
	p[2].x = x2;
	p[2].y = y2;
	p[3].x = x1;
	p[3].y = y2;
	p[4].x = x1;
	p[4].y = y1;

	ok = my_XRenderCompositeDoublePoly(
		DISP, PictOpOver, pen_picture(self), XX->argb_picture,
		XX->flags.antialias ? guts.xrender_a8_format : guts.xrender_a1_format,
		0, 0, 0, 0, p, 5,
		EvenOddRule
	);

	XSync(DISP, false);
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
		if ( XX->paint_alpha < 0x7f ) return true;
		if ( !( p = malloc(( numPts + 1) * sizeof( Point)))) return false;
		for ( i = 0; i < numPts; i++) {
			p[i].x = points[i].x + .5;
			p[i].y = points[i].y + .5;
		}
		if ( XX-> paint_rop2 == ropNoOper )
			XX-> flags.use_stipple_transparency = 1;
		ok = apc_gp_fill_poly( self, numPts, p );
		XX-> flags.use_stipple_transparency = 0;
		free(p);
		return ok;
	}

	if ( !( p = malloc(( numPts + 1) * sizeof( XPointDouble)))) return false;

	for ( i = 0; i < numPts; i++) {
		p[i].x = points[i]. x + XX-> gtransform. x + XX-> btransform. x;
		p[i].y = REVERT(points[i]. y + XX-> gtransform. y + XX-> btransform. y);
		RANGE2(p[i].x, p[i].y);
	}
	p[numPts].x = points[0]. x + XX-> gtransform. x + XX-> btransform. x;
	p[numPts].y = REVERT(points[0]. y + XX-> gtransform. y + XX-> btransform. y);
	RANGE2(p[numPts].x, p[numPts].y);

	ok = my_XRenderCompositeDoublePoly(
		DISP, PictOpOver, pen_picture(self), XX->argb_picture,
		XX->flags.antialias ? guts.xrender_a8_format : guts.xrender_a1_format,
		0, 0, 0, 0, p, numPts,
		((XX->fill_mode & fmWinding) == fmAlternate) ? EvenOddRule : WindingRule
	);
	free( p);

	XSync(DISP, false);
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

typedef struct _Edge Edge;

struct _Edge {
    XLineFixed	edge;
    XFixed	current_x;
    Bool	clockWise;
    Edge	*next, *prev;
};

static int
CompareEdge (const void *o1, const void *o2)
{
    const Edge	*e1 = o1, *e2 = o2;

    return e1->edge.p1.y - e2->edge.p1.y;
}

static XFixed
XRenderComputeX (XLineFixed *line, XFixed y)
{
    XFixed  dx = line->p2.x - line->p1.x;
    double  ex = (double) (y - line->p1.y) * (double) dx;
    XFixed  dy = line->p2.y - line->p1.y;

    return (XFixed) line->p1.x + (XFixed) (ex / dy);
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
	    /*
	     * Find one later in the list that belongs before the
	     * current one
	     */
	    for (en = next; en; en = en->next)
	    {
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
		    break;
		}
	    }
	}
#if 0
	printf ("y: %6.3g:", y / 65536.0);
	for (e = active; e; e = e->next)
	{
	    printf (" %6.3g", e->current_x / 65536.0);
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
	       while (en && cw != 0) {
                   en = en->next;
	           cw += en->clockWise ? 1 : -1;
	       }
	       traps->right = en->edge;
	       if ( !en ) {
	          ok = 0;
		  break;
	       }
	    }
	    traps->right = en->edge;
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
    if (!edges)
	return 0;
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

Bool prima_init_xrender_subsystem(char * error_buf)                 { return true; }
void prima_done_xrender_subsystem(void)                             { }
Bool apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points) { return false; }
Bool apc_gp_aa_bar( Handle self, double x1, double y1, double x2, double y2) { return false; }

#endif

