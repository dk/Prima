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
/*  System dependent font routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

/*
global %Font {
#   int         height;
#   int         width;
#   int         style;
#   int         pitch;
#   int         direction;
   long        resolution;
#   string      name;
#   int         size;
   int         codepage;
   string      family;
   int         vector;
   int         ascent;
   int         descent;
   int         weight;
   int         maximalWidth;
   int         internalLeading   ;
   int         externalLeading   ;
   int         xDeviceRes        ;
   int         yDeviceRes        ;
   int         firstChar         ;
   int         lastChar          ;
   int         breakChar         ;
   int         defaultChar       ;
}
*/



static PHash xfontCache = nil;
static void detail_font_info( PFontInfo f, PFont font, Bool addToCache, Bool bySize);
static Bool have_vector_fonts = false;

static void
strlwr( char *d, const char *s)
{
   while ( *s) {
      *d++ = tolower( *s++);
   }
   *d = '\0';
}

static void
font_query_name( XFontStruct * s, PFontInfo f)
{
   unsigned long v;
   char * c;
   
   /* detailing family */   
   if ( XGetFontProperty( s, FXA_FAMILY_NAME, &v) && v) {
      XCHECKPOINT;
      c = XGetAtomName( DISP, (Atom)v);
      XCHECKPOINT;
      if ( c) {
         f-> flags. family = true;
         strncpy( f-> font. family, c, 255);  f-> font. family[255] = '\0';
         strlwr( f-> lc_family, f-> font. family);
         XFree( c);
      }
   }
   
   /* detailing name */
   if ( XGetFontProperty( s, FXA_FOUNDRY, &v) && v) {
      XCHECKPOINT;
      c = XGetAtomName( DISP, (Atom)v);
      XCHECKPOINT;
      if ( c) {
         f-> flags. name = true;
         snprintf( f-> font. name, 256, "%s %s", c, f-> font. family);
         strlwr( f-> lc_name, f-> font. name);
         XFree( c);
      }
   }
}   

