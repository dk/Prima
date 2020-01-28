#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include "Window.h"
#include "Application.h"
#include "Clipboard.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

PList
apc_get_standard_clipboards( void)
{
	PList l = plist_create( 1, 1);
	if (!l) return nil;
	list_add( l, (Handle)duplicate_string( "Clipboard"));
	list_add( l, (Handle)duplicate_string( "DragDrop"));
	return l;
}

Bool
apc_clipboard_create( Handle self)
{
	char * c = ((PClipboard)self)-> name;
	if ( !c ) return false;

	if (strcmp(c, "Clipboard") == 0)
		guts.clipboards[CLIPBOARD_MAIN] = self;
	else if (strcmp(c, "DragDrop") == 0) {
		guts.clipboards[CLIPBOARD_DND] = self;
		return dnd_clipboard_create();
	} else
		return false;
	return true;
}

Bool
apc_clipboard_destroy( Handle self)
{
	int i;
	if (self == guts.clipboards[CLIPBOARD_DND])
		dnd_clipboard_destroy();
	for ( i = 0; i < 2; i++)
		if ( guts.clipboards[i] == self )
			guts.clipboards[i] = nilHandle;
	return true;
}

Bool
apc_clipboard_open( Handle self)
{
	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_open();

	if ( !OpenClipboard( nil)) apiErrRet;
	return true;
}

Bool
apc_clipboard_close( Handle self)
{
	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_close();

	if ( !CloseClipboard()) apiErrRet;
	return true;
}

Bool
apc_clipboard_clear( Handle self)
{
	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_clear();

	if ( !EmptyClipboard()) apiErrRet;
	return true;
}

static Handle cf2CF( Handle id)
{
	if ( id == cfText)   return CF_TEXT;
	if ( id == cfUTF8)   return CF_UNICODETEXT;
	if ( id == cfBitmap) return CF_BITMAP;
	return id - cfCustom;
}

static struct {
   UINT format;
   char desc[ 20];
} formats[] = {
      {CF_METAFILEPICT     , "CF_METAFILEPICT"
   }, {CF_SYLK             , "CF_SYLK"
   }, {CF_DIF              , "CF_DIF"
   }, {CF_TIFF             , "CF_TIFF"
   }, {CF_OEMTEXT          , "CF_OEMTEXT"
   }, {CF_PENDATA          , "CF_PENDATA"
   }, {CF_RIFF             , "CF_RIFF"
   }, {CF_WAVE             , "CF_WAVE"
   }, {CF_ENHMETAFILE      , "CF_ENHMETAFILE"
   }, {CF_HDROP            , "CF_HDROP"
   }, {CF_LOCALE           , "CF_LOCALE"
   }, {CF_DIBV5            , "CF_DIBV5"
   }, {CF_MAX              , "CF_MAX"
   }
};

char *
cf2name(UINT f)
{
	switch ( f ) {
	case CF_TEXT:
		return duplicate_string("Text");
		break;
	case CF_BITMAP:
		return duplicate_string("Image");
		break;
	case CF_UNICODETEXT:
		return duplicate_string("UTF8");
		break;
	case CF_DIB:
		return duplicate_string("CF_DIB");
		break;
	case CF_PALETTE:
		return duplicate_string("CF_PALETTE");
		break;
	default: {
		int i = 0;
		char name[256];
		while ( formats[i]. format != CF_MAX) {
			if ( formats[i]. format == f)
				return duplicate_string(formats[i]. desc);
			i++;
		}
		if ( GetClipboardFormatName( f, name, 255))
			return duplicate_string(name);
	}}
	return NULL;
}

PList
apc_clipboard_get_formats( Handle self)
{
	UINT f = 0;
	PList list;

	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_get_formats();

	list = plist_create(8, 8);
	while (( f = EnumClipboardFormats( f))) {
		char * name = cf2name(f);
		if ( f )
			list_add(list, (Handle)name);
	}

	return list;
}

Bool
apc_clipboard_has_format( Handle self, Handle id)
{
	id = cf2CF( id);
	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_has_format(id);

	return IsClipboardFormatAvailable( id) ||
		(( id == CF_TEXT) && IsClipboardFormatAvailable( CF_UNICODETEXT));
}

