#include "apricot.h"
#include "guts.h"
#include "Application.h"
#include "Popup.h"
#include "Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#define inherited CDrawable

#define enter_method PWidget_vmt selfvmt = ((( PWidget) self)-> self)
#define my  selfvmt
#define var (( PWidget) self)

#define evOK ( var-> evStack[ var-> evPtr - 1])
#define objCheck if ( var-> stage > csNormal) return

extern Bool Widget_size_notify( Handle self, Handle child, const Rect* metrix);
extern Bool Widget_move_notify( Handle self, Handle child, Point * moveTo);
extern void Widget_pack_slaves( Handle self);
extern void Widget_place_slaves( Handle self);


static Bool
dnd_event_wanted(Handle self, PEvent event)
{
	Bool r;
	SV * ret;
	if ( var-> dndAware == NULL) return false;
	if ( strcmp(var->dndAware, "1") == 0) return true;
	ENTER;
	SAVETMPS;
	ret = call_perl( event->dnd.clipboard, "has_format", "<s", var->dndAware);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}

static void
handle_drag_begin( Handle self, PEvent event)
{
	enter_method;
	if ( !dnd_event_wanted(self, event)) {
		opt_clear(optDropSession);
		return;
	}
	opt_set(optDropSession);
	my-> notify( self, "<sHiiPH", "DragBegin",
		event-> dnd. clipboard,
		event-> dnd. action,
		event-> dnd. modmap,
		event-> dnd. where,
		event-> dnd. counterpart
	);
}

static void
handle_drag_over( Handle self, PEvent event)
{
	dPROFILE;
	enter_method;
	HV * profile;
	SV * ref;

	if ( !is_opt(optDropSession)) {
		Point size = apc_widget_get_size(self);
		event-> dnd.allow      = 0;
		event-> dnd.pad.x      = event-> dnd.pad.y = 0;
		event-> dnd.pad.width  = size.x;
		event-> dnd.pad.height = size.y;
		return;
	}

	profile = newHV();
	ref = newRV_noinc((SV*) profile);

	pset_i(allow,1);
	pset_i(action,dndCopy);
	my-> notify( self, "<sHiiPHS", "DragOver",
		event-> dnd. clipboard,
		event-> dnd. action,
		event-> dnd. modmap,
		event-> dnd. where,
		event-> dnd. counterpart,
		ref
	);

	event-> dnd. allow  = pexist(allow)  ? pget_i(allow)  : 1;
	event-> dnd. action = pexist(action) ? pget_i(action) : dndCopy;
	memset( &event-> dnd.pad, 0, sizeof(Rect));
	if ( pexist(pad)) {
		int rect[4];
		prima_read_point( pget_sv(pad), rect, 4, "Array panic on 'pad'");
		event->dnd.pad.x      = rect[0];
		event->dnd.pad.y      = rect[1];
		event->dnd.pad.width  = rect[2];
		event->dnd.pad.height = rect[3];
	}

	sv_free(ref);
}

static void
handle_drag_end( Handle self, PEvent event)
{
	dPROFILE;
	enter_method;
	HV * profile;
	SV * ref;

	if ( !is_opt(optDropSession)) {
		event-> dnd. allow = 0;
		return;
	}
	opt_clear(optDropSession);

	profile = newHV();
	ref = newRV_noinc((SV*) profile);

	pset_i(allow, 1);
	pset_i(action, event->dnd.action);
	my-> notify( self, "<sHiiPHS", "DragEnd", 
		event-> dnd. allow ? event-> dnd.clipboard : NULL_HANDLE, 
		event-> dnd. action,
		event-> dnd. modmap,
		event-> dnd. where,
		event-> dnd. counterpart,
		ref
	);
	event-> dnd. allow  = pexist(allow)  ? pget_i(allow)  : 1;
	event-> dnd. action = pexist(action) ? pget_i(action) : dndCopy;

	sv_free(ref);
}

