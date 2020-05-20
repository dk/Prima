/********************************/
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

#ifdef WITH_HARFBUZZ
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
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

static int xft_debug_indent = 0;
#define XFTdebug if (pguts->debug & DEBUG_FONTS) xft_debug

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
#define MAX_GLYPH_SIZE (guts.limits.request_length / 256)

#define ROUND_DIRECTION 1000.0
#define IS_ZERO(a)  ((int)(a*ROUND_DIRECTION)==0)
#define ROUGHLY(a) (((int)(a*ROUND_DIRECTION))/ROUND_DIRECTION)


static PHash encodings    = NULL;
static PHash mono_fonts   = NULL; /* family->mono font mapping */
static PHash prop_fonts   = NULL; /* family->proportional font mapping */
static PHash mismatch     = NULL; /* fonts not present in xft base */
static PHash myfont_cache = NULL; /* fonts loaded temporarily */
static char  fontspecific[] = "fontspecific";
static char  utf8_encoding[] = "iso10646-1";
static CharSetInfo * locale = NULL;

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

typedef struct {
	Font font;
	XftFont *orig, *xft_font;
	XftFont *orig_base, *xft_base_font;
	uint32_t fid;
	uint16_t *fonts;
} FontContext;

static void
font_context_init( FontContext * fc, Font * font, uint16_t * fonts, XftFont * orig, XftFont * base)
{
	bzero(fc, sizeof(FontContext));
	fc->font      = *font;
	fc->orig      = fc->xft_font = orig;
	fc->orig_base = fc->xft_base_font = base;
	fc->fid       = 0;
	fc->fonts     = fonts;
}

static void
font_context_next( FontContext * fc )
{
	Font *_src, src, dst;
	uint16_t nfid;

	if ( !fc-> fonts ) return;

	nfid = *(fc->fonts++);
	if ( nfid == fc-> fid ) return;

	fc->fid = nfid;
	if ( nfid == 0 ) {
		fc->xft_font = fc->orig;
		fc->xft_base_font = fc->orig_base;
		return;
	}

	if ( !( _src = prima_font_mapper_get_font(nfid)))
		return;

	src = *_src;
	dst = fc->font;
	src.size = dst.size;
	src.undef.size = 0;

	prima_xft_font_pick( nilHandle, &src, &dst, NULL, &fc->xft_font);
	if ( !fc->orig_base )
		return;
	
	if ( IS_ZERO(fc->font.direction))
		fc-> xft_base_font = fc->xft_font;
	else {
		dst.direction = 0;
		prima_xft_font_pick( nilHandle, &dst, &dst, NULL, &fc->xft_base_font);
	}
}


static void
xft_debug( const char *format, ...)
{
	int i;
	va_list args;
	va_start( args, format);
	fprintf( stderr, "xft: ");
	for ( i = 0; i < xft_debug_indent * 3; i++) fprintf( stderr, " ");
	vfprintf( stderr, format, args);
	fprintf( stderr, "\n");
	va_end( args);
}

void
prima_xft_init(void)
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
	XFTdebug("XFT ok");

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

	mismatch     = hash_create();
	mono_fonts   = hash_create();
	prop_fonts   = hash_create();
	encodings    = hash_create();
	myfont_cache = hash_create();
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
}

static Bool
remove_myfonts( void * f, int keyLen, void * key, void * dummy)
{
	unlink((char*) key);
	return false;
}

