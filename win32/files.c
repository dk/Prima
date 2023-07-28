#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef SO_OPENTYPE
#define SO_OPENTYPE 0x7008
#endif

#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "File.h"

#define var (( PFile) self)->
#define  sys (( PDrawableData)(( PFile) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PFile) view)-> sysData)->

#ifdef __cplusplus
extern "C" {
#endif

#undef  getsockopt

static int  socket_version = 0;
static List  ts;
static List  th;
static List  tf;
static PHash socket_events = NULL;

typedef struct {
	WINHANDLE event;
	int       refcnt;
	int       effective_mask;
} SocketEventRec, *PSocketEventRec;

static void
reset_syshandles(void)
{
	int i;

	for ( i = 0; i < th.count; i++) {
		Handle self = th.items[i];
		switch ( sys s.file.type ) {
		case FHT_STDIN:
			if ( var eventMask & feRead) {
				/* apc_file_attach is responsible for select_handles[] not overflowing */
				select_handles[ select_n_handles++ ] = (HANDLE) sys s.file.object;
				return;
			}
		}
	}
}

static Bool
fill_socket_vector( void * item, int keyLen, void * key, void * params)
{
	PSocketEventRec ev = (PSocketEventRec ) item;
	/* apc_file_attach is responsible for select_handles[] not overflowing */
	if ( ev-> effective_mask != 0 ) {
		select_handles[ select_n_handles++ ] = ev-> event;
		if ( debug ) {
			char buf[64];
			snprintf( buf, 64, "%p ", (void*) ev->event);
			strcat(( char*) params, buf);
		}
	}
	return false;
}

static void
reset_all_handles(void)
{
	char debug_buf[MAX_SELECT_HANDLES * ( 4 + sizeof(void*) * 2 )] = "";
	select_n_handles = 0;
	reset_syshandles();
	hash_first_that( socket_events, fill_socket_vector, debug_buf, NULL, NULL );
	if ( debug ) warn("reset socket events: %s\n", debug_buf);
}

static void
dispatch_file_event( Handle self, int cmd)
{
	Event ev;
	if (debug) warn("main: dealing cmd=%s to %s.%p" ,
		( cmd == cmFileRead) ? "cmFileRead" :
			(( cmd == cmFileWrite) ? "cmFileWrite" : "cmFileException"),
		var name,
		(void*)self
	);
	ev. cmd = cmd;
	if ( var stage == csNormal )
		CComponent(self)-> message( self, &ev);
}

static void
dispatch_file_msg( PList t, int mask, WINHANDLE src)
{
	int i;
	for ( i = 0; i < t->count; i++) {
		Handle self = t->items[ i];
		if (
			( sys s.file.event == src) &&
			( var eventMask & mask )
		) {
			dispatch_file_event(self,
				( mask == feRead)   ? cmFileRead :
				(( mask == feWrite) ? cmFileWrite : cmFileException)
			);
			break;
		}
	}
}

Bool
file_process_events(int cmd, WPARAM param1, LPARAM param2)
{
	Bool events_handled = false;

	switch (cmd) {
	case WM_FILE_REHASH:
		{
			int i;
			for ( i = 0; i < tf.count; i++) {
				Handle self = tf.items[i];
				CFile( self)-> is_active( self, true);
			}
			for ( i = 0; i < tf.count; i++) {
				Handle self = tf.items[i];
				if ( var eventMask & feRead)
					PostMessage( NULL, WM_FILE, feRead, ( LPARAM) self);
				if ( var eventMask & feWrite)
					PostMessage( NULL, WM_FILE, feWrite, ( LPARAM) self);
			}
			PostMessage( NULL, WM_FILE_REHASH, 0, 0);
		}
		break;
	case WM_FILE:
		dispatch_file_msg(&tf, param1, (WINHANDLE) param2);
		break;
	}

	return events_handled;
}

Bool
process_file_msg( WINHANDLE src)
{
	int i, available_events = 0;
	WSANETWORKEVENTS ev;
	dispatch_file_msg( &th, feRead, src );

	for ( i = 0; i < ts.count; i++) {
		Handle self = ts.items[ i];
		if ( sys s.file.event != src)
			continue;
		if ( debug ) warn(
			"message to file(%p) in socket(%p), event(%p)\n", 
			(void*)self, (void*)sys s.file.object, (void*)sys s.file.event
		);

		if ( WSAEnumNetworkEvents((SOCKET) sys s.file.object, (WSAEVENT) sys s.file.event, &ev) != 0 ) {
			rc = WSAGetLastError();
			apcWarn;
			return false;
		}

		/* WSAEnumNetworkEvents returns usable value only once */
		available_events = 0;
		if (ev.lNetworkEvents & (FD_ACCEPT | FD_READ | FD_CLOSE))
			available_events |= feRead;
		if (ev.lNetworkEvents & (FD_CONNECT | FD_WRITE))
			available_events |= feWrite;
		if (ev.lNetworkEvents & FD_OOB)
			available_events |= feException;
		if ( debug ) warn( "available events: %x\n", available_events);
		break;
	}

	if ( available_events == 0 ) return true;

	for ( i = 0; i < ts.count; i++) {
		int e;
		Handle self = ts.items[ i];
		if ( sys s.file.event != src)
			continue;
		if (( e = (available_events & var eventMask)) == 0)
			continue;

		protect_object(self);
		if ( e & feRead)
			dispatch_file_event( self, cmFileRead );
		if ( e & feWrite)
			dispatch_file_event( self, cmFileWrite );
		if ( e & feException)
			dispatch_file_event( self, cmFileException );
		unprotect_object(self);
	}

	return true;
}

static int
mask2wsa(int src)
{
	long dst = 0;
	if ( src & feRead )
		dst |= FD_ACCEPT | FD_READ | FD_CLOSE;
	if ( src & feWrite )
		dst |= FD_CONNECT | FD_WRITE;
	if ( src & feException )
		dst |= FD_OOB;
	return dst;
}


/* more than one socket must refer to same event, with a combined mask,
because WSAEventSelect(s,e1) then WSAEventSelect(s,e2) doesn't work */
static void
socket_event_update_mask( Handle self)
{
	int i, mask;
	PSocketEventRec ev;
	if ( ( ev = (PSocketEventRec) hash_fetch( socket_events, &sys s.file.object, sizeof( sys s.file.object))) == NULL)
		return;

	for ( i = 0, mask = 0; i < ts.count; i++) {
		PFile f = (PFile) ts.items[i];
		if ( dsys(f)s.file.object == sys s.file.object)
			mask |= f->eventMask;
	}

	if ( mask != ev-> effective_mask ) {
		if ( debug ) warn(
			"socket(%p),event(%p) changed mask from %x to %x\n",
			(void*)sys s.file.object, (void*)ev->event, ev->effective_mask, mask
		);
		ev-> effective_mask = mask;
		if ( WSAEventSelect((SOCKET ) sys s.file.object, (WSAEVENT) ev->event, mask2wsa(ev-> effective_mask)) != 0 ) {
			rc = WSAGetLastError();
			apcWarn;
		}
	}
}

Bool
file_subsystem_init( void)
{
	int  _data, _sz = sizeof(int);
	(void)_data;
	(void)_sz;
#ifdef PERL_OBJECT     /* init perl socket library, if any */
	PL_piSock-> Htons( 80);
#else
	win32_htons(80);
#endif
	if ( getsockopt(( SOCKET) INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char*)&_data, &_sz) != 0)
		socket_version = -1; /* no sockets available */
	else
		socket_version = 1;

	list_create(&th, 8, 8);
	list_create(&ts, 8, 8);
	list_create(&tf, 8, 8);
	socket_events = prima_hash_create();

	return true;
}

