#ifndef _WIN32_H_
#define _WIN32_H_
#define _WIN32_WINNT 0x400
#include <windows.h>
#include <winspool.h>
#include "apricot.h"

#define SEVERE_DEBUG

#define IS_NT      (BOOL)( guts. version < 0x80000000)
#define IS_WIN32S  (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(guts. version))<4))
#define IS_WIN95   (BOOL)(!(IS_NT) && !(IS_WIN32S))
#define IS_WIN98   (BOOL)( guts. is98)

#ifdef UNICODE
#error This version of apc_Win32 does not support Unicode
#endif

#ifndef CYGWIN32
#define O_SYNC                      0
#elif defined( _POSIX_SOURCE)
#define O_SYNC                      _FSYNC
#endif

#undef  HWND_DESKTOP
#define HWND_DESKTOP         GetDesktopWindow()


#define DEFAULT_SYSTEM_FONT              "System"
#define DEFAULT_WIDGET_FONT              "MS Shell Dlg"
#define DEFAULT_WIDGET_FONT_SIZE         8
#define COLOR_TOLERANCE                  4
#define HASMATE_MAGIC                    0xDEAF0CE1
#define MENU_ID_AUTOSTART                100
#define TID_USERMAX                      32767
#define REG_STORAGE                      "SOFTWARE\\Perl\\Prima"
#define MAXREGLEN                        1024


#define WM_WRITE_TO_LOG                   ( WM_USER + 0)
#define WM_PRIMA_CREATE                   ( WM_USER + 1)
#define WM_POSTAL                         ( WM_USER + 2)
#define WM_DLGENTERMODAL                  ( WM_USER + 3)
#define WM_ZORDERSYNC                     ( WM_USER + 4)
#define WM_BREAKMSGLOOP                   ( WM_USER + 5)
#define WM_MOUSEENTER                     ( WM_USER + 6)
#define WM_MOUSEEXIT                      ( WM_USER + 7)
#define WM_SETVISIBLE                     ( WM_USER + 8)
#define WM_KEYPACKET                      ( WM_USER + 9)
#define WM_LMOUSECLICK                    ( WM_USER + 10)
#define WM_MMOUSECLICK                    ( WM_USER + 11)
#define WM_RMOUSECLICK                    ( WM_USER + 12)
#define WM_FORCEFOCUS                     ( WM_USER + 13)
#define WM_SYNCMOVE                       ( WM_USER + 14)
#define WM_EXTERNAL                       ( WM_USER + 15)
#define WM_HASMATE                        ( WM_USER + 16)
#define WM_TERMINATE                      ( WM_USER + 99)

#define WC_CUSTOM       0
#define WC_DLG          1
#define WC_APPLICATION  2
#define WC_FRAME        3
#define WC_MENU         4
#define WC_POPUP        5

#define exsLinePattern  1
#define exsLineEnd      2

#define stbPen          1
#define stbBrush        2
#define stbText         4
#define stbBacking      8

/*  #ifndef  SEVERE_DEBUG */
/*  #define apiErr       { rc = GetLastError();    apcError = errApcError; } */
/*  #define apcErr( err)    apcError = err; */
/*  #define apiAltErr( err) { apcError = errApcError; rc = err; } */
/*  #else */
#define apiErr {                                            \
   rc = GetLastError();                                     \
   apcError = errApcError;                                  \
   fprintf( stderr, "WIN_%d (%s) at line %d in %s", rc,     \
      err_msg( rc), __LINE__, __FILE__);                    \
}
#define apcErr( err) {                                      \
   apcError = err;                                          \
}
#define apiAltErr( err) {                                   \
   apcError = errApcError;                                  \
   rc = err;                                                \
   fprintf( stderr, "WIN_%d (%s) at line %d at %s", rc,     \
        err_msg( rc), __LINE__, __FILE__);                  \
}
/*  #endif */
#define apiErrRet         { apiErr;               return false; }
#define apiErrCheckRet    { apiErrCheck; if ( rc) return false; }
#define apcErrRet(err)    { apcErr(err);          return false; }
#define apcErrClear       { apcError = errOk;                   }

