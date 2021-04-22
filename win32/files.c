#ifdef __CYGWIN__

#define SOCKET int
#define _get_osfhandle(a) a

#include <pthread.h>

#else

#include <winsock.h>

void __inline my_fd_zero( fd_set* f)           { FD_ZERO( f); }

#endif

typedef fd_set type_fd_set;
void __inline std_fd_set( int fd, fd_set * f) { FD_SET(fd, f); }

#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Component.h"
#include "File.h"

void __inline my_fd_set( HANDLE fd, type_fd_set * f) { std_fd_set( PTR2UV(fd), f); }

#define var (( PFile) self)->
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __CYGWIN__

#undef  select
#undef  fd_set
#undef  FD_ZERO
#define FD_ZERO my_fd_zero
#undef  FD_SET
#define FD_SET my_fd_set

#endif

static Bool            socketThreadStarted = false;
static Bool            socketSetChanged    = false;
static struct timeval  socketTimeout       = {0, 200000};
static char            socketErrBuf [ 256];

static fd_set socketSet1[3];
static fd_set socketSet2[3];
static int    socketCommands[3] = { feRead, feWrite, feException};

void
#ifdef __CYGWIN__
*
#endif
socket_select( void *dummy)
{
	int count;
	while ( !appDead) {
		if ( socketSetChanged) {
			// updating  handles
			int i;
			if ( WaitForSingleObject( guts. socketMutex, INFINITE) != WAIT_OBJECT_0) {
				strcpy( socketErrBuf, "Failed to obtain socket mutex ownership for thread #2");
				PostThreadMessage( guts. mainThreadId, WM_CROAK, 1, ( LPARAM) &socketErrBuf);
				break;
			}
			for ( i = 0; i < 3; i++)
				memcpy( socketSet1+i, socketSet2+i, sizeof( fd_set));
			socketSetChanged = false;
			ReleaseMutex( guts. socketMutex);
		}

		// calling select()
#ifndef __CYGWIN__
		count = socketSet1[0]. fd_count + socketSet1[1]. fd_count + socketSet1[2]. fd_count;
#else
		count = 0;
		{
			int i,j;
			for ( i = 0; i < FD_SETSIZE; i++)
				for ( j = 0; j < 3; i++)
					if ( FD_ISSET( i, socketSet1+j)) {
						count++;
						goto END;
					}
END:;
		}
#endif
		if ( count > 0) {
			int i, j, result = select( FD_SETSIZE-1, &socketSet1[0], &socketSet1[1], &socketSet1[2], &socketTimeout);
			socketSetChanged = true;
			if ( result == 0) continue;
			if ( result < 0) {
				int err;
#ifndef __CYGWIN__
				if (( err = WSAGetLastError()) == WSAENOTSOCK)
#else
				if (( err = errno) == EBADF)
#endif
				{
					// possibly some socket was closed
					guts. socketPostSync = 1;
					PostThreadMessage( guts. mainThreadId, WM_SOCKET_REHASH, 0, 0);
					while( guts. socketPostSync) Sleep(1);
				} else {
					// some error
					char * msg;
#ifndef __CYGWIN__
					msg = err_msg( err, socketErrBuf);
#else
					strncpy( msg = socketErrBuf, strerror(err), 255);
					socketErrBuf[255] = 0;
#endif
					PostThreadMessage( guts. mainThreadId, WM_CROAK, 0, (LPARAM) msg);
				}
				continue;
			}
			// posting select() results
			for ( j = 0; j < 3; j++)
#ifndef __CYGWIN__
				for ( i = 0; i < socketSet1[j]. fd_count; i++) {
#else
				for ( i = 0; i < FD_SETSIZE; i++) {
					if ( !FD_ISSET( i, socketSet1 + j)) continue;
#endif
					guts. socketPostSync = 1;
					PostThreadMessage( guts. mainThreadId, WM_SOCKET, socketCommands[j],
#ifndef __CYGWIN__
						( LPARAM) socketSet1[j]. fd_array[i]
#else
						( LPARAM) i
#endif
					);
					while( guts. socketPostSync) Sleep(1);
				}
		} else
			// nothing to 'select', sleeping
			Sleep( socketTimeout. tv_sec * 1000 + socketTimeout. tv_usec / 1000);
	}

	// if somehow failed, making restart possible
	socketThreadStarted = false;
#ifdef __CYGWIN__
	return NULL;
#endif
}


static void
reset_sockets( void)
{
	int i;

	// enter section
	if ( socketThreadStarted) {
		if ( WaitForSingleObject( guts. socketMutex, INFINITE) != WAIT_OBJECT_0)
			croak("Failed to obtain socket mutex ownership for thread #1");
	}

	// copying handles
	for ( i = 0; i < 3; i++)
		FD_ZERO( &socketSet2[i]);

	for ( i = 0; i < guts. sockets. count; i++) {
		Handle self = guts. sockets. items[i];
		if ( var eventMask & feRead)
			FD_SET( sys s. file. object, &socketSet2[0]);
		if ( var eventMask & feWrite)
			FD_SET( sys s. file. object, &socketSet2[1]);
		if ( var eventMask & feException)
			FD_SET( sys s. file. object, &socketSet2[2]);
	}

	socketSetChanged = true;

	// leave section and start the thread, if needed
	if ( !socketThreadStarted) {
		if ( !( guts. socketMutex = CreateMutex( NULL, FALSE, NULL))) {
			apiErr;
			croak("Failed to create socket mutex object");
		}
#ifndef __CYGWIN__
		guts. socketThread = ( HANDLE) _beginthread( socket_select, 40960, NULL);
#else
		pthread_create(( pthread_t*) &guts. socketThread, 0, socket_select, NULL);
#endif
		socketThreadStarted = true;
	} else
		ReleaseMutex( guts. socketMutex);
}

void
socket_rehash( void)
{
	int i;
	for ( i = 0; i < guts. sockets. count; i++) {
		Handle self = guts. sockets. items[i];
		CFile( self)-> is_active( self, true);
	}
}


Bool
apc_file_attach( Handle self)
{
	int fhtype;
	objCheck false;

	if ( PFile(self)->fd > FD_SETSIZE ) return false;

	if ( guts. socket_version == 0) {
		int  _data, _sz = sizeof( int);
		(void)_data;
		(void)_sz;
#ifdef __CYGWIN__
		_sz = htons(80);
		guts. socket_version = 2;
#else
#ifdef PERL_OBJECT     // init perl socket library, if any
		PL_piSock-> Htons( 80);
#else
		win32_htons(80);
#endif
		if ( getsockopt(( SOCKET) INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char*)&_data, &_sz) != 0)
			guts. socket_version = -1; // no sockets available
		else
#if PERL_PATCHLEVEL < 8
			guts. socket_version = ( _data == SO_SYNCHRONOUS_NONALERT) ? 1 : 2;
#else
			guts. socket_version = 1;
#endif

#endif
	}

	if ( SOCKETS_NONE)
		return false;

	sys s. file. object = SOCKETS_AS_HANDLES ?
		(( SOCKETHANDLE) _get_osfhandle( var fd)) :
		((INT2PTR(SOCKETHANDLE, var fd)));

	{
		int  _data, _sz = sizeof( int);
		int result =
#ifndef __CYGWIN__
			SOCKETS_AS_HANDLES ?
			WSAAsyncSelect((SOCKET) sys s. file. object, (HWND) NULL, 0, 0) :
#endif
			getsockopt(( SOCKET) sys s. file. object, SOL_SOCKET, SO_TYPE, (char*)&_data, &_sz);
		if ( result != 0)
#ifndef __CYGWIN__
			fhtype = ( WSAGetLastError() == WSAENOTSOCK) ? FHT_OTHER : FHT_SOCKET;
#else
			fhtype = ( errno == EBADF) ? FHT_OTHER : FHT_SOCKET;
#endif
		else
			fhtype = FHT_SOCKET;
	}

	sys s. file. type = fhtype;

	switch ( fhtype) {
	case FHT_SOCKET:
		list_add( &guts. sockets, self);
		reset_sockets();
		break;
	default:
		if ( guts. files. count == 0)
			PostMessage( NULL, WM_FILE, 0, 0);
		list_add( &guts. files, self);
		break;
	}

	return true;
}

