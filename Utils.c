#include "apricot.h"
#include "Utils.h"
#include "Utils.inc"

SV *Utils_query_drives_map( char *firstDrive)
{
   char map[ 256];
   apc_query_drives_map( firstDrive, map);
   return newSVpv( map, 0);
}

int Utils_get_os()
{
   char buf[ 1024];
   return apc_application_get_os_info( buf, buf, buf, buf);
}

int Utils_get_gui()
{
   char buf[ 1024];
   return apc_application_get_gui_info( buf);
}

long Utils_ceil( double x)
{
    return ceil( x);
}

long Utils_floor( double x)
{
    return floor( x);
}
