#include "unix/guts.h"

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  System dependent font routines (unix, x11)             */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

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

static void
strlwr( char *d, const char *s)
{
   while ( *s) {
      *d++ = tolower( *s++);
   }
   *d = '\0';
}

void
prima_init_font_subsystem( void)
{
   char **names;
   int count, i, bad_fonts = 0, vector_fonts = 0;
   PFontInfo info;
   struct timeval t1, t2;
   double d = 0;

   FXA_RESOLUTION_X = XInternAtom( DISP, "RESOLUTION_X", false);
   FXA_RESOLUTION_Y = XInternAtom( DISP, "RESOLUTION_Y", false);
   FXA_PIXEL_SIZE = XInternAtom( DISP, "PIXEL_SIZE", false);
   FXA_SPACING = XInternAtom( DISP, "SPACING", false);
   FXA_RELATIVE_WEIGHT = XInternAtom( DISP, "RELATIVE_WEIGHT", false);
   FXA_FOUNDRY = XInternAtom( DISP, "FOUNDRY", false);
   FXA_AVERAGE_WIDTH = XInternAtom( DISP, "AVERAGE_WIDTH", false);

   gettimeofday( &t1, nil);
   guts. font_names = names = XListFonts( DISP, "*", 1000000, &count);
   if ( !names)
      croak( "error 1 initializing font subsystem");
   guts. n_fonts = count;

   info = malloc( sizeof( FontInfo) * count);
   if ( !info)
      croak( "error initializing font subsystem: not enough memory");
   bzero( info,  sizeof( FontInfo) * count);

   for ( i = 0; i < count; i++) {
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
/* printf( "NH: %d\n", nh); */
      if ( nh == 14) {
	 if ( *c == '+') while (*c && *c != '-')  c++;	    /* skip VERSION */
	 /* from now on *c == '-' is true (on this level) for all valid XLFD names */
	 if ( *c == '-') {
	    /* advance through FOUNDRY */
	    ++c; t = info[i]. font. name;
	    while ( *c && *c != '-') { *t++ = *c++; }
	    *t++ = ' ';
	 }
	 if ( *c == '-') {
	    /* advance through FAMILY_NAME */
	    ++c;  b = t;
	    while ( *c && *c != '-') { *t++ = *c++; }
	    *t = '\0';
	    strcpy( info[i]. font. family, b);
	    info[i]. flags. name = true;
	    info[i]. flags. family = true;

	    strlwr( info[i]. lc_family, info[i]. font. family);
	    strlwr( info[i]. lc_name, info[i]. font. name);

/* printf( "name: %s, family: %s\n", info[i]. font. name, info[i]. font. family); */
	 }
	 if ( *c == '-') {
	    /* advance through WEIGHT_NAME */
	    b = ++c;
	    while ( *c && *c != '-') c++;
	    if ( c-b == 0 ||
		 (c-b == 6 && strncasecmp( b, "medium", 6) == 0) ||
		 (c-b == 7 && strncasecmp( b, "regular", 7) == 0)) {
	       info[i]. font. style = fsNormal;
	       style++;
	       info[i]. font. weight = fwMedium;
	       info[i]. flags. weight = true;
/* printf( "medium or regular\n"); */
	    } else if ( c-b == 4 && strncasecmp( b, "bold", 4) == 0) {
	       info[i]. font. style = fsBold;
	       style++;
	       info[i]. font. weight = fwBold;
	       info[i]. flags. weight = true;
/* printf( "bold\n"); */
	    } else if ( c-b == 8 && strncasecmp( b, "demibold", 8) == 0) {
	       info[i]. font. style = fsBold;
	       style++;
	       info[i]. font. weight = fwSemiBold;
	       info[i]. flags. weight = true;
/* printf( "demibold\n"); */
	    }
	 }
	 if ( *c == '-') {
	    /* advance through SLANT */
	    b = ++c;
	    while ( *c && *c != '-') c++;
	    if ( c-b == 1 && (*b == 'R' || *b == 'r')) {
	       style++;
/* printf( "r-slant\n"); */
	    } else if ( c-b == 1 && (*b == 'I' || *b == 'i')) {
	       info[i]. font. style |= fsItalic;
	       style++;
/* printf( "i-slant\n"); */
	    } else if ( c-b == 1 && (*b == 'O' || *b == 'o')) {
	       info[i]. font. style |= fsItalic;   /* XXX Oblique? */
	       style++;
/* printf( "o-slant\n"); */
	    } else if ( c-b == 2 && (*b == 'R' || *b == 'r') && (b[1] == 'I' || b[1] == 'i')) {
	       info[i]. font. style |= fsItalic;   /* XXX Reverse Italic? */
	       style++;
/* printf( "ri-slant\n"); */
	    } else if ( c-b == 2 && (*b == 'R' || *b == 'r') && (b[1] == 'O' || b[1] == 'o')) {
	       info[i]. font. style |= fsItalic;   /* XXX Reverse Oblique? */
	       style++;
/* printf( "ro-slant\n"); */
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
	       info[i]. font. height = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[i]. font. height) {
		  info[i]. flags. height = true;
/* printf( "height: %d\n", info[i]. font. height); */
	       } else {
/* printf( "vector height\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through POINT_SIZE */
	    c++; b = c;
	    if ( *c != '-')
	       info[i]. font. size = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[i]. font. size) {
		  info[i]. flags. size = true;
/*  printf( "size: %d\n", info[i]. font. size); */
	       } else {
/*  printf( "vector size\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through RESOLUTION_X */
	    c++; b = c;
	    if ( *c != '-')
	       info[i]. font. xDeviceRes = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[i]. font. xDeviceRes) {
/*  printf( "xres: %d\n", info[i]. font. xDeviceRes); */
		  info[i]. flags. xDeviceRes = true;
	       } else {
/*  printf( "vector xres\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through RESOLUTION_Y */
	    c++; b = c;
	    if ( *c != '-')
	       info[i]. font. yDeviceRes = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[i]. font. yDeviceRes) {
/*  printf( "yres: %d\n", info[i]. font. yDeviceRes); */
		  info[i]. flags. yDeviceRes = true;
	       } else {
/*  printf( "vector yres\n"); */
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
	       info[i]. font. pitch = fpVariable;
	       info[i]. flags. pitch = true;
/*  printf( "var spacing\n"); */
	    } else if ( c-b == 1 && (*b == 'm' || *b == 'M')) {
	       info[i]. font. pitch = fpFixed;
	       info[i]. flags. pitch = true;
/*  printf( "fixed spacing\n"); */
	    } else if ( c-b == 1 && (*b == 'c' || *b == 'C')) {
	       info[i]. font. pitch = fpFixed;
	       info[i]. flags. pitch = true;
/*  printf( "fixed (charcell) spacing\n"); */
	    }
/*  else */
/*  printf( "unknown spacing\n"); */
	 }
	 if ( *c == '-') {
	    /* advance through AVERAGE_WIDTH */
	    c++; b = c;
	    if ( *c != '-')
	       info[i]. font. width = strtol( c, &b, 10);
	    if ( c != b) {
	       if ( info[i]. font. width) {
/*  printf( "avewidth: %d\n", info[i]. font. width); */
		  info[i]. flags. width = true;
	       } else {
/*  printf( "vector avewidth\n"); */
		  vector++;
	       }
	       c = b;
	    }
	 }
	 if ( *c == '-') {
	    /* advance through CHARSET_REGISTRY; just skip it;  XXX */
	    ++c;
	    while ( *c && *c != '-') c++;
	 }
	 if ( *c == '-') {
	    /* advance through CHARSET_ENCODING; just skip it;  XXX */
	    ++c;
	    while ( *c && *c != '-') c++;
	    if ( !*c  && info[i]. flags. pitch && 
		 ( vector == 5 || vector == 3 ||
		   ( info[i]. flags. height &&
		     info[i]. flags. size &&
		     info[i]. flags. xDeviceRes &&
		     info[i]. flags. yDeviceRes &&
		     info[i]. flags. width))) {
	       conformant = true;
	       if ( style == 2)
		  info[i]. flags. style = true;

	       if ( vector == 5 || vector == 3) {
		  char pattern[ 1024], *pat = pattern;
		  int dash = 0;
		  info[i]. font. vector = true;

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
		  info[i]. vecname = malloc( pat - pattern);
		  strcpy( info[i]. vecname, pattern);
	       } else
		  info[i]. font. vector = false;
	       info[i]. flags. vector = true;
	       vector_fonts += info[i]. font. vector;
	    }
	 }
      }
      if ( !conformant) {
	 bzero( info+i, sizeof( FontInfo));
/*  printf( "%s\n", names[i]); */
	 bad_fonts++;
      }
      info[i]. xname = names[ i];
      info[i]. sloppy = true;
   }

   guts. font_info = info;

   gettimeofday( &t2, nil);
   if ( t2. tv_usec < t1. tv_usec) {
      t2. tv_sec++;
   }
   d += ( t2. tv_usec - t1. tv_usec) / 1000000.0;
   d += t2. tv_sec - t1. tv_sec;
   printf( "It took %g seconds to load and parse %d/%d/%d font names\n", d, count, bad_fonts, vector_fonts);

   guts. font_hash = hash_create();

   if (0) {
      int i, cnt = 0;
      FILE *f;
      char **fonts = XListFonts( DISP, "-*-*-*-*-*-*-0-*", 100000, &cnt);
      if ( fonts) {
	 f = fopen( "/tmp/fonts", "w");
	 if ( f) {
	    for ( i = 0; i < cnt; i++) {
	       fprintf( f, "%s\n", fonts[i]);
	    }
	    fclose( f);
	 }
	 XFreeFontNames( fonts);
      }
   }
   if (0) {
      /* -adobe-times-medium-i-normal--0-0-75-75-p-0-iso8859-1 */
      /* -cronyx-courier-medium-r-normal--36-0-100-100-m-0-koi8-r */
      XFontStruct *fs = XLoadQueryFont( DISP, "-adobe-times-medium-i-normal--36-0-75-75-p-0-iso8859-1");
      int i;
      if ( fs) {
	 fprintf( stderr, "@@@ Font Info @@@\n");
	 fprintf( stderr, "@@@ Width: %d\n", fs-> max_bounds. width);
	 fprintf( stderr, "@@@ Font ascent: %d\n", fs-> ascent);
	 fprintf( stderr, "@@@ Font descent: %d\n", fs-> descent);
	 fprintf( stderr, "@@@ Max ascent: %d\n", fs-> max_bounds. ascent);
	 fprintf( stderr, "@@@ Max descent: %d\n", fs-> max_bounds. descent);
	 fprintf( stderr, "@@@ Min ascent: %d\n", fs-> min_bounds. ascent);
	 fprintf( stderr, "@@@ Min descent: %d\n", fs-> min_bounds. descent);
	 fprintf( stderr, "@@@ First char: %d\n", fs-> min_char_or_byte2);
	 fprintf( stderr, "@@@ Last char: %d\n", fs-> max_char_or_byte2);
	 if ( fs-> per_char) {
	    fprintf( stderr, "@@@ Looks like proportional font!\n");
	    for ( i = fs-> min_char_or_byte2; i <= fs-> max_char_or_byte2; i++) {
	       fprintf( stderr, "@@@ char: '%c' a: %hd, d: %hd, w: %hd, lb: %hd, rb: %hd\n",
			i,
			fs-> per_char[ i - fs-> min_char_or_byte2]. ascent,
			fs-> per_char[ i - fs-> min_char_or_byte2]. descent,
			fs-> per_char[ i - fs-> min_char_or_byte2]. width,
			fs-> per_char[ i - fs-> min_char_or_byte2]. lbearing,
			fs-> per_char[ i - fs-> min_char_or_byte2]. rbearing);
	    }
	 } else {
	    fprintf( stderr, "@@@ Looks like monospaced font!\n");
	    fprintf( stderr, "@@@ chars '%c'--'%c' a: %hd, d: %hd, w: %hd, lb: %hd, rb: %hd\n",
		     fs-> min_char_or_byte2,
		     fs-> max_char_or_byte2,
		     fs-> max_bounds. ascent,
		     fs-> max_bounds. descent,
		     fs-> max_bounds. width,
		     fs-> max_bounds. lbearing,
		     fs-> max_bounds. rbearing);
	 }
	 XFreeFont( DISP, fs);
      }
   }

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
      hash_destroy( guts. font_hash, true);
      guts. font_hash = nil;
   }

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
      initialized = true;
   }
   *f = font;
   return f;
}

