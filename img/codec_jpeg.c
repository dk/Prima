#include <setjmp.h>
#if defined(WIN32)
/*
MINGW defines setjmp as some serious voodoo that PerlProc_setjmp cannot emulate,
and _setjmp is not equivaluent to setjmp anymore
*/
#pragma push_macro("setjmp")
#pragma push_macro("longjmp")
#endif
#define USE_NO_MINGW_SETJMP_TWO_ARGS
#include "img.h"
#include "img_conv.h"
#include "Image.h"
#undef LOCAL
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#undef HAVE_BOOLEAN
#ifdef BROKEN_PERL_PLATFORM
#undef      FAR
#undef      setjmp
#undef      longjmp
#define     setjmp _setjmp
#endif
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define XMD_H /* fails otherwise on redefined INT32 */
#include <jpeglib.h>
#include <jerror.h>
#if defined(WIN32)
#pragma pop_macro("setjmp")
#pragma pop_macro("longjmp")
#endif

#ifdef __cplusplus
extern "C" {
#endif

static char * jpgext[] = { "jpg", "jpe", "jpeg", NULL };
static int    jpgbpp[] = { imbpp8 | imGrayScale, imbpp24, 0 };

static char * loadOutput[] = {
	"comment",
	"appdata",
	NULL
};

static char * mime[] = {
	"image/jpeg",
	NULL
};

static ImgCodecInfo codec_info = {
	"JPEG",
	"Independent JPEG Group",
	6, 1,    /* version */
	jpgext,    /* extension */
	"JPEG File Interchange Format",     /* file type */
	"JPEG", /* short type */
	NULL,    /* features  */
	"Prima::Image::jpeg",  /* module */
	"Prima::Image::jpeg",  /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM,
	jpgbpp, /* save types */
	loadOutput,
	mime
};

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	codec_info. versionMaj = JPEG_LIB_VERSION / 10;
	codec_info. versionMin = JPEG_LIB_VERSION % 10;
	return (void*)1;
}

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_c( exifTransform, "none");
	return profile;
}

typedef struct _LoadRec {
	struct  jpeg_decompress_struct d;
	struct  jpeg_error_mgr         e;
	jmp_buf                        j;
	Bool                        init;
	Bool                        decompressed;
	Byte *                    channelbuf;
	Byte *                    transformbuf;
} LoadRec;


static void
load_output_message(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	PImgLoadFileInstance fi = ( PImgLoadFileInstance)( cinfo-> client_data);
	LoadRec *l = (LoadRec*)( fi-> instance);
	if ( !l-> init) {
		(*cinfo->err->format_message) (cinfo, buffer);
		strlcpy( fi-> errbuf, buffer, 256);
	}
}

static void
load_error_exit(j_common_ptr cinfo)
{
	static jmp_buf j;
	LoadRec *l = (LoadRec*)((( PImgLoadFileInstance)( cinfo-> client_data))-> instance);
	load_output_message( cinfo);
	memcpy( j, l->j, sizeof(jmp_buf));
	longjmp( j, 1);
}

/* begin ripoff from jdatasrc.c */
typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */
	JOCTET * buffer;		/* start of buffer */
	boolean start_of_file;	        /* have we gotten any data yet? */
	ImgIORequest  *req;
	HV * fp;                        /* frame properties */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */

void
init_source (j_decompress_ptr cinfo)
{
	((my_src_ptr) cinfo->src)->start_of_file = true;
}

