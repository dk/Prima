/***********************************************************/
/*                                                         */
/*  System dependent font routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include <sys/stat.h>
#include <unistd.h>
#include "unix/guts.h"
#include <locale.h>

static PHash xfontCache = nil;
static Bool have_vector_fonts = false;
static PHash encodings = nil;
static char **ignore_encodings;
static int n_ignore_encodings;
static char *s_ignore_encodings;

/* these are freed right after use */
static char * do_default_font = nil;
static char * do_caption_font = nil;
static char * do_msg_font = nil;
static char * do_menu_font = nil;
static char * do_widget_font = nil;
static Bool   do_xft = true;
static Bool   do_core_fonts = true;
static Bool   do_xft_no_antialias = false;
static Bool   do_xft_priority = true;
static Bool   do_no_scaled_fonts = false;
static Bool   do_harfbuzz = true;

static void detail_font_info( PFontInfo f, PFont font, Bool addToCache, Bool bySize);

static void
str_lwr( char *d, const char *s)
{
	while ( *s) {
		*d++ = tolower( *s++);
	}
	*d = '\0';
}

static void
fill_default_font( Font * font )
{
	bzero( font, sizeof( Font));
	strcpy( font-> name, "Default");
	font-> size = 12;
	font-> style = fsNormal;
	font-> pitch = fpDefault;
	font-> vector = fvDefault;
	font-> undef. height = font-> undef. width = font-> undef. vector = 1;
}

/* Extracts font name, charset, foundry etc from X properties, if available.
	Usually it is when X server can access its internal font files directly.
	This means two things:
	- X properties values are not the same as XLFD, and are precise font descriptors
	- alias fonts ( defined via fonts.alias ) don't have X properties
*/
static void
font_query_name( XFontStruct * s, PFontInfo f)
{
	unsigned long v;
	char * c;

	if ( !f-> flags. encoding) {
		c = nil;
		if ( XGetFontProperty( s, FXA_CHARSET_REGISTRY, &v) && v) {
			XCHECKPOINT;
			c = XGetAtomName( DISP, (Atom)v);
			XCHECKPOINT;
			if ( c) {
				f-> flags. encoding = true;
				str_lwr( f-> font. encoding, c);
				XFree( c);
			}
		}

		if ( c) {
			c = nil;
			if ( XGetFontProperty( s, FXA_CHARSET_ENCODING, &v) && v) {
				XCHECKPOINT;
				c = XGetAtomName( DISP, (Atom)v);
				XCHECKPOINT;
				if ( c) {
					strcat( f-> font. encoding, "-");
					str_lwr( f-> font. encoding + strlen( f-> font. encoding), c);
					XFree( c);
				}
			}
		}

		if ( !c) {
			f-> flags. encoding = false;
			f-> font. encoding[0] = 0;
		}
	}

	/* detailing family */
	if ( ! f-> flags. family && XGetFontProperty( s, FXA_FOUNDRY, &v) && v) {
		XCHECKPOINT;
		c = XGetAtomName( DISP, (Atom)v);
		XCHECKPOINT;
		if ( c) {
			f-> flags. family = true;
			strncpy( f-> font. family, c, 255);  f-> font. family[255] = '\0';
			str_lwr( f-> font. family, f-> font. family);
			XFree( c);
		}
	}

	/* detailing name */
	if ( ! f-> flags. name && XGetFontProperty( s, FXA_FAMILY_NAME, &v) && v) {
		XCHECKPOINT;
		c = XGetAtomName( DISP, (Atom)v);
		XCHECKPOINT;
		if ( c) {
			f-> flags. name = true;
			strncpy( f-> font. name, c, 255);  f-> font. name[255] = '\0';
			str_lwr( f-> font. name, f-> font. name);
			XFree( c);
		}
	}

	if ( ! f-> flags. family && ! f-> flags. name) {
		c = f-> xname;
		if ( strchr( c, '-') == NULL) {
			strcpy( f-> font. name, c);
			strcpy( f-> font. family, c);
		} else {
			char * d = c;
			int cnt = 0, lim;
			if ( *d == '-') d++;
			while ( *(c++)) {
				if ( *c == '-' || *(c + 1)==0) {
					if ( c == d ) continue;
					if ( cnt == 0 ) {
						lim = ( c - d > 255 ) ? 255 : c - d;
						strncpy( f-> font. family, d, lim);
						cnt++;
					} else if ( cnt == 1) {
						lim = ( c - d > 255 ) ? 255 : c - d;
						strncpy( f-> font. name, d, lim);
						break;
					} else
						break;
					d = c + 1;
				}
			}

			if (( strlen( f-> font. family) == 0) || (strcmp( f-> font. family, "*") == 0))
				strcpy( f-> font. family, guts. default_font. family);
			if (( strlen( f-> font. name) == 0) || (strcmp( f-> font. name, "*") == 0)) {
				if ( guts. default_font_ok) {
					strcpy( f-> font. name, guts. default_font. name);
				} else {
					Font fx = f-> font;
					fill_default_font( &fx);
					if ( f-> flags. encoding) strcpy( fx. encoding, f-> font. encoding);
					prima_core_font_pick( application, &fx, &fx);
					strcpy( f-> font. name, fx. name);
				}
			} else {
				char c[512];
				snprintf( c, 512, "%s %s", f-> font. family, f-> font. name);
				strncpy( f-> font. name, c, 256);
				f-> font.name[255] = 0;
			}
		}
		str_lwr( f-> font. family, f-> font. family);
		str_lwr( f-> font. name, f-> font. name);
		f-> flags. name = true;
		f-> flags. family = true;
	} else if ( ! f-> flags. family ) {
		str_lwr( f-> font. family, f-> font. name);
		f-> flags. name = true;
	} else if ( ! f-> flags. name ) {
		str_lwr( f-> font. name, f-> font. family);
		f-> flags. name = true;
	}
}

