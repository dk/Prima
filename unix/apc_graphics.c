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
#include "Icon.h"

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
   { 0xcc, 0xcc, 0xcc },        /* Prima.Edit.hilitefore */
   { 0x00, 0x00, 0x00 },	/* Prima.Edit.hilitebac */
   { 0x60, 0x60, 0x60 },        /* Prima.Edit.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Edit.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Edit.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Edit.dark3d */
};

static RGBColor standard_inputline_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Inputline.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Inputline.background */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Inputline.hilitefore */
   { 0x00, 0x00, 0x00 },	/* Prima.Inputline.hiliteback */
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
   { 0xcc, 0xcc, 0xcc },        /* Prima.Listbox.hilitefore */
   { 0x00, 0x00, 0x00 },	/* Prima.Listbox.hiliteback */
   { 0x60, 0x60, 0x60 },        /* Prima.Listbox.disabledfore */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Listbox.disabledback */
   { 0xff, 0xff, 0xff },        /* Prima.Listbox.light3d */
   { 0x80, 0x80, 0x80 },	/* Prima.Listbox.dark3d */
};

static RGBColor standard_menu_colors[] = {
   { 0x00, 0x00, 0x00 },	/* Prima.Menu.foreground */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Menu.background */
   { 0xcc, 0xcc, 0xcc },        /* Prima.Menu.hilitefore */
   { 0x00, 0x00, 0x00 },	/* Prima.Menu.hiliteback */
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
get_standard_color( long class, int index)
{
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

XColor*
prima_allocate_color( Handle self, Color color)
{
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
      if ( !x_color) {
	 /* XXX better free existing colors... */
	 croak( "prima_allocate_color: not enough memory");
      }
      x_color-> red = (short)((unsigned short)c. r << 8);
      x_color-> green = (short)((unsigned short)c. g << 8);
      x_color-> blue = (short)((unsigned short)c. b << 8);
      x_color-> flags = DoRed | DoGreen | DoBlue;
      x_color-> pixel = 0;
      r = XAllocColor( DISP, globalColormap, x_color);
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
prima_prepare_drawable_for_painting( Handle self)
{
   DEFXX;
   unsigned long mask = VIRGIN_GC_MASK;

   XX-> paint_rop = XX-> rop;
   XX-> saved_font = PDrawable( self)-> font;
   XX-> fore = XX-> saved_fore;
   XX-> back = XX-> saved_back;
   XX-> flags. zero_line = XX-> flags. saved_zero_line;
   XX-> gcv. clip_mask = None;
   XX-> gtransform = XX-> transform;

   prima_get_gc( XX);
   XChangeGC( DISP, XX-> gc, mask, &XX-> gcv);
   XCHECKPOINT;
   if ( XX-> dashes) {
      XSetDashes( DISP, XX-> gc, 0, XX-> dashes, XX-> ndashes);
      XX-> paint_ndashes = XX-> ndashes;
      XX-> paint_dashes = malloc( XX-> ndashes);
      memcpy( XX-> paint_dashes, XX-> dashes, XX-> ndashes);
   } else {
      XX-> paint_dashes = malloc(1);
      if ( XX-> ndashes < 0) {
	 XX-> paint_dashes[0] = '\0';
	 XX-> paint_ndashes = 0;
      } else {
	 XX-> paint_dashes[0] = '\1';
	 XX-> paint_ndashes = 1;
      }
   }

   XX-> clip_rect. x = 0;
   XX-> clip_rect. y = 0;
   XX-> clip_rect. width = XX-> size.x;
   XX-> clip_rect. height = XX-> size.y;
   if ( XX-> region) {
      XSetRegion( DISP, XX-> gc, XX-> region);
      XX-> stale_region = XX-> region;
      XX-> region = nil;
   }

   XX-> flags. paint = true;

   if ( !XX-> flags. reload_font && XX-> font && XX-> font-> id) {
      XSetFont( DISP, XX-> gc, XX-> font-> id);
      XCHECKPOINT;
   } else {
      apc_gp_set_font( self, &PDrawable( self)-> font);
      XX-> flags. reload_font = false;
   }
}

void
prima_cleanup_drawable_after_painting( Handle self)
{
   DEFXX;
   prima_release_gc(XX);
   free(XX->paint_dashes);
   XX-> paint_dashes = nil;
   XX-> paint_ndashes = 0;
   XX-> flags. paint = false;
   if ( XX-> flags. reload_font) {
      PDrawable( self)-> font = XX-> saved_font;
   }
   if ( XX-> stale_region) {
      XDestroyRegion( XX-> stale_region);
      XX-> stale_region = nil;
   }
}

Bool
apc_gp_init( Handle self)
{
   DOLBUG( "apc_gp_init()\n");
   X(self)-> resolution = guts. resolution;
   return true;
}

Bool
apc_gp_done( Handle self)
{
   prima_release_gc(X(self));
   return true;
}

Bool
apc_gp_arc( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DEFXX;
   SHIFT( x, y);
   if (( angleEnd > angleStart + 360) && (((int)( angleEnd - angleStart) / 360) % 2) == 1)
      XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
          0, 360 * 64);
   XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   return true;
}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XFillRectangle( DISP, XX-> drawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_clear( Handle self)
{
   DOLBUG( "apc_gp_clear()\n");
   return true;
}

#define GRAD 57.29577951

Bool
apc_gp_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   int sy;
   DEFXX;
   SHIFT( x, y);
   sy = REVERT( y);
   if (( angleEnd > angleStart + 360) && (((int)( angleEnd - angleStart) / 360) % 2) == 1)
      XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
          0, 360 * 64);
   XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   XDrawLine( DISP, XX-> drawable, XX-> gc,
       x + cos( angleStart / GRAD) * radX, sy - sin( angleStart / GRAD) * radY,
       x + cos( angleEnd / GRAD) * radX,   sy - sin( angleEnd / GRAD) * radY
   );
   return true;
}

Bool
apc_gp_draw_poly( Handle self, int n, Point *pp)
{
   DEFXX;
   int i;
   int x = XX-> gtransform. x;
   int y = XX-> size. y - 1 - XX-> gtransform. y;
   XPoint *p = malloc( sizeof( XPoint)*n);

   if (!p)
      croak( "apc_gp_draw_poly(): not enough memory");

   for ( i = 0; i < n; i++) {
      p[i].x = pp[i].x + x;
      p[i].y = y - pp[i].y;
   }

/*    for ( i = 0; i < n; i++) { */
/*       fprintf(stderr, "p[%d]=(XPoint){%hd,%hd}; ", i, p[i].x, p[i].y); */
/*       if ( i % 4 == 0) */
/* 	 fprintf( stderr, "\n"); */
/*    } */
/*    fprintf( stderr, "\n"); */

   if ( XX-> flags. zero_line) {
      XGCValues gcv;
      gcv. line_width = 0;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
   }

   XDrawLines( DISP, XX-> drawable, XX-> gc, p, n, CoordModeOrigin);

   if ( XX-> flags. zero_line) {
      XGCValues gcv;
      gcv. line_width = 1;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
   }

/* p[165]=(XPoint){207,134}; p[166]=(XPoint){207,133}; p[167]=(XPoint){207,132}; p[168]=(XPoint){207,131};  */
/* p[169]=(XPoint){208,130}; p[170]=(XPoint){208,129}; p[171]=(XPoint){208,128}; p[172]=(XPoint){208,127};  */

/* p[172]=(XPoint){207,134}; p[171]=(XPoint){207,133}; p[170]=(XPoint){207,132}; p[169]=(XPoint){207,131};  */
/* p[168]=(XPoint){208,130}; p[167]=(XPoint){208,129}; p[166]=(XPoint){208,128}; p[165]=(XPoint){208,127};  */
/* XSetLineAttributes( DISP, XX-> gc, 0, LineSolid, CapRound, JoinRound); */
/* p[173]=(XPoint){208,126}; p[174]=(XPoint){208,125}; p[175]=(XPoint){208,124}; p[176]=(XPoint){208,123};  */
/* p[177]=(XPoint){208,122}; p[178]=(XPoint){208,121}; p[179]=(XPoint){209,120}; p[180]=(XPoint){209,119};  */
/* p[181]=(XPoint){210,118}; p[182]=(XPoint){210,117}; p[183]=(XPoint){211,116}; p[184]=(XPoint){211,115};  */
/* p[185]=(XPoint){211,114}; p[186]=(XPoint){211,113}; p[187]=(XPoint){212,112}; p[188]=(XPoint){212,111};  */
/* p[189]=(XPoint){213,111}; p[190]=(XPoint){214,110}; p[191]=(XPoint){214,109}; p[192]=(XPoint){215,108};  */
/* p[193]=(XPoint){215,107}; p[194]=(XPoint){216,107}; p[195]=(XPoint){217,106}; p[196]=(XPoint){217,105};  */
/* p[197]=(XPoint){218,105}; p[198]=(XPoint){219,104}; p[199]=(XPoint){219,103}; p[200]=(XPoint){220,102};  */
/* p[201]=(XPoint){220,101}; p[202]=(XPoint){219,100}; p[203]=(XPoint){219,99}; p[204]=(XPoint){219,98};  */
/* p[205]=(XPoint){219,97}; p[206]=(XPoint){220,97}; p[207]=(XPoint){221,96}; p[208]=(XPoint){221,95};  */
/* p[209]=(XPoint){222,94}; p[210]=(XPoint){222,93}; p[211]=(XPoint){223,93}; p[212]=(XPoint){224,93};  */
/* p[213]=(XPoint){225,93}; p[214]=(XPoint){226,93}; p[215]=(XPoint){227,93}; p[216]=(XPoint){228,93};  */
/* p[217]=(XPoint){229,93}; p[218]=(XPoint){230,93}; p[219]=(XPoint){231,93}; p[220]=(XPoint){232,93};  */
/* p[221]=(XPoint){232,94}; p[222]=(XPoint){233,95}; p[223]=(XPoint){234,95}; p[224]=(XPoint){235,95};  */
/* p[225]=(XPoint){236,94}; p[226]=(XPoint){236,93};  */

/*    i = 165; n = 172; */
/*    XDrawLines( DISP, XX-> drawable, XX-> gc, p+i, n-i+1, CoordModeOrigin); */
   free( p);
   return true;
}

Bool
apc_gp_draw_poly2( Handle self, int numPts, Point * points)
{
   DOLBUG( "apc_gp_draw_poly2()\n");
   return true;
}

Bool
apc_gp_ellipse( Handle self, int x, int y, int radX, int radY)
{
   DEFXX;
   SHIFT( x, y);
   XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2, 0, 64*360);
   return true;
}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DEFXX;
   SHIFT( x, y);
   XSetArcMode( DISP, XX-> gc, ArcChord);
   XFillArc( DISP, XX-> drawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   return true;
}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y, int radX, int radY)
{
   DEFXX;
   SHIFT( x, y);
   XFillArc( DISP, XX-> drawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2, 0, 64*360);
   return true;
}

