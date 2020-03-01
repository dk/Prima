/***********************************************************/
/*                                                         */
/*  System dependent menu routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Menu.h"
#include "Image.h"
#include "Window.h"
#include "Application.h"
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#define TREE            (PAbstractMenu(self)->tree)

static PMenuWindow
get_menu_window( Handle self, XWindow xw)
{
	DEFMM;
	PMenuWindow w = XX-> w;
	while (w && w->w != xw)
		w = w->  next;
	return w;
}

extern Cursor predefined_cursors[];

static PMenuWindow
get_window( Handle self, PMenuItemReg m)
{
	DEFMM;
	PMenuWindow w, wx;
	XSetWindowAttributes attrs;

	if ( !( w = malloc( sizeof( MenuWindow)))) return nil;
	bzero(w, sizeof( MenuWindow));
	w-> self = self;
	w-> m = m;
	w-> sz.x = -1;
	w-> sz.y = -1;
	attrs. event_mask = 0
		| KeyPressMask
		| KeyReleaseMask
		| ButtonPressMask
		| ButtonReleaseMask
		| EnterWindowMask
		| LeaveWindowMask
		| PointerMotionMask
		| ButtonMotionMask
		| KeymapStateMask
		| ExposureMask
		| VisibilityChangeMask
		| StructureNotifyMask
		| FocusChangeMask
		| PropertyChangeMask
		| ColormapChangeMask
		| OwnerGrabButtonMask;
	attrs. override_redirect = true;
	attrs. save_under = true;
	attrs. do_not_propagate_mask = attrs. event_mask;
	w->w = XCreateWindow( DISP, guts. root,
				0, 0, 1, 1, 0, CopyFromParent,
				InputOutput, CopyFromParent,
				0
				| CWOverrideRedirect
				| CWEventMask
				| CWSaveUnder
				, &attrs);
	if (!w->w) {
		free(w);
		return nil;
	}
	XCHECKPOINT;
	XSetTransientForHint( DISP, w->w, None);
	hash_store( guts.menu_windows, &w->w, sizeof(w->w), (void*)self);
	wx = XX-> w;
	if ( predefined_cursors[crArrow] == None) {
		XCreateFontCursor( DISP, XC_left_ptr);
		XCHECKPOINT;
	}
	XDefineCursor( DISP, w-> w, predefined_cursors[crArrow]);
	if ( wx) {
		while ( wx-> next ) wx = wx-> next;
		w-> prev = wx;
		wx-> next = w;
	} else
		XX-> w = w;
	return w;
}

static int
item_count( PMenuWindow w)
{
	int i = 0;
	PMenuItemReg m = w->m;

	while (m) {
		i++; m=m->next;
	}
	return i;
}

static void
free_unix_items( PMenuWindow w)
{
	int i;
	if ( w-> um) {
		if ( w-> first < 0) {
			for ( i = 0; i < w->num; i++)
				if ( w-> um[i].pixmap)
					XFreePixmap( DISP, w-> um[i].pixmap);
			free( w-> um);
		}
		w-> um = nil;
	}
	w-> num = 0;
}

static void
menu_window_delete_downlinks( PMenuSysData XX, PMenuWindow wx)
{
	PMenuWindow w = wx-> next;
	{
		XRectangle r;
		Region rgn;
		r. x = 0;
		r. y = 0;
		r. width  = guts. displaySize. x;
		r. height = guts. displaySize. y;
		rgn = XCreateRegion();
		XUnionRectWithRegion( &r, rgn, rgn);
		XSetRegion( DISP, guts. menugc, rgn);
		XDestroyRegion( rgn);
		XSetForeground( DISP, guts. menugc, XX->c[ciBack]);
	}
	while ( w) {
		PMenuWindow xw = w-> next;
		hash_delete( guts. menu_windows, &w-> w, sizeof( w-> w), false);
		XFillRectangle( DISP, w-> w, guts. menugc, 0, 0, w-> sz. x, w-> sz. y);
		XDestroyWindow( DISP, w-> w);
		XFlush( DISP);
		free_unix_items( w);
		free( w);
		w = xw;
	}
	wx-> next = nil;
	XX-> focused = wx;
	XFlush(DISP);
}

#define MENU_XOFFSET 5
#define MENU_CHECK_XOFFSET 10

static int
get_text_width( PCachedFont font, const char * text, int byte_length, Bool utf8, uint32_t * xft_map8)
{
	int ret = 0;
	int char_len = utf8 ? utf8_length(( U8*) text, ( U8*) text + byte_length) : byte_length;
#ifdef USE_XFT
	if ( font-> xft)
		return prima_xft_get_text_width( font, text, char_len, false, utf8, xft_map8, nil);
#endif
	if ( utf8) {
		XChar2b * xc = prima_alloc_utf8_to_wchar( text, char_len);
		if ( xc) {
			ret = XTextWidth16( font-> fs, xc, char_len);
			free( xc);
		}
	} else {
		ret = XTextWidth( font-> fs, text, byte_length);
	}
	return ret;
}

typedef struct {
	void        * xft_drawable;
	uint32_t    * xft_map8;
	Color         rgb;
	unsigned long pixel;
	XWindow       win;
	GC            gc;
	Bool          layered;
	unsigned long * c;
} MenuDrawRec;

static void
get_font_abc( PCachedFont font, char * index, Bool utf8, FontABC * rec, MenuDrawRec * data)
{
	char buf[2];
	XCharStruct * cs;
#ifdef USE_XFT
	if ( font-> xft) {
		Point ovx;
		rec-> b = prima_xft_get_text_width(
			font, index, 1, false,
			utf8, data-> xft_map8, &ovx
		);
		/* not really abc, but enough for its single invocation */
		rec-> a = -ovx. x;
		rec-> c = -ovx. y;
		return;
	}
#endif
	if ( utf8)
		prima_utf8_to_wchar( index, ( XChar2b*) buf, 2, 1);
	else
		buf[0] = *index;
	cs = prima_char_struct( font-> fs, ( XChar2b*) buf, utf8);
	rec-> a = cs-> lbearing;
	rec-> b = cs-> rbearing - cs-> lbearing;
	rec-> c = cs-> width - cs-> rbearing;
}

static void
text_out( PCachedFont font, const char * text, int length, int x, int y,
			Bool utf8, MenuDrawRec * data)
{
#ifdef USE_XFT
	XftColor xftcolor;
	if ( font-> xft) {
		xftcolor.color.red   = COLOR_R16(data-> rgb);
		xftcolor.color.green = COLOR_G16(data-> rgb);
		xftcolor.color.blue  = COLOR_B16(data-> rgb);
		xftcolor.color.alpha = 0xffff;
		xftcolor.pixel       = data-> pixel;
		XftDrawString32(( XftDraw*) data-> xft_drawable, &xftcolor,
							font-> xft, x, y, ( FcChar32*)text, length);
		return;
	}
#endif
	XSetForeground( DISP, data-> gc, data-> pixel);
	if ( utf8)
		XDrawString16( DISP, data->win, data->gc, x, y, ( XChar2b*) text, length);
	else
		XDrawString( DISP, data->win, data->gc, x, y, text, length);
}