#define objCheck          if ( var stage == csDead) return
#define dobjCheck(handle) if ((( PObject)handle)-> stage == csDead) return

typedef struct _HandleOptions_ {
   unsigned aptWM_PAINT             : 1;       // true if inside WM_PAINT
   unsigned aptWinPS                : 1;       // window PS was passed to paint
   unsigned aptCompatiblePS         : 1;       // PS is screen-compatible
   unsigned aptFontExists           : 1;       // font is selected on HPS
   unsigned aptCursorVis            : 1;       // cursor visible flag
   unsigned aptPointerVis           : 1;       // pointer visibility flag
   unsigned aptFocused              : 1;       // set if control if focused
   unsigned aptFirstClick           : 1;       // set if control can process WM_BUTTONXDOWN without pre-activation
   unsigned aptClipOwner            : 1;       // if set, parent of this window is HWND_DESKTOP
   unsigned aptLockVisState         : 1;       // visible/locked flag
   unsigned aptTransparent          : 1;       // transparency flag
   unsigned aptSyncPaint            : 1;       // WS_SYNCPAINT analog
   unsigned aptVisible              : 1;       // visibility flag
   unsigned aptTaskList             : 1;       // Window flag - set if in task list
   unsigned aptDeviceBitmap         : 1;       // == kind_of( CDeviceBitmap)
   unsigned aptBitmap               : 1;       // == kind_of( CImage)
   unsigned aptExtraFont            : 1;       // extra font styles ( angle, shear) has been applied
   unsigned aptDCChangeLock         : 1;       // locks SelectObject() calls
   unsigned aptEnabled              : 1;       // enabled flag
   unsigned aptTextOpaque           : 1;       // gp text drawing flag
   unsigned aptTextOutBaseline      : 1;       // gp text drawing flag
} HandleOptions;

typedef struct _WinGuts
{
    HINSTANCE      instance;           // application instance
    int            cmdShow;            // run command state
    int            appLock;            // application lock count
    int            pointerLock;        // pointer lock count
    HANDLE         ioThread;           // handle of input-output thread
    DWORD          mainThreadId;       // Id of main thread
    Point          displayResolution;  // screen resolution in ppi
    char           defaultFixedFont    [ 256];
    char           defaultVariableFont [ 256];
    char           defaultSystemFont   [ 256];
    Font           windowFont;         // window default font
    Font           menuFont;           // menu default font
    Font           msgFont;            // message default font
    Font           capFont;            // caption default font
    BITMAPINFO     displayBMInfo;      // display bpp & size
    Bool           insertMode;         // fake insert mode
    Point          iconSizeLarge;
    Point          iconSizeSmall;
    Point          pointerSize;
    BYTE           keyState[ 256];     // application key buffer state
    BYTE          *currentKeyState;    // current virtual key buffer state
    HKL            keyLayout;          // key layout, most likely latin for Ctrl+Key mapping
    NONCLIENTMETRICS ncmData;          // windows system data
    List           transp;             // transparent controls list
    int            topWindows;         // count of top-level windows in app
    Bool           focSysDisabled;     // focus system disabled
    Bool           focSysGranted;      // SetFocus() was called inside apc_widget_set_focused
    UINT           errorMode;          // SetErrorMode() result
    DWORD          version;            // GetVersion() cached result
    Point          smDblClk;           // cached SM_CxDOUBLECLK values
    HANDLE         objects[ MAXIMUM_WAIT_OBJECTS]; // handle objects dynamic list
    List           files;              // List of active File objects
    Bool           is98;               // is win98
} WinGuts, *PWinGuts;

typedef struct _WindowData
{
   int   borderIcons;
   int   borderStyle;
   Point hiddenPos;
   Point hiddenSize;
   int   state;
} WindowData;

typedef struct _TimerData
{
   int  timeout;
} TimerData;

typedef struct _PrinterData
{
   PRINTER_INFO_2 ppi;
   char           defPrnBuf[ 256];
   char          *device;
   char          *driver;
   char          *port;
} PrinterData;

