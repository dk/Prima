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

/***********************************************************/
/*                                                         */
/*  System dependent graphics (unix, x11)                  */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Image.h"

#define SORT(a,b)	{ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)	(XX-> size. y + XX-> menuHeight - (a) - 1)
#define SHIFT(a,b)	{ (a) += XX-> gtransform. x + XX-> btransform. x; \
                           (b) += XX-> gtransform. y + XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)
#define RANGE4(a,b,c,d) RANGE(a) RANGE(b) RANGE(c) RANGE(d)
#define REVERSE_BYTES_32(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))
#define REVERSE_BYTES_24(x) ((((x)&0xff)<<16) | ((x)&0xff00) | (((x)&0xff0000)>>8))
#define REVERSE_BYTES_16(x) ((((x)&0xff)<<8 ) | (((x)&0xff00)>>8))

#define COLOR_R16(x) (((x)>>8)&0xFF00)
#define COLOR_G16(x) ((x)&0xFF00)
#define COLOR_B16(x) (((x)<<8)&0xFF00)

static int rop_map[] = {
   GXcopy	/* ropCopyPut */,		/* dest  = src */
   GXxor	/* ropXorPut */,		/* dest ^= src */
   GXand	/* ropAndPut */,		/* dest &= src */
   GXor		/* ropOrPut */,			/* dest |= src */
   GXcopyInverted /* ropNotPut */,		/* dest = !src */
   GXnoop	/* ropNotBlack */,		/* dest = (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNotDestXor */,		/* dest = (!dest) ^ src */	/* XXX */
   GXandReverse	/* ropNotDestAnd */,		/* dest = (!dest) & src */
   GXorReverse	/* ropNotDestOr */,		/* dest = (!dest) | src */
   GXequiv	/* ropNotSrcXor */,		/* dest ^= !src */
   GXandInverted /* ropNotSrcAnd */,		/* dest &= !src */
   GXorInverted	/* ropNotSrcOr */,		/* dest |= !src */
   GXnoop	/* ropNotXor */,		/* dest = !(src ^ dest) */	/* XXX */
   GXnand	/* ropNotAnd */,		/* dest = !(src & dest) */
   GXnor	/* ropNotOr */,			/* dest = !(src | dest) */
   GXnoop	/* ropNotBlackXor */,		/* dest ^= (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNotBlackAnd */,		/* dest &= (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNotBlackOr */,		/* dest |= (src <> 0) ? src */	/* XXX */
   GXnoop	/* ropNoOper */,		/* dest = dest */
   GXclear	/* ropBlackness */,		/* dest = 0 */
   GXset	/* ropWhiteness */,		/* dest = white */
   GXinvert	/* ropInvert */,		/* dest = !dest */
   GXnoop	/* ropPattern */,		/* dest = pattern */		/* YYY */
   GXnoop	/* ropXorPattern */,		/* dest ^= pattern */		/* YYY */
   GXnoop	/* ropAndPattern */,		/* dest &= pattern */		/* YYY */
   GXnoop	/* ropOrPattern */,		/* dest |= pattern */		/* YYY */
   GXnoop	/* ropNotSrcOrPat */,		/* dest |= pattern | (!src) */	/* YYY */
   GXnoop	/* ropSrcLeave */,		/* dest = (src != fore color) ? src : figa */	/* YYY */
   GXnoop	/* ropDestLeave */,		/* dest = (src != back color) ? src : figa */	/* YYY */
};

int
prima_rop_map( int rop)
{
   if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
      return GXnoop;
   else
      return rop_map[ rop];
}

void
prima_get_gc( PDrawableSysData selfxx)
{
   XGCValues gcv;
   Bool bitmap;
   struct gc_head *gc_pool;

   if ( XX-> gc && XX-> gcl) return;

   if ( XX-> gc || XX-> gcl) {
      warn( "UAG_010: internal error");
      return;
   }   

   bitmap = XT_IS_BITMAP(XX) ? true : false;
   gc_pool = bitmap ? &guts.bitmap_gc_pool : &guts.screen_gc_pool;
   XX->gcl = TAILQ_FIRST(gc_pool);
   if (XX->gcl)
      TAILQ_REMOVE(gc_pool, XX->gcl, gc_link);
   if (!XX->gcl) {
      XX-> gc = XCreateGC( DISP, bitmap ? XX-> gdrawable : guts. root, 0, &gcv);
      XCHECKPOINT;
      if (( XX->gcl = alloc1z( GCList))) 
         XX->gcl->gc = XX-> gc;
   }
   if ( XX-> gcl) XX->gc = XX->gcl->gc;
}

void
prima_release_gc( PDrawableSysData selfxx)
{
   Bool bitmap;
   struct gc_head *gc_pool;

   if ( XX-> gc) {
      if ( XX-> gcl == nil)
         warn( "UAG_011: internal error");
      bitmap = XT_IS_BITMAP(XX) ? true : false;
      gc_pool = bitmap ? &guts.bitmap_gc_pool : &guts.screen_gc_pool;
      if ( XX-> gcl) 
         TAILQ_INSERT_HEAD(gc_pool, XX->gcl, gc_link);
      XX->gcl = nil;
      XX->gc = nil;
   } else {
      if ( XX-> gcl) {
         warn( "UAG_012: internal error");
         return;
      }
   }
}

void
prima_prepare_drawable_for_painting( Handle self, Bool inside_on_paint)
{
   DEFXX;
   unsigned long mask = VIRGIN_GC_MASK;
   int w, h;
   XRectangle r;

   XF_IN_PAINT(XX) = true;
   XX-> btransform. x = XX-> btransform. y = 0;
   if ( inside_on_paint && XX-> udrawable && is_opt( optBuffered) && !is_opt( optInDrawInfo) ) {
      if ( XX-> invalid_region) {
         XClipBox( XX-> invalid_region, &r);
         XX-> bsize. x = w = r. width;
         XX-> bsize. y = h = r. height;
         XX-> btransform. x = - r. x;
         XX-> btransform. y = r. y;
      } else {
         XX-> bsize. x = w = XX-> size. x;
         XX-> bsize. y = h = XX-> size. y;
      }
      if ( w <= 0 || h <= 0) goto Unbuffered;
      XX-> gdrawable = XCreatePixmap( DISP, XX-> udrawable, w, h, guts.depth);
      XCHECKPOINT;
      if (!XX-> gdrawable) goto Unbuffered;
   } else if ( XX-> udrawable && !XX-> gdrawable) {
Unbuffered:
      XX-> gdrawable = XX-> udrawable;
   }

   XX-> paint_rop = XX-> rop;
   XX-> paint_rop2 = XX-> rop2;
   XX-> flags. paint_base_line = XX-> flags. base_line;
   XX-> flags. paint_opaque    = XX-> flags. opaque;
   XX-> saved_font = PDrawable( self)-> font;
   XX-> line_width = XX-> gcv. line_width;
   XX-> gcv. clip_mask = None;
   XX-> gtransform = XX-> transform;

   prima_get_gc( XX);
   XX-> gcv. subwindow_mode = (self == application ? IncludeInferiors : ClipByChildren);
   
   XChangeGC( DISP, XX-> gc, mask, &XX-> gcv);
   XCHECKPOINT;
   
   if ( XX-> dashes) {
      XSetDashes( DISP, XX-> gc, 0, (char *) XX-> dashes, XX-> ndashes);
      XX-> paint_ndashes = XX-> ndashes;
      if (( XX-> paint_dashes = malloc( XX-> ndashes)))
         memcpy( XX-> paint_dashes, XX-> dashes, XX-> ndashes);
      XX-> line_style = ( XX-> paint_rop2 == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
   } else {
      XX-> paint_dashes = malloc(1);
      if ( XX-> ndashes < 0) {
	 XX-> paint_dashes[0] = '\0';
	 XX-> paint_ndashes = 0;
      } else {
	 XX-> paint_dashes[0] = '\1';
	 XX-> paint_ndashes = 1;
      }
      XX-> line_style = LineSolid;
   }

   XX-> clip_rect. x = 0;
   XX-> clip_rect. y = 0;
   XX-> clip_rect. width = XX-> size.x;
   XX-> clip_rect. height = XX-> size.y;
   if ( XX-> invalid_region && inside_on_paint && !is_opt( optInDrawInfo)) {
      if ( XX-> btransform. x != 0 || XX-> btransform. y != 0) {
         Region r = XCreateRegion();
         XUnionRegion( r, XX-> invalid_region, r);
         XOffsetRegion( r, XX-> btransform. x, -XX-> btransform. y);
         XSetRegion( DISP, XX-> gc, r);
         XDestroyRegion( r);
      } else {
         XSetRegion( DISP, XX-> gc, XX-> invalid_region);
      }
      XX-> paint_region = XX-> invalid_region;
      XX-> invalid_region = nil;
   }
   XX-> clip_mask_extent. x = XX-> clip_mask_extent. y = 0;

   apc_gp_set_color( self, XX-> saved_fore);
   apc_gp_set_back_color( self, XX-> saved_back);
   memcpy( XX-> saved_fill_pattern, XX-> fill_pattern, sizeof( FillPattern));
   XX-> fill_pattern[0]++; /* force  */
   apc_gp_set_fill_pattern( self, XX-> saved_fill_pattern);

   if ( !XX-> flags. reload_font && XX-> font && XX-> font-> id) {
      /* fprintf( stderr, "set font g: %s\n", XX-> font-> load_name); */
      XSetFont( DISP, XX-> gc, XX-> font-> id);
      XCHECKPOINT;
   } else {
      apc_gp_set_font( self, &PDrawable( self)-> font);
      XX-> flags. reload_font = false;
   }
}

void
prima_cleanup_drawable_after_painting( Handle self)
{
   DEFXX;
   if ( XX-> udrawable && XX-> udrawable != XX-> gdrawable && XX-> gdrawable && !is_opt( optInDrawInfo)) {
      if ( XX-> paint_region) {
         XSetRegion( DISP, XX-> gc, XX-> paint_region);
      } else {
         Region region = XCreateRegion();
         XRectangle r;
         r. x = -XX-> btransform. x;
         r. y = XX-> btransform. y;
         r. width = XX->bsize.x;
         r. height = XX->bsize.y;
         XUnionRectWithRegion( &r, region, region);
         XSetRegion( DISP, XX-> gc, region);
         XDestroyRegion( region);
      }
      XCHECKPOINT;
      XCopyArea( DISP, XX-> gdrawable, XX-> udrawable, XX-> gc,
                 0, 0,
                 XX-> bsize.x, XX-> bsize.y,
                 -XX-> btransform. x, XX-> btransform. y);
      XCHECKPOINT;
      XFreePixmap( DISP, XX-> gdrawable);
      XCHECKPOINT;
      XX-> gdrawable = XX-> udrawable;
      XX-> btransform. x = XX-> btransform. y = 0;
   }
   prima_release_gc(XX);
   memcpy( XX-> fill_pattern, XX-> saved_fill_pattern, sizeof( FillPattern));
   if ( XX-> font && ( --XX-> font-> refCnt <= 0)) {
      prima_free_rotated_entry( XX-> font);
      XX-> font-> refCnt = 0;
   }
   free(XX->paint_dashes);
   XX-> paint_dashes = nil;
   XX-> paint_ndashes = 0;
   XF_IN_PAINT(XX) = false;
   PDrawable( self)-> font = XX-> saved_font;
   if ( XX-> paint_region) {
      XDestroyRegion( XX-> paint_region);
      XX-> paint_region = nil;
   }
   XFlush(DISP);
}

#define PURE_FOREGROUND if (!XX->flags.brush_fore) {\
   XSetForeground( DISP, XX-> gc, XX-> fore. primary);\
   XX->flags.brush_fore=1;\
}\
XSetFillStyle( DISP, XX-> gc, FillSolid);\

