#include "win32\win32guts.h"
#include <stdio.h>
#include <stdarg.h>
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Component.h"

#ifdef __cplusplus
extern "C" {
#endif


static Handle ctx_mb2MB[] =
{
	mbError       , MB_ICONHAND,
	mbQuestion    , MB_ICONQUESTION,
	mbInformation , MB_ICONASTERISK,
	mbWarning     , MB_ICONEXCLAMATION,
	endCtx
};


Bool
apc_beep( int style)
{
	if ( !MessageBeep( ctx_remap_def( style, ctx_mb2MB, true, MB_OK))) apiErrRet;
	return true;
}

Bool
apc_beep_tone( int freq, int duration)
{
	if ( !Beep( freq, duration)) apiErrRet;
	return true;
}

Bool
apc_query_drives_map( const char *firstDrive, char *map, int len)
{
	char *m = map;
	int beg;
	DWORD driveMap;
	int i;

#ifdef __CYGWIN__
	if ( !map || len <= 0) return true;
	*map = 0;
	return true;
#endif
	if ( !map) return false;

	beg = toupper( *firstDrive);
	if (( beg < 'A') || ( beg > 'Z') || ( firstDrive[1] != ':'))
		return false;

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
	return true;
}

static Handle ctx_dt2DRIVE[] =
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
#ifdef __CYGWIN__
	return false;
#endif
	strncpy( buf, drive, 255);             //     sometimes D: isn't enough for 95,
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
apc_getdir( const char *dirname, Bool is_utf8)
{
#ifdef __CYGWIN__
	DIR *dh;
	struct dirent *de;
	PList dirlist = nil;
	char *type, *dname;
	char path[ 2048];
	struct stat s;

	if ( *dirname == '/' && dirname[1] == '/') dirname++;
	if ( strcmp( dirname, "/") == 0)
		dname = "";
	else
		dname = ( char*) dirname;


	if (( dh = opendir( dirname)) && (dirlist = plist_create( 50, 50))) {
		while (( de = readdir( dh))) {
			list_add( dirlist, (Handle)duplicate_string( de-> d_name));
			snprintf( path, 2047, "%s/%s", dname, de-> d_name);
			type = nil;
			if ( stat( path, &s) == 0) {
				switch ( s. st_mode & S_IFMT) {
				case S_IFIFO:        type = "fifo";  break;
				case S_IFCHR:        type = "chr";   break;
				case S_IFDIR:        type = "dir";   break;
				case S_IFBLK:        type = "blk";   break;
				case S_IFREG:        type = "reg";   break;
				case S_IFLNK:        type = "lnk";   break;
				case S_IFSOCK:       type = "sock";  break;
				}
			}
			if ( !type) type = "reg";
			list_add( dirlist, (Handle)duplicate_string( type));
		}
		closedir( dh);
	}
	return dirlist;
#else
	long		 len;
	WCHAR		 scanname[(MAX_PATH+3)*2];
	WIN32_FIND_DATAW FindData;
	HANDLE		 fh;
	WCHAR *          dirname_w;

	DWORD            fattrs;
	PList            ret;
	Bool             wasDot = false, wasDotDot = false;

#define add_entry(file,info)  {                         \
	list_add( ret, ( Handle) duplicate_string(file));   \
	list_add( ret, ( Handle) duplicate_string(info));   \
}

#define add_fentry  {                                                         \
	WideCharToMultiByte(CP_UTF8, 0, \
		FindData.cFileName, -1, \
		(LPSTR)scanname, sizeof(scanname), \
		NULL, false); \
	add_entry((char*) scanname, \
		( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DIR : FILE); \
	if ( wcscmp( L".", FindData.cFileName) == 0)                               \
		wasDot = true;                                                         \
	else if ( wcscmp( L"..", FindData.cFileName) == 0)                         \
		wasDotDot = true;                                                      \
}


#define DIR  "dir"
#define FILE "reg"

	dirname_w = is_utf8 ?
		alloc_utf8_to_wchar( dirname, -1, NULL) :
		alloc_ascii_to_wchar( dirname, -1);

	len = wcslen(dirname_w);
	if (len > MAX_PATH) {
		free(dirname_w);
		return NULL;
	}
		
	/* check to see if filename is a directory */
	fattrs = GetFileAttributesW( dirname_w);
	if ( fattrs == 0xFFFFFFFF || ( fattrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		free(dirname_w);
		return NULL;
	}

	/* Create the search pattern */
	wcscpy(scanname, dirname_w);
	if (scanname[len-1] != '/' && scanname[len-1] != '\\')
		scanname[len++] = '/';
	scanname[len++] = '*';
	scanname[len] = '\0';
	free(dirname_w);

	/* do the FindFirstFile call */
	fh = FindFirstFileW(scanname, &FindData);
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
	while ( FindNextFileW(fh, &FindData))
		add_fentry;
	FindClose(fh);

	if ( !wasDot)
		add_entry( ".",  DIR);
	if ( !wasDotDot)
		add_entry( "..", DIR);

#undef FILE
#undef DIR
	return ret;
#endif
}

Bool
apc_show_message( const char * message, Bool utf8)
{
	Bool ret;
	if ( utf8 && (message = ( char*)alloc_utf8_to_wchar( message, -1, NULL))) {
		ret = MessageBoxW( NULL, ( WCHAR*) message, L"Prima", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND) != 0;
		free(( void*) message);
	} else
		ret = MessageBox( NULL, message, "Prima", MB_OK | MB_TASKMODAL | MB_SETFOREGROUND) != 0;
	return ret;
}

Bool
apc_sys_get_insert_mode()
{
	return guts. insertMode;
}

Bool
apc_sys_set_insert_mode( Bool insMode)
{
	guts. insertMode = insMode;
	return true;
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
		RegQueryValueEx( hKey, "MenuShowDelay", nil, &valType, ( LPBYTE) buf, &valSize);
		RegCloseKey( hKey);
		return atol( buf);
	case svFullDrag       :
		RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey);
		RegQueryValueEx( hKey, "DragFullWindows", nil, &valType, ( LPBYTE)buf, &valSize);
		RegCloseKey( hKey);
		return atol( buf);
	case svDblClickDelay   :
		RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Mouse", 0, KEY_READ, &hKey);
		RegQueryValueEx( hKey, "DoubleClickSpeed", nil, &valType, ( LPBYTE)buf, &valSize);
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
	case svShapeExtension  : return 1;
	case svColorPointer    : return guts. displayBMInfo. bmiHeader. biBitCount > 4;
	case svCanUTF8_Input   : return 1;
	case svCanUTF8_Output  : return 1;
	case svCompositeDisplay: return is_dwm_enabled();
	case svLayeredWidgets: return guts. displayBMInfo. bmiHeader. biBitCount > 8;
	case svFixedPointerSize: return 0;
	case svMenuCheckSize   : return GetSystemMetrics( SM_CXMENUCHECK );
	case svFriBidi         : return use_fribidi;
	default:
		return -1;
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

Bool
hwnd_check_limits( int x, int y, Bool uint)
{
	if ( x > 16383 || y > 16383) return false;
	if ( uint && ( x < -16383 || y < -16383)) return false;
	return true;
}

#define rgxExists      1
#define rgxNotExists   2
#define rgxHasSubkeys  4
#define rgxHasValues   8
#define rgxInUser      16
#define rgxInSys       32

static Bool
prf_exists( HKEY hk, char * path, int * info)
{
	HKEY hKey;
	Handle cache;
	if (( cache = ( Handle) hash_fetch( regnodeMan, path, strlen( path)))) {
		if ( info) *info = cache;
		return cache & rgxExists;
	}

	if ( RegOpenKeyEx( hk, path, 0,
							KEY_READ, &hKey) != ERROR_SUCCESS) {
		hash_store( regnodeMan, path, strlen( path), (void*) rgxNotExists);
		return false;
	}

	cache |= rgxExists;
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
prf_find( HKEY hk, char * path, List * ids, int firstName, char * result)
{
	char buf[ MAXREGLEN];
	int j = 2, info;

	while ( j--) {
		snprintf( buf, MAXREGLEN, "%s\\%s", path, ( char*) ids[j].items[ firstName]);
		if ( prf_exists( hk, buf, nil)) {
			if ( ids[j].count > firstName + 1) {
				if ( prf_find( hk, buf, ids, firstName + 1, result))
					return true;
			} else {
				strcpy( result, buf);
				return true;
			}
		}
	}

	j = 2;
	while ( j--) {
		snprintf( buf, MAXREGLEN, "%s\\*", path);
		if ( prf_exists( hk, buf, &info)) {
			if ( info & rgxHasSubkeys) {
				int i;
				for ( i = ids[j].count - 1; i >= firstName; i--) {
					if ( prf_find( hk, buf, ids, i, result))
						return true;
				}
			}
			if (( info & rgxHasValues) == 0)
				return false;
			strcpy( result, buf);
			return true;
		}
	}
	return false;
}


Bool
apc_fetch_resource( const char *className, const char *name,
						const char *resClass, const char *resName,
						Handle owner, int resType,
						void *val)
{
	Bool res = true;
	HKEY hKey;
	char buf[ MAXREGLEN];
	DWORD type, size, i;
	List ids[ 2];

	i = 2; while( i--) list_create(&ids[i], 8, 8);

	list_add(&ids[1], ( Handle) duplicate_string( name));
	list_add(&ids[0], ( Handle) duplicate_string( className));

	while ( owner) {
		list_insert_at(&ids[1],   ( Handle) prima_normalize_resource_string(
			duplicate_string( PComponent( owner)-> name), false), 0);
		list_insert_at(&ids[0], ( Handle) prima_normalize_resource_string(
			duplicate_string(
				( owner == application) ? "Prima" : CComponent( owner)-> className
			), true), 0);
		owner = PComponent( owner)-> owner;
	}

	if (!( res = prf_find( HKEY_CURRENT_USER, REG_STORAGE, ( List *) &ids, 0, buf)))
		goto FINALIZE;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER, buf, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		res = false;
		goto FINALIZE;
	}

	switch ( resType) {
	case frString:
		type = REG_SZ;
		size = MAXREGLEN;
		if ( RegQueryValueEx( hKey, resName, NULL,
			&type, ( LPBYTE) buf, &size) == ERROR_SUCCESS) {
			char **x = ( char **) val;
			*x = duplicate_string( buf);
		} else
		if ( RegQueryValueEx( hKey, resClass, NULL,
			&type, ( LPBYTE) buf, &size) == ERROR_SUCCESS) {
			char **x = ( char **) val;
			*x = duplicate_string( buf);
		} else
			res = false;
		break;
	case frFont:
		type = REG_SZ;
		size = MAXREGLEN;
		if ( RegQueryValueEx( hKey, resName, NULL,
			&type, ( LPBYTE) buf, &size) == ERROR_SUCCESS)
			font_pp2font( buf, ( Font *) val);
		else
		if ( RegQueryValueEx( hKey, resClass, NULL,
			&type, ( LPBYTE) buf, &size) == ERROR_SUCCESS)
			font_pp2font( buf, ( Font *) val);
		else
			res = false;
		break;
	case frColor:
		type = REG_DWORD;
		size = sizeof( DWORD);
		if ( RegQueryValueEx( hKey, resName, NULL,
			&type, ( LPBYTE) val, &size) != ERROR_SUCCESS)
			res = ( RegQueryValueEx( hKey, resClass, NULL,
			&type, ( LPBYTE) val, &size) == ERROR_SUCCESS);
		else
			res = false;
	}

	RegCloseKey( hKey);

FINALIZE:

	i = 2;
	while( i--) {
		list_delete_all( &ids[i], true);
		list_destroy( &ids[i]);
	}
	return res;
}

Bool
apc_dl_export(char *path)
{
	return LoadLibrary( path) != NULL;
}

WCHAR *
alloc_utf8_to_wchar( const char * utf8, int length, int * mb_len)
{
	WCHAR * ret;
	int size;
	char * u2 = (char*) utf8;
	if ( length > 0) {
		while ( length-- > 0 ) u2 = ( char*) utf8_hop(( U8*) u2, 1);
		length = u2 - utf8;
	}
	size = MultiByteToWideChar(CP_UTF8, 0, utf8, length, NULL, 0);
	if ( size < 0) {
		if ( mb_len ) *mb_len = 0;
		return nil;
	}
	if ( !( ret = malloc( size * sizeof( WCHAR)))) return nil;
	MultiByteToWideChar(CP_UTF8, 0, utf8, length, ret, size);
	if ( mb_len ) *mb_len = size;
	return ret;
}

WCHAR *
alloc_utf8_to_wchar_visual( const char * utf8, int length, int * mb_len)
{
	WCHAR * ret;
	int i, size;
	char * u2 = (char*) utf8;
	if ( length > 0) {
		while ( length-- > 0 ) u2 = ( char*) utf8_hop(( U8*) u2, 1);
		length = u2 - utf8;
	}
	size = MultiByteToWideChar(CP_UTF8, 0, utf8, length, NULL, 0);
	if ( size < 0) {
		if ( mb_len ) *mb_len = 0;
		return nil;
	}
	if ( !( ret = malloc((size + 1) * sizeof( WCHAR)))) return nil;
/*
U+202A (LRE)	LEFT-TO-RIGHT EMBEDDING	Treats the following text as embedded left-to-right.
U+202B (RLE)	RIGHT-TO-LEFT EMBEDDING	Treats the following text as embedded right to left.
U+202D (LRO)	LEFT-TO-RIGHT OVERRIDE	Forces the following characters to be treated as strong left-to-right characters.
U+202E (RLO)	RIGHT-TO-LEFT OVERRIDE	Forces the following characters to be treated as strong right-to-left characters.
U+202C (PDF)	POP DIRECTIONAL FORMATTING CODE	Restores the bidirectional state to what it was before the last LRE, RLE, RLO, or LRO.
U+200E (LRM)	LEFT-TO-RIGHT MARK	Left-to-right strong zero-width character.
U+200F (RLM)	RIGHT-TO-LEFT MARK	Right-to-left strong zero-width character.
*/
	ret[0] = 0x202D;
	MultiByteToWideChar(CP_UTF8, 0, utf8, length, ret + 1, size);
	for ( i = 1; i < size + 1; i++) {
		if (( ret[i] >= 0x202A && ret[i] <= 0x202E) || ret[i] == 0x200F )
			ret[i] = 0x200E;
	}
	if ( mb_len ) *mb_len = size + 1;
	return ret;
}

void
wchar2char( char * dest, WCHAR * src, int lim)
{
	if ( lim < 1) return;
	while ( lim-- && *src) *(dest++) = *((char*)src++);
	if ( lim < 0) dest--;
	*dest = 0;
}

void
char2wchar( WCHAR * dest, char * src, int lim)
{
	int l = strlen( src) + 1;
	if ( lim < 0) lim = l;
	if ( lim == 0) return;
	if ( lim > l) lim = l;
	src  += lim - 2;
	dest += lim - 1;
	*(dest--) = 0;
	while ( --lim) *(dest--) = *(src--);
}

WCHAR *
alloc_ascii_to_wchar( const char * text, int length)
{
	WCHAR * ret;
	if ( text == NULL ) text = "";
	if ( length < 0) length = strlen( text) + 1;
	if ( !( ret = malloc( length * sizeof( WCHAR)))) return nil;
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, text, length, ret, length * 2);
	return ret;
}

