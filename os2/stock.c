/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#include <limits.h>
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#define INCL_VIO
#define INCL_KBD
#include <float.h>
#include <io.h>
#include <fcntl.h>
#include "os2/os2guts.h"
#include "Image.h"
#include "Menu.h"
#include "Window.h"
#include "Printer.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)

// hwnd
void
hwnd_enter_paint( Handle self)
{
   LONG lset[ 2];
   sys fontId = 0;
   sys fontHash = create_fontid_hash();
   sys fillBitmap = nilHandle;
   apc_gp_set_font( self, &var font);

   if ( is_apt( aptWinPS) && self != application) {
      apc_gp_set_color( self, apc_widget_get_color( self, ciFore));
      apc_gp_set_back_color( self, apc_widget_get_color( self, ciBack));
   } else {
      apc_gp_set_color( self, sys lbs[0]);
      apc_gp_set_back_color( self, sys lbs[1]);
   }

   if ( !DevQueryCaps( GpiQueryDevice( sys ps), CAPS_COLOR_PLANES, 2, lset)) apiErr;
   sys bpp = lset[0] * lset[1];
// printf("checkpoint: bpp is %d", sys bpp);
   if ( sys psd == nil) sys psd = malloc( sizeof( PaintSaveData));
   if ( sys psd == nil) return;
   apc_gp_set_text_opaque( self, is_apt( aptTextOpaque));
   sys psd-> fillWinding = sys fillWinding;
   apc_gp_set_fill_winding( self, sys fillWinding);
   apc_gp_set_line_width( self, sys lineWidth);
   apc_gp_set_line_end( self, sys lineEnd);
   apc_gp_set_line_join( self, sys lineJoin);
   apc_gp_set_rop( self, sys rop);
   apc_gp_set_rop2( self, sys rop2);
   apc_gp_set_transform( self, sys transform. x, sys transform. y);
   apc_gp_set_fill_pattern( self, sys fillPattern2);
   sys psd-> font        = var font;
   sys psd-> lineWidth   = sys lineWidth;
   sys psd-> lineEnd     = sys lineEnd;
   sys psd-> lineJoin    = sys lineJoin;
   sys psd-> linePattern = sys linePattern;
   sys psd-> linePatternLen = sys linePatternLen;
   sys psd-> rop         = sys rop;
   sys psd-> rop2        = sys rop2;
   sys psd-> transform   = sys transform;
   sys psd-> textOpaque  = is_apt( aptTextOpaque);
   sys psd-> textOutB    = is_apt( aptTextOutBaseline);
   apc_gp_set_line_pattern( self,
      ( sys linePatternLen > 3) ? sys linePattern : (unsigned char*)(&sys linePattern), sys linePatternLen);
}

void
hwnd_leave_paint( Handle self)
{
   if ( sys fillBitmap) {
      if ( !GpiSetPatternSet( sys ps, LCID_DEFAULT)) apiErr;
      if ( !GpiDeleteSetId( sys ps, 3)) apiErr;
      if ( !GpiDeleteBitmap( sys fillBitmap)) apiErr;
      sys fillBitmap = nilHandle;
   }

   sys fontId = 0;
   destroy_fontid_hash( sys fontHash);
   sys fontHash = nil;
   sys bpp = 0;

   if ( sys linePatternLen > 3) free( sys linePattern);
   if ( sys psd == nil) return;
   var font           = sys psd-> font;
   sys fillWinding    = sys psd-> fillWinding;
   sys lineWidth      = sys psd-> lineWidth;
   sys lineEnd        = sys psd-> lineEnd;
   sys lineJoin       = sys psd-> lineJoin;
   sys linePattern    = sys psd-> linePattern;
   sys linePatternLen = sys psd-> linePatternLen;
   sys rop            = sys psd-> rop;
   sys rop2           = sys psd-> rop2;
   sys transform      = sys psd-> transform;
   apt_assign( aptTextOpaque, sys psd-> textOpaque);
   apt_assign( aptTextOutBaseline, sys psd-> textOutB);
   free( sys psd);
   sys psd = nil;
}

Handle
hwnd_top_level( Handle self)
{
   while ( self) {
      if ( sys className == WC_FRAME) return self;
      self = var owner;
   }
   return nilHandle;
}

Handle
hwnd_frame_top_level( Handle self)
{
   while ( self && ( self != application)) {
      if (( sys className == WC_FRAME) ||
         ( !is_apt( aptClipOwner) && ( var owner != application))) return self;
      self = var owner;
   }
   return nilHandle;
}

// hwnd end

