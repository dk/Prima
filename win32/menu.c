#include "win32\win32guts.h"
#include <commdlg.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "File.h"
#include "Menu.h"
#include "Image.h"
#include "Widget.h"
#include "Window.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

static void
map_tildas( WCHAR * buf, int len)
{
	int j;
	for ( j = 0; j < len; j++) {
		if ( buf[ j] == '~') {
			if ( buf[ j + 1] == '~')
				j++;
			else if ( buf[ j + 1])
				buf[ j] = '&';
			continue;
		} else if ( buf[ j] == '&') {
			memmove( buf + j + 1, buf + j, (len - j + 1) * sizeof( WCHAR));
			j++;
			len++;
			continue;
		}
	}
}


static WCHAR *
map_text_accel( PMenuItemReg i)
{
	char * c;
	int l1, l2 = 0, amps = 0;
	WCHAR *buf, *text, *accel = NULL;

	c = i-> text;
	while (*c++) if ( *c == '&') amps++;
	if ( i-> flags. utf8_text ) {
		text = alloc_utf8_to_wchar( i-> text, prima_utf8_length( i-> text), &l1);
	} else {
		l1 = strlen( i-> text);
		text = alloc_ascii_to_wchar( i-> text, l1);
	}

	if ( i-> accel ) {
		c = i-> accel;
		while (*c++) if ( *c == '&') amps++;
		if ( i-> flags. utf8_accel ) {
			accel = alloc_utf8_to_wchar( i-> accel, prima_utf8_length( i-> accel), &l2);
		} else {
			l2 = strlen( i-> accel);
			accel = alloc_ascii_to_wchar( i-> accel, l2);
		}
	}

	buf = malloc(sizeof(WCHAR) * (l1 + l2 + amps + 2));

	memcpy( buf, text, l1 * sizeof(WCHAR));
	free(text);

	if ( i->accel ) {
		buf[l1] = '\t';
		memcpy( buf + l1 + 1, accel, l2 * sizeof(WCHAR));
		free(accel);
		l2++;
	}

	buf[l1 + l2] = 0;
	map_tildas( buf, l1 + l2);

	return buf;
}

static HMENU
add_item( Bool menuType, Handle menu, PMenuItemReg i)
{
	MENUITEMINFOW menuItem;
	HMENU m;
	PMenuWndData mwd;
	PMenuItemReg first;

	if ( i == nil) return nil;

	if ( menuType)
		m = CreateMenu();
	else
		m = CreatePopupMenu();

	if ( !m) {
		apiErr;
		return nil;
	}
	mwd = ( PMenuWndData) malloc( sizeof( MenuWndData));
	if ( !mwd) {
		DestroyMenu( m);
		return nil;
	}
	mwd-> menu = menu;
	first      = i;
	hash_store( menuMan, &m, sizeof( void*), mwd);

	while ( i != nil)
	{
		memset( &menuItem, 0, sizeof( menuItem));
		menuItem. cbSize   = sizeof( menuItem);
		menuItem. fMask    = MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
		menuItem. fType    = 0;
		menuItem. fType   |= ( i-> flags. divider    ) ? MFT_SEPARATOR    : 0;
		menuItem. fType   |= ( i-> bitmap     ) ? MFT_BITMAP       : 0;
		menuItem. fType   |= ( i-> text       ) ? MFT_STRING       : 0;
		menuItem. fType   |= ( i-> flags. rightAdjust) ? MFT_RIGHTJUSTIFY : 0;
		menuItem. fState   = 0;
		menuItem. fState  |= ( i-> flags. checked )    ? MFS_CHECKED      : 0;
		menuItem. fState  |= ( i-> flags. disabled)    ? MFS_GRAYED       : 0;
		menuItem. wID      = i-> id + MENU_ID_AUTOSTART;
		menuItem. hSubMenu = add_item( menuType, menu, i-> down);
		if (!( i-> flags. divider && i-> flags. rightAdjust)) {
			if ( i-> text) {
				menuItem. dwTypeData = map_text_accel( i);
			} else if ( i-> bitmap && PObject( i-> bitmap)-> stage < csDead)
				menuItem. dwTypeData = ( LPWSTR) image_create_bitmap( i-> bitmap, NULL, NULL, BM_AUTO);
			InsertMenuItemW( m, -1, true, &menuItem);
			if ( i-> text && menuItem. dwTypeData) free( menuItem. dwTypeData);
		}
		menuItem. dwItemData = menu;
		i = i-> next;
	}
	mwd-> id = first-> id;
	return m;
}

