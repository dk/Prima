#include "apricot.h"
#include "guts.h"
#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && defined(PERL_OBJECT)
XSLockManager g_XSLock;
CPerlObj* pPerl;
#endif

PrimaGuts prima_guts;

Handle
create_mate( SV *perlObject)
{
	PAnyObject object;
	Handle self = NULL_HANDLE;
	char *className;
	PVMT vmt;

	/* finding the vmt */
	className = HvNAME( SvSTASH( SvRV( perlObject))); if ( !className) return 0;
	vmt = gimme_the_vmt( className); if ( !vmt) return 0;

	/* allocating an instance */
	object = ( PAnyObject) malloc( vmt-> instanceSize);
	if ( !object) return NULL_HANDLE;

	memset( object, 0, vmt-> instanceSize);
	object-> self = ( PVMT) vmt;
	object-> super = ( PVMT *) vmt-> super;

	(void) hv_store( (HV*)SvRV( perlObject), "__CMATE__", 9, newSViv( PTR2IV(object)), 0);

	/* extra check */
	self = gimme_the_mate( perlObject);
	if ( self != (Handle)object)
		croak( "GUTS007: create_mate() consistency check failed.\n");
	return self;
}


Handle
gimme_the_real_mate( SV *perlObject)
{
	HV *obj;
	SV **mate;
	if ( !SvROK( perlObject)) return NULL_HANDLE;
	obj = (HV*)SvRV( perlObject);
	if ( SvTYPE((SV*)obj) != SVt_PVHV) return NULL_HANDLE;
	mate = hv_fetch( obj, "__CMATE__", 9, 0);
	if ( mate == NULL) return NULL_HANDLE;
	return SvIV( *mate);
}

Handle
gimme_the_mate( SV *perlObject)
{
	Handle cMate;
	cMate = gimme_the_real_mate( perlObject);
	return (( cMate == NULL_HANDLE) || ((( PObject) cMate)-> stage == csDead)) ? NULL_HANDLE : cMate;
}


XS( create_from_Perl)
{
	dXSARGS;
	if ( prima_guts.init_ok <= 2 )
		croak("Prima is not initialized%s.", PL_minus_c ? " under -c mode" : "");
	if (( items - 2 + 1) % 2 != 0)
		croak("Invalid usage of Prima::Object::create");
	{
		Handle  _c_apricot_res_;
		HV *hv = parse_hv( ax, sp, items, mark, 2 - 1, "Object_create");
		_c_apricot_res_ = Object_create(
			( char*) SvPV_nolen( ST( 0)),
			hv
		);
		SPAGAIN;
		SP -= items;
		if ( _c_apricot_res_ && (( PAnyObject) _c_apricot_res_)-> mate && (( PAnyObject) _c_apricot_res_)-> mate != NULL_SV)
		{
			XPUSHs( sv_mortalcopy((( PAnyObject) _c_apricot_res_)-> mate));
			--SvREFCNT( SvRV((( PAnyObject) _c_apricot_res_)-> mate));
		} else XPUSHs( &PL_sv_undef);
		/* push_hv( ax, sp, items, mark, 1, hv); */
		sv_free(( SV *) hv);
	}
	PUTBACK;
	return;
}


XS( destroy_from_Perl)
{
	dXSARGS;
	Handle self;
	if ( items != 1)
		croak ("Invalid usage of Prima::Object::destroy");
	self = gimme_the_real_mate( ST( 0));
	if ( self == NULL_HANDLE)
		croak( "Illegal object reference passed to Prima::Object::destroy");
	{
		Object_destroy( self);
	}
	XSRETURN_EMPTY;
}

void
prima_kill_zombies( void)
{
	while ( prima_guts.kill_chain != NULL)
	{
		PAnyObject killee = prima_guts.kill_chain;
		prima_guts.kill_chain = killee-> killPtr;
		free( killee);
	}
}

void
prima_refcnt_inc( Handle obj)
{
	if ( obj )
		++SvREFCNT( SvRV((( PAnyObject) obj)-> mate));
}

void
prima_refcnt_dec( Handle obj)
{
	if ( obj )
		--SvREFCNT( SvRV((( PAnyObject) obj)-> mate));
}

