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
#include <ctype.h>
#include "Component.h"
#include <Component.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CObject->
#define my  ((( PComponent) self)-> self)
#define var (( PComponent) self)

typedef Bool ActionProc ( Handle self, Handle item, void * params);
typedef ActionProc *PActionProc;

void
Component_init( Handle self, HV * profile)
{
   dPROFILE;
   SV * res;
   HV * hv;
   HE * he;
   inherited init( self, profile);
   if ( !my-> validate_owner( self, &var-> owner, profile)) {
      var-> stage = csDeadInInit;
      croak( "Illegal 'owner' reference passed to %s::%s%s", my-> className, "init",
	     application ? "" : ". Probably you forgot to include 'use Prima::Application' in your code. Error");
   }
   if ( var-> owner)
      ((( PComponent) var-> owner)-> self)-> attach( var-> owner, self);
   my-> set_name( self, pget_sv( name));
   my-> set_delegations( self, pget_sv( delegations));
   var-> evQueue = plist_create( 8, 8);
   apc_component_create( self);

   res = my-> notification_types( self);
   hv = ( HV *) SvRV( res);
   hv_iterinit( hv);
   while (( he = hv_iternext( hv)) != nil) {
      char buf[ 1024];
      SV ** holder;
      int len = snprintf( buf, 1023, "on%s", HeKEY( he));
      holder = hv_fetch( profile, buf, len, 0);
      if ( holder == nil || SvTYPE( *holder) == SVt_NULL) continue;
      my-> add_notification( self, HeKEY( he), *holder, self, -1);
   }
   sv_free( res);
}

void
Component_setup( Handle self)
{
   Event ev = {cmCreate};
   ev. gen. source = self;
   my-> message( self, &ev);

   if ( var-> owner) {   
      ev. cmd = cmChildEnter;
      ev. gen. source = var-> owner;
      ev. gen. H      = self;
      CComponent( var-> owner)-> message( var-> owner, &ev);
   }
}

static Bool bring_by_name( Handle self, PComponent item, char * name)
{
   return strcmp( name, item-> name) == 0;
}

Handle
Component_bring( Handle self, char * componentName)
{
   return my-> first_that_component( self, (void*)bring_by_name, componentName);
}

static Bool
detach_all( Handle child, Handle self)
{
   my-> detach( self, child, true);
   return false;
}

void
Component_cleanup( Handle self)
{
   Event ev = {cmDestroy};
   
   if ( var-> owner) {   
      Event ev = {cmChildLeave};
      ev. gen. source = var-> owner;
      ev. gen. H      = self;
      CComponent( var-> owner)-> message( var-> owner, &ev);
   }

   if ( var-> components != nil)
      list_first_that( var-> components, (void*)detach_all, ( void*) self);

   ev. gen. source = self;
   my-> message( self, &ev);
}

static Bool
free_private_posts( PostMsg * msg, void * dummy)
{
   sv_free( msg-> info1);
   sv_free( msg-> info2);
   free( msg);
   return false;
}

static Bool
free_queue( PEvent event, void * dummy)
{
   free( event);
   return false;
}

static Bool
free_eventref( Handle self, Handle * org)
{
   if ( var-> refs) list_delete( var-> refs, *org);
   my-> unlink_notifier( self, *org);
   return false;
}

void
Component_done( Handle self)
{
   if ( var-> owner) 
      CComponent( var-> owner)-> detach( var-> owner, self, false);
   if ( var-> eventIDs) {
      int i;
      PList list = var-> events;
      hash_destroy( var-> eventIDs, false);
      var-> eventIDs = nil;
      for ( i = 0; i < var-> eventIDCount; i++) {
         int j;
         for ( j = 0; j < list-> count; j += 2)
            sv_free(( SV *) list-> items[ j + 1]);
         list_destroy( list++);
      }
      free( var-> events);
      var-> events = nil;
   }


   if ( var-> refs) {
      Handle * pself = &self;
      list_first_that( var-> refs, (void*)free_eventref, pself);
      plist_destroy( var-> refs);
      var-> refs = nil;
   }

   if ( var-> postList != nil) {
      list_first_that( var-> postList, (void*)free_private_posts, nil);
      list_destroy( var-> postList);
      free( var-> postList);
      var-> postList = nil;
   }
   if ( var-> evQueue != nil)
   {
      list_first_that( var-> evQueue, (void*)free_queue, nil);
      list_destroy( var-> evQueue);
      free( var-> evQueue);
      var-> evQueue = nil;
   }
   if ( var-> components != nil) {
      list_destroy( var-> components);
      free( var-> components);
      var-> components = nil;
   }
   apc_component_destroy( self);
   free( var-> name);
   var-> name = nil;
   free( var-> evStack);
   var-> evStack = nil;
   inherited done( self);
}

