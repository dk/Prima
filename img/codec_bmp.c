/*

Copyright Andy Key, Heiko Nitzsche

gbmbmp.c - OS/2 1.1, 1.2, 2.0 and Windows 3.0 support

Reads and writes any OS/2 1.x bitmap.
Will also read uncompressed, RLE4 and RLE8 Windows 3.x bitmaps too.
There are horrific file structure alignment considerations hence each
word,dword is read individually.

*/

#include "img.h"
#include "Image.h"

static char * bmpext[] = {
	"bmp", "vga", "bga",
	"rle", "dib", "rl4",
	"rl8", NULL,
};

static int    bmpbpp[] = {
	imRGB,
	imbpp8 | imGrayScale, imbpp8,
	imbpp4 | imGrayScale, imbpp4,
	imbpp1 | imGrayScale, imbpp1,
	0
};
static char * bmpfeat[] = {
	"OS/2 BMP 1.0",
	"Windows BMP 1.0",
	"Windows BMP 2.0",
	"Windows BMP 3.0",
	"Windows BMP 3.0 RLE4",
	"Windows BMP 3.0 RLE8",
	NULL
};

static char * loadOutput[] = {
	"HotSpotX",
	"HotSpotY",
	"OS2",
	"Compression",
	"ImportantColors",
	"XResolution",
	"YResolution",
	"BitDepth",
	NULL
};

static char * mime[] = {
	"image/bmp",
	"image/x-bmp",
	"image/x-MS-bmp",
	"image/x-win-bitmap",
	NULL
};

static ImgCodecInfo codec_info = {
	"Windows Bitmap",
	"GBM by Andy Key",
	1, 7,		      /* version */
	bmpext,		      /* extension */
	"Windows Bitmap",     /* file type */
	"BMP",		      /* short type */
	bmpfeat,	      /* features  */
	"",		      /* module */
	"",		      /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_MULTIFRAME | IMG_LOAD_FROM_STREAM |
	IMG_SAVE_TO_FILE   | IMG_SAVE_TO_STREAM,
	bmpbpp,		      /* save types */
	loadOutput,
	mime
};

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	return (void*)1;
}

#ifndef min
#define min(a,b)    (((a)<(b))?(a):(b))
#endif

typedef uint32_t dword;
typedef uint16_t word;
typedef uint8_t  byte;

#define low_byte(w) ((byte)  ((w)&0x00ff)    )
#define high_byte(w)	((byte) (((w)&0xff00)>>8))
#define make_word(a,b)	(((word)a) + (((word)b) << 8))

static Bool
read_word( PImgIORequest req, word *w)
{
	Byte low = 0, high = 0;

	if ( req_read(req, 1, &low) != 1 )
		return false;
	if ( req_read(req, 1, &high) != 1 )
		return false;
	*w = (word) (low + ((word) high << 8));
	return true;
}

static Bool
read_dword(PImgIORequest fd, dword *d)
{
	word low, high;
	if ( !read_word(fd, &low) )
		return false;
	if ( !read_word(fd, &high) )
		return false;
	*d = low + ((dword) high << 16);
	return true;
}


static Bool
write_word(PImgIORequest fd, word w)
{
	byte	low  = (byte) (w & 0xFF);
	byte	high = (byte) (w >> 8);

	req_write( fd, 1, &low);
	req_write( fd, 1, &high);

	return true;
}

static Bool
write_dword(PImgIORequest req, dword d)
{
	write_word(req, (word) (d & 0xFFFF));
	write_word(req, (word) (d >> 16));
	return true;
}

#define BFT_BMAP    0x4d42
#define BFT_BITMAPARRAY 0x4142
#define BCA_UNCOMP  0x00000000L
#define BCA_RLE8    0x00000001L
#define BCA_RLE4    0x00000002L
#define BCA_BITFIELDS	0x00000003L
#define BCA_RLE24   0x00000004L
#define BCA_MAX     BCA_RLE24
#define MSWCC_EOL   0
#define MSWCC_EOB   1
#define MSWCC_DELTA 2

static char * bca_sets[] = {
	"Uncompressed",
	"RLE8",
	"RLE4",
	"Raw bits",
	"RLE24"
};

static void
swap_pal( RGBColor * p)
{
	RGBColor tmp = p[0];
	p[0] = p[1];
	p[1] = tmp;
}

#define outc(x){ strlcpy( fi-> errbuf, x, 256); return false;}
#define outr(fd) { snprintf( fi-> errbuf, 256, "Read error:%s",strerror( req_error( fd))); return false; }
#define outw(fd) { snprintf( fi-> errbuf, 256, "Write error:%s",strerror( req_error( fd))); return false; }
#define outs(fd) { snprintf( fi-> errbuf, 256, "Seek error:%s",strerror( req_error( fd))); return false; }
#define outcm(dd){ snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", (int)(dd)); return false;}
#define outcd(x,dd){ snprintf( fi-> errbuf, 256, x, (int)(dd)); return false;}


