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

#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif


static int ctx_mb2MB[] =
{
   mbError       , MB_ICONHAND,
   mbQuestion    , MB_ICONQUESTION,
   mbInformation , MB_ICONASTERISK,
   mbWarning     , MB_ICONEXCLAMATION,
   endCtx
};


void
apc_beep( int style)
{
   MessageBeep( ctx_remap_def( style, ctx_mb2MB, true, MB_OK));
}

void
apc_beep_tone( int freq, int duration)
{
   Beep( freq, duration);
}

void
apc_query_drives_map( const char *firstDrive, char *map, int len)
{
   char *m = map;
   int beg;
   DWORD driveMap;
   int i;

   if ( !map) return;

   beg = toupper( *firstDrive);
   if (( beg < 'A') || ( beg > 'Z') || ( firstDrive[1] != ':'))
      return;

   beg -= 'A';

   if ( !( driveMap = GetLogicalDrives()))
      apiErr;
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
   return;
}

static int ctx_dt2DRIVE[] =
{
   dtUnknown  , 0               ,
   dtNone     , 1               ,
   dtFloppy   , DRIVE_REMOVABLE ,
   dtHDD      , DRIVE_FIXED     ,
   dtNetwork  , DRIVE_REMOTE    ,
   dtCDROM    , DRIVE_CDROM     ,
   dtMemory   , DRIVE_RAMDISK   ,
   endCtx
};

int
apc_query_drive_type( const char *drive)
{
   char buf[ 256];                        //  Win95 fix
   strncpy( buf, drive, 256);             //     sometimes D: isn't enough for 95,
   if ( buf[1] == ':' && buf[2] == 0) {   //     but ok for D:\.
      buf[2] = '\\';                      //
      buf[3] = 0;                         //
   }                                      //
   return ctx_remap_def( GetDriveType( buf), ctx_dt2DRIVE, false, dtNone);
}

static char userName[ 1024];

char *
apc_get_user_name()
{
   DWORD maxSize = 1024;

   if ( !GetUserName( userName, &maxSize)) apiErr;
   return userName;
}

PList
apc_getdir( const char *dirname)
{
    long		len;
    char		scanname[MAX_PATH+3];
    WIN32_FIND_DATA	FindData;
    HANDLE		fh;

    DWORD               fattrs;
    PList               ret;
    Bool                wasDot = false, wasDotDot = false;

#define add_entry(file,info)  {                                            \
    list_add( ret, ( Handle) strcpy( malloc( strlen( file) + 1), file));   \
    list_add( ret, ( Handle) strcpy( malloc( strlen( info) + 1), info));   \
}

#define add_fentry  {                                                         \
    add_entry( FindData.cFileName,                                            \
       ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DIR : FILE); \
    if ( strcmp( ".", FindData.cFileName) == 0)                               \
       wasDot = true;                                                         \
    else if ( strcmp( "..", FindData.cFileName) == 0)                         \
       wasDotDot = true;                                                      \
}


#define DIR  "dir"
#define FILE "reg"

    len = strlen(dirname);
    if (len > MAX_PATH)
	return NULL;

    /* check to see if filename is a directory */
    fattrs = GetFileAttributes( dirname);
    if ( fattrs == 0xFFFFFFFF || ( fattrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
       return NULL;

    /* Create the search pattern */
    strcpy(scanname, dirname);
    if (scanname[len-1] != '/' && scanname[len-1] != '\\')
	scanname[len++] = '/';
    scanname[len++] = '*';
    scanname[len] = '\0';

    /* do the FindFirstFile call */
    fh = FindFirstFile(scanname, &FindData);
    if (fh == INVALID_HANDLE_VALUE) {
	/* FindFirstFile() fails on empty drives! */
	if (GetLastError() != ERROR_FILE_NOT_FOUND)
	   return NULL;
        ret = plist_create( 2, 16);
        add_entry( ".",  DIR);
        add_entry( "..", DIR);
        return ret;
    }

    ret = plist_create( 16, 16);
    add_fentry;
    while ( FindNextFile(fh, &FindData))
       add_fentry;
    FindClose(fh);

    if ( !wasDot)
       add_entry( ".",  DIR);
    if ( !wasDotDot)
       add_entry( "..", DIR);

    return ret;
}

void *apc_dlopen(char *path, int mode)
{
   (void) mode;
   return LoadLibrary( path);
}

void *dlsym(void *dll, char *symbol)
{
   return GetProcAddress((HMODULE)dll, symbol);
}

static char dlerror_description[256];

char *dlerror(void)
{
   snprintf( dlerror_description, 256, "dlerror: %08x", GetLastError());
   return dlerror_description;
}