typedef struct _PaintSaveData
{
   Color       lbs[2];
   int         lineWidth;
   int         lineEnd;
   unsigned char * linePattern;
   int         linePatternLen;
   FillPattern fillPattern;
   int         rop;
   int         rop2;
   Point       transform;
   Font        font;
   Bool        textOpaque;
   Bool        textOutB;
} PaintSaveData, *PPaintSaveData;

typedef struct _PatResource
{
   DWORD  dotsCount;
   DWORD* dotsPtr;
   DWORD  dots[ 1];
} PatResource, *PPatResource;

typedef struct _EXTPEN
{
   Bool            actual;
   DWORD           style;
   DWORD           lineEnd;
   PatResource  *  patResource;
} EXTPEN, *PEXTPEN;

typedef struct _EXTLOGBRUSH
{
   LOGBRUSH     lb;
   COLORREF     backColor;
   FillPattern  pattern;
} EXTLOGBRUSH, *PEXTLOGBRUSH;

typedef struct _DIBMONOBRUSH
{
   BITMAPINFOHEADER bmiHeader;
   RGBQUAD          bmiColors[2];
   unsigned char    bmiData[32];
} DIBMONOBRUSH, *PDIBMONOBRUSH;

typedef struct _Stylus
{
   LOGPEN       pen;
   EXTLOGBRUSH  brush;
   EXTPEN       extPen;
} Stylus, *PStylus;

typedef struct _DCStylus
{
   Stylus        s;
   int           refcnt;
   HPEN          hpen;
   HBRUSH        hbrush;
} DCStylus, *PDCStylus;

typedef struct _DCFont
{
   Font          font;
   int           refcnt;
   HFONT         hfont;
} DCFont, *PDCFont;


typedef struct _XLOGPALETTE {
   WORD         palVersion;
   WORD         palNumEntries;
   PALETTEENTRY palPalEntry[ 256];
} XLOGPALETTE, *PXLOGPALETTE;

typedef struct _XBITMAPINFO {
   BITMAPINFOHEADER bmiHeader;
   RGBQUAD          bmiColors[ 256];
} XBITMAPINFO, *PXBITMAPINFO;

typedef struct _ItemRegRec {
  int   cmd;
  void *item;
} ItemRegRec, *PItemRegRec;

typedef struct _DrawableData
{
   /* Drawable basic data*/
   HDC            ps;                      // general HDC
   PAINTSTRUCT    paintStruc;              // HDC counterpart
   HBITMAP        bm;                      // cached bitmap
   HPALETTE       pal;                     // cached palette

   /* stylus and font hash management fields */
   PDCStylus      stylusResource;          // current stylus pointer
   int            stylusFlags;             // stylus resource cache( stbXXXX)
   Stylus         stylus;                  // widgets stylus record
   PDCFont        fontResource;            // font resource pointer

   /* Stock objects of HDC - to be restored after paint mode */
   HPEN           stockPen;
   HBRUSH         stockBrush;
   HFONT          stockFont;
   HBITMAP        stockBM;
   HPALETTE       stockPalette;

   /* HDC info fields */
   int            bpp;                     // bits per pixel
   Point          res;                     // resolution

   /* cached gp_GetCharABCWidthsFloat results */
   BYTE           tmPitchAndFamily;
   LONG           tmOverhang;

   /* HDC attributes storage outside paint mode */
   Color          lbs[2];
   int            lineWidth;
   int            lineEnd;
   unsigned char *linePattern;
   int            linePatternLen;
   unsigned char *linePattern2;
   int            linePatternLen2;
   FillPattern    fillPattern;
   FillPattern    fillPattern2;
   int            rop;
   int            rop2;
   Point          transform;
   PPaintSaveData psd;                     // Their values durind paint saved in sys psd

   /* Basic widget fields */
   HWND           handle;                  // Windows handle of a widget
   HWND           owner;                   // Windows owner of a widget
   HWND           parent;                  // Windows parent of a widget
   int            className;               // class name ( WC_XXX)

   /* Widget properties */
   HandleOptions  options;                 // apt_XXX settings
   ColorSet       viewColors;              // widget color palette
   PXLOGPALETTE   p256;                    // cached squeezed palette
   void *         recreateData;            // ViewProfile custom area

   /* Custom data for widget paint in optBuffered state */
   HDC            ps2;                     // original HDC
   HPALETTE       pal2;                    // original palette
   Point          transform2;              // necessary additional transposition

   /* Positioning support fields */
   Point          lastSize;                // last actual size
   int            sizeLockLevel;           // size locking flag
   int            yOverride;               // special cached height value. Used in WM_SIZE<->WM_MOVE interactions

   /* Widget attributes - timers, cursor, pointers, menu, shape */
   int            timeDefsCount;           // count of timers attached.
   PItemRegRec    timeDefs;                // timer list
   Point          cursorPos;               // cursor position
   Point          cursorSize;              // cursor size
   HCURSOR        pointer;                 // pointer handle
   HCURSOR        pointer2;                // user pointer data
   int            pointerId;               // pointer id
   Handle         lastMenu;                // last menu activated by WM_INITMENU or WM_INITMENUPOPUP
   Point          extraBounds;             // used in region calculations
   Point          extraPos;                // used in region calculations

   /* Other class-specific data */
   union {
      TimerData     timer;
      WindowData    window;
      PrinterData   prn;
      HANDLE        fileObject;
      HRGN          imgCachedRegion;      // Image specific field
   } s;
} DrawableData, *PDrawableData;

