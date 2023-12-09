/***********************************************************/
/*                                                         */
/*  System dependent color management                      */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Drawable.h"
#include "Window.h"

/* have two color layouts for panel widgets (lists, edits) and gray widgets (buttons, labels) */
#define COLOR_DEFAULT_TEXT         0x000000
#define COLOR_DEFAULT_GRAY         0xcccccc
#define COLOR_DEFAULT_BACK         0xffffff

#define COLOR_GRAY_NORMAL_TEXT     COLOR_DEFAULT_TEXT
#define COLOR_GRAY_NORMAL_BACK     COLOR_DEFAULT_GRAY
#define COLOR_GRAY_HILITE_TEXT     COLOR_DEFAULT_TEXT
#define COLOR_GRAY_HILITE_BACK     COLOR_DEFAULT_GRAY
#define COLOR_GRAY_DISABLED_TEXT   0x606060
#define COLOR_GRAY_DISABLED_BACK   0xcccccc

#define COLOR_PANEL_NORMAL_TEXT    COLOR_DEFAULT_TEXT
#define COLOR_PANEL_NORMAL_BACK    COLOR_DEFAULT_BACK
#define COLOR_PANEL_HILITE_TEXT    COLOR_DEFAULT_BACK
#define COLOR_PANEL_HILITE_BACK    COLOR_DEFAULT_TEXT
#define COLOR_PANEL_DISABLED_TEXT  0x606060
#define COLOR_PANEL_DISABLED_BACK  0xdddddd

#define COLOR_LIGHT3D              0xffffff
#define COLOR_DARK3D               0x808080

#define COLORSET_GRAY_NORMAL       COLOR_GRAY_NORMAL_TEXT,   COLOR_GRAY_NORMAL_BACK
#define COLORSET_GRAY_HILITE       COLOR_GRAY_HILITE_TEXT,   COLOR_GRAY_HILITE_BACK
#define COLORSET_GRAY_ALT_HILITE   COLOR_GRAY_HILITE_BACK,   COLOR_GRAY_HILITE_TEXT
#define COLORSET_GRAY_DISABLED     COLOR_GRAY_DISABLED_TEXT, COLOR_GRAY_DISABLED_BACK

#define COLORSET_PANEL_NORMAL      COLOR_PANEL_NORMAL_TEXT,   COLOR_PANEL_NORMAL_BACK
#define COLORSET_PANEL_HILITE      COLOR_PANEL_HILITE_TEXT,   COLOR_PANEL_HILITE_BACK
#define COLORSET_PANEL_DISABLED    COLOR_PANEL_DISABLED_TEXT, COLOR_PANEL_DISABLED_BACK

#define COLORSET_3D                COLOR_LIGHT3D, COLOR_DARK3D

#define COLORSET_GRAY              COLORSET_GRAY_NORMAL,  COLORSET_GRAY_HILITE,     COLORSET_GRAY_DISABLED,  COLORSET_3D
#define COLORSET_ALT_GRAY          COLORSET_GRAY_NORMAL,  COLORSET_GRAY_ALT_HILITE, COLORSET_GRAY_DISABLED,  COLORSET_3D
#define COLORSET_PANEL             COLORSET_PANEL_NORMAL, COLORSET_PANEL_HILITE,    COLORSET_PANEL_DISABLED, COLORSET_3D

static Color standard_button_colors[]      = { COLORSET_GRAY     };
static Color standard_checkbox_colors[]    = { COLORSET_GRAY     };
static Color standard_combo_colors[]       = { COLORSET_GRAY     };
static Color standard_dialog_colors[]      = { COLORSET_GRAY     };
static Color standard_edit_colors[]        = { COLORSET_PANEL    };
static Color standard_inputline_colors[]   = { COLORSET_PANEL    };
static Color standard_label_colors[]       = { COLORSET_GRAY     };
static Color standard_listbox_colors[]     = { COLORSET_PANEL    };
static Color standard_menu_colors[]        = { COLORSET_ALT_GRAY };
static Color standard_popup_colors[]       = { COLORSET_ALT_GRAY };
static Color standard_radio_colors[]       = { COLORSET_GRAY     };
static Color standard_scrollbar_colors[]   = { COLORSET_ALT_GRAY };
static Color standard_slider_colors[]      = { COLORSET_GRAY     };
static Color standard_widget_colors[]      = { COLORSET_ALT_GRAY };
static Color standard_window_colors[]      = { COLORSET_GRAY     };
static Color standard_application_colors[] = { COLORSET_GRAY     };

static Color* standard_colors[] = {
	NULL,
	standard_button_colors,		/* Prima.Button.* */
	standard_checkbox_colors,		/* Prima.Checkbox.* */
	standard_combo_colors,		/* Prima.Combo.* */
	standard_dialog_colors,		/* Prima.Dialog.* */
	standard_edit_colors,		/*   ...etc... */
	standard_inputline_colors,
	standard_label_colors,
	standard_listbox_colors,
	standard_menu_colors,
	standard_popup_colors,
	standard_radio_colors,
	standard_scrollbar_colors,
	standard_slider_colors,
	standard_widget_colors,
	standard_window_colors,
	standard_application_colors,
};

static const int MAX_COLOR_CLASS = sizeof(standard_colors) / sizeof( standard_colors[ 0]) - 1;

Color **
prima_standard_colors(int * n_classes)
{
	if ( n_classes ) *n_classes = MAX_COLOR_CLASS + 1;
	return standard_colors;
}

/* maps RGB or cl-constant value to RGB value.  */
Color
prima_map_color( Color clr, int * hint)
{
	long cls;
	if ( hint) *hint = COLORHINT_NONE;
	if (( clr & clSysFlag) == 0) return clr;

	cls = (clr & wcMask) >> 16;
	if ( cls <= 0 || cls > MAX_COLOR_CLASS) cls = (wcWidget) >> 16;
	if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
	if ( clr == clSet)   {
		if ( hint) *hint = COLORHINT_WHITE;
		return 0xffffff;
	} else if ( clr == clClear) {
		if ( hint) *hint = COLORHINT_BLACK;
		return 0;
	} else if ( clr == clInvalid) {
		return 0xffffff;
	} else return standard_colors[cls][(clr & clSysMask) - 1];
}

Color
apc_widget_map_color( Handle self, Color color)
{
	if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0)) color |= PWidget(self)-> widgetClass;
	return prima_map_color( color, NULL);
}

static PHash  hatches;
static Bool   kill_hatches( Pixmap pixmap, int keyLen, void * key, void * dummy);
static Bool   prima_color_new( XColor * xc);

