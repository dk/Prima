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
 * System dependent image routines (unix, x11)
 */

#include "unix/guts.h"
#include "Image.h"
#include "unix/img_api.h"
#include "unix/gif_support.h"

#define APCIMG_VERSION		1

PList imgFormats = nil;
U32 maxPrereadSize = 0;

#define APCERRBUF_LENGTH 1024

#define APCERRT_NONE 0
#define APCERRT_DRIVER 1
#define APCERRT_INTERNAL 2
#define APCERRT_LIBC 3
static int apcErrorType = APCERRT_NONE;

/* apc_image internal error codes */
#define APCIMG_NO_ERROR 0
#define APCIMG_FORMAT_REGISTERED 1
#define APCIMG_EMPTY_FORMAT_LIST 2
#define APCIMG_NOT_ENOUGH_MEMORY 3
#define APCIMG_NO_FORMAT 4
#define APCIMG_INV_PROPERTY_TYPE 5
#define APCIMG_INV_PROPERTY_VAL 6
#define APCIMG_INV_INDEX_VAL 7 /* Obsolete for now; the code is free for use */
#define APCIMG_FORBIDDEN_READALL 8
static int apcErrorCode = APCIMG_NO_ERROR;
static int errDriverIdx = -1;

const char *
apc_image_get_error_message( char *errorMsgBuf, int bufLen)
{
    static char errBuf[ APCERRBUF_LENGTH];
    char *p = errBuf, *msg = "*** error missed ***";
    int len = APCERRBUF_LENGTH;

    if ( errorMsgBuf != NULL) {
	p = errorMsgBuf;
	len = bufLen;
    }

    strncpy( p, "no error", len);
    p[ len - 1] = 0;

    switch ( apcErrorType) {
	case APCERRT_NONE:
	    msg = "no error";
	    break;
	case APCERRT_LIBC:
	    msg = strerror( apcErrorCode);
	    break;
	case APCERRT_INTERNAL:
	    switch ( apcErrorCode) {
		case APCIMG_NO_ERROR:
		    msg = "no error";
		    break;
		case APCIMG_FORMAT_REGISTERED:
		    msg = "image format already registered";
		    break;
		case APCIMG_EMPTY_FORMAT_LIST:
		    msg = "no formats registered";
		    break;
		case APCIMG_NOT_ENOUGH_MEMORY:
		    msg = "not enough memory";
		    break;
		case APCIMG_NO_FORMAT:
		    msg = "no appropriate format found";
		    break;
		case APCIMG_INV_PROPERTY_TYPE:
		    msg = "invalid property type";
		    break;
		case APCIMG_INV_PROPERTY_VAL:
		    msg = "invalid property value";
		    break;
		case APCIMG_INV_INDEX_VAL:
		    msg = "invalid 'index' property value: can't be -1 for load";
		    break;
		case APCIMG_FORBIDDEN_READALL:
		    msg = "'readAll' property is not allowed while more than one profile specified";
		    break;
		default:
		    msg = "*** APC internal: unknown error code";
	    }
	    break;
	case APCERRT_DRIVER:
	    {
		PImgFormat imgf;
		if ( ( errDriverIdx >= 0) && ( errDriverIdx < imgFormats->count)) {
		    DOLBUG( "Asking driver %d for error\n", errDriverIdx);
		    imgf = ( PImgFormat) list_at( imgFormats, errDriverIdx);
		    msg = ( char *) imgf->getError( NULL, 0);
		}
		else {
		    msg = "internal error: wrong driver index!";
		}
	    }
	    break;
	default:
	    msg = "*** APC internal: unknown error type";
    }
    strncpy( p, msg, len);
    p[ len - 1] = 0;

    return p;
}

static void
__apc_image_set_error( int type, int code, ...)
{
    apcErrorType = type;
    apcErrorCode = code;
    if ( type == APCERRT_DRIVER) {
	va_list args;
	va_start( args, code);
	errDriverIdx = va_arg( args, int);
	va_end( args);
    }
}

static void
__apc_image_clear_error()
{
    __apc_image_set_error( APCERRT_NONE, APCIMG_NO_ERROR);
}

/* image & bitmaps */
Bool
apc_image_create( Handle self)
{
    DOLBUG( "apc_image_create()\n");
    return false;
}

void
apc_image_destroy( Handle self)
{
   DEFXX;
   if ( XX-> imageCache) {
      XDestroyImage( XX-> imageCache);
      XX-> imageCache = nil;
   }
}

