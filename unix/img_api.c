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
 */

/*
 * Created by Vadim Belman <voland@plab.ku.dk>, April 1999.
 */
#include "unix/img_api.h"

/* This function must always be used for creating a new image info structure. */
PImgInfo
img_info_create( int propCount)
{
    PImgInfo imgInfo;

    imgInfo = malloc( sizeof( ImgInfo));
    if ( imgInfo) {
	imgInfo->propList = plist_create( propCount > 0 ? propCount : 1, 5);
	if ( imgInfo->propList == nil) {
	    free( imgInfo);
	    return nil;
	}
	/* To avoid possible floating errors all structure fields must be initialized. */
	imgInfo->extraInfo = false;
    }
    /* Function returns either nil or correctly initialized structure. */
    /* All other options are forbidden! */
    return imgInfo;
}

void
img_info_destroy( PImgInfo imgInfo)
{
    /* Duplicate freeing of image info structure is allowed. */
    if ( imgInfo != nil) {
	if ( imgInfo->propList != nil) {
	    img_info_destroy_properties( imgInfo);
	}
	free( imgInfo);
    }
    else {
	/* And this situation is rather a error. */
	DOLBUG( "*Error* nil passed to img_info_destroy\n");
    }
}

void
img_info_destroy_properties( PImgInfo imgInfo)
{
    if ( imgInfo) {
	img_destroy_properties( imgInfo->propList);
	imgInfo->propList = nil;
    }
    else {
	DOLBUG( "*Error* nil passed to img_info_destroy_properties\n");
    }
}

/* Empties a property list without freeing the list itself. */
void
img_destroy_properties( PList propList)
{
    if ( propList) {
	int i;

	for ( i = 0; i < propList->count; i++) {
	    img_free_property( ( PImgProperty) list_at( propList, i));
	}
	list_destroy( propList);
    }
    else {
	DOLBUG( "*Error* nil passed to img_destroy_properties\n");
    }
}

void
img_free_property( PImgProperty imgProp)
{
    if ( imgProp) {
	img_clear_property( imgProp);
	free( imgProp);
    }
    else {
	DOLBUG( "*Error* nil passed to img_free_property\n");
    }
}

void
img_free_bindata( PIMGBinaryData binData)
{
    if ( binData) {
	free( binData->data);
	free( binData);
    }
    else {
	DOLBUG( "*Error* nil passed to img_free_bindata\n");
    }
}