Bool
apc_menu_create( Handle self, Handle owner)
{
	objCheck false;
	dobjCheck( owner) false;
	sys className = WC_MENU;
	sys owner     = DHANDLE( owner);
	return true;
}

static Bool clear_menus( PMenuWndData item, int keyLen, void * key, void * params)
{
	if ( item-> menu == ( Handle) params)
		hash_delete( menuMan, key, keyLen, true);
	return false;
}

Bool
apc_menu_destroy( Handle self)
{
	if ( var handle) {
		objCheck false;
		hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
		if ( IsMenu(( HMENU) var handle) && !DestroyMenu(( HMENU) var handle)) apiErrRet;
		return true;
	}
	return false;
}

PFont
apc_menu_default_font( PFont copyTo)
{
	*copyTo = guts. menuFont;
	copyTo-> pitch = fpDefault;
	return copyTo;
}

Color
apc_menu_get_color( Handle self, int index)
{
	return remap_color( index | var widgetClass, true);
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
	return apc_menu_default_font( font);
}

Bool
apc_menu_set_color( Handle self, Color color, int index)
{
	return true;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
	return true;
}

Bool
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
	PWindow owner = nil;
	Point size;
	Bool resize;
	objCheck false;
	dobjCheck( var owner) false;
	if (( resize = kind_of( var owner, CWindow) &&
		kind_of( self, CMenu) &&
		var stage <= csNormal &&
		((( PAbstractMenu) self)-> self)-> get_selected( self))) {
		owner = ( PWindow) var owner;
		size = owner-> self-> get_size( var owner);
	}

	// since GetMenuItemInfo does not work in NT, do not free menuMan entries here,
	// they'll be freed in apc_menu_destroy anyway.

	DeleteMenu(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
	DrawMenuBar( DHANDLE( var owner));
	if ( resize && apc_window_get_window_state( var owner) == wsNormal)
		owner-> self-> set_size( var owner, size);
	return true;
}

Bool
apc_menu_item_set_accel( Handle self, PMenuItemReg m)
{
	UINT flags;
	WCHAR * buf;

	if ( !var handle) return false;
	objCheck false;
	flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
	if ( flags == 0xFFFFFFFF) return false;

	if ( flags & MF_BITMAP)
		flags = ( flags & ~MF_BITMAP) | MF_STRING;

	apcErrClear;
	buf = map_text_accel( m);
	if ( !ModifyMenuW(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
						m-> id + MENU_ID_AUTOSTART, buf))
		apiErr;
	free( buf);
	return rc == errOk;
}

Bool
apc_menu_item_set_check( Handle self, PMenuItemReg m)
{
	DWORD res;
	if ( !var handle) return false;
	objCheck false;
	res = CheckMenuItem(( HMENU) var handle,
		m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( m-> flags. checked ? MF_CHECKED : MF_UNCHECKED));
	return res != 0xFFFFFFFF;
}

Bool
apc_menu_item_set_enabled( Handle self, PMenuItemReg m)
{
	DWORD res;
	if ( !var handle) return false;
	objCheck false;
	res = EnableMenuItem(( HMENU) var handle,
		m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | ( m-> flags. disabled ? MF_GRAYED : MF_ENABLED));
	DrawMenuBar( DHANDLE( var owner));
	return res != 0xFFFFFFFF;
}

Bool
apc_menu_item_set_key( Handle self, PMenuItemReg m)
{
	return true;
}

