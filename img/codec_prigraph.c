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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */

#include <fcntl.h>
#include <sys/types.h>
#include "img.h"
#include "gbm.h"
#include "img_conv.h"
#include "Image.h"

#ifdef __cplusplus
extern "C" {
#endif


#define itUnknown (-1)
#define itBMP  0
#define itGIF  1
#define itPCX  2
#define itTIF  3
#define itTGA  4
#define itLBM  5
#define itVID  6
#define itPGM  7
#define itPPM  8
#define itKPS  9
#define itIAX  10
#define itXBM  11
#define itSPR  12
#define itPSG  13
#define itGEM  14
#define itCVP  15
#define itJPG  16
#define itPNG  17
#define itMAX  17

static const char * gbm_ids[itMAX+1] = {
	"Bitmap",
	"GIF",
	"PCX",
	"TIFF",
	"Targa",
	"ILBM",
	"YUV12C",
	"Greymap",
	"Pixmap",
	"KIPS",
	"IAX",
	"XBitmap",
	"Sprite",
	"PSEG",
	"GemRas",
	"Portrait",
	"JPEG",
	"PNG"
};

static int gbm_ft_map[itMAX+1];

static int    t_all[] = { 
   imbpp1,  imbpp1  | imGrayScale,
   imbpp4,  
   imbpp8,  imbpp8  | imGrayScale,
   imbpp24,
   0
};

static int t_no24[] = { 
   imbpp1,  imbpp1  | imGrayScale,
   imbpp4,  
   imbpp8,  imbpp8  | imGrayScale,
   0
};   

static int t_1g[] = { 
   imbpp1  | imGrayScale,
   0
};   

static int t_1[] = { 
   imbpp1,  imbpp1  | imGrayScale,
   0
};   

static int t_8[] = { 
   imbpp8,  imbpp8  | imGrayScale,
   0
};   

static int t_24[] = { 
   imbpp24, 
   0
};   

static int t_targa[] = { 
   imbpp8,  imbpp8  | imGrayScale,
   imbpp24, 
   0
};   

static ImgCodecInfo codec_info = {
   "Generalised Bitmap Module",
   "GBM",
   0, 0,   // version
   nil,    // extension
   "",     // file type
   "",     // short type
   nil,    // features 
   "",     // module
   "",     // package
   IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM,
   nil,    // save types
};

static int refCnt = 0;

#define MAX_FAKE_FD  32
static PImgIORequest fdmap[MAX_FAKE_FD+1];

static int disabled_codecs[itMAX+1], n_disabled_codecs = 0;

static int  
std_open(const char *fn, int mode)
{
     int i;

     /* Normal use from within GBM is discouraged, only our code
	can call gbm io */
     if ( mode >= 0) return -1;

     for ( i = 0; i <= MAX_FAKE_FD; i++) {
	if ( fdmap[i] == NULL) {
	   fdmap[i] = ( PImgIORequest) fn;
	   return i;
	}
     }
     return -1;
}

static int  
std_create(const char *fn, int mode)
{
     return std_open( fn, mode);
}

static void 
std_close (int fd)
{
     fdmap[ fd ] = NULL;
}

static long 
std_lseek(int fd, long pos, int whence)
{
     if ( req_seek( fdmap[fd], pos, whence) < 0)
	  return -1;
     return req_tell( fdmap[fd]);
}
static int  
std_read(int fd, void *buf, int len)
{
   return req_read( fdmap[fd], len, buf);
}

static int  
std_write(int fd, const void *buf, int len)
{
   return req_write( fdmap[fd], len, (void*) buf);
}

static Bool
codec_is_enabled( int ft)
{
   int i;
   ft = gbm_ft_map[ft];
   for ( i = 0; i < n_disabled_codecs; i++) {
      if ( disabled_codecs[i] == ft)
         return false;
   }
   return true;
}


static void * 
init( ImgCodecInfo ** info, void * param)
{
   GBMFT gft;
   char * c, ** array;
   int count = 0, len, i;
   
   if ( refCnt++ == 0) {
      bzero( fdmap, sizeof(fdmap));
      gbm_io_setup( std_open, std_create, std_close, std_lseek, std_read, std_write);
      codec_info. versionMaj = gbm_version() / 100;
      codec_info. versionMin = gbm_version() % 100;
   }

   gbm_query_filetype(( int) param, &gft);
   
   *info = ( ImgCodecInfo *) malloc( sizeof( ImgCodecInfo));
   if ( !*info) {
      if ( --refCnt == 0) gbm_deinit(); 
      return false;
   } 

   memcpy( *info, &codec_info, sizeof( ImgCodecInfo));
   (*info)-> fileType = gft. long_name;
   (*info)-> fileShortType = gft. short_name;
   c = gft. extensions;
   while ( 1) {
      if ( *c == '\0' || *c == ' ') count++;
      if ( *c == '\0') break;
      c++;
   }   
   c = gft. extensions;
   len = strlen( c);
   array = (*info)-> fileExtensions = 
      ( char**) malloc( len + 1 + (count + 1) * sizeof( char*));
   if ( !array) {
      if ( --refCnt == 0) gbm_deinit(); 
      return false;
   }
   c = ( char *)(( Byte*) array + (count + 1) * sizeof( char*));
   memcpy( c, gft. extensions, len + 1);
   *array = c;
   while ( *c) {
      if ( *c == ' ') {
         *c = '\0';
         *(++array) = ++c;
      } else {
         *c = tolower( *c);
         c++;
      }   
   }   
   *(++array) = nil;

   switch ( gbm_ft_map[(int)param] ) {
      case itGIF:
         (*info)-> IOFlags |= IMG_LOAD_MULTIFRAME;
         (*info)-> saveTypes = t_no24;
         (*info)-> primaModule  = "Prima::Image::GBM";
         (*info)-> primaPackage = "Prima::Image::GBM::gif";
         break;
      case itLBM:
         (*info)-> saveTypes = t_no24;
         (*info)-> primaModule  = "Prima::Image::GBM";
         (*info)-> primaPackage = "Prima::Image::GBM::lbm";
         break;
      case itSPR:
         (*info)-> saveTypes = t_no24;
         break;
      case itPGM:
      case itKPS:
         (*info)-> saveTypes = t_8;
         break;
      case itPPM:
      case itVID:
         (*info)-> saveTypes = t_24;
         break;
      case itJPG:
         (*info)-> saveTypes = t_24;
         (*info)-> primaModule  = "Prima::Image::jpeg";
         (*info)-> primaPackage = "Prima::Image::jpeg";
         break;
      case itPSG:
         (*info)-> saveTypes = t_1;
         break;
      case itXBM:
         (*info)-> saveTypes = t_1g;
         break;
      case itCVP:   
      case itIAX:
         (*info)-> IOFlags &= ~(IMG_SAVE_TO_FILE|IMG_SAVE_TO_STREAM);
         break;
      case itTGA:
         (*info)-> saveTypes = t_targa;
         break;
      default:
         (*info)-> saveTypes = t_all;
   }

   switch ( gbm_ft_map[(int)param] ) {
   case itTGA:
   case itIAX:
   case itXBM:
   case itSPR:
   case itPSG:
   case itGEM:
   case itCVP:
   /* XXX These actually _can_ load from stream, but gbm cannot autodetect which sub-codec
      to use when there is no file extension.  */
   case itKPS:
   /* KPS loads from 2 files, .KPS and .PAL, so no stream by definition */
         (*info)-> IOFlags &= ~IMG_LOAD_FROM_STREAM;
   }

   return (void*)1;
}   

static void
done( PImgCodec instance)
{
   if ( --refCnt == 0) gbm_deinit();
   free( instance-> info-> fileExtensions);
   free( instance-> info);
}   


typedef struct _ImageSignatures
{
   int type;
   int size;
   char *sig;
} ImageSignatures;


static ImageSignatures signatures[] =
{
   { itBMP, 2, "BM" },
   { itGIF, 6, "GIF87a" },
   { itGIF, 6, "GIF89a" },
   { itPCX, 3, "\x0a\x04\x01" },
   { itPCX, 3, "\x0a\x05\x01" },
   { itTIF, 2, "II" },
   { itTIF, 2, "MM" },
   /* { itTGA, ?, ? }, */
   { itLBM, 4, "FORM" },
   { itVID, 6, "YUV12C" },
   { itPGM, 2, "P5" },
   { itPPM, 2, "P6" },
   { itKPS, 8, "DFIMAG00" },
   /* { itIAX, */
   /* { itXBM, */
   /* { itSPR, */
   /* { itPSG, */
   /* { itGEM, */
   /* { itCVP, */
   { itJPG, 2, "\xff\xd8" },
   { itPNG, 8, "\x89PNG\r\n\x1a\n"}
};

#define N_SIGS ( sizeof( signatures) / sizeof( signatures[ 0]))

static Bool
type_ok( PImgIORequest req, int ft)
{
   char buf[ 8];
   int i;

   if ( !codec_is_enabled(ft)) return false;

   memset( buf, 0, 8);
   if ( req_seek( req, 0, SEEK_SET) < 0) return false;
   if ( req_read( req, 8, buf) < 8) return false;
   if ( req_seek( req, 0, SEEK_SET) < 0) return false;
   for ( i = 0; i < N_SIGS; i++) {
      if (( signatures[ i]. type == ft) &&
          ( memcmp( buf, signatures[ i]. sig, signatures[ i]. size) == 0))
         return true;
   }    
   return false;
}

typedef struct _GBMRec {
   GBM      gbm;
   char   * params; 
   char   * statParams;
   int      ft;
   int      ft_gbm;
   int      fd;
} GBMRec;   

static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   GBMRec * g;

   if ( !type_ok( fi-> req, gbm_ft_map[(int)(instance-> initParam)])) {
      int ft;
      if ( fi-> fileName && gbm_guess_filetype( fi-> fileName, &ft) != 0)
         return nil;
      if ( ft != (int)(instance-> initParam)) 
         return nil;
      if ( !codec_is_enabled( ft))
         return nil;
   }  

   fi-> stop = true;

   g = ( GBMRec*) malloc( sizeof( GBMRec) + 0x100);
   if ( !g) return nil;

   memset( g, 0, sizeof( GBMRec) + 0x100);
   g-> params  = ( char  *)((( Byte *) g) + sizeof( GBMRec));
   g-> statParams = g-> params;
   g-> ft_gbm = (int) (instance-> initParam);
   g-> ft     = gbm_ft_map[ g-> ft_gbm ];
   if ( g-> ft == itBMP) {
      strcat( g-> params, "inv ");
      g-> params += 4;
   }

   if ( g-> ft == itGIF) 
      fi-> frameCount = -1;
   else
      fi-> frameCount = 1;

   if (( g-> fd = gbm_io_open(( const char*) fi-> req, -1)) < 0) {
      free( g);
      return nil;      
   }  

   return g;
}

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
   GBMRec * g = ( GBMRec *) fi-> instance; 
   GBM_ERR rc;
   PImage i = ( PImage) fi-> object;

   snprintf( g-> params, 250, "index=%d", fi-> frame);
   
   if (( rc = gbm_read_header( 
	fi-> fileName ? fi-> fileName : "", 
	g-> fd, g-> ft_gbm, &g-> gbm, g-> statParams)) != 0) {
      strncpy( fi-> errbuf, gbm_err( rc), 256);
      return false;
   }
   
   if ( g-> gbm.bpp != 1 && g-> gbm.bpp != 4 &&
        g-> gbm.bpp != 8 && g-> gbm.bpp != 24) {
      snprintf( fi-> errbuf, 256, "Bits per pixel %d is unsupported", g-> gbm.bpp);
      return false;
   }   

   CImage( fi-> object)-> create_empty( fi-> object, 1, 1, g-> gbm. bpp);
   
   if ( g-> gbm.bpp <= 8) {
      if (( rc = gbm_read_palette( g-> fd, g-> ft_gbm, &g-> gbm, 
             ( GBMRGB *) i-> palette)) != 0) {
         strncpy( fi-> errbuf, gbm_err( rc), 256);
         return false;
      }   
      i-> palSize = 1 << g-> gbm.bpp;
      cm_reverse_palette( i-> palette, i-> palette, 256);
   } else
      i-> palSize = 0;

   if ( fi-> noImageData) {
      hv_store( fi-> frameProperties, "width",  5, newSViv( g-> gbm. w), 0);
      hv_store( fi-> frameProperties, "height", 6, newSViv( g-> gbm. h), 0);
      return true;
   }   
   
   CImage( fi-> object)-> create_empty( fi-> object, 
      g-> gbm. w, g-> gbm. h, i-> type); 

   /* free unused palette entries */
   if ( i-> palSize > 1) {
      int x = i-> palSize - 1;
      Color last = ARGB(i->palette[x].r,i->palette[x].g,i->palette[x].b), curr;
      while (1) {
         x--;
         curr = ARGB(i->palette[x].r,i->palette[x].g,i->palette[x].b);
         if ( curr != last) break;
         i-> palSize--;
      }
      for ( x = 0; x < i-> palSize - 2; x++) {
         curr = ARGB(i->palette[x].r,i->palette[x].g,i->palette[x].b);
         if ( curr == last) {
            i-> palSize--;
            break;
         }
      }
   }

   if (( rc = gbm_read_data( g-> fd, g-> ft_gbm, &g-> gbm, PImage( fi-> object)-> data)) != 0) {
      strncpy( fi-> errbuf, gbm_err( rc), 256);
      return false;
   }  

   return true; 
}   


