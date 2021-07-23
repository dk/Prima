/***********************************************************/
/*                                                         */
/*  System dependent widget management (unix, x11)         */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Window.h"
#include "Application.h"

#define SORT(a,b)       { int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)	( XX-> size. y - (a) - 1)

Bool
apc_widget_map_points( Handle self, Bool toScreen, int n, Point *p)
{
	Point d = {0,0};

	while ( self && (self != application)) {
		DEFXX;
		Point origin;
		if ( XX-> parentHandle) {
			XWindow dummy;
			XTranslateCoordinates( DISP, XX-> client, guts. root, 0, XX-> size.y-1, &origin.x, &origin.y, &dummy);
			origin. y = guts. displaySize. y - origin. y;
			self = application;
		} else {
			origin = XX-> origin;
			self = XX-> flags. clip_owner ? PWidget(self)-> owner : application;
		}
		d. x += origin. x;
		d. y += origin. y;
	}

	if ( !toScreen) {
		d. x = -d. x;
		d. y = -d. y;
	}

	while (n--) {
		p[n]. x += d. x;
		p[n]. y += d. y;
	}
	return true;
}

ApiHandle
apc_widget_get_parent_handle( Handle self)
{
	return X(self)-> parentHandle;
}

Handle
apc_widget_get_z_order( Handle self, int zOrderId)
{
	DEFXX;
	XWindow root, parent, *children;
	unsigned int count;
	int i, inc;
	Handle ret = NULL_HANDLE;

	if ( !( PComponent(self)-> owner))
		return self;

	switch ( zOrderId) {
	case zoFirst:
		i = 1;
		inc = -1;
		break;
	case zoLast:
		i  = 1;
		inc = 1;
		break;
	case zoNext:
		i = 0;
		inc = -1;
		break;
	case zoPrev:
		i = 0;
		inc = 1;
		break;
	default:
		return NULL_HANDLE;
	}

	if ( XQueryTree( DISP, X(PComponent(self)-> owner)-> client,
		&root, &parent, &children, &count) == 0)
			return NULL_HANDLE;

	if ( count == 0) goto EXIT;

	if ( i == 0) {
		int found = -1;
		for ( i = 0; i < count; i++) {
			if ( children[ i] == XX-> client) {
				found = i;
				break;
			}
		}
		if ( found < 0) { /* if !clipOwner */
			ret = self;
			goto EXIT;
		}
		i = found + inc;
		if ( i < 0 || i >= count) goto EXIT; /* last in line */
	} else
		i = ( zOrderId == zoFirst) ? count - 1 : 0;

	while ( 1) {
		Handle who = ( Handle) hash_fetch( guts. windows, (void*)&(children[i]), sizeof(X_WINDOW));
		if ( who) {
			ret = who;
			break;
		}
		i += inc;
		if ( i < 0 || i >= count) break;
	}

EXIT:
	if ( children) XFree( children);
	return ret;
}

void
process_transparents( Handle self)
{
	int i;
	Point sz = X(self)-> size;
	for ( i = 0; i < PWidget(self)-> widgets. count; i++) {
		Handle x = PWidget(self)-> widgets. items[ i];
		if ( X(x)-> flags. transparent &&
			X(x)-> flags. want_visible &&
			!X(x)-> flags. falsely_hidden) {
			Point pos = X(x)-> origin;
			if ( pos. x >= sz.x || pos.y >= sz.y ||
				pos. x + X(x)-> size.x <= 0 ||
				pos. y + X(x)-> size.y <= 0) continue;
			apc_widget_invalidate_rect( x, NULL);
		}
	}
}

int
prima_flush_events( Display * disp, XEvent * ev, Handle self)
{
	XWindow win;
	/* leave only configuration unrelated commands on the queue */
	switch ( ev-> type) {
	case SelectionRequest:
	case SelectionClear:
	case MappingNotify:
	case SelectionNotify:
	case ClientMessage:
	case MapNotify:
	case UnmapNotify:
	case KeymapNotify:
	case KeyPress:
	case KeyRelease:
	case PropertyNotify:
	case ColormapNotify:
	case DestroyNotify:
		return false;
	}

	switch ( ev-> type) {
	case ConfigureNotify:
	case -ConfigureNotify:
		win = ev-> xconfigure. window;
		break;
	case ReparentNotify:
		win = ev-> xreparent. window;
		break;
	default:
		win = ev-> xany. window;
	}

	return win == X(self)-> client || win == X_WINDOW;
}

void
prima_get_view_ex( Handle self, PViewProfile p)
{
	DEFXX;
	if ( !p) return;
	if ( XX-> type. window) {
		p-> pos       = apc_window_get_client_pos( self);
		p-> size      = apc_window_get_client_size( self);
		XFetchName( DISP, X_WINDOW, &p-> title);
	} else {
		p-> pos       = apc_widget_get_pos( self);
		p-> size      = apc_widget_get_size( self);
		p-> title     = NULL;
	}
	p-> capture   = apc_widget_is_captured( self);
	p-> focused   = apc_widget_is_focused( self);
	p-> visible   = apc_widget_is_visible( self);

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
	p-> shape_count = 0;
	if ( XX-> shape_extent. x != 0 && XX-> shape_extent. y != 0)
		p-> shape_rects = XShapeGetRectangles( DISP, X_WINDOW, ShapeBounding, &p-> shape_count, &p->shape_ordering);
#endif
}

void
prima_set_view_ex( Handle self, PViewProfile p)
{
	DEFXX;

	if ( p-> visible ) XMapWindow( DISP, X_WINDOW);
	XX-> origin. x--; /* force it */
	if ( XX-> type. window ) {
		apc_window_set_client_rect( self, p-> pos.x, p-> pos.y, p-> size.x, p->size.y);
		apc_window_set_caption( self, p->title, XX->flags. title_utf8);
		XFree(p->title);
	} else {
		apc_widget_set_rect( self, p-> pos.x, p-> pos.y, p-> size.x, p->size.y);
	}

	if ( p-> focused) apc_widget_set_focused( self);
	if ( p-> capture) apc_widget_set_capture( self, 1, NULL_HANDLE);

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
	if ( p-> shape_count > 0 ) {
		XShapeCombineRectangles( DISP, X_WINDOW, ShapeBounding, 0, 0, p-> shape_rects, p->shape_count, ShapeSet, p->shape_ordering);
		if ( X_WINDOW != XX-> client)
			XShapeCombineRectangles( DISP, XX->client, ShapeBounding, 0, 0, p-> shape_rects, p->shape_count, ShapeSet, p->shape_ordering);
		XFree( p-> shape_rects );
	}
#endif
}

