#include "unix/guts.h"
#include "Application.h"

/* See specs at https://www.freedesktop.org/wiki/Specifications/XDND/ */

#define dndAsk 0x100

static int
xdnd_atom_to_constant( Atom atom )
{
	if ( atom == XdndActionMove)
		return dndMove;
	else if ( atom == XdndActionCopy)
	      return dndCopy;
	else if ( atom == XdndActionLink)
	      return dndLink;
	else if ( atom == XdndActionAsk)
	      return dndAsk;
	return dndNone;
}

static int
xdnd_constant_to_atom( int cmd )
{
	switch ( cmd ) {
	case dndCopy   : return XdndActionCopy;
	case dndMove   : return XdndActionMove;
	case dndLink   : return XdndActionLink;
	case dndAsk    : return XdndActionAsk;
	default        : return None;
	}
}

static XWindow
query_xdnd_target(XWindow w)
{
	Bool found = false;
	XWindow foo, child;
	unsigned mask;
	int i, bar, nprops;
	Atom *atoms;

	atoms = XListProperties(DISP, w, &nprops);
	for (i = 0; i < nprops; i++)
		if (atoms[i] == XdndAware) {
			found = true;
			break;
		}
	if ( nprops) XFree(atoms);
	if ( found ) return w;

	if ( !XQueryPointer( DISP, w, &foo, &child, &bar, &bar, &bar, &bar, &mask))
		return None;
	if ( child == None )
		return None;
	return query_xdnd_target(child);
}

static int
query_pointer(XWindow * receiver, Point *p)
{
	XWindow foo;
	int bar, x, y, ret;
	unsigned mask;

	XQueryPointer( DISP, guts.root, &foo, &foo, &x, &y, &bar, &bar, &mask);
	ret =
		(( mask & Button1Mask) ? mb1 : 0) |
		(( mask & Button2Mask) ? mb2 : 0) |
		(( mask & Button3Mask) ? mb3 : 0) |
		(( mask & Button4Mask) ? mb4 : 0) |
		(( mask & Button5Mask) ? mb5 : 0) |
		(( mask & Button6Mask) ? mb6 : 0) |
		(( mask & Button7Mask) ? mb7 : 0) |
		(( mask & ShiftMask)   ? kmShift : 0) |
		(( mask & ControlMask) ? kmCtrl  : 0) |
		(( mask & Mod1Mask)    ? kmAlt   : 0);

	if ( p ) {
		p->x = x;
		p->y = y;
	}

	if ( receiver )
		*receiver = query_xdnd_target(guts.root);

	return ret;
}


static int
xdnd_read_ask_actions(void)
{
	Atom type;
	int i, n_list, rps, format, ret = 0;
	unsigned long list_size;
	unsigned char *list_data;

	if ( guts. xdndr_action_list_cache > 0 )
		return guts. xdndr_action_list_cache;

	list_data = NULL;

	list_size = 0;
	list_data = malloc(0);
	rps = prima_read_property( guts.xdndr_source, XdndActionList, &type, &format,
		&list_size, &list_data, 0);
	if ( rps != RPS_OK || type != XA_ATOM || format != CF_32)
		return guts. xdndr_action_list_cache = dndCopy;
	n_list = list_size / sizeof(Atom);

	for ( i = 0; i < n_list; i++) {
		int action = xdnd_atom_to_constant(((Atom*)list_data)[i]);
		if (( action & dndMask ) != 0)
			ret |= action;
	}

	free(list_data);
	return guts. xdndr_action_list_cache = ret;
}

static void
xdnd_send_message( XWindow source, Atom cmd, long l0, long l1, long l2, long l3, long l4 )
{
	XClientMessageEvent m;
	memset(&m, 0, sizeof(m));
	m.type = ClientMessage;
	m.display = DISP;
	m.window = source;
	m.message_type = cmd;
	m.format = 32;
	m.data.l[0] = l0;
	m.data.l[1] = l1;
	m.data.l[2] = l2;
	m.data.l[3] = l3;
	m.data.l[4] = l4;
	XCHECKPOINT;
	XSendEvent(DISP, source, False, NoEventMask, (XEvent*)&m);
	XSync( DISP, false);
	XCHECKPOINT;
}

static void
xdnd_send_message_ev( XWindow w, XEvent* ev)
{
	ev->type = ClientMessage;
	ev->xclient.display = DISP;
	ev->xclient.format = 32;
	ev->xclient.window = w;
	XSendEvent(DISP, w, False, NoEventMask, ev);
	XSync( DISP, false);
	XCHECKPOINT;
}

