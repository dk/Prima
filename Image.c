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
 * $Id$
 */

#include "img.h"

#ifndef __unix
#   include <io.h>
#else
#   include <unistd.h>
#endif /* __unix */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include "apricot.h"
#include "Image.h"
#include "img_conv.h"
#include <Image.inc>
#include "Clipboard.h"

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CDrawable->
#define my  ((( PImage) self)-> self)
#define var (( PImage) self)

static Bool Image_set_extended_data( Handle self, HV * profile);

void
Image_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   var->w = pget_i( width);
   var->h = pget_i( height);
   var->conversion = pget_i( conversion);
   opt_assign( optHScaling, pget_B( hScaling));
   opt_assign( optVScaling, pget_B( vScaling));
   if ( !itype_supported( var-> type = pget_i( type))) 
      if ( !itype_importable( var-> type, &var-> type, nil, nil)) {
         warn( "Image::init: cannot set type %08x", var-> type);
         var-> type = imBW;
      }   
   var->lineSize = (( var->w * ( var->type & imBPP) + 31) / 32) * 4;
   var->dataSize = ( var->lineSize) * var->h;
   if ( var-> dataSize > 0) {
      var->data = allocb( var->dataSize);
      if ( var-> data == nil) 
         croak("Image::init: cannot allocate %d bytes", var-> dataSize);
   } else 
      var-> data = nil;
   free( var->palette);
   var->palette = allocn( RGBColor, 256);
   if ( var-> palette == nil) {
      free( var-> data);
      var-> data = nil;
      croak("Image::init: cannot allocate %d bytes", 768);
   }   
   if ( !Image_set_extended_data( self, profile))
      my-> set_data( self, pget_sv( data));
   opt_assign( optPreserveType, pget_B( preserveType));
   var->palSize = (1 << (var->type & imBPP)) & 0x1ff;
   if (!( var->type & imGrayScale))
      apc_img_read_palette( var->palette, pget_sv( palette));
   {
      Point set;
      prima_read_point( pget_sv( resolution), (int*)&set, 2, "RTC0109: Array panic on 'resolution'");
      my-> set_resolution( self, set);
   }
   if ( var->type & imGrayScale) switch ( var->type & imBPP)
   {
   case imbpp1:
      memcpy( var->palette, stdmono_palette, sizeof( stdmono_palette));
      break;
   case imbpp4:
      memcpy( var->palette, std16gray_palette, sizeof( std16gray_palette));
      break;
   case imbpp8:
      memcpy( var->palette, std256gray_palette, sizeof( std256gray_palette));
      break;
   }
   apc_image_create( self);
   my->update_change( self);
}


void
Image_reset( Handle self, int type, SV * palette)
{
   Byte * newData = nil;
   if ( var->stage > csFrozen) return;
   if (!( type & imGrayScale)) {
      switch ( type) {
      case im16:
         if (( var-> type & imBPP) < im16) {
            int c = 1 << ( var-> type & imBPP);
            memcpy( var-> palette + c, cubic_palette16 + c, sizeof( RGBColor) * ( 16 - c));
         }
         break;
      case im256:
         if (( var-> type & imBPP) < im256) {
            int c = 1 << ( var-> type & imBPP);
            memcpy( var-> palette + c, cubic_palette + c, sizeof( RGBColor) * ( 256 - c));
         }
         break;
      }
      apc_img_read_palette( var->palette, palette);
   }
   if ( var->type == imByte && type == im256)
   {
      var->type = type;
      return;
   }
   var->lineSize = (( var->w * ( type & imBPP) + 31) / 32) * 4;
   var->dataSize = ( var->lineSize) * var->h;
   var->palSize = (1 << ( type & imBPP)) & 0x1ff;
   if ( var->dataSize > 0) {
      newData = allocb( var-> dataSize);
      if ( newData == nil) 
         croak("Image::reset: cannot allocate %d bytes", var-> dataSize);
      ic_type_convert( self, newData, var->palette, type);
   }
   free( var->data);
   var->data = newData;
   var->type = type;
   my->update_change( self);
}

void
Image_stretch( Handle self, int width, int height)
{
   Byte * newData = nil;
   int lineSize;
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
   lineSize = (( abs( width) * ( var->type & imBPP) + 31) / 32) * 4;
   newData = allocb( lineSize * abs( height));
   if ( newData == nil) 
         croak("Image::stretch: cannot allocate %d bytes", lineSize * abs( height));
   if ( var-> data)
      ic_stretch( var-> type, var-> data, var-> w, var-> h, 
                  newData, width, height, 
                  is_opt( optHScaling), is_opt( optVScaling));
   free( var->data);
   var->data = newData;
   var->lineSize = lineSize;
   var->dataSize = lineSize * abs( height);
   var->w = abs( width);
   var->h = abs( height);
   my->update_change( self);
}