Bool
apc_image_begin_paint( Handle self)
{
    DOLBUG( "apc_image_begin_paint()\n");
    return false;
}

Bool
apc_image_begin_paint_info( Handle self)
{
    DOLBUG( "apc_image_begin_paint_info()\n");
    return false;
}

void
apc_image_end_paint( Handle self)
{
    DOLBUG( "apc_image_end_paint()\n");
}

void
apc_image_end_paint_info( Handle self)
{
    DOLBUG( "apc_image_end_paint_info()\n");
}

void
apc_image_update_change( Handle self)
{
   DEFXX;
   if ( XX-> imageCache) {
      XDestroyImage( XX-> imageCache);
      XX-> imageCache = nil;
   }
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
    DOLBUG( "apc_dbm_create()\n");
    return false;
}

void
apc_dbm_destroy( Handle self)
{
    DOLBUG( "apc_dbm_destroy()\n");
}

static Bool
image_format_exists( Handle item, void *params)
{
    const char *format_name = ( const char *) params;
    PImgFormat imgFormat = ( PImgFormat) item;
    return ( strcasecmp( format_name, imgFormat->id) == 0);
}

/*
 * apc_register_image_format returns 0 on success or error code. For
 * the first time the only error code available is 1.
 */
Bool
apc_register_image_format( int version, PImgFormat imgf)
{
    if ( ( version > 1)
	 || ( version < 1)
	 || ( imgf == nil)
	 || ( imgf->id == nil)
	 || ( imgf->capabilities == nil)
	 || ( imgf->propertyList == nil)
	 || ( imgf->load == nil)
	 || ( imgf->save == nil)
	 || ( imgf->is_loadable == nil)
	 || ( imgf->is_storable == nil)
	 || ( imgf->getInfo == nil)) {
	/* Invalid parameters */
	return 1;
    }

    if ( imgFormats == nil) {
	imgFormats = plist_create( 10, 5);
    }

    if ( list_first_that( imgFormats, image_format_exists, ( void*)imgf->id) >= 0) {
	/* The format already registered */
	__apc_image_set_error( APCERRT_INTERNAL, APCIMG_FORMAT_REGISTERED);
	return false;
    }

    list_add( imgFormats, ( Handle) imgf);

    if ( imgf->preread_size > maxPrereadSize) {
	maxPrereadSize = imgf->preread_size;
    }

    return true;
}

typedef struct {
    const char *filename; /* Image file name */
    int fd;               /* Image file descriptor */
    char *preread_buf;    /* Preread data */
    int preread_size;     /* This field's value could differ from
			     maxPrereadSize depending on result 
			     of read() or further calls to
			     apc_image_fetch_more. */
    const char *desiredFormat;
    int error;            /* Error code if error occured during checks. */
} __ImgLoadData, *__PImgLoadData;

static __PImgLoadData
load_prepare_data()
{
    __PImgLoadData load_data = ( __PImgLoadData) malloc( sizeof( __ImgLoadData));
    if ( load_data != nil) {
	load_data->preread_buf = (load_data->preread_size = maxPrereadSize) > 0 ?
	    ( char*) malloc( load_data->preread_size) :
	    nil;
	load_data->error = 0;
	load_data->desiredFormat =
	load_data->filename = nil;
	load_data->fd = -1; /* The only valid value to indicate uninitialized file descriptor */
    }
    else {
	__apc_image_set_error( APCERRT_INTERNAL, APCIMG_NOT_ENOUGH_MEMORY);
    }
    return load_data;
}

static void
load_cleanup_data( __PImgLoadData *load_data)
{
    if ( *load_data != nil) {
	if ( ( *load_data)->preread_buf != nil) {
	    free( ( *load_data)->preread_buf);
	}
	free( *load_data);
	*load_data = nil;
    }
}

static Bool
load_img_loadable( Handle item, void *params)
{
    __PImgLoadData load_data = ( __PImgLoadData) params;
    PImgFormat imgFormat = ( PImgFormat) item;
    DOLBUG( "checking %s whether it is loadable as %s\n", load_data->filename, imgFormat->id);
    if ( ( load_data->preread_size < imgFormat->preread_size)
	 || ( ( load_data->desiredFormat != nil)
	      && ( strcasecmp( load_data->desiredFormat, imgFormat->id) != 0))) {
	return false;
    }
    return imgFormat->is_loadable( load_data->fd, 
				   load_data->filename, 
				   load_data->preread_buf, 
				   load_data->preread_size);
}

