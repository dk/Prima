#if defined(WIN32)
#include <setjmp.h>
/*
MINGW defines setjmp as some serious voodoo that PerlProc_setjmp cannot emulate,
and _setjmp is not equivaluent to setjmp anymore
*/
#pragma push_macro("setjmp")
#pragma push_macro("longjmp")
#endif
#define USE_NO_MINGW_SETJMP_TWO_ARGS
#include "generic/config.h"
#define Z_PREFIX
#include <png.h>
#undef Byte
#undef FAR

#ifndef PNG_GAMMA_THRESHOLD
#define PNG_GAMMA_THRESHOLD 0.05
#endif
#ifndef PNG_MAX_GAMMA
#define PNG_MAX_GAMMA 11
#endif

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

#ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
#define APNG
#endif


#include "img.h"
#include "img_conv.h"
#include "Icon.h"

#ifdef BROKEN_PERL_PLATFORM
#undef      setjmp
#undef      longjmp
#define     setjmp _setjmp
#endif
#if defined(WIN32)
#pragma pop_macro("setjmp")
#pragma pop_macro("longjmp")
#endif

#define STREQ(a,b)(strcmp((const char*)(a),b)==0)

#ifdef __cplusplus
extern "C" {
#endif

static char * pngext[] = { "png", nil };
static int    pngbpp[] = {
	imRGB,
	imbpp8 | imGrayScale, imbpp8,
	imbpp4 | imGrayScale, imbpp4,
	imbpp1 | imGrayScale, imbpp1,
	0
};
static char * features[] = {
	"PLTE", "IHDR",
#ifdef PNG_gAMA_SUPPORTED
	"gAMA",
#endif
#ifdef PNG_iCCP_SUPPORTED
	"iCCP",
#endif
#ifdef PNG_bKGD_SUPPORTED
	"bKGD",
#endif
#ifdef PNG_TEXT_SUPPORTED
	"tEXt",
#ifdef PNG_zTXt_SUPPORTED
	"zTXt",
#endif
#endif
#ifdef PNG_oFFs_SUPPORTED
	"oFFs",
#endif
#ifdef PNG_pHYs_SUPPORTED
	"pHYs",
#endif
#ifdef PNG_sCAL_SUPPORTED
	"sCAL",
#endif
#ifdef PNG_sRGB_SUPPORTED
	"sRGB",
#endif
#ifdef PNG_tRNS_SUPPORTED
	"tRNS",
#endif
#ifdef APNG
	"acTL", "fdAT", "fcTL",
#endif
	nil
};

static char * loadOutput[] = {
	"background",
	"gamma",
#ifdef PNG_iCCP_SUPPORTED
	"iccp_name",
	"iccp_profile",
#endif
	"interlaced",
#ifdef PNG_INTRAPIXEL_DIFFERENCING
	"mng_datastream",
#endif
#ifdef PNG_oFFs_SUPPORTED
	"offset_x",
	"offset_y",
	"offset_dimension",
#endif
#ifdef PNG_sRGB_SUPPORTED
	"render_intent",
#endif
#ifdef PNG_pHYs_SUPPORTED
	"resolution_x",
	"resolution_y",
	"resolution_dimension",
#endif
#ifdef PNG_sCAL_SUPPORTED
	"scale_x",
	"scale_y",
	"scale_unit",
#endif
#ifdef PNG_TEXT_SUPPORTED
	"text",
#endif
#ifdef PNG_tRNS_SUPPORTED
	"transparency_table",
	"transparent_color",
	"hasAlpha",
#endif
#ifdef APNG
	"blendMethod",
	"delayTime",
	"disposalMethod",
	"left",
	"loopCount",
	"screenWidth",
	"screenHeight",
	"top",
#endif
	nil
};

static ImgCodecInfo codec_info = {
	"PNG",
	"PNG development group, http://www.libpng.org",
#ifdef PNG_LIBPNG_VER_MAJOR
	PNG_LIBPNG_VER_MAJOR,
#else
	PNG_LIBPNG_VER / 10000,
#endif
#ifdef PNG_LIBPNG_VER_MINOR
	PNG_LIBPNG_VER_MINOR,
#else
	(PNG_LIBPNG_VER % 10000) / 100,
#endif
	/* version */
	pngext,  /* extension */
	"Portable Network Graphics",     /* file type */
	"PNG",  /* short type */
	features,    /* features  */
	"Prima::Image::png",     /* module */
	"Prima::Image::png",     /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM
#ifdef APNG
	| IMG_LOAD_MULTIFRAME
#endif
	,
	pngbpp, /* save types */
	loadOutput
};

static double default_screen_gamma = 2.2;

static void *
init( PImgCodecInfo * info, void * param)
{
	double LUT_exponent;               /* just the lookup table */
	double CRT_exponent = 2.2;         /* just the monitor */

	*info = &codec_info;

/* querying display gamma value - code from PNG specs */
#if defined(NeXT)
	LUT_exponent = 1.0 / 2.2;
	/*
	if (some_next_function_that_returns_gamma(&next_gamma))
		LUT_exponent = 1.0 / next_gamma;
	*/
#elif defined(sgi)
	LUT_exponent = 1.0 / 1.7;
	/* there doesn't seem to be any documented function to get the
		"gamma" value, so we do it the hard way - which is unreliable for remote
		runs anyway */
	{
		FILE * infile = fopen("/etc/config/system.glGammaVal", "r");
		if (infile) {
			double sgi_gamma;
			char tmpline[80];

			fgets(tmpline, 80, infile);
			fclose(infile);
			sgi_gamma = atof(tmpline);
			if (sgi_gamma > 0.0)
					LUT_exponent = 1.0 / sgi_gamma;
		}
	}
#elif defined(Macintosh)
	LUT_exponent = 1.8 / 2.61;
	/*
	if (some_mac_function_that_returns_gamma(&mac_gamma))
		LUT_exponent = mac_gamma / 2.61;
	*/
#else
	LUT_exponent = 1.0;   /* assume no LUT:  most PCs */
#endif

	/* the defaults above give 1.0, 1.3, 1.5 and 2.2, respectively: */
	default_screen_gamma = LUT_exponent * CRT_exponent;

	/* paranoia checks */
	if ( default_screen_gamma < PNG_GAMMA_THRESHOLD || default_screen_gamma > PNG_MAX_GAMMA)
		default_screen_gamma = 2.2;
	if ( default_screen_gamma < PNG_GAMMA_THRESHOLD || default_screen_gamma > PNG_MAX_GAMMA)
		return nil;

	return (void*)1;
}


#define outc(x){ strncpy( fi-> errbuf, x, 256); return false;}
#define outcm(dd){ snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", (int)(dd)); return false;}

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
#ifdef PNG_gAMA_SUPPORTED
	pset_f( gamma, 0.45455);
#endif
	pset_f( screen_gamma, default_screen_gamma);
	/*
	if background is not RGB, ( like clInvalid), use
	whatever it is in the file.
	*/
	pset_f( background, clInvalid);
	return profile;
}

/* first level loads png stream from the I/O, second level loads APNG frames from fdAT chunks */

/* m_ vars are imposed to all subsequent frames */
typedef struct _LoadRec {
	png_structp png_ptr, png_ptr2;
	png_infop   info_ptr, info_ptr2;

	Bool decompressed;
	Bool animated;
	int load_req, current_frame, last_row;
	Bool got_IHDR, want_fdat, got_frame_header;
	Bool has_alpha, want_nibbles, icon, convert_to_rgba;
	png_byte m_dataIHDR[12 + 13];
	png_byte m_dataPLTE[12 + 256 * 3];
	png_byte m_datatRNS[12 + 256];
	int m_sizePLTE, m_sizetRNS, m_hasgAMA, m_type, m_channels;
	double m_gamma;
	Color m_background, m_transparent_color;
	RGBColor m_palette[256];
	Byte * interlace_buffer;
	int interlaced_channels, m_alpha_size, m_palette_size;
} LoadRec;