Bool
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
   return true;
}

Bool
apc_gp_fill_sector( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DEFXX;
   SHIFT( x, y);
   XSetArcMode( DISP, XX-> gc, ArcPieSlice);
   XFillArc( DISP, XX-> drawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   return true;
}

Bool
apc_gp_flood_fill( Handle self, int x, int y, Color borderColor, Bool singleBorder)
{
   DOLBUG( "apc_gp_flood_fill()\n");
   return true;
}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{
   DOLBUG( "apc_gp_get_pixel()\n");
   return 0;
}

Bool
apc_gp_get_region( Handle self, Handle mask)
{
   DOLBUG( "apc_gp_get_region()\n");
   return false;
}

Bool
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
   return true;
}

static void
create_image_cache_1_to_1( PImage img, Bool icon)
{
   PDrawableSysData IMG = X((Handle)img);
   unsigned char *data;
   int y;
   register int x;
   int ls = ((img-> w + 31)/32)*4;
   int h = img-> h, w = img-> w;
   static unsigned char bits[256];
   static Bool initialized = false;
   int ils;
   unsigned char *idata;

   if ( icon) {
      ils = PIcon(img)->maskLine;
      idata = PIcon(img)->mask;
   } else {
      ils = img-> lineSize;
      idata = img-> data;
   }

   data = malloc( ls * h);
   if ( !data) croak( "create_image_cache_1_to_1(): no memory");
   if ( guts.bit_order == MSBFirst) {
      for ( y = h-1; y >= 0; y--) {
	 memcpy( ls*(h-y-1)+data, idata+y*ils, ls);
      }
   } else {
      if (!initialized) {
	 unsigned int i, j;
	 int k;
	 for ( i = 0; i < 256; i++) {
	    bits[i] = 0;
	    j = i;
	    for ( k = 0; k < 8; k++) {
	       bits[i] <<= 1;
	       if ( j & 0x1)
		  bits[i] |= 1;
	       j >>= 1;
	    }
	 }
	 initialized = true;
      }
      for ( y = h-1; y >= 0; y--) {
	 register unsigned char *s = idata+y*ils;
	 register unsigned char *t = ls*(h-y-1)+data;
	 for ( x = 0; x < (w+7)/8; x++) {
	    *t++ = bits[*s++];
	 }
      }
   }

   if (icon) {
      IMG-> icon_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
				       1, XYBitmap, 0, data,
				       w, h, 32, ls);
      if (!IMG-> icon_cache) {
	 free( data);
	 croak( "create_image_cache_1_to_1(): error during icon's XCreateImage()");
      }
   } else {
      IMG-> image_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
					1, XYBitmap, 0, data,
					w, h, 32, ls);
      if (!IMG-> image_cache) {
	 free( data);
	 croak( "create_image_cache_1_to_1(): error during XCreateImage()");
      }
      IMG-> bitmap_back =
	 *prima_allocate_color((Handle)img,
                               ARGB(img->palette[0].r,img->palette[0].g,img->palette[0].b));
      IMG-> bitmap_fore =
         *prima_allocate_color((Handle)img,
                               ARGB(img->palette[1].r,img->palette[1].g,img->palette[1].b));
   }
}

