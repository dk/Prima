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
 *
 * $Id$
 */
/* Created by Dmitry Karasik <dk@plab.ku.dk> */
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#include <sys/types.h>
#include <dirent.h>
// #include <sys/dir.h>
#include "os2/os2guts.h"

static int ctx_mb2WA[] =
{
   mbError       , WA_ERROR,
   mbQuestion    , WA_NOTE,
   mbInformation , WA_NOTE,
   mbWarning     , WA_WARNING,
   endCtx
};

Bool
apc_beep( int style)
{
   if ( !WinAlarm( HWND_DESKTOP, ctx_remap_def( style, ctx_mb2WA, true, WA_WARNING))) apiErrRet;
   return true;
}

Bool
apc_beep_tone( int freq, int duration)
{
   if ( DosBeep( freq, duration)) return false;
   return true;
}

Bool
apc_query_drives_map( const char *firstDrive, char *map, int len)
{
   char *m = map;
   int beg;
   ULONG curDrive, driveMap;
   int i;

   if ( !map) return false;

   beg = toupper( *firstDrive);
   if (( beg < 'A') || ( beg > 'Z') || ( firstDrive[1] != ':'))
      return false;

   beg -= 'A';

   DosQueryCurrentDisk( &curDrive, &driveMap);
   for ( i = beg; i < 26 && m - map + 3 < len; i++)
   {
      if ((driveMap << ( 31 - i)) >> 31)
      {
         *m++ = i + 'A';
         *m++ = ':';
         *m++ = ' ';
      }
   }

   *m = '\0';
   return true;
}

Bool is_cd( int num) {
   ULONG actionStatus;
   HFILE cdDriver;
   APIRET rc;
   struct {
      short n;
      short i;
   } data;
   ULONG dataLen;

   rc = DosOpen( "\\DEV\\CD-ROM2$", &cdDriver, &actionStatus, 0, FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE, 0);
   if (rc) return false;

   dataLen = 4;
   rc = DosDevIOCtl( cdDriver, 0x82, 0x60, 0, 0, 0, &data, 4, &dataLen);
   DosClose( cdDriver);
   if (rc) return false;

   return ( data. i <= num && num <= data. i + data. n - 1);
}

int
apc_query_drive_type( const char *drive)
{
#define BADRC(func) if ((func)!=NO_ERROR) return dtNone;
   char root[ 16];
   int num;
   Bool removable;

   if (( toupper( *drive) < 'A') || ( toupper( *drive) > 'Z') || ( drive[1] != ':'))
      return -1;
   root[0] = toupper( drive[0]);
   root[1] = drive[1];
   root[2] = '\\';
   root[3] = '\0';
   num = root[0] - 'A';

   // Check whether this drive exists
   {
      ULONG curDrive, driveMap;
      BADRC(DosQueryCurrentDisk( &curDrive, &driveMap));
      if (!((driveMap << ( 31 - num)) >> 31)) return dtNone;
   }

   // Check whether it is a CD-ROM
   if ( is_cd( num)) return dtCDROM;

   // Classify by types and ``removableness''
   {
      BIOSPARAMETERBLOCK bpb;
      struct
      {
         char cmd;
         char unit;
      } params = { 0, num};
      ULONG parLen = sizeof( params);
      ULONG bpbLen = sizeof( bpb);

      memset( &bpb, 0, sizeof( bpb));
      bpb. bDeviceType = 5;
      params. cmd = 0; params. unit = num;
      BADRC( DosDevIOCtl( -1, IOCTL_DISK, DSK_GETDEVICEPARAMS,
                          &params, sizeof( params), &parLen,
                          &bpb, sizeof( bpb), &bpbLen));
      removable = !(bpb. fsDeviceAttr & 1);
      switch ( bpb. bDeviceType) {
         case 0:  // various stupid floppies
         case 1:
         case 2:
         case 3:
         case 4:
         case 9:
            if (removable) return dtFloppy;
            break;
         case 5: {
            char buf[ sizeof( FSQBUFFER2) + (3*CCHMAXPATH)] = {0};
            ULONG cbBuf = sizeof( buf);
            PFSQBUFFER2 fs = (void*)buf;
            char *fsname;
            root[2] = '\0';
            if ( DosQueryFSAttach( root, 0, FSAIL_QUERYNAME, (void *)buf, &cbBuf) != NO_ERROR)
               return dtUnknown;
            if (fs-> iType != FSAT_REMOTEDRV)
               return dtHDD;
            fsname = fs->szName + fs->cbName + 1;
            if ( strcmp( fsname, "RAMFS") == 0)
               return dtMemory;
            return dtNetwork;
            } break;
         case 7:  // ``other'' :-)
            if (removable && ( bpb. usBytesPerSector * bpb. cSectors == 512*1440 ||
                               bpb. usBytesPerSector * bpb. cSectors == 512*2880 ||
                               bpb. usBytesPerSector * bpb. cSectors == 512*5760))
               return dtFloppy;
            break;
      }
   }

   return dtUnknown;
#undef BADRC
}


