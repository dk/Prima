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

	if ( prima_guts.objects == NULL)
		return NULL_HANDLE;

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
	if ( prima_guts.objects)
		hash_store( prima_guts.objects, &self, sizeof( self), (void*)1);
	SvREFCNT_dec( profRef);
	if ( var-> stage > csConstructing) {
		if ( var-> mate && ( var-> mate != NULL_SV) && SvRV( var-> mate))
			--SvREFCNT( SvRV( var-> mate));
		return NULL_HANDLE;
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
	SV *mate, *object = NULL;
	int enter_stage = var-> stage;

	if ( var-> stage == csDeadInInit) {
		/* lightweight destroy */
		if ( is_opt( optInDestroyList)) {
			list_delete( &prima_guts.post_destroys, self);
			opt_clear( optInDestroyList);
		}
		if ( prima_guts.objects)
			hash_delete( prima_guts.objects, &self, sizeof( self), false);
		mate = var-> mate;
		var-> stage = csDead;
		var-> mate = NULL_SV;
		if ( mate && object) sv_free( mate);
		return;
	}

	if ( var-> stage > csNormal && var-> stage != csHalfDead)
		return;

	if ( var-> destroyRefCount > 0) {
		if ( !is_opt( optInDestroyList)) {
			opt_set( optInDestroyList);
			list_add( &prima_guts.post_destroys, self);
		}
		return;
	}

	if ( var-> stage == csHalfDead) {
		Handle owner;
		if ( !var-> mate || ( var-> mate == NULL_SV))
			return;
		object = SvRV( var-> mate);
		if ( !object)
			return;
		var-> stage = csFinalizing;
		prima_guts.recursive_call++;
		protect_chain( owner = var-> owner, 1);
		my-> done( self);
		protect_chain( owner, -1);
		prima_guts.recursive_call--;
		if ( is_opt( optInDestroyList)) {
			list_delete( &prima_guts.post_destroys, self);
			opt_clear( optInDestroyList);
		}
		if ( prima_guts.objects)
			hash_delete( prima_guts.objects, &self, sizeof( self), false);
		var-> stage = csDead;
		return;
	}
	var-> stage = csDestroying;
	mate = var-> mate;
	if ( mate && ( mate != NULL_SV)) {
		object = SvRV( mate);
		if ( object) ++SvREFCNT( object);
	}
	if ( object) {
		Handle owner;
		var-> stage = csHalfDead;
		prima_guts.recursive_call++;
		/*  ENTER;
		SAVEINT prima_guts.recursive_call; */
		protect_chain( owner = var-> owner, 1);
		if ( enter_stage > csConstructing)
			my-> cleanup( self);
		else if ( enter_stage == csConstructing && var-> transient_class)
			((PObject_vmt)var-> transient_class)-> cleanup( self);
		if ( var-> stage == csHalfDead) {
			var-> stage = csFinalizing;
			my-> done( self);
			if ( prima_guts.objects)
				hash_delete( prima_guts.objects, &self, sizeof( self), false);
			if ( is_opt( optInDestroyList)) {
				list_delete( &prima_guts.post_destroys, self);
				opt_clear( optInDestroyList);
			}
		}
		protect_chain( owner, -1);
		/*  LEAVE; */
		prima_guts.recursive_call--;
	}
	var-> stage = csDead;
	var-> mate = NULL_SV;
	if ( mate && object) sv_free( mate);

	while (( prima_guts.recursive_call == 0) && ( prima_guts.post_destroys. count > 0)) {
		Handle last = prima_guts.post_destroys. items[ 0];
		prima_guts.recursive_call++;
		Object_destroy( prima_guts.post_destroys. items[ 0]);
		prima_guts.recursive_call--;
		if ( prima_guts.post_destroys. count == 0) break;
		if ( prima_guts.post_destroys. items[ 0] != last) continue;
		if ( prima_guts.post_destroys. count == 1)
			croak("Zombie detected: %p", (void*)last);
		else {
			list_delete_at( &prima_guts.post_destroys, 0);
			list_add( &prima_guts.post_destroys, last);
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
	if ( _c_apricot_self_ != NULL_HANDLE) {
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
