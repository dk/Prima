#include "unix/guts.h"

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  System dependent menu routines (unix, x11)             */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

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
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
}

void
apc_menu_item_set_accel( Handle self, PMenuItemReg m, char * accel)
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
apc_menu_item_set_key( Handle self, PMenuItemReg m, int key)
{
}

void
apc_menu_item_set_text( Handle self, PMenuItemReg m, char * text)
{
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

