/*-
 * Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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

#ifndef _UNIX_GUTS_H_
#define _UNIX_GUTS_H_

#if defined(HAVE_CONFIG_H)
#include "generic/config.h"
#endif
#define Drawable        XDrawable
#define Font            XFont
#define Window          XWindow
#undef  Bool
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif
#if defined( HAVE_X11_EXTENSIONS_XSHM_H) && defined( HAVE_SYS_IPC_H) && defined( HAVE_SYS_SHM_H)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#define USE_MITSHM      1
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
#include "bsd/queue.h"
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


#define CACHE_AUTODETECT 0
#define CACHE_BITMAP     1
#define CACHE_PIXMAP     2
#define CACHE_LOW_RES    3

typedef struct {
   int type;
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
   unsigned direction        : 1;
   unsigned resolution       : 1;
   unsigned name             : 1;
   unsigned size             : 1;
   unsigned codepage         : 1;
   unsigned family           : 1;
   unsigned vector           : 1;
   unsigned ascent           : 1;
   unsigned descent          : 1;
   unsigned weight           : 1;
   unsigned maximalWidth     : 1;
   unsigned internalLeading  : 1;
   unsigned externalLeading  : 1;
   unsigned xDeviceRes       : 1;
   unsigned yDeviceRes       : 1;
   unsigned firstChar        : 1;
   unsigned lastChar         : 1;
   unsigned breakChar        : 1;
   unsigned defaultChar      : 1;
   /* extras */
   unsigned bad_vector	     : 1;
   unsigned sloppy           : 1;
   unsigned disabled         : 1;
   unsigned funky            : 1;
   unsigned intNames         : 1;
   unsigned generic          : 1;
} FontFlags;

typedef struct _FontInfo {
   FontFlags    flags;
   Font         font;
   char         lc_family[256];
   char         lc_name[256];
   char        *vecname;
   char        *xname;
   short int    name_offset;
   short int    info_offset;
} FontInfo, *PFontInfo;

typedef struct _RotatedFont {
   int          direction;
   int          first;
   int          length;
   PrimaXImage**map;
   Point        shift;
   Point        dimension;
   Point        orgBox;
   Pixmap       arena;
   GC           arena_gc;
   Byte        *arena_bits;
   int          lineSize;
   int          defaultChar;
   Fixed        sin, cos, sin2, cos2;
   struct       RotatedFont *next;
} RotatedFont, *PRotatedFont;

typedef struct CachedFont {
   FontFlags    flags;
   Font         font;
   XFontStruct *fs;
   char        *load_name;
   XFont        id;
   PRotatedFont rotated;
   int          underlinePos;
   int          underlineThickness;
   int          refCnt;
} CachedFont, *PCachedFont;

union       _unix_sys_data;

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

#define COMPONENT_SYS_DATA                                                    \
   Handle self;                                                               \
   struct {                                                                   \
      unsigned application           : 1;                                     \
      unsigned bitmap                : 1;                                     \
      unsigned dbm                   : 1;                                     \
      unsigned drawable              : 1;                                     \
      unsigned icon                  : 1;                                     \
      unsigned image                 : 1;                                     \
      unsigned menu                  : 1;                                     \
      unsigned pixmap                : 1;                                     \
      unsigned popup                 : 1;                                     \
      unsigned timer                 : 1;                                     \
      unsigned widget                : 1;                                     \
      unsigned window                : 1;                                     \
      COMPONENT_SYS_DATA_ALIGN                                                \
   } type;                                                                    \
   XrmQuarkList q_class_name;                                                 \
   XrmQuarkList q_instance_name;                                              \
   int n_class_name;                                                          \
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

struct  _drawable_sys_data;

#define VIRGIN_GC_MASK  (       GCBackground    \
                        |       GCCapStyle      \
                        |       GCClipMask      \
                        |       GCForeground    \
                        |       GCFunction      \
                        |       GCLineStyle     \
                        |       GCLineWidth     \
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