void
Component_attach( Handle self, Handle object)
{
   if ( var-> stage > csNormal) return;

   if ( object && kind_of( object, CComponent)) {
      if ( var-> components == nil)
         var-> components = plist_create( 8, 8);
      else
         if ( list_index_of( var-> components, object) >= 0) {
            warn( "Object attach failed");
            return;
         }
      list_add( var-> components, object);
      SvREFCNT_inc( SvRV(( PObject( object))-> mate));
   } else
       warn( "Object attach failed");
}

void
Component_detach( Handle self, Handle object, Bool kill)
{
   if ( object && ( var-> components != nil)) {
      int index = list_index_of( var-> components, object);
      if ( index >= 0) {
         list_delete_at( var-> components, index);
         --SvREFCNT( SvRV(( PObject( object))-> mate));
         if ( kill) Object_destroy( object);
      }
   }
}

SV *
Component_name( Handle self, Bool set, SV * name)
{
   if ( set) {
      free( var-> name);
      var-> name = nil;
      var-> name = duplicate_string( SvPV_nolen( name));
      opt_assign( optUTF8_name, prima_is_utf8_sv(name));
      if ( var-> stage >= csNormal)
         apc_component_fullname_changed_notify( self);
   } else {
      name = newSVpv( var-> name ? var-> name : "", 0);
      if ( is_opt( optUTF8_name)) SvUTF8_on( name);
      return name;
   }
   return nilSV;
}

Handle
Component_owner( Handle self, Bool set, Handle owner)
{
   HV * profile;
   if ( !set)
       return var-> owner;
   profile = newHV();
   pset_H( owner, owner);
   my-> set( self, profile);
   sv_free(( SV *) profile);
   return nilHandle;
}


void
Component_set( Handle self, HV * profile)
{
   /* this can eliminate unwilling items */
   /* from HV before indirect Object::set */
   my-> update_sys_handle( self, profile);

   if ( pexist( owner)) {
      Handle owner, oldOwner = var-> owner;
      if ( !my-> validate_owner( self, &owner, profile))
         croak( "Illegal 'owner' reference passed to %s::%s", my-> className, "set");

      if ( oldOwner && oldOwner != owner) {
         Event ev;
         ev. cmd = cmChildLeave;
         ev. gen. source = oldOwner;
         ev. gen. H      = self;
         if ( oldOwner)
            CComponent( oldOwner)-> message( oldOwner, &ev);
      }

      my-> migrate( self, owner);
      var-> owner = owner;
      pdelete( owner); 

      if ( oldOwner != owner) {
         Event ev;

         ev. cmd = cmChildEnter;
         ev. gen. source = owner;
         ev. gen. H      = self;
         if ( owner)
            CComponent( owner)-> message( owner, &ev);
         
         ev. cmd = cmChangeOwner;
         ev. gen. source = self;
         ev. gen. H      = oldOwner;
         my-> message( self, &ev);
      }
   }

   inherited set ( self, profile);
}

static Bool
find_dup_msg( PEvent event, int * cmd)
{
   return event-> cmd == *cmd;
}