static Bool
handle_xdnd_leave( Handle self)
{
	if (guts.xdnd_disabled) return false;

	if ( guts.xdndr_receiver != self )
		self = guts.xdndr_receiver;
	guts.xdndr_receiver = nilHandle;
	guts.xdndr_source   = None;

	if ( guts.xdnd_clipboard )
		C(guts.xdnd_clipboard)-> xdnd_receiving = false;

	if ( guts.xdndr_widget ) {
		XWindow dummy;
		Event ev = { cmDragEnd };
		ev.dnd.allow = 0;
		ev.dnd.clipboard = nilHandle;
		ev.dnd.modmap  = query_pointer(NULL,&ev.dnd.where);
		ev.dnd.action  = dndNone;
		XTranslateCoordinates(DISP, guts.root, X(guts.xdndr_widget)->client, 
			ev.dnd.where.x, ev.dnd.where.y,
			&ev.dnd.where.x, &ev.dnd.where.y,
			&dummy);
		guts.xdnd_disabled = true;
		CComponent(guts.xdndr_widget)-> message(guts.xdndr_widget, &ev);
		guts.xdnd_disabled = false;
		guts.xdndr_widget = nilHandle;
	}

	return true;
}


static const char *
atom_name(Atom atom)
{
	return atom ? XGetAtomName(DISP,atom) : "None";
}


static void
update_pointer(Handle self, int dnd)
{
	int pointer = crDragNone;
	switch ( dnd ) {
	case dndNone: pointer = crDragNone; break;
	case dndCopy: pointer = crDragCopy; break;
	case dndMove: pointer = crDragMove; break;
	case dndLink: pointer = crDragLink; break;
	}
	apc_pointer_set_shape(self, pointer);
}

static Bool
handle_xdnd_status( Handle self, XEvent* xev)
{
	int old_response;
	Event ev = { cmDragResponse };

	if ( xev->xclient.data.l[0] != guts.xdnds_target)
		return false;

	guts.xdnds_last_drop_response = xev->xclient.data.l[1] & 1;
	if ( xev->xclient.data.l[1] & 2 ) {
		bzero(&guts.xdnds_suppress_events_within, sizeof(Box));
	} else {
		guts.xdnds_suppress_events_within.x = xev->xclient.data.l[2] >> 16;
		guts.xdnds_suppress_events_within.y = xev->xclient.data.l[2] & 0xffff;
		guts.xdnds_suppress_events_within.width  = xev->xclient.data.l[3] >> 16;
		guts.xdnds_suppress_events_within.height = xev->xclient.data.l[3] & 0xffff;
	}

	ev.dnd.allow = guts.xdnds_last_drop_response;

	old_response = guts. xdnds_last_action_response;
	guts. xdnds_last_action_response = (guts.xdnds_version > 1) ?
		xdnd_atom_to_constant(xev->xclient.data.l[4]) : dndCopy;
	ev.dnd.action = guts.xdnds_last_action_response;
	if ( ev.dnd.action == dndAsk )
		ev.dnd.action = guts.xdnds_last_action;

	if (guts.xdnds_widget) {
		guts.xdnd_disabled = true;
		CComponent(guts.xdnds_widget)-> message(guts.xdnds_widget, &ev);
		guts.xdnd_disabled = false;
	}

	if ( guts.xdnds_default_pointers && old_response != guts.xdnds_last_action_response)
		update_pointer(guts.xdnds_widget, guts.xdnds_last_action_response );

	return true;
}

static Bool
handle_xdnd_finished( Handle self, XEvent* xev)
{
	Cdebug("dnd:finished disabled=%d/%x %x\n",guts.xdnd_disabled,xev->xclient.data.l[0],guts.xdnds_target);
	if ( guts.xdnd_disabled )
		return false;
	if ( xev->xclient.data.l[0] != guts.xdnds_target)
		return false;
	if ( guts. xdnds_version > 4 ) {
		guts.xdnds_last_drop_response = xev->xclient.data.l[1] & 1;
		guts.xdnds_last_action_response = (guts.xdnds_last_drop_response ?
			xdnd_atom_to_constant(xev->xclient.data.l[2]) : dndNone);
	} else {
		guts.xdnds_last_drop_response = 1;
	}
	Cdebug("dnd:finish with %d\n", guts.xdnds_last_action_response);
	guts.xdnds_finished = true;
	return true;
}

