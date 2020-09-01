/*-
*
* Created by Dmitry Karasik <dmitry@karasik.eu.org> with great help
* of tiff2png.c by Willem van Schaik and Greg Roelofs
*
*/

#include "img.h"
#include "img_conv.h"
#include "Icon.h"
#if defined(_MSC_VER) && _MSC_VER < 1400 && _MSC_VER > 1200
#define HAVE_INT32
#endif
#include <tiff.h>
#include <tiffio.h>
#include <tiffconf.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PHOTOMETRIC_DEPTH
#  define PHOTOMETRIC_DEPTH 32768
#endif


static char * tiffext[] = { "tif", "tiff", nil };
static int    tiffbpp[] = { imbpp24,
			imbpp8, imByte,
			imShort,
			imbpp4, imbpp4 | imGrayScale,
			imbpp1, imbpp1 | imGrayScale,
			0 };
static char * loadOutput[] = {
	"Photometric",
	"BitsPerSample",
	"SamplesPerPixel",
	"PlanarConfig",
	"SampleFormat",
	"Tiled",
	"Faxpect",

	"Artist",
	"CompressionType",
	/* tibtiff can decompress many types; but compress a few, so CompressionType
		is named so to avoid implicit but impossible compression selection */
	"Copyright",
	"DateTime",
	"DocumentName",
	"HostComputer",
	"ImageDescription",
	"Make",
	"Model",
	"PageName",
	"PageNumber",
	"PageNumber2",
	"ResolutionUnit",
	"Software",
	"XPosition",
	"YPosition",
	"XResolution",
	"YResolution",
	nil
};

static char * tifffeatures[] = {
#ifdef COLORIMETRY_SUPPORT
	"Tag-COLORIMETRY",
#endif
#ifdef   YCBCR_SUPPORT
	"Tag-YCBCR",
#endif
#ifdef   CMYK_SUPPORT
	"Tag-CMYK",
#endif
#ifdef   ICC_SUPPORT
	"Tag-ICC",
#endif
#ifdef PHOTOSHOP_SUPPORT
	"Tag-PPHOTOSHOP",
#endif
#ifdef IPTC_SUPPORT
	"Tag-IPTC",
#endif
#ifdef   CCITT_SUPPORT
	"Compression-CCITT",
#endif
#ifdef   PACKBITS_SUPPORT
	"Compression-PACKBITS",
#endif
#ifdef   LZW_SUPPORT
	"Compression-LZW",
#endif
#ifdef   THUNDER_SUPPORT
	"Compression-THUNDER",
#endif
#ifdef   NEXT_SUPPORT
	"Compression-NEXT",
#endif
#ifdef  LOGLUV_SUPPORT
	"Compression-SGILOG",
	"Compression-SGILOG24",
#endif
#ifdef  JPEG_SUPPORT
	"Compression-JPEG",
#endif
	nil
};

typedef struct {
	int tag;
	char * name;
} TagRec;

static TagRec comptable [] = {
{ COMPRESSION_NONE            , "NONE"},
{ COMPRESSION_CCITTRLE        , "CCITTRLE"},
{ COMPRESSION_CCITTFAX3       , "CCITTFAX3"},
{ COMPRESSION_CCITTFAX4       , "CCITTFAX4"},
{ COMPRESSION_LZW             , "LZW"},
{ COMPRESSION_OJPEG           , "OJPEG"},
{ COMPRESSION_JPEG            , "JPEG"},
{ COMPRESSION_NEXT            , "NEXT"},
{ COMPRESSION_CCITTRLEW       , "CCITTRLEW"},
{ COMPRESSION_PACKBITS        , "PACKBITS"},
{ COMPRESSION_THUNDERSCAN     , "THUNDERSCAN"},
{ COMPRESSION_IT8CTPAD        , "IT8CTPAD"},
{ COMPRESSION_IT8LW           , "IT8LW"},
{ COMPRESSION_IT8MP           , "IT8MP"},
{ COMPRESSION_IT8BL           , "IT8BL"},
{ COMPRESSION_PIXARFILM       , "PIXARFILM"},
{ COMPRESSION_PIXARLOG        , "PIXARLOG"},
{ COMPRESSION_DEFLATE         , "DEFLATE"},
{ COMPRESSION_ADOBE_DEFLATE   , "ADOBE_DEFLATE"},
{ COMPRESSION_DCS             , "DCS"},
{ COMPRESSION_JBIG            , "JBIG"},
{ COMPRESSION_SGILOG          , "SGILOG"},
{ COMPRESSION_SGILOG24        , "SGILOG24"},
};

static TagRec pixeltype [] = {
{ SAMPLEFORMAT_UINT           , "unsigned integer"},
{ SAMPLEFORMAT_INT            , "signed integer"},
{ SAMPLEFORMAT_IEEEFP         , "floating point"},
{ SAMPLEFORMAT_VOID           , "untyped data"},
{ SAMPLEFORMAT_COMPLEXINT     , "complex signed int"},
{ SAMPLEFORMAT_COMPLEXIEEEFP  , "complex floating point"},
};

static char * mime[] = {
	"image/tiff",
	NULL
};

#ifndef TIFF_VERSION
#define TIFF_VERSION TIFF_VERSION_CLASSIC
#endif

static ImgCodecInfo codec_info = {
	"TIFF Bitmap",
	"www.libtiff.org",
	TIFF_VERSION, TIFFLIB_VERSION,    /* version */
	tiffext,    /* extension */
	"Tagged Image File Format",  /* file type */
	"TIFF", /* short type */
	tifffeatures,    /* features  */
	"Prima::Image::tiff",     /* module */
	"Prima::Image::tiff",     /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_MULTIFRAME | IMG_LOAD_FROM_STREAM |
	IMG_SAVE_TO_FILE | IMG_SAVE_MULTIFRAME | IMG_SAVE_TO_STREAM,
	tiffbpp, /* save types */
	loadOutput,
	mime
};

#define outcm(dd) snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", (int)dd)
#define outc(x)   strncpy( fi-> errbuf, x, 256)

static char * errbuf = nil;
static Bool err_signal = 0;

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
/*
It appears that PHOTOMETRIC_MINISWHITE should always be inverted (which
makes sense), but if you find a class of TIFFs / a version of libtiff for
which that is *not* the case, try setting InvertMinIsWhite / INVERT_MINISWHITE to 0.
*/
#define INVERT_MINISWHITE 1
	pset_i( InvertMinIsWhite, INVERT_MINISWHITE);
/* Converts 1-bit grayscale with ratio 2:1 into 2-bit grayscale */
	pset_i( Fax, 0);
	return profile;
}

static tsize_t
my_tiff_read( thandle_t h, tdata_t data, tsize_t size)
{
	return req_read( (PImgIORequest) h, size, data);
}

static tsize_t
my_tiff_write( thandle_t h, tdata_t data, tsize_t size)
{
	return req_write( (PImgIORequest) h, size, data);
}

static toff_t
my_tiff_seek( thandle_t h, toff_t offset, int whence)
{
	if ( req_seek( (PImgIORequest) h, offset, whence) < 0)
		return -1;
	return req_tell( (PImgIORequest) h);
}

static int
my_tiff_close( thandle_t h)
{
	return (( PImgIORequest) h)-> flush ?
		req_flush( (PImgIORequest) h) :
		0;
}

static toff_t
my_tiff_size( thandle_t h)
{
	return 0;
}

static int
my_tiff_map( thandle_t h, tdata_t * data, toff_t * offset)
{
	return 0;
}

