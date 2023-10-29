#include "img.h"
#include "img_conv.h"
#include "Icon.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static Bool initialized = false;

void
apc_img_init( void)
{
	if ( initialized) croak("Attempt to initialize image subsystem twice");
	list_create( &imgCodecs, 8, 8);
	initialized = true;
}

#define CHK if ( !initialized) croak("Image subsystem is not initialized");

void
apc_img_done( void)
{
	int i;

	CHK;
	for ( i = 0; i < imgCodecs. count; i++) {
		PImgCodec c = ( PImgCodec)( imgCodecs. items[ i]);
		if ( c-> instance)
			c-> vmt-> done( c);
		free( c);
	}
	list_destroy( &imgCodecs);
	initialized = false;
}

Bool
apc_img_register( PImgCodecVMT codec, void * initParam)
{
	PImgCodec c;

	CHK;
	if ( !codec) return false;
	c = ( PImgCodec) malloc( sizeof( struct ImgCodec) + codec-> size);
	if ( !c) return false;

	memset( c, 0, sizeof( struct ImgCodec));
	c-> vmt = ( PImgCodecVMT) ((( Byte *) c) + sizeof( struct ImgCodec));
	c-> initParam = initParam;
	memcpy( c-> vmt, codec, codec-> size);
	list_add( &imgCodecs, ( Handle) c);
	return true;
}

static void *
init( PImgCodecInfo * info, void * param)
{
	return NULL;
}

static void
done( PImgCodec instance)
{
}

static HV *
defaults(  PImgCodec instance)
{
	return newHV();
}

static void
check_in(  PImgCodec instance, HV * system, HV * user)
{
}

static void *
open_load(  PImgCodec instance, PImgLoadFileInstance fi)
{
	return NULL;
}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	return false;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	free( fi-> instance);
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return NULL;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return false;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	free( fi-> instance);
}


List imgCodecs;
struct ImgCodecVMT CNullImgCodecVMT = {
	sizeof( struct ImgCodecVMT),
	init,
	done,
	defaults,
	check_in,
	open_load,
	load,
	close_load,
	defaults,
	check_in,
	open_save,
	save,
	close_save
};

void
apc_img_profile_add( HV * to, HV * from, HV * keys)
{
	HE *he;
	hv_iterinit(( HV*) keys);
	for (;;)
	{
		char *key;
		int  keyLen;
		SV ** holder;
		if (( he = hv_iternext( keys)) == NULL)
			return;
		key    = (char*) HeKEY( he);
		keyLen = HeKLEN( he);
		if ( !hv_exists( from, key, keyLen))
			continue;
		holder = hv_fetch( from, key, keyLen, 0);
		if ( holder)
			(void) hv_store( to, key, keyLen, newSVsv( *holder), 0);
	}
}

static ssize_t
stdio_read( void * f, size_t bufsize, void * buffer)
{
	return fread( buffer, 1, bufsize, ( FILE*) f);
}

static ssize_t
stdio_write( void * f, size_t bufsize, void * buffer)
{
	return fwrite( buffer, 1, bufsize, ( FILE*) f);
}

static int
stdio_seek( void * f, long offset, int whence)
{
	return fseek( ( FILE*) f, offset, whence);
}

static long
stdio_tell( void * f)
{
	return ftell( ( FILE*) f);
}


static ImgIORequest std_ioreq = {
	stdio_read,
	stdio_write,
	stdio_seek,
	stdio_tell,
	(void*) fflush,
	(void*) ferror
};

PImgLoadFileInstance
apc_img_open_load( Handle self, char * fileName, Bool is_utf8, PImgIORequest ioreq,  HV * profile, char * error)
{
	dPROFILE;
	int i;
	Bool err = false;
	int  load_mask;
	char dummy_error_buf[256];
	PImgCodec c;
	PImgLoadFileInstance fi;

#define out(x){ err = true;\
	strlcpy( fi->errbuf, x, 256);\
	goto EXIT_NOW;}

