/***********************************************************/
/*                                                         */
/*  System dependent application management (unix, x11)    */
/*                                                         */
/***********************************************************/

#include "apricot.h"
#include "unix/guts.h"
#include "Application.h"
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#if !defined(BYTEORDER)
#error "BYTEORDER is not defined"
#endif
#define LSB32   0x1234
#define LSB64   0x12345678
#define MSB32   0x4321
#define MSB64   0x87654321
#ifndef BUFSIZ
#define BUFSIZ  2048
#endif

UnixGuts guts, *pguts = &guts;
static XErrorEvent *save_xerror_event = NULL;

UnixGuts *
prima_unix_guts(void)
{
	return &guts;
}

static int
x_error_handler( Display *d, XErrorEvent *ev)
{
	int tail = guts. ri_tail;
	int prev = tail;
	char *name = "Prima";
	char buf[BUFSIZ];
	char mesg[BUFSIZ];
	char number[32];

	if ( save_xerror_event ) {
		*save_xerror_event = *ev;
		save_xerror_event = NULL;
		return 0;
	}

	while ( tail != guts. ri_head) {
		if ( guts. ri[ tail]. request > ev-> serial)
			break;
		prev = tail;
		tail++;
		if ( tail >= REQUEST_RING_SIZE)
			tail = 0;
	}

	switch ( ev-> request_code) {
	case 38: /* X_QueryPointer - apc_event uses sequence of XQueryPointer calls,
					to find where the pointer belongs. The error is raised when one
					of the windows disappears . */
	case 42: /* X_SetInputFocus */
		return 0;
	}

#ifdef NEED_X11_EXTENSIONS_XRENDER_H
	if ( ev-> request_code == guts. xft_xrender_major_opcode &&
		ev-> request_code > 127 &&
		ev-> error_code == BadLength)
		/* Xrender large polygon request failed */
		guts. xft_disable_large_fonts = 1;
#endif

#ifdef HAVE_X11_EXTENSIONS_XCOMPOSITE_H
	if ( ev-> request_code == guts. composite_opcode &&
		ev->minor_code == X_CompositeRedirectSubwindows) {
		/* checking for composite manager */
		guts. composite_error_triggered = true;
		return 0;
	}
#endif

	XGetErrorText( d, ev-> error_code, buf, BUFSIZ);
	XGetErrorDatabaseText( d, name, "XError", "X Error", mesg, BUFSIZ);
	fprintf( stderr, "%s: %s, request: %d", mesg, buf, ev->request_code);
	if ( ev->request_code < 128) {
		sprintf( number, "%d", ev->request_code);
		XGetErrorDatabaseText( d, "XRequest", number, "", buf, BUFSIZ);
		fprintf( stderr, "(%s)", buf);
	}
	if ( tail == guts. ri_head && prev == guts. ri_head);
	else if ( tail == guts. ri_head)
		fprintf( stderr, ", after %s:%d\n",
					guts. ri[ prev]. file, guts. ri[ prev]. line);
	else
		fprintf( stderr, ", between %s:%d and %s:%d\n",
					guts. ri[ prev]. file, guts. ri[ prev]. line,
					guts. ri[ tail]. file, guts. ri[ tail]. line);
	return 0;
}

void
prima_save_xerror_event( XErrorEvent *xr)
{
	bzero( xr, sizeof(XErrorEvent));
	save_xerror_event = xr;
}

void
prima_restore_xerror_event( XErrorEvent *xr)
{
	save_xerror_event = NULL;
	if ( xr && xr->display != NULL) x_error_handler( xr-> display, xr);
}


static int
x_io_error_handler( Display *d)
{
	fprintf( stderr, "Fatal input/output X error\n");
	_exit( 1);
	return 0; /* happy now? */
}

static XrmDatabase
get_database( void)
{
	XrmDatabase db = XrmGetStringDatabase( "");
	char filename[PATH_MAX];
	char *c;
	char *resource_data = XResourceManagerString( DISP);
	if ( resource_data) {
		XrmCombineDatabase( XrmGetStringDatabase( resource_data), &db, false);
	} else {
		c = getenv( "HOME");
		if (!c) c = "";
		snprintf( filename, PATH_MAX, "%s/.Xdefaults", c);
		XrmCombineFileDatabase( filename, &db, false);
	}
	return db;
}

static int
get_idepth( void)
{
	int i, n;
	XPixmapFormatValues *format = XListPixmapFormats( DISP, &n);
	int idepth = guts.depth;

	if ( !format) return guts.depth;

	for ( i = 0; i < n; i++)
		if ( format[i]. depth == guts. depth) {
			idepth = format[i]. bits_per_pixel;
			break;
		}
	XFree( format);
	return idepth;
}

