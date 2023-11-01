#ifndef _IMG_IMG_H_
#define _IMG_IMG_H_

#ifndef _APRICOT_H_
#include <apricot.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ImgIORequest {
	ssize_t (*read)      ( void * handle, size_t busize, void * buffer);
	ssize_t (*write)     ( void * handle, size_t busize, void * buffer);
	int    (*seek)       ( void * handle, long offset, int whence);
	long   (*tell)       ( void * handle);
	int    (*flush)      ( void * handle);
	int    (*error)      ( void * handle);
	void   * handle;
} ImgIORequest, *PImgIORequest;

#define req_read(req,size,buf)       ((req)->read(((req)->handle),(size),(buf)))
#define req_write(req,size,buf)      ((req)->write(((req)->handle),(size),(buf)))
#define req_seek(req,offset,whence)  ((req)->seek(((req)->handle),(offset),(whence)))
#define req_tell(req)                ((req)->tell((req)->handle))
#define req_flush(req)               ((req)->flush((req)->handle))
#define req_error(req)               ((req)->error((req)->handle))

/* common data, request for a whole file load */

#define IMG_EVENTS_HEADER_READY 1
#define IMG_EVENTS_DATA_READY   2

struct ImgCodec;
struct ImgCodecVMT;

typedef struct {
	/* instance data, filled by core */
	char          * fileName;
	Bool            is_utf8;
	Bool            req_is_stdio;
	ImgIORequest    sioreq;
} ImgFileIOCommon;

typedef struct _ImgLoadFileInstance {
	/* instance data, filled by core */
	ImgFileIOCommon io;
	PImgIORequest   req;
	int             eventMask;      /* IMG_EVENTS_XXX / if set, Image:: events are issued */

	/* instance data, filled by open_load */
	int             frameCount;     /* total frames in the file; can return -1 if unknown */
	HV            * fileProperties; /* specific file data */
	void          * instance;       /* user instance */
	Bool            wasTruncated;   /* if codec can recover from EOF */

	/* user-specified data - applied to whole file */
	Bool            loadExtras;
	Bool            noImageData;
	Bool            iconUnmask;
	Bool            noIncomplete;
	Bool            blending;
	Bool            wantFrames;

	/* user-specified data - applied to every frame */
	HV            * profile;         /* frame-specific profile, in */
	HV            * frameProperties; /* frame-specific properties, out */

	int             frame;          /* request frame index */
	Handle          object;         /* to be used by load */

	/* internal variables */
	Bool            stop;
	char          * errbuf;         /* $! value */
	HV            * cached_defaults;
	HV            * cached_commons;
	struct ImgCodec * codec;
	char *          baseClassName;
	int             codecID;

	/* scanline event progress */
	unsigned int    eventDelay;     /* in milliseconds */
	struct timeval  lastEventTime;
	int             lastEventScanline;
	int             lastCachedScanline;
} ImgLoadFileInstance, *PImgLoadFileInstance;

/* common data, request for a whole file save */

typedef struct _ImgSaveFileInstance {
	/* instance data, filled by core */
	ImgFileIOCommon io;
	PImgIORequest   req;

	/* instance data, filled by open_save */
	void          * instance;       /* result of open, user data for save session */
	HV            * extras;         /* profile applied to whole save session */

	/* user-specified data - applied to every frame */
	int             frame;
	int             n_frames;       /* how many frames to be saved */
	Handle          object;         /* to be used by save */

	/* internal variables */
	char          * errbuf;         /* $! value */
	struct ImgCodec * codec;
	int             codecID;
	HV            * cached_defaults;
	HV            * cached_commons;

	Bool            autoConvert;
} ImgSaveFileInstance, *PImgSaveFileInstance;

#define IMG_LOAD_FROM_FILE           0x0000001
#define IMG_LOAD_FROM_STREAM         0x0000002
#define IMG_LOAD_MULTIFRAME          0x0000004
#define IMG_SAVE_TO_FILE             0x0000010
#define IMG_SAVE_TO_STREAM           0x0000020
#define IMG_SAVE_MULTIFRAME          0x0000040