void
Image_set( Handle self, HV * profile)
{
   if ( pexist( conversion))
   {
      my-> set_conversion( self, pget_i( conversion));
      pdelete( conversion);
   }
   if ( pexist( hScaling))
   {
      my->set_hScaling( self, pget_B( hScaling));
      pdelete( hScaling);
   }
   if ( pexist( vScaling))
   {
      my->set_vScaling( self, pget_B( vScaling));
      pdelete( vScaling);
   }

   if ( Image_set_extended_data( self, profile))
      pdelete( data);

   if ( pexist( type))
   {
      int newType = pget_i( type);
      if ( !itype_supported( newType))
         warn("RTC0100: Invalid image type requested (%08x) in Image::set_type", newType);
      else 
         if ( !opt_InPaint)
            my-> reset( self, newType, pexist( palette) ? pget_sv( palette) : my->get_palette( self));
      pdelete( palette);
      pdelete( type);
   }

   if ( pexist( resolution))
   {
      Point set;
      prima_read_point( pget_sv( resolution), (int*)&set, 2, "RTC0109: Array panic on 'resolution'");
      my-> set_resolution( self, set);
      pdelete( resolution);
   }

   inherited set ( self, profile);
}


void
Image_done( Handle self)
{
   apc_image_destroy( self);
   my->make_empty( self);
   var->data = nil;
   var->palette = nil;
   inherited done( self);
}

void
Image_make_empty( Handle self)
{
   free( var->data);
   free( var->palette);
   var->w = 0;
   var->h = 0;
   var->type     = 0;
   var->palSize  = 0;
   var->lineSize = 0;
   var->dataSize = 0;
   var->data     = nil;
   my->update_change( self);
}

Bool
Image_hScaling( Handle self, Bool set, Bool scaling)
{
   if ( !set)
      return is_opt( optHScaling);
   opt_assign( optHScaling, scaling);
   return false;
}

Bool
Image_vScaling( Handle self, Bool set, Bool scaling)
{
   if ( !set)
      return is_opt( optVScaling);
   opt_assign( optVScaling, scaling);
   return false;
}

Point
Image_resolution( Handle self, Bool set, Point resolution)
{
   if ( !set)
      return var-> resolution;
   if ( resolution. x <= 0 || resolution. y <= 0)
      resolution = apc_gp_get_resolution( application);
   var-> resolution = resolution;
   return resolution;
}

Point
Image_size( Handle self, Bool set, Point size)
{
   if ( !set)
      return inherited size( self, set, size);
   Image_stretch( self, size.x, size.y);
   return size;
}

SV *
Image_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_image_get_handle( self));
   return newSVpv( buf, 0);
}

SV *
Image_data( Handle self, Bool set, SV * svdata)
{
   void *data;
   STRLEN dataSize;

   if ( var->stage > csFrozen) return nilSV;

   if ( !set)
      return newSVpvn(( char *) var-> data, var-> dataSize);

   data = SvPV( svdata, dataSize);
   if ( is_opt( optInDraw) || dataSize <= 0) return nilSV;

   memcpy( var->data, data, dataSize > var->dataSize ? var->dataSize : dataSize);
   my-> update_change( self);
   return nilSV;
}

