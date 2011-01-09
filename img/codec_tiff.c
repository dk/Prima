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
 * Created by Dmitry Karasik <dmitry@karasik.eu.org> with great help
 * of tiff2png.c by Willem van Schaik and Greg Roelofs
 *
 * $Id$
 */

#include "img.h"
#include "img_conv.h"
#include "Icon.h"
#if defined(_MSC_VER) && _MSC_VER < 1500
#define HAVE_INT32
#endif
#include <tiff.h>
#include <tiffio.h>
#include <tiffconf.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PHOTOMETRIC_DEPTH
#  define PHOTOMETRIC_DEPTH 32768
#endif


static char * tiffext[] = { "tif", "tiff", nil };
static int    tiffbpp[] = { imbpp24, 
                            imbpp8, imByte, 
                            imShort, 
                            imbpp4, imbpp4 | imGrayScale,
                            imbpp1, imbpp1 | imGrayScale, 
                            0 };   
static char * loadOutput[] = { 
   "Photometric",
   "BitsPerSample",
   "SamplesPerPixel",
   "PlanarConfig",
   "Tiled",

   "Artist",
   "CompressionType", 
   /* tibtiff can decompress many types; but compress a few, so CompressionType
      is named so to avoid implicit but impossible compression selection */
   "Copyright",
   "DateTime",
   "DocumentName",
   "HostComputer",
   "ImageDescription",
   "Make",
   "Model",
   "PageName",
   "PageNumber",
   "PageNumber2",
   "ResolutionUnit",
   "Software",
   "XPosition",
   "YPosition",
   "XResolution",
   "YResolution",
   nil
};

static char * tifffeatures[] = { 
#ifdef COLORIMETRY_SUPPORT
   "Tag-COLORIMETRY",
#endif
#ifdef	YCBCR_SUPPORT
   "Tag-YCBCR",
#endif
#ifdef	CMYK_SUPPORT
   "Tag-CMYK",
#endif
#ifdef	ICC_SUPPORT
   "Tag-ICC",
#endif
#ifdef PHOTOSHOP_SUPPORT
   "Tag-PPHOTOSHOP",
#endif
#ifdef IPTC_SUPPORT
   "Tag-IPTC",
#endif
#ifdef	CCITT_SUPPORT
   "Compression-CCITT",
#endif
#ifdef	PACKBITS_SUPPORT
   "Compression-PACKBITS",
#endif
#ifdef	LZW_SUPPORT
   "Compression-LZW",
#endif
#ifdef	THUNDER_SUPPORT
   "Compression-THUNDER",
#endif
#ifdef	NEXT_SUPPORT
   "Compression-NEXT",
#endif
#ifdef  LOGLUV_SUPPORT
   "Compression-SGILOG",
   "Compression-SGILOG24",
#endif
#ifdef  JPEG_SUPPORT
   "Compression-JPEG",
#endif
   nil
};

typedef struct {
   int tag;
   char * name;
} CompType;

static CompType comptable [] = {
  { COMPRESSION_NONE            , "NONE"},
  { COMPRESSION_CCITTRLE        , "CCITTRLE"},
  { COMPRESSION_CCITTFAX3       , "CCITTFAX3"},
  { COMPRESSION_CCITTFAX4       , "CCITTFAX4"},
  { COMPRESSION_LZW             , "LZW"},
  { COMPRESSION_OJPEG           , "OJPEG"},
  { COMPRESSION_JPEG            , "JPEG"},
  { COMPRESSION_NEXT            , "NEXT"},
  { COMPRESSION_CCITTRLEW       , "CCITTRLEW"},
  { COMPRESSION_PACKBITS        , "PACKBITS"},
  { COMPRESSION_THUNDERSCAN     , "THUNDERSCAN"},
  { COMPRESSION_IT8CTPAD        , "IT8CTPAD"},
  { COMPRESSION_IT8LW           , "IT8LW"},
  { COMPRESSION_IT8MP           , "IT8MP"},
  { COMPRESSION_IT8BL           , "IT8BL"},
  { COMPRESSION_PIXARFILM       , "PIXARFILM"},
  { COMPRESSION_PIXARLOG        , "PIXARLOG"},
  { COMPRESSION_DEFLATE         , "DEFLATE"},
  { COMPRESSION_ADOBE_DEFLATE   , "ADOBE_DEFLATE"},
  { COMPRESSION_DCS             , "DCS"},
  { COMPRESSION_JBIG            , "JBIG"},
  { COMPRESSION_SGILOG          , "SGILOG"},
  { COMPRESSION_SGILOG24        , "SGILOG24"},
};

static ImgCodecInfo codec_info = {
   "TIFF Bitmap",
   "www.libtiff.org",
   TIFF_VERSION, TIFFLIB_VERSION,    /* version */
   tiffext,    /* extension */
   "Tagged Image File Format",  /* file type */
   "TIFF", /* short type */
   tifffeatures,    /* features  */
   "Prima::Image::tiff",     /* module */
   "Prima::Image::tiff",     /* package */
   IMG_LOAD_FROM_FILE | IMG_LOAD_MULTIFRAME | IMG_LOAD_FROM_STREAM | 
   IMG_SAVE_TO_FILE | IMG_SAVE_MULTIFRAME | IMG_SAVE_TO_STREAM,
   tiffbpp, /* save types */
   loadOutput
};

#define outcm(dd) snprintf( fi-> errbuf, 256, "No enough memory (%d bytes)", (int)dd)
#define outc(x)   strncpy( fi-> errbuf, x, 256)

