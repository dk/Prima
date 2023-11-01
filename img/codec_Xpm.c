#include "img.h"
#define Drawable        XDrawable
#define Font            XFont
#define Window          XWindow
#undef FUNC
#ifdef __MINGW32__
#	undef Bool
#	define Bool BOOL
#endif
#include <X11/xpm.h>
#undef Font
#undef Drawable
#ifdef __MINGW32__
#	undef Bool
#endif
#undef Window
#define ComplexShape 0
#define XBool int
#undef Complex
#undef FUNC
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

static char * xpmext[] = { "xpm",
#if PRIMA_PLATFORM == apcUnix
	"xpm.gz", "xpm.Z", /* libXpm claims that it can run gzip */
#endif
	NULL };

static int    xpmbpp[] = {
	imbpp8,
	imbpp24,
	imbpp4,
	imbpp1,
	0 };

static char * loadOutput[] = {
	"hotSpotX",
	"hotSpotY",
	"hintsComment",
	"colorsComment",
	"pixelsComment",
	"transparentColors",
	"extensions",
	NULL
};

static char * mime[] = {
	"image/xpm",
	"image/x-xpm",
	"image/x-pixmap",
	NULL
};

static ImgCodecInfo codec_info = {
	"X Pixmap",
	"GROUPE BULL",
	XpmFormat, XpmVersion,    /* version */
	xpmext,    /* extension */
	"X Pixmap File",     /* file type */
	"XPM", /* short type */
	NULL,    /* features  */
	"",     /* module */
	"",     /* package */
	IMG_LOAD_FROM_FILE | IMG_SAVE_TO_FILE,
	xpmbpp, /* save types */
	loadOutput,
	mime
};

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	return (void*)1;
}

typedef struct _LoadRec {
	XpmImage image;
	XpmInfo  info;
	RGBColor *palette;
	Byte     *trans_table;
	Byte     xpalette;
} LoadRec;

