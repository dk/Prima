/***********************************************************/
/*                                                         */
/*  System dependent font routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

static PHash xfontCache = NULL;
static Bool have_vector_fonts = false;
static PHash encodings = NULL;
static char **ignore_encodings;
static int n_ignore_encodings;
static char *s_ignore_encodings;

static Bool   do_core_fonts = true;
static Bool   do_no_scaled_fonts = false;

#define MY_MATRIX (PDrawable(self)->current_state.matrix)
#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding

static Bool detail_font_info( PFontInfo f, PFont font, PCachedFont kf, Bool bySize);
static Bool xlfd_parse_font( char * xlfd_name, PFontInfo info, Bool do_vector_fonts);

static void
str_lwr( char *d, const char *s)
{
	while ( *s) {
		*d++ = tolower( *s++);
	}
	*d = '\0';
}

Bool
prima_corefont_init( char *error_buf)
{
	char **names;
	int count, j , i, bad_fonts = 0, vector_fonts = 0;
	PFontInfo info;

	if ( do_core_fonts) {
		int nocorefonts = 0;
		apc_fetch_resource( "Prima", "", "Nocorefonts", "nocorefonts",
			NULL_HANDLE, frUnix_int, &nocorefonts);
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
	ignore_encodings = NULL;
	s_ignore_encodings = NULL;
	if ( apc_fetch_resource( "Prima", "", "IgnoreEncodings", "ignoreEncodings",
				NULL_HANDLE, frString, &s_ignore_encodings))
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
		NULL_HANDLE, frUnix_int, &guts. no_scaled_fonts);
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

	xfontCache      = hash_create();

	/* locale */
	{
		int len;
		char * s = setlocale( LC_CTYPE, NULL);
		while ( *s && *s != '.') s++;
		while ( *s && *s == '.') s++;
		strlcpy( guts. locale, s, 31);
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

	return true;
}

Bool
prima_corefont_set_option( char * option, char * value)
{
	if ( strcmp( option, "no-core-fonts") == 0) {
		if ( value) warn("`--no-core' option has no parameters");
		do_core_fonts = false;
		return true;
	} else
	if ( strcmp( option, "noscaled") == 0) {
		if ( value) warn("`--noscaled' option has no parameters");
		do_no_scaled_fonts = true;
		return true;
	}
	return false;
}

void
prima_corefont_encodings(PHash hash)
{
	HE *he;
	hv_iterinit(( HV*) encodings);
	for (;;) {
		if (( he = hv_iternext( encodings)) == NULL)
			break;
		hash_store( hash, HeKEY( he), HeKLEN( he), (void*)1);
	}
}

Bool
prima_corefont_encoding( char * encoding)
{
	return hash_fetch( encodings, encoding, strlen( encoding)) != NULL;
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
		c = NULL;
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
			c = NULL;
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
			strlcpy( f-> font. family, c, 255);
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
			strlcpy( f-> font. name, c, 255);
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
						strlcpy( f-> font. family, d, lim);
						cnt++;
					} else if ( cnt == 1) {
						lim = ( c - d > 255 ) ? 255 : c - d;
						strlcpy( f-> font. name, d, lim);
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
					prima_fill_default_font( &fx);
					if ( f-> flags. encoding) strcpy( fx. encoding, f-> font. encoding);
					prima_font_pick( &fx, NULL, NULL, FONTKEY_CORE);
					strcpy( f-> font. name, fx. name);
				}
			} else {
				char c[512];
				snprintf( c, 512, "%s %s", f-> font. family, f-> font. name);
				strlcpy( f-> font. name, c, 256);
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
					prima_fill_default_font( &xf);
				if ( !nofamily) strcpy( xf. family, info-> font. family);
				if ( !noname)   strcpy( xf. name, info-> font. name);
				prima_font_pick( &xf, NULL, NULL, FONTKEY_CORE);
				if ( noname)   strcpy( info-> font. name,   xf. name);
				if ( nofamily) strcpy( info-> font. family, xf. family);
			}
			if (
				( strcmp( info->font.name, "clean") == 0) ||
				( strcmp( info->font.name, "fixed") == 0) ||
				( strcmp( info->font.name, "bitstream charter") == 0) 
			)
				info-> flags. known = 1;
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
			if ( strchr( c, '*') == NULL) {
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

Bool
prima_corefont_pick_default_font_with_encoding(void)
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
		prima_fill_default_font( &guts. default_font);
		strcpy( guts. default_font. name, guts. font_info[best].font.name);
		strcpy( guts. default_font. encoding, guts. locale);
		prima_font_pick( &guts. default_font, NULL, NULL, FONTKEY_CORE);
		guts. default_font. pitch = fpDefault;
		return true;
	}
	return false;
}

