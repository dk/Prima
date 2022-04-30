#include "apricot.h"
#include "Timer.h"
#include "Window.h"
#include "Image.h"
#include "Application.h"
#include <Application.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CWidget->
#define my  ((( PApplication) self)-> self)
#define var (( PApplication) self)

static void Application_HintTimer_handle_event( Handle, PEvent);

void
Application_init( Handle self, HV * profile)
{
	dPROFILE;
	int hintPause = pget_i( hintPause);
	Color hintColor = pget_i( hintColor), hintBackColor = pget_i( hintBackColor);
	SV * hintFont = pget_sv( hintFont);
	SV * sv;
	char * hintClass      = pget_c( hintClass);
	if ( prima_guts.application != NULL_HANDLE)
		croak( "Attempt to create more than one application instance");

	opt_set(optSystemDrawable);
	CDrawable-> init( self, profile);
	list_create( &var->  widgets, 16, 16);
	list_create( &var->  modalHorizons, 0, 8);
	prima_guts.application = self;
	if ( !apc_application_create( self))
		croak( "Error creating application");
/* Widget init */
	SvHV_Font( pget_sv( font), &Font_buffer, "Application::init");
	my-> set_font( self, Font_buffer);
	SvHV_Font( pget_sv( popupFont), &Font_buffer, "Application::init");
	my-> set_popup_font( self, Font_buffer);
	{
		AV * av = ( AV *) SvRV( pget_sv( designScale));
		SV ** holder = av_fetch( av, 0, 0);
		if ( holder)
			var->  designScale. x = SvNV( *holder);
		else
			warn("Array panic on 'designScale'");
		holder = av_fetch( av, 1, 0);
		if ( holder)
			var->  designScale. y = SvNV( *holder);
		else
			warn("Array panic on 'designScale'");
		pdelete( designScale);
	}
	var->  text = newSVpv("", 0);
	var->  hint = newSVpv("", 0);
	opt_set( optModalHorizon);

	/* store extra info */
	{
		HV * hv = ( HV *) SvRV( var-> mate);
		(void) hv_store( hv, "PrinterClass",  12, newSVpv( pget_c( printerClass),  0), 0);
		(void) hv_store( hv, "PrinterModule", 13, newSVpv( pget_c( printerModule), 0), 0);
		(void) hv_store( hv, "HelpClass",     9,  newSVpv( pget_c( helpClass),     0), 0);
		(void) hv_store( hv, "HelpModule",    10, newSVpv( pget_c( helpModule),    0), 0);
	}

	{
		HV * profile = newHV();
		static Timer_vmt HintTimerVmt;

		pset_H( owner, self);
		pset_i( timeout, hintPause);
		pset_c( name, "HintTimer");
		var->  hintTimer = create_instance( "Prima::Timer");
		protect_object( var-> hintTimer);
		hv_clear( profile);
		memcpy( &HintTimerVmt, CTimer, sizeof( HintTimerVmt));
		HintTimerVmt. handle_event = Application_HintTimer_handle_event;
		(( PTimer) var->  hintTimer)-> self = &HintTimerVmt;

		pset_H( owner, self);
		pset_i( color, hintColor);
		pset_i( backColor, hintBackColor);
		pset_i( visible, 0);
		pset_i( selectable, 0);
		pset_i( showHint, 0);
		pset_c( name, "HintWidget");
		pset_sv( font, hintFont);
		var->  hintWidget = create_instance( hintClass);
		protect_object( var->  hintWidget);
		sv_free(( SV *) profile);
	}

	if ( SvTYPE( sv = pget_sv( accelItems)) != SVt_NULL)
		my-> set_accelItems( self, sv);
	if ( SvTYPE( sv = pget_sv( popupItems)) != SVt_NULL)
		my-> set_popupItems( self, sv);
	pdelete( accelTable);
	pdelete( accelItems);
	pdelete( popupItems);

	my-> set( self, profile);
	CORE_INIT_TRANSIENT(Application);
}

void
Application_done( Handle self)
{
	if ( self != prima_guts.application) return;
	unprotect_object( var-> hintTimer);
	unprotect_object( var-> hintWidget);
	list_destroy( &var->  modalHorizons);
	list_destroy( &var->  widgets);
	if ( var-> text ) SvREFCNT_dec( var-> text);
	if ( var-> hint ) SvREFCNT_dec( var-> hint);
	free( var-> helpContext);
	var-> accelTable = var-> hintWidget = var-> hintTimer = NULL_HANDLE;
	var-> helpContext = NULL;
	var-> hint = var-> text = NULL;
	apc_application_destroy( self);
	CDrawable-> done( self);
	prima_guts.application = NULL_HANDLE;
}