static void
throw( png_structp png_ptr)
{
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR < 5
	longjmp( png_ptr-> jmpbuf, 1);
#else
	png_longjmp( png_ptr, 1);
#endif
}


static void
#ifdef PNGAPI
PNGAPI
#endif
warning_fn( png_structp png_ptr, png_const_charp msg)
{
	warn( "%s", msg);
}

static void
#ifdef PNGAPI
PNGAPI
#endif
error_fn( png_structp png_ptr, png_const_charp msg)
{
	char * buf = ( char *) png_get_error_ptr( png_ptr);
	if ( buf) strncpy( buf, msg, 256);
	throw(png_ptr);
}

static void
img_png_write (png_structp png_ptr, png_bytep data, png_size_t size)
{
	req_write( (( PImgLoadFileInstance) png_get_io_ptr(png_ptr))-> req, size, data);
}

static void
img_png_flush (png_structp png_ptr)
{
	req_flush( (( PImgLoadFileInstance) png_get_io_ptr(png_ptr))-> req);
}

static Bool
process_header( PImgLoadFileInstance fi, Bool use_subloader )
{
	dPROFILE;
	LoadRec * l = ( LoadRec *) fi-> instance;
	png_uint_32 width, height;
	int type, channels, bit_depth, color_type, interlace_type, filter, compression_type;
	HV * profile = fi->profile;
	png_structp png_ptr  = use_subloader ? l->png_ptr2  : l->png_ptr;
	png_infop   info_ptr = use_subloader ? l->info_ptr2 : l->info_ptr;
	PIcon i = (PIcon) fi->object;

	l->last_row = -1;
	l->got_frame_header = true;

	if ( l-> interlace_buffer ) {
		free( l-> interlace_buffer );
		l-> interlace_buffer = NULL;
	}

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, &compression_type, &filter);
	channels = l->m_channels;
	type     = l->m_type;
	
	switch ( bit_depth) {
	case 2:
		png_set_packing(png_ptr);
		break;
	case 16:
		png_set_strip_16(png_ptr);
		break;
	}


	switch ( color_type) {
	case PNG_COLOR_TYPE_PALETTE:
		if ( l-> convert_to_rgba ) {
			png_set_bgr(png_ptr);
			png_set_expand(png_ptr);
			channels = 4;
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		}
		break;
	case PNG_COLOR_TYPE_RGB:
	case PNG_COLOR_TYPE_RGB_ALPHA:
		png_set_bgr(png_ptr);
		break;
	}

	/* gamma stuff */
	{
		double screen_gamma = default_screen_gamma, gamma = 0.45455;
		Bool has_gamma = false;

		if ( pexist( screen_gamma)) {
			screen_gamma = pget_f( screen_gamma);
			if ( screen_gamma < PNG_GAMMA_THRESHOLD || screen_gamma > PNG_MAX_GAMMA) {
				sprintf( fi-> errbuf, "Error: screen_gamma value must be within %g..%g", PNG_GAMMA_THRESHOLD, (double)PNG_MAX_GAMMA);
				return false;
			}
		}

#ifdef PNG_gAMA_SUPPORTED
		if ( pexist( gamma)) {
			gamma = pget_f( gamma);
			if ( gamma < PNG_GAMMA_THRESHOLD || gamma > PNG_MAX_GAMMA) {
				sprintf( fi-> errbuf, "Error: gamma value must be within %g..%g", PNG_GAMMA_THRESHOLD, (double)PNG_MAX_GAMMA);
				return false;
			}
			has_gamma = true;
		} else if ( l->m_hasgAMA ) {
			has_gamma = true;
			gamma = l->m_gamma;
		}
#endif
		if ( has_gamma) png_set_gamma(png_ptr, screen_gamma, gamma);
	}

	/* alpha blending */
	if ( !l->icon && fi->blending ) {
		png_color_16 p;
		Color background = l->m_background;
		/* override with user data */
		if ( pexist( background)) 
			background = pget_i( background);
		if (( background & clSysFlag) == 0) {
			p.red   = (background & 0xFF0000) >> 16;
			p.green = (background & 0x00FF00) >> 8;
			p.blue  = (background & 0x0000FF);
			if (
				( color_type == PNG_COLOR_TYPE_GRAY_ALPHA) &&
				(( p.red != p.green) || (p.red != p.blue))
			) {
			/* backgrounding of GRAY_ALPHA with non-gray color can't be performed */
			/* into gray color space */
				png_set_gray_to_rgb(png_ptr);
				png_set_bgr(png_ptr);
				type = 24;
				channels = 4;
			}
			png_set_background(png_ptr, &p, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
		}
		png_set_strip_alpha(png_ptr);
	}

	/* Tell libpng to send us rows for interlaced pngs. */
	if (interlace_type == PNG_INTERLACE_ADAM7) {
		png_set_interlace_handling(png_ptr);
		l->interlaced_channels = channels;
		if ( l-> interlaced_channels < 3 ) l-> interlaced_channels += 2; /* gray -> rgb */
		l->interlace_buffer = malloc(width * height * l->interlaced_channels);
		if ( l->interlace_buffer == NULL ) {
			strcpy( fi-> errbuf, "Not enough memory");
			return false;
		}
	}

	/* sanity check */
	if (( type & imGrayScale) && (channels == 2)) {
		/* this is ok configuration */
	} else if (( type & imBPP) == 24 && channels == 4) {
		/* this is ok too */
	} else if ( color_type & PNG_COLOR_MASK_ALPHA ||
		channels == 2 ||
		channels == 4
	) {
		fi->blending = 0;
		l->icon      = 0;
		png_set_strip_alpha(png_ptr);
		warn("Unknown alpha channel coding scheme (%d %d %d %x)", channels, color_type, bit_depth, type);
	}

	i-> self-> create_empty((Handle)i, 1, 1, type);
	if (( type & imBPP ) < 24) {
		i-> palSize = l-> m_palette_size;
		memcpy( i-> palette, l-> m_palette, l-> m_palette_size * 3);
	}

	l->has_alpha = false;
	if ( l->icon) {
		if ( color_type & PNG_COLOR_MASK_ALPHA ) {
			l->has_alpha = true;
			i-> autoMasking = amNone;
			i-> self-> set_maskType((Handle) i, imbpp8 );
			pset_i( hasAlpha, 1 );
		} else if ( l->m_alpha_size < 0) {
			i-> maskColor = l-> m_transparent_color;
			i-> autoMasking = amMaskColor;
		} else if ( l->m_alpha_size > 0) {
			i-> maskIndex = l-> m_datatRNS[8];
			i-> autoMasking = amMaskIndex;
		}
	}
	
	png_read_update_info(png_ptr, info_ptr);

	if ( fi-> noImageData) {
		profile = fi->frameProperties;
		pset_i( width, width);
		pset_i( height, height);
		return true;
	}

	CImage( fi-> object)-> create_empty( fi-> object, width, height, type);
	if ( l->load_req == 0)
		EVENT_HEADER_READY(fi);

	return true;
}

