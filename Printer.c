#include "apricot.h"
#include "Printer.h"
#include "Printer.inc"
#undef  my
#define inherited CDrawable->
#define my  ((( PPrinter) self)-> self)->
#define var (( PPrinter) self)->

void
Printer_init( Handle self, HV * profile)
{
   char * prn;
   inherited init( self, profile);
   if ( !apc_prn_create( self))
      croak("RTC0070: Cannot create printer");
   prn = pget_c( printer);
   if ( strlen( prn) == 0) prn = my get_default_printer( self);
   my set_printer( self, prn);
}

void
Printer_done( Handle self)
{
   apc_prn_destroy( self);
   inherited done( self);
}

Bool
Printer_begin_doc( Handle self, char * docName)
{
   char buf[ 256];
   if ( is_opt( optInDraw)) return false;
   if ( !docName || *docName == '\0') {
      snprintf( buf, 256, "APC: %s", (( PComponent) application)-> name);
      docName = buf;
   }
   if ( is_opt( optInDrawInfo))
      my end_paint_info( self);
   inherited begin_paint( self);
   if ( !apc_prn_begin_doc( self, docName)) return false;
   return true;
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

Bool Printer_begin_paint( Handle self) { return my begin_doc( self, ""); }
void Printer_end_paint( Handle self) { my abort_doc( self); }

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

int
Printer_get_width( Handle self)
{
   Point p = my get_size( self);
   return p. x;
}

int
Printer_get_height( Handle self)
{
   Point p = my get_size( self);
   return p. y;
}

SV *
Printer_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_prn_get_handle( self));
   return newSVpv( buf, 0);
}
