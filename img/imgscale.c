#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif


#define var (( PImage) self)
#define my  ((( PImage) self)-> self)

#define BS_BYTEIMPACT( type)                                                        \
void bs_##type##_in( type * srcData, type * dstData, int w, int x, int absx, long step)  \
{                                                                                   \
   Fixed count = {0};                                                               \
   int   last = 0;                                                                  \
   int   i;                                                                         \
   int   j    = ( x == absx) ? 0 : ( absx - 1);                                     \
   int   inc  = ( x == absx) ? 1 : -1;                                              \
   dstData[j] = srcData[0];                                                         \
   j += inc;                                                                        \
   for ( i = 0; i < w; i++)                                                         \
   {                                                                                \
      if ( count.i.i > last)                                                        \
      {                                                                             \
         dstData[j] = srcData[i];                                                   \
         j += inc;                                                                  \
         last = count. i. i;                                                        \
      }                                                                             \
      count. l += step;                                                             \
   }                                                                                \
}

#define BS_BYTEEXPAND( type)                                                        \
void bs_##type##_out( type * srcData, type * dstData, int w, int x, int absx, long step) \
{                                                                                   \
   Fixed count = {0};                                                               \
   int   i;                                                                         \
   int   j    = ( x == absx) ? 0 : ( absx - 1);                                     \
   int   inc  = ( x == absx) ? 1 : -1;                                              \
   int   last = 0;                                                                  \
   for ( i = 0; i < absx; i++)                                                      \
   {                                                                                \
      if ( count. i. i > last)                                                      \
      {                                                                             \
         last = count. i. i;                                                        \
         srcData++;                                                                 \
      }                                                                             \
      count. l += step;                                                             \
      dstData[j] = *srcData;                                                        \
      j += inc;                                                                     \
   }                                                                                \
}

BS_BYTEEXPAND( uint8_t)
BS_BYTEEXPAND( int16_t)
BS_BYTEEXPAND( RGBColor)
BS_BYTEEXPAND( int32_t)
BS_BYTEEXPAND( float)
BS_BYTEEXPAND( double)
BS_BYTEEXPAND( Complex)
BS_BYTEEXPAND( DComplex)

BS_BYTEIMPACT( uint8_t)
BS_BYTEIMPACT( int16_t)
BS_BYTEIMPACT( RGBColor)
BS_BYTEIMPACT( int32_t)
BS_BYTEIMPACT( float)
BS_BYTEIMPACT( double)
BS_BYTEIMPACT( Complex)
BS_BYTEIMPACT( DComplex)


void
bs_mono_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed count = {0};
   int   last   = 0;
   register int i, j;
   register U16 xd, xs;

   if ( x == absx)
   {
      xd = (( xs = srcData[0]) >> 7) & 1;
      j = 1;
      for ( i = 0; i < w; i++)
      {
         if (( i & 7) == 0) xs = srcData[ i >> 3];
         xs <<= 1;
         if ( count.i.i > last)
         {
            if (( j & 7) == 0) dstData[ ( j - 1) >> 3] = xd;
            xd <<= 1;
            xd |= ( xs >> 8) & 1;
            j++;
            last = count. i. i;
         }
         count. l += step;
      }
      i = j & 7;
      dstData[( j - 1) >> 3] = xd << ( i ? ( 8 - i) : 0);
   } else {
      j = absx - 1;
      xd = ( xs = srcData[ j >> 3]) & 0x80;
      for ( i = 0; i < w; i++)
      {
         if (( i & 7) == 0) xs = srcData[ i >> 3];
         xs <<= 1;
         if ( count.i.i > last)
         {
            if (( j & 7) == 0) dstData[ ( j + 1) >> 3] = xd;
            xd >>= 1;
            xd |= ( xs >> 1) & 0x80;
            j--;
            last = count. i. i;
         }
         count. l += step;
      }
      dstData[( j + 1) >> 3] = xd;
   }
}