HRGN
region_create( Handle self, Handle mask)
{
   LONG  w, h, x, y, size = 256;
   HRGN    rgn = nilHandle;
   Byte    * idata;
   RECTL   * rdata = nil;
   LONG      rcount = 0;
   RECTL   * current;
   Bool      set = 0;
   HPS       ps = sys ps;

   if ( !mask)
      return nilHandle;

   dobjCheck( mask) nilHandle;

   w = PImage( mask)-> w;
   h = PImage( mask)-> h;
   if ( dsys( mask) s. imgCachedRegion) {
      rgn = GpiCreateRegion( ps, 0, NULL);
      if ( !rgn || rgn == HRGN_ERROR) apiErr;
      GpiCombineRegion( ps, rgn, dsys( mask) s. imgCachedRegion, nilHandle, CRGN_COPY);
      return rgn;
   }

   idata  = PImage( mask)-> data + PImage( mask)-> dataSize - PImage( mask)-> lineSize;

   rdata = malloc( size * sizeof( RECTL));
   if ( !rdata) return nilHandle;

   current = rdata;
   current--;

   for ( y = h - 1; y >= 0; y--) {
      for ( x = 0; x < w; x++) {
         if ( idata[ x >> 3] == 0) {
            x += 7;
            continue;
         }
         if ( idata[ x >> 3] & ( 1 << ( 7 - ( x & 7)))) {
            if ( set && current-> yTop == y && current-> xRight == x)
               current-> xRight++;
            else {
               set = 1;
               if ( rcount >= size) {
                  void * xrdata = realloc( rdata, ( size *= 3) * sizeof( RECTL));
                  if ( !xrdata) {
                     free( rdata);
                     return nilHandle;
                  }
                  rdata = xrdata;
                  current = rdata;
                  current += rcount - 1;
               }
               rcount++;
               current++;
               current-> xLeft   = x;
               current-> yTop    = y + 1;
               current-> xRight  = x + 1;
               current-> yBottom = y;
            }
         }
      }
      idata -= PImage( mask)-> lineSize;
   }

   if ( set) {
      rgn = GpiCreateRegion( ps, rcount, rdata);
      if ( !rgn || rgn == HRGN_ERROR) apiErr;
      dsys( mask) s. imgCachedRegion = GpiCreateRegion( ps, 0, nilHandle);
      GpiCombineRegion( ps, dsys( mask) s. imgCachedRegion, rgn, nilHandle, CRGN_COPY);
   }
   free(( void *) rdata);
   return rgn;
}



// colors
#define stdDisabled  SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW
#define stdHilite    SYSCLR_HILITEFOREGROUND, SYSCLR_HILITEBACKGROUND
#define std3d        SYSCLR_BUTTONLIGHT     , SYSCLR_BUTTONDARK

static int buttonScheme[] = {
   SYSCLR_MENUTEXT, SYSCLR_BUTTONMIDDLE,
   SYSCLR_MENUTEXT, SYSCLR_BUTTONMIDDLE,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_BUTTONMIDDLE,
   std3d
};
static int sliderScheme[] = {
   SYSCLR_WINDOWTEXT,       SYSCLR_SCROLLBAR,
   SYSCLR_WINDOWSTATICTEXT, SYSCLR_SCROLLBAR,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW,
   std3d
};

static int dialogScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_DIALOGBACKGROUND,
   SYSCLR_ACTIVETITLETEXT, SYSCLR_ACTIVETITLETEXTBGND,
   SYSCLR_INACTIVETITLETEXT, SYSCLR_INACTIVETITLETEXTBGND,
   std3d
};
static int staticScheme[] = {
   SYSCLR_WINDOWSTATICTEXT, SYSCLR_WINDOW,
   SYSCLR_WINDOWSTATICTEXT, SYSCLR_WINDOW,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW,
   std3d
};
static int editScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_ENTRYFIELD,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_ENTRYFIELD,
   std3d
};
static int menuScheme[] = {
   SYSCLR_MENUTEXT, SYSCLR_MENU,
   SYSCLR_MENUHILITE, SYSCLR_MENUHILITEBGND,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_MENU,
   std3d
};

static int scrollScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_SCROLLBAR,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_FIELDBACKGROUND,
   std3d
};

static int windowScheme[] = {
   SYSCLR_WINDOWTEXT, SYSCLR_WINDOW,
   SYSCLR_ACTIVETITLETEXT, SYSCLR_ACTIVETITLETEXTBGND,
   SYSCLR_INACTIVETITLETEXT, SYSCLR_INACTIVETITLETEXTBGND,
   std3d
};
static int customScheme[] = {
   SYSCLR_OUTPUTTEXT, SYSCLR_WINDOW,
   stdHilite,
   SYSCLR_MENUDISABLEDTEXT, SYSCLR_WINDOW,
   std3d
};

static int ctx_wc2SCHEME[] =
{
   wcButton    , ( int) &buttonScheme,
   wcCheckBox  , ( int) &buttonScheme,
   wcRadio     , ( int) &buttonScheme,
   wcDialog    , ( int) &dialogScheme,
   wcSlider    , ( int) &sliderScheme,
   wcLabel     , ( int) &staticScheme,
   wcInputLine , ( int) &editScheme,
   wcEdit      , ( int) &editScheme,
   wcListBox   , ( int) &editScheme,
   wcCombo     , ( int) &editScheme,
   wcMenu      , ( int) &menuScheme,
   wcPopup     , ( int) &menuScheme,
   wcScrollBar , ( int) &scrollScheme,
   wcWindow    , ( int) &windowScheme,
   wcWidget    , ( int) &customScheme,
   endCtx
};


