#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef SO_OPENTYPE
#define SO_OPENTYPE 0x7008
#endif

void my_fd_zero( fd_set* f)           { FD_ZERO( f); }
typedef fd_set type_fd_set;
void std_fd_set( intptr_t fd, fd_set * f) { FD_SET(fd, f); }

#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Component.h"
#include "File.h"

void my_fd_set( intptr_t fd, type_fd_set * f) { std_fd_set( fd, f); }

#define var (( PFile) self)->
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->

#ifdef __cplusplus
extern "C" {
#endif

#undef  socket
#undef  listen
#undef  bind
#undef  accept
#undef  select
#undef  connect
#undef  getsockname
#undef  getsockopt
#undef  setsockopt
#undef  send
#undef  recv
#undef  fd_set
#undef  FD_ZERO
#define FD_ZERO my_fd_zero
#undef  FD_SET
#define FD_SET my_fd_set

typedef struct {
	int      cmd;
	int      param1;
	intptr_t param2;
	Bool     change_is_ready, response_is_ready;
	HANDLE   change_mutex, response_mutex;
	HANDLE   thread_id;
	List     objects;
} ThreadStorage;

static ThreadStorage   ts;
static int             socket_version = 0;
static fd_set          socket_set1[3];
static fd_set          socket_set2[3];
static int             socket_commands[3] = { feRead, feWrite, feException};
static SOCKET          socket_control_channel[2] = {0,0};

#define MAX_SYSHANDLES 32
static Bool            syshandle_thread_started = false;
static Bool            syshandle_set_changed    = false;
static int             syshandle_set_count;
static HANDLE          syshandle_set1_handles[MAX_SYSHANDLES];
static HANDLE          syshandle_set2_handles[MAX_SYSHANDLES];
static Byte            syshandle_set1_types[MAX_SYSHANDLES];
static Byte            syshandle_set2_types[MAX_SYSHANDLES];
//static ThreadStorage   syshandle;

#define MUTEX_SIGNATURE 0xff

int
thr_warn( const char *format, ...)
{
	int rc;
	va_list args;
	char buf[256];
	va_start( args, format);
	rc = vsnprintf( buf, 255, format, args);
	buf[rc++] = '\n';
	buf[rc] = 0;
	va_end( args);
	write( 2, buf, rc );
	return rc;
}

#define LOG if (debug) thr_warn
#define socketErr(s) if (debug) thr_warn( "%s failed at %s.%d: %s\n", s, __FILE__, __LINE__, err_msg(WSAGetLastError(), NULL))

HANDLE
mutex_create()
{
	HANDLE mutex;
	if ( !( mutex = CreateMutex( NULL, FALSE, NULL))) {
		apiErr;
		thr_warn("Failed to create mutex object");
		return (HANDLE) 0;
	}
	return mutex;
}

Bool
mutex_take( HANDLE mutex, char * file, int line )
{
	if ( WaitForSingleObject( mutex, INFINITE) == WAIT_OBJECT_0)
		return true;
	thr_warn("Failed to obtain mutex ownership at %s line %d", file, line);
	return false;
}

/* Main thread signals another thread that change is ready for consumption
byt setting change_is_ready flag, and then the thread #2 grabs change_mutex
that generally stays free. After copying the changed data the mutex is released
to be used for next change by the main thread */

static Bool
thread_accept_change( ThreadStorage *t)
{
	return MUTEX_TAKE(t->change_mutex);
}

static void
thread_release_change( ThreadStorage *t)
{
	t->change_is_ready = false;
	ReleaseMutex( t->change_mutex);
}

static void
thread_release_response( ThreadStorage *t)
{
	ReleaseMutex(t-> response_mutex);
	/* don't release and immediately take, because the worker thread may be just slow and
	wouldn't have a chance to sync. Make sure the sync happened with the help of
	the response_is_ready flag */
	while (!t-> response_is_ready) Sleep(1);
	LOG("thread.response released");
	MUTEX_TAKE(t-> response_mutex);
}

/*

Main thread holds response_mutex, which is used for thread #2 to wait to avoid
starvation if f ex the main thread is not serving event loop or just generally
slow. When the thread #2 wants to report a result, it sets the flag
response_is_ready which will be checked inside the main thread event loop, and
will push the main thread to process it with PostThreadMessage if the event
loop is waiting for input. And then it wait for response_mutex.

Once the main thread wakes up, it handles the response and re-acquires the
response_mutex, see how in thread_release_response

*/
static Bool
thread_respond( ThreadStorage *t, int cmd, int param1, intptr_t param2)
{
	Bool ok;
	t-> cmd    = cmd;
	t-> param1 = param1;
	t-> param2 = param2;
	t-> response_is_ready = 1;

	/* wake up the main thread */
	LOG("thread.respond begin sync");
	while ( !PostThreadMessage( guts.main_thread_id, WM_NOOP, 0, 0))
		Sleep(1);
	/* wait until it handles the message to the callbacks */
	ok = MUTEX_TAKE(t-> response_mutex);
	LOG("thread.respond end sync");
	t-> response_is_ready = 0;
	ReleaseMutex(t-> response_mutex);
	return ok;
}

DWORD WINAPI
socket_select( LPVOID dummy)
{
	int j, result;
	LOG("socket thread started");
	while ( !prima_guts.app_is_dead) {
		if ( ts.change_is_ready) {
			int i, count;
			LOG("socket.change_is_ready", __LINE__);
			if ( !thread_accept_change(&ts))
				break;
			/* read incoming changes */
			for ( i = 0; i < 3; i++)
				memcpy( socket_set1+i, socket_set2+i, sizeof( fd_set));
			thread_release_change(&ts);
			count = socket_set1[0]. fd_count + socket_set1[1]. fd_count + socket_set1[2]. fd_count;
			LOG("fd count=%d", count);
			/* stop the thread */
			if ( count == 0 )
				break;
		}

		/* wait for either control socket or data signal */
		LOG("entering select...");
		result = select( FD_SETSIZE-1, &socket_set1[0], &socket_set1[1], &socket_set1[2], NULL);
		LOG("select result=%d", result);

		if ( result == 0) continue;
		if ( result < 0) {
			socketErr("select");
			/* possibly some socket was closed? */
			ts.cmd = WM_SOCKET_REHASH;
			LOG("WM_SOCKET_REHASH sent");
			if ( !thread_respond(&ts, WM_SOCKET_REHASH, 0, 0))
				goto FAIL;
			continue;
		}

		for ( j = 0; j < 3; j++) {
			int i;
			for ( i = 0; i < socket_set1[j]. fd_count; i++) {
				SOCKET s = socket_set1[j].fd_array[i];
				if ( s == socket_control_channel[1] ) {
					if ( j == 0 ) {
						char r;
						LOG("flushing control channel %ld", s);
						if ( recv( s, &r, 1, 0) != 1 ) {
							socketErr("recv");
							goto FAIL;
						}
					}
					continue;
				}
				LOG("WM_SOCKET %d %ld sent", socket_commands[i], s);
				if ( !thread_respond( &ts, WM_SOCKET, socket_commands[j], s))
					goto FAIL;
			}
		}
		ts.change_is_ready = true;
	}
	LOG("socket thread failed");
FAIL:
	LOG("socket thread ended");

	/* if somehow failed, making restart possible */
	ts.thread_id         = 0;
	ts.response_is_ready = false;

	return 0;
}

/* socketpair.c
 * Copyright 2007, 2010 by Nathan C. Myers <ncm@cantrip.org>
 * This code is Free Software.  It may be copied freely, in original or
 * modified form, subject only to the restrictions that (1) the author is
 * relieved from all responsibilities for any use for any purpose, and (2)
 * this copyright notice must be retained, unchanged, in its entirety.  If
 * for any reason the author might be held responsible for any consequences
 * of copying or use, license is withheld.
 */
static Bool
prima_socketpair(SOCKET socks[2])
{
	union {
		struct sockaddr_in inaddr;
		struct sockaddr addr;
	} a;

	SOCKET listener;
	socklen_t addrlen = sizeof(a.inaddr);
	int reuse = 1;

	if (( listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		socketErr("socket");
		return false;
	}

	memset(&a, 0, sizeof(a));
	a.inaddr.sin_family = AF_INET;
	a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	a.inaddr.sin_port = 0;

	socks[0] = socks[1] = INVALID_SOCKET;

	do {
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
			       (char *) &reuse, (socklen_t) sizeof(reuse)) == -1) {
			socketErr("setsockopt(SOL_SOCKET,SO_REUSEADDR)");
			break;
		}
		if (bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR) {
			socketErr("bind");
			break;
		}
		memset(&a, 0, sizeof(a));
		if (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR) {
			socketErr("getsockname");
			break;
		}

		// win32 getsockname may only set the port number, p=0.0005.
		// ( http://msdn.microsoft.com/library/ms738543.aspx ):
		a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		a.inaddr.sin_family = AF_INET;

		if (listen(listener, 1) == SOCKET_ERROR) {
			socketErr("listen");
			break;
		}
		if ((socks[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0)) == INVALID_SOCKET) {
			socketErr("WSASocket(AF_INET,SOCK_STREAM)");
			break;
		}
		if (connect(socks[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR) {
			socketErr("connect");
			break;
		}
		if (( socks[1] = accept(listener, NULL, NULL)) == INVALID_SOCKET) {
			socketErr("accept");
			break;
		}
		closesocket(listener);
		return true;
	} while (0);

	closesocket(listener);
	closesocket(socks[0]);
	closesocket(socks[1]);
	return false;
}

static void
reset_sockets( void)
{
	int i;
	Bool wakeup = true;

	/* enter section */
	if ( ts.thread_id && !MUTEX_TAKE( ts.change_mutex))
		return;

	LOG("reset %d sockets", ts.objects.count);
	/* copying handles */
	for ( i = 0; i < 3; i++)
		FD_ZERO( &socket_set2[i]);

	for ( i = 0; i < ts.objects.count; i++) {
		Handle self = ts.objects.items[i];
		if ( var eventMask & feRead) {
			LOG("change: %s wants to read to %ld", var name, sys s.file.object);
			FD_SET( sys s.file.object,  &socket_set2[0]);
		}
		if ( var eventMask & feWrite) {
			LOG("change: %s wants to write to %ld", var name, sys s.file.object);
			FD_SET( sys s. file.object, &socket_set2[1]);
		}
		if ( var eventMask & feException) {
			LOG("change: %s wants to oob to %ld", var name, sys s.file.object);
			FD_SET( sys s.file.object,  &socket_set2[2]);
		}
	}

	/* set up the control channel to wake up the thread on change */
	if ( socket_control_channel[0] == 0 ) {
		if ( !prima_socketpair(socket_control_channel))
			return;
		LOG("socketpair: %ld %ld", socket_control_channel[0], socket_control_channel[1]);
	}
	FD_SET( socket_control_channel[1], &socket_set2[0]);

	/* leave section and start the thread, if needed */
	if ( !ts.thread_id) {
		ts.change_is_ready = false;
		if ( !( ts.thread_id = CreateThread( NULL, 0, socket_select, NULL, 0, NULL ))) {
			apiErr;
			return;
		}
		LOG("create thread %d", ts.thread_id);
		wakeup = false;
	} else
		ReleaseMutex( ts.change_mutex);

	/* signal the thread */
	ts.change_is_ready = true;
	if ( wakeup ) {
		char s;
		LOG("sending wakeup call via control channel %ld", socket_control_channel[1]);
		if ( send( socket_control_channel[0], &s, 1, 0) != 1 )
			socketErr("send");
	}
}

void
socket_rehash( void)
{
	int i;
	for ( i = 0; i < ts.objects.count; i++) {
		Handle self = ts.objects.items[i];
		CFile( self)-> is_active( self, true);
	}
}

DWORD WINAPI
syshandle_select( LPVOID dummy)
{
	int count = 0;
	DWORD n;
	Bool ok;

	while ( !prima_guts.app_is_dead) {
		if ( syshandle_set_changed) {
			/* updating handles */
			if ( !MUTEX_TAKE( guts.thread_mutex))
				break;
			count = syshandle_set_count;
			memcpy( syshandle_set1_handles, syshandle_set2_handles, sizeof(HANDLE) * syshandle_set_count);
			memcpy( syshandle_set1_types,   syshandle_set2_types,   syshandle_set_count);
			syshandle_set_changed = false;
			ReleaseMutex( guts. thread_mutex);
			if ( count == 0 ) /* stop the thread */
				break;
		}

		/* wait now, wake up by mutex if anything */
		n = WaitForMultipleObjects(
			count, syshandle_set1_handles,
			false, INFINITE
		);
		if ( n == WAIT_TIMEOUT )
			continue;

		if ( n >= WAIT_OBJECT_0 && n <= WAIT_OBJECT_0 + count - 1 ) {
			n -= WAIT_OBJECT_0;

			if (syshandle_set1_types[n] == MUTEX_SIGNATURE ) {
				ReleaseMutex( syshandle_set1_handles[n] );
				continue;
			}

			guts.syshandle_response_type   = syshandle_set1_types[n];
			guts.syshandle_response_handle = syshandle_set1_handles[n];
		} else {
			/* some bad handle? rehash */
			guts.syshandle_response_type   = 0;
			guts.syshandle_response_handle = 0;
		}

		guts.syshandle_post_sync = 1; /* and wake up main thread */
		while ( !PostThreadMessage( guts.main_thread_id, WM_NOOP, 0, 0))
			Sleep(1);
		ok = MUTEX_TAKE( guts.syshandle_mutex_out);
		guts.syshandle_post_sync = 0;
		ReleaseMutex(guts.syshandle_mutex_out);
		if ( !ok ) break;
	}

	/* if somehow failed, making restart possible */
	syshandle_thread_started = false;
	return 0;
}

static void
reset_syshandles( void)
{
	int i;

	/* enter section */
	if ( syshandle_thread_started && !MUTEX_TAKE( guts. thread_mutex))
		return;

	syshandle_set_count = 0;
	for ( i = 0; i < guts. syshandles. count; i++) {
		Handle self = guts. syshandles. items[i];
		switch ( sys s. file. type ) {
		case FHT_STDIN:
			if ( var eventMask & feRead) {
				syshandle_set2_handles[ syshandle_set_count ] = (HANDLE) sys s.file.object;
				syshandle_set2_types  [ syshandle_set_count ] = feRead;
				syshandle_set_count++;
				if ( syshandle_set_count == MAX_SYSHANDLES - 1) goto ENOUGH;
			}
			break;
		}
	}

	if ( syshandle_set_count > 0 ) {
		/* add controlling mutex */
		syshandle_set2_handles[ syshandle_set_count ] = guts. syshandle_mutex_in;
		syshandle_set2_types  [ syshandle_set_count ] = MUTEX_SIGNATURE;
		syshandle_set_count++;
	}

ENOUGH:
	syshandle_set_changed = true;

	/* leave section and start the thread, if needed */
	if ( !syshandle_thread_started) {
		MUTEX_TAKE(guts.syshandle_mutex_out);
		guts. syshandle_thread = CreateThread( NULL, 0, syshandle_select, NULL, 0, NULL );
		syshandle_thread_started = true;
	} else
		ReleaseMutex( guts.thread_mutex);

	/* finally sync-wait for the thread to read the changed data */
	if ( syshandle_set_count > 0 )
		MUTEX_TAKE( guts. syshandle_mutex_in );
}

void
syshandle_rehash( void)
{
	int i;
	for ( i = 0; i < guts. syshandles. count; i++) {
		Handle self = guts. syshandles. items[i];
		CFile( self)-> is_active( self, true);
	}
}

Bool
file_process_events( void)
{
	Bool events_handled = false;
	if ( guts.syshandle_post_sync) {
		MSG m;
		m.message = guts.syshandle_response_handle ? WM_SYSHANDLE : WM_SYSHANDLE_REHASH;
		m.wParam  = (WPARAM) guts.syshandle_response_type;
		m.lParam  = (LPARAM) guts.syshandle_response_handle;
		guts.syshandle_post_sync = 0;
		process_msg(&m);
		guts.syshandle_post_sync = 1;

		/* this thread keeps mutex_out, but needs to release it to signal back
		to the caller thread that the message handling is finished and it can
		continue. This code below is to retake the mutex back */
		ReleaseMutex(guts.syshandle_mutex_out);
		while (!guts.syshandle_post_sync) Sleep(1);
		MUTEX_TAKE(guts.syshandle_mutex_out);

		events_handled = true;
	}

	if ( ts.response_is_ready) {
		events_handled = true;

		switch ( ts.cmd ) {
		case WM_SOCKET: {
			int i;
			int    mask = ts.param1;
			SOCKET src  = ts.param2;
			LOG("WM_SOCKET %d %ld received", mask, src);
			for ( i = 0; i < ts.objects.count; i++) {
				Handle self = ts.objects.items[ i];
				if (
					( sys s.file.object == src) &&
					( var eventMask & mask )
				) {
					Event ev;
					ev. cmd = ( mask == feRead) ? cmFileRead :
						(( mask == feWrite) ? cmFileWrite : cmFileException);
					LOG("Dealing cmd=%x to %s", ev.cmd, var name);
					CComponent(self)-> message( self, &ev);
					break;
				}
			}
			break;
		}
		case WM_SOCKET_REHASH:
			LOG("WM_SOCKET_REHASH received");
			socket_rehash();
			break;
		}

		thread_release_response(&ts);
	}

	return events_handled;
}

static Bool
create_thread_storage( ThreadStorage *t)
{
	memset( t, 0, sizeof(ThreadStorage));
	if ( !( t->change_mutex = mutex_create()))
		return false;
	if ( !( t->response_mutex = mutex_create()))
		return false;
	MUTEX_TAKE(t-> response_mutex);
	list_create(&t->objects, 8, 8);
	return true;
}

static void
delete_thread_storage( ThreadStorage* t)
{
	if ( t->thread_id ) {
		LOG("terminate thread %d", t->thread_id);
		TerminateThread(t->thread_id, 0);
	}
	if ( t-> change_mutex )
		CloseHandle( t-> change_mutex );
	if ( t-> response_mutex )
		CloseHandle( t-> response_mutex );
	list_destroy( &t->objects);
}

Bool
file_subsystem_init( void)
{
	if ( !( guts.thread_mutex = mutex_create()))
		return false;
	if ( !( guts.syshandle_mutex_in = mutex_create()))
		return false;
	if ( !( guts.syshandle_mutex_out = mutex_create()))
		return false;

	if ( !create_thread_storage(&ts))
		return false;

	list_create( &guts. files, 8, 8);
	list_create( &guts. syshandles, 8, 8);

	return true;
}

void
file_subsystem_done( void)
{
	CloseHandle( guts. thread_mutex);
	CloseHandle( guts. syshandle_mutex_in);
	CloseHandle( guts. syshandle_mutex_out);
	list_destroy( &guts. files);
	list_destroy( &guts. syshandles);

	delete_thread_storage(&ts);
	if (socket_control_channel[0]) {
		closesocket( socket_control_channel[0] );
		closesocket( socket_control_channel[1] );
	}
}

Bool
apc_file_attach( Handle self)
{
	int fhtype;
	objCheck false;

	if ( PFile(self)->fd > FD_SETSIZE ) return false;

	if ( socket_version == 0) {
		int  _data, _sz = sizeof( int);
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
	}

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
		list_add( &ts.objects, self);
		reset_sockets();
		break;
	case FHT_STDIN:
		list_add( &guts. syshandles, self);
		reset_syshandles();
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
		list_delete( &ts.objects, self);
		reset_sockets();
		break;
	case FHT_STDIN:
		list_delete( &guts. syshandles, self);
		reset_syshandles();
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
	case FHT_STDIN:
		reset_syshandles();
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