void
bs_mono_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed    count = {0};
   register int i, j = 0;
   register U16 xd = 0, xs;
   int last = 0;

   if ( x == absx)
   {
      xs = srcData[0];
      for ( i = 0; i < absx; i++)
      {
         if ( count. i. i > last)
         {
            last = count.i.i;
            xs <<= 1;
            j++;
            if (( j & 7) == 0) xs = srcData[ j >> 3];
         }
         count.l += step;
         xd <<= 1;
         xd |= ( xs >> 7) & 1;
         if ((( i + 1) & 7) == 0) dstData[ i >> 3] = xd;
      }
      if ( i & 7) dstData[ i >> 3] = xd << ( 8 - ( i & 7));
      /* dstData[ i >> 3] = xd << (( i & 7) ? ( 8 - ( i & 7)) : 0); */
   } else {
      register int k = absx;
      xs = srcData[ j >> 3];
      for ( i = 0; i < absx; i++)
      {
         if ( count. i. i > last)
         {
            last = count.i.i;
            xs <<= 1;
            j++;
            if (( j & 7) == 0) xs = srcData[ j >> 3];
         }
         count.l += step;
         xd >>= 1;
         xd |= xs & 0x80;
         k--;
         if (( k & 7) == 0) dstData[( k + 1) >> 3] = xd;
      }
      dstData[( k + 0) >> 3] = xd;
   }
}

/* nibble stretching functions are requiring *dstData filled with zeros */

void bs_nibble_in( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed count = {0};
   int   last = 0;
   int   i;
   int   j    = ( x == absx) ? 0 : ( absx - 1);
   int   inc  = ( x == absx) ? 1 : -1;
   dstData[ j >> 1] |= ( j & 1) ? ( srcData[ 0] >> 4) : ( srcData[ 0] & 0xF0);
   j += inc;
   for ( i = 0; i < w; i++)
   {
      if ( count.i.i > last)
      {
         if ( i & 1)
            dstData[ j >> 1] |= ( j & 1) ? ( srcData[ i >> 1] & 0x0F) : ( srcData[ i >> 1] << 4);
         else
            dstData[ j >> 1] |= ( j & 1) ? ( srcData[ i >> 1] >> 4) : ( srcData[ i >> 1] & 0xF0);
         j += inc;
         last = count. i. i;
      }
      count. l += step;
   }
}

void bs_nibble_out( uint8_t * srcData, uint8_t * dstData, int w, int x, int absx, long step)
{
   Fixed count = {0};
   int   i, k = 0;
   int   j    = ( x == absx) ? 0 : ( absx - 1);
   int   inc  = ( x == absx) ? 1 : -1;
   int   last = 0;
   for ( i = 0; i < absx; i++)
   {
      if ( count. i. i > last)
      {
         if (( k & 1) == 1) srcData++;
         k++;
         last = count. i. i;
      }
      count. l += step;
      if ( k & 1)
         dstData[ j >> 1] |= ( j & 1) ? ( *srcData & 0x0F) : ( *srcData << 4);
      else
         dstData[ j >> 1] |= ( j & 1) ? ( *srcData >> 4) : ( *srcData & 0xF0);
      j += inc;
   }
}

