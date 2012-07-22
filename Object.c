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
#include "Object.h"
#include <Object.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define my  ((( PObject) self)-> self)
#define var (( PObject) self)

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
   var-> mate = xmate;
   var-> stage = csDeadInInit;
   PUTBACK;
   FREETMPS;
   LEAVE;

   profRef = newRV_inc(( SV *) profile);
   my-> profile_add( self, profRef);
   SPAGAIN;
   {
      dG_EVAL_ARGS;
      ENTER;
      SAVETMPS;
      PUSHMARK( sp);
      XPUSHs( var-> mate);
      sp = push_hv_for_REDEFINED( sp, profile);
      PUTBACK;

      OPEN_G_EVAL;
      PERL_CALL_METHOD( "init", G_VOID|G_DISCARD|G_EVAL);
      if ( SvTRUE( GvSV( PL_errgv))) {
         CLOSE_G_EVAL;
         OPEN_G_EVAL;
         Object_destroy( self);
         CLOSE_G_EVAL;
         croak( "%s", SvPV_nolen( GvSV( PL_errgv)));
      }
      CLOSE_G_EVAL;
      SPAGAIN;
      FREETMPS;
      LEAVE;
   }
   if ( primaObjects)
      hash_store( primaObjects, &self, sizeof( self), (void*)1);
   SvREFCNT_dec( profRef);
   if ( var-> stage > csConstructing) {
      if ( var-> mate && ( var-> mate != nilSV) && SvRV( var-> mate))
         --SvREFCNT( SvRV( var-> mate));
      return nilHandle;
   }
   var-> stage = csNormal;
   my-> setup( self);
   return self;
}

#define csHalfDead csFrozen

static void
protect_chain( Handle self, int direction)
{
   while ( self) {
      var-> destroyRefCount += direction;
      self = var-> owner;
   }
}

void
Object_destroy( Handle self)
{
   SV *mate, *object = nil;
   int enter_stage = var-> stage;

   if ( var-> stage == csDeadInInit) {
      /* lightweight destroy */
      if ( is_opt( optInDestroyList)) {
         list_delete( &postDestroys, self);
         opt_clear( optInDestroyList);
      }
      if ( primaObjects)
         hash_delete( primaObjects, &self, sizeof( self), false);
      mate = var-> mate;
      var-> stage = csDead;
      var-> mate = nilSV;
      if ( mate && object) sv_free( mate);
      return;
   }

   if ( var-> stage > csNormal && var-> stage != csHalfDead)
      return;

   if ( var-> destroyRefCount > 0) {
      if ( !is_opt( optInDestroyList)) {
         opt_set( optInDestroyList);
         list_add( &postDestroys, self);
      }
      return;
   }

   if ( var-> stage == csHalfDead) {
      Handle owner;
      if ( !var-> mate || ( var-> mate == nilSV))
         return;
      object = SvRV( var-> mate);
      if ( !object)
         return;
      var-> stage = csFinalizing;
      recursiveCall++;
      protect_chain( owner = var-> owner, 1);
      my-> done( self);
      protect_chain( owner, -1);
      recursiveCall--;
      if ( is_opt( optInDestroyList)) {
         list_delete( &postDestroys, self);
         opt_clear( optInDestroyList);
      }
      if ( primaObjects)
         hash_delete( primaObjects, &self, sizeof( self), false);
      var-> stage = csDead;
      return;
   }
   var-> stage = csDestroying;
   mate = var-> mate;
   if ( mate && ( mate != nilSV)) {
      object = SvRV( mate);
      if ( object) ++SvREFCNT( object);
   }
   if ( object) {
      Handle owner;
      var-> stage = csHalfDead;
      recursiveCall++;
    /*  ENTER;
        SAVEINT recursiveCall; */
      protect_chain( owner = var-> owner, 1);
      if ( enter_stage > csConstructing) 
         my-> cleanup( self);
      else if ( enter_stage == csConstructing && var-> transient_class) 
         ((PObject_vmt)var-> transient_class)-> cleanup( self);
      if ( var-> stage == csHalfDead) {
         var-> stage = csFinalizing;
         my-> done( self);
         if ( primaObjects)
            hash_delete( primaObjects, &self, sizeof( self), false);
         if ( is_opt( optInDestroyList)) {
            list_delete( &postDestroys, self);
            opt_clear( optInDestroyList);
         }
      }
      protect_chain( owner, -1);
    /*  LEAVE; */
      recursiveCall--;
   }
   var-> stage = csDead;
   var-> mate = nilSV;
   if ( mate && object) sv_free( mate);

   while (( recursiveCall == 0) && ( postDestroys. count > 0)) {
      Handle last = postDestroys. items[ 0];
      recursiveCall++;
      Object_destroy( postDestroys. items[ 0]);
      recursiveCall--;
      if ( postDestroys. count == 0) break;
      if ( postDestroys. items[ 0] != last) continue;
      if ( postDestroys. count == 1)
         croak("RTC0A00: Zombie detected: %p", (void*)last);
      else {
         list_delete_at( &postDestroys, 0);
         list_add( &postDestroys, last);
      }
   }
}

XS( Object_alive_FROMPERL)
{
   dXSARGS;
   Handle _c_apricot_self_;
   int ret;

   if ( items != 1)
      croak("Invalid usage of Prima::Object::%s", "alive");
   _c_apricot_self_ = gimme_the_real_mate( ST( 0));
   SPAGAIN;
   SP -= items;
   if ( _c_apricot_self_ != nilHandle) {
      switch ((( PObject) _c_apricot_self_)-> stage) {
      case csDeadInInit:
      case csConstructing:
          ret = 2;
          break;
      case csNormal:
          ret = 1;
          break;
      default:
          ret = 0;
      }
   } else
      ret = 0;
   XPUSHs( sv_2mortal( newSViv( ret)));
   PUTBACK;
   return;
}


void Object_done    ( Handle self) {}

void Object_init    ( Handle self, HV * profile)
{
   if ( var-> stage != csDeadInInit) croak( "Unexpected call of Object::init");
   var-> stage = csConstructing;
   CORE_INIT_TRANSIENT(Object);
}

void Object_cleanup ( Handle self) {}
void Object_setup( Handle self) {}

#ifdef __cplusplus
}
#endif
