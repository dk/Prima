#ifndef _WIN32_H_
#define _WIN32_H_
#include <windows.h>
#include <winspool.h>
#include <wtypes.h>
#define Rect xxRect
#define Color xxColor
#define Point xxPoint
#ifdef _MSC_VER
#undef __inline__
#define __inline__ inline
#undef __extension__
#define __extension__
#endif
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
typedef HANDLE SOCKETHANDLE;
#undef  HWND_DESKTOP
#define HWND_DESKTOP         guts. desktop_window


#ifdef UNICODE
#error This version of apc_Win32 does not support Unicode
#endif

#define DEFAULT_SYSTEM_FONT              "System"
#define DEFAULT_WIDGET_FONT              (((DWORD)(LOBYTE(LOWORD(guts.version)))>5)?"Segoe UI":"MS Shell Dlg")
#define DEFAULT_WIDGET_FONT_SIZE         (((DWORD)(LOBYTE(LOWORD(guts.version)))>5)?9:8)
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
#define WM_FILE_REHASH                    ( WM_USER + 20)
#define WM_REPAINT_LAYERED                ( WM_USER + 21)
#define WM_DRAG_RESPONSE                  ( WM_USER + 22)
#define WM_XMOUSECLICK                    ( WM_USER + 23)
#define WM_SIGNAL                         ( WM_USER + 24)
#define WM_SYNTHETIC_EVENT                ( WM_USER + 25)
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

#define stbPen          0x01
#define stbBrush        0x02
#define stbText         0x04
#define stbGDIMask      0x0F
#define stbGPBrush      0x10

#define FHT_SOCKET  1
#define FHT_PIPE    2
#define FHT_OTHER   3
#define FHT_STDIN   4

#define apcWarn \
	if (debug) \
		warn( "win32 error 0x%x: '%s' at line %d in %s\n", (unsigned int)rc, \
			err_msg( rc, NULL), __LINE__, __FILE__);   \
	else \
		err_msg( rc, NULL)

#define apcErr( err) guts.apc_error = err
#define apiErr {           \
	rc = GetLastError();    \
	guts.apc_error = errApcError; \
	apcWarn;                \
}
#define apiAltErr( err) {  \
	guts.apc_error = errApcError; \
	rc = err;               \
	apcWarn;                \
}
#define apiErrRet         { apiErr;               return false; }
#define apcErrRet(err)    { apcErr(err);          return false; }
#define apcErrClear       { guts.apc_error = errOk;              }

#define GPCALL rc = (DWORD)
#define apiGPErr { \
	guts.apc_error = errApcError; \
	if ( debug ) \
		warn( "win32 error 0x%x: '%s' at line %d in %s\n", (unsigned int)rc, \
			err_msg_gplus( rc, NULL), __LINE__, __FILE__);   \
	rc |= 0x40000;    \
}
#define apiGPErrCheck if (rc) apiGPErr;
#define apiGPErrCheckRet(f) if (rc) { apiGPErr; return f; }
#define apiHErr(hr) {           \
	guts.apc_error = errApcError; \
	rc = hr;                \
	apcWarn;                \
}

#define apiHErrRet(hr)     { apiHErr(hr);           return false; }

#define objCheck          if ( var stage == csDead) return
#define dobjCheck(handle) if ((( PObject)handle)-> stage == csDead) return

#define SHIFT_X(X)     X -= sys transform2.x
#define SHIFT_Y(Y)     Y = sys last_size.y - (Y) - 1 - sys transform2.y
#define SHIFT_XY(X,Y)  if ( 1 ) { SHIFT_X(X); SHIFT_Y(Y); }
#define SHIFT_POINT(P) SHIFT_XY(P.x,P.y)

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
	unsigned aptGDIPlus              : 1;       // uses GDI+
	unsigned aptWantWorldTransform   : 1;       // SetWorldTransform is wanted for text
	unsigned aptUsedWorldTransform   : 1;       // SetWorldTransform(matrix) mode is on currently
	unsigned aptCachedWorldTransform : 1;       // SetWorldTransform doesn't need to be called repeatedly
} HandleOptions;