static Bool  do_x11     = true;
static Bool  do_sync    = false;
static char* do_display = NULL;
static int   do_debug   = 0;
static Bool  do_icccm_only = false;
static Bool  do_no_shmem   = false;
static Bool  do_no_gtk     = false;
static Bool  do_no_quartz  = false;
static Bool  do_no_xrender = false;

static Bool
init_x11( char * error_buf )
{
	/*XXX*/ /* Namely, support for -display host:0.0 etc. */
	XrmQuark common_quarks_list[20];  /*XXX change number of elements if necessary */
	XrmQuarkList ql = common_quarks_list;
	XGCValues gcv;
	char *common_quarks =
		"String."
		"Blinkinvisibletime.blinkinvisibletime."
		"Blinkvisibletime.blinkvisibletime."
		"Clicktimeframe.clicktimeframe."
		"Doubleclicktimeframe.doubleclicktimeframe."
		"Wheeldown.wheeldown."
		"Wheelup.wheelup."
		"Submenudelay.submenudelay."
		"Scrollfirst.scrollfirst."
		"Scrollnext.scrollnext";

	char * atom_names[AI_count] = {
		"RESOLUTION_X",
		"RESOLUTION_Y",
		"PIXEL_SIZE",
		"SPACING",
		"RELATIVE_WEIGHT",
		"FOUNDRY",
		"AVERAGE_WIDTH",
		"CHARSET_REGISTRY",
		"CHARSET_ENCODING",
		"CREATE_EVENT",
		"WM_DELETE_WINDOW",
		"WM_PROTOCOLS",
		"WM_TAKE_FOCUS",
		"_NET_WM_STATE",
		"_NET_WM_STATE_SKIP_TASKBAR",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_NAME",
		"_NET_WM_ICON_NAME",
		"UTF8_STRING",
		"TARGETS",
		"INCR",
		"PIXEL",
		"FOREGROUND",
		"BACKGROUND",
		"_MOTIF_WM_HINTS",
		"_NET_WM_STATE_MODAL",
		"_NET_SUPPORTED",
		"_NET_WM_STATE_MAXIMIZED_HORIZ",
		"text/plain;charset=utf-8",
		"_NET_WM_STATE_STAYS_ON_TOP",
		"_NET_CURRENT_DESKTOP",
		"_NET_WORKAREA",
		"_NET_WM_STATE_ABOVE",
		"XdndEnter",
		"XdndPosition",
		"XdndStatus",
		"XdndTypeList",
		"XdndActionCopy",
		"XdndDrop",
		"XdndLeave",
		"XdndFinished",
		"XdndSelection",
		"XdndProxy",
		"XdndAware",
		"XdndActionMove",
		"XdndActionLink",
		"XdndActionAsk",
		"XdndActionPrivate",
		"XdndActionList",
	 	"XdndActionDescription",
		"text/plain",
		"_NET_WM_ICON",
		"_NET_FRAME_EXTENTS"
	};
	char hostname_buf[256], *hostname = hostname_buf, *env;

	guts. click_time_frame = 200;
	guts. double_click_time_frame = 200;
	guts. visible_timeout = 500;
	guts. invisible_timeout = 500;
	guts. insert = true;
	guts. last_time = CurrentTime;

	guts. ri_head = guts. ri_tail = 0;
	DISP = XOpenDisplay( do_display);

	if (!DISP) {
		char * disp = getenv("DISPLAY");
		snprintf( error_buf, 256, "Error: Can't open display '%s'",
					do_display ? do_display : (disp ? disp : ""));
		free( do_display);
		do_display = NULL;
		return false;
	}
	free( do_display);
	do_display = NULL;
	XSetErrorHandler( x_error_handler);
	guts.main_error_handler = x_error_handler;
	(void)x_io_error_handler;
	XCHECKPOINT;
	guts.connection = ConnectionNumber( DISP);

	{
		struct sockaddr name;
		socklen_t l = sizeof( name);
		guts. local_connection = getsockname( guts.connection, &name, &l) >= 0 && l == 0;
	}

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
	if ( XShapeQueryExtension( DISP, &guts.shape_event, &guts.shape_error)) {
		guts. shape_extension = true;
	} else {
		guts. shape_extension = false;
	}
#else
	guts. shape_extension = false;
#endif
#ifdef USE_MITSHM
	if ( !do_no_shmem && XShmQueryExtension( DISP)) {
		guts. shared_image_extension = true;
		guts. shared_image_completion_event = XShmGetEventBase( DISP) + ShmCompletion;
	} else {
		guts. shared_image_extension = false;
		guts. shared_image_completion_event = -1;
	}
#else
	guts. shared_image_extension = false;
	guts. shared_image_completion_event = -1;
#endif
	guts. randr_extension = false;
#ifdef HAVE_X11_EXTENSIONS_XRANDR_H
	{
		int dummy, major, minor;
		if ( XRRQueryExtension( DISP, &dummy, &dummy)) {
			if ( XRRQueryVersion( DISP, &major, &minor)) {
				if ( major > 1 || (major == 1 && minor > 2 )) {
					/* XRRGetScreenResourcesCurrent appeared in 1.3 */
					guts. randr_extension = true;
				}
			}
		}
	}
#endif
#ifdef HAVE_X11_EXTENSIONS_XCOMPOSITE_H
	{
		int dummy;
		if (XQueryExtension(DISP, COMPOSITE_NAME, &guts.composite_opcode, &dummy, &dummy))
			guts. composite_extension = true;
	}
#endif
	XrmInitialize();
	guts.db = get_database();
	XrmStringToQuarkList( common_quarks, common_quarks_list);
	guts.qString = *ql++;
	guts.qBlinkinvisibletime = *ql++;
	guts.qblinkinvisibletime = *ql++;
	guts.qBlinkvisibletime = *ql++;
	guts.qblinkvisibletime = *ql++;
	guts.qClicktimeframe = *ql++;
	guts.qclicktimeframe = *ql++;
	guts.qDoubleclicktimeframe = *ql++;
	guts.qdoubleclicktimeframe = *ql++;
	guts.qWheeldown = *ql++;
	guts.qwheeldown = *ql++;
	guts.qWheelup = *ql++;
	guts.qwheelup = *ql++;
	guts.qSubmenudelay = *ql++;
	guts.qsubmenudelay = *ql++;
	guts.qScrollfirst = *ql++;
	guts.qscrollfirst = *ql++;
	guts.qScrollnext = *ql++;
	guts.qscrollnext = *ql++;

	guts. mouse_buttons = XGetPointerMapping( DISP, guts. buttons_map, 256);
	XCHECKPOINT;

	guts. limits. request_length    = XMaxRequestSize( DISP);
	guts. limits. extended_request_length = XExtendedMaxRequestSize( DISP);
	if ( guts. limits. extended_request_length == 0 )
		guts. limits. extended_request_length = guts. limits. request_length;
	guts. limits. XDrawLines        = guts. limits. request_length - 3;
	guts. limits. XFillPolygon      = guts. limits. request_length - 4;
	guts. limits. XDrawSegments     = (guts. limits. request_length - 3) / 2;
	guts. limits. XDrawRectangles   = (guts. limits. request_length - 3) / 2;
	guts. limits. XFillRectangles   = (guts. limits. request_length - 3) / 2;
	guts. limits. XFillArcs         =
		guts. limits. XDrawArcs = (guts. limits. request_length - 3) / 3;
	guts. limits. NetWMIcon         = sqrt(guts. limits. extended_request_length - 16);
	XCHECKPOINT;
	SCREEN = DefaultScreen( DISP);

	/* XXX - return code? */
	guts. root = RootWindow( DISP, SCREEN);
	guts. displaySize. x = DisplayWidth( DISP, SCREEN);
	guts. displaySize. y = DisplayHeight( DISP, SCREEN);
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	guts. cursor_width = guts. cursor_height = XcursorGetDefaultSize(DISP);
#else
	XQueryBestCursor( DISP, guts. root,
							guts. displaySize. x,     /* :-) */
							guts. displaySize. y,
							&guts. cursor_width,
							&guts. cursor_height);
#endif
	XCHECKPOINT;

	TAILQ_INIT( &guts.paintq);
	TAILQ_INIT( &guts.peventq);
	TAILQ_INIT( &guts.bitmap_gc_pool);
	TAILQ_INIT( &guts.screen_gc_pool);
	TAILQ_INIT( &guts.argb_gc_pool);

	guts. currentFocusTime = CurrentTime;
	guts. windows = hash_create();
	guts. menu_windows = hash_create();
	guts. ximages = hash_create();
	gcv. graphics_exposures = false;
	gcv. cap_style = CapProjecting;
	guts. menugc = XCreateGC( DISP, guts. root, GCGraphicsExposures | GCCapStyle, &gcv);
	guts. resolution. x = ( DisplayWidthMM( DISP, SCREEN) > 0) ? 
		25.4 * guts. displaySize. x / DisplayWidthMM( DISP, SCREEN) + .5:
		96;
	guts. resolution. y = ( DisplayHeightMM( DISP, SCREEN) > 0) ? 
		25.4 * DisplayHeight( DISP, SCREEN) / DisplayHeightMM( DISP, SCREEN) + .5:
		96;
	guts. depth = DefaultDepth( DISP, SCREEN);
	guts. idepth = get_idepth();
	if ( guts.depth == 1) guts. qdepth = 1; else
	if ( guts.depth <= 4) guts. qdepth = 4; else
	if ( guts.depth <= 8) guts. qdepth = 8; else
		guts. qdepth = 24;
	guts. byte_order = ImageByteOrder( DISP);
	guts. bit_order = BitmapBitOrder( DISP);
	if ( BYTEORDER == LSB32 || BYTEORDER == LSB64)
		guts. machine_byte_order = LSBFirst;
	else if ( BYTEORDER == MSB32 || BYTEORDER == MSB64)
		guts. machine_byte_order = MSBFirst;
	else {
		sprintf( error_buf, "UAA_001: weird machine byte order: %08x", BYTEORDER);
		return false;
	}

	XInternAtoms( DISP, atom_names, AI_count, 0, guts. atoms);

	guts. null_pointer = NULL_HANDLE;
	guts. pointer_invisible_count = 0;
	guts. files = plist_create( 16, 16);
	prima_rebuild_watchers();
	guts. wm_event_timeout = 100;
	guts. menu_timeout = 200;
	guts. scroll_first = 200;
	guts. scroll_next = 50;
	apc_timer_create( CURSOR_TIMER);
	apc_timer_set_timeout(CURSOR_TIMER, 2);
	apc_timer_create( MENU_TIMER);
	apc_timer_set_timeout( MENU_TIMER,  guts. menu_timeout);
	apc_timer_create( MENU_UNFOCUS_TIMER);
	apc_timer_set_timeout( MENU_UNFOCUS_TIMER, 50);
	if ( !prima_init_clipboard_subsystem (error_buf)) return false;
	if ( !prima_init_color_subsystem     (error_buf)) return false;
	if ( !do_no_xrender)
		if ( !prima_init_xrender_subsystem(error_buf)) return false;
	if ( !prima_init_font_subsystem      (error_buf)) return false;
#ifdef WITH_GTK
	guts. use_gtk = do_no_gtk ? false : ( prima_gtk_init() != NULL );
#endif
#ifdef WITH_COCOA
	if ( prima_cocoa_is_x11_local())
		guts. use_quartz = !do_no_quartz;
#endif
	bzero( &guts. cursor_gcv, sizeof( guts. cursor_gcv));
	guts. cursor_gcv. cap_style = CapButt;
	guts. cursor_gcv. function = GXcopy;

	gethostname( hostname, 256);
	hostname[255] = '\0';
	XStringListToTextProperty((char **)&hostname, 1, &guts. hostname);

	guts. net_wm_maximization = prima_wm_net_state_read_maximization( guts. root, NET_SUPPORTED);

	env = getenv("XDG_SESSION_TYPE");
	if (( env != NULL) && (strcmp(env, "wayland") == 0)) {
		guts. is_xwayland = true;
		Mdebug("XWayland detected\n");
	}

	if ( do_sync) XSynchronize( DISP, true);
	return true;
}