static Bool 
kill_all_objects( Handle self, Handle child, void * dummy)
{
	Object_destroy( child);
	return false;
}


void
Application_cleanup( Handle self)
{
	int i;

	for ( i = 0; i < var-> widgets. count; i++)
		Object_destroy( var-> widgets. items[i]);

	if ( var-> icon)
		my-> detach( self, var-> icon, true);
	var-> icon = NULL_HANDLE;

	my-> first_that_component( self, 0, (void*)kill_all_objects, NULL);

	CDrawable-> cleanup( self);
}


void
Application_set( Handle self, HV * profile)
{
	pdelete( bottom);
	pdelete( buffered);
	pdelete( capture);
	pdelete( centered);
	pdelete( clipOwner);
	pdelete( enabled);
	pdelete( focused);
	pdelete( geometry);
	pdelete( geomHeight);
	pdelete( geomSize);
	pdelete( geomWidth);
	pdelete( growMode);
	pdelete( height);
	pdelete( hintClass);
	pdelete( hintVisible);
	pdelete( layered);
	pdelete( left);
	pdelete( modalHorizon);
	pdelete( origin);
	pdelete( owner);
	pdelete( ownerBackColor);
	pdelete( ownerColor);
	pdelete( ownerFont);
	pdelete( ownerPalette);
	pdelete( ownerShowHint);
	pdelete( palette);
	pdelete( pack);
	pdelete( place);
	pdelete( printerClass);
	pdelete( printerModule);
	pdelete( helpClass);
	pdelete( helpModule);
	pdelete( rect);
	pdelete( rigth);
	pdelete( selectable);
	pdelete( shape);
	pdelete( size);
	pdelete( syncPaint);
	pdelete( tabOrder);
	pdelete( tabStop);
	pdelete( transparent);
	pdelete( text);
	pdelete( top);
	pdelete( visible);
	pdelete( width);
	inherited set( self, profile);
}

void Application_handle_event( Handle self, PEvent event)
{
	switch ( event-> cmd)
	{
		case cmPost:
			if ( event-> gen. H != self)
			{
				((( PComponent) event-> gen. H)-> self)-> message( event-> gen. H, event);
				event-> cmd = 0;
				if ( var->  stage > csNormal) return;
			}
			break;
		case cmIdle:
			my-> notify( self, "<s", "Idle");
			return;
	}
	inherited handle_event ( self, event);
}

void
Application_sync( char * dummy)
{
	apc_application_sync();
}

Bool
Application_yield( char * dummy, Bool wait_for_event)
{
	return apc_application_yield(wait_for_event);
}

Bool
Application_begin_paint( Handle self)
{
	Bool ok;
	if ( !CDrawable-> begin_paint( self))
		return false;
	if ( !( ok = apc_application_begin_paint( self))) {
		CDrawable-> end_paint( self);
		perl_error();
	}
	return ok;
}

Bool
Application_begin_paint_info( Handle self)
{
	Bool ok;
	if ( is_opt( optInDraw))     return true;
	if ( !CDrawable-> begin_paint_info( self))
		return false;
	if ( !( ok = apc_application_begin_paint_info( self))) {
		CDrawable-> end_paint_info( self);
		perl_error();
	}
	return ok;
}

void
Application_detach( Handle self, Handle objectHandle, Bool kill)
{
	inherited detach( self, objectHandle, kill);
	if ( var->  autoClose &&
		( var->  widgets. count == 1) &&
		kind_of( objectHandle, CWidget) &&
		( objectHandle != var->  hintWidget)
		) my-> close( self);
}

void
Application_end_paint( Handle self)
{
	if ( !is_opt( optInDraw)) return;
	apc_application_end_paint( self);
	CDrawable-> end_paint( self);
}

void
Application_end_paint_info( Handle self)
{
	if ( !is_opt( optInDrawInfo)) return;
	apc_application_end_paint_info( self);
	CDrawable-> end_paint_info( self);
}

Bool
Application_focused( Handle self, Bool set, Bool focused)
{
	if ( set) return false;
	return inherited focused( self, set, focused);
}

void Application_bring_to_front( Handle self) {}
void Application_show( Handle self) {}
void Application_hide( Handle self) {}
void Application_insert_behind( Handle self, Handle view) {}
void Application_send_to_back( Handle self) {}

