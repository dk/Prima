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

#include "apricot.h"
#include "Icon.h"
#include "img_conv.h"
#include <Icon.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CImage->
#define my  ((( PIcon) self)-> self)
#define var (( PIcon) self)

static void
produce_mask( Handle self)
{
   Byte * area8 = var-> data;
   Byte * dest = var-> mask;
   Byte * src;
   Byte color = 0;
   RGBColor rgbcolor;
   int i, bpp2;
   int line8Size = (( var-> w * 8 + 31) / 32) * 4, areaLineSize = var-> lineSize;
   int bpp = var-> type & imBPP;
   int w = var-> w, h = var-> h;

   if ( var-> w == 0 || var-> h == 0) return;

   if ( var-> autoMasking == amNone) return;

   if ( var-> autoMasking == amMaskColor) {
      rgbcolor. b = var-> maskColor & 0xFF;
      rgbcolor. g = (var-> maskColor >> 8)  & 0xFF;
      rgbcolor. r = (var-> maskColor >> 16) & 0xFF;
      if ( bpp <= 8)
         color = cm_nearest_color( rgbcolor, var-> palSize, var-> palette);
   } else if ( var-> autoMasking == amMaskIndex) {
      if ( bpp > 8) return;	 
      color = var-> maskIndex;
      bzero( &rgbcolor, sizeof(rgbcolor));
   }

   if ( bpp == imMono) {
      /* mono case simplifies our task */
      int  j = var-> maskSize;
      Byte * mask = var-> mask;
      memcpy ( var-> mask, var-> data, var-> dataSize);
      if ( color == 0) {
         while ( j--) mask[ j] = ~mask[ j];
      }
      var-> palette[color]. r = var-> palette[color]. g = var-> palette[color]. b = 0;
      if ( color > 0)
         var-> type &= ~imGrayScale;
      return;
   }

   /* convert to 8 bit */
   switch ( bpp)
   {
   case im16:
   case im256:
   case imRGB:
      bpp2 = bpp;
      break;
   default:
      bpp2  = im256;
      areaLineSize = line8Size;
      if (!( area8 = allocb( var-> h * line8Size))) return;
      ic_type_convert( self, area8, var-> palette, im256 | ( var-> type & imGrayScale), &var-> palSize, false);
      break;
   }

   if ( var-> autoMasking == amAuto) {  /* calculate transparent color */
      Byte corners [4];
      Byte counts  [4] = {1, 1, 1, 1};
      RGBColor rgbcorners[4];
      int j = areaLineSize, k;

      /* retrieving corner pixels */
      switch ( bpp2) {
      case im16:
         corners[ 0] = area8[ 0] >> 4;
         corners[ 1] = area8[( w - 1) >> 1];
         corners[ 1] = (( w - 1) & 1) ? corners[ 1] & 0x0f : corners[ 1] >> 4;
         corners[ 2] = area8[ j * ( h - 1)] >> 4;
         corners[ 3] = area8[ j * ( h - 1) + (( w - 1) >> 1)];
         corners[ 3] = (( w - 1) & 1) ? corners[ 3] & 0x0f : corners[ 3] >> 4;
         for ( j = 0; j < 4; j++) {
            rgbcorners[j].r = var-> palette[ corners[ j]]. r;
            rgbcorners[j].g = var-> palette[ corners[ j]]. g;
            rgbcorners[j].b = var-> palette[ corners[ j]]. b;
         }
         break;
      case im256:
         corners[ 0] = area8[ 0];
         corners[ 1] = area8[ w - 1];
         corners[ 2] = area8[ j * ( h - 1)];
         corners[ 3] = area8[ j * ( h - 1) + w - 1];
         for ( j = 0; j < 4; j++) {
            rgbcorners[j].r = var-> palette[ corners[ j]]. r;
            rgbcorners[j].g = var-> palette[ corners[ j]]. g;
            rgbcorners[j].b = var-> palette[ corners[ j]]. b;
         }
         break;
      case imRGB:
         rgbcorners[0] = *(PRGBColor)( area8);
         rgbcorners[1] = *(PRGBColor)( area8 + ( w - 1) * 3);
         rgbcorners[2] = *(PRGBColor)( area8 + j * ( h - 1));
         rgbcorners[3] = *(PRGBColor)( area8 + j * ( h - 1) + ( w - 1) * 3);
         for ( j = 0; j < 4; j++) corners[j] = j;
         #define rgbcmp(x,y) ((rgbcorners[x].r == rgbcorners[y].r) &&\
                              (rgbcorners[x].g == rgbcorners[y].g) &&\
                              (rgbcorners[x].b == rgbcorners[y].b))
         if ( rgbcmp(1,0)) corners[1] = 0;
         if ( rgbcmp(2,0)) corners[2] = 0;
         if ( rgbcmp(3,0)) corners[3] = 0;
         if ( rgbcmp(2,1)) corners[2] = corners[1];
         if ( rgbcmp(3,1)) corners[3] = corners[1];
         if ( rgbcmp(3,2)) corners[3] = corners[2];
         #undef rgbcmp
         break;
      }

      /* preliminary comparison to a transparent color candidate ( hack ) */
      for ( j = 0; j < 4; j++) {
         if (
             (( rgbcorners[j]. b) == 0) &&
             (( rgbcorners[j]. g) == 128) &&
             (( rgbcorners[j]. r) == 128)) {
            color = corners[ j];
            rgbcolor = rgbcorners[ j];
            goto colorFound;
         }
      }

      color = corners[ 3]; /* our wild (and possibly bad) guess */
      rgbcolor = rgbcorners[ 3];

      /* sorting */
      for ( j = 0; j < 3; j++)
         for (k = 0; k < 3; k++)
            if ( corners[ k] < corners[ k + 1]) {
               Byte l = corners[ k];
               corners[ k] = corners[ k + 1];
               corners[ k + 1] = l;
            }
      /* forming maximum's vector */
      i = 0;
      for (j = 0; j < 3; j++) if ( corners[ j + 1] == corners[ j]) counts[ i]++; else i++;
      for (j = 0; j < 3; j++)
         for (k = 0; k < 3; k++)
            if ( counts[ k] < counts[ k + 1]) {
               Byte l = counts[ k];
               counts[ k] = counts[ k + 1];
               counts[ k + 1] = l;
               l = corners[ k];
               corners[ k] = corners[ k + 1];
               corners[ k + 1] = l;
            }
      if (( counts[0] > 2) || (( counts[0] == 2) && ( counts[1] == 1))) {
        color = corners[ 0];
        rgbcolor = rgbcorners[ 0];
      } else {
         int colorsToCompare = ( counts[0] == 2) ? 2 : 4;

         /* compare to that yellowish hue... */
         for ( j = 0; j < colorsToCompare; j++)
            if (( rgbcorners[j]. b < 20) &&
                ( rgbcorners[j]. r > 100) &&
                ( rgbcorners[j]. r < 150) &&
                ( rgbcorners[j]. g > 100) &&
                ( rgbcorners[j]. g < 150))
            {
               color = corners[ j];
               rgbcolor = rgbcorners[ j];
               goto colorFound;
            }

         /* compare to MicroSoft Pink */
         for ( j = 0; j < colorsToCompare; j++)
            if (( rgbcorners[j]. g < 20) &&
                ( rgbcorners[j]. r > 200) &&
                ( rgbcorners[j]. b > 200))
            {
               color = corners[ j];
               rgbcolor = rgbcorners[ j];
               goto colorFound;
            }
      }
colorFound:;
   } 

   /* processing transparency */
   memset( var-> mask, 0, var-> maskSize);
   src  = area8;
   for ( i = 0; i < h; i++, dest += var-> maskLine, src += areaLineSize) {
      register int j;
      switch ( bpp2) {
      case im16:
         {
            int max = ( w >> 1) + ( w & 1);
            register int k = 0;
            for ( j = 0; j < max; j++) {
               if ( color == ( src[ j] >> 4))
                  dest[ k >> 3] |= 1 << (7 - ( k & 7));
               if ( color == ( src[ j] & 0x0f))
                  dest[ k >> 3] |= 1 << (6 - ( k & 7));
               k += 2;
            }
         }
         break;
      case imRGB:
         {
            register PRGBColor r = ( PRGBColor) src;
            for ( j = 0; j < w; j++) {
               if (( r-> r == rgbcolor.r) &&
                   ( r-> g == rgbcolor.g) &&
                   ( r-> b == rgbcolor.b))
                   dest[ j >> 3] |= 1 << (7 - ( j & 7));
               r++;
            }
         }
         break;
      default:
         for ( j = 0; j < w; j++)
            if ( src[ j] == color)
               dest[ j >> 3] |= 1 << (7 - ( j & 7));
      }
   }

   /* finalize */
   if ( var-> data != area8) free( area8);
   if ( var-> palSize > color && bpp <= im256) {
      var-> palette[ color]. r = var-> palette[ color]. b = var-> palette[ color]. g = 0;
      if ( color > 0)
         var-> type &= ~imGrayScale;
   }
}