void
prima_notify_sys_handle( Handle self )
{
	Event ev = {cmSysHandle};
	ev. gen. source = self;
	apc_message( self, &ev, false);
}

Bool
apc_widget_create( Handle self, Handle owner, Bool sync_paint,
						Bool clip_owner, Bool transparent, ApiHandle parentHandle, Bool layered)
{
	DEFXX;
	ViewProfile vprf;
	Bool reparent, recreate, layered_requested;
	Handle real_owner, old_parent;
	XWindow parent, old = X_WINDOW;
	XSetWindowAttributes attrs;
	unsigned long valuemask;

	if ( !guts. argb_visual. visual || guts. argb_visual. visualid == guts. visual. visualid)
		layered = false;

	layered_requested = layered;
	layered = ( clip_owner && owner != application ) ? X(owner)->flags. layered : layered_requested;

	XX-> visual   = layered ? &guts. argb_visual : &guts. visual;
	XX-> colormap = layered ? guts. argbColormap : guts. defaultColormap;

	reparent = ( old != NULL_HANDLE) && (
		( clip_owner != XX-> flags. clip_owner) ||
		( parentHandle != XX-> parent)
	);
	recreate = ( old != NULL_HANDLE) && (
		( layered != XX-> flags. layered )
	);
	if ( recreate ) {
		int i, count;
		Handle * list;
		XEvent dummy_ev;

		list  = PWidget(self)-> widgets. items;
		count = PWidget(self)-> widgets. count;
		CWidget(self)-> end_paint_info( self);
		CWidget(self)-> end_paint( self);
		prima_release_gc( XX);
		for( i = 0; i < count; i++)
			prima_get_view_ex( list[ i], ( ViewProfile*)( X( list[ i])-> recreateData = malloc( sizeof( ViewProfile))));

		reparent = false;
		if ( XX-> recreateData) {
			memcpy( &vprf, XX-> recreateData, sizeof( vprf));
			free( XX-> recreateData);
			XX-> recreateData = NULL;
		} else
			prima_get_view_ex( self, &vprf);
		if ( guts. currentMenu && PComponent( guts. currentMenu)-> owner == self) prima_end_menu();
		CWidget( self)-> end_paint_info( self);
		CWidget( self)-> end_paint( self);
		if ( XX-> flags. paint_pending) {
			TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = false;
		}
		/* flush configure events */
		XSync( DISP, false);
		while ( XCheckIfEvent( DISP, &dummy_ev, (XIfEventProcType)prima_flush_events, (XPointer)self));
		hash_delete( guts.windows, (void*)&old, sizeof(old), false);
		XCHECKPOINT;
	}

	old_parent = ( old != NULL_HANDLE ) ? XX->parent : NULL_HANDLE;

	XX-> flags. transparent = !!transparent;
	XX-> type.drawable = true;
	XX-> type.widget = true;
	if ( !clip_owner || ( owner == application)) {
		parent = guts. root;
		real_owner = application;
	} else {
		parent = X( owner)-> client;
		real_owner = owner;
	}

	if ( parentHandle)
		parent = parentHandle;
	XX-> parentHandle = parentHandle;
	XX-> real_parent = XX-> parent = parent;
	XX-> above = NULL_HANDLE;
	XX-> owner = real_owner;

	XX-> flags. clip_owner = !!clip_owner;
	XX-> flags. sync_paint = !!sync_paint;
	XX-> flags. layered    = !!layered;
	XX-> flags. layered_requested = !!layered_requested;

	attrs. event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask
		| ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask
		| ButtonMotionMask | KeymapStateMask | ExposureMask | VisibilityChangeMask
		| StructureNotifyMask | FocusChangeMask | PropertyChangeMask | ColormapChangeMask
		| OwnerGrabButtonMask;
	attrs. override_redirect = true;
	attrs. do_not_propagate_mask = attrs. event_mask;
	attrs. win_gravity = ( clip_owner && ( owner != application))
		? SouthWestGravity : NorthWestGravity;
	attrs. colormap = XX->colormap;

	if ( reparent) {
		Point pos = PWidget(self)-> pos;
		XEvent dummy_ev;

		if ( old_parent == parent ) return true;

		if ( guts. currentMenu && PComponent( guts. currentMenu)-> owner == self) prima_end_menu();
		CWidget( self)-> end_paint_info( self);
		CWidget( self)-> end_paint( self);
		if ( XX-> flags. paint_pending) {
			TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = false;
		}
		/* flush configure events */
		XSync( DISP, false);
		while ( XCheckIfEvent( DISP, &dummy_ev, (XIfEventProcType)prima_flush_events, (XPointer)self));

		XChangeWindowAttributes( DISP, X_WINDOW, CWWinGravity, &attrs);
		XReparentWindow( DISP, X_WINDOW, parent, pos. x,
			X(real_owner)-> size.y - pos. y - X(self)-> size. y);

		XX-> ackOrigin = pos;
		XX-> ackSize   = XX-> size;
		XX-> flags. mapped = XX-> flags. want_visible;
		process_transparents( self);
		return true;
	}

	valuemask =
		0
		/* | CWBackPixmap */
		/* | CWBackPixel */
		/* | CWBorderPixmap */
		/* | CWBorderPixel */
		/* | CWBitGravity */
		| CWWinGravity
		/* | CWBackingStore */
		/* | CWBackingPlanes */
		/* | CWBackingPixel */
		| CWOverrideRedirect
		/* | CWSaveUnder */
		| CWEventMask
		/* | CWDontPropagate */
		| CWColormap
		/* | CWCursor */
	;

	if ( layered ) {
		valuemask |= CWBackPixel | CWBorderPixel;
		attrs. background_pixel = 0;
		attrs. border_pixel = 0;
	}

	XX-> client = X_WINDOW = XCreateWindow( DISP, parent,
									0, 0, 1, 1, 0, XX-> visual-> depth,
									InputOutput, XX-> visual-> visual,
									valuemask, &attrs);
	XCHECKPOINT;
	if (!X_WINDOW) {
		warn("error creating window");
		return false;
	}

	if (recreate) {
		int i, count;
		Handle * list;
		Point pos = PWidget(self)-> pos;
		list  = PWidget(self)-> widgets. items;
		count = PWidget(self)-> widgets. count;
		prima_set_view_ex( self, &vprf);
		XX-> gdrawable = XX-> udrawable = X_WINDOW;
		XX-> ackOrigin = pos;
		XX-> ackSize   = XX-> size;
		XX-> flags. mapped = XX-> flags. want_visible;
		hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);
		for ( i = 0; i < count; i++) ((( PComponent) list[ i])-> self)-> recreate( list[ i]);
		XDestroyWindow( DISP, old);
		prima_notify_sys_handle( self );
		return true;
	}

	XX-> size. x = XX-> size. y =
	XX-> ackOrigin. x = XX-> ackOrigin. y =
	XX-> ackSize. x = XX-> ackSize. y = 0;

	hash_store( guts.windows, &X_WINDOW, sizeof(X_WINDOW), (void*)self);

	XX-> gdrawable = XX-> udrawable = X_WINDOW;

	XX-> flags. position_determined = 1;

	apc_component_fullname_changed_notify( self);

	prima_send_create_event( X_WINDOW);

	return true;
}