static Byte
bit_depth_color( png_uint_16 component, int bit_depth)
{
	switch ( bit_depth ) {
	case 1:
		component = (component > 128) ? 255 : 0;
	case 2:
		component = (255/3) * component;
	case 4:
		component = (255/7) * component;
	case 16:
		component = component >> 8;
	default:
		component = component;
	}
	return component & 0xff;
}

static void
header_available(PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	int bit_depth, color_type, interlace_type, filter, compression_type;
	png_uint_32 width, height;
	
	png_get_IHDR(l->png_ptr, l->info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, &compression_type, &filter);
	if ( fi->loadExtras) {
		HV * profile = fi->fileProperties;
		pset_i( screenWidth, width);
		pset_i( screenHeight, height);
	}

	l->m_channels = (int) png_get_channels(l->png_ptr, l->info_ptr);
	if (l->m_channels > 4) {
		sprintf( fi-> errbuf, "Images with %d channels are not supported", l->m_channels);
		throw( l->png_ptr );
	}

	l->m_type = bit_depth;
	l->want_nibbles = false;
	switch ( bit_depth) {
	case 1: case 4: case 8:
		break;
	case 2:
		l->m_type = 4;
		l->want_nibbles = true;
		break;
	case 16:
		l->m_type = 8;
		break;
	default:
		sprintf( fi-> errbuf, "Bit depth %d is not supported", bit_depth);
		throw( l->png_ptr );
	}

	switch ( color_type) {
	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		l->m_type |= imGrayScale;
		break;
	case PNG_COLOR_TYPE_PALETTE:
		break;
	case PNG_COLOR_TYPE_RGB:
	case PNG_COLOR_TYPE_RGB_ALPHA:
		l->m_type = imRGB;
		break;
	default:
		sprintf( fi-> errbuf, "Unknown file color type: %d", color_type);
		throw( l->png_ptr );
	}


	png_save_uint_32(l->m_dataIHDR, 13);
	memcpy(l->m_dataIHDR + 4, "IHDR", 4);
	png_save_uint_32(l->m_dataIHDR + 8, width);
	png_save_uint_32(l->m_dataIHDR + 12, height);
	l->m_dataIHDR[16] = bit_depth;
	l->m_dataIHDR[17] = color_type;
	l->m_dataIHDR[18] = compression_type;
	l->m_dataIHDR[19] = filter;
	l->m_dataIHDR[20] = interlace_type;

	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_colorp palette;
		int i, palette_size = 0;
		RGBColor *pal;
		if ( !png_get_valid( l->png_ptr, l->info_ptr, PNG_INFO_PLTE)) {
			sprintf( fi-> errbuf, "Palette not found");
			throw( l->png_ptr );
		}
		png_get_PLTE(l->png_ptr, l->info_ptr, &palette, &palette_size);
		pal = l->m_palette;
		l-> m_palette_size = palette_size;
		if ( palette_size > 256 ) palette_size = 256;

		palette_size *= 3;
		png_save_uint_32(l->m_dataPLTE, palette_size);
		memcpy(l->m_dataPLTE + 4, "PLTE", 4);
		memcpy(l->m_dataPLTE + 8, palette, palette_size);
		l->m_sizePLTE = palette_size + 12;

		for ( i = 0; i < l->m_palette_size; i++, palette++, pal++) {
			pal-> r = palette-> red;
			pal-> g = palette-> green;
			pal-> b = palette-> blue;
		}
	}

#ifdef PNG_tRNS_SUPPORTED
	if ( png_get_valid( l->png_ptr, l->info_ptr, PNG_INFO_tRNS)) {
		png_bytep trns_t = 0;
		int trns_n = 0;
		png_color_16p trns_p;
		HV * profile = fi-> frameProperties;

		png_get_tRNS(l->png_ptr, l->info_ptr, &trns_t, &trns_n, &trns_p);

		if (color_type == PNG_COLOR_TYPE_RGB) {
			l->m_transparent_color = ARGB(
				bit_depth_color(trns_p->red, bit_depth),
				bit_depth_color(trns_p->green, bit_depth),
				bit_depth_color(trns_p->blue, bit_depth)
			);
			l->m_alpha_size = -1;
			png_save_uint_16(l->m_datatRNS +  8, trns_p->red);
			png_save_uint_16(l->m_datatRNS + 10, trns_p->green);
			png_save_uint_16(l->m_datatRNS + 12, trns_p->blue);
			trns_n = 6;
			if ( fi-> loadExtras) 
				pset_i( transparent_color, l->m_transparent_color);
		} else if (color_type == PNG_COLOR_TYPE_GRAY) {
			l->m_alpha_size = -1;
			l->m_transparent_color = bit_depth_color(trns_p->gray, bit_depth) * 0x10101;
			png_save_uint_16(l->m_datatRNS + 8, trns_p->gray);
			trns_n = 2;
			if ( fi-> loadExtras) 
				pset_i( transparent_color, l->m_transparent_color);
		} else if (color_type == PNG_COLOR_TYPE_PALETTE) {
			memcpy(l->m_datatRNS + 8, trns_t, trns_n);
			l-> m_alpha_size = trns_n;
			if ( l->icon && (l-> m_alpha_size > 1 || ( l-> m_alpha_size ==  1 && *trns_t != 0 ))) {
				l-> m_type = imRGB;
				l-> convert_to_rgba = true;
			}

			if ( fi-> loadExtras && trns_t) {
				int i;
				HV * profile = fi-> frameProperties;
				AV * av = newAV();
				for ( i = 0; i < trns_n; i++)
					av_push( av, newSViv( trns_t[i]));
				pset_sv_noinc( transparency_table, newRV_noinc(( SV*) av));
			}
		}
		
		png_save_uint_32(l->m_datatRNS, trns_n);
		memcpy(l->m_datatRNS + 4, "tRNS", 4);
		l->m_sizetRNS = trns_n + 12;

	}
#endif

	l->m_hasgAMA = false;
#ifdef PNG_gAMA_SUPPORTED
	if ( png_get_gAMA(l->png_ptr, l->info_ptr, &l->m_gamma)) {
		l->m_hasgAMA = true;
		if ( fi->loadExtras) {
			HV * profile = fi->fileProperties;
			pset_f( gamma, l->m_gamma);
		}
	}
#endif

	/* image background color */
	l-> m_background = clInvalid;
#ifdef PNG_bKGD_SUPPORTED
	if ( png_get_valid( l->png_ptr, l->info_ptr, PNG_INFO_bKGD)) {
		RGBColor r;
		png_color_16p pBackground;
		png_get_bKGD(l->png_ptr, l->info_ptr, &pBackground);

		if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
			r.r = r.g = r.b = bit_depth_color(pBackground->gray, bit_depth);
		} else {
			r.r = bit_depth_color(pBackground->red, bit_depth);
			r.g = bit_depth_color(pBackground->green, bit_depth);
			r.b = bit_depth_color(pBackground->blue, bit_depth);
		}
		l->m_background = ARGB(r.r, r.g, r.b);
	}
#endif

	/* store to load output */
	if ( fi-> loadExtras) {
		HV * profile = fi-> fileProperties;
		if ( l->m_background != clInvalid)
			pset_i( background, l->m_background);
		pset_i( interlaced, interlace_type == PNG_INTERLACE_ADAM7);
#ifdef PNG_INTRAPIXEL_DIFFERENCING
		pset_i( mng_datastream, filter == PNG_INTRAPIXEL_DIFFERENCING);
#endif
	}

	l->got_IHDR = true;
	if ( fi-> frameCount < 0)
		fi->frameCount = 1;
	if ( !l->animated)
		l->current_frame++;

	if (fi->frame == 0) {
		if ( !process_header( fi, false ))
			throw( l->png_ptr );
	}
}