void
Icon_init( Handle self, HV * profile)
{
   dPROFILE;
   inherited init( self, profile);
   my-> set_maskColor( self, pget_i( maskColor));
   my-> set_maskIndex( self, pget_i( maskIndex));
   my-> set_autoMasking( self, pget_i( autoMasking));
   my-> set_mask( self, pget_sv( mask));
   CORE_INIT_TRANSIENT(Icon);
}


SV *
Icon_mask( Handle self, Bool set, SV * svmask)
{
   STRLEN maskSize;
   void * mask;
   int am = var-> autoMasking;
   if ( var-> stage > csFrozen) return nilSV;
   if ( !set)
      return newSVpvn(( char *) var-> mask, var-> maskSize);
   mask = SvPV( svmask, maskSize);
   if ( is_opt( optInDraw) || maskSize <= 0) return nilSV;
   memcpy( var-> mask, mask, maskSize > var-> maskSize ? var-> maskSize : maskSize);
   var-> autoMasking = amNone;
   my-> update_change( self);
   var-> autoMasking = am;
   return nilSV;
}

int
Icon_autoMasking( Handle self, Bool set, int autoMasking)
{
   if ( !set)
      return var-> autoMasking;
   if ( var-> autoMasking == autoMasking) return 0;
   var-> autoMasking = autoMasking;
   if ( is_opt( optInDraw)) return 0;
   my-> update_change( self);
   return 0;
}   