void
prima_init_font_subsystem( void)
{
   char **names;
   int count, j , i, bad_fonts = 0, vector_fonts = 0;
   PFontInfo info;

   FXA_RESOLUTION_X = XInternAtom( DISP, "RESOLUTION_X", false);
   FXA_RESOLUTION_Y = XInternAtom( DISP, "RESOLUTION_Y", false);
   FXA_PIXEL_SIZE = XInternAtom( DISP, "PIXEL_SIZE", false);
   FXA_SPACING = XInternAtom( DISP, "SPACING", false);
   FXA_RELATIVE_WEIGHT = XInternAtom( DISP, "RELATIVE_WEIGHT", false);
   FXA_FOUNDRY = XInternAtom( DISP, "FOUNDRY", false);
   FXA_AVERAGE_WIDTH = XInternAtom( DISP, "AVERAGE_WIDTH", false);

   guts. font_names = names = XListFonts( DISP, "*", INT_MAX, &count);
   if ( !names) croak( "UAF_001: no X memory");

   info = malloc( sizeof( FontInfo) * ( count + 1));
   if ( !info)
      croak( "UAF_002: no memory");
   bzero( info,  sizeof( FontInfo) * ( count + 1));

   for ( i = 0, j = 0; i < count; i++) {
      char *b, *t, *c = names[ i];
      int nh = 0;
      Bool conformant = 0;
      int style = 0;    /* must become 2 if we know it */
      int vector = 0;   /* must become 5, or 3 if we know it */

      /*
       * The code below tries to deduce several values from the name
       * of a font, which cannot be relied upon (as specified by XLFD).
       *
       * Recognizing the bad side of such practice, I cannot think of any
       * other way to get certain font characteristics we need without
       * loading the font information, which is prohibitively expensive
       * here due to enumeration of all the fonts in the system.
       */

      while (*c) if ( *c++ == '-') nh++;
      c = names[ i];
      /* DOLBUG( "NH: %d\n", nh); */
      if ( nh == 14) {
	 if ( *c == '+') while (*c && *c != '-')  c++;	    /* skip VERSION */
	 /* from now on *c == '-' is true (on this level) for all valid XLFD names */
	 if ( *c == '-') {
	    /* advance through FOUNDRY */
	    ++c; t = info[j]. font. name;
	    while ( *c && *c != '-') { *t++ = *c++; }
	    *t++ = ' ';
	 }
	 if ( *c == '-') {
	    /* advance through FAMILY_NAME */
	    ++c;  b = t;
	    while ( *c && *c != '-') { *t++ = *c++; }
	    *t = '\0';
	    strcpy( info[j]. font. family, b);
	    info[j]. flags. name = true;
	    info[j]. flags. family = true;

	    strlwr( info[j]. lc_family, info[j]. font. family);
	    strlwr( info[j]. lc_name, info[j]. font. name);

/* 	    DOLBUG( "name: %s, family: %s\n", info[j]. font. name, info[j]. font. family); */
	 }

	 if ( *c == '-') {
	    /* advance through WEIGHT_NAME */
	    b = ++c;
	    while ( *c && *c != '-') c++;
	    if ( c-b == 0 ||
		 (c-b == 6 && strncasecmp( b, "medium", 6) == 0) ||
		 (c-b == 7 && strncasecmp( b, "regular", 7) == 0)) {
	       info[j]. font. style = fsNormal;
	       style++;
	       info[j]. font. weight = fwMedium;
	       info[j]. flags. weight = true;
/* DOLBUG( "medium or regular\n"); */
	    } else if ( c-b == 4 && strncasecmp( b, "bold", 4) == 0) {
	       info[j]. font. style = fsBold;
	       style++;
	       info[j]. font. weight = fwBold;
	       info[j]. flags. weight = true;
/* DOLBUG( "bold\n"); */
	    } else if ( c-b == 8 && strncasecmp( b, "demibold", 8) == 0) {
	       info[j]. font. style = fsBold;
	       style++;
	       info[j]. font. weight = fwSemiBold;
	       info[j]. flags. weight = true;
/* DOLBUG( "demibold\n"); */
	    }
	 }
	 if ( *c == '-') {
	    /* advance through SLANT */
	    b = ++c;
	    while ( *c && *c != '-') c++;
	    if ( c-b == 1 && (*b == 'R' || *b == 'r')) {
	       style++;
/* DOLBUG( "r-slant\n"); */
	    } else if ( c-b == 1 && (*b == 'I' || *b == 'i')) {
	       info[j]. font. style |= fsItalic;
	       style++;
/* DOLBUG( "i-slant\n"); */
	    } else if ( c-b == 1 && (*b == 'O' || *b == 'o')) {
	       info[j]. font. style |= fsItalic;   /* XXX Oblique? */
	       style++;
/* DOLBUG( "o-slant\n"); */
	    } else if ( c-b == 2 && (*b == 'R' || *b == 'r') && (b[1] == 'I' || b[1] == 'i')) {
	       info[j]. font. style |= fsItalic;   /* XXX Reverse Italic? */
	       style++;
/* DOLBUG( "ri-slant\n"); */
	    } else if ( c-b == 2 && (*b == 'R' || *b == 'r') && (b[1] == 'O' || b[1] == 'o')) {
	       info[j]. font. style |= fsItalic;   /* XXX Reverse Oblique? */
	       style++;
/* DOLBUG( "ro-slant\n"); */
	    }
	 }
	 if ( *c == '-') {
	    /* advance through SETWIDTH_NAME; just skip it;  XXX */
	    ++c;
	    while ( *c && *c != '-') c++;
	 }
	 if ( *c == '-') {
	    /* advance through ADD_STYLE_NAME; just skip it;  XXX */
	    ++c;
	    while ( *c && *c != '-') c++;
	 }
	 if ( *c == '-') {
	    /* advance through PIXEL_SIZE */
	    c++; b = c;
	    if ( *c != '-')
	       info[j]. font. height = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[j]. font. height) {
		  info[j]. flags. height = true;
/* DOLBUG( "height: %d\n", info[j]. font. height); */
	       } else {
/* DOLBUG( "vector height\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through POINT_SIZE */
	    c++; b = c;
	    if ( *c != '-')
	       info[j]. font. size = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[j]. font. size) {
		  info[j]. flags. size = true;
                  info[j]. font. size  = ( info[j]. font. size < 10) ? 
                       1 : ( info[j]. font. size / 10);
/*  DOLBUG( "size: %d\n", info[j]. font. size); */
	       } else {
/*  DOLBUG( "vector size\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through RESOLUTION_X */
	    c++; b = c;
	    if ( *c != '-')
	       info[j]. font. xDeviceRes = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[j]. font. xDeviceRes) {
/*  DOLBUG( "xres: %d\n", info[j]. font. xDeviceRes); */
		  info[j]. flags. xDeviceRes = true;
	       } else {
/*  DOLBUG( "vector xres\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through RESOLUTION_Y */
	    c++; b = c;
	    if ( *c != '-')
	       info[j]. font. yDeviceRes = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[j]. font. yDeviceRes) {
/*  DOLBUG( "yres: %d\n", info[j]. font. yDeviceRes); */
		  info[j]. flags. yDeviceRes = true;
	       } else {
/*  DOLBUG( "vector yres\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through SPACING */
	    b = ++c;
	    while ( *c && *c != '-') c++;
	    if ( c-b == 1 && (*b == 'p' || *b == 'P')) {
	       info[j]. font. pitch = fpVariable;
	       info[j]. flags. pitch = true;
/*  DOLBUG( "var spacing\n"); */
	    } else if ( c-b == 1 && (*b == 'm' || *b == 'M')) {
	       info[j]. font. pitch = fpFixed;
	       info[j]. flags. pitch = true;
/*  DOLBUG( "fixed spacing\n"); */
	    } else if ( c-b == 1 && (*b == 'c' || *b == 'C')) {
	       info[j]. font. pitch = fpFixed;
	       info[j]. flags. pitch = true;
/*  DOLBUG( "fixed (charcell) spacing\n"); */
	    }
/*  else */
/*  DOLBUG( "unknown spacing\n"); */
	 }
	 if ( *c == '-') {
	    /* advance through AVERAGE_WIDTH */
	    c++; b = c;
	    if ( *c != '-')
	       info[j]. font. width = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[j]. font. width) {
/*  DOLBUG( "avewidth: %d\n", info[j]. font. width); */
		  info[j]. flags. width = true;
                  info[j]. font. width  = ( info[j]. font. width < 10) ? 
                       1 : ( info[j]. font. width / 10);
	       } else {
/*  DOLBUG( "vector avewidth\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through CHARSET_REGISTRY; just skip it;  XXX */
	    ++c;
            if (
                 ( strncasecmp( c, "sunolglyph",  strlen("sunolglyph")) == 0) ||
                 ( strncasecmp( c, "sunolcursor", strlen("sunolcursor")) == 0) ||
                 ( strncasecmp( c, "misc",        strlen("misc")) == 0)
               )
                 info[j]. flags. funky = 1;
            
	    while ( *c && *c != '-') c++;
	 }
	 if ( *c == '-') {
            c++;
            if (( strncasecmp( c, "0",  strlen("0")) == 0))
                 info[j]. flags. funky = 1;
            /* advance through CHARSET_ENCODING; just skip it;  XXX */
	    while ( *c && *c != '-') c++;
	    if ( !*c  && info[j]. flags. pitch && 
		 ( vector == 5 || vector == 3 || 
		 // ( vector == 5 || 
		   ( info[j]. flags. height &&
		     info[j]. flags. size &&
		     info[j]. flags. xDeviceRes &&
		     info[j]. flags. yDeviceRes &&
		     info[j]. flags. width))) {
	       conformant = true;
	       if ( style == 2)
		  info[j]. flags. style = true;

	       if ( vector == 5 || vector == 3) {
		  char pattern[ 1024], *pat = pattern;
		  int dash = 0;
		  info[j]. font. vector = true;
		  info[j]. flags. bad_vector = (vector == 3);

		  c = names[ i];
		  while (*c) {
		     if ( *c == '%') {
			*pat++ = *c;
			*pat++ = *c++;
		     } else if ( *c == '-') {
			dash++;
			*pat++ = *c++;
			switch ( dash) {
			case 9: case 10:
			   if ( vector == 3)
			      break;
			case 7: case 8: case 12:
			   *pat++ = '%';
			   *pat++ = 'd';
			   while (*c && *c != '-') c++;
			   break;
			}
		     } else {
			*pat++ = *c++;
		     }
		  }
		  *pat++ = '\0';
		  info[j]. vecname = malloc( pat - pattern);
		  strcpy( info[j]. vecname, pattern);
	       } else
		  info[j]. font. vector = false;
	       info[j]. flags. vector = true;
	       vector_fonts += info[j]. font. vector;
	    }
	 }
      }
      if ( !conformant) {
/*  DOLBUG( "%s\n", names[j]); */
	 bad_fonts++;
         continue;
      }
      info[j]. xname = names[ i];
      info[j]. flags. sloppy = true; 
      if ( !info[j]. flags. vector) 
         croak("%s: reported have no vector flag\n", names[i]);
      if ( !info[j]. flags. vector && !info[j]. flags. width)  
         croak("%s: reported have no width  flag\n", names[i]);
      j++;
   }
   
   guts. font_info = info;
   guts. n_fonts = j + 1;
   if ( vector_fonts > 0) have_vector_fonts = true;

   guts. font_hash = hash_create();
   xfontCache      = hash_create();

   info[j]. xname = "fixed";
   info[j]. flags. sloppy = true;  
   info[j]. flags. vector = true;  
   detail_font_info( info + j, nil, false, false);
}

