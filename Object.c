#include "apricot.h"
#include "Object.h"
#include "Object.inc"

#undef  my
#define my  ((( PObject) self)-> self)->
#define var (( PObject) self)->

#ifdef PARANOID_MALLOC
extern PHash objects;
#endif

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
#ifdef PARANOID_MALLOC
   hash_store( objects, &self, sizeof( self), 1);
#endif
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
   var stage = csNormal;
   my setup( self);
   return self;
}

#define csHalfDead csFrozen

void
Object_destroy( Handle self)
{
   SV *mate, *object = nil;
   if ( var stage > csNormal && var stage != csHalfDead) return;
   if (var stage == csHalfDead) {
      if ( !var mate) return;
      object = SvRV( var mate);
      if ( !object) return;
      var stage = csFinalizing;
      my done( self);
#ifdef PARANOID_MALLOC
   hash_delete( objects, &self, sizeof( self), false);
#endif
      var stage = csDead;
      return;
   }
   var stage = csDestroying;
   mate = var mate;
   if ( mate) {
      object = SvRV( mate);
      if ( object) ++SvREFCNT( object);
   }
   if ( object) {
      var stage = csHalfDead;
      my cleanup( self);
      if (var stage == csHalfDead) {
         var stage = csFinalizing;
         my done( self);
#ifdef PARANOID_MALLOC
   hash_delete( objects, &self, sizeof( self), false);
#endif
      }
   }
   var stage = csDead;
   var mate = nil;
   if ( mate && object) sv_free( mate);
}

void Object_done    ( Handle self) {}
void Object_init    ( Handle self, HV * profile) {}
void Object_cleanup ( Handle self) {}
void Object_setup   ( Handle self) {}