static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   GBMRec * g = ( GBMRec *) fi-> instance;
   gbm_io_close( g-> fd);
   free( g);
}   

static HV *
save_defaults( PImgCodec c)
{
   HV * profile = newHV();
   /*
     png      - compressionLevel       "zlevel=%d"
              - compression(standard,huffman,filtered) "zstrategy=%c"(d,h,f)
              - interlaced             "ilace"
              - description            "Description=%s"
              - noAutoFilter           "nofilters"
              - transparent color index"transcol=%d"
              - transparent color      "transcol=%d/%d/%d"
   */
   switch(( int) c-> initParam) {
   case itGIF:
      pset_i( interlaced, 0);
      pset_i( transparentColorIndex, -1);
      break;   
   case itLBM:
      pset_i( transparentColorIndex, -1);
      break;   
   case itTIF:
      pset_i( compressed, 1); 
      pset_c( description, ""); 
      break;   
   case itJPG:
      pset_i( progressive, 1);
      pset_i( quality, 75); 
      break;   
   case itPNG:
      pset_i( transparentColorIndex, -1);
      pset_i( noAutoFilter, 0);
      pset_i( compressionLevel, -1); // GBM knows better
      pset_c( compression, ""); // alias "standard"
      pset_c( description, ""); 
      break;   
   }   
   return profile;
}