void
prima_free_rotated_entry( PCachedFont f)
{
   while ( f-> rotated) {
      PRotatedFont r = f-> rotated;
      while ( r-> length--) {
          if ( r-> map[ r-> length]) {
             prima_free_ximage( r-> map[ r-> length]);
             r-> map[ r-> length] = nilHandle;
          }      
      }   
      f-> rotated = ( PRotatedFont) r-> next;
      XFreeGC( DISP, r-> arena_gc);
      if ( r-> arena) 
         XFreePixmap( DISP, r-> arena);
      if ( r-> arena_bits)
         free( r-> arena_bits);
      free( r);
   }
}   

static Bool
free_rotated_entries( PCachedFont f, int keyLen, void * key, void * dummy)
{
   prima_free_rotated_entry( f);
   free( f);   
   return false;
}   

void
prima_cleanup_font_subsystem( void)
{
   int i;

   if ( guts. font_names)
      XFreeFontNames( guts. font_names);
   if ( guts. font_info) {
      for ( i = 0; i < guts. n_fonts; i++)
	 if ( guts. font_info[i]. vecname)
	    free( guts. font_info[i]. vecname);
      free( guts. font_info);
   }
   guts. font_names = nil;
   guts. n_fonts = 0;
   guts. font_info = nil;

   if ( guts. font_hash) {
      /* XXX destroy load_name first - enumerating */
      hash_first_that( guts. font_hash, free_rotated_entries, nil, nil, nil); 
      hash_destroy( guts. font_hash, false);
      guts. font_hash = nil;
   }

   hash_destroy( xfontCache, false);
   xfontCache = nil;
}

PFont
apc_font_default( PFont f)
{
   static Font font;
   static Bool initialized = false;
   if ( !initialized) {
      strcpy( font. name, "Helvetica");
      font. height = C_NUMERIC_UNDEF;
      font. size = 12;
      font. width = C_NUMERIC_UNDEF;
      font. style = fsNormal;
      font. pitch = fpDefault;
      apc_font_pick( application, &font, &font);
      font. pitch = fpDefault;
      font. style = fsNormal;
      initialized = true;
   }
   *f = font;
   return f;
}

int
apc_font_load( const char* filename)
{
   DOLBUG( "apc_font_load()\n");
   return 0;
}

static void
dump_font( PFont f)
{
   fprintf( stderr, "*** BEGIN FONT DUMP ***\n");
   fprintf( stderr, "height: %d\n", f-> height);
   fprintf( stderr, "width: %d\n", f-> width);
   fprintf( stderr, "style: %d\n", f-> style);
   fprintf( stderr, "pitch: %d\n", f-> pitch);
   fprintf( stderr, "direction: %d\n", f-> direction);
   fprintf( stderr, "name: %s\n", f-> name ? f-> name : "NONAME");
   fprintf( stderr, "size: %d\n", f-> size);
   fprintf( stderr, "*** END FONT DUMP ***\n");
}

typedef struct _FontKey
{
   int height;
   int width;
   int style;
   int pitch;
   char name[ 256];
} FontKey, *PFontKey;

static void
build_font_key( PFontKey key, PFont f, Bool bySize)
{
   bzero( key, sizeof( FontKey));
   key-> height = bySize ? -f-> size : f-> height;
   key-> width = f-> width;
   key-> style = f-> style & ~(fsUnderlined|fsOutline);
   key-> pitch = f-> pitch;
   strcpy( key-> name, f-> name);
}

PCachedFont
prima_find_known_font( PFont font, Bool refill, Bool bySize)
{
   FontKey key;
   PCachedFont kf;

   build_font_key( &key, font, bySize);
   kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey));
   if ( kf && refill) {
      memcpy( font, &kf-> font, sizeof( Font));
   }
   return kf;
}

static void
add_font_to_cache( PFontKey key, PFontInfo f, const char *name, XFontStruct *s, int uPos, int uThinkness)
{
   PCachedFont kf;

   kf = malloc( sizeof( CachedFont));
   if (!kf) {
     no_memory:
      croak( "UAF_003: no memory");
      return;
   }
   bzero( kf, sizeof( CachedFont));
   kf-> load_name = malloc( strlen( name) + 1);
   if ( !kf-> load_name) {
      goto no_memory;
   }
   strcpy( kf-> load_name, name);
   kf-> id = s-> fid;
   kf-> fs = s;
   memcpy( &kf-> font, &f-> font, sizeof( Font));
   kf-> flags = f-> flags;
   kf-> underlinePos = uPos;
   kf-> underlineThickness = uThinkness;
   kf-> refCnt = 0;
   hash_store( guts. font_hash, key, sizeof( FontKey), kf);
}


