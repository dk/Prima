/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/***********************************************************/
/*                                                         */
/*  System dependent graphics (unix, x11)                  */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Image.h"

#define SORT(a,b)	({ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }})
#define REVERT(a)	({ XX-> size. y - (a) - 1; })
#define SHIFT(a,b)	({ (a) += XX-> gtransform. x; (b) += XX-> gtransform. y; })

static int rop_map[] = {
   GXcopy	/* ropCopyPut */,		/* dest  = src */
   GXxor	/* ropXorPut */,		/* dest ^= src */
   GXand	/* ropAndPut */,		/* dest &= src */
   GXor		/* ropOrPut */,			/* dest |= src */
   GXcopyInverted /* ropNotPut */,		/* dest = !src */
   GXnoop	/* ropNotBlack */,		/* dest = (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNotDestXor */,		/* dest = (!dest) ^ src */	/* XXX */
   GXandReverse	/* ropNotDestAnd */,		/* dest = (!dest) & src */
   GXorReverse	/* ropNotDestOr */,		/* dest = (!dest) | src */
   GXequiv	/* ropNotSrcXor */,		/* dest ^= !src */
   GXandInverted /* ropNotSrcAnd */,		/* dest &= !src */
   GXorInverted	/* ropNotSrcOr */,		/* dest |= !src */
   GXnoop	/* ropNotXor */,		/* dest = !(src ^ dest) */	/* XXX */
   GXnand	/* ropNotAnd */,		/* dest = !(src & dest) */
   GXnor	/* ropNotOr */,			/* dest = !(src | dest) */
   GXnoop	/* ropNotBlackXor */,		/* dest ^= (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNotBlackAnd */,		/* dest &= (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNotBlackOr */,		/* dest |= (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNoOper */,		/* dest = dest */
   GXclear	/* ropBlackness */,		/* dest = 0 */
   GXset	/* ropWhiteness */,		/* dest = white */
   GXinvert	/* ropInvert */,		/* dest = !dest */
   GXnoop	/* ropPattern */,		/* dest = pattern */		/* YYY */
   GXnoop	/* ropXorPattern */,		/* dest ^= pattern */		/* YYY */
   GXnoop	/* ropAndPattern */,		/* dest &= pattern */		/* YYY */
   GXnoop	/* ropOrPattern */,		/* dest |= pattern */		/* YYY */
   GXnoop	/* ropNotSrcOrPat */,		/* dest |= pattern | (!src) */	/* YYY */
   GXnoop	/* ropSrcLeave */,		/* dest = (src != fore color) ? src : figa */	/* YYY */
   GXnoop	/* ropDestLeave */,		/* dest = (src != back color) ? src : figa */	/* YYY */
};

static RGBColor standard_button_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Button.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Button.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Button.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Button.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Button.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Button.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Button.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Button.dark3d */
};

static RGBColor standard_checkbox_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Checkbox.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Checkbox.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Checkbox.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Checkbox.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Checkbox.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Checkbox.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Checkbox.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Checkbox.dark3d */
};

static RGBColor standard_combo_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Combo.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Combo.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Combo.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Combo.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Combo.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Combo.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Combo.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Combo.dark3d */
};

static RGBColor standard_dialog_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Dialog.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Dialog.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Dialog.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Dialog.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Dialog.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Dialog.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Dialog.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Dialog.dark3d */
};

static RGBColor standard_edit_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Edit.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Edit.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Edit.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Edit.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Edit.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Edit.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Edit.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Edit.dark3d */
};

static RGBColor standard_inputline_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Inputline.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Inputline.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Inputline.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Inputline.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Inputline.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Inputline.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Inputline.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Inputline.dark3d */
};

static RGBColor standard_label_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Label.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Label.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Label.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Label.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Label.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Label.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Label.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Label.dark3d */
};