#define CLIPBOARD_MAIN 0
#define CLIPBOARD_DND  1

typedef struct _WinGuts
{
	HINSTANCE      instance;            // application instance
	int            cmd_show;            // run command state
	int            app_lock;            // application lock count
	int            pointer_lock;        // pointer lock count
	DWORD          main_thread_id;      // Id of main thread
	Point          display_resolution;  // screen resolution in ppi
	char           default_fixed_font    [256];
	char           default_variable_font [256];
	char           default_system_font   [256];
	Font           window_font;         // window default font
	Font           menu_font;           // menu default font
	Font           msg_font;            // message default font
	Font           cap_font;            // caption default font
	BITMAPINFO     display_bm_info;     // display bpp & size
	HWND           desktop_window;      // GetDesktopWindow() result
	Bool           insert_mode;         // fake insert mode
	Point          icon_size_large;
	Point          icon_size_small;
	Point          pointer_size;
	BYTE           key_state[ 256];      // application key buffer state
	BYTE           empty_key_state[ 256];// just zeros
	BYTE          *current_key_state;    // current virtual key buffer state
	HKL            key_layout;           // key layout, most likely latin for Ctrl+Key mapping
	NONCLIENTMETRICSW ncmData;           // windows system data
	List           transp;               // transparent controls list
	int            top_windows;          // count of top-level windows in app
	Bool           sys_focus_disabled;   // focus system disabled
	Bool           sys_focus_granted;    // SetFocus() was called inside apc_widget_set_focused
	Bool           sys_focus_dialog;     // system dialog is in action
	UINT           error_mode;           // SetErrorMode() result
	DWORD          version;              // GetVersion() cached result
	Point          cmDOUBLECLK;          // cached SM_CxDOUBLECLK values
	int            mouse_timer;          // is mouse timer started
	Bool           popup_active;         // flag to avoid double popup activation
	Bool           pointer_invisible;
	HWND           console;              // win32-bound console window
	Bool           dont_xlate_message;   // one-time stopper to TranslateMessage() call
	int            utf8_prepend_0x202D;  // newer windows do automatic bidi conversion, this is to cancel it
	WCHAR *      (*alloc_utf8_to_wchar_visual)(const char*,int,int*);
	ULONG_PTR      gdiplus_token;        // GDI+ handle
	Handle         clipboards[2];
	Bool           ole_initialized;
	void*          dnd_data_sender;      // IDropTarget.DragEnter.DataObject dnd storage object
	void*          dnd_data_receiver;    // CLIPBOARD_DND storage object
	Bool           dnd_inside_event;     // to distinguish whether the clipboard is read-only or not
	Bool           dnd_default_cursors;
	void*          drag_source;          // not null if dragging
	Handle         drag_source_widget;   //
	Handle         drag_target;          // last successful drop
	WORD           language_id;          // default shaping language
	char           language_descr[32];
	Bool           application_stop_signal;
	long           apc_error;
	Bool           wc2mb_is_fragile;     // cannot properly process current ACP

	int            get_pixel_needs_emulation; // 0 - not tried, -1 - no, 1 - yes
	HDC            get_pixel_dc_src, get_pixel_dc_dst;

	unsigned long  program_start_ts ;            // epoch
	unsigned int   mouse_double_click_delay;     // ms
	unsigned long  last_mouse_click_ts;          // (time - epoch) * 1000 + msec
	HWND           last_mouse_click_source;
	LPARAM         last_mouse_click_position;
	unsigned int   last_mouse_click_number;
	uint32_t       last_mouse_click_fingerprint; // kmXXX | mbXXX
} WinGuts, *PWinGuts;

