#include "apricot.h"
#include "Timer.h"
#include <Timer.inc>
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CComponent->
#define my  ((( PTimer) self)-> self)
#define var (( PTimer) self)

void
Timer_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	if ( !apc_timer_create( self))
		croak("cannot create timer");
	my-> set_timeout( self, pget_i( timeout));
	CORE_INIT_TRANSIENT(Timer);
}

void
Timer_handle_event( Handle self, PEvent event)
{
	inherited handle_event ( self, event);
	if ( event-> cmd == cmTimer && is_opt( optActive) ) my-> notify( self, "<s", "Tick");
}

Bool
Timer_start( Handle self)
{
	if ( is_opt( optActive)) return true;
	opt_assign( optActive, apc_timer_start( self));
	return is_opt( optActive);
}

void
Timer_stop( Handle self)
{
	if ( !is_opt( optActive)) return;
	apc_timer_stop( self);
	opt_clear( optActive);
}

void
Timer_done( Handle self)
{
	apc_timer_destroy( self);
	inherited done( self);
}

void
Timer_cleanup( Handle self)
{
	my-> stop( self);
	inherited cleanup( self);
}

Bool
Timer_get_active( Handle self)
{
	return is_opt( optActive);
}


SV *
Timer_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_timer_get_handle( self));
	return newSVpv( buf, 0);
}

int
Timer_timeout( Handle self, Bool set, int timeout)
{
	if ( !set)
		return apc_timer_get_timeout( self);
	return ( int) apc_timer_set_timeout( self, timeout);
}

#ifdef __cplusplus
}
#endif
