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
 *
 * $Id$
 */

/*
 * Created by Vadim Belman <voland@plab.ku.dk>, April 1999.
 */

/*
 * System independent image routines
 */

#include "img_api.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <fcntl.h>
#include "gif_support.h"

#ifdef __cplusplus
extern "C" {
#endif


#define APCIMG_VERSION		1

PList imgFormats = nil;
U32 maxPrereadSize = 0;

#define APCERRBUF_LENGTH 1024
static char apcImageErrorMsg[ APCERRBUF_LENGTH];

const char *
apc_image_get_error_message( char *errorMsgBuf, int bufLen)
{
    const char *p = apcImageErrorMsg;
    if ( errorMsgBuf) {
	p = strncpy( errorMsgBuf, apcImageErrorMsg, bufLen - 1);
	errorMsgBuf[ bufLen - 1] = 0; /* Strange design of strncpy, isn't it? 8-\ */
    }
    return p;
}

static void
__apc_image_set_error( const char *fmt, ...)
{
    va_list args;
    va_start( args, fmt);
    vsnprintf( apcImageErrorMsg, APCERRBUF_LENGTH, fmt, args);
    va_end( args);
}

static void
__apc_image_clear_error( void)
{
    apcImageErrorMsg[ 0] = 0;
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

    if ( list_first_that( imgFormats, image_format_exists, ( void *) imgf->id) >= 0) {
	/* The format already registered */
	__apc_image_set_error( "apc_register_image_format: a driver for format id ``%s'' already registered", imgf->id);
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
    Byte *preread_buf;    /* Preread data */
    int preread_size;     /* This field's value could differ from
			     maxPrereadSize depending on result
			     of read() or further calls to
			     apc_image_fetch_more. */
    const char *desiredFormat;
    int error;            /* Error code if error occured during checks. */
} __ImgLoadData, *__PImgLoadData;

static __PImgLoadData
load_prepare_data( void)
{
    __PImgLoadData load_data = alloc1( __ImgLoadData);
    if ( load_data != nil) {
	load_data->preread_buf = (load_data->preread_size = maxPrereadSize) > 0 ?
	    allocb( load_data->preread_size) :
	    nil;
	load_data->error = 0;
	load_data->desiredFormat =
	load_data->filename = nil;
	load_data->fd = -1; /* The only valid value to indicate uninitialized file descriptor */
    }
    else {
	__apc_image_set_error( "load_prepare_data: out of memory");
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
/*    DOLBUG( "checking %s whether it is loadable as %s\n", load_data->filename, imgFormat->id); */
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
__is_boolean_value( void *val)
{
    return ( ( strcasecmp((char*) val, "yes") == 0)
	     || ( strcasecmp((char*) val, "on") == 0)
	     || ( strcasecmp((char*) val, "1") == 0)
	     || ( strcasecmp((char*) val, "no") == 0)
	     || ( strcasecmp((char*) val, "off") == 0)
	     || ( strcasecmp((char*) val, "0") == 0));
}

static Bool
__boolean_value( void *val)
{
    return ( ( strcasecmp((char*) val, "yes") == 0)
	     || ( strcasecmp((char*) val, "on") == 0)
	     || ( strcasecmp((char*) val, "1") == 0));
}

static Bool
__apc_image_correct_property( PImgProps fmtProps,
			      PList propList,
			      PList outPropList,
			      Bool *readAll,
			      Bool *extraInfo,
			      int *convertionAllowed
    )
{
    PImgProperty imgProp, outImgProp = nil;
    int i, j, n;
    Bool rc = true;

    for ( i = ( propList->count - 1); ( i >= 0); i--) {
	Bool isExtraInfo, isReadAll, isConvertionAllowed;
	imgProp = ( PImgProperty) list_at( propList, i);
	if ( readAll || extraInfo || convertionAllowed) {
	    isExtraInfo = ( strcmp( imgProp->name, "extraInfo") == 0);
	    isReadAll = ( strcmp( imgProp->name, "readAll") == 0);
	    isConvertionAllowed = ( strcmp( imgProp->name, "convertionAllowed") == 0);
	    if ( isExtraInfo
		 || isReadAll
		 || isConvertionAllowed) {
		if ( ( imgProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY) {
		    // We don't expect an array here.
		    __apc_image_set_error( "__apc_image_correct_property: standard property ``%s'' cannot be an array", imgProp->name);
		    rc = false;
		}
		else if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
		    __apc_image_set_error( "__apc_image_correct_property: property ``%s'' must be of scalar type", imgProp->name);
		    rc = false;
		}
		else if ( ! isConvertionAllowed
			  && ! __is_boolean_value( imgProp->val.Binary.data)) {
		    __apc_image_set_error( "__apc_image_correct_property: property ``%s'' does not contain a boolean value as it supposed to be", imgProp->name);
		    rc = false;
		}
		else {
		    if ( isExtraInfo && extraInfo) {
			*extraInfo = __boolean_value( imgProp->val.Binary.data);
		    }
		    if ( isReadAll && readAll) {
			*readAll = __boolean_value( imgProp->val.Binary.data);
		    }
		    if ( isConvertionAllowed && convertionAllowed) {
			*convertionAllowed = atoi(( const char *) imgProp->val.Binary.data);
		    }
		}
		/* That was readAll, extraInfo or convertionAllowed properties... */
		continue;
	    }
	}
	for ( j = 0; fmtProps[ j].name; j++) {
	    if ( strcmp( imgProp->name, fmtProps[ j].name) == 0) {
		break;
	    }
	}
	if ( fmtProps[ j].name) {
	    if ( ( fmtProps[ j].type[ 1] == '*')
		 && ( ( imgProp->flags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY)) {
		__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be an array", imgProp->name);
		rc = false;
	    }
	    else if ( ( fmtProps[ j].type[ 1] == '\0')
		      && ( ( imgProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY)) {
		__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is not expected to be an array", imgProp->name);
		rc = false;
	    }
	    else if ( ( fmtProps[ j].type[ 0] == 'p')
		      && ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_PROP)) {
		__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be a profile", imgProp->name);
		rc = false;
	    }
	    else {
/*		DOLBUG( "__apc_image_correct_property: correcting ``%s''\n", imgProp->name); */
		switch ( fmtProps[ j].type[ 0]) {
		    case 'i':
			if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
			    if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_INT) {
				__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be an integer", imgProp->name);
			    }
			    else {
				rc = ( outImgProp = img_duplicate_property( imgProp)) != nil;
				list_add( outPropList, ( Handle) outImgProp);
			    }
			}
			else {
			    if ( fmtProps[ j].type[ 1] == '*') {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_INT,
								       imgProp->used)) != nil;
				for ( n = 0; n < imgProp->used && rc; n++) {
				    rc = img_push_property_value( outImgProp, atoi(( const char *) imgProp->val.pBinary[ n].data));
				}
			    }
			    else {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_INT,
								       0,
								       atoi(( const char *) imgProp->val.Binary.data))) != nil;
			    }
			}
			break;
		    case 'n':
			if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
			    if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_DOUBLE) {
				__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be a double", imgProp->name);
			    }
			    else {
				rc = ( outImgProp = img_duplicate_property( imgProp)) != nil;
				list_add( outPropList, ( Handle) outImgProp);
			    }
			}
			else {
			    if ( fmtProps[ j].type[ 1] == '*') {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_DOUBLE,
								       imgProp->used)) != nil;
				for ( n = 0; n < imgProp->used && rc; n++) {
				    rc = img_push_property_value( outImgProp, atof(( const char *) imgProp->val.pBinary[ n].data));
				}
			    }
			    else {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_DOUBLE,
								       0,
								       atof(( const char *) imgProp->val.Binary.data))) != nil;
			    }
			}
			break;
		    case 's':
			if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
			    if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_STRING) {
				__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be a string", imgProp->name);
			    }
			    else {
				rc = ( outImgProp = img_duplicate_property( imgProp)) != nil;
				list_add( outPropList, ( Handle) outImgProp);
			    }
			}
			else {
			    if ( fmtProps[ j].type[ 1] == '*') {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_STRING,
								       imgProp->used)) != nil;
				for ( n = 0; n < imgProp->used && rc; n++) {
				    rc = img_push_property_value( outImgProp, imgProp->val.pBinary[ n].data);
				}
			    }
			    else {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_STRING,
								       0,
								       imgProp->val.Binary.data)) != nil;
			    }
			}
			break;
		    case 'b':
			if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
			    if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BYTE) {
				__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be a byte", imgProp->name);
			    }
			    else {
				rc = ( outImgProp = img_duplicate_property( imgProp)) != nil;
				list_add( outPropList, ( Handle) outImgProp);
			    }
			}
			else {
			    if ( fmtProps[ j].type[ 1] == '*') {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_BYTE,
								       imgProp->used)) != nil;
				for ( n = 0; n < imgProp->used && rc; n++) {
				    rc = img_push_property_value( outImgProp, atoi(( const char *) imgProp->val.pBinary[ n].data));
				}
			    }
			    else {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_BYTE,
								       0,
								       atoi(( const char *) imgProp->val.Binary.data))) != nil;
			    }
			}
			break;
		    case 'B':
			/* The size of binary data must be decreased by 1 because it was
			   choosen in a way to preserve trailing zero in the Perl string. */
			if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
			    if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
				__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be a binary data", imgProp->name);
			    }
			    else {
				rc = ( outImgProp = img_duplicate_property( imgProp)) != nil;
				list_add( outPropList, ( Handle) outImgProp);
			    }
			}
			else {
			    if ( fmtProps[ j].type[ 1] == '*') {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_BIN,
								       imgProp->used)) != nil;
				for ( n = 0; n < imgProp->used && rc; n++) {
				    rc = img_push_property_value( outImgProp, imgProp->val.pBinary[ n].size - 1, imgProp->val.pBinary[ n].data);
				}
			    }
			    else {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_BIN,
								       0,
								       imgProp->val.Binary.size - 1,
								       imgProp->val.Binary.data)) != nil;
			    }
			}
			break;
		    case 'p':
			if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
			    if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_PROP) {
				__apc_image_set_error( "__apc_image_correct_property: property ``%s'' is expected to be a set of properties", imgProp->name);
			    }
			    else {
				rc = ( outImgProp = img_duplicate_property( imgProp)) != nil;
				list_add( outPropList, ( Handle) outImgProp);
			    }
			}
			else {
			    if ( fmtProps[ j].type[ 1] == '*') {
				rc = ( outImgProp = img_push_property( outPropList,
								       imgProp->name,
								       PROPTYPE_ARRAY | PROPTYPE_PROP,
								       imgProp->used)) != nil;
				for ( n = 0; n < imgProp->used && rc; n++) {
				    rc = img_push_property_value( outImgProp,
								  imgProp->val.pProperties[ n].count)
					&& __apc_image_correct_property( fmtProps[ j].subProps,
									 imgProp->val.pProperties + n,
									 outImgProp->val.pProperties + n,
									 nil,
									 nil,
									 nil
					    );
				}
			    }
			    else {
				rc = ( ( outImgProp = img_push_property( outPropList,
									 imgProp->name,
									 PROPTYPE_PROP,
									 0,
									 imgProp->val.Properties.count)) != nil)
				    && __apc_image_correct_property( fmtProps[ j].subProps,
								     &imgProp->val.Properties,
								     &outImgProp->val.Properties,
								     nil,
								     nil,
								     nil
					);
			    }
			}
			break;
		    default:
			__apc_image_set_error( "__apc_image_correct_property: unsupported property type ``%s'' of property ``%s''\n",
					       fmtProps[ j].type,
					       imgProp->name
			    );
			rc = false;
		}
		if ( rc) {
		    if ( !outImgProp) {
			__apc_image_set_error( "__apc_image_correct_property: internal error #674\n");
		    } else {
		       outImgProp->id = fmtProps[ j].id;
		    }
		}
	    }
	}
    }
    return rc;
}