typedef struct _WindowData
{
	int            border_icons;
	int            border_style;
	Point          hidden_pos;
	Point          hidden_size;
	int            state;
	Handle         old_foc;
	HWND           old_active;
	PHash          effects;
	WINDOWPLACEMENT fs_saved_placement;
} WindowData;

typedef struct _TimerData
{
	int            timeout;
} TimerData;

typedef struct _MenuItemData
{
	int            saved_dc;
} MenuItemData;

typedef struct _FileData
{
	intptr_t       object;
	int            type;
	WINHANDLE      event;
} FileData;

typedef struct
{
	HRGN region;
	int aperture;
} RegionData;


typedef struct _XLOGPALETTE {
	WORD           palVersion;
	WORD           palNumEntries;
	PALETTEENTRY   palPalEntry[ 256];
} XLOGPALETTE, *PXLOGPALETTE;

typedef struct {
	UINT           flags;
	UINT           count;
	ARGB           entries[256];
} XColorPalette, *PXColorPalette;

typedef struct _XBITMAPINFO {
	BITMAPINFOHEADER header;
	RGBQUAD          colors[ 256];
} XBITMAPINFO, *PXBITMAPINFO;

#define BM_NONE    0
#define BM_BITMAP  1
#define BM_PIXMAP  2
#define BM_LAYERED 3
#define BM_AUTO    4

typedef struct _ImageCache
{
	int            cache_type;
	XBITMAPINFO    raw_header;
	Byte*          raw_bits;
	Bool           free_bits;
	HBITMAP        bitmap; /* copy of sys bm, if any */
} ImageCache;

typedef struct _ImageData
{
	HRGN           img_cached_region;
	uint32_t*      argb_bits;
	ImageCache     cache;
} ImageData;

typedef struct _PrinterData
{
	PRINTER_INFO_2 ppi;
	char           def_prn_buf[256];
	char          *device;
	char          *driver;
	char          *port;
} PrinterData;

typedef struct _PaintSaveData
{
	Bool           antialias, fill_mode;
	int            alpha;
	Color          fg, bg;
	unsigned char *line_pattern;
	int            line_pattern_len;
	FillPattern    fill_pattern;
	Point          fill_pattern_offset;
	int            rop, rop2;
	Font           font;
	Bool           text_opaque, text_out_baseline;
} PaintSaveData, *PPaintSaveData;

typedef struct
{
	DWORD          count;
	DWORD*         ptr;
	DWORD          dots[1];
} LinePattern, *PLinePattern;

typedef struct _DIBMONOBRUSH
{
	BITMAPINFOHEADER header;
	RGBQUAD         colors[2];
	unsigned char   data[32];
} DIBMONOBRUSH, *PDIBMONOBRUSH;

typedef struct _DCFont
{
	Font             font;
	int              refcnt;
	HFONT            hfont;
} DCFont, *PDCFont;

#define DCO_PEN           0
#define DCO_BRUSH         1
#define DCO_GP_PEN        2
#define DCO_GP_BRUSH      3
#define DCO_COUNT         4

typedef struct {
	int            type;
	int            refcnt;
	HANDLE         handle;
	Bool           cached;
	unsigned int   rq_size;
	void          *rq;
	char           rq_buf[1];
} DCObject, *PDCObject;

typedef struct {
	int            type;
	LOGPEN         logpen;
	Bool           geometric;
	DWORD          style;
	LinePattern   *line_pattern;
} RQPen, *PRQPen;

typedef struct {
	int            type;
	LOGBRUSH       logbrush;
	COLORREF       color, back_color;
	FillPattern    fill_pattern;
} RQBrush, *PRQBrush;

typedef struct
{
	int            type;
	uint32_t       fg, line_width;
} RQGPPen, *PRQGPPen;

typedef struct
{
	int            type;
	uint32_t       fg, bg, opaque;
	FillPattern    fill_pattern;
} RQGPBrush, *PRQGPBrush;