static void
update_menu_window( PMenuSysData XX, PMenuWindow w)
{
	int x, y = 2 + 2, startx;
	Bool vertical = w != &XX-> wstatic;
	PMenuItemReg m = w->m;
	PUnixMenuItem ix;
	int lastIncOk = 1;
	PCachedFont kf = XX-> font;
	uint32_t *xft_map8 = nil;

#ifdef USE_XFT
	{
		PWindow owner = ( PWindow)(PComponent( w-> self)-> owner);
		const char * encoding = ( XX-> type. popup) ?
			owner-> popupFont. encoding :
			owner-> menuFont. encoding;
		xft_map8 = prima_xft_map8( encoding);
	}
#endif


	free_unix_items( w);
	w-> num = item_count( w);
	ix = w-> um = malloc( sizeof( UnixMenuItem) * w-> num);
	if ( !ix) return;
	bzero( w-> um, sizeof( UnixMenuItem) * w-> num);

	startx = x = vertical ? MENU_XOFFSET * 4 + MENU_CHECK_XOFFSET * 2 : 0;

	if ( vertical) w-> last = -1;
	w-> selected = -100;
	while ( m) {
		if ( m-> flags. divider) {
			ix-> height = vertical ? MENU_ITEM_GAP * 2 : 0;
		} else {
			int l = 0;
			if ( m-> text) {
				int i, ntildas = 0;
				char * t = m-> text;
				ix-> height = MENU_ITEM_GAP * 2 + kf-> font. height;
				for ( i = 0; t[i]; i++) {
					if ( t[i] == '~' && t[i+1]) {
						ntildas++;
						if ( t[i+1] == '~')
							i++;
					}
				}
				ix-> width += startx + get_text_width( kf, m-> text, i,
					m-> flags. utf8_text, xft_map8);
				if ( ntildas)
					ix-> width -= ntildas * get_text_width( kf, "~", 1, false, xft_map8);
			} else if ( m-> bitmap && PObject( m-> bitmap)-> stage < csDead) {
				Pixmap px = prima_std_pixmap( m-> bitmap, CACHE_LOW_RES);
				if ( px) {
					PImage i = ( PImage) m-> bitmap;
					ix-> height += ( i-> h < kf-> font. height) ?  kf-> font. height : i-> h +
						MENU_ITEM_GAP * 2;
					if ( ix-> height > guts. displaySize. y - kf-> font. height - MENU_ITEM_GAP * 3 - 4)
					ix-> height = guts. displaySize. y - kf-> font. height - MENU_ITEM_GAP * 3 - 4;
					ix-> width  += i-> w + startx;
					ix-> pixmap = px;
				}
			}
			if ( m-> accel && ( l = strlen( m-> accel))) {
				ix-> accel_width = get_text_width( kf, m-> accel, l,
					m-> flags. utf8_accel, xft_map8);
			}
			if ( ix-> accel_width + ix-> width > x) x = ix-> accel_width + ix-> width;
		}

		if ( vertical && lastIncOk &&
			y + ix-> height + MENU_ITEM_GAP * 2 + kf-> font. height > guts. displaySize. y) {
			lastIncOk = 0;
			y += MENU_ITEM_GAP * 2 + kf-> font. height;
		}
		m = m-> next;
		if ( lastIncOk) {
			y += ix-> height;
			w-> last++;
		}
		ix++;
		if ( !lastIncOk) break;
	}

	if ( vertical) {
		if ( x > guts. displaySize. x - 64) x = guts. displaySize. x - 64;
		w-> sz.x = x;
		w-> sz.y = y;
		XResizeWindow( DISP, w-> w, x, y);
	}
}

static int
menu_point2item( PMenuSysData XX, PMenuWindow w, int x, int y, PMenuItemReg * m_ptr)
{
	int l = 0, r = 0, index = 0;
	PMenuItemReg m;
	PUnixMenuItem ix;
	if ( !w) return -1;
	m = w-> m;
	ix = w-> um;
	if ( !ix) return -1;
	if ( w == &XX-> wstatic) {
		int right = w-> right;
		l = r = 0;
		if ( x < l) return -1;
		while ( m) {
			if ( m-> flags. divider) {
				if ( right > 0) {
					r += right;
					right = 0;
				}
				if ( x < r) return -1;
			} else {
				if ( index <= w-> last) {
					r += MENU_XOFFSET * 2 + ix-> width;
					if ( m-> accel) r += MENU_XOFFSET/2 + ix-> accel_width;
				} else
					r += MENU_XOFFSET * 2 + XX-> guillemots;
				if (x >= l && x <= r) {
					if ( m_ptr) *m_ptr = m;
					return index;
				}
				if ( index > w-> last) return -1;
			}
			l = r;
			index++;
			ix++;
			m = m-> next;
		}
	} else {
		l = r = 2;
		if ( y < l) return -1;
		while ( m) {
			if ( index > w-> last) {
				r += MENU_ITEM_GAP * 2 + XX-> font-> font. height;
				goto CHECK;
			} else if ( m-> flags. divider) {
				r += MENU_ITEM_GAP * 2;
				if ( y < r) return -1;
			} else {
				r += ix-> height;
			CHECK:
				if ( y >= l && y <= r) {
					if ( m_ptr) *m_ptr = m;
					return index;
				}
				if ( index > w-> last) return -1;
			}
			l = r;
			index++;
			ix++;
			m = m-> next;
		}
	}
	return -1;
}

static Point
menu_item_offset( PMenuSysData XX, PMenuWindow w, int index)
{
	Point ret = {0,0};
	PMenuItemReg m = w-> m;
	PUnixMenuItem ix = w-> um;
	if ( index < 0 || !ix || !m) return ret;
	if ( w == &XX-> wstatic) {
		int right = w-> right;
		while ( m && index--) {
			if ( m-> flags. divider) {
				if ( right > 0) {
					ret. x += right;
					right = 0;
				}
			} else {
				ret. x += MENU_XOFFSET * 2 + ix-> width;
				if ( m-> accel) ret. x += MENU_XOFFSET / 2 + ix-> accel_width;
			}
			ix++;
			m = m-> next;
		}
	} else {
		ret. y = 2;
		ret. x = 2;
		while ( m && index--) {
			ret. y += ix-> height;
			ix++;
			m = m-> next;
		}
	}
	return ret;
}

static Point
menu_item_size( PMenuSysData XX, PMenuWindow w, int index)
{
	PMenuItemReg m = w-> m;
	PUnixMenuItem ix;
	Point ret = {0,0};
	if ( index < 0 || !w-> um || !m) return ret;
	if ( w == &XX-> wstatic) {
		if ( index >= 0 && index <= w-> last) {
			ix = w-> um + index;
			while ( index--) m = m-> next;
			if ( m-> flags. divider) return ret;
			ret. x = MENU_XOFFSET * 2 + ix-> width;
			if ( m-> accel) ret. x += MENU_XOFFSET / 2+ ix-> accel_width;
		} else if ( index == w-> last + 1) {
			ret. x = MENU_XOFFSET * 2 + XX-> guillemots;
		} else
			return ret;
		ret. y = XX-> font-> font. height + MENU_ITEM_GAP * 2;
	} else {
		if ( index >= 0 && index <= w-> last) {
			ix = w-> um + index;
			ret. y = ix-> height;
		} else if ( index == w-> last + 1) {
			ret. y = 2 * MENU_ITEM_GAP + XX-> font-> font. height;
		} else
			return ret;
		ret. x = w-> sz. x - 4;
	}
	return ret;
}

static void
menu_select_item( PMenuSysData XX, PMenuWindow w, int index)
{
	if ( index != w-> selected) {
		XRectangle r;
		Point p1 = menu_item_offset( XX, w, index);
		Point p2 = menu_item_offset( XX, w, w-> selected );
		Point s1 = menu_item_size( XX, w, index);
		Point s2 = menu_item_size( XX, w, w-> selected );
		if ( s1.x == 0 && s1.y == 0) {
			if ( s2.x == 0 && s2.y == 0) return;
			r.x = p2.x; r.y = p2.y;
			r.width = s2.x; r.height = s2.y;
		} else if ( s2.x == 0 && s2.y == 0) {
			r.x = p1.x; r.y = p1.y;
			r.width = s1.x; r.height = s1.y;
		} else {
			r. x = ( p1. x < p2. x) ? p1. x : p2. x;
			r. y = ( p1. y < p2. y) ? p1. y : p2. y;
			r. width  = ( p1.x + s1.x > p2.x + s2.x) ? p1.x + s1.x - r.x : p2.x + s2.x - r.x;
			r. height = ( p1.y + s1.y > p2.y + s2.y) ? p1.y + s1.y - r.y : p2.y + s2.y - r.y;
		}
		w-> selected = ( index < 0) ? -100 : index;
		XClearArea( DISP, w-> w, r.x, r.y, r.width, r.height, true);
		XX-> paint_pending = true;
	}
}