#define outd(x,d){ err = true;\
	snprintf( fi->errbuf, 256, x, d);\
	goto EXIT_NOW;}

	CHK;
	if ( !( fi = malloc(sizeof(ImgLoadFileInstance)))) {
		if ( error) strcpy(error, "Not enough memory");
		return NULL;
	}

	memset( fi, 0, sizeof( ImgLoadFileInstance));
	fi-> baseClassName = "Prima::Image";

	fi-> errbuf = error ? error : dummy_error_buf;
	fi-> errbuf[0] = 0;

	/* open file */
	if ( ioreq == NULL) {
		memcpy( &fi->sioreq, &std_ioreq, sizeof( fi->sioreq));
		if (( fi->sioreq.handle = prima_open_file( fileName, is_utf8, "rb")) == NULL)
			out( strerror( errno));
		fi->req = &fi->sioreq;
		fi->req_is_stdio = true;
		load_mask = IMG_LOAD_FROM_FILE;
	} else {
		fi->req = ioreq;
		fi->req_is_stdio = false;
		load_mask = IMG_LOAD_FROM_STREAM;
	}
	fi-> fileName = fileName;
	fi-> is_utf8  = is_utf8;
	fi-> stop = false;
	fi-> last_frame = -2;
	fi-> codecID = -1;

	/* assigning user file profile */
	if ( pexist( index)) {
		fi-> frameMapSize = 1;
		if ( !( fi-> frameMap  = (int*) malloc( sizeof( int))))
			out("Not enough memory");
		if ((*(fi-> frameMap) = pget_i( index)) < 0)
			out("Invalid index");
	} else if ( pexist( map)) {
		SV * sv = pget_sv( map);
		if ( SvOK( sv)) {
			if ( SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
				AV * av = ( AV*) SvRV( sv);
				int len = av_len( av) + 1;
				if ( !( fi-> frameMap = ( int *) malloc( sizeof( int) * len)))
					out("Not enough memory");
				for ( i = 0; i < len; i++) {
					SV ** holder = av_fetch( av, i, 0);
					if ( !holder) out("Array panic on 'map' property");
					if (( fi-> frameMap[ i] = SvIV( *holder)) < 0)
						out("Invalid index on 'map' property");
				}
				fi-> frameMapSize = len;
			} else
				out("Not an array passed to 'map' property");
		}
	} else if ( pexist( loadAll) && pget_B( loadAll)) {
		fi-> loadAll = true;
	} else {
		fi-> frameMapSize = 1;
		if ( ! (fi-> frameMap = ( int*) malloc( sizeof( int))))
			out("Not enough memory");
		*(fi-> frameMap) = 0;
	}

	if ( pexist( loadExtras) && pget_B( loadExtras))
		fi-> loadExtras = true;

	if ( pexist( noImageData) && pget_B( noImageData))
		fi-> noImageData = true;

	if ( pexist( iconUnmask) && pget_B( iconUnmask))
		fi-> iconUnmask = true;

	if ( pexist( blending) && !pget_B( blending))
		fi-> blending = false;

	if ( pexist( noIncomplete) && pget_B( noIncomplete))
		fi-> noIncomplete = true;

	if ( pexist( wantFrames) && pget_B( wantFrames))
		fi-> wantFrames = true;

	if ( pexist( eventMask))
		fi-> eventMask = pget_i( eventMask);

	if ( pexist( eventDelay))
		fi-> eventDelay = 1000.0 * pget_f( eventDelay);
	if ( fi-> eventDelay <= 0)
		fi-> eventDelay = 100; /* 100 ms. reasonable? */
	EVENT_SCANLINES_RESET(fi);

	if ( pexist( profiles)) {
		SV * sv = pget_sv( profiles);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			fi-> profiles = ( AV *) SvRV( sv);
			fi-> profiles_len = av_len( fi->profiles);
		} else
			out("Not an array passed to 'profiles' property");
	}

	if ( pexist( className)) {
		PVMT vmt;
		fi->baseClassName = pget_c( className);
		vmt = gimme_the_vmt( fi->baseClassName);
		while ( vmt && vmt != (PVMT)CImage)
			vmt = vmt-> base;
		if ( !vmt)
			outd("class '%s' is not a Prima::Image descendant", fi->baseClassName);
	}

	/* all other properties to be parsed by codec */
	fi-> extras = profile;

	fi-> fileProperties = newHV();
	fi-> frameCount = -1;

	/* finding codec */
	{
		Bool * loadmap = ( Bool *) malloc( sizeof( Bool) * imgCodecs. count);

		if ( !loadmap)
			out("Not enough memory");
		memset( loadmap, 0, sizeof( Bool) * imgCodecs. count);
		for ( i = 0; i < imgCodecs. count; i++) {
			c = ( PImgCodec ) ( imgCodecs. items[ i]);
			if ( !c-> instance)
				c-> instance = c-> vmt-> init( &c->info, c-> initParam);
			if ( !c-> instance) { /* failed to initialize, retry next time */
				loadmap[ i] = true;
				continue;
			}
		}
		c = NULL;

		/* finding by extension first */
		if ( fileName) {
			int fileNameLen = strlen( fileName);
			for ( i = 0; i < imgCodecs. count; i++) {
				int j = 0, found = false;
				if ( loadmap[ i]) continue;
				c = ( PImgCodec ) ( imgCodecs. items[ i]);
				while ( c-> info-> fileExtensions[ j]) {
					char * ext = c-> info-> fileExtensions[ j];
					int extLen = strlen( ext);
					if ( extLen < fileNameLen && stricmp( fileName + fileNameLen - extLen, ext) == 0) {
						found = true;
						break;
					}
					j++;
				}
				if ( found) {
					loadmap[ i] = true;

					if ( !( c-> info-> IOFlags & load_mask)) {
						c = NULL;
						continue;
					}
					if (( fi-> instance = c-> vmt-> open_load( c, fi)) != NULL) {
						fi-> codecID = i;
						break;
					}

					if ( fi-> stop) {
						err = true;
						free( loadmap);
						goto EXIT_NOW;
					}
				}
				c = NULL;
			}
		}

		/* use first suitable codec */
		if ( c == NULL) {
			for ( i = 0; i < imgCodecs. count; i++) {
				if ( loadmap[ i]) continue;
				c = ( PImgCodec ) ( imgCodecs. items[ i]);
				if ( !( c-> info-> IOFlags & load_mask)) {
						c = NULL;
						continue;
				}
				if (( fi-> instance = c-> vmt-> open_load( c, fi)) != NULL) {
					fi-> codecID = i;
					break;
				}
				if ( fi-> stop) {
					err = true;
					free( loadmap);
					goto EXIT_NOW;
				}
				c = NULL;
			}
		}
		free( loadmap);
		if ( !c) out("No appropriate codec found");
	}

	if ( fi-> loadAll) {
		if ( fi-> frameCount >= 0) {
			fi-> frameMapSize = fi-> frameCount;
			if ( !( fi-> frameMap  = (int*) malloc( fi-> frameCount * sizeof(int))))
				out("Not enough memory");
			for ( i = 0; i < fi-> frameCount; i++)
				fi-> frameMap[i] = i;
		} else {
			fi-> frameMapSize = INT_MAX;
			fi-> incrementalLoad = true;
		}
	}


	/* use common profile */
	fi-> cached_defaults = c-> vmt-> load_defaults( c);
	fi-> cached_commons = newHV();
	if ( profile) {
		c-> vmt-> load_check_in( c, fi-> cached_commons, profile);
		apc_img_profile_add( fi-> cached_commons, profile, fi-> cached_defaults);
	}

	if ( fi-> loadExtras && c-> info-> fileType)
		(void) hv_store( fi-> fileProperties, "codecID", 7, newSViv( fi-> codecID), 0);

	if ( fi-> frameCount < 0 && fi-> wantFrames) {
		if ( ioreq != NULL)
			req_seek( ioreq, 0, SEEK_SET);
		fi-> frameCount = apc_img_frame_count( fileName, is_utf8, ioreq);
		if ( fi-> loadExtras ) 
			(void) hv_store( fi-> fileProperties, "frames", 6, newSViv( fi-> frameCount), 0);
	}
	fi-> codec = c;

	/* returning info for null load request  */
	if ( self && fi->loadExtras && fi->frameMapSize == 0) {
		HV * extras = newHV();
		SV * sv = newRV_noinc(( SV *) extras);
		apc_img_profile_add( extras, fi->fileProperties,  fi->fileProperties);
		(void) hv_store(( HV* )SvRV((( PAnyObject) self)-> mate), "extras", 6, newSVsv( sv), 0);
		sv_free( sv);
	}