/*
  Routine sets image data almost as Image::set_data, but taking into
  account 'lineSize' and 'type', fields. To be called from bunch routines,
  line ::init or ::set. Returns true if relevant fields were found and
  data extracted and set, and false if user data should be set throught ::set_data.
  Image itself may undergo conversion during the routine; in that case 'palette'
  property may be used also. All these fields, if used, or meant to be used but
  erroneously set, will be deleted regardless of routine success. 
*/
Bool
Image_set_extended_data( Handle self, HV * profile)
{
   void *data, *proc;
   STRLEN dataSize;
   int lineSize = 0, newType = -1, fixType, oldType = -1;
   Bool pexistType, pexistLine, supp;

   if ( !pexist( data)) {
      if ( pexist( lineSize)) {
         warn( "Image: lineSize supplied without data property.");
         pdelete( lineSize);
      }
      return false;
   }   
   
   data = SvPV( pget_sv( data), dataSize);

   // parameters check
   pexistType = pexist( type) && ( newType = pget_i( type)) != var-> type;
   pexistLine = pexist( lineSize) && ( lineSize = pget_i( lineSize)) != var-> lineSize;

   pdelete( lineSize);
   pdelete( type);
   
   if ( !pexistLine && !pexistType) return false;

   if ( is_opt( optInDraw) || dataSize <= 0) 
      goto GOOD_RETURN;

   // determine line size, if any
   if ( pexistLine) {
      if ( lineSize <= 0) {
         warn( "Image::set_data: invalid lineSize:%d passed", lineSize);
         goto GOOD_RETURN;
      }   
      if ( !pexistType) { // plain repadding
         ibc_repad(( Byte*) data, var-> data, lineSize, var-> lineSize, dataSize, var-> dataSize, 0, 0, nil);
         my-> update_change( self);
         goto GOOD_RETURN;
      }   
   }

   // pre-fetch auto conversion, if set in same clause
   if ( pexist( preserveType))
       opt_assign( optPreserveType, pget_B( preserveType));
   if ( is_opt( optPreserveType))
      oldType = var-> type;

    /* getting closest type */
   if (( supp = itype_supported( newType))) {
      fixType = newType;
      proc    = nil;
   } else if ( !itype_importable( newType, &fixType, &proc, nil)) {
      warn( "Image::set_data: invalid image type %08x", newType);
      goto GOOD_RETURN;
   }   
      
   /* fixing image and maybe palette - for known type it's same code as in ::set, */
   /* but here's no sense calling it, just doing what we need. */
   if ( fixType != var-> type) { 
      my-> reset( self, fixType, pexist( palette) ? pget_sv( palette) : my-> get_palette( self));
      pdelete( palette);
   }   

    /* copying user data */
   if ( supp && lineSize == 0) 
       /* same code as in ::set_data */
      memcpy( var->data, data, dataSize > var->dataSize ? var->dataSize : dataSize);
   else {
      // if no explicit lineSize set, assuming x4 padding 
      if ( lineSize == 0)
         lineSize = (( var-> w * ( newType & imBPP) + 31) / 32) * 4;
      // copying using repadding routine
      ibc_repad(( Byte*) data, var-> data, lineSize, var-> lineSize, dataSize, var-> dataSize, 
                 ( newType & imBPP) / 8, ( var-> type & imBPP) / 8, proc
               );
   }   
   my-> update_change( self);
   // if wanted to keep original type, restoring
   if ( is_opt( optPreserveType))
      my-> set_type( self, oldType);
   
GOOD_RETURN:   
   pdelete(data);
   return true;
}   

XS( Image_load_FROMPERL) 
{
   dXSARGS;
   Handle self;
   HV *profile;
   char *fn;
   PList ret;
   Bool err = false;
   char error[256];

   if (( items < 2) || (( items % 2) != 0))
      croak("Invalid usage of Prima::Image::load");
   
   self = gimme_the_mate( ST( 0));
   fn   = ( char *) SvPV( ST( 1), na);
   profile = parse_hv( ax, sp, items, mark, 2, "Image::load");
   if ( !pexist( className)) 
      pset_c( className, self ? my-> className : ( char*) SvPV( ST( 0), na));
   ret = apc_img_load( self, fn, profile, error);
   sv_free(( SV *) profile);
   SPAGAIN;
   SP -= items;
   if ( ret) {
      int i;
      for ( i = 0; i < ret-> count; i++) {
         PAnyObject o = ( PAnyObject) ret-> items[i];
         if ( o && o-> mate && o-> mate != nilSV) {
            XPUSHs( sv_mortalcopy( o-> mate));
            if (( Handle) o != self)
              --SvREFCNT( SvRV( o-> mate));
         } else {
            XPUSHs( &sv_undef);    
            err = true;
         }   
      }
      plist_destroy( ret);
   } else {
      XPUSHs( &sv_undef);   
      err = true;
   }   

   /* This code breaks exception propagation chain
      since it uses $@ for its own needs  */
   if ( err)
      sv_setpv( GvSV( errgv), error);
   else
      sv_setsv( GvSV( errgv), nilSV);

   PUTBACK;
   return;
}   

PList
Image_load_REDEFINED( SV * who, char *filename, HV * profile)
{
   return nil;
}

PList
Image_load( SV * who, char *filename, HV * profile)
{
   PList ret;
   Handle self = gimme_the_mate( who);
   char error[ 256];
   if ( !pexist( className)) 
      pset_c( className, self ? my-> className : ( char*) SvPV( who, na));
   ret = apc_img_load( self, filename, profile, error);
   return ret;
}


