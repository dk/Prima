#ifndef _UNIX_GUTS_H_
#define _UNIX_GUTS_H_

#include "generic/config.h"
#if defined(HAS_GLIBC) && !defined(_GNU_SOURCE)
#	define _GNU_SOURCE
#endif
#define Drawable        XDrawable
#define Font            XFont
#define Window          XWindow
#undef  Bool
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#define Box _prima_Box
#undef TRUE
#undef FALSE
#include <X11/Xregion.h>
#undef Box
#define Box BoxRec
#undef Box
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
#include <X11/extensions/Xrender.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif
#if defined( HAVE_X11_EXTENSIONS_XSHM_H) && defined( HAVE_SYS_IPC_H) && defined( HAVE_SYS_SHM_H)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#define USE_MITSHM      1
#endif
#if defined(WITH_FREETYPE)
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_COLOR_H
#endif
#if defined(WITH_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#endif
#if defined(WITH_FREETYPE) && defined(WITH_FONTCONFIG)
#define USE_FONTQUERY
#endif
#if defined(HAVE_X11_XFT_XFT_H) && defined(HAVE_X11_EXTENSIONS_XRENDER_H)
#include <X11/Xft/Xft.h>
#  if XFT_MAJOR > 1 && FC_MAJOR > 1
#     define USE_XFT
#  endif
#  if XFT_VERSION < 20112
#     define NEED_X11_EXTENSIONS_XRENDER_H
#  endif
#endif
#ifdef HAVE_X11_EXTENSIONS_XRANDR_H
#include <X11/extensions/Xrandr.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_XCOMPOSITE_H
#include <X11/extensions/Xcomposite.h>
#endif
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
#include <X11/Xcursor/Xcursor.h>
#endif

#ifdef WITH_FRIBIDI
#include <fribidi.h>
#define ANALYSIS FriBidiLevel
#else
#define ANALYSIS Byte
#endif

#undef Font
#undef Drawable
#undef Bool
#undef Window
#define ComplexShape 0
#define XBool int
#undef Complex

#ifndef Button6
#define Button6		6
#endif
#ifndef Button7
#define Button7		7
#endif
#ifndef Button8
#define Button8		8
#endif
#ifndef Button9
#define Button9		9
#endif
#ifndef Button10
#define Button10	10
#endif
#ifndef Button11
#define Button11	11
#endif
#ifndef Button12
#define Button12	12
#endif
#ifndef Button13
#define Button13	13
#endif
#ifndef Button14
#define Button14	14
#endif
#ifndef Button15
#define Button15	15
#endif
#ifndef Button16
#define Button16	16
#endif
#ifndef Button6Mask
#define Button6Mask     (1<<13)
#endif
#ifndef Button7Mask
#define Button7Mask     (1<<14)
#endif

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>

#include "../guts.h"
#include "unix/queue.h"
#include "Widget.h"
#include "Image.h"
#include "img_conv.h"

#ifdef USE_MITSHM
/* at least some versions of XShm.h do not prototype XShmGetEventBase() */
extern int XShmGetEventBase( Display*);
#endif

typedef struct _PrimaXImage
{
	Bool shm;
	Bool can_free;
	int ref_cnt;
	void *data_alias;
	int bytes_per_line_alias;
	XImage *image;
#ifdef USE_MITSHM
	XShmSegmentInfo xmem;
#endif
} PrimaXImage;


#define CACHE_INVALID        0
#define CACHE_BITMAP         1
#define CACHE_PIXMAP         2
#define CACHE_LOW_RES        3
#define CACHE_LAYERED        4
#define CACHE_LAYERED_ALPHA  5
#define CACHE_A8             6

typedef struct {
	int type, data_mul, mask_mul, alpha_channel;
	PrimaXImage *image;
	PrimaXImage *icon;
} ImageCache;

typedef struct _RequestInformation
{
	unsigned long request;
	char *file;
	int line;
} RequestInformation, *PRequestInformation;

#define REQUEST_RING_SIZE 512

#define kbModKeyMask	0x00010000
#define kbCodeCharMask	0x00020000
#define kbVirtualMask	0x00040000
#define kbFunctionMask	0x00080000

typedef struct _FontFlags {
	unsigned height           : 1;
	unsigned width            : 1;
	unsigned style            : 1;
	unsigned pitch            : 1;
	unsigned name             : 1;
	unsigned encoding         : 1;
	unsigned size             : 1;
	unsigned family           : 1;
	unsigned vector           : 1;
	unsigned ascent           : 1;
	unsigned descent          : 1;
	unsigned weight           : 1;
	unsigned internalLeading  : 1;
	unsigned xDeviceRes       : 1;
	unsigned yDeviceRes       : 1;
	unsigned maximalWidth     : 1;
	unsigned underlines       : 1;
	/* extras */
	unsigned bad_vector       : 1;
	unsigned sloppy           : 1;
	unsigned disabled         : 1;
	unsigned funky            : 1;
	unsigned heights_cache    : 1;
	unsigned known            : 1;
} FontFlags;

typedef struct _FontInfo {
	FontFlags    flags;
	Font         font;
	char        *vecname;
	char        *xname;
	short int    name_offset;
	short int    info_offset;
	int          heights_cache[2];
} FontInfo, *PFontInfo;

typedef struct _RotatedFont {
	unsigned int  first1;
	unsigned int  first2;
	unsigned int  height;
	unsigned int  width;
	unsigned int  length;
	PrimaXImage **map;
	Point         shift;
	Point         dimension;
	Point         orgBox;
	Pixmap        arena;
	GC            arena_gc;
	Byte         *arena_bits;
	unsigned int  lineSize;
	unsigned long arenaSize;
	int           defaultChar1;
	int           defaultChar2;
	Fixed         matrix[4];
	Fixed         inverse[4];
	struct        RotatedFont *next;
} RotatedFont, *PRotatedFont;

#define MAX_CACHED_FONTS 512

typedef struct CachedFont {
	Byte         type;
	int          ref_cnt;
	int          lock_cnt;
	Font         font;

	/* corefont */
	FontFlags    flags;
	XFontStruct *fs;
	XFont        id;
	PRotatedFont rotated;

	int8_t       has_colors;
#ifdef USE_FONTQUERY
	FT_Face      ft_face;
	FcCharSet   *fc_charset;
#endif
#ifdef USE_XFT
	XftFont     *xft;
	XftFont     *xft_base;
#endif
#if defined(USE_FONTQUERY) || defined(USE_XFT)
	float        width_factor;
#endif
} CachedFont, *PCachedFont;


#define FONTKEY_CORE      1
#define FONTKEY_XFT       2
#define FONTKEY_FREETYPE  4
#define FONTKEY_DEFAULT   3

#pragma pack(1)
typedef struct _FontKey
{
	Byte     type;
	Bool     vector;
	int32_t  height;
	uint16_t width;
	uint16_t style;
	uint16_t pitch;
	int16_t  direction;
	int16_t  matrix[4];
	char     name[256];
} FontKey, *PFontKey;
#pragma pack()

#define MAX_HGS_SIZE 5

typedef struct
{
	int sp;
	int locked;
	int target;
	int xlfd[MAX_HGS_SIZE];
	int prima[MAX_HGS_SIZE];
} HeightGuessStack;

union _unix_sys_data;

#define FIRST_SYS_TIMER         ((Handle)11)
#define CURSOR_TIMER	        ((Handle)11)
#define MENU_TIMER	        ((Handle)12)
#define MENU_UNFOCUS_TIMER	((Handle)13)
#define LAST_SYS_TIMER          ((Handle)13)

#if defined(sgi) && !defined(__GNUC__)
/* multiple compilation and runtime errors otherwise. must be some alignment tricks */
#define COMPONENT_SYS_DATA_ALIGN  unsigned dummy : 20;
#else
#define COMPONENT_SYS_DATA_ALIGN
#endif