static void
my_tiff_unmap( thandle_t h, tdata_t data, toff_t offset)
{
}


static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	TIFF * tiff;
	errbuf = fi-> errbuf;
	err_signal = 0;
	if (!( tiff = TIFFClientOpen( "", "r", (thandle_t) fi-> req,
		my_tiff_read, my_tiff_write,
		my_tiff_seek, my_tiff_close, my_tiff_size,
		my_tiff_map, my_tiff_unmap))) {
		req_seek( fi-> req, 0, SEEK_SET);
		return nil;
	}
	fi-> frameCount = TIFFNumberOfDirectories( tiff);
	fi-> stop = true;
	return tiff;
}

static __INLINE__ unsigned long
get_bits( Byte * src, register unsigned int offset, register int length)
{
	register unsigned long accum = 0;

	src += offset / 8;
	offset %= 8;

	if ( offset > 0 ) {
		register Byte x = *src++;
		register Byte bits = 8 - offset;
		x &= (0xff >> offset);
		if ( length < bits ) {
			x >>= bits - length;
			accum <<= length;
		} else {
			accum <<= bits;
		}
		length -= bits;
		accum |= x;
	}

	while ( length > 0 ) {
		register Byte x = *src++;
		if ( length < 8 ) {
			x >>= 8 - length;
			accum <<= length;
		} else {
			accum <<= 8;
		}
		length -= 8;
		accum |= x;
	}

	return accum;
}

static __INLINE__ unsigned long
get_24bits( Byte * src)
{
	return (src[0] << 16) | (src[1] << 8) | src[2];
}

static void
convert_1x8_to_byte( Byte * src, Byte * dest, int bps, int pixels)
{
	/* note -- does not upgrade the range, f.x. 2-bit 0-3 range stays 0-3 in 8-bit too */
	switch ( bps) {
	case 1:
		bc_mono_byte( src, dest, pixels);
		break;
	case 2:
		{
			register Byte mask = 0xC0, shift = 6;
			while ( pixels--) {
				*dest++ = (*src & mask) >> shift;
				if ( shift == 0) {
					mask = 0xC0;
					shift = 6;
					src++;
				} else {
					mask >>= 2;
					shift -= 2;
				}
			}
		}
		break;

	case 4:
		bc_nibble_byte( src, dest, pixels);
		break;

	case 8:
		memcpy( dest, src, pixels);
		break;

	default:
		{
			unsigned int offset = 0;
			while ( pixels-- ) {
				*dest++ = get_bits( src, offset, bps );
				offset += bps;
			}
		}
	}
}

static void
convert_9x16_to_byte( Byte * src, Byte * dest, int bps, int pixels)
{
	if ( bps < 16 ) {
		unsigned int offset = 0;
		unsigned int shift  = bps - 8;
		while ( pixels-- ) {
			*dest++ = get_bits( src, offset, bps ) >> shift;
			offset += bps;
		}
	} else {
		uint16_t * s = ( uint16_t * ) src;
		while ( pixels-- ) *dest++ = *s++ >> 8;
	}
}

static void
convert_9x16_to_short( Byte * src, Short * dest, int bps, int pixels)
{
	if ( bps < 16 ) {
		unsigned int offset = 0;
		unsigned int shift  = 16 - bps;
		while ( pixels-- ) {
			*dest++ = get_bits( src, offset, bps ) << shift;
			offset += bps;
		}
	} else
		memcpy((Byte *) dest, src, pixels * 2);
}

static void
convert_17x32_to_long( Byte * src, Long * dest, int bps, int pixels)
{
	if ( bps == 24 ) {
		while ( pixels-- ) {
			*dest++ = get_24bits( src ) << 8;
			src += 3;
		}
	}
	else if ( bps < 32 ) {
		unsigned int offset = 0;
		unsigned int shift  = 32 - bps;
		while ( pixels-- ) {
			*dest++ = get_bits( src, offset, bps ) << shift;
			offset += bps;
		}
	} else
		memcpy((Byte *) dest, src, pixels * 4);
}

static void
convert_17x32_to_byte( Byte * src, Byte * dest, int bps, int pixels)
{
	if ( bps == 24 ) {
		while ( pixels-- ) {
			*dest++ = get_24bits( src ) >> 16;
			src += 3;
		}
	} else if ( bps < 32 ) {
		unsigned int offset = 0;
		unsigned int shift  = bps - 8;
		while ( pixels-- ) {
			*dest++ = get_bits( src, offset, bps ) >> shift;
			offset += bps;
		}
	} else {
		uint32_t * s = ( uint32_t * ) src;
		while ( pixels-- ) *dest++ = *s++ >> 24;
	}
}

static void
convert_real_to_byte( Byte * src, Byte * dest, int pixels, int source_format)
{
	switch (source_format) {
		case imFloat: {
			float * s = ( float * ) src;
			while ( pixels-- ) *dest++ = *s++ + 0.5;
			break;
		}
		case imDouble: {
			double * s = ( double * ) src;
			while ( pixels-- ) *dest++ = *s++ + 0.5;
			break;
		}
		default:
			croak("panic: tiff.convert_real_to_byte(%d)", source_format);
	}
}

static void
convert_real_to_real( Byte * src, Byte * dest, int pixels, int source_bits)
{
	memcpy( dest, src, pixels * source_bits / 8);
}

static void
scan_convert( Byte * src, Byte * dest, int pixels, int source_bits, int source_format, int target_bytes, int target_format)
{
	Bool is_source_signed_int = 0, is_target_signed_int = 0;

	/* convert floating point pixels either to float/doubles or 8 bits */
	switch ( source_format ) {
	case imFloat:
	case imDouble:
		switch ( target_format ) {
		case imFloat:
		case imDouble:
			convert_real_to_real( src, dest, pixels, source_bits);
			return;
		}

		if (target_bytes == 1)
			convert_real_to_byte( src, dest, pixels, source_format);
		else
			croak("panic: tiff.scan_convert(float,%d bytes)", target_bytes);
		return;
	case imSignedInt:
		is_source_signed_int = 1;
		break;
	}

	if ( target_format == imSignedInt)
		is_target_signed_int = 1;

	/* convert to either 8, 16, or 32 bits */
	if ( source_bits <= 8 && target_bytes == 1)
		convert_1x8_to_byte( src, dest, source_bits, pixels);
	else if ( source_bits >= 9 && source_bits <= 16) {
		switch ( target_bytes ) {
		case 1:
			convert_9x16_to_byte( src, dest, source_bits, pixels);
			break;
		case 2:
			convert_9x16_to_short( src, (Short*) dest, source_bits, pixels);
			break;
		default:
			croak("panic: tiff.scan_convert(%d to %d bits)", source_bits, target_bytes * 8);
		}
	} else if ( source_bits >= 17 && source_bits <= 32) {
		switch ( target_bytes ) {
		case 1:
			convert_17x32_to_byte( src, dest, source_bits, pixels);
			break;
		case 4:
			convert_17x32_to_long( src, (Long*) dest, source_bits, pixels);
			break;
		default:
			croak("panic: tiff.scan_convert(%d to %d bits)", source_bits, target_bytes * 8);
		}
	} else {
		croak("panic: tiff.scan_convert(%d to %d bits)", source_bits, target_bytes * 8);
	}

	/* convert signed to unsigned */
	if ( is_source_signed_int != is_target_signed_int ) {
#if (BYTEORDER!=0x4321) && (BYTEORDER!=0x87654321)
		dest += target_bytes - 1;
#endif
		while ( pixels-- ) {
			if ( *dest & 0x80 )
				*dest &= 0x7f;
			else
				*dest |= 0x80;
			dest += target_bytes;
		}
	}
}