static void
#ifdef PNGAPI
PNGAPI
#endif
frame_header(png_structp png, png_infop info)
{
	PImgLoadFileInstance fi = (PImgLoadFileInstance) png_get_progressive_ptr(png);
	if ( !process_header( fi, true ))
		throw( png );
}

static void
#ifdef PNGAPI
PNGAPI
#endif
row_available(png_structp png, png_bytep row_buffer, png_uint_32 row_index, int interlace_pass)
{
	PImgLoadFileInstance fi = (PImgLoadFileInstance) png_get_progressive_ptr(png);
	LoadRec * l = ( LoadRec *) fi-> instance;
	PIcon i = PIcon( fi-> object);
	Byte *src, *dst;

	/*
	Nothing to do if the row is unchanged, or the row is outside
	the image bounds: libpng may send extra rows, ignore them to
	make our lives easier.
	*/
	if (!row_buffer || row_index >= i->h)
	    return;

	if ( l-> interlace_buffer) {
		png_bytep row = l->interlace_buffer + (row_index * l->interlaced_channels * i->w);
		png_progressive_combine_row(png, row, row_buffer);
		src = (Byte*) row;
		if ( l->load_req == 0) {
			if (l->last_row > row_index)
				EVENT_SCANLINES_RESET(fi);
			l->last_row = row_index;
		}
	} else {
		src = (Byte*) row_buffer;
	}

	l->decompressed = true;
	dst = i-> data + (i->h - row_index - 1) * i-> lineSize;
	if ( l->has_alpha ) {
		int w  = i->w;
		Byte *a = i->mask + (i->h - row_index - 1) * i-> maskLine;
		if ( i-> type == imRGB ) {
			if ( fi->blending ) {
				while ( w-- > 0 ) {
					register uint16_t r = *src++;
					register uint16_t g = *src++;
					register uint16_t b = *src++;
					register uint16_t A = *src++;
					*dst++ = r * A >> 8;
					*dst++ = g * A >> 8;
					*dst++ = b * A >> 8;
					*a++   = A;
				}
			} else {
				while ( w-- > 0 ) {
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
					*a++   = *src++;
				}
			}
		} else {
			if ( fi->blending ) {
				while ( w-- > 0 ) {
					register uint16_t A = *src++;
					*dst++ = (*src++ * A) >> 8;
					*a++ = A;
				}
			} else {
				while ( w-- > 0 ) {
					*dst++ = *src++;
					*a++ = *src++;
				}
			}
		}
	} else if ( l->want_nibbles == 2) {
		/* convert from 8-bit down to 4-bit */
		bc_byte_nibble_cr( src, dst, i->w, map_stdcolorref);
	} else {
		/* plain gray or rgb */
		memcpy( dst, src, i->w * (i->type & imBPP) / 8);
	}

	if ( l->load_req == 0) /* report events only on first image loaded */
		EVENT_TOPDOWN_SCANLINES_READY(fi,1);
}

static void
png_complete(PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	HV * profile = fi-> fileProperties;
	char * name, *pf;
	int ct, i;
	png_uint_32 pl, py;
	png_int_32 pli, pyi;
	png_textp tx;
	double scx, scy;

	if ( l-> load_req == 0) /* report events only on first image loaded */
		EVENT_TOPDOWN_SCANLINES_FINISHED(fi);

	(void)tx; (void)py; (void)pl; (void)pyi; (void)pli; (void)i; (void)ct;
	(void)pf; (void)name; (void)scx; (void)scy;
		
	/* misc extras  */
	if ( !fi-> loadExtras) return;


#ifdef PNG_sRGB_SUPPORTED
	if ( png_get_sRGB( l->png_ptr, l->info_ptr, &i)) {
		char * c = "none";
		switch (i) {
		case PNG_sRGB_INTENT_SATURATION: c = "saturation"; break;
		case PNG_sRGB_INTENT_PERCEPTUAL: c = "perceptual"; break;
		case PNG_sRGB_INTENT_RELATIVE:   c = "relative"; break;
		case PNG_sRGB_INTENT_ABSOLUTE:   c = "absolute"; break;
		}
		pset_c( render_intent, c);
	}
#endif

#ifdef PNG_iCCP_SUPPORTED
	if ( png_get_iCCP( l->png_ptr, l->info_ptr, &name, &ct, (png_bytepp)&pf, &pl)) {
		pset_c( iccp_name, name);
		if ( pf) pset_sv_noinc( iccp_profile, newSVpv( pf, pl));
	}
#endif

#ifdef PNG_TEXT_SUPPORTED
	if ( png_get_text( l->png_ptr, l->info_ptr, &tx, &ct)) {
		HV * hash = newHV();
		for ( i = 0; i < ct; i++, tx++)
			(void) hv_store( hash, tx-> key, strlen( tx-> key), newSVpv( tx-> text, tx-> text_length), 0);
		pset_sv_noinc( text, newRV_noinc(( SV *) hash));
	}
#endif

#ifdef PNG_oFFs_SUPPORTED
	if ( png_get_oFFs( l->png_ptr, l->info_ptr, &pli, &pyi, &i)) {
		pset_i( offset_x, pli);
		pset_i( offset_y, pyi);
		pset_c( offset_dimension, ( i == PNG_OFFSET_PIXEL) ? "pixel" : "micrometer");
	}
#endif

#ifdef PNG_pHYs_SUPPORTED
	if ( png_get_pHYs( l->png_ptr, l->info_ptr, &pl, &py, &i)) {
		pset_i( resolution_x, pl);
		pset_i( resolution_y, py);
		pset_c( resolution_dimension, ( i == PNG_RESOLUTION_METER) ? "meter" : "unknown");
	}
#endif

#ifdef PNG_sCAL_SUPPORTED
	if ( png_get_sCAL( l->png_ptr, l->info_ptr, &i, &scx, &scy)) {
		pset_f( scale_x, scx);
		pset_f( scale_y, scy);
		pset_c( scale_unit,
			( i == PNG_SCALE_METER) ? "meter" :
			(( i == PNG_SCALE_RADIAN) ? "radian" : "unknown")
		);
	}
#endif
}

