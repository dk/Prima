#include "apricot.h"
#include "Window.h"
#include "Menu.h"
#include "Icon.h"
#include "Application.h"
#include <Window.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  set_text
#undef  my
#define inherited CWidget->
#define my  ((( PWindow) self)-> self)
#define var (( PWindow) self)


void
Window_init( Handle self, HV * profile)
{
	dPROFILE;
	SV * sv;
	inherited init( self, profile);

	opt_set( optSystemSelectable);
	opt_assign( optOwnerIcon, pget_B( ownerIcon));
	opt_assign( optMainWindow, pget_B( mainWindow));
	my-> set_icon( self, pget_H( icon));
	my-> menuColorIndex( self, true, ciFore,          pget_i( menuColor)            );
	my-> menuColorIndex( self, true, ciBack,          pget_i( menuBackColor)        );
	my-> menuColorIndex( self, true, ciHiliteText,    pget_i( menuHiliteColor)      );
	my-> menuColorIndex( self, true, ciHilite,        pget_i( menuHiliteBackColor)  );
	my-> menuColorIndex( self, true, ciDisabledText,  pget_i( menuDisabledColor)    );
	my-> menuColorIndex( self, true, ciDisabled,      pget_i( menuDisabledBackColor));
	my-> menuColorIndex( self, true, ciLight3DColor,  pget_i( menuLight3DColor)     );
	my-> menuColorIndex( self, true, ciDark3DColor,   pget_i( menuDark3DColor)      );
	SvHV_Font( pget_sv( menuFont), &Font_buffer, "Window::init");
	my-> set_menu_font  ( self, Font_buffer);
	if ( SvOK( sv = pget_sv( menuItems)))
		my-> set_menuItems( self, sv);
	my-> set_modalResult( self, pget_i( modalResult));
	my-> set_modalHorizon( self, pget_B( modalHorizon));
	my-> set_effects( self, pget_sv( effects));
	CORE_INIT_TRANSIENT(Window);
}

void
Window_cancel( Handle self)
{
	my-> set_modalResult ( self, mbCancel);
	my-> end_modal( self);
}

void
Window_ok( Handle self)
{
	my-> set_modalResult ( self, mbOK);
	my-> end_modal( self);
}

Bool
Window_can_propagate_key( Handle self)
{
	return !is_opt(optModalHorizon);
}

void
Window_cleanup( Handle self)
{
	if ( var-> modal) my-> cancel( self);
	if ( var->menu ) {
		apc_window_set_menu(self, NULL_HANDLE);
		unprotect_object(var-> menu);
		var->menu = NULL_HANDLE;
	}
	inherited cleanup( self);
}

void
Window_update_sys_handle( Handle self, HV * profile)
{
	dPROFILE;
	Handle owner;
	int borderIcons, borderStyle, windowState, onTop;
	Bool taskListed, syncPaint, originDontCare, sizeDontCare, layered;

	var-> widgetClass = wcWindow;
	if (!(
		pexist( owner) ||
		pexist( syncPaint) ||
		pexist( taskListed) ||
		pexist( borderIcons) ||
		pexist( onTop) ||
		pexist( layered) ||
		pexist( borderStyle)
	)) return;

	owner          = pexist( owner )      ? pget_H( owner )       : var-> owner;
	syncPaint      = pexist( syncPaint)   ? pget_B( syncPaint)    : my-> get_syncPaint( self);
	borderIcons    = pexist( borderIcons) ? pget_i( borderIcons)  : my-> get_borderIcons( self);
	borderStyle    = pexist( borderStyle) ? pget_i( borderStyle)  : my-> get_borderStyle( self);
	taskListed     = pexist( taskListed)  ? pget_B( taskListed)   : my-> get_taskListed( self);
	windowState    = pexist( windowState) ? pget_i( windowState)  : my-> get_windowState( self);
	onTop          = pexist( onTop)       ? (Bool) pget_B( onTop) : -1;
	originDontCare = !( pexist( originDontCare) && pget_B( originDontCare));
	sizeDontCare   = !( pexist( sizeDontCare)   && pget_B( sizeDontCare));
	layered        = pexist( layered)     ? pget_B( layered) : my-> get_layered( self);

	if ( pexist( owner)) my-> cancel_children( self);

	if ( !apc_window_create( self,
		owner, syncPaint, borderIcons, borderStyle, taskListed, windowState,
		onTop, originDontCare, sizeDontCare, layered))
		croak("Cannot create window");

	pdelete( borderStyle);
	pdelete( borderIcons);
	pdelete( syncPaint);
	pdelete( taskListed);
	pdelete( windowState);
	pdelete( onTop);
	pdelete( layered);
}