Bool
apc_widget_begin_paint( Handle self, Bool inside_on_paint)
{
	DEFXX;
	Bool useRPDraw = false;

	if ( guts. appLock > 0) return false;

	if ( XX-> size.x <= 0 || XX-> size.y <= 0) return false;

	if ( XX-> flags. transparent && inside_on_paint) {
		if ( XX-> flags. want_visible && !XX-> flags. falsely_hidden) {
			if ( XX-> parent == guts. root) {
				XEvent ev;
				if ( XX-> flags. transparent_busy) return false;
				XX-> flags. transparent_busy = 1;
				XUnmapWindow( DISP, X_WINDOW);
				XSync( DISP, false);
				while ( XCheckMaskEvent( DISP, ExposureMask, &ev))
					prima_handle_event( &ev, NULL);
				XMapWindow( DISP, X_WINDOW);
				XSync( DISP, false);
				while ( XCheckMaskEvent( DISP, ExposureMask, &ev))
					prima_handle_event( &ev, NULL);
				if ( XX-> flags. paint_pending) {
					TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
					XX-> flags. paint_pending = false;
				}
				XX-> flags. transparent_busy = 0;
			} else
				useRPDraw = true;
		}
	}
	XCHECKPOINT;
	if ( guts. dynamicColors && inside_on_paint) prima_palette_free( self, false);
	prima_no_cursor( self);
	prima_prepare_drawable_for_painting( self, inside_on_paint);
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	XX->argb_picture = XRenderCreatePicture( DISP, XX->gdrawable,
		XF_LAYERED(XX) ? guts.xrender_argb32_format : guts.xrender_display_format,
		0, NULL);
#endif
	if ( useRPDraw) {
		Handle owner = PWidget(self)->owner;
		Point po = apc_widget_get_pos( self);
		Point sz = apc_widget_get_size( self);
		Point so = CWidget(owner)-> get_size( owner);
		XDrawable dc;
		Region region;
		XRectangle xr = {0,0,0,0};
		xr. width = sz.x;
		xr. height = sz.y;

		CWidget(owner)-> begin_paint( owner);
		dc = X(owner)-> gdrawable;
		X(owner)-> gdrawable = XX-> gdrawable;
		X(owner)-> btransform. x = -po. x;
		X(owner)-> btransform. y = so. y - sz. y - po. y;
		if ( X(owner)-> paint_region) {
			XDestroyRegion( X(owner)-> paint_region);
			X(owner)-> paint_region = NULL;
		}
		region = XCreateRegion();
		XUnionRectWithRegion( &xr, region, region);
		if ( XX-> paint_region)
			XIntersectRegion( XX-> paint_region, region, region);
		X(owner)-> paint_region = XCreateRegion();
		XUnionRegion( X(owner)-> paint_region, region, X(owner)-> paint_region);
		XOffsetRegion( X(owner)-> paint_region, -X(owner)-> btransform.x, X(owner)-> btransform.y);
		XSetRegion( DISP, X(owner)-> gc, region);
		X(owner)-> current_region = region;
		X(owner)-> flags. kill_current_region = 1;
		CWidget( owner)-> notify( owner, "sH", "Paint", owner);
		X(owner)-> gdrawable = dc;
		CWidget( owner)-> end_paint( owner);
	}

	XX-> flags. force_flush = !inside_on_paint;

	return true;
}

Bool
apc_widget_begin_paint_info( Handle self)
{
	prima_no_cursor( self);
	prima_prepare_drawable_for_painting( self, false);
	return true;
}

Bool
apc_widget_destroy( Handle self)
{
	DEFXX;
	ConfigureEventPair *n1, *n2;

	if ( guts.xdndr_last_target == self )
		guts.xdndr_last_target = NULL_HANDLE;

	if ( XX-> recreateData) free( XX-> recreateData);

	n1 = TAILQ_FIRST( &XX-> configure_pairs);
	while (n1 != NULL) {
		n2 = TAILQ_NEXT(n1, link);
		free(n1);
		n1 = n2;
	}

	if ( XX-> user_pointer.cursor != None) {
		XFreeCursor( DISP, XX-> user_pointer.cursor);
		XX-> user_pointer.cursor = None;
	}
	if ( XX-> user_pointer.xor != None) {
		XFreePixmap( DISP, XX-> user_pointer.xor);
		XX-> user_pointer.xor = None;
	}
	if ( XX-> user_pointer.and != None) {
		XFreePixmap( DISP, XX-> user_pointer.and);
		XX-> user_pointer.and = None;
	}
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	if ( XX-> user_pointer.xcursor != NULL) {
		XcursorImageDestroy(XX-> user_pointer.xcursor);
		XX-> user_pointer.xcursor = NULL;
	}
#endif
	if ( guts. currentMenu && PComponent( guts. currentMenu)-> owner == self)
		prima_end_menu();
	if ( guts. focused == self)
		guts. focused = NULL_HANDLE;
	XX-> flags.modal = false;
	if ( XX-> flags. paint_pending) {
		TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
		XX-> flags. paint_pending = false;
	}
	if ( XX-> invalid_region) {
		XDestroyRegion( XX-> invalid_region);
		XX-> invalid_region = NULL;
	}

	if ( XX-> flags. dnd_aware )
		apc_dnd_set_aware( self, false );
	if ( guts. xdndr_widget == self )
		guts. xdndr_widget = NULL_HANDLE;
	if ( guts. xdndr_receiver == self )
		guts. xdndr_receiver = NULL_HANDLE;

	if ( X_WINDOW) {
		if ( guts. grab_redirect == XX-> client || guts. grab_redirect == X_WINDOW)
			guts. grab_redirect = NULL_HANDLE;
		if ( guts. grab_widget == self || XX-> flags. grab) {
			XUngrabPointer( DISP, CurrentTime);
			guts. grab_widget = NULL_HANDLE;
		}
		XCHECKPOINT;
		if ( XX-> client != X_WINDOW) {
			XDestroyWindow( DISP, XX-> client);
			hash_delete( guts.windows, (void*)&XX-> client, sizeof(X_WINDOW), false);
		}
		XX-> client = NULL_HANDLE;
		XDestroyWindow( DISP, X_WINDOW);
		XCHECKPOINT;
		hash_delete( guts.windows, (void*)&X_WINDOW, sizeof(X_WINDOW), false);
		X_WINDOW = NULL_HANDLE;
	}
	XFlush( DISP);
	return true;
}