void
protect_object( Handle obj)
{
	PObject o = (PObject)obj;
	if ( o-> protectCount >= 0) o-> protectCount++;
}

void
unprotect_object( Handle obj)
{
	PObject o = (PObject)obj;
	if (!o || o-> protectCount<=0)
		return;
	o-> protectCount--;
	if (o-> protectCount>0) return;
	if (o-> stage == csDead || o-> mate == NULL || o-> mate == NULL_SV)
	{
		PAnyObject ghost, lg;

		lg = NULL;
		ghost = prima_guts.ghost_chain;
		while ( ghost != NULL && ghost != (PAnyObject) o)
		{
			lg    = ghost;
			ghost = (PAnyObject)(ghost-> killPtr);
		}
		if ( ghost == (PAnyObject) o)
		{
			if ( lg == NULL)
				prima_guts.ghost_chain = (PAnyObject)(o-> killPtr);
			else
				lg-> killPtr = o-> killPtr;
			o-> killPtr = prima_guts.kill_chain;
			prima_guts.kill_chain = (PAnyObject)o;
		}
	}
}

Bool
kind_of( Handle object, void *cls)
{
	PVMT vmt = object ? (( PAnyObject) object)-> self : NULL;
	while (( vmt != NULL) && ( vmt != cls))
		vmt = vmt-> base;
	return vmt != NULL;
}

XS( Prima_message_FROMPERL)
{
	dXSARGS;
	(void)items;
	if ( items != 1)
		croak("Invalid usage of Prima::%s", "message");
	apc_show_message((char*) SvPV_nolen( ST(0)), prima_is_utf8_sv(ST(0)));
	XSRETURN_EMPTY;
}

XS( Prima_dl_export)
{
	dXSARGS;
	(void)items;
	if ( items != 1)
		croak("Invalid usage of Prima::%s", "dl_export");
	apc_dl_export((char*) SvPV_nolen( ST(0)));
	XSRETURN_EMPTY;
}

Bool
build_dynamic_vmt( void *vvmmtt, const char *ancestorName, int ancestorVmtSize)
{
	PVMT vmt = ( PVMT) vvmmtt;
	PVMT ancestorVmt = gimme_the_vmt( ancestorName);
	int i, n;
	void **to, **from;

	if ( ancestorVmt == NULL)
	{
		warn( "GUTS001: Cannot locate base class \"%s\" of class \"%s\"\n", ancestorName, vmt-> className);
		return false;
	}
	if ( ancestorVmt-> base != ancestorVmt-> super)
	{
		warn( "GUTS002: Cannot inherit C-class \"%s\" from Perl-class \"%s\"\n", vmt-> className, ancestorName);
		return false;
	}

	vmt-> base = vmt-> super = ancestorVmt;
	n = (ancestorVmtSize - sizeof(VMT)) / sizeof( void *);
	from = (void **)((char *)ancestorVmt + sizeof(VMT));
	to = (void **)((char *)vmt + sizeof(VMT));
	for ( i = 0; i < n; i++) if ( to[i] == NULL) to[i] = from[i];
	build_static_vmt( vmt);
	prima_register_notifications( vmt);
	return true;
}

void
build_static_vmt( void *vvmmtt)
{
	PVMT vmt = ( PVMT) vvmmtt;
	hash_store( prima_guts.vmt_hash, vmt-> className, strlen( vmt-> className), vmt);
}

