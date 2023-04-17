object Prima::Component( Prima::Object)
{
	void        *sysData;            # system-dependent data, defined by apc
	char        *name;
	ApiHandle   handle;              # apc handle

	property Bool      deepChildLookup;
	property Bool      eventFlag;
	property SV *      delegations;
	property SV *      name;
	property Handle    owner;

	method   UV        add_notification( char * name, SV * subroutine, Handle referer = NULL_HANDLE, int index = -1);
	method   void      attach( Handle objectHandle);
	method   Handle    bring( char * componentName, int max_depth = 0);
	method   Bool      can_event();
	method   void      cleanup();
	method   void      clear_event();
	method   void      done();
	method   void      detach( Handle objectHandle, Bool kill);
	method   void      event_error();
	c_only   Handle    first_that_component( int max_depth, void * actionProc, void * params);
	public   void      get_components();
	method   SV      * get_handle();
	public   void      get_notification( char * name, int index);
	c_only   void      handle_event ( PEvent event);
	method   void      init( HV * profile);
	method   int       is_owner( Handle objectHandle);
	c_only   Bool      message( PEvent event);
	c_only   Bool      migrate( Handle attachTo);
	import   SV *      notification_types();
	public   Bool      notify( char * format, ...);
	method   Bool      pop_event();
	method   void      post_message( SV * info1, SV * info2);
	import   SV *      profile_default();
	method   void      push_event();
	c_only   void      recreate();
	method   void      remove_notification( UV id);
	method   void      set( HV * profile);
	method   void      setup();
	public   void      set_notification( char * name, SV * subroutine);
	method   void      unlink_notifier( Handle referer);
	c_only   void      update_sys_handle( HV * profile);
	c_only   Bool      validate_owner( Handle * newOwner, HV * profile);
# internal variables
	char * evStack;
	int    evPtr;
	int    evLimit;
	PList  postList;
	PList  components;
	PList  evQueue;
	PList  refs;
	PList  events;
	PHash  eventIDs;
	int    eventIDCount;
}
