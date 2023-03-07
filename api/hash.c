#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

PHash
prima_hash_create()
{
	PHash ret = newHV();
	list_add( &prima_guts.static_hashes, ( Handle) ret);
	return ret;
}

void
hash_destroy( PHash h, Bool killAll)
{
	HE *he;
	list_delete( &prima_guts.static_hashes, ( Handle) h);
	hv_iterinit( h);
	while (( he = hv_iternext( h)) != NULL) {
		if ( killAll) free( HeVAL( he));
		HeVAL( he) = &PL_sv_undef;
	}
	sv_free(( SV *) h);
}

static SV *ksv = NULL;

#define ksv_check  if ( !ksv) {                                      \
			ksv = newSV( keyLen);                          \
			if (!ksv) croak( "GUTS015: Cannot create SV"); \
		}                                                 \
		sv_setpvn( ksv, ( char *) key, keyLen);           \
		he = hv_fetch_ent( h, ksv, false, 0)


void *
hash_fetch( PHash h, const void *key, int keyLen)
{
	HE *he;
	ksv_check;
	if ( !he) return NULL;
	return HeVAL( he);
}

void *
hash_delete( PHash h, const void *key, int keyLen, Bool kill)
{
	HE *he;
	void *val;
	ksv_check;
	if ( !he) return NULL;
	val = HeVAL( he);
	HeVAL( he) = &PL_sv_undef;
	(void) hv_delete_ent( h, ksv, G_DISCARD, 0);
	if ( kill) {
		free( val);
		return NULL;
	}
	return val;
}

Bool
hash_store( PHash h, const void *key, int keyLen, void *val)
{
	HE *he;
	ksv_check;
	if ( he) {
		HeVAL( he) = &PL_sv_undef;
		(void) hv_delete_ent( h, ksv, G_DISCARD, 0);
	}
	he = hv_store_ent( h, ksv, &PL_sv_undef, 0);
	HeVAL( he) = ( SV *) val;
	return true;
}

void *
hash_first_that( PHash h, PHashProc action, void * params, int * pKeyLen, void ** pKey)
{
	HE *he;

	if ( action == NULL || h == NULL) return NULL;
	hv_iterinit(( HV*) h);
	for (;;)
	{
		void *value, *key;
		int  keyLen;
		if (( he = hv_iternext( h)) == NULL)
			return NULL;
		value  = HeVAL( he);
		key    = HeKEY( he);
		keyLen = HeKLEN( he);
		if (action( value, keyLen, key, params)) {
			if ( pKeyLen) *pKeyLen = keyLen;
			if ( pKey) *pKey = key;
			return value;
		}
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif
