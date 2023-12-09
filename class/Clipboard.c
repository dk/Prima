#include "apricot.h"
#include "Application.h"
#include "Image.h"
#include "Clipboard.h"
#include <Clipboard.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CComponent->
#define my  ((( PClipboard) self)-> self)
#define var (( PClipboard) self)

#define cefInit     0
#define cefDone     1
#define cefStore    2
#define cefFetch    3

struct _ClipboardFormatReg;
typedef SV* ClipboardExchangeFunc( Handle self, struct _ClipboardFormatReg * instance, int function, SV * data);
typedef ClipboardExchangeFunc *PClipboardExchangeFunc;

typedef struct _ClipboardFormatReg
{
	char                          *id;
	Handle                         sysId;
	ClipboardExchangeFunc         *server;
	void                          *data;
	Bool                           written;
	Bool                           success;
} ClipboardFormatReg, *PClipboardFormatReg;

static SV * text_server  ( Handle self, PClipboardFormatReg, int, SV *);
static SV * utf8_server  ( Handle self, PClipboardFormatReg, int, SV *);
static SV * image_server ( Handle self, PClipboardFormatReg, int, SV *);
static SV * binary_server( Handle self, PClipboardFormatReg, int, SV *);

static int clipboards = 0;
static int formatCount = 0;
static Bool protect_formats = false;
static PClipboardFormatReg formats = NULL;

void *
Clipboard_register_format_proc( Handle self, char * format, void * serverProc);

void
Clipboard_init( Handle self, HV * profile)
{
	inherited init( self, profile);
	if ( !apc_clipboard_create(self))
		croak( "Cannot create clipboard");
	if (clipboards == 0) {
		Clipboard_register_format_proc( self, "Text",  (void*)text_server);
		Clipboard_register_format_proc( self, "Image", (void*)image_server);
		Clipboard_register_format_proc( self, "UTF8",  (void*)utf8_server);
		protect_formats = 1;
	}
	clipboards++;
	CORE_INIT_TRANSIENT(Clipboard);
}

void
Clipboard_done( Handle self)
{
	clipboards--;
	if ( clipboards == 0) {
		protect_formats = 0;
		while( formatCount)
			my-> deregister_format( self, formats-> id);
	}
	apc_clipboard_destroy(self);
	inherited done( self);
}

void Clipboard_handle_event( Handle self, PEvent event)
{
	switch ( event-> cmd)
	{
		case cmClipboard: {
			var-> openCount++;
			C_APPLICATION-> push_event( prima_guts.application);
			C_APPLICATION-> notify( prima_guts.application, "<sHss", "Clipboard", self, "copy", (char*)event->gen.p);
			C_APPLICATION-> pop_event( prima_guts.application);
			var-> openCount--;
			return;
		}

	}
	inherited handle_event ( self, event);
}

Bool
Clipboard_validate_owner( Handle self, Handle * owner, HV * profile)
{
	dPROFILE;
	if ( pget_H( owner) != prima_guts.application || prima_guts.application == NULL_HANDLE) return false;
	*owner = prima_guts.application;
	return true;
}

typedef Bool ActionProc ( Handle self, PClipboardFormatReg item, void * params);
typedef ActionProc *PActionProc;

static PClipboardFormatReg
first_that( Handle self, void * actionProc, void * params)
{
	int i;
	PClipboardFormatReg list = formats;
	if ( actionProc == NULL) return NULL;
	for ( i = 0; i < formatCount; i++) {
		if ((( PActionProc) actionProc)( self, list+i, params))
			return list+i;
	}
	return NULL;
}

static Bool
find_format( Handle self, PClipboardFormatReg item, char *format)
{
	return strcmp( item-> id, format) == 0;
}

static Bool
reset_written( Handle self, PClipboardFormatReg item, char *format)
{
	item-> written = false;
	return false;
}

void *
Clipboard_register_format_proc( Handle self, char * format, void * serverProc)
{
	PClipboardFormatReg list = first_that( self, (void*)find_format, format);
	if ( list) {
		my-> deregister_format( self, format);
	}
	if (!( list = allocn( ClipboardFormatReg, formatCount + 1)))
		return NULL;
	if ( formats != NULL) {
		memcpy( list, formats, sizeof( ClipboardFormatReg) * formatCount);
		free( formats);
	}
	formats = list;
	list += formatCount++;
	list-> id     = duplicate_string( format);
	list-> server = ( ClipboardExchangeFunc *) serverProc;
	list-> sysId  = ( Handle) list-> server( self, list, cefInit, NULL_SV);
	return list;
}

