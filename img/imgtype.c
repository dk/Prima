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
 *
 * $Id$
 */
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif



#define var (( PImage) self)

/* Color mappers */
#define BCPARMS      self, dstData, dstPal, dstType
#define BCSELFGRAY   self, var->data, dstPal, imByte
#define ic_MIDCONVERT(from,to)                                    \
{                                                                 \
   Byte * sData = var->data;                                       \
   int  sDataSize = var->dataSize, sLineSize = var->lineSize;       \
   Byte * n = allocb((( var->w * 8 + 31) / 32) * 4 * var->h);    \
   ic_##from##_graybyte_ictNone(self, n, dstPal, imByte);         \
   var->data = n;                                                  \
   var->type = imByte;                                             \
   var->lineSize = (( var->w * 8 + 31) / 32) * 4;                   \
   var->dataSize = var->lineSize * var->h;                           \
   ic_Byte_##to( self, dstData, dstPal, dstType);                 \
   var->data = sData;                                              \
   var->lineSize = sLineSize;                                      \
   var->dataSize = sDataSize;                                      \
   free( n);                                                      \
}


#define ic_MIDCONVERT_REV(from,to,ict)                               \
{                                                                 \
   Byte * sData = var->data;                                       \
   int  sDataSize = var->dataSize, sLineSize = var->lineSize;       \
   Byte * n = allocb((( var->w * 8 + 31) / 32) * 4 * var->h);    \
   ic_##from##_Byte(self, n, dstPal, imByte);         \
   var->data = n;                                                  \
   var->type = imByte;                                             \
   var->lineSize = (( var->w * 8 + 31) / 32) * 4;                   \
   var->dataSize = var->lineSize * var->h;                           \
   ic_graybyte_##to##_ict##ict( self, dstData, dstPal, dstType);                 \
   var->data = sData;                                              \
   var->lineSize = sLineSize;                                      \
   var->dataSize = sDataSize;                                      \
   free( n);                                                      \
}

