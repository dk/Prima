/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Created by Vadim Belman <voland@plab.ku.dk>, April 1999.
 */

#ifdef __BORLANDC__
#define _POSIX_
#endif

#include "img_api.h"
#include "gif_support.h"
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#include <limits.h>

#ifndef F_OK
/*
 * Braindamadged Borland C++ doesn't have specification for access
 * mode constants. Perhaps, it is not the only case...
 */
#define F_OK 0x00
#endif

#define GIFPROP_WIDTH 0
#define GIFPROP_HEIGHT 1
#define GIFPROP_TYPE 2
#define GIFPROP_LINESIZE 3
#define GIFPROP_DATA 4
#define GIFPROP_PALETTE 5
#define GIFPROP_INDEX 6
#define GIFPROP_FORMAT 7
#define GIFPROP_INTERLACED 8
#define GIFPROP_X 9
#define GIFPROP_Y 10
#define GIFPROP_COLORRESOLUTION 11
#define GIFPROP_BGCOLOR 12
#define GIFPROP_PALETTESIZE 13
#define GIFPROP_PALETTEBPP 14
#define GIFPROP_NAME 15
#define GIFPROP_DISPOSALMETHOD 16
#define GIFPROP_USERINPUT 17
#define GIFPROP_TRANSPARENTCOLORINDEX 18
#define GIFPROP_EXTENSIONS 19
#define GIFPROP_DELAYTIME 20
#define GIFPROP_COMMENTS 21
#define GIFPROP_EXTENSION_CODE 22
#define GIFPROP_EXTENSION_BLOCKS 23
#define GIFPROP_OVERWRITE 24
#define GIFPROP_TOTAL 25

ImgFormat gifFormat;

#define GIFERR_MSGLEN           1024

#define GIFERRT_NONE            0
#define GIFERRT_LIBC            1
#define GIFERRT_GIFLIB          2
#define GIFERRT_DRIVER          3
static int errorType = GIFERRT_NONE;

/* Driver error codes */
#define DERR_NO_ERROR                           0
#define DERR_WRONG_PROPERTY_TYPE                1
#define DERR_NOT_ENOUGH_MEMORY                  2
#define DERR_INVALID_INDEX                      3
#define DERR_DATA_NOT_PRESENT                   4 /* No image data for queried index. */
#define DERR_NO_INDEX_PROP                      5 /* Internal: `index' property not found while filling in imgInfo. */
#define DERR_IMGPROP_FETCH                      6 /* Strange one: list_at( propList, ...) returned nil */
#define DERR_WRONG_ARG                          7 /* Wrong argument passed to a function. */
#define DERR_DUPLICATE_INDEX                    8 /* Images with the same indices passed for saving */

static int errorCode = DERR_NO_ERROR;

static int interlaceOffs[] = { 0, 4, 2, 1};
static int interlaceStep[] = { 8, 8, 4, 2};

typedef struct _GIFExtInfo {
    Bool processed; /* Informational flag for distinguishing between recognized and unrecognized GIF extensions. */
    int code;
    List data;
} GIFExtInfo, *PGIFExtInfo;

typedef struct _GIFExtensions {
    int index;
    List extensions;
} GIFExtensions, *PGIFExtensions;

#pragma pack(1)
typedef struct _GIFGraphControlExt {
    Byte blockSize;
    /* XXX NB: This bit-fields sequence requires big-endian bit order. */
    unsigned transparent:1;
    unsigned userInput:1;
    unsigned disposalMethod:3;
    unsigned reserved:3;
    U16 delayTime;
    Byte transparentColorIndex;
} GIFGraphControlExt, *PGIFGraphControlExt;
#pragma pack()

static void
__gif_seterror( int type, int code)
{
    errorType = type;
    errorCode = code;
}

static const char *
__gif_geterror( char *errorMsgBuf, int bufLen)
{
    static char errBuf[ GIFERR_MSGLEN];
    char *p = errBuf, *msg = "*** GIF error missed ***";
    int len = GIFERR_MSGLEN;

    if ( errorMsgBuf != NULL) {
	p = errorMsgBuf;
	len = bufLen;
    }

    switch ( errorType) {
	case GIFERRT_NONE:
	    msg = "no error";
	    break;
	case GIFERRT_LIBC:
	    msg = strerror( errorCode);
	    break;
	case GIFERRT_DRIVER:
	    switch ( errorCode) {
		case DERR_NO_ERROR:
		    msg = "no error";
		    break;
		case DERR_WRONG_PROPERTY_TYPE:
		    msg = "wrong property type";
		    break;
		case DERR_NOT_ENOUGH_MEMORY:
		    msg = "not enough memory";
		    break;
		case DERR_INVALID_INDEX:
		    msg= "invalid image index";
		    break;
		case DERR_DATA_NOT_PRESENT:
		    msg = "*** GIF internal: no image data for the index";
		    break;
		case DERR_NO_INDEX_PROP:
		    msg = "*** GIF internal: no `index' property found while filling in images list";
		    break;
		case DERR_WRONG_ARG:
		    msg = "*** GIF internal: wrong argument passed to a function";
		    break;
		case DERR_DUPLICATE_INDEX:
		    msg = "duplicate indices found";
		    break;
		default:
		    msg = "*** GIF internal: unknown error code";
	    }
	    break;
	case GIFERRT_GIFLIB:
	    switch ( errorCode) {
		case 0:
		    msg = "no error";
		    break;
		case D_GIF_ERR_OPEN_FAILED:
		case E_GIF_ERR_OPEN_FAILED:
		    msg = "open failed";
		    break;
		case D_GIF_ERR_READ_FAILED:
		    msg = "read failed";
		    break;
		case D_GIF_ERR_NOT_GIF_FILE:
		    msg = "not a valid GIF file";
		    break;
		case D_GIF_ERR_NO_SCRN_DSCR:
		    msg = "no screen description block in the file";
		    break;
		case D_GIF_ERR_NO_IMAG_DSCR:
		    msg = "no image description block in the file";
		    break;
		case D_GIF_ERR_NO_COLOR_MAP:
		    msg = "no color map in the file";
		    break;
		case D_GIF_ERR_WRONG_RECORD:
		    msg = "wrong record type";
		    break;
		case D_GIF_ERR_DATA_TOO_BIG:
		    msg = "queried data size is too big";
		    break;
		case D_GIF_ERR_NOT_ENOUGH_MEM:
		case E_GIF_ERR_NOT_ENOUGH_MEM:
		    msg = "not enough memory";
		    break;
		case D_GIF_ERR_CLOSE_FAILED:
		case E_GIF_ERR_CLOSE_FAILED:
		    msg = "close failed";
		    break;
		case D_GIF_ERR_NOT_READABLE:
		    msg = "file is not readable";
		    break;
		case D_GIF_ERR_IMAGE_DEFECT:
		    msg = "image defect detected";
		    break;
		case D_GIF_ERR_EOF_TOO_SOON:
		    msg = "unexpected end of file reached";
		    break;
		case E_GIF_ERR_WRITE_FAILED:
		    msg = "write failed";
		    break;
		case E_GIF_ERR_HAS_SCRN_DSCR:
		    msg = "screen descriptor already been set";
		    break;
		case E_GIF_ERR_HAS_IMAG_DSCR:
		    msg = "image descriptor is still active";
		    break;
		case E_GIF_ERR_NO_COLOR_MAP:
		    msg = "no color map specified";
		    break;
		case E_GIF_ERR_DATA_TOO_BIG:
		    msg = "data is too big (exceeds size of the image)";
		    break;
		case E_GIF_ERR_DISK_IS_FULL:
		    msg = "storage capcity exceeded";
		    break;
		case E_GIF_ERR_NOT_WRITEABLE:
		    msg = "file opened not for writing";
		    break;
		default:
		    msg = "unknown error";
	    }
	    break;
	default:
	    msg = "*** GIF internal: unknown error type";
    }
    strncpy( p, msg, len);
    p[ len - 1] = 0;

    return p;
}