EXIT_NOW:;
	if ( err ) {
		if ( fi->errbuf[0] == 0)
			strcpy(fi->errbuf, "Internal error");
		apc_img_close_load( fi );
		return NULL;
	}

	return fi;

	#undef out
	#undef outd
}

Handle
apc_img_load_next_frame( Handle target, PImgLoadFileInstance fi, char * error )
{
#define out(x){ err = true;\
	strlcpy( fi->errbuf, x, 256);\
	goto EXIT_NOW;}
#define outd(x,d){ err = true;\
	snprintf( fi->errbuf, 256, x, d);\
	goto EXIT_NOW;}

	dPROFILE;
	Bool err = false;
	char dummy_error_buf[256];
	PImgCodec c = fi->codec;
	char * className = fi->baseClassName;
	HV *save_profile = NULL;
	HV *firstObjectExtras = NULL;
	Bool save[5];

	fi->errbuf = error ? error : dummy_error_buf;
	fi->errbuf[0] = 0;
	fi->object = NULL_HANDLE;

	if ( fi-> current_frame >= fi-> frameMapSize ) {
		fi-> eof_is_not_an_error = true;
		goto EXIT_NOW;
	}

	/* loading */
	fi->frame = fi->incrementalLoad ? fi->current_frame : fi->frameMap[fi->current_frame];
	if (
		( fi-> frameCount >= 0 && fi-> frame >= fi-> frameCount) ||
		( !(c-> info-> IOFlags & IMG_LOAD_MULTIFRAME) && fi-> frame > 0)
	) {
		if ( !(c-> info-> IOFlags & IMG_LOAD_MULTIFRAME) && fi-> frameCount < 0)
			fi-> frameCount = fi->current_frame;
		if ( fi->incrementalLoad) {
			/* that means, codec bothered to set frameCount at last - report no error then */
			fi-> eof_is_not_an_error = true;
			goto EXIT_NOW;
		}
		out("Frame index out of range");
	}

	/* query profile */
	save[0] = fi->loadExtras;
	save[1] = fi->noImageData;
	save[2] = fi->iconUnmask;
	save[3] = fi->blending;
	save[4] = fi->noIncomplete;
	save_profile = fi->profile;

	if ( fi->profiles && ( fi->current_frame <= fi->profiles_len)) {
		HV * hv;
		SV ** holder = av_fetch( fi->profiles, fi->current_frame, 0);
		if ( !holder) outd("Array panic on 'profiles[%d]' property", fi->current_frame);
		if ( SvOK( *holder)) {
			if ( SvROK( *holder) && SvTYPE( SvRV( *holder)) == SVt_PVHV)
				hv = ( HV*) SvRV( *holder);
			else
				outd("Not a hash passed to 'profiles[%d]' property", fi->current_frame);
			fi->profile = newHV();
			apc_img_profile_add( fi->profile, fi->cached_commons, fi->cached_commons);
			c-> vmt-> load_check_in( c, fi->profile, hv);
			apc_img_profile_add( fi->profile, hv, fi->cached_defaults);
			{
				HV * profile = hv;
				if ( pexist( loadExtras))
					fi->loadExtras  = pget_B( loadExtras);
				if ( pexist( noImageData))
					fi->noImageData = pget_B( noImageData);
				if ( pexist( iconUnmask))
					fi->iconUnmask = pget_B( iconUnmask);
				if ( pexist( blending))
					fi->blending = pget_B( blending);
			}
		}
	}

	fi->jointFrame = ( fi->frame == fi->last_frame + 1);
	fi->last_frame = fi-> frame;

	/* query className */
	{
		HV * profile = fi->profile;
		if ( pexist( className)) {
			PVMT vmt;
			className = pget_c( className);
			vmt = gimme_the_vmt( className);
			while ( vmt && vmt != (PVMT)CImage)
				vmt = vmt-> base;
			if ( !vmt)
				outd("class '%s' is not a Prima::Image descendant", className);
		}
	}

	/* create storage */
	if (target == NULL_HANDLE) {
		HV * profile = newHV();
		fi->object = Object_create( className, profile);
		sv_free(( SV *) profile);
		if ( !fi->object)
			outd("Failed to create object '%s'", className);
	} else
		fi->object = target;

	if ( fi->iconUnmask && kind_of( fi->object, CIcon))
		PIcon( fi->object)-> autoMasking = amNone;

	fi->frameProperties = newHV();
	if ( fi->loadExtras && c-> info-> fileType)
		(void) hv_store( fi-> frameProperties, "codecID", 7, newSViv( fi->codecID), 0);

	/* loading image */
	if ( !c-> vmt-> load( c, fi)) {
		sv_free(( SV *) fi-> frameProperties);
		if ( fi->object != target)
			Object_destroy( fi->object);
		fi-> frameProperties = NULL;
		fi-> object = NULL_HANDLE;
		if ( fi-> incrementalLoad) {
			if ( fi-> frameCount < 0)
				fi-> frameCount = fi-> frame;
			else if ( fi->frame < fi->frameCount )
				err = true;
			fi-> eof_is_not_an_error = true;
			/* or it is EOF, report no error then */
			goto EXIT_NOW;
		}
		err = true;
		goto EXIT_NOW;
	}

	if ( fi-> loadExtras && fi-> wasTruncated)
		(void) hv_store( fi-> frameProperties, "truncated", 9, newSVpv( fi->errbuf, 0 ), 0);

	/* checking for grayscale */
	{
		PImage i = ( PImage) fi-> object;
		if ( !( i-> type & imGrayScale))
			switch ( i-> type & imBPP) {
			case imbpp1:
				if ( i-> palSize == 2 && memcmp( i-> palette, stdmono_palette, sizeof( stdmono_palette)) == 0)
					i-> type |= imGrayScale;
				break;
			case imbpp4:
				if ( i-> palSize == 16 && memcmp( i-> palette, std16gray_palette, sizeof( std16gray_palette)) == 0)
					i-> type |= imGrayScale;
				break;
			case imbpp8:
				if ( i-> palSize == 256 && memcmp( i-> palette, std256gray_palette, sizeof( std256gray_palette)) == 0)
					i-> type |= imGrayScale;
				break;
			}
	}

	/* updating image */
	if ( !fi-> noImageData) {
		CImage( fi-> object)-> update_change( fi-> object);
		/* loaders are ok to be lazy and use autoMasking for post-creation of the mask */
		if ( fi-> iconUnmask && kind_of( fi-> object, CIcon))
			PIcon( fi-> object)-> autoMasking = amNone;
	}

	/* applying extras */
	if ( fi-> loadExtras) {
		HV * extras = newHV();
		SV * sv = newRV_noinc(( SV *) extras);

		apc_img_profile_add( extras, fi-> fileProperties,  fi-> fileProperties);
		apc_img_profile_add( extras, fi-> frameProperties, fi-> frameProperties);
		if ( fi->current_frame == 0) firstObjectExtras = extras;
		(void) hv_store(( HV* )SvRV((( PAnyObject) fi-> object)-> mate), "extras", 6, newSVsv( sv), 0);
		sv_free( sv);
	} else if ( fi-> noImageData) { /* no extras, report dimensions only */
		HV * extras = newHV();
		SV * sv = newRV_noinc(( SV *) extras), **item;
		if (( item = hv_fetch( fi-> frameProperties, "width", 5, 0)) && SvOK( *item))
			(void) hv_store( extras, "width", 5, newSVsv( *item), 0);
		else
			(void) hv_store( extras, "width", 5, newSViv(PImage(fi->object)-> w), 0);
		if (( item = hv_fetch( fi-> frameProperties, "height", 6, 0)) && SvOK( *item))
			(void) hv_store( extras, "height", 6, newSVsv( *item), 0);
		else
			(void) hv_store( extras, "height", 6, newSViv(PImage(fi->object)-> h), 0);
		(void) hv_store(( HV* )SvRV((( PAnyObject) fi-> object)-> mate), "extras", 6, newSVsv( sv), 0);
		sv_free( sv);
	}

	sv_free(( SV *) fi-> frameProperties);
	if ( firstObjectExtras)
		(void) hv_store( firstObjectExtras, "frames", 6, newSViv( fi->frameCount), 0);
	fi->current_frame++;

