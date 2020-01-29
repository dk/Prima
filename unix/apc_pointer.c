#include "unix/guts.h"
#include "Icon.h"
#undef HAVE_X11_XCURSOR_XCURSOR_H

static int
cursor_map[] = {
	/* crArrow           => */   XC_left_ptr,
	/* crText            => */   XC_xterm,
	/* crWait            => */   XC_watch,
	/* crSize            => */   XC_sizing,
	/* crMove            => */   XC_fleur,
	/* crSizeWest        => */   XC_left_side,
	/* crSizeEast        => */   XC_right_side,
	/* crSizeNE          => */   XC_sb_h_double_arrow,
	/* crSizeNorth       => */   XC_top_side,
	/* crSizeSouth       => */   XC_bottom_side,
	/* crSizeNS          => */   XC_sb_v_double_arrow,
	/* crSizeNW          => */   XC_top_left_corner,
	/* crSizeSE          => */   XC_bottom_right_corner,
	/* crSizeNE          => */   XC_top_right_corner,
	/* crSizeSW          => */   XC_bottom_left_corner,
	/* crInvalid         => */   XC_X_cursor,
        /* crDragNone        => */   XC_X_cursor,
        /* crDragCopy        => */   XC_bottom_side,
        /* crDragMove        => */   XC_bottom_side,
        /* crDragLink        => */   XC_bottom_side,
        /* crDragAsk         => */   XC_question_arrow,
};

#ifdef HAVE_X11_XCURSOR_XCURSOR_H
static const char *
xcursor_map[] = {
	/* crArrow           => */   "left_ptr",
	/* crText            => */   "xterm",
	/* crWait            => */   "watch",
	/* crSize            => */   "sizing",
	/* crMove            => */   "fleur",
	/* crSizeWest        => */   "left_side",
	/* crSizeEast        => */   "right_side",
	/* crSizeNE          => */   "sb_h_double_arrow",
	/* crSizeNorth       => */   "top_side",
	/* crSizeSouth       => */   "bottom_side",
	/* crSizeNS          => */   "sb_v_double_arrow",
	/* crSizeNW          => */   "top_left_corner",
	/* crSizeSE          => */   "bottom_right_corner",
	/* crSizeNE          => */   "top_right_corner",
	/* crSizeSW          => */   "bottom_left_corner",
	/* crInvalid         => */   "X_cursor",
        /* crDragNone        => */   "circle",
        /* crDragCopy        => */   "sb_down_arrow",
        /* crDragMove        => */   "sb_down_arrow",
        /* crDragLink        => */   "sb_down_arrow",
        /* crDragAsk         => */   "question_arrow",
};
#endif

Cursor
predefined_cursors[] = {
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None,
	None
};