Bool
window_subsystem_init( char * error_buf)
{
	bzero( &guts, sizeof( guts));
	guts. debug = do_debug;
	guts. icccm_only = do_icccm_only;
	Mdebug("init x11:%d, debug:%x, sync:%d, display:%s\n", do_x11, guts.debug,
			do_sync, do_display ? do_display : "(default)");
	if ( do_x11) {
		Bool ret = init_x11( error_buf );
		if ( !ret && DISP) {
			XCloseDisplay(DISP);
			DISP = NULL;
		}
		return ret;
	}
	return true;
}

int
prima_debug( const char *format, ...)
{
	int rc = 0;
	va_list args;
	va_start( args, format);
	rc = vfprintf( stderr, format, args);
	va_end( args);
	return rc;
}

Bool
window_subsystem_get_options( int * argc, char *** argv)
{
	static char * x11_argv[] = {
	"no-x11", "runs Prima without X11 display initialized",
	"display", "selects X11 DISPLAY (--display=:0.0)",
	"visual", "X visual id (--visual=0x21, run `xdpyinfo` for list of supported visuals)",
	"sync", "synchronize X connection",
	"icccm", "do not use NET_WM (kde/gnome) and MOTIF extensions, ICCCM only",
	"debug", "turns on debugging on subsystems, selected by characters (--debug=FC). "\
				"Recognized characters are: "\
				" 0(none),"\
				" C(clipboard),"\
				" E(events),"\
				" F(fonts),"\
				" M(miscellaneous),"\
				" P(palettes and colors),"\
				" X(XRDB),"\
				" A(all together)",
#ifdef USE_MITSHM
	"no-shmem",       "do not use shared memory for images",
#endif
	"no-core-fonts", "do not use core fonts",
#ifdef USE_XFT
	"no-xft",        "do not use XFT",
	"no-aa",         "do not anti-alias XFT fonts",
	"font-priority", "match unknown fonts against: 'xft' (default) or 'core'",
#endif
#ifdef WITH_GTK
	"no-gtk",        "do not use GTK",
#endif
#ifdef WITH_HARFBUZZ
	"no-harfbuzz",   "do not use harfbuzz",
#endif
#ifdef WITH_COCOA
	"no-quartz",     "do not use Quartz",
#endif
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	"no-xrender",    "do not use XRender",
#endif
	"font",
#ifdef USE_XFT
				"default prima font in XLFD (-helv-misc-*-*-) or XFT(Helv-12) format",
#else
				"default prima font in XLFD (-helv-misc-*-*-) format",
#endif
	"menu-font", "default menu font",
	"msg-font", "default message box font",
	"widget-font", "default widget font",
	"caption-font", "MDI caption font",
	"noscaled", "do not use scaled instances of fonts",
	"fg", "default foreground color",
	"bg", "default background color",
	"hilite-fg", "default highlight foreground color",
	"hilite-bg", "default highlight background color",
	"disabled-fg", "default disabled foreground color",
	"disabled-bg", "default disabled background color",
	"light", "default light-3d color",
	"dark", "default dark-3d color"
	};
	*argv = x11_argv;
	*argc = sizeof( x11_argv) / sizeof( char*);
	return true;
}

