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

#define SORT(a,b)	({ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }})
#define REVERT(a)	({ XX-> size. y - (a) - 1; })
#define SHIFT(a,b)	({ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; })

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

int
prima_rop_map( int rop)
{
   if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
      return GXnoop;
   else
      return rop_map[ rop];
}

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
   /* super duper debug
   static XColor *black = nil;
   static XColor *white = nil;
   static int i;
   */

   if ( !globalColors) {
      globalColors = hash_create();
      globalColormap = DefaultColormap( DISP, SCREEN);
   }

   /* super duper debug
   if ( !black) {
      black = malloc( sizeof( XColor));
      black-> red = 0;
      black-> green = 0;
      black-> blue = 0;
      black-> flags = DoRed | DoGreen | DoBlue;
      black-> pixel = 0;
      XAllocColor( DISP, globalColormap, black);
      white = malloc( sizeof( XColor));
      white-> red = 0xffff;
      white-> green = 0xffff;
      white-> blue = 0xffff;
      white-> flags = DoRed | DoGreen | DoBlue;
      white-> pixel = 0;
      XAllocColor( DISP, globalColormap, white);
   }
   i++;
   return  i%2 ? black : white;
   */

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

   if ( XX-> udrawable && is_opt( optBuffered)) {
      int w, h;
      if ( XX-> region) {
         XX-> bsize. x = w = XX-> exposed_rect. width;
         XX-> bsize. y = h = XX-> exposed_rect. height;
         XX-> btransform. x = - XX-> exposed_rect. x;
         XX-> btransform. y = XX-> exposed_rect. y;
      } else {
         XX-> bsize. x = w = XX-> size. x;
         XX-> bsize. y = h = XX-> size. y;
         XX-> btransform. x = 0;
         XX-> btransform. y = 0;
      } 
      XX-> gdrawable = XCreatePixmap( DISP, XX-> udrawable, w, h, guts.depth);
      if (!XX-> gdrawable)
         XX-> gdrawable = XX-> udrawable;
   } else if ( XX-> udrawable && !XX-> gdrawable) {
      XX-> gdrawable = XX-> udrawable;
      XX-> btransform. x = 0;
      XX-> btransform. y = 0;
   }

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
      if ( XX-> btransform. x != 0 || XX-> btransform. y != 0) {
         Region r = XCreateRegion();
         XUnionRegion( r, XX-> region, r);
         XOffsetRegion( r, XX-> btransform. x, -XX-> btransform. y);
         XSetRegion( DISP, XX-> gc, r);
         XDestroyRegion( r);
      } else {
         XSetRegion( DISP, XX-> gc, XX-> region);
      }
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
   if ( XX-> udrawable && XX-> udrawable != XX-> gdrawable && XX-> gdrawable) {
      if ( XX-> stale_region && ( XX-> clip_rect. x != 0
                                  || XX-> clip_rect. y != 0
                                  || XX-> clip_rect. width != XX-> size.x
                                  || XX-> clip_rect. height != XX-> size.y)) {
         //XOffsetRegion( XX-> stale_region, -XX-> btransform. x, XX-> btransform. y);
         XSetRegion( DISP, XX-> gc, XX-> stale_region);
         XCHECKPOINT;
      }
      XCopyArea( DISP, XX-> gdrawable, XX-> udrawable, XX-> gc, 
                 0, 0, 
                 XX-> bsize.x, XX-> bsize.y, 
                 -XX-> btransform. x, XX-> btransform. y);
      XCHECKPOINT;
      XFreePixmap( DISP, XX-> gdrawable);
      XCHECKPOINT;
      XX-> gdrawable = XX-> udrawable;
   }
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
   XFlush(DISP);
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
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
          0, 360 * 64);
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   return true;
}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_clear( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
      x1 = 0; y1 = 0;
      x2 = XX-> size. x - 1;
      y2 = XX-> size. y - 1;
   }
   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XSetForeground( DISP, XX-> gc, XX-> back. pixel);
   XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
   XSetForeground( DISP, XX-> gc, XX-> fore. pixel);
   XCHECKPOINT;
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
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
          0, 360 * 64);
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   XDrawLine( DISP, XX-> gdrawable, XX-> gc,
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
   int x = XX-> gtransform. x + XX-> btransform. x;
   int y = XX-> size. y - 1 - XX-> gtransform. y - XX-> btransform. y;
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

   XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, n, CoordModeOrigin);

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
/*    XDrawLines( DISP, XX-> gdrawable, XX-> gc, p+i, n-i+1, CoordModeOrigin); */
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
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2, 0, 64*360);
   return true;
}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd)
{
   DEFXX;
   SHIFT( x, y);
   XSetArcMode( DISP, XX-> gc, ArcChord);
   XFillArc( DISP, XX-> gdrawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   return true;
}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y, int radX, int radY)
{
   DEFXX;
   SHIFT( x, y);
   XFillArc( DISP, XX-> gdrawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2, 0, 64*360);
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
      p[i]. x = (short)points[i]. x + XX-> gtransform. x + XX-> btransform. x;
      p[i]. y = (short)REVERT(points[i]. y + XX-> gtransform. y + XX-> btransform. y);
   }
   p[numPts]. x = (short)points[0]. x + XX-> gtransform. x + XX-> btransform. x;
   p[numPts]. y = (short)REVERT(points[0]. y + XX-> gtransform. y + XX-> btransform. y);

   if ( guts. limits. XFillPolygon >= numPts) {
      XFillPolygon( DISP, XX-> gdrawable, XX-> gc, p, numPts, ComplexShape, CoordModeOrigin);
      XCHECKPOINT;
   } else {
      warn( "apc_gp_fill_poly() XFillPolygon() request size limit reached");
   }
   if ( guts. limits. XDrawLines > numPts) {
      XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, numPts+1, CoordModeOrigin);
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
   XFillArc( DISP, XX-> gdrawable, XX-> gc, x - radX, REVERT( y) - radY, radX * 2, radY * 2,
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
   XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
   return true;
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XDrawRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1, y2 - y1);
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
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
          0, 360 * 64);
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - radX, sy - radY, radX * 2, radY * 2,
       angleStart * 64, ( angleStart + angleEnd) * 64);
   XDrawLine( DISP, XX-> gdrawable, XX-> gc,
       x + cos( angleStart / GRAD) * radX, sy - sin( angleStart / GRAD) * radY,
       x, y
   );
   XDrawLine( DISP, XX-> gdrawable, XX-> gc,
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
   XDrawPoint( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y));
   XCHECKPOINT;
   XSetForeground( DISP, XX-> gc, old);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_text_out( Handle self, const char* text, int x, int y, int len)
{
   DEFXX;
   SHIFT( x, y);
   XDrawString( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y), text, len);
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