static Bool
create_cursor(CustomPointer* cp, Handle icon, Point hot_spot)
{
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	XcursorImage* i;
	PIcon c = PIcon(icon);
	Bool kill;
	int x, y;
	XcursorPixel * dst;
	Byte * src_data, * src_mask;

	if ( hot_spot. x < 0) hot_spot. x = 0;
	if ( hot_spot. y < 0) hot_spot. y = 0;
	if ( hot_spot. x >= c-> w) hot_spot. x = c-> w - 1;
	if ( hot_spot. y >= c-> h) hot_spot. y = c-> h - 1;
	cp-> hot_spot = hot_spot;
	if (( i = XcursorImageCreate( c-> w, c-> h )) == NULL) {
		warn( "XcursorImageCreate(%d,%d) error", c->w, c->h);
		return false;
	}
	i-> xhot = hot_spot. x;
	i-> yhot = c-> h - hot_spot. y - 1;

	if ( c-> type != imRGB || c-> maskType != imbpp8 ) {
		icon = CIcon(icon)->dup(icon);
		kill = true;
		CIcon(icon)-> set_type( icon, imRGB );
		CIcon(icon)-> set_maskType( icon, imbpp8 );
	} else
		kill = false;
	c = PIcon(icon);
	src_data = c->data + c->lineSize * ( c-> h - 1 );
	src_mask = c->mask + c->maskLine * ( c-> h - 1 );
	dst = i->pixels;
	for ( y = 0; y < c-> h; y++) {
		Byte * s_data = src_data, * s_mask = src_mask;
		for ( x = 0; x < c-> w; x++) {
			*(dst++) =
				s_data[0]|
				(s_data[1] << 8)|
				(s_data[2] << 16)|
				(*(s_mask++) << 24)
				;
			s_data += 3;
		}
		src_mask -= c->maskLine;
		src_data -= c->lineSize;
	}
	if ( kill ) Object_destroy(icon);

	cp-> cursor = XcursorImageLoadCursor(DISP, i);
	if ( cp-> cursor == None) {
		XcursorImageDestroy(i);
		warn( "error creating cursor");
		return false;
	}
	cp-> xcursor = i;

	return true;

#else

	Handle cursor;
	Bool noSZ  = PIcon(icon)-> w != guts.cursor_width || PIcon(icon)-> h != guts.cursor_height;
	Bool noBPP = (PIcon(icon)-> type & imBPP) != 1;
	XColor xcb, xcw;
	PIcon c;

	if ( noSZ || noBPP) {
		cursor = CIcon(icon)->dup(icon);
		c = PIcon(cursor);
		if ( cursor == nilHandle) {
			warn( "Error duping user cursor");
			return false;
		}
		if ( noSZ) {
			CIcon(cursor)-> stretch( cursor, guts.cursor_width, guts.cursor_height);
			if ( c-> w != guts.cursor_width || c-> h != guts.cursor_height) {
				warn( "Error stretching user cursor");
				Object_destroy( cursor);
				return false;
			}
		}
		if ( noBPP) {
			CIcon(cursor)-> set_type( cursor, imMono);
			if ((c-> type & imBPP) != 1) {
				warn( "Error black-n-whiting user cursor");
				Object_destroy( cursor);
				return false;
			}
		}
	} else
		cursor = icon;
	if ( !prima_create_icon_pixmaps( cursor, &cp->xor, &cp->and)) {
		warn( "Error creating user cursor pixmaps");
		if ( noSZ || noBPP)
			Object_destroy( cursor);
		return false;
	}
	if ( noSZ || noBPP)
		Object_destroy( cursor);
	if ( hot_spot. x < 0) hot_spot. x = 0;
	if ( hot_spot. y < 0) hot_spot. y = 0;
	if ( hot_spot. x >= guts. cursor_width)  hot_spot. x = guts. cursor_width  - 1;
	if ( hot_spot. y >= guts. cursor_height) hot_spot. y = guts. cursor_height - 1;
	cp-> hot_spot = hot_spot;
	xcb. red = xcb. green = xcb. blue = 0;
	xcw. red = xcw. green = xcw. blue = 0xFFFF;
	xcb. pixel = guts. monochromeMap[0];
	xcw. pixel = guts. monochromeMap[1];
	xcb. flags = xcw. flags = DoRed | DoGreen | DoBlue;
	cp-> cursor = XCreatePixmapCursor( DISP, cp-> xor,
		cp-> and, &xcw, &xcb,
		hot_spot. x, guts.cursor_height - hot_spot. y - 1);
	if ( cp-> cursor == None) {
		warn( "error creating cursor from pixmaps");
		return false;
	}
	return true;
#endif
}


static Bool
load_pointer_font( void)
{
	if ( !guts.pointer_font)
		guts.pointer_font = XLoadQueryFont( DISP, "cursor");
	if ( !guts.pointer_font) {
		warn( "Cannot load cursor font");
		return false;
	}
	return true;
}

static CustomPointer*
is_drag_cursor_available(int id)
{
	if ( id >= crDragCopy && id <= crDragLink ) {
		CustomPointer *cp = guts.xdnd_pointers + id - crDragCopy;
		return (cp->status > 0) ? cp : NULL;
	}
	return NULL;
}

