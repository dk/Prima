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

#include "apricot.h"
#include "Drawable.h"
#include "Drawable.inc"

#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)->
#define var (( PDrawable) self)->

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER           if ( !inPaint) my begin_paint_info( self)
#define gpLEAVE           if ( !inPaint) my end_paint_info( self)

PRGBColor read_palette( int * palSize, SV * palette);

void
Drawable_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   apc_gp_init( self);
   var w = var h = 0;
   my set_color        ( self, pget_i ( color));
   my set_back_color   ( self, pget_i ( backColor));
   {
      SV * sv = pget_sv( fillPattern);
      if ( SvROK( sv)) {
         int i;
         FillPattern fp;
         AV * av = ( AV *) SvRV( sv);
         if ( av_len( av) == 7) {
            for ( i = 0; i < 8; i++) {
               SV ** holder = av_fetch( av, i, 0);
               if ( !holder) {
                  warn("RTC0058: Array panic on 'fillPattern'");
                  break;
               }
               fp[ i] = SvIV( *holder);
            }
            my set_fill_pattern( self, fp);
         } else {
            warn("RTC0056: Illegal fillPattern passed to Drawable::init");
            my set_fill_pattern_id( self, fpSolid);
         }
      } else
         my set_fill_pattern_id( self, SvIV( sv));
   }
   my set_line_end     ( self, pget_i ( lineEnd));
   my set_line_pattern ( self, pget_i ( linePattern));
   my set_line_width   ( self, pget_i ( lineWidth));
   my set_rop          ( self, pget_i ( rop));
   my set_rop2         ( self, pget_i ( rop2));
   my set_text_opaque  ( self, pget_B ( textOpaque));
   my set_text_out_baseline( self, pget_B ( textOutBaseline));
   if ( pexist( transform))
   {
      AV * av = ( AV *) SvRV( pget_sv( transform));
      Point tr = {0,0};
      SV ** holder = av_fetch( av, 0, 0);
      if ( holder) tr.x = SvIV( *holder); else warn("RTC0059: Array panic on 'transform'");
      holder = av_fetch( av, 1, 0);
      if ( holder) tr.y = SvIV( *holder); else warn("RTC0059: Array panic on 'transform'");
      my set_transform( self, tr.x, tr.y);
   }
   SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::init");
   my set_font( self, Font_buffer);
   my set_palette( self, pget_sv( palette));
}

void
Drawable_done( Handle self)
{
   apc_gp_done( self);
   inherited done( self);
}

void
Drawable_cleanup( Handle self)
{
   if ( is_opt( optInDrawInfo))
      my end_paint_info( self);
   if ( is_opt( optInDraw))
      my end_paint( self);
   inherited cleanup( self);
}

Bool
Drawable_begin_paint( Handle self)
{
   if ( is_opt( optInDrawInfo)) my end_paint_info( self);
   opt_set( optInDraw);
   return true;
}

void
Drawable_end_paint( Handle self)
{
   opt_clear( optInDraw);
}

Bool
Drawable_begin_paint_info( Handle self)
{
   if ( is_opt( optInDraw))     return true;
   if ( is_opt( optInDrawInfo)) return false;
   opt_set( optInDrawInfo);
   return true;
}

void
Drawable_end_paint_info( Handle self)
{
   opt_clear( optInDrawInfo);
}


void Drawable_set( Handle self, HV * profile)
{
   if ( pexist( font))
   {
      SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::set");
      my set_font( self, Font_buffer);
      pdelete( font);
   }
   if ( pexist( transform))
   {
      AV * av = ( AV *) SvRV( pget_sv( transform));
      Point tr = {0,0};
      SV ** holder = av_fetch( av, 0, 0);
      if ( holder) tr.x = SvIV( *holder); else warn("RTC0059: Array panic on 'transform'");
      holder = av_fetch( av, 1, 0);
      if ( holder) tr.y = SvIV( *holder); else warn("RTC0059: Array panic on 'transform'");
      my set_transform( self, tr.x, tr.y);
      pdelete( transform);
   }
   if ( pexist( fillPattern))
   {
      SV * sv = pget_sv( fillPattern);
      if ( SvROK( sv))
      {
         int i;
         FillPattern fp;
         AV * av = ( AV *) SvRV( sv);
         if ( av_len( av) == 7) {
            for ( i = 0; i < 8; i++) {
               SV ** holder = av_fetch( av, i, 0);
               if ( !holder) {
                  warn("RTC0058: Array panic on 'fillPattern'");
                  break;
               }
               fp[ i] = SvIV( *holder);
            }
            my set_fill_pattern( self, fp);
         } else
            warn("RTC0057: Illegal fillPattern passed to Drawable::set");
      } else
         my set_fill_pattern_id( self, SvIV( sv));
      pdelete( fillPattern);
   }
   inherited set( self, profile);
}

