#include "win32\win32guts.h"
#include "Window.h"
#include "Application.h"
#include "Clipboard.h"
#include "Icon.h"

#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle


Bool
apc_clipboard_create()
{
   return true;
}

void
apc_clipboard_destroy()
{
}


Bool
apc_clipboard_open()
{
   if ( !OpenClipboard( nil)) apiErrRet;
   return true;
}

void
apc_clipboard_close()
{
   if ( !CloseClipboard()) apiErr;
}

void
apc_clipboard_clear()
{
   if ( !EmptyClipboard()) apiErr;
}

static long cf2CF( long id)
{
   if ( id == cfText)   return CF_TEXT;
   if ( id == cfBitmap) return CF_BITMAP;
   return id - cfCustom;
}

Bool
apc_clipboard_has_format( long id)
{
   return IsClipboardFormatAvailable( cf2CF( id));
}

void *
apc_clipboard_get_data( long id, int * length)
{
   id = cf2CF( id);
   switch( id)
   {
      case CF_BITMAP:
         {
             PImage image      = ( PImage) *length;
             Handle self       = ( Handle) image;
             HBITMAP b = GetClipboardData( CF_BITMAP);
             HBITMAP op, p = GetClipboardData( CF_PALETTE);
             HBITMAP obm = sys bm;
             XBITMAPINFO xbi;
             HDC dc, ops;

             if ( b == nil) {
                apcErr( errInvClipboardData);
                return nil;
             }
             sys bm = b;
             apcErrClear;
             dc = CreateCompatibleDC( dc_alloc());
             ops = sys ps;
             sys ps = dc;

             if ( p) {
                op = SelectPalette( dc, p, 1);
                RealizePalette( dc);
             }
             image_query_bits( self, true);
             if ( p)
                 SelectPalette( dc, op, 1);
             DeleteDC( dc);
             dc_free();
             sys ps = ops;
             sys bm = obm;
             if ( apcError) return nil;
             return ( void*)1;
         }
         break;
      case CF_TEXT:
         {
             char *ret, *ptr;
             int i, len;
             void *ph = GetClipboardData( id);

             if ( ph == nil) {
                apcErr( errInvClipboardData);
                return nil;
             }
             if ( !( ptr = ( char*) GlobalLock( ph)))
                apiErrRet;
             len = *length = strlen( ptr) + 1;
             ret = malloc( *length);
             strcpy( ret, ptr);
             for ( i = 0; i < len - 1; i++)
                if ( ret[ i] == '\r') {
                   memcpy( ret + i, ret + i + 1, len - i + 1);
                   len--;
                }
             GlobalUnlock( ph);
             return ret;
         }
         break;
      default:
         {
            char *ptr;
            void *ret;
            void *ph = GetClipboardData( id);

            if ( ph == nil) {
               apcErr( errInvClipboardData);
               return nil;
            }
            *length = GlobalSize( ph);
            if ( *length == 0)
               return nil; // not an error
            if ( !( ptr = ( char*) GlobalLock( ph)))
               apiErrRet;
            memcpy( ret = malloc( *length), ptr, *length);
            GlobalUnlock( ph);
            return ret;
         }
   }
   return nil;
}

Bool
apc_clipboard_set_data( long id, void * data, int length)
{
   id = cf2CF( id);
   if ( data == nil) {
      if ( !EmptyClipboard()) apiErr;
      return true;
   }

   switch ( id)
   {
      case CF_BITMAP:
         {
            HPALETTE p = palette_create(( Handle) data);
            HBITMAP b = ( HBITMAP) image_make_bitmap_handle(( Handle) data, p);

            if ( b == nilHandle) {
               if ( p) DeleteObject( p);
               apiErrRet;
            }
            if ( !SetClipboardData( CF_BITMAP,  b)) apiErr;
            if ( p)
               if ( !SetClipboardData( CF_PALETTE, p)) apiErr;
         }
         break;
      case CF_TEXT:
         {
             void *ptr;
             HGLOBAL glob = GlobalAlloc( GMEM_DDESHARE, length);
             if ( !glob) apiErrRet;
             if ( !( ptr = GlobalLock( glob))) apiErrRet;
             memcpy( ptr, data, length);
             GlobalUnlock( glob);
             if ( !SetClipboardData( CF_TEXT, glob)) apiErr;

             glob = GlobalAlloc( GMEM_DDESHARE, length);
             if ( !glob) apiErrRet;
             if ( !( ptr = GlobalLock( glob))) apiErrRet;
             CharToOemBuff( data, ptr, length);
             GlobalUnlock( glob);
             if ( !SetClipboardData( CF_OEMTEXT, glob)) apiErr;

             if ( IS_NT) {
                glob = GlobalAlloc( GMEM_DDESHARE, length * sizeof( WCHAR));
                if ( !glob) apiErrRet;
                if ( !( ptr = GlobalLock( glob))) apiErrRet;
                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, data, length, ptr, length * sizeof( WCHAR));
                GlobalUnlock( glob);
                if ( !SetClipboardData( CF_UNICODETEXT, glob)) apiErr;
             }
         }
         break;
      default:
         {
             char* ptr;
             HGLOBAL glob = GlobalAlloc( GMEM_DDESHARE, length);
             if ( !glob) apiErrRet;
             if ( !( ptr = GlobalLock( glob))) {
                apiErr;
                GlobalFree( glob);
                return false;
             }
             memcpy( ptr, data, length);
             GlobalUnlock( glob);
             if ( !SetClipboardData( id, glob)) apiErrRet;
         }
    }
    return false;
}

long
apc_clipboard_register_format( const char * format)
{
   UINT r;
   if ( !( r = RegisterClipboardFormat( format))) apiErrRet;
   return r + cfCustom;
}

void
apc_clipboard_deregister_format( long id)
{
   // Windows doesn't have such functionality. Such a strange malfunction
}

ApiHandle
apc_clipboard_get_handle( Handle self)
{
   return nilHandle;
}