PFont
apc_widget_default_font( PFont f)
{
	memcpy( f, &guts. default_widget_font, sizeof( Font));
	return f;
}

Bool
apc_widget_end_paint( Handle self)
{
	DEFXX;
	XX-> flags. force_flush = 0;

	/* make the unintended layered window opaque */
	if ( !XX-> flags. layered_requested && XF_LAYERED(XX) && XX-> gc) {
		XGCValues gcv;
		Point sz;
		gcv. foreground = 0xFFFFFFFF;
		gcv. function   = GXcopy;
		gcv. fill_style = FillSolid;
		gcv. plane_mask = guts. argb_bits. alpha_mask;
		XChangeGC( DISP, XX->gc, GCPlaneMask|GCForeground|GCFunction|GCFillStyle, &gcv);
		sz = apc_widget_get_size( self);
		XFillRectangle( DISP, XX-> gdrawable, XX-> gc, 0, 0, sz.x, sz.y);
		gcv. plane_mask = 0xFFFFFFFF;
		XChangeGC( DISP, XX->gc, GCPlaneMask, &gcv);
	}

	DELETE_ARGB_PICTURE(XX->argb_picture);
	prima_cleanup_drawable_after_painting( self);
	prima_update_cursor( self);
	return true;
}

Bool
apc_widget_end_paint_info( Handle self)
{
	prima_cleanup_drawable_after_painting( self);
	prima_update_cursor( self);
	return true;
}

Bool
apc_widget_get_clip_by_children( Handle self)
{
	return X(self)->flags. clip_by_children;
}

Bool
apc_widget_get_clip_owner( Handle self)
{
	return X(self)-> flags. clip_owner;
}

Color
apc_widget_get_color( Handle self, int index)
{
	return X(self)-> colors[ index];
}

Bool
apc_widget_get_first_click( Handle self)
{
	return X(self)-> flags. first_click ? true : false;
}

Handle
apc_widget_get_focused( void)
{
	return guts. focused;
}

ApiHandle
apc_widget_get_handle( Handle self)
{
	return X_WINDOW;
}

Rect
apc_widget_get_invalid_rect( Handle self)
{
	DEFXX;
	Rect ret;
	XRectangle r;
	if ( !XX-> invalid_region) {
		Rect r = {0,0,0,0};
		return r;
	}
	XClipBox( XX-> invalid_region, &r);
	ret. left = r.x;
	ret. bottom = XX-> size.y - r.height - r.y;
	ret. right = r.x + r.width;
	ret. top =  XX-> size.y - r.y;
	return ret;
}

Bool
apc_widget_get_layered_request( Handle self)
{
	return X(self)-> flags. layered_requested;
}

Bool
apc_widget_surface_is_layered( Handle self)
{
	return X(self)-> flags. layered;
}

Point
apc_widget_get_pos( Handle self)
{
	DEFXX;
	XWindow r;
	Point ret;
	int x, y;
	unsigned int w, h, d, b;

	if ( XX-> type. window) {
		Rect rc;
		Point p = apc_window_get_client_pos( self);
		prima_get_frame_info( self, &rc);
		p. x -= rc. left;
		p. y -= rc. bottom;
		return p;
	}

	if ( XX-> parentHandle == NULL_HANDLE)
		return XX-> origin;

	XGetGeometry( DISP, X_WINDOW, &r, &x, &y, &w, &h, &b, &d);
	XTranslateCoordinates( DISP, XX-> parentHandle, guts. root, x, y, &x, &y, &r);
	ret. x = x;
	ret. y = DisplayHeight( DISP, SCREEN) - y - w;
	return ret;
}

Bool
apc_widget_get_shape( Handle self, Handle mask)
{
#ifndef HAVE_X11_EXTENSIONS_SHAPE_H
	return false;
#else
	DEFXX;
	XRectangle *r, *rc;
	int i, count, ordering, aperture;
	Region rgn;

	if ( !guts. shape_extension) return false;

	if ( !mask)
		return XX-> shape_extent. x != 0 && XX-> shape_extent. y != 0;

	if ( XX-> shape_extent. x == 0 || XX-> shape_extent. y == 0)
		return false;

	r = rc = XShapeGetRectangles( DISP, X_WINDOW, ShapeBounding, &count, &ordering);

	rgn = GET_REGION(mask)-> region;
	for ( i = aperture = 0; i < count; i++, r++) {
		int h = r-> y + r-> height;
		if ( aperture < h ) aperture = h;
		XUnionRectWithRegion( r, rgn, rgn);
	}
	GET_REGION(mask)-> aperture = aperture;
	XFree( rc);
	return true;
#endif
}

Point
apc_widget_get_size( Handle self)
{
	DEFXX;
	if ( XX-> type. window) {
		Rect rc;
		Point p = apc_window_get_client_size( self);
		prima_get_frame_info( self, &rc);
		p. x += rc. left + rc. right;
		p. y += rc. bottom + rc. top;
		return p;
	}

	return XX-> size;
}

Bool
apc_widget_get_sync_paint( Handle self)
{
	return X(self)-> flags. sync_paint;
}

Bool
apc_widget_get_transparent( Handle self)
{
	return X(self)-> flags. transparent;
}

Bool
apc_widget_is_captured( Handle self)
{
	return X(self)-> flags. grab ? true : false;
}

Bool
apc_widget_is_enabled( Handle self)
{
	return XF_ENABLED(X(self)) ? true : false;
}

