#include "unix/guts.h"
#include "Application.h"
#include <sys/utsname.h>

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  System dependent application management (unix, x11)    */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

static int
x_error_handler( Display *d, XErrorEvent *ev)
{
   int tail = guts. riTail;
   int prev = tail;
   char buf[ 1024];

   while ( tail != guts. riHead) {
      if ( guts. ri[ tail]. request > ev-> serial)
	 break;
      prev = tail;
      tail++;
      if ( tail >= REQUEST_RING_SIZE)
	 tail = 0;
   }

   XGetErrorText( d, ev-> error_code, buf, 1024);
   fprintf( stderr, "%s\n", buf);
   if ( tail == guts. riHead && prev == guts. riHead)
      fprintf( stderr, "This occured in unknown place\n");
   else if ( tail == guts. riHead)
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

Bool
window_subsystem_init( void)
{
/*XXX*/ /* Namely, support for -display host:0.0 etc. */
   guts. riHead = guts. riTail = 0;
   guts. dolbug = getenv( "PRIMA_DOLBUG") ? true : false;
   DISP = XOpenDisplay( nil);
   if (!DISP) return false;
   XSetErrorHandler( x_error_handler);
   (void)x_io_error_handler;
   XCHECKPOINT;

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
   prima_wm_init();
   prima_init_font_subsystem();
   prima_init_image_subsystem();
/*    XSynchronize( DISP, true); */
   return true;
}

void
window_subsystem_cleanup( void)
{
/*XXX*/
fprintf( stderr, "window_subsystem_cleanup()\n");
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
   printf( "Used GC's: %d\n", n);
   for ( n = 0, gcl = guts. free_gcl; gcl; n++, gcl = gcl-> next) {
   }
   printf( "Free GC's: %d\n", n);

   if (DISP) XCloseDisplay( DISP);
   DISP = nil;

   printf( "Total X events: %ld\n", guts. total_events);
   printf( "Handled X events: %ld\n", guts. handled_events);
   printf( "Unhandled X events: %ld\n", guts. unhandled_events);
   printf( "Skipped X events: %ld\n", guts. skipped_events);

   if (guts.windows) hash_destroy( guts.windows, false);
   prima_cleanup_font_subsystem();
}

Bool
apc_application_begin_paint( Handle self)
{
fprintf( stderr, "apc_application_begin_paint()\n");
/*NYI*/
   return true;
}

Bool
apc_application_begin_paint_info( Handle self)
{
fprintf( stderr, "apc_application_begin_paint_info()\n");
/*NYI*/
   return true;
}

Bool
apc_application_create( Handle self)
{
   XSetWindowAttributes attrs;
   DEFXX;

fprintf( stderr, "apc_application_create()\n");

   attrs. event_mask = StructureNotifyMask;
   X_WINDOW = XCreateWindow( DISP, RootWindow( DISP, SCREEN),
			     0, 0, 1, 1, 0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     CWEventMask, &attrs);
   XCHECKPOINT;
fprintf( stderr, "apc_application_create(), window: %08lx\n", X_WINDOW);
   if (!X_WINDOW) return false;
   hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);

   XX-> drawable = RootWindow( DISP, SCREEN);
   XX-> parent = None;
   XX-> origin = (Point){0,0};
   XX-> size = apc_application_get_size( self);
   XX-> owner = nilHandle;

   XX-> flags. clipOwner = 1;
   XX-> flags. syncPaint = 0;

   return true;
}

void
apc_application_close( Handle self)

{
fprintf( stderr, "apc_application_close()\n");
   Object_destroy( self);
}

void
apc_application_destroy( Handle self)
{
fprintf( stderr, "apc_application_destroy()\n");
   if ( X_WINDOW) {
      XDestroyWindow( DISP, X_WINDOW);
      XCHECKPOINT;
      hash_delete( guts.windows, (void*)&X_WINDOW, sizeof(X_WINDOW), false);
   }
}