Color
Icon_maskColor( Handle self, Bool set, Color color)
{
   if ( !set)
      return var-> maskColor;
   if ( var-> maskColor == color) return 0;
   var-> maskColor = color;
   if ( is_opt( optInDraw)) return 0;
   if ( var-> autoMasking == amMaskColor) 
      my-> update_change( self);
   return clInvalid;
}   

int
Icon_maskIndex( Handle self, Bool set, int index)
{
   if ( !set)
      return var-> maskIndex;
   var-> maskIndex = index;
   if ( is_opt( optInDraw)) return 0;
   if ( var-> autoMasking == amMaskIndex) 
      my-> update_change( self);
   return -1;
}   

void
Icon_update_change( Handle self)
{
   inherited update_change( self);

   if ( var-> autoMasking == amNone) {
      int maskLine = (( var-> w + 31) / 32) * 4;
      int maskSize = maskLine * var-> h;
      if ( maskLine != var-> maskLine || maskSize != var-> maskSize) {
         free( var-> mask);
         var-> maskLine = maskLine;
         if (!( var-> mask = allocb( var-> maskSize = maskSize)) && maskSize > 0) {
            my-> make_empty( self);
            warn("Not enough memory: %d bytes", maskSize);
         } else
            memset( var-> mask, 0, maskSize);
      }
      return;
   }   
   
   free( var-> mask);
   if ( var-> data)
   {
      var-> maskLine = (( var-> w + 31) / 32) * 4;
      var-> maskSize = var-> maskLine * var-> h;
      if ( !( var-> mask = allocb( var-> maskSize)) && var-> maskSize > 0) {
          my-> make_empty( self);
          warn("Not enough memory: %d bytes", var-> maskSize);
          return;
      }
      produce_mask( self);
   }
   else
      var-> mask = nil;
}

