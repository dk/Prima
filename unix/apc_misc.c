/***********************************************************/
/*                                                         */
/*  System dependent miscellaneous routines (unix, x11)    */
/*                                                         */
/***********************************************************/

#include <apricot.h>
#include <sys/stat.h>
#include "unix/guts.h"
#include "Application.h"
#include "File.h"
#include "Icon.h"
#define XK_MISCELLANY
#include <X11/keysymdef.h>

/* Miscellaneous system-dependent functions */

#define X_COLOR_TO_RGB(xc)     (ARGB(((xc).red>>8),((xc).green>>8),((xc).blue>>8)))
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)

static XrmQuark
get_class_quark( const char *name)
{
	XrmQuark quark;
	char *s, *t;

	t = s = prima_normalize_resource_string( duplicate_string( name), true);
	if ( t && *t == 'P' && strncmp( t, "Prima__", 7) == 0)
		s = t + 7;
	if ( s && *s == 'A' && strcmp( s, "Application") == 0)
		strcpy( s, "Prima"); /* we have enough space */
	quark = XrmStringToQuark( s);
	free( t);
	return quark;
}

static XrmQuark
get_instance_quark( const char *name)
{
	XrmQuark quark;
	char *s;

	s = duplicate_string( name);
	quark = XrmStringToQuark( prima_normalize_resource_string( s, false));
	free( s);
	return quark;
}