EXIT_NOW:
	/* restore from custom profile, if anything */
	if ( save_profile ) {
		fi->loadExtras   = save[0];
		fi->noImageData  = save[1];
		fi->iconUnmask   = save[2];
		fi->blending     = save[3];
		fi->noIncomplete = save[4];
		if ( fi->profile != save_profile)
			sv_free(( SV *) fi->profile);
		fi->profile      = save_profile;
	}

	#undef out
	#undef outd

	return err ? NULL_HANDLE : fi->object;
}

void
apc_img_close_load( PImgLoadFileInstance fi )
{
	PImgCodec c = fi->codec;
	if ( fi->instance )
		c-> vmt-> close_load( c, fi);
	if ( fi-> cached_defaults)
		sv_free(( SV *) fi-> cached_defaults);
	if ( fi-> cached_commons)
		sv_free(( SV *) fi-> cached_commons);
	if ( fi-> fileProperties)
		sv_free((SV *) fi-> fileProperties);
	if ( fi->req_is_stdio && fi->req != NULL && fi->req-> handle != NULL)
		fclose(( FILE*) fi->req-> handle);
	free( fi->frameMap);
	free(fi);
}


PList
apc_img_load( Handle self, char * fileName, Bool is_utf8, PImgIORequest ioreq,  HV * profile, char * error)
{
	PList ret;
	PImgLoadFileInstance fi;

	if ( !( ret = plist_create( 8, 8))) {
		if ( error ) strcpy( error, "Nor enough memory");
		return NULL;
	}

	if ( !( fi = apc_img_open_load( self, fileName, is_utf8, ioreq, profile, error )))
		return NULL;

	while ( 1 ) {
		Handle img;
		img = apc_img_load_next_frame( self, fi, error );
		if ( img == NULL_HANDLE ) {
			if ( !fi->eof_is_not_an_error )
				list_add( ret, NULL_HANDLE );
			break;
		} else
			list_add( ret, img );
		self = NULL_HANDLE; /* only load 1st frame to self */
	}

	apc_img_close_load( fi );
	return ret;
}