Bool
apc_menu_item_set_text( Handle self, PMenuItemReg m)
{
	WCHAR * buf;
	UINT flags;

	if ( !var handle) return false;
	objCheck false;
	flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
	if ( flags == 0xFFFFFFFF) return false;

	if ( flags & MF_BITMAP)
		flags = ( flags & ~MF_BITMAP) | MF_STRING;

	apcErrClear;
	buf = map_text_accel( m);
	if ( !ModifyMenuW(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
			m-> id + MENU_ID_AUTOSTART, (WCHAR*) buf)) apiErr;
	free( buf);
	return rc == errOk;
}

Bool
apc_menu_item_set_image( Handle self, PMenuItemReg m)
{
	UINT flags;

	if ( !var handle) return false;
	objCheck false;
	dobjCheck( m-> bitmap) false;
	flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
	if ( flags == 0xFFFFFFFF) return false;

	flags |= MF_BITMAP;

	if ( !ModifyMenuW(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, flags,
		m-> id + MENU_ID_AUTOSTART,
		( LPCWSTR) image_create_bitmap( m-> bitmap, NULL, NULL, BM_AUTO))) apiErrRet;
	return true;
}


ApiHandle
apc_menu_get_handle( Handle self)
{
	objCheck 0;
	return ( ApiHandle) var handle;
}

Bool
apc_menu_update( Handle self, PMenuItemReg oldBranch, PMenuItemReg newBranch)
{
	objCheck false;
	dobjCheck( var owner) false;

	if ( kind_of( var owner, CWindow) &&
		kind_of( self, CMenu) &&
		var stage <= csNormal &&
		((( PAbstractMenu) self)-> self)-> get_selected( self)) {
		HMENU h = GetMenu( DHANDLE( var owner));
		PWindow owner = ( PWindow) var owner;
		Point size = owner-> self-> get_size( var owner);
		if ( h) DestroyMenu( h);
		hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
		var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
		SetMenu( DHANDLE( var owner), self ? ( HMENU) var handle : nil);
		DrawMenuBar( DHANDLE( var owner));
		if ( apc_window_get_window_state( var owner) == wsNormal)
			owner-> self-> set_size( var owner, size);
	} else {
		if ( var handle)
			DestroyMenu(( HMENU) var handle);
		hash_first_that( menuMan, clear_menus, ( void *) self, nil, nil);
		var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
	}
	return true;
}

Bool
apc_popup_create( Handle self, Handle owner)
{
	objCheck false;
	dobjCheck( owner) false;
	sys owner = DHANDLE( owner);
	sys className = WC_MENU;
	return true;
}

PFont
apc_popup_default_font( PFont font)
{
	return apc_menu_default_font( font);
}

Bool
apc_popup( Handle self, int x, int y, Rect * anchor)
{
	TPMPARAMS tpm;
	HWND owner;
	Bool ret = true;
	objCheck false;

	if ( guts. popupActive) return false;

	y = dsys( var owner) lastSize. y - y;
	owner = ( HWND)(( PComponent) var owner)-> handle;
	if ( var owner != application) {
		POINT pt;
		pt. x = x;
		pt. y = y;
		if ( !MapWindowPoints( owner, nil, &pt, 1)) apiErr;
		x = pt.x;
		y = pt.y;
	}
	if ( anchor &&
		anchor-> left   &&
		anchor-> right  &&
		anchor-> top    &&
		anchor-> bottom
		)
	{
		RECT r;
		GetWindowRect( owner, &r);
		tpm. cbSize = sizeof( tpm);
		tpm. rcExclude. left   = anchor-> left;
		tpm. rcExclude. right  = anchor-> right;
		tpm. rcExclude. top    = r. bottom - r. top - anchor-> top;
		tpm. rcExclude. bottom = r. bottom - r. top - anchor-> bottom;
		if ( !MapWindowPoints( owner, nil, ( PPOINT) &tpm. rcExclude, 2)) apiErr;
	} else
		anchor = nil;

	guts. popupActive = 1;
	ret = TrackPopupMenuEx(
		( HMENU) var handle, TPM_LEFTBUTTON|TPM_LEFTALIGN|TPM_RIGHTBUTTON,
		x, y, owner, anchor ? &tpm : NULL);
	guts. popupActive = 0;
	return ret;
}


#ifdef __cplusplus
}
#endif