Bool
Component_message( Handle self, PEvent event)
{
   Bool ret      = false;
   if ( var-> stage == csNormal) {
      if ( var-> evQueue) goto Constructing;
ForceProcess:
      protect_object( self);
      my-> push_event( self);
      my-> handle_event( self, event);
      ret = my-> pop_event( self);
      if ( !ret) event-> cmd = 0;
      unprotect_object( self);
   } else if ( var-> stage == csConstructing) {
      if ( var-> evQueue == nil)
          croak("Object set twice to constructing stage");
Constructing:      
      switch ( event-> cmd & ctQueueMask) {
      case ctDiscardable:
         break;
      case ctPassThrough:
         goto ForceProcess;
      case ctSingle:
         event-> cmd = ( event-> cmd & ~ctQueueMask) | ctSingleResponse;
         if ( list_first_that( var-> evQueue, (void*)find_dup_msg, &event-> cmd) >= 0)
	      break;
      default:
         {
            void * ev = malloc( sizeof( Event));
            if ( ev)
               list_add( var-> evQueue, ( Handle) memcpy( ev, event, sizeof( Event)));
         }
      }
   } else if (( var-> stage < csFinalizing) && ( event-> cmd & ctNoInhibit))
      goto ForceProcess;
   return ret;
}


Bool
Component_can_event( Handle self)
{
   return var-> stage == csNormal;
}

void
Component_clear_event( Handle self)
{
   my-> set_eventFlag( self, 0);
}

void
Component_push_event( Handle self)
{
   if ( var-> stage == csDead)
      return;
   if ( var-> evPtr == var-> evLimit) {
      char * newStack = allocs( 16 + var-> evLimit);
      if ( !newStack) croak("Not enough memory");
      if ( var-> evStack) {
         memcpy( newStack, var-> evStack, var-> evLimit);
         free( var-> evStack);
      }
      var-> evStack = newStack;
      var-> evLimit += 16;
   }
   var-> evStack[ var-> evPtr++] = 1;
}

Bool
Component_pop_event( Handle self)
{
   if ( var-> stage == csDead)
      return false;
   if ( !var-> evStack || var-> evPtr <= 0) {
      warn("Component::pop_event call not within message()");
      return false;
   }
   return var-> evStack[ --var-> evPtr];
}


Bool
Component_eventFlag( Handle self, Bool set, Bool eventFlag)
{
   if ( var-> stage == csDead)
      return false;
   if ( !var-> evStack || var-> evPtr <= 0) {
      warn("Component::eventFlag call not within message()");
      return false;
   }
   if ( set)
      var-> evStack[ var-> evPtr - 1] = eventFlag;
   return set ? eventFlag : var-> evStack[ var-> evPtr - 1];
}

void
Component_event_error( Handle self)
{
   apc_beep( mbWarning);
}

SV *
Component_get_handle( Handle self)
{
   return newSVsv( nilSV);
}

static Bool
oversend( PEvent event, Handle self)
{
   my-> message( self, event);
   free( event);
   return false;
}

void
Component_handle_event( Handle self, PEvent event)
{
   switch ( event-> cmd)
   {
   case cmCreate:
      my-> notify( self, "<s", "Create");
      if ( var-> stage == csNormal && var-> evQueue) {
         PList q = var-> evQueue;
         var-> evQueue = nil;
         if ( q-> count > 0)
            list_first_that( q, (void*)oversend, ( void*) self);
         list_destroy( q);
         free( q);
      }
      break;
   case cmDestroy:
      opt_set( optcmDestroy);
      my-> notify( self, "<s", "Destroy");
      opt_clear( optcmDestroy);
      break;
   case cmPost:
      {
         PPostMsg p = ( PPostMsg) event-> gen. p;
         list_delete( var-> postList, ( Handle) p);
         my-> notify( self, "<sSS", "PostMessage", p-> info1, p-> info2);
         if ( p-> info1) sv_free( p-> info1);
         if ( p-> info2) sv_free( p-> info2);
         free( p);
      }
      break;
   case cmChangeOwner:
      my-> notify( self, "<sH", "ChangeOwner", event-> gen. H);
      break;
   case cmChildEnter:
      my-> notify( self, "<sH", "ChildEnter", event-> gen. H);
      break;
   case cmChildLeave:
      my-> notify( self, "<sH", "ChildLeave", event-> gen. H);
      break;
   }
}

int
Component_is_owner( Handle self, Handle objectHandle)
{
   int depth = 1;
   if ( !objectHandle || !kind_of( objectHandle, CComponent)) 
      return 0;
   if ( objectHandle == self) return -1;
   while ( PComponent(objectHandle)-> owner) {
      if ( PComponent(objectHandle)-> owner == self)
	 return depth;
      objectHandle = PComponent(objectHandle)-> owner;
      depth++;
   }
   return 0;
}

