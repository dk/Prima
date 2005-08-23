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
 */

/* $Id$ */

#ifndef _OS2GUTS_H_
#define _OS2GUTS_H_
#include "guts.h"
#define INCL_WIN
#define INCL_DOS
#define INCL_SPL
#define INCL_GPI
#define INCL_SPLDOSPRINT
#include <os2.h>

#ifdef HAVE_PMPRINTF_H
#define __PMPRINTF__  /* OS/2 logging facility */
#include <PMPRINTF.H>
#endif

#define WM_PRIMA_CREATE                   ( WM_USER + 1)
#define WM_MENUCOMMAND                    ( WM_USER + 2)
#define WM_PRESPARAMMENU                  ( WM_USER + 3)
#define WM_POSTAL                         ( WM_USER + 4)
#define WM_MENUCOLORCHANGED               ( WM_USER + 5)
#define WM_DLGENTERMODAL                  ( WM_USER + 6)
// #define WM_DLGPASSIVATE                   ( WM_USER + 7)
// #define WM_DLGPOPUP                       ( WM_USER + 8)
#define WM_MOUSEENTER                     ( WM_USER + 10)
#define WM_MOUSELEAVE                     ( WM_USER + 11)
#define WM_FONTCHANGED                    ( WM_USER + 12)
#define WM_ACTIVATEMENU                   ( WM_USER + 13)
#define WM_VIEWHELP                       ( WM_USER + 14)
#define WM_FORCEFOCUS                     ( WM_USER + 15)
#define WM_ZORDERSYNC                     ( WM_USER + 16)
#define WM_REPAINT                        ( WM_USER + 17)
#define WM_SOCKET                         ( WM_USER + 18)
#define WM_CROAK                          ( WM_USER + 19)
#define WM_SOCKET_REHASH                  ( WM_USER + 20)
#define WM_TERMINATE                      ( WM_USER + 99)
#define WM_FIRST_USER_MESSAGE             ( WM_USER +100)
#define WM_LAST_USER_MESSAGE              ( WM_USER +900)


// OS/2 defaults for apc
#define DEFAULT_FONT_NAME                "Helv"
#define DEFAULT_FONT_SIZE                10
#define DEFAULT_FONT_FACE                "10.Helv"
#define DEFAULT_FIXED_FONT               "Courier"
#define DEFAULT_VARIABLE_FONT            "Helv"
#define DEFAULT_SYSTEM_FONT              "System VIO"
#define CTRL_ID_AUTOSTART                100
#define MENU_ID_AUTOSTART                1000
#define FONT_FONTSPECIFIC                "fontspecific"
#define csAxEvents csFrozen

#define WC_CUSTOM ((PSZ)0)
#define WC_APPLICATION ((PSZ)1)

#define USE_GPIDRAWBITS

typedef struct _ItemRegRec {
  int   cmd;
  void *item;
} ItemRegRec, *PItemRegRec;

typedef struct _SingleColor
{
   Color color;
   int   index;
} SingleColor, *PSingleColor;


typedef struct _OS2Guts
{
   HAB   anchor;             // program main ab
   HMQ   queue;              // message queue
   PID   pid;                // process ID
   HPS   ps;                 // presentation space for runtime needs
   SIZEF defFontBox;         // default font box
   int*  bmf;                // bitmap memory formats list
   int   bmfCount;           // count of bmf entries
   Point displayResolution;  // display resolution ( PPI)
   Font  sysDefFont;         // system default font ( usually 10.System Proportional)
   Bool  monoBitsOk;         // false if mono bitmaps inverse output should be performed
   List  transp;             // transparent controls list
   List  winPsList;          // list of cached presentation spaces
   List  psList;             // list of presentation spaces got thru WinBeginPaint
   void *fontHash;           // 2nd level font hash (for guts. ps)
   int   fontId;
   int   appLock;            // application lock count
   int   pointerLock;        // pointer lock count
   Bool  pointerInvisible;   // pointer visibility state;
   ULONG codePage;           // current system codepage
   Bool  focSysDisabled;     // focus system disabled
   Bool  focSysGranted;      // WinSetFocus() was called inside apc_widget_set_focused
   Bool  appTypePM;          // startup check, whether our application is PM
   List  eventHooks;         // event hook list
   Byte  msgMask[100];       // 800 user-defined messages allowed
   int   socketThread;       // the socket thread ID
   HMTX  socketMutex;        // mutex for the socket thread
   Bool  socketPostSync;     // post-message flag for the socket thread
   List  files;              // files, participating in select()
} OS2Guts;

extern OS2Guts guts;
extern int ctx_kb2VK[];
extern ERRORID rc;
extern int ctx_cr2SPTR[];
extern Bool   appDead;
extern Handle lastMouseOver;

