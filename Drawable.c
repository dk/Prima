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

#include "apricot.h"
#include "Drawable.h"
#include "Image.h"
#include <Drawable.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)
#define var (( PDrawable) self)

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER           if ( !inPaint) my-> begin_paint_info( self)
#define gpLEAVE           if ( !inPaint) my-> end_paint_info( self)

void
Drawable_init( Handle self, HV * profile)
{
   inherited init( self, profile);
   apc_gp_init( self);
   var-> w = var-> h = 0;
   my-> set_color        ( self, pget_i ( color));
   my-> set_backColor    ( self, pget_i ( backColor));
   my-> set_fillPattern  ( self, pget_sv( fillPattern));
   my-> set_lineEnd      ( self, pget_i ( lineEnd));
   my-> set_linePattern  ( self, pget_sv( linePattern));
   my-> set_lineWidth    ( self, pget_i ( lineWidth));
   my-> set_region       ( self, pget_H ( region));
   my-> set_rop          ( self, pget_i ( rop));
   my-> set_rop2         ( self, pget_i ( rop2));
   my-> set_textOpaque   ( self, pget_B ( textOpaque));
   my-> set_textOutBaseline( self, pget_B ( textOutBaseline));
   if ( pexist( transform))
   {
      AV * av = ( AV *) SvRV( pget_sv( transform));
      Point tr = {0,0};
      SV ** holder = av_fetch( av, 0, 0);
      if ( holder) tr.x = SvIV( *holder); else warn("RTC0059: Array panic on 'transform'");
      holder = av_fetch( av, 1, 0);
      if ( holder) tr.y = SvIV( *holder); else warn("RTC0059: Array panic on 'transform'");
      my-> set_transform( self, tr);
   }
   SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::init");
   my-> set_font( self, Font_buffer);
   my-> set_palette( self, pget_sv( palette));
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
      my-> end_paint_info( self);
   if ( is_opt( optInDraw))
      my-> end_paint( self);
   inherited cleanup( self);
}

Bool
Drawable_begin_paint( Handle self)
{
   if ( var-> stage > csFrozen) return false;
   if ( is_opt( optInDrawInfo)) my-> end_paint_info( self);
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
   if ( var-> stage > csFrozen) return false;
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
      my-> set_font( self, Font_buffer);
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
      my-> set_transform( self, tr);
      pdelete( transform);
   }
   if ( pexist( width) && pexist( height)) {
      Point size = { pget_i( width), pget_i( height)};
      my-> set_size( self, size);
      pdelete( width);
      pdelete( height);
   }
   inherited set( self, profile);
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
   if ( useName  ) 
      strcpy( dest-> name, source-> name);

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

