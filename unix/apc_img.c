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
/*
 * System dependent image routines (unix, x11)
 */

#include "unix/guts.h"
#include "Image.h"

/* image & bitmaps */
Bool
apc_image_create( Handle self)
{
    DOLBUG( "apc_image_create()\n");
    return false;
}

Bool
apc_image_destroy( Handle self)
{
   DEFXX;
   if ( XX-> image_cache) {
      XDestroyImage( XX-> image_cache);
      XX-> image_cache = nil;
   }
   if ( XX-> icon_cache) {
      XDestroyImage( XX-> icon_cache);
      XX-> icon_cache = nil;
   }
   return true;
}

/* See unix/apc_graphics.c
Bool
apc_image_begin_paint( Handle self);
*/

Bool
apc_image_begin_paint_info( Handle self)
{
    DOLBUG( "apc_image_begin_paint_info()\n");
    return false;
}

/* See unix/apc_graphics.c
void
apc_image_end_paint( Handle self)
*/

Bool
apc_image_end_paint_info( Handle self)
{
   DOLBUG( "apc_image_end_paint_info()\n");
   return true;
}

Bool
apc_image_update_change( Handle self)
{
   DEFXX;
   PImage img = PImage( self);

   if ( XX-> image_cache) {
      XDestroyImage( XX-> image_cache);
      XX-> image_cache = nil;
   }
   if ( XX-> icon_cache) {
      XDestroyImage( XX-> icon_cache);
      XX-> icon_cache = nil;
   }
   XX-> size. x = img-> w;
   XX-> size. y = img-> h;
   return true;
}

Bool
apc_dbm_create( Handle self, Bool monochrome)
{
    DOLBUG( "apc_dbm_create()\n");
    return false;
}

Bool
apc_dbm_destroy( Handle self)
{
   DOLBUG( "apc_dbm_destroy()\n");
   return true;
}