#if PRIMA_DEBUG
#define apiErr {                                            \
   rc = WinGetLastError( guts. anchor);                     \
   apcError = errApcError;                                  \
   printf("OS2_%03lx at line %d, file %s\n", rc, __LINE__, __FILE__); \
}
#define apcErr( err) {                                      \
   apcError = err;                                          \
   printf("APC_%03x at line %d, file %s\n", err, __LINE__, __FILE__);\
}
#define apiAltErr( err) {                                   \
   apcError = errApcError;                                  \
   rc = err;                                                \
   printf("OS2_%03x at line %d, file %s\n", (int)rc, __LINE__, __FILE__); \
}
#else
#define apiErr       { rc = WinGetLastError( guts. anchor);    apcError = errApcError; }
#define apcErr( err)    apcError = err;
#define apiAltErr( err) { apcError = errApcError; rc = err; }
#endif

#define apiErrRet         { apiErr;               return (Bool)false; }
#define apiErrCheckRet    { apiErrCheck; if ( rc) return (Bool)false; }
#define apcErrRet(err)    { apcErr(err);          return (Bool)false; }
#define apcErrClear       { apcError = errOk;                   }

#define objCheck          if ( var stage == csDead) return
#define dobjCheck(handle) if ((( PObject)handle)-> stage == csDead) return

#define aptWM_PAINT             0x00000001       // true if inside WM_PAINT
#define aptWinPS                0x00000002       // window PS was passed to paint
#define aptCompatiblePS         0x00000004       // PS is screen-compatible
#define aptCursorVis            0x00000010       // cursor visible flag
#define aptFocused              0x00000040       // set if control if focused
#define aptFirstClick           0x00000080       // set if control can process WM_BUTTONXDOWN without pre-activation
#define aptClipOwner            0x00000100       // if set, parent of this window is HWND_DESKTOP
#define aptLockVisState         0x00000200       // visible/locked flag
#define aptTransparent          0x00000400       // transparency flag
#define aptSyncPaint            0x00000800       // WS_SYNCPAINT analog
#define aptVisible              0x00001000       // visibility flag
#define aptTaskList             0x00002000       // Window flag - set if in task list
#define aptDeviceBitmap         0x00004000       // == kind_of( CDeviceBitmap)
#define aptExtraFont            0x00008000       // extra font styles ( angle, shear) has been applied
#define aptEnabled              0x00010000       // set if view is enabled
#define aptTextOpaque           0x00020000       // set if text_out is opaque
#define aptTextOutBaseline      0x00040000       // set if text_out y ref.point is baseline

#define apt_set( option)           (sys options |= option)
#define apt_clear( option)         (sys options &= ~option)
#define is_apt( option)            ((sys options & option)?1:0)
#define apt_assign( option, value) ((value)?apt_set(option):apt_clear(option))

#define cbNoBitmap     0
#define cbScreen       1
#define cbMonoScreen   2
#define cbImage        3

#define psDot         "\3\3"
#define psDash        "\x16\6"
#define psDashDot     "\x9\6\3\6"
#define psDashDotDot  "\x9\3\3\3\3\3"


#define GRAD 572.9577951

typedef struct _WindowData
{
   int   borderIcons;
   int   borderStyle;
   Point hiddenPos;
   Point hiddenSize;
   int   state;

   int    modalResult;
   Point  lastClientSize;
   Point  lastFrameSize;
   Handle oldFoc;
   HWND   oldActive;

} WindowData;

typedef struct _TimerData
{
   int  timeout;
} TimerData;

typedef struct _PrinterData
{
   PRQINFO3 ppi;
   SIZEL    size;
   char *   defaultPrn;
} PrinterData;

typedef struct _PaintSaveData
{
   Color       lbs[2];
   Bool        fillWinding;
   int         lineWidth;
   int         lineEnd;
   int         lineJoin;
   unsigned char *linePattern;
   int         linePatternLen;
   FillPattern fillPattern;
   int         rop;
   int         rop2;
   Point       transform;
   Font        font;
   Bool        textOpaque;
   Bool        textOutB;
} PaintSaveData, *PPaintSaveData;