void
prima_corefont_pp2font( char * ppFontNameSize, PFont font)
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
			Fdebug("font: cannot load %s", ppFontNameSize);
			if ( !guts. default_font_ok) {
				prima_fill_default_font( font);
				prima_font_pick( font, NULL, NULL, 0 );
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
	detail_font_info( &fi, font, NULL, false);
	*font = fi. font;
	if ( guts. default_font_ok) {
		def = &guts. default_font;
	} else {
		prima_fill_default_font( def = &def_dummy);
		if ( !prima_font_pick( def, NULL, NULL, 0)) /* too early */
			goto NO_DEFAULT_FONT;
	}
	if ( font-> height == 0) font-> height = def-> height;
	if ( font-> size   == 0) font-> size   = def-> size;
	if ( strlen( font-> name) == 0) strcpy( font-> name, def-> name);
	if ( strlen( font-> family) == 0) strcpy( font-> family, def-> family);
NO_DEFAULT_FONT:
	prima_font_pick( font, NULL, NULL, 0);
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

	Fdebug("font: %s%s parsed to: %d.[w=%d,s=%g].%s.%s",
			newEntry ? "new " : "", ppFontNameSize, DEBUG_FONT((*font)));
}

static void
cleanup_rotated_font_entry( PRotatedFont r, unsigned int glyph_id )
{
	unsigned int i = r->length;
	while (i--) {
		if ( r-> map[i] )
		if ( r-> map[i] && i != glyph_id ) {
			prima_free_ximage( r-> map[i]);
			r-> map[i] = NULL;
			guts.rotated_font_cache_size -= r->arenaSize;
		}
	}
}

static Bool
free_rotated_entries( PCachedFont f, int keyLen, void * key, void * this_font)
{
	PRotatedFont r;

	if ( f->type != FONTKEY_CORE )
		return false;

	r = f->rotated;
	while ( r ) {
		if ( r != this_font )
			cleanup_rotated_font_entry( r, UINT_MAX );
		r = (PRotatedFont) r-> next;
	}

	return guts.rotated_font_cache_size < MAX_ROTATED_FONT_CACHE_SIZE / 2;
}

static void
cleanup_rotated_font_cache( PRotatedFont this_font, unsigned int this_glyph )
{
	if ( guts. font_hash)
		hash_first_that( guts. font_hash, (void*)free_rotated_entries, this_font, NULL, NULL);
	if ( guts.rotated_font_cache_size < MAX_ROTATED_FONT_CACHE_SIZE )
		return;
	cleanup_rotated_font_entry( this_font, this_glyph);
}


