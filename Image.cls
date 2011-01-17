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
   int            eventMask1;
   int            eventMask2;

   property int    conversion;
   property SV *   data;
   property Bool   hScaling;
   property int    lineSize;
   property SV *   palette;
   property SV *   pixel( int x, int y);
   property Bool   preserveType;
   property Point  resolution;
   property Point  size;
   property double stats( int index);
   property int    type;
   property Bool   vScaling;

   # methods
   method UV     add_notification( char * name, SV * subroutine, Handle referer = nilHandle, int index = -1);
   method Handle bitmap();
   method Bool   begin_paint();
   method Bool   begin_paint_info();
   static SV   * codecs( SV * dummy);
   c_only void   create_empty( int width, int height, int type);
   method void   done();
   method Handle dup();
   method void   end_paint();
   method void   end_paint_info();
   method Handle extract( int x, int y, int width, int height);
   method int    get_bpp();
   method SV   * get_handle();
   method Color  get_nearest_color( Color color);
   c_only void   handle_event( PEvent event);
   method void   init( HV * profile);
   weird  PList  load( SV * className, char *filename, HV * profile);
   c_only void   make_empty();
   method void   map( Color color);
   import SV *   notification_types();
   import SV *   profile_default ();
   method Bool   put_image_indirect( Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop);
   method void   remove_notification( UV id);
   method void   resample( double srcLo, double srcHi, double dstLo, double dstHi);
   weird  int    save( SV *className, char *filename, HV * profile);
   method void   set( HV * profile);
   c_only void   reset( int type, RGBColor * palette, int palSize);
   c_only void   stretch( int width, int height);
   c_only void   update_change();
}
