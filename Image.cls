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
local @ImageStats double[7];

object Prima::Image( Prima::Drawable)
{
   # variables
   int            type;              # imXXXX
   int            lineSize;          # rounded to 32-bit boundary
   int            dataSize;          #  == lineSize * h
   int            conversion;        # ictXXXX
   unsigned char *data;
   ImageStats     stats;
   int            statsCache;
   Point          resolution;

   # methods
   method Handle bitmap();
   method Bool   begin_paint();
   method Bool   begin_paint_info();
   c_only void   create_empty( int width, int height, int type);
   method void   done();
   method Handle dup();
   method void   end_paint();
   method void   end_paint_info();
   method Handle extract( int x, int y, int width, int height);
   method int    get_bpp();
   method int    get_conversion();
   method SV   * get_data();
   method SV   * get_handle();
   weird  Handle get_info( SV *who, char *filename);
   static char * get_status_string( SV *self);
   method SV   * get_palette();
   method Color  get_pixel(int x, int y);
   method Bool   get_preserve_type();
   method Point  get_resolution();
   method Bool   get_h_scaling();
   method Bool   get_v_scaling();
   method double get_stats( int index);
   method int    get_type();
   method void   init( HV * profile);
   weird  Handle load( SV *who, char *filename, PList imgInfo);
   c_only void   make_empty();
   import SV *   profile_default ();
   method void   resample( double srcLo, double srcHi, double dstLo, double dstHi);
   weird  Bool   save( SV *who, char *filename, PList imgInfo);
   method void   set( HV * profile);
   method void   set_conversion( int conversion);
   method void   set_data( SV * data);
   method void   set_height( int height);
   method void   set_palette( SV * palette);
   method Bool   set_pixel(int x, int y, Color color);
   method void   set_preserve_type( Bool preserveType);
   method void   set_h_scaling( Bool scaling);
   method void   set_v_scaling( Bool scaling);
   method void   set_resolution( int x, int y);
   method void   set_size( int width, int height);
   method void   set_stats( double value, int index);
   import void   set_type( int type);
   method void   set_width( int width);
   c_only void   reset( int type, SV * palette);
   c_only void   stretch( int width, int height);
   c_only void   update_change();
}
