object Prima::File( Prima::Component)
{
	int   eventMask;
	int   eventMask2;
	int   userMask;
	int   fd;
	SV *  file;

	property SV *      file;
	property int       fd;
	property int       mask;

	method   UV        add_notification( char * name, SV * subroutine, Handle referer = NULL_HANDLE, int index = -1);
	method   void      init( HV * profile);
	method   void      cleanup();
	method   SV      * get_handle();
	c_only   void      handle_event( PEvent event);
	method   Bool      is_active( Bool autoDetach = false);
	import   SV *      notification_types();
	method   void      remove_notification( UV id);
}
