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
 */

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
#include "img_api.h"
#include "img_conv.h"
#include <Image.inc>
#include "Clipboard.h"

#undef  my
#define inherited CDrawable->
#define my  ((( PImage) self)-> self)
#define var (( PImage) self)

static int Image_read_palette( Handle self, PRGBColor palBuf, SV * palette);

void
Image_init( Handle self, HV * profile)
{
   int xy[2];

   inherited init( self, profile);
   var->w = pget_i( width);
   var->h = pget_i( height);
   var->conversion = pget_i( conversion);
   opt_assign( optHScaling, pget_B( hScaling));
   opt_assign( optVScaling, pget_B( vScaling));
   var->type = pget_i( type);
   var->lineSize = (( var->w * ( var->type & imBPP) + 31) / 32) * 4;
   var->dataSize = ( var->lineSize) * var->h;
   var->data = ( var->dataSize > 0) ? malloc( var->dataSize) : nil;
   free( var->palette);
   var->palette = malloc( 0x100 * sizeof( RGBColor));
   opt_assign( optPreserveType, pget_B( preserveType));
   var->palSize = (1 << (var->type & imBPP)) & 0x1ff;
   if (!( var->type & imGrayScale))
      Image_read_palette( self, var->palette, pget_sv( palette));
   my->set_data( self, pget_sv( data));
   prima_read_point( pget_sv( resolution), xy, 2, "RTC0109: Array panic on 'resolution'");
   my-> set_resolution( self, xy[0], xy[1]);
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

static int
Image_read_palette( Handle self, PRGBColor palBuf, SV * palette)
{
   AV * av;
   int i, count;
   Byte buf[768];

   if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV))
      return 0;
   av = (AV *) SvRV( palette);
   count = av_len( av) + 1;
   if ( count > 768) count = 768;
   count -= count % 3;

   for ( i = 0; i < count; i++)
   {
      SV **itemHolder = av_fetch( av, i, 0);
      if ( itemHolder == nil) return 0;
      buf[ i] = SvIV( *itemHolder);
   }
   memcpy( palBuf, buf, count);
   return count/3;
}

void
Image_reset( Handle self, int type, SV * palette)
{
   Byte * newData = nil;
   if ( var->stage > csNormal) return;
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
      Image_read_palette( self, var->palette, palette);
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
      newData = malloc( var-> dataSize);
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
   if ( var->stage > csNormal) return;
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
   newData = malloc( lineSize * abs( height));
   if ( var-> data)
      ic_stretch( self, newData, width, height, is_opt( optHScaling), is_opt( optVScaling));
   free( var->data);
   var->data = newData;
   var->lineSize = lineSize;
   var->dataSize = lineSize * abs( height);
   var->w = abs( width);
   var->h = abs( height);
   my->update_change( self);
}

static int imTypes[] = {
   imMono, imBW, im16, im256, imRGB, imByte, imShort, imLong, imFloat, -1
};

void
Image_set( Handle self, HV * profile)
{
   if ( pexist( conversion))
   {
      my->set_conversion( self, pget_i( conversion));
      pdelete( conversion);
   }
   if ( pexist( hScaling))
   {
      my->set_h_scaling( self, pget_B( hScaling));
      pdelete( hScaling);
   }
   if ( pexist( vScaling))
   {
      my->set_v_scaling( self, pget_B( vScaling));
      pdelete( vScaling);
   }

   if ( pexist( type))
   {
      int newType = pget_i( type);
      int i = 0;
      while( imTypes[i] != newType && imTypes[i] != -1) i++;
      if ( imTypes[i] == -1) {
         warn("RTC0100: Invalid image type requested (%04x) in Image::set_type", newType);
      } else {
         if ( !opt_InPaint)
            my->reset( self, newType, pexist( palette) ? pget_sv( palette) : my->get_palette( self));
      }
      pdelete( palette);
      pdelete( type);
   }
   if ( pexist( width) && pexist( height))
   {
      if ( !opt_InPaint)
         my->set_size( self, pget_i( width), pget_i( height));
      pdelete( width);
      pdelete( height);
   }
   if ( pexist( resolution))
   {
      Point set;
      prima_read_point( pget_sv( resolution), (int*)&set, 2, "RTC0109: Array panic on 'resolution'");
      my-> set_resolution( self, set.x, set.y);
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

char *
Image_get_status_string( SV *self)
{
   return (char *)apc_image_get_error_message( nil, 0);
}

Bool
Image_get_h_scaling( Handle self)
{
   return is_opt( optHScaling);
}

Bool
Image_get_v_scaling( Handle self)
{
   return is_opt( optVScaling);
}

void
Image_set_h_scaling( Handle self, Bool scaling)
{
   opt_assign( optHScaling, scaling);
}

void
Image_set_v_scaling( Handle self, Bool scaling)
{
   opt_assign( optVScaling, scaling);
}

void
Image_set_resolution( Handle self, int x, int y)
{
   if ( x <= 0 || y <= 0) {
      Point r = apc_gp_get_resolution( application);
      x = r. x;
      x = r. y;
   }
   var-> resolution. x = x;
   var-> resolution. y = y;
}

Point
Image_get_resolution( Handle self)
{
   return var-> resolution;
}

void
Image_set_size( Handle self, int width, int height)
{
   Image_stretch( self, width, height);
}

void
Image_set_width( Handle self, int width)
{
   my->set_size( self, width, var->h);
}

void
Image_set_height( Handle self, int height)
{
   my->set_size( self, var->w, height);
}

SV *
Image_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_image_get_handle( self));
   return newSVpv( buf, 0);
}