#define COMPONENT_SYS_DATA                                                  \
	Handle self;                                                        \
	struct {                                                            \
		unsigned application           : 1;                         \
		unsigned bitmap                : 1;                         \
		unsigned dbm                   : 1;                         \
		unsigned drawable              : 1;                         \
		unsigned icon                  : 1;                         \
		unsigned image                 : 1;                         \
		unsigned menu                  : 1;                         \
		unsigned pixmap                : 1;                         \
		unsigned popup                 : 1;                         \
		unsigned timer                 : 1;                         \
		unsigned widget                : 1;                         \
		unsigned window                : 1;                         \
		COMPONENT_SYS_DATA_ALIGN                                    \
	} type;                                                             \
	XrmQuarkList q_class_name;                                          \
	XrmQuarkList q_instance_name;                                       \
	int n_class_name;                                                   \
	int n_instance_name

typedef struct _timer_sys_data
{
	COMPONENT_SYS_DATA;
	int timeout;
	Handle who;
	struct _timer_sys_data *older;
	struct _timer_sys_data *younger;
	struct timeval when;
} TimerSysData, *PTimerSysData;

typedef struct
{
	Region region;
	int aperture;
} RegionSysData, *PRegionSysData;

struct  _drawable_sys_data;

#define VIRGIN_GC_MASK  (       GCBackground    \
			|       GCClipMask      \
			|       GCForeground    \
			|       GCFunction      \
			|       GCFillRule      \
			|       GCFillStyle      \
			|       GCTileStipXOrigin \
			|       GCTileStipYOrigin \
			|       GCLineStyle     \
			|       GCSubwindowMode )

typedef struct gc_list
{
	GC gc;
	TAILQ_ENTRY(gc_list) gc_link;
} GCList;

TAILQ_HEAD(gc_head,gc_list);

typedef struct pending_event
{
	Handle recipient;
	Event event;
	TAILQ_ENTRY(pending_event) peventq_link;
} PendingEvent;

typedef struct configure_event_pair
{
	TAILQ_ENTRY(configure_event_pair) link;
	int w, h, match;
} ConfigureEventPair;

#define COLOR_R(x) (((x)>>16)&0xFF)
#define COLOR_G(x) (((x)>>8)&0xFF)
#define COLOR_B(x) ((x)&0xFF)
#define COLOR_R16(x) (((x)>>8)&0xFF00)
#define COLOR_G16(x) ((x)&0xFF00)
#define COLOR_B16(x) (((x)<<8)&0xFF00)

#define LPAL_ADDR(i)  (i)>>2
#define LPAL_MASK(i)  (3<<(((i)&3)*2))
#define LPAL_SET(i,j) (((j)&3)<<(((i)&3)*2))
#define LPAL_GET(i,j) (((j)>>(((i)&3)*2))&3)

extern int
prima_lpal_get( Byte * palette, int index);

extern void
prima_lpal_set( Byte * palette, int index, int rank);

#define wlpal_get(widget,index)      prima_lpal_get(X(widget)->palette,index)
#define wlpal_set(widget,index,rank) prima_lpal_set(X(widget)->palette,index,rank)

/* Every color cell in guts.palette is assigned a rank. Its purpose
is to maintain reasonable sharing of available system colors.
See prima_palette_replace which preforms sharing. */
#define RANK_IMMUTABLE 4 /* Static color for 'cubic' filtering - or for Static visuals */
#define RANK_LOCKED    3 /* Colors used in Pixmaps - cannot participate in palette managing, therefore 'locked' */
#define RANK_PRIORITY  2 /* Colors explicitly set by Widget::set_palette */
#define RANK_NORMAL    1 /* Automatically allocated colors for drawing routines */
#define RANK_FREE      0 /* Colors not allocated by XAllocColor. Their values are not reliable */

#define RGB_COMPOSITE(R,G,B) ((((R)&0xFF)<<16)|(((G)&0xFF)<<8)|((B)&0xFF))

typedef struct {
	Byte  r, g, b;
	Byte  rank;
	Bool  touched;
	long  composite;
	List  users;
} MainColorEntry;

typedef struct {
	unsigned long  primary;
	unsigned long  secondary;
	Color          color;
	Byte           balance; /* 0-63 */
} Brush;

#define AI_FXA_RESOLUTION_X               0
#define AI_FXA_RESOLUTION_Y               1
#define AI_FXA_PIXEL_SIZE                 2
#define AI_FXA_SPACING                    3
#define AI_FXA_RELATIVE_WEIGHT            4
#define AI_FXA_FOUNDRY                    5
#define AI_FXA_AVERAGE_WIDTH              6
#define AI_FXA_CHARSET_REGISTRY           7
#define AI_FXA_CHARSET_ENCODING           8
#define AI_CREATE_EVENT                   9
#define AI_WM_DELETE_WINDOW              10
#define AI_WM_PROTOCOLS                  11
#define AI_WM_TAKE_FOCUS                 12
#define AI_NET_WM_STATE                  13
#define AI_NET_WM_STATE_SKIP_TASKBAR     14
#define AI_NET_WM_STATE_MAXIMIZED_VERT   15
#define AI_NET_WM_STATE_MAXIMIZED_HORZ   16
#define AI_NET_WM_NAME                   17
#define AI_NET_WM_ICON_NAME              18
#define AI_UTF8_STRING                   19
#define AI_TARGETS                       20
#define AI_INCR                          21
#define AI_PIXEL                         22
#define AI_FOREGROUND                    23
#define AI_BACKGROUND                    24
#define AI_MOTIF_WM_HINTS                25
#define AI_NET_WM_STATE_MODAL            26
#define AI_NET_SUPPORTED                 27
#define AI_NET_WM_STATE_MAXIMIZED_HORIZ  28
#define AI_UTF8_MIME                     29
#define AI_NET_WM_STATE_STAYS_ON_TOP     30
#define AI_NET_CURRENT_DESKTOP           31
#define AI_NET_WORKAREA                  32
#define AI_NET_WM_STATE_ABOVE            33
#define AI_XdndEnter                     34
#define AI_XdndPosition                  35
#define AI_XdndStatus                    36
#define AI_XdndTypeList                  37
#define AI_XdndActionCopy                38
#define AI_XdndDrop                      39
#define AI_XdndLeave                     40
#define AI_XdndFinished                  41
#define AI_XdndSelection                 42
#define AI_XdndProxy                     43
#define AI_XdndAware                     44
#define AI_XdndActionMove                45
#define AI_XdndActionLink                46
#define AI_XdndActionAsk                 47
#define AI_XdndActionPrivate             48
#define AI_XdndActionList                49
#define AI_XdndActionDescription         50
#define AI_PLAINTEXT_MIME                51
#define AI_NET_WM_ICON                   52
#define AI_NET_FRAME_EXTENTS             53
#define AI_NET_WM_STATE_FULLSCREEN       54
#define AI_count                         55

