/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
 */
/* Created by Dmitry Karasik <dk@plab.ku.dk> */

#ifdef __unix
#include <unistd.h>
#else
#include <stdlib.h>
#endif 

#include "img.h"
#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static Bool initialized = false;   


void 
apc_img_init( void)
{
   if ( initialized) croak("Attempt to initialize image subsystem twice");
   list_create( &imgCodecs, 8, 8);
   initialized = true;
}   

#define CHK if ( !initialized) croak("Image subsystem is not initialized");

void 
apc_img_done( void)
{
   int i;
   
   CHK;   
   for ( i = 0; i < imgCodecs. count; i++) {
      PImgCodec c = ( PImgCodec)( imgCodecs. items[ i]);
      if ( c-> instance)
         c-> vmt-> done( c); 
      free( c);
   }
   list_destroy( &imgCodecs);
   initialized = false;
}   

Bool
apc_img_register( PImgCodecVMT codec, void * initParam)
{
   PImgCodec c; 

   CHK;
   if ( !codec) return false;
   c = ( PImgCodec) malloc( sizeof( struct ImgCodec) + codec-> size);
   if ( !c) return false;
   
   memset( c, 0, sizeof( struct ImgCodec));
   c-> vmt = ( PImgCodecVMT) ((( Byte *) c) + sizeof( struct ImgCodec));
   c-> initParam = initParam;
   memcpy( c-> vmt, codec, codec-> size);
   list_add( &imgCodecs, ( Handle) c);
   return true;
}   

static void * 
init( PImgCodecInfo * info, void * param)
{
   return nil;
}   

static void
done( PImgCodec instance) 
{
}   

static HV *   
defaults(  PImgCodec instance)
{
   return newHV();
}

static void   
check_in(  PImgCodec instance, HV * system, HV * user)
{
}

static void * 
open_load(  PImgCodec instance, PImgLoadFileInstance fi)
{
   return nil;
}

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
   return false;
}   

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   free( fi-> instance);  
}

static void * 
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   return nil;
}

static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   return false;
}   

static void 
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   free( fi-> instance);
}


List imgCodecs;
struct ImgCodecVMT CNullImgCodecVMT = {
  sizeof( struct ImgCodecVMT),
  init,
  done,
  defaults,
  check_in,
  open_load,
  load,
  close_load,
  defaults,
  check_in,
  open_save,
  save,
  close_save
};

void
apc_img_profile_add( HV * to, HV * from, HV * keys)
{
   HE *he;
   hv_iterinit(( HV*) keys);
   for (;;)
   {
      char *key;
      int  keyLen;
      SV ** holder;
      if (( he = hv_iternext( keys)) == nil)
         return;
      key    = (char*) HeKEY( he);
      keyLen = HeKLEN( he);
      if ( !hv_exists( from, key, keyLen))
         continue;
      holder = hv_fetch( from, key, keyLen, 0);
      if ( holder) 
         hv_store( to, key, keyLen, newSVsv( *holder), 0);
   }
}   

PList 
apc_img_load( Handle self, char * fileName, HV * profile, char * error)
{
   int i, profiles_len = 0, lastFrame = -2, codecID = -1;
   PList ret;
   PImgCodec c = nil;
   ImgLoadFileInstance fi;
   AV * profiles = nil;
   HV * def = nil, * firstObjectExtras = nil, * commonHV = nil;
   Bool err = false;
   Bool loadExtras = false, noImageData = false;
   Bool incrementalLoad = false;
   char * baseClassName = "Prima::Image";


#define out(x){ err = true;\
   strncpy( error, x, 256);\
   goto EXIT_NOW;}

