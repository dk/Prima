/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
 *
 * $Id$
 */

#include "apricot.h"
#include "Utils.h"
#include <Utils.inc>

#ifdef __cplusplus
extern "C" {
#endif


SV *Utils_query_drives_map( char *firstDrive)
{
   char map[ 256];
   apc_query_drives_map( firstDrive, map, sizeof( map));
   return newSVpv( map, 0);
}

int
Utils_get_os()
{
   return apc_application_get_os_info( nil, 0, nil, 0, nil, 0, nil, 0);
}

int
Utils_get_gui()
{
   return apc_application_get_gui_info( nil, 0);
}

long Utils_ceil( double x)
{
    return ceil( x);
}

long Utils_floor( double x)
{
    return floor( x);
}

XS(Utils_getdir_FROMPERL) {
   dXSARGS;
   Bool wantarray = ( GIMME_V == G_ARRAY);
   char *dirname;
   PList dirlist;
   int i;

   if ( items >= 2) {
      croak( "invalid usage of Prima::Utils::getdir");
   }
   dirname = SvPV_nolen( ST( 0));
   dirlist = apc_getdir( dirname);
   SPAGAIN;
   SP -= items;
   if ( wantarray) {
      if ( dirlist) {
         EXTEND( sp, dirlist-> count);
         for ( i = 0; i < dirlist-> count; i++) {
            PUSHs( sv_2mortal(newSVpv(( char *)dirlist-> items[i], 0)));
            free(( char *)dirlist-> items[i]);
         }
         plist_destroy( dirlist);
      }
   } else {
      if ( dirlist) {
         XPUSHs( sv_2mortal( newSViv( dirlist-> count / 2)));
         for ( i = 0; i < dirlist-> count; i++) {
            free(( char *)dirlist-> items[i]);
         }
         plist_destroy( dirlist);
      } else {
         XPUSHs( &PL_sv_undef);
      }
   }
   PUTBACK;
   return;
}

#ifdef __cplusplus
}
#endif
