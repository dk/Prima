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
#include "Img.h"
#include "Image.h"
#ifndef __unix  /* Temporary hack */
#include "gbm.h"
#endif /* __unix */
#include "Image.inc"
#include "Clipboard.h"

#undef  my
#define inherited CDrawable->
#define my  ((( PImage) self)-> self)
#define var (( PImage) self)

#include "imgtype.cinc"
#include "imgscale.cinc"

static int Image_read_palette( Handle self, PRGBColor palBuf, SV * palette);

void
Image_init( Handle self, HV * profile)
{
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
   Image_read_palette( self, var->palette, pget_sv( palette));
   my->set_data( self, pget_sv( data));
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
   if (!( type & imGrayScale))
      Image_read_palette( self, var->palette, palette);
   if ( var->type == imByte && type == im256)
   {
      var->type = type;
      return;
   }
   var->lineSize = (( var->w * ( type & imBPP) + 31) / 32) * 4;
   var->dataSize = ( var->lineSize) * var->h;
   var->palSize = (1 << (var->type & imBPP)) & 0x1ff;
   if ( var->dataSize > 0)
   {
      newData = malloc( var->dataSize);
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
   var->status   = ieOK;
   my->update_change( self);
}

char *
Image_get_status_string( Handle self)
{
#ifdef __unix  /* Temporary hack */
   return (char *)apc_image_get_error_message( nil, 0);
#else
   return (char *)gbm_err( var->status);
#endif /* __unix */
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
   int dataSize;
   void *data = SvPV( svdata, dataSize);

   if ( var->stage > csNormal) return;
   if ( is_opt( optInDraw) || dataSize <= 0) return;

   memcpy( var->data, data, dataSize > var->dataSize ? var->dataSize : dataSize);
   my->update_change( self);
}

Bool
Image_save( Handle self, char *filename, HV *profile)
#ifndef SCARY_ERRORS
#define bekilled(__rc) { var->status = __rc; return false;}
#else
#define bekilled(__rc) {                                                                    \
   switch( __rc){                                                                      \
      ieError:          croak("RTC0102: Error saving %s", filename);                            \
      ieInvalidType:    croak("RTC0103: Invalid file type to save %s", filename);               \
      ieFileNotFound:   croak("RTC0104: Cannot open/write %s", filename);                       \
      ieInvalidOptions: croak("RTC0105: Invalid options given for %s", filename);               \
      ieNotSupported:   croak("RTC0106: Requested image format not supported for %s", filename);\
   }                                                                                   \
   return false;                                                                       \
}
#endif
{
#ifdef __unix /* Temporary hack */
   return true;
#else
   int bpp = var->type & imBPP;
   int file;
   GBM gbm;
   GBM_ERR rc = ieOK;
   int fileType;
   char options[1024] = {0};
   char oneOpt[ 256];

   if (( bpp != 1) && ( bpp != 4) && ( bpp != 8) && ( bpp != 24))
      bekilled( ieInvalidType);

   if (( rc = gbm_guess_filetype( filename, &fileType)) != ieOK)
      bekilled( ieInvalidType);

   /*
     gif, iff, lbm - transparent color "transcol=%d"
     gif      - interlaced             "ilace"
     tiff     - compressed             "lzw"
     - description            "imagedescription=%s"
     - special
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
   if ( fileType == itBMP) strcat( options, " inv");  /* GBM is a bad guy */
   if ( pexist( interlaced))   strcat(options, " ilace");
   if ( pexist( compressed))   strcat(options, " lzw");
   if ( pexist( progressive))  strcat(options, " prog");
   if ( pexist( noAutoFilter)) strcat(options, " nofilters");
   if ( pexist( quality))
      strcat(options, (snprintf(oneOpt, 256, " quality=%d",(int) pget_i( quality)), oneOpt));
   if ( pexist( compressionLevel))
      strcat(options, (snprintf(oneOpt, 256, " zlevel=%d",(int) pget_i( compressionLevel)), oneOpt));
   if ( pexist( transparentColorIndex))
      strcat(options, (snprintf(oneOpt, 256, " transcol=%d",(int) pget_i( transparentColorIndex)), oneOpt));
   if ( pexist( transparentColorRGB))
   {
      SV *sv = pget_sv( transparentColorRGB);
      AV *av;
      if ( !SvROK( sv) || ( SvTYPE( SvRV( sv)) != SVt_PVAV)) bekilled( ieInvalidOptions);
      av = ( AV*) SvRV( sv);
      if ( av_len( av) != 2) bekilled( ieInvalidOptions);
      strcat(options, (snprintf(oneOpt, 256, " transcol=%d/%d/%d",
				(int)SvIV( *av_fetch( av, 2, 0)),
				(int)SvIV( *av_fetch( av, 1, 0)),
				(int)SvIV( *av_fetch( av, 0, 0))
	 ), oneOpt));
   }
   if ( pexist( transparentColor))
   {
      unsigned long c = pget_i( transparentColor);
      strcat(options, (snprintf(oneOpt,256, " transcol=%ld/%ld/%ld",
				( c >> 16) & 0xFF,
				( c >> 8 ) & 0xFF,
				c & 0xFF
	 ), oneOpt));
   }
   if ( pexist( compression))
   {
      char * c = pget_c( compression);
      if ( strcmp( c, "standard") == 0) strcat(options, " zstrategy=d");
      else if ( strcmp( c, "huffman") == 0) strcat(options, " zstrategy=h");
      else if ( strcmp( c, "filtered") == 0) strcat(options, " zstrategy=f");
   }
   if ( pexist( description))
   {
      char *cc = oneOpt;
      strcat(options, (snprintf(oneOpt,256, " Description=\"%s\"",(char *) pget_c( description)), oneOpt));
      /* tiff */
      snprintf(oneOpt,256," imagedescription=%s",(char *) pget_c( description));
      while (*++cc) if (*cc == ' ') *cc='_';
      strcat( options, oneOpt);
   }

   file = open( filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
   if ( file == -1) bekilled( ieFileNotFound);

   gbm. w = var->w;
   gbm. h = var->h;
   gbm. bpp = bpp;
   {
      RGBColor pal [ 256];
      cm_reverse_palette( var->palette, pal, 256);
      rc = gbm_write( filename, file, fileType, &gbm, ( GBMRGB *) pal, var->data, options);
   }
   if ( rc != ieOK)
   {
      close( file);
      remove( filename);
      switch ( rc)
      {
      case ieFileNotFound: case ieInvalidType: case ieInvalidOptions: case ieNotSupported:
	 break;
      default:
	 rc = ieError;
      }
      bekilled( rc);
   }
   close( file);
   var->status = ieOK;
   return true;
#endif /* __unix */
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
   if ( is_opt( optPreserveType) && var->type != oldType)
      my->reset( self, oldType, nilSV);
   else
      my->update_change( self);
}

void
Image_end_paint_info( Handle self)
{
   if ( !is_opt( optInDrawInfo)) return;
   apc_image_end_paint_info( self);
   inherited end_paint_info( self);
}


Bool
load_image_indirect( Handle self, char *filename, char *subIndex)
#define checkrc if ( rc != ieOK)  \
        {                         \
            var->status = rc;      \
            free( data);          \
            free( palette);       \
            close( file);         \
            return false;         \
        }
{
#ifdef __unix /* Temporary hack */
   return true;
#else
   GBM gbm;
   unsigned char *data = nil;
   GBM_ERR rc;
   GBMRGB * palette = nil;
   int file;
   int ft;
   int lineSize, dataSize;

   if ( var->stage > csNormal) return false;
   if ( opt_InPaint) return false;

   memset( &gbm, 0, sizeof( GBM));
   file = open( filename, O_RDONLY | O_BINARY);
   if ( file < 0)
   {
      var->status = ieFileNotFound;
      return false;
   }
   ft = image_guess_type( file);
   if ( ft < 0)
   {
      rc = gbm_guess_filetype( filename, &ft);
      checkrc;
   }

   if ( ft == itBMP) strcat( subIndex, " inv");  /* GBM is baran */
   rc = gbm_read_header( filename, file, ft, &gbm, subIndex);
   checkrc;

   palette = malloc( 0x100 * sizeof( GBMRGB));
   rc = gbm_read_palette( file, ft, &gbm, ( GBMRGB*) palette);
   checkrc;

   lineSize = (( gbm. w * gbm. bpp + 31) / 32) * 4;
   dataSize = gbm. h * lineSize;
   data = ( dataSize > 0) ? malloc( dataSize) : nil;
   rc = gbm_read_data( file, ft, &gbm, data);
   checkrc;

   close( file);

   /* init image */
   my->make_empty( self);
   var->w        = gbm. w;
   var->h        = gbm. h;
   var->type     = gbm. bpp;
   var->data     = data;
   var->palette  = ( PRGBColor) palette;
   var->lineSize = lineSize;
   var->dataSize = dataSize;
   var->palSize  = (1 << (var->type & imBPP)) & 0x1ff;
   cm_reverse_palette( var->palette, var->palette, 256);

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
   var->status = ieOK;
   return true;
#endif /* __unix */
}

static void
add_image_profile( HV *profile, PList imgInfo)
{
   SV *value;
   char *key;
   /* int keyLen; */
   AV *ary;
   SV **elem;
   PImgInfo imageInfo;
   PImgProperty imgProp;
   HE *he;

   imageInfo = ( PImgInfo) malloc( sizeof( ImgInfo));
   bzero( imageInfo, sizeof( ImgInfo));
   list_add( imgInfo, ( Handle) imageInfo);
   imageInfo->propList = plist_create( HvKEYS( profile), 1);

   hv_iterinit( profile);
   for (;;)
   {
      if (( he = hv_iternext( profile)) == nil)
         break;
      value = HeVAL( he);
      key = HeKEY( he);
      /* keyLen = HeKLEN( he); */

      if ( SvROK( value)) {
	 int i, n;
	 char **propArray;

	 if ( SvTYPE( SvRV( value)) != SVt_PVAV) {
	    croak( "Invalid usage of Image::load: illegal reference type for key ``%s''", key);
	 }
	 /* Reference to an array */
	 ary = (AV*)SvRV( value);
	 n = av_len( ary) + 1;
	 propArray = n ? malloc( sizeof( char *) * n) : nil;
	 for ( i = 0; i < n; i++) {
	    elem = av_fetch( ary, i, false);
	    if ( ! elem) {
	       free( propArray);
	       croak( "Image::load: cannot fetch element %d of ``%s''", i, key);
	    }
	    if ( SvROK(*elem)) {
	       free( propArray);
	       croak( "Invalid usage of Image::load: array element %d of key ``%s'' cannot be a reference", i, key);
	    }
	    propArray[ i] = duplicate_string( SvPV( *elem, na));
	 }
	 imgProp = apc_image_add_property( imageInfo, key, PROPTYPE_STRING, n);
	 imgProp->val.pString = propArray;
      } else {
	 /* scalar */
	 imgProp = apc_image_add_property( imageInfo, key, PROPTYPE_STRING, -1);
	 imgProp->val.String = duplicate_string( SvPV( value, na));
      }
   }
}

static void
modify_Image( Handle self, PImgInfo imageInfo)
{
    int i;
    HV *extraInfo = nil;
    unsigned reqProps = 0;
#define REQPROP_WIDTH 0x01
#define REQPROP_HEIGHT 0x02
#define REQPROP_TYPE 0x04
#define REQPROP_DATA 0x08
#define REQPROP_PALETTE 0x10
#define REQPROP_LINESIZE 0x20
#define REQPROP_ALL ( REQPROP_WIDTH | REQPROP_HEIGHT | REQPROP_TYPE | \
		      REQPROP_DATA | REQPROP_PALETTE | REQPROP_LINESIZE)

    my->make_empty( self);

    fprintf( stderr, "Prop. list contains %d entries\n", imageInfo->propList->count);

    for ( i = ( imageInfo->propList->count - 1); i >= 0; i--) {
	PImgProperty imgProp = ( PImgProperty) list_at( imageInfo->propList, i);
	if ( strcmp( imgProp->name, "width") == 0) {
	    reqProps |= REQPROP_WIDTH;
	    var->w = imgProp->val.Int;
	}
	else if ( strcmp( imgProp->name, "height") == 0) {
	    reqProps |= REQPROP_HEIGHT;
	    var->h = imgProp->val.Int;
	}
	else if ( strcmp( imgProp->name, "type") == 0) {
	    reqProps |= REQPROP_TYPE;
	    var->type = imgProp->val.Int;
	}
	else if ( strcmp( imgProp->name, "lineSize") == 0) {
	    reqProps |= REQPROP_LINESIZE;
	    var->lineSize = imgProp->val.Int;
	}
	else if ( strcmp( imgProp->name, "data") == 0) {
	    reqProps |= REQPROP_DATA;
	    var->data = imgProp->val.pByte;
	    var->dataSize = imgProp->size;
	    imgProp->val.pByte = NULL;
	    imgProp->size = 0;
	}
	else if ( strcmp( imgProp->name, "palette") == 0) {
	    reqProps |= REQPROP_PALETTE;
	    var->palette = ( PRGBColor) imgProp->val.pByte;
	    var->palSize = imgProp->size;
	    imgProp->val.pByte = NULL;
	    imgProp->size = 0;
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
			      newRV_inc( ( SV *) profile),  0);
		}

		extraInfo = profile;
	    }

	    if ( imgProp->size == -1) {
		switch ( imgProp->flags & PROPTYPE_MASK) {
		    case PROPTYPE_STRING:
			pset_c( imgProp->name, imgProp->val.String);
			break;
		    case PROPTYPE_DOUBLE:
			pset_f( imgProp->name, imgProp->val.Double);
			break;
		    case PROPTYPE_BYTE:
			pset_i( imgProp->name, imgProp->val.Byte);
			break;
		    case PROPTYPE_INT:
		    default:
			pset_i( imgProp->name, imgProp->val.Int);
			break;
		}
	    }
	    else {
		AV *av;
		int j;

		av = newAV();
		if ( ! av) {
		    croak( "Image::load: can't allocate an array");
		}

		for ( j = 0; j < imgProp->size; j++) {
		    SV *sv;
		    switch( imgProp->flags & PROPTYPE_MASK) {
			case PROPTYPE_STRING:
			    sv = newSVpv( imgProp->val.pString[ j], 0);
			    break;
			case PROPTYPE_DOUBLE:
			    sv= newSVnv( imgProp->val.pDouble[ j]);
			    break;
			case PROPTYPE_BYTE:
			    sv = newSViv( imgProp->val.pByte[ j]);
			    break;
			case PROPTYPE_INT:
			default:
			    sv = newSViv( imgProp->val.pInt[ j]);
			    break;
		    }
		    av_push( av, sv);
		}
		pset_sv( imgProp->name, newRV_inc( ( SV *) av));
	    }
	}

	apc_image_clear_property( imgProp);
	list_delete_at( imageInfo->propList, i);
    }

    if ( reqProps != REQPROP_ALL) {
	croak( "*** INTERNAL *** Got an incomplete image information from driver (signature: %04X)", reqProps);
    }
    if ( ( var->lineSize * var->h) != var->dataSize) {
	croak( "Image data/line size inconsistency detected");
    }
}

