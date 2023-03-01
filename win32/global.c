#include "win32\win32guts.h"
#include <ole2.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Window.h"
#include "Application.h"
#include "Menu.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

Bool          debug            = false;
int           FONTSTRUCSIZE;
WinGuts       guts;
Handle        last_mouse_over  = NULL_HANDLE;
WCHAR         last_dead_key    = 0;
int           time_defs_count  = 0;
PItemRegRec   time_defs        = NULL;
MouseClickRec mouse_click      = {0};
PHash         mgr_styli        = NULL; // pen and brush manager
PHash         mgr_fonts        = NULL; // font manager
PHash         mgr_patterns     = NULL; // pattern resource manager
PHash         mgr_menu         = NULL; // HMENU manager
PHash         mgr_images       = NULL; // HBITMAP manager
PHash         mgr_registry     = NULL; // cache for apc_widget_user_profile
PHash         mgr_myfonts      = NULL; // hash of calls to apc_font_load
PHash         mgr_menu_bitmaps = NULL; // HBITMAP manager for SetMenuItemBitmaps 
PHash         mgr_scripts      = NULL; // SCRIPT_CACHE entries per font/script
DWORD         rc;
HCURSOR       std_arrow_cursor;
HBRUSH        std_hollow_brush;
LinePattern   std_hollow_line_pattern;
HPEN          std_hollow_pen;
HBITMAP       std_unchecked_bitmap = NULL;

char *        key_layouts[]   = {  "0409", "0403", "0405", "0406", "0407",
	"0807","0809","080A","080C","0C0C","100C","0810","0814","0816",
	"040A","040B","040C","040E","040F","0410","0413","0414","0415","0416",
	"0417","0418","041A","041D"
};

extern Handle ctx_kb2VK[];
extern Handle ctx_kb2VK2[];
extern Handle ctx_kb2VK3[];


BOOL APIENTRY
DllMain( HINSTANCE hInstance, DWORD reason, LPVOID reserved)
{
	if ( reason == DLL_PROCESS_ATTACH) {
		memset( &guts, 0, sizeof( guts));
		guts. instance = hInstance;
		guts. cmd_show  = SW_SHOWDEFAULT;
	}
	return TRUE;
}

typedef enum _PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE            = 0,
	PROCESS_SYSTEM_DPI_AWARE       = 1,
	PROCESS_PER_MONITOR_DPI_AWARE  = 2
} PROCESS_DPI_AWARENESS;

typedef enum _MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI              = 0,
	MDT_ANGULAR_DPI                = 1,
	MDT_RAW_DPI                    = 2
} MONITOR_DPI_TYPE;

#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONULL            0
#define MONITOR_DEFAULTTOPRIMARY         1
#define MONITOR_DEFAULTTONEAREST         2
#endif

static HRESULT (__stdcall *SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS)  = NULL;
static HRESULT (__stdcall *GetDpiForMonitor)(HMONITOR,MONITOR_DPI_TYPE,UINT*,UINT*) = NULL;
static HRESULT (__stdcall *DwmIsCompositionEnabled)(BOOL *pfEnabled) = NULL;
static BOOL    (__stdcall *ReadConsoleInputExW)( WINHANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead, USHORT wFlags) = NULL;

#if WINVER < 0x0600
static BOOL    (__stdcall *GetUserPreferredUILanguages)(DWORD dwFlags, PULONG pulNumLanguages, PZZWSTR pwszLanguagesBuffer, PULONG pcchLanguagesBuffer) = NULL;
#endif

BOOL
my_GetUserPreferredUILanguages(
	DWORD dwFlags, PULONG pulNumLanguages,
	PZZWSTR pwszLanguagesBuffer, PULONG pcchLanguagesBuffer
) {
#if WINVER < 0x0600
	if ( GetUserPreferredUILanguages == NULL) 
		return false;
#endif
	return GetUserPreferredUILanguages(dwFlags, pulNumLanguages, pwszLanguagesBuffer, pcchLanguagesBuffer);
}

void
dpi_change(void)
{
	UINT dx, dy;
	POINT pt = {1,1};
	HMONITOR m = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	if ( GetDpiForMonitor && (GetDpiForMonitor( m, MDT_EFFECTIVE_DPI, &dx, &dy) == S_OK )) {
		guts. display_resolution. x = dx;
		guts. display_resolution. y = dy;
	}
}

Bool
is_dwm_enabled( void )
{
	if ( DwmIsCompositionEnabled ) {
		BOOL b;
		if ( DwmIsCompositionEnabled(&b) != S_OK) goto NOPE;
		return b;
	} else {
		HKEY hKey;
		DWORD valSize = 256, valType = REG_SZ, dw = 0;
	NOPE:
		if ( LOBYTE(LOWORD(guts.version)) > 5 )
			return 1;
		valType = REG_DWORD;
		valSize = sizeof(DWORD);
		if ( RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\DWM", 0, KEY_READ, &hKey) == 0 ) {
			if ( RegQueryValueEx( hKey, "CompositionPolicy", NULL, &valType, ( LPBYTE)&dw, &valSize) != 0 )
				dw = 1;
			RegCloseKey( hKey);
			return dw == 0;
		} else
			return 0;
	}
}

Bool
read_console_input( WINHANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead, USHORT wFlags)
{
	return ReadConsoleInputExW( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead, wFlags);
}

static void
load_function(HMODULE module, void ** ptr, const char * name)
{
	(*ptr) = (void*) GetProcAddress(module, name);
}

BOOL WINAPI
win32_ctrlhandler(DWORD dwCtrlType)
{
	PostThreadMessage( guts. main_thread_id, WM_SIGNAL, dwCtrlType, 0);
	return FALSE;
}

Bool
window_subsystem_init( char * error_buf)
{
	WNDCLASSW wc;
	HDC dc;
	HBITMAP hbm;
	OSVERSIONINFO os = { sizeof( OSVERSIONINFO)};
	GdiplusStartupInput gdiplusStartupInputDef = { 1, NULL, FALSE, FALSE };

	guts. version  = GetVersion();
	GetVersionEx( &os);
	guts. utf8_prepend_0x202D =
		(( os.dwMajorVersion > 5) || (os.dwMajorVersion == 5 && os.dwMinorVersion > 1)) ?
					1 : 0;
	guts. alloc_utf8_to_wchar_visual =
		guts. utf8_prepend_0x202D ?
			alloc_utf8_to_wchar_visual :
			alloc_utf8_to_wchar;
	guts. main_thread_id = GetCurrentThreadId();
	guts. error_mode = SetErrorMode( SEM_FAILCRITICALERRORS);
	guts. desktop_window = GetDesktopWindow();
	std_arrow_cursor    = LoadCursor( NULL, IDC_ARROW);

	memset( &wc, 0, sizeof( wc));
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = ( WNDPROC) generic_app_handler;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = guts. instance;
	wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)NULL;
	wc.lpszClassName = L"GenericApp";
	RegisterClassW( &wc);

	memset( &wc, 0, sizeof( wc));
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = ( WNDPROC) generic_frame_handler;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = guts. instance;
	wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
	wc.hCursor       = std_arrow_cursor;
	wc.hbrBackground = (HBRUSH)NULL;
	wc.lpszClassName = L"GenericFrame";
	RegisterClassW( &wc);

	memset( &wc, 0, sizeof( wc));
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = ( WNDPROC) layered_frame_handler;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = guts. instance;
	wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
	wc.hCursor       = std_arrow_cursor;
	wc.hbrBackground = (HBRUSH)NULL;
	wc.lpszClassName = L"LayeredFrame";
	RegisterClassW( &wc);

	memset( &wc, 0, sizeof( wc));
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = ( WNDPROC) generic_view_handler;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = guts. instance;
	wc.hIcon         = LoadIcon( guts. instance, IDI_APPLICATION);
	wc.hCursor       = NULL; // LoadCursor( NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)NULL;
	wc.lpszClassName = L"Generic";
	RegisterClassW( &wc);

	mgr_styli        = hash_create();
	mgr_fonts        = hash_create();
	mgr_patterns     = hash_create();
	mgr_menu         = hash_create();
	mgr_images       = hash_create();
	mgr_registry     = hash_create();
	mgr_myfonts      = hash_create();
	mgr_menu_bitmaps = hash_create();
	mgr_scripts      = hash_create();
	create_font_hash();
	{
		LOGBRUSH b = { BS_HOLLOW, 0, 0};
		Font f;
		std_hollow_pen       = CreatePen( PS_NULL, 0, 0);
		std_hollow_brush     = CreateBrushIndirect( &b);
		std_hollow_line_pattern.count = 0;
		std_hollow_line_pattern.ptr   = NULL;
		FONTSTRUCSIZE    = (char *)(&(f. name)) - (char *)(&f);
	}

	if (!( dc = dc_alloc())) return false;
	guts. display_resolution. x = GetDeviceCaps( dc, LOGPIXELSX);
	guts. display_resolution. y = GetDeviceCaps( dc, LOGPIXELSY);

	/* Win7 DWM */
