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
#include "img_conv.h"
#include "Image.h"

/* Prima-specific undefs */
#undef HAVE_STDLIB_H
#undef LOCAL
#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

static char * jpgext[] = { "jpg", "jpe", "jpeg", nil };
static int    jpgbpp[] = { imbpp8 | imGrayScale, imbpp24, 0 };   

static ImgCodecInfo codec_info = {
   "JPEG",
   "Independent JPEG Group",
   6, 1,    /* version */
   jpgext,    /* extension */
   "JPEG File Interchange Format",     /* file type */
   "JPEG", /* short type */
   nil,    /* features  */
   "Prima::Image::jpeg",  /* module */
   "Prima::Image::jpeg",  /* package */
   true,   /* canLoad */
   false,  /* canLoadMultiple  */
   true,   /* canSave */
   false,  /* canSaveMultiple */
   jpgbpp, /* save types */
   nil
};

static void * 
init( PImgCodecInfo * info, void * param)
{
   *info = &codec_info;
   codec_info. versionMaj = JPEG_LIB_VERSION / 10;
   codec_info. versionMin = JPEG_LIB_VERSION % 10;
   return (void*)1;
}   

typedef struct _LoadRec {
   struct  jpeg_decompress_struct d;
   struct  jpeg_error_mgr         e;
   jmp_buf                        j;
   Bool                        init;
} LoadRec;

static void
load_output_message(j_common_ptr cinfo)
{
   char buffer[JMSG_LENGTH_MAX];
   PImgLoadFileInstance fi = ( PImgLoadFileInstance)( cinfo-> client_data); 
   LoadRec *l = (LoadRec*)( fi-> instance);
   if ( !l-> init) {
      (*cinfo->err->format_message) (cinfo, buffer);
      strncpy( fi-> errbuf, buffer, 256);
   }
}

static void 
load_error_exit(j_common_ptr cinfo)
{
   LoadRec *l = (LoadRec*)((( PImgLoadFileInstance)( cinfo-> client_data))-> instance);
   load_output_message( cinfo);
   longjmp( l-> j, 1);
}

static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l;
   char buf[4];

   if ( fseek( fi-> f, 0, SEEK_SET) < 0) return false;
   if ( fread( buf, 1, 4, fi-> f) != 4) return false;
   if (
         ( memcmp( "\xff\xd8\xff\xe0", buf, 4) != 0) &&
         ( memcmp( "\xe0\xff\xd8\xff", buf, 4) != 0)
      ) return false;   
   if ( fseek( fi-> f, 0, SEEK_SET) < 0) return false;
   fi-> stop = true;
   fi-> frameCount = 1;
   
   l = malloc( sizeof( LoadRec));
   if ( !l) return nil;
   memset( l, 0, sizeof( LoadRec));
   l-> d. client_data = ( void*) fi;
   l-> d. err = jpeg_std_error( &l-> e);
   l-> d. err-> output_message = load_output_message;
   l-> d. err-> error_exit = load_error_exit;
   l-> init = true;
   fi-> instance = l;
   if ( setjmp( l-> j) != 0) {
      fi-> instance = nil;
      jpeg_destroy_decompress(&l-> d);
      free( l);
      return false;
   } 
   jpeg_create_decompress( &l-> d);
   jpeg_stdio_src( &l-> d, fi-> f);
   jpeg_read_header( &l-> d, true);
   l-> init = false;
   return l;
}

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l = ( LoadRec *) fi-> instance;
   PImage i = ( PImage) fi-> object;
   int bpp;
  
   if ( setjmp( l-> j) != 0) return false;
   jpeg_start_decompress( &l-> d);
   bpp = l-> d. output_components * 8;   
   if ( bpp != 8 && bpp != 24) {
      sprintf( fi-> errbuf, "Bit depth %d is not supported", bpp);
      return false;
   }   

   if ( bpp == 8) bpp |= imGrayScale;
   CImage( fi-> object)-> create_empty( fi-> object, 1, 1, bpp);
   if ( fi-> noImageData) {
      hv_store( fi-> frameProperties, "width",  5, newSViv( l-> d. output_width), 0);
      hv_store( fi-> frameProperties, "height", 6, newSViv( l-> d. output_height), 0);
      jpeg_abort_decompress( &l-> d);
      return true;
   }   
   
   CImage( fi-> object)-> create_empty( fi-> object, l-> d. output_width, l-> d. output_height, bpp);
   {
      Byte * dest = i-> data + ( i-> h - 1) * i-> lineSize;
      while ( l-> d.output_scanline < l-> d.output_height ) {
	 JSAMPROW sarray[1];
	 int scanlines;
         sarray[0] = dest;
         scanlines = jpeg_read_scanlines(&l-> d, sarray, 1);
         if ( bpp == 24) 
            cm_reverse_palette(( PRGBColor) dest, ( PRGBColor) dest, i-> w);
         dest -= scanlines * i-> lineSize;
      }   
   }   
   jpeg_finish_decompress(&l-> d);
   return true;
}   


