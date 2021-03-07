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

	int  access( SV* path, int mode, Bool effective = 0 );
	Bool chdir( SV* path );
	Bool chmod( SV* path, int mode);
	SV*  getcwd();
	SV*  getenv(SV * varname);
	SV*  last_error();
	SV*  local2sv(SV * text);
	Bool link( SV* oldname, SV * newname );
	Bool mkdir( SV* path, int mode = 0777);
	int  open_file( SV* path, int mode, int perms = 0666);
	Bool rename( SV* oldpath, SV * newpath );
	Bool rmdir( SV* path );
	Bool setenv(SV * varname, SV * value);
	Bool unlink( SV* path );
	SV*  sv2local(SV * text, Bool fail_if_cannot = true);
	Bool utime( double atime, double mtime, SV* path );
}