static void
invert_scanline( Byte * src, int source_bits, int type, int pixels)
{
#undef NEGATE
#define NEGATE(Type) { \
	Type *s = (Type*) src; \
	while ( pixels--) { \
		*s = - *s; \
		s++; \
	}}

	switch (type) {
	case imLong:
		NEGATE(Long)
		break;
	case imShort:
		NEGATE(Short)
		break;
	case imFloat:
		NEGATE(float)
		break;
	case imDouble:
		NEGATE(double)
		break;
	default: {
			int sz  = pixels; /* 1 and 4 bits are safe here with full byte */
			register Byte mask = 0xff >> ( 8 - source_bits );
			while ( sz--) {
				*src = (~*src) & mask;
				src++;
			}
		}
	}
}

#if (BYTEORDER==0x4321) || (BYTEORDER==0x87654321)

static void
convert_abgr_to_rgba( Byte * buffer, int quads)
{
	register uint32_t * x = (uint32_t *) buffer;
	while ( quads-- ) {
		register uint32_t f = *x;
		*x++ =
			(f >> 24) |
			((f >> 8) & 0x00FF00) |
			((f << 8) & 0xFF0000) |
			(f << 24)
			;
	}
}

#endif

/* 4-channels is already presplit as if it was RGBA to reject A early */
static void
convert_cmyk_to_rgb( Byte * buffer, int quads)
{
	Byte * blacks = buffer + quads * 3;
	while (quads--) {
		register unsigned int k = 255 - *(blacks++);
		if ( k > 0 ) {
			buffer[0] = (((255 - buffer[0]) * k) / 255) & 0xff;
			buffer[1] = (((255 - buffer[1]) * k) / 255) & 0xff;
			buffer[2] = (((255 - buffer[2]) * k) / 255) & 0xff;
		} else {
			buffer[0] = 255 - buffer[0];
			buffer[1] = 255 - buffer[1];
			buffer[2] = 255 - buffer[2];
		}
		buffer += 3;
	}
}

static Bool
read_source_format( PImgLoadFileInstance fi, int source_bits, int * source_format)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;
	uint16_t sample_format;
	int i;

	*source_format = 0;

	if ( !TIFFGetField( tiff, TIFFTAG_SAMPLEFORMAT, &sample_format))
		return true;

	for ( i = 0; i < sizeof(pixeltype) / sizeof(TagRec); i++) {
		if ( pixeltype[i].tag == sample_format) {
			if ( fi-> loadExtras)
				pset_c( SampleFormat, pixeltype[i].name);
			break;
		}
	}

	switch ( sample_format) {
	case SAMPLEFORMAT_COMPLEXINT:
	case SAMPLEFORMAT_COMPLEXIEEEFP:
		sprintf( fi-> errbuf, "Unexpected SAMPLEFORMAT: %s", pixeltype[i].name);
		return false;
	case SAMPLEFORMAT_INT:
		*source_format = imSignedInt;
	case SAMPLEFORMAT_UINT:
	case SAMPLEFORMAT_VOID: /* seems valid */
		break;
	case SAMPLEFORMAT_IEEEFP:
		switch (source_bits) {
		case sizeof(float)*8:
			*source_format = imFloat;
			break;
		case sizeof(double)*8:
			*source_format = imDouble;
			break;
		default:
			sprintf( fi-> errbuf,
				"SAMPLEFORMAT in file is %d bits, while supported floats are %d and %d bits, can't convert",
				source_bits, (int)sizeof(float)*8, (int)sizeof(double)*8);
			return false;
		}
		break;
	default:
		sprintf( fi-> errbuf, "Unexpected SAMPLEFORMAT: %d", sample_format);
		return false;
	}

	return true;
}

static Bool
read_source_bits( PImgLoadFileInstance fi, int * source_bits)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;
	uint16_t bits;

	if ( !TIFFGetField( tiff, TIFFTAG_BITSPERSAMPLE, &bits)) {
		*source_bits = 1;
		return true;
	}

	if ( bits > 64 ) {
		sprintf( fi-> errbuf, "Unexpected BITSPERSAMPLE: %d", bits);
		return false;
	}

	if ( fi-> loadExtras) pset_i( BitsPerSample, bits);

	*source_bits = bits;

	return true;
}

static Bool
read_source_sample_per_pixel( PImgLoadFileInstance fi, int * source_spp)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;
	uint16_t spp;

	if ( !TIFFGetField( tiff, TIFFTAG_SAMPLESPERPIXEL, &spp)) {
		*source_spp = 1;
		return true;
	}

	if ( spp < 1 || spp > 4) {
		sprintf( fi-> errbuf, "Unexpected SAMPLESPERPIXEL: %d", spp);
		return false;
	}

	if ( fi-> loadExtras) pset_i( SamplesPerPixel, spp);

	*source_spp = spp;

	return true;
}

static Bool
read_source_planar_config( PImgLoadFileInstance fi, Bool * source_is_planar)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;
	uint16_t pc;

	if ( !TIFFGetField( tiff, TIFFTAG_PLANARCONFIG, &pc)) {
		*source_is_planar = true;
		return true;
	}

	switch ( pc) {
	case PLANARCONFIG_CONTIG:
		if ( fi-> loadExtras) pset_c( PlanarConfig, "contiguous");
		*source_is_planar = true;
		break;
	case PLANARCONFIG_SEPARATE:
		if ( fi-> loadExtras) pset_c( PlanarConfig, "separate");
		*source_is_planar = false;
		break;
	default:
		sprintf( fi-> errbuf, "Unexpected PLANARCONFIG: %d", pc);
		return false;
	}

	return true;
}

static Bool
read_source_resolution( PImgLoadFileInstance fi, float * x, float * y)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;

	if ( !TIFFGetField( tiff, TIFFTAG_XRESOLUTION, x))
		*x = 0.0;
	else
		if ( fi-> loadExtras)
			pset_f( XResolution, *x);

	if ( !TIFFGetField( tiff, TIFFTAG_YRESOLUTION, y))
		*y = 0.0;
	else
		if ( fi-> loadExtras)
			pset_f( YResolution, *y);

	return true;
}