Bool
apc_widget_is_exposed( Handle self)
{
	return X(self)-> flags. exposed ? true : false;
}

Bool
apc_widget_is_focused( Handle self)
{
	return guts. focused == self;
}

Bool
apc_widget_is_responsive( Handle self)
{
	Bool ena = true;
	while ( ena && self != application) {
		ena  = XF_ENABLED(X(self)) ? true : false;
		self = PWidget(self)-> owner;
	}
	return ena;
}

Bool
apc_widget_is_showing( Handle self)
{
	XWindowAttributes attrs;
	DEFXX;

	if ( XX
		&& XGetWindowAttributes( DISP, XX->udrawable, &attrs)
		&& attrs. map_state == IsViewable)
		return true;
	else
		return false;
}

Bool
apc_widget_is_visible( Handle self)
{
	return X(self)-> flags. want_visible ? true : false;
}

Bool
apc_widget_invalidate_rect( Handle self, Rect *rect)
{
	XRectangle r;
	DEFXX;

	if ( rect) {
		SORT( rect-> left,   rect-> right);
		SORT( rect-> bottom, rect-> top);
		r. x = rect-> left;
		r. width = rect-> right - rect-> left;
		r. y = XX-> size. y - rect-> top;
		r. height = rect-> top - rect-> bottom;
	} else {
		r. x = 0;
		r. width = XX-> size. x;
		r. y = 0;
		r. height = XX-> size. y;
	}

	if ( !XX-> invalid_region) {
		XX-> invalid_region = XCreateRegion();
		if ( !XX-> flags. paint_pending) {
			TAILQ_INSERT_TAIL( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = true;
		}
	}

	XUnionRectWithRegion( &r, XX-> invalid_region, XX-> invalid_region);
	if ( XX-> flags. sync_paint) {
		apc_widget_update( self);
	}
	process_transparents( self);
	return true;
}

static Bool
scroll( Handle owner, Handle self, Point * delta)
{
	DEFXX;
	if ( XX-> flags. clip_owner)
		apc_widget_set_pos( self, XX-> origin. x + delta-> x, XX-> origin. y + delta-> y);
	return 0;
}

int
apc_widget_scroll( Handle self, int horiz, int vert,
	Rect *confine, Rect *clip, Bool withChildren)
{
	DEFXX;
	int src_x, src_y, w, h, dst_x, dst_y, iw, ih;
	XRectangle r;
	Region invalid, reg;

	prima_no_cursor( self);
	prima_get_gc( XX);
	XX-> gcv. clip_mask = None;
	XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &XX-> gcv);
	XCHECKPOINT;

	if ( confine) {
		SORT( confine-> left,   confine-> right);
		SORT( confine-> bottom, confine-> top);
		src_x = confine-> left;
		src_y = XX-> size. y - confine-> top;
		w = confine-> right - src_x;
		h = confine-> top - confine-> bottom;
	} else {
		src_x = 0;
		src_y = 0;
		w = XX-> size. x;
		h = XX-> size. y;
	}

	dst_x = src_x + horiz;
	dst_y = src_y - vert;
	iw = w;
	ih = h;

	if (clip) {
		XRectangle cpa;

		SORT( clip-> left,   clip-> right);
		SORT( clip-> bottom, clip-> top);

		r. x = clip-> left;
		r. y = REVERT( clip-> top) + 1;
		r. width = clip-> right - clip-> left;
		r. height = clip-> top - clip-> bottom;
		reg = XCreateRegion();
		XUnionRectWithRegion( &r, reg, reg);
		XSetRegion( DISP, XX-> gc, reg);
		XCHECKPOINT;
		XDestroyRegion( reg);
		cpa. x = src_x;
		cpa. y = src_y;
		cpa. width = w;
		cpa. height = h;
		prima_rect_intersect( &cpa, &r);
		dst_x += -src_x + cpa. x;
		dst_y += -src_y + cpa. y;
		src_x = cpa. x;
		src_y = cpa. y;
		w = cpa. width;
		h = cpa. height;
	}

	if ( src_x < XX-> size. x && src_x + w >= 0 && dst_x < XX-> size. x && dst_x + w >= 0 &&
		src_y < XX-> size. y && src_x + h >= 0 && dst_y < XX-> size. y && dst_y + h >= 0) {
		XGCValues gcv;
		gcv. graphics_exposures = true;
		XChangeGC( DISP, XX-> gc, GCGraphicsExposures, &gcv);
		XCopyArea( DISP, XX-> udrawable, XX-> udrawable, XX-> gc,
		      src_x, src_y, w, h, dst_x, dst_y);
		gcv. graphics_exposures = false;
		XChangeGC( DISP, XX-> gc, GCGraphicsExposures, &gcv);
	}
	prima_release_gc( XX);
	XCHECKPOINT;
	XFlush( DISP);

	r. x = src_x;
	r. y = src_y;
	r. width = w;
	r. height = h;
	invalid = XCreateRegion();
	if ( src_x < XX-> size. x && src_x + w >= 0 &&
		src_y < XX-> size. y && src_y + h >= 0)
		XUnionRectWithRegion( &r, invalid, invalid);
	if ( clip &&
		dst_x < XX-> size. x && dst_x + iw >= 0 &&
		dst_y < XX-> size. y && dst_y + ih >= 0) {
		XRectangle cpa;
		cpa. x = dst_x;
		cpa. y = dst_y;
		cpa. width = w;
		cpa. height = h;
		XUnionRectWithRegion( &cpa, invalid, invalid);
	}

	if ( XX-> invalid_region) {
		reg = XCreateRegion();
		XUnionRegion( XX-> invalid_region, reg, reg);
		XIntersectRegion( reg, invalid, reg);
		XSubtractRegion( XX-> invalid_region, reg, XX-> invalid_region);
		XOffsetRegion( reg, horiz, -vert);
		XUnionRegion( XX-> invalid_region, reg, XX-> invalid_region);
		XDestroyRegion( reg);
	} else
		XX-> invalid_region = XCreateRegion();

	if ( dst_x < XX-> size. x && dst_x + w >= 0 &&
		dst_y < XX-> size. y && dst_y + h >= 0) {
		r. x = dst_x;
		r. y = dst_y;
		reg = XCreateRegion();
		XUnionRectWithRegion( &r, reg, reg);
		XSubtractRegion( invalid, reg, invalid);
		XDestroyRegion( reg);
	}
	XUnionRegion( XX-> invalid_region, invalid, XX-> invalid_region);
	XDestroyRegion( invalid);
	if ( !XX-> flags. paint_pending) {
		TAILQ_INSERT_TAIL( &guts.paintq, XX, paintq_link);
		XX-> flags. paint_pending = true;
	}

	if ( withChildren) {
		Point delta;
		delta. x = horiz;
		delta. y = vert;
		CWidget(self)-> first_that( self, (void*)scroll, &delta);
	}

	process_transparents( self);

	return scrExpose;
}

