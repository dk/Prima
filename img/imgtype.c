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
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif



#define var (( PImage) self)

/* Color mappers */
#define BCPARMS      self, dstData, dstPal, dstType, dstPalSize, palSize_only
#define BCSELFGRAY   self, var->data, dstPal, imByte, dstPalSize, palSize_only

static void
ic_Byte_convert( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only, Bool inplace)
{
   int new_data_size = (( var->w * 8 + 31) / 32) * 4 * var-> h;
   Byte * new_data;
   RGBColor dummy_pal[256];
   int dummy_pal_size = 0;

   if ( !inplace) {
      new_data = allocb( new_data_size);
      if ( !new_data) {
         croak("Not enough memory:%d bytes", new_data_size);
         return;
      }
      memset( new_data, 0, new_data_size);
   } else
      new_data = var-> data;
   ic_type_convert( self, new_data, dummy_pal, imByte, &dummy_pal_size, false);
   if ( !inplace) {
      free( var-> data);
      var-> data = new_data;
   }
   var-> type = imByte;
   var-> dataSize = new_data_size;
   var-> lineSize = new_data_size / var-> h;
   memcpy( var-> palette, std256gray_palette, sizeof(std256gray_palette));
   var-> palSize = 256;
   ic_type_convert( self, dstData, dstPal, dstType, dstPalSize, palSize_only);
}

static void
ic_raize_palette( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)
{
   Byte * odata = var-> data;
   int otype = var-> type, dummy_pal_size = 0;
   ic_type_convert( self, dstData, var-> palette, dstType, &dummy_pal_size, false);
   var-> palSize = dummy_pal_size;
   var-> type = dstType;
   var-> data = dstData;
   ic_type_convert( self, dstData, dstPal, dstType, dstPalSize, palSize_only);
   var-> data = odata;
   var-> type = otype;
}

