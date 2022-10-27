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

	if ( !sv || !SvOK(sv) ||  !SvROK(sv) || SvTYPE( SvRV( sv)) != SVt_PVAV)
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
	register int i;                               \
	register src_t* s = (src_t*)src;              \
	register dst_t* d = (dst_t*)dst;              \
	for ( i = 0; i < n_points; i++)               \
		*(d++) = *(s++);                      \
	}                                             \
	break                                         \

#define xmovd(src_t,dst_t) {                          \
	register int i;                               \
	register src_t* s = (src_t*)src;              \
	register dst_t* d = (dst_t*)dst;              \
	for ( i = 0; i < n_points; i++) {             \
		register src_t x = *(s++);            \
		*(d++) = floor(x + .5);               \
	}}                                            \
	break                                         \

static int
typesize(char type)
{
	switch(type) {
	case 'S': return sizeof(uint16_t);
	case 's': return sizeof(int16_t);
	case 'i': return sizeof(int);
	case 'd': return sizeof(double);
	default: croak("Bad type %c", type);
	}
}

void *
prima_array_convert( int n_points, void * src, char src_type, void * _dst, char dst_type )
{
	int sz, * dst;

	(void) typesize(src_type); /* assert */
	sz = typesize(dst_type);

	if ( _dst != NULL )
		dst = _dst;
	else if ( !( dst = malloc( n_points * sz))) {
		warn("Not enought memory");
		return NULL;
	}

	if ( src_type == dst_type ) {
		memcpy( dst, src, sz * n_points);
	}
	else switch ( src_type ) {
	case 'i':
		switch (dst_type) {
		case 'd': xmovi(int,double);
		case 's': xmovi(int,int16_t);
		case 'S': xmovi(int,uint16_t);
		}
		break;
	case 'S':
		switch (dst_type) {
		case 'd': xmovi(uint16_t,double);
		case 'i': xmovi(uint16_t,int);
		case 's': xmovi(uint16_t,int16_t);
		}
		break;
	case 's':
		switch (dst_type) {
		case 'd': xmovi(int16_t,double);
		case 'i': xmovi(int16_t,int);
		case 'S': xmovi(int16_t,uint16_t);
		}
		break;
	case 'd':
		switch (dst_type) {
		case 'i': xmovd(double,int);
		case 's': xmovd(double,int16_t);
		case 'S': xmovd(double,uint16_t);
		}
		break;
	}

	return dst;
}


void *
prima_read_array( SV * points, char * procName, char type, int div, int min, int max, int * n_points, Bool * do_free)
{
	AV * av;
	int i, count, psize;
	void * p;

	if ( do_free )
		*do_free = false;

	psize = typesize(type);
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
			if (do_free) *do_free = true;
			return prima_array_convert( count, ref, *pack, NULL, type);
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
			*(((int*)p) + i) = floor( SvNV( *psv) + .5 );
			break;
		case 'd':
			*(((double*)p) + i) = SvNV( *psv);
			break;
		case 's':
			*(((int16_t*)p) + i) = floor( SvIV( *psv) + .5 );
			break;
		case 'S':
			*(((uint16_t*)p) + i) = SvUV( *psv);
			break;
		}
	}

	if ( do_free )
		*do_free = true;

	return p;
}

XS(Prima_array_deduplicate_FROMPERL)
{
	dXSARGS;
	void *ref, *cmp;
	char * letter;
	size_t i, new_size, length, orig_length, item_size, cmp_length, min_length;
	if ( items != 3)
		croak ("Invalid usage of ::deduplicate");

	if ( !prima_array_parse( ST(0), &ref, &length, &letter)) {
		warn("invalid array passed to %s", "Prima::array::deduplicate");
		goto EXIT;
	}
	orig_length = length;

	cmp_length = SvIV(ST(1));
	if ( cmp_length < 1 )
		goto EXIT;

	if ( length < 2 * cmp_length )
		goto EXIT;

	min_length = SvIV(ST(2));
	if ( min_length >= length )
		goto EXIT;
	min_length += cmp_length;

	switch (*letter) {
	case 'i':
		item_size = sizeof(int);
		break;
	case 'd':
		item_size = sizeof(double);
		break;
	case 's':
		item_size = sizeof(int16_t);
		break;
	case 'S':
		item_size = sizeof(uint16_t);
		break;
	default:
		warn("invalid array passed to %s", "Prima::array::deduplicate");
		goto EXIT;
	}


	for (
		i = cmp_length, cmp = ref, new_size = cmp_length;
		i <= length - cmp_length;
		i += cmp_length )
	{
		void *new_ref = ((Byte*)ref) + i * item_size;
		if ( memcmp( cmp, new_ref, cmp_length * item_size ) != 0 ) {
			new_size += cmp_length;
			cmp = new_ref;
		} else if ( length >= min_length ) {
			memmove( cmp, new_ref, (length - i) * item_size);
			length -= cmp_length;
			i -= cmp_length;
		} else {
			new_size = min_length - cmp_length;
			break;
		}
	}

	if ( length != orig_length ) {
		SV * tied;
		const MAGIC * mg;
		SV ** ssv;
		AV * av;

		av   = (AV *) SvRV(ST(0));
		mg   = SvTIED_mg(( SV*) av, PERL_MAGIC_tied );
		tied = SvTIED_obj(( SV* ) av, mg );
		av   = (AV*) SvRV(tied);
		ssv   = av_fetch( av, 0, 0);
		prima_array_truncate( *ssv, new_size * item_size);
	}

EXIT:
	XSRETURN_EMPTY;
}


#ifdef __cplusplus
}
#endif
