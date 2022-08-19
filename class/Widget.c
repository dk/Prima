#include "apricot.h"
#include "Application.h"
#include "Icon.h"
#include "Popup.h"
#include "Region.h"
#include "Widget.h"
#include "Window.h"
#include <Widget.inc>

#ifdef __cplusplus
extern "C" {
#endif

#undef  set_text
#undef  get_text
#undef  my
#define inherited CDrawable
#define enter_method PWidget_vmt selfvmt = ((( PWidget) self)-> self)
#define my  selfvmt
#define var (( PWidget) self)

typedef Bool ActionProc ( Handle self, Handle item, void * params);
typedef ActionProc *PActionProc;
#define his (( PWidget) child)

/* local defines */
typedef struct _SingleColor
{
	Color color;
	int   index;
} SingleColor, *PSingleColor;

static Bool get_top_current( Handle self);
static Bool sptr( Handle window, Handle self, void * v);
static Handle find_tabfoc( Handle self);
static Bool showhint_notify ( Handle self, Handle child, void * data);
static Bool hint_notify ( Handle self, Handle child, SV * hint);

/* init, done & update_sys_handle */
void
Widget_init( Handle self, HV * profile)
{
	dPROFILE;
	enter_method;
	SV * sv;
	int geometry;

	opt_set(optSystemDrawable);
	inherited-> init( self, profile);

	list_create( &var-> widgets, 0, 8);
	var-> tabOrder = -1;

	var-> geomInfo. side = 3; /* default pack side is 'top', anchor ='center' */
	var-> geomInfo. anchorx = var-> geomInfo. anchory = 1;

	my-> update_sys_handle( self, profile);
	/* props init */
	/* font and colors */
	SvHV_Font( pget_sv( font), &Font_buffer, "Widget::init");
	my-> set_widgetClass        ( self, pget_i( widgetClass  ));
	my-> set_color              ( self, pget_i( color        ));
	my-> set_backColor          ( self, pget_i( backColor    ));
	my-> set_font               ( self, Font_buffer);
	opt_assign( optOwnerBackColor, pget_B( ownerBackColor));
	opt_assign( optOwnerColor    , pget_B( ownerColor));
	opt_assign( optOwnerFont     , pget_B( ownerFont));
	opt_assign( optOwnerHint     , pget_B( ownerHint));
	opt_assign( optOwnerShowHint , pget_B( ownerShowHint));
	opt_assign( optOwnerPalette  , pget_B( ownerPalette));
	my-> colorIndex( self, true,  ciHiliteText,   pget_i( hiliteColor)      );
	my-> colorIndex( self, true,  ciHilite,       pget_i( hiliteBackColor)  );
	my-> colorIndex( self, true,  ciDisabledText, pget_i( disabledColor)    );
	my-> colorIndex( self, true,  ciDisabled,     pget_i( disabledBackColor));
	my-> colorIndex( self, true,  ciLight3DColor, pget_i( light3DColor)     );
	my-> colorIndex( self, true,  ciDark3DColor,  pget_i( dark3DColor)      );
	my-> set_palette( self, pget_sv( palette));

	/* light props */
	my-> set_autoEnableChildren ( self, pget_B( autoEnableChildren));
	my-> set_briefKeys          ( self, pget_B( briefKeys));
	my-> set_buffered           ( self, pget_B( buffered));
	my-> set_clipChildren       ( self, pget_B( clipChildren));
	my-> set_cursorVisible      ( self, pget_B( cursorVisible));
	my-> set_dndAware           ( self, pget_sv( dndAware));
	my-> set_growMode           ( self, pget_i( growMode));
	my-> set_helpContext        ( self, pget_sv( helpContext));
	my-> set_hint               ( self, pget_sv( hint));
	my-> set_firstClick         ( self, pget_B( firstClick));
	{
		Point hotSpot;
		Handle icon = pget_H( pointerIcon);
		prima_read_point( pget_sv( pointerHotSpot), (int*)&hotSpot, 2, "Array panic on 'pointerHotSpot'");
		if ( icon != NULL_HANDLE && !kind_of( icon, CIcon)) {
			warn("Illegal object reference passed to Widget::pointerIcon");
			icon = NULL_HANDLE;
		}
		apc_pointer_set_user( self, icon, hotSpot);
	}
	my-> set_pointerType        ( self, pget_i( pointerType));
	my-> set_selectingButtons   ( self, pget_i( selectingButtons));
	my-> set_selectable         ( self, pget_B( selectable));
	my-> set_showHint           ( self, pget_B( showHint));
	my-> set_tabOrder           ( self, pget_i( tabOrder));
	my-> set_tabStop            ( self, pget_B( tabStop));
	my-> set_text               ( self, pget_sv( text));

	opt_assign( optScaleChildren, pget_B( scaleChildren));

	/* subcomponents props */
	my-> popupColorIndex( self, true, ciFore,         pget_i( popupColor)              );
	my-> popupColorIndex( self, true, ciBack,         pget_i( popupBackColor)          );
	my-> popupColorIndex( self, true, ciHiliteText,   pget_i( popupHiliteColor)        );
	my-> popupColorIndex( self, true, ciHilite,       pget_i( popupHiliteBackColor)    );
	my-> popupColorIndex( self, true, ciDisabledText, pget_i( popupDisabledColor)      );
	my-> popupColorIndex( self, true, ciDisabled,     pget_i( popupDisabledBackColor)  );
	my-> popupColorIndex( self, true, ciLight3DColor, pget_i( popupLight3DColor)       );
	my-> popupColorIndex( self, true, ciDark3DColor,  pget_i( popupDark3DColor)        );
	SvHV_Font( pget_sv( popupFont), &Font_buffer, "Widget::init");
	my-> set_popup_font  ( self, Font_buffer);
	if ( SvTYPE( sv = pget_sv( popupItems)) != SVt_NULL)
		my-> set_popupItems( self, sv);
	if ( SvTYPE( sv = pget_sv( accelItems)) != SVt_NULL)
		my-> set_accelItems( self, sv);

	/* size, position, enabling, visibliity etc. runtime */
	{
		Point set, set2;
		AV * av;
		SV ** holder;
		NPoint ds = {1,1};

		prima_read_point( pget_sv( sizeMin), (int*)&set, 2, "Array panic on 'sizeMin'");
		prima_read_point( pget_sv( sizeMax), (int*)&set2, 2, "Array panic on 'sizeMax'");
		var-> sizeMax = set2;
		my-> set_sizeMin( self, set);
		my-> set_sizeMax( self, set2);
		prima_read_point( pget_sv( cursorSize), (int*)&set, 2, "Array panic on 'cursorSize'");
		my-> set_cursorSize( self, set);
		prima_read_point( pget_sv( cursorPos), (int*)&set, 2, "Array panic on 'cursorPos'");
		my-> set_cursorPos( self, set);

		av = ( AV *) SvRV( pget_sv( designScale));
		holder = av_fetch( av, 0, 0);
		ds. x = holder ? SvNV( *holder) : 1;
		if ( !holder) warn("Array panic on 'designScale'");
		holder = av_fetch( av, 1, 0);
		ds. y = holder ? SvNV( *holder) : 1;
		if ( !holder) warn("Array panic on 'designScale'");
		my-> set_designScale( self, ds);
	}
	my-> set_enabled     ( self, pget_B( enabled));
	if ( !pexist( originDontCare) || !pget_B( originDontCare)) {
		Point pos;
		pos. x = pget_i( left);
		pos. y = pget_i( bottom);
		my-> set_origin( self, pos);
	} else
		var-> pos = my-> get_origin( self);

	if ( !pexist( sizeDontCare  ) || !pget_B( sizeDontCare  )) {
		Point size;
		size. x = pget_i( width);
		size. y = pget_i( height);
		my-> set_size( self, size);
	} else
		var-> virtualSize = my-> get_size( self);
	var-> geomSize = var-> virtualSize;

	geometry = pget_i(geometry);
	if ( geometry == gtGrowMode ) {
		Bool x = 0, y = 0;
		if ( pget_B( centered)) { x = 1; y = 1; };
		if ( pget_B( x_centered) || ( var-> growMode & gmXCenter)) x = 1;
		if ( pget_B( y_centered) || ( var-> growMode & gmYCenter)) y = 1;
		if ( x || y) my-> set_centered( self, x, y);
		var-> geomInfo. x = x;
		var-> geomInfo. y = y;
	}

	opt_assign( optPackPropagate, pget_B( packPropagate));
	my-> set_packInfo( self, pget_sv( packInfo));
	my-> set_placeInfo( self, pget_sv( placeInfo));
	my-> set_geometry( self, geometry);

	my-> set_shape       ( self, pget_H(  shape));
	my-> set_visible     ( self, pget_B( visible));
	if ( pget_B( capture)) my-> set_capture( self, 1, NULL_HANDLE);
	if ( pget_B( current)) my-> set_current( self, 1);
	CORE_INIT_TRANSIENT(Widget);

	{
		SV * widgets = pget_sv( widgets);
		if ( SvTYPE( widgets) != SVt_NULL) {
			dSP;
			ENTER;
			SAVETMPS;
			PUSHMARK( sp);
			XPUSHs( var-> mate);
			XPUSHs( sv_2mortal( newSVsv( widgets)));
			PUTBACK;
			perl_call_method( "widgets", G_DISCARD);
			SPAGAIN;
			PUTBACK;
			FREETMPS;
			LEAVE;
		}
	}
}

void
Widget_update_sys_handle( Handle self, HV * profile)
{
	dPROFILE;
	enter_method;
	Handle    owner;
	Bool      clipOwner, layered, syncPaint, transparent;
	ApiHandle parentHandle;
	if (!(
		pexist( owner) ||
		pexist( syncPaint) ||
		pexist( clipOwner) ||
		pexist( layered) ||
		pexist( transparent)
	)) return;

	owner        = pexist( owner)        ? pget_H( owner)        : var-> owner;
	clipOwner    = pexist( clipOwner)    ? pget_B( clipOwner)    : my-> get_clipOwner( self);
	parentHandle = pexist( parentHandle) ? pget_i( parentHandle) : apc_widget_get_parent_handle( self);
	layered      = pexist( layered)      ? pget_B( layered)      : my-> get_layered(self);
	syncPaint    = pexist( syncPaint)    ? pget_B( syncPaint)    : my-> get_syncPaint( self);
	transparent  = pexist( transparent)  ? pget_B( transparent)  : my-> get_transparent( self);

	if ( parentHandle) {
		if (( owner != prima_guts.application) && clipOwner)
			croak("Cannot accept 'parentHandle' for non-application child and clip-owner widget");
	}

	if ( !apc_widget_create( self, owner, syncPaint, clipOwner, transparent, parentHandle, layered))
		croak( "Cannot create widget");

	pdelete( transparent);
	pdelete( syncPaint);
	pdelete( clipOwner);
	pdelete( parentHandle);
	pdelete( layered);
}

void
Widget_done( Handle self)
{
	if ( var-> dndAware != NULL ) {
		free( var-> dndAware );
		var-> dndAware = NULL;
	}
	if ( var-> text ) sv_free( var->text);
	var-> text = NULL;
	apc_widget_destroy( self);
	if ( var-> hint ) sv_free( var->hint);
	var-> hint = NULL;
	free( var-> helpContext);
	var-> helpContext = NULL;

	if ( var-> owner) {
		Handle * enum_lists = PWidget( var-> owner)-> enum_lists;
		while ( enum_lists) {
			unsigned int i, count;
			count = (unsigned int) enum_lists[1];
			for ( i = 2; i < count + 2; i++) {
				if ( self == enum_lists[i])
					enum_lists[i] = NULL_HANDLE;
			}
			enum_lists = ( Handle*) enum_lists[0];
		}
	}

	list_destroy( &var-> widgets);
	inherited-> done( self);
}

/* ::a */
void
Widget_attach( Handle self, Handle objectHandle)
{
	if ( objectHandle == NULL_HANDLE) return;
	if ( var-> stage > csNormal) return;
	if ( kind_of( objectHandle, CWidget)) {
		if ( list_index_of( &var-> widgets, objectHandle) >= 0) {
			warn( "Object attach failed");
			return;
		}
		list_add( &var-> widgets, objectHandle);
	}
	inherited-> attach( self, objectHandle);
}

/*::b */
Bool
Widget_begin_paint( Handle self)
{
	Bool ok;
	if ( !inherited-> begin_paint( self))
		return false;
	if ( !( ok = apc_widget_begin_paint( self, false))) {
		inherited-> end_paint( self);
		perl_error();
	}
	return ok;
}

Bool
Widget_begin_paint_info( Handle self)
{
	Bool ok;
	if ( is_opt( optInDraw))     return true;
	if ( !inherited-> begin_paint_info( self))
		return false;
	if ( !( ok = apc_widget_begin_paint_info( self))) {
		inherited-> end_paint_info( self);
		perl_error();
	}
	return ok;
}


void
Widget_bring_to_front( Handle self)
{
	if ( opt_InPaint) return;
	apc_widget_set_z_order( self, NULL_HANDLE, true);
}

/*::c */
Bool
Widget_can_close( Handle self)
{
	enter_method;
	Event ev = { cmClose};
	return ( var-> stage <= csNormal) ? my-> message( self, &ev) : true;
}

Bool
Widget_can_propagate_key( Handle self)
{
	return true;
}

void
Widget_cleanup( Handle self)
{
	Handle ptr;
	enter_method;

	/* disconnect all geometry slaves */
	ptr = var-> packSlaves;
	while ( ptr) {
		PWidget( ptr)-> geometry = gtDefault;
		ptr = PWidget( ptr)-> geomInfo. next;
	}
	var-> packSlaves = NULL_HANDLE;
	ptr = var-> placeSlaves;
	while ( ptr) {
		PWidget( ptr)-> geometry = gtDefault;
		ptr = PWidget( ptr)-> geomInfo. next;
	}
	var-> placeSlaves = NULL_HANDLE;

	my-> set_geometry( self, gtDefault);

	if ( prima_guts.application && P_APPLICATION-> hintUnder == self)
		my-> set_hintVisible( self, 0);

	{
		int i;
		for ( i = 0; i < var-> widgets. count; i++)
			Object_destroy( var-> widgets. items[i]);
	}

	if ( var-> accelTable) {
		unprotect_object(var-> accelTable);
		//my-> detach( self, var-> accelTable, true);
		var-> accelTable = NULL_HANDLE;
	}

	if ( var-> popupMenu ) {
		unprotect_object(var-> popupMenu);
		//my-> detach( self, var-> popupMenu, true);
		var-> popupMenu = NULL_HANDLE;
	}

	inherited-> cleanup( self);
}


Bool
Widget_close( Handle self)
{
	Bool canClose;
	enter_method;
	if ( var-> stage > csNormal) return true;
	if (( canClose = my-> can_close( self))) {
		Object_destroy( self);
	}
	return canClose;
}

Bool
Widget_custom_paint( Handle self)
{
	PList  list;
	void * ret;
	enter_method;
	if ( my-> on_paint != Widget_on_paint) return true;
	if ( var-> eventIDs == NULL) return false;
	ret = hash_fetch( var-> eventIDs, "Paint", 5);
	if ( ret == NULL) return false;
	list = var-> events + PTR2UV( ret) - 1;
	return list-> count > 0;
}

/*::d */
void
Widget_detach( Handle self, Handle objectHandle, Bool kill)
{
	enter_method;
	if ( kind_of( objectHandle, CWidget)) {
		list_delete( &var-> widgets, objectHandle);
		if ( var-> currentWidget == objectHandle && objectHandle != NULL_HANDLE)
			my-> set_currentWidget( self, NULL_HANDLE);
	}
	inherited-> detach( self, objectHandle, kill);
}

DNDResp
Widget_dnd_start(Handle self, int dnd_actions, Bool default_pointers)
{
	DNDResp ret;
	ret.action = apc_dnd_start( self, dnd_actions, default_pointers, &ret.counterpart);
	return ret;
}

/*::e */
void
Widget_end_paint( Handle self)
{
	if ( !is_opt( optInDraw)) return;
	apc_widget_end_paint( self);
	inherited-> end_paint( self);
}

void
Widget_end_paint_info( Handle self)
{
	if ( !is_opt( optInDrawInfo)) return;
	apc_widget_end_paint_info( self);
	inherited-> end_paint_info( self);
}


/*::f */

SV*
Widget_fetch_resource( char *className, char *name, char *classRes, char *res, Handle owner, int resType)
{
	char *str = NULL;
	Color clr;
	void *parm;
	Font font;
	SV * ret = NULL_SV;
	char *r_className, *r_name, *r_classRes, *r_res;

	switch ( resType) {
	case frColor:
		parm = &clr; break;
	case frFont:
		parm = &font;
		bzero( &font, sizeof( font));
		break;
	default:
		parm = &str;
		resType = frString;
	}

	r_className = duplicate_string(className);
	r_name      = duplicate_string(name);
	r_classRes  = duplicate_string(classRes);
	r_res       = duplicate_string(res);

	if ( !apc_fetch_resource(
		prima_normalize_resource_string( r_className, true),
		prima_normalize_resource_string( r_name, false),
		prima_normalize_resource_string( r_classRes, true),
		prima_normalize_resource_string( r_res, false),
		owner, resType, parm))
		goto FAIL;

	switch ( resType) {
	case frColor:
		ret = newSViv( clr);
		break;
	case frFont:
		ret = sv_Font2HV( &font);
		break;
	default:
		ret = str ? newSVpv( str, 0) : NULL_SV;
		free( str);
	}

FAIL:
	free(r_className);
	free(r_name);
	free(r_classRes);
	free(r_res);

	return ret;
}

Handle
Widget_first( Handle self)
{
	return apc_widget_get_z_order( self, zoFirst);
}

Handle
Widget_first_that( Handle self, void * actionProc, void * params)
{
	Handle child  = NULL_HANDLE;
	int i, count  = var-> widgets. count;
	Handle * list;
	if ( actionProc == NULL || count == 0) return NULL_HANDLE;
	if (!(list = allocn( Handle, count + 2))) return NULL_HANDLE;

	list[0] = (Handle)( var-> enum_lists);
	list[1] = (Handle)( count);
	var-> enum_lists = list;
	memcpy( list + 2, var-> widgets. items, sizeof( Handle) * count);

	for ( i = 2; i < count + 2; i++)
	{
		if ( list[i] && (( PActionProc) actionProc)( self, list[ i], params))
		{
			child = list[ i];
			break;
		}
	}
	var-> enum_lists = (Handle*)(*list);
	free( list);
	return child;
}

/*::g */

/*::h */

void
Widget_hide( Handle self)
{
	enter_method;
	my-> set_visible( self, false);
}

void
Widget_hide_cursor( Handle self)
{
	enter_method;
	if ( my-> get_cursorVisible( self))
		my-> set_cursorVisible( self, false);
	else
		var-> cursorLock++;
}

/*::i */
void
Widget_insert_behind ( Handle self, Handle widget)
{
	apc_widget_set_z_order( self, widget, 0);
}

void
Widget_invalidate_rect( Handle self, Rect rect)
{
	enter_method;
	if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
		apc_widget_invalidate_rect( self, &rect);
}


Bool
Widget_is_child( Handle self, Handle owner)
{
	if ( !owner)
		return false;
	while ( self) {
		if ( self == owner)
			return true;
		self = var-> owner;
	}
	return false;
}

/*::j */

/*::l */
Handle
Widget_last( Handle self)
{
	return apc_widget_get_z_order( self, zoLast);
}

Bool
Widget_lock( Handle self)
{
	var-> lockCount++;
	return true;
}

/*::m */
/*::n */
Handle
Widget_next( Handle self)
{
	return apc_widget_get_z_order( self, zoNext);
}

/*::o */
/*::p */

Handle
Widget_prev( Handle self)
{
	return apc_widget_get_z_order( self, zoPrev);
}

/*::q */
/*::r */
void
Widget_repaint( Handle self)
{
	enter_method;
	if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
		apc_widget_invalidate_rect( self, NULL);
}

/*::s */
int
Widget_scroll( Handle self, int dx, int dy, Rect *confine, Rect *clip, Bool withChildren)
{
	enter_method;
	if ( !opt_InPaint && ( var-> stage == csNormal) && !my-> get_locked( self))
		return apc_widget_scroll( self, dx, dy, confine, clip, withChildren);
	return scrError;
}

int
Widget_scroll_REDEFINED( Handle self, int dx, int dy, Rect *confine, Rect *clip, Bool withChildren)
{
	warn("Invalid call of Widget::scroll");
	return scrError;
}

XS( Widget_scroll_FROMPERL)
{
	dPROFILE;
	dXSARGS;
	Handle self;
	int dx, dy, ret;
	Rect *confine = NULL;
	Rect *clip = NULL;
	Rect confine_rect, clip_rect;
	Bool withChildren = false;
	HV *profile;
	int rect[4];

	if ( items < 3 || (items - 3) % 2) goto invalid_usage;
	if (!( self = gimme_the_mate( ST(0)))) goto invalid_usage;
	dx = SvIV( ST(1));
	dy = SvIV( ST(2));
	profile = parse_hv( ax, sp, items, mark, 3, "Widget::scroll");
	if ( pexist( confineRect)) {
		prima_read_point( pget_sv( confineRect), rect, 4, "Array panic on 'confineRect'");
		confine = &confine_rect;
		confine-> left = rect[0];
		confine-> bottom = rect[1];
		confine-> right = rect[2];
		confine-> top = rect[3];
	}
	if ( pexist( clipRect)) {
		prima_read_point( pget_sv( clipRect), rect, 4, "Array panic on 'clipRect'");
		clip = &clip_rect;
		clip-> left = rect[0];
		clip-> bottom = rect[1];
		clip-> right = rect[2];
		clip-> top = rect[3];
	}
	if ( pexist( withChildren)) withChildren = pget_B( withChildren);
	sv_free((SV*)profile);
	ret = Widget_scroll( self, dx, dy, confine, clip, withChildren);
	SPAGAIN;
	SP -= items;
	XPUSHs( sv_2mortal( newSViv( ret)));
	PUTBACK;
	return;
invalid_usage:
	croak ("Invalid usage of %s", "Widget::scroll");
}

void
Widget_send_to_back( Handle self)
{
	apc_widget_set_z_order( self, NULL_HANDLE, false);
}

void
Widget_set( Handle self, HV * profile)
{
	dPROFILE;
	enter_method;
	Handle postOwner = NULL_HANDLE;
	AV *order = NULL;
	int geometry = gtDefault;

	if ( pexist(__ORDER__)) order = (AV*)SvRV(pget_sv( __ORDER__));

	if ( pexist( owner)) {
		if ( !my-> validate_owner( self, &postOwner, profile))
			croak( "Illegal 'owner' reference passed to %s::%s", my-> className, "set");
		if ( postOwner != var-> owner) {
			if ( is_opt( optOwnerColor)) {
				my-> set_color( self, CWidget( postOwner)-> get_color( postOwner));
				opt_set( optOwnerColor);
			}
			if ( is_opt( optOwnerBackColor)) {
				my-> set_backColor( self, CWidget( postOwner)-> get_backColor( postOwner));
				opt_set( optOwnerBackColor);
			}
			if ( is_opt( optOwnerShowHint)) {
				Bool newSH = ( postOwner == prima_guts.application) ? 1 :
					CWidget( postOwner)-> get_showHint( postOwner);
				my-> set_showHint( self, newSH);
				opt_set( optOwnerShowHint);
			}
			if ( is_opt( optOwnerHint)) {
				my-> set_hint( self, CWidget( postOwner)-> get_hint( postOwner));
				opt_set( optOwnerHint);
			}
			if ( is_opt( optOwnerFont)) {
				my-> set_font ( self, CWidget( postOwner)-> get_font( postOwner));
				opt_set( optOwnerFont);
			}
		}
		if ( var-> geometry != gtDefault) {
			geometry = var-> geometry;
			my-> set_geometry( self, gtDefault);
		}
	}

	/* geometry manipulations */
	{
#define iLEFT   0
#define iRIGHT  1
#define iTOP    2
#define iBOTTOM 3
#define iWIDTH  4
#define iHEIGHT 5
		int i, count;
		Bool exists[ 6];
		int  values[ 6];

		bzero( values, sizeof(values));
		if ( pexist( origin))
		{
			int set[2];
			if (order && !pexist(left))   av_push( order, newSVpv("left",0));
			if (order && !pexist(bottom)) av_push( order, newSVpv("bottom",0));
			prima_read_point( pget_sv( origin), set, 2, "Array panic on 'origin'");
			pset_i( left,   set[0]);
			pset_i( bottom, set[1]);
			pdelete( origin);
		}
		if ( pexist( rect))
		{
			int rect[4];
			if (order && !pexist(left)) av_push( order, newSVpv("left",0));
			if (order && !pexist(bottom)) av_push( order, newSVpv("bottom",0));
			if (order && !pexist(width)) av_push( order, newSVpv("width",0));
			if (order && !pexist(height)) av_push( order, newSVpv("height",0));
			prima_read_point( pget_sv( rect), rect, 4, "Array panic on 'rect'");
			pset_i( left,   rect[0]);
			pset_i( bottom, rect[1]);
			pset_i( width,  rect[2] - rect[0]);
			pset_i( height, rect[3] - rect[1]);
			pdelete( rect);
		}
		if ( pexist( size))
		{
			int set[2];
			if (order && !pexist(width)) av_push( order, newSVpv("width",0));
			if (order && !pexist(height)) av_push( order, newSVpv("height",0));
			prima_read_point( pget_sv( size), set, 2, "Array panic on 'size'");
			pset_i( width,  set[0]);
			pset_i( height, set[1]);
			pdelete( size);
		}

		if (( exists[ iLEFT]   = pexist( left)))    values[ iLEFT]   = pget_i( left);
		if (( exists[ iRIGHT]  = pexist( right)))   values[ iRIGHT]  = pget_i( right);
		if (( exists[ iTOP]    = pexist( top)))     values[ iTOP]    = pget_i( top);
		if (( exists[ iBOTTOM] = pexist( bottom ))) values[ iBOTTOM] = pget_i( bottom);
		if (( exists[ iWIDTH]  = pexist( width)))   values[ iWIDTH]  = pget_i( width);
		if (( exists[ iHEIGHT] = pexist( height)))  values[ iHEIGHT] = pget_i( height);

		count = 0;
		for ( i = 0; i < 6; i++) if ( exists[ i]) count++;

		if ( count > 1) {
			if ( exists[ iWIDTH] && exists[ iRIGHT] && exists[ iLEFT]) {
				exists[ iRIGHT] = 0;
				count--;
			}
			if ( exists[ iHEIGHT] && exists[ iTOP] && exists[ iBOTTOM]) {
				exists[ iTOP] = 0;
				count--;
			}
			if ( exists[ iRIGHT] && exists[ iLEFT]) {
				exists[ iWIDTH] = 1;
				values[ iWIDTH] = values[ iRIGHT] - values[ iLEFT];
				exists[ iRIGHT] = 0;
			}
			if ( exists[ iTOP] && exists[ iBOTTOM]) {
				exists[ iHEIGHT] = 1;
				values[ iHEIGHT] = values[ iTOP] - values[ iBOTTOM];
				exists[ iTOP]    = 0;
			}

			if (
				( count == 2) &&
				(
					( exists[ iLEFT]  && exists[ iBOTTOM]) ||
					( exists[ iWIDTH] && exists[ iHEIGHT])
				)
				) {
				Point p;
				if ( exists[ iLEFT]) {
					p. x = values[ iLEFT];
					p. y = values[ iBOTTOM];
					my-> set_origin( self, p);
				} else {
					p. x = values[ iWIDTH];
					p. y = values[ iHEIGHT];
					my-> set_size( self, p);
				}
			} else {
				Rect r;
				if ( !exists[ iWIDTH] || !exists[ iHEIGHT]) {
					Point sz;
					sz = my-> get_size( self);
					if ( !exists[ iWIDTH])  values[ iWIDTH]  = sz. x;
					if ( !exists[ iHEIGHT]) values[ iHEIGHT] = sz. y;
					exists[ iWIDTH] = exists[ iHEIGHT] = 1;
				}
				if ( ( !exists[ iLEFT]   && !exists[ iRIGHT]) ||
					( !exists[ iBOTTOM] && !exists[ iTOP])) {
					Point pos;
					pos = my-> get_origin( self);
					if ( !exists[ iLEFT])   values[ iLEFT]   = pos. x;
					if ( !exists[ iBOTTOM]) values[ iBOTTOM] = pos. y;
					exists[ iLEFT] = exists[ iBOTTOM] = 1;
				}
				if ( !exists[ iLEFT]) {
					exists[ iLEFT] = 1;
					values[ iLEFT] = values[ iRIGHT] - values[ iWIDTH];
				}
				if ( !exists[ iBOTTOM]) {
					exists[ iBOTTOM] = 1;
					values[ iBOTTOM] = values[ iTOP] - values[ iHEIGHT];
				}
				r. left   = values[ iLEFT];
				r. bottom = values[ iBOTTOM];
				r. right  = values[ iLEFT] + values[ iWIDTH];
				r. top    = values[ iBOTTOM] + values[ iHEIGHT];
				my-> set_rect( self, r);
			}
			pdelete( left);
			pdelete( right);
			pdelete( top);
			pdelete( bottom);
			pdelete( width);
			pdelete( height);
		} /* count > 1 */
	}
	if ( pexist( popupFont))
	{
		SvHV_Font( pget_sv( popupFont), &Font_buffer, "Widget::set");
		my-> set_popup_font( self, Font_buffer);
		pdelete( popupFont);
	}
	if ( pexist( pointerIcon) && pexist( pointerHotSpot))
	{
		Point hotSpot;
		Handle icon = pget_H( pointerIcon);
		prima_read_point( pget_sv( pointerHotSpot), (int*)&hotSpot, 2, "Array panic on 'pointerHotSpot'");
		if ( icon != NULL_HANDLE && !kind_of( icon, CIcon)) {
			warn("Illegal object reference passed to Widget.set_pointer_icon");
			icon = NULL_HANDLE;
		}
		apc_pointer_set_user( self, icon, hotSpot);
		if ( var-> pointerType == crUser) my-> first_that( self, (void*)sptr, NULL);
		pdelete( pointerIcon);
		pdelete( pointerHotSpot);
	}
	if ( pexist( designScale))
	{
		AV * av = ( AV *) SvRV( pget_sv( designScale));
		SV ** holder = av_fetch( av, 0, 0);
		NPoint ds = {1,1};
		ds. x = holder ? SvNV( *holder) : 1;
		if ( !holder) warn("Array panic on 'designScale'");
		holder = av_fetch( av, 1, 0);
		ds. y = holder ? SvNV( *holder) : 1;
		if ( !holder) warn("Array panic on 'designScale'");
		my-> set_designScale( self, ds);
		pdelete( designScale);
	}
	if ( pexist( sizeMin)) {
		Point set;
		prima_read_point( pget_sv( sizeMin), (int*)&set, 2, "Array panic on 'sizeMin'");
		my-> set_sizeMin( self, set);
		pdelete( sizeMin);
	}
	if ( pexist( sizeMax)) {
		Point set;
		prima_read_point( pget_sv( sizeMax), (int*)&set, 2, "Array panic on 'sizeMax'");
		my-> set_sizeMax( self, set);
		pdelete( sizeMax);
	}
	if ( pexist( cursorSize)) {
		Point set;
		prima_read_point( pget_sv( cursorSize), (int*)&set, 2, "Array panic on 'cursorSize'");
		my-> set_cursorSize( self, set);
		pdelete( cursorSize);
	}
	if ( pexist( cursorPos)) {
		Point set;
		prima_read_point( pget_sv( cursorPos), (int*)&set, 2, "Array panic on 'cursorPos'");
		my-> set_cursorPos( self, set);
		pdelete( cursorPos);
	}
	if ( pexist( geomSize))
	{
		Point set;
		prima_read_point( pget_sv( geomSize), (int*)&set, 2, "Array panic on 'geomSize'");
		my-> set_geomSize( self, set);
		pdelete( geomSize);
	}

	inherited-> set( self, profile);
	if ( postOwner) {
		my-> set_tabOrder( self, var-> tabOrder);
		my-> set_geometry( self, geometry);
	}
}

void
Widget_setup( Handle self)
{
	enter_method;
	if ( var-> geometry == gtGrowMode && ( var-> geomInfo.x != 0 || var-> geomInfo. y != 0 ))
		my-> set_centered( self, var-> geomInfo. x, var-> geomInfo. y);
	if ( get_top_current( self) &&
		my-> get_enabled( self) &&
		my-> get_visible( self))
		my-> set_selected( self, true);
	inherited-> setup( self);
}

void
Widget_show( Handle self)
{
	enter_method;
	my-> set_visible( self, true);
}

void
Widget_show_cursor( Handle self)
{
	enter_method;
	if ( var-> cursorLock-- <= 0) {
		my-> set_cursorVisible( self, true);
		var-> cursorLock = 0;
	}
}

/*::t */
/*::u */

static Bool
repaint_all( Handle owner, Handle self, void * dummy)
{
	enter_method;
	my-> repaint( self);
	my-> first_that( self, (void*)repaint_all, NULL);
	return false;
}

Bool
Widget_unlock( Handle self)
{
	if ( --var-> lockCount <= 0) {
		var-> lockCount = 0;
		repaint_all( var-> owner, self, NULL);
	}
	return true;
}

void
Widget_update_view( Handle self)
{
	if ( !opt_InPaint) apc_widget_update( self);
}

/*::v */

Bool
Widget_validate_owner( Handle self, Handle * owner, HV * profile)
{
	dPROFILE;
	*owner = pget_H( owner);
	if ( !kind_of( *owner, CWidget)) return false;
	return inherited-> validate_owner( self, owner, profile);
}

/*::w */
/*::x */
/*::y */
/*::z */

/* get_props() */

Font
Widget_get_default_font( char * dummy)
{
	Font font;
	apc_widget_default_font( &font);
	return font;
}

Font
Widget_get_default_popup_font( char * dummy)
{
	Font f;
	apc_popup_default_font( &f);
	return f;
}


NPoint
Widget_designScale( Handle self, Bool set, NPoint designScale)
{
	if ( !set)
		return var-> designScale;
	if ( designScale. x < 0) designScale. x = 0;
	if ( designScale. y < 0) designScale. y = 0;
	var-> designScale = designScale;
	return designScale;
}

int
Widget_growMode( Handle self, Bool set, int growMode)
{
	enter_method;
	Bool x = false, y = false;
	if ( !set)
		return var-> growMode;
	var-> growMode = growMode;
	if ( var-> growMode & gmXCenter) x = true;
	if ( var-> growMode & gmYCenter) y = true;
	if ( var-> geometry == gtGrowMode && (x || y)) my-> set_centered( self, x, y);
	return var-> growMode;
}

SV *
Widget_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_widget_get_handle( self));
	return newSVpv( buf, 0);
}