typedef struct _MenuWndData
{
   Handle     menu;
   int        id;
} MenuWndData, *PMenuWndData;

typedef struct _KeyPacket
{
   HWND     wnd;
   UINT     msg;
   WPARAM   mp1;
   LPARAM   mp2;
   int      mod;
} KeyPacket, *PKeyPacket;

typedef struct _MusClkRec {
   Bool    pending;
   UINT    emsg;
   MSG     msg;
} MusClkRec;

#define STYLUS_USE_PEN( __ps)                              \
   if ( !( sys stylusFlags & stbPen)) {                    \
      if ( __ps)                                           \
         SelectObject( __ps, sys stylusResource-> hpen);   \
      sys stylusFlags |= stbPen;                           \
   }

#define STYLUS_USE_BRUSH( __ps)                            \
   if ( !( sys stylusFlags & stbBrush)) {                  \
      if ( __ps)                                           \
         SelectObject( __ps, sys stylusResource-> hbrush); \
      sys stylusFlags |= stbBrush;                         \
   }

#define STYLUS_USE_TEXT( __ps)                             \
   if ( !( sys stylusFlags & stbText)) {                   \
      if ( __ps)                                           \
         SetTextColor( __ps, sys stylus. pen. lopnColor);  \
      sys stylusFlags |= stbText;                          \
   }

#define STYLUS_USE_BACKING( __ps)                          \
   if ( !( sys stylusFlags & stbBacking)) {                \
      if ( __ps)                                           \
         SetBkColor( __ps, sys stylus. brush. backColor);  \
      sys stylusFlags |= stbBacking;                       \
   }

#define psDot         "\3\3"
#define psDash        "\x16\6"
#define psDashDot     "\x9\6\3\6"
#define psDashDotDot  "\x9\3\3\3\3\3"

#define csAxEvents csFrozen

#define apt_set( option)           ( sys options. option = 1)
#define apt_clear( option)         ( sys options. option = 0)
#define is_apt( option)            ( sys options. option)
#define apt_assign( option, value) ( sys options. option = (value)?1:0)

#define is_declipped( handle)      (                                                    \
   handle && ( dsys(handle) className != WC_FRAME ) &&                                  \
   ( !dsys(handle)options.aptClipOwner || ((( PWidget)handle)-> owner == application))  \
)

#define is_declipped_child( handle) (                                                   \
   handle && ( dsys(handle) className != WC_FRAME ) &&                                  \
   !dsys(handle)options.aptClipOwner                                                    \
)

#define palette_create image_make_bitmap_palette