#define LOAD_FUNC(m,f) load_function(m, (void**) &f, #f)
	if ( os.dwMajorVersion >= 5) {
		HMODULE mod = LoadLibrary("DWMAPI.DLL");
		if ( mod ) {
			LOAD_FUNC(mod, DwmIsCompositionEnabled);
		}
#if WINVER < 0x0600
		mod = LoadLibrary("KERNEL32.DLL");
		if ( mod ) {
			LOAD_FUNC(mod, GetUserPreferredUILanguages);
			LOAD_FUNC(mod, ReadConsoleInputExW);
		}
#endif
	}

	/* Win8 - high dpi awareness stuff */
	if (( os.dwMajorVersion > 5) || (os.dwMajorVersion == 5 && os.dwMinorVersion > 1)) {
		HMODULE shcore = LoadLibrary("SHCORE.DLL");
		if ( shcore ) {
			LOAD_FUNC(shcore, SetProcessDpiAwareness);
			LOAD_FUNC(shcore, GetDpiForMonitor);
		}
		if (
			SetProcessDpiAwareness && GetDpiForMonitor &&
			(SetProcessDpiAwareness( PROCESS_PER_MONITOR_DPI_AWARE) == S_OK )
		)
			dpi_change();
	}
#undef LOAD_FUNC

	{
		LOGFONT lf;
		HFONT   sfont;

		// getting most common font name
		memset( &lf, 0, sizeof( lf));
		lf. lfCharSet        = OEM_CHARSET;
		lf. lfOutPrecision   = OUT_DEFAULT_PRECIS;
		lf. lfClipPrecision  = CLIP_DEFAULT_PRECIS;
		lf. lfQuality        = PROOF_QUALITY;
		lf. lfPitchAndFamily = DEFAULT_PITCH;
		sfont = SelectObject( dc, CreateFontIndirect( &lf));
		GetTextFace( dc, 256, guts. default_system_font);

		// getting common fixed font name
		lf. lfHeight = 320;
		lf. lfPitchAndFamily = FIXED_PITCH;
		DeleteObject( SelectObject( dc, CreateFontIndirect( &lf)));
		GetTextFace( dc, 256, guts. default_fixed_font);

		// getting common variable font name
		lf. lfPitchAndFamily = VARIABLE_PITCH;
		DeleteObject( SelectObject( dc, CreateFontIndirect( &lf)));
		GetTextFace( dc, 256, guts. default_variable_font);
		DeleteObject( SelectObject( dc, sfont));

		// getting system font presets
		reset_system_fonts();
		register_mapper_fonts();
	}

	memset( &guts. display_bm_info, 0, sizeof( guts. display_bm_info));
	guts. display_bm_info. bmiHeader. biSize = sizeof( BITMAPINFO);
	if ( !( hbm = GetCurrentObject( dc, OBJ_BITMAP))) {
		apiErr;
		dc_free();
		return false;
	}

	if ( !GetDIBits( dc, hbm, 0, 0, NULL, &guts. display_bm_info, DIB_PAL_COLORS)) {
		guts. display_bm_info. bmiHeader. biBitCount = GetDeviceCaps( dc, BITSPIXEL);
		guts. display_bm_info. bmiHeader. biPlanes   = GetDeviceCaps( dc, PLANES);
	}

	dc_free();
	guts. insert_mode = true;
	guts. icon_size_small. x = GetSystemMetrics( SM_CXSMICON);
	guts. icon_size_small. y = GetSystemMetrics( SM_CYSMICON);
	guts. icon_size_large. x = GetSystemMetrics( SM_CXICON);
	guts. icon_size_large. y = GetSystemMetrics( SM_CYICON);
	guts. pointer_size. x    = GetSystemMetrics( SM_CXCURSOR);
	guts. pointer_size. y    = GetSystemMetrics( SM_CYCURSOR);
	list_create( &guts. transp, 8, 8);

	// selecting locale layout, more or less latin-like

	{
		char buf[ KL_NAMELENGTH * 2] = "";
		HKL current      = GetKeyboardLayout( 0);
		int i, j, size   = GetKeyboardLayoutList( 0, NULL);
		HKL * kl         = ( HKL *) malloc( sizeof( HKL) * size);

		guts. key_layout = NULL;
		if ( !GetKeyboardLayoutName( buf)) apiErr;
		for ( j = 0; j < ( sizeof( key_layouts) / sizeof( char*)); j++) {
			if ( strncmp( buf + 4, key_layouts[ j], 4) == 0) {
				guts. key_layout = current;
				goto found_1;
			}
		}

		if ( kl) {
			GetKeyboardLayoutList( size, kl);
			for ( i = 0; i < size; i++) {
				ActivateKeyboardLayout( kl[ i], 0);
				if ( !GetKeyboardLayoutName( buf)) apiErr;
				for ( j = 0; j < ( sizeof( key_layouts) / sizeof( char*)); j++) {
					if ( strncmp( buf + 4, key_layouts[ j], 4) == 0) {
						guts. key_layout = kl[ i];
						goto found_2;
					}
				}
			}
		found_2:;
			ActivateKeyboardLayout( current, 0);
		}
	found_1:;
		free( kl);
	}
	guts. current_key_state = guts. key_state;
	memset( guts. empty_key_state, 0, sizeof( guts. empty_key_state));
	guts. cmDOUBLECLK. x = GetSystemMetrics( SM_CXDOUBLECLK);
	guts. cmDOUBLECLK. y = GetSystemMetrics( SM_CYDOUBLECLK);

	GdiplusStartup(&guts.gdiplus_token, &gdiplusStartupInputDef, NULL);
	{
		HRESULT r = OleInitialize(NULL);
		guts. ole_initialized = (r == S_OK || r == S_FALSE );
	}

	switch (GetACP()) {
	case 50220: case 50221: case 50222: case 50225: case 50227: case 50229:
	case 57002: case 57003: case 57004: case 57005: case 57006: case 57007:
	case 57008: case 57009: case 57010: case 57011: case 65000: case 42:
		guts.wc2mb_is_fragile = true;
		break;
	default:
		guts.wc2mb_is_fragile = false;
	}

	if ( !SetConsoleCtrlHandler(win32_ctrlhandler, true))
		apiErr;

	{
		/* Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC). */
		FILETIME ft;
		HKEY hKey;
		DWORD valSize = 256, valType = REG_SZ;
		char buf[ 256] = "";

		GetSystemTimeAsFileTime(&ft);
		guts.program_start_ts = ft.dwHighDateTime * 10000 + ft.dwLowDateTime / 10000;

		RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Mouse", 0, KEY_READ, &hKey);
		RegQueryValueEx( hKey, "DoubleClickSpeed", NULL, &valType, ( LPBYTE)buf, &valSize);
		RegCloseKey( hKey);
		guts.mouse_double_click_delay = atol( buf);
		if (guts.mouse_double_click_delay < 1 ) guts.mouse_double_click_delay = 50;
	}

	if ( !file_subsystem_init())
		return false;

	return true;
}

Bool
window_subsystem_get_options( int * argc, char *** argv)
{
	static char * win32_argv[] = {
		"debug", "turns on debugging"
	};
	*argv = win32_argv;
	*argc = sizeof( win32_argv) / sizeof( char*);
	return true;
}

Bool
window_subsystem_set_option( char * option, char * value)
{
	if ( strcmp( option, "debug") == 0) {
		debug = value ? *value != '0' : true;
		return true;
	}
	return false;
}

static Bool
myfont_cleaner( void * value, int keyLen, void * key, void * dummy)
{
	RemoveFontResource((LPCTSTR)key);
	return false;
}

static Bool
menu_bitmap_cleaner( void * value, int keyLen, void * key, void * dummy)
{
	DeleteObject((HBITMAP) value);
	return false;
}