static void
create_rgb_to_16_lut( int ncolors, const PRGBColor pal, U16 *lut)
{
   /* XXX make this 3,2,5,11-independent */
   Visual *v = DefaultVisual( DISP, SCREEN);
   unsigned long red_mask, green_mask, blue_mask;
   int i;

   red_mask = v-> red_mask;
   green_mask = v-> green_mask;
   blue_mask = v-> blue_mask;
   for ( i = 0; i < ncolors; i++) {
      lut[i] = 0;
      lut[i] |=
	 (((pal[i]. r >> 3) << 11) & red_mask) & 0xffff;
      lut[i] |=
	 (((pal[i]. g >> 2) << 5) & green_mask) & 0xffff;
      lut[i] |=
	 ((pal[i]. b >> 3) & blue_mask) & 0xffff;
   }
}

static void
calc_shifts_rgb_to_24( unsigned long mask,
                       int *right,
                       int *left)
{
   int bc, l;

   l = 0;
   while (( mask & 1) == 0) { l++; mask >>= 1; }
   bc = 0;
   while ( mask) { bc++; mask >>= 1; }
   *right = 8-bc;
   *left = l;
}

static void
create_rgb_to_24_lut( int ncolors, const PRGBColor pal, unsigned long *lut)
{
   Visual *v = DefaultVisual( DISP, SCREEN);
   unsigned long rmask, gmask, bmask;
   int rrsh, grsh, brsh, rlsh, glsh, blsh;
   int i;

   calc_shifts_rgb_to_24( rmask = v-> red_mask, &rrsh, &rlsh);
   calc_shifts_rgb_to_24( gmask = v-> green_mask, &grsh, &glsh);
   calc_shifts_rgb_to_24( bmask = v-> blue_mask, &brsh, &blsh);
   for ( i = 0; i < ncolors; i++) {
      lut[i] = 0;
      lut[i] |=
	 (((pal[i]. r >> rrsh) << rlsh) & rmask);
      lut[i] |=
	 (((pal[i]. g >> grsh) << glsh) & gmask);
      lut[i] |=
	 (((pal[i]. b >> brsh) << blsh) & bmask);
   }
}

