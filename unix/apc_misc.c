#include "unix/guts.h"

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  System dependent miscellaneous routines (unix, x11)    */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

/* Miscellaneous system-dependent functions */

Bool
log_write( const char *format, ...)
{
   return false;
}

Bool
debug_write( const char *format, ...)
{
   return false;
}


/* Component-related functions */

void
apc_component_create( Handle self)
{
   if ( !PComponent( self)-> sysData) {
      PComponent( self)-> sysData = malloc( sizeof( UnixSysData));
      memset( PComponent( self)-> sysData, 0, sizeof( UnixSysData));
   }
}

void
apc_component_destroy( Handle self)
{
   free( PComponent( self)-> sysData);
   PComponent( self)-> sysData = nil;
   X_WINDOW = nilHandle;
}


/* View attributes */
void
apc_cursor_set_pos( Handle self, int x, int y)
{
fprintf( stderr, "apc_cursor_set_pos()\n");
}

void
apc_cursor_set_size( Handle self, int x, int y)
{
fprintf( stderr, "apc_cursor_set_size()\n");
}

void
apc_cursor_set_visible( Handle self, Bool visible)
{
fprintf( stderr, "apc_cursor_set_visible()\n");
}

Point
apc_cursor_get_pos( Handle self)
{
fprintf( stderr, "apc_cursor_get_pos()\n");
   return (Point){0,0};
}

Point
apc_cursor_get_size( Handle self)
{
fprintf( stderr, "apc_cursor_get_size()\n");
   return (Point){0,0};
}

Bool
apc_cursor_get_visible( Handle self)
{
fprintf( stderr, "apc_cursor_get_visible()\n");
   return false;
}

Point
apc_pointer_get_hot_spot( Handle self)
{
fprintf( stderr, "apc_pointer_get_hot_spot()\n");
   return (Point){0,0};
}

Point
apc_pointer_get_pos( Handle self)
{
   Point p;
   XWindow root, child;
   int x, y;
   unsigned int mask;

   if ( !XQueryPointer( DISP, RootWindow( DISP, SCREEN),
			&root, &child, &p. x, &p. y,
			&x, &y, &mask)) {
      croak( "apc_pointer_get_pos(): XQueryPointer() failed");
   }
   p. y = DisplayHeight( DISP, SCREEN) - p. y - 1;
   return p;
}

int
apc_pointer_get_shape( Handle self)
{
fprintf( stderr, "apc_pointer_get_shape()\n");
   return 0;
}

Point
apc_pointer_get_size( Handle self)
{
fprintf( stderr, "apc_pointer_get_size()\n");
   return (Point){0,0};
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
fprintf( stderr, "apc_pointer_get_bitmap()\n");
   return false;
}

Bool
apc_pointer_get_visible( Handle self)
{
fprintf( stderr, "apc_pointer_get_visible()\n");
   return false;
}

void
apc_pointer_set_pos( Handle self, int x, int y)
{
fprintf( stderr, "apc_pointer_set_pos()\n");
}

void
apc_pointer_set_shape( Handle self, int sysPtrId)
{
fprintf( stderr, "apc_pointer_set_shape()\n");
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
fprintf( stderr, "apc_pointer_set_user()\n");
   return false;
}

void
apc_pointer_set_visible( Handle self, Bool visible)
{
fprintf( stderr, "apc_pointer_set_visible()\n");
}

int
apc_pointer_get_state( Handle self)
{
fprintf( stderr, "apc_pointer_get_state()\n");
   return 0;
}

int
apc_kbd_get_state( Handle self)
{
fprintf( stderr, "apc_kbd_get_state()\n");
   return 0;
}


/* Clipboard */

Bool
apc_clipboard_create( void)
{
fprintf( stderr, "apc_clipboard_create()\n");
   return true;
}

void
apc_clipboard_destroy( void)
{
fprintf( stderr, "apc_clipboard_destroy()\n");
}

Bool
apc_clipboard_open( void)
{
fprintf( stderr, "apc_clipboard_open()\n");
   return false;
}

void
apc_clipboard_close( void)
{
fprintf( stderr, "apc_clipboard_close()\n");
}

void
apc_clipboard_clear( void)
{
fprintf( stderr, "apc_clipboard_clear()\n");
}

Bool
apc_clipboard_has_format( long id)
{
fprintf( stderr, "apc_clipboard_has_format()\n");
   return false;
}

void *
apc_clipboard_get_data( long id, int *length)
{
fprintf( stderr, "apc_clipboard_get_data()\n");
   return nil;
}

Bool
apc_clipboard_set_data( long id, void * data, int length)
{
fprintf( stderr, "apc_clipboard_set_data()\n");
   return false;
}

long
apc_clipboard_register_format( char * format)
{
fprintf( stderr, "apc_clipboard_register_format()\n");
   return 0;
}

void
apc_clipboard_deregister_format( long id)
{
fprintf( stderr, "apc_clipboard_deregister_format()\n");
}

/* Timer */

static void
inactivate_timer( PTimerSysData sys)
{
   if ( sys-> older || sys-> younger || guts. oldest == sys) {
      if ( sys-> older) {
	 sys-> older-> younger = sys-> younger;
      } else {
	 guts. oldest = sys-> younger;
      }
      if ( sys-> younger)
	 sys-> younger-> older = sys-> older;
   }
   sys-> older = nil;
   sys-> younger = nil;
}

Bool
apc_timer_create( Handle self, Handle owner, int timeout)
{
   PTimerSysData sys = ((PTimerSysData)(PComponent((self))-> sysData));

   inactivate_timer( sys);
   sys-> timeout = timeout;
   opt_clear( optActive);
   sys-> who = self;

   return true;
}

