#include "apricot.h"
#include "Icon.h"
#include "Icon.inc"

#undef  my
#define inherited CImage->
#define my  ((( PIcon) self)-> self)->
#define var (( PIcon) self)->

extern void ic_nibble_byte_ictNone( Handle, void *, PRGBColor, int);
extern void ic_rgb_byte_ictNone( Handle, void *, PRGBColor, int);


void
produce_mask( Handle self)
{
   Byte * area8 = var data;
   Byte * dest = var mask;
   Byte * src;
   Byte color;
   int i;
   int line8Size = (( var w * 8 + 31) / 32) * 4;
   int bpp = var type & imBPP;

   if ( var w == 0 || var h == 0) return;

   if ( bpp == imMono)
   {
      // mono case simplifies our task
      int j = var maskSize;
      Byte * mask = var mask;
      memcpy ( var mask, var data, var dataSize);
      while ( j--) mask[ j] = ~mask[ j];
      var palette[0]. r = var palette[0]. g = var palette[0]. b = 0;
      return;
   }

   if ( bpp != im256)
   {
      area8 = malloc ( var h * line8Size);
      // convert to 8 bit
      switch ( bpp)
      {
         case im16:
            ic_nibble_byte_ictNone( self, area8, var palette, im256);
            break;
         case imRGB:
            ic_rgb_byte_ictNone( self, area8, var palette, im256);
            break;
      }
   }

   {  // calculate transparent color
      Byte corners [4];
      Byte counts  [4] = {1, 1, 1, 1};
      int j, k;

      // retrieving corner pixels
      corners[ 0] = area8[ 0];
      corners[ 1] = area8[ var w - 1];
      corners[ 2] = area8[ line8Size * var h - line8Size];
      corners[ 3] = area8[ line8Size * ( var h - 1) + var w - 1];

      // preliminary ponos comparison
      for ( j = 0; j < 4; j++) {
         if (
             (( var palette[ corners[ j]]. b) == 0) &&
             (( var palette[ corners[ j]]. g) == 128) &&
             (( var palette[ corners[ j]]. r) == 128)) {
            color = corners[ j];
            goto colorFound;
         }
      }

      color = corners[ 3]; // our wild (and possibly bad) guess
      // sorting
      for (j = 0; j < 3; j++)
         for (k = 0; k < 3; k++)
            if ( corners[ k] < corners[ k + 1]) {
               Byte l = corners[ k];
               corners[ k] = corners[ k + 1];
               corners[ k + 1] = l;
            }
      // forming maximum's vector
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
      if (( counts[0] > 2) || (( counts[0] == 2) && ( counts[1] == 1)))
        color = corners[ 0]; else
      {
         int colorsToCompare = ( counts[0] == 2) ? 2 : 4;

         // compare to ponos
         for ( j = 0; j < colorsToCompare; j++)
            if (( var palette[ corners[ j]]. b < 20) &&
                ( var palette[ corners[ j]]. r > 100) &&
                ( var palette[ corners[ j]]. r < 150) &&
                ( var palette[ corners[ j]]. g > 100) &&
                ( var palette[ corners[ j]]. g < 150))
            {
               color = corners[ j];
               goto colorFound;
            }

         // compare to ponos in a MicroSoft's terminology
         for ( j = 0; j < colorsToCompare; j++)
            if (( var palette[ corners[ j]]. g < 20) &&
                ( var palette[ corners[ j]]. r > 200) &&
                ( var palette[ corners[ j]]. b > 200))
            {
               color = corners[ j];
               goto colorFound;
            }

         // We're in BIG trouble...
colorFound:;
      }
   }

   // processing transparency
   memset( var mask, 0, var maskSize);
   src  = area8;
   for ( i = 0; i < var h; i++, dest += var maskLine, src += line8Size)
   {
      int j;
      for ( j = 0; j < var w; j++)
         if ( src[ j] == color)
            dest[ j >> 3] |= 1 << (7 - ( j & 7));
   }

   // finalize
   if ( bpp != im256)
   {
      free ( var data);
      var data = area8;
      var type = im256;
      var lineSize = line8Size;
      var dataSize = line8Size * var h;
   }

   var palette[ color]. r = var palette[ color]. b = var palette[ color]. g = 0;
}


void
Icon_update_change( Handle self)
{
   inherited update_change( self);
   free( var mask);
   if ( var data)
   {
      var maskLine = (( var w + 31) / 32) * 4;
      var maskSize = var maskLine * var h;
      var mask = malloc ( var maskSize);
      produce_mask( self);
   }
   else
      var mask = nil;
}

void
Icon_update_mask_change( Handle self)
{
   if ( !var data) return;
   produce_mask( self);
}

void
Icon_create_empty( Handle self, int width, int height, int type)
{
   inherited create_empty( self, width, height, type);
   free( var mask);
   if ( var data)
   {
      var maskLine = (( var w + 31) / 32) * 4;
      var maskSize = var maskLine * var h;
      var mask = malloc ( var maskSize);
      memset( var mask, 0, var maskSize);
   }
   else
      var mask = nil;
}

Handle
Icon_dup( Handle self)
{
   Handle h = inherited dup( self);
   PIcon  i = ( PIcon) h;
   memcpy( i-> mask, var mask, var maskSize);
   return h;
}

IconHandle
Icon_split( Handle self)
{
   IconHandle ret = {0,0};
   PImage i;
   HV * profile = newHV();
   char* className = var self-> className;

   pset_H( owner,        var owner);
   pset_i( width,        var w);
   pset_i( height,       var h);
   pset_i( type,         imMono|imGrayScale);
   ret. andMask = Object_create( "Image", profile);
   sv_free(( SV *) profile);
   i = ( PImage) ret. andMask;
   memcpy( i-> data, var mask, var maskSize);

   var self-> className = inherited className;
   ret. xorMask         = inherited dup( self);
   var self-> className = className;

   --SvREFCNT( SvRV( i-> mate));
   return ret;
}

void
Icon_combine( Handle self, Handle xorMask, Handle andMask)
{
   Bool killAM = 0;
   if ( !kind_of( xorMask, CImage) || !kind_of( andMask, CImage))
      return;
   my create_empty( self, PImage( xorMask)-> w, PImage( xorMask)-> h, PImage( xorMask)-> type);
   if (( PImage( andMask)-> type & imBPP) != imMono) {
      killAM = 1;
      andMask = CImage( andMask)-> dup( andMask);
      CImage( andMask)-> set_type( andMask, imMono);
   }
   if ( var w != PImage( andMask)-> w || var h != PImage( andMask)-> h) {
      if ( !killAM) {
         killAM = 1;
         andMask = CImage( andMask)-> dup( andMask);
      }
      CImage( andMask)-> set_size( andMask, var w, var h);
   }

   memcpy( var data, PImage( xorMask)-> data, var dataSize);
   memcpy( var mask, PImage( andMask)-> data, var maskSize);

   if ( killAM) Object_destroy( andMask);
}