static Point
get_predefined_hot_spot( int id)
{
	int idx;
	XFontStruct *fs;
	XCharStruct *cs;
	Point ret = {0,0};

	if ( id < crDefault || id >= crUser)  return ret;

#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	{
		XcursorImage * i;
		if (( i = XcursorLibraryLoadImage( xcursor_map[id] , NULL, guts. cursor_width )) != NULL) {
			ret.x = i->xhot;
			ret.y = i->height - i->yhot - 1;
			XcursorImageDestroy(i);
			return ret;
		}
	}
#endif

	if ( !load_pointer_font())           return ret;

	idx = cursor_map[id];
	fs = guts.pointer_font;
	if ( !fs-> per_char)
		cs = &fs-> min_bounds;
	else if ( idx < fs-> min_char_or_byte2 || idx > fs-> max_char_or_byte2) {
		int default_char = fs-> default_char;
		if ( default_char < fs-> min_char_or_byte2 || default_char > fs-> max_char_or_byte2)
		default_char = fs-> min_char_or_byte2;
		cs = fs-> per_char + default_char - fs-> min_char_or_byte2;
	} else
		cs = fs-> per_char + idx - fs-> min_char_or_byte2;
	ret. x = -cs->lbearing;
	ret. y = guts.cursor_height - cs->ascent;
	if ( ret. x < 0) ret. x = 0;
	if ( ret. y < 0) ret. y = 0;
	if ( ret. x >= guts. cursor_width)  ret. x = guts. cursor_width  - 1;
	if ( ret. y >= guts. cursor_height) ret. y = guts. cursor_height - 1;
	return ret;
}

#ifdef HAVE_X11_XCURSOR_XCURSOR_H
static Bool
xcursor_load( Handle self, int id, Handle icon)
{
	PIcon c;
	int x, y;
	XcursorPixel * src;
	XcursorImage* i;
	Byte * dst_data, * dst_mask;
	Bool kill = false;
	CustomPointer* cp;

	if (( cp = is_drag_cursor_available(id)) != NULL ) {
		i = cp->xcursor;
		goto READ_BITMAP;
	}

	if ( id != crUser ) {
		if (( i = XcursorLibraryLoadImage( xcursor_map[id] , NULL, guts. cursor_width )) == NULL)
			return false;
		kill = true;
	} else if ( self ) {
		DEFXX;
		i = XX-> user_pointer.xcursor;
		kill = false;
	} else
		return false;

READ_BITMAP:
	c = PIcon(icon);
	CIcon(icon)-> create_empty_icon( icon, i->width, i->height, imRGB, imbpp8);
	dst_data = c->data + c->lineSize * ( c-> h - 1 );
	dst_mask = c->mask + c->maskLine * ( c-> h - 1 );
	src = i->pixels;
	for ( y = 0; y < c-> h; y++) {
		Byte * d_data = dst_data, * d_mask = dst_mask;
		for ( x = 0; x < c-> w; x++) {
			*d_data++ = *src & 0xff;
			*d_data++ = (*src >> 8) & 0xff;
			*d_data++ = (*src >> 16) & 0xff;
			*d_mask++ = (*src >> 24) & 0xff;
			src++;
		}
		dst_mask -= c->maskLine;
		dst_data -= c->lineSize;
	}
	if ( kill ) XcursorImageDestroy(i);
	return true;
}
#endif