void Window_handle_event( Handle self, PEvent event)
{
#define evOK ( var-> evStack[ var-> evPtr - 1])
#define objCheck if ( var-> stage > csNormal) return
	switch (event-> cmd)
	{
	case cmColorChanged:
		if ( event-> gen. source == var-> menu) {
			var-> menuColor[ event-> gen. i] = apc_menu_get_color( var-> menu, event-> gen. i);
			return;
		}
		break;
	case cmFontChanged:
		if ( event-> gen. source == var-> menu)
		{
			apc_menu_get_font( var-> menu, &var-> menuFont);
			return;
		}
		break;
	case cmExecute:
		my-> notify( self, "<s", "Execute");
		break;
	case cmEndModal:
		my-> notify( self, "<s", "EndModal");
		break;
	case cmActivate:
		if ( var-> owner)
			PWidget( var-> owner)-> currentWidget = self;
		my-> notify( self, "<s", "Activate");
		break;
	case cmDeactivate:
		my-> notify( self, "<s", "Deactivate");
		break;
	case cmWindowState:
		my-> notify( self, "<si", "WindowState", event-> gen. i);
		break;
	case cmDelegateKey:
		#define leave { my-> clear_event( self); return; }
		if ( var-> modal && event-> key. subcmd == 0)
		{
			Event ev = *event;
			ev. key. cmd   = cmTranslateAccel;
			if ( !my-> message( self, &ev)) leave;
			if ( my-> first_that( self, (void*)prima_accel_notify, &ev)) leave;

			ev. key. cmd    = cmDelegateKey;
			ev. key. subcmd = 1;
			if ( my-> first_that( self, (void*)prima_accel_notify, &ev)) leave;
		}
		objCheck;
		break;
	case cmTranslateAccel:
		if ( event-> key. key == kbEsc && var-> modal)
		{
			my-> cancel( self);
			my-> clear_event( self);
			return;
		}
		break;
	}
	inherited handle_event ( self, event);
}

void
Window_end_modal( Handle self)
{
	Event ev = { cmEndModal};
	if ( !var-> modal) return;

	apc_window_end_modal( self);
	ev. gen. source = self;
	my-> message( self, &ev);
}

int
Window_get_modal( Handle self)
{
	return var-> modal;
}

SV *
Window_get_client_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_window_get_client_handle( self));
	return newSVpv( buf, 0);
}

Handle
Window_get_modal_window( Handle self, int modalFlag, Bool next)
{
	if ( modalFlag == mtExclusive) {
		return next ? var-> nextExclModal   : var-> prevExclModal;
	} else if ( modalFlag == mtShared) {
		return next ? var-> nextSharedModal : var-> prevSharedModal;
	}
	return NULL_HANDLE;
}

Handle
Window_get_horizon( Handle self)
{
	/* self trick is appropriate here;
		don't bump into it accidentally */
	self = var-> owner;
	while ( self != prima_guts.application && !my-> get_modalHorizon( self))
		self = var-> owner;
	return self;
}