boolean
fill_input_buffer (j_decompress_ptr cinfo)
{
	unsigned long nbytes;
	my_src_ptr src = (my_src_ptr) cinfo->src;

	nbytes = req_read( src->req, INPUT_BUF_SIZE, src->buffer);
	if (nbytes <= 0) {
		if (src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = false;

	return true;
}

void
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
		* it doesn't work on pipes.  Not clear that being smart is worth
		* any trouble anyway --- large skips are infrequent.
		*/
	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			* so suspension need not be handled.
			*/
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

/*
* Terminate source --- called by jpeg_finish_decompress
* after all data has been read.  Often a no-op.
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

void
term_source (j_decompress_ptr cinfo)
{
	/* no work necessary here */
}

static boolean j_read_profile(j_decompress_ptr jpeg_info);
static boolean j_read_comment(j_decompress_ptr jpeg_info);

static void
custom_src( j_decompress_ptr cinfo, PImgLoadFileInstance fi)
{
	my_src_ptr src;

	cinfo->src = (struct jpeg_source_mgr *) malloc(sizeof(my_source_mgr));
	src = (void*) cinfo-> src;
	src-> buffer = (JOCTET *) malloc( INPUT_BUF_SIZE * sizeof(JOCTET));
	src-> pub.init_source = init_source;
	src-> pub.fill_input_buffer = fill_input_buffer;
	src-> pub.skip_input_data = skip_input_data;
	src-> pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src-> pub.term_source = term_source;
	src-> pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src-> pub.next_input_byte = NULL; /* until buffer loaded */

	if ( fi-> loadExtras) {
		int i;
		jpeg_set_marker_processor( cinfo, JPEG_COM, j_read_comment);
		for ( i = 1; i < 16; i++)
			jpeg_set_marker_processor( cinfo, (int) (JPEG_APP0+i), j_read_profile);
	}
	src-> req    = fi-> req;
}
/* end ripoff from jdatasrc.c */

/* ripoff from imagemagick/coders/jpeg.c */
static int
j_read_octet(j_decompress_ptr jpeg_info)
{
	if (jpeg_info->src->bytes_in_buffer == 0)
		(void) (*jpeg_info->src->fill_input_buffer)(jpeg_info);
	jpeg_info->src->bytes_in_buffer--;
	return ((int) GETJOCTET(*jpeg_info->src->next_input_byte++));
}

/* Read generic profile  */
static boolean
j_read_profile(j_decompress_ptr jpeg_info)
{
	SV ** sv;
	AV * av;
	HV * fp = ((my_source_mgr *)(jpeg_info-> src))-> fp;
	char * name, *p;
	int marker, l, length;

	length  = j_read_octet( jpeg_info) << 8;
	length += j_read_octet( jpeg_info);
	if (length <= 2) return true;

	length -= 2;
	name = malloc( length);
	if ( !name) return true;

	marker  = jpeg_info->unread_marker - JPEG_APP0;
	p = name;
	l = length;
	while (l--) *p++ = j_read_octet( jpeg_info);

	sv = hv_fetch( fp, "appdata", 7, 0);
	if ( sv == NULL) {
		av = newAV();
		(void) hv_store( fp, "appdata", 7, newRV_noinc((SV*) av), 0);
	} else {
		if ( SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVAV) {
	   		av = (AV*) SvRV( *sv);
		} else {
	   		croak("bad profile 'appdata': expected array");
		}
	}

	av_store( av, marker, newSVpv( name, length));

	free( name);
	return true;
}

static boolean
j_read_comment(j_decompress_ptr jpeg_info)
{
	char *comment, *p;
	int l, length;

	length  = j_read_octet( jpeg_info) << 8;
	length += j_read_octet( jpeg_info);
	if (length <= 2) return true;

	length -= 2;
	comment = malloc( length+1);
	if ( !comment) return true;

	l = length;
	p = comment;
	while (l--) *p++ = j_read_octet( jpeg_info);
	*p = 0;

	(void) hv_store(
		((my_source_mgr *)(jpeg_info-> src))-> fp,
		"comment",  7,
		newSVpv( comment, length), 0);
	free( comment);

	return true;
}
/* end ripoff from imagemagick/coders/jpeg.c */


static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	Byte buf[2];
	jmp_buf j;

	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return false;
	if ( req_read( fi-> req, 2, buf) < 0) {
		req_seek( fi-> req, 0, SEEK_SET);
		return false;
	}
	if ( memcmp( "\xff\xd8", buf, 2) != 0) {
		req_seek( fi-> req, 0, SEEK_SET);
		return false;
	}
	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return false;

	fi-> stop = true;
	fi-> frameCount = 1;

	l = malloc( sizeof( LoadRec));
	if ( !l) return NULL;
	memset( l, 0, sizeof( LoadRec));
	l-> d. client_data = ( void*) fi;
	l-> d. err = jpeg_std_error( &l-> e);
	l-> d. err-> output_message = load_output_message;
	l-> d. err-> error_exit = load_error_exit;
	l-> init = true;
	fi-> instance = l;
	if ( setjmp( j) != 0) {
		fi-> instance = NULL;
		jpeg_destroy_decompress(&l-> d);
		free( l);
		return false;
	}
	memcpy( l->j, j, sizeof(jmp_buf));
	jpeg_create_decompress( &l-> d);
	custom_src( &l-> d, fi);
	l-> init = false;
	return l;
}

