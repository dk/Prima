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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "win32\win32guts.h"
#include "Window.h"
#include "Application.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle


static int
add_timer( Handle timerObject, Handle self)
{
   PItemRegRec pTime;
   int i;
   if ( timerObject == nilHandle) {
      apcErr( errInvObject);
      return 0;
   }

   if ( sys timeDefs) for ( i = 0; i < sys timeDefsCount; i++)
     if ( sys timeDefs[ i]. item == nil)
     {
        sys timeDefs[ i]. item = ( void*) timerObject;
        return i + 1;
     }

   if ( !(pTime = ( PItemRegRec) malloc (( sys timeDefsCount + 1) * sizeof( ItemRegRec))))
      return 0;

   if ( sys timeDefs) {
      memcpy( pTime, sys timeDefs, sys timeDefsCount* sizeof( ItemRegRec));
      free( sys timeDefs);
   }
   sys timeDefs = pTime;
   pTime += sys timeDefsCount++;
   pTime-> item = ( void*) timerObject;
   return sys timeDefsCount;
}

static void
remove_timer( Handle timerObject, Handle self)
{
   int i;
   PItemRegRec list = sys timeDefs;
   for ( i = 0; i < sys timeDefsCount; i++)
   {
      if (( Handle)( list-> item) == timerObject)
      {
          list-> item = nil;
          break;
      }
      list++;
   }
}

Bool
apc_timer_create( Handle self, Handle owner, int timeout)
{
   Bool reset = false;
   objCheck false;
   dobjCheck( owner) false;

   if (( DHANDLE( owner) != sys owner) && ( var handle != nilHandle) && is_opt( optActive)) {
      if ( !KillTimer(( HWND)(( PWidget) var owner)-> handle, var handle)) apiErr;
      remove_timer( self, var owner);
      reset = true;
   }
   sys owner = DHANDLE( owner);
   if ( !( var handle = add_timer( self, owner))) return false;
   sys s. timer. timeout = timeout;
   if ( reset) {
      if ( !SetTimer(( HWND)(( PWidget) owner)-> handle, var handle, sys s. timer. timeout, nil)) {
         opt_clear( optActive);
         apiErrRet;
      }
   }
   return true;
}

Bool
apc_timer_destroy( Handle self)
{
   objCheck false;
   if ( is_opt( optActive) && var handle && IsWindow(( HWND)(( PWidget) var owner)-> handle)) {
      if ( !KillTimer(( HWND)(( PWidget) var owner)-> handle, var handle)) apiErr;
   }
   remove_timer( self, var owner);
   return true;
}

int
apc_timer_get_timeout( Handle self)
{
   objCheck 0;
   return sys s. timer. timeout;
}

Bool
apc_timer_set_timeout( Handle self, int timeout)
{
   objCheck false;
   if ( is_opt( optActive)) {
      KillTimer(( HWND)(( PWidget) var owner)-> handle, var handle);
      if ( !SetTimer(( HWND)(( PWidget) var owner)-> handle, var handle, timeout, nil)) {
         opt_clear( optActive);
         apiErr;
         return false;
      }
   }
   sys s. timer. timeout = timeout;
   return true;
}

Bool
apc_timer_start( Handle self)
{
   objCheck false;
   if ( !SetTimer(( HWND)(( PWidget) var owner)-> handle, var handle, sys s. timer. timeout, nil))
      apiErrRet;
   return true;
}

Bool
apc_timer_stop( Handle self)
{
   objCheck false;
   KillTimer(( HWND)(( PWidget) var owner)-> handle, var handle);
   return true;
}

ApiHandle
apc_timer_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) var handle;
}

#ifdef __cplusplus
}
#endif