void
ic_stretch( int type, Byte * srcData, int srcW, int srcH, Byte * dstData, int w, int h, Bool xStretch, Bool yStretch)
{
   int  absh = h < 0 ? -h : h;
   int  absw = w < 0 ? -w : w;
   int  srcLine = LINE_SIZE(srcW, type);
   int  dstLine = LINE_SIZE(absw, type);

   Fixed xstep, ystep, count;
   int last = 0;
   int i;
   int yMin = ( srcH > absh) ? absh : srcH;
   PStretchProc proc = ( PStretchProc) nil;
   Byte *srcLast = nil;

   if ( w == srcW) xStretch = false;
   if ( h == srcH) yStretch = false;
/* transfer case */
   if ( !xStretch && !yStretch && ( w > 0))
   {
      int y;
      int xMin = (( type & imBPP) < 8) ? 
                  ( srcLine > dstLine) ? dstLine : srcLine :
                  (((( srcW > absw) ? absw : srcW) * ( type & imBPP)) / 8);
      if ( srcW < w || srcH < absh) memset( dstData, 0, dstLine * absh);
      if ( h < 0)
      {
         dstData += dstLine * ( yMin - 1);
         dstLine = -dstLine;
      }
      for ( y = 0; y < yMin; y++, srcData += srcLine, dstData += dstLine) 
         memcpy( dstData, srcData, xMin);
      return;
   }

/* y-only stretch case */
   if ( !xStretch && yStretch && ( w > 0))
   {
      int xMin = (( type & imBPP) < 8) ? 
         ( srcLine > dstLine) ? dstLine : srcLine :
         (((( srcW > absw) ? absw : srcW) * ( type & imBPP)) / 8);
      count. l = 0;
      if ( srcW < w) memset( dstData, 0, dstLine * absh);
      if ( h < 0)
      {
         dstData += dstLine * ( absh - 1);
         dstLine = -dstLine;
      }
      if ( absh < srcH)
      {
         ystep. l = (double) absh / srcH * UINT16_PRECISION;
         memcpy( dstData, srcData, xMin);
         dstData += dstLine;
         for ( i = 0; i < srcH; i++)
         {
            if ( count. i.i > last)
            {
               memcpy( dstData, srcData, xMin);
               dstData += dstLine;
               last = count.i.i;
            }
            count. l += ystep. l;
            srcData += srcLine;
         }
      } else {
         ystep. l = (double) srcH / absh * UINT16_PRECISION;
         for ( i = 0; i < absh; i++)
         {
            if ( count.i.i > last)
            {
               srcData += srcLine;
               last = count.i.i;
            }
            count. l += ystep. l;
            memcpy( dstData, srcData, xMin);
            dstData  += dstLine;
         }
      }
      return;
   }

/* general actions for x-scaling */
   count. l = 0;
   count. l = 0;
   if ( srcW < absw || srcH < absh || ( type & imBPP) == imNibble)
      memset( dstData, 0, dstLine * absh);
   if ( absw < srcW)
      xstep. l = (double) absw / srcW * UINT16_PRECISION;
   else
      xstep. l = (double) srcW / absw * UINT16_PRECISION;
   switch( type)
   {
      case imMono:     case imBW:
         proc = ( PStretchProc)(( srcW > absw) ? bs_mono_in : bs_mono_out);  break;
      case imNibble:   case imNibble|imGrayScale:
         proc = ( PStretchProc)(( srcW > absw) ? bs_nibble_in : bs_nibble_out);     break;
      case imByte:     case im256:
         proc = ( PStretchProc)(( srcW > absw) ? bs_uint8_t_in : bs_uint8_t_out);   break;
      case imRGB:      case imRGB|imGrayScale:
         proc = ( PStretchProc)(( srcW > absw) ? bs_RGBColor_in : bs_RGBColor_out); break;
      case imShort:
         proc = ( PStretchProc)(( srcW > absw) ? bs_int16_t_in : bs_int16_t_out);   break;
      case imLong:
         proc = ( PStretchProc)(( srcW > absw) ? bs_int32_t_in : bs_int32_t_out);   break;
      case imFloat:
         proc = ( PStretchProc)(( srcW > absw) ? bs_float_in : bs_float_out);       break;
      case imDouble:
         proc = ( PStretchProc)(( srcW > absw) ? bs_double_in : bs_double_out);     break;
      case imComplex:  case imTrigComplex:
         proc = ( PStretchProc)(( srcW > absw) ? bs_Complex_in : bs_Complex_out);   break;
      case imDComplex: case imTrigDComplex:
         proc = ( PStretchProc)(( srcW > absw) ? bs_DComplex_in : bs_DComplex_out); break;
      default:
         return;
   }

/* no vertical stretch case */
   if ( !yStretch || ( srcH == -h))
   {
      if ( h < 0)
      {
         dstData += dstLine * ( yMin - 1);
         dstLine = -dstLine;
      }
      for ( i = 0; i < srcH; i++, srcData += srcLine, dstData += dstLine)
         proc( srcData, dstData, srcW, w, absw, xstep.l);
      return;
   }

/* general case */
   if ( h < 0)
   {
      dstData += dstLine * ( absh - 1);
      dstLine = -dstLine;
   }
   if ( absh < srcH)
   {
      ystep. l = (double) absh / srcH * UINT16_PRECISION;
      proc( srcData, dstData, srcW, w, absw, xstep.l);
      dstData += dstLine;
      for ( i = 0; i < srcH; i++)
      {
         if ( count. i.i > last)
         {
            proc( srcData, dstData, srcW, w, absw, xstep.l);
            dstData += dstLine;
            last = count.i.i;
         }
         count. l += ystep. l;
         srcData += srcLine;
      }
   } else {
      ystep. l = (double) srcH / absh * UINT16_PRECISION;
      for ( i = 0; i < absh; i++)
      {
         if ( count.i.i > last)
         {
            srcData += srcLine;
            last = count.i.i;
         }
         count. l += ystep. l;
         if ( srcLast == srcData) {
            memcpy( dstData, dstData - dstLine, dstLine < 0 ? -dstLine : dstLine);
         } else {
            proc( srcData, dstData, srcW, w, absw, xstep.l);
            srcLast = srcData;
         }
         dstData += dstLine;
      }
   }
}

