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
/*  System dependent miscellaneous routines (unix, x11)    */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

/* Miscellaneous system-dependent functions */

Bool
log_write( const char *format, ...)
{
   return false;
}

int
debug_write( const char *format, ...)
{
    int rc = 0;
    if ( guts.dolbug) {
	va_list args;
	va_start( args, format);
	rc = vfprintf( stderr, format, args);
	va_end( args);
    }
    return rc;
}


int
unix_rm_get_int( Handle self, XrmQuark class_detail, XrmQuark name_detail, int default_value)
{
   DEFXX;
   XrmRepresentation type;
   XrmValue value;
   long int r;
   char *end;

   if ( XX && guts.db && XX-> qClassName && XX-> qInstanceName) {
      XX-> qClassName[XX-> nClassName] = class_detail;
      XX-> qClassName[XX-> nClassName + 1] = 0;
      XX-> qInstanceName[XX-> nInstanceName] = name_detail;
      XX-> qInstanceName[XX-> nInstanceName + 1] = 0;
      if ( XrmQGetResource( guts.db,
			    XX-> qInstanceName,
			    XX-> qClassName,
			    &type, &value)) {
	 if ( type == guts.qString) {
	    r = strtol((char *)value. addr, &end, 0);
	    if (*(value. addr) && !*end)
	       return (int)r;
	 }
      }
   }
   return default_value;
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

void
apc_component_fullname_changed_notify( Handle self)
{
   PComponent me = PComponent( self);
   static char convert[256];
   static Bool converted = false;
   XrmQuark qClass, qInstance;
   int i, l, n;
   char *c, *s;
   DEFXX;
   PDrawableSysData UU;
   Handle *list;

   if (!XX)
      return;

   if ( !converted) {
      for ( l = 0; l < 256; l++) {
	 convert[l] = isalnum(l) ? l : '_';
      }
      convert[0] = 0;
      converted = true;
   }

   l = strlen( s = ( self == application ? "Prima" : me-> self-> className));
   c = malloc( l+1);
   if (!c) croak( "apc_component_fullname_changed_notify: not enough memory");
   while ( l >= 0) {
      c[l] = convert[(unsigned)(unsigned char)s[l]];
      l--;
   }
   c[0] = toupper(c[0]);
   qClass = XrmStringToQuark(c);
   free( c);

   l = strlen( s = (me-> name ? me-> name : "noname"));
   c = malloc( l+1);
   if (!c) croak( "apc_component_fullname_changed_notify: not enough memory");
   while ( l >= 0) {
      c[l] = convert[(unsigned)(unsigned char)s[l]];
      l--;
   }
   c[0] = tolower(c[0]);
   qInstance = XrmStringToQuark(c);
   free(c);

   free( XX-> qClassName); XX-> qClassName = nil;
   free( XX-> qInstanceName); XX-> qInstanceName = nil;

   if ( me-> owner && me-> owner != self && PComponent(me-> owner)-> sysData && X(PComponent( me-> owner))-> qClassName) {
      UU = X(PComponent( me-> owner));
      XX-> nClassName = n = UU-> nClassName + 1;
      XX-> qClassName = malloc( sizeof( XrmQuark) * (n + 2));
      memcpy( XX-> qClassName, UU-> qClassName, sizeof( XrmQuark) * n);
      XX-> qClassName[n-1] = qClass;
      XX-> nInstanceName = n = UU-> nInstanceName + 1;
      XX-> qInstanceName = malloc( sizeof( XrmQuark) * (n + 2));
      memcpy( XX-> qInstanceName, UU-> qInstanceName, sizeof( XrmQuark) * n);
      XX-> qInstanceName[n-1] = qInstance;
   } else {
      XX-> nClassName = n = 1;
      XX-> qClassName = malloc( sizeof( XrmQuark) * (n + 2));
      XX-> qClassName[n-1] = qClass;
      XX-> nInstanceName = n = 1;
      XX-> qInstanceName = malloc( sizeof( XrmQuark) * (n + 2));
      XX-> qInstanceName[n-1] = qInstance;
   }

   if ( PComponent(self)-> components && (n = PComponent(self)-> components-> count) > 0) {
      list = malloc( sizeof( Handle) * n);
      memcpy( list, PComponent(self)-> components-> items, sizeof( Handle) * n);
      
      for ( i = 0; i < n; i++) {
	 apc_component_fullname_changed_notify( list[i]);
      }
      free( list);
   }

/*    PComponent her = me; */
/*    char *class, *instance, *c, *s, ch; */
/*    int classLen, instanceLen, classSize, instanceSize, l; */

/*    if ( !me-> sysData) */
/*       return; */

/*    class = malloc( classSize = 2048); classLen = 0; */
/*    instance = malloc( instanceSize = 2048); instanceLen = 0; */

/*    for (;;) { */
/*       if ( her == PComponent( application)) { */
/* 	 c = "Prima"; */
/*       } else { */
/* 	 c = her-> self-> className; */
/*       } */
/*       l = strlen( c); */
/*       if ( l + classLen + 1 >= classSize) { */
/* 	 class = reallocf( class, classSize+=(l>=2047?l+2:2048)); */
/* 	 if (!class) croak( "apc_component_fullname_changed_notify: not enough memory"); */
/*       } */
/*       while ( l) { */
/* 	 class[classLen++] = convert[(unsigned)(unsigned char)c[--l]]; */
/*       } */
/*       class[classLen-1] = toupper(class[classLen-1]); */
/*       class[classLen++] = '.'; */

/*       if ( her-> name) { */
/* 	 c = her-> name; */
/*       } else { */
/* 	 c = "noname"; */
/*       } */
/*       l = strlen( c); */
/*       if ( l + instanceLen + 1 >= instanceSize) { */
/* 	 instance = reallocf( instance, instanceSize+=(l>=2047?l+2:2048)); */
/* 	 if (!instance) croak( "apc_component_fullname_changed_notify: not enough memory"); */
/*       } */
/*       while ( l) { */
/* 	 instance[instanceLen++] = convert[(unsigned)(unsigned char)c[--l]]; */
/*       } */
/*       instance[instanceLen-1] = tolower(instance[instanceLen-1]); */
/*       instance[instanceLen++] = '.'; */

/*       if ( !her-> owner || PComponent( her-> owner) == her) */
/* 	 break; */
/*       her = PComponent( her-> owner); */
/*    } */
/*    class[--classLen] = 0; */
/*    c = class; s = class+classLen-1; while ( c < s) { ch = *c; *c++ = *s; *s-- = ch; } */
/*    instance[--instanceLen] = 0; */
/*    c = instance; s = instance+instanceLen-1; while ( c < s) { ch = *c; *c++ = *s; *s-- = ch; } */
/*    fprintf( stderr, "=============== Class: %s\n", class); */
/*    fprintf( stderr, "=============== Instance: %s\n", instance); */
/*    free( class); */
/*    free( instance); */
}

/* View attributes */
void
apc_cursor_set_pos( Handle self, int x, int y)
{
   DOLBUG( "apc_cursor_set_pos()\n");
}

void
apc_cursor_set_size( Handle self, int x, int y)
{
   DOLBUG( "apc_cursor_set_size()\n");
}

void
apc_cursor_set_visible( Handle self, Bool visible)
{
   DOLBUG( "apc_cursor_set_visible()\n");
}

Point
apc_cursor_get_pos( Handle self)
{
   DOLBUG( "apc_cursor_get_pos()\n");
   return (Point){0,0};
}

Point
apc_cursor_get_size( Handle self)
{
   DOLBUG( "apc_cursor_get_size()\n");
   return (Point){0,0};
}

Bool
apc_cursor_get_visible( Handle self)
{
   DOLBUG( "apc_cursor_get_visible()\n");
   return false;
}

Point
apc_pointer_get_hot_spot( Handle self)
{
   DOLBUG( "apc_pointer_get_hot_spot()\n");
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
   DOLBUG( "apc_pointer_get_shape()\n");
   return 0;
}

Point
apc_pointer_get_size( Handle self)
{
   DOLBUG( "apc_pointer_get_size()\n");
   return (Point){0,0};
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
   DOLBUG( "apc_pointer_get_bitmap()\n");
   return false;
}

Bool
apc_pointer_get_visible( Handle self)
{
   DOLBUG( "apc_pointer_get_visible()\n");
   return false;
}

void
apc_pointer_set_pos( Handle self, int x, int y)
{
   DOLBUG( "apc_pointer_set_pos()\n");
}

void
apc_pointer_set_shape( Handle self, int sysPtrId)
{
   DOLBUG( "apc_pointer_set_shape()\n");
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
   DOLBUG( "apc_pointer_set_user()\n");
   return false;
}

void
apc_pointer_set_visible( Handle self, Bool visible)
{
   DOLBUG( "apc_pointer_set_visible()\n");
}

int
apc_pointer_get_state( Handle self)
{
   DOLBUG( "apc_pointer_get_state()\n");
   return 0;
}

int
apc_kbd_get_state( Handle self)
{
   DOLBUG( "apc_kbd_get_state()\n");
   return 0;
}


/* Clipboard */

Bool
apc_clipboard_create( void)
{
   DOLBUG( "apc_clipboard_create()\n");
   return true;
}

void
apc_clipboard_destroy( void)
{
   DOLBUG( "apc_clipboard_destroy()\n");
}

Bool
apc_clipboard_open( void)
{
   DOLBUG( "apc_clipboard_open()\n");
   return false;
}

void
apc_clipboard_close( void)
{
   DOLBUG( "apc_clipboard_close()\n");
}

void
apc_clipboard_clear( void)
{
   DOLBUG( "apc_clipboard_clear()\n");
}

Bool
apc_clipboard_has_format( long id)
{
   DOLBUG( "apc_clipboard_has_format()\n");
   return false;
}

void *
apc_clipboard_get_data( long id, int *length)
{
   DOLBUG( "apc_clipboard_get_data()\n");
   return nil;
}

Bool
apc_clipboard_set_data( long id, void * data, int length)
{
   DOLBUG( "apc_clipboard_set_data()\n");
   return false;
}

long
apc_clipboard_register_format( char * format)
{
   DOLBUG( "apc_clipboard_register_format()\n");
   return 0;
}

void
apc_clipboard_deregister_format( long id)
{
   DOLBUG( "apc_clipboard_deregister_format()\n");
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
   apc_component_fullname_changed_notify( self);

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
   DOLBUG( "apc_help_open_topic()\n");
   return false;
}

void
apc_help_close( Handle self)
{
   DOLBUG( "apc_help_close()\n");
}

void
apc_help_set_file( Handle self, char * helpFile)
{
   DOLBUG( "apc_help_set_file()\n");
}


/* Messages */

void
apc_message( Handle self, PEvent ev, Bool post)
{
   DOLBUG( "apc_message()\n");
}

void
apc_show_message( char * message)
{
   DOLBUG( "apc_show_message()\n");
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
   DOLBUG( "apc_sys_get_msg_font()\n");
   return nil;
}

PFont
apc_sys_get_caption_font( PFont copyTo)
{
   DOLBUG( "apc_sys_get_caption_font()\n");
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
   case svWheelPresent: return guts.mouse_wheel_up || guts.mouse_wheel_down;
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
   DOLBUG( "apc_beep_tone()\n");
}

char *
apc_system_action( char * params)
{
   DOLBUG( "apc_system_action()\n");
   return nil;
}

void
apc_query_drives_map( char *firstDrive, char *result)
{
   DOLBUG( "apc_query_drives_map()\n");
}

int
apc_query_drive_type( char *drive)
{
   DOLBUG( "apc_query_drive_type()\n");
   return 0;
}

char *
apc_get_user_name( void)
{
   DOLBUG( "apc_get_user_name()\n");
   return nil;
}

void *
apc_dlopen(char *path, int mode)
{
   DOLBUG( "apc_dlopen()\n");
   return nil;
}