/*
static int card[256];
static int cardi = 0;
static Bool
my_XAllocColor( Display * disp, Colormap cm, XColor * xc, int line)
{
	if ( !cardi) {
		cardi = 1;
		bzero( card, 256*sizeof(int));
	}
	if ( !XAllocColor(disp, cm, xc)) {
		printf("Failed alloc of %02x%02x%02x, at %d\n",
			xc-> red>>8, xc-> green>>8, xc-> blue>>8, line);
		return false;
	}
	printf("Alloc %02x%02x%02x, at %d: %d\n",
			xc-> red>>8, xc-> green>>8, xc-> blue>>8,
			line, xc-> pixel);
	card[xc-> pixel]++;
	return true;
}

static void
my_XFreeColors( Display * disp, Colormap cm, long * ls, int count, long pal, int line)
{
	XSynchronize( DISP, true);
	printf("Free at %d:%d items\n", line, count);
	for ( pal = 0; pal < count; pal++, ls++) {
		printf("%ld.", *ls);
		XFreeColors( disp, cm, ls, 1, 0);
		XSync( disp, false);
		if ( !card[*ls]) printf("NO CARD!\n");
		else card[*ls]--;
	}
	printf("done\n");
}

#define XAllocColor(a,b,c) my_XAllocColor(a,b,c,__LINE__)
#define XFreeColors(a,b,c,d,e) my_XFreeColors(a,b,c,d,e,__LINE__)
*/

static Bool
alloc_color( XColor * c)
{
	int diff = 5 * 256,
	r = c-> red,
	g = c-> green,
	b = c-> blue;
	if ( !XAllocColor( DISP, guts. defaultColormap, c)) return false;
	if (
		diff > abs( c-> red   - r) &&
		diff > abs( c-> green - g) &&
		diff > abs( c-> blue  - b)
	) return true;
	XFreeColors( DISP, guts. defaultColormap, &c->pixel, 1, 0);
	return false;
}

/*
	Fills Brush structure. If dithering is needed,
brush.secondary and brush.balance are set. Tries to
get new colors via XAllocColor, assigns new color cells
to self if successful.
	If no brush structure is given, no dithering is
preformed.
	Returns closest matching color, always the same as
brush-> primary.
*/
unsigned long
prima_allocate_color( Handle self, Color color, Brush * brush)
{
	DEFXX;
	Brush b;
	int a[3], hint;

	if ( !brush) brush = &b;
	brush-> balance = 0;
	brush-> color = color = prima_map_color( color, &hint);

	if ( hint)
		return ( brush-> primary = (( hint == COLORHINT_BLACK) ? LOGCOLOR_BLACK : LOGCOLOR_WHITE));

	a[0] = COLOR_R(color);
	a[1] = COLOR_G(color);
	a[2] = COLOR_B(color);

	if ( self && XF_LAYERED(XX) )
		return brush->primary = DEV_RGB(&guts.argb_bits,a[0],a[1],a[2]);

	if ( guts. grayScale) {
		a[0] = a[1] = a[2] = ( a[0] + a[1] + a[2]) / 3;
		color = a[0] * ( 65536 + 256 + 1);
	}
	Pdebug("color: %s asked for %06x", self?PWidget(self)->name:"null", color);
	if (self && XT_IS_BITMAP(XX)) {
		Byte balance = ( a[0] + a[1] + a[2] + 6) / (3 * 4);
		if ( balance < 64) {
			brush-> primary   = 0;
			brush-> secondary = 1;
			brush-> balance   = balance;
		} else
			brush-> primary = 1;
	} else {
		if ( guts. palSize > 0) {
			int ab2;
			Bool dyna = guts. dynamicColors && self && X(self)-> type. widget && ( self != prima_guts.application);
			brush-> primary = prima_color_find( self, color, -1, &ab2, RANK_FREE);

			if ( dyna && ab2 > 12) {
				XColor xc;
				xc. red   = COLOR_R16(color);
				xc. green = COLOR_G16(color);
				xc. blue  = COLOR_B16(color);
				xc. flags = DoGreen | DoBlue | DoRed;
				prima_color_sync();
				if ( alloc_color( &xc)) {
					prima_color_new( &xc);
					Pdebug("color: %s alloc %d ( wanted %06x). got %02x %02x %02x", PWidget(self)-> name, xc.pixel, color, xc.red>>8,xc.green>>8,xc.blue>>8);
					prima_color_add_ref( self, xc. pixel, RANK_NORMAL);
					return brush-> primary = xc. pixel;
				}
			}

			if ( guts. useDithering && (brush != &b) && (ab2 > 12)) {
				if ( guts. grayScale && guts. systemColorMapSize > guts. palSize / 2) {
					int clr = ( COLOR_R(color) + COLOR_G(color) +
							COLOR_B(color)) / 3;
					int grd  = 256 / ( guts. systemColorMapSize - 1);
					int left = clr / grd;
					brush-> balance = ( clr - left * grd) * 64 / grd;
					brush-> primary = guts. systemColorMap[ left];
					brush-> secondary = guts. systemColorMap[ left + 1];
				} else {
					int i;
					Bool cubic = (XX-> type.dbm && guts. dynamicColors) ||
									( guts. colorCubeRib > 4);
DITHER:
					if ( cubic) {
/* fast search of dithering colors - based on color cube.
	used either on restricted drawables ( as dbm) or when
	have enough colors - small cubes give noisy picture

      .  .  .  *R"G"   assume here that blue component does not require dithering
  R |                  R'G' and R"G" are 2 colors blended with proportion to make
    | '''* A           color A. R'G' is a closest cubic color. If A(G)/A(R) < y,
    |    |             R"G" is G-point, if A(G)/A(R) > 1 + y, it's R-point, otherwise
    *---------- G      it's RG-point. (y=sqrt(2)-1=0.41; y=0.41x and y=1.41x are
R'G' , B=0             maximal error lines). balance is computed as diff between
                       R'G' and R"G"
*/
						int base[3], l[3], z[3], r[3], cnt = 0, sum = 0;
						int grd = 256 / ( guts. colorCubeRib - 1);
						for ( i = 0; i < 3; i++) {
							base[i] = a[i] / grd;
							r[i] = l[i] = ( a[i] >= base[i] + grd / 2) ? 1 : 0;
							z[i] = l[i] ? (base[i] + 1) * grd - a[i]: a[i] - base[i] * grd;
						}
						if ( z[1] > 1) {
							int ratio1 = 100 * z[0] / z[1];
							int ratio2 = 100 * z[2] / z[1];
							if ( ratio1 > 59)  r[0] = r[0] ? 0 : 1;
							if ( ratio2 > 59)  r[2] = r[2] ? 0 : 1;
							if ( ratio1 < 141 && ratio2 < 141) r[1] = r[1] ? 0 : 1;
						}  else if ( z[2] > 1) {
							int ratio = 100 * z[0] / z[2];
							if ( ratio > 59) r[0] = r[0] ? 0 : 1;
							if ( ratio < 141) r[2] = r[2] ? 0 : 1;
						} else if ( z[0] > 1) {
							r[0] = r[0] ? 0 : 1;
						}
						for ( i = 0; i < 3; i++)
							if ( r[i] != l[i]) {
								sum += z[i];
								cnt++;
							}
						brush-> primary = guts. systemColorMap[
							l[2] + base[2] +
						( l[1] + base[1]) * guts.colorCubeRib +
						( l[0] + base[0]) * guts.colorCubeRib * guts.colorCubeRib];
						brush-> secondary = guts. systemColorMap[
							r[2] + base[2] +
						( r[1] + base[1]) * guts.colorCubeRib +
						( r[0] + base[0]) * guts.colorCubeRib * guts.colorCubeRib];
						brush-> balance = cnt ? (sum / cnt) * 64 / grd : 0;
					} else {
/*  slow search for dithering counterpart color; takes long time
	but gives closest possible colors.

           A*          A - color to be expressed by B and D
           /|  .       B - closest color
          / |    .     D - candidate color
         /  |      .   C - closest color that can be expressed using B and D
*---*-------*  The objective is to find such D whose AC is minimal and
B   C       D  CD>BD. ( CD = (AD*AD-AB*AB+BD*BD)/2BD, AC=sqrt(AD*AD-CD*CD))
*/
						int b[3], d[3], i;
						int ab2, bd2, ac2, ad2;
						float cd, bd, BMcd=0, BMbd=0;
						int maxDiff = 16777216, bestMatch = -1;
						int mincd = maxDiff;
						b[0] = guts. palette[brush-> primary].r;
						b[1] = guts. palette[brush-> primary].g;
						b[2] = guts. palette[brush-> primary].b;
						Pdebug("color:want %06x, closest is %06x", color, guts.palette[brush-> primary].composite);
						ab2 = (a[0]-b[0])*(a[0]-b[0]) +
								(a[1]-b[1])*(a[1]-b[1]) +
								(a[2]-b[2])*(a[2]-b[2]);
						for ( i = 0; i < guts.palSize; i++) {
							if ( guts.palette[i].rank == RANK_FREE) continue;
							d[0] = guts. palette[i].r;
							d[1] = guts. palette[i].g;
							d[2] = guts. palette[i].b;
							Pdebug("color:tasting %06x", guts.palette[i].composite);
							bd2 = (d[0]-b[0])*(d[0]-b[0]) +
									(d[1]-b[1])*(d[1]-b[1]) +
									(d[2]-b[2])*(d[2]-b[2]);
							bd  = sqrt( bd2);
							if ( bd == 0) continue;
							ad2 = (d[0]-a[0])*(d[0]-a[0]) +
									(d[1]-a[1])*(d[1]-a[1]) +
									(d[2]-a[2])*(d[2]-a[2]);
							cd  = ( ad2 - ab2 + bd2) / (2 * bd);
							Pdebug("color:bd:%g,bd2:%d, ad2:%d, cd:%g", bd, bd2, ad2, cd);
							if ( cd < bd) {
								ac2 = ad2 - cd * cd;
								Pdebug("color:ac2:%d", ac2);
								if ( ac2 < maxDiff || (( ac2 < maxDiff + 12) && (cd < mincd))) {
									maxDiff = ac2;
									bestMatch = i;
									BMcd = cd;
									BMbd = bd;
									mincd = cd;
									if ( mincd < 42) goto ENOUGH;
								}
							}
						}
ENOUGH:;
						if ( !guts. grayScale && maxDiff > (64/(guts.colorCubeRib-1))) {
							cubic = true;
							goto DITHER;
						}
						brush-> secondary = bestMatch;
						brush-> balance   = 63 - BMcd * 64 / BMbd;
					}
				}
			}

			if ( dyna) {
				if ( wlpal_get( self, brush-> primary) == RANK_FREE)
					prima_color_add_ref( self, brush-> primary, RANK_NORMAL);
				if (( brush-> balance > 0) &&
					( wlpal_get( self, brush->secondary) == RANK_FREE))
					prima_color_add_ref( self, brush-> secondary, RANK_NORMAL);
			}
		} else
			brush-> primary = DEV_RGB(&guts.screen_bits, a[0],a[1],a[2]);
	}
	return brush-> primary;
}


