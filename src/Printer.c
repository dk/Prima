#include "apricot.h"
#include "Printer.h"
#include <Printer.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CDrawable->
#define my  ((( PPrinter) self)-> self)
#define var (( PPrinter) self)

void
Printer_init( Handle self, HV * profile)
{
	dPROFILE;
	char * prn;
	inherited init( self, profile);
	if ( !apc_prn_create( self))
		croak("Cannot create printer");
	opt_set(optSystemDrawable);
	prn = pget_c( printer);
	if ( strlen( prn) == 0) prn = my-> get_default_printer( self);
	my-> set_printer( self, prn);
	CORE_INIT_TRANSIENT(Printer);
}

void
Printer_done( Handle self)
{
	apc_prn_destroy( self);
	inherited done( self);
}

Bool
Printer_validate_owner( Handle self, Handle * owner, HV * profile)
{
	dPROFILE;
	if ( pget_H( owner) != application || application == nilHandle) return false;
	*owner = application;
	return true;
}

Bool
Printer_begin_doc( Handle self, char * docName)
{
	Bool ok;
	char buf[ 256];
	if ( is_opt( optInDraw)) return false;
	if ( !docName || *docName == '\0') {
		snprintf( buf, 256, "APC: %s", (( PComponent) application)-> name);
		docName = buf;
	}
	if ( is_opt( optInDrawInfo))
		my-> end_paint_info( self);
	if ( !inherited begin_paint( self))
		return false;
	if ( !( ok = apc_prn_begin_doc( self, docName))) {
		inherited end_paint( self);
		perl_error();
	}
	return ok;
}

Bool
Printer_new_page( Handle self)
{
	Bool ok;
	if ( !is_opt( optInDraw)) return false;
	ok = apc_prn_new_page( self);
	if ( !ok) perl_error();
	return ok;
}

Bool
Printer_end_doc( Handle self)
{
	Bool ret;
	if ( !is_opt( optInDraw)) return false;
	ret = apc_prn_end_doc( self);
	inherited end_paint( self);
	if ( !ret) perl_error();
	return ret;
}

void
Printer_abort_doc( Handle self)
{
	if ( !is_opt( optInDraw)) return;
	inherited end_paint( self);
	apc_prn_abort_doc( self);
}

char *
Printer_printer( Handle self, Bool set, char * printerName)
{
	if ( !set)
		return apc_prn_get_selected( self);
	if ( is_opt( optInDraw))     my-> end_paint( self);
	if ( is_opt( optInDrawInfo)) my-> end_paint_info( self);
	return apc_prn_select( self, printerName) ? "1" : "";
}

Bool Printer_begin_paint( Handle self) { return my-> begin_doc( self, ""); }
void Printer_end_paint( Handle self) { my-> abort_doc( self); }

Bool
Printer_begin_paint_info( Handle self)
{
	Bool ok;
	if ( is_opt( optInDraw))     return true;
	if ( !inherited begin_paint_info( self))
		return false;
	if ( !( ok = apc_prn_begin_paint_info( self))) {
		inherited end_paint_info( self);
		if ( !ok) perl_error();
	}
	return ok;
}

void
Printer_end_paint_info( Handle self)
{
	if ( !is_opt( optInDrawInfo)) return;
	apc_prn_end_paint_info( self);
	inherited end_paint_info( self);
}

extern SV *
Application_fonts( Handle self, char * name, char * encoding);

SV *
Printer_fonts( Handle self, char * name, char * encoding)
{
	return Application_fonts( self, name, encoding);
}

extern SV*
Application_font_encodings( Handle self, char * encoding);

SV*
Printer_font_encodings( Handle self, char * encoding)
{
	return Application_font_encodings( self, encoding);
}

SV *
Printer_printers( Handle self)
{
	int count, i;
	AV * glo = newAV();
	PPrinterInfo info = apc_prn_enumerate( self, &count);
	for ( i = 0; i < count; i++) av_push( glo, sv_PrinterInfo2HV( &info[ i]));
	free( info);
	return newRV_noinc(( SV *) glo);
}

Point
Printer_size( Handle self, Bool set, Point size)
{
	if ( !set)
		return apc_prn_get_size( self);
	return inherited size( self, set, size);
}

SV *
Printer_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_prn_get_handle( self));
	return newSVpv( buf, 0);
}

Point
Printer_resolution( Handle self, Bool set, Point resolution)
{
	if ( set)
		croak("Attempt to write read-only property %s", "Printer::resolution");
	return apc_prn_get_resolution( self);
}

XS( Printer_options_FROMPERL)
{
	dXSARGS;
	Handle self;

	if ( items == 0)
		croak ("Invalid usage of Printer.options");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == nilHandle)
		croak( "Illegal object reference passed to Printer.options");

	switch ( items) {
	case 1: {
		int i, argc = 0;
		char ** argv;
		if ( apc_prn_enum_options( self, &argc, &argv)) {
			EXTEND( sp, argc);
			for ( i = 0; i < argc; i++)
				PUSHs( sv_2mortal( newSVpv( argv[i], 0)));
			free( argv);
		}
		PUTBACK;
		return;
	}
	case 2: {
		char *option, *value;
		option = ( char*) SvPV_nolen( ST(1));
		if ( apc_prn_get_option( self, option, &value)) {
			SPAGAIN;
			XPUSHs( sv_2mortal( newSVpv( value, 0)));
			free( value);
		} else {
			SPAGAIN;
			XPUSHs( nilSV);
		}
		PUTBACK;
		return;
	}
	default: {
		int i, success = 0;
		char *option, *value;

		for ( i = 1; i < items; i+=2) {
			option = ( char*) SvPV_nolen( ST(i));
			value  = (SvOK( ST(i+1)) ? ( char*) SvPV_nolen( ST(i+1)) : nil);
			if ( !value) continue;
			if ( !apc_prn_set_option( self, option, value)) continue;
			success++;
		}

		SPAGAIN;
		XPUSHs( sv_2mortal( newSViv( success)));
		PUTBACK;
		return;
	}}
	return;
}

void Printer_options          ( Handle self) { warn("Invalid call of Printer::options"); }
void Printer_options_REDEFINED( Handle self) { warn("Invalid call of Printer::options"); }

#ifdef __cplusplus
}
#endif