typedef struct _DrawableData
{
   HPS             ps;
   HPS             ps2;
   unsigned long   options;            // aptXXX options
   HBITMAP         bm;                      // cached bitmap
   char         *  bmRaw;                   // cached raw bitmap
   PBITMAPINFO2    bmInfo;              // raw bitmap info
   Font            font;                    // cached font metric
   void         *  fontHash;
   int             fontId;
   HDC             dc;                      // cached HDC
   HDC             dc2;                     // cached HDC
   int             bpp;
// HPS data
   Color           lbs[2];
   Bool            fillWinding;
   int             lineWidth;
   int             lineEnd;
   int             lineJoin;
   unsigned char * linePattern;
   int             linePatternLen;
   FillPattern     fillPattern;
   FillPattern     fillPattern2;
   int             rop;
   int             rop2;
   Point           transform;
   Point           transform2;
   Point           res;
   HBITMAP         fillBitmap;
// HWND data
   ApiHandle       handle2;                 // auxillary handler
   HWND            parent;                  // window parent
   HWND            owner;                   // window owner
   HWND            parentHandle;
   Point           cursorPos;               // cursor position
   Point           cursorSize;              // cursor size
   HPOINTER        pointer;                 // pointer data
   HPOINTER        pointer2;                // user pointer data
   int             pointerId;               // pointer id
   PSZ             className;               // WC_XXXXXXXXXX
   int             timeDefsCount;           // count of timers attached.
   PItemRegRec     timeDefs;                // timer list
   Color           l3dc;                    // light 3d color
   Color           d3dc;                    // dark  3d color
   PPaintSaveData  psd;
   void  *         recreateData;            // ViewProfile custom area
   int             sizeLockLevel;           // redirector flag for var-> virtualSize
   union {
     TimerData     timer;
     WindowData    window;
     PrinterData   prn;
     int           file;
     HRGN          imgCachedRegion;      // Image specific field
   } s;
} DrawableData, *PDrawableData;

typedef struct _MenuWndData
{
   PFNWP      fnwp;
   Handle     menu;
   int        id;
} MenuWndData, *PMenuWndData;

typedef struct _ParentHandleRec
{
   HWND  hwnd;
   int   refcnt;
   PFNWP proc;
} ParentHandleRec, *PParentHandleRec;


typedef struct _BInfo
{
   unsigned long structLength;
   unsigned long w;
   unsigned long h;
   unsigned short planes;  // always 1
   unsigned short bpp;
   RGB2 palette[ 0];
} BInfo, *PBInfo;

typedef struct _BInfo2
{
   unsigned long structLength;
   unsigned long w;
   unsigned long h;
   unsigned short planes;  // always 1
   unsigned short bpp;
   RGB2 palette[ 0x100];
} BInfo2, *PBInfo2;


MRESULT EXPENTRY generic_view_handler     ( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY generic_frame_handler    ( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY generic_menu_handler     ( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);

extern PFont gp_get_font   (  HPS ps, PFont font, Point resolution);
extern PFont view_get_font ( Handle view, PFont font);
extern Point frame2client( Handle self, Point p, Bool f2c);

extern Handle       cmono_enscreen( Handle image);
extern Bool         cursor_update( Handle self);
extern void         bitmap_make_cache( Handle from, Handle self);
extern HBITMAP      bitmap_make_ps( Handle img, HPS * hps, HDC * hdc, PBITMAPINFO2 * bm, int createBitmap);
extern Handle       bitmap_make_handle( Handle img);
extern Handle       enscreen( Handle image);
extern PBITMAPINFO2 get_binfo( Handle self);
extern PBITMAPINFO2 get_screen_binfo( void);
extern Bool         hwnd_check_limits( int x, int y, Bool uint);
extern void         hwnd_enter_paint( Handle self);
extern Handle       hwnd_frame_top_level( Handle self);
extern void         hwnd_leave_paint( Handle self);
extern Handle       hwnd_to_view( HWND win);
extern Handle       hwnd_top_level( Handle self);
extern Bool         image_begin_query( int primType, int * typeToConvert);
extern void         image_query( Handle self, HPS ps);
extern void         image_set_cache( Handle from, Handle self);
extern HPOINTER     pointer_make_handle( Handle self, Handle icon, Point hotSpot);
extern FIXED        float2fixed( double f);
extern double       fixed2float( FIXED f);
extern int          font_font2gp( PFont font, Point res, Bool forceSize);
extern void         font_pp2gp( char * ppFontNameSize, PFont font);
extern void         font_gp2pp( PFont font, char * buf);
extern int          font_style( PFONTMETRICS fm);
extern void         font_fontmetrics2font( PFONTMETRICS m, PFont f, Bool readonly);
extern long         remap_color( HPS ps, long clr, Bool toSystem);
extern Bool         screenable( Handle image);
extern HRGN         region_create( Handle self, Handle mask);


extern Bool create_font_hash          ( void);
extern Bool destroy_font_hash         ( void);
extern Bool add_font_to_hash          ( const PFont key, const PFont font, int vectored, PFONTMETRICS fm, Bool addSizeEntry);
extern Bool get_font_from_hash        ( PFont font, int *vectored, PFONTMETRICS fm, Bool bySize);
extern void *create_fontid_hash       ( void);
extern void destroy_fontid_hash       ( void *hash);
extern int get_fontid_from_hash       ( void *hash, const PFont font, SIZEF *sz, int *vectored);
extern void add_fontid_to_hash        ( void *hash, int id, const PFont font, const SIZEF *sz, int vectored);

extern USHORT font_enc2cp( const char * encoding);
extern char * font_cp2enc( USHORT codepage);


extern void socket_rehash ( void);


#endif