static WCHAR*
path2wchar(const char *name, Bool is_utf8, int * size)
{
	WCHAR * text;
	int xlen = -1;
	if ( size == NULL ) size = &xlen;
	if ( is_utf8 ) {
		text = alloc_utf8_to_wchar( name, *size, size);
	} else {
		*size = strlen( name) + 1;
		text = alloc_ascii_to_wchar( name, *size);
	}
	if ( !text ) errno = ENOMEM;
	return text;
}

void
win32_set_errno(void)
{
	/*
	This isn't perfect, eg. Win32 returns ERROR_ACCESS_DENIED for
	both permissions errors and if the source is a directory, while
	POSIX wants EACCES and EPERM respectively.

	Determined by experimentation on Windows 7 x64 SP1, since MS
	don't document what error codes are returned.
	*/
	switch (GetLastError()) {
	case ERROR_BAD_NET_NAME:
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_PATHNAME:
	case ERROR_FILE_NOT_FOUND:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_DRIVE:
	case ERROR_PATH_NOT_FOUND:
		errno = ENOENT;
		break;
	case ERROR_ALREADY_EXISTS:
		errno = EEXIST;
		break;
	case ERROR_ACCESS_DENIED:
		errno = EACCES;
		break;
	case ERROR_NOT_SAME_DEVICE:
		errno = EXDEV;
		break;
	case ERROR_DISK_FULL:
		errno = ENOSPC;
		break;
	case ERROR_NOT_ENOUGH_QUOTA:
		errno = EDQUOT;
		break;
	default: /* ERROR_INVALID_FUNCTION - eg. on a FAT volume */
		errno = EINVAL;
		break;
	}
}