#define BUFSIZE 16384

typedef struct {
	byte buf[BUFSIZE];
	int inx, cnt, y, lasty;
	unsigned long ls, read;
	PImgLoadFileInstance fi;
	Bool error;
} AHEAD;

static AHEAD *
create_ahead(PImgLoadFileInstance fi, unsigned long lineSize)
{
	AHEAD *ahead;

	if ( (ahead = malloc((size_t) sizeof(AHEAD))) == NULL )
		return NULL;

	ahead-> inx   = 0;
	ahead-> cnt   = 0;
	ahead-> fi    = fi;
	ahead-> error = false;
	ahead-> y     = 0;
	ahead-> lasty = 0;
	ahead-> read  = 0;
	ahead-> ls    = lineSize;

	return ahead;
}

/* returns true on hard failure */
static Bool
destroy_ahead(AHEAD *ahead)
{
	Bool error = ahead-> error;
	if (error & ahead-> fi-> wasTruncated) error = false;
	free(ahead);
	return error;
}

static byte
read_ahead(AHEAD *ahead)
{
	if ( ahead-> inx >= ahead-> cnt ) {
		if ( ahead-> error) return 0;

		ahead-> cnt = req_read( ahead->fi->req, BUFSIZE, ahead->buf);
		if ( ahead-> cnt <= 0 ) {
			snprintf( ahead->fi-> errbuf, 256,
				"Read error:%s",
				(( ahead-> cnt == 0) ?
					"unexpected end of file" :
					strerror( req_error( ahead-> fi-> req))
			));
			ahead-> error = 1;
			if ( !ahead-> fi-> noIncomplete && ahead-> cnt >= 0)
				ahead-> fi-> wasTruncated = true;
			return 0;
		}

		ahead-> read += ahead-> cnt;
		ahead-> lasty = ahead-> y;
		ahead-> y	  = ahead-> read / ahead-> ls;
		ahead-> inx = 0;
		EVENT_SCANLINES_READY(ahead-> fi, ahead-> y - ahead-> lasty, SCANLINES_DIR_BOTTOM_TO_TOP);
	}

	return ahead->buf[ahead->inx++];
}

typedef struct _RGBTriplet {
	dword r;
	dword g;
	dword b;
} RGBTriplet;

typedef struct _LoadRec {
	long base;
	Bool windows;
	dword cbFix;
	dword ulCompression;
	dword cclrUsed;
	dword cclrImportant;
	dword offBits;
	word  xHotspot;
	word  yHotspot;
	Point resolution;
	Bool multiframe;
	int w, h, bpp;
	int passed;
	size_t passed_frame_offset;
	size_t file_start_offset;
	RGBTriplet rgb_offset;
	RGBTriplet rgb_mask;
	RGBTriplet rgb_valid_bits;
} LoadRec;


static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	PImgIORequest fd;
	word usType;

	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return NULL;

	fd = fi-> req;
	if ( !read_word( fd, &usType)) outr(fd);

	if ( usType != BFT_BMAP && usType != BFT_BITMAPARRAY)
		return NULL;

	fi-> stop = true;
	l = malloc( sizeof( LoadRec));
	if ( !l) outcm( sizeof( LoadRec));
	memset( l, 0, sizeof( LoadRec));
	fi-> instance = l;

	l-> multiframe = usType == BFT_BITMAPARRAY;
	l-> passed = -1;
	l-> file_start_offset = l-> passed_frame_offset = req_tell( fi-> req);
	if ( !l-> multiframe) fi-> frameCount = 1;

	return l;
}