#ifdef APNG
static void
process_fcTL(PImgLoadFileInstance fi, png_unknown_chunkp chunk)
{
	HV * profile;
	LoadRec * l = ( LoadRec *) fi-> instance;
        uint32_t w   = png_get_uint_32(chunk->data + 4);
        uint32_t h   = png_get_uint_32(chunk->data + 8);
        uint32_t x   = png_get_uint_32(chunk->data + 12);
        uint32_t y   = png_get_uint_32(chunk->data + 16);
        uint16_t num = png_get_uint_16(chunk->data + 20);
        uint16_t den = png_get_uint_16(chunk->data + 22);
        int dispose  = chunk->data[24];
        int blend    = chunk->data[25];
	static png_byte dataPNG[8] = {137, 80, 78, 71, 13, 10, 26, 10};
	static png_byte datagAMA[16] = {0, 0, 0, 4, 103, 65, 77, 65};

	l->current_frame++;
	if ( l->current_frame != fi->frame)
		return;

	profile = fi-> frameProperties;
	if ( fi-> loadExtras) {
		pset_i( left,      x);
		pset_i( top,       y);
		pset_i( delayTime, den ? (num * 1000 / den) : (num * 10));
		pset_c( disposalMethod,
			(dispose == 2) ? "restore"    :
			(dispose == 1) ? "background" :
			(dispose == 0) ? "none"       : "unknown"
		);
		pset_c( blendMethod, blend ? "blend" : "no_blend");
	}

	if ( !l->got_IHDR) /* this one is for IDAT */
		return;

	if ( fi-> noImageData) {
		pset_i( width, w);
		pset_i( height, h);
		CImage( fi-> object)-> create_empty( fi-> object, 1, 1, l->m_type);
		return;
	}

	if ( l->png_ptr2) {
		png_destroy_read_struct(&l->png_ptr2, &l->info_ptr2, (png_infopp)NULL);
		l->png_ptr2 = NULL;
		l->info_ptr2 = NULL;
	}

	if (( l->png_ptr2 = png_create_read_struct(PNG_LIBPNG_VER_STRING, fi->errbuf, error_fn, warning_fn)) == NULL) {
		strcpy(fi->errbuf, "Not enough memory");
		throw(l->png_ptr);
	}
	if (( l->info_ptr2 = png_create_info_struct(l->png_ptr2)) == NULL) {
		strcpy(fi->errbuf, "Not enough memory");
		throw(l->png_ptr);
	}
	if (setjmp(png_jmpbuf( l->png_ptr2)) != 0)
		throw(l->png_ptr);

	png_set_crc_action(l->png_ptr2, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
	png_set_progressive_read_fn(l->png_ptr2, (void*)fi,
		frame_header, row_available, NULL);

	png_save_uint_32(l->m_dataIHDR + 8, w);
	png_save_uint_32(l->m_dataIHDR + 12, h);
	memcpy(l->m_dataIHDR + 8, chunk->data + 4, 8);
	png_process_data(l->png_ptr2, l->info_ptr2, dataPNG, 8);
	png_process_data(l->png_ptr2, l->info_ptr2, l->m_dataIHDR, 25);
	if ( l->m_hasgAMA) {
		png_save_uint_32(datagAMA + 8, l->m_gamma);
		png_process_data(l->png_ptr2, l->info_ptr2, datagAMA, 16);
	}
	if (l->m_sizePLTE > 0)
		png_process_data(l->png_ptr2, l->info_ptr2, l->m_dataPLTE, l->m_sizePLTE);
	if (l->m_sizetRNS > 0)
		png_process_data(l->png_ptr2, l->info_ptr2, l->m_datatRNS, l->m_sizetRNS);
	l->want_fdat = true;
}
#endif

static int
#ifdef PNGAPI
PNGAPI
#endif
read_chunks(png_structp png, png_unknown_chunkp chunk)
{
#ifdef APNG
	PImgLoadFileInstance fi = (PImgLoadFileInstance) png_get_user_chunk_ptr(png);
	LoadRec * l = ( LoadRec *) fi-> instance;
			
	if ( 
		STREQ(chunk->name, "acTL") &&
		(chunk->size == 8) &&
		!l->animated
	) {
		uint32_t loops, frames;
        	frames  = png_get_uint_32(chunk->data);
		loops   = png_get_uint_32(chunk->data + 4);
		if ( frames > 0 && frames < PNG_UINT_31_MAX && loops < PNG_UINT_31_MAX) {
			l-> animated = true;
			l-> current_frame = -1; /* reset if IHDR changed it */
			fi-> frameCount = frames;
			if ( fi-> loadExtras ) {
				HV * profile = fi->fileProperties;
				pset_i( loopCount, loops);
			}
		}
	} else if (
		STREQ(chunk->name, "fcTL") &&
		(chunk->size == 26) &&
		l->animated
	) {
		process_fcTL(fi, chunk);
	} else if (
		STREQ(chunk->name, "fdAT") &&
		(chunk->size > 4) &&
		l->animated &&
		l->want_fdat
	) {
		l->want_fdat = false;
	        png_save_uint_32(chunk->data, chunk->size - 4);
	        png_process_data(l->png_ptr2, l->info_ptr2, chunk->data, 4);
	        memcpy(chunk->data, "IDAT", 4);
	        png_process_data(l->png_ptr2, l->info_ptr2, chunk->data, chunk->size);
	        png_process_data(l->png_ptr2, l->info_ptr2, chunk->data, 4);
	}

#endif
	return true;
}

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	unsigned char buf[8];

	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return false;
	if ( req_read( fi-> req, 8, buf) != 8) {
		req_seek( fi-> req, 0, SEEK_SET);
		return false;
	}
	if ( png_sig_cmp( buf, 0, 8) != 0) {
		req_seek( fi-> req, 0, SEEK_SET);
		return false;
	}

	fi-> stop = true;

	l = malloc( sizeof( LoadRec));
	if ( !l) outcm( sizeof( LoadRec));
	memset( l, 0, sizeof( LoadRec));

	if ( !( l->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		fi-> errbuf, error_fn, warning_fn))) {
		free( l);
		return false;
	}

	if ( !( l->info_ptr = png_create_info_struct(l->png_ptr))) {
		png_destroy_read_struct(&l->png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		free( l);
		return false;
	}

	if (setjmp(png_jmpbuf(l->png_ptr)) != 0) {
		png_destroy_read_struct(&l->png_ptr, &l->info_ptr, (png_infopp)NULL);
		free(l);
		return false;
	}

	png_process_data(l->png_ptr, l->info_ptr, buf, 8);
        png_set_progressive_read_fn(l->png_ptr, fi, NULL, row_available, NULL);
#ifdef APNG
	{
        	png_byte apngChunks[]= {"acTL\0fcTL\0fdAT\0"};
        	png_set_keep_unknown_chunks(l->png_ptr, PNG_HANDLE_CHUNK_NEVER, apngChunks, 3);
		png_set_read_user_chunk_fn(l->png_ptr, (void*)fi, read_chunks);
	}
#endif

	fi-> instance = l;
	l-> current_frame = -1;
	l-> load_req = -1;
	return l;
}