void
prima_gp_get_clip_rect( Handle self, XRectangle *cr)
{
   DEFXX;

   cr-> x = 0;
   cr-> y = 0;
   cr-> width = XX-> size.x;
   cr-> height = XX-> size.y;
   if ( XX-> flags. paint && ( XX-> region || XX-> stale_region)) {
      prima_rect_intersect( cr, &XX-> exposed_rect);
   }
   if ( XX-> clip_rect. x != 0
        || XX-> clip_rect. y != 0
        || XX-> clip_rect. width != XX-> size.x
        || XX-> clip_rect. height != XX-> size.y) {
      prima_rect_intersect( cr, &XX-> clip_rect);
   }
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   DEFXX;
   XRectangle cr;
   Rect r;

   prima_gp_get_clip_rect( self, &cr);
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
      return false;

   SORT( clipRect. left, clipRect. right);
   SORT( clipRect. bottom, clipRect. top);
   r. x = clipRect. left;
   r. y = REVERT( clipRect. top);
   r. width = clipRect. right - clipRect. left+1;
   r. height = clipRect. top - clipRect. bottom+1;
   XX-> clip_rect = r;
   {
      XRectangle rr = XX-> exposed_rect;
      prima_rect_intersect( &rr, &r);
   }
   region = XCreateRegion();
   XUnionRectWithRegion( &r, region, region);
   if ( XX-> stale_region) {
      XIntersectRegion( region, XX-> stale_region, region);
   }
   if ( XX-> btransform. x != 0 || XX-> btransform. y != 0) {
      XOffsetRegion( region, XX-> btransform. x, -XX-> btransform. y);
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