/*
courtesy from gtk/gdk-pixbuf/io-jpeg.c

Check for exif header and catch endianness
Just skip data until exif header - it should be within 16 bytes from marker start.
Normal structure relative to APP1 marker -
	0x0000: APP1 marker entry = 2 bytes
	0x0002: APP1 length entry = 2 bytes
	0x0004: Exif Identifier entry = 6 bytes
	0x000A: Start of exif header (Byte order entry) - 4 bytes
		- This is what we look for, to determine endianness.
	0x000E: 0th IFD offset pointer - 4 bytes

	exif_marker->data points to the first data after the APP1 marker
	and length entries, which is the exif identification string.
	The exif header should thus normally be found at i=6, below,
	and the pointer to IFD0 will be at 6+4 = 10.
*/
#define BYTEORDER_UNKNOWN -1
#define BYTEORDER_LE 0
#define BYTEORDER_BE 1
#define READ_WORD(c,len,byteorder)  ((len < 2)  ? 0 : (c)[byteorder] + (int)((c)[!byteorder]) * 256)
#define READ_DWORD(c,len,byteorder) ((len < 4)  ? 0 : \
	READ_WORD((c) + byteorder * 2, len, byteorder ) + \
	READ_WORD((c) + (!byteorder) * 2, len, byteorder ) * 65536)

static int
exif_find_orientation_tag( unsigned char * c, STRLEN len, int wipe)
{
	int byteorder = BYTEORDER_UNKNOWN, ifd_offset, ntags;
	unsigned char le_sig[] = { 0x49, 0x49, 0x2a, 0x00 };
	unsigned char be_sig[] = { 0x4d, 0x4d, 0x00, 0x2a };
	if ( len < 36 ) return 0;
#define ADVANCE(x) c += x; len -= x

	if ( memcmp((void*)c, (void*)"Exif\0\0", 6) != 0 ) return 0;
	ADVANCE(6);

	/* find out byteorder */
	if (memmem( c, 16, le_sig, 4) != NULL ) {
		byteorder = BYTEORDER_LE;
	} else if (memmem( c, 16, be_sig, 4) != NULL ) {
		byteorder = BYTEORDER_BE;
	}
	if ( byteorder == BYTEORDER_UNKNOWN ) return 0;
	ADVANCE(4);

	/* Read out the offset pointer to IFD0 */
	ifd_offset = READ_DWORD(c, len, byteorder) - 4;
	if ( ifd_offset < 0 || len < ifd_offset ) return 0;
	ADVANCE(ifd_offset);

	/* Find out how many tags we have in IFD0. As per the exif spec, the first
		two bytes of the IFD contain a count of the number of tags.*/
	ntags = READ_WORD(c, len, byteorder);
	ADVANCE(2);
	if ( ntags * 12 > len ) return 0;

	/* Check through IFD0 for tags of interest */
	while (ntags--) {
		/*  The tags are listed in consecutive 12-byte blocks */
		int tag   = READ_WORD ( c + 0, len - 0, byteorder);
		int type  = READ_WORD ( c + 2, len - 2, byteorder);
		int count = READ_DWORD( c + 4, len - 4, byteorder);
		int value = READ_WORD ( c + 8, len - 8, byteorder);
		/* Is this the orientation tag? */
		if ( tag != 0x112 ) {
			ADVANCE(12);
			continue;
		}
		if ( type != 3 || count != 1 || value > 8 ) return 0;
		if ( wipe ) c[8 + byteorder] = 0;
		return value;
	}

	return 0;
}

