/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "apricot.h"
#include "Application.h"
#include "Image.h"
#include "Clipboard.h"
#include <Clipboard.inc>

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
   long                          sysId;
   ClipboardExchangeFunc         *server;
   void                          *data;
} ClipboardFormatReg, *PClipboardFormatReg;

static SV * text_server  ( Handle self, PClipboardFormatReg, int, SV *);
static SV * image_server ( Handle self, PClipboardFormatReg, int, SV *);
static SV * binary_server( Handle self, PClipboardFormatReg, int, SV *);

static int clipboards = 0;
static int formatCount = 0;
static PClipboardFormatReg formats = nil;

void *
Clipboard_register_format_proc( Handle self, char * format, void * serverProc);

void
Clipboard_init( Handle self, HV * profile)
{
   if ( !application) croak("RTC0020: Cannot create clipboard without application instance");
   inherited init( self, profile);
   CComponent( application)-> attach( application, self);
   if ( !apc_clipboard_create(self))
      croak( "RTC0022: Cannot create clipboard");
   if (clipboards == 0) {
      Clipboard_register_format_proc( self, "Text",  text_server);
      Clipboard_register_format_proc( self, "Image", image_server);
   }
   clipboards++;
}

void
Clipboard_done( Handle self)
{
   clipboards--;
   if ( clipboards == 0) {
      while( formatCount)
         my-> deregister_format( self, formats-> id);
   }
   CComponent( application)-> detach( application, self, false);
   apc_clipboard_destroy(self);
   inherited done( self);
}

typedef Bool ActionProc ( Handle self, PClipboardFormatReg item, void * params);
typedef ActionProc *PActionProc;

static PClipboardFormatReg
first_that( Handle self, void * actionProc, void * params)
{
   int i;
   PClipboardFormatReg list = formats;
   if ( actionProc == nil) return nilHandle;
   for ( i = 0; i < formatCount; i++) {
      if ((( PActionProc) actionProc)( self, list+i, params))
         return list+i;
   }
   return nil;
}

static Bool
find_format( Handle self, PClipboardFormatReg item, char *format)
{
   return strcmp( item-> id, format) == 0;
}

void *
Clipboard_register_format_proc( Handle self, char * format, void * serverProc)
{
   PClipboardFormatReg list = first_that( self, find_format, format);
   if ( list) {
      my-> deregister_format( self, format);
   }
   list = malloc( sizeof( ClipboardFormatReg) * ( formatCount + 1));
   if ( formats != nil) {
      memcpy( list, formats, sizeof( ClipboardFormatReg) * formatCount);
      free( formats);
   }
   formats = list;
   list += formatCount++;
   list-> id     = duplicate_string( format);
   list-> server = ( ClipboardExchangeFunc *) serverProc;
   list-> sysId  = ( long) list-> server( self, list, cefInit, nilSV);
   return list;
}

void
Clipboard_deregister_format( Handle self, char * format)
{
   PClipboardFormatReg fr = first_that( self, find_format, format);
   PClipboardFormatReg list = formats;
   if ( fr == nil) return;
   fr-> server( self, fr, cefDone, nilSV);
   free( fr-> id);
   formatCount--;
   memcpy( fr, fr + 1, sizeof( ClipboardFormatReg) * ( formatCount - ( fr - list)));
   if ( formatCount > 0) {
      fr = malloc( sizeof( ClipboardFormatReg) * formatCount);
      memcpy( fr, list, sizeof( ClipboardFormatReg) * formatCount);
   } else
      fr = nil;
   free( formats);
   formats = fr;
}

Bool
Clipboard_open( Handle self)
{
   var-> openCount++;
   if ( var-> openCount > 1) return true;
   return apc_clipboard_open( self);
}

void
Clipboard_close( Handle self)
{
   if ( var->  openCount > 0) {
     var->  openCount--;
     if ( var->  openCount > 0) return;
     apc_clipboard_close( self);
   } else
      var->  openCount = 0;
}

Bool
Clipboard_format_exists( Handle self, char * format)
{
   Bool ret;
   PClipboardFormatReg fr = first_that( self, find_format, format);
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
   PClipboardFormatReg fr = first_that( self, find_format, format);
   my-> open( self);
   if ( !fr || !my-> format_exists( self, format))
      ret = newSVsv( nilSV);
   else
      ret = fr-> server( self, fr, cefFetch, nilSV);
   my-> close( self);
   return ret;
}

void
Clipboard_store( Handle self, char * format, SV * data)
{
   PClipboardFormatReg fr = first_that( self, find_format, format);

   if ( !fr) return;
   my-> open( self);
   if ( var->  openCount == 1) apc_clipboard_clear( self);
   fr-> server( self, fr, cefStore, data);
   my-> close( self);
}

