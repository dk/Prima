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
#include "File.h"

void my_fd_set( intptr_t fd, type_fd_set * f) { std_fd_set( fd, f); }

#define var (( PFile) self)->
#define  sys (( PDrawableData)(( PFile) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PFile) view)-> sysData)->

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
#undef  write
#undef  fd_set
#undef  FD_ZERO
#define FD_ZERO my_fd_zero
#undef  FD_SET
#define FD_SET my_fd_set

typedef struct {
	volatile int      cmd;
	volatile int      param1;
	volatile intptr_t param2;
	volatile Bool     change_is_ready, response_is_ready;
	volatile HANDLE   change_mutex, response_mutex;
	volatile int      change_refcnt;

	HANDLE   thread_handle;
	DWORD    thread_id;
	List     objects;
	int      rehashing;
} ThreadStorage;

static volatile Bool   thread_noop_flag = 0;

static ThreadStorage   ts;
static int             socket_version = 0;
static fd_set          socket_set2[3];
static SOCKET          socket_control_channel[2] = {0,0};

#define MAX_SYSHANDLES 32
#define MUTEX_SIGNATURE 0xff
static ThreadStorage   th;
static int             syshandle_set_count;
static HANDLE          syshandle_set2_handles[MAX_SYSHANDLES];
static Byte            syshandle_set2_types[MAX_SYSHANDLES];
static HANDLE          syshandle_control_mutex;

static ThreadStorage   tf;

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
#define THR_ID(t) ((t->thread_id == GetCurrentThreadId()) ? ((t == &ts) ? "socket" : "syshandle") : "main")
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

/* win32 mutexes are apparently refcounted */
void
mutex_release( HANDLE mutex )
{
	while ( ReleaseMutex( mutex)) {} ;
}

/* Main thread signals another thread that change is ready for consumption
byt setting change_is_ready flag, and then the thread #2 grabs change_mutex
that generally stays free. After copying the changed data the mutex is released
to be used for next change by the main thread */

static Bool
thread_begin_change( ThreadStorage *t)
{
	LOG("%s: thread_begin_change refcnt=%d waiting for change_mutex..", THR_ID(t), t->change_refcnt);
	if ( !MUTEX_TAKE(t->change_mutex)) return false;
	LOG("%s: thread_begin_change OK refcnt=%d..", THR_ID(t), t->change_refcnt);
	if ( t-> change_refcnt++ > 1 )
		ReleaseMutex(t->change_mutex); /* do not overflow internal refcounting */
	return true;
}

static void
thread_end_change( ThreadStorage *t)
{
	LOG("%s: thread_end_change refcnt=%d..", THR_ID(t), t->change_refcnt - 1);
	if ( --t-> change_refcnt > 0 ) return;
	t-> change_refcnt = 0;
	LOG("%s: release change_mutex", THR_ID(t));
	mutex_release(t->change_mutex);
}

static void
thread_release_response( ThreadStorage *t)
{
	LOG("main: release response_mutex");
	mutex_release(t-> response_mutex);
	Sleep(1);
	LOG("main: waiting to retake response_mutex");
	MUTEX_TAKE(t-> response_mutex);
	LOG("main: retaken response_mutex");
	t-> response_is_ready = 0;
}

/*

Main thread holds response_mutex, which is used for thread #2 to wait to avoid
starvation if f ex the main thread is not serving event loop or just generally
slow. When the thread #2 wants to report a result, it sets the flag
response_is_ready which will be checked inside the main thread event loop, and
will push the main thread to process it with PostThreadMessage if the event
loop is waiting for input. And then it wait for response_mutex.

Once the main thread wakes up, it handles the response and re-acquires the
response_mutex, see how in thread_release_response.

XXX: WM_NOOP that supposed to wake
up the main thread is not delivered once in a while for reasons unknown.
Main thread queue full?

*/
static Bool
thread_respond( ThreadStorage *t, int cmd, int param1, intptr_t param2)
{
	t-> cmd    = cmd;
	t-> param1 = param1;
	t-> param2 = param2;

	/* don't let main thread to handle response until response_is_ready is 1
	and eventual Sleep() is over */
	LOG("%s: thread_respond: acquiring permissions to change", THR_ID(t));
	if ( !thread_begin_change(t))
		return false;
	{
		t-> response_is_ready = 1;
		if ( thread_noop_flag == 0) { /* don't trash the main queue with my messages */
			thread_noop_flag = 1;
			LOG("%s: thread_respond: sending WM_NOOP", THR_ID(t));
			while ( !PostThreadMessage( guts.main_thread_id, WM_NOOP, 0, 0))
				Sleep(1);
		}
	}
	thread_end_change(t);

	LOG("%s: thread_respond: wait for main thread to relinquish response_mutex", THR_ID(t));

	/* new wait for the main thread to handle the event */
	if ( !MUTEX_TAKE(t-> response_mutex))
		return false;
	/*

	the event is handled here

	*/
	mutex_release(t-> response_mutex);
	/* now don't run off until the main thread says so */
	LOG("%s: thread_respond: waiting for flag..", THR_ID(t));
	while (t-> response_is_ready) Sleep(1);
	LOG("%s: thread_respond: end sync", THR_ID(t));
	return true;
}

