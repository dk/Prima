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

static Handle cf2CF( Handle id)
{
   if ( id == cfText)   return CF_TEXT;
   if ( id == cfUTF8)   return CF_UNICODETEXT;
   if ( id == cfBitmap) return CF_BITMAP;
   return id - cfCustom;
}

Bool
apc_clipboard_has_format( Handle self, Handle id)
{
   id = cf2CF( id);
   return IsClipboardFormatAvailable( id) || 
      (( id == CF_TEXT) && IsClipboardFormatAvailable( CF_UNICODETEXT));
}

Bool
apc_clipboard_get_data( Handle self, Handle id, PClipboardDataRec c)
{
   id = cf2CF( id);
   switch( id)
   {
      case CF_BITMAP:
         {
             Handle self  = c-> image;
             HBITMAP b = GetClipboardData( CF_BITMAP);
             HPALETTE op = nil, p = GetClipboardData( CF_PALETTE);
             HBITMAP obm = sys bm;
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
      case CF_UNICODETEXT:
	 {
             WCHAR *ptr;
             Bool ret = false;
             void *ph;
             
             ph = GetClipboardData( CF_UNICODETEXT);
             if ( ph == nil) {
                apcErr( errInvClipboardData);
                return false;
             }
             if ( !( ptr = ( WCHAR*) GlobalLock( ph)))
                apiErrRet;
             c->length = WideCharToMultiByte(CP_UTF8, 0, ptr, -1, NULL, 0, NULL, 0);
             if (( c->data = malloc( c-> length ) )) {
                WideCharToMultiByte(CP_UTF8, 0, ptr, -1, c->data, c->length, NULL, 0); 
                if ( c->length > 0) c->length--; // terminating 0
                ret = true;
             } else {
                c->length = 0;
             }
             GlobalUnlock( ph);
             return ret;
	 }
	 break;
      case CF_TEXT:
	 {
             Byte *ptr;
             int i, len, ret = false;
             void *ph = GetClipboardData( CF_TEXT);

             if ( ph == nil) {
                apcErr( errInvClipboardData);
                return false;
             }
             if ( !( ptr = ( Byte*) GlobalLock( ph)))
                apiErrRet;
             len = strlen( ptr);
             c-> length = 0;
             if ((c-> data = ( Byte *) malloc( len))) {
                for ( i = 0; i < len; i++) 
                   if ( ptr[i] != '\r' || (( i < len) && (ptr[i+1] != '\n'))) 
                      c-> data[c-> length++] = ptr[i];
                ret = true;
             } 
             GlobalUnlock( ph);
             return ret;
         }
         break;
      default:
         {
            char *ptr;
            void *ph = GetClipboardData( id);

            if ( ph == nil) {
               apcErr( errInvClipboardData);
               return false;
            }
            if (( c-> length = GlobalSize( ph)) == 0)
               return true; /* not an error */
            if ( !( ptr = ( char*) GlobalLock( ph)))
               apiErrRet;
            c-> length = *(( int*) ptr);
            ptr += sizeof( int);
            if (( c-> data = malloc( c-> length)))
               memcpy( c-> data, ptr, c-> length);
            GlobalUnlock( ph);
            return true;
         }
   }
   return false;
}

Bool
apc_clipboard_set_data( Handle self, Handle id, PClipboardDataRec c)
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
      case CF_UNICODETEXT:
	 {
            int ulen = MultiByteToWideChar(CP_UTF8, 0, (char*) c-> data, c-> length, NULL, 0) + 1; 
            void *ptr = nil;
            HGLOBAL glob;

            if (( glob = GlobalAlloc( GMEM_DDESHARE, ( ulen + 0) * sizeof( WCHAR)))) {
               if (( ptr = GlobalLock( glob))) {
                  MultiByteToWideChar(CP_UTF8, 0, c-> data, c-> length, ptr, ulen);
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
             HGLOBAL glob = GlobalAlloc( GMEM_DDESHARE, c-> length + sizeof( int));
             if ( !glob) apiErrRet;
             if ( !( ptr = ( char *) GlobalLock( glob))) {
                apiErr;
                GlobalFree( glob);
                return false;
             }
             memcpy( ptr + sizeof( int), c-> data, c-> length);
             memcpy( ptr, &c-> length, sizeof( int));
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
   if ( !( r = RegisterClipboardFormat( format))) apiErrRet;
   return r + cfCustom;
}

Bool
apc_clipboard_deregister_format( Handle self, Handle id)
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
