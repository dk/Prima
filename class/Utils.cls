package Prima::Utils
{
	SV * query_drives_map( char *firstDrive = "A:");
	int  query_drive_type( char *drive) => apc_query_drive_type;
	int  get_os(void);
	int  get_gui(void);
	void beep( int flags = mbError) => apc_beep;
	char *username() => apc_get_user_name;

	int  access( SV* path, int mode, Bool effective = 0 );
	Bool chdir( SV* path );
	Bool chmod( SV* path, int mode);
	Bool closedir( SV * dh );
	SV*  getcwd();
	SV*  getenv(SV * varname);
	SV*  last_error();
	SV*  local2sv(SV * text);
	Bool link( SV* oldname, SV * newname );
	Bool mkdir( SV* path, int mode = 0777);
	SV*  open_dir( SV* path);
	int  open_file( SV* path, int mode, int perms = 0666);
	SV*  read_dir( SV* dh);
	Bool rename( SV* oldpath, SV * newpath );
	Bool rewinddir( SV* dh );
	Bool rmdir( SV* path );
	Bool seekdir(SV * dh, long position);
	Bool setenv(SV * varname, SV * value);
	long telldir(SV * dh);
	Bool unlink( SV* path );
	SV*  sv2local(SV * text, Bool fail_if_cannot = true);
	Bool utime( double atime, double mtime, SV* path );
}