SV *
Image_get_data( Handle self)
{
   if ( var->stage > csNormal) return nilSV;
   return newSVpvn( var->data, var->dataSize);
}

void
Image_set_data( Handle self, SV * svdata)
{
   STRLEN dataSize;
   void *data = SvPV( svdata, dataSize);

   if ( var->stage > csNormal) return;
   if ( is_opt( optInDraw) || dataSize <= 0) return;

   memcpy( var->data, data, dataSize > var->dataSize ? var->dataSize : dataSize);
   my->update_change( self);
}

static void
fill_in_image_profile( HV *profile, PList propList, const char *calledFrom)
{
   SV *value;
   char *key;
   AV *ary;
   SV **elem;
   PImgProperty imgProp;
   HE *he;

   hv_iterinit( profile);
   for (;;)
   {
      if (( he = hv_iternext( profile)) == nil)
         break;
      value = HeVAL( he);
      key = HeKEY( he);
      /* keyLen = HeKLEN( he); */

      if ( SvROK( value)) {
         switch ( SvTYPE( SvRV( value))) {
             case SVt_PVAV:
                 {
                     int i, n;
                     STRLEN len;
                     int elemType = 0; /* Can be either PROPTYPE_BIN or PROPTYPE_PROP
                                          depending on the array content. */
                     /* Reference to an array */
                     ary = (AV*)SvRV( value);
                     n = av_len( ary) + 1;
                     /* The first step must be cheking of array content and choosing
                        elemt type carefully. A mixture of scalars and references isn't
                        allowed as well as the references may be hash references only. */
                     if ( n > 0) {
                         for ( i = 0; i < n; i++) {
                             if ( ! ( elem = av_fetch( ary, i, false))) {
                                 croak( "%s: cannot fetch element %d of ``%s''", calledFrom, i, key);
                             }
                             if ( ( elemType != 0)
                                  && ( ( SvROK( *elem) && ( elemType != PROPTYPE_PROP))
                                       || ( ! SvROK( *elem) && ( elemType == PROPTYPE_PROP)))) {
                                 croak( "%s: mixture of scalars and references in property ``%s'' is not allowed.\n", calledFrom, key);
                             }
                             if ( SvROK( *elem)) {
                                 if ( SvTYPE( SvRV( *elem)) != SVt_PVHV) {
                                     croak( "%s: array element %d of key %s is an array.", calledFrom, i, key);
                                 }
                                 if ( elemType == 0) {
                                     elemType = PROPTYPE_PROP;
                                 }
                             }
                             else if ( elemType != PROPTYPE_BIN) {
                                 elemType = PROPTYPE_BIN;
                             }
                         }
                     }
                     else {
                         /* An empty array received, assuming default value for type. */
                         elemType = PROPTYPE_BIN;
                     }
                     imgProp = img_push_property( propList, key, elemType | PROPTYPE_ARRAY, n > 0 ? n : 1);
                     for ( i = 0; i < n; i++) {
                         /* I do not check for success of fetching since it has been done
                            in type-choosing loop. */
                         elem = av_fetch( ary, i, false);
                         if ( elemType == PROPTYPE_BIN) {
                             char *buf = SvPV( *elem, len);
                             /* len must be increased by 1 to include termination zero. */
                             img_push_property_value( imgProp, len + 1, buf);
                         }
                         else {
                             HV *subprofile = ( HV *)SvRV( *elem);
                             img_push_property_value( imgProp, HvKEYS( subprofile));
                             fill_in_image_profile( subprofile, imgProp->val.pProperties + i, calledFrom);
                         }
                     }
                 }
                 break;
             case SVt_PVHV:
                 {
                     /* Reference to a hash. */
                     HV *subprofile = ( HV *) SvRV( value);
                     imgProp = img_push_property( propList, key, PROPTYPE_PROP, -1, HvKEYS( subprofile));
                     fill_in_image_profile( subprofile, &imgProp->val.Properties, calledFrom);
                 }
                 break;
             default:
                 croak( "Invalid usage of %s: illegal reference type for key ``%s''", calledFrom, key);
         }
      }
      else {
         /* scalar */
         STRLEN len;
         char *buf = SvPV( value, len);
         /* len must be increased by 1 to include termination zero. */
         img_push_property( propList, key, PROPTYPE_BIN, -1, len + 1, buf);
      }
   }
}