typedef struct _WmGenericData {
   Atom deleteWindow;
   Atom protocols;
   Atom takeFocus;
} WmGenericData, *PWmGenericData;

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

typedef struct _UnixGuts
{
   /* Event management */
   Time                         click_time_frame;
   Time                         double_click_time_frame;
   PHash                        clipboards;
   Atom *                       clipboard_formats;
   int                          clipboard_formats_count;
   Atom                         create_event;
   fd_set                       excpt_set;
   PList                        files;
   long                         handled_events;
   XButtonEvent                 last_button_event;
   XButtonEvent                 last_click;
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
   GC                                   menugc;
   TAILQ_HEAD(,_drawable_sys_data)      paintq;
   PHash                                ximages;
   /* Font management */
   PHash                        font_hash;
   PFontInfo                    font_info;
   char                       **font_names;
   Atom                         fxa_average_width;
   Atom                         fxa_foundry;
   Atom                         fxa_pixel_size;
   Atom                         fxa_relative_weight;
   Atom                         fxa_resolution_x;
   Atom                         fxa_resolution_y;
   Atom                         fxa_spacing;
   int                          n_fonts;
   XFontStruct                 *pointer_font;
   Font                         default_font;
   Font                         default_menu_font;
   Font                         default_widget_font;
   Font                         default_msg_font;
   Font                         default_caption_font;
   int                          font_encoding_hack_type; /* see FEHT constants */
   /* Resource management */
   XrmDatabase                  db;
   XrmQuark                     qBackground;
   XrmQuark                     qbackground;
   XrmQuark                     qBlinkinvisibletime;
   XrmQuark                     qblinkinvisibletime;
   XrmQuark                     qBlinkvisibletime;
   XrmQuark                     qblinkvisibletime;
   XrmQuark                     qClicktimeframe;
   XrmQuark                     qclicktimeframe;
   XrmQuark                     qDoubleclicktimeframe;
   XrmQuark                     qdoubleclicktimeframe;
   XrmQuark                     qFont;
   XrmQuark                     qfont;
   XrmQuark                     qForeground;
   XrmQuark                     qforeground;
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
   int                          cursor_height;
   Point                        cursor_pixmap_size;
   Pixmap                       cursor_save;
   Bool                         cursor_shown;
   int                          cursor_width;
   Pixmap                       cursor_xor;
   Bool                         insert;
   int                          invisible_timeout;
   struct _timer_sys_data      *oldest;
   int                          visible_timeout;
   /* Window management */
   Handle                       focused;
   PHash                        menu_windows;
   PHash                        windows;
   /* WM dependancies */
   void                       (*wm_cleanup)( void);
   void                       (*wm_create_window)( Handle, ApiHandle);
   WmGenericData               *wm_data;
   Bool                       (*wm_translate_event)( Handle, XEvent *, PEvent);
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
   struct {
      long XDrawArcs;
      long XDrawLines;
      long XDrawRectangles;
      long XDrawSegments;
      long XFillArcs;
      long XFillPolygon;
      long XFillRectangles;
      long request_length;
   }                            limits;
   Bool                         local_connection;
   Cursor                       null_pointer;
   int                          pointer_invisible_count; /* 0 is visible, > 0 is not, can't be <0 */
   int                          mouse_buttons;
   int                          mouse_wheel_down;
   int                          mouse_wheel_up;
   NPoint                       resolution;
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
   struct MsgDlg               *message_boxes;
   XWindow                      grab_redirect;
   Handle                       grab_widget;
   Point                        grab_translate_mouse;
   int                          scroll_first;
   int                          scroll_next;
   Handle                       currentMenu;
   Handle                       unfocusedMenu;
   int                          menu_timeout;
   XWindow                      lastWMFocus;
   XWindow                      root;
   XVisualInfo                  visual;
   int                          visualClass;
   MainColorEntry *             palette;
   int *                        mappingPlace;
   unsigned long                monochromeMap[2];
   int                          palSize;
   int                          localPalSize;
   int *                        systemColorMap;
   int                          systemColorMapSize;
   int                          colorCubeRib;
   Bool                         dynamicColors;
   Bool                         grayScale;
   Bool                         useDithering;
   Colormap                     defaultColormap;
   FillPattern *                ditherPatterns;
   Point                        displaySize;
   long                         wm_event_timeout;
   int                          red_shift, green_shift, blue_shift;
   int                          red_range, green_range, blue_range;
   Point                        ellipseDivergence;
   int                          appLock;
   XGCValues                    cursor_gcv;
   TimerSysData                 sys_timers[ LAST_SYS_TIMER - FIRST_SYS_TIMER + 1];
} UnixGuts;