static Bool
xlib_cursor_load( Handle self, int id, Handle icon)
{
	XImage *im;
	Pixmap p1 = None, p2 = None;
	Bool free_pixmap = true, success = false;
	GC gc;
	XGCValues gcv;
	char c;
	int w = guts.cursor_width, h = guts.cursor_height;
	CustomPointer *cp = NULL;

	cp = is_drag_cursor_available(id);
	if ( cp != NULL ) {
		free_pixmap = false;
	} else if ( id == crUser ) {
		if ( !self ) return false;
		cp = &X(self)->user_pointer;
		free_pixmap = false;
	} else {
		XFontStruct *fs;
		XCharStruct *cs;
		int idx = cursor_map[id];

		if ( !load_pointer_font()) return false;
		fs = guts.pointer_font;
		if ( !fs-> per_char)
			cs = &fs-> min_bounds;
		else if ( idx < fs-> min_char_or_byte2 || idx > fs-> max_char_or_byte2) {
			int default_char = fs-> default_char;
			if ( default_char < fs-> min_char_or_byte2 || default_char > fs-> max_char_or_byte2)
				default_char = fs-> min_char_or_byte2;
			cs = fs-> per_char + default_char - fs-> min_char_or_byte2;
		} else
			cs = fs-> per_char + idx - fs-> min_char_or_byte2;

		p1 = XCreatePixmap( DISP, guts. root, w, h, 1);
		p2 = XCreatePixmap( DISP, guts. root, w, h, 1);
		gcv. background = 1;
		gcv. foreground = 0;
		gcv. font = guts.pointer_font-> fid;
		gc = XCreateGC( DISP, p1, GCBackground | GCForeground | GCFont, &gcv);
		XFillRectangle( DISP, p1, gc, 0, 0, w, h);
		gcv. background = 0;
		gcv. foreground = 1;
		XChangeGC( DISP, gc, GCBackground | GCForeground, &gcv);
		XFillRectangle( DISP, p2, gc, 0, 0, w, h);
		XDrawString( DISP, p1, gc, -cs-> lbearing, cs-> ascent, (c = (char)(idx+1), &c), 1);
		gcv. background = 1;
		gcv. foreground = 0;
		XChangeGC( DISP, gc, GCBackground | GCForeground, &gcv);
		XDrawString( DISP, p2, gc, -cs-> lbearing, cs-> ascent, (c = (char)(idx+1), &c), 1);
		XDrawString( DISP, p1, gc, -cs-> lbearing, cs-> ascent, (c = (char)idx, &c), 1);
		XFreeGC( DISP, gc);
	}
	if ( cp != NULL ) {
		p1 = cp-> xor;
		p2 = cp-> and;
	}
	CIcon(icon)-> create_empty( icon, w, h, imBW);
	if (( im = XGetImage( DISP, p1, 0, 0, w, h, 1, XYPixmap)) == NULL)
		goto FAIL;
	prima_copy_xybitmap(
		PIcon(icon)-> data, (Byte*)im-> data,
		PIcon(icon)-> w, PIcon(icon)-> h,
		PIcon(icon)-> lineSize, im-> bytes_per_line);
	XDestroyImage( im);
	if (( im = XGetImage( DISP, p2, 0, 0, w, h, 1, XYPixmap)) == NULL)
		goto FAIL;
	prima_copy_xybitmap(
		PIcon(icon)-> mask, (Byte*)im-> data,
		PIcon(icon)-> w, PIcon(icon)-> h,
		PIcon(icon)-> maskLine, im-> bytes_per_line);
	if ( cp != NULL ) {
		int i;
		Byte * mask = PIcon(icon)-> mask;
		for ( i = 0; i < PIcon(icon)-> maskSize; i++)
			mask[i] = ~mask[i];
	}
	success = true;
FAIL:
	if (im != NULL) XDestroyImage( im);
	if ( free_pixmap) {
		XFreePixmap( DISP, p1);
		XFreePixmap( DISP, p2);
	}
	return success;
}

static void
draw_poly( Handle self, int n_points, Point * pts, int dx, int dy)
{
	RegionRec rgnrec, *prgnrec;
	PIcon i = (PIcon) self;
	ImgPaintContext ctx = {
		{0,0,0,0},
		{0,0,0,0},
		ropCopyPut, false,
		{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff },
		{0,0},
		lpSolid,
		NULL,
		{dx,dy}
	};
	Image mask;
	img_fill_dummy(&mask, i->w, i->h, i->maskType, i->mask, NULL);

	Handle rgn = (Handle) create_object("Prima::Region", "");
	apc_region_destroy(rgn);
	rgnrec.type = rgnPolygon;
	rgnrec.data.polygon.n_points  = n_points;
	rgnrec.data.polygon.points    = pts;
	rgnrec.data.polygon.fill_mode = fmWinding;
	apc_region_create(rgn, &rgnrec);
	apc_region_offset(rgn, dx, dy);
	prgnrec = apc_region_copy_rects(rgn);
	Object_destroy(rgn);

	ctx.region = &prgnrec->data.box;
	memset( &ctx.color, 0xff, sizeof(ctx.color));
	img_bar( self, 0, 0, i->w, i-> h, &ctx);
	memset( &ctx.color, 0x0, sizeof(ctx.color));
	img_bar((Handle) &mask, 0, 0, i->w, i-> h, &ctx);
	free(prgnrec);
	ctx.region = NULL;

	memset( &ctx.color, 0x0, sizeof(ctx.color));
	img_polyline( self, n_points, pts, &ctx);
	img_polyline((Handle) &mask, n_points, pts, &ctx);
}