static void
add_image_profile( PImage img, HV *profile, PList imgInfo, const char *calledFrom)
{
   PImgInfo imageInfo;

   if ( ! ( imageInfo = img_info_create( HvKEYS( profile)))) {
       croak( "%s: can't create new image profile.\n", calledFrom);
   }
   fill_in_image_profile( profile, imageInfo->propList, calledFrom);
   if ( img) {
       PImgProperty imgProp;
       img_push_property( imageInfo->propList, "width", PROPTYPE_INT, 0, img->w);
       img_push_property( imageInfo->propList, "height", PROPTYPE_INT, 0, img->h);
       img_push_property( imageInfo->propList, "type", PROPTYPE_INT, 0, img->type);
       img_push_property( imageInfo->propList, "lineSize", PROPTYPE_INT, 0, img->lineSize);
       img_push_property( imageInfo->propList, "lineSize", PROPTYPE_INT, 0, img->lineSize);
       imgProp = img_push_property( imageInfo->propList, "data", PROPTYPE_BYTE | PROPTYPE_ARRAY, img->dataSize);
       memcpy( imgProp->val.pByte, img->data, img->dataSize);
       imgProp = img_push_property( imageInfo->propList, "palette", PROPTYPE_BYTE | PROPTYPE_ARRAY, img->palSize * 3);
       memcpy( imgProp->val.pByte, img->palette, img->palSize * 3);
       img_push_property( imageInfo->propList, "paletteSize", PROPTYPE_INT, 0, img->palSize);
   }
   list_add( imgInfo, ( Handle) imageInfo);
}

static void
add_image_property_to_profile( HV *profile, PImgProperty imgProp, const char *calledFrom)
{
    if ( ( imgProp->flags & PROPTYPE_ARRAY) != PROPTYPE_ARRAY) {
        switch ( imgProp->flags & PROPTYPE_MASK) {
            case PROPTYPE_STRING:
                hv_store( profile,
                          imgProp->name,
                          strlen( imgProp->name),
                          newSVpv( imgProp->val.String, 0),
                          0
                    );
                break;
            case PROPTYPE_BIN:
                hv_store( profile,
                          imgProp->name,
                          strlen( imgProp->name),
                          newSVpv( imgProp->val.Binary.data, imgProp->val.Binary.size),
                          0
                    );
                break;
            case PROPTYPE_DOUBLE:
                hv_store( profile,
                          imgProp->name,
                          strlen( imgProp->name),
                          newSVnv( imgProp->val.Double),
                          0
                    );
                break;
            case PROPTYPE_BYTE:
                hv_store( profile,
                          imgProp->name,
                          strlen( imgProp->name),
                          newSViv( imgProp->val.Byte),
                          0
                    );
                break;
            case PROPTYPE_PROP:
                {
                    int propIdx;
                    HV *hv = newHV();
                    for ( propIdx = 0; propIdx < imgProp->val.Properties.count; propIdx++) {
                        add_image_property_to_profile(
                                hv,
                                ( PImgProperty) list_at( &imgProp->val.Properties, propIdx),
                                calledFrom
                            );
                    }
                    hv_store( profile,
                              imgProp->name,
                              strlen( imgProp->name),
                              newRV_noinc( ( SV *) hv),
                              0
                        );
                }
                break;
            case PROPTYPE_INT:
            default:
                hv_store( profile,
                          imgProp->name,
                          strlen( imgProp->name),
                          newSViv( imgProp->val.Int),
                          0
                    );
                break;
        }
    }
    else {
        AV *av;
        int j;

        av = newAV();
        if ( ! av) {
            croak( "%s: can't allocate an array", calledFrom);
        }

        for ( j = 0; j < imgProp->size; j++) {
            SV *sv;
            switch( imgProp->flags & PROPTYPE_MASK) {
                case PROPTYPE_STRING:
                    sv = newSVpv( imgProp->val.pString[ j], 0);
                    break;
                case PROPTYPE_BIN:
                    sv = newSVpv( imgProp->val.pBinary[ j].data, imgProp->val.pBinary[ j].size);
                    break;
                case PROPTYPE_DOUBLE:
                    sv= newSVnv( imgProp->val.pDouble[ j]);
                    break;
                case PROPTYPE_BYTE:
                    sv = newSViv( imgProp->val.pByte[ j]);
                    break;
                case PROPTYPE_PROP:
                    {
                        int propIdx;
                        HV *hv = newHV();
                        for ( propIdx = 0; propIdx < imgProp->val.pProperties[ j].count; propIdx++) {
                            add_image_property_to_profile(
                                hv,
                                ( PImgProperty) list_at( imgProp->val.pProperties + j, propIdx),
                                calledFrom
                                );
                        }
                        sv = newRV_noinc( ( SV *) hv);
                    }
                    break;
                case PROPTYPE_INT:
                default:
                    sv = newSViv( imgProp->val.pInt[ j]);
                    break;
            }
            av_push( av, sv);
        }
        hv_store( profile, imgProp->name, strlen( imgProp->name), newRV_noinc( ( SV *) av), 0);
    }
}