Bool
window_subsystem_set_option( char * option, char * value)
{
	Mdebug("%s=%s\n", option, value);
	if ( strcmp( option, "no-x11") == 0) {
		if ( value) warn("`--no-x11' option has no parameters");
		do_x11 = false;
		return true;
	} else if ( strcmp( option, "yes-x11") == 0) {
		do_x11 = true;
		return true;
	} else if ( strcmp( option, "display") == 0) {
		free( do_display);
		do_display = duplicate_string( value);
		setenv("DISPLAY", value, 1);
		return true;
	} else if ( strcmp( option, "icccm") == 0) {
		if ( value) warn("`--icccm' option has no parameters");
		do_icccm_only = true;
		return true;
	} else if ( strcmp( option, "no-shmem") == 0) {
		if ( value) warn("`--no-shmem' option has no parameters");
		do_no_shmem = true;
		return true;
	} else if ( strcmp( option, "no-gtk") == 0) {
		if ( value) warn("`--no-gtk' option has no parameters");
		do_no_gtk = true;
		return true;
	} else if ( strcmp( option, "no-quartz") == 0) {
		if ( value) warn("`--no-quartz' option has no parameters");
		do_no_quartz = true;
		return true;
	} else if ( strcmp( option, "no-xrender") == 0) {
		if ( value) warn("`--no-xrender' option has no parameters");
		do_no_xrender = true;
		return true;
	} else if ( strcmp( option, "debug") == 0) {
		if ( !value) {
			warn("`--debug' must be given parameters. `--debug=A` assumed\n");
			guts. debug |= DEBUG_ALL;
			do_debug = guts. debug;
			return true;
		}
		while ( *value) switch ( tolower(*(value++))) {
		case '0':
			guts. debug = 0;
			break;
		case 'c':
			guts. debug |= DEBUG_CLIP;
			break;
		case 'e':
			guts. debug |= DEBUG_EVENT;
			break;
		case 'f':
			guts. debug |= DEBUG_FONTS;
			break;
		case 'm':
			guts. debug |= DEBUG_MISC;
			break;
		case 'p':
			guts. debug |= DEBUG_COLOR;
			break;
		case 'x':
			guts. debug |= DEBUG_XRDB;
			break;
		case 'a':
			guts. debug |= DEBUG_ALL;
			break;
		}
		do_debug = guts. debug;
	} else if ( prima_font_subsystem_set_option( option, value)) {
		return true;
	} else if ( prima_color_subsystem_set_option( option, value)) {
		return true;
	}
	return false;
}

