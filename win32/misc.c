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
apc_query_drives_map( const char *firstDrive, char *map, int len)
{
	char *m = map;
	int beg;
	DWORD driveMap;
	int i;

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
	strlcpy( buf, drive, 255);             //     sometimes D: isn't enough for 95,
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
	return guts. insert_mode;
}

Bool
apc_sys_set_insert_mode( Bool insMode)
{
	guts. insert_mode = insMode;
	return true;
}

Point
get_window_borders( int border_style)
{
	Point ret = { 0, 0};
	switch ( border_style)
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
		RegQueryValueEx( hKey, "MenuShowDelay", NULL, &valType, ( LPBYTE) buf, &valSize);
		RegCloseKey( hKey);
		return atol( buf);
	case svFullDrag       :
		RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey);
		RegQueryValueEx( hKey, "DragFullWindows", NULL, &valType, ( LPBYTE)buf, &valSize);
		RegCloseKey( hKey);
		return atol( buf);
	case svDblClickDelay   :
		return guts.mouse_double_click_delay;
	case svWheelPresent    : return GetSystemMetrics( SM_MOUSEWHEELPRESENT);
	case svXIcon           : return guts. icon_size_large. x;
	case svYIcon           : return guts. icon_size_large. y;
	case svXSmallIcon      : return guts. icon_size_small. x;
	case svYSmallIcon      : return guts. icon_size_small. y;
	case svXPointer        : return guts. pointer_size. x;
	case svYPointer        : return guts. pointer_size. y;
	case svXScrollbar      : return GetSystemMetrics( SM_CXHSCROLL);
	case svYScrollbar      : return GetSystemMetrics( SM_CYVSCROLL);
	case svXCursor         : return GetSystemMetrics( SM_CXBORDER);
	case svAutoScrollFirst : return 200;
	case svAutoScrollNext  : return 50;
	case svInsertMode      : return guts. insert_mode;
	case svXbsNone         : return 0;
	case svYbsNone         : return 0;
	case svXbsSizeable     : return GetSystemMetrics( SM_CXFRAME);
	case svYbsSizeable     : return GetSystemMetrics( SM_CYFRAME);
	case svXbsSingle       : return GetSystemMetrics( SM_CXBORDER);
	case svYbsSingle       : return GetSystemMetrics( SM_CYBORDER);
	case svXbsDialog       : return GetSystemMetrics( SM_CXDLGFRAME);
	case svYbsDialog       : return GetSystemMetrics( SM_CYDLGFRAME);
	case svShapeExtension  : return 1;
	case svColorPointer    : return guts. display_bm_info. bmiHeader. biBitCount > 4;
	case svCanUTF8_Input   : return 1;
	case svCanUTF8_Output  : return 1;
	case svCompositeDisplay: return is_dwm_enabled();
	case svLayeredWidgets: return guts. display_bm_info. bmiHeader. biBitCount > 8;
	case svFixedPointerSize: return 0;
	case svMenuCheckSize   : return GetSystemMetrics( SM_CXMENUCHECK );
	case svFriBidi         : return prima_guts.use_fribidi;
	case svLibThai         : return prima_guts.use_libthai;
	case svAntialias       : return 1;
	default:
		return -1;
	}
	return 0;
}

PFont
apc_sys_get_msg_font( PFont copyTo)
{
	*copyTo = guts. msg_font;
	copyTo-> pitch = fpDefault;
	return copyTo;
}

PFont
apc_sys_get_caption_font( PFont copyTo)
{
	*copyTo = guts. cap_font;
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
	if (( cache = ( Handle) hash_fetch( mgr_registry, path, strlen( path)))) {
		if ( info) *info = cache;
		return cache & rgxExists;
	}

	if ( RegOpenKeyEx( hk, path, 0,
							KEY_READ, &hKey) != ERROR_SUCCESS) {
		hash_store( mgr_registry, path, strlen( path), (void*) rgxNotExists);
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
	hash_store( mgr_registry, path, strlen( path), (void*) cache);
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
		if ( prf_exists( hk, buf, NULL)) {
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
apc_fetch_resource( const char *class_name, const char *name,
	const char *resClass, const char *resName,
	Handle owner, int resType,
	void *val
) {
	Bool res = true;
	HKEY hKey;
	char buf[ MAXREGLEN];
	DWORD type, size, i;
	List ids[ 2];

	i = 2; while( i--) list_create(&ids[i], 8, 8);

	list_add(&ids[1], ( Handle) duplicate_string( name));
	list_add(&ids[0], ( Handle) duplicate_string( class_name));

	while ( owner) {
		list_insert_at(&ids[1],   ( Handle) prima_normalize_resource_string(
			duplicate_string( PComponent( owner)-> name), false), 0);
		list_insert_at(&ids[0], ( Handle) prima_normalize_resource_string(
			duplicate_string(
				( owner == prima_guts.application) ? "Prima" : CComponent( owner)-> className
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
		return NULL;
	}
	if ( !( ret = malloc( size * sizeof( WCHAR)))) return NULL;
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
		return NULL;
	}
	if ( !( ret = malloc((size + 1) * sizeof( WCHAR)))) return NULL;
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
alloc_ascii_to_wchar( const char * text, int *length)
{
	WCHAR * ret;
	int src_len, dst_len;

	src_len = length ? *length : -1;
	if ( length )
		*length = 0;
	if ( text == NULL || src_len == 0 )
		return NULL;
	if ( src_len < 0)
		src_len = strlen( text) + 1;
	if ( ( dst_len = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, text, src_len, NULL, 0)) <= 0)
		return NULL;
	if ( length )
		*length = dst_len;
	if ( !( ret = malloc( dst_len * sizeof( WCHAR)))) return NULL;
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, text, src_len, ret, dst_len * 2);

	return ret;
}

#ifdef __cplusplus
}
#endif
