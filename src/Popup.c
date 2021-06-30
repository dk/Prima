#include "apricot.h"
#include "Popup.h"
#include "Widget.h"
#include <Popup.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CAbstractMenu->
#define my  ((( PPopup) self)-> self)
#define var (( PPopup) self)

void
Popup_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	opt_assign( optAutoPopup, pget_B( autoPopup));
	CORE_INIT_TRANSIENT(Popup);
}

void
Popup_update_sys_handle( Handle self, HV * profile)
{
	dPROFILE;
	Handle xOwner = pexist( owner) ? pget_H( owner) : var-> owner;
	if ( var-> owner && ( xOwner != var-> owner))
		((( PWidget) var-> owner)-> self)-> set_popup( var-> owner, NULL_HANDLE);
	if ( !pexist( owner)) return;
	if ( !apc_popup_create( self, xOwner))
		croak("Cannot create popup");
	var-> system = true;
}


Bool
Popup_autoPopup( Handle self, Bool set, Bool autoPopup)
{
	if ( set)
		opt_assign( optAutoPopup, autoPopup);
	return is_opt( optAutoPopup);
}

Bool
Popup_selected( Handle self, Bool set, Bool selected)
{
	if ( !set)
		return CWidget( var-> owner)-> get_popup( var->  owner) == self;
	if ( var-> stage > csFrozen)
		return false;
	if ( selected)
		CWidget( var-> owner)-> set_popup( var-> owner, self);
	else if ( my-> get_selected( self))
		CWidget( var-> owner)-> set_popup( var-> owner, NULL_HANDLE);
	return false;
}

void
Popup_popup( Handle self, int x, int y, int ancLeft, int ancBottom, int ancRight, int ancTop)
{
	int i;
	PWidget owner = ( PWidget) var-> owner;
	ColorSet color;
	Rect anchor;
	int stage = owner-> stage;
	if ( var-> stage > csNormal) return;
	anchor. left = ancLeft;
	anchor. bottom = ancBottom;
	anchor. right = ancRight;
	anchor. top = ancTop;
	owner-> stage = csFrozen;
	memcpy( color, owner-> popupColor, sizeof( ColorSet));
	for ( i = 0; i < ciMaxId + 1; i++)
		apc_menu_set_color( self, color[ i], i);
	memcpy( owner-> popupColor, color, sizeof( ColorSet));
	apc_menu_set_font( self, &owner-> popupFont);
	owner-> stage = stage;
	apc_popup( self, x, y, &anchor);
}

#ifdef __cplusplus
}
#endif
