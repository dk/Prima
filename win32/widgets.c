#include "win32\win32guts.h"
#include <commdlg.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "File.h"
#include "Menu.h"
#include "Image.h"
#include "Region.h"
#include "Window.h"
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

#define WinShowWindow(WND) SetWindowPos( WND, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
#define WinHideWindow(WND) SetWindowPos( WND, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW);
#define apc_widget_redraw(self)  apc_widget_invalidate_rect(self,NULL)

void
process_transparents( Handle self)
{
	int i;
	RECT mr;
	objCheck;
	GetWindowRect(( HWND) var handle, &mr);
	for ( i = 0; i < var widgets. count; i++) {
		HWND xwnd;
		Handle x = var widgets. items[ i];
		dobjCheck(x);
		xwnd = DHANDLE(x);
		if ( dsys(x) options. aptTransparent && IsWindowVisible( xwnd)) {
			RECT r, dr;
			GetWindowRect( xwnd, &r);
			IntersectRect( &dr, &r, &mr);
			if ( !IsRectEmpty( &dr))
				InvalidateRect( xwnd, NULL, false);
		}
	}
}

typedef struct _ViewProfile {
	ColorSet colors;
	Point    pos;
	Point    size;
	Point    virtSize;
	Bool     enabled;
	Bool     visible;
	Bool     focused;
	Handle   capture;
	HRGN     shape;
} ViewProfile, *PViewProfile;


static void
get_view_ex( Handle self, PViewProfile p)
{
	int i;
	if ( !p) return;
	p-> capture   = apc_widget_is_captured( self);
	for ( i = 0; i <= ciMaxId; i++) p-> colors[ i] = apc_widget_get_color( self, i);
	if ( sys className == WC_FRAME) {
		p-> pos       = apc_window_get_client_pos( self);
		p-> size      = apc_window_get_client_size( self);
	} else {
		p-> pos       = apc_widget_get_pos( self);
		p-> size      = apc_widget_get_size( self);
	}
	p-> virtSize  = var virtualSize;
	p-> enabled   = apc_widget_is_enabled( self);
	p-> focused   = apc_widget_is_focused( self);
	p-> visible   = apc_widget_is_visible( self);
	p-> shape = CreateRectRgn(0,0,0,0);
	if ( sys className == WC_FRAME && is_apt(aptLayered))
		i = GetWindowRgn((HWND) var handle, p->shape);
	else
		i = GetWindowRgn( HANDLE, p->shape);
	if (!i) {
		DeleteObject(p->shape);
		p->shape = NULL;
	}
}

static void
set_view_ex( Handle self, PViewProfile p)
{
	int i;
	Bool clip_by_children;
	if ( sys className == WC_FRAME && is_apt(aptLayered)) {
		SetWindowRgn((HWND) var handle, p-> shape, true);
	} else
		SetWindowRgn( HANDLE, p-> shape, true);
	apc_widget_set_visible( self, false);
	for ( i = 0; i <= ciMaxId; i++) apc_widget_set_color( self, p-> colors[i], i);
	apc_widget_set_font( self, &var font);
	if ( sys className == WC_FRAME) {
		apc_window_set_client_rect( self, p-> pos. x, p-> pos. y, p-> size.x, p-> size.y);
	} else {
		apc_widget_set_rect( self, p-> pos. x, p-> pos. y, p-> size.x, p-> size.y);
	}
	var virtualSize = p-> virtSize;
	apc_widget_set_enabled( self, p-> enabled);
	if ( p-> focused) apc_widget_set_focused( self);

	clip_by_children = is_apt(aptClipByChildren);
	apt_set(aptClipByChildren); /* by default widget created with clipping */
	apc_widget_set_clip_by_children(self, clip_by_children);

	apc_widget_set_visible( self, p-> visible);
	if ( p-> capture) apc_widget_set_capture( self, 1, NULL_HANDLE);
	if ( !InvalidateRect(( HWND) var handle, NULL, false)) apiErr;
	process_transparents( self);
}

static Bool repost_msgs( PostMsg * msg, Handle self)
{
	PostMessage(( HWND) var handle, WM_POSTAL, ( WPARAM) self, ( LPARAM) msg);
	return false;
}