static int
exif_find_angle_tag( unsigned char * c, STRLEN len, int wipe)
{
	int i;
	char * c2, buf[256], sig[] = "AngleInfoRoll>";

	if ((c = (unsigned char*) memmem((const void*)c, len, sig, strlen(sig))) == NULL) return 0;
	c += strlen( sig );
	if ((c2 = strstr((char*)c, "<")) == NULL) return 0;

	strlcpy( buf, (char*)c, c2 - (char*)c);
	i = atoi(buf);
	if ( i == 0 ) return 0;

	if ( wipe ) {
		for ( ; (char*)c < c2; c++) *c = '0';
	}
	if ( i >=  45 && i < 135 ) return 8;
	if ( i >= 135 && i < 225 ) return 3;
	if ( i >= 225 && i < 315 ) return 6;
	return 1;
}

static int
exif_detect_orientation( HV * fp, int wipe )
{
	int i, orientation, ret = 0;
	AV * av;
	SV ** sv = hv_fetch( fp, "appdata", 7, 0);
	if ( !( sv && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVAV)) return 1;
	av = (AV*) SvRV(*sv);
	for ( i = 0; i <= av_len(av); i++) {
		unsigned char * c;
		STRLEN len;
		SV ** ssv = av_fetch( av, i, 0);
		if ( !ssv || !SvPOK( *ssv)) continue;
		c = (unsigned char *) SvPV( *ssv, len );
		if ((orientation = exif_find_orientation_tag( c, len, wipe )) > 0) {
			if ( ret == 0 ) ret = orientation;
			if ( !wipe ) break;
		}
		if ((orientation = exif_find_angle_tag( c, len, wipe )) > 0) {
			if ( ret == 0 ) ret = orientation;
			if ( !wipe ) break;
		}
	}
	return ret;
}

/*
		1        2       3      4         5            6           7          8

	888888  888888      88  88      8888888888  88                  88  8888888888
	88          88      88  88      88  88      88  88          88  88      88  88
	8888      8888    8888  8888    88          8888888888  8888888888          88
	88          88      88  88
	88          88  888888  888888

*/

static void
exif_setup_rotation( PImage i, PImage d, Byte ** dest, int orientation, int * direction )
{
	*d = *i;
	d-> h = 1;
	if ( orientation > 4 ) {
		d-> w = i-> h;
		d-> lineSize = LINE_SIZE(d->w, d->type);
	}
	d-> dataSize = d-> lineSize;

	switch (orientation) {
	case 2:
		*dest = i-> data + ( i-> h - 1) * i-> lineSize;
		*direction = SCANLINES_DIR_TOP_TO_BOTTOM;
		break;
	case 3:
	case 4:
		*dest = i-> data;
		*direction = SCANLINES_DIR_BOTTOM_TO_TOP;
		break;
	case 5:
	case 8:
		*dest = i-> data;
		*direction = SCANLINES_DIR_LEFT_TO_RIGHT;
		break;
	case 6:
	case 7:
		*dest = i-> data + (i-> w - 1) * ( i->type & imBPP) / 8;
		*direction = SCANLINES_DIR_RIGHT_TO_LEFT;
		break;
	}
}

