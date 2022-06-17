/***********************************************************/
/*                                                         */
/*  System dependent graphics (unix, x11)                  */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Image.h"

#define SORT(a,b)	{ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)	(XX-> size. y - (a) - 1)
#define SHIFT(a,b)	{ (a) += XX-> gtransform. x + XX-> btransform. x; \
									(b) += XX-> gtransform. y + XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)
#define REVERSE_BYTES_32(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))
#define REVERSE_BYTES_24(x) ((((x)&0xff)<<16) | ((x)&0xff00) | (((x)&0xff0000)>>8))
#define REVERSE_BYTES_16(x) ((((x)&0xff)<<8 ) | (((x)&0xff00)>>8))

#define COLOR_R16(x) (((x)>>8)&0xFF00)
#define COLOR_G16(x) ((x)&0xFF00)
#define COLOR_B16(x) (((x)<<8)&0xFF00)

static int rop_map[] = {
	GXcopy	/* ropCopyPut */,		/* dest  = src */
	GXxor	/* ropXorPut */,		/* dest ^= src */
	GXand	/* ropAndPut */,		/* dest &= src */
	GXor		/* ropOrPut */,			/* dest |= src */
	GXcopyInverted /* ropNotPut */,		/* dest = !src */
	GXinvert	/* ropInvert */,		/* dest = !dest */
	GXclear	/* ropBlackness */,		/* dest = 0 */
	GXandReverse	/* ropNotDestAnd */,		/* dest = (!dest) & src */
	GXorReverse	/* ropNotDestOr */,		/* dest = (!dest) | src */
	GXset	/* ropWhiteness */,		/* dest = 1 */
	GXandInverted /* ropNotSrcAnd */,		/* dest &= !src */
	GXorInverted	/* ropNotSrcOr */,		/* dest |= !src */
	GXequiv	/* ropNotXor */,		/* dest ^= !src */
	GXnand	/* ropNotAnd */,		/* dest = !(src & dest) */
	GXnor	/* ropNotOr */,			/* dest = !(src | dest) */
	GXnoop	/* ropNoOper */			/* dest = dest */
};

int
prima_rop_map( int rop)
{
	if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
		return GXnoop;
	else
		return rop_map[ rop];
}

struct gc_head*
prima_get_gc( PDrawableSysData selfxx)
{
	XGCValues gcv;
	Bool bitmap, layered;
	struct gc_head *gc_pool;

	if ( XX-> gc && XX-> gcl) return NULL;

	if ( XX-> gc || XX-> gcl) {
		warn( "UAG_010: internal error");
		return NULL;
	}

	bitmap = XT_IS_BITMAP(XX) ? true : false;
	layered = XF_LAYERED(XX) ? true : false;
	gc_pool = bitmap ? &guts.bitmap_gc_pool : ( layered ? &guts.argb_gc_pool : &guts.screen_gc_pool );
	XX->gcl = TAILQ_FIRST(gc_pool);
	if (XX->gcl)
		TAILQ_REMOVE(gc_pool, XX->gcl, gc_link);
	if (!XX->gcl) {
		gcv. graphics_exposures = false;
		XX-> gc = XCreateGC( DISP, (bitmap || layered) ? XX-> gdrawable : guts. root, GCGraphicsExposures, &gcv);
		XCHECKPOINT;
		if (( XX->gcl = alloc1z( GCList)))
			XX->gcl->gc = XX-> gc;
	}
	if ( XX-> gcl) XX->gc = XX->gcl->gc;
	return gc_pool;
}

void
prima_release_gc( PDrawableSysData selfxx)
{
	Bool bitmap, layered;
	struct gc_head *gc_pool;

	if ( XX-> gc) {
		if ( XX-> gcl == NULL)
			warn( "UAG_011: internal error");
		bitmap = XT_IS_BITMAP(XX) ? true : false;
		layered = XF_LAYERED(XX) ? true : false;
		gc_pool = bitmap ? &guts.bitmap_gc_pool : ( layered ? &guts.argb_gc_pool : &guts.screen_gc_pool );
		if ( XX-> gcl)
			TAILQ_INSERT_HEAD(gc_pool, XX->gcl, gc_link);
		XX->gcl = NULL;
		XX->gc = NULL;
	} else {
		if ( XX-> gcl) {
			warn( "UAG_012: internal error");
			return;
		}
	}
}

/*
	macros to multiply line pattern entries to line width in
	a more-less aestethic fashion :-)
*/
#define MAX_DASH_LEN 2048
#define dDASH_FIX(line_width,source,length) \
	int df_i, df_lw = line_width + .5, df_len = length; \
	char df_list[MAX_DASH_LEN], *df_src = (char*)source, *df_dst = df_list
#define DASH_FIX \
	if ( df_lw > 1) {\
		int on = 0;\
		if ( df_len > MAX_DASH_LEN) df_len = MAX_DASH_LEN;\
		for ( df_i = 0; df_i < df_len; df_i++) {\
			unsigned int w = *((unsigned char*)df_src++);\
			if (( on = !on)) {\
				if ( w > 1) w *= df_lw;\
			} else {\
				w = w * df_lw + 1;\
			}\
			if ( w > 255) w = 255;\
			*((unsigned char*)df_dst++) = w;\
		}\
		df_src = df_list;\
	}
#define DASHES df_src, df_len

