#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

static Bool
common_get_options( int * argc, char *** argv)
{
	static char * common_argv[] = {
#ifdef HAVE_OPENMP
		"openmp_threads", "sets number of openmp threads",
#endif
#ifdef WITH_FRIBIDI
		"no-fribidi", "do not use fribidi",
#endif
#ifdef WITH_LIBTHAI
		"no-libthai", "do not use libthai",
#endif
	};
	*argv = common_argv;
	*argc = sizeof( common_argv) / sizeof( char*);
	return true;
}

static Bool
common_set_option( char * option, char * value)
{
	if ( strcmp( option, "openmp_threads") == 0) {
		if ( value) {
			int n = strtol( value, &option, 10);
			if (*option)
				warn("invalid value sent to `--openmp_threads'.");
			else
				prima_omp_set_num_threads(n);
		} else
			warn("`--openmp_threads' must be given parameters.");
		return true;
	}
#ifdef WITH_FRIBIDI
	else if ( strcmp( option, "no-fribidi") == 0) {
		if ( value) warn("`--no-fribidi' option has no parameters");
		prima_guts.use_fribidi = false;
		return true;
	}
#endif
#ifdef WITH_LIBTHAI
	else if ( strcmp( option, "no-libthai") == 0) {
		if ( value) warn("`--no-libthai' option has no parameters");
		prima_guts.use_libthai = false;
		return true;
	}
#endif
	return false;
}

XS(Prima_options)
{
	dXSARGS;
	char * option, * value = NULL;
	(void)items;

	switch ( items) {
	case 0:
		{
			int i, argc1 = 0, argc2 = 0;
			char ** argv1, ** argv2;
			common_get_options( &argc1, &argv1);
			window_subsystem_get_options( &argc2, &argv2);
			EXTEND( sp, argc1 + argc2);
			for ( i = 0; i < argc1; i++)
				PUSHs( sv_2mortal( newSVpv( argv1[i], 0)));
			for ( i = 0; i < argc2; i++)
				PUSHs( sv_2mortal( newSVpv( argv2[i], 0)));
			PUTBACK;
			return;
		}
		break;
	case 2:
		value  = (SvOK( ST(1)) ? ( char*) SvPV_nolen( ST(1)) : NULL);
	case 1:
		option = ( char*) SvPV_nolen( ST(0));
		if ( !common_set_option( option, value))
			window_subsystem_set_option( option, value);
		break;
	default:
		croak("Invalid call to Prima::options");
	}
	SPAGAIN;
	XSRETURN_EMPTY;
}

#ifdef __cplusplus
}
#endif