static Bool
send_cmMenu( Handle self, PMenuItemReg m)
{
	Event ev;
	Handle owner = PComponent( self)-> owner;
	bzero( &ev, sizeof( ev));
	ev. cmd = cmMenu;
	ev. gen. H = self;
	ev. gen. i = m ? m-> id : 0;
	CComponent(owner)-> message( owner, &ev);
	if ( PComponent( owner)-> stage == csDead ||
			PComponent( self)->  stage == csDead) return false;
	if ( self != guts. currentMenu) return false;
	return true;
}

static Bool
menu_enter_item( PMenuSysData XX, PMenuWindow w, int index, int type)
{
	PMenuItemReg m = w-> m;
	int index2 = index, div = 0;

	XX-> focused = w;

	if ( index < 0 || index > w-> last + 1 || !w-> um || !m) return false;
	while ( index2--) {
		if ( m-> flags. divider) div = 1;
		m = m-> next;
	}
	if ( index == w-> last + 1) div = 0;

	if ( m-> flags. disabled && index <= w-> last) return false;

	if ( m-> down || index == w-> last + 1) {
		PMenuWindow w2;
		Point p, s, n = w-> pos;

		if ( w-> next && w-> next-> m == m-> down) {
			XX-> focused = w-> next;
			XMapRaised( DISP, w-> next-> w);
			XFlush(DISP);
			return true;
		}

		if ( index != w-> last + 1) {
			if ( !send_cmMenu( w-> self, m)) return false;
			m = m-> down;
		}

		menu_window_delete_downlinks( XX, w);
		w2 = get_window( w-> self, m);
		if ( !w2) return false;

		update_menu_window( XX, w2);
		p = menu_item_offset( XX, w, index);
		s = menu_item_size( XX, w, index);

		if ( &XX-> wstatic == w) {
			XWindow dummy;
			XTranslateCoordinates( DISP, w->w, guts. root, 0, 0, &n.x, &n.y, &dummy);
			w-> pos = n;
		}

		n. x += p. x;
		n. y += p. y;
		p. x += w-> pos. x;
		p. y += w-> pos. y;
		if ( &XX-> wstatic == w) {
			if ( div) n. x -= w2-> sz. x - s. x;
			if ( p.y + s.y + w2-> sz.y <= guts. displaySize.y)
				n. y = p. y + s. y;
			else if ( w2-> sz.y <= p. y)
				n. y = p. y - w2-> sz. y;
			else
				n. y = 0;
			if ( n. x + w2-> sz. x > guts. displaySize. x)
				n. x = guts. displaySize. x - w2-> sz. x;
			else if ( n. x < 0)
				n. x = 0;
		} else {
			div = 0;
			if ( p.y + w2-> sz.y <= guts. displaySize.y)
				n. y = p. y;
			else if ( w2-> sz.y <= p. y + s. y)
				n. y = p. y + s. y - w2-> sz. y;
			else
				n. y = 0;
			if ( p.x + s. x + w2-> sz.x <= guts. displaySize.x)
				n. x = p. x + s. x;
			else if ( w2-> sz.x <= p.x)
				n. x = p. x - w2-> sz. x;
			else {
				n. x = 0;
				if ( p.y + w2-> sz.y + s.y <= guts. displaySize.y)
					n. y += s.y;
				else if ( w2-> sz.y <= p. y)
					n. y -= s.y;
			}
		}
		XMoveWindow( DISP, w2-> w, n. x, n. y);
		XMapRaised( DISP, w2-> w);
		w2-> pos = n;
		XX-> focused = w2;
		XFlush(DISP);
	} else {
		Handle self = w-> self;
		if (( &XX-> wstatic == w) && ( type == 0)) {
			menu_window_delete_downlinks( XX, w);
			return true;
		}
		prima_end_menu();
		CAbstractMenu( self)-> sub_call( self, m);
		return false;
	}
	return true;
}


static void
store_char( char * src, int srclen, int * srcptr, char * dst, int * dstptr, Bool utf8, MenuDrawRec * data)
{
	if ( *srcptr >= srclen ) return;

	if ( utf8) {
		STRLEN char_len;
		UV uv =
#if PERL_PATCHLEVEL >= 16
			utf8_to_uvchr_buf(( U8*) src + *srcptr, ( U8*) src + srclen, &char_len)
#else
			utf8_to_uvchr(( U8*) src + *srcptr, &char_len)
#endif
		;
		*srcptr += char_len;
		if ( data-> xft_map8) {
			*(( uint32_t*)(dst + *dstptr)) = (uint32_t) uv;
			*dstptr += 4;
		} else {
			(( XChar2b*)(dst + *dstptr))-> byte1 = uv >> 8;
			(( XChar2b*)(dst + *dstptr))-> byte2 = uv & 0xff;
			*dstptr += 2;
		}
	} else {
		if ( data-> xft_map8) {
			uint32_t c = (( U8*) src)[ *srcptr];
			if ( c > 127)
				c = data-> xft_map8[ c - 128];
			*(( uint32_t*)(dst + *dstptr)) = c;
			(*dstptr) += 4;
			(*srcptr)++;
		} else {
			dst[(*dstptr)++] = src[(*srcptr)++];
		}
	}
}

