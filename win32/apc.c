#include "win32\win32guts.h"
#include <commdlg.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "File.h"
#include "Image.h"
#include "Region.h"
#include "Application.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle
#define GET_REGION(obj) (&(dsys(obj)s.region))


Bool
apc_application_begin_paint ( Handle self)
{
	objCheck false;
	apcErrClear;
	if ( !( sys ps = dc_alloc())) apiErrRet;
	apt_set( aptWinPS);
	apt_set( aptCompatiblePS);
	hwnd_enter_paint( self);
	if (( sys pal = palette_create( self))) {
		SelectPalette( sys ps, sys pal, 0);
		RealizePalette( sys ps);
	}
	return true;
}


Bool
apc_application_begin_paint_info( Handle self)
{
	Bool ok = apc_application_begin_paint( self);
	objCheck false;
	if ( ok) {
		HRGN rgn = CreateRectRgn( 0, 0, 0, 0);
		SelectClipRgn( sys ps, rgn);
		DeleteObject( rgn);
	}
	return ok;
}

Bool
apc_application_create( Handle self)
{
	HWND h;
	RECT r;
	MSG msg;
	const WCHAR wnull = 0;

	objCheck false;

	// make sure that no leftover messages, esp.WM_QUIT, are floating around
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE));

	if ( !( h = CreateWindowExW( 0, L"GenericApp", &wnull, 0, 0, 0, 0, 0,
			NULL, NULL, guts. instance, NULL))) apiErrRet;
	sys handle = h;
	sys parent = sys owner = HWND_DESKTOP;
	SetWindowLongPtr( sys handle, GWLP_USERDATA, self);
	PostMessage( sys handle, WM_PRIMA_CREATE, 0, 0);
	sys className = WC_APPLICATION;
	// if ( !SetTimer( h, TID_USERMAX, 100, NULL)) apiErr;
	GetClientRect( h, &r);
	if ( !( var handle = ( Handle) CreateWindowExW( 0,  L"Generic", &wnull, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
		0, 0, r. right - r. left, r. bottom - r. top, h, NULL,
		guts. instance, NULL))) apiErrRet;
	SetWindowLongPtr(( HWND) var handle, GWLP_USERDATA, self);
	apt_set( aptEnabled);
	sys lastSize = apc_application_get_size( self);
	return true;
}

Bool
apc_application_close( Handle self)
{
	PostQuitMessage(0);
	return true;
}

Bool
apc_application_destroy( Handle self)
{
	objCheck false;
	SetWindowLongPtr( sys handle, GWLP_USERDATA, 0);
	if ( IsWindow( sys handle))  {
		if ( guts. mouseTimer) {
			guts. mouseTimer = 0;
			if ( !KillTimer( sys handle, TID_USERMAX)) apiErr;
		}
		if ( !DestroyWindow( sys handle)) apiErr;
	}
	PostThreadMessage( guts. mainThreadId, WM_TERMINATE, 0, 0);
	PostQuitMessage(0);
	prima_guts.application = NULL_HANDLE;
	return true;
}

Bool
apc_application_end_paint( Handle self)
{
	apcErrClear;
	objCheck false;
	hwnd_leave_paint( self);
	if ( sys pal) DeleteObject( sys pal);
	dc_free();
	apt_clear( aptWinPS);
	apt_clear( aptCompatiblePS);
	sys pal = NULL;
	sys ps = NULL;
	return true;
}

Bool
apc_application_end_paint_info( Handle self)
{
	return apc_application_end_paint( self);
}


int
apc_application_get_gui_info( char * description, int len1, char * language, int len2)
{
	if ( description)
		strlcpy( description, "Windows", len1);
	if ( language ) {
		ULONG n_lang, n_words = 128;
		WORD buffer[128];
		if ( my_GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &n_lang, buffer, &n_words)) {
			if ( len2 < n_words ) n_words = len2;
			wchar2char( language, buffer, n_words );
		} else
			*language = 0;
	}
	return guiWindows;
}

Bool
apc_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
	HBITMAP bm, bm2;
	HDC dc, dc2;
	XLOGPALETTE lpg;
	HPALETTE hp, hp2, hp3;
	if ( image == NULL_HANDLE) apcErrRet( errInvParams);
	dobjCheck( image) false;


	apcErrClear;
	if (!( dc  = dc_alloc())) return false;
	lpg. palNumEntries = GetSystemPaletteEntries( dc, 0, 256, lpg. palPalEntry);
	lpg. palVersion = 0x300;

	hp  = CreatePalette(( LOGPALETTE*)&lpg);
	dc2 = CreateCompatibleDC( dc);
	if ( !dc2) {
		DeleteObject( hp);
		dc_free();
		return false;
	}
	hp2 = SelectPalette( dc2, hp, 0);
	RealizePalette( dc2);
	hp3 = SelectPalette( dc, hp, 1);

	bm  = CreateCompatibleBitmap( dc, xLen, yLen);
	if ( !bm) {
		SelectPalette( dc, hp3, 1);
		SelectPalette( dc2, hp2, 1);
		DeleteObject( hp);
		dc_free();
		return false;
	}
	bm2 = SelectObject( dc2, bm);
	BitBlt( dc2, 0, 0, xLen, yLen, dc, x, sys lastSize.y - y - yLen, SRCCOPY);
	SelectObject( dc2, bm2);
	SelectPalette( dc2, hp2, 1);
	DeleteObject( hp);
	DeleteDC( dc2);

	bm2 = dsys(image)bm;
	dsys(image)bm = bm;
	image_query_bits( image, true);
	dsys(image)bm = bm2;
	DeleteObject( bm);
	SelectPalette( dc, hp3, 1);
	dc_free();

	apc_image_update_change( image);

	return true;
}