#define FXA_RESOLUTION_X            pguts->atoms[AI_FXA_RESOLUTION_X           ]
#define FXA_RESOLUTION_Y            pguts->atoms[AI_FXA_RESOLUTION_Y           ]
#define FXA_POINT_SIZE              XA_POINT_SIZE
#define FXA_PIXEL_SIZE              pguts->atoms[AI_FXA_PIXEL_SIZE             ]
#define FXA_SPACING                 pguts->atoms[AI_FXA_SPACING                ]
#define FXA_WEIGHT                  XA_WEIGHT
#define FXA_RELATIVE_WEIGHT         pguts->atoms[AI_FXA_RELATIVE_WEIGHT        ]
#define FXA_FOUNDRY                 pguts->atoms[AI_FXA_FOUNDRY                ]
#define FXA_FAMILY_NAME             XA_FAMILY_NAME
#define FXA_AVERAGE_WIDTH           pguts->atoms[AI_FXA_AVERAGE_WIDTH          ]
#define FXA_CHARSET_REGISTRY        pguts->atoms[AI_FXA_CHARSET_REGISTRY       ]
#define FXA_CHARSET_ENCODING        pguts->atoms[AI_FXA_CHARSET_ENCODING       ]
#define FXA_CAP_HEIGHT              XA_CAP_HEIGHT
#define CREATE_EVENT                pguts->atoms[AI_CREATE_EVENT               ]
#define WM_DELETE_WINDOW            pguts->atoms[AI_WM_DELETE_WINDOW           ]
#define WM_PROTOCOLS                pguts->atoms[AI_WM_PROTOCOLS               ]
#define WM_TAKE_FOCUS               pguts->atoms[AI_WM_TAKE_FOCUS              ]
#define NET_WM_STATE                pguts->atoms[ AI_NET_WM_STATE              ]
#define NET_WM_STATE_SKIP_TASKBAR   pguts->atoms[AI_NET_WM_STATE_SKIP_TASKBAR  ]
#define NET_WM_STATE_MAXIMIZED_VERT pguts->atoms[AI_NET_WM_STATE_MAXIMIZED_VERT]
#define NET_WM_STATE_MAXIMIZED_HORZ pguts->atoms[(pguts-> net_wm_maximize_HORZ_vs_HORIZ > 0) ? pguts-> net_wm_maximize_HORZ_vs_HORIZ : AI_NET_WM_STATE_MAXIMIZED_HORZ]
#define NET_WM_NAME                 pguts->atoms[AI_NET_WM_NAME                ]
#define NET_WM_ICON_NAME            pguts->atoms[AI_NET_WM_ICON_NAME           ]
#define UTF8_STRING                 pguts->atoms[AI_UTF8_STRING                ]
#define CF_TARGETS                  pguts->atoms[AI_TARGETS                    ]
#define XA_INCR                     pguts->atoms[AI_INCR                       ]
#define CF_PIXEL                    pguts->atoms[AI_PIXEL                      ]
#define CF_FOREGROUND               pguts->atoms[AI_FOREGROUND                 ]
#define CF_BACKGROUND               pguts->atoms[AI_BACKGROUND                 ]
#define XA_MOTIF_WM_HINTS           pguts->atoms[AI_MOTIF_WM_HINTS             ]
#define NET_WM_STATE_MODAL          pguts->atoms[AI_NET_WM_STATE_MODAL         ]
#define NET_SUPPORTED               pguts->atoms[AI_NET_SUPPORTED              ]
#define UTF8_MIME                   pguts->atoms[AI_UTF8_MIME                  ]
#define NET_WM_STATE_STAYS_ON_TOP   pguts->atoms[AI_NET_WM_STATE_STAYS_ON_TOP  ]
#define NET_CURRENT_DESKTOP         pguts->atoms[AI_NET_CURRENT_DESKTOP        ]
#define NET_WORKAREA                pguts->atoms[AI_NET_WORKAREA               ]
#define NET_WM_STATE_ABOVE          pguts->atoms[AI_NET_WM_STATE_ABOVE         ]
#define XdndEnter                   pguts->atoms[AI_XdndEnter                  ]
#define XdndPosition                pguts->atoms[AI_XdndPosition               ]
#define XdndStatus                  pguts->atoms[AI_XdndStatus                 ]
#define XdndTypeList                pguts->atoms[AI_XdndTypeList               ]
#define XdndActionCopy              pguts->atoms[AI_XdndActionCopy             ]
#define XdndDrop                    pguts->atoms[AI_XdndDrop                   ]
#define XdndLeave                   pguts->atoms[AI_XdndLeave                  ]
#define XdndFinished                pguts->atoms[AI_XdndFinished               ]
#define XdndSelection               pguts->atoms[AI_XdndSelection              ]
#define XdndProxy                   pguts->atoms[AI_XdndProxy                  ]
#define XdndAware                   pguts->atoms[AI_XdndAware                  ]
#define XdndActionMove              pguts->atoms[AI_XdndActionMove             ]
#define XdndActionLink              pguts->atoms[AI_XdndActionLink             ]
#define XdndActionAsk               pguts->atoms[AI_XdndActionAsk              ]
#define XdndActionPrivate           pguts->atoms[AI_XdndActionPrivate          ]
#define XdndActionList              pguts->atoms[AI_XdndActionList             ]
#define XdndActionDescription       pguts->atoms[AI_XdndActionDescription      ]
#define PLAINTEXT_MIME              pguts->atoms[AI_PLAINTEXT_MIME             ]
#define NET_WM_ICON                 pguts->atoms[AI_NET_WM_ICON                ]
#define NET_FRAME_EXTENTS           pguts->atoms[AI_NET_FRAME_EXTENTS          ]
#define NET_WM_STATE_FULLSCREEN     pguts->atoms[AI_NET_WM_STATE_FULLSCREEN    ]

#define DEBUG_FONTS 0x01
#define DEBUG_CLIP  0x02
#define DEBUG_EVENT 0x04
#define DEBUG_MISC  0x08
#define DEBUG_COLOR 0x10
#define DEBUG_XRDB  0x20
#define DEBUG_ALL   0x3f
#define _debug prima_debug
extern void
prima_debug( const char *format, ...);
extern void
prima_debug2( const char *prefix, const char *format, ...);
extern void
prima_if_debug( const char filter, const char *format, ...);
#define Fdebug if (pguts->debug & DEBUG_FONTS) _debug
#define Cdebug if (pguts->debug & DEBUG_CLIP) _debug
#define Edebug if (pguts->debug & DEBUG_EVENT) _debug
#define Mdebug if (pguts->debug & DEBUG_MISC) _debug
#define Pdebug if (pguts->debug & DEBUG_COLOR) _debug
#define Xdebug if (pguts->debug & DEBUG_XRDB) _debug
#define _F_DEBUG_PITCH(x) ((x==fpDefault)?"default":(x==fpFixed?"fixed":"variable"))
#define _F_DEBUG_STYLE(x) prima_font_debug_style(x)

typedef struct
{
	unsigned int red_shift, green_shift, blue_shift, alpha_shift;
	unsigned int red_range, green_range, blue_range, alpha_range;
	unsigned int red_mask,  green_mask,  blue_mask,  alpha_mask;
} RGBABitDescription, *PRGBABitDescription;

#define DEV_R(descr,val) (((((uint32_t)(val)) << (descr)->red_range  ) >> 8) << (descr)->red_shift)
#define DEV_G(descr,val) (((((uint32_t)(val)) << (descr)->green_range) >> 8) << (descr)->green_shift)
#define DEV_B(descr,val) (((((uint32_t)(val)) << (descr)->blue_range ) >> 8) << (descr)->blue_shift )
#define DEV_A(descr,val) (((((uint32_t)(val)) << (descr)->alpha_range) >> 8) << (descr)->alpha_shift)
#define DEV_RGB(descr,r,g,b)        ( DEV_R(descr,r) | DEV_G(descr,g) | DEV_B(descr,b) )
#define DEV_RGBA(descr,r,g,b,a)     ( DEV_RGB(descr,r,g,b) | DEV_A(descr,a) )
#define COLOR2DEV_RGB(descr,color)  ( DEV_RGB(descr,COLOR_R(color),COLOR_G(color),COLOR_B(color) ) )
#define PALETTE2DEV_RGB(descr,p)  ( DEV_RGB(descr,p.r,p.g,p.b) )
#define COLOR2DEV_RGBA(descr,color,alpha) ( DEV_RGBA(descr,COLOR_R(color),COLOR_G(color),COLOR_B(color),alpha ) )

typedef struct
{
	int status; /* -1 not ok to use, 0 not initialized, 1, ok to use */
	Point hot_spot;
	Cursor cursor;
	Pixmap xor;
	Pixmap and;
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	XcursorImage * xcursor;
#endif
} CustomPointer;

#define MAX_UNICODE_HEX_LENGTH 6

