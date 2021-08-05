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

Bool
prima_init_xrender_subsystem(char * error_buf)
{
	int i, count;
	XRenderPictFormat *f;
	XVisualInfo template, *list;
	XRenderPictureAttributes xrp_attr;

	if ( !guts. render_extension ) return true;

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
	else
		Pdebug("no ARGB visual found\n");

	guts. xrender_a8_format = XRenderFindStandardFormat(DISP, PictStandardA8);
	guts. xrender_a1_format = XRenderFindStandardFormat(DISP, PictStandardA1);

	pen.pixmap      = XCreatePixmap( DISP, guts.root, 8, 8, 32);
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

static void
pen_update(Handle self)
{
	DEFXX;
	int flags =
		GCTileStipXOrigin | GCTileStipYOrigin |
		GCForeground      | GCFillStyle       |
		0;
	int alpha = 0xff000000;

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

	pen.gcv.foreground &= 0xffffff;
	pen.gcv.background &= 0xffffff;
	pen.gcv.foreground |= alpha;
	pen.gcv.background |= alpha;

	pen.gcv.ts_x_origin = XX-> fill_pattern_offset.x;
	pen.gcv.ts_y_origin = XX-> fill_pattern_offset.y;
	if ( !XX-> flags.brush_null_hatch ) {
		pen.gcv.stipple = prima_get_hatch( &XX-> fill_pattern);
		if ( !pen.gcv.stipple ) goto SOLID;
		if ( XX-> paint_rop2 == ropNoOper )
			pen.gcv.background = 0x00000000;
		pen.gcv.fill_style = FillOpaqueStippled;
		flags |= GCStipple | GCBackground;
	} else {
	SOLID:
		pen.gcv.fill_style = FillSolid;
	}
	XChangeGC( DISP, pen.gc, flags, &pen.gcv);

	XFillRectangle( DISP, pen.pixmap, pen.gc, 0, 0, 8, 8);
	guts.xrender_pen_dirty = false;
}

Bool
apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points)
{
	Picture   target;
	XPointDouble *p;
	int i, ok;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( !( p = malloc(( numPts + 1) * sizeof( XPointDouble)))) return false;

	for ( i = 0; i < numPts; i++) {
		p[i].x = points[i]. x + XX-> gtransform. x + XX-> btransform. x;
		p[i].y = REVERT(points[i]. y + XX-> gtransform. y + XX-> btransform. y);
		RANGE2(p[i].x, p[i].y);
	}
	p[numPts].x = points[0]. x + XX-> gtransform. x + XX-> btransform. x;
	p[numPts].y = REVERT(points[0]. y + XX-> gtransform. y + XX-> btransform. y);
	RANGE2(p[numPts].x, p[numPts].y);

	target  = XRenderCreatePicture( DISP, XX->gdrawable, 
		XF_LAYERED(XX) ? guts.xrender_argb32_format : guts.xrender_display_format,
		0, NULL);
	if ( XX-> clip_mask_extent. x != 0 && XX-> clip_mask_extent. y != 0)
		XRenderSetPictureClipRegion(DISP, target, XX->current_region);
	XCHECKPOINT;

	if ( guts.xrender_pen_dirty ) pen_update(self);
	ok = my_XRenderCompositeDoublePoly(
		DISP, PictOpOver, pen.picture, target,
		guts.xrender_a8_format,
		0, 0, 0, 0, p, numPts,
		((XX->fill_mode & fmWinding) == fmAlternate) ? EvenOddRule : WindingRule
	);

	XRenderFreePicture( DISP, target);
	XSync(DISP, false);
	free( p);
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

    return XDoubleToFixed ((b2 - b1) / (m1 - m2));
}

static int
XRenderComputeTrapezoids (Edge		*edges,
			  int		nedges,
			  int		winding,
			  XTrapezoid	*traps,
			  int           maxtraps
			  )
{
    int		ntraps = 0;
    int		inactive;
    Edge	*active;
    Edge	*e, *en, *next;
    XFixed	y, next_y, intersect;

    qsort (edges, nedges, sizeof (Edge), CompareEdge);

    y = edges[0].edge.p1.y;
    active = NULL;
    inactive = 0;
    while (maxtraps > 0 && (active || inactive < nedges))
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
		if (intersect < next_y)
		    next_y = intersect;
	    }
	}
	/* check next inactive point */
	if (inactive < nedges && edges[inactive].edge.p1.y < next_y)
	    next_y = edges[inactive].edge.p1.y;

	/* walk the list generating trapezoids */
	for (e = active; e && (en = e->next); e = en->next)
	{
	    traps->top = y;
	    traps->bottom = next_y;
	    traps->left = e->edge;
	    traps->right = en->edge;
	    traps++;
	    ntraps++;
	    if ( --maxtraps <= 0 ) break;
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
    return ntraps;
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
    int		    i, nedges, ntraps;
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
    ntraps = XRenderComputeTrapezoids (edges, nedges, winding, traps, npoints * npoints + 1);
    /* XXX adjust xSrc/xDst */
    XRenderCompositeTrapezoids (dpy, op, src, dst, maskFormat, xSrc, ySrc, traps, ntraps);
    free (edges);
    return ntraps <= npoints * npoints;
}

#else

Bool prima_init_xrender_subsystem(char * error_buf)                 { return true; }
void prima_done_xrender_subsystem(void)                             { }
Bool apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points) { return false; }

#endif