void
window_subsystem_cleanup( void)
{
	if ( !DISP) return;
	/*XXX*/
	prima_end_menu();
#ifdef WITH_GTK
	if ( guts. use_gtk) prima_gtk_done();
#endif
}

static void
free_gc_pool( struct gc_head *head)
{
	GCList *n1, *n2;

	n1 = TAILQ_FIRST(head);
	while (n1 != NULL) {
		n2 = TAILQ_NEXT(n1, gc_link);
		XFreeGC( DISP, n1-> gc);
		/* XXX */ free(n1);
		n1 = n2;
	}
	TAILQ_INIT(head);
}

void
window_subsystem_done( void)
{
	int i;
	if ( !DISP) return;

	for ( i = 0; i < sizeof(guts.xdnd_pointers) / sizeof(CustomPointer); i++) {
		CustomPointer *cp = guts.xdnd_pointers + i;
		if ( cp-> cursor )
			XFreeCursor( DISP, cp->cursor);
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
		if ( cp-> xcursor != NULL)
			XcursorImageDestroy(cp-> xcursor);
#endif
	}

	if ( guts. hostname. value) {
		XFree( guts. hostname. value);
		guts. hostname. value = NULL;
	}

	prima_end_menu();

	free_gc_pool(&guts.bitmap_gc_pool);
	free_gc_pool(&guts.screen_gc_pool);
	free_gc_pool(&guts.argb_gc_pool);
	prima_done_color_subsystem();
	prima_done_xrender_subsystem();
	free( guts. clipboard_formats);

	XFreeGC( DISP, guts. menugc);
	prima_gc_ximages();          /* verrry dangerous, very quiet please */
	if ( guts.pointer_font) {
		XFreeFont( DISP, guts.pointer_font);
		guts.pointer_font = NULL;
	}
	XCloseDisplay( DISP);
	DISP = NULL;

	plist_destroy( guts. files);
	guts. files = NULL;

	XrmDestroyDatabase( guts.db);
	if (guts.ximages)            hash_destroy( guts.ximages, false);
	if (guts.menu_windows)       hash_destroy( guts.menu_windows, false);
	if (guts.windows)            hash_destroy( guts.windows, false);
	if (guts.clipboards)         hash_destroy( guts.clipboards, false);
	if (guts.clipboard_xfers)    hash_destroy( guts.clipboard_xfers, false);
	prima_cleanup_font_subsystem();
	bzero(&guts, sizeof(guts));
}