static Bool
handle_xdnd_enter( Handle self, XEvent* xev)
{
	int i;
	PClipboardSysData CC;

	if (guts.xdnd_disabled || !guts. xdnd_clipboard) return false;

	/* consistency */
	if ( guts.xdndr_receiver != nilHandle ) {
		handle_xdnd_leave(guts. xdndr_receiver);
		guts. xdndr_receiver = nilHandle;
	}
	CC = C(guts. xdnd_clipboard);
	CC-> xdnd_receiving = true;
	guts. xdndr_receiver = self;
	guts. xdndr_action_list_cache = 0;

	/* pre-read available formats */
	guts.xdndr_source  = xev->xclient.data.l[0];
	guts.xdndr_version = xev->xclient.data.l[1] >> 24;

	if (guts.xdndr_source == guts.xdnds_sender) {
		Cdebug("dnd:enter local\n");
		return true;
	}

	Cdebug("dnd:enter %08x v%d %d %s %s %s\n", guts.xdndr_source, guts.xdndr_version,
		xev->xclient.data.l[1] & 1,
		atom_name(xev->xclient.data.l[2]),
		atom_name(xev->xclient.data.l[3]),
		atom_name(xev->xclient.data.l[4])
	);
	for ( i = 0; i < guts. clipboard_formats_count; i++) {
		prima_detach_xfers( CC, i, true);
		if ( !CC-> xdnd_sending )
			prima_clipboard_kill_item( CC-> internal, i);
		prima_clipboard_kill_item( CC-> external, i);
	}

	/* prefill 1-3 targets as if CF_TARGETS exists */
	if ((xev->xclient.data.l[1] & 1) == 0) {
		int i, size = 0;
		Atom atoms[3];
		for ( i = 2; i <= 4; i++)
			if ( xev->xclient.data.l[i] != None )
				atoms[size++] = xev->xclient.data.l[i];
		if ( !( CC-> external[cfTargets].data = malloc(size * sizeof(Atom))))
			return false;
		memcpy( CC-> external[cfTargets].data, &atoms, size * sizeof(Atom));
		CC-> external[cfTargets].size = size * sizeof(Atom);
	} else {
		Atom type;
		int rps, format;
		unsigned long size = 0;
		unsigned char * data = malloc(0);
		rps = prima_read_property( guts.xdndr_source, XdndTypeList, &type, &format, &size, &data, 0);
		if ( rps != RPS_OK ) {
			free(data);
			return false;
		}
		CC-> external[cfTargets].size = size;
		CC-> external[cfTargets].data = data;
		if (pguts->debug & DEBUG_CLIP)  {
			int i;
			Atom * types = (Atom *) data;
			_debug("dnd clipboard formats:\n");
			for ( i = 0; i < size / sizeof(Atom); i++, types++)
				_debug("%d:%x %s\n", i, *types, XGetAtomName(DISP, *types));
		}
	}
	CC-> external[cfTargets].name = CF_TARGETS;
	prima_clipboard_query_targets(guts. xdnd_clipboard);

	return true;
}