void
prima_corefont_free_cached_font( PCachedFont f)
{
	while ( f-> rotated) {
		PRotatedFont r = f-> rotated;
		while ( r-> length--) {
			if ( r-> map[ r-> length]) {
				prima_free_ximage( r-> map[ r-> length]);
				r-> map[ r-> length] = NULL;
				guts.rotated_font_cache_size -= r->arenaSize;
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

void
prima_corefont_done( void)
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
	guts. font_names = NULL;
	guts. n_fonts = 0;
	guts. font_info = NULL;

	free(ignore_encodings);
	free(s_ignore_encodings);

	if ( guts. font_hash)
		hash_first_that( guts. font_hash, (void*)free_rotated_entries, NULL, NULL, NULL);

	hash_destroy( xfontCache, false);
	xfontCache = NULL;
	hash_destroy( encodings, false);
	encodings = NULL;

}

void
prima_corefont_build_key( PFontKey key, PFont f, Bool bySize)
{
	key-> height = bySize ? -f-> size * FONT_SIZE_GRANULARITY : f-> height;
	key-> width  = f-> width;
	key-> style  = f-> style & ~(fsUnderlined|fsOutline|fsStruckOut) & fsMask;
	key-> pitch  = f-> pitch & fpMask;
	strcpy( key-> name, f-> name);
	strcat( key-> name, "\1");
	strcat( key-> name, f-> encoding);
}

static Bool
detail_font_info( PFontInfo f, PFont font, PCachedFont kf, Bool bySize)
{
	XFontStruct *s = NULL;
	unsigned long v;
	char name[ 1024];
	FontInfo fi;
	PFontInfo of = f;
	int height = 0, size = 0;
	HeightGuessStack hgs;

	if ( f-> vecname) {
		if ( bySize)
			size = font-> size * 10;
		else {
			height = font-> height * 10;
			prima_font_init_try_height( &hgs, height, f-> flags. heights_cache ? f-> heights_cache[0] : height);
			if ( f-> flags. heights_cache)
				height = prima_font_try_height( &hgs, f-> heights_cache[1]);
		}
	}

AGAIN:
	if ( f-> vecname) {
		memmove( &fi, f, sizeof( fi));
		fi. flags. size = fi. flags. height = fi. flags. width = fi. flags. ascent =
			fi. flags. descent = fi. flags. internalLeading = 0;
		f = &fi;

		if ( f-> flags. bad_vector) {
			/* three fields */
			sprintf( name, f-> vecname, height / 10, size, font-> width * 10);
		} else {
			/* five fields */
			sprintf( name, f-> vecname, height / 10, size, 0, 0, font-> width * 10);
		}
		Fdebug("font: construct by %s h=%g, s=%d", bySize ? "size" : "height", (float)height/10, size);
	} else {
		strcpy( name, f-> xname);
	}

	Fdebug( "font: loading %s", name);
	s = hash_fetch( xfontCache, name, strlen( name));

	if ( !s) {
		s = XLoadQueryFont( DISP, name);
		XCHECKPOINT;
		if ( !s) {
			if ( of-> flags. disabled)
				warn( "font %s pick-up error", name);
			of-> flags. disabled = true;
			Fdebug( "font: kill %s", name);
			return false;
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
			int h = prima_font_try_height( &hgs, f-> font. height * 10);
			Fdebug("font height pick: %d::%d => %d, advised %d", hgs.sp-1, font-> height, f-> font. height, h);
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
						( f-> font. size * f-> font. yDeviceRes) / 72.0 + 0.5;
				} else {
					f-> font. internalLeading = 0;
				}
			}
			f-> flags. internalLeading = true;
		}

		if ( !f-> flags. underlines) {
			int underlinePos, underlineThickness;
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
			f-> font.underlinePosition  = underlinePos;
			f-> font.underlineThickness = underlineThickness;
			f-> flags.underlines = true;
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
					f-> font. size = ( f-> font. height - f-> font. internalLeading) * 72.0 / f-> font. height + 0.5;
			}
		}
		f-> flags. size = true;

		/* misc stuff */
		f-> font. resolution       = f-> font. yDeviceRes * 0x10000 + f-> font. xDeviceRes;
		f-> flags. ascent          = true;
		f-> font. ascent           = f-> font. height - s-> max_bounds. descent;
		f-> flags. descent         = true;
		f-> font. descent          = s-> max_bounds. descent;
		f-> font. defaultChar      = s-> default_char;
		f-> font.  firstChar       = s-> min_byte1 * 256 + s-> min_char_or_byte2;
		f-> font.  lastChar        = s-> max_byte1 * 256 + s-> max_char_or_byte2;
		f-> font.  direction       = 0;
		f-> font.  externalLeading =
			abs( s-> max_bounds. ascent  - s-> ascent) +
			abs( s-> max_bounds. descent - s-> descent);
		bzero(&f->font.is_utf8, sizeof(f->font.is_utf8));
		bzero(&f->font.undef, sizeof(f->font.undef));

		/* detailing width */
		if ( f-> font. width == 0 || !f-> flags. width) {
			 if ( f-> vecname && font-> width > 0) {
				f-> font. width = font-> width;
				Fdebug("font: width = copy as is %d", f->font.width);
			} else if ( XGetFontProperty( s, FXA_AVERAGE_WIDTH, &v) && v) {
				XCHECKPOINT;
				f-> font. width = (v + 5) / 10;
				Fdebug("font: width = FXA_AVERAGE_WIDTH %d(%d)", f->font.width, v);
			} else {
				f-> font. width = s-> max_bounds. width;
				Fdebug("font: width = max_width %d", f->font.width);
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
				Fdebug("font: force width = max_width %d", f->font.width);
			}
		}
		of-> flags. sloppy = false;
	}

	if ( !kf )
		return true;

	/* detailing stuff */
	*font = f-> font;

	Fdebug("font cache match: %dx%d(%g)%s.%s/%s %s", DEBUG_FONT((*font)), _F_DEBUG_STYLE(font->style), _F_DEBUG_PITCH(font->pitch));
	kf-> id = s-> fid;
	kf-> fs = s;
	kf-> flags = f-> flags;

	return true;
}