#define outd(x,d){ err = true;\
   snprintf( error, 256, x, d);\
   goto EXIT_NOW;}   
   
   CHK;
   memset( &fi, 0, sizeof( fi));
   ret = plist_create( 8, 8);
   if ( !ret) out("Not enough memory")

   strcpy( error, "Internal error");
   fi. errbuf = error;

   /* open file */
   if (( fi. f = ( FILE *) fopen( fileName, "rb")) == NULL)
      out( strerror( errno));
   fi. fileName = fileName;
   fi. stop = false;

   /* assigning user file profile */
   if ( pexist( index)) {
      fi. frameMapSize = 1;
      if ( !( fi. frameMap  = (int*) malloc( sizeof( int))))
         out("Not enough memory");
      if ((*fi. frameMap = pget_i( index)) < 0)
         out("Invalid index");
   } else if ( pexist( map)) {
      SV * sv = pget_sv( map);
      if ( SvOK( sv)) {
         if ( SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
            AV * av = ( AV*) SvRV( sv);
            int len = av_len( av) + 1;
            if ( !( fi. frameMap = ( int *) malloc( sizeof( int) * len)))
               out("Not enough memory");
            for ( i = 0; i < len; i++) {
               SV ** holder = av_fetch( av, i, 0);
               if ( !holder) out("Array panic on 'map' property");
               if (( fi. frameMap[ i] = SvIV( *holder)) < 0)
                  out("Invalid index on 'map' property");
            }   
            fi. frameMapSize = len;
         } else 
            out("Not an array passed to 'map' property");
      }
   } else if ( pexist( loadAll) && pget_B( loadAll)) {
      fi. loadAll = true;
   } else {
      fi. frameMapSize = 1;
      if ( ! (fi. frameMap = ( int*) malloc( sizeof( int))))
         out("Not enough memory");
     *fi. frameMap = 0;
   }   

   if ( pexist( loadExtras) && pget_B( loadExtras))
      fi. loadExtras = loadExtras = true;

   if ( pexist( noImageData) && pget_B( noImageData))
      fi. noImageData = noImageData = true;

   if ( pexist( profiles)) {
      SV * sv = pget_sv( profiles);
      if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
         profiles = ( AV *) SvRV( sv);
         profiles_len = av_len( profiles);
      } else 
         out("Not an array passed to 'profiles' property");
   }  

   if ( pexist( className)) {
      PVMT vmt;
      baseClassName = pget_c( className);
      vmt = gimme_the_vmt( baseClassName);
      while ( vmt && vmt != (PVMT)CImage) 
         vmt = vmt-> base;
      if ( !vmt) 
         outd("class '%s' is not a Prima::Image descendant", baseClassName);
   }      

   /* all other properties to be parsed by codec */
   fi. extras = profile;

   fi. fileProperties = newHV(); 
   fi. frameCount = -1;

   /* finding codec */
   {
      Bool * loadmap = ( Bool *) malloc( sizeof( Bool) * imgCodecs. count);
      char * xc = fileName + strlen( fileName);

      if ( !loadmap) 
         out("Not enough memory");
      memset( loadmap, 0, sizeof( Bool) * imgCodecs. count);
      for ( i = 0; i < imgCodecs. count; i++) {
         c = ( PImgCodec ) ( imgCodecs. items[ i]);
         if ( !c-> instance)
            c-> instance = c-> vmt-> init( &c->info, c-> initParam);
         if ( !c-> instance) { /* failed to initialize, retry next time */
            loadmap[ i] = true;
            continue;
         } 
      }
      
      while ( xc != fileName) {
         if ( *xc == '.') 
            break;
         xc--;
      }   
      if ( *xc == '.') xc++;

      /* finding by extension first */
      c = nil;
      for ( i = 0; i < imgCodecs. count; i++) {
         int j = 0, found = false;
         if ( loadmap[ i]) continue;
         c = ( PImgCodec ) ( imgCodecs. items[ i]);
         while ( c-> info-> fileExtensions[ j]) {
            if ( stricmp( c-> info-> fileExtensions[ j], xc) == 0) {
               found = true;
               break;
            }   
            j++;
         } 
         if ( found) {
            loadmap[ i] = true;

            if ( !c-> info-> canLoad)
               continue;

            if (( fi. instance = c-> vmt-> open_load( c, &fi)) != NULL) {
               codecID = i;
               break;
            }   

            if ( fi. stop) { 
               err = true; 
               free( loadmap);
               goto EXIT_NOW; 
            }
         }   
         c = nil;
      }   
      
      /* use first suitable codec */
      if ( c == nil) {
         for ( i = 0; i < imgCodecs. count; i++) {        
            if ( loadmap[ i]) continue;
            c = ( PImgCodec ) ( imgCodecs. items[ i]);
            if ( !c-> info-> canLoad)
                  continue;
            if (( fi. instance = c-> vmt-> open_load( c, &fi)) != NULL) {
               codecID = i;
               break;
            }   
            if ( fi. stop) { 
               err = true; 
               free( loadmap);
               goto EXIT_NOW; 
            }
            c = nil;
         }
      }
      free( loadmap);
      if ( !c) out("No appropriate codec found");
   }   

   if ( fi. loadAll) {
      if ( fi. frameCount >= 0) {
         fi. frameMapSize = fi. frameCount;
         if ( !( fi. frameMap  = (int*) malloc( fi. frameCount * sizeof(int))))
            out("Not enough memory");
         for ( i = 0; i < fi. frameCount; i++)
            fi. frameMap[i] = i;
      } else {
         fi. frameMapSize = INT_MAX;
         incrementalLoad = true;
      }   
   }   

   /* use common profile */
   def = c-> vmt-> load_defaults( c); 
   commonHV = newHV();
   if ( profile) {
      c-> vmt-> load_check_in( c, commonHV, profile);
      apc_img_profile_add( commonHV, profile, def);
   }

   if ( fi. loadExtras && c-> info-> fileType) 
      hv_store( fi. fileProperties, "codecID", 7, newSViv( codecID), 0);

   /* loading */
   for ( i = 0; i < fi. frameMapSize; i++) {
      HV * profile = commonHV;
      char * className = baseClassName;

      fi. frame = incrementalLoad ? i : fi. frameMap[ i];
      if (( fi. frameCount >= 0 && fi. frame >= fi. frameCount) || 
          ( !c-> info-> canLoadMultiple && fi. frame > 0)) { 
         if ( !c-> info-> canLoadMultiple && fi. frameCount < 0)
            fi. frameCount = i; 
         if ( incrementalLoad) 
            /* that means, codec bothered to set frameCount at last - report no error then */
            goto EXIT_NOW;
         c-> vmt-> close_load( c, &fi);
         out("Frame index out of range");
      }   

      fi. loadExtras  = loadExtras;
      fi. noImageData = noImageData;

      /* query profile */
      if ( profiles && ( i <= profiles_len)) {
         HV * hv;
         SV ** holder = av_fetch( profiles, i, 0);
         if ( !holder) outd("Array panic on 'profiles[%d]' property", i);
         if ( SvOK( *holder)) {
            if ( SvROK( *holder) && SvTYPE( SvRV( *holder)) == SVt_PVHV) 
               hv = ( HV*) SvRV( *holder);
            else
               outd("Not a hash passed to 'profiles[%d]' property", i); 
            profile = newHV();
            apc_img_profile_add( profile, commonHV, commonHV);
            c-> vmt-> load_check_in( c, profile, hv);
            apc_img_profile_add( profile, hv, def);
            { 
               HV * profile = hv;
               if ( pexist( loadExtras))
                 fi. loadExtras  = pget_B( loadExtras);
               if ( pexist( noImageData))
                 fi. noImageData = pget_B( noImageData);
            }   
         }
      }   

      fi. jointFrame = ( fi. frame == lastFrame + 1);
      fi. profile    = profile;
      lastFrame = fi. frame;
      
      /* query className */
      if ( pexist( className)) {
         PVMT vmt;
         className = pget_c( className);
         vmt = gimme_the_vmt( className);
         while ( vmt && vmt != (PVMT)CImage) 
            vmt = vmt-> base;
         if ( !vmt) {
            if ( fi. profile != commonHV) sv_free(( SV *) fi. profile);      
            outd("class '%s' is not a Prima::Image descendant", className);
         }   
      }      

      /* create storage */
      if (( i > 0) || ( self == nilHandle)) {
         HV * profile = newHV();
         fi. object = Object_create( className, profile);
         sv_free(( SV *) profile); 
         if ( !fi. object) {
            if ( fi. profile != commonHV) sv_free(( SV *) fi. profile);
            outd("Failed to create object '%s'", className);
         }   
      } else
         fi. object = self;

      fi. frameProperties = newHV();

      /* loading image */
      if ( !c-> vmt-> load( c, &fi)) {
         c-> vmt-> close_load( c, &fi);
         sv_free(( SV *) fi. frameProperties);
         if ( fi. object != self)
            Object_destroy( fi. object);
         if ( fi. profile != commonHV) sv_free(( SV *) fi. profile);
         if ( incrementalLoad) {
            if ( fi. frameCount < 0) fi. frameCount = fi. frame;
            goto EXIT_NOW; /* EOF, report no error */
         }   
         err = true;
	 goto EXIT_NOW;
      }  

      /* checking for grayscale */
      {
         PImage i = ( PImage) fi. object;
         if ( !( i-> type & imGrayScale)) 
            switch ( i-> type & imBPP) {
            case imbpp1:
               if ( i-> palSize == 2 && memcmp( i-> palette, stdmono_palette, sizeof( stdmono_palette)) == 0)
                  i-> type |= imGrayScale;
               break;
            case imbpp4:
               if ( i-> palSize == 16 && memcmp( i-> palette, std16gray_palette, sizeof( std16gray_palette)) == 0)
                  i-> type |= imGrayScale;
               break;
            case imbpp8:
               if ( i-> palSize == 256 && memcmp( i-> palette, std256gray_palette, sizeof( std256gray_palette)) == 0)
                  i-> type |= imGrayScale;
               break;
            }
      }   

      /* updating image */
      if ( !fi. noImageData)
         CImage( fi. object)-> update_change( fi. object);

      /* applying extras */
      if ( fi. loadExtras) {
         HV * extras = newHV();
         SV * sv = newRV_noinc(( SV *) extras);
         
         apc_img_profile_add( extras, fi. fileProperties,  fi. fileProperties);
         apc_img_profile_add( extras, fi. frameProperties, fi. frameProperties);
         if ( i == 0) firstObjectExtras = extras; 
         hv_store(( HV* )SvRV((( PAnyObject) fi. object)-> mate), "extras", 6, newSVsv( sv), 0);
         sv_free( sv);
      } 

      sv_free(( SV *) fi. frameProperties);
      if ( fi. profile != commonHV) sv_free(( SV *) fi. profile);

      list_add( ret, fi. object);
   }

   c-> vmt-> close_load( c, &fi);

   /* returning info for null load request  */
   if ( self && loadExtras && fi. frameMapSize == 0) {
      HV * extras = newHV();
      SV * sv = newRV_noinc(( SV *) extras);
      apc_img_profile_add( extras, fi. fileProperties,  fi. fileProperties);
      firstObjectExtras = extras; 
      hv_store(( HV* )SvRV((( PAnyObject) self)-> mate), "extras", 6, newSVsv( sv), 0);
      sv_free( sv);
   }   
   