SV*
Application_fonts( Handle self, char * name, char * encoding)
{
	int count, i;
	AV * glo = newAV();
	PFont fmtx = apc_fonts( self,
		(name && name[0])         ? name : NULL,
		(encoding && encoding[0]) ? encoding : NULL,
		&count);
	for ( i = 0; i < count; i++) {
		SV * sv      = sv_Font2HV( &fmtx[ i]);
		HV * profile = ( HV*) SvRV( sv);
		if ( fmtx[i].is_utf8.name ) {
			SV ** entry = hv_fetch(( HV*) SvRV( sv), "name", 4, 0);
			if ( entry && SvOK( *entry))
				SvUTF8_on( *entry);
		}
		if ( fmtx[i].is_utf8.family ) {
			SV ** entry = hv_fetch(( HV*) SvRV( sv), "family", 6, 0);
			if ( name && SvOK( *entry))
				SvUTF8_on( *entry);
		}
		if ( fmtx[i].is_utf8.encoding ) {
			SV ** entry = hv_fetch(( HV*) SvRV( sv), "encoding", 8, 0);
			if ( name && SvOK( *entry))
				SvUTF8_on( *entry);
		}
		if ( name[0] == 0 && encoding[0] == 0) {
			/* Read specially-coded (const char*) encodings[] vector,
			stored in fmtx[i].encoding. First pointer is filled with 0s,
			except the last byte which is a counter. Such scheme
			allows max 31 encodings per entry to be coded with sizeof(char*)==8.
			The interface must be re-implemented, but this requires
			either change in gencls syntax so arrays can be members of hashes,
			or passing of a dynamic-allocated pointer vector here.
			*/
			char ** enc = (char**) fmtx[i].encoding;
			unsigned char * shift = (unsigned char*) enc + sizeof(char *) - 1, j = *shift;
			AV * loc = newAV();
			pset_sv_noinc( encoding, newSVpv(( j > 0) ? *(++enc) : "", 0));
			while ( j--) av_push( loc, newSVpv(*(enc++),0));
			pset_sv_noinc( encodings, newRV_noinc(( SV*) loc));
		}
		pdelete( resolution);
		pdelete( codepage);
		av_push( glo, sv);
	}
	free( fmtx);
	return newRV_noinc(( SV *) glo);
}

SV*
Application_font_mapper_action( char * dummy, HV * profile)
{
	dPROFILE;
	SV * ret = NULL_SV;
	char * command;
	int cmd;
	Font font;
	if ( !pexist(command) ) {
		warn("command expected");
		goto EXIT;
	}

	command = pget_c(command);
	if ( strcmp(command, "get_font") == 0 ) {
		PFont f;
		if ( !pexist(index) ) {
			warn("index expected");
			goto EXIT;
		}
		f = prima_font_mapper_get_font(pget_i(index));
		if (!f) goto EXIT;

		ret = sv_Font2HV( f );
	} else if ( strcmp(command, "get_count") == 0 ) {
		ret = newSViv( prima_font_mapper_action(pfmaGetCount, NULL));
	} else if (
		((strcmp(command, "disable")    == 0 ) && (cmd = pfmaIsActive )) ||
		((strcmp(command, "enable")     == 0 ) && (cmd = pfmaPassivate)) ||
		((strcmp(command, "is_enabled") == 0 ) && (cmd = pfmaActivate )) ||
		((strcmp(command, "passivate")  == 0 ) && (cmd = pfmaIsEnabled)) ||
		((strcmp(command, "activate")   == 0 ) && (cmd = pfmaEnable   )) ||
		((strcmp(command, "is_active")  == 0 ) && (cmd = pfmaDisable  )) ||
		((strcmp(command, "get_index")  == 0 ) && (cmd = pfmaGetIndex ))
	) {
		if ( !pexist(font) ) {
			warn("font expected");
			goto EXIT;
		}
		SvHV_Font(pget_sv(font), &font, "Application::font_mapper");
		ret = newSViv( prima_font_mapper_action(cmd, &font));
	} else {
		warn("unknown command");
	}

EXIT:
	hv_clear(profile);
	return ret;
}