void
Drawable_set_fill_pattern( Handle self, FillPattern pattern)
{
   apc_gp_set_fill_pattern( self, pattern);
}

void
Drawable_set_fill_pattern_id( Handle self, int patternId )
{
   if (( patternId < 0) || ( patternId > fpMaxId)) patternId = fpSolid;
   my set_fill_pattern( self, fillPatterns[ patternId]);
}

void
Drawable_set_font( Handle self, Font font)
{
   apc_font_pick( self, &font, &var font);
   apc_gp_set_font( self, &var font);
}

void
Drawable_set_palette( Handle self, SV * palette)
{
   int ops = var palSize;
   if ( var stage > csNormal) return;
   free( var palette);
   var palette = read_palette( &var palSize, palette);
   if ( ops == 0 && var palSize == 0) return; // do not bother apc
   apc_gp_set_palette( self);
}


Font *
Drawable_font_match( char * dummy, Font * source, Font * dest, Bool pick)
{
   if ( pick)
      apc_font_pick( nilHandle, source, dest);
   else
      Drawable_font_add( nilHandle, source, dest);
   return dest;
}

Bool
Drawable_font_add( Handle self, Font * source, Font * dest)
{
   Bool useHeight = source-> height    != C_NUMERIC_UNDEF;
   Bool useWidth  = source-> width     != C_NUMERIC_UNDEF;
   Bool useSize   = source-> size      != C_NUMERIC_UNDEF;
   Bool usePitch  = source-> pitch     != C_NUMERIC_UNDEF;
   Bool useStyle  = source-> style     != C_NUMERIC_UNDEF;
   Bool useDir    = source-> direction != C_NUMERIC_UNDEF;
   Bool useName   = strcmp( source-> name, C_STRING_UNDEF) != 0;

   // assignning values
   if ( useHeight) dest-> height    = source-> height;
   if ( useWidth ) dest-> width     = source-> width;
   if ( useDir   ) dest-> direction = source-> direction;
   if ( useStyle ) dest-> style     = source-> style;
   if ( usePitch ) dest-> pitch     = source-> pitch;
   if ( useSize  ) dest-> size      = source-> size;
   if ( useName  ) strcpy( dest-> name, source-> name);

   // nulling dependencies
   if ( !useHeight && useSize)
      dest-> height = 0;
   if ( !useWidth && ( usePitch || useHeight || useName || useSize || useDir || useStyle))
      dest-> width = 0;
   if ( !usePitch && ( useStyle || useName || useDir || useWidth))
      dest-> pitch = fpDefault;
   if ( useHeight)
      dest-> size = 0;

   // validating entries
   if ( dest-> height <= 0) dest-> height = 1;
      else if ( dest-> height > 16383 ) dest-> height = 16383;
   if ( dest-> width  <  0) dest-> width  = 1;
      else if ( dest-> width  > 16383 ) dest-> width  = 16383;
   if ( dest-> size   <= 0) dest-> size   = 1;
      else if ( dest-> size   > 16383 ) dest-> size   = 16383;
   if ( dest-> name[0] == 0)
      strcpy( dest-> name, "Default");

   return useSize && !useHeight;
}


void
Drawable_set_text_out_baseline( Handle self, Bool baseline)
{
   opt_assign( optTextOutBaseLine, baseline);
}

int
Drawable_get_paint_state( Handle self)
{
   if ( is_opt( optInDraw))
      return 1;
   else if ( is_opt( optInDrawInfo))
      return 2;
   else
      return 0;
}

