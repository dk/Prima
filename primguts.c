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
/* Guts library, main file */
#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#ifdef __unix
#include <floatingpoint.h>
#include <sys/types.h>
#endif /* __unix */
#include <dirent.h>
#define GENERATE_TABLE_GENERATOR yes
#include "apricot.h"
#include "guts.h"
#include "Object.h"
#include "Component.h"
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


#define USE_MAGICAL_STORAGE 0

#define MODULE "Prima Guts"

#include "Types.inc"
#include "thunks.tinc"

long   apcError = 0;

static PerlInterpreter *myPerl;
static char evaluation[ 4096];
static PHash vmtHash = nil;
static List  staticObjects;

char *
duplicate_string( const char *s)
{
   int l = strlen( s) + 1;
   char *d = malloc( l);
   memcpy( d, s, l);
   return d;
}

#ifdef PRIMA_NEED_OWN_STRICMP
int
stricmp(const char *s1, const char *s2)
{
   /* Code was taken from FreeBSD 4.0 /usr/src/lib/libc/string/strcasecmp.c */
   const unsigned char *u1 = (const unsigned *)s1;
   const unsigned char *u2 = (const unsigned *)s2;
   while (tolower(*u1) == tolower(*u2++))
      if (*u1++ == '\0')
         return 0;
   return (tolower(*u1) - tolower(*--u2));
}
#endif

PHash primaObjects = nil;

#ifdef PERL_CALL_SV_DIE_BUG_AWARE
I32
clean_perl_call_method( char* methname, I32 flags)
{
   I32 ret;
   ret = perl_call_method( methname, flags | G_EVAL);
   if ( !(SvTRUE( GvSV( errgv))))
      return ret;
   if (( flags & (G_SCALAR|G_DISCARD|G_ARRAY)) == G_SCALAR)
   {
      dSP;
      SPAGAIN;
      (void)POPs;
   }
   croak( SvPV( GvSV( errgv), na));
   return ret;
}

I32
clean_perl_call_pv( char* subname, I32 flags)
{
   I32 ret;
   ret = perl_call_pv( subname, flags | G_EVAL);
   if ( !(SvTRUE( GvSV( errgv))))
      return ret;
   if (( flags & (G_SCALAR|G_DISCARD|G_ARRAY)) == G_SCALAR)
   {
      dSP;
      SPAGAIN;
      (void)POPs;
   }
   croak( SvPV( GvSV( errgv), na));
   return ret;
}
#endif

#if ( PERL_PATCHLEVEL == 4)
SV* readonly_clean_sv_2mortal( SV* sv) {
   if ( SvREADONLY( sv))
      return sv;
   return Perl_sv_2mortal( sv);
}
#else
// We should never call this;  but we define it in order to make .def files happy
SV* readonly_clean_sv_2mortal( SV* sv) {
   return Perl_sv_2mortal( sv);
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
   Handle self;
   char *className;
   PVMT vmt;

   // finding the vmt
   className = HvNAME( SvSTASH( SvRV( perlObject))); if ( !className) return 0;
   vmt = gimme_the_vmt( className); if ( !vmt) return 0;

   // allocating an instance
   object = malloc( vmt-> instanceSize); memset( object, 0, vmt-> instanceSize);
   object-> self = (void*)vmt; object-> super = (void*)vmt-> super;

   if (USE_MAGICAL_STORAGE)
   {
      // assigning the tilde-magic
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
      // another scheme, uses hash slot
      hv_store( (HV*)SvRV( perlObject), "__CMATE__", 9, newSViv((IV)object), 0);
   }

   // extra check
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

XS( Object_alive_FROMPERL)
{
   dXSARGS;
   Handle _c_apricot_self_;
   int ret;

   if ( items != 1)
      croak("Invalid usage of Object::%s", "alive");
   _c_apricot_self_ = gimme_the_real_mate( ST( 0));
   if ( _c_apricot_self_ == nilHandle)
      croak( "Illegal object reference passed to Object::%s", "alive");
   SPAGAIN;
   SP -= items;

   switch ((( PObject) _c_apricot_self_)-> stage) {
   case csConstructing:
       ret = 2;
       break;
   case csNormal:
       ret = 1;
       break;
   default:
       ret = 0;
   }
   XPUSHs( sv_2mortal( newSViv( ret)));
   PUTBACK;
   return;
}

XS( create_from_Perl)
{
   dXSARGS;
   if (( items - 2 + 1) % 2 != 0)
      croak("Invalid usage of Object::create");
   {
      Handle  _c_apricot_res_;
      HV *hv = parse_hv( ax, sp, items, mark, 2 - 1, "Object_create");
      _c_apricot_res_ = Object_create(
         ( char*) SvPV( ST( 0), na),
         hv
      );
      SPAGAIN;
      SP -= items;
      if ( _c_apricot_res_ && (( PAnyObject) _c_apricot_res_)-> mate && (( PAnyObject) _c_apricot_res_)-> mate != nilSV)
      {
         XPUSHs( sv_mortalcopy((( PAnyObject) _c_apricot_res_)-> mate));
         --SvREFCNT( SvRV((( PAnyObject) _c_apricot_res_)-> mate));
      } else XPUSHs( &sv_undef);
      // push_hv( ax, sp, items, mark, 1, hv);
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
      croak ("Invalid usage of Object::destroy");
   self = gimme_the_real_mate( ST( 0));
   if ( self == nilHandle)
      croak( "Illegal object reference passed to Object::destroy");
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
         (( PAnyObject) self)-> killPtr = killChain;
         killChain = ( PAnyObject) self;
      }
   }
   XSRETURN_EMPTY;
}