PVMT
gimme_the_vmt( const char *className)
{
	PVMT vmt;
	PVMT originalVmt = NULL;
	int vmtSize;
	HV *stash;
	SV **proc;
	char *newClassName;
	int i;
	void **addr;
	SV **vmtAddr;
	SV **isaGlob;
	SV **inheritedName;
	VmtPatch *patch; int patchLength;
	PVMT patchWhom;

	/* Check whether this class has been already built... */
	vmtAddr = ( SV **) hash_fetch( prima_guts.vmt_hash, (char *)className, strlen( className));
	if ( vmtAddr != NULL) return ( PVMT) vmtAddr;

	/* No;  try to find inherited VMT... */
	stash = gv_stashpv( (char *)className, false);
	if ( stash == NULL)
	{
		croak( "GUTS003: Cannot locate package %s\n", className);
		return NULL;     /* Definitely wrong! */
	}

	isaGlob = hv_fetch( stash, "ISA", 3, 0);
	if (! (( isaGlob == NULL) ||
		( *isaGlob == NULL) ||
		( !GvAV(( GV *) *isaGlob)) ||
		( av_len( GvAV(( GV *) *isaGlob)) < 0)
	))
	{
		/* ISA found! */
		inheritedName = av_fetch( GvAV(( GV *) *isaGlob), 0, 0);
		if ( inheritedName != NULL)
			originalVmt = gimme_the_vmt( SvPV_nolen( *inheritedName));
		else
			return NULL; /* The error message will be printed by the previous incarnation */
	}
	if ( !originalVmt) {
		croak( "GUTS005: Error finding ancestor's VMT for %s\n", className);
		return NULL;
	}
	/* Do we really need to do this? */
	if ( strEQ( className, originalVmt-> className))
		return originalVmt;

	vmtSize = originalVmt-> vmtSize;
	vmt = ( PVMT) malloc( vmtSize);
	if ( !vmt) return NULL;

	memcpy( vmt, originalVmt, vmtSize);
	newClassName = duplicate_string( className);
	vmt-> className = newClassName;
	vmt-> base = originalVmt;

	/* Not particularly effective now... */
	patchWhom = originalVmt;
	while ( patchWhom != NULL)
	{
		if ( patchWhom-> base == patchWhom-> super)
		{
			patch = patchWhom-> patch;
			patchLength = patchWhom-> patchLength;
			for ( i = 0; i < patchLength; i++)
			{
				proc = hv_fetch( stash, patch[ i]. name, strlen( patch[ i]. name), 0);
				if (! (( proc == NULL) || ( *proc == NULL) || ( !GvCV(( GV *) *proc))))
				{
					addr = ( void **)((( char *)vmt) + ((( char *)( patch[ i]. vmtAddr)) - (( char *)patchWhom)));
					*addr = patch[ i]. procAddr;
				}
			}
		}
		patchWhom = patchWhom-> base;
	}

	/* Store newly created vmt into our hash... */
	hash_store( prima_guts.vmt_hash, (char *)className, strlen( className), vmt);
	list_add( &prima_guts.static_objects, (Handle) vmt);
	list_add( &prima_guts.static_objects, (Handle) vmt-> className);
	prima_register_notifications( vmt);
	return vmt;
}

typedef struct _RemapHashNode_ {
	Handle key;
	Handle val;
	struct _RemapHashNode_ *next;
} RemapHashNode, *PRemapHashNode;

typedef struct _RemapHash_ {
	PRemapHashNode table[1];
} RemapHash, *PRemapHash;

