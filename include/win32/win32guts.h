#ifndef _WIN32_H_
#define _WIN32_H_
#include <windows.h>
#include <winspool.h>
#define Rect xxRect
#define Color xxColor
#define Point xxPoint
#include <gdiplus/gdiplus.h>
#undef Rect
#undef Color
#undef Point
#include "apricot.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SEVERE_DEBUG
typedef HANDLE WINHANDLE;

#ifdef __CYGWIN__
typedef int SOCKETHANDLE;
#else
typedef HANDLE SOCKETHANDLE;
#endif

#undef  HWND_DESKTOP
#define HWND_DESKTOP         guts. desktopWindow


#ifdef UNICODE
#error This version of apc_Win32 does not support Unicode
#endif

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
#define WM_MOUSEENTER                     ( WM_USER + 6)
#define WM_MOUSEEXIT                      ( WM_USER + 7)
#define WM_SETVISIBLE                     ( WM_USER + 8)
#define WM_KEYPACKET                      ( WM_USER + 9)
#define WM_LMOUSECLICK                    ( WM_USER + 10)
#define WM_MMOUSECLICK                    ( WM_USER + 11)
#define WM_RMOUSECLICK                    ( WM_USER + 12)
#define WM_FORCEFOCUS                     ( WM_USER + 13)
#define WM_SYNCMOVE                       ( WM_USER + 14)
#define WM_SOCKET                         ( WM_USER + 15)
#define WM_SOCKET_REHASH                  ( WM_USER + 16)
#define WM_EXTERNAL                       ( WM_USER + 17)
#define WM_HASMATE                        ( WM_USER + 18)
#define WM_FILE                           ( WM_USER + 19)
#define WM_CROAK                          ( WM_USER + 20)
#define WM_REPAINT_LAYERED                ( WM_USER + 21)
#define WM_DRAG_RESPONSE                  ( WM_USER + 22)
#define WM_TERMINATE                      ( WM_USER + 99)
#define WM_FIRST_USER_MESSAGE             ( WM_USER +100)
#define WM_LAST_USER_MESSAGE              ( WM_USER +900)

#ifndef WM_DPICHANGED
#define WM_DPICHANGED                     0x02E0
#endif

#define WC_CUSTOM       0
#define WC_DLG          1
#define WC_APPLICATION  2
#define WC_FRAME        3
#define WC_MENU         4
#define WC_POPUP        5

#define exsLinePattern  1
#define exsLineEnd      2
#define exsLineJoin     4

#define stbPen          0x01
#define stbBrush        0x02
#define stbText         0x04
#define stbBacking      0x08
#define stbGDIMask      0x0F
#define stbGPPen        0x10
#define stbGPBrush      0x20

#define SOCKETS_NONE         ( guts. socket_version == -1)
#define SOCKETS_AS_HANDLES   ( guts. socket_version == 1)
#define SOCKETS_NATIVE       ( guts. socket_version == 2)

#define FHT_SOCKET  1
#define FHT_PIPE    2
#define FHT_OTHER   3

#define apcWarn \
	if (debug) \
		warn( "win32 error 0x%x: '%s' at line %d in %s\n", (unsigned int)rc, \
			err_msg( rc, nil), __LINE__, __FILE__);   \
	else \
		err_msg( rc, nil)

#define apcErr( err) apcError = err
#define apiErr {           \
	rc = GetLastError();    \
	apcError = errApcError; \
	apcWarn;                \
}
#define apiAltErr( err) {  \
	apcError = errApcError; \
	rc = err;               \
	apcWarn;                \
}
#define apiErrRet         { apiErr;               return false; }
#define apcErrRet(err)    { apcErr(err);          return false; }
#define apcErrClear       { apcError = errOk;                   }

#define GPCALL rc = (DWORD)
#define apiGPErr { \
	apcError = errApcError; \
	if ( debug ) err_msg_gplus(rc, NULL);\
	rc |= 0x40000;    \
}
#define apiGPErrCheck if (rc) apiGPErr;
#define apiGPErrCheckRet(f) if (rc) { apiGPErr; return f; }
#define apiHErr(hr) {           \
	apcError = errApcError; \
	rc = hr;                \
	apcWarn;                \
}

