/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
 *
 * $Id$
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
#define SHIFT(a,b)	({ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; })
#define REVERSE_BYTES_32(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))
#define REVERSE_BYTES_24(x) ((((x)&0xff)<<16) | ((x)&0xff00) | (((x)&0xff0000)>>8))
#define REVERSE_BYTES_16(x) ((((x)&0xff)<<8 ) | (((x)&0xff00)>>8))

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
      warn( "UAG_001: illegal color class: %08x", class);
      return RGB_BLACK;
   }
   if ( index < 0 || index > MAX_COLOR_INDEX) {
      warn( "UAG_002: illegal color index: %d", index);
      return RGB_BLACK;
   }
   return standard_colors[ cls][ index];
}

static Color 
map_color( Color color)
{
   RGBColor r;
   if ( color >= 0) return color;
   /* XXX - remove this -1: */
   r = get_standard_color(color, (int)((unsigned long)color & ~(unsigned long)(wcMask|0x80000000))-1);
   return ( r.r << 16) | (r.g << 8) | r.b;
}   

Color
apc_widget_map_color( Handle self, Color color)
{
   if ((color < 0) && (( color & wcMask) == 0)) color |= PWidget(self)->widgetClass;
   return map_color( color);
}   


static PHash globalColors = nil;
static Colormap globalColormap;