long
remap_color( HPS ps, long clr, Bool toSystem)
{
   long c = clr;
   if ( toSystem && ( c & clSysFlag))
   {
      int * scheme = ( int *) ctx_remap_def( clr & wcMask, ctx_wc2SCHEME, true, ( int) &customScheme);
      if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
      if ( clr == clSet)   c = 0xffffff; else
      if ( clr == clClear) c = 0; else
      c = WinQuerySysColor( HWND_DESKTOP, scheme[ (clr & clSysMask) - 1], 0);
   }
   return c;
}

Color
apc_lookup_color( const char * colorName)
{
   char buf[ 256];
   char *b;
   int len;

#define xcmp( name, stlen, retval)  if (( len == stlen) && ( strcmp( name, buf) == 0)) return retval

   strncpy( buf, colorName, 256);
   len = strlen( buf);
   for ( b = buf; *b; b++) *b = tolower(*b);

   switch( buf[0]) {
   case 'a':
       xcmp( "aqua", 4, 0x00FFFF);
       xcmp( "azure", 5, ARGB(240,255,255));
       break;
   case 'b':
       xcmp( "black", 5, 0x000000);
       xcmp( "blanchedalmond", 14, ARGB( 255,235,205));
       xcmp( "blue", 4, 0x000080);
       xcmp( "brown", 5, 0x808000);
       xcmp( "beige", 5, ARGB(245,245,220));
       break;
   case 'c':
       xcmp( "cyan", 4, 0x008080);
       xcmp( "chocolate", 9, ARGB(210,105,30));
       break;
   case 'd':
       xcmp( "darkgray", 8, 0x404040);
       break;
   case 'e':
       break;
   case 'f':
       xcmp( "fuchsia", 7, 0xFF00FF);
       break;
   case 'g':
       xcmp( "green", 5, 0x008000);
       xcmp( "gray", 4, 0x808080);
       xcmp( "gray80", 6, ARGB(204,204,204));
       xcmp( "gold", 4, ARGB(255,215,0));
       break;
   case 'h':
       xcmp( "hotpink", 7, ARGB(255,105,180));
       break;
   case 'i':
       xcmp( "ivory", 5, ARGB(255,255,240));
       break;
   case 'j':
       break;
   case 'k':
       xcmp( "khaki", 5, ARGB(240,230,140));
       break;
   case 'l':
       xcmp( "lime", 4, 0x00FF00);
       xcmp( "lightgray", 9, 0xC0C0C0);
       xcmp( "lightblue", 9, 0x0000FF);
       xcmp( "lightgreen", 10, 0x00FF00);
       xcmp( "lightcyan", 9, 0x00FFFF);
       xcmp( "lightmagenta", 12, 0xFF00FF);
       xcmp( "lightred", 8, 0xFF0000);
       xcmp( "lemon", 5, ARGB(255,250,205));
       break;
   case 'm':
       xcmp( "maroon", 6, 0x800000);
       xcmp( "magenta", 7, 0x800080);
       break;
   case 'n':
       xcmp( "navy", 4, 0x000080);
       break;
   case 'o':
       xcmp( "olive", 5, 0x808000);
       xcmp( "orange", 6, ARGB(255,165,0));
       break;
   case 'p':
       xcmp( "purple", 6, 0x800080);
       xcmp( "peach", 5, ARGB(255,218,185));
       xcmp( "peru", 4, ARGB(205,133,63));
       xcmp( "pink", 4, ARGB(255,192,203));
       xcmp( "plum", 4, ARGB(221,160,221));
       break;
   case 'q':
       break;
   case 'r':
       xcmp( "red", 3, 0x800000);
       xcmp( "royalblue", 9, ARGB(65,105,225));
       break;
   case 's':
       xcmp( "silver", 6, 0xC0C0C0);
       xcmp( "sienna", 6, ARGB(160,82,45));
       break;
   case 't':
       xcmp( "teal", 4, 0x008080);
       xcmp( "turquoise", 9, ARGB(64,224,208));
       xcmp( "tan", 3, ARGB(210,180,140));
       xcmp( "tomato", 6, ARGB(255,99,71));
       break;
   case 'u':
       break;
   case 'w':
       xcmp( "white", 5, 0xFFFFFF);
       xcmp( "wheat", 5, ARGB(245,222,179));
       break;
   case 'v':
       xcmp( "violet", 6, ARGB(238,130,238));
       break;
   case 'x':
       break;
   case 'y':
       xcmp( "yellow", 6, 0xFFFF00);
       break;
   case 'z':
       break;
   }

#undef xcmp

   return clInvalid;
}
// colors end
// fonts

#define fgBitmap 0
#define fgVector 1

