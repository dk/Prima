#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void BitBltProc( Byte * src, Byte * dst, int count);
typedef BitBltProc *PBitBltProc;

static void
bitblt_copy( Byte * src, Byte * dst, int count)
{
   memcpy( dst, src, count);
}

static void
bitblt_move( Byte * src, Byte * dst, int count)
{
   memmove( dst, src, count);
}

static void
bitblt_or( Byte * src, Byte * dst, int count)
{
   while ( count--) *(dst++) |= *(src++);
}

static void
bitblt_and( Byte * src, Byte * dst, int count)
{
   while ( count--) *(dst++) &= *(src++);
}

static void
bitblt_xor( Byte * src, Byte * dst, int count)
{
   while ( count--) *(dst++) ^= *(src++);
}

static void
bitblt_not( Byte * src, Byte * dst, int count)
{
   while ( count--) *(dst++) = ~(*(src++));
}

static void
bitblt_notdstand( Byte * src, Byte * dst, int count)
{
   while ( count--) {
      *dst = ~(*dst) & (*(src++));
      dst++;
   }
}

static void
bitblt_notdstor( Byte * src, Byte * dst, int count)
{
   while ( count--) {
      *dst = ~(*dst) | (*(src++));
      dst++;
   }
}

static void
bitblt_notsrcand( Byte * src, Byte * dst, int count)
{
   while ( count--) *(dst++) &= ~(*(src++));
}

static void
bitblt_notsrcor( Byte * src, Byte * dst, int count)
{
   while ( count--) *(dst++) |= ~(*(src++));
}

static void
bitblt_notxor( Byte * src, Byte * dst, int count)
{
   while ( count--) {
      *dst = ~( *(src++) ^ (*dst));
      dst++;
   }
}

static void
bitblt_notand( Byte * src, Byte * dst, int count)
{
   while ( count--) {
      *dst = ~( *(src++) & (*dst));
      dst++;
   }
}

static void
bitblt_notor( Byte * src, Byte * dst, int count)
{
   while ( count--) {
      *dst = ~( *(src++) | (*dst));
      dst++;
   }
}

static void
bitblt_black( Byte * src, Byte * dst, int count)
{
   memset( dst, 0, count);
}

static void
bitblt_white( Byte * src, Byte * dst, int count)
{
   memset( dst, 0xff, count);
}

static void
bitblt_invert( Byte * src, Byte * dst, int count)
{
   while ( count--) {
      *dst = ~(*dst);
      dst++;
   }
}

static PBitBltProc
find_blt_proc( int rop )
{
   PBitBltProc proc = NULL;
   switch ( rop) {
   case ropCopyPut:
      proc = bitblt_copy;
      break;
   case ropAndPut:
      proc = bitblt_and;
      break;
   case ropOrPut:
      proc = bitblt_or;
      break;
   case ropXorPut:
      proc = bitblt_xor;
      break;
   case ropNotPut:
      proc = bitblt_not;
      break;
   case ropNotDestAnd:
      proc = bitblt_notdstand;
      break;
   case ropNotDestOr:
      proc = bitblt_notdstor;
      break;
   case ropNotSrcAnd:
      proc = bitblt_notsrcand;
      break;
   case ropNotSrcOr:
      proc = bitblt_notsrcor;
      break;
   case ropNotXor:
      proc = bitblt_notxor;
      break;
   case ropNotAnd:
      proc = bitblt_notand;
      break;
   case ropNotOr:
      proc = bitblt_notor;
      break;
   case ropBlackness:
      proc = bitblt_black;
      break;
   case ropWhiteness:
      proc = bitblt_white;
      break;
   case ropInvert:
      proc = bitblt_invert;
      break;
   default:
      proc = bitblt_copy;
   }
   return proc;
}