#define QUERYDIFF_BY_SIZE        (-1)
#define QUERYDIFF_BY_HEIGHT      (-2)

static double
query_diff( PFontInfo fi, PFont f, char * lcname, int selector)
{
	double diff = 0.0;
	int enc_match = 0, name_match = 0;

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
		name_match = 1;
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

	if ( !name_match && !fi-> flags. known ) diff += 2.0;

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

PCachedFont
prima_corefont_match(PFont match, Bool by_size, PCachedFont kf)
{
	PFontInfo info = guts. font_info;
	int i, n = guts. n_fonts, index, lastIndex;
	int query_type = by_size ? QUERYDIFF_BY_SIZE : QUERYDIFF_BY_HEIGHT;
	double minDiff;
	char lcname[ 256];
	HeightGuessStack hgs;

	if (n == 0) return NULL;

	if ( strcmp( match-> name, "Default") == 0)
		strcpy( match-> name, "helvetica");

	if ( by_size) {
		Fdebug("font reqS:%g(h=%d)x%d.%s.%s %s", match-> size, match-> height, match->width, _F_DEBUG_STYLE(match-> style), _F_DEBUG_PITCH(match-> pitch), match-> name, match-> encoding);
	} else {
		Fdebug("font reqH:%d(s=%g)x%d.%s.%s %s", match-> height, match-> size, match->width, _F_DEBUG_STYLE(match-> style), _F_DEBUG_PITCH(match-> pitch), match-> name, match-> encoding);
	}

	if ( !hash_fetch( encodings, match-> encoding, strlen( match-> encoding)))
		match-> encoding[0] = 0;

	if ( !by_size)
		prima_font_init_try_height( &hgs, match-> height, match-> height);

	str_lwr( lcname, match-> name);
AGAIN:
	index = lastIndex = -1;
	minDiff = INT_MAX;
	for ( i = 0; i < n; i++) {
		double diff;
		if ( info[i]. flags. disabled) continue;
		diff = query_diff( info + i, match, lcname, query_type);
		if ( diff < minDiff) {
			lastIndex = index;
			index = i;
			minDiff = diff;
		}
		if ( diff < 1) break;
	}
	if (index < 0) {
		Fdebug("font: no more fonts to match, bail out");
		return NULL;
	}

	i = index;
	Fdebug("font: #%d (diff=%g): %s", i, minDiff, info[i].xname);
	Fdebug("font: pick:%d(%g)x%d.%s.%s %s/%s %s.%s", info[i].font. height, info[i].font. size, info[i].font. width, _F_DEBUG_STYLE(info[i].font. style), _F_DEBUG_PITCH(info[i].font. pitch), info[i].font. name, info[i].font. encoding, info[i].flags.sloppy?"sloppy":"", info[i].vecname?"vector":"");

	if ( !by_size && info[ i]. flags. sloppy && !info[ i]. vecname) {
		if (!detail_font_info( info + i, match, NULL, by_size)) {
			Fdebug("font: bad one, try again");
			goto AGAIN;
		}
		if ( minDiff < query_diff( info + i, match, lcname, by_size)) {
			int h = prima_font_try_height( &hgs, info[i]. font. height);
			if ( h > 0) {
				Fdebug("font: try again with new height %d", h);
				query_type = h;
				goto AGAIN;
			}
		}
	}

	if (!detail_font_info( info + index, match, kf, by_size)) {
		Fdebug("font: bad match, try again");
		goto AGAIN;
	}

	return kf;
}

static PRotatedFont
find_or_allocate_rotated_font_entry( PCachedFont f, Fixed *fm, Fixed *im, Matrix cm)
{
	int    i, rbox_x[4], rbox_y[4], box_x[4], box_y[4], box[4];
	XGCValues xgv;
	PRotatedFont r = NULL, *pr = &f-> rotated;

	/* finding record for given matrix without translation */
	while (*pr) {
		if (memcmp( (*pr)-> matrix, fm, sizeof(Fixed)*4) == 0) {
			r = *pr;
			break;
		}
		pr = ( PRotatedFont *) &((*pr)-> next);
	}

	/* creating startup values for new entry */
	r = *pr = malloc( sizeof( RotatedFont));
	if ( !r) {
		warn("Not enough memory");
		return NULL;
	}
	bzero( r, sizeof( RotatedFont));
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
			*pr = NULL;
			free( r);
			warn("Not enough memory");
			return NULL;
		}
		bzero( r-> map, r-> length * sizeof( void*));
	}

	memcpy(r->matrix,  fm, sizeof(Fixed) * 4);
	memcpy(r->inverse, im, sizeof(Fixed) * 4);