int
apc_fs_access(const char *name, Bool is_utf8, int mode, Bool effective)
{
	WCHAR *buf;
	int ret;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(name, is_utf8, NULL)))
			return false;
		ret = _waccess(buf, mode);
		free(buf);
	} else
		ret = access(name, mode);

	return ret;
}

Bool
apc_fs_chdir(const char *path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		if ( !( ok = SetCurrentDirectoryW(buf)))
			win32_set_errno();
		free(buf);
	} else {
		if ( !( ok = SetCurrentDirectoryA(path)))
			win32_set_errno();
	}

	return ok;
}

Bool
apc_fs_chmod( const char *path, Bool is_utf8, int mode)
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		ok = (_wchmod(buf, mode) == 0);
		free(buf);
	} else
		ok = (chmod(path, mode) == 0);

	return ok;
}

char *
alloc_wchar_to_utf8( WCHAR * src, int * len )
{
	int xlen = -1, srclen;
	char * ret;

	if ( !len ) len = &xlen;
	srclen = *len;

	if (( *len = WideCharToMultiByte(CP_UTF8, 0, src, srclen, NULL, 0, NULL, false)) == 0 ) {
		errno = EINVAL;
		return NULL;
	}
	if ( !( ret = malloc( *len ))) {
		errno = ENOMEM;
		return NULL;
	}
	if ( WideCharToMultiByte(CP_UTF8, 0, src, srclen, ret, *len, NULL, false) == 0) {
		free(ret);
		errno = EINVAL;
		return NULL;
	}
	return ret;
}