static void
xdnd_synthesize_cursor(Handle self, int id)
{
	PIcon i  = (PIcon) self;
	int side = ((i->h < i->w) ? i->h : i->w) / 2;
	int step1 = side / 3; /* worst case 2 */
	int step2 = step1 * 2;
	int step3 = step1 * 3;

	CIcon(self)->set_maskType(self, 1);

	/* draw a little + = or arrow for copy,move,link in the bottom quarter */
	switch ( id ) {
	case crDragCopy: {
		Point plus[13] = {
			{step1, 0},     {step2, 0},
			{step2, step1}, {step3, step1},
			{step3, step2}, {step2, step2},
			{step2, step3}, {step1, step3},
			{step1, step2}, {0, step2},
			{0, step1},     {step1, step1},
			{step1, 0}
		};
		draw_poly( self, 13, plus, i->w/2, 0 );
		break;
	}
	case crDragMove: {
		Point bar[5] = {
			{0, 0}, {step3, 0},
			{step3, step1}, {0, step1},
			{0, 0}
		};
		draw_poly( self, 5, bar, i->w/2, 0 );
		draw_poly( self, 5, bar, i->w/2, step1 * 1.5 + 1);
		break;
	}
	case crDragLink: {
		Point arrow[11] = {
			{step2, 0}, 
			{step3, step1},
			{step2, step2}, {step3, step2},
			{step3, step3}, {0,step3},
			{0,0}, {step1, 0},
			{step1, step1}, {step2, 0},
		};
		draw_poly( self, 10, arrow, i->w/2, 0 );
		break;
	}}
}

static Bool
create_xdnd_pointer(int id, CustomPointer* cp)
{
	Handle self;
	Bool use_xcursor = false;
	if ( cp-> status > 0 ) return true;
	if ( cp-> status < 0 ) return false;

	self = (Handle) create_object("Prima::Icon", "");

#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	if (xcursor_load(nilHandle, crArrow, self))
		use_xcursor = true;
#endif
	if ( !use_xcursor )
		xlib_cursor_load(nilHandle, crArrow, self);

	if ( PImage(self)->w < 16 || PImage(self)->h < 16 ) {
		/* too small */
		cp-> status = -1;
		Object_destroy(self);
		return false;
	}

	xdnd_synthesize_cursor(self, id);

	cp-> status = 1;
	create_cursor(cp, self, get_predefined_hot_spot(crArrow));
	Object_destroy(self);

	return true;
}

static int
get_cursor( Handle self, Pixmap *source, Pixmap *mask, Point *hot_spot, Cursor *cursor)
{
	int id = X(self)-> pointer_id;

	while ( self && ( id = X(self)-> pointer_id) == crDefault)
		self = PWidget(self)-> owner;
	switch ( id ) {
	case crDefault:
		id = crArrow;
		break;
	case crUser:
		if (source)       *source   = X(self)-> user_pointer.xor;
		if (mask)         *mask     = X(self)-> user_pointer.and;
		if (hot_spot)     *hot_spot = X(self)-> user_pointer.hot_spot;
		if (cursor)       *cursor   = X(self)-> user_pointer.cursor;
		break;
	case crDragCopy:
	case crDragMove:
	case crDragLink: {
		CustomPointer *cp = guts.xdnd_pointers + id - crDragCopy;
		if ( create_xdnd_pointer(id, cp)) {
			if (source)       *source   = cp->xor;
			if (mask)         *mask     = cp->and;
			if (hot_spot)     *hot_spot = cp->hot_spot;
			if (cursor)       *cursor   = cp->cursor;
		}
		break;
	}}

	return id;
}

