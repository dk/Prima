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
/* apc.c --- apc/ api for os/2 */
#include <limits.h>
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#define INCL_VIO
#define INCL_KBD
#include <float.h>
#include <io.h>
#include <fcntl.h>
#include "os2/os2guts.h"
#include "Icon.h"
#include "Window.h"
#include "Application.h"
#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE  (( sys className == WC_FRAME) ? WinQueryWindow( var handle, QW_PARENT) : var handle)
#define DHANDLE( view) (((( PDrawableData)(( PComponent) view)-> sysData)->className == WC_FRAME) ? WinQueryWindow((( PWidget) view)->handle, QW_PARENT) : (( PWidget) view)->handle)

// Pointer

#define putbmp(x,y,bm) {                                          \
  POINTL point = {x,y};                                               \
  HPS display = WinGetScreenPS( HWND_DESKTOP);                    \
  WinDrawBitmap(display,bm,nil,&point,CLR_WHITE,CLR_BLACK,DBM_NORMAL);\
  WinReleasePS( display);                                         \
}


HPOINTER
pointer_make_handle( Handle self, Handle icon, Point hotSpot)
{
    HBITMAP m, b = nilHandle;
    BInfo2 bm;
    PIcon i = ( PIcon) icon;
    POINTERINFO pi = { true, hotSpot. x, hotSpot. y};
    Point ptSize;
    HPOINTER ret = nilHandle;

    // create mono bitmap
    unsigned char * mbuf;
    if ( kind_of( icon, CIcon))
    {
       mbuf = malloc( i-> maskSize * 2);
       if ( !mbuf && i-> maskSize > 0) return nilHandle;
       memset( &mbuf[ 0], 0, i-> maskSize);
       memcpy( &mbuf[ i-> maskSize], i-> mask, i-> maskSize);
    } else {
       int maskSize = (( i-> w + 31) / 32) * 4 * i-> h;
       mbuf = malloc( maskSize * 2);
       if ( !mbuf && i-> maskSize > 0) return nilHandle;
       memset( mbuf, 0, maskSize * 2);
    }

    if ((( i-> type & imBPP) != imMono) ||
         ( ARGB( i-> palette[0].r, i-> palette[0].g, i-> palette[0].b) != clBlack) ||
         ( ARGB( i-> palette[1].r, i-> palette[1].g, i-> palette[1].b) != clWhite)
       ) {
       b = ( HBITMAP) bitmap_make_handle( icon);
    }
    else
    {
       int j, d = i-> dataSize;
       for ( j = 0; j < d; j++) mbuf[ j] = i-> data[ j];
    }

    memset( &bm, 0, sizeof ( bm));
    bm. structLength = sizeof ( BInfo);
    bm. w            = i-> w;
    bm. h            = i-> h * 2;
    bm. bpp          = 1;
    bm. planes       = 1;
    bm. palette[1].bRed = bm. palette[1].bGreen = bm. palette[1].bBlue = 0xFF;
    bm. palette[0].bRed = bm. palette[0].bGreen = bm. palette[0].bBlue = 0;
    m = GpiCreateBitmap( guts. ps, ( PBITMAPINFOHEADER2) &bm, CBM_INIT, mbuf, ( PBITMAPINFO2) &bm);
    free( mbuf);
    if ( m == nilHandle) {
       apiErr;
       return nilHandle;
    }
    // scaling to pointer size
    ptSize = ( Point){
       WinQuerySysValue( HWND_DESKTOP, SV_CXPOINTER),
       WinQuerySysValue( HWND_DESKTOP, SV_CYPOINTER)
    };
    if ( i-> w != ptSize. x || i-> h != ptSize. y)
    {
       HBITMAP old, sm, sb = nilHandle;
       BITMAPINFOHEADER2 bi;
       POINTL pt [4] = {
         { 0, 0}, { ptSize. x - 1, ptSize. y - 1},
         { 0, 0}, { i->w, i->h}
       };
       if ( b)
       {
          // stretch color bits
          bi. cbFix = sizeof( bi);
          if ( !GpiQueryBitmapInfoHeader( b, &bi)) apiErr;
          bi. cx = ptSize. x;
          bi. cy = ptSize. y;
          sb = GpiCreateBitmap( guts. ps, &bi, 0, nil, nil);
          if ( sb == nilHandle) {
             apiErr;
             GpiDeleteBitmap( m);
             return nilHandle;
          }
          old = GpiSetBitmap( guts. ps, sb);
          if ( old == HBM_ERROR) apiErr;
          if ( GpiWCBitBlt( guts. ps, b, 4, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) {
             apiErr;
             GpiDeleteBitmap( m);
             return nilHandle;
          }
       } else {
          old = GpiSetBitmap( guts. ps, nilHandle);
          if ( old == HBM_ERROR) apiErr;
       }
       // stretch mono bits
       bi. cbFix = sizeof( bi);
       if ( !GpiQueryBitmapInfoHeader( m, &bi)) {
          apiErr;
          GpiDeleteBitmap( m);
          return nilHandle;
       }
       bi. cx = ptSize. x;
       bi. cy = ptSize. y * 2;
       pt[ 1]. y = ptSize. y * 2 - 1;
       pt[ 3]. y = i-> h * 2,
       sm = GpiCreateBitmap( guts. ps, &bi, 0, nil, nil);
       if ( sm == nilHandle) {
          apiErr;
          GpiDeleteBitmap( m);
          return nilHandle;
       }
       if ( GpiSetBitmap( guts. ps, sm) == HBM_ERROR) apiErr;
       if ( GpiWCBitBlt( guts. ps, m, 4, pt, ROP_SRCCOPY, BBO_IGNORE) == GPI_ERROR) {
          apiErr;
          GpiSetBitmap( guts. ps, nilHandle);
          GpiDeleteBitmap( sm);
          GpiDeleteBitmap( m);
          return nilHandle;
       }
       // restore old bitmap
       if ( GpiSetBitmap( guts. ps, old) == HBM_ERROR) apiErr;
       if ( b) if ( !GpiDeleteBitmap( b)) apiErr;
       if ( !GpiDeleteBitmap( m)) apiErr;
       m = sm;
       b = sb;
    }
    pi. hbmPointer = pi. hbmMiniPointer = m;
    pi. hbmColor   = pi. hbmMiniColor   = b;
    // create pointer
    ret = WinCreatePointerIndirect( HWND_DESKTOP, &pi);
    if ( !ret) {
       apiErr;
       if ( b) GpiDeleteBitmap( b);
       GpiDeleteBitmap( m);
       return nilHandle;
    }
    if ( b) GpiDeleteBitmap( b);
    GpiDeleteBitmap( m);
    return ret;
}

Point
apc_pointer_get_pos( Handle self)
{
   POINTL p;
   if ( !WinQueryPointerPos( HWND_DESKTOP, &p)) apiErr;
   return ( Point){ p.x, p.y};
}

Bool
apc_pointer_set_pos( Handle self, int x, int y)
{
   if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);
   if ( !WinSetPointerPos( HWND_DESKTOP, x, y)) apiErrRet;
   return true;
}