XColor*
prima_allocate_color( Handle self, Color color)
{
   RGBColor c;
   XColor *x_color;
   static XColor bitmap_white = { pixel: 1, red: 0xffff, green: 0xffff, blue: 0xffff};
   static XColor bitmap_black = { pixel: 0, red: 0x0000, green: 0x0000, blue: 0x0000};

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

   if ( self == nilHandle ||
        ( XT_IS_IMAGE(X(self)) && (PImage(self)-> type & imBPP) == 1) ||
        XT_IS_BITMAP(X(self))
      ) {
      if ((unsigned short)c.r + (unsigned short)c.g + (unsigned short)c.b > 381)
         return &bitmap_white;
      else
         return &bitmap_black;
   }

   x_color = hash_fetch( globalColors, &c, sizeof(c));
   if ( !x_color) {
      Status r;
      x_color = malloc( sizeof( XColor));
      if ( !x_color) {
	 /* XXX better free existing colors... */
	 croak( "UAG_003: no memory");
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
   Bool bitmap;
   struct gc_head *gc_pool;

   if ( XX-> gc && XX-> gcl) return;

   if ( XX-> gc || XX-> gcl) croak( "UAG_004: internal error");

   bitmap = XT_IS_BITMAP(XX) ? true : false;
   gc_pool = bitmap ? &guts.bitmap_gc_pool : &guts.screen_gc_pool;
   XX->gcl = TAILQ_FIRST(gc_pool);
   if (XX->gcl)
      TAILQ_REMOVE(gc_pool, XX->gcl, gc_link);
   if (!XX->gcl) {
      XX->gcl = alloc1z( GCList);
      XX->gcl->gc = XCreateGC( DISP, bitmap ? XX-> gdrawable : RootWindow( DISP, SCREEN), 0, &gcv);
      XCHECKPOINT;
   }
   XX->gc = XX->gcl->gc;
}

void
prima_release_gc( PDrawableSysData selfxx)
{
   Bool bitmap;
   struct gc_head *gc_pool;

   if ( XX-> gc) {
      if ( XX-> gcl == nil) croak( "UAG_005: internal error");
      bitmap = XT_IS_BITMAP(XX) ? true : false;
      gc_pool = bitmap ? &guts.bitmap_gc_pool : &guts.screen_gc_pool;
      TAILQ_INSERT_HEAD(gc_pool, XX->gcl, gc_link);
      XX->gcl = nil;
      XX->gc = nil;
   } else {
      if ( XX-> gcl) croak( "UAG_006: internal error");
   }
}


void
prima_prepare_drawable_for_painting( Handle self)
{
   DEFXX;
   unsigned long mask = VIRGIN_GC_MASK;
   int w, h;
   XRectangle r;

   if ( XX-> udrawable && is_opt( optBuffered)) {
      if ( XX-> region) {
         XClipBox( XX-> region, &r);
         XX-> bsize. x = w = r. width;
         XX-> bsize. y = h = r. height;
         XX-> btransform. x = - r. x;
         XX-> btransform. y = r. y;
      } else {
         XX-> bsize. x = w = XX-> size. x;
         XX-> bsize. y = h = XX-> size. y;
         XX-> btransform. x = 0;
         XX-> btransform. y = 0;
      }
      XX-> gdrawable = XCreatePixmap( DISP, XX-> udrawable, w, h, guts.depth);
      XCHECKPOINT;
      if (!XX-> gdrawable) goto Unbuffered;
   } else if ( XX-> udrawable && !XX-> gdrawable) {
Unbuffered:
      XX-> gdrawable = XX-> udrawable;
      XX-> btransform. x = 0;
      XX-> btransform. y = 0;
   }

   XX-> paint_rop = XX-> rop;
   XX-> paint_rop2 = XX-> rop2;
   XX-> flags. paint_base_line = XX-> flags. base_line;
   XX-> flags. paint_opaque    = XX-> flags. opaque;
   XX-> saved_font = PDrawable( self)-> font;
   XX-> fore = XX-> saved_fore;
   XX-> back = XX-> saved_back;
   XX-> flags. zero_line = XX-> flags. saved_zero_line;
   XX-> gcv. clip_mask = None;
   XX-> gtransform = XX-> transform;

   prima_get_gc( XX);
   XX-> gcv. subwindow_mode = (self == application ? IncludeInferiors : ClipByChildren);
   
   XChangeGC( DISP, XX-> gc, mask, &XX-> gcv);
   XCHECKPOINT;
   
   if ( XX-> dashes) {
      XSetDashes( DISP, XX-> gc, 0, XX-> dashes, XX-> ndashes);
      XX-> paint_ndashes = XX-> ndashes;
      XX-> paint_dashes = malloc( XX-> ndashes);
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

   XF_IN_PAINT(XX) = true;

   memcpy( XX-> saved_fill_pattern, XX-> fill_pattern, sizeof( FillPattern));
   XX-> fill_pattern[0]++; // force 
   apc_gp_set_fill_pattern( self, XX-> saved_fill_pattern);

   if ( !XX-> flags. reload_font && XX-> font && XX-> font-> id) {
      // fprintf( stderr, "set font g: %s\n", XX-> font-> load_name);
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
      if ( XX-> stale_region) {
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
   memcpy( XX-> fill_pattern, XX-> saved_fill_pattern, sizeof( FillPattern));
   if ( XX-> fp_pixmap != None) {
      XFreePixmap( DISP, XX-> fp_pixmap);
      XX-> fp_pixmap = None;
   }   
   if ( XX-> font && ( --XX-> font-> refCnt <= 0)) {
      prima_free_rotated_entry( XX-> font);
      XX-> font-> refCnt = 0;
   }
   free(XX->paint_dashes);
   XX-> paint_dashes = nil;
   XX-> paint_ndashes = 0;
   XF_IN_PAINT(XX) = false;
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

static int
arc_completion( double * angleStart, double * angleEnd, int * needFigure)
{
   int max;
   double diff = ((long)( fabs( *angleEnd - *angleStart) * 1000 + 0.5)) / 1000;

   if ( diff == 0) {
      *needFigure = false;
      return 0;
   }

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

#define ELLIPSE_RECT x - ( dX + 1) / 2 + 1, y - dY / 2, dX - 1, dY - 1


Bool
apc_gp_arc( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   DEFXX;
   int compl, needf;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_007: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   y = REVERT( y);
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   while ( compl--)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 360 * 64);
   if ( needf)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
          angleStart * 64, ( angleEnd - angleStart) * 64);
   return true;
}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_008: put begin_paint somewhere");
      return false;
   }

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

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_009: put begin_paint somewhere");
      return false;
   }

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
apc_gp_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   int compl, needf;
   DEFXX;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_010: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   y = REVERT( y);
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   while ( compl--)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 360 * 64);
   if ( !needf) return true;
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
       angleStart * 64, ( angleEnd - angleStart) * 64);
   XDrawLine( DISP, XX-> gdrawable,
      XX-> gc, x + cos( angleStart / GRAD) * dX / 2, y - sin( angleStart / GRAD) * dY / 2,
               x + cos( angleEnd / GRAD) * dX / 2,   y - sin( angleEnd / GRAD) * dY / 2);
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

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_011: put begin_paint somewhere");
      return false;
   }

   if ((p = malloc( sizeof( XPoint)*n)) == nil)
      croak( "UAG_012: no memory");

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
apc_gp_draw_poly2( Handle self, int np, Point *pp)
{
   DEFXX;
   int i;
   int x = XX-> gtransform. x + XX-> btransform. x;
   int y = XX-> size. y - 1 - XX-> gtransform. y - XX-> btransform. y;
   XSegment *s;
   int n = np / 2;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_013: put begin_paint somewhere");
      return false;
   }

   if ((s = malloc( sizeof( XSegment)*n)) == nil)
      croak( "UAG_014: no memory");

   for ( i = 0; i < n; i++) {
      s[i].x1 = pp[i*2].x + x;
      s[i].y1 = y - pp[i*2].y;
      s[i].x2 = pp[i*2+1].x + x;
      s[i].y2 = y - pp[i*2+1].y;
   }

   if ( XX-> flags. zero_line) {
      XGCValues gcv;
      gcv. line_width = 0;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
   }

   XDrawSegments( DISP, XX-> gdrawable, XX-> gc, s, n);

   if ( XX-> flags. zero_line) {
      XGCValues gcv;
      gcv. line_width = 1;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
   }

   free( s);
   return true;
}