XS(ext_std_print)
{
   int isError;
   int i;
   char buf[ 2048];
   char *c = buf;
   dXSARGS;

   strcpy( buf, "");
   if ( items >= 2)
   {
      isError = SvTRUE( SvRV( ST( 0)));
      if ( isError)
      {
         // f = stderr;
         // fprintf( f, "STDERR: ");
      }
      else
      {
         // f = stdout;
         // fprintf( f, "STDOUT: ");
      }
      for ( i = 1; i < items; i++)
      {
         // fprintf( f, "%s", SvPV( ST( i), na));
         snprintf( c, 2048, "%s", SvPV( ST( i), na));
         c += strlen( c);
      }
      fprintf( stderr, "%s", buf);
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

Bool
build_dynamic_vmt( void *vvmmtt, const char *ancestorName, int ancestorVmtSize)
{
   PVMT vmt = vvmmtt;
   PVMT ancestorVmt = gimme_the_vmt( ancestorName);
   int i, n;
   void **to, **from;

   if ( ancestorVmt == nil)
   {
      log_write( "GUTS001: Cannot locate base class \"%s\" of class \"%s\"\n", ancestorName, vmt-> className);
      return false;
   }
   if ( ancestorVmt-> base != ancestorVmt-> super)
   {
      log_write( "GUTS002: Cannot inherit C-class \"%s\" from Perl-class \"%s\"\n", vmt-> className, ancestorName);
      return false;
   }

   vmt-> base = vmt-> super = ancestorVmt;
   n = (ancestorVmtSize - sizeof(VMT)) / sizeof( void *);
   from = (void **)((char *)ancestorVmt + sizeof(VMT));
   to = (void **)((char *)vmt + sizeof(VMT));
   for ( i = 0; i < n; i++) if ( to[i] == nil) to[i] = from[i];
   build_static_vmt( vmt);
   return true;
}

void
build_static_vmt( void *vvmmtt)
{
   PVMT vmt = vvmmtt;
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

   // Check whether this class has been already built...
   vmtAddr = hash_fetch( vmtHash, (char *)className, strlen( className));
   if ( vmtAddr != nil) return ( PVMT) vmtAddr;

   // No;  try to find inherited VMT...
   stash = gv_stashpv( (char *)className, false);
   if ( stash == nil)
   {
      croak( "GUTS003: Cannot locate package %s\n", className);
      return nil;     // Definitely wrong!
   }

   isaGlob = hv_fetch( stash, "ISA", 3, 0);
   if (! (( isaGlob == nil) ||
          ( *isaGlob == nil) ||
          ( !GvAV(( GV *) *isaGlob)) ||
          ( av_len( GvAV(( GV *) *isaGlob)) < 0)
      ))
   {
      // ISA found!
      inheritedName = av_fetch( GvAV(( GV *) *isaGlob), 0, 0);
      if ( inheritedName != nil)
         originalVmt = gimme_the_vmt( SvPV( *inheritedName, na));
      else
         return nil;    // The error message will be printed by the previous incarnation
   }
   if ( !originalVmt)
   {
      croak( "GUTS005: Error finding ancestor's VMT for %s\n", className);
      return nil;
   }
   // Do we really need to do this?
   if ( strEQ( className, originalVmt-> className))
      return originalVmt;

#ifdef PARANOID_MALLOC
   debug_write( "%lu Dynamic vmt creation (%d) for %s\n", timestamp(), originalVmt-> vmtSize, className);
#endif
   vmtSize = originalVmt-> vmtSize;
   vmt = malloc( vmtSize);
   memcpy( vmt, originalVmt, vmtSize);
   newClassName = malloc( strlen( className) + 1);
   strcpy( newClassName, className);
   vmt-> className = newClassName;
   vmt-> base = originalVmt;

   // Not particularly effective now...
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
               addr = ( void *)((( char *)vmt) + ((( char *)( patch[ i]. vmtAddr)) - (( char *)patchWhom)));
               *addr = patch[ i]. procAddr;
            }
         }
      }
      patchWhom = patchWhom-> base;
   }

   // Store newly created vmt into our hash...
   hash_store( vmtHash, (char *)className, strlen( className), vmt);
   list_add( &staticObjects, (Handle) vmt);
   list_add( &staticObjects, (Handle) vmt-> className);

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
delegate_sub( Handle self, char *methodName, const char *format, ...)
{
   SV *toReturn;
   PComponent obj = ( PComponent) self;
   HV * hv   = ( HV*) SvRV( *hv_fetch(( HV*) SvRV( obj-> mate), "__DELEGATORS__", 14, 0));
   SV ** sub = hv_fetch( hv, methodName, strlen( methodName), 0);
   va_list params;

   if ( sub == nil) croak("GUTS015: Inconsistent __DELEGATORS__ storage in %s for %s", obj-> name, methodName);
   va_start( params, format);
   toReturn = call_perl_indirect(( Handle)(( PComponent) obj-> delegateTo)-> mate,
      ( char*)SvRV( *sub),
      format, false, true, params);
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
         return &sv_undef;
   }

   if ( format[ 0] == '<')
   {
      format += 1;
      returns = true;
   }

   // Parameter check
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
         retCount = ( coderef) ?
            perl_call_sv(( SV *) subName, G_SCALAR|G_EVAL) :
            perl_call_method( subName, G_SCALAR|G_EVAL);
         SPAGAIN;
         if ( SvTRUE( GvSV( errgv)))
         {
            (void)POPs;
            croak( SvPV( GvSV( errgv), na));    // propagate
         }
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
         if ( coderef) perl_call_sv(( SV *) subName, G_DISCARD|G_EVAL);
            else perl_call_method( subName, G_DISCARD|G_EVAL);
         if ( SvTRUE( GvSV( errgv)))
         {
            croak( SvPV( GvSV( errgv), na));    // propagate
         }
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
      // check the validity of a key
      if (!( SvPOK( ST( i)) && ( !SvROK( ST( i)))))
         croak( "GUTS011: Illegal value for a profile key (argument #%d) passed to ``%s''", i, methodName);
      // and add the pair
      he = hv_store_ent( hv, ST( i), newSVsv( ST( i+1)), 0);
      av_push( order, newSVsv( ST( i)));
   }
   hv_store( hv, "__ORDER__", 9, newRV_noinc((SV *)order), 0);
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
      // XSRETURN( callerReturns);
   }

   rorder = hv_fetch( hv, "__ORDER__", 9, 0);
   if ( rorder != nil && *rorder != nil && SvROK( *rorder) && SvTYPE(SvRV(*rorder)) == SVt_PVAV) {
      int i, l;
      AV *order = (AV*)SvRV(*rorder);
      SV **key;

      n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
      n--; EXTEND( sp, n*2);

      // push everything in proper order
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

   // Calculate the length of our hv
   n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
   EXTEND( sp, n*2);

   // push everything
   hv_iterinit( hv);
   while (( he = hv_iternext( hv)) != nil)
   {
      PUSHs( sv_2mortal( newSVsv( hv_iterkeysv( he))));
      PUSHs( sv_2mortal( newSVsv( HeVAL( he))));
   }
   sv_free(( SV *) hv);
   PUTBACK;
   return;
   // XSRETURN( callerReturns + n*2);
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

      // push everything in proper order
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

   // Calculate the length of our hv
   n = 0; hv_iterinit( hv); while ( hv_iternext( hv) != nil) n++;
   EXTEND( sp, n*2);

   // push everything
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
      hv_store_ent( hv, k, newSVsv( v), 0);
      av_push( order, newSVsv( k));
   }
   hv_store( hv, "__ORDER__", 9, newRV_noinc((SV *)order), 0);
   return expected;
}