extern Bool         appDead;
extern DIBMONOBRUSH bmiHatch;
extern PHash        fontMan;
extern int          FONTSTRUCSIZE;
extern int          FONTSTRUCSIZE2;
extern WinGuts      guts;
extern PHash        imageMan;
extern PHash        menuMan;
extern MusClkRec    musClk;
extern PHash        patMan;
extern DWORD        rc;
extern PHash        stylusMan;
extern HBRUSH       hBrushHollow;
extern PatResource  hPatHollow;
extern HPEN         hPenHollow;
extern PHash        regnodeMan;
extern Handle       lastMouseOver;

LRESULT CALLBACK    generic_app_handler      ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    generic_frame_handler    ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    generic_view_handler     ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);

extern int          arc_completion( double * angleStart, double * angleEnd, int * needFigure);
extern Bool         add_font_to_hash( const PFont key, const PFont font, int vectored, Bool addSizeEntry);
extern void         adjust_line_end( int  x1, int  y1, int * x2, int * y2, Bool forth);
extern void         cm_squeeze_palette( PRGBColor source, int srcColors, PRGBColor dest, int destColors);
extern Bool         create_font_hash( void);
extern Bool         cursor_update( Handle self);
extern HDC          dc_alloc( void);
extern void         dc_free( void);
extern HDC          dc_compat_alloc( HDC compatDC);
extern void         dc_compat_free( void);
extern void         dbm_recreate( Handle self);
extern Bool         destroy_font_hash( void);
extern char *       err_msg( DWORD errId);
extern Bool         erratic_line( Handle self);
extern PDCFont      font_alloc( Font * data, Point * resolution);
extern void         font_change( Handle self, Font * font);
extern void         font_clean( void);
extern void         font_font2logfont( Font * font, LOGFONT * lf);
extern void         font_free( PDCFont res, Bool permanent);
extern void         font_logfont2font( LOGFONT * lf, Font * font, Point * resolution);
extern void         font_pp2font( char * presParam, Font * font);
extern void         font_textmetric2font( TEXTMETRIC * tm, Font * fm, Bool readOnly);
extern Bool         get_font_from_hash( PFont font, int *vectored, Bool bySize);
extern Point        get_window_borders( int borderStyle);
extern int          gp_arc( Handle self, int x, int y, int radX, int radY, double angleStart, double angleEnd, int drawState);
extern int          gp_line( Handle self, int x1, int y1, int x2, int y2, int draw);
extern Bool         hwnd_check_limits( int x, int y, Bool uint);
extern void         hwnd_enter_paint( Handle self);
extern Handle       hwnd_frame_top_level( Handle self);
extern void         hwnd_leave_paint( Handle self);
extern Handle       hwnd_to_view( HWND win);
extern Handle       hwnd_top_level( Handle self);
extern void         image_destroy_cache( Handle self);
extern Handle       image_enscreen( Handle image, Handle screen);
extern BITMAPINFO * image_get_binfo( Handle img, XBITMAPINFO * bi);
extern HBITMAP      image_make_bitmap_handle( Handle img, HPALETTE palette);
extern HPALETTE     image_make_bitmap_palette( Handle img);
extern HICON        image_make_icon_handle( Handle img, Point size, Point * hotSpot, Bool forPointer);
extern void         image_query_bits( Handle self, Bool forceNewImage);
extern Bool         image_screenable( Handle image, Handle screen, int * bitCount);
extern void         image_set_cache( Handle from, Handle self);
extern void         mod_free( BYTE * modState);
extern BYTE *       mod_select( int mod);
extern Bool         palette_change( Handle self);
extern long         palette_match( Handle self, long color);
extern int          palette_match_color( XLOGPALETTE * lp, long clr, int * diffFactor);
extern PPatResource patres_fetch( unsigned char * pattern, int len);
extern UINT         patres_user( unsigned char * pattern, int len);
extern void         process_transparents( Handle self);
extern long         remap_color( long clr, Bool toSystem);
extern PDCStylus    stylus_alloc( PStylus data);
extern void         stylus_change( Handle self);
extern void         stylus_clean( void);
extern Bool         stylus_complex( PStylus stylus, HDC dc);
extern Bool         stylus_extpenned( PStylus stylus, int excludeFlags);
extern void         stylus_free( PDCStylus res, Bool permanent);
extern DWORD        stylus_get_extpen_style( PStylus s);
extern HRGN         region_create( Handle mask);

#endif

