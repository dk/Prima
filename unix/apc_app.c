/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
#if !defined(BYTEORDER)
#error "BYTEORDER is not defined"
#endif
#define LSB32   0x1234
#define MSB32   0x4321
#ifndef BUFSIZ
#define BUFSIZ  2048
#endif

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

   if ( ev-> request_code == 42 /*X_SetInputFocus*/) return 0;

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

Bool
window_subsystem_init( void)
{
   /*XXX*/ /* Namely, support for -display host:0.0 etc. */
   XrmQuark common_quarks_list[26];  /*XXX change number of elements if necessary */
   XrmQuarkList ql = common_quarks_list;
   XGCValues gcv;
   char *common_quarks =
      "String."
      "Background.background."
      "Blinkinvisibletime.blinkinvisibletime."
      "Blinkvisibletime.blinkvisibletime."
      "Clicktimeframe.clicktimeframe."
      "Doubleclicktimeframe.doubleclicktimeframe."
      "Font.font."
      "Foreground.foreground."
      "Wheeldown.wheeldown."
      "Wheelup.wheelup."
      "Submenudelay.submenudelay."
      "Scrollfirst.scrollfirst."
      "Scrollnext.scrollnext";
   
   guts. click_time_frame = 200;
   guts. double_click_time_frame = 200;
   guts. visible_timeout = 500;
   guts. invisible_timeout = 500;
   guts. insert = true;
   guts. last_time = CurrentTime;

   guts. ri_head = guts. ri_tail = 0;
   DISP = XOpenDisplay( nil);
   if (!DISP) return false;
   XSetErrorHandler( x_error_handler);
   guts.main_error_handler = x_error_handler;
   (void)x_io_error_handler;
   XCHECKPOINT;
   guts.connection = ConnectionNumber( DISP);

   {
      struct sockaddr name;
      int l = sizeof( name);
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
   if ( XShmQueryExtension( DISP)) {
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
   
   guts.create_event = XInternAtom( DISP, "PRIMA_CREATE", false);

   XrmInitialize();
   guts.db = get_database();
   XrmStringToQuarkList( common_quarks, common_quarks_list);
   guts.qString = *ql++;
   guts.qBackground = *ql++;
   guts.qbackground = *ql++;
   guts.qBlinkinvisibletime = *ql++;
   guts.qblinkinvisibletime = *ql++;
   guts.qBlinkvisibletime = *ql++;
   guts.qblinkvisibletime = *ql++;
   guts.qClicktimeframe = *ql++;
   guts.qclicktimeframe = *ql++;
   guts.qDoubleclicktimeframe = *ql++;
   guts.qdoubleclicktimeframe = *ql++;
   guts.qFont = *ql++;
   guts.qfont = *ql++;
   guts.qForeground = *ql++;
   guts.qforeground = *ql++;
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

   guts. windows = hash_create();
   guts. menu_windows = hash_create();
   guts. ximages = hash_create();
   guts. menugc = XCreateGC( DISP, guts. root, 0, &gcv);
   guts. resolution. x = 25.4 * guts. displaySize. x / DisplayWidthMM( DISP, SCREEN);
   guts. resolution. y = 25.4 * DisplayHeight( DISP, SCREEN) / DisplayHeightMM( DISP, SCREEN);
   guts. depth = DefaultDepth( DISP, SCREEN);
   guts. idepth = get_idepth();
   if ( guts.depth == 1) guts. qdepth = 1; else
   if ( guts.depth <= 4) guts. qdepth = 4; else
   if ( guts.depth <= 8) guts. qdepth = 8; else
      guts. qdepth = 24;
   guts. byte_order = ImageByteOrder( DISP);
   guts. bit_order = BitmapBitOrder( DISP);
   if ( BYTEORDER == LSB32)
      guts. machine_byte_order = LSBFirst;
   else if ( BYTEORDER == MSB32)
      guts. machine_byte_order = MSBFirst;
   else {
      warn( "UAA_001: weird machine byte order: %08x", BYTEORDER);
      return false;
   }   

   guts. null_pointer = nilHandle;
   guts. pointer_invisible_count = 0;
   guts. files = plist_create( 16, 16);
   prima_rebuild_watchers();
   prima_wm_init();
   guts. wm_event_timeout = 100000;
   guts. menu_timeout = 200;
   guts. scroll_first = 200;
   guts. scroll_next = 50;
   apc_timer_create( CURSOR_TIMER, nilHandle, 2);
   apc_timer_create( MENU_TIMER,   nilHandle, guts. menu_timeout);
   if ( !prima_init_clipboard_subsystem()) return false;
   if ( !prima_init_color_subsystem()) return false;
   if ( !prima_init_font_subsystem()) return false;

   {
      XGCValues gcv;
      Pixmap px = XCreatePixmap( DISP, guts.root, 4, 4, 1);
      GC gc = XCreateGC( DISP, px, 0, &gcv);
      XImage *xi;
      XSetForeground( DISP, gc, 0);
      XFillRectangle( DISP, px, gc, 0, 0, 5, 5);
      XSetForeground( DISP, gc, 1);
      XDrawArc( DISP, px, gc, 0, 0, 4, 4, 0, 360 * 64);
      if (( xi = XGetImage( DISP, px, 0, 0, 4, 4, 1, XYPixmap))) {
         int i;
         Byte *data[4];
         if ( xi-> bitmap_bit_order == LSBFirst) 
            prima_mirror_bytes( xi-> data, xi-> bytes_per_line * 4);
         for ( i = 0; i < 4; i++) data[i] = (Byte*)xi-> data + i * xi-> bytes_per_line;
#define PIX(x,y) ((data[y][0] & (0x80>>(x)))!=0)
         if (  PIX(2,1) && !PIX(3,1)) guts. ellipseDivergence.x = -1; else
         if ( !PIX(2,1) && !PIX(3,1)) guts. ellipseDivergence.x = 1; 
         if (  PIX(1,2) && !PIX(1,3)) guts. ellipseDivergence.y = -1; else
         if ( !PIX(1,2) && !PIX(1,3)) guts. ellipseDivergence.y = 1; 
#undef PIX                          
         XDestroyImage( xi);
      }
      XFreeGC( DISP, gc);
      XFreePixmap( DISP, px);
   }
/*    XSynchronize( DISP, true); */
   return true;
}

void
window_subsystem_cleanup( void)
{
   /*XXX*/
   prima_end_menu();
   if ( guts. wm_cleanup)
      guts. wm_cleanup();
}

static void
free_gc_pool( struct gc_head *head)
{
   GCList *n1, *n2;

   n1 = TAILQ_FIRST(head);
   while (n1 != nil) {
      n2 = TAILQ_NEXT(n1, gc_link);
      /* XXX */ free(n1);
      n1 = n2;
   }
   TAILQ_INIT(head);
}

void
window_subsystem_done( void)
{
   prima_end_menu();
   free_gc_pool(&guts.bitmap_gc_pool);
   free_gc_pool(&guts.screen_gc_pool);
   prima_done_color_subsystem();
   free( guts. clipboard_formats);

   if ( DISP) {
      XFreeGC( DISP, guts. menugc);
      prima_gc_ximages();          /* verrry dangerous, very quiet please */
      if ( guts.pointer_font) {
         XFreeFont( DISP, guts.pointer_font);
         guts.pointer_font = nil;
      }
      XCloseDisplay( DISP);
      DISP = nil;
   }
   
   plist_destroy( guts. files);
   guts. files = nil;

   XrmDestroyDatabase( guts.db);
   if (guts.ximages)            hash_destroy( guts.ximages, false);
   if (guts.menu_windows)       hash_destroy( guts.menu_windows, false);
   if (guts.windows)            hash_destroy( guts.windows, false);
   if (guts.clipboards)         hash_destroy( guts.clipboards, false);
   prima_cleanup_font_subsystem();
}

Bool
apc_application_begin_paint( Handle self)
{
   if ( guts. appLock > 0) return false;
   prima_prepare_drawable_for_painting( self, false);
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

   XX-> type.application = true;
   XX-> type.widget = true;
   XX-> type.drawable = true;

   attrs. event_mask = StructureNotifyMask;
   X_WINDOW = XCreateWindow( DISP, guts. root,
			     0, 0, 1, 1, 0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     CWEventMask, &attrs);
   XCHECKPOINT;
   if (!X_WINDOW) return false;
   hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);

   XX-> pointer_id = crArrow;
   XX-> gdrawable = XX-> udrawable = guts. root;
   XX-> parent = None;
   XX-> origin = ( Point){0,0};
   XX-> ackSize = XX-> size = apc_application_get_size( self);
   XX-> owner = nilHandle;

   XX-> flags. clip_owner = 1;
   XX-> flags. sync_paint = 0;

   apc_component_fullname_changed_notify( self);
   guts. mouse_wheel_down = unix_rm_get_int( self, guts.qWheeldown, guts.qwheeldown, 0);
   guts. mouse_wheel_up = unix_rm_get_int( self, guts.qWheelup, guts.qwheelup, 0);
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
   Object_destroy( self);
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
   if ( description) {
      strncpy( description, "X Window System", len);
      description[len-1] = 0;
   }
   return guiXLib;
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
         h = prima_xw2h( to);
         return ( h == application) ? nilHandle : h;
      }
   }
   return nilHandle;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
   /* NIY */
   return guts. root;
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

