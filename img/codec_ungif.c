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

#include "img.h"
#include "img_conv.h"
#include "Image.h"
#include "Icon.h"
#include <gif_lib.h>

#ifdef __cplusplus
extern "C" {
#endif

static char * gifext[] = { "gif", nil };
static char * gifver[] = { "gif87a", "gif89a", nil };
static int    gifbpp[] = { 
   imbpp1, imbpp1 | imGrayScale,
   imbpp4, 
   imbpp8, imbpp8 | imGrayScale,
   0   
};   
static char * loadOutput[] = { 
   "screenWidth", 
   "screenHeight",
   "screenColorResolution",
   "screenBackGroundColor",
   "screenPalette",
   "delayTime",
   "disposalMethod",
   "userInput",
   "transparentColorIndex",
   "comment",
   "left",
   "top",
   "interlaced",
   nil
};   

static ImgCodecInfo codec_info = {
   "GIFLIB",
   "ftp://prtr-13.ucsc.edu/pub/libungif/",
   1, 0,      /* version */
   gifext,    /* extension */
   "Graphics Interchange Format",     /* file type */
   "GIF", /* short type */
   gifver,    /* features  */
   "Prima::Image::gif",     /* module */
   "Prima::Image::gif",     /* package */
   true,   /* canLoad */
   true,   /* canLoadMultiple  */
   true,   /* canSave */
   true,   /* canSaveMultiple */
   gifbpp, /* save types */
   loadOutput
};

static void * 
init( ImgCodecInfo ** info, void * param)
{
   char vd[1024];
   *info = &codec_info;
   sscanf( GIF_LIB_VERSION, "%s %d.%d", vd, &((*info)-> versionMaj), &((*info)-> versionMin));
   if ((*info)-> versionMaj > 4) EGifSetGifVersion( "89a");
   return (void*)1;
}  

typedef struct _LoadRec {
  GifFileType * gft;
  GifRecordType grt;
  int           passed;
  int           transparent;
  HV          * content;
} LoadRec;

static SV * 
make_palette_sv( ColorMapObject * pal)
{
   AV * av = newAV();
   SV * sv = newRV_noinc(( SV *) av);
   if ( pal) {
      int i;
      GifColorType * c = pal-> Colors;
      for ( i = 0; i < pal-> ColorCount; i++) {
         av_push( av, newSViv(( int) c-> Blue));
         av_push( av, newSViv(( int) c-> Green));
         av_push( av, newSViv(( int) c-> Red));
         c++;
      }   
   }   
   return sv;
}   

static void
copy_palette( PImage i, ColorMapObject * pal)
{
   int j;
   PRGBColor r = i-> palette;
   GifColorType * c = pal-> Colors;
   
   if ( !pal) return;
   i-> palSize = ( pal-> ColorCount > 256) ? 256 : pal-> ColorCount;
   for ( j = 0; j < i-> palSize; j++) {
      r-> r = c-> Red;
      r-> g = c-> Green;
      r-> b = c-> Blue;
      c++;
      r++;
   }   
}   

static void
format_error( int errorID, char * errbuf, int line)
{
   char * msg = nil;
   switch ( errorID) {
       case D_GIF_ERR_OPEN_FAILED:
       case E_GIF_ERR_OPEN_FAILED:
           msg = "open failed"; break;
       case D_GIF_ERR_READ_FAILED:
           msg = "read failed"; break;
       case D_GIF_ERR_NOT_GIF_FILE:
           msg = "not a valid GIF file"; break;
       case D_GIF_ERR_NO_SCRN_DSCR:
           msg = "no screen description block in the file"; break;
       case D_GIF_ERR_NO_IMAG_DSCR:
           msg = "no image description block in the file"; break;
       case D_GIF_ERR_NO_COLOR_MAP:
           msg = "no color map in the file"; break;
       case D_GIF_ERR_WRONG_RECORD:
           msg = "wrong record type"; break;
       case D_GIF_ERR_DATA_TOO_BIG:
           msg = "queried data size is too big"; break;
       case D_GIF_ERR_NOT_ENOUGH_MEM:
       case E_GIF_ERR_NOT_ENOUGH_MEM:
           msg = "not enough memory"; break;
       case D_GIF_ERR_CLOSE_FAILED:
       case E_GIF_ERR_CLOSE_FAILED:
           msg = "close failed"; break;
       case D_GIF_ERR_NOT_READABLE:
           msg = "file is not readable"; break;
       case D_GIF_ERR_IMAGE_DEFECT:
           msg = "image defect detected"; break;
       case D_GIF_ERR_EOF_TOO_SOON:
           msg = "unexpected end of file reached"; break;
       case E_GIF_ERR_WRITE_FAILED:
           msg = "write failed"; break;
       case E_GIF_ERR_HAS_SCRN_DSCR:
           msg = "screen descriptor already been set"; break;
       case E_GIF_ERR_HAS_IMAG_DSCR:
           msg = "image descriptor is still active"; break;
       case E_GIF_ERR_NO_COLOR_MAP:
           msg = "no color map specified"; break;
       case E_GIF_ERR_DATA_TOO_BIG:
           msg = "data is too big (exceeds size of the image)"; break;
       case E_GIF_ERR_DISK_IS_FULL:
           msg = "storage capacity exceeded"; break;
       case E_GIF_ERR_NOT_WRITEABLE:
           msg = "file opened not for writing"; break;
   }
   if ( msg) snprintf( errbuf, 256, "%s", msg);
}   

static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l = malloc( sizeof( LoadRec));
   HV * profile = fi-> fileProperties;
   
   if ( !l) return nil;
   memset( l, 0, sizeof( LoadRec));
   
   if ( !( l-> gft = DGifOpenFileName( fi-> fileName))) {
      free( l);
      return nil;
   }   
   fi-> stop = true;

   l-> content = newHV();
   l-> passed  = -1;
   l-> transparent = -1;

   /* safe to kill */
   fclose( fi-> f);
   fi-> f = NULL;

   if ( fi-> loadExtras) {
      pset_i( screenWidth,           l-> gft-> SWidth);
      pset_i( screenHeight,          l-> gft-> SHeight);
      pset_i( screenColorResolution, l-> gft-> SColorResolution);
      pset_i( screenBackGroundColor, l-> gft-> SBackGroundColor);
      pset_sv_noinc( screenPalette,  make_palette_sv( l-> gft-> SColorMap));
   }
   return l;
}