/* Resizing with filters - stolen from ImageMagick MagickCore/resize.c */

typedef double FilterFunc( const double x );
typedef struct {
   unsigned int id;
   FilterFunc * filter;
   unsigned int support;
} FilterRec;

#define PI 3.14159265358

static double 
filter_sinc(const double x) {
  /*
    Scaled sinc(x) function using a trig call:
      sinc(x) == sin(pi x)/(pi x).
  */
  if (x != 0.0) {
     const double alpha= (double) (PI * x);
     return (sin((double) alpha) / alpha);
  }
  return ((double) 1.0);
}

static FilterRec filters[] = {
   { istSinc, filter_sinc, 4 }
};

static void
stretch_horizontal( FilterFunc * filter, double * contributions, double support, int channels, Byte * src_data, int src_w, int src_h, Byte * dst_data, int dst_w, int dst_h, double x_factor)
{
   int x, y, c, src_line_size, dst_line_size, x_lim;
   
   src_line_size = LINE_SIZE(src_w * channels, imDouble);
   dst_line_size = LINE_SIZE(dst_w * channels, imDouble);
 
   x_lim = src_w;
   for (x = 0; x < dst_w; x++) {
      double bisect, density;
      int n, start, stop, offset;
      Byte *src, *dst;

      bisect = (double) (x + 0.5) / x_factor;
      start  = bisect - support + 0.5;
      if ( start < 0 ) start = 0;
      stop   = bisect + support + 0.5;
      if ( stop > src_w ) stop = src_w;

      density = 0.0;
      for (n = 0; n < (stop-start); n++) {
         contributions[n] = filter(((double) (start+n)-bisect+0.5));
         density += contributions[n];
      }

      if ( density != 0.0 && density != 1.0 ) {
         int i;
         for ( i = 0; i < n; i++) contributions[i] /= density;
      }
  
      dst = dst_data + x     * channels * sizeof(double);
      src = src_data + start * channels * sizeof(double);
      for ( c = 0; c < channels; c++, src += sizeof(double), dst += sizeof(double)) {
         Byte *src_y = src, *dst_y = dst;
         for ( y = 0; y < dst_h; y++, src_y += src_line_size, dst_y += dst_line_size ) {
            register int j;
            register double pixel = 0.0, *src_j = (double*)src_y;
            for ( j = 0; j < n; j++, src_j += channels)
               pixel += contributions[j] * *src_j;
            *((double*)dst_y) = pixel;
         }
      }
   }
}