static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l = ( LoadRec *) fi-> instance;
   jpeg_destroy_decompress(&l-> d);
   free( l);
}

typedef struct _SaveRec {
   struct  jpeg_compress_struct   c;
   struct  jpeg_error_mgr         e;
   jmp_buf                        j;
   Byte                       * buf;
   Bool                        init;
} SaveRec;


static void
save_output_message(j_common_ptr cinfo)
{
   char buffer[JMSG_LENGTH_MAX];
   PImgSaveFileInstance fi = ( PImgSaveFileInstance)( cinfo-> client_data); 
   SaveRec *l = (SaveRec*)( fi-> instance);
   if ( !l-> init) {
      (*cinfo->err->format_message) (cinfo, buffer);
      strncpy( fi-> errbuf, buffer, 256);
   }
}

static void 
save_error_exit(j_common_ptr cinfo)
{
   SaveRec *l = (SaveRec*)((( PImgSaveFileInstance)( cinfo-> client_data))-> instance);
   save_output_message( cinfo);
   longjmp( l-> j, 1);
}

static HV *
save_defaults( PImgCodec c)
{
   HV * profile = newHV();
   pset_i( quality, 75);
   pset_i( progressive, 0);
   return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   SaveRec * l;
   
   l = malloc( sizeof( SaveRec));
   if ( !l) return nil;
   
   memset( l, 0, sizeof( SaveRec));
   l-> c. client_data = ( void*) fi;
   l-> c. err = jpeg_std_error( &l-> e);
   l-> c. err-> output_message = save_output_message;
   l-> c. err-> error_exit = save_error_exit;
   l-> init = true;
   fi-> instance = l;
   if ( setjmp( l-> j) != 0) {
      fi-> instance = nil;
      jpeg_destroy_compress(&l-> c);
      free( l);
      return false;
   } 
   jpeg_create_compress( &l-> c);
   jpeg_stdio_dest( &l-> c, fi-> f);
   l-> init = false;
   return l;
}

static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   PImage i = ( PImage) fi-> object;
   SaveRec * l = ( SaveRec *) fi-> instance;
   HV * profile = fi-> objectExtras;
   
   if ( setjmp( l-> j) != 0) return false;

   l-> c. image_width  = i-> w;
   l-> c. image_height = i-> h;
   l-> c. input_components = ((( i-> type & imBPP) == 24) ? 3 : 1);
   l-> c. in_color_space   = ((( i-> type & imBPP) == 24) ? JCS_RGB : JCS_GRAYSCALE);
   jpeg_set_defaults( &l-> c);
   
   if ( pexist( quality)) {
      int q = pget_i( quality);
      if ( q < 0 || q > 100) {
         strcpy( fi-> errbuf, "quality must be in 0..100");
         return false;
      }   
      jpeg_set_quality(&l-> c, q, true /* limit to baseline-JPEG values */);      
   }   

   /* Optionally allow simple progressive output. */
   if ( pexist( progressive) && pget_B( progressive)) 
      jpeg_simple_progression(&l-> c);

   if ( l-> c. input_components == 3) { /* RGB */
      l-> buf = malloc( i-> lineSize);
      if ( !l-> buf) {
         strcpy( fi-> errbuf, "not enough memory");
         return false;
      }   
   }                    

   jpeg_start_compress( &l-> c, true);
   
   {
      Byte * src = i-> data + ( i-> h - 1) * i-> lineSize;
      while ( l-> c.next_scanline < i-> h ) {
	 JSAMPROW sarray[1];
         if ( l-> buf) {
            cm_reverse_palette(( PRGBColor) src, (PRGBColor) l-> buf, i-> w);
            sarray[0] = l-> buf;
         } else 
            sarray[0] = src;
	 jpeg_write_scanlines(&l-> c, sarray, 1);
         src -= i-> lineSize;
      }   
   } 
   jpeg_finish_compress( &l-> c);
   return true;
}   

static void 
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   SaveRec * l = ( SaveRec *) fi-> instance;
   free( l-> buf);
   jpeg_destroy_compress(&l-> c);
   free( l);
}

void 
apc_img_codec_jpeg( void )
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