static Bool
kill_objects( void * item, int keyLen, Handle * self, void * dummy)
{
   Object_destroy( *self);
   return false;
}

Bool appDead = false;
SV** temporary_prf_Sv;

/* Dynamic loading support */

extern void * dlsym(void *handle, char *symbol);
extern char * dlerror(void);

#ifndef RTLD_LAZY
# define RTLD_LAZY 1    /* Solaris 1 */
#endif

#ifndef HAS_DLERROR
# ifdef __NetBSD__
#  define dlerror() strerror(errno)
# else
#  define dlerror() "Unknown error - dlerror() not implemented"
# endif
#endif

/* Stolen from dlutils.c: */

/* pointer to allocated memory for last error message */
static char *LastError  = (char*)NULL;

/* flag for immediate rather than lazy linking (spots unresolved symbol) */
static int dl_nonlazy = 0;

#ifdef DL_LOADONCEONLY
static HV *dl_loaded_files = Nullhv;    /* only needed on a few systems */
#endif


#ifdef DEBUGGING
static int dl_debug = 0;        /* value copied from $DynaLoader::dl_error */
#define DLDEBUG(level,code)     if (dl_debug>=level) { code; }
#else
#define DLDEBUG(level,code)
#endif


static void
dl_generic_private_init()       /* called by dl_*.xs dl_private_init() */
{
    char *perl_dl_nonlazy;
#ifdef DEBUGGING
    dl_debug = SvIV( perl_get_sv("DynaLoader::dl_debug", 0x04) );
#endif
    if ( (perl_dl_nonlazy = getenv("PERL_DL_NONLAZY")) != NULL )
        dl_nonlazy = atoi(perl_dl_nonlazy);
    if (dl_nonlazy)
        DLDEBUG(1,PerlIO_printf(PerlIO_stderr(), "DynaLoader bind mode is 'non-lazy'\n"));
#ifdef DL_LOADONCEONLY
    if (!dl_loaded_files)
        dl_loaded_files = newHV(); /* provide cache for dl_*.xs if needed */
#endif
}


/* SaveError() takes printf style args and saves the result in LastError */
#ifdef STANDARD_C
static void
SaveError(char* pat, ...)
#else
/*VARARGS0*/
static void
SaveError(pat, va_alist)
    char *pat;
    va_dcl
#endif
{
    va_list args;
    char *message;
    int len;

    /* This code is based on croak/warn, see mess() in util.c */

// #define saferealloc realloc
// #define safemalloc malloc

#ifdef I_STDARG
    va_start(args, pat);
#else
    va_start(args);
#endif
    message = mess(pat, &args);
    va_end(args);

    len = strlen(message) + 1 ; /* include terminating null char */

    /* Allocate some memory for the error message */
    if (LastError)
        LastError = (char*)saferealloc(LastError, len) ;
    else
        LastError = (char *) safemalloc(len) ;

    /* Copy message into LastError (including terminating null char)    */
    strncpy(LastError, message, len) ;
    DLDEBUG(2,PerlIO_printf(PerlIO_stderr(), "DynaLoader: stored error msg '%s'\n",LastError));
}

