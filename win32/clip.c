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
#include "Window.h"
#include "Application.h"
#include "Clipboard.h"
#include "Icon.h"

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
   return l;
}

Bool
apc_clipboard_create( Handle self)
{
   if ( !((PClipboard)self)-> name
        || strlen(((PClipboard)self)-> name) != 9
        || ((PClipboard)self)-> name[0] != 'C'
        || strcmp(((PClipboard)self)-> name, "Clipboard") != 0) {
      return false;
   }
   return true;
}

Bool
apc_clipboard_destroy( Handle self)
{
   return true;
}


Bool
apc_clipboard_open( Handle self)
{
   if ( !OpenClipboard( nil)) apiErrRet;
   return true;
}

Bool
apc_clipboard_close( Handle self)
{
   if ( !CloseClipboard()) apiErrRet;
   return true;
}

Bool
apc_clipboard_clear( Handle self)
{
   if ( !EmptyClipboard()) apiErrRet;
   return true;
}

static long cf2CF( long id)
{
   if ( id == cfText)   return CF_TEXT;
   if ( id == cfBitmap) return CF_BITMAP;
   return id - cfCustom;
}

Bool
apc_clipboard_has_format( Handle self, long id)
{
   return IsClipboardFormatAvailable( cf2CF( id));
}

void *
apc_clipboard_get_data( Handle self, long id, int * length)
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
            *length = *(( int*) ptr);
            ptr += sizeof( int);
            memcpy( ret = malloc( *length), ptr, *length);
            GlobalUnlock( ph);
            return ret;
         }
   }
   return nil;
}

Bool
apc_clipboard_set_data( Handle self, long id, void * data, int length)
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
         return true;
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
         return true;
      default:
         {
             char* ptr;
             HGLOBAL glob = GlobalAlloc( GMEM_DDESHARE, length + sizeof( int));
             if ( !glob) apiErrRet;
             if ( !( ptr = GlobalLock( glob))) {
                apiErr;
                GlobalFree( glob);
                return false;
             }
             memcpy( ptr + sizeof( int), data, length);
             memcpy( ptr, &length, sizeof( int));
             GlobalUnlock( glob);
             if ( !SetClipboardData( id, glob)) apiErrRet;
             return true;
         }
    }
    return false;
}

long
apc_clipboard_register_format( Handle self, const char * format)
{
   UINT r;
   if ( !( r = RegisterClipboardFormat( format))) apiErrRet;
   return r + cfCustom;
}

Bool
apc_clipboard_deregister_format( Handle self, long id)
{
   // Windows doesn't have such functionality. Such a strange malfunction
   return true;
}

ApiHandle
apc_clipboard_get_handle( Handle self)
{
   return nilHandle;
}