void
window_subsystem_done()
{
	if (guts. ole_initialized)
		OleUninitialize();
	free( time_defs);
	time_defs = NULL;

	file_subsystem_done();

	list_destroy( &guts. transp);
	destroy_font_hash();

	font_clean();
	stylus_clean();

	GdiplusShutdown(guts.gdiplus_token);

	hash_destroy( mgr_images,       false);
	hash_destroy( mgr_menu,         false);
	hash_destroy( mgr_patterns,     true);
	hash_destroy( mgr_fonts,        true);
	hash_destroy( mgr_styli,        true);
	hash_destroy( mgr_registry,     false);

	hash_first_that( mgr_menu_bitmaps, menu_bitmap_cleaner, NULL, NULL, NULL);
	hash_destroy( mgr_menu_bitmaps, false);
	hash_destroy( mgr_scripts,      true);

	hash_first_that( mgr_myfonts, myfont_cleaner, NULL, NULL, NULL);
	hash_destroy( mgr_myfonts,      false);
	DeleteObject( std_hollow_pen);
	DeleteObject( std_hollow_brush);
	if ( std_unchecked_bitmap && std_unchecked_bitmap != (HBITMAP)-1)
		DeleteObject( std_unchecked_bitmap);
	SetErrorMode( guts. error_mode);

	if ( guts.get_pixel_needs_emulation == 1 ) {
		DeleteDC( guts.get_pixel_dc_dst);
		dc_free();
	}
}

void
window_subsystem_cleanup()
{
	while ( guts. app_lock > 0) apc_application_unlock( prima_guts.application);
	while ( guts. pointer_lock < 0) {
		ShowCursor( 1);
		guts. pointer_lock++;
	}
}

static char err_buf[ 256] = "";
char * err_msg( DWORD errId, char * buffer)
{
	LPVOID lpMsgBuf;
	int len;
	if ( buffer == NULL) buffer = err_buf;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errId,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		( LPTSTR) &lpMsgBuf, 0, NULL);
	if ( lpMsgBuf)
		strlcpy( buffer, ( const char *) lpMsgBuf, 256);
	else
		buffer[0] = 0;
	buffer[ 255] = 0;
	LocalFree( lpMsgBuf);

	/* chomp! */
	len = strlen(buffer);
	while ( len > 0) {
		len--;
		if ( buffer[len] != '\xD' && buffer[len] != '\xA' && buffer[len] != '.')
			break;
		buffer[len] = 0;
	}

	return buffer;
}

char *
err_msg_gplus( GpStatus errId, char * buffer)
{
	if ( buffer == nil) buffer = err_buf;
	switch(errId) {
	case Ok                        : strcpy(buffer, "Ok");                               break;
	case GenericError              : strcpy(buffer, "GDI+ generic error");               break;
	case InvalidParameter          : strcpy(buffer, "GDI+ invalid parameter");           break;
	case OutOfMemory               : strcpy(buffer, "GDI+ out of memory");               break;
	case ObjectBusy                : strcpy(buffer, "GDI+ object busy");                 break;
	case InsufficientBuffer        : strcpy(buffer, "GDI+ insufficient buffer");         break;
	case NotImplemented            : strcpy(buffer, "GDI+ not implemented");             break;
	case Win32Error                : strcpy(buffer, "GDI+ Win32 error");                 break;
	case WrongState                : strcpy(buffer, "GDI+ Wrong state");                 break;
	case Aborted                   : strcpy(buffer, "GDI+ aborted");                     break;
	case FileNotFound              : strcpy(buffer, "GDI+ file not found");              break;
	case ValueOverflow             : strcpy(buffer, "GDI+ value overflow");              break;
	case AccessDenied              : strcpy(buffer, "GDI+ access denied");               break;
	case UnknownImageFormat        : strcpy(buffer, "GDI+ unknown image format");        break;
	case FontFamilyNotFound        : strcpy(buffer, "GDI+ font family not found");       break;
	case FontStyleNotFound         : strcpy(buffer, "GDI+ font style not found");        break;
	case NotTrueTypeFont           : strcpy(buffer, "GDI+ not a TrueType font");         break;
	case UnsupportedGdiplusVersion : strcpy(buffer, "GDI+ unsupported Gdiplus version"); break;
	case GdiplusNotInitialized     : strcpy(buffer, "GDI+ not initialized");             break;
	case PropertyNotFound          : strcpy(buffer, "GDI+ property not found");          break;
	case PropertyNotSupported      : strcpy(buffer, "GDI+ property not supported");      break;
	case ProfileNotFound           : strcpy(buffer, "GDI+ profile not found");           break;
	default                        : strcpy(buffer, "GDI+ unknown error");
	}
	return buffer;
}

char *
apc_last_error(void)
{
	switch (guts.apc_error) {
	case errApcError              : return err_buf;
	case errOk                    : return NULL;
	case errInvObject             : return "Bad object";
	case errInvParams             : return "Bad parameters";
	case errInvWindowIcon         : return "Bad window icon";
	case errInvClipboardData      : return "Bad clipboard request";
	case errInvPrinter            : return "Bad printer request";
	case errNoPrinters            : return "No printers";
	case errUserCancelled         : return "User cancelled";
	default                       : return "Unknown error";
	}
}

