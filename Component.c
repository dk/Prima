#include <ctype.h>
#include "apricot.h"
#include "Component.h"
#include "Component.inc"

#undef  my
#define inherited CObject->
#define my  ((( PComponent) self)-> self)->
#define var (( PComponent) self)->

typedef Bool ActionProc ( Handle self, Handle item, void * params);
typedef ActionProc *PActionProc;

static void
dyna_set( Handle self, HV * profile)
{
#define dyna( Method)				\
   Component_set_dyna_method( self,		\
			      "on" # Method,	\
			      (SV*)profile,	\
			      &var on##Method)
   dyna( Create);
   dyna( Destroy);
   dyna( PostMessage);
}

void
Component_init( Handle self, HV * profile)
{
   Handle owner = pget_H( owner);
   if (( owner != nilHandle) && !kind_of( owner, CComponent))
      croak( "Illegal object reference passed to Component.init");
   inherited init( self, profile);
   var owner = owner;
   my set_name( self, pget_c( name));
   my set_delegate_to( self, pget_H( delegateTo));
   dyna_set( self, profile);
   var evQueue = malloc( sizeof( List));
   list_create( var evQueue,  8, 8);
   apc_component_create( self);
}

void
Component_setup( Handle self)
{
   Event ev = {cmCreate};
   ev. gen. source = self;
   my message( self, &ev);
}

void
Component_cleanup( Handle self)
{
   Event ev = {cmDestroy};
   ev. gen. source = self;
   my message( self, &ev);
}

static Bool
free_private_posts( PostMsg * msg, void * dummy )
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
free_reference( Handle self, void * dummy)
{
   var delegateTo = nilHandle;
   return false;
}

/* #ifdef PARANOID_MALLOC */
/* #include "Image.h" */
/* #endif */

void
Component_done( Handle self)
{
   if ( var refList != nil) {
      list_first_that( var refList, free_reference, nil);
      list_destroy( var refList);
      free( var refList);
      var refList = nil;
   }
   if ( var postList != nil) {
      list_first_that( var postList, ( PListProc) free_private_posts, nil);
      list_destroy( var postList);
      free( var postList);
      var postList = nil;
   }
   if ( var evQueue != nil)
   {
      list_first_that( var evQueue, ( PListProc) free_queue, nil);
      list_destroy( var evQueue);
      free( var evQueue);
      var evQueue = nil;
   }

   if ( var refs) {
      char *k;
      I32 l;
      SV *obj;

      hv_iterinit( var refs);
      while (( obj = hv_iternextsv( var refs, &k, &l)) && k)
	 my detach( self, atol( k), true);
      /* sv_free(( SV*) var refs); */  /* Will be freed by perl */
      var refs = nil;
   }

   apc_component_destroy( self);
/* #ifdef PARANOID_MALLOC */
/* if ( kind_of( self, CImage)) */
/*    debug_write( "Freeing image %s\n", var name); */
/* #endif */
   free( var name);
   free( var evStack);
   inherited done( self);
}

void
Component_attach( Handle self, Handle object)
{
   char key[ 20];
   if ( object && kind_of( object, CComponent)) {
      if ( !var refs) {
	 HV *me = ( HV*) SvRV( var mate);
	 if ( SvTYPE( me) == SVt_PVHV && ( var refs = newHV()))
	    hv_store( me, "__CREFS__", 9, newRV(( SV*) var refs), 0);
	 else {
	    if ( var refs)
	       sv_free(( SV*) var refs);
	    var refs = nil;
	    croak( "RTC0040: Component attach failed");
	 }
      }
      snprintf( key, 20, "%lu", object);
      hv_store( var refs, key, strlen( key),
                newRV( SvRV( PComponent( object)-> mate)), 0);
   }
}

void
Component_detach( Handle self, Handle object, Bool kill)
{
   char key[ 20];

   if ( object && var refs) {
      snprintf( key, 20, "%lu", object);
      hv_delete( var refs, key, strlen( key), G_DISCARD);
      if ( kill)
	 Object_destroy( object);
   }
}

char *
Component_get_name( Handle self)
{
   return var name ? var name : "";
}

Handle
Component_get_owner( Handle self)
{
  return var owner;
}

Handle
Component_get_delegate_to( Handle self)
{
   return var delegateTo;
}

void
Component_set( Handle self, HV * profile)
{
   /* this can eliminate unwilling items */
   /* from HV before indirect Object::set */
   my update_sys_handle( self, profile);

   if ( pexist( owner))
   {
      Handle theOwner;
      var owner = pget_H( owner);
      if (( var owner != nilHandle) && !kind_of( var owner, CComponent))
         croak( "RTC0047: Illegal object reference passed to Component::set_owner");
      if ( var owner == nilHandle) var owner = application;
      theOwner = var owner;

      while ( theOwner) {
         if ( theOwner == self)
            croak( "RTC0048: Invalid owner reference passed to Component::set_owner");
         theOwner = PComponent( theOwner)-> owner;
      }

      pdelete( owner);                    /* like this. */
   }
   dyna_set( self, profile);
   inherited set ( self, profile);
}

void
Component_set_name( Handle self, char * name)
{
   free( var name);
   var name = malloc( strlen ( name) + 1);
   strcpy( var name, name);
   if ( var stage == csNormal)
      my update_delegator( self);
}

void
Component_set_delegate_to( Handle self, Handle delegateTo)
{
   Handle old = var delegateTo;
   if ( old == delegateTo) return;
   var delegateTo = delegateTo;
   if ( var stage == csNormal)
      my update_delegator( self);
   if ( old && (( PComponent) old)-> refList)
      list_delete((( PComponent) old)-> refList, self);
   if ( delegateTo) {
      PComponent next = ( PComponent) var owner;
      while ( next && (( Handle) next != delegateTo))
	 next = ( PComponent) (( PComponent) next)-> owner;
      if ( next == nil) {
         next = ( PComponent) delegateTo;
         if ( next-> refList == nil)
            list_create( next-> refList = malloc( sizeof( List)), 8, 8);
         list_add( next-> refList, self);
      }
   }
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
   if ( var stage == csNormal) {
ForceProcess:
      protect_object( self);
      my push_event( self);
      my handle_event( self, event);
      my pop_event( self);
      if ( var evStack[ var evPtr] == 0) event-> cmd = 0;
      ret = var evStack[ var evPtr];
      unprotect_object( self);
   } else if ( var stage == csConstructing) {
      if ( var evQueue == nil)
          croak("RTC0041: Object set twice to constructing stage");
      switch ( event-> cmd & ctQueueMask) {
      case ctDiscardable:
	 break;
      case ctPassThrough:
	 goto ForceProcess;
      case ctSingle:
	 event-> cmd = ( event-> cmd & ~ctQueueMask) | ctSingleResponse;
	 if ( list_first_that( var evQueue,
			       ( PListProc) find_dup_msg,
			       (void*) event-> cmd)
	      >= 0)
	    break;
      default:
	 list_add( var evQueue,
		   ( Handle) memcpy( malloc( sizeof( Event)),
				     event, sizeof( Event)));
      }
   } else if ( var stage > csNormal
	       && var stage < csDead
	       && ( event-> cmd & ctQueueMask) == ctPassThrough)
      goto ForceProcess;
   return ret;
}


Bool
Component_can_event( Handle self)
{
   return var stage == csNormal;
}

void
Component_clear_event( Handle self)
{
   my set_event_flag( self, 0);
}

void
Component_push_event( Handle self)
{
   if ( var evPtr == var evLimit) {
      char * newStack = malloc( 16 + var evLimit);
      if ( var evStack) {
         memcpy( newStack, var evStack, var evLimit);
         free( var evStack);
      }
      var evStack = newStack;
      var evLimit += 16;
   }
   var evStack[ var evPtr++] = 1;
}

Bool
Component_pop_event( Handle self)
{
   if ( !var evStack || var evPtr <= 0)
      croak("RTC0042: Component::pop_event call not within message()");
   return var evStack[ var evPtr--];
}

void
Component_set_event_flag( Handle self, Bool eventFlag)
{
   if ( !var evStack || var evPtr <= 0)
      croak("RTC0043: Component::eventFlag call not within message()");
   var evStack[ var evPtr - 1] = eventFlag;
}

Bool
Component_get_event_flag( Handle self)
{
   if ( !var evStack || var evPtr <= 0)
      croak("RTC0044: Component::eventFlag call not within message()");
   return var evStack[ var evPtr - 1];
}

void
Component_event_error( Handle self)
{
   apc_beep( mbWarning);
}

static Bool
oversend( PEvent event, Handle self)
{
   my message( self, event);
   free( event);
   return false;
}

void
Component_handle_event( Handle self, PEvent event)
{
#undef dyna
#define dyna( Method)					\
   if ( var on##Method)					\
      cv_call_perl( var mate, var on##Method, "")
   switch ( event-> cmd)
   {
   case cmCreate:
      my update_delegator( self);
      my on_create( self);
      if ( is_dmopt( dmCreate)) delegate_sub( self, "Create", "H", self);
      dyna( Create);
      if ( var stage == csNormal)
      {
         if ( var evQueue-> count > 0)
            list_first_that( var evQueue, ( PListProc)oversend, ( void*) self);
         list_destroy( var evQueue);
         free( var evQueue);
         var evQueue = nil;
      }
      break;
   case cmDestroy:
      my on_destroy( self);
      if ( is_dmopt( dmDestroy)) delegate_sub( self, "Destroy", "H", self);
      dyna( Destroy);
      break;
   case cmPost:
      {
         PPostMsg p = ( PPostMsg) event-> gen. p;
         my on_postmessage( self, p-> info1, p-> info2);
         if ( var stage >= csDead) return;
         if ( is_dmopt( dmPostMessage))
            delegate_sub( self, "PostMessage", "HSS", self, p-> info1, p-> info2);
         if ( var stage >= csDead) return;
         if ( var onPostMessage)
            cv_call_perl( var mate, var onPostMessage, "SS", p-> info1, p-> info2);
         if ( var stage >= csDead) return;
         if ( p-> info1) sv_free( p-> info1);
         if ( p-> info2) sv_free( p-> info2);
         free( p);
         list_delete( var postList, ( Handle) p);
      }
   break;
   }
}

Bool
Component_migrate( Handle self, Handle attachTo)
{
    PComponent detachFrom = PComponent( var owner ? var owner : application);
    PComponent attachTo_  = PComponent( attachTo  ? attachTo  : application);
    Handle     theOwner   = ( Handle) attachTo_;

    if ( !theOwner || !kind_of( theOwner, CComponent))
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
   pset_H( owner, var owner);
   my update_sys_handle( self, profile);
   sv_free(( SV *) profile);
}

Handle
Component_first_that_component( Handle self, void * actionProc, void * params)
{
   Handle child = nilHandle;
   int i, size = 16, count = 0;
   Handle * list = nil;

   if ( actionProc == nil)
      return nilHandle;
   if ( var refs)
   {
      hv_iterinit( var refs);
      list = malloc( sizeof( Handle) * size);
      for (;;)
      {
         char *k;
         I32 l;
         SV * obj = hv_iternextsv( var refs, &k, &l);
         if (( k == nil) || ( obj == nil)) break;
         if ( count >= size)
         {
            Handle *l2 = malloc( sizeof( Handle) * size * 2);
            size *= 2;
            memcpy( l2, list, sizeof( Handle) * count);
            free( list);
            list = l2;
         }
         list[ count++] = atol( k);
      }
   }

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
   PPostMsg p = malloc( sizeof( PostMsg));
   Event ev = { cmPost};
   p-> info1  = newSVsv( info1);
   p-> info2  = newSVsv( info2);
   p-> h      = self;
   if ( var postList == nil)
      list_create( var postList = malloc( sizeof( List)), 8, 8);
   list_add( var postList, ( Handle) p);
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

HV *
Component_get_dynas( Handle self)
{
   return ( HV*) SvRV( *hv_fetch(( HV*) SvRV( var mate),
				 "__DYNAS__", 9, 0));
}

HV *
Component_get_delegators( Handle self)
{
   return ( HV*) SvRV( *hv_fetch(( HV*) SvRV( var mate),
				 "__DELEGATORS__", 14, 0));
}


void
Component_update_delegator( Handle self)
{
   HV * profile;
   call_perl( self, "__update_delegator", "");
   memset( &var delegatedMessages, 0, sizeof( var delegatedMessages));
   if ( var delegateTo == nilHandle)
      return;
   profile = my get_delegators( self);
#define delegator( MsgName) if ( pexist( MsgName)) dmopt_set( dm##MsgName)
   delegator( Create);
   delegator( Destroy);
   delegator( PostMessage);
}

void
Component_set_dyna_method( Handle self, char * methodName,
			   SV * container, SV** variable)
{
   SV ** psv;
   int len = strlen( methodName);
   HV * profile = ( HV*) container;
   Bool assign;
   Bool is_hash = SvTYPE( container) == SVt_PVHV;
   if ( is_hash) {
      psv = hv_fetch( profile, methodName, len, 0);
      if (!psv) return;
      assign = true;
   } else {
      assign  = !is_hash;
      is_hash = false;
      psv = &container;
   }
   container = ( SvROK( *psv) && ( SvTYPE( SvRV( *psv)) == SVt_PVCV))
      ? newSVsv( *psv) : nilSV;
   if ( is_hash)
      hv_delete( profile, methodName, len, G_DISCARD);
   if ( assign) {
      profile = ( HV*) SvRV( *hv_fetch(( HV*) SvRV( var mate),
				       "__DYNAS__", 9, 0));
      hv_store( profile, methodName, len, container, 0);
      *variable = ( container == nilSV) ? nil : SvRV( container);
   }
}

XS( Component_notify_FROMPERL)
{
   dXSARGS;
   Handle self;
   PComponent owner;
   HV   * hv;
   char * note;
   int    rnt, snt, noteLen;
   SV   * dyna      = nil;
   SV   * delegator = nil;
   char buf[1024];
   char *s;
   SV **argsv;
   SV   * res;
   int  argsc = items - 1;
   Bool is0 = false, is1 = false, is2 = false;
   int  evPtr, ret = -1;

#define call(callType)                                                               \
{                                                                                    \
   {                                                                                 \
      dSP;                                                                           \
      int i;                                                                         \
      ENTER;                                                                         \
      SAVETMPS;                                                                      \
      PUSHMARK( sp);                                                                 \
      EXTEND( sp, argsc + (( callType == 1) ? 1 : 0));                               \
      if ( callType == 1) PUSHs( owner-> mate);                                      \
      for ( i = 0; i < argsc; i++) PUSHs( argsv[ i]);                                \
      PUTBACK;                                                                       \
      if ( callType == 0)                                                            \
         perl_call_method( buf, G_DISCARD | G_EVAL);                                 \
      else                                                                           \
         perl_call_sv(( callType == 1) ? delegator : dyna, G_DISCARD | G_EVAL);      \
      SPAGAIN;                                                                       \
      if ( SvTRUE( GvSV( errgv))) croak( SvPV( GvSV( errgv), na));                   \
      PUTBACK;                                                                       \
      FREETMPS;                                                                      \
      LEAVE;                                                                         \
   }                                                                                 \
   SPAGAIN;                                                                          \
   if ( var stage != csNormal || var evPtr != evPtr) is0 = is1 = is2 = false;        \
}

#define evOK  ( var evStack[ var evPtr - 1])

   if ( items < 2)
      croak ("Invalid usage of Component.notify");
   SP -= items;
   self    = gimme_the_mate( ST( 0));
   note    = ( char*) SvPV( ST( 1), na);
   noteLen = strlen( note);
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Component.notify");
   if ( var stage != csNormal) XSRETURN_EMPTY;
   res = my notification_types( self);
   hv = ( HV *) SvRV( res);

   SPAGAIN;
   if ( hv_exists( hv, note, strlen( note)))
   {
     temporary_prf_Sv = hv_fetch( hv, note, noteLen, 0);
     if ( !temporary_prf_Sv || !SvOK(*temporary_prf_Sv) || SvTYPE(*temporary_prf_Sv) == SVt_NULL)
        croak("RTC0045: Inconsistent storage in %s::notification_types for %s during Component.notify", var self-> className, note);
     rnt = SvIV( *temporary_prf_Sv);
     snt = rnt & (ntEvent | ntMultiple);
   } else {
     rnt = ntPrivateFirst;
     snt = ntMultiple;
   }
   sv_free( res);

   hv = ( HV*) SvRV( *hv_fetch(( HV*) SvRV( ST( 0)), "__DYNAS__", 9, 0));

   strcat( strcpy( buf, "on"), note);
   temporary_prf_Sv = hv_fetch( hv, buf, noteLen + 2, 0);
   if ( temporary_prf_Sv && SvOK(*temporary_prf_Sv))
   {
      if ( SvTYPE(*temporary_prf_Sv) == SVt_NULL)
         dyna = nil;
      else {
         if ( SvTYPE(*temporary_prf_Sv) != SVt_RV ||
              SvTYPE(SvRV(*temporary_prf_Sv)) != SVt_PVCV)
            croak("RTC0046: Inconsistent storage in %s::__DYNAS__ for %s during Component.notify", var self-> className, note);
        dyna = *temporary_prf_Sv;
      }
   }
   is2 = dyna != nil;

   owner = ( PComponent) var delegateTo;
   if ( owner && owner-> stage != csNormal) owner = nil;
   if ( owner) {
      hv = ( HV*) SvRV( *hv_fetch(( HV*) SvRV( ST( 0)), "__DELEGATORS__", 14, 0));
      temporary_prf_Sv = hv_fetch( hv, note, strlen( note), 0);
      if ( temporary_prf_Sv && SvOK(*temporary_prf_Sv)) {
         if ( SvTYPE(*temporary_prf_Sv) == SVt_NULL)
            owner = nil;
         else {
            if ( SvTYPE(*temporary_prf_Sv) != SVt_RV ||
                 SvTYPE(SvRV(*temporary_prf_Sv)) != SVt_PVCV)
               croak("RTC0046: Inconsistent storage in %s::__DELEGATORS__ for %s during Component.notify", var self-> className, note);
            delegator = *temporary_prf_Sv;
         }
      } else
         owner = nil;
      SPAGAIN;
   }
   is1 = owner != nil;

   strcat( strcpy( buf, "on_"), note);
   for ( s = buf; *s; s++)
      *s = tolower(*s);
   if ( query_method( self, buf, 0)) is0 = true;
   SPAGAIN;

   argsv = malloc( argsc * sizeof( SV*));
   {
      int i;
      for ( i = 0; i < argsc; i++) argsv[ i] = ST( i + 1);
   }
   argsv[ 0] = ST( 0);

   my push_event( self);
   SPAGAIN;
   evPtr = var evPtr;

   if ( rnt & ntCustomFirst)
   {
      switch( snt)
      {
         case ntSingle:
            if ( is2) call( 2) else
            if ( is1) call( 1) else
            if ( is0) call( 0)
            break;
         case ntEvent:
            if ( is2) call( 2)
            if ( is1 && evOK) call( 1)
            if ( is0 && evOK) call( 0)
            break;
         default:
            if ( is2) call(2)
            if ( is1) call(1)
            if ( is0) call(0)
      }
   } else {
      switch( snt)
      {
         case ntSingle:
            if ( is0) call( 0) else
            if ( is1) call( 1) else
            if ( is2) call( 2)
            break;
         case ntEvent:
            if ( is0) call( 0)
            if ( is1 && evOK) call( 1)
            if ( is2 && evOK) call( 2)
            break;
         default:
            if ( is0) call(0)
            if ( is1) call(1)
            if ( is2) call(2)
      }
   }

   if ( var stage < csDead) ret = my pop_event( self);
   SPAGAIN;
   SP -= items;
   XPUSHs( sv_2mortal( newSViv( ret)));
   free( argsv);
   PUTBACK;
}

void
Component_notify( Handle self, char * methodName)
{
   croak("Invalid call of of Component::notify");
}

void
Component_notify_REDEFINED( Handle self, char * methodName)
{
   croak("Invalid call of of Component::notify");
}