void
prima_xft_done(void)
{
	int i;
	if ( !guts. use_xft) return;
	for ( i = 0; i < STD_CHARSETS; i++)
		if ( std_charsets[i]. fcs)
			FcCharSetDestroy( std_charsets[i]. fcs);
	FcCharSetDestroy( fontspecific_charset. fcs);
	FcCharSetDestroy( utf8_charset. fcs);
	hash_destroy( encodings, false);
	hash_destroy( mismatch, false);
	hash_destroy( prop_fonts, true);
	hash_destroy( mono_fonts, true);

	hash_first_that( myfont_cache, (void*)remove_myfonts, NULL, NULL, NULL);
	hash_destroy( myfont_cache, false);
	myfont_cache = NULL;

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

static void
fcpattern2fontnames( FcPattern * pattern, Font * font)
{
	FcChar8 * s;

	if ( FcPatternGetString( pattern, FC_FAMILY, 0, &s) == FcResultMatch)
		font-> is_utf8.name = utf8_flag_strncpy( font-> name, (char*)s, 255);
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

static void
fcpattern2font( FcPattern * pattern, PFont font)
{
	int i, j;
	double d = 1.0;
	FcCharSet *c = NULL;

	/* FcPatternPrint( pattern); */
	fcpattern2fontnames(pattern, font);

	font-> style = 0;
	font-> undef. style = 0;
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

	if ( FcPatternGetInteger( pattern, FC_SPACING, 0, &i) == FcResultMatch) {
		font-> pitch = (( i == FC_PROPORTIONAL) ? fpVariable : fpFixed);
		font-> undef. pitch = 0;
	}

	if ( FcPatternGetDouble( pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch) {
		font->height = d + 0.5;
		font-> undef. height = 0;
		XFTdebug("height factor read:%d (%g)", font-> height, d);
	}

	font-> width = 100; /* warning, FC_WIDTH does not reflect FC_MATRIX scale changes */
	if ( FcPatternGetDouble( pattern, FC_WIDTH, 0, &d) == FcResultMatch) {
		font->width = d + 0.5;
		XFTdebug("width factor read:%d (%g)", font-> width, d);
	}
	font-> width = ( font-> width * font-> height) / 100.0 + .5;
	font->undef. width = 0;

	if ( FcPatternGetDouble( pattern, FC_SIZE, 0, &d) == FcResultMatch) {
		font->size = d + 0.5;
		font->undef. size = 0;
		XFTdebug("size factor read:%d (%g)", font-> size, d);
	} else if (!font-> undef.height && font->yDeviceRes != 0) {
		font-> size = font-> height * 72.27 / font-> yDeviceRes + .5;
		font-> undef. size = 0;
		XFTdebug("size calculated:%d", font-> size);
	} else {
		XFTdebug("size unknown");
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

static void
xft_build_font_key( PFontKey key, PFont f, Bool bySize)
{
	bzero( key, sizeof( FontKey));
	key-> height = bySize ? -f-> size : f-> height;
	key-> width  = f-> width;
	key-> style  = f-> style & ~(fsUnderlined|fsOutline|fsStruckOut) & fsMask;
	key-> pitch  = f-> pitch & fpMask;
	key-> vector = (f-> vector == fvBitmap) ? 0 : 1;
	key-> direction = ROUGHLY(f-> direction);
	strcpy( key-> name, f-> name);
}

static XftFont *
try_size( Handle self, Font f, double size)
{
	XftFont * xft = NULL;
	bzero( &f.undef, sizeof(f.undef));
	f. undef. height = f. undef. width = 1;
	f. size = size + 0.5;
	f. direction = 0.0;
	xft_debug_indent++;
	prima_xft_font_pick( self, &f, &f, &size, &xft);
	xft_debug_indent--;
	return xft;
}

/* find a most similar monospace/proportional font by name and family */
static char *
find_good_font_by_family( Font * f, int fc_spacing )
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

			fcpattern2fontnames( *ppat, &f);
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
	XFTdebug("trying to find %s pitch replacement for %s/%s", 
		(fc_spacing == FC_MONO ) ? "fixed" : "variable",
		f->name, f->family);

	/* try to find same family and same 1st word in font name */
	{
		char *c, *w, word1[255], word2[255];
		PHash font_hash = (fc_spacing == FC_MONO) ? mono_fonts : prop_fonts;
		c = hash_fetch( font_hash, f->family, strlen(f->family));
		if ( !c ) {
			XFTdebug("no default font for that family");
			return NULL;
		}
		if ( strcmp( c, f->name) == 0) {
			XFTdebug("same font");
			return NULL; /* same font */
		}

		strcpy( word1, c);
		strcpy( word2, f->name);
		if (( w = strchr( word1, ' '))) *w = 0;
		if (( w = strchr( word2, ' '))) *w = 0;
		if ( strcmp( word1, word2 ) != 0 ) {
			XFTdebug("default font %s doesn't look similar", c);
			return NULL;
		}
		XFTdebug("replaced with %s", c);

		return c;
	}
}

static void
xft_store_font(Font * k, Font * v, Bool by_size, XftFont * xft, XftFont * xft_base)
{
	FontKey key;
	PCachedFont kf;
	xft_build_font_key( &key, k, by_size);
	if ( !hash_fetch( guts. font_hash, &key, sizeof(FontKey))) {
		if (( kf = malloc( sizeof( CachedFont)))) {
			bzero( kf, sizeof( CachedFont));
			memcpy( &kf-> font, v, sizeof( Font));
			kf-> font. style &= ~(fsUnderlined|fsOutline|fsStruckOut) & fsMask;
			kf-> xft      = xft;
			kf-> xft_base = xft_base;
			hash_store( guts. font_hash, &key, sizeof( FontKey), kf);
			XFTdebug("store %x(%x):%dx%d.%s.%s.%s^%g", xft, xft_base, key.height, key.width, _F_DEBUG_STYLE(key.style), _F_DEBUG_PITCH(key.pitch), key.name, ROUGHLY(key.direction));
		}
	}
}

static int force_xft_monospace_emulation = 0;
static int try_xft_monospace_emulation_by_name = 0;

/* Bug #128146: libXft might use another shared libfontconfig library, as
 * observed on a badly configured macos, and thus FcPattern* returned by it
 * can crash everything */
static FcPattern *
my_XftFontMatch(Display        *dpy,
              int               screen,
              _Xconst FcPattern *pattern,
              FcResult          *result)
{
    FcPattern   *new;
    FcPattern   *match;
    new = FcPatternDuplicate (pattern);
    if (!new)
        return NULL;
    FcConfigSubstitute (NULL, new, FcMatchPattern);
    XftDefaultSubstitute (dpy, screen, new);
    match = FcFontMatch (NULL, new, result);
    FcPatternDestroy (new);
    return match;
}

Bool
prima_xft_font_pick( Handle self, Font * source, Font * dest, double * size, XftFont ** xft_result)
{
	FcPattern *request, *match;
	FcResult res = FcResultNoMatch;
	Font requested_font, loaded_font;
	Bool by_size;
	CharSetInfo * csi;
	XftFont * xf;
	FontKey key;
	PCachedFont kf, kf_base = NULL;
	int i, base_width = 1, exact_pixel_size = 0, cache_results = 1;
	double pixel_size;

	if ( !guts. use_xft) return false;

	requested_font = *dest;
	by_size = Drawable_font_add( self, source, &requested_font);
	pixel_size = requested_font. height;

	if ( guts. xft_disable_large_fonts > 0) {
		/* xft is unable to deal with large polygon requests.
			we must cut the large fonts here, before Xlib croaks */
		if (
			( by_size && ( requested_font. size >= MAX_GLYPH_SIZE)) ||
			(!by_size && ( requested_font. height >= MAX_GLYPH_SIZE / 72.27 * guts. resolution. y))
		)
			return false;
	}

	/* we don't want to cache fractional sizes because these can lead to incoherent results
		depending on whether we match a particular height by size or by height */
	if ( by_size && size && *size * 100 != requested_font.size * 100 ) {
		cache_results = 0;
		XFTdebug("not caching results because size %g is fractional", *size);
	}

	/* see if the font is not present in xft - the hashed negative matches
			are stored with width=0, as the width alterations are derived */
	xft_build_font_key( &key, &requested_font, by_size);
	XFTdebug("want %dx%d.%s.%s.%s/%s^%g.%d", key.height, key. width, _F_DEBUG_STYLE(key.style), _F_DEBUG_PITCH(key.pitch), key.name, requested_font.encoding, ROUGHLY(requested_font.direction), requested_font.vector);

	key. width = 0;
	if ( hash_fetch( mismatch, &key, sizeof( FontKey))) {
		XFTdebug("refuse");
		return false;
	}
	key. width = requested_font. width;

	/* convert encoding */
	csi = ( CharSetInfo*) hash_fetch( encodings, requested_font. encoding, strlen( requested_font. encoding));
	if ( !csi) {
		/* xft has no such encoding, pass it back */
		if ( prima_core_font_encoding( requested_font. encoding) || !guts. xft_priority)
			return false;
		csi = locale;
	}

	/* see if cached font exists */
	if (( kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
		*dest = kf-> font;
		strcpy( dest-> encoding, csi-> name);
		if ( requested_font. style & fsStruckOut) dest-> style |= fsStruckOut;
		if ( requested_font. style & fsUnderlined) dest-> style |= fsUnderlined;
		if ( xft_result ) *xft_result = kf-> xft;
		return true;
	}
	/* see if the non-xscaled font exists */
	if ( key. width != 0) {
		key. width = 0;
		if ( !( kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
			Font s = *source, d = *dest;
			s. width = d. width = 0;
			XFTdebug("try nonscaled font");
			xft_debug_indent++;
			prima_xft_font_pick( self, &s, &d, size, NULL);
			xft_debug_indent--;
		}
		if ( kf || ( kf = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
			base_width = kf-> font. width;
			if ( FcPatternGetDouble( kf-> xft-> pattern, FC_PIXEL_SIZE, 0, &pixel_size) == FcResultMatch) {
				exact_pixel_size = 1;
				XFTdebug("existing base font %x %dx0 suggests exact_pixel_size %g base_width %d", kf->xft, key.height, pixel_size, base_width);
			}
		} else { /* if fails, cancel x scaling and see if it failed due to banning */
			if ( hash_fetch( mismatch, &key, sizeof( FontKey))) return false;
			requested_font. width = 0;
		}
	}
	/* see if the non-rotated font exists */
	if ( !IS_ZERO(key. direction)) {
		key. direction = 0.0;
		key. width = requested_font. width;
		if ( !( kf_base = hash_fetch( guts. font_hash, &key, sizeof( FontKey)))) {
			Font s = *source, d = *dest;
			s. direction = d. direction = 0.0;
			XFTdebug("try nonrotated font");
			xft_debug_indent++;
			prima_xft_font_pick( self, &s, &d, size, NULL);
			xft_debug_indent--;
			/* if fails, cancel rotation and see if the base font is banned  */
			if ( !( kf_base = hash_fetch( guts. font_hash, &key, sizeof( FontKey))))
				requested_font. direction = 0.0;
		}
		if ( !IS_ZERO(requested_font. direction)) {
			/* as requested_font. height != FC_PIXEL_SIZE, read the correct request
				from the non-rotated font */
			if ( FcPatternGetDouble( kf_base-> xft-> pattern, FC_PIXEL_SIZE, 0, &pixel_size) == FcResultMatch) {
				XFTdebug("existing base font %x %dx%d dir=0 suggests exact_pixel_size %g", kf_base->xft, key.height, key.width, pixel_size);
				exact_pixel_size = 1;
			}
		}
	}

	/* create FcPattern request */
	if ( !( request = FcPatternCreate())) return false;
	if ( strcmp( requested_font. name, "Default") != 0)
		FcPatternAddString( request, FC_FAMILY, ( FcChar8*) requested_font. name);
	if ( by_size) {
		if ( size) {
			FcPatternAddDouble( request, FC_SIZE, *size);
			XFTdebug("FC_SIZE = %.1f", *size);
		} else {
			FcPatternAddDouble( request, FC_SIZE, requested_font. size);
			XFTdebug("FC_SIZE = %d", requested_font. size);
		}
	} else {
		FcPatternAddDouble( request, FC_PIXEL_SIZE, pixel_size);
		XFTdebug("FC_PIXEL_SIZE = %g", pixel_size);
	}
	FcPatternAddInteger( request, FC_SPACING,
		(requested_font. pitch == fpFixed && force_xft_monospace_emulation) ? FC_MONO : FC_PROPORTIONAL);

	FcPatternAddInteger( request, FC_SLANT, ( requested_font. style & fsItalic) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
	FcPatternAddInteger( request, FC_WEIGHT,
		( requested_font. style & fsBold) ? FC_WEIGHT_BOLD :
		( requested_font. style & fsThin) ? FC_WEIGHT_THIN : FC_WEIGHT_NORMAL);
	FcPatternAddBool( request, FC_SCALABLE, requested_font. vector > fvBitmap );
	if ( !IS_ZERO(requested_font. direction) || requested_font. width != 0) {
		FcMatrix mat;
		FcMatrixInit(&mat);
		if ( requested_font. width != 0) {
			FcMatrixScale( &mat, ( double) requested_font. width / base_width, 1);
			XFTdebug("FcMatrixScale %g", ( double) requested_font. width / base_width);
		}
		if ( !IS_ZERO(requested_font. direction))
			FcMatrixRotate( &mat, cos(requested_font.direction * 3.14159265358 / 180.0), sin(requested_font.direction * 3.14159265358 / 180.0));
		FcPatternAddMatrix( request, FC_MATRIX, &mat);
	}

	if ( guts. xft_no_antialias)
		FcPatternAddBool( request, FC_ANTIALIAS, 0);

	/* match best font - must return something useful; the match is statically allocated */
	match = my_XftFontMatch( DISP, SCREEN, request, &res);
	if ( !match) {
		XFTdebug("XftFontMatch error");
		FcPatternDestroy( request);
		return false;
	}
	/* if (pguts->debug & DEBUG_FONTS) { FcPatternPrint(match); } */
	FcPatternDestroy( request);

	/* xft does a rather bad job with synthesizing a monospaced
	font out of a proportional one ... try to find one ourself,
	or bail out if it is the case
	*/
	if ( requested_font.pitch == fpFixed && !force_xft_monospace_emulation) {
		int spacing = -1;

		if (
			( FcPatternGetInteger( match, FC_SPACING, 0, &spacing) == FcResultMatch) &&
			( spacing != FC_MONO )
		) {
			Font font_with_family;
			char * monospace_font;
			font_with_family = requested_font;
			fcpattern2fontnames(match, &font_with_family);
			FcPatternDestroy( match);

			if (!try_xft_monospace_emulation_by_name && ( monospace_font = find_good_font_by_family(&font_with_family, FC_MONO))) {
				/* try a good mono font, again */
				Bool ret;
				Font s = *source;
				strcpy(s.name, monospace_font);
				s.undef.name = 0;
				XFTdebug("try fixed pitch");
				try_xft_monospace_emulation_by_name++;
				ret = prima_xft_font_pick( self, &s, dest, size, xft_result);
				try_xft_monospace_emulation_by_name--;
				XFTdebug("fixed pitch done");
				return ret;
			} else {
				Bool ret;
				XFTdebug("force ugly monospace");
				force_xft_monospace_emulation++;
				ret = prima_xft_font_pick( self, source, dest, size, xft_result);
				force_xft_monospace_emulation--;
				return ret;
			}
		}
	} else if ( requested_font.pitch == fpVariable ) {
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
			Font font_with_family;
			char * proportional_font;
			font_with_family = requested_font;
			fcpattern2fontnames(match, &font_with_family);

			if (( proportional_font = find_good_font_by_family(&font_with_family, FC_PROPORTIONAL))) {
				/* try a good variable font, again */
				Font s = *source;
				s.undef.name = 0;
				strcpy(s.name, proportional_font);
				XFTdebug("try variable pitch");
				FcPatternDestroy( match);
				return prima_xft_font_pick( self, &s, dest, size, xft_result);
			} else {
				XFTdebug("variable pitch is not found within family %s", font_with_family.family);
			}
		}

	}

	/* Manually check if font contains wanted encoding - matching by FcCharSet
		can't set threshold on how many glyphs can be omitted */
	if ( !(
		(strcmp( requested_font. encoding, fontspecific) != 0) ||
		(strcmp( requested_font. encoding, utf8_encoding) != 0)
	)) {
		FcCharSet *c = NULL;
		FcPatternGetCharSet( match, FC_CHARSET, 0, &c);
		if ( c && (FcCharSetCount(c) == 0)) {
			XFTdebug("charset mismatch (%s)", requested_font. encoding);
			FcPatternDestroy( match);
			return false;
		}
	}

	/* Check if the matched font is scalable -- see comments in the beginning
		of the file about non-scalable fonts in Xft */
	if ( requested_font. vector > fvBitmap ) {
		FcBool scalable;
		if (( FcPatternGetBool( match, FC_SCALABLE, 0, &scalable) == FcResultMatch) && !scalable) {
			xft_build_font_key( &key, &requested_font, by_size);
			key. width = 0;
			hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
			XFTdebug("refuse bitmapped font");
			FcPatternDestroy( match);
			return false;
		}
	}

	/* XXX copy font details - very important these are correct !!! */
	/* strangely enough, if the match is used after XftFontOpenPattern, it is
		destroyed */
	loaded_font = requested_font;
	if ( !kf_base) {
		Bool underlined = loaded_font. style & fsUnderlined;
		Bool strike_out  = loaded_font. style & fsStruckOut;
		fcpattern2font( match, &loaded_font);
		if ( requested_font. width > 0) loaded_font. width = requested_font. width;
		if ( underlined) loaded_font. style |= fsUnderlined;
		if ( strike_out) loaded_font. style |= fsStruckOut;
	}


	/* check name match */
	{
		FcChar8 * s = NULL;
		FcPatternGetString( match, FC_FAMILY, 0, &s);
		if ( !s || strcmp(( const char*) s, requested_font. name) != 0) {
			int i, n = guts. n_fonts;
			PFontInfo info = guts. font_info;

			if ( !guts. xft_priority) {
				XFTdebug("name mismatch");
			NAME_MISMATCH:
				xft_build_font_key( &key, &requested_font, by_size);
				key. width = 0;
				hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
				FcPatternDestroy( match);
				return false;
			}

			/* check if core has cached face name */
			if ( prima_find_known_font( &requested_font, false, by_size)) {
				XFTdebug("pass to cached core");
				goto NAME_MISMATCH;
			}

			/* check if core has non-cached face name */
			for ( i = 0; i < n; i++) {
				if (
					info[i]. flags. disabled ||
					!info[i].flags.name ||
					(strcmp( info[i].font.name, requested_font.name) != 0)
				) continue;
				XFTdebug("pass to core");
				goto NAME_MISMATCH;
			}
		}
	}

	/* load the font */
	xf = XftFontOpenPattern( DISP, match);
	if ( !xf) {
		xft_build_font_key( &key, &requested_font, by_size);
		key. width = 0;
		hash_store( mismatch, &key, sizeof( FontKey), (void*)1);
		XFTdebug("XftFontOpenPattern error");
		FcPatternDestroy( match);
		return false;
	}
	XFTdebug("load font %x", xf);

	if ( kf_base) {
		/* A bit hacky, since the rotated font may be substituted by Xft.
		We skip the non-scalable fonts earlier to assure this doesn't happen,
		but anyway it's not 100% */
		Bool underlined = loaded_font. style & fsUnderlined;
		Bool strike_out  = loaded_font. style & fsStruckOut;
		loaded_font = kf_base-> font;
		loaded_font. direction = requested_font. direction;
		strcpy( loaded_font. encoding, csi-> name);
		if ( underlined) loaded_font. style |= fsUnderlined;
		if ( strike_out) loaded_font. style |= fsStruckOut;
	} else {
		loaded_font. internalLeading = xf-> height - loaded_font. size * guts. resolution. y / 72.27 + 0.5;
		if ( !by_size && !exact_pixel_size) {
			/* Try to locate the corresponding size and
			the correct height - FC_PIXEL_SIZE is not correct most probably
			multiply size by 10 to address pixel-wise heights correctly.
			*/
			HeightGuessStack hgs;
			int h, sz, last_sz = -1;
			XftFont * guessed_font = NULL;

			sz = 10.0 * (float) loaded_font. size * (float) loaded_font. height / (float) xf->height;
			XFTdebug("need to figure the corresponding size - try %g first...", ( double) sz / 10.0);
			guessed_font = try_size( self, loaded_font, ( double) sz / 10.0);

			if ( guessed_font) {
				h = guessed_font-> height;
				XFTdebug("got height = %d", h);
				if ( h != requested_font. height) {
					XFTdebug("not good enough, try approximation from size %g", ( double) sz / 10.0);
					prima_init_try_height( &hgs, requested_font. height, sz);
					while ( 1) {
						last_sz = sz;
						sz = prima_try_height( &hgs, h);
						if ( sz < 0) break;
						guessed_font = try_size( self, loaded_font, ( double) sz / 10.0);
						if ( !guessed_font) break;
						h = guessed_font-> height;
						XFTdebug("size %.1f got us %d pixels", ( float) sz / 10.0, h);
						if ( h == 0 || h == requested_font. height) break;
					}
				}
				if ( sz < 0) sz = last_sz;
				if ( sz > 0) {
					loaded_font. size = (double) sz / 10.0 + 0.5;
					XFTdebug("found size: %.1f (%d)", ( float) sz / 10.0, loaded_font. size);
				}
			} else {
				XFTdebug("found nothing");
			}

			if ( guessed_font && requested_font. height != xf-> height) {
				xf = guessed_font;
				loaded_font. height = xf-> height;
				XFTdebug("redirect to font %x", xf);
			}
			XFTdebug("guessed size %d", loaded_font.size);
		} else {
			loaded_font. height = xf-> height;
			XFTdebug("set height: %d", loaded_font.height);
		}

		loaded_font. maximalWidth = xf-> max_advance_width;
		/* calculate average font width */
		if ( loaded_font. pitch != fpFixed) {
			FcChar32 c;
			XftFont *x = kf_base ? kf_base-> xft : xf;
			int num = 0, sum = 0;
			for ( c = 63; c < 126; c+=4) {
				FT_UInt ft_index;
				XGlyphInfo glyph;
				if ( !( ft_index = XftCharIndex( DISP, x, c))) continue;
				XftGlyphExtents( DISP, x, &ft_index, 1, &glyph);
				if ( glyph. xOff > 0 && glyph. xOff < xf->max_advance_width) {
					sum += glyph. xOff;
					num++;
				} else
					XFTdebug( "!! font %s returns bad XftGlyphExtents", loaded_font.name);
			}
			loaded_font. width = ( num > 10) ? ((float) sum / num + 0.5) : loaded_font. maximalWidth;
		} else
			loaded_font. width = loaded_font. maximalWidth;
	}

	{
		XftFont * base = kf_base ? kf_base-> xft : xf;
		loaded_font. descent = base-> descent;
		loaded_font. ascent  = base-> ascent;
	}

	if ( cache_results ) {
		/* create hash entry for subsequent loads of same font */
		xft_store_font(&requested_font, &loaded_font, by_size, xf, kf_base ? kf_base-> xft : xf);
		/* and with the matched by height and size */
		for ( i = 0; i < 2; i++)
			xft_store_font(&loaded_font, &loaded_font, i, xf, kf_base ? kf_base-> xft : xf);
	}

	*dest = loaded_font;
	if ( xft_result ) *xft_result = xf;
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
	if ( IS_ZERO(PDrawable( self)-> font. direction)) {
		XX-> xft_font_sin = 0.0;
		XX-> xft_font_cos = 1.0;
	} else {
		XX-> xft_font_sin = sin( font-> direction / 57.29577951);
		XX-> xft_font_cos = cos( font-> direction / 57.29577951);
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
	if ( !kf || !kf-> xft) return NULL;
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

	/* make dynamic */
	if (( *retCount + s->nfont == 0) || !( newarray = realloc( array, sizeof(Font) * (*retCount + s-> nfont * ALL_CHARSETS)))) {
		FcFontSetDestroy(s);
		return array;
	}
	ppat = s-> fonts;
	f = newarray + *retCount;
	bzero( f, sizeof( Font) * s-> nfont * ALL_CHARSETS);

	names = hash_create();

	for ( i = 0; i < s->nfont; i++, ppat++) {
		FcCharSet *c = NULL;
		fcpattern2font( *ppat, f);
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
				strncpy( f-> encoding, encoding, 255);
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
					strncpy( f-> encoding, std_charsets[j]. name, 255);
					f++;
				}
			}
			/* if no encodings found, assume fontspecific, otherwise always provide iso10646 */
			fcpattern2font( *ppat, f);
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
prima_xft_font_encodings( PHash hash)
{
	int i;
	for ( i = 0; i < STD_CHARSETS; i++) {
		if ( !std_charsets[i]. enabled) continue;
		hash_store( hash, std_charsets[i]. name, strlen(std_charsets[i]. name), (void*) (std_charsets + i));
	}
	hash_store( hash, utf8_encoding, strlen(utf8_encoding), (void*) &utf8_charset);
}

static FcChar32 *
xft_text2ucs4( const unsigned char * text, int len, Bool utf8, uint32_t * map8)
{
	FcChar32 *ret, *r;
	if ( utf8) {
		STRLEN charlen, bytelen = strlen(( const char *) text);
		(void)bytelen;

		if ( len < 0) len = prima_utf8_length(( char*) text, -1);
		if ( !( r = ret = malloc( len * sizeof( FcChar32)))) return NULL;
		while ( len--) {
			*(r++) = prima_utf8_uvchr(text, bytelen, &charlen);
			text += charlen;
			bytelen -= charlen;
			if ( charlen == 0 ) break;
		}
	} else {
		int i;
		if ( len < 0) len = strlen(( char*) text);
		if ( !( ret = malloc( len * sizeof( FcChar32)))) return NULL;
		for ( i = 0; i < len; i++)
			ret[i] = ( text[i] < 128) ? text[i] : map8[ text[i] - 128];
	}
	return ret;
}

/*
x11 has problems with text_out strings that are wider than
64K pixel - it wraps the coordinates and produces mess. This hack is
although ugly, but is better that X11 default behaviour, and
at least can be excused by the fact that all GP spaces have
their geometrical limits.
*/
static int
check_width(PCachedFont self, int len)
{
	int div;
	div = 65535L / (self-> font. maximalWidth ? self-> font. maximalWidth : 1);
	if ( div <= 0) div = 1;
	if ( len > div) len = div;
	return len;
}

#define UPDATE_OVERHANGS(_len,_flags)                                             \
	if ( i == 0) {                                                            \
		if (( _flags & toAddOverhangs ) && glyph. x > 0) ret += glyph. x; \
		if ( overhangs) overhangs-> x = glyph. x;                         \
	}                                                                         \
	if ( i == _len - 1) {                                                     \
		int c = glyph. xOff - glyph. width + glyph. x;                    \
		if ( (_flags & toAddOverhangs) && c < 0) ret -= c;                \
		if ( overhangs) overhangs-> y = -c;                               \
	}

int
prima_xft_get_text_width(
	PCachedFont self, const char * text, int len, int flags,
	uint32_t * map8, Point * overhangs
) {
	int i, ret = 0, bytelen = 0;
	XftFont * font = self-> xft_base;

	if ( overhangs) overhangs-> x = overhangs-> y = 0;
	if ( flags & toUTF8 ) bytelen = strlen(text);
	len = check_width(self, len);

	for ( i = 0; i < len; i++) {
		FcChar32 c;
		FT_UInt ft_index;
		XGlyphInfo glyph;
		if ( flags & toUTF8) {
			STRLEN charlen;
			c = ( FcChar32) prima_utf8_uvchr(text, bytelen, &charlen);
			text += charlen;
			bytelen -= charlen;
			if ( charlen == 0 ) break;
		} else if ( ((Byte*)text)[i] > 127) {
			c = map8[ ((Byte*)text)[i] - 128];
		} else
			c = text[i];
		ft_index = XftCharIndex( DISP, font, c);
		XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
		ret += glyph. xOff;
		if ( (flags & toAddOverhangs ) || overhangs) {
			UPDATE_OVERHANGS(len,flags)
		}
	}
	return ret;
}

int
prima_xft_get_glyphs_width( PCachedFont self, PGlyphsOutRec t, Point * overhangs)
{
	int i, ret = 0;
	FontContext fc;

	font_context_init(&fc, &self->font, t->fonts, self->xft, NULL);
	if ( overhangs) overhangs-> x = overhangs-> y = 0;

	t->len = check_width(self, t->len);
	for ( i = 0; i < t->len; i++) {
		FT_UInt ft_index;
		XGlyphInfo glyph;
		ft_index = t->glyphs[i];
		font_context_next(&fc);
		XftGlyphExtents( DISP, fc.xft_font, &ft_index, 1, &glyph);
		ret += glyph. xOff;
		if ( (t->flags & toAddOverhangs ) || overhangs) {
			UPDATE_OVERHANGS(t->len,t->flags)
		}
	}
	return ret;
}

static Point *
get_box( Handle self, Point * ovx, int advance ) 
{
	DEFXX;
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	if ( !pt) return NULL;

	if ( ovx->x < 0 ) ovx->x = 0;
	if ( ovx->y < 0 ) ovx->y = 0;

	pt[0].y = pt[2]. y = XX-> font-> font. ascent - 1;
	pt[1].y = pt[3]. y = - XX-> font-> font. descent;
	pt[4].y = 0;
	pt[4].x = advance;
	pt[3].x = pt[2]. x = advance + ovx->y;
	pt[0].x = pt[1]. x = - ovx->x;

	if ( !XX-> flags. paint_base_line) {
		int i;
		for ( i = 0; i < 4; i++) pt[i]. y += XX-> font-> font. descent;
	}

	if ( !IS_ZERO(PDrawable( self)-> font. direction)) {
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

Point *
prima_xft_get_text_box( Handle self, const char * text, int len, int flags)
{
	Point ovx;
	return get_box(self, &ovx, prima_xft_get_text_width(
		X(self)-> font, text, len, flags,
		X(self)-> xft_map8, &ovx)
	);
}

Point *
prima_xft_get_glyphs_box( Handle self, PGlyphsOutRec t)
{
	Point ovx;
	return get_box(self, &ovx, prima_xft_get_glyphs_width(
		X(self)-> font, t, &ovx
	));
}

static XftFont *
create_no_aa_font( XftFont * font)
{
	FcPattern * request;
	if (!( request = FcPatternDuplicate( font-> pattern))) return NULL;
	FcPatternDel( request, FC_ANTIALIAS);
	FcPatternAddBool( request, FC_ANTIALIAS, 0);
	return XftFontOpenPattern( DISP, request);
}

#define SORT(a,b)       { int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)       (XX-> size. y - (a) - 1)
#define SHIFT(a,b)      { (a) += XX-> gtransform. x + XX-> btransform. x; \
									(b) += XX-> gtransform. y + XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)

/* emulate win32 clearing off alpha bits on any Gdi operation */

#define EMULATE_ALPHA_CLEARING 0

static void
XftDrawGlyph_layered( PDrawableSysData selfxx, 
	_Xconst XftColor *color, XftFont * font, 
	int x, int y, _Xconst FT_UInt glyph
) {
	XftColor black;
	XGlyphInfo extents;

	XftGlyphExtents( DISP, font, &glyph, 1, &extents);

	if (
		!XX-> xft_shadow_pixmap ||
		extents. width  > XX-> xft_shadow_extentions.x ||
		extents. height > XX-> xft_shadow_extentions.y
	) {
		int w, h;
		if ( XX-> xft_shadow_pixmap ) {
			XFreePixmap( DISP, XX-> xft_shadow_pixmap);
			XftDrawDestroy( XX-> xft_shadow_drawable);
			XX-> xft_shadow_pixmap   = 0;
			XX-> xft_shadow_drawable = NULL;
		}
		w = extents. width * 4;
		h = extents. height * 4;
		if ( w < 32 ) w = 32;
		if ( h < 32 ) h = 32;
		XX-> xft_shadow_extentions. x = w;
		XX-> xft_shadow_extentions. y = h;
		XX-> xft_shadow_pixmap   = XCreatePixmap( DISP, guts.root, w, h, 1);
		XX-> xft_shadow_drawable = XftDrawCreateBitmap( DISP, XX-> xft_shadow_pixmap );
	}

	if ( !XX-> xft_shadow_gc ) {
		XGCValues gcv;
		gcv. foreground = 1;
		XX-> xft_shadow_gc = XCreateGC( DISP, XX-> xft_shadow_pixmap, GCForeground, &gcv);
	}

	XFillRectangle( DISP, XX-> xft_shadow_pixmap, XX-> xft_shadow_gc,
		0, 0, XX-> xft_shadow_extentions.x, XX-> xft_shadow_extentions.y);

	black.color.red   =
	black.color.green =
	black.color.blue  =
	black.pixel       = 0x1;
	black.color.alpha = 0x0;
	XftDrawGlyphs( XX-> xft_shadow_drawable, &black, font, extents.x, extents.y, &glyph, 1);
	XftDrawGlyphs( XX-> xft_drawable, color, font, x, y, &glyph, 1);

	XCopyPlane( DISP, XX-> xft_shadow_pixmap, XX-> gdrawable, XX-> gc, 0, 0, extents.width, extents.height,
		x - extents.x, y - extents.y, 1);
}

/* When plotting rotated fonts, xft does not account for the accumulated
	roundoff error, and thus the text line is shown at different angle
	than requested. We track this and align the reference point when it
	deviates from the ideal line */
static void
xft_draw_glyphs( PDrawableSysData selfxx,
		_Xconst XftColor *color, int x, int y,
		_Xconst FcChar32 *string, int len,
		PGlyphsOutRec t)
{
	XGCValues old_gcv, gcv;
	int i, ox, oy, shift;
	FontContext fc;
	uint16_t * advances  = t ? t->advances : NULL;
	int16_t  * positions = t ? t->positions : NULL;
	Bool straight = IS_ZERO(XX-> font-> font. direction);

	font_context_init(&fc, &XX->font->font, 
		t ? t->fonts : NULL, 
		XX->font->xft, advances ? NULL : XX->font->xft_base);

	ox = x;
	oy = y;
	shift = 0;
	if ( XX-> flags. layered && EMULATE_ALPHA_CLEARING) {
		FT_UInt ft_index;
		/* prepare xrender */
		XftDrawGlyphs( XX-> xft_drawable, color, XX->font->xft, x, y, &ft_index, 0);

		XGetGCValues( DISP, XX-> gc, GCFunction|GCBackground|GCForeground|GCPlaneMask, &old_gcv);
		gcv. foreground = 0xffffffff;
		gcv. background = 0x00000000;
		gcv. function   = GXand;
		gcv. plane_mask = guts. argb_bits. alpha_mask;
		XChangeGC( DISP, XX-> gc, GCFunction|GCBackground|GCForeground|GCPlaneMask, &gcv);
	}

	if (t) len = t->len;
	for ( i = 0; i < len; i++) {
		int cx, cy, dx = 0, dy = 0;
		FT_UInt ft_index;
		XGlyphInfo glyph;

		if ( t ) {
			ft_index = t->glyphs[i];
			font_context_next(&fc);
			if ( advances ) {
				register int x, y;
				shift += *(advances++);
				x = *(positions++);
				y = *(positions++);
				if ( straight ) {
					dx = x;
					dy = y;
				} else {
					dx = (int)(x * XX-> xft_font_cos + 0.5) - (int)(y * XX-> xft_font_sin + .5);
					dy = (int)(x * XX-> xft_font_sin + 0.5) + (int)(y * XX-> xft_font_cos + .5);
				}
			} else
				goto CHECK_EXTENTS;
		} else {
			ft_index = XftCharIndex( DISP, fc.xft_font, string[i]);
		CHECK_EXTENTS:
			XftGlyphExtents( DISP, fc.xft_base_font, &ft_index, 1, &glyph);
			shift += glyph. xOff;
		}
		if ( straight ) {
			cx = ox + shift;
			cy = oy;
		} else {
			cx = ox + (int)(shift * XX-> xft_font_cos + 0.5);
			cy = oy - (int)(shift * XX-> xft_font_sin + 0.5);
		}
		if ( XX-> flags. layered && EMULATE_ALPHA_CLEARING)
			XftDrawGlyph_layered( XX, color, fc.xft_font, x + dx, y - dy, ft_index);
		else
			XftDrawGlyphs( XX-> xft_drawable, color, fc.xft_font, x + dx, y - dy, &ft_index, 1);
		x = cx;
		y = cy;
	}

	if ( XX-> flags. layered && EMULATE_ALPHA_CLEARING)
		XChangeGC( DISP, XX-> gc, GCFunction|GCBackground|GCForeground|GCPlaneMask, &old_gcv);
}

static void
my_XftDrawString32( PDrawableSysData selfxx,
		_Xconst XftColor *color, int x, int y,
		_Xconst FcChar32 *string, int len)
{
	if ( IS_ZERO(XX-> font-> font. direction) && !XX-> flags. layered )
		XftDrawString32( XX-> xft_drawable, color, XX-> font-> xft, x, y, string, len);
	else
		xft_draw_glyphs( XX, color, x, y, string, len, NULL);
}

static int
filter_unsupported_rops( PDrawableSysData selfxx, int rop, XftColor * xftcolor )
{
	/* filter out unsupported rops */
	switch ( rop) {
	case ropBlackness:
		xftcolor->color.red   =
		xftcolor->color.green =
		xftcolor->color.blue  =
		xftcolor->pixel       = 0;
		rop = ropCopyPut;
		break;
	case ropWhiteness:
		xftcolor->color.red   =
		xftcolor->color.green =
		xftcolor->color.blue  = 0xffff;
		xftcolor->pixel       = 0xffffffff;
		rop = ropCopyPut;
		break;
	case ropXorPut:
	case ropOrPut:
	case ropNotSrcAnd:
	case ropNotSrcXor:
	case ropNotSrcOr:
	case ropAndPut:
		xftcolor->color.red   = COLOR_R16(XX->fore.color);
		xftcolor->color.green = COLOR_G16(XX->fore.color);
		xftcolor->color.blue  = COLOR_B16(XX->fore.color);
		xftcolor->pixel       = XX-> fore. primary;
		break;
	case ropNotPut:
		xftcolor->color.red   = COLOR_R16(~XX->fore.color);
		xftcolor->color.green = COLOR_G16(~XX->fore.color);
		xftcolor->color.blue  = COLOR_B16(~XX->fore.color);
		xftcolor->pixel       = ~XX-> fore. primary;
		rop = ropCopyPut;
		break;
	default:
		xftcolor->color.red   = COLOR_R16(XX->fore.color);
		xftcolor->color.green = COLOR_G16(XX->fore.color);
		xftcolor->color.blue  = COLOR_B16(XX->fore.color);
		xftcolor->pixel       = XX-> fore. primary;
		rop = ropCopyPut;
	}

	return rop;
}

/* force-remove antialiasing, xft doesn't have a better API for this */
static XftFont *
get_no_aa_font( PDrawableSysData selfxx, XftFont * font)
{
	FcBool aa;
	if (
		( FcPatternGetBool( font-> pattern, FC_ANTIALIAS, 0, &aa) == FcResultMatch)
		&& aa
	) {
		XftFont * f = create_no_aa_font( font);
		if ( f)
			font = XX-> font-> xft_no_aa = f;
	}

	return font;
}

static void
setup_alpha(PDrawableSysData selfxx, XftColor * xftcolor, XftFont ** font)
{
	if ( XX-> flags. layered) {
		xftcolor->color.alpha = 0xffff;
	} else if ( XX-> type. bitmap) {
		xftcolor->color.alpha = (
			(
				xftcolor->color.red/3 + 
				xftcolor->color.green/3 + 
				xftcolor->color.blue/3
			) > (0xff00 / 2)
		) ?
			0xffff :
			0
			;
		if ( !guts. xft_no_antialias && !XX-> font-> xft_no_aa)
			*font = get_no_aa_font(XX, *font);
	} else {
		xftcolor->color.alpha = 0xffff;
	}
}

static void
paint_text_background( Handle self, Point * p, int x, int y )
{
	DEFXX;
	int i;
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
}

/* emulate over- and understriking */
static void
overstrike( Handle self, int x, int y, Point *ovx, int advance)
{
	DEFXX;
	int lw = apc_gp_get_line_width( self);
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

	if ( ovx->x < 0 ) ovx->x = 0;
	if ( ovx->y < 0 ) ovx->y = 0;
	if ( PDrawable( self)-> font. style & fsUnderlined) {
		ay = d;
		x1 = x - ovx->x * c - ay * s + 0.5;
		y1 = y - ovx->x * s + ay * c + 0.5;
		advance += ovx->y;
		x2 = x + advance * c - ay * s + 0.5;
		y2 = y + advance * s + ay * c + 0.5;
		XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
	}

	if ( PDrawable( self)-> font. style & fsStruckOut) {
		ay = (XX-> font-> font.ascent - XX-> font-> font.internalLeading)/2;
		x1 = x - ovx->x * c - ay * s + 0.5;
		y1 = y - ovx->x * s + ay * c + 0.5;
		advance += ovx->y;
		x2 = x + advance * c - ay * s + 0.5;
		y2 = y + advance * s + ay * c + 0.5;
		XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
	}

	if ( lw != 1)
		apc_gp_set_line_width( self, lw);
}

static void
allocate_xftdraw_surface(PDrawableSysData selfxx)
{
	if ( !XX-> xft_drawable) {
		if ( XX-> type. bitmap)
			XX-> xft_drawable = XftDrawCreateBitmap( DISP, XX-> gdrawable );
		else
			XX-> xft_drawable = XftDrawCreate( DISP,
				XX-> gdrawable, XX->visual->visual, XX->colormap
			);
		XftDrawSetSubwindowMode( XX-> xft_drawable,
			XX-> flags.clip_by_children ? ClipByChildren : IncludeInferiors);
		XCHECKPOINT;
	}
	if ( !XX-> flags. xft_clip) {
		XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
		XX-> flags. xft_clip = 1;
	}
}

typedef struct {
	int x, y, dx, dy, width, height;
	Pixmap canvas;
	GC gc;
} TextBlit;

static Bool
open_text_blit(Handle self, int x, int y, int dx, int rop, TextBlit * tb)
{
	DEFXX;
	XGCValues gcv;
	int i;
	Rect rc;
	Point p[4], offset;

	bzero( &rc, sizeof(rc));
	tb-> x   = x;
	tb-> y   = y;
	tb-> dx  = dx;
	tb-> dy  = XX-> font-> font. height;

	offset. x = offset. y = 0;
	p[0].x = p[2].x = 0;
	p[0].y = p[1].y = 0;
	p[1].x = p[3].x = tb->dx;
	p[2].y = p[3].y = tb->dy;
	rc. left = rc. right = rc. top = rc. bottom = 0;
	for ( i = 1; i < 4; i++) {
		int x = p[i].x * XX-> xft_font_cos - p[i].y * XX-> xft_font_sin + 0.5;
		int y = p[i].x * XX-> xft_font_sin + p[i].y * XX-> xft_font_cos + 0.5;
		if ( rc. left    > x) rc. left   = x;
		if ( rc. right   < x) rc. right  = x;
		if ( rc. bottom  > y) rc. bottom = y;
		if ( rc. top     < y) rc. top    = y;
	}
	tb->width  = rc. right  - rc. left   + 1;
	tb->height = rc. top    - rc. bottom + 1;

	tb->canvas = XCreatePixmap( DISP, guts. root,
		tb->width, tb->height,
		XX-> type. bitmap ? 1 : guts. depth);
	if ( !tb->canvas)
		return false;

	tb->dx = -rc. left;
	tb->dy = -rc. bottom;
	tb->gc = XCreateGC( DISP, tb->canvas, 0, &gcv);
	switch ( rop) {
	case ropAndPut:
	case ropNotSrcXor:
	case ropNotSrcOr:
		XSetForeground( DISP, tb->gc, 0xffffffff);
		break;
	default:
		XSetForeground( DISP, tb->gc, 0x0);
		break;
	}
	XFillRectangle( DISP, tb->canvas, tb->gc, 0, 0, tb->width, tb->height);
	XftDrawChange( XX-> xft_drawable, tb->canvas);
	if ( XX-> flags. xft_clip)
		XftDrawSetClip( XX-> xft_drawable, 0);

	return true;
}

static void
close_text_blit(PDrawableSysData selfxx, TextBlit * tb)
{
	XftDrawChange( XX-> xft_drawable, XX-> gdrawable);
	if ( XX-> flags. xft_clip)
		XftDrawSetClip( XX-> xft_drawable, XX-> current_region);
	XCHECKPOINT;
	XCopyArea( DISP,
		tb->canvas, XX-> gdrawable, XX-> gc,
		0, 0, tb->width, tb->height,
		tb->x - tb->dx, REVERT( tb->y - tb->dy + tb->height)
	);
	XFreeGC( DISP, tb->gc);
	XFreePixmap( DISP, tb->canvas);
}

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

Bool
prima_xft_text_out( Handle self, const char * text, int x, int y, int len, int flags)
{
	DEFXX;
	FcChar32 *ucs4;
	XftColor xftcolor;
	XftFont *font = XX-> font-> xft;
	int rop = XX-> paint_rop;
	Point baseline;

	if ( len == 0) return true;
	len = check_width( XX->font, len );
	rop = filter_unsupported_rops( XX, rop, &xftcolor );
	setup_alpha( XX, &xftcolor, &font );

	/* paint background if opaque */
	if ( XX-> flags. paint_opaque) {
		Point * p = prima_xft_get_text_box( self, text, len, flags);
		paint_text_background( self, p, x, y );
		free( p);
	}

	SHIFT(x,y);
	RANGE2(x,y);
	/* xft doesn't allow shifting glyph reference point - need to adjust manually */
	baseline.x = - PDrawable(self)-> font. descent * XX-> xft_font_sin;
	baseline.y = - PDrawable(self)-> font. descent * ( 1 - XX-> xft_font_cos )
					+ XX-> font-> font. descent;
	if ( !XX-> flags. paint_base_line) {
		x += baseline.x;
		y += baseline.y;
	}

	/* convert text string to unicode */
	if ( !( ucs4 = xft_text2ucs4(( unsigned char*) text, len, flags & toUTF8, X( self)-> xft_map8)))
		return false;

	allocate_xftdraw_surface(XX);
	if ( rop != ropCopyPut) {
		/* emulate rops by blitting the text */
		int dx;
		TextBlit tb;
		dx = prima_xft_get_text_width( XX-> font, text, len,
			flags | toAddOverhangs, X(self)-> xft_map8, NULL);
		if (!open_text_blit(self, x, y, dx, rop, &tb))
			goto COPY_PUT;
		my_XftDrawString32( XX, &xftcolor,
			tb.dx + baseline.x, tb.height - tb.dy - baseline.y,
			ucs4, len);
		close_text_blit(XX, &tb);
		x -= baseline.x;
		y -= baseline.y;
	} else {
	COPY_PUT:
		my_XftDrawString32( XX, &xftcolor, x, REVERT( y) + 1, ucs4, len);
	}
	free( ucs4);
	XCHECKPOINT; 

	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {
		Point ovx;
		overstrike(self, x, y, &ovx, prima_xft_get_text_width(
			XX-> font, text, len, flags | toAddOverhangs,
			X(self)-> xft_map8, &ovx) - 1
		);
	}
	XFLUSH;

	return true;
}

Bool
prima_xft_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y)
{
	DEFXX;
	XftColor xftcolor;
	XftFont *font = XX-> font-> xft;
	int rop = XX-> paint_rop;
	Point baseline;
		
	t-> flags |= toAddOverhangs; /* for overstriking etc */

	if ( t->len == 0) return true;
	t->len = check_width( XX->font, t->len );
	rop = filter_unsupported_rops( XX, rop, &xftcolor );
	setup_alpha( XX, &xftcolor, &font );

	/* paint background if opaque */
	if ( XX-> flags. paint_opaque) {
		Point * p = prima_xft_get_glyphs_box( self, t);
		paint_text_background( self, p, x, y );
		free( p);
	}

	SHIFT(x,y);
	RANGE2(x,y);
	/* xft doesn't allow shifting glyph reference point - need to adjust manually */
	baseline.x = - PDrawable(self)-> font. descent * XX-> xft_font_sin;
	baseline.y = - PDrawable(self)-> font. descent * ( 1 - XX-> xft_font_cos )
					+ XX-> font-> font. descent;
	if ( !XX-> flags. paint_base_line) {
		x += baseline.x;
		y += baseline.y;
	}

	allocate_xftdraw_surface(XX);
	if ( rop != ropCopyPut) {
		/* emulate rops by blitting the text */
		int dx;
		TextBlit tb;
		dx = prima_xft_get_glyphs_width( XX-> font, t, NULL);
		if (!open_text_blit(self, x, y, dx, rop, &tb))
			goto COPY_PUT;
		xft_draw_glyphs(XX, &xftcolor,
			tb.dx + baseline.x, tb.height - tb.dy - baseline.y,
			NULL, 0, t);
		close_text_blit(XX, &tb);
		x -= baseline.x;
		y -= baseline.y;
	} else {
	COPY_PUT:
		xft_draw_glyphs(XX, &xftcolor, x, REVERT(y) + 1, NULL, 0, t);
	}
	XCHECKPOINT; 

	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {
		Point ovx;
		t-> flags |= toAddOverhangs;
		overstrike(self, x, y, &ovx, prima_xft_get_glyphs_width(
			XX-> font, t, &ovx) - 1);
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

static unsigned long *
get_font_ranges( FcCharSet *c, int * count)
{
	FcChar32 ucs4, last = 0, haslast = 0;
	FcChar32 map[FC_CHARSET_MAP_SIZE];
	FcChar32 next;
	int size = 16;
	unsigned long * ret;

#define ADD(ch,flag) if(!xft_add_item(&ret,count,&size,ch,flag)) return ret

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

unsigned long *
prima_xft_get_font_ranges( Handle self, int * count)
{
	return get_font_ranges(X(self)-> font-> xft-> charset, count);
}

char *
prima_xft_get_font_languages( Handle self)
{
#if FC_VERSION >= 21100
	FcPattern *pat;
	FcLangSet *ls;
	FcStrSet  *ss;
	FcStrList *l;
	FcChar8  *s;
	char *ret, *p;
	int size;

	if ( !(pat = X(self)-> font-> xft-> pattern))
		return NULL;
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

PFontABC
prima_xft_get_font_abc( Handle self, int firstChar, int lastChar, int flags)
{
	PFontABC abc;
	int i, len = lastChar - firstChar + 1;
	XftFont *font = X(self)-> font-> xft_base;

	if ( !( abc = malloc( sizeof( FontABC) * len)))
		return NULL;

	for ( i = 0; i < len; i++) {
		FT_UInt ft_index;
		XGlyphInfo glyph;
		if ( flags & toGlyphs ) {
			ft_index = i + firstChar;
		} else {
			FcChar32 c = i + firstChar;
			if ( !(flags & toUnicode) && c > 128)
				c = X(self)-> xft_map8[ c - 128];
			ft_index = XftCharIndex( DISP, font, c);
		}
		XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
		abc[i]. a = -glyph. x;
		abc[i]. b = glyph. width;
		abc[i]. c = glyph. xOff - glyph. width + glyph. x;
	}

	return abc;
}

PFontABC
prima_xft_get_font_def( Handle self, int firstChar, int lastChar, int flags)
{
	PFontABC abc;
	int i, len = lastChar - firstChar + 1;
	XftFont *font = X(self)-> font-> xft_base;

	if ( !( abc = malloc( sizeof( FontABC) * len)))
		return NULL;

	for ( i = 0; i < len; i++) {
		FT_UInt ft_index;
		XGlyphInfo glyph;
		if ( flags & toGlyphs ) {
			ft_index = i + firstChar;
		} else {
			FcChar32 c = i + firstChar;
			if ( !(flags & toUnicode) && c > 128)
				c = X(self)-> xft_map8[ c - 128];
			ft_index = XftCharIndex( DISP, font, c);
		}
		XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
		abc[i]. a = X(self)-> font-> font. descent - glyph. height + glyph. y; /* XXX yOff ? */
		abc[i]. b = glyph. height;
		abc[i]. c = X(self)-> font-> font. ascent - glyph. y;
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
	FcCharSet * c = NULL;
	Font f, def = guts. default_font;

	bzero( &f, sizeof( Font));
	f. undef. height = f. undef. width = f. undef. size = f. undef. vector = f. undef. pitch = 1;
	fcpattern2font( p, &f);
	f. undef. width = 1;
	FcPatternGetCharSet( p, FC_CHARSET, 0, &c);
	if ( c && (FcCharSetCount(c) > 0)) {
		int i;
		for ( i = 0; i < STD_CHARSETS; i++) {
			if ( !std_charsets[i]. enabled) continue;
			if ( FcCharSetIntersectCount( std_charsets[i]. fcs, c) >= std_charsets[i]. glyphs - 1) {
				strcpy( f. encoding, std_charsets[i]. name);
				break;
			}
		}
	}
	FcPatternDestroy( p);
	if ( !prima_xft_font_pick( nilHandle, &f, &def, NULL, NULL)) return false;
	*font = def;
	XFTdebug( "parsed ok: %d.%s", def.size, def.name);
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

int
prima_xft_load_font( char* filename)
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

	return count;
}

void
prima_xft_gp_destroy( Handle self )
{
	DEFXX;
	if ( XX-> xft_drawable) {
		XftDrawDestroy( XX-> xft_drawable);
		XX-> xft_drawable = NULL;
	}
	if ( XX-> xft_shadow_drawable) {
		XftDrawDestroy( XX-> xft_shadow_drawable);
		XX-> xft_shadow_drawable = NULL;
	}
	if ( XX-> xft_shadow_pixmap) {
		XFreePixmap( DISP, XX-> xft_shadow_pixmap);
		XX-> xft_shadow_pixmap = 0;
	}
	if ( XX-> xft_shadow_gc) {
		XFreeGC( DISP, XX-> xft_shadow_gc);
		XX-> xft_shadow_gc = 0;
	}
}

typedef struct {
	int count, size, last_ptr;
	int *buffer;
} OutlineStorage;

#define STORE_POINT(p) if(p) {\
	storage->buffer[ storage->last_ptr + 1 ]++;\
	storage->buffer[ storage->count++ ] = p->x;\
	storage->buffer[ storage->count++ ] = p->y;\
}

static int
store_command( OutlineStorage * storage, int cmd, const FT_Vector * p1, const FT_Vector * p2, const FT_Vector * p3)
{
	if ( storage-> size == 0 ) {
		storage-> size = 256;
		if ( !( storage-> buffer = malloc(sizeof(int) * storage->size)))
			return 1;

	} else if ( storage-> count + 7 >= storage->size ) {
		int * r;
		storage-> size *= 2;
		if (( r = realloc( storage->buffer, sizeof(int) * storage->size)) == NULL ) {
			warn("Not enough memory");
			free( storage-> buffer );
			storage-> buffer = NULL;
			storage-> count = 0;
			return 1;
		}
		storage-> buffer = r;
	}

	if ( 
		storage-> last_ptr < 0 || storage->buffer[storage->last_ptr] != cmd || 
		cmd != ggoLine
	) {
		storage->last_ptr = storage->count;
		storage->buffer[ storage->count++ ] = cmd;
		storage->buffer[ storage->count++ ] = 0;
	}

	STORE_POINT(p1)
	STORE_POINT(p2)
	STORE_POINT(p3)

	return 0;
}

static int
ftoutline_line(const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoLine, to, NULL, NULL);
}

static int
ftoutline_move(const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoMove, to, NULL, NULL);
}

static int
ftoutline_conic(const FT_Vector* control, const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoConic, control, to, NULL);
}

static int
ftoutline_cubic(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoCubic, control1, control2, to);
}

int
prima_xft_get_glyph_outline( Handle self, int index, int flags, int ** buffer)
{
	DEFXX;
	FcChar32 c;
	FT_Face face;
	FT_UInt ft_index;
	FT_Int32 ft_flags = FT_LOAD_NO_BITMAP |
		(( flags & ggoUseHints ) ? 0 : FT_LOAD_NO_HINTING);
	FT_Outline_Funcs funcs = {
		ftoutline_move,
		ftoutline_line,
		ftoutline_conic,
		ftoutline_cubic,
		0, 0
	};
	OutlineStorage storage = { 0, 0, -1, NULL };

	if ( !( face = XftLockFace( XX->font->xft)))
		return -1;

	c = ((flags & (ggoUnicode|ggoGlyphIndex)) || index <= 128) ? index : XX-> xft_map8[index - 128];
	ft_index = (flags & ggoGlyphIndex) ? c : XftCharIndex( DISP, XX->font->xft, c);
	if (
		(FT_Load_Glyph (face, ft_index, ft_flags) == 0) &&
		(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	)
  		FT_Outline_Decompose( &face->glyph->outline, &funcs, (void*)&storage);

	XftUnlockFace(XX->font->xft);

	*buffer = storage.buffer;
	return storage.count;
}

unsigned long *
prima_xft_mapper_query_ranges(PFont font, int * count)
{
	char name[256];
	XftFont * xft = NULL;
	strncpy(name, font->name, 256);
	prima_xft_font_pick( nilHandle, font, font, NULL, &xft);
	if ( !xft || strcmp( font->name, name ) != 0 ) {
		*count = 0;
		return NULL;
	}
	return get_font_ranges( xft->charset, count);
}

Bool
prima_xft_text_shaper( Handle self, PTextShapeRec r, uint32_t * map8)
{
        int i;
	uint16_t *glyphs;
	uint32_t *text;
	XftFont *font = X(self)->font->xft;

        for ( 
		i = 0, glyphs = r->glyphs, text = r->text;
		i < r->len; 
		i++
	) {
		uint32_t c = *(text++);
		if ( map8 && c > 128 ) c = map8[c];
               	*(glyphs++) = XftCharIndex(DISP, font, c);
	}
	r-> n_glyphs = r->len;

	if ( r-> advances ) {
		uint16_t *advances;
		for (
			i = 0, glyphs = r->glyphs, advances = r->advances;
			i < r-> len;
			i++, glyphs++, advances++
		) {
			XGlyphInfo glyph;
			FT_UInt ft_index = *glyphs;
			XftGlyphExtents( DISP, font, &ft_index, 1, &glyph);
			*advances = glyph.xOff;
		}
		bzero(r->positions, r->len * 2 * sizeof(int16_t));
	}

        return true;
}

Bool
prima_xft_text_shaper_bytes( Handle self, PTextShapeRec r)
{
	return prima_xft_text_shaper( self, r, X(self)-> xft_map8);
}

Bool
prima_xft_text_shaper_ident( Handle self, PTextShapeRec r)
{
	return prima_xft_text_shaper( self, r, NULL);
}

#ifdef WITH_HARFBUZZ
Bool
prima_xft_text_shaper_harfbuzz( Handle self, PTextShapeRec r)
{
	DEFXX;
	Bool ret = true;
	int i, j;
	FT_Face face;
	hb_buffer_t *buf;
	hb_font_t *font;
	hb_glyph_info_t *glyph_info;
	hb_glyph_position_t *glyph_pos;

	if ( !( face = XftLockFace( XX->font->xft))) /*XXX*/
		return -1;

	buf = hb_buffer_create();
	hb_buffer_add_utf32(buf, r->text, r->len, 0, -1);

#if HB_VERSION_ATLEAST(1,0,3)
	hb_buffer_set_cluster_level(buf, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
#endif
	hb_buffer_set_direction(buf, (r->flags & toRTL) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
/*
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_script (buf, hb_script_from_string ("Arab", -1));
*/
	if ( r-> language != NULL )
		hb_buffer_set_language(buf, hb_language_from_string(r->language, -1));
	hb_buffer_guess_segment_properties (buf);


	font = hb_ft_font_create(face, NULL);

	hb_shape(font, buf, NULL, 0);

	glyph_info = hb_buffer_get_glyph_infos(buf, &r->n_glyphs);
	glyph_pos  = hb_buffer_get_glyph_positions(buf, &r->n_glyphs);

	for (i = j = 0; i < r->n_glyphs; i++) {
		uint32_t c = glyph_info[i].cluster;
		if ( c > r-> len ) {
			/* something bad happened? */
			warn("harfbuzz shaping asssertion failed: got cluster=%d for strlen=%d", c, r->len);
			guts. use_harfbuzz = false;
			ret = false;
			break;
		}
                r->indexes[i] = c;
                r->glyphs[i]   = glyph_info[i].codepoint;
		if ( glyph_pos ) {
			r->advances[i]    = floor(glyph_pos[i].x_advance / 64.0 + .5);
			r->positions[j++] = floor(glyph_pos[i].x_offset  / 64.0 + .5);
			r->positions[j++] = floor(glyph_pos[i].y_offset  / 64.0 + .5);
		}
	}
 
	hb_buffer_destroy(buf);
	hb_font_destroy(font);
	XftUnlockFace(XX->font->xft);

	return ret;
}
#endif

void
prima_xft_init_font_substitution(void)
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
			if (( f = prima_font_mapper_save_font(guts.default_font.name)) != NULL ) {
				f->is_utf8 = guts.default_font.is_utf8;
				f->undef.name = 0;
				strncpy(f->family, guts.default_font.family,256);
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
	os = FcObjectSetBuild( FC_FAMILY, FC_FOUNDRY, FC_SCALABLE, FC_SPACING, (void*) 0);
	s = FcFontList( 0, pat, os);
	FcObjectSetDestroy( os);
	FcPatternDestroy( pat);

	if ( !s) return;

	ppat = s-> fonts;
	for ( i = 0; i < s->nfont; i++, ppat++) {
		PFont f;
		int j;
		FcChar8 * s;

		if ( FcPatternGetString(*ppat, FC_FAMILY, 0, &s) != FcResultMatch)
			continue;
		/* XXX has a rather short GPOS table. Far from an ideal solution (i.e. hack)
		but Courier is found on many systems */
		if ( strcmp((char*) s, "Courier New") == 0)
			continue;
		if ( !( f = prima_font_mapper_save_font((const char*) s)))
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

#else
#error Required: Xft version 2.1.0 or higher; fontconfig version 2.0.1 or higher. To compile without Xft, re-run 'perl Makefile.PL WITH_XFT=0'
#endif /* USE_XFT */