static Bool move_back( PWidget self, PWidget child, int * delta)
{
	RECT r;
	int oStage = child-> stage;

	if ( !dsys( child) options. aptClipOwner) return false;

	child-> stage = csFrozen;
	GetWindowRect( DHANDLE( child), &r);
	if ( dsys( child) options. aptClipOwner)
		MapWindowPoints( NULL, ( HWND) self-> handle, ( LPPOINT)&r, 2);
	SetWindowPos( DHANDLE( child), 0, r. left, r. top + *delta, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	child-> stage = oStage;

	return false;
}


static Bool
local_wnd( HWND who, HWND client)
{
	PComponent v;
	Handle self;
	if ( who == client)
		return true;
	self = GetWindowLongPtr( client, GWLP_USERDATA);
	v = (PComponent) hwnd_to_view( who);
	while (v && ( Handle) v != prima_guts.application)
	{
		if ( (Handle)v == self) return true;
		v = ( PComponent) ( v-> owner);
	}
	return false;
}

static Bool
find_oid( PAbstractMenu menu, PMenuItemReg m, int id)
{
	return m-> down && ( m-> down-> id == id);
}

Handle ctx_deadkeys[] = {
	0x5E, 0x302, // Circumflex accent
	0x60, 0x300, // Grave accent
	0xA8, 0x308, // Diaeresis
	0xB4, 0x301, // Acute accent
	0xB8, 0x327, // Cedilla
	endCtx
};

static void
zorder_sync( Handle self, HWND me, LPWINDOWPOS lp)
{
	if ( lp-> hwndInsertAfter == HWND_TOP ||
		lp-> hwndInsertAfter == HWND_NOTOPMOST ||
		lp-> hwndInsertAfter == HWND_TOPMOST) {
		me = GetNextWindow( me, GW_HWNDPREV);
		if ( me)
			PostMessage( me, WM_ZORDERSYNC, 0, 0);
	} else if ( lp-> hwndInsertAfter == HWND_BOTTOM) {
		me = GetNextWindow( me, GW_HWNDNEXT);
		if ( me)
			PostMessage( me, WM_ZORDERSYNC, 0, 0);
	}
}

static Bool
id_match( Handle self, PMenuItemReg m, void * params)
{
	return m-> id == *(( int*) params);
}

static unsigned long
get_current_timestamp()
{
	/* Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC). */
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ft.dwHighDateTime * 10000 + ft.dwLowDateTime / 10000 - guts.program_start_ts;
}

static Byte
get_mouse_fingerprint( WPARAM mp1 )
{
	return
		(( mp1 & MK_CONTROL )         ? kmCtrl   : 0) |
		(( mp1 & MK_SHIFT   )         ? kmShift  : 0) |
		(( GetKeyState( VK_MENU) < 0) ? kmAlt    : 0);
}

LRESULT CALLBACK generic_view_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
	LRESULT ret = 0;
	Handle  self   = GetWindowLongPtr( win, GWLP_USERDATA);
	PWidget v      = ( PWidget) self;
	UINT    orgMsg = msg;
	Event   ev;
	Bool    hiStage   = false;
	Bool    message_result = true;

	if ( !self || prima_guts.app_is_dead)
		return DefWindowProcW( win, msg, mp1, mp2);

	memset( &ev, 0, sizeof (ev));
	ev. gen. source = self;

	switch ( msg) {
	case WM_NCACTIVATE:
		// if activation or deactivation is concerned with declipped window ( e.g.self),
		// notify its top level frame so that it will have the chance to redraw itself correspondingly
		if ( is_declipped_child( self) && !Widget_is_child( hwnd_to_view(( HWND) mp2), hwnd_top_level( self))) {
			Handle x = hwnd_top_level( self);
			if ( x) SendMessage( DHANDLE( x), WM_NCACTIVATE, mp1, mp2);
		}
		break;
	case WM_MOUSEACTIVATE:
		// if pointing to non-active frame, but its declipped child is active at the moment,
		// cancel activation - it could produce unwilling focus changes
		if ( sys class_name == WC_FRAME) {
			Handle x = hwnd_to_view( GetActiveWindow());
			if ( is_declipped_child(x) && Widget_is_child( x, self))
				return MA_NOACTIVATE;
		}
		break;
	case WM_CLOSE:
		if ( guts. sys_focus_dialog) return 0;
		if ( sys class_name != WC_FRAME)
			return 0;
		break;
	case WM_COMMAND:
		if (( HIWORD( mp1) == 0 /* menu source */) && ( mp2 == 0)) {
			if ( LOWORD( mp1) <= MENU_ID_AUTOSTART) {
				HWND active = GetFocus();
				if ( active != NULL) SendMessage( active, LOWORD( mp1), 0, 0);
			} else if ( sys last_menu) {
				PAbstractMenu a = ( PAbstractMenu) sys last_menu;
				if ( a-> stage <= csNormal)
					a-> self-> sub_call_id(( Handle) a, LOWORD( mp1) - MENU_ID_AUTOSTART);
			}
		}
		break;
	case WM_CONTEXTMENU:
		{
			POINT a;
			a. x = ( short)LOWORD( mp2);
			a. y = ( short)HIWORD( mp2);
			ev. cmd       = cmPopup;
			// mouse event
			ev. gen. B    = ( GetKeyState( VK_LBUTTON) < 0) | ( GetKeyState( VK_RBUTTON) < 0);
			if ( !ev. gen. B && GetSystemMetrics( SM_MOUSEPRESENT))
				GetCursorPos(( POINT*) &a);

			MapWindowPoints( NULL, win, &a, 1);
			ev. gen. P. x = a. x;
			ev. gen. P. y = sys last_size. y - a. y - 1;
		}
		break;
	case WM_DRAG_RESPONSE:
		SetCursor( sys pointer );
		break;
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*) mp2;
			if ( dis-> CtlType == ODT_MENU && dis-> itemData != 0) {
				RECT r;
				self = (Handle) dis-> itemData;
				v = (PWidget) self;
				GetClientRect(WindowFromDC(dis->hDC), &r);
				ev.cmd          = cmMenuItemPaint;
				ev.gen.i        = (Handle) dis-> itemID - MENU_ID_AUTOSTART;
				ev.gen.p        = (void*) dis->hDC;
				ev.gen.B        = ((dis-> itemState & ODS_SELECTED) != 0);
				ev.gen.P.x      = r.right;
				ev.gen.P.y      = r.bottom;
				ev.gen.R.left   = dis-> rcItem.left;
				ev.gen.R.bottom = r.bottom - dis-> rcItem.bottom;
				ev.gen.R.right  = dis-> rcItem.right - 1;
				ev.gen.R.top    = r.bottom - dis-> rcItem.top - 1;
			}
		}
		break;
	case WM_ENABLE:
		ev. cmd = mp1 ? cmEnable : cmDisable;
		hiStage = true;
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_FORCEFOCUS:
		if ( mp2)
			((( PWidget) mp2)-> self)-> set_selected(( Handle) mp2, 1);
		return 0;
	case WM_HASMATE:
		*(( Handle*) mp2) = self;
		return HASMATE_MAGIC;
	case WM_IME_CHAR:
		if ( apc_widget_is_responsive( self)) {
			ev. cmd = cmKeyDown;
			ev. key. mod  = kmUnicode;
			ev. key. key  = kbNoKey;
			ev. key. code = mp1;
			ev. key. repeat = 1;
		}
		break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if ( mp2 & ( 1 << 29)) ev. key. mod = kmAlt;
	case WM_KEYDOWN:
	case WM_KEYUP:
		if ( apc_widget_is_responsive( self)) {
			BYTE * key_state;
			Bool up = ( msg == WM_KEYUP) || ( msg == WM_SYSKEYUP);
			Bool extended = mp2 & ( 1 << 24);
			UINT scan = ( HIWORD( mp2) & 0xFF) | ( up ? 0x80000000 : 0);
			int deadPollCount = 0;
			HKL kl = GetKeyboardLayout(0);

			// basic assignments
			ev. cmd = up ? cmKeyUp : cmKeyDown;
			ev. key. key    = ctx_remap_def( mp1, ctx_kb2VK, false, kbNoKey);
			ev. key. code   = mp1;
			ev. key. repeat = mp2 & 0x000000FF;
			if (ev. key. repeat == 0) ev. key. repeat = 1;

			// VK validations
			if ( extended) {
				int ks = ev. key. key;
				ev. key. key = ctx_remap_def( ks, ctx_kb2VK3, true, ks);
				if ( ev. key. key != ks)
					extended = false; // avoid (Ctrl|Alt)R+KeyPad combinations
			} else if ( mp1 >= VK_NUMPAD0 && mp1 <= VK_DIVIDE)
				extended = true; // include numpads

			ev. key. mod   = 0 |
				( extended ? kmKeyPad : 0) |
				(( GetKeyState( VK_SHIFT)   < 0) ? kmShift : 0) |
				(( GetKeyState( VK_CONTROL) < 0) ? kmCtrl  : 0) |
				(( GetKeyState( VK_MENU)    < 0) ? kmAlt   : 0);

			key_state = guts. key_state;
AGAIN:
			if ( P_APPLICATION-> wantUnicodeInput) {
				WCHAR keys[ 2];
				// unicode mapping
				switch ( ToUnicodeEx( mp1, scan, key_state, keys, 2, 0, kl)) {
				case 1: // char
					if ( last_dead_key ) {
						WCHAR wcBuffer[3];
						WCHAR out[3];
						wcBuffer[0] = keys[0];
						wcBuffer[1] = last_dead_key;
						wcBuffer[2] = '\0';
						if ( FoldStringW(MAP_PRECOMPOSED, (LPWSTR) wcBuffer, 3, (LPWSTR) out, 3) )
							keys[0] = out[0];
					}
					if ( !deadPollCount && ( GetKeyState( VK_MENU) < 0) && ( GetKeyState( VK_SHIFT) >= 0)) {
						WCHAR keys2[2];
						if (( ToUnicodeEx( mp1, scan, guts. empty_key_state, keys2, 2, 0, kl) == 1) &&
							( keys2[0] != keys[0])) {
							/* example - (AltGr+2) == '@' on danish keyboard.
								this hack is to tell whether the key without mods
								will give same character code ...  */
							ev. key. mod &= ~(kmAlt|kmCtrl|kmShift);
						}
					}
					if (!up) last_dead_key = 0;
					break;
				case 2: { // dead key
						last_dead_key = ctx_remap_def( keys[0], ctx_deadkeys, true, keys[0]);
						keys[ 0] = 0;
						   ev. key. mod |= kmDeadKey;
					}
					break;
				case 0: // virtual key
					if ( deadPollCount == 0) {
					/* can't have character code - maybe fish out without mods? */
						key_state = guts. empty_key_state;
						deadPollCount = 1;
						goto AGAIN;
					} else {
					/* same meaning without mods, no code anyway */
						keys[ 0] = 0;
					}
					if (!up) last_dead_key = 0;
					break;
				default:
					ev. key. mod |= kmDeadKey;
					if (!up) last_dead_key = 0;
				}
				ev. key. code = keys[ 0];
				ev. key. mod |= kmUnicode;
			} else {
				BYTE keys[ 4];
				switch ( ToAsciiEx( mp1, scan, key_state, (LPWORD) keys, 0, kl)) {
				case 1: // char
					if ( last_dead_key ) {
						BYTE cBuffer[3];
						BYTE out[3];
						cBuffer[0] = keys[0];
						cBuffer[1] = last_dead_key;
						cBuffer[2] = '\0';
						if ( FoldStringA(MAP_PRECOMPOSED, (LPSTR) cBuffer, 3, (LPSTR) out, 3) )
		   					keys[0] = out[0];
					}
					if ( !deadPollCount && ( GetKeyState( VK_MENU) < 0) && ( GetKeyState( VK_SHIFT) >= 0)) {
						BYTE keys2[4];
						if (( ToAsciiEx( mp1, scan, guts. empty_key_state, (LPWORD) keys2, 0, kl) == 1) &&
							( keys2[0] != keys[0])) {
							/* example - (AltGr+2) == '@' on danish keyboard.
								this hack is to tell whether the key without mods
								will give same character code ...  */
							ev. key. mod &= ~(kmAlt|kmCtrl|kmShift);
						}
					}
					break;
				case 2: // dead key 
					last_dead_key = keys[0];
					keys[ 0] = 0;
						ev. key. mod |= kmDeadKey;
					break;
				case 0: // virtual key
					if ( deadPollCount == 0) {
					/* can't have character code - maybe fish out without mods? */
						key_state = guts. empty_key_state;
						deadPollCount = 1;
						goto AGAIN;
					} else {
					/* same meaning without mods, no code anyway */
						keys[ 0] = 0;
					}
					if (!up) last_dead_key = 0;
					break;
				default:
					ev. key. mod |= kmDeadKey;
					if (!up) last_dead_key = 0;
				}
				ev. key. code = keys[ 0];
			}

			// simulated key codes
			if ( ev. key. key == kbTab && ( ev. key. mod & kmShift))
				ev. key. key = kbBackTab;

			if ( ev. key. code >= 'A' && ev. key. code <= 'z' && ev. key. mod & kmCtrl) {
				ev. key. code = toupper(ev. key. code & 0xFF) - '@';
				if (!( ev. key. mod & kmShift)) ev. key. code = tolower( ev. key. code);
			}
		}
		break;
	case WM_INITMENUPOPUP:
		if ( HIWORD( mp2)) break; // do not use system popup
	case WM_INITMENU:
		{
			PMenuWndData mwd = ( PMenuWndData) hash_fetch( mgr_menu, &mp1, sizeof( void*));
			PMenuItemReg m = NULL;
			sys last_menu = mwd ? mwd-> menu : NULL_HANDLE;
			if ( mwd && mwd-> menu && ( PAbstractMenu(mwd-> menu)->stage <= csNormal)) {
				m = ( PMenuItemReg) AbstractMenu_first_that( mwd-> menu, find_oid, INT2PTR(void*,mwd->id), true);
				hiStage    = true;
				ev. cmd    = cmMenu;
				ev. gen. H = mwd-> menu;
				ev. gen. i = m ? m-> id : 0;
			}
			if (( msg == WM_INITMENUPOPUP) && ( m == NULL))
				ev. cmd = 0;
		}
		break;
	case WM_KILLFOCUS:
		if (( HWND) mp1 != win) {
			ev. cmd = cmReleaseFocus;
			hiStage = true;
			apt_assign( aptFocused, 0);
			DestroyCaret();
		}
		break;
	case WM_LBUTTONDOWN:
		ev. pos. button = mbLeft;
		goto MB_DOWN;
	case WM_RBUTTONDOWN:
		ev. pos. button = mbRight;
		goto MB_DOWN;
	case WM_MBUTTONDOWN:
		ev. pos. button = mbMiddle;
		goto MB_DOWN;
	case WM_XBUTTONDOWN:
		ev. pos. button = (HIWORD(mp1) == XBUTTON1) ? mb4 : mb5;
		goto MB_DOWN;
	case WM_LBUTTONUP:
		ev. pos. button = mbLeft;
		goto MB_UP;
	case WM_RBUTTONUP:
		ev. pos. button = mbRight;
		goto MB_UP;
	case WM_MBUTTONUP:
		ev. pos. button = mbMiddle;
		goto MB_UP;
	case WM_XBUTTONUP:
		ev. pos. button = (HIWORD(mp1) == XBUTTON1) ? mb4 : mb5;
		goto MB_UP;
	case WM_LBUTTONDBLCLK:
		ev. pos. button = mbLeft;
		goto MB_DBLCLK;
	case WM_RBUTTONDBLCLK:
		ev. pos. button = mbRight;
		goto MB_DBLCLK;
	case WM_MBUTTONDBLCLK:
		ev. pos. button = mbMiddle;
		goto MB_DBLCLK;
	case WM_XBUTTONDBLCLK:
		ev. pos. button = (HIWORD(mp1) == XBUTTON1) ? mb4 : mb5;
		goto MB_DBLCLK;
	case WM_LMOUSECLICK:
		ev. pos. button = mbLeft;
		goto MB_CLICK;
	case WM_RMOUSECLICK:
		ev. pos. button = mbRight;
		goto MB_CLICK;
	case WM_MMOUSECLICK:
		ev. pos. button = mbMiddle;
		goto MB_CLICK;
	case WM_XMOUSECLICK:
		ev. pos. button = (HIWORD(mp1) == XBUTTON1) ? mb4 : mb5;
		goto MB_CLICK;
	case WM_MOUSEWHEEL:
		{
			POINT p;
			p. x = (short)LOWORD( mp2);
			p. y = (short)HIWORD( mp2);
			ev. cmd         = cmMouseWheel;
			ev. pos. button = ( short) HIWORD( mp1);
			MapWindowPoints( NULL, win, &p, 1);
			ev. pos. where. x = p. x;
			ev. pos. where. y = sys last_size. y - p. y - 1;
		}
		goto MB_MAIN_NOPOS;
	case WM_MOUSEMOVE:
		ev. cmd = cmMouseMove;
		if ( self != last_mouse_over) {
			Handle old = last_mouse_over;
			last_mouse_over = self;
			if ( old && ( PWidget( old)-> stage == csNormal))
				SendMessage(( HWND)(( PWidget) old)-> handle, WM_MOUSEEXIT, mp1, mp2);
			SendMessage( win, WM_MOUSEENTER, mp1, mp2);
			if ( !guts. mouse_timer) {
				guts. mouse_timer = 1;
				if ( !SetTimer( dsys(prima_guts.application)handle, TID_USERMAX, 100, NULL)) apiErr;
			}
		}
		goto MB_MAIN;
	case WM_MOUSEENTER:
		ev. cmd = cmMouseEnter;
		goto MB_MAIN;
	case WM_MOUSEEXIT:
		ev. cmd = cmMouseLeave;
		goto MB_MAIN;
	MB_DOWN:
		ev. cmd = cmMouseDown;
		goto MB_MAINACT;
	MB_UP:
		ev. cmd = cmMouseUp;
		goto MB_MAINACT;
	MB_DBLCLK:
		ev. pos. nth = 2;
	MB_CLICK:
		ev. cmd = cmMouseClick;
		/* is this a 3rd etc click? */
		{
			unsigned long ts     = get_current_timestamp();
			Bool is_continuation =
				(ts - guts.last_mouse_click_ts < guts.mouse_double_click_delay) &&
				(guts.last_mouse_click_fingerprint == (ev.pos.button | get_mouse_fingerprint( mp1 ) | apc_pointer_get_state(self))) &&
				(mp2 == guts.last_mouse_click_position) &&
				(win == guts.last_mouse_click_source)
				;
			if ( is_continuation ) {
				ev. pos. nth = ++guts.last_mouse_click_number;
			} else {
				guts.last_mouse_click_number = ( ev. pos. nth == 0 ) ? 1 : 2;
				guts.last_mouse_click_fingerprint = ev.pos.button | get_mouse_fingerprint( mp1 ) | apc_pointer_get_state(self);
				guts.last_mouse_click_source = win;
				guts.last_mouse_click_position = mp2;
			}
			guts.last_mouse_click_ts = ts;
		}

		goto MB_MAINACT;
	MB_MAINACT:
		if ( !is_apt( aptEnabled) || !apc_widget_is_responsive( self))
		{
			if ( ev. cmd == cmMouseDown || (ev. cmd == cmMouseClick && ev. pos. nth > 1))
				MessageBeep( MB_OK);
			return 0;
		}
		goto MB_MAIN;
	MB_MAIN:
		if ( ev. cmd == cmMouseDown && !is_apt( aptFirstClick)) {
			Handle x = self;
			while ( dsys(x) class_name != WC_FRAME && ( x != prima_guts.application)) x = (( PWidget) x)-> owner;
			if ( x != prima_guts.application && !local_wnd( GetActiveWindow(), DHANDLE( x)))
			{
				ev. cmd = 0; // yes, we abandon mousedown but we should force selection:
				if (P_APPLICATION-> hintUnder == self) v-> self-> set_hintVisible( self, 0);
				if (( v-> options. optSelectable) && ( v-> selectingButtons & ev. pos. button))
					apc_widget_set_focused( self);
			}
		}
		ev. pos. where. x = (short)LOWORD( mp2);
		ev. pos. where. y = sys last_size. y - (short)HIWORD( mp2) - 1;
	MB_MAIN_NOPOS:
		ev. pos. mod  = get_mouse_fingerprint( mp1 ) | apc_pointer_get_state(self);
		break;
	case WM_MEASUREITEM: {
		MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT*) mp2;
		if ( mis-> CtlType == ODT_MENU && mis-> itemData != 0) {
			ev.cmd     = cmMenuItemMeasure;
			self = (Handle) mis-> itemData;
			v = (PWidget) self;
			ev.gen.i   = (Handle) mis-> itemID - MENU_ID_AUTOSTART;
			ev.gen.P.x = ev.gen.P.y = 0;
		}
		break;
	}
	case WM_MENUCHAR: {
		int key;
		PMenuWndData mwd;
		ev. key. key    = ctx_remap_def( mp1, ctx_kb2VK2, false, kbNoKey);
		ev. key. code   = LOWORD(mp1);
		ev. key. mod   |=
			(( GetKeyState( VK_SHIFT)   < 0) ? kmShift : 0) |
			(( GetKeyState( VK_CONTROL) < 0) ? kmCtrl  : 0) |
			(( GetKeyState( VK_MENU)    < 0) ? kmAlt   : 0);
		if (( ev. key. mod & kmCtrl) && ( ev. key. code <= 'z'))
			ev. key. code += 'A' - 1;
		key = CAbstractMenu-> translate_key( NULL_HANDLE, ev. key. code, ev. key. key, ev. key. mod);
		if ( v-> self-> process_accel( self, key))
			return MAKELONG( 0, MNC_CLOSE);

		ev.key.code = tolower(ev.key.code);
		if (( mwd = (MenuWndData*) hash_fetch(mgr_menu, &mp2, sizeof(mp2))) != NULL) {
			int pos = 0;
			PMenuItemReg m = CAbstractMenu(mwd->menu)-> first_that(mwd->menu, (void*)id_match, &mwd->id, false);
			while ( m != NULL ) {
				if ( m-> flags.custom_draw && m-> text != NULL ) {
					char * t = m-> text;
					while (*t) {
						if ( t[0] == '~' && tolower(t[1]) == ev.key.code )
							return MAKELONG( pos, MNC_EXECUTE);
						t++;
					}
				}
				m = m-> next;
				pos++;
			}
		}
		break;
	}
	case WM_SYNCMOVE: {
		Handle parent = v-> self-> get_parent(( Handle) v);
		if ( parent) {
			Point pos  = var self-> get_origin( self);
			ev. cmd    = cmMove;
			ev. gen. P = pos;
			if ( pos. x == var pos. x && pos. y == var pos. y) ev. cmd = 0;
		}
		break;
	}
	case WM_MOVE: {
		Handle parent = v-> self-> get_parent(( Handle) v);
		if ( parent) {
			Point sz = CWidget(parent)-> get_size( parent);
			ev. cmd = cmMove;
			ev. gen . P. x = ( short) LOWORD( mp2);
			ev. gen . P. y = sz. y - ( short) HIWORD( mp2) - sys y_override;
			if ( is_apt( aptTransparent))
				InvalidateRect( win, NULL, false);
		}
		break;
	}
	case WM_NCHITTEST:
		if ( guts. sys_focus_dialog) return HTERROR;
		// dlg protect code - protecting from user actions
		if ( !guts. sys_focus_disabled && ( Application_map_focus( prima_guts.application, self) != self))
			return HTERROR;
		break;
	case WM_PAINT:
		ev. cmd = cmPaint;
		if (
			( sys class_name == WC_CUSTOM) &&
			( var stage == csNormal) &&
			( list_index_of( &guts. transp, self) >= 0)
			)
			return 0;

		if (
			( var self->get_locked(self) > 0) || /* or WM_PAINT bashing occurs */
			is_apt( aptLayered ) ||
			( opt_InPaint && !is_apt(aptWM_PAINT) )
		) {
			PAINTSTRUCT ps;
			BeginPaint(win, &ps);
			EndPaint(win, &ps);
			return 0;
		}

		break;
	case WM_QUERYNEWPALETTE:
		return palette_change( self);
	case WM_PALETTECHANGED:
		if (( HWND) mp1 != win) {
			Handle mp = hwnd_to_view(( HWND) mp1);
			if ( mp && ( hwnd_top_level( mp) == hwnd_top_level( self)))
				return 0;
			palette_change( self);
		}
		break;
	case WM_POSTAL:
		ev. cmd    = cmPost;
		ev. gen. H = ( Handle) mp1;
		ev. gen. p = ( void *) mp2;
		break;
	case WM_PRIMA_CREATE:
		ev. cmd = cmSetup;
		break;
	case WM_REPAINT_LAYERED:
		if ( var stage == csNormal )
			hwnd_repaint_layered( self, true );
		break;
	case WM_SETFOCUS:
		if ( guts. sys_focus_dialog) return 1;
		// dlg protect code - general case
		if ( !guts. sys_focus_disabled && !guts. sys_focus_granted) {
			Handle hf = Application_map_focus( prima_guts.application, self);
			if ( hf != self) {
				PostMessage( win, WM_FORCEFOCUS, 0, ( LPARAM) hf);
				return 1;
			}
		}
		if (( HWND) mp1 != win) {
			ev. cmd = cmReceiveFocus;
			hiStage = true;
			apt_assign( aptFocused, 1);
			cursor_update( self);
		}
		break;
	case WM_SETVISIBLE:
	if ( list_index_of( &guts. transp, self) < 0) {
		if ( v-> stage <= csNormal) ev. cmd = mp1 ? cmShow : cmHide;
		hiStage = true;
		apt_assign( aptVisible, mp1);
	}
	break;
	case WM_SIZE:
		ev. cmd = cmSize;
		ev. gen. R. left   = sys last_size. x;
		ev. gen. R. bottom = sys last_size. y;
		sys last_size. x    = ev. gen. R. right  = ev. gen . P. x = ( short) LOWORD( mp2);
		sys last_size. y    = ev. gen. R. top    = ev. gen . P. y = ( short) HIWORD( mp2);
		if ( ev. gen. R. top != ev. gen. R. bottom) {
			int delta = ev. gen. R. top - ev. gen. R. bottom;
			Widget_first_that( self, move_back, &delta);
			if ( is_apt( aptFocused)) cursor_update(( Handle) self);
		}
		if ( sys size_lock_level == 0 && var stage <= csNormal)
			var virtualSize = sys last_size;
		break;
	case WM_WINDOWPOSCHANGING: {
		LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
		if ( sys class_name == WC_CUSTOM) {
			if (( l-> flags & SWP_NOSIZE) == 0) {
				ev. cmd = cmCalcBounds;
				ev. gen. R. right = l-> cx;
				ev. gen. R. top   = l-> cy;
			}
		}
		if (( l-> flags & SWP_NOZORDER) == 0)
			zorder_sync( self, win, l);
		break;
	}
	case WM_WINDOWPOSCHANGED: {
		LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
		if (( l-> flags & SWP_NOZORDER) == 0)
			PostMessage( win, WM_ZORDERSYNC, 0, 0);
		if (( l-> flags & SWP_NOSIZE) == 0) {
			sys y_override = l-> cy;
			SendMessage( win, WM_SYNCMOVE, 0, 0);
		}
		if ( l-> flags & SWP_HIDEWINDOW) SendMessage( win, WM_SETVISIBLE, 0, 0);
		if ( l-> flags & SWP_SHOWWINDOW) SendMessage( win, WM_SETVISIBLE, 1, 0);
		break;
	}
	case WM_ZORDERSYNC:
		ev. cmd = cmZOrderChanged;
		break;
	}

	if ( hiStage)
		ret = DefWindowProcW( win, msg, mp1, mp2);

	if ( ev. cmd)
		message_result = v-> self-> message( self, &ev);
	else
		ev. cmd = orgMsg;

	if ( v-> stage > csNormal) orgMsg = 0; // protect us from dead body

	switch ( orgMsg) {
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*) mp2;
		if ( dis-> CtlType == ODT_MENU && dis-> itemData != 0)
			return (LRESULT) 1;
		break;
	}
	case WM_DESTROY:
		v-> handle = NULL_HANDLE;       // tell apc not to kill this HWND
		SetWindowLongPtr( win, GWLP_USERDATA, 0);
		Object_destroy(( Handle) v);
		break;
	case WM_PAINT:
		return 0;
	case WM_SYSKEYDOWN:
		if ( !message_result)
			guts. dont_xlate_message = true;
		break;
	case WM_SYSKEYUP:
		// ev. cmd = 1; // forced call DefWindowProc superseded for test reasons
		break;
	case WM_MEASUREITEM: {
		MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT*) mp2;
		if ( mis-> CtlType == ODT_MENU && mis-> itemData != 0) {
			mis-> itemWidth  = ev.gen.P.x;
			mis-> itemHeight = ev.gen.P.y;
			return (LRESULT) 1;
		}
		break;
	}
	case WM_MOUSEMOVE:
		SetCursor( is_apt( aptEnabled) ? sys pointer : std_arrow_cursor);
		break;
	case WM_MOUSEWHEEL:
		return ( LRESULT)1;
	case WM_WINDOWPOSCHANGING: {
		LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
		if ( sys class_name == WC_CUSTOM) {
			if (( l-> flags & SWP_NOSIZE) == 0) {
				int dy = l-> cy - ev. gen. R. top;
				l-> cx = ev. gen. R. right;
				l-> cy = ev. gen. R. top;
				l-> y += dy;
			}
			return false;
		}
		if (( l-> flags & SWP_NOZORDER) == 0)
			zorder_sync( self, win, l);
		break;
	}
	}

	if ( ev. cmd && !hiStage)
		ret = DefWindowProcW( win, msg, mp1, mp2);

	return ret;
}