void
Icon_stretch( Handle self, int width, int height)
{
   Byte * newMask = nil;
   int lineSize, oldW = var-> w, oldH = var-> h, am = var-> autoMasking;
   if ( var->stage > csFrozen) return;
   if ( width  >  65535) width  =  65535;
   if ( height >  65535) height =  65535;
   if ( width  < -65535) width  = -65535;
   if ( height < -65535) height = -65535;
   if (( width == var->w) && ( height == var->h)) return;
   if ( width == 0 || height == 0)
   {
      my->create_empty( self, 0, 0, var->type);
      return;
   }
   
   lineSize = (( abs( width) + 31) / 32) * 4;
   newMask  = allocb( lineSize * abs( height));
   if ( newMask == nil && lineSize > 0) {
      my-> make_empty( self);
      croak("Icon::stretch: cannot allocate %d bytes", lineSize * abs( height));
   }
   var-> autoMasking = amNone;
   if ( var-> mask) 
      ic_stretch( imMono, var-> mask, oldW, oldH, newMask, width, height, is_opt( optHScaling), is_opt( optVScaling));
   inherited stretch( self, width, height);
   free( var-> mask);
   var->mask = newMask;
   var->maskLine = lineSize;
   var->maskSize = lineSize * abs( height);
   inherited stretch( self, width, height);
   var-> autoMasking = am;
}

void
Icon_create_empty( Handle self, int width, int height, int type)
{
   inherited create_empty( self, width, height, type);
   free( var-> mask);
   if ( var-> data)
   {
      var-> maskLine = (( var-> w + 31) / 32) * 4;
      var-> maskSize = var-> maskLine * var-> h;
      if ( !( var-> mask = allocb( var-> maskSize)) && var-> maskSize > 0) {
         my-> make_empty( self);
         warn("Not enough memory: %d bytes", var-> maskSize);
         return;
      }
      memset( var-> mask, 0, var-> maskSize);
   }
   else
      var-> mask = nil;
}

Handle
Icon_dup( Handle self)
{
   Handle h = inherited dup( self);
   PIcon  i = ( PIcon) h;
   memcpy( i-> mask, var-> mask, var-> maskSize);
   i-> autoMasking = var-> autoMasking;
   i-> maskColor   = var-> maskColor;
   i-> maskIndex   = var-> maskIndex;
   return h;
}

IconHandle
Icon_split( Handle self)
{
   IconHandle ret = {0,0};
   PImage i;
   HV * profile = newHV();
   char* className = var-> self-> className;

   pset_H( owner,        var-> owner);
   pset_i( width,        var-> w);
   pset_i( height,       var-> h);
   pset_i( type,         imMono|imGrayScale);
   pset_i( conversion,   var->conversion);
   pset_i( hScaling,     is_opt( optHScaling));
   pset_i( vScaling,     is_opt( optVScaling));
   pset_i( preserveType, is_opt( optPreserveType));

   ret. andMask = Object_create( "Prima::Image", profile);
   sv_free(( SV *) profile);
   i = ( PImage) ret. andMask;
   memcpy( i-> data, var-> mask, var-> maskSize);
   i-> self-> update_change(( Handle) i);

   var-> self-> className = inherited className;
   ret. xorMask         = inherited dup( self);
   var-> self-> className = className;

   --SvREFCNT( SvRV( i-> mate));
   return ret;
}

void
Icon_combine( Handle self, Handle xorMask, Handle andMask)
{
   Bool killAM = 0;
   int am = var-> autoMasking;

   
   if ( !kind_of( xorMask, CImage) || !kind_of( andMask, CImage))
      return;
   my-> create_empty( self, PImage( xorMask)-> w, PImage( xorMask)-> h, PImage( xorMask)-> type);
   if (( PImage( andMask)-> type & imBPP) != imMono) {
      killAM = 1;
      andMask = CImage( andMask)-> dup( andMask);
      CImage( andMask)-> set_type( andMask, imMono);
   }
   if ( var-> w != PImage( andMask)-> w || var-> h != PImage( andMask)-> h) {
      if ( !killAM) {
         killAM = 1;
         andMask = CImage( andMask)-> dup( andMask);
      }
      CImage( andMask)-> set_size( andMask, my-> get_size( self));
   }

   memcpy( var-> data, PImage( xorMask)-> data, var-> dataSize);
   memcpy( var-> mask, PImage( andMask)-> data, var-> maskSize);
   memcpy( var-> palette, PImage( xorMask)-> palette, 768);
   var-> palSize = PImage( xorMask)-> palSize;

   if ( killAM) Object_destroy( andMask);

   var-> autoMasking = amNone;
   my-> update_change( self);
   var-> autoMasking = am;
}

#ifdef __cplusplus
}
#endif