static void * 
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   GBMRec * g;

   /* Don't care about fi-> append, we don't save multiframes;
      that means, we don't care about recognizing file on save
      altogether. That means also that all requests can be 
      processed inside  save(), but let's keep open_save and 
      close_save that way. The only bad thing is that we can't
      complain about not supported image format on early stage */
      
   g = ( GBMRec*)malloc( sizeof( GBMRec) + 0x400);
   if ( !g) return nil;

   memset( g, 0, sizeof( GBMRec) + 0x400);
   g-> params  = ( char  *)((( Byte *) g) + sizeof( GBMRec));
   g-> statParams = g-> params;
   g-> ft_gbm = (int) (instance-> initParam);
   g-> ft     = gbm_ft_map[ g-> ft_gbm ];
   if ( g-> ft == itBMP) {
      strcat( g-> params, "inv ");
      g-> params += 4;
   }   

   if (( g-> fd = gbm_io_create(( const char*) fi-> req, -1)) < 0) {
      free( g);
      return nil;      
   }  

   return g;
}

static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   dPROFILE;
   GBMRec * g = ( GBMRec *) fi-> instance; 
   GBM_ERR rc;
   PImage i = ( PImage) fi-> object;
   RGBColor pal[ 256];
   char oneOpt[ 256];
   HV * profile = fi-> objectExtras;

   g-> gbm. w = i-> w;
   g-> gbm. h = i-> h;
   g-> gbm. bpp = i-> type & imBPP;

   /*
     gif, iff, lbm - transparent color "transcol=%d"
     gif      - interlaced             "ilace"
     tiff     - compressed             "lzw"
              - description            "imagedescription=%s"
     jpeg     - quality                "quality=%d"
              - progressive            "prog"
     png      - compressionLevel       "zlevel=%d"
              - compression(standard,huffman,filtered) "zstrategy=%c"(d,h,f)
              - interlaced             "ilace"
              - description            "Description=%s"
              - noAutoFilter           "nofilters"
              - transparent color index"transcol=%d"
              - transparent color      "transcol=%d/%d/%d"
   */

   if ( g-> ft == itGIF) 
      if ( pexist( interlaced) && pget_B( interlaced)) 
           strcat(g-> params, " ilace");
   
   if ( g-> ft == itTIF) {
      if ( pexist( compressed) && pget_B( compressed))
         strcat(g-> params, " lzw");
      
      if ( pexist( description) && strlen( pget_c( description))) {
         char *cc = oneOpt;
         snprintf(oneOpt,256," imagedescription=%s",(char *) pget_c( description));
         while (*++cc) if (*cc == ' ') *cc='_';
         strcat( g-> params, oneOpt);
      }
   }   
   
   if ( g-> ft == itJPG) {
      if ( pexist( progressive) && pget_B( progressive))  
         strcat(g-> params, " prog");
      if ( pexist( quality) && pget_i( quality) >= 0 )
         strcat(g-> params, (snprintf(oneOpt, 256, " quality=%d",(int) pget_i( quality)), oneOpt));
   }   
   
   if ( g-> ft == itPNG) {
      if ( pexist( noAutoFilter) && pget_B( noAutoFilter)) 
         strcat(g-> params, " nofilters");
      if ( pexist( compressionLevel) && (pget_i( compressionLevel) >= 0))
         strcat(g-> params, (snprintf(oneOpt, 256, " zlevel=%d",(int) pget_i( compressionLevel)), oneOpt));
      if ( pexist( compression) && strlen( pget_c( compression))) {
         char * c = pget_c( compression);
         if ( strcmp( c, "standard") == 0) strcat(g-> params, " zstrategy=d");
         else if ( strcmp( c, "huffman") == 0) strcat(g-> params, " zstrategy=h");
         else if ( strcmp( c, "filtered") == 0) strcat(g-> params, " zstrategy=f");
      }
      if ( pexist( description) && strlen( pget_c( description)))
         strcat(g-> params, (snprintf(oneOpt,256, " Description=\"%s\"",(char *) pget_c( description)), oneOpt));
   }

   if ( g-> ft == itGIF || g-> ft == itPNG || g-> ft == itLBM) {
      if ( pexist( transparentColorIndex) && pget_i( transparentColorIndex) >= 0)
         strcat(g-> params, (snprintf(oneOpt, 256, " transcol=%d",(int) pget_i( transparentColorIndex)), oneOpt));
   }

   memset( pal, 0, sizeof( pal));
   cm_reverse_palette( i-> palette, pal, i-> palSize);

   if (( rc = gbm_write( 
      fi-> fileName ? fi-> fileName : "gbm", 
      g-> fd, g-> ft_gbm, &g-> gbm, ( GBMRGB *) pal, i-> data, g-> statParams)) != 0
   ) {
      strncpy( fi-> errbuf, gbm_err( rc), 256);
      return false;
   }    
   
   return true; 
}   

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   GBMRec * g = ( GBMRec *) fi-> instance;
   gbm_io_close( g-> fd);
   free( g);
}   