void
Clipboard_clear( Handle self)
{
   my-> open( self);
   apc_clipboard_clear( self);
   my-> close( self);
}

int
Clipboard_get_format_count( Handle self)
{
   PClipboardFormatReg list = formats;
   int i, ret = 0;

   my-> open( self);
   for ( i = 0; i < formatCount; i++) {
      if ( !apc_clipboard_has_format( self, list[ i]. sysId)) continue;
      ret++;
   }
   my-> close( self);
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
   return formatCount;
}

Bool
Clipboard_register_format( Handle self, char * format)
{
   void * proc;
   if (( strlen( format) == 0)          ||
       ( strcmp( format, "Text") == 0)  ||
       ( strcmp( format, "Image") == 0))
      return false;
   proc = Clipboard_register_format_proc( self, format, binary_server);
   return proc != nil;
}


XS( Clipboard_get_formats_FROMPERL)
{
   dXSARGS;
   Handle self;
   int i;
   PClipboardFormatReg list;

   if ( items != 1)
      croak ("Invalid usage of Clipboard.get_formats");
   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Clipboard.get_formats");
   my-> open( self);
   list = formats;
   for ( i = 0; i < formatCount; i++) {
      if ( !apc_clipboard_has_format( self, list[ i]. sysId)) continue;
      XPUSHs( sv_2mortal( newSVpv( list[ i]. id, 0)));
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
   if ( self == nilHandle)
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

   fprintf( stderr, "items: %ld\n", items);
   (void)ax; SP -= items;
   l = apc_get_standard_clipboards();
   fprintf( stderr, "ilcnt: %d\n", l-> count);
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

void Clipboard_get_formats                       ( Handle self) { warn("Invalid call of Clipboard::get_formats"); }
void Clipboard_get_formats_REDEFINED             ( Handle self) { warn("Invalid call of Clipboard::get_formats"); }
void Clipboard_get_registered_formats            ( Handle self) { warn("Invalid call of Clipboard::get_registered_formats"); }
void Clipboard_get_registered_formats_REDEFINED  ( Handle self) { warn("Invalid call of Clipboard::get_registered_formats"); }
void Clipboard_get_standard_clipboards               ( Handle self) { warn("Invalid call of Clipboard::get_standard_clipboards"); }
void Clipboard_get_standard_clipboards_REDEFINED     ( Handle self) { warn("Invalid call of Clipboard::get_standard_clipboards"); }

static SV *
text_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
   switch( function) {
   case cefInit:
      return ( SV *) cfText;
   case cefFetch:
      {
         int len;
         char *str;
         SV * ret;

         str = apc_clipboard_get_data( self, cfText, &len);
         if ( str) {
            ret = newSVpv( str, 0);
            free( str);
            return ret;
         }
      }
      break;
   case cefStore:
      {
         char *str = SvPV( data, na);
         apc_clipboard_set_data( self, cfText, ( void*) str, strlen( str)+1);
      }
      break;
   }
   return nilSV;
}

static SV *
image_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
    switch( function) {
    case cefInit:
       return ( SV *) cfBitmap;
    case cefFetch:
       {
          HV * profile = newHV();
          Handle image;
          image = Object_create( "Prima::Image", profile);
          sv_free(( SV *) profile);
          if ( apc_clipboard_get_data( self, cfBitmap, (void*)(&image)) != nil) {
             --SvREFCNT( SvRV( PImage(image)->  mate));
             return newSVsv( PImage(image)->  mate);
          }
          Object_destroy( image);
       }
       break;
    case cefStore:
       {
          Handle image = gimme_the_mate( data);

          if ( !kind_of( image, CImage)) {
             warn("RTC0023: Not an image passed to clipboard");
             return nilSV;
          }
          apc_clipboard_set_data( self, cfBitmap, ( void *) image, 0);
       }
       break;
    }
    return nilSV;
}

static SV *
binary_server( Handle self, PClipboardFormatReg instance, int function, SV * data)
{
   switch( function) {
   case cefInit:
      return ( SV*) apc_clipboard_register_format( self, instance-> id);
   case cefDone:
      apc_clipboard_deregister_format( self, instance-> sysId);
      break;
   case cefFetch:
      {
         int len;
         void *xdata = apc_clipboard_get_data( self, instance-> sysId, &len);
         if ( xdata) {
            SV * ret = newSVpv( xdata, len);
            free( xdata);
            return ret;
         }
      }
      break;
   case cefStore:
      {
         STRLEN len;
         void * xdata = SvPV( data, len);
         apc_clipboard_set_data( self, instance-> sysId, xdata, len);
      }
      break;
   }
   return nilSV;
}