XS( Image_save_FROMPERL) {
    dXSARGS;
   Bool result = false;
   Bool as_class = false;
   Handle self;
   char *class_name;
   PList info = nil;
   int i;
   HV *profile;

    if ( items >= 2) {
        SV *who = ST( 0);
        char *filename = SvPV( ST( 1), na);

        if ( ! ( self = gimme_the_mate( who))) {
            PVMT vmt;
            if ( ! SvPOK( who)) {
                croak( "Call Image->save() either as instance method or as class method");
            }
            class_name = SvPV( who, na);
            vmt = gimme_the_vmt( class_name);
            while ( vmt && vmt != (PVMT)CImage) {
                vmt = vmt-> base;
            }
            if ( !vmt) {
                croak( "Are you nuts?  Class ``%s'' is not inherited from ``Image''", class_name);
            }
            as_class = true;
        }
        else {
            if ( !kind_of( self, CImage)) {
                croak( "Are you nuts?  This object is not inherited from ``Image''");
            }
        }

        if ( ( items > ( as_class ? 2 : 3)) && ( ( items % 2) != 0)) {
            croak( "Odd number of parameters for Image::save\n");
        }

        if ( as_class) {
            info = plist_create( ( items - 2) / 2, 1);
            for ( i = 2; i < items; i += 2) {
                SV *arg0 = ST( i);
                SV *arg1 = ST( i + 1);
                Handle img;
                if ( ! ( img = gimme_the_mate( arg0))) {
                    croak( "Parameter #%d in call to Image::save is not an object", i + 1);
                }
                if ( ! kind_of( img, CImage)) {
                    croak( "Parameter #%d in call to Image::save is not an Image or its derivative", i + 1);
                }
                if ( ( ! SvROK( arg1)) || ( SvTYPE( SvRV( arg1)) != SVt_PVHV)) {
                    croak( "Parameter #%d in call to Image::save must be a hash reference\n", i + 1);
                }
                add_image_profile( PImage( img), ( HV *) SvRV( arg1), info, "Image::save");
            }
        }
        else {
            info = plist_create( 1, 1);
            if ( items == 3) {
                SV *arg = ST( 2);
                if ( ( ! SvROK( arg)) || ( SvTYPE( SvRV( arg)) != SVt_PVHV)) {
                    croak( "Second parameter in call to Image::save must be a hash reference\n");
                }
                add_image_profile( PImage( self), ( HV *) SvRV( arg), info, "Image::save");
            }
            else {
                profile = parse_hv( ax, sp, items, mark, 2, "Image::load");
                add_image_profile( PImage( self), profile, info, "Image::save");
                sv_free( ( SV *) profile);
            }
        }

        result = apc_image_save( filename, nil, info);

        if ( info) {
            for ( i = 0; i < info->count; i++) {
                img_info_destroy( ( PImgInfo) list_at( info, i));
            }
            plist_destroy( info);
        }

        SPAGAIN;
        SP -= items;

        XPUSHs( sv_2mortal( newSViv( result)));
        PUTBACK;
        return;
    }
    croak ("Invalid usage of %s", "Image::save");
}

Bool
Image_save_REDEFINED( SV *who, char *filename, PList imgInfo)
{
   /* XXX - one day we might want to implement this function in a more clever way */
   warn( "Invalid call of Image::save(): ask developers to make it valid");
   return false;
}

Bool
Image_save( SV *who, char *filename, PList imgInfo)
{
   (void)who;
   if ( apc_image_save( filename, NULL, imgInfo)) {
      return false;
   } else {
      return false;
   }
}

int
Image_get_type( Handle self)
{
   return var->type;
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
   if ( is_opt( optInDraw)) return false;
   inherited begin_paint( self);
   if ( !( ok = apc_image_begin_paint( self)))
      inherited end_paint( self);
   return ok;
}