Bool
apc_gp_ellipse( Handle self, int x, int y, int dX, int dY)
{
   DEFXX;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_015: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   y = REVERT( y);
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 64*360);
   return true;
}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   DEFXX;
   int compl, needf;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_016: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   y = REVERT( y);

   XSetArcMode( DISP, XX-> gc, ArcChord);
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   while ( compl--)
      XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);

   if ( needf)
      XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY,
          angleStart * 64, ( angleEnd - angleStart) * 64);
   return true;
}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y,  int dX, int dY)
{
   DEFXX;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_017: put begin_paint somewhere");
      return false;
   }
   SHIFT( x, y);
   y = REVERT( y);
   XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
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

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_018: put begin_paint somewhere");
      return false;
   }

   if ( numPts >= size) {
      free( p);
      size = numPts + 1;
      p = malloc( size * sizeof( XPoint));
      if ( !p) croak( "UAG_019: no memory");
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
      warn( "UAG_020: request too large");
   }
   if ( guts. limits. XDrawLines > numPts) {
      XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, numPts+1, CoordModeOrigin);
      XCHECKPOINT;
   } else {
      warn( "UAG_021: request too large");
   }
   return true;
}

Bool
apc_gp_fill_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   DEFXX;
   int compl, needf;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_022: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   y = REVERT( y);
   XSetArcMode( DISP, XX-> gc, ArcPieSlice);

   compl = arc_completion( &angleStart, &angleEnd, &needf);
   while ( compl--)
      XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);

   if ( needf)
      XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY,
         angleStart * 64, ( angleEnd - angleStart) * 64);
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
color_to_pixel( Color color)
{
   uint32_t pv;
   RGBLUTEntry * lut;
   int depth = get_pixel_depth( guts. idepth);

   switch ( depth) {
   case 1:
      pv = color ? 1 : 0;
      break;
   case 4:
   case 8:
      warn("UAG_034: Not supported pixel depth");
      return 0;
   case 16:   
   case 24:   
   case 32:   
      lut = prima_rgblut(); 
      pv = 
         ((( color & 0xFF)     >>        lut[2]. revShift)  << lut[2].shift) |
         ((( color & 0xFF00)   >> ( 8 +  lut[1]. revShift)) << lut[1].shift) |
         ((( color & 0xFF0000) >> ( 16 + lut[0]. revShift)) << lut[0].shift);
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
      warn("UAG_034: Not supported pixel depth");
      return 0;
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
      fs-> i = nil;
   }   
   
   y -= fs-> first;
   
   if ( fs-> lists[y] == nil)
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
      while ( x < xr) {
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
   
   if ( !opt_InPaint) return false;
   
   s. singleBorder = singleBorder;
   s. drawable     = XX-> gdrawable;
   s. gc           = XX-> gc;
   SHIFT( x, y);
   y = REVERT( y);
   color = map_color( color);
   prima_gp_get_clip_rect( self, &cr);

   s. clip. left   = cr. x;
   s. clip. top    = cr. y;
   s. clip. right  = cr. x + cr. width  - 1;
   s. clip. bottom = cr. y + cr. height - 1;
   if ( cr. width <= 0 || cr. height <= 0) return false;
   s. i = nil;
   
   s. color = color_to_pixel( color);
   s. depth = get_pixel_depth( guts. idepth);
   
   switch( s. depth) { case 4: case 8: return false;}   /* XXX */

   s. first = s. clip. top;
   s. lists = malloc(( s. clip. bottom - s. clip. top + 1) * sizeof( void*));
   bzero( s. lists, ( s. clip. bottom - s. clip. top + 1) * sizeof( void*));

   if ( fs_get_pixel( &s, x, y)) {
      fill( &s, x, y, -1, x, x);
      ret = true;
   }   

   if ( s. i) XDestroyImage( s. i);

   for ( x = 0; x < s. clip. bottom - s. clip. top + 1; x++)
      if ( s. lists[x]) 
         plist_destroy( s.lists[x]);
   free( s. lists);

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
   static RGBLUTEntry * lut = nil;

   if ( !opt_InPaint) return clInvalid;
   SHIFT( x, y);

   if ( x < 0 || x >= XX-> size.x || y < 0 || y >= XX-> size.y)
      return clInvalid;

   if ( XT_IS_DBM(XX)) {
      pixmap = XT_IS_PIXMAP(XX) ? true : false;
   } else {
      pixmap = guts. idepth > 1;
   }   
   
   im = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y - y - 1, 1, 1, 
                   pixmap ? AllPlanes : 1,
                   pixmap ? ZPixmap   : XYPixmap);
   XCHECKPOINT;
   if ( !im) return clInvalid;

   if ( pixmap) {
      switch ( guts. idepth) {
      case 4:
      case 8:
         croak( "UAG_023: not implemented");
      case 16:
         p32 = *(( uint16_t*)(im-> data));
         if ( guts.machine_byte_order != guts.byte_order) 
            p32 = REVERSE_BYTES_16(p32);
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
         if ( !lut) lut = prima_rgblut();
         c = 
              lut[2]. lut[(p32 & lut[2]. mask) >> lut[2]. shift] | 
            ( lut[1]. lut[(p32 & lut[1]. mask) >> lut[1]. shift] << 8) | 
            ( lut[0]. lut[(p32 & lut[0]. mask) >> lut[0]. shift] << 16);
         break;
      }   
   } else {
      if ( im-> data[0] & (( guts.bit_order == MSBFirst) ? 0x80 : 1))
         c = 0xffffff;
      else
         c = 0;
   }   

   XDestroyImage( im);
   return c;
}

