#include "apricot.h"
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
	{ "width", GIFPROP_WIDTH, "i", "Image width"}, /* For index == -1 means screen width */
	{ "height", GIFPROP_HEIGHT, "i", "Image height"}, /* For index == -1 means screen height */
	{ "type", GIFPROP_TYPE, "i", "Image type (bpp)"}, 
	{ "name", GIFPROP_NAME, "s", "Image-specific description"}, /* Only returned by read */
	{ "lineSize", GIFPROP_LINESIZE, "i", "Length of a scan line in bytes"},
	{ "data", GIFPROP_DATA, "b*", "Image data"},
	{ "palette", GIFPROP_PALETTE, "b*", "Image palette"},
	{ "index", GIFPROP_INDEX, "i", "Position in image list"}, /* -1 refers to the whole image file */
	{ "format", GIFPROP_FORMAT, "s", "File format"}, /* May appear only for an image with index == -1 */
	{ "interlaced", GIFPROP_INTERLACED, "i", "Image is interlaced"},
	{ "X", GIFPROP_X, "i", "Left offset"},
	{ "Y", GIFPROP_Y, "i", "Top offset"},
	{ "colorResolution", GIFPROP_COLORRESOLUTION, "i", "How many colors can be generated"},
	{ "bgColor", GIFPROP_BGCOLOR, "i", "Background color"},
	{ "paletteSize", GIFPROP_PALETTESIZE, "i", "Number of entries in image palette"},
	{ "paletteBPP", GIFPROP_PALETTEBPP, "i", "Image palette BPP"},
	{ "disposalMethod", GIFPROP_DISPOSALMETHOD, "b", "Image disposal method in animation GIF"},
	{ "userInput", GIFPROP_USERINPUT, "b", "User input expected"},
	{ "transparentColorIndex", GIFPROP_TRANSPARENTCOLORINDEX, "b", "Transparent color index"},
	{ "comments", GIFPROP_COMMENTS, "s*", "GIF file comments"},
	{ "extensions", GIFPROP_EXTENSIONS, "B*", "GIF extensions"},
	{ "delayTime", GIFPROP_DELAYTIME, "i", "Delay in ms after showing the image in animation GIF"},
	{ nil, 0, nil, nil}
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
    GifRecordType gifType;
    GifImageDesc *gifChunk;
    GifPixelType **data;
    int lastIndex = -1;
    Bool succeed = true;
    int imgDescCount = 0;
    int i, gifrc;
    Bool done;
    List gifChunks, queriedIdx, imageData, imageExtensions;

    DOLBUG( "Got %d profile(s)\n", imgInfo->count);
    if ( readAll) {
	/* If been queried for all images then there must no more than one image profile 
	   in the imgInfo list. If it contains properties we must remove them. */
	PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, 0);
	DOLBUG( "Got %d properties for readAll operation\n", imageInfo->propList->count);
	for ( i = ( imageInfo->propList->count - 1); i >= 0; i--) {
	    PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, i);
	    apc_image_clear_property( imgProp);
	    free( imgProp);
	    list_delete_at( imageInfo->propList, i);
	}
    }
    else {
	list_create( &queriedIdx, imgInfo->count, 1);
	for ( i = 0; ( i < imgInfo->count) && succeed; i++) {
	    int indexPropIdx;
	    PImgProperty imgProp;
	    PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
/*  	    DOLBUG( "Image #%d: %d properties\n", i, imageInfo->propList->count); */
	    indexPropIdx = list_first_that( imageInfo->propList, property_name, ( void*) "index");
	    if ( indexPropIdx >= 0) {
		imgProp = ( PImgProperty) list_at( imageInfo->propList, indexPropIdx);
	    }
	    else {
		imgProp = apc_image_add_property( imageInfo, "index", PROPTYPE_INT, -1);
		if ( imgProp != NULL) imgProp->val.Int = ++lastIndex;
	    }
	    if ( imgProp->size == -1) {
		if ( imgProp->val.Int > lastIndex) {
		    lastIndex = imgProp->val.Int;
		}
		list_add( &queriedIdx, imgProp->val.Int);
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

/*      DOLBUG( "The last index is %d, queried for %d indexes\n", lastIndex, queriedIdx.count); */

/*      DOLBUG( "opening %s (%s)\n", filename, readAll ? "all images" : "selected"); */

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

	gifrc = DGifGetRecordType( gif, &gifType);
	if ( gifrc != GIF_OK) {
/*  	    DOLBUG( "*** Can't get next record type\n"); */
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
		*data = NULL;
		list_add( &imageData, ( Handle) data);
	    }
	}

	switch ( gifType) {
	    case SCREEN_DESC_RECORD_TYPE:
		DOLBUG( "Screen record\n");
		break;
	    case TERMINATE_RECORD_TYPE:
		DOLBUG( "Termination record\n");
		break;
	    case IMAGE_DESC_RECORD_TYPE:
		DOLBUG( "Description record\n");
		if ( ! readAll) {
		    if( ( done = ( imgDescCount > lastIndex))) {
			DOLBUG( "The last one...\n");
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
		    DOLBUG( "Image count: %d; Scr: %dx%d, Img: %dx%d+%d+%d, ColorRes: %d\n", 
			     gif->ImageCount,
			     gif->SWidth,
			     gif->SHeight,
			     gifChunk->Width,
			     gifChunk->Height,
			     gifChunk->Left,
			     gifChunk->Top,
			     gif->SColorResolution
			);
		    if ( idxQueried && readData) {
			int pixelCount = gifChunk->Width * gifChunk->Height;
			data = ( GifPixelType **) list_at( &imageData, imgDescCount);
			*data = malloc( sizeof( GifPixelType) * pixelCount);
			if ( *data == NULL) {
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
				DOLBUG( ".");
				gifrc = DGifGetCodeNext( gif, &codeBlock);
			    }
			    DOLBUG( " ");
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
		DOLBUG( "Extension record: ");
		extCode = -1;
		gifrc = DGifGetExtension( gif, &extCode, &extData);
		DOLBUG( "code is %02x, ", extCode);
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
			int ti;
			DOLBUG( "{size:%d}[ ", extData[ 0] + 1);
			for ( ti = 0; ti <= extData[ 0]; ti++) {
			    DOLBUG( "%02x ", extData[ ti]);
			}
			DOLBUG( "]");
			if ( idxQueried) {
			    Byte *extDataCopy = malloc( extData[ 0] + 1);
			    memcpy( extDataCopy, extData, extData[ 0] + 1);
			    list_add( &gifExtension->data, ( Handle) extDataCopy);
			}
			gifrc = DGifGetExtensionNext( gif, &extData);
		    } while ( ( extData != NULL) && ( gifrc == GIF_OK));
		}
		DOLBUG( "\n");
		if ( gifrc != GIF_OK) {
		    __gif_seterror( GIFERRT_GIFLIB, GifLastError());
		    succeed = false;
		}
		break;
	    default:
		DOLBUG( "Unknown record type\n");
	}
	done = done || ( ! succeed);
    } while( ( gifType != TERMINATE_RECORD_TYPE) && ( ! done));

    if ( succeed) {
	DOLBUG( "Got %d images.\n", imgDescCount);

	if ( readAll) {
	    /* Now we must create slots for all the images have been readed. */
	    /* The first slot is already here. We can just fill it in. */
	    /* For all subsequent images the first one will be used as a template. */
	    PImgInfo imageInfo;
	    PImgProperty imgProp;
	    PImgInfo firstImg = ( PImgInfo) list_at( imgInfo, 0);
	    imgProp = apc_image_add_property( firstImg, "index", PROPTYPE_INT, -1);
	    if ( imgProp != NULL) imgProp->val.Int = 0;

/*  	    if ( firstImg->extraInfo) { */
		/* XXX If queried for extra image information then we create an entry
		   for golbal image info. */
/*  		imageInfo = malloc( sizeof( ImgInfo)); */
/*  		imageInfo->extraInfo = firstImg->extraInfo; */
/*  		imageInfo->propList = plist_create( 15, 5); */
/*  		imgProp = apc_image_add_property( imageInfo, "index", PROPTYPE_INT, -1); */
/*  		if ( imgProp != NULL) imgProp->val.Int = -1; */
/*  		list_insert_at( imgInfo, ( Handle) imageInfo, 0); */
/*  	    } */

	    for ( i = 1; i < imgDescCount; i++) {
		imageInfo = malloc( sizeof( ImgInfo));
		imageInfo->extraInfo = firstImg->extraInfo;
		imageInfo->propList = plist_create( 15, 5);
		imgProp = apc_image_add_property( imageInfo, "index", PROPTYPE_INT, -1);
		if ( imgProp != NULL) imgProp->val.Int = i;
		list_add( imgInfo, ( Handle) imageInfo);
	    }
	}

	DOLBUG( "Have %d profile(s)\n", imgInfo->count);

	/* At this stage we must have fully prepared imgInfo list which all the items 
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
	    DOLBUG( "idx: %d\n", index);
	    for ( j = 0; j < imageInfo->propList->count; j++) {
		apc_image_clear_property( ( PImgProperty) list_at( imageInfo->propList, j));
	    }
	    plist_destroy( imageInfo->propList);
	    imageInfo->propList = plist_create( 5, 5);

	    gifChunk = ( GifImageDesc *) list_at( &gifChunks, index);
	    if ( readData) {
		data = ( GifPixelType **) list_at( &imageData, index);
	    }

	    imgProp = apc_image_add_property( imageInfo, "index", PROPTYPE_INT, -1);
	    if ( imgProp != NULL) imgProp->val.Int = index;

	    imgProp = apc_image_add_property( imageInfo, "format", PROPTYPE_STRING, -1);
	    if ( imgProp != NULL) imgProp->val.String = duplicate_string( gifFormat.id);

	    imgProp = apc_image_add_property( imageInfo, "type", PROPTYPE_INT, -1);
	    if ( imgProp != NULL) imgProp->val.Int = im256;

	    for ( j = 0; ( j < imageExtensions.count) && succeed; j++) {
		PGIFExtensions gifExtensions;
		gifExtensions = ( PGIFExtensions) list_at( &imageExtensions, j);
		DOLBUG( "Checking extensions for image %d (%d)\n", index, gifExtensions->index);
		if ( gifExtensions->index == index) {
		    PGIFExtInfo gifExtension;
		    int k, extIdx;
		    int processedCount = 0;

		    for ( k = 0; ( k < gifExtensions->extensions.count) && succeed; k++) {
			gifExtension = ( PGIFExtInfo) list_at( &gifExtensions->extensions, k);
			DOLBUG( "Analyzing extension with code %02x (%d out of %d)\n", 
				gifExtension->code,
				k,
				gifExtensions->extensions.count
			    );
			switch ( gifExtension->code) {
			    case GRAPHICS_EXT_FUNC_CODE:
				{
				    /* There cannot be more than one data block for this kind of extension. */
				    PGIFGraphControlExt controlData = ( PGIFGraphControlExt) list_at( &gifExtension->data, 0);

				    imgProp = apc_image_add_property( imageInfo, "disposalMethod", PROPTYPE_BYTE, -1);
				    if ( imgProp != NULL) imgProp->val.Byte = controlData->disposalMethod;

				    imgProp = apc_image_add_property( imageInfo, "userInput", PROPTYPE_BYTE, -1);
				    if ( imgProp != NULL) imgProp->val.Byte = controlData->userInput;

				    imgProp = apc_image_add_property( imageInfo, "delayTime", PROPTYPE_INT, -1);
				    if ( imgProp != NULL) imgProp->val.Int = controlData->delayTime * 10;

				    if ( controlData->transparent) {
					imgProp = apc_image_add_property( imageInfo, "transparentColorIndex", PROPTYPE_BYTE, -1);
					if ( imgProp != NULL) imgProp->val.Byte = controlData->transparentColorIndex;
				    }

				    gifExtension->processed = true;
				}
				break;
			    case COMMENT_EXT_FUNC_CODE:
				{
				    PImgProperty imgProp;
				    int commentsPropIdx = list_first_that( imageInfo->propList, ( void *) property_name, "comments");
				    char **newPString;
				    int propIdx, propBaseIdx;
				    DOLBUG( "Adding comment to the property list of image #%d\n", index);
				    if ( commentsPropIdx == -1) {
					imgProp = apc_image_add_property( imageInfo, "comments", PROPTYPE_STRING, 0);
					if ( imgProp != NULL) imgProp->val.pString = NULL;
				    }
				    else {
					imgProp = ( PImgProperty) list_at( imageInfo->propList, commentsPropIdx);
				    }
				    if ( imgProp != NULL) {
					propBaseIdx = propIdx = imgProp->size; /* Index for a new comment. */
					imgProp->size += gifExtension->data.count;
					if ( ( newPString = realloc( imgProp->val.pString, imgProp->size * sizeof( char **))) == NULL) {
					    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
					    succeed = false;
					}
					else {
					    for ( ; propIdx < imgProp->size; propIdx++) {
						Byte *commentData = ( Byte *) list_at( &gifExtension->data, propIdx - propBaseIdx);
						imgProp->val.pString = newPString;
						imgProp->val.pString[ propIdx] = malloc( commentData[ 0] + 1);
						memcpy( imgProp->val.pString[ propIdx], commentData + 1, commentData[ 0]);
						imgProp->val.pString[ propIdx][ commentData[ 0]] = 0;
					    }
					}
				    }
				    
				    gifExtension->processed = true;
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

		    imgProp = NULL;

		    for ( k = 0, extIdx = 0; ( k < gifExtensions->extensions.count) && succeed; k++) {
			int n;
			PIMGBinaryData bin = NULL;
			Byte *p;

			gifExtension = ( PGIFExtInfo) list_at( &gifExtensions->extensions, k);
			if ( gifExtension->processed) {
			    DOLBUG( "Skipping extension with code %02x\n", gifExtension->code);
			    continue;
			}

			if ( imgProp == NULL) {
			    DOLBUG( "Adding `extensions' property for index %d\n", index);
			    imgProp = apc_image_add_property( imageInfo, "extensions", PROPTYPE_BIN, gifExtensions->extensions.count - processedCount);
			    if ( imgProp != NULL) imgProp->val.pBinary = malloc( sizeof( IMGBinaryData) * ( gifExtensions->extensions.count - processedCount));
			}

			bin = imgProp->val.pBinary + extIdx;
			extIdx++;
			bin->size = 1;
			for ( n = 0; n < gifExtension->data.count; n++) {
			    Byte *extData = ( Byte *) list_at( &gifExtension->data, n);
			    bin->size += extData[ 0] + 1;
			}
			p = bin->data = malloc( bin->size);
			if ( bin->data == NULL) {
			    __gif_seterror( GIFERRT_DRIVER, DERR_NOT_ENOUGH_MEMORY);
			    succeed = false;
			    continue;
			}
			*p++ = gifExtension->code;
			DOLBUG( "%02x(%d bytes total size):", gifExtension->code, bin->size);
			for ( n = 0; n < gifExtension->data.count; n++) {
			    int ti;
			    Byte *extData = ( Byte *) list_at( &gifExtension->data, n);
			    memcpy( p, extData, extData[ 0] + 1);
			    DOLBUG( "%p[", p);
			    for ( ti = 0; ti <= extData[ 0]; ti++) {
				DOLBUG( "%02x ", p[ ti]);
			    }
			    DOLBUG( "]\n");
			    p += extData[ 0] + 1;
			}
			DOLBUG( "\n");
		    }
		    break; /* extensions scanning loop */
		}
	    }

	    if ( readData) {
		char *buf;
		imgProp = apc_image_add_property( imageInfo, "name", PROPTYPE_STRING, -1);
		if ( imgProp != NULL) {
		    buf = malloc( PATH_MAX + 16);
		    snprintf( buf, PATH_MAX + 16, "%s:%d", filename, index);
		    imgProp->val.String = buf;
		}
	    }

	    if ( index == -1) {
		imgProp = apc_image_add_property( imageInfo, "width", PROPTYPE_INT, -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SWidth;

		imgProp = apc_image_add_property( imageInfo, "height", PROPTYPE_INT, -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SHeight;

		imgProp = apc_image_add_property( imageInfo, "colorResolution", PROPTYPE_INT, -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SColorResolution;

		imgProp = apc_image_add_property( imageInfo, "bgColor", PROPTYPE_INT, -1);
		if ( imgProp != NULL) imgProp->val.Int = gif->SBackGroundColor;

		if ( gif->SColorMap != NULL) {
		    int n;
		    imgProp = apc_image_add_property( imageInfo, "paletteSize", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gif->SColorMap->ColorCount;

		    imgProp = apc_image_add_property( imageInfo, "paletteBPP", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gif->SColorMap->ColorCount;

		    imgProp = apc_image_add_property( imageInfo, "palette", PROPTYPE_BYTE, 3 * gif->SColorMap->ColorCount);
		    if ( imgProp != NULL) imgProp->val.pByte = ( Byte *) malloc( imgProp->size);
		    for ( n = 0; n < gif->SColorMap->ColorCount; n++) {
			Byte *palEntry = ( imgProp->val.pByte + n * 3);
			palEntry[ 0] = gif->SColorMap->Colors[ n].Blue;
			palEntry[ 1] = gif->SColorMap->Colors[ n].Green;
			palEntry[ 2] = gif->SColorMap->Colors[ n].Red;
		    }
		}

		if ( readData) {
		    imgProp = apc_image_add_property( imageInfo, "data", PROPTYPE_BYTE, gif->SWidth * gif->SHeight);
		    if ( imgProp != NULL) imgProp->val.pByte = calloc( 1, gif->SWidth * gif->SHeight);

		    imgProp = apc_image_add_property( imageInfo, "lineSize", PROPTYPE_BYTE, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gif->SWidth;
		}
	    }
	    else if ( index >= 0) {
		ColorMapObject *palette;

		if ( index < imgDescCount) {
		    int n;

		    imgProp = apc_image_add_property( imageInfo, "width", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunk->Width;

		    imgProp = apc_image_add_property( imageInfo, "height", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunk->Height;

		    imgProp = apc_image_add_property( imageInfo, "type", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = im256;

		    imgProp = apc_image_add_property( imageInfo, "X", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunk->Left;

		    imgProp = apc_image_add_property( imageInfo, "Y", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunk->Top;

		    imgProp = apc_image_add_property( imageInfo, "interlaced", PROPTYPE_INT, -1);
		    if ( imgProp != NULL) imgProp->val.Int = gifChunk->Interlace ? 1 : 0;

		    palette = ( gifChunk->ColorMap != NULL ?
				gifChunk->ColorMap :
				gif->SColorMap);

		    imgProp = apc_image_add_property( imageInfo, "palette", PROPTYPE_BYTE, 3 * palette->ColorCount);
		    if ( imgProp != NULL) imgProp->val.pByte = malloc( imgProp->size);
		    for ( n = 0; n < palette->ColorCount; n++) {
			Byte *palEntry = ( imgProp->val.pByte + n * 3);
			palEntry[ 0] = palette->Colors[ n].Blue;
			palEntry[ 1] = palette->Colors[ n].Green;
			palEntry[ 2] = palette->Colors[ n].Red;
		    }

		    if ( readData) {
			if ( *data != NULL) {
			    int pixelCount = gifChunk->Width * gifChunk->Height;
			    int pass = 0, Y; /* Pass number for interlaced images. */
			    imgProp = apc_image_add_property( imageInfo, "data", PROPTYPE_BYTE, pixelCount);
			    if ( imgProp != NULL) {
				int ySrc, yDst;
				imgProp->val.pByte = malloc( sizeof( Byte) * pixelCount);
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
			    imgProp = apc_image_add_property( imageInfo, "lineSize", PROPTYPE_INT, -1);
			    if ( imgProp != NULL) imgProp->val.Int = gifChunk->Width;
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
    else { 
	DOLBUG( "*** FAILED!!! ***\n");
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
	    if ( data && *data) {
		free( *data);
		free( data);
	    }
	}
    }
    list_destroy( &gifChunks);
    list_destroy( &imageData);
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
    DOLBUG( "__gif_loadable( %s)\n", filename);
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