static Bool
my_read( PImgLoadFileInstance fi, ssize_t size, void * data)
{
	ssize_t n;
	if ( size == 0) return true;
	n = req_read( fi-> req, size, data);
	if ( n < 0 ) {
		snprintf( fi-> errbuf, 256, "I/O error:%s", strerror(req_error( fi-> req)));
		return false;
	} else if ( n < size ) {
		snprintf( fi-> errbuf, 256, "I/O error: premature data end");
		return false;
	} else {
		return true;
	}
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi);

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
#define BUFSIZE 16384
	png_uint_32 size;
	Byte buf[BUFSIZE], chunk[5];
	LoadRec * l = ( LoadRec *) fi-> instance;
	HV * profile = fi-> profile;
	Bool had_DAT = false, reset_to_chunk_start = false, got_frame_body = false;

	l-> load_req++;

	/* rewind */
	if ( fi->frame > 0 && fi->frame < l->current_frame) {
		void * newinst, * oldinst = fi->instance;
		if (( newinst = open_load(instance, fi)) == NULL) {
			fi->instance = oldinst;
			strcpy(fi->errbuf, "Cannot reinitialize loader");
			return false;
		}
		(( LoadRec *) newinst)-> load_req = l-> load_req;
		fi->instance = oldinst;
		close_load(instance, fi);
		fi->instance = newinst;
		l = ( LoadRec *) newinst;
	}

	l->icon = fi->object ? kind_of( fi-> object, CIcon) : false;
	if ( l->icon && pexist( background)) {
		strcpy( fi-> errbuf, "Option 'background' cannot be set when loading to an Icon object");
		return false;
	}

	if (setjmp(png_jmpbuf(l->png_ptr)) != 0) {
		l = ( LoadRec *) fi-> instance;
		if ( l && l->decompressed ) {
			fi-> wasTruncated = 1;
			return !fi->noIncomplete;
		}
		return false;
	}


	/* PNG chunk structure:
		4      - length
		4      - chunk type
		length - payload
		4      - crc
	*/
	while ( 1 ) {
		png_uint_32 n_need, n_read;
		Bool feed;

		/* read chunk header */
		if ( !my_read( fi, 8, (void*)buf))
			return false;
		memcpy( chunk, buf + 4, 4);
		chunk[4] = 0;
		size = png_get_uint_32(buf);

		/* see if we need to feed the data to libpng :
		- any frame needs acTL, IHDR, RNS etc common chunks
		- first frame needs IDATs, and possibly first fcTL, if APNG
		- next frames need fcTL and fdATs
		- extras need anything after payload, such at tEXt etc

		current_frame is increased by fcTL's in animated mode, and by IHDR otherwise
		*/
		feed = true;
		if ( fi->frame == 0 ) {
			if ( STREQ(chunk, "IDAT")) 
				had_DAT = true;
			else if (had_DAT) {
				if (!fi->noImageData) 
					got_frame_body = true;
				if (
					!fi->loadExtras ||
					/* report extras on last image only to avoid constant rescans on loadAll */
					l->load_req < ((fi->frameMap ? fi->frameMapSize : fi->frameCount) - 1)
				) {
					reset_to_chunk_start = true;
					break;
				}
			}

			if ( STREQ(chunk, "fdAT")) {
				feed = false;
			} else if ( had_DAT && STREQ(chunk, "fcTL")) {
				feed = false;
				l->current_frame++;
			}
		} else if ( fi->frame > 0) {
			if ( STREQ(chunk, "fdAT")) {
				if ( fi->frame != l->current_frame)
					feed = false;
				else
					had_DAT = true;
			} else if ( had_DAT ) {
				if (!fi->noImageData && fi->frame == l->current_frame) 
					got_frame_body = true;
				if (
					!fi->loadExtras ||
					/* report extras on last image only to avoid constant rescans on loadAll */
					l->load_req < ((fi->frameMap ? fi->frameMapSize : fi->frameCount) - 1)
				) {
					reset_to_chunk_start = true;
					break;
				}
			}

			if ( STREQ(chunk, "IDAT")) {
				feed = false;
			} else if ( fi->frame != l->current_frame + 1 && STREQ(chunk, "fcTL")) {
				feed = false;
				l->current_frame++;
			}
		} else {
			/* local special case: null load request, assumed loadExtras = true */
			if ( 
				STREQ(chunk, "fcTL") || STREQ(chunk, "fdAT") || STREQ(chunk, "IDAT")
			)
				feed = false;
		}

		/* we count frames by headers passed, as data may be skipped */
		if ( fi->noImageData ) {
			if ( STREQ(chunk, "fdAT") || STREQ(chunk, "IDAT")) 
				feed = false;
		}

		/* libpng calls it when IDAT is met and it is too late to stop its unrolling - 
		call it ourselves */
		if ( !l->got_frame_header && STREQ(chunk, "IDAT"))
			header_available(fi);

		if ( !feed ) {
			if ( req_seek( fi-> req, size + 4, SEEK_CUR) < 0) {
				snprintf( fi-> errbuf, 256, "I/O error:%s", strerror(req_error( fi-> req)));
				return false;
			}
			continue;
		}

		/* don't feed it to libpng because it insists on having IDAT if IHDR was eaten.
		   but we don't want to unroll IDAT if we don't need it */
		if ( STREQ(chunk, "IEND"))
			break;

		/* read first, or the only needed, portion */
		n_read = n_need = size + 4;
		if ( n_read > BUFSIZE - 8)
			n_read = BUFSIZE - 8;
		if (!my_read( fi, n_read, (void*)(buf + 8)))
			return false;
		png_process_data(l->png_ptr, l->info_ptr, buf, n_read + 8);
		n_need -= n_read;
		
		/* read to the end of chunk */
		while ( n_need > 0 ) {
			n_read = n_need;
			if ( n_read > BUFSIZE)
				n_read = BUFSIZE;
			if (!my_read( fi, n_read, (void*)buf))
				return false;
			png_process_data(l->png_ptr, l->info_ptr, buf, n_read);
			n_need -= n_read;
		}
	}

	if ( !l->got_frame_header) {
		sprintf(fi->errbuf, "Frame header not found");
		return false;
	}
	if ( !fi->noImageData && !got_frame_body) {
		sprintf(fi->errbuf, "Frame data not found");
		return false;
	}

	png_complete(fi);
	if ( reset_to_chunk_start ) {
		if ( req_seek( fi->req, -8, SEEK_CUR) < 0) {
			snprintf( fi-> errbuf, 256, "I/O error:%s", strerror(req_error( fi-> req)));
			return false;
		}
	}

	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;

	if ( fi-> loadExtras && fi->frameMapSize == 0) {
		/* null load request, ignore failure */
		fi-> frame = -1;
		load( instance, fi);
	}
	if ( l-> interlace_buffer) free( l-> interlace_buffer);
	if ( l->png_ptr2)
		png_destroy_read_struct(&l->png_ptr2, &l->info_ptr2, (png_infopp)NULL);
	png_destroy_read_struct(&l->png_ptr, &l->info_ptr, (png_infopp)NULL);
	free( l);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( background, clInvalid);
#ifdef PNG_gAMA_SUPPORTED
	pset_f( gamma, 0.45455);
#endif
#ifdef PNG_iCCP_SUPPORTED
	pset_c( iccp_name, "unspecified");
	pset_c( iccp_profile, "");
#endif
	pset_i( interlaced, 0);
#ifdef PNG_INTRAPIXEL_DIFFERENCING
	pset_i( mng_datastream, 0);
#endif
#ifdef PNG_oFFs_SUPPORTED
	pset_i( offset_x, 0);
	pset_i( offset_y, 0);
	pset_c( offset_dimension, "pixel");
#endif
#ifdef PNG_sRGB_SUPPORTED
	pset_c( render_intent, "none");
#endif
#ifdef PNG_pHYs_SUPPORTED
	pset_i( resolution_x, 0);
	pset_i( resolution_y, 0);
	pset_c( resolution_dimension, "meter");
#endif
#ifdef PNG_sCAL_SUPPORTED
	pset_f( scale_x, 1);
	pset_f( scale_y, 1);
	pset_c( scale_unit, "unknown");
#endif
#ifdef PNG_TEXT_SUPPORTED
	pset_sv_noinc( text, newRV_noinc((SV*)newHV()));
#endif
#ifdef PNG_tRNS_SUPPORTED
	pset_sv_noinc( transparent_color, newSViv(clInvalid));
	pset_sv_noinc( transparent_color_index, newSViv(-1));
#endif
	return profile;
}