SV *
Widget_get_parent_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_widget_get_parent_handle( self));
	return newSVpv( buf, 0);
}

int
Widget_hintVisible( Handle self, Bool set, int hintVisible)
{
	Bool wantVisible;
	if ( !set)
		return P_APPLICATION-> hintVisible;
	if ( var-> stage >= csDead) return false;
	wantVisible = ( hintVisible != 0);
	if ( wantVisible == P_APPLICATION-> hintVisible) return false;
	if ( wantVisible) {
		if ( SvTYPE(var->hint) == SVt_NULL || (SvCUR( var-> hint) == 0)) return false;
		if ( hintVisible > 0) P_APPLICATION-> hintActive = -1; /* immediate */
	}
	C_APPLICATION-> set_hint_action( prima_guts.application, self, wantVisible, false);
	return false;
}

Bool
Widget_get_locked( Handle self)
{
	while ( self) {
		if ( var-> lockCount != 0) return true;
		self = var-> owner;
	}
	return false;
}


Handle
Widget_get_parent( Handle self)
{
	enter_method;
	return my-> get_clipOwner( self) ? var-> owner : prima_guts.application;
}

Point
Widget_get_pointer_size( char*dummy)
{
	return apc_pointer_get_size( NULL_HANDLE);
}

Font
Widget_get_popup_font( Handle self)
{
	return var-> popupFont;
}