void
prima_prepare_drawable_for_painting( Handle self, Bool inside_on_paint)
{
	DEFXX;
	unsigned long mask = VIRGIN_GC_MASK;
	int w, h;
	XRectangle r;

	XF_IN_PAINT(XX) = true;
	XX-> btransform. x = XX-> btransform. y = 0;
	XX-> gcv. ts_x_origin = XX-> gcv. ts_y_origin = 0;
	if ( inside_on_paint && XX-> udrawable && is_opt( optBuffered) && !is_opt( optInDrawInfo) ) {
		if ( XX-> invalid_region) {
			XClipBox( XX-> invalid_region, &r);
			XX-> bsize. x = w = r. width;
			XX-> bsize. y = h = r. height;
			XX-> btransform. x = - r. x;
			XX-> btransform. y = r. y;
		} else {
			r. x = r. y = 0;
			XX-> bsize. x = w = XX-> size. x;
			XX-> bsize. y = h = XX-> size. y;
		}
		if ( w <= 0 || h <= 0) goto Unbuffered;
		XX-> gdrawable = XCreatePixmap( DISP, XX-> udrawable, w, h, XX->visual->depth);
		XCHECKPOINT;
		if (!XX-> gdrawable) goto Unbuffered;
		XX-> gcv. ts_x_origin = -r.x;
		XX-> gcv. ts_y_origin = -r.y;
	} else if ( XX-> udrawable && !XX-> gdrawable) {
Unbuffered:
		XX-> gdrawable = XX-> udrawable;
	}

	XX-> paint_rop = XX-> rop;
	XX-> paint_rop2 = XX-> rop2;
	XX-> paint_alpha = XX-> alpha;
	XX-> paint_line_width = XX-> line_width;
	XX-> flags. paint_base_line = XX-> flags. base_line;
	XX-> flags. paint_opaque    = XX-> flags. opaque;
	XX-> saved_font = PDrawable( self)-> font;
	XX-> gcv. clip_mask = None;
	XX-> gtransform = XX-> transform;
	XX-> fill_mode = XX->saved_fill_mode;

	prima_get_gc( XX);
	XX-> gcv. subwindow_mode = (XX->flags.clip_by_children ? ClipByChildren : IncludeInferiors);

	XChangeGC( DISP, XX-> gc, mask, &XX-> gcv);
	XCHECKPOINT;

	if ( XX-> dashes) {
		dDASH_FIX( XX-> paint_line_width, XX-> dashes, XX-> ndashes);
		DASH_FIX;
		XSetDashes( DISP, XX-> gc, 0, DASHES);
		XX-> paint_ndashes = XX-> ndashes;
		if (( XX-> paint_dashes = malloc( XX-> ndashes)))
			memcpy( XX-> paint_dashes, XX-> dashes, XX-> ndashes);
		XX-> line_style = ( XX-> paint_rop2 == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
	} else {
		XX-> paint_dashes = malloc(1);
		if ( XX-> ndashes < 0) {
			XX-> paint_dashes[0] = '\0';
			XX-> paint_ndashes = 0;
		} else {
			XX-> paint_dashes[0] = '\1';
			XX-> paint_ndashes = 1;
		}
		XX-> line_style = LineSolid;
	}

	XX-> clip_rect. x = 0;
	XX-> clip_rect. y = 0;
	XX-> clip_rect. width = XX-> size.x;
	XX-> clip_rect. height = XX-> size.y;
	if ( XX-> invalid_region && inside_on_paint && !is_opt( optInDrawInfo)) {
		if ( XX-> flags. kill_current_region) {
			XDestroyRegion( XX-> current_region);
			XX-> flags. kill_current_region = 0;
		}
		if ( XX-> btransform. x != 0 || XX-> btransform. y != 0) {
			Region r = XCreateRegion();
			XUnionRegion( r, XX-> invalid_region, r);
			XOffsetRegion( r, XX-> btransform. x, -XX-> btransform. y);
			XSetRegion( DISP, XX-> gc, r);
			XX-> current_region = r;
			XX-> flags. kill_current_region = 1;
		} else {
			XSetRegion( DISP, XX-> gc, XX-> invalid_region);
			XX-> current_region = XX-> invalid_region;
			XX-> flags. kill_current_region = 0;
		}
		XX-> paint_region = XX-> invalid_region;
		XX-> invalid_region = NULL;
	}
	XX-> clip_mask_extent. x = XX-> clip_mask_extent. y = 0;
	XX-> flags. xft_clip = 0;

	apc_gp_set_antialias( self, XX-> flags.saved_antialias);
	apc_gp_set_color( self, XX-> saved_fore);
	apc_gp_set_back_color( self, XX-> saved_back);
	memcpy( XX-> saved_fill_pattern, XX-> fill_pattern, sizeof( FillPattern));
	XX-> fill_pattern[0]++; /* force  */
	apc_gp_set_fill_pattern( self, XX-> saved_fill_pattern);
	XX-> saved_fill_pattern_offset = XX-> fill_pattern_offset;
	apc_gp_set_fill_pattern_offset( self, XX-> saved_fill_pattern_offset);

	if ( !XX-> flags. reload_font && XX-> font && XX-> font-> id) {
		XSetFont( DISP, XX-> gc, XX-> font-> id);
		XCHECKPOINT;
	} else {
		apc_gp_set_font( self, &PDrawable( self)-> font);
		XX-> flags. reload_font = false;
	}
}

static void
gc_stack_free( Handle self, PPaintState state)
{
	if ( state-> dashes )
		free(state->dashes);
	if ( state-> in_paint ) {
		if ( state-> paint.gcl)
			TAILQ_INSERT_HEAD(state->paint.gc_pool, state->paint.gcl, gc_link);
		if ( state-> paint.region )
			XDestroyRegion( state-> paint.region );
		if ( state-> paint.kill_tile )
			XFreePixmap( DISP, state-> paint.tile);
		if ( state-> paint.kill_stipple )
			XFreePixmap( DISP, state-> paint.stipple);
	}
	if ( state-> fill_image )
		unprotect_object( state-> fill_image );
	if ( state-> user_destructor )
		state-> user_destructor(self, state->user_data, state->user_data_size, state->in_paint);
	free(state);
}

static Bool
gc_stack_free_paints_only( Handle item, void * p)
{
	PPaintState state = ( PPaintState ) item;
	if ( !state->in_paint) return true;
	gc_stack_free((Handle) p, state);
	return false;
}

static Bool
gc_stack_free_all( Handle item, void * p)
{
	gc_stack_free((Handle) p, ( PPaintState ) item);
	return false;
}

static void
cleanup_gc_stack(Handle self, Bool all)
{
	DEFXX;
	if ( XX && XX-> gc_stack ) {
		if ( all ) {
			list_first_that(XX-> gc_stack, &gc_stack_free_all, (void*) self);
			plist_destroy(XX-> gc_stack);
			XX-> gc_stack = NULL;
		} else
			list_grep(XX-> gc_stack, &gc_stack_free_paints_only, (void*) self);
	}
}

/* COW the pixmaps on the stack */
static void
cleanup_stipples( Handle self )
{
	DEFXX;
	Bool kill = false;

	if ( XX-> gc_stack ) {
		int i;
		for ( i = XX->gc_stack->count - 1; i >= 0; i--) {
			PPaintState state = ( PPaintState ) XX->gc_stack->items[i];
			if ( !state->paint.tile || !state->paint.stipple)
				continue;

			if ( state-> paint.tile       ) state-> paint.kill_tile       = true;
			if ( state-> paint.stipple    ) state-> paint.kill_stipple    = true;
			kill = false;
			break;
		}
	}

	if ( kill ) {
		if ( XX-> fp_tile )
			XFreePixmap( DISP, XX-> fp_tile);
		if ( XX-> fp_stipple )
			XFreePixmap( DISP, XX-> fp_stipple);
	}

	XX-> fp_stipple = XX-> fp_tile = 0;
}

void
prima_cleanup_drawable_after_painting( Handle self)
{
	DEFXX;

	cleanup_gc_stack(self, 0);
	cleanup_stipples(self);
	DELETE_ARGB_PICTURE(XX->argb_picture);
	CLEANUP_RENDER_STIPPLES(self);
#ifdef USE_XFT
	prima_xft_gp_destroy( self );
#endif
	if ( XX-> flags. kill_current_region) {
		XDestroyRegion( XX-> current_region);
		XX-> flags. kill_current_region = 0;
	}
	XX-> current_region = 0;
	XX-> flags. xft_clip = 0;
	if ( XX-> udrawable && XX-> udrawable != XX-> gdrawable && XX-> gdrawable && !is_opt( optInDrawInfo)) {
		if ( XX-> paint_region) {
			XSetRegion( DISP, XX-> gc, XX-> paint_region);
		} else {
			Region region = XCreateRegion();
			XRectangle r;
			r. x = -XX-> btransform. x;
			r. y = XX-> btransform. y;
			r. width = XX->bsize.x;
			r. height = XX->bsize.y;
			XUnionRectWithRegion( &r, region, region);
			XSetRegion( DISP, XX-> gc, region);
			XDestroyRegion( region);
		}
		XCHECKPOINT;
		XSetFunction( DISP, XX-> gc, GXcopy);
		XCopyArea( DISP, XX-> gdrawable, XX-> udrawable, XX-> gc,
					0, 0,
					XX-> bsize.x, XX-> bsize.y,
					-XX-> btransform. x, XX-> btransform. y);
		XCHECKPOINT;
		XFreePixmap( DISP, XX-> gdrawable);
		XCHECKPOINT;
		XX-> gdrawable = XX-> udrawable;
		XX-> btransform. x = XX-> btransform. y = 0;
	}
	prima_release_gc(XX);
	memcpy( XX-> fill_pattern, XX-> saved_fill_pattern, sizeof( FillPattern));
	XX-> fill_pattern_offset = XX-> saved_fill_pattern_offset;
	if ( XX-> font && ( --XX-> font-> refCnt <= 0)) {
		prima_free_rotated_entry( XX-> font);
		XX-> font-> refCnt = 0;
	}
	if ( XX-> paint_dashes) {
		free(XX->paint_dashes);
		XX-> paint_dashes = NULL;
	}
	XX-> paint_ndashes = 0;
	XF_IN_PAINT(XX) = false;
	PDrawable( self)-> font = XX-> saved_font;
	if ( XX-> paint_region) {
		XDestroyRegion( XX-> paint_region);
		XX-> paint_region = NULL;
	}
	XFlush(DISP);
}

#define PURE_FOREGROUND if (!XX->flags.brush_fore) {\
	XSetForeground( DISP, XX-> gc, XX-> fore. primary);\
	XX->flags.brush_fore=1;\
}\
if (!XX->flags.brush_back && XX-> paint_rop2 == ropCopyPut) {\
	XSetBackground( DISP, XX-> gc, XX-> back. primary);\
	XX->flags.brush_back=1;\
}\
XSetFillStyle( DISP, XX-> gc, FillSolid);\

Bool
prima_make_brush( Handle self, int colorIndex)
{
	DEFXX;
	Pixmap p;
	if ( XX-> fp_stipple || XX-> fp_tile ) {
		/* custom image */
		if ( colorIndex > 0) return false;
		XCHECKPOINT;
		if ( XX-> fp_stipple) {
			XSetStipple( DISP, XX-> gc, XX-> fp_stipple);
			XCHECKPOINT;
			if (!XX->flags.brush_fore) {
				XSetForeground( DISP, XX-> gc, XX-> fore. primary);
				XX->flags.brush_fore = 1;
			}
			if (!XX->flags.brush_back) {
				XSetBackground( DISP, XX-> gc, XX-> back. primary);
				XX->flags.brush_back = 1;
			}
			XSetFillStyle( DISP, XX-> gc,
				XX->flags.use_stipple_transparency ? FillStippled : FillOpaqueStippled);
		} else {
			XSetTile( DISP, XX-> gc, XX-> fp_tile);
			XCHECKPOINT;
			XSetFillStyle( DISP, XX-> gc, FillTiled);
		}
	} else if ( XT_IS_BITMAP(XX) || ( guts. idepth == 1)) {
		int i;
		FillPattern mix, *p1, *p2;
		if ( colorIndex > 0) return false;
		p1 = &guts. ditherPatterns[ 64 - (XX-> fore. primary ? 64 : XX-> fore. balance)];
		p2 = &guts. ditherPatterns[ 64 - (XX-> back. primary ? 64 : XX-> back. balance)];
		for ( i = 0; i < sizeof( FillPattern); i++)
			mix[i] = ((*p1)[i] & XX-> fill_pattern[i]) | ((*p2)[i] & ~XX-> fill_pattern[i]);
		XSetForeground( DISP, XX-> gc, 1);
		XSetBackground( DISP, XX-> gc, 0);
		XX-> flags. brush_fore = 0;
		XX-> flags. brush_back = 0;
		if (
			( memcmp( mix, fillPatterns[ fpSolid], sizeof( FillPattern)) != 0) &&
			( p = prima_get_hatch( &mix))
			) {
			XSetStipple( DISP, XX-> gc, p);
			XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
		} else
			XSetFillStyle( DISP, XX-> gc, FillSolid);
	} else if ( XX-> flags. brush_null_hatch && !XX-> flags. use_stipple_transparency) {
		if ( colorIndex > 0) return false;
		if ( XX-> fore. balance) {
			p = prima_get_hatch( &guts. ditherPatterns[ XX-> fore. balance]);
			if ( p) {
				XSetStipple( DISP, XX-> gc, p);
				XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
				XSetBackground( DISP, XX-> gc, XX-> fore. secondary);
				XX-> flags. brush_back = 0;
			} else /* failure */
				XSetFillStyle( DISP, XX-> gc, FillSolid);
		} else
			XSetFillStyle( DISP, XX-> gc, FillSolid);
		if (!XX->flags.brush_fore) {
			XSetForeground( DISP, XX-> gc, XX-> fore. primary);
			XX->flags.brush_fore = 1;
		}
	} else if ( XX->fore.balance == 0 && XX->back.balance == 0) {
		if ( colorIndex > 0) return false;

		p = prima_get_hatch( &XX-> fill_pattern);
		XSetFillStyle( DISP, XX-> gc, p ?
			(XX->flags.use_stipple_transparency ? FillStippled : FillOpaqueStippled) :
			FillSolid);
		if ( p) XSetStipple( DISP, XX-> gc, p);
		if (!XX->flags.brush_fore) {
			XSetForeground( DISP, XX-> gc, XX-> fore. primary);
			XX->flags.brush_fore = 1;
		}
		if (p && !XX->flags.brush_back) {
			XSetBackground( DISP, XX-> gc, XX-> back. primary);
			XX->flags.brush_back = 1;
		}
	} else {
		switch ( colorIndex) {
		case 0: /* back mix */
			if ( XX-> back. balance) {
				p = prima_get_hatch( &guts. ditherPatterns[ XX-> back. balance]);
				if ( p) {
					XSetStipple( DISP, XX-> gc, p);
					XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
					XSetBackground( DISP, XX-> gc, XX-> back. secondary);
				} else  /* failure */
					XSetFillStyle( DISP, XX-> gc, FillSolid);
			} else
				XSetFillStyle( DISP, XX-> gc, FillSolid);
			XSetForeground( DISP, XX-> gc, XX-> back. primary);
			XX-> flags. brush_back = 0;
			break;
		case 1: /* fore mix */
			if ( memcmp( XX-> fill_pattern, fillPatterns[fpEmpty], sizeof(FillPattern))==0)
				return false;
			if ( XX-> fore. balance) {
				int i;
				FillPattern fp;
				memcpy( &fp, &guts. ditherPatterns[ XX-> fore. balance], sizeof(FillPattern));
				for ( i = 0; i < 8; i++)
					fp[i] &= XX-> fill_pattern[i];
				p = prima_get_hatch( &fp);
			} else
				p = prima_get_hatch( &XX-> fill_pattern);
			if ( !p) return false;
			XSetStipple( DISP, XX-> gc, p);
			XSetFillStyle( DISP, XX-> gc, FillStippled);
			if ( !XX-> flags. brush_fore) {
				XSetForeground( DISP, XX-> gc, XX-> fore. primary);
				XX-> flags. brush_fore = 1;
			}
			break;
		case 2: /* fore mix with fill pattern */
			if ( memcmp( XX-> fill_pattern, fillPatterns[fpEmpty], sizeof(FillPattern))==0)
				return false;
			if ( XX-> fore. balance ) {
				int i;
				FillPattern fp;
				memcpy( &fp, &guts. ditherPatterns[ XX-> fore. balance], sizeof(FillPattern));
				for ( i = 0; i < 8; i++)
					fp[i] = (~fp[i]) & XX-> fill_pattern[i];
				p = prima_get_hatch( &fp);
				if ( !p) return false;
				XSetStipple( DISP, XX-> gc, p);
				XSetFillStyle( DISP, XX-> gc, FillStippled);
				XSetForeground( DISP, XX-> gc, XX-> fore. secondary);
				XX-> flags. brush_fore = 0;
				break;
			} else
				return false;
		default:
			return false;
		}
	}
	return true;
}

