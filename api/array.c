#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

SV *
prima_array_new( size_t size)
{
	SV * sv;
	if ( size == 0 ) return newSVpv("", 0);
	sv = newSV( size );
	SvPOK_only(sv);
	SvCUR_set(sv, size );
	return sv;
}

void
prima_array_truncate( SV * array, size_t length )
{
	SvCUR_set(array, length );
	SvPOK_only(array);
}

SV *
prima_array_tie( SV * array, size_t size_of_entry, char * letter)
{
	SV * tie;
	AV * av1, * av2;

	av1 = newAV();
	av_push(av1, array);
	av_push(av1, newSViv(size_of_entry));
	av_push(av1, newSVpv(letter, 1));
	tie = newRV_noinc((SV*) av1);
	sv_bless(tie, gv_stashpv("Prima::array", GV_ADD));

	av2 = newAV();
	hv_magic(av2, (GV*)tie, PERL_MAGIC_tied);
	SvREFCNT(tie)--;
	return newRV_noinc((SV*) av2);
}

Bool
prima_array_parse( SV * sv, void ** ref, size_t * length, char ** letter)
{
	SV * tied;
	const MAGIC * mg;
	SV ** ssv;
	AV * av;
	int cur;

	if ( !SvROK(sv) || SvTYPE( SvRV( sv)) != SVt_PVAV)
		return false;
	av = (AV *) SvRV(sv);

	if (( mg = SvTIED_mg(( SV*) av, PERL_MAGIC_tied )) == NULL)
		return false;

	tied = SvTIED_obj(( SV* ) av, mg );
	if ( !tied || !SvROK(tied) || !sv_isa( tied, "Prima::array" ))
		return false;

	av = (AV*) SvRV(tied);
	if ( SvTYPE((SV*) av) != SVt_PVAV)
		croak("panic: corrupted array");

	ssv = av_fetch( av, 0, 0);
	if ( ssv == NULL ) croak("panic: corrupted array");
	if( ref) *ref = SvPVX(*ssv);
	cur  = SvCUR(*ssv);

	ssv = av_fetch( av, 1, 0);
	if ( ssv == NULL || SvIV(*ssv) <= 0 ) croak("panic: corrupted array");
	if( length) *length = cur / SvIV(*ssv);

	ssv = av_fetch( av, 2, 0);
	if ( ssv == NULL ) croak("panic: corrupted array");
	if( letter) *letter = SvPV(*ssv, PL_na);

	return true;
}

Bool
prima_read_point( SV *rv_av, int * pt, int number, char * error)
{
	SV ** holder;
	int i;
	AV *av;
	Bool result = true;

	if ( !rv_av || !SvROK( rv_av) || ( SvTYPE( SvRV( rv_av)) != SVt_PVAV)) {
		result = false;
		if ( error) croak( "%s", error);
	} else {
		av = (AV*)SvRV(rv_av);
		for ( i = 0; i < number; i++) {
			holder = av_fetch( av, i, 0);
			if ( holder)
				pt[i] = SvIV( *holder);
			else {
				pt[i] = 0;
				result = false;
				if ( error) croak( "%s", error);
			}
		}
	}
	return result;
}

#define xmovi(src_t,dst_t) {                          \
	int i;                                        \
	src_t* src = (src_t*)ref;                     \
	dst_t* dst = (dst_t*)p;                       \
	for ( i = 0; i < count; i++)                  \
		*(dst++) = *(src++);                  \
	}                                             \
	break                                         \

#define xmovd(src_t,dst_t) {                          \
	int i;                                        \
	src_t* src = (src_t*)ref;                     \
	dst_t* dst = (dst_t*)p;                       \
	for ( i = 0; i < count; i++) {                \
		register src_t x = *(src++);          \
		*(dst++) = x + ((x < 0) ? -.5 : +.5); \
	}}                                            \
	break                                         \


void *
prima_read_array( SV * points, char * procName, char type, int div, int min, int max, int * n_points, Bool * do_free)
{
	AV * av;
	int i, count, psize;
	void * p;

	switch(type) {
	case 's': psize = sizeof(uint16_t); break;
	case 'i': psize = sizeof(int);      break;
	case 'd': psize = sizeof(double);   break;
	default: croak("Bad type %c", type);
	}
	if ( !SvROK( points) || ( SvTYPE( SvRV( points)) != SVt_PVAV)) {
		warn("Invalid array reference passed to %s", procName);
		return NULL;
	}
	av = ( AV *) SvRV( points);
	count = av_len( av) + 1;
	if ( min == max && count != min * div ) {
		warn("%s: array must contain %d elements", procName, min * div);
		return NULL;
	}
	if ( count < min * div ) {
		warn("%s: array must contain at least %d elements", procName, min * div);
		return NULL;
	}
	if ( max >= 0 && count > max * div ) {
		warn("%s: array must contain maximum %d elements", procName, max * div);
		return NULL;
	}
	if ( count % div != 0 ) {
		warn("%s: number of elements in an array must be a multiple of %d", procName, div);
		return NULL;
	}
	if ( n_points)
		*n_points = count / div;
	if ( count == 0)
		return NULL;

	{
		void * ref;
		char * pack;
		if ( prima_array_parse( points, &ref, NULL, &pack )) {
			if (*pack == type && do_free) {
				*do_free = false;
				return ref;
			}

			if (!( p = malloc( psize * count))) {
				warn("not enough memory");
				return false;
			}
			if (do_free) *do_free = true;

			if ( *pack == type )
				memcpy( p, ref, psize * count);
			else switch ( *pack ) {
			case 'i':
				switch (type) {
				case 'd': xmovi(int,double);
				case 's': xmovi(int,uint16_t);
				}
				break;
			case 's':
				switch (type) {
				case 'd': xmovi(uint16_t,double);
				case 'i': xmovi(uint16_t,int);
				}
				break;
			case 'd':
				switch (type) {
				case 'i': xmovd(double,int);
				case 's': xmovd(double,uint16_t);
				}
				break;
			}
			return p;
		}
	}


	if (!( p = malloc( psize * count))) {
		warn("not enough memory");
		return NULL;
	}

	for ( i = 0; i < count; i++)
	{
		SV** psv = av_fetch( av, i, 0);
		if ( psv == NULL) {
			free( p);
			warn("Array panic on item %d on %s", i, procName);
			return NULL;
		}
		switch (type) {
		case 'i':
			*(((int*)p) + i) = SvIV( *psv);
			break;
		case 'd':
			*(((double*)p) + i) = SvNV( *psv);
			break;
		case 's':
			*(((uint16_t*)p) + i) = SvIV( *psv);
			break;
		}
	}

	if ( do_free )
		*do_free = true;

	return p;
}

#ifdef __cplusplus
}
#endif