static Bool
xlfd_parse_font( char * xlfd_name, PFontInfo info, Bool do_vector_fonts)
{
	char *b, *t, *c = xlfd_name;
	int nh = 0;
	Bool conformant = 0;
	int style = 0;    /* must become 2 if we know it */
	int vector = 0;   /* must become 5, or 3 if we know it */

        bzero( &info-> font, sizeof(info->font));
        bzero( &info-> flags, sizeof(info->flags));

	info-> flags. sloppy = true;

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
	c = xlfd_name;
	if ( nh == 14) {
		if ( *c == '+') while (*c && *c != '-')  c++;	    /* skip VERSION */
		/* from now on *c == '-' is true (on this level) for all valid XLFD names */
		t = info-> font. family;
		if ( *c == '-') {
			/* advance through FOUNDRY */
			++c;
			while ( *c && *c != '-') { *t++ = *c++; }
			*t++ = '\0';
		}
		if ( *c == '-') {
			/* advance through FAMILY_NAME */
			++c;  b = t;
			while ( *c && *c != '-') { *t++ = *c++; }
			info-> name_offset = c - xlfd_name;
			*t = '\0';
			info-> flags. name = true;
			info-> flags. family = true;
			strcpy( info-> font. name, b);

			if (
				( info-> font.family[0] == '*' && info-> font.family[1] == 0)  ||
				( b[0] == '*' && b[1] == 0)
			) {
				Font xf;
				int noname =  ( b[0] == '*' && b[1] == 0);
				int nofamily = ( info-> font.family[0] == '*' && info-> font.family[1] == 0);
				if ( guts. default_font_ok)
					xf = guts. default_font;
				else
					fill_default_font( &xf);
				if ( !nofamily) strcpy( xf. family, info-> font. family);
				if ( !noname)   strcpy( xf. name, info-> font. name);
				prima_core_font_pick( nilHandle, &xf, &xf);
				if ( noname)   strcpy( info-> font. name,   xf. name);
				if ( nofamily) strcpy( info-> font. family, xf. family);
			}
		}

		if ( *c == '-') {
			/* advance through WEIGHT_NAME */
			b = ++c;
			while ( *c && *c != '-') c++;
			if (
				c-b == 0 ||
		 		(c-b == 6 && strncasecmp( b, "medium", 6) == 0) ||
		 		(c-b == 7 && strncasecmp( b, "regular", 7) == 0)
			) {
				info-> font. style = fsNormal;
				style++;
				info-> font. weight = fwMedium;
				info-> flags. weight = true;
			} else if ( c-b == 4 && strncasecmp( b, "bold", 4) == 0) {
				info-> font. style = fsBold;
				style++;
				info-> font. weight = fwBold;
				info-> flags. weight = true;
			} else if ( c-b == 8 && strncasecmp( b, "demibold", 8) == 0) {
				info-> font. style = fsBold;
				style++;
				info-> font. weight = fwSemiBold;
				info-> flags. weight = true;
			} else if ( c-b == 1 && *b == '*') {
				info-> font. style  = fsNormal;
				style++;
				info-> font. weight = fwMedium;
				info-> flags. weight = true;
			}
		}
		if ( *c == '-') {
			/* advance through SLANT */
			b = ++c;
			while ( *c && *c != '-') c++;
			if ( c-b == 1 && (*b == 'R' || *b == 'r')) {
				style++;
			} else if ( c-b == 1 && (*b == 'I' || *b == 'i')) {
				info-> font. style |= fsItalic;
				style++;
			} else if ( c-b == 1 && (*b == 'O' || *b == 'o')) {
				info-> font. style |= fsItalic;   /* XXX Oblique? */
				style++;
			} else if ( c-b == 2 && (*b == 'R' || *b == 'r') && (b[1] == 'I' || b[1] == 'i')) {
				info-> font. style |= fsItalic;   /* XXX Reverse Italic? */
				style++;
			} else if ( c-b == 2 && (*b == 'R' || *b == 'r') && (b[1] == 'O' || b[1] == 'o')) {
				info-> font. style |= fsItalic;   /* XXX Reverse Oblique? */
				style++;
			}
		}
		if ( *c == '-') {
			/* advance through SETWIDTH_NAME */
			b = ++c;
			while ( *c && *c != '-') c++;
			if ( c-b == 9 && strncasecmp( b, "condensed", 9) == 0) {
				info-> font. style |= fsThin;
				style++;
			}
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
				info-> font. height = strtol( c, &b, 10);
			if ( c != b) {
				if ( info-> font. height) {
					info-> flags. height = true;
				} else {
					vector++;
				}
				c = b;
			} else if ( strncmp( c, "*-", 2) == 0) c++;
		}
		if ( *c == '-') {
			/* advance through POINT_SIZE */
			c++; b = c;
			if ( *c != '-')
				info-> font. size = strtol( c, &b, 10);
			if ( c != b) {
				if ( info-> font. size) {
					info-> flags. size = true;
					info-> font. size  = ( info-> font. size < 10) ?
						1 : ( info-> font. size / 10);
				} else {
					vector++;
				}
				c = b;
			} else if ( strncmp( c, "*-", 2) == 0) c++;
		}
		if ( *c == '-') {
			/* advance through RESOLUTION_X */
			c++; b = c;
			if ( *c != '-')
				info-> font. xDeviceRes = strtol( c, &b, 10);
			if ( c != b) {
				if ( info-> font. xDeviceRes) {
					info-> flags. xDeviceRes = true;
				} else {
					vector++;
				}
				c = b;
			} else if ( strncmp( c, "*-", 2) == 0) c++;
		}
		if ( *c == '-') {
			/* advance through RESOLUTION_Y */
			c++; b = c;
			if ( *c != '-')
				info-> font. yDeviceRes = strtol( c, &b, 10);
			if ( c != b) {
				if ( info-> font. yDeviceRes) {
					info-> flags. yDeviceRes = true;
				} else {
					vector++;
				}
				c = b;
			} else if ( strncmp( c, "*-", 2) == 0) c++;
		}
		if ( *c == '-') {
			/* advance through SPACING */
			b = ++c;
			while ( *c && *c != '-') c++;
			if ( c - b == 1) {
				if ( strchr( "pP", *b)) {
					info-> font. pitch = fpVariable;
					info-> flags. pitch = true;
				} else if ( strchr( "mMcC", *b)) {
					info-> font. pitch = fpFixed;
					info-> flags. pitch = true;
				} else if ( *b == '*') {
					info-> font. pitch = fpDefault;
					info-> flags. pitch = true;
				}
			}
		}
		if ( *c == '-') {
			/* advance through AVERAGE_WIDTH */
			c++; b = c;
			if ( *c != '-')
				info-> font. width = strtol( c, &b, 10);
			if ( c != b) {
				if ( info-> font. width) {
					info-> flags. width = true;
					info-> font. width  = ( info-> font. width < 10) ?
						1 : ( info-> font. width / 10);
				} else {
					vector++;
				}
				c = b;
			} else if ( strncmp( c, "*-", 2) == 0) c++;
		}
		if ( *c == '-') {
			/* advance through CHARSET_REGISTRY;  */
			++c;
			info-> info_offset = c - xlfd_name;
			if ( strchr( c, '*') == nil) {
				info-> flags. encoding = 1;
				strcpy( info-> font. encoding, c);
				hash_store( encodings, c, strlen( c), (void*)1);
			} else
				info-> font. encoding[0] = 0;
			if (
				( strncasecmp( c, "sunolglyph",  strlen("sunolglyph")) == 0) ||
				( strncasecmp( c, "sunolcursor", strlen("sunolcursor")) == 0) ||
				( strncasecmp( c, "misc",        strlen("misc")) == 0)
				)
				info-> flags. funky = 1;

			while ( *c && *c != '-') c++;
		}
		if ( *c == '-') {
			int m;
			c++;
			for (m = 0; m < n_ignore_encodings; m++) {
				if (strcmp(c, ignore_encodings[m]) == 0)
					goto skip_font;
			}
			if (
				(strncmp( c, "0",  strlen("0")) == 0) ||
				(strncmp( c, "fontspecific", strlen("fontspecific")) == 0) ||
				(strncmp( c, "special", strlen("special")) == 0)
				)
				info-> flags. funky = 1;

			/* advance through CHARSET_ENCODING; just skip it;  XXX */
			while ( *c && *c != '-') c++;
			if ( !*c && info-> flags. pitch &&
		      ( !do_vector_fonts || vector == 5 || vector == 3 ||
				( info-> flags. height &&
					info-> flags. size &&
					info-> flags. xDeviceRes &&
					info-> flags. yDeviceRes &&
					info-> flags. width))) {
				conformant = true;
				if ( style == 2) info-> flags. style = true;

				if ( do_vector_fonts && ( vector == 5 || vector == 3)) {
					char pattern[ 1024], *pat = pattern;
					int dash = 0;
					info-> font. vector = fvScalableBitmap;
					if ( guts. no_scaled_fonts) info-> flags. disabled = true;
					info-> flags. bad_vector = (vector == 3);

					c = xlfd_name;
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
					if (( info-> vecname = malloc( pat - pattern)))
						strcpy( info-> vecname, pattern);
				} else
					info-> font. vector = fvBitmap;
				info-> flags. vector = true;
			}
		}
	}
skip_font:
	return conformant;
}

static Bool
pick_default_font_with_encoding(void)
{
	PFontInfo info;
	int i, best = -1, best_weight = 0, max_weight = 5;

	if ( !guts. no_scaled_fonts) max_weight++;
	for ( i = 0, info = guts. font_info; i < guts. n_fonts; i++, info++) {
		if ( strcmp( info-> font. encoding, guts. locale) == 0) {
			int weight = 0;
			if ( info-> font. style == fsNormal) weight++;
			if ( info-> font. weight == fwMedium) weight++;
			if ( info-> font. pitch == fpVariable) weight++;
			if ( info-> font. vector > fvBitmap) weight++;
			if (
				( strcmp( info-> font.name, "helvetica") == 0 ) ||
				( strcmp( info-> font.name, "arial") == 0 )
			)
				weight+=2;
			if (
				( strcmp( info-> font.name, "lucida") == 0 ) ||
				( strcmp( info-> font.name, "verdana") == 0 )
			)
				weight++;
			if ( weight > best_weight) {
				best_weight = weight;
				best = i;
				if ( weight == max_weight) break;
			}
		}
	}

	if ( best >= 0) {
		fill_default_font( &guts. default_font);
		strcpy( guts. default_font. name, guts. font_info[best].font.name);
		strcpy( guts. default_font. encoding, guts. locale);
		prima_core_font_pick( application, &guts. default_font, &guts. default_font);
		guts. default_font. pitch = fpDefault;
		return true;
	}
	return false;
}

