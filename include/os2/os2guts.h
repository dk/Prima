#ifndef _OS2GUTS_H_
#define _OS2GUTS_H_
#include "guts.h"
#define INCL_WIN
#define INCL_DOS
#define INCL_SPL
#define INCL_GPI
#define INCL_SPLDOSPRINT
#include <os2.h>

#define WM_WRITE_TO_LOG                   ( WM_USER + 0)
#define WM_PRIMA_CREATE                   ( WM_USER + 1)
#define WM_MENUCOMMAND                    ( WM_USER + 2)
#define WM_PRESPARAMMENU                  ( WM_USER + 3)
#define WM_POSTAL                         ( WM_USER + 4)
#define WM_MENUCOLORCHANGED               ( WM_USER + 5)
#define WM_DLGENTERMODAL                  ( WM_USER + 6)
#define WM_DLGPASSIVATE                   ( WM_USER + 7)
#define WM_DLGPOPUP                       ( WM_USER + 8)
#define WM_BREAKMSGLOOP                   ( WM_USER + 9)
#define WM_MOUSEENTER                     ( WM_USER + 10)
#define WM_MOUSELEAVE                     ( WM_USER + 11)
#define WM_FONTCHANGED                    ( WM_USER + 12)
#define WM_ACTIVATEMENU                   ( WM_USER + 13)
#define WM_VIEWHELP                       ( WM_USER + 14)
#define WM_TERMINATE                      ( WM_USER + 99)

// OS/2 defaults for apc
#define DEFAULT_FONT_NAME                "Helv"
#define DEFAULT_FONT_SIZE                10
#define DEFAULT_FONT_FACE                "10.Helv"
#define DEFAULT_FIXED_FONT               "Courier"
#define DEFAULT_VARIABLE_FONT            "Helv"
#define DEFAULT_SYSTEM_FONT              "System VIO"
#define CTRL_ID_AUTOSTART                100
#define MENU_ID_AUTOSTART                1000
#define csAxEvents csFrozen

#define WC_CUSTOM ((PSZ)0)
#define WC_DIALOG ((PSZ)1)
#define WC_APPLICATION ((PSZ)2)


#define USE_GPIDRAWBITS

typedef struct _ItemRegRec {
  int   cmd;
  void *item;
} ItemRegRec, *PItemRegRec;

typedef struct _OS2Guts
{
   HAB   anchor;             // program main ab
   HMQ   queue;              // message queue
   PID   pid;                // process ID
   HWND  logger;             // logger window
   HWND  loggerListBox;      //    listbox that contains log data
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
   ULONG codePage;           // current system codepage
   HWND  helpWnd;            // help window id
} OS2Guts;

extern OS2Guts guts;
extern int ctx_kb2VK[];
extern APIRET rc;
extern void cursor_update( Handle self);
extern int ctx_cr2SPTR[];
extern Bool   appDead;
extern Handle firstModal;
extern Handle dlgModal;

#define  SEVERE_DEBUG
#ifndef  SEVERE_DEBUG
#define apiErr       { rc = WinGetLastError( guts. anchor);    apcError = errApcError; }
#define apcErr( err)    apcError = err;
#define apiAltErr( err) { apcError = errApcError; rc = err; }
#else
#define apiErr {                                            \
   rc = WinGetLastError( guts. anchor);                     \
   apcError = errApcError;                                  \
   log_write("OS2_%03x at line %d", rc, __LINE__);          \
}
#define apcErr( err) {                                      \
   apcError = err;                                          \
   log_write("APC_%03x at line %d", err, __LINE__);          \
}
#define apiAltErr( err) {                                   \
   apcError = errApcError;                                  \
   rc = err;                                                \
   log_write("OS2_%03x at line %d", rc, __LINE__);          \
}
#endif
#define apiErrRet         { apiErr;               return false; }
#define apiErrCheckRet    { apiErrCheck; if ( rc) return false; }
#define apcErrRet(err)    { apcErr(err);          return false; }
#define apcErrClear       { apcError = errOk;                   }

