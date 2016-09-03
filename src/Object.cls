object Prima::Object
{
	int  stage;
	int  protectCount;
	int  destroyRefCount;
	Handle  owner;
	void *  transient_class;
	ObjectOptions options;
	method void   cleanup();
	method void   done();
	method void   init( HV * profile);
	import void   profile_add      ( SV * profile);
	import void   profile_check_in ( SV * profile, SV * default_profile);
	import SV *   profile_default  ();
	import void   set( HV * profile);
	method void   setup();
}