Bool
prima_make_brush( DrawableSysData * XX, int colorIndex)
{
   Pixmap p;
   if ( XT_IS_BITMAP(XX) || ( guts. idepth == 1)) {
      int i;
      FillPattern mix, *p1, *p2;
      if ( colorIndex > 0) return false;
      p1 = &guts. ditherPatterns[ 64 - (XX-> fore. primary ? 64 : XX-> fore. balance)];
      p2 = &guts. ditherPatterns[ 64 - (XX-> back. primary ? 64 : XX-> back. balance)];
      for ( i = 0; i < sizeof( FillPattern); i++) 
         mix[i] = ((*p1)[i] & XX-> fill_pattern[i]) | ((*p2)[i] & ~XX-> fill_pattern[i]);
      XSetForeground( DISP, XX-> gc, 1);
      XSetBackground( DISP, XX-> gc, 0);
      XX-> flags. brush_fore = 0;
      XX-> flags. brush_back = 0;
      if (
          ( memcmp( mix, fillPatterns[ fpSolid], sizeof( FillPattern)) != 0) &&
          ( p = prima_get_hatch( &mix))
         ) {
         XSetStipple( DISP, XX-> gc, p);
         XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
      } else
         XSetFillStyle( DISP, XX-> gc, FillSolid);
   } else if ( XX-> flags. brush_null_hatch) {
      if ( colorIndex > 0) return false;
      if ( XX-> fore. balance) {
         p = prima_get_hatch( &guts. ditherPatterns[ XX-> fore. balance]);
         if ( p) {
            XSetStipple( DISP, XX-> gc, p);
            XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
            XSetBackground( DISP, XX-> gc, XX-> fore. secondary);
            XX-> flags. brush_back = 0;
         } else /* failure */
            XSetFillStyle( DISP, XX-> gc, FillSolid);
      } else 
         XSetFillStyle( DISP, XX-> gc, FillSolid);
      if (!XX->flags.brush_fore) {
         XSetForeground( DISP, XX-> gc, XX-> fore. primary);
         XX->flags.brush_fore = 1;
      }
   } else if ( XX->fore.balance == 0 && XX->back.balance == 0) {
      if ( colorIndex > 0) return false;
      
      p = prima_get_hatch( &XX-> fill_pattern);
      XSetFillStyle( DISP, XX-> gc, p ? FillOpaqueStippled : FillSolid);
      if ( p) XSetStipple( DISP, XX-> gc, p);
      if (!XX->flags.brush_fore) {
         XSetForeground( DISP, XX-> gc, XX-> fore. primary);
         XX->flags.brush_fore = 1;
      }
      if (p && !XX->flags.brush_back) {
         XSetBackground( DISP, XX-> gc, XX-> back. primary);
         XX->flags.brush_back = 1;
      }
   } else {
      switch ( colorIndex) {
      case 0: /* back mix */
         if ( XX-> back. balance) {
            p = prima_get_hatch( &guts. ditherPatterns[ XX-> back. balance]);
            if ( p) {
               XSetStipple( DISP, XX-> gc, p);
               XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
               XSetBackground( DISP, XX-> gc, XX-> back. secondary);
            } else  /* failure */
               XSetFillStyle( DISP, XX-> gc, FillSolid);
         } else 
            XSetFillStyle( DISP, XX-> gc, FillSolid);
         XSetForeground( DISP, XX-> gc, XX-> back. primary);
         XX-> flags. brush_back = 0;
         break;
      case 1: /* fore mix */
         if ( memcmp( XX-> fill_pattern, fillPatterns[fpEmpty], sizeof(FillPattern))==0)
            return false;
         if ( XX-> fore. balance) {
            int i;
            FillPattern fp;
            memcpy( &fp, &guts. ditherPatterns[ XX-> fore. balance], sizeof(FillPattern));
            for ( i = 0; i < 8; i++)
               fp[i] &= XX-> fill_pattern[i];
            p = prima_get_hatch( &fp);
         } else 
            p = prima_get_hatch( &XX-> fill_pattern);
         if ( !p) return false;
         XSetStipple( DISP, XX-> gc, p);
         XSetFillStyle( DISP, XX-> gc, FillStippled);
         if ( !XX-> flags. brush_fore) {
            XSetForeground( DISP, XX-> gc, XX-> fore. primary);
            XX-> flags. brush_fore = 1;
         }
         break;
      case 2: /* fore mix with fill pattern */
         if ( memcmp( XX-> fill_pattern, fillPatterns[fpEmpty], sizeof(FillPattern))==0)
            return false;
         if ( XX-> fore. balance ) {
            int i;
            FillPattern fp;
            memcpy( &fp, &guts. ditherPatterns[ XX-> fore. balance], sizeof(FillPattern));
            for ( i = 0; i < 8; i++) 
               fp[i] = (~fp[i]) & XX-> fill_pattern[i];
            p = prima_get_hatch( &fp);
            if ( !p) return false;
            XSetStipple( DISP, XX-> gc, p);
            XSetFillStyle( DISP, XX-> gc, FillStippled);
            XSetForeground( DISP, XX-> gc, XX-> fore. secondary);
            XX-> flags. brush_fore = 0;
            break;
         } else
            return false;
      default:
         return false;
      }
   }
   return true;
}
   
Bool
apc_gp_init( Handle self)
{
   X(self)-> resolution = guts. resolution;
   if ( guts. dynamicColors && !prima_palette_alloc( self)) return false;
   return true;
}

Bool
apc_gp_done( Handle self)
{
   if ( !X(self)) return false;
   if ( guts. dynamicColors) {
      prima_palette_free( self, true);
      free(X(self)-> palette);
   }
   prima_release_gc(X(self));
   return true;
}

static int
arc_completion( double * angleStart, double * angleEnd, int * needFigure)
{
   int max;
   double diff = ((long)( fabs( *angleEnd - *angleStart) * 1000 + 0.5)) / 1000;

   if ( diff == 0) {
      *needFigure = false;
      return 0;
   }

   while ( *angleStart > *angleEnd)
      *angleEnd += 360;

   while ( *angleStart < 0) {
      *angleStart += 360;
      *angleEnd   += 360;
   }

   while ( *angleStart >= 360) {
      *angleStart -= 360;
      *angleEnd   -= 360;
   }

   while ( *angleEnd >= *angleStart + 360)
      *angleEnd -= 360;

   if ( diff < 360) {
      *needFigure = true;
      return 0;
   }

   max = (int)(diff / 360);
   *needFigure = ( max * 360) != diff;
   return ( max % 2) ? 1 : 2;
}

static void
calculate_ellipse_divergence(void)
{
   static Bool init = false;
   if ( !init) {
      XGCValues gcv;
      Pixmap px = XCreatePixmap( DISP, guts.root, 4, 4, 1);
      GC gc = XCreateGC( DISP, px, 0, &gcv);
      XImage *xi;
      XSetForeground( DISP, gc, 0);
      XFillRectangle( DISP, px, gc, 0, 0, 5, 5);
      XSetForeground( DISP, gc, 1);
      XDrawArc( DISP, px, gc, 0, 0, 4, 4, 0, 360 * 64);
      if (( xi = XGetImage( DISP, px, 0, 0, 4, 4, 1, XYPixmap))) {
         int i;
         Byte *data[4];
         if ( xi-> bitmap_bit_order == LSBFirst) 
            prima_mirror_bytes(( Byte*) xi-> data, xi-> bytes_per_line * 4);
         for ( i = 0; i < 4; i++) data[i] = (Byte*)xi-> data + i * xi-> bytes_per_line;
#define PIX(x,y) ((data[y][0] & (0x80>>(x)))!=0)
         if (  PIX(2,1) && !PIX(3,1)) guts. ellipseDivergence.x = -1; else
         if ( !PIX(2,1) && !PIX(3,1)) guts. ellipseDivergence.x = 1; 
         if (  PIX(1,2) && !PIX(1,3)) guts. ellipseDivergence.y = -1; else
         if ( !PIX(1,2) && !PIX(1,3)) guts. ellipseDivergence.y = 1; 
#undef PIX                          
         XDestroyImage( xi);
      }
      XFreeGC( DISP, gc);
      XFreePixmap( DISP, px);
      init = true;
   }
}

