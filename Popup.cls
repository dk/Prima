object Popup( AbstractMenu)
{
   method void   done();
   method Bool   get_auto();
   method Bool   get_selected();
   method void   init( HV * profile);
   method void   set_auto( Bool autoPopup);
   method void   set_selected( Bool selected);
   method void   popup( int x, int y);
   import SV *   profile_default ();
   c_only void   update_sys_handle( HV * profile);
}
