/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */
/* Created by: Dmitry Karasik <dk@plab.ku.dk> */
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#define INCL_DOSSEMAPHORES
#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include "os2/os2guts.h"
#include "Component.h"
#include "File.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PFile) self)->

static Bool            socketThreadStarted = false;
static Bool            socketSetChanged    = false;
static struct timeval  socketTimeout       = {0, 200000};
static char            socketErrBuf [ 256];

static fd_set socketSet1[3];
static fd_set socketSet2[3];
static int    fd_max;
static int    socketCommands[3] = { feRead, feWrite, feException};

static void
socket_select( void *dummy)
{
   while ( !appDead) {
      if ( socketSetChanged) {
         // updating  handles
         int i;
         if ( DosRequestMutexSem( guts. socketMutex, ( ULONG) SEM_INDEFINITE_WAIT) != NO_ERROR) {
             strcpy( socketErrBuf, "Failed to obtain socket mutex ownership for thread #2");
             WinPostQueueMsg( guts. queue, WM_CROAK, ( MPARAM) 1, ( MPARAM) &socketErrBuf);
             break;
         }
         for ( i = 0; i < 3; i++)
            memcpy( socketSet1+i, socketSet2+i, sizeof( fd_set));
         socketSetChanged = false;
         DosReleaseMutexSem( guts. socketMutex);
      }

      // calling select()
      if ( fd_max >= 0) {
         int i, j, result = select( fd_max + 1, &socketSet1[0], &socketSet1[1], &socketSet1[2], &socketTimeout);
         socketSetChanged = true;
         if ( result == 0) continue;
         if ( result < 0) {
            if ( errno == EINTR) continue;
            if ( errno == EBADF) {
               guts. socketPostSync = 1;
               WinPostQueueMsg( guts. queue, WM_SOCKET_REHASH, 0, 0);
               while( guts. socketPostSync) DosSleep(1);
            } else {
               strcpy( socketErrBuf, strerror( errno));
               WinPostQueueMsg( guts. queue, WM_CROAK, 0, ( MPARAM) &socketErrBuf);
            }
            continue;
         }
         // posting select() results
         for ( j = 0; j < 3; j++)
            for ( i = 0; i < FD_SETSIZE; i++) {
               if ( !FD_ISSET( i, socketSet1 + j)) continue;
               guts. socketPostSync = 1;
               WinPostQueueMsg( guts. queue, WM_SOCKET,
                   ( MPARAM) socketCommands[j], ( MPARAM) i);
               while( guts. socketPostSync) DosSleep(1);
            }
      } else
         // nothing to 'select', sleeping
         DosSleep( socketTimeout. tv_sec * 1000 + socketTimeout. tv_usec / 1000);
   }

   // if somehow failed, making restart possible
   socketThreadStarted = false;
}

static void
reset_sockets( void)
{
   int i;

   // enter section
   if ( socketThreadStarted) {
      if ( DosRequestMutexSem( guts. socketMutex, ( ULONG) SEM_INDEFINITE_WAIT) != NO_ERROR)
          croak("Failed to obtain socket mutex ownership for thread #1");
   }

   // copying handles
   for ( i = 0; i < 3; i++)
      FD_ZERO( &socketSet2[i]);

   fd_max = -1;

   for ( i = 0; i < guts. files. count; i++) {
      Handle self = guts. files. items[i];
      if (
           ( var eventMask & ( feRead | feWrite | feException)) &&
           ( fd_max < sys s. file)
         ) {
           fd_max = sys s. file;
           if ( var eventMask & feRead)
              FD_SET( sys s. file, &socketSet2[0]);
           if ( var eventMask & feWrite)
              FD_SET( sys s. file, &socketSet2[1]);
           if ( var eventMask & feException)
              FD_SET( sys s. file, &socketSet2[2]);
      }
   }

   socketSetChanged = true;

   // leave section and start the thread, if needed
   if ( !socketThreadStarted) {
      if ( DosCreateMutexSem( NULL, &guts. socketMutex, 0, FALSE) != NO_ERROR) {
         apiErr;
         croak("Failed to create socket mutex object");
      }
      guts. socketThread  = _beginthread( socket_select, NULL, 40960, NULL);
      socketThreadStarted = true;
   } else
      DosReleaseMutexSem( guts. socketMutex);
}

void
socket_rehash( void)
{
   int i;
   for ( i = 0; i < guts. files. count; i++) {
      Handle self = guts. files. items[i];
      CFile( self)-> is_active( self, true);
   }
}

Bool
apc_file_attach( Handle self)
{
   objCheck false;

   sys s. file = var fd;

   list_add( &guts. files, self);
   reset_sockets();

   return true;
}


Bool
apc_file_detach( Handle self)
{
   list_delete( &guts. files, self);
   reset_sockets();
   return true;
}

Bool
apc_file_change_mask( Handle self)
{
   reset_sockets();
   return true;
}