Handle
ctx_remap_def( Handle value, Handle *table, Bool direct, Handle default_value)
{
	register PRemapHash hash;
	register PRemapHashNode node;

	if ( table == NULL) return default_value;
	if ( table[0] != endCtx) {
		/* Hash was not built before;  building */
		Handle *tbl;
		PRemapHash hash1, hash2;
		PRemapHashNode next;
		int sz = 0;

		tbl = table;
		while ((*tbl) != endCtx) {
			tbl += 2;
			sz++;
		}

		/* First way build hash */
		hash = ( PRemapHash)  malloc( sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1) + sizeof( RemapHashNode) * sz);
		if ( !hash) return default_value;
		bzero( hash, sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
		tbl = table;
		next = ( PRemapHashNode )(((char *)hash) + sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
		while ((*tbl) != endCtx) {
			Handle key = (*tbl)&0x1F;
			if (hash->table[key]) {
				/* Already exists something */
				node = hash->table[key];
				while ( node-> next) node = node-> next;
				/* node->next = malloc( sizeof( RemapHashNode)); */
				node->next = next++;
				node->next-> key = tbl[0];
				node->next-> val = tbl[1];
				node->next-> next = NULL;
			} else {
				/* hash->table[key] = malloc( sizeof( RemapHashNode)); */
				hash->table[key] = next++;
				hash->table[key]-> key = tbl[0];
				hash->table[key]-> val = tbl[1];
				hash->table[key]-> next = NULL;
			}
			tbl += 2;
		}
		hash1 = hash;

		/* Second way build hash */
		hash = ( PRemapHash) malloc( sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1) + sizeof( RemapHashNode) * sz);
		if ( !hash) {
			free( hash1);
			return default_value;
		}
		bzero( hash, sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
		tbl = table;
		next = ( PRemapHashNode)(((char *)hash) + sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
		while ((*tbl) != endCtx) {
			Handle key = tbl[1] & 0x1F;
			if (hash->table[key]) {
				/* Already exists something */
				node = hash->table[key];
				while ( node-> next) node = node-> next;
				/* node->next = malloc( sizeof( RemapHashNode)); */
				node->next = next++;
				node->next-> key = tbl[1];
				node->next-> val = tbl[0];
				node->next-> next = NULL;
			} else {
				/* hash->table[key] = malloc( sizeof( RemapHashNode)); */
				hash->table[key] = next++;
				hash->table[key]-> key = tbl[1];
				hash->table[key]-> val = tbl[0];
				hash->table[key]-> next = NULL;
			}
			tbl += 2;
		}
		hash2 = hash;
		table[0] = endCtx;
		table[1] = list_add( &prima_guts.static_objects, ( Handle) hash1);
		table[2] = list_add( &prima_guts.static_objects, ( Handle) hash2);
	}

	hash = ( PRemapHash) list_at( &prima_guts.static_objects, direct ? table[1] : table[2]);
	node = hash->table[value&0x1F];
	while ( node) {
		if (node->key == value) return node->val;
		node = node->next;
	}
	return default_value;
}

void *
create_object( const char *objClass, const char *types, ...)
{
	va_list params;
	HV *profile;
	char *s;
	Handle res;

	va_start( params, types);
	profile = newHV();
	while (*types) {
		s = va_arg( params, char *);
		switch (*types) {
		case 'i':
			(void) hv_store( profile, s, strlen( s), newSViv(va_arg(params, int)), 0);
			break;
		case 's':
			(void) hv_store( profile, s, strlen( s), newSVpv(va_arg(params, char *),0), 0);
			break;
		case 'n':
			(void) hv_store( profile, s, strlen( s), newSVnv(va_arg(params, double)), 0);
			break;
		default:
			croak( "GUTS014: create_object: illegal parameter type");
		}
		types++;
	}
	va_end( params);
	res = Object_create((char *)objClass, profile);
	if ( res)
		--SvREFCNT( SvRV((( PAnyObject) res)-> mate));
	sv_free(( SV *) profile);
	return (void*)res;
}

Handle
apc_get_application(void)
{
	return prima_guts.application;
}

Handle
apc_get_core_version(void)
{
	return PRIMA_CORE_VERSION;
}

PPrimaGuts prima_api_guts(void)
{
	return &prima_guts;
}

Bool
prima_sv_bool(SV *sv)
{
	return SvTRUE(sv);
}

FillPattern fillPatterns[] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
{0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF},
{0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01},
{0x70, 0x38, 0x1C, 0x0E, 0x07, 0x83, 0xC1, 0xE0},
{0xE1, 0xC3, 0x87, 0x0F, 0x1E, 0x3C, 0x78, 0xF0},
{0x4B, 0x96, 0x2D, 0x5A, 0xB4, 0x69, 0xD2, 0xA5},
{0x88, 0x88, 0x88, 0xFF, 0x88, 0x88, 0x88, 0xFF},
{0x18, 0x24, 0x42, 0x81, 0x18, 0x24, 0x42, 0x81},
{0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC},
{0x00, 0x08, 0x00, 0x80, 0x00, 0x08, 0x00, 0x80},
{0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00, 0x88},
{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55},
{0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff},
{0x51, 0x22, 0x15, 0x88, 0x45, 0x22, 0x54, 0x88},
{0x02, 0x27, 0x05, 0x00, 0x20, 0x72, 0x50, 0x00}
};

#ifdef __cplusplus
}
#endif