void
apc_application_end_paint( Handle self)
{
fprintf( stderr, "apc_application_end_paint()\n");
/*NYI*/
}

void
apc_application_end_paint_info( Handle self)
{
fprintf( stderr, "apc_application_end_paint_info()\n");
}

int
apc_application_get_gui_info( char * description)
{
   if ( description)
      strcpy( description, "X Window System");
   return guiXLib;
}

Handle
apc_application_get_view_from_point( Handle self, Point point)
{
fprintf( stderr, "apc_application_get_view_from_point()\n");
/*NYI*/
   return nilHandle;
}

Handle
apc_application_get_handle( Handle self, ApiHandle apiHandle)
{
fprintf( stderr, "apc_application_get_handle()\n");
/*NYI*/
   return nilHandle;
}

int
apc_application_get_os_info( char * system,
			     char * release,
			     char * vendor,
			     char * arch)
{
   static struct utsname name;
   static Bool fetched = false;

   if (!fetched) {
      if ( uname(&name)!=0) {
	 strcpy( name. sysname, "Some UNIX");
	 strcpy( name. release, "Unknown version of UNIX");
	 strcpy( name. machine, "Unknown architecture");
      }
      fetched = true;
   }

   if (system) strcpy( system, name. sysname);
   if (release) strcpy( release, name. release);
   if (vendor) strcpy( vendor, "Unknown vendor");
   if (arch) strcpy( arch, name. machine);

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

void
apc_application_go( Handle self)
{
/*NCI*/
   XEvent ev, next_event;
   fd_set read_set;
   int connection = ConnectionNumber( DISP);
   struct timeval timeout;
   int r, n;

   XNoOp( DISP);
   XFlush( DISP);
//   XCHECKPOINT;

   for (;;) {
      if (!application) return;
      if (( n = XEventsQueued( DISP, QueuedAlready))) {
	 goto FetchAndProcess;
      }
      FD_ZERO( &read_set);
      FD_SET( connection, &read_set);
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
	    CComponent( timer-> who)-> message( timer-> who, &e);
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

      if (( r = select( connection+1, &read_set, nil, nil, &timeout)) > 0 &&
	  FD_ISSET( connection, &read_set)) {
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
	       if (!application) return;
	       XNextEvent( DISP, &next_event);
	       XCHECKPOINT;
	       guts. total_events++;
	       handle_event( &ev, &next_event);
	       n--;
	       memcpy( &ev, &next_event, sizeof( XEvent));
	    }
	    if (!application) return;
	    guts. total_events++;
	    handle_event( &ev, nil);
	 }
 	 XNoOp( DISP);
 	 XFlush( DISP);
      } else if ( r < 0) {
	 fprintf( stderr, "select() error: %d\n", errno);
      } else {
/*    	 printf( "timeout loop\n"); */
 	 XNoOp( DISP);
 	 XFlush( DISP);
      }
      kill_zombies();
   }
   if ( application) Object_destroy( application);
   application = nilHandle;
fprintf( stderr, "-- apc_application_go()\n");
}

void
apc_application_lock( Handle self)
{
fprintf( stderr, "apc_application_lock()\n");
/*NYI*/
}

void
apc_application_unlock( Handle self)
{
fprintf( stderr, "apc_application_unlock()\n");
/*NYI*/
}

static XBool
any_event( Display *d, XEvent *ev, XPointer arg)
{
   (void)d; (void)ev; (void)arg; (void)any_event;
   return true;
}

void
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
	 if (!application) return;
	 XNextEvent( DISP, &next_event);
	 XCHECKPOINT;
	 guts. total_events++;
	 handle_event( &ev, &next_event);
	 n--;
	 memcpy( &ev, &next_event, sizeof( XEvent));
      }
      if (!application) return;
      guts. total_events++;
      handle_event( &ev, nil);
   }
   XNoOp( DISP);
   XFlush( DISP);
}