Point
apc_pointer_get_hot_spot( Handle self)
{
   POINTERINFO p;
   if ( !WinQueryPointerInfo( sys pointer, &p)) apiErr;
   return ( Point){ p. xHotspot, p. yHotspot};
}

Point
apc_pointer_get_size( Handle self)
{
   return ( Point) {
      WinQuerySysValue( HWND_DESKTOP, SV_CXPOINTER),
      WinQuerySysValue( HWND_DESKTOP, SV_CYPOINTER)
   };
}

int
apc_pointer_get_shape( Handle self)
{
   return sys pointerId;
}

Bool
apc_pointer_get_visible( Handle self)
{
   return !guts. pointerInvisible;
}

int ctx_cr2SPTR[] =
{
   crArrow,      SPTR_ARROW,
   crText,       SPTR_TEXT,
   crWait,       SPTR_WAIT,
   crSize,       SPTR_MOVE,
   crMove,       SPTR_MOVE,
   crSizeWest,   SPTR_SIZEWE,
   crSizeEast,   SPTR_SIZEWE,
   crSizeWE,     SPTR_SIZEWE,
   crSizeNorth,  SPTR_SIZENS,
   crSizeSouth,  SPTR_SIZENS,
   crSizeNS,     SPTR_SIZENS,
   crSizeNW,     SPTR_SIZENWSE,
   crSizeSE,     SPTR_SIZENWSE,
   crSizeNE,     SPTR_SIZENESW,
   crSizeSW,     SPTR_SIZENESW,
   crInvalid,    SPTR_ILLEGAL,
   endCtx
};


