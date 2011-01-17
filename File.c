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
#ifdef PerlIO
typedef PerlIO *FileStream;
#else
#define PERLIO_IS_STDIO 1
typedef FILE *FileStream;
#define PerlIO_fileno(f) fileno(f)
#endif
#include "File.h"
#include <File.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CComponent
#define my  ((( PFile) self)-> self)
#define var (( PFile) self)

static void File_reset_notifications( Handle self);

void
File_init( Handle self, HV * profile)
{
   dPROFILE;
   var-> fd = -1;
   inherited-> init( self, profile);
   my-> set_mask( self, pget_i( mask));
   var-> eventMask2 =
     ( query_method( self, "on_read",    0) ? feRead      : 0) |
     ( query_method( self, "on_write",   0) ? feWrite     : 0) |
     ( query_method( self, "on_exception", 0) ? feException : 0);
   File_reset_notifications( self);
   my-> set_file( self, pget_sv( file));
   CORE_INIT_TRANSIENT(File);
}

void
File_cleanup( Handle self)
{
   my-> set_file( self, nilSV);
   inherited-> cleanup( self);
}

Bool
File_is_active( Handle self, Bool autoDetach)
{
   if (!var-> file || SvTYPE( var-> file) != SVt_NULL)
      return false;
   if ( !IoIFP( sv_2io( var-> file))) {
      if ( autoDetach)
         my-> set_file( self, nilSV);
      return false;
   }
   return true;
}

void
File_handle_event( Handle self, PEvent event)
{
   inherited-> handle_event ( self, event);
   if ( var-> stage > csNormal) return;
   switch ( event-> cmd) {
   case cmFileRead:
      my-> notify( self, "<sS", "Read", var-> file ? var-> file : nilSV);
      break;
   case cmFileWrite:
      my-> notify( self, "<sS", "Write", var-> file ? var-> file : nilSV);
      break;
   case cmFileException:
      my-> notify( self, "<sS", "Exception", var-> file ? var-> file : nilSV);
      break;
   }
}

SV *
File_file( Handle self, Bool set, SV * file)
{
   if ( !set)
      return var-> file ? newSVsv( var-> file) : nilSV;
   if ( var-> file) {
      apc_file_detach( self);
      sv_free( var-> file);
   }
   var-> file = nil;
   var-> fd = -1;
   if ( file && ( SvTYPE( file) != SVt_NULL)) {
      FileStream f = IoIFP(sv_2io(file));
      if (!f) {
         warn("RTC0A0: Not a IO reference passed to File::set_file");
      } else {
         var-> file = newSVsv( file);
         var-> fd = PerlIO_fileno( f);
         if ( !apc_file_attach( self)) {
            sv_free( var-> file);
            var-> file = nil;
            var-> fd   = -1;
         }
      }
   }
   return nilSV;
}

SV *
File_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08x", var-> fd);
   return newSVpv( buf, 0);
}

int
File_mask( Handle self, Bool set, int mask)
{
   if ( !set)
      return var-> userMask;
   var-> userMask = mask;
   File_reset_notifications( self);
   return mask;
}

UV
File_add_notification( Handle self, char * name, SV * subroutine, Handle referer, int index)
{
   UV id = inherited-> add_notification( self, name, subroutine, referer, index);
   if ( id != 0) File_reset_notifications( self);
   return id;
}

void
File_remove_notification( Handle self, UV id)
{
   inherited-> remove_notification( self, id);
   File_reset_notifications( self);
}


static void
File_reset_notifications( Handle self)
{
   int i, mask = var-> eventMask2;
   PList  list;
   void * ret[ 3];
   int    cmd[ 3] = { feRead, feWrite, feException};

   if ( var-> eventIDs == nil) {
      var-> eventMask = var-> eventMask2 & var-> userMask;
      return;
   }

   ret[0] = hash_fetch( var-> eventIDs, "Read",      4);
   ret[1] = hash_fetch( var-> eventIDs, "Write",     5);
   ret[2] = hash_fetch( var-> eventIDs, "Exception", 9);

   for ( i = 0; i < 3; i++) {
      if ( ret[i] == nil) continue;
      list = var-> events + PTR2IV( ret[i]) - 1;
      if ( list-> count > 0) mask |= cmd[ i];
   }

   mask &= var-> userMask;

   if ( var-> eventMask != mask) {
      var-> eventMask = mask;
      if ( var-> file)
	 apc_file_change_mask( self);
   }
}

#ifdef __cplusplus
}
#endif
