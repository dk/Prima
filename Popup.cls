object Popup( AbstractMenu)
{
   method Bool   get_auto();
   method Bool   get_selected();
   method void   init( HV * profile);
   method void   set_auto( Bool autoPopup);
   method void   set_selected( Bool selected);
   method void   popup( int x, int y, int ancLeft = 0, int ancBottom = 0, int ancRight = 0, int ancTop = 0);
   import SV *   profile_default();
   c_only void   update_sys_handle( HV * profile);
}