Handle
hwnd_to_view( HWND win)
{
	Handle h;
	LONG_PTR ll;
	if (( !win) || ( !IsWindow( win)))
		return NULL_HANDLE;
	if ( GetWindowThreadProcessId( win, NULL) != guts. mainThreadId)
		return NULL_HANDLE;
	h = GetWindowLongPtr( win, GWLP_USERDATA);
	if ( !h) return NULL_HANDLE;
	ll = GetWindowLongPtr( win, GWLP_WNDPROC);
	if (
		( ll == ( LONG_PTR) generic_view_handler) ||
		( ll == ( LONG_PTR) generic_app_handler) ||
		( ll == ( LONG_PTR) generic_frame_handler)
		) return h;

	if ( SendMessage( win, WM_HASMATE, 0, ( LPARAM) &h) == (LRESULT) HASMATE_MAGIC)
		return h;
	return NULL_HANDLE;
}


int
apc_application_get_os_info( char *system, int slen,
									char *release, int rlen,
									char *vendor, int vlen,
									char *arch, int alen)
{
	SYSTEM_INFO si;
	OSVERSIONINFO os = { sizeof( OSVERSIONINFO)};
	DWORD  version;
	GetSystemInfo( &si);
	version = GetVersion();
	GetVersionEx( &os);
	if ( system)
		strlcpy( system, "Windows NT", slen);
	if ( vendor)
		strlcpy( vendor, "Microsoft", vlen);
	if ( arch) {
		char * pb = "Unknown";
#if defined( __BORLANDC__) && ! ( defined( __cplusplus) || defined( _ANONYMOUS_STRUCT))
		switch ( si. u. s. wProcessorArchitecture) {
#else
		switch ( si. wProcessorArchitecture) {
#endif
		case PROCESSOR_ARCHITECTURE_INTEL :   pb = "i386";  break;
		case PROCESSOR_ARCHITECTURE_MIPS  :   pb = "MIPS";  break;
		case PROCESSOR_ARCHITECTURE_ALPHA :   pb = "Alpha"; break;
		case PROCESSOR_ARCHITECTURE_PPC   :   pb = "PPC";   break;
		}
		strlcpy( arch, pb, alen);
	}
	if ( release)
		snprintf( release, rlen, "%d.%d",
			LOBYTE( LOWORD( version)),
			HIBYTE( LOWORD( version)));
	return apcWin32;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
	return hwnd_to_view(( HWND) apiHandle);
}

Rect
apc_application_get_indents( Handle self)
{
	Point size;
	UINT rc;
	Rect ret = {0,0,0,0};
	APPBARDATA d;

	size = apc_application_get_size( self);

	memset( &d, 0, sizeof(d));
	d. cbSize = sizeof(d);
	rc = SHAppBarMessage( ABM_GETSTATE, &d);
	if (( rc & ABS_AUTOHIDE) == 0) {
		memset( &d, 0, sizeof(d));
		d. cbSize = sizeof(d);
		rc = SHAppBarMessage( ABM_GETTASKBARPOS, &d);
		switch ( d. uEdge) {
		case ABE_TOP:
			ret. top = d. rc. bottom;
			if ( ret. top < 0) ret. top = 0;
			break;
		case ABE_BOTTOM:
			ret. bottom = size. y - d. rc. top;
			if ( ret. bottom < 0) ret. bottom = 0;
			break;
		case ABE_RIGHT:
			ret. right = size. x - d. rc. left;
			if ( ret. right < 0) ret. right = 0;
			break;
		case ABE_LEFT:
			ret. left = d. rc. right;
			if ( ret. left < 0) ret. left = 0;
			break;
		}
	}

	return ret;
}

Point
apc_application_get_size( Handle self)
{
	RECT  r;
	Point ret = {0,0};
	objCheck ret;
	GetWindowRect( HWND_DESKTOP, &r);
	ret. x = r. right;
	ret. y = r. bottom;
	return ret;
}

#define PRIMA_MAX_MONITORS 1024
typedef struct {
	int nrects;
	int max_height;
	Box rects[PRIMA_MAX_MONITORS];
} EnumMonitorData;

static BOOL
_enum_monitors( HMONITOR monitor, HDC dc, LPRECT rect, LPARAM data)
{
	EnumMonitorData * d;
	Box * current;

	d = ( EnumMonitorData * ) data;
	if ( d-> nrects >= PRIMA_MAX_MONITORS ) return false;
	current = &d->rects[ d->nrects ];

	current-> x = rect-> left;
	current-> y = rect-> bottom; /* fixup later */
	current-> width = rect-> right - rect-> left;
	current-> height = rect-> bottom - rect-> top;
	if ( d-> max_height < rect-> bottom ) d-> max_height = rect-> bottom;
	d->nrects++;

	return true;
}

Box *
apc_application_get_monitor_rects( Handle self, int * nrects)
{
	int i;
	Box * ret;
	EnumMonitorData d = {0,0};

	EnumDisplayMonitors( NULL, NULL, (MONITORENUMPROC) _enum_monitors, (LPARAM) &d);
	if ( d. nrects == 0) return NULL;
	if (!(ret = malloc( d.nrects * sizeof(Box) )))
		return NULL;

	for ( i = 0; i < d.nrects; i++)
		d.rects[i]. y = d.max_height - d.rects[i]. y;
	memcpy( ret, d.rects, d.nrects * sizeof(Box));
	*nrects = d.nrects;
	return ret;
}

int
apcUpdateWindow( HWND win )
{
	int ret;
	ret = UpdateWindow( win );
	exception_check_raise();
	return ret;
}

static Bool
files_rehash( Handle self, void * dummy)
{
	CFile( self)-> is_active( self, true);
	return false;
}

Bool
process_msg( MSG * msg)
{
	Bool postpone_msg_translation = false;
	switch ( msg-> message)
	{
	case WM_TERMINATE:
	case WM_QUIT:
		return false;
	case WM_CROAK:
		if ( msg-> wParam)
			croak("%s", ( char *) msg-> lParam);
		else
			warn("%s", ( char *) msg-> lParam);
		return true;
	case WM_SYSKEYDOWN:
		/*
			If Prima handles an Alt-Key combination that is also handled by a menu
			in TranslateMessage(), we need to prevent the message from being
			processed by the menu, by setting guts.dont_xlate_message flag.
		*/
		postpone_msg_translation = true;
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
		GetKeyboardState( guts. keyState);
		break;
	case WM_KEYPACKET: {
		KeyPacket * kp = ( KeyPacket *) msg-> lParam;
		BYTE * mod = mod_select( kp-> mod);
		Bool wui = P_APPLICATION-> wantUnicodeInput;
		P_APPLICATION-> wantUnicodeInput = kp-> mod & kmUnicode;
		SendMessage( kp-> wnd, kp-> msg, kp-> mp1, kp-> mp2);
		mod_free( mod);
		P_APPLICATION->wantUnicodeInput = wui;
		exception_check_raise();
		break;
	}
	case WM_LBUTTONDOWN:  musClk. emsg = WM_LBUTTONUP; goto MUS1;
	case WM_MBUTTONDOWN:  musClk. emsg = WM_MBUTTONUP; goto MUS1;
	case WM_RBUTTONDOWN:  musClk. emsg = WM_RBUTTONUP; goto MUS1;
	case WM_XBUTTONDOWN:  musClk. emsg = WM_XBUTTONUP; goto MUS1;
	MUS1:
		musClk. pending = 1;
		musClk. msg     = *msg;
		musClk. msg. wParam &=  MK_CONTROL|MK_SHIFT;
		break;
	case WM_LBUTTONUP:   musClk. msg. message = WM_LMOUSECLICK; goto MUS2;
	case WM_MBUTTONUP:   musClk. msg. message = WM_MMOUSECLICK; goto MUS2;
	case WM_RBUTTONUP:   musClk. msg. message = WM_RMOUSECLICK; goto MUS2;
	case WM_XBUTTONUP:   musClk. msg. message = WM_XMOUSECLICK; goto MUS2;
	MUS2:
		if ( musClk. pending &&
			( musClk. emsg         == msg-> message) &&
			( musClk. msg. hwnd    == msg-> hwnd)    &&
			( musClk. msg. wParam  == ( msg-> wParam & ( MK_CONTROL|MK_SHIFT))) &&
			( abs( musClk. msg. time  - msg-> time) < 200)
			)
			PostMessage( msg-> hwnd, musClk. msg. message, msg-> wParam, msg-> lParam);
		musClk. pending = 0;
		break;
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		musClk. pending = 0;
		break;
	case WM_SOCKET: {
		int i;
		SOCKETHANDLE socket = ( SOCKETHANDLE) msg-> lParam;
		for ( i = 0; i < guts. sockets. count; i++) {
			Handle self = guts. sockets. items[ i];
			if (( sys s. file. object == socket) &&
				( PFile( self)-> eventMask & msg-> wParam)) {
				Event ev;
				ev. cmd = ( msg-> wParam == feRead) ? cmFileRead :
					(( msg-> wParam == feWrite) ? cmFileWrite : cmFileException);
				CComponent( self)-> message( self, &ev);
				break;
			}
		}
		guts. socketPostSync = 0; // clear semaphore
		return true;
	}
	case WM_SOCKET_REHASH:
		socket_rehash();
		guts. socketPostSync = 0; // clear semaphore
		return true;
	case WM_FILE:
		if ( msg-> wParam == 0) {
			int i;

			if ( guts. files. count == 0) return true;

			list_first_that( &guts. files, files_rehash, NULL);
			for ( i = 0; i < guts. files. count; i++) {
				Handle self = guts. files. items[i];
				if ( PFile( self)-> eventMask & feRead)
					PostMessage( NULL, WM_FILE, feRead, ( LPARAM) self);
				if ( PFile( self)-> eventMask & feWrite)
					PostMessage( NULL, WM_FILE, feWrite, ( LPARAM) self);
			}
			PostMessage( NULL, WM_FILE, 0, 0);
		} else {
			int i;
			Handle self = NULL_HANDLE;
			for ( i = 0; i < guts. files. count; i++)
				if (( guts. files. items[i] == ( Handle) msg-> lParam) &&
					( PFile(guts. files. items[i])-> eventMask & msg-> wParam)) {
					self = ( Handle) msg-> lParam;
					break;
				}
			if ( self) {
				Event ev;
				ev. cmd = ( msg-> wParam == feRead) ? cmFileRead : cmFileWrite;
				CComponent( self)-> message( self, &ev);
			}
		}
		return true;
	}
	if ( !postpone_msg_translation)
		TranslateMessage( msg);
	DispatchMessage(msg);
	exception_check_raise();
	if ( postpone_msg_translation) {
		if ( guts. dont_xlate_message)
			guts. dont_xlate_message = false;
		else
			TranslateMessage( msg);
	}
	prima_kill_zombies();
	return true;
}

Bool
apc_application_go( Handle self)
{
	MSG msg;
	objCheck false;

	guts. application_stop_signal = false;
	while ( !guts. application_stop_signal && GetMessage( &msg, NULL, 0, 0) && process_msg( &msg));
	guts. application_stop_signal = false;
	return true;
}

Bool
HWND_lock( Bool lock)
{
	if ( lock)
	{
		if ( guts. appLock++ == 0) return LockWindowUpdate( HWND_DESKTOP);
	}
	else
	{
		if ( --guts. appLock == 0) return LockWindowUpdate( NULL);
	}
	return true;
}

Bool
apc_application_lock( Handle self)
{
	return HWND_lock( true);
}

Bool
apc_application_unlock( Handle self)
{
	return HWND_lock( false);
}

Bool
apc_application_stop( Handle self)
{
	if ( prima_guts.application == NULL_HANDLE ) return false;
	guts. application_stop_signal = true;
	return true;
}

Bool
apc_application_sync( void)
{
	return true;
}

Bool
apc_application_yield(Bool wait_for_event)
{
	MSG msg;
	Bool got_events = false;
	guts. application_stop_signal = false;
	while ( !guts. application_stop_signal && PeekMessage( &msg, NULL, 0, 0, PM_REMOVE)) {
		got_events = true;
		if ( !process_msg( &msg)) {
			PostThreadMessage( guts. mainThreadId, prima_guts.app_is_dead ? WM_QUIT : WM_TERMINATE, 0, 0);
			return false;
		}
	}
	if ( prima_guts.application && wait_for_event && !got_events && !guts. application_stop_signal) {
		Event ev;
		ev. cmd = cmIdle;
		C_APPLICATION-> message( prima_guts.application, &ev);
		if ( prima_guts.application ) {
			GetMessage( &msg, NULL, 0, 0);
			process_msg( &msg);
		}
	}
	guts. application_stop_signal = false;
	return prima_guts.application != NULL_HANDLE;
}

Handle
apc_application_get_widget_from_point( Handle self, Point point)
{
	DWORD pid, tid;
	POINT pt;
	HWND  p;

	objCheck NULL_HANDLE;
	pt.x = point. x;
	pt.y = sys lastSize. y - point. y - 1;
	p    =  WindowFromPoint( pt);

	if ( p) {
		POINT xp = pt;
		MapWindowPoints( HWND_DESKTOP, p, &xp, 1);
		p = ChildWindowFromPointEx( p, xp, CWP_SKIPINVISIBLE);
	} else
		p = ChildWindowFromPointEx( HWND_DESKTOP, pt, CWP_SKIPINVISIBLE);

	if ( !p) return NULL_HANDLE;
	if ( !( tid = GetWindowThreadProcessId( p, &pid))) apiErr;
	if ( tid != guts. mainThreadId) return NULL_HANDLE;
	return ( Handle) GetWindowLongPtr( p, GWLP_USERDATA);
}

// Component
Bool
apc_component_create( Handle self)
{
	PComponent c = ( PComponent) self;
	PDrawableData d = ( PDrawableData) c-> sysData;

	objCheck false;

	if ( d) return false;
	d = ( PDrawableData) malloc( sizeof( DrawableData));
	if ( !d) return false;
	memset( d, 0, sizeof( DrawableData));
	c-> sysData = d;
	return true;
}

Bool
apc_component_destroy( Handle self)
{
	PComponent    c = ( PComponent) self;
	PDrawableData d = ( PDrawableData) c-> sysData;
	objCheck false;
	var handle = NULL_HANDLE;
	if ( d == NULL) return false;
	free( d);
	c-> sysData = NULL;
	return true;
}

Bool
apc_component_fullname_changed_notify( Handle self)
{
	return true;
}

// View attributes

int
apc_kbd_get_state( Handle self)
{
	return
		(( GetKeyState( VK_MENU)    < 0) ? kmAlt      : 0) |
		(( GetKeyState( VK_CONTROL) < 0) ? kmCtrl     : 0) |
		(( GetKeyState( VK_SHIFT)   < 0) ? kmShift    : 0);
}

Handle ctx_kb2VK[] = {
	kbNoKey       ,   0                 ,
	kbAltL        ,   VK_MENU           ,
	kbAltR        ,   VK_RMENU          ,
	kbCtrlL       ,   VK_CONTROL        ,
	kbCtrlR       ,   VK_RCONTROL       ,
	kbShiftL      ,   VK_SHIFT          ,
	kbShiftR      ,   VK_RSHIFT         ,
	kbBackspace   ,   VK_BACK           ,
	kbTab         ,   VK_TAB            ,
	kbPause       ,   VK_PAUSE          ,
	kbEsc         ,   VK_ESCAPE         ,
	kbSpace       ,   VK_SPACE          ,
	kbPgUp        ,   VK_PRIOR          ,
	kbPgDn        ,   VK_NEXT           ,
	kbEnd         ,   VK_END            ,
	kbHome        ,   VK_HOME           ,
	kbLeft        ,   VK_LEFT           ,
	kbUp          ,   VK_UP             ,
	kbRight       ,   VK_RIGHT          ,
	kbDown        ,   VK_DOWN           ,
	kbPrintScr    ,   VK_PRINT          ,
	kbInsert      ,   VK_INSERT         ,
	kbDelete      ,   VK_DELETE         ,
	kbEnter       ,   VK_RETURN         ,
	kbF1          ,   VK_F1             ,
	kbF2          ,   VK_F2             ,
	kbF3          ,   VK_F3             ,
	kbF4          ,   VK_F4             ,
	kbF5          ,   VK_F5             ,
	kbF6          ,   VK_F6             ,
	kbF7          ,   VK_F7             ,
	kbF8          ,   VK_F8             ,
	kbF9          ,   VK_F9             ,
	kbF10         ,   VK_F10            ,
	kbF11         ,   VK_F11            ,
	kbF12         ,   VK_F12            ,
	kbF13         ,   VK_F13            ,
	kbF14         ,   VK_F14            ,
	kbF15         ,   VK_F15            ,
	kbF16         ,   VK_F16            ,
	kbNumLock     ,   VK_NUMLOCK        ,
	kbScrollLock  ,   VK_SCROLL         ,
	kbCapsLock    ,   VK_CAPITAL        ,
	kbClear       ,   VK_CLEAR          ,
	kbSelect      ,   VK_SELECT         ,
	kbExecute     ,   VK_EXECUTE        ,
	kbSysRq       ,   VK_SNAPSHOT       ,
	endCtx
};

Handle ctx_kb2VK2[] = {
	kbBackspace   ,   VK_BACK           ,
	kbTab         ,   VK_TAB            ,
	kbEsc         ,   VK_ESCAPE         ,
	kbSpace       ,   VK_SPACE          ,
	kbEnter       ,   VK_RETURN         ,
	endCtx
};

Handle ctx_kb2VK3[] = {
	kbAltL        ,   kbAltR            ,
	kbShiftL      ,   kbShiftR          ,
	kbCtrlL       ,   kbCtrlR           ,
	endCtx
};

Bool
apc_message( Handle self, PEvent ev, Bool post)
{
	ULONG msg;
	USHORT mp1s = 0;
	objCheck false;
	switch ( ev-> cmd) {
	case cmPost:
		if (post)
			PostMessage(( HWND) var handle, WM_POSTAL, ( WPARAM) ev-> gen. H, ( LPARAM) ev-> gen. p);
		else {
			SendMessage(( HWND) var handle, WM_POSTAL, ( WPARAM) ev-> gen. H, ( LPARAM) ev-> gen. p);
			exception_check_raise();
		}
		break;
	case cmMouseMove:
		msg = WM_MOUSEMOVE;
		goto general;
	case cmMouseUp:
		if ( ev-> pos. button & mbMiddle) msg = WM_MBUTTONUP; else
		if ( ev-> pos. button & mbRight)  msg = WM_RBUTTONUP; else
		if ( ev-> pos. button & mbLeft)   msg = WM_XBUTTONUP; else
		{
			msg  = WM_XBUTTONUP;
			mp1s = MAKEWPARAM(0, (ev-> pos. button & mb4) ? XBUTTON1 : XBUTTON2);
		}
		goto general;
	case cmMouseDown:
		if ( ev-> pos. button & mbMiddle) msg = WM_MBUTTONDOWN; else
		if ( ev-> pos. button & mbRight)  msg = WM_RBUTTONDOWN; else
		if ( ev-> pos. button & mbLeft)   msg = WM_LBUTTONDOWN; else
		{
			msg = WM_XBUTTONDOWN;
			mp1s = MAKEWPARAM(0, (ev-> pos. button & mb4) ? XBUTTON1 : XBUTTON2);
		}
		goto general;
	case cmMouseWheel:
		msg  = WM_MOUSEWHEEL;
		mp1s = ( SHORT) ev-> pos. button;
		goto general;
	case cmMouseClick:
		if ( ev-> pos. dblclk) {
			if ( ev-> pos. button & mbMiddle) msg = WM_MBUTTONDBLCLK; else
			if ( ev-> pos. button & mbRight)  msg = WM_RBUTTONDBLCLK; else
			if ( ev-> pos. button & mbLeft)   msg = WM_LBUTTONDBLCLK; else
			{
				msg = WM_XBUTTONDBLCLK;
				mp1s = MAKEWPARAM(0, (ev-> pos. button & mb4) ? XBUTTON1 : XBUTTON2);
			}
		} else {
			Event newEvent = *ev;
			if ( ev-> pos. button & mbMiddle) msg = WM_MMOUSECLICK; else
			if ( ev-> pos. button & mbRight)  msg = WM_RMOUSECLICK; else
			if ( ev-> pos. button & mbLeft)   msg = WM_LMOUSECLICK; else
			{
				msg = WM_XMOUSECLICK;
				mp1s = MAKEWPARAM(0, (ev-> pos. button & mb4) ? XBUTTON1 : XBUTTON2);
			}
			newEvent. cmd = cmMouseDown;
			apc_message( self, &newEvent, post);
			newEvent. cmd = cmMouseUp;
			apc_message( self, &newEvent, post);
		}
	general: {
		LPARAM mp2 = MAKELPARAM( ev-> pos. where. x, sys lastSize. y - ev-> pos. where. y - 1);
		WPARAM mp1 = mp1s |
			(( ev-> pos. mod & kmShift) ? MK_SHIFT   : 0) |
			(( ev-> pos. mod & kmCtrl ) ? MK_CONTROL : 0);
		if ( post) {
			KeyPacket * kp;
			kp = ( KeyPacket *) malloc( sizeof( KeyPacket));
			if ( kp) {
				kp-> mp1 = mp1;
				kp-> mp2 = mp2;
				kp-> msg = msg;
				kp-> wnd = ( HWND) var handle;
				kp-> mod = ev-> pos. mod;
				PostMessage( 0, WM_KEYPACKET, 0, ( LPARAM) kp);
			}
		} else {
			BYTE * mod = NULL;
			Bool wui = P_APPLICATION-> wantUnicodeInput;
			if (( GetKeyState( VK_MENU) < 0) ^ (( ev-> pos. mod & kmAlt) != 0))
				mod = mod_select( ev-> pos. mod);
			P_APPLICATION-> wantUnicodeInput = ev-> key. mod & kmUnicode;
			SendMessage(( HWND) var handle, msg, mp1, mp2);
			if ( mod) mod_free( mod);
			P_APPLICATION-> wantUnicodeInput = wui;
		}
		break;
	}
	case cmKeyDown:
	case cmKeyUp: {
		WPARAM mp1;
		LPARAM mp2;
		int scan = 0;
		UINT msg;
		Bool specF10 = ( ev-> key. key == kbF10) && !( ev-> key. mod & kmAlt);
		// constructing mp1
		if ( ev-> key. key == kbNoKey) {
			if ( ev-> key. code == 0) {
				if ( ev-> key. mod & kmAlt   ) mp1 = VK_MENU;    else
				if ( ev-> key. mod & kmShift ) mp1 = VK_SHIFT;   else
				if ( ev-> key. mod & kmCtrl  ) mp1 = VK_CONTROL; else
					return false;
			} else {
				SHORT c = VkKeyScan(( CHAR ) ev-> key. code);
				if ( c == -1) {
					HKL kl = guts. keyLayout ? guts. keyLayout : GetKeyboardLayout( 0);
					c = VkKeyScanEx(( CHAR) ev-> key. code, kl);
					if ( c == -1) return false;
					scan = MapVirtualKeyEx( LOBYTE( c), 0, kl);
				} else {
					scan = MapVirtualKey( LOBYTE( c), 0);
				}
				mp1 = LOBYTE( c);
				c = HIBYTE( c);
				ev-> key. mod |=
					(( c & 1) ? kmShift : 0) |
					(( c & 2) ? kmCtrl  : 0) |
					(( c & 4) ? kmAlt   : 0);
			}
		} else {
			if ( ev-> key. key == kbBackTab) {
				mp1  = VK_TAB;
				ev-> key. mod |= kmShift;
			} else {
				mp1 = ctx_remap( ev-> key. key, ctx_kb2VK, true);
				if ( mp1 == 0) return false;
			}
			scan = MapVirtualKey( mp1, 0);
		}

		// constructing msg
		msg = ( ev-> key. mod & kmAlt) ? (
			( ev-> cmd == cmKeyUp) ? WM_SYSKEYUP : WM_SYSKEYDOWN
		) : (
			( ev-> cmd == cmKeyUp) ? WM_KEYUP : WM_KEYDOWN
		);

		// constructing mp2
		mp2 = MAKELPARAM( ev-> key. repeat,  scan);
		switch ( msg) {
		case WM_KEYDOWN:
			mp2 |= 0x00000000;
			break;
		case WM_SYSKEYDOWN:
			mp2 |= 0x20000000;
			break;
		case WM_KEYUP:
			mp2 |= 0xC0000000;
			break;
		case WM_SYSKEYUP:
			mp2 |= 0xE0000000;
			break;
		}

		if ( specF10)
			msg = ( ev-> cmd == cmKeyUp) ? WM_SYSKEYUP : WM_SYSKEYDOWN;

		if ( post) {
			KeyPacket * kp;
			kp = ( KeyPacket *) malloc( sizeof( KeyPacket));
			if ( kp) {
				kp-> mp1 = mp1;
				kp-> mp2 = mp2;
				kp-> msg = msg;
				kp-> wnd = HANDLE;
				kp-> mod = ev-> key. mod;
				PostMessage( 0, WM_KEYPACKET, 0, ( LPARAM) kp);
			}
		} else {
			Bool wui = P_APPLICATION-> wantUnicodeInput;
			BYTE * mod = mod_select( ev-> key. mod);
			P_APPLICATION-> wantUnicodeInput = ev-> key. mod & kmUnicode;
			SendMessage( HANDLE, msg, mp1, mp2);
			mod_free( mod);
			P_APPLICATION-> wantUnicodeInput = wui;
		}
		break;
	}
	default:
		return false;
	}
	return true;
}

/* Convert explorer-string format (asciiz,asciiz,...,0) into
backslash-escaped string. 
Spaces and backslashes are escaped */
static char *
duplicate_zz_string( const WCHAR * c)
{
	int sz = 1;
	WCHAR * d = ( WCHAR *) c;
	WCHAR buf[20480];

	while ( d[0] || d[1]) {
		if ( *d == L' ' || *d == L'\\') sz++;
		sz++;
		d++;
	}

	d = buf;
	while ( c[0] || c[1]) {
		if ( !*c) {
			*d++ = L' ';
			c++;
			continue;
		}
		if ( *c == L' ' || *c == L'\\')
			*d++ = L'\\';
		*d++ = *c++;
	}
	*d++ = 0;

	return alloc_wchar_to_utf8(buf, &sz);
}

/* performs non-standard windows open file function */
static char *
win32_openfile( const char * params)
{
	static OPENFILENAMEW o;
	static Bool initialized = false;
	static WCHAR filter[2048]    = L"";
	static WCHAR defext[32]      = L"";
	static WCHAR directory[2048] = L"";
	static WCHAR title[256]      = L"";
	static WCHAR filters[20480];

#define UTF8COPY(dst) {\
	MultiByteToWideChar(CP_UTF8, 0, params, -1, dst, sizeof(dst)/sizeof(WCHAR));\
	dst[sizeof(dst)/sizeof(WCHAR) - 1] = 0;\
}
	if ( !initialized) {
		memset( &o, 0, sizeof(o));
		o. lStructSize = sizeof(o);
		o. nMaxFile = 20479;
		o. nMaxCustFilter = 2047;
		initialized = true;
	}

	if ( strncmp( params, "filters=", 8) == 0) {
		params += 8;
		if ( strcmp( params, "NULL") == 0) {
			o. lpstrFilter = NULL;
		} else {
			/* copy \0\0-terminated string */
			const char *p = params;
			int paramsz = 2, fsz = sizeof(filters)/sizeof(WCHAR);
			while ( p[0] || p[1]) p++, paramsz++;
			MultiByteToWideChar(CP_UTF8, 0, params, paramsz, filters, fsz);
			filters[fsz - 1] = filters[fsz - 2] = 0;
			o. lpstrFilter = filters;
		}
	} else if ( strncmp( params, "directory", 9) == 0) {
		params += 9;
		if ( *params == '=') {
			params += 1;
			if ( strcmp( params, "NULL") == 0) {
				o. lpstrInitialDir = NULL;
			} else {
				UTF8COPY(directory);
				o. lpstrInitialDir = directory;
			}
		} else {
			return alloc_wchar_to_utf8( directory, NULL);
		}
	} else if ( strncmp( params, "title=", 6) == 0) {
		params += 6;
		if ( strcmp( params, "NULL") == 0) {
			o. lpstrTitle = NULL;
		} else {
			UTF8COPY(title);
			o. lpstrTitle = title;
		}
	} else if ( strncmp( params, "defext=", 7) == 0) {
		params += 7;
		if ( strcmp( params, "NULL") == 0) {
			o. lpstrDefExt = NULL;
		} else {
			UTF8COPY(defext);
			o. lpstrDefExt = defext;
		}
	} else if ( strncmp( params, "filter=", 7) == 0) {
		params += 7;
		if ( strcmp( params, "NULL") == 0) {
			o. lpstrCustomFilter = NULL;
		} else if ( strcmp( params, "DEFAULT") == 0) {
			o. lpstrCustomFilter = filter;
		} else {
			UTF8COPY(filter);
			o. lpstrCustomFilter = filter;
		}
	} else if ( strncmp( params, "filterindex", 11) == 0) {
		params += 11;
		if ( *params == '=') {
			int fi = 0;
			sscanf( params + 1, "%d", &fi);
			o. nFilterIndex = fi;
		} else {
			char buf[25];
			sprintf( buf, "%d", (int) o. nFilterIndex);
			return duplicate_string( buf);
		}
	} else if ( strncmp( params, "flags=", 6) == 0) {
		params += 6;
		o. Flags = 0;
		while ( *params) {
			char * cp = ( char *) params, pp;
			while ( *cp && *cp != ',') cp++;
			pp = *cp;
			*cp = 0;
			if ( stricmp( params, "READONLY") == 0) o. Flags |=              OFN_READONLY; else
			if ( stricmp( params, "OVERWRITEPROMPT") == 0) o. Flags |=       OFN_OVERWRITEPROMPT; else
			if ( stricmp( params, "HIDEREADONLY") == 0) o. Flags |=          OFN_HIDEREADONLY; else
			if ( stricmp( params, "NOCHANGEDIR") == 0) o. Flags |=           OFN_NOCHANGEDIR; else
			if ( stricmp( params, "SHOWHELP") == 0) o. Flags |=              OFN_SHOWHELP; else
			if ( stricmp( params, "NOVALIDATE") == 0) o. Flags |=            OFN_NOVALIDATE; else
			if ( stricmp( params, "ALLOWMULTISELECT") == 0) o. Flags |=      OFN_ALLOWMULTISELECT; else
			if ( stricmp( params, "EXTENSIONDIFFERENT") == 0) o. Flags |=    OFN_EXTENSIONDIFFERENT; else
			if ( stricmp( params, "PATHMUSTEXIST") == 0) o. Flags |=         OFN_PATHMUSTEXIST; else
			if ( stricmp( params, "FILEMUSTEXIST") == 0) o. Flags |=         OFN_FILEMUSTEXIST; else
			if ( stricmp( params, "CREATEPROMPT") == 0) o. Flags |=          OFN_CREATEPROMPT; else
			if ( stricmp( params, "SHAREAWARE") == 0) o. Flags |=            OFN_SHAREAWARE; else
			if ( stricmp( params, "NOREADONLYRETURN") == 0) o. Flags |=      OFN_NOREADONLYRETURN; else
			if ( stricmp( params, "NOTESTFILECREATE") == 0) o. Flags |=      OFN_NOTESTFILECREATE; else
			if ( stricmp( params, "NONETWORKBUTTON") == 0) o. Flags |=       OFN_NONETWORKBUTTON; else
			if ( stricmp( params, "NOLONGNAMES") == 0) o. Flags |=           OFN_NOLONGNAMES; else
#ifndef OFN_EXPLORER
#define OFN_EXPLORER 0
#define OFN_NODEREFERENCELINKS 0
#define OFN_LONGNAMES 0
#endif
			if ( stricmp( params, "EXPLORER") == 0) o. Flags |=              OFN_EXPLORER; else
			if ( stricmp( params, "NODEREFERENCELINKS") == 0) o. Flags |=    OFN_NODEREFERENCELINKS; else
			if ( stricmp( params, "LONGNAMES") == 0) o. Flags |=             OFN_LONGNAMES; else
			warn("win32.OpenFile: Unknown constant OFN_%s", params);
			params = cp + 1;
			if ( !pp) break;
		}
	} else if (
		( strncmp( params, "open", 4) == 0) ||
		( strncmp( params, "save", 4) == 0)
	) {
		Bool ret;
		WCHAR filename[20480] = L"";

		guts. focSysDialog = 1;
		o. lpstrFile = filename;
		ret = (strncmp( params, "open", 4) == 0) ?
			GetOpenFileNameW( &o) :
			GetSaveFileNameW( &o);
		if ( ret == 0) {
			DWORD error;
			error = CommDlgExtendedError();
			if ( error != 0) {
				warn("win32.OpenFile: Get%sFileName error %lu at line %d at %s\n",
			 		(strncmp( params, "open", 4) == 0) ? "Open" : "Save",
					error,
			 		__LINE__, __FILE__
				);
			}
		}
		guts. focSysDialog = 0;
		if ( !ret) return 0;
		wcsncpy( directory, o. lpstrFile, o. nFileOffset);
		if ( o. Flags & OFN_ALLOWMULTISELECT) {
			if ( o. Flags & OFN_EXPLORER)
				return duplicate_zz_string( o. lpstrFile + o. nFileOffset );
			else
				return alloc_wchar_to_utf8( o. lpstrFile + o. nFileOffset, NULL );
		}
		return alloc_wchar_to_utf8( o. lpstrFile, NULL);
	} else {
		warn("win32.OpenFile: Unknown function %s", params);
	}

	return 0;
}