static Bool
property_name( Handle item, void *params)
{
    PImgProperty imgProp = ( PImgProperty) item;
    const char *propName = ( const char *)params;
    return ( strcmp( imgProp->name, propName) == 0);
}

static Bool
__gif_read( int fd, const char *filename, PList imgInfo, Bool readData, Bool readAll)
{
    GifFileType *gif = nil;
    GifRecordType gifRecType;
    GifImageDesc *gifChunk = nil;
    GifPixelType **data = nil;
    int lastIndex = -1, lastQueriedIndex = -1;
    Bool succeed = true;
    int imgDescCount = 0;
    int i, gifrc;
    Bool done;
    List gifChunks, queriedIdx, imageData, imageExtensions;

    if ( readAll) {
	/* If have been queried for all images then there must be no
	   more than one image profile in the imgInfo list. If it
	   contains properties we must remove them. */
	img_clear_properties( ( ( PImgInfo) list_at( imgInfo, 0))->propList);
    }
    else {
	/* Have been queried for specific images, collecting necessary
           information. */
	list_create( &queriedIdx, imgInfo->count, 1);
	for ( i = 0; ( i < imgInfo->count) && succeed; i++) {
	    int indexPropIdx;
	    PImgProperty imgProp;
	    PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
	    indexPropIdx = list_first_that( imageInfo->propList, property_name, ( void*) "index");
	    if ( indexPropIdx >= 0) {
		imgProp = ( PImgProperty) list_at( imageInfo->propList, indexPropIdx);
		if ( imgProp == nil) {
		    __gif_seterror( GIFERRT_DRIVER, DERR_IMGPROP_FETCH);
		}
	    }
	    else {
		imgProp = img_info_add_property( imageInfo, "index", PROPTYPE_INT, -1, ++lastQueriedIndex);
		if ( imgProp == nil) {
		    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
		}
	    }
	    if ( imgProp == nil) {
		succeed = false;
		continue;
	    }
	    if ( ( imgProp->flags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY) {
		lastQueriedIndex = imgProp->val.Int;
		if ( imgProp->val.Int > lastIndex) {
		    lastIndex = lastQueriedIndex;
		}
		list_add( &queriedIdx, lastQueriedIndex);
	    }
	    else {
		/* ERROR: The property contains an array. */
		__gif_seterror( GIFERRT_DRIVER, DERR_WRONG_PROPERTY_TYPE);
		succeed = false;
		continue;
	    }
	}
    }

    if ( ! succeed) {
	return false;
    }

    gif = DGifOpenFileName( filename);
    if ( gif == NULL) {
	__gif_seterror( GIFERRT_GIFLIB, GifLastError());
	return false;
    }

    list_create( &gifChunks, readAll ? 1 : queriedIdx.count, readAll ? 5 : 1);
    list_create( &imageExtensions, readAll ? 1 : queriedIdx.count, readAll ? 5 : 1);
    if ( readData) {
	list_create( &imageData, readAll ? 1 : queriedIdx.count, readAll ? 5 : 1);
    }

    done = false;
    do {
	Byte *extData = nil;
	int extCode;

	gifrc = DGifGetRecordType( gif, &gifRecType);
	if ( gifrc != GIF_OK) {
	    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
	    done = true;
	    succeed = false;
	    break;
	}

	/* We must add an entry for every image in the file... */
	while ( gifChunks.count <= imgDescCount) {
	    gifChunk = malloc( sizeof( GifImageDesc));
	    gifChunk->ColorMap = NULL;
	    list_add( &gifChunks, ( Handle) gifChunk);
	}
	if ( readData) {
	    /* And the same for the image content. */
	    while ( imageData.count <= imgDescCount) {
		data = malloc( sizeof( GifPixelType *));
		*data = nil;
		list_add( &imageData, ( Handle) data);
	    }
	}

	switch ( gifRecType) {
	    case SCREEN_DESC_RECORD_TYPE:
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    case IMAGE_DESC_RECORD_TYPE:
		if ( ! readAll) {
		    if( ( done = ( imgDescCount > lastIndex))) {
			break;
		    }
		}
		gifrc = DGifGetImageDesc( gif);
		if ( gifrc != GIF_OK) {
		    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
		    succeed = false;
		}
		else {
		    int codeSize;
		    Byte *codeBlock = nil;
		    Bool idxQueried = readAll; /* Presume that we need all indexes... */

		    for ( i = 0; ( i < queriedIdx.count) && ( ! idxQueried); i++) {
			idxQueried = ( imgDescCount == list_at( &queriedIdx, i));
		    }

		    gifChunk = ( GifImageDesc *) list_at( &gifChunks, imgDescCount);
		    memcpy( ( void *) gifChunk, ( void *) &gif->Image, sizeof( GifImageDesc));
		    if ( ( gif->Image.ColorMap != NULL) && idxQueried) {
			gifChunk->ColorMap = MakeMapObject( gif->Image.ColorMap->ColorCount, gif->Image.ColorMap->Colors);
		    }
		    else {
			gifChunk->ColorMap = NULL;
		    }
		    if ( idxQueried && readData) {
			int pixelCount = gifChunk->Width * gifChunk->Height;
			data = ( GifPixelType **) list_at( &imageData, imgDescCount);
			*data = malloc( sizeof( GifPixelType) * pixelCount);
			if ( *data == nil) {
			    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			    succeed = false;
			}
			else {
			    gifrc = DGifGetLine( gif, *data, pixelCount);
			}
		    }
		    else {
			gifrc = DGifGetCode( gif, &codeSize, &codeBlock);
			DOLBUG( "Code size is %d\n", codeSize);
			if ( gifrc == GIF_OK) {
			    while( ( codeBlock != NULL) && ( gifrc == GIF_OK)) {
				gifrc = DGifGetCodeNext( gif, &codeBlock);
			    }
			}
		    }
		    if ( gifrc != GIF_OK) {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		    imgDescCount++;
		}
		break;
	    case EXTENSION_RECORD_TYPE:
		extCode = -1;
		gifrc = DGifGetExtension( gif, &extCode, &extData);
		if ( gifrc == GIF_OK) {
		    Bool idxQueried = readAll; /* Presume that we need all indexes... */
		    /* Graphic Control Extension belongs to the next image. */
		    /* All others, supposedly, to the current one. */
		    int curIndex = imgDescCount - ( extCode == GRAPHICS_EXT_FUNC_CODE ? 0 : 1);
		    PGIFExtensions gifExtensions;
		    PGIFExtInfo gifExtension = nil;

		    for ( i = 0; ( i < queriedIdx.count) && ( ! idxQueried); i++) {
			idxQueried = ( curIndex == list_at( &queriedIdx, i));
		    }

		    if ( idxQueried) {
			for ( i = 0; i < imageExtensions.count; i++) {
			    gifExtensions = ( PGIFExtensions) list_at( &imageExtensions, i);
			    if ( gifExtensions->index == curIndex) {
				break;
			    }
			}
			if ( i >= imageExtensions.count) {
			    gifExtensions = malloc( sizeof( GIFExtensions));
			    gifExtensions->index = curIndex;
			    list_create( &gifExtensions->extensions, 1, 1);
			    list_add( &imageExtensions, ( Handle) gifExtensions);
			}
			gifExtension = malloc( sizeof( GIFExtInfo));
			gifExtension->code = extCode;
			gifExtension->processed = false;
			list_create( &gifExtension->data, 1, 1);
			list_add( &gifExtensions->extensions, ( Handle) gifExtension);
		    }

		    do {
			if ( idxQueried) {
			    Byte *extDataCopy = malloc( extData[ 0] + 1);
			    memcpy( extDataCopy, extData, extData[ 0] + 1);
			    list_add( &gifExtension->data, ( Handle) extDataCopy);
			}
			gifrc = DGifGetExtensionNext( gif, &extData);
		    } while ( ( extData != NULL) && ( gifrc == GIF_OK));
		}
		if ( gifrc != GIF_OK) {
		    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
		    succeed = false;
		}
		break;
	    default:
		;
	}
	done = done || ( ! succeed);
    } while( ( gifRecType != TERMINATE_RECORD_TYPE) && ( ! done));

    if ( succeed) {

	if ( readAll) {
	    /* Now we have to create slots for all the images have been readed. */
	    /* The first slot is already here. We can just fill it in. */
	    /* For all subsequent images the first one will be used as a template. */
	    PImgInfo imageInfo;
	    PImgProperty imgProp;
	    PImgInfo firstImg = ( PImgInfo) list_at( imgInfo, 0);
	    succeed = ( imgProp = img_info_add_property( firstImg, "index", PROPTYPE_INT, -1, -1)) != nil;

	    for ( i = 0; ( i < imgDescCount) && succeed; i++) {
		if ( ( imageInfo = img_info_create( 15)) == nil) {
		    succeed = false;
		    continue;
		}
		imageInfo->extraInfo = firstImg->extraInfo;
		list_add( imgInfo, ( Handle) imageInfo);
		if ( ( imgProp = img_info_add_property( imageInfo, "index", PROPTYPE_INT, -1, i)) == nil) {
		    succeed = false;
		}
	    }
	    if ( ! succeed) {
		/* This is the only possible error at this point. */
		__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
	    }
	}

	/* At this stage we must have fully prepared imgInfo list in which all the items
	   already contains valid `index' properties. */
	for ( i = 0; ( i < imgInfo->count) && succeed; i++) {
	    int indexPropIdx = -1, j, index = -1;
	    PImgInfo imageInfo;
	    PImgProperty imgProp;

	    imageInfo = ( PImgInfo) list_at( imgInfo, i);
	    indexPropIdx = list_first_that( imageInfo->propList, property_name, ( void *) "index");
	    if ( indexPropIdx >= 0) {
		index = ( ( PImgProperty) list_at( imageInfo->propList, indexPropIdx))->val.Int;
	    }
	    else {
		__gif_seterror( GIFERRT_DRIVER, DERR_NO_INDEX_PROP);
		succeed = false;
		continue;
	    }
	    /* At this point everything is ready to fill in image
               information with a real data. So, all old properties
               are obsolete. */
	    img_clear_properties( imageInfo->propList);

	    gifChunk = ( GifImageDesc *) list_at( &gifChunks, index);
	    if ( readData) {
		data = ( GifPixelType **) list_at( &imageData, index);
	    }

	    imgProp = img_info_add_property( imageInfo, "index", PROPTYPE_INT, -1, index);
	    if ( imgProp != nil) imgProp = img_info_add_property( imageInfo, "format", PROPTYPE_STRING, -1, gifFormat.id);
	    if ( imgProp != nil) imgProp = img_info_add_property( imageInfo, "type", PROPTYPE_INT, -1, im256);
	    if ( imgProp == nil) {
		succeed = false;
		__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
		continue;
	    }

	    for ( j = 0; ( j < imageExtensions.count) && succeed; j++) {
		PGIFExtensions gifExtensions;
		gifExtensions = ( PGIFExtensions) list_at( &imageExtensions, j);
		if ( gifExtensions->index == index) {
		    PGIFExtInfo gifExtension;
		    int k, extIdx;
		    int processedCount = 0;

		    for ( k = 0; ( k < gifExtensions->extensions.count) && succeed; k++) {
			gifExtension = ( PGIFExtInfo) list_at( &gifExtensions->extensions, k);
			switch ( gifExtension->code) {
			    case GRAPHICS_EXT_FUNC_CODE:
				{
				    /* There cannot be more than one data block for this kind of extension. */
				    PGIFGraphControlExt controlData = ( PGIFGraphControlExt) list_at( &gifExtension->data, 0);

				    imgProp = img_info_add_property( imageInfo, "disposalMethod", PROPTYPE_BYTE, -1, controlData->disposalMethod);
				    if ( imgProp != NULL) {
					imgProp = img_info_add_property( imageInfo, "userInput", PROPTYPE_BYTE, -1, controlData->userInput);
				    }
				    if ( imgProp != NULL) {
					imgProp = img_info_add_property( imageInfo, "delayTime", PROPTYPE_INT, -1, controlData->delayTime * 10);
				    }
				    if ( imgProp != NULL) {
					if ( controlData->transparent) {
					    imgProp = img_info_add_property( imageInfo, "transparentColorIndex", PROPTYPE_BYTE, -1, controlData->transparentColorIndex);
					}
				    }
				    if ( imgProp != nil) {
					gifExtension->processed = true;
				    }
				    else {
					succeed = false;
					__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
					continue;
				    }
				}
				break;
			    case COMMENT_EXT_FUNC_CODE:
				{
				    PImgProperty imgProp;
				    int commentsPropIdx = list_first_that( imageInfo->propList, ( void *) property_name, "comments");
				    if ( commentsPropIdx == -1) {
					if ( ( imgProp = img_info_add_property( imageInfo,
										"comments",
										PROPTYPE_STRING | PROPTYPE_ARRAY,
										1)) == nil) {
					    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
					}
				    }
				    else {
					if ( ( imgProp = ( PImgProperty) list_at( imageInfo->propList, commentsPropIdx)) == nil) {
					    __gif_seterror( GIFERRT_DRIVER, DERR_IMGPROP_FETCH);
					}
				    }
				    if ( ( succeed = ( imgProp != nil))) {
					Byte *commentData = ( Byte *) list_at( &gifExtension->data, 0);
					int dataLen = commentData[ 0];
					/* Dirty hack... Making a C-style string from a
                                           Pascal-style one. 8) */
					memmove( commentData, commentData + 1, dataLen);
					commentData[ dataLen] = 0;
					img_push_property_value( imgProp, commentData);
				    }
				    if ( succeed) {
					gifExtension->processed = true;
				    }
				}
				break;
			}
			if ( gifExtension->processed) {
			    processedCount++;
			}
		    }

		    if ( ! succeed) {
			continue;
		    }

		    imgProp = nil;

		    for ( k = 0; ( k < gifExtensions->extensions.count) && succeed; k++) {
			int n;

			gifExtension = ( PGIFExtInfo) list_at( &gifExtensions->extensions, k);
			if ( gifExtension->processed) {
			    continue;
			}

			if ( imgProp == nil) {
			    imgProp = img_info_add_property(
				    imageInfo,
				    "extensions",
				    PROPTYPE_PROP | PROPTYPE_ARRAY,
				    gifExtensions->extensions.count - processedCount
				);
			}
			if ( imgProp == nil) {
			    succeed = false;
			    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			    continue;
			}

			/* Then prepare a new property list in the property list array. */
			/* Current imgProp->used value is an index for the new array entry. */
			extIdx = imgProp->used;
			if ( ( succeed = img_push_property_value( imgProp, gifExtension->data.count + 1))) {
			    PImgProperty extProp = nil;
			    succeed = img_push_property(
				    imgProp->val.pProperties + extIdx,
				    "code",
				    PROPTYPE_BYTE,
				    -1,
				    gifExtension->code
				) != nil;
			    if ( succeed) {
				extProp = img_push_property(
					imgProp->val.pProperties + extIdx,
					"blocks",
					PROPTYPE_BIN | PROPTYPE_ARRAY,
					gifExtension->data.count
				    );
			    }
			    for ( n = 0; ( n < gifExtension->data.count) && succeed; n++) {
				Byte *extData = ( Byte *) list_at( &gifExtension->data, n);
				succeed = img_push_property_value(
					extProp,
					( int) extData[ 0],
					extData + 1
				    );
			    }
			}
			if ( ! succeed) {
			    /* Again, the only possible error. */
			    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			}
		    }
		    break; /* extensions scanning loop */
		}
	    }

	    if ( ! succeed) {
		continue;
	    }

	    if ( readData) {
		char buf[ PATH_MAX + 16];
		snprintf( buf, PATH_MAX + 16, "%s:%d", filename, index);
		if ( ! ( succeed = img_info_add_property( imageInfo, "name", PROPTYPE_STRING, -1, buf) != nil)) {
		    continue;
		}
	    }

	    if ( index == -1) {
		imgProp = img_info_add_property( imageInfo, "width", PROPTYPE_INT, -1, gif->SWidth);
		if ( imgProp != nil) {
		    imgProp = img_info_add_property( imageInfo, "height", PROPTYPE_INT, -1, gif->SHeight);
		}
		if ( imgProp != nil) {
		    imgProp = img_info_add_property( imageInfo, "colorResolution", PROPTYPE_INT, -1, gif->SColorResolution);
		}
		if ( imgProp != nil) {
		    imgProp = img_info_add_property( imageInfo, "bgColor", PROPTYPE_INT, -1, gif->SBackGroundColor);
		}
		if ( ! ( succeed = ( imgProp != nil))) {
		    continue;
		}

		if ( gif->SColorMap != NULL) {
		    int n;

		    imgProp = img_info_add_property( imageInfo, "paletteSize", PROPTYPE_INT, -1, gif->SColorMap->ColorCount);
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "paletteBPP", PROPTYPE_INT, -1, gif->SColorMap->BitsPerPixel);
		    }
		    if ( imgProp != nil) {
			succeed =
			    ( imgProp = img_info_add_property( imageInfo,
							       "palette",
							       PROPTYPE_BYTE | PROPTYPE_ARRAY,
							       3 * gif->SColorMap->ColorCount
				)) != nil;
			for ( n = 0; ( n < gif->SColorMap->ColorCount) && succeed; n++) {
			    succeed =
				img_push_property_value( imgProp, gif->SColorMap->Colors[ n].Blue)
				&& img_push_property_value( imgProp, gif->SColorMap->Colors[ n].Green)
				&& img_push_property_value( imgProp, gif->SColorMap->Colors[ n].Red);
			}
		    }
		    if ( ! succeed) {
			continue;
		    }
		}

		if ( readData) {
		    /* New array property is always zeroed. */
		    imgProp = img_info_add_property(
			    imageInfo,
			    "data",
			    PROPTYPE_BYTE | PROPTYPE_ARRAY,
			    gif->SWidth * gif->SHeight
			);
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "lineSize", PROPTYPE_INT, -1, gif->SWidth);
		    }
		    succeed = imgProp != nil;
		}
	    }
	    else if ( index >= 0) {
		ColorMapObject *palette = nil;

		if ( index < imgDescCount) {
		    int n;

		    imgProp = img_info_add_property( imageInfo, "width", PROPTYPE_INT, -1, gifChunk->Width);
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "height", PROPTYPE_INT, -1, gifChunk->Height);
		    }
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "type", PROPTYPE_INT, -1, im256);
		    }
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "XOffset", PROPTYPE_INT, -1, gifChunk->Left);
		    }
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "YOffset", PROPTYPE_INT, -1, gifChunk->Top);
		    }
		    if ( imgProp != nil) {
			imgProp = img_info_add_property( imageInfo, "interlaced", PROPTYPE_INT, -1, gifChunk->Interlace ? 1 : 0);
		    }
		    if ( imgProp != nil) {
			palette = ( gifChunk->ColorMap != NULL ?
				    gifChunk->ColorMap :
				    gif->SColorMap);
			succeed = ( imgProp = img_info_add_property( imageInfo,
								     "paletteSize",
								     PROPTYPE_INT,
								     -1,
								     palette->ColorCount
			    )) != nil;
/*  			if ( succeed) { */
/*  			    DOLBUG( "GIF palette size: %d from %d\n", imgProp->val.Int, palette->ColorCount); */
/*  			} */
/*  			else { */
/*  			    DOLBUG( "GIF SHIT HAPPENED\n"); */
/*  			} */
			if ( succeed) {
			    succeed = img_info_add_property( imageInfo,
							     "paletteBPP",
							     PROPTYPE_INT,
							     -1,
							     palette->BitsPerPixel
				) != nil;
			}
			if ( succeed) {
			    succeed = ( imgProp = img_info_add_property( imageInfo,
									 "palette",
									 PROPTYPE_BYTE | PROPTYPE_ARRAY,
									 3 * palette->ColorCount
				)) != nil;
			}
			for ( n = 0; ( n < palette->ColorCount) && succeed; n++) {
			    succeed =
				img_push_property_value( imgProp, palette->Colors[ n].Blue)
				&& img_push_property_value( imgProp, palette->Colors[ n].Green)
				&& img_push_property_value( imgProp, palette->Colors[ n].Red);
			}
		    }

		    if ( ! succeed) {
			__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			continue;
		    }

		    if ( readData) {
			if ( *data != nil) {
			    int pixelCount = gifChunk->Width * gifChunk->Height;
			    int pass = 0, Y; /* Pass number for interlaced images. */
			    imgProp = img_info_add_property(
				    imageInfo,
				    "data",
				    PROPTYPE_BYTE | PROPTYPE_ARRAY,
				    pixelCount
				);
			    if ( imgProp != nil) {
				int ySrc, yDst;
				for ( ySrc = 0, yDst = 0, Y = 0;
				      ySrc < pixelCount;
				      ySrc += gifChunk->Width) {
				    memcpy( imgProp->val.pByte + pixelCount - yDst - gifChunk->Width,
					    *data + ySrc,
					    gifChunk->Width
					);
				    if ( gifChunk->Interlace) {
					yDst += gifChunk->Width * interlaceStep[ pass];
					Y += interlaceStep[ pass];
					if ( yDst >= pixelCount) {
					    pass++;
					    Y = interlaceOffs[ pass];
					    yDst = gifChunk->Width * interlaceOffs[ pass];
					}
				    }
				    else {
					yDst += gifChunk->Width;
				    }
				}
			    }
			    if ( imgProp != nil) {
				imgProp = img_info_add_property( imageInfo, "lineSize", PROPTYPE_INT, -1, gifChunk->Width);
			    }
			    if ( ! ( succeed = imgProp != nil)) {
				__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			    }
			}
			else {
			    __gif_seterror( GIFERRT_DRIVER, DERR_DATA_NOT_PRESENT);
			    succeed = false;
			}
		    }
		}
	    }
	    else {
		__gif_seterror( GIFERRT_DRIVER, DERR_INVALID_INDEX);
		succeed = false;
	    }
	}
    }

    gifrc = DGifCloseFile( gif);
    if ( ( gifrc != GIF_OK) && succeed) {
	__gif_seterror( GIFERRT_GIFLIB, GifLastError());
	succeed = false;
    }

    for ( i = 0; i < gifChunks.count; i++) {
	gifChunk = ( GifImageDesc *) list_at( &gifChunks, i);
	if ( gifChunk->ColorMap != NULL) {
	    FreeMapObject( gifChunk->ColorMap);
	}
	free( gifChunk);

	if ( readData) {
	    data = ( GifPixelType **) list_at( &imageData, i);
	    if ( data) {
		if ( *data) {
		    free( *data);
		}
		free( data);
	    }
	}
    }
    list_destroy( &gifChunks);
    if ( readData) {
	list_destroy( &imageData);
    }
    for ( i = 0; i < imageExtensions.count; i++) {
	int j;
	PGIFExtensions gifExtensions = ( PGIFExtensions) list_at( &imageExtensions, i);
	for ( j = 0; j < gifExtensions->extensions.count; j++) {
	    int k;
	    PGIFExtInfo gifExtension = ( PGIFExtInfo) list_at( &gifExtensions->extensions, j);
	    for ( k = 0; k < gifExtension->data.count; k++) {
		free( ( Byte *) list_at( &gifExtension->data, k));
	    }
	    list_destroy( &gifExtension->data);
	    free( gifExtension);
	}
	list_destroy( &gifExtensions->extensions);
	free( gifExtensions);
    }
    list_destroy( &imageExtensions);
    if ( ! readAll) {
	list_destroy( &queriedIdx);
    }

    return succeed;
}