SV *
Drawable_linePattern( Handle self, Bool set, SV * pattern)
{
   if ( set) {
      STRLEN len;
      unsigned char *pat = ( unsigned char *) SvPV( pattern, len);
      if ( len > 255) len = 255;
      apc_gp_set_line_pattern( self, pat, len);
   } else {
      unsigned char ret[ 256];
      int len = apc_gp_get_line_pattern( self, ret);
      return newSVpvn((char*) ret, len);
   }
   return nilSV;
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
Drawable_resolution( Handle self, Bool set, Point resolution)
{
   gpARGS;
   if ( set)
      croak("Attempt to write read-only property %s", "Drawable::resolution");
   gpENTER;
   resolution = apc_gp_get_resolution( self);
   gpLEAVE;
   return resolution;
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
Drawable_get_font_abc( Handle self, int first, int last)
{
   int i;
   AV * av = newAV();
   PFontABC abc;

   if ( first < 0) first = 0;
   if ( last  < 0) last  = 255;
   if ( first > 255) first = 255;
   if ( last  > 255) last  = 255;

   if ( first > last)
     abc = nil;
   else {
     gpARGS;
     gpENTER;
     abc = apc_gp_get_font_abc( self, first, last);
     gpLEAVE;
   }

   if ( abc != nil) {
      for ( i = 0; i <= last - first; i++) {
         av_push( av, newSVnv( abc[ i]. a));
         av_push( av, newSVnv( abc[ i]. b));
         av_push( av, newSVnv( abc[ i]. c));
      }
      free( abc);
   }
   return newRV_noinc(( SV *) av);
}


SV *
Drawable_get_handle( Handle self)
{
   char buf[ 256];
   snprintf( buf, 256, "0x%08lx", apc_gp_get_handle( self));
   return newSVpv( buf, 0);
}

int
Drawable_height( Handle self, Bool set, int height)
{
   Point p = my-> get_size( self);
   if ( !set)
      return p. y;
   p. y = height;
   my-> set_size( self, p);
   return height;
}

Point
Drawable_size ( Handle self, Bool set, Point size)
{
   if ( set)
      croak("Attempt to write read-only property %s", "Drawable::size");
   size. x = var-> w;
   size. y = var-> h;
   return size;
}

int
Drawable_width( Handle self, Bool set, int width)
{
   Point p = my-> get_size( self);
   if ( !set)
      return p. x;
   p. x = width;
   my-> set_size( self, p);
   return width;
}

void
Drawable_put_image( Handle self, int x , int y , Handle image )
{
   Point size;
   if ( image == nilHandle) return;
   size = ((( PDrawable) image)-> self)-> get_size( image);
   apc_gp_put_image ( self, image, x, y, 0, 0, size.x, size.y, my-> get_rop( self));
}

void
Drawable_stretch_image(Handle self, int x, int y, int xDest, int yDest, Handle image)
{
   Point size;
   if ( image == nilHandle) return;
   size = ((( PDrawable) image)-> self)-> get_size( image);
   apc_gp_stretch_image ( self, image, x, y, 0, 0, xDest, yDest, size.x, size.y,  my-> get_rop( self));
}

void
Drawable_put_image_indirect( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
   if ( image == nilHandle) return;
   apc_gp_stretch_image ( self, image, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
}


void
Drawable_text_out( Handle self, char * text, int x, int y, int len)
{
   if ( len < 0) len = strlen( text);
   apc_gp_text_out( self, text, x, y, len);
}

void
polypoints( Handle self, SV * points, char * procName, int mod, Bool (*procPtr)(Handle,int,Point*))
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
   p = allocn( Point, count);
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

   if ( len < 0) len = strlen( text);
   gpENTER;
   p = apc_gp_get_text_box( self, text, len);
   gpLEAVE;
   av = newAV();
   for ( i = 0; i < 5; i++) {
      av_push( av, newSViv( p[ i]. x));
      av_push( av, newSViv( p[ i]. y));
   };
   free( p);
   return newRV_noinc(( SV *) av);
}

static char **
do_text_wrap( Handle self, TextWrapRec *t, PFontABC abc)
{
   float width[ 256];
   int start = 0, i, lSize = 16;
   float w = 0, inc = 0;
   char **ret;
   Bool wasTab    = 0;
   Bool doWidthBreak = t-> width >= 0;
   int tildeIndex = -100, tildeLPos = 0, tildeLine = 0, tildePos = 0, tildeOffset = 0;
   unsigned char * text    = ( unsigned char*) t-> text;

   ret = allocn( char*, lSize);
   for ( i = 0; i < 256; i++) {
      width[i] = abc[i]. a + abc[i]. b + abc[i]. c;
      abc[i]. c = ( abc[i]. c < 0) ? - abc[i]. c : 0;
      abc[i]. a = ( abc[i]. a < 0) ? - abc[i]. a : 0;
   }

// macro for adding string/chunk into result table
#define lAdd(end) {                                       \
   int l = end - start;                                   \
   char *c = nil;                                         \
   if (!( t-> options & twReturnChunks)) {                \
      c = allocs( l + 1);                                 \
      memcpy( c, &text[start], l);                        \
      c[ l] = 0;                                          \
   }                                                      \
   if ( tildeIndex >= 0 && tildeIndex >= start &&         \
        tildeIndex < end)                                 \
   {                                                      \
      tildeLine = t-> t_line = t-> count;                 \
      tildePos = tildeLPos   = tildeIndex - start;        \
      if ( tildeIndex == end - 1) {                       \
         t-> t_line++;                                    \
         tildeLPos = 0;                                   \
      }                                                   \
   }                                                      \
   if ( t-> count == lSize) {                             \
      char ** n = allocn( char*, lSize + 16);             \
      memcpy( n, ret, sizeof( char*) * lSize);            \
      lSize += 16;                                        \
      free( ret);                                         \
      ret = n;                                            \
   }                                                      \
   if ( t-> options & twReturnChunks) {                   \
      ret[ t-> count++] = (char*) start;                  \
      ret[ t-> count++] = (char*) l;                      \
   } else                                                 \
      ret[ t-> count++] = c;                              \
   start += l;                                            \
}

// determining ~ character location
    if ( t-> options & twCalcMnemonic) {
       for ( i = 0; i < t-> textLen - 1; i++)
          if ( text[ i] == '~') {
             unsigned char c = text[ i + 1];
             if ( c == '~' || c < ' ') {
                i++;
                continue;
             } else {
                tildeIndex = i;
                break;
             }
          }
    }
// scanning line accumulating widths and breaking if necessary
    t-> count = 0;
    w = abc[ text[ 0]]. a;
    for ( i = 0; i < t-> textLen; i++)
    {
       float winc;

       switch ( text[ i])
       {
          case '\t':
             if (!( t-> options & twCalcTabs)) goto _default;
             if ( t-> options & twSpaceBreak)
             {
                lAdd( i); start++;
                w = abc[ text[ i + 1]]. a;
                continue;
             }
             winc = width[' '] * t-> tabIndent;
             inc  = abc[' ']. c;
             wasTab = true;
             break;
          case '\n':
             if (!( t-> options & twNewLineBreak)) goto _default;
             lAdd( i); start++; w = abc[ text[ i + 1]]. a;
             continue;
          case ' ':
             if (!( t-> options & twSpaceBreak)) goto _default;
             lAdd( i); start++; w = abc[ text[ i + 1]]. a;
             continue;
          case '~':
             if ( i == tildeIndex) {
                tildeOffset = w;
                inc = winc = 0;
                break;
             }
          _default: default:
             winc = width[ text[ i]];
             inc  = abc[ text[ i]]. c;
       }
       
       if ( doWidthBreak && w + winc + inc > t-> width)
       {
          if (( i - start == 0) || (( i - 1 == tildeIndex) && ( i - start == 1))) {
            // case when even single char could not fit in given width.
             if ( t-> options & twBreakSingle)
             {
                // do not return anything in this case
                int j;
                if (!( t-> options & twReturnChunks)) {
                   for ( j = 0; j < t-> count; j++) free( ret[ j]);
                   ret[ 0] = duplicate_string("");
                }
                t-> count = 0;
                return ret;
             } else
                // or fit this character
                lAdd( i + 1);
          } else {
             unsigned char * c;
             lAdd( i);
             if ( t-> options & twWordBreak)
             {
                // checking whether break was at word boundary
                unsigned char rc = text[ i];
                int len;
                if ( t-> options & twReturnChunks)
                {
                   c   = &text[ (int)ret[ t-> count - 2]];
                   len = (int) ret[ t-> count - 1];
                }
                else  {
                  c = ( unsigned char *) ret[ t-> count - 1];
                  len = strlen(( const char *) c);
                }
                if ( rc != ' ' && rc != '\t' && rc != '\n') {
                   // determining whether this line could be split
                   int j;
                   Bool ok = false;
                   for ( j = len; j >= 0; j--)
                      if ( c[ j] == ' ' || c[ j] == '\n' || c[ j] == '\t') {
                         ok = true;
                         break;
                      }
                   if ( ok)
                   {
                      start -= len - j - 1;
                      if ( t-> options & twReturnChunks)
                         ret[ t-> count - 1] = ( char *) j;
                      else
                         c[ j] = 0;
                      i -= len - j - 1;
                   }
                }
             }
             i--; // repeat again
          }
          w = 0;
          continue;
       } else
          w += winc;
    }
// adding or skipping last line
    if ( t-> textLen - start > 0 || t-> count == 0) lAdd( t-> textLen);
// removing ~ and determining it's location
    if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
        unsigned char *l = ( unsigned char *) ret[ tildeLine];
        t-> t_char = l[ tildePos+1];
        if ( t-> options & twCollapseTilde)
           memmove(( void *)( l + tildePos), l + tildePos + 1, strlen(( const char *) l) - tildePos);
        l = ( unsigned char *) ret[ t-> t_line];
        w = tildeOffset;
        t-> t_start = w - 1;
        t-> t_end   = w + width[(unsigned)l[tildeLPos]];
    } else {
        t-> t_start = t-> t_end = t-> t_line = -1;
    }
// expanding tabs
    if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab)
    {
       for ( i = 0; i < t-> count; i++)
       {
          int tabs = 0, len = 0;
          char *substr = ret[ i], *n;
          while (*substr) { if ( *substr == '\t') tabs++; substr++; len++; }
          if ( tabs == 0) continue;
          n = allocs( len + tabs * t-> tabIndent + 1);
          substr = ret[ i];
          len = 0;
          while ( *substr)
          {
             if ( *substr == '\t')
             {
                int j = t-> tabIndent;
                while ( j--) n[ len++] = ' ';
             } else
                n[ len++] = *substr;
             substr++;
          }
          free( ret[ i]);
          n[ len] = 0;
          ret[ i] = n;
       }
    }
    return ret;
}

