object Menu ( AbstractMenu)
{
   method void   done();
   method Bool   get_selected();
   method void   set_selected( Bool selected);
   c_only void   update_sys_handle( HV * profile);
}