#define ELLIPSE_RECT x - ( dX + 1) / 2 + 1, y - dY / 2, dX-guts.ellipseDivergence.x, dY-guts.ellipseDivergence.y
#define FILL_ANTIDEFECT_REPAIRABLE \
      ( rop_map[XX-> paint_rop] == GXcopy ||\
        rop_map[XX-> paint_rop] == GXset  ||\
        rop_map[XX-> paint_rop] == GXclear) 
#define FILL_ANTIDEFECT_OPEN {\
  XGCValues gcv;\
  gcv. line_width = 1;\
  gcv. line_style = LineSolid;\
  XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);\
}   
#define FILL_ANTIDEFECT_CLOSE {\
  XGCValues gcv;\
  gcv. line_width = XX-> line_width;\
  gcv. line_style = ( XX-> paint_rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;\
  XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);\
}   

Bool
apc_gp_arc( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   DEFXX;
   int compl, needf;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);

   SHIFT( x, y);
   y = REVERT( y);
   PURE_FOREGROUND;
   calculate_ellipse_divergence();
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   while ( compl--)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 360 * 64);
   if ( needf)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
          angleStart * 64, ( angleEnd - angleStart) * 64);
   return true;
}

Bool
apc_gp_bar( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;
   int mix = 0;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   RANGE4( x1, y1, x2, y2);
   while ( prima_make_brush( XX, mix++)) 
      XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_clear( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   
   if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) {
      x1 = 0; y1 = 0;
      x2 = XX-> size. x - 1;
      y2 = XX-> size. y - 1;
   }
   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   RANGE4( x1, y1, x2, y2);
   
   /* clean color entries, leave just background & foreground. XXX */
   if ( guts. dynamicColors && x1 <= 0 && x2 > XX-> size.x && y1 <= 0 && y2 >= XX-> size.y) {
      prima_palette_free(self,false);
      apc_gp_set_color(self, XX-> fore. color);
      apc_gp_set_back_color(self, XX-> back. color);
   }
   
   XSetForeground( DISP, XX-> gc, XX-> back. primary);
   if ( XX-> back. balance > 0) {
      Pixmap p = prima_get_hatch( &guts. ditherPatterns[ XX-> back. balance]);
      if ( p) {
         XSetFillStyle( DISP, XX-> gc, FillOpaqueStippled);
         XSetStipple( DISP, XX-> gc, p);
         XSetBackground( DISP, XX-> gc, XX-> back. secondary);
      } else
         XSetFillStyle( DISP, XX-> gc, FillSolid);
   } else 
      XSetFillStyle( DISP, XX-> gc, FillSolid);
   XX-> flags. brush_fore = 0;
   XFillRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1 + 1, y2 - y1 + 1);
   
   return true;
}

#define GRAD 57.29577951

Bool
apc_gp_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   int compl, needf;
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);

   SHIFT( x, y);
   y = REVERT( y);
   PURE_FOREGROUND;
   compl = arc_completion( &angleStart, &angleEnd, &needf);
   calculate_ellipse_divergence();
   while ( compl--)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 360 * 64);
   if ( !needf) return true;
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
       angleStart * 64, ( angleEnd - angleStart) * 64);
   XDrawLine( DISP, XX-> gdrawable,
      XX-> gc, x + cos( angleStart / GRAD) * dX / 2, y - sin( angleStart / GRAD) * dY / 2,
               x + cos( angleEnd / GRAD) * dX / 2,   y - sin( angleEnd / GRAD) * dY / 2);
   return true;
}

Bool
apc_gp_draw_poly( Handle self, int n, Point *pp)
{
   DEFXX;
   int i;
   int x = XX-> gtransform. x + XX-> btransform. x;
   int y = XX-> size. y + XX-> menuHeight - 1 - XX-> gtransform. y - XX-> btransform. y;
   XPoint *p;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if ((p = malloc( sizeof( XPoint)*n)) == nil) 
      return false;

   for ( i = 0; i < n; i++) {
      p[i].x = pp[i].x + x;
      p[i].y = y - pp[i].y;
      RANGE2(p[i].x, p[i].y);
   }

   PURE_FOREGROUND;
   XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, n, CoordModeOrigin);

   free( p);
   return true;
}

Bool
apc_gp_draw_poly2( Handle self, int np, Point *pp)
{
   DEFXX;
   int i;
   int x = XX-> gtransform. x + XX-> btransform. x;
   int y = XX-> size. y + XX-> menuHeight - 1 - XX-> gtransform. y - XX-> btransform. y;
   XSegment *s;
   int n = np / 2;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if ((s = malloc( sizeof( XSegment)*n)) == nil) return false;

   for ( i = 0; i < n; i++) {
      s[i].x1 = pp[i*2].x + x;
      s[i].y1 = y - pp[i*2].y;
      s[i].x2 = pp[i*2+1].x + x;
      s[i].y2 = y - pp[i*2+1].y;
      RANGE4(s[i].x1, s[i].y1, s[i].x2, s[i].y2);
   }

   PURE_FOREGROUND;
   XDrawSegments( DISP, XX-> gdrawable, XX-> gc, s, n);

   free( s);
   return true;
}

Bool
apc_gp_ellipse( Handle self, int x, int y, int dX, int dY)
{
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);

   SHIFT( x, y);
   y = REVERT( y);
   PURE_FOREGROUND;
   calculate_ellipse_divergence();
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT, 0, 64*360);
   return true;
}

Bool
apc_gp_fill_chord( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   DEFXX;
   int compl, needf, mix = 0;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);

   SHIFT( x, y);
   y = REVERT( y);

   XSetArcMode( DISP, XX-> gc, ArcChord);
   FILL_ANTIDEFECT_OPEN;  
   
   while ( prima_make_brush( XX, mix++)) {
      compl = arc_completion( &angleStart, &angleEnd, &needf);
      while ( compl--) {
         XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
         if ( FILL_ANTIDEFECT_REPAIRABLE)
            XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 0, 64*360);
      }

      if ( needf) {
         XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY,
             angleStart * 64, ( angleEnd - angleStart) * 64);
         if ( FILL_ANTIDEFECT_REPAIRABLE)
            XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 
               angleStart * 64, ( angleEnd - angleStart) * 64);
      }
   }
   FILL_ANTIDEFECT_CLOSE;
   return true;
}

Bool
apc_gp_fill_ellipse( Handle self, int x, int y,  int dX, int dY)
{
   DEFXX;
   int mix = 0;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);
   SHIFT( x, y);
   y = REVERT( y);

   FILL_ANTIDEFECT_OPEN;
   while ( prima_make_brush( XX, mix++)) { 
      XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
      if ( FILL_ANTIDEFECT_REPAIRABLE)
         XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 0, 64*360);
   }
   FILL_ANTIDEFECT_CLOSE;
   return true;
}

Bool
apc_gp_fill_poly( Handle self, int numPts, Point *points)
{
   /* XXX - beware, current implementation will not deal correctly with different rops and tiles */
   XPoint *p;
   DEFXX;
   int i, mix = 0;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if ( !( p = malloc(( numPts + 1) * sizeof( XPoint)))) return false;

   for ( i = 0; i < numPts; i++) {
      p[i]. x = (short)points[i]. x + XX-> gtransform. x + XX-> btransform. x;
      p[i]. y = (short)REVERT(points[i]. y + XX-> gtransform. y + XX-> btransform. y);
      RANGE2(p[i].x, p[i].y);
   }
   p[numPts]. x = (short)points[0]. x + XX-> gtransform. x + XX-> btransform. x;
   p[numPts]. y = (short)REVERT(points[0]. y + XX-> gtransform. y + XX-> btransform. y);
   RANGE2(p[numPts].x, p[numPts].y);

   FILL_ANTIDEFECT_OPEN;
   if ( guts. limits. XFillPolygon >= numPts) {
      while ( prima_make_brush( XX, mix++)) {
         XFillPolygon( DISP, XX-> gdrawable, XX-> gc, p, numPts, ComplexShape, CoordModeOrigin);
         if ( FILL_ANTIDEFECT_REPAIRABLE)
            XDrawLines( DISP, XX-> gdrawable, XX-> gc, p, numPts+1, CoordModeOrigin);
      }
      XCHECKPOINT;
   } else 
      warn( "Prima::Drawable::fill_poly: too many points");
   FILL_ANTIDEFECT_CLOSE;
   free( p);
   return true;
}

Bool
apc_gp_fill_sector( Handle self, int x, int y, int dX, int dY, double angleStart, double angleEnd)
{
   DEFXX;
   int compl, needf, mix = 0;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);

   SHIFT( x, y);
   y = REVERT( y);
   XSetArcMode( DISP, XX-> gc, ArcPieSlice);

   FILL_ANTIDEFECT_OPEN;
   while ( prima_make_brush( XX, mix++)) {
      compl = arc_completion( &angleStart, &angleEnd, &needf);
      while ( compl--) {
         XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY, 0, 64*360);
         if ( FILL_ANTIDEFECT_REPAIRABLE)
            XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 0, 64*360);
      }

      if ( needf) {
         XFillArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX, dY,
            angleStart * 64, ( angleEnd - angleStart) * 64);
         if ( FILL_ANTIDEFECT_REPAIRABLE)
            XDrawArc( DISP, XX-> gdrawable, XX-> gc, x - ( dX + 1) / 2 + 1, y - dY / 2, dX-1, dY-1, 
               angleStart * 64, ( angleEnd - angleStart) * 64);
      }
   }
   FILL_ANTIDEFECT_CLOSE;
   return true;
}