Bool 
img_put( Handle dest, Handle src, int dstX, int dstY, int srcX, int srcY, int dstW, int dstH, int srcW, int srcH, int rop)
{
   Point srcSz, dstSz;
   int asrcW, asrcH;
   Bool newObject = false;

   if ( dest == nilHandle || src == nilHandle) return false;
   if ( rop == ropNoOper) return false;

   if ( kind_of( src, CIcon)) {
      /* since src is always treated as read-only, 
         employ a nasty hack here, re-assigning
         all mask values to data */
      Byte * data  = PImage( src)-> data;
      int dataSize = PImage( src)-> dataSize; 
      int lineSize = PImage( src)-> lineSize; 
      int palSize  = PImage( src)-> palSize; 
      int palSize2;
      int type     = PImage( src)-> type;
      void *self   = PImage( src)-> self; 
      RGBColor palette[256];
      if ( PIcon( src)-> maskType == imbpp1) 
         memcpy( PImage( src)-> palette, stdmono_palette, palSize2 = 6);
      else
         memcpy( PImage( src)-> palette, std256gray_palette, palSize2 = 768);
      memcpy( palette, PImage( src)-> palette, palSize2);
      PImage( src)-> self     =  CImage;
      PImage( src)-> type     =  PIcon( src)-> maskType | imGrayScale; 
      PImage( src)-> data     =  PIcon( src)-> mask;
      PImage( src)-> lineSize =  PIcon( src)-> maskLine;
      PImage( src)-> dataSize =  PIcon( src)-> maskSize;
      PImage( src)-> palSize  =  ( PIcon( src)-> maskType == imbpp1) ? 2 : 256;
      img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropAndPut);
      PImage( src)-> self     = self;
      PImage( src)-> type     = type;
      PImage( src)-> data     = data; 
      PImage( src)-> lineSize = lineSize; 
      PImage( src)-> dataSize = dataSize; 
      PImage( src)-> palSize  = palSize;
      memcpy( PImage( src)-> palette, palette, palSize2);
      rop = ropXorPut;
   }  
   
   srcSz. x = PImage(src)-> w;
   srcSz. y = PImage(src)-> h;
   dstSz. x = PImage(dest)-> w;
   dstSz. y = PImage(dest)-> h;

   if ( dstW < 0) {
      dstW = abs( dstW);
      srcW = -srcW;
   }
   if ( dstH < 0) {
      dstH = abs( dstH);
      srcH = -srcH;
   }
   
   asrcW = abs( srcW);
   asrcH = abs( srcH);

   if ( srcX >= srcSz. x || srcX + srcW <= 0 ||
        srcY >= srcSz. y || srcY + srcH <= 0 ||
        dstX >= dstSz. x || dstX + dstW <= 0 ||
        dstY >= dstSz. y || dstY + dstH <= 0)
      return true;

   /* check if we can do it without expensive scalings and extractions */
   if ( 
         ( srcW == dstW) && ( srcH == dstH) &&
         ( srcX >= 0) && ( srcY >= 0) && ( srcX + srcW <= srcSz. x) && ( srcY + srcH <= srcSz. y) 
      ) 
      goto NOSCALE;

   if ( srcX != 0 || srcY != 0 || asrcW != srcSz. x || asrcH != srcSz. y) {
     /* extract source rectangle */
      Handle x;
      int ssx = srcX, ssy = srcY, ssw = asrcW, ssh = asrcH;
      if ( ssx < 0) {
         ssw += ssx;
         ssx = 0;
      }
      if ( ssy < 0) {
         ssh += ssy;
         ssy = 0;
      }
      x = CImage( src)-> extract( src, ssx, ssy, ssw, ssh);
      if ( !x) return false;

      if ( srcX < 0 || srcY < 0 || srcX + asrcW >= srcSz. x || srcY + asrcH > srcSz. y) {
         HV * profile;
         Handle dx;
         int dsx = 0, dsy = 0, dsw = PImage(x)-> w, dsh = PImage(x)-> h, type = PImage( dest)-> type;

         if ( asrcW != srcW || asrcH != srcH) { /* reverse before application */
            CImage( x)-> stretch( x, srcW, srcH);
            srcW = asrcW;
            srcH = asrcH;
            if ( PImage(x)-> w != asrcW || PImage(x)-> h != asrcH) {
               Object_destroy( x);
               return true;
            }
         }

         if (( type & imBPP) < 8) type = imbpp8;

         profile = newHV();
         pset_i( type,        type);
         pset_i( width,       asrcW);
         pset_i( height,      asrcH);
         pset_i( conversion,  PImage( src)-> conversion);
         dx = Object_create( "Prima::Image", profile);
         sv_free((SV*)profile);
         if ( !dx) {
            Object_destroy( x);
            return false;
         }
         if ( PImage( dx)-> palSize > 0) {
            PImage( dx)-> palSize = PImage( x)-> palSize;
            memcpy( PImage( dx)-> palette, PImage( x)-> palette, 768);
         }
         memset( PImage( dx)-> data, 0, PImage( dx)-> dataSize);

         if ( srcX < 0) dsx = asrcW - dsw;
         if ( srcY < 0) dsy = asrcH - dsh;
         img_put( dx, x, dsx, dsy, 0, 0, dsw, dsh, dsw, dsh, ropCopyPut);
         Object_destroy( x);
         x = dx;
      }

      src = x;
      newObject = true;
      srcX = srcY = 0;
   } 

   if ( srcW != dstW || srcH != dstH) {
      /* stretch & reverse */
      if ( !newObject) {
         src = CImage( src)-> dup( src);
         if ( !src) goto EXIT;
         newObject = true;
      }
      if ( srcW != asrcW) { 
         dstW = -dstW;
         srcW = asrcW;
      }
      if ( srcH != asrcH) { 
         dstH = -dstH;
         srcH = asrcH;
      }
      CImage(src)-> stretch( src, dstW, dstH);
      if ( PImage(src)-> w != dstW || PImage(src)-> h != dstH) goto EXIT;
      dstW = abs( dstW);
      dstH = abs( dstH);
   }

