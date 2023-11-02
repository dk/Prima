#include "img.h"
#include <sys/types.h>
#include "apricot.h"
#include "Image.h"
#include "Image_private.h"
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PerlIO
typedef PerlIO *FileStream;
#else
#define PERLIO_IS_STDIO 1
typedef FILE *FileStream;
#define PerlIO_fileno(f) fileno(f)
#endif

static ssize_t
img_perlio_read( void * f, size_t bufsize, void * buffer)
{
#ifdef PerlIO
	return PerlIO_read(( FileStream) f, buffer, bufsize);
#else
	return fread( buffer, 1, bufsize, ( FileStream) f);
#endif
}

static ssize_t
img_perlio_write( void * f, size_t bufsize, void * buffer)
{
#ifdef PerlIO
	return PerlIO_write( ( FileStream) f, buffer, bufsize);
#else
	return fwrite( buffer, 1, bufsize, ( FileStream) f);
#endif
}

static int
img_perlio_seek( void * f, long offset, int whence)
{
#ifdef PerlIO
	return PerlIO_seek( ( FileStream) f, offset, whence);
#else
	return fseek( ( FileStream) f, offset, whence);
#endif
}

static long
img_perlio_tell( void * f)
{
#ifdef PerlIO
	return PerlIO_tell( ( FileStream) f);
#else
	return ftell( ( FileStream) f);
#endif
}

static int
img_perlio_flush( void * f)
{
#ifdef PerlIO
	return PerlIO_flush( ( FileStream) f);
#else
	return fflush( ( FileStream) f);
#endif
}

static int
img_perlio_error( void * f)
{
#ifdef PerlIO
	return PerlIO_error( ( FileStream) f);
#else
	return ferror( ( FileStream) f);
#endif
}

static ImgIORequest perl_ioreq = {
	img_perlio_read,
	img_perlio_write,
	img_perlio_seek,
	img_perlio_tell,
	img_perlio_flush,
	img_perlio_error
};

static ImgIORequest*
fill_ioreq( SV * sv, ImgFileIOCommon *req )
{
	FileStream f = NULL;
	ImgIORequest *pioreq;

	if ( SvROK(sv) && SvTYPE( SvRV( sv)) == SVt_PVGV)
		f = IoIFP(sv_2io(sv));

	if ( f != NULL) {
		pioreq             = &req->sioreq;
		req->sioreq        = perl_ioreq;
		req->sioreq.handle = f;
		req->fileName      = NULL;
		req->is_utf8       = false;
	} else {
		req->fileName      = ( char *) SvPV_nolen(sv);
		req->is_utf8       = prima_is_utf8_sv(sv);
		pioreq             = NULL;
	}

	return pioreq;
}

XS( Image_load_FROMPERL)
{
	dPROFILE;
	dXSARGS;
	Handle self;
	SV * sv;
	HV *profile;
	PList ret = NULL;
	Bool err = false;
	ImgIORequest *pioreq;
	ImgFileIOCommon sioreq;
	char error[256];
	Bool open_load = false, load_next_frame = false, close_load = false;

	if (( items < 2) || (( items % 2) != 0))
		croak("Invalid usage of Prima::Image::load");

	self    = gimme_the_mate( ST( 0));
	sv      = ST(1);
	profile = parse_hv( ax, sp, items, mark, 2, "Image::load");

	if ( pexist(session) && pget_B(session)) {
		if ( !self )
			croak("Cannot start loading session without an object");
		if ( var-> loading_session ) {
			if ( SvOK(sv))
				croak("Another loading session is in progress");

			load_next_frame = true;
			if ( pexist(rewind)) {
				(( PImgLoadFileInstance) var-> loading_session)->frame = pget_i(rewind);
				pdelete(rewind);
			}
		} else
			open_load = true;
	}

	if ( !pexist( className) && !load_next_frame)
		pset_c( className, self ? my-> className : ( char*) SvPV_nolen( ST( 0)));
	pset_i( eventMask, self ? var-> eventMask2 : 0);

	if ( !load_next_frame )
		pioreq = fill_ioreq(sv, &sioreq);

	if ( load_next_frame ) {
		Handle obj;
		PImgLoadFileInstance fi = (PImgLoadFileInstance) var-> loading_session;
		if ( !( ret = plist_create(1,1)))
			croak("Not enough memory");
		obj = apc_img_load_next_frame( NULL_HANDLE, fi, profile, error);
		if ( obj == NULL_HANDLE ) {
			err = true;
			close_load = true;
		} else
			fi-> frame++;
		list_add( ret, obj );
	} else if ( open_load ) {
		PImgLoadFileInstance fi;
		fi = apc_img_open_load( sioreq.fileName, sioreq.is_utf8, pioreq, profile, error);
		if ( fi ) {
			if ( fi-> loadExtras ) {
				HV * extras = newHV();
				SV * sv = newRV_noinc(( SV *) extras);
				apc_img_profile_add( extras, fi->fileProperties,  fi->fileProperties);
				(void) hv_store(( HV* )SvRV(var->mate), "extras", 6, newSVsv(sv), 0);
				sv_free( sv);
			}
			var-> loading_session = (void*) fi;
		} else
			err = true;
	} else
		ret = apc_img_load( self, sioreq.fileName, sioreq.is_utf8, pioreq, profile, error);

	sv_free(( SV *) profile);
	SPAGAIN;
	SP -= items;

	if ( open_load && var-> loading_session ) {
		XPUSHs( newSViv(1) );
	} else if ( ret) {
		int i;
		for ( i = 0; i < ret-> count; i++) {
			PAnyObject o = ( PAnyObject) ret-> items[i];
			if ( o && o-> mate && o-> mate != NULL_SV) {
				XPUSHs( sv_mortalcopy( o-> mate));
				if (( Handle) o != self)
				--SvREFCNT( SvRV( o-> mate));
			} else {
				XPUSHs( &PL_sv_undef);
				err = true;
			}
		}
		plist_destroy( ret);
	} else {
		XPUSHs( &PL_sv_undef);
		err = true;
	}

	if ( close_load ) {
		apc_img_close_load(( PImgLoadFileInstance) var-> loading_session );
		var-> loading_session = NULL;
	}

	/* This code breaks exception propagation chain
		since it uses $@ for its own needs  */
	if ( err)
		sv_setpv( GvSV( PL_errgv), error);
	else
		sv_setsv( GvSV( PL_errgv), NULL_SV);

	PUTBACK;
	return;
}