EXIT_NOW:;
   if ( fi. frameCount < 0 && pexist( wantFrames) && pget_i( wantFrames)) {
      fi. frameCount = apc_img_frame_count( fileName);
   }
   if ( firstObjectExtras)
      hv_store( firstObjectExtras, "frames", 6, newSViv( fi. frameCount), 0);
   if ( err && ret)
      list_add( ret, nilHandle); /* indicate the error */
   if ( def)
      sv_free(( SV *) def);
   if ( commonHV)
      sv_free(( SV *) commonHV);
   if ( fi. fileProperties) 
      sv_free((SV *) fi. fileProperties); 
   if ( fi. f) 
      fclose( fi. f);
   free( fi. frameMap);
   return ret;
#undef out   
#undef outd
}   

int 
apc_img_frame_count( char * fileName)
{
   PImgCodec c = nil;
   ImgLoadFileInstance fi;
   int i, frameMap, ret = 0;
   char error[256];

   CHK;
   memset( &fi, 0, sizeof( fi));
   /* open file */
   if (( fi. f = ( FILE *) fopen( fileName, "rb")) == NULL) goto EXIT_NOW;
   
   /* assigning request */
   fi. fileName = fileName;
   fi. frameMapSize   = frameMap = 0;
   fi. frameMap       = &frameMap;
   fi. loadExtras     = true;
   fi. noImageData    = true;
   fi. extras         = newHV();
   fi. fileProperties = newHV(); 
   fi. frameCount = -1;
   fi. errbuf     = error;
   fi. stop       = false;

   /* finding codec */
   {
      Bool * loadmap = ( Bool*) malloc( sizeof( Bool) * imgCodecs. count);
      char * xc = fileName + strlen( fileName);

      if ( !loadmap) 
         return 0;
      memset( loadmap, 0, sizeof( Bool) * imgCodecs. count);
      for ( i = 0; i < imgCodecs. count; i++) {
         c = ( PImgCodec ) ( imgCodecs. items[ i]);
         if ( !c-> instance)
            c-> instance = c-> vmt-> init( &c->info, c-> initParam);
         if ( !c-> instance) { /* failed to initialize, retry next time */
            loadmap[ i] = true;
            continue;
         } 
      }
      
      while ( xc != fileName) {
         if ( *xc == '.') 
            break;
         xc--;
      }   
      if ( *xc == '.') xc++;

      /* finding by extension first */
      c = nil;
      for ( i = 0; i < imgCodecs. count; i++) {
         int j = 0, found = false;
         if ( loadmap[ i]) continue;
         c = ( PImgCodec ) ( imgCodecs. items[ i]);
         while ( c-> info-> fileExtensions[ j]) {
            if ( stricmp( c-> info-> fileExtensions[ j], xc) == 0) {
               found = true;
               break;
            }   
            j++;
         } 
         if ( found) {
            loadmap[ i] = true;

            if ( !c-> info-> canLoad)
               continue;
            if (( fi. instance = c-> vmt-> open_load( c, &fi)) != NULL)
               break;
            
            if ( fi. stop) { 
               free( loadmap);
               goto EXIT_NOW; 
            }
         }   
         c = nil;
      }   
      
      if ( c == nil) {
         for ( i = 0; i < imgCodecs. count; i++) {        
            if ( loadmap[ i]) continue;
            c = ( PImgCodec ) ( imgCodecs. items[ i]);
            if ( !c-> info-> canLoad)
                  continue;
            if (( fi. instance = c-> vmt-> open_load( c, &fi)) != NULL)
               break;
            if ( fi. stop) { 
               free( loadmap);
               goto EXIT_NOW; 
            }
           c = nil;
         }
      }
      free( loadmap);
      if ( !c) goto EXIT_NOW;
   }  
   
   /* can tell now? */
   
   if ( fi. frameCount >= 0) {
      c-> vmt-> close_load( c, &fi);
      ret = fi. frameCount;
      goto EXIT_NOW;
   }  

   if ( !c-> info-> canLoadMultiple) {
      c-> vmt-> close_load( c, &fi);
      ret = 1; /* single-framed file. what else? */
      goto EXIT_NOW;
   }   

   /* if can't, trying to load huge index, hoping that if */
   /* codec have a sequential access, it eventually meet the  */
   /* EOF and report the frame count */
   {
      HV * profile = newHV();
      fi. object = Object_create( "Prima::Image", profile);
      sv_free(( SV *) profile); 
      frameMap = fi. frame = INT_MAX;
      fi. frameProperties = newHV();
   }                  
   
   /* loading image */
   if ( c-> vmt-> load( c, &fi) || fi. frameCount >= 0) {
      /* well, INT_MAX frame is ok, and maybe more, but can't report more anyway */
      c-> vmt-> close_load( c, &fi);
      ret = ( fi. frameCount < 0) ? INT_MAX : fi. frameCount;
      goto EXIT_NOW;
   }   

   /* can't report again - so loading as may as we can */
   fi. loadAll = true;
   for ( i = 0; i < INT_MAX; i++) {
      fi. jointFrame = i > 0;
      frameMap = fi. frame = i;
      if ( !c-> info-> canLoadMultiple) {
         c-> vmt-> close_load( c, &fi);
         if ( !( fi. instance = c-> vmt-> open_load( c, &fi))) {
            ret = i;
            goto EXIT_NOW;
         }   
      }   
      if ( !c-> vmt-> load( c, &fi) || fi. frameCount >= 0) {
         c-> vmt-> close_load( c, &fi);   
         ret = ( fi. frameCount < 0) ? i : fi. frameCount;
         goto EXIT_NOW;
      }   
   }   

   c-> vmt-> close_load( c, &fi);
   
EXIT_NOW:;
   if ( fi. object)
      Object_destroy( fi. object);
   if ( fi. extras)
      sv_free(( SV *) fi. extras);
   if ( fi. frameProperties)
      sv_free(( SV *) fi. frameProperties);
   if ( fi. fileProperties)
      sv_free(( SV *) fi. fileProperties);
   if ( fi. f) 
      fclose( fi. f);
   return ret;
}   