typedef struct _UnixGuts
{
	/* Event management */
	Time                         click_time_frame;
	Time                         double_click_time_frame;
	PHash                        clipboards;
	PHash                        clipboard_xfers;
	Atom *                       clipboard_formats;
	int                          clipboard_formats_count;
	long                         clipboard_event_timeout;
	fd_set                       excpt_set;
	PList                        files;
	long                         handled_events;
	XButtonEvent                 last_button_event;
	XButtonEvent                 last_click;
	unsigned int                 last_mouseclick_number;
	Time                         last_time;
	int (*                       main_error_handler   )(Display*,XErrorEvent*);
	int                          max_fd;
	int                          modal_count;
	TAILQ_HEAD(,pending_event)   peventq;
	fd_set                       read_set;
	long                         total_events;
	long                         skipped_events;
	long                         unhandled_events;
	fd_set                       write_set;
	/* Graphics */
	struct gc_head               bitmap_gc_pool;
	struct gc_head               screen_gc_pool;
	struct gc_head               argb_gc_pool;
	GC                                   menugc;
	TAILQ_HEAD(,_drawable_sys_data)      paintq;
	TAILQ_HEAD(,_drawable_sys_data)      exposeq;
	PHash                                ximages;
	/* Font management */
	PHash                        font_hash;
	PHash                        fc_mismatch;
	PFontInfo                    font_info;
	char                       **font_names;
	int                          n_fonts;
	XFontStruct                 *pointer_font;
	Bool                         default_font_ok;
	Font                         default_font;
	Font                         default_menu_font;
	Font                         default_widget_font;
	Font                         default_msg_font;
	Font                         default_caption_font;
	int                          no_scaled_fonts;
	/* Resource management */
	XrmDatabase                  db;
	XrmQuark                     qBlinkinvisibletime;
	XrmQuark                     qblinkinvisibletime;
	XrmQuark                     qBlinkvisibletime;
	XrmQuark                     qblinkvisibletime;
	XrmQuark                     qClicktimeframe;
	XrmQuark                     qclicktimeframe;
	XrmQuark                     qDoubleclicktimeframe;
	XrmQuark                     qdoubleclicktimeframe;
	XrmQuark                     qString;
	XrmQuark                     qWheeldown;
	XrmQuark                     qwheeldown;
	XrmQuark                     qWheelup;
	XrmQuark                     qwheelup;
	XrmQuark                     qSubmenudelay;
	XrmQuark                     qsubmenudelay;
	XrmQuark                     qScrollfirst;
	XrmQuark                     qscrollfirst;
	XrmQuark                     qScrollnext;
	XrmQuark                     qscrollnext;
	/* Timers & cursors */
	unsigned int                 cursor_height;
	Point                        cursor_pixmap_size;
	Pixmap                       cursor_save;
	Bool                         cursor_shown;
	unsigned int                 cursor_width;
	Pixmap                       cursor_xor;
	Bool                         cursor_layered;
	Bool                         insert;
	int                          invisible_timeout;
	struct _timer_sys_data      *oldest;
	int                          visible_timeout;
	/* Window management */
	Handle                       focused;
	PHash                        menu_windows;
	PHash                        windows;
	/* XServer info */
	int                          bit_order;
	unsigned char                buttons_map[256];
	int                          byte_order;
	int                          connection;
	int                          depth;
	Display                     *display;
	int                          machine_byte_order;
	int                          idepth; /* image depth; can be 32 if depth == 24 */
	int                          qdepth; /* image depth for querying */
	int                          argb_depth; /* image depth for xrender RGBA */
	struct {
		long XDrawArcs;
		long XDrawLines;
		long XDrawRectangles;
		long XDrawSegments;
		long XFillArcs;
		long XFillPolygon;
		long XFillRectangles;
		long NetWMIcon;
		long request_length;
		long extended_request_length;
	}                            limits;
	Cursor                       null_pointer;
	int                          pointer_invisible_count; /* 0 is visible, > 0 is not, can't be <0 */
	int                          mouse_buttons;
	int                          mouse_wheel_down;
	int                          mouse_wheel_up;
	Point                        resolution;
	RequestInformation           ri[REQUEST_RING_SIZE];
	int                          ri_head;
	int                          ri_tail;
	int                          screen_number;
	Bool                         shape_extension;
	int                          shape_event;
	int                          shape_error;
	Bool                         shared_image_extension;
	int                          shared_image_completion_event;
	Bool                         xshmattach_failed;
	int                          use_xft;
	Bool                         xft_priority;
	Bool                         xft_disable_large_fonts;
	int                          xft_xrender_major_opcode;
	Bool                         randr_extension;
	Bool                         render_extension;
	Bool                         render_supports_argb32;
	Bool                         render_matrix_enabled;
	Bool                         composite_extension;
	int                          composite_opcode;
	Bool                         composite_error_triggered;
	struct MsgDlg               *message_boxes;
	XWindow                      grab_redirect;
	Handle                       grab_widget;
	Point                        grab_translate_mouse;
	Handle                       grab_confine;
	int                          scroll_first;
	int                          scroll_next;
	Handle                       currentMenu;
	Time                         currentFocusTime;
	Handle                       unfocusedMenu;
	int                          menu_timeout;
	XWindow                      root;
	XVisualInfo                  visual;
	int                          visualClass;
	XVisualInfo                  argb_visual;
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	XRenderPictFormat *          xrender_display_format;
	XRenderPictFormat *          xrender_argb32_format;
	XRenderPictFormat *          xrender_a8_format;
	XRenderPictFormat *          xrender_a1_format;
#endif
	Bool                         xrender_pen_dirty;
	MainColorEntry *             palette;
	int                          mappingPlace[256];
	unsigned long                monochromeMap[2];
	int                          palSize;
	int                          localPalSize;
	int *                        systemColorMap;
	int                          systemColorMapSize;
	int                          colorCubeRib;
	Bool                         dynamicColors;
	Bool                         grayScale;
	Bool                         useDithering;
	Bool                         privateColormap;
	Colormap                     defaultColormap;
	Colormap                     argbColormap;
	FillPattern *                ditherPatterns;
	Point                        displaySize;
	long                         wm_event_timeout;
	RGBABitDescription           screen_bits;
	RGBABitDescription           argb_bits;
	Point                        ellipseDivergence;
	int                          appLock;
	XGCValues                    cursor_gcv;
	TimerSysData                 sys_timers[ LAST_SYS_TIMER - FIRST_SYS_TIMER + 1];
	Bool                         applicationClose;
	char                         locale[32];
	XFontStruct *                font_abc_nil_hack;
	Atom                         atoms[AI_count];
	XTextProperty                hostname;
	unsigned int			debug;
	Bool                         icccm_only;
	Bool                         net_wm_maximization;
	int                          net_wm_maximize_HORZ_vs_HORIZ;
	Bool                         net_wm_fullscreen;
	int                          use_gtk;
	int                          use_quartz;
	Bool                         use_harfbuzz;
	Bool                         use_freetype;
	Bool                         is_xwayland;
	/* DND: Common */
	Handle                       xdnd_clipboard;
	int                          xdnd_disabled;
	/* DND: Receiver */
	Handle                       xdndr_receiver, xdndr_widget, xdndr_last_target;
	XWindow                      xdndr_source;
	long                         xdndr_timestamp;
	int                          xdndr_version, xdndr_last_action, xdndr_action_list_cache;
	Bool                         xdndr_last_drop_response;
	Box                          xdndr_suppress_events_within; /* in prima coordinates */
	/* DND: Sender */
	Handle                       xdnds_widget;
	XWindow                      xdnds_sender, xdnds_target;
	int                          xdnds_version, xdnds_last_action, xdnds_last_action_response;
	Bool                         xdnds_last_drop_response, xdnds_escape_key, xdnds_finished;
	Bool                         xdnds_default_pointers;
	Box                          xdnds_suppress_events_within; /* in root coordinates */

	CustomPointer                xdnd_pointers[5]; /* none,copy,link,move,ask */

	int                          unicode_hex_input_flags;
	char                         unicode_hex_input_buffer[MAX_UNICODE_HEX_LENGTH + 1];

	Bool                         application_stop_signal;
	int                          debug_indent;

	Bool                         use_xim;
	XIM                          xim;
	XIC                          xic;
	char *                       xic_buffer;
	unsigned int                 xic_bufsize;

#define MAX_ROTATED_FONT_CACHE_SIZE (1024*1024)
	unsigned long int            rotated_font_cache_size;
	int                          try_fc_monospace_emulation_by_name;
	int                          force_fc_monospace_emulation;

#define SKIP_GTK_FONT_DEFAULT 1
#define SKIP_GTK_FONT_MSG     2
#define SKIP_GTK_FONT_MENU    4
#define SKIP_GTK_FONT_CAPTION 8
#define SKIP_GTK_FONT_WIDGET 16
	int                          skip_gtk_font;
	Bool                         x11_local;
	Bool                         is_darwin;
} UnixGuts;