NOSCALE:   

   if (( PImage( dest)-> type & imBPP) < 8) {
      PImage i = ( PImage) dest;
      int type = i-> type;
      if (rop != ropCopyPut || i-> conversion == ictNone) { 
         Handle b8 = i-> self-> dup( dest);
         PImage j  = ( PImage) b8;
         int mask  = (1 << (type & imBPP)) - 1;
         int sz;
         Byte *dj, *di;
         Byte colorref[256];
         j-> self-> reset( b8, imbpp8, nil, 0);
         sz = j-> dataSize;
         dj = j-> data;
         /* change 0/1 to 0x000/0xfff for correct masking */
         while ( sz--) {
            if ( *dj == mask) *dj = 0xff;
            dj++;
         }
         img_put( b8, src, dstX, dstY, 0, 0, dstW, dstH, PImage(src)-> w, PImage(src)-> h, rop);
         for ( sz = 0; sz < 256; sz++) colorref[sz] = ( sz > mask) ? mask : sz;
         dj = j-> data;
         di = i-> data;

         for ( sz = 0; sz < i-> h; sz++, dj += j-> lineSize, di += i-> lineSize) {
            if (( type & imBPP) == 1) 
               bc_byte_mono_cr( dj, di, i-> w, colorref);
            else
               bc_byte_nibble_cr( dj, di, i-> w, colorref);
         }
         Object_destroy( b8);
      } else {
         int conv = i-> conversion;
         i-> conversion = PImage( src)-> conversion;
         i-> self-> reset( dest, imbpp8, nil, 0);
         img_put( dest, src, dstX, dstY, 0, 0, dstW, dstH, PImage(src)-> w, PImage(src)-> h, rop);
         i-> self-> reset( dest, type, nil, 0);
         i-> conversion = conv;
      }
      goto EXIT;
   } 

   if ( PImage( dest)-> type != PImage( src)-> type) {
      int type = PImage( src)-> type & imBPP;
      int mask = (1 << type) - 1;
      /* equalize type */
      if ( !newObject) {
         src = CImage( src)-> dup( src);
         if ( !src) goto EXIT;
         newObject = true;
      }
      CImage( src)-> reset( src, PImage( dest)-> type, nil, 0);
      if ( type < 8 && rop != ropCopyPut) { 
         /* change 0/1 to 0x000/0xfff for correct masking */
         int sz   = PImage( src)-> dataSize;
         Byte * d = PImage( src)-> data;
         while ( sz--) {
            if ( *d == mask) *d = 0xff;
            d++;
         }
         memset( PImage( src)-> palette + 255, 0xff, sizeof(RGBColor));
      }
   }

   if ( PImage( dest)-> type == imbpp8) {
      /* equalize palette */
      Byte colorref[256], *s;
      int sz, i = PImage( src)-> dataSize;
      if ( !newObject) {
         src = CImage( src)-> dup( src);
         if ( !src) goto EXIT;
         newObject = true;
      }
      cm_fill_colorref( 
         PImage( src)-> palette, PImage( src)-> palSize,
         PImage( dest)-> palette, PImage( dest)-> palSize,
         colorref);
      s = PImage( src)-> data;
      /* identity transform for padded ( 1->xfff, see above ) pixels */
      for ( sz = PImage( src)-> palSize; sz < 256; sz++)
         colorref[sz] = sz;
      while ( i--) {
         *s = colorref[ *s];
         s++;
      }
   }

   if ( dstX < 0 || dstY < 0 || dstX + dstW >= dstSz. x || dstY + dstH >= dstSz. y) {
      /* adjust destination rectangle */
      if ( dstX < 0) {
         dstW += dstX;
         srcX -= dstX;
         dstX = 0;
      }
      if ( dstY < 0) {
         dstH += dstY;
         srcY -= dstY;
         dstY = 0;
      }
      if ( dstX + dstW > dstSz. x)
         dstW = dstSz. x - dstX;
      if ( dstY + dstH > dstSz. y) 
         dstH = dstSz. y - dstY;
   }

   /* checks done, do put_image */
   {
      int  y, dyd, dys, count, pix;
      Byte *dptr, *sptr;
      PBitBltProc proc = find_blt_proc(rop);

      pix = ( PImage( dest)-> type & imBPP ) / 8;
      dyd = PImage( dest)-> lineSize;
      dys = PImage( src)-> lineSize;
      sptr = PImage( src )-> data + dys * srcY + pix * srcX;
      dptr = PImage( dest)-> data + dyd * dstY + pix * dstX;
      count = dstW * pix;

      if ( proc == bitblt_copy && dest == src) /* incredible */
         proc = bitblt_move;
      
      for ( y = 0; y < dstH; y++, sptr += dys, dptr += dyd) 
         proc( sptr, dptr, count);
   }