void
prima_handle_menu_event( XEvent *ev, XWindow win, Handle self)
{
	switch ( ev-> type) {
	case Expose: {
		DEFMM;
		PMenuWindow w;
		PUnixMenuItem ix;
		PMenuItemReg m;
		int mx, my;
		Bool vertical;
		int sz = 1024, l, i, slen, x, y;
		char *s;
		char *t;
		int right, last = 0;
		int xtoffs, descent;
		PCachedFont kf;
		MenuDrawRec draw;
		unsigned long clr;
		Color rgb;
		PWindow owner;

		kf = XX-> font;
		XX-> paint_pending = false;
		if ( XX-> wstatic. w == win) {
			w = XX-> w;
			vertical = false;
		} else {
			if ( !( w = get_menu_window( self, win))) return;
			vertical = true;
		}
		right  = vertical ? 0 : w-> right;
		xtoffs = vertical ? MENU_CHECK_XOFFSET : 0;
		m  = w-> m;
		mx = w-> sz.x - 1;
		my = w-> sz.y - 1;
		ix = w-> um;
		if ( !ix) return;

		owner = ( PWindow)(PComponent( w-> self)-> owner);
		bzero( &draw, sizeof( draw));
		draw. win = win;
		draw. layered = vertical ? false : X(owner)->flags. layered;
		if ( draw.layered ) {
			XGCValues gcv;
			gcv. graphics_exposures = false;
			draw. gc = XCreateGC( DISP, X(owner)->gdrawable, GCGraphicsExposures, &gcv);
			draw. c  = XX->argb_c;
		} else {
			draw. gc = guts. menugc;
			draw. c  = XX->c;
		}
#ifdef USE_XFT
		if ( kf-> xft) {
			char * encoding = ( XX-> type. popup) ?
				owner-> popupFont. encoding :
				owner-> menuFont. encoding;
			draw. xft_map8 = prima_xft_map8( encoding);
			draw. xft_drawable = XftDrawCreate( DISP, win,
				draw.layered ? guts. argb_visual. visual : guts. visual. visual,
				draw.layered ? guts. argbColormap : guts. defaultColormap);
			descent = kf-> xft-> descent;
		} else
#endif
		descent = kf-> fs->max_bounds.descent;

		{
			XRectangle r;
			Region rgn;
			r. x = ev-> xexpose. x;
			r. y = ev-> xexpose. y;
			r. width = ev-> xexpose. width;
			r. height = ev-> xexpose. height;
			rgn = XCreateRegion();
			XUnionRectWithRegion( &r, rgn, rgn);
			XSetRegion( DISP, draw.gc, rgn);
#ifdef USE_XFT
			if ( draw. xft_drawable) XftDrawSetClip(( XftDraw*) draw.xft_drawable, rgn);
#endif
			XDestroyRegion( rgn);
		}

#ifdef USE_XFT
		if ( !kf-> xft)
#endif
			XSetFont( DISP, draw.gc, kf-> id);
		XSetForeground( DISP, draw.gc, draw.c[ciBack]);
		XSetBackground( DISP, draw.gc, draw.c[ciBack]);
		if ( vertical) {
			XFillRectangle( DISP, win, draw.gc, 2, 2, mx-1, my-1);
			XSetForeground( DISP, draw.gc, draw.c[ciDark3DColor]);
			XDrawLine( DISP, win, draw.gc, 0, 0, 0, my);
			XDrawLine( DISP, win, draw.gc, 0, 0, mx-1, 0);
			XDrawLine( DISP, win, draw.gc, mx-1, my-1, 2, my-1);
			XDrawLine( DISP, win, draw.gc, mx-1, my-1, mx-1, 1);
			XSetForeground( DISP, draw.gc, guts. monochromeMap[0]);
			XDrawLine( DISP, win, draw.gc, mx, my, 1, my);
			XDrawLine( DISP, win, draw.gc, mx, my, mx, 0);
			XSetForeground( DISP, draw.gc, draw.c[ciLight3DColor]);
			XDrawLine( DISP, win, draw.gc, 1, 1, 1, my-1);
			XDrawLine( DISP, win, draw.gc, 1, 1, mx-2, 1);
		} else
			XFillRectangle( DISP, win, draw.gc, 0, 0, w-> sz.x, w-> sz.y);
		y = vertical ? 2 : 0;
		x = 0;
		if ( !( s = malloc( sz))) goto EXIT;
		while ( m) {
			Bool selected = false;

			/* printf("%d %d %d %s\n", last, w-> selected, w-> last, m-> text); */
			if ( last == w-> selected) {
				Point sz = menu_item_size( XX, w, last);
				Point p = menu_item_offset( XX, w, last);
				XSetForeground( DISP, draw.gc, draw.c[ciHilite]);
				XFillRectangle( DISP, win, draw.gc, p.x, p.y, sz. x, sz.y);
				clr = draw.c[ciHiliteText];
				rgb = XX-> rgb[ciHiliteText];
				selected = true;
			} else {
				clr = draw.c[ciFore];
				rgb = XX-> rgb[ciFore];
			}

			if ( last > w-> last) {
				char buf[8];
				int s = 0, d = 0;
				draw. pixel = clr;
				draw. rgb   = rgb;
				store_char( ">", strlen(">"), &s, buf, &d, 0, &draw);
				s = 0;
				store_char( ">", strlen(">"), &s, buf, &d, 0, &draw);
				text_out( kf, buf, 2,
						x + MENU_XOFFSET + xtoffs,
						y + MENU_ITEM_GAP + kf-> font. height - kf-> font. descent,
						false, &draw);
				break;
			}

			if ( m-> flags. divider) {
				if ( vertical) {
					y += MENU_ITEM_GAP - 1;
					XSetForeground( DISP, draw.gc, draw.c[ciDark3DColor]);
					XDrawLine( DISP, win, draw.gc, 1, y, mx-1, y);
					y++;
					XSetForeground( DISP, draw.gc, draw.c[ciLight3DColor]);
					XDrawLine( DISP, win, draw.gc, 1, y, mx-1, y);
					y += MENU_ITEM_GAP;
				} else if ( right > 0) {
					x += right;
					right = 0;
				}
			} else {
				int deltaY = 0;

				if ( m-> flags. disabled) {
					clr = draw.c[ciDisabledText];
					rgb = XX-> rgb[ ciDisabledText];
				}

				deltaY = ix-> height;
				if ( m-> text) {
					int lineStart, lineEnd = 0, haveDash = 0;
					int ay = y + deltaY - MENU_ITEM_GAP - kf-> font. descent;
					int text_len = strlen(m-> text);

					t = m-> text;
					for (;;) {
						l = i = slen = lineEnd = haveDash = 0;
						lineStart = -1;
						while ( l < sz - 1 && t[i]) {
							if (t[i] == '~' && t[i+1]) {
								if ( t[i+1] == '~') {
									int dummy = 0;
									store_char( "~", strlen("~"), &dummy, s, &l, false, &draw);
									i += 2;
									slen++;
								} else {
									if ( !haveDash) {
										haveDash = 1;
										lineEnd = lineStart + 1 +
											get_text_width( kf, t + i + 1, 1,
												m-> flags. utf8_text, draw. xft_map8);
									}
									i++;
								}
							} else {
								if ( !haveDash) {
									FontABC abc;
									get_font_abc( kf, t+i, m-> flags. utf8_text, &abc, &draw);
									if ( lineStart < 0)
										lineStart = ( abc.a < 0) ? -abc.a : 0;
									lineStart += abc.a + abc.b + abc.c;
								}
								store_char( t, text_len, &i, s, &l, m-> flags. utf8_text, &draw);
								slen++;
							}
						}
						if ( t[i]) {
							free(s);
							if ( !( s = malloc( sz *= 2))) goto EXIT;
						} else
							break;
					}
					if ( m-> flags. disabled && !selected) {
						draw. pixel = draw.c[ciLight3DColor];
						draw. rgb   = XX->rgb[ciLight3DColor];
						text_out( kf, s, slen, x+MENU_XOFFSET+xtoffs+1, ay+1,
							m-> flags. utf8_text, &draw);
					}
					draw. pixel = clr;
					draw. rgb   = rgb;
					text_out( kf, s, slen, x+MENU_XOFFSET+xtoffs, ay, m-> flags. utf8_text, &draw);
					if ( haveDash) {
						if ( m-> flags. disabled && !selected) {
							XSetForeground( DISP, draw.gc, draw.c[ciLight3DColor]);
							XDrawLine( DISP, win, draw.gc, x+MENU_XOFFSET+xtoffs+lineStart+1, ay+descent-1+1,
								x+MENU_XOFFSET+xtoffs+lineEnd+1, ay+descent-1+1);
						}
						XSetForeground( DISP, draw.gc, clr);
						XDrawLine( DISP, win, draw.gc, x+MENU_XOFFSET+xtoffs+lineStart, ay+descent-1,
							x+MENU_XOFFSET+xtoffs+lineEnd, ay+descent-1);
					}
				} else if ( m-> bitmap && ix-> pixmap) {
					if ( selected) XSetFunction( DISP, draw.gc, GXcopyInverted);
					XCopyArea( DISP, ix-> pixmap, win, draw.gc, 0, 0, ix-> width, ix-> height, x+MENU_XOFFSET+xtoffs, y + MENU_ITEM_GAP);
					if ( selected) XSetFunction( DISP, draw.gc, GXcopy);
				}
				if ( !vertical) x += ix-> width + MENU_XOFFSET;

				if ( m-> accel) {
					int i, ul = 0, sl = 0, dl = 0;
					int zx = vertical ?
						mx - MENU_XOFFSET - MENU_CHECK_XOFFSET - ix-> accel_width :
						x + MENU_XOFFSET/2;
					int zy = vertical ?
						y + ( deltaY + kf-> font. height) / 2 - kf-> font. descent:
						y + deltaY - MENU_ITEM_GAP - kf-> font. descent;
					int accel_len = strlen(m-> accel);

					if ( m-> flags. utf8_accel)
						ul = prima_utf8_length( m-> accel);
					else
						ul = accel_len;
					if (( ul * 4 + 4) > sz) {
						free(s);
						if ( !( s = malloc( sz = (ul * 4 + 4)))) goto EXIT;
					}
					for ( i = 0; i < ul; i++)
						store_char( m-> accel, accel_len, &sl, s, &dl, m-> flags. utf8_accel, &draw);
					if ( m-> flags. disabled && !selected) {
						draw. pixel = draw.c[ciLight3DColor];
						draw. rgb   = XX->rgb[ciLight3DColor];
						text_out( kf, s, ul, zx + 1, zy + 1,
							m-> flags. utf8_accel, &draw);
					}
					draw. pixel = clr;
					draw. rgb   = rgb;
					text_out( kf, s, ul, zx, zy,
						m-> flags. utf8_accel, &draw);
					if ( !vertical)
						x += ix-> accel_width + MENU_XOFFSET/2;
				}
				if ( !vertical) x += MENU_XOFFSET;

				if ( vertical && m-> down) {
					int ave    = kf-> font. height * 0.4;
					int center = y + deltaY / 2;
					XPoint p[3];
					p[0].x = mx - MENU_CHECK_XOFFSET/2;
					p[0].y = center;
					p[1].x = mx - ave - MENU_CHECK_XOFFSET/2;
					p[1].y = center - ave * 0.6;
					p[2].x = mx - ave - MENU_CHECK_XOFFSET/2;
					p[2].y = center + ave * 0.6 + 1;
					if ( m-> flags. disabled && !selected) {
						int i;
						XSetForeground( DISP, draw.gc, draw.c[ciLight3DColor]);
						for ( i = 0; i < 3; i++) {
							p[i].x++;
							p[i].y++;
						}
						XFillPolygon( DISP, win, draw.gc, p, 3, Nonconvex, CoordModeOrigin);
						for ( i = 0; i < 3; i++) {
							p[i].x--;
							p[i].y--;
						}
					}
					XSetForeground( DISP, draw.gc, clr);
					XFillPolygon( DISP, win, draw.gc, p, 3, Nonconvex, CoordModeOrigin);
				}
				if ( m-> flags. checked && vertical) {
					/* don't draw check marks on horizontal menus - they look ugly */
					int bottom = y + deltaY - MENU_ITEM_GAP - ix-> height * 0.2;
					int ax = x + MENU_XOFFSET / 2;
					XGCValues gcv;
					gcv. line_width = 3;
					XChangeGC( DISP, draw.gc, GCLineWidth, &gcv);
					if ( m-> flags. disabled && !selected) {
						XSetForeground( DISP, draw.gc, draw.c[ciLight3DColor]);
						XDrawLine( DISP, win, draw.gc, ax + 1 + 1 , y + deltaY / 2 + 1, ax + MENU_XOFFSET - 2 + 1, bottom - 1);
						XDrawLine( DISP, win, draw.gc, ax + MENU_XOFFSET - 2 + 1, bottom + 1, ax + MENU_CHECK_XOFFSET + 1, y + MENU_ITEM_GAP + ix-> height * 0.2);
					}
					XSetForeground( DISP, draw.gc, clr);
					XDrawLine( DISP, win, draw.gc, ax + 1, y + deltaY / 2, ax + MENU_XOFFSET - 2, bottom);
					XDrawLine( DISP, win, draw.gc, ax + MENU_XOFFSET - 2, bottom, ax + MENU_CHECK_XOFFSET, y + MENU_ITEM_GAP + ix-> height * 0.2);
					gcv. line_width = 1;
					XChangeGC( DISP, draw.gc, GCLineWidth, &gcv);
				}
				if ( vertical) y += deltaY;
			}
			m = m-> next;
			ix++;
			last++;
		}
		free(s);
	EXIT:;
#ifdef USE_XFT
		if ( draw. xft_drawable)
			XftDrawDestroy( draw. xft_drawable);
#endif
		if ( draw. layered ) XFreeGC( DISP, draw. gc);
		XFlush(DISP);
	}
	break;
	case ConfigureNotify: {
		DEFMM;
		if ( XX-> wstatic. w == win) {
			PMenuWindow  w = XX-> w;
			PMenuItemReg m;
			PUnixMenuItem ix;
			int x = 0;
			int stage = 0;
			if ( w-> sz. x == ev-> xconfigure. width &&
				w-> sz. y == ev-> xconfigure. height) return;
			if ( guts. currentMenu == self) prima_end_menu();
			w-> sz. x = ev-> xconfigure. width;
			w-> sz. y = ev-> xconfigure. height;
			XClearArea( DISP, win, 0, 0, 0, 0, true);

AGAIN:
			w-> last = -1;
			m = w-> m;
			ix = w-> um;
			while ( m) {
				int dx = 0;
				if ( !m-> flags. divider) {
					dx += MENU_XOFFSET * 2 + ix-> width;
					if ( m-> accel) dx += MENU_XOFFSET / 2 + ix-> accel_width;
				}
				if ( x + dx >= w-> sz.x) {
					if ( stage == 0) { /* now we are sure that >> should be drawn - check again */
						x = MENU_XOFFSET * 2 + XX-> guillemots;
						stage++;
						goto AGAIN;
					}
					break;
				}
				x += dx;
				w-> last++;
				m = m-> next;
				ix++;
			}
			m = w-> m;
			ix = w-> um;
			w-> right = 0;
			if ( w-> last >= w-> num - 1) {
				Bool hit = false;
				x = 0;
				while ( m) {
					if ( m-> flags. divider) {
						hit = true;
						break;
					} else {
						x += MENU_XOFFSET * 2 + ix-> width;
						if ( m-> accel) x += MENU_XOFFSET / 2 + ix-> accel_width;
					}
					m = m-> next;
					ix++;
				}
				if ( hit) {
					w-> right = 0;
					while ( m) {
						if ( !m-> flags. divider) {
							w-> right += MENU_XOFFSET * 2 + ix-> width;
							if ( m-> accel) w-> right += MENU_XOFFSET / 2 + ix-> accel_width;
						}
						m = m-> next;
						ix++;
					}
					w-> right = w-> sz.x - w-> right - x;
				}
			}
		}
	}
	break;
	case ButtonPress:
	case ButtonRelease: {
		DEFMM;
		int px, first = 0;
		PMenuWindow w;
		XWindow focus = nilHandle;
		if ( prima_no_input( X(PComponent(self)->owner), false, true)) return;
		if ( ev-> xbutton. button != Button1) return;

		if ( XX-> wstatic. w == win) {
			Handle x = guts. focused, owner = PComponent(self)-> owner;
			while ( x && !X(x)-> type. window) x = PComponent( x)-> owner;
			if ( x != owner) {
				XSetInputFocus( DISP, focus = PComponent( owner)-> handle,
					RevertToNone, ev-> xbutton. time);
			}
		}

		if ( !( w = get_menu_window( self, win))) {
			prima_end_menu();
			return;
		}
		px = menu_point2item( XX, w, ev-> xbutton. x, ev-> xbutton.y, nil);
		if ( px < 0) {
			if ( XX-> wstatic. w == win)
				prima_end_menu();
			return;
		}
		if ( guts. currentMenu != self) {
			int rev;
			if ( ev-> type == ButtonRelease) return;
			if ( guts. currentMenu)
				prima_end_menu();
			if ( focus)
				XX-> focus = focus;
			else
				XGetInputFocus( DISP, &XX-> focus, &rev);
			if ( !XX-> type. popup) {
				Handle topl = PComponent( self)-> owner;
				Handle who  = ( Handle) hash_fetch( guts.windows, (void*)&XX-> focus, sizeof(XX-> focus));
				while ( who) {
					if ( XT_IS_WINDOW(X(who))) {
						if ( who != topl) XX-> focus = PComponent( topl)-> handle;
						break;
					}
					who = PComponent( who)-> owner;
				}
			}
			first = 1;
		}
		XSetInputFocus( DISP, XX-> w-> w, RevertToNone, CurrentTime);
		guts. currentMenu = self;
		if ( first && ( ev-> type == ButtonPress) && ( !send_cmMenu( self, nil)))
			return;
		if ( !first && ( ev-> type == ButtonPress)) return;
		apc_timer_stop( MENU_TIMER);
		menu_select_item( XX, w, px);
		if ( !ev-> xbutton. send_event) {
			if ( !menu_enter_item( XX, w, px, ( ev-> type == ButtonPress) ? 0 : 1))
				return;
		} else
			XX-> focused = w;

		ev-> xbutton. x += w-> pos. x;
		ev-> xbutton. y += w-> pos. y;
		if ( w-> next &&
			ev-> xbutton. x >= w-> next-> pos.x &&
			ev-> xbutton. y >= w-> next-> pos.y &&
			ev-> xbutton. x <  w-> next-> pos.x + w-> next-> sz.x &&
			ev-> xbutton. y <  w-> next-> pos.y + w-> next-> sz.y)
			{ /* simulate mouse move, as X are stupid enough to not do it  */
			int x = ev-> xbutton.x, y = ev-> xbutton. y;
			ev-> xmotion. x = x - w-> next-> pos. x;
			ev-> xmotion. y = y - w-> next-> pos. y;
			win = w-> next-> w;
			goto MOTION_NOTIFY;
		}
	}
	break;
	MOTION_NOTIFY:
	case MotionNotify: if ( guts. currentMenu == self) {
		DEFMM;
		PMenuItemReg m;
		PMenuWindow w = get_menu_window( self, win);
		int px = menu_point2item( XX, w, ev-> xmotion.x, ev-> xmotion.y, nil);
		menu_select_item( XX, w, px);
		m = w-> m;
		if ( px >= 0) {
			int x = px;
			while ( x--) m = m-> next;
			if ( px != w-> last + 1) m = m-> down;
			if ( !w-> next || w-> next-> m != m) {
				apc_timer_set_timeout( MENU_TIMER, (XX-> wstatic. w == win) ? 2 : guts. menu_timeout);
				XX-> focused = w;
			}
		}
		while ( w-> next) {
			menu_select_item( XX, w-> next, -1);
			w = w-> next;
		}
	}
	break;
	case FocusOut:
		if ( self == guts. currentMenu) {
			switch ( ev-> xfocus. detail) {
			case NotifyVirtual:
			case NotifyPointer:
			case NotifyPointerRoot:
			case NotifyDetailNone:
			case NotifyNonlinearVirtual:
				return;
			}
			apc_timer_stop( MENU_UNFOCUS_TIMER);
			apc_timer_start( MENU_UNFOCUS_TIMER);
			guts. unfocusedMenu = self;
		}
		break;
	case FocusIn:
		if ( guts. unfocusedMenu && self == guts. unfocusedMenu && self == guts. currentMenu) {
			switch ( ev-> xfocus. detail) {
			case NotifyVirtual:
			case NotifyPointer:
			case NotifyPointerRoot:
			case NotifyDetailNone:
			case NotifyNonlinearVirtual:
				return;
			}
			apc_timer_stop( MENU_UNFOCUS_TIMER);
			guts. unfocusedMenu = nilHandle;
		}
		break;
	case KeyPress: {
		DEFMM;
		char str_buf[ 256];
		KeySym keysym;
		int str_len, d = 0, piles = 0;
		PMenuWindow w;
		PMenuItemReg m;

		str_len = XLookupString( &ev-> xkey, str_buf, 256, &keysym, nil);
		if ( prima_handle_menu_shortcuts( PComponent(self)-> owner, ev, keysym) != 0)
			return;

		if ( self != guts. currentMenu) return;
		apc_timer_stop( MENU_TIMER);
		if ( !XX-> focused) return;
		/* navigation */
		w = XX-> focused;
		m = w-> m;
		switch (keysym) {
		case XK_Left:
		case XK_KP_Left:
			if ( w == &XX-> wstatic) { /* horizontal menu */
				d--;
			} else if ( w != XX-> w) { /* not a popup root */
				if ( w-> prev) menu_window_delete_downlinks( XX, w-> prev);
				if ( w-> prev == &XX-> wstatic) {
					d--;
					piles = 1;
				} else
					return;
			}
			break;
		case XK_Right:
		case XK_KP_Right:
			if ( w == &XX-> wstatic) { /* horizontal menu */
				d++;
			} else if ( w-> selected >= 0) {
				int sel;
				sel = w-> selected;
				if ( sel >= 0) {
					while ( sel--) m = m-> next;
				}
				if ( m-> down || w-> selected == w-> last + 1) {
					if ( menu_enter_item( XX, w, w-> selected, 1) && w-> next)
						menu_select_item( XX, w-> next, 0);
				} else if ( w-> prev == &XX-> wstatic) {
					menu_window_delete_downlinks( XX, XX-> w);
					piles = 1;
					d++;
				} else
					return;
			}
			break;
		case XK_Up:
		case XK_KP_Up:
			if ( w != &XX-> wstatic) d--;
			break;
		case XK_Down:
		case XK_KP_Down:
			if ( w == &XX-> wstatic) {
				int sel = w-> selected;
				if ( sel >= 0) {
					while ( sel--) m = m-> next;
				}
				if ( m-> down || w-> selected == w-> last + 1) {
					if ( menu_enter_item( XX, w, w-> selected, 1) && w-> next)
						menu_select_item( XX, w-> next, 0);
				}
			} else
				d++;
			break;
		case XK_KP_Enter:
		case XK_Return:
			menu_enter_item( XX, w, w-> selected, 1);
			return;
		case XK_Escape:
			if ( w-> prev)
				menu_window_delete_downlinks( XX, w-> prev);
			else
				prima_end_menu();
			return;
		default:
			goto NEXT_STAGE;
		}

		if ( piles) w = XX-> focused = w-> prev;

		if ( d != 0) {
			int sel = w-> selected;
			PMenuItemReg m;
			int z, last = w-> last + (( w-> num == w-> last + 1) ? 0 : 1);

			if ( sel < -1) sel = -1;
			while ( 1) {
				if ( d > 0) {
					sel = ( sel >= last) ? 0 : sel + 1;
				} else {
					sel = ( sel <= 0) ? last : sel - 1;
				}
				m = w-> m;
				z = sel;
				while ( z--) m = m-> next;
				if ( sel == w-> last + 1 || !m-> flags. divider) {
					menu_select_item( XX, w, sel);
					menu_window_delete_downlinks( XX, w);
					if ( piles) {
						if ( menu_enter_item( XX, w, sel, 0) && w-> next)
							menu_select_item( XX, w-> next, 0);
					}
					break;
				}
			}

		}
		return;
NEXT_STAGE:

		if ( str_len == 1) {
			int i;
			char c = tolower( str_buf[0]);
			for ( i = 0; i <= w-> last; i++) {
				if ( m-> text) {
					int j = 0;
					char * t = m-> text, z = 0;
					while ( t[j]) {
						if ( t[j] == '~' && t[j+1]) {
							if ( t[j+1] == '~')
								j += 2;
							else {
								z = tolower(t[j+1]);
								break;
							}
						}
						j++;
					}
					if ( z == c) {
						if ( menu_enter_item( XX, w, i, 1) && w-> next)
							menu_select_item( XX, w, i);
						return;
					}
				}
				m = m-> next;
			}
		}
	}
	break;
	case MenuTimerMessage:
	if ( self == guts. currentMenu) {
		DEFMM;
		PMenuWindow w;
		PMenuItemReg m;
		int s;

		if ( !( w = XX-> focused)) return;
		m = w-> m;
		s = w-> selected;
		if ( s < 0) return;
		while ( s--) m = m-> next;
		if ( m-> down || w-> selected == w-> last + 1)
			menu_enter_item( XX, w, w-> selected, 0);
		else
			menu_window_delete_downlinks( XX, w);
	}
	break;
	}
}

