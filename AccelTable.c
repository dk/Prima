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
#include "AccelTable.h"
#include "Widget.h"
#include <AccelTable.inc>
#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CAbstractMenu->
#define my  ((( PAccelTable) self)-> self)
#define var (( PAccelTable) self)

void
AccelTable_init( Handle self, HV * profile)
{
   dPROFILE;
   inherited init( self, profile);
   var-> system = false;
   my-> set_items( self, pget_sv( items));
   CORE_INIT_TRANSIENT(AccelTable);
}

void
AccelTable_set_items( Handle self, SV * menuItems)
{
   if ( var-> stage > csFrozen) return;
   my-> dispose_menu( self, var->  tree);
   var-> tree = ( PMenuItemReg) my-> new_menu( self, menuItems, 0);
}

Bool
AccelTable_selected( Handle self, Bool set, Bool selected)
{
   if ( !set)
       return CWidget( var-> owner)-> get_accelTable( var-> owner) == self;
   if ( var-> stage > csFrozen)
      return false;
   if ( selected)
      CWidget( var-> owner)-> set_accelTable( var->  owner, self);
   else if ( my-> get_selected( self))
      CWidget( var-> owner)-> set_accelTable( var->  owner, nilHandle);
   return false;
}


#ifdef __cplusplus
}
#endif
