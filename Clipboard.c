#include "apricot.h"
#include "Application.h"
#include "Image.h"
#include "Clipboard.h"
#include "Clipboard.inc"

#undef  my
#define inherited CComponent->
#define my  ((( PClipboard) self)-> self)->
#define var (( PClipboard) self)->

#define cefInit     0
#define cefDone     1
#define cefStore    2
#define cefFetch    3

typedef SV* ClipboardExchangeFunc ( void * instance, int function, int subCommand, SV * data);
typedef ClipboardExchangeFunc *PClipboardExchangeFunc;


typedef struct _ClipboardFormatReg
{
   char                   *id;
   long                    sysId;
   PClipboardExchangeFunc  server;
   void                   *inst;
} ClipboardFormatReg, *PClipboardFormatReg;


SV * text_server  ( void *, int, int, SV *);
SV * image_server ( void *, int, int, SV *);

void
Clipboard_init( Handle self, HV * profile)
{
   PApplication app = ( PApplication) application;
   if ( !application) croak("RTC0020: Cannot create clipboard without application instance");
   if ( app-> clipboard) croak( "RTC0021: Attempt to create more than one clipboard instance");
   inherited init( self, profile);
   CComponent( application)-> attach( application, self);
   if ( !apc_clipboard_create())
      croak( "RTC0022: Cannot create clipboard");
   {
      my register_format( self, "Text",  text_server);
      my register_format( self, "Image", image_server);
   }
}

void
Clipboard_done( Handle self)
{
   PApplication app = ( PApplication) application;
   if ( var openCount > 0) apc_clipboard_close();
   while( var formatCount)
       my deregister_format( self, (( PClipboardFormatReg) var formats)-> id);
   CComponent( application)-> detach( application, self, false);
   apc_clipboard_destroy();
   app-> clipboard = nilHandle;
   inherited done( self);
}

typedef Bool ActionProc ( Handle self, PClipboardFormatReg item, void * params);
typedef ActionProc *PActionProc;

static PClipboardFormatReg
first_that( Handle self, void * actionProc, void * params)
{
   int i;
   PClipboardFormatReg list = ( PClipboardFormatReg) var formats;
   if ( actionProc == nil) return nilHandle;
   for ( i = 0; i < var formatCount; i++)
   {
      if ((( PActionProc) actionProc)( self, list+i, params))
         return list+i;
   }
   return nil;
}

static Bool
find_format( Handle self, PClipboardFormatReg item, char * format)
{
   return strcmp( item-> id, format) == 0;
}

void *
Clipboard_register_format( Handle self, char * format, void * serverProc)
{
   PClipboardFormatReg fr = first_that( self, find_format, format);
   PClipboardFormatReg list;
   if ( fr)
   {
      fr = ( PClipboardFormatReg)( fr-> server);
      my deregister_format( self, format);
   }
   list = malloc( sizeof( ClipboardFormatReg) * ( var formatCount + 1));
   if ( var formats != nil)
   {
      memcpy( list, var formats, sizeof( ClipboardFormatReg) * var formatCount);
      free( var formats);
   }
   var formats = list;
   list += var formatCount++;
   strcpy( list-> id  = malloc( strlen( format) + 1), format);
   list-> server      = ( PClipboardExchangeFunc) serverProc;
   list-> inst        = ( void *) fr;
   list-> sysId       = (long) list-> server( &list-> inst, cefInit, 0, nilSV);
   return ( void *) fr;
}

void
Clipboard_deregister_format( Handle self, char * format)
{
   PClipboardFormatReg fr = first_that( self, find_format, format);
   PClipboardFormatReg list = ( PClipboardFormatReg) var formats;
   if ( fr == nil) return;
   fr-> server( fr-> inst, cefDone, 0, nilSV);
   free( fr-> id);
   var formatCount--;
   memcpy( fr, fr + 1, sizeof( ClipboardFormatReg) * ( var formatCount - ( fr - list)));
   if ( var formatCount > 0)
   {
      fr = malloc( sizeof( ClipboardFormatReg) * var formatCount);
      memcpy( fr, list, sizeof( ClipboardFormatReg) * var formatCount);
   } else
      fr = nil;
   free( var formats);
   var formats = fr;
}

Bool
Clipboard_open( Handle self)
{
   var openCount++;
   if ( var openCount > 1) return true;
   return apc_clipboard_open();
}

void
Clipboard_close( Handle self)
{
   if ( var openCount > 0)
   {
     var openCount--;
     if ( var openCount > 0) return;
     apc_clipboard_close();
   } else
      var openCount = 0;
}

Bool
Clipboard_format_exists( Handle self, char * format)
{
   Bool ret;
   PClipboardFormatReg fr = first_that( self, find_format, format);
   if ( !fr) return false;
   my open( self);
   ret = apc_clipboard_has_format( fr-> sysId);
   my close( self);
   return ret;
}

