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
#include "Object.h"
#include "Object.inc"

#undef  my
#define my  ((( PObject) self)-> self)->
#define var (( PObject) self)->

SV *
Object_can( Handle self, char *methodName, Bool cacheIt)
{
   CV *cv = query_method( self, methodName, cacheIt);
   if (cv) return newRV(( SV*) cv);
   return nilSV;
}

Handle
Object_create( char *className, HV * profile)
{
   dSP;
   Handle self = 0;

   SV *xmate;
   SV *profRef;

   if ( primaObjects == nil)
      return nilHandle;

   ENTER;
   SAVETMPS;
   PUSHMARK( sp);
   XPUSHs( sv_2mortal( newSVpv( className, 0)));
   PUTBACK;
   PERL_CALL_METHOD( "CREATE", G_SCALAR);
   SPAGAIN;
   xmate = newRV_inc( SvRV( POPs));
   self = create_mate( xmate);
   var mate = xmate;
   PUTBACK;
   FREETMPS;
   LEAVE;

   profRef = newRV_inc(( SV *) profile);
   my profile_add( self, profRef);
   var stage = csConstructing;
   SPAGAIN;
   if ( primaObjects)
      hash_store( primaObjects, &self, sizeof( self), (void*)1);
   if ( my init == Object_init_REDEFINED)
   {
      ENTER;
      SAVETMPS;
      PUSHMARK( sp);
      XPUSHs( var mate);
      sp = push_hv_for_REDEFINED( sp, profile);
      PUTBACK;
      PERL_CALL_METHOD( "init", G_VOID|G_DISCARD);
      SPAGAIN;
      PUTBACK;
      FREETMPS;
      LEAVE;
   }
   else
   {
      my init( self, profile);
   }
   SvREFCNT_dec( profRef);
   if ( var stage != csConstructing) {
      if ( var mate && ( var mate != nilSV) && SvRV( var mate))
         --SvREFCNT( SvRV( var mate));
      return nilHandle;
   }
   var stage = csNormal;
   my setup( self);
   return self;
}

#define csHalfDead csFrozen

void
Object_destroy( Handle self)
{
   SV *mate, *object = nil;
   if ( var stage > csNormal && var stage != csHalfDead)
      return;
   if ( var stage == csHalfDead) {
      if ( !var mate || ( var mate == nilSV))
         return;
      object = SvRV( var mate);
      if ( !object)
         return;
      var stage = csFinalizing;
      my done( self);
      if ( primaObjects)
         hash_delete( primaObjects, &self, sizeof( self), false);
      var stage = csDead;
      return;
   }
   var stage = csDestroying;
   mate = var mate;
   if ( mate && ( mate != nilSV)) {
      object = SvRV( mate);
      if ( object) ++SvREFCNT( object);
   }
   if ( object) {
      var stage = csHalfDead;
      my cleanup( self);
      if (var stage == csHalfDead) {
         var stage = csFinalizing;
         my done( self);
         if ( primaObjects)
            hash_delete( primaObjects, &self, sizeof( self), false);
      }
   }
   var stage = csDead;
   var mate = nilSV;
   if ( mate && object) sv_free( mate);
}

extern Handle
gimme_the_real_mate( SV *perlObject);


XS( Object_alive_FROMPERL)
{
   dXSARGS;
   Handle _c_apricot_self_;
   int ret;

   if ( items != 1)
      croak("Invalid usage of Prima::Object::%s", "alive");
   _c_apricot_self_ = gimme_the_real_mate( ST( 0));
   if ( _c_apricot_self_ == nilHandle)
      croak( "Illegal object reference passed to Prima::Object::%s", "alive");
   SPAGAIN;
   SP -= items;

   switch ((( PObject) _c_apricot_self_)-> stage) {
   case csConstructing:
       ret = 2;
       break;
   case csNormal:
       ret = 1;
       break;
   default:
       ret = 0;
   }
   XPUSHs( sv_2mortal( newSViv( ret)));
   PUTBACK;
   return;
}


void Object_done    ( Handle self) {}
void Object_init    ( Handle self, HV * profile) {}
void Object_cleanup ( Handle self) {}
void Object_setup   ( Handle self) {}

