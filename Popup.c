#include "apricot.h"
#include "Popup.h"
#include "Widget.h"
#include "Popup.inc"

#undef  my
#define inherited CAbstractMenu->
#define my  ((( PPopup) self)-> self)->
#define var (( PPopup) self)->

void
Popup_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   opt_assign( optAutoPopup, pget_B( auto));
}

void
Popup_update_sys_handle( Handle self, HV * profile)
{
   Handle xOwner = pexist( owner) ? pget_H( owner) : var owner;
   if ( var owner && ( xOwner != var owner))
      ((( PWidget) var owner)-> self)-> set_popup( var owner, nilHandle);
   var anchored = true;
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
   ((( PWidget) var owner)-> self)-> set_popup( var owner, selected ? self : nilHandle);
}

Bool
Popup_get_selected( Handle self)
{
   return (((( PWidget) var owner)-> self)-> get_popup( var owner) == self);
}

void
Popup_popup( Handle self, int x, int y, int ancLeft, int ancBottom, int ancRight, int ancTop)
{
   int i;
   PWidget owner = ( PWidget) var owner;
   ColorSet color;
   Rect anchor = {ancLeft, ancBottom, ancRight, ancTop};
   int stage = owner-> stage;
   owner-> stage = csFrozen;
   memcpy( color, owner-> popupColor, sizeof( ColorSet));
   for ( i = 0; i < ciMaxId + 1; i++)
     apc_menu_set_color( self, color[ i], i);
   memcpy( owner-> popupColor, color, sizeof( ColorSet));
   apc_menu_set_font( self, &owner-> popupFont);
   owner-> stage = stage;
   apc_popup( self, x, y, &anchor);
}