void
ic_type_convert( Handle self,
                 Byte * dstData, PRGBColor dstPal, int dstType)
{
   int srcType = var->type;
   int orgDstType = dstType;

   switch( srcType)
   {
      case imBW:
      case im16  + imGrayScale:
      case imRGB + imGrayScale:
         srcType &=~ imGrayScale;
         break;
   }

   switch( dstType)
   {
      case imBW:
      case im16  + imGrayScale:
      case imRGB + imGrayScale:
         dstType &=~ imGrayScale;
         break;
   }

   if ( srcType == dstType)
   {
      memcpy( dstData, var->data, var->dataSize);
      if ( dstPal != var->palette)
         memcpy( dstPal, var->palette, var->palSize);
      else if ( orgDstType & imGrayScale) switch( dstType)
      {
         case imbpp1: memcpy( dstPal, stdmono_palette,    sizeof( stdmono_palette)); break;
         case imbpp4: memcpy( dstPal, std16gray_palette,  sizeof( std16gray_palette)); break;
         case imbpp8: memcpy( dstPal, std256gray_palette, sizeof( std256gray_palette)); break;
      }
      return;
   }

   switch( srcType)
   {
      case imMono: switch( dstType)
      {
         case im16:      ic_mono_nibble_ictNone(BCPARMS);    break;
         case im256:     ic_mono_byte_ictNone(BCPARMS);      break;
         case imByte:    ic_mono_graybyte_ictNone(BCPARMS);  break;
         case imRGB:     ic_mono_rgb_ictNone(BCPARMS);       break;
         case imShort:   ic_MIDCONVERT(mono, short);         break;
         case imLong:    ic_MIDCONVERT(mono, long);          break;
         case imFloat:   ic_MIDCONVERT(mono, float);         break;
         case imDouble:  ic_MIDCONVERT(mono, double);        break;
         case imComplex: ic_MIDCONVERT(mono, float_complex); break;
         case imDComplex:ic_MIDCONVERT(mono, double_complex);break;
      }
      break; /* imMono */

      case im16: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_nibble_mono_ictNone(BCPARMS);     break;
               case ictHalftone:
                  ic_nibble_mono_ictHalftone(BCPARMS); break;
               case ictErrorDiffusion:
                  ic_nibble_mono_ictErrorDiffusion(BCPARMS); break;   
            }
            break;
         case im256:
            ic_nibble_byte_ictNone(BCPARMS);           break;
         case imByte:
            ic_nibble_graybyte_ictNone(BCPARMS);       break;
         case imRGB:
            ic_nibble_rgb_ictNone(BCPARMS);            break;
         case imShort:  ic_MIDCONVERT(nibble, short);        break;
         case imLong:   ic_MIDCONVERT(nibble, long);         break;
         case imFloat:  ic_MIDCONVERT(nibble, float);        break;
         case imDouble: ic_MIDCONVERT(nibble, double);       break;
         case imComplex: ic_MIDCONVERT(nibble, float_complex); break;
         case imDComplex:ic_MIDCONVERT(nibble, double_complex);break;
      }
      break; /* im16 */

      case im256: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_byte_mono_ictNone(BCPARMS);       break;
                  break;
               case ictHalftone:
                  ic_byte_mono_ictHalftone(BCPARMS);   break;
               case ictErrorDiffusion:
                  ic_byte_mono_ictErrorDiffusion(BCPARMS); break;
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictNone:
                   ic_byte_nibble_ictNone(BCPARMS);     break;
               case ictHalftone:
                   ic_byte_nibble_ictHalftone(BCPARMS); break;
               case ictErrorDiffusion:
                   ic_byte_nibble_ictErrorDiffusion(BCPARMS); break;    
            }
            break;
         case imByte:
            ic_byte_graybyte_ictNone(BCPARMS);          break;
         case imRGB:
            ic_byte_rgb_ictNone(BCPARMS); break;
         case imShort:  ic_MIDCONVERT(byte, short);        break;
         case imLong:   ic_MIDCONVERT(byte, long);         break;
         case imFloat:  ic_MIDCONVERT(byte, float);        break;
         case imDouble: ic_MIDCONVERT(byte, double);       break;
         case imComplex: ic_MIDCONVERT(byte, float_complex); break;
         case imDComplex:ic_MIDCONVERT(byte, double_complex);break;
      }
      break; /* im256 */

      case imByte: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_graybyte_mono_ictNone(BCPARMS); break;
               case ictHalftone:
                  ic_graybyte_mono_ictHalftone(BCPARMS); break;
               case ictErrorDiffusion:
                  ic_graybyte_mono_ictErrorDiffusion(BCPARMS); break;   
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictNone:
                   ic_graybyte_nibble_ictNone(BCPARMS); break;
               case ictHalftone:
                   ic_graybyte_nibble_ictHalftone(BCPARMS); break;
               case ictErrorDiffusion:
                   ic_graybyte_nibble_ictErrorDiffusion(BCPARMS); break;    
            }
            break;
         case im256:
            break;
         case imRGB:
            ic_graybyte_rgb_ictNone(BCPARMS); break;
         case imShort  : ic_Byte_short( BCPARMS);  break;
         case imLong   : ic_Byte_long( BCPARMS);   break;
         case imFloat  : ic_Byte_float( BCPARMS);  break;
         case imDouble : ic_Byte_double( BCPARMS); break;
         case imComplex: ic_Byte_float_complex(BCPARMS); break;
         case imDComplex: ic_Byte_double_complex(BCPARMS); break;
         break;         
      }
      break; /* imByte */

      case imShort:  switch ( dstType)
      {
         case imMono  :
            ic_short_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_mono_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_mono_ictHalftone(BCPARMS); break;
            }
            break;
         case im16  :
            ic_short_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_nibble_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_nibble_ictHalftone(BCPARMS); break;
            }
            break;
         case im256:
            ic_short_Byte(BCPARMS);
            break;
         case imRGB   :
            ic_short_Byte( BCSELFGRAY);
            var->type = imByte;
            ic_graybyte_rgb_ictNone( BCPARMS);
            break;
         case imByte   : ic_short_Byte( BCPARMS);   break;
         case imLong   : ic_short_long( BCPARMS);   break;
         case imFloat  : ic_short_float( BCPARMS);  break;
         case imDouble : ic_short_double( BCPARMS); break;
         case imComplex: ic_short_float_complex(BCPARMS); break;
         case imDComplex: ic_short_double_complex(BCPARMS); break;
      }
      break;
      /* imShort */

      case imLong:  switch ( dstType)
      {
         case imMono  :
            ic_long_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_mono_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_mono_ictHalftone(BCPARMS); break;
            }
            break;
         case im16  :
            ic_long_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_nibble_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_nibble_ictHalftone(BCPARMS); break;
            }
            break;
         case im256:
            ic_long_Byte(BCPARMS);
            break;
         case imRGB   :
            ic_long_Byte( BCSELFGRAY);
            var->type = imByte;
            ic_graybyte_rgb_ictNone( BCPARMS);
            break;
         case imByte   : ic_long_Byte( BCPARMS);   break;
         case imShort  : ic_long_short( BCPARMS);  break;
         case imFloat  : ic_long_float( BCPARMS);  break;
         case imDouble : ic_long_double( BCPARMS); break;
         case imComplex: ic_long_float_complex(BCPARMS); break;
         case imDComplex: ic_long_double_complex(BCPARMS); break;
      }
      break;
      /* imLong */

      case imFloat:  switch ( dstType)
      {
         case imMono  :
            ic_float_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_mono_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_mono_ictHalftone(BCPARMS); break;
            }
            break;
         case im16  :
            ic_float_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_nibble_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_nibble_ictHalftone(BCPARMS); break;
            }
            break;
         case im256:
            ic_float_Byte(BCPARMS);
            break;
         case imRGB   :
            ic_float_Byte( BCSELFGRAY);

            ic_graybyte_rgb_ictNone( BCPARMS);
            break;
         case imByte   : ic_float_Byte( BCPARMS);   break;
         case imShort  : ic_float_short( BCPARMS);  break;
         case imLong   : ic_float_long( BCPARMS);   break;
         case imDouble : ic_float_double( BCPARMS); break;
         case imComplex: ic_float_float_complex(BCPARMS); break;
         case imDComplex: ic_float_double_complex(BCPARMS); break;
      }
      break;
      /* imFloat */


      case imDouble:  switch ( dstType)
      {
         case imMono  :
            ic_double_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_mono_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_mono_ictHalftone(BCPARMS); break;
            }
            break;
         case im16  :
            ic_double_Byte( BCSELFGRAY);
            var->type = imByte;
            switch ( var->conversion)
            {
               case ictNone:     ic_graybyte_nibble_ictNone(BCPARMS);     break;
               case ictHalftone: ic_graybyte_nibble_ictHalftone(BCPARMS); break;
            }
            break;
         case im256:
            ic_double_Byte(BCPARMS);
            break;
         case imRGB   :
            ic_double_Byte( BCSELFGRAY);
            var->type = imByte;
            ic_graybyte_rgb_ictNone( BCPARMS);
            break;
         case imByte   : ic_double_Byte( BCPARMS);   break;
         case imShort  : ic_double_short( BCPARMS);  break;
         case imLong   : ic_double_long( BCPARMS);   break;
         case imFloat  : ic_double_float( BCPARMS);  break;
         case imComplex: ic_double_float_complex(BCPARMS); break;
         case imDComplex: ic_double_double_complex(BCPARMS); break;
      }
      break;
      /* imDouble */

      case imRGB: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_rgb_mono_ictNone(BCPARMS); break;
               case ictHalftone:
                  ic_rgb_mono_ictHalftone(BCPARMS); break;
               case ictErrorDiffusion:
                  ic_rgb_mono_ictErrorDiffusion(BCPARMS); break;
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_rgb_nibble_ictNone(BCPARMS); break;
               case ictHalftone:
                  ic_rgb_nibble_ictHalftone(BCPARMS); break;
               case ictErrorDiffusion:
                  ic_rgb_nibble_ictErrorDiffusion(BCPARMS); break;   
            }
            break;
         case im256:
            switch ( var->conversion)
            {
            case ictNone:
               ic_rgb_byte_ictNone(BCPARMS); break;
            case ictHalftone:
               ic_rgb_byte_ictHalftone(BCPARMS); break;
            case ictErrorDiffusion:
               ic_rgb_byte_ictErrorDiffusion(BCPARMS); break;
            }
            break;
         case imByte:
            ic_rgb_graybyte_ictNone(BCPARMS); break;
            break;
         case imShort:  ic_MIDCONVERT(rgb, short);        break;
         case imLong:   ic_MIDCONVERT(rgb, long);         break;
         case imFloat:  ic_MIDCONVERT(rgb, float);        break;
         case imDouble: ic_MIDCONVERT(rgb, double);       break;
         case imComplex: ic_MIDCONVERT(rgb, float_complex); break;
         case imDComplex:ic_MIDCONVERT(rgb, double_complex);break;
      }
      break; /* imRGB */

      case imComplex: switch( dstType) {
          case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_MIDCONVERT_REV(float_complex,mono,None); break;
               case ictHalftone:
                  ic_MIDCONVERT_REV(float_complex,mono,Halftone); break;
               case ictErrorDiffusion:
                  ic_MIDCONVERT_REV(float_complex,mono,ErrorDiffusion); break;
            }
            break; 
          case im16:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_MIDCONVERT_REV(float_complex,nibble,None); break;
               case ictHalftone:
                  ic_MIDCONVERT_REV(float_complex,nibble,Halftone); break;
               case ictErrorDiffusion:
                  ic_MIDCONVERT_REV(float_complex,nibble,ErrorDiffusion); break;
            }
            break; 
          case imRGB:     ic_MIDCONVERT_REV( float_complex,rgb,None); break;  
          case im256:   
          case imByte:    ic_float_complex_Byte(BCPARMS); break;
          case imShort:   ic_float_complex_short(BCPARMS); break;
          case imLong:    ic_float_complex_long(BCPARMS); break;
          case imDouble:  ic_float_complex_double(BCPARMS); break;
          case imFloat:   ic_float_complex_float( BCPARMS); break;
      }                   
      case imDComplex: switch( dstType) {
          case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_MIDCONVERT_REV(double_complex,mono,None); break;
               case ictHalftone:
                  ic_MIDCONVERT_REV(double_complex,mono,Halftone); break;
               case ictErrorDiffusion:
                  ic_MIDCONVERT_REV(double_complex,mono,ErrorDiffusion); break;
            }
            break; 
          case im16:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_MIDCONVERT_REV(double_complex,nibble,None); break;
               case ictHalftone:
                  ic_MIDCONVERT_REV(double_complex,nibble,Halftone); break;
               case ictErrorDiffusion:
                  ic_MIDCONVERT_REV(double_complex,nibble,ErrorDiffusion); break;
            }
            break;
          case imRGB:     ic_MIDCONVERT_REV(double_complex,rgb,None); break;  
          case im256:
          case imByte:    ic_double_complex_Byte(BCPARMS); break;
          case imShort:   ic_double_complex_short(BCPARMS); break;
          case imLong:    ic_double_complex_long(BCPARMS); break;
          case imDouble:  ic_double_complex_double(BCPARMS); break;
          case imFloat:   ic_double_complex_float( BCPARMS); break;
      }                   
   }
}