SV*
Application_font_encodings( Handle self, char * encoding)
{
	AV * glo = newAV();
	HE *he;
	PHash h = apc_font_encodings( self);

	if ( !h) return newRV_noinc(( SV *) glo);
	hv_iterinit(( HV*) h);
	for (;;)
	{
		void *key;
		STRLEN  keyLen;
		if (( he = hv_iternext( h)) == NULL)
			break;
		key    = HeKEY( he);
		keyLen = HeKLEN( he);
		av_push( glo, newSVpvn(( char*) key, keyLen));
	}
	return newRV_noinc(( SV *) glo);
}

Font
Application_get_default_font( char * dummy)
{
	Font font;
	apc_font_default( &font);
	return font;
}

Font
Application_get_message_font( char * dummy)
{
	Font font;
	apc_sys_get_msg_font( &font);
	return font;
}

Font
Application_get_caption_font( char * dummy)
{
	Font font;
	apc_sys_get_caption_font( &font);
	return font;
}


int
Application_get_default_cursor_width( char * dummy)
{
	return apc_sys_get_value( svXCursor);
}

Point
Application_get_default_scrollbar_metrics( char * dummy)
{
	Point ret;
	ret. x = apc_sys_get_value( svXScrollbar);
	ret. y = apc_sys_get_value( svYScrollbar);
	return ret;
}

Point
Application_get_default_window_borders( char * dummy, int borderStyle)
{
	Point ret = { 0,0};
	switch ( borderStyle) {
	case bsNone:
		ret.x = svXbsNone;
		ret.y = svYbsNone;
		break;
	case bsSizeable:
		ret.x = svXbsSizeable;
		ret.y = svYbsSizeable;
		break;
	case bsSingle:
		ret.x = svXbsSingle;
		ret.y = svYbsSingle;
		break;
	case bsDialog:
		ret.x = svXbsDialog;
		ret.y = svYbsDialog;
		break;
	default:
		return ret;
	}
	ret. x = apc_sys_get_value( ret. x);
	ret. y = apc_sys_get_value( ret. y);
	return ret;
}

int
Application_get_system_value( char * dummy, int sysValue)
{
	return apc_sys_get_value( sysValue);
}

SV *
Application_get_system_info( char * dummy)
{
	HV * profile = newHV();
	char system   [ 1024];
	char release  [ 1024];
	char vendor   [ 1024];
	char arch     [ 1024];
	char gui_desc [ 1024];
	char gui_lang [ 1024];
	int  os, gui;

	os  = apc_application_get_os_info( system, sizeof( system),
					release, sizeof( release),
					vendor, sizeof( vendor),
					arch, sizeof( arch));
	gui = apc_application_get_gui_info( gui_desc, sizeof( gui_desc), gui_lang, sizeof(gui_lang));

	pset_i( apc,            os);
	pset_i( gui,            gui);
	pset_c( system,         system);
	pset_c( release,        release);
	pset_c( vendor,         vendor);
	pset_c( architecture,   arch);
	pset_c( guiDescription, gui_desc);
	pset_c( guiLanguage,    gui_lang);

	return newRV_noinc(( SV *) profile);
}

Handle
Application_get_widget_from_handle( Handle self, SV * handle)
{
	ApiHandle apiHandle;
	if ( SvIOK( handle))
		apiHandle = SvUVX( handle);
	else
		apiHandle = sv_2uv( handle);
	return apc_application_get_handle( self, apiHandle);
}

Handle
Application_get_hint_widget( Handle self)
{
	return var->  hintWidget;
}

static Bool
icon_notify ( Handle self, Handle child, Handle icon)
{
	if ( kind_of( child, CWindow) && (( PWidget) child)-> options. optOwnerIcon) {
		CWindow( child)-> set_icon( child, icon);
		PWindow( child)-> options. optOwnerIcon = 1;
	}
	return false;
}

Handle
Application_icon( Handle self, Bool set, Handle icon)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;

	if ( !set)
		return var-> icon;

	if ( icon && !kind_of( icon, CImage)) {
		warn("Illegal object reference passed to Application::icon");
		return NULL_HANDLE;
	}
	if ( icon) {
		icon = ((( PImage) icon)-> self)-> dup( icon);
		++SvREFCNT( SvRV((( PAnyObject) icon)-> mate));
	}
	my-> first_that( self, (void*)icon_notify, (void*)icon);
	if ( var-> icon)
		my-> detach( self, var-> icon, true);
	var-> icon = icon;
	if ( icon && ( list_index_of( var-> components, icon) < 0))
		my-> attach( self, icon);
	return NULL_HANDLE;
}

Handle
Application_get_focused_widget( Handle self)
{
	return apc_widget_get_focused();
}

