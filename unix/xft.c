/*
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

/*********************************/
/*                               */
/*  Xft - client-side X11 fonts  */
/*                               */
/*********************************/


/*

   USAGE
   -----
   To use specific Xft fonts, set Prima font names in your X resource 
   database in fontconfig formats - for example, Palatino-12. Consult
   'man fontconfig' for the syntax, but be aware that Prima uses only
   size, weight, and name fields.

   IMPLEMENTATION NOTES
   --------------------

   This implementations doesn't work with non-scalable Xft fonts,
   since their rasterization capabilities are currently ( June 2003) very limited - 
   no scaling and no rotation. Plus, the selection of the non-scalable fonts is
   a one big something, and I believe that one in apc_font.c is enough.

   The following Xft/fontconfig problems, if fixed in th next versions, need to be 
   taken into the consideration:
     - no font/glyph data - internal leading, underscore size/position,
       no strikeout size/position, average width.
     - no raster operations
     - no glyph reference point shift
     - no client-side access to glyph bitmaps
     - no on-the-fly antialiasing toggle
     - no text background painting ( only rectangles )
     - no text strikeout / underline drawing routines
     - produces xlib failures for large polygons - answered to be a Xrender bug;
       the X error handler catches this now.

   TO DO
   -----
    - The full set of raster operations - not supported by xft ( stupid )
    - apc_show_message - probably never will be implemented though
    - Investigate if ICONV can be replaced by native perl's ENCODE interface
    - Under some circumstances Xft decides to put antialiasing aside, for
      example, on the paletted displays. Check, if some heuristics that would
      govern whether Xft is to be used there, are needed.
       
 */

#include "unix/guts.h"

#ifdef USE_XFT

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

/* fontconfig version < 2.2.0 */
#ifndef FC_WEIGHT_NORMAL
#define FC_WEIGHT_NORMAL 80
#endif
#ifndef FC_WEIGHT_THIN
#define FC_WEIGHT_THIN 0
#endif
#ifndef FC_WIDTH
#define FC_WIDTH "width"
#endif

typedef struct {
   char      *name;
   FcCharSet *fcs;
   int        glyphs;
   Bool       enabled;
   uint32_t   map[128];   /* maps characters 128-255 into unicode */
} CharSetInfo;

static CharSetInfo std_charsets[] = {
    { "iso8859-1",  nil, 0, 1 }
#ifdef HAVE_ICONV_H    
    ,
    { "iso8859-2",  nil, 0, 0 },
    { "iso8859-3",  nil, 0, 0 },
    { "iso8859-4",  nil, 0, 0 },
    { "iso8859-5",  nil, 0, 0 },
    { "iso8859-7",  nil, 0, 0 },
    { "iso8859-8",  nil, 0, 0 },
    { "iso8859-9",  nil, 0, 0 },
    { "iso8859-10", nil, 0, 0 },
    { "iso8859-13", nil, 0, 0 },
    { "iso8859-14", nil, 0, 0 },
    { "iso8859-15", nil, 0, 0 },
    { "koi8-r",     nil, 0, 0 }  /* this is special - change the constant
                                    KOI8_INDEX as well when updating
                                    the table */
/* You are welcome to add more 8-bit charsets here - just keep in mind
   that each encoding requires iconv() to load a file */
#endif    
};

#define KOI8_INDEX 12
#define MAX_CHARSET (sizeof(std_charsets)/sizeof(CharSetInfo))
#define MAX_GLYPH_SIZE (guts.limits.request_length / 256)

static PHash encodings    = nil;
static PHash mismatch     = nil; /* fonts not present in xft base */
static char  fontspecific[] = "fontspecific";
static CharSetInfo * locale = nil;

#ifdef NEED_X11_EXTENSIONS_XRENDER_H
/* piece of Xrender guts */
typedef struct _XExtDisplayInfo {
    struct _XExtDisplayInfo *next;      
    Display *display;                   
    XExtCodes *codes;                   
    XPointer data;                      
} XExtDisplayInfo;

extern XExtDisplayInfo *
XRenderFindDisplay (Display *dpy);
#endif

void
prima_xft_init(void)
{
   CharSetInfo *csi;
   int i;
   FcCharSet * fcs_ascii;
#ifdef HAVE_ICONV_H
   iconv_t ii;
   unsigned char in[128], *iptr;
   char ucs4[12];
   size_t ibl, obl;
   uint32_t *optr;
   int j;
#endif  

#ifdef NEED_X11_EXTENSIONS_XRENDER_H
   { /* snatch error code from xrender guts */
      XExtDisplayInfo *info = XRenderFindDisplay( DISP);
      if ( info && info-> codes)
         guts. xft_xrender_major_opcode = info-> codes-> major_opcode;
   }
#endif   

   if ( !apc_fetch_resource( "Prima", "", "UseXFT", "usexft", 
                              nilHandle, frUnix_int, &guts. use_xft))
      guts. use_xft = 1;
   if ( guts. use_xft) {
      if ( !XftInit(0)) guts. use_xft = 0;
   }
   /* After this point guts.use_xft must never be altered */
   if ( !guts. use_xft) return;
   Fdebug("XFT ok\n");

   csi = std_charsets;
   fcs_ascii = FcCharSetCreate();
   for ( i = 32; i < 127; i++)  FcCharSetAddChar( fcs_ascii, i);


   std_charsets[0]. fcs = FcCharSetUnion( fcs_ascii, fcs_ascii);
   for ( i = 161; i < 255; i++) FcCharSetAddChar( std_charsets[0]. fcs, i);
   for ( i = 128; i < 255; i++) std_charsets[0]. map[i - 128] = i;
   std_charsets[0]. glyphs = ( 127 - 32) + ( 255 - 161);
      
#ifdef HAVE_ICONV_H
   sprintf( ucs4, "UCS-4%cE", (guts.machine_byte_order == LSBFirst) ? 'L' : 'B');
   for ( i = 1; i < MAX_CHARSET; i++) {
      memset( std_charsets[i]. map, 0, sizeof(std_charsets[i]. map));

      guts. machine_byte_order = LSBFirst;

      ii = iconv_open(ucs4, std_charsets[i]. name);
      if ( ii == (iconv_t)(-1)) continue;

      std_charsets[i]. fcs = FcCharSetUnion( fcs_ascii, fcs_ascii);
      for ( j = 0; j < 128; j++) in[j] = j + 128;
      iptr = in;
      optr = std_charsets[i]. map;
      ibl = 128;
      obl = 128 * sizeof( uint32_t);
      while ( 1 ) {
         int ret = iconv( ii, ( char **) &iptr, &ibl, ( char **) &optr, &obl);
         if ( ret < 0 && errno == EILSEQ) {
            iptr++;
            optr++;
            ibl--;
            obl -= sizeof(uint32_t);
            continue;
         }
         break;
      }
      iconv_close(ii);

      optr = std_charsets[i]. map - 128;
      std_charsets[i]. glyphs = 127 - 32;
      for ( j = (( i == KOI8_INDEX) ? 191 : 161); j < 256; j++) 
         /* koi8 hack - 161-190 are pseudo-graphic symbols, not really characters,
            so don't use them for font matching by a charset */
         if ( optr[j]) {
            FcCharSetAddChar( std_charsets[i]. fcs, optr[j]);
            std_charsets[i]. glyphs++;
         }
      if ( std_charsets[i]. glyphs > 127 - 32) 
         std_charsets[i]. enabled = true;
   }
#endif

   mismatch     = hash_create();
   encodings    = hash_create();
   for ( i = 0; i < MAX_CHARSET; i++) {
      int length = 0;
      char upcase[256], *dest = upcase, *src = std_charsets[i].name;
      if ( !std_charsets[i]. enabled) continue;
      while ( *src) {
         *dest++ = toupper(*src++);
         length++;
      }
      hash_store( encodings, upcase, length, (void*) (std_charsets + i));
      hash_store( encodings, std_charsets[i]. name, length, (void*) (std_charsets + i));
   }
 
   locale = hash_fetch( encodings, guts. locale, strlen( guts.locale));
   if ( !locale) locale = std_charsets;
   FcCharSetDestroy( fcs_ascii);
}