#pragma pack(1)
typedef struct _GIFGraphControlExt {
    Byte packedFields;
    U16 delayTime;
    Byte transparentColorIndex;
} GIFGraphControlExt, *PGIFGraphControlExt;
#pragma pack()

static void
load_extension( PImgLoadFileInstance fi, int code, Byte * data, Bool privateExtensions)
{
   LoadRec * l = ( LoadRec *) fi-> instance;
   HV * profile = l-> content;
   
   switch( code) {
   case GRAPHICS_EXT_FUNC_CODE: 
      data++;
      {
      PGIFGraphControlExt ext = ( PGIFGraphControlExt) data;
      Byte p = ext-> packedFields;
      if ( fi-> loadExtras) {
         pset_i( delayTime,      ext-> delayTime);
         pset_i( disposalMethod, ( p & 0x1c) >> 2);
         pset_i( userInput,      ( p & 2) ? 1 : 0);
      }
      if ( p & 1) {
         if ( fi-> loadExtras)
            pset_i( transparentColorIndex, ext-> transparentColorIndex);
         l-> transparent = ext-> transparentColorIndex;
      }    
      break;
   }   
   case COMMENT_EXT_FUNC_CODE: if ( fi-> loadExtras) {
      SV * sv = newSVpv( data + 1, *data);
      if ( privateExtensions && pexist( comment)) {
         /* enable long comments */
         sv_catsv( pget_sv( comment), sv);
         sv_free( sv);
      } else
         pset_sv_noinc( comment, sv); 
      }
      break;
   }                              
}   

#define out { format_error( GifLastError(), fi-> errbuf, __LINE__); return false;}
#define outc(x){ strncpy( fi-> errbuf, x, 256); return false;}
#define outcm(dd){ snprintf( fi-> errbuf, 256, "No enough memory (%d bytes)", dd); return false;}