Bool
apc_widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
	int r;
	XWindow confine_to = None;
	DEFXX;

	if ( capture) {
		XWindow z = XX-> client;
		Time t = guts. last_time;
		Cursor cursor = prima_get_cursor(self);
		if ( confineTo && PWidget(confineTo)-> handle)
			confine_to = PWidget(confineTo)-> handle;
AGAIN:
		r = XGrabPointer( DISP, z, false, 0
			| ButtonPressMask
			| ButtonReleaseMask
			| PointerMotionMask
			| ButtonMotionMask, GrabModeAsync, GrabModeAsync,
			confine_to, cursor, t
		);
		XCHECKPOINT;
		if ( r != GrabSuccess) {
			XWindow root = guts. root, rx;
			if (( r == GrabNotViewable) && ( root != z)) {
				XTranslateCoordinates( DISP, z, guts. root, 0, 0,
					&guts. grab_translate_mouse.x, &guts. grab_translate_mouse.y, &rx);
				guts. grab_redirect = z;
				guts. grab_widget = self;
				z = root;
				goto AGAIN;
			}
			if ( r == GrabInvalidTime) {
				t = CurrentTime;
				goto AGAIN;
			}
			guts. grab_redirect = NULL_HANDLE;
			return false;
		} else {
			XX-> flags. grab   = true;
			guts. grab_widget  = self;
			guts. grab_confine = confineTo;
		}
	} else if ( XX-> flags. grab) {
		guts. grab_redirect = NULL_HANDLE;
		XUngrabPointer( DISP, CurrentTime);
		XCHECKPOINT;
		XX-> flags. grab = false;
		guts. grab_widget = NULL_HANDLE;
	}
	XFlush( DISP);
	return true;
}

Bool
apc_widget_set_clip_by_children( Handle self, Bool clip_by_children)
{
	DEFXX;
	XX->flags. clip_by_children = clip_by_children;
	if ( XF_IN_PAINT(XX) ) {
		XX-> gcv. subwindow_mode = (XX->flags.clip_by_children ? ClipByChildren : IncludeInferiors);
		XChangeGC( DISP, XX-> gc, GCSubwindowMode, &XX-> gcv);
	}
	return true;
}

Bool
apc_widget_set_color( Handle self, Color color, int i)
{
	Event e = {cmColorChanged};

	X(self)-> colors[ i] = color;
	if ( i == ciFore)
		apc_gp_set_color( self, color);
	else if ( i == ciBack)
		apc_gp_set_back_color( self, color);

	bzero( &e, sizeof(e));
	e. gen. source = self;
	e. gen. i      = i;
	apc_message( self, &e, false);

	return true;
}

Bool
apc_widget_set_enabled( Handle self, Bool enable)
{
	DEFXX;

	if ( enable == XF_ENABLED(XX)) return true;
	XF_ENABLED(XX) = enable;
	prima_simple_message(self, enable ? cmEnable : cmDisable, false);
	return true;
}

Bool
apc_widget_set_first_click( Handle self, Bool firstClick)
{
	X(self)-> flags. first_click = firstClick ? 1 : 0;
	return true;
}

static int
flush_refocus( Display * disp, XEvent * ev, void * dummy)
{
	return ev-> type == ClientMessage &&
			ev-> xclient. message_type == WM_PROTOCOLS &&
		(Atom) ev-> xclient. data. l[0] == WM_TAKE_FOCUS;
}

Bool
apc_widget_set_focused( Handle self)
{
	int rev;
	XWindow focus = None, xfoc;
	XEvent ev;
	if ( guts. message_boxes) return false;
	if ( self && ( self != CApplication( application)-> map_focus( application, self)))
		return false;
	if ( self) {
		if (XT_IS_WINDOW(X(self))) return true; /* already done in activate() */
		focus = X_WINDOW;
	}
	XGetInputFocus( DISP, &xfoc, &rev);
	if ( xfoc == focus) return true;

	{ /* code for no-wm environment */
		Handle who = ( Handle) hash_fetch( guts.windows, (void*)&xfoc, sizeof(xfoc)), x = self;
		while ( who && XT_IS_WINDOW(X(who))) who = PComponent( who)-> owner;
		while ( x && !XT_IS_WINDOW(X(x)) && X(x)->flags.clip_owner) x = PComponent( x)-> owner;
		if ( x && x != application && x != who && XT_IS_WINDOW(X(x)))
			XSetInputFocus( DISP, PComponent(x)-> handle, RevertToNone, guts. currentFocusTime);
	}

	XSetInputFocus( DISP, focus, RevertToParent, guts. currentFocusTime);
	XCHECKPOINT;

	XSync( DISP, false);
	while ( XCheckMaskEvent( DISP, FocusChangeMask|ExposureMask, &ev))
		prima_handle_event( &ev, NULL);
	while ( XCheckIfEvent( DISP, &ev, (XIfEventProcType)flush_refocus, (XPointer)0));
	return true;
}

Bool
apc_widget_set_font( Handle self, PFont font)
{
	apc_gp_set_font( self, font);
	prima_simple_message( self, cmFontChanged, false);
	return true;
}

Bool
apc_widget_set_palette( Handle self)
{
	return prima_palette_replace( self, false);
}