static int
can_access_root_screen(void)
{
	static int result = -1;
	XImage * im;
	XErrorEvent xr;

	if ( result >= 0 ) return result;
	result = 0;

	XFlush(DISP);
	prima_save_xerror_event(&xr);
	im = XGetImage( DISP, guts.root, 0, 0, 1, 1, AllPlanes, ZPixmap); /* XWayland fails here */
	prima_restore_xerror_event(NULL);
	if (im == NULL) goto EXIT;

	XDestroyImage( im);

#ifdef WITH_GTK_NONX11
	/* detect XQuartz */
	{
		char * display_str = getenv("DISPLAY");
		if ( display_str ) {
			struct stat s;
			if ((stat( display_str, &s) >= 0) && S_ISSOCK(s.st_mode))  /* is a socket */
				goto EXIT;
		}
	}
#endif

	result = 1;
EXIT:
	return result;
}

Bool
apc_application_begin_paint( Handle self)
{
	DEFXX;
	if ( guts. appLock > 0) return false;
	if ( !can_access_root_screen()) return false;
	prima_prepare_drawable_for_painting( self, false);
	XX-> flags. force_flush = 1;
	return true;
}

Bool
apc_application_begin_paint_info( Handle self)
{
	prima_prepare_drawable_for_painting( self, false);
	return true;
}

