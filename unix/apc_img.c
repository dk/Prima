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
#include "img_api.h"
#include "unix/gif_support.h"

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
__apc_image_clear_error()
{
    apcImageErrorMsg[ 0] = 0;
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
   if ( XX-> image_cache) {
      XDestroyImage( XX-> image_cache);
      XX-> image_cache = nil;
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
   if ( XX-> image_cache) {
      XDestroyImage( XX-> image_cache);
      XX-> image_cache = nil;
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

static Bool
__apc_image_correct_property( PImgProps fmtProps,
			      PList propList,
			      PList outPropList,
			      Bool *readAll,
			      Bool *extraInfo
    )
{
    PImgProperty imgProp, outImgProp;
    int i, j, n;
    Bool rc = true;

    for ( i = ( propList->count - 1); ( i >= 0) && rc; i--) {
	Bool isExtraInfo, isReadAll;
	imgProp = ( PImgProperty) list_at( propList, i);
	if ( readAll || extraInfo) {
	    isExtraInfo = ( strcmp( imgProp->name, "extraInfo") == 0);
	    isReadAll = ( strcmp( imgProp->name, "readAll") == 0);
	    if ( isExtraInfo || isReadAll) {
		if ( ( imgProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY) {
		    // We don't expect an array here.
		    __apc_image_set_error( "__apc_image_correct_property: standard property ``%s'' cannot be an array", imgProp->name);
		    rc = false;
		}
		else if ( ( imgProp->flags & PROPTYPE_MASK) != PROPTYPE_BIN) {
		    __apc_image_set_error( "__apc_image_correct_property: property ``%s'' must be of scalar type", imgProp->name);
		    rc = false;
		}
		else if ( ! __is_boolean_value( imgProp->val.Binary.data)) {
		    __apc_image_set_error( "__apc_image_correct_property: property ``%s'' does not contain a boolean value", imgProp->name);
		    rc = false;
		}
		else {
		    if ( isExtraInfo && extraInfo) {
			*extraInfo = __boolean_value( imgProp->val.Binary.data);
		    }
		    if ( isReadAll && readAll) {
			*readAll = __boolean_value( imgProp->val.Binary.data);
		    }
		}
		/* That was readAll or extraInfo properties... */
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
		switch ( fmtProps[ j].type[ 0]) {
		    case 'i':
			if ( fmtProps[ j].type[ 1] == '*') {
			    rc = ( outImgProp = img_push_property( outPropList,
								   imgProp->name,
								   PROPTYPE_ARRAY | PROPTYPE_INT,
								   imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, atoi( imgProp->val.pBinary[ n].data));
			    }
			}
			else {
			    rc = ( outImgProp = img_push_property( outPropList,
								   imgProp->name,
								   PROPTYPE_INT,
								   0,
								   atoi( imgProp->val.Binary.data))) != nil;
			}
			break;
		    case 'n':
			if ( fmtProps[ j].type[ 1] == '*') {
			    rc = ( outImgProp = img_push_property( outPropList,
								   imgProp->name,
								   PROPTYPE_ARRAY | PROPTYPE_DOUBLE,
								   imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, atof( imgProp->val.pBinary[ n].data));
			    }
			}
			else {
			    rc = ( outImgProp = img_push_property( outPropList,
								   imgProp->name,
								   PROPTYPE_DOUBLE,
								   0,
								   atof( imgProp->val.Binary.data))) != nil;
			}
			break;
		    case 's':
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
			break;
		    case 'b':
			if ( fmtProps[ j].type[ 1] == '*') {
			    rc = ( outImgProp = img_push_property( outPropList,
								   imgProp->name,
								   PROPTYPE_ARRAY | PROPTYPE_BYTE,
								   imgProp->used)) != nil;
			    for ( n = 0; n < imgProp->used && rc; n++) {
				rc = img_push_property_value( outImgProp, atoi( imgProp->val.pBinary[ n].data));
			    }
			}
			else {
			    rc = ( outImgProp = img_push_property( outPropList,
								   imgProp->name,
								   PROPTYPE_BYTE,
								   0,
								   atoi( imgProp->val.Binary.data))) != nil;
			}
			break;
		    case 'B':
			/* The size of binary data must be decreased by 1 because it was
			   choosen in a way to preserve trailing zero in the Perl string. */
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
			break;
		    case 'p':
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
								   nil);
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
								 nil
				    );
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
		    outImgProp->id = fmtProps[ j].id;
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
    Bool rc = true;

    rc = ( ( outImageInfo = img_info_create( propList->count)) != nil)
	&& __apc_image_correct_property( imgFormat->propertyList, 
					 propList, 
					 outImageInfo->propList, 
					 readAll, 
					 &outImageInfo->extraInfo
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
						       imgFormat->getError( NULL, 0));
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

void
prima_init_image_subsystem( void)
{
   apc_register_image_format( APCIMG_VERSION, &gifFormat);
}

void
prima_cleanup_image_subsystem( void)
{
    plist_destroy( imgFormats);
}