Color
apc_gp_get_nearest_color( Handle self, Color color)
{
   DOLBUG("apc_gp_get_nearest_color");
   return color;
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

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_025: put begin_paint somewhere");
      return false;
   }

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

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_026: put begin_paint somewhere");
      return false;
   }

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   XDrawRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1, y2 - y1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_sector( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
   int compl, needf;
   DEFXX;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_027: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   y = REVERT( y);

   compl = arc_completion( &angleStart, &angleEnd, &needf);
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
   DEFXX;
   Pixmap px;
   ImageCache *cache;
   PImage img;
   GC gc;
   XGCValues gcv;

   if ( !XF_IN_PAINT(XX)) {
      if (mask) warn( "UAG_028: not implemented");
      return false;
   }

   if (mask == nilHandle) {
      px = None;
   } else {
      img = PImage(mask);
      cache = prima_create_image_cache(img, nilHandle);
      px = XCreatePixmap(DISP, RootWindow( DISP, SCREEN), img->w, img->h, 1);
      gc = XCreateGC(DISP, px, 0, &gcv);
      prima_put_ximage(px, gc, cache->image, 0, 0, 0, 0, img->w, img->h);
      XFreeGC( DISP, gc);
   }
   XSetClipMask(DISP, XX->gc, px);
   if ( px != None) {
      XFreePixmap( DISP, px);
   }
   return true;
}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
   DEFXX;
   XColor *c = prima_allocate_color( self, color);
   unsigned long old = XX-> fore. pixel;

   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_029: put begin_paint somewhere");
      return false;
   }

   SHIFT( x, y);
   XSetForeground( DISP, XX-> gc, c-> pixel);
   XCHECKPOINT;
   XDrawPoint( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y));
   XCHECKPOINT;
   XSetForeground( DISP, XX-> gc, old);
   XCHECKPOINT;
   return true;
}

