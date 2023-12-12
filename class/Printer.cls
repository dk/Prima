object Prima::Printer( Prima::Drawable)
{
	property char * printer;
	property Point  resolution;
	property Point  size;

	method void   abort_doc();
	method Bool   begin_doc( char * docName = "");
	method Bool   begin_paint();
	method Bool   begin_paint_info();
	method void   done();
	method Bool   end_doc();
	method void   end_paint();
	method void   end_paint_info();
	method SV*    fonts( char * name = "", char * encoding = "");
	method SV*    font_encodings();
	method void   init( HV * profile);
	method Bool   new_page();
	public void   options();
	method SV *   printers();
	method Bool   setup_dialog()                     => apc_prn_setup;
	c_only Bool   validate_owner( Handle * newOwner, HV * profile);

	method char * get_default_printer()              => apc_prn_get_default;
	method SV   * get_handle();
}