static char * errbuf = nil;
static Bool err_signal = 0;

static HV *
load_defaults( PImgCodec c)
{
   HV * profile = newHV();
/*
 It appears that PHOTOMETRIC_MINISWHITE should always be inverted (which
 makes sense), but if you find a class of TIFFs / a version of libtiff for
 which that is *not* the case, try setting InvertMinIsWhite / INVERT_MINISWHITE to 0.
*/
#define INVERT_MINISWHITE 1
   pset_i( InvertMinIsWhite, INVERT_MINISWHITE);
/* Converts 1-bit grayscale with ratio 2:1 into 2-bit grayscale */
   pset_i( Fax, 0);
   return profile;
}

static tsize_t
my_tiff_read( thandle_t h, tdata_t data, tsize_t size)
{
    return req_read( (PImgIORequest) h, size, data);
}

static tsize_t
my_tiff_write( thandle_t h, tdata_t data, tsize_t size)
{
    return req_write( (PImgIORequest) h, size, data);
}

static toff_t
my_tiff_seek( thandle_t h, toff_t offset, int whence)
{
    if ( req_seek( (PImgIORequest) h, offset, whence) < 0)
       return -1;
    return req_tell( (PImgIORequest) h);
}

static int
my_tiff_close( thandle_t h)
{
    return (( PImgIORequest) h)-> flush ? 
       req_flush( (PImgIORequest) h) :
       0;
}

static toff_t
my_tiff_size( thandle_t h)
{
    return 0;
}

static int
my_tiff_map( thandle_t h, tdata_t * data, toff_t * offset)
{
    return 0;
}

static void
my_tiff_unmap( thandle_t h, tdata_t data, toff_t offset)
{
}


static void * 
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   TIFF * tiff;
   errbuf = fi-> errbuf;
   err_signal = 0;
   if (!( tiff = TIFFClientOpen( "", "r", (thandle_t) fi-> req,
      my_tiff_read, my_tiff_write,
      my_tiff_seek, my_tiff_close, my_tiff_size, 
      my_tiff_map, my_tiff_unmap))) {
      req_seek( fi-> req, 0, SEEK_SET);
      return nil;
   }
   fi-> frameCount = TIFFNumberOfDirectories( tiff);
   fi-> stop = true;
   return tiff;
}


static void
scan_convert( Byte * src, Byte * dest, int width, int bps)
{
   switch ( bps) {
   case 1:
      bc_mono_byte( src, dest, width);
      break;
   case 2: {
         register Byte mask = 0x03, shift = 0;
         while ( width--) {
           *dest++ = (*src & mask) >> shift;
           if ( shift == 6) { 
              mask = 0x03;
              shift = 0;
              src++;
           } else {
              mask <<= 2;
              shift += 2;
           }
        }
      } break;
   case 4:
      bc_nibble_byte( src, dest, width);
      break;
   case 8:
      memcpy( dest, src, width);
      break;
   case 16:
      memcpy( dest, src, width * 2);
      break;
   }
}

static void
bc_short_byte( unsigned short *src, Byte *dst, int width)
{
   Byte **psrc = ( Byte **) &src;
   while ( width--) {
      *dst++ = *src >> 8;
      *psrc += 2;
   }
}         