/*
	1(0,y)  2(x,y)
	0(0,0)  3(x,0)
*/
	box_x[0] = box_y[0] = box_x[1] = box_y[3] = 0;
	r-> orgBox. x = box_x[2] = box_x[3] = f-> fs-> max_bounds. width;
	r-> orgBox. y = box_y[1] = box_y[2] = f-> fs-> max_bounds. ascent + f-> fs-> max_bounds. descent;

	for ( i = 0; i < 4; i++) {
		rbox_x[i] = box_x[i];
		rbox_y[i] = box_y[i];
		prima_matrix_apply_int_to_int(cm, &rbox_x[i], &rbox_y[i]);
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
	r-> arenaSize = r-> lineSize * r-> orgBox. y;
	if ( !( r-> arena_bits = malloc( r-> arenaSize)))
		goto FAILED;

	r-> arena = XCreatePixmap( DISP, guts. root, r-> orgBox.x, r-> orgBox. y, 1);
	if ( !r-> arena) {
		free( r-> arena_bits);
FAILED:
		*pr = NULL;
		free( r-> map);
		free( r);
		warn("Cannot create pixmap");
		return NULL;
	}
	XCHECKPOINT;
	r-> arena_gc = XCreateGC( DISP, r-> arena, 0, &xgv);
	XCHECKPOINT;
	XSetFont( DISP, r-> arena_gc, f-> id);
	XCHECKPOINT;
	XSetBackground( DISP, r-> arena_gc, 0);

	return r;
}

static PRotatedFont
find_or_allocate_straight_font_entry( PCachedFont f)
{
	Matrix cm;
	Fixed mx[4];
	prima_matrix_set_identity(cm);
	mx[0].l = mx[3].l = 1 * UINT16_PRECISION;
	mx[1].l = mx[2].l = 0;
	return find_or_allocate_rotated_font_entry(f, mx, mx, cm);
}

static PrimaXImage *
render_bitmap_glyph( PCachedFont f, PRotatedFont r, XChar2b index, Bool wide)
{
	XCharStruct *cs;
	XImage *ximage;
	PrimaXImage *px;

	cs = f-> fs-> per_char ?
		f-> fs-> per_char +
			( index. byte1 - f-> fs-> min_byte1) * r-> width +
			index. byte2 - f-> fs-> min_char_or_byte2 :
		&(f-> fs-> min_bounds);
	XSetForeground( DISP, r-> arena_gc, 0);
	XFillRectangle( DISP, r-> arena, r-> arena_gc, 0, 0, r-> orgBox. x, r-> orgBox .y);
	XSetForeground( DISP, r-> arena_gc, 1);
	if (wide)
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
	if ( !( ximage = XGetImage( DISP, r-> arena, 0, 0, r-> orgBox. x, r-> orgBox. y, 1, XYPixmap))) {
		warn("Can't get image %dx%d", r-> orgBox.x, r-> orgBox.y);
		return NULL;
	}
	XCHECKPOINT;
	prima_copy_1bit_ximage( r-> arena_bits, ximage, false);
	XDestroyImage( ximage);

	if ( !( px = prima_prepare_ximage( r-> dimension. x, r-> dimension. y, CACHE_BITMAP))) {
		warn("Can't get image %dx%d", r-> orgBox.x, r-> orgBox.y);
		return NULL;
	}

	return px;
}

