#include "apricot.h"
#include "Window.h"
#include "Menu.h"
#include "Menu.inc"

#undef  my
#define inherited CAbstractMenu->
#define my  ((( PMenu) self)-> self)->
#define var (( PMenu) self)->


void
Menu_done( Handle self)
{
   ((( PWindow) var owner)-> self)-> set_menu( var owner, nilHandle);
   inherited done( self);
}

void
Menu_update_sys_handle( Handle self, HV * profile)
{
   Handle xOwner = pexist( owner) ? pget_H( owner) : var owner;
   if ( var owner && ( xOwner != var owner)) ((( PWindow) var owner)-> self)-> set_menu( var owner, nilHandle);
   var anchored = true;
   if (!(
       pexist( owner) ||
       pexist( items)
    )) return;
   if ( pexist( items)) {
      my dispose_menu( self, var tree);
      var tree = my new_menu( self, pget_sv( items), 0);
   }
   if ( !apc_menu_create( self, xOwner))
      croak("RTC0060: Cannot create menu");
   pdelete( owner);
   pdelete( items);
}

void
Menu_set_selected( Handle self, Bool selected)
{
   inherited set_selected( self, selected);
   ((( PWindow) var owner)-> self)-> set_menu( var owner, selected ? self : nilHandle);
}

Bool
Menu_get_selected( Handle self)
{
   return (((( PWindow) var owner)-> self)-> get_menu( var owner) == self);
}