static int imTypes[] = {
   imbpp1, imbpp1|imGrayScale, 
   imbpp4, imbpp4|imGrayScale,
   imbpp8, imbpp8|imGrayScale,
   imRGB, 
   imShort, imLong, imFloat, imDouble,
   imComplex, imDComplex, imTrigComplex, imTrigDComplex,
   -1
};

Bool
itype_supported( int type)
{
    int i = 0;
    while( imTypes[i] != type && imTypes[i] != -1) i++;
    return imTypes[i] != -1;
}   

typedef struct _ImageSignatures
{
   int type;
   int size;
   char *sig;
} ImageSignatures;

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
   { itJPG, 4, "\xff\xd8\xff\xe0" },
   { itJPG, 4, "\xe0\xff\xd8\xff" },
   { itPNG, 8, "\x89PNG\r\n\x1a\n"}
};

#define N_SIGS ( sizeof( signatures) / sizeof( signatures[ 0]))

int
image_guess_type( int fd)
{
   char buf[ 8];
   int i;
   off_t savePos = lseek( fd, 0, SEEK_SET);
   memset( buf, 0, 8);
   read( fd, buf, 8);
   lseek( fd, savePos, SEEK_SET);
   for ( i = 0; i < N_SIGS; i++)
      if ( memcmp( buf, signatures[ i]. sig, signatures[ i]. size) == 0)
         return signatures[ i]. type;
   return itUnknown;
}

void
init_image_support()
{
   cm_init_colormap();
}

#ifdef __cplusplus
}
#endif