static int
get_pixel_depth( int depth) 
{
   if ( depth ==  1) return  1; else
   if ( depth <=  4) return  4; else
   if ( depth <=  8) return  8; else
   if ( depth <= 16) return 16; else
   if ( depth <= 24) return 24; else
   return 32;
}   
   

static uint32_t
color_to_pixel( Handle self, Color color, int depth)
{
   uint32_t pv;

   if ( depth == 1) {
      pv = color ? 1 : 0;
   } else if ( guts.palSize > 0 ) {
      pv = prima_color_find( self, color, -1, nil, RANK_FREE);
   } else {
      switch ( depth) {
      case 16:   
      case 24:   
      case 32:   
         pv = 
            (((COLOR_R(color) << guts. red_range  ) >> 8) << guts.   red_shift) |
            (((COLOR_G(color) << guts. green_range) >> 8) << guts. green_shift) |
            (((COLOR_B(color) << guts. blue_range ) >> 8) << guts.  blue_shift);
         if ( guts.machine_byte_order != guts.byte_order)  
            switch( depth) {
            case 16:
               pv = REVERSE_BYTES_16( pv);
               break;   
            case 24:
               pv = REVERSE_BYTES_24( pv);
               break;   
            case 32:
               pv = REVERSE_BYTES_32( pv);
               break;            
            }   
          break;
      default:
         warn("UAG_005: Not supported pixel depth");
         return 0;
      }
   }
   return pv;
}  

typedef struct {
   XImage *  i;
   Rect      clip;
   uint32_t  color;
   int       depth;
   int       y;
   Bool      singleBorder;
   XDrawable drawable;
   GC        gc;
   int       first;
   PList  *  lists;
} FillSession;

static Bool 
fs_get_pixel( FillSession * fs, int x, int y)
{
   if ( x < fs-> clip. left || x > fs-> clip. right || y < fs-> clip. top || y > fs-> clip. bottom)  {
      return false;
   }   

   if ( fs-> lists[ y - fs-> first]) {
      PList l = fs-> lists[ y - fs-> first];
      int i;
      for ( i = 0; i < l-> count; i+=2) {
         if (((int) l-> items[i+1] >= x) && ((int)l->items[i] <= x))
            return false;
      }   
   }   
   
   if ( !fs-> i || y != fs-> y) {
      XCHECKPOINT;
      if ( fs-> i) XDestroyImage( fs-> i);
      XCHECKPOINT;
      fs-> i = XGetImage( DISP, fs-> drawable, fs-> clip. left, y, 
                          fs-> clip. right - fs-> clip. left + 1, 1, 
                          ( fs-> depth == 1) ? 1 : AllPlanes, 
                          ( fs-> depth == 1) ? XYPixmap : ZPixmap);
      XCHECKPOINT;
      if ( !fs-> i) {
         return false;
      }   
      fs-> y = y;
   }   

   x -= fs-> clip. left;
   
   switch( fs-> depth) {
   case 1:
      {
         Byte xz = *((Byte*)(fs->i->data) + (x >> 3));
         uint32_t v = ( guts.bit_order == MSBFirst) ?
            ( xz & ( 0x80 >> ( x & 7)) ? 1 : 0) :
            ( xz & ( 0x01 << ( x & 7)) ? 1 : 0);
         return fs-> singleBorder ? 
            ( v == fs-> color) : ( v != fs-> color);
      }   
      break;
   case 4:
      {
         Byte xz = *((Byte*)(fs->i->data) + (x >> 1));
         uint32_t v = ( x & 1) ? ( xz & 0xF) : ( xz >> 4);
         return fs-> singleBorder ? 
            ( v == fs-> color) : ( v != fs-> color);
      }
      break;
   case 8:
     return fs-> singleBorder ? 
       ( fs-> color == *((Byte*)(fs->i->data) + x)) :
       ( fs-> color != *((Byte*)(fs->i->data) + x));
   case 16:
     return fs-> singleBorder ? 
       ( fs-> color == *((uint16_t*)(fs->i->data) + x)):
       ( fs-> color != *((uint16_t*)(fs->i->data) + x));
   case 24:
      return fs-> singleBorder ? 
       ( memcmp(( Byte*)(fs->i->data) + x, &fs->color, 3) == 0) :
       ( memcmp(( Byte*)(fs->i->data) + x, &fs->color, 3) != 0);
   case 32:
      return fs-> singleBorder ? 
       ( fs-> color == *((uint32_t*)(fs->i->data) + x)): 
       ( fs-> color != *((uint32_t*)(fs->i->data) + x));
   }  
   return false;
}   

static void
hline( FillSession * fs, int x1, int y, int x2)
{
   XFillRectangle( DISP, fs-> drawable, fs-> gc, x1, y, x2 - x1 + 1, 1);
   
   if ( y == fs-> y && fs-> i) {
      XDestroyImage( fs-> i);
      fs-> i = nil;
   }   
   
   y -= fs-> first;
   
   if ( fs-> lists[y] == nil)
      fs-> lists[y] = plist_create( 32, 128);
   list_add( fs-> lists[y], ( Handle) x1);
   list_add( fs-> lists[y], ( Handle) x2);
}   

static int
fill( FillSession * fs, int sx, int sy, int d, int pxl, int pxr)
{
   int x, xr = sx;
   while ( sx > fs-> clip. left  && fs_get_pixel( fs, sx - 1, sy)) sx--;
   while ( xr < fs-> clip. right && fs_get_pixel( fs, xr + 1, sy)) xr++;
   hline( fs, sx, sy, xr);

   if ( sy + d >= fs-> clip. top && sy + d <= fs-> clip. bottom) {
      x = sx;
      while ( x <= xr) {
         if ( fs_get_pixel( fs, x, sy + d)) {
            x = fill( fs, x, sy + d, d, sx, xr);
         }   
         x++;
      }   
   }   
   
   if ( sy - d >= fs-> clip. top && sy - d <= fs-> clip. bottom) {
      x = sx;
      while ( x < pxl) {
         if ( fs_get_pixel( fs, x, sy - d)) {
            x = fill( fs, x, sy - d, -d, sx, xr);
         }   
         x++;
      }   
      x = pxr;
      while ( x < xr) {
         if ( fs_get_pixel( fs, x, sy - d)) {
            x = fill( fs, x, sy - d, -d, sx, xr);
         }   
         x++;
      }   
   }   
   return xr;
}   

Bool
apc_gp_flood_fill( Handle self, int x, int y, Color color, Bool singleBorder)
{
   DEFXX;
   Bool ret = false;
   XRectangle cr;
   FillSession s;
   int mix = 0, hint;
   
   if ( !opt_InPaint) return false;
   
   s. singleBorder = singleBorder;
   s. drawable     = XX-> gdrawable;
   s. gc           = XX-> gc;
   SHIFT( x, y);
   y = REVERT( y);
   color = prima_map_color( color, &hint);
   prima_gp_get_clip_rect( self, &cr);
   cr. y += XX-> menuHeight;

   s. clip. left   = cr. x;
   s. clip. top    = cr. y;
   s. clip. right  = cr. x + cr. width  - 1;
   s. clip. bottom = cr. y + cr. height - 1;
   if ( cr. width <= 0 || cr. height <= 0) return false;
   s. i = nil;
   s. depth = XT_IS_BITMAP(X(self)) ? 1 : guts. idepth;
   s. depth = get_pixel_depth( s. depth);
   s. color = hint ? 
      (( hint == COLORHINT_BLACK) ? LOGCOLOR_BLACK : LOGCOLOR_WHITE)
      : color_to_pixel( self, color, s.depth);
   
   s. first = s. clip. top;
   if ( !( s. lists = malloc(( s. clip. bottom - s. clip. top + 1) * sizeof( void*)))) 
      return false;
   bzero( s. lists, ( s. clip. bottom - s. clip. top + 1) * sizeof( void*));

   prima_make_brush( XX, mix++);
   if ( fs_get_pixel( &s, x, y)) {
      fill( &s, x, y, -1, x, x);
      ret = true;
   }  

   while ( prima_make_brush( XX, mix++)) {
      for ( y = 0; y < s. clip. bottom - s. clip. top + 1; y++)
         if ( s. lists[y]) 
            for ( x = 0; x < s.lists[y]-> count; x += 2) 
               XFillRectangle( DISP, s.drawable, s.gc, 
                   (int)s.lists[y]->items[x], y,
                   (int)s.lists[y]->items[x+1] - (int)s.lists[y]->items[x], 1);
   }

   if ( s. i) XDestroyImage( s. i);

   for ( x = 0; x < s. clip. bottom - s. clip. top + 1; x++)
      if ( s. lists[x]) 
         plist_destroy( s.lists[x]);
   free( s. lists);

   return ret;
}