static void
handle_drag_query( Handle self, PEvent event)
{
	dPROFILE;
	enter_method;
	HV * profile = newHV();
	SV * ref = newRV_noinc((SV*) profile);
	pset_i(allow, event->dnd.allow);
	my-> notify( self, "<siHS", "DragQuery", event->dnd.modmap, event->dnd.counterpart, ref);
	if (pexist(allow))
		event-> dnd.allow = pget_i(allow);
	event-> dnd.action = pexist(action) ? pget_i(action) : 0;
	sv_free(ref);
}

static void
handle_key_down( Handle self, PEvent event)
{
	enter_method;
	int i;
	int rep = event-> key. repeat;
	Handle next = NULL_HANDLE;
	if ( is_opt( optBriefKeys))
		rep = 1;
	else
		event-> key. repeat = 1;
	for ( i = 0; i < rep; i++) {
		my-> notify( self, "<siiii", "KeyDown",
			event-> key.code, event-> key. key, event-> key. mod, event-> key. repeat);
		objCheck;
		if ( evOK) {
			Event ev = *event;
			ev. key. source = self;
			ev. cmd         = var-> owner ? cmDelegateKey : cmTranslateAccel;
			ev. key. subcmd = 0;
			if ( !my-> message( self, &ev)) {
				my-> clear_event( self);
				return;
			}
			objCheck;
		}
		if ( !evOK) break;

		switch( event-> key. key) {
		case kbF1:
		case kbHelp:
			my-> help( self);
			my-> clear_event( self);
			return;
		case kbLeft:
			next = my-> next_positional( self, -1, 0);
			break;
		case kbRight:
			next = my-> next_positional( self, 1, 0);
			break;
		case kbUp:
			next = my-> next_positional( self, 0, 1);
			break;
		case kbDown:
			next = my-> next_positional( self, 0, -1);
			break;
		case kbTab:
			next = my-> next_tab( self, true);
			break;
		case kbBackTab:
			next = my-> next_tab( self, false);
			break;
		default:;
		}
		if ( next) {
			CWidget( next)-> set_selected( next, true);
			objCheck;
			my-> clear_event( self);
			return;
		}
	}
}

Bool
prima_accel_notify ( Handle group, Handle self, PEvent event)
{
	enter_method;
	if (( self != event-> key. source) && my-> get_enabled( self))
		return ( var-> stage <= csNormal) ? !my-> message( self, event) : false;
	else
		return false;
}

static void
handle_delegate_key( Handle self, PEvent event)
{
	enter_method;
	switch ( event-> key. subcmd) {
		case 0: {
			Event ev = *event;
			ev. cmd         = cmTranslateAccel;
			if ( !my-> message( self, &ev)) {
				my-> clear_event( self);
				return;
			}
			objCheck;

			if ( my-> first_that( self, (void*)prima_accel_notify, &ev)) {
				my-> clear_event( self);
				return;
			}
			objCheck;
			ev. cmd         = cmDelegateKey;
			ev. key. subcmd = 1;
			if ( my-> first_that( self, (void*)prima_accel_notify, &ev)) {
				my-> clear_event( self);
				return;
			}
			if ( var-> owner && my->can_propagate_key(self))
			{
				if ( var-> owner == prima_guts.application)
					ev. cmd = cmTranslateAccel;
				else
					ev. key. subcmd = 0;
				ev. key. source = self;
				if (!(((( PWidget) var-> owner)-> self)-> message( var-> owner, &ev))) {
					objCheck;
					my-> clear_event( self);
					return;
				}
			}
		}
		break;
		case 1: {
			Event ev = *event;
			ev. cmd  = cmTranslateAccel;
			if ( my-> first_that( self, (void*)prima_accel_notify, &ev)) {
				my-> clear_event( self);
				return;
			}
			objCheck;
			ev = *event;
			if ( my-> first_that( self, (void*)prima_accel_notify, &ev)) {
				my-> clear_event( self);
				return;
			}
		}
		break;
	}
}

static Bool
find_dup_msg( PEvent event, int * cmd)
{
	return event-> cmd == *cmd;
}