static void
create_image_cache_4_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   U32 lut[ 256];
   U16 lut1[ 16];
   unsigned char *data;
   int x, y;
   int ls = ((img-> w * 16 + 31)/32)*4;
   int h = img-> h, w = img-> w;
   unsigned i;

   create_rgb_to_16_lut( 16, img-> palette, lut1);
   for ( i = 0; i < 256; i++) {
      lut[i] = ((U32)lut1[(i & 0xf0) >> 4]) | (((U32)lut1[(i & 0x0f) >> 0]) << 16);
   }
   data = malloc( ls * h);
   if ( !data) {
      croak( "create_image_cache_4_to_16(): no memory");
   }
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U32 *d = (U32*)(ls*(h-y-1)+data);
      for ( x = 0; x < (w+1)/2; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> image_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
				     guts. depth, ZPixmap, 0, data,
				     w, h, 32, ls);
   if (!IMG-> image_cache) {
      free( data);
      croak( "create_image_cache_4_to_16(): error during XCreateImage()");
   }
}

static void
create_image_cache_8_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   U16 lut[ 256];
   U16 *data;
   int x, y;
   int ls = ((img-> w * 16 + 31)/32)*4;
   int h = img-> h, w = img-> w;

   create_rgb_to_16_lut( img-> palSize, img-> palette, lut);

   data = malloc( ls * h);
   if ( !data) {
      croak( "create_image_cache_8_to_16(): no memory");
   }
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U16 *d = (U16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> image_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
				     guts. depth, ZPixmap, 0, (unsigned char*)data,
				     w, h, 32, ls);
   if (!IMG-> image_cache) {
      free( data);
      croak( "create_image_cache_8_to_16(): error during XCreateImage()");
   }
}

static void
create_image_cache_8_to_24( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   unsigned long lut[ 256];
   U32 *data;
   int x, y;
   int ls = ((img-> w * 24 + 31)/32)*4;
   int h = img-> h, w = img-> w;

   create_rgb_to_24_lut( img-> palSize, img-> palette, lut);

   data = malloc( ls * h);
   if ( !data) {
      croak( "create_image_cache_8_to_24(): no memory");
   }
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U32 *d = (U32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
         ((unsigned char *)d)--;
      }
   }

   IMG-> image_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
				     guts. depth, ZPixmap, 0, (unsigned char*)data,
				     w, h, 32, ls);
   if (!IMG-> image_cache) {
      free( data);
      croak( "create_image_cache_8_to_24(): error during XCreateImage()");
   }
}

static void
create_image_cache_8_to_32( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   unsigned long lut[ 256];
   U32 *data;
   int x, y;
   int ls = ((img-> w * 32 + 31)/32)*4;
   int h = img-> h, w = img-> w;

   create_rgb_to_24_lut( img-> palSize, img-> palette, lut);

   data = malloc( ls * h);
   if ( !data) {
      croak( "create_image_cache_8_to_32(): no memory");
   }
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U32 *d = (U32*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lut[line[x]];
      }
   }

   IMG-> image_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
				     guts. depth, ZPixmap, 0, (unsigned char*)data,
				     w, h, 32, ls);
   if (!IMG-> image_cache) {
      free( data);
      croak( "create_image_cache_8_to_32(): error during XCreateImage()");
   }
}

static void
create_image_cache_24_to_16( PImage img)
{
   PDrawableSysData IMG = X((Handle)img);
   static U16 lur[256], lub[256], lug[256];
   static Bool initialize = true;
   U16 *data;
   int x, y;
   int i;
   RGBColor pal[256];
   int ls = ((img-> w * 16 + 31)/32)*4;
   int h = img-> h, w = img-> w;

   if ( initialize) {
      for ( i = 0; i < 256; i++) {
	 pal[i]. r = i; pal[i]. g = 0; pal[i]. b = 0;
      }
      create_rgb_to_16_lut( 256, pal, lur);
      for ( i = 0; i < 256; i++) {
	 pal[i]. r = 0; pal[i]. g = i; pal[i]. b = 0;
      }
      create_rgb_to_16_lut( 256, pal, lug);
      for ( i = 0; i < 256; i++) {
	 pal[i]. r = 0; pal[i]. g = 0; pal[i]. b = i;
      }
      create_rgb_to_16_lut( 256, pal, lub);
      initialize = false;
   }

   data = malloc( h * ls);
   if ( !data) {
      croak( "create_image_cache_24_to_16(): no memory");
   }
   for ( y = h-1; y >= 0; y--) {
      register unsigned char *line = img-> data + y*img-> lineSize;
      register U16 *d = (U16*)(ls*(h-y-1)+(unsigned char *)data);
      for ( x = 0; x < w; x++) {
	 *d++ = lub[line[0]] | lug[line[1]] | lur[line[2]];
	 line += 3;
      }
   }

   IMG-> image_cache = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
				     guts. depth, ZPixmap, 0, (unsigned char*)data,
				     img-> w, img-> h, 32, ls);
   if (!IMG-> image_cache) {
      free( data);
      croak( "create_image_cache_24_to_16(): error during XCreateImage()");
   }
}


