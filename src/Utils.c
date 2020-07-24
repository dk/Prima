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
		if ( !isUTF8_CHAR(str, end)) return false;
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

#ifdef __cplusplus
}
#endif
