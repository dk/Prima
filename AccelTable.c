#include "apricot.h"
#include "AccelTable.h"
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
   my dispose_menu( self, var tree);
   var tree = my new_menu( self, menuItems, 0);
}