void 
apc_img_codec_prigraph( void )
{
   int i, j, nft;
   struct ImgCodecVMT vmt;
   memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
   vmt. init       = init;      
   vmt. done       = done;
   vmt. open_load  = open_load;
   vmt. load       = load; 
   vmt. close_load = close_load; 
   vmt. save_defaults = save_defaults;
   vmt. open_save  = open_save;
   vmt. save       = save; 
   vmt. close_save = close_save; 
   
   /* relinquish support for non-prigraph codecs */
   for ( i = 0; i < imgCodecs. count; i++) {
      char * t;
      PImgCodec c = ( PImgCodec ) ( imgCodecs. items[ i]);
      /* we initialize all codecs here, not good, these will begin loading
         DLLs -- but anyway this is temporary because the plan is for
	 prigraph to go away */
      if ( !c-> instance)
         c-> instance = c-> vmt-> init( &c->info, c-> initParam);
      if ( !c-> instance)
         continue;
      if ( !c-> info)
         continue;
      t = c-> info-> fileShortType;
      if ( !t)
         continue;

      if ( strcmp( t, "BMP") == 0)
         disabled_codecs[ n_disabled_codecs++ ] = itBMP;
      else if ( strcmp( t, "GIF") == 0)
         disabled_codecs[ n_disabled_codecs++ ] = itGIF;
      else if ( strcmp( t, "JPEG") == 0)
         disabled_codecs[ n_disabled_codecs++ ] = itJPG;
      else if ( strcmp( t, "PNG") == 0)
         disabled_codecs[ n_disabled_codecs++ ] = itPNG;
      else if ( strcmp( t, "TIFF") == 0)
         disabled_codecs[ n_disabled_codecs++ ] = itTIF;
      else if ( strcmp( t, "XBM") == 0)
         disabled_codecs[ n_disabled_codecs++ ] = itXBM;
   }
      
   /* KPS cannot load from single file, requires .PAL */
   disabled_codecs[ n_disabled_codecs++ ] = itKPS;

   gbm_init();
   
   /* fill map */
   gbm_query_n_filetypes(&nft);
   for ( i = 0; i < nft; i++) {
      GBMFT gft;
      gbm_query_filetype( i, &gft);
      for ( j = 0; j <= itMAX; j++) {
         if ( strcmp( gft. short_name, gbm_ids[j]) == 0) {
            gbm_ft_map[i] = j;
            break;
         }
      }
   }
   /* register codecs */
   for ( i = 0; i < nft; i++)
      if ( codec_is_enabled(i))
         apc_img_register( &vmt, (void*)i);
}  

#ifdef __cplusplus
}
#endif
