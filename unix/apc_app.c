/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
 */

/***********************************************************/
/*                                                         */
/*  System dependent application management (unix, x11)    */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Application.h"
#include "File.h"
#include <sys/utsname.h>

static int
x_error_handler( Display *d, XErrorEvent *ev)
{
   int tail = guts. ri_tail;
   int prev = tail;
   char buf[ 1024];

   while ( tail != guts. ri_head) {
      if ( guts. ri[ tail]. request > ev-> serial)
	 break;
      prev = tail;
      tail++;
      if ( tail >= REQUEST_RING_SIZE)
	 tail = 0;
   }

   XGetErrorText( d, ev-> error_code, buf, 1024);
   fprintf( stderr, "%s\n", buf);
   if ( tail == guts. ri_head && prev == guts. ri_head)
      fprintf( stderr, "This occured in unknown place\n");
   else if ( tail == guts. ri_head)
      fprintf( stderr, "This occured somewhere after the checkpoint at line %d and file %s\n",
	       guts. ri[ prev]. line, guts. ri[ prev]. file);
   else
      fprintf( stderr, "This occured somewhere between the checkpoint at\n"
	       " line %d and file %s, and another checkpoint at\n"
	       " line %d and file %s\n",
	       guts. ri[ prev]. line, guts. ri[ prev]. file,
	       guts. ri[ tail]. line, guts. ri[ tail]. file);
   _exit( 1);
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

Bool
window_subsystem_init( void)
{
   /*XXX*/ /* Namely, support for -display host:0.0 etc. */
   XrmQuark common_quarks_list[16];
   XrmQuarkList ql = common_quarks_list;
   char *common_quarks =
      "String."
      "Background.background."
      "Blinkinvisibletime.blinkinvisibletime."
      "Blinkvisibletime.blinkvisibletime."
      "Font.font."
      "Foreground.foreground."
      "Wheeldown.wheeldown."
      "Wheelup.wheelup";
   
   guts. visible_timeout = 500;
   guts. invisible_timeout = 500;
   guts. insert = true;

   guts. ri_head = guts. ri_tail = 0;
   DISP = XOpenDisplay( nil);
   if (!DISP) return false;
   XSetErrorHandler( x_error_handler);
   (void)x_io_error_handler;
   XCHECKPOINT;
   guts.connection = ConnectionNumber( DISP);
   
#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
   if ( XShapeQueryExtension( DISP, &guts.shape_event, &guts.shape_error)) {
      guts. shape_extension = true;
   } else {
      guts. shape_extension = false;
   }
#else
   guts. shape_extension = false;
#endif

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
   guts.qbackground = *ql++;
   guts.qFont = *ql++;
   guts.qfont = *ql++;
   guts.qForeground = *ql++;
   guts.qforeground = *ql++;
   guts.qWheeldown = *ql++;
   guts.qwheeldown = *ql++;
   guts.qWheelup = *ql++;
   guts.qwheelup = *ql++;

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
   XQueryBestCursor( DISP, RootWindow( DISP, SCREEN),
		     DisplayWidth( DISP, SCREEN),     /* :-) */
		     DisplayHeight( DISP, SCREEN),
		     &guts. cursor_width,
		     &guts. cursor_height);
   XCHECKPOINT;

   guts. windows = hash_create();
   guts. resolution. x = 25.4 * DisplayWidth( DISP, SCREEN) / DisplayWidthMM( DISP, SCREEN);
   guts. resolution. y = 25.4 * DisplayHeight( DISP, SCREEN) / DisplayHeightMM( DISP, SCREEN);
   guts. depth = DefaultDepth( DISP, SCREEN);
   guts. byte_order = ImageByteOrder( DISP);
   guts. bit_order = BitmapBitOrder( DISP);
   
   guts. files = plist_create( 16, 16);
   prima_rebuild_watchers();
   prima_wm_init();
   prima_init_font_subsystem();
/*    XSynchronize( DISP, true); */
   return true;
}

void
window_subsystem_cleanup( void)
{
   /*XXX*/
   if ( guts. wm_cleanup)
      guts. wm_cleanup();
}

void
window_subsystem_done( void)
{
   int n;
   GCList *gcl;

   for ( n = 0, gcl = guts. used_gcl; gcl; n++, gcl = gcl-> next) {
   }
   DOLBUG( "Used GC's: %d\n", n);
   for ( n = 0, gcl = guts. free_gcl; gcl; n++, gcl = gcl-> next) {
   }
   DOLBUG( "Free GC's: %d\n", n);

   if (DISP) XCloseDisplay( DISP);
   DISP = nil;
   
   plist_destroy( guts. files);
   guts. files = nil;

   DOLBUG( "Total X events: %ld\n", guts. total_events);
   DOLBUG( "Handled X events: %ld\n", guts. handled_events);
   DOLBUG( "Unhandled X events: %ld\n", guts. unhandled_events);
   DOLBUG( "Skipped X events: %ld\n", guts. skipped_events);

   XrmDestroyDatabase( guts.db);
   if (guts.windows) hash_destroy( guts.windows, false);
   prima_cleanup_font_subsystem();
}

Bool
apc_application_begin_paint( Handle self)
{
   prima_prepare_drawable_for_painting( self);
   return true;
}

Bool
apc_application_begin_paint_info( Handle self)
{
   DOLBUG( "apc_application_begin_paint_info()\n");
/*NYI*/
   return true;
}

Bool
apc_application_create( Handle self)
{
   XSetWindowAttributes attrs;
   DEFXX;

   attrs. event_mask = StructureNotifyMask;
   X_WINDOW = XCreateWindow( DISP, RootWindow( DISP, SCREEN),
			     0, 0, 1, 1, 0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     CWEventMask, &attrs);
   XCHECKPOINT;
   if (!X_WINDOW) return false;
   hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);

   XX-> drawable = RootWindow( DISP, SCREEN);
   XX-> parent = None;
   XX-> origin = (Point){0,0};
   XX-> size = apc_application_get_size( self);
   XX-> owner = nilHandle;

   XX-> flags. clip_owner = 1;
   XX-> flags. sync_paint = 0;

   apc_component_fullname_changed_notify( self);
   guts. mouse_wheel_down = unix_rm_get_int( self, guts.qWheeldown, guts.qwheeldown, 0);
   guts. mouse_wheel_up = unix_rm_get_int( self, guts.qWheelup, guts.qwheelup, 0);
   guts. visible_timeout = unix_rm_get_int( self, guts.qBlinkvisibletime, guts.qblinkvisibletime, 200);
   guts. invisible_timeout = unix_rm_get_int( self, guts.qBlinkinvisibletime, guts.qblinkinvisibletime, 200);

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
   DOLBUG( "apc_application_end_paint_info()\n");
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

   from = to = RootWindow(DISP,SCREEN);
   p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
   while (XTranslateCoordinates(DISP, from, to, p.x, p.y, &p.x, &p.y, &child)) {
      if (child) {
         from = to;
         to = child;
      } else {
         if ( to == from) to = X_WINDOW;
         return prima_xw2h( to);
      }
   }
   return nilHandle;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
   /* NIY */
   return RootWindow(DISP,SCREEN);
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
   return (Point){
      DisplayWidth( DISP, SCREEN),
      DisplayHeight( DISP, SCREEN)
   };
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

void
apc_watch_notify( PFileList f, int kind)
{
   dSP;
   ENTER;
   SAVETMPS;
   PUSHMARK( sp);
   EXTEND( sp, 2);
   PUSHs((SV*)f->glob);
   PUSHs( sv_2mortal( newSViv( kind)));
   PUTBACK;
#ifdef PERL_CALL_SV_DIE_BUG_AWARE
   perl_call_sv(( SV *) f->sub, G_DISCARD|G_EVAL);
   if ( SvTRUE( GvSV( errgv))) {
      croak( SvPV( GvSV( errgv), na));
   }
#else
   perl_call_sv(( SV *) f->sub, G_DISCARD);
#endif
   SPAGAIN; FREETMPS; LEAVE;
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

Bool
apc_application_go( Handle self)
{
/*NCI*/
   XEvent ev, next_event;
   fd_set read_set, write_set, excpt_set;
   struct timeval timeout;
   int r, n, i;

   XNoOp( DISP);
   XFlush( DISP);
//   XCHECKPOINT;

   for (;;) {
      if (!application) return false;
      if (( n = XEventsQueued( DISP, QueuedAlready))) {
	 goto FetchAndProcess;
      }
      read_set = guts.read_set;
      write_set = guts.write_set;
      excpt_set = guts.excpt_set;
      if ( guts. oldest) {
	 if ( gettimeofday( &timeout, nil) != 0) {
	    croak( "apc_application_go() gettimeofday() returned: %s", strerror( errno));
	 }
	 if ( guts. oldest-> when. tv_sec < timeout. tv_sec ||
	      ( guts. oldest-> when. tv_sec == timeout. tv_sec &&
		guts. oldest-> when. tv_usec <= timeout. tv_usec)) {
	    Event e;
	    PTimerSysData timer = guts. oldest;

	    e. cmd = cmTimer;
	    apc_timer_start( timer-> who);
	    if ( timer-> who == CURSOR_TIMER) {
	       prima_cursor_tick();
	    } else {
	       CComponent( timer-> who)-> message( timer-> who, &e);
	    }
	 }
	 if ( guts. oldest) {
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
	    timeout. tv_usec = 200000;
	 }
      } else {
	 timeout. tv_sec = 0;
	 timeout. tv_usec = 200000;
      }

      if (( r = select( guts.max_fd+1, &read_set, &write_set, &excpt_set, &timeout)) > 0 &&
	  FD_ISSET( guts.connection, &read_set)) {
	 if (( n = XEventsQueued( DISP, QueuedAfterFlush)) <= 0) {
	    /* just like tcl/perl tk do, to avoid an infinite loop */
	    sig_t oldHandler = signal( SIGPIPE, SIG_IGN);
	    XNoOp( DISP);
	    XFlush( DISP);
	    (void) signal( SIGPIPE, oldHandler);
	 }
FetchAndProcess:
	 if (n && application) {
	    XNextEvent( DISP, &ev);
	    XCHECKPOINT;
	    n--;
	    while ( n > 0) {
	       if (!application) return true;
	       XNextEvent( DISP, &next_event);
	       XCHECKPOINT;
	       guts. total_events++;
	       prima_handle_event( &ev, &next_event);
	       n--;
	       memcpy( &ev, &next_event, sizeof( XEvent));
	    }
	    if (!application) return true;
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
		  Event e;
		  bzero( &e, sizeof e);
		  e. cmd = cmFileRead;
		  f-> self-> message((Handle)f,&e);
		  break;
	       }
	       if ( FD_ISSET( f->fd, &write_set)) {
		  Event e;
		  bzero( &e, sizeof e);
		  e. cmd = cmFileWrite;
		  f-> self-> message((Handle)f,&e);
		  break;
	       }
	       if ( FD_ISSET( f->fd, &excpt_set)) {
		  Event e;
		  bzero( &e, sizeof e);
		  e. cmd = cmFileException;
		  f-> self-> message((Handle)f,&e);
		  break;
	       }
	    }
	 } else {
	    XNoOp( DISP);
	    XFlush( DISP);
	 }
      }
      {
	 PPaintList pl = guts. paint_list;
	 Event e;
	 while ( pl) {
	    PPaintList cur = pl;
	    Handle self = cur-> obj;
	    guts. paint_list = pl = pl-> next;

	    /* if ( alive_check) */
	    e. cmd = cmPaint;
	    CComponent( self)-> message( self, &e);
	    free( cur);
	 }
      }
      kill_zombies();
   }
   if ( application) Object_destroy( application);
   application = nilHandle;
   return true;
}