static int interlaceOffs[] = { 0, 4, 2, 1};
static int interlaceStep[] = { 8, 8, 4, 2};

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
   PImage i = ( PImage) fi-> object;
   LoadRec * l = ( LoadRec *) fi-> instance;
   HV * profile = fi-> frameProperties; 
   Bool loop = true;
   Bool privateExtensions = true;

   /* Reopen file if rewind requested */
   if ( fi-> frame <= l-> passed) {
      DGifCloseFile( l-> gft);
      if ( !( l-> gft = DGifOpenFileName( fi-> fileName))) out;
      sv_free(( SV*) l-> content);   
      l-> content = newHV();
      l-> passed  = -1;
      l-> transparent = -1;
   }   

   while ( loop) {
      if ( DGifGetRecordType( l-> gft, &l-> grt) != GIF_OK) out;
      switch( l-> grt) {
         
      case SCREEN_DESC_RECORD_TYPE: 
         break;   
         
      case TERMINATE_RECORD_TYPE:
         fi-> frameCount = l-> passed + 1; /* can report how many frames now */
         outc("Frame index out of range");
         
      case IMAGE_DESC_RECORD_TYPE:
         if ( DGifGetImageDesc( l-> gft) != GIF_OK) out;

         if ( ++l-> passed != fi-> frame) { /* skip image block */
             int sz;
             Byte * block = nil;
             if ( DGifGetCode( l-> gft, &sz, &block) != GIF_OK) out;
             while ( block) {
                if ( DGifGetCodeNext( l-> gft, &block) != GIF_OK) out;
             }  
             privateExtensions = false;
             break;
         }   

         /* loading palette */
         { 
            int type = l-> gft-> Image.ColorMap ? l-> gft-> Image.ColorMap-> BitsPerPixel : 
                     ( l-> gft-> SColorMap      ? l-> gft-> SColorMap-> BitsPerPixel : imbpp1);
            if ( type != 1) type = ( type <= 4) ? 4 : 8;
            i-> self-> create_empty(( Handle) i, 1, 1, type);
         }
         
         if ( l-> gft-> Image.ColorMap != nil)
            copy_palette( i, l-> gft-> Image. ColorMap);
         else if ( l-> gft-> SColorMap) {
            copy_palette( i, l-> gft-> SColorMap);
            if ( fi-> loadExtras) pset_i( useScreenPalette, 1);
         } else
            i-> palSize = 0;

         if ( fi-> loadExtras) {
            pset_i( interlaced, l-> gft-> Image. Interlace ? 1 : 0);
            pset_i( left,       l-> gft-> Image. Left);
            pset_i( top,        l-> gft-> Image. Top); 
         }   

         if ( fi-> noImageData) {
            pset_i( width,      l-> gft-> Image. Width);
            pset_i( height,     l-> gft-> Image. Height);
         } else {
            GifPixelType * data;
            int ls = sizeof( GifPixelType) * l-> gft-> Image. Width;
            i-> self-> create_empty(( Handle) i, 
                l-> gft-> Image. Width, l-> gft-> Image. Height, i-> type);
            data = ( GifPixelType *) malloc( ls * i-> h);
            if ( !data) outcm( ls * i-> h);
            if ( DGifGetLine( l-> gft, data, i-> w * i-> h) != GIF_OK) {
               free( data);
               out;
            }   

            /* copying & converting data */
            {
               int j, pass = 0, idst = 0;
               Byte * src = data, *dst = i-> data;

               for ( j = 0; j < i-> h; j++) {
                  dst = i-> data + ( i-> h - 1 - idst) * i-> lineSize;
                  if ( l-> gft-> Image. Interlace) {
                     idst += interlaceStep[ pass];
                     if ( idst >= i-> h && pass < 3)
                        idst = interlaceOffs[ ++pass];
                  } else 
                     idst++;
        
                  switch( i-> type & imBPP) {
                  case imbpp1:
                     bc_byte_mono_cr( src, dst, i-> w, map_stdcolorref);   break;
                  case imbpp4:
                     bc_byte_nibble_cr( src, dst, i-> w, map_stdcolorref); break;
                  default:
                     memcpy( dst, src, i-> w);
                  }   
                  src += ls;
               }   
            }   

            free( data);

            /* applying transparent index to Icon */
            if ( kind_of( fi-> object, CIcon) && 
                 ( l-> transparent >= 0) &&
                 ( l-> transparent < PIcon( fi-> object)-> palSize)) {
               PRGBColor p = PIcon( fi-> object)-> palette;
               p += l-> transparent;
               PIcon( fi-> object)-> maskColor = ARGB( p->r, p-> g, p-> b);
               PIcon( fi-> object)-> autoMasking = amMaskColor;
            }   
         }   
         
         loop = false;
         break;
         
      case EXTENSION_RECORD_TYPE:   
         {
            int code = -1;
            Byte * data = nil;
            if ( DGifGetExtension( l-> gft, &code, &data) != GIF_OK) out;
            if ( data)
               load_extension( fi, code, data, privateExtensions);
            while ( data) {
               if ( DGifGetExtensionNext( l-> gft, &data) != GIF_OK) out;
               if ( data)
                  load_extension( fi, code, data, privateExtensions);
            } 
            privateExtensions = true;
         }   
         break;
      default:;
      }   
   }   

   /* frame loaded successfully, add slice of extra info */
   if ( fi-> loadExtras) {
      apc_img_profile_add( profile, l-> content, l-> content);
      pset_i( privateExtensions, privateExtensions);
   }   
   
   return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   LoadRec * l = ( LoadRec *) fi-> instance;
   sv_free(( SV*) l-> content);
   DGifCloseFile( l-> gft);
   free( l);
}   