static Point
gp_get_text_overhangs( Handle self, const char *text, int len)
{
   DEFXX;
   Point ret;
   if ( len > 0) {
      XCharStruct * cs;
      int index = text[0];
      if ( index < XX-> font-> fs-> min_char_or_byte2) 
         index = XX-> font-> fs-> default_char;
      if ( index > XX-> font-> fs-> max_char_or_byte2) 
         index = XX-> font-> fs-> default_char;
      cs = XX-> font-> fs-> per_char ? XX-> font-> fs-> per_char + index 
       - XX-> font-> fs-> min_char_or_byte2 : &(XX-> font-> fs-> min_bounds);
      ret. x = ( cs-> lbearing < 0) ? - cs-> lbearing : 0;

      index = text[len-1];
      if ( index < XX-> font-> fs-> min_char_or_byte2) 
         index = XX-> font-> fs-> default_char;
      if ( index > XX-> font-> fs-> max_char_or_byte2) 
         index = XX-> font-> fs-> default_char;
      cs = XX-> font-> fs-> per_char ? XX-> font-> fs-> per_char + index 
       - XX-> font-> fs-> min_char_or_byte2 : &(XX-> font-> fs-> min_bounds);
      ret. y = (( cs-> width - cs-> rbearing) < 0) ? cs-> rbearing - cs-> width : 0;
   } else
      ret. x = ret. y = 0;
   return ret;
}   

static Bool
gp_text_out_rotated( Handle self, const char* text, int x, int y, int len) 
{
   DEFXX;
   int i;
   PRotatedFont r;
   XCharStruct *cs;
   int px, py = ( XX-> flags. paint_base_line) ?  XX-> font-> font. descent : 0; 
   int ax = 0, ay = 0;
   int psx, psy, dsx, dsy;
   Fixed rx, ry;

   if ( !prima_update_rotated_fonts( XX-> font, ( char*) text, len, PDrawable( self)-> font. direction, &r)) 
      return false;

   for ( i = 0; i < len; i++) {
      unsigned char index = ( unsigned char) text[i];
      /* acquire actual character index */
      if ( index < r-> first || index >= r-> first + r-> length) {
         if ( r-> defaultChar < 0) continue;
         index = ( unsigned char) r-> defaultChar;
      }
      if ( r-> map[index] == nil) continue;
      cs = XX-> font-> fs-> per_char ? XX-> font-> fs-> per_char + index 
         - XX-> font-> fs-> min_char_or_byte2 : &(XX-> font-> fs-> min_bounds);
  
      /* find reference point in pixmap */
      px = ( cs-> lbearing < 0) ? -cs-> lbearing : 0;
      rx. l = px * r-> cos2. l - py * r-> sin2. l + 0x8000;
      ry. l = px * r-> sin2. l + py * r-> cos2. l + 0x8000;
      psx = rx. i. i - r-> shift. x;
      psy = ry. i. i - r-> shift. y;
      
      /* find glyph position */
      rx. l = ax * r-> cos2. l - ay * r-> sin2. l + 0x8000;
      ry. l = ax * r-> sin2. l + ay * r-> cos2. l + 0x8000;
      dsx = x + rx. i. i - psx;
      dsy = REVERT( y + ry. i. i) + psy - r-> dimension. y + 1;

      /*       
      printf("shift %d %d\n", r-> shift.x, r-> shift.y);            
      printf("point ref: %d %d => %d %d. dims: %d %d, [%d %d %d]\n", px, py, psx, psy, r-> dimension.x, r-> dimension.y, 
           cs-> lbearing, cs-> rbearing - cs-> lbearing, cs-> width - cs-> rbearing);
      printf("plot ref: %d %d => %d %d\n", ax, ay, rx.i.i, ry.i.i);
      printf("at: %d %d ( sz = %d), dest: %d %d\n", x, y, XX-> size.y, dsx, dsy);
      */
      
/*   GXandReverse   ropNotDestAnd */		/* dest = (!dest) & src */
/*   GXorReverse    ropNotDestOr */		/* dest = (!dest) | src */
/*   GXequiv        ropNotSrcXor */		/* dest ^= !src */
/*   GXandInverted  ropNotSrcAnd */		/* dest &= !src */
/*   GXorInverted   ropNotSrcOr */		/* dest |= !src */
/*   GXnand         ropNotAnd */		/* dest = !(src & dest) */
/*   GXnor          ropNotOr */		/* dest = !(src | dest) */
/*   GXinvert       ropInvert */		/* dest = !dest */
      
      switch ( XX-> paint_rop) { /* XXX Limited set edition - either expand to full list or find new way to display bitmaps */
      case ropXorPut:  
         XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXxor); 
         break;
      case ropOrPut:   
         XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXor);
         break;
      case ropAndPut:  
         XSetBackground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXand);
         break;
      case ropNotPut:   
      case ropBlackness:
         XSetForeground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
         XSetBackground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXand);
         break;
      case ropWhiteness:
         XSetForeground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXor);
         break;   
      default:   
         XSetForeground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
         XSetBackground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXand);
      }
      XPutImage( DISP, XX-> gdrawable, XX-> gc, r-> map[index]-> image, 0, 0, dsx, dsy, r-> dimension.x, r-> dimension.y);
      XCHECKPOINT; 
      switch ( XX-> paint_rop) {
      case ropAndPut:   
      case ropOrPut:
      case ropXorPut:
      case ropBlackness:   
      case ropWhiteness:
         break;
      case ropNotPut:   
         XSetForeground( DISP, XX-> gc, XX-> fore. pixel);
         XSetBackground( DISP, XX-> gc, WhitePixel( DISP, SCREEN));
         XSetFunction( DISP, XX-> gc, GXorInverted);
         goto DISPLAY;
      default:   
          XSetForeground( DISP, XX-> gc, XX-> fore. pixel);
          XSetBackground( DISP, XX-> gc, BlackPixel( DISP, SCREEN));
          XSetFunction( DISP, XX-> gc, GXor);
      DISPLAY:          
          XPutImage( DISP, XX-> gdrawable, XX-> gc, r-> map[index]-> image, 0, 0, dsx, dsy, r-> dimension.x, r-> dimension.y);
          XCHECKPOINT;
      }
      ax += cs-> width;
   }  
   apc_gp_set_rop( self, XX-> paint_rop);
   XSetForeground( DISP, XX-> gc, XX-> fore. pixel);
   XSetBackground( DISP, XX-> gc, XX-> back. pixel);

   if ( PDrawable( self)-> font. style & fsUnderlined) {
      int lw = apc_gp_get_line_width( self);
      int tw = apc_gp_get_text_width( self, text, len, true) - 1;
      int d  = XX-> font-> underlinePos;
      Point ovx = gp_get_text_overhangs( self, text, len);
      int x1, y1, x2, y2;
      if ( lw != XX-> font-> underlineThickness)
         apc_gp_set_line_width( self, XX-> font-> underlineThickness);

      ay = d + ( XX-> flags. paint_base_line ? 0 : XX-> font-> font. descent);
      rx. l = -ovx.x * r-> cos2. l - ay * r-> sin2. l + 0.5;
      ry. l = -ovx.x * r-> sin2. l + ay * r-> cos2. l + 0.5;
      x1 = x + rx. i. i;
      y1 = y + ry. i. i;
      tw += ovx.y;
      rx. l = tw * r-> cos2. l - ay * r-> sin2. l + 0.5;
      ry. l = tw * r-> sin2. l + ay * r-> cos2. l + 0.5;
      x2 = x + rx. i. i;
      y2 = y + ry. i. i;
      
      XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2)); 
      if ( lw != XX-> font-> underlineThickness) 
         apc_gp_set_line_width( self, lw);
   }   
   return true;
}   