int
apc_img_save( Handle self, char * fileName, HV * profile, char * error)
{
   int i;
   PImgCodec c = nil;
   ImgSaveFileInstance fi;
   AV * images = nil;
   HV * def = nil, * commonHV = nil;
   Bool err = false;
   int codecID = -1;
   int xself = self ? 1 : 0;
   int ret = 0;
   Bool autoConvert = true;

#define out(x){ err = true;\
   strncpy( error, x, 256);\
   goto EXIT_NOW;}

#define outd(x,d){ err = true;\
   snprintf( error, 256, x, d);\
   goto EXIT_NOW;}   
   
   CHK;
   memset( &fi, 0, sizeof( fi));

   if ( pexist( append) && pget_B( append))
      fi. append = true;

   if ( pexist( autoConvert))
      autoConvert = pget_B( autoConvert);

   /* open file */
   if ( fi. append) {
      FILE * f = ( FILE *) fopen( fileName, "rb");
      fclose( f);
      if ( !f) fi. append = false;
   }   
   
   fi. errbuf = error;
   if (( fi. f = ( FILE *) fopen( fileName, fi. append ? "rb+" : "wb+" )) == NULL)
      out( strerror( errno));

   fi. fileName     = fileName;
   
   fi. frameMapSize = xself;
   if ( pexist( images)) {
      SV * sv = pget_sv( images);
      if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
         images = ( AV *) SvRV( sv);
         fi. frameMapSize += av_len( images) + 1;
      } else 
         out("Not an array passed to 'images' property");
   }   
   if ( fi. frameMapSize == 0)
      out("Nothing to save");

   /* fill array of objects */
   if ( !( fi. frameMap = ( Handle *) malloc( sizeof( Handle) * fi. frameMapSize)))
      out("Not enough memory");
   memset( fi. frameMap, 0, sizeof( Handle) * fi. frameMapSize);
   
   for ( i = 0; i < fi. frameMapSize; i++) {
      Handle obj = nilHandle;

      /* query profile */
      if ( self && (i == 0)) {
         obj = self;
         if ( !kind_of( obj, CImage))
            out("Not a Prima::Image descendant passed"); 
      } else if ( images) {
         SV ** holder = av_fetch( images, i - xself, 0);
         Handle obj;
         if ( !holder) outd("Array panic on 'images[%d]' property", i - xself);
         obj = gimme_the_mate( *holder);
         if ( !obj) 
            outd("Invalid object reference passed in 'images[%d]'", i - xself);
         if ( !kind_of( obj, CImage))
            outd("Not a Prima::Image descendant passed in 'images[%d]'", i - xself);
      } else
         out("Logic error");
      fi. frameMap[ i] = obj;
   } 

   /* all other properties to be parsed by codec */
   fi. extras = profile;

   /* finding codec */
   strcpy( error, "No appropriate codec found");
   {
      Bool * savemap = ( Bool*) malloc( sizeof( Bool) * imgCodecs. count);
      char * xc = fileName + strlen( fileName);

      if ( !savemap)
         out("Not enough memory");
      memset( savemap, 0, sizeof( Bool) * imgCodecs. count);
      
      for ( i = 0; i < imgCodecs. count; i++) {
         c = ( PImgCodec ) ( imgCodecs. items[ i]);
         if ( !c-> instance)
            c-> instance = c-> vmt-> init( &c->info, c-> initParam);
         if ( !c-> instance) { /* failed to initialize, retry next time */
            savemap[ i] = true;
            continue;
         } 
      }

      /* checking 'codecID', if available */
      {
         SV * c = nil;
         if ( pexist( codecID))
            c = pget_sv( codecID);
         else if ( self && 
                   hv_exists(( HV*)SvRV((( PAnyObject) self)-> mate), 
                              "extras", 6)) {
            SV ** sv = hv_fetch(( HV*)SvRV((( PAnyObject) self)-> mate), "extras", 6, 0);
            if ( sv && SvOK( *sv) && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVHV) {
               HV * profile = ( HV *) SvRV( *sv);
               if ( pexist( codecID)) 
                  c = pget_sv( codecID);
            }   
         }
         if ( c && SvOK( c)) { /* accept undef */
            codecID = SvIV( c);
            if ( codecID < 0) codecID = imgCodecs. count - codecID;
         }
      }
         
      /* find codec */
      c = nil;
      if ( codecID >= 0) {
         if ( codecID >= imgCodecs. count) 
             out("Codec index out of range");

         c = ( PImgCodec ) ( imgCodecs. items[ codecID]);

         if ( !c-> info-> canSave) 
            out("Codec cannot save images");
         
         if (( fi. append || ( fi. frameMapSize > 1)) && 
               !c-> info-> canSaveMultiple) 
            out("Codec cannot append frames");
         
         if (( fi. instance = c-> vmt-> open_save( c, &fi)) == NULL) 
            out("Codec cannot handle this file");

         if ( !autoConvert) {
            int j, *k = c-> info-> saveTypes, ok = 0;
            for ( j = 0; j < fi. frameMapSize; j++) {
               int type = PImage( fi. frameMap[j])-> type;
               while ( *k) {
                  if ( type == *k) {
                     ok = 1;
                     break;
                  }   
                  k++;
               }   
            }   
            if ( !ok) 
               out("Image type(s) not supported by the codec specified");
         }      
      }   
     
      if ( !c) {
         /* finding codec by extension  */
         while ( xc != fileName) {
            if ( *xc == '.') 
               break;
            xc--;
         }   
         if ( *xc == '.') xc++;
         
         for ( i = 0; i < imgCodecs. count; i++) {
            int j = 0, found = false;
            if ( savemap[ i]) continue;
            c = ( PImgCodec ) ( imgCodecs. items[ i]);
            while ( c-> info-> fileExtensions[ j]) {
               if ( stricmp( c-> info-> fileExtensions[ j], xc) == 0) {
                  found = true;
                  break;
               }   
               j++;
            } 

            if ( found) {
               savemap[ i] = true;
               if ( !c-> info-> canSave) 
                  continue;
               
               if (( fi. append || ( fi. frameMapSize > 1)) 
                   && !c-> info-> canSaveMultiple)
                  continue;
               
               if ( !autoConvert) {
                  int j, *k = c-> info-> saveTypes, ok = 0;
                  for ( j = 0; j < fi. frameMapSize; j++) {
                     int type = PImage( fi. frameMap[j])-> type;
                     while ( *k) {
                        if ( type == *k) {
                           ok = 1;
                           break;
                        }   
                        k++;
                     }   
                  }   
                  if ( !ok) 
                     continue;
               }      
              
               if (( fi. instance = c-> vmt-> open_save( c, &fi)) != NULL)
                  break;
            }   
            c = nil;
         }   
      }
      
      free( savemap);
      if ( !c) { /* use pre-formatted error string */
         err = true;
         goto EXIT_NOW;
      }   
   }   

   snprintf( error, 256, "Error saving %s", fileName);

   /* use common profile */
   def = c-> vmt-> save_defaults( c); 
   commonHV = newHV();
   if ( profile) {
      c-> vmt-> save_check_in( c, commonHV, profile);
      apc_img_profile_add( commonHV, profile, def);
   }
   
   /* saving */
   for ( i = 0; i < fi. frameMapSize; i++) {
      HV * profile = commonHV;
      PImage im;

      im = ( PImage) fi. frameMap[ i];
      if ( hv_exists(( HV*)SvRV( im-> mate), "extras", 6)) {
         SV ** sv = hv_fetch(( HV*)SvRV( im-> mate), "extras", 6, 0);
         if ( sv && SvOK( *sv) && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVHV) {
            HV * hv = ( HV *) SvRV( *sv);
            profile = newHV();
            apc_img_profile_add( profile, commonHV, commonHV);
            c-> vmt-> save_check_in( c, profile, hv); 
            apc_img_profile_add( profile, hv, def);
         }   
      }   
      
      fi. frame  = i;
      fi. object = fi. frameMap[ i];
      fi. objectExtras = profile;

      /* converting image to format with maximum bit depth */
      if ( autoConvert) {
         int *k = c-> info-> saveTypes;
         int max = *k & imBPP, supported = false;
         while ( *k) {
            if ( im-> type == *k) {
               supported = true;
               break;
            }   
            if ( max < ( *k & imBPP))
               max = *k;
            k++;
         }   
         if ( !supported) {
            im-> self-> set_type(( Handle) im, max);
            if ( max != im-> type) outd("Failed converting image to type '%04x'", max);
         }
      }      
      
      /* saving image */
      if ( !c-> vmt-> save( c, &fi)) {
         c-> vmt-> close_save( c, &fi);
         if ( fi. objectExtras != commonHV) sv_free(( SV *) fi. objectExtras);
	 err = true;
	 goto EXIT_NOW;
      }  

      if ( fi. objectExtras != commonHV) sv_free(( SV *) fi. objectExtras);
      ret++;
   }

   c-> vmt-> close_save( c, &fi);
   