static Bool recursiveFF = 0;
static FONTMETRICS fmtx;

FIXED float2fixed( double f)
{
   double d1, d2;
   d2 = modf( f, &d1);
   return MAKEFIXED( d1, d2 * 65536);
}

double fixed2float( FIXED f)
{
   double r;
   r = FIXEDINT( f) + FIXEDFRAC( f) / 65536.0;
   return r;
}

static int
font_font2gp_internal( PFont font, Point res, Bool forceSize)
{
   int resId, resValue, vecId, widValue, heiValue, ascent, descent, cp, cppassed = 0;
   LONG i, count;
   PFONTMETRICS fm;
   Bool useWidth       = font-> width != 0;
   Bool useDirection   = font-> direction   != 0;
   Bool usePitch       = font-> pitch;
   #define gotta( w, result){font-> width  = w;       \
                         free( fm);                \
                         font-> ascent = ascent;   \
                         font-> descent = descent; \
                         return result;}
   #define ascents( fontmtx)               \
       ascent = fontmtx.  lMaxAscender;    \
       descent = fontmtx.  lMaxDescender;

   cp = font_enc2cp( font-> encoding);

   i = 0;
   // querying font for given name
   {
      SIZEF sz;
      if ( !GpiQueryCharBox( guts. ps, &sz)) apiErr;
      if ( !GpiSetCharBox( guts. ps, &guts. defFontBox)) apiErr;
      count = GpiQueryFonts( guts. ps, QF_PUBLIC, font->name, &i, sizeof( FONTMETRICS), nil);
      if ( count < 0) apiErrRet;
      if ( !( fm = malloc( count * sizeof( FONTMETRICS))))
         return 0;
      if ( GpiQueryFonts( guts. ps, QF_PUBLIC, font->name, &count, sizeof( FONTMETRICS), fm) < 0)
        apiErrRet;
      if ( !GpiSetCharBox( guts. ps, &sz)) apiErr;
   }

   resId = vecId = -1;
   resValue = widValue = heiValue = INT_MAX;
   // cycling for appropriate value
   for ( i = 0; i < count; i++)
   {
      PFONTMETRICS f = &fm[ i];

      if ( cp != 65535 && cp != f-> usCodePage) continue;
      cppassed++;

      if ( usePitch)
      {
         int fpitch = ( f-> fsType & FM_TYPE_FIXED) ? fpFixed : fpVariable;
         if ( fpitch != font-> pitch) continue;
      }

      if ( f-> fsDefn & FM_DEFN_OUTLINE)
         vecId = i;
      else {
         long scr = ( f-> sXDeviceRes - res. x) * ( f-> sXDeviceRes - res. x) +
                    ( f-> sYDeviceRes - res. y) * ( f-> sYDeviceRes - res. y);
         if ( scr < resValue && ( !forceSize || ( f-> sNominalPointSize == font-> size * 10))) {
            resId    = i;
            resValue = scr;
            heiValue = ( f-> lMaxBaselineExt - font-> height) * ( f-> lMaxBaselineExt - font-> height);
            if ( useWidth)
               widValue = ( f-> lAveCharWidth - font-> width) * ( f-> lAveCharWidth - font-> width);
         } else if ( !forceSize && scr == resValue) {
            long hei = ( f-> lMaxBaselineExt - font-> height) * ( f-> lMaxBaselineExt - font-> height);
            if ( hei < heiValue) {
               resId    = i;
               heiValue = hei;
               if ( useWidth)
                  widValue = ( f-> lAveCharWidth - font-> width) * ( f-> lAveCharWidth - font-> width);
            } else if ( useWidth && ( hei == heiValue)) {
               long wid = ( f-> lAveCharWidth - font-> width) * ( f-> lAveCharWidth - font-> width);
               if ( wid < widValue) {
                  resId    = i;
                  widValue = wid;
               }
            }
         }
      } // end else
   } // end loop

   if ( cppassed == 0 && cp != 65535) { // no codepage
      font-> encoding[0] = 0;
      return font_font2gp_internal( font, res, forceSize);
   }

 // is font aspectable?
   if ( useDirection) {
      if ( vecId < 0) {
         if ( !usePitch && ( resId >= 0))
            font-> pitch = ( fm[ resId]. fsType & FM_TYPE_FIXED) ? fpFixed : fpVariable;
         usePitch = 1; // if not aspectable, force it.
      }
      resId = -1;
   }

   // have resolution ok?
   if (
         resId >= 0 &&                                  // if resolution match found
         resValue < 16 &&                               // and difference is no more than 4 points
         ( forceSize ||                                 // enough for size pickup -
            ( heiValue < 5 &&                           // but if picking for height,
               ( !useWidth ||                           // 4 pts y-difference is ok, and enough
                  ( widValue == 0 && heiValue == 0)     // if no width pickup specified, else
               )                                        // same 4 pts x-diff check.
            )
         )
      )
   {
      font-> height = fm[ resId]. lMaxBaselineExt;
      ascents( fm[ resId]);
      memcpy( &fmtx, &fm[ resId], sizeof( fmtx));
      font-> size   = fm[ resId]. sNominalPointSize / 10;
      font_fontmetrics2font( &fm[ resId], font, true);
      gotta( fm[ resId]. lAveCharWidth, fgBitmap);
   }
   // or vector font - for any purpose?

   if ( vecId >= 0)
   {
      if ( forceSize) {
         int selSize = fm[ vecId]. sNominalPointSize;
         // font-> height = fm[ vecId]. lMaxBaselineExt * font-> size * 10.0 / selSize + 0.5;
         font-> height = font-> size * res. y / 72.0 + 0.5;
         ascent  = fm[ vecId]. lMaxAscender  * font-> size * 10.0 / selSize + 0.5;
         descent = fm[ vecId]. lMaxDescender * font-> size * 10.0 / selSize + 0.5;
         memcpy( &fmtx, &fm[ vecId], sizeof( fmtx));
         #define fmtx_adj( field) fmtx. field = fmtx. field * font-> size * 10 / selSize + 0.5
         fmtx_adj( lAveCharWidth    );
         fmtx_adj( lMaxBaselineExt  );
         fmtx_adj( lMaxAscender     );
         fmtx_adj( lLowerCaseAscent );
         fmtx_adj( lMaxDescender    );
         fmtx_adj( lLowerCaseDescent);
         fmtx_adj( lExternalLeading );
         fmtx_adj( lInternalLeading );
         fmtx_adj( lMaxCharInc      );
         #undef fmtx_adj
         // gotta( fm[ vecId]. lMaxCharInc * font-> size * 10.0 / selSize + 0.5, fgVector);
      } else {
         #define mult font-> height / fm[ vecId]. lMaxBaselineExt + 0.5
         ascent  = fm[ vecId]. lMaxAscender  * mult;
         descent = fm[ vecId]. lMaxDescender * mult;
         font-> size = font-> height * 72.0 / res. y + 0.5;
         memcpy( &fmtx, &fm[ vecId], sizeof( fmtx));
         #define fmtx_adj( field) fmtx. field = fmtx. field * mult
         fmtx_adj( lAveCharWidth    );
         fmtx_adj( lMaxBaselineExt  );
         fmtx_adj( lMaxAscender     );
         fmtx_adj( lLowerCaseAscent );
         fmtx_adj( lMaxDescender    );
         fmtx_adj( lLowerCaseDescent);
         fmtx_adj( lExternalLeading );
         fmtx_adj( lInternalLeading );
         fmtx_adj( lMaxCharInc      );
         #undef fmtx_adj
         #undef mult
      }
      font_fontmetrics2font( &fm[ vecId], font, true);
      if ( useWidth)
         gotta( font-> width, fgVector)
      else
         gotta( font-> height, fgVector)
   }

   // no need in this
   free( fm);  fm = nil;


   // if strict font not found, use subplacing
   if ( usePitch)
   {
       int ogp  = font-> pitch;
       int ret;
       strcpy( font-> name, ( font-> pitch == fpFixed) ?
           DEFAULT_FIXED_FONT : DEFAULT_VARIABLE_FONT);
       font-> pitch = fpDefault;
       ret = font_font2gp_internal( font, res, forceSize);
       if ( ogp == fpFixed && ( strcmp( font-> name, DEFAULT_FIXED_FONT) != 0))
       {
          strcpy( font-> name, DEFAULT_SYSTEM_FONT);
          font-> pitch = fpDefault;
          ret = font_font2gp_internal( font, res, forceSize);
       }
       return ret;
   }

   // Font not found. So use general representation for height and width
   strcpy( font-> name, DEFAULT_FONT_NAME);
   if ( recursiveFF == 0)
   {
      // trying to catch default font witch correct values
      int r;
      recursiveFF++;
      r = font_font2gp_internal( font, res, forceSize);
      recursiveFF--;
      return r;
   } else {
      int r;
      // if not succeeded, to avoid recursive call use "wild guess".
      // This could be achieved if system does not have "Helv" font
      *font = guts. sysDefFont;
      font-> pitch = fpDefault;
      recursiveFF++;
      r = font_font2gp_internal( font, res, forceSize);
      recursiveFF--;
      return r;
   }
   return fgBitmap;
}