Bool
apc_gp_init( Handle self)
{
	if ( guts. dynamicColors && !prima_palette_alloc( self)) return false;
	return true;
}

Bool
apc_gp_done( Handle self)
{
	DEFXX;
	if (!XX) return false;
	cleanup_gc_stack(self, 1);
	if ( XX-> dashes) {
		free(XX-> dashes);
		XX-> dashes = NULL;
	}
	XX-> ndashes = 0;
	if ( guts. dynamicColors) {
		prima_palette_free( self, true);
		free(XX-> palette);
	}
	prima_release_gc(XX);
	return true;
}

static int
arc_completion( double * angleStart, double * angleEnd, int * needFigure)
{
	int max;
	long diff = (long)( fabs( *angleEnd - *angleStart) * 64 + 0.5);

	if ( diff == 0) {
		*needFigure = false;
		return 0;
	}
	diff /= 64;

	while ( *angleStart > *angleEnd)
		*angleEnd += 360;

	while ( *angleStart < 0) {
		*angleStart += 360;
		*angleEnd   += 360;
	}

	while ( *angleStart >= 360) {
		*angleStart -= 360;
		*angleEnd   -= 360;
	}

	while ( *angleEnd >= *angleStart + 360)
		*angleEnd -= 360;

	if ( diff < 360) {
		*needFigure = true;
		return 0;
	}

	max = (int)(diff / 360);
	*needFigure = ( max * 360) != diff;
	return ( max % 2) ? 1 : 2;
}

static void
calculate_ellipse_divergence(void)
{
	static Bool init = false;
	if ( !init) {
		XGCValues gcv;
		Pixmap px = XCreatePixmap( DISP, guts.root, 4, 4, 1);
		GC gc = XCreateGC( DISP, px, 0, &gcv);
		XImage *xi;
		XSetForeground( DISP, gc, 0);
		XFillRectangle( DISP, px, gc, 0, 0, 5, 5);
		XSetForeground( DISP, gc, 1);
		XDrawArc( DISP, px, gc, 0, 0, 4, 4, 0, 360 * 64);
		if (( xi = XGetImage( DISP, px, 0, 0, 4, 4, 1, XYPixmap))) {
			int i;
			Byte *data[4];
			if ( xi-> bitmap_bit_order == LSBFirst)
				prima_mirror_bytes(( Byte*) xi-> data, xi-> bytes_per_line * 4);
			for ( i = 0; i < 4; i++) data[i] = (Byte*)xi-> data + i * xi-> bytes_per_line;
#define PIX(x,y) ((data[y][0] & (0x80>>(x)))!=0)
			if (  PIX(2,1) && !PIX(3,1)) guts. ellipseDivergence.x = -1; else
			if ( !PIX(2,1) && !PIX(3,1)) guts. ellipseDivergence.x = 1;
			if (  PIX(1,2) && !PIX(1,3)) guts. ellipseDivergence.y = -1; else
			if ( !PIX(1,2) && !PIX(1,3)) guts. ellipseDivergence.y = 1;
#undef PIX
			XDestroyImage( xi);
		}
		XFreeGC( DISP, gc);
		XFreePixmap( DISP, px);
		init = true;
	}
}

#define ELLIPSE_RECT x - ( dX + 1) / 2 + 1, y - dY / 2, dX-guts.ellipseDivergence.x, dY-guts.ellipseDivergence.y
#define FILL_ANTIDEFECT_REPAIRABLE (((XX->fill_mode & fmOverlay) != 0) && \
		( rop_map[XX-> paint_rop] == GXcopy ||\
		rop_map[XX-> paint_rop] == GXset  ||\
		rop_map[XX-> paint_rop] == GXclear))
#define FILL_ANTIDEFECT_OPEN {\
XGCValues gcv;\
gcv. line_width = 1;\
gcv. line_style = LineSolid;\
XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);\
}
#define FILL_ANTIDEFECT_CLOSE {\
XGCValues gcv;\
gcv. line_width = XX-> paint_line_width + .5;\
XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);\
}

Bool
apc_gp_arc( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
	DEFXX;
	int compl, needf;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);

	SHIFT( x, y);
	y = REVERT( y);
	PURE_FOREGROUND;
	calculate_ellipse_divergence();
	compl = arc_completion( &angleStart, &angleEnd, &needf);
	while ( compl--)
		XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 360 * 64);
	if ( needf)
		XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
			angleStart * 64, ( angleEnd - angleStart) * 64);
	XFLUSH;
	return true;
}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
	DEFXX;
	int mix = 0;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	SHIFT( x1, y1); SHIFT( x2, y2);
	SORT( x1, x2); SORT( y1, y2);
	RANGE4( x1, y1, x2, y2);
	while ( prima_make_brush( self, mix++))
		XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
	XCHECKPOINT;
	XFLUSH;
	return true;
}

Bool
apc_gp_bars( Handle self, int nr, Rect *rr)
{
	DEFXX;
	XRectangle *r, *rp;
	int i, mix = 0;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ((r = malloc( sizeof( XRectangle)*nr)) == NULL) return false;

	for ( i = 0, rp = r; i < nr; i++, rr++, rp++) {
		SHIFT( rr->left,rr-> bottom); SHIFT( rr->right, rr->top);
		SORT( rr->left, rr->right); SORT( rr->bottom, rr->top);
		RANGE4( rr->left, rr->bottom, rr->right, rr->top);
		rp->x = rr->left;
		rp->y = REVERT(rr->top);
		rp->width = rr->right - rr->left + 1;
		rp->height = rr->top - rr->bottom + 1;
	}

	while ( prima_make_brush( self, mix++))
		XFillRectangles( DISP, XX-> gdrawable, XX-> gc, r, nr);

	XCHECKPOINT;
	XFLUSH;
	free( r);
	return true;
}

Bool
apc_gp_alpha( Handle self, int alpha, int x1, int y1, int x2, int y2)
{
	DEFXX;
	int pixel;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( !XF_LAYERED(XX)) return false;
	if ( XT_IS_WIDGET(XX) && !XX->flags. layered_requested) return false;

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
		x1 = 0; y1 = 0;
		x2 = XX-> size. x - 1;
		y2 = XX-> size. y - 1;
	}
	SHIFT( x1, y1); SHIFT( x2, y2);
	SORT( x1, x2); SORT( y1, y2);
	RANGE4( x1, y1, x2, y2);

	pixel = ((alpha << guts. argb_bits. alpha_range) >> 8) << guts. argb_bits. alpha_shift;
	XSetForeground( DISP, XX-> gc, pixel);
	XX-> flags. brush_fore = 0;
	XSetPlaneMask( DISP, XX-> gc, guts. argb_bits. alpha_mask);
	XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
	XSetPlaneMask( DISP, XX-> gc, AllPlanes);
	XFLUSH;

	return true;
}