Cursor
prima_get_cursor(Handle self)
{
	DEFXX;
	CustomPointer* cp;
	if ( XX-> flags. pointer_obscured )
		return prima_null_pointer();
	else if ( XX->pointer_id == crUser )
		return XX-> user_pointer.cursor;
	else if (( cp = is_drag_cursor_available( XX-> pointer_id )) != NULL )
		return cp->cursor;
	else
		return XX->actual_pointer;
}

Point
apc_pointer_get_hot_spot( Handle self)
{
	Point hot_spot;
	int id = get_cursor(self, nil, nil, &hot_spot, nil);
	return (id == crUser || is_drag_cursor_available(id) != NULL)
		? hot_spot : get_predefined_hot_spot(id);
}

Point
apc_pointer_get_pos( Handle self)
{
	Point p;
	XWindow root, child;
	int x, y;
	unsigned int mask;

	if ( !XQueryPointer( DISP, guts. root,
			&root, &child, &p. x, &p. y,
			&x, &y, &mask))
		return guts. displaySize;
	p. y = guts. displaySize. y - p. y - 1;
	return p;
}

int
apc_pointer_get_shape( Handle self)
{
	return X(self)->pointer_id;
}

Point
apc_pointer_get_size( Handle self)
{
	Point p;
	p.x = guts.cursor_width;
	p.y = guts.cursor_height;
	return p;
}

Bool
apc_pointer_get_bitmap( Handle self, Handle icon)
{
	int id = get_cursor( self, nil, nil, nil, nil);
	if ( id < crDefault || id > crUser)  return false;

#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	if (xcursor_load(self, id, icon))
		return true;
#endif
	return xlib_cursor_load(self, id, icon);
}

Bool
apc_pointer_get_visible( Handle self)
{
	return guts. pointer_invisible_count == 0;
}

Bool
apc_pointer_set_pos( Handle self, int x, int y)
{
	XEvent ev;
	if ( !XWarpPointer( DISP, None, guts. root,
		0, 0, guts. displaySize.x, guts. displaySize.y, x, guts. displaySize.y - y - 1))
		return false;
	XCHECKPOINT;
	XSync( DISP, false);
	while ( XCheckMaskEvent( DISP, PointerMotionMask|EnterWindowMask|LeaveWindowMask, &ev))
		prima_handle_event( &ev, nil);
	return true;
}

Bool
apc_pointer_set_shape( Handle self, int id)
{
	DEFXX;
	Cursor uc = None;

	if ( id < crDefault || id > crUser)  return false;
	XX-> pointer_id = id;
	id = get_cursor( self, nil, nil, nil, &uc);
	if ( id == crUser || is_drag_cursor_available(id)) {
		if ( uc != None ) {
			if ( self != application) {
				if ( guts. pointer_invisible_count < 0) {
					if ( !XX-> flags. pointer_obscured) {
						XDefineCursor( DISP, XX-> udrawable, prima_null_pointer());
						XX-> flags. pointer_obscured = 1;
					}
				} else {
					XDefineCursor( DISP, XX-> udrawable, uc);
					XX-> flags. pointer_obscured = 0;
				}
				XCHECKPOINT;
			}
		} else
			id = crArrow;
	} else {
		if ( predefined_cursors[id] == None) {
			predefined_cursors[id] =
				XCreateFontCursor( DISP, cursor_map[id]);
			XCHECKPOINT;
		}
		XX-> actual_pointer = predefined_cursors[id];
		if ( self != application) {
			if ( guts. pointer_invisible_count < 0) {
				if ( !XX-> flags. pointer_obscured) {
					XDefineCursor( DISP, XX-> udrawable, prima_null_pointer());
					XX-> flags. pointer_obscured = 1;
				}
			} else {
				XDefineCursor( DISP, XX-> udrawable, predefined_cursors[id]);
				XX-> flags. pointer_obscured = 0;
			}
			XCHECKPOINT;
		}
	}
	XFlush( DISP);
	if ( guts. grab_widget)
		apc_widget_set_capture( guts. grab_widget, true, guts. grab_confine);
	return true;
}