static Bool
handle_xdnd_position( Handle self, XEvent* xev)
{
	Box box;
	Bool ret = false;
	int x, y, dx, dy, action, modmap;
	XWindow from, to, child = None;
	Handle h = nilHandle;
	XEvent xr;

	bzero(&xr, sizeof(xr));

	/* Cdebug("xdnd:position disabled=%d\n",guts.xdnd_disabled); */
	if (guts.xdnd_disabled)
		goto FAIL;

	if ( guts. xdndr_receiver != self ) {
		handle_xdnd_leave(guts. xdndr_receiver);
		goto FAIL;
	}
	guts.xdndr_source = xev->xclient.data.l[0];

	/* find target window */
	x = xev->xclient.data.l[2] >> 16;
	y = xev->xclient.data.l[2] & 0xffff;
	XTranslateCoordinates(DISP, guts.root, X(self)->client, x, y, &x, &y, &child);
	from = to = X(self)->client;
	while (XTranslateCoordinates(DISP, from, to, x, y, &x, &y, &child)) {
		if (child) {
			from = to;
			to = child;
		} else if ( to == from)
			to = X(self)->client;

		h = (Handle) hash_fetch( guts.windows, (void*)&to, sizeof(to));
		if (
			!h ||
			h == application ||
			!X(h)->flags.enabled
		)
			break;

		if ( !child )
			break;
	}
	if ( h == application || !X(h)->flags.enabled)
		h = nilHandle;
	XCHECKPOINT;
	/* Cdebug("dnd:position old widget %08x, new %08x\n", guts.xdndr_widget, h); */
	modmap  = query_pointer(NULL,NULL);
	dx = x = xev->xclient.data.l[2] >> 16;
	dy = y = xev->xclient.data.l[2] & 0xffff;
	XTranslateCoordinates(DISP, guts.root, X(h)->client, x, y, &x, &y, &to);
	dx -= x;
	dy -= y;
	y = X(h)->size.y - y - 1;
	/* Cdebug("xdnd:final position %d %d for %08x/%08x\n",x,y,h); */

	/* send enter/leave messages */
	if ( guts. xdndr_widget != h && guts. xdndr_widget != nilHandle ) {
		Event ev = { cmDragEnd };
		ev.dnd.allow = 0;
		ev.dnd.clipboard = nilHandle;
		ev.dnd.modmap  = modmap;
		ev.dnd.where.x = x;
		ev.dnd.where.y = y;
		ev.dnd.action  = dndNone;
		if ( h )
			protect_object(h);
		guts.xdnd_disabled = true;
		CComponent(guts.xdndr_widget)-> message(guts.xdndr_widget, &ev);
		guts.xdndr_widget = nilHandle;
		guts.xdnd_disabled = false;
		if ( h ) {
			int h_stage = PObject(h)-> stage;
			unprotect_object(h);
			if (h_stage == csDead) goto FAIL;
		}
		if ( !guts.xdnd_clipboard ) goto FAIL;
	}
	if ( !h ||
		PObject(h)->stage != csNormal ||
		!X(h)->flags.dnd_aware
	)
		goto FAIL;

	action = dndCopy;
	if (guts.xdndr_version > 1)
		action = xdnd_atom_to_constant(xev->xclient.data.l[4]);

	if ( guts.xdndr_widget != h ) {
		Event ev = { cmDragBegin };
		ev.dnd.clipboard = guts.xdnd_clipboard;
		ev.dnd.modmap  = modmap;
		ev.dnd.where.x = x;
		ev.dnd.where.y = y;
		ev.dnd.action  = action;
		guts.xdnd_disabled = true;
		CComponent(h)-> message(h, &ev);
		guts.xdnd_disabled = false;
		if (PObject(h)->stage != csNormal || !guts.xdnd_clipboard)
			goto FAIL;

		guts.xdndr_widget = h;
		bzero(&guts. xdndr_suppress_events_within, sizeof(Box));
		guts. xdndr_last_action = dndCopy;
	}

	if (!(
		action == guts.xdndr_last_action &&
		guts.xdndr_suppress_events_within.width > 0 &&
		guts.xdndr_suppress_events_within.height > 0 &&
		(
			x < guts.xdndr_suppress_events_within.x ||
			x > guts.xdndr_suppress_events_within.width + guts.xdndr_suppress_events_within.x ||
			y < guts.xdndr_suppress_events_within.y ||
			y > guts.xdndr_suppress_events_within.height + guts.xdndr_suppress_events_within.y
		)
	)) {
		Event ev = { cmDragOver };

		if ( action == dndAsk )
			action = xdnd_read_ask_actions();
		if ( action == dndNone )
			action = dndCopy;

		ev.dnd.clipboard = guts.xdnd_clipboard;
		ev.dnd.where.x = x;
		ev.dnd.where.y = y;
		ev.dnd.action  = action;
		ev.dnd.modmap  = query_pointer(NULL,NULL);
		guts.xdnd_disabled = true;
		CComponent(h)-> message(h, &ev);
		guts.xdnd_disabled = false;

		if (ev.dnd.pad.x < 0) ev.dnd.pad.x = 0;
		if (ev.dnd.pad.y < 0) ev.dnd.pad.y = 0;
		if (ev.dnd.pad.x + ev.dnd.pad.width > X(self)->size.x)
			ev.dnd.pad.width = X(self)->size.x - ev.dnd.pad.x;
		if (ev.dnd.pad.y + ev.dnd.pad.height > X(self)->size.y)
			ev.dnd.pad.height = X(self)->size.y - ev.dnd.pad.y;
		if (ev.dnd.pad.width < 0 || ev.dnd.pad.height < 0)
			bzero(&ev.dnd.pad, sizeof(ev.dnd.pad));

		guts.xdndr_suppress_events_within = ev.dnd.pad;
		guts.xdndr_last_drop_response     = ev.dnd.allow;
		guts.xdndr_last_action            = ev.dnd.action;
	}

	box. x = dx + guts.xdndr_suppress_events_within.x;
	box. y = dy + X(guts.xdndr_widget)->size.y - guts.xdndr_suppress_events_within.y -
		guts.xdndr_suppress_events_within.height - 1;
	box. width  = guts. xdndr_suppress_events_within. width;
	box. height = guts. xdndr_suppress_events_within. height;
	if (box.x      < 0)      box.x      = 0;
	if (box.x      > 0xffff) box.x      = 0xffff;
	if (box.y      < 0)      box.y      = 0;
	if (box.y      > 0xffff) box.y      = 0xffff;
	if (box.width  < 0)      box.width  = 0;
	if (box.width  > 0xffff) box.width  = 0xffff;
	if (box.height < 0)      box.height = 0;
	if (box.height > 0xffff) box.height = 0xffff;

	XCHECKPOINT;
	xr.xclient.data.l[1] = 
		guts.xdndr_last_drop_response |
			((guts.xdndr_suppress_events_within.width > 0 &&
			guts.xdndr_suppress_events_within.height > 0) ? 0 : 2),
	xr.xclient.data.l[2] = (box.x     << 16 ) | box.y;
	xr.xclient.data.l[3] = (box.width << 16 ) | box.height;
	xr.xclient.data.l[4] = guts.xdndr_last_drop_response ? xdnd_constant_to_atom( guts.xdndr_last_action) : None;
	ret = true;

FAIL:
	xr.xclient.data.l[0] = X_WINDOW;
	xr.xclient.message_type = XdndStatus;
	if (guts.xdndr_source == guts.xdnds_sender)
		handle_xdnd_status(guts.xdndr_receiver, &xr);
	else
		xdnd_send_message_ev(guts.xdndr_source, &xr);

	return ret;
}

