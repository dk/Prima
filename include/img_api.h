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
#ifndef __IMG_API_H__
#define __IMG_API_H__

#include "apricot.h"

typedef struct _IMGCapInfo {
   char *id;
   /*
    * Type characters:
    * i - int
    * n - double
    * s - string
    *
    * A type character suffixed with asterisks means array.
    */
   char *type;
   char *descr;
   int size;
   union {
      int Int;
      double Double;
      char *String;

      int *pInt;
      double *pDouble;
      char **pString;
   } val;
} ImgCapInfo, *PImgCapInfo;

typedef struct _IMGProperties {
   char *name;
   int id;
   /*
    * Property types are the same as for capabilities but with addition of
    * b - byte.
    * B - binary data.
    * p - property.
    */
   char *type;
   char *descr;
   struct _IMGProperties *subProps; /* This field contains a list of subproperties for property type. */
} ImgProps, *PImgProps;

typedef struct _IMGBinaryData {
    int size;
    Byte *data;
} IMGBinaryData, *PIMGBinaryData;

typedef struct _IMGProperty { /* To be passed for Load/Save related operations. */
   char *name;
   int id;
   int size; /* Size of array if property contains an array. */
   int used; /* Number of used elements in the array */
   U16 flags;
#define PROPTYPE_MASK    0x00ff
#define PROPTYPE_INT     0x0001
#define PROPTYPE_DOUBLE  0x0002
#define PROPTYPE_STRING  0x0003
#define PROPTYPE_BYTE    0x0004
#define PROPTYPE_BIN     0x0005
#define PROPTYPE_PROP    0x0006 /* Value is another property. */
#define PROPTYPE_ARRAY   0x0100 /* Property contains an array of elements of specified type. */
   union {
      int Int;
      double Double;
      char *String;
      Byte Byte;
      IMGBinaryData Binary;
      List Properties; /* List of PImgProperty. */

      int *pInt;
      double *pDouble;
      char **pString;
      Byte *pByte;
      PIMGBinaryData pBinary;
      PList pProperties;
   } val;
} ImgProperty, *PImgProperty;

typedef struct _IMGInfo {
   Bool extraInfo;
   PList propList;
} ImgInfo, *PImgInfo;

typedef Bool IMGF_Load( int fd, const char *filename, PList imgInfo, Bool readAll);
typedef IMGF_Load *PIMGF_Load;
typedef Bool IMGF_Save( const char *filename, PList imgInfo);
typedef IMGF_Save *PIMGF_Save;
typedef Bool IMGF_Loadable( int fd, const char *filename, Byte *preread_buf, U32 buf_size);
typedef IMGF_Loadable *PIMGF_Loadable;
typedef Bool IMGF_Storable( const char *filename, PList imgInfo);
typedef IMGF_Storable *PIMGF_Storable;
typedef Bool IMGF_GetInfo( int fd, const char *filename, PList imgInfo, Bool readAll);
typedef IMGF_GetInfo *PIMGF_GetInfo;
typedef const char *IMGF_GetErrorMsg( char *errorMsgBuf, int bufLen);
typedef IMGF_GetErrorMsg *PIMGF_GetErrorMsg;

typedef struct _IMGFormat {
   char *id;
   char *descr;

   PImgCapInfo capabilities;
   PImgProps propertyList;
   int preread_size;

   PIMGF_Load load;
   PIMGF_Save save;
   PIMGF_Loadable is_loadable;
   PIMGF_Storable is_storable;
   PIMGF_GetInfo getInfo;
   PIMGF_GetErrorMsg getError;
} ImgFormat, *PImgFormat;

extern PList imgFormats;

extern PImgInfo
img_info_create( int propCount);

extern void
img_info_destroy( PImgInfo imgInfo);

extern void
img_info_destroy_properties( PImgInfo imgInfo);

extern void
img_clear_properties( PList propList);

extern void
img_destroy_properties( PList propList);

extern void
img_free_property( PImgProperty imgProp);

extern void
img_free_bindata( PIMGBinaryData binData);

extern void
img_clear_property( PImgProperty imgProp);

extern void
img_info_delete_property_at( PImgInfo imgInfo, int index);

extern void
img_delete_property_at( PList propList, int index);

extern PImgProperty
img_property_create( const char *name, U16 propFlags, int propArraySize);

/* The functions pushes a new property into existing property list. */
extern PImgProperty
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
    );

/* Adds a property into a existing property list. */
extern PImgProperty
img_push_property( PList propList,
		   const char *propName, 
		   U16 propFlags,
		   int propArraySize,
		   ...
    );

/* Adds a new property to image info property list. */
extern PImgProperty
img_info_add_property( PImgInfo imgInfo, 
		       const char *propName, 
		       U16 propFlags, 
		       int propArraySize,
		       ...
    );

/* Parameter arg has the same meaning as in img_push_property_v() */
extern Bool
img_push_property_value_v( PImgProperty imgProp, va_list arg);

extern Bool
img_push_property_value( PImgProperty imgProp, ...);

#endif
