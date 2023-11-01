#include "img.h"

#include <stdio.h>
#include <ctype.h>
#define X_PROTOCOL              11
#define _Xconst
#define False                    0
#define True                     1
#define BitmapSuccess		0
#define BitmapOpenFailed 	1
#define BitmapFileInvalid 	2
#define BitmapNoMemory		3
#undef  RETURN
#undef __UNIXOS2__
#define Xmalloc                 malloc
#define Xfree                   free

static int
XReadBitmapFileData (
	_Xconst char *filename,
	int is_utf8,
	unsigned int *width,                /* RETURNED */
	unsigned int *height,               /* RETURNED */
	unsigned char **data,               /* RETURNED */
	int *x_hot,                         /* RETURNED */
	int *y_hot)                         /* RETURNED */
	;

static int XFree(void* ptr);

#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static char * xbmext[] = { "xbm", NULL };
static int    xbmbpp[] = { imbpp1 | imGrayScale, 0 };

static char * loadOutput[] = {
	"hotSpotX",
	"hotSpotY",
	NULL
};

static char * mime[] = {
	"image/xbm",
	"image/x-xbm",
	"image/x-bitmap",
	NULL
};

static ImgCodecInfo codec_info = {
	"X11 Bitmap",
	"X Consortium",
	X_PROTOCOL, 5,    /* version */
	xbmext,    /* extension */
	"X11 Bitmap File",     /* file type */
	"XBM", /* short type */
	NULL,    /* features  */
	"",     /* module */
	"",     /* package */
	IMG_LOAD_FROM_FILE | IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM,
	xbmbpp, /* save types */
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
	int w, h, yh, yw;
	Byte * data;
} LoadRec;

static Byte*
mirror_bits( void)
{
	static Bool initialized = false;
	static Byte bits[256];
	unsigned int i, j;
	int k;

	if (!initialized) {
		for ( i = 0; i < 256; i++) {
			bits[i] = 0;
			j = i;
			for ( k = 0; k < 8; k++) {
				bits[i] <<= 1;
				if ( j & 0x1)
					bits[i] |= 1;
				j >>= 1;
			}
		}
		initialized = true;
	}

	return bits;
}

static void
mirror_bytes( unsigned char *data, int dataSize)
{
	Byte *mirrored_bits = mirror_bits();
	while ( dataSize--) {
		*data = mirrored_bits[*data];
		data++;
	}
}

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	unsigned int w, h;
	int yh, yw;
	Byte * data;

	if( XReadBitmapFileData( fi-> io.fileName, fi-> io.is_utf8, &w, &h, &data, &yw, &yh) != BitmapSuccess)
		return NULL;

	fi-> stop = true;
	fi-> frameCount = 1;

	l = malloc( sizeof( LoadRec));
	if ( !l) return NULL;

	l-> w  = w;
	l-> h  = h;
	l-> yw = yw;
	l-> yh = yh;
	l-> data = data;

	return l;
}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	HV * profile = fi-> frameProperties;
	PImage i = ( PImage) fi-> object;
	int ls, h;
	Byte * src, * dst;

	if ( fi-> loadExtras) {
		pset_i( hotSpotX, l-> yw);
		pset_i( hotSpotY, l-> yh);
	}

	if ( fi-> noImageData) {
		CImage( fi-> object)-> set_type( fi-> object, imbpp1 | imGrayScale);
		pset_i( width,      l-> w);
		pset_i( height,     l-> h);
		return true;
	}

	CImage( fi-> object)-> create_empty( fi-> object, l-> w, l-> h, imbpp1 | imGrayScale);
	ls = ( l-> w >> 3) + (( l-> w & 7) ? 1 : 0);
	src = l-> data;
	h   = l-> h;
	dst = i-> data + ( l->h - 1 ) * i-> lineSize;
	while ( h--) {
		int w = ls;
		Byte * d = dst, * s = src;
		/* in order to comply with imGrayScale, revert bits
			rather that palette */
		while ( w--) *(d++) = ~ *(s++);
		src += ls;
		dst -= i-> lineSize;
	}
	mirror_bytes( i-> data, i-> dataSize);
	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	XFree( l-> data);
	free( fi-> instance);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( hotSpotX, 0);
	pset_i( hotSpotY, 0);
	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return (void*)1;
}

