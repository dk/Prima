/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

/***********************************************************/
/*                                                         */
/*  System dependent application management (unix, x11)    */
/*                                                         */
/***********************************************************/

#include "apricot.h"
#include "unix/guts.h"
#include "Application.h"
#include "File.h"
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
      "text/plain;charset=UTF-8",
      "_NET_WM_STATE_STAYS_ON_TOP",
      "_NET_CURRENT_DESKTOP",
      "_NET_WORKAREA",
      "_NET_WM_STATE_ABOVE"
   };
   char hostname_buf[256], *hostname = hostname_buf;

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
      do_display = nil;
      return false;
   }
   free( do_display);
   do_display = nil;
   XSetErrorHandler( x_error_handler);
   guts.main_error_handler = x_error_handler;
   (void)x_io_error_handler;
   XCHECKPOINT;
   guts.connection = ConnectionNumber( DISP);

   {
      struct sockaddr name;
      unsigned int l = sizeof( name);
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
      int dummy;
      if ( XRRQueryExtension( DISP, &dummy, &dummy))
         guts. randr_extension = true;
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

   guts. limits. request_length = XMaxRequestSize( DISP);
   guts. limits. XDrawLines = guts. limits. request_length - 3;
   guts. limits. XFillPolygon = guts. limits. request_length - 4;
   guts. limits. XDrawSegments = (guts. limits. request_length - 3) / 2;
   guts. limits. XDrawRectangles = (guts. limits. request_length - 3) / 2;
   guts. limits. XFillRectangles = (guts. limits. request_length - 3) / 2;
   guts. limits. XFillArcs =
      guts. limits. XDrawArcs = (guts. limits. request_length - 3) / 3;
   XCHECKPOINT;
   SCREEN = DefaultScreen( DISP);

   /* XXX - return code? */
   guts. root = RootWindow( DISP, SCREEN);
   guts. displaySize. x = DisplayWidth( DISP, SCREEN);
   guts. displaySize. y = DisplayHeight( DISP, SCREEN);
   XQueryBestCursor( DISP, guts. root,
		     guts. displaySize. x,     /* :-) */
		     guts. displaySize. y,
		     &guts. cursor_width,
		     &guts. cursor_height);
   XCHECKPOINT;
   
   TAILQ_INIT( &guts.paintq);
   TAILQ_INIT( &guts.peventq);
   TAILQ_INIT( &guts.bitmap_gc_pool);
   TAILQ_INIT( &guts.screen_gc_pool);

   guts. currentFocusTime = CurrentTime;
   guts. windows = hash_create();
   guts. menu_windows = hash_create();
   guts. ximages = hash_create();
   gcv. graphics_exposures = false;
   guts. menugc = XCreateGC( DISP, guts. root, GCGraphicsExposures, &gcv);
   guts. resolution. x = 25.4 * guts. displaySize. x / DisplayWidthMM( DISP, SCREEN) + .5;
   guts. resolution. y = 25.4 * DisplayHeight( DISP, SCREEN) / DisplayHeightMM( DISP, SCREEN) + .5;
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

   guts. null_pointer = nilHandle;
   guts. pointer_invisible_count = 0;
   guts. files = plist_create( 16, 16);
   prima_rebuild_watchers();
   guts. wm_event_timeout = 100;
   guts. menu_timeout = 200;
   guts. scroll_first = 200;
   guts. scroll_next = 50;
   apc_timer_create( CURSOR_TIMER, nilHandle, 2);
   apc_timer_create( MENU_TIMER,   nilHandle, guts. menu_timeout);
   apc_timer_create( MENU_UNFOCUS_TIMER,   nilHandle, 50);
   if ( !prima_init_clipboard_subsystem( error_buf)) return false;
   if ( !prima_init_color_subsystem( error_buf)) return false;
   if ( !prima_init_font_subsystem( error_buf)) return false;
   bzero( &guts. cursor_gcv, sizeof( guts. cursor_gcv));
   guts. cursor_gcv. cap_style = CapButt;
   guts. cursor_gcv. function = GXcopy;

   gethostname( hostname, 256);
   hostname[255] = '\0';
   XStringListToTextProperty((char **)&hostname, 1, &guts. hostname);
   
   guts. net_wm_maximization = prima_wm_net_state_read_maximization( guts. root, NET_SUPPORTED);

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
	 DISP = nil;
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
      return true;
   } else if ( strcmp( option, "icccm") == 0) {
      if ( value) warn("`--icccm' option has no parameters");
      do_icccm_only = true;
      return true;
   } else if ( strcmp( option, "no-shmem") == 0) {
      if ( value) warn("`--no-shmem' option has no parameters");
      do_no_shmem = true;
      return true;
   } else if ( strcmp( option, "debug") == 0) {
      if ( !value) {
	 warn("`--debug' must be given parameters. `--debug=A` assumed\n");
	 guts. debug |= DEBUG_ALL;
         do_debug = guts. debug;
	 return true;
      }
      while ( *value) switch ( tolower(*(value++))) {
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
   prima_gtk_done();
#endif
}

static void
free_gc_pool( struct gc_head *head)
{
   GCList *n1, *n2;

   n1 = TAILQ_FIRST(head);
   while (n1 != nil) {
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
   if ( !DISP) return;

   if ( guts. hostname. value) {
      XFree( guts. hostname. value);
      guts. hostname. value = nil;
   }

   prima_end_menu();

   free_gc_pool(&guts.bitmap_gc_pool);
   free_gc_pool(&guts.screen_gc_pool);
   prima_done_color_subsystem();
   free( guts. clipboard_formats);

   XFreeGC( DISP, guts. menugc);
   prima_gc_ximages();          /* verrry dangerous, very quiet please */
   if ( guts.pointer_font) {
      XFreeFont( DISP, guts.pointer_font);
      guts.pointer_font = nil;
   }
   XCloseDisplay( DISP);
   DISP = nil;
   
   plist_destroy( guts. files);
   guts. files = nil;

   XrmDestroyDatabase( guts.db);
   if (guts.ximages)            hash_destroy( guts.ximages, false);
   if (guts.menu_windows)       hash_destroy( guts.menu_windows, false);
   if (guts.windows)            hash_destroy( guts.windows, false);
   if (guts.clipboards)         hash_destroy( guts.clipboards, false);
   if (guts.clipboard_xfers)    hash_destroy( guts.clipboard_xfers, false);
   prima_cleanup_font_subsystem();
}

Bool
apc_application_begin_paint( Handle self)
{
   DEFXX;
   if ( guts. appLock > 0) return false;
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
   XX-> owner = nilHandle;

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
apc_application_get_gui_info( char * description, int len)
{
#ifdef WITH_GTK2
   if ( description) {
      strncpy( description, "X Window System + GTK2", len);
      description[len-1] = 0;
   }
   return guiGTK2;
#else
   if ( description) {
      strncpy( description, "X Window System", len);
      description[len-1] = 0;
   }
   return guiXLib;
#endif
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
         return ( h == application) ? nilHandle : h;
      }
   }
   return nilHandle;
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

#ifndef SYS_NMLN
#ifdef _SYS_NAMELEN
#define SYS_NMLN _SYS_NAMELEN
#else
#define SYS_NMLN 64
#endif
#endif   

   if (!fetched) {
      if ( uname(&name)!=0) {
	 strncpy( name. sysname, "Some UNIX", SYS_NMLN);
	 name. sysname[ SYS_NMLN-1] = 0;
	 strncpy( name. release, "Unknown version of UNIX", SYS_NMLN);
	 name. release[ SYS_NMLN-1] = 0;
	 strncpy( name. machine, "Unknown architecture", SYS_NMLN);
	 name. machine[ SYS_NMLN-1] = 0;
      }
      fetched = true;
   }

   if (system) {
      strncpy( system, name. sysname, slen);
      system[ slen-1] = 0;
   }
   if (release) {
      strncpy( release, name. release, rlen);
      release[ rlen-1] = 0;
   }
   if (vendor) {
      strncpy( vendor, "Unknown vendor", vlen);
      vendor[ vlen-1] = 0;
   }
   if (arch) {
      strncpy( arch, name. machine, alen);
      arch[ alen-1] = 0;
   }

   return apcUnix;
}

Point
apc_application_get_size( Handle self)
{
   return guts. displaySize;
}

Rect2 *
apc_application_get_monitor_rects( Handle self, int * nrects)
{
#ifdef HAVE_X11_EXTENSIONS_XRANDR_H
   XRRScreenResources * sr;
   Rect2 * ret = nil;

   if ( !guts. randr_extension) {
      *nrects = 0;
      return nil;
   }

   XCHECKPOINT;
   sr = XRRGetScreenResources(DISP,guts.root);
   if ( sr ) {
      int i;
      ret = malloc(sizeof(Rect2) * sr->ncrtc);
      *nrects = sr->ncrtc;
      for ( i = 0; i < sr->ncrtc; i++) {
	 XRRCrtcInfo * ci = XRRGetCrtcInfo (DISP, sr, sr->crtcs[i]);
	 ret[i].x      = ci->x;
	 ret[i].y      = guts.displaySize.y - ci->height - ci->y;
	 ret[i].width  = ci->width;
	 ret[i].height = ci->height;
	 XRRFreeCrtcInfo(ci);
      }
      XRRFreeScreenResources(sr);
      XCHECKPOINT;
   } else {
      *nrects = sr->ncrtc;
   }
   return ret;
#else   
   *nrects = 0;
   return nil;
#endif
}

typedef struct {
   int no;
   void *sub;
   void *glob;
} FileList, *PFileList;

Bool
apc_watch_filehandle( int no, void *sub, void *glob)
{
   PFileList f = malloc( sizeof( FileList));
   if ( !f) return false;
   f->no = no;
   f->sub = sub;
   f->glob = glob;
   list_add( guts.files, (Handle)f);
   return true;
}

#ifdef PerlIO
typedef PerlIO *FileStream;
#else
#define PERLIO_IS_STDIO 1
typedef FILE *FileStream;
#define PerlIO_fileno(f) fileno(f)
#endif

static Bool
purge_invalid_watchers( Handle self, void *dummy)
{
   ((PFile)self)->self->is_active(self,true);
   return false;
}

static void
perform_pending_paints( void)
{
   PDrawableSysData selfxx, next;

   for ( XX = TAILQ_FIRST( &guts.paintq); XX != nil; ) {
      next = TAILQ_NEXT( XX, paintq_link);
      if ( XX-> flags. paint_pending && (guts. appLock == 0) &&
         (PWidget( XX->self)-> stage == csNormal)) {
         TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
         XX-> flags. paint_pending = false;
         prima_simple_message( XX-> self, cmPaint, false);
         /* handle the case where this widget is locked */
         if (XX->invalid_region) {
            XDestroyRegion(XX->invalid_region);
            XX->invalid_region = nil;
         }
      } 
      XX = next;
   }
}

static void
send_pending_events( void)
{
   PendingEvent *pe, *next;
   int stage;

   for ( pe = TAILQ_FIRST( &guts.peventq); pe != nil; ) {
      next = TAILQ_NEXT( pe, peventq_link);
      if (( stage = PComponent( pe->recipient)-> stage) != csConstructing) {
         TAILQ_REMOVE( &guts.peventq, pe, peventq_link);
      }
      if ( stage == csNormal)
         apc_message( pe-> recipient, &pe-> event, false);
      if ( stage != csConstructing) {
         free( pe);
      }
      pe = next;
   }
}

Bool
prima_one_loop_round( Bool wait, Bool careOfApplication)
{
   XEvent ev, next_event;
   fd_set read_set, write_set, excpt_set;
   struct timeval timeout;
   int r, i, queued_events;
   PTimerSysData timer;

   if ( guts. applicationClose) return false;

   if (( queued_events = XEventsQueued( DISP, QueuedAlready))) {
      goto FetchAndProcess;
   }
   read_set = guts.read_set;
   write_set = guts.write_set;
   excpt_set = guts.excpt_set;
   if ( guts. oldest) {
      gettimeofday( &timeout, nil);
      if ( guts. oldest-> when. tv_sec < timeout. tv_sec ||
           ( guts. oldest-> when. tv_sec == timeout. tv_sec &&
             guts. oldest-> when. tv_usec <= timeout. tv_usec)) {
         timer = guts. oldest;
         apc_timer_start( timer-> who);
         if ( timer-> who == CURSOR_TIMER) {
            prima_cursor_tick();
         } else if ( timer-> who == MENU_TIMER) {
            apc_timer_stop( MENU_TIMER);
            if ( guts. currentMenu) {
               XEvent ev;
               ev. type = MenuTimerMessage;
               prima_handle_menu_event( &ev, M(guts. currentMenu)-> w-> w, guts. currentMenu);
            }
         } else if ( timer-> who == MENU_UNFOCUS_TIMER) {
            prima_end_menu();
         } else {
            prima_simple_message( timer-> who, cmTimer, false);
         }
         gettimeofday( &timeout, nil);
      }
      if ( guts. oldest && wait) {
         if ( guts. oldest-> when. tv_sec < timeout. tv_sec) {
            timeout. tv_sec = 0;
            timeout. tv_usec = 0;
         } else {
            timeout. tv_sec = guts. oldest-> when. tv_sec - timeout. tv_sec;
            if ( guts. oldest-> when. tv_usec < timeout. tv_usec) {
               if ( timeout. tv_sec == 0) {
                  timeout. tv_sec = 0;
                  timeout. tv_usec = 0;
               } else {
                  timeout. tv_sec--;
                  timeout. tv_usec = 1000000 - (timeout. tv_usec - guts. oldest-> when. tv_usec);
               }
            } else {
               timeout. tv_usec = guts. oldest-> when. tv_usec - timeout. tv_usec;
            }
         }
         if ( timeout. tv_sec > 0 || timeout. tv_usec > 200000) {
            timeout. tv_sec = 0;
            timeout. tv_usec = 200000;
         }
      } else {
         timeout. tv_sec = 0;
         if ( wait)
            timeout. tv_usec = 200000;
         else
            timeout. tv_usec = 0;
      }
   } else {
      timeout. tv_sec = 0;
      if ( wait)
         timeout. tv_usec = 200000;
      else
         timeout. tv_usec = 0;
   }
   if (( r = select( guts.max_fd+1, &read_set, &write_set, &excpt_set, &timeout)) > 0 &&
       FD_ISSET( guts.connection, &read_set)) {
      if (( queued_events = XEventsQueued( DISP, QueuedAfterFlush)) <= 0) {
         /* just like tcl/perl tk do, to avoid an infinite loop */
         RETSIGTYPE oldHandler = signal( SIGPIPE, SIG_IGN);
         XNoOp( DISP);
         XFlush( DISP);
         (void) signal( SIGPIPE, oldHandler);
      }
FetchAndProcess:
      if ( queued_events && ( application || !careOfApplication)) {
         XNextEvent( DISP, &ev);
         XCHECKPOINT;
         queued_events--;
         while ( queued_events > 0) {
            if (!application && careOfApplication) return false;
            XNextEvent( DISP, &next_event);
            XCHECKPOINT;
            prima_handle_event( &ev, &next_event);
            guts. total_events++;
            queued_events = XEventsQueued( DISP, QueuedAlready);
            memcpy( &ev, &next_event, sizeof( XEvent));
         }
         if (!application && careOfApplication) return false;
         guts. total_events++;
         prima_handle_event( &ev, nil);
      }
      XNoOp( DISP);
      XFlush( DISP);
   } else if ( r < 0) {
      list_first_that( guts.files, (void*)purge_invalid_watchers, nil);
   } else {
      if ( r > 0) {
         for ( i = 0; i < guts.files->count; i++) {
            PFile f = (PFile)list_at( guts.files,i);
            if ( FD_ISSET( f->fd, &read_set) && (f->eventMask & feRead)) {
               prima_simple_message((Handle)f, cmFileRead, false);
               break;
            } else if ( FD_ISSET( f->fd, &write_set) && (f->eventMask & feWrite)) {
               prima_simple_message((Handle)f, cmFileWrite, false);
               break;
            } else if ( FD_ISSET( f->fd, &excpt_set) && (f->eventMask & feException)) {
               prima_simple_message((Handle)f, cmFileException, false);
               break;
            }
         }
      } else {
         XNoOp( DISP);
         XFlush( DISP);
      }
   }
   send_pending_events();
   perform_pending_paints();
   kill_zombies();
   return application != nilHandle;
}

Bool
apc_application_go( Handle self)
{
   if (!application) return false;

   XNoOp( DISP);
   XFlush( DISP);

   while ( prima_one_loop_round( true, true))
      ;

   if ( application) Object_destroy( application);
   application = nilHandle;
   CHECK_LEAKS;
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
apc_application_sync(void)
{
   XSync( DISP, false);
   return true;
}

Bool
apc_application_yield( void)
{
   if (!application) return false;
   XSync( DISP, false);
   prima_one_loop_round( false, true);
   XSync( DISP, false);
   prima_one_loop_round( false, true);
   return true;
}