static char *
wstr2ascii( WCHAR * src, int * len, Bool fail_if_cannot )
{
	char * ret;
	int srclen = *len;
	DWORD flags = 0;
#ifdef WC_ERR_INVALID_CHARS
	if ( fail_if_cannot ) flags |= WC_ERR_INVALID_CHARS;
#endif
	if (( *len = WideCharToMultiByte(CP_ACP, flags, src, srclen, NULL, 0, NULL, false)) == 0 )
		return NULL;
	if ( !( ret = malloc( *len )))
		return NULL;
	if ( WideCharToMultiByte(CP_ACP, flags, src, srclen, ret, *len, NULL, false) == 0) {
		free(ret);
		return NULL;
	}
#ifndef WC_ERR_INVALID_CHARS
	if ( fail_if_cannot ) {
		int nq1 = 0, nq2 = 0, xlen;
		char * ret2 = ret;
		xlen = srclen;
		while (xlen--) if ( *(src++) == L'?' ) nq1++;
		xlen = srclen;
		while (xlen--) if ( *(ret2++) == '?' ) nq2++;
		if ( nq2 > nq1 ) {
			free(ret);
			return NULL;
		}
	}
#endif
	return ret;
}

char *
apc_fs_from_local(const char * text, int * len)
{
	char * ret;
	WCHAR * buf;
	if ( !( buf = alloc_ascii_to_wchar( text, *len)))
		return NULL;
	ret = alloc_wchar_to_utf8( buf, len );
	free( buf );
	return ret;
}


