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

/***********************************************************/
/*                                                         */
/*  System dependent printing (unix, x11)                  */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

/* printer */

Bool
apc_prn_create( Handle self)
{
   DOLBUG( "apc_prn_create()\n");
   return true;
}

Bool
apc_prn_destroy( Handle self)
{
   DOLBUG( "apc_prn_destroy()\n");
   return true;
}

PrinterInfo *
apc_prn_enumerate( Handle self, int * count)
{
   DOLBUG( "apc_prn_enumerate()\n");
   return nil;
}

Bool
apc_prn_select( Handle self, const char* printer)
{
   DOLBUG( "apc_prn_select()\n");
   return false;
}

char *
apc_prn_get_selected( Handle self)
{
   DOLBUG( "apc_prn_get_selected()\n");
   return nil;
}

Point
apc_prn_get_size( Handle self)
{
   DOLBUG( "apc_prn_get_size()\n");
   return (Point){0,0};
}

Point
apc_prn_get_resolution( Handle self)
{
   DOLBUG( "apc_prn_get_resolution()\n");
   return (Point){0,0};
}

char *
apc_prn_get_default( Handle self)
{
   DOLBUG( "apc_prn_get_default()\n");
   return nil;
}

Bool
apc_prn_setup( Handle self)
{
   DOLBUG( "apc_prn_setup()\n");
   return false;
}

Bool
apc_prn_begin_doc( Handle self, const char* docName)
{
   DOLBUG( "apc_prn_begin_doc()\n");
   return false;
}

Bool
apc_prn_begin_paint_info( Handle self)
{
   DOLBUG( "apc_prn_begin_paint_info()\n");
   return false;
}

Bool
apc_prn_end_doc( Handle self)
{
   DOLBUG( "apc_prn_end_doc()\n");
   return true;
}

Bool
apc_prn_end_paint_info( Handle self)
{
   DOLBUG( "apc_prn_end_paint_info()\n");
   return true;
}

Bool
apc_prn_new_page( Handle self)
{
   DOLBUG( "apc_prn_new_page()\n");
   return true;
}

Bool
apc_prn_abort_doc( Handle self)
{
   DOLBUG( "apc_prn_abort_doc()\n");
   return true;
}