EXIT:
   if ( newObject) Object_destroy( src);

   return true;
}

void 
img_bar( Handle dest, int x, int y, int w, int h, int rop, void * color)
{
   PImage i     = (PImage) dest;
   Byte * data  = i-> data;
   int dataSize = i-> dataSize; 
   int lineSize = i-> lineSize; 
   int palSize  = i-> palSize; 
   int type     = i-> type;
   int pixSize  = (type & imBPP) / 8;
   PBitBltProc proc;
#define BLT_BUFSIZE 1024
   Byte blt_buffer[BLT_BUFSIZE];
   int j, k, offset, blt_bytes, blt_step;
   Byte filler, lmask, rmask, *p, *q;
  
   if ( x < 0 ) {
      w += x;
      x = 0;
   }
   if ( y < 0 ) {
      h += y;
      y = 0;
   }
   if ( x + w > i->w ) w = i->w - x;
   if ( y + h > i->h ) h = i->h - y;
   if ( w <= 0 || h <= 0 ) return;

   switch ( type & imBPP) {
   case imbpp1:
      filler = (*((Byte*)color)) ? 255 : 0;
      blt_bytes = (( x + w - 1) >> 3) - (x >> 3) + 1;
      blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes;
      memset( blt_buffer, filler, blt_step);
      lmask = ( x & 7 ) ? 255 << ( 8 - x & 7) : 0;
      rmask = (( x + w) & 7 ) ? 255 >> ((x + w) & 7) : 0;
      offset = x >> 3;
      break;
   case imbpp4:
      filler = (*((Byte*)color)) * 17;
      blt_bytes = (( x + w - 1) >> 1) - (x >> 1) + 1;
      blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes;
      memset( blt_buffer, filler, blt_step);
      lmask = ( x & 1 )       ? 0xf0 : 0;
      rmask = (( x + w) & 1 ) ? 0x0f : 0;
      offset = x >> 1;
      break;
   case imbpp8:
      filler = (*((Byte*)color));
      blt_bytes = w;
      blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE : blt_bytes;
      memset( blt_buffer, filler, blt_step);
      lmask = rmask = 0;
      offset = x;
      break;
   default:
      blt_bytes = w * pixSize;
      blt_step = (blt_bytes > BLT_BUFSIZE) ? BLT_BUFSIZE / pixSize : w;
      for ( j = 0, p = blt_buffer; j < blt_step; j ++ )
         for ( k = 0, q = (Byte*) color; k < pixSize; k++ )
            *(p++) = *(q++);
      blt_step *= pixSize;
      lmask = rmask = 0;
      offset = x * pixSize;
   }

   data += lineSize * y + offset;
   proc = find_blt_proc(rop);

#define DEBUG_ 1
#ifdef DEBUG
   warn("%d/%d, blt_bytes:%d, blt_step:%d, lmask:%02x, rmask:%02x, buf:%02x%02x%02x%02x\n", 
   	x, w, blt_bytes, blt_step, lmask, rmask, blt_buffer[0], blt_buffer[1], blt_buffer[2], blt_buffer[3]);
#endif	

   for ( j = 0; j < h; j++) {
      int bytes = blt_bytes;
      Byte lsave = *data, rsave = data[blt_bytes - 1], *p = data;
      while ( bytes > 0 ) {
         proc( blt_buffer, p, ( bytes > blt_step ) ? blt_step : bytes );
         bytes -= blt_step;
         p += blt_step;
      }
      if ( lmask ) *data = (lsave & lmask) | (*data & ~lmask);
      if ( rmask ) data[blt_bytes-1] = (rsave & rmask) | (data[blt_bytes-1] & ~rmask);
      data -= lineSize;
   }
}

#ifdef __cplusplus
}
#endif