Handle
Application_get_active_window( Handle self)
{
	return apc_window_get_active();
}

Bool
Application_autoClose( Handle self, Bool set, Bool autoClose)
{
	if ( !set)
		return var->  autoClose;
	return var-> autoClose = autoClose;
}

SV *
Application_sys_action( char * dummy, char * params)
{
	char * i = apc_system_action( params);
	SV * ret = i ? newSVpv( i, 0) : NULL_SV;
	free( i);
	return ret;
}

typedef struct _SingleColor
{
	Color color;
	int   index;
} SingleColor, *PSingleColor;


Color
Application_colorIndex( Handle self, Bool set, int index, Color color)
{
	if ( var->  stage > csFrozen) return clInvalid;
	if (( index < 0) || ( index > ciMaxId)) return clInvalid;
	if ( !set) {
		switch ( index) {
		case ciFore:
			return opt_InPaint ?
				CDrawable-> get_color ( self) : var-> colors[ index];
		case ciBack:
			return opt_InPaint ?
				CDrawable-> get_backColor ( self) : var-> colors[ index];
		default:
			return  var->  colors[ index];
		}
	} else {
		SingleColor s;
		s. color = color;
		s. index = index;
		if ( !opt_InPaint) my-> first_that( self, (void*)prima_single_color_notify, &s);
		if ( opt_InPaint) switch ( index) {
			case ciFore:
				CDrawable-> set_color ( self, color);
				break;
			case ciBack:
				CDrawable-> set_backColor ( self, color);
				break;
		}
		var-> colors[ index] = color;
	}
	return clInvalid;
}

void
Application_set_font( Handle self, Font font)
{
	if ( !opt_InPaint) my-> first_that( self, (void*)prima_font_notify, &font);
	apc_font_pick( self, &font, & var-> font);
	if ( opt_InPaint) apc_gp_set_font ( self, &var-> font);
}

Bool
Application_close( Handle self)
{
	if ( var->  stage > csNormal) return true;
	return my-> can_close( self) ? ( apc_application_close( self), true) : false;
}

Bool
Application_insertMode( Handle self, Bool set, Bool insMode)
{
	if ( !set)
		return apc_sys_get_insert_mode();
	return apc_sys_set_insert_mode( insMode);
}


Handle
Application_get_modal_window( Handle self, int modalFlag, Bool topMost)
{
	if ( modalFlag == mtExclusive) {
		return topMost ? var-> topExclModal   : var-> exclModal;
	} else if ( modalFlag == mtShared) {
		return topMost ? var-> topSharedModal : var-> sharedModal;
	}
	return NULL_HANDLE;
}

SV *
Application_get_monitor_rects( Handle self)
{
	int i, nrects;
	Box * rects = apc_application_get_monitor_rects(self, &nrects);
	AV * ret = newAV();
	for ( i = 0; i < nrects; i++) {
		AV * rect = newAV();
		av_push( rect, newSViv( rects[i].x));
		av_push( rect, newSViv( rects[i].y));
		av_push( rect, newSViv( rects[i].width));
		av_push( rect, newSViv( rects[i].height));
		av_push( ret, newRV_noinc(( SV *) rect));
	}
	free(rects);

	/* .. or return at least the current size */
	if ( nrects == 0) {
		AV * rect = newAV();
		Point sz = apc_application_get_size(self);
		av_push( rect, newSViv( 0));
		av_push( rect, newSViv( 0));
		av_push( rect, newSViv( sz.x));
		av_push( rect, newSViv( sz.y));
		av_push( ret, newRV_noinc(( SV *) rect));
	}

	return newRV_noinc(( SV *) ret);
}


Handle
Application_get_parent( Handle self)
{
	return NULL_HANDLE;
}

Point
Application_get_scroll_rate( Handle self)
{
	Point ret;
	ret. x = apc_sys_get_value( svAutoScrollFirst);
	ret. y = apc_sys_get_value( svAutoScrollNext);
	return ret;
}