Bool
Component_migrate( Handle self, Handle attachTo)
{
    PComponent detachFrom = PComponent( var-> owner ? var-> owner : application);
    PComponent attachTo_  = PComponent( attachTo  ? attachTo  : application);

    if ( detachFrom != attachTo_) {
       if ( attachTo_)
          attachTo_ -> self-> attach(( Handle) attachTo_, self);
       if ( detachFrom)
          detachFrom-> self-> detach(( Handle) detachFrom, self, false);
    }

    return detachFrom != attachTo_;
}

void
Component_recreate( Handle self)
{
   HV * profile = newHV();
   pset_H( owner, var-> owner);
   my-> update_sys_handle( self, profile);
   sv_free(( SV *) profile);
}

Handle
Component_first_that_component( Handle self, void * actionProc, void * params)
{
   Handle child = nilHandle;
   int i, count;
   Handle * list = nil;

   if ( actionProc == nil || var-> components == nil)
      return nilHandle;
   count = var-> components-> count;
   if ( count == 0) return nilHandle;
   if ( !( list = allocn( Handle, count))) return nilHandle;
   memcpy( list, var-> components-> items, sizeof( Handle) * count);

   for ( i = 0; i < count; i++)
   {
      if ((( PActionProc) actionProc)( self, list[ i], params))
      {
         child = list[ i];
         break;
      }
   }
   free( list);
   return child;
}

void
Component_post_message( Handle self, SV * info1, SV * info2)
{
   PPostMsg p;
   Event ev = { cmPost};
   if ( var-> stage > csNormal) return;
   if (!( p = alloc1( PostMsg))) return;
   p-> info1  = newSVsv( info1);
   p-> info2  = newSVsv( info2);
   p-> h      = self;
   if ( var-> postList == nil)
      list_create( var-> postList = ( List*) malloc( sizeof( List)), 8, 8);
   list_add( var-> postList, ( Handle) p);
   ev. gen. p = p;
   ev. gen. source = ev. gen. H = self;
   apc_message( application, &ev, true);
}


void
Component_update_sys_handle( Handle self, HV * profile)
{
}

Bool
Component_validate_owner( Handle self, Handle * owner, HV * profile)
{
   dPROFILE;
   *owner = pget_H( owner);

   if ( *owner != nilHandle) {
      Handle x = *owner;
      
      if (((( PObject) x)-> stage > csNormal) || !kind_of( x, CComponent))
         return false;

      while ( x) {
         if ( x == self)
            return false;
         x = PComponent( x)-> owner;
      }
   }

   return true;
}

void
Component_on_create( Handle self)
{
}

void
Component_on_destroy( Handle self)
{
}

void
Component_on_postmessage( Handle self, SV * info1, SV * info2)
{
}

XS( Component_event_hook_FROMPERL)
{
   dXSARGS;
   SV *hook;

   if ( items == 0) {
   GET_CASE:
      if ( eventHook)
         XPUSHs( sv_2mortal( newSVsv(( SV *) eventHook)));
      else
         XPUSHs( &PL_sv_undef);
      PUTBACK;
      return;
   }

   hook = ST(0);
   /* shift unless ref $_[0] */
   if ( SvPOK(hook) && !SvROK(hook)) {
      if ( items == 1) goto GET_CASE;
      hook = ST(1);
   }

   if ( SvTYPE(hook) == SVt_NULL) {
      if ( eventHook) sv_free( eventHook);
      eventHook = nil;
      PUTBACK;
      return;
   }

   if ( !SvROK( hook) || ( SvTYPE( SvRV( hook)) != SVt_PVCV)) {
      warn("Not a CODE reference passed to Prima::Component::event_hook");
      PUTBACK;
      return; 
   }
      
   if ( eventHook) sv_free( eventHook);
   eventHook = newSVsv( hook);
   PUTBACK;
   return;
}