static Bool
handle_xdnd_drop( Handle self, XEvent* xev)
{
	Event ev;
	Atom action;
	XEvent xr;
	XWindow last_source;

	if (!guts.xdnd_clipboard || guts.xdnd_disabled) return false;

	if (
		guts.xdndr_receiver != self ||
		guts.xdndr_widget == nilHandle
	) {
		handle_xdnd_leave(guts. xdndr_receiver);
		return false;
	}

	Cdebug("dnd:drop from %08x\n", guts.xdndr_source);
	if ( X(guts.xdndr_widget)->flags.dnd_aware) {
		XWindow dummy;
		ev.cmd = cmDragEnd;
		ev.dnd.modmap  = query_pointer(NULL,&ev.dnd.where);
		XTranslateCoordinates(DISP, guts.root, X(guts.xdndr_widget)->client, 
			ev.dnd.where.x, ev.dnd.where.y,
			&ev.dnd.where.x, &ev.dnd.where.y,
			&dummy);
		guts.xdndr_source = xev->xclient.data.l[0];
		guts.xdndr_timestamp = (guts.xdndr_version >= 1) ? xev-> xclient.data.l[2] : CurrentTime;
		ev.dnd.action = guts.xdndr_last_action;
		ev.dnd.allow = guts.xdndr_last_action != dndNone;
		ev.dnd.clipboard = ev.dnd.allow ? guts.xdnd_clipboard : nilHandle;
		guts.xdnd_disabled = true;
		CComponent(guts.xdndr_widget)-> message(guts.xdndr_widget, &ev);
		guts.xdnd_disabled = false;
	} else {
		ev.dnd.allow = 0;
		ev.dnd.action = dndCopy;
	}

	/* cleanup */
	guts.xdndr_widget   = nilHandle;
	last_source = guts.xdndr_source;
	guts.xdndr_source   = None;
	guts.xdndr_receiver = nilHandle;
	if ( guts. xdnd_clipboard )
		C(guts.xdnd_clipboard)->xdnd_receiving = false;

	/* respond */
	if (ev.dnd.allow) {
		if (( ev.dnd.action & dndMask ) == 0) ev.dnd.action = None;
		/* never return a combination, if DragEnd never bothered to ask */
		if ( ev.dnd.action & dndCopy ) ev.dnd.action = dndCopy;
		else if ( ev.dnd.action & dndMove ) ev.dnd.action = dndMove;
		else if ( ev.dnd.action & dndLink ) ev.dnd.action = dndLink;
		else ev.dnd.action = dndNone;
		if ( ev.dnd.action == dndNone ) ev.dnd.allow = false;
		action = xdnd_constant_to_atom( ev.dnd.action );
	}
	XCHECKPOINT;

	bzero(&xr, sizeof(xr));
	xr.xclient.message_type = XdndFinished;
	xr.xclient.data.l[0] = X_WINDOW;
	xr.xclient.data.l[1] = ev.dnd.allow;
	xr.xclient.data.l[2] = ev.dnd.allow ? action : None;
	if (last_source == guts.xdnds_sender)
		handle_xdnd_finished(guts.xdndr_receiver, &xr);
	else
		xdnd_send_message_ev(last_source, &xr);

	return true;
}

Bool
prima_handle_dnd_event( Handle self, XEvent *xev)
{
	if ( xev-> xclient. message_type == XdndEnter)
		return handle_xdnd_enter( self, xev);
	else if ( xev-> xclient. message_type == XdndPosition)
		return handle_xdnd_position( self, xev);
	else if ( xev-> xclient. message_type == XdndLeave) {
		Cdebug("dnd:leave %08x\n",guts.xdndr_widget);
		return handle_xdnd_leave( self);
	}
	else if ( xev-> xclient. message_type == XdndDrop)
		return handle_xdnd_drop( self, xev);
	else if ( xev-> xclient. message_type == XdndStatus)
		return handle_xdnd_status( self, xev);
	else if ( xev-> xclient. message_type == XdndFinished)
		return handle_xdnd_finished( self, xev);
	return false;
}