static void
exif_transform_scanline( PImage i, PImage d, Byte ** dest, int orientation)
{
	switch (orientation) {
	case 2:
		img_mirror((Handle) d, 0);
		memcpy( *dest, d-> data, d-> lineSize);
		*dest -= i-> lineSize;
		break;
	case 3:
		img_integral_rotate((Handle) d, *dest, 0, 180);
		*dest += i-> lineSize;
		break;
	case 4:
		memcpy( *dest, d-> data, d-> lineSize);
		*dest += i-> lineSize;
		break;
	case 5:
		img_integral_rotate((Handle) d, *dest, i-> lineSize, 270);
		*dest += (d->type & imBPP) / 8;
		break;
	case 6:
		img_integral_rotate((Handle) d, *dest, i-> lineSize, 270);
		*dest -= (d->type & imBPP) / 8;
		break;
	case 7:
		img_integral_rotate((Handle) d, *dest, i-> lineSize, 90);
		*dest -= (d->type & imBPP) / 8;
		break;
	case 8:
		img_integral_rotate((Handle) d, *dest, i-> lineSize, 90);
		*dest += (d->type & imBPP) / 8;
		break;
	}
}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	dPROFILE;
	LoadRec * l;
	PImage i;
	int bpp, orientation, width, height;
	HV * profile;
	jmp_buf j;

	if ( setjmp( j) != 0) {
		l = ( LoadRec *) fi-> instance;
		if ( l && l->decompressed ) {
			fi-> wasTruncated = 1;
			return !fi->noIncomplete;
		}
		return false;
	}
	l = ( LoadRec *) fi-> instance;
	memcpy( l->j, j, sizeof(jmp_buf));
	i = ( PImage) fi-> object;
	orientation = 0;
	profile = fi-> profile;

	((my_source_mgr*)(l-> d. src))-> fp = fi-> frameProperties;
	jpeg_read_header( &l-> d, true);

	jpeg_start_decompress( &l-> d);
	bpp = l-> d. output_components * 8;
	if ( bpp != 8 && bpp != 24 && bpp != 32) {
		sprintf( fi-> errbuf, "Bit depth %d is not supported", bpp);
		return false;
	}

	width  = l-> d. output_width;
	height = l-> d. output_height;
	if ( pexist( exifTransform )) {
		char * cmd = pget_c(exifTransform);
		int wipe = 0, transform = 0;
		if ( strcmp(cmd, "wipe") == 0 ) {
			wipe = transform = 1;
		} else if ( strcmp( cmd, "auto" ) == 0 ) {
			transform = 1;
		}
		if ( transform ) {
			orientation = exif_detect_orientation(fi->frameProperties, wipe);
			if ( orientation > 4 ) {
				int i = width;
				width = height;
				height = i;
			}
		}
	}

	if ( bpp == 32) bpp = 24;
	if ( bpp == 8) bpp |= imGrayScale;
	CImage( fi-> object)-> create_empty( fi-> object, 1, 1, bpp);
	if ( fi-> noImageData) {
		(void) hv_store( fi-> frameProperties, "width",  5, newSViv( width), 0);
		(void) hv_store( fi-> frameProperties, "height", 6, newSViv( height), 0);
		jpeg_abort_decompress( &l-> d);
		return true;
	}

	CImage( fi-> object)-> create_empty( fi-> object, width, height, bpp);
	EVENT_HEADER_READY(fi);
	{
		Byte * dest = i-> data + ( i-> h - 1) * i-> lineSize;
		int direction = SCANLINES_DIR_TOP_TO_BOTTOM;
		Image rotation;

		if (l-> d. output_components == 4) {
			if ( !(l->channelbuf = malloc( l-> d. output_width * 4))) {
				sprintf( fi-> errbuf, "Not enough memory");
				return false;
			}
		}

		if ( orientation > 1 ) {
			if ( !(l->transformbuf = malloc( l-> d. output_width * 4))) {
				sprintf( fi-> errbuf, "Not enough memory");
				return false;
			}
			exif_setup_rotation( i, &rotation, &dest, orientation, &direction );
	 		rotation. data = l-> transformbuf;
		}

		while ( l-> d.output_scanline < l-> d.output_height ) {
			JSAMPROW sarray[1];
			int scanlines;
	 		Byte * prima_pixels = (orientation > 1) ? rotation. data : dest;
			sarray[0] = (l-> d. output_components == 4) ? l->channelbuf : prima_pixels;
			scanlines = jpeg_read_scanlines(&l-> d, sarray, 1);
			if ( scanlines == 0 ) {
	    			fi-> wasTruncated = 1;
		    		if ( fi->noIncomplete ) {
					sprintf( fi-> errbuf, "Image is truncated");
					return false;
		    		}
		    		break;
		 	}
			l->decompressed = true;

			switch (l-> d. output_components) {
			case 3:
				cm_reverse_palette(( PRGBColor) prima_pixels, ( PRGBColor) prima_pixels, l->d.output_width);
				break;
			case 4: {
				register Byte * s = l->channelbuf - 4;
				register Byte * d = prima_pixels;
				register int w = l->d.output_width;
				register Byte mul;
				while (w-- > 0) {
					s += 7;
					mul = *s--;
					*d++ = *s-- * mul / 255;
					*d++ = *s-- * mul / 255;
					*d++ = *s * mul / 255;
				}
			}
			break; }
			if ( orientation > 1 ) {
				exif_transform_scanline( i, &rotation, &dest, orientation );
			} else {
				dest -= i-> lineSize;
			}
			EVENT_SCANLINES_READY(fi,1,direction);
		}
	}
	EVENT_TOPDOWN_SCANLINES_FINISHED(fi);
	jpeg_finish_decompress(&l-> d);
	return true;
}