Bool
apc_file_detach( Handle self)
{
	switch ( sys s. file. type) {
	case FHT_SOCKET:
		list_delete( &guts. sockets, self);
		reset_sockets();
		break;
	default:
		list_delete( &guts. files, self);
	}
	return true;
}

Bool
apc_file_change_mask( Handle self)
{
	switch ( sys s. file. type) {
	case FHT_SOCKET:
		reset_sockets();
		break;
	default:;
	}
	return true;
}

PList
apc_getdir( const char *dirname, Bool is_utf8)
{
#ifdef __CYGWIN__
	DIR *dh;
	struct dirent *de;
	PList dirlist = nil;
	char *type, *dname;
	char path[ 2048];
	struct stat s;

	if ( *dirname == '/' && dirname[1] == '/') dirname++;
	if ( strcmp( dirname, "/") == 0)
		dname = "";
	else
		dname = ( char*) dirname;


	if (( dh = opendir( dirname)) && (dirlist = plist_create( 50, 50))) {
		while (( de = readdir( dh))) {
			list_add( dirlist, (Handle)duplicate_string( de-> d_name));
			snprintf( path, 2047, "%s/%s", dname, de-> d_name);
			type = nil;
			if ( stat( path, &s) == 0) {
				switch ( s. st_mode & S_IFMT) {
				case S_IFIFO:        type = "fifo";  break;
				case S_IFCHR:        type = "chr";   break;
				case S_IFDIR:        type = "dir";   break;
				case S_IFBLK:        type = "blk";   break;
				case S_IFREG:        type = "reg";   break;
				case S_IFLNK:        type = "lnk";   break;
				case S_IFSOCK:       type = "sock";  break;
				}
			}
			if ( !type) type = "reg";
			list_add( dirlist, (Handle)duplicate_string( type));
		}
		closedir( dh);
	}
	return dirlist;
#else
	long		 len;
	WCHAR		 scanname[(MAX_PATH+3)*2];
	WIN32_FIND_DATAW FindData;
	HANDLE		 fh;
	WCHAR *          dirname_w;

	DWORD            fattrs;
	PList            ret;
	Bool             wasDot = false, wasDotDot = false;

#define add_entry(file,info)  {                         \
	list_add( ret, ( Handle) duplicate_string(file));   \
	list_add( ret, ( Handle) duplicate_string(info));   \
}