Bool
clipboard_get_data(int cfid, PClipboardDataRec c, void * p1, void * p2)
{
	switch( cfid)
	{
		case CF_DIB: {
			PIcon i;
			XBITMAPINFO * bi = (XBITMAPINFO *) p1;
			printf("DIB\n");
			if ( bi-> bmiHeader. biCompression != BI_RGB )
				return false;
			if ( bi-> bmiHeader. biBitCount == 32 ) {
				i = (PIcon) create_object("Prima::Icon",
					"iiiii",
					"maskType",    8,
					"width",       bi-> bmiHeader. biWidth,
					"height",      bi-> bmiHeader. biHeight,
					"type",        imRGB,
					"autoMasking", amNone
				);
				if ( !i) return false;
				image_argb_query_bits((Handle) i);
			} else if (
				bi-> bmiHeader. biBitCount == 1 ||
				bi-> bmiHeader. biBitCount == 4 ||
				bi-> bmiHeader. biBitCount == 8
			) {
				Byte * data;
				int j, colors = bi-> bmiHeader. biClrUsed;

				i = (PIcon) create_object("Prima::Image",
					"iii",
					"width",       bi-> bmiHeader. biWidth,
					"height",      bi-> bmiHeader. biHeight,
					"type",        bi-> bmiHeader. biBitCount
				);
				if ( !i) return false;
				if ( colors > 256 ) colors = 256;
				i->palSize = colors;
				for ( j = 0; j < colors; j++) {
					i-> palette[ j]. r = bi-> bmiColors[ j]. rgbRed  ;
					i-> palette[ j]. g = bi-> bmiColors[ j]. rgbGreen;
					i-> palette[ j]. b = bi-> bmiColors[ j]. rgbBlue ;
				}

				data = (Byte*)bi;
				data += sizeof(BITMAPINFOHEADER) + bi-> bmiHeader.biClrUsed * sizeof(RGBQUAD);
				memcpy( i->data, data, i->dataSize );
			} else
				return false;
			c->image = (Handle) i;
			return true;
		}
		case CF_BITMAP: {
			Handle self;
			HBITMAP b = (HBITMAP) p1;
			HPALETTE op = nil, p = (HPALETTE) p2;
			HBITMAP obm;
			HDC dc, ops;

			self = (Handle) create_object("Prima::Image", "");
			obm = sys bm;
			sys bm = b;
			apcErrClear;
			if (!( dc = CreateCompatibleDC( dc_alloc()))) {
				Object_destroy(self);
				return false;
			}
			ops = sys ps;
			sys ps = dc;

			if ( p) {
				op = SelectPalette( dc, p, 1);
				RealizePalette( dc);
			}
			image_query_bits(self, true);
			if ( p)
				SelectPalette( dc, op, 1);
			DeleteDC( dc);
			dc_free();
			sys ps = ops;
			sys bm = obm;
			c->image = self;
			return true;
		}
		case CF_UNICODETEXT: {
			WCHAR *ptr;
			Bool ret = false;

			if ( !( ptr = ( WCHAR*) GlobalLock( p1)))
				apiErrRet;
			apcErrClear;
			c->length = WideCharToMultiByte(CP_UTF8, 0, ptr, -1, NULL, 0, NULL, 0);
			if (( c->data = malloc( c-> length ) )) {
				WideCharToMultiByte(CP_UTF8, 0, ptr, -1, (LPSTR)c->data, c->length, NULL, 0);
				if ( c->length > 0) c->length--; // terminating 0
				ret = true;
			} else {
				c->length = 0;
			}
			GlobalUnlock( p1);
			return ret;
		}
		case CF_TEXT: {
			Byte *ptr;
			int i, len, ret = false;

			if ( !( ptr = ( Byte*) GlobalLock( p1)))
				apiErrRet;
			apcErrClear;
			len = strlen(( char*) ptr);
			c-> length = 0;
			if ((c-> data = ( Byte *) malloc( len))) {
				for ( i = 0; i < len; i++)
					if ( ptr[i] != '\r' || (( i < len) && (ptr[i+1] != '\n')))
						c-> data[c-> length++] = ptr[i];
				ret = true;
			}
			GlobalUnlock( p1);
			return ret;
		}
		default: {
			char *ptr;
			if (( c-> length = GlobalSize( p1)) == 0)
				return true; /* not an error */
			if ( !( ptr = ( char*) GlobalLock( p1)))
				apiErrRet;
			if (( c-> data = malloc( c-> length)))
				memcpy( c-> data, ptr, c-> length);
			GlobalUnlock( p1);
			return true;
		}
	}
	return false;
}

Bool
apc_clipboard_get_data( Handle self, Handle id, PClipboardDataRec c)
{
	void *ph;

printf("%x\n", id);
	id = cf2CF( id);
printf("%x\n", id);
	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_get_data(id, c);

	if ((ph = GetClipboardData(id)) == NULL) {
		apcErr( errInvClipboardData);
		return false;
	}

	return clipboard_get_data(id, c, ph,
		(id == CF_BITMAP) ? (void*) GetClipboardData( CF_PALETTE) : NULL);
}

