object Printer( Drawable)
{
   method void   init( HV * profile);
   method void   done();
   method Bool   begin_doc( char * docName = "");
   method void   end_doc();
   method void   abort_doc();
   method void   new_page();
   method SV *   printers();
   method Bool   begin_paint();
   method Bool   begin_paint_info();
   method void   end_paint();
   method void   end_paint_info();
   method Bool   set_printer( char * printerName);
   method char * get_default_printer()              => apc_prn_get_default;
   method SV   * get_handle();
   method char * get_printer()                      => apc_prn_get_selected;
   method Point  get_resolution()                   => apc_prn_get_resolution;
   method Point  get_size()                         => apc_prn_get_size;
   method int    get_width();
   method int    get_height();
   method Bool   setup_dialog()                     => apc_prn_setup;
}