extern UnixGuts  guts;
extern UnixGuts* pguts;

#define XCHECKPOINT						\
	if ( DISP ) {							\
		pguts-> ri[ pguts-> ri_head]. line = __LINE__;			\
		pguts-> ri[ pguts-> ri_head]. file = __FILE__;			\
		pguts-> ri[ pguts-> ri_head]. request = NextRequest(DISP);	\
		pguts-> ri_head++;						\
		if ( pguts-> ri_head >= REQUEST_RING_SIZE)			\
			pguts-> ri_head = 0;					\
		if ( pguts-> ri_tail == pguts-> ri_head) {			\
			pguts-> ri_tail++;					\
			if ( pguts-> ri_tail >= REQUEST_RING_SIZE)		\
				pguts-> ri_tail = 0;				\
		}								\
	}

#define APC_BAD_SIZE INT_MAX
#define APC_BAD_ORIGIN INT_MAX


#define XT_IS_APPLICATION(x)    ((x)->type.application)
#define XT_IS_BITMAP(x)         ((x)->type.bitmap)
#define XT_IS_DBM(x)            ((x)->type.dbm)
#define XT_IS_DRAWABLE(x)       ((x)->type.drawable)
#define XT_IS_ICON(x)           ((x)->type.icon)
#define XT_IS_IMAGE(x)          ((x)->type.image)
#define XT_IS_MENU(x)           ((x)->type.menu)
#define XT_IS_PIXMAP(x)         ((x)->type.pixmap)
#define XT_IS_POPUP(x)          ((x)->type.popup)
#define XT_IS_TIMER(x)          ((x)->type.timer)
#define XT_IS_WIDGET(x)         ((x)->type.widget)
#define XT_IS_WINDOW(x)         ((x)->type.window)

typedef struct _drawable_sys_data
{
	COMPONENT_SYS_DATA;
	XDrawable udrawable;
	XDrawable gdrawable;
	XWindow parent;
	Point origin, size, bsize;
	Point btransform;
	Point ackOrigin, ackSize, ackFrameSize;
	int menuHeight;
	int menuColorImmunity;
	Point decorationSize;
	Handle owner;  /* The real one */
	XWindow real_parent; /* top levels */
	XWindow parentHandle; /* top levels */
	XWindow above;
	Rect zoomRect;
	XGCValues gcv;
	GC gc;
	GCList *gcl;
	Brush fore, back;
	Color saved_fore, saved_back;
	ColorSet colors;
	Region invalid_region, paint_region, current_region, cached_region;
	XRectangle clip_rect;
	FillPattern fill_pattern;
	Point fill_pattern_offset;
	int fill_mode;
	Pixmap fp_tile, fp_stipple, fp_render_pen;
	XID fp_render_picture;
	Matrix matrix;
	int rop;
	int rop2;
	int alpha;
	int line_style;
	unsigned char *dashes;
	int ndashes;
	Point clip_mask_extent, shape_extent, shape_offset;
	PCachedFont font;
	Point cursor_pos;
	Point cursor_size;
	CustomPointer user_pointer;
	int pointer_id;
	Cursor actual_pointer;
	void * recreateData;
	XWindow client;
	struct {
		unsigned antialias                : 1;
		unsigned base_line                : 1;
		unsigned brush_fore               : 1;
		unsigned brush_back               : 1;
		unsigned brush_null_hatch         : 1;
		unsigned clip_by_children	  : 1;
		unsigned clip_owner	          : 1;
		unsigned configured               : 1;
		unsigned cursor_visible		  : 1;
		unsigned dnd_aware                : 1;
		unsigned drop_target              : 1;
		unsigned enabled               	  : 1;
		unsigned expose_pending           : 1;
		unsigned exposed                  : 1;
		unsigned falsely_hidden           : 1;
		unsigned first_click              : 1;
		unsigned force_flush              : 1;
		unsigned fullscreen               : 1;
		unsigned fullscreen_emulated      : 1;
		unsigned grab                     : 1;
		unsigned has_icon                 : 1;
		unsigned layered                  : 1;
		unsigned layered_requested        : 1;
		unsigned iconic                   : 1;
		unsigned mapped                   : 1;
		unsigned matrix_used              : 1;
		unsigned modal                    : 1;
		unsigned kill_current_region      : 1;
		unsigned opaque                   : 1;
		unsigned on_top                   : 1;
		unsigned paint                    : 1;
		unsigned paint_pending            : 1;
		unsigned pointer_obscured         : 1;
		unsigned position_determined      : 1;
		unsigned sizeable                 : 1;
		unsigned sizemax_set              : 1;
		unsigned sync_paint               : 1;
		unsigned task_listed              : 1;
		unsigned title_utf8               : 1;
		unsigned transparent              : 1;
		unsigned transparent_busy         : 1;
		unsigned want_visible             : 1;
		unsigned withdrawn                : 1;
		unsigned zoomed                   : 1;
		unsigned xft_clip                 : 1;
		unsigned xrender_sync             : 1;
	} flags;
	ImageCache image_cache;
	Handle preexec_focus;
	TAILQ_ENTRY(_drawable_sys_data) paintq_link;
	TAILQ_ENTRY(_drawable_sys_data) exposeq_link;
	TAILQ_HEAD(,configure_event_pair)    configure_pairs;
	Byte * palette;
	int borderIcons;
	XVisualInfo * visual;
	Colormap colormap;
	PList gc_stack;
#ifdef USE_FONTQUERY
	uint32_t * fc_map8;
	Matrix     fc_font_matrix;
#endif
#ifdef USE_XFT
	XftDraw  * xft_drawable;
	XftDraw  * xft_shadow_drawable;
	Point      xft_shadow_extentions;
	Pixmap     xft_shadow_pixmap;
	GC         xft_shadow_gc;
#endif
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	Picture    argb_picture;
#endif
} DrawableSysData, *PDrawableSysData;

#define XF_ENABLED(x)   ((x)->flags.enabled)
#define XF_IN_PAINT(x)  ((x)->flags.paint)
#define XF_LAYERED(x)  ((x)->flags.layered)
#define XFLUSH          if (XX->flags.force_flush) XFlush(DISP)
#define XRENDER_SYNC    if (XX->flags.xrender_sync) { XFlush(DISP); XX->flags.xrender_sync = false; }
#define XRENDER_SYNC_NEEDED XX->flags.xrender_sync = true

typedef struct _PaintState
{
	Bool in_paint;
	struct {
		Brush fore, back;
		GC gc;
		GCList *gcl;
		struct gc_head* gc_pool;
		Region region;
		Pixmap tile, stipple;
		Bool kill_tile, kill_stipple;
	} paint;
	struct {
		Color fore, back;
		XGCValues gcv;
	} nonpaint;
	int alpha, fill_mode, n_dashes, rop, rop2;
	Bool antialias, text_opaque, text_baseline, null_hatch;
	Point fill_pattern_offset;
	Handle fill_image;
	FillPattern fill_pattern;
	Font font;
	unsigned char *dashes;

	unsigned int user_data_size;
	GCStorageFunction * user_destructor;
	void *user_data, *user_context;
	char user_data_buf[1]; /* this needs to be the last */
} PaintState, *PPaintState;

#define MenuTimerMessage   1021

#define MENU_ITEM_GAP 4
#define MENU_XOFFSET 5
#define MENU_CHECK_XOFFSET 10

typedef struct
{
	Bool is_mono;
	Bool use_stippling;
	Handle xor;
	Handle and;
	long fore, back;
} MenuBitmap, *PMenuBitmap;

typedef struct _menu_item
{
	int          x;
	int          y;
	int          width;
	int          height;
	int          accel_width;
	int          icon_width;
	MenuBitmap   bitmap, icon;
} UnixMenuItem, *PUnixMenuItem;

typedef struct _menu_window
{
	Handle               self;
	XWindow              w;
	Point                sz;
	Point                pos;
	PMenuItemReg         m;
	int                  num;
	PUnixMenuItem        um;
	struct _menu_window *next;
	struct _menu_window *prev;
	int                  selected;
	int                  right;
	int                  last;
	int                  first;
	Region               rgn;
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	Picture              argb_picture;
#endif
} MenuWindow, *PMenuWindow;