static RGBColor standard_listbox_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Listbox.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Listbox.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Listbox.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Listbox.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Listbox.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Listbox.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Listbox.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Listbox.dark3d */
};

static RGBColor standard_menu_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Menu.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Menu.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Menu.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Menu.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Menu.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Menu.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Menu.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Menu.dark3d */
};

static RGBColor standard_popup_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Popup.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Popup.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Popup.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Popup.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Popup.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Popup.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Popup.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Popup.dark3d */
};

static RGBColor standard_radio_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Radio.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Radio.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Radio.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Radio.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Radio.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Radio.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Radio.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Radio.dark3d */
};

static RGBColor standard_scrollbar_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Scrollbar.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Scrollbar.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Scrollbar.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Scrollbar.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Scrollbar.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Scrollbar.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Scrollbar.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Scrollbar.dark3d */
};

static RGBColor standard_slider_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Slider.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Slider.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Slider.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Slider.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Slider.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Slider.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Slider.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Slider.dark3d */
};

static RGBColor standard_widget_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Widget.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Widget.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Widget.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Widget.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Widget.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Widget.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Widget.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Widget.dark3d */
};

static RGBColor standard_window_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Window.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Window.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Window.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Window.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Window.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Window.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Window.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Window.dark3d */
};

static RGBColor standard_application_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Application.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Application.background */
   { 0x00, 0x00, 0x00 },	/* Prima.Application.hilitefore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Application.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Application.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Application.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Application.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Application.dark3d */
};

static PRGBColor standard_colors[] = {
   nil,
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

static const int MAX_COLOR_CLASS = sizeof( standard_colors) / sizeof( standard_colors[ 0]) - 1;
static const int MAX_COLOR_INDEX = ciMaxId;
static const RGBColor RGB_BLACK = { 0, 0, 0 };

static RGBColor
get_standard_color( long class, int index) {
   long cls = (class & wcMask) >> 16;
   if ( cls <= 0 || cls > MAX_COLOR_CLASS) {
      warn( "Illegal color class specified: %08x", class);
      return RGB_BLACK;
   }
   if ( index < 0 || index > MAX_COLOR_INDEX) {
      warn( "Illegal color index specified: %d", index);
      return RGB_BLACK;
   }
   return standard_colors[ cls][ index];
}

static PHash globalColors = nil;
static Colormap globalColormap;

static XColor*
allocate_color( Handle self, Color color) {
   RGBColor c;
   XColor *x_color;

   if ( !globalColors) {
      globalColors = hash_create();
      globalColormap = DefaultColormap( DISP, SCREEN);
   }

   if ( color < 0) {
      /* XXX - remove this -1: */
      c = get_standard_color( color, (int)((unsigned long)color & ~(unsigned long)(wcMask|0x80000000))-1);
   } else {
      c = *(( PRGBColor) &color);
   }

   x_color = hash_fetch( globalColors, &c, sizeof(c));
   if ( !x_color) {
      Status r;
      x_color = malloc( sizeof( XColor));
      if ( !x_color)
	 croak( "allocate_color: not enough memory");
      x_color-> red = (short)((unsigned short)c. r << 8);
      x_color-> green = (short)((unsigned short)c. g << 8);
      x_color-> blue = (short)((unsigned short)c. b << 8);
      x_color-> flags = DoRed | DoGreen | DoBlue;
      x_color-> pixel = 0;
      r = XAllocColor( DISP, globalColormap, x_color);
      printf( "*** Setting color of %02x/%02x/%02x from %02x/%02x/%02x : %d***\n",
	      x_color-> red, x_color-> green, x_color-> blue,
	      (int)c.r,(int)c.g,(int)c.b, r);
      hash_store( globalColors, &c, sizeof(c), x_color);
   }

   return x_color;
}

void
prima_get_gc( PDrawableSysData selfxx)
{
   XGCValues gcv;

   if ( XX-> gc && XX-> gcl)
      return;

   if ( XX-> gc || XX-> gcl) {
      croak( "prima_get_gc: internal error");
   }

   if ( guts. free_gcl) {
      XX-> gcl = guts. free_gcl;
      guts. free_gcl = XX-> gcl-> next;
   } else {
      XX-> gcl = malloc( sizeof( GCList));
      XX-> gcl-> gc = XCreateGC( DISP, RootWindow( DISP, SCREEN), 0, &gcv);
      XCHECKPOINT;
   }

   XX-> gcl-> prev = nil;
   XX-> gcl-> next = guts. used_gcl;
   guts. used_gcl = XX-> gcl;
   XX-> gc = XX-> gcl-> gc;
   XX-> gcl-> holder = XX;
}

void
prima_release_gc( PDrawableSysData selfxx)
{
   if ( XX-> gc) {
      if ( !XX-> gcl) {
	 croak( "prima_release_gc: internal error #2");
      }
      if ( XX-> gcl-> prev) {
	 XX-> gcl-> prev-> next = XX-> gcl-> next;
      } else {
	 guts. used_gcl = XX-> gcl-> next;
      }
      XX-> gcl-> prev = nil;
      XX-> gcl-> next = guts. free_gcl;
      guts. free_gcl = XX-> gcl;
      XX-> gcl-> holder = nil;
      XX-> gc = nil;
      XX-> gcl = nil;
   } else {
      if ( XX-> gcl) {
	 croak( "prima_release_gc: internal error #2");
      }
   }
}

void
apc_gp_init( Handle self)
{
   DOLBUG( "apc_gp_init()\n");
   X(self)-> resolution = guts. resolution;
}

void
apc_gp_done( Handle self)
{
   prima_release_gc(X(self));
}

void
apc_gp_arc( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DOLBUG( "apc_gp_arc()\n");
}

void
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XFillRectangle( DISP, XX-> drawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
   XCHECKPOINT;
}

void
apc_gp_clear( Handle self)
{
   DOLBUG( "apc_gp_clear()\n");
}

void
apc_gp_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DOLBUG( "apc_gp_chord()\n");
}