SV *
Clipboard_fetch( Handle self, char * format)
{
   PClipboardFormatReg fr = first_that( self, find_format, format);
   SV * ret;
   my open( self);
   if ( !fr || !my format_exists( self, format))
      ret = newSVsv( nilSV);
   else
      ret = fr-> server( fr-> inst, cefFetch, 0, nilSV);
   my close( self);
   return ret;
}

void
Clipboard_store( Handle self, char * format, SV * data)
{
   PClipboardFormatReg fr = first_that( self, find_format, format);
   if ( !fr) return;
   my open( self);
   if ( var openCount == 1) apc_clipboard_clear();
   fr-> server( fr-> inst, cefStore, 0, data);
   my close( self);
}

void
Clipboard_clear( Handle self)
{
   my open( self);
   apc_clipboard_clear();
   my close( self);
}

int
Clipboard_get_format_count( Handle self)
{
   PClipboardFormatReg list = ( PClipboardFormatReg) var formats;
   int i, ret = 0;
   my open( self);
   for ( i = 0; i < var formatCount; i++)
   {
      if ( !apc_clipboard_has_format( list[ i]. sysId)) continue;
      ret++;
   }
   my close( self);
   return ret;
}

SV *
Clipboard_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_clipboard_get_handle( self));
   return newSVpv( buf, 0);
}


int
Clipboard_get_registered_format_count( Handle self)
{
   return var formatCount;
}


XS( Clipboard_get_formats_FROMPERL)
{
   dXSARGS;
   Handle self;
   int i;
   PClipboardFormatReg list;

   if ( items < 1)
      croak ("Invalid usage of Clipboard.get_formats");
   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Clipboard.get_formats");
   my open( self);
   list = ( PClipboardFormatReg) var formats;
   for ( i = 0; i < var formatCount; i++)
   {
      if ( !apc_clipboard_has_format( list[ i]. sysId)) continue;
      XPUSHs( sv_2mortal( newSVpv( list[ i]. id, 0)));
   }
   my close( self);
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
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Clipboard.get_registered_formats");
   list = ( PClipboardFormatReg) var formats;
   EXTEND( sp, var formatCount);
   for ( i = 0; i < var formatCount; i++)
      PUSHs( sv_2mortal( newSVpv( list[ i]. id, 0)));
   PUTBACK;
}


void Clipboard_get_formats                     ( Handle self) { warn("Invalid call of Clipboard::get_formats"); }
void Clipboard_get_formats_REDEFINED           ( Handle self) { warn("Invalid call of Clipboard::get_formats"); }
void Clipboard_get_registered_formats          ( Handle self) { warn("Invalid call of Clipboard::get_registered_formats"); }
void Clipboard_get_registered_formats_REDEFINED( Handle self) { warn("Invalid call of Clipboard::get_registered_formats"); }


SV *
text_server ( void * instance, int function, int subCommand, SV * data)
{
   switch( function)
   {
      case cefInit:
         return (SV*)cfText;
      case cefFetch:
         {
            int len;
            char *str = apc_clipboard_get_data( cfText, &len);
            SV * ret;
            if ( str) {
               ret = newSVpv( str, 0);
               free( str);
               return ret;
            }
         }
         break;
      case cefStore:
         {
            char * str = SvPV( data, na);
            apc_clipboard_set_data( cfText, ( void*) str, strlen( str)+1);
         }
         break;
   }
   return nilSV;
}

SV *
image_server( void * instance, int function, int subCommand, SV * data)
{
    switch( function)
    {
      case cefInit:
         return (SV*)cfBitmap;
      case cefFetch:
         {
            HV * profile = newHV();
            Handle self = Object_create( "Image", profile);
            sv_free(( SV *) profile);
            if ( apc_clipboard_get_data( cfBitmap, (void*)(&self)) != nil) {
               --SvREFCNT( SvRV( var mate));
               return newSVsv( var mate);
            }
            Object_destroy( self);
         }
         break;
      case cefStore:
         {
            Handle image = gimme_the_mate( data);
            if ( !kind_of( image, CImage)) {
               warn("RTC0023: Not an image passed to clipboard");
               return nilSV;
            }
            apc_clipboard_set_data( cfBitmap, (void*)image, 0);
         }
         break;
    }
   return nilSV;
}

/*
// This is example of writing custom clipboard format servers -
// the only difference is that format ID is retrieved dynamically
// from system in cefInit and then released in cefDone.
// Although, user server can subplace one of standart formats and
// ( mainly) can be system-dependent.

SV *
user_server ( void * instance, int function, int subCommand, SV * data)
{
   switch( function)
   {
      case cefInit:
         *((int*) instance) = apc_clipboard_register_format("User data");
         return *((int*) instance);
      case cefDone:
         apc_clipboard_deregister_format(( int)instance);
         break;
      case cefFetch:
         .....
}
*/


