/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
/* apc.c --- apc/ api for os/2 */
#include <limits.h>
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#include "os2/os2guts.h"
#include "Image.h"
#include "Printer.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)


static void ppi_create( PRQINFO3 * dest, PRQINFO3 * source)
{
#define SZCPY(field) dest-> field = duplicate_string( source-> field)
   memcpy( dest, source, sizeof( PRQINFO3));
   SZCPY( pszName);
   SZCPY( pszSepFile);
   SZCPY( pszPrProc);
   SZCPY( pszParms);
   SZCPY( pszComment);
   SZCPY( pszPrinters);
   SZCPY( pszDriverName);
   if ( source-> pDriverData)
   {
      if (( dest-> pDriverData = malloc( source-> pDriverData-> cb)))
        memcpy( dest-> pDriverData, source-> pDriverData, source-> pDriverData-> cb);
   }
}


static void ppi_destroy( PRQINFO3 * ppi)
{
   if ( !ppi) return;
   free( ppi-> pszName);
   free( ppi-> pszSepFile);
   free( ppi-> pszPrProc);
   free( ppi-> pszParms);
   free( ppi-> pszComment);
   free( ppi-> pszPrinters);
   free( ppi-> pszDriverName);
   free( ppi-> pDriverData);
   memset( ppi, 0, sizeof( PRQINFO3));
}


static int
prn_query( char * printer, PRQINFO3 * info)
{
   ULONG returned, total, needed;
   SPLERR sprc = SplEnumQueue( nil, 3, nil, 0, &returned, &total, &needed, nil);
   PRQINFO3 *ppi, *useThis = nil;
   int i;
   Bool useDefault = ( printer == nil || strlen( printer) == 0);

   if (( sprc != ERROR_MORE_DATA) && ( sprc != NO_ERROR)) {
      apiAltErr( sprc);
      return -1;
   }
   if ( total == 0) return 0;
   ppi  = malloc( needed);
   if ( !ppi && needed > 0) return -1;
   sprc = SplEnumQueue( nil, 3, ppi, needed, &returned, &total, &needed, nil);
   if ( sprc != 0) {
      apiAltErr( sprc);
      free( ppi);
      return -1;
   }
   for ( i = 0; i < returned; i++)
   {
      if ( useDefault && ( ppi[ i]. fsType & PRQ3_TYPE_APPDEFAULT))
      {
         useThis = &ppi[ i];
         break;
      }
      if ( !useDefault && ( strcmp( printer, ppi[ i]. pszComment) == 0))
      {
         useThis = &ppi[ i];
         break;
      }
   }
   if ( useDefault && useThis == nil) useThis = ppi;
   if ( useThis) ppi_create( info, useThis);
   if ( !useThis) apcErr( errInvPrinter);
   free( ppi);
   return useThis ? 1 : -2;
}

PrinterInfo*
apc_prn_enumerate( Handle self, int * count)
{
   ULONG returned, total, needed;
   SPLERR sprc = SplEnumQueue( nil, 3, nil, 0, &returned, &total, &needed, nil);
   PRQINFO3 * ppi;
   PPrinterInfo list;
   int i;

   *count = 0;

   if (( sprc != ERROR_MORE_DATA) && ( sprc != NO_ERROR)) {
      apiAltErr( sprc);
      return nil;
   }
   if ( total == 0) {
      apcErr( errNoPrinters);
      return nil;
   }
   ppi = malloc( needed);
   if ( !ppi && needed > 0) return nil;
   sprc = SplEnumQueue( nil, 3, ppi, needed, &returned, &total, &needed, nil);
   if ( sprc != 0) {
      apiAltErr( sprc);
      free( ppi);
      return nil;
   }

   if ( !( list = malloc( returned * sizeof( PrinterInfo)))) {
      free( ppi);
      return nil;
   }
   for ( i = 0; i < returned; i++)
   {
      strncpy( list[ i]. name, ppi[ i]. pszComment, 255);       list[ i]. name[ 255]   = 0;
      strncpy( list[ i]. device, ppi[ i]. pszDriverName, 255);  list[ i]. device[ 255] = 0;
      list[ i]. defaultPrinter = ppi[ i]. fsType & PRQ3_TYPE_APPDEFAULT;
   }
   *count = returned;
   free( ppi);
   return list;
}