static Bool
alloc_main_color_range( XColor * xc, int count, int maxDiff)
{
	int idx;
	Bool err = false;

	if ( count > guts. palSize) return false;

	for ( idx = 0; idx < count; idx++)
		xc[idx]. pixel = 0xFFFFFFFF;

	for ( idx = 0; idx < count; idx++) {
		int R = xc[idx]. red;
		int G = xc[idx]. green;
		int B = xc[idx]. blue;
		if ( !XAllocColor( DISP, guts. defaultColormap, &xc[idx])) {
			err = true;
			break;
		}
		if ( xc[idx]. pixel >= guts. palSize) {
			warn("color index out of range returned from XAllocColor()\n");
			return false;
		}
		if (( xc[idx]. blue / 256 - B / 256) * ( xc[idx]. blue / 256 - B / 256) +
			( xc[idx]. green / 256 - G / 256) * ( xc[idx]. green / 256 - G / 256) +
			( xc[idx]. red / 256 - R / 256) * ( xc[idx]. red / 256 - R / 256) >
			maxDiff) {
			err = true;
			break;
		}
	}

	if ( err) {
		unsigned long cnt = 0, free[32];
		for ( idx = 0; idx < count; idx++)
			if ( xc[idx]. pixel != 0xFFFFFFFF) {
				free[ cnt++] = xc[idx]. pixel;
				if ( cnt == 32) {
					XFreeColors( DISP, guts. defaultColormap, free, 32, 0);
					cnt = 0;
				}
			}
		if ( cnt > 0)
			XFreeColors( DISP, guts. defaultColormap, free, cnt, 0);
		return false;
	}
	return true;
}

static Bool
create_std_palettes( XColor * xc, int count)
{
	int idx;
	if ( !( guts. palette = malloc( sizeof( MainColorEntry) * guts. palSize)))
		return false;
	if ( !( guts. systemColorMap = malloc( sizeof( int) * count))) {
		free( guts. palette);
		guts. palette = NULL;
		return false;
	}
	bzero( guts. palette, sizeof( MainColorEntry) * guts. palSize);

	for ( idx = 0; idx < guts. palSize; idx++) {
		guts. palette[ idx]. rank = RANK_FREE;
		list_create( &guts. palette[idx]. users, 0, 16);
	}

	for ( idx = 0; idx < count; idx++) {
		int pixel = xc[idx]. pixel;
		guts. palette[ pixel]. r = xc[idx]. red / 256;
		guts. palette[ pixel]. g = xc[idx]. green / 256;
		guts. palette[ pixel]. b = xc[idx]. blue / 256;
		guts. palette[ pixel]. composite = RGB_COMPOSITE(
			guts. palette[ pixel]. r,
			guts. palette[ pixel]. g,
			guts. palette[ pixel]. b);
		guts. palette[ pixel]. rank = RANK_IMMUTABLE;
		guts. systemColorMap[ idx] = pixel;
	}

	guts. systemColorMapSize = count;

	return true;
}

