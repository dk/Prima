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

/*
      stock.c

      Stock objects - pens, brushes, fonts
*/

#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include <ctype.h>
#include "Img.h"
#include "Window.h"
#include "Image.h"
#include "Printer.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

// Stylus section
PDCStylus
stylus_alloc( PStylus data)
{
   Bool extPen = data-> extPen. actual;
   PDCStylus ret = hash_fetch( stylusMan, data, sizeof( Stylus) - ( extPen ? 0 : sizeof( EXTPEN)));
   if ( ret == nil) {
      LOGPEN * p;
      LOGBRUSH * b;
      LOGBRUSH   xbrush;

      if ( hash_count( stylusMan) > 128)
         stylus_clean();

      ret = malloc( sizeof( DCStylus));
      memcpy( &ret-> s, data, sizeof( Stylus));
      ret-> refcnt = 0;
      p = &ret-> s. pen;

      if ( IS_WIN95 && extPen) {
         extPen = 0;
         p-> lopnStyle = PS_SOLID;
      }

      if ( !extPen) {
         if ( !( ret-> hpen = CreatePenIndirect( p))) {
            apiErr;
            ret-> hpen = CreatePen( PS_SOLID, 0, 0);
         }
      } else {
         int i, delta = p-> lopnWidth. x > 1 ? p-> lopnWidth. x - 1 : 0;
         LOGBRUSH pb;
	 pb. lbStyle = BS_SOLID;
	 pb. lbColor = ret-> s. pen. lopnColor;
	 pb. lbHatch = 0;
         for ( i = 1; i < ret-> s. extPen. patResource-> dotsCount; i += 2)
            ret-> s. extPen. patResource-> dotsPtr[ i] += delta;
         if ( !( ret-> hpen   = ExtCreatePen( ret-> s. extPen. style, p-> lopnWidth. x, &pb,
            ret-> s. extPen. patResource-> dotsCount,
            ret-> s. extPen. patResource-> dotsPtr
         ))) {
            if ( !IS_WIN95) apiErr;
            ret-> hpen = CreatePen( PS_SOLID, 0, 0);
         }
         for ( i = 1; i < ret-> s. extPen. patResource-> dotsCount; i += 2)
            ret-> s. extPen. patResource-> dotsPtr[ i] -= delta;
      }
      b = &ret-> s. brush. lb;
      if ( ret-> s. brush. lb. lbStyle == BS_DIBPATTERNPT) {
         if ( ret-> s. brush. backColor == ret-> s. pen. lopnColor) {
            // workaround Win32 bug with mono bitmaps -
            // if color and backColor are the same, but fill pattern present, backColor
            // value is ignored by some unknown, but certainly important reason :)
            xbrush. lbStyle = BS_SOLID;
            xbrush. lbColor = ret-> s. pen. lopnColor;
            xbrush. lbHatch = 0;
            b = &xbrush;
         } else {
            int i;
            for ( i = 0; i < 8; i++) bmiHatch. bmiData[ i * 4] = ret-> s. brush. pattern[ i];
            bmiHatch. bmiColors[ 0]. rgbRed   =  ( ret-> s. brush. backColor & 0xFF);
            bmiHatch. bmiColors[ 0]. rgbGreen = (( ret-> s. brush. backColor >> 8) & 0xFF);
            bmiHatch. bmiColors[ 0]. rgbBlue  = (( ret-> s. brush. backColor >> 16) & 0xFF);
            bmiHatch. bmiColors[ 1]. rgbRed   =  ( ret-> s. pen. lopnColor & 0xFF);
            bmiHatch. bmiColors[ 1]. rgbGreen = (( ret-> s. pen. lopnColor >> 8) & 0xFF);
            bmiHatch. bmiColors[ 1]. rgbBlue  = (( ret-> s. pen. lopnColor >> 16) & 0xFF);
         }
      }
      if ( !( ret-> hbrush = CreateBrushIndirect( b))) {
         apiErr;
         ret-> hbrush = CreateSolidBrush( RGB( 255, 255, 255));
      }
      hash_store( stylusMan, &ret-> s, sizeof( Stylus) - ( extPen ? 0 : sizeof( EXTPEN)), ret);
   }
   ret-> refcnt++;
   return ret;
}

void
stylus_free( PDCStylus res, Bool permanent)
{
   if ( !res || --res-> refcnt > 0) return;
   if ( !permanent) {
      res-> refcnt = 0;
      return;
   }
   if ( res-> hpen)   DeleteObject( res-> hpen);
   if ( res-> hbrush) DeleteObject( res-> hbrush);
   res-> hpen = res-> hpen = nil;
   hash_delete( stylusMan, &res-> s, sizeof( Stylus) - ( res-> s. extPen. actual ? 0 : sizeof( EXTPEN)), true);
}

void
stylus_change( Handle self)
{
   PDCStylus p;
   PDCStylus newP;
   if ( is_apt( aptDCChangeLock)) return;
   p    = sys stylusResource;
   newP = stylus_alloc( &sys stylus);
   if ( p != newP) {
      sys stylusResource = newP;
      sys stylusFlags = 0;
   }
   stylus_free( p, false);
}

static Bool _st_cleaner( PDCStylus s, int keyLen, void * key, void * dummy) {
   if ( s-> refcnt <= 0) stylus_free( s, true);
   return false;
}

void
stylus_clean()
{
   hash_first_that( stylusMan, _st_cleaner, nil, nil, nil);
}

Bool
stylus_extpenned( PStylus s, int excludeFlags)
{
   Bool ext = false;
   if ( s-> pen. lopnWidth. x > 1) {
       if (( s-> pen. lopnStyle != PS_SOLID) && ( s-> pen. lopnStyle != PS_NULL))
          return true;
   } else if ( s-> pen. lopnStyle == PS_USERSTYLE)
      return true;
   if ( !( excludeFlags & exsLinePattern))
      ext |= s-> pen. lopnStyle == PS_USERSTYLE;
   if ( !( excludeFlags & exsLineEnd))
      ext |= s-> extPen. lineEnd != PS_ENDCAP_ROUND;
   return ext;
}