LRESULT CALLBACK generic_frame_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
	LRESULT ret = 0;
	Handle  self   = GetWindowLongPtr( win, GWLP_USERDATA);
	PWidget   v    = ( PWidget) self;
	UINT    orgMsg = msg;
	Event   ev;
	Bool    hiStage   = false;

	if ( !self)
		return DefWindowProcW( win, msg, mp1, mp2);

	memset( &ev, 0, sizeof (ev));
	ev. gen. source = self;

	switch ( msg) {
	case WM_ACTIVATE:
		if ( guts. sys_focus_dialog) return 1;
		// dlg protect code - protecting from window activation
		if ( LOWORD( mp1) && !guts. sys_focus_disabled) {
			Handle hf = Application_map_focus( prima_guts.application, self);
			if ( hf != self) {
				guts. sys_focus_disabled = 1;
				Application_popup_modal( prima_guts.application);
				PostMessage( win, msg, 0, 0);
				guts. sys_focus_disabled = 0;
				return 1;
			}
		}
		ev. cmd = ( LOWORD( mp1) != WA_INACTIVE) ? cmActivate : cmDeactivate;
		hiStage = true;
		break;
	case WM_CLOSE:
		if ( guts. sys_focus_dialog) return 0;
		ev. cmd = cmClose;
		break;
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_INITMENUPOPUP:
	case WM_INITMENU:
	case WM_MEASUREITEM:
	case WM_MENUCHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SETVISIBLE:
	case WM_ENABLE:
	case WM_FORCEFOCUS:
	case WM_MOUSEWHEEL:
	case WM_ZORDERSYNC:
		return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
	case WM_QUERYNEWPALETTE:
		return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
	case WM_PALETTECHANGED:
		if (( HWND) mp1 == win) return 0;
		return generic_view_handler(( HWND) v-> handle, msg, mp1, mp2);
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if ( generic_view_handler(( HWND) v-> handle, msg, mp1, mp2) == 0)
			return 0;
		hiStage = true;
		break;
	case WM_DLGENTERMODAL:
		ev. cmd = mp1 ? cmExecute : cmEndModal;
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_NCACTIVATE:
		if ( guts. sys_focus_dialog) return 1;

		if (( mp1 == 0) && ( mp2 != 0)) {
			Handle x = hwnd_to_view(( HWND) mp2);
			if ( is_declipped_child( x) && Widget_is_child( x, self)) {
				return 1;
			}
		}
		// dlg protect code - protecting from window activation
		if ( mp1 && !guts. sys_focus_disabled) {
			Handle hf = Application_map_focus( prima_guts.application, self);
			if ( hf != self) {
				guts. sys_focus_disabled = 1;
				Application_popup_modal( prima_guts.application);
				PostMessage( win, msg, 0, 0);
				guts. sys_focus_disabled = 0;
				return 1;
			}
		}
		break;
	case WM_NCHITTEST:
		if ( guts. sys_focus_dialog) return HTERROR;
		// dlg protect code - protecting from user actions
		if ( !guts. sys_focus_disabled) {
			Handle foc = Application_map_focus( prima_guts.application, self);
			if ( foc != self) {
				return ( foc == apc_window_get_active()) ? HTERROR : HTCLIENT;
			}
		}
		break;
	case WM_SETFOCUS:
		if ( guts. sys_focus_dialog) return 1;

		// dlg protect code - general case
		if ( !guts. sys_focus_disabled && !guts. sys_focus_granted) {
			Handle hf = Application_map_focus( prima_guts.application, self);
			if ( hf != self) {
				PostMessage( win, WM_FORCEFOCUS, 0, ( LPARAM) hf);
				return 1;
			}
		}

		// This code is about to protect frame events when set_selected would
		// grant SetFocus() call to another frame.
		{
			Handle x  = var self-> get_selectee( self);
			Handle w  = x;
			Bool hasCO = w == self;
			while ( w && w != self) {
				if ( !dsys( w) options. aptClipOwner) {
					hasCO = true;
					break;
				}
				w = (( PWidget) w)-> owner;
			}
			if ( !hasCO) {
				var self-> set_selected( self, true);
			}
			// else we do not select any widget, but still have a chance to resize frame :)
		}
		break;
	case WM_SIZE:
		if ( !is_apt( aptIgnoreSizeMessages)) {
			int state = wsNormal;
			Bool doWSChange = false;
			if (( int) mp1 == SIZE_RESTORED) {
				state = wsNormal;
				if ( sys s. window. state != state)
					doWSChange = true;
			} else if (( int) mp1 == SIZE_MAXIMIZED) {
				state = wsMaximized;
				doWSChange = true;
			} else if (( int) mp1 == SIZE_MINIMIZED) {
				state = wsMinimized;
				doWSChange = true;
			}
			if ( doWSChange) {
				ev.gen.i = sys s.window.state = state;
				ev.cmd = cmWindowState;
			}
		}
		break;
	case WM_SYNCMOVE: {
		Handle parent = v-> self-> get_parent(( Handle) v);
		if ( parent) {
			Point pos  = var self-> get_origin( self);
			ev. cmd    = cmMove;
			ev. gen. P = pos;
			if ( pos. x == var pos. x && pos. y == var pos. y) ev. cmd = 0;
		}
		break;
	}
	case WM_MOVE: {
		Handle parent = v-> self-> get_parent(( Handle) v);
		if ( parent) {
			Point sz = CWidget(parent)-> get_size( parent);
			ev. cmd = cmMove;
			ev. gen . P. x = ( short) LOWORD( mp2);
			ev. gen . P. y = sz. y - ( short) HIWORD( mp2) - sys y_override;
		}
		break;
	}
// case WM_SYSCHAR:return 1;
	case WM_TIMER:
		if ( mp1 == TID_USERMAX) {
			POINT p;
			HWND wp;
			if ( last_mouse_over && !GetCapture() && ( PObject( last_mouse_over)-> stage == csNormal)) {
				HWND desktop = HWND_DESKTOP;
				GetCursorPos( &p);
				wp = WindowFromPoint( p);
				if ( wp) {
					POINT xp = p;
					MapWindowPoints( desktop, wp, &xp, 1);
					wp = ChildWindowFromPointEx( wp, xp, CWP_SKIPINVISIBLE);
				} else
					wp = ChildWindowFromPointEx( wp, p, CWP_SKIPINVISIBLE);
				if ( wp != ( HWND)(( PWidget) last_mouse_over)-> handle)
				{
					HWND old = ( HWND)(( PWidget) last_mouse_over)-> handle;
					Handle s;
					last_mouse_over = NULL_HANDLE;
					SendMessage( old, WM_MOUSEEXIT, 0, 0);
					s = hwnd_to_view( wp);
					if ( s && ( HWND)(( PWidget) s)-> handle == wp)
					{
						MapWindowPoints( desktop, wp, &p, 1);
						SendMessage( wp, WM_MOUSEENTER, 0, MAKELPARAM( p. x, p. y));
						last_mouse_over = s;
					} else if ( guts. mouse_timer) {
						guts. mouse_timer = 0;
						if ( !KillTimer( dsys(prima_guts.application)handle, TID_USERMAX)) apiErr;
					}
				}
			}
			return 0;
		} else {
			int id = mp1 - 1;
			if ( id >= 0 && id < time_defs_count) {
				ev. gen. H = ( Handle) time_defs[ id]. item;
				if ( ev. gen. H) {
					v = ( PWidget)( self = ev. gen. H);
					ev. cmd = cmTimer;
				}
			}
		}
		break;
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO l = ( LPMINMAXINFO) mp2;
		Point min = var self-> get_sizeMin( self);
		Point max = var self-> get_sizeMax( self);
		Point bor = get_window_borders( sys s. window. border_style);
		int   dy  = 0 +
			(( sys s. window. border_icons & biTitleBar) ? GetSystemMetrics( SM_CYCAPTION) : 0) +
			( PWindow(self)-> menu ? GetSystemMetrics( SM_CYMENU) : 0);
		l-> ptMinTrackSize. x = min. x + bor.x * 2;
		l-> ptMinTrackSize. y = min. y + bor.y * 2 + dy;
		l-> ptMaxTrackSize. x = max. x + bor.x * 2;
		l-> ptMaxTrackSize. y = max. y + bor.y * 2 + dy;
		break;
	}
	case WM_WINDOWPOSCHANGED:
		if ( !is_apt(aptIgnoreSizeMessages)) {
			LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
			if (( l-> flags & SWP_NOZORDER) == 0)
				PostMessage( win, WM_ZORDERSYNC, 0, 0);
			if (( l-> flags & SWP_NOSIZE) == 0) {
				RECT r;
				GetClientRect( win, &r);
				sys y_override = r. bottom - r. top;
				SendMessage( win, WM_SYNCMOVE, 0, 0);
			}
			if ( l-> flags & SWP_HIDEWINDOW) SendMessage( win, WM_SETVISIBLE, 0, 0);
			if ( l-> flags & SWP_SHOWWINDOW) SendMessage( win, WM_SETVISIBLE, 1, 0);
			{
				RECT r;
				GetClientRect( win, &r);
				SetWindowPos(( HWND) var handle, 0, 0, 0, r. right, r.bottom, SWP_NOZORDER);
			}
		}
		break;
	}

	if ( hiStage)
		ret = DefWindowProcW( win, msg, mp1, mp2);

	if ( ev. cmd) v-> self-> message( self, &ev); else ev. cmd = orgMsg;

	if ( var stage == csDead) orgMsg = 0;

	switch ( orgMsg) {
	case WM_CLOSE:
		if ( ev. cmd) {
			if ( sys class_name == WC_FRAME && PWindow(self)->modal) {
				CWindow( self)-> cancel( self);
				return 0;
			} else {
				SetWindowLongPtr( win, GWLP_USERDATA, 0);
				Object_destroy(( Handle) v);
			}
			break;
		} else
			return 0;
	}

	if ( ev. cmd && !hiStage)
		ret = DefWindowProcW( win, msg, mp1, mp2);
	return ret;
}

