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

/***********************************************************/
/*                                                         */
/*  System dependent menu routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

/* Menus & Pop-ups */
Bool
apc_menu_create( Handle self, Handle owner)
{
   return false;
}

void
apc_menu_destroy( Handle self)
{
}

PFont
apc_menu_default_font( PFont f)
{
   return apc_font_default( f);
}

Color
apc_menu_get_color( Handle self, int index)
{
   return 0;
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
   return nil;
}

void
apc_menu_set_color( Handle self, Color color, int index)
{
}

void
apc_menu_set_font( Handle self, PFont font)
{
}

void
apc_menu_update( Handle self, PMenuItemReg oldBranch, PMenuItemReg newBranch)
{
}

void
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
}

void
apc_menu_item_set_accel( Handle self, PMenuItemReg m, const char * accel)
{
}

void
apc_menu_item_set_check( Handle self, PMenuItemReg m, Bool check)
{
}

void
apc_menu_item_set_enabled( Handle self, PMenuItemReg m, Bool enabled)
{
}

void
apc_menu_item_set_image( Handle self, PMenuItemReg m, Handle image)
{
}

void
apc_menu_item_set_key( Handle self, PMenuItemReg m, int key)
{
}

void
apc_menu_item_set_text( Handle self, PMenuItemReg m, const char * text)
{
}

ApiHandle
apc_menu_get_handle( Handle self)
{
   return nilHandle;
}

Bool
apc_popup_create( Handle self, Handle owner)
{
   return false;
}

PFont
apc_popup_default_font( PFont f)
{
   return apc_font_default( f);
}

Bool
apc_popup( Handle self, int x, int y, Rect * anchor)
{
   return false;
}

