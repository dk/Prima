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


void  apc_show_message( const char * message)
{
   MessageBox( NULL, message, "Prima", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
}

Bool
apc_sys_get_insert_mode()
{
   return guts. insertMode;
}

void
apc_sys_set_insert_mode( Bool insMode)
{
   guts. insertMode = insMode;
}

Point
get_window_borders( int borderStyle)
{
   Point ret = { 0, 0};
   switch ( borderStyle)
   {
      case bsSizeable:
         ret. x = GetSystemMetrics( SM_CXFRAME);
         ret. y = GetSystemMetrics( SM_CYFRAME);
         break;
      case bsSingle:
         ret. x = GetSystemMetrics( SM_CXBORDER);
         ret. y = GetSystemMetrics( SM_CYBORDER);
         break;
      case bsDialog:
         ret. x = GetSystemMetrics( SM_CXDLGFRAME);
         ret. y = GetSystemMetrics( SM_CYDLGFRAME);
         break;
   }
   return ret;
}

int
apc_sys_get_value( int sysValue)
{
   HKEY hKey;
   DWORD valSize = 256, valType = REG_SZ;
   char buf[ 256] = "";

   switch ( sysValue) {
   case svYMenu          :
       return guts. ncmData. iMenuHeight;
   case svYTitleBar      :
       return guts. ncmData. iCaptionHeight;
   case svMousePresent   :
       return GetSystemMetrics( SM_MOUSEPRESENT);
   case svMouseButtons   :
       return GetSystemMetrics( SM_CMOUSEBUTTONS);
   case svSubmenuDelay   :
       RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey);
       RegQueryValueEx( hKey, "MenuShowDelay", nil, &valType, buf, &valSize);
       RegCloseKey( hKey);
       return atol( buf);
   case svFullDrag       :
       RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey);
       RegQueryValueEx( hKey, "DragFullWindows", nil, &valType, buf, &valSize);
       RegCloseKey( hKey);
       return atol( buf);
   case svDblClickDelay   :
       RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Mouse", 0, KEY_READ, &hKey);
       RegQueryValueEx( hKey, "DoubleClickSpeed", nil, &valType, buf, &valSize);
       RegCloseKey( hKey);
       return atol( buf);
   case svWheelPresent    : return GetSystemMetrics( SM_MOUSEWHEELPRESENT);
   case svXIcon           : return guts. iconSizeLarge. x;
   case svYIcon           : return guts. iconSizeLarge. y;
   case svXSmallIcon      : return guts. iconSizeSmall. x;
   case svYSmallIcon      : return guts. iconSizeSmall. y;
   case svXPointer        : return guts. pointerSize. x;
   case svYPointer        : return guts. pointerSize. y;
   case svXScrollbar      : return GetSystemMetrics( SM_CXHSCROLL);
   case svYScrollbar      : return GetSystemMetrics( SM_CYVSCROLL);
   case svXCursor         : return GetSystemMetrics( SM_CXBORDER);
   case svAutoScrollFirst : return 200;
   case svAutoScrollNext  : return 50;
   case svInsertMode      : return guts. insertMode;
   case svXbsNone         : return 0;
   case svYbsNone         : return 0;
   case svXbsSizeable     : return GetSystemMetrics( SM_CXFRAME);
   case svYbsSizeable     : return GetSystemMetrics( SM_CYFRAME);
   case svXbsSingle       : return GetSystemMetrics( SM_CXBORDER);
   case svYbsSingle       : return GetSystemMetrics( SM_CYBORDER);
   case svXbsDialog       : return GetSystemMetrics( SM_CXDLGFRAME);
   case svYbsDialog       : return GetSystemMetrics( SM_CYDLGFRAME);
   default:
      apcErr( errInvParams);
   }
   return 0;
}

PFont
apc_sys_get_msg_font( PFont copyTo)
{
   *copyTo = guts. msgFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}