static void
detail_font_info( PFontInfo f, PFont font, Bool addToCache, Bool bySize)
{
   XFontStruct *s = nil;
   unsigned long v;
   char *c;
   char name[ 1024];
   FontInfo fi;
   PFontInfo of = f;
   int weight, height, size;
   FontKey key;
   Bool askedDefaultPitch;
   int pickable = 0, pickValue = 0;
   
   if ( f-> font. vector) {
      memcpy( &fi, f, sizeof( fi));
      f = &fi;
   }
   
   if ( f-> vecname) {
      if ( bySize) {
         height = 0;
         pickValue = size   = font-> size * 10;
      } else {
         pickValue = height = font-> height;
         size   = 0;
      }   
   }   
    
   if ( f-> vecname) {
      pickable = 1;
PICK_AGAIN:         
      if ( f-> flags. xDeviceRes) {
         /* three fields */
         sprintf( name, f-> vecname, height, size, font-> width * 10);
      } else {
         /* five fields */
         sprintf( name, f-> vecname, height, size, 
                  (int)(guts. resolution. x + 0.5),
                  (int)(guts. resolution. y + 0.5),
                  font-> width * 10);
      }
   } else {
      strcpy( name, f-> xname);
   }
   
   /* printf( "loading %s\n", name); */
   s = hash_fetch( xfontCache, name, strlen( name));
   
   if ( !s) {
      s = XLoadQueryFont( DISP, name);
      XCHECKPOINT;
      if ( !s) {
         if ( !font)
            croak( "UAF_004: font %s load error", name);

         if ( of-> flags. disabled) 
            croak( "UAF_004: font %s pick-up error", name);
         of-> flags. disabled = true;
              
         /* printf( "kill %s\n", name); */
         apc_font_pick( nilHandle, font, font);
         of-> flags. disabled = false;
         return;
      } else {
         hash_store( xfontCache, name, strlen( name), s);
      }   
   }

   if ( addToCache && font) {
      /* trust the slant part of style */
      font-> style = f-> font. style;
   }   

   if ( f-> flags. sloppy || f-> font. vector) {
      int preSize = 1;
      of-> flags. sloppy = false;

      /* detailing y-resolution */
      if ( XGetFontProperty( s, FXA_RESOLUTION_Y, &v) && v) {
         XCHECKPOINT;
         f-> flags. yDeviceRes = true;
         f-> font. yDeviceRes = v;
      }
      /* detailing x-resolution */
      if ( XGetFontProperty( s, FXA_RESOLUTION_X, &v) && v) {
         XCHECKPOINT;
         f-> flags. xDeviceRes = true;
         f-> font. xDeviceRes = v;
      }
      /* detailing point size */
      if ( XGetFontProperty( s, FXA_POINT_SIZE, &v) && v) {
         XCHECKPOINT;
         f-> flags. size = true;
         f-> font. size = ( v < 10) ? 1 : ( v / 10);
         preSize = v;
      } 

      
      f-> flags. height = true;
      f-> font. height = s-> ascent + s-> descent;
         
      /* detailing pixel size */
      /*
      if ( XGetFontProperty( s, FXA_PIXEL_SIZE, &v) && v) {
         XCHECKPOINT;
         f-> flags. height = true;
         f-> font. height = v;
      }*/
      /* if ( !f-> flags. height && f-> flags. size && f-> flags. yDeviceRes) { */
         /* XWS 1990 XLFD p. 570 */
      /*   f-> flags. height = true;
         f-> font. height = (int)(0.5+f-> font. yDeviceRes*f-> font. size/722.7);
      } */

      if ( !f-> flags. xDeviceRes) {
         f-> font. xDeviceRes = guts. resolution. x;
         f-> flags. xDeviceRes = true;
      }   

      if ( !f-> flags. yDeviceRes) {
         f-> font. yDeviceRes = guts. resolution. y;
         f-> flags. yDeviceRes = true;
      } 

      if ( !f-> flags. size) {
         /* assume internalLeading = 0 */
         f-> flags. size = true;
         f-> font. size  = f-> font. height * 72.27 / f-> font. yDeviceRes + 0.5;
         preSize = f-> font. height * 722.7 / f-> font. yDeviceRes + 0.5;
         f-> flags. internalLeading = true;
         f-> font. internalLeading  = 0;
      } else {
         f-> flags. internalLeading = true;
         f-> font. internalLeading  = f-> font. height - (int)(0.5+f-> font. yDeviceRes * preSize / 722.7);
         /* Prima still doesn't define 'font size' term - should it be that value inside
            the font headers or the size related to Drawable's resolution? The following
            line sticks to the second definition. Comment it out if the first one is
            preferable */   
         f-> font. size = (( f-> font. height - f-> font. internalLeading) * 72.27) / guts. resolution. y + 0.5;
      }

      if ( pickable) {
         int * a = bySize ? &size : &height;
         int   b = bySize ? f-> font. size : f-> font. height;
         int   m = bySize ? 10 : 1;
         pickable = 0;
         if ( b != pickValue && b != 0) {
            *a = *a * pickValue / ( b * m) + 0.5;
            if ( *a != pickValue) 
               goto PICK_AGAIN;
         }
      }   
      
      f-> flags. resolution      = true;
      f-> font. resolution       = f-> font. yDeviceRes * 0x10000 + f-> font. xDeviceRes;
      f-> flags. ascent          = true;
      f-> font. ascent           = s-> ascent;
      f-> flags. descent         = true;
      f-> font. descent          = s-> descent;
      f-> flags. defaultChar     = true;
      f-> font. defaultChar      = s-> default_char;
      f-> flags. firstChar       = true;
      f-> font.  firstChar       = s-> min_char_or_byte2;
      f-> flags. lastChar        = true;
      f-> font.  lastChar        = s-> max_char_or_byte2;
      f-> flags. direction       = true;
      f-> font.  direction       = 0;

      /* detailing maximalWidth */
      f-> flags. maximalWidth = true;   
      if ( s-> per_char) {
         int kl = 0, kr = 255, k;
         f-> font. maximalWidth = 0;
         if ( kl < s-> min_char_or_byte2) kl = s-> min_char_or_byte2;
         if ( kr > s-> max_char_or_byte2) kr = s-> max_char_or_byte2;
         for ( k = kl; k <= kr; k++) {
            int x = s-> per_char[k - s-> min_char_or_byte2]. width;
            if ( f-> font. maximalWidth < x)
               f-> font. maximalWidth = x;
         }      
      } else 
         f-> font. maximalWidth = s-> max_bounds. width;

/*      if ( s-> ascent + 0*s-> descent != f-> font. height) 
         croak( "%s: %d <=> %d", f-> font.name, f-> font. height, s-> ascent + s-> descent*0);*/
  
      /* detailing spacing;  can trust if already known */
      if ( !f-> flags. pitch && XGetFontProperty( s, FXA_SPACING, &v) && v) {
         XCHECKPOINT;
         c = XGetAtomName( DISP, (Atom)v);
         XCHECKPOINT;
         if ( c && c[0] && !c[1]) {
            if ( *c == 'p' || *c == 'P') {
               of-> font. pitch = f-> font. pitch = fpVariable;
               of-> flags. pitch = f-> flags. pitch = true;
            } else if ( *c == 'm' || *c == 'M') {
               of-> font. pitch = f-> font. pitch = fpFixed;
               of-> flags. pitch = f-> flags. pitch = true;
            } else if ( *c == 'c' || *c == 'C') {
               of-> font. pitch = f-> font. pitch = fpFixed;
               of-> flags. pitch = f-> flags. pitch = true;
            }
         }
         if (c) {
            XFree( c);
         }
      }
      /* detailing weight (style) */
      if ( XGetFontProperty( s, FXA_RELATIVE_WEIGHT, &v) && v) {
         XCHECKPOINT;
         of-> font. style &= ~fsBold;
         of-> font. style &= ~fsThin;
         f-> font. style &= ~fsBold;
         f-> font. style &= ~fsThin;
         if ( v < 40) {
            f-> font. style |= fsThin;
            of-> font. style |= fsThin;
         } else if ( v >= 60) {
            f-> font. style |= fsBold;
            of-> font. style |= fsBold;
         }
         /* of-> font. style = f-> font. style = 2; */ /* Kakogo hrena??????? XXX */
         of-> flags. weight = f-> flags. weight = true;
         weight = (v + 5)/10;
         if (weight >= 10) weight--;
         if (weight <= 0) weight++;
         of-> font. weight = f-> font. weight = weight;
      }
      /* detailing [average] width */
      if ( XGetFontProperty( s, FXA_AVERAGE_WIDTH, &v) && v) {
         XCHECKPOINT;
         f-> flags. width = true;
         f-> font. width = v / 10;
      }
      
      /* detailing name and family */
      if ( !f-> flags. intNames) {
         char name[256], family[256];
         int i;
         strcpy( name, f-> font. name);
         strcpy( family, f-> font. family);
         font_query_name( s, f);
         f-> flags. intNames = 1;
         for ( i = 0; i < guts. n_fonts; i++) {
            if ( !guts. font_info[i]. flags. intNames &&
                 ( strcmp( guts. font_info[i]. font. name, name) == 0) &&
                 ( strcmp( guts. font_info[i]. font. family, family) == 0)
               ) {
               strcpy( guts. font_info[i]. font. name,   f-> font. name);
               strcpy( guts. font_info[i]. font. family, f-> font. family);
               guts. font_info[i]. flags. name = f-> flags. name;
               guts. font_info[i]. flags. family   = f-> flags. family;
               guts. font_info[i]. flags. intNames = 1;
            }   
         }   
      }   
      


      /* XXX YYY ZZZ */
      /* Things left undetailed for now: */
      /*  slant part of style, vector(ha-ha) */
      /* XXX YYY ZZZ */
      /* Things left undetermined for now: */
      /*  codepage, ascent, descent, extLead, breakCh */
 /* printf("accept:%d.%d.{%d}.%s; res:%d %d\n", f-> font.height, f-> font.size, f-> font. style, f-> font.name, 
       f-> font. xDeviceRes, f-> font. yDeviceRes); */
      
   }   

/*
  FAQ> 	{ "FOUNDRY", 0, atom, 0},
  FAQ> 	{ "WEIGHT_NAME", 0, atom, 0},
  FAQ> 	{ "SLANT", 0, atom, 0},
  FAQ> 	{ "SETWIDTH_NAME", 0, atom, 0},
  FAQ> 	{ "ADD_STYLE_NAME", 0, atom, 0},
  FAQ> 	{ "AVERAGE_WIDTH", 0, average_width, 0},
  FAQ> 	{ "CHARSET_REGISTRY", 0, atom, 0},
  FAQ> 	{ "CHARSET_ENCODING", 0, atom, 0},
  */

   if ( addToCache && font) {
      int underlinePos = 0, underlineThickness = 1;
      
      /* detailing underline things */
      if ( XGetFontProperty( s, XA_UNDERLINE_POSITION, &v) && v) {
         XCHECKPOINT;
         underlinePos = v;
      } else
         underlinePos = - s-> descent + 1;
      
      if ( XGetFontProperty( s, XA_UNDERLINE_THICKNESS, &v) && v) {
         XCHECKPOINT;
         underlineThickness = v;
      } else
         underlineThickness = 1;

      
      build_font_key( &key, font, bySize); 
 /* printf("add to :%d.%d.{%d}.%s\n", f-> font.height, f-> font.size, f-> font. style, f-> font.name); */
      add_font_to_cache( &key, f, name, s, underlinePos, underlineThickness);
      askedDefaultPitch = font-> pitch == fpDefault;
      memcpy( font, &f-> font, sizeof( Font));
      build_font_key( &key, font, false);
      if ( !hash_fetch( guts. font_hash, &key, sizeof( FontKey))) {
         add_font_to_cache( &key, f, name, s, underlinePos, underlineThickness);
      }
      
      if ( askedDefaultPitch && font-> pitch != fpDefault) {
        int pitch = font-> pitch;
        font-> pitch = fpDefault;
        build_font_key( &key, font, false);
        if ( !hash_fetch( guts. font_hash, &key, sizeof( FontKey))) {
           add_font_to_cache( &key, f, name, s, underlinePos, underlineThickness);
        }
        font-> pitch = pitch;
      }
   }
}