XS( Component_notify_FROMPERL)
{
   dXSARGS;
   Handle   self;
   SV     * res;
   HV     * hv;
   char   * name, * s;
   int      nameLen, rnt, i, ret = -1, evPtr;
   SV    ** argsv;
   int      argsc = items - 1;
   char     buf[ 1024];
   SV     * privMethod;
   Handle * sequence;
   int      seqCount = 0, stage = csNormal;
   PList    list = nil;

   if ( items < 2)
      croak ("Invalid usage of Component.notify");
   SP -= items;
   self    = gimme_the_mate( ST( 0));
   name    = ( char*) SvPV_nolen( ST( 1));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Component.notify");

   if ( eventHook) {
      dSP;
      dPUB_ARGS;
      dG_EVAL_ARGS;
      
      int flag;
      ENTER;
      SAVETMPS;
      PUSHMARK( sp);
      EXTEND( sp, items);
      for ( i = 0; i < items; i++) PUSHs( ST( i));
      PUTBACK;
      OPEN_G_EVAL;
      perl_call_sv( eventHook, G_SCALAR | G_EVAL);
      SPAGAIN;
      if ( SvTRUE( GvSV( PL_errgv))) {
         (void)POPs;
         PUB_CHECK;
         CLOSE_G_EVAL;
         croak( "%s", SvPV_nolen( GvSV( PL_errgv)));
      } 
      CLOSE_G_EVAL;
      SPAGAIN;
      flag = POPi;
      FREETMPS;
      LEAVE;
      SPAGAIN;
      if ( !flag) XSRETURN_IV(0);
   }

   if ( var-> stage != csNormal) {
      if ( !is_opt( optcmDestroy)) XSRETURN_IV(1);
      opt_clear( optcmDestroy);
      stage = var-> stage;
   }

   res = my-> notification_types( self);
   hv = ( HV *) SvRV( res);
   SPAGAIN;

   /* fetching notification type */
   nameLen = strlen( name);
   if ( hv_exists( hv, name, nameLen)) {
      SV ** holder = hv_fetch( hv, name, nameLen, 0);
      if ( !holder || !SvOK(*holder) || SvTYPE(*holder) == SVt_NULL)
         croak("Inconsistent storage in %s::notification_types for %s during Component.notify", var-> self-> className, name);
      rnt = SvIV( *holder);
   } else {
      warn("Unknown notification:%s", name);
      rnt = ntDefault;
   }
   sv_free( res);
   SPAGAIN;

   /* searching private on_xxx method */
   strncat( strcpy( buf, "on_"), name, 1020);
   for ( s = buf; *s; s++) *s = tolower(*s);
   privMethod = ( SV *) query_method( self, buf, 0);
   if ( privMethod) {
      privMethod = newRV( privMethod);
      seqCount++;
   }
   SPAGAIN;

   /* searching dynamic onXxxx subs */
   if ( var-> eventIDs) {
      void * ret;
      ret = hash_fetch( var-> eventIDs, name, nameLen);
      if ( ret != nil) {
         list = var-> events + PTR2UV( ret) - 1;
         seqCount += list-> count;
      }
   }

   if ( seqCount == 0) XSRETURN_IV(1);

   /* filling calling sequence */
   sequence = ( Handle *) malloc( seqCount * 2 * sizeof( void *));
   if ( !sequence) XSRETURN_IV(1);

   i = 0;
   if ( privMethod && (( rnt & ntCustomFirst) == 0)) {
      sequence[ i++] = self;
      sequence[ i++] = ( Handle) privMethod;
   }
   if ( list) {
      int j;
      if ( rnt & ntFluxReverse) {
         for ( j = list-> count - 1; j > 0; j -= 2) {
            sequence[ i++] = list-> items[ j - 1];
            sequence[ i++] = list-> items[ j];
         }
      } else
         memcpy( sequence + i, list-> items, list-> count * sizeof( Handle));
   }
   if ( privMethod && ( rnt & ntCustomFirst)) {
      sequence[ i++] = self;
      sequence[ i++] = ( Handle) privMethod;
   }

   /* copying arguments passed from perl */
   argsv = ( SV **) malloc( argsc * sizeof( SV *));
   if ( !argsv) {
      free( sequence);
      XSRETURN_IV(1);
   }

   for ( i = 0; i < argsc; i++) argsv[ i] = ST( i + 1);
   argsv[ 0] = ST( 0);

   /* entering event */
   my-> push_event( self);
   SPAGAIN;

   /* cycling subs */
   rnt &= ntMultiple | ntEvent;
   evPtr = var-> evPtr;
   for ( i = 0; i < seqCount; i += 2) {
      dSP;
      dPUB_ARGS;
      dG_EVAL_ARGS;
      int j;

      ENTER;
      SAVETMPS;
      PUSHMARK( sp);
      EXTEND( sp, argsc + (( sequence[ i] == self) ? 0 : 1));
      if ( sequence[ i] != self)
         PUSHs((( PAnyObject)( sequence[i]))-> mate);
      for ( j = 0; j < argsc; j++) PUSHs( argsv[ j]);
      PUTBACK;
      OPEN_G_EVAL;
      perl_call_sv(( SV*) sequence[ i + 1], G_DISCARD | G_EVAL);
      if ( SvTRUE( GvSV( PL_errgv))) {
         PUB_CHECK;
         CLOSE_G_EVAL;
         if ( privMethod) sv_free( privMethod);
         free( argsv);
         free( sequence);
         croak( "%s", SvPV_nolen( GvSV( PL_errgv)));
      } 
      CLOSE_G_EVAL;
      SPAGAIN;
      FREETMPS;
      LEAVE;
      if (( var-> stage != stage) ||
          ( var-> evPtr != evPtr) ||
          ( rnt == ntSingle) ||
         (( rnt == ntEvent) && ( var-> evStack[ var-> evPtr - 1] == 0))
         ) break;
   }
   SPAGAIN;

   /* leaving */
   if ( privMethod) sv_free( privMethod);
   if ( var-> stage < csDead) ret = my-> pop_event( self);
   SPAGAIN;
   SP -= items;
   XPUSHs( sv_2mortal( newSViv( ret)));
   free( argsv);
   free( sequence);
   PUTBACK;
}

