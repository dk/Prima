/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */

#ifndef _IMG_IMG_H_
#define _IMG_IMG_H_

#ifndef _APRICOT_H_
#include <apricot.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// common data, request for a whole file load

typedef struct _ImgLoadFileInstance {
  // instance data, filled by core
  char          * fileName;
  FILE          * f; 

  // instance data, filled by open_load
  int             frameCount;     // total frames in the file; can return -1 if unknown
  HV            * fileProperties; // specific file data
  void          * instance;       // user instance

  // user-specified data - applied to whole file
  Bool            loadExtras; 
  Bool            loadAll;
  Bool            noImageData;
  HV            * extras;         // profile applied to all frames

  // user-specified data - applied to every frame
  HV            * profile;         // frame-specific profile, in
  HV            * frameProperties; // frame-specific properties, out
  
  int             frame;          // request frame index
  Bool            jointFrame;     // true, if last frame was a previous one
  Handle          object;         // to be used by load

  // internal variables
  int             frameMapSize;   
  int           * frameMap;
  Bool            stop;
  char          * errbuf;         // $! value
} ImgLoadFileInstance, *PImgLoadFileInstance;

// common data, request for a whole file save

typedef struct _ImgSaveFileInstance {
  // instance data, filled by core
  char          * fileName;
  FILE          * f; 
  Bool            append;         // true if append, false if rewrite

  // instance data, filled by open_save
  void          * instance;       // result of open, user data for save session
  HV            * extras;         // profile applied to whole save session

  // user-specified data - applied to every frame
  int             frame;
  Handle          object;         // to be used by save
  HV            * objectExtras;   // extras supplied to image object
  
  // internal variables
  int             frameMapSize;   
  Handle        * frameMap;
  char          * errbuf;         // $! value
} ImgSaveFileInstance, *PImgSaveFileInstance;

// codec info
typedef struct _ImgCodecInfo {
   char  * name;              // DUFF codec
   char  * vendor;            // Duff & Co. 
   int     versionMaj;        // 1
   int     versionMin;        // 0
   char ** fileExtensions;    // duf, duff
   char  * fileType;          // Dumb File Format 
   char  * fileShortType;     // DUFF
   char ** featuresSupported; // duff-version 1, duff-rgb, duff-cmyk
   char  * primaModule;       // Prima::ImgPlugins::duff.pm
   char  * primaPackage;      // Prima::ImgPlugins::duff
   Bool    canLoad;
   Bool    canLoadMultiple;  
   Bool    canSave;          
   Bool    canSaveMultiple;  
   int   * saveTypes;         // imMono, imBW ... 0
   char ** loadOutput;        // hash keys reported by load 
} ImgCodecInfo, *PImgCodecInfo;

struct ImgCodec;
struct ImgCodecVMT;

typedef struct ImgCodecVMT *PImgCodecVMT;
typedef struct ImgCodec    *PImgCodec;

struct ImgCodec {
   struct ImgCodecVMT * vmt;
   PImgCodecInfo info;
   void         *instance;
   void         *initParam;
};

struct ImgCodecVMT {
  int       size;
  void * (* init)            ( PImgCodecInfo * info, void * param);
  void   (* done)            ( PImgCodec instance);
  HV *   (* load_defaults)   ( PImgCodec instance);
  void   (* load_check_in)   ( PImgCodec instance, HV * system, HV * user);
  void * (* open_load)       ( PImgCodec instance, PImgLoadFileInstance fi);
  Bool   (* load)            ( PImgCodec instance, PImgLoadFileInstance fi);
  void   (* close_load)      ( PImgCodec instance, PImgLoadFileInstance fi);
  HV *   (* save_defaults)   ( PImgCodec instance);
  void   (* save_check_in)   ( PImgCodec instance, HV * system, HV * user);
  void * (* open_save)       ( PImgCodec instance, PImgSaveFileInstance fi);
  Bool   (* save)            ( PImgCodec instance, PImgSaveFileInstance fi);
  void   (* close_save)      ( PImgCodec instance, PImgSaveFileInstance fi);
};

extern List               imgCodecs;
extern struct ImgCodecVMT CNullImgCodecVMT;
extern char * imgPVEmptySet[];
extern int    imgIVEmptySet[];

extern void  apc_img_init(void);
extern void  apc_img_done(void);
extern Bool  apc_img_register( PImgCodecVMT codec, void * initParam);

extern int   apc_img_frame_count( char * fileName);
extern PList apc_img_load( Handle self, char * fileName, HV * profile, char * error);
extern int   apc_img_save( Handle self, char * fileName, HV * profile, char * error);

extern void  apc_img_codecs( PList result);
extern HV *  apc_img_info2hash( PImgCodec c);

extern void  apc_img_profile_add( HV * to, HV * from, HV * keys);
extern int   apc_img_read_palette( PRGBColor palBuf, SV * palette);

#ifdef __cplusplus
}
#endif


#endif