typedef struct _SaveRec {
	png_structp png_ptr;
	png_infop info_ptr;
	Byte * b8_4;
	Byte * line;
} SaveRec;

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * l;

	l = malloc( sizeof( SaveRec));
	if ( !l) return nil;

	memset( l, 0, sizeof( SaveRec));

	if ( !( l-> png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		fi-> errbuf, error_fn, warning_fn))) {
		free( l);
		return false;
	}

	if ( !( l-> info_ptr = png_create_info_struct(l->png_ptr))) {
		png_destroy_write_struct(&l->png_ptr, (png_infopp)NULL);
		free( l);
		return false;
	}

	fi-> instance = l;

	if (setjmp(png_jmpbuf( l-> png_ptr))) {
		/* If we get here, we had a problem inside open_load */
		png_destroy_write_struct(&l-> png_ptr, &l-> info_ptr);
		fi-> instance = nil;
		free( l);
		return false;
	}
	png_set_write_fn( l-> png_ptr, fi, img_png_write, img_png_flush);

	return l;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	PIcon i;
	SaveRec * l;
	HV * profile;
	Bool icon;
	Byte * alpha;

	l = ( SaveRec *) fi-> instance;
	if ( setjmp( png_jmpbuf( l-> png_ptr)) != 0) return false;

	l = ( SaveRec *) fi-> instance;
	icon = kind_of( fi-> object, CIcon);
	i = ( PIcon) fi-> object;
	profile = fi-> objectExtras;
	alpha = NULL;

	/* header */
	{
		int bit_depth, color_type, interlace = PNG_INTERLACE_NONE,
			filter = PNG_FILTER_TYPE_DEFAULT;
		if (( i-> type & imBPP) == 24) {
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_RGB;
		} else {
			color_type = ( i-> type & imGrayScale) ?
				PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_PALETTE;
			bit_depth = i-> type & imBPP;
		}
		if ( pexist( interlaced) && pget_B( interlaced))
			interlace = PNG_INTERLACE_ADAM7;
#ifdef PNG_INTRAPIXEL_DIFFERENCING
		if ( pexist( mng_datastream) && pget_B( mng_datastream))
			filter = PNG_INTRAPIXEL_DIFFERENCING;
#endif
		if ( icon && i-> autoMasking != amMaskIndex && i-> autoMasking != amMaskColor ) {
			Bool autoConvert;
			{
				/* recognize autoConvert as the image subsystem does */
				HV * profile = fi-> extras;
				autoConvert = pexist( autoConvert) ? pget_B( autoConvert) : true;
			}

			/* png doesn't support paletted images with alpha channel? */
			if ( color_type == PNG_COLOR_TYPE_PALETTE) {
				if ( !autoConvert)
					outc("Cannot apply alpha channel to a paletted image");
				CImage( i)-> set_type(( Handle) i, imRGB);
				if ( i-> type != imRGB)
					outc("Failed converting image to type im::RGB");
				color_type = PNG_COLOR_TYPE_RGB;
			}

			/* and does not alpha with bit_depth other that 8 or 24 bits? */
			if ( bit_depth != 8) {
				if ( !autoConvert)
					outc( "Image depth must be of 8 bits to be saved with alpha channel");
				CImage( i)-> set_type(( Handle) i, imbpp8 | ( i-> type & imGrayScale));
				if (( i-> type & imBPP) != 8)
					outc("Failed converting image to 8-bit");
				bit_depth = 8;
			}

			if ( i-> maskType != imbpp8) {
				if ( !autoConvert)
					outc("maskType is not of type im::bpp8");
				i-> self-> set_maskType( fi-> object, imbpp8);
				if ( i-> maskType != imbpp8 )
					outc("Failed converting icon mask to type im::bpp8");
			}
			if (( l-> line = malloc( i-> w * (( color_type == PNG_COLOR_TYPE_GRAY) ? 2 : 4)))) {
				alpha = i-> mask;
				color_type |= PNG_COLOR_MASK_ALPHA;
			}
		}
		png_set_IHDR( l-> png_ptr, l-> info_ptr, i-> w, i-> h, bit_depth, color_type,
			interlace, PNG_COMPRESSION_TYPE_DEFAULT, filter);
	}

	/* palette */
	if ((( i-> type & imBPP) <= 8) && ((i-> type & imGrayScale) == 0)) {
		RGBColor * pal = i-> palette;
		int num_palette, j;
		png_color palette[256];

		num_palette = ( i-> palSize < 256) ? i-> palSize : 256;
		for ( j = 0; j < num_palette; j++, pal++) {
			palette[j].red   = pal-> r;
			palette[j].green = pal-> g;
			palette[j].blue  = pal-> b;
		}
		png_set_PLTE(l->png_ptr, l->info_ptr, palette, num_palette);
	}

	/* sRGB */
#ifdef PNG_sRGB_SUPPORTED
	if ( pexist( render_intent)) {
		char * c = pget_c( render_intent);
		if ( stricmp( c, "none") != 0) {
			int i;
			if ( stricmp( c, "saturation") == 0) i = PNG_sRGB_INTENT_SATURATION; else
			if ( stricmp( c, "perceptual") == 0) i = PNG_sRGB_INTENT_PERCEPTUAL; else
			if ( stricmp( c, "relative")   == 0) i = PNG_sRGB_INTENT_RELATIVE; else
			if ( stricmp( c, "absolute")   == 0) i = PNG_sRGB_INTENT_ABSOLUTE; else {
				snprintf( fi-> errbuf, 256, "Unknown render_intent option '%s'", c);
				return false;
			}
			png_set_sRGB( l-> png_ptr, l-> info_ptr, i);
		}
	}
#endif

	/* gamma */
#ifdef PNG_gAMA_SUPPORTED
	if ( pexist( gamma)) {
		double gamma = pget_f( gamma);
		if ( gamma < PNG_GAMMA_THRESHOLD || gamma > PNG_MAX_GAMMA) {
			sprintf( fi-> errbuf, "Gamma value must be within %g..%g", PNG_GAMMA_THRESHOLD, (double)PNG_MAX_GAMMA);
			return false;
		}
		png_set_gAMA( l-> png_ptr, l-> info_ptr, gamma);
	}
#endif

	/* iccp */
#ifdef PNG_iCCP_SUPPORTED
	if ( pexist( iccp_name) || pexist( iccp_profile)) {
		char * prf = nil;
		STRLEN plen = 0;
		char * name = pexist( iccp_name) ? pget_c( iccp_name) : "unspecified";
		if ( pexist( iccp_profile)) prf = SvPV( pget_sv( iccp_profile), plen);
		png_set_iCCP( l-> png_ptr, l-> info_ptr, name, 0, (void*) prf, plen);
	}
#endif

	/* tRNS */
#ifdef PNG_tRNS_SUPPORTED
	{
		png_color_16 trns_p[256];
		png_byte trns_t[256];
		int trns_n = 0;
		Color color = pexist( transparent_color) ? pget_i( transparent_color) : clInvalid;
		int   index = pexist( transparent_color_index) ? pget_i( transparent_color_index) : -1;

		if ( index > i-> palSize)
			index = i-> palSize - 1;
		if ( icon && (i-> autoMasking == amMaskIndex) && ( index < 0))
			index = i-> maskIndex;

		if ( color & clSysFlag)
			color = clInvalid;
		if ( icon && (i-> autoMasking == amMaskColor) && ( color == clInvalid))
			color = i-> maskColor;
		memset( trns_p, 0, sizeof( trns_p));
		if ( color != clInvalid || index >= 0) {
			memset( trns_t, 255, sizeof( trns_t));
			if (( i-> type & imBPP) < 24) {
				int x;
				if ( index < 0) {
					RGBColor r;
					r.r = (color & 0xFF0000) >> 16;
					r.g = (color & 0x00FF00) >> 8;
					r.b = (color & 0x0000FF);
					x = cm_nearest_color( r, i-> palSize, i-> palette);
				} else {
					x = index;
				}
				trns_t[x] = 0;
				trns_n = x + 1;
				trns_p[0].index = x;
				trns_p[x].index = x;
				png_set_tRNS(l->png_ptr, l-> info_ptr, trns_t, trns_n, trns_p);
			} else if ( color != clInvalid) {
				trns_p[0].red   = (color & 0xFF0000) >> 16;
				trns_p[0].green = (color & 0x00FF00) >> 8;
				trns_p[0].blue  = (color & 0x0000FF);
				png_set_tRNS(l->png_ptr, l-> info_ptr, nil, 1, trns_p);
			}

			/* maybe it is a good idea to adjust automatically backgound  */
			if ( !pexist( background) || (pget_i( background) & clSysFlag))
				pset_i( background, color);
		}
	}