int
font_font2gp( PFont font, Point res, Bool forceSize)
{
   int vectored;
   Font key;
   Bool addSizeEntry;

   font-> resolution = res. y * 0x10000 + res. x;
   if ( get_font_from_hash( font, &vectored, &fmtx, forceSize))
      return vectored;
   memcpy( &key, font, sizeof( Font));
   vectored = font_font2gp_internal( font, res, forceSize);
   font-> resolution = res. y * 0x10000 + res. x;
   if ( forceSize) {
      key. height = font-> height;
      key. width  = font-> width;
      addSizeEntry = true;
   } else {
      key. size  = font-> size;
      addSizeEntry = vectored ? (( key. height == key. width)  || ( key. width == 0)) : true;
   }
   add_font_to_hash( &key, font, vectored, &fmtx, addSizeEntry);
   return vectored;
}

void
font_pp2gp( char * ppFontNameSize, PFont font)
{
   char * p = strchr( ppFontNameSize, '.');
   int i;

   if ( p)
   {
      font-> size = atoi( ppFontNameSize);
      p++;
   } else {
      font-> size = DEFAULT_FONT_SIZE;
      p = DEFAULT_FONT_NAME;
   }
   font-> width = font-> height = C_NUMERIC_UNDEF;
   font-> direction = 0;
   font-> pitch = fpDefault;
   font-> style = fsNormal;
   strcpy( font-> name, p);
   p = font-> name;
   for ( i = strlen( p) - 1; i >= 0; i--)
   {
      if ( p[ i] == '.')
      {
         if ( strcmp( "Italic",     &p[ i + 1]) == 0) font-> style |= fsItalic;
         if ( strcmp( "Underscore", &p[ i + 1]) == 0) font-> style |= fsUnderlined;
         if ( strcmp( "Strikeout",  &p[ i + 1]) == 0) font-> style |= fsStruckOut;
         if ( strcmp( "Bold",       &p[ i + 1]) == 0) font-> style |= fsBold;
         if ( strcmp( "Outline",    &p[ i + 1]) == 0) font-> style |= fsOutline;
         p[ i] = 0;
      }
   }
}