static Bool
__is_boolean_value( const char *val)
{
    return ( ( strcasecmp( val, "yes") == 0)
	     || ( strcasecmp( val, "on") == 0)
	     || ( strcasecmp( val, "1") == 0)
	     || ( strcasecmp( val, "no") == 0)
	     || ( strcasecmp( val, "off") == 0)
	     || ( strcasecmp( val, "0") == 0));
}

static Bool
__boolean_value( const char *val)
{
    return ( ( strcasecmp( val, "yes") == 0)
	     || ( strcasecmp( val, "on") == 0)
	     || ( strcasecmp( val, "1") == 0));
}

Bool
__apc_image_correct_properties( PImgInfo imageInfo, PImgFormat imgFormat, Bool readData, Bool *readAll)
{
    PImgProps fmtProps = imgFormat->propertyList;
    PImgProperty imgProp, outImgProp;
    PImgInfo outImageInfo;
    PList propList = imageInfo->propList;
    int i, j, n;
    Bool rc = true;

    rc = ( outImageInfo = img_info_create( propList->count)) != nil;

    DOLBUG( "Have %d properties on entering __apc_image_correct_properties\n", propList->count);
    for ( i = ( propList->count - 1); ( i >= 0) && rc; i--) {
	Bool isExtraInfo, isReadAll;
	imgProp = ( PImgProperty) list_at( propList, i);
	isExtraInfo = ( strcmp( imgProp->name, "extraInfo") == 0);
	isReadAll = ( strcmp( imgProp->name, "readAll") == 0);
	DOLBUG( "Correcting ``%s''\n", imgProp->name);
	if ( isExtraInfo || isReadAll) {
	    if ( ( imgProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY) {
		// We don't expect an array here.
		__apc_image_set_error( APCERRT_INTERNAL, APCIMG_INV_PROPERTY_TYPE);
		DOLBUG( "Property `%s' is of a wrong type\n", imgProp->name);
		rc = false;
	    }
	    else if ( ! __is_boolean_value( imgProp->val.String)) {
		__apc_image_set_error( APCERRT_INTERNAL, APCIMG_INV_PROPERTY_VAL);
		DOLBUG( "Property %s: wrong value \'%s\'\n", imgProp->name, imgProp->val.String);
		rc = false;
	    }
	    else {
		if ( isExtraInfo) {
		    outImageInfo->extraInfo = __boolean_value( imgProp->val.String);
		}
		else if ( isReadAll) {
		    *readAll = __boolean_value( imgProp->val.String);
		}
	    }
	    continue;
	}
	for ( j = 0; fmtProps[ j].name; j++) {
	    if ( strcmp( imgProp->name, fmtProps[ j].name) == 0) {
		break;
	    }
	}
	if ( fmtProps[ j].name) {
	    if ( ( ( fmtProps[ j].type[ 1] == '*') 
		   && ( ( imgProp->flags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY))
		 || ( ( fmtProps[ j].type[ 1] == '\0') 
		      && ( ( imgProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY))) {
		__apc_image_set_error( APCERRT_INTERNAL, APCIMG_INV_PROPERTY_TYPE);
		DOLBUG( "Property %s: wrong type\n", imgProp->name);
		rc = false;
	    }
	    else {
		switch ( fmtProps[ j].type[ 0]) {
		    case 'i':
			if ( fmtProps[ j].type[ 0] == '*') {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_INT,
								       imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, atoi( imgProp->val.pString[ n]));
			    }
			}
			else {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_INT,
								       0,
								       atoi( imgProp->val.String))) != nil;
			}
			break;
		    case 'n':
			if ( fmtProps[ j].type[ 0] == '*') {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_DOUBLE,
								       imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, atof( imgProp->val.pString[ n]));
			    }
			}
			else {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_DOUBLE,
								       0,
								       atof( imgProp->val.String))) != nil;
			}
			break;
		    case 's':
			if ( fmtProps[ j].type[ 0] == '*') {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_STRING,
								       imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, imgProp->val.pString[ n]);
			    }
			}
			else {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_STRING,
								       0,
								       imgProp->val.String)) != nil;
			}
			break;
		    case 'b':
			if ( fmtProps[ j].type[ 0] == '*') {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_BYTE,
								       imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, atoi( imgProp->val.pString[ n]));
			    }
			}
			else {
			    rc = ( outImgProp = img_info_add_property( outImageInfo,
								       imgProp->name,
								       PROPTYPE_BYTE,
								       0,
								       atoi( imgProp->val.String))) != nil;
			}
			break;
		    default:
			DOLBUG( "Unsupported property type `%s' for property `%s'\n", 
				fmtProps[ j].type[ 0],
				imgProp->name
			    );
			rc = false;
		}
		if ( rc) {
		    outImgProp->id = fmtProps[ j].id;
		}
	    }
	}
    }

    if ( rc) {
	/* Clearing the old properties list. */
	img_destroy_properties( imageInfo->propList);
	/* And duplicating the new ImgInfo structure. */
	memcpy( imageInfo, outImageInfo, sizeof( ImgInfo));
	/* propList is now being contained in imageInfo... */
	outImageInfo->propList = nil;
    }

    img_info_destroy( outImageInfo);

    DOLBUG( "__apc_image_correct_properties: returning %d\n", ( int) rc);
    DOLBUG( "Have %d properties on exiting __apc_image_correct_properties\n", imageInfo->propList->count);

    return rc;
}