/* local menu access hacks; it's good idea to have
	hot keys changeable through resources, but have no
	idea ( and desire :) how to plough throgh it */
int
prima_handle_menu_shortcuts( Handle self, XEvent * ev, KeySym keysym)
{
	int ret = 0;
	int mod =
		(( ev-> xkey. state & ShiftMask)	? kmShift : 0) |
		(( ev-> xkey. state & ControlMask)? kmCtrl  : 0) |
		(( ev-> xkey. state & Mod1Mask)	? kmAlt   : 0);

	if ( mod == kmShift && keysym == XK_F9) {
		Event e;
		bzero( &e, sizeof(e));
		e. cmd    = cmPopup;
		e. gen. B = false;
		e. gen. P = apc_pointer_get_pos( application);
		e. gen. H = self;
		apc_widget_map_points( self, false, 1, &e. gen. P);
		CComponent( self)-> message( self, &e);
		if ( PObject( self)-> stage == csDead) return -1;
		ret = 1;
	}

	if ( mod == 0 && keysym == XK_F10) {
		Handle ps = self;
		while ( PComponent( self)-> owner) {
			ps = self;
			if ( XT_IS_WINDOW(X(self))) break;
			self = PComponent( self)-> owner;
		}
		self = ps;

		if ( XT_IS_WINDOW(X(self)) && PWindow(self)-> menu) {
			if ( !guts. currentMenu) {
				XEvent ev;
				bzero( &ev, sizeof( ev));
				ev. type = ButtonPress;
				ev. xbutton. button = Button1;
				ev. xbutton. send_event = true;
				prima_handle_menu_event( &ev, M(PWindow(self)-> menu)-> w-> w, PWindow(self)-> menu);
			} else
				prima_end_menu();
			ret = 1;
		}
	}

	if ( !guts. currentMenu && mod == kmAlt) {   /* handle menu bar keys */
		KeySym keysym;
		char str_buf[ 256];
		Handle ps = self;

		while ( PComponent( self)-> owner) {
			ps = self;
			if ( XT_IS_WINDOW(X(self))) break;
			self = PComponent( self)-> owner;
		}
		self = ps;

		if ( XT_IS_WINDOW(X(self)) && PWindow(self)-> menu &&
				1 == XLookupString( &ev-> xkey, str_buf, 256, &keysym, nil)) {
			int i;
			PMenuSysData selfxx = M(PWindow(self)-> menu);
			char c = tolower( str_buf[0]);
			PMenuWindow w = XX-> w;
			PMenuItemReg m = w-> m;

			for ( i = 0; i <= w-> last; i++) {
				if ( m-> text) {
					int j = 0;
					char * t = m-> text, z = 0;
					while ( t[j]) {
						if ( t[j] == '~' && t[j+1]) {
							if ( t[j+1] == '~')
								j += 2;
							else {
								z = tolower(t[j+1]);
								break;
							}
						}
						j++;
					}
					if ( z == c) {
						XEvent ev;
						bzero( &ev, sizeof( ev));
						ev. type = ButtonPress;
						ev. xbutton. button = Button1;
						ev. xbutton. send_event = true;
						prima_handle_menu_event( &ev, w-> w, PWindow(self)-> menu);
						if ( menu_enter_item( XX, w, i, 1) && w-> next)
							menu_select_item( XX, w, i);
						return 1;
					}
				}
				m = m-> next;
			}
		}
	}
	return ret;
}