XS( Image_save_FROMPERL) 
{
   dXSARGS;
   Handle self;
   HV *profile;
   char *fn;
   int ret;
   char error[256];

   if (( items < 2) || (( items % 2) != 0))
      croak("Invalid usage of Prima::Image::save");
   
   self = gimme_the_mate( ST( 0));
   fn   = ( char *) SvPV( ST( 1), na);
   profile = parse_hv( ax, sp, items, mark, 2, "Image::save");
   ret = apc_img_save( self, fn, profile, error);
   sv_free(( SV *) profile);
   SPAGAIN;
   SP -= items;
   XPUSHs( sv_2mortal( newSViv(( ret > 0) ? ret : -ret)));
   
   /* This code breaks exception propagation chain
      since it uses $@ for its own needs  */
   if ( ret <= 0)
      sv_setpv( GvSV( errgv), error);
   else
      sv_setsv( GvSV( errgv), nilSV);
   PUTBACK;
   return;
}   

int
Image_save_REDEFINED( SV * who, char *filename, HV * profile)
{
   return 0;
}

int
Image_save( SV * who, char *filename, HV * profile)
{
   Handle self = gimme_the_mate( who);
   char error[ 256];
   if ( !pexist( className)) 
      pset_c( className, self ? my-> className : ( char*) SvPV( who, na));
   return apc_img_save( self, filename, profile, error);
}

int
Image_type( Handle self, Bool set, int type)
{
   HV * profile;
   if ( !set)
      return var->type;
   profile = newHV();
   pset_i( type, type);
   my-> set( self, profile);
   sv_free(( SV *) profile);
   return nilHandle;
}

int
Image_get_bpp( Handle self)
{
   return var->type & imBPP;
}


Bool
Image_begin_paint( Handle self)
{
   Bool ok;
   if ( !inherited begin_paint( self))
      return false;
   if ( !( ok = apc_image_begin_paint( self)))
      inherited end_paint( self);
   return ok;
}

Bool
Image_begin_paint_info( Handle self)
{
   Bool ok;
   if ( is_opt( optInDraw))     return true;
   if ( !inherited begin_paint_info( self))
      return false;
   if ( !( ok = apc_image_begin_paint_info( self)))
      inherited end_paint_info( self);
   return ok;
}


void
Image_end_paint( Handle self)
{
   int oldType = var->type;
   if ( !is_opt( optInDraw)) return;
   apc_image_end_paint( self);
   inherited end_paint( self);
   if ( is_opt( optPreserveType) && var->type != oldType) {
      my->reset( self, oldType, nilSV);
   } else {
      switch( var->type)
      {
         case imbpp1:
            if ( memcmp( var->palette, stdmono_palette, sizeof( stdmono_palette)) == 0)
               var->type |= imGrayScale;
            break;
         case imbpp4:
            if ( memcmp( var->palette, std16gray_palette, sizeof( std16gray_palette)) == 0)
               var->type |= imGrayScale;
            break;
         case imbpp8:
            if ( memcmp( var->palette, std256gray_palette, sizeof( std256gray_palette)) == 0)
               var->type |= imGrayScale;
            break;
      }
      my->update_change( self);
   }
}

void
Image_end_paint_info( Handle self)
{
   if ( !is_opt( optInDrawInfo)) return;
   apc_image_end_paint_info( self);
   inherited end_paint_info( self);
}

void
Image_update_change( Handle self)
{
   if ( var-> stage <= csNormal) apc_image_update_change( self);
   var->statsCache = 0;
}

double
Image_stats( Handle self, Bool set, int index, double value)
{
   if ( index < 0 || index > isMaxIndex) return NAN;
   if ( set) {
      var-> stats[ index] = value;
      var-> statsCache |= 1 << index;
      return 0;
   } else {
#define gather_stats(TYP) if ( var->data) {                \
         TYP *src = (TYP*)var->data, *stop, *s;            \
         maxv = minv = *src;                              \
         for ( y = 0; y < var->h; y++) {                   \
            s = src;  stop = s + var->w;                   \
            while (s != stop) {                           \
               v = (double)*s;                            \
               sum += v;                                  \
               sum2 += v*v;                               \
               if ( minv > v) minv = v;                   \
               if ( maxv < v) maxv = v;                   \
               s++;                                       \
            }                                             \
            src = (TYP*)(((Byte *)src) + var->lineSize);   \
         }                                                \
      }
      int y;
      double sum = 0.0, sum2 = 0.0, minv = 0.0, maxv = 0.0, v;

      if ( var->statsCache & ( 1 << index)) return var->stats[ index];
      /* calculate image stats */
      switch ( var->type) {
         case imByte:    gather_stats(uint8_t);break;
         case imShort:   gather_stats(int16_t);  break;
         case imLong:    gather_stats(int32_t);   break;
         case imFloat:   gather_stats(float);  break;
         case imDouble:  gather_stats(double); break;
         default:        return NAN;
      }
      if ( var->w * var->h > 0)
      {
         var->stats[ isSum] = sum;
         var->stats[ isSum2] = sum2;
         sum /= var->w * var->h;
         sum2 /= var->w * var->h;
         sum2 = sum2 - sum*sum;
         var->stats[ isMean] = sum;
         var->stats[ isVariance] = sum2;
         var->stats[ isStdDev] = sqrt(sum2);
         var->stats[ isRangeLo] = minv;
         var->stats[ isRangeHi] = maxv;
      } else {
         for ( y = 0; y <= isMaxIndex; y++) var->stats[ y] = 0;
      }
      var->statsCache = (1 << (isMaxIndex + 1)) - 1;
   }
   return var->stats[ index];
}

