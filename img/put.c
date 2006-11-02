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
      int type     = PImage( src)-> type;
      void *self   = PImage( src)-> self; 
      RGBColor palette[2];
      memcpy( palette, PImage( src)-> palette, 6);
      memcpy( PImage( src)-> palette, stdmono_palette, 6);
      PImage( src)-> self     =  CImage;
      PImage( src)-> type     =  imbpp1 | imGrayScale; 
      PImage( src)-> data     =  PIcon( src)-> mask;
      PImage( src)-> lineSize =  PIcon( src)-> maskLine;
      PImage( src)-> dataSize =  PIcon( src)-> maskSize;
      PImage( src)-> palSize  =  2;
      img_put( dest, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH, ropAndPut);
      PImage( src)-> self     = self;
      PImage( src)-> type     = type;
      PImage( src)-> data     = data; 
      PImage( src)-> lineSize = lineSize; 
      PImage( src)-> dataSize = dataSize; 
      PImage( src)-> palSize  = palSize;
      memcpy( PImage( src)-> palette, palette, 6);
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
         int mask  = (1 << type) - 1;
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
         for ( sz = 0; sz < i-> h; sz++, dj += j-> lineSize, di += i-> lineSize) 
            bc_byte_mono_cr( dj, di, i-> w, colorref);
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
      PBitBltProc proc;

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


#ifdef __cplusplus
}
#endif