#endif

	/* bKGD */
#ifdef PNG_bKGD_SUPPORTED
	if ( pexist( background)) {
		Color background = pget_i( background);
		if (( background & clSysFlag) == 0) {
			png_color_16 p;
			p.red   = (background & 0xFF0000) >> 16;
			p.green = (background & 0x00FF00) >> 8;
			p.blue  = (background & 0x0000FF);
			png_set_bKGD(l-> png_ptr, l-> info_ptr, &p);
		}
	}
#endif

	/* text */
#ifdef PNG_TEXT_SUPPORTED
	if ( pexist( text)) {
		HV * hv;
		HE * he;
		SV * sv = pget_sv( text);
		png_text * txt;
		int i = 0, len;

		if ( !SvOK(sv) || !SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVHV)
			outc("'text' is not a hash");
		hv = ( HV*) SvRV( sv);
		len = HvKEYS(( HV*) hv);
		if ( !( txt = malloc( len * sizeof( png_text)))) outcm( len * sizeof( png_text));
		memset( txt, 0, len * sizeof( png_text));

		hv_iterinit( hv);
		for (;;) {
			STRLEN vlen;
			if (( he = hv_iternext( hv)) == nil) break;
			txt[i]. compression =
#ifdef PNG_zTXt_SUPPORTED
				PNG_TEXT_COMPRESSION_zTXt;
#else
				PNG_TEXT_COMPRESSION_NONE;
#endif
			txt[i]. key =  (char*) HeKEY( he);
			txt[i]. text = (char*) SvPV( HeVAL( he), vlen);
			txt[i]. text_length = vlen;
			i++;
		}
		png_set_text(l-> png_ptr, l-> info_ptr, txt, len);
		free( txt);
	}
#endif

	/* offset */
#ifdef PNG_oFFs_SUPPORTED
	if ( pexist( offset_x) || pexist( offset_y) || pexist( offset_dimension)) {
		int ox = pexist( offset_x) ? pget_i( offset_x) : 0;
		int oy = pexist( offset_y) ? pget_i( offset_y) : 0;
		int dm = PNG_OFFSET_PIXEL;

		if ( pexist( offset_dimension)) {
			char * c = pget_c( offset_dimension);
			if ( stricmp( c, "pixel") == 0) dm = PNG_OFFSET_PIXEL; else
			if ( stricmp( c, "micrometer") == 0) dm = PNG_OFFSET_MICROMETER; else {
				snprintf( fi-> errbuf, 256, "unknown offset_dimension '%s'", c);
				return false;
			}
		}
		png_set_oFFs( l-> png_ptr, l-> info_ptr, ox, oy, dm);
	}
#endif

	/* phys */
#ifdef PNG_pHYs_SUPPORTED
	if ( pexist( resolution_x) || pexist( resolution_y) || pexist( resolution_dimension)) {
		int ox = pexist( resolution_x) ? pget_i( resolution_x) : 0;
		int oy = pexist( resolution_y) ? pget_i( resolution_y) : 0;
		int dm = PNG_RESOLUTION_METER;

		if ( pexist( resolution_dimension)) {
			char * c = pget_c( resolution_dimension);
			if ( stricmp( c, "unknown") == 0) dm = PNG_RESOLUTION_UNKNOWN; else
			if ( stricmp( c, "meter") == 0)   dm = PNG_RESOLUTION_METER; else {
				snprintf( fi-> errbuf, 256, "unknown resolution_dimension '%s'", c);
				return false;
			}
		}
		png_set_pHYs( l-> png_ptr, l-> info_ptr, ox, oy, dm);
	}
#endif

	/* scale */
#ifdef PNG_sCAL_SUPPORTED
	if ( pexist( scale_x) || pexist( scale_y) || pexist( scale_unit)) {
		double ox = pexist( scale_x) ? pget_f( scale_x) : 1;
		double oy = pexist( scale_y) ? pget_f( scale_y) : 1;
		int dm = PNG_SCALE_UNKNOWN;

		if ( pexist( scale_unit)) {
			char * c = pget_c( scale_unit);
			if ( stricmp( c, "unknown") == 0) dm = PNG_SCALE_UNKNOWN; else
			if ( stricmp( c, "meter") == 0)   dm = PNG_SCALE_METER;   else
			if ( stricmp( c, "radian") == 0)   dm = PNG_SCALE_RADIAN; else {
				snprintf( fi-> errbuf, 256, "unknown scale_unit '%s'", c);
				return false;
			}
		}
		png_set_sCAL( l-> png_ptr, l-> info_ptr, dm, ox, oy);
	}
#endif

	/* done with header, write it now */
	png_write_info(l-> png_ptr, l-> info_ptr);

	if (( i-> type & imBPP) == 24) png_set_bgr(l-> png_ptr);

	/* writing data */
	{
		int pass, num_pass = png_set_interlace_handling(l-> png_ptr);
		for (pass = 0; pass < num_pass; pass++) {
			int h = i-> h;
			Byte * data = i-> data + (i-> h - 1) * i-> lineSize;
			Byte * a_data = alpha;
			if ( a_data )
				a_data += ( h - 1) * i-> maskLine;
			while ( h--) {
				if ( a_data ) {
					int j;
					Byte * src = data, * dst = l-> line, *a = a_data;
					if ( i-> type == imRGB) {
						for ( j = 0; j < i-> w; j++) {
							*dst++ = *src++;
							*dst++ = *src++;
							*dst++ = *src++;
							*dst++ = *a++;
						}
					} else {
						for ( j = 0; j < i-> w; j++) {
							*dst++ = *src++;
							*dst++ = *a++;
						}
					}
					png_write_row( l-> png_ptr, l-> line);
				} else {
					png_write_row( l-> png_ptr, data);
				}
				data -= i-> lineSize;
				if ( a_data) a_data -= i-> maskLine;
			}
		}
	}

	/* end - now comments or time, if any */
	png_write_end(l-> png_ptr, NULL);
	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * l = ( SaveRec *) fi-> instance;
	png_destroy_write_struct(&l-> png_ptr, &l-> info_ptr);
	if ( l-> b8_4) free( l-> b8_4);
	if ( l-> line) free( l-> line);
	free( l);
}

void
apc_img_codec_png( void )
{
	struct ImgCodecVMT vmt;

	int ver_release=
#ifdef PNG_LIBPNG_VER_RELEASE
	PNG_LIBPNG_VER_RELEASE
#else
	PNG_LIBPNG_VER % 100
#endif
	;

	if ( png_access_version_number() != codec_info.versionMaj * 10000 + codec_info.versionMin * 100 + ver_release) {
		unsigned int v = (unsigned int) png_access_version_number();
		warn("Application built with libpng-%d.%d.%d but running with %d.%d.%d\n",
			codec_info.versionMaj, codec_info.versionMin, ver_release,
			v / 10000, (v % 10000) / 100, v % 100);
		return;
	}

	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));

	vmt. init          = init;

	vmt. load_defaults = load_defaults;
	vmt. open_load     = open_load;
	vmt. load          = load;
	vmt. close_load    = close_load;

	vmt. save_defaults = save_defaults;
	vmt. open_save     = open_save;
	vmt. save          = save;
	vmt. close_save    = close_save;

	apc_img_register( &vmt, nil);
}


#ifdef __cplusplus
}
#endif
