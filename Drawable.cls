#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
# $Id$

use Types;

object Prima::Drawable ( Prima::Component)
{
   Font         font;
   int          w;
   int          h;
   PRGBColor    palette;       	       # logical palette
   int          palSize;               # logical palette size
   int          splinePrecision;
   int          splinePrecision_saved;

   property     Color         backColor;
   property     Color         color;
   property     Rect          clipRect;
   property     Bool          fillWinding;
   property     SV * 	      fillPattern;
   property     int           height;
   property     int           lineEnd;
   property     int           lineJoin;
   property     SV *          linePattern;
   property     int           lineWidth;
   property     SV *          palette;
   property     SV *          pixel( int x, int y);
   property     Handle        region;
   property     Point         resolution;
   property     int           rop;
   property     int           rop2;
   property     Point         size;
   property     int           splinePrecision;
   property     Bool          textOpaque;
   property     Bool          textOutBaseline;
   property     Point         translate;
   property     int           width;

   # globals
   method void  init( HV * profile);
   method void  done();
   method void  cleanup();
   method Bool  begin_paint();
   method Bool  begin_paint_info();
   method void  end_paint();
   method void  end_paint_info();
   static Font* font_match( char * dummy, Font * source, Font * dest, Bool pick = true);
   c_only Bool	font_add( Font * source, Font * dest);
   import SV *  profile_default();

   method void  set( HV * profile);
   method void  set_font( Font font);

   method int   get_bpp();
   method Font  get_font();
   method SV *  get_font_abc( int first = -1, int last = -1, Bool unicode = 0);
   method SV *  get_font_ranges();
   method SV *  get_handle();
   method Color get_nearest_color( Color color);
   method int   get_paint_state();
   method SV *  get_physical_palette();
   method int   get_text_width( SV * text, Bool addOverhang = 0);
   method SV *  get_text_box( SV * text);
   static SV *  render_spline( SV * obj, SV * points, int precision = -1);

   # operators
   method Bool  arc( int x, int y, int dX, int dY, double startAngle, double endAngle) => apc_gp_arc;
   method Bool  bar(int x1, int y1, int x2, int y2)        => apc_gp_bar;
   method Bool  chord( int x, int y, int dX, int dY, double startAngle, double endAngle) => apc_gp_chord;
   method Bool  clear(int x1=-1, int y1=-1, int x2=-1, int y2=-1)        => apc_gp_clear;
   method Bool  ellipse(int x, int y, int dX, int dY)      => apc_gp_ellipse;
   method Bool  fill_chord( int x, int y, int dX, int dY, double startAngle, double endAngle) => apc_gp_fill_chord;
   method Bool  fill_ellipse(int x, int y, int dX, int dY) => apc_gp_fill_ellipse;
   method Bool  fillpoly( SV * points);
   method Bool  fill_sector( int x, int y, int dX, int dY, double startAngle, double endAngle) => apc_gp_fill_sector;
   method Bool  fill_spline( SV * points);
   method Bool  flood_fill( int x, int y, Color color, Bool singleBorder = 1) => apc_gp_flood_fill;
   method Bool  line(int x1, int y1, int x2, int y2)       => apc_gp_line;
   method Bool  lines( SV * points);
   method Bool  polyline( SV * points);
   method Bool  put_image_indirect( Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop);
   method Bool  rectangle(int x1, int y1, int x2, int y2)  => apc_gp_rectangle;
   method Bool  sector( int x, int y, int dX, int dY, double startAngle, double endAngle) => apc_gp_sector;
   method Bool  spline( SV * points);
   method Bool  text_out( SV * text, int x, int y);
   method SV*   text_wrap( SV * text, int width, int options = twDefault, int tabIndent = 8);

   # font_abc caches
   PFontABC font_abc_ascii;
   PList    font_abc_unicode;

}