void
Window_exec_enter_proc( Handle self, Bool sharedExec, Handle insertBefore)
{
	if ( var-> modal)
		return;

	if ( sharedExec) {
		Handle mh = my-> get_horizon( self);
		var-> modal = mtShared;

		/* adding new modal horizon in global mh-list */
		if ( mh != prima_guts.application && !PWindow(mh)-> nextSharedModal)
			list_add( &P_APPLICATION-> modalHorizons, mh);

		if (( var-> nextSharedModal = insertBefore)) {
			/* inserting window in between of modal list */
			Handle *iBottom = ( mh == prima_guts.application) ?
				&PApplication(mh)-> sharedModal : &PWindow(mh)-> nextSharedModal;
			var-> prevSharedModal = PWindow( insertBefore)-> prevSharedModal;
			if ( *iBottom == insertBefore)
				*iBottom = self;
		} else {
			/* inserting window on top of modal list */
			Handle *iTop = ( mh == prima_guts.application) ?
				&PApplication(mh)-> topSharedModal : &PWindow(mh)-> topSharedModal;
			if ( *iTop)
				PWindow( *iTop)-> nextSharedModal = self;
			else {
				if ( mh == prima_guts.application)
					PApplication(mh)-> sharedModal = self;
				else
					PWindow(mh)-> nextSharedModal = self;
			}
			var-> prevSharedModal = *iTop;
			*iTop = self;
		}
	}
	/* end of shared exec */
	else
	/* start of exclusive exec */
	{
		PApplication app = ( PApplication) prima_guts.application;
		var-> modal = mtExclusive;
		if (( var-> nextExclModal = insertBefore)) {
			var-> prevExclModal = PWindow( insertBefore)-> prevExclModal;
			if ( app-> exclModal == insertBefore)
				app-> exclModal = self;
		} else {
			var-> prevExclModal = app-> topExclModal;
			if ( app-> exclModal)
				PWindow( app-> topExclModal)-> nextExclModal = self;
			else
				app-> exclModal = self;
			app-> topExclModal = self;
		}
	}
}

void
Window_exec_leave_proc( Handle self)
{
	if ( !var-> modal)
		return;

	if ( var-> modal == mtShared) {
		Handle mh = my-> get_horizon( self);

		if ( var-> prevSharedModal) {
			PWindow prev = ( PWindow) var-> prevSharedModal;
			if ( prev-> nextSharedModal == self)
				prev-> nextSharedModal = var-> nextSharedModal;
		}
		if ( var-> nextSharedModal) {
			PWindow next = ( PWindow) var-> nextSharedModal;
			if ( next-> prevSharedModal == self)
				next-> prevSharedModal = var-> prevSharedModal;
		}
		if ( mh == prima_guts.application) {
			PApplication app = ( PApplication) prima_guts.application;
			if ( app) {
				if ( app-> sharedModal == self)
					app-> sharedModal = var-> nextSharedModal;
				if ( app-> topSharedModal == self)
					app-> topSharedModal = var-> prevSharedModal;
			}
		} else {
			PWindow win = ( PWindow) mh;
			if ( win-> nextSharedModal == self)
				win-> nextSharedModal = var-> nextSharedModal;
			if ( win-> topSharedModal == self)
				win-> topSharedModal = var-> prevSharedModal;
			/* removing horizon from global mh-list */
			if ( !win-> nextSharedModal)
				list_delete( &P_APPLICATION-> modalHorizons, mh);
		}

		var-> prevSharedModal = var-> nextSharedModal = NULL_HANDLE;
	} /* end of shared exec */
	else
	/* start of exclusive exec */
	{
		PApplication app = ( PApplication) prima_guts.application;
		if ( var-> prevExclModal) {
			PWindow prev = ( PWindow) var-> prevExclModal;
			if ( prev-> nextExclModal == self)
				prev-> nextExclModal = var-> nextExclModal;
		}
		if ( var-> nextExclModal) {
			PWindow next = ( PWindow) var-> nextExclModal;
			if ( next-> prevExclModal == self)
				next-> prevExclModal = var-> prevExclModal;
		}
		if ( app) {
			if ( app-> exclModal == self)
				app-> exclModal = var-> nextExclModal;
			if ( app-> topExclModal == self)
				app-> topExclModal = var-> prevExclModal;
		}
		var-> prevExclModal = var-> nextExclModal = NULL_HANDLE;
	}
	var-> modal = mtNone;
}

void
Window_cancel_children( Handle self)
{
	protect_object( self);
	if ( my-> get_modalHorizon( self)) {
		Handle next = var-> nextSharedModal;
		while ( next) {
			CWindow( next)-> cancel( next);
			next = var-> nextSharedModal;
		}
	} else {
		Handle mh   = my-> get_horizon( self);
		Handle next = ( mh == prima_guts.application) ?
				PApplication(mh)-> sharedModal :
				PWindow(mh)-> nextSharedModal;
		while ( next) {
			if ( Widget_is_child( next, self)) {
				CWindow( next)-> cancel( next);
				next = PWindow(mh)-> nextSharedModal;
			} else
				next = PWindow(next)-> nextSharedModal;
		}
	}
	unprotect_object( self);
}