void
Image_resample( Handle self, double srcLo, double srcHi, double dstLo, double dstHi)
{
#define RSPARMS self, var->data, var->type, srcLo, srcHi, dstLo, dstHi
   switch ( var->type)
   {
      case imByte:   rs_Byte_Byte     ( RSPARMS); break;
      case imShort:  rs_Short_Short   ( RSPARMS); break;
      case imLong:   rs_Long_Long     ( RSPARMS); break;
      case imFloat:  rs_float_float   ( RSPARMS); break;
      case imDouble: rs_double_double ( RSPARMS); break;
      default: return;
   }
   my->update_change( self);
}

SV *
Image_palette( Handle self, Bool set, SV * palette)
{
   if ( var->stage > csFrozen) return nilSV;
   if ( set) {
      if ( var->type & imGrayScale) return nilSV;
      if ( !var->palette)           return nilSV;
      if ( !apc_img_read_palette( var->palette, palette))
         warn("RTC0107: Invalid array reference passed to Image::palette");
      my-> update_change( self);
   } else {
      int i;
      AV * av = newAV();
      int colors = ( 1 << ( var->type & imBPP)) & 0x1ff;
      Byte * pal = ( Byte*) var->palette;
      if (( var->type & imGrayScale) && (( var->type & imBPP) > imbpp8)) colors = 256;
      for ( i = 0; i < colors*3; i++) av_push( av, newSViv( pal[ i]));
      return newRV_noinc(( SV *) av);
   }
   return nilSV;
}

int
Image_conversion( Handle self, Bool set, int conversion)
{
   if ( !set)
      return var-> conversion;
   return var-> conversion = conversion;
}

void
Image_create_empty( Handle self, int width, int height, int type)
{
   free( var->data);
   var->w = width;
   var->h = height;
   var->type     = type;
   var->lineSize = (( var->w * ( var->type & imBPP) + 31) / 32) * 4;
   var->dataSize = var->lineSize * var->h;
   var->palSize  = (1 << (var->type & imBPP)) & 0x1ff;
   if ( var->dataSize > 0)
   {
      var->data = allocb( var->dataSize);
      if ( var-> data == nil) 
         croak("Image::create_empty: cannot allocate %d bytes", var-> dataSize);
      memset( var->data, 0, var->dataSize);
   } else
      var->data = nil;
   if ( var->type & imGrayScale) switch ( var->type & imBPP)
   {
   case imbpp1:
      memcpy( var->palette, stdmono_palette, sizeof( stdmono_palette));
      break;
   case imbpp4:
      memcpy( var->palette, std16gray_palette, sizeof( std16gray_palette));
      break;
   case imbpp8:
      memcpy( var->palette, std256gray_palette, sizeof( std256gray_palette));
      break;
   }
}

Bool
Image_preserveType( Handle self, Bool set, Bool preserveType)
{
   if ( !set)
      return is_opt( optPreserveType);
   opt_assign( optPreserveType, preserveType);
   return false;
}