static HDC prn_info_dc( Handle self)
{
   char * c;
   DEVOPENSTRUC dev;
   char buf[ 1024];
   HDC ret;

   memset( &dev, 0, sizeof( dev));
   dev. pszLogAddress = sys s. prn. ppi. pszName;
   strcpy( buf, sys s. prn. ppi. pszDriverName);
   c = strchr( buf, '.');
   if ( c) *c = '\0';
   dev. pszDriverName = buf;
   dev. pdriv         = sys s. prn. ppi. pDriverData;
   dev. pszDataType   = nil;
   dev. pszComment    = sys s. prn. ppi. pszComment;
   if ( strlen( sys s. prn. ppi. pszPrProc)) dev. pszQueueProcName   = sys s. prn. ppi. pszPrProc;
   if ( strlen( sys s. prn. ppi. pszParms))  dev. pszQueueProcParams = sys s. prn. ppi. pszParms;
   dev .pszSpoolerParams = nil;
   dev .pszNetworkParams = nil;
   ret = DevOpenDC( guts. anchor, OD_INFO, "*", 3, (PDEVOPENDATA)&dev, nilHandle);
   if ( ret == nilHandle) apiErr;
   return ret;
}

Bool
apc_prn_create( Handle self)
{
   return true;
}

Bool
apc_prn_select( Handle self, const char* printer)
{
   int rcx;
   PRQINFO3 ppi;
   HDC dc;
   rcx = prn_query(( char*) printer, &ppi);
   if ( rcx > 0)
   {
      ppi_destroy( &sys s. prn. ppi);
      memcpy( &sys s. prn. ppi, &ppi, sizeof( ppi));
   } else
      return false;

   if ( !( dc = prn_info_dc( self))) return false;
   if ( !DevQueryCaps( dc, CAPS_HORIZONTAL_FONT_RES, 1, ( PLONG) &sys res. x)) apiErrRet;
   if ( !DevQueryCaps( dc, CAPS_VERTICAL_FONT_RES, 1, ( PLONG) &sys res. y)) apiErrRet;
   if ( !DevCloseDC( dc)) apiErr;
   return true;
}

char*
apc_prn_get_selected( Handle self)
{
   return sys s. prn. ppi. pszComment;
}

char *
apc_prn_get_default( Handle self)
{
   PRQINFO3 p;
   int rc = prn_query( nil, &p);
   if ( rc <= 0) return "";
   free( sys s. prn. defaultPrn);
   if ( !( sys s. prn. defaultPrn = malloc( strlen(( char*) p. pszComment) + 1))) {
      ppi_destroy( &p);
      return "";
   }
   strcpy( sys s. prn. defaultPrn, ( char*) p. pszComment);
   ppi_destroy( &p);
   return sys s. prn. defaultPrn;
}

Point
apc_prn_get_size( Handle self)
{
   HDC dc;
   PHCINFO hc;
   LONG i, forms;
   Point toRet = {0,0};

   if ( is_opt( optInDraw)) return ( Point){ sys s. prn. size. cx, sys s. prn. size. cy};
   if ( !( dc = prn_info_dc( self))) return toRet;
   forms = DevQueryHardcopyCaps( dc, 0, 0, nil);
   if ( forms == DQHC_ERROR) {
      apiErr;
      return toRet;
   }
   if ( !( hc = malloc( forms * sizeof( HCINFO)))) return toRet;
   if ( DevQueryHardcopyCaps( dc, 0, forms, hc) == DQHC_ERROR) {
      apiErr;
      return toRet;
   }
   if ( !DevCloseDC( dc)) apiErr;
   for ( i = 0; i < forms; i++)
   {
      if ( hc[ i]. flAttributes & HCAPS_CURRENT)
      {
         toRet = ( Point){ hc[i]. xPels, hc[i]. yPels};
         break;
      }
   }
   if ( toRet.x == 0 && toRet. y == 0 && forms != 0)
      toRet = ( Point) { hc[0]. xPels, hc[0]. yPels};
   free( hc);
   return toRet;
}