static Bool
update_quarks_cache( Handle self)
{
	PComponent me = PComponent( self);
	XrmQuark qClass, qInstance;
	int n;
	DEFXX;
	PDrawableSysData UU;

	if (!XX)
		return false;

	qClass = get_class_quark( self == application ? "Prima" : me-> self-> className);
	qInstance = get_instance_quark( me-> name ? me-> name : "noname");

	free( XX-> q_class_name); XX-> q_class_name = nil;
	free( XX-> q_instance_name); XX-> q_instance_name = nil;

	if ( me-> owner && me-> owner != self && PComponent(me-> owner)-> sysData && X(PComponent( me-> owner))-> q_class_name) {
		UU = X(PComponent( me-> owner));
		XX-> n_class_name = n = UU-> n_class_name + 1;
		if (( XX-> q_class_name = malloc( sizeof( XrmQuark) * (n + 3))))
			memcpy( XX-> q_class_name, UU-> q_class_name, sizeof( XrmQuark) * n);
		XX-> q_class_name[n-1] = qClass;
		XX-> n_instance_name = n = UU-> n_instance_name + 1;
		if (( XX-> q_instance_name = malloc( sizeof( XrmQuark) * (n + 3))))
			memcpy( XX-> q_instance_name, UU-> q_instance_name, sizeof( XrmQuark) * n);
		XX-> q_instance_name[n-1] = qInstance;
	} else {
		XX-> n_class_name = n = 1;
		if (( XX-> q_class_name = malloc( sizeof( XrmQuark) * (n + 3))))
			XX-> q_class_name[n-1] = qClass;
		XX-> n_instance_name = n = 1;
		if (( XX-> q_instance_name = malloc( sizeof( XrmQuark) * (n + 3))))
			XX-> q_instance_name[n-1] = qInstance;
	}
	return true;
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

Bool
apc_fetch_resource( const char *className, const char *name,
						const char *resClass, const char *res,
						Handle owner, int resType,
						void *result)
{
	PDrawableSysData XX;
	XrmQuark *classes, *instances, backup_classes[3], backup_instances[3];
	XrmRepresentation type;
	XrmValue value;
	int nc, ni;
	char *s;
	XColor clr;

	if ( owner == nilHandle) {
		classes           = backup_classes;
		instances         = backup_instances;
		nc = ni = 0;
	} else {
		if (!update_quarks_cache( owner)) return false;
		XX                   = X(owner);
		if (!XX) return false;
		classes              = XX-> q_class_name;
		instances            = XX-> q_instance_name;
		if ( classes == nil || instances == nil) return false;
		nc                   = XX-> n_class_name;
		ni                   = XX-> n_instance_name;
	}
	classes[nc++]        = get_class_quark( className);
	instances[ni++]      = get_instance_quark( name);
	classes[nc++]        = get_class_quark( resClass);
	instances[ni++]      = get_instance_quark( res);
	classes[nc]          = 0;
	instances[ni]        = 0;

	if (guts. debug & DEBUG_XRDB) {
		int i;
		_debug( "misc: inst: ");
		for ( i = 0; i < ni; i++) {
			_debug( "%s ", XrmQuarkToString( instances[i]));
		}
		_debug( "\nmisc: class: ");
		for ( i = 0; i < nc; i++) {
			_debug( "%s ", XrmQuarkToString( classes[i]));
		}
		_debug( "\n");
	}

	if ( XrmQGetResource( guts.db,
				instances,
				classes,
				&type, &value)) {
		if ( type == guts.qString) {
			s = (char *)value.addr;
			Xdebug("found %s\n", s);
			switch ( resType) {
			case frString:
				*((char**)result) = duplicate_string( s);
				break;
			case frColor:
				if (!XParseColor( DISP, DefaultColormap( DISP, SCREEN), s, &clr))
					return false;
				*((Color*)result) = X_COLOR_TO_RGB(clr);
				Xdebug("color: %06x\n", *((Color*)result));
				break;
			case frFont:
				prima_font_pp2font( s, ( Font *) result);
#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding
				Xdebug("font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT((*(( Font *) result))));
				break;
			case frUnix_int:
				*((int*)result) = atoi( s);
				Xdebug("int: %d\n", *((int*)result));
				break;
			default:
				return false;
			}
			return true;
		}
	}

	return false;
}

Color
apc_lookup_color( const char * colorName)
{
	char buf[ 256];
	char *b;
	int len;
	XColor clr;

	if ( DISP && XParseColor( DISP, DefaultColormap( DISP, SCREEN), colorName, &clr))
		return X_COLOR_TO_RGB(clr);

#define xcmp( name, stlen, retval)  if (( len == stlen) && ( strcmp( name, buf) == 0)) return retval

	strncpy( buf, colorName, 256);
	len = strlen( buf);
	for ( b = buf; *b; b++) *b = tolower(*b);

	switch( buf[0]) {
	case 'a':
		xcmp( "aqua", 4, 0x00FFFF);
		xcmp( "azure", 5, ARGB(240,255,255));
		break;
	case 'b':
		xcmp( "black", 5, 0x000000);
		xcmp( "blanchedalmond", 14, ARGB( 255,235,205));
		xcmp( "blue", 4, 0x000080);
		xcmp( "brown", 5, 0x808000);
		xcmp( "beige", 5, ARGB(245,245,220));
		break;
	case 'c':
		xcmp( "cyan", 4, 0x008080);
		xcmp( "chocolate", 9, ARGB(210,105,30));
		break;
	case 'd':
		xcmp( "darkgray", 8, 0x404040);
		break;
	case 'e':
		break;
	case 'f':
		xcmp( "fuchsia", 7, 0xFF00FF);
		break;
	case 'g':
		xcmp( "green", 5, 0x008000);
		xcmp( "gray", 4, 0x808080);
		xcmp( "gray80", 6, ARGB(204,204,204));
		xcmp( "gold", 4, ARGB(255,215,0));
		break;
	case 'h':
		xcmp( "hotpink", 7, ARGB(255,105,180));
		break;
	case 'i':
		xcmp( "ivory", 5, ARGB(255,255,240));
		break;
	case 'j':
		break;
	case 'k':
		xcmp( "khaki", 5, ARGB(240,230,140));
		break;
	case 'l':
		xcmp( "lime", 4, 0x00FF00);
		xcmp( "lightgray", 9, 0xC0C0C0);
		xcmp( "lightblue", 9, 0x0000FF);
		xcmp( "lightgreen", 10, 0x00FF00);
		xcmp( "lightcyan", 9, 0x00FFFF);
		xcmp( "lightmagenta", 12, 0xFF00FF);
		xcmp( "lightred", 8, 0xFF0000);
		xcmp( "lemon", 5, ARGB(255,250,205));
		break;
	case 'm':
		xcmp( "maroon", 6, 0x800000);
		xcmp( "magenta", 7, 0x800080);
		break;
	case 'n':
		xcmp( "navy", 4, 0x000080);
		break;
	case 'o':
		xcmp( "olive", 5, 0x808000);
		xcmp( "orange", 6, ARGB(255,165,0));
		break;
	case 'p':
		xcmp( "purple", 6, 0x800080);
		xcmp( "peach", 5, ARGB(255,218,185));
		xcmp( "peru", 4, ARGB(205,133,63));
		xcmp( "pink", 4, ARGB(255,192,203));
		xcmp( "plum", 4, ARGB(221,160,221));
		break;
	case 'q':
		break;
	case 'r':
		xcmp( "red", 3, 0x800000);
		xcmp( "royalblue", 9, ARGB(65,105,225));
		break;
	case 's':
		xcmp( "silver", 6, 0xC0C0C0);
		xcmp( "sienna", 6, ARGB(160,82,45));
		break;
	case 't':
		xcmp( "teal", 4, 0x008080);
		xcmp( "turquoise", 9, ARGB(64,224,208));
		xcmp( "tan", 3, ARGB(210,180,140));
		xcmp( "tomato", 6, ARGB(255,99,71));
		break;
	case 'u':
		break;
	case 'w':
		xcmp( "white", 5, 0xFFFFFF);
		xcmp( "wheat", 5, ARGB(245,222,179));
		break;
	case 'v':
		xcmp( "violet", 6, ARGB(238,130,238));
		break;
	case 'x':
		break;
	case 'y':
		xcmp( "yellow", 6, 0xFFFF00);
		break;
	case 'z':
		break;
	}

#undef xcmp

	return clInvalid;
}

/* Component-related functions */

Bool
apc_component_create( Handle self)
{
	if ( !PComponent( self)-> sysData) {
		if ( !( PComponent( self)-> sysData = malloc( sizeof( UnixSysData))))
			return false;
		bzero( PComponent( self)-> sysData, sizeof( UnixSysData));
		((PUnixSysData)(PComponent(self)->sysData))->component. self = self;
	}
	return true;
}

Bool
apc_component_destroy( Handle self)
{
	DEFXX;
	if ( XX-> q_instance_name) {
		free( XX-> q_instance_name);
		XX-> q_instance_name = nil;
	}
	if ( XX-> q_class_name) {
		free( XX-> q_class_name);
		XX-> q_class_name = nil;
	}

	free( PComponent( self)-> sysData);
	PComponent( self)-> sysData = nil;
	X_WINDOW = nilHandle;
	return true;
}

Bool
apc_component_fullname_changed_notify( Handle self)
{
	Handle *list;
	PComponent me = PComponent( self);
	int i, n;

	if ( self == nilHandle) return false;
	if (!update_quarks_cache( self)) return false;

	if ( me-> components && (n = me-> components-> count) > 0) {
		if ( !( list = allocn( Handle, n))) return false;
		memcpy( list, me-> components-> items, sizeof( Handle) * n);

		for ( i = 0; i < n; i++) {
			apc_component_fullname_changed_notify( list[i]);
		}
		free( list);
	}

	return true;
}

/* Cursor support */

void
prima_no_cursor( Handle self)
{
	if ( self && guts.focused == self && X(self)
		&& !(XF_IN_PAINT(X(self)))
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
		XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &guts. cursor_gcv);
		XCHECKPOINT;
		XCopyArea( DISP, guts. cursor_save, XX-> udrawable, XX-> gc,
					0, 0, w, h, x, y);
		XCHECKPOINT;
		prima_release_gc( XX);
		guts. cursor_shown = false;
	}
}

void
prima_update_cursor( Handle self)
{
	if (
		guts.focused == self
		&& !(XF_IN_PAINT(X(self)))
	) {
		DEFXX;
		int x, y, w, h;

		h = XX-> cursor_size. y;
		y = XX-> size. y - (h + XX-> cursor_pos. y);
		x = XX-> cursor_pos. x;
		w = XX-> cursor_size. x;

		if ( !guts. cursor_save || !guts. cursor_xor
			|| w > guts. cursor_pixmap_size. x
			|| h > guts. cursor_pixmap_size. y ||
			XX-> flags. layered != guts. cursor_layered
			)
		{
			if ( !guts. cursor_save) {
				guts. cursor_gcv. background = 0;
				guts. cursor_gcv. foreground = 0xffffffff;
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
			guts. cursor_save = XCreatePixmap( DISP, XX-> udrawable,
														guts. cursor_pixmap_size. x,
														guts. cursor_pixmap_size. y,
														XX-> visual-> depth);
			guts. cursor_xor  = XCreatePixmap( DISP, XX-> udrawable,
														guts. cursor_pixmap_size. x,
														guts. cursor_pixmap_size. y,
														XX-> visual-> depth);
			guts. cursor_layered = XX-> flags. layered;
		}

		prima_get_gc( XX);
		XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &guts. cursor_gcv);
		XCHECKPOINT;
		XCopyArea( DISP, XX-> udrawable, guts. cursor_save, XX-> gc,
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
	if (
		guts. focused &&
		X(guts. focused)-> flags. cursor_visible &&
		!(XF_IN_PAINT(X(guts. focused)))
	) {
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
		XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &guts. cursor_gcv);
		XCHECKPOINT;
		XCopyArea( DISP, pixmap, XX-> udrawable, XX-> gc, 0, 0, w, h, x, y);
		XCHECKPOINT;
		prima_release_gc( XX);
		XFlush( DISP);
		XCHECKPOINT;
	} else {
		apc_timer_stop( CURSOR_TIMER);
		guts. cursor_shown = !guts. cursor_shown;
	}
}

Bool
apc_cursor_set_pos( Handle self, int x, int y)
{
	DEFXX;
	prima_no_cursor( self);
	RANGE2(x,y);
	XX-> cursor_pos. x = x;
	XX-> cursor_pos. y = y;
	prima_update_cursor( self);
	return true;
}

Bool
apc_cursor_set_size( Handle self, int x, int y)
{
	DEFXX;
	prima_no_cursor( self);
	if ( x < 0) x = 1;
	if ( y < 0) y = 1;
	if ( x > 16383) x = 16383;
	if ( y > 16383) y = 16383;
	XX-> cursor_size. x = x;
	XX-> cursor_size. y = y;
	prima_update_cursor( self);
	return true;
}

Bool
apc_cursor_set_visible( Handle self, Bool visible)
{
	DEFXX;
	if ( XX-> flags. cursor_visible != visible) {
		prima_no_cursor( self);
		XX-> flags. cursor_visible = visible;
		prima_update_cursor( self);
	}
	return true;
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

/* File */

void
prima_rebuild_watchers( void)
{
	int i;
	PFile f;

	FD_ZERO( &guts.read_set);
	FD_ZERO( &guts.write_set);
	FD_ZERO( &guts.excpt_set);
	FD_SET( guts.connection, &guts.read_set);
	guts.max_fd = guts.connection;
	for ( i = 0; i < guts.files->count; i++) {
		f = (PFile)list_at( guts.files,i);
		if ( f-> eventMask & feRead) {
			FD_SET( f->fd, &guts.read_set);
			if ( f->fd > guts.max_fd)
				guts.max_fd = f->fd;
		}
		if ( f-> eventMask & feWrite) {
			FD_SET( f->fd, &guts.write_set);
			if ( f->fd > guts.max_fd)
				guts.max_fd = f->fd;
		}
		if ( f-> eventMask & feException) {
			FD_SET( f->fd, &guts.excpt_set);
			if ( f->fd > guts.max_fd)
				guts.max_fd = f->fd;
		}
	}
}

Bool
apc_file_attach( Handle self)
{
	if ( PFile(self)->fd >= FD_SETSIZE ) return false;

	if ( list_index_of( guts.files, self) >= 0) {
		prima_rebuild_watchers();
		return true;
	}
	protect_object( self);
	list_add( guts.files, self);
	prima_rebuild_watchers();
	return true;
}

Bool
apc_file_detach( Handle self)
{
	int i;
	if (( i = list_index_of( guts.files, self)) >= 0) {
		list_delete_at( guts.files, i);
		unprotect_object( self);
		prima_rebuild_watchers();
	}
	return true;
}

Bool
apc_file_change_mask( Handle self)
{
	return apc_file_attach( self);
}

int
apc_pointer_get_state( Handle self)
{
	XWindow foo;
	int bar;
	unsigned mask;
	XQueryPointer( DISP, guts.root,  &foo, &foo, &bar, &bar, &bar, &bar, &mask);
	return
		(( mask & Button1Mask) ? mb1 : 0) |
		(( mask & Button2Mask) ? mb2 : 0) |
		(( mask & Button3Mask) ? mb3 : 0) |
		(( mask & Button4Mask) ? mb4 : 0) |
		(( mask & Button5Mask) ? mb5 : 0) |
		(( mask & Button6Mask) ? mb6 : 0) |
		(( mask & Button7Mask) ? mb7 : 0);
}

int
apc_kbd_get_state( Handle self)
{
	XWindow foo;
	int bar;
	unsigned int mask;
	XQueryPointer( DISP, guts.root, &foo, &foo, &bar, &bar, &bar, &bar, &mask);
	return
		(( mask & ShiftMask)   ? kmShift : 0) |
		(( mask & ControlMask) ? kmCtrl  : 0) |
		(( mask & Mod1Mask)    ? kmAlt   : 0);
}

static void
close_msgdlg( struct MsgDlg * md)
{
	md-> active  = false;
	md-> pressed = false;
	if ( md-> grab)
		XUngrabPointer( DISP, CurrentTime);
	md-> grab    = false;
	XUnmapWindow( DISP, md-> w);
	XFlush( DISP);
	if ( md-> next == nil) {
		XSetInputFocus( DISP, md-> focus, md-> focus_revertTo, CurrentTime);
		XCHECKPOINT;
	}
}

void
prima_msgdlg_event( XEvent * ev, struct MsgDlg * md)
{
	XWindow w = ev-> xany. window;
	switch ( ev-> type) {
	case ConfigureNotify:
		md-> winSz. x = ev-> xconfigure. width;
		md-> winSz. y = ev-> xconfigure. height;
		break;
	case Expose:
		{
			int i, y = md-> textPos. y;
			int d = md-> pressed ? 2 : 0;
			XSetForeground( DISP, md-> gc, md-> bg. primary);
			if ( md-> bg. balance > 0) {
				Pixmap p = prima_get_hatch( &guts. ditherPatterns[ md-> bg. balance]);
				if ( p) {
					XSetStipple( DISP, md-> gc, p);
					XSetFillStyle( DISP, md-> gc, FillOpaqueStippled);
					XSetBackground( DISP, md-> gc, md-> bg. secondary);
				}
			}
			XFillRectangle( DISP, w, md-> gc, 0, 0, md-> winSz.x, md-> winSz.y);
			if ( md-> bg. balance > 0)
				XSetFillStyle( DISP, md-> gc, FillSolid);
			XSetForeground( DISP, md-> gc, md-> fg);
			for ( i = 0; i < md-> wrappedCount; i++) {
				if ( md-> wide)
					XDrawString16( DISP, w, md-> gc,
					( md-> winSz.x - md-> widths[i]) / 2, y,
						( XChar2b*) md-> wrapped[i], md-> lengths[i]);
				else
					XDrawString( DISP, w, md-> gc,
					( md-> winSz.x - md-> widths[i]) / 2, y,
						md-> wrapped[i], md-> lengths[i]);
				y += md-> font-> height + md-> font-> externalLeading;
			}
			XDrawRectangle( DISP, w, md-> gc,
				md-> btnPos.x-1, md-> btnPos.y-1, md-> btnSz.x+2, md-> btnSz.y+2);
			XDrawString( DISP, w, md-> gc,
				md-> btnPos.x + ( md-> btnSz.x - md-> OKwidth) / 2 + d,
				md-> btnPos.y + md-> font-> height + md-> font-> externalLeading +
				( md-> btnSz.y - md-> font-> height - md-> font-> externalLeading) / 2 - 2 + d,
				"OK", 2);
			XSetForeground( DISP, md-> gc,
				md-> pressed ? md-> d3d : md-> l3d);
			XDrawLine( DISP, w, md-> gc,
				md-> btnPos.x, md-> btnPos.y + md-> btnSz.y - 1,
				md-> btnPos.x, md-> btnPos. y);
			XDrawLine( DISP, w, md-> gc,
				md-> btnPos.x + 1, md-> btnPos. y,
				md-> btnPos.x + md-> btnSz.x - 1, md-> btnPos. y);
			XSetForeground( DISP, md-> gc,
				md-> pressed ? md-> l3d : md-> d3d);
			XDrawLine( DISP, w, md-> gc,
				md-> btnPos.x, md-> btnPos.y + md-> btnSz.y,
				md-> btnPos.x + md-> btnSz.x, md-> btnPos.y + md-> btnSz.y);
			XDrawLine( DISP, w, md-> gc,
				md-> btnPos.x + md-> btnSz.x, md-> btnPos.y + md-> btnSz.y - 1,
				md-> btnPos.x + md-> btnSz.x, md-> btnPos.y + 1);
		}
		break;
	case ButtonPress:
		if ( !md-> grab &&
			( ev-> xbutton. button == Button1) &&
			( ev-> xbutton. x >= md-> btnPos. x ) &&
			( ev-> xbutton. x < md-> btnPos. x + md-> btnSz.x) &&
			( ev-> xbutton. y >= md-> btnPos. y ) &&
			( ev-> xbutton. y < md-> btnPos. y + md-> btnSz.y)) {
			md-> pressed = true;
			md-> grab = true;
			XClearArea( DISP, w, md-> btnPos.x, md-> btnPos.y,
				md-> btnSz.x, md-> btnSz.y, true);
			XGrabPointer( DISP, w, false,
				ButtonReleaseMask | PointerMotionMask | ButtonMotionMask,
				GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
		}
		break;
	case MotionNotify:
		if ( md-> grab) {
			Bool np =
			(( ev-> xmotion. x >= md-> btnPos. x ) &&
				( ev-> xmotion. x < md-> btnPos. x + md-> btnSz.x) &&
				( ev-> xmotion. y >= md-> btnPos. y ) &&
				( ev-> xmotion. y < md-> btnPos. y + md-> btnSz.y));
			if ( np != md-> pressed) {
				md-> pressed = np;
				XClearArea( DISP, w, md-> btnPos.x, md-> btnPos.y,
					md-> btnSz.x, md-> btnSz.y, true);
			}
		}
		break;
	case KeyPress:
		{
			char str_buf[256];
			KeySym keysym;
			int str_len = XLookupString( &ev-> xkey, str_buf, 256, &keysym, nil);
			if (
				( keysym == XK_Return) ||
				( keysym == XK_Escape) ||
				( keysym == XK_KP_Enter) ||
				( keysym == XK_KP_Space) ||
				(( str_len == 1) && ( str_buf[0] == ' '))
				)
				close_msgdlg( md);
		}
		break;
	case ButtonRelease:
		if ( md-> grab &&
			( ev-> xbutton. button == Button1)) {
			md-> grab = false;
			XUngrabPointer( DISP, CurrentTime);
			if ( md-> pressed) close_msgdlg( md);
		}
		break;
	case ClientMessage:
		if (( ev-> xclient. message_type == WM_PROTOCOLS) &&
			(( Atom) ev-> xclient. data. l[0] == WM_DELETE_WINDOW))
			close_msgdlg( md);
		break;
	}
}

extern char ** Drawable_do_text_wrap( Handle, TextWrapRec *);

Bool
apc_show_message( const char * message, Bool utf8)
{
	char ** wrapped;
	Font f;
	Point appSz, appPos;
	Point textSz;
	Point winSz;
	TextWrapRec twr;
	int i;
	struct MsgDlg md, **storage;
	Bool ret = true;
	PList font_abc_unicode = nil;
	PFontABC font_abc_ascii = nil;

	if ( !DISP) {
		warn( "%s", message);
		return true;
	}

	if ( guts. grab_widget)
		apc_widget_set_capture( guts. grab_widget, 0, 0);

	appSz = apc_application_get_size( nilHandle);
	appPos.x = 0;
	appPos.y = 0;

	/* multi-monitor centering */
	{
		int i, nrects = 0;
		Box *best = nil, *rects = apc_application_get_monitor_rects( application, &nrects);
		for ( i = 0; i < nrects; i++) {
				Box * curr = rects + i;
				if ( best == nil || best-> x > curr->x || best->y > curr->y)
						best = curr;
		}
		if ( best ) {
				appPos.x = best->x;
				appPos.y = best->y;
				appSz.x  = best->width;
				appSz.y  = best->height;
		}
	}

	/* acquiring message font and wrapping message text */
	{
		PCachedFont cf;
		XFontStruct *fs;
		int max;

		apc_sys_get_msg_font( &f);
		prima_core_font_pick( nilHandle, &f, &f);
		cf = prima_find_known_font( &f, false, false);
		if ( !cf || !cf-> id) {
			warn( "%s", message);
			return false;
		}
		fs = XQueryFont( DISP, cf-> id);
		if (!fs) {
			warn( "%s", message);
			return false;
		}

		twr. text      = ( char *) message;
		twr. utf8_text = utf8;
		twr. textLen   = strlen( message);
		twr. utf8_textLen = utf8 ? prima_utf8_length( message) : twr. textLen;
		twr. width     = appSz. x * 2 / 3;
		twr. tabIndent = 3;
		twr. options   = twNewLineBreak | twWordBreak | twReturnLines;
		twr. ascii     = &font_abc_ascii;
		twr. unicode   = &font_abc_unicode;
		guts. font_abc_nil_hack = fs;
		wrapped = Drawable_do_text_wrap( nilHandle, &twr);

		if ( font_abc_ascii) free( font_abc_ascii);
		if ( font_abc_unicode) {
			int i;
			for ( i = 0; i < font_abc_unicode-> count; i += 2)
				free(( void*) font_abc_unicode-> items[ i + 1]);
			plist_destroy( font_abc_unicode);
		}

		if ( !( md. widths  = malloc( twr. count * sizeof(int)))) {
			XFreeFontInfo( nil, fs, 1);
			warn( "%s", message);
			return false;
		}

		if ( !( md. lengths = malloc( twr. count * sizeof(int)))) {
			free( md. widths);
			XFreeFontInfo( nil, fs, 1);
			warn( "%s", message);
			return false;
		}

		/* find text extensions */
		max = 0;
		for ( i = 0; i < twr. count; i++) {
			if ( utf8) {
				char * w;
				md. lengths[i] = prima_utf8_length( wrapped[i]);
				w = ( char *) prima_alloc_utf8_to_wchar( wrapped[i], md. lengths[i]);
				if ( !w) goto EXIT;
				free( wrapped[i]);
				wrapped[i] = w;
				md. widths[i] = XTextWidth16( fs, ( XChar2b*) wrapped[i], md. lengths[i]);
			} else {
				md. widths[i] = XTextWidth( fs, wrapped[i],
					md. lengths[i] = strlen( wrapped[i]));
			}
			if ( md. widths[i] > max) max = md. widths[i];
		}
		textSz. x = max;
		textSz. y = twr. count * ( f. height + f. externalLeading);

		md. wrapped       = wrapped;
		md. wrappedCount  = twr. count;
		md. font          = &f;
		md. fontId        = cf-> id;
		md. OKwidth       = XTextWidth( fs, "OK", 2);
		md. btnSz.x       = md. OKwidth + 2 + 10;
		if ( md. btnSz. x < 56) md. btnSz. x = 56;
		md. btnSz.y       = f. height + f. externalLeading + 2 + 12;

		winSz. x = textSz. x + 4;
		if ( winSz. x < md. btnSz. x + 2) winSz. x = md. btnSz.x + 2;
		winSz. x += f. width * 4;
		winSz. y = textSz. y + 2 + 12 + md. btnSz. y + f. height;
		while ( winSz. y + 12 >= appSz.y) {
			winSz. y -= f. height + f. externalLeading;
			md. wrappedCount--;
		}
		md. btnPos. x = ( winSz. x - md. btnSz. x) / 2;
		md. btnPos. y = winSz. y - 2 - md. btnSz. y - f. height / 2;
		md. textPos. x = 2;
		md. textPos. y = f. height * 3 / 2 + 2;
		md. winSz = winSz;

		XFreeFontInfo( nil, fs, 1);
	}

	md. wide    = utf8;
	md. active  = true;
	md. next    = nil;
	md. pressed = false;
	md. grab    = false;
	XGetInputFocus( DISP, &md. focus, &md. focus_revertTo);
	XCHECKPOINT;
	{
		char * prima = "Prima";
		XTextProperty p;
		XSizeHints xs;
		XSetWindowAttributes attrs;
		Atom net_data[2];
		attrs. event_mask = 0
			| KeyPressMask
			| ButtonPressMask
			| ButtonReleaseMask
			| ButtonMotionMask
			| PointerMotionMask
			| StructureNotifyMask
			| ExposureMask;
		attrs. override_redirect = false;
		attrs. do_not_propagate_mask = attrs. event_mask;

		md. w = XCreateWindow( DISP, guts. root,
			appPos.x + ( appSz.x - winSz.x) / 2, appPos.y + ( appSz.y - winSz.y) / 2,
			winSz.x, winSz.y, 0, CopyFromParent, InputOutput,
			CopyFromParent, CWEventMask | CWOverrideRedirect, &attrs);
		XCHECKPOINT;
		if ( !md. w) {
			ret = false;
			goto EXIT;
		}
		XSetWMProtocols( DISP, md. w, &WM_DELETE_WINDOW, 1);
		XCHECKPOINT;
		xs. flags = PMinSize | PMaxSize | USPosition;
		xs. min_width  = xs. max_width  = winSz.x;
		xs. min_height = xs. max_height = winSz. y;
		xs. x = appPos.x + ( appSz.x - winSz.x) / 2;
		xs. y = appPos.y + ( appSz.y - winSz.y) / 2;
		XSetWMNormalHints( DISP, md. w, &xs);
		if ( XStringListToTextProperty( &prima, 1, &p) != 0) {
			XSetWMIconName( DISP, md. w, &p);
			XSetWMName( DISP, md. w, &p);
			XFree( p. value);
		}
		net_data[0] = NET_WM_STATE_SKIP_TASKBAR;
		net_data[1] = NET_WM_STATE_MODAL;
		XChangeProperty( DISP, md. w, NET_WM_STATE, XA_ATOM, 32,
			PropModeReplace, ( unsigned char *) net_data, 2);
	}

	storage = &guts. message_boxes;
	while ( *storage) storage = &((*storage)-> next);
	*storage = &md;

	{
#define CLR(x) prima_allocate_color( nilHandle,prima_map_color(x,nil),nil)
		XGCValues gcv;
		gcv. font = md. fontId;
		md. gc = XCreateGC( DISP, md. w, GCFont, &gcv);
		md. fg  = CLR(clFore | wcDialog);
		prima_allocate_color( nilHandle, prima_map_color(clBack | wcDialog,nil), &md. bg);
		md. l3d = CLR(clLight3DColor | wcDialog);
		md. d3d = CLR(clDark3DColor  | wcDialog);
#undef CLR
	}


	XMapWindow( DISP, md. w);
	XMoveResizeWindow( DISP, md. w,
		appPos.x + ( appSz.x - winSz.x) / 2, appPos.y + ( appSz.y - winSz.y) / 2, winSz.x, winSz.y);
	XNoOp( DISP);
	while ( md. active && !guts. applicationClose) {
		XFlush( DISP);
		prima_one_loop_round( WAIT_ALWAYS, false);
	}

	XFreeGC( DISP, md. gc);
	XDestroyWindow( DISP, md. w);
	*storage = md. next;
EXIT:
	free( md. widths);
	free( md. lengths);
	for ( i = 0; i < twr. count; i++)
		free( wrapped[i]);
	free( wrapped);

	return ret;
}

/* system metrics */

Bool
apc_sys_get_insert_mode( void)
{
	return guts. insert;
}

PFont
apc_sys_get_msg_font( PFont f)
{
	memcpy( f, &guts. default_msg_font, sizeof( Font));
	return f;
}

PFont
apc_sys_get_caption_font( PFont f)
{
	memcpy( f, &guts. default_caption_font, sizeof( Font));
	return f;
}

static int
is_composite_display(void)
{
	if ( guts. argb_visual. visual == NULL ) return false;

#ifndef HAVE_X11_EXTENSIONS_XCOMPOSITE_H
	return -1;
#else
	/* try to become a compmgr */
	XCHECKPOINT;
	guts. composite_error_triggered = false;
	XCompositeRedirectSubwindows( DISP, guts.root, CompositeRedirectManual);
	XCHECKPOINT;
	XSync(DISP, false);
	if ( guts. composite_error_triggered )
		return true;
	XCompositeUnredirectSubwindows( DISP, guts.root, CompositeRedirectManual);
	XCHECKPOINT;
	XSync(DISP, false);
	if ( guts. composite_error_triggered )
		return true;

	return false;
#endif
}

int
apc_sys_get_value( int v)  /* XXX one big XXX */
{
	switch ( v) {
	case svYMenu: {
		Font f;
		apc_menu_default_font( &f);
		return f. height + MENU_ITEM_GAP * 2;
	}
	case svYTitleBar: /* XXX */ return 20;
	case svMousePresent:		return guts. mouse_buttons > 0;
	case svMouseButtons:		return guts. mouse_buttons;
	case svSubmenuDelay:  /* XXX ? */ return guts. menu_timeout;
	case svFullDrag: /* XXX ? */ return false;
	case svWheelPresent:		return guts.mouse_wheel_up || guts.mouse_wheel_down;
	case svXIcon:
	case svYIcon:
	case svXSmallIcon:
	case svYSmallIcon:
		{
			int ret[4], n;
			XIconSize * sz = NULL;
			if ( XGetIconSizes( DISP, guts.root, &sz, &n) && ( n > 0) && (sz != NULL)) {
				ret[0] = sz-> max_width;
				ret[1] = sz-> max_height;
				ret[2] = sz-> min_width;
				ret[3] = sz-> min_height;
			} else {
				ret[0] = ret[1] = 64;
				ret[2] = ret[3] = 20;
			}
			if ( sz) XFree( sz);
			return ret[v - svXIcon];
		}
		break;
	case svXPointer:		return guts. cursor_width;
	case svYPointer:		return guts. cursor_height;
	case svXScrollbar:		return 19;
	case svYScrollbar:		return 19;
	case svXCursor:		return 1;
	case svAutoScrollFirst:	return guts. scroll_first;
	case svAutoScrollNext:	return guts. scroll_next;
	case svXbsNone:		return 0;
	case svYbsNone:		return 0;
	case svXbsSizeable:		return 3; /* XXX */
	case svYbsSizeable:		return 3; /* XXX */
	case svXbsSingle:		return 1; /* XXX */
	case svYbsSingle:		return 1; /* XXX */
	case svXbsDialog:		return 2; /* XXX */
	case svYbsDialog:		return 2; /* XXX */
	case svShapeExtension:	return guts. shape_extension;
	case svDblClickDelay:        return guts. double_click_time_frame;
	case svColorPointer:         return 
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
		1
#else
		0
#endif
		;
	case svCanUTF8_Input:        return 1;
	case svCanUTF8_Output:       return 1;
	case svCompositeDisplay:     return is_composite_display();
	case svLayeredWidgets:       return guts. argb_visual. visual != NULL;
	case svDWM:                  return 0;
	case svFixedPointerSize:     return
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
		0
#else
		1
#endif
;
	case svMenuCheckSize   : return MENU_CHECK_XOFFSET;
	default:
		return -1;
	}
}

Bool
apc_sys_set_insert_mode( Bool insMode)
{
	guts. insert = !!insMode;
	return true;
}

/* etc */

Bool
apc_beep( int style)
{
	/* XXX - mbError, mbQuestion, mbInformation, mbWarning */
	if ( DISP)
		XBell( DISP, 0);
	return true;
}

Bool
apc_beep_tone( int freq, int duration)
{
	XKeyboardControl xkc;
	XKeyboardState   xks;
	struct timeval timeout;

	if ( !DISP) return false;

	XGetKeyboardControl( DISP, &xks);
	xkc. bell_pitch    = freq;
	xkc. bell_duration = duration;
	XChangeKeyboardControl( DISP, KBBellPitch | KBBellDuration, &xkc);

	XBell( DISP, 100);
	XFlush( DISP);

	xkc. bell_pitch    = xks. bell_pitch;
	xkc. bell_duration = xks. bell_duration;
	XChangeKeyboardControl( DISP, KBBellPitch | KBBellDuration, &xkc);

	timeout. tv_sec  = duration / 1000;
	timeout. tv_usec = 1000 * (duration % 1000);
	select( 0, nil, nil, nil, &timeout);

	return true;
}

char *
apc_system_action( const char *s)
{
	int l = strlen( s);
	switch (*s) {
	case 'b':
		if ( l == 7 && strcmp( s, "browser") == 0)
			return duplicate_string("netscape");
		break;
	case 'c':
		if ( l == 19 && strcmp( s, "can.shape.extension") == 0 && guts.shape_extension)
			return duplicate_string( "yes");
		else if ( l == 26 && strcmp( s, "can.shared.image.extension") == 0 && guts.shared_image_extension)
			return duplicate_string( "yes");
		break;
	case 'D':
		if ( l == 7 && ( strcmp( s, "Display") == 0)) {
			char * c = malloc(19);
			if ( c) snprintf( c, 18, "0x%p", DISP);
			return c;
		}
		break;
	case 'g':
		if ( l > 15 && strncmp( "get.frame.info ", s, 15) == 0) {
			char *end;
			XWindow w = strtoul( s + 15, &end, 0);
			Handle self;
			Rect r;
			char buf[ 80];

			if (*end == '\0' &&
				( self = prima_xw2h( w)) &&
				prima_get_frame_info( self, &r) &&
				snprintf( buf, sizeof(buf), "%d %d %d %d", r.left, r.bottom, r.right, r.top) < sizeof(buf))
				return duplicate_string( buf);
			return duplicate_string("");
		} else if ( strncmp( s, "gtk.OpenFile.", 13) == 0) {
			s += 13;
#ifdef WITH_GTK
			if ( guts. use_gtk )
				return prima_gtk_openfile(( char*) s);
#endif
			return nil;
		}
		break;
	case 'r':
		if ( strncmp( s, "resolution", 10) == 0) {
			int dx, dy;
			int i = sscanf( s + 10, "%u %u", &dx, &dy);
			if ( i != 2 || (dx < 1 || dy < 1)) {
				warn("Bad resolution\n");
				return 0;
			}
			guts. resolution. x = dx;
			guts. resolution. y = dy;
			return nil;
		}
		break;
	case 's':
		if ( strcmp( "synchronize", s) == 0) {
			XSynchronize( DISP, true);
			return nil;
		}
		if ( strncmp( "setfont ", s, 8) == 0) {
			Handle self = nilHandle;
			char font[1024];
			XWindow win;
			int i = sscanf( s + 8, "%lu %s", &win, font);
			if ( i != 2 || !(self = prima_xw2h( win)))  {
				warn( "Bad parameters to sysaction setfont");
				return 0;
			}
			if ( !opt_InPaint) return 0;
			XSetFont( DISP, X(self)-> gc, XLoadFont( DISP, font));
			return nil;
		}
		if ( strcmp( "shaper", s) == 0) {
			char shaper[64] = "";
#ifdef USE_XFT
			if ( guts. use_xft ) strcat(shaper, "xft ");
#endif
#ifdef WITH_FRIBIDI
			if ( guts. use_fribidi ) strcat(shaper, "fribidi ");
#endif
#ifdef WITH_HARFBUZZ
			if ( guts. use_harfbuzz ) strcat(shaper, "harfbuzz ");
#endif
			shaper[strlen(shaper)-1] = 0;
			return duplicate_string(shaper);
		}
		break;
	case 't':
		if ( strncmp( "textout16 ", s, 10) == 0) {
			Handle self = nilHandle;
			unsigned char text[1024];
			XWindow win;
			int x, y, len;
			int i = sscanf( s + 10, "%lu %d %d %s", &win, &x, &y, text);
			if ( i != 4 || !(self = prima_xw2h( win)))  {
				warn( "Bad parameters to sysaction textout16");
				return 0;
			}
			if ( !opt_InPaint) return 0;
			len = strlen((char*) text);
			for ( i = 0; i < len; i++) if ( text[i]==255) text[i] = 0;
			XDrawString16( DISP, win, X(self)-> gc, x, y, ( XChar2b *) text, len / 2);
			return nil;
		}
		break;
	case 'u':
		if ( strcmp( s, "unix_guts") == 0)
			return (char*) &guts;
		break;
	case 'x':
		if ( strncmp(s, "xquartz.", 8) == 0) {
			s += 8;
#ifdef WITH_COCOA
			if ( guts. use_quartz )
				return prima_cocoa_system_action(( char*) s);
#endif
			return nil;
		}
		break;
	case 'X':
		if ( strcmp( s, "XOpenDisplay") == 0) {
			char err_buf[512];
			if ( DISP)
				return duplicate_string( "X display already opened");
			window_subsystem_set_option( "yes-x11", NULL);
			if ( !window_subsystem_init( err_buf))
				return duplicate_string( err_buf);
			return NULL;
		}
		break;
	}
	warn("Unknown sysaction:%s", s);
	return nil;
}

Bool
apc_query_drives_map( const char* firstDrive, char *result, int len)
{
	if ( !result || len <= 0) return true;
	*result = 0;
	return true;
}

int
apc_query_drive_type( const char *drive)
{
	return dtNone;
}

char *
apc_get_user_name( void)
{
	char * c = getlogin();
	return c ? c : "";
}

Bool
apc_dl_export(char *path)
{
	/* XXX */
	return true;
}

PList
apc_getdir( const char *dirname)
{
	DIR *dh;
	struct dirent *de;
	PList dirlist = nil;
	char *type;
	char path[ 2048];
	struct stat s;

	if (( dh = opendir( dirname)) && (dirlist = plist_create( 50, 50))) {
		while (( de = readdir( dh))) {
			list_add( dirlist, (Handle)duplicate_string( de-> d_name));
#if defined(DT_REG) && defined(DT_DIR)
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
			default:
#endif
								snprintf( path, 2047, "%s/%s", dirname, de-> d_name);
								type = nil;
								if ( stat( path, &s) == 0) {
									switch ( s. st_mode & S_IFMT) {
									case S_IFIFO:        type = "fifo";  break;
									case S_IFCHR:        type = "chr";   break;
									case S_IFDIR:        type = "dir";   break;
									case S_IFBLK:        type = "blk";   break;
									case S_IFREG:        type = "reg";   break;
									case S_IFLNK:        type = "lnk";   break;
									case S_IFSOCK:       type = "sock";  break;
#ifdef S_IFWHT
									case S_IFWHT:        type = "wht";   break;
#endif
									}
								}
								if ( !type)     type = "unknown";
#if defined(DT_REG) && defined(DT_DIR)
			}
#endif
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


void
prima_utf8_to_wchar( const char * utf8, XChar2b * u16, int src_len_bytes, int target_len_xchars )
{
	STRLEN charlen;
	while ( target_len_xchars--) {
		register UV u = prima_utf8_uvchr(utf8, src_len_bytes, &charlen);
		if ( u < 0x10000) {
			u16-> byte1 = u >> 8;
			u16-> byte2 = u & 0xff;
		} else
			u16-> byte1 = u16-> byte2 = 0xff;
		u16++;
		utf8 += charlen;
		src_len_bytes -= charlen;
		if ( src_len_bytes <= 0 || charlen == 0) break;
	}
}

XChar2b *
prima_alloc_utf8_to_wchar( const char * utf8, int length_chars)
{
	XChar2b * ret;
	if ( length_chars < 0) length_chars = prima_utf8_length( utf8) + 1;
	if ( !( ret = malloc( length_chars * sizeof( XChar2b)))) return nil;
	prima_utf8_to_wchar( utf8, ret, strlen(utf8), length_chars);
	return ret;
}

void
prima_wchar2char( char * dest, XChar2b * src, int lim)
{
	if ( lim < 1) return;
	while ( lim-- && src-> byte1 && src-> byte2) *(dest++) = (src++)-> byte2;
	if ( lim < 0) dest--;
	*dest = 0;
}

void
prima_char2wchar( XChar2b * dest, char * src, int lim)
{
	int l = strlen( src) + 1;
	if ( lim < 1) return;
	if ( lim > l) lim = l;
	src  += lim - 2;
	dest += lim - 1;
	dest-> byte1 = dest-> byte2 = 0;
	dest--;
	while ( lim--) {
		dest-> byte2 = *(src--);
		dest-> byte1 = 0;
		dest--;
	}
}

char *
apc_last_error( void )
{
	return NULL;
}
/* printer stubs */

Bool   apc_prn_create( Handle self) { return false; }
Bool   apc_prn_destroy( Handle self) { return true; }
Bool   apc_prn_select( Handle self, const char* printer) { return false; }
char * apc_prn_get_selected( Handle self) { return nil; }
Point  apc_prn_get_size( Handle self) { Point r = {0,0}; return r; }
Point  apc_prn_get_resolution( Handle self) { Point r = {0,0}; return r; }
char * apc_prn_get_default( Handle self) { return nil; }
Bool   apc_prn_setup( Handle self) { return false; }
Bool   apc_prn_begin_doc( Handle self, const char* docName) { return false; }
Bool   apc_prn_begin_paint_info( Handle self) { return false; }
Bool   apc_prn_end_doc( Handle self) { return true; }
Bool   apc_prn_end_paint_info( Handle self) { return true; }
Bool   apc_prn_new_page( Handle self) { return true; }
Bool   apc_prn_abort_doc( Handle self) { return true; }
ApiHandle   apc_prn_get_handle( Handle self) { return ( ApiHandle) 0; }
Bool   apc_prn_set_option( Handle self, char * option, char * value) { return false; }

Bool apc_prn_get_option( Handle self, char * option, char ** value)
{
	*value = nil;
	return false;
}

Bool apc_prn_enum_options( Handle self, int * count, char *** options)
{
	*count = 0;
	return false;
}

PrinterInfo *
apc_prn_enumerate( Handle self, int * count)
{
	*count = 0;
	return nil;
}