static Bool
rewind_to_frame( PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	PImgIORequest fd = fi-> req;
	word usType;
	dword cbSize2, offNext, dummy;

	if ( !l-> multiframe)
		return true;

	if ( l-> passed >= fi-> frame) {
		/* reset to first frame */
		l-> passed = -1;
		l-> passed_frame_offset = l-> file_start_offset;
	}

	if ( req_seek( fd, l-> passed_frame_offset, SEEK_CUR) < 0)
		outs(fd);

	while ( ++l-> passed < fi-> frame ) {
		if ( !read_dword( fd, &cbSize2) )
			outr(fd);
		if ( !read_dword( fd, &offNext) )
			outr(fd);
		if ( offNext == 0L ) {
			fi-> frameCount = l-> passed;
			snprintf( fi-> errbuf, 256, "Index %d is out of range", fi-> frame);
			return false;
		}
		if ( req_seek( fd, (long )offNext, SEEK_SET) < 0)
		 	outs(fd);
		if ( !read_word( fi-> req, &usType) )
			outr(fd);
		if ( usType != BFT_BITMAPARRAY ) {
			snprintf( fi-> errbuf, 256, "Bad array magic at index %d", l-> passed);
			return false;
		}
	}
	if ( !read_dword( fd, &cbSize2) )
		outr(fd);
	if ( !read_dword( fd, &offNext) )
		outr(fd);
	if ( !read_dword( fd, &dummy) )
		outr(fd);
	if ( !read_word(fd, &usType) )
		outr(fd);
	if ( usType != BFT_BMAP ) {
		snprintf( fi-> errbuf, 256, "Bad magic at index %d", l-> passed);
		return false;
	}
	l-> passed_frame_offset = offNext;

	return true;
}

static dword
count_mask_bits(dword mask, dword * bitoffset)
{
	dword testmask = 1; /* start with the least significant bit */
	dword counter  = 0;
	dword index    = 0;

	/* find offset of first bit */
	while (((mask & testmask) == 0) && (index < 31)) {
		index++;
		testmask <<= 1;
	}
	*bitoffset = index;

	/* count the bits set in the rest of the mask */
	while ((testmask <= mask) && (index < 31)) {
		if (mask & testmask) {
			counter++;
		}
		index++;
		testmask <<= 1;
	}

	return counter;
}