typedef struct _PaintState
{
	Bool               in_paint;
	struct {
		int        stylus_flags;
		PDCObject  dc_obj[DCO_COUNT];
		RQPen      rq_pen;
		RQBrush    rq_brush;
		PDCFont    dc_font;
		float      font_sin, font_cos;
		Bool       wt_want, wt_used, wt_cached;
	} paint;
	struct {
		HPALETTE   palette;
	} nonpaint;
	PaintSaveData      common;
	Handle             fill_image;

	unsigned int       user_data_size;
	GCStorageFunction *user_destructor;
	void              *user_data, *user_context;
	char               user_data_buf[1]; /* this needs to be the last */
} PaintState, *PPaintState;


typedef struct _DrawableData
{
	/* Drawable basic data*/
	HDC            ps;                  // general HDC
	GpGraphics    *graphics;            // GDI+ context
	PAINTSTRUCT    paint_struct;        // HDC counterpart
	HBITMAP        bm;                  // cached bitmap
	HPALETTE       pal;                 // cached palette
	PList          gc_stack;            // push/pop

	/* pen, brush, and font hash management fields */
	int            stylus_flags;        // stylus resource cache( stbXXXX)
	PDCObject      current_dc_obj[DCO_COUNT];
	RQPen          rq_pen;
	RQBrush        rq_brush;
	PDCFont        dc_font;

	/* Stock objects of HDC - to be restored after paint mode */
	HPEN           stock_pen;
	HBRUSH         stock_brush;
	HFONT          stock_font;
	HBITMAP        stock_bitmap;
	HPALETTE       stock_palette;

	/* HDC info fields */
	int            bpp;                 // bits per pixel
	Point          res;                 // resolution

	/* for opaque stroke emulation */
	int            alpha;

	/* cached GetTextMetrics */
	BYTE           tmPitchAndFamily;
	LONG           tmOverhang;
	int            otmsStrikeoutSize, otmsStrikeoutPosition, otmsUnderscoreSize, otmsUnderscorePosition;
	float          font_sin, font_cos;

	/* HDC attributes storage outside paint mode */
	Color          fg, bg;
	int            fill_mode;
	unsigned char *line_pattern;
	int            line_pattern_len;
	FillPattern    fill_pattern;
	Point          fill_pattern_offset;
	int            rop;
	int            rop2;

	/* Basic widget fields */
	HWND           handle;              // Windows handle of a widget
	HWND           owner;               // Windows owner of a widget
	HWND           parent;              // Windows parent of a widget
	HWND           parent_handle;
	int            class_name;          // class name ( WC_XXX)

	/* Widget properties */
	HandleOptions  options;             // apt_XXX settings
	ColorSet       view_colors;         // widget color palette
	PXLOGPALETTE   p256;                // cached squeezed palette
	void *         recreate_data;       // ViewProfile custom area

	/* Custom data for widget paint in optBuffered state */
	HDC            ps2;                 // original HDC
	HPALETTE       pal2;                // original palette
	Point          transform2;          // necessary additional transposition
	Point          effective_view;      // area to be drawn, possibly with a backed bitmap

	/* Positioning support fields */
	Point          last_size;           // last actual size
	int            size_lock_level;     // size locking flag
	int            y_override;          // special cached height value. Used in WM_SIZE<->WM_MOVE interactions

	/* Widget attributes - timers, cursor, pointers, menu, shape */
	Point          cursor_pos;          // cursor position
	Point          cursor_size;         // cursor size
	HCURSOR        pointer;             // pointer handle
	HCURSOR        pointer2;            // user pointer data
	int            pointer_id;          // pointer id
	Handle         last_menu;           // last menu activated by WM_INITMENU or WM_INITMENUPOPUP
	Point          extra_bounds;        // used in region calculations
	Point          extra_pos;           // used in region calculations
	Point          layered_pos;         // delayed layered window positioning

	/* Widget DND stuff */
	void*          drop_target;

	/* Layered subpaint */
	Point          layered_paint_offset;
	HDC            layered_paint_surface;
	HRGN           layered_parent_region;

	/* alpha text amulation */
	HDC            alpha_arena_dc;
	HBITMAP        alpha_arena_bitmap;
	uint32_t*      alpha_arena_ptr;
	Point          alpha_arena_size;
	Bool           alpha_arena_font_changed;
	HFONT          alpha_arena_stock_font;
	HBITMAP        alpha_arena_stock_bitmap;
	uint32_t*      alpha_arena_palette;

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

typedef struct _MouseClickRec {
	Bool    pending;
	UINT    emsg;
	MSG     msg;
} MouseClickRec;

#define STYLUS_USE_PEN                                               \
	if ( !( sys stylus_flags & stbPen)) {                        \
		if ( select_pen(self))                               \
		   sys stylus_flags |= stbPen;                       \
	}

#define STYLUS_USE_BRUSH                                             \
	if ( !( sys stylus_flags & stbBrush)) {                      \
		if ( select_brush(self))                             \
		   sys stylus_flags |= stbBrush;                     \
	}


#define STYLUS_USE_TEXT                                              \
	if ( !( sys stylus_flags & stbText)) {                       \
		if (sys ps)                                          \
			SetTextColor(sys ps, sys rq_pen.logpen.lopnColor);\
		sys stylus_flags |= stbText;                         \
	}

#define STYLUS_USE_GP_BRUSH                                          \
	if ( !( sys stylus_flags & stbGPBrush)) {                    \
		if ( select_gp_brush(self))                          \
			sys stylus_flags |= stbGPBrush;              \
	}

#define STYLUS_FREE_PEN              sys stylus_flags &= ~stbPen
#define STYLUS_FREE_BRUSH            sys stylus_flags &= ~stbBrush
#define STYLUS_FREE_PEN_AND_BRUSH    sys stylus_flags &= ~(stbBrush|stbPen)
#define STYLUS_FREE_GP_PEN
#define STYLUS_FREE_GP_BRUSH         sys stylus_flags &= ~stbGPBrush
#define STYLUS_FREE_GP               sys stylus_flags &= ~stbGPBrush
#define STYLUS_FREE_ALL              sys stylus_flags &= ~(stbGPBrush|stbBrush|stbPen|stbText)
#define STYLUS_FREE_TEXT             sys stylus_flags &= ~stbText

#define CURRENT_PEN      ((HPEN)  sys current_dc_obj[DCO_PEN]->handle)
#define CURRENT_BRUSH    ((HBRUSH)sys current_dc_obj[DCO_BRUSH]->handle)
#define CURRENT_GP_PEN   ((GpPen*)( sys current_dc_obj[DCO_GP_PEN]   ? sys current_dc_obj[DCO_GP_PEN]->handle   : NULL))
#define CURRENT_GP_BRUSH ((GpPen*)( sys current_dc_obj[DCO_GP_BRUSH] ? sys current_dc_obj[DCO_GP_BRUSH]->handle : NULL))

#define csAxEvents csFrozen

#define apt_set( option)           ( sys options. option = 1)
#define apt_clear( option)         ( sys options. option = 0)
#define is_apt( option)            ( sys options. option)
#define apt_assign( option, value) ( sys options. option = (value)?1:0)

#define is_declipped( handle)      (                                                        \
	handle && ( dsys(handle) class_name != WC_FRAME ) &&                                \
	( !dsys(handle)options.aptClipOwner || ((( PWidget)handle)-> owner == application)) \
)

