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

#include <ctype.h>
#include "apricot.h"
#include "Component.h"
#include <Component.inc>

#undef  my
#define inherited CObject->
#define my  ((( PComponent) self)-> self)
#define var (( PComponent) self)

typedef Bool ActionProc ( Handle self, Handle item, void * params);
typedef ActionProc *PActionProc;

void
Component_init( Handle self, HV * profile)
{
   Handle owner = pget_H( owner);
   SV * res;
   HV * hv;
   HE * he;
   if (( owner != nilHandle) && (((( PObject) owner)-> stage > csNormal) || !kind_of( owner, CComponent)))
      croak( "Illegal object reference passed to Component.init");
   inherited init( self, profile);
   var-> owner = owner;
   my-> set_name( self, pget_c( name));
   my-> set_delegations( self, pget_sv( delegations));
   var-> evQueue = malloc( sizeof( List));
   list_create( var-> evQueue,  8, 8);
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
}

static Bool bring_by_name( Handle self, PComponent item, char * name)
{
   return strcmp( name, item-> name) == 0;
}

Handle
Component_bring( Handle self, char * componentName)
{
   return my-> first_that_component( self, bring_by_name, componentName);
}

void
Component_cleanup( Handle self)
{
   Event ev = {cmDestroy};
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

static Bool
detach_all( Handle child, Handle self)
{
   my-> detach( self, child, true);
   return false;
}

/* #ifdef PARANOID_MALLOC */
/* #include "Image.h" */
/* #endif */

void
Component_done( Handle self)
{
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
      list_first_that( var-> refs, free_eventref, pself);
      plist_destroy( var-> refs);
      var-> refs = nil;
   }

   if ( var-> postList != nil) {
      list_first_that( var-> postList, free_private_posts, nil);
      list_destroy( var-> postList);
      free( var-> postList);
      var-> postList = nil;
   }
   if ( var-> evQueue != nil)
   {
      list_first_that( var-> evQueue, free_queue, nil);
      list_destroy( var-> evQueue);
      free( var-> evQueue);
      var-> evQueue = nil;
   }
   if ( var-> components != nil) {
      list_first_that( var-> components, detach_all, ( void*) self);
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
      if ( var-> components == nil) {
         var-> components = malloc( sizeof( List));
         list_create( var-> components, 8, 8);
      } else
         if ( list_index_of( var-> components, object) > 0) {
            warn( "RTC0040: Object attach failed");
            return;
         }
      list_add( var-> components, object);
      SvREFCNT_inc( SvRV(( PObject( object))-> mate));
   } else
       warn( "RTC0040: Object attach failed");
}

void
Component_detach( Handle self, Handle object, Bool kill)
{
   if ( object && ( var-> components != nil)) {
      int index = list_index_of( var-> components, object);
      if ( index >= 0) {
         list_delete_at( var-> components, index);
         SvREFCNT_dec( SvRV(( PObject( object))-> mate));
         if ( kill) Object_destroy( object);
      }
   }
}

char *
Component_get_name( Handle self)
{
   return var-> name ? var-> name : "";
}

Handle
Component_get_owner( Handle self)
{
  return var-> owner;
}

void
Component_set( Handle self, HV * profile)
{
   /* this can eliminate unwilling items */
   /* from HV before indirect Object::set */
   my-> update_sys_handle( self, profile);

   if ( pexist( owner))
   {
      Handle theOwner;
      var-> owner = pget_H( owner);
      if (( var-> owner != nilHandle) && !kind_of( var-> owner, CComponent))
         croak( "RTC0047: Illegal object reference passed to Component::set_owner");
      if ( var-> owner == nilHandle) var-> owner = application;
      theOwner = var-> owner;

      while ( theOwner) {
         if ( theOwner == self)
            croak( "RTC0048: Invalid owner reference passed to Component::set_owner");
         theOwner = PComponent( theOwner)-> owner;
      }

      pdelete( owner);                    /* like this. */
   }
   inherited set ( self, profile);
}

void
Component_set_name( Handle self, char * name)
{
   if ( var-> stage > csNormal) return;
   free( var-> name);
   var-> name = malloc( strlen ( name) + 1);
   strcpy( var-> name, name);
   apc_component_fullname_changed_notify( self);
}

static Bool
find_dup_msg( PEvent event, int cmd)
{
   return event-> cmd == cmd;
}