static void
create_image_cache( PImage img, Bool icon)
{
   PDrawableSysData IMG = X((Handle)img);

   if ( !IMG-> image_cache) {
      if ( !img-> palette) {
	 croak( "create_image_cache(): no palette, ouch!");
      }
      switch (img-> type & imBPP) {
      case 1:
	 create_image_cache_1_to_1( img, false);
	 break;
      case 4:
	 switch (guts.depth) {
	 case 16:
	    create_image_cache_4_to_16( img);
	    break;
	 default:
	    croak( "create_image_cache(): unsupported screen depth for 4-bit images");
	 }
	 break;
      case 8:
	 switch (guts.depth) {
	 case 16:
	    create_image_cache_8_to_16( img);
	    break;
         case 24:
            create_image_cache_8_to_24( img);
            break;
         case 32:
            create_image_cache_8_to_32( img);
            break;
	 default:
	    croak( "create_image_cache(): unsupported screen depth for 8-bit images");
	 }
	 break;
      case 24:
	 switch (guts.depth) {
	 case 16:
	    create_image_cache_24_to_16( img);
	    break;
         case 24:
            /* create_image_cache_24_to_24( img); */
            break;
	 default:
	    croak( "create_image_cache(): unsupported screen depth for 24-bit images");
	 }
	 break;
      default:
	 croak( "create_image_cache(): unsupported BPP");
      }
   }
   if ( icon && !IMG-> icon_cache) {
      create_image_cache_1_to_1( img, true);
   }
}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
   DEFXX;
   PDrawableSysData IMG = X(image);
   PImage img = PImage( image);
   unsigned long f = 0, b = 0;
   Bool icon = kind_of((Handle)img, CIcon);
   int func, ofunc;
   XGCValues gcv;

   /* 1) XXX - rop - correct support! */
   /* 2) XXX - Shared Mem Image Extension! */
   create_image_cache( img, icon);
   SHIFT( x, y);
   if ( XGetGCValues( DISP, XX-> gc, GCFunction, &gcv) == 0) {
      warn( "apc_gp_put_image(): XGetGCValues() error");
   }
   ofunc = gcv. function;
   if ( icon) {
      func = GXand;
      f = XX-> fore. pixel;
      b = XX-> back. pixel;
      XSetForeground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
      XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
      if ( func != ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
      XPutImage( DISP, XX-> drawable, XX-> gc, IMG-> icon_cache,
		 xFrom, img-> h - yFrom - yLen,
		 x, REVERT(y) - yLen + 1, xLen, yLen);
      XCHECKPOINT;
      XSetForeground( DISP, XX-> gc, f);
      XSetBackground( DISP, XX-> gc, b);
      func = GXxor;
      if ( func == ofunc)
	 XSetFunction( DISP, XX-> gc, func);
      XCHECKPOINT;
   } else {
      if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
	 func = GXnoop;
      else
	 func = rop_map[ rop];
   }
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, func);
   if (( img-> type & imBPP) == 1) {
      f = XX-> fore. pixel;
      b = XX-> back. pixel;
      XSetForeground( DISP, XX-> gc, IMG-> bitmap_fore. pixel);
      XSetBackground( DISP, XX-> gc, IMG-> bitmap_back. pixel);
      XCHECKPOINT;
   }
   XPutImage( DISP, XX-> drawable, XX-> gc, IMG-> image_cache,
	      xFrom, img-> h - yFrom - yLen,
	      x, REVERT(y) - yLen + 1, xLen, yLen);
   XCHECKPOINT;
   if (( img-> type & imBPP) == 1) {
      XSetForeground( DISP, XX-> gc, f);
      XSetBackground( DISP, XX-> gc, b);
      XCHECKPOINT;
   }
   if ( func != ofunc)
      XSetFunction( DISP, XX-> gc, ofunc);
   return true;
}

Bool
apc_image_begin_paint( Handle self)
{
   DEFXX;
   PImage img = PImage( self);

   XX-> drawable = XCreatePixmap( DISP, RootWindow( DISP, SCREEN), img-> w, img-> h, guts. depth);
   XCHECKPOINT;
   prima_prepare_drawable_for_painting( self);
   apc_gp_put_image( self, self, 0, 0, 0, 0, img-> w, img-> h, ropCopyPut);
   /*                ^^^^^ ^^^^    :-)))  */
   return true;
}

static void
calc_masks_and_lut_16_to_24( unsigned long mask,
			     unsigned long *mask1,
			     unsigned long *mask2,
			     int *bit_count,
			     unsigned char *lut)
{
   unsigned i;
   unsigned long m;
   int bc;

   *mask2 = mask;
   *bit_count = 0;
   while (( *mask2 & 1) == 0) { (*bit_count)++; *mask2 >>= 1; }
   m = *mask2;
   bc = 0;
   while ( m) { bc++; m >>= 1; }
   bc = 8 - bc;
   *mask1 = mask;
   for ( i = 0; i <= *mask2; i++) {
      lut[i] = i << bc;
   }
}

