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
 */

/***********************************************************/
/*                                                         */
/*  System dependent color management                      */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Drawable.h"
#include "Window.h"


static Color standard_button_colors[] = {
   0x000000,	/* Prima.Button.foreground */
   0xcccccc,        /* Prima.Button.background */
   0x000000,	/* Prima.Button.hilitefore */
   0xcccccc,        /* Prima.Button.hiliteback */
   0x606060,        /* Prima.Button.disabledfore */
   0xcccccc,        /* Prima.Button.disabledback */
   0xffffff,        /* Prima.Button.light3d */
   0x808080,	/* Prima.Button.dark3d */
};

static Color standard_checkbox_colors[] = {
   0x000000,	/* Prima.Checkbox.foreground */
   0xcccccc,        /* Prima.Checkbox.background */
   0x000000,	/* Prima.Checkbox.hilitefore */
   0xcccccc,        /* Prima.Checkbox.hiliteback */
   0x606060,        /* Prima.Checkbox.disabledfore */
   0xcccccc,        /* Prima.Checkbox.disabledback */
   0xffffff,        /* Prima.Checkbox.light3d */
   0x808080,	/* Prima.Checkbox.dark3d */
};

static Color standard_combo_colors[] = {
   0x000000,	/* Prima.Combo.foreground */
   0xcccccc,        /* Prima.Combo.background */
   0x000000,	/* Prima.Combo.hilitefore */
   0xcccccc,        /* Prima.Combo.hiliteback */
   0x606060,        /* Prima.Combo.disabledfore */
   0xcccccc,        /* Prima.Combo.disabledback */
   0xffffff,        /* Prima.Combo.light3d */
   0x808080,	/* Prima.Combo.dark3d */
};

static Color standard_dialog_colors[] = {
   0x000000,	/* Prima.Dialog.foreground */
   0xcccccc,        /* Prima.Dialog.background */
   0x000000,	/* Prima.Dialog.hilitefore */
   0xcccccc,        /* Prima.Dialog.hiliteback */
   0x606060,        /* Prima.Dialog.disabledfore */
   0xcccccc,        /* Prima.Dialog.disabledback */
   0xffffff,        /* Prima.Dialog.light3d */
   0x808080,	/* Prima.Dialog.dark3d */
};

static Color standard_edit_colors[] = {
   0x000000,	/* Prima.Edit.foreground */
   0xcccccc,        /* Prima.Edit.background */
   0xcccccc,        /* Prima.Edit.hilitefore */
   0x000000,	/* Prima.Edit.hilitebac */
   0x606060,        /* Prima.Edit.disabledfore */
   0xcccccc,        /* Prima.Edit.disabledback */
   0xffffff,        /* Prima.Edit.light3d */
   0x808080,	/* Prima.Edit.dark3d */
};

static Color standard_inputline_colors[] = {
   0x000000,	/* Prima.Inputline.foreground */
   0xcccccc,        /* Prima.Inputline.background */
   0xcccccc,        /* Prima.Inputline.hilitefore */
   0x000000,	/* Prima.Inputline.hiliteback */
   0x606060,        /* Prima.Inputline.disabledfore */
   0xcccccc,        /* Prima.Inputline.disabledback */
   0xffffff,        /* Prima.Inputline.light3d */
   0x808080,	/* Prima.Inputline.dark3d */
};

static Color standard_label_colors[] = {
   0x000000,	/* Prima.Label.foreground */
   0xcccccc,        /* Prima.Label.background */
   0x000000,	/* Prima.Label.hilitefore */
   0xcccccc,        /* Prima.Label.hiliteback */
   0x606060,        /* Prima.Label.disabledfore */
   0xcccccc,        /* Prima.Label.disabledback */
   0xffffff,        /* Prima.Label.light3d */
   0x808080,	/* Prima.Label.dark3d */
};

static Color standard_listbox_colors[] = {
   0x000000,	/* Prima.Listbox.foreground */
   0xcccccc,        /* Prima.Listbox.background */
   0xcccccc,        /* Prima.Listbox.hilitefore */
   0x000000,	/* Prima.Listbox.hiliteback */
   0x606060,        /* Prima.Listbox.disabledfore */
   0xcccccc,        /* Prima.Listbox.disabledback */
   0xffffff,        /* Prima.Listbox.light3d */
   0x808080,	/* Prima.Listbox.dark3d */
};