static Bool
__gif_load( int fd, const char *filename, PList imgInfo, Bool readAll)
{
    return __gif_read( fd, filename, imgInfo, true, readAll);
}

static Bool
__gif_loadable( int fd, const char *filename, Byte *preread_buf, U32 buf_size)
{
    return ( strncmp( "GIF", preread_buf, 3) == 0);
}

static Bool
__gif_storable( const char *filename, PList imgInfo)
{
    if ( filename == NULL) {
	__gif_seterror( GIFERRT_DRIVER, DERR_WRONG_ARG);
	return false;
    }
    return ( strcasecmp( filename + strlen( filename) - 4, ".gif") == 0);
}

static Bool
__gif_compatible( PList imgInfo)
{
    Bool rc = true;
    int i;

    for ( i = 0; ( i < imgInfo->count) && rc; i++) {
	PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
	int j;

	for ( j = 0; ( j < imageInfo->propList->count) && rc; j++) {
	    PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, j);
	    if ( imgProp->id == GIFPROP_TYPE) {
		rc = ( ( imgProp->val.Int & imBPP) == imbpp8);
	    }
	}
    }

    return rc;
}

static Bool
__gif_getinfo( int fd, const char *filename, PList imgInfo, Bool readAll)
{
    return __gif_read( fd, filename, imgInfo, false, readAll);
}