Bool
Image_begin_paint_info( Handle self)
{
   Bool ok;
   if ( is_opt( optInDraw))     return true;
   if ( is_opt( optInDrawInfo)) return false;
   inherited begin_paint_info( self);
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

static void
modify_Image( Handle self, PImgInfo imageInfo)
{
    int i;
    HV *extraInfo = nil;
    unsigned gotProps = 0;
    unsigned paletteType = 0;
#define REQPROP_WIDTH 0x01
#define REQPROP_HEIGHT 0x02
#define REQPROP_TYPE 0x04
#define REQPROP_DATA 0x08
#define REQPROP_PALETTE 0x10
#define REQPROP_LINESIZE 0x20
#define REQPROP_PALETTESIZE 0x40
#define EXISTPROP_PALETTETYPE 0x80
#define REQPROP_ALL ( REQPROP_WIDTH | REQPROP_HEIGHT | REQPROP_TYPE | \
                      REQPROP_DATA | REQPROP_PALETTE | REQPROP_LINESIZE | \
                      REQPROP_PALETTESIZE)

    ++SvREFCNT( SvRV(PImage(self)->mate));
    my->make_empty( self);

    for ( i = 0 ; i < imageInfo->propList->count; i++) {
        PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, i);
        if ( strcmp( imgProp->name, "width") == 0) {
            gotProps |= REQPROP_WIDTH;
            var->w = imgProp->val.Int;
        }
        else if ( strcmp( imgProp->name, "height") == 0) {
            gotProps |= REQPROP_HEIGHT;
            var->h = imgProp->val.Int;
        }
        else if ( strcmp( imgProp->name, "type") == 0) {
            gotProps |= REQPROP_TYPE;
            var->type = imgProp->val.Int;
        }
        else if ( strcmp( imgProp->name, "lineSize") == 0) {
            gotProps |= REQPROP_LINESIZE;
            var->lineSize = imgProp->val.Int;
        }
        else if ( strcmp( imgProp->name, "data") == 0) {
            gotProps |= REQPROP_DATA;
            var->data = imgProp->val.pByte;
            var->dataSize = imgProp->size;
            imgProp->val.pByte = NULL;
            imgProp->size = 0;
        }
        else if ( strcmp( imgProp->name, "palette") == 0) {
            gotProps |= REQPROP_PALETTE;
            var->palette = ( PRGBColor) imgProp->val.pByte;
            imgProp->val.pByte = NULL;
            imgProp->size = 0;
        }
        else if ( strcmp( imgProp->name, "paletteSize") == 0) {
            gotProps |= REQPROP_PALETTESIZE;
            var->palSize = imgProp->val.Int;
        }
        else if ( strcmp( imgProp->name, "name") == 0) {
            my->set_name( self, imgProp->val.String);
            free( imgProp->val.String);
            imgProp->val.String = nil;
        }
	else if ( strcmp( imgProp->name, "paletteType") == 0) {
	    gotProps |= EXISTPROP_PALETTETYPE;
	    paletteType = imgProp->val.Int;
	}
        else if ( imageInfo->extraInfo) {
            HV *profile = extraInfo;
            if ( ! profile) {
                if ( hv_exists( ( HV *) SvRV( var->mate), "extraInfo", 9)) {
                    SV **prf;
                    prf = hv_fetch( ( HV *) SvRV( var->mate), "extraInfo", 9, 0);
                    if ( ! prf
                         || ! SvROK( *prf)
                         || SvTYPE( SvRV( *prf)) != SVt_PVHV ) {
                        croak( "Image::load can't fetch ``extraInfo''");
                    }
                    profile = ( HV *) SvRV( *prf);
                    hv_clear( profile);
                } else {
                    profile = newHV();
                    if ( ! profile) {
                        croak( "Image::load: can't create new ``extraInfo'' profile");
                    }
                    hv_store( ( HV *) SvRV( var->mate), "extraInfo", 9,
                              newRV_noinc( ( SV *) profile),  0);
                }

                extraInfo = profile;
            }

            add_image_property_to_profile( profile, imgProp, "Image::load");
        }
    }

    if ( ( gotProps & REQPROP_ALL) != REQPROP_ALL) {
        croak( "*** INTERNAL *** Got an incomplete image information from driver (signature: %04X)", gotProps);
    }
    if ( ( var->lineSize * var->h) != var->dataSize) {
        croak( "Image data/line size inconsistency detected");
    }
    if ( ( imageInfo->convertionAllowed >= ICL_NONDESTRUCTIVE)
	 && ( ( gotProps & EXISTPROP_PALETTETYPE) == EXISTPROP_PALETTETYPE)
	 && ( paletteType != var->type)) {
	/* DOLBUG( "Setting palette to type %02x from %02x, palsize: %d\n", paletteType, var->type, var->palSize); */
	my->set_type( self, paletteType);
    }
    --SvREFCNT( SvRV(PImage(self)->mate));
    my->update_change( self);
    /* DOLBUG( "Got type: %02x\n", var->type); */
}

