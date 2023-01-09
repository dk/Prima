#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

static char* exception_text = NULL;
static Bool  exception_blocking = 0;

void
exception_remember(char * text)
{
	if ( !exception_blocking ) croak( "%s", text );

	if ( exception_text ) {
		char * new_text = realloc(exception_text, strlen(text) + strlen(exception_text) + 1);
		if ( !new_text )
			croak("not enough memory");
		strcat( exception_text = new_text, text );
	} else {
		exception_text = duplicate_string( text );
	}
}

Bool
exception_charged(void)
{
	return exception_text != NULL;
}

Bool
exception_block(Bool block)
{
	Bool old = exception_blocking;
	exception_blocking = block;
	return old;
}

void
exception_check_raise(void)
{
	char buf[1024];
	if ( !exception_text ) return;
	strlcpy( buf, exception_text, 1024 );
	free( exception_text );
	exception_text = NULL;
	croak("%s", buf);
}

void
exception_dispatch_pending_signals( void )
{
	if (PL_sig_pending)
		Perl_despatch_signals(aTHX);
}


#ifdef __cplusplus
}
#endif
