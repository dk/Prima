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
   CComponent( var owner)-> attach( var owner, self);
   my update_sys_handle( self, profile);
   if ( pexist( onTick))
   {
      SV ** psv  = hv_fetch( profile, "onTick", 6, 0);
      var onTick = ( SvROK( *psv) && ( SvTYPE( SvRV( *psv)) == SVt_PVCV))
         ? SvRV( newSVsv( *psv))
         : nil;
      pdelete( onTick);
   }
}

void
Timer_update_sys_handle( Handle self, HV * profile)
{
   Handle xOwner = pexist( owner) ? pget_H( owner) : var owner;
   if ( var owner) my migrate( self, xOwner);
   if (!( pexist( owner))) return;
   if ( !apc_timer_create( self, xOwner, pexist( timeout)
                           ? pget_i( timeout)
                           : my get_timeout( self)))
      croak("RTC0063: cannot create timer");
   pdelete( owner);
   if ( pexist( timeout)) pdelete( timeout);
   var owner = xOwner;
}

void
Timer_handle_event( Handle self, PEvent event)
{
#define objCheck if ( var stage > csNormal) return
   inherited handle_event ( self, event);
   switch ( event-> cmd)
   {
      case cmTimer:
         my on_tick( self);
         objCheck;
         if ( is_dmopt( dmTick))
            delegate_sub( self, "Tick", "H", self);
         objCheck;
         if ( var onTick) cv_call_perl( var mate, var onTick, "");
         break;
   }
}

void
Timer_on_tick( Handle self)
{
}

void
Timer_set( Handle self, HV * profile)
{
   if ( pexist( onTick))
   {
      SV ** psv  = hv_fetch( profile, "onTick", 6, 0);
      var onTick = ( SvROK( *psv) && ( SvTYPE( SvRV( *psv)) == SVt_PVCV))
                     ? SvRV( newSVsv( *psv))
                     : nil;
      pdelete( onTick);
   }
   inherited set( self, profile);
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
   my stop( self);
   CComponent( var owner)-> detach( var owner, self, false);
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

Bool
Timer_get_active( Handle self)
{
   return is_opt( optActive);
}


SV *
Timer_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_timer_get_handle( self));
   return newSVpv( buf, 0);
}