Bool
stylus_complex( PStylus s, HDC dc)
{
   int rop;
   if ( s-> brush. lb. lbStyle == BS_DIBPATTERNPT)
      return true;
   if (( s-> pen. lopnStyle != PS_SOLID) &&
       ( s-> pen. lopnStyle != PS_NULL))
      return true;
   rop = GetROP2( dc);
   if (( rop != R2_COPYPEN) &&
       ( rop != R2_WHITE  ) &&
       ( rop != R2_NOP    ) &&
       ( rop != R2_BLACK  )) return true;
   return false;
}


DWORD
stylus_get_extpen_style( PStylus s)
{
   return s-> extPen. lineEnd | s-> pen. lopnStyle | PS_GEOMETRIC;
}

PPatResource
patres_fetch( char * pattern, int len)
{
   int i;
   PPatResource r = hash_fetch( patMan, pattern, len);
   if ( r)
      return r;

   r = malloc( sizeof( PatResource) + sizeof( DWORD) * len);
   r-> dotsCount = len;
   r-> dotsPtr   = r-> dots;
   for ( i = 0; i < len; i++) {
      DWORD x = ( DWORD) pattern[ i];
      if ( i & 1)
         x++;
      else
         if ( x > 0) x--;
      r-> dots[ i] = x;
   }
   hash_store( patMan, pattern, len, r);
   return r;
}

UINT
patres_user( char * pattern, int len)
{
   if ( IS_WIN95) {
      // no user line patterns
      switch ( len) {
      case 0:
         return PS_NULL;
      case 1:
         return pattern[0] ? PS_SOLID : PS_NULL;
      case 2:
         if ( memcmp( pattern, psDot, 2) == 0)
            return PS_DOT;
         if ( memcmp( pattern, lpDot, 2) == 0)
            return PS_DOT;
         if ( memcmp( pattern, psDash, 2) == 0)
            return PS_DASH;
         if ( memcmp( pattern, lpDash, 2) == 0)
            return PS_DASH;
         return PS_DOT;
      case 4:
         return PS_DASHDOT;
      case 5:
      case 6:
         return PS_DASHDOTDOT;
      default:
         return PS_SOLID;
      }
   } else {
      switch ( len) {
      case 0:
         return PS_NULL;
      case 1:
         return pattern[0] ? PS_SOLID : PS_NULL;
      default:
         return PS_USERSTYLE;
      }
   }
}

// Stylus end

// Font section


#define FONTHASH_SIZE 563
#define FONTIDHASH_SIZE 23

typedef struct _FontInfo {
   Font          font;
   int           vectored;
} FontInfo;

typedef struct _FontHashNode
{
   struct _FontHashNode *next;
   struct _FontHashNode *next2;
   Font key;
   FontInfo value;
} FontHashNode, *PFontHashNode;

typedef struct _FontHash
{
   PFontHashNode buckets[ FONTHASH_SIZE];
} FontHash, *PFontHash;

static FontHash fontHash;
static FontHash fontHashBySize;

static unsigned long
elf_hash( const char *name, int size)
{
   unsigned long   h = 0, g;

   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   while ( *name)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
   }
   return h;
}

static unsigned long
elf_hash_by_size( const Font *f)
{
   unsigned long   h = 0, g;
   char *name = (char *)&(f-> width);
   int size = (char *)(&(f-> name)) - (char *)name;

   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   while ( *name)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
   }
   name = (char *)&(f-> size);
   size = sizeof( f-> size);
   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   return h;
}

static PFontHashNode
find_node( const PFont font, Bool bySize)
{
   unsigned long i;
   PFontHashNode node;
   int sz;

   if ( font == nil) return nil;
   if (bySize) {
      sz = (char *)(&(font-> name)) - (char *)&(font-> width);
      i = elf_hash_by_size( font) % FONTHASH_SIZE;
   } else {
      sz = (char *)(&(font-> name)) - (char *)font;
      i = elf_hash((const char *)font, sz) % FONTHASH_SIZE;
   }
   if (bySize)
      node = fontHashBySize. buckets[ i];
   else
      node = fontHash. buckets[ i];
   if ( bySize) {
      while ( node != nil)
      {
         if (( memcmp( &(font-> width), &(node-> key. width), sz) == 0) &&
             ( strcmp( font-> name, node-> key. name) == 0 ) &&
             (font-> size == node-> key. size))
            return node;
         node = node-> next2;
      }
   } else {
      while ( node != nil)
      {
         if (( memcmp( font, &(node-> key), sz) == 0) && ( strcmp( font-> name, node-> key. name) == 0 ))
            return node;
         node = node-> next;
      }
   }
   return nil;
}

Bool
add_font_to_hash( const PFont key, const PFont font, int vectored, Bool addSizeEntry)
{
   PFontHashNode node;
   unsigned long i;
   unsigned long j;
   int sz;

//   if ( find_node( key) != nil)
//      return false ;
   node = malloc( sizeof( FontHashNode));
   if ( node == nil) return false;
   memcpy( &(node-> key), key, sizeof( Font));
   memcpy( &(node-> value. font), font, sizeof( Font));
   node-> value. vectored = vectored;
   sz = (char *)(&(key-> name)) - (char *)key;
   i = elf_hash((const char *)key, sz) % FONTHASH_SIZE;
   node-> next = fontHash. buckets[ i];
   fontHash. buckets[ i] = node;
   if ( addSizeEntry) {
      j = elf_hash_by_size( key) % FONTHASH_SIZE;
      node-> next2 = fontHashBySize. buckets[ j];
      fontHashBySize. buckets[ j] = node;
   }
   return true;
}

Bool
get_font_from_hash( PFont font, int *vectored, Bool bySize)
{
   PFontHashNode node = find_node( font, bySize);
   if ( node == nil) return false;
   *font = node-> value. font;
   *vectored = node-> value. vectored;
   return true;
}

Bool
create_font_hash( void)
{
   memset ( &fontHash, 0, sizeof( FontHash));
   memset ( &fontHashBySize, 0, sizeof( FontHash));
   return true;
}

Bool
destroy_font_hash( void)
{
   PFontHashNode node, node2;
   int i;
   for ( i = 0; i < FONTHASH_SIZE; i++)
   {
      node = fontHash. buckets[ i];
      while ( node)
      {
         node2 = node;
         node = node-> next;
         free( node2);
      }
   }
   create_font_hash();     // just in case...
   return true;
}


