/************************/
/*                      */
/*    Cursor support    */
/*                      */
/************************/


#include <apricot.h>
#include "unix/guts.h"
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)

void
prima_no_cursor( Handle self)
{
	if ( self && guts.focused == self && X(self)
		&& !(XF_IN_PAINT(X(self)))
		&& X(self)-> flags. cursor_visible
		&& guts. cursor_save
		&& guts. cursor_shown)
	{
		DEFXX;
		int x, y, w, h;

		h = XX-> cursor_size. y;
		y = XX-> size. y - (h + XX-> cursor_pos. y);
		x = XX-> cursor_pos. x;
		w = XX-> cursor_size. x;

		prima_get_gc( XX);
		XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &guts. cursor_gcv);
		XCHECKPOINT;
		XCopyArea( DISP, guts. cursor_save, XX-> udrawable, XX-> gc,
					0, 0, w, h, x, y);
		XFlush(DISP);
		XCHECKPOINT;
		prima_release_gc( XX);
		guts. cursor_shown = false;
	}
}

void
prima_update_cursor( Handle self)
{
	if (
		guts.focused == self
		&& !(XF_IN_PAINT(X(self)))
	) {
		DEFXX;
		int x, y, w, h;

		h = XX-> cursor_size. y;
		y = XX-> size. y - (h + XX-> cursor_pos. y);
		x = XX-> cursor_pos. x;
		w = XX-> cursor_size. x;

		if ( !guts. cursor_save || !guts. cursor_xor
			|| w > guts. cursor_pixmap_size. x
			|| h > guts. cursor_pixmap_size. y ||
			XX-> flags. layered != guts. cursor_layered
			)
		{
			if ( !guts. cursor_save) {
				guts. cursor_gcv. background = 0;
				guts. cursor_gcv. foreground = 0xffffffff;
			}
			if ( guts. cursor_save) {
				XFreePixmap( DISP, guts. cursor_save);
				guts. cursor_save = 0;
			}
			if ( guts. cursor_xor) {
				XFreePixmap( DISP, guts. cursor_xor);
				guts. cursor_xor = 0;
			}
			if ( guts. cursor_pixmap_size. x < w)
				guts. cursor_pixmap_size. x = w;
			if ( guts. cursor_pixmap_size. y < h)
				guts. cursor_pixmap_size. y = h;
			if ( guts. cursor_pixmap_size. x < 16)
				guts. cursor_pixmap_size. x = 16;
			if ( guts. cursor_pixmap_size. y < 64)
				guts. cursor_pixmap_size. y = 64;
			guts. cursor_save = XCreatePixmap( DISP, XX-> udrawable,
				guts. cursor_pixmap_size. x,
				guts. cursor_pixmap_size. y,
				XX-> visual-> depth);
			guts. cursor_xor  = XCreatePixmap( DISP, XX-> udrawable,
				guts. cursor_pixmap_size. x,
				guts. cursor_pixmap_size. y,
				XX-> visual-> depth);
			guts. cursor_layered = XX-> flags. layered;
		}

		prima_get_gc( XX);
		XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &guts. cursor_gcv);
		XCopyArea( DISP, XX-> udrawable, guts. cursor_save, XX-> gc,
			x, y, w, h, 0, 0);
		XCopyArea( DISP, guts. cursor_save, guts. cursor_xor, XX-> gc,
			0, 0, w, h, 0, 0);
		XSetFunction( DISP, XX-> gc, GXxor);
		XFillRectangle( DISP, guts. cursor_xor, XX-> gc, 0, 0, w, h);
		prima_release_gc( XX);
		XCHECKPOINT;

		if ( XX-> flags. cursor_visible) {
			guts. cursor_shown = false;
			prima_cursor_tick();
		} else {
			apc_timer_stop( CURSOR_TIMER);
		}
	}
}

void
prima_cursor_tick( void)
{
	if (
		guts. focused &&
		X(guts. focused)-> flags. cursor_visible &&
		!(XF_IN_PAINT(X(guts. focused)))
	) {
		PDrawableSysData selfxx = X(guts. focused);
		Pixmap pixmap;
		int x, y, w, h;

		if ( guts. cursor_shown) {
			if ( guts.invisible_timeout == 0 ) return;
			guts. cursor_shown = false;
			apc_timer_set_timeout( CURSOR_TIMER, guts. invisible_timeout);
			pixmap = guts. cursor_save;
		} else {
			guts. cursor_shown = true;
			apc_timer_set_timeout( CURSOR_TIMER, guts. visible_timeout);
			pixmap = guts. cursor_xor;
		}

		h = XX-> cursor_size. y;
		y = XX-> size. y - (h + XX-> cursor_pos. y);
		x = XX-> cursor_pos. x;
		w = XX-> cursor_size. x;

		prima_get_gc( XX);
		XChangeGC( DISP, XX-> gc, VIRGIN_GC_MASK, &guts. cursor_gcv);
		XCHECKPOINT;
		XCopyArea( DISP, pixmap, XX-> udrawable, XX-> gc, 0, 0, w, h, x, y);
		XCHECKPOINT;
		prima_release_gc( XX);
		XFlush( DISP);
		XCHECKPOINT;
	} else {
		apc_timer_stop( CURSOR_TIMER);
		guts. cursor_shown = !guts. cursor_shown;
	}
}

Bool
apc_cursor_set_pos( Handle self, int x, int y)
{
	DEFXX;
	prima_no_cursor( self);
	RANGE2(x,y);
	XX-> cursor_pos. x = x;
	XX-> cursor_pos. y = y;
	prima_update_cursor( self);
	if ( guts.use_xim ) prima_xim_update_cursor(self);
	return true;
}

Bool
apc_cursor_set_size( Handle self, int x, int y)
{
	DEFXX;
	prima_no_cursor( self);
	if ( x < 0) x = 1;
	if ( y < 0) y = 1;
	if ( x > 16383) x = 16383;
	if ( y > 16383) y = 16383;
	XX-> cursor_size. x = x;
	XX-> cursor_size. y = y;
	prima_update_cursor( self);
	if ( guts.use_xim ) prima_xim_update_cursor(self);
	return true;
}

Bool
apc_cursor_set_visible( Handle self, Bool visible)
{
	DEFXX;
	if ( XX-> flags. cursor_visible != visible) {
		prima_no_cursor( self);
		XX-> flags. cursor_visible = visible;
		prima_update_cursor( self);
	}
	return true;
}

Point
apc_cursor_get_pos( Handle self)
{
	return X(self)-> cursor_pos;
}

Point
apc_cursor_get_size( Handle self)
{
	return X(self)-> cursor_size;
}

Bool
apc_cursor_get_visible( Handle self)
{
	return X(self)-> flags. cursor_visible;
}

int
apc_pointer_get_state( Handle self)
{
	XWindow foo;
	int bar;
	unsigned mask;
	XQueryPointer( DISP, guts.root,  &foo, &foo, &bar, &bar, &bar, &bar, &mask);
	return
		(( mask & Button1Mask) ? mb1 : 0) |
		(( mask & Button2Mask) ? mb2 : 0) |
		(( mask & Button3Mask) ? mb3 : 0) |
		(( mask & Button4Mask) ? mb4 : 0) |
		(( mask & Button5Mask) ? mb5 : 0) |
		(( mask & Button6Mask) ? mb6 : 0) |
		(( mask & Button7Mask) ? mb7 : 0);
}