Bool
apc_gp_can_draw_alpha( Handle self)
{
	DEFXX;
	if (XT_IS_BITMAP(XX) || (( XT_IS_PIXMAP(XX) || XT_IS_APPLICATION(XX)) && guts.depth==1))
		return false;
	else
		return
			guts.render_extension
#ifdef WITH_COCOA
			&& !prima_cocoa_is_x11_local()
#endif
		;
}

Bool
apc_gp_clear( Handle self, int x1, int y1, int x2, int y2)
{
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
		x1 = 0; y1 = 0;
		x2 = XX-> size. x - 1;
		y2 = XX-> size. y - 1;
	}
	SHIFT( x1, y1); SHIFT( x2, y2);
	SORT( x1, x2); SORT( y1, y2);
	RANGE4( x1, y1, x2, y2);

	/* clean color entries, leave just background & foreground. XXX */
	if ( guts. dynamicColors && x1 <= 0 && x2 > XX-> size.x && y1 <= 0 && y2 >= XX-> size.y) {
		prima_palette_free(self,false);
		apc_gp_set_color(self, XX-> fore. color);
		apc_gp_set_back_color(self, XX-> back. color);
	}

	XSetForeground( DISP, XX-> gc, XX-> back. primary);
	if ( XX-> back. balance > 0) {
		Pixmap p = prima_get_hatch( &guts. ditherPatterns[ XX-> back. balance]);
		if ( p) {
			XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
			XSetStipple( DISP, XX-> gc, p);
			XSetBackground( DISP, XX-> gc, XX-> back. secondary);
		} else
			XSetFillStyle( DISP, XX-> gc, FillSolid);
	} else
		XSetFillStyle( DISP, XX-> gc, FillSolid);
	XX-> flags. brush_fore = 0;
	XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
	XFLUSH;

	return true;
}

#define GRAD 57.29577951

Bool
apc_gp_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
	int compl, needf;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);

	SHIFT( x, y);
	y = REVERT( y);
	PURE_FOREGROUND;
	compl = arc_completion( &angleStart, &angleEnd, &needf);
	calculate_ellipse_divergence();
	while ( compl--)
		XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 360 * 64);
	if ( !needf) return true;
	XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
		angleStart * 64, ( angleEnd - angleStart) * 64);
	XDrawLine( DISP, XX-> gdrawable, XX-> gc,
		x + cos( angleStart / GRAD) * dX / 2, y - sin( angleStart / GRAD) * dY / 2,
		x + cos( angleEnd / GRAD) * dX / 2,   y - sin( angleEnd / GRAD) * dY / 2);
	XFLUSH;
	return true;
}

Bool
apc_gp_draw_poly( Handle self, int n, Point *pp)
{
	DEFXX;
	int i;
	int x = XX-> gtransform. x + XX-> btransform. x;
	int y = XX-> size. y - 1 - XX-> gtransform. y - XX-> btransform. y;
	XPoint *p;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ((p = malloc( sizeof( XPoint)*n)) == NULL)
		return false;

	for ( i = 0; i < n; i++) {
		p[i].x = pp[i].x + x;
		p[i].y = y - pp[i].y;
		RANGE2(p[i].x, p[i].y);
	}

	PURE_FOREGROUND;
	XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, n, CoordModeOrigin);

	free( p);
	XFLUSH;
	return true;
}

Bool
apc_gp_draw_poly2( Handle self, int np, Point *pp)
{
	DEFXX;
	int i;
	int x = XX-> gtransform. x + XX-> btransform. x;
	int y = XX-> size. y - 1 - XX-> gtransform. y - XX-> btransform. y;
	XSegment *s;
	int n = np / 2;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ((s = malloc( sizeof( XSegment)*n)) == NULL) return false;

	for ( i = 0; i < n; i++) {
		s[i].x1 = pp[i*2].x + x;
		s[i].y1 = y - pp[i*2].y;
		s[i].x2 = pp[i*2+1].x + x;
		s[i].y2 = y - pp[i*2+1].y;
		RANGE4(s[i].x1, s[i].y1, s[i].x2, s[i].y2);
	}

	PURE_FOREGROUND;
	XDrawSegments( DISP, XX-> gdrawable, XX-> gc, s, n);

	free( s);
	XFLUSH;
	return true;
}

Bool
apc_gp_ellipse( Handle self, int x, int y, int dX, int dY)
{
	DEFXX;

	if ( dX == 1 || dY == 1 ) /* Xorg bug */
		return apc_gp_rectangle( self, x - dX / 2, y - dY / 2, x + dX / 2, y + dY / 2);

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);

	SHIFT( x, y);
	y = REVERT( y);
	PURE_FOREGROUND;
	calculate_ellipse_divergence();
	XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 64*360);
	XFLUSH;
	return true;
}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
	DEFXX;
	int compl, needf, mix = 0;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);

	SHIFT( x, y);
	y = REVERT( y);

	XSetArcMode( DISP, XX-> gc, ArcChord);
	FILL_ANTIDEFECT_OPEN;

	while ( prima_make_brush( self, mix++)) {
		compl = arc_completion( &angleStart, &angleEnd, &needf);
		while ( compl--) {
			XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
			if ( FILL_ANTIDEFECT_REPAIRABLE)
				XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 0, 64*360);
		}

		if ( needf) {
			XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY,
				angleStart * 64, ( angleEnd - angleStart) * 64);
			if ( FILL_ANTIDEFECT_REPAIRABLE)
				XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1,
					angleStart * 64, ( angleEnd - angleStart) * 64);
		}
	}
	FILL_ANTIDEFECT_CLOSE;
	XFLUSH;
	return true;
}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y,  int dX, int dY)
{
	DEFXX;
	int mix = 0;

	if ( dX == 1 || dY == 1 ) /* Xorg bug */
		return apc_gp_bar( self, x - dX / 2, y - dY / 2, x + dX / 2, y + dY / 2);

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);
	SHIFT( x, y);
	y = REVERT( y);

	FILL_ANTIDEFECT_OPEN;
	while ( prima_make_brush( self, mix++)) {
		XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
		if ( FILL_ANTIDEFECT_REPAIRABLE)
			XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 0, 64*360);
	}
	FILL_ANTIDEFECT_CLOSE;
	XFLUSH;
	return true;
}

Bool
apc_gp_fill_poly( Handle self, int numPts, Point *points)
{
	/* XXX - beware, current implementation will not deal correctly with different rops and tiles */
	XPoint *p;
	DEFXX;
	int i, mix = 0;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( !( p = malloc(( numPts + 1) * sizeof( XPoint)))) return false;

	for ( i = 0; i < numPts; i++) {
		p[i]. x = (short)points[i]. x + XX-> gtransform. x + XX-> btransform. x;
		p[i]. y = (short)REVERT(points[i]. y + XX-> gtransform. y + XX-> btransform. y);
		RANGE2(p[i].x, p[i].y);
	}
	p[numPts]. x = (short)points[0]. x + XX-> gtransform. x + XX-> btransform. x;
	p[numPts]. y = (short)REVERT(points[0]. y + XX-> gtransform. y + XX-> btransform. y);
	RANGE2(p[numPts].x, p[numPts].y);

	FILL_ANTIDEFECT_OPEN;
	if ( guts. limits. XFillPolygon >= numPts) {
		while ( prima_make_brush( self, mix++)) {
			XFillPolygon( DISP, XX-> gdrawable, XX-> gc, p, numPts, ComplexShape, CoordModeOrigin);
			if ( FILL_ANTIDEFECT_REPAIRABLE) {
				XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, numPts+1, CoordModeOrigin);
			}
		}
		XCHECKPOINT;
	} else
		warn( "Prima::Drawable::fill_poly: too many points");
	FILL_ANTIDEFECT_CLOSE;
	free( p);
	XFLUSH;
	return true;
}

Bool
apc_gp_fill_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
	DEFXX;
	int compl, needf, mix = 0;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);

	SHIFT( x, y);
	y = REVERT( y);
	XSetArcMode( DISP, XX-> gc, ArcPieSlice);

	FILL_ANTIDEFECT_OPEN;
	while ( prima_make_brush( self, mix++)) {
		compl = arc_completion( &angleStart, &angleEnd, &needf);
		while ( compl--) {
			XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
			if ( FILL_ANTIDEFECT_REPAIRABLE)
				XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 0, 64*360);
		}

		if ( needf) {
			XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY,
				angleStart * 64, ( angleEnd - angleStart) * 64);
			if ( FILL_ANTIDEFECT_REPAIRABLE)
				XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1,
					angleStart * 64, ( angleEnd - angleStart) * 64);
		}
	}
	FILL_ANTIDEFECT_CLOSE;
	XFLUSH;
	return true;
}

static int
get_pixel_depth( int depth)
{
	if ( depth ==  1) return  1; else
	if ( depth <=  4) return  4; else
	if ( depth <=  8) return  8; else
	if ( depth <= 16) return 16; else
	if ( depth <= 24) return 24; else
	return 32;
}


static uint32_t
color_to_pixel( Handle self, Color color, int depth)
{
	uint32_t pv;

	if ( depth == 1) {
		pv = color ? 1 : 0;
	} else if ( guts.palSize > 0 ) {
		pv = prima_color_find( self, color, -1, NULL, RANK_FREE);
	} else {
		PRGBABitDescription bd = GET_RGBA_DESCRIPTION;
		switch ( depth) {
		case 16:
		case 24:
		case 32:
			pv = COLOR2DEV_RGB(bd,color);
			if ( guts.machine_byte_order != guts.byte_order)
				switch( depth) {
				case 16:
					pv = REVERSE_BYTES_16( pv);
					break;
				case 24:
					pv = REVERSE_BYTES_24( pv);
					break;
				case 32:
					pv = REVERSE_BYTES_32( pv);
					break;
				}
			break;
		default:
			warn("UAG_005: Not supported pixel depth");
			return 0;
		}
	}
	return pv;
}