XS( Image_load_FROMPERL) {
   dXSARGS;
   Bool result;
   Bool as_class = false;
   Bool wantarray = ( GIMME_V == G_ARRAY);
   Bool singleProfile = true;
   Handle self;
   char *class_name = nil;
   PList info = nil;
   int i;
   HV *hv;

   if ( items >= 2) {
      SV *who = ST(0);
      char *filename = SvPV( ST( 1), na);

      if (!( self = gimme_the_mate( who))) {
         PVMT vmt;
         if (!SvPOK( who)) {
            croak( "Call Image->load() either as instance method or as class method");
         }
         class_name = SvPV( who, na);
         vmt = gimme_the_vmt( class_name);
         while ( vmt && vmt != (PVMT)CImage) {
            vmt = vmt-> base;
         }
         if ( !vmt) {
            croak( "Are you nuts?  Class ``%s'' is not inherited from ``Image''", class_name);
         }
         as_class = true;
      } else {
         if ( !kind_of( self, CImage)) {
            croak( "Are you nuts?  This object is not inherited from ``Image''");
         }
      }

      if ( items > 2 && SvROK( ST( 2)) && SvTYPE( SvRV( ST(2))) == SVt_PVHV) {
         /* assuming multi-profile format */
         singleProfile = false;
         if (!as_class) {
            croak ("Invalid usage of Image::load: multiple profiles are not allowed when called as an instance method");
         }
         for ( i = 2; i < items; i++) {
            if ( !SvROK( ST( i)) || SvTYPE( SvRV( ST(i))) != SVt_PVHV) {
               croak ("Invalid usage of Image::load: hash reference expected for argument %d", i);
            }
         }
         info = plist_create( items - 2, 1);
         for ( i = 2; i < items; i++) {
            add_image_profile( nil, ( HV *) SvRV( ST( i)), info, "Image::load");
         }
      }
      else {
         /* assuming normal single profile */
         if ( items % 2 != 0) {
            croak ("Invalid usage of Image::load: odd number of optional arguments");
         }
         info = plist_create( 1, 1);
         hv = parse_hv( ax, sp, items, mark, 2, "Image::load");
         add_image_profile( nil, hv, info, "Image::load");
         sv_free(( SV*)hv);
      }
/*    DOLBUG( "Image_load_FROMPERL: calling apc_image_read\n"); */
      result = apc_image_read( filename, info, true);
/*    DOLBUG( "Image_load_FROMPERL: apc_image_read( %s) %s\n",
              filename,
              result ? "succeed" : "failed"
          ); */
      if ( ! result) {
          char errorBuf[ 1024];
          apc_image_get_error_message( errorBuf, 1024);
/*        DOLBUG( "Error message: %s\n", errorBuf); */
      }
      SPAGAIN;
      SP -= items;
      if ( result) {
          if ( as_class) {
              Handle *selves;
              selves = malloc( info->count * sizeof( Handle));
              for ( i = 0; i < info->count; i++) {
                  self = ( Handle) create_object( class_name, "", nil);
                  SPAGAIN;
                  SP -= items;
                  modify_Image( self, ( PImgInfo) list_at( info, i));
                  SPAGAIN;
                  SP -= items;
                  selves[ i] = self;
              }
              if ( wantarray || ( singleProfile && ( info->count <= 1))) {
                  for ( i = 0; i < info-> count; i++) {
                      XPUSHs( sv_mortalcopy( PImage( selves[ i])-> mate));
                  }
              }
              else {
                  AV *av = newAV();
                  for ( i = 0; i < info-> count; i++) {
                      av_push( av, newSVsv( PImage( selves[ i])-> mate));
                  }
                  XPUSHs( sv_2mortal( newRV_noinc( (SV*) av)));
              }
              free( selves);
          }
          else {
              if ( info->count > 1) {
                  croak( "Invalid usage of Image::load: request of multiple images when called as an instance method");
              }
              else {
                  modify_Image( self, ( PImgInfo) list_at( info, 0));
                  SPAGAIN;
                  SP -= items;
                  XPUSHs( sv_mortalcopy( PImage( self)-> mate));
              }
          }
      }
      else {
          if ( ! wantarray) {
              XPUSHs( &sv_undef);
          }
      }

      if ( info) {
          for ( i = 0; i < info->count; i++) {
              img_info_destroy( ( PImgInfo) list_at( info, i));
          }
          plist_destroy( info);
      }

      PUTBACK;
      return;
   }
   croak ("Invalid usage of %s", "Image::load");
}

Handle
Image_load_REDEFINED( SV *who, char *filename, PList imgInfo)
{
   /* XXX - one day we might want to implement this function in a more clever way */
   warn( "Invalid call of Image::load(): ask developers to make it valid");
   return nilHandle;
}

Handle
Image_load( SV *who, char *filename, PList imgInfo)
{
   (void)who;
   if ( apc_image_read( filename, imgInfo, true)) {
      return nilHandle;
   } else {
      return nilHandle;
   }
}