Bool
apc_gp_text_out( Handle self, const char* text, int x, int y, int len)
{
   DEFXX;
   SHIFT( x, y);

   
   if ( !XF_IN_PAINT(XX)) {
      warn( "UAG_030: put begin_paint somewhere");
      return false;
   }

   if (0) {
      fprintf( stderr, "H: %d, D: %d, A: %d, BD: %d, BA: %d\n",
               XX-> font-> font. height,
               XX-> font-> fs-> descent,
               XX-> font-> fs-> ascent,
               XX-> font-> fs-> max_bounds. descent,
               XX-> font-> fs-> max_bounds. ascent
               );
   }

   /* paint background if opaque */
   if ( XX-> flags. paint_opaque) {
      int i;
      Point * p = apc_gp_get_text_box( self, text, len);
      FillPattern fp;
      memcpy( &fp, apc_gp_get_fill_pattern( self), sizeof( FillPattern));
      XSetForeground( DISP, XX-> gc, XX-> back. pixel);
      XSetFunction( DISP, XX-> gc, GXcopy);
      apc_gp_set_fill_pattern( self, fillPatterns[fpSolid]);
      for ( i = 0; i < 4; i++) {
         p[i].x += x;
         p[i].y += y;
      }   
      i = p[2].x; p[2].x = p[3].x; p[3].x = i;
      i = p[2].y; p[2].y = p[3].y; p[3].y = i;
      
      apc_gp_fill_poly( self, 4, p);
      apc_gp_set_rop( self, XX-> paint_rop);
      XSetForeground( DISP, XX-> gc, XX-> fore. pixel);
      apc_gp_set_fill_pattern( self, fp);
      free( p);
   }  

   if ( PDrawable( self)-> font. direction != 0) 
      return gp_text_out_rotated( self, text, x, y, len);

   if ( !XX-> flags. paint_base_line)
      y += XX-> font-> font. descent;

   XDrawString( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y), text, len);
   XCHECKPOINT;

   if ( PDrawable( self)-> font. style & fsUnderlined) {
      int lw = apc_gp_get_line_width( self);
      int tw = apc_gp_get_text_width( self, text, len, true);
      int d  = XX-> font-> underlinePos;
      Point ovx = gp_get_text_overhangs( self, text, len);
      if ( lw != XX-> font-> underlineThickness)
         apc_gp_set_line_width( self, XX-> font-> underlineThickness);
      XDrawLine( DISP, XX-> gdrawable, XX-> gc, 
         x - ovx.x, REVERT( y + d), x + tw - 1 + ovx.y, REVERT( y + d)); 
      if ( lw != XX-> font-> underlineThickness) 
         apc_gp_set_line_width( self, lw);
   }   
   
   return true;
}