Color
apc_gp_get_pixel( Handle self, int x, int y)
{
   DEFXX;
   Color c = 0;
   XImage *im;
   Bool pixmap;
   uint32_t p32 = 0;

   if ( !opt_InPaint) return clInvalid;
   SHIFT( x, y);

   if ( x < 0 || x >= XX-> size.x || y < 0 || y >= XX-> size.y)
      return clInvalid;

   if ( XT_IS_DBM(XX)) {
      pixmap = XT_IS_PIXMAP(XX) ? true : false;
   } else {
      pixmap = guts. idepth > 1;
   }   
   
   im = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y + XX-> menuHeight - y - 1, 1, 1, 
                   pixmap ? AllPlanes : 1,
                   pixmap ? ZPixmap   : XYPixmap);
   XCHECKPOINT;
   if ( !im) return clInvalid;

   if ( pixmap) {
      if ( guts. palSize > 0) {
         int pixel;
         if ( guts. idepth <= 8) 
            pixel = (*( U8*)( im-> data)) & (( 1 << guts.idepth) - 1);
         else
            pixel = (*( U16*)( im-> data)) & (( 1 << guts.idepth) - 1);
         if ( guts.palette[pixel]. rank == RANK_FREE) {
            XColor xc;
            xc.pixel = pixel;
            XQueryColors( DISP, guts. defaultColormap, &xc, 1);
            c = RGB_COMPOSITE(xc.red>>8,xc.green>>8,xc.blue>>8);
         } else 
            c = guts.palette[pixel]. composite;
      } else {
         switch ( guts. idepth) {
         case 16:
            p32 = *(( uint16_t*)(im-> data));
            if ( guts.machine_byte_order != guts.byte_order) 
               p32 = REVERSE_BYTES_16(p32);
            goto COMP;
         case 24:   
            p32 = (im-> data[0] << 16) | (im-> data[1] << 8) | im-> data[2];
            if ( guts.machine_byte_order != guts.byte_order) 
               p32 = REVERSE_BYTES_24(p32);
            goto COMP;
         case 32:
            p32 = *(( uint32_t*)(im-> data));
            if ( guts.machine_byte_order != guts.byte_order) 
               p32 = REVERSE_BYTES_32(p32);
         COMP:   
            c = 
              ((((p32 & guts. visual. blue_mask)  >> guts. blue_shift) << 8) >> guts. blue_range) |
              (((((p32 & guts. visual. green_mask) >> guts. green_shift) << 8) >> guts. green_range) << 8) |
              (((((p32 & guts. visual. red_mask)   >> guts. red_shift)   << 8) >> guts. red_range) << 16);
            break;
         default:
            warn("UAG_009: get_pixel not implemented for %d depth", guts.idepth);
         }   
      }
   } else {
      c = ( im-> data[0] & ((guts.bit_order == MSBFirst) ? 0x80 : 1)) 
         ? 0xffffff : 0;
   }   

   XDestroyImage( im);
   return c;
}

Color
apc_gp_get_nearest_color( Handle self, Color color)
{
   if ( guts. palSize > 0) 
      return guts. palette[ prima_color_find( self, color, -1, nil, RANK_FREE)]. composite;
   if ( guts. visualClass == TrueColor || guts. visualClass == DirectColor) {
      XColor xc;
      xc. red   = COLOR_R16(color);
      xc. green = COLOR_G16(color);
      xc. blue  = COLOR_B16(color);
      if ( XAllocColor( DISP, guts. defaultColormap, &xc)) {
         XFreeColors( DISP, guts. defaultColormap, &xc. pixel, 1, 0); 
         return 
            (( xc. red   & 0xFF00) << 8) |
            (( xc. green & 0xFF00)) |
            (( xc. blue  & 0xFF00) >> 8);
      }
   }
   return color;
}   

PRGBColor
apc_gp_get_physical_palette( Handle self, int * colors)
{
   int i;
   PRGBColor p;
   XColor * xc;
   
   *colors = 0;
   
   if ( guts. palSize == 0) return nil;
   if ( !( p = malloc( guts. palSize * sizeof( RGBColor))))
      return nil;
   if ( !( xc = malloc( guts. palSize * sizeof( XColor)))) {
      free( p);
      return nil;
   }
   for ( i = 0; i < guts. palSize; i++) xc[i]. pixel = i;
   XQueryColors( DISP, guts. defaultColormap, xc, guts. palSize);
   XCHECKPOINT;
   for ( i = 0; i < guts. palSize; i++) {
      p[i]. r = xc[i]. red   >> 8;
      p[i]. g = xc[i]. green >> 8;
      p[i]. b = xc[i]. blue  >> 8;
   }
   free( xc);
   *colors = guts. palSize;
   return p;
}

Bool
apc_gp_get_region( Handle self, Handle mask)
{
   DEFXX;
   int depth;

   if ( !XF_IN_PAINT(XX)) return false;

   if ( !mask) 
      return XX-> clip_mask_extent. x != 0 && XX-> clip_mask_extent. y != 0;
      
   if ( XX-> clip_mask_extent. x == 0 || XX-> clip_mask_extent. y == 0)
      return false;

   XSetClipOrigin( DISP, XX-> gc, 0, 0);

   depth = XT_IS_BITMAP(XX) ? 1 : guts. qdepth;
   CImage( mask)-> create_empty( mask, XX-> clip_mask_extent. x, XX-> clip_mask_extent. y, depth);
   CImage( mask)-> begin_paint( mask);
   XCHECKPOINT;
   XSetForeground( DISP, XX-> gc, ( depth == 1) ? 1 : guts. monochromeMap[1]);
   XFillRectangle( DISP, X(mask)-> gdrawable, XX-> gc, 0, 0, XX-> clip_mask_extent.x + 1, XX-> clip_mask_extent.y + 1);
   XCHECKPOINT;
   XX-> flags. brush_fore = 0;
   CImage( mask)-> end_paint( mask);
   XCHECKPOINT;
   if ( depth != 1) CImage( mask)-> set_type( mask, imBW);

   XSetClipOrigin( DISP, XX-> gc, XX-> btransform.x, 
       - XX-> btransform. y + XX-> size. y + XX-> menuHeight - XX-> clip_mask_extent.y);
   return true;
}

Bool
apc_gp_line( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   SHIFT( x1, y1); SHIFT( x2, y2);
   RANGE4(x1, y1, x2, y2); /* not really correct */
   PURE_FOREGROUND;
   if (( XX-> line_width == 0) && (x1 == x2 || y1 == y2)) {
      XGCValues gcv;
      gcv. line_width = 1;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
   }
   XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
   if (( XX-> line_width == 0) && (x1 == x2 || y1 == y2)) {
      XGCValues gcv;
      gcv. line_width = 0;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
   }
   return true;
}

Bool
apc_gp_rectangle( Handle self, int x1, int y1, int x2, int y2)
{
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   SHIFT( x1, y1); SHIFT( x2, y2);
   SORT( x1, x2); SORT( y1, y2);
   RANGE4(x1, y1, x2, y2);
   PURE_FOREGROUND;
   XDrawRectangle( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y2), x2 - x1, y2 - y1);
   XCHECKPOINT;
   return true;
}

Bool
apc_gp_sector( Handle self, int x, int y,  int dX, int dY, double angleStart, double angleEnd)
{
   int compl, needf;
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;
   if ( dX <= 0 || dY <= 0) return false;
   RANGE4(x, y, dX, dY);

   SHIFT( x, y);
   y = REVERT( y);

   compl = arc_completion( &angleStart, &angleEnd, &needf);
   PURE_FOREGROUND;
   calculate_ellipse_divergence();
   while ( compl--)
      XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
          0, 360 * 64);
   if ( !needf) return true;
   XDrawArc( DISP, XX-> gdrawable, XX-> gc, ELLIPSE_RECT,
       angleStart * 64, ( angleEnd - angleStart) * 64);
   XDrawLine( DISP, XX-> gdrawable, XX-> gc,
       x + cos( angleStart / GRAD) * dX / 2, y - sin( angleStart / GRAD) * dY / 2,
       x, y
   );
   XDrawLine( DISP, XX-> gdrawable, XX-> gc,
       x, y,
       x + cos( angleEnd / GRAD) * dX / 2, y - sin( angleEnd / GRAD) * dY / 2
   );
   return true;
}

Bool
apc_gp_set_palette( Handle self)
{
   if ( XT_IS_WIDGET(X(self))) return true;
   return prima_palette_replace( self, false);
}

Bool
apc_gp_set_region( Handle self, Handle mask)
{
   DEFXX;
   Pixmap px;
   ImageCache *cache;
   PImage img;
   GC gc;
   XGCValues gcv;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if (mask == nilHandle) {
      px = None;
      XX-> clip_mask_extent. x = XX-> clip_mask_extent. y = 0;
   } else {
      img = PImage(mask);
      cache = prima_create_image_cache(img, nilHandle, CACHE_BITMAP);
      if ( !cache) return false;
      px = XCreatePixmap(DISP, guts. root, img->w, img->h, 1);
      gc = XCreateGC(DISP, px, 0, &gcv);
      prima_put_ximage(px, gc, cache->image, 0, 0, 0, 0, img->w, img->h);
      XFreeGC( DISP, gc);
      XX-> clip_mask_extent. x = img-> w;
      XX-> clip_mask_extent. y = img-> h;
      XSetClipOrigin( DISP, XX-> gc, XX-> btransform.x, 
         - XX-> btransform. y + XX-> size. y + XX-> menuHeight - img-> h);
   }
   XSetClipMask(DISP, XX->gc, px);
   if ( px != None) XFreePixmap( DISP, px);
   return true;
}

Bool
apc_gp_set_pixel( Handle self, int x, int y, Color color)
{
   DEFXX;

   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   SHIFT( x, y);
   XSetForeground( DISP, XX-> gc, prima_allocate_color( self, color, nil));
   XDrawPoint( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y));
   XX-> flags. brush_fore = 0;
   return true;
}

static Point
gp_get_text_overhangs( Handle self, const char *text, int len, Bool wide)
{
   DEFXX;
   Point ret;
   if ( len > 0) {
      XCharStruct * cs;
      cs = prima_char_struct( XX-> font-> fs, (void*) text, wide);  
      ret. x = ( cs-> lbearing < 0) ? - cs-> lbearing : 0;
      text += (len - 1) * wide ? 2 : 1;
      cs = prima_char_struct( XX-> font-> fs, (void*) text, wide);  
      ret. y = (( cs-> width - cs-> rbearing) < 0) ? cs-> rbearing - cs-> width : 0;
   } else
      ret. x = ret. y = 0;
   return ret;
}   

static int
gp_get_text_width( Handle self, const char *text, int len, Bool addOverhang, Bool wide);