PList
Image_load_REDEFINED( SV * who, SV *filename, HV * profile)
{
	return NULL;
}

PList
Image_load( SV * who, SV *filename, HV * profile)
{
	PList ret;
	Handle self = gimme_the_mate( who);
	char error[ 256];
	if ( !pexist( className))
		pset_c( className, self ? my-> className : ( char*) SvPV_nolen( who));
	ret = apc_img_load( self, SvPV_nolen(filename), prima_is_utf8_sv(filename), NULL, profile, error);
	return ret;
}

XS( Image_save_FROMPERL)
{
	dPROFILE;
	dXSARGS;
	Handle self;
	HV *profile;
	SV *sv;
	int ret = 0;
	char error[256];
	ImgIORequest *pioreq;
	ImgFileIOCommon sioreq;
	Bool open_save = false, save_next_frame = false, close_save = false;

	if (( items < 2) || (( items % 2) != 0))
		croak("Invalid usage of Prima::Image::save");

	self    = gimme_the_mate( ST( 0));
	sv      = ST(1);
	profile = parse_hv( ax, sp, items, mark, 2, "Image::save");

	if ( pexist(session) && pget_B(session)) {
		if ( !self )
			croak("Cannot start saving session without an object");
		if ( var-> saving_session )
			save_next_frame = true;
		else
			open_save = true;
	}

	if ( !save_next_frame )
		pioreq = fill_ioreq(sv, &sioreq );

	if ( save_next_frame ) {
		Handle obj;
		PImgSaveFileInstance fi = (PImgSaveFileInstance) var-> saving_session;

		obj = gimme_the_mate(sv);
		if ( !obj || !kind_of(obj, CImage))
			croak("Bad image passed");

		if (apc_img_save_next_frame( obj, fi, profile, error))
			ret = 1;
		else
			close_save = true;
	} else if ( open_save ) {
		if ( !pexist(frames))
			croak("`frames' option expected");
		if (( var-> saving_session = apc_img_open_save(
			sioreq.fileName, sioreq.is_utf8,
			pget_i(frames),
			pioreq, profile, error
		)))
			ret = 1;
	} else
		ret = apc_img_save( self, sioreq.fileName, sioreq.is_utf8, pioreq, profile, error);

	sv_free(( SV *) profile);
	SPAGAIN;
	SP -= items;
	XPUSHs( sv_2mortal( newSViv(( ret > 0) ? ret : -ret)));

	if ( close_save ) {
		apc_img_close_save(( PImgSaveFileInstance) var-> saving_session, false );
		var-> saving_session = NULL;
	}

	/* This code breaks exception propagation chain
		since it uses $@ for its own needs  */
	if ( ret <= 0)
		sv_setpv( GvSV( PL_errgv), error);
	else
		sv_setsv( GvSV( PL_errgv), NULL_SV);
	PUTBACK;
	return;
}

int
Image_save_REDEFINED( SV * who, SV *filename, HV * profile)
{
	return 0;
}

int
Image_save( SV * who, SV *filename, HV * profile)
{
	Handle self = gimme_the_mate( who);
	char error[ 256];
	if ( !pexist( className))
		pset_c( className, self ? my-> className : ( char*) SvPV_nolen( who));
	return apc_img_save( self, SvPV_nolen(filename), prima_is_utf8_sv(filename), NULL, profile, error);
}

SV *
Image_codecs( SV * dummy, int codecID)
{
	PList p = plist_create( 16, 16);
	apc_img_codecs( p);
	if ( codecID < 0 ) {
		int i;
		AV * av = newAV();
		for ( i = 0; i < p-> count; i++) {
			PImgCodec c = ( PImgCodec ) p-> items[ i];
			HV * profile = apc_img_info2hash( c);
			pset_i( codecID, i);
			av_push( av, newRV_noinc(( SV *) profile));
		}
		plist_destroy( p);
		return newRV_noinc(( SV *) av);
	} else if ( codecID < p-> count ) {
		PImgCodec c = ( PImgCodec ) p-> items[ codecID];
		HV * profile = apc_img_info2hash( c);
		pset_i( codecID, codecID);
		return newRV_noinc(( SV *) profile);
	} else {
		return &PL_sv_undef;
	}
}


#ifdef __cplusplus
}
#endif
