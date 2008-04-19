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

#include "img.h"
#define Drawable        XDrawable
#define Font            XFont
#define Window          XWindow
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef Font
#undef Drawable
#undef Bool
#undef Window
#define ComplexShape 0
#define XBool int
#undef Complex
#undef FUNC
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
   IMG_LOAD_FROM_FILE | IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM,
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

static Byte*
mirror_bits( void)
{
   static Bool initialized = false;
   static Byte bits[256];
   unsigned int i, j;
   int k;

   if (!initialized) {
      for ( i = 0; i < 256; i++) {
         bits[i] = 0;
         j = i;
         for ( k = 0; k < 8; k++) {
            bits[i] <<= 1;
            if ( j & 0x1)
               bits[i] |= 1;
            j >>= 1;
         }
      }
      initialized = true;
   }

   return bits;
}

static void
mirror_bytes( unsigned char *data, int dataSize)
{
   Byte *mirrored_bits = mirror_bits();
   while ( dataSize--) {
      *data = mirrored_bits[*data];
      data++;
   }
}



static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l;
   unsigned int w, h;
   int yh, yw;
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
   mirror_bytes( i-> data, i-> dataSize);
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

static void
myprintf( PImgIORequest req, const char *format, ...)
{
	int len;
	char buf[2048];
        va_list args;
        va_start( args, format);
        len = vsnprintf( buf, 2048, format, args);
        va_end( args);
	req_write( req, len, buf);
}

static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   dPROFILE;
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
   if ( xc == NULL) xc = "xbm";
   name = xc;
   while ( *xc) {
      if ( *xc == '/') 
         name = xc + 1;
      xc++;
   }   
   xc = malloc( strlen( name) + 1);
   if ( xc) strcpy( xc, name);
   name = xc;
   
   while (*xc) {
      if ( *xc == '.') {
         *xc = 0;
         break;
      }   
      xc++;
   } 
   
   myprintf( fi-> req, "#define %s_width %d\n", name, i-> w);
   myprintf( fi-> req, "#define %s_height %d\n", name, i-> h);
   if ( pexist( hotSpotX))
      myprintf( fi-> req, "#define %s_x_hot %d\n", name, (int)pget_i( hotSpotX));
   if ( pexist( hotSpotY))
      myprintf( fi-> req, "#define %s_y_hot %d\n", name, (int)pget_i( hotSpotY));
   myprintf( fi-> req, "static char %s_bits[] = {\n  ", name);

  
   while ( h--) {
      Byte * s1 = l;
      int w = ls;
      
      memcpy( s1, s, ls);
      mirror_bytes( s1, ls);
      
      while ( w--) {
         if ( first) {
           first = 0;
         } else {
           myprintf( fi-> req, ", ");
         }  
         if ( col++ == 11) {
            col = 0;
            myprintf( fi-> req, "\n  ");
         }   
         myprintf( fi-> req, "0x%02x", (Byte)~(*(s1++)));
      }   
      s -= i-> lineSize;
   }  

   myprintf( fi-> req, "};\n");
   
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