Color
Image_pixel( Handle self, Bool set, int x, int y, Color color)
{
#define BGRto32(pal) ((var->palette[pal].r<<16) | (var->palette[pal].g<<8) | (var->palette[pal].b))
   if (!set) {
      if ( opt_InPaint)
         return inherited pixel(self,false,x,y,color);
      if ((x>=var->w) || (x<0) || (y>=var->h) || (y<0))
         return clInvalid;
      switch (var->type & imBPP) {
      case imbpp1:
         {
            Byte p=var->data[var->lineSize*y+(x>>3)];
            p=(p >> (7-(x & 7))) & 1;
            return ((var->type & imGrayScale) ? (p ? 255 : 0) : BGRto32(p));
         }
      case imbpp4:
         {
            Byte p=var->data[var->lineSize*y+(x>>1)];
            p=(x&1) ? p & 0x0f : p>>4;
            return ((var->type & imGrayScale) ? (p*255L)/15 : BGRto32(p));
         }
      case imbpp8:
         {
            Byte p=var->data[var->lineSize*y+x];
            return ((var->type & imGrayScale) ? p :  BGRto32(p));
         }
      case imbpp16:
         {
            short p=*(short*)(var->data + (var->lineSize*y+x*2));
            return p;
         }
      case imbpp24:
         {
            RGBColor p=*(PRGBColor)(var->data + (var->lineSize*y+x*3));
            return (p.r<<16) | (p.g<<8) | p.b;
         }
      case imbpp32:
         {
            long p;
            if (var->type & imRealNumber) {
               float pf=*(float*)(var->data + (var->lineSize*y+x*4));
               double rangeLo = my-> stats( self, false, isRangeLo, 0);
               p=((pf - rangeLo)*LONG_MAX)/(var->stats[isRangeHi] - rangeLo);
            }
            else {
               p=*(long*)(var->data + (var->lineSize*y+x*4));
            }
            return p;
         }
      case imbpp64:
         {
            double pd=*(double*)(var->data + (var->lineSize*y+x*8));
            double rangeLo;
            if ((var->type & imComplexNumber) || (var->type & imTrigComplexNumber)) {
               return 0;
            }
            rangeLo = my-> stats( self, false, isRangeLo, 0);
            return ((pd - rangeLo)/(var->stats[isRangeHi] - rangeLo))*LONG_MAX;
         }
      default:
         return 0;
      }
#undef BGRto32
   } else {
      RGBColor rgb;
#define LONGtoBGR(lv,clr)   ((clr).b=(lv)&0xff,(clr).g=((lv)>>8)&0xff,(clr).r=((lv)>>16)&0xff,(clr))
      if ( is_opt( optInDraw)) {
         return inherited pixel(self,true,x,y,color);
      }
      if ((x>=var->w) || (x<0) || (y>=var->h) || (y<0)) {
         return color;
      }
      switch (var->type & imBPP) {
      case imbpp1  :
         {
            int x1=7-(x&7);
            Byte p=(((var->type & imGrayScale) ? color/255 : cm_nearest_color(LONGtoBGR(color,rgb),var->palSize,var->palette)) & 1);
            Byte *pd=var->data+(var->lineSize*y+(x>>3));
            *pd&=~(1 << x1);
            *pd|=(p << x1);
         }
         break;
      case imbpp4  :
         {
            Byte p=((var->type & imGrayScale) ? (color*15)/255 : cm_nearest_color(LONGtoBGR(color,rgb),var->palSize,var->palette));
            Byte *pd=var->data+(var->lineSize*y+(x>>1));
            if (x&1) {
               *pd&=0xf0;
            }
            else {
               p<<=4;
               *pd&=0x0f;
            }
            *pd|=p;
         }
         break;
      case imbpp8:
         {
            if (var->type & imGrayScale) {
               var->data[(var->lineSize)*y+x]=color;
            }
            else {
               var->data[(var->lineSize)*y+x]=cm_nearest_color(LONGtoBGR(color,rgb),(var->palSize),(var->palette));
            }
         }
         break;
      case imbpp16 :
         {
            *(short*)(var->data+(var->lineSize*y+(x<<1)))=color;
         }
         break;
      case imbpp24 :
         {
            LONGtoBGR(color,rgb);
            memcpy((var->data + (var->lineSize*y+x*3)),&rgb,sizeof(RGBColor));
         }
         break;
      case imbpp32 :
         {
            if (var->type & imRealNumber) {
               double rangeLo = my-> stats( self, false, isRangeLo, 0);
               *(float*)(var->data+(var->lineSize*y+(x<<2)))=(((float)color)/(LONG_MAX))*(var->stats[isRangeHi]-rangeLo)+rangeLo;
            }
            else {
               *(long*)(var->data+(var->lineSize*y+(x<<2)))=color;
            }
         }
         break;
      case imbpp64 :
         if (var->type & imRealNumber) {
            double rangeLo = my-> stats( self, false, isRangeLo, 0);
            *(double*)(var->data+(var->lineSize*y+(x<<2)))=(((double)color)/(LONG_MAX))*(var->stats[isRangeHi]-rangeLo)+rangeLo;
         }
         break;
      default:
         return color;
      }
      my->update_change( self);
#undef LONGtoBGR
      return color;
   }
}