static Bool _ft_cleaner( PDCFont f, int keyLen, void * key, void * dummy) {
   if ( f-> refcnt <= 0) font_free( f, true);
   return false;
}


PDCFont
font_alloc( Font * data, Point * resolution)
{
   PDCFont ret = hash_fetch( fontMan, data, FONTSTRUCSIZE + strlen( data-> name));
   if ( ret == nil) {
      LOGFONT logfont;
      PFont   f;

      if ( IS_WIN95 && ( hash_count( fontMan) > 128))
         font_clean();

      ret = malloc( sizeof( DCFont));
      memcpy( f = &ret-> font, data, sizeof( Font));
      ret-> refcnt = 0;
      font_font2logfont( f, &logfont);
      if ( !( ret-> hfont  = CreateFontIndirect( &logfont))) {
         LOGFONT lf;
         apiErr;
         memset( &lf, 0, sizeof( lf));
         ret-> hfont = CreateFontIndirect( &lf);
      }
      hash_store( fontMan, &ret-> font, FONTSTRUCSIZE + strlen( ret-> font. name), ret);
   }
   ret-> refcnt++;
   return ret;
}

void
font_free( PDCFont res, Bool permanent)
{
   if ( !res || --res-> refcnt > 0) return;
   if ( !permanent) {
      res-> refcnt = 0;
      return;
   }
   if ( res-> hfont) {
      DeleteObject( res-> hfont);
      res-> hfont = nil;
   }
   hash_delete( fontMan, &res-> font, FONTSTRUCSIZE + strlen( res-> font. name), true);
}

void
font_change( Handle self, Font * font)
{
   PDCFont p;
   PDCFont newP;
   if ( is_apt( aptDCChangeLock)) return;
   p    = sys fontResource;
   newP = ( sys fontResource = font_alloc( font, &sys res));
   font_free( p, false);
   if ( sys ps)
       SelectObject( sys ps, newP-> hfont);
}

void
font_clean()
{
    hash_first_that( fontMan, _ft_cleaner, nil, nil, nil);
}


#define MASK_CODEPAGE  0x00FF
#define MASK_FAMILY    0xFF00
#define FF_MASK        0x00F0

void
font_logfont2font( LOGFONT * lf, Font * f, Point * res)
{
   TEXTMETRIC tm;
   HDC dc = dc_alloc();
   HFONT hf = SelectObject( dc, CreateFontIndirect( lf));

   GetTextMetrics( dc, &tm);
   DeleteObject( SelectObject( dc, hf));
   dc_free();

   if ( !res) res = &guts. displayResolution;
   f-> height              = tm. tmHeight;
   f-> size                = ( f-> height - tm. tmInternalLeading) * 72.0 / res-> y + 0.5;
   f-> width               = lf-> lfWidth;
   f-> direction           = lf-> lfEscapement;
   f-> style               = 0 |
      ( lf-> lfItalic     ? fsItalic     : 0) |
      ( lf-> lfUnderline  ? fsUnderlined : 0) |
      ( lf-> lfStrikeOut  ? fsStruckOut  : 0) |
     (( lf-> lfWeight >= 700) ? fsBold   : 0);
   f-> pitch               = ((( lf-> lfPitchAndFamily & 3) == DEFAULT_PITCH) ? fpDefault :
      ((( lf-> lfPitchAndFamily & 3) == VARIABLE_PITCH) ? fpVariable : fpFixed));
   strncpy( f-> name, lf-> lfFaceName, LF_FACESIZE);
   f-> codepage            = lf-> lfCharSet | ((lf-> lfPitchAndFamily & FF_MASK) << 8);
   f-> name[ LF_FACESIZE] = 0;
}

void
font_font2logfont( Font * f, LOGFONT * lf)
{
   lf-> lfHeight           = f-> height;
   lf-> lfWidth            = f-> width;
   lf-> lfEscapement       = f-> direction;
   lf-> lfOrientation      = f-> direction;
   lf-> lfWeight           = ( f-> style & fsBold)       ? 800 : 400;
   lf-> lfItalic           = ( f-> style & fsItalic)     ? 1 : 0;
   lf-> lfUnderline        = ( f-> style & fsUnderlined) ? 1 : 0;
   lf-> lfStrikeOut        = ( f-> style & fsStruckOut)  ? 1 : 0;
   lf-> lfCharSet          = f-> codepage & MASK_CODEPAGE;
   lf-> lfOutPrecision     = OUT_TT_PRECIS;
   lf-> lfClipPrecision    = CLIP_DEFAULT_PRECIS;
   lf-> lfQuality          = PROOF_QUALITY;

   lf-> lfPitchAndFamily   = (( f-> codepage & MASK_FAMILY) >> 8) |
      (( f-> pitch == fpDefault)  ? DEFAULT_PITCH :
      (( f-> pitch == fpVariable) ? VARIABLE_PITCH : FIXED_PITCH));
   strncpy( lf-> lfFaceName, f-> name, LF_FACESIZE);
}

void
font_textmetric2font( TEXTMETRIC * tm, Font * fm, Bool readonly)
{
   if ( !readonly) {
      fm-> size                   = ( tm-> tmHeight - tm-> tmInternalLeading) * 72.0 / guts. displayResolution.y + 0.5;
      fm-> width                  = tm-> tmAveCharWidth;
      fm-> height                 = tm-> tmHeight;
      fm-> style                  = 0 |
                                 ( tm-> tmItalic     ? fsItalic     : 0) |
                                 ( tm-> tmUnderlined ? fsUnderlined : 0) |
                                 ( tm-> tmStruckOut  ? fsStruckOut  : 0) |
                                 (( tm-> tmWeight >= 700) ? fsBold   : 0);
   }
   fm-> pitch                  =
      (( tm-> tmPitchAndFamily & TMPF_FIXED_PITCH)               ? fpVariable : fpFixed);
   fm-> vector                 =
      (( tm-> tmPitchAndFamily & ( TMPF_VECTOR | TMPF_TRUETYPE)) ? true       : false   );
   fm-> weight                 = tm-> tmWeight / 100;
   fm-> ascent                 = tm-> tmAscent;
   fm-> descent                = tm-> tmDescent;
   fm-> maximalWidth           = tm-> tmMaxCharWidth;
   fm-> internalLeading        = tm-> tmInternalLeading;
   fm-> externalLeading        = tm-> tmExternalLeading;
   fm-> xDeviceRes             = tm-> tmDigitizedAspectX;
   fm-> yDeviceRes             = tm-> tmDigitizedAspectY;
   fm-> firstChar              = tm-> tmFirstChar;
   fm-> lastChar               = tm-> tmLastChar;
   fm-> breakChar              = tm-> tmBreakChar;
   fm-> defaultChar            = tm-> tmDefaultChar;
   fm-> codepage               = tm-> tmCharSet | ((tm-> tmPitchAndFamily & FF_MASK) << 8);
}