XS( Image_load_FROMPERL) {
   dXSARGS;
   Bool result;
   Bool as_class = false;
   Bool wantarray = ( GIMME_V == G_ARRAY);
   Handle self;
   char *class_name;
   PList info;
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
	    add_image_profile(( HV *)SvRV( ST(i)), info);
	 }
      } else {
	 /* assuming normal single profile */
	 if ( items % 2 != 0) {
	    croak ("Invalid usage of Image::load: odd number of optional arguments");
	 }
	 info = plist_create( 1, 1);
	 hv = parse_hv( ax, sp, items, mark, 2, "Image::load");
	 add_image_profile( hv, info);
	 sv_free(( SV*)hv);
      }
      result = apc_image_read( filename, info, true);
      SPAGAIN;
      SP -= items;
      if ( result) {
	  if ( as_class) {
	  }
	  else {
	      if ( info->count > 1) {
		  croak( "Invalid usage of Image::load: request of multiple images when called as an instance method");
	      }
	      else {
		  modify_Image( self, ( PImgInfo) list_at( info, 0));
	      }
	  }
      }
      else {
	  if ( ! wantarray) {
	      XPUSHs( &sv_undef);
	  }
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
#ifdef __unix
   (void)who;
   if ( apc_image_read( filename, imgInfo, true)) {
      return nilHandle;
   } else {
      return nilHandle;
   }
#else
   Bool ret;
   char buf[ 15];
   if ( index >= 0) snprintf( buf, 15, "index=%d", index); else buf[ 0] = 0;
   ret = load_image_indirect( self, filename, buf);
   if ( ret) my->update_change( self);
   return ret;
#endif /* __unix */
}