#define MENU_PALETTE_SIZE (ciMaxId + 2)
typedef struct _menu_sys_data
{
	COMPONENT_SYS_DATA;
	Bool                 paint_pending;
	PMenuWindow          w;
	MenuWindow           wstatic;
	PCachedFont          font;
	int                  guillemots;
	Bool                 layered;
	unsigned long        c[MENU_PALETTE_SIZE];
	unsigned long        argb_c[MENU_PALETTE_SIZE];
	Color                rgb[MENU_PALETTE_SIZE];
	XWindow              focus;
	PMenuWindow          focused;
} MenuSysData, *PMenuSysData;

#define cfTargets    (cfCustom  + 0)
#define cfCOUNT      (cfTargets + 1)
/* XXX not implemented
#define cfPalette    (cfCustom  + 1)
#define cfForeground (cfCustom  + 2)
#define cfBackground (cfCustom  + 3)
#define cfCOUNT      (cfCustom  + 4)
*/

#define CF_32        (sizeof(long)*8)        /* 32-bit properties are hacky */

typedef struct {
	IV size;
	unsigned char * data;
	Handle image;
	Atom name;
	Bool immediate;
} ClipboardDataItem, *PClipboardDataItem;

typedef struct _clipboard_sys_data
{
	COMPONENT_SYS_DATA;
	Atom                 selection;
	Atom                 target;
	Bool                 opened;
	Bool                 inside_event;
	Bool                 need_write;
	Handle               selection_owner;
	PClipboardDataItem   external;
	PClipboardDataItem   internal;
	PList                xfers;
	Bool                 xdnd_sending, xdnd_receiving;
} ClipboardSysData, *PClipboardSysData;

typedef struct
{
	Handle               self;
	unsigned char      * data;
	unsigned long        size;
	unsigned int         blocks;
	unsigned int         offset;
	Bool                 data_detached;
	Bool                 data_master;
	long                 id;
	XWindow              requestor;
	Atom                 property;
	Atom                 target;
	int                  format;
	struct timeval       time;
	unsigned long        delay;
} ClipboardXfer;

typedef unsigned char ClipboardXferKey[sizeof(XWindow)+sizeof(Atom)];

#define CLIPBOARD_XFER_KEY(key,window,property) \
	memcpy(key,&window,sizeof(XWindow));\
	memcpy(((unsigned char*)key) + sizeof(XWindow),&property,sizeof(Atom))

typedef union _unix_sys_data
{
	ClipboardSysData             clipboard;
	struct {
		COMPONENT_SYS_DATA;
	}                            component;
	DrawableSysData              drawable;
	MenuSysData                  menu;
	TimerSysData                 timer;
	RegionSysData                region;
} UnixSysData, *PUnixSysData;

#define DISP		(pguts-> display)
#define SCREEN		(pguts-> screen_number)
#define VISUAL          (pguts-> visual. visual)
#define DRIN		pguts-> display, pguts-> screen_number
#define X_WINDOW	(PComponent(self)-> handle)
#define X(obj)		((PDrawableSysData)(PComponent((obj))-> sysData))
#define DEFXX		PDrawableSysData selfxx = (self == NULL_HANDLE ? NULL : X(self))
#define M(obj)		((PMenuSysData)(PComponent((obj))-> sysData))
#define DEFMM           PMenuSysData selfxx = ((PMenuSysData)(PComponent((self))-> sysData))
#define C(obj)		((PClipboardSysData)(PComponent((obj))-> sysData))
#define DEFCC		PClipboardSysData selfxx = C(self)
#define XX		selfxx
#define WHEEL_DELTA	120
#define GET_RGBA_DESCRIPTION X(self)->flags.layered ? &guts. argb_bits : &guts. screen_bits
#define GET_REGION(obj) (&((PUnixSysData)(PComponent((obj))-> sysData))->region)

typedef U8 ColorComponent;

extern UnixGuts *
prima_unix_guts(void);

extern Handle
prima_xw2h( XWindow win);

extern void
prima_handle_event( XEvent *ev, XEvent *next_event);

extern void
prima_handle_menu_event( XEvent *ev, XWindow win, Handle self);

extern void
prima_handle_selection_event( XEvent *ev, XWindow win, Handle self);

extern void
prima_save_xerror_event( XErrorEvent *xr);

extern void
prima_restore_xerror_event( XErrorEvent *xr);

extern struct gc_head*
prima_get_gc( PDrawableSysData);

extern void
prima_get_fill_pattern_offsets( Handle self, int * x, int * y );

extern void
prima_rebuild_watchers( void);

extern void
prima_release_gc( PDrawableSysData);

extern Bool
prima_init_clipboard_subsystem( char * error_buf);

#define ROUND_DIRECTION 1000.0
#define IS_ZERO(a)   ((int)(a*ROUND_DIRECTION)==0)
#define ROUND2INT(a) ((int)(a*ROUND_DIRECTION))
#define ROUGHLY(a)   (ROUND2INT(a)/ROUND_DIRECTION)


extern Bool
prima_font_init_subsystem(void);

extern Bool
prima_font_init_x11( char * error_buf);

extern Bool
prima_font_subsystem_set_option( char *, char *);

extern void
prima_font_cleanup_subsystem( void);

extern const char *
prima_font_debug_style(int style);

extern void
prima_font_init_try_height( HeightGuessStack * p, int target, int firstMove );

extern int
prima_font_try_height( HeightGuessStack * p, int height);

extern PCachedFont
prima_font_pick( PFont source, Matrix matrix, PFont dest, unsigned int selection );

extern Point *
prima_get_text_box( Handle self, Point ovx, int advance );

extern Bool
prima_corefont_init( char *error_buf);

extern void
prima_corefont_done( void);

extern PFont
prima_corefont_fonts( Handle self, const char *facename, const char * encoding, int *retCount);

extern Bool
prima_corefont_pick_default_font_with_encoding(void);

extern Bool
prima_corefont_set_option( char * option, char * value);

extern Byte*
prima_corefont_get_glyph_bitmap( Handle self, uint16_t index, Bool mono, PPoint offset, PPoint size, int *advance);

extern PFontABC
prima_corefont_xfont2abc( XFontStruct * fs, int firstChar, int lastChar);

extern PFontABC
prima_corefont_xfont2def( Handle self, int first, int last);

extern void
prima_corefont_pp2font( char * ppFontNameSize, PFont font);

extern PCachedFont
prima_corefont_match(PFont match, Bool by_size, PCachedFont kf);

extern Point
prima_corefont_get_text_overhangs( XFontStruct *fs, const char *text, int len, int flags);

extern Bool
prima_corefont_encoding( char * encoding);

extern void
prima_corefont_encodings(PHash hash);

extern void
prima_corefont_free_cached_font( PCachedFont f);

extern void
prima_corefont_build_key( PFontKey key, PFont f, Bool bySize);

extern void
prima_fill_default_font( Font * font );

extern Bool
prima_init_color_subsystem( char * error_buf);

extern Bool
prima_init_xrender_subsystem( char * error_buf, Bool disable_argb32);

extern Bool
prima_find_color_mask_range( unsigned long mask, unsigned int * shift, unsigned int * range);

extern Bool
prima_color_subsystem_set_option( char *, char *);

extern void
prima_done_color_subsystem( void);

extern void
prima_done_xrender_subsystem( void);

extern int
prima_color_find( Handle self, long color, int maxDiff, int * diff, int maxRank);

extern Bool
prima_palette_replace( Handle self, Bool fast);

#define COLORHINT_NONE  0
#define COLORHINT_BLACK 1
#define COLORHINT_WHITE 2

#define LOGCOLOR_BLACK 0
#define LOGCOLOR_WHITE (pguts->palSize?(pguts->palSize-1):0xffffffff)

extern Color
prima_map_color( Color color, int * hint);

extern unsigned long
prima_allocate_color( Handle self, Color color, Brush * brush);

extern void
prima_palette_free( Handle self, Bool priority);

extern Bool
prima_palette_alloc( Handle self);

extern Bool
prima_color_add_ref( Handle self, int index, int rank);

extern int
prima_color_sync( void);

