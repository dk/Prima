#include "unix/guts.h"

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#ifdef USE_FONTQUERY

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
	{ "iso8859-1",  NULL, 0, 1 }
#ifdef HAVE_ICONV_H
	,
	{ "iso8859-2",  NULL, 0, 0 },
	{ "iso8859-3",  NULL, 0, 0 },
	{ "iso8859-4",  NULL, 0, 0 },
	{ "iso8859-5",  NULL, 0, 0 },
	{ "iso8859-7",  NULL, 0, 0 },
	{ "iso8859-8",  NULL, 0, 0 },
	{ "iso8859-9",  NULL, 0, 0 },
	{ "iso8859-10", NULL, 0, 0 },
	{ "iso8859-13", NULL, 0, 0 },
	{ "iso8859-14", NULL, 0, 0 },
	{ "iso8859-15", NULL, 0, 0 },
	{ "koi8-r",     NULL, 0, 0 }  /* this is special - change the constant
					KOI8_INDEX as well when updating
					the table */
/* You are welcome to add more 8-bit charsets here - just keep in mind
	that each encoding requires iconv() to load a file */
#endif
};

static CharSetInfo fontspecific_charset = { "fontspecific", NULL, 0, 1 };
static CharSetInfo utf8_charset         = { "iso10646-1",   NULL, 0, 1 };

#define KOI8_INDEX 12
#define MAX_CHARSET (sizeof(std_charsets)/sizeof(CharSetInfo))
#define STD_CHARSETS MAX_CHARSET
#define EXTRA_CHARSETS 1
#define ALL_CHARSETS (STD_CHARSETS+EXTRA_CHARSETS)

#define MY_MATRIX (PDrawable(self)->current_state.matrix)

static PHash encodings    = NULL;
static PHash mono_fonts   = NULL; /* family->mono font mapping */
static PHash prop_fonts   = NULL; /* family->proportional font mapping */
static char  fontspecific[] = "fontspecific";
static char  utf8_encoding[] = "iso10646-1";
static CharSetInfo * locale = NULL;

#define FCdebug(...) if (pguts->debug & DEBUG_FONTS) prima_debug2("fc", __VA_ARGS__)

