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


#include "img.h"
#include "img_conv.h"
#include "Icon.h"

#ifdef BROKEN_PERL_PLATFORM
#undef      setjmp
#undef      longjmp
#define     setjmp _setjmp
#endif

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
	nil
};

static char * loadOutput[] = { 
	"background",
	"blending",
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
	"transparent_colors",
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
	"",     /* module */
	"",     /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM,
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
#define outcm(dd){ snprintf( fi-> errbuf, 256, "No enough memory (%d bytes)", (int)(dd)); return false;}

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
	/* alpha blending:
		1 - combine user or file background with color channel and alpha channel
		0 - load color channels and alpha channel as is, without blending
	*/
	pset_i( blending, 1); 
	return profile;
}

typedef struct _LoadRec {
	png_structp png_ptr;
	png_infop info_ptr;
	Byte * b8_4;
	Byte * line;
} LoadRec;

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
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR < 5
	longjmp( png_ptr-> jmpbuf, 1);
#else
	png_longjmp( png_ptr, 1);
#endif
}

static void
img_png_read (png_structp png_ptr, png_bytep data, png_size_t size)
{
	req_read( (( PImgLoadFileInstance) png_get_io_ptr(png_ptr))-> req, size, data);
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

static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	unsigned char buf[8];

	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return false;
	if ( req_read( fi-> req, 8, buf) < 0) {
		req_seek( fi-> req, 0, SEEK_SET);
		return false;
	}
	if ( png_sig_cmp( buf, 0, 8) != 0) {
		req_seek( fi-> req, 0, SEEK_SET);
		return false;
	}

	fi-> stop = true;
	fi-> frameCount = 1;

	l = malloc( sizeof( LoadRec));
	if ( !l) outcm( sizeof( LoadRec));
	memset( l, 0, sizeof( LoadRec));

	if ( !( l-> png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		fi-> errbuf, error_fn, warning_fn))) {
		free( l);
		return false;
	}

	if ( !( l-> info_ptr = png_create_info_struct(l->png_ptr))) {
		png_destroy_read_struct(&l->png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		free( l);
		return false;
	}

	fi-> instance = l;
	if (setjmp(png_jmpbuf( l-> png_ptr))) {
		/* If we get here, we had a problem inside open_load */
		png_destroy_read_struct(&l-> png_ptr, &l-> info_ptr, (png_infopp)NULL);
		fi-> instance = nil;
		free( l);
		return false;
	}

	png_set_read_fn( l-> png_ptr, fi, img_png_read);
	png_set_sig_bytes( l-> png_ptr, 8);
	return l;
}

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	dPROFILE;
	LoadRec * l = ( LoadRec *) fi-> instance;
	png_uint_32 width, height;
	int obd, bit_depth, color_type, interlace_type, bpp, number_passes, pass, filter;
	HV * profile;
	Color background;
	int channels, trns_n;
	Bool icon, blending, post_blending;
	Byte * alpha;
	png_bytep trns_t;
	png_color_16p trns_p;

	if (setjmp(png_jmpbuf( l-> png_ptr)) != 0) return false;

	profile = fi-> profile;

	png_read_info(l->png_ptr, l->info_ptr);

	png_get_IHDR(l->png_ptr, l->info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, &filter);
	obd = bit_depth;
	channels = (int) png_get_channels(l->png_ptr, l->info_ptr);
	icon = kind_of( fi-> object, CIcon);
	
	switch ( bit_depth) {
	case 1: case 4: case 8:
		break;
	case 2:
		png_set_packing(l->png_ptr);
		bit_depth = 4;
		break;
	case 16:   
		png_set_strip_16(l->png_ptr);
		bit_depth = 8;
		break;
	default:
		sprintf( fi-> errbuf, "Bit depth %d is not supported", bit_depth);
		return false;
	}

	
	switch ( color_type) {
	case PNG_COLOR_TYPE_GRAY:
		bpp = bit_depth | imGrayScale;
		break;
	case PNG_COLOR_TYPE_PALETTE:
		bpp = bit_depth;
		break;
	case PNG_COLOR_TYPE_RGB:
		png_set_bgr(l-> png_ptr);
		bpp = 24;
		break; 
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		bpp = bit_depth | imGrayScale;
		break; 
	case PNG_COLOR_TYPE_RGB_ALPHA:
		png_set_bgr(l-> png_ptr);
		bpp = 24;
		break; 
	default:
		sprintf( fi-> errbuf, "Unknown file color type: %d", color_type);
		return false;
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
		} else if ( png_get_gAMA(l-> png_ptr, l-> info_ptr, &gamma)) {
			has_gamma = true;
			pset_f( gamma, gamma);
		}