Bool
apc_prn_destroy( Handle self)
{
   ppi_destroy( &sys s. prn. ppi);
   if ( sys ps) if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( sys dc) if ( !DevCloseDC( sys dc)) apiErr;
   free( sys s. prn. defaultPrn);
   sys dc = sys ps = nilHandle;
   return true;
}

Bool
apc_prn_begin_doc( Handle self, const char* docName)
{
   LONG out = 0;
   char * c;
   DEVOPENSTRUC dev;
   char buf[ 1024];
   char bufcm[ 1024];
   LONG res;

   memset( &dev, 0, sizeof( dev));
   dev. pszLogAddress = sys s. prn. ppi. pszName;
   strcpy( buf, sys s. prn. ppi. pszDriverName);
   c = strchr( buf, '.');
   if ( c) *c = '\0';
   dev. pszDriverName = buf;
   dev. pdriv         = sys s. prn. ppi. pDriverData;
   dev. pszDataType   = "PM_Q_STD";
   strcpy( bufcm, application ? (( PComponent) application)-> name : "APC");
   dev. pszComment    = bufcm;

   if ( strlen( sys s. prn. ppi. pszPrProc)) dev. pszQueueProcName   = sys s. prn. ppi. pszPrProc;
   if ( strlen( sys s. prn. ppi. pszParms))  dev. pszQueueProcParams = sys s. prn. ppi. pszParms;
   dev .pszSpoolerParams = nil;
   dev .pszNetworkParams = nil;
   sys dc = DevOpenDC( guts. anchor, OD_QUEUED, "*", 9, (PDEVOPENDATA)&dev, nilHandle);
   if ( !sys dc) apiErrRet;
   DevQueryCaps( sys dc, CAPS_WIDTH,  1, &sys s. prn. size. cx );
   DevQueryCaps( sys dc, CAPS_HEIGHT, 1, &sys s. prn. size. cy );
   sys ps = GpiCreatePS( guts. anchor, sys dc, &sys s. prn. size,
      PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
   if ( !sys ps)
   {
      apiErr;
      DevCloseDC( sys dc);
      sys dc = nilHandle;
      return false;
   }
   res = DevEscape( sys dc, DEVESC_STARTDOC, strlen( docName), (char*)docName, &out, nil);
   if ( res != DEV_OK)
   {
      apiAltErr( res);
      GpiDestroyPS( sys ps);
      DevCloseDC( sys dc);
      sys dc = sys ps = nilHandle;
      return false;
   }
   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;
   hwnd_enter_paint( self);
   return true;
}


Bool
apc_prn_begin_paint_info( Handle self)
{
   char * c;
   DEVOPENSTRUC dev;
   char buf[ 1024];
   char bufcm[ 1024];

   memset( &dev, 0, sizeof( dev));
   dev. pszLogAddress = sys s. prn. ppi. pszName;
   strcpy( buf, sys s. prn. ppi. pszDriverName);
   c = strchr( buf, '.');
   if ( c) *c = '\0';
   dev. pszDriverName = buf;
   dev. pdriv         = sys s. prn. ppi. pDriverData;
   dev. pszDataType   = "PM_Q_STD";
   strcpy( bufcm, application ? (( PComponent) application)-> name : "APC");
   dev. pszComment    = bufcm;

   if ( strlen( sys s. prn. ppi. pszPrProc)) dev. pszQueueProcName   = sys s. prn. ppi. pszPrProc;
   if ( strlen( sys s. prn. ppi. pszParms))  dev. pszQueueProcParams = sys s. prn. ppi. pszParms;
   dev .pszSpoolerParams = nil;
   dev .pszNetworkParams = nil;
   sys dc = DevOpenDC( guts. anchor, OD_INFO, "*", 9, (PDEVOPENDATA)&dev, nilHandle);
   if ( !sys dc) apiErrRet;
   DevQueryCaps( sys dc, CAPS_WIDTH,  1, &sys s. prn. size. cx );
   DevQueryCaps( sys dc, CAPS_HEIGHT, 1, &sys s. prn. size. cy );
   sys ps = GpiCreatePS( guts. anchor, sys dc, &sys s. prn. size,
      PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
   if ( !sys ps) {
      apiErr;
      DevCloseDC( sys dc);
      sys dc = nilHandle;
      return false;
   }
   if ( !GpiCreateLogColorTable( sys ps, 0, LCOLF_RGB, 0, 0, nil)) apiErr;
   hwnd_enter_paint( self);
   return true;
}

Bool
apc_prn_end_doc( Handle self)
{
   LONG out = 0;
   LONG res;
   hwnd_leave_paint( self);
   res = DevEscape( sys dc, DEVESC_ENDDOC, 0, nil, &out, nil);
   if ( res != DEV_OK) {
      apiAltErr( res);
      DevEscape( sys dc, DEVESC_ABORTDOC, 0, nil, &out, nil);
   }
   GpiDestroyPS( sys ps);
   DevCloseDC( sys dc);
   sys dc = sys ps = nilHandle;
   return true;
}

Bool
apc_prn_end_paint_info( Handle self)
{
   hwnd_leave_paint( self);
   GpiDestroyPS( sys ps);
   DevCloseDC( sys dc);
   sys dc = sys ps = nilHandle;
   return true;
}


Bool
apc_prn_new_page( Handle self)
{
   LONG out = 0;
   LONG res = DEV_ERROR;
   if ( sys dc) res = DevEscape( sys dc, DEVESC_NEWFRAME, 0, nil, &out, nil);
   if ( res != DEV_OK) apiAltErr( res);
   return res == DEV_OK;
}

Bool
apc_prn_abort_doc( Handle self)
{
   LONG out = 0;
   LONG res;
   hwnd_leave_paint( self);
   res = DevEscape( sys dc, DEVESC_ABORTDOC, 0, nil, &out, nil);
   if ( res != DEV_OK) apiAltErr( res);
   if ( !GpiDestroyPS( sys ps)) apiErr;
   if ( !DevCloseDC( sys dc)) apiErr;
   sys dc = sys ps = nilHandle;
   return true;
}

Bool
apc_prn_setup( Handle self)
{
   char *c;
   char drv[ 1024];
   char prns[ 1024];
   LONG sprc;
   HDC  dc;

   strcpy( prns, sys s. prn. ppi. pszPrinters);
   c = strchr( prns, ',');
   if ( c) *c = '\0';
   strcpy( drv, sys s. prn. ppi. pszDriverName);
   c = strchr( drv, '.');
   if ( c) {
     *c = '\0';
     c++;
   }

   sprc = DevPostDeviceModes( guts. anchor, nil, drv,
      c, prns, DPDM_POSTJOBPROP);
   if ( sprc == DPDM_ERROR) apiErrRet;
   if ( sprc == DPDM_NONE) apcErrRet( errNoPrnSettableOptions);
   if ( sprc <= 0) apiErrRet;
   if ( sprc > sys s. prn. ppi. pDriverData-> cb) apiErrRet;
   if ( DevPostDeviceModes( guts. anchor,
      sys s. prn. ppi. pDriverData,
      drv,
      c, prns,
      DPDM_POSTJOBPROP
   ) != DEV_OK) apiErrRet;

   if ( !( dc = prn_info_dc( self))) return false;
   if ( !DevQueryCaps( dc, CAPS_HORIZONTAL_FONT_RES, 1, ( PLONG) &sys res. x)) apiErrRet;
   if ( !DevQueryCaps( dc, CAPS_VERTICAL_FONT_RES, 1, ( PLONG) &sys res. y)) apiErrRet;
   if ( !DevCloseDC( dc)) apiErr;

   return true;
}

Point
apc_prn_get_resolution( Handle self)
{
   return sys res;
}

ApiHandle
apc_prn_get_handle( Handle self)
{
   objCheck 0;
   return ( ApiHandle) sys ps;
}


Bool  
apc_prn_set_option( Handle self, char * option, char * value) 
{ 
    return false; 
}

Bool apc_prn_get_option( Handle self, char * option, char ** value) 
{ 
   *value = nil;
   return false; 
}

Bool apc_prn_enum_options( Handle self, int * count, char *** options) 
{ 
    *count = 0;
    return false; 
}