static void
dl_private_init()
{
    (void)dl_generic_private_init();
}

XS(XS_DynaLoader_dl_load_file)
{
    dXSARGS;
    if (items < 1 || items > 2)
        croak("Usage: DynaLoader::dl_load_file(filename, flags=0)");
    {
        char *  filename = (char *)SvPV(ST(0),na);
        int     flags;
    int mode = RTLD_LAZY;
        void *  RETVAL;

        if (items < 2)
            flags = 0;
        else {
            flags = (int)SvIV(ST(1));
        }
#ifdef RTLD_NOW
    if (dl_nonlazy)
        mode = RTLD_NOW;
#endif
    if (flags & 0x01)
#ifdef RTLD_GLOBAL
        mode |= RTLD_GLOBAL;
#else
        warn("Can't make loaded symbols global on this platform while loading %s",filename);
#endif
    DLDEBUG(1,PerlIO_printf(PerlIO_stderr(), "dl_load_file(%s,%x):\n", filename,flags));
    RETVAL = apc_dlopen(filename, mode) ;
    DLDEBUG(2,PerlIO_printf(PerlIO_stderr(), " libref=%lx\n", (unsigned long) RETVAL));
    ST(0) = sv_newmortal() ;
    if (RETVAL == NULL)
        SaveError("%s",dlerror()) ;
    else
        sv_setiv( ST(0), (IV)RETVAL);
    }
    XSRETURN(1);
}

XS(XS_DynaLoader_dl_find_symbol)
{
    dXSARGS;
    if (items != 2)
        croak("Usage: DynaLoader::dl_find_symbol(libhandle, symbolname)");
    {
        void *  libhandle = (void *)SvIV(ST(0));
        char *  symbolname = (char *)SvPV(ST(1),na);
        void *  RETVAL;
#ifdef DLSYM_NEEDS_UNDERSCORE
    symbolname = form("_%s", symbolname);
#endif
    DLDEBUG(2, PerlIO_printf(PerlIO_stderr(),
                             "dl_find_symbol(handle=%lx, symbol=%s)\n",
                             (unsigned long) libhandle, symbolname));
    RETVAL = dlsym(libhandle, symbolname);
    DLDEBUG(2, PerlIO_printf(PerlIO_stderr(),
                             "  symbolref = %lx\n", (unsigned long) RETVAL));
    ST(0) = sv_newmortal() ;
    if (RETVAL == NULL)
        SaveError("%s",dlerror()) ;
    else
        sv_setiv( ST(0), (IV)RETVAL);
    }
    XSRETURN(1);
}

XS(XS_DynaLoader_dl_undef_symbols)
{
    dXSARGS;
    (void)ax;
    if (items != 0)
        croak("Usage: DynaLoader::dl_undef_symbols()");
    SP -= items;
    {
        PUTBACK;
        return;
    }
}

XS(XS_DynaLoader_dl_install_xsub)
{
    dXSARGS;
    if (items < 2 || items > 3)
        croak("Usage: DynaLoader::dl_install_xsub(perl_name, symref, filename=\"$Package\")");
    {
        char *  perl_name = (char *)SvPV(ST(0),na);
        void *  symref = (void *)SvIV(ST(1));
        char *  filename;

        if (items < 3)
            filename = "DynaLoader";
        else {
            filename = (char *)SvPV(ST(2),na);
        }
    ST(0)=sv_2mortal(newRV((SV*)newXS(perl_name, (void(*)( struct cv*))symref, filename)));
    }
    XSRETURN(1);
}

XS(XS_DynaLoader_dl_error)
{
    dXSARGS;
    if (items != 0)
        croak("Usage: DynaLoader::dl_error()");
    {
        char *  RETVAL;
    RETVAL = LastError ;
        ST(0) = sv_newmortal();
        sv_setpv((SV*)ST(0), RETVAL);
    }
    XSRETURN(1);
}

#ifdef __cplusplus
extern "C"
#endif
XS(boot_DynaLoader)
{
    dXSARGS;
    char* file = __FILE__;

   (void)items;

    XS_VERSION_BOOTCHECK ;

        newXS("DynaLoader::dl_load_file", XS_DynaLoader_dl_load_file, file);
        newXS("DynaLoader::dl_find_symbol", XS_DynaLoader_dl_find_symbol, file);
        newXS("DynaLoader::dl_undef_symbols", XS_DynaLoader_dl_undef_symbols, file);
        newXS("DynaLoader::dl_install_xsub", XS_DynaLoader_dl_install_xsub, file);
        newXS("DynaLoader::dl_error", XS_DynaLoader_dl_error, file);

    /* Initialisation Section */

    (void)dl_private_init();


    /* End of Initialisation Section */

    ST(0) = &sv_yes;
    XSRETURN(1);
}