static void
stretch_vertical( FilterFunc * filter, double * contributions, double support, Byte * src_data, int src_w, int src_h, Byte * dst_data, int dst_w, int dst_h, double y_factor)
{
   int x, y, c, src_line_size, dst_line_size;
   
   src_line_size = LINE_SIZE(src_w, imDouble);
   dst_line_size = LINE_SIZE(dst_w, imDouble);

   for ( y = 0; y < dst_h; y++) {
      double bisect, density;
      int n, start, stop;
      Byte * src, * dst;

      bisect = (double) (y + 0.5) / y_factor;
      start  = bisect - support +0.5;
      if ( start < 0 ) start = 0;
      stop   = bisect + support +0.5;
      if ( stop > src_h ) stop = src_h;

      density = 0.0;
      for (n = 0; n < (stop-start); n++) {
         contributions[n] = filter(((double) (start+n)-bisect+0.5));
         density += contributions[n];
      }

      if ( density != 0.0 && density != 1.0 ) {
         int i;
         for ( i = 0; i < n; i++) contributions[i] /= density;
      }
 
      src = src_data + start * src_line_size;
      dst = dst_data;
      for ( x = 0; x < dst_w; x++, src += sizeof(double), dst += sizeof(double)) {
         int j;
         double pixel = 0.0;
	 Byte * src_y = src;
         for ( j = 0; j < n; j++, src_y += src_line_size)
            pixel += contributions[j] * *((double*)(src_y));
         *((double*)(dst)) = pixel;
      }

      dst_data += dst_line_size;
   }
}

