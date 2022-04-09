#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

FILE*
prima_open_file( const char *text, Bool is_utf8, const char * mode)
{
	int fd, o, m;
	const char * omode = mode;
	char *cwd = NULL;
	FILE * ret;

	(void)cwd;

	switch ( *mode++ ) {
	case 'r':
		m = O_RDONLY;
		o = 0;
		break;
	case 'w':
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;
	case 'a':
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	if ( *mode == 'b' ) {
		mode++;
#ifdef O_BINARY
		o |= O_BINARY;
#endif
	}
	if ( *mode == '+' ) m = O_RDWR;

#if defined(PERL_IMPLICIT_SYS)
	if (
		(*text != '/') &&
		!(isalpha(text[0]) && text[1] == ':')
	) {
		cwd = apc_fs_getcwd();
		apc_fs_chdir(PerlEnv_get_childdir(), false);
	}
#endif

	if (( fd = apc_fs_open_file( text, is_utf8, m | o, 0666)) < 0) {
		free(cwd);
		return NULL;
	}

#if defined(PERL_IMPLICIT_SYS)
	if (cwd) {
		apc_fs_chdir(cwd, true);
		free(cwd);
	}
#endif

	if (!( ret = fdopen( fd, omode ))) {
		close(fd);
		return NULL;
	}

	if ( o & O_APPEND )
		fseek( ret, 0, SEEK_END);
	else
		fseek( ret, 0, SEEK_SET);

	return ret;
}

#ifdef __cplusplus
}
#endif