#endif      

		if ( has_gamma) png_set_gamma(l->png_ptr, screen_gamma, gamma);
	}

	/* alpha blending */
	blending = pexist( blending) ? pget_B(blending) : 1;
	post_blending = false;
	if ( !icon && !blending)
		png_set_strip_alpha(l-> png_ptr);

	/* image background color */
	background = clInvalid;
#ifdef PNG_bKGD_SUPPORTED   
	if ( png_get_valid( l-> png_ptr, l-> info_ptr, PNG_INFO_bKGD)) {
		RGBColor r;
		png_color_16p pBackground;
		png_get_bKGD(l-> png_ptr, l-> info_ptr, &pBackground);
		
		if (obd == 16) {
			r.r = pBackground->red   >> 8;
			r.g = pBackground->green >> 8;
			r.b = pBackground->blue  >> 8;
		} else if (color_type == PNG_COLOR_TYPE_GRAY && obd < 8) {
			if (obd == 1)
					r.r = r.g = r.b = pBackground->gray? 255 : 0;
			else if (obd == 2)
					r.r = r.g = r.b = (255/3) * pBackground->gray;
			else 
					r.r = r.g = r.b = (255/15) * pBackground->gray;
		} else {
			r.r = (Byte)pBackground->red;
			r.g = (Byte)pBackground->green;
			r.b = (Byte)pBackground->blue;
		}
		background = ARGB(r.r, r.g, r.b);
	}