int
Window_execute( Handle self, Handle insertBefore)
{
	if ( var-> modal)
		return mbCancel;

	protect_object( self);
	if ( insertBefore
		&& ( insertBefore == self
			|| !kind_of( insertBefore, CWindow)
			|| PWindow( insertBefore)-> modal != mtExclusive))
		insertBefore = NULL_HANDLE;
	if ( !apc_window_execute( self, insertBefore))
		var-> modalResult = mbCancel;

	unprotect_object( self);
	return var-> modalResult;
}

Bool
Window_execute_shared( Handle self, Handle insertBefore)
{
	if ( var-> modal || var-> nextSharedModal) return false;
	if ( insertBefore &&
		(( insertBefore == self) ||
		( !kind_of( insertBefore, CWindow)) ||
		( PWindow( insertBefore)-> modal != mtShared) ||
		( CWindow( insertBefore)-> get_horizon( insertBefore) != my-> get_horizon( self))))
			insertBefore = NULL_HANDLE;
	return apc_window_execute_shared( self, insertBefore);
}

Bool
Window_modalHorizon( Handle self, Bool set, Bool modalHorizon)
{
	if ( !set)
		return is_opt( optModalHorizon);
	if ( is_opt( optModalHorizon) == modalHorizon) return false;
	my-> cancel_children( self);
	opt_assign( optModalHorizon, modalHorizon);
	return modalHorizon;
}

int
Window_modalResult ( Handle self, Bool set, int modalResult)
{
	if ( !set)
		return var-> modalResult;
	return var-> modalResult = modalResult;
}

static void
activate( Handle self, Bool ok)
{
	if ( var-> stage == csNormal) {
		if ( ok)
			apc_window_activate( self);
		else
			if ( apc_window_is_active( self))
				apc_window_activate( NULL_HANDLE);
	}
}

Bool
Window_focused( Handle self, Bool set, Bool focused)
{
	if ( set)
		activate( self, focused);
	return inherited focused( self, set, focused);
}

void Window_set( Handle self, HV * profile)
{
	dPROFILE;
	Bool owner_icon = false;

	if ( pexist( menuFont)) {
		SvHV_Font( pget_sv( menuFont), &Font_buffer, "Window::set");
		my-> set_menu_font( self, Font_buffer);
		pdelete( menuFont);
	}

	if ( pexist( owner)) {
		owner_icon = pexist( ownerIcon) ? pget_B( ownerIcon) : my-> get_ownerIcon( self);
		pdelete( ownerIcon);
	}

	if ( pexist( frameOrigin) || pexist( frameSize)) {
		Bool io = 0, is = 0;
		Point o, s;
		if ( pexist( frameOrigin)) {
			int set[2];
			prima_read_point( pget_sv( frameOrigin), set, 2, "Array panic on 'frameOrigin'");
			pdelete( frameOrigin);
			o. x = set[0];
			o. y = set[1];
			io = 1;
		} else {
			o.x = o. y = 0;
		}
		if ( pexist( frameSize)) {
			int set[2];
			prima_read_point( pget_sv( frameSize), set, 2, "Array panic on 'frameSize'");
			pdelete( frameSize);
			s. x = set[0];
			s. y = set[1];
			is = 1;
		} else {
			s.x = s. y = 0;
		}
		if ( is && io)
			apc_widget_set_rect( self, o. x, o. y, s. x, s. y);
		else if ( io)
			my-> set_frameOrigin( self, o);
		else
			my-> set_frameSize( self, s);
}

	inherited set( self, profile);
	if ( owner_icon) {
		my-> set_ownerIcon( self, 1);
		opt_set( optOwnerIcon);
	}
}

static Bool
icon_notify ( Handle self, Handle child, Handle icon)
{
	if ( kind_of( child, CWindow) && (( PWindow) child)-> options. optOwnerIcon) {
		(( PWindow) child)-> self-> set_icon( child, icon);
		(( PWindow) child)-> options. optOwnerIcon = 1;
	}
	return false;
}