Bool
Component_notify( Handle self, char * format, ...)
{
   Bool r;
   SV * ret;
   va_list args;
   va_start( args, format);
   ENTER;
   SAVETMPS;
   ret = call_perl_indirect( self, "notify", format, true, false, args);
   va_end( args);
   r = ( ret && SvIOK( ret)) ? SvIV( ret) : 0;
   if ( ret) my-> set_eventFlag( self, r);
   FREETMPS;
   LEAVE;
   return r;
}

Bool
Component_notify_REDEFINED( Handle self, char * format, ...)
{
   Bool r;
   SV * ret;
   va_list args;
   va_start( args, format);
   ENTER;
   SAVETMPS;
   ret = call_perl_indirect( self, "notify", format, true, false, args);
   va_end( args);
   r = ( ret && SvIOK( ret)) ? SvIV( ret) : 0;
   if ( ret) my-> set_eventFlag( self, r);
   FREETMPS;
   LEAVE;
   return r;
}

XS( Component_get_components_FROMPERL)
{
   dXSARGS;
   Handle self;
   Handle * list;
   int i, count;

   if ( items != 1)
      croak ("Invalid usage of Component.get_components");
   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Component.get_components");
   if ( var-> components) {
      count = var-> components-> count;
      list  = var-> components-> items;
      EXTEND( sp, count);
      for ( i = 0; i < count; i++) 
         PUSHs( sv_2mortal( newSVsv((( PAnyObject) list[ i])-> mate)));
   }
   PUTBACK;
   return;
}

void Component_get_components          ( Handle self) { warn("Invalid call of Component::get_components"); }
void Component_get_components_REDEFINED( Handle self) { warn("Invalid call of Component::get_components"); }