EXIT_NOW:;
   free( fi. frameMap);
   if ( fi. f)
      fclose( fi. f);
   if ( err)
      unlink( fileName);
   if ( def)
      sv_free(( SV *) def);
   if ( commonHV)
      sv_free(( SV *) commonHV);
   return err ? -ret : ret;
#undef out   
#undef outd
}   
void
apc_img_codecs( PList ret)
{
   int i;
   PImgCodec c;
   
   CHK;
   for ( i = 0; i < imgCodecs. count; i++) {
      c = ( PImgCodec ) ( imgCodecs. items[ i]);
      if ( !c-> instance)
         c-> instance = c-> vmt-> init( &c->info, c-> initParam);
      if ( !c-> instance)  /* failed to initialize, retry next time */
         continue;
      list_add( ret, ( Handle) c);
   }  
}

int
apc_img_read_palette( PRGBColor palBuf, SV * palette)
{
   AV * av;
   int i, count;
   Byte buf[768];

   if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV))
      return 0;
   av = (AV *) SvRV( palette);
   count = av_len( av) + 1;
   if ( count > 768) count = 768;
   count -= count % 3;

   for ( i = 0; i < count; i++)
   {
      SV **itemHolder = av_fetch( av, i, 0);
      if ( itemHolder == nil) return 0;
      buf[ i] = SvIV( *itemHolder);
   }
   memcpy( palBuf, buf, count);
   return count/3;
}