#define add_fentry  {                                                         \
	WideCharToMultiByte(CP_UTF8, 0, \
		FindData.cFileName, -1, \
		(LPSTR)scanname, sizeof(scanname), \
		NULL, false); \
	add_entry((char*) scanname, \
		( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DIR : FILE); \
	if ( wcscmp( L".", FindData.cFileName) == 0)                               \
		wasDot = true;                                                         \
	else if ( wcscmp( L"..", FindData.cFileName) == 0)                         \
		wasDotDot = true;                                                      \
}


#define DIR  "dir"
#define FILE "reg"

	dirname_w = is_utf8 ?
		alloc_utf8_to_wchar( dirname, -1, NULL) :
		alloc_ascii_to_wchar( dirname, -1);

	len = wcslen(dirname_w);
	if (len > MAX_PATH) {
		free(dirname_w);
		return NULL;
	}
		
	/* check to see if filename is a directory */
	fattrs = GetFileAttributesW( dirname_w);
	if ( fattrs == 0xFFFFFFFF || ( fattrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		free(dirname_w);
		return NULL;
	}

	/* Create the search pattern */
	wcscpy(scanname, dirname_w);
	if (scanname[len-1] != '/' && scanname[len-1] != '\\')
		scanname[len++] = '/';
	scanname[len++] = '*';
	scanname[len] = '\0';
	free(dirname_w);

	/* do the FindFirstFile call */
	fh = FindFirstFileW(scanname, &FindData);
	if (fh == INVALID_HANDLE_VALUE) {
		/* FindFirstFile() fails on empty drives! */
		if (GetLastError() != ERROR_FILE_NOT_FOUND)
			return NULL;
		ret = plist_create( 2, 16);
		add_entry( ".",  DIR);
		add_entry( "..", DIR);
		return ret;
	}

	ret = plist_create( 16, 16);
	add_fentry;
	while ( FindNextFileW(fh, &FindData))
		add_fentry;
	FindClose(fh);

	if ( !wasDot)
		add_entry( ".",  DIR);
	if ( !wasDotDot)
		add_entry( "..", DIR);

#undef FILE
#undef DIR
	return ret;
#endif
}