Handle
Widget_get_selectee( Handle self)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( is_opt( optSelectable))
		return self;
	else if ( var-> currentWidget) {
		PWidget w = ( PWidget) var-> currentWidget;
		if ( w-> options. optSystemSelectable && !w-> self-> get_clipOwner(( Handle) w))
			return ( Handle) w;
		else
			return w-> self-> get_selectee(( Handle) w);
	} else if ( is_opt( optSystemSelectable))
		return self;
	else
		return find_tabfoc( self);
}

Point
Widget_get_virtual_size( Handle self)
{
	return var-> virtualSize;
}

/* set_props() */

Bool
Widget_set_capture( Handle self, Bool capture, Handle confineTo)
{
	if ( opt_InPaint) return false;
	return apc_widget_set_capture( self, capture, confineTo);
}

void
Widget_set_centered( Handle self, Bool x, Bool y)
{
	enter_method;
	Handle parent = my-> get_parent( self);
	Point size    = CWidget( parent)-> get_size( parent);
	Point mysize  = my-> get_size ( self);
	Point mypos   = my-> get_origin( self);
	Point delta   = {0,0};

	if ( !x && !y ) return;

	if ( parent == prima_guts.application ) {
		int i, nrects = 0;
		Point mouse = apc_pointer_get_pos(prima_guts.application);
		Box *best = NULL, *rects = apc_application_get_monitor_rects( prima_guts.application, &nrects);
		for ( i = 0; i < nrects; i++) {
			Box * curr = rects + i;
			if (
				curr->x <= mouse.x &&
				curr->y <= mouse.y &&
				curr->x + curr->width > mouse.x &&
				curr->y + curr->height > mouse.y
			) {
				best = curr;
				break;
			}
		}
		if ( best ) {
			delta.x = best->x;
			delta.y = best->y;
			size.x  = best->width;
			size.y  = best->height;
		}
	}
	if ( x) mypos. x = ( size. x - mysize. x) / 2 + delta.x;
	if ( y) mypos. y = ( size. y - mysize. y) / 2 + delta.y;
	my-> set_origin( self, mypos);
}