Bool
apc_application_create( Handle self)
{
	XSetWindowAttributes attrs;
	DEFXX;
	if ( !DISP) {
		Mdebug("apc_application_create: failed, x11 layer is not up\n");
		return false;
	}

	XX-> type.application = true;
	XX-> type.widget = true;
	XX-> type.drawable = true;

	attrs. event_mask = StructureNotifyMask | PropertyChangeMask;
	XX-> client = X_WINDOW = XCreateWindow( DISP, guts. root,
		0, 0, 1, 1, 0, CopyFromParent,
		InputOutput, CopyFromParent,
		CWEventMask, &attrs);
	XCHECKPOINT;
	if (!X_WINDOW) return false;
	hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);

	XX-> pointer_id = crArrow;
	XX-> gdrawable = XX-> udrawable = guts. root;
	XX-> parent = None;
	XX-> origin. x = 0;
	XX-> origin. y = 0;
	XX-> ackSize = XX-> size = apc_application_get_size( self);
	XX-> owner = NULL_HANDLE;
	XX-> visual = &guts. visual;

	XX-> flags. clip_owner = 1;
	XX-> flags. sync_paint = 0;

	apc_component_fullname_changed_notify( self);
	guts. mouse_wheel_down = unix_rm_get_int( self, guts.qWheeldown, guts.qwheeldown, 5);
	guts. mouse_wheel_up = unix_rm_get_int( self, guts.qWheelup, guts.qwheelup, 4);
	guts. click_time_frame = unix_rm_get_int( self, guts.qClicktimeframe, guts.qclicktimeframe, guts. click_time_frame);
	guts. double_click_time_frame = unix_rm_get_int( self, guts.qDoubleclicktimeframe, guts.qdoubleclicktimeframe, guts. double_click_time_frame);
	guts. visible_timeout = unix_rm_get_int( self, guts.qBlinkvisibletime, guts.qblinkvisibletime, guts. visible_timeout);
	guts. invisible_timeout = unix_rm_get_int( self, guts.qBlinkinvisibletime, guts.qblinkinvisibletime, guts. invisible_timeout);
	guts. menu_timeout = unix_rm_get_int( self, guts.qSubmenudelay, guts.qsubmenudelay, guts. menu_timeout);
	guts. scroll_first = unix_rm_get_int( self, guts.qScrollfirst, guts.qscrollfirst, guts. scroll_first);
	guts. scroll_next = unix_rm_get_int( self, guts.qScrollnext, guts.qscrollnext, guts. scroll_next);

	prima_send_create_event( X_WINDOW);
	return true;
}

Bool
apc_application_close( Handle self)
{
	guts. applicationClose = true;
	return true;
}

Bool
apc_application_destroy( Handle self)
{
	if ( X_WINDOW) {
		XDestroyWindow( DISP, X_WINDOW);
		XCHECKPOINT;
		hash_delete( guts.windows, (void*)&X_WINDOW, sizeof(X_WINDOW), false);
	}
	prima_guts.application = NULL_HANDLE;
	return true;
}

Bool
apc_application_end_paint( Handle self)
{
	DEFXX;
	XX-> flags. force_flush = 0;
	prima_cleanup_drawable_after_painting( self);
	return true;
}

Bool
apc_application_end_paint_info( Handle self)
{
	prima_cleanup_drawable_after_painting( self);
	return true;
}

int
apc_application_get_gui_info( char * description, int len1, char * language, int len2)
{
	int ret = guiXLib;
	if ( description)
		strlcpy( description, "X Window System", len1);

#ifdef WITH_GTK
	if ( guts. use_gtk ) {
		if ( description) {
			strncat( description, " + GTK", len1);
#ifdef WITH_GTK_NONX11
			strncat( description, " with native support", len1);
#endif
#ifdef WITH_COCOA
			if ( guts. use_quartz)
				strncat( description, " + Cocoa", len1);
#endif
		}
		ret = guiGTK;
	}
#endif

	if ( description)
		description[len1-1] = 0;

	if ( language ) {
		char * lang = getenv("LANG");
		if ( lang ) {
			while (len2 > 1 && (*lang == '-' || islower((int)*lang))) {
				*(language++) = *(lang++);
			}
			*language = 0;
		} else
			*language = 0;
	}

	return ret;
}

Handle
apc_application_get_widget_from_point( Handle self, Point p)
{
	XWindow from, to, child;

	from = to = guts. root;
	p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
	while (XTranslateCoordinates(DISP, from, to, p.x, p.y, &p.x, &p.y, &child)) {
		if (child) {
			from = to;
			to = child;
		} else {
			Handle h;
			if ( to == from) to = X_WINDOW;
			h = (Handle)hash_fetch( guts.windows, (void*)&to, sizeof(to));
			return ( h == prima_guts.application) ? NULL_HANDLE : h;
		}
	}
	return NULL_HANDLE;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
	return prima_xw2h(( XWindow) apiHandle);
}

static Bool
wm_net_get_current_workarea( Rect * r)
{
	Bool ret = false;
	unsigned long n;
	unsigned long *desktop = NULL, *workarea = NULL, *w;

	if ( guts. icccm_only) return false;

	desktop = ( unsigned long *) prima_get_window_property( guts. root,
					NET_CURRENT_DESKTOP, XA_CARDINAL,
					NULL, NULL,
					&n);
	if ( desktop == NULL || n < 1) goto EXIT;
	Mdebug("wm: current desktop = %d\n", *desktop);

	workarea = ( unsigned long *) prima_get_window_property( guts. root,
					NET_WORKAREA, XA_CARDINAL,
					NULL, NULL,
					&n);
	if ( desktop == NULL || n < 1 || n <= *desktop ) goto EXIT;

	w = workarea + *desktop * 4; /* XYWH quartets */
	r-> left   = w[0];
	r-> top    = w[1];
	r-> right  = w[2];
	r-> bottom = w[3];
	ret = true;
	Mdebug("wm: current workarea = %d:%d:%d:%d\n", w[0], w[1], w[2], w[3]);

EXIT:
	free( workarea);
	free( desktop);
	return ret;
}