Bool
apc_pointer_set_shape( Handle self, int sysPtrId)
{
   HPOINTER user = sys pointer2;
   if ( sysPtrId < crDefault || sysPtrId > crUser) return false;
   sys pointerId = sysPtrId;
   if ( sysPtrId == crDefault)
   {
      Handle owner = var owner;
      while( owner)
      {
         sysPtrId = dsys( owner) pointerId;
         if ( sysPtrId != crDefault) break;
         owner = (( PComponent) owner)-> owner;
      }
      if ( sysPtrId == crDefault) sysPtrId = crArrow;
      if ( sysPtrId == crUser) user = dsys( owner) pointer2;
   }
   sys pointer = ( sysPtrId == crUser) ? user :
      WinQuerySysPointer( HWND_DESKTOP,
      ctx_remap_def( sysPtrId, ctx_cr2SPTR, true, SPTR_ARROW),
      false);
   if ( var stage == csNormal)
      if ( !WinSetPointer( HWND_DESKTOP, sys pointer)) apiErrRet;
   return true;
}

Bool
apc_pointer_set_visible( Handle self, Bool visible)
{
   if ( var stage == csNormal)
   {
      guts. pointerInvisible = !visible;
      if ( !WinShowPointer( HWND_DESKTOP, visible)) apiErrRet;
      guts. pointerLock += visible ? 1 : -1;
   }
   return true;
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
   BInfo2 bi;
   HBITMAP bDup = nilHandle, bSave;
   POINTERINFO p;
   BITMAPINFOHEADER bh;
   PIcon i = ( PIcon) icon;
   POINTL pt[4] = {{0,0},{0,0},{0,0},{0,0}};
   Bool ret = true;

   if ( icon == nilHandle)
      apcErrRet( errInvParams);

   if ( WinQueryPointerInfo( sys pointer, &p) == 0) apiErrRet;
   self = icon;
   memset( &bi, 0, sizeof( bi));
   bi. structLength = sizeof( BInfo);
   bi. planes = 1;
   bi. bpp    = 1;

   // querying bitmap characterictics
   if ( p. hbmColor)
   {
      // pointer have a color bitmap
      int type;
      if ( !GpiQueryBitmapParameters( p. hbmColor, &bh)) apiErrRet;
      // creating storage
      if ( bh. cBitCount > 8) bh. cBitCount = 24;
      image_begin_query( bh. cBitCount, &type);
      bh. cBitCount = type;
      i-> self-> create_empty( icon, bh. cx, bh. cy, bh. cBitCount);
      // reading color map
      apcErrClear;
      bSave = GpiSetBitmap( guts. ps, p. hbmColor);
      if ( bSave == HBM_ERROR) apiErrRet;
      image_query( icon, guts. ps);
      if ( apcError) return false;
      // creating color duplicate
      bDup = bitmap_make_handle( icon);
      if ( apcError) return false;
      // query mono XOR map to xor the color duplicate
      if ( GpiSetBitmap( guts. ps, p. hbmPointer) == HBM_ERROR) apiErr;
      if ( GpiQueryBitmapBits( guts. ps, 0, i-> h, (PBYTE)i-> mask, ( PBITMAPINFO2)&bi) < 0) {
         apiErr;
         GpiSetBitmap( guts. ps, bSave);
         GpiDeleteBitmap( bDup);
         return false;
      }
      if ( GpiSetBitmap( guts. ps, bDup) == HBM_ERROR) apiErr;
      pt [1]. x = i-> w - 1;     // dest to
      pt [1]. y = i-> h - 1;
      pt [3]. x = i-> w;         // src to
      pt [3]. y = i-> h;
      GpiSetColor( guts. ps, CLR_WHITE);
      GpiSetBackColor( guts. ps, CLR_BLACK);
      if ( GpiDrawBits( guts. ps, i-> mask, ( PBITMAPINFO2) &bi, 4, pt, ROP_SRCINVERT, BBO_IGNORE) == GPI_ERROR) {
         apiErr;
         ret = false;
      }
      image_query( icon, guts. ps);
      if ( apcError) ret = false;
   } else {
      int type;
      // pointer does not have color bitmap
      if ( !GpiQueryBitmapParameters( p. hbmPointer, &bh)) apiErrRet;
      if (bh. cBitCount > 8) bh. cBitCount = 24;
      image_begin_query( bh. cBitCount, &type);
      bh. cBitCount = type;
      i-> self-> create_empty( icon, bh. cx, bh. cy / 2, bh. cBitCount);
      bSave = GpiSetBitmap( guts. ps, p. hbmPointer);
      if ( bSave == HBM_ERROR) apiErr;
      if ( GpiQueryBitmapBits( guts. ps, 0, i-> h, (PBYTE)i-> data, ( PBITMAPINFO2)&bi) < 0)
      {
         apiErr;
         GpiSetBitmap( guts. ps, p. hbmPointer);
         return false;
      }
   }

   // query mono AND map
   if ( GpiSetBitmap( guts. ps, p. hbmPointer) == HBM_ERROR) apiErr;
   if ( bDup) if ( !GpiDeleteBitmap( bDup)) apiErr;
   if ( GpiQueryBitmapBits( guts. ps, i-> h, i-> h, (PBYTE)i-> mask, ( PBITMAPINFO2)&bi) < 0) ret = false;
   // end up
   if ( GpiSetBitmap( guts. ps, bSave) == HBM_ERROR) apiErrRet;
   return ret;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hotSpot)
{
   if ( sys pointer2) {
      if ( !WinSetPointer( HWND_DESKTOP, NULLHANDLE)) apiErr;
      if ( !WinDestroyPointer( sys pointer2)) apiErr;
      sys pointer2 = nilHandle;
   }
   apcErrClear;
   sys pointer2 = icon ? pointer_make_handle( self, icon, hotSpot) : nilHandle;
   if ( apcError) return false;
   if ( sys pointerId == crUser)
   {
      sys pointer = sys pointer2;
      if ( var stage == csNormal)
          if ( !WinSetPointer( HWND_DESKTOP, sys pointer)) apiErrRet;
   }
   return true;
}