static WCHAR*
path2wchar(const char *name, Bool is_utf8, int * size)
{
	WCHAR * text;
	int xlen = -1;
	if ( size == NULL ) size = &xlen;
	if ( is_utf8 ) {
		text = alloc_utf8_to_wchar( name, *size, size);
	} else {
		*size = strlen( name) + 1;
		text = alloc_ascii_to_wchar( name, *size);
	}
	if ( !text ) errno = ENOMEM;
	return text;
}

void
win32_set_errno(void)
{
	/*
	This isn't perfect, eg. Win32 returns ERROR_ACCESS_DENIED for
	both permissions errors and if the source is a directory, while
	POSIX wants EACCES and EPERM respectively.

	Determined by experimentation on Windows 7 x64 SP1, since MS
	don't document what error codes are returned.
	*/
	switch (GetLastError()) {
	case ERROR_BAD_NET_NAME:
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_PATHNAME:
	case ERROR_FILE_NOT_FOUND:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_DRIVE:
	case ERROR_PATH_NOT_FOUND:
		errno = ENOENT;
		break;
	case ERROR_ALREADY_EXISTS:
		errno = EEXIST;
		break;
	case ERROR_ACCESS_DENIED:
		errno = EACCES;
		break;
	case ERROR_NOT_SAME_DEVICE:
		errno = EXDEV;
		break;
	case ERROR_DISK_FULL:
		errno = ENOSPC;
		break;
	case ERROR_NOT_ENOUGH_QUOTA:
		errno = EDQUOT;
		break;
	default: /* ERROR_INVALID_FUNCTION - eg. on a FAT volume */
		errno = EINVAL;
		break;
	}
}

typedef struct {
	long             position;
	HANDLE           handle;
	WIN32_FIND_DATAW fd;
	Bool             error;
	WCHAR            path[MAX_PATH+3];
} Win32_Dirhandle;

int
apc_fs_access(const char *name, Bool is_utf8, int mode, Bool effective)
{
	WCHAR *buf;
	int ret;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(name, is_utf8, NULL)))
			return false;
		ret = _waccess(buf, mode);
		free(buf);
	} else
		ret = access(name, mode);

	return ret;
}

Bool
apc_fs_chdir(const char *path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		if ( !( ok = SetCurrentDirectoryW(buf)))
			win32_set_errno();
		free(buf);
	} else {
		if ( !( ok = SetCurrentDirectoryA(path)))
			win32_set_errno();
	}

	return ok;
}

Bool
apc_fs_chmod( const char *path, Bool is_utf8, int mode)
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		ok = (_wchmod(buf, mode) == 0);
		free(buf);
	} else
		ok = (chmod(path, mode) == 0);

	return ok;
}

char *
alloc_wchar_to_utf8( WCHAR * src, int * len )
{
	int xlen = -1, srclen;
	char * ret;

	if ( !len ) len = &xlen;
	srclen = *len;

	if (( *len = WideCharToMultiByte(CP_UTF8, 0, src, srclen, NULL, 0, NULL, false)) == 0 ) {
		errno = EINVAL;
		return NULL;
	}
	if ( !( ret = malloc( *len ))) {
		errno = ENOMEM;
		return NULL;
	}
	if ( WideCharToMultiByte(CP_UTF8, 0, src, srclen, ret, *len, NULL, false) == 0) {
		free(ret);
		errno = EINVAL;
		return NULL;
	}
	return ret;
}