void
font_pp2font( char * presParam, Font * f)
{
   int i;
   char * p = strchr( presParam, '.');

   memset( f, 0, sizeof( Font));
   if ( p) {
      f-> size = atoi( presParam);
      p++;
   } else
      f-> size = 10;

   strncpy( f-> name, p, 256);
   p = f-> name;
   for ( i = strlen( p) - 1; i >= 0; i--)  {
      if ( p[ i] == '.')  {
         if ( stricmp( "Italic",    &p[ i + 1]) == 0) f-> style |= fsItalic;
         if ( stricmp( "Bold",      &p[ i + 1]) == 0) f-> style |= fsBold;
         if ( stricmp( "Underscore",&p[ i + 1]) == 0) f-> style |= fsUnderlined;
         if ( stricmp( "StrikeOut", &p[ i + 1]) == 0) f-> style |= fsStruckOut;
         p[ i] = 0;
      }
   }
   f-> width = f-> height = C_NUMERIC_UNDEF;
}

PFont
apc_font_default( PFont font)
{
   *font = guts. windowFont;
   font-> pitch = fpDefault;
   return font;
}

int
apc_font_load( const char* filename)
{
   return 0;
}

#define fgBitmap 0
#define fgVector 1

static Bool recursiveFF = 0;

typedef struct _FEnumStruc
{
   int           count;
   int           resValue;
   int           heiValue;
   int           widValue;
   int           fType;
   Bool          useWidth;
   Bool          useVector;
   Bool          usePitch;
   Bool          forceSize;
   Bool          vecId;
   Bool          matchILead;
   PFont         font;
   TEXTMETRIC    tm;
   LOGFONT       lf;
   Point         res;
   char          name[ LF_FACESIZE];
   char          family[ LF_FULLFACESIZE];
} FEnumStruc, *PFEnumStruc;

int CALLBACK
fep( ENUMLOGFONT FAR *e, NEWTEXTMETRIC FAR *t, int type, PFEnumStruc es)
{
   Font * font = es-> font;
   long hei, res;

#define do_copy                                                         \
   es-> count++;                                                        \
   es-> fType = type;                                                   \
   memcpy( &es-> tm, t, sizeof( TEXTMETRIC));                           \
   memcpy( es-> family, e-> elfFullName, LF_FULLFACESIZE);          \
   memcpy( &es-> lf,    &e-> elfLogFont, sizeof( LOGFONT));             \
   memcpy( es-> name,   e-> elfLogFont. lfFaceName, LF_FACESIZE)

   if ( es-> usePitch)
   {
      int fpitch = ( t-> tmPitchAndFamily & TMPF_FIXED_PITCH) ? fpVariable : fpFixed;
      if ( fpitch != font-> pitch)
         return 1; // so this font cannot be selected due pitch pickup failure
   }

   if ( type & TRUETYPE_FONTTYPE) {
      do_copy;
      es-> vecId = 1;
      return 0; // enough; no further enumeration requred, since Win renders TrueType quite good
                // to not mix these with raster fonts.
   }
   if ( type == 0) {
      do_copy;
      es-> vecId = 1;
      return 1; // it's vector font; but keep enumeration in case we'll get better match
   }

   // bitmapped fonts:
   // determining best match for primary measure - size or height
   if ( es-> forceSize) {
      long xs  = ( t-> tmHeight - ( es-> matchILead ? t-> tmInternalLeading : 0))
                 * 72.0 / es-> res. y + 0.5;
      hei = ( xs - font-> size) * ( xs - font-> size);
   } else {
      long dv = t-> tmHeight - font-> height - ( es-> matchILead ? 0 : t-> tmInternalLeading);
      hei = dv * dv;
   }

   // resolution difference
   res = ( t-> tmDigitizedAspectY - es-> res. y) * ( t-> tmDigitizedAspectY - es-> res. y);

   if ( hei < es-> heiValue) {
      // current height is closer than before...
      es-> heiValue = hei;
      es-> resValue = res;
      if ( es-> useWidth)
         es-> widValue = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
      do_copy;
   } else if ( es-> useWidth && ( hei == es-> heiValue)) {
      // height is same close, but width is requested and close that before
      long wid = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
      if ( wid < es-> widValue) {
         es-> widValue = wid;
         es-> resValue = res;
         do_copy;
      } else if (( wid == es-> widValue) && ( res < es-> resValue)) {
         // height and width are same close, but resolution is closer
         es-> resValue = res;
         do_copy;
      }
   } else if (( res < es-> resValue) && ( hei == es-> heiValue)) {
      // height is same close, but resolution is closer
      es-> resValue = res;
      do_copy;
   }
   return 1;
#undef do_copy
}

extern int font_font2gp( PFont font, Point res, Bool forceSize, HDC dc);

static void
font_logfont2textmetric( HDC dc, LOGFONT * lf, TEXTMETRIC * tm)
{
   HFONT hf = SelectObject( dc, CreateFontIndirect( lf));
   GetTextMetrics( dc, tm);
   DeleteObject( SelectObject( dc, hf));
}

