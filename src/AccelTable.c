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