static void
convert_16_to_24( XImage *i, PImage img)
{
   static unsigned char lur[256], lub[256], lug[256];  /* is ``static'' reliable here?? */
   static Bool initialize = true;
   static unsigned long rm1, bm1, gm1, rm2, bm2, gm2;
   static int rbc, bbc, gbc;
   int y, x, h, w;
   U16 *d;
   unsigned char *line;

   if ( initialize) {
      Visual *v = DefaultVisual( DISP, SCREEN);

      calc_masks_and_lut_16_to_24( v-> red_mask, &rm1, &rm2, &rbc, lur);
      calc_masks_and_lut_16_to_24( v-> green_mask, &gm1, &gm2, &gbc, lug);
      calc_masks_and_lut_16_to_24( v-> blue_mask, &bm1, &bm2, &bbc, lub);

      initialize = false;
   }

   h = img-> h; w = img-> w;
   for ( y = 0; y < h; y++) {
      d = (U16 *)(i-> data + (h-y-1)*i-> bytes_per_line);
      line = img-> data + y*img-> lineSize;
      for ( x = 0; x < w; x++) {
	 *line++ = lub[(*d & bm1) >> bbc];
	 *line++ = lug[(*d & gm1) >> gbc];
	 *line++ = lur[(*d & rm1) >> rbc];
	 d++;
      }
   }
}

static void
slurp_image( Handle self, Pixmap px)
{
   int target_depth;
   XImage *i;
   PImage img = PImage( self);

   if ( px) {
      i = XGetImage( DISP, px, 0, 0, img-> w, img-> h, AllPlanes, ZPixmap);
      XCHECKPOINT;

      target_depth = guts. depth;
      if ( target_depth == 16)
	 target_depth = 24;
      if (( img-> type & imBPP) != target_depth) {
	 CImage( self)-> create_empty( self, img-> w, img-> h, target_depth);
      }
      if ( guts. depth != target_depth) {
	 switch ( guts. depth) {
	 case 16:
	    switch ( target_depth) {
	    case 24:
	       convert_16_to_24( i, img);
	       break;
	    default: goto slurp_image_unsupported_depth;
	    }
	    break;
slurp_image_unsupported_depth:
	 default:
	    XDestroyImage( i);
	    croak( "slurp_image(): unsupported depth-target depth combination");
	 }
      } else {
	 /* just copy with care */
      }
   }
}