/* codec info */
typedef struct _ImgCodecInfo {
	char  * name;              /* DUFF codec */
	char  * vendor;            /* Duff & Co. */
	int     versionMaj;        /* 1 */
	int     versionMin;        /* 0 */
	char ** fileExtensions;    /* duf, duff */
	char  * fileType;          /* Dumb File Format  */
	char  * fileShortType;     /* DUFF */
	char ** featuresSupported; /* duff-version 1, duff-rgb, duff-cmyk */
	char  * primaModule;       /* Prima::ImgPlugins::duff.pm */
	char  * primaPackage;      /* Prima::ImgPlugins::duff */
	unsigned int IOFlags;      /* IMG_XXX */
	int   * saveTypes;         /* imMono, imBW ... 0 */
	char ** loadOutput;        /* hash keys reported by load  */
	char ** mime;              /* image/duf, x-image/duff */
} ImgCodecInfo, *PImgCodecInfo;

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

extern int   apc_img_frame_count( char * fileName, Bool is_utf8, PImgIORequest ioreq);
extern PList apc_img_load( Handle self, char * fileName, Bool is_utf8, PImgIORequest ioreq, HV * profile, char * error);

extern PImgLoadFileInstance
             apc_img_open_load( char * fileName, Bool is_utf8, PImgIORequest ioreq,  HV * profile, char * error);
extern Handle
             apc_img_load_next_frame( Handle target, PImgLoadFileInstance fi, HV * profile, char * error );
extern void  apc_img_close_load( PImgLoadFileInstance fi );

extern int   apc_img_save( Handle self, char * fileName, Bool is_utf8, PImgIORequest ioreq, HV * profile, char * error);
extern PImgSaveFileInstance
             apc_img_open_save( char * fileName, Bool is_utf8, int n_frames, PImgIORequest ioreq, HV * profile, char * error);
extern Bool  apc_img_save_next_frame( Handle source, PImgSaveFileInstance fi, HV * profile, char * error );
extern void  apc_img_close_save( PImgSaveFileInstance fi, Bool unlink_file );

extern void  apc_img_codecs( PList result);
extern HV *  apc_img_info2hash( PImgCodec c);

extern void  apc_img_profile_add( HV * to, HV * from, HV * keys);
extern int   apc_img_read_palette( PRGBColor palBuf, SV * palette, Bool triplets);

/* event macros */
#define SCANLINES_DIR_LEFT_TO_RIGHT 0
#define SCANLINES_DIR_RIGHT_TO_LEFT 1
#define SCANLINES_DIR_TOP_TO_BOTTOM 2
#define SCANLINES_DIR_BOTTOM_TO_TOP 3
extern void  apc_img_notify_header_ready( PImgLoadFileInstance fi);
extern void  apc_img_notify_scanlines_ready( PImgLoadFileInstance fi, int scanlines, int direction);

#define EVENT_HEADER_READY(fi) \
if ( fi-> eventMask & IMG_EVENTS_HEADER_READY) \
	apc_img_notify_header_ready((fi))

#define EVENT_SCANLINES_RESET(fi) {\
(fi)-> lastEventScanline = (fi)-> lastCachedScanline = 0; \
gettimeofday( &(fi)-> lastEventTime, NULL);\
}

#define EVENT_SCANLINES_READY(fi,scanlines,dir) \
if ( (fi)-> eventMask & IMG_EVENTS_DATA_READY) \
	apc_img_notify_scanlines_ready((fi),scanlines,dir)
#define EVENT_SCANLINES_FINISHED(fi,dir) \
if ( (fi)-> eventMask & IMG_EVENTS_DATA_READY) {\
	fi-> lastEventTime.tv_sec = fi-> lastEventTime.tv_usec = 0;\
	apc_img_notify_scanlines_ready((fi),0,dir); \
}
#define EVENT_TOPDOWN_SCANLINES_READY(fi,scanlines) EVENT_SCANLINES_READY(fi,scanlines,SCANLINES_DIR_TOP_TO_BOTTOM)
#define EVENT_TOPDOWN_SCANLINES_FINISHED(fi) EVENT_SCANLINES_FINISHED(fi,SCANLINES_DIR_TOP_TO_BOTTOM)

#ifdef __cplusplus
}
#endif


#endif
