object Timer( Component)
{
   method void   done();
   method Bool   get_active();
   method int    get_timeout()               => apc_timer_get_timeout;
   method SV *   get_handle();
   c_only void   handle_event ( PEvent event);
   method void   init( HV * profile);
   import SV *   profile_default  ();
   method void   set( HV * profile);
   method void   set_timeout( int timeout)   => apc_timer_set_timeout;
   method Bool   start();
   method void   stop();
   method void   update_delegator();
   c_only void   update_sys_handle( HV * profile);
   event  void   on_tick();
   SV *   onTick;
}
