#include "unix/guts.h"

static void
inactivate_timer( PTimerSysData sys)
{
	if ( sys-> older || sys-> younger || guts. oldest == sys) {
		if ( sys-> older) {
			sys-> older-> younger = sys-> younger;
		} else {
			guts. oldest = sys-> younger;
		}
		if ( sys-> younger)
			sys-> younger-> older = sys-> older;
	}
	sys-> older = NULL;
	sys-> younger = NULL;
}

static void
fetch_sys_timer( Handle self, PTimerSysData *s, Bool *real_timer)
{
	if ( self == 0) {
		*s = NULL;
		*real_timer = false;
	} else if ( self >= FIRST_SYS_TIMER && self <= LAST_SYS_TIMER) {
		*s = &guts. sys_timers[ self - FIRST_SYS_TIMER];
		*real_timer = false;
	} else {
		*s = ((PTimerSysData)(PComponent((self))-> sysData));
		*real_timer = true;
	}
}

#define ENTERTIMER \
		PTimerSysData sys; \
		Bool real; \
		\
		fetch_sys_timer( self, &sys, &real)

Bool
apc_timer_create( Handle self)
{
	ENTERTIMER;

	sys-> type.timer = true;
	inactivate_timer( sys);
	sys-> who = self;
	if (real)
		apc_component_fullname_changed_notify( self);
	return true;
}

Bool
apc_timer_destroy( Handle self)
{
	ENTERTIMER;

	inactivate_timer( sys);
	sys-> timeout = 0;
	if (real) opt_clear( optActive);
	return true;
}

int
apc_timer_get_timeout( Handle self)
{
	ENTERTIMER;
	return sys-> timeout;
}

Bool
apc_timer_set_timeout( Handle self, int timeout)
{
	ENTERTIMER;

	sys-> timeout = timeout;
	if ( !real || is_opt( optActive))
		apc_timer_start( self);
	return real ? (prima_guts.application != NULL_HANDLE) : true;
}

Bool
apc_timer_start( Handle self)
{
	PTimerSysData before;
	ENTERTIMER;

	inactivate_timer( sys);
	if ( real && !prima_guts.application ) return false;

	gettimeofday( &sys-> when, NULL);
	sys-> when. tv_sec += sys-> timeout / 1000;
	sys-> when. tv_usec += (sys-> timeout % 1000) * 1000;

	before = guts. oldest;
	if ( before) {
		while ( before-> when. tv_sec < sys-> when. tv_sec ||
				( before-> when. tv_sec == sys-> when. tv_sec &&
					before-> when. tv_usec <= sys-> when. tv_usec)) {
			if ( !before-> younger) {
				before-> younger = sys;
				sys-> older = before;
				before = NULL;
				break;
			}
			before = before-> younger;
		}
		if ( before) {
			if ( before-> older) {
				sys-> older = before-> older;
				before-> older-> younger = sys;
			} else {
				guts. oldest = sys;
			}
			sys-> younger = before;
			before-> older = sys;
		}
	} else {
		guts. oldest = sys;
	}

	if ( real ) opt_set( optActive);
	return true;
}

Bool
apc_timer_stop( Handle self)
{
	ENTERTIMER;
	inactivate_timer( sys);
	if ( real) opt_clear( optActive);
	return true;
}


ApiHandle
apc_timer_get_handle( Handle self)
{
	ENTERTIMER;
	return (ApiHandle) sys;
}

