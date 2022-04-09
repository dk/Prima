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
		((( PWindow) var-> owner)-> self)-> set_menu( var-> owner, NULL_HANDLE);
	if ( !pexist( owner)) return;
	if ( !apc_menu_create( self, xOwner))
		croak("Cannot create menu");
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
		CWindow( var-> owner)-> set_menu( var-> owner, NULL_HANDLE);
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