static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	my_src_ptr src = (my_src_ptr) l->d.src;
	free( src-> buffer);
	free( src);
	free(l->transformbuf);
	free(l->channelbuf);
	l->d.src = NULL;
	jpeg_destroy_decompress(&l-> d);
	free( l);
}

typedef struct _SaveRec {
	struct  jpeg_compress_struct   c;
	struct  jpeg_error_mgr         e;
	jmp_buf                        j;
	Byte                       * buf;
	Bool                        init;
} SaveRec;


static void
save_output_message(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	PImgSaveFileInstance fi = ( PImgSaveFileInstance)( cinfo-> client_data);
	SaveRec *l = (SaveRec*)( fi-> instance);
	if ( !l-> init) {
		(*cinfo->err->format_message) (cinfo, buffer);
		strlcpy( fi-> errbuf, buffer, 256);
	}
}

static void
save_error_exit(j_common_ptr cinfo)
{
	jmp_buf j;
	SaveRec *l = (SaveRec*)((( PImgSaveFileInstance)( cinfo-> client_data))-> instance);
	save_output_message( cinfo);
	memcpy( j, l->j, sizeof(jmp_buf));
	longjmp( j, 1);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( quality, 75);
	pset_i( progressive, 0);
	pset_c( comment, "");
	pset_sv( appdata, newRV_noinc((SV*) newAV()));
	return profile;
}

/* begin ripoff from jdatadst.c */

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

/* Expanded data destination object for stdio output */

typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */
	PImgIORequest req;
	JOCTET * buffer;		/* start of buffer */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

void
init_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	/* Allocate the output buffer --- it will be released when done with image */
	dest->buffer = (JOCTET *) malloc( OUTPUT_BUF_SIZE * sizeof(JOCTET));
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}


/* Empty the output buffer --- called whenever buffer fills up.  */

boolean
empty_output_buffer (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	if ( req_write(dest->req, OUTPUT_BUF_SIZE, dest->buffer) !=
			(size_t) OUTPUT_BUF_SIZE)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return true;
}


/*
* Terminate destination --- called by jpeg_finish_compress
* after all data has been written.  Usually needs to flush buffer.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

void
term_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

	/* Write any data remaining in the buffer */
	if (datacount > 0) {
		if (req_write(dest->req, datacount, dest->buffer) != datacount)
			ERREXIT(cinfo, JERR_FILE_WRITE);
	}
	req_flush(dest->req);
	/* Make sure we wrote the output file OK */
	if (req_error(dest->req))
		ERREXIT(cinfo, JERR_FILE_WRITE);
}

void
custom_dest(j_compress_ptr cinfo, PImgIORequest req)
{
	my_dest_ptr dest;

	cinfo->dest = (struct jpeg_destination_mgr *) malloc( sizeof(my_destination_mgr));
	dest = (my_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->req = req;
}

/* end ripoff from jdatadst.c */


static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * l;
	jmp_buf j;

	l = malloc( sizeof( SaveRec));
	if ( !l) return NULL;

	memset( l, 0, sizeof( SaveRec));
	l-> c. client_data = ( void*) fi;
	l-> c. err = jpeg_std_error( &l-> e);
	l-> c. err-> output_message = save_output_message;
	l-> c. err-> error_exit = save_error_exit;
	l-> init = true;
	fi-> instance = l;

	if ( setjmp( j) != 0) {
		fi-> instance = NULL;
		jpeg_destroy_compress(&l-> c);
		free( l);
		return false;
	}
	memcpy( l->j, j, sizeof(jmp_buf));
	jpeg_create_compress( &l-> c);
	custom_dest( &l-> c, fi-> req);
	l-> init = false;
	return l;
}