static HV *
save_defaults( PImgCodec c)
{
   HV * profile = newHV();
   AV * av = newAV();
   pset_i( screenWidth,  -1);
   pset_i( screenHeight, -1);
   pset_i( screenColorResolution, 256);
   pset_i( screenBackGroundColor, 0);

      av_push( av, newSViv(0));
      av_push( av, newSViv(0));
      av_push( av, newSViv(0));
      av_push( av, newSViv(0xff));
      av_push( av, newSViv(0xff));
      av_push( av, newSViv(0xff));
   pset_sv_noinc( screenPalette, newRV_noinc(( SV *) av));

   pset_i( delayTime,      1);
   pset_i( disposalMethod, 0);
   pset_i( userInput,      0);
   pset_i( transparentColorIndex, 0);

   pset_c( comment, "");

   pset_i( left, 0);
   pset_i( top,  0);
   pset_i( interlaced, 0);
   
   return profile;
}


static void * 
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   GifFileType * g;
   if ( !( g = EGifOpenFileName( fi-> fileName, 0)))
      return nil;

   /* safe to kill */
   fclose( fi-> f);
   fi-> f = NULL;

   return g;
}

static ColorMapObject * 
make_colormap( PRGBColor r, int sz)
{
   int j;
   ColorMapObject * ret = MakeMapObject( sz, nil);
   GifColorType   * c;   
   if ( !ret) return nil;
   c = ret-> Colors;
   for ( j = 0; j < sz; j++) {
      c-> Red   = r-> r;
      c-> Green = r-> g;
      c-> Blue  = r-> b;
      c++;
      r++;
   }   
   return ret;
}   


