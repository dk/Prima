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

#ifdef __cplusplus
}
#endif
