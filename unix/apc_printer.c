#include "unix/guts.h"

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  System dependent printing (unix, x11)                  */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

/* printer */

Bool
apc_prn_create( Handle self)
{
fprintf( stderr, "apc_prn_create()\n");
   return true;
}

void
apc_prn_destroy( Handle self)
{
fprintf( stderr, "apc_prn_destroy()\n");
}

PrinterInfo *
apc_prn_enumerate( Handle self, int * count)
{
fprintf( stderr, "apc_prn_enumerate()\n");
   return nil;
}

Bool
apc_prn_select( Handle self, char * printer)
{
fprintf( stderr, "apc_prn_select()\n");
   return false;
}

char *
apc_prn_get_selected( Handle self)
{
fprintf( stderr, "apc_prn_get_selected()\n");
   return nil;
}

Point
apc_prn_get_size( Handle self)
{
fprintf( stderr, "apc_prn_get_size()\n");
   return (Point){0,0};
}

Point
apc_prn_get_resolution( Handle self)
{
fprintf( stderr, "apc_prn_get_resolution()\n");
   return (Point){0,0};
}

char *
apc_prn_get_default( Handle self)
{
fprintf( stderr, "apc_prn_get_default()\n");
   return nil;
}

Bool
apc_prn_setup( Handle self)
{
fprintf( stderr, "apc_prn_setup()\n");
   return false;
}

Bool
apc_prn_begin_doc( Handle self, char *docName)
{
fprintf( stderr, "apc_prn_begin_doc()\n");
   return false;
}

Bool
apc_prn_begin_paint_info( Handle self)
{
fprintf( stderr, "apc_prn_begin_paint_info()\n");
   return false;
}

void
apc_prn_end_doc( Handle self)
{
fprintf( stderr, "apc_prn_end_doc()\n");
}

void
apc_prn_end_paint_info( Handle self)
{
fprintf( stderr, "apc_prn_end_paint_info()\n");
}

void
apc_prn_new_page( Handle self)
{
fprintf( stderr, "apc_prn_new_page()\n");
}

void
apc_prn_abort_doc( Handle self)
{
fprintf( stderr, "apc_prn_abort_doc()\n");
}