static void
myprintf( PImgIORequest req, const char *format, ...)
{
		int len;
		char buf[2048];
		va_list args;
		va_start( args, format);
		len = vsnprintf( buf, 2048, format, args);
		va_end( args);
		req_write( req, len, buf);
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	PImage i = ( PImage) fi-> object;
	Byte * l;
	int h = i-> h, col = -1;
	Byte * s = i-> data + ( h - 1) * i-> lineSize;
	char * xc = fi-> io.fileName, * name;
	int ls = ( i-> w >> 3) + (( i-> w & 7) ? 1 : 0);
	int first = 1;
	HV * profile = fi-> objectExtras;

	l = malloc( ls);
	if ( !l) return false;

	/* extracting name */
	if ( xc == NULL) xc = "xbm";
	name = xc;
	while ( *xc) {
		if ( *xc == '/')
			name = xc + 1;
		xc++;
	}
	xc = malloc( strlen( name) + 1);
	if ( xc) strcpy( xc, name);
	name = xc;

	while (*xc) {
		if ( *xc == '.') {
			*xc = 0;
			break;
		}
		xc++;
	}

	myprintf( fi-> req, "#define %s_width %d\n", name, i-> w);
	myprintf( fi-> req, "#define %s_height %d\n", name, i-> h);
	if ( pexist( hotSpotX))
		myprintf( fi-> req, "#define %s_x_hot %d\n", name, (int)pget_i( hotSpotX));
	if ( pexist( hotSpotY))
		myprintf( fi-> req, "#define %s_y_hot %d\n", name, (int)pget_i( hotSpotY));
	myprintf( fi-> req, "static char %s_bits[] = {\n  ", name);


	while ( h--) {
		Byte * s1 = l;
		int w = ls;

		memcpy( s1, s, ls);
		mirror_bytes( s1, ls);

		while ( w--) {
			if ( first) {
			first = 0;
			} else {
			myprintf( fi-> req, ", ");
			}
			if ( col++ == 11) {
				col = 0;
				myprintf( fi-> req, "\n  ");
			}
			myprintf( fi-> req, "0x%02x", (Byte)~(*(s1++)));
		}
		s -= i-> lineSize;
	}

	myprintf( fi-> req, "};\n");

	free( l);
	free( name);
	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
}

void
apc_img_codec_X11( void )
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

/* $Xorg: RdBitF.c,v 1.5 2001/02/09 02:03:35 xorgcvs Exp $ */
/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/lib/X11/RdBitF.c,v 3.6 2003/04/13 19:22:17 dawes Exp $ */

/*
*	Code to read bitmaps from disk files. Interprets
*	data from X10 and X11 bitmap files and creates
*	Pixmap representations of files. Returns Pixmap
*	ID and specifics about image.
*
*	Modified for speedup by Jim Becker, changed image
*	data parsing logic (removed some fscanf()s).
*	Aug 5, 1988
*
* Note that this file and ../Xmu/RdBitF.c look very similar....  Keep them
* that way (but don't use common source code so that people can have one
* without the other).
*/

#define MAX_SIZE 255

/* shared data for the image read/parse logic */
static short hexTable[256];		/* conversion value */
static Bool initialized = False;	/* easier to fill in at run time */


/*
*	Table index for the hex values. Initialized once, first time.
*	Used for translation value or delimiter significance lookup.
*/
static void initHexTable(void)
{
	/*
	* We build the table at run time for several reasons:
	*
	*     1.  portable to non-ASCII machines.
	*     2.  still reentrant since we set the init flag after setting table.
	*     3.  easier to extend.
	*     4.  less prone to bugs.
	*/
	hexTable['0'] = 0;	hexTable['1'] = 1;
	hexTable['2'] = 2;	hexTable['3'] = 3;
	hexTable['4'] = 4;	hexTable['5'] = 5;
	hexTable['6'] = 6;	hexTable['7'] = 7;
	hexTable['8'] = 8;	hexTable['9'] = 9;
	hexTable['A'] = 10;	hexTable['B'] = 11;
	hexTable['C'] = 12;	hexTable['D'] = 13;
	hexTable['E'] = 14;	hexTable['F'] = 15;
	hexTable['a'] = 10;	hexTable['b'] = 11;
	hexTable['c'] = 12;	hexTable['d'] = 13;
	hexTable['e'] = 14;	hexTable['f'] = 15;

	/* delimiters of significance are flagged w/ negative value */
	hexTable[' '] = -1;	hexTable[','] = -1;
	hexTable['}'] = -1;	hexTable['\n'] = -1;
	hexTable['\t'] = -1;

	initialized = True;
}