typedef struct {
	XImage *  i;
	Rect      clip;
	uint32_t  color;
	int       depth;
	int       y;
	Bool      singleBorder;
	XDrawable drawable;
	GC        gc;
	int       first;
	PList  *  lists;
} FillSession;

static Bool
fs_get_pixel( FillSession * fs, int x, int y)
{
	if ( x < fs-> clip. left || x > fs-> clip. right || y < fs-> clip. top || y > fs-> clip. bottom)  {
		return false;
	}

	if ( fs-> lists[ y - fs-> first]) {
		PList l = fs-> lists[ y - fs-> first];
		int i;
		for ( i = 0; i < l-> count; i+=2) {
			if (((int) l-> items[i+1] >= x) && ((int)l->items[i] <= x))
				return false;
		}
	}

	if ( !fs-> i || y != fs-> y) {
		XCHECKPOINT;
		if ( fs-> i) XDestroyImage( fs-> i);
		XCHECKPOINT;
		fs-> i = XGetImage( DISP, fs-> drawable, fs-> clip. left, y,
								fs-> clip. right - fs-> clip. left + 1, 1,
								( fs-> depth == 1) ? 1 : AllPlanes,
								( fs-> depth == 1) ? XYPixmap : ZPixmap);
		XCHECKPOINT;
		if ( !fs-> i) {
			return false;
		}
		fs-> y = y;
	}

	x -= fs-> clip. left;

	switch( fs-> depth) {
	case 1:
		{
			Byte xz = *((Byte*)(fs->i->data) + (x >> 3));
			uint32_t v = ( guts.bit_order == MSBFirst) ?
				( xz & ( 0x80 >> ( x & 7)) ? 1 : 0) :
				( xz & ( 0x01 << ( x & 7)) ? 1 : 0);
			return fs-> singleBorder ?
				( v == fs-> color) : ( v != fs-> color);
		}
		break;
	case 4:
		{
			Byte xz = *((Byte*)(fs->i->data) + (x >> 1));
			uint32_t v = ( x & 1) ? ( xz & 0xF) : ( xz >> 4);
			return fs-> singleBorder ?
				( v == fs-> color) : ( v != fs-> color);
		}
		break;
	case 8:
	return fs-> singleBorder ?
		( fs-> color == *((Byte*)(fs->i->data) + x)) :
		( fs-> color != *((Byte*)(fs->i->data) + x));
	case 16:
	return fs-> singleBorder ?
		( fs-> color == *((uint16_t*)(fs->i->data) + x)):
		( fs-> color != *((uint16_t*)(fs->i->data) + x));
	case 24:
		return fs-> singleBorder ?
		( memcmp(( Byte*)(fs->i->data) + x, &fs->color, 3) == 0) :
		( memcmp(( Byte*)(fs->i->data) + x, &fs->color, 3) != 0);
	case 32:
		return fs-> singleBorder ?
		( fs-> color == *((uint32_t*)(fs->i->data) + x)):
		( fs-> color != *((uint32_t*)(fs->i->data) + x));
	}
	return false;
}

static void
hline( FillSession * fs, int x1, int y, int x2)
{
	XFillRectangle( DISP, fs-> drawable, fs-> gc, x1, y, x2 - x1 + 1, 1);

	if ( y == fs-> y && fs-> i) {
		XDestroyImage( fs-> i);
		fs-> i = NULL;
	}

	y -= fs-> first;

	if ( fs-> lists[y] == NULL)
		fs-> lists[y] = plist_create( 32, 128);
	list_add( fs-> lists[y], ( Handle) x1);
	list_add( fs-> lists[y], ( Handle) x2);
}

static int
fill( FillSession * fs, int sx, int sy, int d, int pxl, int pxr)
{
	int x, xr = sx;
	while ( sx > fs-> clip. left  && fs_get_pixel( fs, sx - 1, sy)) sx--;
	while ( xr < fs-> clip. right && fs_get_pixel( fs, xr + 1, sy)) xr++;
	hline( fs, sx, sy, xr);

	if ( sy + d >= fs-> clip. top && sy + d <= fs-> clip. bottom) {
		x = sx;
		while ( x <= xr) {
			if ( fs_get_pixel( fs, x, sy + d)) {
				x = fill( fs, x, sy + d, d, sx, xr);
			}
			x++;
		}
	}

	if ( sy - d >= fs-> clip. top && sy - d <= fs-> clip. bottom) {
		x = sx;
		while ( x < pxl) {
			if ( fs_get_pixel( fs, x, sy - d)) {
				x = fill( fs, x, sy - d, -d, sx, xr);
			}
			x++;
		}
		x = pxr;
		while ( x <= xr) {
			if ( fs_get_pixel( fs, x, sy - d)) {
				x = fill( fs, x, sy - d, -d, sx, xr);
			}
			x++;
		}
	}
	return xr;
}

Bool
apc_gp_flood_fill( Handle self, int x, int y, Color color, Bool singleBorder)
{
	DEFXX;
	Bool ret = false;
	XRectangle cr;
	FillSession s;
	int mix = 0, hint;

	if ( !opt_InPaint) return false;

	s. singleBorder = singleBorder;
	s. drawable     = XX-> gdrawable;
	s. gc           = XX-> gc;
	SHIFT( x, y);
	y = REVERT( y);
	color = prima_map_color( color, &hint);
	prima_gp_get_clip_rect( self, &cr, 1);

	s. clip. left   = cr. x;
	s. clip. top    = cr. y;
	s. clip. right  = cr. x + cr. width  - 1;
	s. clip. bottom = cr. y + cr. height - 1;
	if ( cr. width <= 0 || cr. height <= 0) return false;
	s. i = NULL;
	s. depth = XT_IS_BITMAP(X(self)) ? 1 : guts. idepth;
	s. depth = get_pixel_depth( s. depth);
	s. color = hint ?
		(( hint == COLORHINT_BLACK) ? LOGCOLOR_BLACK : LOGCOLOR_WHITE)
		: color_to_pixel( self, color, s.depth);

	s. first = s. clip. top;
	if ( !( s. lists = malloc(( s. clip. bottom - s. clip. top + 1) * sizeof( void*))))
		return false;
	bzero( s. lists, ( s. clip. bottom - s. clip. top + 1) * sizeof( void*));

	prima_make_brush( self, mix++);
	if ( fs_get_pixel( &s, x, y)) {
		fill( &s, x, y, -1, x, x);
		ret = true;
	}

	while ( prima_make_brush( self, mix++)) {
		for ( y = 0; y < s. clip. bottom - s. clip. top + 1; y++)
			if ( s. lists[y])
				for ( x = 0; x < s.lists[y]-> count; x += 2)
					XFillRectangle( DISP, s.drawable, s.gc,
						(int)s.lists[y]->items[x], y,
						(int)s.lists[y]->items[x+1] - (int)s.lists[y]->items[x], 1);
	}

	if ( s. i) XDestroyImage( s. i);

	for ( x = 0; x < s. clip. bottom - s. clip. top + 1; x++)
		if ( s. lists[x])
			plist_destroy( s.lists[x]);
	free( s. lists);
	XFLUSH;

	return ret;
}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{
	DEFXX;
	Color c = 0;
	XImage *im;
	Bool pixmap;
	uint32_t p32 = 0;

	if ( !opt_InPaint) return clInvalid;
	SHIFT( x, y);

	if ( x < 0 || x >= XX-> size.x || y < 0 || y >= XX-> size.y)
		return clInvalid;

	if ( XT_IS_DBM(XX)) {
		pixmap = XT_IS_PIXMAP(XX) ? true : false;
	} else if ( XT_IS_BITMAP(XX)) {
		pixmap = 0;
	} else {
		pixmap = guts. idepth > 1;
	}

	im = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y - y - 1, 1, 1,
						pixmap ? AllPlanes : 1,
						pixmap ? ZPixmap   : XYPixmap);
	XCHECKPOINT;
	if ( !im) return clInvalid;

	if ( pixmap) {
		if ( guts. palSize > 0) {
			int pixel;
			if ( guts. idepth <= 8)
				pixel = (*( U8*)( im-> data)) & (( 1 << guts.idepth) - 1);
			else
				pixel = (*( U16*)( im-> data)) & (( 1 << guts.idepth) - 1);
			if ( guts.palette[pixel]. rank == RANK_FREE) {
				XColor xc;
				xc.pixel = pixel;
				XQueryColors( DISP, guts. defaultColormap, &xc, 1);
				c = RGB_COMPOSITE(xc.red>>8,xc.green>>8,xc.blue>>8);
			} else
				c = guts.palette[pixel]. composite;
		} else {
			PRGBABitDescription bd = GET_RGBA_DESCRIPTION;
			int r, g, b, rmax, gmax, bmax, depth;
			rmax = gmax = bmax = 0xff;
			depth = XF_LAYERED(XX) ? guts. argb_visual. depth : guts. idepth;
			switch ( depth) {
			case 16:
				p32 = *(( uint16_t*)(im-> data));
				if ( guts.machine_byte_order != guts.byte_order)
					p32 = REVERSE_BYTES_16(p32);
				rmax = 0xff & ( 0xff << ( 8 - bd-> red_range));
				gmax = 0xff & ( 0xff << ( 8 - bd-> green_range));
				bmax = 0xff & ( 0xff << ( 8 - bd-> blue_range));
				goto COMP;
			case 24:
				p32 = (im-> data[0] << 16) | (im-> data[1] << 8) | im-> data[2];
				if ( guts.machine_byte_order != guts.byte_order)
					p32 = REVERSE_BYTES_24(p32);
				goto COMP;
			case 32:
				p32 = *(( uint32_t*)(im-> data));
				if ( guts.machine_byte_order != guts.byte_order)
					p32 = REVERSE_BYTES_32(p32);
			COMP:
				r = ((((p32 & bd-> red_mask)   >> bd->red_shift) << 8)   >> bd-> red_range) & 0xff;
				g = ((((p32 & bd-> green_mask) >> bd->green_shift) << 8) >> bd-> green_range) & 0xff;
				b = ((((p32 & bd-> blue_mask)  >> bd->blue_shift) << 8)  >> bd-> blue_range) & 0xff;
				if ( r == rmax ) r = 0xff;
				if ( g == gmax ) g = 0xff;
				if ( b == bmax ) b = 0xff;
				c = b | ( g << 8 ) | ( r << 16);
				break;
			default:
				warn("UAG_009: get_pixel not implemented for %d depth", guts.idepth);
			}
		}
	} else {
		c = ( im-> data[0] & ((guts.bit_order == MSBFirst) ? 0x80 : 1))
			? 0xffffff : 0;
	}

	XDestroyImage( im);
	return c;
}