char *
apc_fs_to_local(const char * text, Bool fail_if_cannot, int * len)
{
	WCHAR *buf;
	char * ret;

	if ( !( buf = path2wchar(text, true, len)))
		return NULL;
	ret = wstr2ascii( buf, len, fail_if_cannot );
	free(buf);

	return ret;
}

char*
apc_fs_getcwd(void)
{
	int i;
	WCHAR fn[MAX_PATH+1];

	if ( !GetCurrentDirectoryW(MAX_PATH+1, fn)) {
		errno = EACCES;
		return NULL;
	}
	for ( i = 0; i < MAX_PATH; i++) {
		if ( fn[i] == 0 ) break;
		if ( fn[i] == L'\\' ) fn[i] = L'/';
	}
	return alloc_wchar_to_utf8(fn, NULL);
}

char*
apc_fs_getenv(const char * varname, Bool is_utf8, Bool * do_free)
{
	WCHAR * e, * buf;

	if ( !( buf = path2wchar(varname, is_utf8, NULL)))
		return NULL;
	e = _wgetenv(buf);
	free(buf);
	if ( !e ) return NULL;

	*do_free = true;
	return alloc_wchar_to_utf8(e, NULL);
}

Bool
apc_fs_link( const char* oldname, Bool is_old_utf8, const char * newname, Bool is_new_utf8 )
{
	WCHAR *buf1, *buf2;
	Bool ok;

	if ( !( buf1 = path2wchar(oldname, is_old_utf8, NULL)))
		return false;
	if ( !( buf2 = path2wchar(newname, is_new_utf8, NULL))) {
		free(buf1);
		return false;
	}
	if ( !( ok = ( CreateHardLinkW(buf2, buf1, NULL) == 0)))
		win32_set_errno();
	free(buf2);
	free(buf1);

	return ok;
}