void
Widget_set_font( Handle self, Font font)
{
	enter_method;
	if ( var-> stage > csFrozen) return;
	if ( !opt_InPaint) my-> first_that( self, (void*)prima_font_notify, &font);
	if ( var-> handle == NULL_HANDLE) return; /* aware of call from Drawable::init */
	if ( opt_InPaint) {
		inherited->set_font(self, font);
	}
	else {
		apc_font_pick( self, &font, & var-> font);
		opt_clear( optOwnerFont);
		apc_widget_set_font( self, & var-> font);
		my-> repaint( self);
	}
}

void
Widget_set_popup_font( Handle self, Font font)
{
	apc_font_pick( self, &font, &var-> popupFont);
}


/* static iterators */
static Handle
find_tabfoc( Handle self)
{
	int i;
	Handle toRet;
	for ( i = 0; i < var-> widgets. count; i++) {
		PWidget w = ( PWidget)( var-> widgets. items[ i]);
		if (
			w-> self-> get_selectable(( Handle) w) &&
			w-> self-> get_enabled(( Handle) w)
		)
			return ( Handle) w;
	}
	for ( i = 0; i < var-> widgets. count; i++)
		if (( toRet = find_tabfoc( var-> widgets. items[ i])))
			return toRet;
	return NULL_HANDLE;
}