#define is_declipped_child( handle) (                                                       \
	handle && ( dsys(handle) class_name != WC_FRAME ) &&                                \
	!dsys(handle)options.aptClipOwner                                                   \
)

#define palette_create image_create_palette

typedef struct _ItemRegRec {
	int            cmd;
	void          *item;
} ItemRegRec, *PItemRegRec;

extern Bool            app_dead;
extern Bool            debug;
extern int             FONTSTRUCSIZE;
extern WinGuts         guts;
extern Handle          last_mouse_over;
extern PHash           mgr_fonts;
extern PHash           mgr_myfonts;
extern PHash           mgr_images;
extern PHash           mgr_menu;
extern PHash           mgr_menu_bitmaps;
extern PHash           mgr_patterns;
extern PHash           mgr_registry;
extern PHash           mgr_scripts;
extern PHash           mgr_styli;
extern MouseClickRec   mouse_click;
extern DWORD           rc;
extern HCURSOR         std_arrow_cursor;
extern HBRUSH          std_hollow_brush;
extern LinePattern     std_hollow_line_pattern;
extern HPEN            std_hollow_pen;
extern HBITMAP         std_unchecked_bitmap;
extern int             time_defs_count;
extern PItemRegRec     time_defs;

#define MAX_SELECT_HANDLES (MAXIMUM_WAIT_OBJECTS-1)
extern WINHANDLE       select_handles[MAX_SELECT_HANDLES];
extern unsigned int    select_n_handles;