int
font_font2gp_internal( PFont font, Point res, Bool forceSize, HDC theDC)
{
#define out( retVal)  if ( !theDC) dc_free(); return retVal
   FEnumStruc es;
   HDC  dc            = theDC ? theDC : dc_alloc();
   Bool useNameSubplacing = false;

   es. count          = es. vecId = 0;
   es. resValue       = es. heiValue = es. widValue = INT_MAX;
   es. useWidth       = font-> width != 0;
   es. useVector      = font-> direction != 0;
   es. usePitch       = font-> pitch != fpDefault;
   es. res            = res;
   es. forceSize      = forceSize;
   es. font           = font;
   es. matchILead     = forceSize ? ( font-> size >= 0) : ( font-> height >= 0);

   useNameSubplacing = es. usePitch;
   if ( font-> height < 0) font-> height *= -1;
   if ( font-> size   < 0) font-> size   *= -1;

   EnumFontFamilies( dc, font-> name, ( FONTENUMPROC) fep, ( LPARAM) &es);

   // checking matched font, if available
   if ( es. count > 0) {
      if ( es. useVector) {
         if ( !es. vecId) useNameSubplacing = 1; // try other font if bitmap wouldn't fit
         es. resValue = es. heiValue = INT_MAX;  // cancel bitmap font selection
      }

      // special synthesized GDI font case
      if (
             es. useWidth && ( es. widValue > 0) &&
           ( es. heiValue == 0) &&
           ( es. fType & RASTER_FONTTYPE) &&
           ( font-> style & fsBold)
         ) {
            int r;
            Font xf = *font;
            xf. width--;
            xf. style = 0; // any style could affect tmOverhang
            r = font_font2gp( &xf, res, forceSize, dc);
            if (( r == fgBitmap) && ( xf. width < font-> width)) {
               LOGFONT lpf;
               TEXTMETRIC tm;
               font_font2logfont( &xf, &lpf);
               lpf. lfWeight = 700;
               font_logfont2textmetric( dc, &lpf, &tm);
               if ( xf. width + tm. tmOverhang == font-> width) {
                  font_textmetric2font( &tm, font, true);
                  font-> direction = 0;
                  font-> size   = xf. size;
                  font-> height = xf. height;
                  font-> maximalWidth += tm. tmOverhang;
                  out( fgBitmap);
               }
            }
         }

      // have resolution ok? so using raster font mtx
      if (
           ( es. heiValue < 2) &&
           ( es. fType & RASTER_FONTTYPE) &&
           ( !es. useWidth  || (( es. widValue == 0) && ( es. heiValue == 0)))
         )
      {
         font-> height   = es. tm. tmHeight;
         // if synthesized embolding added, we have to reflect that...
         // ( 'cos it increments B-extent of a char cell).
         if ( font-> style & fsBold) {
            LOGFONT lpf = es. lf;
            TEXTMETRIC tm;
            lpf. lfWeight = 700; // ignore italics, it changes tmOverhang also
            font_logfont2textmetric( dc, &lpf, &tm);
            es. tm. tmMaxCharWidth += tm. tmOverhang;
            es. lf. lfWidth        += tm. tmOverhang;
         }
         font_textmetric2font( &es. tm, font, true);
         font-> direction = 0;
         strncpy( font-> family, es. family, LF_FULLFACESIZE);
         font-> size     = ( es. tm. tmHeight - es. tm. tmInternalLeading) * 72.0 / res.y + 0.5;
         font-> width    = es. lf. lfWidth;
         font-> codepage = es. tm.tmCharSet | ((es.tm.tmPitchAndFamily & FF_MASK) << 8);
         out( fgBitmap);
      }

      // or vector font - for any purpose?
      // if so, it could guaranteed that font-> height == tmHeight
      if ( es. vecId) {
         LOGFONT lpf = es. lf;
         TEXTMETRIC tm;

         // since proportional computation of small items as InternalLeading
         // gives incorrect result, querying the only valid source - GetTextMetrics
         if ( forceSize) {
            lpf. lfHeight = font-> size * res. y / 72.0 + 0.5;
            if ( es. matchILead) lpf. lfHeight *= -1;
         } else
            lpf. lfHeight = es. matchILead ? font-> height : -font-> height;

         lpf. lfWidth = es. useWidth ? font-> width : 0;

         font_logfont2textmetric( dc, &lpf, &tm);
         if ( forceSize)
            font-> height = tm. tmHeight;
         else
            font-> size = ( tm. tmHeight - tm. tmInternalLeading) * 72.0 / res. y + 0.5;

         if ( !es. useWidth)
            font-> width = tm. tmAveCharWidth;

         font_textmetric2font( &tm, font, true);
         strncpy( font-> family, es. family, LF_FULLFACESIZE);
         font-> codepage = tm. tmCharSet | ((tm. tmPitchAndFamily & FF_MASK) << 8);
         out( fgVector);
      }
   }

   // if strict match not found, use subplacing
   if ( useNameSubplacing)
   {
       int ret;
       int ogp  = font-> pitch;
       // setting some other( maybe other) font name
       strcpy( font-> name, font-> pitch == fpFixed ?
          guts. defaultFixedFont : guts. defaultVariableFont);
       font-> pitch = fpDefault;
       ret = font_font2gp( font, res, forceSize, dc);
       // if that alternative match succeeded with name subplaced again, skip
       // that result and use DEFAULT_SYSTEM_FONT match
       if (( ogp == fpFixed) && ( strcmp( font-> name, guts. defaultFixedFont) != 0)) {
          strcpy( font-> name, DEFAULT_SYSTEM_FONT);
          font-> pitch = fpDefault;
          ret = font_font2gp( font, res, forceSize, dc);
       }
       out( ret);
   }

   // font not found, so use general representation for height and width
   strcpy( font-> name, guts. defaultSystemFont);
   if ( recursiveFF == 0)
   {
      // trying to catch default font with correct values
      int r;
      recursiveFF++;
      r = font_font2gp( font, res, forceSize, dc);
      recursiveFF--;
      out( r);
   } else {
      int r;
      // if not succeeded, to avoid recursive call use "wild guess".
      // This could be achieved if system does not have "System" font
      *font = guts. windowFont;
      font-> pitch = fpDefault;
      recursiveFF++;
      r = ( recursiveFF < 3) ? font_font2gp( font, res, forceSize, dc) : fgBitmap;
      recursiveFF--;
      out( r);
   }
   return fgBitmap;
#undef out
}

static int
font_font2gp( PFont font, Point res, Bool forceSize, HDC dc)
{
   int vectored;
   Font key;
   Bool addSizeEntry;

   font-> resolution = res. y * 0x10000 + res. x;
   if ( get_font_from_hash( font, &vectored, forceSize))
      return vectored;
   memcpy( &key, font, sizeof( Font));
   vectored = font_font2gp_internal( font, res, forceSize, dc);
   font-> resolution = res. y * 0x10000 + res. x;
   if ( forceSize) {
      key. height = font-> height;
      key. width  = font-> width;
      addSizeEntry = true;
   } else {
      key. size  = font-> size;
      addSizeEntry = vectored ? (( key. height == key. width)  || ( key. width == 0)) : true;
   }
   add_font_to_hash( &key, font, vectored, addSizeEntry);
   return vectored;
}

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
   if ( self) objCheck false;
   font_font2gp( dest, self ? sys res : guts. displayResolution,
      Drawable_font_add( self, source, dest), self ? sys ps : 0);
   return true;
}