void
file_subsystem_done( void)
{
	prima_hash_destroy( socket_events, true );
	list_destroy(&tf);
	list_destroy(&ts);
	list_destroy(&th);
}

Bool
apc_file_attach( Handle self)
{
	int fhtype;
	PSocketEventRec ev;
	objCheck false;

	if (socket_version == -1)
		return false;

	sys s.file.object = _get_osfhandle( var fd);

	if ( GetStdHandle( STD_INPUT_HANDLE ) == (HANDLE) sys s.file.object ) {
		fhtype = FHT_STDIN;
	} else {
		int result = WSAAsyncSelect((SOCKET) sys s. file. object, (HWND) NULL, 0, 0);
		if ( result != 0)
			fhtype = ( WSAGetLastError() == WSAENOTSOCK) ? FHT_OTHER : FHT_SOCKET;
		else
			fhtype = FHT_SOCKET;
	}

	sys s. file. type = fhtype;

	switch ( fhtype) {
	case FHT_SOCKET:
		if ( select_n_handles >= MAX_SELECT_HANDLES )
			return false;
		list_add( &ts, self);
		if ( !( ev = (PSocketEventRec) hash_fetch( socket_events, &sys s.file.object, sizeof( sys s.file.object)))) {
			ev = malloc( sizeof(SocketEventRec));
			memset( ev, 0, sizeof(SocketEventRec));
			ev-> event = (WINHANDLE) WSACreateEvent();
			hash_store( socket_events, &sys s.file.object, sizeof( sys s.file.object ), ev);
		}

		if ( debug ) warn(
			"file(%p) is socket(%p) and event(%p)\n", 
			(void*)self, (void*)sys s.file.object, (void*)ev->event
		);
		sys s.file.event = ev-> event;
		ev-> refcnt++;
		socket_event_update_mask(self);
		reset_all_handles();
		break;
	case FHT_STDIN:
		if ( th.count == 0 && select_n_handles >= MAX_SELECT_HANDLES )
			return false;
		sys s.file.event = (WINHANDLE) sys s.file.object;
		list_add( &th, self);
		reset_all_handles();
		break;
	default:
		if ( tf.count == 0)
			PostMessage( NULL, WM_FILE_REHASH, 0, 0);
		list_add( &tf, self);
		break;
	}

	return true;
}

