/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
 */

#include "apricot.h"
#include "Popup.h"
#include "Widget.h"
#include <Popup.inc>

#undef  my
#define inherited CAbstractMenu->
#define my  ((( PPopup) self)-> self)
#define var (( PPopup) self)

void
Popup_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   opt_assign( optAutoPopup, pget_B( auto));
}

void
Popup_update_sys_handle( Handle self, HV * profile)
{
   Handle xOwner = pexist( owner) ? pget_H( owner) : var-> owner;
   if ( var-> owner && ( xOwner != var-> owner))
      ((( PWidget) var-> owner)-> self)-> set_popup( var-> owner, nilHandle);
   if ( !pexist( owner)) return;
   if ( !apc_popup_create( self, xOwner))
      croak("RTC0061: Cannot create popup");
   pdelete( owner);
}


Bool
Popup_get_auto( Handle self)
{
  return is_opt( optAutoPopup);
}

void
Popup_set_auto( Handle self, Bool autoPopup)
{
   opt_assign( optAutoPopup, autoPopup);
}

void
Popup_set_selected( Handle self, Bool selected)
{
   inherited set_selected( self, selected);
   if ( selected)
      ((( PWidget) var-> owner)-> self)-> set_popup( var-> owner, self);
   else if ( my-> get_selected( self))
      ((( PWidget) var-> owner)-> self)-> set_popup( var-> owner, nilHandle);
}

Bool
Popup_get_selected( Handle self)
{
   return (((( PWidget) var-> owner)-> self)-> get_popup( var-> owner) == self);
}

void
Popup_popup( Handle self, int x, int y, int ancLeft, int ancBottom, int ancRight, int ancTop)
{
   int i;
   PWidget owner = ( PWidget) var-> owner;
   ColorSet color;
   Rect anchor = {ancLeft, ancBottom, ancRight, ancTop};
   int stage = owner-> stage;
   if ( var-> stage > csNormal) return;
   owner-> stage = csFrozen;
   memcpy( color, owner-> popupColor, sizeof( ColorSet));
   for ( i = 0; i < ciMaxId + 1; i++)
     apc_menu_set_color( self, color[ i], i);
   memcpy( owner-> popupColor, color, sizeof( ColorSet));
   apc_menu_set_font( self, &owner-> popupFont);
   owner-> stage = stage;
   apc_popup( self, x, y, &anchor);
}