void
Clipboard_deregister_format( Handle self, char * format)
{
	PClipboardFormatReg fr, list;

	if ( protect_formats && (
		( strlen( format) == 0)          ||
		( strcmp( format, "Text") == 0)  ||
		( strcmp( format, "UTF8") == 0)  ||
		( strcmp( format, "Image") == 0)))
		return;

	fr = first_that( self, (void*)find_format, format);
	if ( fr == NULL) return;
	list = formats;
	fr-> server( self, fr, cefDone, NULL_SV);
	free( fr-> id);
	formatCount--;
	memmove( fr, fr + 1, sizeof( ClipboardFormatReg) * ( formatCount - ( fr - list)));
	if ( formatCount > 0) {
		if (( fr = allocn( ClipboardFormatReg, formatCount)))
			memcpy( fr, list, sizeof( ClipboardFormatReg) * formatCount);
	} else
		fr = NULL;
	free( formats);
	formats = fr;
}

Bool
Clipboard_open( Handle self)
{
	var-> openCount++;
	if ( var-> openCount > 1) return true;

	first_that( self, (void*) reset_written, NULL);
	return apc_clipboard_open( self);
}

void
Clipboard_close( Handle self)
{
	PClipboardFormatReg text, utf8;
	if ( var-> openCount <= 0) {
		var-> openCount = 0;
		return;
	}

	var-> openCount--;
	if ( var->  openCount > 0) return;
	text = formats + cfText;
	utf8 = formats + cfUTF8;
	/* automatically downgrade UTF8 to TEXT */
	if ( utf8-> written && !text-> written) {
		SV *utf8_sv, *text_sv;
		if (( utf8_sv = utf8-> server( self, utf8, cefFetch, NULL_SV))) {
			STRLEN bytelen;
			unsigned int charlen;
			const char * src;
			src = SvPV( utf8_sv, bytelen);
			text_sv = newSVpvn("", 0);
			while ( bytelen > 0) {
				register UV u = prima_utf8_uvchr(src, bytelen, &charlen);
				char c = ( u < 0x7f) ? u : '?';
				src += charlen;
				bytelen -= charlen;
				sv_catpvn( text_sv, &c, 1);
				if ( charlen == 0 ) break;
			}
			text-> server( self, text, cefFetch, text_sv);
			sv_free( text_sv);
		}
	}
	apc_clipboard_close( self);
}

Bool
Clipboard_format_exists( Handle self, char * format)
{
	Bool ret;
	PClipboardFormatReg fr = first_that( self, (void*)find_format, format);
	if ( !fr) return false;
	my-> open( self);
	ret = apc_clipboard_has_format( self, fr-> sysId);
	my-> close( self);
	return ret;
}

SV *
Clipboard_fetch( Handle self, char * format)
{
	SV * ret;
	PClipboardFormatReg fr = first_that( self, (void*)find_format, format);
	my-> open( self);
	if ( !fr || !my-> format_exists( self, format))
		ret = newSVsv( NULL_SV);
	else
		ret = fr-> server( self, fr, cefFetch, NULL_SV);
	my-> close( self);
	return ret;
}

Bool
Clipboard_store( Handle self, char * format, SV * data)
{
	PClipboardFormatReg fr = first_that( self, (void*)find_format, format);

	if ( !fr) return false;
	if ( !my-> open( self)) return false;
	if ( var->  openCount == 1) {
		first_that( self, (void*) reset_written, NULL);
		apc_clipboard_clear( self);
	}
	fr-> server( self, fr, cefStore, data);
	my-> close( self);
	return fr-> success;
}

void
Clipboard_clear( Handle self)
{
	my-> open( self);
	first_that( self, (void*) reset_written, NULL);
	apc_clipboard_clear( self);
	my-> close( self);
}

SV *
Clipboard_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_clipboard_get_handle( self));
	return newSVpv( buf, 0);
}


Bool
Clipboard_register_format( Handle self, char * format)
{
	void * proc;
	if (( strlen( format) == 0)          ||
		( strcmp( format, "Text") == 0)  ||
		( strcmp( format, "UTF8") == 0)  ||
		( strcmp( format, "Image") == 0))
		return false;
	proc = Clipboard_register_format_proc( self, format, (void*)binary_server);
	return proc != NULL;
}


XS( Clipboard_get_formats_FROMPERL)
{
	dXSARGS;
	Handle self;
	int i;
	Bool include_unregistered;

	if ( items != 1 && items != 2)
		croak ("Invalid usage of Clipboard.get_formats");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Clipboard.get_formats");
	include_unregistered = (items > 1) ? SvBOOL(ST(1)) : false;
	my-> open( self);
	if ( include_unregistered ) {
		PList list = apc_clipboard_get_formats(self);
		if (list) for ( i = 0; i < list->count; i++) {
			XPUSHs( sv_2mortal( newSVpv( (char*) list->items[i], 0)));
			free( (void*) list->items[i] );
		}
		free(list);
	} else {
		PClipboardFormatReg list = formats;
		for ( i = 0; i < formatCount; i++) {
			if ( !apc_clipboard_has_format( self, list[ i]. sysId)) continue;
			XPUSHs( sv_2mortal( newSVpv( list[ i]. id, 0)));
		}
	}
	my-> close( self);
	PUTBACK;
}

