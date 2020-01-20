#include "unix/guts.h"
#include "Application.h"

/* ActionList */

static int
xdnd_atom_to_constant( Atom atom )
{
	if ( atom == XdndActionMove)
		return dndMove;
	else if ( atom == XdndActionCopy)
	      return dndCopy;
	else if ( atom == XdndActionLink)
	      return dndLink;
	else if ( atom == XdndActionPrivate)
	      return dndPrivate;
	else if ( atom == XdndActionAsk)
	      return dndAsk;
	return dndUnknown;
}

static int
xdnd_constant_to_atom( int cmd )
{
	switch ( cmd ) {
	case dndCopy   : return XdndActionCopy;
	case dndMove   : return XdndActionMove;
	case dndLink   : return XdndActionLink;
	case dndAsk    : return XdndActionAsk;
	case dndPrivate: return XdndActionPrivate;
	default        : return None;
	}
}

static void
xdnd_fill_ask_actions(PDNDEvent ev)
{
	Atom type;
	int i, n_list, rps, format;
	unsigned long list_size, descr_size;
	unsigned char *list_data, *descr_data;
	char *str, *p;
	PList l;

	descr_data = list_data = NULL;
	l = &ev->actions;
	bzero( l, sizeof(List));
	
	list_size = 0;
	list_data = malloc(0);
	rps = prima_read_property( guts.xdnd_source, XdndActionList, &type, &format, 
		&list_size, &list_data, 0);
	if ( rps != RPS_OK || type != XA_ATOM || format != CF_32)
		goto BAILOUT;
	n_list = list_size / sizeof(long);

	descr_size = 0;
	descr_data = malloc(0);
	rps = prima_read_property( guts.xdnd_source, XdndActionDescription, &type, &format, 
		&descr_size, &descr_data, 0);
	if ( rps != RPS_OK || type != XA_STRING || format != 8)
		goto BAILOUT;

	i = 0;
	p = str = (char*) descr_data;
	while ( i < descr_size && l-> count < n_list ) {
		int index, cmd;
		Atom action;
		if ( *(p++) ) continue;
		index = l-> count / 3;
		action = ((Atom*)(list_data))[index];
		cmd = xdnd_atom_to_constant(action);
		if ( cmd == dndUnknown ) cmd |= index;
		list_add(l, (Handle) cmd);
		list_add(l, (Handle) duplicate_string( str ));
		list_add(l, (Handle) action);
		str = p;
	}

	free(descr_data);
	free(list_data);
	return;

BAILOUT:
	if ( list_data ) free(list_data);
	if ( descr_data ) free(descr_data);
	l-> count = 0;
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

static Bool
handle_xdnd_leave( Handle self)
{
	if (guts.xdnd_disabled) return false;

	if ( guts.xdnd_receiver != self )
		self = guts.xdnd_receiver;
	guts.xdnd_receiver = nilHandle;

	if ( guts.xdnd_clipboard ) {
		C(guts.xdnd_clipboard)-> xdnd = false;
		guts.xdnd_clipboard = nilHandle;
	}

	if ( guts.xdnd_widget ) {
		Event ev = { cmEndDrag };
		ev.dnd.allow = 0;
		ev.dnd.clipboard = nilHandle;
		guts.xdnd_disabled = true;
		CComponent(guts.xdnd_widget)-> message(guts.xdnd_widget, &ev);
		guts.xdnd_disabled = false;
		guts.xdnd_widget = nilHandle;
	}

	return true;
}

static const char *
atom_name(Atom atom)
{
	return atom ? XGetAtomName(DISP,atom) : "None";
}

static Bool
handle_xdnd_enter( Handle self, XEvent* xev)
{
	int i;
	PClipboardSysData CC;

	if (guts.xdnd_disabled) return false;

	/* consistency */
	if ( guts.xdnd_receiver != nilHandle ) {
		handle_xdnd_leave(guts. xdnd_receiver);
		guts. xdnd_receiver = nilHandle;
	}
	if ( !(guts. xdnd_clipboard = (Handle)hash_fetch( guts.clipboards, "XdndSelection", 13))) {
		Cdebug("clipboard XdndSelection not found\n");
		return false;
	}

	guts. xdnd_receiver = self;
	CC = C(guts. xdnd_clipboard);
	CC->xdnd = true;

	/* pre-read available formats */
	guts.xdnd_source  = xev->xclient.data.l[0];
	guts.xdnd_version = xev->xclient.data.l[1] >> 24;
	Cdebug("dnd:enter %08x v%d %d %s %s %s\n", guts.xdnd_source, guts.xdnd_version,
		xev->xclient.data.l[1] & 1,
		atom_name(xev->xclient.data.l[2]), 
		atom_name(xev->xclient.data.l[3]), 
		atom_name(xev->xclient.data.l[4])
	);
	for ( i = 0; i < guts. clipboard_formats_count; i++) {
		prima_detach_xfers( CC, i, true);
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
		rps = prima_read_property( guts.xdnd_source, XdndTypeList, &type, &format, &size, &data, 0);
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

static void
no_drop(Handle self, XEvent* xev)
{
	XCHECKPOINT;
	xdnd_send_message( guts.xdnd_source, XdndStatus, X_WINDOW, 0, 0, 0, None);
}

static Bool
handle_xdnd_position( Handle self, XEvent* xev)
{
	int x, y, action;
	XWindow from, to, child = None;
	Handle h = nilHandle;
	
	/* Cdebug("xdnd:position disabled=%d clip=%x\n",guts.xdnd_disabled,guts.xdnd_clipboard); */
	if (guts.xdnd_disabled || !guts.xdnd_clipboard) {
		no_drop(self,xev);
		return false;
	}

	if ( guts. xdnd_receiver != self ) {
		handle_xdnd_leave(guts. xdnd_receiver);
		no_drop(self,xev);
		return false;
	}
	guts.xdnd_source = xev->xclient.data.l[0];

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
	/* Cdebug("dnd:position old widget %08x, new %08x\n", guts.xdnd_widget, h); */

	/* send enter/leave messages */
	if ( guts. xdnd_widget != h && guts. xdnd_widget != nilHandle ) {
		Event ev = { cmEndDrag };
		ev.dnd.allow = 0;
		ev.dnd.clipboard = nilHandle;
		if ( h )
			protect_object(h);
		guts.xdnd_disabled = true;
		CComponent(guts.xdnd_widget)-> message(guts.xdnd_widget, &ev);
		guts.xdnd_widget = nilHandle;
		guts.xdnd_disabled = false;
		if ( h ) {
			int h_stage = PObject(h)-> stage;
			unprotect_object(h);
			if (h_stage == csDead) {
				no_drop(self,xev);
				return false;
			}
		}
		if ( !guts.xdnd_clipboard ) {
			no_drop(self,xev);
			return false;
		}
	}
	if ( !h ||
		PObject(h)->stage != csNormal ||
		!X(h)->flags.dnd_aware
	) {
		no_drop(self,xev);
		return true;
	}

	if ( guts.xdnd_widget != h ) {
		Event ev = { cmDragDrop };
		ev.dnd.clipboard = guts.xdnd_clipboard;
		guts.xdnd_disabled = true;
		CComponent(h)-> message(h, &ev);
		guts.xdnd_disabled = false;
		if (PObject(h)->stage != csNormal || !guts.xdnd_clipboard) {
			no_drop(self,xev);
			return false;
		}

		guts.xdnd_widget = h;
		bzero(&guts. xdnd_suppress_events_within, sizeof(Box));
		guts. xdnd_last_action = dndCopy;
	}

	/* check position, suppress events if the sender doesn't respect that */
	XCHECKPOINT;
	x = xev->xclient.data.l[2] >> 16;
	y = xev->xclient.data.l[2] & 0xffff;
	XTranslateCoordinates(DISP, guts.root, X(h)->client, x, y, &x, &y, &to);
	y = X(h)->size.y - y - 1;
	/* Cdebug("xdnd:final position %d %d for %08x/%08x\n",x,y,h); */
	action = dndCopy;
	if (guts.xdnd_version > 1)
		 action = xdnd_atom_to_constant(xev->xclient.data.l[4]);
	if (!(
		action == guts.xdnd_last_action &&
		guts.xdnd_suppress_events_within.width > 0 &&
		guts.xdnd_suppress_events_within.height > 0 &&
		(
			x < guts.xdnd_suppress_events_within.x ||
			x > guts.xdnd_suppress_events_within.width + guts.xdnd_suppress_events_within.x ||
			y < guts.xdnd_suppress_events_within.y ||
			y > guts.xdnd_suppress_events_within.height + guts.xdnd_suppress_events_within.y
		)
	)) {
		Event ev = { cmDragOver };
		guts.xdnd_last_action = action;

		ev.dnd.clipboard = guts.xdnd_clipboard;
		ev.dnd.where.x = x;
		ev.dnd.where.y = y;
		ev.dnd.action  = action;
		guts.xdnd_disabled = true;
		CComponent(h)-> message(h, &ev);
		guts.xdnd_disabled = false;

		guts.xdnd_suppress_events_within = ev.dnd.pad;
		guts.xdnd_last_drop_response     = ev.dnd.allow;
		if (ev.dnd.pad.x      < 0)      ev.dnd.pad.x      = 0;
		if (ev.dnd.pad.x      > 0xffff) ev.dnd.pad.x      = 0xffff;
		if (ev.dnd.pad.y      < 0)      ev.dnd.pad.y      = 0;
		if (ev.dnd.pad.y      > 0xffff) ev.dnd.pad.y      = 0xffff;
		if (ev.dnd.pad.width  < 0)      ev.dnd.pad.width  = 0;
		if (ev.dnd.pad.width  > 0xffff) ev.dnd.pad.width  = 0xffff;
		if (ev.dnd.pad.height < 0)      ev.dnd.pad.height = 0;
		if (ev.dnd.pad.height > 0xffff) ev.dnd.pad.height = 0xffff;
	}

	XCHECKPOINT;
	xdnd_send_message(
		guts.xdnd_source, XdndStatus,
		X_WINDOW, guts.xdnd_last_drop_response,
		(guts.xdnd_suppress_events_within.x     << 16 ) | (guts.displaySize.y - guts.xdnd_suppress_events_within.y - 1),
		(guts.xdnd_suppress_events_within.width << 16 ) | guts.xdnd_suppress_events_within.height,
		guts.xdnd_last_drop_response ? xdnd_constant_to_atom( guts.xdnd_last_action) : None
	);

	return true;
}

static Bool
handle_xdnd_drop( Handle self, XEvent* xev)
{
	Event ev;
	Atom action;

	if (guts.xdnd_disabled || !guts.xdnd_clipboard) return false;

	if (
		guts.xdnd_receiver != self ||
		guts.xdnd_widget == nilHandle ||
		guts.xdnd_clipboard == nilHandle
	) {
		handle_xdnd_leave(guts. xdnd_receiver);
		return false;
	}
	
	Cdebug("dnd:drop from %08x\n", guts.xdnd_source);

	ev.cmd = cmEndDrag;
	ev.dnd.allow = 1;
	ev.dnd.action = dndCopy;
	ev.dnd.clipboard = guts.xdnd_clipboard;
	ev.dnd.actions.count = 0;
	guts.xdnd_source = xev->xclient.data.l[0];
	guts.xdnd_timestamp = (guts.xdnd_version >= 1) ? xev-> xclient.data.l[2] : CurrentTime;
	if ( guts.xdnd_last_action == dndAsk ) {
		ev.dnd.action = dndAsk;
		xdnd_fill_ask_actions(&ev.dnd);
	}
	guts.xdnd_disabled = true;
	CComponent(guts.xdnd_widget)-> message(guts.xdnd_widget, &ev);
	guts.xdnd_disabled = false;

	/* cleanup */
	if ( guts.xdnd_clipboard) {
		PClipboardSysData CC = C(guts.xdnd_clipboard);
		CC->xdnd = true;
	}
	guts.xdnd_widget   = nilHandle;
	guts.xdnd_receiver = nilHandle;

	/* respond */
	action = XdndActionCopy;
	if ( ev.dnd.actions.count > 0 ) {
		int i;
		PList l = &ev.dnd.actions;
		for ( i = 0; i < l->count / 3; i+=3) {
			if ( ev.dnd.action == (int) l->items[i])
				action = (Atom) l->items[i+2];
			free((void*) l->items[i+1]);
		}
		list_destroy(l);
	} else 
		action = xdnd_constant_to_atom( ev.dnd.action );

	XCHECKPOINT;
	xdnd_send_message(
		guts.xdnd_source, XdndFinished,
		X_WINDOW, ev.dnd.allow,
		ev.dnd.allow ? action : None,
		0, 0
	);

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
		Cdebug("dnd:leave %08x\n",guts.xdnd_widget);
		return handle_xdnd_leave( self);
	}
	else if ( xev-> xclient. message_type == XdndDrop)
		return handle_xdnd_drop( self, xev);
	return false;
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
		XDeleteProperty( DISP, X_WINDOW, XdndAware); /* XXX check when inside drop event */
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
