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
#include "DeviceBitmap.h"
#include "DeviceBitmap.inc"

#undef  my
#define inherited CDrawable->
#define my  ((( PDeviceBitmap) self)-> self)
#define var (( PDeviceBitmap) self)

void
DeviceBitmap_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   var-> w = pget_i( width);
   var-> h = pget_i( height);
   var-> monochrome = pget_B( monochrome);
   if ( !apc_dbm_create( self, var-> monochrome))
      croak("RTC0110: Cannot create device bitmap");
   inherited begin_paint( self);
   opt_set( optInDraw);
}

void
DeviceBitmap_done( Handle self)
{
   apc_dbm_destroy( self);
   inherited done( self);
}

Bool DeviceBitmap_begin_paint      ( Handle self) { return true;}
Bool DeviceBitmap_begin_paint_info ( Handle self) { return true;}
void DeviceBitmap_end_paint        ( Handle self) { return;}
Bool DeviceBitmap_get_monochrome   ( Handle self) { return var-> monochrome; }

static Handle xdup( Handle self, char * className)
{
   Handle h;
   PDrawable i;
   HV * profile = newHV();

   pset_H( owner,        var-> owner);
   pset_i( width,        var-> w);
   pset_i( height,       var-> h);
   pset_i( type,         var-> monochrome ? imMono : imRGB);

   h = Object_create( className, profile);
   sv_free(( SV *) profile);
   i = ( PDrawable) h;
   i-> self-> begin_paint( h);
   i-> self-> put_image( h, 0, 0, self);
   i-> self-> end_paint( h);
   --SvREFCNT( SvRV( i-> mate));
   return h;
}

Handle DeviceBitmap_image( Handle self) { return xdup( self, "Prima::Image"); }
Handle DeviceBitmap_icon( Handle self) { return xdup( self, "Prima::Icon"); }

SV *
DeviceBitmap_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_dbm_get_handle( self));
   return newSVpv( buf, 0);
}