void
prima_xft_done(void)
{
   int i;
   if ( !guts. use_xft) return;
   for ( i = 0; i < MAX_CHARSET; i++)
      if ( std_charsets[i]. fcs)
         FcCharSetDestroy( std_charsets[i]. fcs);
   hash_destroy( encodings, false);
   hash_destroy( mismatch, false);
}

static unsigned short
utf8_flag_strncpy( char * dst, const char * src, unsigned int maxlen, unsigned short is_utf8_flag)
{
	int is_utf8 = 0;
	while ( maxlen-- && *src) {
		if ( *((unsigned char*)src) > 0x7f) 
			is_utf8 = 1;
		*(dst++) = *(src++);
	}
	*dst = 0;
	return is_utf8 ? is_utf8_flag : 0;
}

static void
fcpattern2font( FcPattern * pattern, PFont font)
{
   FcChar8 * s;
   int i, j;
   double d = 1.0;
   FcCharSet *c = nil;

   /* FcPatternPrint( pattern); */
   if ( FcPatternGetString( pattern, FC_FAMILY, 0, &s) == FcResultMatch)
      font-> utf8_flags |= utf8_flag_strncpy( font-> name, (char*)s, 255, FONT_UTF8_NAME);
   if ( FcPatternGetString( pattern, FC_FOUNDRY, 0, &s) == FcResultMatch)
      font-> utf8_flags |= utf8_flag_strncpy( font-> family, (char*)s, 255, FONT_UTF8_FAMILY);
   font-> style = 0;
   if ( FcPatternGetInteger( pattern, FC_SLANT, 0, &i) == FcResultMatch) 
      if ( i == FC_SLANT_ITALIC || i == FC_SLANT_OBLIQUE)
         font-> style |= fsItalic;
   if ( FcPatternGetInteger( pattern, FC_WEIGHT, 0, &i) == FcResultMatch) {
      if ( i <= FC_WEIGHT_LIGHT)
         font-> style |= fsThin;
      else if ( i >= FC_WEIGHT_BOLD)
         font-> style |= fsBold;
   }
   if ( FcPatternGetInteger( pattern, FC_SPACING, 0, &i) == FcResultMatch)
      font-> pitch = (( i == FC_PROPORTIONAL) ? fpVariable : fpFixed);

   if ( FcPatternGetInteger( pattern, FC_PIXEL_SIZE, 0, &font-> height) == FcResultMatch) {
       Fdebug("xft:height factor read:%d\n", font-> height);
   }
   font-> width = 100; /* warning, FC_WIDTH does not reflect FC_MATRIX scale changes */
   if ( FcPatternGetInteger( pattern, FC_WIDTH, 0, &font-> width) == FcResultMatch) {
       Fdebug("xft:width factor read:%d\n", font-> width);
   }
   font-> width = ( font-> width * font-> height) / 100.0 + .5;
   font-> yDeviceRes = guts. resolution. y;
   FcPatternGetInteger( pattern, FC_DPI, 0, &font-> yDeviceRes);
   if ( font-> yDeviceRes <= 0)
      font-> yDeviceRes = guts. resolution. y;
   FcPatternGetBool( pattern, FC_SCALABLE, 0, &font-> vector);
   FcPatternGetDouble( pattern, FC_ASPECT, 0, &d);
   font-> xDeviceRes = font-> yDeviceRes * d;
   if ( 
         (FcPatternGetInteger( pattern, FC_SIZE, 0, &font-> size) != FcResultMatch) &&
         (font-> height != C_NUMERIC_UNDEF)
      ) {
      font-> size = font-> height * 72.27 / font-> yDeviceRes + .5;
      Fdebug("xft:size calculated:%d\n", font-> size);
   }

   font-> firstChar = 32; font-> lastChar = 255;
   font-> breakChar = 32; font-> defaultChar = 32;
   if (( FcPatternGetCharSet( pattern, FC_CHARSET, 0, &c) == FcResultMatch) && c) {
      FcChar32 ucs4, next, map[FC_CHARSET_MAP_SIZE];
      if (( ucs4 = FcCharSetFirstPage( c, map, &next)) != FC_CHARSET_DONE) {
         for (i = 0; i < FC_CHARSET_MAP_SIZE; i++) 
            if ( map[i] ) {
               for (j = 0; j < 32; j++)
                  if (map[i] & (1 << j)) {
                      FcChar32 u = ucs4 + i * 32 + j;
                      font-> firstChar = u;
                      if ( font-> breakChar   < u) font-> breakChar   = u;
                      if ( font-> defaultChar < u) font-> defaultChar = u;
                      goto STOP_1;
                  }
            }
STOP_1:;
         while ( next != FC_CHARSET_DONE)
            ucs4 = FcCharSetNextPage (c, map, &next);
         for (i = FC_CHARSET_MAP_SIZE - 1; i >= 0; i--) 
            if ( map[i] ) {
               for (j = 31; j >= 0; j--)
                  if (map[i] & (1 << j)) {
                      FcChar32 u = ucs4 + i * 32 + j;
                      font-> lastChar = u;
                      if ( font-> breakChar   > u) font-> breakChar   = u;
                      if ( font-> defaultChar > u) font-> defaultChar = u;
                      goto STOP_2;
                  }
            }
STOP_2:;
      }
   }
   
   /* XXX other details? */
   font-> descent = font-> height / 4;
   font-> ascent  = font-> height - font-> descent;
   font-> maximalWidth = font-> width;
   font-> internalLeading = 0;
   font-> externalLeading = 0;
}

static void
xft_build_font_key( PFontKey key, PFont f, Bool bySize)
{
   bzero( key, sizeof( FontKey));
   key-> height = bySize ? -f-> size : f-> height;
   key-> width = f-> width;
   key-> style = f-> style & ~(fsUnderlined|fsOutline|fsStruckOut);
   key-> pitch = f-> pitch;
   key-> direction = f-> direction;
   strcpy( key-> name, f-> name);
}