static Bool
thread_enter_control( ThreadStorage *t)
{
	if ( t-> rehashing ) {
		th.rehashing = 1;
		return false;
	}

	if ( t->thread_id && !thread_begin_change( t))
		return false;

	return true;
}

static Bool
thread_leave_control( ThreadStorage *t, void * proc)
{
	if ( !t->thread_id) {
		t->change_is_ready = false;
		if ( !( t->thread_handle = CreateThread( NULL, 0, proc, NULL, 0, &t->thread_id ))) {
			apiErr;
			return false;
		}
		LOG("main: create thread %d", t->thread_id);
	} else
		thread_end_change( t);
	t-> change_is_ready = true;
	return true;
}

DWORD WINAPI
socket_select( LPVOID dummy)
{
	int j, result;
	fd_set socket_set1[3];
	static int socket_commands[3] = { feRead, feWrite, feException};

	LOG("socket: started");
	while ( !prima_guts.app_is_dead) {
		if ( ts.change_is_ready) {
			int i, count;
			LOG("socket: change is ready");
			if ( !thread_begin_change(&ts))
				break;
			/* read incoming changes */
			for ( i = 0; i < 3; i++)
				memcpy( socket_set1+i, socket_set2+i, sizeof( fd_set));
			ts.change_is_ready = false;
			thread_end_change(&ts);
			count = socket_set1[0]. fd_count + socket_set1[1]. fd_count + socket_set1[2]. fd_count;
			LOG("socket: fd count=%d", count);
			/* stop the thread */
			if ( count == 0 )
				break;
		}

		/* wait for either control socket or data signal */
		LOG("socket: entering select...");
		result = select( FD_SETSIZE-1, &socket_set1[0], &socket_set1[1], &socket_set1[2], NULL);
		LOG("socket: select result=%d", result);

		if ( result == 0) continue;
		if ( result < 0) {
			socketErr("select");
			/* possibly some socket was closed? */
			ts.cmd = WM_SOCKET_REHASH;
			LOG("socket: WM_SOCKET_REHASH sent");
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
						LOG("socket: flushing control channel %ld", s);
						if ( recv( s, &r, 1, 0) != 1 ) {
							socketErr("recv");
							goto FAIL;
						}
					}
					continue;
				}
				LOG("socket: WM_SOCKET %d %ld sent", socket_commands[i], s);
				if ( !thread_respond( &ts, WM_SOCKET, socket_commands[j], s))
					goto FAIL;
			}
		}
		ts.change_is_ready = true;
	}
	LOG("socket: failed");
FAIL:
	LOG("socket: ended");

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
	Bool wakeup = ts.thread_id != 0;

	if ( !thread_enter_control( &ts))
		return;

	LOG("main: reset %d sockets", ts.objects.count);
	/* copying handles */
	for ( i = 0; i < 3; i++)
		FD_ZERO( &socket_set2[i]);

	for ( i = 0; i < ts.objects.count; i++) {
		Handle self = ts.objects.items[i];
		if ( var eventMask & feRead) {
			LOG("main: %s wants to read to %ld", var name, sys s.file.object);
			FD_SET( sys s.file.object,  &socket_set2[0]);
		}
		if ( var eventMask & feWrite) {
			LOG("main: %s wants to write to %ld", var name, sys s.file.object);
			FD_SET( sys s. file.object, &socket_set2[1]);
		}
		if ( var eventMask & feException) {
			LOG("main: %s wants to oob to %ld", var name, sys s.file.object);
			FD_SET( sys s.file.object,  &socket_set2[2]);
		}
	}

	/* set up the control channel to wake up the thread on change */
	if ( socket_control_channel[0] == 0 ) {
		if ( !prima_socketpair(socket_control_channel))
			return;
		LOG("main: socketpair: %ld %ld", socket_control_channel[0], socket_control_channel[1]);
	}
	FD_SET( socket_control_channel[1], &socket_set2[0]);

	/* don't bombard the control channel because the worker can be stuck waiting for
	the change mutex, and we won't release it in thread_leave_control below if this call
	is already deep in message handler change brackets. If that is indeed the case, waking
	is not needed because the worker is not in select(), but in mutex_take(), too */
	wakeup = ts.thread_id != 0 && ts.change_refcnt <= 1;

	if ( !thread_leave_control( &ts, socket_select))
		return;

	/* signal the thread */
	if ( wakeup ) {
		char s;
		LOG("main: sending wakeup call via control channel %ld", socket_control_channel[1]);
		if ( send( socket_control_channel[0], &s, 1, 0) != 1 )
			socketErr("send");
	}
}

