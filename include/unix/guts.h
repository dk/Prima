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

typedef struct {
   PrimaXImage *image;
   PrimaXImage *icon;
   XColor fore;
   XColor back;
   Bool bitmap;
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
   int height           : 1;
   int width            : 1;
   int style            : 1;
   int pitch            : 1;
   int direction        : 1;
   int resolution       : 1;
   int name             : 1;
   int size             : 1;
   int codepage         : 1;
   int family           : 1;
   int vector           : 1;
   int ascent           : 1;
   int descent          : 1;
   int weight           : 1;
   int maximalWidth     : 1;
   int internalLeading  : 1;
   int externalLeading  : 1;
   int xDeviceRes       : 1;
   int yDeviceRes       : 1;
   int firstChar        : 1;
   int lastChar         : 1;
   int breakChar        : 1;
   int defaultChar      : 1;
   /* extras */
   int bad_vector	: 1;
   int sloppy           : 1;
   int disabled         : 1;
   int funky            : 1;
   int intNames         : 1;
} FontFlags;

typedef struct _FontInfo {
   FontFlags    flags;
   Font         font;
   char         lc_family[256];
   char         lc_name[256];
   char        *vecname;
   char        *xname;
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

typedef struct _CachedFont {
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
struct     _timer_sys_data;
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

struct MsgDlg;

typedef struct _WmGenericData {
   Atom deleteWindow;
   Atom protocols;
   Atom takeFocus;
} WmGenericData, *PWmGenericData;

struct _UnixGuts
{
   /* Event management */
   Time                         click_time_frame;
   Time                         double_click_time_frame;
   PHash                        clipboards;
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
   /* Timers & cursors */
   int                          cursor_height;
   Point                        cursor_pixmap_size;
   Pixmap                       cursor_save;
   Bool                         cursor_shown;
   struct _timer_sys_data      *cursor_timer;
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
} guts;

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

#define COMPONENT_SYS_DATA                                                    \
   Handle self;                                                               \
   struct {                                                                   \
      int application           : 1;                                          \
      int bitmap                : 1;                                          \
      int dbm                   : 1;                                          \
      int drawable              : 1;                                          \
      int icon                  : 1;                                          \
      int image                 : 1;                                          \
      int menu                  : 1;                                          \
      int pixmap                : 1;                                          \
      int popup                 : 1;                                          \
      int timer                 : 1;                                          \
      int widget                : 1;                                          \
      int window                : 1;                                          \
   } type;                                                                    \
   XrmQuarkList q_class_name;                                                 \
   XrmQuarkList q_instance_name;                                              \
   int n_class_name;                                                          \
   int n_instance_name

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
   Point origin, known_origin;
   Point size, known_size, bsize;
   Point transform, gtransform, btransform;
   Handle owner;  /* The real one */
   XWindow real_parent; /* top levels */
   XWindow above;
   XGCValues gcv;
   GC gc;
   GCList *gcl;
   XColor fore, back, saved_fore, saved_back;
   ColorSet colors;
   Region region;
   Region stale_region;
   XRectangle clip_rect;
   FillPattern fill_pattern, saved_fill_pattern;
   Pixmap fp_pixmap;
   int rop, paint_rop;
   int rop2, paint_rop2;
   int line_style;
   unsigned char *dashes, *paint_dashes;
   int ndashes, paint_ndashes;
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
   struct {
      int base_line                     : 1;
      int clip_owner			: 1;
      int cursor_visible		: 1;
      int do_size_hints			: 1;
      int enabled               	: 1;
      int exposed			: 1;
      int focused       	        : 1;
      int grab                  	: 1;
      int mapped			: 1;
      int modal                         : 1;
      int no_size			: 1;
      int opaque                	: 1;
      int paint                 	: 1;
      int paint_base_line               : 1;
      int paint_opaque                  : 1;
      int paint_pending                 : 1;
      int pointer_obscured              : 1;
      int process_configure_notify	: 1;
      int reload_font			: 1;
      int saved_zero_line       	: 1;
      int sync_paint			: 1;
      int zero_line             	: 1;
   } flags;
   ImageCache bitmap_cache;
   ImageCache screen_cache;
   Handle preexec_focus;
   TAILQ_ENTRY(_drawable_sys_data) paintq_link;
} DrawableSysData, *PDrawableSysData;

#define XF_ENABLED(x)   ((x)->flags.enabled)
#define XF_IN_PAINT(x)  ((x)->flags.paint)

#define CURSOR_TIMER	((Handle)11)

typedef struct _timer_sys_data
{
   COMPONENT_SYS_DATA;
   int timeout;
   Handle who;
   struct _timer_sys_data *older;
   struct _timer_sys_data *younger;
   struct timeval when;
} TimerSysData, *PTimerSysData;

typedef struct _menu_item
{
   int          x;
   int          y;
   int          ux;
   int          uy;
   int          ul;
   char        *text;
} UnixMenuItem, *PUnixMenuItem;

typedef struct _menu_window
{
   Handle               self;
   XWindow              w;
   Point                sz;
   PMenuItemReg         m;
   int                  num;
   PUnixMenuItem        um;
   struct _menu_window *next;
   struct _menu_window *prev;
} MenuWindow, *PMenuWindow;

typedef struct _menu_sys_data
{
   COMPONENT_SYS_DATA;
   PMenuWindow          w;
   PCachedFont          font;
   XColor               c[ciMaxId+1];
} MenuSysData, *PMenuSysData;

typedef struct _clipboard_sys_data
{
   COMPONENT_SYS_DATA;
   char                *name;
   Atom                 atom;
   Time                 have_since;
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
#define DRIN		guts. display, guts. screen_number
#define X_WINDOW	(PComponent(self)-> handle)
#define X(obj)		((PDrawableSysData)(PComponent((obj))-> sysData))
#define DEFXX		PDrawableSysData selfxx = (self == nilHandle ? nil : X(self))
#define XX		selfxx
#define WHEEL_DELTA	120

typedef U8 ColorComponent;

typedef struct
{
   unsigned long  mask;
   unsigned long  revMask;
             int  shift;
             int  revShift;
   ColorComponent lut[256];
} RGBLUTEntry;


extern Handle
prima_xw2h( XWindow win);

extern void
prima_handle_event( XEvent *ev, XEvent *next_event);

extern void
prima_handle_menu_event( XEvent *ev, XWindow win, Handle self);

extern void
prima_get_gc( PDrawableSysData);

extern void
prima_rebuild_watchers( void);

extern void
prima_release_gc( PDrawableSysData);

extern void
prima_init_font_subsystem( void);

extern XColor*
prima_allocate_color( Handle self, Color color);

extern void
prima_copy_xybitmap( unsigned char *data, const unsigned char *idata, int w, int h, int ls, int ils);

extern void
prima_mirror_bytes( unsigned char *data, int dataSize);

extern Bool
prima_create_icon_pixmaps( Handle bw_icon, Pixmap *xor, Pixmap *and);

extern ImageCache*
prima_create_image_cache( PImage img, Handle drawable);

void
prima_put_ximage( XDrawable win, GC gc, PrimaXImage *i,
                  int src_x, int src_y, int dst_x, int dst_y,
                  int width, int height);

Bool
prima_query_image( Handle self, XImage * image);

RGBLUTEntry *
prima_rgblut(void);

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
prima_prepare_drawable_for_painting( Handle self);

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

typedef Bool (*prima_wm_hook)( void);

extern PFontABC
prima_xfont2abc( XFontStruct * fs, int firstChar, int lastChar);

extern PCachedFont
prima_find_known_font( PFont font, Bool refill, Bool bySize);

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
   XColor *fg, *bg, *l3d, *d3d;
   XWindow w;
   int     focus_revertTo;
   XWindow focus;
};

extern void
prima_msgdlg_event( XEvent* ev, struct MsgDlg * md);

#endif

/* this does not belong here */
#ifdef PRIMA_WM_SUPPORT
static prima_wm_hook registered_window_managers[] = {
   prima_wm_generic     /* This must be the last item */
};
#endif