Bool
apc_fs_mkdir( const char* path, Bool is_utf8, int mode)
{
	WCHAR *buf;
	Bool ok;

	if ( !( buf = path2wchar(path, is_utf8, NULL)))
		return false;
	ok = (_wmkdir(buf) == 0);
	if ( ok ) _wchmod(buf, mode);
	free(buf);

	return ok;
}

int
apc_fs_open_file( const char* path, Bool is_utf8, int flags, int mode)
{
	WCHAR *buf;
	int f;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		f = _wopen(buf, flags, mode);
		free(buf);
	} else
		f = open(path, flags, mode);

	return f;
}

Bool
apc_fs_rename( const char* oldname, Bool is_old_utf8, const char * newname, Bool is_new_utf8 )
{
	Bool ok;
	WCHAR *buf1, *buf2;
	if ( !( buf1 = path2wchar(oldname, is_old_utf8, NULL)))
		return false;
	if ( !( buf2 = path2wchar(newname, is_new_utf8, NULL))) {
		free(buf1);
		return false;
	}
	ok = ( _wrename(buf1, buf2) == 0);
	free(buf2);
	free(buf1);
	return ok;
}

Bool
apc_fs_rmdir( const char* path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		ok = (_wrmdir(buf) == 0);
		free(buf);
	} else 
		ok = (rmdir(path) == 0);

	return ok;
}