Color
apc_gp_get_nearest_color( Handle self, Color color)
{
	if ( guts. palSize > 0)
		return guts. palette[ prima_color_find( self, color, -1, NULL, RANK_FREE)]. composite;
	if ( guts. visualClass == TrueColor || guts. visualClass == DirectColor) {
		XColor xc;
		xc. red   = COLOR_R16(color);
		xc. green = COLOR_G16(color);
		xc. blue  = COLOR_B16(color);
		if ( XAllocColor( DISP, guts. defaultColormap, &xc)) {
			XFreeColors( DISP, guts. defaultColormap, &xc. pixel, 1, 0);
			return
				(( xc. red   & 0xFF00) << 8) |
				(( xc. green & 0xFF00)) |
				(( xc. blue  & 0xFF00) >> 8);
		}
	}
	return color;
}

PRGBColor
apc_gp_get_physical_palette( Handle self, int * colors)
{
	int i;
	PRGBColor p;
	XColor * xc;

	*colors = 0;

	if ( guts. palSize == 0) return NULL;
	if ( !( p = malloc( guts. palSize * sizeof( RGBColor))))
		return NULL;
	if ( !( xc = malloc( guts. palSize * sizeof( XColor)))) {
		free( p);
		return NULL;
	}
	for ( i = 0; i < guts. palSize; i++) xc[i]. pixel = i;
	XQueryColors( DISP, guts. defaultColormap, xc, guts. palSize);
	XCHECKPOINT;
	for ( i = 0; i < guts. palSize; i++) {
		p[i]. r = xc[i]. red   >> 8;
		p[i]. g = xc[i]. green >> 8;
		p[i]. b = xc[i]. blue  >> 8;
	}
	free( xc);
	*colors = guts. palSize;
	return p;
}

Bool
apc_gp_line( Handle self, int x1, int y1, int x2, int y2)
{
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	SHIFT( x1, y1); SHIFT( x2, y2);
	RANGE4(x1, y1, x2, y2); /* not really correct */
	PURE_FOREGROUND;
	if (((int)(XX-> paint_line_width + .5) == 0) && (x1 == x2 || y1 == y2)) {
		XGCValues gcv;
		gcv. line_width = 1;
		XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
	}
	XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
	if (((int)(XX-> paint_line_width + .5) == 0) && (x1 == x2 || y1 == y2)) {
		XGCValues gcv;
		gcv. line_width = 0;
		XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
	}
	XFLUSH;
	return true;
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
	DEFXX;
	int lw = XX-> paint_line_width + .5;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	SHIFT( x1, y1); SHIFT( x2, y2);
	SORT( x1, x2); SORT( y1, y2);
	RANGE4(x1, y1, x2, y2);
	PURE_FOREGROUND;
	if ( lw > 0 && (lw % 2) == 0) {
		y2--;
		y1--;
	}
	XDrawRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1, y2 - y1);
	XCHECKPOINT;
	XFLUSH;
	return true;
}

Bool
apc_gp_sector( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
	int compl, needf;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;
	if ( dX <= 0 || dY <= 0) return false;
	RANGE4(x, y, dX, dY);

	SHIFT( x, y);
	y = REVERT( y);

	compl = arc_completion( &angleStart, &angleEnd, &needf);
	PURE_FOREGROUND;
	calculate_ellipse_divergence();
	while ( compl--)
		XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
			0, 360 * 64);
	if ( !needf) return true;
	XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
		angleStart * 64, ( angleEnd - angleStart) * 64);
	XDrawLine( DISP, XX-> gdrawable, XX-> gc,
		x + cos( angleStart / GRAD) * dX / 2, y - sin( angleStart / GRAD) * dY / 2,
		x, y
	);
	XDrawLine( DISP, XX-> gdrawable, XX-> gc,
		x, y,
		x + cos( angleEnd / GRAD) * dX / 2, y - sin( angleEnd / GRAD) * dY / 2
	);
	XFLUSH;
	return true;
}

Bool
apc_gp_set_palette( Handle self)
{
	if ( XT_IS_WIDGET(X(self))) return true;
	return prima_palette_replace( self, false);
}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	SHIFT( x, y);
	XSetForeground( DISP, XX-> gc, prima_allocate_color( self, color, NULL));
	XDrawPoint( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y));
	XX-> flags. brush_fore = 0;
	XFLUSH;
	return true;
}

/* gpi settings */
int
apc_gp_get_alpha( Handle self)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		return XX-> paint_alpha;
	} else {
		return XX-> alpha;
	}
}

Bool
apc_gp_get_antialias( Handle self)
{
	DEFXX;
	return XF_IN_PAINT(XX) ? XX-> flags.antialias : XX->flags.saved_antialias;
}

Color
apc_gp_get_back_color( Handle self)
{
	DEFXX;
	return ( XF_IN_PAINT(XX)) ? XX-> back. color : prima_map_color( XX-> saved_back, NULL);
}

int
apc_gp_get_bpp( Handle self)
{
	DEFXX;
	if ( XT_IS_BITMAP(XX)) return 1;
	if ( XF_LAYERED(XX)) return guts. argb_depth;
	return guts. depth;
}

Color
apc_gp_get_color( Handle self)
{
	DEFXX;
	return ( XF_IN_PAINT(XX)) ? XX-> fore. color : prima_map_color(XX-> saved_fore, NULL);
}

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{
	DEFXX;
	unsigned long * ret = NULL;
	XFontStruct * fs;
#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_get_font_ranges( self, count);
#endif
	fs = XX-> font-> fs;
	*count = (fs-> max_byte1 - fs-> min_byte1 + 1) * 2;
	if (( ret = malloc( sizeof( unsigned long) * ( *count)))) {
		int i;
		for ( i = fs-> min_byte1; i <= fs-> max_byte1; i++) {
			ret[(i - fs-> min_byte1) * 2 + 0] = i * 256 + fs-> min_char_or_byte2;
			ret[(i - fs-> min_byte1) * 2 + 1] = i * 256 + fs-> max_char_or_byte2;
		}
	}
	return ret;
}

char *
apc_gp_get_font_languages( Handle self)
{
	DEFXX;
	char * ret;
#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_get_font_languages(self);
#endif
	if ( XX-> font-> flags.funky )
		return NULL;
	if ( !( ret = malloc(4)))
		return NULL;
	memcpy(ret, "en\0\0", 4);
	return ret;
}

int
apc_gp_get_fill_mode( Handle self)
{
	DEFXX;
	return XF_IN_PAINT(XX) ? XX->fill_mode : XX->saved_fill_mode;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
	return &(X(self)-> fill_pattern);
}

Point
apc_gp_get_fill_pattern_offset( Handle self)
{
	Point fpo = X(self)-> fill_pattern_offset;
	fpo.y = 8 - fpo.y;
	return fpo;
}

int
apc_gp_get_line_end( Handle self)
{
	DEFXX;
	int cap;
	XGCValues gcv;

	if ( XF_IN_PAINT(XX)) {
		if ( XGetGCValues( DISP, XX-> gc, GCCapStyle, &gcv) == 0) {
			warn( "UAG_006: error querying GC values");
			cap = CapButt;
		} else {
			cap = gcv. cap_style;
		}
	} else {
		cap = XX-> gcv. cap_style;
	}
	if ( cap == CapRound)
		return leRound;
	else if ( cap == CapProjecting)
		return leSquare;
	return leFlat;
}

int
apc_gp_get_line_join( Handle self)
{
	DEFXX;
	int join;
	XGCValues gcv;

	if ( XF_IN_PAINT(XX)) {
		if ( XGetGCValues( DISP, XX-> gc, GCJoinStyle, &gcv) == 0) {
			warn( "UAG_006: error querying GC values");
			join = JoinRound;
		} else {
			join = gcv. join_style;
		}
	} else {
		join = XX-> gcv. join_style;
	}
	if ( join == JoinMiter)
		return ljMiter;
	else if ( join == JoinBevel)
		return ljBevel;
	return ljRound;
}

float
apc_gp_get_line_width( Handle self)
{
	DEFXX;
	return XF_IN_PAINT(XX) ? XX-> paint_line_width : XX-> line_width;
}

int
apc_gp_get_line_pattern( Handle self, unsigned char *dashes)
{
	DEFXX;
	int n;
	if ( XF_IN_PAINT(XX)) {
		n = XX-> paint_ndashes;
		if ( XX-> paint_dashes)
			memcpy( dashes, XX-> paint_dashes, n);
		else
			bzero( dashes, n);
	} else {
		n = XX-> ndashes;
		if ( n < 0) {
			n = 0;
			strcpy(( char*) dashes, "");
		} else if ( n == 0) {
			n = 1;
			strcpy(( char*) dashes, "\1");
		} else {
			memcpy( dashes, XX-> dashes, n);
		}
	}
	return n;
}

float
apc_gp_get_miter_limit( Handle self)
{
	DEFXX;
	/* xorg.miArcJoin: don't miter arcs with less than 11 degrees between them */
	if ( XF_IN_PAINT(XX) ) return 1.0 / sin((11.0 * 3.14159265358 / 180) / 2);
	return XX->miter_limit;
}