Bool
Component_message( Handle self, PEvent event)
{
   Bool ret      = false;
   if ( var-> stage == csNormal) {
ForceProcess:
      protect_object( self);
      my-> push_event( self);
      my-> handle_event( self, event);
      ret = my-> pop_event( self);
      if ( !ret) event-> cmd = 0;
      unprotect_object( self);
   } else if ( var-> stage == csConstructing) {
      if ( var-> evQueue == nil)
          croak("RTC0041: Object set twice to constructing stage");
      switch ( event-> cmd & ctQueueMask) {
      case ctDiscardable:
         break;
      case ctPassThrough:
         goto ForceProcess;
      case ctSingle:
         event-> cmd = ( event-> cmd & ~ctQueueMask) | ctSingleResponse;
         if ( list_first_that( var-> evQueue, find_dup_msg, (void*) event-> cmd) >= 0)
	      break;
      default:
	      list_add( var-> evQueue, ( Handle) memcpy( malloc( sizeof( Event)),
				event, sizeof( Event)));
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
   my-> set_event_flag( self, 0);
}

void
Component_push_event( Handle self)
{
   if ( var-> stage == csDead)
      return;
   if ( var-> evPtr == var-> evLimit) {
      char * newStack = malloc( 16 + var-> evLimit);
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
      warn("RTC0042: Component::pop_event call not within message()");
      return false;
   }
   return var-> evStack[ --var-> evPtr];
}

void
Component_set_event_flag( Handle self, Bool eventFlag)
{
   if ( var-> stage == csDead)
      return;
   if ( !var-> evStack || var-> evPtr <= 0) {
      warn("RTC0043: Component::eventFlag call not within message()");
      return;
   }
   var-> evStack[ var-> evPtr - 1] = eventFlag;
}

Bool
Component_get_event_flag( Handle self)
{
   if ( var-> stage == csDead)
      return false;
   if ( !var-> evStack || var-> evPtr <= 0) {
      warn("RTC0044: Component::eventFlag call not within message()");
      return false;
   }
   return var-> evStack[ var-> evPtr - 1];
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
      if ( var-> stage == csNormal)
      {
         if ( var-> evQueue-> count > 0)
            list_first_that( var-> evQueue, oversend, ( void*) self);
         list_destroy( var-> evQueue);
         free( var-> evQueue);
         var-> evQueue = nil;
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
   }
}

Bool
Component_migrate( Handle self, Handle attachTo)
{
    PComponent detachFrom = PComponent( var-> owner ? var-> owner : application);
    PComponent attachTo_  = PComponent( attachTo  ? attachTo  : application);
    Handle     theOwner   = ( Handle) attachTo_;

    if ( !theOwner || attachTo_-> stage > csNormal || !kind_of( theOwner, CComponent))
       croak( "RTC0049: Invalid owner reference passed to Component::set_owner");

    while ( theOwner) {
       if ( theOwner == self)
          croak( "RTC0049: Invalid owner reference passed to Component::set_owner");
       theOwner = PComponent( theOwner)-> owner;
    }

    if ( detachFrom != attachTo_) {
       attachTo_ -> self-> attach(( Handle) attachTo_, self);
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
   list = malloc( sizeof( Handle) * count);
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
   p = malloc( sizeof( PostMsg));
   p-> info1  = newSVsv( info1);
   p-> info2  = newSVsv( info2);
   p-> h      = self;
   if ( var-> postList == nil)
      list_create( var-> postList = malloc( sizeof( List)), 8, 8);
   list_add( var-> postList, ( Handle) p);
   ev. gen. p = p;
   ev. gen. source = ev. gen. H = self;
   apc_message( application, &ev, true);
}


void
Component_update_sys_handle( Handle self, HV * profile)
{
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
   int      seqCount = 0;
   PList    list = nil;

   if ( items < 2)
      croak ("Invalid usage of Component.notify");
   SP -= items;
   self    = gimme_the_mate( ST( 0));
   name    = ( char*) SvPV( ST( 1), na);
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Component.notify");

   if ( var-> stage != csNormal) {
      if ( !is_opt( optcmDestroy)) XSRETURN_IV(1);
      opt_clear( optcmDestroy);
   }

   res = my-> notification_types( self);
   hv = ( HV *) SvRV( res);
   SPAGAIN;

   /* fetching notification type */
   nameLen = strlen( name);
   if ( hv_exists( hv, name, nameLen)) {
      SV ** holder = hv_fetch( hv, name, nameLen, 0);
      if ( !holder || !SvOK(*holder) || SvTYPE(*holder) == SVt_NULL)
         croak("RTC0045: Inconsistent storage in %s::notification_types for %s during Component.notify", var-> self-> className, name);
      rnt = SvIV( *holder);
   } else {
      warn("Unknown notification:%s", name);
      rnt = ntDefault;
   }
   sv_free( res);
   SPAGAIN;

   /* searching private on_xxx method */
   strncat( strcpy( buf, "on_"), name, 1023);
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
         list = var-> events + ( int) ret - 1;
         seqCount += list-> count;
      }
   }

   if ( seqCount == 0) XSRETURN_IV(1);

   /* filling calling sequence */
   sequence = ( Handle *) malloc( seqCount * 2 * sizeof( void *));
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
   argsv = malloc( argsc * sizeof( SV *));
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
      int j;
      SV * errSave;

      ENTER;
      SAVETMPS;
      PUSHMARK( sp);
      EXTEND( sp, argsc + (( sequence[ i] == self) ? 0 : 1));
      if ( sequence[ i] != self)
         PUSHs((( PAnyObject)( sequence[i]))-> mate);
      for ( j = 0; j < argsc; j++) PUSHs( argsv[ j]);
      PUTBACK;
      errSave = SvTRUE( GvSV( errgv)) ? newSVsv( GvSV( errgv)) : nil;
      perl_call_sv(( SV*) sequence[ i + 1], G_DISCARD | G_EVAL);
      if ( SvTRUE( GvSV( errgv))) {
         PUB_CHECK;
         if ( errSave) {
            sv_catsv( GvSV( errgv), errSave);
            sv_free( errSave);
         }
         croak( SvPV( GvSV( errgv), na));
      } else if ( errSave) {
         sv_setsv( GvSV( errgv), errSave);
         sv_free( errSave);
      }


      SPAGAIN;
      FREETMPS;
      LEAVE;
      if (( var-> stage != csNormal) ||
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
   free( sequence);
   free( argsv);
   PUTBACK;
}

Bool
Component_notify( Handle self, char * format, ...)
{
   Bool r;
   SV * ret;
   va_list args;
   va_start( args, format);
   ret = call_perl_indirect( self, "notify", format, true, false, args);
   va_end( args);
   r = ( ret && SvIOK( ret)) ? SvIV( ret) : 0;
   if ( ret) my-> set_event_flag( self, r);
   return r;
}

Bool
Component_notify_REDEFINED( Handle self, char * format, ...)
{
   Bool r;
   SV * ret;
   va_list args;
   va_start( args, format);
   ret = call_perl_indirect( self, "notify", format, true, false, args);
   va_end( args);
   r = ( ret && SvIOK( ret)) ? SvIV( ret) : 0;
   if ( ret) my-> set_event_flag( self, r);
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
      PUTBACK;
   }
   return;
}

void Component_get_components          ( Handle self) { warn("Invalid call of Component::get_components"); }
void Component_get_components_REDEFINED( Handle self) { warn("Invalid call of Component::get_components"); }

long
Component_add_notification( Handle self, char * name, SV * subroutine, Handle referer, int index)
{
   void * ret;
   PList  list;
   int    nameLen = strlen( name);

   if ( !hv_exists(( HV *) SvRV( my-> notification_types( self)), name, nameLen)) {
       warn("RTC04B: No such event %s", name);
       return 0;
   }

   if ( !subroutine || !SvROK( subroutine) || ( SvTYPE( SvRV( subroutine)) != SVt_PVCV)) {
      warn("RTC04C: Not a CODE reference passed to %s to Component::add_notification", name);
      return 0;
   }

   if ( referer == nilHandle) referer = self;

   if ( var-> eventIDs == nil) {
      var-> eventIDs = hash_create();
      ret = nil;
   } else
      ret = hash_fetch( var-> eventIDs, name, nameLen);

   if ( ret == nil) {
      hash_store( var-> eventIDs, name, nameLen, ( void*)( var-> eventIDCount + 1));
      if ( var-> events == nil)
         var-> events = malloc( sizeof( List));
      else {
         void * cf = realloc( var-> events, ( var-> eventIDCount + 1) * sizeof( List));
         if ( cf == nil) free( var-> events);
         var-> events = cf;
      }
      if ( var-> events == nil) croak("No enough memory");
      list = var-> events + var-> eventIDCount++;
      list_create( list, 2, 2);
   } else
      list = var-> events + ( int) ret - 1;

   ret = ( void *) newSVsv( subroutine);
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
   return ( long) ret;
}

void
Component_remove_notification( Handle self, long id)
{
   int i = var-> eventIDCount;
   PList  list = var-> events;

   if ( list == nil) return;

   while ( i--) {
      int j;
      for ( j = 0; j < list-> count; j += 2) {
         if ((( long) list-> items[ j + 1]) != id) continue;
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
   event = ( char *) SvPV( ST( 1), na);
   ret = hash_fetch( var-> eventIDs, event, strlen( event));
   if ( ret == nil) XSRETURN_EMPTY;
   list = var-> events + ( int) ret - 1;

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
Component_get_delegations( Handle self)
{
   HE * he;
   AV * av = newAV();
   Handle last = nilHandle;
   if ( var-> stage > csNormal || var-> eventIDs == nil) newRV_noinc(( SV*) av);

   hv_iterinit( var-> eventIDs);
   while (( he = hv_iternext( var-> eventIDs)) != nil) {
      int i;
      char * event = ( char *) HeKEY( he);
      PList list = var-> events + ( int) HeVAL( he) - 1;
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

void
Component_set_delegations( Handle self, SV * delegations)
{
   int i, len;
   AV * av;
   Handle referer;
   char *name;

   if ( var-> stage > csNormal) return;
   if ( !var-> owner) return;
   if ( !SvROK( delegations) || SvTYPE( SvRV( delegations)) != SVt_PVAV) return;

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
         char * event = SvPV( *holder, na);
         snprintf( buf, 1023, "%s_%s", name, event);
         sub = query_method( referer, buf, 0);
         if ( sub == nil) continue;
         my-> add_notification( self, event, subref = newRV(( SV*) sub), referer, -1);
         sv_free( subref);
      }
   }
}
