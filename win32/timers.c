#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Window.h"
#include "Application.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle
#define OWNER DHANDLE(prima_guts.application)

static int
add_timer( Handle timerObject)
{
	PItemRegRec pTime;
	int i;
	if ( timerObject == NULL_HANDLE) {
		apcErr( errInvObject);
		return 0;
	}
	if ( timeDefsCount >= TID_USERMAX - 1 )
		return 0;

	if ( timeDefs) for ( i = 0; i < timeDefsCount; i++)
	if ( timeDefs[ i]. item == NULL)
	{
		timeDefs[ i]. item = ( void*) timerObject;
		return i + 1;
	}

	if ( !(pTime = ( PItemRegRec) malloc (( timeDefsCount + 1) * sizeof( ItemRegRec))))
		return 0;

	if ( timeDefs) {
		memcpy( pTime, timeDefs, timeDefsCount * sizeof( ItemRegRec));
		free( timeDefs);
	}
	timeDefs = pTime;
	pTime += timeDefsCount++;
	pTime-> item = ( void*) timerObject;
	return timeDefsCount;
}

static void
remove_timer( Handle timerObject)
{
	int i;
	PItemRegRec list = timeDefs;
	for ( i = 0; i < timeDefsCount; i++)
	{
		if (( Handle)( list-> item) == timerObject)
		{
			list-> item = NULL;
			break;
		}
		list++;
	}
}

Bool
apc_timer_create( Handle self)
{
	objCheck false;

	if ( !( var handle = add_timer( self))) return false;
	return true;
}

Bool
apc_timer_destroy( Handle self)
{
	objCheck false;
	if ( prima_guts.application && is_opt( optActive) && var handle ) {
		if ( !KillTimer( OWNER, var handle)) apiErr;
	}
	remove_timer( self);
	return true;
}

int
apc_timer_get_timeout( Handle self)
{
	objCheck 0;
	return sys s. timer. timeout;
}

Bool
apc_timer_set_timeout( Handle self, int timeout)
{
	objCheck false;
	if ( !prima_guts.application ) return false;
	if ( is_opt( optActive)) {
		if ( !SetTimer( OWNER, var handle, timeout, NULL)) {
			opt_clear( optActive);
			apiErr;
			return false;
		}
	}
	sys s. timer. timeout = timeout;
	return true;
}

Bool
apc_timer_start( Handle self)
{
	objCheck false;
	if ( !prima_guts.application ) return false;
	if ( !SetTimer( OWNER, var handle, sys s. timer. timeout, NULL))
		apiErrRet;
	return true;
}

Bool
apc_timer_stop( Handle self)
{
	objCheck false;
	if ( !prima_guts.application ) return false;
	KillTimer( OWNER, var handle);
	return true;
}

ApiHandle
apc_timer_get_handle( Handle self)
{
	objCheck 0;
	return ( ApiHandle) var handle;
}

#ifdef __cplusplus
}
#endif