UV
Component_add_notification( Handle self, char * name, SV * subroutine, Handle referer, int index)
{
   UV     ret;
   PList  list;
   int    nameLen = strlen( name);
   SV   * res;

   res = my-> notification_types( self);
   if ( !hv_exists(( HV *) SvRV( res), name, nameLen)) {
       sv_free( res);
       warn("No such event %s", name);
       return 0;
   }
   sv_free( res);

   if ( !subroutine || !SvROK( subroutine) || ( SvTYPE( SvRV( subroutine)) != SVt_PVCV)) {
      warn("Not a CODE reference passed to %s to Component::add_notification", name);
      return 0;
   }

   if ( referer == nilHandle) referer = self;

   if ( var-> eventIDs == nil) {
      var-> eventIDs = hash_create();
      ret = 0;
   } else
      ret = PTR2UV(hash_fetch( var-> eventIDs, name, nameLen));

   if ( ret == 0) {
      hash_store( var-> eventIDs, name, nameLen, INT2PTR(void*, var-> eventIDCount + 1));
      if ( var-> events == nil)
         var-> events = ( List*) malloc( sizeof( List));
      else {
         void * cf = realloc( var-> events, ( var-> eventIDCount + 1) * sizeof( List));
         if ( cf == nil) free( var-> events);
         var-> events = ( List*) cf;
      }
      if ( var-> events == nil) croak("No enough memory");
      list = var-> events + var-> eventIDCount++;
      list_create( list, 2, 2);
   } else
      list = var-> events +  PTR2UV( ret) - 1;

   ret   = PTR2UV(newSVsv( subroutine));
   index = list_insert_at( list, referer, index);
   list_insert_at( list, ( Handle) ret, index + 1);

   if ( referer != self) {
      if ( PComponent( referer)-> refs == nil)
         PComponent( referer)-> refs = plist_create( 2, 2);
      else
         if ( list_index_of( PComponent( referer)-> refs, self) >= 0) goto NO_ADDREF;
      list_add( PComponent( referer)-> refs, self);
   NO_ADDREF:;
      if ( var-> refs == nil)
         var-> refs = plist_create( 2, 2);
      else
         if ( list_index_of( var-> refs, referer) >= 0) goto NO_SELFREF;
      list_add( var-> refs, referer);
   NO_SELFREF:;
   }
   return ret;
}

void
Component_remove_notification( Handle self, UV id)
{
   int i = var-> eventIDCount;
   PList  list = var-> events;

   if ( list == nil) return;

   while ( i--) {
      int j;
      for ( j = 0; j < list-> count; j += 2) {
         if ((( UV ) list-> items[ j + 1]) != id) continue;
         sv_free(( SV *) list-> items[ j + 1]);
         list_delete_at( list, j + 1);
         list_delete_at( list, j);
         return;
      }
      list++;
   }
}

void
Component_unlink_notifier( Handle self, Handle referer)
{
   int i = var-> eventIDCount;
   PList  list = var-> events;

   if ( list == nil) return;

   while ( i--) {
      int j;
   AGAIN:
      for ( j = 0; j < list-> count; j += 2) {
         if ((( long) list-> items[ j]) != referer) continue;
         sv_free(( SV *) list-> items[ j + 1]);
         list_delete_at( list, j + 1);
         list_delete_at( list, j);
         goto AGAIN;
      }
      list++;
   }
}

XS( Component_get_notification_FROMPERL)
{
   dXSARGS;
   Handle self;
   char * event;
   void * ret;
   PList  list;

   if ( items < 2)
      croak ("Invalid usage of Component.get_notification");

   SP -= items;
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Component.get_notification");

   if ( var-> eventIDs == nil) XSRETURN_EMPTY;
   event = ( char *) SvPV_nolen( ST( 1));
   ret = hash_fetch( var-> eventIDs, event, strlen( event));
   if ( ret == nil) XSRETURN_EMPTY;
   list = var-> events + PTR2UV( ret) - 1;

   if ( items < 3) {
      int i;
      if ( GIMME_V == G_ARRAY) {
         int count = (int)( list-> count * 1.5);
         EXTEND( sp, count);
         for ( i = 0; i < list-> count; i += 2) {
            PUSHs( sv_2mortal( newSVsv((( PAnyObject)( list-> items[i]))-> mate)));
            PUSHs( sv_2mortal( newSVsv(( SV *) list->items[i + 1])));
            PUSHs( sv_2mortal( newSViv(( long) list->items[i + 1])));
         }
      } else
         XPUSHs( sv_2mortal( newSViv( list-> count / 2)));
      PUTBACK;
   } else {
      int index = SvIV( ST( 2));
      int count = list-> count / 2;
      if ( index >= count || index < -count) XSRETURN_EMPTY;
      if ( index < 0) index = count + index;
      EXTEND( sp, 3);
      PUSHs( sv_2mortal( newSVsv((( PAnyObject) list->items[index * 2])-> mate)));
      PUSHs( sv_2mortal( newSVsv(( SV *) list->items[index * 2 + 1])));
      PUSHs( sv_2mortal( newSViv(( long) list->items[index * 2 + 1])));
      PUTBACK;
   }
}