static Point *
gp_get_text_box( Handle self, const char * text, int len, Bool wide);

static Bool
gp_text_out_rotated( Handle self, const char * text, int x, int y, int len, Bool wide) 
{
   DEFXX;
   int i;
   PRotatedFont r;
   XCharStruct *cs;
   int px, py = ( XX-> flags. paint_base_line) ?  XX-> font-> font. descent : 0; 
   int ax = 0, ay = 0;
   int psx, psy, dsx, dsy;
   Fixed rx, ry;

   if ( !prima_update_rotated_fonts( XX-> font, text, len, wide, PDrawable( self)-> font. direction, &r)) 
      return false;

   for ( i = 0; i < len; i++) {
      XChar2b index;

      /* acquire actual character index */
      index. byte1 = wide ? (( XChar2b*) text+i)-> byte1 : 0;
      index. byte2 = wide ? (( XChar2b*) text+i)-> byte2 : *((unsigned char*)text+i);
      
      if ( index. byte1 < r-> first1 || index. byte1 >= r-> first1 + r-> height ||
           index. byte2 < r-> first2 || index. byte2 >= r-> first2 + r-> width) {
         if ( r-> defaultChar1 < 0 || r-> defaultChar2 < 0) continue;
         index. byte1 = ( unsigned char) r-> defaultChar1;
         index. byte2 = ( unsigned char) r-> defaultChar2;
      }   

      /* querying character */
      if ( r-> map[(index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2] == nil) continue;
      cs = XX-> font-> fs-> per_char ? 
         XX-> font-> fs-> per_char + 
            ( index. byte1 - XX-> font-> fs-> min_byte1) * r-> width + 
              index. byte2 - XX-> font-> fs-> min_char_or_byte2 :
         &(XX-> font-> fs-> min_bounds);
  
      /* find reference point in pixmap */
      px = ( cs-> lbearing < 0) ? -cs-> lbearing : 0;
      rx. l = px * r-> cos2. l - py * r-> sin2. l + UINT16_PRECISION/2;
      ry. l = px * r-> sin2. l + py * r-> cos2. l + UINT16_PRECISION/2;
      psx = rx. i. i - r-> shift. x;
      psy = ry. i. i - r-> shift. y;
      
      /* find glyph position */
      rx. l = ax * r-> cos2. l - ay * r-> sin2. l + UINT16_PRECISION/2;
      ry. l = ax * r-> sin2. l + ay * r-> cos2. l + UINT16_PRECISION/2;
      dsx = x + rx. i. i - psx;
      dsy = REVERT( y + ry. i. i) + psy - r-> dimension. y + 1;

      /*       
      printf("shift %d %d\n", r-> shift.x, r-> shift.y);            
      printf("point ref: %d %d => %d %d. dims: %d %d, [%d %d %d]\n", px, py, psx, psy, r-> dimension.x, r-> dimension.y, 
           cs-> lbearing, cs-> rbearing - cs-> lbearing, cs-> width - cs-> rbearing);
      printf("plot ref: %d %d => %d %d\n", ax, ay, rx.i.i, ry.i.i);
      printf("at: %d %d ( sz = %d), dest: %d %d\n", x, y, XX-> size.y, dsx, dsy);
      */
      
/*   GXandReverse   ropNotDestAnd */		/* dest = (!dest) & src */
/*   GXorReverse    ropNotDestOr */		/* dest = (!dest) | src */
/*   GXequiv        ropNotSrcXor */		/* dest ^= !src */
/*   GXandInverted  ropNotSrcAnd */		/* dest &= !src */
/*   GXorInverted   ropNotSrcOr */		/* dest |= !src */
/*   GXnand         ropNotAnd */		/* dest = !(src & dest) */
/*   GXnor          ropNotOr */		/* dest = !(src | dest) */
/*   GXinvert       ropInvert */		/* dest = !dest */
      
      switch ( XX-> paint_rop) { /* XXX Limited set edition - either expand to full list or find new way to display bitmaps */
      case ropXorPut:  
         XSetBackground( DISP, XX-> gc, 0);
         XSetFunction( DISP, XX-> gc, GXxor); 
         break;
      case ropOrPut:   
         XSetBackground( DISP, XX-> gc, 0);
         XSetFunction( DISP, XX-> gc, GXor);
         break;
      case ropAndPut:  
         XSetBackground( DISP, XX-> gc, 0xffffffff);
         XSetFunction( DISP, XX-> gc, GXand);
         break;
      case ropNotPut:   
      case ropBlackness:
         XSetForeground( DISP, XX-> gc, 0);
         XSetBackground( DISP, XX-> gc, 0xffffffff);
         XSetFunction( DISP, XX-> gc, GXand);
         break;
      case ropWhiteness:
         XSetForeground( DISP, XX-> gc, 0xffffffff);
         XSetBackground( DISP, XX-> gc, 0);
         XSetFunction( DISP, XX-> gc, GXor);
         break;   
      default:   
         XSetForeground( DISP, XX-> gc, 0);
         XSetBackground( DISP, XX-> gc, 0xffffffff);
         XSetFunction( DISP, XX-> gc, GXand);
      }
      XPutImage( DISP, XX-> gdrawable, XX-> gc, 
          r-> map[(index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2]-> image, 
          0, 0, dsx, dsy, r-> dimension.x, r-> dimension.y);
      XCHECKPOINT; 
      switch ( XX-> paint_rop) {
      case ropAndPut:   
      case ropOrPut:
      case ropXorPut:
      case ropBlackness:   
      case ropWhiteness:
         break;
      case ropNotPut:   
         XSetForeground( DISP, XX-> gc, XX-> fore. primary);
         XSetBackground( DISP, XX-> gc, 0xffffffff);
         XSetFunction( DISP, XX-> gc, GXorInverted);
         goto DISPLAY;
      default:   
          XSetForeground( DISP, XX-> gc, XX-> fore. primary);
          XSetBackground( DISP, XX-> gc, 0);
          XSetFunction( DISP, XX-> gc, GXor);
      DISPLAY:         
          XPutImage( DISP, XX-> gdrawable, XX-> gc, 
              r-> map[(index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2]-> image, 
              0, 0, dsx, dsy, r-> dimension.x, r-> dimension.y);
          XCHECKPOINT;
      }
      ax += cs-> width;
   }  
   apc_gp_set_rop( self, XX-> paint_rop);
   XSetFillStyle( DISP, XX-> gc, FillSolid);
   XSetForeground( DISP, XX-> gc, XX-> fore. primary);
   XSetBackground( DISP, XX-> gc, XX-> back. primary);
   XX-> flags. brush_fore = 1;
   XX-> flags. brush_back = 1;

   if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {      
      int lw = apc_gp_get_line_width( self);
      int tw = gp_get_text_width( self, text, len, true, wide) - 1;
      int d  = XX-> font-> underlinePos;
      Point ovx = gp_get_text_overhangs( self, text, len, wide);
      int x1, y1, x2, y2;
      if ( lw != XX-> font-> underlineThickness)
         apc_gp_set_line_width( self, XX-> font-> underlineThickness);

      if ( PDrawable( self)-> font. style & fsUnderlined) {
         ay = d + ( XX-> flags. paint_base_line ? 0 : XX-> font-> font. descent);
         rx. l = -ovx.x * r-> cos2. l - ay * r-> sin2. l + 0.5;
         ry. l = -ovx.x * r-> sin2. l + ay * r-> cos2. l + 0.5;
         x1 = x + rx. i. i;
         y1 = y + ry. i. i;
         tw += ovx.y;
         rx. l = tw * r-> cos2. l - ay * r-> sin2. l + 0.5;
         ry. l = tw * r-> sin2. l + ay * r-> cos2. l + 0.5;
         x2 = x + rx. i. i;
         y2 = y + ry. i. i;
         XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2)); 
      }

      if ( PDrawable( self)-> font. style & fsStruckOut) {
         ay =  PDrawable( self)-> font.height/2 + ( XX-> flags. paint_base_line ? 0 : XX-> font-> font. descent);
         rx. l = -ovx.x * r-> cos2. l - ay * r-> sin2. l + 0.5;
         ry. l = -ovx.x * r-> sin2. l + ay * r-> cos2. l + 0.5;
         x1 = x + rx. i. i;
         y1 = y + ry. i. i;
         tw += ovx.y;
         rx. l = tw * r-> cos2. l - ay * r-> sin2. l + 0.5;
         ry. l = tw * r-> sin2. l + ay * r-> cos2. l + 0.5;
         x2 = x + rx. i. i;
         y2 = y + ry. i. i;
         XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2)); 
      }

      if ( lw != XX-> font-> underlineThickness) 
         apc_gp_set_line_width( self, lw);
   }   
   return true;
}   