LRESULT CALLBACK    generic_app_handler      ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    generic_frame_handler    ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    layered_frame_handler    ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);
LRESULT CALLBACK    generic_view_handler     ( HWND win, UINT  msg, WPARAM mp1, LPARAM mp2);

extern Bool         aa_text_out( Handle self, int x, int y, void * text, int len, Bool wide);
extern Bool         aa_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y, int * text_advance, HFONT font);
extern void         aa_free_arena(Handle self, Bool for_reuse);
extern WCHAR *      alloc_utf8_to_wchar( const char * utf8, int length, int * mb_len);
extern WCHAR *      alloc_utf8_to_wchar_visual( const char * utf8, int length, int * mb_len);
extern WCHAR *      alloc_ascii_to_wchar( const char * text, int *length);
extern char *       alloc_wchar_to_utf8( WCHAR * src, int * len );
extern int          apcUpdateWindow( HWND wnd );
extern Bool         add_font_to_hash( const PFont key, const PFont font, Bool addSizeEntry);
extern char *       cf2name( UINT cf );
extern void         char2wchar( WCHAR * dest, char * src, int lim);
extern void         cleanup_gc_stack(Handle self, Bool all);
extern Bool         clipboard_get_data(int cfid, PClipboardDataRec c, void * p1, void * p2);
extern void         cm_squeeze_palette( PRGBColor source, int srcColors, PRGBColor dest, int destColors);
extern Bool         create_font_hash( void);
extern Bool         cursor_update( Handle self);
extern HDC          dc_alloc( void);
extern void         dc_free( void);
extern HDC          dc_compat_alloc( HDC compatDC);
extern void         dc_compat_free( void);
extern void         dbm_recreate( Handle self);
extern Bool         destroy_font_hash( void);
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
extern void         dpi_change(void);
extern char *       err_msg( DWORD errId, char * buffer);
extern char *       err_msg_gplus( GpStatus errId, char * buffer);
extern Bool         file_process_events(int cmd, WPARAM param1, LPARAM param2);
extern void         file_subsystem_done( void);
extern Bool         file_subsystem_init( void);
extern PDCFont      font_alloc( Font * data);
extern void         font_change( Handle self, Font * font);
extern void         font_clean( void);
extern void         font_font2logfont( Font * font, LOGFONTW * lf);
extern void         font_free( PDCFont res, Bool permanent);
extern void         font_logfont2font( LOGFONTW * lf, Font * font, Point * resolution);
extern void         font_pp2font( char * presParam, Font * font);
extern void         font_textmetric2font( TEXTMETRICW * tm, Font * fm, Bool readOnly);
extern Bool         get_font_from_hash( PFont font, Bool bySize);
extern Point        get_window_borders( int border_style);
extern void         gp_get_text_box( Handle self, ABC * abc, Point * pt);
extern void         gp_get_text_widths( Handle self, const char* text, int len, int flags, ABC * extents);
extern Bool         hwnd_check_limits( int x, int y, Bool uint);
extern void         hwnd_enter_paint( Handle self);
extern Handle       hwnd_frame_top_level( Handle self);
extern void         hwnd_leave_paint( Handle self);
extern Bool         hwnd_lock( Bool lock);
extern Handle       hwnd_to_view( HWND win);
extern Handle       hwnd_top_level( Handle self);
extern Handle       hwnd_layered_top_level( Handle self);
extern Bool         hwnd_repaint_layered( Handle self, Bool now);
extern void         image_argb_query_bits( Handle self);
extern HICON        image_make_icon_handle( Handle img, Point size, Point * hot_spot);
extern HBITMAP      image_create_argb_dib_section( HDC dc, int w, int h, uint32_t ** ptr);
extern HBITMAP      image_create_bitmap_by_type( Handle self, HPALETTE pal, XBITMAPINFO * bitmapinfo, int bm_type);
extern HBITMAP      image_create_bitmap( Handle self );
extern BITMAPINFO*  image_create_color_pattern_dib( Handle self);
extern void *       image_create_dib(Handle image, Bool global_alloc);
extern GpTexture*   image_create_gp_pattern( Handle self, Handle image, unsigned int alpha );
extern BITMAPINFO*  image_create_mono_pattern_dib(Handle self, COLORREF fg, COLORREF bg);
extern HPALETTE     image_create_palette( Handle self);
extern void         image_destroy_cache( Handle self);
extern void         image_fill_bitmap_cache( Handle self, int bm_type, Handle optimize_for_surface);
extern BITMAPINFO*  image_fill_bitmap_info( Handle self, XBITMAPINFO * bi, int bm_type);
extern void         image_query_bits( Handle self, Bool forceNewImage);
extern Bool         is_dwm_enabled(void);
extern void         mod_free( BYTE * modState);
extern BYTE *       mod_select( int mod);
extern Bool         palette_change( Handle self);
extern long         palette_match( Handle self, long color);
extern int          palette_match_color( XLOGPALETTE * lp, long clr, int * diff_factor);
extern PLinePattern patres_fetch( unsigned char * pattern, int len);
extern UINT         patres_user( unsigned char * pattern, int len);
extern Bool         process_file_msg( WINHANDLE src);
extern Bool         process_msg( MSG * msg);
extern void         process_transparents( Handle self);
extern HRGN         region_create( Handle mask);
extern void         register_mapper_fonts(void);
extern long         remap_color( long clr, Bool toSystem);
extern void         reset_system_fonts(void);
extern void         syshandle_rehash( void);
extern Bool         select_pen(Handle self);
extern Bool         select_brush(Handle self);
extern Bool         select_gp_brush(Handle self);
extern Bool         select_world_transform(Handle self, Bool want_transform);
extern void         stylus_clean( void);
extern PDCObject    stylus_fetch( void * key );
extern Bool         stylus_is_complex(Handle self);
extern Bool         stylus_is_geometric( Handle self);
extern void         stylus_release( Handle self );
extern GpPen*       stylus_gp_get_pen(int line_width, uint32_t color);
extern HPEN         stylus_get_pen( DWORD style, DWORD line_width, COLORREF color );
extern HBRUSH       stylus_get_solid_brush( COLORREF color );
extern void         wchar2char( char * dest, WCHAR * src, int lim);
extern Bool         yield( Bool wait_for_event );

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

#ifndef CONSOLE_READ_NOREMOVE
#define CONSOLE_READ_NOREMOVE   0x0001
#endif

#ifndef CONSOLE_READ_NOWAIT
#define CONSOLE_READ_NOWAIT     0x0002
#endif

Bool
read_console_input(
	WINHANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength,
	LPDWORD lpNumberOfEventsRead, USHORT wFlags
);

#ifdef __cplusplus
}
#endif


#endif