int
Drawable_get_bpp( Handle self)
{
   gpARGS;
   int ret;
   gpENTER;
   ret = apc_gp_get_bpp( self);
   gpLEAVE;
   return ret;
}

Color
Drawable_get_nearest_color( Handle self, Color color)
{
   gpARGS;
   gpENTER;
   color = apc_gp_get_nearest_color( self, color);
   gpLEAVE;
   return color;
}

Point
Drawable_get_resolution( Handle self)
{
   gpARGS;
   Point ret;
   gpENTER;
   ret = apc_gp_get_resolution( self);
   gpLEAVE;
   return ret;
}


SV *
Drawable_get_physical_palette( Handle self)
{
   gpARGS;
   int i, nCol;
   AV * av = newAV();
   PRGBColor r;

   gpENTER;
   r = apc_gp_get_physical_palette( self, &nCol);
   gpLEAVE;

   for ( i = 0; i < nCol; i++) {
      av_push( av, newSViv( r[ i].b));
      av_push( av, newSViv( r[ i].g));
      av_push( av, newSViv( r[ i].r));
   }
   free( r);
   return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_font_abc( Handle self)
{
   gpARGS;
   int i;
   AV * av = newAV();
   PFontABC abc;


   gpENTER;
   abc = apc_gp_get_font_abc( self);
   gpLEAVE;

   for ( i = 0; i < 256; i++) {
      av_push( av, newSVnv( abc[ i]. a));
      av_push( av, newSVnv( abc[ i]. b));
      av_push( av, newSVnv( abc[ i]. c));
   }

   free( abc);
   return newRV_noinc(( SV *) av);
}


FillPattern *
Drawable_get_fill_pattern( Handle self)
{
   return apc_gp_get_fill_pattern( self);
}

Font
Drawable_get_font( Handle self)
{
   return var font;
}

SV *
Drawable_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_gp_get_handle( self));
   return newSVpv( buf, 0);
}


int
Drawable_get_height( Handle self)
{
  return var h;
}

SV *
Drawable_get_palette( Handle self)
{
   AV * av = newAV();
   int i;
   int colors = var palSize;
   Byte * pal = ( Byte*) var palette;
   for ( i = 0; i < colors * 3; i++) av_push( av, newSViv( pal[ i]));
   return newRV_noinc(( SV *) av);
}


Bool
Drawable_get_text_out_baseline( Handle self)
{
   return is_opt( optTextOutBaseLine);
}


Point
Drawable_get_size ( Handle self)
{
   Point ret = { my get_width ( self), my get_height ( self)};
   return ret;
}

int
Drawable_get_width( Handle self)
{
   return var w;
}

void
Drawable_put_image( Handle self, int x , int y , Handle image )
{
   Point size;
   if ( image == nilHandle) return;
   size = ((( PDrawable) image)-> self)-> get_size( image);
   apc_gp_put_image ( self, image, x, y, 0, 0, size.x, size.y, my get_rop( self));
}

void
Drawable_stretch_image(Handle self, int x, int y, int xDest, int yDest, Handle image)
{
   Point size;
   if ( image == nilHandle) return;
   size = ((( PDrawable) image)-> self)-> get_size( image);
   apc_gp_stretch_image ( self, image, x, y, 0, 0, xDest, yDest, size.x, size.y,  my get_rop( self));
}


void
Drawable_text_out( Handle self, char * text, int x, int y, int len)
{
   if ( !is_opt( optTextOutBaseLine) && var font. direction == 0) y += var font. descent;
   if ( len < 0) len = strlen( text);
   apc_gp_text_out( self, text, x, y, len);
}