void
Image_update_change( Handle self)
{
   apc_image_update_change( self);
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
      case imByte:    gather_stats(U8);     break;
      case imShort:   gather_stats(I16);    break;
      case imLong:    gather_stats(I32);    break;
      case imFloat:   gather_stats(float);  break;
      case imDouble:  gather_stats(double); break;
      default:        return NAN;
   }
   if ( var->w * var->h > 0)
   {
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
   if (( var->type & imGrayScale) || (( var->type & imBPP) > imbpp8))
      var->conversion = ictNone;
   else
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
   var->status = ieOK;
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
                p=(p >> (x & 7)) & 1;
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
                    p=((pf - var->stats[isRangeLo])/(var->stats[isRangeHi] - var->stats[isRangeLo]))*LONG_MAX;
                }
                else {
                    p=*(long*)(var->data + (var->lineSize*y+x*4));
                }
                return p;
            }
        case imbpp64:
            {
                double pd=*(double*)(var->data + (var->lineSize*y+x*8));
                if ((var->type & imComplexNumber) || (var->type & imTrigComplexNumber)) {
                    return 0;
                }
                return ((pd - var->stats[isRangeLo])/(var->stats[isRangeHi] - var->stats[isRangeLo]))*LONG_MAX;
            }
        default:
            return 0;
    }
    #undef BGRto32
}