static const char * NETBIOS_username( void)
{
    char *username;
    char errmod[256];
    static unsigned short ( *Net32WkstaGetInfo) ( const unsigned char * pszServer,
                                           unsigned long         sLevel,
                                           unsigned char       * pbBuffer,
                                           unsigned long         cbBuffer,
                                           unsigned long       * pcbTotalAvail);
    APIRET rc;
    ULONG avail;
    HMODULE hNETAPI;
    static Bool badModule;
    static char *buf = nil;

    if (!Net32WkstaGetInfo && !badModule) {
        rc=DosLoadModule( errmod, 256, "NETAPI32", &hNETAPI);
        if (rc!=0) {
            badModule = true;
            return NULL;
        }
        rc=DosQueryProcAddr( hNETAPI, 0, "Net32WkstaGetInfo", ( PFN*) &Net32WkstaGetInfo);
        if (rc!=0) {
            badModule = true;
            DosFreeModule(hNETAPI);
            return NULL;
        }
    }
    free( buf); buf = nil;
    rc = Net32WkstaGetInfo(NULL, 1, NULL, 0, &avail);
    if ( !( buf = ( char*) malloc( avail))) return NULL;
    rc = Net32WkstaGetInfo( NULL, 1, buf, avail, &avail);
    username = *( char**) ( buf+14);
    return ( rc==0 && username[ 0] ? username : NULL);
}

static const char * UNIXstyle_username( void)
{
    const char *username = getenv( "USER");
    if ( !username) {
        username = getenv( "LOGNAME");
    }
    return username;
}

char *
apc_get_user_name( void)
{
    const char *username=NETBIOS_username();
    if (!username) {
        username=UNIXstyle_username();
    }
    return ( username ? (char *)username : "");
}


PList
apc_getdir( const char *dirname)
{
   DIR * dh;
   struct dirent *de;
   PList dirlist = nil;
   char dirn[ 4];

   if ( dirname == nil) return nil;
   if (( strlen( dirname) == 2) && ( dirname[1] == ':')) {
      strcpy( dirn, dirname);
      strcat( dirn, "\\");
      dirname = dirn;
   }

   if (( dh = opendir( dirname)) && (dirlist = plist_create( 50, 50))) {
      while (( de = readdir( dh)))
      {
         list_add( dirlist, (Handle) duplicate_string( de-> d_name));
         list_add( dirlist, (Handle) duplicate_string(( de-> d_attr & A_DIR) ? "dir" : "reg"));
      }
      closedir( dh);
   }
   return dirlist;
}

Bool
apc_dl_export(char *path)
{
   APIRET rc = 0;
   PSZ oldLibPath = NULL, newLibPath;
   ULONG bSize = 4096;
   int done = 0;

   do {
      char *p1, *p2;

      rc = DosAllocMem( ( PPVOID)&oldLibPath, bSize, PAG_COMMIT | PAG_READ | PAG_WRITE);
      if ( rc != NO_ERROR) {
          break;
      }
      rc = DosQueryExtLIBPATH( oldLibPath, BEGIN_LIBPATH);
      if ( rc != NO_ERROR) {
         DosFreeMem( oldLibPath);
         if ( rc == ERROR_INSUFFICIENT_BUFFER) {
            bSize += 4096;
            continue;
         }
         break;
      }
      newLibPath = ( PSZ)malloc( strlen( oldLibPath) + strlen( path) + 1);
      if ( newLibPath == NULL) {
         DosFreeMem( oldLibPath);
         rc = ERROR_NOT_ENOUGH_MEMORY;
         break;
      }
      strcpy( newLibPath, path);
      p1 = strrchr( newLibPath, '/');
      p2 = strrchr( newLibPath, '\\');
      if ( p1 == NULL) {
         p1 = p2;
      }
      else if ( p1 < p2) {
         p1 = p2;
      }
      if ( p1 != NULL) {
         *p1++ = ';';
         *p1 = 0;
         strcat( newLibPath, oldLibPath);
         rc = DosSetExtLIBPATH( newLibPath, BEGIN_LIBPATH);
      }
      free( newLibPath);
      DosFreeMem( oldLibPath);
      done = 1;
   } while ( ! done);
   return rc == NO_ERROR;
}

Bool
apc_sys_get_insert_mode( void)
{
   return WinQuerySysValue( HWND_DESKTOP, SV_INSERTMODE);
}