void
font_gp2pp( PFont font, char * buf)
{
   sprintf ( buf, "%d.%s%s%s%s%s%s", font-> size, font-> name,
      ( font-> style & fsItalic)     ? ".Italic"     : "",
      ( font-> style & fsBold)       ? ".Bold"       : "",
      ( font-> style & fsStruckOut)  ? ".Strikeout"  : "",
      ( font-> style & fsUnderlined) ? ".Underlined" : "",
      ( font-> style & fsOutline)    ? ".Outline"    : ""
   );
}

int
font_style( PFONTMETRICS fm)
{
   int style = fsNormal; // 0
   if ( fm-> fsSelection & FM_SEL_ITALIC)     style |= fsItalic;
   if ( fm-> fsSelection & FM_SEL_UNDERSCORE) style |= fsUnderlined;
   if ( fm-> fsSelection & FM_SEL_STRIKEOUT)  style |= fsStruckOut;
   if ( fm-> fsSelection & FM_SEL_BOLD)       style |= fsBold;
   if ( fm-> fsSelection & FM_SEL_OUTLINE)    style |= fsOutline;
   return style;
}

USHORT
font_enc2cp( const char * encoding)
{
   if ( !encoding || !encoding[0]) return 65535;
   if ( stricmp( encoding, FONT_FONTSPECIFIC) == 0)
      return 65400;
   return ( USHORT) atoi( encoding);
}

char *
font_cp2enc( USHORT codepage)
{
   static Bool initialized = false;
   static PHash hash = nil;
   char * d, buf[32];
   void * r;
   if ( !initialized) {
      hash = hash_create();
      initialized = true;
   }
   if ( codepage == 65400)
      d = FONT_FONTSPECIFIC;
   else
      sprintf( d = buf, "%d", codepage);
   r = hash_fetch( hash, d, strlen( d));
   if ( r) return ( char*) r;
   d = duplicate_string( d);
   hash_store( hash, d, strlen( d), d);
   return d;
}


void
font_fontmetrics2font( PFONTMETRICS m, PFont f, Bool readonly)
{
   if ( !readonly) {
      f-> size               = m-> sNominalPointSize / 10;
//    f-> width              = m-> lAveCharWidth    ;
      f-> width              = m-> lMaxCharInc      ;
      f-> height             = m-> lMaxBaselineExt  ;
      f-> style              = font_style( m)       ;
   }
   f-> pitch              =
      (( m-> fsType & FM_TYPE_FIXED   ) ? fpFixed  : fpVariable);
   strcpy( f-> family,   m-> szFamilyname);
   if ( m-> fsType & FM_TYPE_FACETRUNC)
      WinQueryAtomName( WinQuerySystemAtomTable(), m-> FaceNameAtom, f-> name, 256);
   else
      strcpy( f-> name, m-> szFacename);
   f-> weight             = m-> usWeightClass    ;
   f-> ascent             = m-> lMaxAscender     ;
   f-> descent            = m-> lMaxDescender    ;
   f-> maximalWidth       = m-> lMaxCharInc      ;
   f-> internalLeading    = m-> lInternalLeading ;
   f-> externalLeading    = m-> lExternalLeading ;
   f-> xDeviceRes         = m-> sXDeviceRes      ;
   f-> yDeviceRes         = m-> sYDeviceRes      ;
   f-> firstChar          = m-> sFirstChar       ;
   f-> lastChar           = m-> sLastChar        ;
   f-> breakChar          = m-> sBreakChar       ;
   f-> defaultChar        = m-> sDefaultChar     ;
   f-> utf8_flags         = 0                    ;
   strcpy( f-> encoding, font_cp2enc( m-> usCodePage));
}