void
apc_gp_draw_poly( Handle self, int numPts, Point *points)
{
   DOLBUG( "apc_gp_draw_poly()\n");
}

void
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{
   DOLBUG( "apc_gp_draw_poly2()\n");
}

void
apc_gp_ellipse( Handle self, int x, int y, int radX, int radY)
{
   DOLBUG( "apc_gp_ellipse()\n");
}

void
apc_gp_fill_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DOLBUG( "apc_gp_fill_chord()\n");
}

void
apc_gp_fill_ellipse( Handle self, int x, int y, int radX, int radY)
{
   DOLBUG( "apc_gp_fill_ellipse()\n");
}

void
apc_gp_fill_poly( Handle self, int numPts, Point *points)
{
   /* XXX - beware, current implementation will not deal correctly with different rops and tiles */
   static XPoint *p = nil;
   static int size = 0;
   DEFXX;
   int i;

   if ( numPts >= size) {
      free( p);
      size = numPts + 1;
      p = malloc( size * sizeof( XPoint));
      if ( !p)
	 croak( "apc_gp_fill_poly: not enough memory");
   }

   for ( i = 0; i < numPts; i++) {
      p[i]. x = (short)points[i]. x + XX-> gtransform. x;
      p[i]. y = (short)REVERT(points[i]. y + XX-> gtransform. y);
   }
   p[numPts]. x = (short)points[0]. x + XX-> gtransform. x;
   p[numPts]. y = (short)REVERT(points[0]. y + XX-> gtransform. y);

   if ( guts. limits. XFillPolygon >= numPts) {
      XFillPolygon( DISP, XX-> drawable, XX-> gc, p, numPts, ComplexShape, CoordModeOrigin);
      XCHECKPOINT;
   } else {
      warn( "apc_gp_fill_poly() XFillPolygon() request size limit reached");
   }
   if ( guts. limits. XDrawLines > numPts) {
      XDrawLines( DISP, XX-> drawable, XX-> gc, p, numPts+1, CoordModeOrigin);
      XCHECKPOINT;
   } else {
      warn( "apc_gp_fill_poly() XDrawLines() request size limit reached");
   }
}

