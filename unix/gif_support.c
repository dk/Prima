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
#include "img_api.h"
#include "unix/gif_support.h"
#include <unistd.h>
#include <signal.h>

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

ImgFormat gifFormat = {
    "GIF", "Graphics Interchange Format",
    ( ImgCapInfo[]) { 
	{ "type", "i*", "Valid image formats list", 2, { pInt:( int[]){ 
	    im256, 
	    imByte}}},
	{ "multi", "i", "Supports multiple images", 0, { Int:1}},
	{ nil, nil, nil, 0, { Int:0}}
    },
    ( ImgProps[]) {
	{ "width", GIFPROP_WIDTH, "i", "Image width", nil}, /* For index == -1 means screen width */
	{ "height", GIFPROP_HEIGHT, "i", "Image height", nil}, /* For index == -1 means screen height */
	{ "type", GIFPROP_TYPE, "i", "Image type (bpp)", nil}, 
	{ "name", GIFPROP_NAME, "s", "Image-specific description", nil}, /* Only returned by read */
	{ "lineSize", GIFPROP_LINESIZE, "i", "Length of a scan line in bytes", nil},
	{ "data", GIFPROP_DATA, "b*", "Image data", nil},
	{ "palette", GIFPROP_PALETTE, "b*", "Image palette", nil},
	{ "index", GIFPROP_INDEX, "i", "Position in image list", nil}, /* -1 refers to the whole image file */
	{ "format", GIFPROP_FORMAT, "s", "File format", nil}, /* May appear only for an image with index == -1 */
	{ "interlaced", GIFPROP_INTERLACED, "i", "Image is interlaced", nil},
	{ "XOffset", GIFPROP_X, "i", "Left offset", nil},
	{ "YOffset", GIFPROP_Y, "i", "Top offset", nil},
	{ "colorResolution", GIFPROP_COLORRESOLUTION, "i", "How many colors can be generated", nil},
	{ "bgColor", GIFPROP_BGCOLOR, "i", "Background color", nil},
	{ "paletteSize", GIFPROP_PALETTESIZE, "i", "Number of entries in image palette", nil},
	{ "paletteBPP", GIFPROP_PALETTEBPP, "i", "Image palette BPP", nil},
	{ "disposalMethod", GIFPROP_DISPOSALMETHOD, "b", "Image disposal method in animation GIF", nil},
	{ "userInput", GIFPROP_USERINPUT, "b", "User input expected", nil},
	{ "transparentColorIndex", GIFPROP_TRANSPARENTCOLORINDEX, "b", "Transparent color index", nil},
	{ "comments", GIFPROP_COMMENTS, "s*", "GIF file comments", nil},
	{ "extensions", GIFPROP_EXTENSIONS, "p*", "GIF extensions",
	  ( ImgProps[]) {
	      { "code", GIFPROP_EXTENSION_CODE, "i", "GIF extension code"},
	      { "blocks", GIFPROP_EXTENSION_BLOCKS, "B*", "GIF extension blocks"},
	      { nil, 0, nil, nil, nil}
	  }
	},
	{ "delayTime", GIFPROP_DELAYTIME, "i", "Delay in ms after showing the image in animation GIF", nil},
	{ nil, 0, nil, nil, nil}
    },
    3,
    __gif_load,
    __gif_save,
    __gif_loadable,
    __gif_storable,
    __gif_getinfo,
    __gif_geterror
};

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
    /* NB: This bit-fields sequence requires big-endian bit order. */
    unsigned char transparent:1;
    unsigned char userInput:1;
    unsigned char disposalMethod:3;
    unsigned char reserved:3;
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

const char *
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
		case DERR_DATA_NOT_PRESENT:
		    msg = "*** GIF internal: no image data for the index";
		    break;
		case DERR_NO_INDEX_PROP:
		    msg = "*** GIF internal: no `index' property found while filling in images list";
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
    GifFileType *gif;
    GifRecordType gifRecType;
    GifImageDesc *gifChunk;
    GifPixelType **data;
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
	Byte *extData;
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
		    Byte *codeBlock;
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
		    PGIFExtInfo gifExtension;

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
	    succeed = ( imgProp = img_info_add_property( firstImg, "index", PROPTYPE_INT, -1, 0)) != nil;

	    for ( i = 1; ( i < imgDescCount) && succeed; i++) {
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
			    PImgProperty extProp;
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
			imgProp = img_info_add_property( imageInfo, "lineSize", PROPTYPE_BYTE, -1, gif->SWidth);
		    }
		    succeed = imgProp != nil;
		}
	    }
	    else if ( index >= 0) {
		ColorMapObject *palette;

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
			if ( succeed) {
			    DOLBUG( "GIF palette size: %d from %d\n", imgProp->val.Int, palette->ColorCount);
			}
			else {
			    DOLBUG( "GIF SHIT HAPPENED\n");
			}
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

Bool
__gif_load( int fd, const char *filename, PList imgInfo, Bool readAll)
{
    return __gif_read( fd, filename, imgInfo, true, readAll);
}

Bool
__gif_save( const char *filename, PList imgInfo)
{
    return false;
}

Bool
__gif_loadable( int fd, const char *filename, Byte *preread_buf, U32 buf_size)
{
    return ( strncmp( "GIF", preread_buf, 3) == 0);
}

Bool
__gif_storable( const char *filename, PList imgInfo)
{
    return false;
}

Bool
__gif_getinfo( int fd, const char *filename, PList imgInfo, Bool readAll)
{
    return __gif_read( fd, filename, imgInfo, false, readAll);
}