#define apiHErrRet(hr)     { apiHErr(hr);           return false; }

#define objCheck          if ( var stage == csDead) return
#define dobjCheck(handle) if ((( PObject)handle)-> stage == csDead) return

typedef struct _HandleOptions_ {
	unsigned aptWM_PAINT             : 1;       // true if inside WM_PAINT
	unsigned aptWinPS                : 1;       // window PS was passed to paint
	unsigned aptCompatiblePS         : 1;       // PS is screen-compatible
	unsigned aptFontExists           : 1;       // font is selected on HPS
	unsigned aptCursorVis            : 1;       // cursor visible flag
	unsigned aptFocused              : 1;       // set if control if focused
	unsigned aptFirstClick           : 1;       // set if control can process WM_BUTTONXDOWN without pre-activation
	unsigned aptClipOwner            : 1;       // if set, parent of this window is HWND_DESKTOP
	unsigned aptLockVisState         : 1;       // visible/locked flag
	unsigned aptTransparent          : 1;       // transparency flag
	unsigned aptSyncPaint            : 1;       // WS_SYNCPAINT analog
	unsigned aptVisible              : 1;       // visibility flag
	unsigned aptTaskList             : 1;       // Window flag - set if in task list
	unsigned aptDeviceBitmap         : 1;       // == kind_of( CDeviceBitmap)
	unsigned aptBitmap               : 1;       // buffered widget
	unsigned aptImage                : 1;       // == kind_of( CImage)
	unsigned aptIcon                 : 1;       // == kind_of( CIcon)
	unsigned aptPrinter              : 1;       // == kind_of( CPrinter)
	unsigned aptExtraFont            : 1;       // extra font styles ( angle, shear) has been applied
	unsigned aptDCChangeLock         : 1;       // locks SelectObject() calls
	unsigned aptEnabled              : 1;       // enabled flag
	unsigned aptTextOpaque           : 1;       // gp text drawing flag
	unsigned aptTextOutBaseline      : 1;       // gp text drawing flag
	unsigned aptWinPosDetermined     : 1;       // 0 when size is set, but position is not
	unsigned aptOnTop                : 1;       // HWND_TOPMOST is set
	unsigned aptLayered              : 1;       // WS_EX_LAYERED
	unsigned aptRepaintPending       : 1;       // for optLayered
	unsigned aptMovePending          : 1;       // for optLayered
	unsigned aptLayeredPaint         : 1;       // painting children of layered window
	unsigned aptLayeredRequested     : 1;       // Prima wants layered
	unsigned aptClipByChildren       : 1;       // cached clipping by children
	unsigned aptIgnoreSizeMessages   : 1;       // during window recreation
} HandleOptions;

#define CLIPBOARD_MAIN 0
#define CLIPBOARD_DND  1