XS( Clipboard_get_registered_formats_FROMPERL)
{
	dXSARGS;
	Handle self;
	int i;
	PClipboardFormatReg list;

	if ( items < 1)
		croak ("Invalid usage of Clipboard.get_registered_formats");
	SP -= items;
	self = gimme_the_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Clipboard.get_registered_formats");
	list = formats;
	EXTEND( sp, formatCount);
	for ( i = 0; i < formatCount; i++)
		PUSHs( sv_2mortal( newSVpv( list[ i]. id, 0)));
	PUTBACK;
}

XS( Clipboard_get_standard_clipboards_FROMPERL)
{
	dXSARGS;
	int i;
	PList l;

	(void)ax; SP -= items;
	l = apc_get_standard_clipboards();
	if ( l && l-> count > 0) {
		EXTEND( sp, l-> count);
		for ( i = 0; i < l-> count; i++) {
			char *cc = (char *)list_at( l, i);
			PUSHs( sv_2mortal( newSVpv(cc, 0)));
		}
	}
	if (l) {
		list_delete_all( l, true);
		plist_destroy( l);
	}
	PUTBACK;
}

void Clipboard_get_formats                       ( Handle self, Bool unr) { warn("Invalid call of Clipboard::get_formats"); }
void Clipboard_get_formats_REDEFINED             ( Handle self, Bool unr) { warn("Invalid call of Clipboard::get_formats"); }
void Clipboard_get_registered_formats            ( Handle self) { warn("Invalid call of Clipboard::get_registered_formats"); }
void Clipboard_get_registered_formats_REDEFINED  ( Handle self) { warn("Invalid call of Clipboard::get_registered_formats"); }
void Clipboard_get_standard_clipboards               ( Handle self) { warn("Invalid call of Clipboard::get_standard_clipboards"); }
void Clipboard_get_standard_clipboards_REDEFINED     ( Handle self) { warn("Invalid call of Clipboard::get_standard_clipboards"); }

static SV *
text_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
	ClipboardDataRec c;

	switch( function) {
	case cefInit:
		return ( SV *) cfText;

	case cefFetch:
		if ( apc_clipboard_get_data( self, cfText, &c)) {
			data = newSVpv(( char*) c. data, c. length);
			free( c. data);
			return data;
		}
		break;

	case cefStore:
		if ( prima_is_utf8_sv( data)) {
			/* jump to UTF8. close() will later downgrade data to ascii, if any */
			instance = formats + cfUTF8;
			return instance-> server( self, instance, cefStore, data);
		} else {
			STRLEN l;
			c. data   = ( Byte*) SvPV( data, l);
			c. length = l;
			instance-> success = apc_clipboard_set_data( self, cfText, &c);
			instance-> written = true;
		}
		break;
	}
	return NULL_SV;
}

static SV *
utf8_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
	ClipboardDataRec c;
	STRLEN l;

	switch( function) {
	case cefInit:
		return ( SV *) cfUTF8;

	case cefFetch:
		if ( apc_clipboard_get_data( self, cfUTF8, &c)) {
			data = newSVpv(( char*) c. data, c. length);
			SvUTF8_on( data);
			free( c. data);
			return data;
		}
		break;

	case cefStore:
		c. data   = ( Byte*) SvPV( data, l);
		c. length = l;
		instance-> success = apc_clipboard_set_data( self, cfUTF8, &c);
		instance-> written = true;
		break;
	}
	return NULL_SV;
}

static SV *
image_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
	ClipboardDataRec c;
	switch( function) {
	case cefInit:
		return ( SV *) cfBitmap;
	case cefFetch:
		if ( apc_clipboard_get_data( self, cfBitmap, &c))
			return newSVsv( PImage(c. image)->  mate);
		break;
	case cefStore:
		c. image = gimme_the_mate( data);

		if ( !kind_of( c. image, CImage)) {
			warn("Not an image passed to clipboard");
			return NULL_SV;
		}
		instance-> success = apc_clipboard_set_data( self, cfBitmap, &c);
		instance-> written = true;
		break;
	}
	return NULL_SV;
}

static SV *
binary_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
	ClipboardDataRec c;
	switch( function) {
	case cefInit:
		return ( SV*) apc_clipboard_register_format( self, instance-> id);
	case cefDone:
		apc_clipboard_deregister_format( self, instance-> sysId);
		break;
	case cefFetch:
		if ( apc_clipboard_get_data( self, instance-> sysId, &c)) {
			SV * ret = newSVpv((char*) c. data, c. length);
			free( c. data);
			return ret;
		}
		break;
	case cefStore:
		if ( !SvOK(data)) {
			c. data = NULL;
			c. length = -1;
		} else {
			STRLEN l;
			c. data   = ( Byte*) SvPV( data, l);
			c. length = l;
		}
		instance-> success = apc_clipboard_set_data( self, instance-> sysId, &c);
		instance-> written = true;
		break;
	}
	return NULL_SV;
}

#ifdef __cplusplus
}
#endif
