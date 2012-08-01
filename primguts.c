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
/* Guts library, main file */

#define GENERATE_TABLE_GENERATOR yes
#include "apricot.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include <dirent.h>
#include "guts.h"
#include "Object.h"
#include "Component.h"
#include "File.h"
#include "Clipboard.h"
#include "DeviceBitmap.h"
#include "Drawable.h"
#include "Widget.h"
#include "Window.h"
#include "Image.h"
#include "Icon.h"
#include "AbstractMenu.h"
#include "AccelTable.h"
#include "Menu.h"
#include "Popup.h"
#include "Application.h"
#include "Timer.h"
#include "Utils.h"
#include "Printer.h"
#include "img_conv.h"


#define USE_MAGICAL_STORAGE 0

#define MODULE "Prima Guts"

#include <Types.inc>

#ifdef __cplusplus
extern "C" {
#endif

#include "thunks.tinc"


#if defined(_MSC_VER) && defined(PERL_OBJECT)
XSLockManager g_XSLock;
CPerlObj* pPerl;
#endif

static PHash vmtHash = nil;
static List  staticObjects;
static List  staticHashes;
static int   prima_init_ok = 0;

Handle application = nilHandle;
long   apcError = 0;
List   postDestroys;
int    recursiveCall = 0;
PHash  primaObjects = nil;
SV *   eventHook = nil;

char *
duplicate_string( const char *s)
{
   int l;
   char *d;

   if (!s) return nil;
   l = strlen( s) + 1;
   d = ( char*)malloc( l);
   if ( d) memcpy( d, s, l);
   return d;
}

void *
prima_mallocz( size_t sz)
{
   void *p = malloc( sz);
   if (p)
      bzero( p, sz);
   return p;
}

char *
prima_normalize_resource_string( char *name, Bool isClass)
{
   static Bool initialize = true;
   static char table[256];
   int i;
   unsigned char *s;

   if ( initialize) {
      for ( i = 0; i < 256; i++) {
         table[i] = isalnum(i) ? i : '_';
      }
      table[0] = 0;
      initialize = false;
   }

   s = (unsigned char*)name;
   while (*s) {
      *s = table[*s];
      s++;
   }
   name[0] = isClass ? toupper(name[0]) : tolower(name[0]);
   return name;
}

#ifndef HAVE_BZERO
void
bzero( void * data, size_t size)
{
   memset( data, 0, size);
}
#endif

#ifdef PRIMA_NEED_OWN_STRICMP
int
stricmp(const char *s1, const char *s2)
{
   /* Code was taken from FreeBSD 4.0 /usr/src/lib/libc/string/strcasecmp.c */
   const unsigned char *u1 = (const unsigned char *)s1;
   const unsigned char *u2 = (const unsigned char *)s2;
   while (tolower(*u1) == tolower(*u2++))
      if (*u1++ == '\0')
         return 0;
   return (tolower(*u1) - tolower(*--u2));
}
#endif

#ifdef PRIMA_NEED_OWN_STRNICMP
int
strnicmp(const char *s1, const char *s2, size_t count)
{
   const unsigned char *u1 = (const unsigned char *)s1;
   const unsigned char *u2 = (const unsigned char *)s2;
   if ( count == 0) return 0;
   while (tolower(*u1) == tolower(*u2++)) 
      if (--count == 0 || *u1++ == '\0')
         return 0;
   return (tolower(*u1) - tolower(*--u2));
}
#endif
    
#ifndef HAVE_STRCASESTR
/* Code was taken from FreeBSD 4.8 /usr/src/lib/libc/string/strcasestr.c */
char *
strcasestr( register const char * s,  register const char * find)
{
        register char c, sc;
        register size_t len;

        if ((c = *find++) != 0) {
                c = tolower((unsigned char)c);
                len = strlen(find);
                do {
                        do {
                                if ((sc = *s++) == 0)
                                        return (NULL);
                        } while ((char)tolower((unsigned char)sc) != c);
                } while (strnicmp(s, find, len) != 0);
                s--;
        }
        return ((char *)s);
}
#endif


#ifndef HAVE_REALLOCF
/*
   This code was taken from FreeBSD 4.0 /usr/src/lib/libc/stdlib/reallocf.c
   Thanks, Poul Henning!  :-)
 */
void *
reallocf(void *ptr, size_t size)
{
   void *nptr;

   nptr = realloc(ptr, size);
   if (!nptr && ptr)
      free(ptr);
   return (nptr);
}
#endif

#if ! ( defined( HAVE_SNPRINTF) || defined( HAVE__SNPRINTF))
/*
 * It is stupid, but Borland C/C++ 5.02 doesn't have snprintf/vsnprintf in its
 * RTL.
 */
int
vsnprintf( char *buf, size_t len, const char *format, va_list args)
{
    int rc;
    rc = vsprintf( buf, format, args);
    if ( rc >= len) {
        /* We'd better die here rather than wait for memory corruption consequences! */
        fprintf( stderr, "snprintf/vsnprintf buffer overflow, memory corruption possible. Blame Borland for this error!");
        exit( 1);
    }
    return rc;
}

int
snprintf( char *buf, size_t len, const char *format, ...)
{
    int rc;
    va_list args;
    va_start( args, format);
    rc = vsnprintf( buf, len, format, args);
    va_end( args);
    return rc;
}
#endif

#ifdef PERL_CALL_SV_DIE_BUG_AWARE

I32
clean_perl_call_method( char* methname, I32 flags)
{
   I32 ret;
   dPUB_ARGS;
   dG_EVAL_ARGS;

   if ( !( flags & G_EVAL)) { OPEN_G_EVAL; }
   ret = perl_call_method( methname, flags | G_EVAL);
   if ( SvTRUE( GvSV( PL_errgv))) {
      PUB_CHECK;
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
   dPUB_ARGS;
   dG_EVAL_ARGS;

   if ( !( flags & G_EVAL)) { OPEN_G_EVAL; }
   ret = perl_call_pv( subname, flags | G_EVAL);
   if ( SvTRUE( GvSV( PL_errgv))) {
      PUB_CHECK;
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
#endif

SV *
eval( char *string)
{
   return perl_eval_pv( string, FALSE);
}

Handle
create_mate( SV *perlObject)
{
   PAnyObject object;
   Handle self = nilHandle;
   char *className;
   PVMT vmt;

   /* finding the vmt */
   className = HvNAME( SvSTASH( SvRV( perlObject))); if ( !className) return 0;
   vmt = gimme_the_vmt( className); if ( !vmt) return 0;

   /* allocating an instance */
   object = ( PAnyObject) malloc( vmt-> instanceSize);
   if ( !object) return nilHandle;

   memset( object, 0, vmt-> instanceSize);
   object-> self = ( PVMT) vmt;
   object-> super = ( PVMT *) vmt-> super;

   if (USE_MAGICAL_STORAGE)
   {
      /* assigning the tilde-magic */
      MAGIC *mg;

      sv_magic( SvRV( perlObject), SvRV( perlObject), '~', (char*)&object, sizeof(void*));
      if ( !SvMAGICAL( SvRV( perlObject)) || !(mg = mg_find( SvRV( perlObject), '~')) || mg-> mg_len < sizeof(void*))
      {
         croak( "GUTS006: create_mate() magic trick failed.\n");
         return 0;
      }
   }
   else
   {
      /* another scheme, uses hash slot */
      (void) hv_store( (HV*)SvRV( perlObject), "__CMATE__", 9, newSViv( PTR2IV(object)), 0);
   }

   /* extra check */
   self = gimme_the_mate( perlObject);
   if ( self != (Handle)object)
      croak( "GUTS007: create_mate() consistency check failed.\n");
   return self;
}


Handle
gimme_the_real_mate( SV *perlObject)
{
   if (USE_MAGICAL_STORAGE)
   {
      MAGIC *mg;
      return
         SvROK( perlObject) &&
         SvMAGICAL(SvRV( perlObject)) &&
         (mg = mg_find( SvRV( perlObject), '~')) &&
         (mg-> mg_len >= sizeof(void*)) ?
            (Handle)*((void**)(mg-> mg_ptr)) : nilHandle;
   }
   else
   {
      HV *obj;
      SV **mate;
      if ( !SvROK( perlObject)) return nilHandle;
      obj = (HV*)SvRV( perlObject);
      if ( SvTYPE((SV*)obj) != SVt_PVHV) return nilHandle;
      mate = hv_fetch( obj, "__CMATE__", 9, 0);
      if ( mate == nil) return nilHandle;
      return SvIV( *mate);
   }
}

Handle
gimme_the_mate( SV *perlObject)
{
   Handle cMate;
   cMate = gimme_the_real_mate( perlObject);
   return (( cMate == nilHandle) || ((( PObject) cMate)-> stage == csDead)) ? nilHandle : cMate;
}


XS( create_from_Perl)
{
   dXSARGS;
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
      if ( _c_apricot_res_ && (( PAnyObject) _c_apricot_res_)-> mate && (( PAnyObject) _c_apricot_res_)-> mate != nilSV)
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
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Prima::Object::destroy");
   {
      Object_destroy( self);
   }
   XSRETURN_EMPTY;
}

static PAnyObject killChain = nil;
static PObject ghostChain = nil;

void
kill_zombies( void)
{
   while ( killChain != nil)
   {
      PAnyObject killee = killChain;
      killChain = killee-> killPtr;
      free( killee);
   }
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
   if (o-> stage == csDead || o-> mate == nil || o-> mate == nilSV)
   {
      PObject ghost, lg;

      lg = nil;
      ghost = ghostChain;
      while ( ghost != nil && ghost != o)
      {
        lg    = ghost;
        ghost = (PObject)(ghost-> killPtr);
      }
      if ( ghost == o)
      {
         if ( lg == nil)
            ghostChain = (PObject)(o-> killPtr);
         else
            lg-> killPtr = o-> killPtr;
         o-> killPtr = killChain;
         killChain = (PAnyObject)o;
      }
   }
}

XS( destroy_mate)
{
   dXSARGS;
   Handle self;

   if ( items != 1)
      croak ("Invalid usage of ::destroy_mate");
   self = gimme_the_real_mate( ST( 0));

   if ( self == nilHandle)
      croak( "Illegal object reference passed to ::destroy_mate");
   {
      Object_destroy( self);
      if (((PObject)self)-> protectCount > 0)
      {
         (( PObject) self)-> killPtr = (PAnyObject)ghostChain;
         ghostChain = ( PObject) self;
      }
      else
      {
         free(( void*) self);
      }
   }
   XSRETURN_EMPTY;
}

Bool
kind_of( Handle object, void *cls)
{
   PVMT vmt = object ? (( PAnyObject) object)-> self : nil;
   while (( vmt != nil) && ( vmt != cls))
      vmt = vmt-> base;
   return vmt != nil;
}

CV *
query_method( Handle object, char *methodName, Bool cacheIt)
{
   if ( object == nilHandle)
      return nil;
   return sv_query_method((( PObject) object)-> mate, methodName, cacheIt);
}

CV *
sv_query_method( SV *sv, char *methodName, Bool cacheIt)
{
   HV *stash = nil;

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
   return nil;
}

static void
register_notifications( PVMT vmt)
{
   SV *package;
   SV *nt_sub;
   SV *nt_ref;
   HV *nt;
   PVMT v = vmt;
   HE *he;
   char buf[ 1024];

   while (( v != nil) && ( v != (PVMT) CComponent)) v = v-> base;
   if (!v) return;
   package = newSVpv( vmt-> className, 0);
   if ( !package) croak( "GUTS016: Not enough memory");
   nt_sub = ( SV*) sv_query_method( package, "notification_types", 0);
   if ( !nt_sub) croak( "GUTS016: Invalid package %s", vmt-> className);
   nt_ref = cv_call_perl( package, nt_sub, "<");
   if ( !nt_ref || !SvROK(nt_ref) || SvTYPE(SvRV(nt_ref)) != SVt_PVHV)
      croak( "GUTS016: %s: Bad notification_types() return value", vmt-> className);
   nt = (HV*)SvRV(nt_ref);

   hv_iterinit( nt);
   while (( he = hv_iternext( nt)) != nil) {
      snprintf( buf, 1024, "on%s", HeKEY( he));
      if (sv_query_method( package, buf, 0)) continue;
      snprintf( buf, 1024, "%s::on%s", vmt-> className, HeKEY( he));
      newXS( buf, Component_set_notification_FROMPERL, vmt-> className);
   }
   sv_free( package);
}

XS(Prima_options)
{
   dXSARGS;
   char * option, * value = nil;
   (void)items;

   switch ( items) {
   case 0:
      {
	 int i, argc = 0;
	 char ** argv;
	 window_subsystem_get_options( &argc, &argv);
         EXTEND( sp, argc);
	 for ( i = 0; i < argc; i++) 
            PUSHs( sv_2mortal( newSVpv( argv[i], 0)));
	 PUTBACK;
	 return;    
      }
      break;
   case 2:
      value  = (SvOK( ST(1)) ? ( char*) SvPV_nolen( ST(1)) : nil);
   case 1:
      option = ( char*) SvPV_nolen( ST(0));
      window_subsystem_set_option( option, value);
      break;
   default:
      croak("Invalid call to Prima::options");
   }
   SPAGAIN;
   XSRETURN_EMPTY;
}

XS(Prima_init)
{
   dXSARGS;
   char error_buf[256] = "Error initializing Prima";
   (void)items;

   if ( items < 1) croak("Invalid call to Prima::init"); 

   {
      SV * ref;
      SV * package = newSVpv( "Prima::Object", 0);
      if ( !package) croak( "GUTS016: Not enough memory");
      ref = ( SV *) sv_query_method( package, "profile_default", 0);
      sv_free( package);
      if ( !ref) croak("'use Prima;' call required in main script");
   }

   if ( prima_init_ok == 0) {
      register_notifications((PVMT)CComponent);
      register_notifications((PVMT)CFile);
      register_notifications((PVMT)CAbstractMenu);
      register_notifications((PVMT)CAccelTable);
      register_notifications((PVMT)CMenu);
      register_notifications((PVMT)CPopup);
      register_notifications((PVMT)CClipboard);
      register_notifications((PVMT)CTimer);
      register_notifications((PVMT)CDrawable);
      register_notifications((PVMT)CImage);
      register_notifications((PVMT)CIcon);
      register_notifications((PVMT)CDeviceBitmap);
      register_notifications((PVMT)CWidget);
      register_notifications((PVMT)CWindow);
      register_notifications((PVMT)CApplication);
      register_notifications((PVMT)CPrinter);
      prima_init_ok++;
   }
   
   if ( prima_init_ok == 1) {
      prima_init_image_subsystem();
      prima_init_ok++;
   }

   if ( prima_init_ok == 2) {
      if ( !window_subsystem_init( error_buf)) 
	 croak( "%s", error_buf);
      prima_init_ok++;
   }
   SPAGAIN;
   XSRETURN_EMPTY;
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

   if ( ancestorVmt == nil)
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
   for ( i = 0; i < n; i++) if ( to[i] == nil) to[i] = from[i];
   build_static_vmt( vmt);
   register_notifications( vmt);
   return true;
}

void
build_static_vmt( void *vvmmtt)
{
   PVMT vmt = ( PVMT) vvmmtt;
   hash_store( vmtHash, vmt-> className, strlen( vmt-> className), vmt);
}

#ifdef PARANOID_MALLOC
static unsigned long
timestamp( void);
#endif

PVMT
gimme_the_vmt( const char *className)
{
   PVMT vmt;
   PVMT originalVmt = nil;
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
   vmtAddr = ( SV **) hash_fetch( vmtHash, (char *)className, strlen( className));
   if ( vmtAddr != nil) return ( PVMT) vmtAddr;

   /* No;  try to find inherited VMT... */
   stash = gv_stashpv( (char *)className, false);
   if ( stash == nil)
   {
      croak( "GUTS003: Cannot locate package %s\n", className);
      return nil;     /* Definitely wrong! */
   }

   isaGlob = hv_fetch( stash, "ISA", 3, 0);
   if (! (( isaGlob == nil) ||
          ( *isaGlob == nil) ||
          ( !GvAV(( GV *) *isaGlob)) ||
          ( av_len( GvAV(( GV *) *isaGlob)) < 0)
      ))
   {
      /* ISA found! */
      inheritedName = av_fetch( GvAV(( GV *) *isaGlob), 0, 0);
      if ( inheritedName != nil)
         originalVmt = gimme_the_vmt( SvPV_nolen( *inheritedName));
      else
         return nil; /* The error message will be printed by the previous incarnation */
   }
   if ( !originalVmt) {
      croak( "GUTS005: Error finding ancestor's VMT for %s\n", className);
      return nil;
   }
   /* Do we really need to do this? */
   if ( strEQ( className, originalVmt-> className))
      return originalVmt;

#ifdef PARANOID_MALLOC
   debug_write( "%lu Dynamic vmt creation (%d) for %s\n", timestamp(), originalVmt-> vmtSize, className);
#endif
   vmtSize = originalVmt-> vmtSize;
   vmt = ( PVMT) malloc( vmtSize);
   if ( !vmt) return nil;

   memcpy( vmt, originalVmt, vmtSize);
   newClassName = duplicate_string( className);
   vmt-> className = newClassName;
   vmt-> base = originalVmt;

   /* Not particularly effective now... */
   patchWhom = originalVmt;
   while ( patchWhom != nil)
   {
      if ( patchWhom-> base == patchWhom-> super)
      {
         patch = patchWhom-> patch;
         patchLength = patchWhom-> patchLength;
         for ( i = 0; i < patchLength; i++)
         {
            proc = hv_fetch( stash, patch[ i]. name, strlen( patch[ i]. name), 0);
            if (! (( proc == nil) || ( *proc == nil) || ( !GvCV(( GV *) *proc))))
            {
               addr = ( void **)((( char *)vmt) + ((( char *)( patch[ i]. vmtAddr)) - (( char *)patchWhom)));
               *addr = patch[ i]. procAddr;
            }
         }
      }
      patchWhom = patchWhom-> base;
   }

   /* Store newly created vmt into our hash... */
   hash_store( vmtHash, (char *)className, strlen( className), vmt);
   list_add( &staticObjects, (Handle) vmt);
   list_add( &staticObjects, (Handle) vmt-> className);
   register_notifications( vmt);
   return vmt;
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
   SV *toReturn = nil;
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
            PUSHs( _Handle ? (( PAnyObject) _Handle)-> mate : nilSV);
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
#ifdef PERL_CALL_SV_DIE_BUG_AWARE
         dPUB_ARGS;
         dG_EVAL_ARGS;
         OPEN_G_EVAL;
         retCount = ( coderef) ?
            perl_call_sv(( SV *) subName, G_SCALAR|G_EVAL) :
            perl_call_method( subName, G_SCALAR|G_EVAL);
         SPAGAIN;
         if ( SvTRUE( GvSV( PL_errgv)))
         {
            (void)POPs;
            PUB_CHECK;
            CLOSE_G_EVAL;
            croak( "%s", SvPV_nolen( GvSV( PL_errgv)));    /* propagate */
         }
         CLOSE_G_EVAL;
#else
         retCount = ( coderef) ?
            perl_call_sv(( SV *) subName, G_SCALAR) :
            perl_call_method( subName, G_SCALAR);
         SPAGAIN;
#endif
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
#ifdef PERL_CALL_SV_DIE_BUG_AWARE
         dPUB_ARGS;
         dG_EVAL_ARGS;
         OPEN_G_EVAL;
         if ( coderef) perl_call_sv(( SV *) subName, G_DISCARD|G_EVAL);
            else perl_call_method( subName, G_DISCARD|G_EVAL);
         if ( SvTRUE( GvSV( PL_errgv)))
         {
            PUB_CHECK;
            CLOSE_G_EVAL;
            croak( "%s", SvPV_nolen( GvSV( PL_errgv)));    /* propagate */
         }
         CLOSE_G_EVAL;
#else
         if ( coderef) perl_call_sv(( SV *) subName, G_DISCARD);
            else perl_call_method( subName, G_DISCARD);
#endif
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
      HE *he;
      /* check the validity of a key */
      if (!( SvPOK( ST( i)) && ( !SvROK( ST( i)))))
         croak( "GUTS011: Illegal value for a profile key (argument #%d) passed to ``%s''", i, methodName);
      /* and add the pair */
      he = hv_store_ent( hv, ST( i), newSVsv( ST( i+1)), 0);
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
   if ( rorder != nil && *rorder != nil && SvROK( *rorder) && SvTYPE(SvRV(*rorder)) == SVt_PVAV) {
      int i, l;
      AV *order = (AV*)SvRV(*rorder);
      SV **key;

      n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
      n--; EXTEND( sp, n*2);

      /* push everything in proper order */
      l = av_len(order);
      for ( i = 0; i <= l; i++) {
         key = av_fetch(order, i, 0);
         if (key == nil || *key == nil) croak( "GUTS008:  Illegal key in order array in push_hv()");
         if ( !hv_exists_ent( hv, *key, 0)) continue;
         PUSHs( sv_2mortal( newSVsv( *key)));
         PUSHs( sv_2mortal( newSVsv( HeVAL(hv_fetch_ent(hv, *key, 0, 0)))));
      }

      sv_free(( SV *) hv);
      PUTBACK;
      return;
   }

   /* Calculate the length of our hv */
   n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
   EXTEND( sp, n*2);

   /* push everything */
   hv_iterinit( hv);
   while (( he = hv_iternext( hv)) != nil)
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
   if ( rorder != nil && *rorder != nil && SvROK( *rorder) && SvTYPE(SvRV(*rorder)) == SVt_PVAV) {
      int i, l;
      AV *order = (AV*)SvRV(*rorder);
      SV **key;

      n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
      n--; EXTEND( sp, n*2);

      /* push everything in proper order */
      l = av_len(order);
      for ( i = 0; i <= l; i++) {
         key = av_fetch(order, i, 0);
         if (key == nil || *key == nil) croak( "GUTS008:  Illegal key in order array in push_hv_for_REDEFINED()");
         if ( !hv_exists_ent( hv, *key, 0)) continue;
         PUSHs( sv_2mortal( newSVsv( *key)));
         PUSHs( sv_2mortal( newSVsv( HeVAL( hv_fetch_ent(hv, *key, 0, 0)))));
      }

      return sp;
   }

   /* Calculate the length of our hv */
   n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
   EXTEND( sp, n*2);

   /* push everything */
   hv_iterinit( hv);
   while (( he = hv_iternext( hv)) != nil)
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


static Bool
kill_objects( void * item, int keyLen, Handle * self, void * dummy)
{
   Object_destroy( *self);
   return false;
}

void
perl_error(void)
{
    char * error = apc_last_error();
    if ( error == NULL) error = "unknown system error";
    sv_setpv( GvSV( PL_errgv), error);
}

Bool appDead = false;

Bool dolbug;
Bool waitBeforeQuit;

#ifdef PARANOID_MALLOC
static void output_mallocs( void);
#endif

#if (PERL_PATCHLEVEL == 5)
#define PRIMAPERL_scopestack_ix PL_scopestack_ix
#define PRIMAPERL_defstash PL_defstash
#define PRIMAPERL_curstash PL_curstash
#define PRIMAPERL_endav PL_endav
#elif (PERL_PATCHLEVEL == 4)
#define PRIMAPERL_scopestack_ix scopestack_ix
#define PRIMAPERL_defstash defstash
#define PRIMAPERL_curstash curstash
#define PRIMAPERL_endav endav
#endif

XS(Utils_getdir_FROMPERL);

static Bool 
kill_hashes( PHash hash, void * dummy)
{
   hash_destroy( hash, false);
   return false;
}

XS( prima_cleanup)
{
   dXSARGS;
   (void)items;

   if ( application) Object_destroy( application);
   appDead = true;
   hash_first_that( primaObjects, (void*)kill_objects, nil, nil, nil);
   hash_destroy( primaObjects, false);
   primaObjects = nil;
   if ( prima_init_ok > 1) prima_cleanup_image_subsystem();
   if ( prima_init_ok > 2) window_subsystem_cleanup();
   hash_destroy( vmtHash, false);
   vmtHash = nil;
   list_delete_all( &staticObjects, true);
   list_destroy( &staticObjects);
   list_destroy( &postDestroys);
   kill_zombies();
   if ( prima_init_ok > 2) window_subsystem_done();
   list_first_that( &staticHashes, (void*)kill_hashes, nil);
   list_destroy( &staticHashes);
#ifdef PARANOID_MALLOC
   output_mallocs();
#endif
   prima_init_ok = 0;

   ST(0) = &PL_sv_yes;
   XSRETURN(1);
}

static void
register_constants( void)
{
   register_nt_constants();
   register_kb_constants();
   register_km_constants();
   register_mb_constants();
   register_ta_constants();
   register_cl_constants();
   register_ci_constants();
   register_wc_constants();
   register_cm_constants();
   register_rop_constants();
   register_gm_constants();
   register_lp_constants();
   register_fp_constants();
   register_le_constants();
   register_lj_constants();
   register_fs_constants();
   register_fw_constants();
   register_bi_constants();
   register_bs_constants();
   register_ws_constants();
   register_sv_constants();
   register_im_constants();
   register_ict_constants();
   register_is_constants();
   register_am_constants();
   register_apc_constants();
   register_gui_constants();
   register_dt_constants();
   register_cr_constants();
   register_sbmp_constants();
   register_tw_constants();
   register_fds_constants();
   register_fdo_constants();
   register_fe_constants();
   register_fr_constants();
   register_mt_constants();
   register_gt_constants();
}

XS( Object_alive_FROMPERL);
XS( Component_event_hook_FROMPERL);

XS( boot_Prima)
{
   char *dolbug_str;
   dXSARGS;
   (void)items;

   XS_VERSION_BOOTCHECK;

   if ( ( dolbug_str = getenv( "PRIMA_DOLBUG")) != NULL) {
      dolbug = ( Bool) atoi( dolbug_str);
   }

#define TYPECHECK(s1,s2) \
  if (sizeof(s1) != (s2)) { \
      printf("Error: type %s is %d bytes long (expected to be %d)", #s1, (int)sizeof(s1), s2); \
      ST(0) = &PL_sv_no; \
      XSRETURN(1); \
  }
   TYPECHECK( uint8_t,  1);
   TYPECHECK( int8_t,   1);
   TYPECHECK( uint16_t, 2);
   TYPECHECK( int16_t,  2);
   TYPECHECK( uint32_t, 4);
   TYPECHECK( int32_t,  4);
   TYPECHECK( void*, (int)sizeof(Handle));
#undef TYPECHECK

   list_create( &staticObjects, 16, 16);
   list_create( &staticHashes, 16, 16);
   primaObjects = hash_create();
   vmtHash      = hash_create();
   list_create( &postDestroys, 16, 16);

   /* register hard coded XSUBs */
   newXS( "::destroy_mate", destroy_mate, MODULE);
   newXS( "Prima::cleanup", prima_cleanup, "Prima");
   newXS( "Prima::init", Prima_init, "Prima");
   newXS( "Prima::options", Prima_options, "Prima");
   newXS( "Prima::Utils::getdir", Utils_getdir_FROMPERL, "Prima::Utils");
   /* register built-in classes */
   newXS( "Prima::Object::create",  create_from_Perl, "Prima::Object");
   newXS( "Prima::Object::destroy", destroy_from_Perl, "Prima::Object");
   newXS( "Prima::Object::alive", Object_alive_FROMPERL, "Prima::Object");
   newXS( "Prima::Component::event_hook", Component_event_hook_FROMPERL, "Prima::Component");
   newXS( "Prima::message", Prima_message_FROMPERL, "Prima");
   newXS( "Prima::dl_export", Prima_dl_export, "Prima");
   register_constants();
   register_Object_Class();
   register_Utils_Package();
   register_Component_Class();
   register_File_Class();
   register_Clipboard_Class();
   register_DeviceBitmap_Class();
   register_Drawable_Class();
   register_Widget_Class();
   register_Window_Class();
   register_Image_Class();
   init_image_support();
   register_Icon_Class();
   register_AbstractMenu_Class();
   register_AccelTable_Class();
   register_Menu_Class();
   register_Popup_Class();
   register_Application_Class();
   register_Timer_Class();
   register_Printer_Class();

   ST(0) = &PL_sv_yes;
   XSRETURN(1);
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

   if ( table == nil) return default_value;
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
                 node->next-> next = nil;
              } else {
                 /* hash->table[key] = malloc( sizeof( RemapHashNode)); */
                 hash->table[key] = next++;
                 hash->table[key]-> key = tbl[0];
                 hash->table[key]-> val = tbl[1];
                 hash->table[key]-> next = nil;
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
              Handle key = tbl[1]&0x1F;
              if (hash->table[key]) {
                 /* Already exists something */
                 node = hash->table[key];
                 while ( node-> next) node = node-> next;
                 /* node->next = malloc( sizeof( RemapHashNode)); */
                 node->next = next++;
                 node->next-> key = tbl[1];
                 node->next-> val = tbl[0];
                 node->next-> next = nil;
              } else {
                 /* hash->table[key] = malloc( sizeof( RemapHashNode)); */
                 hash->table[key] = next++;
                 hash->table[key]-> key = tbl[1];
                 hash->table[key]-> val = tbl[0];
                 hash->table[key]-> next = nil;
              }
              tbl += 2;
      }
      hash2 = hash;
      table[0] = endCtx;
      table[1] = list_add( &staticObjects, ( Handle) hash1);
      table[2] = list_add( &staticObjects, ( Handle) hash2);
   }

   hash = ( PRemapHash) list_at( &staticObjects, direct ? table[1] : table[2]);
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
   while (*types)
   {
      s = va_arg( params, char *);
      switch (*types)
      {
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
   return application;
}

Handle
apc_get_core_version()
{
    return PRIMA_CORE_VERSION;
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


/* list section */

#ifdef PARANOID_MALLOC
void
paranoid_list_create( PList slf, int size, int delta, char *fil, int ln)
{
   char *buf;
   int blen;
   if ( !slf) return;
   buf = allocs( blen = strlen( fil) + strlen( __FILE__) + 9);
   snprintf( buf, blen, "%s(%d),%s", fil, ln, __FILE__);
   memset( slf, 0, sizeof( List));
   slf-> delta = ( delta > 0) ? delta : 1;
   slf-> size  = size;
   slf-> items = ( size > 0) ? _test_malloc( size * sizeof( Handle), __LINE__, buf, nilHandle) : nil;
   free( buf);
}

PList
paranoid_plist_create( int size, int delta, char *fil, int ln)
{
   char *buf;
   int blen;
   PList new_list;
   buf = allocs( blen = strlen( fil) + strlen( __FILE__) + 9);
   snprintf( buf, blen, "%s(%d),%s", fil, ln, __FILE__);
   new_list = ( PList) _test_malloc( sizeof( List), __LINE__, buf, nilHandle);
   if ( new_list != nil) {
      paranoid_list_create( new_list, size, delta, buf, __LINE__);
   }
   free( buf);
   return new_list;
}
#else
void
list_create( PList slf, int size, int delta)
{
   if ( !slf) return;
   memset( slf, 0, sizeof( List));
   slf-> delta = ( delta > 0) ? delta : 1;
   if (( slf-> size = size) > 0) {
      if ( !( slf-> items = allocn( Handle, size)))
         slf-> size = 0;
   } else
      slf-> items = nil;
}

PList
plist_create( int size, int delta)
{
   PList new_list = alloc1( List);
   if ( new_list != nil) {
      list_create( new_list, size, delta);
   }
   return new_list;
}
#endif

void
list_destroy( PList slf)
{
   if ( !slf) return;
   free( slf-> items);
   slf-> items = nil;
   slf-> count = 0;
   slf-> size  = 0;
}

void
plist_destroy( PList slf)
{
   if ( slf != NULL) {
      list_destroy( slf);
      free( slf);
   }
}

int
list_add( PList slf, Handle item)
{
   if ( !slf) return -1;
   if ( slf-> count == slf-> size)
   {
      Handle * old = slf-> items;
      if ( !( slf-> items = allocn(Handle, ( slf-> size + slf-> delta))))
         return -1;
      if ( old) {
         memcpy( slf-> items, old, slf-> size * sizeof( Handle));
         free( old);
      }
      slf-> size += slf-> delta;
   }
   slf-> items[ slf-> count++] = item;
   return slf-> count - 1;
}

int
list_insert_at( PList slf, Handle item, int pos)
{
   int max, ret;
   Handle save;
   ret = list_add( slf, item);
   if ( ret < 0) return ret;
   max = slf-> count - 1;
   if ( pos < 0 || pos >= max) return ret;
   save = slf-> items[ max];
   memmove( &slf-> items[ pos + 1], &slf-> items[ pos], ( max - pos) * sizeof( Handle));
   slf-> items[ pos] = save;
   return pos;
}

int
list_index_of( PList slf, Handle item)
{
   int i;
   if ( !slf ) return -1;
   for ( i = 0; i < slf-> count; i++)
      if ( slf-> items[ i] == item) return i;
   return -1;
}

void
list_delete( PList slf, Handle item)
{
   list_delete_at( slf, list_index_of( slf, item));
}

void
list_delete_at( PList slf, int index)
{
   if ( !slf || index < 0 || index >= slf-> count) return;
   slf-> count--;
   if ( index == slf-> count) return;
   memmove( &slf-> items[ index], &slf-> items[ index + 1], ( slf-> count - index) * sizeof( Handle));
}

Handle
list_at( PList slf, int index)
{
   return (( index < 0 || !slf) || index >= slf-> count) ? nilHandle : slf-> items[ index];
}

int
list_first_that( PList slf, void * action, void * params)
{
   int toRet = -1, i, cnt = slf-> count;
   Handle * list;
   if ( !action || !slf || !cnt) return -1;
   if ( !( list = allocn( Handle, slf-> count)))
      return -1;
   memcpy( list, slf-> items, slf-> count * sizeof( Handle));
   for ( i = 0; i < cnt; i++)
      if ((( PListProc) action)( list[ i], params)) {
         toRet = i;
         break;
      }
   free( list);
   return toRet;
}

void
list_delete_all( PList slf, Bool kill)
{
   if ( !slf || ( slf-> count == 0)) return;
   if ( kill ) {
      int i;
      for ( i = 0; i < slf-> count; i++)
         free(( void*) slf-> items[ i]);
   }
   slf-> count = 0;
}

PHash
prima_hash_create()
{
   PHash ret = newHV();
   list_add( &staticHashes, ( Handle) ret);
   return ret;
}

void
hash_destroy( PHash h, Bool killAll)
{
   HE *he;
   list_delete( &staticHashes, ( Handle) h);
   hv_iterinit( h);
   while (( he = hv_iternext( h)) != nil) {
      if ( killAll) free( HeVAL( he));
      HeVAL( he) = &PL_sv_undef;
   }
   sv_free(( SV *) h);
}

static SV *ksv = nil;

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
   if ( !he) return nil;
   return HeVAL( he);
}

void *
hash_delete( PHash h, const void *key, int keyLen, Bool kill)
{
   HE *he;
   void *val;
   ksv_check;
   if ( !he) return nil;
   val = HeVAL( he);
   HeVAL( he) = &PL_sv_undef;
   (void) hv_delete_ent( h, ksv, G_DISCARD, 0);
   if ( kill) {
      free( val);
      return nil;
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
hash_first_that( PHash h, void * action, void * params, int * pKeyLen, void ** pKey)
{
   HE *he;

   if ( action == nil || h == nil) return nil;
   hv_iterinit(( HV*) h);
   for (;;)
   {
      void *value, *key;
      int  keyLen;
      if (( he = hv_iternext( h)) == nil)
         return nil;
      value  = HeVAL( he);
      key    = HeKEY( he);
      keyLen = HeKLEN( he);
      if ((( PHashProc) action)( value, keyLen, key, params)) {
         if ( pKeyLen) *pKeyLen = keyLen;
         if ( pKey) *pKey = key;
         return value;
      }
   }
   return nil;
}

int
prima_utf8_length( const char * utf8)
{
#ifdef PERL_SUPPORTS_UTF8
   int ret = 0;
   while ( *utf8) {
      utf8 = ( char*) utf8_hop(( U8*) utf8, 1);
      ret++;
   }
   return ret;
#else
   return 0;
#endif
}

Bool
prima_is_utf8_sv( SV * sv)
{
#ifdef PERL_SUPPORTS_UTF8
   /* from Encode.xs */
   if (SvGMAGICAL(sv)) {
      SV * sv2 = newSVsv(sv); /* GMAGIG will be done */
      Bool ret = SvUTF8(sv2) ? 1 : 0;
      SvREFCNT_dec(sv2); /* it was a temp copy */
      return ret;
   } else {
      return SvUTF8(sv) ? 1 : 0;
   }
#else
   return 0;
#endif
}


#ifndef PERL_SUPPORTS_UTF8
UV
prima_utf8_to_uv( U8 * utf8, STRLEN * len)
{
   *len = 1;
   return (UV) *utf8;
}

U8 *
prima_uv_to_utf8( U8 * utf8, UV uv)
{
   *(utf8++) = ( U8) uv;
   return utf8;
}
#endif

#ifdef PARANOID_MALLOC
#undef malloc
#undef free
#if PRIMA_PLATFORM != apcUnix
#define HAVE_FTIME
#endif

#include <sys/timeb.h>

static PHash hash = nil;
Handle self = 0;

static unsigned long
timestamp( void)
{
#ifdef HAVE_FTIME
   struct timeb t;
   ftime( &t);
   return t. time * 1000 + t. millitm;
#else
   struct timeval t;
   struct timezone tz;
   gettimeofday( &t, &tz);
   return t.tv_sec * 1000 + t.tv_usec;
#endif
}

static void
output_mallocs( void)
{
   HE *he;
   DOLBUG( "=========================== Reporing heap problems ===========================\n");
   hv_iterinit( hash);
   DOLBUG( "Iteration done...\n");
   while (( he = hv_iternext( hash)) != nil) {
      DOLBUG( "%s\n", (char *)HeVAL( he));
      free( HeVAL( he));
      HeVAL( he) = &PL_sv_undef;
   }
   DOLBUG( "=========================== Report done ===========================\n");
   sv_free(( SV *) hash);
}

void *
_test_malloc( size_t size, int ln, char *fil, Handle self)
{
   void *mlc;
   char s[512];
   char obj[ 256];
   char *c1, *c2;

   if (!hash) {
       hash = hash_create();
   }
   mlc = malloc( size);
   c1 = strrchr( fil, '/');
   c2 = strrchr( fil, '\\');
   if (c1<c2) c1 = c2;
   if (c1>0) fil = c1+1;
   if (self && hash_fetch( primaObjects, &self, sizeof(self))) {
      if ( kind_of( self, CComponent) && (( PComponent) self)-> name)
         sprintf( obj, "%s(%s)", ((( PObject) self)-> self)-> className, (( PComponent) self)-> name);
      else
         sprintf( obj, "%s(?)", ((( PObject) self)-> self)-> className);
   } else
      strcpy( obj, "NOSELF");
   hash_store( hash, &mlc, sizeof(mlc),
      strcpy( malloc( 1 + sprintf( s, "%lu %p %s(%d) %s %lu", timestamp(), mlc, fil, ln, obj, ( unsigned long) size)), s)
   );
   return mlc;
}

void *
_test_realloc( void * ptr, size_t size, int ln, char *fil, Handle self)
{
   void * nptr = _test_malloc( size, ln, fil, self);
   if ( nptr == NULL) return NULL;
   memcpy( nptr, ptr, size);
   _test_free( ptr, ln, fil, self);
   return nptr;
}

void
_test_free( void *ptr, int ln, char *fil, Handle self)
{
   free( ptr);
   hash_delete( hash, &ptr, sizeof(ptr), false);
}

/* to make freaking Windows happy */
#if PRIMA_PLATFORM != apcUnix

#undef list_create
#undef plist_create

void list_create( PList slf, int size, int delta) {}
PList plist_create( int size, int delta) {}
#endif

#endif /* PARANOID_MALLOC */

#if PRIMA_PLATFORM != apcUnix
int
debug_write( const char *format, ...)
{
   FILE *f;
   int rc;
   va_list arg_ptr;

   if( ( f = ( FILE*) fopen( "C:\\PRIMAERR.LOG", "at")) == NULL) {
       return false;
   }
   va_start( arg_ptr, format);
   rc = vfprintf( f, format, arg_ptr);
   va_end( arg_ptr);
   fclose( f);

   return ( rc != EOF);
}
#else
int
debug_write( const char *format, ...)
{
    int rc = 0;
    if ( dolbug) {
        va_list args;
        va_start( args, format);
        rc = vfprintf( stderr, format, args);
        va_end( args);
    }
    return rc;
}
#endif

#ifdef __cplusplus
}
#endif