static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   GifFileType * g = ( GifFileType *) fi-> instance; 
   PImage i = ( PImage) fi-> object;
   HV * profile = fi-> objectExtras;

   if ( fi-> frame == 0) {  
      /* put screen description */
      int w = i-> w, h = i-> h, cr = i-> palSize, bg = 0, ps = i-> palSize;
      RGBColor * r = i-> palette;
      RGBColor rgbc[ 256];
      ColorMapObject * c;
    
      if ( pexist( screenWidth))           w  = pget_i( screenWidth);
      if ( pexist( screenHeight))          h  = pget_i( screenHeight);
      if ( pexist( screenColorResolution)) cr = pget_i( screenColorResolution);
      if ( pexist( screenBackGroundColor)) bg = pget_i( screenBackGroundColor);
      if ( pexist( screenPalette)) 
         ps = apc_img_read_palette( r = rgbc, pget_sv( screenPalette));
      c = make_colormap( r, ps);
      if ( !c) outcm( ps * 3);
      if ( w < 0) w = i-> w;
      if ( h < 0) h = i-> h;
      if (EGifPutScreenDesc( g, w, h, c-> BitsPerPixel, bg, c) != GIF_OK) {
         FreeMapObject( c);
         out;
      }
      FreeMapObject( c);
   }   
   
   /* writing extras */

   /* comments  */
   if ( pexist( comment)) {
      char * c = pget_c( comment);
      int len  = strlen( c);
      while ( len > 0) {
         char c1[ 256];
         strncpy( c1, c, 256);
         c1[255] = 0;
         c += 255;
         len -= 255;
         EGifPutComment( g, c1);
      }
   }   
   
   /* graphics control block */
   if ( pexist( delayTime)      || 
        pexist( disposalMethod) ||
        pexist( userInput)      ||
        pexist( transparentColorIndex)) {

      GIFGraphControlExt ext = { 0, 0, 0};
      int disposalMethod = 0, userInput = 0, transparent = 0;
      
      if ( pexist( delayTime))      ext. delayTime = pget_i( delayTime);
      if ( pexist( disposalMethod)) disposalMethod = pget_i( disposalMethod);
      if ( pexist( userInput))      userInput      = pget_i( userInput);
      if ( pexist( transparentColorIndex)) {
         int t = pget_i( transparentColorIndex);
         if ( t >= 0) {
            transparent = 1;
            ext. transparentColorIndex = t;
         }
      }   
      if ( disposalMethod < 0 || disposalMethod > 3) outc("disposalMethod not in 0..3");
      ext. packedFields = 
         ( transparent    ? 1 : 0) |
         ( userInput      ? 2 : 0) |
         (( disposalMethod & 7) << 2);
      if ( EGifPutExtension( g, GRAPHICS_EXT_FUNC_CODE, 
         sizeof( GIFGraphControlExt), &ext) != GIF_OK)
         out;
   }

   /* writing image */
   {
      ColorMapObject * c = make_colormap( i-> palette, i-> palSize);
      int ox = 0, oy = 0, interlaced = 0;

      GifPixelType * data;
      int ls = sizeof( GifPixelType) * i-> w;
      int j, pass = 0, idst = 0;
      Byte * src, * dst;
      
      if ( !c) outcm( i-> palSize * 3);
      if ( pexist( left)) ox = pget_i( left);
      if ( pexist( top )) oy = pget_i( top);
      if ( pexist( interlaced )) interlaced = pget_i( interlaced);
      
      if ( EGifPutImageDesc( g, ox, oy, i-> w, i-> h, interlaced, c) != GIF_OK) {
         FreeMapObject( c);
         out;
      }
      FreeMapObject( c);

      data = malloc( ls * i-> h);
      if ( !data) outcm( ls * i-> h);
      dst = data;

      /* copying & converting data */
      for ( j = 0; j < i-> h; j++) {
         src = i-> data + ( i-> h - 1 - idst) * i-> lineSize;
         if ( interlaced) {
            idst += interlaceStep[ pass];
            if ( idst >= i-> h && pass < 3)
               idst = interlaceOffs[ ++pass];
         } else
            idst++;
         
         switch( i-> type & imBPP) {
         case imbpp1:
            bc_mono_byte( src, dst, i-> w); break;
         case imbpp4:
            bc_nibble_byte( src, dst, i-> w); break;
         default:
            memcpy( dst, src, i-> w);         
         } 
         dst += ls;   
      }   
      if ( EGifPutLine( g, data, i-> w * i-> h) != GIF_OK) {
         free( data);
         out;
      }
      free( data);
   }
   return true;
}   


static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   EGifCloseFile(( GifFileType *) fi-> instance);
}   


void 
apc_img_codec_ungif( void )
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

#undef out   
#undef outc   

#ifdef __cplusplus
}
#endif