Bool
apc_clipboard_set_data( Handle self, Handle id, PClipboardDataRec c)
{
	id = cf2CF( id);
	if (self == guts.clipboards[CLIPBOARD_DND])
		return dnd_clipboard_set_data(id, c);

	switch ( id)
	{
		case CF_BITMAP:
			{
				HPALETTE p = palette_create( c-> image);
				HBITMAP b = ( HBITMAP) image_create_bitmap( c-> image, p, NULL, BM_AUTO);

				if ( b == nil) {
					if ( p) DeleteObject( p);
					apiErrRet;
				}
				if ( !SetClipboardData( CF_BITMAP,  b)) apiErr;
				if ( p)
					if ( !SetClipboardData( CF_PALETTE, p)) apiErr;
			}
			return true;
		case CF_UNICODETEXT:
			{
				int ulen = MultiByteToWideChar(CP_UTF8, 0, (char*) c-> data, c-> length, NULL, 0) + 1;
				void *ptr = nil;
				HGLOBAL glob;

				if (( glob = GlobalAlloc( GMEM_DDESHARE, ( ulen + 0) * sizeof( WCHAR)))) {
					if (( ptr = GlobalLock( glob))) {
						MultiByteToWideChar(CP_UTF8, 0, (LPSTR)c-> data, c-> length, ptr, ulen);
						GlobalUnlock( glob);
						if ( !SetClipboardData( CF_UNICODETEXT, glob)) apiErr;
					} else {
						GlobalFree( glob);
						apiErr;
					}
				} else apiErr;
			}
			return true;
		case CF_TEXT:
			{
				int ulen = c-> length;
				int i, cr = 0;
				void *ptr = nil, *oemptr = nil;
				char *dst;
				HGLOBAL glob, oemglob;

				for ( i = 0; i < c-> length; i++)
					if (c-> data[i] == '\n' && ( i == 0 || c-> data[i-1] != '\r'))
						cr++;

				glob    = GlobalAlloc( GMEM_DDESHARE, ulen + cr + 1);
				oemglob = GlobalAlloc( GMEM_DDESHARE, ulen + cr + 1);
				if ( glob)    ptr    = GlobalLock( glob);
				if ( oemglob) oemptr = GlobalLock( oemglob);

				if ( ptr && oemptr) {
					dst = ( char *) ptr;
					for ( i = 0; i < c-> length; i++) {
						if ( c-> data[i] == '\n' && ( i == 0 || c-> data[i-1] != '\r'))
							*(dst++) = '\r';
						*(dst++) = c-> data[i];
					}
					*dst = 0;
					CharToOemBuff(( LPCTSTR) ptr, ( LPTSTR) oemptr, ulen + cr + 1);
					GlobalUnlock( oemptr);
					GlobalUnlock( ptr);
					if ( !SetClipboardData( CF_TEXT, glob)) apiErr;
					if ( !SetClipboardData( CF_OEMTEXT, oemglob)) apiErr;
				} else {
					apiErr;
					if ( ptr) GlobalUnlock( ptr);
					if ( oemptr) GlobalUnlock( oemptr);
					if ( glob) GlobalFree( glob);
					if ( oemglob) GlobalFree( oemglob);
				}
			}
			return true;
		default:
			{
				char* ptr;
				HGLOBAL glob = GlobalAlloc( GMEM_DDESHARE, c-> length);
				if ( !glob) apiErrRet;
				if ( !( ptr = ( char *) GlobalLock( glob))) {
					apiErr;
					GlobalFree( glob);
					return false;
				}
				memcpy( ptr, c-> data, c-> length);
				GlobalUnlock( glob);
				if ( !SetClipboardData( id, glob)) apiErrRet;
				return true;
			}
	}
	return false;
}

Handle
apc_clipboard_register_format( Handle self, const char * format)
{
	UINT r;
	int i = 0;
	while ( formats[i]. format != CF_MAX) {
		if ( strcmp(formats[i]. desc, format) == 0) 
			return formats[i]. format + cfCustom;
		i++;
	}
	if ( !( r = RegisterClipboardFormat( format))) apiErrRet;
	return r + cfCustom;
}

Bool
apc_clipboard_deregister_format( Handle self, Handle id)
{
	return true;
}

ApiHandle
apc_clipboard_get_handle( Handle self)
{
	return nilHandle;
}

Bool
apc_clipboard_is_dnd( Handle self)
{
	return self == guts.clipboards[CLIPBOARD_DND];
}

#ifdef __cplusplus
}
#endif