static Bool
read_bmp_header( PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	PImgIORequest fd = fi-> req;
	dword cbSize, cbFix;

	l-> base = req_tell(fd) - 2L; /* BM */
	if ( l-> base < 0)
		outs(fd);
	if ( !read_dword(fd, &cbSize) )
		outr(fd);
	if ( !read_word(fd, &l-> xHotspot) )
		outr(fd);
	if ( !read_word(fd, &l-> yHotspot) )
		outr(fd);
	if ( !read_dword(fd, &l-> offBits) )
		outr(fd);
	if ( !read_dword(fd, &cbFix) )
		outr(fd);

	if ( cbFix == 12 ) {
		/* OS/2 1.x uncompressed bitmap */
		word cx, cy, cPlanes, cBitCount;

		if ( !read_word(fd, &cx) )
			outr(fd);
		if ( !read_word(fd, &cy) )
			outr(fd);
		if ( !read_word(fd, &cPlanes) )
			outr(fd);
		if ( !read_word(fd, &cBitCount) )
			outr(fd);

		if ( cx == 0 || cy == 0 )
			outc("Bad size");
		if ( cPlanes != 1 )
			outcd("Number of bitmap planes is %d, must be 1", cPlanes);
		if ( cBitCount != 1 && cBitCount != 4 && cBitCount != 8 && cBitCount != 24 )
			outcd("Bit count is %d, must be 1, 4, 8 or 24", cBitCount);

		l-> w   = (int) cx;
		l-> h   = (int) cy;
		l-> bpp = (int) cBitCount;
		l-> windows = false;
	}
	else if (
		cbFix >= 16 && cbFix <= 64 &&
		((cbFix & 3) == 0 || cbFix == 42 || cbFix == 46)
	) {
		/* OS/2 and Windows 3 */
		word cPlanes = 0, cBitCount = 0, usUnits = 0, usReserved, usRecording, usRendering = 0;
		dword ulWidth = 0, ulHeight = 0, ulCompression = 0;
		dword ulSizeImage, ulXPelsPerMeter = 0, ulYPelsPerMeter = 0;
		dword cclrUsed = 0, cclrImportant = 0, cSize1, cSize2, ulColorEncoding, ulIdentifier;
		Bool ok;

		ok	= read_dword(fd, &ulWidth);
		ok &= read_dword(fd, &ulHeight);
		ok &= read_word(fd, &cPlanes);
		ok &= read_word(fd, &cBitCount);
		if ( cbFix > 16 )
			ok &= read_dword(fd, &ulCompression);
		else
			ulCompression = BCA_UNCOMP;

		if ( cbFix > 20 )
			ok &= read_dword(fd, &ulSizeImage);
		if ( cbFix > 24 )
			ok &= read_dword(fd, &ulXPelsPerMeter);
		if ( cbFix > 28 )
			ok &= read_dword(fd, &ulYPelsPerMeter);
		if ( cbFix > 32 )
			ok &= read_dword(fd, &cclrUsed);
		else
			cclrUsed = ( (dword)1 << cBitCount );
		if ( cBitCount != 24 && cclrUsed == 0 )
			cclrUsed = ( (dword)1 << cBitCount );

		/* Protect against badly written bitmaps! */
		if ( cclrUsed > ( (dword)1 << cBitCount ) )
			cclrUsed = ( (dword)1 << cBitCount );

		if ( cbFix > 36 )
			ok &= read_dword(fd, &cclrImportant);
		if ( cbFix > 40 )
			ok &= read_word(fd, &usUnits);
		if ( cbFix > 42 )
			ok &= read_word(fd, &usReserved);
		if ( cbFix > 44 )
			ok &= read_word(fd, &usRecording);
		if ( cbFix > 46 )
			ok &= read_word(fd, &usRendering);
		if ( cbFix > 48 )
			ok &= read_dword(fd, &cSize1);
		if ( cbFix > 52 )
			ok &= read_dword(fd, &cSize2);
		if ( cbFix > 56 )
			ok &= read_dword(fd, &ulColorEncoding);
		if ( cbFix > 60 )
			ok &= read_dword(fd, &ulIdentifier);

		if ( !ok )
			outr(fd);

		if ( ulWidth == 0L || ulHeight == 0L )
			outc("Bad image size");
		if ( cPlanes != 1 )
			outcd("Number of bitmap planes is %d, must be 1", cPlanes);
		if (
			cBitCount != 1 &&
			cBitCount != 4 &&
			cBitCount != 8 &&
			cBitCount != 16 &&
			cBitCount != 24 &&
			cBitCount != 32
		)
			outcd("Bit count is %d, must be 1, 4, 8, 16, 24 or 32", cBitCount);

		l-> w   = (int) ulWidth;
		l-> h   = (int) ulHeight;
		l-> bpp = (int) cBitCount;
		l-> windows       = true;
		l-> cbFix	  = cbFix;
		l-> ulCompression = ulCompression;
		l-> cclrUsed      = cclrUsed;
		l-> cclrImportant = cclrImportant;
		l-> resolution.x  = ulXPelsPerMeter;
		l-> resolution.y  = ulYPelsPerMeter;
	} else
		outc("cbFix is bad");

	if ( l-> bpp == 16 || l-> bpp == 32 ) {
		switch ( l-> ulCompression) {
		case BCA_UNCOMP:
			l-> rgb_offset. b = 0;
			l-> rgb_offset. g = (l->bpp == 16) ?  5 :  8;
			l-> rgb_offset. r = (l->bpp == 16) ? 10 : 16;

			/* set color masks to either 16bpp (5,5,5) or 32bpp (8,8,8) */
			l-> rgb_mask. b = (l-> bpp == 16) ? 0x001f : 0x000000ff;
			l-> rgb_mask. g = (l-> bpp == 16) ? 0x03e0 : 0x0000ff00;
			l-> rgb_mask. r = (l-> bpp == 16) ? 0x7c00 : 0x00ff0000;

			l-> rgb_valid_bits. b =
			l-> rgb_valid_bits. g =
			l-> rgb_valid_bits. r = (l->bpp == 16) ?  5 :  8;
			break;

		case BCA_BITFIELDS: {
			Bool ok = 1;

			/* Read BI_BITFIELDS color masks from the header (where usually the palette is) */
			/* These are strangely stored as dwords in the order of R-G-B. */
			if ( req_seek(fd, (long) (l-> base + 14L + l-> cbFix), SEEK_SET) < 0)
				outs(fd);
			ok &= read_dword(fd, &l-> rgb_mask. r);
			ok &= read_dword(fd, &l-> rgb_mask. g);
			ok &= read_dword(fd, &l-> rgb_mask. b);
			if ( !ok )
				outr(fd);

			/* count the bits used in each mask */
			l-> rgb_valid_bits. b = count_mask_bits( l-> rgb_mask. b, &l-> rgb_offset. b);
			l-> rgb_valid_bits. g = count_mask_bits( l-> rgb_mask. g, &l-> rgb_offset. g);
			l-> rgb_valid_bits. r = count_mask_bits( l-> rgb_mask. r, &l-> rgb_offset. r);

			/* Only up to 8 bit per mask are allowed */
			if (
				l-> rgb_valid_bits. b > 8 ||
				l-> rgb_valid_bits. g > 8 ||
				l-> rgb_valid_bits. r > 8
			)
				outc("Bad bit masks for non-24bits RGB data");

			/* check for non-overlapping bits */
			if (
				l-> rgb_valid_bits. b +
				l-> rgb_valid_bits. g +
				l-> rgb_valid_bits. r > l-> bpp
			)
				outc("Bad bit masks for non-24bits RGB data");


			if (
				l-> rgb_offset. b + l-> rgb_valid_bits. b > l-> rgb_offset. g ||
				l-> rgb_offset. g + l-> rgb_valid_bits. g > l-> rgb_offset. r ||
				l-> rgb_offset. r + l-> rgb_valid_bits. r > l-> bpp
			)
				outc("Bad bit masks for non-24bits RGB data");

			l-> rgb_valid_bits. r = 8 - l-> rgb_valid_bits. r;
			l-> rgb_valid_bits. g = 8 - l-> rgb_valid_bits. g;
			l-> rgb_valid_bits. b = 8 - l-> rgb_valid_bits. b;
			break;
		}

		default:
			outcd("compression type is %d, expected 0 or 3", l->ulCompression);
	}}
	return true;
}

