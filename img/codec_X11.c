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
 */

#include "img.h"
#include "unix/guts.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif

static char * xbmext[] = { "xbm", nil };
static int    xbmbpp[] = { imbpp1 | imGrayScale, 0 };   

static char * loadOutput[] = { 
   "hotSpotX",
   "hotSpotY",
   nil
};

static ImgCodecInfo codec_info = {
   "X11 Bitmap",
   "X Consortium",
   X_PROTOCOL, 5,    /* version */
   xbmext,    /* extension */
   "X11 Bitmap File",     /* file type */
   "XBM", /* short type */
   nil,    /* features  */
   "",     /* module */
   "",     /* package */
   true,   /* canLoad */
   false,  /* canLoadMultiple  */
   true,   /* canSave */
   false,  /* canSaveMultiple */
   xbmbpp, /* save types */
   loadOutput
};

static void * 
init( PImgCodecInfo * info, void * param)
{
   *info = &codec_info;
   return (void*)1;
}   

typedef struct _LoadRec {
   int w, h, yh, yw;
   Byte * data;
} LoadRec;


static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l;
   int w, h, yh, yw;
   Byte * data;

   if( XReadBitmapFileData( fi-> fileName, &w, &h, &data, &yw, &yh) != BitmapSuccess)
      return nil;

   fi-> stop = true;
   fi-> frameCount = 1;
   
   l = malloc( sizeof( LoadRec));
   if ( !l) return nil;

   l-> w  = w;
   l-> h  = h;
   l-> yw = yw;
   l-> yh = yh;
   l-> data = data;

   return l;
}

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l = ( LoadRec *) fi-> instance;
   HV * profile = fi-> frameProperties;
   PImage i = ( PImage) fi-> object;
   int ls, h;
   Byte * src, * dst;

   if ( fi-> loadExtras) {
      pset_i( hotSpotX, l-> yw);
      pset_i( hotSpotY, l-> yh);
   }  

   if ( fi-> noImageData) {
      CImage( fi-> object)-> set_type( fi-> object, imbpp1 | imGrayScale);
      pset_i( width,      l-> w);
      pset_i( height,     l-> h);
      return true;      
   }
   
   CImage( fi-> object)-> create_empty( fi-> object, l-> w, l-> h, imbpp1 | imGrayScale);
   ls = ( l-> w >> 3) + (( l-> w & 7) ? 1 : 0);
   src = l-> data;
   h   = l-> h;
   dst = i-> data + ( l->h - 1 ) * i-> lineSize;
   while ( h--) {
      int w = ls;
      Byte * d = dst, * s = src;
      /* in order to comply with imGrayScale, revert bits
         rather that palette */
      while ( w--) *(d++) = ~ *(s++);
      src += ls;
      dst -= i-> lineSize;
   }   
   prima_mirror_bytes( i-> data, i-> dataSize);
   return true;
}   

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l = ( LoadRec *) fi-> instance;
   XFree( l-> data);
   free( fi-> instance);  
}

static HV *
save_defaults( PImgCodec c)
{
   HV * profile = newHV();
   pset_i( hotSpotX, 0);
   pset_i( hotSpotY, 0);
   return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   return (void*)1;
}

static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   PImage i = ( PImage) fi-> object;
   Byte * l;
   int h = i-> h, col = -1;
   Byte * s = i-> data + ( h - 1) * i-> lineSize;
   char * xc = fi-> fileName, * name;
   int ls = ( i-> w >> 3) + (( i-> w & 7) ? 1 : 0);
   int first = 1;
   HV * profile = fi-> objectExtras;

   l = malloc( ls);
   if ( !l) return false;

   /* extracting name */
   name = xc;
   while ( *xc) {
      if ( *xc == '/') 
         name = xc + 1;
      xc++;
   }   
   xc = malloc( strlen( name + 1));
   strcpy( xc, name);
   name = xc;
   
   while (*xc) {
      if ( *xc == '.') {
         *xc = 0;
         break;
      }   
      xc++;
   }  
   
   fprintf( fi-> f, "#define %s_width %d\n", name, i-> w);
   fprintf( fi-> f, "#define %s_height %d\n", name, i-> h);
   if ( pexist( hotSpotX))
      fprintf( fi-> f, "#define %s_x_hot %d\n", name, (int)pget_i( hotSpotX));
   if ( pexist( hotSpotY))
      fprintf( fi-> f, "#define %s_y_hot %d\n", name, (int)pget_i( hotSpotY));
   fprintf( fi-> f, "static char %s_bits[] = {\n  ", name);

  
   while ( h--) {
      Byte * s1 = l;
      int w = ls;
      
      memcpy( s1, s, ls);
      prima_mirror_bytes( s1, ls);
      
      while ( w--) {
         if ( first) {
           first = 0;
         } else {
           fprintf( fi-> f, ", ");
         }  
         if ( col++ == 11) {
            col = 0;
            fprintf( fi-> f, "\n  ");
         }   
         fprintf( fi-> f, "0x%02x", (Byte)~(*(s1++)));
      }   
      s -= i-> lineSize;
   }  

   fprintf( fi-> f, "};\n");
   
   free( l);
   free( name);
   return true;
}   

static void 
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
}

void 
apc_img_codec_X11( void )
{
   struct ImgCodecVMT vmt;
   memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
   vmt. init          = init;
   vmt. open_load     = open_load;
   vmt. load          = load; 
   vmt. close_load    = close_load; 
   vmt. save_defaults = save_defaults;
   vmt. open_save     = open_save;
   vmt. save          = save; 
   vmt. close_save    = close_save;
   apc_img_register( &vmt, nil);
}

#ifdef __cplusplus
}
#endif
