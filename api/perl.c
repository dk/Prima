#include "apricot.h"
#include "guts.h"
#include "Component.h"

#ifdef __cplusplus
extern "C" {
#endif

I32
clean_perl_call_method( char* methname, I32 flags)
{
	I32 ret;
	dG_EVAL_ARGS;

	if ( !( flags & G_EVAL)) { OPEN_G_EVAL; }
	ret = perl_call_method( methname, flags | G_EVAL);
	if ( SvTRUE( GvSV( PL_errgv))) {
		if (( flags & (G_SCALAR|G_DISCARD|G_ARRAY)) == G_SCALAR) {
			dSP;
			SPAGAIN;
			(void)POPs;
		}
		if ( flags & G_EVAL) return ret;
		CLOSE_G_EVAL;
		croak( "%s", SvPV_nolen( GvSV( PL_errgv)));
	}

	if ( !( flags & G_EVAL)) { CLOSE_G_EVAL; }
	return ret;
}

I32
clean_perl_call_pv( char* subname, I32 flags)
{
	I32 ret;
	dG_EVAL_ARGS;

	if ( !( flags & G_EVAL)) { OPEN_G_EVAL; }
	ret = perl_call_pv( subname, flags | G_EVAL);
	if ( SvTRUE( GvSV( PL_errgv))) {
		if (( flags & (G_SCALAR|G_DISCARD|G_ARRAY)) == G_SCALAR) {
			dSP;
			SPAGAIN;
			(void)POPs;
		}
		if ( flags & G_EVAL) return ret;
		CLOSE_G_EVAL;
		croak( "%s", SvPV_nolen( GvSV( PL_errgv)));
	}

	if ( !( flags & G_EVAL)) { CLOSE_G_EVAL; }
	return ret;
}

SV *
eval( char *string)
{
	return perl_eval_pv( string, FALSE);
}

CV *
query_method( Handle object, char *methodName, Bool cacheIt)
{
	if ( object == NULL_HANDLE)
		return NULL;
	return sv_query_method((( PObject) object)-> mate, methodName, cacheIt);
}

CV *
sv_query_method( SV *sv, char *methodName, Bool cacheIt)
{
	HV *stash = NULL;

	if ( SvROK( sv)) {
		sv = (SV*)SvRV( sv);
		if ( SvOBJECT( sv))
			stash = SvSTASH(sv);
	} else {
		stash = gv_stashsv( sv, false);
	}

	if ( stash) {
		GV *gv = gv_fetchmeth( stash, methodName, strlen( methodName), cacheIt ? 0 : -1);
		if ( gv && isGV( gv))
			return GvCV(gv);
	}
	return NULL;
}

SV *
notify_perl( Handle self, char *methodName, const char *format, ...)
{
	SV *toReturn;
	char subName[ 256];
	va_list params;

	snprintf( subName, 256, "%s_%s", (( PComponent) self)-> name, methodName);
	va_start( params, format);
	toReturn = call_perl_indirect((( PComponent) self)-> owner,
					subName, format, true, false, params);
	va_end( params);
	return toReturn;
}


SV *
call_perl( Handle self, char *subName, const char *format, ...)
{
	SV *toReturn;
	va_list params;

	va_start( params, format);
	toReturn = call_perl_indirect( self, subName, format, true, false, params);
	va_end( params);
	return toReturn;
}

SV *
sv_call_perl( SV * mate, char *subName, const char *format, ...)
{
	SV *toReturn;
	va_list params;

	va_start( params, format);
	toReturn = call_perl_indirect(( Handle) mate, subName, format, false, false, params);
	va_end( params);
	return toReturn;
}

SV *
cv_call_perl( SV * mate, SV * coderef, const char *format, ...)
{
	SV *toReturn;
	va_list params;
	va_start( params, format);
	toReturn = call_perl_indirect(( Handle) mate, (char*)coderef, format, false, true, params);
	va_end( params);
	return toReturn;
}

SV *
call_perl_indirect( Handle self, char *subName, const char *format, Bool c_decl, Bool coderef, va_list params)
{
	int i;
	Handle _Handle;
	int _int;
	char * _string;
	double _number;
	Point _Point;
	Rect _Rect;
	SV * _SV;
	Bool returns = false;
	SV *toReturn = NULL;
	int retCount;
	int stackExtend = 1;


	if ( coderef)
	{
		if ( SvTYPE(( SV *) subName) != SVt_PVCV) return toReturn;
	} else {
		if (  c_decl && !query_method          ( self, subName, 0))
			return toReturn;
		if ( !c_decl && !sv_query_method(( SV *) self, subName, 0))
			return &PL_sv_undef;
	}

	if ( format[ 0] == '<')
	{
		format += 1;
		returns = true;
	}

	/* Parameter check */
	i = 0;
	while ( format[ i] != '\0')
	{
		switch ( format[ i])
		{
		case 'i':
		case 's':
		case 'U':
		case 'n':
		case 'H':
		case 'S':
			stackExtend++;
			break;
		case 'P':
			stackExtend += 2;
			break;
		case 'R':
			stackExtend += 4;
			break;
		default:
			croak( "GUTS004: Illegal parameter description (%c) in call to %s()",
						format[ i], ( coderef) ? "code reference" : subName);
			return toReturn;
		}
		i++;
	}
	{
		dSP;
		ENTER;
		SAVETMPS;
		PUSHMARK( sp);
		EXTEND( sp, stackExtend);
		PUSHs(( c_decl) ? (( PAnyObject) self)-> mate : ( SV *) self);

		i = 0;
		while ( format[ i] != '\0')
		{
			switch ( format[ i])
			{
			case 'i':
				_int = va_arg( params, int);
				PUSHs( sv_2mortal( newSViv( _int)));
				break;
			case 's':
				_string = va_arg( params, char *);
				PUSHs( sv_2mortal( newSVpv( _string, 0)));
				break;
			case 'U':
				_string = va_arg( params, char *);
				_SV = newSVpv( _string, 0 );
				_int = va_arg( params, int);
				if ( _int ) SvUTF8_on(_SV);
				PUSHs( sv_2mortal( _SV ));
				break;
			case 'n':
				_number = va_arg( params, double);
				PUSHs( sv_2mortal( newSVnv( _number)));
				break;
			case 'S':
				_SV = va_arg( params, SV *);
				PUSHs( sv_2mortal( newSVsv( _SV)));
				break;
			case 'P':
				_Point = va_arg( params, Point);
				PUSHs( sv_2mortal( newSViv( _Point. x)));
				PUSHs( sv_2mortal( newSViv( _Point. y)));
				break;
			case 'H':
				_Handle = va_arg( params, Handle);
				PUSHs( _Handle ? (( PAnyObject) _Handle)-> mate : NULL_SV);
				break;
			case 'R':
				_Rect = va_arg( params, Rect);
				PUSHs( sv_2mortal( newSViv( _Rect. left)));
				PUSHs( sv_2mortal( newSViv( _Rect. bottom)));
				PUSHs( sv_2mortal( newSViv( _Rect. right)));
				PUSHs( sv_2mortal( newSViv( _Rect. top)));
				break;
			}
			i++;
		}

		PUTBACK;
		if ( returns)
		{
			dG_EVAL_ARGS;
			OPEN_G_EVAL;
			retCount = ( coderef) ?
				perl_call_sv(( SV *) subName, G_SCALAR|G_EVAL) :
				perl_call_method( subName, G_SCALAR|G_EVAL);
			SPAGAIN;
			if ( SvTRUE( GvSV( PL_errgv)))
			{
				(void)POPs;
				CLOSE_G_EVAL;
				croak( "%s", SvPV_nolen( GvSV( PL_errgv)));    /* propagate */
			}
			CLOSE_G_EVAL;
			if ( retCount == 1)
			{
				toReturn = newSVsv( POPs);
			}
			PUTBACK;
			FREETMPS;
			LEAVE;
			if ( toReturn)
				toReturn = sv_2mortal( toReturn);
		}
		else
		{
			dG_EVAL_ARGS;
			OPEN_G_EVAL;
			if ( coderef) perl_call_sv(( SV *) subName, G_DISCARD|G_EVAL);
				else perl_call_method( subName, G_DISCARD|G_EVAL);
			if ( SvTRUE( GvSV( PL_errgv)))
			{
				CLOSE_G_EVAL;
				croak( "%s", SvPV_nolen( GvSV( PL_errgv)));    /* propagate */
			}
			CLOSE_G_EVAL;
			SPAGAIN; FREETMPS; LEAVE;
		}
	}
	return toReturn;
}

HV *
parse_hv( I32 ax, SV **sp, I32 items, SV **mark, int expected, const char *methodName)
{
	HV *hv;
	AV *order;
	int i;

	if (( items - expected) % 2 != 0)
		croak( "GUTS010: Incorrect profile (odd number of arguments) passed to ``%s''", methodName);

	hv = newHV();
	order = newAV();
	for ( i = expected; i < items; i += 2)
	{
		/* check the validity of a key */
		if (!( SvPOK( ST( i)) && ( !SvROK( ST( i)))))
			croak( "GUTS011: Illegal value for a profile key (argument #%d) passed to ``%s''", i, methodName);
		/* and add the pair */
		(void) hv_store_ent( hv, ST( i), newSVsv( ST( i+1)), 0);
		av_push( order, newSVsv( ST( i)));
	}
	(void) hv_store( hv, "__ORDER__", 9, newRV_noinc((SV *)order), 0);
	return hv;
}


void
push_hv( I32 ax, SV **sp, I32 items, SV **mark, int callerReturns, HV *hv)
{
	int n;
	HE *he;
	int wantarray = GIMME_V;
	SV **rorder;

	if ( wantarray != G_ARRAY)
	{
		sv_free((SV *)hv);
		PUTBACK;
		return;
		/* XSRETURN( callerReturns); */
	}

	rorder = hv_fetch( hv, "__ORDER__", 9, 0);
	if ( rorder != NULL && *rorder != NULL && SvROK( *rorder) && SvTYPE(SvRV(*rorder)) == SVt_PVAV) {
		int i, l;
		AV *order = (AV*)SvRV(*rorder);
		SV **key;

		n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != NULL) n++;
		n--; EXTEND( sp, n*2);

		/* push everything in proper order */
		l = av_len(order);
		for ( i = 0; i <= l; i++) {
			key = av_fetch(order, i, 0);
			if (key == NULL || *key == NULL) croak( "GUTS008:  Illegal key in order array in push_hv()");
			if ( !hv_exists_ent( hv, *key, 0)) continue;
			PUSHs( sv_2mortal( newSVsv( *key)));
			PUSHs( sv_2mortal( newSVsv( HeVAL(hv_fetch_ent(hv, *key, 0, 0)))));
		}

		sv_free(( SV *) hv);
		PUTBACK;
		return;
	}

	/* Calculate the length of our hv */
	n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != NULL) n++;
	EXTEND( sp, n*2);

	/* push everything */
	hv_iterinit( hv);
	while (( he = hv_iternext( hv)) != NULL)
	{
		PUSHs( sv_2mortal( newSVsv( hv_iterkeysv( he))));
		PUSHs( sv_2mortal( newSVsv( HeVAL( he))));
	}
	sv_free(( SV *) hv);
	PUTBACK;
	return;
	/* XSRETURN( callerReturns + n*2); */
}