static void
update_layered_frame(Handle self)
{
	HRGN r1, r2;
	RECT frame, client;
	POINT frame_size, client_size;
	Point delta_upper_left, delta_lower_right, move;
	HWND win = HANDLE;

	delta_lower_right = get_window_borders( sys s. window. border_style);
	GetWindowRect(win, &frame);
	GetClientRect(win, &client);
	frame_size. x = frame. right  - frame. left;
	frame_size. y = frame. bottom - frame. top;
	client_size. x = client. right  - client. left;
	client_size. y = client. bottom - client. top;
	r2 = CreateRectRgn( 0, 0, frame_size.x, frame_size.y);
	r1 = CreateRectRgn( 0, 0, client_size.x, client_size.y);
	delta_upper_left.x = frame_size.x - client_size.x - delta_lower_right.x;
	delta_upper_left.y = frame_size.y - client_size.y - delta_lower_right.y;
	OffsetRgn( r1, delta_upper_left.x, delta_upper_left.y);
	CombineRgn( r2, r2, r1, RGN_XOR);
	if (!SetWindowRgn( win, r2, true)) apiErr;
	DeleteObject(r1);
	DeleteObject(r2);

	move.x = frame.left + delta_upper_left.x;
	move.y = frame.top  + delta_upper_left.y;
	if ( !SetWindowPos(( HWND ) var handle, win,
		client.left + move.x, client.top + move.y, client_size.x, client_size.y,
		SWP_NOACTIVATE)) apiErr;
	hwnd_repaint_layered( self, false );
}