extern Pixmap
prima_get_hatch( FillPattern * fp);

extern void
prima_copy_1bit_ximage( unsigned char *data, XImage *i, Bool to_ximage);

extern void
prima_mirror_bytes( unsigned char *data, int dataSize);

extern Bool
prima_create_icon_pixmaps( Handle bw_icon, Pixmap *xor, Pixmap *and);

extern ImageCache*
prima_image_cache( PImage img, int type, int data_mul, int mask_mul, int alpha_channel);

#define SIMPLE_IMAGE_CACHE(img,type) prima_image_cache((PImage)(img),type,255,255,0)
#define IMGDUP_X(d)          X(IMGDUP_H(d))

extern Bool
prima_put_ximage( XDrawable win, GC gc, PrimaXImage *i,
		int src_x, int src_y, int dst_x, int dst_y,
		int width, int height);

extern Bool
prima_query_image( Handle self, XImage * image);

extern Bool
prima_std_query_image( Handle self, Pixmap px);

extern Pixmap
prima_std_pixmap( Handle self, int type);

#ifdef HAVE_X11_EXTENSIONS_XRENDER_H

#define DELETE_ARGB_PICTURE(x) if ((x)) { \
	XRenderFreePicture( DISP, x);     \
	x = 0;                            \
}

#define CLIP_ARGB_PICTURE(x,region) if ((x)) \
	XRenderSetPictureClipRegion(DISP, x, region);

#define CREATE_ARGB_PICTURE(drawable, depth, target) \
	if ( guts.render_extension) target = prima_render_create_picture(drawable, depth)

#define CLEANUP_RENDER_STIPPLES(self) \
	if (guts.render_extension) prima_render_cleanup_stipples(self)

extern Picture
prima_render_create_picture(XDrawable drawable, int depth);

void
prima_render_cleanup_stipples(Handle self);

#else

#define CREATE_ARGB_PICTURE(drawable, depth, target)
#define DELETE_ARGB_PICTURE(x)
#define CLIP_ARGB_PICTURE(x,region)
#define CLEANUP_RENDER_STIPPLES(self)

#endif

extern void
prima_cleanup_drawable_after_painting( Handle self);

extern void
prima_cursor_tick( void);

extern void
prima_no_cursor( Handle self);

extern Cursor
prima_null_pointer( void);

#define WAIT_NEVER   0
#define WAIT_ALWAYS  1
#define WAIT_IF_NONE 2

extern Bool
prima_one_loop_round( int wait, Bool careOfApplication);

extern void
prima_prepare_drawable_for_painting( Handle self, Bool inside_on_paint);

extern Bool
prima_simple_message( Handle self, int cmd, Bool is_post);

extern void
prima_update_cursor( Handle self);

extern Bool
prima_update_rotated_fonts( PCachedFont f, const char * text, int len, Bool wide,
	NPoint trig_cache, Matrix matrix, PRotatedFont *result, Bool * ok_to_not_rotate);

#define frUnix_int 1000

extern int
prima_rm_get_int( Handle self, XrmQuark class_detail, XrmQuark name_detail, int default_value);

extern void
prima_rect_union( XRectangle *t, const XRectangle *s);

extern void
prima_rect_intersect( XRectangle *t, const XRectangle *s);

extern void
prima_send_create_event( XWindow win);

extern void
prima_gc_ximages( void);

extern void
prima_ximage_event( XEvent*);

extern PrimaXImage*
prima_prepare_ximage( int width, int height, int format);

extern Bool
prima_free_ximage( PrimaXImage *i);

extern int
prima_rop_map( int rop);

extern void
prima_gp_get_clip_rect( Handle self, XRectangle *cr, Bool for_internal_paints);

extern XWindow
prima_find_frame_window( XWindow w);

extern Bool
prima_get_frame_info( Handle self, PRect r);

extern void
prima_send_cmSize( Handle self, Point oldSize);

extern Bool
prima_no_input( PDrawableSysData XX, Bool ignore_horizon, Bool beep);

extern void
process_transparents( Handle self);

extern Bool
apc_window_set_visible( Handle self, Bool show);

extern void
apc_SetWMNormalHints( Handle self, XSizeHints * hints);

extern Bool
prima_window_reset_menu( Handle self, int newMenuHeight);

extern void
prima_end_menu(void);

extern int
prima_handle_menu_shortcuts( Handle self, XEvent * ev, KeySym keysym);

extern void
prima_wm_sync( Handle self, int eventType);

extern Bool
prima_wm_net_state_read_maximization( XWindow window, Atom property);

extern unsigned char *
prima_get_window_property( XWindow window, Atom property, Atom req_type, Atom * actual_type,
			int * actual_format, unsigned long * nitems);

extern void
prima_utf8_to_wchar( const char * utf8, XChar2b * u16, int src_len_bytes, int target_len_xchars);

extern XChar2b *
prima_alloc_utf8_to_wchar( const char * utf8, int length_chars);

extern void
prima_wchar2char( char * dest, XChar2b * src, int lim);

extern void
prima_char2wchar( XChar2b * dest, char * src, int lim);

extern XCharStruct *
prima_char_struct( XFontStruct * xs, void * c, Bool wide);

extern Color**
prima_standard_colors(int * n_classes);

struct MsgDlg {
	struct MsgDlg * next;
	Font  * font;
	Point   btnPos;
	Point   btnSz;
	Bool    wide;
	char ** wrapped;
	int     count;
	int     wrappedCount;
	int    *widths, *lengths;
	int     OKwidth;
	Point   textPos;
	Bool    active;
	Bool    pressed;
	Bool    grab;
	int     fontId;
	Point   winSz;
	GC      gc;
	unsigned long fg, l3d, d3d;
	Brush   bg;
	XWindow w;
	int     focus_revertTo;
	XWindow focus;
};

extern void
prima_msgdlg_event( XEvent* ev, struct MsgDlg * md);

typedef void (*RETSIGTYPE)(int);

#undef XDestroyImage
#define XDestroyImage prima_XDestroyImage
extern void
prima_XDestroyImage( XImage * x);

typedef int (*XIfEventProcType)(Display*,XEvent*,XPointer);

#endif

#ifdef USE_XFT

extern void
prima_xft_init( void);

extern void
prima_xft_init_font_substitution( void);

extern void
prima_xft_done( void);

extern void
prima_xft_gp_destroy( Handle self );

extern void
prima_xft_build_key( PFontKey key, PFont f, Matrix matrix, Bool bySize);

extern PCachedFont
prima_xft_match( Font * font, Matrix matrix, Bool by_size, PCachedFont cf);

extern int
prima_xft_get_text_width( PCachedFont self, const char * text, int len,
			int flags, uint32_t * map8,
			Point * overhangs);

extern int
prima_xft_get_glyphs_width( Handle self, PCachedFont selfxx, PGlyphsOutRec glyphs,
			int flags, Point * overhangs);

extern Point *
prima_xft_get_text_box( Handle self, const char * text, int len, int flags);

extern Point *
prima_xft_get_glyphs_box( Handle self, PGlyphsOutRec glyphs);

extern Bool
prima_xft_text_out( Handle self, const char * text, int x, int y, int len, int flags);

extern Bool
prima_xft_glyphs_out( Handle self, PGlyphsOutRec glyphs, int x, int y);

extern unsigned long *
prima_xft_get_font_ranges( Handle self, int * count);

extern char *
prima_xft_get_font_languages( Handle self);

extern PFontABC
prima_xft_get_font_abc( Handle self, int firstChar, int lastChar, int flags);

extern PFontABC
prima_xft_get_font_def( Handle self, int firstChar, int lastChar, int flags);

extern int
prima_xft_get_glyph_outline( Handle self, unsigned int index, unsigned int flags, int ** buffer);

extern PCachedFont
prima_xft_get_cache( PFont font, Matrix matrix);

extern Bool
prima_xft_parse( char * ppFontNameSize, Font * font);

extern void
prima_xft_update_region( Handle self);

extern int
prima_xft_load_font( char * fontName );

extern Bool
prima_xft_text_shaper_ident( Handle self, PTextShapeRec r);