static Bool
validate_xchar2b( PRotatedFont r, XChar2b *index)
{
	if (
		index-> byte1 < r-> first1 || index-> byte1 >= r-> first1 + r-> height ||
		index-> byte2 < r-> first2 || index-> byte2 >= r-> first2 + r-> width
	) {
		if ( r-> defaultChar1 < 0 || r-> defaultChar2 < 0)
			return false;
		index-> byte1 = ( unsigned char) r-> defaultChar1;
		index-> byte2 = ( unsigned char) r-> defaultChar2;
	}
	return true;
}

Bool
prima_update_rotated_fonts( PCachedFont f, const char * text, int len, Bool wide, double direction, Matrix matrix, PRotatedFont * result,
	Bool * ok_to_not_rotate)
{
	Fixed fm[4], im[4];
	Matrix cm;
	PRotatedFont r;
	int i;

	while ( direction < 0)     direction += 360.0;
	while ( direction > 360.0) direction -= 360.0;

	/* granulate direction and matrix */
	{
		int i;
		Matrix m1, inv;
		double d, ad, bc;
		prima_matrix_set_identity(m1);
		if ( direction != 0.0 ) {
			double s = sin( direction / 57.29577951);
			double c = cos( direction / 57.29577951);
			m1[0] = c;
			m1[1] = s;
			m1[2] = -s;
			m1[3] = c;
		}
		if ( prima_matrix_is_identity(matrix)) {
			COPY_MATRIX(m1, cm);
		} else {
			Matrix m2;
			COPY_MATRIX_WITHOUT_TRANSLATION( matrix, m2 );
			prima_matrix_multiply( m1, m2, cm);
		}

		/* can inverse? */
		ad = cm[0] * cm[3];
		bc = cm[1] * cm[2];
		if ( ad - bc == 0.0 )
			return false;
		d = ad - bc;

		for ( i = 0; i < 4; i++) {
			/* bitmap fonts don't require much precision anyway */
			double m = floor( cm[i] * 1000.0 ) / 1000.0;
			if ( m < -8192.0 ) m = -8192.0;
			if ( m >  8192.0 ) m =  8192.0;
			fm[i].l = m * UINT16_PRECISION;

			inv[i] = cm[i] / d;
			if ( inv[i] < -8192.0 ) inv[i] = -8192.0;
			if ( inv[i] >  8192.0 ) inv[i] =  8192.0;
		}
		im[0].l =  inv[3] * UINT16_PRECISION;
		im[1].l = -inv[1] * UINT16_PRECISION;
		im[2].l = -inv[2] * UINT16_PRECISION;
		im[3].l =  inv[0] * UINT16_PRECISION;
	}

	if ( prima_matrix_is_translated_only(matrix) && direction == 0.0) {
		if ( ok_to_not_rotate) *ok_to_not_rotate = true;
		return false;
	}
	if ( ok_to_not_rotate) *ok_to_not_rotate = false;

	if ( !( r = find_or_allocate_rotated_font_entry(f, fm, im, cm)))
		return false;

	/* processing character records */
	for ( i = 0; i < len; i++) {
		XChar2b index;
		PrimaXImage * px;
		Byte * ndata;
		unsigned int glyph_id;

		index. byte1 = wide ? (( XChar2b*) text + i)-> byte1 : 0;
		index. byte2 = wide ? (( XChar2b*) text + i)-> byte2 : *((unsigned char*)text + i);

		/* querying character */
		if ( !validate_xchar2b( r, &index))
			continue;

		if ( r-> map[( index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2])
			continue;

		if ( !( px = render_bitmap_glyph( f, r, index, wide )))
			return false;

		ndata = ( Byte*) px-> data_alias;
		bzero( ndata, px-> bytes_per_line_alias * r-> dimension. y);

		/* rotating */
		{
			int x, y, fast = r-> orgBox. y * r-> orgBox. x > 600;
			Fixed lx, ly;
			Byte * dst = ndata + px-> bytes_per_line_alias * ( r-> dimension. y - 1);

			for ( y = r-> shift. y; y < r-> shift. y + r-> dimension. y; y++) {
				lx. l = r-> shift. x * r-> inverse[0]. l + y * r-> inverse[2]. l;
				ly. l = r-> shift. x * r-> inverse[1]. l + y * r-> inverse[3]. l + UINT16_PRECISION/2;
				if ( fast) {
					lx. l += UINT16_PRECISION/2;
					for ( x = 0; x < r-> dimension. x; x++) {
						if (
							ly. i. i >= 0 && ly. i. i < r-> orgBox. y &&
							lx. i. i >= 0 && lx. i. i < r-> orgBox. x
						) {
							Byte * src = r-> arena_bits + r-> lineSize * ly. i. i;
							if ( src[ lx . i. i >> 3] & ( 1 << ( 7 - ( lx . i. i & 7))))
								dst[ x >> 3] |= 1 << ( 7 - ( x & 7));
						}
						lx. l += r-> inverse[0]. l;
						ly. l += r-> inverse[1]. l;
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
						lx. l += r-> inverse[0]. l;
						ly. l += r-> inverse[1]. l;
					}
				}
				dst -= px-> bytes_per_line_alias;
			}
		}

		if ( guts. bit_order != MSBFirst)
			prima_mirror_bytes( ndata, r-> dimension.y * px-> bytes_per_line_alias);
		glyph_id = ( index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2;
		r-> map[glyph_id] = px;

		guts.rotated_font_cache_size += r-> arenaSize;
		if ( guts.rotated_font_cache_size > MAX_ROTATED_FONT_CACHE_SIZE )
			cleanup_rotated_font_cache( r, glyph_id );
	}

	if ( result)
		*result = r;

	return true;
}

PrimaXImage*
prepare_straight_glyph(PCachedFont f, PRotatedFont r, XChar2b index)
{
	PrimaXImage *px;
	Byte *src, *dst;
	unsigned int i, src_stride, dst_stride, ylim, glyph_id;

	if ( !validate_xchar2b( r, &index))
		return NULL;

	if (( px = r-> map[( index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2]) != NULL)
		return px;

	if ( !( px = render_bitmap_glyph( f, r, index, true )))
		return NULL;

	src_stride = r->lineSize;
	dst_stride = px->bytes_per_line_alias;
	src        = r->arena_bits + src_stride * (r->orgBox.y - 1);
	dst        = (Byte*) px-> data_alias;
	ylim       = r->shift.y + r->dimension.y;
	for ( i = r->shift.y; i < ylim; i++, src -= src_stride, dst += dst_stride) {
		memcpy(dst, src, dst_stride);
		if ( guts. bit_order != MSBFirst)
			prima_mirror_bytes( dst, dst_stride);
	}

	glyph_id = ( index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2;
	r-> map[glyph_id] = px;

	guts.rotated_font_cache_size += r-> arenaSize;
	if ( guts.rotated_font_cache_size > MAX_ROTATED_FONT_CACHE_SIZE )
		cleanup_rotated_font_cache( r, glyph_id );

	return px;
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

Byte*
prima_corefont_get_glyph_bitmap( Handle self, uint16_t index, Bool mono, PPoint offset, PPoint size, int *advance)
{
	DEFXX;
	PRotatedFont r;
	XCharStruct *cs;
	XChar2b ch;
	Bool straight = false;
	int i, px;
	Fixed rx, ry;
	Byte *ret, *src, *dst;
	int src_stride, dst_stride, bytes;
	XImage *xi;

	ch.byte1 = index >> 8;
	ch.byte2 = index & 0xff;
	if ( !prima_update_rotated_fonts( XX-> font,
		(const char*)&ch, 1, true,
		PDrawable( self)-> font. direction, 
		MY_MATRIX,
		&r, &straight
	)) {
		if ( !straight )
			return NULL;
		if ( !( r = find_or_allocate_straight_font_entry(XX->font)))
			return NULL;
		if ( !prepare_straight_glyph(XX->font, r, ch))
			return NULL;
	}

	if ( !validate_xchar2b( r, &ch))
		return NULL;
	cs = XX-> font-> fs-> per_char ?
		XX-> font-> fs-> per_char +
			( ch. byte1 - XX-> font-> fs-> min_byte1) * r-> width +
			ch. byte2 - XX-> font-> fs-> min_char_or_byte2 :
		&(XX-> font-> fs-> min_bounds);

	/* querying character */
	if ( r-> map[(ch. byte1 - r-> first1) * r-> width + ch. byte2 - r-> first2] == NULL)
		return NULL;
	xi = r-> map[(ch. byte1 - r-> first1) * r-> width + ch. byte2 - r-> first2]-> image;

	/* find reference point in pixmap */
	px = ( cs-> lbearing < 0) ? -cs-> lbearing : 0;
	rx. l = px * r-> matrix[0]. l + UINT16_PRECISION/2;
	ry. l = px * r-> matrix[1]. l + UINT16_PRECISION/2;

	size-> x = r-> dimension. x;
	size-> y = r-> dimension. y;
	offset-> x = rx. i. i - r-> shift. x;
	offset-> y = ry. i. i - r-> shift. y - XX-> font-> font. descent;
	if (advance) *advance = cs-> width;

	src_stride = xi-> bytes_per_line;
	dst_stride = (( size->x * (mono ? 1 : 8) + 31) / 32) * 4;
	bytes      = ( src_stride < dst_stride ) ? src_stride : dst_stride;

	for (
		i = size-> y - 1, src = ((Byte*) xi-> data) + (size-> y - 1) * src_stride;
		i >= 0;
		i++, src -= src_stride
	) {
		register unsigned int j = bytes;
		register Byte *r = src;
		while ( j--)
			if ( *(r++) != 0 ) goto STOP1;
		size->y--;
		offset->y++;
	}
	STOP1:
	for ( i = 0, src = (Byte*) xi->data; i < size->y; i++, src += src_stride) {
		register unsigned int j = bytes;
		register Byte *r = src;
		while ( j--)
			if ( *(r++) != 0 ) goto STOP2;
		size->y--;
	}
	STOP2:
	if ( size->y <= 0 ) {
		size->x = size->y = offset->x = offset->y = 0;
		return malloc(0);
	}

	if ( !( ret = malloc(dst_stride * size->y)))
		return NULL;
	dst  = ret + dst_stride * (size->y - 1);
	for (
		i = 0;
		i < size->y;
		i++, src += src_stride, dst -= dst_stride
	) {
		if ( guts.bit_order == MSBFirst) {
			if ( mono )
				memcpy(dst, src, bytes);
			else
				bc_mono_byte( src, dst, size->x);
		} else {
			register Byte *d = dst, *s = src, *mirrored_bits = prima_mirror_bits();
			if ( mono ) {
				register unsigned int j = bytes;
				while (j--) *d++ = mirrored_bits[*s++];
			} else {
				Byte buf[128], *s = src, *d = dst;
				unsigned int j = bytes;
				while ( j > 0 ) {
					register Byte *b = buf;
					register unsigned int n = (j > sizeof(buf)) ? sizeof(buf) : j;
					unsigned int m = n;
					while (n--) *b++ = mirrored_bits[*s++];
					bc_mono_byte( b, d, m );
					d += m;
					j -= m;
				}
			}
		}
	}
	return ret;
}

static PFont
spec_fonts( int *retCount)
{
	int i, count = guts. n_fonts;
	PFontInfo info = guts. font_info;
	PFont fmtx = NULL;
	List list;
	PHash hash = NULL;

	list_create( &list, 256, 256);

	*retCount = 0;

	if ( !( hash = hash_create())) {
		list_destroy( &list);
		return NULL;
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
			return NULL;
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
		return NULL;
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
prima_corefont_fonts( Handle self, const char *facename, const char * encoding, int *retCount)
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
	if ( !table && count > 0) return NULL;

	if ( facename == NULL) {
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
		return NULL;
	}
	for ( i = 0; i < n_table; i++) {
		fmtx[i] = table[i]-> font;
	}
	free( table);

	return fmtx;
}