#ifndef __unix
XS( Image_get_info_FROMPERL)
{
   warn( "Image::get_info() is unsupported in this version yet.");
   return;
}
#else
XS( Image_get_info_FROMPERL)
{
   dXSARGS;
   Bool result;
   Bool wantarray = ( GIMME_V == G_ARRAY);
   Bool singleProfile = true;
   Handle self;
   char *class_name;
   PList info = nil;
   int i;
   HV *hv;

   if ( items >= 2) {
      SV *who = ST(0);
      char *filename = SvPV( ST( 1), na);

      if ( ! ( self = gimme_the_mate( who))) {
         PVMT vmt;
         if (!SvPOK( who)) {
            croak( "Image->get_info() called not as class method");
         }
         class_name = SvPV( who, na);
         vmt = gimme_the_vmt( class_name);
         while ( vmt && vmt != (PVMT)CImage) {
            vmt = vmt-> base;
         }
         if ( !vmt) {
            croak( "Are you nuts?  Class ``%s'' is not inherited from ``Image''", class_name);
         }
      } else {
          croak( "Image::get_info cannot be called as an instance method");
      }

      if ( items > 2 && SvROK( ST( 2)) && SvTYPE( SvRV( ST(2))) == SVt_PVHV) {
         /* assuming multi-profile format */
         singleProfile = false;
         for ( i = 2; i < items; i++) {
            if ( !SvROK( ST( i)) || SvTYPE( SvRV( ST(i))) != SVt_PVHV) {
               croak ("Invalid usage of Image::get_info: hash reference expected for argument %d", i);
            }
         }
         info = plist_create( items - 2, 1);
         for ( i = 2; i < items; i++) {
            add_image_profile( nil, ( HV *) SvRV( ST(i)), info, "Image::get_info");
         }
      } else {
         /* assuming normal single profile */
         if ( items % 2 != 0) {
            croak ("Invalid usage of Image::get_info: odd number of optional arguments");
         }
         info = plist_create( 1, 1);
         hv = parse_hv( ax, sp, items, mark, 2, "Image::get_info");
         add_image_profile( nil, hv, info, "Image::get_info");
         sv_free(( SV*)hv);
      }
      result = apc_image_read( filename, info, false);
      SPAGAIN;
      SP -= items;
      if ( result) {
          HV **hvInfo;
          hvInfo = malloc( info->count * sizeof( HV *));
          for ( i = 0; i < info->count; i++) {
              int j;
              PImgInfo imageInfo = ( PImgInfo) list_at( info, i);
              SPAGAIN;
              SP -= items;
              hvInfo[ i] = newHV();
              for ( j = 0; j < imageInfo->propList->count; j++) {
                  PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, j);
                  add_image_property_to_profile( hvInfo[ i], imgProp, "Image::get_info");
              }
          }
          if ( wantarray || ( singleProfile && ( info->count <= 1))) {
              if ( wantarray && singleProfile && ( info->count <= 1)) {
                  hv_delete( hvInfo[ 0], "__ORDER__", 9, G_DISCARD);
                  push_hv( ax, sp, items, mark, 0, hvInfo[ 0]);
                  SPAGAIN;
              } else {
                  for( i = 0; i < info->count; i++) {
                      XPUSHs( sv_2mortal( newRV_noinc( (SV*) hvInfo[ i])));
                  }
              }
          }
          else {
              AV *av = newAV();
              for ( i = 0; i < info-> count; i++) {
                  av_push( av, newRV_noinc( ( SV*) hvInfo[ i]));
              }
              XPUSHs( sv_2mortal( newRV_noinc( (SV*) av)));
          }
          free( hvInfo);
      }
      else {
          if ( ! wantarray) {
              XPUSHs( &sv_undef);
          }
      }

      if ( info) {
          for ( i =0; i < info->count; i++) {
              img_info_destroy( ( PImgInfo) list_at( info, i));
          }
          plist_destroy( info);
      }

      PUTBACK;
      return;
   }
   croak ("Invalid usage of %s", "Image::get_info");
}
#endif

Handle
Image_get_info_REDEFINED( SV *who, char *filename)
{
   /* XXX - one day we might want to implement this function in a more clever way */
   warn( "Invalid call of Image::get_info(): ask developers to make it valid");
   return nilHandle;
}

Handle
Image_get_info( SV *who, char *filename)
{
   (void)who;
   return nilHandle;
}

void
Image_update_change( Handle self)
{
   if ( var-> stage <= csNormal) apc_image_update_change( self);
   var->statsCache = 0;
}

int Image_get_conversion( Handle self) { return var->conversion;}

