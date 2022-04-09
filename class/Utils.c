#include "apricot.h"
#include "Utils.h"
#include <Utils.inc>

#ifdef __cplusplus
extern "C" {
#endif


SV *Utils_query_drives_map( char *firstDrive)
{
	char map[ 256];
	apc_query_drives_map( firstDrive, map, sizeof( map));
	return newSVpv( map, 0);
}

int
Utils_get_os(void)
{
	return apc_application_get_os_info( NULL, 0, NULL, 0, NULL, 0, NULL, 0);
}

int
Utils_get_gui(void)
{
	return apc_application_get_gui_info( NULL, 0, NULL, 0);
}

long Utils_ceil( double x)
{
	return ceil( x);
}

long Utils_floor( double x)
{
	return floor( x);
}

static Bool
is_valid_utf8( unsigned char * str )
{
	int len = 0, hi8 = 0;
	unsigned char * c = str;
	while (*c) {
		len++;
		if ( *c > 0x7f ) hi8 = 1;
		c++;
	}
	if ( !hi8 ) return false;
#if PERL_PATCHLEVEL >= 22
	while ( str < c ) {
		unsigned char * end = utf8_hop( str, 1 );
		if ( end > c ) return false;
		if ( !isUTF8_CHAR(str, end))
			return false;
		str = end;
	}
#endif
	return true;
}

XS(Utils_getdir_FROMPERL) {
	dXSARGS;
	Bool wantarray = ( GIMME_V == G_ARRAY);
	char *dirname;
	PList dirlist;
	int i;

	if ( items >= 2) {
		croak( "invalid usage of Prima::Utils::getdir");
	}
	dirname = SvPV_nolen( ST( 0));
	dirlist = apc_getdir( dirname, prima_is_utf8_sv(ST(0)));
	SPAGAIN;
	SP -= items;
	if ( wantarray) {
		if ( dirlist) {
			EXTEND( sp, dirlist-> count);
			for ( i = 0; i < dirlist-> count; i++) {
				char * entry = ( char *)dirlist-> items[i];
				SV * sv      = newSVpv(entry, 0);
				if (is_valid_utf8((unsigned char*) entry))
					SvUTF8_on(sv);
				PUSHs( sv_2mortal(sv));
				free(( char *)dirlist-> items[i]);
			}
			plist_destroy( dirlist);
		}
	} else {
		if ( dirlist) {
			XPUSHs( sv_2mortal( newSViv( dirlist-> count / 2)));
			for ( i = 0; i < dirlist-> count; i++) {
				free(( char *)dirlist-> items[i]);
			}
			plist_destroy( dirlist);
		} else {
			XPUSHs( &PL_sv_undef);
		}
	}
	PUTBACK;
	return;
}

int
Utils_access( SV* path, int mode, Bool effective )
{
	return apc_fs_access( SvPV_nolen(path), prima_is_utf8_sv(path), mode, effective);
}

Bool
Utils_chdir( SV* path )
{
	return apc_fs_chdir( SvPV_nolen(path), prima_is_utf8_sv(path));
}

Bool
Utils_chmod( SV* path, int mode)
{
	return apc_fs_chmod( SvPV_nolen(path), prima_is_utf8_sv(path), mode);
}

static PDirHandleRec
get_dh(const char * method, SV * sv)
{
	PDirHandleRec d;
	if ( !SvROK(sv) || SvTYPE( SvRV( sv)) != SVt_PVMG)
		goto WARN;
	if ( !sv_isa( sv, "Prima::Utils::DIRHANDLE" ))
		goto WARN;
	d = (PDirHandleRec) prima_array_get_storage(SvRV(sv));
	if (!d-> is_active) {
		errno = EBADF;
		return false;
	}
	return d;

WARN:
	warn("Prima::Utils::%s: invalid dirhandle", method);
	errno = EBADF;
	return false;
}

Bool
Utils_closedir(SV * dh)
{
	PDirHandleRec d;
	if (( d = get_dh("closedir", dh)) == NULL )
		return false;
	d-> is_active = false;
	return apc_fs_closedir(d);
}

SV*
Utils_getcwd()
{
	SV * ret;
	char *cwd;

	if (( cwd = apc_fs_getcwd()) == NULL )
		return NULL_SV;
	ret = newSVpv( cwd, 0 );
	if ( is_valid_utf8((unsigned char*) cwd))
		SvUTF8_on(ret);
	free(cwd);
	return ret;
}

SV*
Utils_getenv(SV * varname)
{
	SV * ret;
	char *val;
	Bool is_utf8, do_free = false;

	is_utf8 = prima_is_utf8_sv(varname);
	if (( val = apc_fs_getenv(SvPV_nolen(varname), is_utf8, &do_free)) == NULL )
		return NULL_SV;
	ret = newSVpv( val, 0 );
	if ( is_valid_utf8((unsigned char*) val))
		SvUTF8_on(ret);
	if ( do_free ) free(val);
	return ret;
}

SV *
Utils_last_error()
{
	SV * ret = NULL_SV;
	char * p = apc_last_error();
	if ( p ) {
		ret = newSVpv( p, 0);
		free(p);
	}
	return ret;
}

Bool
Utils_link( SV* oldname, SV * newname )
{
	return apc_fs_link(
		SvPV_nolen(oldname), prima_is_utf8_sv(oldname),
		SvPV_nolen(newname), prima_is_utf8_sv(newname)
	);
}

SV *
Utils_local2sv(SV * text)
{
	SV * ret;
	char * buf, *src;
	STRLEN xlen;
	int len;
	if ( prima_is_utf8_sv(text) )
		return newSVsv( text );
	src = SvPV(text, xlen);
	len = xlen;
	if ( !( buf = apc_fs_from_local(src, &len)))
		return NULL_SV;
	if ( buf == src ) {
		ret = newSVsv( text );
		if ( is_valid_utf8((unsigned char*) src))
			SvUTF8_on(ret);
		return ret;
	}

	ret = newSVpv( buf, len );
	if ( is_valid_utf8((unsigned char*) buf))
		SvUTF8_on(ret);
	free(buf);

	return ret;
}


Bool
Utils_mkdir( SV* path, int mode)
{
	return apc_fs_mkdir( SvPV_nolen(path), prima_is_utf8_sv(path), mode);
}

SV *
Utils_open_dir(SV * path)
{
	SV * ret = NULL_SV;
	PDirHandleRec dh;
	SV * dhsv;

	if (( dhsv = prima_array_new(sizeof(DirHandleRec))) == NULL) {
		errno = ENOMEM;
		return NULL_SV;
	}
	if (( dh = (PDirHandleRec) prima_array_get_storage(dhsv)) == NULL) {
		errno = ENOMEM;
		return NULL_SV;
	}
	bzero(dh, sizeof(DirHandleRec));
	dh-> is_utf8 = prima_is_utf8_sv(path);
	if ( !apc_fs_opendir( SvPV_nolen(path), dh)) {
		sv_free(dhsv);
		return NULL_SV;
	}
	dh-> is_active = true;

	ret = newRV_noinc(dhsv);
	sv_bless(ret, gv_stashpv("Prima::Utils::DIRHANDLE", GV_ADD));


	return ret;
}

int
Utils_open_file( SV* path, int mode, int perms)
{
	return apc_fs_open_file( SvPV_nolen(path), prima_is_utf8_sv(path), mode, perms);
}

SV*
Utils_read_dir(SV * dh)
{
	PDirHandleRec d;
	char buf[PATH_MAX_UTF8];
	SV * ret;
	if (( d = get_dh("read_dir", dh)) == NULL ) {
		errno = EBADF;
		warn("Prima::Utils::read_dir: invalid dirhandle");
		return NULL_SV;
	}
	if (!d-> is_active) {
		errno = EBADF;
		return NULL_SV;
	}

	if ( !apc_fs_readdir(d, buf)) return NULL_SV;

	ret = newSVpv(buf, 0);
	if (is_valid_utf8((unsigned char*) buf))
		SvUTF8_on(ret);

	return ret;
}

Bool
Utils_rename( SV* oldpath, SV * newpath )
{
	return apc_fs_rename(
		SvPV_nolen(oldpath), prima_is_utf8_sv(oldpath),
		SvPV_nolen(newpath), prima_is_utf8_sv(newpath)
	);
}

Bool
Utils_rewinddir( SV * dh )
{
	PDirHandleRec d;
	if (( d = get_dh("rewinddir", dh)) == NULL )
		return false;
	return apc_fs_rewinddir(d);
}

Bool
Utils_rmdir( SV* path )
{
	return apc_fs_rmdir( SvPV_nolen(path), prima_is_utf8_sv(path));
}

Bool
Utils_seekdir( SV * dh, long position )
{
	PDirHandleRec d;
	if (( d = get_dh("seekdir", dh)) == NULL )
		return false;
	return apc_fs_seekdir(d, position);
}

Bool
Utils_setenv(SV * varname, SV * value)
{
	return apc_fs_setenv(
		SvPV_nolen(varname), prima_is_utf8_sv(varname),
		(SvTYPE(value) != SVt_NULL) ? SvPV_nolen(value) : NULL,
		(SvTYPE(value) != SVt_NULL) ? prima_is_utf8_sv(value) : false
	);
}

XS(Utils_stat_FROMPERL) {
	dXSARGS;
	char *name;
	StatRec stats;
	int ok;
	Bool link = false;
	U8 gimme;

	if ( items > 2)
		croak( "invalid usage of Prima::Utils::stat");
	if ( items > 1 )
		link = SvBOOL(ST(1));

	name = SvPV_nolen( ST( 0));
	ok = apc_fs_stat( name, prima_is_utf8_sv(ST(0)), link, &stats);
	SPAGAIN;
	SP -= items;
	gimme = GIMME_V;
	if ( gimme != G_ARRAY ) {
		if ( gimme != G_VOID )
			XPUSHs(newSViv(ok));
	} else if ( ok) {
		EXTEND( sp, 11 );
		PUSHs( sv_2mortal(newSVuv( stats. dev    )));
		PUSHs( sv_2mortal(newSVuv( stats. ino    )));
		PUSHs( sv_2mortal(newSVuv( stats. mode   )));
		PUSHs( sv_2mortal(newSVuv( stats. nlink  )));
		PUSHs( sv_2mortal(newSVuv( stats. uid    )));
		PUSHs( sv_2mortal(newSVuv( stats. gid    )));
		PUSHs( sv_2mortal(newSVuv( stats. rdev   )));
		PUSHs( sv_2mortal(newSVuv( stats. size   )));
		PUSHs( sv_2mortal(newSVnv( stats. atim   )));
		PUSHs( sv_2mortal(newSVnv( stats. mtim   )));
		PUSHs( sv_2mortal(newSVnv( stats. ctim   )));
		if (stats. blksize >= 0 )
			XPUSHs( sv_2mortal(newSVuv( stats. blksize)));
		if (stats. blocks  >= 0 )
			XPUSHs( sv_2mortal(newSVuv( stats. blocks )));
	}
	PUTBACK;
	return;
}

static int
utf8len( const char * utf8, int maxlen)
{
	int ulen = 0;
	while ( maxlen > 0 ) {
		const char *u = (char*) utf8_hop(( U8*) utf8, 1);
		ulen++;
		maxlen -= u - utf8;
		utf8 = u;
	}
	return ulen;
}

long
Utils_telldir( SV * dh )
{
	PDirHandleRec d;
	if (( d = get_dh("telldir", dh)) == NULL )
		return false;
	return apc_fs_telldir(d);
}

SV *
Utils_sv2local(SV * text, Bool fail_if_cannot)
{
	SV * ret;
	char * buf, * src;
	STRLEN xlen;
	int len;
	if ( !prima_is_utf8_sv(text) )
		return newSVsv( text );
	src = SvPV(text, xlen);
	len = utf8len( src, xlen );
	if ( !( buf = apc_fs_to_local(src, fail_if_cannot, &len)))
		return NULL_SV;
	if ( buf == src ) {
		ret = newSVsv( text );
		SvUTF8_off(ret);
		return ret;
	}
	ret = newSVpv( buf, len );
	free(buf);

	return ret;
}


Bool
Utils_unlink( SV* path )
{
	return apc_fs_unlink( SvPV_nolen(path), prima_is_utf8_sv(path));
}

Bool
Utils_utime( double atime, double mtime, SV* path )
{
	return apc_fs_utime( atime, mtime, SvPV_nolen(path), prima_is_utf8_sv(path));
}

#ifdef __cplusplus
}
#endif