void
apc_gp_fill_sector( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DOLBUG( "apc_gp_fill_sector()\n");
}

void
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{
   DOLBUG( "apc_gp_flood_fill()\n");
}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{
   DOLBUG( "apc_gp_get_pixel()\n");
   return 0;
}

void
apc_gp_line( Handle self, int x1, int y1, int x2, int y2)
{
   /* !!! - this function will not work correctly for cosmetic (width 0) lines */
   /*       (but we are avoiding them anyway, for now) */
   /* XXX - implement a general case of line end correction */
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   if ( y1 == y2) {
      SORT( x1, x2); x2++;
   } else if ( x1 == x2) {
      SORT( y1, y2); y1--;
   }
   XDrawLine( DISP, XX-> drawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
}

static Bool
create_image_cache_8_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   int i;
   U16 lut[ 256];
   unsigned long red_mask, green_mask, blue_mask;
   Visual *v = DefaultVisual( DISP, SCREEN);
   U16 *data, *d;
   int x, y;

   red_mask = v-> red_mask;
   green_mask = v-> green_mask;
   blue_mask = v-> blue_mask;
   for ( i = 0; i < img-> palSize; i++) { /* XXX ? Is palSize inconsistent? 256 or 768? */
      lut[i] = 0;
      lut[i] |=
	 (((img-> palette[i]. r >> 3) << 11) & red_mask) & 0xffff;
      lut[i] |= 
	 (((img-> palette[i]. g >> 2) << 5) & green_mask) & 0xffff;
      lut[i] |=
	 ((img-> palette[i]. b >> 3) & blue_mask) & 0xffff;
   }

   d = data = malloc( img-> w * img-> h * sizeof( U16));
   if ( !data) {
      warn( "no memory");
      return false;
   }
   for ( y = img-> h-1; y >= 0; y--) {
      unsigned char *line = img-> data + y*img-> lineSize;
      for ( x = 0; x < img-> w; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> imageCache = XCreateImage( DISP, v,
				    guts. depth, ZPixmap, 0, (unsigned char*)data,
				    img-> w, img-> h, 8, 0);
   if (!IMG-> imageCache) {
      free( d);
      warn( "error during XCreateImage()");
      return false;
   }
   return true;
}

static Bool
create_image_cache( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);

   if ( IMG-> imageCache)
      return true;

   if (( img-> type & imBPP) != 8) {
      croak( "Unsupported img-> bpp");
   }
   if ( !img-> palette) {
      croak( "No palette, ouch!");
   }

   switch ( guts. depth) {
   case 16:
      return create_image_cache_8_to_16( img);
   default:
      croak( "Unsupported guts. depth");
   }
   return true;
}

void
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   DEFXX;
   PDrawableSysData IMG = X(image);
   PImage img = PImage( image);

   if ( !create_image_cache( img))
      croak( "Error creating image cache");
   SHIFT( x, y);
   XPutImage( DISP, XX-> drawable, XX-> gc, IMG-> imageCache,
	      xFrom, img-> h - yFrom - yLen,
	      x, REVERT(y) - yLen, xLen, yLen);
}

void
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XDrawRectangle( DISP, XX-> drawable, XX-> gc, x1, REVERT( y2), x2 - x1, y2 - y1);
   XCHECKPOINT;
}

void
apc_gp_sector( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DOLBUG( "apc_gp_sector()\n");
}

void
apc_gp_set_palette( Handle self)
{
   DOLBUG( "apc_gp_set_palette()\n");
}