static double 
query_diff( PFontInfo fi, PFont f, char * lcname, Bool by_size)
{
   double diff = 0.0;
   if ( fi->  flags. pitch) {
      if ( f-> pitch == fpDefault && fi-> font. pitch == fpFixed) {
         diff += 1.0;
      } else if ( f-> pitch == fpFixed && fi-> font. pitch == fpVariable) {
         diff += 16000.0;
      } else if ( f-> pitch == fpVariable && fi-> font. pitch == fpFixed) {
         diff += 16000.0;
      }
   } else if ( f-> pitch != fpDefault) {
      diff += 10000.0;  /* 2/3 of the worst case */
   }
   
   if ( fi-> flags. name && strcmp( lcname, fi-> lc_name) == 0) {
      diff += 0.0;
   } else if ( fi-> flags. family && strcmp( lcname, fi-> lc_family) == 0) {
      diff += 1000.0;
   } else if ( fi-> flags. family && strstr( fi-> lc_family, lcname)) {
      diff += 2000.0;
   } else if ( !fi-> flags. family) {
      diff += 8000.0;
   } else if ( fi-> flags. name && strstr( fi->  lc_name, lcname)) {
      diff += 7000.0;
   } else {
      diff += 10000.0;
      if ( fi-> flags. funky) diff += 10000.0; 
   }

   if ( fi-> font. vector) {
      if ( fi-> flags. bad_vector) {
         diff += 20.0;    
      }   
   } else {   
      int a, b;
      if ( by_size) {
         a = fi-> font. size;
         b = f-> size;
      } else {
         a = fi-> font. height;
         b = f-> height;
      }  
      if ( a > b) {
         if ( have_vector_fonts) diff += 1000000.0;
         diff += 600.0; 
         diff += 150.0 * (a - b);
      } else if ( a < b) {
         if ( have_vector_fonts) diff += 1000000.0;
         diff += 150.0 * ( b - a);
      }
   } 

   if ( f-> width) {
      if ( fi-> font. vector) {
         if ( fi-> flags. bad_vector) {
            diff += 20.0;    
         }   
      } else {
         if ( fi-> font. width > f-> width) {
            if ( have_vector_fonts) diff += 1000000.0;
            diff += 200.0;
            diff += 50.0 * (fi->  font. width - f-> width); 
         } else if ( fi-> font. width < f-> width) {
            if ( have_vector_fonts) diff += 1000000.0;
            diff += 50.0 * (f-> width - fi->  font. width);
         }   
      }   
   }   
   
   if ( fi->  flags. xDeviceRes && fi-> flags. yDeviceRes) {
      diff += 30.0 * (int)fabs( 0.5 +
         ( 100.0 * guts. resolution. y / guts. resolution. x) -
         ( 100.0 * fi->  font. yDeviceRes / fi->  font. xDeviceRes));
   }

   if ( fi->  flags. yDeviceRes) {
      diff += 1.0 * (int)fabs( guts. resolution. y - fi->  font. yDeviceRes + 0.5);
   }
   if ( fi->  flags. xDeviceRes) {
      diff += 1.0 * (int)fabs( guts. resolution. x - fi->  font. xDeviceRes + 0.5);
   }

   if ( fi-> flags. style && ( f-> style & ~(fsUnderlined|fsOutline))== fi->  font. style) {
      diff += 0.0;
   } else if ( !fi->  flags. style) {
      diff += 2000.0;
   } else {
      diff += 5000.0;
   }
   return diff;
}   

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
   PFontInfo info = guts. font_info;
   int i, n = guts. n_fonts, pickCount = 0, index = -1, lastIndex = -1;
   Bool by_size = Drawable_font_add( self, source, dest);
   double minDiff = INT_MAX, lastDiff = INT_MAX;
   char lcname[ 256];
   Bool underlined = dest-> style & fsUnderlined;
   int  direction  = dest-> direction;
  
   /*
   if ( by_size) {
      printf("reqS:%d.[%d]{%d}.%s\n", dest-> size, dest-> height, dest-> style, dest-> name);
   } else {
      printf("reqH:%d.[%d]{%d}.%s\n", dest-> height, dest-> size, dest-> style, dest-> name);
   }
   */

   if ( prima_find_known_font( dest, true, by_size)) {
      if ( underlined) dest-> style |= fsUnderlined;
      dest-> direction = direction;
      return true;
   }

   strlwr( lcname, dest-> name);