static Color standard_menu_colors[] = {
   0x000000,	/* Prima.Menu.foreground */
   0xcccccc,        /* Prima.Menu.background */
   0xcccccc,        /* Prima.Menu.hilitefore */
   0x000000,	/* Prima.Menu.hiliteback */
   0x606060,        /* Prima.Menu.disabledfore */
   0xcccccc,        /* Prima.Menu.disabledback */
   0xffffff,        /* Prima.Menu.light3d */
   0x808080,	/* Prima.Menu.dark3d */
};

static Color standard_popup_colors[] = {
   0x000000,	/* Prima.Popup.foreground */
   0xcccccc,        /* Prima.Popup.background */
   0xcccccc,        /* Prima.Popup.hilitefore */
   0x000000,	    /* Prima.Popup.hiliteback */
   0x606060,        /* Prima.Popup.disabledfore */
   0xcccccc,        /* Prima.Popup.disabledback */
   0xffffff,        /* Prima.Popup.light3d */
   0x808080,	/* Prima.Popup.dark3d */
};

static Color standard_radio_colors[] = {
   0x000000,	/* Prima.Radio.foreground */
   0xcccccc,        /* Prima.Radio.background */
   0x000000,	/* Prima.Radio.hilitefore */
   0xcccccc,        /* Prima.Radio.hiliteback */
   0x606060,        /* Prima.Radio.disabledfore */
   0xcccccc,        /* Prima.Radio.disabledback */
   0xffffff,        /* Prima.Radio.light3d */
   0x808080,	/* Prima.Radio.dark3d */
};

static Color standard_scrollbar_colors[] = {
   0x000000,	/* Prima.Scrollbar.foreground */
   0xcccccc,        /* Prima.Scrollbar.background */
   0x000000,	/* Prima.Scrollbar.hilitefore */
   0xcccccc,        /* Prima.Scrollbar.hiliteback */
   0x606060,        /* Prima.Scrollbar.disabledfore */
   0xcccccc,        /* Prima.Scrollbar.disabledback */
   0xffffff,        /* Prima.Scrollbar.light3d */
   0x808080,	/* Prima.Scrollbar.dark3d */
};

static Color standard_slider_colors[] = {
   0x000000,	/* Prima.Slider.foreground */
   0xcccccc,        /* Prima.Slider.background */
   0x000000,	/* Prima.Slider.hilitefore */
   0xcccccc,        /* Prima.Slider.hiliteback */
   0x606060,        /* Prima.Slider.disabledfore */
   0xcccccc,        /* Prima.Slider.disabledback */
   0xffffff,        /* Prima.Slider.light3d */
   0x808080,	/* Prima.Slider.dark3d */
};

static Color standard_widget_colors[] = {
   0x000000,	/* Prima.Widget.foreground */
   0xcccccc,        /* Prima.Widget.background */
   0x000000,	/* Prima.Widget.hilitefore */
   0xcccccc,        /* Prima.Widget.hiliteback */
   0x606060,        /* Prima.Widget.disabledfore */
   0xcccccc,        /* Prima.Widget.disabledback */
   0xffffff,        /* Prima.Widget.light3d */
   0x808080,	/* Prima.Widget.dark3d */
};

static Color standard_window_colors[] = {
   0x000000,	/* Prima.Window.foreground */
   0xcccccc,        /* Prima.Window.background */
   0x000000,	/* Prima.Window.hilitefore */
   0xcccccc,        /* Prima.Window.hiliteback */
   0x606060,        /* Prima.Window.disabledfore */
   0xcccccc,        /* Prima.Window.disabledback */
   0xffffff,        /* Prima.Window.light3d */
   0x808080,	/* Prima.Window.dark3d */
};

static Color standard_application_colors[] = {
   0x000000,	/* Prima.Application.foreground */
   0xcccccc,        /* Prima.Application.background */
   0x000000,	/* Prima.Application.hilitefore */
   0xcccccc,        /* Prima.Application.hiliteback */
   0x606060,        /* Prima.Application.disabledfore */
   0xcccccc,        /* Prima.Application.disabledback */
   0xffffff,        /* Prima.Application.light3d */
   0x808080,	/* Prima.Application.dark3d */
};