void
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
   DEFXX;
   XColor *c = allocate_color( self, color);
   unsigned long old = XX-> fore. pixel;

   SHIFT( x, y);
   XSetForeground( DISP, XX-> gc, c-> pixel);
   XCHECKPOINT;
   XDrawPoint( DISP, XX-> drawable, XX-> gc, x, REVERT( y));
   XCHECKPOINT;
   XSetForeground( DISP, XX-> gc, old);
   XCHECKPOINT;
}

void
apc_gp_stretch_image( Handle self, Handle image,
		      int x, int y, int xFrom, int yFrom,
		      int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   DOLBUG( "apc_gp_stretch_image()\n");
}

void
apc_gp_text_out( Handle self, const char* text, int x, int y, int len)
{
   DEFXX;
   SHIFT( x, y);
   XDrawString( DISP, XX-> drawable, XX-> gc, x, REVERT( y), text, len);
   XCHECKPOINT;
}

char **
apc_gp_text_wrap( Handle self, TextWrapRec * t)
{
   DOLBUG( "apc_gp_text_wrap()\n");
   return nil;
}


/* gpi settings */
Color
apc_gp_get_back_color( Handle self)
{
   DEFXX;
   XColor c = ( XX-> flags. paint) ? XX-> back : XX-> savedBack;
   return ARGB( c. red >> 8, c. green >> 8, c. blue >> 8);
}

int
apc_gp_get_bpp( Handle self)
{
   return guts. depth;
}

Color
apc_gp_get_color( Handle self)
{
   DEFXX;
   XColor c = ( XX-> flags. paint) ? XX-> fore : XX-> savedFore;
   return ARGB( c. red >> 8, c. green >> 8, c. blue >> 8);
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   DOLBUG( "apc_gp_get_clip_rect()\n");
   return (Rect){0,0,0,0};
}

PFontABC
apc_gp_get_font_abc( Handle self)
{
   DOLBUG( "apc_gp_get_font_abc()\n");
   return nil;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
   return &(X(self)-> fillPattern);
}

int
apc_gp_get_line_end( Handle self)
{
   DOLBUG( "apc_gp_get_line_end()\n");
   return 0;
}

int
apc_gp_get_line_width( Handle self)
{
   DEFXX;
   int w;
   XGCValues gcv;

   if ( XX-> flags. paint) {
      if ( XX-> flags. zeroLine)
	 w = 0;
      else {
	 if ( XGetGCValues( DISP, XX-> gc, GCLineWidth, &gcv) == 0) {
	    warn( "apc_gp_get_line_width(): XGetGCValues() error");
	 }
	 w = gcv. line_width;
      }
   } else {
      if ( XX-> flags. savedZeroLine)
	 w = 0;
      else
	 w = XX-> gcv. line_width;
   }
   return w;
}

int
apc_gp_get_line_pattern( Handle self)
{
   DOLBUG( "apc_gp_get_line_pattern()\n");
   return 0;
}

Point
apc_gp_get_resolution( Handle self)
{
   DOLBUG( "apc_gp_get_resolution()\n");
   return (Point){0,0};
}

int
apc_gp_get_rop( Handle self)
{
   DEFXX;
   if ( XX-> flags. paint) {
      return XX-> paintRop;
   } else {
      return XX-> rop;
   }
}

int
apc_gp_get_rop2( Handle self)
{
   DOLBUG( "apc_gp_get_rop2()\n");
   return 0;
}

int
apc_gp_get_text_width( Handle self, const char *text, int len, Bool addOverhang)
{
   DEFXX;
   if ( !XX-> font) {
      apc_gp_set_font( self, &PDrawable( self)-> font);
   }
   return XTextWidth( XX-> font-> fs, text, len);
   return 0;
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len)
{
   DOLBUG( "apc_gp_get_text_box()\n");
   return nil;
}

