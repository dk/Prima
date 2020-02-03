object Prima::Clipboard( Prima::Component)
{
	int openCount;

	method void   clear();
	method void   close();
	method void   deregister_format( char * format);
	method void   done();
	method SV *   fetch( char * format);
	method Bool   format_exists( char * format);
	method SV *   get_handle();
	public void   get_formats( Bool include_unregistered = 0 );
	public void   get_registered_formats();
	public void   get_standard_clipboards();
	method void   init( HV * profile);
	method Bool   is_dnd() => apc_clipboard_is_dnd;
	method Bool   open();
	method Bool   register_format( char * format);
	method Bool   store( char * format, SV * data);
	c_only Bool   validate_owner( Handle * newOwner, HV * profile);
}
