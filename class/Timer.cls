object Prima::Timer( Prima::Component)
{
	property int timeout;
	method void   cleanup();
	method void   done();
	method Bool   get_active();
	method SV *   get_handle();
	c_only void   handle_event ( PEvent event);
	method void   init( HV * profile);
	import SV *   notification_types();
	import SV *   profile_default  ();
	method Bool   start();
	method void   stop();
}