typedef struct _WinGuts
{
	HINSTANCE      instance;           // application instance
	int            cmdShow;            // run command state
	int            appLock;            // application lock count
	int            pointerLock;        // pointer lock count
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
	HWND           desktopWindow;      // GetDesktopWindow() result
	Bool           insertMode;         // fake insert mode
	Point          iconSizeLarge;
	Point          iconSizeSmall;
	Point          pointerSize;
	BYTE           keyState[ 256];     // application key buffer state
	BYTE           emptyKeyState[ 256];// just zeros
	BYTE          *currentKeyState;    // current virtual key buffer state
	HKL            keyLayout;          // key layout, most likely latin for Ctrl+Key mapping
	NONCLIENTMETRICSW ncmData;         // windows system data
	List           transp;             // transparent controls list
	int            topWindows;         // count of top-level windows in app
	Bool           focSysDisabled;     // focus system disabled
	Bool           focSysGranted;      // SetFocus() was called inside apc_widget_set_focused
	Bool           focSysDialog;       // system dialog is in action
	UINT           errorMode;          // SetErrorMode() result
	DWORD          version;            // GetVersion() cached result
	Point          smDblClk;           // cached SM_CxDOUBLECLK values
	int            socket_version;     // socket behavior type
	List           files;              // List of active File objects
	int            mouseTimer;         // is mouse timer started
	Bool           popupActive;        // flag to avoid double popup activation
	Bool           pointerInvisible;
	HWND           console;            // win32-bound console window
	Byte           msgMask[100];       // 800 user-defined messages allowed
// socket variables
	List           sockets;            // List of watchable sockets
	HANDLE         socketMutex;        // thread semaphore
	HANDLE         socketThread;       // thread id
	Bool           socketPostSync;     // semaphore
	Bool           dont_xlate_message; // one-time stopper to TranslateMessage() call
	int            utf8_prepend_0x202D;// newer windows do automatic bidi conversion, this is to cancel it
	WCHAR *      (*alloc_utf8_to_wchar_visual)(const char*,int,int*);
	ULONG_PTR      gdiplusToken;       // GDI+ handle
	Handle         clipboards[2];
	Bool           ole_initialized;
	void*          dndDataSender;      // IDropTarget.DragEnter.DataObject dnd storage object
	void*          dndDataReceiver;    // CLIPBOARD_DND storage object
	Bool           dndInsideEvent;     // to distinguish whether the clipboard is read-only or not
	Bool           dndDefaultCursors;
	void*          dragSource;         // not null if dragging
	Handle         dragSourceWidget;   //
	Handle         dragTarget;         // last successful drop
	WORD           language_id;        // default shaping language
	char           language_descr[32];
	Bool           application_stop_signal;
} WinGuts, *PWinGuts;

typedef struct _WindowData
{
	int    borderIcons;
	int    borderStyle;
	Point  hiddenPos;
	Point  hiddenSize;
	int    state;
	Handle oldFoc;
	HWND   oldActive;
	PHash  effects;
} WindowData;

typedef struct _TimerData
{
	int  timeout;
} TimerData;

typedef struct _MenuItemData
{
	int  saved_dc;
} MenuItemData;

typedef struct _FileData
{
	SOCKETHANDLE object;
	int          type;
} FileData;

typedef struct
{
	HRGN region;
	int aperture;
} RegionData;


typedef struct _XLOGPALETTE {
	WORD         palVersion;
	WORD         palNumEntries;
	PALETTEENTRY palPalEntry[ 256];
} XLOGPALETTE, *PXLOGPALETTE;

typedef struct _XBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD          bmiColors[ 256];
} XBITMAPINFO, *PXBITMAPINFO;

#define BM_NONE    0
#define BM_BITMAP  1
#define BM_PIXMAP  2
#define BM_LAYERED 3
#define BM_AUTO    4

typedef struct _ImageCache
{
	int         cacheType;
	XBITMAPINFO rawHeader;
	Byte*       rawBits;
	Bool        freeBits;
	HBITMAP     bitmap; /* copy of sys bm, if any */
} ImageCache;

typedef struct _ImageData
{
	HRGN        imgCachedRegion;
	uint32_t*   argbBits;
	ImageCache  cache;
} ImageData;

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
	Bool        fillMode;
	int         lineWidth;
	int         lineEnd;
	int         lineJoin;
	unsigned char * linePattern;
	int         linePatternLen;
	float       miterLimit;
	FillPattern fillPattern;
	Point       fillPatternOffset;
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
	DWORD           lineJoin;
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

typedef struct _GPStylus
{
	int type, opaque;
	uint32_t fg, bg;
	FillPattern fill;
} GPStylus, *PGPStylus;

typedef struct _DCGPStylus
{
	GPStylus s;
	int refcnt;
	int line_width;
	GpPen * pen;
	GpBrush * brush;
} DCGPStylus, *PDCGPStylus;