/* make sure it's synchronous when talking local */
static void
xdnd_send_leave_message(void)
{
	if (guts.xdndr_source == guts.xdnds_sender)
		handle_xdnd_leave( guts.xdndr_receiver );
	else
		xdnd_send_message( guts.xdnds_target, XdndLeave, 0, 0, 0, 0, None);
}

static void
xdnd_send_drop_message(void)
{
	XEvent ev;
	bzero(&ev, sizeof(ev));
	ev.xclient.message_type = XdndDrop;
	ev.xclient.data.l[0] = guts.xdnds_sender;
	ev.xclient.data.l[2] = CurrentTime;
	if (guts.xdndr_source == guts.xdnds_sender)
		handle_xdnd_drop( guts.xdndr_receiver, &ev );
	else
		xdnd_send_message_ev(guts.xdnds_target, &ev);
}

static void
xdnds_send_position_message(Point ptr, int action)
{
	XEvent ev;
	bzero(&ev, sizeof(ev));
	ev.xclient.message_type = XdndPosition;
	ev.xclient.data.l[0] = guts.xdnds_sender;
	ev.xclient.data.l[2] = (ptr.x << 16)|ptr.y;
	ev.xclient.data.l[3] = CurrentTime;
	ev.xclient.data.l[4] = action;
	if (guts.xdndr_source == guts.xdnds_sender)
		handle_xdnd_position( guts.xdndr_receiver, &ev );
	else
		xdnd_send_message_ev(guts.xdnds_target, &ev);
}

static void
xdnds_send_enter_message(int rps, Atom * targets)
{
	Handle local;
	XEvent ev;
	bzero(&ev, sizeof(ev));
	ev.xclient.message_type = XdndEnter;
	ev.xclient.data.l[0] = guts.xdnds_sender;
	ev.xclient.data.l[1] = (((guts.xdnds_version < 5) ? guts.xdnds_version : 5) << 24) | (rps > 3);
	ev.xclient.data.l[2] = (rps > 0) ? targets[0] : None;
	ev.xclient.data.l[3] = (rps > 1) ? targets[1] : None;
	ev.xclient.data.l[4] = (rps > 2) ? targets[2] : None;
	local = ( Handle) hash_fetch( guts. windows, (void*)&(guts.xdnds_target), sizeof(XWindow));
	if (local)
		handle_xdnd_enter( local, &ev );
	else
		xdnd_send_message_ev(guts.xdnds_target, &ev);
}

Bool
apc_dnd_get_aware( Handle self )
{
	return X(self)->flags. dnd_aware;
}

static Bool
find_dnd_aware( Handle self, Handle widget, void * cmd)
{
	if ( X(widget)->flags. dnd_aware) return true;
	return CWidget(widget)->first_that(widget, find_dnd_aware, NULL) != nilHandle;
}

void
prima_update_dnd_aware( Handle self )
{
	DEFXX;
	Bool has_drop_target;
	Bool has_property    = XX->flags. drop_target;

	has_drop_target = XX->flags.dnd_aware ?
		true :
		(CWidget(self)->first_that(self, find_dnd_aware, NULL) != nilHandle);

	if ( has_drop_target == has_property ) return;
	XX->flags. drop_target = has_drop_target;

	if ( has_drop_target ) {
		Atom version = 5;
		XChangeProperty(DISP, X_WINDOW, XdndAware, XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&version, 1);
	} else
		XDeleteProperty( DISP, X_WINDOW, XdndAware);
}

Bool
apc_dnd_set_aware( Handle self, Bool is_target )
{
	DEFXX;
	Bool src = XX->flags. dnd_aware ? 1 : 0;
	Bool dst = is_target ? 1 : 0;
	Handle top_level;

	if ( src == dst ) return true;
	top_level = CApplication(application)-> top_frame( application, self);
	if ( top_level == application ) return false;

	XX->flags. dnd_aware = dst;
	prima_update_dnd_aware( top_level );
	return true;
}

Handle
apc_dnd_get_clipboard( Handle self )
{
	return guts. xdnd_clipboard;
}

static Bool
send_drag_response(Handle self, Bool allow, int action)
{
	Event ev = { cmDragResponse };
	ev.dnd.allow = allow;
	ev.dnd.action = action;
	guts.xdnd_disabled = true;
	CComponent(self)-> message(self, &ev);
	guts.xdnd_disabled = false;
	return PObject(self)->stage == csNormal;
}