static Bool
__apc_image_correct_properties( PImgInfo imageInfo, PImgFormat imgFormat, Bool *readAll)
{
    PImgInfo outImageInfo;
    PList propList = imageInfo->propList;
    Bool rc;

    rc = ( ( outImageInfo = img_info_create( propList->count)) != nil)
	&& __apc_image_correct_property( imgFormat->propertyList,
					 propList,
					 outImageInfo->propList,
					 readAll,
					 &outImageInfo->extraInfo,
					 &outImageInfo->convertionAllowed
	    );
    if ( rc) {
	/* Clearing the old properties list. */
	img_destroy_properties( imageInfo->propList);
	/* And duplicating the new ImgInfo structure. */
	memcpy( imageInfo, outImageInfo, sizeof( ImgInfo));
	/* propList is now being contained in imageInfo... */
	outImageInfo->propList = nil;
    }

    img_info_destroy( outImageInfo);

    return rc;
}

static Bool
adjust_line_size( PList imgInfo)
{
    int i;

/*    DOLBUG( "Adjusting lineSize\n"); */

    for ( i = 0; i < imgInfo->count; i++) {
	PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
	PImgProperty lineSizeProp = nil, dataProp = nil;
	int h = 0, j;

	for ( j = 0; j < imageInfo->propList->count; j++) {
	    PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, j);

	    if ( strcmp( imgProp->name, "data") == 0) {
		dataProp = imgProp;
	    }
	    else if ( strcmp( imgProp->name, "lineSize") == 0) {
		lineSizeProp = imgProp;
	    }
	    else if ( strcmp( imgProp->name, "height") == 0) {
		h = imgProp->val.Int;
	    }
	}

	if ( ( dataProp != nil)
	     && ( lineSizeProp != nil)
	     && ( ( dataProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY)
	     && ( ( dataProp->flags & PROPTYPE_MASK) == PROPTYPE_BYTE)
	     && ( ( lineSizeProp->flags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY)
	     && ( ( lineSizeProp->flags & PROPTYPE_MASK) == PROPTYPE_INT)
	     && ( ( lineSizeProp->val.Int % 4) != 0)) {
	    int newSize = ( lineSizeProp->val.Int / 4 + 1) * 4;
	    Byte *newData = allocb( newSize * h);
	    Byte *po = newData, *pi = dataProp->val.pByte;

	    if ( ! newData) {
		return false;
	    }

	    for ( j = 0; j < h; j++) {
		memcpy( po, pi, lineSizeProp->val.Int);
		po += newSize;
		pi += lineSizeProp->val.Int;
	    }

	    free( dataProp->val.pByte);
	    dataProp->val.pByte = newData;
	    dataProp->used = dataProp->size = newSize * h;
	    lineSizeProp->val.Int = newSize;
	}
    }
    return true;
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
		    if ( format_idx != -1) {
			int i;
			Bool correction_succeed = true, readAll = false;
			PImgFormat imgFormat = ( PImgFormat) list_at( imgFormats, format_idx);
			for ( i = 0; ( i < imgInfo->count) && correction_succeed; i++) {
			    PImgInfo imageInfo = ( PImgInfo) list_at( imgInfo, i);
			    correction_succeed = __apc_image_correct_properties( imageInfo, imgFormat, &readAll);
			}
			if ( readAll && correction_succeed && ( imgInfo->count > 1)) {
			    __apc_image_set_error( "apc_image_read(%s): readAll cannot be used with multiple image profiles", filename);
			    correction_succeed = false;
			}
			if ( correction_succeed) {
			    rc = ( readData ?
				   imgFormat->load( load_data->fd, load_data->filename, imgInfo, readAll) :
				   imgFormat->getInfo( load_data->fd, load_data->filename, imgInfo, readAll));
			    if ( ! rc) {
				__apc_image_set_error( "apc_image_read(%s): error in %s driver: %s",
						       filename,
						       imgFormat->id,
						       imgFormat->getError( NULL, 0));
			    }
			    if ( readData) {
				if ( ! adjust_line_size( imgInfo)) {
				    __apc_image_set_error( "apc_image_read(%s): lineSize correction failed",
							   filename);
				}
			    }
			}
		    }
		    else {
			__apc_image_set_error( "apc_image_read(%s): no appropriate image format found",
					       filename);
		    }
		}
		else {
		    __apc_image_set_error( "apc_image_read: image preread failed for %s: %s",
					   filename,
					   strerror( errno));
		}
		close( load_data->fd);
	    }
	    else {
		__apc_image_set_error( "apc_image_read(%s): open failed: %s",
				       filename,
				       strerror( errno));
	    }
	    load_cleanup_data( &load_data);
	}
    }
    else {
	__apc_image_set_error( "apc_image_read(%s): no any image format driver registered yet", filename);
    }
    return rc;
}