void
prima_end_menu(void)
{
	PMenuSysData XX;
	PMenuWindow w;
	apc_timer_stop( MENU_TIMER);
	apc_timer_stop( MENU_UNFOCUS_TIMER);
	guts. unfocusedMenu = nilHandle;
	if ( !guts. currentMenu) return;
	XX = M(guts. currentMenu);
	{
		XRectangle r;
		Region rgn;
		unsigned long * c = XX->layered ? XX->argb_c : XX->c;
		r. x = 0;
		r. y = 0;
		r. width  = guts. displaySize. x;
		r. height = guts. displaySize. y;
		rgn = XCreateRegion();
		XUnionRectWithRegion( &r, rgn, rgn);
		XSetRegion( DISP, guts. menugc, rgn);
		XDestroyRegion( rgn);
		XSetForeground( DISP, guts. menugc, c[ciBack]);
	}
	w = XX-> w;
	if ( XX-> focus)
		XSetInputFocus( DISP, XX-> focus, RevertToNone, CurrentTime);
	menu_window_delete_downlinks( XX, XX-> w);
	XX-> focus = nilHandle;
	XX-> focused = nil;
	if ( XX-> w != &XX-> wstatic) {
		hash_delete( guts. menu_windows, &w-> w, sizeof( w-> w), false);
		XDestroyWindow( DISP, w-> w);
		free_unix_items( w);
		free( w);
		XX-> w = nil;
	} else {
		XX-> w-> next = nil;
		menu_select_item( XX, XX-> w, -100);
	}
	guts. currentMenu = nilHandle;
	XFlush(DISP);
}

