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
#include "win32\win32guts.h"
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
   id = cf2CF( id);
   return IsClipboardFormatAvailable( id) || 
      (( id == CF_TEXT) && IsClipboardFormatAvailable( CF_UNICODETEXT));
}

Bool
apc_clipboard_get_data( Handle self, long id, PClipboardDataRec c)
{
   id = cf2CF( id);
   switch( id)
   {
      case CF_BITMAP:
         {
             PImage image = (PImage) c-> image;
             Handle self  = c-> image;
             HBITMAP b = GetClipboardData( CF_BITMAP);
             HBITMAP op, p = GetClipboardData( CF_PALETTE);
             HBITMAP obm = sys bm;
             XBITMAPINFO xbi;
             HDC dc, ops;

             if ( b == nil) {
                apcErr( errInvClipboardData);
                return false;
             }
             sys bm = b;
             apcErrClear;
             if (!( dc = CreateCompatibleDC( dc_alloc()))) return false;
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
             return apcError ? 0 : 1;
         }
         break;
      case CF_TEXT:
         if ( wantUnicodeInput && HAS_WCHAR && IsClipboardFormatAvailable( CF_UNICODETEXT)) {
             WCHAR *ptr, *p;
             int i, len, size;
             char *utf;
             Bool ret = false;
             void *ph = GetClipboardData( CF_UNICODETEXT);

             if ( ph == nil) {
                apcErr( errInvClipboardData);
                return false;
             }
             if ( !( ptr = ( WCHAR*) GlobalLock( ph)))
                apiErrRet;
             c-> text. utf8 = true;
             c-> text. length = len = 0;
             p = ptr;
             while ( *(p++)) len++;
             p = ptr;
             size = len + 8; 
             if (( utf = c-> text. text = malloc( size))) {
                for ( i = 0; i < len; i++, p++) {
                   if ( *p == '\r') continue;
                   if ( c-> text. length > size - 8) {
                      int dt = utf - c-> text. text;
                      char * d = malloc( size *= 2);
                      if ( !d) goto CLEAR;
                      memcpy( d, c-> text. text, c-> text. length);
                      utf = c-> text. text + dt;
                   }
                   utf = uvchr_to_utf8( utf, *p);
                }
             }
             ret = true;
          CLEAR:;
             *utf = 0;
             c-> text. length = strlen( utf);
             GlobalUnlock( ph);
             return ret;
         } else {
             char *ptr;
             int i, len, ret = false;
             void *ph = GetClipboardData( CF_TEXT);

             if ( ph == nil) {
                apcErr( errInvClipboardData);
                return false;
             }
             if ( !( ptr = ( char*) GlobalLock( ph)))
                apiErrRet;
             c-> text. utf8 = false;
             len = strlen( ptr);
             c-> text. length = 0;
             if ((c-> text. text = ( char *) malloc( len))) {
                for ( i = 0; i < len; i++) 
                   if ( ptr[i] != '\r' || (( i < len) && (ptr[i+1] != '\n'))) 
                      c-> text. text[c-> text. length++] = ptr[i];
                ret = true;
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
               return false;
            }
            if (( c-> binary. length = GlobalSize( ph)) == 0)
               return true; /* not an error */
            if ( !( ptr = ( char*) GlobalLock( ph)))
               apiErrRet;
            c-> binary. length = *(( int*) ptr);
            ptr += sizeof( int);
            if (( c-> binary. data = malloc( c-> binary. length)))
               memcpy( c-> binary. data, ptr, c-> binary. length);
            GlobalUnlock( ph);
            return true;
         }
   }
   return false;
}

Bool
apc_clipboard_set_data( Handle self, long id, PClipboardDataRec c)
{
   id = cf2CF( id);
   switch ( id)
   {
      case CF_BITMAP:
         {
            HPALETTE p = palette_create( c-> image);
            HBITMAP b = ( HBITMAP) image_make_bitmap_handle( c-> image, p);

            if ( b == nil) {
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
            int ulen = c-> text. utf8 ? 
                    utf8_length( c-> text. text, c-> text. text + c-> text. length) : 
                    c-> text. length;
            int i, cr = 0;
            void *ptr, *oemptr;
            char *dst;
            HGLOBAL glob, oemglob;

            for ( i = 0; i < c-> text. length; i++) 
               if (c-> text. text[i] == '\n' && ( i == 0 || c-> text. text[i-1] != '\r')) 
                  cr++;

            if ( !HAS_WCHAR || !c-> text. utf8) {
               glob    = GlobalAlloc( GMEM_DDESHARE, ulen + cr + 1);
               oemglob = GlobalAlloc( GMEM_DDESHARE, ulen + cr + 1);
               if ( glob)    ptr    = GlobalLock( glob);
               if ( oemglob) oemptr = GlobalLock( oemglob);

               if ( ptr && oemptr) {
                  if ( c-> text. utf8) {
                     WCHAR * buf = malloc( sizeof( WCHAR) * ulen);
                     if ( !buf) goto FAIL;
                     utf8_to_wchar( c-> text. text, buf, ulen);
                     WideCharToMultiByte( CP_ACP,   0, buf, ulen, ptr,    ulen + cr + 1, nil, nil);
                     WideCharToMultiByte( CP_OEMCP, 0, buf, ulen, oemptr, ulen + cr + 1, nil, nil);
                  } else {
                     dst = ( char *) ptr;
                     for ( i = 0; i < c-> text. length; i++) {
                        if ( c-> text. text[i] == '\n' && ( i == 0 || c-> text. text[i-1] != '\r')) 
                           *(dst++) = '\r';
                        *(dst++) = c-> text. text[i];
                     }
                     *dst = 0;
                     CharToOemBuff(( LPCTSTR) ptr, ( LPTSTR) oemptr, ulen + cr + 1);
                  }
                  GlobalUnlock( oemptr);
                  GlobalUnlock( ptr);
                  if ( !SetClipboardData( CF_TEXT, glob)) apiErr;
                  if ( !SetClipboardData( CF_OEMTEXT, oemglob)) apiErr;
               } else {
                  apiErr;
               FAIL:
                  if ( ptr) GlobalUnlock( ptr);
                  if ( oemptr) GlobalUnlock( oemptr);
                  if ( glob) GlobalFree( glob);
                  if ( oemglob) GlobalFree( oemglob);
               }
            }
            
            if ( HAS_WCHAR) {
               if (( glob = GlobalAlloc( GMEM_DDESHARE, ( ulen + 1) * sizeof( WCHAR)))) {
                  if (( ptr = GlobalLock( glob))) {
                     if ( c-> text. utf8)
                        utf8_to_wchar( c-> text. text, ptr, ulen + 1);
                     else
                        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, 
                            c-> text. text, c-> text. length + 1, 
                            ( LPWSTR) ptr,  ulen + 1);
                     GlobalUnlock( glob);
                     if ( !SetClipboardData( CF_UNICODETEXT, glob)) apiErr;
                  } else {
                     GlobalFree( glob);
                     apiErr;
                  }
               } else apiErr;
            } 
         }
         return true;
      default:
         {
             char* ptr;
             HGLOBAL glob = GlobalAlloc( GMEM_DDESHARE, c-> text. length + sizeof( int));
             if ( !glob) apiErrRet;
             if ( !( ptr = ( char *) GlobalLock( glob))) {
                apiErr;
                GlobalFree( glob);
                return false;
             }
             memcpy( ptr + sizeof( int), c-> text. text, c-> text. length);
             memcpy( ptr, &c-> text. length, sizeof( int));
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

#ifdef __cplusplus
}
#endif