static void
handle_move( Handle self, PEvent event)
{
	enter_method;
	Bool doNotify = false;
	Point oldP;
	if ( var-> stage == csNormal && var-> evQueue == NULL) {
		doNotify = true;
	} else if ( var-> stage > csNormal) {
		return;
	} else if ( var-> evQueue != NULL) {
		int i = list_first_that( var-> evQueue, (void*)find_dup_msg, &event-> cmd);
		PEvent n;
		if ( i < 0) {
			if ( !( n = alloc1( Event))) goto MOVE_EVENT;
			memcpy( n, event, sizeof( Event));
			n-> gen. B = 1;
			n-> gen. R. left = n-> gen. R. bottom = 0;
			list_add( var-> evQueue, ( Handle) n);
		} else
			n = ( PEvent) list_at( var-> evQueue, i);
		n-> gen. P = event-> gen. P;
	}
MOVE_EVENT:;
	if ( !event-> gen. B)
		my-> first_that( self, (void*) Widget_move_notify, &event-> gen. P);
	if ( doNotify) oldP = var-> pos;
	var-> pos = event-> gen. P;
	if ( doNotify &&
		(oldP. x != event-> gen. P. x ||
			oldP. y != event-> gen. P. y)) {
		my-> notify( self, "<sPP", "Move", oldP, event-> gen. P);
		objCheck;
		if ( var->geometry == gtGrowMode && var-> growMode & gmCenter)
			my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
	}
}

static void
handle_popup( Handle self, PEvent event)
{
	enter_method;
	Handle org = self;
	my-> notify( self, "<siP", "Popup", event-> gen. B, event-> gen. P. x, event-> gen. P. y);
	objCheck;
	if ( evOK) {
		while ( self) {
			PPopup p = ( PPopup) CWidget( self)-> get_popup( self);
			if ( p && p-> self-> get_autoPopup(( Handle) p)) {
				Point px = event-> gen. P;
				apc_widget_map_points( org,  true,  1, &px);
				apc_widget_map_points( self, false, 1, &px);
				p-> self-> popup(( Handle) p, px. x, px. y ,0,0,0,0);
				CWidget( org)-> clear_event( org);
				return;
			}
			self = var-> owner;
		}
	}
}

static void
handle_size( Handle self, PEvent event)
{
	enter_method;
	/* expecting new size in P, old & new size in R. */
	Bool doNotify = false;
	if ( var-> stage == csNormal && var-> evQueue == NULL) {
		doNotify = true;
	} else if ( var-> stage > csNormal) {
		return;
	} else if ( var-> evQueue != NULL) {
		int i = list_first_that( var-> evQueue, (void*)find_dup_msg, &event-> cmd);
		PEvent n;
		if ( i < 0) {
			if ( !( n = alloc1( Event))) goto SIZE_EVENT;
			memcpy( n, event, sizeof( Event));
			n-> gen. B = 1;
			n-> gen. R. left = n-> gen. R. bottom = 0;
			list_add( var-> evQueue, ( Handle) n);
		} else
			n = ( PEvent) list_at( var-> evQueue, i);
		n-> gen. P. x = n-> gen. R. right  = event-> gen. P. x;
		n-> gen. P. y = n-> gen. R. top    = event-> gen. P. y;
	}
SIZE_EVENT:;
	if ( var->geometry == gtGrowMode && var-> growMode & gmCenter)
		my-> set_centered( self, var-> growMode & gmXCenter, var-> growMode & gmYCenter);
	if ( !event-> gen. B)
		my-> first_that( self, (void*) Widget_size_notify, &event-> gen. R);
	if ( doNotify) {
		Point oldSize;
		oldSize. x = event-> gen. R. left;
		oldSize. y = event-> gen. R. bottom;
		my-> notify( self, "<sPP", "Size", oldSize, event-> gen. P);
	}
	Widget_pack_slaves( self);
	Widget_place_slaves( self) ;
}

static Bool
pquery ( Handle window, Handle self, void * v)
{
	enter_method;
	Event ev = {cmClose};
	return ( var-> stage <= csNormal) ? !my-> message( self, &ev) : false;
}