void
img_clear_property( PImgProperty imgProp)
{
    if ( imgProp != nil) {
	int i;
	free( imgProp->name);
	if ( ( imgProp->flags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY) {
	    switch ( ( imgProp->flags & PROPTYPE_MASK)) {
		case PROPTYPE_STRING:
		    for ( i = 0; i < imgProp->used; i++) {
			free( imgProp->val.pString[ i]);
		    }
		    free( imgProp->val.pString);
		    break;   
		case PROPTYPE_PROP:
		    for ( i = 0; i < imgProp->used; i++) {
			img_destroy_properties( &imgProp->val.pProperties[ i]);
			list_destroy( &imgProp->val.pProperties[ i]);
		    }
		    free( imgProp->val.pProperties);
		    break;
		case PROPTYPE_BIN:
		    for ( i = 0; i < imgProp->used; i++) {
			free( imgProp->val.pBinary[ i].data);
		    }
		    free( imgProp->val.pBinary);
		    break;
		case PROPTYPE_DOUBLE:
		    free( imgProp->val.pDouble);
		    break;
		case PROPTYPE_INT:
		    free( imgProp->val.pInt);
		    break;
		case PROPTYPE_BYTE:
		    free( imgProp->val.pByte);
		    break;
		default:
		    croak( "Corrupted ImgProperty structure: unknown data type 0x%02x\n", imgProp->flags & PROPTYPE_MASK);
		    break;
	    }
	}
	else {
	    switch ( ( imgProp->flags & PROPTYPE_MASK)) {
		case PROPTYPE_STRING:
		    free( imgProp->val.String);
		    break;
		case PROPTYPE_BIN:
		    free( imgProp->val.Binary.data);
		    break;
		case PROPTYPE_PROP:
		    for ( i = 0; i < imgProp->val.Properties.count; i++) {
			img_free_property( ( PImgProperty) list_at( &imgProp->val.Properties, i));
		    }
		    list_destroy( &imgProp->val.Properties);
		    break;
	    }
	}
	bzero( imgProp, sizeof( ImgProperty));
    }
}

void
img_info_delete_property_at( PImgInfo imgInfo, int index)
{
    img_delete_property_at( imgInfo->propList, index);
}

void
img_delete_property_at( PList propList, int index)
{
    PImgProperty imgProp;

    imgProp = ( PImgProperty) list_at( propList, index);
    if ( imgProp != nil) {
	img_free_property( imgProp);
	list_delete_at( propList, index);
    }
}

PImgProperty
img_property_create( const char *name, U16 propFlags, int propArraySize)
{
    PImgProperty imgProp;

    imgProp = malloc( sizeof( ImgProperty));
    if ( imgProp) {
	imgProp->name = duplicate_string( name);
	if ( imgProp->name == nil) {
	    free( imgProp);
	    return nil;
	}
	imgProp->flags = propFlags;
	imgProp->size = -1;
	imgProp->used = 0;
	if ( (propFlags & PROPTYPE_ARRAY) == PROPTYPE_ARRAY) {
	    int arraySize;
	    imgProp->size = propArraySize > 0 ? propArraySize : 1;
	    switch ( propFlags & PROPTYPE_MASK) {
		case PROPTYPE_INT:
		    imgProp->val.pInt = malloc( arraySize = sizeof( int) * propArraySize);
		    bzero( imgProp->val.pInt, arraySize);
		    break;
		case PROPTYPE_DOUBLE:
		    imgProp->val.pDouble = malloc( arraySize = sizeof( double) * propArraySize);
		    bzero( imgProp->val.pDouble, arraySize);
		    break;
		case PROPTYPE_BYTE:
		    imgProp->val.pByte = malloc( arraySize = sizeof( Byte) * propArraySize);
		    bzero( imgProp->val.pByte, arraySize);
		    break;
		case PROPTYPE_STRING:
		    imgProp->val.pString = malloc( arraySize = sizeof( char *) * propArraySize);
		    bzero( imgProp->val.pString, arraySize);
		    break;
		case PROPTYPE_BIN:
		    imgProp->val.pBinary = malloc( arraySize = sizeof( IMGBinaryData) * propArraySize);
		    bzero( imgProp->val.pBinary, arraySize);
		    break;
		case PROPTYPE_PROP:
		    imgProp->val.pProperties = malloc( arraySize = sizeof( List) * propArraySize);
		    bzero( imgProp->val.pProperties, arraySize);
		    break;
		default:
		    croak( "Unsupported or unknown property type: 0x%02x\n", propFlags & PROPTYPE_MASK);
		    break;
	    }
	}
    }

    return imgProp;
}

/* The functions pushes a new property into existing property list. */
PImgProperty
img_push_property_v( PList propList,
		     const char *propName, 
		     U16 propFlags, /* Combination of PROPTYPE_* constants. */
		     int propArraySize, /* Ignored for non-array property. */
		     va_list arg /* Optional parameter might be used for immediate 
				    initialization of non-array property.
				    For PROPTYPE_PROP this just an initial size of
				    properties list. This function might be used to
				    fill the list.
				    Binary data must be passed in two parameters:
				    the first is size (int) and the second is pointer
				    to the data.
				    *NOTE*: the data will be copied as well as it being 
				    done for a String value. */
    )
{
    PImgProperty imgProp;

    imgProp = img_property_create( propName, propFlags, propArraySize);
    if ( imgProp) {
	if ( (propFlags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY) {
	    switch ( propFlags & PROPTYPE_MASK) {
		case PROPTYPE_INT:
		    {
			int intVal = va_arg( arg, int);
			imgProp->val.Int = intVal;
		    }
		    break;
		case PROPTYPE_DOUBLE:
		    {
			double doubleVal = va_arg( arg, double);
			imgProp->val.Double = doubleVal;
		    }
		    break;
		case PROPTYPE_BYTE:
		    {
			Byte byteVal = va_arg( arg, Byte);
			imgProp->val.Byte = byteVal;
		    }
		    break;
		case PROPTYPE_STRING:
		    {
			char *stringVal = va_arg( arg, char *);
			imgProp->val.String = duplicate_string( stringVal);
		    }
		    break;
		case PROPTYPE_BIN:
		    {
			int binSize = va_arg( arg, int);
			Byte *binData = va_arg( arg, Byte *);
			imgProp->val.Binary.size = binSize;
			imgProp->val.Binary.data = malloc( binSize);
			memcpy( imgProp->val.Binary.data, binData, binSize);
		    }
		    break;
		case PROPTYPE_PROP:
		    {
			int listSize = va_arg( arg, int);
			list_create( &imgProp->val.Properties, listSize, 5);
		    }
		    break;
		default:
		    croak( "Unknown property type: 0x%02x\n", propFlags & PROPTYPE_MASK);
		    break;
	    }
	}
	list_add( propList, ( Handle) imgProp);
    }

    return imgProp;
}

/* Adds a property into a existing property list. */
PImgProperty
img_push_property( PList propList,
		   const char *propName, 
		   U16 propFlags,
		   int propArraySize,
		   ...
    )
{
    PImgProperty imgProp;
    va_list args;
    va_start( args, propArraySize);
    imgProp = img_push_property_v( propList, propName, propFlags, propArraySize, args);
    va_end( args);
    return imgProp;
}

/* Adds a new property to image info property list. */
PImgProperty
img_info_add_property( PImgInfo imgInfo, 
		       const char *propName, 
		       U16 propFlags, 
		       int propArraySize,
		       ...
    )
{
    PImgProperty imgProp = nil;
    if ( imgInfo->propList == nil) {
	imgInfo->propList = plist_create( 10, 5);
    }
    if ( imgInfo->propList != nil) {
	va_list args;
	va_start( args, propArraySize);
	imgProp = img_push_property_v( imgInfo->propList, propName, propFlags, propArraySize, args);
	va_end( args);
    }
    return imgProp;
}

/* Parameter arg has the same meaning as in img_push_property_v() */
Bool
img_push_property_value_v( PImgProperty imgProp, va_list arg)
{
    Bool rc = true;
    if ( ( imgProp->flags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY) {
	croak( "Attempt to push not into array property.\n");
    }
    switch ( imgProp->flags & PROPTYPE_MASK) {
	case PROPTYPE_INT:
	    {
		int intVal = va_arg( arg, int);
		if ( imgProp->used == imgProp->size) {
		    imgProp->size += 1;
		    rc = ( imgProp->val.pInt = reallocf( imgProp->val.pInt, sizeof( int) * imgProp->size)) != nil;
		}
		if ( rc) {
		    imgProp->val.pInt[ imgProp->used++] = intVal;
		}
	    }
	    break;
	case PROPTYPE_DOUBLE:
	    {
		double doubleVal = va_arg( arg, double);
		if ( imgProp->used == imgProp->size) {
		    imgProp->size += 1;
		    rc = ( imgProp->val.pDouble = reallocf( imgProp->val.pDouble, sizeof( double) * imgProp->size)) != nil;
		}
		if ( rc) {
		    imgProp->val.pDouble[ imgProp->used++] = doubleVal;
		}
	    }
	    break;
	case PROPTYPE_BYTE:
	    {
		Byte byteVal = va_arg( arg, Byte);
		if ( imgProp->used == imgProp->size) {
		    imgProp->size += 1;
		    rc = ( imgProp->val.pByte = reallocf( imgProp->val.pByte, sizeof( Byte) * imgProp->size)) != nil;
		}
		if ( rc) {
		    imgProp->val.pByte[ imgProp->used++] = byteVal;
		}
	    }
	    break;
	case PROPTYPE_STRING:
	    {
		char *stringVal = va_arg( arg, char *);
		if ( imgProp->used == imgProp->size) {
		    imgProp->size += 1;
		    rc = ( imgProp->val.pString = reallocf( imgProp->val.pString, sizeof( char *) * imgProp->size)) != nil;
		}
		if ( rc) {
		    imgProp->val.pString[ imgProp->used++] = duplicate_string( stringVal);
		}
	    }
	    break;
	case PROPTYPE_BIN:
	    {
		int binSize = va_arg( arg, int);
		Byte *binData = va_arg( arg, Byte *);
		if ( imgProp->used == imgProp->size) {
		    imgProp->size += 1;
		    rc = ( imgProp->val.pBinary = reallocf( imgProp->val.pBinary, sizeof( IMGBinaryData) * imgProp->size)) != nil;
		}
		if ( rc) {
		    rc = ( imgProp->val.pBinary[ imgProp->used].data = malloc( binSize)) != nil;
		}
		if ( rc) {
		    imgProp->val.pBinary[ imgProp->used].size = binSize;
		    memcpy( imgProp->val.pBinary[ imgProp->used++].data, binData, binSize);
		}
	    }
	    break;
	case PROPTYPE_PROP:
	    {
		int listSize = va_arg( arg, int);
		if ( imgProp->used == imgProp->size) {
		    imgProp->size++;
		    rc = ( imgProp->val.pProperties = reallocf( imgProp->val.pProperties, sizeof( List) * imgProp->size)) != nil;
		}
		if ( rc) {
		    list_create( imgProp->val.pProperties + imgProp->used++, listSize, 5);
		}
	    }
	    break;
	default:
	    croak( "Unknown property type: 0x%02x\n", imgProp->flags & PROPTYPE_MASK);
	    break;
    }
    return rc;
}

Bool
img_push_property_value( PImgProperty imgProp, ...)
{
    Bool rc;
    va_list arg;
    va_start( arg, imgProp);
    rc = img_push_property_value_v( imgProp, arg);
    va_end( arg);
    return rc;
}