/*
*	read next hex value in the input stream, return -1 if EOF
*/
static int
NextInt (
	FILE *fstream)
{
	int	ch;
	int	value = 0;
	int gotone = 0;
	int done = 0;

	/* loop, accumulate hex value until find delimiter  */
	/* skip any initial delimiters found in read stream */

	while (!done) {
		ch = getc(fstream);
		if (ch == EOF) {
				value	= -1;
				done++;
		} else {
				/* trim high bits, check type and accumulate */
				ch &= 0xff;
				if (isascii(ch) && isxdigit(ch)) {
					value = (value << 4) + hexTable[ch];
					gotone++;
				} else if ((hexTable[ch]) < 0 && gotone)
				done++;
		}
	}
	return value;
}

int
XReadBitmapFileData (
	_Xconst char *filename,
	int is_utf8,
	unsigned int *width,                /* RETURNED */
	unsigned int *height,               /* RETURNED */
	unsigned char **data,               /* RETURNED */
	int *x_hot,                         /* RETURNED */
	int *y_hot)                         /* RETURNED */
{
	FILE *fstream;                      /* handle on file  */
	unsigned char *bits = NULL;         /* working variable */
	char line[MAX_SIZE];                /* input line from file */
	int size;                           /* number of bytes of data */
	char name_and_type[MAX_SIZE];       /* an input line */
	char *type;                         /* for parsing */
	int value;                          /* from an input line */
	int version10p;                     /* boolean, old format */
	int padding;                        /* to handle alignment */
	int bytes_per_line;                 /* per scanline of data */
	unsigned int ww = 0;                /* width */
	unsigned int hh = 0;                /* height */
	int hx = -1;                        /* x hotspot */
	int hy = -1;                        /* y hotspot */

	/* first time initialization */
	if (initialized == False) initHexTable();

	if (!(fstream = prima_open_file(filename, is_utf8, "r")))
		return BitmapOpenFailed;

	/* error cleanup and return macro	*/
#define	RETURN(code) \
{ if (bits) Xfree ((char *)bits); fclose (fstream); return code; }

	while (fgets(line, MAX_SIZE, fstream)) {
		if (strlen(line) == MAX_SIZE-1)
				RETURN (BitmapFileInvalid);
		if (sscanf(line,"#define %s %d",name_and_type,&value) == 2) {
			if (!(type = strrchr(name_and_type, '_')))
			type = name_and_type;
			else
			type++;

			if (!strcmp("width", type))
			ww = (unsigned int) value;
			if (!strcmp("height", type))
			hh = (unsigned int) value;
			if (!strcmp("hot", type)) {
				if (type-- == name_and_type || type-- == name_and_type)
					continue;
				if (!strcmp("x_hot", type))
					hx = value;
				if (!strcmp("y_hot", type))
					hy = value;
			}
			continue;
		}

		if (sscanf(line, "static short %s = {", name_and_type) == 1)
			version10p = 1;
		else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
			version10p = 0;
		else if (sscanf(line, "static char %s = {", name_and_type) == 1)
			version10p = 0;
		else
			continue;

		if (!(type = strrchr(name_and_type, '_')))
			type = name_and_type;
		else
			type++;

		if (strcmp("bits[]", type) != 0)
			continue;

		if (!ww || !hh)
			RETURN (BitmapFileInvalid);

		if ((ww % 16) && ((ww % 16) < 9) && version10p)
			padding = 1;
		else
			padding = 0;

		bytes_per_line = (ww+7)/8 + padding;

		size = bytes_per_line * hh;
		bits = (unsigned char *) Xmalloc ((unsigned int) size);
		if (!bits)
			RETURN (BitmapNoMemory);

		if (version10p) {
			unsigned char *ptr;
			int bytes;
			for (bytes=0, ptr=bits; bytes<size; (bytes += 2)) {
				if ((value = NextInt(fstream)) < 0)
					RETURN (BitmapFileInvalid);
				*(ptr++) = value;
				if (!padding || ((bytes+2) % bytes_per_line))
					*(ptr++) = value >> 8;
			}
		} else {
			unsigned char *ptr;
			int bytes;

			for (bytes=0, ptr=bits; bytes<size; bytes++, ptr++) {
				if ((value = NextInt(fstream)) < 0)
					RETURN (BitmapFileInvalid);
				*ptr=value;
			}
		}
	}					/* end while */

	fclose(fstream);
	if (!bits)
		return (BitmapFileInvalid);

	*data = bits;
	*width = ww;
	*height = hh;
	if (x_hot) *x_hot = hx;
	if (y_hot) *y_hot = hy;

	return (BitmapSuccess);
}

int XFree(void* ptr)
{
	free(ptr);
	return 0;
}

#ifdef __cplusplus
}
#endif