Bool
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, Bool utf8)
{
   DEFXX;
   
   if ( PObject( self)-> options. optInDrawInfo) return false;
   if ( !XF_IN_PAINT(XX)) return false;

   if ( len == 0) return true;

   if ( utf8)  
      if ( !( text = ( char *) prima_alloc_utf8_to_wchar( text, len))) return false;
   
   /* paint background if opaque */
   if ( XX-> flags. paint_opaque) {
      int i;
      Point * p = gp_get_text_box( self, text, len, utf8);
      FillPattern fp;
      memcpy( &fp, apc_gp_get_fill_pattern( self), sizeof( FillPattern));
      XSetForeground( DISP, XX-> gc, XX-> back. primary);
      XX-> flags. brush_back = 0;
      XX-> flags. brush_fore = 1; 
      XX-> fore. balance = 0;
      XSetFunction( DISP, XX-> gc, GXcopy);
      apc_gp_set_fill_pattern( self, fillPatterns[fpSolid]);
      for ( i = 0; i < 4; i++) {
         p[i].x += x;
         p[i].y += y;
      }   
      i = p[2].x; p[2].x = p[3].x; p[3].x = i;
      i = p[2].y; p[2].y = p[3].y; p[3].y = i;
      
      apc_gp_fill_poly( self, 4, p);
      apc_gp_set_rop( self, XX-> paint_rop);
      apc_gp_set_color( self, XX-> fore. color);
      apc_gp_set_fill_pattern( self, fp);
      free( p); 
   }  
   SHIFT( x, y);
   RANGE2(x,y);

   if ( PDrawable( self)-> font. direction != 0) {
      Bool ret = gp_text_out_rotated( self, text, x, y, len, utf8);
      if ( utf8) free(( char *) text);
      return ret;
   }

   if ( !XX-> flags. paint_base_line)
      y += XX-> font-> font. descent;

   XSetFillStyle( DISP, XX-> gc, FillSolid);
   if ( !XX-> flags. brush_fore) {
      XSetForeground( DISP, XX-> gc, XX-> fore. primary);
      XX-> flags. brush_fore = 1;
   }

   if ( utf8)
      XDrawString16( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y) + 1, (XChar2b*) text, len);
   else
      XDrawString( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y) + 1, ( char*) text, len);
   XCHECKPOINT;
   
   if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {
      int lw = apc_gp_get_line_width( self);
      int tw = gp_get_text_width( self, text, len, true, utf8);
      int d  = XX-> font-> underlinePos;
      Point ovx = gp_get_text_overhangs( self, text, len, utf8);
      if ( lw != XX-> font-> underlineThickness)
         apc_gp_set_line_width( self, XX-> font-> underlineThickness);
      if ( PDrawable( self)-> font. style & fsUnderlined)
         XDrawLine( DISP, XX-> gdrawable, XX-> gc, 
            x - ovx.x, REVERT( y + d), x + tw - 1 + ovx.y, REVERT( y + d)); 
      if ( PDrawable( self)-> font. style & fsStruckOut)
         XDrawLine( DISP, XX-> gdrawable, XX-> gc, 
            x - ovx.x, REVERT( y + PDrawable( self)-> font.height/2), 
            x + tw - 1 + ovx.y, REVERT( y + PDrawable( self)-> font.height/2)); 
      if ( lw != XX-> font-> underlineThickness) 
         apc_gp_set_line_width( self, lw);
   }   

   if ( utf8) free(( char *) text);
   
   return true;
}

/* gpi settings */
Color
apc_gp_get_back_color( Handle self)
{
   DEFXX;
   return ( XF_IN_PAINT(XX)) ? XX-> back. color : prima_map_color( XX-> saved_back, nil);
}

int
apc_gp_get_bpp( Handle self)
{
   return guts. depth;
}

Color
apc_gp_get_color( Handle self)
{
   DEFXX;
   return ( XF_IN_PAINT(XX)) ? XX-> fore. color : prima_map_color(XX-> saved_fore, nil);
}

/* returns rect in X coordinates BUT without menuHeight deviation */
void
prima_gp_get_clip_rect( Handle self, XRectangle *cr)
{
   DEFXX;
   XRectangle r;

   cr-> x = 0;
   cr-> y = XX-> menuHeight;
   cr-> width = XX-> size.x;
   cr-> height = XX-> size.y;
   if ( XF_IN_PAINT(XX) && ( XX-> invalid_region || XX-> paint_region)) {
      XClipBox( XX-> invalid_region ? XX-> invalid_region : XX-> paint_region,
                &r);
      prima_rect_intersect( cr, &r);
   }
   cr-> y -= XX-> menuHeight;
   if ( XX-> clip_rect. x != 0
        || XX-> clip_rect. y != 0
        || XX-> clip_rect. width != XX-> size.x
        || XX-> clip_rect. height != XX-> size.y) {
      prima_rect_intersect( cr, &XX-> clip_rect);
   }
}

Rect
apc_gp_get_clip_rect( Handle self)
{
   DEFXX;
   XRectangle cr;
   Rect r;

   prima_gp_get_clip_rect( self, &cr);
   r. left = cr. x;
   r. top = XX-> size. y - cr. y - 1;
   r. bottom = r. top - cr. height + 1;
   r. right = cr. x + cr. width - 1;
   return r;
}

PFontABC
prima_xfont2abc( XFontStruct * fs, int firstChar, int lastChar)
{
   PFontABC abc = malloc( sizeof( FontABC) * (lastChar - firstChar + 1));
   XCharStruct *cs;
   int k, l, d = fs-> max_char_or_byte2 - fs-> min_char_or_byte2 + 1;
   int default_char1 = fs-> default_char >> 8;
   int default_char2 = fs-> default_char & 0xff;
   if ( !abc) return nil;
   
   if ( default_char2 < fs-> min_char_or_byte2 || default_char2 > fs-> max_char_or_byte2 ||
        default_char1 < fs-> min_byte1 || default_char1 > fs-> max_byte1) {
        default_char1 = fs-> min_byte1;
        default_char2 = fs-> min_char_or_byte2;
   }
   for ( k = firstChar, l = 0; k <= lastChar; k++, l++) {
      int i1 = k >> 8;
      int i2 = k & 0xff;
      if ( !fs-> per_char)
	 cs = &fs-> min_bounds;
      else if ( i2 < fs-> min_char_or_byte2 || i2 > fs-> max_char_or_byte2 ||
                i1 < fs-> min_byte1 || i1 > fs-> max_byte1)
	 cs = fs-> per_char + 
              (default_char1 - fs-> min_byte1) * d +
               default_char2 - fs-> min_char_or_byte2;
      else
	 cs = fs-> per_char + 
              (i1 - fs-> min_byte1) * d +
               i2 - fs-> min_char_or_byte2;
      abc[l]. a = cs-> lbearing;
      abc[l]. b = cs-> rbearing - cs-> lbearing;
      abc[l]. c = cs-> width - cs-> rbearing;
   }
   return abc;
}   

PFontABC
apc_gp_get_font_abc( Handle self, int firstChar, int lastChar, Bool unicode)
{
   PFontABC abc;
   if ( self) {
      DEFXX;
      if (!XX-> font) apc_gp_set_font( self, &PDrawable( self)-> font);
      if (!XX-> font) return nil;
      abc = prima_xfont2abc( XX-> font-> fs, firstChar, lastChar);
   } else
      abc = prima_xfont2abc( guts. font_abc_nil_hack, firstChar, lastChar);
   return abc;
}

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{
   DEFXX;
   unsigned long * ret = nil;
   XFontStruct * fs;
   if (!XX-> font) apc_gp_set_font( self, &PDrawable( self)-> font);
   if (!XX-> font) return nil;
   fs = XX-> font-> fs;
   *count = (fs-> max_byte1 - fs-> min_byte1 + 1) * 2;
   if (( ret = malloc( sizeof( unsigned long) * ( *count)))) {
      int i;
      for ( i = fs-> min_byte1; i <= fs-> max_byte1; i++) {
         ret[(i - fs-> min_byte1) * 2 + 0] = i * 256 + fs-> min_char_or_byte2;
         ret[(i - fs-> min_byte1) * 2 + 1] = i * 256 + fs-> max_char_or_byte2;
      }
   }
   return ret;
}

FillPattern *
apc_gp_get_fill_pattern( Handle self)
{
   return &(X(self)-> fill_pattern);
}

int
apc_gp_get_line_end( Handle self)
{
   DEFXX;
   int cap;
   XGCValues gcv;

   if ( XF_IN_PAINT(XX)) {
      if ( XGetGCValues( DISP, XX-> gc, GCCapStyle, &gcv) == 0) {
         warn( "UAG_006: error querying GC values");
         cap = CapButt;
      } else {
         cap = gcv. cap_style;
      }
   } else {
      cap = XX-> gcv. cap_style;
   }
   if ( cap == CapRound)
      return leRound;
   else if ( cap == CapProjecting)
      return leSquare;
   return leFlat;
}

int
apc_gp_get_line_width( Handle self)
{
   DEFXX;
   return XF_IN_PAINT(XX) ? XX-> line_width : XX-> gcv. line_width; 
}

int
apc_gp_get_line_pattern( Handle self, unsigned char *dashes)
{
   DEFXX;
   int n;
   if ( XF_IN_PAINT(XX)) {
      n = XX-> paint_ndashes;
      if ( XX-> paint_dashes) 
         memcpy( dashes, XX-> paint_dashes, n);
      else
         bzero( dashes, n);
   } else {
      n = XX-> ndashes;
      if ( n < 0) {
	 n = 0;
	 strcpy(( char*) dashes, "");
      } else if ( n == 0) {
	 n = 1;
	 strcpy(( char*) dashes, "\1");
      } else {
	 memcpy( dashes, XX-> dashes, n);
      }
   }
   return n;
}

Point
apc_gp_get_resolution( Handle self)
{
   Point ret;
   if ( self) {
      ret.x = X(self)-> resolution.x;
      ret.y = X(self)-> resolution.y;
   } else {
      ret.x = guts.resolution.x;
      ret.y = guts.resolution.y;
   }
   return ret;
}

int
apc_gp_get_rop( Handle self)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      return XX-> paint_rop;
   } else {
      return XX-> rop;
   }
}

int
apc_gp_get_rop2( Handle self)
{
   DEFXX;
   if ( XF_IN_PAINT(XX))
      return XX-> paint_rop2;
   else
      return XX-> rop2;
}

static int
gp_get_text_width( Handle self, const char *text, int len, Bool addOverhang, Bool wide)
{
   DEFXX;
   int ret;
   
   if ( !XX-> font) apc_gp_set_font( self, &PDrawable( self)-> font);
   if ( !XX-> font) return 0;
   ret = wide ? 
      XTextWidth16( XX-> font-> fs, ( XChar2b *) text, len) :
      XTextWidth  ( XX-> font-> fs, (char*) text, len);
   if ( addOverhang) {
      Point ovx = gp_get_text_overhangs( self, text, len, wide);
      ret += ovx. x + ovx. y;
   }
   return ret;
}