typedef
struct _GIFImgPreSave {
    int index;
    PImgProperty *imgProp;
} GIFImgPreSave, *PGIFImgPreSave;

/* qsort compare function */
static int
_index_compare( const void *x1, const void *x2)
{
    return ( ( PGIFImgPreSave) x1)->index - ( ( PGIFImgPreSave) x2)->index;
}

static ColorMapObject *
palette2colormap( int paletteSize, Byte *palette)
{
    ColorMapObject *cmap = ( ColorMapObject *) malloc( sizeof( ColorMapObject));
    if ( cmap != NULL) {
	cmap->Colors = ( GifColorType *) malloc( sizeof( GifColorType) * paletteSize);
	if ( cmap->Colors != NULL) {
	    int i, j;
/*  	    cmap->BitsPerPixel = 0; */
/*  	    for ( n = ( paletteSize >> 1); n != 0; n >>= 1) { */
/*  		cmap->BitsPerPixel++; */
/*  	    } */
	    cmap->ColorCount = paletteSize;
	    cmap->BitsPerPixel = BitSize( paletteSize);
	    for ( i = 0, j = 0; i < paletteSize; i++, j += 3) {
		cmap->Colors[ i].Blue  = palette[ j];
		cmap->Colors[ i].Green = palette[ j + 1];
		cmap->Colors[ i].Red   = palette[ j + 2];
	    }
	}
	else {
	    free( cmap);
	    cmap = nil;
	}
    }
    return cmap;
}