void
Image_set_pixel( Handle self,int x,int y,Color color)
{
    RGBColor rgb;
    #define LONGtoBGR(lv,clr)   ((clr).b=(lv)&0xff,(clr).g=((lv)>>8)&0xff,(clr).r=((lv)>>16)&0xff,(clr))
    if ( is_opt( optInDraw)) {
        inherited set_pixel(self,x,y,color);
        return;
    }
    if ((x>=var->w) || (x<0) || (y>=var->h) || (y<0)) {
        return;
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
                    *(float*)(var->data+(var->lineSize*y+(x<<2)))=(((float)color)/(LONG_MAX))*(var->stats[isRangeHi]-var->stats[isRangeLo])+var->stats[isRangeLo];
                }
                else {
                    *(long*)(var->data+(var->lineSize*y+(x<<2)))=color;
                }
            }
            break;
        case imbpp64 :
            if (var->type & imRealNumber) {
                *(double*)(var->data+(var->lineSize*y+(x<<2)))=(((double)color)/(LONG_MAX))*(var->stats[isRangeHi]-var->stats[isRangeLo])+var->stats[isRangeLo];
            }
            break;
        default:
            return;
    }
    my->update_change( self);
    #undef LONGtoBGR
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
   h = Object_create( "DeviceBitmap", profile);
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
      Handle img = ( Handle) create_object( "Image", "iiii",
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
