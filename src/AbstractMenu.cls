object Prima::AbstractMenu ( Prima::Component)
{
	PMenuItemReg  tree;
	property Bool   selected;
	property SV   * accel  ( char * varName);
	property SV   * action ( char * varName);
	property Bool   autoToggle( char * varName);
	property Bool   checked( char * varName);
	property Handle icon   ( char * varName);
	property int    group  ( char * varName);
	property SV   * options( char * varName);
	property Bool   enabled( char * varName);
	property Handle image  ( char * varName);
	property SV   * key    ( char * varName);
	property SV   * submenu( char * varName);
	property SV   * text   ( char * varName);

	method void   cleanup();
	method void   done();
	method Bool   execute( char * varName);
	method SV   * find_item_by_key( int key );
	c_only void * first_that( void * actionProc, void * params, Bool useDisabled);
	method SV   * get_handle();
	method SV   * get_item( char * varName, Bool fullTree = 0);
	method SV   * get_items( char * varName, Bool fullTree = 1);
	method SV   * get_children( char * varName);
	method Bool   has_item( char * varName);
	method void   init( HV * profile);
	method void   insert( SV * menuItems, char * rootName, int index);
	method Bool   is_custom( char * varName);
	method Bool   is_submenu( char * varName);
	method Bool   is_separator( char * varName);
	c_only void   handle_event ( PEvent event);
	import SV *   notification_types();
	import SV *   profile_default ();
	method void   remove( char * varName);
	method void   set( HV * profile);
	method void   set_items( SV * menuItems);
	method void   set_variable ( char * varName, SV * newName);
	static int    translate_accel( Handle self, char * accel);
	static int    translate_key( Handle self, int code, int key, int mod);
	static int    translate_shortcut( Handle self, char * key);
	c_only Bool   validate_owner( Handle * newOwner, HV * profile);

#	private
	Bool   anchored;
	Bool   system;
	unsigned int autoEnum;
	c_only void   dispose_menu( void * menu);
	c_only void * new_menu( SV * sv, int level, void * avt);
	c_only Bool   sub_call( PMenuItemReg m);
	c_only Bool   sub_call_id ( int sysId);
	c_only Bool   sub_call_key ( int key);
	c_only char * make_var_context( PMenuItemReg m, char * buffer);
	c_only char * make_id_context( int id, char * buffer);
}