DWORD WINAPI
syshandle_select( LPVOID dummy)
{
	int count = 0;
	DWORD n;
	HANDLE  syshandle_set1_handles[MAX_SYSHANDLES];
	Byte    syshandle_set1_types[MAX_SYSHANDLES];

	LOG("syshandle: started");
	while ( !prima_guts.app_is_dead) {
		if ( th.change_is_ready) {
			if ( !thread_begin_change(&th))
				break;
			/* read incoming changes */
			count = syshandle_set_count;
			memcpy( syshandle_set1_handles, syshandle_set2_handles, sizeof(HANDLE) * syshandle_set_count);
			memcpy( syshandle_set1_types,   syshandle_set2_types,   syshandle_set_count);
			th.change_is_ready = false;
			thread_end_change(&th);
			LOG("syshandle: got %d handles", count);
			if ( count == 0 ) /* stop the thread */
				break;
		}

		/* wait now, wake up by mutex if anything */
		LOG("syshandle: wait for %d objects", count);
		n = WaitForMultipleObjects(
			count, syshandle_set1_handles,
			false, INFINITE
		);
		LOG("syshandle: got %d", n);
		if ( n == WAIT_TIMEOUT )
			continue;

		if ( n >= WAIT_OBJECT_0 && n <= WAIT_OBJECT_0 + count - 1 ) {
			n -= WAIT_OBJECT_0;

			if (syshandle_set1_types[n] == MUTEX_SIGNATURE ) {
				LOG("syshandle: control mutex caught");
				mutex_release( syshandle_set1_handles[n] );
			}

			LOG("syshandle: sending WM_SYSHANDLE");
			if ( !thread_respond(&th, WM_SYSHANDLE, syshandle_set1_types[n], (intptr_t) syshandle_set1_handles[n]))
				goto FAIL;
		} else {
			/* some bad handle? rehash */
			LOG("syshandle: sending WM_SYSHANDLE_REHASH");
			if ( !thread_respond(&th, WM_SYSHANDLE_REHASH, 0, 0))
				goto FAIL;
		}
	}

FAIL:
	/* if somehow failed, making restart possible */
	LOG("syshandle: ended");
	th.thread_id = 0;
	th.response_is_ready = 0;
	return 0;
}

static void
reset_syshandles( void)
{
	int i;

	if ( !thread_enter_control( &ts))
		return;
	LOG("main: change syshandle");

	syshandle_set_count = 0;
	for ( i = 0; i < th.objects.count; i++) {
		Handle self = th.objects.items[i];
		switch ( sys s.file.type ) {
		case FHT_STDIN:
			if ( var eventMask & feRead) {
				syshandle_set2_handles[ syshandle_set_count ] = (HANDLE) sys s.file.object;
				syshandle_set2_types  [ syshandle_set_count ] = feRead;
				syshandle_set_count++;
				LOG("main: %s wants to read", var name);
				if ( syshandle_set_count == MAX_SYSHANDLES - 1) goto ENOUGH;
			}
			break;
		}
	}

	if ( syshandle_set_count > 0 ) {
		/* add controlling mutex */
		syshandle_set2_handles[ syshandle_set_count ] = syshandle_control_mutex;
		syshandle_set2_types  [ syshandle_set_count ] = MUTEX_SIGNATURE;
		syshandle_set_count++;
		LOG("main: add syshandle_control_mutex");
	}

ENOUGH:
	if ( !thread_leave_control( &th, syshandle_select))
		return;
	mutex_release( syshandle_control_mutex );
}

