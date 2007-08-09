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

#include "apricot.h"
#include "Window.h"
#include "Menu.h"
#include <Menu.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CAbstractMenu->
#define my  ((( PMenu) self)-> self)
#define var (( PMenu) self)

void
Menu_update_sys_handle( Handle self, HV * profile)
{
   dPROFILE;
   Handle xOwner = pexist( owner) ? pget_H( owner) : var-> owner;
   var-> system = true;
   if ( var-> owner && ( xOwner != var-> owner))
      ((( PWindow) var-> owner)-> self)-> set_menu( var-> owner, nilHandle);
   if ( !pexist( owner)) return;
   if ( !apc_menu_create( self, xOwner))
      croak("RTC0060: Cannot create menu");
}

Bool
Menu_selected( Handle self, Bool set, Bool selected)
{
   if ( !set)
       return CWindow( var-> owner)-> get_menu( var->  owner) == self;
   if ( var-> stage > csFrozen)
      return false;
   if ( selected)
      CWindow( var-> owner)-> set_menu( var-> owner, self);
   else if ( my-> get_selected( self))
      CWindow( var-> owner)-> set_menu( var-> owner, nilHandle);
   return false;
}

Bool
Menu_validate_owner( Handle self, Handle * owner, HV * profile)
{
   dPROFILE;
   *owner = pget_H( owner);
   if ( !kind_of( *owner, CWindow)) return false;
   return inherited validate_owner( self, owner, profile);
}

#ifdef __cplusplus
}
#endif
