/***********************************************************/
/*                                                         */
/*  System dependent miscellaneous routines (unix, x11)    */
/*                                                         */
/***********************************************************/

#include <apricot.h>
#include "unix/guts.h"

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
		XX-> q_instance_name = NULL;
	}
	if ( XX-> q_class_name) {
		free( XX-> q_class_name);
		XX-> q_class_name = NULL;
	}

	free( PComponent( self)-> sysData);
	PComponent( self)-> sysData = NULL;
	X_WINDOW = NULL_HANDLE;
	return true;
}

Bool
apc_component_fullname_changed_notify( Handle self)
{
	Handle *list;
	PComponent me = PComponent( self);
	int i, n;

	if ( self == NULL_HANDLE) return false;
	if (!prima_update_quarks_cache( self)) return false;

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
	if ( md-> next == NULL) {
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
			int str_len = XLookupString( &ev-> xkey, str_buf, 256, &keysym, NULL);
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

Bool
apc_show_message( const char * message, Bool utf8)
{
	int * wrapped;
	Font f;
	Point appSz, appPos;
	Point textSz;
	Point winSz;
	TextWrapRec twr;
	int i, j;
	struct MsgDlg md, **storage;
	Bool ret = false;
	PList font_abc_unicode = NULL;
	PFontABC font_abc_ascii = NULL;
	XFontStruct *fs = NULL;

	if ( !DISP) {
		warn( "%s", message);
		return true;
	}

	if ( guts. grab_widget)
		apc_widget_set_capture( guts. grab_widget, 0, 0);

	appSz = apc_application_get_size( NULL_HANDLE);
	appPos.x = 0;
	appPos.y = 0;

	/* multi-monitor centering */
	{
		int i, nrects = 0;
		Box *best = NULL, *rects = apc_application_get_monitor_rects( prima_guts.application, &nrects);
		for ( i = 0; i < nrects; i++) {
				Box * curr = rects + i;
				if ( best == NULL || best-> x > curr->x || best->y > curr->y)
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
		int max;

		apc_sys_get_msg_font( &f);
		f. pitch = fpDefault;
#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding
		if ( !( cf = prima_font_pick( &f, NULL, NULL, FONTKEY_CORE))) {
			warn( "%s", message);
			return false;
		}
		fs = XQueryFont( DISP, cf-> id);
		if (!fs) {
			warn( "%s", message);
			return false;
		}

		bzero(&twr, sizeof(twr));
		twr. text         = ( char *) message;
		twr. utf8_text    = utf8;
		twr. textLen      = strlen( message);
		twr. utf8_textLen = utf8 ? prima_utf8_length( message, -1) : twr. textLen;
		twr. width        = appSz. x * 2 / 3;
		twr. tabIndent    = 3;
		twr. options      = twNewLineBreak | twWordBreak | twReturnLines;
		twr. ascii        = &font_abc_ascii;
		twr. unicode      = &font_abc_unicode;
		guts. font_abc_nil_hack = fs;
		wrapped = CDrawable->do_text_wrap( NULL_HANDLE, &twr, NULL, NULL);

		if ( font_abc_ascii) free( font_abc_ascii);
		if ( font_abc_unicode) {
			for ( i = 0; i < font_abc_unicode-> count; i += 2)
				free(( void*) font_abc_unicode-> items[ i + 1]);
			plist_destroy( font_abc_unicode);
		}

		md.wrappedCount = md.count = twr.count / 4;
		if ( !( md.widths  = malloc( md.count * sizeof(int))))
			goto EXIT;
		if ( !( md.lengths = malloc( md.count * sizeof(int))))
			goto EXIT;
		if ( !( md.wrapped = malloc( md.count * sizeof(char*))))
			goto EXIT;
		bzero(md.wrapped, md.count * sizeof(char*));

		/* find text extensions */
		max = 0;
		for ( i = j = 0; i < md.count; i++, j += 4) {
			md.lengths[i] = wrapped[j+3];
			if (utf8) {
				if (!(md.wrapped[i] = (char*)prima_alloc_utf8_to_wchar( message + wrapped[j], md.lengths[i])))
					goto EXIT;
				md.widths[i] = XTextWidth16( fs, (XChar2b*) md.wrapped[i], md.lengths[i]);
			} else {
				if (!(md.wrapped[i] = malloc( md.lengths[i] + 1)))
					goto EXIT;
				memcpy(md.wrapped[i], message + wrapped[j], md.lengths[i]);
				md.wrapped[i][md.lengths[i]] = 0;
				md. widths[i] = XTextWidth( fs, md.wrapped[i], md.lengths[i]);
			}
			if ( md. widths[i] > max) max = md. widths[i];
		}
		textSz. x = max;
		textSz. y = md.count * ( f. height + f. externalLeading);

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
	}

	md. wide    = utf8;
	md. active  = true;
	md. next    = NULL;
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
		if ( !md. w) goto EXIT;
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
#define CLR(x) prima_allocate_color( NULL_HANDLE,prima_map_color(x,NULL),NULL)
		XGCValues gcv;
		gcv. font = md. fontId;
		gcv. cap_style = CapProjecting;
		md. gc = XCreateGC( DISP, md. w, GCFont | GCCapStyle, &gcv);
		md. fg  = CLR(clFore | wcDialog);
		prima_allocate_color( NULL_HANDLE, prima_map_color(clBack | wcDialog,NULL), &md. bg);
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
	ret = true;
EXIT:
	XFreeFontInfo( NULL, fs, 1);
	if ( md.widths ) free( md.widths);
	if ( md.lengths) free( md.lengths);
	if ( md.wrapped ) {
		for ( i = 0; i < md.count; i++) 
			free(md.wrapped[i]);
		free(md.wrapped);
	}
	md.wrappedCount = 0;

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
	case svYTitleBar:    /* XXX */ return 20;
	case svMousePresent:  return guts. mouse_buttons > 0;
	case svMouseButtons:  return guts. mouse_buttons;
	case svSubmenuDelay: /* XXX ? */ return guts. menu_timeout;
	case svFullDrag:     /* XXX ? */ return false;
	case svWheelPresent: return guts.mouse_wheel_up || guts.mouse_wheel_down;
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
	case svXPointer:        return guts. cursor_width;
	case svYPointer:        return guts. cursor_height;
	case svXScrollbar:      return 19;
	case svYScrollbar:      return 19;
	case svXCursor:         return 1;
	case svAutoScrollFirst: return guts. scroll_first;
	case svAutoScrollNext:  return guts. scroll_next;
	case svXbsNone:	        return 0;
	case svYbsNone:	        return 0;
	case svXbsSizeable:     return 3; /* XXX */
	case svYbsSizeable:     return 3; /* XXX */
	case svXbsSingle:       return 1; /* XXX */
	case svYbsSingle:       return 1; /* XXX */
	case svXbsDialog:       return 2; /* XXX */
	case svYbsDialog:       return 2; /* XXX */
	case svShapeExtension:	return guts. shape_extension;
	case svDblClickDelay:   return guts. double_click_time_frame;
	case svColorPointer:    return 
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
	case svFixedPointerSize:     return
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
		0
#else
		1
#endif
;
	case svMenuCheckSize   : return MENU_CHECK_XOFFSET;
	case svFriBidi         : return prima_guts.use_fribidi;
	case svLibThai         : return prima_guts.use_libthai;
	case svAntialias       : return guts. argb_visual. visual != NULL;
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
	select( 0, NULL, NULL, NULL, &timeout);

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
			return NULL;
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
			return NULL;
		}
		break;
	case 's':
		if ( strcmp( "synchronize", s) == 0) {
			XSynchronize( DISP, true);
			return NULL;
		}
		if ( strncmp( "setfont ", s, 8) == 0) {
			Handle self = NULL_HANDLE;
			char font[1024];
			XWindow win;
			int i = sscanf( s + 8, "%lu %s", &win, font);
			if ( i != 2 || !(self = prima_xw2h( win)))  {
				warn( "Bad parameters to sysaction setfont");
				return 0;
			}
			if ( !opt_InPaint) return 0;
			XSetFont( DISP, X(self)-> gc, XLoadFont( DISP, font));
			return NULL;
		}
		if ( strcmp( "shaper", s) == 0) {
			char shaper[64] = "";
#ifdef USE_XFT
			if ( guts. use_xft ) strcat(shaper, "xft ");
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
			Handle self = NULL_HANDLE;
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
			return NULL;
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
			return NULL;
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
	return NULL;
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
	unsigned int charlen;
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
	if ( length_chars < 0) length_chars = prima_utf8_length( utf8, -1) + 1;
	if ( !( ret = malloc( length_chars * sizeof( XChar2b)))) return NULL;
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

/* printer stubs */

Bool   apc_prn_create( Handle self) { return false; }
Bool   apc_prn_destroy( Handle self) { return true; }
Bool   apc_prn_select( Handle self, const char* printer) { return false; }
char * apc_prn_get_selected( Handle self) { return NULL; }
Point  apc_prn_get_size( Handle self) { Point r = {0,0}; return r; }
Point  apc_prn_get_resolution( Handle self) { Point r = {0,0}; return r; }
char * apc_prn_get_default( Handle self) { return NULL; }
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
	*value = NULL;
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
	return NULL;
}

char *
apc_last_error( void )
{
	return NULL;
}