LRESULT CALLBACK layered_frame_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
	Handle  self = GetWindowLongPtr( win, GWLP_USERDATA);

	if ( !self)
		return DefWindowProcW( win, msg, mp1, mp2);

	switch ( msg) {
	case WM_NCACTIVATE:
	case WM_NCHITTEST:
	case WM_SETFOCUS:
		return DefWindowProcW( win, msg, mp1, mp2);

	case WM_SIZE:
	case WM_MOVE:
		update_layered_frame(self);
		return DefWindowProcW( win, msg, mp1, mp2);

	case WM_WINDOWPOSCHANGED: {
		LPWINDOWPOS l = ( LPWINDOWPOS) mp2;
		Bool updated = false;

		if (( l-> flags & SWP_NOSIZE) == 0) {
			RECT r;
			update_layered_frame(self);
			updated = true;

			GetClientRect( win, &r);
			sys y_override = r. bottom - r. top;
			SendMessage( win, WM_SYNCMOVE, 0, 0);
		}
		if (( l-> flags & SWP_NOMOVE) == 0) {
			if ( !updated ) {
				update_layered_frame(self);
				updated = true;
			}
		}
		if (( l-> flags & SWP_NOZORDER) == 0) {
			PostMessage( win, WM_ZORDERSYNC, 0, 0);
			if ( !updated ) {
				update_layered_frame(self);
				updated = true;
			}
		}
		if ( l-> flags & SWP_HIDEWINDOW) {
			ShowWindow((HWND) var handle, SW_HIDE);
			SendMessage( win, WM_SETVISIBLE, 0, 0);
		}
		if ( l-> flags & SWP_SHOWWINDOW) {
			ShowWindow((HWND) var handle, SW_SHOW);
			SendMessage( win, WM_SETVISIBLE, 1, 0);
		}
		return DefWindowProcW( win, msg, mp1, mp2);
	}

	}

	return generic_frame_handler( win, msg, mp1, mp2 );
}