typedef struct {
    const char *filename;
    PList imgInfo;
    PList outImgInfo;
    const char *desiredFormat;
    List formats; /* List of formats reported the file being storable. */
} __ImgSaveData, *__PImgSaveData;

static __PImgSaveData
save_prepare_data( void)
{
    __PImgSaveData save_data = alloc1( __ImgSaveData);
    if ( save_data != nil) {
	save_data->filename = nil;
	list_create( &save_data->formats, 5, 5);
    }
    return save_data;
}

static void
save_cleanup_data( __PImgSaveData *save_data)
{
    if ( *save_data != nil) {
	list_destroy( &( *save_data)->formats);
	free( *save_data);
	*save_data = nil;
    }
}

static Bool
save_img_storable( Handle item, void *params)
{
    PImgFormat imgFormat = ( PImgFormat) item;
    __PImgSaveData save_data = ( __PImgSaveData) params;
    if ( save_data->desiredFormat != nil) {
	if ( strcasecmp( save_data->desiredFormat, imgFormat->id) != 0) {
	    return false;
	}
	else {
	    list_add( &save_data->formats, item);
	    return true;
	}
    }
    else if ( imgFormat->is_storable( save_data->filename, save_data->imgInfo)) {
	list_add( &save_data->formats, item);
    }
    return false;
}