static Bool
__gif_save( const char *filename, PList imgInfo)
{
    Bool fileExists = false;
    Bool overwrite = true;
    Bool succeed = true;
    Bool igif_terminated;
    const char *fname;
    char tmpname[ PATH_MAX + 1];
    PGIFImgPreSave imgList;
    int i, j;
    int lastIndex = -1, index;
    GifFileType *ogif = nil, *igif = nil;
    GifRecordType gifRecType;

    DOLBUG( "Processing %d image profiles\n", imgInfo->count);

    /* Preparing slots for collecting & sorting images properties */
    imgList = ( PGIFImgPreSave) malloc( sizeof( GIFImgPreSave) * imgInfo->count);
    for ( i = 0; i < imgInfo->count; i++) {
	PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
	imgList[ i].imgProp = ( PImgProperty *) malloc( sizeof( PImgProperty) * GIFPROP_TOTAL);

	for ( j = 0; j < GIFPROP_TOTAL; j++) {
	    imgList[ i].imgProp[ j] = nil;
	}

	for ( j = 0; j < imageInfo->propList->count; j++) {
	    PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, j);
	    imgList[ i].imgProp[ imgProp->id] = imgProp;
	    switch ( imgProp->id) {
		case GIFPROP_INDEX:
		    lastIndex = imgProp->val.Int;
		    break;
		case GIFPROP_OVERWRITE:
		    overwrite = imgProp->val.Int != 0;
		    break;
	    }
	}

	imgList[ i].index = lastIndex++;
    }

    qsort( imgList, imgInfo->count, sizeof( GIFImgPreSave), _index_compare);

    /* We do not allow duplicate indices */
    for ( i = 0; i < ( imgInfo->count - 1); i++) {
	if ( imgList[ i].index < -1) {
	    succeed = false;
	    __gif_seterror( GIFERRT_DRIVER, DERR_INVALID_INDEX);
	}
	if ( imgList[ i].index == imgList[ i + 1].index) {
	    succeed = false;
	    __gif_seterror( GIFERRT_DRIVER, DERR_DUPLICATE_INDEX);
	}
    }

    EGifSetGifVersion( "89a");
    DOLBUG( "Set GIF version\n");

    if ( succeed) {
	if ( ( ! overwrite) && ( access( filename, F_OK) == 0)) {
	    /* We will merge existing file with new data. */
	    char *p, tmpdir[ PATH_MAX];
	    int num = 0;

	    DOLBUG( "%s will not be overwritten\n", filename);
	    fileExists = true;

	    /* First we have to prepare a temporary file name.  */
	    strncpy( tmpdir, filename, PATH_MAX);
	    tmpdir[ PATH_MAX] = 0;
	    if ( ( p = strrchr( tmpdir, '/')) != NULL) {
		p[ 1] = 0; /* I.e. we keep trailing / in directory name. */
	    }
	    else {
		tmpdir[ 0] = 0; /* I.e. we will have empty directory name. */
	    }
	    do {
		snprintf( tmpname, PATH_MAX, "%s%d.gif", tmpdir, num++);
	    } while ( access( tmpname, F_OK) == 0);

	    fname = tmpname;
	}
	else {
	    fname = filename;
	}
	DOLBUG( "Using %s for output\n", fname);
    }

    if ( succeed) {
	ogif = EGifOpenFileName( ( char *) fname, 0);
	if ( ogif == NULL) {
	    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
	    succeed = false;
	}
	DOLBUG( "Opened %s for writing\n", fname);
    }

    if ( succeed && fileExists) {
	igif = DGifOpenFileName( filename);
	if ( igif == NULL) {
	    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
	    succeed = false;
	}
	igif_terminated = false;
    }
    else {
	/* Sure, it has never been opened... */
	igif_terminated = true;
    }

    if ( succeed) {
	int sWidth, sHeight, sColorRes, sBgColor;
	ColorMapObject *sColorMap = NULL;
	if ( imgList[ 0].index == -1) {
	    sWidth = imgList[ 0].imgProp[ GIFPROP_WIDTH]->val.Int;
	    sHeight = imgList[ 0].imgProp[ GIFPROP_HEIGHT]->val.Int;
	}
	else if ( fileExists) {
	    sWidth = igif->SWidth;
	    sHeight = igif->SHeight;
	}
	else {
	    sWidth = sHeight = 1;
	    for ( i = 0; i < imgInfo->count; i++) {
		if ( imgList[ 0].imgProp[ GIFPROP_WIDTH]->val.Int > sWidth) {
		    sWidth = imgList[ 0].imgProp[ GIFPROP_WIDTH]->val.Int;
		}
		if ( imgList[ 0].imgProp[ GIFPROP_HEIGHT]->val.Int > sHeight) {
		    sHeight = imgList[ 0].imgProp[ GIFPROP_HEIGHT]->val.Int;
		}
	    }
	}
	if ( ( imgList[ 0].index != -1) && fileExists) {
	    sColorRes = igif->SColorResolution;
	    sBgColor = igif->SBackGroundColor;
	    sColorMap = MakeMapObject( igif->SColorMap->ColorCount, igif->SColorMap->Colors);
	}
	else {
	    sColorRes = imgList[ 0].imgProp[ GIFPROP_COLORRESOLUTION] != nil  ?
		imgList[ 0].imgProp[ GIFPROP_COLORRESOLUTION]->val.Int :
		imgList[ 0].imgProp[ GIFPROP_PALETTESIZE]->val.Int;
	    sBgColor = imgList[ 0].imgProp[ GIFPROP_BGCOLOR] != nil ? imgList[ 0].imgProp[ GIFPROP_BGCOLOR]->val.Int : 0;
	    /* Actually, it shouldn't be necessary to have a global color map, but giflib
               has a bug which makes it mandatory. */
	    sColorMap = palette2colormap( imgList[ 0].imgProp[ GIFPROP_PALETTESIZE]->val.Int,
					  imgList[ 0].imgProp[ GIFPROP_PALETTE]->val.pByte
		);
	}
	DOLBUG( "sColorMap: %d colors, %d bpp, colorRes: %d\n",
		sColorMap ? sColorMap->ColorCount : -1,
		sColorMap ? sColorMap->BitsPerPixel : -1,
		sColorRes);
	if ( ! ( succeed = EGifPutScreenDesc( ogif,
					      sWidth,
					      sHeight,
					      sColorRes,
					      sBgColor,
					      sColorMap) == GIF_OK)) {
	    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
	}
	DOLBUG( "Put screen desc\n");
	FreeMapObject( sColorMap);
    }

    if ( succeed) {
	int extCode, codeSize;
	Byte *extData = nil;
	Byte *codeBlock;
	int gifrc = GIF_OK;
	lastIndex = -2; /* Here it acts as index of last image been read from igif. */
	index = -1;
	i = 0;
	if ( igif) {
	    /* This call is necessary because later we will always stay on a record
               beginning. */
	    gifrc = DGifGetRecordType( igif, &gifRecType);
	    if ( gifrc != GIF_OK) {
		__gif_seterror( GIFERRT_GIFLIB, GifLastError());
		succeed = false;
	    }
	    DOLBUG( "Got record type %d, gifrc == %d\n", gifRecType, gifrc);
	}
	while ( succeed && ( ( i < imgInfo->count) || ! igif_terminated)) {
	    DOLBUG( "imgList[ %d].index = %d, index=%d\n", i, imgList[ i].index, index);
	    if ( imgList[ i].index == index) {
		int xOffset, yOffset, interlaced;
		if ( ( imgList[ i].imgProp[ GIFPROP_TRANSPARENTCOLORINDEX] != nil)
		     || ( imgList[ i].imgProp[ GIFPROP_USERINPUT] != nil)
		     || ( imgList[ i].imgProp[ GIFPROP_DISPOSALMETHOD] != nil)
		     || ( imgList[ i].imgProp[ GIFPROP_DELAYTIME] != nil)) {
		    /* So, we have data for Graphics Control Extension. */
		    GIFGraphControlExt graphControlExt;
		    DOLBUG( "Writing graphics control\n");
		    if ( imgList[ i].imgProp[ GIFPROP_TRANSPARENTCOLORINDEX] != nil) {
			graphControlExt.transparent = 1;
			graphControlExt.transparentColorIndex = imgList[ i].imgProp[ GIFPROP_TRANSPARENTCOLORINDEX]->val.Byte;
		    }
		    else {
			graphControlExt.transparent = 0;
			graphControlExt.transparentColorIndex = 0;
		    }
		    graphControlExt.userInput = ( imgList[ i].imgProp[ GIFPROP_USERINPUT] != nil ?
						  imgList[ i].imgProp[ GIFPROP_USERINPUT]->val.Byte :
						  0);
		    graphControlExt.disposalMethod = ( imgList[ i].imgProp[ GIFPROP_DISPOSALMETHOD] != nil ?
						       imgList[ i].imgProp[ GIFPROP_DISPOSALMETHOD]->val.Byte :
						       0);
		    graphControlExt.delayTime = ( imgList[ i].imgProp[ GIFPROP_DELAYTIME] != nil ?
						  imgList[ i].imgProp[ GIFPROP_DELAYTIME]->val.Int / 10 :
						  0);
		    gifrc = EGifPutExtension( ogif,
					       GRAPHICS_EXT_FUNC_CODE,
					       sizeof( GIFGraphControlExt),
					       ( ( Byte *) &graphControlExt) + 1
			);
		    if ( gifrc != GIF_OK) {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		    DOLBUG( "Put graphics extension\n");
		}
		if ( succeed) {
		    ColorMapObject *colorMap = palette2colormap(
			    imgList[ i].imgProp[ GIFPROP_PALETTESIZE]->val.Int,
			    imgList[ i].imgProp[ GIFPROP_PALETTE]->val.pByte
			);
		    xOffset = ( imgList[ i].imgProp[ GIFPROP_X] != nil ?
				imgList[ i].imgProp[ GIFPROP_X]->val.Int :
				0);
		    yOffset = ( imgList[ i].imgProp[ GIFPROP_Y] != nil ?
				imgList[ i].imgProp[ GIFPROP_Y]->val.Int :
				0);
		    interlaced = ( imgList[ i].imgProp[ GIFPROP_INTERLACED] != nil ?
				   imgList[ i].imgProp[ GIFPROP_INTERLACED]->val.Int :
				   0);
		    gifrc = EGifPutImageDesc( ogif,
					      xOffset,
					      yOffset,
					      imgList[ i].imgProp[ GIFPROP_WIDTH]->val.Int,
					      imgList[ i].imgProp[ GIFPROP_HEIGHT]->val.Int,
					      interlaced,
					      colorMap
			);
		    if ( gifrc != GIF_OK) {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		    DOLBUG( "Put Image desc for index %d (i==%d)\n", index, i);
		    FreeMapObject( colorMap);
		}
		if ( succeed) {
		    int h;
		    int pass = 0;
		    int row = interlaced ? interlaceOffs[ pass] : 0;
		    int height = imgList[ i].imgProp[ GIFPROP_HEIGHT]->val.Int;
		    int width = imgList[ i].imgProp[ GIFPROP_WIDTH]->val.Int;
		    int lineSize = imgList[ i].imgProp[ GIFPROP_LINESIZE]->val.Int;
		    Byte *data = imgList[ i].imgProp[ GIFPROP_DATA]->val.pByte;
		    for ( h = 0; ( h < height) && succeed; h++) {
			gifrc = EGifPutLine( ogif,
					     data + ( height - row - 1) * lineSize,
					     width
			    );
			if ( gifrc != GIF_OK) {
			    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
			    succeed = false;
			}
			else {
			    if ( interlaced) {
				row += interlaceStep[ pass];
				if ( row >= height) {
				    pass++;
				    row = interlaceOffs[ pass];
				}
			    }
			    else {
				row++;
			    }
			}
		    }
		}
		if ( succeed && ( imgList[ i].imgProp[ GIFPROP_COMMENTS] != nil)) {
		    int cnum;
		    for ( cnum = 0; ( cnum < imgList[ i].imgProp[ GIFPROP_COMMENTS]->used) && succeed; cnum++) {
			gifrc = EGifPutComment( ogif, imgList[ i].imgProp[ GIFPROP_COMMENTS]->val.pString[ cnum]);
			if ( gifrc != GIF_OK) {
			    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
			    succeed = false;
			}
			DOLBUG( "Put comment #%d\n", cnum);
		    }
		}
		i++;
	    }
	    else if ( igif_terminated) {
		/* An empty image must be put into the output. */
		gifrc = EGifPutImageDesc( ogif, 0, 0, 1, 1, 0, NULL);
		if ( gifrc == GIF_OK) {
		    gifrc = EGifPutPixel( ogif, 0);
		}
		if ( gifrc != GIF_OK) {
		    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
		    succeed = false;
		}
	    }
	    else {
		/* An image from igif must be taken and be put into the output. */
		Bool done;
		while ( succeed && ( ( lastIndex + 1) < index) && ! igif_terminated) {
		    /* First we must find image with corresponding index or termination
                       record. */
		    switch ( gifRecType) {
			case IMAGE_DESC_RECORD_TYPE:
			    {
				/* Founded image data must be skipped for now. */
				lastIndex++;
				gifrc = DGifGetImageDesc( igif);
				if ( gifrc == GIF_OK) {
				    gifrc = DGifGetCode( igif, &codeSize, &codeBlock);
				    while ( ( gifrc == GIF_OK) && ( codeBlock != NULL)) {
					gifrc = DGifGetCodeNext( igif, &codeBlock);
				    }
				}
			    }
			    break;
			case EXTENSION_RECORD_TYPE:
			    {
				if ( extData == nil) {
				    /* If extData != nil then it means we have read it
                                       somewhen before and decided that we don't need
                                       it. */
				    gifrc = DGifGetExtension( igif, &extCode, &extData);
				    DOLBUG( "Reading(skipping) extension code %d\n", extCode);
				}
				while ( ( gifrc == GIF_OK) && ( extData != NULL)) {
				    gifrc = DGifGetExtensionNext( igif, &extData);
				}
			    }
			    break;
			case TERMINATE_RECORD_TYPE:
			    igif_terminated = true;
			    break;
			default:
				/* I have no idea what must be done with unknown record
                                   types. */
			    ;
		    }
		    if ( gifrc == GIF_OK) {
			gifrc = DGifGetRecordType( igif, &gifRecType);
		    }
		    if ( gifrc != GIF_OK) {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		}
		/* We've skipped all spare image data blocks at this moment, and the next
                   one, if it exists, is our goal. But if it is prepended with a graphics
                   control extension block then we must move the extension into the output
                   as well. And, again, any other kind of extension block must be
                   skipped. */
		done = false;
		while ( succeed && ! igif_terminated && ! done) {
		    if ( gifrc == GIF_OK) {
			switch ( gifRecType) {
			    case EXTENSION_RECORD_TYPE:
				if ( extData == nil) {
				    gifrc = DGifGetExtension( igif, &extCode, &extData);
				}
				if ( ( gifrc == GIF_OK) && ( extCode != GRAPHICS_EXT_FUNC_CODE)) {
				    DOLBUG( "Found extension code %d\n", extCode);
				    while ( ( gifrc == GIF_OK) && extData) {
					gifrc = DGifGetExtensionNext( igif, &extData);
				    }
				    if ( gifrc == GIF_OK) {
					gifrc = DGifGetRecordType( igif, &gifRecType);
				    }
				}
				else {
				    done = true;
				}
				break;
			    case IMAGE_DESC_RECORD_TYPE:
				done = true;
				break;
			    case TERMINATE_RECORD_TYPE:
				igif_terminated = true;
				break;
			    default:
				;
			}
		    }
		    if ( gifrc != GIF_OK) {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		}
		if ( igif_terminated) {
		    /* We don't have an image to put into ouput because input dried out
                       . Therefore we must put and empty image. */
		    continue;
		}
		/* Finally, we are ready to move an image with optional extension blocks
                   from the input the output. */
		done = false;
		while ( succeed && ! igif_terminated && ! done) {
		    /* Record type has been read during the last skipping loop. */
		    switch ( gifRecType) {
			case EXTENSION_RECORD_TYPE:
			    {
				if ( extData == nil) {
				    /* Otherwise it means we have read some data in the
                                       previous loop. */
				    gifrc = DGifGetExtension( igif, &extCode, &extData);
				}
				if ( gifrc == GIF_OK) {
				    DOLBUG( "Writing (trying) extension with code %d (%d<=>%d)\n", extCode, lastIndex, index);
				    if ( ( extCode == GRAPHICS_EXT_FUNC_CODE)
					 && ( lastIndex == index)) {
					/* Yeah, necessary image is in output already! */
					done = true;
					continue;
				    }
				    else {
					/* This extension block belongs to the image which
                                           goes into the output. */
					gifrc = EGifPutExtension( ogif, extCode, extData[ 0], extData + 1);
					while ( ( gifrc == GIF_OK) && ( extData != NULL)) {
					    gifrc = DGifGetExtensionNext( igif, &extData);
					    if ( ( gifrc == GIF_OK) && ( extData != NULL)) {
						gifrc = EGifPutExtension( ogif, extCode, extData[ 0], extData + 1);
					    }
					}
				    }
				}
			    }
			    break;
			case IMAGE_DESC_RECORD_TYPE:
			    {
				if ( lastIndex == index) {
				    done = true;
				    continue;
				}
				else {
				    gifrc = DGifGetImageDesc( igif);
				    if ( gifrc == GIF_OK) {
					EGifPutImageDesc( ogif,
							  igif->Image.Left,
							  igif->Image.Top,
							  igif->Image.Width,
							  igif->Image.Height,
							  igif->Image.Interlace,
							  igif->Image.ColorMap
					    );
					gifrc = DGifGetCode( igif, &codeSize, &codeBlock);
					if ( gifrc == GIF_OK) {
					    gifrc = EGifPutCode( ogif, codeSize, codeBlock);
					}
				    }
				    while ( ( gifrc == GIF_OK) && ( codeBlock != NULL)) {
					gifrc = DGifGetCodeNext( igif, &codeBlock);
					if ( gifrc == GIF_OK) {
					    gifrc = EGifPutCodeNext( ogif, codeBlock);
					}
				    }
				    lastIndex++; /* Note that now lastIndex is equal to the
						    index! */
				    DOLBUG( "Wrote image desc\n");
				}
			    }
			    break;
			case TERMINATE_RECORD_TYPE:
			    igif_terminated = true;
			    break;
			default:
			    ;
		    }
		    if ( gifrc == GIF_OK) {
			igif_terminated = ( gifRecType == TERMINATE_RECORD_TYPE);
			if ( ! igif_terminated && ! ( done && ( extData != nil))) {
			    gifrc = DGifGetRecordType( igif, &gifRecType);
			}
		    }
		    else {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		}
	    }
	    index++;
	}
    }

    if ( igif) {
	DGifCloseFile( igif);
    }
    if ( ogif) {
	EGifCloseFile( ogif);
    }

    if ( fileExists) {
	if ( succeed) {
	    unlink( filename);
	    rename( tmpname, filename);
	}
	else {
	    unlink( tmpname);
	}
    }

    for ( i = 0; i < imgInfo->count; i++) {
	free( imgList[ i].imgProp);
    }
    free( imgList);

    return succeed;
}

static ImgCapInfo gif_caps[ 3];
static int supportedTypes[ 2] = { im256, imByte};
static ImgProps propExtProps[ 3];
static ImgProps gif_props[ GIFPROP_TOTAL];

ImgFormat *
__gif_init()
{
    int i;

    gifFormat.id = "GIF";
    gifFormat.descr = "Graphics Interchange Format";
    gifFormat.preread_size = 3;
    gifFormat.load = __gif_load;
    gifFormat.save = __gif_save;
    gifFormat.is_loadable = __gif_loadable;
    gifFormat.is_storable = __gif_storable;
    gifFormat.is_compatible = __gif_compatible;
    gifFormat.getInfo = __gif_getinfo;
    gifFormat.getError = __gif_geterror;

    gif_caps[ 0].id = "type";
    gif_caps[ 0].type = "i*";
    gif_caps[ 0].descr = "Valid image formats list";
    gif_caps[ 0].size = 2;
    gif_caps[ 0].val.pInt = supportedTypes;

    gif_caps[ 1].id = "multi";
    gif_caps[ 1].type = "i";
    gif_caps[ 1].descr = "Supports multiple images";
    gif_caps[ 1].size = 0;
    gif_caps[ 1].val.Int = 1;

    gif_caps[ 2].id = nil;
    gif_caps[ 2].type = nil;
    gif_caps[ 2].descr = nil;
    gif_caps[ 2].size = 0;
    gif_caps[ 2].val.Int = 0;

    gifFormat.capabilities = gif_caps;

    propExtProps[ 0].name = "code"; 
    propExtProps[ 0].id = GIFPROP_EXTENSION_CODE;
    propExtProps[ 0].type = "i";
    propExtProps[ 0].descr = "GIF extension code";
    propExtProps[ 0].subProps = nil;

    propExtProps[ 1].name = "blocks"; 
    propExtProps[ 1].id = GIFPROP_EXTENSION_BLOCKS;
    propExtProps[ 1].type = "B*";
    propExtProps[ 1].descr = "GIF extension blocks";
    propExtProps[ 1].subProps = nil;

    propExtProps[ 2].name = nil;
    propExtProps[ 2].id = 0;
    propExtProps[ 2].type = nil;
    propExtProps[ 2].descr = nil;
    propExtProps[ 2].subProps = nil;

    i = -1;
    /* For index == -1 means screen width */
    gif_props[ i++].name = "width";
    gif_props[ i].id = GIFPROP_WIDTH;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Image width";
    gif_props[ i]. subProps = nil;

    gif_props[ i++].name = "height";
    gif_props[ i].id = GIFPROP_HEIGHT;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Image height";
    gif_props[ i]. subProps = nil;

    gif_props[ i++].name = "type";
    gif_props[ i++].id = GIFPROP_TYPE;
    gif_props[ i++].type = "i";
    gif_props[ i++].descr = "Image type (bpp)";
    gif_props[ i++].subProps = nil;

    gif_props[ i++].name = "name";
    gif_props[ i].id = GIFPROP_NAME;
    gif_props[ i].type = "s";
    gif_props[ i].descr = "Image-specific description";
    gif_props[ i].subProps = nil; /* Only returned by read */

    gif_props[ i++].name = "lineSize";
    gif_props[ i].id = GIFPROP_LINESIZE;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Length of a scan line in bytes";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "data";
    gif_props[ i].id = GIFPROP_DATA;
    gif_props[ i].type = "b*";
    gif_props[ i].descr = "Image data";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "palette";
    gif_props[ i].id = GIFPROP_PALETTE;
    gif_props[ i].type = "b*";
    gif_props[ i].descr = "Image palette";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "index";
    gif_props[ i].id = GIFPROP_INDEX;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Position in image list";
    gif_props[ i].subProps = nil; /* -1 refers to the whole image file */

    gif_props[ i++].name = "format";
    gif_props[ i].id = GIFPROP_FORMAT;
    gif_props[ i].type = "s";
    gif_props[ i].descr = "File format";
    gif_props[ i].subProps = nil; /* May appear only for an image with index == -1 */

    gif_props[ i++].name = "overwrite";
    gif_props[ i].id = GIFPROP_OVERWRITE;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Non-zero means we must overwrite destination file";
    gif_props[ i].subProps = nil;  /* Save only. Ignored for any image with index != -1 */

    gif_props[ i++].name = "interlaced";
    gif_props[ i].id = GIFPROP_INTERLACED;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Image is interlaced";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "XOffset";
    gif_props[ i].id = GIFPROP_X;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Left offset";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "YOffset";
    gif_props[ i].id = GIFPROP_Y;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Top offset";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "colorResolution";
    gif_props[ i].id = GIFPROP_COLORRESOLUTION;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "How many colors can be generated";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "bgColor";
    gif_props[ i].id = GIFPROP_BGCOLOR;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Background color";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "paletteSize";
    gif_props[ i].id = GIFPROP_PALETTESIZE;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Number of entries in image palette";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "paletteBPP";
    gif_props[ i].id = GIFPROP_PALETTEBPP;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Image palette BPP";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "disposalMethod";
    gif_props[ i].id = GIFPROP_DISPOSALMETHOD;
    gif_props[ i].type = "b";
    gif_props[ i].descr = "Image disposal method in animation GIF";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "userInput";
    gif_props[ i].id = GIFPROP_USERINPUT;
    gif_props[ i].type = "b";
    gif_props[ i].descr = "User input expected";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "transparentColorIndex";
    gif_props[ i].id = GIFPROP_TRANSPARENTCOLORINDEX;
    gif_props[ i].type = "b";
    gif_props[ i].descr = "Transparent color index";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "comments";
    gif_props[ i].id = GIFPROP_COMMENTS;
    gif_props[ i].type = "s*";
    gif_props[ i].descr = "GIF file comments";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name = "extensions";
    gif_props[ i].id = GIFPROP_EXTENSIONS;
    gif_props[ i].type = "p*";
    gif_props[ i].descr = "GIF extensions";
    gif_props[ i].subProps = propExtProps;

    gif_props[ i++].name = "delayTime";
    gif_props[ i].id = GIFPROP_DELAYTIME;
    gif_props[ i].type = "i";
    gif_props[ i].descr = "Delay in ms after showing the image in animation GIF";
    gif_props[ i].subProps = nil;

    gif_props[ i++].name =  nil;
    gif_props[ i].id = 0;
    gif_props[ i].type = nil;
    gif_props[ i].descr = nil;
    gif_props[ i].subProps = nil;
    
    gifFormat.propertyList = gif_props;

    return &gifFormat;
}