static Bool
get_top_current( Handle self)
{
	PWidget o  = ( PWidget) var-> owner;
	Handle  me = self;
	while ( o) {
		if ( o-> currentWidget != me)
			return false;
		me = ( Handle) o;
		o  = ( PWidget) o-> owner;
	}
	return true;
}

static Bool
sptr( Handle window, Handle self, void * v)
{
	enter_method;
	/* does nothing but refreshes system pointer */
	if ( var-> pointerType == crDefault)
		my-> set_pointerType( self, crDefault);
	return false;
}

/* static iterators for ownership notifications */

Bool
prima_font_notify ( Handle self, Handle child, void * font)
{
	if ( his-> options. optOwnerFont) {
		his-> self-> set_font ( child, *(( PFont) font));
		his-> options. optOwnerFont = 1;
	}
	return false;
}

static Bool
showhint_notify ( Handle self, Handle child, void * data)
{
	if ( his-> options. optOwnerShowHint) {
		his-> self-> set_showHint ( child, *(( Bool *) data));
		his-> options. optOwnerShowHint = 1;
	}
	return false;
}

static Bool
hint_notify ( Handle self, Handle child, SV * hint)
{
	if ( his-> options. optOwnerHint) {
		his-> self-> set_hint( child, hint);
		his-> options. optOwnerHint = 1;
	}
	return false;
}

Bool
prima_single_color_notify ( Handle self, Handle child, void * color)
{
	PSingleColor s = ( PSingleColor) color;
	if ( his-> options. optOwnerColor && ( s-> index == ciFore))
	{
		his-> self-> colorIndex ( child, true, s-> index, s-> color);
		his-> options. optOwnerColor = 1;
	} else if (( his-> options. optOwnerBackColor) && ( s-> index == ciBack))
	{
		his-> self-> colorIndex ( child, true, s-> index, s-> color);
		his-> options. optOwnerBackColor = 1;
	} else if ( s-> index > ciBack)
		his-> self-> colorIndex ( child, true, s-> index, s-> color);
	return false;
}

static Bool
auto_enable_children( Handle self, Handle child, void * enable)
{
	apc_widget_set_enabled( child, PTR2UV( enable) != 0);
	return false;
}
/* properties section */

SV *
Widget_accelItems( Handle self, Bool set, SV * accelItems)
{
	dPROFILE;
	enter_method;
	if ( var-> stage > csFrozen) return NULL_SV;
	if ( !set)
		return var-> accelTable ?
			CAbstractMenu( var-> accelTable)-> get_items( var-> accelTable, "", true) : NULL_SV;
	if ( var-> accelTable == NULL_HANDLE) {
		HV * profile = newHV();
		if ( SvTYPE( accelItems)) pset_sv( items, accelItems);
		pset_H ( owner, self);
		my-> set_accelTable( self, create_instance( "Prima::AccelTable"));
		sv_free(( SV *) profile);
	} else
		CAbstractMenu( var-> accelTable)-> set_items( var-> accelTable, accelItems);
	return NULL_SV;
}

Handle
Widget_accelTable( Handle self, Bool set, Handle accelTable)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( !set)
		return var-> accelTable;
	if ( accelTable && !kind_of( accelTable, CAbstractMenu)) return NULL_HANDLE;
	if ( var->accelTable ) unprotect_object(var-> accelTable);
	var-> accelTable = accelTable;
	if ( var->accelTable ) protect_object(var-> accelTable);
	return NULL_HANDLE;
}

Color
Widget_backColor( Handle self, Bool set, Color color)
{
	enter_method;
	if (!set) return my-> colorIndex( self, false, ciBack, 0);
	my-> colorIndex( self, true, ciBack, color);
	return color;
}

int
Widget_bottom( Handle self, Bool set, int bottom)
{
	enter_method;
	Point p = my-> get_origin( self);
	if ( !set)
		return p. y;
	p. y = bottom;
	my-> set_origin( self, p);
	return 0;
}

Bool
Widget_autoEnableChildren( Handle self, Bool set, Bool autoEnableChildren)
{
	if ( !set)
		return is_opt( optAutoEnableChildren);
	opt_assign( optAutoEnableChildren, autoEnableChildren);
	return false;
}

Bool
Widget_briefKeys( Handle self, Bool set, Bool briefKeys)
{
	if ( !set)
		return is_opt( optBriefKeys);
	opt_assign( optBriefKeys, briefKeys);
	return false;
}