typedef struct _DrawableData
{
	/* Drawable basic data*/
	HDC            ps;                      // general HDC
	GpGraphics    *graphics;                // GDI+ context
	PAINTSTRUCT    paintStruc;              // HDC counterpart
	HBITMAP        bm;                      // cached bitmap
	HPALETTE       pal;                     // cached palette

	/* stylus and font hash management fields */
	PDCStylus      stylusResource;          // current stylus pointer
	int            stylusFlags;             // stylus resource cache( stbXXXX)
	Stylus         stylus;                  // widgets stylus record
	PDCFont        fontResource;            // font resource pointer
	PDCGPStylus    stylusGPResource;        // GDI+ resource pointer

	/* Stock objects of HDC - to be restored after paint mode */
	HPEN           stockPen;
	HBRUSH         stockBrush;
	HFONT          stockFont;
	HBITMAP        stockBM;
	HPALETTE       stockPalette;

	/* HDC info fields */
	int            bpp;                     // bits per pixel
	Point          res;                     // resolution

	/* for opaque stroke emulation */
	int            currentROP;
	int            currentROP2;
	int            alpha;
	HPEN	       opaquePen;

	/* cached GetTextMetrics */
	BYTE           tmPitchAndFamily;
	LONG           tmOverhang;
	int            otmsStrikeoutSize, otmsStrikeoutPosition, otmsUnderscoreSize, otmsUnderscorePosition;
	float          font_sin, font_cos;

	/* HDC attributes storage outside paint mode */
	Color          lbs[2];
	int            fillMode, psFillMode;
	int            lineWidth;
	int            lineEnd;
	int            lineJoin;
	unsigned char *linePattern;
	int            linePatternLen;
	FillPattern    fillPattern;
	FillPattern    fillPattern2;
	Point          fillPatternOffset;
	int            rop;
	int            rop2;
	float          miterLimit;
	Point          transform;
	PPaintSaveData psd;                     // Their values during paint saved in sys psd

	/* Basic widget fields */
	HWND           handle;                  // Windows handle of a widget
	HWND           owner;                   // Windows owner of a widget
	HWND           parent;                  // Windows parent of a widget
	HWND           parentHandle;
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
	Point          cursorPos;               // cursor position
	Point          cursorSize;              // cursor size
	HCURSOR        pointer;                 // pointer handle
	HCURSOR        pointer2;                // user pointer data
	int            pointerId;               // pointer id
	Handle         lastMenu;                // last menu activated by WM_INITMENU or WM_INITMENUPOPUP
	Point          extraBounds;             // used in region calculations
	Point          extraPos;                // used in region calculations
	Point          layeredPos;              // delayed layered window positioning

	/* Widget DND stuff */
	void*          dropTarget;

	/* Layered subpaint */
	Point          layeredPaintOffset;
	HDC            layeredPaintSurface;
	HRGN           layeredParentRegion;

	/* Other class-specific data */
	union {
		TimerData     timer;
		WindowData    window;
		PrinterData   prn;
		FileData      file;
		ImageData     image;
		RegionData    region;
		MenuItemData  menuitem;
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

#define STYLUS_USE_GP_PEN                          \
	if ( !( sys stylusFlags & stbGPPen)) {                \
		if ( stylus_gp_alloc_pen(self) == NULL ) return false; \
		sys stylusFlags |= stbGPPen;                      \
		if ( sys stylus.pen.lopnWidth.x != sys stylusGPResource->line_width ) {\
			 GPCALL GdipSetPenWidth(sys stylusGPResource->pen, sys stylusGPResource->line_width = sys stylus.pen.lopnWidth.x);\
			 apiGPErrCheck; \
		}\
	}

#define STYLUS_USE_GP_BRUSH                          \
	if ( !( sys stylusFlags & stbGPBrush)) {                \
		if ( stylus_gp_alloc_brush(self) == NULL ) return false; \
		sys stylusFlags |= stbGPBrush;                      \
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

#define palette_create image_create_palette

typedef struct _ItemRegRec {
	int   cmd;
	void *item;
} ItemRegRec, *PItemRegRec;

extern Bool         appDead;
extern Bool         debug;
extern DIBMONOBRUSH bmiHatch;
extern PHash        fontMan;
extern PHash        myfontMan;
extern int          FONTSTRUCSIZE;
extern WinGuts      guts;
extern PHash        imageMan;
extern PHash        menuMan;
extern MusClkRec    musClk;
extern PHash        patMan;
extern DWORD        rc;
extern PHash        stylusMan;
extern PHash        stylusGpMan;
extern HBRUSH       hBrushHollow;
extern PatResource  hPatHollow;
extern HPEN         hPenHollow;
extern PHash        regnodeMan;
extern Handle       lastMouseOver;
extern int          timeDefsCount;
extern PItemRegRec  timeDefs;
extern PHash        menuBitmapMan;
extern HBITMAP      uncheckedBitmap;
extern PHash        scriptCacheMan;

LRESULT CALLBACK    generic_app_handler      ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    generic_frame_handler    ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    layered_frame_handler    ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    generic_view_handler     ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);

extern int          arc_completion( double * angleStart, double * angleEnd, int * needFigure);
extern Bool         add_font_to_hash( const PFont key, const PFont font, Bool addSizeEntry);
extern void         cm_squeeze_palette( PRGBColor source, int srcColors, PRGBColor dest, int destColors);
extern Bool         create_font_hash( void);
extern Bool         cursor_update( Handle self);
extern HDC          dc_alloc( void);
extern void         dc_free( void);
extern HDC          dc_compat_alloc( HDC compatDC);
extern void         dc_compat_free( void);
extern void         dbm_recreate( Handle self);
extern Bool         destroy_font_hash( void);
extern char *       err_msg( DWORD errId, char * buffer);
extern char *       err_msg_gplus( GpStatus errId, char * buffer);
extern PDCFont      font_alloc( Font * data);
extern void         font_change( Handle self, Font * font);
extern void         font_clean( void);
extern void         font_font2logfont( Font * font, LOGFONTW * lf);
extern void         font_free( PDCFont res, Bool permanent);
extern void         font_logfont2font( LOGFONTW * lf, Font * font, Point * resolution);
extern void         font_pp2font( char * presParam, Font * font);
extern void         font_textmetric2font( TEXTMETRICW * tm, Font * fm, Bool readOnly);
extern Bool         get_font_from_hash( PFont font, Bool bySize);
extern Point        get_window_borders( int borderStyle);
extern Bool         hwnd_check_limits( int x, int y, Bool uint);
extern void         hwnd_enter_paint( Handle self);
extern Handle       hwnd_frame_top_level( Handle self);
extern void         hwnd_leave_paint( Handle self);
extern Handle       hwnd_to_view( HWND win);
extern Handle       hwnd_top_level( Handle self);
extern Handle       hwnd_layered_top_level( Handle self);
extern Bool         hwnd_repaint_layered( Handle self, Bool now);
extern HICON        image_make_icon_handle( Handle img, Point size, Point * hotSpot);
extern void         image_query_bits( Handle self, Bool forceNewImage);
extern void         image_argb_query_bits( Handle self);
extern HBITMAP      image_create_bitmap_by_type( Handle self, HPALETTE pal, XBITMAPINFO * bitmapinfo, int bm_type);
extern HBITMAP      image_create_bitmap( Handle self );
extern HBITMAP      image_create_argb_dib_section( HDC dc, int w, int h, uint32_t ** ptr);
extern HPALETTE     image_create_palette( Handle self);
extern void         image_destroy_cache( Handle self);
extern BITMAPINFO*  image_fill_bitmap_info( Handle self, XBITMAPINFO * bi, int bm_type);
extern void         mod_free( BYTE * modState);
extern BYTE *       mod_select( int mod);
extern Bool         palette_change( Handle self);
extern long         palette_match( Handle self, long color);
extern int          palette_match_color( XLOGPALETTE * lp, long clr, int * diffFactor);
extern PPatResource patres_fetch( unsigned char * pattern, int len);
extern UINT         patres_user( unsigned char * pattern, int len);
extern void         process_transparents( Handle self);
extern long         remap_color( long clr, Bool toSystem);
extern void         socket_rehash( void);
extern PDCStylus    stylus_alloc( PStylus data);
extern void         stylus_change( Handle self);
extern void         stylus_clean( void);
extern Bool         stylus_complex( PStylus stylus, HDC dc);
extern Bool         stylus_extpenned( PStylus stylus);
extern void         stylus_free( PDCStylus res, Bool permanent);
extern DWORD        stylus_get_extpen_style( PStylus s);
extern GpPen*       stylus_gp_alloc_pen(Handle self);
extern GpBrush*     stylus_gp_alloc_brush(Handle self);
extern void         stylus_gp_clean( void);
extern void         stylus_gp_free( PDCGPStylus res, Bool permanent);
extern HRGN         region_create( Handle mask);
extern WCHAR *      alloc_utf8_to_wchar( const char * utf8, int length, int * mb_len);
extern WCHAR *      alloc_utf8_to_wchar_visual( const char * utf8, int length, int * mb_len);
extern WCHAR *      alloc_ascii_to_wchar( const char * text, int length);
extern char *       alloc_wchar_to_utf8( WCHAR * src, int * len );
extern void         wchar2char( char * dest, WCHAR * src, int lim);
extern void         char2wchar( WCHAR * dest, char * src, int lim);
extern int          apcUpdateWindow( HWND wnd );
extern void         reset_system_fonts(void);
extern void         register_mapper_fonts(void);
extern void         dpi_change(void);
extern Bool         is_dwm_enabled(void);
extern Bool         dnd_clipboard_create(void);
extern void         dnd_clipboard_destroy(void);
extern Bool         dnd_clipboard_open(void);
extern Bool         dnd_clipboard_close(void);
extern Bool         dnd_clipboard_clear(void);
extern PList        dnd_clipboard_get_formats(void);
extern Bool         dnd_clipboard_get_data( Handle id, PClipboardDataRec c);
extern Bool         dnd_clipboard_has_format( Handle id);
extern Bool         dnd_clipboard_set_data( Handle id, PClipboardDataRec c);
extern PList        dnd_clipboard_get_formats();
extern char *       cf2name( UINT cf );
extern Bool         clipboard_get_data(int cfid, PClipboardDataRec c, void * p1, void * p2);
extern void *       image_create_dib(Handle image, Bool global_alloc);
extern Bool         HWND_lock( Bool lock);
extern Bool         process_msg( MSG * msg);

/* compatibility to MSVC 6 */
#ifndef GWLP_USERDATA
#	define GWLP_USERDATA GWL_USERDATA
#	define GWLP_WNDPROC  GWL_WNDPROC
#	define LONG_PTR      LONG
#	define GetWindowLongPtr GetWindowLong
#	define SetWindowLongPtr SetWindowLong
#endif

#ifndef WS_EX_LAYERED
#define ULW_ALPHA 0x00000002
#define WS_EX_LAYERED 0x00080000
WINUSERAPI
BOOL
WINAPI
UpdateLayeredWindow(
    __in HWND hWnd,
    __in_opt HDC hdcDst,
    __in_opt POINT* pptDst,
    __in_opt SIZE* psize,
    __in_opt HDC hdcSrc,
    __in_opt POINT* pptSrc,
    __in COLORREF crKey,
    __in_opt BLENDFUNCTION* pblend,
    __in DWORD dwFlags);
#endif

#ifndef MUI_LANGUAGE_NAME
#define MUI_LANGUAGE_NAME 0x8
#define MUI_COMPLEX_SCRIPT_FILTER 0x200
#endif

BOOL
my_GetUserPreferredUILanguages(
	DWORD dwFlags, PULONG pulNumLanguages,
	PZZWSTR pwszLanguagesBuffer, PULONG pcchLanguagesBuffer
);


#ifdef __cplusplus
}
#endif


#endif