static char *
wstr2ascii( WCHAR * src, int * len, Bool fail_if_cannot )
{
	char * ret;
	int srclen = *len;
	DWORD flags = 0;
#ifdef WC_ERR_INVALID_CHARS
	if ( fail_if_cannot ) flags |= WC_ERR_INVALID_CHARS;
#endif
	if (( *len = WideCharToMultiByte(CP_ACP, flags, src, srclen, NULL, 0, NULL, false)) == 0 )
		return NULL;
	if ( !( ret = malloc( *len )))
		return NULL;
	if ( WideCharToMultiByte(CP_ACP, flags, src, srclen, ret, *len, NULL, false) == 0) {
		free(ret);
		return NULL;
	}
#ifndef WC_ERR_INVALID_CHARS
	if ( fail_if_cannot ) {
		int nq1 = 0, nq2 = 0, xlen;
		char * ret2 = ret;
		xlen = srclen;
		while (xlen--) if ( *(src++) == L'?' ) nq1++;
		xlen = srclen;
		while (xlen--) if ( *(ret2++) == '?' ) nq2++;
		if ( nq2 > nq1 ) {
			free(ret);
			return NULL;
		}
	}
#endif
	return ret;
}

char *
apc_fs_from_local(const char * text, int * len)
{
	char * ret;
	WCHAR * buf;
	if ( !( buf = alloc_ascii_to_wchar( text, *len)))
		return NULL;
	ret = alloc_wchar_to_utf8( buf, len );
	free( buf );
	return ret;
}


char *
apc_fs_to_local(const char * text, Bool fail_if_cannot, int * len)
{
	WCHAR *buf;
	char * ret;

	if ( !( buf = path2wchar(text, true, len)))
		return NULL;
	ret = wstr2ascii( buf, len, fail_if_cannot );
	free(buf);

	return ret;
}

Bool
apc_fs_closedir( PDirHandleRec dh)
{
	Bool ok;
	Win32_Dirhandle *d = (Win32_Dirhandle*) dh-> handle;
	ok = FindClose(d->handle);
	free(d);
	return ok;
}

char*
apc_fs_getcwd(void)
{
	int i;
	WCHAR fn[MAX_PATH+1];

	if ( !GetCurrentDirectoryW(MAX_PATH+1, fn)) {
		errno = EACCES;
		return NULL;
	}
	for ( i = 0; i < MAX_PATH; i++) {
		if ( fn[i] == 0 ) break;
		if ( fn[i] == L'\\' ) fn[i] = L'/';
	}
	return alloc_wchar_to_utf8(fn, NULL);
}

char*
apc_fs_getenv(const char * varname, Bool is_utf8, Bool * do_free)
{
	WCHAR * e, * buf;

	if ( !( buf = path2wchar(varname, is_utf8, NULL)))
		return NULL;
	e = _wgetenv(buf);
	free(buf);
	if ( !e ) return NULL;

	*do_free = true;
	return alloc_wchar_to_utf8(e, NULL);
}

Bool
apc_fs_link( const char* oldname, Bool is_old_utf8, const char * newname, Bool is_new_utf8 )
{
	WCHAR *buf1, *buf2;
	Bool ok;

	if ( !( buf1 = path2wchar(oldname, is_old_utf8, NULL)))
		return false;
	if ( !( buf2 = path2wchar(newname, is_new_utf8, NULL))) {
		free(buf1);
		return false;
	}
	if ( !( ok = ( CreateHardLinkW(buf2, buf1, NULL) == 0)))
		win32_set_errno();
	free(buf2);
	free(buf1);

	return ok;
}

Bool
apc_fs_mkdir( const char* path, Bool is_utf8, int mode)
{
	WCHAR *buf;
	Bool ok;

	if ( !( buf = path2wchar(path, is_utf8, NULL)))
		return false;
	ok = (_wmkdir(buf) == 0);
	if ( ok ) _wchmod(buf, mode);
	free(buf);

	return ok;
}