#define outcm(dd) snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", (int)(dd))
#define outc(x)   strlcpy( fi-> errbuf, x, 256)

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;

	XpmImage image;
	XpmInfo info;

	info. valuemask = XpmComments | XpmColorTable | XpmExtensions;

	switch ( XpmReadFileToXpmImage( fi-> io.fileName, &image, &info)) {
	case XpmSuccess:
		break;
	case XpmNoMemory:
		fi-> stop = true;
		return NULL;
	case XpmFileInvalid:
	case XpmOpenFailed:
	default:
		return NULL;
	}

	fi-> frameCount = 1;
	fi-> stop = true;

	l = malloc( sizeof( LoadRec) + ( sizeof( RGBColor) + 1) * image. ncolors);
	if ( !l) {
		XpmFreeXpmImage( &image);
		XpmFreeXpmInfo( &info);
		outcm( sizeof( LoadRec) + ( sizeof( RGBColor) + 1) * image. ncolors);
		return NULL;
	}

	memset( l, 0, sizeof( LoadRec) + ( sizeof( RGBColor) + 1) * image. ncolors);
	l-> image       = image;
	l-> info        = info;
	l-> palette     = ( RGBColor*) ( &l-> xpalette);
	l-> trans_table = ( Byte*)( l->palette) + sizeof( RGBColor) * image. ncolors;

	return l;
}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	HV * profile = fi-> frameProperties;
	PIcon i = ( PIcon) fi-> object;
	int type, transparent_count = 0;

	/* determine type */
	if ( l-> image. ncolors <= 2)   type = imbpp1; else
	if ( l-> image. ncolors <= 16)  type = imbpp4; else
	if ( l-> image. ncolors <= 256) type = imbpp8; else type = imbpp24;

	/* acquire color mapping */
	{
		int i, j;
		PHash c = hash_create();
		XpmColor * x = l-> image. colorTable;
		unsigned int offsets[5];
		offsets[0] = (Handle)(&x-> c_color)  - (Handle)x;
		offsets[1] = (Handle)(&x-> g_color)  - (Handle)x;
		offsets[2] = (Handle)(&x-> g4_color) - (Handle)x;
		offsets[3] = (Handle)(&x-> m_color)  - (Handle)x;
		offsets[4] = (Handle)(&x-> symbolic) - (Handle)x;
		for ( i = 0; i < l-> image. ncolors; i++, x++) {
			for ( j = 0; j < 5; j++) {
				char * s = *((char**)((char *)x + offsets[j]));
				if ( s) {
					Color xc;
					if ( s[0] == '#' && strlen(s) < 14 && s[1] != 0) {
						char buf[ 14], *e;
						int ln = strlen(s)-1, r, g, b;
						strcpy( buf, s + 1);
						while ( ln % 3) {
							strcat( buf, "0");
							ln++;
						}
						ln /= 3;
						b = strtol( buf + ln * 2, &e, 16); buf[ ln * 2] = 0;
						if ( *e) break;
						g = strtol( buf + ln, &e, 16); buf[ ln] = 0;
						if ( *e) break;
						r = strtol( buf, &e, 16);
						if ( *e) break;
						switch ( ln) {
						case 1:
							b *= 16; g *= 16; r *= 16;
							break;
						case 3:
							b /= 16; g /= 16; r /= 16;
							break;
						case 4:
							b /= 256; g /= 256; r /= 256;
							break;
						}
						xc = ARGB( r, g, b);
					} else {
						xc = (Color) PTR2UV(hash_fetch( c, s, strlen(s)));
						if ( xc) {
							xc -= 10;
						} else if ( stricmp( s, "none") == 0)  {
							transparent_count++;
							l-> trans_table[ i] = 1;
							xc = 0;
						} else {
							xc = apc_lookup_color( s);
							hash_store( c, s, strlen(s), INT2PTR(void*, (Color)xc + 10));
							if ( xc == clInvalid) xc = 0;
						}
					}
					l-> palette[ i]. b =   xc & 0xff;
					l-> palette[ i]. g = ( xc >> 8  ) & 0xff;
					l-> palette[ i]. r = ( xc >> 16 ) & 0xff;
					break;
				}
			}
		}
		hash_destroy( c, false);
	}

	/* extras */
	if ( fi-> loadExtras) {
		if ( transparent_count) {
			int i;
			AV * av = newAV();
			for ( i = 0; i < l-> image. ncolors; i++)
				if ( l-> trans_table[i])
					av_push( av, newSViv( i));
			pset_sv_noinc( transparentColors, newRV_noinc(( SV*) av));
		}

		if ( l-> info. valuemask & XpmHotspot) {
			pset_i( hotSpotX, l-> info. x_hotspot);
			pset_i( hotSpotY, l-> info. y_hotspot);
		}

		if ( l-> info. valuemask & XpmComments) {
			if ( l-> info. hints_cmt)
				pset_c( hintsComment, l-> info. hints_cmt);
			if ( l-> info. colors_cmt)
				pset_c( colorsComment, l-> info. colors_cmt);
			if ( l-> info. pixels_cmt)
				pset_c( pixelsComment, l-> info. pixels_cmt);
		}


		if ( l-> info. valuemask & XpmExtensions && l-> info. nextensions > 0) {
			int i, j;
			HV * hash = newHV();
			XpmExtension * e = l-> info. extensions;
			for ( i = 0; i < l-> info. nextensions; i++, e++) {
				SV * string = newSVpv("", 0);
				for ( j = 0; j < e-> nlines; j++) {
					sv_catpv( string, e-> lines[j]);
					if ( j + 1 < e-> nlines)
						sv_catpv( string, "\n");
				}
				(void) hv_store( hash, e-> name, strlen( e-> name), string, 0);
			}
			pset_sv_noinc( extensions, newRV_noinc((SV*)hash));
		}
	}


	if ( fi-> noImageData) {
		CImage( fi-> object)-> set_type( fi-> object, type);
		pset_i( width,      l-> image. width);
		pset_i( height,     l-> image. height);
		if ( type < imbpp24) {
			memcpy( i-> palette, l-> palette, l-> image. ncolors * sizeof( RGBColor));
			i-> palSize = l-> image. ncolors;
		}
		return true;
	}

	CImage( fi-> object)-> create_empty( fi-> object, l-> image. width, l-> image. height, type);
	if ( type < imbpp24) {
		memcpy( i-> palette, l-> palette, l-> image. ncolors * sizeof( RGBColor));
		i-> palSize = l-> image. ncolors;
	}

	/* read data */
	if ( i-> h > 0 && i-> w > 0) {
		unsigned int x, y, *data = l-> image.data;
		Byte * dst = i-> data + ( i-> h - 1 ) * i-> lineSize;
		for ( y = 0; y < i-> h; y++) {
			Byte * d = dst;
			for ( x = 0; x < i-> w; x++, data++) {
				switch ( type) {
				case imbpp1:
					if ( *data) *d |= 0x80 >> ( x % 8 );
					if (( x % 8) == 7) d++;
					break;
				case imbpp4:
					if ( x % 2)
						*(d++) |= (*data) & 0xf;
					else
						*d = ((*data) & 0xf) << 4;
					break;
				case imbpp8:
					*(d++) = *data;
					break;
				case imbpp24:
					*(d++) = l-> palette[ *data]. b;
					*(d++) = l-> palette[ *data]. g;
					*(d++) = l-> palette[ *data]. r;
					break;
				}
			}
			dst -= i-> lineSize;
		}
	}

	/* read transparency info */
	if ( kind_of( fi-> object, CIcon) && transparent_count) {
		unsigned int x, y, *data = l-> image.data;
		Byte * dst = i-> mask + ( i-> h - 1 ) * i-> maskLine;
		for ( y = 0; y < i-> h; y++) {
			Byte * d = dst;
			for ( x = 0; x < i-> w; x++, data++) {
				if ( l-> trans_table[*data]) *d |= 0x80 >> ( x % 8 );
				if (( x % 8) == 7) d++;
			}
			dst -= i-> maskLine;
		}
		i-> autoMasking = amNone;
	}

	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	XpmFreeXpmImage( &l-> image);
	XpmFreeXpmInfo( &l-> info);
	free( fi-> instance);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( hotSpotX, 0);
	pset_i( hotSpotY, 0);
	pset_c( hintsComment, "");
	pset_c( colorsComment, "");
	pset_c( pixelsComment, "");
	pset_sv_noinc( extensions, newRV_noinc((SV*)newHV()));
	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return (void*)1;
}