/*
 * readData - false value means that only information about the image should be obtained.
 *            I.e. getInfo call must be issued.
 */
Bool
apc_image_read( const char *filename, PList imgInfo, Bool readData)
{
    Bool rc = false;
    char *format = nil;
    __apc_image_clear_error();
    if ( imgFormats != nil) {
	__PImgLoadData load_data = load_prepare_data();
	if ( load_data != nil) {
	    load_data->filename = filename;
	    load_data->desiredFormat = format;
/* Local hack. Must be removed in future! */
#ifndef O_BINARY
#define O_BINARY 0
#endif
	    load_data->fd = open( load_data->filename, O_RDONLY | O_BINARY);
	    if ( load_data->fd != -1) {
		load_data->preread_size = read( load_data->fd, load_data->preread_buf, load_data->preread_size);
		if ( load_data->preread_size != -1) {
		    int format_idx = list_first_that( imgFormats, load_img_loadable, load_data);
		    DOLBUG( "format_idx: %d\n", format_idx);
		    if ( format_idx != -1) {
			int i;
			Bool correction_succeed = true, readAll = false;
			PImgFormat imgFormat = ( PImgFormat) list_at( imgFormats, format_idx);
			DOLBUG( "%s for %s as %s\n", 
				( readData ? "Loading image" : "Getting info"), 
				load_data->filename, 
				imgFormat->id
			    );
			for ( i = 0; ( i < imgInfo->count) && correction_succeed; i++) {
			    PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
			    correction_succeed = __apc_image_correct_properties( imageInfo, 
										 imgFormat, 
										 readData, 
										 &readAll
				);
			}
			if ( readAll && correction_succeed && ( imgInfo->count > 1)) {
			    __apc_image_set_error( APCERRT_INTERNAL, APCIMG_FORBIDDEN_READALL);
			    correction_succeed = false;
			}
			if ( correction_succeed) {
			    rc = ( readData ?
				   imgFormat->load( load_data->fd, load_data->filename, imgInfo, readAll) :
				   imgFormat->getInfo( load_data->fd, load_data->filename, imgInfo, readAll));
			    if ( ! rc) {
				DOLBUG( "Error in driver %d\n", format_idx);
				__apc_image_set_error( APCERRT_DRIVER, 0, format_idx);
			    }
			}
		    }
		    else {
			__apc_image_set_error( APCERRT_INTERNAL, APCIMG_NO_FORMAT);
		    }
		}
		else {
		    __apc_image_set_error( APCERRT_LIBC, errno);
		}
	    }
	    else {
		__apc_image_set_error( APCERRT_LIBC, errno);
	    }
	    load_cleanup_data( &load_data);
	}
    }
    else {
	__apc_image_set_error( APCERRT_INTERNAL, APCIMG_EMPTY_FORMAT_LIST);
    }
    DOLBUG( "apc_image_read: returning %d\n", ( int) rc);
    return rc;
}

typedef struct {
    const char *filename;
    PList imgInfo;
    const char *desiredFormat;
} __ImgSaveData, *__PImgSaveData;

static __PImgSaveData
save_prepare_data()
{
    __PImgSaveData save_data = ( __PImgSaveData) malloc( sizeof( __ImgSaveData));
    if ( save_data != nil) {
	save_data->filename = nil;
    }
    return save_data;
}