static void
xs_init()
{
   char *file = __FILE__;
   dXSUB_SYS;
   newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

Bool waitBeforeQuit;
extern void dump_logger(void);

#if defined(BROKEN_COMPILER) || defined(__unix)
double NAN;
#endif

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

XS( prima_cleanup)
{
   dXSARGS;
   (void)items;

   appDead = true;
   hash_first_that( primaObjects, kill_objects, nil, nil, nil);
   hash_destroy( primaObjects, false);
   primaObjects = nil;
   window_subsystem_cleanup();
   hash_destroy( vmtHash, false);
   list_delete_all( &staticObjects, true);
   list_destroy( &staticObjects);
   kill_zombies();
   window_subsystem_done();
#ifdef PARANOID_MALLOC
   output_mallocs();
#endif

   ST(0) = &sv_yes;
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
   register_fs_constants();
   register_fw_constants();
   register_bi_constants();
   register_bs_constants();
   register_ws_constants();
   register_sv_constants();
   register_im_constants();
   register_ict_constants();
   register_is_constants();
   register_apc_constants();
   register_gui_constants();
   register_dt_constants();
   register_cr_constants();
   register_sbmp_constants();
   register_hmp_constants();
   register_tw_constants();
   register_fds_constants();
   register_fdo_constants();
}

XS( boot_Prima)
{
   dXSARGS;
   (void)items;

fprintf( stderr, "boot_Prima\n");
   XS_VERSION_BOOTCHECK;

#ifdef BROKEN_COMPILER
   {
      union {U8 c[8];double d;} nan = {{00, 00, 00, 00, 00, 00, 0xf8, 0xff}};
      NAN = nan. d;
   }
#endif /* BROKEN_COMPILER */
#ifdef __unix
   {
      /* What we actually need here is not unix */
      /* We need the control over mathematical exceptions, that's it */
      double zero; /* ``volatile'' to cheat clever optimizers */
      zero = 0.0;
      // fpresetsticky(FP_X_INV|FP_X_DZ);
      // fpsetmask(~(FP_X_INV|FP_X_DZ));
      // NAN = 0.0 / zero;
NAN = 0.0;
      // fpresetsticky(FP_X_INV|FP_X_DZ);
      // fpsetmask(FP_X_INV|FP_X_DZ);
   }
#endif /* __unix */

   if ( !window_subsystem_init()) {
      apc_show_message( "Error initializing PRIMA");
      ST(0) = &sv_no;
      XSRETURN(1);
   };
   primaObjects = hash_create();
   vmtHash      = hash_create();
   list_create( &staticObjects, 16, 16);

   /* register hard coded XSUBs */
   newXS( "TiedStdOut::PRINT", ext_std_print, MODULE);
   newXS( "::destroy_mate", destroy_mate, MODULE);
   newXS( "Prima::cleanup", prima_cleanup, "Prima");
   newXS( "Utils::getdir", Utils_getdir_FROMPERL, "Utils");
   /* register built-in classes */
   newXS( "Object::create",  create_from_Perl, "Object");
   newXS( "Object::destroy", destroy_from_Perl, "Object");
   newXS( "Object::alive", Object_alive_FROMPERL, "Object");
   register_constants();
   register_Object_Class();
   register_Utils_Package();
   register_Component_Class();
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

   ST(0) = &sv_yes;
   XSRETURN(1);
}

int
prima( const char *primaPath, int argc, char **argv)
{
   char *pargv[256] = { "", ""};
   int pargc = 2;
   SV *sv;
   char *mainModule = "UserInit.pm";
   char incpath[1024];
   dJMPENV;
   int jumpRet;
   I32 oldScope;

#ifdef BROKEN_COMPILER
   {
      union {U8 c[8];double d;} nan = {{00, 00, 00, 00, 00, 00, 0xf8, 0xff}};
      NAN = nan. d;
   }
#endif /* BROKEN_COMPILER */
#ifdef __unix
   {
      /* What we actually need here is not unix */
      /* We need the control over mathematical exceptions, that's it */
      volatile double zero = 0.0; /* ``volatile'' to cheat clever optimizers */
      fpsetmask(~FP_X_INV);
      NAN = 0.0 / zero;
      fpresetsticky(FP_X_INV);
      fpsetmask(FP_X_INV);
   }
#endif /* __unix */

   if ( !window_subsystem_init()) {
      apc_show_message( "Error initializing PRIMA");
      return apcError;
   };

   fprintf( stderr, "*** PRIMA Engine started ***\n");
   snprintf( incpath, 1024, "-I%s;%sScript;%smodules",
      getenv( "PRIMA_ADDITIONAL_PATH"),
      primaPath, primaPath);
   pargv[1] = incpath;
   // startup @INC patch

   {
      int i;
      Bool wasModule = false;
      for ( i = 1; i < argc; i++) {
         if (!argv[i] || strlen(argv[i]) <= 0) continue;
         if (!wasModule && argv[i][0] == '-') {
            // our option
            if (strcmp(argv[i], "-p") == 0) {
               pargv[pargc++] = "-d:DProf";
            } else if ( strncmp( argv[i], "-I", 2) == 0) {
               pargv[pargc++] = argv[i];
            } else {
               fprintf( stderr, "Unrecognized option \"%s\" passed on startup\n", argv[i]);
               fprintf( stderr, "Currently the only option allowed is \"-p\"\n");
               apc_show_message( "Unrecognized option passed on startup.\nSee Prima Log for details");
               goto NoPerlDying;
            }
         } else if (!wasModule) {
            // module names
            mainModule = argv[i];
            wasModule = true;
            pargv[pargc++] = "-we";
            pargv[pargc++] = "sub __{}__;";
         } else {
            // module parameters
            pargv[pargc++] = argv[i];
         }
      }
      if (!wasModule) {
         pargv[pargc++] = "-we";
         pargv[pargc++] = "sub __{}__;";
      }
   }

#ifdef __EMX__
   {
      char *e = "";
      Perl_OS2_init( &e);
   }
#endif
   myPerl = perl_alloc();
   perl_construct( myPerl);
#ifdef __EMX__
   _fpreset();              // Perl & PM behave strangely without this
#endif
   perl_parse( myPerl, (void(*)(void))xs_init, pargc, pargv, NULL);

   primaObjects = hash_create();
   vmtHash      = hash_create();
   list_create( &staticObjects, 16, 16);

   { // Construct @INC
     char *additionalPath = getenv( "PRIMA_ADDITIONAL_PATH");
     char *inc = evaluation + sprintf( evaluation, "use lib (");
     char *tok = nil;
     if (additionalPath) tok = strtok( additionalPath, ";");
     while ( tok)
     {
        inc += sprintf( inc, "'%s', ", tok);
        tok = strtok( nil, ";");
     }
     sprintf( inc, "'%sScript', '%smodules'); 1;", primaPath, primaPath);
     fprintf( stderr, "|%s|\n", evaluation);
     sv = eval( evaluation);
   }

   // register hard coded XSUBs
   newXS( "TiedStdOut::PRINT", ext_std_print, MODULE);
   newXS( "::destroy_mate", destroy_mate, MODULE);
   newXS( "Utils::getdir", Utils_getdir_FROMPERL, "Utils");
   // register built-in classes
   newXS( "Object::create",  create_from_Perl, "Object");
   newXS( "Object::destroy", destroy_from_Perl, "Object");
   newXS( "Object::alive", Object_alive_FROMPERL, "Object");
   register_constants();
   register_Object_Class();
   register_Utils_Package();
   register_Component_Class();
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

   sprintf( evaluation, "# Prima Guts\n# line 1 \"Prima Guts\"\nuse Prima; &Prima::prima(\'%s\');", mainModule ? mainModule : "UserInit.pm");

   oldScope = PRIMAPERL_scopestack_ix;
   JMPENV_PUSH( jumpRet);
   switch( jumpRet)
   {
       case 1:
          /* fall thru */
          STATUS_ALL_FAILURE;
       case 2:
          /* my_exit() was called */
          while ( PRIMAPERL_scopestack_ix >= oldScope) LEAVE;
          FREETMPS;
          PRIMAPERL_curstash = PRIMAPERL_defstash;
          if ( PRIMAPERL_endav) call_list( oldScope, PRIMAPERL_endav);
          JMPENV_POP;
          goto DieHard;
       case 3:
          JMPENV_POP;
          apc_show_message("Panic: topenv");
          goto DieHard;
   }
   sv = eval( evaluation);
   JMPENV_POP;

   appDead = true;
   window_subsystem_cleanup();
   if ( SvTRUE( sv))
      fprintf( stderr, "*** Seems to be OK ***\n");
   else
   {
      fprintf( stderr, "*** %s\n", SvPV(GvSV( errgv), na));
      apc_show_message("Abnormal termination\nSee Prima Log for details");
      goto NoPerlDying;
   }
   perl_run( myPerl);
   hash_first_that( primaObjects, kill_objects, nil, nil, nil);
   hash_destroy( primaObjects, false);
   primaObjects = nil;
   perl_destruct( myPerl);
   hash_destroy( vmtHash, false);
   list_delete_all( &staticObjects, true);
   list_destroy( &staticObjects);
DieHard:
   appDead = true;
   window_subsystem_cleanup();
   if (waitBeforeQuit)
      apc_show_message( "I was asked to wait before quitting...");
NoPerlDying:
   kill_zombies();
   window_subsystem_done();
#ifdef PARANOID_MALLOC
   if (myPerl) output_mallocs();
#endif
   perl_free( myPerl);
   return 0;
}

typedef struct _RemapHashNode_ {
   int key;
   int val;
   struct _RemapHashNode_ *next;
} RemapHashNode, *PRemapHashNode;

typedef struct _RemapHash_ {
   PRemapHashNode table[1];
} RemapHash, *PRemapHash;

int
ctx_remap_def( int value, int *table, Bool direct, int default_value)
{
   register PRemapHash hash;
   register PRemapHashNode node;

   if ( table == nil) return default_value;
   if ( table[0] != endCtx) {
      /* Hash was not built before;  building */
      int *tbl;
      PRemapHash hash1, hash2;
      PRemapHashNode next;
      int sz = 0;

      tbl = table;
      while ((*tbl) != endCtx) {
        tbl += 2;
              sz++;
      }

      /* First way build hash */
      hash = malloc( sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1) + sizeof( RemapHashNode) * sz);
      memset( hash, 0, sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
      tbl = table;
      next = (void *)(((char *)hash) + sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
      while ((*tbl) != endCtx) {
              int key = (*tbl)&0x1F;
              if (hash->table[key]) {
                 /* Already exists something */
                 node = hash->table[key];
                 while ( node-> next) node = node-> next;
                 // node->next = malloc( sizeof( RemapHashNode));
                 node->next = next++;
                 node->next-> key = tbl[0];
                 node->next-> val = tbl[1];
                 node->next-> next = nil;
              } else {
                 // hash->table[key] = malloc( sizeof( RemapHashNode));
                 hash->table[key] = next++;
                 hash->table[key]-> key = tbl[0];
                 hash->table[key]-> val = tbl[1];
                 hash->table[key]-> next = nil;
              }
              tbl += 2;
      }
      hash1 = hash;

      /* Second way build hash */
      hash = malloc( sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1) + sizeof( RemapHashNode) * sz);
      memset( hash, 0, sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
      tbl = table;
      next = (void *)(((char *)hash) + sizeof(RemapHash) + sizeof( PRemapHashNode) * (32-1));
      while ((*tbl) != endCtx) {
              int key = tbl[1]&0x1F;
              if (hash->table[key]) {
                 /* Already exists something */
                 node = hash->table[key];
                 while ( node-> next) node = node-> next;
                 // node->next = malloc( sizeof( RemapHashNode));
                 node->next = next++;
                 node->next-> key = tbl[1];
                 node->next-> val = tbl[0];
                 node->next-> next = nil;
              } else {
                 // hash->table[key] = malloc( sizeof( RemapHashNode));
                 hash->table[key] = next++;
                 hash->table[key]-> key = tbl[1];
                 hash->table[key]-> val = tbl[0];
                 hash->table[key]-> next = nil;
              }
              tbl += 2;
      }
      hash2 = hash;
      table[0] = endCtx;
      table[1] = (int)hash1;
      table[2] = (int)hash2;
      list_add( &staticObjects, ( Handle) hash1);
      list_add( &staticObjects, ( Handle) hash2);
   }

   hash = (void*)(direct ? table[1] : table[2]);
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
            hv_store( profile, s, strlen( s), newSViv(va_arg(params, int)), 0);
            break;
         case 's':
            hv_store( profile, s, strlen( s), newSVpv(va_arg(params, char *),0), 0);
            break;
         case 'n':
            hv_store( profile, s, strlen( s), newSVnv(va_arg(params, double)), 0);
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


XS(Utils_getdir_FROMPERL) {
   dXSARGS;
   Bool wantarray = ( GIMME_V == G_ARRAY);
   char *dirname;
   PList dirlist;
   int i;

   if ( items >= 2) {
      croak( "invalid usage of Utils::getdir");
   }
   dirname = SvPV( ST( 0), na);
   dirlist = apc_getdir( dirname);
   SPAGAIN;
   SP -= items;
   if ( wantarray) {
      if ( dirlist) {
         EXTEND( sp, dirlist-> count);
         for ( i = 0; i < dirlist-> count; i++) {
            PUSHs( sv_2mortal(newSVpv(( char *)dirlist-> items[i], 0)));
            free(( char *)dirlist-> items[i]);
         }
         plist_destroy( dirlist);
      }
   } else {
      if ( dirlist) {
         XPUSHs( sv_2mortal( newSViv( dirlist-> count / 2)));
         for ( i = 0; i < dirlist-> count; i++) {
            free(( char *)dirlist-> items[i]);
         }
         plist_destroy( dirlist);
      } else {
         XPUSHs( &sv_undef);
      }
   }
   PUTBACK;
   return;
}

// list section

#ifdef PARANOID_MALLOC
void
paranoid_list_create( PList slf, int size, int delta, char *fil, int ln)
{
   char *buf;
   int blen;
   if ( !slf) return;
   buf = malloc( blen = strlen( fil) + strlen( __FILE__) + 9);
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
   buf = malloc( blen = strlen( fil) + strlen( __FILE__) + 9);
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
   slf-> size  = size;
   slf-> items = ( size > 0) ? malloc( size * sizeof( Handle)) : nil;
}

PList
plist_create( int size, int delta)
{
   PList new_list = ( PList) malloc( sizeof( List));
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
      slf-> items = malloc(( slf-> size + slf-> delta) * sizeof( Handle));
      if ( old) {
         memcpy( slf-> items, old, slf-> size * sizeof( Handle));
         free( old);
      }
      slf-> size += slf-> delta;
   }
   slf-> items[ slf-> count++] = item;
   return slf-> count - 1;
}

void
list_insert_at( PList slf, Handle item, int pos)
{
   int max;
   Handle save;
   if ( list_add( slf, item) < 0) return;
   max = slf-> count - 1;
   if ( pos < 0 || pos >= max) return;
   save = slf-> items[ max];
   memmove( &slf-> items[ pos + 1], &slf-> items[ pos], ( max - pos) * sizeof( Handle));
   slf-> items[ pos] = save;
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
   list = malloc( slf-> count * sizeof( Handle));
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

#ifdef PERLY_HASH

PHash
hash_create( void)
{
   return newHV();
}

void
hash_destroy( PHash h, Bool killAll)
{
   HE *he;
   hv_iterinit( h);
   while (( he = hv_iternext( h)) != nil) {
      if ( killAll) free( HeVAL( he));
      HeVAL( he) = &sv_undef;
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
   HeVAL( he) = &sv_undef;
   hv_delete_ent( h, ksv, G_DISCARD, 0);
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
   if ( he) return false;
   he = hv_store_ent( h, ksv, &sv_undef, 0);
   HeVAL( he) = val;
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

long
hash_count( PHash hash)
{
   return HvKEYS(( HV*) hash);
}
#else  /* PERLY_HASH */

#define HASH_BUCKETS 33

typedef struct _HashNode_ {
   int   keyLen;
   void* key;
   void* value;
   struct _HashNode_ *next;
} HashNode, *PHashNode;

typedef struct _Hash_ {
   long count;
   PHashNode buckets[ HASH_BUCKETS];
} Hash;

PHash
hash_create( void)
{
   Hash * h = malloc( sizeof( Hash));
   if ( h) memset ( h, 0, sizeof( Hash));
   return ( PHash) h;
}

void
hash_destroy( PHash h, Bool killAll)
{
   long i;
   for ( i = 0; i < HASH_BUCKETS; i++) {
      PHashNode node = (( Hash *) h)-> buckets[ i];
      while ( node != nil) {
         PHashNode next = node-> next;
         free( node-> key);
         if ( killAll) free( node-> value);
         free( node);
         node = next;
      }
   }
   free( h);
}

static PHashNode
hash_findnode( Hash * h, const void *key, int keyLen, long * bucket)
{
   unsigned long i;
   PHashNode node;

   {
      unsigned long g;
      const char * data = key;
      int size = keyLen;
      i = 0;
      while ( size)  {
         i = ( i << 4) + *data++;
         if (( g = i & 0xF0000000))
            i ^= g >> 24;
         i &= ~g;
         size--;
      }
      i %= HASH_BUCKETS;
   }

   node = h-> buckets[ i];
   if ( bucket) *bucket = i;
   while ( node != nil) {
      if (( keyLen == node-> keyLen) && ( memcmp( key, node-> key, keyLen) == 0))
         return node;
      node = node-> next;
   }

   return nil;
}

Bool
hash_store( PHash h, const void *key, int keyLen, void *val)
{
   long i;
   PHashNode node = hash_findnode(( Hash *)h, key, keyLen, &i);
   if ( node != nil) {
      node-> value = val;
      return true;
   }
   node = malloc( sizeof( HashNode));
   if ( node == nil) return false;
   node-> key = malloc( keyLen);
   node-> keyLen = keyLen;
   if ( node-> key == nil) {
      free( node);
      return false;
   }
   memcpy( node-> key, key, keyLen);
   node-> value = val;
   node-> next  = (( Hash *)h)-> buckets[ i];
   (( Hash *)h)-> buckets[ i] = node;
   (( Hash *)h)-> count++;
   return true;
}

void *
hash_fetch( PHash h, const void *key, int keyLen)
{
   long i;
   PHashNode node = hash_findnode(( Hash *)h, key, keyLen, &i);
   if ( node != nil)
      return node-> value;
   return nil;
}

void *
hash_delete( PHash h, const void *key, int keyLen, Bool kill)
{
   long i;
   PHashNode node = hash_findnode(( Hash *)h, key, keyLen, &i);
   if ( node != nil) {
      void * val = node-> value;
      if ( node == (( Hash *)h)-> buckets[ i])
         (( Hash *)h)-> buckets[ i] = node-> next;
      else {
         PHashNode node2 = (( Hash *)h)-> buckets[ i];
         while ( node2-> next != node) node2 = node2-> next;
         node2-> next = node-> next;
      }
      free( node-> key);
      free( node);
      if ( kill) free( val);
      (( Hash *)h)-> count--;
      return val;
   }
   return nil;
}

long
hash_count( PHash h)
{
   return (( Hash *)h)-> count;
}

void *
hash_first_that( PHash h, void * action, void * params, int * pKeyLen, void ** pKey)
{
   long i, e = 0, entries = hash_count( h);
   void * ret = nil;
   PHashNode * array = malloc( sizeof( PHashNode) * entries);
   for ( i = 0; i < HASH_BUCKETS; i++) {
      PHashNode node = (( Hash *)h)-> buckets[ i];
      while ( node != nil) {
         array[ e++] = node;
         node = node-> next;
      }
   }
   for ( i = 0; i < entries; i++) {
      if ((( PHashProc) action)( array[i]->value,
            array[i]->keyLen, array[i]->key, params)) {
         if ( pKeyLen) *pKeyLen = array[i]-> keyLen;
         if ( pKey) *pKey = array[i]-> key;
         ret = array[i]-> value;
         break;
      }
   }
   free( array);
   return ret;
}

#endif /* PERLY_HASH */


#ifdef PARANOID_MALLOC
#undef malloc
#undef free
#undef list_create
#undef plist_create
#ifndef __unix
#define HAVE_FTIME
#endif
#ifndef PERLY_HASH
#error Cannot use non-perly hashes
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
      HeVAL( he) = &sv_undef;
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
   char *c;
   char *c1, *c2;

#ifndef __unix
   if (!myPerl) return malloc( size);
#endif
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
   sprintf( s, "%lu %p %s(%d) %s %lu", timestamp(), mlc, fil, ln, obj, ( unsigned long) size);
   c = malloc( strlen(s)+1);
   strcpy( c, s);
   hash_store( hash, &mlc, sizeof(mlc), c);
   return mlc;
}

void
_test_free( void *ptr, int ln, char *fil, Handle self)
{
   free( ptr);
   hash_delete( hash, &ptr, sizeof(ptr), true);
}

/* to make freaking Windows happy */
void list_create( PList slf, int size, int delta) {}
PList plist_create( int size, int delta) {}

#endif /* PARANOID_MALLOC */