static Bool   
load( PImgCodec instance, PImgLoadFileInstance fi)
{
   TIFF * tiff = ( TIFF *) fi-> instance;
   HV * profile = fi-> frameProperties;
   PIcon i = ( PIcon) fi-> object;
   uint16 resunit;
   char * photometric_descr = nil;
   unsigned short photometric, bps, spp, planar, comp_method;
   int x, y, w, h, bpp = 0, palSize = 0, icon, tiled, rgba_striped = 0,
      InvertMinIsWhite = INVERT_MINISWHITE, strip_bps, faxpect = 0;
   float xres, yres;
   unsigned short *redcolormap, *greencolormap, *bluecolormap;
   Byte *tiffstrip, *tiffline, *tifftile, *primaline, *primamask = nil;
   size_t stripsz, linesz, tilesz = 0L;
   uint32 tile_width, tile_height, num_tilesX = 0L, rowsperstrip;
   Byte bw_colorref[256];
   
   errbuf = fi-> errbuf;
   err_signal = 0;
   
   if ( !TIFFSetDirectory( tiff, (tdir_t) fi-> frame)) {
      outc( "Frame index out of range");
      return false;
   }

   if ( !TIFFGetField( tiff, TIFFTAG_PHOTOMETRIC, &photometric)) {
      outc("Cannot query PHOTOMETRIC tag");
      return false;
   }
   if ( !TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w)) {
      outc("Cannot query IMAGEWIDTH tag");
      return false;
   }
   if ( !TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h)) {
      outc("Cannot query IMAGELENGTH tag");
      return false;
   }

   if ( !TIFFGetField( tiff, TIFFTAG_BITSPERSAMPLE, &bps))  bps = 1;
      else if ( fi-> loadExtras) pset_i( BitsPerSample, bps);
   if ( bps != 16 && bps != 8 && bps != 4 && bps != 2 && bps != 1) {
      sprintf( fi-> errbuf, "Unexpected BITSPERSAMPLE: %d", bps);
      return false;
   }
   if ( !TIFFGetField( tiff, TIFFTAG_SAMPLESPERPIXEL, &spp))  spp = 1;
      else if ( fi-> loadExtras) pset_i( SamplesPerPixel, spp);
   if ( spp < 1 || spp > 4) {
      sprintf( fi-> errbuf, "Unexpected SAMPLESPERPIXEL: %d", spp);
      return false;
   }
   if ( !TIFFGetField( tiff, TIFFTAG_PLANARCONFIG, &planar))  planar = 1; 
     else if ( fi-> loadExtras) pset_i( PlanarConfig, planar);
   if ( planar != PLANARCONFIG_CONTIG && planar != PLANARCONFIG_SEPARATE) {
      sprintf( fi-> errbuf, "Unexpected PLANARCONFIG: %d", planar);
      return false;
   }
   if ( !TIFFGetField( tiff, TIFFTAG_XRESOLUTION, &xres))     xres = 0.0;
     else if ( fi-> loadExtras) pset_i( XResolution, xres);
   if ( !TIFFGetField( tiff, TIFFTAG_YRESOLUTION, &yres))     yres = 0.0;
     else if ( fi-> loadExtras) pset_i( YResolution, yres);
   if ( !TIFFGetField( tiff, TIFFTAG_RESOLUTIONUNIT, &resunit))
        resunit = RESUNIT_INCH;  /* default (see libtiff tif_dir.c) */
     else if ( fi-> loadExtras) 
       pset_c( ResolutionUnit, 
         (resunit==RESUNIT_INCH) ? "inch" : (resunit==RESUNIT_CENTIMETER?"centimeter":"none"));
   tiled = TIFFIsTiled(tiff);
   if ( fi-> loadExtras) pset_i( Tiled, tiled);

   /* calculate prima image bpp and color count */
   palSize = 1 << bps;
   switch ( photometric) {
   case PHOTOMETRIC_MINISWHITE:
   case PHOTOMETRIC_MINISBLACK:
      if ( bps > 8) bpp = imShort; else
      if ( bps > 4) bpp = imByte; else
      if ( bps > 2) bpp = imbpp4 | imGrayScale; else
      if ( bps > 1) bpp = imbpp4; else
                    bpp = imbpp1 | imGrayScale;
      photometric_descr = ( photometric == PHOTOMETRIC_MINISBLACK) ? "MinIsWhite" : "MinIsBlack";
      break;
   case PHOTOMETRIC_PALETTE:
      if ( !TIFFGetField( tiff, TIFFTAG_COLORMAP, &redcolormap, &greencolormap, &bluecolormap)) {
         outc("Cannot query COLORMAP tag");
         return false;
      }
      if ( bps > 4) bpp = imbpp8; else 
      if ( bps > 1) bpp = imbpp4; else
                    bpp = imbpp1;
      photometric_descr = "Palette";
      break;
#ifdef JPEG_SUPPORT
   case PHOTOMETRIC_YCBCR:
      if ( !TIFFGetField( tiff, TIFFTAG_COMPRESSION, &comp_method)) {
         outc("Cannot query COMPRESSION");
         return false;
      }
      if ( comp_method != COMPRESSION_JPEG || planar != PLANARCONFIG_CONTIG) {
         sprintf( fi-> errbuf, "Don't know how to handle photometric YCbCr with" \
            "compression %d (%sJPEG) and planar config %d (%scontiguous)",
            comp_method, comp_method == COMPRESSION_JPEG? "" : "not ",
            planar, planar == PLANARCONFIG_CONTIG ? "" : "not "
         );
         return false;
      }
      /* can rely on libjpeg to convert to RGB */
      TIFFSetField( tiff, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
      photometric_descr = "YCbCr";
      photometric = PHOTOMETRIC_RGB;
      spp = 3;
      /* fall thru... */
#endif
   case PHOTOMETRIC_RGB:
      if ( !photometric_descr) photometric_descr = "RGB";
      bpp = imbpp24;
      break;
   case PHOTOMETRIC_LOGL:
   case PHOTOMETRIC_LOGLUV:
      if ( !TIFFGetField( tiff, TIFFTAG_COMPRESSION, &comp_method)) {
         outc("Cannot query COMPRESSION tag");
         return false;
      }
      if (comp_method != COMPRESSION_SGILOG && comp_method != COMPRESSION_SGILOG24) {
         sprintf( fi-> errbuf, "Don't know how to handle photometric LOGL%s with" \
           " compression %d (not SGILOG)",
           photometric == PHOTOMETRIC_LOGLUV ? "UV" : "",
           comp_method);
         return false;
      }
      /* rely on library to convert to RGB/greyscale */
      if (bps > 8 && photometric == PHOTOMETRIC_LOGL) {
         /* SGILOGDATAFMT_16BIT converts to 16-bit short */
         TIFFSetField(tiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_16BIT);
         bps = 16;
      } else {
         /* SGILOGDATAFMT_8BIT converts to normal grayscale or RGB format.
            v3.5.7 handles 16-bit LOGLUV incorrectly, so do 8bit also here */
         TIFFSetField(tiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
         bps = 8;
      }

      if (photometric == PHOTOMETRIC_LOGL) {
         photometric = PHOTOMETRIC_MINISBLACK;
         photometric_descr = "LogL";
         bpp = ( bps > 8) ? imShort : imByte;
      } else {
         photometric = PHOTOMETRIC_RGB;
         bpp = imbpp24;
         photometric_descr = "LogLUV";
      }
      break;
#ifdef JPEG_SUPPORT
   case PHOTOMETRIC_SEPARATED:
      bpp = imbpp24;
      spp = 4;
      rgba_striped = 1;
      photometric_descr = "Separated";
      photometric = PHOTOMETRIC_RGB;
      break;
#endif
   default:
      /* fallback, to RGBA strips */
      bpp = imbpp24;
      spp = 4;
      rgba_striped = 1;
      photometric_descr = 
        photometric == PHOTOMETRIC_MASK?      "MASK" :
        photometric == PHOTOMETRIC_CIELAB?    "CIELAB" :
        photometric == PHOTOMETRIC_DEPTH?     "DEPTH" :
        photometric == PHOTOMETRIC_SEPARATED? "SEPARATED" :
        photometric == PHOTOMETRIC_YCBCR?     "YCBCR" :
                                              "unknown";
      photometric = PHOTOMETRIC_RGB;
      break;
   }

   /* check bps and spp combinations - 3 and 4 samples for RGB, 1 and 2 for the others */
   if 
      (
          (( bpp == imbpp24) && ( spp != 3 && spp != 4)) 
          ||
          (( bpp != imbpp24) && ( spp != 1 && spp != 2)) 
       ) {
      sprintf( fi-> errbuf, "Cannot handle combination SAMPLESPERPIXEL=%d, BITSPERSAMPLE=%d", spp, bps);
      return false;
   }

   /* set misc tags */
   if ( fi-> loadExtras) {
      uint16 u16, u16_2;
      char * ch;
      float n;
      pset_c( Photometric, photometric_descr);
      if ( TIFFGetField( tiff, TIFFTAG_ARTIST, &ch)) 
         pset_c( Artist, ch);
      if ( TIFFGetField( tiff, TIFFTAG_COMPRESSION, &u16)) {
         int i, found = 0;
         for ( i = 0; i < sizeof(comptable) / sizeof(CompType); i++) {
            if ( comptable[i].tag == u16) {
               pset_c( CompressionType, comptable[i].name);
               found = 1;
               break;
            }
         }
         if ( !found) pset_i( CompressionType, u16);
      }
      if ( TIFFGetField( tiff, TIFFTAG_COPYRIGHT, &ch)) 
         pset_c( Copyright, ch);
      if ( TIFFGetField( tiff, TIFFTAG_DATETIME, &ch)) 
         pset_c( DateTime, ch);
      if ( TIFFGetField( tiff, TIFFTAG_DOCUMENTNAME, &ch)) 
         pset_c( DocumentName, ch);
      if ( TIFFGetField( tiff, TIFFTAG_HOSTCOMPUTER, &ch)) 
         pset_c( HostComputer, ch);
      if ( TIFFGetField( tiff, TIFFTAG_IMAGEDESCRIPTION, &ch)) 
         pset_c( ImageDescription, ch);
      if ( TIFFGetField( tiff, TIFFTAG_MAKE, &ch)) 
         pset_c( Make, ch);
      if ( TIFFGetField( tiff, TIFFTAG_MODEL, &ch)) 
         pset_c( Model, ch);
      if ( TIFFGetField( tiff, TIFFTAG_PAGENAME, &ch)) 
         pset_c( PageName, ch);
      if ( TIFFGetField( tiff, TIFFTAG_SOFTWARE, &ch)) 
         pset_c( Software, ch);
      if ( TIFFGetField( tiff, TIFFTAG_XPOSITION, &n)) 
         pset_f( XPosition, n);
      if ( TIFFGetField( tiff, TIFFTAG_YPOSITION, &n)) 
         pset_f( YPosition, n);
      if ( TIFFGetField( tiff, TIFFTAG_PAGENUMBER, &u16, &u16_2)) {
         pset_i( PageNumber, u16);
         pset_i( PageNumber2, u16_2);
      }
   }
   
   /* check load options */   
   {
      dPROFILE;
      HV * profile = fi-> profile;
      if ( pexist( InvertMinIsWhite)) InvertMinIsWhite = pget_i( InvertMinIsWhite);
  /* check fax option applicability */
      if ( bps == 1 && 
         ( photometric == PHOTOMETRIC_MINISWHITE || photometric == PHOTOMETRIC_MINISBLACK) &&
         xres > 0 && yres > 0 && 
         xres / yres > 1.95 && xres / yres < 2.05 &&
         pexist( Fax)) {
         faxpect = pget_i( Fax);
         if ( faxpect) {
            xres /= 2;
            bpp   = imbpp4;
            w    /= 2;
            bps   = 2;
        }
     }
   }
  
   /* done prerequisite tiff parsing, leave early if we can */
   if ( fi-> noImageData) {
      CImage( fi-> object)-> create_empty( fi-> object, 1, 1, bpp);
      pset_i( width,  w);
      pset_i( height, h);
   } else 
      CImage( fi-> object)-> create_empty( fi-> object, w, h, bpp);
   EVENT_HEADER_READY(fi);

   /* check if palette available */
   if ( photometric == PHOTOMETRIC_PALETTE) { 
      RGBColor *p = i-> palette, last;
      if ( palSize > 256) palSize = 256;
      i-> palSize = palSize;
      
      for ( x = 0; x < palSize; x++, p++) {
         p-> r = redcolormap[x]   >> 8;
         p-> g = greencolormap[x] >> 8;
         p-> b = bluecolormap[x]  >> 8;
      }

      /* optimize palette, since tiff palette must be **2 only */
      p = i-> palette + palSize - 1;
      last = *p--;
      for ( x = palSize - 1; x > 0; x--, p--) {
         if ( p->r == last.r && p->g == last.g && p->b == last.b) {
            i-> palSize--;
         } else {
            /* see if this color is also present somewhere */
            p = i-> palette;
            for ( x = 0; x < i-> palSize - 1; x++) {
               if ( p->r == last.r && p->g == last.g && p->b == last.b) {
                  i-> palSize--;
                  break;
               }
            }
            break;
         }
      } 
   } else if ( bps == 2) { /* build non-default grayscale palette */
      PRGBColor p = i-> palette;
      i-> palSize = 4;
      p[0].r = p[0].g = p[0].b = 0;
      p[3].r = p[3].g = p[3].b = 255;
      if ( faxpect) {
         p[1].r = p[1].g = p[1].b = 
         p[2].r = p[2].g = p[2].b = 127;
      } else {
         i-> palSize = 4;
         p[1].r = p[1].g = p[1].b = 85;
         p[2].r = p[2].g = p[2].b = 170;
      }
   }

   /* leave early */
   if ( fi-> noImageData) return true;

   icon = kind_of( fi-> object, CIcon);
   if ( icon) i-> autoMasking = amNone;

   /* allocate space for one line (or row of tiles) of TIFF image */
   tiffline = tifftile = tiffstrip = nil;
   linesz = TIFFScanlineSize(tiff);

   if (tiled) {
      int z;
      if ( !TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tile_width)) {
         outc("Cannot query TILEWIDTH tag");
         return false;
      }
      if ( tile_width < 1) {
         sprintf( fi-> errbuf, "Invalid TILEWIDTH=%ld", (long)tile_width);
         return false;
      }
      if ( !TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tile_height)) {
         outc("Cannot query TILELENGTH tag");
         return false;
      }
      if ( tile_height < 1) {
         sprintf( fi-> errbuf, "Invalid TILELENGTH=%ld", (long)tile_height);
         return false;
      }
      num_tilesX = (w + tile_width - 1) / tile_width;
      tilesz = TIFFTileSize(tiff);
      /* check if linesz is big enough */
      z = tilesz / tile_height * num_tilesX;
      if ( linesz < z) linesz = z;

      rowsperstrip = 1;
   } else {
      tile_width  = w;
      tile_height = 1;
      num_tilesX  = 1;
      tilesz      = linesz;
      if ( rgba_striped) {
         if( !TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &rowsperstrip) ) {
             outc("Cannot query ROWSPERSTRIP tag");
             return false;
         }
      } else
	 rowsperstrip = 1;
   }


   /* setup buffers for twofold size for byte and intrapixel conversion */
   strip_bps = ( bps > 8) ? 2 : 1;
   if ( !( tifftile = (Byte*) malloc( strip_bps * w * rowsperstrip * tile_height * spp * 2))) {
      outcm( strip_bps * w * spp * 2);
      return false;
   }
   stripsz = strip_bps * rowsperstrip * tile_height * w * spp;
   if ( !( tiffstrip = (Byte*) malloc( stripsz * 2))) {
      free( tifftile);
      outcm( stripsz * 2);
      return false;
   }
   tiffline = tiffstrip; /* just set the line to the top of the strip.
                          * we'll move it through below. */

   /* printf("w:%d, bps:%d, spp:%d, planar:%d, tile_height:%d, strip_sz:%d, bpp:%d\n", w, bps, spp, planar, tile_height, stripsz, bpp); */
   /* setting up destination pointers */
   primaline = i-> data + ( h - 1) * i-> lineSize;
   if ( icon) {
      primamask = i-> mask + ( h - 1) * i-> maskLine;
      /* create colorref for alpha downsampling */
      for ( x = 0;   x < 128; x++) bw_colorref[x] = 1;
      for ( x = 128; x < 256; x++) bw_colorref[x] = 0;
   }

   for ( y = 0; y < h; y++, primaline -= i-> lineSize) {
      /* read from file - tiled and not tiled */
      if ( tiled) {
         int col, ok = 1;
         /* Is it time for a new strip? */
         if (( y % tile_height) == 0) {
            for (col = 0; ok && col < num_tilesX; col++) {
               Byte *dest, *src;
               int r, dd, sd, rows, cols;
               int tileno = col+(y/tile_height)*num_tilesX;
               /* read the tile into the array */
	       int ret = rgba_striped ?
		  TIFFReadRGBATile( tiff, col * tile_width, y, (void*) tifftile) :
                  TIFFReadEncodedTile(tiff, tileno, tifftile, tilesz);
               if (!ret) {
                  ok = 0;
                  break;
               }

               /* copy this tile into the row buffer */
               dest = tiffstrip + stripsz + col * strip_bps * spp * tile_width;
	       rows = ((y + tile_height) > h) ? h - y : tile_height;
	       cols = (col == num_tilesX - 1) ? w - col * tile_width : tile_width;
               dd   = w * spp;
	       if ( rgba_striped) {
	          /* RGBATiles are reversed */
		  sd   = - (tile_width * spp);
                  src  = tifftile - sd * (tile_height - 1); 
	       } else {
                  sd   = tilesz / tile_height;
                  src  = tifftile;
	       }
               for (r = 0; r < rows; r++, src += sd, dest += dd)
                  scan_convert( src, dest, cols * spp, bps);
            }
            tiffline = tiffstrip; /* set tileline to top of strip */
         } else 
            tiffline = tiffstrip + (y % tile_height) * w * spp;
      } else if ( rgba_striped) {
         /* Is it time for a new strip? */
         if (( y % rowsperstrip) == 0) {
            Byte *dest, *src;
            int r, rows, dd, sd;
            if ( TIFFReadRGBAStrip( tiff, y, (void*) tifftile) < 0) {
               if ( !( errbuf && errbuf[0]))
                 sprintf( fi-> errbuf, "Error reading scanline %d", y);
               free(tifftile);
               free(tiffstrip);
               return false;
	    }
	    rows = ((y + rowsperstrip) > h) ? h - y : rowsperstrip;
            dest = tiffstrip + stripsz;
            dd   = sd = spp * w;
            src  = tifftile + sd * (rows - 1);
	    /* RGBAStrips are reversed */
            for (r = 0; r < rows; r++, src -= sd, dest += dd) 
                scan_convert( src, dest, sd, bps);
            tiffline = tiffstrip; /* set tileline to top of strip */
	 } else
            tiffline = tiffstrip + (y % rowsperstrip) * spp * w;
      } else {
         int s = 0, reads = ( planar == PLANARCONFIG_CONTIG) ? 1 : spp;
         int dw = w * (( planar == PLANARCONFIG_CONTIG) ? spp : 1);
         Byte * d = tiffline + stripsz;
         for ( s = 0; s < reads; s++, d += w * strip_bps) {
            if ( TIFFReadScanline( tiff, tiffline, y, (tsample_t) s) < 0) {
               if ( !( errbuf && errbuf[0]))
                 sprintf( fi-> errbuf, "Error reading scanline %d", y);
               free(tifftile);
               free(tiffstrip);
               return false;
            }
            scan_convert( tiffline, d, dw, bps);
         }
      }

      /* convert intrapixel layout into planar layout to extract alpha in separate space  */
      {
         Byte * dst0 = tiffline, *dst1;
         Byte * src0 = tiffline + stripsz, *src1, *src2;
         x = w;
         switch ( strip_bps * 100 + planar * 10 + spp) {
         case 112:
            dst1 = dst0 + w;
            while ( x--) {
               *dst0++ = *src0++;
               *dst1++ = *src0++;
            }
            break;
         case 114:
            dst1 = dst0 + 3 * w;
            while ( x--) {
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst1++ = *src0++;
            }
            break;
         case 124:
            memcpy( dst0 + w * 3, src0 + w * 3, w);
         case 123:
            src1 = src0 + w;
            src2 = src1 + w;
            while ( x--) {
               *dst0++ = *src0++;
               *dst0++ = *src1++;
               *dst0++ = *src2++;
            }
            break;
         case 212:
            dst1 = dst0 + w * 2;
            while ( x--) {
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst1++ = *src0++;
               *dst1++ = *src0++;
            }
            break;
         case 214:
            dst1 = dst0 + 6 * w;
            while ( x--) {
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst1++ = *src0++;
               *dst1++ = *src0++;
            }
            break;
         case 224:
            memcpy( dst0 + w * 6, src0 + w * 6, w * 2);
         case 223:
            src1 = src0 + w * 2;
            src2 = src1 + w * 2;
            while ( x--) {
               *dst0++ = *src0++;
               *dst0++ = *src0++;
               *dst0++ = *src1++;
               *dst0++ = *src1++;
               *dst0++ = *src2++;
               *dst0++ = *src2++;
            }
            break;
         default:
            memcpy( dst0, src0, w * spp * strip_bps);
         }
      }

      /* invert data, if any */
      if ( InvertMinIsWhite && photometric == PHOTOMETRIC_MINISWHITE) {
         Byte *s = tiffline;
         int  sz = w * strip_bps * spp;
         register Byte mask = ( bps > 4) ? 0xff : (1 << bps) - 1;
         while ( sz--) {
            *s = (~*s) & mask;
            s++;
         }
      }

      /* copy data into image */
      switch ( bpp) {
      case imbpp1: case imbpp1 | imGrayScale:
         bc_byte_mono_cr( tiffline, primaline, w, map_stdcolorref);
         break;
      case imbpp4: case imbpp4 | imGrayScale:
         bc_byte_nibble_cr( tiffline, primaline, w, map_stdcolorref);
         break;
      case imbpp8: case imbpp8 | imGrayScale:
         memcpy( primaline, tiffline, w);
         break;
      case imRGB: 
         if ( bps == 16) bc_short_byte(( unsigned short*) tiffline, tiffline, w * 3);
         cm_reverse_palette(( RGBColor*) tiffline, ( RGBColor*) primaline, w);
         break;
      case imShort:
         memcpy( primaline, tiffline, w * 2);
         break;
      }

      /* do alpha channel */
      if ( icon && ( spp == 2 || spp == 4)) {
         Byte * alpha = tiffline + w * ( spp - 1 ) * strip_bps;
         if ( bps == 16) bc_short_byte(( unsigned short*) alpha, alpha, w);
         bc_byte_mono_cr( alpha, primamask, w, bw_colorref);
         primamask -= i-> maskLine;
      }
      EVENT_TOPDOWN_SCANLINES_READY(fi,1);
   }
   EVENT_SCANLINES_FINISHED(fi);
   
   /* finalize */
   free( tiffstrip);
   free( tifftile);

   return true;
}   

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
   errbuf = fi-> errbuf;
   err_signal = 0;
   TIFFClose(( TIFF*) fi-> instance);
   errbuf = nil;
}