static unsigned long
argb_color(Color color)
{
	int a[3];

	a[0] = COLOR_R(color);
	a[1] = COLOR_G(color);
	a[2] = COLOR_B(color);

	return
		(((a[0] << guts. argb_bits. red_range  ) >> 8) << guts. argb_bits.   red_shift) |
		(((a[1] << guts. argb_bits. green_range) >> 8) << guts. argb_bits. green_shift) |
		(((a[2] << guts. argb_bits. blue_range ) >> 8) << guts. argb_bits.  blue_shift) |
		(((0xff << guts. argb_bits. alpha_range ) >> 8) << guts. argb_bits. alpha_shift);
}

Bool
apc_menu_create( Handle self, Handle owner)
{
	DEFMM;
	int i;
	apc_menu_destroy( self);
	XX-> type.menu = true;
	XX-> w         = &XX-> wstatic;
	XX-> w-> self  = self;
	XX-> w-> m     = TREE;
	XX-> w-> first = 0;
	XX-> w-> sz.x  = 0;
	XX-> w-> sz.y  = 0;
	for ( i = 0; i <= ciMaxId; i++)
		XX-> c[i] = prima_allocate_color(
			nilHandle,
			prima_map_color( PWindow(owner)-> menuColor[i], nil),
			nil);
	XX-> layered = X(owner)->flags. layered;
	if ( XX-> layered ) {
		for ( i = 0; i <= ciMaxId; i++)
			XX-> argb_c[i] = argb_color(
				prima_map_color( PWindow(owner)-> menuColor[i], nil)
			);
	}
	apc_menu_set_font( self, &PWindow(owner)-> menuFont);
	return true;
}

Bool
apc_menu_destroy( Handle self)
{
	if ( guts. currentMenu == self) prima_end_menu();
	return true;
}

PFont
apc_menu_default_font( PFont f)
{
	memcpy( f, &guts. default_menu_font, sizeof( Font));
	return f;
}

Color
apc_menu_get_color( Handle self, int index)
{
	Color c;
	if ( index < 0 || index > ciMaxId) return clInvalid;
	c = M(self)-> c[index];
	if ( guts. palSize > 0)
		return guts. palette[c]. composite;
	return
		((((c & guts. visual. blue_mask)  >> guts. screen_bits. blue_shift) << 8) >> guts. screen_bits. blue_range) |
	(((((c & guts. visual. green_mask) >> guts. screen_bits. green_shift) << 8) >> guts. screen_bits. green_range) << 8) |
	(((((c & guts. visual. red_mask)   >> guts. screen_bits. red_shift)   << 8) >> guts. screen_bits. red_range) << 16);
}

PFont
apc_menu_get_font( Handle self, PFont font)
{
	DEFMM;
	if ( !XX-> font)
		return apc_menu_default_font( font);
	memcpy( font, &XX-> font-> font, sizeof( Font));
	return font;
}

Bool
apc_menu_set_color( Handle self, Color color, int i)
{
	DEFMM;
	if ( i < 0 || i > ciMaxId) return false;
	XX-> rgb[i] = prima_map_color( color, nil);
	if ( !XX-> type.popup) {
		if ( X(PWidget(self)-> owner)-> menuColorImmunity) {
			X(PWidget(self)-> owner)-> menuColorImmunity--;
			return true;
		}
		if ( X_WINDOW) {
			prima_palette_replace( PWidget(self)-> owner, false);
			if ( !XX-> paint_pending) {
				XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
				XX-> paint_pending = true;
			}
		}
	} else {
		XX-> c[i] = prima_allocate_color( nilHandle, XX-> rgb[i], nil);
		if ( XX-> layered )
			XX-> argb_c[i] = argb_color( prima_map_color( XX->rgb[i], nil));
	}
	return true;
}

/* apc_menu_set_font is in apc_font.c */

void
menu_touch( Handle self, PMenuItemReg who, Bool kill)
{
	DEFMM;
	PMenuWindow w, lw = nil;

	if ( guts. currentMenu != self) return;

	w = XX-> w;
	while ( w) {
		if ( w-> m == who) {
			if ( kill || lw == nil)
				prima_end_menu();
			else
				menu_window_delete_downlinks( M(self), lw);
			return;
		}
		lw = w;
		w = w-> next;
	}
}

static void
menu_reconfigure( Handle self)
{
	XEvent ev;
	DEFMM;
	ev. type = ConfigureNotify;
	ev. xconfigure. width  = X(PComponent(self)-> owner)-> size.x;
	ev. xconfigure. height = X(PComponent(self)-> owner)-> size.y;
	XX-> w-> sz. x = ev. xconfigure. width - 1; /* force cache flush */
	prima_handle_menu_event( &ev, PMenu(self)-> handle, self);
}