Bool
apc_pointer_set_user( Handle self, Handle icon, Point hot_spot)
{
	DEFXX;

	if ( XX-> user_pointer.cursor != None) {
		XFreeCursor( DISP, XX-> user_pointer.cursor);
		XX-> user_pointer.cursor = None;
	}
	if ( XX-> user_pointer.xor != None) {
		XFreePixmap( DISP, XX-> user_pointer.xor);
		XX-> user_pointer.xor = None;
	}
	if ( XX-> user_pointer.and != None) {
		XFreePixmap( DISP, XX-> user_pointer.and);
		XX-> user_pointer.and = None;
	}
#ifdef HAVE_X11_XCURSOR_XCURSOR_H
	if ( XX-> user_pointer.xcursor != NULL) {
		XcursorImageDestroy(XX-> user_pointer.xcursor);
		XX-> user_pointer.xcursor = NULL;
	}
#endif
	if ( icon != nilHandle) {
		Bool ok;
		ok = create_cursor(&XX->user_pointer, icon, hot_spot);
		if ( !ok ) return false;

		if ( XX-> pointer_id == crUser && self != application) {
			if ( guts. pointer_invisible_count < 0) {
				if ( !XX-> flags. pointer_obscured) {
					XDefineCursor( DISP, XX-> udrawable, prima_null_pointer());
					XX-> flags. pointer_obscured = 1;
				}
			} else {
				XDefineCursor( DISP, XX-> udrawable, XX-> user_pointer.cursor);
				XX-> flags. pointer_obscured = 0;
			}
			XCHECKPOINT;
		}
	}
	XFlush( DISP);
	if ( guts. grab_widget)
		apc_widget_set_capture( guts. grab_widget, true, guts. grab_confine);
	return true;
}

Bool
apc_pointer_set_visible( Handle self, Bool visible)
{
	/* maintaining hide/show count */
	if ( visible) {
		if ( guts. pointer_invisible_count == 0)
			return true;
		if ( ++guts. pointer_invisible_count < 0)
			return true;
	} else {
		if ( guts. pointer_invisible_count-- < 0)
			return true;
	}

	/* setting pointer for widget under cursor */
	{
		Point p    = apc_pointer_get_pos( application);
		Handle wij = apc_application_get_widget_from_point( application, p);
		if ( wij) {
			X(wij)-> flags. pointer_obscured = (visible ? 0 : 1);
			XDefineCursor( DISP, X(wij)-> udrawable, prima_get_cursor(wij));
		}
	}
	XFlush( DISP);
	if ( guts. grab_widget)
		apc_widget_set_capture( guts. grab_widget, true, guts. grab_confine);
	return true;
}

Cursor
prima_null_pointer( void)
{
	if ( guts. null_pointer == nilHandle) {
		Handle nullc = ( Handle) create_object( "Prima::Icon", "", nil);
		PIcon  n = ( PIcon) nullc;
		Pixmap xor, and;
		XColor xc;
		if ( nullc == nilHandle) {
			warn("Error creating icon object");
			return nilHandle;
		}
		n-> self-> create_empty( nullc, 16, 16, imBW);
		memset( n-> mask, 0xFF, n-> maskSize);
		if ( !prima_create_icon_pixmaps( nullc, &xor, &and)) {
			warn( "Error creating null cursor pixmaps");
			Object_destroy( nullc);
			return nilHandle;
		}
		Object_destroy( nullc);
		xc. red = xc. green = xc. blue = 0;
		xc. pixel = guts. monochromeMap[0];
		xc. flags = DoRed | DoGreen | DoBlue;
		guts. null_pointer = XCreatePixmapCursor( DISP, xor, and, &xc, &xc, 0, 0);
		XCHECKPOINT;
		XFreePixmap( DISP, xor);
		XFreePixmap( DISP, and);
		if ( !guts. null_pointer) {
			warn( "Error creating null cursor from pixmaps");
			return nilHandle;
		}
	}
	return guts. null_pointer;
}