static AV * fill_plist( char * key, char ** list, HV * profile)
{
   AV * av = newAV();
   if ( !list) list = imgPVEmptySet;
   while ( *list) {
      av_push( av, newSVpv( *list, 0));
      list++;
   }   
   hv_store( profile, key, strlen( key), newRV_noinc(( SV *) av), 0);
   return av;
}  

static void fill_ilist( char * key, int * list, HV * profile)
{
   AV * av = newAV();
   if ( !list) list = imgIVEmptySet;
   while ( *list) {
      av_push( av, newSViv( *list));
      list++;
   }   
   hv_store( profile, key, strlen( key), newRV_noinc(( SV *) av), 0);
}   


HV *  
apc_img_info2hash( PImgCodec codec)
{
   HV * profile, * hv;
   AV * av;
   PImgCodecInfo c;

   CHK;
   profile = newHV();
   if ( !codec) return profile;

   if ( !codec-> instance)
      codec-> instance = codec-> vmt-> init( &codec->info, codec-> initParam);
   if ( !codec-> instance) 
      return profile;
   c = codec-> info;
      
   pset_c( name,   c-> name);
   pset_c( vendor, c-> vendor);
   pset_i( versionMajor, c-> versionMaj);
   pset_i( versionMinor, c-> versionMin);
   fill_plist( "fileExtensions", c-> fileExtensions, profile);
   pset_c( fileType, c-> fileType);
   pset_c( fileShortType, c-> fileShortType);
   fill_plist( "featuresSupported", c-> featuresSupported, profile);
   pset_c( module,  c-> primaModule);
   pset_c( package, c-> primaPackage);
   pset_i( canLoad,         c-> canLoad);
   pset_i( canSave        , c-> canSave);
   pset_i( canLoadMultiple, c-> canLoadMultiple);
   pset_i( canSaveMultiple, c-> canSaveMultiple);
   
   
   fill_ilist( "types",  c-> canSave ? c-> saveTypes : nil, profile);

   if ( c-> canLoad) {
      hv = codec-> vmt-> load_defaults( codec);
      if ( c-> canLoadMultiple) {
         hv_store( hv, "index",        5, newSViv(0),     0);
         hv_store( hv, "map",          3, newSVsv(nilSV), 0);
         hv_store( hv, "loadAll",      7, newSViv(0),     0);
         hv_store( hv, "wantFrames",  10, newSViv(0),     0);
      }
      hv_store( hv, "loadExtras",  10, newSViv(0),     0);
      hv_store( hv, "noImageData", 11, newSViv(0),     0);
      hv_store( hv, "className",    9, newSVpv("Prima::Image", 0), 0);
   } else
      hv = newHV();
   pset_sv_noinc( loadInput, newRV_noinc(( SV *) hv));
   
   av = fill_plist( "loadOutput", c-> canLoad ? c-> loadOutput : nil, profile);
   if ( c-> canLoad) {
      if ( c-> canLoadMultiple) 
         av_push( av, newSVpv( "frames", 0));
      av_push( av, newSVpv( "height", 0));
      av_push( av, newSVpv( "width",  0));
      av_push( av, newSVpv( "codecID", 0));      
   }
   
   if ( c-> canSave) {
      hv = codec-> vmt-> save_defaults( codec);
      if ( c-> canSaveMultiple)
         hv_store( hv, "append",       6, newSViv(0), 0);
      hv_store( hv, "autoConvert", 10, newSViv(1), 0);
      hv_store( hv, "codecID",     7,  newSVsv( nilSV), 0);
   } else
      hv = newHV();
   pset_sv_noinc( saveInput, newRV_noinc(( SV *) hv));
   return profile;
}   

char * imgPVEmptySet[] = { nil };
int    imgIVEmptySet[] = { 0 };


#ifdef __cplusplus
}
#endif