static Bool kill_img_cache( Handle self, int keyLen, void * key, void * killDBM)
{
	if ( is_apt( aptDeviceBitmap)) {
		if ( killDBM) dbm_recreate( self);
	} else
		image_destroy_cache( self);
	return false;
}

LRESULT CALLBACK generic_app_handler( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2)
{
	switch ( msg) {
		case WM_DISPLAYCHANGE: {
			HDC dc = dc_alloc();
			int oldBPP = guts. display_bm_info. bmiHeader. biBitCount;
			HBITMAP hbm;

			if ( dc) {
				guts. display_bm_info. bmiHeader. biBitCount = 0;
				guts. display_bm_info. bmiHeader. biSize = sizeof( BITMAPINFO);
				if ( !( hbm = GetCurrentObject( dc, OBJ_BITMAP))) apiErr;

				if ( !GetDIBits( dc, hbm, 0, 0, NULL, &guts. display_bm_info, DIB_PAL_COLORS)) {
					guts. display_bm_info. bmiHeader. biBitCount = ( int) mp1;
					guts. display_bm_info. bmiHeader. biPlanes   = GetDeviceCaps( dc, PLANES);
				};
			}
			dsys( prima_guts.application) last_size. x = ( short) LOWORD( mp2);
			dsys( prima_guts.application) last_size. y = ( short) HIWORD( mp2);
			if ( dc) {
				if ( oldBPP != guts. display_bm_info. bmiHeader. biBitCount)
					hash_first_that( mgr_images, kill_img_cache, (void*)1, NULL, NULL);
				dc_free();
			}
			break;
		}
		case WM_FONTCHANGE:
			destroy_font_hash();
			break;
		case WM_DPICHANGED: {
			Event ev = {cmFontChanged};
			dpi_change();
			reset_system_fonts();
			destroy_font_hash();
			font_clean();
			PComponent(prima_guts.application)-> self-> message( prima_guts.application, &ev);
			break;
		}
		case WM_COMPACTING:
			stylus_clean();
			font_clean();
			destroy_font_hash();
			hash_first_that( mgr_images, kill_img_cache, NULL, NULL, NULL);
			hash_destroy( mgr_registry, false);
			mgr_registry = hash_create();
			break;
		case WM_QUERYNEWPALETTE:
		case WM_PALETTECHANGED:
			return 0;
	}
	return generic_frame_handler( win, msg, mp1, mp2);
}

#ifdef __cplusplus
}
#endif
