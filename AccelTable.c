#include "apricot.h"
#include "AccelTable.h"
#include "Widget.h"
#include "AccelTable.inc"

#undef  my
#define inherited CAbstractMenu->
#define my  ((( PAccelTable) self)-> self)->
#define var (( PAccelTable) self)->

void
AccelTable_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   my set_items( self, pget_sv( items));
}

void
AccelTable_set_items( Handle self, SV * menuItems)
{
   if ( var stage > csNormal) return;
   my dispose_menu( self, var tree);
   var tree = my new_menu( self, menuItems, 0);
}


void
AccelTable_set_selected( Handle self, Bool selected)
{
   inherited set_selected( self, selected);
   if ( selected)
      ((( PWidget) var owner)-> self)-> set_accel_table( var owner, self);
   else if ( my get_selected( self))
      ((( PWidget) var owner)-> self)-> set_accel_table( var owner, nilHandle);
}

Bool
AccelTable_get_selected( Handle self)
{
   return (((( PWidget) var owner)-> self)-> get_accel_table( var owner) == self);
}