static Bool
save_img_compatible( Handle item, void *params)
{
    PImgFormat imgFormat = ( PImgFormat) item;
    __PImgSaveData save_data = ( __PImgSaveData) params;
    Bool rc;
    int i;

/*    DOLBUG( "save_img_compatible: trying format %s: ", imgFormat->id); */
    save_data->outImgInfo = plist_create( save_data->imgInfo->count, 1);
    for ( i = 0; i < save_data->imgInfo->count; i++) {
	PImgInfo imageInfo, outImageInfo;
	imageInfo = ( PImgInfo) list_at( save_data->imgInfo, i);
	outImageInfo = img_info_create( imageInfo->propList->count);
	__apc_image_correct_property( imgFormat->propertyList,
				      imageInfo->propList,
				      outImageInfo->propList,
				      nil,
				      nil,
				      nil
	    );
	list_add( save_data->outImgInfo, ( Handle)outImageInfo);
    }
    rc = imgFormat->is_compatible( save_data->outImgInfo);
    if ( ! rc) {
	for ( i = 0; i < save_data->outImgInfo->count; i++) {
	    PImgInfo imageInfo = ( PImgInfo) list_at( save_data->outImgInfo, i);
	    img_info_destroy( imageInfo);
	}
	plist_destroy( save_data->outImgInfo);
    }
/*    DOLBUG( "%s\n", rc ? "compatible" : "incompatible"); */
    return rc;
}