extern UnixGuts guts;

#define FXA_RESOLUTION_X guts. fxa_resolution_x
#define FXA_RESOLUTION_Y guts. fxa_resolution_y
#define FXA_POINT_SIZE XA_POINT_SIZE
#define FXA_PIXEL_SIZE guts. fxa_pixel_size
#define FXA_SPACING guts. fxa_spacing
#define FXA_WEIGHT XA_WEIGHT
#define FXA_RELATIVE_WEIGHT guts. fxa_relative_weight
#define FXA_FOUNDRY guts. fxa_foundry
#define FXA_FAMILY_NAME XA_FAMILY_NAME
#define FXA_AVERAGE_WIDTH guts. fxa_average_width

#define FEHT_NONE                      0
#define FEHT_MIXED_NAMES               1
#define FEHT_MIXED_AND_UNMIXED_NAMES   2

#define XCHECKPOINT						\
   STMT_START {							\
      guts. ri[ guts. ri_head]. line = __LINE__;			\
      guts. ri[ guts. ri_head]. file = __FILE__;			\
      guts. ri[ guts. ri_head]. request = NextRequest(DISP);	\
      guts. ri_head++;						\
      if ( guts. ri_head >= REQUEST_RING_SIZE)			\
	 guts. ri_head = 0;					\
      if ( guts. ri_tail == guts. ri_head) {			\
	 guts. ri_tail++;					\
	 if ( guts. ri_tail >= REQUEST_RING_SIZE)		\
	    guts. ri_tail = 0;					\
      }								\
   } STMT_END

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
   NPoint resolution;
   Point origin, size, bsize;
   Point transform, gtransform, btransform;
   Point ackOrigin, ackSize;   
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
   Region invalid_region, paint_region;
   XRectangle clip_rect;
   FillPattern fill_pattern, saved_fill_pattern;
   Pixmap fp_pixmap;
#if defined(sgi) && !defined(__GNUC__)
/* multiple compilation and runtime errors otherwise. must be some alignment tricks */
   char dummy_b_1[2];
#endif
   int rop, paint_rop;
   int rop2, paint_rop2;
   int line_style, line_width;
   unsigned char *dashes, *paint_dashes;
   int ndashes, paint_ndashes;
   Point clip_mask_extent, shape_extent, shape_offset;
   PCachedFont font;
   Font saved_font;
   Point cursor_pos;
   Point cursor_size;
   Point pointer_hot_spot;
   int pointer_id;
   Cursor actual_pointer;
   Cursor user_pointer;
   Pixmap user_p_source;
   Pixmap user_p_mask;
   void * recreateData;
   struct {
      unsigned base_line                : 1;
      unsigned brush_fore               : 1;
      unsigned brush_back               : 1;
      unsigned brush_null_hatch         : 1;
      unsigned clip_owner	        : 1;
      unsigned configured               : 1;
      unsigned cursor_visible		: 1;
      unsigned enabled               	: 1;
      unsigned exposed			: 1;
      unsigned falsely_hidden           : 1;
      unsigned first_click              : 1;
      unsigned grab                 	: 1;
      unsigned has_icon                 : 1;
      unsigned iconic                   : 1;
      unsigned mapped			: 1;
      unsigned modal                    : 1;
      unsigned opaque                	: 1;
      unsigned paint                    : 1;
      unsigned paint_base_line          : 1;
      unsigned paint_opaque             : 1;
      unsigned paint_pending            : 1;
      unsigned pointer_obscured         : 1;
      unsigned position_determined      : 1;
      unsigned process_configure_notify	: 1;
      unsigned reload_font		: 1;
      unsigned sizeable                 : 1;
      unsigned size_determined          : 1;
      unsigned sync_paint               : 1;
      unsigned transparent              : 1;
      unsigned transparent_busy         : 1;
      unsigned want_visible             : 1;
      unsigned withdrawn                : 1;
      unsigned zoomed                   : 1;
   } flags;
   ImageCache image_cache;
   Handle preexec_focus;
   TAILQ_ENTRY(_drawable_sys_data) paintq_link;
   Byte * palette;
} DrawableSysData, *PDrawableSysData;