int
apc_dnd_start( Handle self, int actions, Bool default_pointers)
{
	Bool got_session = false;
	Point ptr, last_ptr = { -1, -1 };
	int ret = dndNone, i, modmap, first_modmap, n_actions = 0;
	Atom actions_list[3], curr_action, last_action = -1;
	char *ac_ptr, actions_descriptions[16] = ""; /* Copy Move Link */
	PClipboardSysData CC;
	Handle top_level, banned_receiver = None;
	XWindow last_xdndr_source = None;
	int old_pointer;

	if ( guts.xdnd_disabled || guts.xdnds_widget || !guts.xdnd_clipboard ) {
		Cdebug("dnd:already is action\n");
		return -1;
	}
	if ((actions & dndMask) == 0) {
		Cdebug("dnd:bad actions\n");
		return -1;
	}
	top_level = CApplication(application)-> top_frame( application, self);
	if ( top_level == application ) {
		Cdebug("dnd:no toplevel window\n");
		return -1;
	}

	ac_ptr = actions_descriptions;
	for ( i = 0x1; i <= dndLink; i <<= 1) {
		int action = actions & i;
		if ( actions == 0 ) continue;

		actions_list[n_actions++] = xdnd_constant_to_atom(action);
		switch ( action ) {
		case dndCopy: strncpy(ac_ptr, "Copy", 5); break;
		case dndMove: strncpy(ac_ptr, "Move", 5); break;
		case dndLink: strncpy(ac_ptr, "Link", 5); break;
		default     : continue;
		}
		ac_ptr += 5;
	}
	if ( n_actions == 0) {
		Cdebug("dnd:no actions\n");
		return -1;
	}

	if ( prima_clipboard_fill_targets(guts.xdnd_clipboard) == 0) {
		Cdebug("dnd:no clipboard data\n");
		return -1; /* nothing to drag */
	}

	guts. xdnds_default_pointers = default_pointers;
	guts. xdnds_sender = PWidget(top_level)->handle;
	guts. xdnds_widget = self;
	guts. xdnds_finished = false;
	modmap = query_pointer(NULL,NULL);
	first_modmap = modmap & 0xffff;
	guts.xdnds_escape_key = false;
	guts.xdnds_last_drop_response = false;
	guts.xdnds_target = None;
	guts.xdnds_version = 0;
	guts.xdnds_last_action = guts.xdnds_last_action_response = dndNone;
	bzero( &guts.xdnds_suppress_events_within, sizeof(Box));
	protect_object(self);
	Cdebug("dnd:begin\n");

	CC = C(guts.xdnd_clipboard);
	prima_detach_xfers( CC, cfTargets, true);
	prima_clipboard_kill_item( CC-> internal, cfTargets);
	CC->internal[cfTargets].name = XdndTypeList;

	old_pointer = apc_pointer_get_shape(self);
	apc_pointer_set_shape(self, crDragNone);
	apc_widget_set_capture(self, true, nilHandle);

	if ( !default_pointers && !X(self)->flags. dnd_aware) {
		if ( !send_drag_response(self, false, dndNone)) goto EXIT;
	}

	XChangeProperty(DISP, guts. xdnds_sender, XdndActionList, XA_ATOM, 32,
		PropModeReplace, (unsigned char*)&actions_list, n_actions);
	XChangeProperty(DISP, guts. xdnds_sender, XdndActionDescription, XA_STRING, 8,
		PropModeReplace, (unsigned char*)&actions_descriptions, ac_ptr - actions_descriptions);
	XCHECKPOINT;

	guts.xdnds_last_action = actions;
	curr_action = (n_actions > 1) ? XdndActionAsk : actions_list[0];
	while ( prima_one_loop_round( WAIT_ALWAYS, true)) {
		int new_modmap;
		XWindow new_receiver;

		if ( !guts.xdnds_widget || !guts.xdnd_clipboard ) {
			Cdebug("dnd:objects killed\n");
			ret = dndNone;
			goto EXIT;
		}

		new_modmap = query_pointer(&new_receiver, &ptr);
		if ( new_receiver == banned_receiver && banned_receiver != None )
			new_receiver = guts.xdnds_target;

		if ( guts.xdnds_escape_key )
			new_modmap |= kmEscape;
		if ( new_modmap == modmap && new_receiver == guts.xdnds_target && last_ptr.x == ptr.x && last_ptr.y == ptr.y)
			continue;
		last_ptr = ptr;

		if ( new_receiver != guts.xdnds_target ) {
			banned_receiver = None;
			Cdebug("dnd:target %08x => %08x\n", guts.xdnds_target, new_receiver);
			if ( got_session && guts.xdnds_target ) {
				xdnd_send_leave_message();
				guts. xdnds_target = None;
				got_session = false;
			}

			if ( new_receiver ) {
				Atom type, *targets;
				int rps, format;
				unsigned long size = 0;
				unsigned char * data = malloc(0);

				rps = prima_read_property( new_receiver, XdndAware, &type, &format,
					&size, &data, 0);
				if ( rps != RPS_OK || type != XA_ATOM || format != CF_32 || size != sizeof(Atom)) {
					free(data);
					Cdebug("dnd:bad XdndAware\n");
					banned_receiver = new_receiver;
					continue;
				}
				guts.xdnds_version = *((Atom*)data);
				free(data);
				rps = prima_clipboard_fill_targets(guts.xdnd_clipboard);
				if ( rps == 0 ) {
					Cdebug("dnd:failed to query clipboard targets\n");
					ret = -1;
					goto EXIT;
				}

				got_session = true;
				guts.xdnds_target = new_receiver;
				targets = (Atom*) CC->internal[cfTargets].data;
				Cdebug("dnd:send enter to %08x\n",guts.xdnds_target);
				xdnds_send_enter_message(rps, targets);
			}
		}

		if (got_session) {
			Box b = guts.xdnds_suppress_events_within;
			XCHECKPOINT;
			if (!(
				b.width > 0 && b.height > 0 &&
				ptr.x >= b.x && ptr.y >= b.y &&
				ptr.x <= b.x + b.width && ptr.y <= b.y + b.height &&
				curr_action == last_action
			)) {
				xdnds_send_position_message(ptr, curr_action);
				last_action = curr_action;
			}
		} else {
			if ( default_pointers)
				apc_pointer_set_shape(self, crDragNone);
			guts.xdnds_last_drop_response = false;
			guts.xdnds_last_action_response = dndNone;
			if ( !send_drag_response(self, false, dndNone)) goto EXIT;
		}

		if ( new_modmap != modmap) {
			Event ev = { cmDragQuery };
			Cdebug("dnd:modmap %08x => %08x\n", modmap, new_modmap);

			/* suggest allow action */
			ev.dnd.modmap = modmap = new_modmap;
			if ( modmap & kmEscape )
				ev.dnd.allow = -1;
			else if (first_modmap != (modmap & first_modmap))
				ev.dnd.allow = got_session ? 1 : -1;
			else
				ev.dnd.allow = 0;

			guts.xdnd_disabled = true;
			CComponent(guts.xdnds_widget)-> message(guts.xdnds_widget, &ev);
			guts.xdnd_disabled = false;
			guts.xdnds_escape_key = false;
			ev.dnd.action &= dndMask;

			if ( ev.dnd.action != 0 ) {
				last_action = curr_action;
				curr_action = xdnd_constant_to_atom(ev.dnd.action);
				if ( curr_action == None )
					curr_action = XdndActionAsk;
			}
			if ( ev.dnd.allow != 0 ) {
				XCHECKPOINT;
				if (
					got_session &&
					ev.dnd.allow > 0 &&
					guts. xdnds_last_drop_response &&
					guts. xdnds_last_action_response != None
				) {
					last_xdndr_source = guts.xdndr_source;
					xdnd_send_drop_message();
					guts.xdnds_widget = nilHandle;
					ret = guts.xdnds_last_action_response;
					Cdebug("dnd:drop\n");
				} else {
					xdnd_send_leave_message();
					guts.xdnds_widget = nilHandle;
					got_session = false;
					Cdebug("dnd:leave\n");
				}
				break;
			} else {
				xdnds_send_position_message(ptr, curr_action);
			}
		}

		XSync( DISP, false);
	}

	if (ret == dndNone) goto EXIT;

	if (guts. xdnds_version < 2 ) {
		if (ret == dndAsk) ret = dndCopy; /* shouldn't happen */
		goto EXIT;
	}

	/* wait for XdndFinished */
	if (last_xdndr_source != guts.xdnds_sender) {
		XCHECKPOINT;
		guts.xdnds_finished = false;
		guts.xdnds_widget = self;
		guts.xdnds_escape_key = false;
		Cdebug("dnd:wait for finalization\n");
		while ( 
			prima_one_loop_round( WAIT_ALWAYS, true) && 
			!guts.xdnds_finished && 
			!guts.xdnds_escape_key
		) {
			XSync( DISP, false);
		}
		Cdebug("dnd:finalize by %s\n", guts.xdnds_escape_key ? "escape" : "event");
		guts.xdnds_widget = nilHandle;
		guts.xdnds_escape_key = false;
	}

	if (guts.xdnds_last_drop_response) {
		ret = guts. xdnds_last_action_response;
		if (ret == dndAsk) ret = dndCopy; /* shouldn't happen */
	} else
		ret = dndNone;

EXIT:
	if ( PObject(self)->stage == csNormal ) {
		apc_widget_set_capture(self, 0, nilHandle);
		apc_pointer_set_shape(self, old_pointer);
	}
	unprotect_object(self);
	guts.xdnds_widget = nilHandle;
	guts.xdnds_target = None;
	Cdebug("dnd:stop\n");
	return ret;
}