static PFont
spec_fonts( int count, PFONTMETRICS fm, int * retCount)
{
   int i;
   List list;
   PHash hash = nil;
   PFont fmtx = nil;

   list_create( &list, 256, 256);
   if ( !( hash = hash_create())) {
      list_destroy( &list);
      return nil;
   }

   for ( i = 0; i < count; i++) {
      PFont f;

      f = hash_fetch( hash, fm[i]. szFacename, strlen( fm[ i]. szFacename));
      if ( f) {
         char ** enc = (char**) f-> encoding;
         unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
         if ( *shift + 2 < 256 / sizeof(char*)) {
            int j, exists = 0;
            char * e = font_cp2enc( fm[i]. usCodePage);
            for ( j = 1; j <= *shift; j++) {
               if ( strcmp( enc[j], e) == 0) {
                  exists = 1;
                  break;
               }
            }
            if ( exists) continue;
            *(enc + ++(*shift)) = e;
         }
         continue;
      }

      if ( !( f = ( PFont) malloc( sizeof( Font)))) {
         if ( hash) hash_destroy( hash, false);
         list_delete_all( &list, true);
         list_destroy( &list);
         return nil;
      }

      font_fontmetrics2font( &fm[ i], f, false);

      { /* multi-encoding format */
         char ** enc = (char**) f-> encoding;
         unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
         memset( f-> encoding, 0, 256);
         *(enc + ++(*shift)) = font_cp2enc( fm[i]. usCodePage);
         hash_store( hash, fm[i]. szFacename, strlen( fm[ i]. szFacename), f);
      }
      list_add( &list, ( Handle) f);
   }
   if ( hash) hash_destroy( hash, false);

   if ( list. count == 0) goto Nothing;

   fmtx = ( PFont) malloc( list. count * sizeof( Font));
   if ( !fmtx) {
      list_delete_all( &list, true);
      list_destroy( &list);
      return nil;
   }

   *retCount = list. count;
      for ( i = 0; i < list. count; i++)
         memcpy( fmtx + i, ( void *) list. items[ i], sizeof( Font));
   list_delete_all( &list, true);

Nothing:
   list_destroy( &list);
   return fmtx;
}

PFont
apc_fonts( Handle self, const char* facename, const char * encoding, int * retCount)
{
   PFont fmtx;
   PFONTMETRICS fm;
   LONG i = 0, j, count;
   Bool hasdc = 0;
   HPS ps;
   USHORT cp = 65535;

   if ( self == nilHandle || self == application)
      ps = guts. ps;
   else if ( kind_of( self, CPrinter)) {
      if ( !is_opt( optInDraw) && !is_opt( optInDrawInfo)) {
         hasdc = 1;
         CPrinter( self)-> begin_paint_info( self);
      }
      ps = sys ps;
   } else
      return nil;

   apcErrClear;
   if ( encoding) cp = font_enc2cp( encoding);
   count = GpiQueryFonts( guts. ps, QF_PUBLIC, facename, &i, sizeof( FONTMETRICS), nil);
   if ( count < 0) { apiErr; return nil; }
   if ( !( fm = malloc( count * sizeof( FONTMETRICS)))) {
      if ( hasdc) CPrinter( self)-> end_paint_info( self);
      return nil;
   }
   if ( GpiQueryFonts( guts. ps, QF_PUBLIC, facename, &count, sizeof( FONTMETRICS), fm) < 0) {
       apiErr;
       free( fm);
       if ( hasdc) CPrinter( self)-> end_paint_info( self);
       return nil;
   }

   if ( facename == nil && encoding == nil) {
      fmtx = spec_fonts( count, fm, retCount);
      free( fm);
      if ( hasdc) CPrinter( self)-> end_paint_info( self);
      return fmtx;
   }


   if ( facename == nil) {
      int     rc = 0;
      char ** table = malloc( count * sizeof( char*));
      if ( !table) {
         free( fm);
         if ( hasdc) CPrinter( self)-> end_paint_info( self);
         return nil;
      }
      for ( i = 0; i < count; i++)
      {
         Bool found = false;

         if ( cp != 65535 && cp != fm[ i]. usCodePage) {
            fm[ i].szFacename[0] = 0;
            continue;
         }

         for ( j = 0; j < rc; j++)
         {
            // if ( strcmp( fm[ i].szFamilyname, table[ j]) == 0)
            if ( strcmp( fm[ i].szFacename, table[ j]) == 0)
            {
               found = true;
               fm[ i].szFacename[0] = 0;
               break;
            }
         }
         if ( !found) table[ rc++] = fm[ i].szFacename;
      }
      free( table);
      *retCount = rc;
   } else {
      *retCount = 0;
      for ( i = 0; i < count; i++) {
         if ( cp != 65535 && cp != fm[ i]. usCodePage) {
            fm[ i].szFacename[0] = 0;
            continue;
         }
         (*retCount)++;
      }
   }

   if ( !( fmtx = malloc( *retCount * sizeof( Font)))) {
      free( fm);
      if ( hasdc) CPrinter( self)-> end_paint_info( self);
      *retCount = 0;
      return nil;
   }
   memset( fmtx, 0, *retCount * sizeof( Font));

   for ( i = 0, j = 0; i < count; i++)
   {
      if ( fm[ i]. szFacename[0] == 0) continue;
      font_fontmetrics2font( &fm[ i], &fmtx[ j], false);
      j++;
   }
   free( fm);
   if ( hasdc) CPrinter( self)-> end_paint_info( self);
   return fmtx;
}