Bool
apc_image_save( const char *filename, const char *format, PList imgInfo)
{
    Bool rc = false;
    __apc_image_clear_error();
    if ( ( imgInfo != nil) && ( filename != nil)) {
	__PImgSaveData save_data = save_prepare_data();
	if ( save_data != nil) {
	    int format_idx = -1;
	    save_data->filename = filename;
	    save_data->desiredFormat = format;
	    save_data->imgInfo = imgInfo;
	    list_first_that( imgFormats, save_img_storable, ( void *) save_data);
	    if ( save_data->formats.count > 0) {
		format_idx = list_first_that( &save_data->formats, save_img_compatible, ( void *)save_data);
	    }
	    if ( format_idx != -1) {
		PImgFormat imgFormat = ( PImgFormat) list_at( imgFormats, format_idx);
		rc = imgFormat->save( filename, save_data->outImgInfo);
		if ( ! rc) {
		    __apc_image_set_error( "apc_image_save(%s): error in %s driver: %s",
					   filename,
					   imgFormat->id,
					   imgFormat->getError( NULL, 0));
		}
	    }
	    else {
		__apc_image_set_error( "apc_image_save(%s): no compatible image formats found",
				       filename
		    );
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
    Byte *newbuf;
    int rd;
    Bool rc = false;

    if ( preread_size <= load_data->preread_size) {
	return true;
    }
    if ( lseek( load_data->fd, 0, SEEK_SET) != -1) {
	if ( ( newbuf = allocb( preread_size)) != nil) {
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

void
prima_init_image_subsystem( void)
{
   apc_register_image_format( APCIMG_VERSION, __gif_init());
}

void
prima_cleanup_image_subsystem( void)
{
    plist_destroy( imgFormats);
}

#ifdef __cplusplus
}
#endif