static void
fill_cubic( XColor * xc, int d)
{
	int b, g, r, d2 = d * d, frac = 65535 / ( d - 1);
	for ( b = 0; b < d; b++) for ( g = 0; g < d; g++) for ( r = 0; r < d; r++) {
		int idx = b + g * d + r * d2;
		xc[idx]. blue = b * frac;
		xc[idx]. green = g * frac;
		xc[idx]. red = r * frac;
	}
}

static char * do_visual = NULL;
static PList color_options = NULL;

static void
set_color_class( int class, char * option, char * value)
{
	if ( !value) {
		warn("`%s' must be given a value -- skipped\n", option);
		return;
	}
	if ( !color_options) color_options = plist_create( 8, 8);
	if ( !color_options) return;
	list_add( color_options, ( Handle) class);
	list_add( color_options, ( Handle) duplicate_string(value));
}

static void
apply_color_class( int c_class, Color value)
{
	int i;
	Color ** t = standard_colors + 1;
	for ( i = 1; i < MAX_COLOR_CLASS; i++, t++) (*t)[c_class] = value;
	Mdebug("color: class %d=%06x", c_class, value);
}

/* find color bounds and test if they are contiguous */
Bool
prima_find_color_mask_range( unsigned long mask, unsigned int * shift, unsigned int * range)
{
	int i, from = 0, to = 0, stage = 0;
	for ( i = 0; i < 32; i++) {
		switch ( stage) {
		case 0:
			if (( mask & ( 1 << i)) != 0) {
				from = i;
				stage++;
			}
			break;
		case 1:
			if (( mask & ( 1 << i)) == 0) {
				to = i;
				stage++;
			}
			break;
		case 2:
			if (( mask & ( 1 << i)) != 0) {
				warn( "panic: unsupported pixel representation (0x%08lx)", mask);
				return false;
			}
		}
	}
	if ( to == 0) to = 32;
	*shift   = from;
	*range   = to - from;
	return true;
}