Bool
apc_widget_set_pos( Handle self, int x, int y)
{
	DEFXX;
	Event e;
	if ( XX-> type. window) {
		Rect rc;
		prima_get_frame_info( self, &rc);
		return apc_window_set_client_pos( self, x + rc. left, y + rc. bottom);
	}

	if ( XX-> parentHandle == NULL_HANDLE && x == XX-> origin.x && y == XX-> origin. y)
		return true;
	if ( XX-> client == guts. grab_redirect) {
		XWindow rx;
		XTranslateCoordinates( DISP, XX-> client, guts. root, 0, 0,
			&guts. grab_translate_mouse.x, &guts. grab_translate_mouse.y, &rx);
	}
	bzero( &e, sizeof( e));
	e. cmd = cmMove;
	e. gen. source = self;
	XX-> origin. x = e. gen. P. x = x;
	XX-> origin. y = e. gen. P. y = y;
	y = X(XX-> owner)-> size. y - XX-> size.y - y;
	if ( XX-> parentHandle) {
		XWindow cld;
		XTranslateCoordinates( DISP, PWidget(XX-> owner)-> handle, XX-> parentHandle, x, y, &x, &y, &cld);
	}
	XMoveWindow( DISP, X_WINDOW, x, y);
	XCHECKPOINT;
	apc_message( self, &e, false);
	if ( PObject( self)-> stage == csDead) return false;
	if ( XX-> flags. transparent)
		apc_widget_invalidate_rect( self, NULL);
	return true;
}

Bool
apc_widget_set_shape( Handle self, Handle mask)
{
#ifndef HAVE_X11_EXTENSIONS_SHAPE_H
	return false;
#else
	DEFXX;
	XRectangle xr;

	if ( !guts. shape_extension) return false;

	if ( !mask) {
		if ( XX-> shape_extent. x == 0 || XX-> shape_extent. y == 0) return true;
		XShapeCombineMask( DISP, X_WINDOW, ShapeBounding, 0, 0, None, ShapeSet);
		if ( X_WINDOW != XX-> client)
			XShapeCombineMask( DISP, XX-> client, ShapeBounding, 0, 0, None, ShapeSet);
		XX-> shape_extent. x = XX-> shape_extent. y = 0;
		return true;
	}

	XShapeCombineRegion( DISP, X_WINDOW, ShapeBounding, 0, XX->size.y - GET_REGION(mask)->aperture + XX->menuHeight, GET_REGION(mask)->region, ShapeSet);
	if ( XX-> menuHeight > 0 ) {
	/*
		XXX This static shape approach doesn't work when menuHeight is dynamically changed.
			Need to implement something more elaborated.
	*/
		xr. x = 0;
		xr. y = 0;
		xr. width  = XX->size.x;
		xr. height = XX->menuHeight;
		XShapeCombineRectangles( DISP, X_WINDOW, ShapeBounding, 0, 0, &xr, 1, ShapeUnion, 0);
	}
	XClipBox( GET_REGION(mask)->region, &xr);
	XX-> shape_extent. x = xr. x + xr. width;
	XX-> shape_extent. y = GET_REGION(mask)->aperture;
	XX-> shape_offset. x = 0;
	XX-> shape_offset. y = XX-> menuHeight;
	return true;
#endif
}

/* Used instead of XUnmapWindow sometimes because when a focused
	widget gets hidden, the X server's revert_to is sometimes
	weirdly set to RevertToPointerRoot ( mwm is the guilty one ) */
static void
apc_XUnmapWindow( Handle self)
{
	Handle z = guts. focused;
	while ( z) {
		if ( z == self) {
			if (PComponent(self)-> owner) {
				z = PComponent(self)-> owner;
				while ( z && !X(z)-> type. window) z = PComponent(z)-> owner;
				if ( z && z != application)
					XSetInputFocus( DISP, PComponent(z)-> handle, RevertToNone, guts. currentFocusTime);
			}
			break;
		}
		z = PComponent(z)-> owner;
	}
	XUnmapWindow( DISP, X_WINDOW);
}

void
prima_send_cmSize( Handle self, Point oldSize)
{
	DEFXX;
	Event e;

	bzero( &e, sizeof(e));
	e. gen. source = self;
	e. cmd = cmSize;
	e. gen. R. left = oldSize. x;
	e. gen. R. bottom = oldSize. y;
	e. gen. P. x = e. gen. R. right = XX-> size. x;
	e. gen. P. y = e. gen. R. top = XX-> size. y;
	{
		int i, y = XX-> size. y, count = PWidget( self)-> widgets. count;
		for ( i = 0; i < count; i++) {
			PWidget child = PWidget( PWidget( self)-> widgets. items[i]);
			if ((( PWidget(child)-> growMode & gmDontCare) == 0) &&
				( !X(child)-> flags. clip_owner || ( child-> owner == application))) {
				XMoveWindow( DISP, child-> handle, X(child)-> origin.x, y - X(child)-> size.y - X(child)-> origin. y);
			}
		}
	}
	apc_message( self, &e, false);
}

Bool
apc_widget_set_size( Handle self, int width, int height)
{
	DEFXX;
	Point sz = XX-> size;
	PWidget widg = PWidget( self);
	int x, y;

	if ( XX-> type. window) {
		Rect rc;
		prima_get_frame_info( self, &rc);
		return apc_window_set_client_size( self, width - rc. left - rc. right, height - rc. bottom - rc. top);
	}

	widg-> virtualSize. x = width;
	widg-> virtualSize. y = height;

	width = ( width >= widg-> sizeMin. x)
			? (( width <= widg-> sizeMax. x)
				? width
				: widg-> sizeMax. x)
			: widg-> sizeMin. x;

	height = ( height >= widg-> sizeMin. y)
			? (( height <= widg-> sizeMax. y)
				? height
				: widg-> sizeMax. y)
			: widg-> sizeMin. y;

	if ( XX-> parentHandle == NULL_HANDLE && XX-> size. x == width && XX-> size. y == height)
		return true;

	XX-> size. x = width;
	XX-> size. y = height;

	x = XX-> origin. x;
	y = X(XX-> owner)-> size. y - XX-> size.y - XX-> origin. y;
	if ( XX-> parentHandle) {
		XWindow cld;
		XTranslateCoordinates( DISP, PWidget(XX-> owner)-> handle, XX-> parentHandle, x, y, &x, &y, &cld);
	}
	if ( width != 0 && height != 0) {
		if ( XX-> client != X_WINDOW)
			XMoveResizeWindow( DISP, XX-> client, 0, XX-> menuHeight, width, height);
		XMoveResizeWindow( DISP, X_WINDOW, x, y, width, height);
		if ( XX-> flags. falsely_hidden) {
			if ( XX-> flags. want_visible) XMapWindow( DISP, X_WINDOW);
			XX-> flags. falsely_hidden = 0;
		}
	} else {
		if ( XX-> flags. want_visible) apc_XUnmapWindow( self);
		if ( XX-> client != X_WINDOW)
			XMoveResizeWindow( DISP, XX-> client, 0, XX-> menuHeight, ( width == 0) ? 1 : width, ( height == 0) ? 1 : height);
		XMoveResizeWindow( DISP, X_WINDOW, x, y, ( width == 0) ? 1 : width, ( height == 0) ? 1 : height);
		XX-> flags. falsely_hidden = 1;
	}
	prima_send_cmSize( self, sz);
	if ( PObject( self)-> stage == csDead) return false;
	return true;
}