int
apc_pointer_get_state( Handle self)
{
   return
   (( WinGetKeyState( HWND_DESKTOP, VK_BUTTON1) & 0x8000) ? mbLeft   : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_BUTTON2) & 0x8000) ? mbRight  : 0) |
   (( WinGetKeyState( HWND_DESKTOP, VK_BUTTON3) & 0x8000) ? mbMiddle : 0);
}

// Cursor

Bool
cursor_update( Handle self)
{
   if ( !is_apt( aptFocused))
      return true;
   WinDestroyCursor( var handle);
   if ( is_apt( aptCursorVis))
   {
      if ( !WinCreateCursor( var handle,
         sys cursorPos. x, sys cursorPos. y,
         sys cursorSize. x, sys cursorSize. y,
         CURSOR_SOLID | CURSOR_FLASH, nil)) apiErrRet;
      if ( !WinShowCursor( var handle, is_apt( aptCursorVis))) apiErrRet;
   }
   return true;
}

Bool
apc_cursor_set_pos( Handle self, int x, int y)
{
   if ( !hwnd_check_limits( x, y, true)) apcErrRet( errInvParams);
   sys cursorPos = ( Point){ x, y};
   return cursor_update( self);
}

Bool
apc_cursor_set_size( Handle self, int x, int y)
{
   if ( !hwnd_check_limits( x, y, false)) apcErrRet( errInvParams);
   sys cursorSize = ( Point){ x, y};
   return cursor_update( self);
}

Bool
apc_cursor_set_visible( Handle self, Bool visible)
{
   apt_assign( aptCursorVis, visible);
   return cursor_update( self);
}

Point
apc_cursor_get_pos( Handle self)
{
   return sys cursorPos;
}

Point
apc_cursor_get_size( Handle self)
{
   return sys cursorSize;
}

Bool
apc_cursor_get_visible( Handle self)
{
   return is_apt( aptCursorVis);
}