void
prima_fc_init(void)
{
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

	fcs_ascii = FcCharSetCreate();
	for ( i = 32; i < 127; i++)  FcCharSetAddChar( fcs_ascii, i);

	std_charsets[0]. fcs = FcCharSetUnion( fcs_ascii, fcs_ascii);
	for ( i = 161; i < 255; i++) FcCharSetAddChar( std_charsets[0]. fcs, i);
	for ( i = 128; i < 255; i++) std_charsets[0]. map[i - 128] = i;
	std_charsets[0]. glyphs = ( 127 - 32) + ( 255 - 161);

#ifdef HAVE_ICONV_H
	sprintf( ucs4, "UCS-4%cE", (guts.machine_byte_order == LSBFirst) ? 'L' : 'B');
	for ( i = 1; i < STD_CHARSETS; i++) {
		memset( std_charsets[i]. map, 0, sizeof(std_charsets[i]. map));

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

	guts.fc_mismatch = hash_create();
	mono_fonts       = hash_create();
	prop_fonts       = hash_create();
	encodings        = hash_create();
	for ( i = 0; i < STD_CHARSETS; i++) {
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

	fontspecific_charset. fcs = FcCharSetCreate();
	for ( i = 128; i < 256; i++) fontspecific_charset. map[i - 128] = i;
	hash_store( encodings, fontspecific, strlen(fontspecific), (void*) &fontspecific_charset);

	utf8_charset. fcs = FcCharSetCreate();
	for ( i = 128; i < 256; i++) utf8_charset. map[i - 128] = i;
	hash_store( encodings, utf8_encoding, strlen(utf8_encoding), (void*) &utf8_charset);

	locale = hash_fetch( encodings, guts. locale, strlen( guts.locale));
	if ( !locale) locale = std_charsets;
	FcCharSetDestroy( fcs_ascii);

	prima_fc_init_font_substitution();
}

static Bool
utf8_flag_strncpy( char * dst, const char * src, unsigned int maxlen)
{
	Bool is_utf8 = false;
	while ( maxlen-- && *src) {
		if ( *((unsigned char*)src) > 0x7f)
			is_utf8 = true;
		*(dst++) = *(src++);
	}
	*dst = 0;
	return is_utf8;
}

void
prima_fc_init_font_substitution(void)
{
	int i;
	FcFontSet * s;
	FcPattern   *pat, **ppat;
	FcObjectSet *os;

	if ( guts.default_font_ok) {
		pat = FcPatternCreate();
		FcPatternAddBool( pat, FC_SCALABLE, 1);
		FcPatternAddString( pat, FC_FAMILY, (FcChar8*) guts.default_font.name);
		os = FcObjectSetBuild( FC_FAMILY, (void*) 0);
		s = FcFontList( 0, pat, os);
		if ( s && s->nfont ) {
			PFont f;
			if (( f = prima_font_mapper_save_font(guts.default_font.name, 0)) != NULL ) {
				f->is_utf8 = guts.default_font.is_utf8;
				f->undef.name = 0;
				strlcpy(f->family, guts.default_font.family,256);
				f->undef.vector = 0;
				f->vector = guts.default_font.vector;
				f->undef.pitch = 0;
				f->pitch  = guts.default_font.pitch;
			}
		}
		FcObjectSetDestroy( os);
		FcPatternDestroy( pat);
		FcFontSetDestroy(s);
	}

	pat = FcPatternCreate();
	FcPatternAddBool( pat, FC_SCALABLE, 1);
	os = FcObjectSetBuild( FC_FAMILY, FC_FOUNDRY, FC_SCALABLE, FC_SPACING, FC_WEIGHT, FC_SLANT, (void*) 0);
	s = FcFontList( 0, pat, os);
	FcObjectSetDestroy( os);
	FcPatternDestroy( pat);

	if ( !s) return;

	ppat = s-> fonts;
	for ( i = 0; i < s->nfont; i++, ppat++) {
		PFont f;
		int j, slant, weight;
		unsigned int style = 0;
		FcChar8 * s;

		if ( FcPatternGetString(*ppat, FC_FAMILY, 0, &s) != FcResultMatch)
			continue;

		if ( FcPatternGetInteger( *ppat, FC_SLANT, 0, &slant) == FcResultMatch) {
			if ( slant == FC_SLANT_ITALIC || slant == FC_SLANT_OBLIQUE)
				style |= fsItalic;
		}
		if ( FcPatternGetInteger( *ppat, FC_WEIGHT, 0, &weight) == FcResultMatch) {
			if ( weight <= FC_WEIGHT_LIGHT )
				style |= fsThin;
			else if ( weight >= FC_WEIGHT_BOLD)
				style |= fsBold;
		}

		if ( !( f = prima_font_mapper_save_font((const char*) s, style)))
			continue;

		f-> is_utf8.name = utf8_flag_strncpy( f->name, (char*)s, 255);
		f-> undef.name = 0;

		if ( FcPatternGetString(*ppat, FC_FOUNDRY, 0, &s) == FcResultMatch) {
			f->is_utf8.family = utf8_flag_strncpy( f->family, (char*)s, 255);
		}
		if ( FcPatternGetInteger(*ppat, FC_SPACING, 0, &j) == FcResultMatch) {
			f->pitch = (( i == FC_PROPORTIONAL) ? fpVariable : fpFixed);
			f->undef.pitch = 0;
		}

		f->undef.vector = 0;
		f->vector = fvOutline;
	}

	FcFontSetDestroy(s);
}

void
prima_fc_done(void)
{
	int i;
	for ( i = 0; i < STD_CHARSETS; i++)
		if ( std_charsets[i]. fcs)
			FcCharSetDestroy( std_charsets[i]. fcs);
	FcCharSetDestroy( fontspecific_charset. fcs);
	FcCharSetDestroy( utf8_charset. fcs);
	hash_destroy( encodings, false);
	hash_destroy( prop_fonts, true);
	hash_destroy( mono_fonts, true);
	hash_destroy( guts.fc_mismatch, false);
}

void
prima_fc_pattern2fontnames( FcPattern * pattern, Font * font)
{
	FcChar8 * s;

	if ( FcPatternGetString( pattern, FC_FAMILY, 0, &s) == FcResultMatch) {
		font-> is_utf8.name = utf8_flag_strncpy( font-> name, (char*)s, 255);
		font-> undef.name = 0;
	}
	if ( FcPatternGetString( pattern, FC_FOUNDRY, 0, &s) == FcResultMatch)
		font-> is_utf8.family = utf8_flag_strncpy( font-> family, (char*)s, 255);

	/* fake family */
	if (
		( strcmp(font->family, "") == 0) ||
		( strcmp(font->family, "unknown") == 0)
	) {
		char * name   = font->name;
		char * family = font->family;
		while (*name && *name != ' ') {
			*family++ = (*name < 127 ) ? tolower(*name) : *name;
			name++;
		}
		*family = 0;
	}
}

const char *
prima_fc_pattern2encoding( FcPattern *p)
{
	const char *ret = NULL;
	FcCharSet * c = NULL;
	FcPatternGetCharSet( p, FC_CHARSET, 0, &c);
	if ( c && (FcCharSetCount(c) > 0)) {
		int i;
		for ( i = 0; i < STD_CHARSETS; i++) {
			if ( !std_charsets[i]. enabled) continue;
			if ( FcCharSetIntersectCount( std_charsets[i]. fcs, c) >= std_charsets[i]. glyphs - 1) {
				ret = std_charsets[i]. name;
				break;
			}
		}
	}
	FcPatternDestroy( p);
	return ret;
}

/* find a most similar monospace/proportional font by name and family */
char *
prima_fc_find_good_font_by_family( Font * f, int fc_spacing )
{
	static Bool initialized = 0;

	if ( !initialized ) {
		/* iterate over all monospace and proportional font, build family->name (i.e best default match) hash */
		int i;
		FcFontSet * s;
		FcPattern   *pat, **ppat;
		FcObjectSet *os;

		initialized = 1;

		pat = FcPatternCreate();
		FcPatternAddBool( pat, FC_SCALABLE, 1);
		os = FcObjectSetBuild( FC_FAMILY, FC_CHARSET, FC_ASPECT,
			FC_SLANT, FC_WEIGHT, FC_SIZE, FC_PIXEL_SIZE, FC_SPACING,
			FC_FOUNDRY, FC_SCALABLE, FC_DPI,
			(void*) 0);
		s = FcFontList( 0, pat, os);
		FcObjectSetDestroy( os);
		FcPatternDestroy( pat);
		if ( !s) return NULL;

		ppat = s-> fonts;
		for ( i = 0; i < s->nfont; i++, ppat++) {
			Font f;
			int spacing = FC_PROPORTIONAL, slant, len, weight;
			FcBool scalable;
			PHash font_hash;

			/* only regular fonts */
			if (
				( FcPatternGetInteger( *ppat, FC_SLANT, 0, &slant) != FcResultMatch) ||
				( slant == FC_SLANT_ITALIC || slant == FC_SLANT_OBLIQUE)
			)
				continue;
			if (
				( FcPatternGetInteger( *ppat, FC_WEIGHT, 0, &weight) != FcResultMatch) ||
				( weight <= FC_WEIGHT_LIGHT || weight >= FC_WEIGHT_BOLD)
			)
				continue;
			if (
				( FcPatternGetBool( *ppat, FC_SCALABLE, 0, &scalable) != FcResultMatch) ||
				( scalable == 0 )
			)
				continue;

			prima_fc_pattern2fontnames( *ppat, &f);
			len = strlen(f.family);

			/* sort fonts by family and spacing */
			font_hash = (
				( FcPatternGetInteger( *ppat, FC_SPACING, 0, &spacing) == FcResultMatch) &&
				( spacing == FC_MONO )
			) ?
				mono_fonts : prop_fonts;
			if ( hash_fetch( font_hash, f.family, len))
				continue;

			if ( spacing == FC_MONO ) {
				char * p;
				#define MONO_TAG " Mono"
				if (( p = strcasestr(f.name, MONO_TAG)) == NULL )
					continue;
				p += strlen(MONO_TAG);
				if ( *p != ' ' && *p != 0 )
					continue;
				#undef MONO_TAG
			}

			hash_store( font_hash, f.family, len, duplicate_string(f.name));
		}
		FcFontSetDestroy(s);
	}
	/* initialized ok */
	FCdebug("trying to find %s pitch replacement for %s/%s",
		(fc_spacing == FC_MONO ) ? "fixed" : "variable",
		f->name, f->family);

	/* try to find same family and same 1st word in font name */
	{
		char *c, *w, word1[255], word2[255];
		PHash font_hash = (fc_spacing == FC_MONO) ? mono_fonts : prop_fonts;
		c = hash_fetch( font_hash, f->family, strlen(f->family));
		if ( !c ) {
			FCdebug("no default font for that family");
			return NULL;
		}
		if ( strcmp( c, f->name) == 0) {
			FCdebug("same font");
			return NULL; /* same font */
		}

		strcpy( word1, c);
		strcpy( word2, f->name);
		if (( w = strchr( word1, ' '))) *w = 0;
		if (( w = strchr( word2, ' '))) *w = 0;
		if ( strcmp( word1, word2 ) != 0 ) {
			FCdebug("default font %s doesn't look similar", c);
			return NULL;
		}
		FCdebug("replaced with %s", c);

		return c;
	}
}

void
prima_fc_pattern2font( FcPattern * pattern, PFont font)
{
	int i, j;
	double d = 1.0;
	FcCharSet *c = NULL;

	/* FcPatternPrint( pattern); */
	prima_fc_pattern2fontnames(pattern, font);

	font-> style = 0;
	font-> undef.style = 0;
	if ( FcPatternGetInteger( pattern, FC_SLANT, 0, &i) == FcResultMatch)
		if ( i == FC_SLANT_ITALIC || i == FC_SLANT_OBLIQUE)
			font-> style |= fsItalic;
	if ( FcPatternGetInteger( pattern, FC_WEIGHT, 0, &i) == FcResultMatch) {
		if ( i <= FC_WEIGHT_LIGHT)
			font-> style |= fsThin;
		else if ( i >= FC_WEIGHT_BOLD)
			font-> style |= fsBold;
		font-> weight = i * fwUltraBold / FC_WEIGHT_ULTRABOLD;
	}

	font-> xDeviceRes = guts. resolution. x;
	font-> yDeviceRes = guts. resolution. y;
	if ( FcPatternGetDouble( pattern, FC_DPI, 0, &d) == FcResultMatch)
		font-> yDeviceRes = d + 0.5;
	if ( FcPatternGetDouble( pattern, FC_ASPECT, 0, &d) == FcResultMatch)
		font-> xDeviceRes = font-> yDeviceRes * d;

	font-> undef.pitch = 1;
	if ( FcPatternGetInteger( pattern, FC_SPACING, 0, &i) == FcResultMatch) {
		font-> pitch = (( i == FC_PROPORTIONAL) ? fpVariable : fpFixed);
		font-> undef.pitch = 0;
	}

	font-> undef.height = 1;
	if ( FcPatternGetDouble( pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch) {
		font-> height = d + 0.5;
		font-> undef. height = 0;
		FCdebug("height factor read:%d (%g)", font-> height, d);
	}

	font-> width = 100; /* warning, FC_WIDTH does not reflect FC_MATRIX scale changes */
	if ( FcPatternGetDouble( pattern, FC_WIDTH, 0, &d) == FcResultMatch) {
		font->width = d + 0.5;
		FCdebug("width factor read:%d (%g)", font-> width, d);
	}
	font-> width = ( font-> width * font-> height) / 100.0 + .5;
	font->undef. width = 0;

	if ( FcPatternGetDouble( pattern, FC_SIZE, 0, &d) == FcResultMatch) {
		font-> size = d;
		font-> undef. size = 0;
		FCdebug("size factor read:%g", font-> size);
	} else if (!font-> undef.height && font->yDeviceRes != 0) {
		font-> size = font-> height * 72.27 / font-> yDeviceRes;
		font-> undef. size = 0;
		FCdebug("size calculated:%g", font-> size);
	} else {
		font-> undef. size = 1;
		FCdebug("size unknown");
	}

	/* fvBitmap is 0, fvOutline is 1 */
	font-> vector = fvOutline;
	if ( FcPatternGetBool( pattern, FC_SCALABLE, 0, &font-> vector) == FcResultMatch) 
		font-> undef. vector = 0;

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
prima_fc_fonts( PFont array, const char *facename, const char * encoding, int *retCount)
{
	FcFontSet * s;
	FcPattern   *pat, **ppat;
	FcObjectSet *os;
	PFont newarray, f;
	PHash names = NULL;
	CharSetInfo * csi = NULL;
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

	if ( *retCount + s->nfont == 0) {
		FcFontSetDestroy(s);
		return array;
	}

	/* make dynamic */
	i = sizeof(Font) * (*retCount + s-> nfont * ALL_CHARSETS);
	newarray = array ? realloc(array,i) : malloc(i);
	if ( !newarray) {
		FcFontSetDestroy(s);
		return array;
	}

	ppat = s-> fonts;
	f = newarray + *retCount;
	bzero( f, sizeof( Font) * s-> nfont * ALL_CHARSETS);

	names = hash_create();

	for ( i = 0; i < s->nfont; i++, ppat++) {
		FcCharSet *c = NULL;
		prima_fc_pattern2font( *ppat, f);
		FcPatternGetCharSet( *ppat, FC_CHARSET, 0, &c);
		if ( c && FcCharSetCount(c) == 0) continue;
		if ( encoding) {
			/* case 1 - encoding is set, filter only given encoding */

			if ( c && ((signed) FcCharSetIntersectCount( csi-> fcs, c) >= csi-> glyphs - 1)) {
				if ( !facename) {
					/* and, if no facename set, each facename is reported only once */
					if ( hash_fetch( names, f-> name, strlen( f-> name))) continue;
					hash_store( names, f-> name, strlen( f-> name), ( void*)1);
				}
				strlcpy( f-> encoding, encoding, 255);
				f++;
			}
		} else if ( facename) {
			/* case 2 - facename only is set, report each facename with every encoding */
			int j;
			Font * tmpl = f;
			for ( j = 0; j < STD_CHARSETS; j++) {
				if ( !std_charsets[j]. enabled) continue;
				if ( FcCharSetIntersectCount( c, std_charsets[j]. fcs) >= std_charsets[j]. glyphs - 1) {
					*f = *tmpl;
					strlcpy( f-> encoding, std_charsets[j]. name, 255);
					f++;
				}
			}
			/* if no encodings found, assume fontspecific, otherwise always provide iso10646 */
			prima_fc_pattern2font( *ppat, f);
			strcpy( f-> encoding, (f == tmpl) ? fontspecific : utf8_encoding);
			f++;
		} else if ( !facename && !encoding) {
			/* case 3 - report unique facenames and store list of encodings
				into the hack array */
			if ( hash_fetch( names, f-> name, strlen( f-> name)) == (void*)1) continue;
			hash_store( names, f-> name, strlen( f-> name), (void*)1);
			if ( c) {
				int j, found = 0;
				char ** enc = (char**) f-> encoding;
				unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
				for ( j = 0; j < STD_CHARSETS; j++) {
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
				*(enc + ++(*shift)) = found ? utf8_encoding : fontspecific;
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
prima_fc_font_encodings( PHash hash)
{
	int i;
	for ( i = 0; i < STD_CHARSETS; i++) {
		if ( !std_charsets[i]. enabled) continue;
		hash_store( hash, std_charsets[i]. name, strlen(std_charsets[i]. name), (void*) (std_charsets + i));
	}
	hash_store( hash, utf8_encoding, strlen(utf8_encoding), (void*) &utf8_charset);
}

static Bool
fc_add_item( unsigned long ** list, int * count, int * size, FcChar32 chr,
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
prima_fc_get_font_ranges( FcCharSet *c, int * count)
{
	FcChar32 ucs4, last = 0, haslast = 0;
	FcChar32 map[FC_CHARSET_MAP_SIZE];
	FcChar32 next;
	int size = 16;
	unsigned long * ret;

#define ADD(ch,flag) if(!fc_add_item(&ret,count,&size,ch,flag)) return ret

	*count = 0;
	if ( !c) return false;
	if ( !( ret = malloc( sizeof( unsigned long) * size))) return NULL;

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

char *
prima_fc_get_font_languages( FcPattern *pat)
{
#if FC_VERSION >= 21100
	FcLangSet *ls;
	FcStrSet  *ss;
	FcStrList *l;
	FcChar8  *s;
	char *ret, *p;
	int size;

	FcPatternGetLangSet(pat, FC_LANG, 0, &ls);
	if ( !ls )
		return NULL;
	if ( !(ss = FcLangSetGetLangs(ls)))
		return NULL;
	if ( !(l = FcStrListCreate (ss)))
		return NULL;

	size = 1024; /* longest line from 'fc-list -v' is 779 */
	if ( !(p = ret = malloc(size)))
		goto FAIL;

	FcStrListFirst(l);
	while ((s = FcStrListNext(l)) != NULL) {
		int len = strlen((char*)s);
		if ( p - ret + len + 1 + 1 > size ) {
			char * p2;
			size *= 2;
			if ( !( p2 = realloc(ret, size)))
				goto FAIL;
			p   = p2 + (p - ret);
			ret = p2;
		}
		strcpy( p, (char*) s );
		p += len + 1;
	}
	*p = 0;
	FcStrListDone(l);
	return ret;

FAIL:
	FcStrListDone(l);
	free(ret);
#endif
	return NULL;
}

uint32_t *
prima_fc_map8( const char * encoding)
{
	CharSetInfo * csi;
	if ( !( csi = hash_fetch( encodings, encoding, strlen( encoding))))
		csi = locale;
	return csi-> map;
}

const char *
prima_fc_find_encoding( const char *encoding )
{
	CharSetInfo *csi;
	csi = ( CharSetInfo*) hash_fetch( encodings, encoding, strlen( encoding));
	if (!csi)
		csi = locale;
	return csi->name;
}

int
prima_fc_load_font( char* filename)
{
	int count = 0;
	struct stat s;
	FcPattern * font;
	FcConfig * config;
	FcFontSet *fonts;

	/* -f $filename or die */
	if (stat( filename, &s) < 0) {
		warn("%s", strerror(errno));
		return 0;
	}

#define FAIL(msg) { warn(msg); return 0; }

	if (( s.st_mode & S_IFDIR) != 0)
		FAIL("Must not be a directory")
	if ( !( font = FcFreeTypeQuery ((const FcChar8*)filename, 0, NULL, &count)))
		FAIL("Format not recognized")
	FcPatternDestroy (font);

	if ( count == 0 )
		FAIL("No fonts found in file")
	if ( !(config = FcConfigGetCurrent()))
		FAIL("FcConfigGetCurrent error")
	if ( !( fonts = FcConfigGetFonts(config, FcSetSystem)))
		FAIL("FcConfigGetFonts(FcSetSystem) error")
	if ( !( FcFileScan(fonts, NULL, NULL, NULL, (const FcChar8*)filename, FcFalse)))
		FAIL("FcFileScan error")
	prima_fc_init_font_substitution();

	return count;
}

void
prima_fc_set_font( Handle self, PFont font )
{
	DEFXX;
	Bool need_matrix, need_rotation;

	XX-> fc_map8 = prima_fc_map8(font->encoding);

	need_matrix   = !prima_matrix_is_translated_only( MY_MATRIX );
	need_rotation = !IS_ZERO(font-> direction);
	if ( need_matrix || need_rotation ) {
		Matrix m1, m2;
		prima_matrix_set_identity(m1);
		if ( need_rotation ) {
			double s = sin( font-> direction / 57.29577951);
			double c = cos( font-> direction / 57.29577951);
			m1[0] = c;
			m1[1] = s;
			m1[2] = -s;
			m1[3] = c;
		}
		COPY_MATRIX_WITHOUT_TRANSLATION( MY_MATRIX, m2 );
		prima_matrix_multiply( m1, m2, XX->fc_font_matrix );
	} else {
		prima_matrix_set_identity(XX->fc_font_matrix);
	}
}

#define FCAM_OK           0
#define FCAM_TRY_MONO     1
#define FCAM_EMULATE_MONO 2
#define FCAM_REDO         3

int
prima_fc_suggest_pitch( PFont requested_font, PFont suggested_font, FcPattern *match)
{
	/* xft does a rather bad job with synthesizing a monospaced
	font out of a proportional one ... try to find one myself,
	or bail out if it is the case
	*/

	*suggested_font = *requested_font;

	if ( requested_font-> pitch == fpFixed && !guts.force_fc_monospace_emulation) {
		int spacing = -1;

		if (
			( FcPatternGetInteger( match, FC_SPACING, 0, &spacing) == FcResultMatch) &&
			( spacing != FC_MONO )
		) {
			Font font_with_family;
			char * monospace_font;
			font_with_family = *requested_font;
			prima_fc_pattern2fontnames(match, &font_with_family);

			if (!guts.try_fc_monospace_emulation_by_name && ( monospace_font = prima_fc_find_good_font_by_family(&font_with_family, FC_MONO))) {
				/* try a good mono font, again */
				strcpy(suggested_font->name, monospace_font);
				suggested_font->undef.name = 0;
				FCdebug("try fixed pitch");
				guts.try_fc_monospace_emulation_by_name++;
				guts.debug_indent++;
				return FCAM_TRY_MONO;
			} else {
				FCdebug("force ugly monospace");
				guts.force_fc_monospace_emulation++;
				guts.debug_indent++;
				return FCAM_EMULATE_MONO;
			}
		}
	} else if ( requested_font->pitch == fpVariable ) {
		/*
			xft picks a monospaced font when a proportional one is requested if the name points at it.
			Not that this is wrong, but in Prima terms pitch is heavier than name (this concept was borrowed from win32).
			So try to pick a variable font of the same family, if there is one. Same algorithm as with fixed fonts,
			but not as strict - if we can't find a proportional font within same family, so be it then
		*/
		int spacing = -1;

		if (
			( FcPatternGetInteger( match, FC_SPACING, 0, &spacing) == FcResultMatch) &&
			( spacing == FC_MONO ) /* for our purpose all what is not FC_MONO is good enough to be fpVariable */
		) {
			char * proportional_font;
			prima_fc_pattern2fontnames(match, suggested_font);

			if (( proportional_font = prima_fc_find_good_font_by_family(suggested_font, FC_PROPORTIONAL))) {
				/* try a good variable font, again */
				suggested_font->undef.name = 0;
				strcpy(suggested_font->name, proportional_font);
				FCdebug("try variable pitch");
				guts.debug_indent++;
				return FCAM_REDO;
			} else {
				FCdebug("variable pitch is not found within family %s", suggested_font->family);
			}
		}

	}

	return FCAM_OK;
}

void
prima_fc_end_suggestion(int flag)
{
	switch (flag) {
	case FCAM_TRY_MONO:
		guts.try_fc_monospace_emulation_by_name--;
		guts.debug_indent--;
		FCdebug("fixed pitch done");
		break;
	case FCAM_EMULATE_MONO:
		guts.debug_indent--;
		guts.force_fc_monospace_emulation--;
		FCdebug("emulated mono done");
		break;
	case FCAM_REDO:
		guts.debug_indent--;
		break;
	}
}

Bool
prima_fc_encoding_is_supported( char *encoding, FcPattern *match)
{
	/* Manually check if font contains wanted encoding - matching by FcCharSet
		can't set threshold on how many glyphs can be omitted */
	if ( !(
		(strcmp( encoding, "fontspecific") != 0) ||
		(strcmp( encoding, "iso10646-1"  ) != 0)
	)) {
		FcCharSet *c = NULL;
		FcPatternGetCharSet( match, FC_CHARSET, 0, &c);
		if ( c && (FcCharSetCount(c) == 0)) {
			FCdebug("charset mismatch (%s)", encoding);
			return false;
		}
	}

	return true;
}


#endif