Point
apc_gp_get_resolution( Handle self)
{
	return guts. resolution;
}

int
apc_gp_get_rop( Handle self)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		return XX-> paint_rop;
	} else {
		return XX-> rop;
	}
}

int
apc_gp_get_rop2( Handle self)
{
	DEFXX;
	if ( XF_IN_PAINT(XX))
		return XX-> paint_rop2;
	else
		return XX-> rop2;
}

Point
apc_gp_get_transform( Handle self)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		return XX-> gtransform;
	} else {
		return XX-> transform;
	}
}

Bool
apc_gp_get_text_opaque( Handle self)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		return XX-> flags. paint_opaque ? true : false;
	} else {
		return XX-> flags. opaque ? true : false;
	}
}

Bool
apc_gp_get_text_out_baseline( Handle self)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		return XX-> flags. paint_base_line ? true : false;
	} else {
		return XX-> flags. base_line ? true : false;
	}
}

Bool
apc_gp_set_alpha( Handle self, int alpha)
{
	DEFXX;

	if (XT_IS_BITMAP(XX) || (( XT_IS_PIXMAP(XX) || XT_IS_APPLICATION(XX)) && guts.depth==1))
		alpha = 255;
	if ( !guts.render_extension)
		alpha = 255;

	if ( XF_IN_PAINT(XX)) {
		if ( XX-> paint_alpha == alpha)
			return true;
		XX-> paint_alpha = alpha;
		guts.xrender_pen_dirty = true;
	} else {
		XX-> alpha = alpha;
	}
	return true;
}

Bool
apc_gp_set_antialias( Handle self, Bool antialias )
{
	DEFXX;
	if ( antialias ) {
		if (XT_IS_BITMAP(XX) || (( XT_IS_PIXMAP(XX) || XT_IS_APPLICATION(XX)) && guts.depth==1))
			return false;
		if ( !guts.render_extension)
			return false;
	}
	if ( XF_IN_PAINT(XX) ) {
		XX-> flags.antialias = antialias;
	} else {
		XX-> flags.saved_antialias = antialias;
	}
	return true;
}

Bool
apc_gp_set_back_color( Handle self, Color color)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		prima_allocate_color( self, color, &XX-> back);
		XX-> flags. brush_back = 0;
		guts.xrender_pen_dirty = true;
	} else
		XX-> saved_back = color;
	return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		prima_allocate_color( self, color, &XX-> fore);
		XX-> flags. brush_fore = 0;
		guts.xrender_pen_dirty = true;
	} else
		XX-> saved_fore = color;
	return true;
}

static Pixmap
create_tile( Handle self, Handle image, Bool mono )
{
	DEFXX;
	Pixmap px;
	int depth, flag;
	PImage i = (PImage) image;
	ImageCache *cache;
	GC gc;
	XGCValues gcv;

	if ( mono || XT_IS_BITMAP(XX)) {
		depth = 1;
		flag = CACHE_BITMAP;
	} else if ( XF_LAYERED(XX) ) {
		depth = guts.argb_depth;
		flag = X(image)->type.icon ? CACHE_LAYERED_ALPHA : CACHE_LAYERED;
	} else {
		depth = guts.depth;
		flag = CACHE_PIXMAP;
	}

	px = XCreatePixmap( DISP, guts.root, i->w, i->h, depth);
	XCHECKPOINT;
	if ( !px ) return 0;

	if (!(cache = prima_image_cache((PImage) image, flag))) {
		XFreePixmap(DISP, px);
		return 0;
	}

	gcv.foreground = 0xffffffff;
	gcv.background = 0;
	if ( !( gc = XCreateGC( DISP, px, GCGraphicsExposures|GCForeground|GCBackground, &gcv))) {
		XFreePixmap(DISP, px);
		return 0;
	}
	XCHECKPOINT;

	prima_put_ximage( px, gc, cache->image, 0,0,0,0, i->w, i->h);
	XFreeGC(DISP, gc);

	return px;
}

Bool
apc_gp_set_fill_image( Handle self, Handle image)
{
	DEFXX;
	PImage i = (PImage) image;

	if ( !XF_IN_PAINT(XX)) return false;
	if ( i->stage != csNormal ) return false;

	cleanup_stipples(self);
	if ( i->type == imBW && !X(image)->type.icon)
		XX->fp_stipple = create_tile(self, image, 1);
	else
		XX->fp_tile    = create_tile(self, image, 0);
	XCHECKPOINT;

	guts.xrender_pen_dirty = true;

	return true;
}

Bool
apc_gp_set_fill_image( Handle self, Handle image)
{
	return false;
}

Bool
apc_gp_set_fill_mode( Handle self, int fillMode)
{
	DEFXX;
	int fill_rule;
	XGCValues gcv;

	fill_rule = ((fillMode & fmWinding) == fmAlternate) ? EvenOddRule : WindingRule;
	if ( XF_IN_PAINT(XX)) {
		gcv. fill_rule = fill_rule;
		XX-> fill_mode = fillMode;
		XChangeGC( DISP, XX-> gc, GCFillRule, &gcv);
		XCHECKPOINT;
	} else {
		XX-> gcv. fill_rule = fill_rule;
		XX-> saved_fill_mode = fillMode;
	}
	return true;
}

Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
	DEFXX;
	if ( memcmp( pattern, XX-> fill_pattern, sizeof(FillPattern)) == 0)
		return true;
	XX-> flags. brush_null_hatch =
	( memcmp( pattern, fillPatterns[fpSolid], sizeof(FillPattern)) == 0);
	memcpy( XX-> fill_pattern, pattern, sizeof( FillPattern));
	if ( XF_IN_PAINT(XX)) {
		cleanup_stipples(self);
		guts.xrender_pen_dirty = true;
	}
	return true;
}

Bool
apc_gp_set_fill_pattern_offset( Handle self, Point fpo)
{
	DEFXX;
	XGCValues gcv;

	fpo. y = 8 - fpo.y;
	XX-> fill_pattern_offset = fpo;

	if ( XF_IN_PAINT(XX)) {
		gcv. ts_x_origin = fpo. x;
		gcv. ts_y_origin = fpo. y;
		XChangeGC( DISP, XX-> gc, GCTileStipXOrigin | GCTileStipYOrigin, &gcv);
		XCHECKPOINT;
		guts.xrender_pen_dirty = true;
	} else {
		XX-> gcv. ts_x_origin = fpo. x;
		XX-> gcv. ts_y_origin = fpo. y;
	}
	return true;
}

/*- see apc_font.c
void
apc_gp_set_font( Handle self, PFont font)
*/

Bool
apc_gp_set_line_end( Handle self, int lineEnd)
{
	DEFXX;
	int cap = CapButt;
	XGCValues gcv;

	if ( lineEnd == leFlat)
		cap = CapButt;
	else if ( lineEnd == leSquare)
		cap = CapProjecting;
	else if ( lineEnd == leRound)
		cap = CapRound;

	if ( XF_IN_PAINT(XX)) {
		gcv. cap_style = cap;
		XChangeGC( DISP, XX-> gc, GCCapStyle, &gcv);
		XCHECKPOINT;
	} else {
		XX-> gcv. cap_style = cap;
	}
	return true;
}

Bool
apc_gp_set_line_join( Handle self, int lineJoin)
{
	DEFXX;
	int join = JoinRound;
	XGCValues gcv;

	if ( lineJoin == ljRound)
		join = JoinRound;
	else if ( lineJoin == ljBevel)
		join = JoinBevel;
	else if ( lineJoin == ljMiter)
		join = JoinMiter;

	if ( XF_IN_PAINT(XX)) {
		gcv. join_style = join;
		XChangeGC( DISP, XX-> gc, GCJoinStyle, &gcv);
		XCHECKPOINT;
	} else {
		XX-> gcv. join_style = join;
	}
	return true;
}

Bool
apc_gp_set_line_width( Handle self, float line_width)
{
	DEFXX;
	XGCValues gcv;

	if ( XF_IN_PAINT(XX)) {
		XX-> paint_line_width = line_width;
		gcv. line_width = line_width +.5;
		if ( !( XX-> paint_ndashes == 0 || (XX-> paint_ndashes == 1 && XX-> paint_dashes[0] == 1))) {
			dDASH_FIX( line_width, XX-> paint_dashes, XX-> paint_ndashes);
			DASH_FIX;
			XSetDashes( DISP, XX-> gc, 0, DASHES);
		}
		XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
		XCHECKPOINT;
	} else {
		XX-> gcv. line_width = line_width + .5;
		XX-> line_width = line_width;
	}
	return true;
}

Bool
apc_gp_set_line_pattern( Handle self, unsigned char *pattern, int len)
{
	DEFXX;
	XGCValues gcv;

	if ( XF_IN_PAINT(XX)) {
		if ( len == 0 || (len == 1 && pattern[0] == 1)) {
			gcv. line_style = LineSolid;
			XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
		} else {
			dDASH_FIX(XX-> line_width, pattern, len);
			DASH_FIX;
			gcv. line_style = ( XX-> paint_rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;
			XSetDashes( DISP, XX-> gc, 0, DASHES);
			XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
		}
		XX-> line_style = gcv. line_style;
		free(XX->paint_dashes);
		if (( XX-> paint_dashes = malloc( len)))
			memcpy( XX-> paint_dashes, pattern, len);
		XX-> paint_ndashes = len;
	} else {
		free( XX-> dashes);
		if ( len == 0) {					/* lpNull */
			XX-> dashes = NULL;
			XX-> ndashes = -1;
			XX-> gcv. line_style = LineSolid;
		} else if ( len == 1 && pattern[0] == 1) {	/* lpSolid */
			XX-> dashes = NULL;
			XX-> ndashes = 0;
			XX-> gcv. line_style = LineSolid;
		} else {						/* the rest */
			XX-> dashes = malloc( len);
			memcpy( XX-> dashes, pattern, len);
			XX-> ndashes = len;
			XX-> gcv. line_style = ( XX-> rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;
		}
	}
	return true;
}

Bool
apc_gp_set_miter_limit( Handle self, float miter_limit)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) return false;
	XX-> miter_limit = miter_limit;
	return true;
}