static Bool
req_read_big( PImgLoadFileInstance fi, int h, unsigned long lineSize, Byte * data)
{
	unsigned long size = h * lineSize, read = 0;
	int lasty = 0, y = 0;

	if ( fi-> eventMask & IMG_EVENTS_DATA_READY) {
		/* read and notify */
		while ( size > 0) {
			ssize_t r = req_read(
				fi-> req,
				( BUFSIZE > size) ? size : BUFSIZE,
				data
			);
			if ( r < 0)
				outr( fi-> req);
			if ( r == 0) {
				if ( fi-> noIncomplete)
					outc("Read error: unexpected end of file")
				else
					size = 0;
			}
			read += r;
			size -= r;
			data += r;
			lasty = y;
			y = read / lineSize;
			EVENT_SCANLINES_READY(fi, y - lasty, SCANLINES_DIR_BOTTOM_TO_TOP);
		}
	} else {
		/* just read */
		ssize_t r = req_read( fi-> req, size, data);
		if ( r < 0)
			outr( fi-> req);
		if ( r != size && fi-> noIncomplete)
			outc( "Read error: unexpected end of file");
	}

	return true;
}

/* Read 16bpp data with compression BI_RGB or BI_BITFIELDS (will be mapped to 24bpp, lossless) */
static Bool
read_16_32_bpp( PImgLoadFileInstance fi, PImage i, int bpp, unsigned long stride_dst)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	int h, stride_src = ((i-> w * 16 + 31) / 32) * 4;
	Byte *src, *dst;

	if ( !( src = (Byte*) malloc(stride_src)))
		outcm(stride_src);

	dst = i-> data; /* write pointer */
	for (h = 0; h < i-> h; h++) {
		int block_count = i-> w;
		const word *  src16  = (const word *)  src;
		const dword * src32  = (const dword *) src;
		Byte * line  = dst;
		ssize_t r = req_read( fi-> req, stride_src, src);

		if ( r != stride_src ) {
			free( src);
			if ( r < 0)
				outr( fi-> req);
			if ( fi-> noIncomplete)
				outc("Read error: unexpected end of file");
			h = i->h;
			fi-> wasTruncated = true;
		}

		/* Extract red, green, blue. */
		/* Encoding starts at least significant bit, then xB,yG,zR (most significant bit is unused). */
		/* Map these into 24bpp BGR. */
#define GetX(X,SRC) (((SRC & l-> rgb_mask.X) >> l->rgb_offset.X) << l->rgb_valid_bits.X)
		if ( bpp == 16 ) {
			while (block_count > 0) {
				register word data16 = *src16++;
				*line++ = GetX(b,data16);
				*line++ = GetX(g,data16);
				*line++ = GetX(r,data16);
				--block_count;
			}
		} else {
			while (block_count > 0) {
				register dword data32 = *src32++;
				*line++ = GetX(b,data32);
				*line++ = GetX(g,data32);
				*line++ = GetX(r,data32);
				--block_count;
			}
		}
#undef GetX
		dst += stride_dst;
		EVENT_SCANLINES_READY(fi, 1, SCANLINES_DIR_BOTTOM_TO_TOP);
	}

	free(src);
	return true;
}