SV **
push_hv_for_REDEFINED( SV **sp, HV *hv)
{
	int n;
	HE *he;
	SV **rorder;

	rorder = hv_fetch( hv, "__ORDER__", 9, 0);
	if ( rorder != NULL && *rorder != NULL && SvROK( *rorder) && SvTYPE(SvRV(*rorder)) == SVt_PVAV) {
		int i, l;
		AV *order = (AV*)SvRV(*rorder);
		SV **key;

		n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != NULL) n++;
		n--; EXTEND( sp, n*2);

		/* push everything in proper order */
		l = av_len(order);
		for ( i = 0; i <= l; i++) {
			key = av_fetch(order, i, 0);
			if (key == NULL || *key == NULL) croak( "GUTS008:  Illegal key in order array in push_hv_for_REDEFINED()");
			if ( !hv_exists_ent( hv, *key, 0)) continue;
			PUSHs( sv_2mortal( newSVsv( *key)));
			PUSHs( sv_2mortal( newSVsv( HeVAL( hv_fetch_ent(hv, *key, 0, 0)))));
		}

		return sp;
	}

	/* Calculate the length of our hv */
	n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != NULL) n++;
	EXTEND( sp, n*2);

	/* push everything */
	hv_iterinit( hv);
	while (( he = hv_iternext( hv)) != NULL)
	{
		PUSHs( sv_2mortal( newSVsv( hv_iterkeysv( he))));
		PUSHs( sv_2mortal( newSVsv( HeVAL( he))));
	}
	return sp;
}

int
pop_hv_for_REDEFINED( SV **sp, int returned, HV *hv, int expected)
{
	int i;
	AV *order;

	if (( returned - expected) % 2 != 0)
		croak( "GUTS012: Cannot create HV from the odd number of arguments returned (%d,%d)", returned, expected);

	hv_clear( hv);
	order = newAV();
	for ( i = 0; i < returned - expected; i += 2)
	{
		SV *v = POPs;
		SV *k = POPs;
		if (!( SvPOK( k) && ( !SvROK( k))))
			croak( "GUTS013: Illegal value for a profile key passed");
		(void) hv_store_ent( hv, k, newSVsv( v), 0);
		av_push( order, newSVsv( k));
	}
	(void) hv_store( hv, "__ORDER__", 9, newRV_noinc((SV *)order), 0);
	return expected;
}

void
perl_error(void)
{
	char * error = apc_last_error();
	if ( error == NULL) error = "unknown system error";
	sv_setpv( GvSV( PL_errgv), error);
}

#ifdef __cplusplus
}
#endif