int
apc_font_load( char * filename)
{
fprintf( stderr, "apc_font_load()\n");
   return 0;
}

static void
dump_font( PFont f)
{
   printf( "*** BEGIN FONT DUMP ***\n");
   printf( "height: %d\n", f-> height);
   printf( "width: %d\n", f-> width);
   printf( "style: %d\n", f-> style);
   printf( "pitch: %d\n", f-> pitch);
   printf( "direction: %d\n", f-> direction);
   printf( "name: %s\n", f-> name ? f-> name : "NONAME");
   printf( "size: %d\n", f-> size);
   printf( "*** END FONT DUMP ***\n");
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
build_font_key( PFontKey key, PFont f)
{
   bzero( key, sizeof( FontKey));
   key-> height = f-> height;
   key-> width = f-> width;
   key-> style = f-> style;
   key-> pitch = f-> pitch;
   strcpy( key-> name, f-> name);
}

static PCachedFont
find_known_font( PFont font, Bool refill)
{
   FontKey key;
   PCachedFont kf;

   build_font_key( &key, font);
   kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey));
   if ( kf && refill) {
      memcpy( font, &kf-> font, sizeof( Font));
   }

   return kf;
}

static void
add_font_to_cache( PFontKey key, PFontInfo f, const char *name, XFontStruct *s)
{
   PCachedFont kf;

   kf = malloc( sizeof( CachedFont));
   if (!kf) {
     no_memory:
      croak( "add_font_to_cache(): not enough memory");
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

   hash_store( guts. font_hash, key, sizeof( FontKey), kf);
}

static void
detail_font_info( PFontInfo f, PFont font)
{
   XFontStruct *s;
   unsigned long v;
   char *c;
   char name[ 1024];
   FontInfo fi;
   PFontInfo of = f;
   Bool vector = f-> flags. vector && f-> font. vector;
   int weight;
   FontKey key;
   Bool detailed = vector && !f-> sloppy;

   if ( f-> sloppy || vector) {
      if ( vector) {
	 memcpy( &fi, f, sizeof( fi));
	 f = &fi;
      }

      printf( "xname: %s\n", f-> xname);
      if ( f-> vecname) {
	 if ( f-> flags. xDeviceRes) {
	    /* three fields */
	    sprintf( name, f-> vecname, font-> height, 0, font-> width);
	 } else {
	    /* five fields */
	    sprintf( name, f-> vecname, font-> height, 0, 
		     (int)(guts. resolution. x + 0.5),
		     (int)(guts. resolution. y + 0.5),
		     font-> width);
	 }
	 printf( "my query: %s\n", name);
      } else {
	 strcpy( name, f-> xname);
      }

      s = XLoadQueryFont( DISP, name);
      XCHECKPOINT;
      if ( !s) {
	 croak( "Error loading font: %s", name);
	 return;
      }

      f-> sloppy = false;

      /* detailing full (x) font name */
      if ( XGetFontProperty( s, XA_FONT, &v) && v) {
	 XCHECKPOINT;
	 c = XGetAtomName( DISP, (Atom)v);
	 XCHECKPOINT;
	 if ( c) {
	    f-> vecname = malloc( strlen( c) + 1);
	    strcpy( f-> vecname, c);
	    f-> flags. name = true;
	    XFree( c);
	 }
      }
      /* detailing family */
      if ( !detailed && XGetFontProperty( s, FXA_FAMILY_NAME, &v) && v) {
	 XCHECKPOINT;
	 c = XGetAtomName( DISP, (Atom)v);
	 XCHECKPOINT;
	 if ( c) {
	    of-> flags. family = f-> flags. family = true;
	    strncpy( f-> font. family, c, 255);  f-> font. family[255] = '\0';
	    strlwr( f-> lc_family, f-> font. family);
	    if ( f != of) {
	       strncpy( of-> font. family, c, 255);  of-> font. family[255] = '\0';
	       strlwr( of-> lc_family, of-> font. family);
	    }
	    XFree( c);
	 }
      }
      /* detailing name */
      if ( !detailed && XGetFontProperty( s, FXA_FOUNDRY, &v) && v) {
	 XCHECKPOINT;
	 c = XGetAtomName( DISP, (Atom)v);
	 XCHECKPOINT;
	 if ( c) {
	    of-> flags. name = f-> flags. name = true;
	    snprintf( f-> font. name, 256, "%s %s", c, f-> font. family);
	    strlwr( f-> lc_name, f-> font. name);
	    if ( f != of) {
	       snprintf( of-> font. name, 256, "%s %s", c, of-> font. family);
	       strlwr( of-> lc_name, of-> font. name);
	    }
	    XFree( c);
	 }
      }
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
	 f-> font. size = v;
      }
      /* detailing pixel size */
      if ( XGetFontProperty( s, FXA_PIXEL_SIZE, &v) && v) {
	 XCHECKPOINT;
	 f-> flags. height = true;
	 f-> font. height = v;
      }
      if (!f-> flags. height && f-> flags. size && f-> flags. yDeviceRes) {
	 /* XWS 1990 XLFD p. 570 */
	 f-> flags. height = true;
	 f-> font. height = (int)(0.5+f-> font. yDeviceRes*f-> font. size/722.7);
      }
      /* detailing spacing;  can trust if already known */
      if ( !detailed && !f-> flags. pitch && XGetFontProperty( s, FXA_SPACING, &v) && v) {
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
      if ( !detailed && XGetFontProperty( s, FXA_RELATIVE_WEIGHT, &v) && v) {
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
	 of-> font. style = f-> font. style = 2; /* XXX */
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
	 f-> font. width = v;
      }

      /* XXX YYY ZZZ */
      /* Things which somtimes need better detailing: */
      /*  [average] width through ABC */

      /* XXX YYY ZZZ */
      /* Things left undetailed for now: */
      /*  slant part of style, vector(ha-ha) */

      /* XXX YYY ZZZ */
      /* Things left undetermined for now: */
      /*  direction, resolution (global), */
      /*  codepage, ascent, descent, maxWidth, intLead, */
      /*  extLead, firstCh, lastCh, breakCh, defCh */

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

   }
   build_font_key( &key, font);
   memcpy( font, &f-> font, sizeof( Font));
   add_font_to_cache( &key, f, name, s);
}

static int
compare_difference( const void *aa, const void *bb)
{
   struct {
      int n;
      double diff;
   } *a = (void*)aa, *b = (void*)bb;

   if ( a-> diff < b-> diff)
      return -1;
   else if ( a-> diff > b-> diff)
      return 1;
   else
      return 0;
}

void
apc_font_pick( Handle self, PFont source, PFont dest)
{
   PFontInfo info = guts. font_info;
   int i, n = guts. n_fonts;
   Bool by_size = Drawable_font_add( self, source, dest);
   double diff;
   PFont f = dest;
   char name[ 256];
   struct {
      int n;
      double diff;
   } *ordered;

   dump_font( dest); /* to prevent warnings */
   if ( !isalpha(dest->name[0])) {
      *((char *)0) = 7;
      croak( "Bad name!");
   }

   if ( by_size) {
      f-> height = (int)( 10.0 * guts. resolution. y * f-> size / 722.7 + 0.5);
   }

   if ( find_known_font( f, true)) {
      return;
   }

   ordered = malloc( sizeof( *ordered) * n);

   strlwr( name, f-> name);

   for ( i = 0; i < n; i++) {
      diff = 0.0;

      if ( info[i]. flags. pitch) {
	 if ( f-> pitch == fpDefault && info[i]. font. pitch == fpFixed) {
	    diff += 1.0;
	 } else if ( f-> pitch == fpFixed && info[i]. font. pitch == fpVariable) {
	    diff += 15000.0;
	 } else if ( f-> pitch == fpVariable && info[i]. font. pitch == fpFixed) {
	    diff += 350.0;
	 }
      } else if ( f-> pitch != fpDefault) {
	 diff += 10000.0;  /* 2/3 of the worst case */
      }

      if ( info[i]. flags. name && strcmp( name, info[i]. lc_name) == 0) {
	 diff += 0.0;
      } else if ( info[i]. flags. family && strcmp( name, info[i]. lc_family) == 0) {
	 diff += 1000.0;
      } else if ( info[i]. flags. family && strstr( info[i]. lc_family, name)) {
	 diff += 2000.0;
      } else if ( !info[i]. flags. family) {
	 diff += 8000.0;
      } else if ( info[i]. flags. name && strstr( info[i]. lc_name, name)) {
	 diff += 7000.0;
      } else {
	 diff += 10000.0;
      }

      if ( !info[i]. flags. vector) {
	 /* baaaad */
      } else if ( info[i]. font. vector) {
      } else {
	 if ( info[i]. font. height > f-> height) {
	    diff += 600.0;
	    diff += 150.0 * (info[i]. font. height - f-> height);
	 }
	 if ( info[i]. font. height < f-> height) {
	    diff += 150.0 * (f-> height - info[i]. font. height);
	 }
      }

      if ( f-> width && info[i]. flags. vector && !info[i]. font. vector) {
	 if ( info[i]. flags. width && info[i]. font. width > f-> width) {
	    diff += 50.0 * (info[i]. font. width - f-> width);
	 } else if ( info[i]. flags. width && info[i]. font. width < f-> width) {
	    diff += 50.0 * (f-> width - info[i]. font. width);
	 } else if ( !info[i]. flags. width) {
	    diff += 50.0 * f-> width;  /* big mismatch; XXX */
	 }
      }

      if ( info[i]. flags. xDeviceRes && info[i]. flags. yDeviceRes) {
	 diff += 30.0 * (int)fabs( 0.5 +
	    ( 100.0 * guts. resolution. y / guts. resolution. x) -
	    ( 100.0 * info[i]. font. yDeviceRes / info[i]. font. xDeviceRes));
      }

      if ( info[i]. flags. yDeviceRes) {
	 diff += 1.0 * (int)fabs( guts. resolution. y - info[i]. font. yDeviceRes + 0.5);
      }
      if ( info[i]. flags. xDeviceRes) {
	 diff += 1.0 * (int)fabs( guts. resolution. x - info[i]. font. xDeviceRes + 0.5);
      }

      if ( info[i]. flags. style && f-> style == info[i]. font. style) {
	 diff += 0.0;
      } else if ( !info[i]. flags. style) {
	 diff += 2000.0;
      } else {
	 diff += 5000.0;
      }

      ordered[i]. n = i;
      ordered[i]. diff = diff;
   }

   qsort( ordered, n, sizeof( *ordered), compare_difference);

   i = ordered[0]. n;
   detail_font_info( info + i, f);

/*
   for ( i = 0; i < 10; i++) {
      printf( "%d (%g): %s\n", i, ordered[i]. diff, info[ordered[i]. n]. xname);
   }
   printf( "................................\n");
   for ( i = n - 10; i < n; i++) {
      printf( "%d (%g): %s\n", i, ordered[i]. diff, info[ordered[i]. n]. xname);
   }

croak( "Ala-ulu");
*/
   free( ordered);
}

PFont
apc_fonts( char *facename, int *retCount)
{
fprintf( stderr, "apc_fonts()\n");
   return nil;
}

void
apc_gp_set_font( Handle self, PFont font)
{
   DEFXX;
   Bool reload = false;

   PCachedFont kf = find_known_font( font, false);
   if ( !kf) {
      Font dest = *font;
      apc_font_pick( self, font, &dest);
      kf = find_known_font( &dest, false);
   }
   if ( !kf) {
      dump_font( font);
      croak( "apc_gp_set_font(): the font was not cached");
   }
   if ( XX-> font == kf) {
      /* extra sanity check */
      if ( !kf-> id) {
	 croak( "apc_gp_set_font(): [internal] already set, no id");
      }
   } else {
      if ( !kf-> id) {
	 kf-> fs = XLoadQueryFont( DISP, kf-> load_name);
	 kf-> id = kf-> fs-> fid;
	 if ( !kf-> fs) {
	    croak( "apc_gp_set_font(): error loading font %s", kf-> load_name);
	 }
      }
      kf-> ref_cnt++;
      if ( XX-> font) {
	 reload = true;
	 if ( --XX-> font-> ref_cnt <= 0) {
	    if ( XX-> font-> id) {
	       XFreeFont( DISP, XX-> font-> fs);
	    }
	    XX-> font-> id = 0;
	    XX-> font-> fs = nil;
	    XX-> font-> ref_cnt = 0;
	 }
      }
      XX-> font = kf;
   }

   if ( XX-> flags. paint) {
      XX-> flags. reloadFont = reload;
      XSetFont( DISP, XX-> gc, XX-> font-> id);
      XCHECKPOINT;
   }
}