Bool
prima_init_color_subsystem(char * error_buf)
{
	int id, count, mask = VisualScreenMask|VisualDepthMask|VisualIDMask;
	XVisualInfo template, *list = NULL;

	/* check if non-default depth is selected */
	id = -1;
	{
		char * c, * end;
		if (( c = do_visual) ||
			apc_fetch_resource( "Prima", "", "Visual", "visual",
										NULL_HANDLE, frString, &c)) {
			id = strtol( c, &end, 0);
			if ( *end) id = -1;
			if ( c != do_visual) free( c);
			template. visualid = id;
			list = XGetVisualInfo( DISP, VisualIDMask, &template, &count);
			if ( count <= 0) {
				warn("warning: visual id '%s' is not found\n", c);
				if ( list) XFree( list);
				id = -1;
			}
		}
	}
	free( do_visual);
	do_visual = NULL;

FALLBACK_TO_DEFAULT_VISUAL:
	if ( id < 0) {
		template. screen   = SCREEN;
		template. depth    = guts. depth;
		template. visualid = XVisualIDFromVisual( XDefaultVisual( DISP, SCREEN));

		list = XGetVisualInfo( DISP, mask, &template, &count);
		if ( count == 0) {
			sprintf( error_buf, "panic: no visuals found");
			return false;
		}
	}

	guts. visual = list[0];
	guts. visualClass = guts. visual.
#if defined(__cplusplus) || defined(c_plusplus)
c_class;
#else
class;
#endif
	XFree( list);

	if ( guts. depth > 11 && guts. visualClass != TrueColor && guts. visualClass != DirectColor) {/* XXX */
		if ( id >= 0) {
			warn("warning: visual 0x%x is unusable: cannot use %d bit depth for something not TrueColor or DirectColor\n", id, guts. depth);
			id = -1;
			goto FALLBACK_TO_DEFAULT_VISUAL;
		} else
			sprintf( error_buf, "panic: %d bit depth is not true color", guts. depth);
		return false;
	}


	guts. useDithering     = true;
	guts. dynamicColors    = false;
	guts. grayScale        = false;
	guts. palSize          = 1 << guts. depth;
	guts. palette          = NULL;
	guts. systemColorMap   = NULL;
	guts. systemColorMapSize = 0;
	guts. colorCubeRib     = 0;

	if ( id >= 0) {
		guts. defaultColormap = XCreateColormap( DISP, guts. root, guts.visual.visual, AllocNone);
		guts. privateColormap = 1;
	} else
		guts. defaultColormap  = DefaultColormap( DISP, SCREEN);

	guts. monochromeMap[0] = BlackPixel( DISP, SCREEN);
	guts. monochromeMap[1] = WhitePixel( DISP, SCREEN);
	switch ( guts. visualClass) {
	case DirectColor:
	case TrueColor:
		guts. useDithering = false;
		guts. palSize = 0;
		break;

	case PseudoColor:
		{
			int max;
			XColor xc[216];
			guts. dynamicColors = true;
			if ( guts. privateColormap && guts. palSize > 26 ) {
				if ( guts. palSize > 125) max = 6; else
				if ( guts. palSize > 64)  max = 5; else
				if ( guts. palSize > 27)  max = 4; else max = 3;
			} else
				max = 2;
			fill_cubic( xc, max);
			if ( !alloc_main_color_range( xc, max * max * max, 27))
				goto BLACK_WHITE;
			if ( !create_std_palettes( xc, max * max * max)) {
				sprintf( error_buf, "No memory");
				return false;
			}
			guts. colorCubeRib = max;
		}
		break;

	case StaticColor:
		{
			int d = 6, cd = 1;
			XColor xc[216];
			while ( d > 1) {
				if ( d * d * d <= guts. palSize) {
					fill_cubic( xc, d);
					if ( alloc_main_color_range( xc, d * d * d, cd * 3)) {
						if ( !create_std_palettes( xc, d * d * d)) {
							sprintf( error_buf, "No memory");
							return false;
						}
						break;
					}
				}
				d--;
				cd += 2;
			}
			if ( !guts. palette) goto BLACK_WHITE;
			if ( d < 2) goto BLACK_WHITE_ALLOCATED;
			guts. colorCubeRib = d;
		}
		break;

	case StaticGray:
	case GrayScale:
		{
			XColor xc[256];
			int maxSteps = ( guts. visualClass == GrayScale && !guts. privateColormap ) ? 5 : 8;
			int wantSteps = ( maxSteps > guts. depth) ? guts. depth : maxSteps;
			while ( wantSteps > 1) {
				int i, shades = 1 << wantSteps, c = 0, ndiv = 65536 / (shades-1);
				for ( i = 0; i < shades; i++) {
					xc[i].red = xc[i]. green = xc[i]. blue = c;
					if (( c += ndiv) > 65535) c = 65535;
				}
				if ( alloc_main_color_range( xc, shades, 768 / shades)) {
					if ( !create_std_palettes( xc, shades)) {
						sprintf( error_buf, "No memory");
						return false;
					}
					break;
				}
				wantSteps--;
			}
			if ( !guts. palette) goto BLACK_WHITE;
			if ( wantSteps < 1) goto BLACK_WHITE_ALLOCATED;
			if ( wantSteps > 4) guts. useDithering = false;
			guts. colorCubeRib = wantSteps;
			guts. grayScale = true;
		}
		break;
	default:
BLACK_WHITE:
		{
			XColor xc[2];
			if ( guts. privateColormap) {
				xc[0]. red = xc[0]. green = xc[0]. blue = 0;
				xc[1]. red = xc[1]. green = xc[1]. blue = 0xffff;
				XAllocColor( DISP, guts. defaultColormap, xc);
				XAllocColor( DISP, guts. defaultColormap, xc + 1);
				guts. monochromeMap[0] = xc[0].pixel;
				guts. monochromeMap[1] = xc[1].pixel;
			} else {
				xc[0]. pixel = BlackPixel( DISP, SCREEN);
				xc[1]. pixel = WhitePixel( DISP, SCREEN);
				XQueryColors( DISP, guts. defaultColormap, xc, 2);
			}
			XCHECKPOINT;
			if ( !alloc_main_color_range( xc, 2, 65536) ||
				!create_std_palettes( xc, 2)) {
				sprintf( error_buf, "panic: unable to initialize color system");
				return false;
			}
		}
BLACK_WHITE_ALLOCATED:
		guts. useDithering = true;
		guts. grayScale    = true;
		guts. colorCubeRib = 1;
		break;
	}

	if ( guts. palSize > 0 &&
		(guts. visualClass == StaticColor ||
		guts. visualClass == StaticGray)) {
		int i;
		XColor * xc;
		MainColorEntry * p;
		if ( !( xc = malloc( sizeof( XColor) * guts. palSize))) {
			sprintf( error_buf, "No memory");
			return false;
		}
		for ( i = 0; i < guts. palSize; i++) xc[i]. pixel = i;
		XQueryColors( DISP, guts. defaultColormap, xc, guts. palSize);
		XCHECKPOINT;
		p = guts. palette;
		for ( i = 0; i < guts. palSize; i++) {
			p-> r = xc[i]. red / 256;
			p-> g = xc[i]. green / 256;
			p-> b = xc[i]. blue / 256;
			p-> composite = RGB_COMPOSITE( p-> r, p-> g, p-> b);
			if ( p-> rank == RANK_FREE) p-> rank  = RANK_IMMUTABLE + 1;
			p++;
		}
		free( xc);
	}

	if (( guts. ditherPatterns = malloc( sizeof( FillPattern) * 65))) {
		int i, x, y;
		FillPattern * p = guts. ditherPatterns;
		Byte map [64] = {
			0, 48, 12, 60,  3, 51, 15, 63,
			32, 16, 44, 28, 35, 19, 47, 31,
			8, 56,  4, 52, 11, 59,  7, 55,
			40, 24, 36, 20, 43, 27, 39, 23,
			2, 50, 14, 62,  1, 49, 13, 61,
			34, 18, 46, 30, 33, 17, 45, 29,
			10, 58,  6, 54,  9, 57,  5, 53,
			42, 26, 38, 22, 41, 25, 37, 21
		};
		bzero( p, sizeof( FillPattern) * 65);
		for ( i = 0; i < 65; i++, p++)
			for ( y = 0; y < 8; y++)
				for ( x = 0; x < 8; x++)
					if ( i <= map[y * 8 + x])
						(*p)[y] |= 1 << x;

	} else {
		sprintf( error_buf, "No memory");
		return false;
	}
	if ( guts. palSize == 0) {
		prima_find_color_mask_range( guts. visual. red_mask,   &guts. screen_bits. red_shift,   &guts. screen_bits. red_range);
		prima_find_color_mask_range( guts. visual. green_mask, &guts. screen_bits. green_shift, &guts. screen_bits. green_range);
		prima_find_color_mask_range( guts. visual. blue_mask,  &guts. screen_bits. blue_shift,  &guts. screen_bits. blue_range);
		guts. screen_bits. red_mask   = guts. visual. red_mask;
		guts. screen_bits. green_mask = guts. visual. green_mask;
		guts. screen_bits. blue_mask  = guts. visual. blue_mask;
	}
	guts. localPalSize = guts. palSize / 4 + ((guts. palSize % 4) ? 1 : 0);
	hatches = hash_create();

	/* get XRDB colors */
	{
		Color c;
		if ( apc_fetch_resource( "Prima", "", "Color", "color", NULL_HANDLE, frColor, &c))
			apply_color_class( ciFore, c);
		if ( apc_fetch_resource( "Prima", "", "Back", "backColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciBack, c);
		if ( apc_fetch_resource( "Prima", "", "HiliteColor", "hiliteColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciHiliteText, c);
		if ( apc_fetch_resource( "Prima", "", "HiliteBackColor", "hiliteBackColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciHilite, c);
		if ( apc_fetch_resource( "Prima", "", "DisabledColor", "disabledColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciDisabledText, c);
		if ( apc_fetch_resource( "Prima", "", "DisabledBackColor", "disabledBackColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciDisabled, c);
		if ( apc_fetch_resource( "Prima", "", "Light3DColor", "light3DColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciLight3DColor, c);
		if ( apc_fetch_resource( "Prima", "", "Dark3DColor", "dark3DColor", NULL_HANDLE, frColor, &c))
			apply_color_class( ciDark3DColor, c);
	}


	/* parse user colors */
	if ( color_options) {
		int i, c_class;
		char *value;
		XColor xcolor;
		for ( i = 0; i < color_options-> count; i+=2) {
			c_class = (int)   color_options-> items[i];
			value   = (char*) color_options-> items[i+1];
			if ( XParseColor( DISP, DefaultColormap( DISP, SCREEN), value, &xcolor)) {
				apply_color_class( c_class, ARGB(xcolor.red >> 8, xcolor.green >> 8, xcolor.blue >> 8));
			} else {
				warn("Cannot parse color value `%s`", value);
			}
			free( value);
		}
		plist_destroy( color_options);
	}

	return true;
}

Bool
prima_color_subsystem_set_option( char * option, char * value)
{
	if ( strcmp( option, "visual") == 0) {
		if ( value) {
			free( do_visual);
			do_visual = duplicate_string( value);
			Mdebug( "set visual: %s", do_visual);
		} else
			warn("`--visual' must be given value");
		return true;
	} else if ( strcmp( option, "fg") == 0) {
		set_color_class( ciFore, option, value);
	} else if ( strcmp( option, "bg") == 0) {
		set_color_class( ciBack, option, value);
	} else if ( strcmp( option, "hilite-bg") == 0) {
		set_color_class( ciHilite, option, value);
	} else if ( strcmp( option, "hilite-fg") == 0) {
		set_color_class( ciHiliteText, option, value);
	} else if ( strcmp( option, "disabled-bg") == 0) {
		set_color_class( ciDisabled, option, value);
	} else if ( strcmp( option, "disabled-fg") == 0) {
		set_color_class( ciDisabledText, option, value);
	} else if ( strcmp( option, "light") == 0) {
		set_color_class( ciLight3DColor, option, value);
	} else if ( strcmp( option, "dark") == 0) {
		set_color_class( ciDark3DColor, option, value);
	}
	return false;
}

typedef struct
{
	int count;
	unsigned long free[256];
} FreeColorsStruct;

void
prima_done_color_subsystem( void)
{
	int i;
	FreeColorsStruct fc;

	if ( DISP) {
		hash_first_that( hatches, (void*)kill_hatches, NULL, NULL, NULL);
		fc. count = 0;

		for ( i = 0; i < guts. palSize; i++) {
			list_destroy( &guts. palette[i]. users);
			if (
				!guts. privateColormap &&
				guts. palette[i]. rank > RANK_FREE &&
				guts. palette[i]. rank <= RANK_IMMUTABLE) {
				fc. free[ fc. count++] = i;
				if ( fc. count == 256) {
					XFreeColors( DISP, guts. defaultColormap, fc. free, 256, 0);
					fc. count = 0;
				}
			}
		}
		if ( fc. count > 0)
			XFreeColors( DISP, guts. defaultColormap, fc. free, fc. count, 0);
		XFreeColormap( DISP, guts. defaultColormap);
	}

	hash_destroy( hatches, false);
	guts. defaultColormap = 0;
	free( guts. ditherPatterns);
	free( guts. palette);
	free( guts. systemColorMap);
	guts. palette = NULL;
	guts. systemColorMap = NULL;
	guts. ditherPatterns = NULL;
}

/*
	Finds closest possible color in system palette.
	Colors can be selectively filtered using maxRank
	parameter - if it is greater that RANK_FREE, the colors
	with rank lower that maxRank are not matched. Ranking can
	make sense when self != NULL and self != application, and
	of course when color cell manipulation is possible. In other
	words, local palette is never used if maxRank > RANK_FREE.
	maxDiff tells the maximal difference for a color. If
	no color is found that is closer than maxDiff, -1 is returned
	and pointer to actual diff is returned.
	*/
int
prima_color_find( Handle self, long color, int maxDiff, int * diff, int maxRank)
{
	int i, j, ret = -1;
	int global;
	int b = color & 0xff;
	int g = (color >> 8) & 0xff;
	int r = (color >> 16) & 0xff;
	int lossy = maxDiff != 0;
	if ( maxDiff < 0) maxDiff = 256 * 256 * 3;
	global = self ? (X(self)-> type. widget && ( self != prima_guts.application)) : true;

	maxDiff++;
	if ( global || !guts. dynamicColors || (maxRank > RANK_FREE)) {
		for ( i = 0; i < guts. palSize; i++) {
			if ( guts. palette[i]. rank > maxRank) {
				if ( lossy) {
					int d =
						( b - guts. palette[i].b) * ( b - guts. palette[i].b) +
						( g - guts. palette[i].g) * ( g - guts. palette[i].g) +
						( r - guts. palette[i].r) * ( r - guts. palette[i].r);
					if ( d < maxDiff) {
						ret = i;
						maxDiff = d;
						if ( maxDiff == 0) break;
					}
				} else {
					if ( color == guts. palette[i]. composite) {
						ret = i;
						break;
					}
				}
			}
		}
	} else {
		for ( j = 0; j < guts. systemColorMapSize + guts. palSize; j++) {
			if ( j < guts. systemColorMapSize)
				i = guts. systemColorMap[j];
			else {
				i = j - guts. systemColorMapSize;
				if ( wlpal_get( self, i) == RANK_FREE) continue;
			}
			if ( lossy) {
				int d =
					( b - guts. palette[i].b) * ( b - guts. palette[i].b) +
					( g - guts. palette[i].g) * ( g - guts. palette[i].g) +
					( r - guts. palette[i].r) * ( r - guts. palette[i].r);
				if ( d < maxDiff) {
					ret = i;
					maxDiff = d;
					if ( maxDiff == 0) break;
				}
			} else {
				if ( color == guts. palette[i]. composite) {
					ret = i;
					break;
				}
			}
		}
	}
	if ( diff) *diff = maxDiff;
	return ret;
}

static Bool
prima_color_new( XColor * xc)
{
	MainColorEntry * p = guts. palette + xc-> pixel;
	if ( p-> rank != RANK_FREE) {
		XFreeColors( DISP, guts. defaultColormap, &xc-> pixel, 1, 0);
		return false;
	}
	p-> r = xc-> red >> 8;
	p-> g = xc-> green >> 8;
	p-> b = xc-> blue >> 8;
	p-> composite = RGB_COMPOSITE(p->r,p->g,p->b);
	return true;
}

/*
	Adds reference to widget that is responsible
	for a color cell with given rank. Main palette
	rank can be risen in response, but not lowered -
	that is accomplished by prima_color_sync.
	*/
Bool
prima_color_add_ref( Handle self, int index, int rank)
{
	int r, nr = (rank == RANK_PRIORITY) ? 2 : 1;
	if ( index < 0 || index >= guts. palSize) return false;
	if ( guts. palette[index]. rank == RANK_IMMUTABLE) return false;
	if ( !self || ( self == prima_guts.application)) return false;
	r = wlpal_get(self, index);
	if ( r != 0 && r <= nr) return false;
	if ( r == 0) list_add( &guts. palette[index]. users, self);
	if ( rank > guts. palette[index]. rank)
		guts. palette[index]. rank = rank;
	wlpal_set( self, index, nr);
	Pdebug("color:%s %s %d %d", PWidget(self)-> name, r ? "raised to " : "added as", nr, index);
	return true;
}

/* Frees stale color references */
int
prima_color_sync( void)
{
	int i, count = 0, freed = 0;
	unsigned long free[32];
	MainColorEntry * p = guts. palette;
	for ( i = 0; i < guts. palSize; i++, p++) {
		if ( p-> touched) {
			int j, max = RANK_FREE;
			for ( j = 0; j < p-> users. count; j++) {
				int rank;
				if ( X(p-> users. items[j])-> type. widget) {
					rank = wlpal_get( p-> users. items[j], i);
					if ( rank > 0)
						rank = ( rank > 1) ? RANK_PRIORITY : RANK_NORMAL;
				} else
					rank = RANK_LOCKED;
				if ( max < rank) max = rank;
				if ( max == RANK_LOCKED) break;
			}
			p-> rank = max;
			if ( max == RANK_FREE) {
				free[ count++] = i;
				if ( count == 32) {
					XFreeColors( DISP, guts. defaultColormap, free, 32, 0);
					count = 0;
					freed += 32;
				}
			}
			p-> touched = false;
		}
	}
	if ( count > 0)
		XFreeColors( DISP, guts. defaultColormap, free, count, 0);
	return freed + count;
}

/* updates contents of DefaultColormap.  */
/* NB - never to be called with 'fast' set to true. */

Bool
prima_palette_replace( Handle self, Bool fast)
{
	DEFXX;
	Bool restricted = fast || XX-> type. dbm;
	int rank, psz, i, j, granted, stage, menu = 0;
	unsigned long * req;
	RGBColor * rqx;
	MainColorEntry * p;
	List widgets;

	if ( !guts. dynamicColors) return true;
	if ( self == prima_guts.application) return true;

	if ( XX-> type.widget) rank = RANK_PRIORITY; else
	if ( XX-> type.image || XX-> type. dbm) rank = RANK_LOCKED; else
		return false;

	if ( !fast) prima_palette_free( self, true); /* remove old entries */

	psz = PDrawable( self)-> palSize + menu;
	if ( XT_IS_WINDOW(X(self)) && PWindow(self)-> menu)
		psz += (menu = ciMaxId + 1);
	if ( psz == 0) {
		prima_color_sync();
		return true;
	}

	if ( !( req = malloc( sizeof( unsigned long) * psz)))
		return false;

	for ( i = 0; i < psz - menu; i++)
		req[i] = RGB_COMPOSITE(
			PWidget( self)-> palette[i].r,
			PWidget( self)-> palette[i].g,
			PWidget( self)-> palette[i].b);
	for ( i = psz - menu; i < psz; i++)
		req[i] = PWindow(self)-> menuColor[ i - psz + menu];

	granted = 0;

	if ( !restricted) XGrabServer( DISP);

	/* fetch actual colors - they are useful when no free colorcells
		available, but colormap has some good colors, which we don't
		possess */
	if ( !restricted) {
		int count = 0, j;
		XColor xc[32];
		for ( i = 0; i < guts.palSize; i++)
			if ( guts.palette[i].rank == RANK_FREE) {
				xc[count++].pixel = i;
				if ( count == 32) {
					XQueryColors( DISP, guts. defaultColormap, xc, 32);
					for ( j = 0; j < 32; j++) prima_color_new( &xc[j]);
					count = 0;
				}
			}
		if ( count > 0) {
			XQueryColors( DISP, guts. defaultColormap, xc, count);
			for ( j = 0; j < count; j++) prima_color_new( &xc[j]);
		}
	}

	Pdebug("color replace:%s find match for %d colors", PWidget(self)-> name, psz);
	/* find out if any allocated entries are present already */
	for ( i = 0; i < psz; i++)
		if (( req[i] & 0x80000000) == 0) {
			unsigned long c = req[i];
			for ( j = 0; j < guts. palSize; j++) {
				MainColorEntry * p = guts. palette + j;
				int pixel = j;
				if ( p-> composite == c) {
					if ( !restricted && (p-> rank == RANK_FREE)) {
						XColor xc;
						xc. red   = COLOR_R16(req[i]);
						xc. green = COLOR_G16(req[i]);
						xc. blue  = COLOR_B16(req[i]);
						if ( alloc_color(&xc)) {
							if ( prima_color_new( &xc))
								/* to protect from sync - give actual status on SUCCESS */
								guts.palette[xc.pixel].rank = RANK_IMMUTABLE + 1;
							pixel = xc.pixel;
						} else
							continue;
					}
					req[i] |= 0x80000000;
					if ( !restricted) prima_color_add_ref( self, pixel, rank);
					granted++;
					break;
				}
			}
		}

	Pdebug("color replace: granted %d", granted);
	if ( restricted) {
		free( req);
		return true;
	}


	stage = RANK_NORMAL;
	list_create( &widgets, 32, 128);
	if ( granted == psz) {
		free( req);
		goto SUCCESS;
	}

ALLOC_STAGE:
	/* allocate some colors */
	prima_color_sync();
	XCHECKPOINT;
	for ( i = 0; i < psz; i++) {
		if (( req[i] & 0x80000000) == 0) {
			XColor xc;
			xc. red   = COLOR_R16(req[i]);
			xc. green = COLOR_G16(req[i]);
			xc. blue  = COLOR_B16(req[i]);
			if ( alloc_color( &xc)) {
				prima_color_new( &xc);
				prima_color_add_ref( self, xc. pixel, rank);
				granted++;
				req[i] |= 0x80000000;
			} else
				break;
		}
	}
	Pdebug("color replace :ok - now %d are granted", granted);

	if ( granted == psz) {
		free( req);
		goto SUCCESS;
	}

	if ( stage == RANK_NORMAL) {
		/* try to remove RANK_NORMAL colors */
		p = guts. palette;
		for ( i = 0; i < guts. palSize; i++, p++) {
			if ( p-> rank == RANK_NORMAL) {
				int j;
				for ( j = 0; j < p-> users. count; j++) {
					Handle wij = p-> users. items[j];
					if ( list_index_of( &widgets, wij) < 0)
						list_add( &widgets, wij);
					if ( wlpal_get(wij, i) == RANK_NORMAL)
						wlpal_set( wij, i, RANK_FREE);
				}
				list_delete_all( &p-> users, false);
				p-> touched = true;
				stage = RANK_PRIORITY;
			}
		}
		if ( stage == RANK_PRIORITY) goto ALLOC_STAGE;
	}

	free( req);

	if ( XX-> type. image) goto SUCCESS;

	/* try to remove RANK_PRIORITY entries */
	p = guts. palette;
	for ( i = 0; i < guts. palSize; i++, p++) {
		if ( p-> rank == RANK_PRIORITY) {
			int j;
			for ( j = 0; j < p-> users. count; j++) {
				Handle wij = p-> users. items[j];
				if ( X(wij)-> type. widget && list_index_of( &widgets, wij) < 0)
					list_add( &widgets, wij);
				wlpal_set( wij, i, RANK_FREE);
			}
			list_delete_all( &p-> users, false);
			p-> touched = true;
		}
	}

	psz = prima_color_sync();
	if ( psz == 0) goto SUCCESS; /* free no RANK_PRIORITY colors :(  */
	XCHECKPOINT;

	/* collect big palette */
	j = 0;
	for ( i = 0; i < guts. palSize; i++)
		if ( guts. palette[i]. rank != RANK_FREE)
			j++;
	stage = j; /* immutable and locked colors */
	for ( i = 0; i < widgets. count; i++) {
		j += PWidget( widgets. items[i])-> palSize;
		if ( XT_IS_WINDOW(X(widgets. items[i])) &&
			PWindow(widgets. items[i])-> menu)
			j += ciMaxId + 1;
	}

	Pdebug("color: BIG:%d vs %d", j, psz);
	if ( !( rqx = malloc( sizeof( RGBColor) * j))) goto SUCCESS; /* :O */

	{
		RGBColor * r = rqx;
		for ( i = 0; i < guts. palSize; i++)
			if ( guts. palette[i]. rank != RANK_FREE) {
				r-> r = guts. palette[i]. r;
				r-> g = guts. palette[i]. g;
				r-> b = guts. palette[i]. b;
				r++;
			}
		for ( i = 0; i < widgets. count; i++) {
			memcpy( r, PWidget( widgets. items[i])-> palette,
				PWidget( widgets. items[i])-> palSize * sizeof( RGBColor));
			r += PWidget( widgets. items[i])-> palSize;
			if ( XT_IS_WINDOW(X(widgets. items[i])) &&
				PWindow(widgets. items[i])-> menu) {
				int k;
				for ( k = 0; k <= ciMaxId; k++, r++) {
					r-> r = COLOR_R(PWindow(widgets. items[i])-> menuColor[k]);
					r-> g = COLOR_G(PWindow(widgets. items[i])-> menuColor[k]);
					r-> b = COLOR_B(PWindow(widgets. items[i])-> menuColor[k]);
				}
			}
		}
	}

	/* squeeze palette */
	if ( j > psz + stage) {
		int k, tolerance = 0, t2 = 0, lim = psz + stage;
		while ( 1) {
			for ( i = 0; i < j; i++) {
				RGBColor r = rqx[i];
				for ( k = (( i + 1) > stage) ? i + 1 : stage; k < j; ) {
					if (
						( r.r - rqx[k].r) * ( r.r - rqx[k].r) +
						( r.g - rqx[k].g) * ( r.g - rqx[k].g) +
						( r.b - rqx[k].b) * ( r.b - rqx[k].b)
						<= t2) {
						if ( k < j - 1) rqx[k] = rqx[j-1];
						if ( --j <= lim) goto ENOUGH;
					} else
						k++;
				}
			}
			tolerance += 2;
			t2 = tolerance * tolerance;
		}
ENOUGH:;
	}

	Pdebug("color replace: ok. XAllocColor again");
	granted = 0;
	for ( i = stage; i < stage + psz; i++) {
		XColor xc;
		xc. red   = rqx[i]. r << 8;
		xc. green = rqx[i]. g << 8;
		xc. blue  = rqx[i]. b << 8;
		if ( alloc_color( &xc)) {
			if ( prima_color_new( &xc)) {
				/* give new color NORMAL status - to be cleaned automatically */
				/* upon 1st sync() invocation */
				guts. palette[xc. pixel]. touched = 1;
				guts. palette[xc. pixel]. rank = RANK_NORMAL;
				granted++;
			}
		} else
			break;
	}
	free( rqx);
	Pdebug("color replace: ok - %d out of %d ", granted, psz);
	XCHECKPOINT;

	/* now give away colors that can be mapped to reduced palette */
	prima_palette_replace( self, true);
	for ( i = 0; i < widgets. count; i++)
		prima_palette_replace( widgets. items[i], true);
	XCHECKPOINT;

SUCCESS:

	/* restore status of pre-fetched colors */
	for ( i = 0; i < guts. palSize; i++)
		if ( guts.palette[i].rank == RANK_IMMUTABLE + 1)
			guts.palette[i].rank = RANK_PRIORITY;

	prima_color_sync();

	XUngrabServer( DISP);

	for ( i = 0; i < widgets. count; i++)
		if ( PWidget( widgets. items[i])-> stage < csDead)
			apc_widget_invalidate_rect( widgets. items[i], NULL);

	Pdebug("color replace: exit");
	list_destroy( &widgets);
	return true;
}

Bool
prima_palette_alloc( Handle self)
{
	if ( !guts. dynamicColors) return true;
	if ( !( X(self)-> palette = malloc( guts. localPalSize)))
		return false;
	bzero( X(self)-> palette,  guts. localPalSize);
	return true;
}

void
prima_palette_free( Handle self, Bool priority)
{
	int i, max = priority ? 2 : 1;
	if ( !guts. dynamicColors) return;
	for ( i = 0; i < guts. palSize; i++) {
		int rank = wlpal_get(self,i);
		if ( rank > RANK_FREE && max >= rank) {
			wlpal_set( self, i, RANK_FREE);
			list_delete( &guts. palette[i]. users, self);
			Pdebug("color: %s free %d, %d", PWidget(self)-> name, i, rank);
			guts. palette[i]. touched = true;
		}
	}
	Pdebug(":%s for %s", priority ? "PRIO" : "", PWidget(self)-> name);
}

int
prima_lpal_get( Byte * palette, int index)
{
	return LPAL_GET( index, palette[ LPAL_ADDR( index ) ]);
}

void
prima_lpal_set( Byte * palette, int index, int rank )
{
	palette[ LPAL_ADDR( index ) ] &=~ LPAL_MASK( index);
	palette[ LPAL_ADDR( index ) ] |=  LPAL_SET( index, rank);
}


static Bool kill_hatches( Pixmap pixmap, int keyLen, void * key, void * dummy)
{
	XFreePixmap( DISP, pixmap);
	return false;
}

Pixmap
prima_get_hatch( FillPattern * fp)
{
	int i;
	Pixmap p;
	FillPattern fprev;
	Byte *mirrored_bits;
	if ( memcmp( fp, fillPatterns[fpSolid], sizeof( FillPattern)) == 0)
		return NULL_HANDLE;
	if (( p = ( Pixmap) hash_fetch( hatches, fp, sizeof( FillPattern))))
		return p;

	mirrored_bits = prima_mirror_bits();
	for ( i = 0; i < sizeof( FillPattern); i++) {
		fprev[i] = (*fp)[ sizeof(FillPattern) - i - 1];
		fprev[i] = mirrored_bits[fprev[i]];
	}
	if (( p = XCreateBitmapFromData( DISP, guts. root, (char*)fprev, 8, 8)) == None) {
		hash_first_that( hatches, (void*)kill_hatches, NULL, NULL, NULL);
		hash_destroy( hatches, false);
		hatches = hash_create();
		if (( p = XCreateBitmapFromData( DISP, guts. root, (char*)fprev, 8, 8)) == None)
			return NULL_HANDLE;
	}
	hash_store( hatches, fp, sizeof( FillPattern), ( void*) p);
	return p;
}