Rect
apc_application_get_indents( Handle self)
{
	Point sz;
	Rect  r;

	bzero( &r, sizeof( r));
	if ( do_icccm_only) return r;

	sz = apc_application_get_size( self);
	if ( wm_net_get_current_workarea( &r)) {
		r. right  = sz. x - r.right   - r. left;
		r. bottom = sz. y - r. bottom - r. top;
		if ( r. left   < 0) r. left   = 0;
		if ( r. top    < 0) r. top    = 0;
		if ( r. right  < 0) r. right  = 0;
		if ( r. bottom < 0) r. bottom = 0;
	}

	return r;
}

int
apc_application_get_os_info( char *system, int slen,
			char *release, int rlen,
			char *vendor, int vlen,
			char *arch, int alen)
{
	static struct utsname name;
	static Bool fetched = false;

	if (!fetched) {
		if ( uname(&name)!=0) {
			strlcpy( name. sysname, "Some UNIX", sizeof(name.sysname));
			strlcpy( name. release, "Unknown version of UNIX", sizeof(name.release));
			strlcpy( name. machine, "Unknown architecture", sizeof(name.machine));
		}
		fetched = true;
	}

	if (system)
		strlcpy( system, name. sysname, slen);
	if (release)
		strlcpy( release, name. release, rlen);
	if (vendor)
		strlcpy( vendor, "Unknown vendor", vlen);
	if (arch)
		strlcpy( arch, name. machine, alen);

	return apcUnix;
}

Point
apc_application_get_size( Handle self)
{
	return guts. displaySize;
}

Box *
apc_application_get_monitor_rects( Handle self, int * nrects)
{
#if defined(HAVE_X11_EXTENSIONS_XRANDR_H) && defined(HAVE_XRANDR_1_5)
	XRRMonitorInfo *m;
	Box * ret = NULL;
	int n;

	if ( !guts. randr_extension) {
		*nrects = 0;
		return NULL;
	}

	XCHECKPOINT;
	m = XRRGetMonitors(DISP,guts.root,false,&n);
	if ( m ) {
		int i;
		XRRMonitorInfo * mi = m;
		ret = malloc(sizeof(Box) * n);
		*nrects = n;
		for ( i = 0; i < n; i++, mi++) {
			ret[i].x      = mi->x;
			ret[i].y      = guts.displaySize.y - mi->height - mi->y;
			ret[i].width  = mi->width;
			ret[i].height = mi->height;
			if ( i > 0 && mi->primary ) {
				/* primary comes first, if any */
				Box first = *ret;
				*ret = *(ret + i);
				*(ret + i) = first;
			}
		}
		XRRFreeMonitors(m);
		XCHECKPOINT;
	} else {
		*nrects = 0;
	}
	return ret;
#else
	*nrects = 0;
	return NULL;
#endif
}

Bool
apc_application_go( Handle self)
{
	if (!prima_guts.application) return false;

	XNoOp( DISP);
	XFlush( DISP);
	guts. application_stop_signal = false;
	while ( !guts. application_stop_signal && prima_one_loop_round( WAIT_ALWAYS, true))
		;
	guts. application_stop_signal = false;
	return true;
}

Bool
apc_application_lock( Handle self)
{
	guts. appLock++;
	return true;
}

Bool
apc_application_unlock( Handle self)
{
	if ( guts. appLock > 0) guts. appLock--;
	return true;
}

Bool
apc_application_stop( Handle self)
{
	if ( prima_guts.application == NULL_HANDLE ) return false;
	guts. application_stop_signal = true;
	return true;
}

Bool
apc_application_sync(void)
{
	XSync( DISP, false);
	return true;
}

Bool
apc_application_yield( Bool wait_for_event)
{
	if (!prima_guts.application) return false;
	guts. application_stop_signal = false;
	prima_one_loop_round(wait_for_event ? WAIT_IF_NONE : WAIT_NEVER, true);
	guts. application_stop_signal = false;
	XSync( DISP, false);
	return prima_guts.application != NULL_HANDLE && !guts. applicationClose;
}