void
polypoints( Handle self, SV * points, char * procName, int mod, void (*procPtr)(Handle,int,Point*))
{
   AV * av;
   int i, count;
   Point * p;

   if ( !SvROK( points) || ( SvTYPE( SvRV( points)) != SVt_PVAV)) {
      warn("RTC0050: Invalid array reference passed to Drawable::%s", procName);
      return;
   }
   av = ( AV *) SvRV( points);
   count = av_len( av) + 1;
   if ( count % mod) {
      warn("RTC0051: Drawable::%s: Number of elements in an array must be a multiple of %d",
	   procName, mod);
      return;
   }
   count /= 2;
   if ( count < 2) return;
   p = malloc( count * sizeof( Point));
   for ( i = 0; i < count; i++)
   {
       SV** psvx = av_fetch( av, i * 2, 0);
       SV** psvy = av_fetch( av, i * 2 + 1, 0);
       if (( psvx == nil) || ( psvy == nil)) {
          free( p);
          warn("RTC0052: Array panic on item pair %d on Drawable::%s", i, procName);
          return;
       }
       p[ i]. x = SvIV( *psvx);
       p[ i]. y = SvIV( *psvy);
   }
   procPtr( self, count, p);
   free( p);
}


void
Drawable_polyline( Handle self, SV * points)
{
   polypoints( self, points, "polyline", 2, apc_gp_draw_poly);
}

void
Drawable_lines( Handle self, SV * points)
{
   polypoints( self, points, "lines", 4, apc_gp_draw_poly2);
}

void
Drawable_fillpoly( Handle self, SV * points)
{
   polypoints( self, points, "fillpoly", 2, apc_gp_fill_poly);
}

int
Drawable_get_text_width( Handle self, char * text, int len, Bool addOverhang)
{
   gpARGS;
   int res;
   if ( len < 0) len = strlen( text);
   gpENTER;
   res = apc_gp_get_text_width( self, text, len, addOverhang);
   gpLEAVE;
   return res;
}

SV *
Drawable_get_text_box( Handle self, char * text, int len)
{
   gpARGS;
   Point * p;
   AV * av;
   int i;
   int yAdd = 0;

   if ( !is_opt( optTextOutBaseLine) && ( var font. direction == 0))
      yAdd = var font. descent;
   if ( len < 0) len = strlen( text);
   gpENTER;
   p = apc_gp_get_text_box( self, text, len);
   gpLEAVE;
   av = newAV();
   for ( i = 0; i < 5; i++) {
      av_push( av, newSViv( p[ i]. x));
      av_push( av, newSViv( p[ i]. y + yAdd));
   };
   free( p);
   return newRV_noinc(( SV *) av);
}

SV*
Drawable_text_wrap( Handle self, char * text, int width, int options, int tabIndent, int textLen)
{
   gpARGS;
   TextWrapRec t   = {text, textLen, width, tabIndent, options};
   Bool retChunks  = t. options & twReturnChunks;
   char** c;
   int i;
   AV * av;

   av = newAV();
   if ( t. tabIndent < 0) t. tabIndent = 0;
   if ( t. textLen   < 0) t. textLen   = strlen( t. text);

   gpENTER;
   c = apc_gp_text_wrap( self, &t);
   gpLEAVE;

   for ( i = 0; i < t. count; i++) {
      av_push( av, retChunks ? ( newSViv(( int) c[ i])) : ( newSVpv( c[ i], 0)));
      if ( !retChunks) free( c[i]);
   }
   free( c);
   if  ( t. options & ( twCalcMnemonic | twCollapseTilde))
   {
      HV * profile = newHV();
      char b[2] = {t. t_char, 0};
      pset_i( tildeStart, t. t_start);
      pset_i( tildeEnd,   t. t_end);
      pset_i( tildeLine,  t. t_line);
      pset_c( tildeChar,  b);
      av_push( av, newRV_noinc(( SV *) profile));
   }
   return newRV_noinc(( SV *) av);
}

PRGBColor
read_palette( int * palSize, SV * palette)
{
   AV * av;
   int i, count;
   Byte * buf;

   if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV)) {
      *palSize = 0;
      return nil;
   }
   av = (AV *) SvRV( palette);
   count = av_len( av) + 1;
   *palSize = count / 3;
   count = *palSize * 3;
   if ( count == 0) return nil;

   buf = malloc( count);

   for ( i = 0; i < count; i++)
   {
      SV **itemHolder = av_fetch( av, i, 0);
      if ( itemHolder == nil)
         return ( PRGBColor) buf;
      buf[ i] = SvIV( *itemHolder);
   }

   return ( PRGBColor) buf;
}