Bool
apc_gp_set_rop( Handle self, int rop)
{
	DEFXX;
	int function;

	if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
		function = GXnoop;
	else
		function = rop_map[ rop];

	if ( XF_IN_PAINT(XX)) {
		if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
			rop = ropNoOper;
		XX-> paint_rop = rop;
		XSetFunction( DISP, XX-> gc, function);
		guts.xrender_pen_dirty = true;
		XCHECKPOINT;
	} else {
		XX-> gcv. function = function;
		XX-> rop = rop;
	}
	return true;
}

Bool
apc_gp_set_rop2( Handle self, int rop)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		if ( XX-> paint_rop2 == rop) return true;
		XX-> paint_rop2 = ( rop == ropCopyPut) ? ropCopyPut : ropNoOper;
		if ( XX-> line_style != LineSolid) {
			XGCValues gcv;
			gcv. line_style = ( rop == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
			XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
		}
		guts.xrender_pen_dirty = true;
	} else {
		XX-> rop2 = rop;
		if ( XX-> gcv. line_style != LineSolid)
			XX-> gcv. line_style = ( rop == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
	}
	return true;
}

Bool
apc_gp_set_transform( Handle self, int x, int y)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		XX-> gtransform. x = x;
		XX-> gtransform. y = y;
	} else {
		XX-> transform. x = x;
		XX-> transform. y = y;
	}
	return true;
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		XX-> flags. paint_opaque = !!opaque;
	} else {
		XX-> flags. opaque = !!opaque;
	}
	return true;
}

Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
	DEFXX;
	if ( XF_IN_PAINT(XX)) {
		XX-> flags. paint_base_line = !!baseline;
	} else {
		XX-> flags. base_line = !!baseline;
	}
	return true;
}

ApiHandle
apc_gp_get_handle( Handle self)
{
	return ( ApiHandle) X(self)-> gdrawable;
}

Bool
<<<<<<< HEAD
apc_gp_push(Handle self, GCStorageFunction * destructor, void * user_data, unsigned int user_data_size)
{
	DEFXX;
	int size;
	PPaintState state;

	if ( !XX-> gc_stack ) {
		if ( !( XX-> gc_stack = plist_create(4,4))) return false;
	}
	size = sizeof(PaintState) + user_data_size;
	if ( !( state = malloc(size))) return false;
	if ( list_add( XX-> gc_stack, (Handle) state) < 0) return false;

	bzero(state, size);
	state->user_data = state->user_data_buf;
	memcpy( state-> user_data, user_data, user_data_size);
	state->user_data_size = user_data_size;
	state->user_destructor = destructor;

	state->in_paint = XF_IN_PAINT(XX);

	if ( state-> in_paint ) {
		state->antialias = XX->flags.antialias;
		state->paint.fore = XX->fore;
		state->paint.back = XX->back;
		state->fill_mode = XX->fill_mode;
		state->alpha = XX->paint_alpha;
		state->line_width = XX->paint_line_width;
		state->n_dashes = XX-> paint_ndashes;
		if (( state->dashes = malloc( XX-> paint_ndashes)) != NULL)
			memcpy( state->dashes, XX-> paint_dashes, XX->paint_ndashes);
		state->rop  = XX->paint_rop;
		state->rop2 = XX->paint_rop2;
		state->transform = XX->gtransform;
		state->text_baseline = XX->flags. paint_base_line;
		state->text_opaque   = XX->flags. paint_opaque;

		state->paint.stipple = XX->fp_stipple;
		state->paint.tile = XX->fp_tile;

		state->paint.gc  = XX-> gc;
		state->paint.gcl = XX-> gcl;
		XX->gcl = NULL;
		XX->gc = NULL;
		state->paint.gc_pool = prima_get_gc(XX);
		XCopyGC( DISP, state->paint.gc, (1 << (GCLastBit + 1)) - 1, XX->gc);
		XCHECKPOINT;

		if ( XX-> current_region ) {
			state->paint.region = XCreateRegion();
			XUnionRegion( state->paint.region, XX-> current_region, state->paint.region);
			XSetRegion( DISP, state->paint.gc, state->paint.region );
			XCHECKPOINT;
		} else
			state->paint.region = 0;
	} else {
		state->nonpaint.gcv = XX->gcv;
		state->antialias = XX->flags.saved_antialias;
		state->nonpaint.fore = XX->saved_fore;
		state->nonpaint.back = XX->saved_back;
		state->fill_mode = XX->saved_fill_mode;
		state->alpha = XX->alpha;
		state->line_width = XX->line_width;
		if (( state->n_dashes = XX-> ndashes) > 0) {
			if (( state->dashes = malloc( XX-> ndashes)) != NULL)
				memcpy( state->dashes, XX-> dashes, XX->ndashes);
		} else
			state->dashes = NULL;
		state->rop  = XX->rop;
		state->rop2 = XX->rop2;
		state->transform = XX->transform;
		state->text_baseline = XX->flags.base_line;
		state->text_opaque   = XX->flags.opaque;
	}
	state->miter_limit = XX->miter_limit;
	memcpy( state->fill_pattern, XX->fill_pattern, sizeof(FillPattern));
	state->fill_pattern_offset = XX->fill_pattern_offset;
	state->null_hatch = XX->flags.brush_null_hatch;
	state->font = PDrawable(self)->font;

	if ( PDrawable(self)->fillPatternImage )
		protect_object( state->fill_image = PDrawable(self)->fillPatternImage );
	return true;
}

Bool
apc_gp_pop( Handle self, void * user_data)
{
	DEFXX;
	PPaintState state;

	if ( !XX-> gc_stack ) return false;
	if ( XX-> gc_stack-> count <= 0 ) return false;
	if ( !( state = ( PPaintState) list_at( XX-> gc_stack, XX-> gc_stack-> count - 1))) return false;
	list_delete_at( XX-> gc_stack, XX-> gc_stack->count - 1);

	if ( user_data )
		memcpy( user_data, state->user_data, state->user_data_size);

	if ( state-> in_paint ) {
		XX->flags.antialias = state->antialias;
		XX->fore = state->paint.fore;
		XX->back = state->paint.back;
		XX-> flags. brush_fore = 0;
		XX-> flags. brush_back = 0;
		XX-> fill_mode = state->fill_mode;
		XX-> paint_alpha = state-> alpha;
		XX-> paint_line_width = state-> line_width;
		free( XX-> paint_dashes);
		XX->paint_dashes = state->dashes;
		XX->paint_ndashes = state->n_dashes;
		XX->paint_rop = state->rop;
		XX->paint_rop2 = state->rop2;
		XX->gtransform = state->transform;
		XX->flags. paint_base_line = state->text_baseline;
		XX->flags. paint_opaque = state->text_opaque;
		PDrawable(self)->font = state->font;
		apc_gp_set_font(self, &PDrawable(self)->font);

		if ( XX->fp_stipple != state->paint.stipple ) {
			if ( XX-> fp_stipple )
				XFreePixmap( DISP, XX->fp_stipple);
			XX->fp_stipple = state->paint.stipple;
		}
		if ( XX->fp_tile != state->paint.tile ) {
			if ( XX-> fp_tile )
				XFreePixmap( DISP, XX->fp_tile);
			XX->fp_tile = state->paint.tile;
		}

		prima_release_gc(XX);
		XX->gc  = state->paint.gc;
		XX->gcl = state->paint.gcl;

		if ( XX-> current_region && XX-> flags. kill_current_region )
			XDestroyRegion( XX-> current_region );
		XX-> current_region = state-> paint.region;
		if ( !state-> paint.region ) {
			XRectangle r = {0,0,XX->size.x,XX->size.y};
			XX-> current_region = XCreateRegion();
			XUnionRectWithRegion( &r, XX->current_region, XX->current_region);
		}
		XX-> flags. kill_current_region = 1;
#ifdef USE_XFT
		if ( XX-> xft_drawable) prima_xft_update_region( self);
#endif
		CLIP_ARGB_PICTURE(XX->argb_picture, XX->current_region);
		guts.xrender_pen_dirty = true;
	} else {
		XX->gcv = state->nonpaint.gcv;
		XX->flags.saved_antialias = state->antialias;
		XX->saved_fore = state->nonpaint.fore;
		XX->saved_back = state->nonpaint.back ;
		XX->saved_fill_mode = state->fill_mode;
		XX->alpha = state->alpha ;
		XX->line_width = state->line_width;
		free( XX-> dashes);
		XX->dashes = state->dashes;
		XX->ndashes = state->n_dashes;
		XX->rop  = state->rop;
		XX->rop2 = state->rop2;
		XX->transform = state->transform;
		XX->flags.base_line = state->text_baseline;
		XX->flags.opaque = state->text_opaque;
	}

	memcpy( XX->fill_pattern, state->fill_pattern, sizeof(FillPattern));
	XX-> fill_pattern_offset = state->fill_pattern_offset;
	XX-> flags.brush_null_hatch = state->null_hatch;
	XX->miter_limit = state->miter_limit;

	if (PDrawable(self)->fillPatternImage)
		unprotect_object(PDrawable(self)->fillPatternImage);
	PDrawable(self)->fillPatternImage = state-> fill_image;

	free(state);
	return true;
=======
apc_gp_push( Handle self)
{
	return false;
}

Bool
apc_gp_pop( Handle self)
{
	return false;
>>>>>>> 3e818d51 (apply graphic_context())
}