static void
rehash_files( ThreadStorage *t)
{
	int i;
	t-> rehashing = 2;
	for ( i = 0; i < t->objects.count; i++) {
		Handle self = t->objects.items[i];
		CFile( self)-> is_active( self, true);
	}

	if ( t-> rehashing == 1 ) {
		if ( t == &ts )
			reset_sockets();
		else if ( t == &th )
			reset_syshandles();
		t-> rehashing = 0;
	} else
		t-> rehashing = 0;
}

static void
dispatch_file_event( ThreadStorage *t)
{
	int i;
	int mask     = t->param1;
	intptr_t src = t->param2;
	for ( i = 0; i < t->objects.count; i++) {
		Handle self = t->objects.items[ i];
		if (
			( sys s.file.object == src) &&
			( var eventMask & mask )
		) {
			Event ev;
			LOG("main: dealing cmd=%s to %s",
				( mask == feRead) ? "cmFileRead" :
					(( mask == feWrite) ? "cmFileWrite" : "cmFileException"),
				var name
			);
			ev. cmd = ( mask == feRead) ? cmFileRead :
				(( mask == feWrite) ? cmFileWrite : cmFileException);
			CComponent(self)-> message( self, &ev);
			break;
		}
	}
}

Bool
file_process_events(int cmd, WPARAM param1, LPARAM param2)
{
	Bool events_handled = false;

	switch (cmd) {
	case WM_NOOP:
		thread_noop_flag = 0;
		LOG("main: WM_NOOP");
		break;
	case WM_FILE_REHASH:
		{
			int i;
			rehash_files(&tf);
			for ( i = 0; i < tf.objects.count; i++) {
				Handle self = tf.objects.items[i];
				if ( var eventMask & feRead)
					PostMessage( NULL, WM_FILE, feRead, ( LPARAM) self);
				if ( var eventMask & feWrite)
					PostMessage( NULL, WM_FILE, feWrite, ( LPARAM) self);
			}
			PostMessage( NULL, WM_FILE_REHASH, 0, 0);
		}
		break;
	case WM_FILE:
		tf.param1 = param1;
		tf.param2 = param2;
		dispatch_file_event(&tf);
		break;
	}

	if ( th.response_is_ready) {
		thread_noop_flag = 0;
		if ( thread_begin_change(&th)) {
			events_handled = true;

			switch ( th.cmd ) {
			case WM_SYSHANDLE:
				if ( th.param1 == MUTEX_SIGNATURE ) {
					LOG("main: retake syshandle control mutex");
					MUTEX_TAKE(syshandle_control_mutex);
				} else {
					LOG("main: WM_SYSHANDLE");
					dispatch_file_event(&th);
				}
				break;
			case WM_SYSHANDLE_REHASH:
				LOG("main: WM_SYSHANDLE_REHASH");
				rehash_files(&th);
				break;
			}

			thread_end_change( &th);
			thread_release_response(&th);
		}
	}

	if ( ts.response_is_ready) {
		thread_noop_flag = 0;
		if ( thread_begin_change(&ts)) {
			events_handled = true;
			switch ( ts.cmd ) {
			case WM_SOCKET:
				LOG("main: WM_SOCKET");
				dispatch_file_event(&ts);
				break;
			case WM_SOCKET_REHASH:
				LOG("main: WM_SOCKET_REHASH");
				rehash_files(&ts);
				break;
			}

			thread_end_change( &ts);
			thread_release_response(&ts);
		}
	}

	return events_handled;
}

static Bool
create_thread_storage( ThreadStorage *t)
{
	memset((void*) t, 0, sizeof(ThreadStorage));
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
		LOG("main: terminate %s thread", (t == &ts) ? "socket" : "syshandle");
		TerminateThread(t->thread_handle, 0);
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
	if ( !( syshandle_control_mutex = mutex_create()))
		return false;
	if ( !create_thread_storage(&ts))
		return false;
	if ( !create_thread_storage(&th))
		return false;
	list_create(&tf.objects, 8, 8);
	return true;
}

void
file_subsystem_done( void)
{
	list_destroy(&tf.objects);

	delete_thread_storage(&th);
	CloseHandle( syshandle_control_mutex );

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
		list_add( &th.objects, self);
		reset_syshandles();
		break;
	default:
		if ( tf.objects.count == 0)
			PostMessage( NULL, WM_FILE_REHASH, 0, 0);
		list_add( &tf.objects, self);
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
		list_delete( &th.objects, self);
		reset_syshandles();
		break;
	default:
		list_delete( &tf.objects, self);
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