Bool
apc_file_detach( Handle self)
{
	PSocketEventRec ev;
	switch ( sys s. file. type) {
	case FHT_SOCKET:
		if ( ( ev = (PSocketEventRec) hash_fetch( socket_events, &sys s.file.object, sizeof( sys s.file.object)))) {
			if ( 0 == --ev-> refcnt ) {
				WSACloseEvent(( WSAEVENT ) ev-> event );
				hash_delete( socket_events, &sys s.file.object, sizeof( sys s.file.object), true);
			}
		}
		list_delete( &ts, self);
		reset_all_handles();
		break;
	case FHT_STDIN:
		list_delete( &th, self);
		reset_all_handles();
		break;
	default:
		list_delete( &tf, self);
	}
	return true;
}

Bool
apc_file_change_mask( Handle self)
{
	switch ( sys s. file. type) {
	case FHT_SOCKET:
		socket_event_update_mask(self);
		reset_all_handles();
		break;
	case FHT_STDIN:
		reset_all_handles();
		break;
	default:;
	}
	return true;
}

PList
apc_getdir( const char *dirname, Bool is_utf8)
{
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
		alloc_ascii_to_wchar( dirname, NULL);

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
		text = alloc_ascii_to_wchar( name, size);
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
#ifdef EDQUOT
	case ERROR_NOT_ENOUGH_QUOTA:
		errno = EDQUOT;
		break;
#endif
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
	if ( fail_if_cannot && !guts.wc2mb_is_fragile)
		flags |= WC_ERR_INVALID_CHARS;
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
		xlen = *len;
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
	if ( !( buf = alloc_ascii_to_wchar( text, len)))
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
	WCHAR * buf, e[32768];
	Bool ok;

	if ( !( buf = path2wchar(varname, is_utf8, NULL)))
		return NULL;
	ok = (GetEnvironmentVariableW(buf, e, sizeof(e)) > 0);
	free(buf);
	if ( !ok ) return NULL;

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

	ok = (SetEnvironmentVariableW(buf1, buf2) != 0);

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