Bool
Widget_buffered( Handle self, Bool set, Bool buffered)
{
	if ( !set)
		return is_opt( optBuffered);
	if ( !opt_InPaint)
		opt_assign( optBuffered, buffered);
	return false;
}

Bool
Widget_clipChildren( Handle self, Bool set, Bool clip_by_children)
{
	if ( set)
		return apc_widget_set_clip_by_children(self, clip_by_children);
	else
		return apc_widget_get_clip_by_children(self);
}

Bool
Widget_clipOwner( Handle self, Bool set, Bool clipOwner)
{
	HV * profile;
	enter_method;
	if ( !set)
		return apc_widget_get_clip_owner( self);
	profile = newHV();
	pset_i( clipOwner, clipOwner);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return false;
}

Color
Widget_color( Handle self, Bool set, Color color)
{
	enter_method;
	if (!set)
		return my-> colorIndex( self, false, ciFore, 0);
	return my-> colorIndex( self, true, ciFore, color);
}

Color
Widget_colorIndex( Handle self, Bool set, int index, Color color)
{
	if ( !set) {
		if ( index < 0 || index > ciMaxId) return clInvalid;
		switch ( index) {
		case ciFore:
			return opt_InPaint ? inherited-> get_color ( self) : apc_widget_get_color( self, ciFore);
		case ciBack:
			return opt_InPaint ? inherited-> get_backColor ( self) : apc_widget_get_color( self, ciBack);
		default:
			return apc_widget_get_color( self, index);
		}
	} else {
		enter_method;
		SingleColor s;
		s. color = color;
		s. index = index;
		if (( index < 0) || ( index > ciMaxId)) return clInvalid;
		if ( !opt_InPaint) my-> first_that( self, (void*)prima_single_color_notify, &s);

		if ( var-> handle == NULL_HANDLE) return clInvalid; /* aware of call from Drawable::init */
		if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0))
			color |= var-> widgetClass;
		if ( opt_InPaint) {
			switch ( index) {
				case ciFore:
					inherited-> set_color ( self, color);
					break;
				case ciBack:
					inherited-> set_backColor ( self, color);
					break;
				default:
					apc_widget_set_color ( self, color, index);
			}
		} else {
			switch ( index) {
				case ciFore:
					opt_clear( optOwnerColor);
					break;
				case ciBack:
					opt_clear( optOwnerBackColor);
					break;
			}
			apc_widget_set_color( self, color, index);
			my-> repaint( self);
		}
	}
	return 0;
}

Bool
Widget_current( Handle self, Bool set, Bool current)
{
	PWidget o;
	if ( var-> stage > csFrozen) return false;
	if ( !set)
		return var-> owner && ( PWidget( var-> owner)-> currentWidget == self);
	o = ( PWidget) var-> owner;
	if ( o == NULL) return false;
	if ( current)
		o-> self-> set_currentWidget( var-> owner, self);
	else
		if ( o-> currentWidget == self)
			o-> self-> set_currentWidget( var-> owner, NULL_HANDLE);
	return current;
}

Handle
Widget_currentWidget( Handle self, Bool set, Handle widget)
{
	enter_method;
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( !set)
		return var-> currentWidget;
	if ( widget) {
		if ( !widget || ( PWidget( widget)-> stage > csFrozen) ||
			( PWidget( widget)-> owner != self)
		) return NULL_HANDLE;
		var-> currentWidget = widget;
	} else
		var-> currentWidget = NULL_HANDLE;

	/* adjust selection if we're in currently selected chain */
	if ( my-> get_selected( self))
		my-> set_selectedWidget( self, widget);
	return NULL_HANDLE;
}

Point
Widget_cursorPos( Handle self, Bool set, Point cursorPos)
{
	if ( !set)
		return apc_cursor_get_pos( self);
	apc_cursor_set_pos( self, cursorPos. x, cursorPos. y);
	return cursorPos;
}

Point
Widget_cursorSize( Handle self, Bool set, Point cursorSize)
{
	if ( !set)
		return apc_cursor_get_size( self);
	apc_cursor_set_size( self, cursorSize. x, cursorSize. y);
	return cursorSize;
}

Bool
Widget_cursorVisible( Handle self, Bool set, Bool cursorVisible)
{
	if ( !set)
		return apc_cursor_get_visible( self);
	return apc_cursor_set_visible( self, cursorVisible);
}

SV *
Widget_dndAware( Handle self, Bool set, SV * dndAware)
{
	if ( !set) {
		if ( var->dndAware == NULL)
			return &PL_sv_undef;
		else if ( strcmp(var->dndAware, "1") == 0)
			return newSViv(1);
		else
			return newSVpv(var->dndAware, 0);
	} else if ( var->dndAware == NULL && SvBOOL(dndAware)) {
		if ( apc_dnd_set_aware( self, true))
			var-> dndAware = duplicate_string( SvPV_nolen( dndAware));
	} else if ( var->dndAware != NULL ) {
		free(var-> dndAware);
		if ( !SvBOOL(dndAware)) {
			var->dndAware = NULL;
			apc_dnd_set_aware( self, false);
		} else
			var-> dndAware = duplicate_string( SvPV_nolen( dndAware));
	}
	return &PL_sv_undef;
}

Bool
Widget_enabled( Handle self, Bool set, Bool enabled)
{
	if ( !set) return apc_widget_is_enabled( self);
	if ( !apc_widget_set_enabled( self, enabled))
		return false;
	if ( is_opt( optAutoEnableChildren))
		CWidget(self)-> first_that( self, (void*)auto_enable_children, INT2PTR(void*,enabled));
	return true;
}

Bool
Widget_firstClick( Handle self, Bool set, Bool firstClick)
{
	return set ?
		apc_widget_set_first_click( self, firstClick) :
		apc_widget_get_first_click( self);
}

Bool
Widget_focused( Handle self, Bool set, Bool focused)
{
	enter_method;
	if ( var-> stage > csNormal) return false;
	if ( !set)
		return apc_widget_is_focused( self);

	if ( focused) {
		PWidget x = ( PWidget)( var-> owner);
		Handle current = self;
		while ( x) {
			x-> currentWidget = current;
			current = ( Handle) x;
			x = ( PWidget) x-> owner;
		}
		var-> currentWidget = NULL_HANDLE;
		if ( var-> stage == csNormal)
			apc_widget_set_focused( self);
	} else
		if ( var-> stage == csNormal && my-> get_selected( self))
			apc_widget_set_focused( NULL_HANDLE);
	return focused;
}

SV *
Widget_helpContext( Handle self, Bool set, SV *helpContext)
{
	if ( set) {
		if ( var-> stage > csFrozen) return NULL_SV;
		free( var-> helpContext);
		var-> helpContext = NULL;
		var-> helpContext = duplicate_string( SvPV_nolen( helpContext));
		opt_assign( optUTF8_helpContext, prima_is_utf8_sv(helpContext));
	} else {
		helpContext = newSVpv( var-> helpContext ? var-> helpContext : "", 0);
		if ( is_opt( optUTF8_helpContext)) SvUTF8_on( helpContext);
		return helpContext;
	}
	return NULL_SV;
}

SV *
Widget_get_hint( Handle self)
{
	return newSVsv(var->hint);
}

void
Widget_set_hint( Handle self, SV *hint)
{
	enter_method;
	if ( var-> stage > csFrozen) return;
	my-> first_that( self, (void*)hint_notify, (void*)hint);
	if ( var-> hint ) sv_free( var-> hint );
	var-> hint = newSVsv( hint);
	if ( prima_guts.application && P_APPLICATION-> hintVisible &&
		P_APPLICATION-> hintUnder == self)
	{
		Handle hintWidget = P_APPLICATION-> hintWidget;
		if ( SvTYPE(var->hint) == SVt_NULL || (SvLEN( var-> hint) == 0))
			my-> set_hintVisible( self, 0);
		if ( hintWidget)
			CWidget(hintWidget)-> set_text( hintWidget, my-> get_hint( self));
	}
	opt_clear( optOwnerHint);
}

Bool
Widget_layered( Handle self, Bool set, Bool layered)
{
	HV * profile;
	enter_method;
	if ( !set)
		return apc_widget_get_layered_request( self);
	profile = newHV();
	pset_i( layered, layered);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return false;
}

int
Widget_left( Handle self, Bool set, int left)
{
	enter_method;
	Point p = my-> get_origin( self);
	if ( !set)
		return p. x;
	p. x = left;
	my-> set_origin( self, p);
	return 0;
}

Point
Widget_origin( Handle self, Bool set, Point origin)
{
	if ( !set)
		return apc_widget_get_pos( self);
	apc_widget_set_pos( self, origin.x, origin.y);
	return origin;
}

Bool
Widget_ownerBackColor( Handle self, Bool set, Bool ownerBackColor)
{
	enter_method;
	if ( !set)
		return is_opt( optOwnerBackColor);
	opt_assign( optOwnerBackColor, ownerBackColor);
	if ( is_opt( optOwnerBackColor) && var-> owner)
	{
		my-> set_backColor( self, ((( PWidget) var-> owner)-> self)-> get_backColor( var-> owner));
		opt_set( optOwnerBackColor);
		my-> repaint ( self);
	}
	return false;
}