Bool
prima_find_accel( Handle self, Handle item, int * key)
{
	return ( kind_of( item, CAbstractMenu)
				&& CAbstractMenu(item)-> sub_call_key( item, *key));
}

void Widget_handle_event( Handle self, PEvent event)
{
	enter_method;
	inherited-> handle_event ( self, event);
	objCheck;
	switch ( event-> cmd)
	{
		case cmCalcBounds:
		{
			Point min, max;
			min = var-> sizeMin;
			max = var-> sizeMax;
			if (( min. x > 0) && ( min. x > event-> gen. R. right  )) event-> gen. R. right  = min. x;
			if (( min. y > 0) && ( min. y > event-> gen. R. top    )) event-> gen. R. top    = min. y;
			if (( max. x > 0) && ( max. x < event-> gen. R. right  )) event-> gen. R. right  = max. x;
			if (( max. y > 0) && ( max. y < event-> gen. R. top    )) event-> gen. R. top    = max. y;
		}
		break;
		case cmSetup:
			if ( !is_opt( optSetupComplete)) {
				opt_set( optSetupComplete);
				my-> notify( self, "<s", "Setup");
			}
			break;
		case cmRepaint:
			my-> repaint( self);
			break;
		case cmPaint        :
			if ( !opt_InPaint && !my-> get_locked( self))
				if ( inherited-> begin_paint( self)) {
					if ( apc_widget_begin_paint( self, true)) {
						Bool flag = exception_block(true);
						my-> notify( self, "<sH", "Paint", self);
						exception_block(flag);
						if ( var-> stage == csNormal ) apc_widget_end_paint( self);
						EXCEPTION_CHECK_RAISE;
						objCheck;
						inherited-> end_paint( self);
					} else
						inherited-> end_paint( self);
				}
			break;
		case cmEnable       :
			my-> notify( self, "<s", "Enable");
			break;
		case cmDisable      :
			my-> notify( self, "<s", "Disable");
			break;
		case cmReceiveFocus :
			my-> notify( self, "<s", "Enter");
			break;
		case cmReleaseFocus :
			my-> notify( self, "<s", "Leave");
			break;
		case cmShow         :
			my-> notify( self, "<s", "Show");
			break;
		case cmHide         :
			my-> notify( self, "<s", "Hide");
			break;
		case cmHint:
			my-> notify( self, "<si", "Hint", event-> gen. B);
			break;
		case cmClose        :
			if ( my-> first_that( self, (void*)pquery, NULL)) {
				my-> clear_event( self);
				return;
			}
			objCheck;
			my-> notify( self, "<s", "Close");
			break;
		case cmZOrderChanged:
			my-> notify( self, "<s", "ZOrderChanged");
			break;
		case cmClick:
			my-> notify( self, "<s", "Click");
			break;
		case cmColorChanged:
			if ( !kind_of( event-> gen. source, CPopup))
				my-> notify( self, "<si", "ColorChanged", event-> gen. i);
			else
				var-> popupColor[ event-> gen. i] =
					apc_menu_get_color( event-> gen. source, event-> gen. i);
			break;
		case cmFontChanged:
			if ( !kind_of( event-> gen. source, CPopup))
				my-> notify( self, "<s", "FontChanged");
			else
				apc_menu_get_font( event-> gen. source, &var-> popupFont);
			break;
		case cmMenu:
			if ( event-> gen. H) {
				char buffer[16], *context;
				context = ((( PAbstractMenu) event-> gen. H)-> self)-> make_id_context( event-> gen. H, event-> gen. i, buffer);
				my-> notify( self, "<sHs", "Menu", event-> gen. H, context);
			}
			break;
		case cmMouseClick:
			my-> notify( self, "<siiPi", "MouseClick",
				event-> pos. button, event-> pos. mod, event-> pos. where, event-> pos. dblclk);
			break;
		case cmMouseDown:
			if (P_APPLICATION-> hintUnder == self)
				my-> set_hintVisible( self, 0);
			objCheck;
			if (((event-> pos. button & var-> selectingButtons) != 0) && my-> get_selectable( self))
				my-> set_selected( self, true);
			objCheck;
			my-> notify( self, "<siiP", "MouseDown",
				event-> pos. button, event-> pos. mod, event-> pos. where);
			break;
		case cmMouseUp:
			my-> notify( self, "<siiP", "MouseUp",
				event-> pos. button, event-> pos. mod, event-> pos. where);
			break;
		case cmMouseMove:
			if (P_APPLICATION-> hintUnder == self)
				my-> set_hintVisible( self, -1);
			objCheck;
			my-> notify( self, "<siP", "MouseMove", event-> pos. mod, event -> pos. where);
			break;
		case cmMouseWheel:
			my-> notify( self, "<siPi", "MouseWheel",
				event-> pos. mod, event -> pos. where,
				event-> pos. button); /* +n*delta == up, -n*delta == down */
			break;
		case cmMouseEnter:
			my-> notify( self, "<siP", "MouseEnter", event-> pos. mod, event -> pos. where);
			objCheck;
			if ( prima_guts.application && is_opt( optShowHint) && (P_APPLICATION-> options. optShowHint) && var-> hint && SvCUR(var-> hint))
				C_APPLICATION-> set_hint_action( prima_guts.application, self, true, true);
			break;
		case cmMouseLeave:
			if ( prima_guts.application && is_opt( optShowHint))
				C_APPLICATION-> set_hint_action( prima_guts.application, self, false, true);
			my-> notify( self, "<s", "MouseLeave");
			break;
		case cmKeyDown:
			handle_key_down(self, event);
			break;
		case cmDelegateKey:
			handle_delegate_key( self, event );
			break;
		case cmTranslateAccel:
		{
			int key = CAbstractMenu-> translate_key( NULL_HANDLE, event-> key. code, event-> key. key, event-> key. mod);
			if ( my-> first_that_component( self, (void*)prima_find_accel, &key)) {
				my-> clear_event( self);
				return;
			}
			objCheck;
			my-> notify( self, "<siii", "TranslateAccel",
				event-> key.code, event-> key. key, event-> key. mod);
			break;
		}
		case cmKeyUp:
			my-> notify( self, "<siii", "KeyUp",
			event-> key.code, event-> key. key, event-> key. mod);
			break;
		case cmMenuCmd:
			if ( event-> gen. source)
				((( PAbstractMenu) event-> gen. source)-> self)-> 
					sub_call_id( event-> gen. source, event-> gen. i);
			break;
		case cmMove:
			handle_move(self, event);
			break;
		case cmPopup:
			handle_popup(self, event);
			break;
		case cmSize:
			handle_size( self, event);
			break;
		case cmDragBegin       :
			handle_drag_begin( self, event );
			break;
		case cmDragOver       :
			handle_drag_over( self, event );
			break;
		case cmDragEnd        :
			handle_drag_end( self, event );
			break;
		case cmDragQuery       :
			handle_drag_query( self, event );
			break;
		case cmDragResponse     :
			my-> notify( self, "<siiH", "DragResponse", 
				event->dnd.allow, event->dnd.action, event->dnd.counterpart);
			break;
		case cmMenuItemMeasure  :
		case cmMenuItemPaint    :
			{
				Handle menu = event->gen.H;
				event->gen.H = self;
				PComponent(menu)->self->handle_event(menu, event);
				break;
			}
	}
}

