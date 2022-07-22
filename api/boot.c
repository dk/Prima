#define GENERATE_TABLE_GENERATOR yes
#include "apricot.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include <dirent.h>
#include "guts.h"
#include "Object.h"
#include "Component.h"
#include "File.h"
#include "Clipboard.h"
#include "DeviceBitmap.h"
#include "Drawable.h"
#include "Widget.h"
#include "Window.h"
#include "Image.h"
#include "Icon.h"
#include "AbstractMenu.h"
#include "AccelTable.h"
#include "Menu.h"
#include "Popup.h"
#include "Application.h"
#include "Timer.h"
#include "Utils.h"
#include "Printer.h"
#include "Region.h"
#include "img_conv.h"

#include <Types.inc>

#ifdef __cplusplus
extern "C" {
#endif

#include "thunks.tinc"

static void
cv_set_prototype(char * package, char * method, char * prototype)
{
	HV * stash;
	GV * gv;
	CV * cv;
	if (!(stash = gv_stashpvn(package, strlen(package), 0)))
		return;
	if ( !( gv = gv_fetchmeth( stash, method, strlen( method), 0)))
		return;
	if (!( cv = GvCV(gv)))
		return;
	sv_setpv((SV *)cv, prototype);
}

void
prima_register_notifications( PVMT vmt)
{
	SV *package;
	SV *nt_sub;
	SV *nt_ref;
	HV *nt;
	PVMT v = vmt;
	HE *he;
	char buf[ 1024];

	while (( v != NULL) && ( v != (PVMT) CComponent)) v = v-> base;
	if (!v) return;
	package = newSVpv( vmt-> className, 0);
	if ( !package) croak( "GUTS016: Not enough memory");
	nt_sub = ( SV*) sv_query_method( package, "notification_types", 0);
	if ( !nt_sub) croak( "GUTS016: Invalid package %s", vmt-> className);
	nt_ref = cv_call_perl( package, nt_sub, "<");
	if ( !nt_ref || !SvROK(nt_ref) || SvTYPE(SvRV(nt_ref)) != SVt_PVHV)
		croak( "GUTS016: %s: Bad notification_types() return value", vmt-> className);
	nt = (HV*)SvRV(nt_ref);

	hv_iterinit( nt);
	while (( he = hv_iternext( nt)) != NULL) {
		snprintf( buf, 1024, "on%s", HeKEY( he));
		if (sv_query_method( package, buf, 0)) continue;
		snprintf( buf, 1024, "%s::on%s", vmt-> className, HeKEY( he));
		newXS( buf, Component_set_notification_FROMPERL, vmt-> className);
	}
	sv_free( package);
}

XS(Prima_init)
{
	dXSARGS;
	char error_buf[256] = "Error initializing Prima";
	(void)items;

	if ( items < 1) croak("Invalid call to Prima::init");

	{
		SV * ref;
		SV * package = newSVpv( "Prima::Object", 0);
		if ( !package) croak( "GUTS016: Not enough memory");
		ref = ( SV *) sv_query_method( package, "profile_default", 0);
		sv_free( package);
		if ( !ref) croak("'use Prima;' call required in main script");
	}

	if ( prima_guts.init_ok == 0) {
		prima_register_notifications((PVMT)CComponent);
		prima_register_notifications((PVMT)CFile);
		prima_register_notifications((PVMT)CAbstractMenu);
		prima_register_notifications((PVMT)CAccelTable);
		prima_register_notifications((PVMT)CMenu);
		prima_register_notifications((PVMT)CPopup);
		prima_register_notifications((PVMT)CClipboard);
		prima_register_notifications((PVMT)CTimer);
		prima_register_notifications((PVMT)CDrawable);
		prima_register_notifications((PVMT)CImage);
		prima_register_notifications((PVMT)CIcon);
		prima_register_notifications((PVMT)CDeviceBitmap);
		prima_register_notifications((PVMT)CWidget);
		prima_register_notifications((PVMT)CWindow);
		prima_register_notifications((PVMT)CApplication);
		prima_register_notifications((PVMT)CPrinter);
		prima_register_notifications((PVMT)CRegion);
		prima_guts.init_ok++;
	}

	if ( prima_guts.init_ok == 1) {
		prima_init_image_subsystem();
		prima_guts.init_ok++;
	}

	if ( prima_guts.init_ok == 2) {
		prima_init_font_mapper();
		if ( !window_subsystem_init( error_buf))
			croak( "%s", error_buf);
		prima_guts.init_ok++;
	}
	SPAGAIN;
	XSRETURN_EMPTY;
}

static Bool
kill_objects( void * item, int keyLen, Handle * self, void * dummy)
{
	Object_destroy( *self);
	return false;
}

static Bool
kill_hashes( PHash hash, void * dummy)
{
	hash_destroy( hash, false);
	return false;
}

XS( prima_cleanup)
{
	dXSARGS;
	(void)items;

	if ( prima_guts.application) Object_destroy( prima_guts.application);
	prima_guts.app_is_dead = true;
	hash_first_that( prima_guts.objects, (void*)kill_objects, NULL, NULL, NULL);
	hash_destroy( prima_guts.objects, false);
	prima_guts.objects = NULL;
	if ( prima_guts.init_ok > 1) prima_cleanup_image_subsystem();
	if ( prima_guts.init_ok > 2) {
		window_subsystem_cleanup();
		prima_cleanup_font_mapper();
	}
	hash_destroy( prima_guts.vmt_hash, false);
	prima_guts.vmt_hash = NULL;
	list_delete_all( &prima_guts.static_objects, true);
	list_destroy( &prima_guts.static_objects);
	list_destroy( &prima_guts.post_destroys);
	prima_kill_zombies();
	if ( prima_guts.init_ok > 2) window_subsystem_done();
	list_first_that( &prima_guts.static_hashes, (void*)kill_hashes, NULL);
	list_destroy( &prima_guts.static_hashes);
	prima_guts.init_ok = 0;

	ST(0) = &PL_sv_yes;
	XSRETURN(1);
}

XS( destroy_mate)
{
	dXSARGS;
	Handle self;

	if ( items != 1)
		croak ("Invalid usage of ::destroy_mate");
	self = gimme_the_real_mate( ST( 0));

	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to ::destroy_mate");
	{
		Object_destroy( self);
		if (((PObject)self)-> protectCount > 0) {
			(( PObject) self)-> killPtr = (PAnyObject)prima_guts.ghost_chain;
			prima_guts.ghost_chain = ( PAnyObject) self;
		} else {
			free(( void*) self);
		}
	}
	XSRETURN_EMPTY;
}

static void
register_constants( void)
{
	register_am_constants();
	register_apc_constants();
	register_bi_constants();
	register_bs_constants();
	register_ci_constants();
	register_cl_constants();
	register_cm_constants();
	register_cr_constants();
	register_dbt_constants();
	register_dnd_constants();
	register_dt_constants();
	register_fdo_constants();
	register_fds_constants();
	register_fe_constants();
	register_fm_constants();
	register_fp_constants();
	register_fr_constants();
	register_fs_constants();
	register_fv_constants();
	register_fw_constants();
	register_ggo_constants();
	register_gm_constants();
	register_gt_constants();
	register_gui_constants();
	register_ict_constants();
	register_ictd_constants();
	register_ictp_constants();
	register_im_constants();
	register_is_constants();
	register_ist_constants();
	register_kb_constants();
	register_km_constants();
	register_le_constants();
	register_lj_constants();
	register_lp_constants();
	register_mb_constants();
	register_mt_constants();
	register_nt_constants();
	register_ps_constants();
	register_rgn_constants();
	register_rgnop_constants();
	register_rop_constants();
	register_sbmp_constants();
	register_scr_constants();
	register_sv_constants();
	register_ta_constants();
	register_to_constants();
	register_ts_constants();
	register_tw_constants();
	register_wc_constants();
	register_ws_constants();
}

/* register builtin classes */
static void
register_classes( void)
{
	register_AbstractMenu_Class();
	register_AccelTable_Class();
	register_Application_Class();
	register_Clipboard_Class();
	register_Component_Class();
	register_DeviceBitmap_Class();
	register_Drawable_Class();
	register_File_Class();
	register_Icon_Class();
	register_Image_Class();
	register_Menu_Class();
	register_Object_Class();
	register_Popup_Class();
	register_Printer_Class();
	register_Region_Class();
	register_Timer_Class();
	register_Utils_Package();
	register_Widget_Class();
	register_Window_Class();
}

/* register hard coded XSUBs */
static void
register_xsubs( void)
{
	newXS( "::destroy_mate", destroy_mate, "Prima Guts");
	newXS( "Prima::cleanup", prima_cleanup, "Prima");
	newXS( "Prima::init", Prima_init, "Prima");
	newXS( "Prima::options", Prima_options, "Prima");
	newXS( "Prima::Utils::getdir", Utils_getdir_FROMPERL, "Prima::Utils");
	newXS( "Prima::Utils::stat", Utils_stat_FROMPERL, "Prima::Utils");
	newXS( "Prima::Utils::DIRHANDLE::DESTROY", Utils_closedir_FROMPERL, "Prima::Utils");
	newXS( "Prima::Object::create",  create_from_Perl, "Prima::Object");
	newXS( "Prima::Object::destroy", destroy_from_Perl, "Prima::Object");
	newXS( "Prima::Object::alive", Object_alive_FROMPERL, "Prima::Object");
	newXS( "Prima::array::deduplicate", Prima_array_deduplicate_FROMPERL, "Prima::array");
	newXS( "Prima::Component::event_hook", Component_event_hook_FROMPERL, "Prima::Component");
	newXS( "Prima::message", Prima_message_FROMPERL, "Prima");
	newXS( "Prima::dl_export", Prima_dl_export, "Prima");
	cv_set_prototype("Prima::Utils", "closedir", "$");
	cv_set_prototype("Prima::Utils", "rewinddir", "$");
	cv_set_prototype("Prima::Utils", "seekdir", "$$");
	cv_set_prototype("Prima::Utils", "telldir", "$");
}

/* This stuff is not part of the API! Yes, I have been warned. */
#ifndef PERL_VERSION_DECIMAL
#  define PERL_VERSION_DECIMAL(r,v,s) (r*1000000 + v*1000 + s)
#endif
#ifndef PERL_DECIMAL_VERSION
#  define PERL_DECIMAL_VERSION \
	PERL_VERSION_DECIMAL(PERL_REVISION,PERL_VERSION,PERL_SUBVERSION)
#endif
#ifndef PERL_VERSION_GE
#  define PERL_VERSION_GE(r,v,s) \
	(PERL_DECIMAL_VERSION >= PERL_VERSION_DECIMAL(r,v,s))
#endif
#ifndef PERL_VERSION_LE
#  define PERL_VERSION_LE(r,v,s) \
	(PERL_DECIMAL_VERSION <= PERL_VERSION_DECIMAL(r,v,s))
#endif

XS( boot_Prima)
{
	dXSARGS;
	(void)items;

#if PERL_VERSION_LE(5, 21, 5)
	XS_VERSION_BOOTCHECK;
#  ifdef XS_APIVERSION_BOOTCHECK
	XS_APIVERSION_BOOTCHECK;
#  endif
#endif

#define TYPECHECK(s1,s2) \
if (sizeof(s1) != (s2)) { \
	printf("Error: type %s is %d bytes long (expected to be %d)", #s1, (int)sizeof(s1), s2); \
	ST(0) = &PL_sv_no; \
	XSRETURN(1); \
}
	TYPECHECK( uint8_t,  1);
	TYPECHECK( int8_t,   1);
	TYPECHECK( uint16_t, 2);
	TYPECHECK( int16_t,  2);
	TYPECHECK( uint32_t, 4);
	TYPECHECK( int32_t,  4);
	TYPECHECK( void*, (int)sizeof(Handle));
	TYPECHECK( Point, 2*(int)sizeof(int));
	TYPECHECK( NPoint, 2*(int)sizeof(double));
	TYPECHECK( Rect, 4*(int)sizeof(int));
#undef TYPECHECK

	bzero(&prima_guts, sizeof(prima_guts));
	list_create( &prima_guts.static_objects, 16, 16);
	list_create( &prima_guts.static_hashes,  16, 16);
	list_create( &prima_guts.post_destroys,  16, 16);
	prima_guts.objects   = hash_create();
	prima_guts.vmt_hash  = hash_create();

	register_constants();
	register_classes();
	register_xsubs();
	init_image_support();

#if PERL_VERSION_LE(5, 21, 5)
#  if PERL_VERSION_GE(5, 9, 0)
	if (PL_unitcheckav)
		call_list(PL_scopestack_ix, PL_unitcheckav);
#  endif
	XSRETURN_YES;
#else
	Perl_xs_boot_epilog(aTHX_ ax);
#endif
}

#ifdef __cplusplus
}
#endif