Handle
Image_bitmap( Handle self)
{
   Handle h;
   HV * profile = newHV();

   pset_H( owner,        var->owner);
   pset_i( width,        var->w);
   pset_i( height,       var->h);
   pset_sv( palette,     my->get_palette( self));
   pset_i( monochrome,   (var-> type & imBPP) == 1);
   h = Object_create( "Prima::DeviceBitmap", profile);
   sv_free(( SV *) profile);
   CDrawable( h)-> put_image( h, 0, 0, self);
   --SvREFCNT( SvRV( PDrawable( h)-> mate));
   return h;
}


Handle
Image_dup( Handle self)
{
   Handle h;
   PImage i;
   HV * profile = newHV();

   pset_H( owner,        var->owner);
   pset_i( width,        var->w);
   pset_i( height,       var->h);
   pset_i( type,         var->type);
   pset_i( conversion,   var->conversion);
   pset_i( hScaling,     is_opt( optHScaling));
   pset_i( vScaling,     is_opt( optVScaling));
   pset_i( preserveType, is_opt( optPreserveType));

   h = Object_create( var->self-> className, profile);
   sv_free(( SV *) profile);
   i = ( PImage) h;
   memcpy( i-> palette, var->palette, 768);
   if ( i-> type != var->type)
      croak("RTC0108: Image::dup consistency failed");
   else
      memcpy( i-> data, var->data, var->dataSize);
   memcpy( i-> stats, var->stats, sizeof( var->stats));
   i-> statsCache = var->statsCache;

   if ( hv_exists(( HV*)SvRV( var-> mate), "extras", 6)) {
      SV ** sv = hv_fetch(( HV*)SvRV( var-> mate), "extras", 6, 0);
      if ( sv && SvOK( *sv) && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVHV)
         hv_store(( HV*)SvRV( i-> mate), "extras", 6, newSVsv( *sv), 0);
   }
   
   --SvREFCNT( SvRV( i-> mate));
   return h;
}

Handle
Image_extract( Handle self, int x, int y, int width, int height)
{
   Handle h;
   PImage i;
   HV * profile;
   unsigned char * data = var->data;
   int ls = var->lineSize;

   if ( var->w == 0 || var->h == 0) return my->dup( self);
   if ( x < 0) x = 0;
   if ( y < 0) y = 0;
   if ( x >= var->w) x = var->w - 1;
   if ( y >= var->h) y = var->h - 1;
   if ( width  + x > var->w) width  = var->w - x;
   if ( height + y > var->h) height = var->h - y;
   if ( width <= 0 || height <= 0) return my->dup( self);

   profile = newHV();
   pset_H( owner,        var->owner);
   pset_i( width,        width);
   pset_i( height,       height);
   pset_i( type,         var->type);
   pset_i( conversion,   var->conversion);
   pset_i( hScaling,     is_opt( optHScaling));
   pset_i( vScaling,     is_opt( optVScaling));
   pset_i( preserveType, is_opt( optPreserveType));

   h = Object_create( var->self-> className, profile);
   sv_free(( SV *) profile);

   i = ( PImage) h;
   memcpy( i-> palette, var->palette, 768);
   if (( var->type & imBPP) >= 8) {
      int pixelSize = ( var->type & imBPP) / 8;
      while ( height > 0) {
         height--;
         memcpy( i-> data + height * i-> lineSize,
                 data + ( y + height) * ls + pixelSize * x,
                 pixelSize * width);
      }
   } else if (( var->type & imBPP) == 4) {
      while ( height > 0) {
         height--;
         bc_nibble_copy( data + ( y + height) * ls, i-> data + height * i-> lineSize, x, width);
      }
   } else if (( var->type & imBPP) == 1) {
      while ( height > 0) {
         height--;
         bc_mono_copy( data + ( y + height) * ls, i-> data + height * i-> lineSize, x, width);
      }
   }
   --SvREFCNT( SvRV( i-> mate));
   return h;
}

/*
  divide the pixels, by whether they match color or not on two 
  groups, F and B. Both are converted correspondingly to the settings
  of color/backColor and rop/rop2. Possible variations:
  rop == rop::NoOper,    pixel value remains ths same
  rop == rop::CopyPut,   use the color value
  rop == rop::Blackness, use black pixel
  rop == rop::Whiteness, use white pixel
  rop == rop::AndPut   , result is dest & color value
  etc...
*/   

