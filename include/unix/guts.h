#ifndef _UNIX_GUTS_H_
#define _UNIX_GUTS_H_

#define Font XFont
#define Drawable XDrawable
#define Window XWindow
#undef Bool
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>

#include "../guts.h"
#include "Widget.h"

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
} FontFlags;

typedef struct _FontInfo {
   char *xname;
   char *vecname;
   Font font;
   FontFlags flags;
   char lc_name[ 256];
   char lc_family[ 256];
   Bool sloppy;
} FontInfo, *PFontInfo;

typedef struct _CachedFont {
   char *load_name;
   int ref_cnt;
   XFontStruct *fs;
   XFont id;
   Font font;
   FontFlags flags;
} CachedFont, *PCachedFont;

union _unix_sys_data;
struct _timer_sys_data;
struct _drawable_sys_data;

#define VIRGIN_GC_MASK (GCLineWidth|GCBackground|GCForeground|GCFunction|GCClipMask|GCLineStyle)

typedef struct _gc_list
{
   struct _gc_list *next;
   struct _gc_list *prev;
   GC gc;
   struct _drawable_sys_data *holder;
} GCList;

typedef struct _paint_list
{
   struct _paint_list *next;
   Handle obj;
} PaintList, *PPaintList;

struct _UnixGuts
{
   Display *display;
   int screen_number;
   PHash windows;
   NPoint resolution;
   int depth;
   int byte_order;
   int bit_order;
   GCList *free_gcl;
   GCList *used_gcl;
   PPaintList paint_list;
   struct _timer_sys_data *oldest;
   struct _timer_sys_data *cursor_timer;
   Pixmap cursor_save, cursor_xor;
   Point cursor_pixmap_size;
   Bool cursor_shown;
   int visible_timeout, invisible_timeout;
   struct {
      long request_length;
      long XDrawLines;
      long XFillPolygon;
      long XDrawSegments;
      long XDrawRectangles;
      long XFillRectangles;
      long XDrawArcs;
      long XFillArcs;
   } limits;
   Bool insert;
   int mouse_buttons;
   int mouse_wheel_up;
   int mouse_wheel_down;
   unsigned char buttons_map[ 256];
   int cursor_width;
   int cursor_height;
   Handle focused;

   char **font_names;  /* to be XFreed */
   int n_fonts;
   PFontInfo font_info;
   PHash font_hash;
   /* Useful font-related atoms not found as XA_XXX;  see FXA_XXX below */
   Atom fxa_resolution_x;
   Atom fxa_resolution_y;
   Atom fxa_pixel_size;
   Atom fxa_spacing;
   Atom fxa_relative_weight;
   Atom fxa_foundry;
   Atom fxa_average_width;

   /* resource management */
   XrmDatabase db;

   /* generally used quarks */
   XrmQuark qString;
   XrmQuark qBackground;
   XrmQuark qbackground;
   XrmQuark qBlinkinvisibletime;
   XrmQuark qblinkinvisibletime;
   XrmQuark qBlinkvisibletime;
   XrmQuark qblinkvisibletime;
   XrmQuark qFont;
   XrmQuark qfont;
   XrmQuark qForeground;
   XrmQuark qforeground;
   XrmQuark qWheeldown;
   XrmQuark qwheeldown;
   XrmQuark qWheelup;
   XrmQuark qwheelup;

   /* statistics */
   long int total_events;
   long int handled_events;
   long int skipped_events;
   long int unhandled_events;

   /* debugging -  XCHECKPOINT */
   RequestInformation ri[ REQUEST_RING_SIZE];
   int ri_head, ri_tail;
   Bool dolbug;