Bool
apc_application_lock( Handle self)
{
   DOLBUG( "apc_application_lock()\n");
   /*NYI*/
   return true;
}

Bool
apc_application_unlock( Handle self)
{
   DOLBUG( "apc_application_unlock()\n");
   /*NYI*/
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
/*NCI*/
/* Timer support */
/* It should be more like go() */
   XEvent ev, next_event;
   int n;

   XFlush( DISP);
   if (( n = XEventsQueued( DISP, QueuedAfterFlush)) <= 0) {
      /* just like tcl/perl tk do, to avoid an infinite loop */
      sig_t oldHandler = signal( SIGPIPE, SIG_IGN);
      XNoOp( DISP);
      XFlush( DISP);
      (void) signal( SIGPIPE, oldHandler);
   }
   if (n && application) {
      XNextEvent( DISP, &ev);
      XCHECKPOINT;
      n--;
      while ( n > 0) {
	 if (!application) return true;
	 XNextEvent( DISP, &next_event);
	 XCHECKPOINT;
	 guts. total_events++;
	 prima_handle_event( &ev, &next_event);
	 n--;
	 memcpy( &ev, &next_event, sizeof( XEvent));
      }
      if (!application) return true;
      guts. total_events++;
      prima_handle_event( &ev, nil);
   }
   XNoOp( DISP);
   XFlush( DISP);
   return true;
}