Bool
Widget_ownerColor( Handle self, Bool set, Bool ownerColor)
{
	enter_method;
	if ( !set)
		return is_opt( optOwnerColor);
	opt_assign( optOwnerColor, ownerColor);
	if ( is_opt( optOwnerColor) && var-> owner)
	{
		my-> set_color( self, ((( PWidget) var-> owner)-> self)-> get_color( var-> owner));
		opt_set( optOwnerColor);
		my-> repaint( self);
	}
	return false;
}

Bool
Widget_ownerFont( Handle self, Bool set, Bool ownerFont )
{
	enter_method;
	if ( !set)
		return is_opt( optOwnerFont);
	opt_assign( optOwnerFont, ownerFont);
	if ( is_opt( optOwnerFont) && var-> owner)
	{
		my-> set_font ( self, ((( PWidget) var-> owner)-> self)-> get_font ( var-> owner));
		opt_set( optOwnerFont);
		my-> repaint ( self);
	}
	return false;
}

Bool
Widget_ownerHint( Handle self, Bool set, Bool ownerHint )
{
	enter_method;
	if ( !set)
		return is_opt( optOwnerHint);
	opt_assign( optOwnerHint, ownerHint);
	if ( is_opt( optOwnerHint) && var-> owner)
	{
		my-> set_hint( self, ((( PWidget) var-> owner)-> self)-> get_hint ( var-> owner));
		opt_set( optOwnerHint);
	}
	return false;
}

Bool
Widget_ownerPalette( Handle self, Bool set, Bool ownerPalette)
{
	enter_method;
	if ( !set)
		return is_opt( optOwnerPalette);
	if ( ownerPalette) my-> set_palette( self, NULL_SV);
	opt_assign( optOwnerPalette, ownerPalette);
	return false;
}

Bool
Widget_ownerShowHint( Handle self, Bool set, Bool ownerShowHint )
{
	enter_method;
	if ( !set)
		return is_opt( optOwnerShowHint);
	opt_assign( optOwnerShowHint, ownerShowHint);
	if ( is_opt( optOwnerShowHint) && var-> owner)
	{
		my-> set_showHint( self, CWidget( var-> owner)-> get_showHint ( var-> owner));
		opt_set( optOwnerShowHint);
	}
	return false;
}

SV *
Widget_palette( Handle self, Bool set, SV * palette)
{
	int colors;
	if ( !set)
		return inherited-> palette( self, set, palette);

	if ( var-> stage > csFrozen) return NULL_SV;
	if ( var-> handle == NULL_HANDLE) return NULL_SV; /* aware of call from Drawable::init */

	colors = var-> palSize;
	free( var-> palette);
	var-> palette = prima_read_palette( &var-> palSize, palette);
	opt_clear( optOwnerPalette);
	if ( colors == 0 && var-> palSize == 0)
		return NULL_SV; /* do not bother apc */
	if ( opt_InPaint)
		apc_gp_set_palette( self);
	else
		apc_widget_set_palette( self);
	return NULL_SV;
}

Handle
Widget_pointerIcon( Handle self, Bool set, Handle icon)
{
	enter_method;
	Point hotSpot;

	if ( var-> stage > csFrozen) return NULL_HANDLE;

	if ( !set) {
		HV * profile = newHV();
		Handle icon = Object_create( "Prima::Icon", profile);
		sv_free(( SV *) profile);
		apc_pointer_get_bitmap( self, icon);
		--SvREFCNT( SvRV((( PAnyObject) icon)-> mate));
		return icon;
	}

	if ( icon != NULL_HANDLE && !kind_of( icon, CIcon)) {
		warn("Illegal object reference passed to Widget::pointerIcon");
		return NULL_HANDLE;
	}
	hotSpot = my-> get_pointerHotSpot( self);
	apc_pointer_set_user( self, icon, hotSpot);
	if ( var-> pointerType == crUser) my-> first_that( self, (void*)sptr, NULL);
	return NULL_HANDLE;
}

Point
Widget_pointerHotSpot( Handle self, Bool set, Point hotSpot)
{
	enter_method;
	Handle icon;
	if ( !set)
		return apc_pointer_get_hot_spot( self);
	if ( var-> stage > csFrozen) return hotSpot;
	icon = my-> get_pointerIcon( self);
	apc_pointer_set_user( self, icon, hotSpot);
	if ( var-> pointerType == crUser) my-> first_that( self, (void*)sptr, NULL);
	return hotSpot;
}

int
Widget_pointerType( Handle self, Bool set, int type)
{
	enter_method;
	if ( var-> stage > csFrozen) return 0;
	if ( !set)
		return var-> pointerType;
	var-> pointerType = type;
	apc_pointer_set_shape( self, type);
	my-> first_that( self, (void*)sptr, NULL);
	return type;
}

Point
Widget_pointerPos( Handle self, Bool set, Point p)
{
	if ( !set) {
		p = apc_pointer_get_pos( self);
		apc_widget_map_points( self, false, 1, &p);
		return p;
	}
	apc_widget_map_points( self, true, 1, &p);
	apc_pointer_set_pos( self, p. x, p. y);
	return p;
}

Handle
Widget_popup( Handle self, Bool set, Handle popup)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( !set)
		return var-> popupMenu;

	if ( popup && !kind_of( popup, CPopup)) return NULL_HANDLE;
	if ( var->popupMenu ) unprotect_object(var-> popupMenu);
	var-> popupMenu = popup;
	if ( var->popupMenu ) protect_object(var-> popupMenu);
	return NULL_HANDLE;
}

Color
Widget_popupColorIndex( Handle self, Bool set, int index, Color color)
{
	if (( index < 0) || ( index > ciMaxId)) return clInvalid;
	if ( !set)
		return var-> popupColor[ index];
	if ((( color & clSysFlag) != 0) && (( color & wcMask) == 0)) color |= wcPopup;
	var-> popupColor[ index] = color;
	return color;
}

SV *
Widget_popupItems( Handle self, Bool set, SV * popupItems)
{
	dPROFILE;
	enter_method;
	if ( var-> stage > csFrozen) return NULL_SV;
	if ( !set)
		return var-> popupMenu ?
			CAbstractMenu( var-> popupMenu)-> get_items( var-> popupMenu, "", true) : NULL_SV;

	if ( var-> popupMenu == NULL_HANDLE) {
		if ( SvTYPE( popupItems)) {
			HV * profile = newHV();
			pset_sv( items, popupItems);
			pset_H ( owner, self);
			my-> set_popup( self, create_instance( "Prima::Popup"));
			sv_free(( SV *) profile);
		}
	}
	else
		CAbstractMenu(var-> popupMenu)-> set_items(var-> popupMenu, popupItems);
	return popupItems;
}


Rect
Widget_rect( Handle self, Bool set, Rect r)
{
	enter_method;
	if ( !set) {
		Point p   = my-> get_origin( self);
		Point s   = my-> get_size( self);
		r. left   = p. x;
		r. bottom = p. y;
		r. right  = p. x + s. x;
		r. top    = p. y + s. y;
	} else
		apc_widget_set_rect( self, r. left, r. bottom, r. right - r. left, r. top - r. bottom);
	return r;
}

int
Widget_right( Handle self, Bool set, int right)
{
	enter_method;
	Point p;
	Rect r = my-> get_rect( self);
	if ( !set)
		return r. right;
	p. x = r. left - r. right + right;
	p. y = r. bottom;
	my-> set_origin( self, p);
	return 0;
}

Bool
Widget_scaleChildren( Handle self, Bool set, Bool scaleChildren)
{
	if ( !set)
		return is_opt( optScaleChildren);
	opt_assign( optScaleChildren, scaleChildren);
	return false;
}

Bool
Widget_selectable( Handle self, Bool set, Bool selectable)
{
	if ( !set)
		return is_opt( optSelectable);
	opt_assign( optSelectable, selectable);
	return false;
}