static void hshow( Handle self)
{
	PWidget_vmt hintUnder = CWidget( var->  hintUnder);
	SV * text = hintUnder-> get_hint( var->  hintUnder);
	Point size  = hintUnder-> get_size( var->  hintUnder);
	Point s = my-> get_size( self);
	Point fin = {0,0};
	Point pos = fin;
	Point mouse = my-> get_pointerPos( self);
	Point hintSize;
	PWidget_vmt hintWidget = CWidget( var->  hintWidget);

	apc_widget_map_points( var-> hintUnder, true, 1, &pos);

	hintWidget-> set_text( var->  hintWidget, text);
	hintSize = hintWidget-> get_size( var->  hintWidget);

	fin. x = mouse. x - 16;
	fin. y = pos. y - hintSize. y - 1;
	if ( fin. y > mouse. y - hintSize. y - 32) fin. y = mouse. y - hintSize. y - 32;

	if ( fin. x + hintSize. x >= s. x) fin. x = pos. x - hintSize. x;
	if ( fin. x < 0) fin. x = 0;
	if ( fin. y + hintSize. y >= s. y) fin. y = pos. y - hintSize. y;
	if ( fin. y < 0) fin. y = pos. y + size. y + 1;
	if ( fin. y < 0) fin. y = 0;

	hintWidget-> set_origin( var->  hintWidget, fin);
	hintWidget-> show( var->  hintWidget);
	hintWidget-> bring_to_front( var->  hintWidget);
}

void
Application_HintTimer_handle_event( Handle timer, PEvent event)
{
	CComponent-> handle_event( timer, event);
	if ( event-> cmd == cmTimer) {
		Handle self = prima_guts.application;
		CTimer(timer)-> stop( timer);
		if ( var->  hintActive == 1) {
			Event ev = {cmHint};
			if (   !var->hintUnder
				|| apc_application_get_widget_from_point( self, my-> get_pointerPos(self)) != var->hintUnder
				|| PObject( var-> hintUnder)-> stage != csNormal)
				return;
			ev. gen. B = true;
			ev. gen. H = var->  hintUnder;
			var->  hintVisible = 1;
			if (( PWidget( var->  hintUnder)-> stage == csNormal) &&
				( CWidget( var->  hintUnder)-> message( var->  hintUnder, &ev)))
				hshow( self);
		} else if ( var->  hintActive == -1)
			var->  hintActive = 0;
	}
}

void
Application_set_hint_action( Handle self, Handle view, Bool show, Bool byMouse)
{
	if ( var-> stage >= csFrozen) return;
	if ( show && !is_opt( optShowHint)) return;
	if ( show)
	{
		var->  hintUnder = view;
		if ( var->  hintActive == -1)
		{
			Event ev = {cmHint};
			ev. gen. B = true;
			ev. gen. H = view;
			((( PTimer) var->  hintTimer)-> self)-> stop( var-> hintTimer);
			var->  hintVisible = 1;
			if (( PWidget( view)-> stage == csNormal) &&
				( CWidget( view)-> message( view, &ev)))
				hshow( self);
		} else {
			if ( !byMouse && var->  hintActive == 1) return;
			CTimer( var->  hintTimer)-> start( var-> hintTimer);
		}
		var->  hintActive = 1;
	} else {
		int oldHA = var->  hintActive;
		int oldHV = var->  hintVisible;
		if ( oldHA != -1)
			((( PTimer) var-> hintTimer)-> self)-> stop( var-> hintTimer);
		if ( var->  hintVisible)
		{
			Event ev = {cmHint};
			ev. gen. B = false;
			ev. gen. H = view;
			var->  hintVisible = 0;
			if (( PWidget( view)-> stage != csNormal) ||
				( CWidget( view)-> message( view, &ev)))
				CWidget( var->  hintWidget)-> hide( var->  hintWidget);
		}
		if ( oldHA != -1) var->  hintActive = 0;
		if ( byMouse && oldHV) {
			var->  hintActive = -1;
			CTimer( var->  hintTimer)-> start( var->  hintTimer);
		}
	}
}

Color
Application_hintColor( Handle self, Bool set, Color hintColor)
{
	if ( !set)
		return CWidget( var-> hintWidget)-> get_color( var->  hintWidget);
	return CWidget( var->  hintWidget)-> set_color( var->  hintWidget, hintColor);
}

Color
Application_hintBackColor( Handle self, Bool set, Color hintBackColor)
{
	if ( !set)
		return CWidget( var->  hintWidget)-> get_backColor( var-> hintWidget);
	return CWidget( var->  hintWidget)-> set_backColor( var->  hintWidget, hintBackColor);
}

int
Application_hintPause( Handle self, Bool set, int hintPause)
{
	if ( !set)
		return CTimer( var->  hintTimer)-> get_timeout( var->  hintTimer);
	return CTimer( var->  hintTimer)-> set_timeout( var->  hintTimer, hintPause);
}