#define XF_ENABLED(x)   ((x)->flags.enabled)
#define XF_IN_PAINT(x)  ((x)->flags.paint)

#define MenuTimerMessage   1021

#define MENU_ITEM_GAP 4

typedef struct _menu_item
{
   int          x;
   int          y;
   int          width;
   int          height;
   int          accel_width;
   Pixmap       pixmap;
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
} MenuWindow, *PMenuWindow;

typedef struct _menu_sys_data
{
   COMPONENT_SYS_DATA;
   Bool                 paint_pending;
   PMenuWindow          w;
   MenuWindow           wstatic;
   PCachedFont          font;
   int                  guillemots;
   unsigned long        c[ciMaxId+1];
   XWindow              focus;
   PMenuWindow          focused;
} MenuSysData, *PMenuSysData;

#define cfPixmap  2
#define cfTargets 3

typedef struct {
   IV size;
   unsigned char * data;
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
} ClipboardSysData, *PClipboardSysData;

typedef union _unix_sys_data
{
   ClipboardSysData             clipboard;
   struct {
      COMPONENT_SYS_DATA;
   }                            component;
   DrawableSysData              drawable;
   MenuSysData                  menu;
   TimerSysData                 timer;
} UnixSysData, *PUnixSysData;

#define DISP		(guts. display)
#define SCREEN		(guts. screen_number)
#define VISUAL          (guts. visual. visual)
#define DRIN		guts. display, guts. screen_number
#define X_WINDOW	(PComponent(self)-> handle)
#define X(obj)		((PDrawableSysData)(PComponent((obj))-> sysData))
#define DEFXX		PDrawableSysData selfxx = (self == nilHandle ? nil : X(self))
#define M(obj)		((PMenuSysData)(PComponent((obj))-> sysData))
#define DEFMM           PMenuSysData selfxx = ((PMenuSysData)(PComponent((self))-> sysData))
#define C(obj)		((PClipboardSysData)(PComponent((obj))-> sysData))
#define DEFCC		PClipboardSysData selfxx = C(self)
#define XX		selfxx
#define WHEEL_DELTA	120

typedef U8 ColorComponent;

extern Handle
prima_xw2h( XWindow win);

extern void
prima_handle_event( XEvent *ev, XEvent *next_event);

extern void
prima_handle_menu_event( XEvent *ev, XWindow win, Handle self);

extern void
prima_handle_selection_event( XEvent *ev, XWindow win, Handle self);

extern void
prima_get_gc( PDrawableSysData);

extern void
prima_rebuild_watchers( void);

extern void
prima_release_gc( PDrawableSysData);

extern Bool
prima_init_clipboard_subsystem( void);

extern Bool
prima_init_font_subsystem( void);

extern Bool
prima_init_color_subsystem( void);

extern void
prima_done_color_subsystem( void);

extern int
prima_color_find( Handle self, long color, int maxDiff, int * diff, int maxRank);