SV*
Drawable_text_wrap( Handle self, char * text, int width, int options, int tabIndent, int textLen)
{
   gpARGS;
   TextWrapRec t;
   Bool retChunks;
   char** c;
   int i;
   AV * av;
   PFontABC abc;

   t. text = text;
   t. textLen = textLen;
   t. width = width;
   t. tabIndent = tabIndent;
   t. options = options;
   retChunks = t. options & twReturnChunks;

   av = newAV();
   if ( t. tabIndent < 0) t. tabIndent = 0;
   if ( t. textLen   < 0) t. textLen   = strlen( t. text);

   if ( my-> get_font_abc == Drawable_get_font_abc) {
      gpENTER;
      abc = apc_gp_get_font_abc( self, 0, 255);
      gpLEAVE;
   } else {
      SV * sv = my-> get_font_abc( self, 0, 255);
      if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
         AV * av = ( AV*) SvRV( sv);
         int n   = av_len( av) + 1;
         if ( n > 256 * 3) n = 256 * 3;
         n = ( n / 3) * 3;
         if (( abc = malloc( 256 * sizeof( FontABC)))) {
            int i, j = 0;
            if ( n < 256) memset( abc, 0, 256 * sizeof( FontABC));
            for ( i = 0; i < n; i += 3) {
               SV ** holder = av_fetch( av, i, 0);
               if ( holder) abc[j]. a = ( float) SvNV( *holder);
               holder = av_fetch( av, i + 1, 0);
               if ( holder) abc[j]. b = ( float) SvNV( *holder);
               holder = av_fetch( av, i + 2, 0);
               if ( holder) abc[j]. c = ( float) SvNV( *holder);
               j++;
            }   
         } 
      } 
      sv_free( sv);
   }
   if ( abc == nil) return nil;
   
   c = do_text_wrap( self, &t, abc);
   free( abc);

   for ( i = 0; i < t. count; i++) {
      av_push( av, retChunks ? ( newSViv(( int) c[ i])) : ( newSVpv( c[ i], 0)));
      if ( !retChunks) free( c[i]);
   }
   free( c);
   if  ( t. options & ( twCalcMnemonic | twCollapseTilde))
   {
      HV * profile = newHV();
      char b[2] = { 0, 0};
      b[ 0] = t. t_char;
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

   buf = allocb( count);

   for ( i = 0; i < count; i++)
   {
      SV **itemHolder = av_fetch( av, i, 0);
      if ( itemHolder == nil)
         return ( PRGBColor) buf;
      buf[ i] = SvIV( *itemHolder);
   }

   return ( PRGBColor) buf;
}