Bool
apc_widget_set_rect( Handle self, int x, int y, int width, int height)
{
	DEFXX;
	Point sz = XX-> size;
	PWidget widg = PWidget( self);
	Event e;

	if ( XX-> type. window) {
		Rect rc;
		prima_get_frame_info( self, &rc);
		return apc_window_set_client_rect( self, x + rc. left, y + rc. bottom,
			width - rc. left - rc. right, height - rc. bottom - rc. top);
	}

	widg-> virtualSize. x = width;
	widg-> virtualSize. y = height;

	width = ( width >= widg-> sizeMin. x)
			? (( width <= widg-> sizeMax. x)
				? width
				: widg-> sizeMax. x)
			: widg-> sizeMin. x;

	height = ( height >= widg-> sizeMin. y)
			? (( height <= widg-> sizeMax. y)
				? height
				: widg-> sizeMax. y)
			: widg-> sizeMin. y;

	if ( XX-> parentHandle == NULL_HANDLE &&
		XX-> size. x == width && XX-> size. y == height &&
		x == XX-> origin.x && y == XX-> origin. y)
		return true;

	if ( XX-> client == guts. grab_redirect) {
		XWindow rx;
		XTranslateCoordinates( DISP, XX-> client, guts. root, 0, 0,
			&guts. grab_translate_mouse.x, &guts. grab_translate_mouse.y, &rx);
	}

	XX-> size. x = width;
	XX-> size. y = height;

	bzero( &e, sizeof( e));
	e. cmd = cmMove;
	e. gen. source = self;
	XX-> origin. x = e. gen. P. x = x;
	XX-> origin. y = e. gen. P. y = y;
	y = X(XX-> owner)-> size. y - height - XX-> origin. y;
	if ( XX-> parentHandle) {
		XWindow cld;
		XTranslateCoordinates( DISP, PWidget(XX-> owner)-> handle, XX-> parentHandle, x, y, &x, &y, &cld);
	}
	if ( width != 0 && height != 0) {
		if ( XX-> client != X_WINDOW)
			XMoveResizeWindow( DISP, XX-> client, 0, XX-> menuHeight, width, height);
		XMoveResizeWindow( DISP, X_WINDOW, x, y, width, height);
		if ( XX-> flags. falsely_hidden) {
			if ( XX-> flags. want_visible) XMapWindow( DISP, X_WINDOW);
			XX-> flags. falsely_hidden = 0;
		}
	} else {
		if ( XX-> flags. want_visible) apc_XUnmapWindow( self);
		if ( XX-> client != X_WINDOW)
			XMoveResizeWindow( DISP, XX-> client, 0, XX-> menuHeight, ( width == 0) ? 1 : width, ( height == 0) ? 1 : height);
		XMoveResizeWindow( DISP, X_WINDOW, x, y, ( width == 0) ? 1 : width, ( height == 0) ? 1 : height);
		XX-> flags. falsely_hidden = 1;
	}
	apc_message( self, &e, false);
	if ( PObject( self)-> stage == csDead) return false;
	prima_send_cmSize( self, sz);
	if ( PObject( self)-> stage == csDead) return false;
	if ( XX-> flags. transparent)
		apc_widget_invalidate_rect( self, NULL);
	return true;
}

Bool
apc_widget_set_size_bounds( Handle self, Point min, Point max)
{
	DEFXX;
	if ( XX-> type. window) {
		XSizeHints hints;
		bzero( &hints, sizeof( hints));
		apc_SetWMNormalHints( self, &hints);
	}
	return true;
}

Bool
apc_widget_set_visible( Handle self, Bool show)
{
	DEFXX;
	int oldShow;
	if ( XX-> type. window)
		return apc_window_set_visible( self, show);

	oldShow = XX-> flags. want_visible ? 1 : 0;
	XX-> flags. want_visible = show;
	if ( !XX-> flags. falsely_hidden) {
		if ( show)
			XMapWindow( DISP, X_WINDOW);
		else
			apc_XUnmapWindow( self);
		XCHECKPOINT;
	}
	if ( oldShow != ( show ? 1 : 0))
		prima_simple_message(self, show ? cmShow : cmHide, false);
	return true;
}

Bool
apc_widget_set_z_order( Handle self, Handle behind, Bool top)
{
	XWindow windoze[2];

	/* top does not matter if behind is non-NULL */
	if (behind) {
		windoze[0] = PComponent(behind)->handle;
		windoze[1] = X_WINDOW;
		XRestackWindows( DISP, windoze, 2);
		XCHECKPOINT;
	} else if (top) {
		XRaiseWindow( DISP, X_WINDOW);
		XCHECKPOINT;
	} else {
		XLowerWindow( DISP, X_WINDOW);
		XCHECKPOINT;
	}

	if ( X(self)-> type. window)
		prima_wm_sync( self, ConfigureNotify);
	else
		prima_simple_message( self, cmZOrderChanged, false);
	return true;
}

Bool
apc_widget_update( Handle self)
{
	DEFXX;

	if ( XX-> invalid_region) {
		if ( XX-> flags. paint_pending) {
			TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = false;
		}
		prima_simple_message( self, cmPaint, false);
		XSync(DISP, false);
	}
	return true;
}

Bool
apc_widget_validate_rect( Handle self, Rect rect)
{
	XRectangle r;
	DEFXX;
	Region rgn;

	SORT( rect. left,   rect. right);
	SORT( rect. bottom, rect. top);


	r. x = rect. left;
	r. width = rect. right - rect. left;
	r. y = XX-> size. y - rect. top;
	r. height = rect. top - rect. bottom;

	if ( !XX-> invalid_region)
		return true;

	if ( !( rgn = XCreateRegion()))
		return false;

	XUnionRectWithRegion( &r, rgn, rgn);
	XSubtractRegion( XX-> invalid_region, rgn, XX-> invalid_region);
	XDestroyRegion( rgn);

	if ( XEmptyRegion( XX-> invalid_region)) {
		if ( XX-> flags. paint_pending) {
			TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = false;
		}
		XDestroyRegion( XX-> invalid_region);
		XX-> invalid_region = NULL;
	}
	return true;
}