static void
save_cleanup_data( __PImgSaveData *save_data)
{
    if ( *save_data != nil) {
	free( *save_data);
	*save_data = nil;
    }
}

static Bool
save_img_storable( Handle item, void *params)
{
    PImgFormat imgFormat = ( PImgFormat) item;
    __PImgSaveData save_data = ( __PImgSaveData) params;
    if ( ( save_data->desiredFormat != nil)
	 && ( strcasecmp( save_data->desiredFormat, imgFormat->id) != 0)) {
	return false;
    }
    return imgFormat->is_storable( save_data->filename, save_data->imgInfo);
}

Bool
apc_image_save( const char *filename, const char *format, PList imgInfo)
{
    Bool rc = false;
    __apc_image_clear_error();
    if ( ( imgInfo != nil) && ( filename != nil)) {
	__PImgSaveData save_data = save_prepare_data();
	if ( save_data != nil) {
	    int format_idx;
	    save_data->filename = filename;
	    save_data->desiredFormat = format;
	    save_data->imgInfo = imgInfo;
	    format_idx = list_first_that( imgFormats, save_img_storable, ( void*) save_data);
	    if ( format_idx != -1) {
		PImgFormat imgFormat = ( PImgFormat) list_at( imgFormats, format_idx);
		rc = ( imgFormat->save( save_data->filename, save_data->imgInfo) == 0);
	    }
	    save_cleanup_data( &save_data);
	}
    }
    return rc;
}

/* This is driver suppport only function. */
Bool
apc_image_fetch_more( __PImgLoadData load_data, int preread_size)
{
    char *newbuf;
    int rd;
    Bool rc = false;

    if ( preread_size <= load_data->preread_size) {
	return true;
    }
    if ( lseek( load_data->fd, 0, SEEK_SET) != -1) {
	if ( ( newbuf = ( char *) malloc( preread_size)) != nil) {
	    rd = read( load_data->fd, newbuf, preread_size);
	    if ( rd < preread_size) {
		free( newbuf);
	    }
	    else {
		free( load_data->preread_buf);
		load_data->preread_buf = newbuf;
		load_data->preread_size = preread_size;
		rc = true;
	    }
	}
    }
    return rc;
}

/*  PImgProperty */
/*  apc_image_add_property( PImgInfo imageInfo, const char *propName, U16 propType, int propArraySize) */
/*  { */
/*      PImgProperty imgProp; */

/*      if ( ( imageInfo == nil) || ( imageInfo->propList == nil) || ( propName == nil)) { */
/*  	return nil; */
/*      } */

/*      imgProp = malloc( sizeof( ImgProperty)); */
/*      if ( imgProp != nil) { */
/*  	imgProp->name = duplicate_string( propName); */
/*  	imgProp->flags = propType; */
/*  	imgProp->size = propArraySize; */
/*  	list_add( imageInfo->propList, ( Handle) imgProp); */
/*      } */

/*      return imgProp; */
/*  } */

/*  void */
/*  apc_image_clear_property( PImgProperty imgProp) */
/*  { */
/*      if ( imgProp != nil) { */
/*  	free( imgProp->name); */
/*  	if ( imgProp->size == -1) { */
/*  	    switch ( imgProp->flags & PROPTYPE_MASK) { */
/*  		case PROPTYPE_STRING: */
/*  		    free( imgProp->val.String); */
/*  		    break; */
/*  		case PROPTYPE_BIN: */
/*  		    free( imgProp->val.Binary.data); */
/*  		    break; */
/*  	    } */
/*  	} */
/*  	else { */
/*  	    if ( imgProp->size > 0) { */
/*  		int i; */
/*  		switch ( imgProp->flags & PROPTYPE_MASK) { */
/*  		    case PROPTYPE_STRING: */
/*  			for ( i = 0; i < imgProp->size; i++) { */
/*  			    free( imgProp->val.pString[ i]); */
/*  			} */
/*  			break; */
/*  		    case PROPTYPE_BIN: */
/*  			for ( i = 0; i < imgProp->size; i++) { */
/*  			    if ( imgProp->val.pBinary[ i].data != nil) { */
/*  				free( imgProp->val.pBinary[ i].data); */
/*  			    } */
/*  			} */
/*  			break; */
/*  		} */
/*  		switch ( imgProp->flags & PROPTYPE_MASK) { */
/*  		    case PROPTYPE_INT: */
/*  			free( imgProp->val.pInt); */
/*  			break; */
/*  		    case PROPTYPE_DOUBLE: */
/*  			free( imgProp->val.pDouble); */
/*  			break; */
/*  		    case PROPTYPE_STRING: */
/*  			free( imgProp->val.pString); */
/*  			break; */
/*  		    case PROPTYPE_BIN: */
/*  			free( imgProp->val.pBinary); */
/*  			break; */
/*  		    case PROPTYPE_BYTE: */
/*  		    default: */
/*  			free( imgProp->val.pByte); */
/*  			break; */
/*  		} */
/*  	    } */
/*  	} */
/*      } */
/*  } */