void
apc_timer_destroy( Handle self)
{
   PTimerSysData sys = ((PTimerSysData)(PComponent((self))-> sysData));

   inactivate_timer( sys);
   sys-> timeout = 0;
   opt_clear( optActive);
}

int
apc_timer_get_timeout( Handle self)
{
   return ((PTimerSysData)(PComponent((self))-> sysData))-> timeout;
}

void
apc_timer_set_timeout( Handle self, int timeout)
{
   PTimerSysData sys = ((PTimerSysData)(PComponent((self))-> sysData));

   sys-> timeout = timeout;
   if ( is_opt( optActive))
      apc_timer_start( self);
}

Bool
apc_timer_start( Handle self)
{
   PTimerSysData sys = ((PTimerSysData)(PComponent((self))-> sysData));
   PTimerSysData before;

   inactivate_timer( sys);
   if ( gettimeofday( &sys-> when, nil) != 0) {
      croak( "apc_timer_start() gettimeofday() returned: %s", strerror( errno));
   }
   sys-> when. tv_sec += sys-> timeout / 1000;
   sys-> when. tv_usec += (sys-> timeout % 1000) * 1000;

   before = guts. oldest;
   if ( before) {
      while ( before-> when. tv_sec < sys-> when. tv_sec ||
	      ( before-> when. tv_sec == sys-> when. tv_sec &&
		before-> when. tv_usec <= sys-> when. tv_usec)) {
	 if ( !before-> younger) {
	    before-> younger = sys;
	    sys-> older = before;
	    before = nil;
	    break;
	 }
	 before = before-> younger;
      }
      if ( before) {
	 if ( before-> older) {
	    sys-> older = before-> older;
	 } else {
	    guts. oldest = sys;
	 }
	 sys-> younger = before;
      }
   } else {
      guts. oldest = sys;
   }

   opt_set( optActive);
   return true;
}

void
apc_timer_stop( Handle self)
{
   PTimerSysData sys = ((PTimerSysData)(PComponent((self))-> sysData));

   inactivate_timer( sys);
   opt_clear( optActive);
}


/* Help */

Bool
apc_help_open_topic( Handle self, long command)
{
fprintf( stderr, "apc_help_open_topic()\n");
   return false;
}

void
apc_help_close( Handle self)
{
fprintf( stderr, "apc_help_close()\n");
}

void
apc_help_set_file( Handle self, char * helpFile)
{
fprintf( stderr, "apc_help_set_file()\n");
}


/* Messages */

void
apc_message( Handle self, PEvent ev, Bool post)
{
fprintf( stderr, "apc_message()\n");
}

void
apc_show_message( char * message)
{
fprintf( stderr, "apc_show_message()\n");
}

/* system metrics */

Bool
apc_sys_get_insert_mode( void)
{
   return guts. insert;
}

PFont
apc_sys_get_msg_font( PFont copyTo)
{
fprintf( stderr, "apc_sys_get_msg_font()\n");
   return nil;
}

PFont
apc_sys_get_caption_font( PFont copyTo)
{
fprintf( stderr, "apc_sys_get_caption_font()\n");
   return nil;
}

int
apc_sys_get_value( int v)  /* XXX one big XXX */
{
   switch ( v) {
   case svYMenu: /* XXX sensible menu height - query? */ return 20;
   case svYTitleBar: /* XXX */ return 20;
   case svMousePresent: return guts. mouse_buttons > 0;
   case svMouseButtons: return guts. mouse_buttons;
   case svSubmenuDelay:  /* XXX ? */ return 50;
   case svFullDrag: /* XXX ? */ return false;
   case svWheelPresent: return false;
   case svXIcon: /* XXX wm query */ return 64;
   case svYIcon: /* XXX wm query */ return 64;
   case svXSmallIcon: /* XXX wm query */ return 20;
   case svYSmallIcon:  /* XXX wm query */ return 20;
   case svXPointer: return guts. cursor_width;
   case svYPointer: return guts. cursor_height;
   case svXScrollbar: return 16;
   case svYScrollbar: return 16;
   case svXCursor: return 3;  /* XXX I don't know what it means */
   case svAutoScrollFirst: return 200;
   case svAutoScrollNext: return 50;
   case svXbsNone: return 0;
   case svYbsNone: return 0;
   case svXbsSizeable: return 3; /* XXX */
   case svYbsSizeable: return 3; /* XXX */
   case svXbsSingle: return 1; /* XXX */
   case svYbsSingle: return 1; /* XXX */
   case svXbsDialog: return 2; /* XXX */
   case svYbsDialog: return 2; /* XXX */
   default:
      warn( "apc_sys_get_value(): illegal query: %d", v);
   }
   return 0;
}

void
apc_sys_set_insert_mode( Bool insMode)
{
   guts. insert = !!insMode;
}

/* etc */

void
apc_beep( int style)
{
   /* XXX - mbError, mbQuestion, mbInformation, mbWarning */
   if ( DISP)
      XBell( DISP, 0);
}

void
apc_beep_tone( int freq, int duration)
{
fprintf( stderr, "apc_beep_tone()\n");
}

char *
apc_system_action( char * params)
{
fprintf( stderr, "apc_system_action()\n");
   return nil;
}

void
apc_query_drives_map( char *firstDrive, char *result)
{
fprintf( stderr, "apc_query_drives_map()\n");
}

int
apc_query_drive_type( char *drive)
{
fprintf( stderr, "apc_query_drive_type()\n");
   return 0;
}

char *
apc_get_user_name( void)
{
fprintf( stderr, "apc_get_user_name()\n");
   return nil;
}

void *
apc_dlopen(char *path, int mode)
{
fprintf( stderr, "apc_dlopen()\n");
   return nil;
}