Bool
prima_init_font_subsystem( char * error_buf)
{
	char **names;
	int count, j , i, bad_fonts = 0, vector_fonts = 0;
	PFontInfo info;

	if ( do_core_fonts) {
		int nocorefonts = 0;
		apc_fetch_resource( "Prima", "", "Nocorefonts", "nocorefonts",
			nilHandle, frUnix_int, &nocorefonts);
		if ( nocorefonts) do_core_fonts = false;
	}

	guts. font_names = names = XListFonts( DISP,
		do_core_fonts ? "*" : "-*-fixed-*",
		INT_MAX, &count);
	if ( !names) {
		sprintf( error_buf, "No fonts returned by XListFonts, cannot continue");
		return false;
	}

	info = malloc( sizeof( FontInfo) * count);
	if ( !info) {
		sprintf( error_buf, "No memory (%d bytes)", (int)sizeof(FontInfo)*count);
		return false;
	}
	bzero( info,  sizeof( FontInfo) * count);

	n_ignore_encodings = 0;
	ignore_encodings = nil;
	s_ignore_encodings = nil;
	if ( apc_fetch_resource( "Prima", "", "IgnoreEncodings", "ignoreEncodings",
				nilHandle, frString, &s_ignore_encodings))
	{
		char *e = s_ignore_encodings;
		char *s = e;
		while (*e) {
			while (*e && *e != ';') {
				e++;
			}
			if (*e == ';') {
				n_ignore_encodings++;
				*e = '\0';
				s = ++e;
			} else if (e > s) {
				n_ignore_encodings++;
			}
		}
		ignore_encodings = malloc( sizeof( char*) * n_ignore_encodings);
		if ( !ignore_encodings) {
			sprintf( error_buf, "No memory");
			return false;
		}
		bzero( ignore_encodings,  sizeof( char*) * n_ignore_encodings);
		s = s_ignore_encodings;
		for (i = 0; i < n_ignore_encodings; i++) {
			ignore_encodings[i] = s;
			while (*s) s++;
			s++;
		}
	}

	encodings = hash_create();

	apc_fetch_resource( "Prima", "", "Noscaledfonts", "noscaledfonts",
		nilHandle, frUnix_int, &guts. no_scaled_fonts);
	if ( do_no_scaled_fonts) guts. no_scaled_fonts = 1;

	for ( i = 0, j = 0; i < count; i++) {
		if ( xlfd_parse_font( names[i], info + j, true)) {
			vector_fonts += ( info[j]. font. vector == fvBitmap ) ? 0 : 1;
			info[j]. xname = names[ i];
			j++;
		} else
			bad_fonts++;
	}

	guts. font_info = info;
	guts. n_fonts = j;
	if ( vector_fonts > 0) have_vector_fonts = true;

	guts. font_hash = hash_create();
	xfontCache      = hash_create();

	/* locale */
	{
		int len;
		char * s = setlocale( LC_CTYPE, NULL);
		while ( *s && *s != '.') s++;
		while ( *s && *s == '.') s++;
		strncpy( guts. locale, s, 31);
		guts. locale[31] = 0;
		len = strlen( guts. locale);
		if ( !hash_fetch( encodings, guts. locale, len)) {
			str_lwr( guts. locale, guts. locale);
			if ( !hash_fetch( encodings, guts. locale, len) &&
				(
					( strncmp( guts. locale, "iso-", 4) == 0)||
					( strncmp( guts. locale, "iso_", 4) == 0)
				)
				) {
				s = guts. locale + 3;
				while ( *s) {
					*s = s[1];
					s++;
				}
				if ( !hash_fetch( encodings, guts. locale, len - 1))
					guts. locale[0] = 0;
			}
		}
		if ( strcmp( guts. locale, "utf-8" ) == 0 )
			strcpy( guts. locale, "iso10646-1");
	}

	guts. xft_no_antialias = do_xft_no_antialias;
	guts. xft_priority     = do_xft_priority;
#ifdef USE_XFT
	if ( do_xft) prima_xft_init();
#endif
	guts. use_harfbuzz     = do_xft && do_harfbuzz;

	prima_font_pp2font( "fixed", nil);
	Fdebug("font: init\n");
	if ( do_default_font) {
		XrmPutStringResource( &guts.db, "Prima.font", do_default_font);
		prima_font_pp2font( do_default_font, &guts. default_font);
		free( do_default_font);
		do_default_font = nil;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "font",
		nilHandle, frFont, &guts. default_font)) {
		fill_default_font( &guts. default_font);
		apc_font_pick( application, &guts. default_font, &guts. default_font);
		guts. default_font. pitch = fpDefault;
		/*
			Although apc_font_pick() does respect $LANG, it does not always picks
			up a font with the correct encoding here, because we use a hard-coded
			string "Default". Whereas users can set an alias for "Default",
			or set the default font via XRDB:Prima.font option, it is not done by
			default, so here we pick such a font that contains the user-specified
			encoding, and has more or less reasonable metrics.
		*/
		if ( guts. locale[0] && (strcmp( guts. locale, guts. default_font. encoding) != 0)) {
			pick_default_font_with_encoding();
		}
	}
	guts. default_font_ok = 1;