AGAIN:   
   for ( i = 0; i < n; i++) {
      double diff;
      if ( info[i]. flags. disabled) continue;
      diff = query_diff( info + i, dest, lcname, by_size);
      if ( diff < minDiff) {
         lastIndex = index;
         index = i;
         lastDiff = minDiff;
         minDiff = diff;
      }   
      if ( diff < 1) break;
   }

   i = index;

   /* printf( "#0: %d (%g): %s\n", i, minDiff, info[i].xname); */
     
   
   if ( info[ i]. flags. sloppy && pickCount++ < 20) { 
      detail_font_info( info + i, dest, false, by_size); 
      if ( minDiff < query_diff( info + i, dest, lcname, by_size))
         goto AGAIN;
   } 

   /* 
      printf( "took diff %f after %d steps out of %d\n", minDiff, pickCount, n); 
      if ( lastIndex >= 0)
         printf( "#1: %d (%g): %s %d\n", lastIndex, lastDiff, info[lastIndex]. xname, info[lastIndex]. font. vector);
   */
   detail_font_info( info + index, dest, true, by_size);

   if ( underlined) dest-> style |= fsUnderlined;
   dest-> direction = direction;
   return true;
}

PFont
apc_fonts( Handle self, const char *facename, int *retCount)
{
   int i, count = guts. n_fonts;
   PFontInfo info = guts. font_info;
   PFontInfo * table; 
   int n_table, needRecount = 0, maxRecount = 3;
   PFont fmtx;
   Font defaultFont;
   
AGAIN:   
   *retCount = 0;
   n_table = 0;
   
/*   if ( self && kind_of( self, CPrinter)) { 
      return nil;
   }  XXX */


   /* stage 1 - browse through names and validate records */
   table = malloc( count * sizeof( PFontInfo));
   if ( facename == nil) {
      PHash hash = hash_create();
      for ( i = 0; i < count; i++) {
         int len;
         if ( info[ i]. flags. disabled) continue;
         len = strlen( info[ i].font.name);
         if ( hash_fetch( hash, info[ i].font.name, len))
            continue;
         hash_store( hash, info[ i].font.name, len, (void*)1);
         table[ n_table++] = info + i;
      }
      hash_destroy( hash, false);
      *retCount = n_table;
   } else {
      for ( i = 0; i < count; i++) {
         if ( info[ i]. flags. disabled) continue;
         if ( stricmp( info[ i].font.name, facename) == 0) {
            table[ n_table++] = info + i;
         }
      }   
      *retCount = n_table;
   }   

   fmtx = malloc( n_table * sizeof( Font)); 
   
   defaultFont. width  = 0;
   defaultFont. height = 16;
   defaultFont. size   = 0;
      
   for ( i = 0; i < n_table; i++) {
      if ( table[i]-> flags. sloppy) {
         if ( !table[i]-> flags. intNames) needRecount++;
         detail_font_info( table[i], &defaultFont, false, false);
      }   
      fmtx[i] = table[i]-> font;
   }   
   free( table);

   if ( needRecount && --maxRecount) {
      free( fmtx);
      goto AGAIN;
   }   
   
   return fmtx;
}

