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
		guts. xrender_argb_pic_format = f;
		guts. argb_depth = f-> depth;
		Pdebug("selected visual 0x%x for ARGB operations\n", list[i].visualid);
		break;
	}
	if ( list) XFree( list);

	/* find compat format for putting regular pixmaps */
	if (!(guts. xrender_argb_compat_format = XRenderFindVisualFormat(DISP, guts.visual.visual))) {
		guts. xrender_argb_pic_format = NULL;
		guts. argb_visual. visual = NULL;
		guts. argb_depth = 0;
	}

	if ( guts. argb_visual. visual )
		guts. argbColormap = XCreateColormap( DISP, guts. root, guts. argb_visual. visual, AllocNone);
	else
		Pdebug("no ARGB visual found\n");

	guts. xrender_a8_format = XRenderFindStandardFormat(DISP, PictStandardA8);

	pen.pixmap      = XCreatePixmap( DISP, guts.root, 8, 8, 32);
	pen.gcv.graphics_exposures = false;
	pen.gc          = XCreateGC( DISP, pen.pixmap, GCGraphicsExposures, &pen.gcv);
	xrp_attr.repeat = RepeatNormal;
	pen.picture     = XRenderCreatePicture( DISP, pen.pixmap, guts.xrender_argb_pic_format, CPRepeat, &xrp_attr);
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
	int i;
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
		XF_LAYERED(XX) ? guts.xrender_argb_pic_format : guts.xrender_argb_compat_format,
		0, NULL);
	if ( XX-> clip_mask_extent. x != 0 && XX-> clip_mask_extent. y != 0)
		XRenderSetPictureClipRegion(DISP, target, XX->current_region);
	XCHECKPOINT;

	if ( guts.xrender_pen_dirty ) pen_update(self);
	XRenderCompositeDoublePoly(
		DISP, PictOpOver, pen.picture, target,
		guts.xrender_a8_format,
		0, 0, 0, 0, p, numPts,
		((XX->fill_mode & fmWinding) == fmAlternate) ? EvenOddRule : WindingRule
	);

	XRenderFreePicture( DISP, target);
	XSync(DISP, false);
	free( p);
	XCHECKPOINT;
	return true;
}

#else

Bool prima_init_xrender_subsystem(char * error_buf)                 { return true; }
void prima_done_xrender_subsystem(void)                             { }
Bool apc_gp_aa_fill_poly( Handle self, int numPts, NPoint * points) { return false; }

#endif