void
Application_set_hint_font( Handle self, Font hintFont)
{
	CWidget( var-> hintWidget)-> set_font( var->  hintWidget, hintFont);
}


Font
Application_get_hint_font( Handle self)
{
	return CWidget( var->  hintWidget)-> get_font( var->  hintWidget);
}

Bool
Application_showHint( Handle self, Bool set, Bool showHint)
{
	if ( !set)
		return inherited showHint( self, set, showHint);
	opt_assign( optShowHint, showHint);
	return false;
}

Handle Application_next( Handle self) { return self;}
Handle Application_prev( Handle self) { return self;}

Bool
Application_layered( Handle self, Bool set, Bool layered)
{
	return false;
}

SV *
Application_palette( Handle self, Bool set, SV * palette)
{
	return CDrawable-> palette( self, set, palette);
}

Handle
Application_top_frame( Handle self, Handle from)
{
	while ( from) {
		if (
			kind_of( from, CWindow) && (
			( PWidget( from)-> owner == prima_guts.application) ||
			!CWidget( from)-> get_clipOwner(from)
		))
			return from;
		from = PWidget( from)-> owner;
	}
	return prima_guts.application;
}

Handle
Application_get_image( Handle self, int x, int y, int xLen, int yLen)
{
	HV * profile;
	Handle i;
	Bool ret;
	Point sz;
	if ( var->  stage > csFrozen) return NULL_HANDLE;
	if ( x < 0 || y < 0 || xLen <= 0 || yLen <= 0) return NULL_HANDLE;
	sz = apc_application_get_size( self);
	if ( x + xLen > sz. x) xLen = sz. x - x;
	if ( y + yLen > sz. y) yLen = sz. y - y;
	if ( x >= sz. x || y >= sz. y || xLen <= 0 || yLen <= 0) return NULL_HANDLE;

	profile = newHV();
	i = Object_create( "Prima::Image", profile);
	sv_free(( SV *) profile);
	ret = apc_application_get_bitmap( self, i, x, y, xLen, yLen);
	--SvREFCNT( SvRV((( PAnyObject) i)-> mate));
	return ret ? i : NULL_HANDLE;
}

/*
* Cannot return NULL_HANDLE.
*/
Handle
Application_map_focus( Handle self, Handle from)
{
	Handle topFrame = my-> top_frame( self, from);
	Handle topShared;

	if ( var->  topExclModal)
		return ( topFrame == var->  topExclModal) ? from : var->  topExclModal;

	if ( !var->  topSharedModal && var->  modalHorizons. count == 0)
		return from; /* return from if no shared modals active */

	if ( topFrame == self) {
		if ( !var->  topSharedModal) return from;
		topShared = var->  topSharedModal;
	} else {
		Handle horizon =
			( !CWindow( topFrame)-> get_modalHorizon( topFrame)) ?
			CWindow( topFrame)-> get_horizon( topFrame) : topFrame;
		if ( horizon == self)
			topShared = var->  topSharedModal;
		else
			topShared = PWindow( horizon)-> topSharedModal;
	}

	return ( !topShared || ( topShared == topFrame)) ? from : topShared;
}

static Handle
popup_win( Handle xTop)
{
	PWindow_vmt top = CWindow( xTop);
	if ( !top-> get_visible( xTop))
		top-> set_visible( xTop, 1);
	if ( top-> get_windowState( xTop) == wsMinimized)
		top-> set_windowState( xTop, wsNormal);
	top-> set_selected( xTop, 1);
	return xTop;
}

Handle
Application_popup_modal( Handle self)
{
	Handle ha = apc_window_get_active();
	Handle xTop;

	if ( var->  topExclModal) {
	/* checking exclusive modal chain */
		xTop = ( !ha || ( PWindow(ha)->modal == 0)) ? var->  exclModal : ha;
		while ( xTop) {
			if ( PWindow(xTop)-> nextExclModal) {
				CWindow(xTop)-> bring_to_front( xTop);
				xTop = PWindow(xTop)-> nextExclModal;
			} else {
				return popup_win( xTop);
			}
		}
	} else {
		if ( !var->  topSharedModal && var->  modalHorizons. count == 0)
			return NULL_HANDLE; /* return from if no shared modals active */
		/* checking shared modal chains */
		if ( ha) {
			xTop = ( PWindow(ha)->modal == 0) ? CWindow(ha)->get_horizon(ha) : ha;
			if ( xTop == prima_guts.application) xTop = var->  sharedModal;
		} else
			xTop = var->  sharedModal ? var->  sharedModal : var->  modalHorizons. items[ 0];

		while ( xTop) {
			if ( PWindow(xTop)-> nextSharedModal) {
				CWindow(xTop)-> bring_to_front( xTop);
				xTop = PWindow(xTop)-> nextSharedModal;
			} else {
				return popup_win( xTop);
			}
		}
	}

	return NULL_HANDLE;
}