PHash
apc_font_encodings( Handle self)
{
   PHash ret = nil;
   PFONTMETRICS fm;
   Bool hasdc = 0;
   HPS ps;
   char buf[64], *fontspecific = FONT_FONTSPECIFIC;
   int len, fslen = strlen( fontspecific);
   LONG i = 0, count;

   if ( self == nilHandle || self == application)
      ps = guts. ps;
   else if ( kind_of( self, CPrinter)) {
      if ( !is_opt( optInDraw) && !is_opt( optInDrawInfo)) {
         hasdc = 1;
         CPrinter( self)-> begin_paint_info( self);
      }
      ps = sys ps;
   } else
      return nil;

   apcErrClear;
   count = GpiQueryFonts( guts. ps, QF_PUBLIC, NULL, &i, sizeof( FONTMETRICS), nil);
   if ( count < 0) { apiErr; return nil; }
   if ( !( fm = malloc( count * sizeof( FONTMETRICS)))) {
      if ( hasdc) CPrinter( self)-> end_paint_info( self);
      return nil;
   }
   if ( GpiQueryFonts( guts. ps, QF_PUBLIC, NULL, &count, sizeof( FONTMETRICS), fm) < 0) {
       apiErr;
       free( fm);
       if ( hasdc) CPrinter( self)-> end_paint_info( self);
       return nil;
   }

   ret = hash_create();
   for ( i = 0; i < count; i++) {
       char * x;
       if ( fm[ i]. usCodePage == 65400) {
          x = fontspecific;
          len = fslen;
       } else
          len = sprintf( x = buf, "%d", fm[i]. usCodePage);
       hash_store( ret, x, len, (void*)1);
   }

   free( fm);
   if ( hasdc) CPrinter( self)-> end_paint_info( self);

   return ret;
}

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
   font_font2gp( dest, self ? sys res : guts. displayResolution,
      Drawable_font_add( self, source, dest));
   return true;
}

PFont
apc_font_default( PFont font)
{
   *font = guts. sysDefFont;
   font-> pitch = fpDefault;
   return font;
}

PFont
apc_sys_get_caption_font( PFont font)
{
   char buf  [256];
   apcErrClear;
   PrfQueryProfileString( HINI_USERPROFILE, "PM_SystemFonts", "WindowTitles", DEFAULT_FONT_FACE, buf, 255);
   font_pp2gp( buf, font);
   apc_font_pick( nilHandle, font, font);
   return font;
}

PFont
apc_sys_get_msg_font( PFont font)
{
   *font = guts. sysDefFont;
   font-> pitch = fpDefault;
   return font;
}

PFont
gp_get_font( HPS ps, PFont font, Point res)
{
   Bool vector;
   if ( !GpiQueryFontMetrics( ps, sizeof( fmtx), &fmtx)) { apiErr; return font; }
   vector = fmtx. fsDefn & FM_DEFN_OUTLINE;
   font-> direction   = 0;
   if ( vector)
   {
      SIZEF sz;
      GRADIENTL g;
      GpiQueryCharBox( ps, &sz);
      font-> width   = FIXEDINT( sz. cx);
      font-> height  = FIXEDINT( sz. cy);
      font-> size    = fixed2float( sz. cy) * 72.0 / res. y;
      GpiQueryCharAngle( ps, &g);
      if ( g. x != 0 || g. y != 0) font-> direction   = atan2( g. y, g. x) * GRAD;
   } else {
      font-> height = font-> width = C_NUMERIC_UNDEF;
      font-> size   = fmtx. sNominalPointSize / 10;
   }
   font-> style = font_style( &fmtx);
   font_fontmetrics2font( &fmtx, font, true);
   font-> resolution = res. y * 0x10000 + res. x;
   return font;
}

PFont
view_get_font ( Handle self, PFont font)
{
   char buf  [256];
   if ( WinQueryPresParam( var handle, PP_FONTNAMESIZE, 0, nil, 256,
        &buf, QPF_NOINHERIT) && strchr( buf, '.'))
   {
      font_pp2gp( buf, font);
      apc_font_pick( self, font, font);
   }
      else apc_widget_default_font( font);
   return font;
}

int
apc_font_load( const char* filename)
{
   return 0;
}



// fonts end