Bool
apc_image_end_paint( Handle self)
{
   DEFXX;
   slurp_image( self, XX-> drawable);
   prima_cleanup_drawable_after_painting( self);
   if ( XX-> drawable) {
      XFreePixmap( DISP, XX-> drawable);
      XCHECKPOINT;
      XX-> drawable = 0;
   }
   return true;
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XDrawRectangle( DISP, XX-> drawable, XX-> gc, x1, REVERT( y2), x2 - x1, y2 - y1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_sector( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   int sy;
   DEFXX;
   SHIFT( x, y);
   sy = REVERT( y);
   if (( angleEnd > angleStart + 360) && (((int)( angleEnd - angleStart) / 360) % 2) == 1)
      XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
          0, 360 * 64);
   XDrawArc( DISP, XX-> drawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   XDrawLine( DISP, XX-> drawable, XX-> gc,
       x + cos( angleStart / GRAD) * radX, sy - sin( angleStart / GRAD) * radY,
       x, y
   );
   XDrawLine( DISP, XX-> drawable, XX-> gc,
       x, y,
       x + cos( angleEnd / GRAD) * radX,   sy - sin( angleEnd / GRAD) * radY
   );
   return true;
}

Bool
apc_gp_set_palette( Handle self)
{
   DOLBUG( "apc_gp_set_palette()\n");
   return true;
}

Bool
apc_gp_set_region( Handle self, Handle mask)
{
   DOLBUG( "apc_gp_set_region()\n");
   return true;
}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
   DEFXX;
   XColor *c = prima_allocate_color( self, color);
   unsigned long old = XX-> fore. pixel;

   SHIFT( x, y);
   XSetForeground( DISP, XX-> gc, c-> pixel);
   XCHECKPOINT;
   XDrawPoint( DISP, XX-> drawable, XX-> gc, x, REVERT( y));
   XCHECKPOINT;
   XSetForeground( DISP, XX-> gc, old);
   XCHECKPOINT;
   return true;
}

extern void
ic_stretch( Handle, Byte *, int, int, Bool, Bool);

static XImage*
create_stretched_image( PImage img, int x, int y, int w, int h, int tw, int th)
{
   unsigned char *src_data = nil, *dst_data;
   PDrawableSysData IMG = X(img);
   PImage hack;
   int i;
   int ls = (( img->w * guts.depth + 31) / 32) * 4;
   int tls = (( tw * guts.depth + 31) / 32) * 4;
   XImage *r;

   /* XXX this sub is extremely hacky; see Image_stretch and ic_stretch to understand */
   hack = malloc( sizeof( Image));
   if ( !hack) croak( "create_stretched_image(): no memory");
   bzero( hack, sizeof( Image));
   hack-> w = w;
   hack-> h = h;
   hack-> type = guts.depth | imGrayScale;
   hack-> lineSize = (( w * guts.depth + 31) / 32) * 4;
   hack-> dataSize = hack-> lineSize * h;
   if ( x == 0 && y == 0 && w == img->w && h == img->h)
      hack-> data = IMG-> image_cache-> data;
   else {
      src_data = malloc( hack-> dataSize);
      if ( !src_data) croak( "create_stretched_image(): no memory");
      for ( i = 0; i < h; i++) {
	 memcpy( src_data + i*hack-> lineSize, IMG-> image_cache-> data + (i+img->h-y-h+1)*ls + x*guts.depth/8, hack-> lineSize);
      }
      hack-> data = src_data;
   }
   dst_data = malloc( th * tls);
   if ( !dst_data) croak( "create_stretched_image(): no memory");
   ic_stretch((Handle)hack, dst_data, tw, th, tw != w, th != h);
   r = XCreateImage( DISP, DefaultVisual( DISP, SCREEN),
		     guts. depth, ZPixmap, 0, (unsigned char*)dst_data,
		     tw, th, 32, tls);
   free( src_data);
   free( hack);
   if (!r) {
      free( dst_data);
      croak( "create_stretched_image(): error creating XImage");
   }
   return r;
}

Bool
apc_gp_stretch_image( Handle self, Handle image,
		      int x, int y, int xFrom, int yFrom,
		      int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   DEFXX;
   PImage img = PImage( image);
   PDrawableSysData IMG = X(image);
   XImage *stretch;
   unsigned long f = 0, b = 0;

   if ( xDestLen == xLen && yDestLen == yLen) {
      apc_gp_put_image( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
      return true;
   }

   /* 1) XXX - rop - correct support! */
   /* 2) XXX - Shared Mem Image Extension! */
   create_image_cache( img, false);
   SHIFT( x, y);

   /* fprintf( stderr, "x%d y%d xf%d yf%d xdl%d ydl%d xl%d yl%d\n", x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen); */
   if ( x < 0 || y < 0 || xDestLen > XX-> size.x || yDestLen > XX-> size.y) {
      /* optimization might be necessary */
      stretch = create_stretched_image( img, xFrom, yFrom, xLen, yLen, xDestLen, yDestLen);
   } else {
      stretch = create_stretched_image( img, xFrom, yFrom, xLen, yLen, xDestLen, yDestLen);
   }

   if (( img-> type & imBPP) == 1) {
      f = XX-> fore. pixel;
      b = XX-> back. pixel;
      XSetForeground( DISP, XX-> gc, IMG-> bitmap_fore. pixel);
      XSetBackground( DISP, XX-> gc, IMG-> bitmap_back. pixel);
      XCHECKPOINT;
   }
   XPutImage( DISP, XX-> drawable, XX-> gc, stretch,
	      0, 0, x, REVERT(y) - yDestLen + 1, xDestLen, yDestLen);
   XCHECKPOINT;
   XDestroyImage( stretch);
   XCHECKPOINT;
   if (( img-> type & imBPP) == 1) {
      XSetForeground( DISP, XX-> gc, f);
      XSetBackground( DISP, XX-> gc, b);
      XCHECKPOINT;
   }
   return true;
}

Bool
apc_gp_text_out( Handle self, const char* text, int x, int y, int len)
{
   DEFXX;
   SHIFT( x, y);
   XDrawString( DISP, XX-> drawable, XX-> gc, x, REVERT( y), text, len);
   XCHECKPOINT;
   return true;
}

/* gpi settings */
Color
apc_gp_get_back_color( Handle self)
{
   DEFXX;
   XColor c = ( XX-> flags. paint) ? XX-> back : XX-> saved_back;
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
   XColor c = ( XX-> flags. paint) ? XX-> fore : XX-> saved_fore;
   return ARGB( c. red >> 8, c. green >> 8, c. blue >> 8);
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   DEFXX;
   XRectangle cr;
   Rect r;

   cr. x = 0;
   cr. y = 0;
   cr. width = XX-> size.x;
   cr. height = XX-> size.y;
   if ( XX-> flags. paint && ( XX-> region || XX-> stale_region)) {
      prima_rect_intersect( &cr, &XX-> exposed_rect);
      prima_rect_intersect( &cr, &XX-> clip_rect);
   }
   r. left = cr. x;
   r. top = XX-> size. y - cr. y;
   r. bottom = r. top - cr. height;
   r. right = cr. x + cr. width;
   return r;
}

PFontABC
apc_gp_get_font_abc( Handle self, int firstChar, int lastChar)
{
   DEFXX;
   PFontABC abc;
   XFontStruct *fs;
   XCharStruct *cs;
   int k;

   if (!XX-> font) apc_gp_set_font( self, &PDrawable( self)-> font);
   fs = XQueryFont( DISP, XX-> font-> id);
   if (!fs) return nil;

   abc = malloc( sizeof( FontABC) * (lastChar - firstChar + 1));
   for ( k = firstChar; k <= lastChar; k++) {
      if ( !fs-> per_char)
	 cs = &fs-> min_bounds;
      else if ( k < fs-> min_char_or_byte2 || k > fs-> max_char_or_byte2)
	 cs = fs-> per_char + fs-> default_char - fs-> min_char_or_byte2;
      else
	 cs = fs-> per_char + k - fs-> min_char_or_byte2;
      abc[k]. a = cs-> lbearing;
      abc[k]. b = cs-> rbearing - cs-> lbearing;
      abc[k]. c = cs-> width - cs-> rbearing;
   }
   XFreeFontInfo( nil, fs, 1);
   return abc;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
   return &(X(self)-> fill_pattern);
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
      if ( XX-> flags. zero_line)
	 w = 0;
      else {
	 if ( XGetGCValues( DISP, XX-> gc, GCLineWidth, &gcv) == 0) {
	    warn( "apc_gp_get_line_width(): XGetGCValues() error");
	 }
	 w = gcv. line_width;
      }
   } else {
      if ( XX-> flags. saved_zero_line)
	 w = 0;
      else
	 w = XX-> gcv. line_width;
   }
   return w;
}

int
apc_gp_get_line_pattern( Handle self, char *dashes)
{
   DEFXX;
   int n;

   if ( XX-> flags. paint) {
      n = XX-> paint_ndashes;
      memcpy( dashes, XX-> paint_dashes, n);
   } else {
      n = XX-> ndashes;
      if ( n < 0) {
	 n = 0;
	 strcpy( dashes, "");
      } else if ( n == 0) {
	 n = 1;
	 strcpy( dashes, "\1");
      } else {
	 memcpy( dashes, XX-> paint_dashes, n);
      }
   }
   return n;
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
      return XX-> paint_rop;
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

Bool
apc_gp_get_text_out_baseline( Handle self)
{
   DOLBUG( "apc_gp_get_text_out_baseline()\n");
   return false;
}

Bool
apc_gp_set_back_color( Handle self, Color color)
{
   DEFXX;
   XColor *c = prima_allocate_color( self, color);

   if ( XX-> flags. paint) {
      XX-> back = *c;
      XSetBackground( DISP, XX-> gc, c-> pixel);
      XCHECKPOINT;
   } else {
      XX-> saved_back = *c;
      XX-> gcv. background = c-> pixel;
   }
   return true;
}

Bool
apc_gp_set_clip_rect( Handle self, Rect clipRect)
{
   DEFXX;
   Region region;
   XRectangle r;

   if ( !XX-> flags. paint)
      return true;

   SORT( clipRect. left, clipRect. right);
   SORT( clipRect. bottom, clipRect. top);
   r. x = clipRect. left;
   r. y = REVERT( clipRect. top) + 1;
   r. width = clipRect. right - clipRect. left;
   r. height = clipRect. top - clipRect. bottom;
   XX-> clip_rect = r;
   region = XCreateRegion();
   XUnionRectWithRegion( &r, region, region);
   if ( XX-> stale_region) {
      XIntersectRegion( region, XX-> stale_region, region);
   }
   XSetRegion( DISP, XX-> gc, region);
   XDestroyRegion( region);
   return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
   DEFXX;
   XColor *c = prima_allocate_color( self, color);

   if ( XX-> flags. paint) {
      XX-> fore = *c;
      XSetForeground( DISP, XX-> gc, c-> pixel);
      XCHECKPOINT;
   } else {
      XX-> saved_fore = *c;
      XX-> gcv. foreground = c-> pixel;
   }
   return true;
}

Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   DEFXX;

   memcpy( XX-> fill_pattern, pattern, sizeof( FillPattern));
   DOLBUG( "apc_gp_set_fill_pattern()\n");
   return true;
}

/*- see apc_font.c
void
apc_gp_set_font( Handle self, PFont font)
*/

Bool
apc_gp_set_line_end( Handle self, int lineEnd)
{
   DOLBUG( "apc_gp_set_line_end()\n");
   return true;
}

Bool
apc_gp_set_line_width( Handle self, int line_width)
{
   DEFXX;
   XGCValues gcv;
   int zero_line = line_width == 0;

   if ( zero_line)
      line_width = 1;

   if ( XX-> flags. paint) {
      XX-> flags. zero_line = zero_line;
      gcv. line_width = line_width;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
      XCHECKPOINT;
   } else {
      XX-> flags. saved_zero_line = zero_line;
      XX-> gcv. line_width = line_width;
   }
   return true;
}

Bool
apc_gp_set_line_pattern( Handle self, char *pattern, int len)
{
   DEFXX;
   XGCValues gcv;

   if ( XX-> flags. paint) {
      if ( len == 0 || (len == 1 && pattern[0] == 1)) {
	 gcv. line_style = LineSolid;
	 XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      } else {
	 gcv. line_style = LineOnOffDash;
	 XSetDashes( DISP, XX-> gc, 0, pattern, len);
	 XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      }
      free(XX->paint_dashes);
      XX-> paint_dashes = malloc( len);
      memcpy( XX-> paint_dashes, pattern, len);
      XX-> paint_ndashes = len;
   } else {
      free( XX-> dashes);
      if ( len == 0) {					/* lpNull */
	 XX-> dashes = nil;
	 XX-> ndashes = -1;
	 XX-> gcv. line_style = LineSolid;
      } else if ( len == 1 && pattern[0] == 1) {	/* lpSolid */
	 XX-> dashes = nil;
	 XX-> ndashes = 0;
	 XX-> gcv. line_style = LineSolid;
      } else {						/* the rest */
	 XX-> dashes = malloc( len);
	 memcpy( XX-> dashes, pattern, len);
	 XX-> ndashes = len;
	 XX-> gcv. line_style = LineOnOffDash;
      }
   }
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

   if ( XX-> flags. paint) {
      XX-> paint_rop = rop;
      XSetFunction( DISP, XX-> gc, function);
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
   DOLBUG( "apc_gp_set_rop2()\n");
   return true;
}

Bool
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
   return true;
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
   DOLBUG( "apc_gp_set_text_opaque()\n");
   return true;
}

Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
   DOLBUG( "apc_gp_set_text_out_baseline()\n");
   return true;
}