void Component_get_notification          ( Handle self, char * name, int index) { warn("Invalid call of Component::get_notification"); }
void Component_get_notification_REDEFINED( Handle self, char * name, int index) { warn("Invalid call of Component::get_notification"); }

XS( Component_set_notification_FROMPERL)
{
   dXSARGS;
   GV * gv;
   SV * sub;
   char * name, * convname;
   Handle self;

   if ( items < 1)
      croak ("Invalid usage of Component::notification property");
   self = gimme_the_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Component::notification property");
   if ( CvANON( cv) || !( gv = CvGV( cv))) croak("Cannot be called as anonymous sub");

   sub = sv_newmortal();
   gv_efullname3( sub, gv, nil);
   name = SvPVX( sub);
   if ( items < 2)
      croak( "Attempt to read write-only property %s", name);
   convname = name;
   while ( *(name++)) {
      if ( *name == ':') convname = name + 1;
   }

   sub = ST( 1);
   if ( convname[0] == 'o' && convname[1] == 'n')
      my-> add_notification( self, convname + 2, sub, self, -1);
   XSRETURN_EMPTY;
}

void Component_set_notification          ( Handle self, char * name, SV * subroutine) { warn("Invalid call of Component::set_notification"); }
void Component_set_notification_REDEFINED( Handle self, char * name, SV * subroutine) { warn("Invalid call of Component::set_notification"); }

SV *
Component_delegations( Handle self, Bool set, SV * delegations)
{
   if ( set) {
      int i, len;
      AV * av;
      Handle referer;
      char *name;

      if ( var-> stage > csNormal) return nilSV;
      if ( !SvROK( delegations) || SvTYPE( SvRV( delegations)) != SVt_PVAV) return nilSV;

      referer = var-> owner;
      name    = var-> name;
      av = ( AV *) SvRV( delegations);
      len = av_len( av);
      for ( i = 0; i <= len; i++) {
         SV **holder = av_fetch( av, i, 0);
         if ( !holder) continue;
         if ( SvROK( *holder)) {
            Handle mate = gimme_the_mate( *holder);
            if (( mate == nilHandle) || !kind_of( mate, CComponent)) continue;
            referer = mate;
         } else if ( SvPOK( *holder)) {
            CV * sub;
            SV * subref;
            char buf[ 1024];
            char * event = SvPV_nolen( *holder);

	    if ( referer == nilHandle)
	       croak("Event delegations for objects without owners must be provided with explicit referer");
            snprintf( buf, 1023, "%s_%s", name, event);
            sub = query_method( referer, buf, 0);
            if ( sub == nil) continue;
            my-> add_notification( self, event, subref = newRV(( SV*) sub), referer, -1);
            sv_free( subref);
         }
      }
   } else {
      HE * he;
      AV * av = newAV();
      Handle last = nilHandle;
      if ( var-> stage > csNormal || var-> eventIDs == nil)
         return newRV_noinc(( SV*) av);

      hv_iterinit( var-> eventIDs);
      while (( he = hv_iternext( var-> eventIDs)) != nil) {
         int i;
         char * event = ( char *) HeKEY( he);
         PList list = var-> events + PTR2UV( HeVAL( he)) - 1;
         for ( i = 0; i < list-> count; i += 2) {
            if ( list-> items[i] != last) {
               last = list-> items[i];
               av_push( av, newSVsv((( PAnyObject) last)-> mate));
            }
            av_push( av, newSVpv( event, 0));
         }
      }
      return newRV_noinc(( SV*) av);
   }
   return nilSV;
}

#ifdef __cplusplus
}
#endif