/*  void */
/*  apc_image_free_property( PImgProperty imgProp) */
/*  { */
/*      apc_image_clear_property( imgProp); */
/*      free( imgProp); */
/*  } */

void
prima_init_image_subsystem( void)
{
   apc_register_image_format( APCIMG_VERSION, &gifFormat);
}

/*  void */
/*  ___apc_image_test() */
/*  { */
/*      PList imgInfo = nil; */
/*      PImgInfo imageInfo; */
/*      PImgProperty imgProp; */

/*      DOLBUG( "___apc_image_test\n"); */
/*      DOLBUG( "registering GIF format: %s\n", apc_register_image_format( APCIMG_VERSION, &gifFormat) ? "ok" : "failed"); */

/*      imgInfo = plist_create( 5, 5); */

/*      imageInfo = ( PImgInfo) malloc( sizeof( ImgInfo)); */
/*      imageInfo->propList = plist_create( 5, 5); */
/*      imgProp = ( PImgProperty) malloc( sizeof( ImgProperty)); */
/*      imgProp->name = duplicate_string( "index"); */
/*      imgProp->size = -1; */
/*      imgProp->val.Int = 5; */
/*      list_add( imageInfo->propList, ( Handle) imgProp); */
/*      list_add( imgInfo, ( Handle) imageInfo); */
/*      DOLBUG( "First one\n"); */

/*      imageInfo = ( PImgInfo) malloc( sizeof( ImgInfo)); */
/*      imageInfo->propList = plist_create( 5, 5); */
/*      imgProp = ( PImgProperty) malloc( sizeof( ImgProperty)); */
/*      imgProp->name = duplicate_string( "index"); */
/*      imgProp->size = -1; */
/*      imgProp->val.Int = 2; */
/*      list_add( imageInfo->propList, ( Handle) imgProp); */
/*      list_add( imgInfo, ( Handle) imageInfo); */

/*      imageInfo = ( PImgInfo) malloc( sizeof( ImgInfo)); */
/*      imageInfo->propList = plist_create( 5, 5); */
/*      imgProp = ( PImgProperty) malloc( sizeof( ImgProperty)); */
/*      imgProp->name = duplicate_string( "index"); */
/*      imgProp->size = -1; */
/*      imgProp->val.Int = 10; */
/*      list_add( imageInfo->propList, ( Handle) imgProp); */
/*      imgProp = ( PImgProperty) malloc( sizeof( ImgProperty)); */
/*      imgProp->name = duplicate_string( "index1"); */
/*      imgProp->size = -1; */
/*      imgProp->val.Int = 16; */
/*      list_add( imageInfo->propList, ( Handle) imgProp); */
/*      list_add( imgInfo, ( Handle) imageInfo); */

/*      imageInfo = ( PImgInfo) malloc( sizeof( ImgInfo)); */
/*      imageInfo->propList = plist_create( 5, 5); */
/*      imgProp = ( PImgProperty) malloc( sizeof( ImgProperty)); */
/*      imgProp->name = duplicate_string( "index"); */
/*      imgProp->size = -1; */
/*      imgProp->val.Int = 1; */
/*      list_add( imageInfo->propList, ( Handle) imgProp); */
/*      list_add( imgInfo, ( Handle) imageInfo); */

/*      imageInfo = ( PImgInfo) malloc( sizeof( ImgInfo)); */
/*      imageInfo->propList = plist_create( 5, 5); */
/*      imgProp = ( PImgProperty) malloc( sizeof( ImgProperty)); */
/*      list_add( imgInfo, ( Handle) imageInfo); */

/*      DOLBUG( "Getting info about image...\n"); */

/*      if ( ! apc_image_read( "/home/voland/tmp/eagle8.gif", imgInfo, true)) { */
/*  	DOLBUG( "getinfo failed: "); */
/*  	DOLBUG( "%s\n", apc_image_get_error_message( NULL, 0)); */
/*      } */
/*  } */
