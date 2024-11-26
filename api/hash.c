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

#define ksv_init  if ( !ksv) {                                         \
			ksv = newSV( 32);                              \
			if (!ksv) croak( "GUTS015: Cannot create SV"); \
		}                                                      \

#define ksv_check  \
	ksv_init; \
	sv_setpvn( ksv, ( char *) key, keyLen);\
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
hash_fetch_key( PHash h, const void *key, int keyLen)
{
	HE *he;
	ksv_check;
	if ( !he) return NULL;
	return HeKEY( he);
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

Bool
hash_store_release( PHash h, const void *key, int keyLen, void *val)
{
	HE *he;
	ksv_check;
	if ( he) {
		free( HeVAL(he) );
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

#define INIT_CACHE \
	if ( key_size > MAX_CACHE_KEY_LEN ) croak("cache key too big"); \
	k.type = type; \
	memcpy( k.data, key, key_size)

#define LOOKUP prima_guts.cache, &k, sizeof(k.type) + key_size

void
prima_cache_release( int type, void *key, unsigned int key_size)
{
	PrimaCacheKey k;
	PrimaValueKey *v;
	INIT_CACHE;
	if (( v = (PrimaValueKey*) hash_fetch(LOOKUP)) == NULL)
		return;
	if ( v->refcnt > 0 )
		v->refcnt++;
	if ( v-> refcnt == 0)
		hash_delete( LOOKUP, true);
}

PrimaValueKey*
prima_cache_get( int type, void *key, unsigned int key_size)
{
	PrimaCacheKey k;
	INIT_CACHE;
	return (PrimaValueKey*) hash_fetch(LOOKUP);
}

void
prima_cache_delete( int type, void *key, unsigned int key_size)
{
	PrimaCacheKey k;
	INIT_CACHE;
	hash_delete(LOOKUP, true);
}

void
prima_cache_set( int type, void *key, unsigned int key_size, void* value, unsigned int value_size)
{
	PrimaCacheKey k;
	PrimaValueKey *v;
	INIT_CACHE;

	if ( !( v = malloc(sizeof(PrimaValueKey) + value_size))) {
		warn("not enough memory: %d bytes", value_size);
		return;
	}

	v->refcnt = 1;
	v->size   = value_size;
	memcpy( v->data, value, value_size);
	hash_store(LOOKUP, v);
}

void
prima_cache_purge( int type, unsigned int max_entries)
{
#define MAX_HE 1024
	HE **he_ptr, *he_buf[MAX_HE];
	unsigned int count = 0, n_he = 0;

	if (HvKEYS(prima_guts.cache) < max_entries)
		return;

	if (max_entries > MAX_HE) {
		if ( !( he_ptr = malloc(max_entries * sizeof(HE*))))
			return;
	} else
		he_ptr = (HE**) &he_buf;

	hv_iterinit(prima_guts.cache);

	for (;;)
	{
		HE *he;
		PrimaCacheKey *key;
		if (( he = hv_iternext( prima_guts.cache)) == NULL)
			return;
		key    = (PrimaCacheKey*) HeKEY( he);
		if ( key->type != type )
			continue;
		count++;
		he_buf[n_he++] = he;
	}

	if ( count < max_entries ) {
		if ( he_ptr != (HE**) &he_buf ) free( he_ptr );
		return;
	}

	for ( count = 0; count < n_he; count++) {
		HE *he = he_ptr[count];
		ksv_init;
		sv_setpvn( ksv, ( char *) HeKEY(he), HeKLEN(he));
		free(HeVAL( he));
		HeVAL( he) = &PL_sv_undef;
		(void) hv_delete_ent( prima_guts.cache, ksv, G_DISCARD, 0);
	}
	if ( he_ptr != (HE**) &he_buf ) free( he_ptr );
#undef MAX_HE
}

#ifdef __cplusplus
}
#endif