Handle
Window_icon( Handle self, Bool set, Handle icon)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;

	if ( !set) {
		if ( apc_window_get_icon( self, NULL_HANDLE)) {
			HV * profile = newHV();
			Handle i = Object_create( "Prima::Icon", profile);
			sv_free(( SV *) profile);
			apc_window_get_icon( self, i);
			--SvREFCNT( SvRV((( PAnyObject) i)-> mate));
			return i;
		} else
			return NULL_HANDLE;
	}

	if ( icon && !kind_of( icon, CImage)) {
		warn("Illegal object reference passed to Window::icon");
		return NULL_HANDLE;
	}
	my-> first_that( self, (void*)icon_notify, (void*)icon);
	apc_window_set_icon( self, icon);
	opt_clear( optOwnerIcon);
	return NULL_HANDLE;
}

Bool
Window_mainWindow( Handle self, Bool set, Bool mainWindow)
{
	if ( !set)
		return is_opt( optMainWindow);
	opt_assign( optMainWindow, mainWindow);
	return false;
}

Handle
Window_menu( Handle self, Bool set, Handle menu)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( !set)
		return var-> menu;
	if ( menu && !kind_of( menu, CMenu)) return NULL_HANDLE;

	if ( var->menu )
		unprotect_object(var-> menu);
	apc_window_set_menu( self, menu);
	var-> menu = menu;
	if ( var->menu )
		protect_object(var-> menu);
	return NULL_HANDLE;
}

SV *
Window_menuItems( Handle self, Bool set, SV * menuItems)
{
	dPROFILE;
	if ( var-> stage > csFrozen) return NULL_SV;

	if ( !set)
		return var-> menu ? CMenu( var-> menu)-> get_items( var-> menu, "", true) : NULL_SV;

	if ( var-> menu == NULL_HANDLE) {
		if ( SvTYPE( menuItems)) {
			Handle menu;
			HV * profile = newHV();
			pset_sv( items, menuItems);
			pset_H ( owner, self);
			pset_i ( selected, false);
			if (( menu = create_instance( "Prima::Menu")) != NULL_HANDLE) {
				int i;
				ColorSet color;
				my-> set_menu( self, menu );
				memcpy( color, var-> menuColor, sizeof( ColorSet));
				for ( i = 0; i < ciMaxId + 1; i++)
					apc_menu_set_color( menu, color[ i], i);
				memcpy( var-> menuColor, color, sizeof( ColorSet));
				apc_menu_set_font( menu, &var-> menuFont);
			}
			sv_free(( SV *) profile);
		}
	} else
		CMenu( var-> menu)-> set_items( var-> menu, menuItems);
	return menuItems;
}

Color
Window_menuColorIndex( Handle self, Bool set, int index, Color color)
{
	if (( index < 0) || ( index > ciMaxId)) return clInvalid;
	if ( !set)
		return  var-> menuColor[ index];
	if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0)) color |= wcMenu;
	var-> menuColor[ index] = color;
	if ( var-> menu) apc_menu_set_color( var-> menu, color, index);
	return clInvalid;
}

void
Window_set_menu_font( Handle self, Font font)
{
	apc_font_pick( self, &font, &var-> menuFont);
	if ( var-> menu) apc_menu_set_font( var-> menu, &var-> menuFont);
}

Font
Window_get_menu_font( Handle self)
{
	return var-> menuFont;
}

Font
Window_get_default_menu_font( char * dummy)
{
	Font f;
	apc_menu_default_font( &f);
	return f;
}

Bool
Window_onTop( Handle self, Bool set, Bool onTop)
{
	HV * profile;
	if ( !set)
		return apc_window_get_on_top( self);
	profile = newHV();
	pset_i( onTop, onTop);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return true;
}

Bool
Window_ownerIcon( Handle self, Bool set, Bool ownerIcon)
{
	if ( !set)
		return is_opt( optOwnerIcon);
	opt_assign( optOwnerIcon, ownerIcon);
	if ( is_opt( optOwnerIcon) && var-> owner) {
		Handle icon = ( var-> owner == prima_guts.application) ?
			C_APPLICATION-> get_icon(prima_guts.application) :
			CWindow(      var-> owner)-> get_icon( var-> owner);
		my-> set_icon( self, icon);
		opt_set( optOwnerIcon);
	}
	return false;
}

Bool
Window_process_accel( Handle self, int key)
{
	return (
		var-> modal ?
			my-> first_that_component( self, 0, (void*)prima_find_accel, &key)
			: inherited process_accel( self, key)
		) != NULL_HANDLE;
}

void  Window_on_execute( Handle self) {}
void  Window_on_endmodal( Handle self) {}
void  Window_on_activate( Handle self) {}
void  Window_on_deactivate( Handle self) {}
void  Window_on_windowstate( Handle self, int windowState) {}

Bool
Window_transparent( Handle self, Bool set, Bool transparent)
{
	return false;
}

int
Window_borderIcons( Handle self, Bool set, int borderIcons)
{
	HV * profile;
	if ( !set)
		return apc_window_get_border_icons( self);
	profile = newHV();
	pset_i( borderIcons, borderIcons);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return NULL_HANDLE;
}

int
Window_borderStyle( Handle self, Bool set, int borderStyle)
{
	HV * profile;
	if ( !set)
		return apc_window_get_border_style( self);
	profile = newHV();
	pset_i( borderStyle, borderStyle);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return NULL_HANDLE;
}

void
Window_done( Handle self)
{
	if ( var-> effects ) sv_free( var->effects);
	var-> effects = NULL;
	inherited done( self);
}

SV *
Window_effects( Handle self, Bool set, SV * effects)
{
	if ( !set )
		return var->effects ? newSVsv(var->effects) : &PL_sv_undef;

	if ( var-> effects ) sv_free( var-> effects );
	if ( effects ) {
		if (SvROK( effects) && ( SvTYPE( SvRV( effects)) == SVt_PVHV)) {
			var-> effects = newSVsv(effects);
			apc_window_set_effects( self, (HV*) SvRV(var-> effects));
		} else if (!SvROK(effects) || SvOK(SvRV(effects))) {
			var-> effects = NULL;
			apc_window_set_effects( self, NULL );
		} else {
			croak("Not a hash or undef passed to Window.effects");
		}
	}

	return NULL_SV;
}

Point
Window_frameOrigin( Handle self, Bool set, Point frameOrigin)
{
	if ( !set)
		return apc_widget_get_pos( self);
	apc_widget_set_pos( self, frameOrigin.x, frameOrigin.y);
	return frameOrigin;
}

Point
Window_frameSize( Handle self, Bool set, Point frameSize)
{
	if ( !set)
		return apc_widget_get_size( self);
	apc_widget_set_size( self, frameSize.x, frameSize.y);
	return frameSize;
}

Point
Window_origin( Handle self, Bool set, Point origin)
{
	if ( !set)
		return apc_window_get_client_pos( self);
	apc_window_set_client_pos( self, origin.x, origin.y);
	return origin;
}

Rect
Window_rect( Handle self, Bool set, Rect r)
{
	if ( !set)
		return inherited rect( self, set, r);
	apc_window_set_client_rect( self, r. left, r. bottom, r. right - r. left, r. top - r. bottom);
	return r;
}

Bool
Window_selected( Handle self, Bool set, Bool selected)
{
	if (!set)
		return inherited get_selected( self);
	activate( self, selected);
	inherited selected( self, set, selected);
	return selected;
}

Point
Window_size( Handle self, Bool set, Point size)
{
	if ( !set)
		return apc_window_get_client_size( self);
	apc_window_set_client_size( self, size.x, size.y);
	return size;
}

Bool
Window_taskListed( Handle self, Bool set, Bool taskListed)
{
	HV * profile;
	if ( !set)
		return apc_window_get_task_listed( self);
	profile = newHV();
	pset_i( taskListed, taskListed);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return false;
}

void
Window_set_text( Handle self, SV * text)
{
	inherited set_text( self, text);
	if ( var-> text )
		apc_window_set_caption( self, SvPV_nolen( var-> text ), prima_is_utf8_sv( var-> text ));
	else
		apc_window_set_caption( self, "", 0);
}

Bool
Window_validate_owner( Handle self, Handle * owner, HV * profile)
{
	dPROFILE;
	*owner = pget_H( owner);
	if ( *owner != prima_guts.application && !kind_of( *owner, CWidget)) return false;
	return inherited validate_owner( self, owner, profile);
}

int
Window_windowState( Handle self, Bool set, int windowState)
{
	if ( !set)
		return apc_window_get_window_state( self);
	return ( int) apc_window_set_window_state( self, windowState);
}

#ifdef __cplusplus
}
#endif