Bool
apc_gp_set_font( Handle self, PFont font)
{
   DEFXX;
   Bool reload;
   
   PCachedFont kf = prima_find_known_font( font, false, false);
   if ( !kf || !kf-> id) {
      dump_font( font);
      croak( "UAF_007: internal error (kf:%08x)", (uint)kf); /* the font was not cached, can't be */
   }

   reload = XX-> font != kf && XX-> font != nil;

   if ( reload) {
      kf-> refCnt++;
      if ( XX-> font && ( --XX-> font-> refCnt <= 0)) {
         prima_free_rotated_entry( XX-> font);
         XX-> font-> refCnt = 0;
      }   
   }   

   XX-> font = kf;

   if ( XX-> flags. paint) {
      XX-> flags. reload_font = reload;
      // fprintf( stderr, "set font: %s\n", XX-> font-> load_name);
      XSetFont( DISP, XX-> gc, XX-> font-> id);
      XCHECKPOINT;
   }
   
   return true;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
   PMenuSysData selfxx = ((PMenuSysData)(PComponent((self))-> sysData));
   PCachedFont kf;

   font-> direction = 0; /* skip unnecessary logic */
   
   kf = prima_find_known_font( font, false, false);

   if ( !kf || !kf-> id) {
      dump_font( font);
      croak( "UAF_010: internal error (kf:%08x)", (uint)kf); /* the font was not cached, can't be */
   }

   XX-> font = kf;
   return true;
}