static Color* standard_colors[] = {
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

Color 
prima_map_color( Color clr, int * hint)
{
   long cls;
   if ( hint) *hint = COLORHINT_NONE;
   if ( clr >= 0) return clr;
   
   cls = (clr & wcMask) >> 16;
   if ( cls <= 0 || cls > MAX_COLOR_CLASS) cls = wcWidget;
   if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
   if ( clr == clSet)   {
      if ( hint) *hint = COLORHINT_WHITE;
      return 0xffffff; 
   } else if ( clr == clClear) {
      if ( hint) *hint = COLORHINT_BLACK;
      return 0; 
   } else return standard_colors[cls][clr-1];
}   

Color
apc_widget_map_color( Handle self, Color color)
{
   if ((color < 0) && (( color & wcMask) == 0)) color |= PWidget(self)->widgetClass;
   return prima_map_color( color, nil);
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
      if ( !card[*ls]) printf("jopa!\n");
       else card[*ls]--;
   }
   printf("done\n");
}

#define XAllocColor(a,b,c) my_XAllocColor(a,b,c,__LINE__)
#define XFreeColors(a,b,c,d,e) my_XFreeColors(a,b,c,d,e,__LINE__)
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
  //  printf("%s asked for %06x\n", self?PWidget(self)->name:"null", color);
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
         Bool dyna = guts. dynamicColors && self && X(self)-> type. widget && ( self != application);
         brush-> primary = prima_color_find( self, color, -1, &ab2, RANK_FREE);
         
         if ( dyna && ab2 > 12) {
            XColor xc;
            xc. red   = COLOR_R16(color);
            xc. green = COLOR_G16(color);
            xc. blue  = COLOR_B16(color);
            xc. flags = DoGreen | DoBlue | DoRed;
            prima_color_sync();
            if ( XAllocColor( DISP, guts. defaultColormap, &xc)) {
               if ( prima_color_new( &xc)) {
                  // printf("%s alloc %d ( wanted %06x). got %02x %02x %02x\n", PWidget(self)-> name, xc.pixel, color, xc.red>>8,xc.green>>8,xc.blue>>8);
                  prima_color_add_ref( self, xc. pixel, RANK_NORMAL);
                  return brush-> primary = xc. pixel;
               }
            }
         }

         if ( guts. useDithering && (brush != &b) && (ab2 > 12)) {
            if ( guts. grayScale) {
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
 R |                  R'G' and R"G" are 2 colors blended with proprotion to make
   | '''* A           color A. R'G' is a closest cubic color. If A(G)/A(R) < y,
   |    |             R"G" is G-point, if A(G)/A(R) > 1 + y, it's R-point, otherwise
   *---------- G      it's RG-point. (y=sqrt(2)-1=0.41; y=0.41x and y=1.41x are
R'G' , B=0            maximal error lines). balance is computed as diff between
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
//                  printf("want %06x, closest is %06x\n", color, guts.palette[brush-> primary].composite);
                  ab2 = (a[0]-b[0])*(a[0]-b[0]) +
                        (a[1]-b[1])*(a[1]-b[1]) +
                        (a[2]-b[2])*(a[2]-b[2]);
                  for ( i = 0; i < guts.palSize; i++) {
                     if ( guts.palette[i].rank == RANK_FREE) continue;
                     d[0] = guts. palette[i].r;
                     d[1] = guts. palette[i].g;
                     d[2] = guts. palette[i].b;
                     // printf("tasting %06x\n", guts.palette[i].composite);
                     bd2 = (d[0]-b[0])*(d[0]-b[0]) +
                           (d[1]-b[1])*(d[1]-b[1]) +
                           (d[2]-b[2])*(d[2]-b[2]);
                     bd  = sqrt( bd2);
                     if ( bd == 0) continue;
                     ad2 = (d[0]-a[0])*(d[0]-a[0]) +
                           (d[1]-a[1])*(d[1]-a[1]) +
                           (d[2]-a[2])*(d[2]-a[2]);
                     cd  = ( ad2 - ab2 + bd2) / (2 * bd);
                     // printf("bd:%g,bd2:%d, ad2:%d, cd:%g\n", bd, bd2, ad2, cd);
                     if ( cd < bd) {
                        ac2 = ad2 - cd * cd;
                        // printf("ac2:%d\n", ac2);
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
                  if ( maxDiff > (64/(guts.colorCubeRib-1))) {
                     cubic = true;
                     goto DITHER;
                  } 
                  brush-> secondary = bestMatch;
                  brush-> balance   = 63 - BMcd * 64 / BMbd;
                  // printf("MIX with %d of %06x\n", brush-> balance, guts.palette[bestMatch].composite);
               }
            }
         }
         
         if ( dyna) {
            Byte * p = X(self)-> palette; 
            if (( p[LPAL_ADDR(brush->primary)]&LPAL_MASK(brush->primary)) == 0) 
               prima_color_add_ref( self, brush-> primary, RANK_NORMAL);
            if (( brush-> balance > 0) &&
               (( p[LPAL_ADDR(brush->secondary)]&LPAL_MASK(brush->secondary)) == 0)) 
               prima_color_add_ref( self, brush-> secondary, RANK_NORMAL);
         }
      } else  
         brush-> primary = 
            (((a[0] << guts. red_range  ) >> 8) << guts.   red_shift) |
            (((a[1] << guts. green_range) >> 8) << guts. green_shift) |
            (((a[2] << guts. blue_range ) >> 8) << guts.  blue_shift);
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
      if (( xc[idx]. blue / 256 - B / 256) * ( xc[idx]. blue / 256 - B / 256) +
          ( xc[idx]. green / 256 - G / 256) * ( xc[idx]. green / 256 - G / 256) +
          ( xc[idx]. red / 256 - R / 256) * ( xc[idx]. red / 256 - R / 256) > 
          maxDiff) {
         err = true;
         break;
      }
   }

   if ( err) {
      long cnt = 0, free[32];
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
   if ( !( guts. palette = malloc( sizeof( MainColorEntry) * guts. palSize))) {
   NO_MEM:
      warn("not enough memory");
      return false;
   }
   if ( !( guts. systemColorMap = malloc( sizeof( int) * count))) {
      free( guts. palette);
      guts. palette = nil;
      goto NO_MEM;
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

Bool
prima_init_color_subsystem(void)
{
   int i, count, preferred, found = -1;
   XVisualInfo template, *list;

   template. screen = SCREEN;
   template. depth  = guts. depth;
   list = XGetVisualInfo( DISP, VisualScreenMask|VisualDepthMask,
                          &template, &count);
   if ( count == 0) {
      warn("panic: no visuals found\n");
      return false;
   }

   if ( guts. depth <= 4) preferred = StaticColor; else
   if ( guts. depth <= 8) preferred = PseudoColor; else
                          preferred = TrueColor;

   for ( i = 0; i < count; i++) {
      int cls = list[i].
#if defined(__cplusplus) || defined(c_plusplus)
c_class;
#else
class;
#endif
      if ( cls == preferred) {
         found = i;
         break;
      }
   }
   if ( found < 0) found = 0;
   guts. visual = list[found]; 
   guts. visualClass = list[i].
#if defined(__cplusplus) || defined(c_plusplus)
c_class;
#else
class;
#endif
   XFree( list);
   if ( guts. depth > 11 && guts. visualClass != TrueColor) {/* XXX */
      warn("panic: %d bit depth is not true color\n", guts. depth);
      return false;
   }
   if ( guts. depth <= 8 && 
        ((guts. visualClass == TrueColor)||
        (guts. visualClass == DirectColor))) {
      warn("panic: display is not palette-based (%d bit depth)\n", guts. depth);
      return false;
   }
   
   guts. defaultColormap  = DefaultColormap( DISP, SCREEN); 
   guts. useDithering     = true;
   guts. dynamicColors    = false;
   guts. grayScale        = false;
   guts. palSize          = 1 << guts. depth;
   guts. palette          = nil;
   guts. systemColorMap   = nil;
   guts. systemColorMapSize = 0;
   guts. colorCubeRib     = 0;

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
         XColor xc[8];
         guts. dynamicColors = true;
         fill_cubic( xc, 2);
         if ( !alloc_main_color_range( xc, 8, 27))
            goto BLACK_WHITE;
         if ( !create_std_palettes( xc, 8)) return false;
         guts. colorCubeRib = 2;
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
                  if ( !create_std_palettes( xc, d * d * d)) return false;
                  break;
               }
            }
            d--; 
            cd += 2;
         }
         if ( d < 2) goto BLACK_WHITE_ALLOCATED;
         guts. colorCubeRib = d;
      }
      break;

   case StaticGray:
   case GrayScale:
      {
         XColor xc[256];
         int maxSteps = ( guts. visualClass == GrayScale) ? 5 : 8;
         int wantSteps = ( maxSteps > guts. depth) ? guts. depth : maxSteps;
         while ( wantSteps > 1) {
            int i, shades = 1 << wantSteps, c = 0, ndiv = 65536 / (shades-1);
            for ( i = 0; i < shades; i++) {
               xc[i].red = xc[i]. green = xc[i]. blue = c;
               if (( c += ndiv) > 65535) c = 65535;
            }
            if ( alloc_main_color_range( xc, shades, 256 / shades)) {
               if ( !create_std_palettes( xc, shades)) return false;
               break;
            }
            wantSteps--;
         }
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
         xc[0]. pixel = BlackPixel( DISP, SCREEN);
         xc[1]. pixel = WhitePixel( DISP, SCREEN);
         XQueryColors( DISP, guts. defaultColormap, xc, 2);
         XCHECKPOINT;
         if ( !alloc_main_color_range( xc, 2, 65536) || 
              !create_std_palettes( xc, 2)) {
            warn("panic: unable to initialize color system");
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
         warn("not enough memory");
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
      warn("not enough memory\n");
      return false;
   }
   if ( guts. palSize) {
      int sz = ( guts. palSize < 256) ? 256 : guts. palSize;
      if (!( guts. mappingPlace = malloc( sizeof( int) * sz))) {
         warn("not enough memory\n");
         return false;
      }
   } else {
      int i, j, from[3] = {0,0,0}, to[3] = {0,0,0}, stage[3] = {0,0,0}, lim[3]; 
      unsigned long mask[3] = {guts. visual. red_mask, guts. visual. green_mask, guts. visual. blue_mask};
      // find color bounds and test if they are contiguous
      for ( j = 0; j < 3; j++) {
         for ( i = 0; i < 32; i++) {
            switch ( stage[j]) {
            case 0:
               if (( mask[j] & ( 1 << i)) != 0) {
                  from[j] = i;
                  stage[j]++;
               }
               break;
            case 1:
               if (( mask[j] & ( 1 << i)) == 0) {
                  to[j] = i;
                  stage[j]++;
               }
               break;
            case 2:
               if (( mask[j] & ( 1 << i)) != 0) {
                  warn("panic: unsupported pixel representation (0x%08lx,0x%08lx,0x%08lx)\n", 
                     mask[0], mask[1], mask[2]);
                  return false;
               }
            }
         }
         if ( to[j] == 0) to[j] = 32;
         lim[j] = 1 << (to[j] - from[j]);
      }

      guts. red_shift   = from[0];
      guts. green_shift = from[1];
      guts. blue_shift  = from[2];
      guts. red_range   = to[0] - from[0];
      guts. green_range = to[1] - from[1];
      guts. blue_range  = to[2] - from[2];
   }
   guts. localPalSize = guts. palSize / 4 + ((guts. palSize % 4) ? 1 : 0);
   hatches = hash_create();
   return true;
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
      hash_first_that( hatches, kill_hatches, nil, nil, nil);
      fc. count = 0;
      
      for ( i = 0; i < guts. palSize; i++) {
         list_destroy( &guts. palette[i]. users);
         if ( 
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
   free( guts. mappingPlace);
   free( guts. ditherPatterns);
   free( guts. palette);
   free( guts. systemColorMap);
   guts. palette = nil;
   guts. systemColorMap = nil;
   guts. ditherPatterns = nil;
   guts. mappingPlace = nil;
}

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
   global = self ? (X(self)-> type. widget && ( self != application)) : true;

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
      Byte * p = X(self)-> palette;
      for ( j = 0; j < guts. systemColorMapSize + guts. palSize; j++) {
         if ( j < guts. systemColorMapSize) 
            i = guts. systemColorMap[j];
         else {
            i = j - guts. systemColorMapSize;
            if (( p[LPAL_ADDR(i)] & LPAL_MASK(i)) == 0) continue;
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

Bool
prima_color_add_ref( Handle self, int index, int rank)
{
   Byte * p;
   int r, nr = (rank == RANK_PRIORITY) ? 2 : 1;
   if ( index < 0 || index >= guts. palSize) return false;
   if ( guts. palette[index]. rank == RANK_IMMUTABLE) return false;
   if ( !self || ( self == application)) return false;
   p = X(self)-> palette;
   r = LPAL_GET(index,p[LPAL_ADDR(index)]);
   if ( r != 0 && r <= nr) return false;
   if ( r == 0) list_add( &guts. palette[index]. users, self);
   if ( rank > guts. palette[index]. rank)
      guts. palette[index]. rank = rank;
   p[LPAL_ADDR(index)] &=~ LPAL_MASK(index);
   p[LPAL_ADDR(index)] |=  LPAL_SET(index, nr);
   // printf("%s %s %d %d\n", PWidget(self)-> name, r ? "raised to " : "added as", nr, index);
   return true;
}

int
prima_color_sync( void)
{
   int i, count = 0, freed = 0;
   long free[32];
   MainColorEntry * p = guts. palette;
   for ( i = 0; i < guts. palSize; i++, p++) {
      if ( p-> touched) {
         int j, max = RANK_FREE;
         for ( j = 0; j < p-> users. count; j++) {
            int rank;
            if ( X(p-> users. items[j])-> type. widget) {
               rank = LPAL_GET(i,X(p-> users. items[j])-> palette[ LPAL_ADDR(i)]);
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

// updates contents of DefaultColormap. 
// NB - never to be called with 'fast' set to true.

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
   if ( self == application) return true;
   
   if ( XX-> type.widget) rank = RANK_PRIORITY; else
   if ( XX-> type.image || XX-> type. dbm) rank = RANK_LOCKED; else
      return false;

   if ( !fast) prima_palette_free( self, true); // remove old entries
  
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
   
   // fetch actual colors - they are useful when no free colorcells
   // available, but colormap has some good colors, which we don't
   // possess
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
      if ( count > 0) 
         for ( j = 0; j < count; j++) prima_color_new( &xc[j]);
   }
   
   // printf("%s find match for %d colors\n", PWidget(self)-> name, psz);
   // find out if any allocated entries are present already
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
                  if ( XAllocColor( DISP, guts. defaultColormap, &xc)) {
                     if ( prima_color_new( &xc))
                        // to protect from sync - give actual status on SUCCESS
                        guts.palette[xc.pixel].rank = RANK_IMMUTABLE + 1; 
                     pixel = xc.pixel;
                  } else
                     continue;
               }
               req[i] |= 0x80000000;
               prima_color_add_ref( self, pixel, rank);
               granted++;
               break;
            }
         }
      }

    // printf("granted %d\n", granted);
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
   // allocate some colors
   prima_color_sync();
   XCHECKPOINT;
   for ( i = 0; i < psz; i++) 
      if (( req[i] & 0x80000000) == 0) {
         XColor xc;
         xc. red   = COLOR_R16(req[i]);
         xc. green = COLOR_G16(req[i]);
         xc. blue  = COLOR_B16(req[i]);
         if ( XAllocColor( DISP, guts. defaultColormap, &xc)) {
            if ( prima_color_new( &xc)) {
               prima_color_add_ref( self, xc. pixel, rank);
               granted++;
            } 
            req[i] |= 0x80000000;
         } else 
            break;
      }
     // printf("ok - now %d are granted\n", granted);
  
   if ( granted == psz) {
      free( req);
      goto SUCCESS;
   }
       
   if ( stage == RANK_NORMAL) {
       // try to remove RANK_NORMAL colors
       p = guts. palette;
       for ( i = 0; i < guts. palSize; i++, p++) {
          if ( p-> rank == RANK_NORMAL) {
             int j;
             for ( j = 0; j < p-> users. count; j++) {
                Handle wij = p-> users. items[j];
                if ( list_index_of( &widgets, wij) < 0)
                   list_add( &widgets, wij);
                if ( LPAL_GET(i,X(wij)-> palette[LPAL_ADDR(i)]) == 1)
                   X(wij)-> palette[LPAL_ADDR(i)] &=~ LPAL_MASK(i);
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
  
   // try to remove RANK_PRIORITY entries
   p = guts. palette;
   for ( i = 0; i < guts. palSize; i++, p++) {
      if ( p-> rank == RANK_PRIORITY) {
         int j;
         for ( j = 0; j < p-> users. count; j++) {
            Handle wij = p-> users. items[j];
            if ( list_index_of( &widgets, wij) < 0)
               list_add( &widgets, wij);
            X(wij)-> palette[LPAL_ADDR(i)] &=~ LPAL_MASK(i);
         }
         list_delete_all( &p-> users, false);
         p-> touched = true;
      }
   }
   
   psz = prima_color_sync();
   if ( psz == 0) goto SUCCESS; // free no RANK_PRIORITY colors :( 
   XCHECKPOINT;

   // collect big palette
   j = 0;
   for ( i = 0; i < guts. palSize; i++)
      if ( guts. palette[i]. rank != RANK_FREE) 
         j++;
   stage = j; // immutable and locked colors
   for ( i = 0; i < widgets. count; i++) {
      j += PWidget( widgets. items[i])-> palSize;
      if ( XT_IS_WINDOW(X(widgets. items[i])) && 
           PWindow(widgets. items[i])-> menu)
         j += ciMaxId + 1;
   }
   
    // printf("BIG:%d vs %d\n", j, psz);
   if ( !( rqx = malloc( sizeof( RGBColor) * j))) goto SUCCESS; // :O
   
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
   
   // squeeze palette
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
ENOUGH:      
   }
   
   // printf("ok. XAllocColor again\n");
   granted = 0;
   for ( i = stage; i < stage + psz; i++) {
      XColor xc;
      xc. red   = rqx[i]. r << 8;
      xc. green = rqx[i]. g << 8;
      xc. blue  = rqx[i]. b << 8;
      if ( XAllocColor( DISP, guts. defaultColormap, &xc)) {
         if ( prima_color_new( &xc)) {
            // give new color NORMAL status - to be cleaned automatically
            // upon 1st sync() invocation
            guts. palette[xc. pixel]. touched = 1;
            guts. palette[xc. pixel]. rank = RANK_NORMAL;
            granted++;
          } 
      } else
         break;
   }
   free( rqx);
   // printf("ok - %d out of %d \n", granted, psz);
   XCHECKPOINT;
   
   // now give away colors that can be mapped to reduced palette
   prima_palette_replace( self, true);
   for ( i = 0; i < widgets. count; i++) 
      prima_palette_replace( widgets. items[i], true);
   XCHECKPOINT;
   
SUCCESS:

   // restore status of pre-fetched colors
   for ( i = 0; i < guts. palSize; i++)
      if ( guts.palette[i].rank == RANK_IMMUTABLE + 1)
         guts.palette[i].rank = RANK_PRIORITY;
           
   prima_color_sync(); 
   for ( i = 0; i < widgets. count; i++)
      if ( PWidget( widgets. items[i])-> stage < csDead)
         apc_widget_invalidate_rect( widgets. items[i], nil);
   
   //  printf("EXIT\n");
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
   Byte * p;
   if ( !guts. dynamicColors) return;
   p = X(self)-> palette;   
   for ( i = 0; i < guts. palSize; i++) {
      int rank = LPAL_GET(i,p[LPAL_ADDR(i)]);
      if ( rank > 0 && max >= rank) {
         p[LPAL_ADDR(i)] &=~ LPAL_MASK(i);
         list_delete( &guts. palette[i]. users, self);
         // printf("%s free %d, %d\n", PWidget(self)-> name, i, p[LPAL_ADDR(i)] & LPAL_MASK(i));
         guts. palette[i]. touched = true;
      }
   }
   /*
   for ( i = 0; i < guts. palSize; i++) { 
      int d = LPAL_GET(i,p[LPAL_ADDR(i)]);
      if ( d) printf("LEFT-1:%d at[%d,%d](%02x)\n", d, i, LPAL_ADDR(i), LPAL_MASK(i));
      if ( p[i>>2]) printf("LEFT-2:%02x at[%d,%d](%02x)\n", p[i>>2], i, LPAL_ADDR(i), LPAL_MASK(i));
   }
   printf(":%s for %s\n", priority ? "PRIO" : "", PWidget(self)-> name);
   */
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
   if ( memcmp( fp, fillPatterns[fpSolid], sizeof( FillPattern)) == 0)
      return nilHandle;
   if (( p = ( Pixmap) hash_fetch( hatches, fp, sizeof( FillPattern))))
      return p;
   for ( i = 0; i < sizeof( FillPattern); i++)
      fprev[i] = (*fp)[ sizeof(FillPattern) - i - 1];
   if (( p = XCreateBitmapFromData( DISP, guts. root, (char*)fprev, 8, 8)) == None) {
      hash_first_that( hatches, kill_hatches, nil, nil, nil);
      hash_destroy( hatches, false);
      hatches = hash_create();
      if (( p = XCreateBitmapFromData( DISP, guts. root, (char*)fprev, 8, 8)) == None) 
         return nilHandle;
   }
   hash_store( hatches, fp, sizeof( FillPattern), ( void*) p);
   return p;
}