static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	HV * profile = fi-> frameProperties;
	LoadRec * l = ( LoadRec *) fi-> instance;
	PImgIORequest fd = fi-> req;
	PImage img;
	int cLinesWorth; /* bmp alignment and prima alignment are identical, by 4-byte boundary */
	int bpp;
	Byte * data;

	if ( !rewind_to_frame(fi))
		return false;
	if ( !read_bmp_header(fi))
		return false;

	img = PImage( fi-> object);
	bpp = ( l-> bpp == 16 || l-> bpp == 32 ) ? 24 : l-> bpp;
	if ( fi-> noImageData) {
		pset_i( width,	l-> w);
		pset_i( height, l-> h);
		CImage( fi-> object)-> create_empty( fi-> object, 1, 1, bpp);
	} else {
		CImage( fi-> object)-> create_empty( fi-> object, l-> w, l-> h, bpp);
		EVENT_HEADER_READY( fi);
	}
	data = img-> data;

	/* read palette */
	if ( bpp != 24) {
		int i;
		byte b[4];
		PRGBColor pal;

		pal = img-> palette;
		if ( l-> windows ) { /* Windows */
			if ( req_seek(fd, (long) (l-> base + 14L + l-> cbFix), SEEK_SET) < 0)
				outs(fd);
			img-> palSize = (int) l-> cclrUsed;
			for ( i = 0; i < (int) l-> cclrUsed; i++, pal++ ) {
				if ( req_read( fd, 4, b) != 4)
					outr(fd);
				pal-> b = b[0];
				pal-> g = b[1];
				pal-> r = b[2];
			}
		} else { /* OS/2 */
			if ( req_seek(fd, (long) (l-> base + 26L), SEEK_SET) < 0)
				outs(fd);
			img-> palSize = 1 << l-> bpp;
			for ( i = 0; i < img-> palSize; i++, pal++ ) {
				if ( req_read( fd, 3, b) != 3)
					outr(fd);
				pal-> b = b[0];
				pal-> g = b[1];
				pal-> r = b[2];
			}
		}
	} else {
		img-> palSize = 0;
	}

	if ( bpp == 1)
		swap_pal( img-> palette);

	if ( fi-> loadExtras) {
		char * c;
		if ( !l-> windows)
		pset_i( OS2, 1);
		pset_i( HotSpotX, l-> xHotspot);
		pset_i( HotSpotY, l-> yHotspot);
		pset_i( BitDepth, l-> bpp);

		c = ( l-> ulCompression > BCA_MAX) ?
			"Unknown" : bca_sets[ l-> ulCompression];
		pset_c( Compression, c);
		if ( l-> windows) {
			pset_i( ImportantColors, l-> cclrImportant);
			pset_i( XResolution, l-> resolution.x);
			pset_i( YResolution, l-> resolution.y);
		}
	}

	if ( fi-> noImageData)
		return true;

	/* read data */
	cLinesWorth = ((bpp * l->w + 31) / 32) * 4;

	if ( l-> windows ) {
		if ( req_seek( fd, (long) l-> offBits, SEEK_SET) < 0)
	   		outs(fd);

		switch ( (int) l-> ulCompression ) {

		case BCA_UNCOMP:
			switch ( l-> bpp) {
			case 1:
			case 4:
			case 8:
			case 24:
				if ( !req_read_big(fi, l-> h, cLinesWorth, data))
					return false;
				break;
			case 16:
				if ( !read_16_32_bpp(fi, PImage(fi-> object), 16, cLinesWorth))
					return false;
				break;
			case 32:
				if ( !read_16_32_bpp(fi, PImage(fi-> object), 32, cLinesWorth))
					return false;
				break;
			default:
				outc("Unsupported bit depth");
			}
			break;

		case BCA_RLE8: {
			AHEAD *ahead;
			int x = 0, y = 0;
			Bool eof8 = false;

			if ( (ahead = create_ahead(fi, cLinesWorth)) == NULL )
				outcm(sizeof(AHEAD));

			while ( !eof8 ) {
				byte c = read_ahead(ahead);
				byte d = read_ahead(ahead);

				if ( c ) {
					memset(data, d, c);
					x += c;
					data += c;
				} else switch ( d ) {
					 case MSWCC_EOL: {
						int to_eol = cLinesWorth - x;

						memset( data, 0, (size_t) to_eol);
						data += to_eol;
						x = 0;
						if ( ++y == l->h )
							eof8 = true;
						}
						break;
					 case MSWCC_EOB:
						if ( y < l->h ) {
							int to_eol = cLinesWorth - x;

							memset(data, 0, (size_t) to_eol);
							x = 0; y++;
							data += to_eol;
							while ( y < l->h ) {
								memset(data, 0, (size_t) cLinesWorth);
								data += cLinesWorth;
								y++;
							}
						}
						eof8 = true;
						break;
					 case MSWCC_DELTA: {
					 	byte dx = read_ahead(ahead);
					 	byte dy = read_ahead(ahead);
					 	int fill = dx + dy * cLinesWorth;
					 	memset(data, 0, (size_t) fill);
					 	data += fill;
					 	x += dx; y += dy;
					 	if ( y == l->h )
					 		eof8 = true;
					 	}
					 	break;

					 default: {
					 	int n = (int) d;

					 	while ( n-- > 0 )
					 		*data++ = read_ahead(ahead);
					 	x += d;
					 	if ( d & 1 )
					 		read_ahead(ahead); /* Align */
					 	}
					 	break;
					 }
				}
				if ( destroy_ahead(ahead))
					return false;
			}
			break;

		case BCA_RLE4: {
			AHEAD *ahead;
			int x = 0, y = 0;
			Bool eof4 = false;
			int inx = 0;

			if ( (ahead = create_ahead(fi, cLinesWorth)) == NULL )
				outcm(sizeof(AHEAD));

			memset(data, 0, l->h * cLinesWorth);

			while ( !eof4 ) {
				byte c = read_ahead(ahead);
				byte d = read_ahead(ahead);

				if ( c ) {
					byte h, l;
					int i;
					if ( x & 1 ) {
						h = (byte) (d >> 4);
						l = (byte) (d << 4);
					} else {
						h = (byte) (d&0xf0);
						l = (byte) (d & 0x0f);
					}
					for ( i = 0; i < (int) c; i++, x++ ) {
						if ( x & 1U )
							data[inx++] |= l;
						else
							data[inx]   |= h;
					}
				} else switch ( d ) {
					case MSWCC_EOL:
							x = 0;
							if ( ++y == l->h )
								eof4 = true;
							inx = cLinesWorth * y;
							break;

					case MSWCC_EOB:
							eof4 = true;
							break;

					case MSWCC_DELTA: {
							byte dx = read_ahead(ahead);
							byte dy = read_ahead(ahead);

							x += dx; y += dy;
							inx = y * cLinesWorth + (x/2);

							if ( y == l->h )
								eof4 = true;
							}
							break;

					default: {
							int i, nr = 0;

							if ( x & 1 ) {
								for ( i = 0; i+2 <= (int) d; i += 2 ) {
									byte b = read_ahead(ahead);
									data[inx++] |= (b >> 4);
									data[inx  ] |= (b << 4);
									nr++;
								}
								if ( i < (int) d ) {
									data[inx++] |= (read_ahead(ahead) >> 4);
									nr++;
								}
							} else {
								for ( i = 0; i+2 <= (int) d; i += 2 ) {
									data[inx++] = read_ahead(ahead);
									nr++;
								}
								if ( i < (int) d ) {
									data[inx] = read_ahead(ahead);
									nr++;
								}
							}
							x += d;

							if ( nr & 1 )
								read_ahead(ahead); /* Align input stream to next word */
							}
							break;
					}
				}

				if ( destroy_ahead(ahead))
					return false;
			}
			break;
		case BCA_BITFIELDS:
			switch ( l-> bpp) {
			case 16:
				if ( !read_16_32_bpp(fi, PImage(fi-> object), 16, cLinesWorth))
					return false;
				break;
			case 32:
				if ( !read_16_32_bpp(fi, PImage(fi-> object), 32, cLinesWorth))
					return false;
				break;
			default:
				outcd("Unsupported bit depth %d, expected 16 or 32", l-> bpp);
			}
			break;
		default:
			outc("compression type not uncompressed, RLE4 or RLE8");
		}
	} else {
		/* OS/2 */
		if ( req_seek(fd, l-> offBits, SEEK_SET) < 0)
				outs(fd);
		if ( !req_read_big(fi, cLinesWorth, l->h, data))
				return false;
	}
	EVENT_SCANLINES_FINISHED(fi, SCANLINES_DIR_BOTTOM_TO_TOP);

	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	free( l);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( OS2, 0);
	pset_i( HotSpotX, 0);
	pset_i( HotSpotY, 0);
	pset_i( ImportantColors, 0);
	pset_i( XResolution, 0);
	pset_i( YResolution, 0);
	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return (void*)1;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	PImage i = ( PImage) fi-> object;
	HV * profile = fi-> extras;
	int cRGB;
	PImgIORequest fd = fi-> req;
	RGBColor rgb_1bpp[2], * palette = i-> palette;

	Bool os2	= false;
	word xHotspot	= 0;
	word yHotspot	= 0;
	dword cclrImportant = 0;
	dword cxResolution  = 0;
	dword cyResolution  = 0;

	if ( pexist( OS2))
		os2 = pget_B( OS2);
	if ( pexist( HotSpotX))
		xHotspot = pget_i( HotSpotX);
	if ( pexist( HotSpotY))
		yHotspot = pget_i( HotSpotY);
	if ( pexist( ImportantColors))
		cclrImportant = pget_i( ImportantColors);
	if ( pexist( XResolution))
		cxResolution = pget_i( XResolution);
	if ( pexist( YResolution))
		cyResolution = pget_i( YResolution);

	cRGB = ( (1 << (i-> type & imBPP)) & 0x1ff ); /* 1->2, 4->16, 8->256, 24->0 */

	/* ...handle messy 1bpp case */
	if ( cRGB == 2 ) {
		/*
		The palette entries inside a 1bpp PM bitmap are not honored, or handled
		correctly by most programs. Current thinking is that they have no actual
		meaning. Under OS/2 PM, bitmap 1's re fg and 0's are bg, and it is the job of
		the displayer to pick fg and bg. We will pick fg=black, bg=white in the bitmap
		file we save. If we do not write black and white, we find that most programs
		will incorrectly honor these entries giving unpredictable (and often black on
		a black background!) results.
		*/

		/* DK adds: since we swap OS/2 mono palette on load, we do the same
		on save. The reason is that if the programmed doesn't care about OS/2
		problems (which is most probable), we simply restore the status quo.
		If he does (surprise!) then he can also swap the palette manually
		*/
		memcpy( &rgb_1bpp, palette, sizeof(RGBColor) * 2);
		swap_pal( rgb_1bpp);
		palette = rgb_1bpp;
	}

	if ( os2 ) {
		word usType = BFT_BMAP;
		dword cbFix     = (dword) 12;
		word cx	    = (word) i->w;
		word cy	    = (word) i->h;
		word cPlanes    = (word) 1;
		word cBitCount  = (word) i->type & imBPP;
		    int cLinesWorth = (((cBitCount * cx + 31) / 32) * cPlanes) * 4;
		dword offBits   = (dword) 26 + cRGB * (dword) 3;
		    dword cbSize    = offBits + (dword) cy * (dword) cLinesWorth;
		int j, total, actual;

		write_word(fd, usType);
		write_dword(fd, cbSize);
		write_word(fd, xHotspot);
		write_word(fd, yHotspot);
		write_dword(fd, offBits);
		write_dword(fd, cbFix);
		write_word(fd, cx);
		write_word(fd, cy);
		write_word(fd, cPlanes);
		write_word(fd, cBitCount);

		for ( j = 0; j < cRGB; j++, palette++ ) {
			byte b[3];
			b[0] = palette-> b;
			b[1] = palette-> g;
			b[2] = palette-> r;
			if ( req_write(fd, 3, b) != 3 )
				outw(fd);
		}

		total = i->h * cLinesWorth;
		actual = req_write(fd, total, i-> data);
		if ( actual != total )
				outw(fd);
	} else {
		/* Windows */
		word usType		= BFT_BMAP;
		dword cbFix		= (dword) 40;
		dword cx		= (dword) i->w;
		dword cy		= (dword) i->h;
		word cPlanes	        = (word) 1;
		word cBitCount	        = (word) i->type & imBPP;
		int cLinesWorth	        = (((cBitCount * (int) cx + 31) / 32) * cPlanes) * 4;
		dword offBits	        = (dword) 54 + cRGB * (dword) 4;
		dword cbSize	        = offBits + (dword) cy * (dword) cLinesWorth;
		dword ulCompression     = BCA_UNCOMP;
		dword cbImage	        = (dword) cLinesWorth * (dword) i->h;
		dword cclrUsed	        = i-> palSize;
		int j, total, actual;

		write_word(fd, usType);
		write_dword(fd, cbSize);
		write_word(fd, xHotspot);
		write_word(fd, yHotspot);
		write_dword(fd, offBits);

		write_dword(fd, cbFix);
		write_dword(fd, cx);
		write_dword(fd, cy);
		write_word(fd, cPlanes);
		write_word(fd, cBitCount);
		write_dword(fd, ulCompression);
		write_dword(fd, cbImage);
		write_dword(fd, cxResolution);
		write_dword(fd, cyResolution);
		write_dword(fd, cclrUsed);
		write_dword(fd, cclrImportant);

		for ( j = 0; j < cRGB; j++, palette++ ) {
			byte b[4];

			b[0] = palette-> b;
			b[1] = palette-> g;
			b[2] = palette-> r;
			b[3] = 0;
			if ( req_write(fd, 4, b) != 4 )
				outw(fd);
		}

		total = i-> h * cLinesWorth;
		actual = req_write(fd, total, i-> data);
		if ( actual != total )
			outw(fd);
	}

	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
}

void
apc_img_codec_bmp( void )
{
	struct ImgCodecVMT vmt;
	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
	vmt. init	   = init;
	vmt. open_load	   = open_load;
	vmt. load	   = load;
	vmt. close_load    = close_load;
	vmt. save_defaults = save_defaults;
	vmt. open_save	   = open_save;
	vmt. save	   = save;
	vmt. close_save    = close_save;
	apc_img_register( &vmt, NULL);
}

#ifdef __cplusplus
}
#endif