int
apc_img_frame_count( char * fileName, Bool is_utf8, PImgIORequest ioreq )
{
	PImgCodec c = NULL;
	ImgLoadFileInstance fi;
	int i, frameMap, ret = 0;
	char error[256];
	ImgIORequest sioreq;
	int load_mask;

	CHK;
	memset( &fi, 0, sizeof( fi));
	/* open file */
	if ( ioreq == NULL) {
		memcpy( &sioreq, &std_ioreq, sizeof( sioreq));
		if (( sioreq. handle = prima_open_file( fileName, is_utf8, "rb")) == NULL)
			goto EXIT_NOW;
		fi. req = &sioreq;
		fi. req_is_stdio = true;
		load_mask = IMG_LOAD_FROM_FILE;
	} else {
		fi. req = ioreq;
		fi. req_is_stdio = false;
		load_mask = IMG_LOAD_FROM_STREAM;
	}

	/* assigning request */
	fi. fileName = fileName;
	fi. is_utf8  = is_utf8;
	fi. frameMapSize   = frameMap = 0;
	fi. frameMap       = &frameMap;
	fi. loadExtras     = true;
	fi. noImageData    = true;
	fi. iconUnmask     = false;
	fi. blending       = false;
	fi. noIncomplete   = false;
	fi. extras         = newHV();
	fi. fileProperties = newHV();
	fi. frameCount = -1;
	fi. errbuf     = error;
	fi. stop       = false;

	/* finding codec */
	{
		Bool * loadmap = ( Bool*) malloc( sizeof( Bool) * imgCodecs. count);

		if ( !loadmap)
			return 0;
		memset( loadmap, 0, sizeof( Bool) * imgCodecs. count);
		for ( i = 0; i < imgCodecs. count; i++) {
			c = ( PImgCodec ) ( imgCodecs. items[ i]);
			if ( !c-> instance)
				c-> instance = c-> vmt-> init( &c->info, c-> initParam);
			if ( !c-> instance) { /* failed to initialize, retry next time */
				loadmap[ i] = true;
				continue;
			}
		}

		c = NULL;

		/* finding by extension first */
		if ( fileName) {
			int fileNameLen = strlen( fileName);
			for ( i = 0; i < imgCodecs. count; i++) {
				int j = 0, found = false;
				if ( loadmap[ i]) continue;
				c = ( PImgCodec ) ( imgCodecs. items[ i]);
				while ( c-> info-> fileExtensions[ j]) {
					char * ext = c-> info-> fileExtensions[ j];
					int extLen = strlen( ext);
					if ( extLen < fileNameLen && stricmp( fileName + fileNameLen - extLen, ext) == 0) {
						found = true;
						break;
					}
					j++;
				}
				if ( found) {
					loadmap[ i] = true;

					if ( !( c-> info-> IOFlags & load_mask)) {
						c = NULL;
						continue;
					}
					if (( fi. instance = c-> vmt-> open_load( c, &fi)) != NULL)
						break;

					if ( fi. stop) {
						free( loadmap);
						goto EXIT_NOW;
					}
				}
				c = NULL;
			}
		}

		if ( c == NULL) {
			for ( i = 0; i < imgCodecs. count; i++) {
				if ( loadmap[ i]) continue;
				c = ( PImgCodec ) ( imgCodecs. items[ i]);
				if ( !( c-> info-> IOFlags & load_mask)) {
						c = NULL;
						continue;
				}
				if (( fi. instance = c-> vmt-> open_load( c, &fi)) != NULL)
					break;
				if ( fi. stop) {
					free( loadmap);
					goto EXIT_NOW;
				}
				c = NULL;
			}
		}
		free( loadmap);
		if ( !c) goto EXIT_NOW;
	}

	/* can tell now? */

	if ( fi. frameCount >= 0) {
		c-> vmt-> close_load( c, &fi);
		ret = fi. frameCount;
		goto EXIT_NOW;
	}

	if ( !( c-> info-> IOFlags & IMG_LOAD_MULTIFRAME)) {
		c-> vmt-> close_load( c, &fi);
		ret = 1; /* single-framed file. what else? */
		goto EXIT_NOW;
	}

	/* if can't, trying to load huge index, hoping that if */
	/* codec have a sequential access, it eventually meet the  */
	/* EOF and report the frame count */
	{
		HV * profile = newHV();
		fi. object = Object_create( "Prima::Image", profile);
		sv_free(( SV *) profile);
		frameMap = fi. frame = INT_MAX;
		fi. frameProperties = newHV();
	}

	/* loading image */
	if ( c-> vmt-> load( c, &fi) || fi. frameCount >= 0) {
		/* well, INT_MAX frame is ok, and maybe more, but can't report more anyway */
		c-> vmt-> close_load( c, &fi);
		ret = ( fi. frameCount < 0) ? INT_MAX : fi. frameCount;
		goto EXIT_NOW;
	}

	/* can't report again - so loading as may as we can */
	fi. loadAll = true;
	for ( i = 0; i < INT_MAX; i++) {
		fi. jointFrame = i > 0;
		frameMap = fi. frame = i;
		if ( !( c-> info-> IOFlags & IMG_LOAD_MULTIFRAME)) {
			c-> vmt-> close_load( c, &fi);
			if ( !( fi. instance = c-> vmt-> open_load( c, &fi))) {
				ret = i;
				goto EXIT_NOW;
			}
		}
		if ( !c-> vmt-> load( c, &fi) || fi. frameCount >= 0) {
			c-> vmt-> close_load( c, &fi);
			ret = ( fi. frameCount < 0) ? i : fi. frameCount;
			goto EXIT_NOW;
		}
	}

	c-> vmt-> close_load( c, &fi);

EXIT_NOW:;
	if ( fi. object)
		Object_destroy( fi. object);
	if ( fi. extras)
		sv_free(( SV *) fi. extras);
	if ( fi. frameProperties)
		sv_free(( SV *) fi. frameProperties);
	if ( fi. fileProperties)
		sv_free(( SV *) fi. fileProperties);
	if ( ioreq == NULL && fi.req != NULL && fi. req-> handle != NULL)
		fclose(( FILE*) fi. req-> handle);
	return ret;
}

int
apc_img_save( Handle self, char * fileName, Bool is_utf8, PImgIORequest ioreq, HV * profile, char * error)
{
	dPROFILE;
	int i;
	PImgCodec c = NULL;
	ImgSaveFileInstance fi;
	AV * images = NULL;
	HV * def = NULL, * commonHV = NULL;
	Bool err = false;
	int codecID = -1;
	int xself = self ? 1 : 0;
	int ret = 0;
	Bool autoConvert = true;
	ImgIORequest sioreq;
	int save_mask;
	char dummy_error_buf[256];

#define out(x){ err = true;\
	strlcpy( fi.errbuf, x, 256);\
	goto EXIT_NOW;}