static Bool
create_group( Handle self, Handle owner, Bool syncPaint, Bool clipOwner,
				Bool taskListed, int className, DWORD style, DWORD exstyle,
				Bool usePos, Bool useSize,
				ViewProfile * vprf, HWND parentHandle)
{
	HWND ret = NULL;
	HWND old        = HANDLE;
	HWND old2       = (HWND) var handle;
	HWND behind     = HWND_TOP;
	HWND ownerView  = ( HWND) (( PWidget) owner)-> handle;
	HWND parentView = (( PDrawableData)(( PComponent) owner)-> sysData)-> parent;
	int  count = 0;
	Bool reset = false;
	Handle * list = NULL;
	const WCHAR wnull = 0;

	if ( HANDLE)
	{
		int i;
		if (( DHANDLE( owner) == sys owner) && ( clipOwner == is_apt( aptClipOwner)))
		{
			behind = GetWindow( HANDLE, GW_HWNDPREV);
			if ( behind == NULL) behind = HWND_TOP;
		}
		var stage = csAxEvents;
		if ( kind_of( self, CWidget))
		{
			PWidget g = ( PWidget) self;
			list  = g-> widgets. items;
			count = g-> widgets. count;
		}
		var self-> end_paint_info( self);
		var self-> end_paint( self);
		reset = true;
		for( i = 0; i < count; i++)
			get_view_ex( list[ i], ( ViewProfile*)( dsys( list[ i]) recreateData = malloc( sizeof( ViewProfile))));
	}

	switch (( int) className)
	{
	case WC_FRAME:
		{
			HWND frame;
			RECT r;
			int  rcp[4] = {0,0,0,0};
			Bool layered = is_apt(aptLayered);
			if ( !clipOwner) parentView = HWND_DESKTOP;
			if ( !taskListed && ( parentView == HWND_DESKTOP || parentView == NULL))
				parentView = DHANDLE( application);
			if ( !usePos)  rcp[0] = rcp[1] = CW_USEDEFAULT;
			if ( !useSize) rcp[2] = rcp[3] = CW_USEDEFAULT;
			if ( !( frame = CreateWindowExW( exstyle, layered ? L"LayeredFrame" : L"GenericFrame", &wnull,
					style | WS_CLIPCHILDREN,
					rcp[0], rcp[1], rcp[2], rcp[3],
					parentView, NULL, guts. instance, NULL)))
				apiErrRet;
			if ( !SetWindowPos( frame, behind, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
				apiErr;
			if ((( PApplication) application)-> topExclModal &&
				((( PApplication) application)-> topExclModal != self)
				)
				if ( !SetWindowPos( frame, DHANDLE((( PApplication) application)-> topExclModal),
						0,0,0,0,SWP_NOMOVE | SWP_NOSIZE  | SWP_NOACTIVATE))
					apiErr;
			GetClientRect( frame, &r);
			if ( old2 )
				SetWindowLongPtr( old2, GWLP_USERDATA, 0);
			if ( old )
				SetWindowLongPtr( old, GWLP_USERDATA, 0);
			if ( layered ) {
				if ( !( ret = CreateWindowExW( WS_EX_LAYERED|WS_EX_TOOLWINDOW,  L"Generic", &wnull,
						WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						r. left, r. top, r. right, r. bottom, NULL, NULL,
						guts. instance, NULL)))
					apiErr;
			} else {
				if ( !( ret = CreateWindowExW( 0,  L"Generic", &wnull,
						WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						0, 0, r. right - r. left, r. bottom - r. top, frame, NULL,
						guts. instance, NULL)))
					apiErr;
			}
			sys lastSize. x = r. right  - r. left;
			sys yOverride = sys lastSize. y = r. bottom - r. top;
			sys handle = frame;
			SetWindowLongPtr( frame, GWLP_USERDATA, ( LONG) self);
		}
		break;
	case WC_CUSTOM:
		if ( !parentHandle && ( !clipOwner || owner == application)) {
				style &= ~WS_CHILD;
				style |= WS_POPUP;
				exstyle |= WS_EX_TOOLWINDOW;
		}
		if ( parentHandle) parentView = parentHandle;
		sys parentHandle = parentHandle;

		if ( old )
			SetWindowLongPtr( old, GWLP_USERDATA, 0);
		if ( !( ret = CreateWindowExW( exstyle,  L"Generic", &wnull,
				style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0,
				parentView, NULL, guts. instance, NULL)))
			apiErrRet;
		if ( !SetWindowPos( ret, behind, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
			apiErr;
		sys handle = ret;
		break;
	}

	apt_assign( aptClipOwner, clipOwner);
	apt_assign( aptSyncPaint, syncPaint);
	apt_set( aptEnabled);
	apt_clear( aptRepaintPending );
	apt_clear( aptMovePending );
	sys className = className;
	sys parent  = ret;
	var handle  = ( Handle) ret;
	sys owner   = ownerView;
	if ( !reset)
		sys pointer = LoadCursor( guts. instance, IDC_ARROW);
	SetWindowLongPtr( ret, GWLP_USERDATA, ( LONG) self);

	if ( reset)
	{
		int i;
		Handle oldOwner = var owner; var owner = owner;
		if ( sys className == WC_FRAME) {
			apc_window_set_client_rect( self, vprf-> pos. x, vprf-> pos. y, vprf-> size. x, vprf-> size. y);
		} else {
			apc_widget_set_rect( self, vprf-> pos. x, vprf-> pos. y, vprf-> size. x, vprf-> size. y);
		}
		var owner = oldOwner;
		for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
		if ( sys className == WC_FRAME)
		{
			SetMenu( HANDLE, GetMenu( old));
			SetMenu( old, NULL);
		}
		if ( old2 != old ) {
			if ( !DestroyWindow( old2 ))
				apiErr;
		}
		if ( !DestroyWindow( old))
			apiErr;
		if ( var postList) list_first_that( var postList, repost_msgs, ( void*)self);
	}
	PostMessage( ret, WM_PRIMA_CREATE, 0, 0);

	if ( !reset) {
		/* set manually cmMove and cmSize when windows are configured automatically */
		if ( !usePos) {
			Event e;
			Point sz = apc_window_get_client_pos( self);
			memset( &e, 0, sizeof(e));
			e. gen. source = self;
			e. cmd = cmMove;
			e. gen. P. x = sz. x;
			e. gen. P. y = sz. y;
			CComponent(self)->message( self, &e);
			if ( PObject( self)-> stage == csDead) return false;
		}
		if ( !useSize) {
			Event e;
			Point sz = apc_window_get_client_size( self);
			memset( &e, 0, sizeof(e));
			e. gen. source = self;
			e. cmd = cmSize;
			e. gen. P. x = e. gen. R. right = sz. x;
			e. gen. P. y = e. gen. R. top = sz. y;
			CComponent(self)->message( self, &e);
			if ( PObject( self)-> stage == csDead) return false;
		}
	}
	return true;
}

static void
notify_sys_handle( Handle self )
{
	Event ev = {cmSysHandle};
	objCheck;
	ev. gen. source = self;
	var self-> message( self, &ev);
}

// Window
Bool
apc_window_create( Handle self, Handle owner, Bool syncPaint, int borderIcons,
						int borderStyle, Bool taskList, int windowState,
						int on_top, Bool usePos, Bool useSize, Bool layered)
{
	Bool reset = false;
	ViewProfile vprf;
	int oStage = var stage;
	WCHAR * saved_caption = NULL;
	WindowData ws;
	HICON icon = (HICON) NULL_HANDLE;
	WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
	DWORD style = WS_CLIPCHILDREN | WS_OVERLAPPED
		| (( borderIcons &  biSystemMenu) ? WS_SYSMENU     : 0)
		| (( borderIcons &  biMinimize)   ? WS_MINIMIZEBOX : 0)
		| (( borderIcons &  biMaximize)   ? WS_MAXIMIZEBOX : 0)
		| (( borderIcons &  biTitleBar)   ? 0              : WS_POPUP)
		| (( borderStyle == bsSizeable)   ? WS_THICKFRAME  : 0)
		| (( borderStyle == bsSingle  )   ? WS_BORDER      : 0)
		| (( borderStyle == bsDialog  )   ? WS_BORDER      : 0)
		| (( windowState == wsMinimized)  ? WS_MINIMIZE    : 0)
		| (( windowState == wsMaximized)  ? WS_MAXIMIZE    : 0)
	;
	DWORD exstyle = 0
		| (( borderStyle == bsDialog  )   ? WS_EX_DLGMODALFRAME : 0)
	;

	objCheck false;
	dobjCheck( owner) false;
	if ( guts. displayBMInfo. bmiHeader. biBitCount <= 8) layered = false;

	if ( !kind_of( self, CWidget)) apcErrRet( errInvObject);
	apcErrClear;
	if (( var handle != NULL_HANDLE) && (
			( DHANDLE( owner) != sys owner)
		|| ( borderStyle != sys s. window. borderStyle)
		|| ( borderIcons != sys s. window. borderIcons)
		|| ( taskList    != is_apt( aptTaskList))
		|| ( layered     != is_apt( aptLayered))
	))
	{
		int len = GetWindowTextLengthW( HANDLE ) + 1;
		if (( saved_caption = (WCHAR*) malloc( sizeof(WCHAR) * len)) != NULL )
			GetWindowTextW( HANDLE, saved_caption, len );
		// Windows 8 shell is observed to send WM_SIZE(0,0) on ShowWindow(SW_SHOWNORMAL)
		// when application started with /min flag
		apt_set( aptIgnoreSizeMessages );
		apc_window_set_window_state( self, windowState);
		apt_clear( aptIgnoreSizeMessages );
		// prevent cmSize/cmWindowStage message loss if recreate goes with WS_XXX change.
		if ( sys recreateData) {
			memcpy( &vprf, sys recreateData, sizeof( vprf));
			free( sys recreateData);
			sys recreateData = NULL;
		} else
			get_view_ex( self, &vprf);
		ws = sys s. window;
		if ( !GetWindowPlacement( HANDLE, &wp)) apiErr;
		if ( wp. showCmd == SW_SHOWMINIMIZED && windowState != wsMinimized )
			wp. showCmd = ( windowState == wsMaximized ) ? SW_SHOWMAXIMIZED : SW_NORMAL;
		usePos = useSize = 1; // prevent using shell-position flags for recreate
		icon = ( HICON) SendMessage( HANDLE, WM_GETICON, ICON_BIG, 0);
		reset = true;
	}
	HWND_lock( true);

	if ( !reset) apt_set( aptClipByChildren );
	apt_assign( aptLayeredRequested, layered );
	apt_assign( aptLayered, layered );

	if ( reset || ( var handle == NULL_HANDLE))
		if ( !create_group( self, owner, syncPaint, false,
				taskList, WC_FRAME, style, exstyle, usePos, useSize, &vprf, NULL)) {
			if ( on_top >= 0) {
				apt_assign( aptOnTop, on_top);
				if ( on_top > 0)
					SetWindowPos( sys handle, HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
			HWND_lock( false);
			if ( saved_caption ) free( saved_caption );
			return false;
		}
	ws. borderStyle = sys s. window. borderStyle = borderStyle;
	ws. borderIcons = sys s. window. borderIcons = borderIcons;
	ws. state       = sys s. window. state       = windowState;
	apt_assign( aptSyncPaint, syncPaint);
	apt_assign( aptTaskList,  taskList);
	if ( usePos) apt_set( aptWinPosDetermined);
	if ( reset)
	{
		Handle oldOwner = var owner;

		var owner = owner;
		apc_window_set_window_state( self, windowState);
		var owner = oldOwner;
		set_view_ex( self, &vprf);
		sys s. window = ws;
		if ( windowState != wsMaximized)
			GetWindowRect( HANDLE, &wp. rcNormalPosition);
		if ( !SetWindowPlacement( HANDLE, &wp)) apiErr;
		var stage = oStage;
		if ( icon) SendMessage( HANDLE, WM_SETICON, ICON_BIG, ( LPARAM) icon);
	}
	else {
		guts. topWindows++;
	}
	if ( on_top >= 0) {
		apt_assign( aptOnTop, on_top);
		if ( on_top > 0)
			SetWindowPos( sys handle, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	SetWindowTextW( HANDLE, saved_caption );
	if ( saved_caption ) free( saved_caption );
	HWND_lock( false);
	if ( reset ) {
		notify_sys_handle( self );
		if ( layered ) hwnd_repaint_layered(self, false);
	}
	return guts.apcError == 0;
}

Bool
apc_window_activate( Handle self)
{
	if ( self) {
		HWND w;
		objCheck false;
		w = HANDLE;
		SetForegroundWindow( w);
		SetActiveWindow( w);
		return true;
	}
	return false;
}

Bool
apc_window_is_active( Handle self)
{
	return apc_window_get_active() == self;
}

Handle
apc_window_get_active( void)
{
	Handle self = hwnd_to_view( GetActiveWindow());
	if ( !self)
		return NULL_HANDLE;
	if ( sys className == WC_FRAME)
		return self;
	else if ( !is_apt( aptClipOwner)) {
		return hwnd_top_level( self);
	} else
		return NULL_HANDLE;
}

Bool
apc_window_close( Handle self)
{
	objCheck true;
	if ( !PostMessage( HANDLE, WM_CLOSE, 0, 0)) apiErrRet;
	return true;
}

int
apc_window_get_border_icons( Handle self)
{
	objCheck 0;
	return sys s. window. borderIcons;
}

int
apc_window_get_border_style( Handle self)
{
	objCheck 0;
	return sys s. window. borderStyle;
}

ApiHandle
apc_window_get_client_handle( Handle self)
{
	objCheck 0;
	return ( ApiHandle) var handle;
}

Point
apc_window_get_client_pos( Handle self)
{
	Point delta = get_window_borders( sys s. window. borderStyle);
	Handle parent = var self-> get_parent( self);
	Point p = {0,0}, sz   = CWidget( parent)-> get_size( parent);
	RECT  r;

	objCheck p;
	if ( apc_window_get_window_state( self) == wsMinimized) {
		WINDOWPLACEMENT w = { sizeof( WINDOWPLACEMENT)};
		if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
		p. x = w. rcNormalPosition. left + delta. x;
		p. y = sz. y - w. rcNormalPosition. bottom + delta. y;
	} else {
		GetWindowRect( HANDLE, &r);
		p. x = r. left + delta. x;
		p. y = sz. y - r. bottom + delta. y;
	}
	return p;
}

Point
apc_window_get_client_size( Handle self)
{
	RECT r;
	Point p = { 0,0};
	objCheck p;
	if ( apc_window_get_window_state( self) == wsMinimized) {
		// cannot acquire client extension at this time. Using euristic calculations.
		WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
		Point delta = get_window_borders( sys s. window. borderStyle);
		int   menuY  = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
		int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
							GetSystemMetrics( SM_CYCAPTION) : 0;
		if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
		p. x = w. rcNormalPosition. right  - w. rcNormalPosition. left - delta. x * 2;
		p. y = w. rcNormalPosition. bottom - w. rcNormalPosition. top  - delta. y * 2
			- menuY - titleY;
	} else {
		GetWindowRect(( HWND) var handle, &r);
		p. x = r. right - r. left;
		p. y = r. bottom - r. top;
	}
	return p;
}

Bool
apc_window_get_icon( Handle self, Handle icon)
{
	HICON    p;
	HCURSOR  save;
	Bool ret;

	objCheck false;
	p = ( HICON) SendMessage( HANDLE, WM_GETICON, ICON_BIG, 0);
	if ( icon == NULL_HANDLE) return p != NULL;

	save = sys pointer;
	sys pointer = p;
	ret = apc_pointer_get_bitmap( self, icon);
	sys pointer = save;
	return ret;
}

int
apc_window_get_window_state( Handle self)
{
	WINDOWPLACEMENT s = {sizeof( WINDOWPLACEMENT)};
	objCheck wsNormal;
	if ( !GetWindowPlacement( HANDLE, &s)) apiErr;
	if ( s. showCmd == SW_SHOWMAXIMIZED) return wsMaximized;
	if ( s. showCmd == SW_SHOWMINIMIZED) return wsMinimized;
	return wsNormal;
}

Bool
apc_window_get_task_listed( Handle self)
{
	objCheck false;
	return is_apt( aptTaskList);
}

Bool
apc_window_get_on_top( Handle self)
{
	objCheck false;
	return is_apt( aptOnTop);
}

Bool
apc_window_set_caption( Handle self, const char * caption, Bool utf8)
{
	objCheck false;
	if ( utf8) {
		WCHAR * c = alloc_utf8_to_wchar( caption, -1, NULL);
		if ( !( rc = SetWindowTextW( HANDLE, c))) apiErr;
		free( c);
	} else {
		if ( !( rc = SetWindowTextA( HANDLE, caption))) apiErr;
	}
	return rc == 0;
}

Bool
apc_window_set_client_pos( Handle self, int x, int y)
{
	Point delta = get_window_borders( sys s. window. borderStyle);
	RECT  r;
	Handle parent = var self-> get_parent( self);
	Point sz = CWidget( parent)-> get_size( parent);

	objCheck false;
	if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);
	apt_set( aptWinPosDetermined);

	if ( var stage == csConstructing && apc_window_get_window_state( self) != wsNormal) {
		WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
		if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
		w. rcNormalPosition. top    += sz. y - y + delta. y - w. rcNormalPosition. bottom;
		w. rcNormalPosition. bottom  = sz. y - y + delta. y;
		w. rcNormalPosition. right  += x - delta. x - w. rcNormalPosition. left;
		w. rcNormalPosition. left    = x - delta. x;
		w. flags   = 0;
		if ( !SetWindowPlacement( HANDLE, &w)) apiErr;
	} else {
		if ( !GetWindowRect( HANDLE, &r)) apiErr;
		x -= delta. x;
		y  = sz. y - ( r. bottom - r. top) - y + delta. y;
		SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return true;
}

Bool
apc_window_set_client_size( Handle self, int x, int y)
{
	RECT r, c, c2;
	HWND h;
	int  ws = apc_window_get_window_state( self);

	objCheck false;
	if ( !hwnd_check_limits( x, y, false)) apcErrRet( errInvParams);

	h = HANDLE;
	if (( var stage == csConstructing && ws != wsNormal) || ws == wsMinimized) {
		WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
		Point delta = get_window_borders( sys s. window. borderStyle);

		var virtualSize. x = x;
		var virtualSize. y = y;
		if ( x < 0) x = 0;
		if ( y < 0) y = 0;
		if ( !GetWindowPlacement( h, &w)) apiErr;
		if ( !GetWindowRect( h, &c2)) apiErr;
		if ( ws == wsMaximized) {
			if ( !GetClientRect( h, &c)) apiErr;
		}
		else {
			// cannot acquire client extension at this time. Using euristic calculations.
			int  menuY = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
			int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
								GetSystemMetrics( SM_CYCAPTION) : 0;
			c = c2;
			c. right  -= delta. x * 2;
			c. bottom -= delta. y * 2 + menuY + titleY;
		}
		w. rcNormalPosition. top    = w. rcNormalPosition. bottom - y - ( c2. bottom - c2. top - c. bottom + c. top);
		w. rcNormalPosition. right  = x + ( c2. right - c2. left - c. right + c. left) + w. rcNormalPosition. left;
		w. flags   = 0;
		if ( !SetWindowPlacement( h, &w)) apiErr;
	} else {
		if ( !GetWindowRect( h, &r)) apiErr;
		if ( !GetClientRect( h, &c)) apiErr;
		sys sizeLockLevel++;
		var virtualSize. x = x;
		var virtualSize. y = y;
		if ( x < 0) x = 0;
		if ( y < 0) y = 0;
		SetWindowPos( h, 0,
			r. left,
			r. top - y + ( c. bottom - c. top),
			x + r. right  - r. left - c. right + c. left,
			y + r. bottom - r. top  - c. bottom + c. top,
			SWP_NOZORDER | SWP_NOACTIVATE |
				( is_apt( aptWinPosDetermined) ? 0 : SWP_NOMOVE)
			);
		sys sizeLockLevel--;
	}
	return true;
}

Bool
apc_window_set_client_rect( Handle self, int x, int y, int width, int height)
{
	RECT r, c, c2;
	HWND h;
	int  ws = apc_window_get_window_state( self);
	Point delta = get_window_borders( sys s. window. borderStyle);
	Handle parent = var self-> get_parent( self);
	Point sz = CWidget( parent)-> get_size( parent);

	objCheck false;
	if ( !hwnd_check_limits( x, y, false)) apcErrRet( errInvParams);
	if ( !hwnd_check_limits( width, height, false)) apcErrRet( errInvParams);
	apt_set( aptWinPosDetermined);

	h = HANDLE;
	if (( var stage == csConstructing && ws != wsNormal) || ws == wsMinimized) {
		WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
		Point delta = get_window_borders( sys s. window. borderStyle);

		var virtualSize. x = width;
		var virtualSize. y = height;
		if ( width < 0) width = 0;
		if ( height < 0) height = 0;
		if ( !GetWindowPlacement( h, &w)) apiErr;
		if ( !GetWindowRect( h, &c2)) apiErr;
		if ( ws == wsMaximized) {
			if ( !GetClientRect( h, &c)) apiErr;
		}
		else {
			// cannot acquire client extension at this time. Using euristic calculations.
			int  menuY = (( PWindow) self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0;
			int   titleY = ( sys s. window. borderIcons & biTitleBar) ?
								GetSystemMetrics( SM_CYCAPTION) : 0;
			c = c2;
			c. right  -= delta. x * 2;
			c. bottom -= delta. y * 2 + menuY + titleY;
		}
		w. rcNormalPosition. bottom = sz. y - y + delta. y;
		w. rcNormalPosition. left   = x - delta. x;
		w. rcNormalPosition. top    = w. rcNormalPosition. bottom - height - ( c2. bottom - c2. top - c. bottom + c. top);
		w. rcNormalPosition. right  = width + ( c2. right - c2. left - c. right + c. left) + w. rcNormalPosition. left;
		w. flags   = 0;
		if ( !SetWindowPlacement( h, &w)) apiErr;
	} else {
		if ( !GetWindowRect( h, &r)) apiErr;
		if ( !GetClientRect( h, &c)) apiErr;
		sys sizeLockLevel++;
		x -= delta. x;
		y  = sz. y - y - height - ( r. bottom - r. top  - c. bottom + c. top) + delta. y;
		var virtualSize. x = width;
		var virtualSize. y = height;
		if ( width < 0) width = 0;
		if ( height < 0) height = 0;
		SetWindowPos( h, 0,
			x, y,
			width + r. right  - r. left - c. right + c. left,
			height + r. bottom - r. top  - c. bottom + c. top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		sys sizeLockLevel--;
	}
	return true;
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
	Point size;
	objCheck false;
	apcErrClear;
	size = var self-> get_size( self);
	SetMenu( HANDLE, menu ? ( HMENU) (( PComponent) menu)-> handle : NULL);
	DrawMenuBar( HANDLE);
	if ( apc_window_get_window_state( self) == wsNormal)
		var self-> set_size( self, size);
	return guts.apcError == 0;
}

Bool
apc_window_set_effects( Handle self, HV * effects )
{
	return false;
}

Bool
apc_window_set_icon( Handle self, Handle icon)
{
	HICON i;
	objCheck false;
	i = icon ? image_make_icon_handle( icon, guts. iconSizeLarge, NULL) : NULL;
	i = ( HICON) SendMessage( HANDLE, WM_SETICON, ICON_BIG, ( LPARAM) i);
	if ( i) DestroyIcon( i);
	return true;
}

Bool
apc_window_set_window_state( Handle self, int state)
{
	int  fl = -1;
	objCheck false;
	switch ( state)
	{
		case wsMaximized: fl = SW_SHOWMAXIMIZED; break;
		case wsMinimized: fl = SW_MINIMIZE; break;
		case wsNormal   : fl = SW_SHOWNORMAL; break;
	}
	if ( fl > 0)
	{
		ShowWindow( HANDLE, fl);
		sys s. window. state = state;
	}
	return true;
}

static Bool
window_start_modal( Handle self, Bool shared, Handle insertBefore)
{
	HWND wnd;
	HWND active = GetActiveWindow();

	objCheck false;
	wnd = HANDLE;
	if ( sys className != WC_FRAME) { apcErr( errInvParams); return false; }

	sys s. window. oldFoc = apc_widget_get_focused();
	if ( sys s. window. oldFoc) protect_object( sys s. window. oldFoc);
	sys s. window. oldActive = active;

	// setting window up
	guts. focSysDisabled = 1;
	CWindow( self)-> exec_enter_proc( self, shared, insertBefore);
	apc_widget_set_enabled( self, 1);
	SetWindowPos( wnd, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
	if ( sys s. window. state == wsMinimized)
		ShowWindow( wnd, SW_RESTORE);
	if ( !insertBefore) {
		SetActiveWindow( wnd);
		SetForegroundWindow( wnd);
	} else {
		HWND zorder;
		dobjCheck( insertBefore) false;
		zorder = GetWindow( DHANDLE( insertBefore), GW_HWNDNEXT);
		if ( !zorder) zorder = HWND_BOTTOM;
		SetWindowPos( wnd, zorder, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		if ( active)
			SetActiveWindow( active);
	}
	objCheck false;
	PostMessage( wnd, WM_DLGENTERMODAL, 1, 0);
	guts. focSysDisabled = 0;
	return true;
}

Bool
apc_window_execute( Handle self, Handle insertBefore)
{
	objCheck false;
	if ( !window_start_modal( self, false, insertBefore))
		return false;
	// message loop
	{
		MSG msg;
		while ( GetMessage( &msg, NULL, 0, 0)) {
			if ( !process_msg( &msg)) {
				if ( !prima_guts.app_is_dead)
					PostThreadMessage( guts. mainThreadId, WM_TERMINATE, 0, 0);
				break;
			}
			if ( self && !(( PWindow) self)-> modal)
				break;
		}
	}
	// !!note - at this point object may be unaccessible (except var area only).
	return true;
}

Bool
apc_window_execute_shared( Handle self, Handle insertBefore)
{
	objCheck false;
	return window_start_modal( self, true, insertBefore);
}

Bool
apc_window_end_modal( Handle self)
{
	HWND wnd;
	objCheck false;
	wnd = HANDLE;
	guts. focSysDisabled = 1;
	CWindow( self)-> exec_leave_proc( self);
	WinHideWindow( wnd);
	objCheck false;
	apc_widget_set_enabled( self, 0);
	objCheck false;
	if ( application) {
		Handle who = Application_popup_modal( application);
		if ( sys s. window. oldActive)
			SetActiveWindow( sys s. window. oldActive);
		if ( !who && var owner)
			CWidget( var owner)-> set_selected( var owner, 1);
		if (( who = sys s. window. oldFoc)) {
			if ( PWidget( who)-> stage == csNormal)
				CWidget( who)-> set_focused( who, 1);
			unprotect_object( who);
		}
	}
	guts. focSysDisabled = 0;
	return true;
}

// View management
Bool
apc_widget_map_points( Handle self, Bool toScreen, int count, Point * p)
{
	int i;
	Point sz = ((( PWidget) self)-> self)-> get_size( self);
	Point appSz = apc_application_get_size( application);

	if ( self == application)
		return true;

	objCheck false;

	if ( toScreen) {
		for ( i = 0; i < count; i++)
			p[i]. y = sz. y - p[i]. y;
		MapWindowPoints(( HWND) var handle, NULL, ( POINT * ) p, count);
		for ( i = 0; i < count; i++)
			p[i]. y = appSz. y - p[i].y;
	} else {
		for ( i = 0; i < count; i++)
			p[i]. y = appSz. y - p[i]. y;
		MapWindowPoints( NULL, ( HWND) var handle, ( POINT * ) p, count);
		for ( i = 0; i < count; i++)
			p[i]. y = sz. y - p[i]. y;
	}
	return true;
}

Color
apc_widget_map_color( Handle self, Color color)
{
	if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0)) color |= var widgetClass;
	return remap_color( remap_color( color, true), false);
}

Bool
apc_widget_create( Handle self, Handle owner, Bool syncPaint, Bool clipOwner,
						Bool transparent, ApiHandle parentHandle, Bool layered)
{
	Bool reset = false, redraw = false;
	ViewProfile vprf;
	int oStage = var stage;
	int exstyle;

	objCheck false;
	dobjCheck( owner) false;

	if ( !kind_of( self, CWidget)) apcErr( errInvObject);
	apcErrClear;

	if ( parentHandle && !IsWindow(( HWND) parentHandle))
		return false;

	exstyle = 0;
	if ( guts. displayBMInfo. bmiHeader. biBitCount <= 8) layered = false;

	redraw = is_apt( aptLayeredRequested ) != layered;
	apt_assign( aptLayeredRequested, layered );
	apt_clear( aptLayered );
	if ( layered && (( owner == application || !clipOwner ))) {
		apt_set( aptLayered );
		exstyle |= WS_EX_LAYERED;
	}

	if (( var handle != NULL_HANDLE) &&
			(( DHANDLE( owner) != sys owner)                 ||
			(( HWND) parentHandle != sys parentHandle)       ||
			( clipOwner       != is_apt( aptClipOwner))
		))
	{
		if ( sys recreateData) {
			memcpy( &vprf, sys recreateData, sizeof( vprf));
			free( sys recreateData);
			sys recreateData = NULL;
		} else
			get_view_ex( self, &vprf);
		reset = true;
	}
	if ( !reset) apt_set( aptClipByChildren );
	if ( reset || ( var handle == NULL_HANDLE))
		create_group( self, owner, syncPaint, clipOwner, 0, WC_CUSTOM,
			WS_CHILD, exstyle, 1, 1, &vprf, ( HWND) parentHandle);
	apt_set( aptWinPosDetermined);
	if ( reset)
	{
		Handle oldOwner = var owner; var owner = owner;
		set_view_ex( self, &vprf);
		var owner = oldOwner;
		var stage = oStage;
	}
	if ( is_apt( aptTransparent) != transparent && !reset)
		redraw = true;
	if ( redraw) apc_widget_redraw( self);
	apt_assign( aptTransparent, transparent);
	if ( reset) {
		notify_sys_handle( self );
		apc_widget_redraw( self);
	}
	return guts.apcError == 0;
}

Bool
apc_widget_begin_paint( Handle self, Bool insideOnPaint)
{
	Bool useRPDraw = false;
	objCheck false;
	apcErrClear;

	if ( is_apt( aptLayeredPaint )) {
		sys transform2 = sys layeredPaintOffset;
	} else if ( is_apt( aptTransparent)) {
		if ( IsWindowVisible(( HWND) var handle)) {
			HWND parent = GetParent( HANDLE);
			if ( !parent) {
				MSG  msg;
				list_add( &guts. transp, self);
				WinHideWindow(( HWND) var handle);
				if ( parent) apcUpdateWindow( parent);
				while ( PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE)) {
					DispatchMessage(&msg);
					exception_check_raise();
				}
				if ( !parent) Sleep( 1);
				WinShowWindow(( HWND) var handle);
				apcUpdateWindow(( HWND) var handle);
				list_delete( &guts. transp, self);
			} else
				useRPDraw = true;
		}
		sys transform2. x = 0;
		sys transform2. y = 0;
	} else {
		sys transform2. x = 0;
		sys transform2. y = 0;
	}

	apt_set( aptWinPS);
	apt_set( aptCompatiblePS);
	apt_assign( aptWM_PAINT, insideOnPaint);

	if ( is_apt( aptLayeredPaint )) {
		sys ps = sys layeredPaintSurface;
	} else if ( is_apt( aptLayered )) {
		RECT r;

		insideOnPaint = false;
		GetWindowRect( HANDLE, &r);
		sys ps2     = GetDC(NULL);
		sys ps      = CreateCompatibleDC(sys ps2);
		sys bm      = CreateCompatibleBitmap(sys ps2, r. right - r. left, r. bottom - r. top);
		sys stockBM = SelectObject(sys ps, sys bm);
	} else if ( insideOnPaint) {
		if ( !( sys ps = BeginPaint(( HWND) var handle, &sys paintStruc))) apiErrRet;
	} else {
		if ( !( sys ps = GetDC(( HWND) var handle))) apiErrRet;
	}

	if ( !is_apt(aptLayeredPaint) && is_opt( optBuffered) && insideOnPaint) {
		RECT r;
		HBITMAP bm;
		HDC dc;

		GetClipBox( sys ps, &r);
		var w = r. right  - r. left;
		var h = r. bottom - r. top;

		if ( var w == 0 || var h == 0) {
			if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
			apt_clear( aptWinPS);
			apt_clear( aptWM_PAINT);
			apt_clear( aptCompatiblePS);
			sys ps = NULL;
			return false;
		}

		if ( !( dc = CreateCompatibleDC( sys ps))) apiErr;

		if ( sys pal) {
			sys stockPalette = SelectPalette( dc, sys pal, 1);
			RealizePalette( dc);
			sys pal2 = SelectPalette( sys ps, sys pal, 1);
			RealizePalette( sys ps);
		}

		if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
			apt_clear( aptCompatiblePS);

		bm = CreateCompatibleBitmap( sys ps, var w, var h);

		if ( bm) {
			sys ps2 = sys ps;
			sys ps  = dc;
			sys stockBM = SelectObject( dc, bm);
			sys bm = bm;
			SetBrushOrgEx( dc, -r. left, -r. top, NULL);
			apc_gp_set_transform( self, -r. left, -r. top);
			sys transform2. x = r. left;
			sys transform2. y = r. top;
			apt_set( aptBitmap);
		} else
			apiErr;
	} else if ( !is_apt(aptLayered) && !is_apt(aptLayeredPaint)) {
		if ( sys pal) {
			sys stockPalette = SelectPalette( sys ps, sys pal, 1);
			RealizePalette( sys ps);
		}
	}

	if ( useRPDraw) {
		HDC dc;
		Handle owner = var owner;
		Point tr = dsys(owner)transform2;
		Point ed = apc_widget_get_pos( self);
		Point sz = apc_widget_get_size( self);
		Point so = CWidget(owner)-> get_size( owner);
		Bool flag;
		int saved_dc_state;
		POINT brushorg;
		FLOAT miter;

		CWidget( owner)-> begin_paint( owner);
		dc = dsys( owner) ps;
		dsys( owner) ps = sys ps;
		dsys(owner) transform2. x += ed. x;
		dsys(owner) transform2. y += so. y - sz. y - ed. y;
		apc_gp_set_transform( owner, 0, 0);
		apc_gp_set_text_out_baseline( owner, dsys(owner) options. aptTextOutBaseline);

		saved_dc_state = SaveDC( sys ps );
		SelectObject( sys ps, GetCurrentObject( dc, OBJ_PEN));
		SelectObject( sys ps, GetCurrentObject( dc, OBJ_BRUSH));
		SelectObject( sys ps, GetCurrentObject( dc, OBJ_FONT));
		SelectObject( sys ps, GetCurrentObject( dc, OBJ_PAL));
		SelectObject( sys ps, GetCurrentObject( dc, OBJ_EXTPEN));
		SetTextAlign( sys ps, GetTextAlign( dc ));
		GetBrushOrgEx( dc, &brushorg);
		SetBrushOrgEx( sys ps, brushorg.x, brushorg.y, NULL);
		GetMiterLimit( dc, &miter);
		SetMiterLimit( sys ps, miter, NULL);
		SetBkMode( sys ps, GetBkMode( dc ));
		SetROP2( sys ps, GetROP2( dc ));
		SetStretchBltMode( sys ps, GetStretchBltMode( dc ));

		flag = exception_block(true);
		CWidget( owner)-> notify( owner, "sH", "Paint", owner);
		exception_block(flag);

		RestoreDC( sys ps, saved_dc_state);

		dsys(owner)transform2 = tr;
		apc_gp_set_transform( owner, 0, 0);
		dsys( owner) ps = dc;
		CWidget( owner)-> end_paint( owner);
	}

	hwnd_enter_paint( self);

	if ( useRPDraw) {
		apc_gp_set_transform( self, sys transform. x, sys transform. y);
	}

	if ( is_apt(aptLayeredPaint)) {
		Rect r;
		r. left   = 0;
		r. bottom = 0;
		r. right  = sys lastSize. x - 1;
		r. top    = sys lastSize. y - 1;
		apc_gp_set_clip_rect(self, r);
	}

	return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
	HRGN rgn;
	objCheck false;
	apt_set( aptWinPS);
	apt_set( aptCompatiblePS);
	sys transform2. x = 0;
	sys transform2. y = 0;
	if ( !( sys ps = GetDC(( HWND) var handle))) apiErrRet;
	hwnd_enter_paint( self);
	rgn = CreateRectRgn( 0, 0, 0, 0);
	SelectClipRgn( sys ps, rgn);
	DeleteObject( rgn);
	return true;
}


Bool
apc_widget_destroy( Handle self)
{
	objCheck false;
	SetWindowLongPtr( HANDLE, GWLP_USERDATA, 0);
	if ( sys pointer2) {
		if ( sys pointer2 == sys pointer) SetCursor( NULL); // un-use resource first
		if ( !DestroyCursor( sys pointer2)) apiErr;
	}
	if ( sys recreateData) free( sys recreateData);
	if ( self == lastMouseOver) lastMouseOver = NULL_HANDLE;
	if ( self == guts.dragTarget) guts.dragTarget = NULL_HANDLE;
	if ( var handle == NULL_HANDLE) return true;

	if ( sys className == WC_FRAME)
		guts. topWindows--;

	if ( !DestroyWindow( HANDLE)) apiErrRet;
	return true;
}

PFont
apc_widget_default_font( PFont copyTo)
{
	*copyTo = guts. windowFont;
	copyTo-> pitch = fpDefault;
	return copyTo;
}

static void
subpaint_layered_widgets( HWND self, HDC ps, HDC alpha_dc, POINT screen_offset, HRGN parent_shape)
{
	HWND child = GetTopWindow( self );
	if ( child ) child = GetWindow( child, GW_HWNDLAST );
	while ( child != NULL ) {
		Handle h;
		RECT r;
		HRGN shape;
		Event ev;
		POINT child_offset, size;

		h = hwnd_to_view(child);
		if ( h == NULL_HANDLE || !IsWindowVisible(child) || !dsys(h) options. aptClipOwner) {
			child = GetWindow( child, GW_HWNDPREV);
			continue;
		}

		GetWindowRect( child, &r);
		r. left   -= screen_offset. x;
		r. top    -= screen_offset. y;
		r. right  -= screen_offset. x;
		r. bottom -= screen_offset. y;
		size. x = r.right  - r. left;
		size. y = r.bottom - r. top;
		child_offset. x = r. left;
		child_offset. y = r. top;

		dsys(h) options. aptLayeredPaint = 1;
		dsys(h) layeredPaintOffset. x = -r. left;
		dsys(h) layeredPaintOffset. y = -r. top;
		dsys(h) layeredPaintSurface   = ps;

		shape = CreateRectRgn(0,0,0,0);
		if ( GetWindowRgn( child, shape)) {
			HRGN rect = CreateRectRgn( 0, 0, size.x, size.y);
			CombineRgn( shape, shape, rect, RGN_AND);
			DeleteObject( rect );
		} else {
			DeleteObject( shape );
			shape = CreateRectRgn( 0, 0, size.x, size.y);
		}
		OffsetRgn( shape, child_offset. x, child_offset. y);
		if ( parent_shape ) CombineRgn( shape, shape, parent_shape, RGN_AND );
		dsys(h) layeredParentRegion = shape;

		ev. cmd = cmPaint;
		CWidget(h)-> message( h, &ev);

		if ( !dsys(h) options. aptLayeredRequested) {
			/* assigning opaque alpha over the child rect so Windows passes mouse events in */
			StretchBlt( ps, r.left, r.top, size.x, size.y, alpha_dc, 0, 0, 1, 1, SRCPAINT);
		}
		dsys(h) options. aptLayeredPaint = 0;

		subpaint_layered_widgets( child, ps, alpha_dc, screen_offset, shape);

		DeleteObject( shape );
		child = GetWindow( child, GW_HWNDPREV);
	}
}

Bool
apc_widget_end_paint( Handle self)
{
	objCheck false;

	hwnd_leave_paint( self);

	if ( is_apt( aptLayeredPaint )) {
		// do nothing
	} else if ( is_apt( aptLayered )) {
		RECT r;
		SIZE size;
		POINT src, pos, *ppos = NULL;
		BLENDFUNCTION bf;
		HBITMAP alpha_bm, stock_alpha_bm;
		HDC alpha_dc;
		uint32_t * alpha_pixels;

		GetWindowRect((HWND) var handle, &r);
		if ( r. right - r. left <= 0 || r. bottom - r. top <= 0)
			goto SKIP_ALPHA;

		alpha_dc = CreateCompatibleDC( sys ps );
		alpha_bm = image_create_argb_dib_section( alpha_dc, 1, 1, &alpha_pixels);
		stock_alpha_bm = SelectObject( alpha_dc, alpha_bm);
		*alpha_pixels = 0xFF000000;

		/* subpaint children */
		src. x = r. left;
		src. y = r. top;
		subpaint_layered_widgets((HWND) var handle, sys ps, alpha_dc, src, NULL);

		SelectObject( alpha_dc, stock_alpha_bm);
		DeleteObject( alpha_bm );
		DeleteDC( alpha_dc );

		size. cx = r. right - r. left;
		size. cy = r. bottom - r. top;
		src. x = 0;
		src. y = 0;
		bf. AlphaFormat         = AC_SRC_ALPHA;
		bf. SourceConstantAlpha = 255;
		bf. BlendFlags          = 0;
		bf. BlendOp             = AC_SRC_OVER;
		if (is_apt(aptMovePending)) {
			pos. x = sys layeredPos. x;
			pos. y = sys layeredPos. y;
			ppos = &pos;
			apt_clear(aptMovePending);
		}
		if ( !UpdateLayeredWindow((HWND) var handle, NULL, ppos, &size, sys ps, &src, 0, &bf, ULW_ALPHA))
			apiErr;
	SKIP_ALPHA:
		SelectObject(sys ps, sys stockBM);
		DeleteObject(sys bm);
		DeleteDC(sys ps);
		ReleaseDC(( HWND) var handle, sys ps2);
		sys ps = sys ps2 = NULL;
		sys bm = sys stockBM = NULL;
	} else if ( is_opt( optBuffered)) {
		apt_clear( aptBitmap);
		if ( sys bm != NULL) {
			if ( !BitBlt( sys ps2, sys transform2. x, sys transform2. y, var w, var h, sys ps, 0, 0, SRCCOPY)) apiErr;
			if ( sys stockBM)
				SelectObject( sys ps, sys stockBM);
			DeleteObject( sys bm);
		}

		if ( sys pal) {
			SelectPalette( sys ps2, sys pal2, 1);
			SelectPalette( sys ps, sys stockPalette, 1);
			RealizePalette( sys ps2);
			sys pal2 = NULL;
		}

		DeleteDC( sys ps);
		sys ps = sys ps2;
		sys bm = NULL;
		sys ps2 = NULL;
		sys stockBM = NULL;
	}

	if ( !is_apt(aptLayeredPaint) && sys ps != NULL) {
		if ( is_apt( aptWinPS) && is_apt( aptWM_PAINT)) {
			if ( !EndPaint(( HWND) var handle, &sys paintStruc)) apiErr;
		} else if ( is_apt( aptWinPS))
			if ( !ReleaseDC(( HWND) var handle, sys ps)) apiErr;
	}
	sys ps = NULL;
	sys pal2 = NULL;
	apt_clear( aptWinPS);
	apt_clear( aptWM_PAINT);
	apt_clear( aptCompatiblePS);
	return true;
}

Bool
apc_widget_end_paint_info( Handle self)
{
	Bool ok = true;
	objCheck false;
	hwnd_leave_paint( self);
	if ( !( ok = ReleaseDC(( HWND) var handle, sys ps))) apiErr;
	sys ps = NULL;
	apt_clear( aptWinPS);
	apt_clear( aptCompatiblePS);
	return ok;
}

Bool
apc_widget_get_clip_by_children( Handle self)
{
	objCheck false;
	return is_apt(aptClipByChildren);
}

Bool
apc_widget_get_clip_owner( Handle self)
{
	objCheck false;
	return is_apt( aptClipOwner);
}

Color
apc_widget_get_color( Handle self, int index)
{
	objCheck clInvalid;
	return sys viewColors[ index];
}

Bool
apc_widget_get_first_click( Handle self)
{
	objCheck false;
	return is_apt( aptFirstClick);
}

Handle
apc_widget_get_focused()
{
	return hwnd_to_view( GetFocus());
}

static PRECT map_Rect( Handle self, Rect * rect)
{
	RECT  __rectangle;
	objCheck ( PRECT) rect;
	GetWindowRect(( HWND) var handle, &__rectangle);
	if ( is_apt( aptClipOwner) && ( var owner != application))
		MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT) &__rectangle, 2);
	rect-> bottom = ( __rectangle. bottom - __rectangle. top) - rect-> bottom;
	rect-> top    = ( __rectangle. bottom - __rectangle. top) - rect-> top;
// that is due the difference in field placement between Rect and RECT structures
	if ( rect-> bottom > rect-> top) {
		LONG i = rect-> bottom;
		rect-> bottom = rect-> top;
		rect-> top    = i;
	}
	return ( PRECT) rect;
}

static Rect * map_RECT( Handle self, RECT * rect)
{
	RECT __rectangle;
	objCheck ( Rect*)rect;
	GetWindowRect(( HWND) var handle, &__rectangle);
	if ( is_apt( aptClipOwner) && ( var owner != application))
		MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&__rectangle, 2);
	rect-> bottom = ( __rectangle. bottom - __rectangle. top) - rect-> bottom;
	rect-> top    = ( __rectangle. bottom - __rectangle. top) - rect-> top;
// that is due the difference in field placement between Rect and RECT structures
	if ( rect-> bottom < rect-> top) {
		LONG i = rect-> bottom;
		rect-> bottom = rect-> top;
		rect-> top    = i;
	}
	return ( Rect *) rect;
}

ApiHandle
apc_widget_get_handle( Handle self)
{
	objCheck 0;
	return ( ApiHandle) HANDLE;
}

ApiHandle
apc_widget_get_parent_handle( Handle self)
{
	objCheck 0;
	return ( ApiHandle) sys parentHandle;
}


Rect
apc_widget_get_invalid_rect( Handle self)
{
	Rect r  = {0,0,0,0};
	objCheck r;
	if ( GetUpdateRect(( HWND) var handle, ( PRECT) &r, false))
		return *( map_RECT( self, ( PRECT) &r));
	return r;
}

Bool
apc_widget_get_layered_request( Handle self)
{
	return is_apt( aptLayeredRequested);
}

Bool
apc_widget_surface_is_layered( Handle self)
{
	Handle top;
	objCheck false;

	if ( is_apt( aptLayered)) return true;
	top = hwnd_layered_top_level(self);
	if ( top && dsys(top) options. aptLayered ) return true;
	return false;
}

Point
apc_widget_get_pos( Handle self)
{
	RECT  r;
	Point p, sz = {0,0};
	Handle parent;
	objCheck sz;
	parent = is_apt( aptClipOwner) ? var owner : application;
	sz = ((( PWidget) parent)-> self)-> get_size( parent);
	if ( sys className == WC_FRAME && apc_window_get_window_state( self) == wsMinimized) {
		WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
		if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
		p. x = w. rcNormalPosition. left;
		p. y = sz. y - w. rcNormalPosition. bottom;
	} else if ( is_apt( aptMovePending)) {
		GetWindowRect( HANDLE, &r);
		p. x = sys layeredPos. x;
		p. y = sys layeredPos. y + r. bottom - r.top;
	} else {
		GetWindowRect( HANDLE, &r);
		if ( is_apt( aptClipOwner) && ( var owner != application))
			MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
		p. x = r. left;
		p. y = sz. y - r. bottom;
	}
	return p;
}

Point
apc_widget_get_size( Handle self)
{
	RECT r;
	Point p = {0,0};
	objCheck p;
	if ( sys className == WC_FRAME && apc_window_get_window_state( self) == wsMinimized) {
		WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
		if ( !GetWindowPlacement( HANDLE, &w)) apiErr;
		p. x = w. rcNormalPosition. right  - w. rcNormalPosition. left;
		p. y = w. rcNormalPosition. bottom - w. rcNormalPosition. top;
	} else {
		GetWindowRect( HANDLE, &r);
		if ( is_apt( aptClipOwner) && ( var owner != application))
			MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
		p. x = r. right - r. left;
		p. y = r. bottom - r. top;
	}
	return p;
}

Handle
apc_widget_get_z_order( Handle self, int zOrderId)
{
	Handle h;
	HWND   w;
	UINT   cmd1, cmd2;
	objCheck NULL_HANDLE;

	switch ( zOrderId) {
	case zoFirst:
		cmd1 = GW_HWNDFIRST;
		cmd2 = GW_HWNDNEXT;
		break;
	case zoLast:
		cmd1 = GW_HWNDLAST;
		cmd2 = GW_HWNDPREV;
		break;
	case zoNext:
		cmd1 = cmd2 = GW_HWNDNEXT;
		break;
	case zoPrev:
		cmd1 = cmd2 = GW_HWNDPREV;
		break;
	default:
		apcErr( errInvParams);
		return NULL_HANDLE;
	}

	w = GetWindow( HANDLE, cmd1);
	if ( !w) return NULL_HANDLE;
	h = hwnd_to_view( w);
	while ( h == NULL_HANDLE) {
		w = GetWindow( w, cmd2);
		if ( !w) return NULL_HANDLE;
		h = hwnd_to_view( w);
	}

	return h;
}

Bool
apc_widget_get_shape( Handle self, Handle mask)
{
	HRGN rgn;
	int res;
	RECT rect;

	if ( !mask ) {
		rgn = CreateRectRgn(0,0,0,0);
		res = GetWindowRgn( HANDLE, rgn);
		res = GetClipRgn( sys ps, rgn );
		DeleteObject(rgn);
		return res != ERROR;
	}

	rgn = GET_REGION(mask)-> region;
	res = GetWindowRgn( HANDLE, rgn);
	if ( res == ERROR)
		return false;

	GetRgnBox(rgn, &rect);
	OffsetRgn( rgn, -sys extraPos. x, -sys extraPos. y);
	GET_REGION(mask)-> aperture = sys lastSize. y - rect.top;

	return true;
}


Bool
apc_widget_get_sync_paint( Handle self)
{
	objCheck false;
	return is_apt( aptSyncPaint);
}

Bool
apc_widget_get_transparent( Handle self)
{
	objCheck false;
	return is_apt( aptTransparent);
}

Bool
apc_widget_is_captured( Handle self)
{
	objCheck false;
	return GetCapture() == ( HWND) var handle;
}

Bool
apc_widget_is_enabled( Handle self)
{
	objCheck false;
	return is_apt( aptEnabled);
	// return IsWindowEnabled( HANDLE);
}

Bool
apc_widget_is_responsive( Handle self)
{
	Bool ena = true;
	objCheck false;
	while ( ena && self != application) {
		// ena  = IsWindowEnabled( HANDLE);
		ena  = is_apt( aptEnabled);
		self = var owner;
	}
	return ena;
}

Bool
apc_widget_is_focused( Handle self)
{
	objCheck false;
	return is_apt( aptFocused);
}

Bool
apc_widget_is_visible( Handle self)
{
	objCheck false;
	return ( GetWindowLong(HANDLE, GWL_STYLE) & WS_VISIBLE) ? 1 : 0;
}

Bool
apc_widget_is_showing( Handle self)
{
	objCheck false;
	return IsWindowVisible( HANDLE);
}

Bool
apc_widget_is_exposed( Handle self)
{
	HWND h;
	HRGN rgnSave = NULL;
	HRGN rgn     = NULL;
	int  rgnSaveType, rgnType;

	objCheck false;

	h = ( HWND) var handle;
	if ( !IsWindowVisible( h)) return false;

	rgnSave = CreateRectRgn(0,0,0,0);
	rgn     = CreateRectRgn(0,0,0,0);
	rgnSaveType = GetUpdateRgn( h, rgnSave, FALSE);
	rgnSaveType = ( rgnSaveType == COMPLEXREGION || rgnSaveType == SIMPLEREGION);

	if ( rgnSaveType) ValidateRect( h, NULL);

	InvalidateRect( h, NULL, false);
	rgnType = GetUpdateRgn( h, rgn, FALSE);
	ValidateRect( h, NULL);

	if ( rgnSaveType) InvalidateRgn( h, rgnSave, FALSE);
	DeleteObject( rgnSave);
	DeleteObject( rgn);
	return ( rgnType == COMPLEXREGION || rgnType == SIMPLEREGION);
}

Bool
apc_widget_invalidate_rect( Handle self, Rect * rect)
{
	PRECT pRect = rect ? map_Rect( self, rect) : NULL;
	objCheck false;
	if ( !InvalidateRect (( HWND) var handle, pRect, false)) apiErr;
	hwnd_repaint_layered( self, false );
	if ( is_apt( aptSyncPaint) && !apcUpdateWindow(( HWND) var handle)) apiErr;
	objCheck false;
	process_transparents( self);
	return true;
}

int
apc_widget_scroll( Handle self, int horiz, int vert, Rect * r, Rect *cr, Bool scrollChildren)
{
	PRECT pRect = r ? map_Rect( self, r) : NULL;
	PRECT pClipRect = cr ? map_Rect( self, cr) : NULL;
	Point sz = apc_widget_get_size( self);
	int ret;
	objCheck scrError;

	HideCaret(( HWND) var handle);

	if ( pClipRect) {
		if ( pClipRect-> left < 0) pClipRect-> left = 0;
		if ( pClipRect-> top  < 0) pClipRect-> top = 0;
		if ( pClipRect-> right  > sz. x) pClipRect-> right = sz. x;
		if ( pClipRect-> bottom > sz. y) pClipRect-> bottom = sz. y;
	}

	if ( pRect) {
		if ( pRect-> left < -sz. x * 2) pRect-> left = -sz. x * 2;
		if ( pRect-> top  < -sz. y * 2) pRect-> top = -sz. y * 2;
		if ( pRect-> right  > sz. x * 2) pRect-> right = sz. x * 2;
		if ( pRect-> bottom > sz. y * 2) pRect-> bottom = sz. y * 2;
	}

	if ( horiz > sz. x || horiz < -sz. x || vert > sz. y || vert < -sz. y) {
		if ( pRect && pClipRect) {
			RECT rc;
			UnionRect( &rc, (RECT*)pRect, (RECT*)pClipRect);
			InvalidateRect(( HWND) var handle, &rc, false);
		} else
			InvalidateRect(( HWND) var handle, pRect ? pRect : pClipRect, false);
		ret = scrExpose;
	} else {
		int rr = ScrollWindowEx(( HWND) var handle,
			horiz, -vert, pRect, pClipRect, NULL, NULL,
			SW_INVALIDATE | ( scrollChildren ? SW_SCROLLCHILDREN : 0)
		);
		if (!rr) apiErr;
		ret = ( rr == 1 ) ? scrNoExpose : scrExpose;
	}

	objCheck scrError;
	if ( is_apt( aptSyncPaint) && !apcUpdateWindow(( HWND) var handle)) apiErr;
	process_transparents( self);
	ShowCaret(( HWND) var handle);

	return ret;
}

Bool
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
	objCheck false;
	if ( capture) {
		SetCapture(( HWND) var handle);
		if ( confineTo) {
			RECT r;
			GetWindowRect(( HWND) PComponent( confineTo)-> handle, &r);
			if ( !ClipCursor( &r)) apiErrRet;
		}
	} else {
		if ( !ReleaseCapture()) apiErrRet;
		if ( !ClipCursor( NULL)) apiErrRet;
	}
	return true;
}

#define check_swap( parm1, parm2) if ( parm1 > parm2) { int parm3 = parm1; parm1 = parm2; parm2 = parm3;}

Bool
apc_widget_set_clip_by_children( Handle self, Bool clip_by_children)
{
	objCheck false;
	LONG f;
	MSG  msg;
	Bool is_clipped;

	is_clipped = is_apt(aptClipByChildren);
	clip_by_children = !!clip_by_children;
	if ( is_clipped == clip_by_children )
		return true;

	if ( sys className == WC_FRAME) {
		f = GetWindowLong(( HWND ) var handle, GWL_STYLE);
		if ( clip_by_children )
			f |= WS_CLIPCHILDREN;
		else
			f &= ~WS_CLIPCHILDREN;
		SetWindowLong(( HWND ) var handle, GWL_STYLE, f);
		while ( PeekMessage( &msg, (HWND) var handle, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
	}

	f = GetWindowLong(HANDLE, GWL_STYLE);
	if ( clip_by_children )
		f |= WS_CLIPCHILDREN;
	else
		f &= ~WS_CLIPCHILDREN;
	SetWindowLong( HANDLE, GWL_STYLE, f);
	while ( PeekMessage( &msg, HANDLE, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
	return true;
}

Bool
apc_widget_set_color( Handle self, Color color, int index)
{
	Event ev = {cmColorChanged};
	objCheck false;
	sys viewColors[ index] = color;

	ev. gen. source = self;
	ev. gen. i      = index;
	var self-> message( self, &ev);
	return true;
}

Bool
apc_widget_set_enabled( Handle self, Bool enable)
{
	objCheck false;
	apt_assign( aptEnabled, enable);
	if (( sys className == WC_FRAME) || ( var owner == application))
		EnableWindow( HANDLE, enable);
	else
		SendMessage( HANDLE, WM_ENABLE, ( WPARAM) enable, 0);
	return true;
}

Bool
apc_widget_set_first_click( Handle self, Bool firstClick)
{
	objCheck false;
	apt_assign( aptFirstClick, firstClick);
	return true;
}

Bool
apc_widget_set_focused( Handle self)
{
	if ( self && ( self != Application_map_focus( application, self)))
		return false;
	guts. focSysGranted++;
	SetFocus( self ? (( HWND) var handle) : NULL);
	guts. focSysGranted--;
	return true;
}

Bool
apc_widget_set_font( Handle self, PFont font)
{
	Event ev = {cmFontChanged};
	objCheck false;
	ev. gen. source = self;
	var self-> message( self, &ev);
	return true;
}

Bool
apc_widget_set_palette( Handle self)
{
	objCheck false;
	apc_gp_set_palette( self);
	if ( guts. displayBMInfo. bmiHeader. biBitCount == 8)
		palette_change( self);
	return true;
}

Bool
apc_widget_set_pos( Handle self, int x, int y)
{
	Handle parent;
	Point sz;
	RECT r;

	objCheck false;
	if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);

	parent = is_apt( aptClipOwner) ? var owner : application;
	sz = ((( PWidget) parent)-> self)-> get_size( parent);
	apt_set( aptWinPosDetermined);

	if ( sys className == WC_FRAME) {
		HWND h = HANDLE;
		int  ws = apc_window_get_window_state( self);
		if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
			WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
			if ( !GetWindowPlacement( h, &w)) apiErrRet;
			w. rcNormalPosition. top    += sz. y - y - w. rcNormalPosition. bottom;
			w. rcNormalPosition. bottom  = sz. y - y;
			w. rcNormalPosition. right  += x - w. rcNormalPosition. left;
			w. rcNormalPosition. left    = x;
			w. flags = 0;
			if ( !SetWindowPlacement( h, &w)) apiErrRet;
			return true;
		}
	}
	if ( !GetWindowRect( HANDLE, &r)) apiErrRet;
	if ( is_apt( aptClipOwner) && ( var owner != application))
		MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
	if ( sys parentHandle) {
		POINT ppos;
		ppos. x = x;
		ppos. y = dsys( application) lastSize. y - y;
		MapWindowPoints( NULL, sys parentHandle, ( LPPOINT)&ppos, 1);
		GetWindowRect( sys parentHandle, &r);
		x = ppos. x;
		y = ppos. y;
	} else
		y = sz. y - y - r. bottom + r. top;

	if ( is_apt(aptLayered) && is_apt(aptRepaintPending)) {
		apt_set(aptMovePending);
		sys layeredPos. x = x;
		sys layeredPos. y = y;
	} else {
		if ( !SetWindowPos( HANDLE, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErrRet;
	}
	return true;
}

Bool
apc_widget_set_size( Handle self, int width, int height)
{
	RECT r;
	HWND h;
	objCheck false;

	if ( !hwnd_check_limits( width, height, false)) apcErrRet( errInvParams);
	h = HANDLE;
	if ( sys className == WC_FRAME) {
		int  ws = apc_window_get_window_state( self);
		if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
			WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
			if ( !GetWindowPlacement( h, &w)) apiErrRet;
			if ( width  < 0) width = 0;
			if ( height < 0) height = 0;
			w. rcNormalPosition. top    = w. rcNormalPosition. bottom - height;
			w. rcNormalPosition. right  = width + w. rcNormalPosition. left;
			w. flags = 0;
			if ( !SetWindowPlacement( h, &w)) apiErrRet;
			return true;
		}
	}
	if ( !GetWindowRect( h, &r)) apiErrRet;
	if ( is_apt( aptClipOwner) && ( var owner != application))
		MapWindowPoints( NULL, ( HWND)((( PWidget) var owner)-> handle), ( LPPOINT)&r, 2);
	if ( sys parentHandle)
		MapWindowPoints( NULL, sys parentHandle, ( LPPOINT)&r, 2);

	if ( sys className != WC_FRAME) {
		sys sizeLockLevel++;
		var virtualSize. x = width;
		var virtualSize. y = height;
	}
	if ( height < 0) height = 0;
	if ( width  < 0) width  = 0;
	if ( is_apt(aptMovePending)) {
		int dx = r. right  - r. left;
		int dy = r. bottom - r. top;
		r. left = sys layeredPos. x;
		r. top  = sys layeredPos. y;
		r. right  = r. left + dx;
		r. bottom = r. top  + dy;
		apt_clear(aptMovePending);
	}
	if ( !SetWindowPos( h, 0,
		r. left, r. bottom - height,
		width, height,
		SWP_NOZORDER | SWP_NOACTIVATE |
			( is_apt( aptWinPosDetermined) ? 0 : SWP_NOMOVE)
		)) apiErrRet;
	hwnd_repaint_layered( self, false );
	if ( sys className != WC_FRAME) sys sizeLockLevel--;
	return true;
}

Bool
apc_widget_set_rect( Handle self, int x, int y, int width, int height)
{
	HWND h;
	Handle parent;
	Point sz;
	objCheck false;

	if ( !hwnd_check_limits( width, height, false)) apcErrRet( errInvParams);
	if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);

	parent = is_apt( aptClipOwner) ? var owner : application;
	sz = ((( PWidget) parent)-> self)-> get_size( parent);
	apt_set( aptWinPosDetermined);

	h = HANDLE;
	if ( sys className == WC_FRAME) {
		int  ws = apc_window_get_window_state( self);
		if (( var stage == csConstructing && ws != wsNormal) || ( ws == wsMinimized)) {
			WINDOWPLACEMENT w = {sizeof(WINDOWPLACEMENT)};
			if ( !GetWindowPlacement( h, &w)) apiErrRet;
			if ( width  < 0) width = 0;
			if ( height < 0) height = 0;
			w. rcNormalPosition. left    = x;
			w. rcNormalPosition. bottom  = sz. y - y;
			w. rcNormalPosition. right   = x + width;
			w. rcNormalPosition. top     = sz. y - y - height;
			w. flags = 0;
			if ( !SetWindowPlacement( h, &w)) apiErrRet;
			return true;
		}
	}

	if ( sys className != WC_FRAME) {
		sys sizeLockLevel++;
		var virtualSize. x = width;
		var virtualSize. y = height;
	}
	if ( height < 0) height = 0;
	if ( width  < 0) width  = 0;
	if ( sys parentHandle) {
		POINT ppos;
		ppos. x = x;
		ppos. y = dsys( application) lastSize. y - y;
		MapWindowPoints( NULL, sys parentHandle, ( LPPOINT)&ppos, 1);
		x = ppos. x;
		y = ppos. y;
	} else
		y = sz. y - y - height;

	apt_clear(aptMovePending);
	if ( !SetWindowPos( h, 0, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE))
		apiErrRet;
	hwnd_repaint_layered( self, false );
	if ( sys className != WC_FRAME) sys sizeLockLevel--;
	return true;
}


Bool
apc_widget_set_size_bounds( Handle self, Point min, Point max)
{
	return true;
}

Bool
apc_widget_set_shape( Handle self, Handle mask)
{
	HRGN rgn = NULL;
	RECT xr;
	objCheck false;

	if ( !mask) {
		if ( sys className == WC_FRAME && is_apt(aptLayered))
			SetWindowRgn((HWND) var handle, NULL, true);
		else
			SetWindowRgn( HANDLE, NULL, true);
		hwnd_repaint_layered( self, false );
		return true;
	}


	rgn = CreateRectRgn(0,0,0,0);
	CombineRgn( rgn, GET_REGION(mask)->region, NULL, RGN_COPY);
	GetRgnBox( rgn, &xr);
	sys extraBounds. x = xr. right - 1;
	sys extraBounds. y = GET_REGION(mask)->aperture;
	if ( sys className == WC_FRAME && !is_apt(aptLayered)) {
		Point delta = get_window_borders( sys s. window. borderStyle);
		Point sz    = apc_widget_get_size( self);
		Point p     = sys extraBounds;
		HRGN  r1, r2;

		OffsetRgn( rgn, delta.x, sz. y - p.y - delta.y);
		sys extraPos. x = delta.x;
		sys extraPos. y = sz. y - p.y - delta.y;
		r1 = CreateRectRgn( 0, 0, 8192, 8192);
		r2 = CreateRectRgn( delta. x, sz. y - delta. y - p.y,
			delta.x + p.x, sz. y - delta. y);
		CombineRgn( r1, r1, r2, RGN_XOR);
		CombineRgn( rgn, rgn, r1, RGN_OR);
		DeleteObject( r1);
		DeleteObject( r2);
		if ( !SetWindowRgn( HANDLE, rgn, true))
			apiErrRet;
		DeleteObject(rgn);
	} else if ( sys className == WC_FRAME ) {
		sys extraPos. x = sys extraPos. y = 0;
		if ( !SetWindowRgn((HWND) var handle, rgn, true))
			apiErrRet;
	} else {
		sys extraPos. x = sys extraPos. y = 0;
		if ( !SetWindowRgn( HANDLE, rgn, true))
			apiErrRet;
	}
	DeleteObject(rgn);

	if ( is_apt(aptMovePending)) {
		apt_clear(aptMovePending);
		if ( !SetWindowPos( HANDLE, 0, sys layeredPos. x, sys layeredPos. y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE))
			apiErrRet;
	}
	hwnd_repaint_layered( self, false );
	return true;
}

Bool
apc_widget_set_visible( Handle self, Bool show)
{
	objCheck false;
	if ( !SetWindowPos( HANDLE, NULL, 0, 0, 0, 0,
		( show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)
		| SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) apiErrRet;
	objCheck false;
	if ( !is_apt( aptClipOwner)) {
		InvalidateRect(( HWND) var handle, NULL, false);
		objCheck false;
		process_transparents( self);
	}
	hwnd_repaint_layered(self, false);
	return true;
}

Bool
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
	HWND opt = ( top) ? HWND_TOP : HWND_BOTTOM;
	objCheck false;
	if ( behind != NULL_HANDLE) {
		dobjCheck( behind) false;
		opt = DHANDLE( behind);
	}
	if ( !SetWindowPos( HANDLE, opt, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)) apiErrRet;
	return true;
}

Bool
apc_widget_update( Handle self)
{
	objCheck false;
	if ( !apcUpdateWindow(( HWND) var handle)) apiErrRet;
	return true;
}

Bool
apc_widget_validate_rect( Handle self, Rect rect)
{
	objCheck false;
	if ( !ValidateRect (( HWND) var handle, map_Rect( self, &rect))) apiErrRet;
	return true;
}


#ifdef __cplusplus
}
#endif