Bool
apc_fs_stat(const char *name, Bool is_utf8, Bool link, PStatRec statrec)
{
	WCHAR *buf;
	struct _stat statbuf;
	int sz = -1, osz;

	if ( !( buf = path2wchar(name, is_utf8, &sz)))
		return false;
	osz = sz;

	/* from win32_stat:
	stat() is buggy with a trailing slashes, except for the root directory of a drive:
	remove additional trailing slashes */
	while ( sz > 2 && ( buf[sz - 2] == L'\\' || buf[sz - 2] == L'/') ) {
		buf[sz - 2] = 0;
		sz--;
	}
	/* add back slash if we otherwise end up with just a drive letter */
	if ( sz == 3 && isALPHA(buf[0]) && buf[1] == L':' ) {
		if ( osz == sz ) {
			WCHAR * buf2;
			buf2 = realloc(buf, sz * 2 + 2);
			if ( !buf2 ) {
				free(buf);
				errno = ENOMEM;
				return false;
			}
			buf = buf2;
		}
		buf[sz++ - 1] = L'\\';
		buf[sz   - 1]   = 0;
	}
	if ( _wstat(buf, &statbuf) < 0 ) {
		free(buf);
		return 0;
	}
	free(buf);

	statrec-> dev     = statbuf. st_dev;
	statrec-> ino     = statbuf. st_ino;
	statrec-> mode    = statbuf. st_mode;
	statrec-> nlink   = statbuf. st_nlink;
	statrec-> uid     = statbuf. st_uid;
	statrec-> gid     = statbuf. st_gid;
	statrec-> rdev    = statbuf. st_rdev;
	statrec-> size    = statbuf. st_size;
	statrec-> blksize = -1;
	statrec-> blocks  = -1;
	statrec-> atim    = (float) statbuf.st_atime;
	statrec-> mtim    = (float) statbuf.st_mtime;
	statrec-> ctim    = (float) statbuf.st_ctime;
	return 1;
}

Bool
apc_fs_unlink( const char* path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( is_utf8 ) {
		if ( !( buf = path2wchar(path, is_utf8, NULL)))
			return false;
		ok = (_wunlink(buf) == 0);
		free(buf);
	} else
		ok = (unlink(path) == 0);

	return ok;
}

/* from win32/win32.c */

/* fix utime() so it works on directories in NT */
static Bool
filetime_from_time(PFILETIME pFileTime, float Time)
{
	time_t sec = (time_t) Time;
	struct tm *pTM = localtime(&sec);
	SYSTEMTIME SystemTime;
	FILETIME LocalTime;

	if (pTM == NULL)
		return false;

	SystemTime.wYear   = pTM->tm_year + 1900;
	SystemTime.wMonth  = pTM->tm_mon + 1;
	SystemTime.wDay    = pTM->tm_mday;
	SystemTime.wHour   = pTM->tm_hour;
	SystemTime.wMinute = pTM->tm_min;
	SystemTime.wSecond = pTM->tm_sec;
	SystemTime.wMilliseconds = ( Time - (int) Time ) * 1000;

	return SystemTimeToFileTime(&SystemTime, &LocalTime) &&
	       LocalFileTimeToFileTime(&LocalTime, pFileTime);
}

Bool
apc_fs_setenv(const char * varname, Bool is_name_utf8, const char * value, Bool is_value_utf8)
{
	WCHAR *buf1, *buf2;
	Bool ok = false;

	if ( !( buf1 = path2wchar(varname, is_name_utf8, NULL)))
		return false;
	if ( !( buf2 = path2wchar(value,   is_value_utf8, NULL))) {
		free(buf1);
		return false;
	}

	ok = (_wputenv_s(buf1, buf2) == 0);

	free(buf2);
	free(buf1);

	return ok;
}

static Bool
win32_utimes(float atime, float mtime, WCHAR * filename)
{
	Bool ok = false;
	HANDLE handle;
	FILETIME ftCreate;
	FILETIME ftAccess;
	FILETIME ftWrite;

	/* This will (and should) still fail on readonly files */
	handle = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		errno = EINVAL;
		return false;
	}

	if (!GetFileTime(handle, &ftCreate, &ftAccess, &ftWrite)) {
		win32_set_errno();
		goto EXIT;
	}
	if (
		!filetime_from_time(&ftAccess, atime) ||
		!filetime_from_time(&ftWrite, mtime)
	) {
		errno = EINVAL;
		goto EXIT;
	}
	if ( !SetFileTime(handle, &ftCreate, &ftAccess, &ftWrite)) {
		win32_set_errno();
		goto EXIT;
	}

	CloseHandle(handle);
	ok = true;
EXIT:
	return ok;
}


Bool
apc_fs_utime( double atime, double mtime, const char* path, Bool is_utf8 )
{
	WCHAR *buf;
	Bool ok;

	if ( !( buf = path2wchar(path, is_utf8, NULL)))
		return false;
	ok = win32_utimes(atime, mtime, buf);
	free(buf);

	return ok;
}

#ifdef __cplusplus
}
#endif