Color
Drawable_backColor( Handle self, Bool set, Color color)
{
   if (!set) return apc_gp_get_back_color( self);
   apc_gp_set_back_color( self, color);
   return color;
}

Color
Drawable_color( Handle self, Bool set, Color color)
{
   if (!set) return apc_gp_get_color( self);
   apc_gp_set_color( self, color);
   return color;
}

Rect
Drawable_clipRect( Handle self, Bool set, Rect clipRect)
{
   if ( !set)
      return apc_gp_get_clip_rect( self);
   apc_gp_set_clip_rect( self, clipRect);
   return clipRect;
}

int
Drawable_lineEnd( Handle self, Bool set, int lineEnd)
{
   if (!set) return apc_gp_get_line_end( self);
   apc_gp_set_line_end( self, lineEnd);
   return lineEnd;
}

int
Drawable_lineWidth( Handle self, Bool set, int lineWidth)
{
   if (!set) return apc_gp_get_line_width( self);
   apc_gp_set_line_width( self, lineWidth);
   return lineWidth;
}

SV *
Drawable_palette( Handle self, Bool set, SV * palette)
{
   int colors;
   if ( var-> stage > csFrozen) return nilSV;
   colors = var-> palSize;
   if ( set) {
      free( var-> palette);
      var-> palette = read_palette( &var-> palSize, palette);
      if ( colors == 0 && var-> palSize == 0) return nilSV; // do not bother apc
      apc_gp_set_palette( self);
   } else {
      AV * av = newAV();
      int i;
      Byte * pal = ( Byte*) var-> palette;
      for ( i = 0; i < colors * 3; i++) av_push( av, newSViv( pal[ i]));
      return newRV_noinc(( SV *) av);
   }
   return nilSV;
}