#define outd(x,d){ err = true;\
	snprintf( fi.errbuf, 256, x, d);\
	goto EXIT_NOW;}

	CHK;
	memset( &fi, 0, sizeof( fi));

	if ( pexist( append) && pget_B( append))
		fi. append = true;

	if ( pexist( autoConvert))
		autoConvert = pget_B( autoConvert);

	/* open file */
	if ( fi. append && ioreq == NULL) {
		FILE * f = ( FILE *) prima_open_file( fileName, is_utf8, "rb");
		if ( !f)
			fi. append = false;
		else
			fclose( f);
	}

	fi. errbuf = error ? error : dummy_error_buf;
	if ( ioreq == NULL) {
		memcpy( &sioreq, &std_ioreq, sizeof( sioreq));
		if (( sioreq. handle = prima_open_file( fileName, is_utf8, fi. append ? "rb+" : "wb+" )) == NULL)
			out( strerror( errno));
		fi. req = &sioreq;
		fi. req_is_stdio = true;
		save_mask = IMG_SAVE_TO_FILE;
	} else {
		fi. req = ioreq;
		fi. req_is_stdio = false;
		save_mask = IMG_SAVE_TO_STREAM;
	}

	fi. fileName     = fileName;
	fi. is_utf8      = is_utf8;

	fi. frameMapSize = xself;
	if ( pexist( images)) {
		SV * sv = pget_sv( images);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			images = ( AV *) SvRV( sv);
			fi. frameMapSize += av_len( images) + 1;
		} else
			out("Not an array passed to 'images' property");
	}
	if ( fi. frameMapSize == 0)
		out("Nothing to save");

	/* fill array of objects */
	if ( !( fi. frameMap = ( Handle *) malloc( sizeof( Handle) * fi. frameMapSize)))
		out("Not enough memory");
	memset( fi. frameMap, 0, sizeof( Handle) * fi. frameMapSize);

	for ( i = 0; i < fi. frameMapSize; i++) {
		Handle obj = NULL_HANDLE;

		/* query profile */
		if ( self && (i == 0)) {
			obj = self;
			if ( !kind_of( obj, CImage))
				out("Not a Prima::Image descendant passed");
			if ( PImage(obj)-> w == 0 || PImage(obj)-> h == 0)
				out("Cannot save a null image");
		} else if ( images) {
			SV ** holder = av_fetch( images, i - xself, 0);
			if ( !holder) outd("Array panic on 'images[%d]' property", i - xself);
			obj = gimme_the_mate( *holder);
			if ( !obj)
				outd("Invalid object reference passed in 'images[%d]'", i - xself);
			if ( !kind_of( obj, CImage))
				outd("Not a Prima::Image descendant passed in 'images[%d]'", i - xself);
			if ( PImage(obj)-> w == 0 || PImage(obj)-> h == 0)
				out("Cannot save a null image");
		} else
			out("Logic error");
		fi. frameMap[ i] = obj;
	}

	/* all other properties to be parsed by codec */
	fi. extras = profile;

	/* finding codec */
	strcpy( fi.errbuf, "No appropriate codec found");
	{
		Bool * savemap = ( Bool*) malloc( sizeof( Bool) * imgCodecs. count);

		if ( !savemap)
			out("Not enough memory");
		memset( savemap, 0, sizeof( Bool) * imgCodecs. count);

		for ( i = 0; i < imgCodecs. count; i++) {
			c = ( PImgCodec ) ( imgCodecs. items[ i]);
			if ( !c-> instance)
				c-> instance = c-> vmt-> init( &c->info, c-> initParam);
			if ( !c-> instance) { /* failed to initialize, retry next time */
				savemap[ i] = true;
				continue;
			}
		}

		/* checking 'codecID', if available */
		{
			SV * c = NULL;
			if ( pexist( codecID))
				c = pget_sv( codecID);
			else if ( self &&  (( PAnyObject) self)-> mate &&
				hv_exists(( HV*)SvRV((( PAnyObject) self)-> mate), "extras", 6)
			) {
				SV ** sv = hv_fetch(( HV*)SvRV((( PAnyObject) self)-> mate), "extras", 6, 0);
				if ( sv && SvOK( *sv) && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVHV) {
					HV * profile = ( HV *) SvRV( *sv);
					if ( pexist( codecID))
						c = pget_sv( codecID);
				}
			}
			if ( c && SvOK( c)) { /* accept undef */
				codecID = SvIV( c);
				if ( codecID < 0) codecID = imgCodecs. count - codecID;
			}
		}

		/* find codec */
		c = NULL;
		if ( codecID >= 0) {
			if ( codecID >= imgCodecs. count)
				out("Codec index out of range");

			c = ( PImgCodec ) ( imgCodecs. items[ codecID]);
			if ( !( c-> info-> IOFlags & save_mask))
					out( ioreq ?
						"Codec cannot save images to streams" :
						"Codec cannot save images");

			if ( fi. frameMapSize > 1 &&
					!( c-> info-> IOFlags & IMG_SAVE_MULTIFRAME))
				out("Codec cannot save mutiframe images");
			if ( fi. append &&
					!( c-> info-> IOFlags & IMG_SAVE_APPEND))
				out("Codec cannot append frames");

			if (( fi. instance = c-> vmt-> open_save( c, &fi)) == NULL)
				out("Codec cannot handle this file");

			if ( !autoConvert) {
				int j, *k = c-> info-> saveTypes, ok = 0;
				for ( j = 0; j < fi. frameMapSize; j++) {
					int type = PImage( fi. frameMap[j])-> type;
					while ( *k) {
						if ( type == *k) {
							ok = 1;
							break;
						}
						k++;
					}
				}
				if ( !ok)
					out("Image type(s) not supported by the codec specified");
			}
		}

		if ( !c && fileName) {
			int fileNameLen = strlen( fileName);
			/* finding codec by extension  */
			for ( i = 0; i < imgCodecs. count; i++) {
				int j = 0, found = false;
				if ( savemap[ i]) continue;
				c = ( PImgCodec ) ( imgCodecs. items[ i]);
				while ( c-> info-> fileExtensions[ j]) {
					char * ext = c-> info-> fileExtensions[ j];
					int extLen = strlen( ext);
					if ( extLen < fileNameLen && stricmp( fileName + fileNameLen - extLen, ext) == 0) {
						found = true;
						break;
					}
					j++;
				}

				if ( found) {
					savemap[ i] = true;
					if ( !( c-> info-> IOFlags & save_mask)) {
						c = NULL;
						continue;
					}

					if ( fi. frameMapSize > 1
						&& !( c-> info-> IOFlags & IMG_SAVE_MULTIFRAME)) {
						c = NULL;
						continue;
					}
					if ( fi. append
						&& !( c-> info-> IOFlags & IMG_SAVE_APPEND)) {
						c = NULL;
						continue;
					}

					if ( !autoConvert) {
						int j, *k = c-> info-> saveTypes, ok = 0;
						for ( j = 0; j < fi. frameMapSize; j++) {
							int type = PImage( fi. frameMap[j])-> type;
							while ( *k) {
								if ( type == *k) {
									ok = 1;
									break;
								}
								k++;
							}
						}
						if ( !ok) {
							c = NULL;
							continue;
						}
					}

					if (( fi. instance = c-> vmt-> open_save( c, &fi)) != NULL)
						break;
				}
				c = NULL;
			}
		}

		free( savemap);
		if ( !c) { /* use pre-formatted error string */
			err = true;
			goto EXIT_NOW;
		}
	}

	snprintf( fi.errbuf, 256, "Error saving %s", fileName ? fileName : "to stream");

	/* use common profile */
	def = c-> vmt-> save_defaults( c);
	commonHV = newHV();
	if ( profile) {
		c-> vmt-> save_check_in( c, commonHV, profile);
		apc_img_profile_add( commonHV, profile, def);
	}

	/* saving */
	for ( i = 0; i < fi. frameMapSize; i++) {
		HV * profile = commonHV;
		PImage im;

		im = ( PImage) fi. frameMap[ i];
		if ( im-> mate && hv_exists(( HV*)SvRV( im-> mate), "extras", 6)) {
			SV ** sv = hv_fetch(( HV*)SvRV( im-> mate), "extras", 6, 0);
			if ( sv && SvOK( *sv) && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVHV) {
				HV * hv = ( HV *) SvRV( *sv);
				profile = newHV();
				apc_img_profile_add( profile, commonHV, commonHV);
				c-> vmt-> save_check_in( c, profile, hv);
				apc_img_profile_add( profile, hv, def);
			}
		}

		fi. frame  = i;
		fi. object = fi. frameMap[ i];
		fi. objectExtras = profile;

		/* converting image to format with maximum bit depth and category flags match */
		if ( autoConvert) {
			int *k = c-> info-> saveTypes;
			int max = *k & imBPP, best = *k, supported = false;
			int flags = im-> type & imCategory, bestflags = *k & imCategory, bestmatch;
#define dBITS(a) int i = 0x80, match = ( flags & (a)) >> 8
#define CALCBITS(x) { \
	x = 0;\
	while ( i >>= 1 ) if ( match & i ) x++; \
}
			{
				dBITS( bestflags );
				CALCBITS( bestmatch )
			}
			while ( *k) {
				if ( im-> type == *k) {
					supported = true;
					break;
				}
				if ( max < ( *k & imBPP)) {
					dBITS( bestflags = ( *k & imCategory));
					max       = *k & imBPP;
					best      = *k;
					CALCBITS( bestmatch );
				} else if ( max == ( *k & imBPP)) {
					dBITS( *k );
					int testmatch;
					CALCBITS( testmatch );
					if ( testmatch > bestmatch ) {
						best      = *k;
						bestflags = *k & imCategory;
						bestmatch = testmatch;
					}
				}
				k++;
			}
			if ( !supported) {
				im-> self-> set_type(( Handle) im, best);
				if ( best != im-> type) outd("Failed converting image to type '%04x'", best);
			}
		}

		/* saving image */
		if ( !c-> vmt-> save( c, &fi)) {
			c-> vmt-> close_save( c, &fi);
			if ( fi. objectExtras != commonHV) sv_free(( SV *) fi. objectExtras);
			err = true;
			goto EXIT_NOW;
		}

		if ( fi. objectExtras != commonHV) sv_free(( SV *) fi. objectExtras);
		ret++;
	}

	c-> vmt-> close_save( c, &fi);