static Bool
j_write_extras( PImgSaveFileInstance fi, j_compress_ptr j, int marker, SV * data)
{
	STRLEN len;
	char * p;

	p = SvPV( data, len);
	if ( len > 65535 ) {
		strcpy( fi-> errbuf, "appdata length must be less than 65536 bytes");
		return false;
	}
	jpeg_write_marker( j, marker, ( unsigned char *) p, len);
	return true;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	PImage i;
	SaveRec * l;
	AV * appdata;
	HV * profile;
	jmp_buf j;

	l = ( SaveRec *) fi-> instance;
	if ( setjmp( j) != 0) return false;

	memcpy( l->j, j, sizeof(jmp_buf));
	i = ( PImage) fi-> object;
	l = ( SaveRec *) fi-> instance;
	appdata = NULL;
	profile = fi-> extras;

	l-> c. image_width  = i-> w;
	l-> c. image_height = i-> h;
	l-> c. input_components = ((( i-> type & imBPP) == 24) ? 3 : 1);
	l-> c. in_color_space   = ((( i-> type & imBPP) == 24) ? JCS_RGB : JCS_GRAYSCALE);
	jpeg_set_defaults( &l-> c);

	if ( pexist( quality)) {
		int q = pget_i( quality);
		if ( q < 0 || q > 100) {
			strcpy( fi-> errbuf, "quality must be in 0..100");
			return false;
		}
		jpeg_set_quality(&l-> c, q, true /* limit to baseline-JPEG values */);
	}

	/* Optionally allow simple progressive output. */
	if ( pexist( progressive) && pget_B( progressive))
		jpeg_simple_progression(&l-> c);

	if ( l-> c. input_components == 3) { /* RGB */
		l-> buf = malloc( i-> lineSize);
		if ( !l-> buf) {
			strcpy( fi-> errbuf, "not enough memory");
			return false;
		}
	}

	if ( pexist( appdata)) {
		SV * sv = pget_sv( appdata);
		if ( !SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV) {
			strcpy( fi-> errbuf, "'appdata' must be an array");
			return false;
		}
		appdata = (AV*) SvRV( sv);
	}

	jpeg_start_compress( &l-> c, true);

	/* write extras */
	if ( pexist( comment))
		if ( !j_write_extras( fi, &l-> c, JPEG_COM, pget_sv( comment)))
			return false;

	if ( appdata) {
		int marker;
		SV ** sv;
		for ( marker = 1; marker < 16; marker++) {
			sv = av_fetch( appdata, marker, 0);
			if ( sv && *sv && SvOK( *sv))
	     			if ( !j_write_extras( fi, &l-> c, JPEG_APP0 + marker, *sv))
					return false;
		}
	}

	/* write pixels */
	{
		Byte * src = i-> data + ( i-> h - 1) * i-> lineSize;
		while ( l-> c.next_scanline < i-> h ) {
	 		JSAMPROW sarray[1];
			if ( l-> buf) {
				cm_reverse_palette(( PRGBColor) src, (PRGBColor) l-> buf, i-> w);
				sarray[0] = l-> buf;
			} else
				sarray[0] = src;
	 		jpeg_write_scanlines(&l-> c, sarray, 1);
			src -= i-> lineSize;
		}
	}
	jpeg_finish_compress( &l-> c);
	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * l = ( SaveRec *) fi-> instance;
	my_dest_ptr dest = (my_dest_ptr) l->c.dest;
	free( dest-> buffer);
	free( dest);
	free( l-> buf);
	l->c.dest = NULL;
	jpeg_destroy_compress(&l-> c);
	free( l);
}

void
apc_img_codec_jpeg( void )
{
	struct ImgCodecVMT vmt;
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
	apc_img_register( &vmt, NULL);
}

#ifdef __cplusplus
}
#endif