void
Image_map( Handle self, Color color)
{
   Byte * d;
   RGBColor c;   
   int   type = var-> type, height = var-> h, i, ls;
   int   rop[2]; 
   RGBColor r[2];

   if ( var-> data == nil) return;

   rop[0] = my-> get_rop( self);
   rop[1] = my-> get_rop2( self);
   if ( rop[0] == ropNoOper && rop[1] == ropNoOper) return;

   for ( i = 0; i < 2; i++) {
      int not = 0;
      
      switch( rop[i]) {
      case ropBlackness:
         r[i]. r = r[i]. g = r[i]. b = 0;
         rop[i] = ropCopyPut;
         break;
      case ropWhiteness:
         r[i]. r = r[i]. g = r[i]. b = 0xff;
         rop[i] = ropCopyPut;
         break;
      case ropNoOper:
         break;   
      default: {   
         Color c = my-> get_color( self);
         r[i]. r = ( c >> 16) & 0xff;
         r[i]. g = ( c >> 8) & 0xff;
         r[i]. b = c & 0xff;
      }} 
      
      switch ( rop[i]) {
      case ropNotPut:
          rop[i] = ropCopyPut; not = 1; break;
      case ropNotSrcXor:
          rop[i] = ropXorPut; not = 1; break;    
      case ropNotSrcAnd:
          rop[i] = ropAndPut; not = 1; break;    
      case ropNotSrcOr:
          rop[i] = ropOrPut; not = 1; break;    
      }  
      
      if ( not) {
         r[i]. r = ~ r[i]. r;
         r[i]. g = ~ r[i]. g;
         r[i]. b = ~ r[i]. b;
      }
   }         

   c. r = ( color >> 16) & 0xff;
   c. g = ( color >> 8) & 0xff;
   c. b = color & 0xff;
   
   my-> set_type( self, imRGB);

   d = ( Byte * ) var-> data;
   ls = var-> lineSize;
   
   while ( height--) {
      PRGBColor data = ( PRGBColor) d;
      for ( i = 0; i < var-> w; i++) {
         int z = ( data-> r == c.r && data-> g == c.g && data-> b == c.b) ? 0 : 1;
         switch( rop[z]) {
         case ropAndPut:     
            data-> r &= r[z]. r; data-> g &= r[z]. g; data-> b &= r[z]. b; break;
         case ropXorPut:     
            data-> r ^= r[z]. r; data-> g ^= r[z]. g; data-> b ^= r[z]. b; break;
         case ropOrPut:      
            data-> r |= r[z]. r; data-> g |= r[z]. g; data-> b |= r[z]. b; break;
         case ropNotDestAnd: 
            data-> r = ( ~data-> r) & r[z].r; data-> g = ( ~data-> g) & r[z].g; data-> b = ( ~data-> b) & r[z].b; break;
         case ropNotDestOr:  
            data-> r = ( ~data-> r) | r[z].r; data-> g = ( ~data-> g) | r[z].g; data-> b = ( ~data-> b) | r[z].b; break;
         case ropNotDestXor: 
            data-> r = ( ~data-> r) ^ r[z].r; data-> g = ( ~data-> g) ^ r[z].g; data-> b = ( ~data-> b) ^ r[z].b; break;
         case ropNotAnd:     
            data-> r = ~(data-> r & r[z].r); data-> g = ~(data-> g & r[z].g); data-> b = ~(data-> b & r[z].b); break;
         case ropNotOr:      
            data-> r = ~(data-> r | r[z].r); data-> g = ~(data-> g | r[z].g); data-> b = ~(data-> b | r[z].b); break;
         case ropNotXor:     
            data-> r = ~(data-> r ^ r[z].r); data-> g = ~(data-> g ^ r[z].g); data-> b = ~(data-> b ^ r[z].b); break;
         case ropNoOper:     
            break;   
         case ropInvert:     
            data-> r = ~r[z]. r; data-> g = ~r[z]. g; data-> b = ~r[z]. b; break;
         default:            
            data-> r = r[z]. r; data-> g = r[z]. g; data-> b = r[z]. b;
         }      
         data++;
      }   
      d += ls;
   }   

   if ( is_opt( optPreserveType) && var->type != type) 
      my-> set_type( self, type);
   else
      my-> update_change( self);
}   

SV * 
Image_codecs( SV * dummy)
{
   int i;
   AV * av = newAV();
   PList p = plist_create( 16, 16);
   apc_img_codecs( p);  
   for ( i = 0; i < p-> count; i++) {
      PImgCodec c = ( PImgCodec ) p-> items[ i];
      av_push( av, newRV_noinc(( SV *) apc_img_info2hash( c))); 
   }  
   plist_destroy( p);
   return newRV_noinc(( SV *) av); 
}

#ifdef __cplusplus
}
#endif