static BOOL CALLBACK
find_console( HWND w, LPARAM ptr)
{
	DWORD pid;
	char buf[256];
	DWORD tid = GetWindowThreadProcessId( w, &pid);
	if ( tid != guts. mainThreadId) return TRUE;
	if ( GetClassName( w, buf, 255) == 0) return TRUE;
	if ( strcmp( buf, "ConsoleWindowClass") != 0) return TRUE;
	*(HWND*)(ptr) = w;
	return FALSE;
}

char *
apc_system_action( const char * params)
{
	switch ( *params) {
	case 'b':
#define STR "browser"
		if ( strncmp( params, STR, strlen( STR)) == 0) {
#undef STR
			HKEY k;
			DWORD valSize = MAX_PATH, valType = REG_SZ, res;
			char buf[ MAX_PATH] = "";
			RegOpenKeyEx( HKEY_CLASSES_ROOT, "http\\shell\\open\\command", 0, KEY_READ, &k);
			res = RegQueryValueEx( k, NULL, NULL, &valType, ( LPBYTE)buf, &valSize);
			RegCloseKey( k);
			if ( res != ERROR_SUCCESS) return NULL;
			return duplicate_string( buf);
		}
		break;
	case 'r':
		if ( strncmp( params, "resolution", 10) == 0) {
			int dx, dy;
			int i = sscanf( params + 10, "%u %u", &dx, &dy);
			if ( i != 2 || (dx < 1 || dy < 1)) {
				warn("Bad resolution\n");
				return 0;
			}
			guts. displayResolution. x = dx;
			guts. displayResolution. y = dy;
			reset_system_fonts();
			destroy_font_hash();
			font_clean();
		}
		break;
	case 'w':
		if ( strncmp( params, "win32.SetVersion", 16) == 0) {
			const char * ver = params + 17;
			while ( *ver && ( *ver == ' '  || *ver == '\t')) ver++;

			if ( stricmp( ver, "NT") == 0) {
				guts. version = 0;
			} else if (( stricmp( ver, "95") == 0) || ( stricmp( ver, "mustdie") == 0)) {
				guts. version = 0x80000095;
			} else if ( stricmp( ver, "98") == 0) {
				guts. version = 0x80000098;
			} else {
				guts. version = 0x80000000;
			}
		} else if ( strncmp( params, "win32.ConsoleWindow", 19) == 0) {
			params += 19;

			if ( !guts. console) {
				EnumWindows((WNDENUMPROC) find_console, (LPARAM) &guts. console);
				if ( strcmp( params, " exists") == 0) return 0;
				if ( !guts. console) {
					warn("No associated console window found");
					return 0;
				}
			}

			if ( strcmp( params, " exists") == 0) {
				char * p = ( char *) malloc(12);
				if ( p) snprintf( p, 12, PR_HANDLE_FMT, ( Handle) guts. console);
				return p;
			} else
			if ( strcmp( params, " hide") == 0)     { ShowWindow( guts. console, SW_HIDE); } else
			if ( strcmp( params, " show") == 0)     { ShowWindow( guts. console, SW_SHOW); } else
			if ( strcmp( params, " minimize") == 0) { ShowWindow( guts. console, SW_MINIMIZE); } else
			if ( strcmp( params, " maximize") == 0) { ShowWindow( guts. console, SW_SHOWMAXIMIZED ); } else
			if ( strcmp( params, " restore") == 0)  { ShowWindow( guts. console, SW_RESTORE); } else
			if ( strcmp( params, " topmost") == 0)  { SetWindowPos( guts. console, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE); } else
			if ( strcmp( params, " notopmost") == 0)  { SetWindowPos( guts. console, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE); } else
			if ( strncmp( params, " text", 5) == 0)  {
				char * p = (char*)params + 5;
				while ( *p == ' ') p++;
				if ( *p) {
					SetWindowText( guts. console, p);
				} else {
					int lc = GetWindowTextLength( guts. console);
					p = (char*)malloc( lc + 2);
					if ( p) GetWindowText( guts. console, p, lc+1);
					return p;
				}
			} else {
				warn( "Bad parameters '%s' to sysaction win32.ConsoleWindow", params);
				return 0;
			}
		} else if ( strncmp( params, "win32.OpenFile.", 15) == 0) {
			params += 15;
			return win32_openfile( params);
		} else
			goto DEFAULT;
		break;
	DEFAULT:
	default:
		warn( "Unknown sysaction \"%s\"", params);
	}
	return 0;
}

int
apc_application_get_mapper_font( Handle self, int index, Font * font)
{
	return 0;
}

int
apc_application_set_mapper_font( Handle self, int index, Font * font)
{
	return 0;
}


#ifdef __cplusplus
}
#endif