int CALLBACK
fep2( ENUMLOGFONT FAR *e, NEWTEXTMETRIC FAR *t, int type, PList lst)
{
   PFont fm = malloc( sizeof( Font));
   font_textmetric2font(( TEXTMETRIC *) t, fm, false);
   fm-> direction = fm-> resolution = 0;
   strncpy( fm-> name,     e-> elfLogFont. lfFaceName, LF_FACESIZE);
   strncpy( fm-> family,   e-> elfFullName,            LF_FULLFACESIZE);
   list_add( lst, ( Handle) fm);
   return 1;
}

PFont
apc_fonts( Handle self, const char* facename, int * retCount)
{
   PFont fmtx = nil;
   int  i;
   HDC  dc;
   List lst;
   Bool hasdc = 0;
   apcErrClear;

   *retCount = 0;
   if ( self == nilHandle || self == application)
      dc = dc_alloc();
   else if ( kind_of( self, CPrinter)) {
      if ( !is_opt( optInDraw) && !is_opt( optInDrawInfo)) {
         hasdc = 1;
         CPrinter( self)-> begin_paint_info( self);
      }
      dc = sys ps;
   } else
      return nil;

   list_create( &lst, 256, 256);
   EnumFontFamilies( dc, facename, ( FONTENUMPROC) fep2, ( LPARAM) &lst);

   if ( self == nilHandle || self == application)
      dc_free();
   else if ( hasdc)
      CPrinter( self)-> end_paint_info( self);

   if ( lst. count == 0) goto Nothing;
   *retCount = lst. count;
   fmtx = malloc( *retCount * sizeof( Font));
   for ( i = 0; i < lst. count; i++)
      memcpy( &fmtx[ i], ( void *) lst. items[ i], sizeof( Font));
   list_delete_all( &lst, true);
Nothing:
   list_destroy( &lst);
   return fmtx;
}

// Font end
// Colors section


#define stdDisabled  COLOR_GRAYTEXT        ,  COLOR_WINDOW
#define stdHilite    COLOR_HIGHLIGHTTEXT   ,  COLOR_HIGHLIGHT
#define std3d        COLOR_BTNHIGHLIGHT    ,  COLOR_BTNSHADOW

static int buttonScheme[] = {
   COLOR_BTNTEXT,   COLOR_BTNFACE,
   COLOR_BTNTEXT,   COLOR_BTNFACE,
   COLOR_GRAYTEXT,  COLOR_BTNFACE,
   std3d
};
static int sliderScheme[] = {
   COLOR_WINDOWTEXT,       COLOR_SCROLLBAR,
   COLOR_WINDOWTEXT,       COLOR_SCROLLBAR,
   stdDisabled,
   std3d
};

static int dialogScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   COLOR_CAPTIONTEXT, COLOR_ACTIVECAPTION,
   COLOR_INACTIVECAPTIONTEXT, COLOR_INACTIVECAPTION,
   std3d
};
static int staticScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   stdDisabled,
   std3d
};
static int editScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   stdHilite,
   stdDisabled,
   std3d
};
static int menuScheme[] = {
   COLOR_MENUTEXT, COLOR_MENU,
   stdHilite,
   stdDisabled,
   std3d
};

static int scrollScheme[] = {
   COLOR_WINDOWTEXT,    COLOR_SCROLLBAR,
   stdHilite,
   stdDisabled,
   std3d
};