EXIT_NOW:;
	free( fi. frameMap);
	if ( ioreq == NULL && fi. req != NULL && fi. req-> handle != NULL)
		fclose(( FILE*) fi. req-> handle);
	if ( err && fileName)
		apc_fs_unlink( fileName, is_utf8 );
	if ( def)
		sv_free(( SV *) def);
	if ( commonHV)
		sv_free(( SV *) commonHV);
	return err ? -ret : ret;
#undef out
#undef outd
}
void
apc_img_codecs( PList ret)
{
	int i;
	PImgCodec c;

	CHK;
	for ( i = 0; i < imgCodecs. count; i++) {
		c = ( PImgCodec ) ( imgCodecs. items[ i]);
		if ( !c-> instance)
			c-> instance = c-> vmt-> init( &c->info, c-> initParam);
		if ( !c-> instance)  /* failed to initialize, retry next time */
			continue;
		list_add( ret, ( Handle) c);
	}
}

int
apc_img_read_palette( PRGBColor palBuf, SV * palette, Bool triplets)
{
	AV * av;
	int i, count;
	Byte buf[768];

	if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV))
		return 0;
	av = (AV *) SvRV( palette);
	count = av_len( av) + 1;

	if ( triplets) {
		if ( count > 768) count = 768;
		count -= count % 3;

		for ( i = 0; i < count; i++)
		{
			SV **itemHolder = av_fetch( av, i, 0);
			if ( itemHolder == NULL) return 0;
			buf[ i] = SvIV( *itemHolder);
		}
		memcpy( palBuf, buf, count);
		return count/3;
	} else {
		int j;
		if ( count > 256) count = 256;

		for ( i = 0, j = 0; i < count; i++)
		{
			Color c;
			SV **itemHolder = av_fetch( av, i, 0);
			if ( itemHolder == NULL) return 0;
			c = (Color)(SvIV( *itemHolder));
			buf[j++] = c & 0xFF;
			buf[j++] = (c >> 8) & 0xFF;
			buf[j++] = (c >> 16) & 0xFF;
		}
		memcpy( palBuf, buf, j);
		return count;
	}
}


static AV * fill_plist( char * key, char ** list, HV * profile)
{
	AV * av = newAV();
	if ( !list) list = imgPVEmptySet;
	while ( *list) {
		av_push( av, newSVpv( *list, 0));
		list++;
	}
	(void) hv_store( profile, key, strlen( key), newRV_noinc(( SV *) av), 0);
	return av;
}

static void fill_ilist( char * key, int * list, HV * profile)
{
	AV * av = newAV();
	if ( !list) list = imgIVEmptySet;
	while ( *list) {
		av_push( av, newSViv( *list));
		list++;
	}
	(void) hv_store( profile, key, strlen( key), newRV_noinc(( SV *) av), 0);
}