void
Widget_key_event( Handle self, int command, int code, int key, int mod, int repeat, Bool post)
{
	Event ev;
	if ( command != cmKeyDown && command != cmKeyUp) return;
	memset( &ev, 0, sizeof( ev));
	if ( repeat <= 0) repeat = 1;
	ev. cmd = command;
	ev. key. code   = code;
	ev. key. key    = key;
	ev. key. mod    = mod;
	ev. key. repeat = repeat;
	apc_message( self, &ev, post);
}

void
Widget_mouse_event( Handle self, int command, int button, int mod, int x, int y, Bool dbl, Bool post)
{
	Event ev;
	if ( command != cmMouseDown
		&& command != cmMouseUp
		&& command != cmMouseClick
		&& command != cmMouseMove
		&& command != cmMouseWheel
		&& command != cmMouseEnter
		&& command != cmMouseLeave
	) return;

	memset( &ev, 0, sizeof( ev));
	ev. cmd = command;
	ev. pos. where. x = x;
	ev. pos. where. y = y;
	ev. pos. mod    = mod;
	ev. pos. button = button;
	if ( command == cmMouseClick) ev. pos. dblclk = dbl;
	apc_message( self, &ev, post);
}

void
Widget_post_message( Handle self, SV * info1, SV * info2)
{
	PPostMsg p;
	Event ev = { cmPost};
	if ( var-> stage > csNormal) return;
	if (!( p = alloc1( PostMsg))) return;
	p-> info1  = newSVsv( info1);
	p-> info2  = newSVsv( info2);
	p-> h = self;
	if ( var-> postList == NULL)
		var-> postList = plist_create( 8, 8);
	list_add( var-> postList, ( Handle) p);
	ev. gen. p = p;
	ev. gen. source = ev. gen. H = self;
	apc_message( self, &ev, true);
}