static int windowScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   COLOR_CAPTIONTEXT, COLOR_ACTIVECAPTION,
   COLOR_INACTIVECAPTIONTEXT, COLOR_INACTIVECAPTION,
   std3d
};
static int customScheme[] = {
   COLOR_WINDOWTEXT, COLOR_WINDOW,
   stdHilite,
   stdDisabled,
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
remap_color( long clr, Bool toSystem)
{
   PRGBColor cp = ( PRGBColor) &clr;
   unsigned char sw = cp-> r;
   if ( toSystem && clr < 0) {
      long c = clr;
      int * scheme = ( int *) ctx_remap_def( clr & wcMask, ctx_wc2SCHEME, true, ( int) &customScheme);
      if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
      if ( clr == 0) return 0xFFFFFF; // clInvalid
      c = GetSysColor( scheme[ clr - 1]);
      return c;
   }
   cp-> r = cp-> b;
   cp-> b = sw;
   return clr;
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

// Colors end
// Miscellaneous

static HDC cachedScreenDC = nil;
static int cachedScreenRC = 0;
static HDC cachedCompatDC = nil;
static int cachedCompatRC = 0;


HDC dc_alloc()
{
   if ( cachedScreenRC++ == 0)
      cachedScreenDC = GetDC( 0);
   return cachedScreenDC;
}

void dc_free()
{
   if ( --cachedScreenRC <= 0)
      ReleaseDC( 0, cachedScreenDC);
   if ( cachedScreenRC < 0)
      cachedScreenRC = 0;
}

HDC dc_compat_alloc( HDC compatDC)
{
   if ( cachedCompatRC++ == 0)
      cachedCompatDC = CreateCompatibleDC( compatDC);
   return cachedCompatDC;
}

void dc_compat_free()
{
   if ( --cachedCompatRC <= 0)
      DeleteDC( cachedCompatDC);
   if ( cachedCompatRC < 0)
      cachedCompatRC = 0;
}

void
hwnd_enter_paint( Handle self)
{
   GetObject( sys stockPen   = GetCurrentObject( sys ps, OBJ_PEN),
      sizeof( LOGPEN), &sys stylus. pen);
   GetObject( sys stockBrush = GetCurrentObject( sys ps, OBJ_BRUSH),
      sizeof( LOGBRUSH), &sys stylus. brush);
   sys stockFont      = GetCurrentObject( sys ps, OBJ_FONT);
   if ( !sys stockPalette)
      sys stockPalette = GetCurrentObject( sys ps, OBJ_PAL);
   font_free( sys fontResource, false);
   sys stylusResource = nil;
   sys fontResource   = nil;
   sys stylusFlags    = 0;
   sys stylus. extPen. actual = false;
   apt_set( aptDCChangeLock);
   sys bpp = GetDeviceCaps( sys ps, BITSPIXEL);
   apc_gp_set_color( self, sys viewColors[ ciFore]);
   apc_gp_set_back_color( self, sys viewColors[ ciBack]);

   if ( sys psd == nil) sys psd = malloc( sizeof( PaintSaveData));
   apc_gp_set_text_opaque( self, is_apt( aptTextOpaque));
   apc_gp_set_text_out_baseline( self, is_apt( aptTextOutBaseline));
   apc_gp_set_line_width( self, sys lineWidth);
   apc_gp_set_line_end( self, sys lineEnd);
   apc_gp_set_line_pattern( self,
       ( sys linePatternLen > 3) ? sys linePattern : ( char*)&sys linePattern,
       sys linePatternLen);
   apc_gp_set_rop( self, sys rop);
   apc_gp_set_rop2( self, sys rop2);
   apc_gp_set_transform( self, sys transform. x, sys transform. y);
   apc_gp_set_fill_pattern( self, sys fillPattern2);
   sys psd-> font           = var font;
   sys psd-> lineWidth      = sys lineWidth;
   sys psd-> lineEnd        = sys lineEnd;
   sys psd-> linePattern    = sys linePattern;
   sys psd-> linePatternLen = sys linePatternLen;
   sys psd-> rop            = sys rop;
   sys psd-> rop2           = sys rop2;
   sys psd-> transform      = sys transform;
   sys psd-> textOpaque     = is_apt( aptTextOpaque);
   sys psd-> textOutB       = is_apt( aptTextOutBaseline);

   apt_clear( aptDCChangeLock);
   stylus_change( self);
   apc_gp_set_font( self, &var font);
   var font. resolution = sys res. y * 0x10000 + sys res. x;
   SetStretchBltMode( sys ps, COLORONCOLOR);
}

void
hwnd_leave_paint( Handle self)
{
   SelectObject( sys ps,  sys stockPen);
   SelectObject( sys ps,  sys stockBrush);
   SelectObject( sys ps,  sys stockFont);
   SelectPalette( sys ps, sys stockPalette, 0);
   sys stockPen = sys stockBrush = sys stockFont = sys stockPalette = nilHandle;
   stylus_free( sys stylusResource, false);
   if ( sys psd != nil) {
      var font           = sys psd-> font;
      sys lineWidth      = sys psd-> lineWidth;
      sys lineEnd        = sys psd-> lineEnd;
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
   free( sys charTable);
   free( sys charTable2);
   (void*)sys charTable = (void*)sys charTable2 = nil;
   sys bpp = 0;
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


typedef struct _ModData
{
   BYTE  ks  [ 256];
   BYTE  kss [ 3];
   BYTE *gks;
} ModData;


BYTE *
mod_select( int mod)
{
   ModData * ks;

   ks = malloc( sizeof( ModData));
   GetKeyboardState( ks-> ks);
   ks-> kss[ 0]   = ks-> ks[ VK_MENU];
   ks-> kss[ 1]   = ks-> ks[ VK_CONTROL];
   ks-> kss[ 2]   = ks-> ks[ VK_SHIFT];
   ks-> ks[ VK_MENU   ] = ( mod & kmAlt  ) ? 0x80 : 0;
   ks-> ks[ VK_CONTROL] = ( mod & kmCtrl ) ? 0x80 : 0;
   ks-> ks[ VK_SHIFT  ] = ( mod & kmShift) ? 0x80 : 0;
   SetKeyboardState( ks-> ks);
   ks-> gks = guts. currentKeyState;
   guts. currentKeyState = ks-> ks;
   return ( BYTE*) ks;
}

void
mod_free( BYTE * modState)
{
   ModData * ks = ( ModData*) modState;
   ks-> ks[ VK_MENU   ] = ks-> kss[ 0];
   ks-> ks[ VK_CONTROL] = ks-> kss[ 1];
   ks-> ks[ VK_SHIFT  ] = ks-> kss[ 2];
   SetKeyboardState( ks-> ks);
   guts. currentKeyState = ks-> gks;
   free( ks);
}

typedef struct _SzList
{
   List l;
   int  sz;
   PRGBColor p;
} SzList, *PSzList;

static Bool
pal_count( Handle window, Handle self, PSzList l)
{
   if ( !is_apt( aptClipOwner) && ( window != application))
      return false;
   if ( var palSize > 0) {
      list_add( &l->l, self);
      l->sz += var palSize;
   }
   if ( var widgets. count > 0)
      CWidget( self)-> first_that( self, pal_count, l);
   return false;
}

static Bool
pal_collect( Handle self, PSzList l)
{
   memcpy( l-> p, var palette, var palSize * sizeof( RGBColor));
   l-> p  += var palSize;
   return false;
}

static Bool
repaint_all( Handle owner, Handle self, void * dummy)
{
   objCheck false;
   if ( !is_apt( aptTransparent)) {
      if ( !InvalidateRect(( HWND) var handle, nil, false)) apiErr;
      if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
      objCheck false;
      var self-> first_that( self, repaint_all, nil);
   }
   process_transparents( self);
   return false;
}

void
hwnd_repaint( Handle self)
{
   objCheck;
   if ( !InvalidateRect (( HWND) var handle, NULL, false)) apiErr;
   if ( is_apt( aptSyncPaint) && !UpdateWindow(( HWND) var handle)) apiErr;
   objCheck;
   var self-> first_that( self, repaint_all, nil);
   process_transparents( self);
}

Bool
palette_change( Handle self)
{
   SzList l;
   PRGBColor p;
   PRGBColor d;
   int nColors = ( 1 << (
       guts. displayBMInfo. bmiHeader. biBitCount *
       guts. displayBMInfo. bmiHeader. biPlanes
   )) & 0x1FF;
   int i;
   HPALETTE pal;
   XLOGPALETTE xlp = {0x300};
   HDC dc;
   int rCol = 0;

   if ( nColors == 0)
      return false;

   self = hwnd_frame_top_level( self);
   if ( self == nilHandle) return false;

   list_create( &l.l, 32, 32);
   l. sz = 0;
   if ( var palSize > 0) {
      list_add( &l.l, self);
      l.sz += var palSize;
   }
   if ( var widgets. count > 0)
      CWidget( self)-> first_that( self, pal_count, &l);

   if ( l. l. count == 0) {
      list_destroy( &l.l);
      return false;
   }


   xlp. palNumEntries = l. sz;
   p = malloc( sizeof( RGBColor) * l. sz);
   d = malloc( sizeof( RGBColor) * nColors);

   l. p = p;
   list_first_that( &l.l, pal_collect, &l);
   cm_squeeze_palette( p, xlp. palNumEntries, d, nColors);
   xlp. palNumEntries = nColors;

   for ( i = 0; i < nColors; i++) {
      xlp. palPalEntry[ i]. peRed    = d[ i]. r;
      xlp. palPalEntry[ i]. peGreen  = d[ i]. g;
      xlp. palPalEntry[ i]. peBlue   = d[ i]. b;
      xlp. palPalEntry[ i]. peFlags  = 0;
   }

   free( d);
   free( p);

   pal = CreatePalette(( LOGPALETTE *) &xlp);

   dc  = GetDC( HANDLE);
   pal  = SelectPalette( dc, pal, 0);
   rCol = RealizePalette( dc);
   DeleteObject( SelectPalette( dc, pal, 0));
   ReleaseDC( HANDLE, dc);

   if ( rCol > 0) hwnd_repaint( self);

   list_destroy( &l.l);
   return true;
}

int
palette_match_color( XLOGPALETTE * lp, long clr, int * diffFactor)
{
   int diff = 0x10000, cdiff = 0, ret = 0, nCol = lp-> palNumEntries;
   RGBColor color;

   if ( nCol == 0) {
      if ( diffFactor) *diffFactor = 0;
      return clr;
   }

   color. r = clr & 0xFF;
   color. g = ( clr >> 8)  & 0xFF;
   color. b = ( clr >> 16) & 0xFF;

   while( nCol--)
   {
      int dr=abs((int)color. r - (int)lp-> palPalEntry[ nCol]. peRed),
          dg=abs((int)color. g - (int)lp-> palPalEntry[ nCol]. peGreen),
          db=abs((int)color. b - (int)lp-> palPalEntry[ nCol]. peBlue);
      cdiff=dr*dr+dg*dg+db*db;
      if ( cdiff < diff) {
         ret  = nCol;
         diff = cdiff;
         if ( cdiff == 0) break;
      }
   }

   if ( diffFactor) *diffFactor = cdiff;
   return ret;
}


long
palette_match( Handle self, long clr)
{
   XLOGPALETTE lp;
   int cdiff;
   RGBColor color;

   lp. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lp. palPalEntry);

   if ( lp. palNumEntries == 0) {
      apiErr;
      return clr;
   }

   color. r = clr & 0xFF;
   color. g = ( clr >> 8)  & 0xFF;
   color. b = ( clr >> 16) & 0xFF;

   palette_match_color( &lp, clr, &cdiff);

   if ( cdiff >= COLOR_TOLERANCE)
      return clr;

   lp. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lp. palPalEntry);

   palette_match_color( &lp, clr, &cdiff);

   if ( cdiff >= COLOR_TOLERANCE)
      return clr;

   return PALETTERGB( color.r, color.g, color.b);
}


HRGN
region_create( Handle mask)
{
   LONG i, w, h, x, y, size = 256;
   HRGN    rgn = NULL;
   Byte    * idata;
   RGNDATA * rdata = nil;
   RECT    * current;
   Bool      set = 0;

   if ( !mask)
      return NULL;

   dobjCheck( mask) NULL;

   w = PImage( mask)-> w;
   h = PImage( mask)-> h;
   if ( dsys( mask) s. imgCachedRegion) {
      rgn = CreateRectRgn(0,0,0,0);
      CombineRgn( rgn, dsys( mask) s. imgCachedRegion, nil, RGN_COPY);
      return rgn;
   }

   idata  = PImage( mask)-> data + PImage( mask)-> dataSize - PImage( mask)-> lineSize;

   rdata = ( RGNDATA*) malloc( sizeof( RGNDATAHEADER) + size * sizeof( RECT));
   rdata-> rdh. nCount = 0;
   current = ( RECT * ) &( rdata-> Buffer);
   current--;

   for ( y = 0; y < h; y++) {
      for ( x = 0; x < w; x++) {
         if ( idata[ x >> 3] == 0) {
            x += 7;
            continue;
         }
         if ( idata[ x >> 3] & ( 1 << ( 7 - ( x & 7)))) {
            if ( set && current-> top == y && current-> right == x)
               current-> right++;
            else {
               set = 1;
               if ( rdata-> rdh. nCount >= size) {
                  rdata = realloc( rdata, sizeof( RGNDATAHEADER) + ( size *= 3) * sizeof( RECT));
                  current = ( RECT * ) &( rdata-> Buffer);
                  current += rdata-> rdh. nCount - 1;
               }
               rdata-> rdh. nCount++;
               current++;
               current-> left   = x;
               current-> top    = y;
               current-> right  = x + 1;
               current-> bottom = y + 1;
            }
         }
      }
      idata -= PImage( mask)-> lineSize;
   }

   if ( set) {
      rdata-> rdh. dwSize          = sizeof( RGNDATAHEADER);
      rdata-> rdh. iType           = RDH_RECTANGLES;
      rdata-> rdh. nRgnSize        = rdata-> rdh. nCount * sizeof( RECT);
      rdata-> rdh. rcBound. left   = 0;
      rdata-> rdh. rcBound. top    = 0;
      rdata-> rdh. rcBound. right  = h;
      rdata-> rdh. rcBound. bottom = w;

      if ( !( rgn = ExtCreateRegion( NULL,
         sizeof( RGNDATAHEADER) + ( rdata-> rdh. nCount * sizeof( RECT)), rdata))) {
         apcErr( 900);
      }

      dsys( mask) s. imgCachedRegion = CreateRectRgn(0,0,0,0);
      CombineRgn( dsys( mask) s. imgCachedRegion, rgn, nil, RGN_COPY);
   }
   free( rdata);

   return rgn;
}