Bool
Application_pointerVisible( Handle self, Bool set, Bool pointerVisible)
{
	if ( !set)
		return apc_pointer_get_visible( self);
	return apc_pointer_set_visible( self, pointerVisible);
}

Point
Application_size( Handle self, Bool set, Point size)
{
	if ( set) return size;
	return apc_application_get_size( self);
}

Point
Application_origin( Handle self, Bool set, Point origin)
{
	Point p = { 0, 0};
	return p;
}

Bool
Application_modalHorizon( Handle self, Bool set, Bool modalHorizon)
{
	return true;
}

Bool
Application_textDirection( Handle self, Bool set, Bool textDirection)
{
	if ( !set ) return var->textDirection;
	return var-> textDirection = textDirection;
}

#define UISCALING_STEP 4
double
Application_uiScaling( Handle self, Bool set, double uiScaling)
{
	if ( !set ) return var->uiScaling;
	if ( uiScaling < 0.00001 ) {
		Point res = my-> get_resolution(self);
		uiScaling  = (double)((int)((double) res.x/ (96.0/UISCALING_STEP) + 0.5)) / UISCALING_STEP; /* 96-143 = 1.5, 144-191 = 1.5 etc */
		if ( uiScaling < 0.25 ) uiScaling = 0.25;
	}
	return var-> uiScaling = uiScaling;
}

Bool
Application_wantUnicodeInput( Handle self, Bool set, Bool want_ui)
{
	if ( !set) return var-> wantUnicodeInput;
	if ( apc_sys_get_value( svCanUTF8_Input))
		var-> wantUnicodeInput = want_ui;
	return 0;
}


void   Application_update_sys_handle( Handle self, HV * profile) {}
Bool   Application_get_capture( Handle self) { return false; }
Bool   Application_set_capture( Handle self, Bool capture, Handle confineTo) { return false; }
void   Application_set_centered( Handle self, Bool x, Bool y) {}

Bool   Application_tabStop( Handle self, Bool set, Bool tabStop)       { return false; }
Bool   Application_selectable( Handle self, Bool set, Bool selectable) { return false; }
Handle Application_shape( Handle self, Bool set, Handle mask)          { return NULL_HANDLE; }
Bool   Application_syncPaint( Handle self, Bool set, Bool syncPaint)   { return false; }
Bool   Application_visible( Handle self, Bool set, Bool visible)       { return true; }
Bool   Application_buffered( Handle self, Bool set, Bool buffered)     { return false; }
Bool   Application_enabled( Handle self, Bool set, Bool enable)        { return true;}
int    Application_growMode( Handle self, Bool set, int flags)         { return 0; }
Bool   Application_hintVisible( Handle self, Bool set, Bool visible)   { return false; }
Handle Application_owner( Handle self, Bool set, Handle owner)         { return NULL_HANDLE; }
Bool   Application_ownerColor( Handle self, Bool set, Bool ownerColor) { return false; }
Bool   Application_ownerBackColor( Handle self, Bool set, Bool ownerBackColor) { return false; }
Bool   Application_ownerFont( Handle self, Bool set, Bool ownerFont)   { return false; }
Bool   Application_ownerShowHint( Handle self, Bool set, Bool ownerShowHint) { return false; }
Bool   Application_ownerPalette( Handle self, Bool set, Bool ownerPalette) { return false; }
Bool   Application_clipChildren( Handle self, Bool set, Bool clip)   { return true; }
Bool   Application_clipOwner( Handle self, Bool set, Bool clip_by_children)   { return false; }
int    Application_tabOrder( Handle self, Bool set, int tabOrder)      { return 0; }
SV   * Application_get_text    ( Handle self)                          { return NULL_SV; }
void   Application_set_text    ( Handle self, SV * text)               { }
Bool   Application_transparent( Handle self, Bool set, Bool transparent) { return false; }
Bool   Application_validate_owner( Handle self, Handle * owner, HV * profile) { *owner = NULL_HANDLE; return true; }

#ifdef __cplusplus
}
#endif