void
ic_type_convert( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)
{
   int srcType = var->type;
   int orgDstType = dstType;

   /* remove redundant combinations */
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
  
   /* fill grayscale palette, if any */
   if ( orgDstType & imGrayScale) 
      switch( orgDstType & imBPP) {
      case imbpp1: 
         memcpy( dstPal, stdmono_palette, sizeof( stdmono_palette)); 
         *dstPalSize = 2;
         break;
      case imbpp4: 
         memcpy( dstPal, std16gray_palette, sizeof( std16gray_palette)); 
         *dstPalSize = 16;
         break;
      case imbpp8: 
         memcpy( dstPal, std256gray_palette, sizeof( std256gray_palette)); 
         *dstPalSize = 256;
         break;
      }

   /* no palette conversion, same type, - out */
   if ( srcType == dstType && (*dstPalSize == 0 || (
       (srcType != imbpp1) && (srcType != imbpp4) && (srcType != imbpp8) 
    ))) {
      memcpy( dstData, var->data, var->dataSize);
      if (( orgDstType & imGrayScale) == 0) {
         memcpy( dstPal, var->palette, var->palSize);
         *dstPalSize = var-> palSize;
      }
      return;
   }

   switch( srcType)
   {
      case imMono: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictOrdered:
               case ictErrorDiffusion:
                   if ( palSize_only || *dstPalSize != 0) {
                      palSize_only = false;
                      *dstPalSize = 0;
                   }
               case ictNone:
                   ic_mono_mono_ictNone(BCPARMS); 
                   break;
               case ictOptimized:
                   ic_mono_mono_ictOptimized(BCPARMS); 
                   break;
            }
            break;
         case im16:      
            if ( *dstPalSize > 0) 
               ic_raize_palette(BCPARMS);
            else
               ic_mono_nibble_ictNone(BCPARMS);
            break;
         case im256:
            if ( *dstPalSize > 0)
               ic_raize_palette(BCPARMS);
            else 
               ic_mono_byte_ictNone(BCPARMS);
            break;
         case imByte:    ic_mono_graybyte_ictNone(BCPARMS);  break;
         case imRGB:     ic_mono_rgb_ictNone(BCPARMS);       break;
         case imShort: case imLong: case imFloat: 
         case imDouble: case imComplex: case imDComplex:
             ic_Byte_convert( BCPARMS, false);
             break;
      }
      break; /* imMono */

      case im16: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_nibble_mono_ictNone(BCPARMS);     
                  break;
               case ictOrdered:
                  ic_nibble_mono_ictOrdered(BCPARMS); 
                  break;
               case ictOptimized:
                  if ( *dstPalSize > 0) {
                     ic_nibble_mono_ictOptimized(BCPARMS); break;
                     break;
                  }
               case ictErrorDiffusion:
                  ic_nibble_mono_ictErrorDiffusion(BCPARMS); 
                  break;   
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictOrdered:
               case ictErrorDiffusion:
                   if ( palSize_only || *dstPalSize != 0) {
                      palSize_only = false;
                      *dstPalSize = 0;
                   }
               case ictNone:
                   ic_nibble_nibble_ictNone(BCPARMS); 
                   break;
               case ictOptimized:
                   ic_nibble_nibble_ictOptimized(BCPARMS); 
                   break;
            }
            break;
         case im256:
            if ( *dstPalSize > 0) 
               ic_raize_palette(BCPARMS);
            else 
               ic_nibble_byte_ictNone(BCPARMS);
            break;
         case imByte:
            ic_nibble_graybyte_ictNone(BCPARMS);       break;
         case imRGB:
            ic_nibble_rgb_ictNone(BCPARMS);            break;
         case imShort: case imLong: case imFloat: 
         case imDouble: case imComplex: case imDComplex:
             ic_Byte_convert( BCPARMS, false);
             break;
      }
      break; /* im16 */

      case im256: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_byte_mono_ictNone(BCPARMS);       
                  break;
               case ictOrdered:
                  ic_byte_mono_ictOrdered(BCPARMS);   
                  break;
               case ictOptimized:
                  if ( *dstPalSize > 0) {
                     ic_byte_mono_ictOptimized(BCPARMS); break;
                     break;
                  }
               case ictErrorDiffusion:
                  ic_byte_mono_ictErrorDiffusion(BCPARMS); 
                  break;
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictNone:
                   ic_byte_nibble_ictNone(BCPARMS);     
                   break;
               case ictOrdered:
                   ic_byte_nibble_ictOrdered(BCPARMS); break;
               case ictErrorDiffusion:
                   ic_byte_nibble_ictErrorDiffusion(BCPARMS); break;    
               case ictOptimized:
                   ic_byte_nibble_ictOptimized(BCPARMS); break;    
            }
            break;
         case im256:
            switch ( var->conversion)
            {
               case ictOrdered:
               case ictErrorDiffusion:
                   if ( palSize_only || *dstPalSize != 0) {
                      palSize_only = false;
                      *dstPalSize = 0;
                   }
               case ictNone:
                   ic_byte_byte_ictNone(BCPARMS); 
                   break;
               case ictOptimized:
                   ic_byte_byte_ictOptimized(BCPARMS); 
                   break;
            }
            break;
         case imByte:
            ic_byte_graybyte_ictNone(BCPARMS);          break;
         case imRGB:
            ic_byte_rgb_ictNone(BCPARMS); break;
         case imShort: case imLong: case imFloat: 
         case imDouble: case imComplex: case imDComplex:
             ic_Byte_convert( BCPARMS, false);
             break;
      }
      break; /* im256 */

      case imByte: switch( dstType)
      {
         case imMono:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_byte_mono_ictNone(BCPARMS); 
                  break;
               case ictOrdered:
                  ic_graybyte_mono_ictOrdered(BCPARMS); break;
               case ictOptimized:
                  if ( *dstPalSize > 0) {
                     ic_byte_mono_ictOptimized(BCPARMS); break;
                     break;
                  }
               case ictErrorDiffusion:
                  ic_graybyte_mono_ictErrorDiffusion(BCPARMS); break;   
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_byte_nibble_ictNone(BCPARMS); 
                  break;
               case ictOrdered:
                  ic_graybyte_nibble_ictOrdered(BCPARMS); 
                  break;
               case ictOptimized:
                  if ( *dstPalSize > 0) {
                     ic_byte_nibble_ictOptimized(BCPARMS); 
                     break;    
                  }
               case ictErrorDiffusion:
                  ic_graybyte_nibble_ictErrorDiffusion(BCPARMS); 
                  break;    
            }
            break;
         case im256:
            switch ( var->conversion)
            {
               case ictOrdered:
               case ictErrorDiffusion:
                   if ( palSize_only || *dstPalSize != 0) {
                      palSize_only = false;
                      *dstPalSize = 0;
                   }
               case ictNone:
                  ic_byte_byte_ictNone(BCPARMS); 
                  break;
               case ictOptimized:
                  ic_byte_byte_ictOptimized(BCPARMS); 
                  break;
            }
            break;
         case imRGB:
            ic_graybyte_rgb_ictNone(BCPARMS); break;
         case imShort  : ic_Byte_Short( BCPARMS);  break;
         case imLong   : ic_Byte_Long( BCPARMS);   break;
         case imFloat  : ic_Byte_float( BCPARMS);  break;
         case imDouble : ic_Byte_double( BCPARMS); break;
         case imComplex: ic_Byte_float_complex(BCPARMS); break;
         case imDComplex: ic_Byte_double_complex(BCPARMS); break;
         break;         
      }
      break; /* imByte */

      case imShort:  switch ( dstType)
      {
         case imMono: case im16: case imRGB:
            ic_Byte_convert( BCPARMS, true);
            break;
         case im256:
            if ( *dstPalSize > 0) {
               ic_Byte_convert( BCPARMS, true);
               break;
            }
         case imByte   : ic_Short_Byte( BCPARMS);   break;
         case imLong   : ic_Short_Long( BCPARMS);   break;
         case imFloat  : ic_Short_float( BCPARMS);  break;
         case imDouble : ic_Short_double( BCPARMS); break;
         case imComplex: ic_Short_float_complex(BCPARMS); break;
         case imDComplex: ic_Short_double_complex(BCPARMS); break;
      }
      break;
      /* imShort */

      case imLong:  switch ( dstType)
      {
         case imMono: case im16: case imRGB:
            ic_Byte_convert( BCPARMS, true);
            break;
         case im256:
            if ( *dstPalSize > 0) {
               ic_Byte_convert( BCPARMS, true);
               break;
            }
         case imByte   : ic_Long_Byte( BCPARMS);   break;
         case imShort  : ic_Long_Short( BCPARMS);  break;
         case imFloat  : ic_Long_float( BCPARMS);  break;
         case imDouble : ic_Long_double( BCPARMS); break;
         case imComplex: ic_Long_float_complex(BCPARMS); break;
         case imDComplex: ic_Long_double_complex(BCPARMS); break;
      }
      break;
      /* imLong */

      case imFloat:  switch ( dstType)
      {
         case imMono: case im16: case imRGB:
            ic_Byte_convert( BCPARMS, true);
            break;
         case im256:
            if ( *dstPalSize > 0) {
               ic_Byte_convert( BCPARMS, true);
               break;
            }
         case imByte   : ic_float_Byte( BCPARMS);   break;
         case imShort  : ic_float_Short( BCPARMS);  break;
         case imLong   : ic_float_Long( BCPARMS);   break;
         case imDouble : ic_float_double( BCPARMS); break;
         case imComplex: ic_float_float_complex(BCPARMS); break;
         case imDComplex: ic_float_double_complex(BCPARMS); break;
      }
      break;
      /* imFloat */


      case imDouble:  switch ( dstType)
      {
         case imMono: case im16: case imRGB:
            ic_Byte_convert( BCPARMS, true);
            break;
         case im256:
            if ( *dstPalSize > 0) {
               ic_Byte_convert( BCPARMS, true);
               break;
            }
         case imByte   : ic_double_Byte( BCPARMS);   break;
         case imShort  : ic_double_Short( BCPARMS);  break;
         case imLong   : ic_double_Long( BCPARMS);   break;
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
               case ictOrdered:
                  ic_rgb_mono_ictOrdered(BCPARMS); break;
               case ictOptimized:
                  if ( *dstPalSize > 0) {
                     ic_rgb_mono_ictOptimized(BCPARMS); break;
                     break;
                  }
               case ictErrorDiffusion:
                  ic_rgb_mono_ictErrorDiffusion(BCPARMS); break;
            }
            break;
         case im16:
            switch ( var->conversion)
            {
               case ictNone:
                  ic_rgb_nibble_ictNone(BCPARMS); break;
               case ictOrdered:
                  ic_rgb_nibble_ictOrdered(BCPARMS); break;
               case ictErrorDiffusion:
                  ic_rgb_nibble_ictErrorDiffusion(BCPARMS); break;   
               case ictOptimized:
                  ic_rgb_nibble_ictOptimized(BCPARMS); break;
            }
            break;
         case im256:
            switch ( var->conversion)
            {
            case ictNone:
               ic_rgb_byte_ictNone(BCPARMS); break;
            case ictOrdered:
               ic_rgb_byte_ictOrdered(BCPARMS); break;
            case ictErrorDiffusion:
               ic_rgb_byte_ictErrorDiffusion(BCPARMS); break;
            case ictOptimized:
               ic_rgb_byte_ictOptimized(BCPARMS); break;
            }
            break;
         case imByte:
            ic_rgb_graybyte_ictNone(BCPARMS); 
            break;
         case imShort: case imLong: case imFloat: 
         case imDouble: case imComplex: case imDComplex:
            ic_Byte_convert( BCPARMS, false);
            break;
      }
      break; /* imRGB */

      case imComplex: switch( dstType) {
          case imMono: case im16: case imRGB:
             ic_Byte_convert( BCPARMS, true);
             break;
          case im256:   
            if ( *dstPalSize > 0) {
               ic_Byte_convert( BCPARMS, true);
               break;
            }
          case imByte:    ic_float_complex_Byte(BCPARMS); break;
          case imShort:   ic_float_complex_Short(BCPARMS); break;
          case imLong:    ic_float_complex_Long(BCPARMS); break;
          case imDouble:  ic_float_complex_double(BCPARMS); break;
          case imFloat:   ic_float_complex_float( BCPARMS); break;
      }
      break;
      /* imComplex */

      case imDComplex: switch( dstType) {
          case imMono: case im16: case imRGB:
             ic_Byte_convert( BCPARMS, true);
             break;
          case im256:
            if ( *dstPalSize > 0) {
               ic_Byte_convert( BCPARMS, true);
               break;
            }
          case imByte:    ic_double_complex_Byte(BCPARMS); break;
          case imShort:   ic_double_complex_Short(BCPARMS); break;
          case imLong:    ic_double_complex_Long(BCPARMS); break;
          case imDouble:  ic_double_complex_double(BCPARMS); break;
          case imFloat:   ic_double_complex_float( BCPARMS); break;
      }     
      break;
      /* imDComplex */
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

void
init_image_support(void)
{
   cm_init_colormap();
}

#ifdef __cplusplus
}
#endif