Bool
prima_update_rotated_fonts( PCachedFont f, char * text, int len, int direction, PRotatedFont * result)
{
   PRotatedFont * pr = &f-> rotated;
   PRotatedFont r = nil;
   int i;
   
   if ( direction == 0)
      return false;

   /* finding record for given direction */
   while (*pr) {
      if ((*pr)-> direction == direction) {
         r = *pr;
         break;
      }      
      pr = ( PRotatedFont *) &((*pr)-> next);
   }  

   if ( !r) { /* creating startup values for new entry */
      double sin1, cos1, sin2, cos2, rad;
      int    rbox_x[4], rbox_y[4], box_x[4], box_y[4], box[4];
      XGCValues xgv;

      r = *pr = malloc( sizeof( RotatedFont));
      if ( !r) {
         warn("Not enough memory");
         return false;
      }   
      bzero( r, sizeof( RotatedFont));
      r-> direction = direction;
      r-> first   = f-> fs-> min_char_or_byte2; 
      r-> length  = ( f-> fs-> max_char_or_byte2 > 255 ? 255 : f-> fs-> max_char_or_byte2) 
         - r-> first + 1;
      if ( r-> length < 0) r-> length = 0;
      r-> defaultChar = f-> fs-> default_char;
      if ( r-> defaultChar < r-> first || r-> defaultChar >= r-> first + r-> length)
         r-> defaultChar = -1;
         
      if ( r-> length > 0) {
         if ( !( r-> map = malloc( r-> length * sizeof( void*)))) {
            *pr = nil;
            free( r);
            warn("Not enough memory");
            return false;
         }
         bzero( r-> map, r-> length * sizeof( void*));
      }    
      while ( direction < 0) direction += 3600;
      direction %= 3600;
      if ( direction == 0) return false;
      rad = direction * 3.14159 / 1800.0;
      r-> sin. l = ( sin1 = sin( -rad)) * 0x10000;
      r-> cos. l = ( cos1 = cos( -rad)) * 0x10000;
      r-> sin2.l = ( sin2 = sin(  rad)) * 0x10000;
      r-> cos2.l = ( cos2 = cos(  rad)) * 0x10000;
      
/*
   1(0,y)  2(x,y)
   0(0,0)  3(x,0)
*/   
      box_x[0] = box_y[0] = box_x[1] = box_y[3] = 0;
      r-> orgBox. x = box_x[2] = box_x[3] = f-> fs-> max_bounds. width;
      r-> orgBox. y = box_y[1] = box_y[2] = f-> fs-> max_bounds. ascent + f-> fs-> max_bounds. descent;
      for ( i = 0; i < 4; i++) {
         rbox_x[i] = box_x[i] * cos2 - box_y[i] * sin2 + 0.5;
         rbox_y[i] = box_x[i] * sin2 + box_y[i] * cos2 + 0.5;
         box[i] = 0; 
      }   
      for ( i = 0; i < 4; i++) {
         if ( rbox_x[i] < box[0]) box[0] = rbox_x[i];
         if ( rbox_y[i] < box[1]) box[1] = rbox_y[i];
         if ( rbox_x[i] > box[2]) box[2] = rbox_x[i]; 
         if ( rbox_y[i] > box[3]) box[3] = rbox_y[i]; 
      }   
      r-> dimension. x = box[2] - box[0] + 1; 
      r-> dimension. y = box[3] - box[1] + 1; 
      r-> shift. x = box[0];
      r-> shift. y = box[1];
     
      r-> lineSize = (( r-> orgBox. x + 31) / 32) * 4;
      if ( !( r-> arena_bits = malloc( r-> lineSize * r-> orgBox. y)))
         goto FAILED;

      r-> arena = XCreatePixmap( DISP, guts. root, r-> orgBox.x, r-> orgBox. y, 1);  
      if ( !r-> arena) {
         free( r-> arena_bits);
FAILED:         
         *pr = nil;
         free( r-> map);
         free( r);
         warn("Cannot create pixmap");
         return false;
      }   
      XCHECKPOINT;
      r-> arena_gc = XCreateGC( DISP, r-> arena, 0, &xgv);
      XCHECKPOINT;
      XSetFont( DISP, r-> arena_gc, f-> id);
      XCHECKPOINT;
      XSetBackground( DISP, r-> arena_gc, 0);
   }   

   /* processing character records */
   for ( i = 0; i < len; i++) {
      unsigned char index = ( unsigned char) text[i];
      XCharStruct * cs;
      XImage * ximage;
      PrimaXImage * px;
      Byte * ndata;
      
      /* querying character */
      if ( index < r-> first || index >= r-> first + r-> length) {
         if ( r-> defaultChar < 0) continue;
         index = ( unsigned char) r-> defaultChar;
      }   
      if ( r-> map[index]) continue;
      cs = f-> fs-> per_char ? f-> fs-> per_char + index - f-> fs-> min_char_or_byte2 :
         &(f-> fs-> min_bounds);
      XSetForeground( DISP, r-> arena_gc, 0);
      XFillRectangle( DISP, r-> arena, r-> arena_gc, 0, 0, r-> orgBox. x, r-> orgBox .y);
      XSetForeground( DISP, r-> arena_gc, 1);
      // XDrawRectangle( DISP, r-> arena, r-> arena_gc, 0, 0, r-> orgBox. x-1, r-> orgBox .y-1);
      XDrawString( DISP, r-> arena, r-> arena_gc, 
          ( cs-> lbearing < 0) ? -cs-> lbearing : 0, 
          r-> orgBox. y - f-> fs-> descent - 1,
          &index, 1);
      // XDrawLine( DISP, r-> arena, r-> arena_gc, 0, r-> orgBox .y-1, 8, r-> orgBox .y-1);
      XCHECKPOINT;

      /* getting glyph bits */
      ximage = XGetImage( DISP, r-> arena, 0, 0, r-> orgBox. x, r-> orgBox. y, 1, XYPixmap); 
      if ( !ximage) {
         warn("Can't get image");
         return false;
      }   
      XCHECKPOINT;
      prima_copy_xybitmap( r-> arena_bits, ximage-> data, r-> orgBox. x, r-> orgBox. y, 
         r-> lineSize,  ximage-> bytes_per_line);
      XDestroyImage( ximage);
      
      px = prima_prepare_ximage( r-> dimension. x, r-> dimension. y, 1);
      if ( !px) {
         warn( "Can't query image"); 
         return false; 
      }   
      ndata = ( Byte*) px-> data_alias;
      bzero( ndata, px-> bytes_per_line_alias * r-> dimension. y); 
      
      /* rotating */
      {
         int x, y, fast = r-> orgBox. y * r-> orgBox. x > 600;
         Fixed lx, ly;
         Byte * dst = ndata + px-> bytes_per_line_alias * ( r-> dimension. y - 1);

         for ( y = r-> shift. y; y < r-> shift. y + r-> dimension. y; y++) {
            lx. l = r-> shift. x * r-> cos. l - y * r-> sin. l;
            if ( fast)
               lx. l += 0x8000;
            ly. l = r-> shift. x * r-> sin. l + y * r-> cos. l + 0x8000;
            if ( fast) {
               for ( x = 0; x < r-> dimension. x; x++) {
               if ( ly. i. i >= 0 && ly. i. i < r-> orgBox. y &&
                    lx. i. i >= 0 && lx. i. i < r-> orgBox. x) {
                     Byte * src = r-> arena_bits + r-> lineSize * ly. i. i;
                     if ( src[ lx . i. i >> 3] & ( 1 << ( 7 - ( lx . i. i & 7)))) 
                         dst[ x >> 3] |= 1 << ( 7 - ( x & 7));
                  }  
                  lx. l += r-> cos. l;
                  ly. l += r-> sin. l;
               } 
            } else {
               for ( x = 0; x < r-> dimension. x; x++) {
                  if ( ly. i. i >= 0 && ly. i. i < r-> orgBox. y && lx. i. i >= 0 && lx. i. i < r-> orgBox. x) {
                     long pv;
                     Byte * src = r-> arena_bits + r-> lineSize * ly. i. i;
                     pv = 0;
                     if ( src[ lx . i. i >> 3] & ( 1 << ( 7 - ( lx . i. i & 7))))  
                        pv += ( 0x10000 - lx. i. f);
                     if ( lx. i. i < r-> orgBox. x - 1) {
                        if ( src[( lx. i. i + 1) >> 3] & ( 1 << ( 7 - (( lx. i. i + 1) & 7))))  
                           pv += lx. i. f; 
                     } else {
                        if ( src[ lx . i. i >> 3] & ( 1 << ( 7 - ( lx . i. i & 7))))  
                           pv += 0x8000;
                     }   
                     if ( pv >= 0x8000)
                        dst[ x >> 3] |= 1 << ( 7 - ( x & 7)); 
                  } 
                  lx. l += r-> cos. l;
                  ly. l += r-> sin. l;
               }
            }   
            dst -= px-> bytes_per_line_alias;
         }   
      }   

      if ( guts. bit_order != MSBFirst)
         prima_mirror_bytes( ndata, r-> dimension.y * px-> bytes_per_line_alias);
      r-> map[index] = px;
   }   

   if ( result)
      *result = r;
   
   return true;
}   