   /* window manager specifics */
   void *wm_data;
   void (*wm_create_window)( Handle, ApiHandle);
   void (*wm_cleanup)( void);
   Bool (*wm_translate_event)( Handle, XEvent *, PEvent);
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

#define COMPONENT_SYS_DATA \
   XrmQuarkList q_class_name; \
   XrmQuarkList q_instance_name; \
   int n_class_name; \
   int n_instance_name

typedef struct _drawable_sys_data /* more like widget_sys_data */
{
   COMPONENT_SYS_DATA; /* must be the first */
   XDrawable drawable;
   XWindow parent;
   NPoint resolution;
   Point origin, known_origin;
   Point size, known_size;
   Point transform, gtransform;
   Handle owner;  /* The real one */
   XWindow real_parent; /* top levels */
   XGCValues gcv;
   GC gc;
   GCList *gcl;
   XColor fore, back, saved_fore, saved_back;
   ColorSet colors;
   Region region;
   Region stale_region;
   XRectangle exposed_rect;
   XRectangle clip_rect;
   Rect scroll_rect; /* clipping */
   FillPattern fill_pattern;
   int rop, paint_rop;
   char *dashes, *paint_dashes;
   int ndashes, paint_ndashes;
   PCachedFont font;
   Font saved_font;
   Point cursor_pos;
   Point cursor_size;
   struct {
      int clip_owner			: 1;
      int sync_paint			: 1;
      int mapped			: 1;
      int exposed			: 1;
      int paint                 	: 1;
      int saved_zero_line       	: 1;
      int zero_line             	: 1;
      int grab                  	: 1;
      int enabled               	: 1;
      int focused       	        : 1;
      int reload_font			: 1;
      int do_size_hints			: 1;
      int no_size			: 1;
      int cursor_visible		: 1;
      int process_configure_notify	: 1;
   } flags;
   XImage *image_cache;
   XImage *icon_cache;
   XColor bitmap_fore, bitmap_back;
} DrawableSysData, *PDrawableSysData;

#define CURSOR_TIMER	((Handle)11)

typedef struct _timer_sys_data
{
   COMPONENT_SYS_DATA; /* must be the first */
   int timeout;
   Handle who;
   struct _timer_sys_data *older;
   struct _timer_sys_data *younger;
   struct timeval when;
} TimerSysData, *PTimerSysData;

typedef union _unix_sys_data
{
   struct {
      COMPONENT_SYS_DATA;
   } component;
   DrawableSysData drawable;
   TimerSysData timer;
} UnixSysData, *PUnixSysData;

#define DISP		(guts. display)
#define SCREEN		(guts. screen_number)
#define DRIN		guts. display, guts. screen_number
#define X_WINDOW	(PComponent(self)-> handle)
#define X(obj)		((PDrawableSysData)(PComponent((obj))-> sysData))
#define DEFXX		PDrawableSysData selfxx = X(self)
#define XX		selfxx
#define WHEEL_DELTA	120

extern void
prima_handle_event( XEvent *ev, XEvent *next_event);

extern void
prima_get_gc( PDrawableSysData);

extern void
prima_release_gc( PDrawableSysData);

extern void
prima_init_font_subsystem( void);

extern void
prima_init_image_subsystem( void);

extern void
prima_cleanup_drawable_after_painting( Handle self);

extern void
prima_cleanup_font_subsystem( void);

extern void
prima_cleanup_image_subsystem( void);

extern void
prima_cursor_tick( void);

extern void
prima_no_cursor( Handle self);

extern void
prima_prepare_drawable_for_painting( Handle self);

extern void
prima_update_cursor( Handle self);

extern int
unix_rm_get_int( Handle self, XrmQuark class_detail, XrmQuark name_detail, int default_value);

/* rectangle arithmetic */
extern void
prima_rect_union( XRectangle *t, const XRectangle *s);

extern void
prima_rect_intersect( XRectangle *t, const XRectangle *s);

/* Interaction with Window Managers */

extern void
prima_wm_init( void);

typedef Bool (*prima_wm_hook)( void);

#endif

#ifdef PRIMA_WM_SUPPORT		/* Do you know what are you doing? */
static prima_wm_hook registered_window_managers[] = {
   /* This should _always_ be the last item */
   prima_wm_generic
};
#endif