Bool
apc_sys_set_insert_mode( Bool insMode)
{
   WinSetSysValue( HWND_DESKTOP, SV_INSERTMODE, insMode);
   return true;
}

int
apc_sys_get_value( int sysValue)
{
   switch ( sysValue) {
   case svYMenu          :
       return WinQuerySysValue( HWND_DESKTOP, SV_CYMENU);
   case svYTitleBar      :
       return WinQuerySysValue( HWND_DESKTOP, SV_CYTITLEBAR);
   case svMousePresent   :
       return WinQuerySysValue( HWND_DESKTOP, SV_MOUSEPRESENT);
   case svMouseButtons   :
       return WinQuerySysValue( HWND_DESKTOP, SV_CMOUSEBUTTONS);
   case svSubmenuDelay   :
       return 100;
   case svFullDrag       :
       return WinQuerySysValue( HWND_DESKTOP, SV_DYNAMICDRAG);
   case svDblClickDelay   :
       return WinQuerySysValue( HWND_DESKTOP, SV_DBLCLKTIME);
   case svWheelPresent    :
       return 0; /* XXX */
   case svXIcon           :
       return WinQuerySysValue( HWND_DESKTOP, SV_CXICON);
   case svYIcon           :
       return WinQuerySysValue( HWND_DESKTOP, SV_CYICON);
   case svXSmallIcon      :
       return WinQuerySysValue( HWND_DESKTOP, SV_CXMINMAXBUTTON);
   case svYSmallIcon      :
       return WinQuerySysValue( HWND_DESKTOP, SV_CYMINMAXBUTTON);
   case svXPointer        :
       return WinQuerySysValue( HWND_DESKTOP, SV_CXPOINTER);
   case svYPointer        :
       return WinQuerySysValue( HWND_DESKTOP, SV_CYPOINTER);
   case svXScrollbar      :
       return WinQuerySysValue( HWND_DESKTOP, SV_CXVSCROLL);
   case svYScrollbar      :
       return WinQuerySysValue( HWND_DESKTOP, SV_CYHSCROLL);
   case svXCursor         :
       return WinQuerySysValue( HWND_DESKTOP, SV_CXBORDER);
   case svAutoScrollFirst :
      return WinQuerySysValue( HWND_DESKTOP, SV_FIRSTSCROLLRATE);
   case svAutoScrollNext  :
      return WinQuerySysValue( HWND_DESKTOP, SV_SCROLLRATE);
   case svInsertMode      :
      return WinQuerySysValue( HWND_DESKTOP, SV_INSERTMODE);
   case svXbsNone         :
      return 0;
   case svYbsNone         :
      return 0;
   case svXbsSizeable     :
      return WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER);
   case svYbsSizeable     :
      return WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER);
   case svXbsSingle       :
      return WinQuerySysValue( HWND_DESKTOP, SV_CXBORDER);
   case svYbsSingle       :
      return WinQuerySysValue( HWND_DESKTOP, SV_CYBORDER);
   case svXbsDialog       :
      return WinQuerySysValue( HWND_DESKTOP, SV_CXDLGFRAME);
   case svYbsDialog       :
      return WinQuerySysValue( HWND_DESKTOP, SV_CYDLGFRAME);
   case svShapeExtension  :
      return 0;
   case svColorPointer    :
      {
         int i, max = 1;
         for ( i = 0; i < guts. bmfCount * 2; i+=2) {
            if ( max < guts. bmf[i] * guts. bmf[i+1])
               max = guts. bmf[i] * guts. bmf[i+1];
         }
         return max > 1;
      }
      break;
   case svCanUTF8_Input :
      return 0;
   case svCanUTF8_Output :
      return 0;
   default:
      return -1;
   }
   return 0;
}

char *
apc_system_action( const char* params)
{
   char *ret = nil;

   if ( stricmp( params, "wait.before.quit") == 0) { waitBeforeQuit = 1; }
   else if ( stricmp( params, "sigsegv") == 0)
   {
      char *t = 0;
      int   i = t[100];
      (void)i;
   }
   else if ( strnicmp( params, "echo", 4) == 0) {
      printf( "%s", (char*) params);
   }
   return ret;
}


PHash
apc_widget_user_profile( char * name, Handle owner)
{
   return nil;
}

Bool
hwnd_check_limits( int x, int y, Bool uint)
{
   if ( x > 16383 || y > 16383) return false;
   if ( uint && ( x < -16383 || y < -16383)) return false;
   return true;
}


Bool
apc_fetch_resource( const char *className, const char *name,
                    const char *resClass, const char *resName,
                    Handle owner, int resType,
                    void *val)
{
   return false;
}

char *
apc_last_error( void )
{
   return NULL;
}