extern Bool
prima_xft_text_shaper_bytes( Handle self, PTextShapeRec r);

extern Bool
prima_xft_text_shaper_harfbuzz( Handle self, PTextShapeRec r);

extern unsigned long *
prima_xft_mapper_query_ranges(PFont font, int * count, unsigned int * flags);

extern Byte*
prima_xft_get_glyph_bitmap( Handle self, uint16_t index, unsigned int flags, PPoint offset, PPoint size, int *advance, int *bpp);

extern Bool
prima_xft_font_has_color(Handle self);

#endif

extern Bool
prima_text_shaper_core_text( Handle self, PTextShapeRec r);

extern void
prima_xim_init(void);

extern void
prima_xim_done(void);

extern Bool
prima_xim_handle_key_press( Handle self, XKeyEvent *ev, Event *e, KeySym *sym);

extern void
prima_xim_focus_in(Handle self);

extern void
prima_xim_focus_out(void);

extern void
prima_xim_update_cursor( Handle self);

extern U32
prima_keysym_to_keycode( KeySym keysym, XKeyEvent *ev, U8 character );

#ifdef WITH_GTK
extern Display*
prima_gtk_init( void);

extern Bool
prima_gtk_done( void);

extern char *
prima_gtk_openfile( char * params);

extern Bool
prima_gtk_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen);
#endif

#ifdef WITH_COCOA
extern uint32_t*
prima_cocoa_application_get_bitmap( int x, int y, int xLen, int yLen, int yMax);

extern char *
prima_cocoa_system_action( char * params);
#endif

typedef struct _ViewProfile {
Point        pos;
Point        size;
Point        virtualSize;
Bool         visible;
Bool         focused;
Handle       capture;
char *       title;
int          shape_count;
int          shape_ordering;
XRectangle * shape_rects;
} ViewProfile, *PViewProfile;

extern void
prima_set_view_ex( Handle self, PViewProfile p);

extern void
prima_get_view_ex( Handle self, PViewProfile p);

extern void
prima_notify_sys_handle( Handle self );

extern int
prima_flush_events( Display * disp, XEvent * ev, Handle self);

extern Region
prima_region_create( Handle mask);

extern Handle
prima_find_toplevel_window(Handle self);

extern Handle
prima_find_root_parent(Handle self);

extern Byte*
prima_mirror_bits( void);

extern int
prima_copy_region_data(void * region);

#define RPS_OK       0
#define RPS_PARTIAL  1
#define RPS_NODATA   2
#define RPS_ERROR    3

extern Bool
prima_handle_dnd_event( Handle self, XEvent *xev);

extern int
prima_read_property( XWindow window, Atom property, Atom * type, int * format,
	unsigned long * size, unsigned char ** data, Bool delete_property);

extern void
prima_clipboard_kill_item( PClipboardDataItem item, Handle id);

extern void
prima_detach_xfers( PClipboardSysData XX, Handle id, Bool clear_original_data);

extern void
prima_clipboard_query_targets( Handle self );

extern int
prima_clipboard_fill_targets( Handle self);

extern void
prima_update_dnd_aware( Handle self );

extern Cursor
prima_get_cursor(Handle self);

extern void
prima_paint_text_background( Handle self, Point * p, int x, int y );

extern Bool
prima_update_quarks_cache( Handle self);

#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
int
my_XRenderCompositeDoublePoly (Display		    *dpy,
			    int			    op,
			    Picture		    src,
			    Picture		    dst,
			    _Xconst XRenderPictFormat	*maskFormat,
			    int			    xSrc,
			    int			    ySrc,
			    int			    xDst,
			    int			    yDst,
			    _Xconst XPointDouble    *fpoints,
			    int			    npoints,
			    int			    winding);
Picture
prima_pen_picture(Handle self);
#endif

#ifdef USE_FONTQUERY

#define FT266_TRUNC(x)    ((x) >> 6)
#define FT266_ROUND(x)    (((x)+32) & -64)
#define FT266_to_short(x) ((short)(FT266_TRUNC(FT266_ROUND(x))))

extern void
prima_fc_init(void);

extern void
prima_fc_init_font_substitution(void);

extern void
prima_fc_done(void);

extern char *
prima_fc_get_font_languages( FcPattern *pat);

extern uint32_t *
prima_fc_map8( const char * encoding);

extern const char *
prima_fc_find_encoding( const char *encoding );

extern unsigned long *
prima_fc_get_font_ranges( FcCharSet *c, int * count);

extern int
prima_fc_load_font( char* filename);

extern void
prima_fc_build_font_key( int type, PFontKey key, PFont f, Bool bySize, Matrix matrix);

extern void
prima_fc_set_font( Handle self, PFont font );

extern PFont
prima_fc_fonts( PFont array, const char *facename, const char * encoding, int *retCount);

extern void
prima_fc_font_encodings( PHash hash);

extern void
prima_fc_pattern2fontnames( FcPattern * pattern, Font * font);

extern void
prima_fc_pattern2font( FcPattern * pattern, PFont font);

extern char *
prima_fc_find_good_font_by_family( Font * f, int fc_spacing );

extern const char *
prima_fc_pattern2encoding( FcPattern *p);

extern int
prima_fc_suggest_pitch( PFont requested_font, PFont suggested_font, FcPattern *match);

extern void
prima_fc_end_suggestion(int flag);

extern Bool
prima_fc_encoding_is_supported( char *encoding, FcPattern *match);

extern Bool
prima_fq_begin_query(Handle self);

extern void
prima_fq_free_cached_font( PCachedFont f);

extern void
prima_fq_build_key( PFontKey key, PFont f);

extern PCachedFont
prima_fq_match( PFont dest, Bool by_size, PCachedFont kf);

extern Bool
prima_fq_set_font( Handle self, PCachedFont kf);

extern Byte*
prima_fq_get_glyph_bitmap( Handle self, uint16_t index, unsigned int flags, PPoint offset, PPoint size, int *advance, int *bpp);

extern int
prima_fq_get_glyph_outline( Handle self, unsigned int index, unsigned int flags, int ** buffer);

extern Bool
prima_fq_text_shaper_ident( Handle self, PTextShapeRec r);

extern Bool
prima_fq_text_shaper_bytes( Handle self, PTextShapeRec r);

extern unsigned long *
prima_fq_mapper_query_ranges(PFont font, int * count, unsigned int * flags);

extern Bool
prima_fq_text_shaper_harfbuzz( Handle self, PTextShapeRec r);

extern int
prima_fq_get_text_width( Handle self, const char * text, int len, int flags, Point * overhangs);

extern int
prima_fq_get_glyphs_width( Handle self, PGlyphsOutRec t, int flags, Point * overhangs);

extern Point *
prima_fq_get_text_box( Handle self, const char * text, int len, int flags);

extern Point *
prima_fq_get_glyphs_box( Handle self, PGlyphsOutRec t);

extern PFontABC
prima_fq_get_font_abc( Handle self, int firstChar, int lastChar, int flags);

extern PFontABC
prima_fq_get_font_def( Handle self, int firstChar, int lastChar, int flags);

extern unsigned long *
prima_fq_get_font_ranges( Handle self, int * count);

extern char *
prima_fq_get_font_languages( Handle self);

extern void
prima_fq_apply_synthetic_fields(PCachedFont kf, PFont source, PFont dest);

extern Bool
prima_ft_init( void);

extern void
prima_ft_done( void);

extern FT_Face
prima_ft_lock_face( char *filename, int index);

extern void
prima_ft_unlock_face(FT_Face face);

Bool
prima_ft_combining_supported( FT_Face face );

extern int
prima_ft_get_glyph_outline( FT_Face face, FT_UInt ft_index, FT_Int32 ft_flags, Bool want_color, int ** buffer);

extern Byte*
prima_ft_get_glyph_bitmap( FT_Face face, FT_UInt index, FT_Int32 flags, PPoint offset, PPoint size, int *advance, int *bpp);

extern Bool
prima_ft_font_has_color( FT_Face face);

extern Bool
prima_ft_text_shaper_harfbuzz( FT_Face face, PTextShapeRec r);

extern void
prima_ft_detail_tt_font( FT_Face face, PFont font, float mul);

#endif