Color
Drawable_pixel( Handle self, Bool set, int x, int y, Color color)
{
   if (!set) {
      return apc_gp_get_pixel( self, x, y);
   }
   apc_gp_set_pixel( self, x, y, color);
   return color;
}

Handle
Drawable_region( Handle self, Bool set, Handle mask)
{
   if ( var-> stage > csFrozen) return nilHandle;

   if ( set) {
      if ( mask && !kind_of( mask, CImage)) {
         warn("RTC005A: Illegal object reference passed to Drawable::region");
         return nilHandle;
      }

      if ( mask && (( PImage( mask)-> type & imBPP) != imbpp1)) {
         Handle i = CImage( mask)-> dup( mask);
         ++SvREFCNT( SvRV( PImage( i)-> mate));
         CImage( i)-> set_conversion( i, ictNone);
         CImage( i)-> set_type( i, imBW);
         apc_gp_set_region( self, i);
         --SvREFCNT( SvRV( PImage( i)-> mate));
         Object_destroy( i);
      } else
         apc_gp_set_region( self, mask);

   } else if ( apc_gp_get_region( self, nilHandle)) {
      HV * profile = newHV();
      Handle i = Object_create( "Prima::Image", profile);
      sv_free(( SV *) profile);
      apc_gp_get_region( self, i);
      --SvREFCNT( SvRV((( PAnyObject) i)-> mate));
      return i;
   }

   return nilHandle;
}

int
Drawable_rop( Handle self, Bool set, int rop)
{
   if (!set) return apc_gp_get_rop( self);
   apc_gp_set_rop( self, rop);
   return rop;
}

int
Drawable_rop2( Handle self, Bool set, int rop2)
{
   if (!set) return apc_gp_get_rop2( self);
   apc_gp_set_rop2( self, rop2);
   return rop2;
}

Bool
Drawable_textOpaque( Handle self, Bool set, Bool opaque)
{
   if (!set) return apc_gp_get_text_opaque( self);
   apc_gp_set_text_opaque( self, opaque);
   return opaque;
}

Bool
Drawable_textOutBaseline( Handle self, Bool set, Bool textOutBaseline)
{
   if (!set) return apc_gp_get_text_out_baseline( self);
   apc_gp_set_text_out_baseline( self, textOutBaseline);
   return textOutBaseline;
}

Point
Drawable_transform( Handle self, Bool set, Point transform)
{
   if (!set) return apc_gp_get_transform( self);
   apc_gp_set_transform( self, transform. x, transform. y);
   return transform;
}

SV *
Drawable_fillPattern( Handle self, Bool set, SV * svpattern)
{
   int i;
   if ( !set) {
      AV * av;
      FillPattern * fp = apc_gp_get_fill_pattern( self);
      if ( !fp) return nilSV;
      av = newAV();
      for ( i = 0; i < 8; i++) av_push( av, newSViv(( int) (*fp)[i]));
      return newRV_noinc(( SV *) av);
   } else {
      if ( SvROK( svpattern) && ( SvTYPE( SvRV( svpattern)) == SVt_PVAV)) {
          FillPattern fp;
          AV * av = ( AV *) SvRV( svpattern);
          if ( av_len( av) != 7) {
             warn("RTC0056: Illegal fillPattern passed to Drawable::fillPattern");
             return nilSV;
          }
          for ( i = 0; i < 8; i++) {
            SV ** holder = av_fetch( av, i, 0);
            if ( !holder) {
               warn("RTC0057: Array panic on Drawable::fillPattern");
               return nilSV;
            }
            fp[ i] = SvIV( *holder);
         }
         apc_gp_set_fill_pattern( self, fp);
      } else {
         int id = SvIV( svpattern);
         if (( id < 0) || ( id > fpMaxId)) {
            warn("RTC0058: fillPattern index out of range passed to Drawable::fillPattern");
            return nilSV;
         }
         apc_gp_set_fill_pattern( self, fillPatterns[ id]);
      }
   }
   return nilSV;
}

Font
Drawable_get_font( Handle self)
{
   return var-> font;
}

void
Drawable_set_font( Handle self, Font font)
{
   apc_font_pick( self, &font, &var-> font);
   apc_gp_set_font( self, &var-> font);
}


#ifdef __cplusplus
}
#endif