typedef struct {
   int no;
   void *sub;
   void *glob;
} FileList, *PFileList;

Bool
apc_watch_filehandle( int no, void *sub, void *glob)
{
   PFileList f = malloc( sizeof( FileList));
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
            if ( guts. currentMenu) {
               XEvent ev;
               ev. type = MenuTimerMessage;
               prima_handle_menu_event( &ev, M(guts. currentMenu)-> w-> w, guts. currentMenu);
            }
            apc_timer_stop( MENU_TIMER);
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
         sig_t oldHandler = signal( SIGPIPE, SIG_IGN);
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
      list_first_that( guts.files, purge_invalid_watchers, nil);
   } else {
      if ( r > 0) {
         for ( i = 0; i < guts.files->count; i++) {
            PFile f = (PFile)list_at( guts.files,i);
            if ( FD_ISSET( f->fd, &read_set)) {
               prima_simple_message((Handle)f, cmFileRead, false);
               break;
            } else if ( FD_ISSET( f->fd, &write_set)) {
               prima_simple_message((Handle)f, cmFileWrite, false);
               break;
            } else if ( FD_ISSET( f->fd, &excpt_set)) {
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

static XBool
any_event( Display *d, XEvent *ev, XPointer arg)
{
   (void)d; (void)ev; (void)arg; (void)any_event;
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
