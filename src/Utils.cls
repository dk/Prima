package Prima::Utils
{
	SV * query_drives_map( char *firstDrive = "A:");
	int  query_drive_type( char *drive) => apc_query_drive_type;
	int  get_os(void);
	int  get_gui(void);
	void beep( int flags = mbError) => apc_beep;
	long ceil( double x);
	long floor( double x);
	void sound( int freq = 2000, int dur = 100) => apc_beep_tone;
	char *username() => apc_get_user_name;
	char *last_error() => apc_last_error;
}
