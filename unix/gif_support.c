#include "apricot.h"
#include "unix/gif_support.h"
#include <unistd.h>
#include <signal.h>

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
	{ "index", "i", "Position in image list"}, /* -1 refers to the whole image file */
	{ "format", "s", "File format"}, /* May appear only for an image with index == -1 */
	{ "interlaced", "i", "Image is interlaced"},
	{ "X", "i", "Left offset"},
	{ "Y", "i", "Top offset"},
	{ "width", "i", "Image width"}, /* For index == -1 means screen width */
	{ "height", "i", "Image height"}, /* For index == -1 means screen height */
	{ "colorResolution", "i", "How many colors can be generated"},
	{ "bgColor", "i", "Background color"},
	{ "paletteSize", "i", "Number of entries in image palette"},
	{ "paletteBPP", "i", "Image palette BPP"},
	{ "palette", "b*", "Image palette"},
	{ "lineSize", "i", "Length of a scan line in bytes"},
	{ "data", "b*", "Image data"},
	{ nil, nil, nil}
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

static int errorCode = DERR_NO_ERROR;

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
    return ( strcmp( imgProp->id, propName) == 0);
}

static Bool
__gif_read( int fd, const char *filename, PList imgInfo, Bool readData)
{
    GifFileType *gif;
    GifRecordType gifType;
    GifImageDesc *gifChunks;
    GifPixelType **imageData;
    int lastIndex = -1;
    Bool succeed = true, readAll = false;
    int imgDescCount = 0;
    int *queriedIdx;
    int i, gifrc;
    Bool done;

    fprintf( stderr, "Queried for %d images\n", imgInfo->count);
    queriedIdx = ( int*) malloc( sizeof( int) * imgInfo->count);
    if ( queriedIdx == NULL) {
	__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
	return false;
    }
    for ( i = 0; ( i < imgInfo->count) && succeed; i++) {
	int indexPropIdx;
	PImgProperty imgProp;
	PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
	fprintf( stderr, "Image #%d: %d properties\n", i, imageInfo->propList->count);
	indexPropIdx = list_first_that( imageInfo->propList, property_name, ( void*) "index");
	if ( indexPropIdx >= 0) {
	    imgProp = ( PImgProperty) list_at( imageInfo->propList, indexPropIdx);
	}
	else {
	    imgProp = apc_image_add_property( imageInfo, "index", -1);
	    imgProp->val.Int = 0;
	}
	if ( imgProp->size == -1) {
	    if ( imgProp->val.Int > lastIndex) {
		lastIndex = imgProp->val.Int;
	    }
	    queriedIdx[ i] = imgProp->val.Int;
	}
	else {
	    /* ERROR: The property contains an array. */
	    __gif_seterror( GIFERRT_DRIVER, DERR_WRONG_PROPERTY_TYPE);
	    succeed = false;
	    continue;
	}
    }

    if ( ! succeed) {
	return false;
    }

    if ( ( lastIndex == -1) && readData) {
	printf( "We need all images\n");
	readAll = true;
	fprintf( stderr, "Unsupported mode (yet)\n");
	return false;
    }
    else {
	printf( "We need %d images\n", lastIndex + 1);
    }

    fprintf( stderr, "opening %s\n", filename);

    gif = DGifOpenFileName( filename);
    if ( gif == NULL) {
	__gif_seterror( GIFERRT_GIFLIB, GifLastError());
	return false;
    }

    /* XXX this is a subject to change! */
    gifChunks = ( GifImageDesc *) malloc( sizeof( GifImageDesc) * ( lastIndex + 1));
    if ( gifChunks == NULL) {
	DGifCloseFile( gif);
	__gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
	return false;
    }
    for ( i = 0; i <= lastIndex; i++) {
	gifChunks[ i].ColorMap = NULL;
    }
    if ( readData) {
	imageData = ( GifPixelType **) malloc( sizeof( GifPixelType *) * ( lastIndex + 1));
	if ( imageData == NULL) {
	    free( gifChunks);
	    DGifCloseFile( gif);
	    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
	    return false;
	}
	for ( i = 0; i <= lastIndex; i++) {
	    imageData[ i] = NULL;
	}
    }

    if ( gif->SColorMap != NULL) {
	fprintf( stderr, "GLOBAL: ColorCnt: %d, BPP: %d",
		 gif->SColorMap->ColorCount,
		 gif->SColorMap->BitsPerPixel
	    );
	for ( i = 0; i < gif->SColorMap->ColorCount; i++) {
	    fprintf( stderr, ", [%d,%d,%d]",
		     gif->SColorMap->Colors[ i].Red,
		     gif->SColorMap->Colors[ i].Green,
		     gif->SColorMap->Colors[ i].Blue
		);
	}
	fprintf( stderr, "\n");
    }

    done = false;
    do {
	Byte *extData;
	int extCode;

	gifrc = DGifGetRecordType( gif, &gifType);
	if ( gifrc != GIF_OK) {
	    fprintf( stderr, "*** Can't get next record type\n");
	    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
	    done = true;
	    succeed = false;
	    break;
	}
	switch ( gifType) {
	    case SCREEN_DESC_RECORD_TYPE:
		fprintf( stderr, "Screen record\n");
		break;
	    case TERMINATE_RECORD_TYPE:
		fprintf( stderr, "Termination record\n");
		break;
	    case IMAGE_DESC_RECORD_TYPE:
		gifrc = DGifGetImageDesc( gif);
		if ( gifrc != GIF_OK) {
		    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
		    succeed = false;
		}
		else {
		    int codeSize;
		    Byte *codeBlock;
		    Bool idxQueried = false;

		    for ( i = 0; ( i <= lastIndex) && ( ! idxQueried); i++) {
			idxQueried = ( imgDescCount == queriedIdx[ i]);
		    }

		    memcpy( ( void*) ( gifChunks + imgDescCount), ( void*) &gif->Image, sizeof( GifImageDesc));
		    if ( ( gif->Image.ColorMap != NULL) && idxQueried) {
			gifChunks[ imgDescCount].ColorMap = MakeMapObject( gif->Image.ColorMap->ColorCount, gif->Image.ColorMap->Colors);
		    }
		    else {
			gifChunks[ imgDescCount].ColorMap = NULL;
		    }
		    fprintf( stderr, "Image count: %d; Scr: %dx%d, Img: %dx%d+%d+%d, ColorRes: %d\n", 
			     gif->ImageCount,
			     gif->SWidth,
			     gif->SHeight,
			     gifChunks[ imgDescCount].Width,
			     gifChunks[ imgDescCount].Height,
			     gifChunks[ imgDescCount].Left,
			     gifChunks[ imgDescCount].Top,
			     gif->SColorResolution
			);
		    if ( gif->SColorMap != NULL) {
			fprintf( stderr, "ColorCnt: %d, BPP: %d",
				 gif->SColorMap->ColorCount,
				 gif->SColorMap->BitsPerPixel
			    );
			for ( i = 0; i < gif->SColorMap->ColorCount; i++) {
			    fprintf( stderr, ", [%d,%d,%d]",
				     gif->SColorMap->Colors[ i].Red,
				     gif->SColorMap->Colors[ i].Green,
				     gif->SColorMap->Colors[ i].Blue
				);
			}
			fprintf( stderr, "\n");
		    }
		    if ( gifChunks[ imgDescCount].ColorMap != NULL) {
			fprintf( stderr, "ImgColorMap: cnt: %d, bpp: %d", 
				 gifChunks[ imgDescCount].ColorMap->ColorCount,
				 gifChunks[ imgDescCount].ColorMap->BitsPerPixel
			    );
			for ( i = 0; i < gifChunks[ imgDescCount].ColorMap->ColorCount; i++) {
			    fprintf( stderr, ", [%d,%d,%d]",
				     gifChunks[ imgDescCount].ColorMap->Colors[ i].Red,
				     gifChunks[ imgDescCount].ColorMap->Colors[ i].Green,
				     gifChunks[ imgDescCount].ColorMap->Colors[ i].Blue
				);
			}
			fprintf( stderr, "\n");
		    }
		    if ( idxQueried && readData) {
			int pixelCount = gifChunks[ imgDescCount].Width * gifChunks[ imgDescCount].Height;
			imageData[ imgDescCount] = ( GifPixelType *) malloc( sizeof( GifPixelType) * pixelCount);
			if ( imageData[ imgDescCount] == NULL) {
			    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			    succeed = false;
			}
			else {
			    gifrc = DGifGetLine( gif, imageData[ imgDescCount], pixelCount);
			}
		    }
		    else {
			gifrc = DGifGetCode( gif, &codeSize, &codeBlock);
			if ( gifrc == GIF_OK) {
			    while( ( codeBlock != NULL) && ( gifrc == GIF_OK)) {
				fprintf( stderr, ".");
				gifrc = DGifGetCodeNext( gif, &codeBlock);
			    }
			    fprintf( stderr, " ");
			}
		    }
		    if ( gifrc != GIF_OK) {
			__gif_seterror( GIFERRT_GIFLIB, GifLastError());
			succeed = false;
		    }
		    //fprintf( stderr, "Image description record (%s)\n", gifrc == GIF_OK ? "succeed" : "failed");
		    imgDescCount++;
		}
		break;
	    case EXTENSION_RECORD_TYPE:
		extCode = -1;
		gifrc = DGifGetExtension( gif, &extCode, &extData);
		if ( gifrc == GIF_OK) {
		    do {
			//fprintf( stderr, "Extension record, code == %d\n", extCode);
			gifrc = DGifGetExtensionNext( gif, &extData);
		    } while ( ( extData != NULL) && ( gifrc == GIF_OK));
		}
		if ( gifrc != GIF_OK) {
		    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
		    succeed = false;
		}
		//fprintf( stderr, "Extension record, code == %d (%s)\n", extCode, gifrc == GIF_OK ? "succeed" : "failed");
		break;
	    default:
		fprintf( stderr, "Unknown record type\n");
	}
	done = done || ( ! succeed) || ( imgDescCount > lastIndex) || ( ! readAll);
    } while( ( gifType != TERMINATE_RECORD_TYPE) && ( ! done));

    if ( succeed) {
	fprintf( stderr, "Got %d images (???)\n", imgDescCount);

	for ( i = 0; ( i < imgInfo->count) && succeed; i++) {
	    int indexPropIdx, index = -1;
	    PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
	    indexPropIdx = list_first_that( imageInfo->propList, property_name, ( void*) "index");
	    if ( indexPropIdx >= 0) {
		index = ( ( PImgProperty) list_at( imageInfo->propList, indexPropIdx))->val.Int;
	    }
	    fprintf( stderr, "idx: %d\n", index);
	    plist_destroy( imageInfo->propList);
	    imageInfo->propList = plist_create( 5, 5);

	    if ( index == -1) {
		PImgProperty imgProp;

		imgProp = apc_image_add_property( imageInfo, "index", -1);
		if ( imgProp != NULL) imgProp->val.Int = -1;

		imgProp = apc_image_add_property( imageInfo, "format", -1);
		if ( imgProp != NULL) imgProp->val.String = duplicate_string( gifFormat.id);

		imgProp = apc_image_add_property( imageInfo, "width", -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SWidth;

		imgProp = apc_image_add_property( imageInfo, "height", -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SHeight;

		imgProp = apc_image_add_property( imageInfo, "colorResolution", -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SColorResolution;

		imgProp = apc_image_add_property( imageInfo, "bgColor", -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SBackGroundColor;

		if ( gif->SColorMap != NULL) {
		    imgProp = apc_image_add_property( imageInfo, "paletteSize", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gif->SColorMap->ColorCount;

		    imgProp = apc_image_add_property( imageInfo, "paletteBPP", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gif->SColorMap->ColorCount;

		    imgProp = apc_image_add_property( imageInfo, "palette", 3 * gif->SColorMap->ColorCount);
		    imgProp->val.pByte = ( Byte *) malloc( imgProp->size);
		    for ( i = 0; i < gif->SColorMap->ColorCount; i++) {
			Byte *palEntry = ( imgProp->val.pByte + i * 3);
			palEntry[ 0] = gif->SColorMap->Colors[ i].Red;
			palEntry[ 1] = gif->SColorMap->Colors[ i].Green;
			palEntry[ 2] = gif->SColorMap->Colors[ i].Blue;
		    }
		}
	    }
	    else if ( index >= 0) {
		PImgProperty imgProp;
		ColorMapObject *palette;

		imgProp = apc_image_add_property( imageInfo, "index", -1);
		if ( imgProp != NULL) imgProp->val.Int = -1;

		imgProp = apc_image_add_property( imageInfo, "format", -1);
		if ( imgProp != NULL) imgProp->val.String = duplicate_string( gifFormat.id);

		if ( index < imgDescCount) {
		    imgProp = apc_image_add_property( imageInfo, "width", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunks[ index].Width;

		    imgProp = apc_image_add_property( imageInfo, "height", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunks[ index].Height;

		    imgProp = apc_image_add_property( imageInfo, "X", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunks[ index].Left;

		    imgProp = apc_image_add_property( imageInfo, "Y", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunks[ index].Top;

		    imgProp = apc_image_add_property( imageInfo, "interlaced", -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunks[ index].Interlace;

		    palette = ( gifChunks[ index].ColorMap != NULL ?
				gifChunks[ index].ColorMap :
				gif->SColorMap);

		    imgProp = apc_image_add_property( imageInfo, "palette", 3 * palette->ColorCount);
		    imgProp->val.pByte = ( Byte *) malloc( imgProp->size);
		    for ( i = 0; i < palette->ColorCount; i++) {
			Byte *palEntry = ( imgProp->val.pByte + i * 3);
			palEntry[ 0] = palette->Colors[ i].Red;
			palEntry[ 1] = palette->Colors[ i].Green;
			palEntry[ 2] = palette->Colors[ i].Blue;
		    }

		    if ( readData) {
			if ( imageData[ index] != NULL) {
			    int pixelCount = gifChunks[ index].Width * gifChunks[ index].Height;
			    imgProp = apc_image_add_property( imageInfo, "data", pixelCount);
			    if ( imgProp != NULL) {
				imgProp->val.pByte = ( Byte*) malloc( sizeof( Byte) * pixelCount);
				memcpy( imgProp->val.pByte, imageData[ index], pixelCount);
			    }
			    imgProp = apc_image_add_property( imageInfo, "lineSize", -1);
			    if ( imgProp != NULL) imgProp->val.Int = gifChunks[ index].Width;
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

    for ( i = 0; i <= lastIndex; i++) {
	if ( gifChunks[ i].ColorMap != NULL) {
	    FreeMapObject( gifChunks[ i].ColorMap);
	}
    }
    free( gifChunks);
    free( queriedIdx);
    if ( readData) {
	for ( i = 0; i <= lastIndex; i++) {
	    free( imageData[ i]);
	}
	free( imageData);
    }

    return succeed;
}

Bool
__gif_load( int fd, const char *filename, PList imgInfo)
{
    return __gif_read( fd, filename, imgInfo, true);
}

Bool
__gif_save( const char *filename, PList imgInfo)
{
    return false;
}

Bool
__gif_loadable( int fd, const char *filename, Byte *preread_buf, U32 buf_size)
{
    fprintf( stderr, "__gif_loadable( %s)\n", filename);
    return ( strncmp( "GIF", preread_buf, 3) == 0);
}

Bool
__gif_storable( const char *filename, PList imgInfo)
{
    return false;
}

Bool
__gif_getinfo( int fd, const char *filename, PList imgInfo)
{
    return __gif_read( fd, filename, imgInfo, false);
}