int
apc_gp_get_text_width( Handle self, const char * text, int len, Bool addOverhang, Bool utf8)
{
   int ret;
   if ( utf8)  
      if ( !( text = ( char *) prima_alloc_utf8_to_wchar( text, len))) return 0;
   ret = gp_get_text_width( self, text, len, addOverhang, utf8);
   if ( utf8)
      free(( char*) text);
   return ret;
}

static Point *
gp_get_text_box( Handle self, const char * text, int len, Bool wide)
{
   DEFXX;
   Point * pt = ( Point *) malloc( sizeof( Point) * 5);
   int x;
   Point ovx;
   
   if ( !pt) return nil;

   if ( !XX-> font) 
      apc_gp_set_font( self, &PDrawable( self)-> font);
   if ( !XX-> font) 
      return nil;
   
   x = wide ? 
      XTextWidth16( XX-> font-> fs, ( XChar2b*) text, len) :
      XTextWidth( XX-> font-> fs, (char*)text, len);
   ovx = gp_get_text_overhangs( self, text, len, wide);

   pt[0].y = pt[2]. y = XX-> font-> font. ascent - 1;
   pt[1].y = pt[3]. y = - XX-> font-> font. descent;
   pt[4].y = 0;
   pt[4].x = x;
   pt[3].x = pt[2]. x = x - ovx. y;
   pt[0].x = pt[1]. x = - ovx. x;

   if ( !XX-> flags. paint_base_line) {
      int i;
      for ( i = 0; i < 5; i++) pt[i]. y += XX-> font-> font. descent;
   }   
   
   if ( PDrawable( self)-> font. direction != 0) {
      int i;
      double s = sin( PDrawable( self)-> font. direction / 572.9577951);
      double c = cos( PDrawable( self)-> font. direction / 572.9577951);
      for ( i = 0; i < 5; i++) {
         double x = pt[i]. x * c - pt[i]. y * s;
         double y = pt[i]. x * s + pt[i]. y * c;
         pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
         pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
      }
   }
 
   return pt;
}

Point *
apc_gp_get_text_box( Handle self, const char * text, int len, Bool utf8)
{
   Point * ret;
   if ( utf8)  
      if ( !( text = ( char *) prima_alloc_utf8_to_wchar( text, len))) return 0;
   ret = gp_get_text_box( self, text, len, utf8);
   if ( utf8)
      free(( char*) text);
   return ret;
}   

Point
apc_gp_get_transform( Handle self)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      return XX-> gtransform;
   } else {
      return XX-> transform;
   }
}

Bool
apc_gp_get_text_opaque( Handle self)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      return XX-> flags. paint_opaque ? true : false;
   } else {
      return XX-> flags. opaque ? true : false;
   }
}

Bool
apc_gp_get_text_out_baseline( Handle self)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      return XX-> flags. paint_base_line ? true : false;
   } else {
      return XX-> flags. base_line ? true : false;
   }
}

Bool
apc_gp_set_clip_rect( Handle self, Rect clipRect)
{
   DEFXX;
   Region region;
   XRectangle r;

   if ( !XF_IN_PAINT(XX))
      return false;

   SORT( clipRect. left, clipRect. right);
   SORT( clipRect. bottom, clipRect. top);
   r. x = clipRect. left;
   r. y = REVERT( clipRect. top);
   r. width = clipRect. right - clipRect. left+1;
   r. height = clipRect. top - clipRect. bottom+1;
   XX-> clip_rect = r;
   XX-> clip_rect. y -= XX-> menuHeight;
   region = XCreateRegion();
   XUnionRectWithRegion( &r, region, region);
   if ( XX-> paint_region) 
      XIntersectRegion( region, XX-> paint_region, region);
   if ( XX-> btransform. x != 0 || XX-> btransform. y != 0) {
      XOffsetRegion( region, XX-> btransform. x, -XX-> btransform. y);
   } 
   XSetRegion( DISP, XX-> gc, region);
   XDestroyRegion( region);
   return true;
}

Bool
apc_gp_set_back_color( Handle self, Color color)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      prima_allocate_color( self, color, &XX-> back);
      XX-> flags. brush_back = 0;
   } else 
      XX-> saved_back = color;
   return true;
}

Bool
apc_gp_set_color( Handle self, Color color)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      prima_allocate_color( self, color, &XX-> fore);
      XX-> flags. brush_fore = 0;
   } else 
      XX-> saved_fore = color;
   return true;
}

Bool
apc_gp_set_fill_pattern( Handle self, FillPattern pattern)
{
   DEFXX;
   if ( memcmp( pattern, XX-> fill_pattern, sizeof(FillPattern)) == 0)
      return true;
   XX-> flags. brush_null_hatch = 
     ( memcmp( pattern, fillPatterns[fpSolid], sizeof(FillPattern)) == 0);
   memcpy( XX-> fill_pattern, pattern, sizeof( FillPattern));
   return true;
}

/*- see apc_font.c
void
apc_gp_set_font( Handle self, PFont font)
*/

Bool
apc_gp_set_line_end( Handle self, int lineEnd)
{
   DEFXX;
   int cap = CapButt;
   XGCValues gcv;

   if ( lineEnd == leFlat)
      cap = CapButt;
   else if ( lineEnd == leSquare)
      cap = CapProjecting;
   else if ( lineEnd == leRound)
      cap = CapRound;

   if ( XF_IN_PAINT(XX)) {
      gcv. cap_style = cap;
      XChangeGC( DISP, XX-> gc, GCCapStyle, &gcv);
      XCHECKPOINT;
   } else {
      XX-> gcv. cap_style = cap;
   }
   return true;
}

Bool
apc_gp_set_line_width( Handle self, int line_width)
{
   DEFXX;
   XGCValues gcv;

   if ( XF_IN_PAINT(XX)) {
      XX-> line_width = gcv. line_width = line_width;
      XChangeGC( DISP, XX-> gc, GCLineWidth, &gcv);
      XCHECKPOINT;
   } else
      XX-> gcv. line_width = line_width;

   return true;
}

Bool
apc_gp_set_line_pattern( Handle self, unsigned char *pattern, int len)
{
   DEFXX;
   XGCValues gcv;

   if ( XF_IN_PAINT(XX)) {
      if ( len == 0 || (len == 1 && pattern[0] == 1)) {
	 gcv. line_style = LineSolid;
	 XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      } else {
	 gcv. line_style = ( XX-> paint_rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;
	 XSetDashes( DISP, XX-> gc, 0, (char*)pattern, len);
	 XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      }
      XX-> line_style = gcv. line_style;
      free(XX->paint_dashes);
      if (( XX-> paint_dashes = malloc( len)))
         memcpy( XX-> paint_dashes, pattern, len);
      XX-> paint_ndashes = len;
   } else {
      free( XX-> dashes);
      if ( len == 0) {					/* lpNull */
	 XX-> dashes = nil;
	 XX-> ndashes = -1;
	 XX-> gcv. line_style = LineSolid;
      } else if ( len == 1 && pattern[0] == 1) {	/* lpSolid */
	 XX-> dashes = nil;
	 XX-> ndashes = 0;
	 XX-> gcv. line_style = LineSolid;
      } else {						/* the rest */
	 XX-> dashes = malloc( len);
	 memcpy( XX-> dashes, pattern, len);
	 XX-> ndashes = len;
	 XX-> gcv. line_style = ( XX-> rop2 == ropNoOper) ? LineOnOffDash : LineDoubleDash;
      }
   }
   return true;
}

Bool
apc_gp_set_rop( Handle self, int rop)
{
   DEFXX;
   int function;

   if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
      function = GXnoop;
   else
      function = rop_map[ rop];

   if ( XF_IN_PAINT(XX)) {
      if ( rop < 0 || rop >= sizeof( rop_map)/sizeof(int))
         rop = ropNoOper;
      XX-> paint_rop = rop;
      XSetFunction( DISP, XX-> gc, function);
      XCHECKPOINT;
   } else {
      XX-> gcv. function = function;
      XX-> rop = rop;
   }
   return true;
}

Bool
apc_gp_set_rop2( Handle self, int rop)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      if ( XX-> paint_rop2 == rop) return true;
      XX-> paint_rop2 = ( rop == ropCopyPut) ? ropCopyPut : ropNoOper;
      if ( XX-> line_style != LineSolid) {
         XGCValues gcv;
         gcv. line_style = ( rop == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
         XChangeGC( DISP, XX-> gc, GCLineStyle, &gcv);
      }   
   } else {
      XX-> rop2 = rop;
      if ( XX-> gcv. line_style != LineSolid)
         XX-> gcv. line_style = ( rop == ropCopyPut) ? LineDoubleDash : LineOnOffDash;
   }   
   return true;
}

Bool
apc_gp_set_transform( Handle self, int x, int y)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      XX-> gtransform. x = x;
      XX-> gtransform. y = y;
   } else {
      XX-> transform. x = x;
      XX-> transform. y = y;
   }
   return true;
}

Bool
apc_gp_set_text_opaque( Handle self, Bool opaque)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      XX-> flags. paint_opaque = !!opaque;
   } else {
      XX-> flags. opaque = !!opaque;
   }
   return true;
}

Bool
apc_gp_set_text_out_baseline( Handle self, Bool baseline)
{
   DEFXX;
   if ( XF_IN_PAINT(XX)) {
      XX-> flags. paint_base_line = !!baseline;
   } else {
      XX-> flags. base_line = !!baseline;
   }
   return true;
}

ApiHandle
apc_gp_get_handle( Handle self)
{
   return ( ApiHandle) X(self)-> gdrawable;
}

