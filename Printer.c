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
   char * prn;
   inherited init( self, profile);
   CComponent( var-> owner)-> attach( var-> owner, self);
   if ( !apc_prn_create( self))
      croak("RTC0070: Cannot create printer");
   prn = pget_c( printer);
   if ( strlen( prn) == 0) prn = my-> get_default_printer( self);
   my-> set_printer( self, prn);
}

void
Printer_done( Handle self)
{
   CComponent( var-> owner)-> detach( var-> owner, self, false);
   apc_prn_destroy( self);
   inherited done( self);
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
   inherited begin_paint( self);
   if ( !( ok = apc_prn_begin_doc( self, docName)))
      inherited end_paint( self);
   return ok;
}

void
Printer_new_page( Handle self)
{
   if ( !is_opt( optInDraw)) return;
   apc_prn_new_page( self);
}

void
Printer_end_doc( Handle self)
{
   if ( !is_opt( optInDraw)) return;
   apc_prn_end_doc( self);
   inherited end_paint( self);
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
   if ( is_opt( optInDrawInfo)) return false;
   inherited begin_paint_info( self);
   if ( !( ok = apc_prn_begin_paint_info( self)))
      inherited end_paint_info( self);
   return ok;
}

void
Printer_end_paint_info( Handle self)
{
   if ( !is_opt( optInDrawInfo)) return;
   apc_prn_end_paint_info( self);
   inherited end_paint_info( self);
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
   snprintf( buf, 256, "0x%08lx", apc_prn_get_handle( self));
   return newSVpv( buf, 0);
}

SV*
Printer_fonts( Handle self, char * name)
{
   int count, i;
   AV * glo = newAV();
   PFont fmtx = apc_fonts( self, strlen( name) ? name : nil, &count);
   for ( i = 0; i < count; i++) {
      SV * sv      = sv_Font2HV( &fmtx[ i]);
      HV * profile = ( HV*) SvRV( sv);
      pdelete( resolution);
      pdelete( codepage);
      av_push( glo, sv);
   }
   free( fmtx);
   return newRV_noinc(( SV *) glo);
}

Point
Printer_resolution( Handle self, Bool set, Point resolution)
{
   if ( set)
      croak("Attempt to write read-only property %s", "Printer::resolution");
   return apc_prn_get_resolution( self);
}


#ifdef __cplusplus
}
#endif
