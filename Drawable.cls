#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
use Types;

object Prima::Drawable ( Prima::Component)
{
   Font        font;
   int         w;
   int         h;
   PRGBColor   palette;		       # logical palette
   int         palSize;                # logical palette size

   # globals
   method void   init( HV * profile);
   method void   done();
   method void   cleanup();
   method Bool   begin_paint();
   method Bool   begin_paint_info();
   method void   end_paint();
   method void   end_paint_info();
   static Font * font_match( char * dummy, Font * source, Font * dest, Bool pick = true);
   c_only Bool	 font_add( Font * source, Font * dest);
   import SV *   profile_default();

   # properties
   method void   set( HV * profile);
   method Bool   set_back_color( Color color)       => apc_gp_set_back_color;
   method Bool   set_clip_rect( Rect clipRect)      => apc_gp_set_clip_rect;
   method Bool   set_color( Color color)            => apc_gp_set_color;
   method void   set_fill_pattern ( FillPattern pattern);
   method void   set_fill_pattern_id ( int patternId);
   method void   set_font( Font font);
   method Bool   set_line_end( int lend)           => apc_gp_set_line_end;
   method Bool   set_line_pattern( int lpattern)   => apc_gp_set_line_pattern;
   method Bool   set_line_width( int lwidth)       => apc_gp_set_line_width;
   method void   set_palette( SV * palette);
   method void   set_region( Handle mask);
   method Bool   set_rop  ( int rop)               => apc_gp_set_rop;
   method Bool   set_rop2 ( int rop)               => apc_gp_set_rop2;
   method Bool   set_transform ( int x, int y)     => apc_gp_set_transform;
   method Bool   set_text_opaque( Bool opaque)     => apc_gp_set_text_opaque;
   method Bool   set_text_out_baseline( Bool bas)  => apc_gp_set_text_out_baseline;

   method int           get_bpp();
   method Color         get_back_color()                  => apc_gp_get_back_color;
   method Rect          get_clip_rect()                   => apc_gp_get_clip_rect;
   method Color         get_color()                       => apc_gp_get_color;
   method FillPattern * get_fill_pattern();
   method Font          get_font();
   method SV *          get_font_abc();
   method SV *          get_handle();
   method int           get_height();
   method int           get_line_end()                    => apc_gp_get_line_end;
   method int           get_line_pattern()                => apc_gp_get_line_pattern;
   method int           get_line_width()                  => apc_gp_get_line_width;
   method Color         get_nearest_color( Color color);
   method int           get_paint_state();
   method SV *          get_palette();
   method SV *          get_physical_palette();
   method Handle        get_region();
   method Point         get_resolution();
   method int           get_rop()                         => apc_gp_get_rop;
   method int           get_rop2()                        => apc_gp_get_rop2;
   method Point         get_size();
   method int           get_text_width( char * text, int len = -1, Bool addOverhang = 0);
   method SV *          get_text_box( char * text, int len = -1);
   method Point         get_transform()                   => apc_gp_get_transform;
   method Bool          get_text_opaque()                 => apc_gp_get_text_opaque;
   method Bool          get_text_out_baseline()           => apc_gp_get_text_out_baseline;
   method int           get_width();

   # operators
   method Bool  arc( int x, int y, int Rx, int Ry, double startAngle, double endAngle) => apc_gp_arc;
   method Bool  bar(int x1, int y1, int x2, int y2)        => apc_gp_bar;
   method Bool  chord( int x, int y, int Rx, int Ry, double startAngle, double endAngle) => apc_gp_chord;
   method Color get_pixel(int x, int y)                    => apc_gp_get_pixel;
   method Bool  ellipse(int x, int y, int Rx, int Ry)      => apc_gp_ellipse;
   method Bool  fill_chord( int x, int y, int Rx, int Ry, double startAngle, double endAngle) => apc_gp_fill_chord;
   method Bool  fill_ellipse(int x, int y, int Rx, int Ry) => apc_gp_fill_ellipse;
   method void  fillpoly( SV * points);
   method Bool  fill_sector( int x, int y, int Rx, int Ry, double startAngle, double endAngle) => apc_gp_fill_sector;
   method Bool  flood_fill( int x, int y, Color color, Bool singleBorder = 1) => apc_gp_flood_fill;
   method Bool  line(int x1, int y1, int x2, int y2)       => apc_gp_line;
   method void  lines( SV * points);
   method void  put_image(int x, int y, Handle image);
   method void  polyline( SV * points);
   method void  put_image_indirect( Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop);
   method Bool  rectangle(int x1, int y1, int x2, int y2)  => apc_gp_rectangle;
   method Bool  sector( int x, int y, int Rx, int Ry, double startAngle, double endAngle) => apc_gp_sector;
   method Bool  set_pixel(int x, int y, Color color)       => apc_gp_set_pixel;
   method void  stretch_image(int x, int y, int xDest, int yDest, Handle image);
   method void  text_out( char * text, int x, int y, int len = -1);
   method SV*   text_wrap( char * text, int width, int options = twDefault, int tabIndent = 8, int textLen = -1);
}