/* gpi settings */
Color
apc_gp_get_back_color( Handle self)
{
   DEFXX;
   XColor c = ( XF_IN_PAINT(XX)) ? XX-> back : XX-> saved_back;
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
   XColor c = ( XF_IN_PAINT(XX)) ? XX-> fore : XX-> saved_fore;
   return ARGB( c. red >> 8, c. green >> 8, c. blue >> 8);
}

void
prima_gp_get_clip_rect( Handle self, XRectangle *cr)
{
   DEFXX;
   XRectangle r;

   cr-> x = 0;
   cr-> y = 0;
   cr-> width = XX-> size.x;
   cr-> height = XX-> size.y;
   if ( XF_IN_PAINT(XX) && ( XX-> region || XX-> stale_region)) {
      XClipBox( XX-> region ? XX-> region : XX-> stale_region,
                &r);
      prima_rect_intersect( cr, &r);
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
   r. top = XX-> size. y - cr. y - 1;
   r. bottom = r. top - cr. height + 1;
   r. right = cr. x + cr. width - 1;
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
   DEFXX;
   int cap;
   XGCValues gcv;

   if ( XF_IN_PAINT(XX)) {
      if ( XGetGCValues( DISP, XX-> gc, GCCapStyle, &gcv) == 0) {
         warn( "UAG_031: error querying GC values");
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
apc_gp_get_line_width( Handle self)
{
   DEFXX;
   int w;
   XGCValues gcv;

   if ( XF_IN_PAINT(XX)) {
      if ( XX-> flags. zero_line)
	 w = 0;
      else {
	 if ( XGetGCValues( DISP, XX-> gc, GCLineWidth, &gcv) == 0) {
            warn( "UAG_032: error querying GC values");
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
apc_gp_get_line_pattern( Handle self, unsigned char *dashes)
{
   DEFXX;
   int n;
   if ( XF_IN_PAINT(XX)) {
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
	 memcpy( dashes, XX-> dashes, n);
      }
   }
   return n;
}

Point
apc_gp_get_resolution( Handle self)
{
   if ( self)
      return (Point){X(self)-> resolution. x, X(self)-> resolution. y};
   else
      return (Point){guts.resolution.x, guts.resolution.y};
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

int
apc_gp_get_text_width( Handle self, const char *text, int len, Bool addOverhang)
{
   DEFXX;

   if ( !XX-> font) {
      apc_gp_set_font( self, &PDrawable( self)-> font);
   }
   
   if ( addOverhang) {
      Point ovx = gp_get_text_overhangs( self, text, len);
      return ovx. x + ovx. y + XTextWidth( XX-> font-> fs, text, len);
   } else
      return XTextWidth( XX-> font-> fs, text, len);
}

Point *
apc_gp_get_text_box( Handle self, const char* text, int len)
{
   DEFXX;
   Point * pt = ( Point *) malloc( sizeof( Point) * 5);
   int x;
   Point ovx;

   if ( !XX-> font) 
      apc_gp_set_font( self, &PDrawable( self)-> font);
   
   x = XTextWidth( XX-> font-> fs, text, len);
   ovx = gp_get_text_overhangs( self, text, len);

   pt[0].y = pt[2]. y = XX-> font-> font. ascent;
   pt[1].y = pt[3]. y = - XX-> font-> font. descent;
   pt[4].y = 0;
   pt[4].x = x;
   pt[3].x = pt[2]. x = x - ovx. y;
   pt[0].x = pt[1]. x = - ovx. x;

   if ( !XX-> flags. paint_base_line) {
      int i;
      for ( i = 0; i < 5; i++) pt[i]. y += XX-> font-> font. descent;
   }   
   
   if ( PDrawable( self)-> font. direction != 0) {
      int i;
      double s = sin( PDrawable( self)-> font. direction / 572.9577951);
      double c = cos( PDrawable( self)-> font. direction / 572.9577951);
      for ( i = 0; i < 5; i++) {
         int x = pt[i]. x;
         int y = pt[i]. y;
         pt[i]. x = x * c - y * s + 0.5;
         pt[i]. y = x * s + y * c + 0.5;
      }
   }
 
   return pt;
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
apc_gp_set_back_color( Handle self, Color color)
{
   DEFXX;
   XColor *c = prima_allocate_color( self, color);

   if ( XF_IN_PAINT(XX)) {
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

   if ( !XF_IN_PAINT(XX))
      return false;

   SORT( clipRect. left, clipRect. right);
   SORT( clipRect. bottom, clipRect. top);
   r. x = clipRect. left;
   r. y = REVERT( clipRect. top);
   r. width = clipRect. right - clipRect. left+1;
   r. height = clipRect. top - clipRect. bottom+1;
   XX-> clip_rect = r;
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

   if ( XF_IN_PAINT(XX)) {
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
   Bool dflt;
#define patcmp(p1,p2) (memcmp((p1),(p2),sizeof(FillPattern)))
#define patcpy(pt,ps) (memcpy((pt),(ps),sizeof(FillPattern)))

   if ( patcmp( pattern, XX-> fill_pattern) == 0)
      return true;

   patcpy( XX-> fill_pattern, pattern);
   dflt = (patcmp( pattern, fillPatterns[fpSolid]) == 0);
   
   if ( XF_IN_PAINT(XX)) {
      if ( XX-> fp_pixmap)
         XFreePixmap( DISP, XX-> fp_pixmap);
      XX-> fp_pixmap = None;
      if ( !dflt)
         if (( XX-> fp_pixmap = XCreateBitmapFromData( 
               DISP, XX-> gdrawable, pattern, 8, 8)) == None) {
             warn( "UAG_033: error creating stipple");
             dflt = true;
         }      
      XSetFillStyle( DISP, XX-> gc, dflt ? FillSolid : FillOpaqueStippled);
      if ( !dflt) XSetStipple( DISP, XX-> gc, XX-> fp_pixmap);
      XCHECKPOINT;
   }
   return true;
#undef patcmp
#undef patcpy
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
apc_gp_set_line_width( Handle self, int line_width)
{
   DEFXX;
   XGCValues gcv;
   int zero_line = line_width == 0;

   if ( zero_line)
      line_width = 1;

   if ( XF_IN_PAINT(XX)) {
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
apc_gp_set_line_pattern( Handle self, unsigned char *pattern, int len)
{
   DEFXX;
   XGCValues gcv;

   if ( XF_IN_PAINT(XX)) {
      if ( len == 0 || (len == 1 && pattern[0] == 1)) {
	 gcv. line_style = LineSolid;
	 XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      } else {
	 gcv. line_style = ( XX-> paint_rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;
	 XSetDashes( DISP, XX-> gc, 0, pattern, len);
	 XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      }
      XX-> line_style = gcv. line_style;
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
	 XX-> gcv. line_style = ( XX-> rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;
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

   if ( XF_IN_PAINT(XX)) {
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
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      if ( XX-> paint_rop2 == rop) return true;
      XX-> paint_rop2 = ( rop == ropCopyPut) ? ropCopyPut : ropNoOper;
      if ( XX-> line_style != LineSolid) {
         XGCValues gcv;
         gcv. line_style = ( rop == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
         XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      }   
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