#define aptWM_PAINT             0x00000001       // true if inside WM_PAINT
#define aptWinPS                0x00000002       // window PS was passed to paint
#define aptCompatiblePS         0x00000004       // PS is screen-compatible
#define aptFontExists           0x00000008       // font is selected on HPS
#define aptCursorVis            0x00000010       // cursor visible flag
#define aptPointerVis           0x00000020       // pointer visibility flag
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

#define apt_set( option)           (sys options |= option)
#define apt_clear( option)         (sys options &= ~option)
#define is_apt( option)            ((sys options & option)?1:0)
#define apt_assign( option, value) ((value)?apt_set(option):apt_clear(option))

typedef struct _WindowData
{
   int   borderIcons;
   int   borderStyle;
   Point hiddenPos;
   Point hiddenSize;
   int   state;

   Handle nextModal;
   int    modalResult;
   Point  lastClientSize;
   Point  lastFrameSize;
} WindowData;

typedef struct _TimerData
{
   int  timeout;
   Bool active;
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
   int         lineWidth;
   int         lineEnd;
   int         linePattern;
   FillPattern fillPattern;
   int         rop;
   int         rop2;
   Point       transform;
   Font        font;
   Bool        textOpaque;
} PaintSaveData, *PPaintSaveData;

typedef struct _DrawableData
{
   HPS      ps;
   HPS      ps2;
   unsigned long options;            // aptXXX options
   HBITMAP  bm;                      // cached bitmap
   char    *bmRaw;                   // cached raw bitmap
   PBITMAPINFO2 bmInfo;              // raw bitmap info
   Font     font;                    // cached font metric
   void    *fontHash;
   int      fontId;
   HDC      dc;                      // cached HDC
   HDC      dc2;                     // cached HDC
// HPS data
   Color       lbs[2];
   int         lineWidth;
   int         lineEnd;
   int         linePattern;
   FillPattern fillPattern;
   FillPattern fillPattern2;
   int         rop;
   int         rop2;
   Point       transform;
   Point       transform2;
   Point       res;
   float *     charTable;
   PFont       saveFont;
   Bool        textOpaque;
   HBITMAP     fillBitmap;
// HWND data
   ApiHandle handle2;               // auxillary handler
   HWND    parent;                  // window parent
   HWND    owner;                   // window owner
   int     lockState;               // locking count
   Point   cursorPos;               // cursor position
   Point   cursorSize;              // cursor size
   HPOINTER pointer;                // pointer data
   HPOINTER pointer2;               // user pointer data
   int     pointerId;               // pointer id
   PSZ     className;               // WC_XXXXXXXXXX
   int     timeDefsCount;           // count of timers attached.
   PItemRegRec timeDefs;            // timer list
   Color   l3dc;                    // light 3d color
   Color   d3dc;                    // dark  3d color
   PPaintSaveData  psd;
   union {
     TimerData     timer;
     WindowData    window;
     PrinterData   prn;
   } s;
} DrawableData, *PDrawableData;

typedef struct _MenuWndData
{
   PFNWP      fnwp;
   Handle     menu;
   int        id;
} MenuWndData, *PMenuWndData;

MRESULT EXPENTRY generic_view_handler     ( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY generic_frame_handler    ( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY generic_menu_handler     ( HWND win, ULONG msg, MPARAM mp1, MPARAM mp2);

extern void
create_logger_window( void);

extern PFont gp_get_font   (  HPS ps, PFont font, Point resolution);
extern PFont view_get_font ( Handle view, PFont font);
extern Point frame2client( Handle self, Point p, Bool f2c);

extern Bool create_font_hash          ( void);
extern Bool destroy_font_hash         ( void);
extern Bool add_font_to_hash          ( const PFont key, const PFont font, int vectored, PFONTMETRICS fm, Bool addSizeEntry);
extern Bool get_font_from_hash        ( PFont font, int *vectored, PFONTMETRICS fm, Bool bySize);
extern void *create_fontid_hash       ( void);
extern void destroy_fontid_hash       ( void *hash);
extern int get_fontid_from_hash       ( void *hash, const PFont font, SIZEF *sz, int *vectored);
extern void add_fontid_to_hash        ( void *hash, int id, const PFont font, const SIZEF *sz, int vectored);

#endif