static PCachedFont
try_size( Handle self, Font f, double size)
{
   FontKey key;
   f. size = size;
   f. height = f. width = C_NUMERIC_UNDEF;
   f. direction = 0;
   if ( !prima_xft_font_pick( self, &f, &f, &size)) return nil;
   f. width = 0;
   xft_build_font_key( &key, &f, true);
   return ( PCachedFont) hash_fetch( guts. font_hash, &key, sizeof( FontKey));
}


Bool
prima_xft_font_pick( Handle self, Font * source, Font * dest, double * size)
{
   FcPattern *request, *match;
   FcResult res = FcResultNoMatch;
   Font f, f1;
   Bool by_size;
   CharSetInfo * csi;
   XftFont * xf;
   FontKey key; 
   PCachedFont kf, kf_base = nil;
   int i, base_width = 1, pixel_size, exact_pixel_size = 0;
  
   if ( !guts. use_xft) return false;

   f = *dest;
   by_size = Drawable_font_add( self, source, &f);
   pixel_size = f. height;

   if ( guts. xft_disable_large_fonts > 0) {
      /* xft is unable to deal with large polygon requests. 
         we must cut the large fonts here, before Xlib croaks */
      if (
            ( by_size && ( f. size >= MAX_GLYPH_SIZE)) ||
            (!by_size && ( f. height >= MAX_GLYPH_SIZE / 72.27 * guts. resolution. y))
         ) 
         return false;
   }

   /* see if the font is not present in xft - the hashed negative matches
         are stored with width=0, as the width alterations are derived */
   xft_build_font_key( &key, &f, by_size);
   Fdebug("xft:want %d.%d.%d.%d.%s\n", key.height, key. width, key.style, key.pitch, key.name);
   
   key. width = 0;
   if ( hash_fetch( mismatch, &key, sizeof( FontKey))) {
      Fdebug("xft: refuse\n");
      return false;
   }
   key. width = f. width;

   /* convert encoding */
   csi = ( CharSetInfo*) hash_fetch( encodings, f. encoding, strlen( f. encoding));
   if ( !csi) {
      /* xft has no such encoding, pass it back */
      if ( prima_core_font_encoding( f. encoding) || !guts. xft_priority)
         return false;
      csi = locale;
   }

   /* see if cached font exists */
   if (( kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
      *dest = kf-> font;
      strcpy( dest-> encoding, csi-> name);
      if ( f. style & fsStruckOut) dest-> style |= fsStruckOut;
      if ( f. style & fsUnderlined) dest-> style |= fsUnderlined;
      return true;
   }
   /* see if the non-xscaled font exists */
   if ( key. width != 0) {
      key. width = 0;
      if ( !( kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
         Font s = *source, d = *dest;
         s. width = d. width = 0;
         prima_xft_font_pick( self, &s, &d, size);
      }
      if ( kf || ( kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
         base_width = kf-> font. width;
         if ( FcPatternGetInteger( kf-> xft-> pattern, FC_PIXEL_SIZE, 0, &pixel_size) == FcResultMatch)
            exact_pixel_size = 1;
      } else { /* if fails, cancel x scaling and see if it failed due to banning */
         if ( hash_fetch( mismatch, &key, sizeof( FontKey))) return false;
         f. width = 0;
      }
   }
   /* see if the non-rotated font exists */
   if ( key. direction != 0) {
      key. direction = 0;
      key. width = f. width;
      if ( !( kf_base = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
         Font s = *source, d = *dest;
         s. direction = d. direction = 0;
         prima_xft_font_pick( self, &s, &d, size);
         /* if fails, cancel rotation and see if the base font is banned  */
         if ( !( kf_base = hash_fetch( guts. font_hash, &key, sizeof( FontKey))))
            f. direction = 0;
      }
      if ( f. direction != 0) {
         /* as f. height != FC_PIXEL_SIZE, read the correct request
            from the non-rotated font */
         if ( FcPatternGetInteger( kf_base-> xft-> pattern, FC_PIXEL_SIZE, 0, &pixel_size) == FcResultMatch)
            exact_pixel_size = 1;
      }
   }
   
   /* create FcPattern request */
   if ( !( request = FcPatternCreate())) return false;
   if ( strcmp( f. name, "Default") != 0) 
      FcPatternAddString( request, FC_FAMILY, ( FcChar8*) f. name);
   if ( by_size) {
      if ( size)
         FcPatternAddDouble( request, FC_SIZE, *size);
      else
         FcPatternAddInteger( request, FC_SIZE, f. size);
   } else
      FcPatternAddInteger( request, FC_PIXEL_SIZE, pixel_size);
   FcPatternAddInteger( request, FC_SLANT, ( f. style & fsItalic) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
   FcPatternAddInteger( request, FC_SPACING, ( f. pitch == fpFixed) ? FC_MONO : FC_PROPORTIONAL);
   FcPatternAddInteger( request, FC_WEIGHT, 
                        ( f. style & fsBold) ? FC_WEIGHT_BOLD :
                        ( f. style & fsThin) ? FC_WEIGHT_THIN : FC_WEIGHT_NORMAL);
   FcPatternAddInteger( request, FC_SCALABLE, 1);
   if ( f. direction != 0 || f. width != 0) {
      FcMatrix mat;
      FcMatrixInit(&mat);
      if ( f. width != 0)
         FcMatrixScale( &mat, ( double) f. width / base_width, 1);
      if ( f. direction != 0)
         FcMatrixRotate( &mat, cos(f.direction * 3.14159265358 / 180.0), sin(f.direction * 3.14159265358 / 180.0));
      FcPatternAddMatrix( request, FC_MATRIX, &mat);
   }

   if ( guts. xft_no_antialias)
      FcPatternAddBool( request, FC_ANTIALIAS, 0);

   /* match best font - must return something useful; the match is statically allocated */
   match = XftFontMatch( DISP, SCREEN, request, &res);
   if ( !match) {
      Fdebug("xft: XftFontMatch error\n");
      FcPatternDestroy( request);
      return false;
   }
   FcPatternDestroy( request);
   
   /* Manually check if font contains wanted encoding - matching by FcCharSet 
      can't set threshold on how many glyphs can be omitted */
   {
      FcCharSet *c = nil;
      FcPatternGetCharSet( match, FC_CHARSET, 0, &c);
      if ( c && (
             ( FcCharSetCount(c) == 0) ||
             
             (
                f. encoding[0] && 
                ( strcmp( f. encoding, fontspecific) != 0) &&
                ( FcCharSetIntersectCount( csi-> fcs, c) < csi-> glyphs - 1)
             )
         )) {
         xft_build_font_key( &key, &f, by_size);
         key. width = 0;
         hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
         Fdebug("xft: charset mismatch\n");
         FcPatternDestroy( match);
         return false;
      }
   }

   /* Check if the matched font is scalable -- see comments in the beginning 
      of the file about non-scalable fonts in Xft */
   {
      FcBool scalable;
      if (( FcPatternGetBool( match, FC_SCALABLE, 0, &scalable) == FcResultMatch) && !scalable) {
         xft_build_font_key( &key, &f, by_size);
         key. width = 0;
         hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
	 Fdebug("xft: refuse bitmapped font\n");
         FcPatternDestroy( match);
         return false;
      }
   }

   /* XXX copy font details - very important these are correct !!! */
   /* strangely enough, if the match is used after XftFontOpenPattern, it is
      destroyed */
   f1 = f;
   if ( !kf_base) {
      Bool underlined = f1. style & fsUnderlined;
      Bool strike_out  = f1. style & fsStruckOut;
      fcpattern2font( match, &f1);
      if ( f. width > 0) f1. width = f. width;
      if ( underlined) f1. style |= fsUnderlined;
      if ( strike_out) f1. style |= fsStruckOut;
   }


   /* check name match */
   {
      FcChar8 * s = nil;
      FcPatternGetString( match, FC_FAMILY, 0, &s);
      if ( !s || strcmp(( const char*) s, f. name) != 0) {
	 int i, n = guts. n_fonts;
         PFontInfo info = guts. font_info;

	 if ( !guts. xft_priority) {
	    Fdebug("xft: name mismatch\n");
	 NAME_MISMATCH:
	    xft_build_font_key( &key, &f, by_size);
	    key. width = 0;
	    hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
            FcPatternDestroy( match);
	    return false;
	 }
	 
         /* check if core has cached face name */
	 if ( prima_find_known_font( &f, false, by_size)) {
	    Fdebug("xft: pass to cached core\n");
	    goto NAME_MISMATCH;
	 }

         /* check if core has non-cached face name */
         for ( i = 0; i < n; i++) {
            if ( 
		  info[i]. flags. disabled || 
		  !info[i].flags.name ||
	          (strcmp( info[i].font.name, f.name) != 0) 
	       ) continue;
	    Fdebug("xft: pass to core\n");
	    goto NAME_MISMATCH;
         }
      }
   }
  
   /* load the font */
   xf = XftFontOpenPattern( DISP, match);
   if ( !xf) {
      xft_build_font_key( &key, &f, by_size);
      key. width = 0;
      hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
      Fdebug("xft: XftFontOpenPattern error\n");
      FcPatternDestroy( match);
      return false;
   }
   if ( kf_base) {
      /* A bit hacky, since the rotated font may be substituted by Xft.
         We skip the non-scalable fonts earlier to assure this doesn't happen,
         but anyway it's not 100% */
      Bool underlined = f1. style & fsUnderlined;
      Bool strike_out  = f1. style & fsStruckOut;
      f1 = kf_base-> font; 
      f1. direction = f. direction;
      strcpy( f1. encoding, csi-> name);
      if ( underlined) f1. style |= fsUnderlined;
      if ( strike_out) f1. style |= fsStruckOut;
   } else {
      f1. internalLeading = xf-> height - f1. size * guts. resolution. y / 72.27 + 0.5;
      if ( !by_size && !exact_pixel_size) {
         /* Try to locate the corresponding size and
            height; multiply size by 10 to address pixel-wise heights correctly.
            The max. screen resolution allowable here is therefore 720 dpi.
            Experiments show that factors more than 10 are bad, since too 
            precise arguments befuddle the heights guesser, as heights are still
            integers. Initially I thought that it was the Prima flaw that it doesn't
            provide size precision less than 1.0, but it is not so - Xft's
            xf->height and fc's FC_PIXEL_SIZE are way different, and this
            cannot be compensated, except by guess.  */
         HeightGuessStack hgs;
         PCachedFont ksz = nil;
         int h, sz = f1. size * 10, last_sz = -1;

         ksz = try_size( self, f1, ( double) sz / 10.0);
         if ( ksz) {
            h = ksz-> font. height;
            Fdebug("xft-match:init:%d, %d => %d\n", f1.height, sz, h);
            if ( h != f1. height) {
               prima_init_try_height( &hgs, f1. height, sz);
               while ( 1) {
                  last_sz = sz;
                  sz = prima_try_height( &hgs, h);
                  if ( sz < 0) break;
                  ksz = try_size( self, f1, ( double) sz / 10.0);
                  if ( !ksz) break;
                  h = ksz-> font. height;
                  Fdebug("%d => %d\n", sz, h);
                  if ( h == f1. height) break;
               }
            }
            if ( sz < 0) sz = last_sz;
            Fdebug("fini:%d\n", sz);
            if ( sz > 0) f1. size = (double) sz / 10.0 + 0.5;
         }

         if ( ksz && f1. height != xf-> height) {
            /* the height returned is invalid, but cache it anyway */
            xft_build_font_key( &key, &f1, false);
            if ( !hash_fetch( guts. font_hash, &key, sizeof(FontKey))) {
               if (( kf = malloc( sizeof( CachedFont)))) {
                  bzero( kf, sizeof( CachedFont));
                  memcpy( &kf-> font, &f1, sizeof( Font));
         	  kf-> font. style &= ~(fsUnderlined|fsOutline|fsStruckOut);
                  kf-> xft = kf-> xft_base = xf;
                  hash_store( guts. font_hash, &key, sizeof( FontKey), kf);
                  Fdebug("xft:store %x:%d.%d.%d.%d.%s\n", kf->xft, key.height, key. width, key.style, key.pitch, key.name);
               }
            }

            /* and, finally, replace the font and compute internal leading */
            xf = ksz-> xft; 
            f1. height = xf-> height;
            f1. internalLeading = xf-> height - f1. size * guts. resolution. y / 72.27 + 0.5;
         }
         Fdebug("xft:sz:%d, h:%d\n", f1.size, f1.height); 
      } else 
         f1. height  = xf-> height;

      f1. ascent  = xf-> ascent;
      f1. descent = xf-> descent;
   
      f1. maximalWidth = xf-> max_advance_width;
      /* calculate average font width */
      if ( f1. pitch != fpFixed) {
         FcChar32 c;
         XftFont *x = kf_base ? kf_base-> xft : xf;
         int num = 0, sum = 0;
         for ( c = 63; c < 126; c+=4) {
            FT_UInt ft_index;
            XGlyphInfo glyph;
            if ( !( ft_index = XftCharIndex( DISP, x, c))) continue;
            XftGlyphExtents( DISP, x, &ft_index, 1, &glyph);
            sum += glyph. xOff;
            num++;
         }
         f1. width = ( num > 10) ? (sum / num) : f1. maximalWidth;
      } else
         f1. width = f1. maximalWidth;
   }
   
   /* create hash entry for subsequent loads of same font */
   xft_build_font_key( &key, &f, by_size);
   if ( !hash_fetch( guts. font_hash, &key, sizeof(FontKey))) {
      if (( kf = malloc( sizeof( CachedFont)))) {
         bzero( kf, sizeof( CachedFont));
         memcpy( &kf-> font, &f1, sizeof( Font));
         kf-> font. style &= ~(fsUnderlined|fsOutline|fsStruckOut);
         kf-> xft = xf;
         kf-> xft_base = kf_base ? kf_base-> xft : xf;
         hash_store( guts. font_hash, &key, sizeof( FontKey), kf);
         Fdebug("xft:store %x:%d.%d.%d.%d.%s\n", kf->xft, key.height, key. width, key.style, key.pitch, key.name); 
      }
   }
   
   /* and with the matched by height and size */
   for ( i = 0; i < 2; i++) {
      xft_build_font_key( &key, &f1, i);
      if ( !hash_fetch( guts. font_hash, &key, sizeof(FontKey))) {
         if (( kf = malloc( sizeof( CachedFont)))) {
            bzero( kf, sizeof( CachedFont));
            memcpy( &kf-> font, &f1, sizeof( Font));
            kf-> font. style &= ~(fsUnderlined|fsOutline|fsStruckOut);
            kf-> xft = xf;
            kf-> xft_base = kf_base ? kf_base-> xft : xf;
            hash_store( guts. font_hash, &key, sizeof( FontKey), kf);
            Fdebug("xft:store %x:%d.%d.%d.%d.%s\n", kf->xft, key.height, key. width, key.style, key.pitch, key.name); 
         }
      }
   }

   *dest = f1;
   return true;
}

Bool
prima_xft_set_font( Handle self, PFont font)
{
   DEFXX;
   CharSetInfo * csi;
   PCachedFont kf = prima_xft_get_cache( font);
   if ( !kf) return false;
   XX-> font = kf;
   if ( !( csi = hash_fetch( encodings, font-> encoding, strlen( font-> encoding))))
      csi = locale;
   XX-> xft_map8 = csi-> map;
   if ( PDrawable( self)-> font. direction != 0) {
      XX-> xft_font_sin = sin( font-> direction / 57.29577951);
      XX-> xft_font_cos = cos( font-> direction / 57.29577951);
   } else {
      XX-> xft_font_sin = 0.0;
      XX-> xft_font_cos = 1.0;
   }
   return true;
}

PCachedFont
prima_xft_get_cache( PFont font)
{
   FontKey key;
   PCachedFont kf;
   xft_build_font_key( &key, font, false);
   kf = ( PCachedFont) hash_fetch( guts. font_hash, &key, sizeof( FontKey));
   if ( !kf || !kf-> xft) return nil;
   return kf;
}

/*
   performs 3 types of queries:
   defined(facename) - list of fonts with facename and encoding, if defined encoding
   defined(encoding) - list of fonts with encoding
   !defined(encoding) && !defined(facename) - list of all fonts with all available encodings.

   since apc_fonts does the same and calls xft_fonts, array is the list of fonts
   filled already, so xft_fonts reallocates the list when needed.

   XXX - find proper font metrics, but where??
 */
PFont
prima_xft_fonts( PFont array, const char *facename, const char * encoding, int *retCount)
{
   FcFontSet * s;
   FcPattern   *pat, **ppat;
   FcObjectSet *os;
   PFont newarray, f;
   PHash names = nil;
   CharSetInfo * csi = nil;
   int i;

   if ( encoding) {
      if ( !( csi = ( CharSetInfo*) hash_fetch( encodings, encoding, strlen( encoding))))
         return array;
   }

   pat = FcPatternCreate();
   if ( facename) FcPatternAddString( pat, FC_FAMILY, ( FcChar8*) facename);
   FcPatternAddBool( pat, FC_SCALABLE, 1);
   os = FcObjectSetBuild( FC_FAMILY, FC_CHARSET, FC_ASPECT, 
        FC_SLANT, FC_WEIGHT, FC_SIZE, FC_PIXEL_SIZE, FC_SPACING,
        FC_FOUNDRY, FC_SCALABLE, FC_DPI,
        (void*) 0);
   s = FcFontList( 0, pat, os);
   FcObjectSetDestroy( os);
   FcPatternDestroy( pat);
   if ( !s) return array;

   /* XXX make dynamic */
   if ( !( newarray = realloc( array, sizeof(Font) * (*retCount + s-> nfont * MAX_CHARSET)))) {
      FcFontSetDestroy(s);
      return array;
   }
   ppat = s-> fonts; 
   f = newarray + *retCount;
   bzero( f, sizeof( Font) * s-> nfont * MAX_CHARSET);

   names = hash_create();
   
   for ( i = 0; i < s->nfont; i++, ppat++) {
      FcCharSet *c = nil;
      fcpattern2font( *ppat, f);
      FcPatternGetCharSet( *ppat, FC_CHARSET, 0, &c);
      if ( c && FcCharSetCount(c) == 0) continue;
      if ( encoding) {
         /* case 1 - encoding is set, filter only given encoding */
         if ( c && ( FcCharSetIntersectCount( csi-> fcs, c) >= csi-> glyphs - 1)) {
            if ( !facename) {
               /* and, if no facename set, each facename is reported only once */
               if ( hash_fetch( names, f-> name, strlen( f-> name))) continue;
               hash_store( names, f-> name, strlen( f-> name), ( void*)1);
            }
            strncpy( f-> encoding, encoding, 255);
            f++;
         } 
      } else if ( facename) {
         /* case 2 - facename only is set, report each facename with every encoding */
         int j;
         Font * tmpl = f;
         for ( j = 0; j < MAX_CHARSET; j++) {
            if ( !std_charsets[j]. enabled) continue;
            if ( FcCharSetIntersectCount( c, std_charsets[j]. fcs) >= std_charsets[j]. glyphs - 1) {
               *f = *tmpl;
               strncpy( f-> encoding, std_charsets[j]. name, 255);
               f++;
            }
         }
         if ( f == tmpl) {/* no encodings found */
            strcpy( f-> encoding, fontspecific);
            f++;
         }
      } else if ( !facename && !encoding) { 
         /* case 3 - report unique facenames and store list of encodings
            into the hack array */
         if ( hash_fetch( names, f-> name, strlen( f-> name)) == (void*)1) continue;
         hash_store( names, f-> name, strlen( f-> name), (void*)1);
         if ( c) {
            int j, found = 0;
            char ** enc = (char**) f-> encoding;
            unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
            for ( j = 0; j < MAX_CHARSET; j++) {
               if ( !std_charsets[j]. enabled) continue;
               if ( *shift + 2 >= 256 / sizeof(char*)) break;
               if ( FcCharSetIntersectCount( c, std_charsets[j]. fcs) >= std_charsets[j]. glyphs - 1) {
                  char buf[ 512];
                  int len = snprintf( buf, 511, "%s-charset-%s", f-> name, std_charsets[j]. name);
                  if ( hash_fetch( names, buf, len) == (void*)2) continue;
                  hash_store( names, buf, len, (void*)2);
                  *(enc + ++(*shift)) = std_charsets[j]. name;
                  found = 1;
               }
            }
            if ( !found)
               *(enc + ++(*shift)) = fontspecific;
         }
         f++;
      }
   }

   *retCount = f - newarray;
   
   hash_destroy( names, false);
   
   FcFontSetDestroy(s);
   return newarray;
}

void
prima_xft_font_encodings( PHash hash)
{
   int i;
   for ( i = 0; i < MAX_CHARSET; i++) {
      if ( !std_charsets[i]. enabled) continue;
      hash_store( hash, std_charsets[i]. name, strlen(std_charsets[i]. name), (void*) (std_charsets + i));
   }
}
   
static FcChar32 *
xft_text2ucs4( const unsigned char * text, int len, Bool utf8, uint32_t * map8)
{
   FcChar32 *ret, *r;
   if ( utf8) {
      STRLEN charlen;
      if ( len < 0) len = prima_utf8_length(( char*) text);
      if ( !( r = ret = malloc( len * sizeof( FcChar32)))) return nil;
      while ( len--) {
         *(r++) = utf8_to_uvchr(( U8*) text, &charlen);
         text += charlen;
      }
   } else {
      int i;
      if ( len < 0) len = strlen(( char*) text);
      if ( !( ret = malloc( len * sizeof( FcChar32)))) return nil;
      for ( i = 0; i < len; i++) 
         ret[i] = ( text[i] < 128) ? text[i] : map8[ text[i] - 128];
   }
   return ret;
}

int
prima_xft_get_text_width( PCachedFont self, const char * text, int len, Bool addOverhang, 
                          Bool utf8, uint32_t * map8, Point * overhangs)
{
   int i, ret = 0;
   XftFont * font = self-> xft_base;

   if ( overhangs) overhangs-> x = overhangs-> y = 0;

   for ( i = 0; i < len; i++) {
      FcChar32 c;
      FT_UInt ft_index;
      XGlyphInfo glyph;
      if ( utf8) {
         STRLEN charlen;
         c = ( FcChar32) utf8_to_uvchr(( U8*) text, &charlen);
         text += charlen;
      } else if ( ((Byte*)text)[i] > 127) {
         c = map8[ ((Byte*)text)[i] - 128];
      } else
         c = text[i];
      ft_index = XftCharIndex( DISP, font, c);
      XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
      ret += glyph. xOff;
      if ( addOverhang || overhangs) {
         if ( i == 0) {
            if ( glyph. x > 0) {
               if ( addOverhang) ret += glyph. x;
               if ( overhangs)   overhangs-> x = glyph. x;
            }
         } 
         if ( i == len - 1) {
            int c = glyph. xOff - glyph. width + glyph. x;
            if ( c < 0) {
               if ( addOverhang) ret -= c;
               if ( overhangs)   overhangs-> y = -c;
            }
         }
      }
   }
   return ret;
}

Point *
prima_xft_get_text_box( Handle self, const char * text, int len, Bool utf8)
{
   DEFXX;
   Point ovx;
   int width;
   Point * pt = ( Point *) malloc( sizeof( Point) * 5);
   if ( !pt) return nil;

   width = prima_xft_get_text_width( XX-> font, text, len, 
      false, utf8, X(self)-> xft_map8, &ovx);

   pt[0].y = pt[2]. y = XX-> font-> font. ascent - 1;
   pt[1].y = pt[3]. y = - XX-> font-> font. descent;
   pt[4].y = 0;
   pt[4].x = width;
   pt[3].x = pt[2]. x = width + ovx. y;
   pt[0].x = pt[1]. x = - ovx. x;

   if ( !XX-> flags. paint_base_line) {
      int i;
      for ( i = 0; i < 4; i++) pt[i]. y += XX-> font-> font. descent;
   }   
   
   if ( PDrawable( self)-> font. direction != 0) {
      int i;
      double s = sin( PDrawable( self)-> font. direction / 57.29577951);
      double c = cos( PDrawable( self)-> font. direction / 57.29577951);
      for ( i = 0; i < 5; i++) {
         double x = pt[i]. x * c - pt[i]. y * s;
         double y = pt[i]. x * s + pt[i]. y * c;
         pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
         pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
      }
   }
 
   return pt;
}

static XftFont *
create_no_aa_font( XftFont * font)
{
   FcPattern * request;
   if (!( request = FcPatternDuplicate( font-> pattern))) return nil;
   FcPatternDel( request, FC_ANTIALIAS);
   FcPatternAddBool( request, FC_ANTIALIAS, 0);
   return XftFontOpenPattern( DISP, request);
}

#define SORT(a,b)	{ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)	(XX-> size. y - (a) - 1)
#define SHIFT(a,b)	{ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)

/* When plotting rotated fonts, xft does not account for the accumulated
   roundoff error, and thus the text line is shown at different angle
   than requested. We track this and align the reference point when it
   deviates from the ideal line */
void
my_XftDrawString32( PDrawableSysData selfxx,
	_Xconst XftColor *color, int x, int y,
	_Xconst FcChar32 *string, int len)
{
	int i, lastchar, lx, ly, ox, oy, shift;
	if ( XX-> font-> font. direction == 0) {
		XftDrawString32( XX-> xft_drawable, color, XX-> font-> xft, x, y, string, len);
		return;
	}
	lx = ox = x;
	ly = oy = y;
	lastchar = 0;
	shift = 0;
	for ( i = 0; i < len; i++) {
		int cx, cy;
		FT_UInt ft_index;
		XGlyphInfo glyph;
		ft_index = XftCharIndex( DISP, XX-> font-> xft, string[i]);
		XftGlyphExtents( DISP, XX-> font-> xft, &ft_index, 1, &glyph);
		lx += glyph. xOff;
		ly += glyph. yOff;
		XftGlyphExtents( DISP, XX-> font-> xft_base, &ft_index, 1, &glyph);
		shift += glyph. xOff;
		cx = ox + (int)(shift * XX-> xft_font_cos + 0.5);
		cy = oy - (int)(shift * XX-> xft_font_sin + 0.5);
		if ( cx == lx && cy == ly) continue;

		XftDrawString32( XX-> xft_drawable, color, XX-> font-> xft, 
			x, y, string + lastchar, i - lastchar + 1);
		lastchar = i + 1;
		x = lx = cx;
		y = ly = cy;
	}

	if ( lastchar < len)
		XftDrawString32( XX-> xft_drawable, color, XX-> font-> xft, 
			x, y, string + lastchar, len - lastchar);
}

Bool
prima_xft_text_out( Handle self, const char * text, int x, int y, int len, Bool utf8)
{
   DEFXX;
   FcChar32 *ucs4;
   XftColor xftcolor;
   XftFont *font = XX-> font-> xft;
   int rop = XX-> paint_rop;
   Point baseline;

   if ( len == 0) return true;

   /* filter out unsupported rops */
   switch ( rop) {
   case ropBlackness:
      xftcolor.color.red   = 
      xftcolor.color.green = 
      xftcolor.color.blue  = 
      xftcolor.pixel       = 0;
      rop = ropCopyPut;
      break;
   case ropWhiteness:
      xftcolor.color.red   = 
      xftcolor.color.green = 
      xftcolor.color.blue  = 0xffff;
      xftcolor.pixel       = 0xffffffff;
      rop = ropCopyPut;
      break;   
   case ropXorPut:  
   case ropOrPut:   
   case ropNotSrcAnd:   
   case ropNotSrcXor:   
   case ropNotSrcOr:   
   case ropAndPut:  
      xftcolor.color.red   = COLOR_R16(XX->fore.color);
      xftcolor.color.green = COLOR_G16(XX->fore.color);
      xftcolor.color.blue  = COLOR_B16(XX->fore.color);
      xftcolor.pixel       = XX-> fore. primary;
      break;
   case ropNotPut:
      xftcolor.color.red   = COLOR_R16(~XX->fore.color);
      xftcolor.color.green = COLOR_G16(~XX->fore.color);
      xftcolor.color.blue  = COLOR_B16(~XX->fore.color);
      xftcolor.pixel       = ~XX-> fore. primary;
      rop = ropCopyPut;
      break;
   default:
      xftcolor.color.red   = COLOR_R16(XX->fore.color);
      xftcolor.color.green = COLOR_G16(XX->fore.color);
      xftcolor.color.blue  = COLOR_B16(XX->fore.color);
      xftcolor.pixel       = XX-> fore. primary;
      rop = ropCopyPut;
   }

   if ( XX-> type. bitmap) {
      xftcolor.color.alpha = 
	 ((xftcolor.color.red/3 + xftcolor.color.green/3 + xftcolor.color.blue/3) > (0xff00 / 2)) ?
	    0xffff : 0;
      /* force-remove antialiasing, xft doesn't have a better API for this */
      if ( !guts. xft_no_antialias && !XX-> font-> xft_no_aa) {
         FcBool aa;
         if ( 
	      ( FcPatternGetBool( font-> pattern, FC_ANTIALIAS, 0, &aa) == FcResultMatch)
	      && aa
	    ) {
	    XftFont * f = create_no_aa_font( font);
	    if ( f)
	       font = XX-> font-> xft_no_aa = f;
	 }
       }
   } else {
      xftcolor.color.alpha = 0xffff;
   }

   /* paint background if opaque */
   if ( XX-> flags. paint_opaque) {
      int i;
      Point * p = prima_xft_get_text_box( self, text, len, utf8);
      FillPattern fp;
      memcpy( &fp, apc_gp_get_fill_pattern( self), sizeof( FillPattern));
      XSetForeground( DISP, XX-> gc, XX-> back. primary);
      XX-> flags. brush_back = 0;
      XX-> flags. brush_fore = 1; 
      XX-> fore. balance = 0;
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
      apc_gp_set_color( self, XX-> fore. color);
      apc_gp_set_fill_pattern( self, fp);
      free( p); 
   }  
   SHIFT( x, y);
   RANGE2(x,y);

   /* xft doesn't allow shifting glyph reference point - need to adjust manually */
   baseline. x = - PDrawable(self)-> font. descent * XX-> xft_font_sin;
   baseline. y = - PDrawable(self)-> font. descent * ( 1 - XX-> xft_font_cos ) 
                 + XX-> font-> font. descent;
   if ( !XX-> flags. paint_base_line) {
      x += baseline. x;
      y += baseline. y;
   }

   /* allocate xftdraw surface */
   if ( !XX-> xft_drawable) {
      if ( XX-> type. bitmap) 
         XX-> xft_drawable = XftDrawCreateBitmap( DISP, XX-> gdrawable ); 
      else
         XX-> xft_drawable = XftDrawCreate( DISP, XX-> gdrawable, 
                                         guts. visual. visual, guts. defaultColormap);
      XftDrawSetSubwindowMode( XX-> xft_drawable, 
         ( self == application) ? IncludeInferiors : ClipByChildren);
      XCHECKPOINT;
   }
   if ( !XX-> flags. xft_clip) {
      XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
      XX-> flags. xft_clip = 1;
   }
  
   /* convert text string to unicode */
   if ( !( ucs4 = xft_text2ucs4(( unsigned char*) text, len, utf8, X( self)-> xft_map8))) 
      return false;
  
   if ( rop != ropCopyPut) {
   /* emulate rops by blitting the text */ 
      XGCValues gcv;
      GC gc;
      int dx  = prima_xft_get_text_width( XX-> font, text, len, 
          true, utf8, X(self)-> xft_map8, nil);
      int dy  = XX-> font-> font. height;
      int i, width, height;
      Rect rc;
      Point p[4], offset;
      Pixmap canvas;

      bzero( &rc, sizeof(rc));
      offset. x = offset. y = 0;
      p[0].x = p[2].x = 0;
      p[0].y = p[1].y = 0;
      p[1].x = p[3].x = dx;
      p[2].y = p[3].y = dy;
      rc. left = rc. right = rc. top = rc. bottom = 0;
      for ( i = 1; i < 4; i++) {
          int x = p[i].x * XX-> xft_font_cos - p[i].y * XX-> xft_font_sin + 0.5;
          int y = p[i].x * XX-> xft_font_sin + p[i].y * XX-> xft_font_cos + 0.5;
	  if ( rc. left    > x) rc. left   = x;
	  if ( rc. right   < x) rc. right  = x;
	  if ( rc. bottom  > y) rc. bottom = y;
	  if ( rc. top     < y) rc. top    = y;
      }
      width  = rc. right  - rc. left   + 1;
      height = rc. top    - rc. bottom + 1;

      canvas = XCreatePixmap( DISP, guts. root, width, height, 
                                     XX-> type. bitmap ? 1 : guts. depth);
      if ( !canvas) goto COPY_PUT;
      dx = -rc. left;
      dy = -rc. bottom;
      gc = XCreateGC( DISP, canvas, 0, &gcv);
      switch ( rop) {
      case ropAndPut:
      case ropNotSrcXor:
      case ropNotSrcOr:
         XSetForeground( DISP, gc, 0xffffffff);
         break;
      default:
         XSetForeground( DISP, gc, 0x0);
         break;
      }
      XFillRectangle( DISP, canvas, gc, 0, 0, width, height);
      XftDrawChange( XX-> xft_drawable, canvas);
      if ( XX-> flags. xft_clip)
         XftDrawSetClip( XX-> xft_drawable, 0);
      my_XftDrawString32( XX, &xftcolor, dx + baseline.x, height - dy - baseline. y, ucs4, len);
      XftDrawChange( XX-> xft_drawable, XX-> gdrawable);
      if ( XX-> flags. xft_clip)
         XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
      XCHECKPOINT;
      x -= baseline.x;
      y -= baseline.y;
      XCopyArea( DISP, canvas, XX-> gdrawable, XX-> gc, 0, 0, width, height, x - dx, REVERT( y - dy + height));
      XFreeGC( DISP, gc);
      XFreePixmap( DISP, canvas);
   } else {
   COPY_PUT:
      my_XftDrawString32( XX, &xftcolor, x, REVERT( y) + 1, ucs4, len);
   }
   free( ucs4);
   /*
      If you're guided here by something like

         X Error: BadLength (poly request too large or internal Xlib length error), 

      then you probably need to know that your X server is out of date.
      The error is caused by a Xrender bug, and you have the following options:
      - update your X server
      - set guts.xft_disable_large_fonts to 1, which would explicitly tell Prima not to wait
        for Xlib to bark on large polygons
      - set guts.xft_disable_large_fonts to 1 and decrease MAX_GLYPH_SIZE, if the former 
        option is not enough
    */
   XCHECKPOINT;
      
  
   /* emulate over- and understriking */ 
   if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {      
      Point ovx;
      int lw = apc_gp_get_line_width( self);
      int tw = prima_xft_get_text_width( XX-> font, text, len, false, utf8, 
                  X(self)-> xft_map8, &ovx) - 1;
      int d  = - PDrawable(self)-> font. descent;
      int ay, x1, y1, x2, y2;
      double c = XX-> xft_font_cos, s = XX-> xft_font_sin;

      XSetFillStyle( DISP, XX-> gc, FillSolid);
      if ( !XX-> flags. brush_fore) {
         XSetForeground( DISP, XX-> gc, XX-> fore. primary);
         XX-> flags. brush_fore = 1;
      }

      if ( lw != 1)
         apc_gp_set_line_width( self, 1);

      if ( PDrawable( self)-> font. style & fsUnderlined) {
         ay = d;
         x1 = x - ovx.x * c - ay * s + 0.5;
         y1 = y - ovx.x * s + ay * c + 0.5;
         tw += ovx.y;
         x2 = x + tw * c - ay * s + 0.5;
         y2 = y + tw * s + ay * c + 0.5;
         XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2)); 
      }

      if ( PDrawable( self)-> font. style & fsStruckOut) {
         ay = (XX-> font-> font.ascent - XX-> font-> font.internalLeading)/2;
         x1 = x - ovx.x * c - ay * s + 0.5;
         y1 = y - ovx.x * s + ay * c + 0.5;
         tw += ovx.y;
         x2 = x + tw * c - ay * s + 0.5;
         y2 = y + tw * s + ay * c + 0.5;
         XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2)); 
      }

      if ( lw != 1) 
         apc_gp_set_line_width( self, lw);
   }  
   XFLUSH;

   return true;
}
                             
static Bool
xft_add_item( unsigned long ** list, int * count, int * size, FcChar32 chr, 
              Bool decrease_count_if_failed)
{
   if ( *count >= *size) {
      unsigned long * newlist = realloc( *list, sizeof( unsigned long) * ( *size *= 2));
      if ( !newlist) {
         if ( decrease_count_if_failed) (*count)--;
         return false;
      }
      *list = newlist;
   }
   (*list)[(*count)++] = ( unsigned long ) chr;
   return true;
}

unsigned long *
prima_xft_get_font_ranges( Handle self, int * count)
{
   FcChar32 ucs4, last = 0, haslast = 0;
   FcChar32 map[FC_CHARSET_MAP_SIZE];
   FcChar32 next;
   FcCharSet *c = X(self)-> font-> xft-> charset;
   int size = 16;
   unsigned long * ret;

#define ADD(ch,flag) if(!xft_add_item(&ret,count,&size,ch,flag)) return ret

   *count = 0;
   if ( !c) return false;
   if ( !( ret = malloc( sizeof( unsigned long) * size))) return nil;

   if ( FcCharSetCount(c) == 0) {
      /* better than nothing */
      ADD( 32, true);
      ADD( 128, false);
      return ret;
   }

   for (ucs4 = FcCharSetFirstPage (c, map, &next);
        ucs4 != FC_CHARSET_DONE;
        ucs4 = FcCharSetNextPage (c, map, &next))
   {
       int i, j;
       for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
           if (map[i]) {
               for (j = 0; j < 32; j++)
                   if (map[i] & (1 << j)) {
                       FcChar32 u = ucs4 + i * 32 + j;
                       if ( haslast) {
                          if ( last != u - 1) {
                             ADD( last,true);
                             ADD( u,false);
                          }
                       } else {
                          ADD( u,false);
                          haslast = 1;
                       }
                       last = u;
                   }
           }
   }
   if ( haslast) ADD( last,true);

   return ret;
}

PFontABC
prima_xft_get_font_abc( Handle self, int firstChar, int lastChar, Bool unicode)
{
   PFontABC abc;
   int i, len = lastChar - firstChar + 1;
   XftFont *font = X(self)-> font-> xft_base;

   if ( !( abc = malloc( sizeof( FontABC) * len))) 
      return nil;

   for ( i = 0; i < len; i++) {
      FcChar32 c = i + firstChar;
      FT_UInt ft_index;
      XGlyphInfo glyph;
      if ( !unicode && c > 128) {
         c = X(self)-> xft_map8[ c - 128];
      }
      ft_index = XftCharIndex( DISP, font, c);
      XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
      abc[i]. a = -glyph. x;
      abc[i]. b = glyph. width;
      abc[i]. c = glyph. xOff - glyph. width + glyph. x;
   }

   return abc;
}

uint32_t *
prima_xft_map8( const char * encoding)
{
   CharSetInfo * csi;
   if ( !( csi = hash_fetch( encodings, encoding, strlen( encoding))))
      csi = locale;
   return csi-> map;
}

Bool
prima_xft_parse( char * ppFontNameSize, Font * font)
{
   FcPattern * p = FcNameParse(( FcChar8*) ppFontNameSize);
   FcCharSet * c = nil;
   Font f, def = guts. default_font;

   bzero( &f, sizeof( Font));
   f. height = f. width = f. size = C_NUMERIC_UNDEF;
   fcpattern2font( p, &f);
   f. width = C_NUMERIC_UNDEF;
   FcPatternGetCharSet( p, FC_CHARSET, 0, &c);
   if ( c && (FcCharSetCount(c) > 0)) {
      int i;
      for ( i = 0; i < MAX_CHARSET; i++) {
          if ( !std_charsets[i]. enabled) continue;
          if ( FcCharSetIntersectCount( std_charsets[i]. fcs, c) >= std_charsets[i]. glyphs - 1) {
             strcpy( f. encoding, std_charsets[i]. name);
             break;
          }
      }
   }
   FcPatternDestroy( p);
   if ( !prima_xft_font_pick( nilHandle, &f, &def, nil)) return false;
   *font = def;
   Fdebug( "parsed ok: %d.%s\n", def.size, def.name);
   return true;
}

void
prima_xft_update_region( Handle self)
{
   DEFXX;
   if ( XX-> xft_drawable) {
      XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
      XX-> flags. xft_clip = 1;
   }
}

#else
#error Required: Xft version 2.1.0 or higher; fontconfig version 2.0.1 or higher. To compile without Xft, re-run 'perl Makefile.PL WITH_XFT=0'
#endif /* USE_XFT */