static Bool
read_other_tags( PImgLoadFileInstance fi)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;
	uint16_t u16, u16_2;
	char * ch;
	float n;

	if ( !fi-> loadExtras) return true;

	if ( !TIFFGetField( tiff, TIFFTAG_RESOLUTIONUNIT, &u16))
		u16 = RESUNIT_INCH;  /* default (see libtiff tif_dir.c) */
	else
		pset_c( ResolutionUnit,
			( u16 == RESUNIT_INCH      ) ? "inch" :
			( u16 == RESUNIT_CENTIMETER  ? "centimeter" :
													"none"
	));

	if ( TIFFGetField( tiff, TIFFTAG_ARTIST, &ch))
		pset_c( Artist, ch);

	if ( TIFFGetField( tiff, TIFFTAG_COMPRESSION, &u16)) {
		int i, found = 0;
		for ( i = 0; i < sizeof(comptable) / sizeof(TagRec); i++) {
			if ( comptable[i].tag == u16) {
				pset_c( CompressionType, comptable[i].name);
				found = 1;
				break;
			}
		}
		if ( !found) pset_i( CompressionType, u16);
	}

	if ( TIFFGetField( tiff, TIFFTAG_COPYRIGHT, &ch))
		pset_c( Copyright, ch);

	if ( TIFFGetField( tiff, TIFFTAG_DATETIME, &ch))
		pset_c( DateTime, ch);

	if ( TIFFGetField( tiff, TIFFTAG_DOCUMENTNAME, &ch))
		pset_c( DocumentName, ch);

	if ( TIFFGetField( tiff, TIFFTAG_HOSTCOMPUTER, &ch))
		pset_c( HostComputer, ch);

	if ( TIFFGetField( tiff, TIFFTAG_IMAGEDESCRIPTION, &ch))
		pset_c( ImageDescription, ch);

	if ( TIFFGetField( tiff, TIFFTAG_MAKE, &ch))
		pset_c( Make, ch);

	if ( TIFFGetField( tiff, TIFFTAG_MODEL, &ch))
		pset_c( Model, ch);

	if ( TIFFGetField( tiff, TIFFTAG_PAGENAME, &ch))
		pset_c( PageName, ch);

	if ( TIFFGetField( tiff, TIFFTAG_SOFTWARE, &ch))
		pset_c( Software, ch);

	if ( TIFFGetField( tiff, TIFFTAG_XPOSITION, &n))
		pset_f( XPosition, n);

	if ( TIFFGetField( tiff, TIFFTAG_YPOSITION, &n))
		pset_f( YPosition, n);

	if ( TIFFGetField( tiff, TIFFTAG_PAGENUMBER, &u16, &u16_2)) {
		pset_i( PageNumber, u16);
		pset_i( PageNumber2, u16_2);
	}

	return true;
}

/* reads palette, fixes colors if necessary */
static Bool
read_palette( PImgLoadFileInstance fi, int source_bits, int * colors, RGBColor * palette)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	RGBColor *p, last;
	int x, entry, steps;
	unsigned short *redcolormap, *greencolormap, *bluecolormap;

	if ( !TIFFGetField( tiff, TIFFTAG_COLORMAP, &redcolormap, &greencolormap, &bluecolormap)) {
		outc("Cannot query COLORMAP tag");
		return false;
	}

	p = palette;
	steps = ( source_bits <= 8 ) ? 1 : ( 1 << ( source_bits - 8 ));
	for ( x = entry = 0; x < *colors; x++, p++, entry += steps) {
		p-> r = redcolormap[entry]   >> 8;
		p-> g = greencolormap[entry] >> 8;
		p-> b = bluecolormap[entry]  >> 8;
	}

	/* optimize palette, since tiff palette must be **2 only */
	p = palette + *colors - 1;
	last = *p--;
	for ( x = *colors - 1; x > 0; x--, p--) {
		if ( p->r == last.r && p->g == last.g && p->b == last.b) {
			(*colors)--;
		} else {
			/* see if this color is also present somewhere */
			p = palette;
			for ( x = 0; x < *colors - 1; x++) {
				if ( p->r == last.r && p->g == last.g && p->b == last.b) {
					(*colors)--;
					break;
				}
			}
			break;
		}
	}

	return true;
}

static void
build_grayscale_palette( int source_bits, RGBColor * palette)
{
	int i, colors;
	float accum, step;

	if ( source_bits > 8 ) croak("panic: tiff.build_gray_palette(%d)", source_bits);

	colors     = 1 << source_bits;
	step       = 255.0 / (colors - 1);
	for ( i = 0, accum = 0; i < colors; i++, accum += step, palette++) {
		int intensity = accum + 0.5;
		palette-> r = palette-> g = palette-> b = intensity;
	}
}


