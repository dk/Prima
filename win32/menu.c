#include "win32\win32guts.h"
#include <commdlg.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "File.h"
#include "Menu.h"
#include "Icon.h"
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
		text = alloc_utf8_to_wchar( i-> text, prima_utf8_length( i-> text, -1), &l1);
	} else {
		l1 = strlen( i-> text);
		text = alloc_ascii_to_wchar( i-> text, l1);
	}

	if ( i-> accel ) {
		c = i-> accel;
		while (*c++) if ( *c == '&') amps++;
		if ( i-> flags. utf8_accel ) {
			accel = alloc_utf8_to_wchar( i-> accel, prima_utf8_length( i-> accel, -1), &l2);
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

/* Convert 1-bit icons to bitmaps, icons to argb, others to pixmaps */
static HBITMAP
create_menu_bitmap( Handle bitmap )
{
	HBITMAP ret = NULL;
	PIcon i = (PIcon) bitmap;

	if ( i == NULL || i-> stage >= csDead )
		return NULL;

	if (kind_of(bitmap, CIcon)) {
		if (( i-> type & imBPP) == 1) {
			ret = image_create_bitmap( bitmap);
		} else {
			Handle dup = CImage(bitmap)->dup(bitmap);
			CIcon(dup)->set_autoMasking(dup, amNone);
			if ( i-> type != imRGB )
				CIcon(dup)->set_type(dup, imRGB);
			if ( i-> maskType != 8 )
				CIcon(dup)->set_maskType(dup, 8);
			CIcon(dup)-> premultiply_alpha(dup, NULL);
			ret = image_create_bitmap( dup);
			Object_destroy(dup);
		}
	} else {
		if (( i-> type & imBPP) == 1) {
			Handle dup = CImage(bitmap)->dup(bitmap);
			CIcon(dup)->set_type(dup, 4);
			ret = image_create_bitmap( bitmap);
			Object_destroy(dup);
		}
	}

	if ( ret == NULL )
		ret = image_create_bitmap( bitmap);

	return ret;
}

typedef struct {
	HMENU menu;
	int   id;
} BitmapKey;

static void
build_bitmap_key( HMENU menu, PMenuItemReg m, BitmapKey * key)
{
	memset(key, 0, sizeof(BitmapKey));
	key->menu = menu;
	key->id   = m->id;
}

static HBITMAP
unchecked_bitmap(void)
{
	int cx, x1, y1, x2, y2, i, sz;
	HDC dc;
	uint32_t * ptr;
	HPEN stock_pen;
	HPEN stock_brush;
	HBITMAP stock_bm;

	if ( uncheckedBitmap == (HBITMAP)(-1)) return NULL;
	if ( uncheckedBitmap != NULL) return uncheckedBitmap;

	uncheckedBitmap = (HBITMAP) -1;

	cx = GetSystemMetrics( SM_CXMENUCHECK ) - 1;
	dc = GetDC(NULL);
	if ( !( uncheckedBitmap = image_create_argb_dib_section( dc, cx, cx, &ptr))) {
		ReleaseDC(NULL, dc);
		return NULL;
	}
	ReleaseDC(NULL, dc);

	dc = CreateCompatibleDC(NULL);
	stock_bm = SelectObject( dc, uncheckedBitmap);

	sz = cx * cx;
	bzero(ptr, sz * 4);
	x1 = y1 = (cx > 10) ? cx / 4 : 0;
	x2 = y2 = cx - x1;

	stock_brush = SelectObject( dc, hBrushHollow);

	stock_pen = SelectObject( dc, CreatePen( PS_SOLID, 0, 0x404040));
	MoveToEx( dc, x1, y2, NULL);
	LineTo( dc, x2, y2);
	LineTo( dc, x2, y1);

	DeleteObject( SelectObject( dc, CreatePen( PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW))));
	MoveToEx( dc, x2, y1, NULL);
	LineTo( dc, x1, y1);
	LineTo( dc, x1, y2 + 1);

	x1++; y1++; x2--; y2--;
	MoveToEx( dc, x1, y2, NULL);
	LineTo( dc, x2, y2);
	LineTo( dc, x2, y1);

	DeleteObject( SelectObject( dc, CreatePen( PS_SOLID, 0, GetSysColor(COLOR_BTNHIGHLIGHT))));
	MoveToEx( dc, x2, y1, NULL);
	LineTo( dc, x1, y1);
	LineTo( dc, x1, y2 + 1);

	for ( i = 0; i < sz; i++, ptr++)
		if ( *ptr )
			*ptr |= 0xff000000;

	DeleteObject(SelectObject( dc, stock_brush ));
	SelectObject( dc, stock_pen );
	SelectObject(dc, stock_bm);
	DeleteDC(dc);

	return uncheckedBitmap;
}

static HMENU
add_item( Bool menuType, Handle menu, PMenuItemReg i)
{
	MENUITEMINFOW mii;
	HMENU m;
	PMenuWndData mwd;
	PMenuItemReg first;

	if ( i == NULL) return NULL;

	if ( menuType)
		m = CreateMenu();
	else
		m = CreatePopupMenu();

	if ( !m) {
		apiErr;
		return NULL;
	}
	mwd = ( PMenuWndData) malloc( sizeof( MenuWndData));
	if ( !mwd) {
		DestroyMenu( m);
		return NULL;
	}
	mwd-> menu = menu;
	first      = i;
	hash_store( menuMan, &m, sizeof(HMENU), mwd);

	while ( i != NULL)
	{
		bzero( &mii, sizeof(mii));
		mii.cbSize   = sizeof(mii);
		mii.fMask    = MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE | MIIM_ID | MIIM_DATA;

		mii.fType    = 0;
		mii.fType   |= ( i-> flags. divider    ) ? MFT_SEPARATOR    : 0;
		mii.fType   |= ( i-> bitmap            ) ? MFT_BITMAP       : 0;
		mii.fType   |= ( i-> text              ) ? MFT_STRING       : 0;
		mii.fType   |= ( i-> flags. rightAdjust) ? MFT_RIGHTJUSTIFY : 0;
		mii.fType   |= ( i-> flags. custom_draw) ? MFT_OWNERDRAW    : 0;

		mii.fState   = 0;
		mii.fState  |= ( i-> flags. checked    ) ? MFS_CHECKED      : 0;
		mii.fState  |= ( i-> flags. disabled   ) ? MFS_GRAYED       : 0;

		mii.wID      = i-> id + MENU_ID_AUTOSTART;
		mii.hSubMenu = add_item( menuType, menu, i-> down);
		mii.dwItemData = menu;

		if (!( i-> flags. divider && i-> flags. rightAdjust)) {
			if ( i-> text)
				mii. dwTypeData = map_text_accel( i);
			else if ( i-> bitmap )
				mii. dwTypeData = (WCHAR*) create_menu_bitmap( i-> bitmap);
			if ( i-> icon ) {
				BitmapKey key;
				HBITMAP bitmap = create_menu_bitmap( i-> icon);
				build_bitmap_key(m, i, &key);
				hash_store( menuBitmapMan, &key, sizeof(key), (void*) bitmap);
				mii. fMask |= MIIM_CHECKMARKS;
				mii. hbmpChecked = mii. hbmpUnchecked = bitmap;
			} else if ( i-> flags. autotoggle ) {
				mii. fMask |= MIIM_CHECKMARKS;
				mii. hbmpUnchecked = unchecked_bitmap();
			}
			InsertMenuItemW( m, -1, true, &mii);
			if ( i-> text && mii.dwTypeData)
				free( mii. dwTypeData);
		}

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

static Bool
clear_menus( PMenuWndData item, int keyLen, void * key, void * params)
{
	if ( item-> menu == ( Handle) params)
		hash_delete( menuMan, key, keyLen, true);
	return false;
}

static Bool
clear_bitmaps(void * bm, int keyLen, BitmapKey * key, void * menu)
{
	if ( key-> menu == ( HMENU) menu)
		hash_delete( menuBitmapMan, key, keyLen, true);
	return false;
}

Bool
apc_menu_destroy( Handle self)
{
	if ( var handle) {
		objCheck false;
		hash_first_that( menuMan, clear_menus, ( void *) self, NULL, NULL);
		hash_first_that( menuBitmapMan, clear_bitmaps, ( void *) var handle, NULL, NULL);
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

static void
free_bitmaps( BitmapKey *key, PMenuItemReg m)
{
	if ( m-> icon ) {
		key-> id = m-> id;
		hash_delete( menuBitmapMan, &key, sizeof(key), false);
	}
	if ( m-> next )
		free_bitmaps( key, m-> next);
	if ( m-> down )
		free_bitmaps( key, m-> down);
}

static void
free_submenus(HMENU menu)
{
	int i, n;
	hash_delete( menuMan, &menu, sizeof(HMENU), true);
	n = GetMenuItemCount(menu);
	for ( i = 0; i < n; i++) {
		HMENU submenu = GetSubMenu(menu, i);
		if ( submenu )
			free_submenus(submenu);
	}
}

Bool
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
	PWindow owner = NULL;
	Point size;
	Bool resize;
	BitmapKey key;
	MENUITEMINFO mii;

	objCheck false;
	dobjCheck( var owner) false;
	if (( resize = kind_of( var owner, CWindow) &&
		kind_of( self, CMenu) &&
		var stage <= csNormal &&
		((( PAbstractMenu) self)-> self)-> get_selected( self))) {
		owner = ( PWindow) var owner;
		size = owner-> self-> get_size( var owner);
	}

	build_bitmap_key((HMENU) var handle, m, &key);
	free_bitmaps(&key, m);

	bzero(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask  = MIIM_SUBMENU;
	GetMenuItemInfo(( HMENU) var handle, m->id + MENU_ID_AUTOSTART, false, &mii);
	if (mii.hSubMenu) free_submenus(mii.hSubMenu);

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

static Bool
update_check_icons( Handle self, PMenuItemReg m)
{
	UINT flags;
	HBITMAP bitmap = NULL;
	BitmapKey key;
	Bool ret = true;
	MENUITEMINFO mii;

	if ( !var handle) return false;
	objCheck false;
	flags = GetMenuState(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND);
	if ( flags == 0xFFFFFFFF) return false;

	build_bitmap_key((HMENU) var handle, m, &key);
	hash_delete( menuBitmapMan, &key, sizeof(key), false);

	bzero(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask  = MIIM_CHECKMARKS;

	if ( m-> icon ) {
		if ( ( bitmap = create_menu_bitmap( m-> icon)) != NULL)
			hash_store( menuBitmapMan, &key, sizeof(key), (void*) bitmap);
		else {
			apiErr;
			ret = false;
		}
		mii.hbmpChecked = mii.hbmpUnchecked = bitmap;
	} else if ( m-> flags.autotoggle ) {
		mii. hbmpUnchecked = unchecked_bitmap();
	}


	return ret;
}

Bool
apc_menu_item_set_autotoggle( Handle self, PMenuItemReg m)
{
	return update_check_icons(self, m);
}

Bool
apc_menu_item_set_enabled( Handle self, PMenuItemReg m)
{
	DWORD res;
	if ( !var handle) return false;
	objCheck false;
	res = EnableMenuItem(( HMENU) var handle, m-> id + MENU_ID_AUTOSTART, MF_BYCOMMAND | (
		m-> flags. disabled ? MF_GRAYED : MF_ENABLED
	));
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
		( LPCWSTR) create_menu_bitmap( m-> bitmap))) apiErrRet;
	return true;
}

Bool
apc_menu_item_set_icon( Handle self, PMenuItemReg m)
{
	return update_check_icons(self, m);
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
		if ( h) {
			hash_first_that( menuBitmapMan, clear_bitmaps, ( void *) h, NULL, NULL);
			DestroyMenu( h);
		}
		hash_first_that( menuMan, clear_menus, ( void *) self, NULL, NULL);
		var handle = ( Handle) add_item( kind_of( self, CMenu), self, (( PMenu) self)-> tree);
		SetMenu( DHANDLE( var owner), self ? ( HMENU) var handle : NULL);
		DrawMenuBar( DHANDLE( var owner));
		if ( apc_window_get_window_state( var owner) == wsNormal)
			owner-> self-> set_size( var owner, size);
	} else {
		if ( var handle) {
			hash_first_that( menuBitmapMan, clear_bitmaps, ( void *) var handle, NULL, NULL);
			DestroyMenu(( HMENU) var handle);
		}
		hash_first_that( menuMan, clear_menus, ( void *) self, NULL, NULL);
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
	if ( var owner != prima_guts.application) {
		POINT pt;
		pt. x = x;
		pt. y = y;
		if ( !MapWindowPoints( owner, NULL, &pt, 1)) apiErr;
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
		if ( !MapWindowPoints( owner, NULL, ( PPOINT) &tpm. rcExclude, 2)) apiErr;
	} else
		anchor = NULL;

	guts. popupActive = 1;
	ret = TrackPopupMenuEx(
		( HMENU) var handle, TPM_LEFTBUTTON|TPM_LEFTALIGN|TPM_RIGHTBUTTON,
		x, y, owner, anchor ? &tpm : NULL);
	guts. popupActive = 0;
	return ret;
}

Bool
apc_menu_item_begin_paint( Handle self, PEvent event)
{
	objCheck false;
	apcErrClear;

	self = event->gen.H;
	sys transform2. x = 0;
	sys transform2. y = 0;
	apt_set( aptCompatiblePS);
	sys ps = (HDC) event->gen.p;
	sys lastSize = event->gen.P;
	sys s.menuitem.saved_dc = SaveDC(sys ps);
	hwnd_enter_paint( self);
	return true;
}

Bool
apc_menu_item_end_paint( Handle self, PEvent event)
{
	objCheck false;
	self = event->gen.H;
	hwnd_leave_paint( self);
	RestoreDC(sys ps, sys s.menuitem.saved_dc);
	sys ps = NULL;
	apt_clear( aptCompatiblePS);
	return true;
}


#ifdef __cplusplus
}
#endif