Bool
Widget_process_accel( Handle self, int key)
{
	enter_method;
	if ( my-> first_that_component( self, (void*)prima_find_accel, &key)) return true;
	return kind_of( var-> owner, CWidget) ?
			((( PWidget) var-> owner)-> self)->process_accel( var-> owner, key) : false;
}

void
Widget_on_paint( Handle self, SV * canvas)
{
	int i;
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK( sp);
	XPUSHs( canvas);
	for ( i = 0; i < 4; i++)
		XPUSHs( sv_2mortal( newSViv( -1)));
	PUTBACK;
	PERL_CALL_METHOD( "clear", G_DISCARD);
	SPAGAIN;
	PUTBACK;
	FREETMPS;
	LEAVE;
}

/*
void Widget_on_click( Handle self) {}
void Widget_on_change( Handle self) {}
void Widget_on_close( Handle self) {}
void Widget_on_colorchanged( Handle self, int colorIndex){}
void Widget_on_disable( Handle self) {}
void Widget_on_dragdrop( Handle self, Handle source , int x , int y ) {}
void Widget_on_dragover( Handle self, Handle source , int x , int y , int state ) {}
void Widget_on_enable( Handle self) {}
void Widget_on_enddrag( Handle self, Handle target , int x , int y ) {}
void Widget_on_fontchanged( Handle self) {}
void Widget_on_enter( Handle self) {}
void Widget_on_keydown( Handle self, int code , int key , int shiftState, int repeat ) {}
void Widget_on_keyup( Handle self, int code , int key , int shiftState ) {}
void Widget_on_menu( Handle self, Handle menu, char * variable) {}
void Widget_on_setup( Handle self) {}
void Widget_on_size( Handle self, Point oldSize, Point newSize) {}
void Widget_on_move( Handle self, Point oldPos, Point newPos) {}
void Widget_on_show( Handle self) {}
void Widget_on_hide( Handle self) {}
void Widget_on_hint( Handle self, Bool show) {}
void Widget_on_translateaccel( Handle self, int code , int key , int shiftState ) {}
void Widget_on_zorderchanged( Handle self) {}
void Widget_on_popup( Handle self, Bool mouseDriven, int x, int y) {}
void Widget_on_mouseclick( Handle self, int button , int shiftState , int x , int y , Bool dbl ) {}
void Widget_on_mousedown( Handle self, int button , int shiftState , int x , int y ) {}
void Widget_on_mouseup( Handle self, int button , int shiftState , int x , int y ) {}
void Widget_on_mousemove( Handle self, int shiftState , int x , int y ) {}
void Widget_on_mousewheel( Handle self, int shiftState , int x , int y, int z ) {}
void Widget_on_mouseenter( Handle self, int shiftState , int x , int y ) {}
void Widget_on_mouseleave( Handle self ) {}
void Widget_on_leave( Handle self) {}
*/


#ifdef __cplusplus
}
#endif