static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	TIFF * tiff = ( TIFF *) fi-> instance;
	HV * profile = fi-> frameProperties;
	PIcon i = ( PIcon) fi-> object;
	char * photometric_descr = nil;
	unsigned short photometric, comp_method;
	int x, y, w, h, icon, tiled, rgba_striped = 0, cmyk = 0,
		InvertMinIsWhite = INVERT_MINISWHITE, faxpect = 0, full_image = 0, full_rgba_image = 0,
		read_failure = 0;
	int source_bits, source_format, source_samples, mid_bytes, mid_format, target_type, target_samples;
	Bool source_is_planar, build_gray_palette = 0;
	float xres, yres;
	Byte *tiffstrip, *tiffline, *tifftile, *primaline, *primamask = nil;
	size_t stripsz, linesz, tilesz = 0L;
	uint32 tile_width, tile_height, num_tilesX = 0L, rowsperstrip;
	Byte bw_colorref[256];

	errbuf = fi-> errbuf;
	err_signal = 0;

	if ( !TIFFSetDirectory( tiff, (tdir_t) fi-> frame)) {
		outc( "Frame index out of range");
		return false;
	}

	if ( !TIFFGetField( tiff, TIFFTAG_PHOTOMETRIC, &photometric)) {
		outc("Cannot query PHOTOMETRIC tag");
		return false;
	}
	if ( !TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w)) {
		outc("Cannot query IMAGEWIDTH tag");
		return false;
	}
	if ( !TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h)) {
		outc("Cannot query IMAGELENGTH tag");
		return false;
	}

	if (!read_source_bits( fi, &source_bits))
		return false;
	if (!read_source_format( fi, source_bits, &source_format))
		return false;
	if (!read_source_sample_per_pixel( fi, &source_samples))
		return false;
	if (!read_source_planar_config( fi, &source_is_planar))
		return false;
	if (!read_source_resolution( fi, &xres, &yres))
		return false;
	if (!read_other_tags(fi))
		return false;

	tiled = TIFFIsTiled(tiff);
	if ( fi-> loadExtras) pset_i( Tiled, tiled);

	/* calculate prima image bpp and color count */
	target_samples = 0;
	switch ( photometric) {
	case PHOTOMETRIC_MINISWHITE:
	case PHOTOMETRIC_MINISBLACK:
		if ( source_format & imRealNumber ) {
			target_type = source_format;
		} else {
			if ( source_bits > 16) target_type = imLong; else
			if ( source_bits >  8) target_type = imShort; else
			if ( source_bits == 8) target_type = imByte; else
			if ( source_bits >  4) target_type = imbpp8; else
			if ( source_bits == 4) target_type = imbpp4 | imGrayScale; else
			if ( source_bits >  1) target_type = imbpp4; else
										target_type = imbpp1 | imGrayScale;

			/* build our own grayscale palette */
			if ( !( target_type & imGrayScale)) build_gray_palette = 1;
		}
		photometric_descr = ( photometric == PHOTOMETRIC_MINISWHITE) ? "MinIsWhite" : "MinIsBlack";
		break;
	case PHOTOMETRIC_PALETTE:
		if ( source_bits > 4) target_type = imbpp8; else
		if ( source_bits > 1) target_type = imbpp4; else
		                      target_type = imbpp1;
		photometric_descr = "Palette";
		break;
#ifdef JPEG_SUPPORT
	case PHOTOMETRIC_YCBCR:
		if ( !TIFFGetField( tiff, TIFFTAG_COMPRESSION, &comp_method)) {
			outc("Cannot query COMPRESSION");
			return false;
		}
		photometric_descr = "YCbCr";
		photometric = PHOTOMETRIC_RGB;
		if ( comp_method == COMPRESSION_JPEG ) {
			/* can rely on libjpeg to convert to RGB */
			TIFFSetField( tiff, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
			source_samples = 3;
		} else {
			full_rgba_image = 1;
		}
		/* fall thru... */
#endif
	case PHOTOMETRIC_RGB:
		if ( !photometric_descr) photometric_descr = "RGB";
		target_type = imbpp24;
		break;
	case PHOTOMETRIC_LOGL:
	case PHOTOMETRIC_LOGLUV:
		if ( !TIFFGetField( tiff, TIFFTAG_COMPRESSION, &comp_method)) {
			outc("Cannot query COMPRESSION tag");
			return false;
		}
		if (comp_method != COMPRESSION_SGILOG && comp_method != COMPRESSION_SGILOG24) {
			sprintf( fi-> errbuf, "Don't know how to handle photometric LOGL%s with" \
			" compression %d (not SGILOG)",
			photometric == PHOTOMETRIC_LOGLUV ? "UV" : "",
			comp_method);
			return false;
		}
		/* rely on library to convert to RGB/greyscale */
		if (source_bits > 8 && photometric == PHOTOMETRIC_LOGL) {
			/* SGILOGDATAFMT_16BIT converts to 16-bit short */
			TIFFSetField(tiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_16BIT);
			source_bits = 16;
		} else {
			/* SGILOGDATAFMT_8BIT converts to normal grayscale or RGB format.
				v3.5.7 handles 16-bit LOGLUV incorrectly, so do 8bit also here */
			TIFFSetField(tiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
			source_bits = 8;
		}

		if ( source_format == imSignedInt ) /* SGILODATAFMT are reported as unsigned */
			source_format = 0;

		if (photometric == PHOTOMETRIC_LOGL) {
			photometric = PHOTOMETRIC_MINISBLACK;
			photometric_descr = "LogL";
			target_type = ( source_bits > 8) ? imShort : imByte;
		} else {
			photometric = PHOTOMETRIC_RGB;
			target_type = imbpp24;
			photometric_descr = "LogLUV";
		}
		break;
#ifdef JPEG_SUPPORT
	case PHOTOMETRIC_SEPARATED:
		target_type = imbpp24;
		source_samples = 4;
		cmyk = 1;
		target_samples = 3;
		photometric_descr = "Separated";
		photometric = PHOTOMETRIC_RGB;
		break;
#endif
	default:
		/* fallback, to RGBA strips */
		target_type = imbpp24;
		source_samples = 4;
		rgba_striped = 1;
		photometric_descr =
		photometric == PHOTOMETRIC_MASK?      "MASK" :
		photometric == PHOTOMETRIC_CIELAB?    "CIELAB" :
		photometric == PHOTOMETRIC_DEPTH?     "DEPTH" :
		photometric == PHOTOMETRIC_SEPARATED? "Separated" :
		photometric == PHOTOMETRIC_YCBCR?     "YCbCr" :
		                                       "unknown";
		photometric = PHOTOMETRIC_RGB;
		break;
	}

	/* CMYK -> RGB */
	if ( target_samples == 0 ) target_samples = source_samples;

	if ( fi-> loadExtras)
		pset_c( Photometric, photometric_descr);

	/* based on target_type, decide mid_type: it is always 8-bit aligned */
	switch (target_type) {
	case imbpp24:
		mid_bytes  = 1;
		mid_format = 0;
		break;
	case imShort:
		mid_bytes  = 2;
		mid_format = imSignedInt;
		break;
	case imLong:
		mid_bytes  = 4;
		mid_format = imSignedInt;
		break;
	case imFloat:
	case imDouble:
		mid_bytes  = target_type & imBPP;
		mid_format = target_type;
		break;
	default: {
		int bits   = target_type & imBPP;
		mid_bytes  = (bits / 8) + ((bits % 8) ? 1 : 0);
		mid_format = 0;
	}}

	/* check source_bits and source_samples combinations - 3 and 4 samples for RGB, 1 and 2 for the others */
	if
		(
			(( target_type == imbpp24) && ( source_samples != 3 && source_samples != 4))
			||
			(( target_type != imbpp24) && ( source_samples != 1 && source_samples != 2))
		) {
		sprintf( fi-> errbuf, "Cannot handle combination SAMPLESPERPIXEL=%d, BITSPERSAMPLE=%d", source_samples, source_bits);
		return false;
	}

	/* Also check source_bits (tiff pixel format) and target_type (wanted prima format) as an extra assertion measure */
	if (( target_type & imRealNumber ) == 0) {
		switch ( target_type & imBPP ) {
		case imbpp1:
			if ( source_bits == 1 ) goto VALID_COMBINATION;
		case imbpp4:
			if ( source_bits <= 4 && source_bits >= 2)   goto VALID_COMBINATION;
		case imbpp8:
			if ( source_bits <= 8 && source_bits >= 5)   goto VALID_COMBINATION;
		case imbpp16:
			if ( source_bits <= 16 && source_bits >= 9 ) goto VALID_COMBINATION;
		case imbpp24:
			goto VALID_COMBINATION;
		case imbpp32:
			if ( source_bits <= 32 && source_bits >= 17 ) goto VALID_COMBINATION;
		}
	} else {
		if ( target_type == ( source_bits | source_format )) goto VALID_COMBINATION;
	}
	sprintf( fi-> errbuf, "Cannot handle combination PHOTOMETRIC=%s, BITSPERSAMPLE=%d", photometric_descr, source_bits);
	return false;
VALID_COMBINATION:


	/* check load options */
	{
		dPROFILE;
		HV * profile = fi-> profile;
		if ( pexist( InvertMinIsWhite)) InvertMinIsWhite = pget_i( InvertMinIsWhite);

		/* check fax option applicability */
		if ( source_bits == 1 &&
			( photometric == PHOTOMETRIC_MINISWHITE || photometric == PHOTOMETRIC_MINISBLACK) &&
			xres > 0 && yres > 0 &&
			xres / yres > 1.9 && xres / yres < 2.1
		) {
			comp_method = 0;
			TIFFGetField( tiff, TIFFTAG_COMPRESSION, &comp_method);
			if (
				( comp_method == COMPRESSION_CCITTFAX3 || comp_method == COMPRESSION_CCITTFAX4) &&
				( !pexist(Fax) || pget_i(Fax) )
			) {
				xres /= 2;
				target_type = imbpp4;
				w /= 2;
				source_bits = 2;
				faxpect = 1;
				build_gray_palette = 1;
			}
		}
	}

	if ( faxpect) pset_i( Faxpect, 1);

	/* done prerequisite tiff parsing, leave early if we can */
	if ( fi-> noImageData) {
		CImage( fi-> object)-> create_empty( fi-> object, 1, 1, target_type);
		pset_i( width,  w);
		pset_i( height, h);
	} else
		CImage( fi-> object)-> create_empty( fi-> object, w, h, target_type);
	EVENT_HEADER_READY(fi);

	/* check if palette available */
	i-> palSize = (source_bits < 8) ? (1 << source_bits) : 256;
	if ( photometric == PHOTOMETRIC_PALETTE) {
		if ( !read_palette( fi, source_bits, &i-> palSize, i-> palette)) return false;
	} else if ( build_gray_palette) {
		build_grayscale_palette( source_bits, i-> palette);
	}

	/* leave early */
	if ( fi-> noImageData) return true;

	icon = kind_of( fi-> object, CIcon);
	if ( icon) i-> autoMasking = amNone;

	/* allocate space for one line (or row of tiles) of TIFF image */
	tiffline = tifftile = tiffstrip = nil;
	linesz = TIFFScanlineSize(tiff);

	if (tiled) {
		int z;
		if ( !TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tile_width)) {
			outc("Cannot query TILEWIDTH tag");
			return false;
		}
		if ( tile_width < 1) {
			sprintf( fi-> errbuf, "Invalid TILEWIDTH=%ld", (long)tile_width);
			return false;
		}
		if ( !TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tile_height)) {
			outc("Cannot query TILELENGTH tag");
			return false;
		}
		if ( tile_height < 1) {
			sprintf( fi-> errbuf, "Invalid TILELENGTH=%ld", (long)tile_height);
			return false;
		}
		num_tilesX = (w + tile_width - 1) / tile_width;
		tilesz = TIFFTileSize(tiff);
		/* check if linesz is big enough */
		z = tilesz / tile_height * num_tilesX;
		if ( linesz < z) linesz = z;

		rowsperstrip = 1;
	} else {
		tile_width  = w;
		tile_height = 1;
		num_tilesX  = 1;
		tilesz      = linesz;
		if ( rgba_striped) {
			if( !TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &rowsperstrip) ) {
				outc("Cannot query ROWSPERSTRIP tag");
				return false;
			}
		} else
			rowsperstrip = 1;

		if ( !source_is_planar && source_samples > 1 ) {
			/* need to read full image because LZW can't seek between planes */
			full_image  = 1;
		}
	}

	if ( full_image || full_rgba_image)
		tile_height = h;

	if ( full_rgba_image ) {
		mid_bytes = 1;
		source_samples = 4;
	}

	/* setup two buffers, both twofold size for byte and intrapixel conversion */
	stripsz = mid_bytes * rowsperstrip * tile_height * w * source_samples;
	if ( stripsz < linesz) stripsz = linesz; /* our size should be really enough, */
	if ( stripsz < tilesz) stripsz = tilesz; /* but just to be extra paranoid */

	if ( !( tifftile = (Byte*) malloc( stripsz * 2 * 2))) {
		outcm( stripsz * 2 * 2);
		return false;
	}
	tiffstrip = tifftile + stripsz * 2;

	tiffline = tiffstrip; /* just set the line to the top of the strip.
	                         * we'll move it through below. */

	/* printf("w:%d, source_bits:%d, source_samples:%d, planar:%d, tile_height:%d, strip_sz:%d, target_type:%d\n", w, source_bits, spp, source_is_planar, tile_height, stripsz, taregt_format); */
	/* setting up destination pointers */
	primaline = i-> data + ( h - 1) * i-> lineSize;
	if ( icon) {
		primamask = i-> mask + ( h - 1) * i-> maskLine;
		/* create colorref for alpha downsampling */
		for ( x = 0;   x < 128; x++) bw_colorref[x] = 1;
		for ( x = 128; x < 256; x++) bw_colorref[x] = 0;
	}

	for ( y = 0; y < h; y++, primaline -= i-> lineSize) {
		/* read from file - tiled and not tiled */
		if ( tiled) {
			int col;
			/* Is it time for a new strip? */
			if (( y % tile_height) == 0) {

				if ( read_failure) goto END_LOOP; /* process lines from the last tile, and then fail */

				for (col = 0; col < num_tilesX; col++) {
					Byte *dest, *src;
					int r, dd, sd, rows, cols;
					int tileno = col+(y/tile_height)*num_tilesX;
					/* read the tile into the array */
					Bool ok = rgba_striped ?
						TIFFReadRGBATile( tiff, col * tile_width, y, (void*) tifftile) :
						( TIFFReadEncodedTile(tiff, tileno, tifftile, tilesz) >= 0 );
					if (!ok) {
						if ( !( errbuf && errbuf[0]))
						sprintf( fi-> errbuf, "Error reading tile");
						read_failure = 1;
					}

					/* copy this tile into the row buffer */
					dest = tiffstrip + stripsz + col * mid_bytes * source_samples * tile_width;
					rows = ((y + tile_height) > h) ? h - y : tile_height;
					cols = (col == num_tilesX - 1) ? w - col * tile_width : tile_width;
					dd   = w * source_samples;
					if ( rgba_striped) {
						/* RGBATiles are reversed */
						sd   = - ((int)tile_width * source_samples);
						src  = tifftile - sd * (tile_height - 1);
					} else {
						sd   = tilesz / tile_height;
						src  = tifftile;
					}

					for (r = 0; r < rows; r++, src += sd, dest += dd)
						scan_convert( src, dest, cols * source_samples, source_bits, source_format, mid_bytes, mid_format);
					if ( read_failure) break;
				}
				tiffline = tiffstrip; /* set tileline to top of strip */
			} else
				tiffline = tiffstrip + (y % tile_height) * w * source_samples;
		} else if ( rgba_striped) {
			/* Is it time for a new strip? */
			if (( y % rowsperstrip) == 0) {
				Byte *dest, *src;
				int r, rows, dd, sd;

				if ( read_failure) goto END_LOOP; /* process lines from the last stripe, and then fail */

				if ( !TIFFReadRGBAStrip( tiff, y, (void*) tifftile)) {
					if ( !( errbuf && errbuf[0]))
					sprintf( fi-> errbuf, "Error reading scanline %d", y);
					read_failure = 1;
				}
				rows = ((y + rowsperstrip) > h) ? h - y : rowsperstrip;
				dest = tiffstrip + stripsz;
				dd   = sd = source_samples * w;
				src  = tifftile + sd * (rows - 1);

				/* RGBAStrips are reversed */
				for (r = 0; r < rows; r++, src -= sd, dest += dd)
					scan_convert( src, dest, sd, source_bits, source_format, mid_bytes, mid_format);
				tiffline = tiffstrip; /* set tileline to top of strip */
			} else
				tiffline = tiffstrip + (y % rowsperstrip) * source_samples * w;
		} else if ( full_image) {
			/* read whole file, once; make interleaved scanlines */
			if ( y == 0) {
				int y, s, line_width = w * mid_bytes, skip_width = line_width * source_samples;
				Byte * d0 = tiffline + stripsz;
				for ( s = 0; s < source_samples; s++, d0 += line_width) {
					Byte * d = d0;
					for ( y = 0; y < h; y++, d += skip_width) {
						if ( TIFFReadScanline( tiff, tiffline, y, (tsample_t) s) < 0) {
							if ( !( errbuf && errbuf[0]))
								sprintf( fi-> errbuf, "Error reading scanline %d:%d", s, y);
							read_failure = 1;
						}
						scan_convert( tiffline, d, w, source_bits, source_format, mid_bytes, mid_format);
						if ( read_failure ) break;
					}
					if ( read_failure ) break;
				}
			} else {
				/* just advance the pointer */
				tiffline += w * mid_bytes * source_samples;
			}
		} else if ( full_rgba_image) {
			/* read whole file */
			if ( y == 0) {
				if ( !TIFFReadRGBAImageOriented(tiff, w, h, (uint32*) tiffline, 0, ORIENTATION_BOTLEFT)) {
					if ( !( errbuf && errbuf[0]))
					sprintf( fi-> errbuf, "Error reading image");
					read_failure = 1;
				}
			} else {
				/* just advance the pointer */
				tiffline += w * 4;
			}
			scan_convert( tiffline, tiffline + stripsz, w * 4, source_bits, source_format, mid_bytes, mid_format);
		} else {
			int s = 0, reads = source_is_planar ? 1 : source_samples;
			int dw = w * ( source_is_planar ? source_samples : 1);
			Byte * d = tiffline + stripsz;
			for ( s = 0; s < reads; s++, d += w * mid_bytes) {
				if ( TIFFReadScanline( tiff, tiffline, y, (tsample_t) s) < 0) {
					if ( !( errbuf && errbuf[0]))
					sprintf( fi-> errbuf, "Error reading scanline %d", y);
					read_failure = 1;
				}
				scan_convert( tiffline, d, dw, source_bits, source_format, mid_bytes, mid_format);
				if ( read_failure) goto END_LOOP;
			}
		}
	END_LOOP:

#if (BYTEORDER==0x4321) || (BYTEORDER==0x87654321)
		if ( full_rgba_image || rgba_striped )
			convert_abgr_to_rgba( tiffline + stripsz, w);
#endif

		/* convert intrapixel layout into planar layout to extract alpha in separate space  */
		{
			Byte * dst0 = tiffline, *dst1;
			Byte * src0 = tiffline + stripsz, *src1, *src2;
			register Byte byte_counter = mid_bytes;
			x = w;
			switch ((source_is_planar ? 10 : 20) + source_samples) {
			case 12:
				dst1 = dst0 + w * byte_counter;
				while ( x--) {
					byte_counter = mid_bytes;
					while ( byte_counter--) *dst0++ = *src0++;
					byte_counter = mid_bytes;
					while ( byte_counter--) *dst1++ = *src0++;
				}
				break;
			case 14:
				dst1 = dst0 + 3 * byte_counter * w;
				while ( x--) {
					byte_counter = mid_bytes;
					while ( byte_counter--) {
						*dst0++ = *src0++;
						*dst0++ = *src0++;
						*dst0++ = *src0++;
					}
					byte_counter = mid_bytes;
					while ( byte_counter--)
						*dst1++ = *src0++;
				}
				break;
			case 24:
				/* copy alpha, the 4th channel */
				memcpy( dst0 + w * 3 * byte_counter, src0 + w * 3 * byte_counter, w * byte_counter);
			case 23:
				src1 = src0 + w * byte_counter;
				src2 = src1 + w * byte_counter;
				while ( x--) {
					byte_counter = mid_bytes;
					while ( byte_counter--) *dst0++ = *src0++;
					byte_counter = mid_bytes;
					while ( byte_counter--) *dst0++ = *src1++;
					byte_counter = mid_bytes;
					while ( byte_counter--) *dst0++ = *src2++;
				}
				break;
			default:
				memcpy( dst0, src0, w * source_samples * mid_bytes);
			}
		}

		/* upgrade 8-bit data, if these were used for RGB from 1-to-7 bit source */
		if ( source_bits < 8 && target_type == imbpp24 ) {
			int shift = 8 - source_bits, bytes = w * 3;
			Byte * x = tiffline;
			while ( bytes-- ) *x++ <<= shift;
		}

		/* 4-channels is already presplit as if it was RGBA to reject A early */
		if ( cmyk )
			convert_cmyk_to_rgb( tiffline, w);

		/* invert data, if any */
		if ( InvertMinIsWhite && photometric == PHOTOMETRIC_MINISWHITE)
			invert_scanline( tiffline, source_bits, target_type, w * source_samples);

		/* copy data into image */
		switch ( target_type) {
		case imbpp1: case imbpp1 | imGrayScale:
			bc_byte_mono_cr( tiffline, primaline, w, map_stdcolorref);
			break;
		case imbpp4: case imbpp4 | imGrayScale:
			bc_byte_nibble_cr( tiffline, primaline, w, map_stdcolorref);
			break;
		case imbpp8: case imbpp8 | imGrayScale:
			memcpy( primaline, tiffline, w);
			break;
		case imRGB:
			cm_reverse_palette(( RGBColor*) tiffline, ( RGBColor*) primaline, w);
			break;
		case imShort:
		case imLong:
		case imFloat:
		case imDouble:
			memcpy( primaline, tiffline, w * mid_bytes);
			break;
		}

		/* do alpha channel */
		if ( icon && ( target_samples == 2 || target_samples == 4)) {
			Byte * alpha = tiffline + w * ( target_samples - 1 ) * mid_bytes;
			bc_byte_mono_cr( alpha, primamask, w, bw_colorref);
			primamask -= i-> maskLine;
		}
		EVENT_TOPDOWN_SCANLINES_READY(fi,1);

		if ( read_failure && !full_image) break;
	}

	/* finalize */
	free( tifftile);

	if ( read_failure) {
		if ( fi-> noIncomplete) return false;

		/* it's not incomplete, it's a real libtiff error camouflaged inside */
		if ( y == 0 ) return false;
	} else {
		EVENT_TOPDOWN_SCANLINES_FINISHED(fi);
	}

	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	errbuf = fi-> errbuf;
	err_signal = 0;
	TIFFClose(( TIFF*) fi-> instance);
	errbuf = nil;
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_c( Software, "Prima");
	pset_c( Artist, "");
	pset_c( Copyright, "");
	pset_c( Compression, "NONE");
	pset_c( DateTime, "");
	pset_c( DocumentName, "");
	pset_c( HostComputer, "");
	pset_c( ImageDescription, "");
	pset_c( Make, "");
	pset_c( Model, "");
	pset_c( PageName, "");
	pset_i( PageNumber, 1);
	pset_i( PageNumber2, 1);
	pset_c( ResolutionUnit, "none");
	pset_i( XPosition, 0);
	pset_i( YPosition, 0);
	pset_i( XResolution, 1200);
	pset_i( YResolution, 1200);

	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	TIFF * tiff;
	errbuf = fi-> errbuf;
	err_signal = 0;
	if (!( tiff = TIFFClientOpen( "", "w", (thandle_t) fi-> req,
		my_tiff_read, my_tiff_write,
		my_tiff_seek, my_tiff_close, my_tiff_size,
		my_tiff_map, my_tiff_unmap)))
		return nil;
	return tiff;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	PIcon i = ( PIcon) fi-> object;
	TIFF * tiff = ( TIFF*) fi-> instance;
	Bool icon = kind_of( fi-> object, CIcon);
	int x, y;
	HV * profile = fi-> objectExtras;
	uint16 u16;

	errbuf = fi-> errbuf;
	err_signal = 0;

	TIFFSetField( tiff, TIFFTAG_IMAGEWIDTH, i-> w);
	TIFFSetField( tiff, TIFFTAG_IMAGELENGTH, i-> h);

	u16 = COMPRESSION_NONE;
	if ( pexist( Compression)) {
		int found = 0;
		char * c = pget_c( Compression);
		for ( x = 0; x < sizeof( comptable) / sizeof( TagRec); x++) {
			if ( strcmp( comptable[x]. name, c) == 0) {
				u16 = comptable[x]. tag;
				found = 1;
			}
		}
		if ( !found) {
			snprintf( fi-> errbuf, 256, "Invalid Compression '%s'", c);
			return false;
		}
	}
	TIFFSetField(tiff, TIFFTAG_COMPRESSION, u16);
	if (u16 == COMPRESSION_CCITTFAX3)
		TIFFSetField(tiff, TIFFTAG_GROUP3OPTIONS, GROUP3OPT_2DENCODING + GROUP3OPT_FILLBITS);

	u16 = RESUNIT_NONE;
	if ( pexist( ResolutionUnit)) {
		char * c = pget_c( ResolutionUnit);
		if ( stricmp( c, "inch") == 0) u16 = RESUNIT_INCH; else
		if ( stricmp( c, "centimeter") == 0) u16 = RESUNIT_CENTIMETER; else
		if ( stricmp( c, "none") == 0) u16 = RESUNIT_NONE; else {
			snprintf( fi-> errbuf, 256, "Invalid Compression '%s'", c);
			return false;
		}
	}
	TIFFSetField( tiff, TIFFTAG_RESOLUTIONUNIT, u16);

	if ( pexist( Artist))
		TIFFSetField( tiff, TIFFTAG_ARTIST, pget_c( Artist));
	if ( pexist( Copyright))
		TIFFSetField( tiff, TIFFTAG_COPYRIGHT, pget_c( Copyright));
	if ( pexist( DateTime))
		TIFFSetField( tiff, TIFFTAG_DATETIME, pget_c( DateTime));
	if ( pexist( DocumentName))
		TIFFSetField( tiff, TIFFTAG_DOCUMENTNAME, pget_c( DocumentName));
	if ( pexist( HostComputer))
		TIFFSetField( tiff, TIFFTAG_HOSTCOMPUTER, pget_c( HostComputer));
	if ( pexist( ImageDescription))
		TIFFSetField( tiff, TIFFTAG_IMAGEDESCRIPTION, pget_c( ImageDescription));
	if ( pexist( Make))
		TIFFSetField( tiff, TIFFTAG_MAKE, pget_c( Make));
	if ( pexist( Model))
		TIFFSetField( tiff, TIFFTAG_MODEL, pget_c( Model));
	if ( pexist( PageName))
		TIFFSetField( tiff, TIFFTAG_PAGENAME, pget_c( PageName));
	if ( pexist( Software))
		TIFFSetField( tiff, TIFFTAG_SOFTWARE, pget_c( Software));
	else
		TIFFSetField( tiff, TIFFTAG_SOFTWARE, "Prima");
	if ( pexist( XPosition))
		TIFFSetField( tiff, TIFFTAG_XPOSITION, pget_f( XPosition));
	if ( pexist( YPosition))
		TIFFSetField( tiff, TIFFTAG_YPOSITION, pget_f( YPosition));
	if ( pexist( XResolution))
		TIFFSetField( tiff, TIFFTAG_XRESOLUTION, pget_f( XResolution));
	if ( pexist( YResolution))
		TIFFSetField( tiff, TIFFTAG_YRESOLUTION, pget_f( YResolution));
	{
		Bool r1 = pexist( PageNumber), r2 = pexist( PageNumber2);
		uint16 u2;
		if (( r1 && !r2) || ( !r1 && r2)) {
			outc( "Fields PageNumber and PageNumber2 must be present simultaneously");
			return false;
		}
		if ( r1 && r2) {
			u16 = pget_i( PageNumber);
			u2 = pget_i( PageNumber2);
			TIFFSetField( tiff, TIFFTAG_PAGENUMBER, u16, u2);
		}
	}

	/* write data */
	TIFFSetField( tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField( tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField( tiff, TIFFTAG_ROWSPERSTRIP, 1);
	if ( !icon && i-> type != imRGB) {
		PRGBColor p = i-> palette;
		Byte * r = i-> data + ( i-> h - 1 ) * i-> lineSize;
		uint16 photometric = PHOTOMETRIC_MINISBLACK;
		switch ( i-> type) {
		case imbpp1:
			if ( p[0].r == 0 && p[0].g == 0 && p[0].b == 0 &&
				p[1].r == 255 && p[1].g == 255 && p[1].b == 255)
				photometric = PHOTOMETRIC_MINISBLACK;
			else if
				( p[1].r == 0 && p[1].g == 0 && p[1].b == 0 &&
				p[0].r == 255 && p[0].g == 255 && p[0].b == 255)
				photometric = PHOTOMETRIC_MINISWHITE;
			else
				photometric = PHOTOMETRIC_PALETTE;
			break;
		case imbpp4:
		case imbpp8:
			photometric = PHOTOMETRIC_PALETTE;
			break;
		default:
			photometric = PHOTOMETRIC_MINISBLACK;
			break;
		}
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric);
		TIFFSetField( tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, i-> type & imBPP);

		if ( photometric == PHOTOMETRIC_PALETTE) {
			int x, lim = (i-> palSize > 256) ? 256 : i-> palSize;
			uint16 red[256], green[256], blue[256];
			for ( x = 0; x < lim; x++, p++) {
				red  [x] = p-> r << 8;
				green[x] = p-> g << 8;
				blue [x] = p-> b << 8;
			}
			TIFFSetField( tiff, TIFFTAG_COLORMAP, red, green, blue);
		}
		for ( y = 0; y < i-> h; y++) {
			if ( !TIFFWriteScanline( tiff, r, y, 0) || err_signal)
				return false;
			r -= i-> lineSize;
		}
	} else if ( !icon && i-> type == imRGB) {
		Byte * conv;
		Byte * r = i-> data + ( i-> h - 1 ) * i-> lineSize;
		if ( !( conv = malloc( i-> lineSize))) {
			outcm( i-> lineSize);
			return false;
		}
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField( tiff, TIFFTAG_SAMPLESPERPIXEL, 3);
		TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, 8);
		for ( y = 0; y < i-> h; y++) {
			cm_reverse_palette(( RGBColor*) r, ( RGBColor*) conv, i-> w);
			if ( !TIFFWriteScanline( tiff, conv, y, 0) || err_signal) {
				free( conv);
				return false;
			}
			r -= i-> lineSize;
		}
		free( conv);
	} else { /* icon */
		Byte * conv1, * conv2;
		Byte * r, * mask = i-> mask + ( i-> h - 1 ) * i-> maskLine;
		Handle dup = CImage( fi-> object)-> dup( fi-> object);
		int lineSize;
		if ( !dup) return false;
		if ( !( conv1 = malloc( i-> lineSize + i-> w * 2))) {
			Object_destroy( dup);
			outcm( i-> lineSize + i-> w * 2);
			return false;
		}
		conv2 = conv1 + i-> lineSize + i-> w;
		CImage( dup)-> reset( dup, imRGB, nil, 0);
		lineSize = PImage( dup)-> lineSize;
		r = PImage( dup)-> data + ( i-> h - 1 ) * lineSize;
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
		TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, 8);
		for ( y = 0; y < i-> h; y++) {
			Byte * sconv1 = conv1 + 3, * sconv2 = conv2;
			bc_mono_byte( mask, conv2, i-> w);
			bc_rgb_bgri( r, conv1, i-> w);
			for ( x = 0; x < i-> w; x++, sconv1 += 4, sconv2++)
				*sconv1 = *sconv2 ? 0 : 255;
			if ( !TIFFWriteScanline( tiff, conv1, y, 0) || err_signal) {
				free( conv1);
				Object_destroy( dup);
				return false;
			}
			r -= lineSize;
			mask -= i-> maskLine;
		}
		free( conv1);
		Object_destroy( dup);
	}

	TIFFWriteDirectory( tiff);

	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	errbuf = fi-> errbuf;
	err_signal = 0;
	TIFFClose(( TIFF*) fi-> instance);
	errbuf = nil;
}

static TIFFErrorHandler old_error_handler, old_warning_handler;

static void
error_handler( const char* module, const char* fmt, va_list ap)
{
	if ( errbuf) vsnprintf( errbuf, 255, fmt, ap);
	err_signal = 1;
}

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	codec_info. vendor  = ( char *) TIFFGetVersion();
	old_error_handler   = TIFFSetErrorHandler(( TIFFErrorHandler) error_handler);
	old_warning_handler = TIFFSetWarningHandler(( TIFFErrorHandler) nil);
	return (void*)1;
}

static void
done( PImgCodec instance)
{
	TIFFSetErrorHandler( old_error_handler);
	TIFFSetErrorHandler( old_warning_handler);
}

void
apc_img_codec_tiff( void )
{
	struct ImgCodecVMT vmt;
	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
	vmt. init          = init;
	vmt. load_defaults = load_defaults;
	vmt. done          = done;
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