double
Image_get_stats( Handle self, int index)
{
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
   double sum = 0.0, sum2 = 0.0, minv = 0.0, maxv = 0.0, v;
   int y;

   if ( index < 0 || index > isMaxIndex) return NAN;
   if ( var->statsCache & ( 1 << index)) return var->stats[ index];
   /* calculate image stats */
   switch (var->type) {
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

   return var->stats[ index];
}

void
Image_set_stats( Handle self, double value, int index)
{
   if ( index < 0 || index > isMaxIndex) return;
   var->stats[ index] = value;
   var->statsCache |= 1 << index;
}

void
Image_resample( Handle self, double srcLo, double srcHi, double dstLo, double dstHi)
{
#define RSPARMS self, var->data, var->type, srcLo, srcHi, dstLo, dstHi
   switch ( var->type)
   {
      case imByte:   rs_Byte_Byte     ( RSPARMS); break;
      case imShort:  rs_short_short   ( RSPARMS); break;
      case imLong:   rs_long_long     ( RSPARMS); break;
      case imFloat:  rs_float_float   ( RSPARMS); break;
      case imDouble: rs_double_double ( RSPARMS); break;
      default: return;
   }
   my->update_change( self);
}

SV *
Image_get_palette( Handle self)
{
   AV * av = newAV();
   int i;
   int colors = ( 1 << ( var->type & imBPP)) & 0x1ff;
   Byte * pal = ( Byte*) var->palette;
   if (( var->type & imGrayScale) && (( var->type & imBPP) > imbpp8)) colors = 256;
   for ( i = 0; i < colors*3; i++) av_push( av, newSViv( pal[ i]));
   return newRV_noinc(( SV *) av);
}

void
Image_set_palette( Handle self, SV * palette)
{
   if ( var->stage > csNormal) return;
   if ( var->type & imGrayScale)
      return;
   if ( !var->palette)
      return;

   if ( !Image_read_palette( self, var->palette, palette))
      warn("RTC0107: Invalid array reference passed to Image::set_palette");
   my->update_change( self);
}

void Image_set_conversion( Handle self, int conversion)
{
   var->conversion = conversion;
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
      var->data = malloc( var->dataSize);
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

void
Image_set_preserve_type( Handle self, Bool preserveType)
{
   opt_assign( optPreserveType, preserveType);
}

Bool
Image_get_preserve_type( Handle self)
{
   return is_opt( optPreserveType);
}

Color
Image_get_pixel( Handle self,int x,int y)
{
    #define BGRto32(pal) ((var->palette[pal].r<<16) | (var->palette[pal].g<<8) | (var->palette[pal].b))
    if ( opt_InPaint)
        return inherited get_pixel(self,x,y);
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
                    double rangeLo = my-> get_stats( self, isRangeLo);
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
                rangeLo = my-> get_stats( self, isRangeLo);
                return ((pd - rangeLo)/(var->stats[isRangeHi] - rangeLo))*LONG_MAX;
            }
        default:
            return 0;
    }
    #undef BGRto32
}

Bool
Image_set_pixel( Handle self,int x,int y,Color color)
{
    RGBColor rgb;
    #define LONGtoBGR(lv,clr)   ((clr).b=(lv)&0xff,(clr).g=((lv)>>8)&0xff,(clr).r=((lv)>>16)&0xff,(clr))
    if ( is_opt( optInDraw)) {
        inherited set_pixel(self,x,y,color);
        return true;
    }
    if ((x>=var->w) || (x<0) || (y>=var->h) || (y<0)) {
        return true;
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
                    double rangeLo = my-> get_stats( self, isRangeLo);
                    *(float*)(var->data+(var->lineSize*y+(x<<2)))=(((float)color)/(LONG_MAX))*(var->stats[isRangeHi]-rangeLo)+rangeLo;
                }
                else {
                    *(long*)(var->data+(var->lineSize*y+(x<<2)))=color;
                }
            }
            break;
        case imbpp64 :
            if (var->type & imRealNumber) {
                double rangeLo = my-> get_stats( self, isRangeLo);
                *(double*)(var->data+(var->lineSize*y+(x<<2)))=(((double)color)/(LONG_MAX))*(var->stats[isRangeHi]-rangeLo)+rangeLo;
            }
            break;
        default:
            return false;
    }
    my->update_change( self);
    #undef LONGtoBGR
    return true;
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
   if ( i-> type != var->type) {
      /* Object does not support given type, but Image supports them all */
      Handle img = ( Handle) create_object( "Prima::Image", "iiii",
             "width"     , var->w,
             "height"    , var->h,
             "type"      , var->type,
             "conversion", var->conversion
      );
      memcpy( PImage(img)-> palette, var->palette, 768);
      memcpy( PImage(img)-> data,    var->data,    var->dataSize);
      CImage(img)->set_type( img, i-> type);
      if ( i->dataSize != PImage( img)->dataSize)
         croak("RTC0108: Image::dup consistency failed");
      memcpy( i-> data,    PImage(img)-> data,    PImage(img)->dataSize);
      memcpy( i-> palette, PImage(img)-> palette, 768);
      Object_destroy( img);
   } else
      memcpy( i-> data, var->data, var->dataSize);
   memcpy( i-> stats, var->stats, sizeof( var->stats));
   i-> statsCache = var->statsCache;
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