Bool
Widget_selected( Handle self, Bool set, Bool selected)
{
	enter_method;
	if ( !set)
		return my-> get_selectedWidget( self) != NULL_HANDLE;

	if ( var-> stage > csFrozen) return selected;
	if ( selected) {
		if ( is_opt( optSelectable) && !is_opt( optSystemSelectable)) {
			my-> set_focused( self, true);
		} else
		if ( var-> currentWidget) {
			PWidget w = ( PWidget) var-> currentWidget;
			if ( w-> options. optSystemSelectable && !w-> self-> get_clipOwner(( Handle) w))
				w-> self-> bring_to_front(( Handle) w); /* <- very uncertain !!!! */
			else
				w-> self-> set_selected(( Handle) w, true);
		} else
		if ( is_opt( optSystemSelectable)) {
			/* nothing to do with Widget, reserved for Window */
		}
		else {
			PWidget toFocus = ( PWidget) find_tabfoc( self);
			if ( toFocus)
				toFocus-> self-> set_selected(( Handle) toFocus, 1);
			else {
			/* if group has no selectable widgets and cannot be selected by itself, */
			/* process chain of bring_to_front(), followed by set_focused(1) call, if available */
				PWidget x = ( PWidget) var-> owner;
				List  lst;
				int i;

				list_create( &lst, 8, 8);
				while ( x) {
					if ( !toFocus && x-> options. optSelectable) {
						toFocus = x;  /* choose closest owner to focus */
						break;
					}
					if (( Handle) x != prima_guts.application && !kind_of(( Handle) x, CWindow))
						list_insert_at( &lst, ( Handle) x, 0);
					x = ( PWidget) x-> owner;
				}

				if ( toFocus)
					toFocus-> self-> set_focused(( Handle) toFocus, 1);

				for ( i = 0; i < lst. count; i++) {
					PWidget v = ( PWidget) list_at( &lst, i);
					v-> self-> bring_to_front(( Handle) v);
				}
				list_destroy( &lst);
			}
		} /* end set_selected( true); */
	} else
		my-> set_focused( self, false);
	return selected;
}

Handle
Widget_selectedWidget( Handle self, Bool set, Handle widget)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;

	if ( !set) {
		if ( var-> stage <= csNormal) {
			Handle foc = apc_widget_get_focused();
			PWidget  f = ( PWidget) foc;
			while( f) {
				if (( Handle) f == self) return foc;
				f = ( PWidget) f-> owner;
			}
		}
		return NULL_HANDLE;

		/* classic solution should be recursive and inheritant call */
		/* of get_selected() here, when Widget would return state of */
		/* child-group selected state until Widget::selected() called; */
		/* thus, each of them would call apc_widget_get_focused - that's expensive, */
		/* so that's the reason not to use classic object model here. */
	}

	if ( widget) {
		if ( PWidget( widget)-> owner == self)
			CWidget( widget)-> set_selected( widget, true);
	} else {
		/* give selection up to hierarchy chain */
		Handle s = self;
		while ( s) {
			if ( CWidget( s)-> get_selectable( s)) {
				CWidget( s)-> set_selected( s, true);
				break;
			}
			s = PWidget( s)-> owner;
		}
	}
	return NULL_HANDLE;
}

int
Widget_selectingButtons( Handle self, Bool set, int sb)
{
	if ( !set)
		return var-> selectingButtons;
	return var-> selectingButtons = sb;
}

Handle
Widget_shape( Handle self, Bool set, Handle mask)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;

	if ( !set) {
		if ( apc_widget_get_shape( self, NULL_HANDLE)) {
			HV * profile = newHV();
			Handle i = Object_create( "Prima::Region", profile);
			sv_free(( SV *) profile);
			apc_widget_get_shape( self, i);
			--SvREFCNT( SvRV((( PAnyObject) i)-> mate));
			return i;
		} else
			return NULL_HANDLE;
	}

	if ( mask && kind_of( mask, CRegion)) {
		apc_widget_set_shape( self, mask);
		return NULL_HANDLE;
	}

	if ( mask && !kind_of( mask, CImage)) {
		warn("Illegal object reference passed to Drawable::region");
		return NULL_HANDLE;
	}

	if ( mask ) {
		Handle region;
		HV * profile = newHV();

		pset_H( image, mask );
		region = Object_create("Prima::Region", profile);
		sv_free(( SV *) profile);

		apc_widget_set_shape( self, region);
		Object_destroy(region);
	} else
		apc_widget_set_shape( self, NULL_HANDLE);

	return NULL_HANDLE;
}

Bool
Widget_showHint( Handle self, Bool set, Bool showHint )
{
	enter_method;
	Bool oldShowHint = is_opt( optShowHint);
	if ( !set)
		return oldShowHint;
	my-> first_that( self, (void*)showhint_notify, &showHint);
	opt_clear( optOwnerShowHint);
	opt_assign( optShowHint, showHint);
	if ( prima_guts.application && !is_opt( optShowHint) && oldShowHint) my-> set_hintVisible( self, 0);
	return false;
}

Point
Widget_size( Handle self, Bool set, Point size)
{
	if ( !set)
		return apc_widget_get_size( self);
	apc_widget_set_size( self, size.x, size.y);
	return size;
}

Bool
Widget_syncPaint( Handle self, Bool set, Bool syncPaint)
{
	HV * profile;
	enter_method;
	if ( !set)
		return apc_widget_get_sync_paint( self);
	profile = newHV();
	pset_i( syncPaint, syncPaint);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return false;
}


Bool
Widget_transparent( Handle self, Bool set, Bool transparent)
{
	HV * profile;
	enter_method;
	if ( !set)
		return apc_widget_get_transparent( self);
	profile = newHV();
	pset_i( transparent, transparent);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return false;
}

SV *
Widget_get_text( Handle self)
{
	return newSVsv(var->text);
}

void
Widget_set_text( Handle self, SV *text)
{
	if ( var-> stage > csFrozen) return;
	if ( var-> text ) sv_free( var-> text );
	var-> text = newSVsv(text);
}

int
Widget_top( Handle self, Bool set, int top)
{
	enter_method;
	Point p;
	Rect  r   = my-> get_rect( self);
	if ( !set)
		return r. top;
	p. x = r. left;
	p. y = r. bottom - r. top + top;
	my-> set_origin( self, p);
	return 0;
}

Bool
Widget_visible( Handle self, Bool set, Bool visible)
{
	return set ?
		apc_widget_set_visible( self, visible) :
		apc_widget_is_visible( self);
}

int
Widget_widgetClass( Handle self, Bool set, int widgetClass)
{
	enter_method;
	if ( !set)
		return var-> widgetClass;
	var-> widgetClass = widgetClass;
	my-> repaint( self);
	return 0;
}

/* XS section */
XS( Widget_client_to_screen_FROMPERL)
{
	dXSARGS;
	Handle self;
	int i, count;
	Point * points;

	if (( items % 2) != 1)
		croak ("Invalid usage of Widget::client_to_screen");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Widget::client_to_screen");
	count  = ( items - 1) / 2;
	if ( !( points = allocn( Point, count))) {
		PUTBACK;
		return;
	}
	for ( i = 0; i < count; i++) {
		points[i]. x = SvIV( ST( i * 2 + 1));
		points[i]. y = SvIV( ST( i * 2 + 2));
	}
	apc_widget_map_points( self, true, count, points);
	EXTEND( sp, count * 2);
	for ( i = 0; i < count; i++) {
		PUSHs( sv_2mortal( newSViv( points[i].x)));
		PUSHs( sv_2mortal( newSViv( points[i].y)));
	}
	PUTBACK;
	free( points);
	return;
}

XS( Widget_screen_to_client_FROMPERL)
{
	dXSARGS;
	Handle self;
	int i, count;
	Point * points;

	if (( items % 2) != 1)
		croak ("Invalid usage of Widget::screen_to_client");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Widget::screen_to_client");
	count  = ( items - 1) / 2;
	if ( !( points = allocn( Point, count))) {
		PUTBACK;
		return;
	}
	for ( i = 0; i < count; i++) {
		points[i]. x = SvIV( ST( i * 2 + 1));
		points[i]. y = SvIV( ST( i * 2 + 2));
	}
	apc_widget_map_points( self, false, count, points);
	EXTEND( sp, count * 2);
	for ( i = 0; i < count; i++) {
		PUSHs( sv_2mortal( newSViv( points[i].x)));
		PUSHs( sv_2mortal( newSViv( points[i].y)));
	}
	PUTBACK;
	free( points);
	return;
}


XS( Widget_get_widgets_FROMPERL)
{
	dXSARGS;
	Handle self;
	Handle * list;
	int i, count;

	if ( items != 1)
		croak ("Invalid usage of Widget.get_widgets");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Widget.get_widgets");
	count = var-> widgets. count;
	list  = var-> widgets. items;
	EXTEND( sp, count);
	for ( i = 0; i < count; i++)
		PUSHs( sv_2mortal( newSVsv((( PAnyObject) list[ i])-> mate)));
	PUTBACK;
	return;
}

void Widget_get_widgets          ( Handle self) { warn("Invalid call of Widget::get_widgets"); }
void Widget_get_widgets_REDEFINED( Handle self) { warn("Invalid call of Widget::get_widgets"); }
void Widget_screen_to_client ( Handle self) { warn("Invalid call of Widget::screen_to_client"); }
void Widget_screen_to_client_REDEFINED ( Handle self) { warn("Invalid call of Widget::screen_to_client"); }
void Widget_client_to_screen ( Handle self) { warn("Invalid call of Widget::screen_to_client"); }
void Widget_client_to_screen_REDEFINED ( Handle self) { warn("Invalid call of Widget::screen_to_client"); }

#ifdef __cplusplus
}
#endif