Bool
ic_stretch_filtered( Handle self, int w, int h, int scaling )
{
   int absw, absh, channels, target_ls, target_ds, target_type, org_type, channel2_type, fw, fh, flw, i, support_size;
   Bool mirror_x, mirror_y;
   double factor_x, factor_y, scale_x, scale_y, *contributions, support_x, support_y ;
   Byte * target_data, * filter_data;
   FilterRec * filter = NULL;

   for ( i = 0; i < sizeof(filters) / sizeof(FilterRec); i++) {
      if ( filters[i]. id == scaling ) {
         filter = &filters[i];
         break;
      }
   }
   if ( !filter ) 
      croak("no appropriate scaling filter found");

   org_type = var-> type;
   absw = abs(w);
   absh = abs(h);
   mirror_x = w < 0;
   mirror_y = h < 0;

   /* if it's cheaper to mirror before the conversion, do it */
   if ( mirror_y && var-> h < h ) {
      img_mirror( self, 1 );
      mirror_y = 0;
   }
   
   /* convert to double, and use last chance to mirror horizontally */
   switch (var-> type & imCategory) {
   case imColor: 
      channels = 3;
      target_type = imRGB;
      break;
   case imComplexNumber:
      channels = 2;
      target_type = imDComplex;
      break;
   case imTrigComplexNumber:
      channels = 2;
      target_type = imTrigDComplex;
      break;
   default:
      channels = 1;
      target_type = imDouble;
   }

   if ( var-> type != target_type) my-> set_type( self, target_type );
   if ( mirror_x ) {
      if ( var-> lineSize < LINE_SIZE( absw, target_type)) {
         img_mirror( self, 0 );
         mirror_x = 0;
      }
   }

   /* convert to multi-channel imDouble structures */
   if ( var-> type == imRGB ) {
      var-> type = imByte;
      var-> w *= 3;
      absw *= 3;
      my-> set_type( self, imDouble );
   }
   if ( channels == 2 ) {
      var-> w *= 2;
      absw *= 2;
      channel2_type = var-> type;
      var-> type = imDouble;
   }

   /* allocate space for semi-filtered and target data */
   factor_x = (double) absw / (double) var-> w;
   factor_y = (double) absh / (double) var-> h;
   if (factor_y > factor_y) {
      fw = absw;
      fh = var-> h;
   } else {
      fw = var-> w;
      fh = absh;
   }
   flw = LINE_SIZE( fw, imDouble);
   if ( !( filter_data = malloc( flw * fh )))
      croak("not enough memory: %d bytes", flw * fh);
   target_ls = LINE_SIZE( absw, imDouble);
   target_ds = absh * target_ls;
   if ( !( target_data = malloc( target_ds ))) {
      free( filter_data );
      croak("not enough memory: %d bytes", target_ds);
   }

   scale_x = 1.0 / factor_x;
   if ( scale_x < 1.0 ) scale_x = 1.0;
   scale_y = 1.0 / factor_y;
   if ( scale_y < 1.0 ) scale_y = 1.0;
   support_x = scale_x * filter-> support;
   support_y = scale_y * filter-> support;
   /* Support too small even for nearest neighbour: Reduce to point sampling.  */
   if (support_x < 0.5) support_x = (double) 0.5;
   if (support_y < 0.5) support_y = (double) 0.5;
   support_size = (int)(sizeof(double) * 2.0 * (( support_x < support_y ) ? support_y : support_x) * 3.0);
   if (!(contributions = malloc(support_size))) {
      free( filter_data );
      free( target_data );
      croak("not enough memory: %d bytes", support_size);
   }

   /* stretch */
   if (factor_x > factor_y) {
       stretch_horizontal( filter->filter, contributions, support_x, channels, var-> data, var-> w / channels, var-> h, filter_data, fw / channels, fh, factor_x);
       stretch_vertical  ( filter->filter, contributions, support_y, filter_data, fw, fh, target_data, absw, absh, factor_y );
   } else {
       stretch_vertical  ( filter->filter, contributions, support_y, var-> data, var-> w, var-> h, filter_data, fw, fh, factor_y);
       stretch_horizontal( filter->filter, contributions, support_x, channels, filter_data, fw / channels, fh, target_data, absw / channels, absh, factor_x);
   }
   free( contributions );
   free( filter_data );

   /* clamp values */
   if ( channels != 2 && (org_type & (imRealNumber|imComplexNumber|imTrigComplexNumber)) == 0) {
      register double min, max;
      register double * t = (double *) target_data;
      register int x;
      int y, ls = LINE_SIZE( absw, imDouble );

      switch ( org_type & imBPP ) {
      case 16:
         min = INT16_MIN;
         max = INT16_MAX;
         break;
      case 32:
         min = INT32_MIN;
         max = INT32_MAX;
         break;
      default:
         min = 0;
         max = 255;
         break;
      }
      for ( y = 0; y < absh; y++, t = (double*)((Byte*)t + ls)) {
         for ( x = 0; x < absw; x++) {
            if ( t[x] < min ) 
               t[x] = min;
            else if ( t[x] > max ) 
               t[x] = max;
         }
      }
   }

   /* convert back */
   if ( channels == 2 ) {
      absw /= 2;
      var-> w /= 2;
      var-> type = channel2_type;
   }
   var-> w = absw;
   var-> h = absh;
   var-> lineSize = LINE_SIZE( absw, var-> type );
   var-> dataSize = var-> lineSize * absh;
   free( var-> data );
   var-> data = target_data;
   if ( channels == 3 ) {
      my-> set_type( self, imByte );
      var-> type = imRGB;
      var-> w /= 3;
      w /= 3;
   }
   if ( is_opt( optPreserveType) && var-> type != org_type )
      my-> set_type( self, org_type );
   if ( mirror_x ) img_mirror( self, 0 );
   if ( mirror_y ) img_mirror( self, 1 );
}

#ifdef __cplusplus
}
#endif