Bool
apc_fs_opendir( const char* path, PDirHandleRec dh)
{
	WCHAR * buf;
	DWORD fattrs;
	int len;
	Win32_Dirhandle * d;

	if ( !( buf = path2wchar( path, dh-> is_utf8, NULL ))) {
		errno = ENOMEM;
		return false;
	}

	len = wcslen(buf);
	if (len > MAX_PATH) {
		free(buf);
		errno = ENOMEM;
		return false;
	}
	fattrs = GetFileAttributesW( buf);
	if ( fattrs == 0xFFFFFFFF ) {
		free(buf);
		errno = ENOENT;
		return false;
	}
	if (( fattrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		free(buf);
		errno = ENOTDIR;
		return false;
	}

	if ( !( dh-> handle = malloc(sizeof(Win32_Dirhandle)))) {
		free(buf);
		errno = ENOMEM;
		return false;
	}
	d = ( Win32_Dirhandle*) dh->handle;
	bzero( d, sizeof( Win32_Dirhandle ));

	wcscpy(d->path, buf);
	if (d->path[len-1] != '/' && d->path[len-1] != '\\')
		d->path[len++] = '/';
	d->path[len++] = '*';
	d->path[len] = '\0';
	free(buf);

	d->handle = FindFirstFileW( d->path, &d->fd);
	if ( d->handle == INVALID_HANDLE_VALUE ) {
		d-> error = true;
		win32_set_errno();
		return false;
	}
	d-> position = 0;
	return true;
}

int
apc_fs_open_file( const char* path, Bool is_utf8, int flags, int mode)
{
	WCHAR *buf;
	int f;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		f = _wopen(buf, flags, mode);
		free(buf);
	} else
		f = open(path, flags, mode);

	return f;
}

Bool
apc_fs_readdir( PDirHandleRec dh, char * entry)
{
	Win32_Dirhandle *d = (Win32_Dirhandle*) dh-> handle;
	if ( d-> error )
		return false;
	if ( d-> position > 0 ) {
		if ( !FindNextFileW(d->handle, &d->fd)) {
			d-> error = true;
			return false;
		}
	}
	d-> position++;
	WideCharToMultiByte(CP_UTF8, 0, d->fd.cFileName, -1, entry, MAX_PATH, NULL, false);
	return true;
}

Bool
apc_fs_rename( const char* oldname, Bool is_old_utf8, const char * newname, Bool is_new_utf8 )
{
	Bool ok;
	WCHAR *buf1, *buf2;
	if ( !( buf1 = path2wchar(oldname, is_old_utf8, NULL)))
		return false;
	if ( !( buf2 = path2wchar(newname, is_new_utf8, NULL))) {
		free(buf1);
		return false;
	}
	ok = ( _wrename(buf1, buf2) == 0);
	free(buf2);
	free(buf1);
	return ok;
}

Bool
apc_fs_rewinddir( PDirHandleRec dh )
{
	Win32_Dirhandle *d = (Win32_Dirhandle*) dh-> handle;

	d-> error = false;
	FindClose(d-> handle);
	d->handle = FindFirstFileW( d->path, &d->fd);
	if ( d->handle == INVALID_HANDLE_VALUE ) {
		d-> error = true;
		win32_set_errno();
		return false;
	}
	d-> position = 0;
	return true;
}

Bool
apc_fs_rmdir( const char* path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		ok = (_wrmdir(buf) == 0);
		free(buf);
	} else 
		ok = (rmdir(path) == 0);

	return ok;
}

Bool
apc_fs_seekdir( PDirHandleRec dh, long position )
{
	Win32_Dirhandle *d = (Win32_Dirhandle*) dh-> handle;

	if ( position == d->position ) return true;
	if ( position < d->position || d->error ) {
		if ( !apc_fs_rewinddir(dh))
			return false;
	}

	while ( position != d->position ) {
		char buf[PATH_MAX_UTF8];
		if ( !apc_fs_readdir(dh, buf))
			return false;
	}

	return true;
}