static HV *
save_defaults( PImgCodec c)
{
   HV * profile = newHV();
   pset_c( Software, "Prima");
   pset_c( Artist, "");
   pset_c( Copyright, "");
   pset_c( Compression, "NONE");
   pset_c( DateTime, "");
   pset_c( DocumentName, "");
   pset_c( HostComputer, "");
   pset_c( ImageDescription, "");
   pset_c( Make, "");
   pset_c( Model, "");
   pset_c( PageName, "");
   pset_i( PageNumber, 1);
   pset_i( PageNumber2, 1);
   pset_c( ResolutionUnit, "none");
   pset_i( XPosition, 0);
   pset_i( YPosition, 0);
   pset_i( XResolution, 1200);
   pset_i( YResolution, 1200);
   
   return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   TIFF * tiff;
   errbuf = fi-> errbuf;
   err_signal = 0;
   if (!( tiff = TIFFClientOpen( "", "w", (thandle_t) fi-> req,
      my_tiff_read, my_tiff_write,
      my_tiff_seek, my_tiff_close, my_tiff_size, 
      my_tiff_map, my_tiff_unmap)))
      return nil;
   return tiff;
}

static Bool   
save( PImgCodec instance, PImgSaveFileInstance fi)
{
   dPROFILE;
   PIcon i = ( PIcon) fi-> object;
   TIFF * tiff = ( TIFF*) fi-> instance;
   Bool icon = kind_of( fi-> object, CIcon);
   int x, y;
   HV * profile = fi-> objectExtras;
   uint16 u16;
   
   errbuf = fi-> errbuf;
   err_signal = 0;

   TIFFSetField( tiff, TIFFTAG_IMAGEWIDTH, i-> w);
   TIFFSetField( tiff, TIFFTAG_IMAGELENGTH, i-> h);

   u16 = COMPRESSION_NONE;
   if ( pexist( Compression)) {
      int found = 0;
      char * c = pget_c( Compression);
      for ( x = 0; x < sizeof( comptable) / sizeof( CompType); x++) {
         if ( strcmp( comptable[x]. name, c) == 0) {
            u16 = comptable[x]. tag;
            found = 1;
         }
      }
      if ( !found) {
         snprintf( fi-> errbuf, 256, "Invalid Compression '%s'", c);
         return false;
      }
   }
   TIFFSetField(tiff, TIFFTAG_COMPRESSION, u16);
   if (u16 == COMPRESSION_CCITTFAX3)
      TIFFSetField(tiff, TIFFTAG_GROUP3OPTIONS, GROUP3OPT_2DENCODING + GROUP3OPT_FILLBITS);

   u16 = RESUNIT_NONE;
   if ( pexist( ResolutionUnit)) {
      char * c = pget_c( ResolutionUnit);
      if ( stricmp( c, "inch") == 0) u16 = RESUNIT_INCH; else
      if ( stricmp( c, "centimeter") == 0) u16 = RESUNIT_CENTIMETER; else
      if ( stricmp( c, "none") == 0) u16 = RESUNIT_NONE; else {
         snprintf( fi-> errbuf, 256, "Invalid Compression '%s'", c);
         return false;
      }
   }
   TIFFSetField( tiff, TIFFTAG_RESOLUTIONUNIT, u16);

   if ( pexist( Artist)) 
      TIFFSetField( tiff, TIFFTAG_ARTIST, pget_c( Artist));
   if ( pexist( Copyright)) 
      TIFFSetField( tiff, TIFFTAG_COPYRIGHT, pget_c( Copyright));
   if ( pexist( DateTime)) 
      TIFFSetField( tiff, TIFFTAG_DATETIME, pget_c( DateTime));
   if ( pexist( DocumentName)) 
      TIFFSetField( tiff, TIFFTAG_DOCUMENTNAME, pget_c( DocumentName));
   if ( pexist( HostComputer)) 
      TIFFSetField( tiff, TIFFTAG_HOSTCOMPUTER, pget_c( HostComputer));
   if ( pexist( ImageDescription)) 
      TIFFSetField( tiff, TIFFTAG_IMAGEDESCRIPTION, pget_c( ImageDescription));
   if ( pexist( Make)) 
      TIFFSetField( tiff, TIFFTAG_MAKE, pget_c( Make));
   if ( pexist( Model)) 
      TIFFSetField( tiff, TIFFTAG_MODEL, pget_c( Model));
   if ( pexist( PageName)) 
      TIFFSetField( tiff, TIFFTAG_PAGENAME, pget_c( PageName));
   if ( pexist( Software)) 
      TIFFSetField( tiff, TIFFTAG_SOFTWARE, pget_c( Software));
   else
      TIFFSetField( tiff, TIFFTAG_SOFTWARE, "Prima");
   if ( pexist( XPosition)) 
      TIFFSetField( tiff, TIFFTAG_XPOSITION, pget_f( XPosition));
   if ( pexist( YPosition)) 
      TIFFSetField( tiff, TIFFTAG_YPOSITION, pget_f( YPosition));
   if ( pexist( XResolution)) 
      TIFFSetField( tiff, TIFFTAG_XRESOLUTION, pget_f( XResolution));
   if ( pexist( YResolution)) 
      TIFFSetField( tiff, TIFFTAG_YRESOLUTION, pget_f( YResolution));
   {
      Bool r1 = pexist( PageNumber), r2 = pexist( PageNumber2);
      uint16 u2;
      if (( r1 && !r2) || ( !r1 && r2)) {
         outc( "Fields PageNumber and PageNumber2 must be present simultaneously");
         return false;
      }
      if ( r1 && r2) {
         u16 = pget_i( PageNumber);
         u2 = pget_i( PageNumber2);
         TIFFSetField( tiff, TIFFTAG_PAGENUMBER, u16, u2);
      }
   }
   
   /* write data */
   TIFFSetField( tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
   TIFFSetField( tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
   TIFFSetField( tiff, TIFFTAG_ROWSPERSTRIP, 1);
   if ( !icon && i-> type != imRGB) {
      PRGBColor p = i-> palette;
      Byte * r = i-> data + ( i-> h - 1 ) * i-> lineSize;
      uint16 photometric = PHOTOMETRIC_MINISBLACK;
      switch ( i-> type) {
      case imbpp1: 
         if ( p[0].r == 0 && p[0].g == 0 && p[0].b == 0 &&
              p[1].r == 255 && p[1].g == 255 && p[1].b == 255) 
             photometric = PHOTOMETRIC_MINISBLACK;
         else if 
            ( p[1].r == 0 && p[1].g == 0 && p[1].b == 0 &&
              p[0].r == 255 && p[0].g == 255 && p[0].b == 255) 
             photometric = PHOTOMETRIC_MINISWHITE;
         else 
             photometric = PHOTOMETRIC_PALETTE;
         break;
      case imbpp4:
      case imbpp8:
         photometric = PHOTOMETRIC_PALETTE;
         break;
      default:
         photometric = PHOTOMETRIC_MINISBLACK;
         break;
      }
      TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric);
      TIFFSetField( tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
      TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, i-> type & imBPP);
         
      if ( photometric == PHOTOMETRIC_PALETTE) {
         int x, lim = (i-> palSize > 256) ? 256 : i-> palSize;
         uint16 red[256], green[256], blue[256];
         for ( x = 0; x < lim; x++, p++) {
            red  [x] = p-> r << 8;
            green[x] = p-> g << 8;
            blue [x] = p-> b << 8;
         }
         TIFFSetField( tiff, TIFFTAG_COLORMAP, red, green, blue);
      }
      for ( y = 0; y < i-> h; y++) {
         if ( !TIFFWriteScanline( tiff, r, y, 0) || err_signal)
            return false;
         r -= i-> lineSize;
      }
   } else if ( !icon && i-> type == imRGB) {
      Byte * conv;
      Byte * r = i-> data + ( i-> h - 1 ) * i-> lineSize;
      if ( !( conv = malloc( i-> lineSize))) {
         outcm( i-> lineSize);
         return false;
      }
      TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      TIFFSetField( tiff, TIFFTAG_SAMPLESPERPIXEL, 3);
      TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, 8);
      for ( y = 0; y < i-> h; y++) {
         cm_reverse_palette(( RGBColor*) r, ( RGBColor*) conv, i-> w);
         if ( !TIFFWriteScanline( tiff, conv, y, 0) || err_signal) {
            free( conv);
            return false;
         }
         r -= i-> lineSize;
      }
      free( conv);
   } else { /* icon */
      Byte * conv1, * conv2;
      Byte * r, * mask = i-> mask + ( i-> h - 1 ) * i-> maskLine;
      Handle dup = CImage( fi-> object)-> dup( fi-> object);
      int lineSize;
      if ( !dup) return false;
      if ( !( conv1 = malloc( i-> lineSize + i-> w * 2))) {
         Object_destroy( dup);
         outcm( i-> lineSize + i-> w * 2);
         return false;
      }
      conv2 = conv1 + i-> lineSize + i-> w;
      CImage( dup)-> reset( dup, imRGB, nil, 0); 
      lineSize = PImage( dup)-> lineSize;
      r = PImage( dup)-> data + ( i-> h - 1 ) * lineSize;
      TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
      TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, 8);
      for ( y = 0; y < i-> h; y++) {
         Byte * sconv1 = conv1 + 3, * sconv2 = conv2;
         bc_mono_byte( mask, conv2, i-> w);
         bc_rgb_bgri( r, conv1, i-> w);
         for ( x = 0; x < i-> w; x++, sconv1 += 4, sconv2++) 
            *sconv1 = *sconv2 ? 0 : 255;
         if ( !TIFFWriteScanline( tiff, conv1, y, 0) || err_signal) {
            free( conv1);
            Object_destroy( dup);
            return false;
         }
         r -= lineSize;
         mask -= i-> maskLine;
      }
      free( conv1);
      Object_destroy( dup);
   }

   TIFFWriteDirectory( tiff);

   return true;
}

static void 
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
   errbuf = fi-> errbuf;
   err_signal = 0;
   TIFFClose(( TIFF*) fi-> instance);
   errbuf = nil;
}

static TIFFErrorHandler old_error_handler, old_warning_handler;

static void
error_handler( const char* module, const char* fmt, va_list ap)
{
   if ( errbuf) vsnprintf( errbuf, 255, fmt, ap);
   err_signal = 1;
}

static void * 
init( PImgCodecInfo * info, void * param)
{
   *info = &codec_info;
   codec_info. vendor  = ( char *) TIFFGetVersion(); 
   old_error_handler   = TIFFSetErrorHandler(( TIFFErrorHandler) error_handler);
   old_warning_handler = TIFFSetWarningHandler(( TIFFErrorHandler) nil);
   return (void*)1;
}   

static void
done( PImgCodec instance)
{
   TIFFSetErrorHandler( old_error_handler);
   TIFFSetErrorHandler( old_warning_handler);
}

void 
apc_img_codec_tiff( void )
{
   struct ImgCodecVMT vmt;
   memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
   vmt. init          = init;
   vmt. load_defaults = load_defaults;
   vmt. done          = done;
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
