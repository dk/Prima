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

   if ( XX && guts.db && XX-> q_class_name && XX-> q_instance_name) {
      XX-> q_class_name[XX-> n_class_name] = class_detail;
      XX-> q_class_name[XX-> n_class_name + 1] = 0;
      XX-> q_instance_name[XX-> n_instance_name] = name_detail;
      XX-> q_instance_name[XX-> n_instance_name + 1] = 0;
      if ( XrmQGetResource( guts.db,
			    XX-> q_instance_name,
			    XX-> q_class_name,
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
      bzero( PComponent( self)-> sysData, sizeof( UnixSysData));
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

   free( XX-> q_class_name); XX-> q_class_name = nil;
   free( XX-> q_instance_name); XX-> q_instance_name = nil;

   if ( me-> owner && me-> owner != self && PComponent(me-> owner)-> sysData && X(PComponent( me-> owner))-> q_class_name) {
      UU = X(PComponent( me-> owner));
      XX-> n_class_name = n = UU-> n_class_name + 1;
      XX-> q_class_name = malloc( sizeof( XrmQuark) * (n + 2));
      memcpy( XX-> q_class_name, UU-> q_class_name, sizeof( XrmQuark) * n);
      XX-> q_class_name[n-1] = qClass;
      XX-> n_instance_name = n = UU-> n_instance_name + 1;
      XX-> q_instance_name = malloc( sizeof( XrmQuark) * (n + 2));
      memcpy( XX-> q_instance_name, UU-> q_instance_name, sizeof( XrmQuark) * n);
      XX-> q_instance_name[n-1] = qInstance;
   } else {
      XX-> n_class_name = n = 1;
      XX-> q_class_name = malloc( sizeof( XrmQuark) * (n + 2));
      XX-> q_class_name[n-1] = qClass;
      XX-> n_instance_name = n = 1;
      XX-> q_instance_name = malloc( sizeof( XrmQuark) * (n + 2));
      XX-> q_instance_name[n-1] = qInstance;
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

/* Cursor support */

static XGCValues cursor_gcv = {
   line_width: 0,
   background: 0,
   foreground: 0,
   clip_mask: None,
   function: GXcopy
};

void
prima_no_cursor( Handle self)
{
   if ( guts.focused == self
	&& X(self)-> flags. cursor_visible
	&& guts. cursor_save)
   {
      DEFXX;
      int x, y, w, h;
      
      h = XX-> cursor_size. y;
      y = XX-> size. y - (h + XX-> cursor_pos. y);
      x = XX-> cursor_pos. x;
      w = XX-> cursor_size. x;

      prima_get_gc( XX);
      XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &cursor_gcv);
      XCHECKPOINT;
      XCopyArea( DISP, guts. cursor_save, XX-> drawable, XX-> gc,
		 0, 0, w, h, x, y);
      XCHECKPOINT;
      prima_release_gc( XX);
      guts. cursor_shown = false;
   }
}

void
prima_update_cursor( Handle self)
{
   if ( guts.focused == self) {
      DEFXX;
      int x, y, w, h;
      
      h = XX-> cursor_size. y;
      y = XX-> size. y - (h + XX-> cursor_pos. y);
      x = XX-> cursor_pos. x;
      w = XX-> cursor_size. x;
      
      if ( !guts. cursor_save || !guts. cursor_xor
	   || w > guts. cursor_pixmap_size. x
	   || h > guts. cursor_pixmap_size. y)
      {
	 if ( !guts. cursor_save) {
	    cursor_gcv. background = BlackPixel( DISP, SCREEN);
	    cursor_gcv. foreground = WhitePixel( DISP, SCREEN);
	 }
	 if ( guts. cursor_save) {
	    XFreePixmap( DISP, guts. cursor_save);
	    guts. cursor_save = 0;
	 }
	 if ( guts. cursor_xor) {
	    XFreePixmap( DISP, guts. cursor_xor);
	    guts. cursor_xor = 0;
	 }
	 if ( guts. cursor_pixmap_size. x < w)
	    guts. cursor_pixmap_size. x = w;
	 if ( guts. cursor_pixmap_size. y < h)
	    guts. cursor_pixmap_size. y = h;
	 if ( guts. cursor_pixmap_size. x < 16)
	    guts. cursor_pixmap_size. x = 16;
	 if ( guts. cursor_pixmap_size. y < 64)
	    guts. cursor_pixmap_size. y = 64;
	 guts. cursor_save = XCreatePixmap( DISP, XX-> drawable,
					    guts. cursor_pixmap_size. x,
					    guts. cursor_pixmap_size. y,
					    guts. depth);
	 guts. cursor_xor  = XCreatePixmap( DISP, XX-> drawable,
					    guts. cursor_pixmap_size. x,
					    guts. cursor_pixmap_size. y,
					    guts. depth);
      }

      prima_get_gc( XX);
      XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &cursor_gcv);
      XCHECKPOINT;
      XCopyArea( DISP, XX-> drawable, guts. cursor_save, XX-> gc,
		 x, y, w, h, 0, 0);
      XCHECKPOINT;
      XCopyArea( DISP, guts. cursor_save, guts. cursor_xor, XX-> gc,
		 0, 0, w, h, 0, 0);
      XCHECKPOINT;
      XSetFunction( DISP, XX-> gc, GXxor);
      XCHECKPOINT;
      XFillRectangle( DISP, guts. cursor_xor, XX-> gc, 0, 0, w, h);
      XCHECKPOINT;
      prima_release_gc( XX);
      
      if (!guts. cursor_timer) {
	 guts. cursor_timer = malloc( sizeof( TimerSysData));
	 bzero( guts. cursor_timer, sizeof( TimerSysData));
	 apc_timer_create( CURSOR_TIMER, nilHandle, 2);
      }
      if ( XX-> flags. cursor_visible) {
	 guts. cursor_shown = false;
	 prima_cursor_tick();
      } else {
	 apc_timer_stop( CURSOR_TIMER);
      }
   }
}

void
prima_cursor_tick( void)
{
   if ( guts. focused && X(guts. focused)-> flags. cursor_visible) {
      PDrawableSysData selfxx = X(guts. focused);
      Pixmap pixmap;
      int x, y, w, h;

      if ( guts. cursor_shown) {
	 guts. cursor_shown = false;
	 apc_timer_set_timeout( CURSOR_TIMER, guts. invisible_timeout);
	 pixmap = guts. cursor_save;
      } else {
	 guts. cursor_shown = true;
	 apc_timer_set_timeout( CURSOR_TIMER, guts. visible_timeout);
	 pixmap = guts. cursor_xor;
      }

      h = XX-> cursor_size. y;
      y = XX-> size. y - (h + XX-> cursor_pos. y);
      x = XX-> cursor_pos. x;
      w = XX-> cursor_size. x;

      prima_get_gc( XX);
      XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &cursor_gcv);
      XCHECKPOINT;
      XCopyArea( DISP, pixmap, XX-> drawable, XX-> gc, 0, 0, w, h, x, y);
      XCHECKPOINT;
      prima_release_gc( XX);
      XFlush( DISP);
      XCHECKPOINT;
   } else {
      apc_timer_stop( CURSOR_TIMER);
      guts. cursor_shown = !guts. cursor_shown;
   }
}

void
apc_cursor_set_pos( Handle self, int x, int y)
{
   DEFXX;
   prima_no_cursor( self);
   XX-> cursor_pos. x = x;
   XX-> cursor_pos. y = y;
   prima_update_cursor( self);
}

void
apc_cursor_set_size( Handle self, int x, int y)
{
   DEFXX;
   prima_no_cursor( self);
   XX-> cursor_size. x = x;
   XX-> cursor_size. y = y;
   prima_update_cursor( self);
}

void
apc_cursor_set_visible( Handle self, Bool visible)
{
   DEFXX;
   if ( XX-> flags. cursor_visible != visible) {
      prima_no_cursor( self);
      XX-> flags. cursor_visible = visible;
      prima_update_cursor( self);
   }
}

Point
apc_cursor_get_pos( Handle self)
{
   return X(self)-> cursor_pos;
}

Point
apc_cursor_get_size( Handle self)
{
   return X(self)-> cursor_size;
}

Bool
apc_cursor_get_visible( Handle self)
{
   return X(self)-> flags. cursor_visible;
}

/* View attributes */
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
apc_clipboard_register_format( const char* format)
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

static void
fetch_sys_timer( Handle self, PTimerSysData *s, Bool *real_timer)
{
   if ( self == 0) {
      *s = nil;
      *real_timer = false;
   } else if ( self == CURSOR_TIMER) {
      *s = guts. cursor_timer;
      *real_timer = false;
   } else {
      *s = ((PTimerSysData)(PComponent((self))-> sysData));
      *real_timer = true;
   }
}

#define ENTERTIMER \
	PTimerSysData sys; \
	Bool real; \
	\
	fetch_sys_timer( self, &sys, &real)

Bool
apc_timer_create( Handle self, Handle owner, int timeout)
{
   ENTERTIMER;

   inactivate_timer( sys);
   sys-> timeout = timeout;
   sys-> who = self;
   if (real) {
      opt_clear( optActive);
      apc_component_fullname_changed_notify( self);
   }
   return true;
}

void
apc_timer_destroy( Handle self)
{
   ENTERTIMER;

   inactivate_timer( sys);
   sys-> timeout = 0;
   if (real) opt_clear( optActive);
}

int
apc_timer_get_timeout( Handle self)
{
   ENTERTIMER;
   return sys-> timeout;
}

void
apc_timer_set_timeout( Handle self, int timeout)
{
   ENTERTIMER;

   sys-> timeout = timeout;
   if ( !real || is_opt( optActive))
      apc_timer_start( self);
}

Bool
apc_timer_start( Handle self)
{
   PTimerSysData before;
   ENTERTIMER;

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

   if ( real) opt_set( optActive);
   return true;
}

void
apc_timer_stop( Handle self)
{
   ENTERTIMER;

   inactivate_timer( sys);
   if ( real) opt_clear( optActive);
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
apc_help_set_file( Handle self, const char* helpFile)
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
apc_show_message( const char * message)
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
   case svXCursor: return 1;
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
apc_system_action( const char* params)
{
   DOLBUG( "apc_system_action()\n");
   return nil;
}

void
apc_query_drives_map( const char* firstDrive, char *result, int len)
{
   if ( !result || len <= 0) return;
   *result = 0;
}

int
apc_query_drive_type( const char *drive)
{
   return dtNone;
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

PList
apc_getdir( const char *dirname)
{
   DIR *dh;
   struct dirent *de;
   PList dirlist = nil;
   char *type;

   if (( dh = opendir( dirname)) && (dirlist = plist_create( 50, 50))) {
      while (( de = readdir( dh))) {
	 list_add( dirlist, (Handle)duplicate_string( de-> d_name));
	 switch ( de-> d_type) {
	 case DT_FIFO:	type = "fifo";	break;
	 case DT_CHR:	type = "chr";	break;
	 case DT_DIR:	type = "dir";	break;
	 case DT_BLK:	type = "blk";	break;
	 case DT_REG:	type = "reg";	break;
	 case DT_LNK:	type = "lnk";	break;
	 case DT_SOCK:	type = "sock";	break;
#ifdef DT_WHT
	 case DT_WHT:	type = "wht";	break;
#endif
	 default:	type = "unknown";
	 }
	 list_add( dirlist, (Handle)duplicate_string( type));
      }
      closedir( dh);
   }
   return dirlist;
}

void
prima_rect_union( XRectangle *t, const XRectangle *s)
{
   XRectangle r;
   
   if ( t-> x < s-> x) r. x = t-> x; else r. x = s-> x;
   if ( t-> y < s-> y) r. y = t-> y; else r. y = s-> y;
   if ( t-> x + t-> width > s-> x + s-> width)
      r. width = t-> x + t-> width - r. x;
   else
      r. width = s-> x + s-> width - r. x;
   if ( t-> y + t-> height > s-> y + s-> height)
      r. height = t-> y + t-> height - r. y;
   else
      r. height = s-> y + s-> height - r. y;
   *t = r;
}

void
prima_rect_intersect( XRectangle *t, const XRectangle *s)
{
   XRectangle r;
   int w, h;
   
   if ( t-> x > s-> x) r. x = t-> x; else r. x = s-> x;
   if ( t-> y > s-> y) r. y = t-> y; else r. y = s-> y;
   if ( t-> x + t-> width < s-> x + s-> width)
      w = t-> x + (int)t-> width - r. x;
   else
      w = s-> x + (int)s-> width - r. x;
   if ( t-> y + t-> height < s-> y + s-> height)
      h = t-> y + (int)t-> height - r. y;
   else
      h = s-> y + (int)s-> height - r. y;
   if ( w < 0 || h < 0) {
      r. x = 0; r. y = 0; r. width = 0; r. height = 0;
   } else {
      r. width = w; r. height = h;
   }
   *t = r;
}