static void
menubar_repaint( Handle self)
{
	DEFMM;
	if ( !XT_IS_POPUP(XX) && XX-> w == &XX-> wstatic && PMenu(self)-> handle) {
		XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
		XX-> paint_pending = true;
	}
}

Bool
apc_menu_update( Handle self, PMenuItemReg oldBranch, PMenuItemReg newBranch)
{
	DEFMM;
	if ( !XT_IS_POPUP(XX) && XX-> w-> m == oldBranch) {
		if ( guts. currentMenu == self) prima_end_menu();
		XX-> w-> m = newBranch;
		if ( PMenu(self)-> handle) {
			update_menu_window( XX, XX-> w);
			menu_reconfigure( self);
			XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
			XX-> paint_pending = true;
		}
	}
	menu_touch( self, oldBranch, true);
	return true;
}

Bool
apc_menu_item_delete( Handle self, PMenuItemReg m)
{
	DEFMM;
	if ( !XT_IS_POPUP(XX) && XX-> w-> m == m) {
		if ( guts. currentMenu == self) prima_end_menu();
		XX-> w-> m = TREE;
		if ( PMenu(self)-> handle) {
			update_menu_window( XX, XX-> w);
			menu_reconfigure( self);
			XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
			XX-> paint_pending = true;
		}
	}
	menu_touch( self, m, true);
	return true;
}

Bool
apc_menu_item_set_accel( Handle self, PMenuItemReg m)
{
	menu_touch( self, m, false);
	return true;
}

Bool
apc_menu_item_set_check( Handle self, PMenuItemReg m)
{
	menu_touch( self, m, false);
	return true;
}

Bool
apc_menu_item_set_enabled( Handle self, PMenuItemReg m)
{
	menu_touch( self, m, false);
	menubar_repaint( self);
	return true;
}

Bool
apc_menu_item_set_image( Handle self, PMenuItemReg m)
{
	menu_touch( self, m, false);
	menubar_repaint( self);
	return true;
}

Bool
apc_menu_item_set_key( Handle self, PMenuItemReg m)
{
	menu_touch( self, m, false);
	return true;
}

Bool
apc_menu_item_set_text( Handle self, PMenuItemReg m)
{
	menu_touch( self, m, false);
	menubar_repaint( self);
	return true;
}

ApiHandle
apc_menu_get_handle( Handle self)
{
	return nilHandle;
}

Bool
apc_popup_create( Handle self, Handle owner)
{
	DEFMM;
	apc_menu_destroy( self);
	XX-> type.menu = true;
	XX-> type.popup = true;
	return true;
}

PFont
apc_popup_default_font( PFont f)
{
	memcpy( f, &guts. default_menu_font, sizeof( Font));
	return f;
}

Bool
apc_popup( Handle self, int x, int y, Rect *anchor)
{
	DEFMM;
	PMenuItemReg m;
	PMenuWindow w;
	XWindow dummy;
	PDrawableSysData owner;
	int dx, dy;

	prima_end_menu();
	if (!(m=TREE)) return false;
	guts. currentMenu = self;
	if ( !send_cmMenu( self, nil)) return false;
	if (!(w = get_window(self,m))) return false;
	update_menu_window(XX, w);
	if ( anchor-> left == 0 && anchor-> right == 0 && anchor-> top == 0 && anchor-> bottom == 0) {
		anchor-> left = anchor-> right = x;
		anchor-> top = anchor-> bottom = y;
	} else {
		if ( x < anchor-> left)   anchor-> left   = x;
		if ( x > anchor-> right)  anchor-> right  = x;
		if ( y < anchor-> bottom) anchor-> bottom = y;
		if ( y > anchor-> top)    anchor-> top    = y;
	}
	owner = X(PComponent(self)->owner);
	y = owner-> size. y - y;
	anchor-> bottom = owner-> size. y - anchor-> bottom;
	anchor-> top = owner-> size. y - anchor-> top;
	dx = dy = 0;
	XTranslateCoordinates( DISP, owner->udrawable, guts. root, dx, dy, &dx, &dy, &dummy);
	x += dx;
	y += dy;
	anchor-> left   += dx;
	anchor-> right  += dx;
	anchor-> top    += dy;
	anchor-> bottom += dy;
	if ( anchor-> bottom + w-> sz.y <= guts. displaySize.y)
		y = anchor-> bottom;
	else if ( w-> sz. y < anchor-> top)
		y = anchor-> top - w-> sz. y;
	else
		y = 0;
	if ( anchor-> right + w-> sz.x <= guts. displaySize.x)
		x = anchor-> right;
	else if ( w-> sz.x < anchor-> left)
		x = anchor-> left - w-> sz. x;
	else
		x = 0;
	w-> pos. x = x;
	w-> pos. y = y;
	XX-> focused = w;
	XGetInputFocus( DISP, &XX-> focus, &dx);
	XMoveWindow( DISP, w->w, x, y);
	XMapRaised( DISP, w->w);
	XSetInputFocus( DISP, w->w, RevertToNone, CurrentTime);
	XFlush( DISP);
	XCHECKPOINT;
	return true;
}

Bool
apc_window_set_menu( Handle self, Handle menu)
{
	DEFXX;
	int y = XX-> menuHeight;
	Bool repal = false;

	if ( XX-> menuHeight > 0) {
		PMenu m = ( PMenu) PWindow( self)-> menu;
		PMenuWindow w = M(m)-> w;
		if ( m-> handle == guts. currentMenu) prima_end_menu();
		hash_delete( guts. menu_windows, &w-> w, sizeof( w-> w), false);
		XDestroyWindow( DISP, w-> w);
		free_unix_items( w);
		m-> handle = nilHandle;
		M(m)-> paint_pending = false;
		M(m)-> focused = nil;
		y = 0;
		repal = true;
	}

	if ( menu) {
		PMenu m = ( PMenu) menu;
		XSetWindowAttributes attrs;
		unsigned long valuemask;
		attrs. event_mask =           KeyPressMask | ButtonPressMask  | ButtonReleaseMask
			| EnterWindowMask     | LeaveWindowMask | ButtonMotionMask | ExposureMask
			| StructureNotifyMask | FocusChangeMask | OwnerGrabButtonMask
			| PointerMotionMask;
		attrs. do_not_propagate_mask = attrs. event_mask;
		attrs. win_gravity = NorthWestGravity;
		valuemask = CWWinGravity| CWEventMask;
		if ( XF_LAYERED(XX)) {
			valuemask |= CWBackPixel | CWBorderPixel;
			attrs. background_pixel = 0xFFFFFFFF;
			attrs. border_pixel     = 0xFFFFFFFF;
		}
		y = PWindow(self)-> menuFont. height + MENU_ITEM_GAP * 2;
		M(m)-> w-> w = m-> handle = XCreateWindow( DISP, X_WINDOW,
			0, 0, 1, 1, 0, CopyFromParent,
			InputOutput, CopyFromParent, valuemask, &attrs);
		hash_store( guts. menu_windows, &m-> handle, sizeof( m-> handle), m);
		XResizeWindow( DISP, m-> handle, XX-> size.x, y);
		XMapRaised( DISP, m-> handle);
		M(m)-> paint_pending = true;
		M(m)-> focused = nil;
		update_menu_window(M(m), M(m)-> w);
		menu_reconfigure( menu);
		repal = true;
		/* make it immune to necessary color change calls */
		XX-> menuColorImmunity = ciMaxId + 1;
	}
	prima_window_reset_menu( self, y);
	if ( repal) prima_palette_replace( self, false);
	if ( menu) {
		int i;
		Bool layered = XF_LAYERED(XX);
		XX->flags.layered = false; /* we need non-layered colors for popup menus */
		for ( i = 0; i <= ciMaxId; i++) {
			M(menu)-> c[i] = prima_allocate_color( self,
				prima_map_color( PWindow(self)-> menuColor[i], nil), nil);
		}
		XX->flags.layered = layered;
		M(menu)-> layered = XX->flags. layered;
		if ( M(menu)-> layered ) {
			for ( i = 0; i <= ciMaxId; i++)
				M(menu)-> argb_c[i] = argb_color(
					prima_map_color( PWindow(self)-> menuColor[i], nil)
				);
		}
	}
	return true;
}