typedef struct {
	int delta;
	XpmImage * image;
} CalcData;

static char * encoder = "qwertyuiop[]';lkjhgfdsazxcvbnm,./`1234567890-=QWERTYUIOP{}ASDFGH";
static char * hex = "0123456789ABCDEF";

static Bool
prepare_xpm_color( void * value, int keyLen, Color * color_ptr, CalcData * cd)
{
	Color color = *color_ptr;
	IV v = PTR2IV(value) - 1, cpp = cd-> image-> cpp, lpp = 6;
	char * c;

	c = cd-> image-> colorTable[ v]. c_color =
		(char*) cd-> image-> colorTable + cd-> delta;

	if ( color != clInvalid) {
		/* optimized snprintf( c, 8, "#%06x", (unsigned int)color); */
		c += 7;
		*(c--) = 0;
		while ( lpp--) {
			*(c--) = hex[ color % 16];
			color /= 16;
		}
		*c = '#';
	} else
		strcpy( c, "None");
	cd-> delta += 8;

	c = cd-> image-> colorTable[ v]. string =
		(char*) cd-> image-> colorTable + cd-> delta;
	if ( color != clInvalid) {
		while ( cpp--) {
			*(c++) = encoder[ v % 64];
			v /= 64;
		}
	} else {
		while ( cpp--) *(c++) = ' ';
	}
	*c = 0;
	cd-> delta += 5;

	return false;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	XpmInfo  info;
	XpmImage image;
	HV * profile = fi-> objectExtras;
	PIcon i = ( PIcon) fi-> object;
	int ret = XpmOpenFailed;
	char * extholder = NULL;
	Bool icon = kind_of( fi-> object, CIcon);

	memset( &info, 0, sizeof(info));
	memset( &image, 0, sizeof(image));

	info. valuemask = XpmSize;
	image. width     = i-> w;
	image. height    = i-> h;
	image. ncolors   = (( i-> type & imBPP) > imbpp8) ? 16777216 : i-> palSize;
	if ( !( image. data = malloc( i-> w * i-> h * sizeof(int)))) {
		outcm( i-> w * i-> h * sizeof(int));
		return false;
	}
	memset( image. data, 0, i-> w * i-> h * sizeof(int));

	/* prepare pixels and palette */
	if ( image. ncolors > 256) { /* reduce colors altogether */
		CalcData cd;
		PHash hash = hash_create();
		int x, y, transcolor = -1;
		unsigned int *dest = image. data;
		Byte * p = i-> data + ( i-> h - 1) * i-> lineSize;
		Byte * mask = icon ? i-> mask + ( i-> h - 1) * i-> maskLine : NULL;

		for ( y = 0; y < i-> h; y++) {
			RGBColor * pp = ( RGBColor *) p;
			for ( x = 0; x < i-> w; x++, pp++) {
				if ( !icon || !( mask[ x >> 3] & ( 0x80 >> ( x & 7)))) {
					Color key = ARGB(pp->r,pp->g,pp->b);
					Handle val = (Handle) hash_fetch( hash, &key, sizeof(key));
					if ( val == 0)
						hash_store( hash, &key, sizeof(key), (void*)(val = (hash_count( hash) + 1)));
					*(dest++) = val - 1;
				} else {
					if ( transcolor < 0) {
						Color key = clInvalid;
						transcolor = hash_count( hash);
						hash_store( hash, &key, sizeof(key), INT2PTR(void*, transcolor + 1));
					}
					*(dest++) = transcolor;
				}
			}
			p -= i-> lineSize;
			if ( icon) mask -= i-> maskLine;
		}
		image. ncolors = hash_count( hash);
		image. cpp     = ( image. ncolors <= 4096) ?
			(( image. ncolors <= 64)     ? 1 : 2) :
			(( image. ncolors <= 262144) ? 3 : 4);
		if ( !( image. colorTable = malloc( image. ncolors * ( sizeof( XpmColor) + 13)))) {
			free( image. data);
			hash_destroy( hash, false);
			outcm( image. ncolors * ( sizeof( XpmColor) + 13));
			return false;
		}
		memset( image. colorTable, 0, image. ncolors * ( sizeof( XpmColor) + 13));
		cd. delta = image. ncolors * sizeof( XpmColor);
		cd. image = &image;
		hash_first_that( hash, (void*)prepare_xpm_color, &cd, NULL, NULL);
		hash_destroy( hash, false);
	} else {
		CalcData cd;
		int x, y, type = i-> type & imBPP, val = 0, transcolor = -1, ncolors = image. ncolors;
		unsigned int *dest = image. data;
		Byte * p = i-> data + ( i-> h - 1) * i-> lineSize;
		Byte * mask = icon ? i-> mask + ( i-> h - 1) * i-> maskLine : NULL;

		if ( mask) transcolor = image. ncolors++;

		for ( y = 0; y < i-> h; y++) {
			Byte * s = p;
			for ( x = 0; x < i-> w; x++) {
				switch ( type) {
				case imbpp1:
					val = ( *s & ( 0x80 >> ( x % 8))) ? 1 : 0;
					if ( x % 8 == 7) s++;
					break;
				case imbpp4:
					val = (( x % 2) ? *(s++) : (*s >> 4)) & 0xf;
					break;
				case imbpp8:
					val = *(s++);
					break;
				}
				if ( !icon || !( mask[ x >> 3] & ( 0x80 >> ( x & 7)))) {
					*(dest++) = val;
				} else {
					*(dest++) = transcolor;
				}
			}
			p -= i-> lineSize;
			if ( icon) mask -= i-> maskLine;
		}
		if ( !( image. colorTable = malloc( image. ncolors * ( sizeof( XpmColor) + 13)))) {
			free( image. data);
			outcm( image. ncolors * ( sizeof( XpmColor) + 13));
			return false;
		}
		memset( image. colorTable, 0, image. ncolors * ( sizeof( XpmColor) + 13));
		image. cpp = ( image. ncolors > 64) ? 2 : 1;
		cd. delta = image. ncolors * sizeof( XpmColor);
		cd. image = &image;
		for ( x = 0; x < ncolors; x++) {
			Color c = ARGB(i-> palette[x].r,i-> palette[x].g,i-> palette[x].b);
			prepare_xpm_color(INT2PTR(void*,x+1), 0, &c, &cd);
		}
		if ( icon) {
			Color c = clInvalid;
			prepare_xpm_color( INT2PTR(void*,transcolor+1), 0, &c, &cd);
		}
	}

	/* extras */
	if ( pexist( hintsComment)) {
		info. valuemask |= XpmComments;
		info. hints_cmt = pget_c( hintsComment);
	}
	if ( pexist( colorsComment)) {
		info. valuemask |= XpmComments;
		info. colors_cmt = pget_c( colorsComment);
	}
	if ( pexist( pixelsComment)) {
		info. valuemask |= XpmComments;
		info. pixels_cmt = pget_c( pixelsComment);
	}


	if ( pexist( hotSpotX) || pexist( hotSpotY)) {
		info. valuemask |= XpmHotspot;
		info. x_hotspot = pexist( hotSpotX) ? pget_i( hotSpotX) : 0;
		info. y_hotspot = pexist( hotSpotY) ? pget_i( hotSpotY) : 0;
	}

	if ( pexist( extensions)) {
		HV * hv;
		HE * he;
		SV * sv = pget_sv( extensions);
		char * c, **ptr;
		int i = 0, ptrs = 0, strholder = 0;

		if ( !SvOK(sv) || !SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVHV) {
			outc("'extensions' is not a hash");
			goto EXIT;
		}
		hv = ( HV*) SvRV( sv);
		info. nextensions = HvKEYS(( HV*) hv);
		if ( !( info. extensions = malloc( sizeof( XpmExtension) * info. nextensions))) {
			outcm( sizeof( XpmExtension) * info. nextensions);
			goto EXIT;
		}
		memset( info. extensions, 0, sizeof( XpmExtension) * info. nextensions);

		hv_iterinit( hv);
		for (;;) {
			char * c;
			STRLEN vlen;
			if (( he = hv_iternext( hv)) == NULL) break;
			info. extensions[i]. name  = (char*) HeKEY( he);
			c = (char*) SvPV( HeVAL( he), vlen);
			info. extensions[i]. lines = (char**)c;
			strholder += vlen + 1;
			info. extensions[i]. nlines = 1;
			while (*c) if ( *(c++) == '\n') info. extensions[i]. nlines++;
			ptrs += info. extensions[i]. nlines;
			i++;
		}

		if ( !( extholder = malloc( strholder + ptrs * sizeof( char*)))) {
			outcm( strholder + ptrs * sizeof( char*));
			goto EXIT;
		}
		memset( extholder, 0, strholder + ptrs * sizeof( char*));
		ptr = ( char**) extholder;
		c   = extholder + ptrs * sizeof( char*);
		for ( i = 0; i < info. nextensions; i++) {
			Bool crlf = false;
			char * x = ( char*) info. extensions[i]. lines;
			info. extensions[i]. lines = ptr;
			*(ptr++) = c;
			while ( *x) {
				switch ( *x) {
				case '\n':
					crlf = true;
					*(c++) = 0;
					*(ptr++) = c;
					break;
				case '\r':
					if ( crlf) {
						crlf = false;
						break;
					}
				default:
					crlf = false;
					*(c++) = *x;
				}
				x++;
			}
			*(c++) = 0;
		}
		info. valuemask |= XpmExtensions;
	}

	ret = XpmWriteFileFromXpmImage( fi-> io.fileName, &image, &info);

EXIT:
	free( extholder);
	free( info. extensions);
	free( image. colorTable);
	free( image. data);

	return ret == XpmSuccess;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
}

void
apc_img_codec_Xpm( void )
{
	struct ImgCodecVMT vmt;
	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
	vmt. init          = init;
	vmt. open_load     = open_load;
	vmt. load          = load;
	vmt. close_load    = close_load;
	vmt. save_defaults = save_defaults;
	vmt. open_save     = open_save;
	vmt. save          = save;
	vmt. close_save    = close_save;
	apc_img_register( &vmt, NULL);
}

#ifdef __cplusplus
}
#endif