Point
apc_gp_get_transform( Handle self)
{
   DEFXX;
   if ( XX-> flags. paint) {
      return XX-> gtransform;
   } else {
      return XX-> transform;
   }
}

Bool
apc_gp_get_text_opaque( Handle self)
{
   DOLBUG( "apc_gp_get_text_opaque()\n");
   return true;
}

void
apc_gp_set_back_color( Handle self, Color color)
{
   DEFXX;
   XColor *c = allocate_color( self, color);

   if ( XX-> flags. paint) {
      XX-> back = *c;
      XSetBackground( DISP, XX-> gc, c-> pixel);
      XCHECKPOINT;
   } else {
      XX-> savedBack = *c;
      XX-> gcv. background = c-> pixel;
   }
}

void
apc_gp_set_clip_rect( Handle self, Rect clipRect)
{
   DOLBUG( "apc_gp_set_clip_rect()\n");
}

void
apc_gp_set_color( Handle self, Color color)
{
   DEFXX;
   XColor *c = allocate_color( self, color);

   if ( XX-> flags. paint) {
      XX-> fore = *c;
      XSetForeground( DISP, XX-> gc, c-> pixel);
      XCHECKPOINT;
   } else {
      XX-> savedFore = *c;
      XX-> gcv. foreground = c-> pixel;
   }
}

void
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   DEFXX;

   memcpy( XX-> fillPattern, pattern, sizeof( FillPattern));
   DOLBUG( "apc_gp_set_fill_pattern()\n");
}

/*- see apc_font.c
void
apc_gp_set_font( Handle self, PFont font)
*/

void
apc_gp_set_line_end( Handle self, int lineEnd)
{
   DOLBUG( "apc_gp_set_line_end()\n");
}

void
apc_gp_set_line_width( Handle self, int lineWidth)
{
   DEFXX;
   XGCValues gcv;
   int zeroLine = lineWidth == 0;

   if ( zeroLine)
      lineWidth = 1;

   if ( XX-> flags. paint) {
      XX-> flags. zeroLine = zeroLine;
      gcv. line_width = lineWidth;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
      XCHECKPOINT;
   } else {
      XX-> flags. savedZeroLine = zeroLine;
      XX-> gcv. line_width = lineWidth;
   }
}

void
apc_gp_set_line_pattern( Handle self, int pattern)
{
//#define    lpNull           0x0000     /* */
//#define    lpSolid          0xFFFF     /* ___________ */
//#define    lpDash           0xF0F0     /* __ __ __ __ */
//#define    lpLongDash       0xFF00     /* _____ _____ */
//#define    lpShortDash      0xCCCC     /* _ _ _ _ _ _ */
//#define    lpDot            0x5555     /* . . . . . . */
//#define    lpDotDot         0x4444     /* ............ */
//#define    lpDashDot        0xFAFA     /* _._._._._._ */
//#define    lpDashDotDot     0xEAEA     /* _.._.._.._.. */
   DOLBUG( "apc_gp_set_line_pattern()\n");
}

void
apc_gp_set_rop( Handle self, int rop)
{
   DEFXX;
   int function;

   if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
      function = GXnoop;
   else
      function = rop_map[ rop];

   if ( XX-> flags. paint) {
      XX-> paintRop = rop;
      XSetFunction( DISP, XX-> gc, function);
      XCHECKPOINT;
   } else {
      XX-> gcv. function = function;
      XX-> rop = rop;
   }
}

void
apc_gp_set_rop2( Handle self, int rop)
{
   DOLBUG( "apc_gp_set_rop2()\n");
}

void
apc_gp_set_transform( Handle self, int x, int y)
{
   DEFXX;
   if ( XX-> flags. paint) {
      XX-> gtransform. x = x;
      XX-> gtransform. y = y;
   } else {
      XX-> transform. x = x;
      XX-> transform. y = y;
   }
}

void
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
   DOLBUG( "apc_gp_set_text_opaque()\n");
}