Bool
apc_fs_stat(const char *name, Bool is_utf8, Bool link, PStatRec statrec)
{
	WCHAR *buf;
	struct _stat statbuf;
	int sz = -1, osz;

	if ( !( buf = path2wchar(name, is_utf8, &sz)))
		return false;
	osz = sz;

	/* from win32_stat:
	stat() is buggy with a trailing slashes, except for the root directory of a drive:
	remove additional trailing slashes */
	while ( sz > 2 && ( buf[sz - 2] == L'\\' || buf[sz - 2] == L'/') ) {
		buf[sz - 2] = 0;
		sz--;
	}
	/* add back slash if we otherwise end up with just a drive letter */
	if ( sz == 3 && isALPHA(buf[0]) && buf[1] == L':' ) {
		if ( osz == sz ) {
			WCHAR * buf2;
			buf2 = realloc(buf, sz * 2 + 2);
			if ( !buf2 ) {
				free(buf);
				errno = ENOMEM;
				return false;
			}
			buf = buf2;
		}
		buf[sz++ - 1] = L'\\';
		buf[sz   - 1]   = 0;
	}
	if ( _wstat(buf, &statbuf) < 0 ) {
		free(buf);
		return 0;
	}
	free(buf);

	statrec-> dev     = statbuf. st_dev;
	statrec-> ino     = statbuf. st_ino;
	statrec-> mode    = statbuf. st_mode;
	statrec-> nlink   = statbuf. st_nlink;
	statrec-> uid     = statbuf. st_uid;
	statrec-> gid     = statbuf. st_gid;
	statrec-> rdev    = statbuf. st_rdev;
	statrec-> size    = statbuf. st_size;
	statrec-> blksize = -1;
	statrec-> blocks  = -1;
	statrec-> atim    = (float) statbuf.st_atime;
	statrec-> mtim    = (float) statbuf.st_mtime;
	statrec-> ctim    = (float) statbuf.st_ctime;
	return 1;
}

long
apc_fs_telldir( PDirHandleRec dh )
{
	Win32_Dirhandle *d = (Win32_Dirhandle*) dh-> handle;
	return d->position;
}

Bool
apc_fs_unlink( const char* path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		ok = (_wunlink(buf) == 0);
		free(buf);
	} else
		ok = (unlink(path) == 0);

	return ok;
}

/* from win32/win32.c */

/* fix utime() so it works on directories in NT */
static Bool
filetime_from_time(PFILETIME pFileTime, float Time)
{
	time_t sec = (time_t) Time;
	struct tm *pTM = localtime(&sec);
	SYSTEMTIME SystemTime;
	FILETIME LocalTime;

	if (pTM == NULL)
		return false;

	SystemTime.wYear   = pTM->tm_year + 1900;
	SystemTime.wMonth  = pTM->tm_mon + 1;
	SystemTime.wDay    = pTM->tm_mday;
	SystemTime.wHour   = pTM->tm_hour;
	SystemTime.wMinute = pTM->tm_min;
	SystemTime.wSecond = pTM->tm_sec;
	SystemTime.wMilliseconds = ( Time - (int) Time ) * 1000;

	return SystemTimeToFileTime(&SystemTime, &LocalTime) &&
	       LocalFileTimeToFileTime(&LocalTime, pFileTime);
}

Bool
apc_fs_setenv(const char * varname, Bool is_name_utf8, const char * value, Bool is_value_utf8)
{
	WCHAR *buf1, *buf2;
	Bool ok = false;

	if ( !( buf1 = path2wchar(varname, is_name_utf8, NULL)))
		return false;
	if ( !( buf2 = path2wchar(value,   is_value_utf8, NULL))) {
		free(buf1);
		return false;
	}

	ok = (_wputenv_s(buf1, buf2) == 0);

	free(buf2);
	free(buf1);

	return ok;
}

static Bool
win32_utimes(float atime, float mtime, WCHAR * filename)
{
	Bool ok = false;
	HANDLE handle;
	FILETIME ftCreate;
	FILETIME ftAccess;
	FILETIME ftWrite;

	/* This will (and should) still fail on readonly files */
	handle = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		errno = EINVAL;
		return false;
	}

	if (!GetFileTime(handle, &ftCreate, &ftAccess, &ftWrite)) {
		win32_set_errno();
		goto EXIT;
	}
	if (
		!filetime_from_time(&ftAccess, atime) ||
		!filetime_from_time(&ftWrite, mtime)
	) {
		errno = EINVAL;
		goto EXIT;
	}
	if ( !SetFileTime(handle, &ftCreate, &ftAccess, &ftWrite)) {
		win32_set_errno();
		goto EXIT;
	}

	CloseHandle(handle);
	ok = true;
EXIT:
	return ok;
}


Bool
apc_fs_utime( double atime, double mtime, const char* path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( !( buf = path2wchar(path, is_utf8, NULL)))
		return false;
	ok = win32_utimes(atime, mtime, buf);
	free(buf);

	return ok;
}



#ifdef __cplusplus
}
#endif
