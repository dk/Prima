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

XS( Image_load_FROMPERL)
{
	dXSARGS;
	Handle self;
	SV * sv;
	HV *profile;
	char *fn;
	PList ret;
	Bool err = false, is_utf8;
	FileStream f = NULL;
	ImgIORequest ioreq, *pioreq;
	char error[256];

	if (( items < 2) || (( items % 2) != 0))
		croak("Invalid usage of Prima::Image::load");

	self = gimme_the_mate( ST( 0));

	sv   = ST(1);
	if ( SvROK(sv) && SvTYPE( SvRV( sv)) == SVt_PVGV)
		f = IoIFP(sv_2io(ST(1)));

	if ( f != NULL) {
		pioreq        = &ioreq;
		ioreq. handle = f;
		ioreq. read   = img_perlio_read;
		ioreq. write  = img_perlio_write;
		ioreq. seek   = img_perlio_seek;
		ioreq. tell   = img_perlio_tell;
		ioreq. flush  = img_perlio_flush;
		ioreq. error  = img_perlio_error;
		fn            = NULL;
		is_utf8       = false;
	} else {
		fn            = ( char *) SvPV_nolen( ST( 1));
		is_utf8       = prima_is_utf8_sv(ST(1));
		pioreq        = NULL;
	}

	profile = parse_hv( ax, sp, items, mark, 2, "Image::load");
	if ( !pexist( className))
		pset_c( className, self ? my-> className : ( char*) SvPV_nolen( ST( 0)));
	pset_i( eventMask, self ? var-> eventMask2 : 0);
	ret = apc_img_load( self, fn, is_utf8, pioreq, profile, error);
	sv_free(( SV *) profile);
	SPAGAIN;
	SP -= items;
	if ( ret) {
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
	dXSARGS;
	Handle self;
	HV *profile;
	char *fn;
	int ret;
	char error[256];
	FileStream f = NULL;
	SV * sv;
	Bool is_utf8;
	ImgIORequest ioreq, *pioreq;

	if (( items < 2) || (( items % 2) != 0))
		croak("Invalid usage of Prima::Image::save");

	self = gimme_the_mate( ST( 0));

	sv   = ST(1);
	if ( SvROK(sv) && SvTYPE( SvRV( sv)) == SVt_PVGV)
		f = IoIFP(sv_2io(ST(1)));

	if ( f != NULL) {
		pioreq        = &ioreq;
		ioreq. handle = f;
		ioreq. read   = img_perlio_read;
		ioreq. write  = img_perlio_write;
		ioreq. seek   = img_perlio_seek;
		ioreq. tell   = img_perlio_tell;
		ioreq. flush  = img_perlio_flush;
		ioreq. error  = img_perlio_error;
		fn            = NULL;
		is_utf8       = false;
	} else {
		fn            = ( char *) SvPV_nolen( ST( 1));
		is_utf8       = prima_is_utf8_sv( ST(1) );
		pioreq        = NULL;
	}

	profile = parse_hv( ax, sp, items, mark, 2, "Image::save");
	ret = apc_img_save( self, fn, is_utf8, pioreq, profile, error);
	sv_free(( SV *) profile);
	SPAGAIN;
	SP -= items;
	XPUSHs( sv_2mortal( newSViv(( ret > 0) ? ret : -ret)));

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
