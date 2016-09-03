object Prima::Menu ( Prima::AbstractMenu)
{
	property Bool selected;
	c_only void   update_sys_handle( HV * profile);
	c_only Bool   validate_owner( Handle * newOwner, HV * profile);
}