PFont
apc_sys_get_caption_font( PFont copyTo)
{
   *copyTo = guts. capFont;
   copyTo-> pitch = fpDefault;
   return copyTo;
}

#define rgxExists      1
#define rgxNotExists   2
#define rgxHasSubkeys  4
#define rgxHasValues   8


static Bool
prf_exists( char * path, int * info)
{
   HKEY hKey;
   long cache;

   if ( cache = ( long) hash_fetch( regnodeMan, path, strlen( path))) {
      if ( info) *info = cache;
      return cache & rgxExists;
   }

   if ( RegOpenKeyEx( HKEY_CURRENT_USER, path, 0,
                      KEY_READ, &hKey) != ERROR_SUCCESS) {
        hash_store( regnodeMan, path, strlen( path), (void*) rgxNotExists);
        return false;
   }

   cache = rgxExists;
   if ( info) {
      char buf[ MAXREGLEN];
      DWORD len = MAXREGLEN, subkeys = 0, msk, mc, values, mvn, mvd, sd;
      FILETIME ft;
      RegQueryInfoKey( hKey, buf, &len, NULL, &subkeys, &msk, &mc, &values,
         &mvn, &mvd, &sd, &ft);
      if ( subkeys > 0) cache |= rgxHasSubkeys;
      if ( values  > 0) cache |= rgxHasValues;
      *info = cache;
   }
   hash_store( regnodeMan, path, strlen( path), (void*) cache);
   RegCloseKey( hKey);
   return true;
}

static Bool
prf_find( char * path, List * names, int firstName, char * result)
{
   char buf[ MAXREGLEN];
   int info;

   _snprintf( buf, MAXREGLEN, "%s\\%s", path, names-> items[ firstName]);
   if ( prf_exists( buf, nil)) {
      if ( names-> count > firstName + 1) {
         if ( prf_find( buf, names, firstName + 1, result)) return true;
      } else {
         strcpy( result, buf);
         return true;
      }
   }

   _snprintf( buf, MAXREGLEN, "%s\\*", path);
   if ( prf_exists( buf, &info)) {
      if ( info & rgxHasSubkeys) {
         int i;
         for ( i = names-> count - 1; i > firstName; i--)
            if ( prf_find( buf, names, i, result))
               return true;
      }
      if (( info & rgxHasValues) == 0)
         return false;
      strcpy( result, buf);
      return true;
   }
   return false;
}

static char * regColors[] = {
   "color",
   "backColor",
   "hiliteColor",
   "disabledColor",
   "hiliteBackColor",
   "disabledBackColor",
   "light3DColor",
   "dark3DColor"
};

extern PHash
apc_widget_user_profile( PList names)
{
   char buf[ MAXREGLEN];
   HKEY hKey;
   Bool res;
   DWORD type, size, dw, i;
   PHash ret;

   res = prf_find( REG_STORAGE, names, 0, buf);
   if ( !res) return nil;


   if ( RegOpenKeyEx( HKEY_CURRENT_USER, buf, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
      return nil;

   ret = hash_create();

   for ( i = 0; i < sizeof( regColors) / sizeof( char*); i++) {
      Color * c;
      type = REG_DWORD;
      size = sizeof( DWORD);
      if ( RegQueryValueEx( hKey, regColors[i], NULL,
           &type, ( LPBYTE) &dw, &size) != ERROR_SUCCESS) continue;
      c = malloc( sizeof( c));
      *c = dw;
      hash_store( ret, regColors[i], strlen(regColors[i]), c);
   }
   type = REG_SZ;
   size = MAXREGLEN;
   if ( RegQueryValueEx( hKey, "Font", NULL,
        &type, ( LPBYTE) buf, &size) == ERROR_SUCCESS) {
      Font * f = malloc( sizeof( Font));
      font_pp2font( buf, f);
      hash_store( ret, "Font", strlen("Font"), f);
   }

   RegCloseKey( hKey);
   return ret;
}