#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding
	Fdebug("default font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_font));
	if ( do_menu_font) {
		XrmPutStringResource( &guts.db, "Prima.menu_font", do_menu_font);
		prima_font_pp2font( do_menu_font, &guts. default_menu_font);
		free( do_menu_font);
		do_menu_font = nil;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "menu_font",
									nilHandle, frFont, &guts. default_menu_font)) {
		memcpy( &guts. default_menu_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("menu font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_menu_font));

	if ( do_widget_font) {
		XrmPutStringResource( &guts.db, "Prima.widget_font", do_widget_font);
		prima_font_pp2font( do_widget_font, &guts. default_widget_font);
		free( do_widget_font);
		do_widget_font = nil;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "widget_font",
									nilHandle, frFont, &guts. default_widget_font)) {
		memcpy( &guts. default_widget_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("widget font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_widget_font));

	if ( do_msg_font) {
		XrmPutStringResource( &guts.db, "Prima.message_font", do_widget_font);
		prima_font_pp2font( do_msg_font, &guts. default_msg_font);
		free( do_msg_font);
		do_msg_font = nil;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "message_font",
									nilHandle, frFont, &guts. default_msg_font)) {
		memcpy( &guts. default_msg_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("msg font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_msg_font));

	if ( do_caption_font) {
		XrmPutStringResource( &guts.db, "Prima.caption_font", do_widget_font);
		prima_font_pp2font( do_caption_font, &guts. default_caption_font);
		free( do_caption_font);
		do_caption_font = nil;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "caption_font",
												nilHandle, frFont, &guts. default_caption_font)) {
		memcpy( &guts. default_caption_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("caption font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_caption_font));

	return true;
}

Bool
prima_font_subsystem_set_option( char * option, char * value)
{
	if ( strcmp( option, "no-core-fonts") == 0) {
		if ( value) warn("`--no-core' option has no parameters");
		do_core_fonts = false;
		return true;
	} else
#ifdef USE_XFT
	if ( strcmp( option, "no-xft") == 0) {
		if ( value) warn("`--no-xft' option has no parameters");
		do_xft = false;
		return true;
	} else
	if ( strcmp( option, "no-aa") == 0) {
		if ( value) warn("`--no-antialias' option has no parameters");
		do_xft_no_antialias = true;
		return true;
	} else
	if ( strcmp( option, "font-priority") == 0) {
		if ( !value) {
			warn("`--font-priority' must be given parameters, either 'core' or 'xft'");
			return false;
		}
		if ( strcmp( value, "core") == 0)
			do_xft_priority = false;
		else if ( strcmp( value, "xft") == 0)
			do_xft_priority = true;
		else
			warn("Invalid value '%s' to `--font-priority' option. Valid are 'core' and 'xft'", value);
		return true;
	} else
#endif
#ifdef WITH_HARFBUZZ
	if ( strcmp( option, "no-harfbuzz") == 0) {
		if ( value) warn("`--no-harfbuzz' option has no parameters");
		do_harfbuzz = false;
		return true;
	} else
#endif
	if ( strcmp( option, "noscaled") == 0) {
		if ( value) warn("`--noscaled' option has no parameters");
		do_no_scaled_fonts = true;
		return true;
	} else
	if ( strcmp( option, "font") == 0) {
		free( do_default_font);
		do_default_font = duplicate_string( value);
		Mdebug( "set default font: %s\n", do_default_font);
		return true;
	} else
	if ( strcmp( option, "menu-font") == 0) {
		free( do_menu_font);
		do_menu_font = duplicate_string( value);
		Mdebug( "set menu font: %s\n", do_menu_font);
		return true;
	} else
	if ( strcmp( option, "widget-font") == 0) {
		free( do_widget_font);
		do_widget_font = duplicate_string( value);
		Mdebug( "set menu font: %s\n", do_widget_font);
		return true;
	} else
	if ( strcmp( option, "msg-font") == 0) {
		free( do_msg_font);
		do_msg_font = duplicate_string( value);
		Mdebug( "set msg font: %s\n", do_msg_font);
		return true;
	} else
	if ( strcmp( option, "caption-font") == 0) {
		free( do_caption_font);
		do_caption_font = duplicate_string( value);
		Mdebug( "set caption font: %s\n", do_caption_font);
		return true;
	}
	return false;
}

void
prima_font_pp2font( char * ppFontNameSize, PFont font)
{
	int i, newEntry = 0, len, dash = 0;
	FontInfo fi;
	XFontStruct * xf;
	Font dummy, def_dummy, *def;
	char buf[512], *p;

	if ( !font) font = &dummy;


	/* check if font is XLFD and ends in -*-*, so we can replace it with $LANG */
	len = strlen( ppFontNameSize);
	for ( i = 0, p = ppFontNameSize; i < len; i++, p++)
		if ( *p == '-') dash++;
	if (( dash == 14) && guts. locale[0] && (strcmp( ppFontNameSize + len - 4, "-*-*") == 0)) {
		memcpy( buf, ppFontNameSize, len - 3);
		buf[ len - 3] = 0;
		strncat( buf, guts. locale, 512 - strlen(buf) - 1);
		buf[511] = 0;
		ppFontNameSize = buf;
		len = strlen( ppFontNameSize);
	}

	/* check if the parsed font already present */
	memset( font, 0, sizeof( Font));
	for ( i = 0; i < guts. n_fonts; i++) {
		if ( strcmp( guts. font_info[i]. xname, ppFontNameSize) == 0) {
			*font = guts. font_info[i]. font;
			return;
		}
	}

	xf = ( XFontStruct * ) hash_fetch( xfontCache, ppFontNameSize, len);

	if ( !xf ) {
		xf = XLoadQueryFont( DISP, ppFontNameSize);
		if ( !xf) {
			Fdebug("font: cannot load %s\n", ppFontNameSize);
			if ( !guts. default_font_ok) {
				fill_default_font( font);
				apc_font_pick( application, font, font);
				font-> pitch = fpDefault;
			}
#ifdef USE_XFT
			if ( !guts. use_xft || !prima_xft_parse( ppFontNameSize, font))
#endif
				if ( font != &guts. default_font)
					*font = guts. default_font;
			return;
		}
		hash_store( xfontCache, ppFontNameSize, len, xf);
		newEntry = 1;
	}

	bzero( &fi, sizeof( fi));
	fi. flags. sloppy = true;
	fi. xname = ppFontNameSize;
	xlfd_parse_font( ppFontNameSize, &fi, false);
	font_query_name( xf, &fi);
	detail_font_info( &fi, font, false, false);
	*font = fi. font;
	if ( guts. default_font_ok) {
		def = &guts. default_font;
	} else {
		fill_default_font( def = &def_dummy);
		apc_font_pick( application, def, def);
	}
	if ( font-> height == 0) font-> height = def-> height;
	if ( font-> size   == 0) font-> size   = def-> size;
	if ( strlen( font-> name) == 0) strcpy( font-> name, def-> name);
	if ( strlen( font-> family) == 0) strcpy( font-> family, def-> family);
	apc_font_pick( application, font, font);
	if (
		( stricmp( font-> family, fi. font. family) == 0) &&
		( stricmp( font-> name, fi. font. name) == 0)
		) newEntry = 0;

	if ( newEntry ) {
		PFontInfo n = realloc( guts. font_info, sizeof( FontInfo) * (guts. n_fonts + 1));
		if ( n) {
			guts. font_info = n;
			fi. font = *font;
			guts. font_info[ guts. n_fonts++] = fi;
		}
	}
	Fdebug("font: %s%s parsed to: %d.[w=%d,s=%d].%s.%s\n",
			newEntry ? "new " : "", ppFontNameSize, DEBUG_FONT((*font)));
}

void
prima_free_rotated_entry( PCachedFont f)
{
	while ( f-> rotated) {
		PRotatedFont r = f-> rotated;
		while ( r-> length--) {
			if ( r-> map[ r-> length]) {
				prima_free_ximage( r-> map[ r-> length]);
				r-> map[ r-> length] = nil;
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

	free(ignore_encodings);
	free(s_ignore_encodings);

	if ( guts. font_hash) {
		hash_first_that( guts. font_hash, (void*)free_rotated_entries, nil, nil, nil);
		hash_destroy( guts. font_hash, false);
		guts. font_hash = nil;
	}

	hash_destroy( xfontCache, false);
	xfontCache = nil;
	hash_destroy( encodings, false);
	encodings = nil;
#ifdef USE_XFT
	prima_xft_done();
#endif

}

PFont
apc_font_default( PFont f)
{
	memcpy( f, &guts. default_font, sizeof( Font));
	return f;
}

int
apc_font_load( Handle self, char* filename)
{
#ifdef USE_XFT
	return prima_xft_load_font(filename);
#else
	return 0;
#endif
}

static void
dump_font( PFont f)
{
	if ( !DISP) return;
	fprintf( stderr, "*** BEGIN FONT DUMP ***\n");
	fprintf( stderr, "height: %d\n", f-> height);
	fprintf( stderr, "width: %d\n", f-> width);
	fprintf( stderr, "style: %d\n", f-> style);
	fprintf( stderr, "pitch: %d\n", f-> pitch);
	fprintf( stderr, "direction: %g\n", f-> direction);
	fprintf( stderr, "name: %s\n", f-> name);
	fprintf( stderr, "family: %s\n", f-> family);
	fprintf( stderr, "size: %d\n", f-> size);
	fprintf( stderr, "*** END FONT DUMP ***\n");
}

void
prima_build_font_key( PFontKey key, PFont f, Bool bySize)
{
	bzero( key, sizeof( FontKey));
	key-> height = bySize ? -f-> size : f-> height;
	key-> width = f-> width;
	key-> style = f-> style & ~(fsUnderlined|fsOutline|fsStruckOut) & fsMask;
	key-> pitch = f-> pitch & fpMask;
	key-> direction = 0;
	strcpy( key-> name, f-> name);
	strcat( key-> name, "\1");
	strcat( key-> name, f-> encoding);
}

PCachedFont
prima_find_known_font( PFont font, Bool refill, Bool bySize)
{
	FontKey key;
	PCachedFont kf;

	prima_build_font_key( &key, font, bySize);
	kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey));
	if ( kf && refill) {
		memcpy( font, &kf-> font, sizeof( Font));
	}
	return kf;
}

static Bool
add_font_to_cache( PFontKey key, PFontInfo f, const char *name, XFontStruct *s, int uPos, int uThinkness)
{
	PCachedFont kf;

	kf = malloc( sizeof( CachedFont));
	if (!kf) {
		warn( "no memory");
		return false;
	}
	bzero( kf, sizeof( CachedFont));
	kf-> id = s-> fid;
	kf-> fs = s;
	memcpy( &kf-> font, &f-> font, sizeof( Font));
	kf-> font. style &= ~(fsUnderlined|fsOutline|fsStruckOut) & fsMask;
	kf-> font. pitch &= fpMask;
	kf-> flags = f-> flags;
	kf-> underlinePos = uPos;
	kf-> underlineThickness = uThinkness;
	kf-> refCnt = 0;
	hash_store( guts. font_hash, key, sizeof( FontKey), kf);
	return true;
}

void
prima_init_try_height( HeightGuessStack * p, int target, int firstMove )
{
	p-> locked = 0;
	p-> sp     = 1;
	p-> target = target;
	p-> xlfd[0] = firstMove;
}

int
prima_try_height( HeightGuessStack * p, int height)
{
	int ret = -1;

	if ( p-> locked != 0) return p-> locked;
	if ( p-> sp >= MAX_HGS_SIZE) goto DONT_ADVISE;

	if ( p-> sp > 1) {
		int x0 = p-> xlfd[ p-> sp - 2];
		int x1 = p-> xlfd[ p-> sp - 1];
		int y0 = p-> prima[ p-> sp - 2];
		int y1 = p-> prima[ p-> sp - 1] = height;
		int d0 = y0 - p-> target;
		int d1 = y1 - p-> target;
		if (( d0 < 0 && d1 < 0 && d1 >= d0) || ( d0 > 0 && d1 > 0 && d1 <= d0)) { /* underflow */
			int sp = p-> sp - 2;
			while ( d1 == d0 && sp-- > 0) { /* not even moved */
				x0 = p-> xlfd[ sp];
				y0 = p-> prima[ sp];
				d0 = y0 - p-> target;
			}
			if ( d1 != d0) {
				ret = x0 + ( x1 - x0) * ( p-> target - y0) / ( y1 - y0) + 0.5;
				if ( ret == x1 ) ret += ( d1 < 0) ? 1 : -1;
				if ( ret < 1 ) ret = 1;
			}
		} else if ((( d0 < 0 && d1 > 0) || ( d0 > 0 && d1 < 0)) && ( abs(x0 - x1) > 1)) { /* overflow */
			ret = x0 + ( x1 - x0) * ( p-> target - y0) / ( y1 - y0) + 0.5;
			if ( ret == x0 ) ret += ( d0 < 0) ? 1 : -1; else
			if ( ret == x1 ) ret += ( d1 < 0) ? 1 : -1;
			if ( ret < 1 ) ret = 1;
		}
		/* printf("-- [%d=>%d],[%d=>%d] (%d %d)\n", x0, y0, x1, y1, d0, d1); */
	} else {
		if ( height > 0) /* sp == 0 */
			ret = p-> xlfd[0] * p-> target / height;
		p-> prima[ p-> sp - 1] = height;
	}

	if ( ret > 0) {
		int i;
		for ( i = 0; i < p-> sp; i++)
			if ( p-> xlfd[ i] == ret) { /* advised already? */
				ret = -1;
				break;
			}
	}

	p-> xlfd[ p-> sp] = ret;

	p-> sp++;

DONT_ADVISE:
	if ( ret < 0) { /* select closest match */
		int i, best_i = -1, best_diff = INT_MAX, diff, sp = p-> sp - 1;
		for ( i = 0; i < sp; i++) {
			diff = p-> target - p-> prima[i];
			if ( diff < 0) diff += 1000;
			if ( best_diff > diff) {
				if ( p-> xlfd[i] < 0 ) continue;
				best_diff = diff;
				best_i = i;
			}
		}
		if ( best_i >= 0 && p-> xlfd[best_i] != p-> xlfd[ sp - 1])
			ret = p-> xlfd[best_i];
		p-> locked = -1;
	}

	return ret;
}

static void
detail_font_info( PFontInfo f, PFont font, Bool addToCache, Bool bySize)
{
	XFontStruct *s = nil;
	unsigned long v;
	char name[ 1024];
	FontInfo fi;
	PFontInfo of = f;
	int height = 0, size = 0;
	FontKey key;
	Bool askedDefaultPitch;
	HeightGuessStack hgs;

	if ( f-> vecname) {
		if ( bySize)
			size = font-> size * 10;
		else {
			height = font-> height * 10;
			prima_init_try_height( &hgs, height, f-> flags. heights_cache ? f-> heights_cache[0] : height);
			if ( f-> flags. heights_cache)
				height = prima_try_height( &hgs, f-> heights_cache[1]);
		}
	}

AGAIN:
	if ( f-> vecname) {
		memmove( &fi, f, sizeof( fi));
		fi. flags. size = fi. flags. height = fi. flags. width = fi. flags. ascent =
			fi. flags. descent = fi. flags. internalLeading = fi. flags. externalLeading = 0;
		f = &fi;

		if ( f-> flags. bad_vector) {
			/* three fields */
			sprintf( name, f-> vecname, height / 10, size, font-> width * 10);
		} else {
			/* five fields */
			sprintf( name, f-> vecname, height / 10, size, 0, 0, font-> width * 10);
		}
		Fdebug("font: construct h=%g, s=%d\n", (float)height/10, size);
	} else {
		strcpy( name, f-> xname);
	}

	Fdebug( "font: loading %s\n", name);
	s = hash_fetch( xfontCache, name, strlen( name));

	if ( !s) {
		s = XLoadQueryFont( DISP, name);
		XCHECKPOINT;
		if ( !s) {
			if ( !font)
				warn( "font %s load error", name);
			if ( of-> flags. disabled)
				warn( "font %s pick-up error", name);
			of-> flags. disabled = true;

			Fdebug( "font: kill %s\n", name);
			if ( font) apc_font_pick( nilHandle, font, font);
			of-> flags. disabled = false;
			return;
		} else {
			hash_store( xfontCache, name, strlen( name), s);
		}
	}

	if ( f-> flags. sloppy || f-> vecname) {
		/* check first if height is o.k. */
		int h = s-> max_bounds. ascent + s-> max_bounds. descent;
		if ( h > 0 ) /* empty fonts like 'space' or just downright broken fonts */
			f-> font. height = h;
		f-> flags. height = true;
		if ( f-> vecname && !bySize && f-> font. height != font-> height) {
			int h = prima_try_height( &hgs, f-> font. height * 10);
			Fdebug("font height pick: %d::%d => %d, advised %d\n", hgs.sp-1, font-> height, f-> font. height, h);
			if ( h > 9) {
				if ( !of-> flags. heights_cache) {
					of-> heights_cache[0] = font-> height * 10;
					of-> heights_cache[1] = f-> font. height;
				}
				height = h;
				goto AGAIN;
			}
		}

		/* detailing y-resolution */
		if ( !f-> flags. yDeviceRes) {
			if ( XGetFontProperty( s, FXA_RESOLUTION_Y, &v) && v) {
				XCHECKPOINT;
				f-> font. yDeviceRes = v;
			} else {
				f-> font. yDeviceRes = 72;
			}
			f-> flags. yDeviceRes = true;
		}

		/* detailing x-resolution */
		if ( !f-> flags. xDeviceRes) {
			if ( XGetFontProperty( s, FXA_RESOLUTION_X, &v) && v) {
				XCHECKPOINT;
				f-> font. xDeviceRes = v;
			} else {
				f-> font. xDeviceRes = 72;
			}
			f-> flags. xDeviceRes = true;
		}

		/* detailing internal leading */
		if ( !f-> flags. internalLeading) {
			if ( XGetFontProperty( s, FXA_CAP_HEIGHT, &v) && v) {
				XCHECKPOINT;
				f-> font. internalLeading = s-> max_bounds. ascent - v;
			} else {
				if ( f-> flags. height) {
					f-> font. internalLeading = s-> max_bounds. ascent + s-> max_bounds. descent - f-> font. height;
				} else if ( f-> flags. size) {
					f-> font. internalLeading = s-> max_bounds. ascent + s-> max_bounds. descent -
						( f-> font. size * f-> font. yDeviceRes) / 72.27 + 0.5;
				} else {
					f-> font. internalLeading = 0;
				}
			}
			f-> flags. internalLeading = true;
		}

		/* detailing point size and height */
		if ( bySize) {
			if ( f-> vecname)
				f-> font. size = size / 10;
			else if ( !f-> flags. size)
				f-> font. size = font-> size;
		} else {

			if ( f-> vecname && f-> font. height < font-> height) { /* adjust anyway */
				f-> font. internalLeading += font-> height - f-> font. height;
				f-> font. height = font-> height;
			}

			if ( !f-> flags. size) {
				if ( XGetFontProperty( s, FXA_POINT_SIZE, &v) && v) {
					XCHECKPOINT;
					f-> font. size = ( v < 10) ? 1 : ( v / 10);
				} else
					f-> font. size = ( f-> font. height - f-> font. internalLeading) * 72.27 / f-> font. height + 0.5;
			}
		}
		f-> flags. size = true;

		/* misc stuff */
		f-> flags. resolution      = true;
		f-> font. resolution       = f-> font. yDeviceRes * 0x10000 + f-> font. xDeviceRes;
		f-> flags. ascent          = true;
		f-> font. ascent           = f-> font. height - s-> max_bounds. descent;
		f-> flags. descent         = true;
		f-> font. descent          = s-> max_bounds. descent;
		f-> flags. defaultChar     = true;
		f-> font. defaultChar      = s-> default_char;
		f-> flags. firstChar       = true;
		f-> font.  firstChar       = s-> min_byte1 * 256 + s-> min_char_or_byte2;
		f-> flags. lastChar        = true;
		f-> font.  lastChar        = s-> max_byte1 * 256 + s-> max_char_or_byte2;
		f-> flags. direction       = true;
		f-> font.  direction       = 0;
		f-> flags. externalLeading = true;
		f-> font.  externalLeading =
						abs( s-> max_bounds. ascent  - s-> ascent) +
						abs( s-> max_bounds. descent - s-> descent);
		f-> font.  utf8_flags      = 0;

		/* detailing width */
		if ( f-> font. width == 0 || !f-> flags. width) {
			 if ( f-> vecname && font-> width > 0) {
				f-> font. width = font-> width;
				Fdebug("font: width = copy as is %d\n", f->font.width);
			} else if ( XGetFontProperty( s, FXA_AVERAGE_WIDTH, &v) && v) {
				XCHECKPOINT;
				f-> font. width = (v + 5) / 10;
				Fdebug("font: width = FXA_AVERAGE_WIDTH %d(%d)\n", f->font.width, v);
			} else {
				f-> font. width = s-> max_bounds. width;
				Fdebug("font: width = max_width %d\n", f->font.width);
			}
		}
		f-> flags. width = true;

		/* detailing maximalWidth */
		if ( !f-> flags. maximalWidth) {
			f-> flags. maximalWidth = true;
			if ( s-> per_char) {
				int kl = 0, kr = 255, k;
				int rl = 0, rr = 255, r, d;
				f-> font. maximalWidth = 0;
				if ( rl < s-> min_byte1) rl = s-> min_byte1;
				if ( rr > s-> max_byte1) rr = s-> max_byte1;
				if ( kl < s-> min_char_or_byte2) kl = s-> min_char_or_byte2;
				if ( kr > s-> max_char_or_byte2) kr = s-> max_char_or_byte2;
				d = kr - kl + 1;
				for ( r = rl; r <= rr; r++)
					for ( k = kl; k <= kr; k++) {
						int x = s-> per_char[( r - s-> min_byte1) * d + k - s-> min_char_or_byte2]. width;
						if ( f-> font. maximalWidth < x)
							f-> font. maximalWidth = x;
					}
			} else {
				f-> font. width = f-> font. maximalWidth = s-> max_bounds. width;
				Fdebug("font: force width = max_width %d\n", f->font.width);
			}
		}
		of-> flags. sloppy = false;
	}

	if ( addToCache && font) {
		/* detailing stuff */
		int underlinePos = 0, underlineThickness = 1;

		/* trust the slant part of style */
		font-> style = f-> font. style;

		/* detailing underline things */
		if ( XGetFontProperty( s, XA_UNDERLINE_POSITION, &v) && v) {
			XCHECKPOINT;
			underlinePos =  -s-> max_bounds. descent + v;
		} else
			underlinePos = - s-> max_bounds. descent + 1;

		if ( XGetFontProperty( s, XA_UNDERLINE_THICKNESS, &v) && v) {
			XCHECKPOINT;
			underlineThickness = v;
		} else
			underlineThickness = 1;

		underlinePos -= underlineThickness;
		if ( -underlinePos + underlineThickness / 2 > s-> max_bounds. descent)
			underlinePos = -s-> max_bounds. descent + underlineThickness / 2;

		prima_build_font_key( &key, font, bySize);
		Fdebug("font cache add: %d(%d)x%d.%s.%s %s/%s\n", f-> font.height, f-> font.size, f->font.width, _F_DEBUG_STYLE(f-> font. style), _F_DEBUG_PITCH(f->font. pitch), f-> font.name, f-> font.encoding);
		if ( !add_font_to_cache( &key, f, name, s, underlinePos, underlineThickness))
			return;
		askedDefaultPitch = font-> pitch == fpDefault;
		memcpy( font, &f-> font, sizeof( Font));
		prima_build_font_key( &key, font, false);
		if ( !hash_fetch( guts. font_hash, &key, sizeof( FontKey))) {
			if ( !add_font_to_cache( &key, f, name, s, underlinePos, underlineThickness))
				return;
		}

		if ( askedDefaultPitch && font-> pitch != fpDefault) {
			int pitch = font-> pitch;
			font-> pitch = fpDefault;
			prima_build_font_key( &key, font, false);
			if ( !hash_fetch( guts. font_hash, &key, sizeof( FontKey))) {
				if ( !add_font_to_cache( &key, f, name, s, underlinePos, underlineThickness))
					return;
			}
			font-> pitch = pitch;
		}
	}
}

#define QUERYDIFF_BY_SIZE        (-1)
#define QUERYDIFF_BY_HEIGHT      (-2)

static double
query_diff( PFontInfo fi, PFont f, char * lcname, int selector)
{
	double diff = 0.0;
	int enc_match = 0;

	if ( fi-> flags. encoding && f-> encoding[0]) {
		if ( strcmp( f-> encoding, fi-> font. encoding) != 0)
			diff += 16000.0;
		else
			enc_match = 1;
	}

	if ( guts. locale[0] && !f-> encoding[0] && fi-> flags. encoding) {
		if ( strcmp( fi-> font. encoding, guts. locale) != 0)
			diff += 50;
	}

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

	if ( fi-> flags. name && stricmp( lcname, fi-> font. name) == 0) {
		diff += 0.0;
	} else if ( fi-> flags. family && stricmp( lcname, fi-> font. family) == 0) {
		diff += 1000.0;
	} else if ( fi-> flags. family && strcasestr( fi-> font. family, lcname)) {
		diff += 2000.0;
	} else if ( !fi-> flags. family) {
		diff += 8000.0;
	} else if ( fi-> flags. name && strcasestr( fi-> font. name, lcname)) {
		diff += 7000.0;
	} else {
		diff += 10000.0;
		if ( fi-> flags. funky && !enc_match) diff += 10000.0;
	}

	if ( fi-> font. vector > fvBitmap) {
		if ( fi-> flags. bad_vector) {
			diff += 20.0;
		}
	} else {
		int a, b;
		switch ( selector) {
		case QUERYDIFF_BY_SIZE:
			a = fi-> font. size;
			b = f-> size;
			break;
		case QUERYDIFF_BY_HEIGHT:
			a = fi-> font. height;
			b = f-> height;
			break;
		default:
			a = fi-> font. height;
			b = fi-> flags. sloppy ? selector : f-> height;
			break;
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
		if ( fi-> font. vector > fvBitmap ) {
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
			(float)( 100.0 * guts. resolution. y / guts. resolution. x) -
			(float)( 100.0 * fi->  font. yDeviceRes / fi->  font. xDeviceRes));
	}

	if ( fi->  flags. yDeviceRes) {
		diff += 1.0 * abs( guts. resolution. y - fi->  font. yDeviceRes);
	}
	if ( fi->  flags. xDeviceRes) {
		diff += 1.0 * abs( guts. resolution. x - fi->  font. xDeviceRes);
	}

	if ( fi-> flags. style && ( f-> style & ~(fsUnderlined|fsOutline|fsStruckOut))== fi->  font. style) {
		diff += 0.0;
	} else if ( !fi->  flags. style) {
		diff += 2000.0;
	} else {
		if (( f-> style & fsThin) != ( fi-> font. style & fsThin))
			diff += 1500.0;
		if (( f-> style & fsBold) != ( fi-> font. style & fsBold))
			diff += 3000.0;
		if (( f-> style & fsItalic) != ( fi-> font. style & fsItalic))
			diff += 3000.0;
	}
	return diff;
}

Bool
prima_core_font_pick( Handle self, PFont source, PFont dest)
{
	PFontInfo info = guts. font_info;
	int i, n = guts. n_fonts, index, lastIndex;
	Bool by_size = Drawable_font_add( self, source, dest);
	int query_type = by_size ? QUERYDIFF_BY_SIZE : QUERYDIFF_BY_HEIGHT;
	double minDiff;
	char lcname[ 256];
	Bool underlined = dest-> style & fsUnderlined;
	Bool struckout  = dest-> style & fsStruckOut;
	int  direction  = dest-> direction;
	HeightGuessStack hgs;

	if ( n == 0) return false;

	if ( strcmp( dest-> name, "Default") == 0)
		strcpy( dest-> name, "helvetica");

	if ( prima_find_known_font( dest, true, by_size)) {
		if ( underlined) dest-> style |= fsUnderlined;
		if ( struckout) dest-> style |= fsStruckOut;
		dest-> direction = direction;
		return true;
	}

	if ( by_size) {
		Fdebug("font reqS:%d(h=%d)x%d.%s.%s %s/%s\n", dest-> size, dest-> height, dest->width, _F_DEBUG_STYLE(dest-> style), _F_DEBUG_PITCH(dest-> pitch), dest-> name, dest-> encoding);
	} else {
		Fdebug("font reqH:%d(s=%d)x%d.%s.%s %s/%s\n", dest-> height, dest-> size, dest->width, _F_DEBUG_STYLE(dest-> style), _F_DEBUG_PITCH(dest-> pitch), dest-> name, dest-> encoding);
	}

	if ( !hash_fetch( encodings, dest-> encoding, strlen( dest-> encoding)))
		dest-> encoding[0] = 0;

	if ( !by_size) prima_init_try_height( &hgs, dest-> height, dest-> height);

	str_lwr( lcname, dest-> name);
AGAIN:
	index = lastIndex = -1;
	minDiff = INT_MAX;
	for ( i = 0; i < n; i++) {
		double diff;
		if ( info[i]. flags. disabled) continue;
		diff = query_diff( info + i, dest, lcname, query_type);
		if ( diff < minDiff) {
			lastIndex = index;
			index = i;
			minDiff = diff;
		}
		if ( diff < 1) break;
	}

	i = index;

	Fdebug("font: #%d (diff=%g): %s\n", i, minDiff, info[i].xname);
	Fdebug("font: pick:%d(%d)x%d.%s.%s %s/%s %s.%s\n", info[i].font. height, info[i].font. size, info[i].font. width, _F_DEBUG_STYLE(info[i].font. style), _F_DEBUG_PITCH(info[i].font. pitch), info[i].font. name, info[i].font. encoding, info[i].flags.sloppy?"sloppy":"", info[i].vecname?"vector":"");

	if ( !by_size && info[ i]. flags. sloppy && !info[ i]. vecname) {
		detail_font_info( info + i, dest, false, by_size);
		if ( minDiff < query_diff( info + i, dest, lcname, by_size)) {
			int h = prima_try_height( &hgs, info[i]. font. height);
			if ( h > 0) {
				query_type = h;
				goto AGAIN;
			}
		}
	}

	detail_font_info( info + index, dest, true, by_size);

	if ( underlined) dest-> style |= fsUnderlined;
	if ( struckout) dest-> style |= fsStruckOut;
	dest-> direction = direction;
	return true;
}

Bool
prima_core_font_encoding( char * encoding)
{
	return hash_fetch( encodings, encoding, strlen( encoding)) != NULL;
}

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
#ifdef USE_XFT
	if ( guts. use_xft) {
		if ( prima_xft_font_pick( self, source, dest, nil, nil))
			return true;
	}
#endif
	return prima_core_font_pick( self, source, dest);
}


static PFont
spec_fonts( int *retCount)
{
	int i, count = guts. n_fonts;
	PFontInfo info = guts. font_info;
	PFont fmtx = nil;
	List list;
	PHash hash = nil;

	list_create( &list, 256, 256);

	*retCount = 0;


	if ( !( hash = hash_create())) {
		list_destroy( &list);
		return nil;
	}

	/* collect font info */
	for ( i = 0; i < count; i++) {
		int len;
		PFont fm;
		if ( info[ i]. flags. disabled) continue;

		len = strlen( info[ i].font.name);

		fm = hash_fetch( hash, info[ i].font.name, len);
		if ( fm) {
			char ** enc = (char**) fm-> encoding;
			unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
			if ( *shift + 2 < 256 / sizeof(char*)) {
				int j, exists = 0;
				for ( j = 1; j <= *shift; j++) {
					if ( strcmp( enc[j], info[i].xname + info[i].info_offset) == 0) {
						exists = 1;
						break;
					}
				}
				if ( exists) continue;
				*(enc + ++(*shift)) = info[i].xname + info[i].info_offset;
			}
			continue;
		}

		if ( !( fm = ( PFont) malloc( sizeof( Font)))) {
			if ( hash) hash_destroy( hash, false);
			list_delete_all( &list, true);
			list_destroy( &list);
			return nil;
		}

		*fm = info[i]. font;

		{ /* multi-encoding format */
			char ** enc = (char**) fm-> encoding;
			unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
			memset( fm-> encoding, 0, 256);
			*(enc + ++(*shift)) = info[i].xname + info[i].info_offset;
			hash_store( hash, info[ i].font.name, strlen( info[ i].font.name), fm);
		}

		list_add( &list, ( Handle) fm);
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

#ifdef USE_XFT
	if ( guts. use_xft)
		fmtx = prima_xft_fonts( fmtx, nil, nil, retCount);
#endif

	return fmtx;
}

PFont
apc_fonts( Handle self, const char *facename, const char * encoding, int *retCount)
{
	int i, count = guts. n_fonts;
	PFontInfo info = guts. font_info;
	PFontInfo * table;
	int n_table;
	PFont fmtx;

	if ( !facename && !encoding) return spec_fonts( retCount);

	*retCount = 0;
	n_table = 0;

	/* stage 1 - browse through names and validate records */
	table = malloc( count * sizeof( PFontInfo));
	if ( !table && count > 0) return nil;

	if ( facename == nil) {
		PHash hash = hash_create();
		for ( i = 0; i < count; i++) {
			int len;
			if ( info[ i]. flags. disabled) continue;
			len = strlen( info[ i].font.name);
			if ( hash_fetch( hash, info[ i].font.name, len) ||
				strcmp( info[ i].xname + info[ i].info_offset, encoding) != 0)
				continue;
			hash_store( hash, info[ i].font.name, len, (void*)1);
			table[ n_table++] = info + i;
		}
		hash_destroy( hash, false);
		*retCount = n_table;
	} else {
		for ( i = 0; i < count; i++) {
			if ( info[ i]. flags. disabled) continue;
			if (
				( stricmp( info[ i].font.name, facename) == 0) &&
				(
					!encoding ||
					( strcmp( info[ i].xname + info[ i].info_offset, encoding) == 0)
				)
			) {
				table[ n_table++] = info + i;
			}
		}
		*retCount = n_table;
	}

	fmtx = malloc( n_table * sizeof( Font));
	bzero( fmtx, n_table * sizeof( Font));
	if ( !fmtx && n_table > 0) {
		*retCount = 0;
		free( table);
		return nil;
	}
	for ( i = 0; i < n_table; i++) {
		fmtx[i] = table[i]-> font;
	}
	free( table);

#ifdef USE_XFT
	if ( guts. use_xft)
		fmtx = prima_xft_fonts( fmtx, facename, encoding, retCount);
#endif

	return fmtx;
}

PHash
apc_font_encodings( Handle self )
{
	HE *he;
	PHash hash = hash_create();
	if ( !hash) return nil;

#ifdef USE_XFT
	if ( guts. use_xft)
		prima_xft_font_encodings( hash);
#endif

	hv_iterinit(( HV*) encodings);
	for (;;) {
		if (( he = hv_iternext( encodings)) == nil)
			break;
		hash_store( hash, HeKEY( he), HeKLEN( he), (void*)1);
	}
	return hash;
}

Bool
apc_gp_set_font( Handle self, PFont font)
{
	DEFXX;
	Bool reload;
	PCachedFont kf;

#ifdef USE_XFT
	if ( guts. use_xft && prima_xft_set_font( self, font)) return true;
#endif

	kf = prima_find_known_font( font, false, false);
	if ( !kf || !kf-> id) {
		dump_font( font);
		if ( DISP) warn( "internal error (kf:%p)", kf); /* the font was not cached, can't be */
		return false;
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
		XSetFont( DISP, XX-> gc, XX-> font-> id);
		XCHECKPOINT;
	}

	return true;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
	DEFMM;
	Bool xft_metrics = 0;
	PCachedFont kf = nil;

	font-> direction = 0; /* skip unnecessary logic */

#ifdef USE_XFT
	if ( guts. use_xft) {
		kf = prima_xft_get_cache( font);
		if ( kf) xft_metrics = 1;
	}
#endif

	if ( !kf) {
		kf = prima_find_known_font( font, false, false);
		if ( !kf || !kf-> id) {
			dump_font( font);
			warn( "internal error (kf:%p)", kf); /* the font was not cached, can't be */
			return false;
		}
	}

	XX-> font = kf;
	if ( !xft_metrics) {
		XX-> guillemots = XTextWidth( kf-> fs, ">>", 2);
	} else {
#ifdef USE_XFT
		XX-> guillemots = prima_xft_get_text_width( kf, ">>", 2, toAddOverhangs, nil, nil);
#endif
	}
	if ( !XX-> type. popup && X_WINDOW) {
		if (( kf-> font. height + 4) != X(PComponent(self)-> owner)-> menuHeight) {
			prima_window_reset_menu( PComponent(self)-> owner, kf-> font. height + MENU_ITEM_GAP * 2);
			XResizeWindow( DISP, X_WINDOW, XX-> w-> sz.x, XX-> w-> sz.y = kf-> font. height + MENU_ITEM_GAP * 2);
		} else if ( !XX-> paint_pending) {
			XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
			XX-> paint_pending = true;
		}
	}
	return true;
}

int
apc_gp_get_glyph_outline( Handle self, int index, int flags, int ** buffer)
{
#ifdef USE_XFT
	if ( guts.use_xft && X(self)-> font-> xft)
		return prima_xft_get_glyph_outline( self, index, flags, buffer);
#endif
	return 0;
}


Bool
prima_update_rotated_fonts( PCachedFont f, const char * text, int len, Bool wide, double direction, PRotatedFont * result,
	Bool * ok_to_not_rotate)
{
	PRotatedFont * pr = &f-> rotated;
	PRotatedFont r = nil;
	int i;

	while ( direction < 0)     direction += 360.0;
	while ( direction > 360.0) direction -= 360.0;

	/* granulate direction */
	{
		double g;
		int x = f-> fs-> max_bounds. width;
		int y = f-> fs-> max_bounds. ascent + f-> fs-> max_bounds. descent;
		if ( x < y) x = y;
		g = fabs(0.785398 - atan2(x+1,x)) * 90.0 / 3.14159265358;
		if ( g > 0) direction = floor(direction / g) * g;
	}

	if ( direction == 0.0) {
		if ( ok_to_not_rotate) *ok_to_not_rotate = true;
		return false;
	}
	if ( ok_to_not_rotate) *ok_to_not_rotate = false;

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
		r-> first1  = f-> fs-> min_byte1;
		r-> first2  = f-> fs-> min_char_or_byte2;
		r-> width   = ( f-> fs-> max_char_or_byte2 > 255 ? 255 : f-> fs-> max_char_or_byte2)
			- r-> first2 + 1;
		if ( r-> width < 0) r-> width = 0;
		r-> height = f-> fs-> max_byte1 - f-> fs-> min_byte1 + 1;
		r-> length = r-> width * r-> height;
		r-> defaultChar1 = f-> fs-> default_char >> 8;
		r-> defaultChar2 = f-> fs-> default_char & 0xff;

		if ( r-> defaultChar1 < r-> first1 || r-> defaultChar1 >= r-> first1 + r-> height ||
			r-> defaultChar2 < r-> first2 || r-> defaultChar2 >= r-> first2 + r-> width)
			r-> defaultChar1 = r-> defaultChar2 = -1;

		if ( r-> length > 0) {
			if ( !( r-> map = malloc( r-> length * sizeof( void*)))) {
				*pr = nil;
				free( r);
				warn("Not enough memory");
				return false;
			}
			bzero( r-> map, r-> length * sizeof( void*));
		}
		rad = direction * 3.14159265358 / 180.0;
		r-> sin. l = ( sin1 = sin( -rad)) * UINT16_PRECISION;
		r-> cos. l = ( cos1 = cos( -rad)) * UINT16_PRECISION;
		r-> sin2.l = ( sin2 = sin(  rad)) * UINT16_PRECISION;
		r-> cos2.l = ( cos2 = cos(  rad)) * UINT16_PRECISION;

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
		XChar2b index;
		XCharStruct * cs;
		XImage * ximage;
		PrimaXImage * px;
		Byte * ndata;

		index. byte1 = wide ? (( XChar2b*) text + i)-> byte1 : 0;
		index. byte2 = wide ? (( XChar2b*) text + i)-> byte2 : *((unsigned char*)text + i);

		/* querying character */
		if ( index. byte1 < r-> first1 || index. byte1 >= r-> first1 + r-> height ||
			index. byte2 < r-> first2 || index. byte2 >= r-> first2 + r-> width) {
			if ( r-> defaultChar1 < 0 || r-> defaultChar2 < 0) continue;
			index. byte1 = ( unsigned char) r-> defaultChar1;
			index. byte2 = ( unsigned char) r-> defaultChar2;
		}

		if ( r-> map[( index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2]) continue;
		cs = f-> fs-> per_char ?
			f-> fs-> per_char +
				( index. byte1 - f-> fs-> min_byte1) * r-> width +
				index. byte2 - f-> fs-> min_char_or_byte2 :
			&(f-> fs-> min_bounds);
		XSetForeground( DISP, r-> arena_gc, 0);
		XFillRectangle( DISP, r-> arena, r-> arena_gc, 0, 0, r-> orgBox. x, r-> orgBox .y);
		XSetForeground( DISP, r-> arena_gc, 1);
		if ( wide)
			XDrawString16( DISP, r-> arena, r-> arena_gc,
				( cs-> lbearing < 0) ? -cs-> lbearing : 0,
				r-> orgBox. y - f-> fs-> max_bounds. descent,
				&index, 1);
		else
			XDrawString( DISP, r-> arena, r-> arena_gc,
				( cs-> lbearing < 0) ? -cs-> lbearing : 0,
				r-> orgBox. y - f-> fs-> max_bounds. descent,
				(const char *)&index. byte2, 1);
		XCHECKPOINT;

		/* getting glyph bits */
		ximage = XGetImage( DISP, r-> arena, 0, 0, r-> orgBox. x, r-> orgBox. y, 1, XYPixmap);
		if ( !ximage) {
			warn("Can't get image %dx%d", r-> orgBox.x, r-> orgBox.y);
			return false;
		}
		XCHECKPOINT;
		prima_copy_xybitmap( r-> arena_bits, (Byte*)ximage-> data, r-> orgBox. x, r-> orgBox. y,
			r-> lineSize,  ximage-> bytes_per_line);
		XDestroyImage( ximage);

		px = prima_prepare_ximage( r-> dimension. x, r-> dimension. y, CACHE_BITMAP);
		if ( !px) return false;
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
					lx. l += UINT16_PRECISION/2;
				ly. l = r-> shift. x * r-> sin. l + y * r-> cos. l + UINT16_PRECISION/2;
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
								pv += ( UINT16_PRECISION - lx. i. f);
							if ( lx. i. i < r-> orgBox. x - 1) {
								if ( src[( lx. i. i + 1) >> 3] & ( 1 << ( 7 - (( lx. i. i + 1) & 7))))
									pv += lx. i. f;
							} else {
								if ( src[ lx . i. i >> 3] & ( 1 << ( 7 - ( lx . i. i & 7))))
									pv += UINT16_PRECISION/2;
							}
							if ( pv >= UINT16_PRECISION/2)
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
		r-> map[( index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2] = px;
	}

	if ( result)
		*result = r;

	return true;
}

XCharStruct *
prima_char_struct( XFontStruct * xs, void * c, Bool wide)
{
	XCharStruct * cs;
	int d = xs-> max_char_or_byte2 - xs-> min_char_or_byte2 + 1;
	int index1        = wide ? (( XChar2b*) c)-> byte1 : 0;
	int index2        = wide ? (( XChar2b*) c)-> byte2 : *((char*)c);
	int default_char1 = wide ? ( xs-> default_char >> 8) : 0;
	int default_char2 = xs-> default_char & 0xff;

	if ( default_char1 < xs-> min_byte1 ||
		default_char1 > xs-> max_byte1)
		default_char1 = xs-> min_byte1;
	if ( default_char2 < xs-> min_char_or_byte2 ||
		default_char2 > xs-> max_char_or_byte2)
		default_char2 = xs-> min_char_or_byte2;

	if ( index1 < xs-> min_byte1 ||
		index1 > xs-> max_byte1) {
		index1 = default_char1;
		index2 = default_char2;
	}
	if ( index2 < xs-> min_char_or_byte2 ||
		index2 > xs-> max_char_or_byte2) {
		index1 = default_char1;
		index2 = default_char2;
	}
	cs = xs-> per_char ?
		xs-> per_char +
		( index1 - xs-> min_byte1) * d +
		( index2 - xs-> min_char_or_byte2) :
		&(xs-> min_bounds);
	return cs;
}

const char *
prima_font_debug_style(int style)
{
	static char buf[256];
	char * p = buf;

	if ( style & fsBold) *p++ = 'B';
	if ( style & fsThin) *p++ = 'T';
	if ( style & fsItalic) *p++ = 'I';
	if ( style & fsUnderlined) *p++ = 'U';
	if ( style & fsStruckOut) *p++ = 'S';
	if ( style & fsOutline) *p++ = 'O';
	if ( style & ~fsMask) *p++ = '+';
	if ( style == 0) *p++ = '0';
	*p++ = 0;

	return buf;
}