HV *
apc_img_info2hash( PImgCodec codec)
{
	HV * profile, * hv;
	AV * av;
	PImgCodecInfo c;

	CHK;
	profile = newHV();
	if ( !codec) return profile;

	if ( !codec-> instance)
		codec-> instance = codec-> vmt-> init( &codec->info, codec-> initParam);
	if ( !codec-> instance)
		return profile;
	c = codec-> info;

	pset_c( name,   c-> name);
	pset_c( vendor, c-> vendor);
	pset_i( versionMajor, c-> versionMaj);
	pset_i( versionMinor, c-> versionMin);
	fill_plist( "fileExtensions", c-> fileExtensions, profile);
	pset_c( fileType, c-> fileType);
	pset_c( fileShortType, c-> fileShortType);
	fill_plist( "featuresSupported", c-> featuresSupported, profile);
	pset_c( module,  c-> primaModule);
	pset_c( package, c-> primaPackage);
	pset_i( canLoad,         c-> IOFlags & IMG_LOAD_FROM_FILE);
	pset_i( canLoadStream  , c-> IOFlags & IMG_LOAD_FROM_STREAM);
	pset_i( canLoadMultiple, c-> IOFlags & IMG_LOAD_MULTIFRAME);
	pset_i( canSave        , c-> IOFlags & IMG_SAVE_TO_FILE);
	pset_i( canSaveStream  , c-> IOFlags & IMG_SAVE_TO_STREAM);
	pset_i( canSaveMultiple, c-> IOFlags & IMG_SAVE_MULTIFRAME);
	pset_i( canAppend,       c-> IOFlags & IMG_SAVE_APPEND);

	fill_ilist( "types",  c-> saveTypes, profile);

	if ( c-> IOFlags & ( IMG_LOAD_FROM_FILE|IMG_LOAD_FROM_STREAM)) {
		hv = codec-> vmt-> load_defaults( codec);
		if ( c-> IOFlags & IMG_LOAD_MULTIFRAME) {
			(void) hv_store( hv, "index",        5, newSViv(0),     0);
			(void) hv_store( hv, "map",          3, newSVsv(NULL_SV), 0);
			(void) hv_store( hv, "loadAll",      7, newSViv(0),     0);
			(void) hv_store( hv, "wantFrames",  10, newSViv(0),     0);
		}
		(void) hv_store( hv, "loadExtras",   10, newSViv(0),     0);
		(void) hv_store( hv, "noImageData",  11, newSViv(0),     0);
		(void) hv_store( hv, "iconUnmask",   10, newSViv(0),     0);
		(void) hv_store( hv, "blending",      8, newSViv(0),     0);
		(void) hv_store( hv, "noIncomplete", 12, newSViv(0),     0);
		(void) hv_store( hv, "className",     9, newSVpv("Prima::Image", 0), 0);
	} else
		hv = newHV();
	pset_sv_noinc( loadInput, newRV_noinc(( SV *) hv));

	av = fill_plist( "loadOutput", c-> loadOutput, profile);
	if ( c-> IOFlags & ( IMG_LOAD_FROM_FILE|IMG_LOAD_FROM_STREAM)) {
		if ( c-> IOFlags & IMG_LOAD_MULTIFRAME)
			av_push( av, newSVpv( "frames", 0));
		av_push( av, newSVpv( "height", 0));
		av_push( av, newSVpv( "width",  0));
		av_push( av, newSVpv( "codecID", 0));
		av_push( av, newSVpv( "truncated", 0));
	}

	if ( c-> IOFlags & ( IMG_SAVE_TO_FILE|IMG_SAVE_TO_STREAM)) {
		hv = codec-> vmt-> save_defaults( codec);
		if ( c-> IOFlags & IMG_SAVE_MULTIFRAME)
			(void) hv_store( hv, "append",       6, newSViv(0), 0);
		(void) hv_store( hv, "autoConvert", 11, newSViv(1), 0);
		(void) hv_store( hv, "codecID",     7,  newSVsv( NULL_SV), 0);
	} else
		hv = newHV();
	pset_sv_noinc( saveInput, newRV_noinc(( SV *) hv));

	fill_plist( "mime", c-> mime, profile);

	return profile;
}

char * imgPVEmptySet[] = { NULL };
int    imgIVEmptySet[] = { 0 };

void
apc_img_notify_header_ready( PImgLoadFileInstance fi)
{
	Event e = { cmImageHeaderReady };
	e. gen. p = fi-> frameProperties;
	CImage( fi-> object)-> message( fi-> object, &e);
}

void
apc_img_notify_scanlines_ready( PImgLoadFileInstance fi, int scanlines, int direction)
{
	Event e;
	int height, width;
	unsigned int dt;
	struct timeval t;

	fi-> lastCachedScanline += scanlines;
	gettimeofday( &t, NULL);
	dt =
		t.tv_sec * 1000 + t.tv_usec / 1000 -
		fi-> lastEventTime.tv_sec * 1000 - fi-> lastEventTime.tv_usec / 1000;

	if ( dt < fi-> eventDelay) return;
	if ( fi-> lastEventScanline == fi-> lastCachedScanline) return;

	e. cmd = cmImageDataReady;
	height = PImage( fi-> object)-> h;
	width  = PImage( fi-> object)-> w;

	switch ( direction ) {
	case SCANLINES_DIR_TOP_TO_BOTTOM:
		e. gen. R. left   = 0;
		e. gen. R. right  = width - 1;
		e. gen. R. bottom = height - fi-> lastCachedScanline;
		e. gen. R. top    = height - fi-> lastEventScanline  - 1;
		break;
	case SCANLINES_DIR_BOTTOM_TO_TOP:
		e. gen. R. left   = 0;
		e. gen. R. right  = width - 1;
		e. gen. R. bottom = fi-> lastEventScanline;
		e. gen. R. top    = fi-> lastCachedScanline - 1;
		break;
	case SCANLINES_DIR_LEFT_TO_RIGHT:
		e. gen. R. left   = fi-> lastEventScanline;
		e. gen. R. right  = fi-> lastCachedScanline - 1;
		e. gen. R. bottom = 0;
		e. gen. R. top    = height - 1;
		break;
	case SCANLINES_DIR_RIGHT_TO_LEFT:
		e. gen. R. left    = width - fi-> lastCachedScanline;
		e. gen. R. right   = width - fi-> lastEventScanline - 1;
		e. gen. R. bottom  = 0;
		e. gen. R. top     = height - 1;
		break;
	}
	CImage( fi-> object)-> message( fi-> object, &e);

	gettimeofday( &fi-> lastEventTime, NULL);
	fi-> lastEventScanline = fi-> lastCachedScanline;
}
#ifdef __cplusplus
}
#endif
