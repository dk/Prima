#include "apricot.h"
#include "Timer.h"
#include "Timer.inc"

#undef  my
#define inherited CComponent->
#define my  ((( PTimer) self)-> self)->
#define var (( PTimer) self)->

void
Timer_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   ((( PComponent) var owner)-> self)-> attach( var owner, self);
   my update_sys_handle( self, profile);
   if ( pexist( onTick))
   {
      SV ** psv  = hv_fetch( profile, "onTick", 6, 0);
      var onTick = ( SvROK( *psv) && ( SvTYPE( SvRV( *psv)) == SVt_PVCV)) ? SvRV( newSVsv( *psv)) : nil;
      pdelete( onTick);
   }
}

void
Timer_update_sys_handle( Handle self, HV * profile)
{
   Handle xOwner = pexist( owner) ? pget_H( owner) : var owner;
   if ( var owner) my migrate( self, xOwner);
   if (!(
       pexist( owner) ||
       pexist( timeout)
    )) return;
   if ( !apc_timer_create( self, xOwner, pexist( timeout) ? pget_i( timeout) : my get_timeout( self)))
      croak("RTC0063: cannot create timer");
   pdelete( owner);
   pdelete( timeout);
   var owner = xOwner;
}

void
Timer_handle_event( Handle self, PEvent event)
{
   inherited handle_event ( self, event);
   switch ( event-> cmd)
   {
      case cmTimer:
         my on_tick( self);
         if ( is_dmopt( dmTick))
            delegate_sub( self, "Tick", "H", self);
         if ( var onTick) cv_call_perl( var mate, var onTick, "");
         break;
   }
}

void Timer_on_tick( Handle self){}

void
Timer_set( Handle self, HV * profile)
{
   if ( pexist( onTick))
   {
      SV ** psv  = hv_fetch( profile, "onTick", 6, 0);
      var onTick = ( SvROK( *psv) && ( SvTYPE( SvRV( *psv)) == SVt_PVCV)) ? SvRV( newSVsv( *psv)) : nil;
      pdelete( onTick);
   }
   inherited set( self, profile);
}

void
Timer_done( Handle self)
{
   ((( PComponent) var owner)-> self)-> detach( var owner, self, false);
   apc_timer_destroy( self);
   inherited done( self);
}

void
Timer_update_delegator( Handle self)
{
   HV * profile;
   inherited update_delegator( self);
   if ( var delegateTo == nilHandle) return;
   profile = my get_delegators( self);
   if ( pexist( Tick)) dmopt_set( dmTick);
}