#endif   
	/* store to load output */
	if ( fi-> loadExtras) {
		HV * profile = fi-> frameProperties;
		if ( background != clInvalid)
			pset_i( background, background);
	}
	/* set blending background */
	if ( icon && pexist( background)) {
		strcpy( fi-> errbuf, "Option 'background' cannot be set when loading to an Icon object");
		return false;
	}
	if ( !icon && blending ) {
		png_color_16 p;
		/* override with user data */
		if ( pexist( background) && ((pget_i( background) & clSysFlag) == 0)) 
			background = pget_i( background);
		p.red   = (background & 0xFF0000) >> 16;
		p.green = (background & 0x00FF00) >> 8;
		p.blue  = (background & 0x0000FF);
		if (
			( color_type == PNG_COLOR_TYPE_GRAY_ALPHA) &&
			(( p.red != p.green) || (p.red != p.blue))
			) { /* backgrounding of GRAY_ALPHA with non-gray color can't be performed */
				/* into gray color space */
			png_set_gray_to_rgb(l-> png_ptr);
			png_set_bgr(l-> png_ptr);
			bpp = 24;
			channels = 4;
		}
		png_set_background(l->png_ptr, &p, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
	}

	number_passes = png_set_interlace_handling(l-> png_ptr);

	/* loading tRNS information */
	trns_n = 0;
	trns_t = nil;
	trns_p = nil;
#ifdef PNG_tRNS_SUPPORTED   
	if ( png_get_tRNS( l-> png_ptr, l-> info_ptr, &trns_t, &trns_n, &trns_p)) {
		if ( fi-> loadExtras) {
			int i;
			AV * av = newAV();
			HV * profile = fi-> frameProperties;
			for ( i = 0; i < trns_n; i++) 
				av_push( av, newSViv( trns_p[i]. index));
			pset_sv_noinc( transparent_colors, newRV_noinc(( SV*) av));
			if ( trns_t) {
				av = newAV();
				for ( i = 0; i < trns_n; i++) 
					av_push( av, newSViv( trns_t[i]));
				pset_sv_noinc( transparency_table, newRV_noinc(( SV*) av));
			}
		}
	}
#endif   

	/* synchronize final settings */
	png_read_update_info(l-> png_ptr, l-> info_ptr);

	CImage( fi-> object)-> create_empty( fi-> object, 1, 1, bpp);

	/* palette, if any */
	if ( bpp < 24) {
		RGBColor * pal = PImage( fi-> object)-> palette;
		int num_palette, i;
		png_colorp palette;
		if (png_get_PLTE(l->png_ptr, l->info_ptr, &palette, &num_palette)) {
			if ( num_palette > 256) num_palette = 256;
			PImage( fi-> object)-> palSize = num_palette;
			for ( i = 0; i < num_palette; i++, palette++, pal++) {
				pal-> r = palette-> red;
				pal-> g = palette-> green;
				pal-> b = palette-> blue;
			}
		}
	} 

	if ( fi-> noImageData) {
		(void) hv_store( fi-> frameProperties, "width",  5, newSViv( width), 0);
		(void) hv_store( fi-> frameProperties, "height", 6, newSViv( height), 0);
		if ( fi-> loadExtras) { /* skip data and read info blocks */
			for (pass = 0; pass < number_passes; pass++) {
				int y;
				for (y = 0; y < height; y++) 
					png_read_row( l->png_ptr, NULL, NULL);
			}
			goto READ_END;
		}
		return true;
	}

	/* reading bits */
	CImage( fi-> object)-> create_empty( fi-> object, width, height, bpp);

	EVENT_HEADER_READY(fi);

	/* create buffer for 8 to 4 bpp conversion */
	if ( obd == 2) 
		if ( !( l-> b8_4 = malloc( width))) outcm( width);

	/* prepare buffers for alpha strip */
	if ( bpp == imByte && channels == 2) {
		if ( !( l-> line = malloc( width * 2))) outcm( width * 2);
	} else if (( bpp & imBPP) == 24 && channels == 4) {
		if ( !( l-> line = malloc( width * 4))) outcm( width * 4);
	} else if ( color_type & PNG_COLOR_MASK_ALPHA || 
					channels == 2 || 
					channels == 4) {
		blending = 0;
		icon     = 0;
		png_set_strip_alpha(l-> png_ptr);
		warn("Unknown alpha channel coding scheme (%d %d %d %x)", channels, color_type, obd, bpp); 
	}
	
	if (( color_type & PNG_COLOR_MASK_ALPHA) && icon ) { 
		CIcon( fi-> object)-> set_autoMasking( fi-> object, amNone );
		CIcon( fi-> object)-> set_maskType( fi-> object, imbpp8 );
		alpha = PIcon(fi->object)-> mask;
		if (( bpp & imBPP ) != 24 ) post_blending = true; /* cannot blend 8 bits inplace */
	} else
		alpha = NULL;

	/* cycle through scanlines  */
	for (pass = 0; pass < number_passes; pass++) {
		int y;
		Byte * data = PImage( fi-> object)-> data;
		Byte * a_data = alpha;
		
		EVENT_SCANLINES_RESET(fi);
		data += ( height - 1) * PImage( fi-> object)-> lineSize;
		if ( a_data)
			a_data += ( height - 1) * PIcon( fi-> object)-> maskLine;
		for (y = 0; y < height; y++) {
			if ( a_data ) {
				int i;
				Byte * dst = data, * src = l-> line, *a = a_data;
				png_read_row(l->png_ptr, l-> line, NULL);
				if ( channels == 4 ) {
					if ( blending ) {
						for ( i = 0; i < width; i++) {
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
						for ( i = 0; i < width; i++) {
							*dst++ = *src++;
							*dst++ = *src++;
							*dst++ = *src++;
							*a++   = *src++;
						}
					}
				} else {
					for ( i = 0; i < width; i++) {
						*dst++ = *src++;
						*a++ = *src++;
					}
				}       
			} else {
				if ( obd == 2) { /* convert from 8-bit down to 4-bit */
					png_read_row(l->png_ptr, l-> b8_4, NULL);
					bc_byte_nibble_cr( l-> b8_4, data, width, map_stdcolorref);
				} else
					png_read_row(l->png_ptr, data, NULL);
			}
			data -= PImage( fi-> object)-> lineSize;
			if ( a_data) a_data -= PIcon( fi-> object)-> maskLine;

			EVENT_TOPDOWN_SCANLINES_READY(fi,1);
		}
		EVENT_TOPDOWN_SCANLINES_FINISHED(fi);
	}

	/* adjusting icon mask if possible */
	if ( icon && !alpha && trns_n ) {
		if ( trns_t) {
			if ( blending ) {
				PRGBColor p = PIcon( fi-> object)-> palette;
				int i;
				/* transparency values per pixel table is present */
				for ( i = 0; i < trns_n; i++) {
					p[i]. r = ((uint16_t) p[i]. r * trns_t[i]) >> 8;
					p[i]. g = ((uint16_t) p[i]. g * trns_t[i]) >> 8;
					p[i]. b = ((uint16_t) p[i]. b * trns_t[i]) >> 8;
				}
			}
			PIcon( fi-> object)-> autoMasking = amNone;
		} else {
			PIcon( fi-> object)-> maskIndex = trns_p[0].index;
			PIcon( fi-> object)-> autoMasking = amMaskIndex;
		}
	}
	if ( post_blending )
		CIcon( fi-> object )-> premultiply_alpha( fi-> object, (SV*) NULL );

	/* misc extras  */
READ_END:   
		
	if ( fi-> loadExtras) {
		HV * profile = fi-> frameProperties;
		char * name, *pf;
		int ct, i;
		png_uint_32 pl, py;
		png_int_32 pli, pyi;
		png_textp tx;
		double scx, scy;

		(void)tx; (void)py; (void)pl; (void)pyi; (void)pli; (void)i; (void)ct;
		(void)pf; (void)name; (void)scx; (void)scy;

		png_read_end(l-> png_ptr, l-> info_ptr);

		pset_i( interlaced, interlace_type == PNG_INTERLACE_ADAM7);
#ifdef PNG_INTRAPIXEL_DIFFERENCING
		pset_i( mng_datastream, filter == PNG_INTRAPIXEL_DIFFERENCING);
#endif
	
#ifdef PNG_sRGB_SUPPORTED
		if ( png_get_sRGB( l-> png_ptr, l-> info_ptr, &i)) {
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
		if ( png_get_iCCP( l-> png_ptr, l-> info_ptr, &name, &ct, (png_bytepp)&pf, &pl)) {
			pset_c( iccp_name, name);
			if ( pf) pset_sv_noinc( iccp_profile, newSVpv( pf, pl));
		}
#endif

#ifdef PNG_TEXT_SUPPORTED      
		if ( png_get_text( l-> png_ptr, l-> info_ptr, &tx, &ct)) {
			HV * hash = newHV();
			for ( i = 0; i < ct; i++, tx++) 
				(void) hv_store( hash, tx-> key, strlen( tx-> key), newSVpv( tx-> text, tx-> text_length), 0);
			pset_sv_noinc( text, newRV_noinc(( SV *) hash));
		}
#endif      

#ifdef PNG_oFFs_SUPPORTED      
		if ( png_get_oFFs( l-> png_ptr, l-> info_ptr, &pli, &pyi, &i)) {
			pset_i( offset_x, pli);
			pset_i( offset_y, pyi);
			pset_c( offset_dimension, ( i == PNG_OFFSET_PIXEL) ? "pixel" : "micrometer");
		}
#endif      
	
#ifdef PNG_pHYs_SUPPORTED      
		if ( png_get_pHYs( l-> png_ptr, l-> info_ptr, &pl, &py, &i)) {
			pset_i( resolution_x, pl);
			pset_i( resolution_y, py);
			pset_c( resolution_dimension, ( i == PNG_RESOLUTION_METER) ? "meter" : "unknown");
		}
#endif      
	
#ifdef PNG_sCAL_SUPPORTED      
		if ( png_get_sCAL( l-> png_ptr, l-> info_ptr, &i, &scx, &scy)) {
			pset_f( scale_x, scx);
			pset_f( scale_y, scy);
			pset_c( scale_unit, 
				( i == PNG_SCALE_METER) ? "meter" :
				(( i == PNG_SCALE_RADIAN) ? "radian" : "unknown")
			);
		}
#endif      
	}

	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	if ( l-> b8_4) free( l-> b8_4);
	if ( l-> line) free( l-> line);
	png_destroy_read_struct(&l-> png_ptr, &l-> info_ptr, (png_infopp)NULL);
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