extern Bool
prima_palette_replace( Handle self, Bool fast);

#define COLORHINT_NONE  0
#define COLORHINT_BLACK 1
#define COLORHINT_WHITE 2 

#define LOGCOLOR_BLACK 0
#define LOGCOLOR_WHITE (guts.palSize?(guts.palSize-1):0xffffffff)

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
prima_copy_xybitmap( unsigned char *data, const unsigned char *idata, int w, int h, int ls, int ils);

extern void
prima_mirror_bytes( unsigned char *data, int dataSize);

extern Bool
prima_create_icon_pixmaps( Handle bw_icon, Pixmap *xor, Pixmap *and);

extern ImageCache*
prima_create_image_cache( PImage img, Handle drawable, int type);

void
prima_put_ximage( XDrawable win, GC gc, PrimaXImage *i,
                  int src_x, int src_y, int dst_x, int dst_y,
                  int width, int height);

Bool
prima_query_image( Handle self, XImage * image);

Bool
prima_std_query_image( Handle self, Pixmap px);

Pixmap
prima_std_pixmap( Handle self, int type);

extern void
prima_cleanup_drawable_after_painting( Handle self);

extern void
prima_cleanup_font_subsystem( void);

extern void
prima_cursor_tick( void);

extern void
prima_no_cursor( Handle self);

extern Bool
prima_one_loop_round( Bool wait, Bool careOfApplication);

extern void
prima_prepare_drawable_for_painting( Handle self, Bool inside_on_paint);

extern Bool
prima_simple_message( Handle self, int cmd, Bool is_post);

extern void
prima_update_cursor( Handle self);

extern Bool
prima_update_rotated_fonts( PCachedFont f, char * text, int len, int direction, PRotatedFont *result);

extern void
prima_free_rotated_entry( PCachedFont f);

extern int
unix_rm_get_int( Handle self, XrmQuark class_detail, XrmQuark name_detail, int default_value);

extern void
prima_rect_union( XRectangle *t, const XRectangle *s);

extern void
prima_rect_intersect( XRectangle *t, const XRectangle *s);

extern void
prima_send_create_event( XWindow win);

extern void
prima_wm_init( void);

extern void
prima_gc_ximages( void);

extern void
prima_ximage_event( XEvent*);

extern PrimaXImage*
prima_prepare_ximage( int width, int height, Bool bitmap);

extern Bool
prima_free_ximage( PrimaXImage *i);

extern int
prima_rop_map( int rop);

extern void
prima_gp_get_clip_rect( Handle self, XRectangle *cr);

extern XWindow
prima_find_frame_window( XWindow w);

extern Bool
prima_get_frame_info( Handle self, PRect r);

extern void
prima_send_cmSize( Handle self, Point oldSize);

extern Bool
prima_no_input( PDrawableSysData XX, Bool ignore_horizon, Bool beep);

extern Bool
apc_window_set_visible( Handle self, Bool show);

extern Bool
prima_window_reset_menu( Handle self, int newMenuHeight);

extern void
prima_end_menu(void);

extern int
prima_handle_menu_shortcuts( Handle self, XEvent * ev, KeySym keysym); 
   
extern void
prima_wm_sync( Handle self, int eventType);

typedef Bool (*prima_wm_hook)( void);

extern PFontABC
prima_xfont2abc( XFontStruct * fs, int firstChar, int lastChar);

extern PCachedFont
prima_find_known_font( PFont font, Bool refill, Bool bySize);

extern void
prima_font_pp2font( char * ppFontNameSize, PFont font);

struct MsgDlg {
   struct MsgDlg * next;
   Font  * font;
   Point   btnPos;
   Point   btnSz;
   char ** wrapped;
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

#endif

/* this does not belong here */
#ifdef PRIMA_WM_SUPPORT
static prima_wm_hook registered_window_managers[] = {
   prima_wm_generic     /* This must be the last item */
};
#endif

