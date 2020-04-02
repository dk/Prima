/***********************************************************/
/*                                                         */
/*  System dependent graphics (unix, x11)                  */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Image.h"

#define SORT(a,b)	{ int swp; if ((a) > (b)) { swp=(a); (a)=(b); (b)=swp; }}
#define REVERT(a)	(XX-> size. y - (a) - 1)
#define SHIFT(a,b)	{ (a) += XX-> gtransform. x + XX-> btransform. x; \
			(b) += XX-> gtransform. y + XX-> btransform. y; }
#define RANGE(a)        { if ((a) < -16383) (a) = -16383; else if ((a) > 16383) a = 16383; }
#define RANGE2(a,b)     RANGE(a) RANGE(b)

static Point
gp_get_text_overhangs( Handle self, const char *text, int len, int flags)
{
	DEFXX;
	Point ret;
	if ( len > 0) {
		XCharStruct * cs;
		cs = prima_char_struct( XX-> font-> fs, (void*) text, flags);
		ret. x = ( cs-> lbearing < 0) ? - cs-> lbearing : 0;
		text += (len - 1) * ((flags & (toUTF8 | toGlyphs)) ? 2 : 1);
		cs = prima_char_struct( XX-> font-> fs, (void*) text, flags);
		ret. y = (( cs-> width - cs-> rbearing) < 0) ? cs-> rbearing - cs-> width : 0;
	} else
		ret. x = ret. y = 0;
	return ret;
}

static int
gp_get_text_width( Handle self, const char *text, int len, int flags);

static Point *
gp_get_text_box( Handle self, const char * text, int len, int flags);

static Bool
gp_text_out_rotated( Handle self, const char * text, int x, int y, int len, int flags, Bool * ok_to_not_rotate)
{
	DEFXX;
	int i;
	PRotatedFont r;
	XCharStruct *cs;
	int px, py = ( XX-> flags. paint_base_line) ?  XX-> font-> font. descent : 0;
	int ax = 0, ay = 0;
	int psx, psy, dsx, dsy;
	Fixed rx, ry;
	Bool wide = flags & toUnicode;

	if ( !prima_update_rotated_fonts( XX-> font,
		text, len, wide,
		PDrawable( self)-> font. direction, &r, ok_to_not_rotate
	))
		return false;

	for ( i = 0; i < len; i++) {
		XChar2b index;

		/* acquire actual character index */
		index. byte1 = wide ? (( XChar2b*) text + i)-> byte1 : 0;
		index. byte2 = wide ? (( XChar2b*) text + i)-> byte2 : *((unsigned char*)text + i);

		if ( index. byte1 < r-> first1 || index. byte1 >= r-> first1 + r-> height ||
			index. byte2 < r-> first2 || index. byte2 >= r-> first2 + r-> width) {
			if ( r-> defaultChar1 < 0 || r-> defaultChar2 < 0) continue;
			index. byte1 = ( unsigned char) r-> defaultChar1;
			index. byte2 = ( unsigned char) r-> defaultChar2;
		}

		/* querying character */
		if ( r-> map[(index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2] == nil) continue;
		cs = XX-> font-> fs-> per_char ?
			XX-> font-> fs-> per_char +
				( index. byte1 - XX-> font-> fs-> min_byte1) * r-> width +
				index. byte2 - XX-> font-> fs-> min_char_or_byte2 :
			&(XX-> font-> fs-> min_bounds);

		/* find reference point in pixmap */
		px = ( cs-> lbearing < 0) ? -cs-> lbearing : 0;
		rx. l = px * r-> cos2. l - py * r-> sin2. l + UINT16_PRECISION/2;
		ry. l = px * r-> sin2. l + py * r-> cos2. l + UINT16_PRECISION/2;
		psx = rx. i. i - r-> shift. x;
		psy = ry. i. i - r-> shift. y;

		/* find glyph position */
		rx. l = ax * r-> cos2. l - ay * r-> sin2. l + UINT16_PRECISION/2;
		ry. l = ax * r-> sin2. l + ay * r-> cos2. l + UINT16_PRECISION/2;
		dsx = x + rx. i. i - psx;
		dsy = REVERT( y + ry. i. i) + psy - r-> dimension. y + 1;

		if ( guts. debug & DEBUG_FONTS) {
			_debug("shift %d %d\n", r-> shift.x, r-> shift.y);
			_debug("point ref: %d %d => %d %d. dims: %d %d, [%d %d %d]\n", px, py, psx, psy, r-> dimension.x, r-> dimension.y,
				cs-> lbearing, cs-> rbearing - cs-> lbearing, cs-> width - cs-> rbearing);
			_debug("plot ref: %d %d => %d %d\n", ax, ay, rx.i.i, ry.i.i);
			_debug("at: %d %d ( sz = %d), dest: %d %d\n", x, y, XX-> size.y, dsx, dsy);
		}

/*   GXandReverse   ropNotDestAnd */		/* dest = (!dest) & src */
/*   GXorReverse    ropNotDestOr */		/* dest = (!dest) | src */
/*   GXequiv        ropNotSrcXor */		/* dest ^= !src */
/*   GXandInverted  ropNotSrcAnd */		/* dest &= !src */
/*   GXorInverted   ropNotSrcOr */		/* dest |= !src */
/*   GXnand         ropNotAnd */		/* dest = !(src & dest) */
/*   GXnor          ropNotOr */		/* dest = !(src | dest) */
/*   GXinvert       ropInvert */		/* dest = !dest */

		switch ( XX-> paint_rop) { /* XXX Limited set edition - either expand to full list or find new way to display bitmaps */
		case ropXorPut:
			XSetBackground( DISP, XX-> gc, 0);
			XSetFunction( DISP, XX-> gc, GXxor);
			break;
		case ropOrPut:
			XSetBackground( DISP, XX-> gc, 0);
			XSetFunction( DISP, XX-> gc, GXor);
			break;
		case ropAndPut:
			XSetBackground( DISP, XX-> gc, 0xffffffff);
			XSetFunction( DISP, XX-> gc, GXand);
			break;
		case ropNotPut:
		case ropBlackness:
			XSetForeground( DISP, XX-> gc, 0);
			XSetBackground( DISP, XX-> gc, 0xffffffff);
			XSetFunction( DISP, XX-> gc, GXand);
			break;
		case ropWhiteness:
			XSetForeground( DISP, XX-> gc, 0xffffffff);
			XSetBackground( DISP, XX-> gc, 0);
			XSetFunction( DISP, XX-> gc, GXor);
			break;
		default:
			XSetForeground( DISP, XX-> gc, 0);
			XSetBackground( DISP, XX-> gc, 0xffffffff);
			XSetFunction( DISP, XX-> gc, GXand);
		}
		XPutImage( DISP, XX-> gdrawable, XX-> gc,
			r-> map[(index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2]-> image,
			0, 0, dsx, dsy, r-> dimension.x, r-> dimension.y);
		XCHECKPOINT;
		switch ( XX-> paint_rop) {
		case ropAndPut:
		case ropOrPut:
		case ropXorPut:
		case ropBlackness:
		case ropWhiteness:
			break;
		case ropNotPut:
			XSetForeground( DISP, XX-> gc, XX-> fore. primary);
			XSetBackground( DISP, XX-> gc, 0xffffffff);
			XSetFunction( DISP, XX-> gc, GXorInverted);
			goto DISPLAY;
		default:
			XSetForeground( DISP, XX-> gc, XX-> fore. primary);
			XSetBackground( DISP, XX-> gc, 0);
			XSetFunction( DISP, XX-> gc, GXor);
		DISPLAY:
			XPutImage( DISP, XX-> gdrawable, XX-> gc,
				r-> map[(index. byte1 - r-> first1) * r-> width + index. byte2 - r-> first2]-> image,
				0, 0, dsx, dsy, r-> dimension.x, r-> dimension.y);
			XCHECKPOINT;
		}
		ax += cs-> width;
	}
	apc_gp_set_rop( self, XX-> paint_rop);
	XSetFillStyle( DISP, XX-> gc, FillSolid);
	XSetForeground( DISP, XX-> gc, XX-> fore. primary);
	XSetBackground( DISP, XX-> gc, XX-> back. primary);
	XX-> flags. brush_fore = 1;
	XX-> flags. brush_back = 1;

	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut)) {
		int lw = apc_gp_get_line_width( self);
		int tw = gp_get_text_width( self, text, len, flags | toAddOverhangs) - 1;
		int d  = XX-> font-> underlinePos;
		Point ovx = gp_get_text_overhangs( self, text, len, flags);
		int x1, y1, x2, y2;
		if ( lw != XX-> font-> underlineThickness)
			apc_gp_set_line_width( self, XX-> font-> underlineThickness);

		if ( PDrawable( self)-> font. style & fsUnderlined) {
			ay = d + ( XX-> flags. paint_base_line ? 0 : XX-> font-> font. descent);
			rx. l = -ovx.x * r-> cos2. l - ay * r-> sin2. l + 0.5;
			ry. l = -ovx.x * r-> sin2. l + ay * r-> cos2. l + 0.5;
			x1 = x + rx. i. i;
			y1 = y + ry. i. i;
			tw += ovx.y;
			rx. l = tw * r-> cos2. l - ay * r-> sin2. l + 0.5;
			ry. l = tw * r-> sin2. l + ay * r-> cos2. l + 0.5;
			x2 = x + rx. i. i;
			y2 = y + ry. i. i;
			XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
		}

		if ( PDrawable( self)-> font. style & fsStruckOut) {
			ay = (PDrawable( self)-> font.ascent - PDrawable( self)-> font.internalLeading)/2 +
				+ ( XX-> flags. paint_base_line ? 0 : XX-> font-> font. descent);
			rx. l = -ovx.x * r-> cos2. l - ay * r-> sin2. l + 0.5;
			ry. l = -ovx.x * r-> sin2. l + ay * r-> cos2. l + 0.5;
			x1 = x + rx. i. i;
			y1 = y + ry. i. i;
			tw += ovx.y;
			rx. l = tw * r-> cos2. l - ay * r-> sin2. l + 0.5;
			ry. l = tw * r-> sin2. l + ay * r-> cos2. l + 0.5;
			x2 = x + rx. i. i;
			y2 = y + ry. i. i;
			XDrawLine( DISP, XX-> gdrawable, XX-> gc, x1, REVERT( y1), x2, REVERT( y2));
		}

		if ( lw != XX-> font-> underlineThickness)
			apc_gp_set_line_width( self, lw);
	}
	XFLUSH;
	return true;
}

static void
paint_text_background( Handle self, const char * text, int x, int y, int len, int flags)
{	
	DEFXX;
	int i;
	Point * p;
	FillPattern fp;

	memcpy( &fp, apc_gp_get_fill_pattern( self), sizeof( FillPattern));
	p = gp_get_text_box( self, text, len, flags);
	XSetForeground( DISP, XX-> gc, XX-> back. primary);
	XX-> flags. brush_back = 0;
	XX-> flags. brush_fore = 1;
	XX-> fore. balance = 0;
	XSetFunction( DISP, XX-> gc, GXcopy);
	apc_gp_set_fill_pattern( self, fillPatterns[fpSolid]);
	for ( i = 0; i < 4; i++) {
		p[i].x += x;
		p[i].y += y;
	}
	i = p[2].x; p[2].x = p[3].x; p[3].x = i;
	i = p[2].y; p[2].y = p[3].y; p[3].y = i;

	apc_gp_fill_poly( self, 4, p);
	apc_gp_set_rop( self, XX-> paint_rop);
	apc_gp_set_color( self, XX-> fore. color);
	apc_gp_set_fill_pattern( self, fp);
	free( p);
}

static void
draw_text_underline(Handle self, const char * text, int x, int y, int len, int flags)
{
	DEFXX;
	int lw = apc_gp_get_line_width( self);
	int tw = gp_get_text_width( self, text, len, flags | toAddOverhangs);
	int d  = XX-> font-> underlinePos;
	Point ovx = gp_get_text_overhangs( self, text, len, flags);
	if ( lw != XX-> font-> underlineThickness)
		apc_gp_set_line_width( self, XX-> font-> underlineThickness);
	if ( PDrawable( self)-> font. style & fsUnderlined)
		XDrawLine( DISP, XX-> gdrawable, XX-> gc,
			x - ovx.x, REVERT( y + d), x + tw - 1 + ovx.y, REVERT( y + d));
	if ( PDrawable( self)-> font. style & fsStruckOut) {
		int scy = REVERT( y + (XX-> font-> font.ascent - XX-> font-> font.internalLeading)/2);
		XDrawLine( DISP, XX-> gdrawable, XX-> gc,
			x - ovx.x, scy, x + tw - 1 + ovx.y, scy);
	}
	if ( lw != XX-> font-> underlineThickness)
		apc_gp_set_line_width( self, lw);
}

static Bool
text_out( Handle self, const char * text, int x, int y, int len, int flags)
{
	DEFXX;
	if ( !XX-> flags. paint_base_line)
		y += XX-> font-> font. descent;

	XSetFillStyle( DISP, XX-> gc, FillSolid);
	if ( !XX-> flags. brush_fore) {
		XSetForeground( DISP, XX-> gc, XX-> fore. primary);
		XX-> flags. brush_fore = 1;
	}

	if ( flags & toUTF8)
		XDrawString16( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y) + 1, (XChar2b*) text, len);
	else
		XDrawString( DISP, XX-> gdrawable, XX-> gc, x, REVERT( y) + 1, ( char*) text, len);
	XCHECKPOINT;
	return true;
}

Bool
apc_gp_text_out( Handle self, const char * text, int x, int y, int len, int flags)
{
	Bool ret;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( len == 0) return true;
	if ( len > 65535 ) len = 65535;
	flags &= ~toGlyphs;

#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_text_out( self, text, x, y, len, flags);
#endif

	if ( flags & toUTF8 )
		if ( !( text = ( char *) prima_alloc_utf8_to_wchar( text, len))) return false;

	if ( XX-> flags. paint_opaque)
		paint_text_background( self, text, x, y, len, flags);

	SHIFT(x, y);
	RANGE2(x,y);
	if ( PDrawable( self)-> font. direction != 0) {
		Bool ok_to_not_rotate = false;
		Bool ret;
		ret = gp_text_out_rotated( self, text, x, y, len, flags, &ok_to_not_rotate);
		if ( !ok_to_not_rotate) {
			if ( flags & toUTF8) free(( char *) text);
			return ret;
		}
	}

	ret = text_out(self, text, x, y, len, flags);
	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut))
		draw_text_underline(self, text, x, y, len, flags);

	if ( flags & toUTF8) free(( char *) text);
	XFLUSH;

	return ret;
}

static int need_swap_bytes = -1;
static void
swap_bytes( register uint16_t * area, int len ) 
{
	if ( need_swap_bytes < 0 ) {
		XChar2b  b = { 0x1, 0x23 };
		uint16_t a = 0x123, *pb = (uint16_t*) &b;
		need_swap_bytes = a != *pb;
	}
	if ( need_swap_bytes ) while (len-- > 0) {	
		register uint16_t x = *area;
		*(area++) = (( x & 0xff ) << 8) | (x >> 8);
	}
}
#define SWAP_BYTES(area,len) if (need_swap_bytes) swap_bytes(area,len)

Bool
apc_gp_glyphs_out( Handle self, PGlyphsOutRec t, int x, int y)
{
	Bool ret;
	DEFXX;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( t->len == 0) return true;
	if ( t->len > 65535 ) t->len = 65535;

#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_glyphs_out( self, t, x, y);
#endif

	SWAP_BYTES(t->glyphs,t->len);
	if ( XX-> flags. paint_opaque)
		paint_text_background( self, (const char*) t->glyphs, x, y, t->len, toUnicode);

	SHIFT(x, y);
	RANGE2(x,y);
	if ( PDrawable( self)-> font. direction != 0) {
		Bool ok_to_not_rotate = false;
		ret = gp_text_out_rotated( self, (const char*) t->glyphs,
			x, y, t->len, toUnicode, &ok_to_not_rotate);
		if ( !ok_to_not_rotate)
			goto EXIT;
	}

	ret = text_out(self, (const char*) t->glyphs, x, y, t->len, toUnicode);

	if ( PDrawable( self)-> font. style & (fsUnderlined|fsStruckOut))
		draw_text_underline(self, (const char*) t->glyphs, x, y, t->len, toUnicode);
	XFLUSH;

EXIT:
	SWAP_BYTES(t->glyphs,t->len);
	return ret;
}

Bool
text_shaper_core_text( Handle self, PTextShapeRec r)
{
        int i;
        for ( i = 0; i < r->len; i++) {
                uint32_t c = r->text[i];
                if ( c > 0xffff ) c = 0x0;
                r->glyphs[i] = c;
        }
        r-> n_glyphs = r->len;
	if ( r-> advances ) {
		uint16_t * advances = r-> advances;
		XCharStruct *cs;
		XFontStruct *fs = X(self)-> font-> fs;
		int default_char1 = fs-> default_char >> 8;
		int default_char2 = fs-> default_char & 0xff;
		int i, d = fs-> max_char_or_byte2 - fs-> min_char_or_byte2 + 1;
		int a = 0, c = 0;
		if ( default_char2 < fs-> min_char_or_byte2 || default_char2 > fs-> max_char_or_byte2 ||
			default_char1 < fs-> min_byte1 || default_char1 > fs-> max_byte1) {
			default_char1 = fs-> min_byte1;
			default_char2 = fs-> min_char_or_byte2;
		}
		for ( i = 0; i < r-> len; i++, advances++) {
			int i1 = i >> 8;
			int i2 = i & 0xff;
			if ( !fs-> per_char)
				cs = &fs-> min_bounds;
			else if ( i2 < fs-> min_char_or_byte2 || i2 > fs-> max_char_or_byte2 ||
						i1 < fs-> min_byte1 || i1 > fs-> max_byte1)
				cs = fs-> per_char +
					(default_char1 - fs-> min_byte1) * d +
						default_char2 - fs-> min_char_or_byte2;
			else
				cs = fs-> per_char +
					(i1 - fs-> min_byte1) * d +
						i2 - fs-> min_char_or_byte2;
			if ( i == 0 ) a = cs-> lbearing;
			if ( i == r-> len - 1 ) c = cs-> width - cs-> rbearing;
			*advances = cs->width;
		}
		*(advances++) = (a < 0) ? -a : 0;
		*(advances++) = (c < 0) ? -c : 0;
		bzero( r-> positions, r-> n_glyphs * 2 * sizeof(uint16_t));
	}
        return true;
}

PTextShapeFunc
apc_gp_get_text_shaper( Handle self, int * type)
{
	*type = SHAPING_EMULATION;
#ifdef USE_XFT
	if ( ( X(self)-> font-> xft) ) {
		*type = SHAPING_GLYPH_MAPPING;
#ifdef WITH_HARFBUZZ
		if ( guts. use_harfbuzz ) {
			*type = SHAPING_FULL;
			return prima_xft_text_shaper_harfbuzz;
		} else
#endif
			return prima_xft_text_shaper_ident;
	}
#endif

	return text_shaper_core_text;
}

PFontABC
prima_xfont2abc( XFontStruct * fs, int firstChar, int lastChar)
{
	PFontABC abc = malloc( sizeof( FontABC) * (lastChar - firstChar + 1));
	XCharStruct *cs;
	int k, l, d = fs-> max_char_or_byte2 - fs-> min_char_or_byte2 + 1;
	int default_char1 = fs-> default_char >> 8;
	int default_char2 = fs-> default_char & 0xff;
	if ( !abc) return nil;

	if ( default_char2 < fs-> min_char_or_byte2 || default_char2 > fs-> max_char_or_byte2 ||
		default_char1 < fs-> min_byte1 || default_char1 > fs-> max_byte1) {
		default_char1 = fs-> min_byte1;
		default_char2 = fs-> min_char_or_byte2;
	}
	for ( k = firstChar, l = 0; k <= lastChar; k++, l++) {
		int i1 = k >> 8;
		int i2 = k & 0xff;
		if ( !fs-> per_char)
			cs = &fs-> min_bounds;
		else if ( i2 < fs-> min_char_or_byte2 || i2 > fs-> max_char_or_byte2 ||
					i1 < fs-> min_byte1 || i1 > fs-> max_byte1)
			cs = fs-> per_char +
				(default_char1 - fs-> min_byte1) * d +
					default_char2 - fs-> min_char_or_byte2;
		else
			cs = fs-> per_char +
				(i1 - fs-> min_byte1) * d +
					i2 - fs-> min_char_or_byte2;
		abc[l]. a = cs-> lbearing;
		abc[l]. b = cs-> rbearing - cs-> lbearing;
		abc[l]. c = cs-> width - cs-> rbearing;
	}
	return abc;
}

PFontABC
apc_gp_get_font_abc( Handle self, int firstChar, int lastChar, int flags)
{
	PFontABC abc;

	if ( self) {
		DEFXX;
#ifdef USE_XFT
		if ( XX-> font-> xft)
			return prima_xft_get_font_abc( self, firstChar, lastChar, flags);
#endif

		abc = prima_xfont2abc( XX-> font-> fs, firstChar, lastChar);
	} else
		abc = prima_xfont2abc( guts. font_abc_nil_hack, firstChar, lastChar);
	return abc;
}

PFontABC
prima_xfont2def( Handle self, int first, int last)
{
	DEFXX;
	XCharStruct *max;
	Pixmap pixmap;
	GC gc;
	XGCValues gcv;
	int i, j, k, w, h, ls;
	PFontABC ret;
	XImage *xi;

	if ( !( ret = malloc( sizeof(FontABC) * ( last - first + 1 ) )))
		return nil;
	bzero( ret, sizeof(FontABC) * ( last - first + 1 ));

	max = &XX-> font-> fs-> max_bounds;
	w  = max-> width * 3;
	h  = max-> descent + max-> ascent;
	ls = (( w + 31) / 32) * 4;
	w  = ls * 8;
	pixmap = XCreatePixmap( DISP, guts. root, w, h, 1);
	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, pixmap, GCGraphicsExposures, &gcv);
	XSetFont( DISP, gc, XX-> font-> id);

	for ( i = 0; i <= last - first; i++) {
		XChar2b ch;
		ch. byte1 = (first + i) / 256;
		ch. byte2 = (first + i) % 256;
		XSetForeground( DISP, gc, 0);
		XFillRectangle( DISP, pixmap, gc, 0, 0, w, h);
		XSetForeground( DISP, gc, 1);
		XDrawString16( DISP, pixmap, gc, 10, h - XX->font->font. descent, &ch, 1);

		if (!(xi = XGetImage( DISP, pixmap, 0, 0, w, h, 1, XYPixmap))) {
			free( ret );
			ret = nil;
			break;
		}
		/*
		for ( j = 0; j < h; j++) {
			int k, l;
			for ( k = 0; k < ls; k++) {
				Byte * p = (Byte*)xi-> data + j * xi-> bytes_per_line + k;
				printf(".");
				for ( l = 0; l < 8; l++) {
					int z = (*p) & ( 1 << l );
					printf("%s", z ? "*" : " ");
				}
			}
			printf("\n");
		}
		*/
		for ( j = 0; j < h; j++) {
			Byte * p = (Byte*)xi-> data + j * xi-> bytes_per_line;
			for ( k = 0; k < ls; k++, p++) {
				if ( *p != 0 ) {
					ret[i]. c = j;
					goto FOUND_C;
				}
			}
		}
		FOUND_C:
		for ( j = h - 1; j >= 0; j--) {
			Byte * p = (Byte*)xi-> data + j * xi-> bytes_per_line;
			for ( k = 0; k < ls; k++, p++) {
				if ( *p != 0 ) {
					ret[i]. a = h - j - 1;
					goto FOUND_A;
				}
			}
		}
		FOUND_A:
		if ( ret[i].a != 0 || ret[i].c != 0)
			ret[i]. b = h - ret[i]. a - ret[i]. c;
		XDestroyImage( xi);
	}

	XFreeGC( DISP, gc);
	XFreePixmap( DISP, pixmap);

	return ret;
}

PFontABC
apc_gp_get_font_def( Handle self, int firstChar, int lastChar, int flags)
{
	PFontABC abc;
#ifdef USE_XFT
	DEFXX;
	if ( XX-> font-> xft)
		return prima_xft_get_font_def( self, firstChar, lastChar, flags);
#endif
	abc = prima_xfont2def( self, firstChar, lastChar);
	return abc;
}

static int
gp_get_text_width( Handle self, const char *text, int len, int flags)
{
	DEFXX;
	int ret;
	ret = (flags & toUTF8) ?
		XTextWidth16( XX-> font-> fs, ( XChar2b *) text, len) :
		XTextWidth  ( XX-> font-> fs, (char*) text, len);
	if ( flags & toAddOverhangs ) {
		Point ovx = gp_get_text_overhangs( self, text, len, flags);
		ret += ovx. x + ovx. y;
	}
	return ret;
}

int
apc_gp_get_text_width( Handle self, const char * text, int len, int flags)
{
	int ret;

	flags &= ~toGlyphs;
	if ( len > 65535 ) len = 65535;

#ifdef USE_XFT
	if ( X(self)-> font-> xft)
		return prima_xft_get_text_width( X(self)-> font, text, len, flags,
			X(self)-> xft_map8, nil);
#endif

	if ( flags & toUTF8 )
		if ( !( text = ( char *) prima_alloc_utf8_to_wchar( text, len))) return 0;
	ret = gp_get_text_width( self, text, len, flags);
	if ( flags & toUTF8)
		free(( char*) text);
	return ret;
}

int
apc_gp_get_glyphs_width( Handle self, PGlyphsOutRec t)
{
	int ret;
	if ( t->len > 65535 ) t->len = 65535;

#ifdef USE_XFT
	if ( X(self)-> font-> xft)
		return prima_xft_get_glyphs_width( X(self)-> font, t,
			X(self)-> xft_map8, nil);
#endif

	SWAP_BYTES(t->glyphs,t->len);
	ret = gp_get_text_width( self, (char*) t->glyphs, t->len, toUTF8 );
	SWAP_BYTES(t->glyphs,t->len);
	return ret;
}

static Point *
gp_get_text_box( Handle self, const char * text, int len, int flags)
{
	DEFXX;
	Point * pt = ( Point *) malloc( sizeof( Point) * 5);
	int x;
	Point ovx;

	if ( !pt) return nil;
	if ( flags & toGlyphs ) flags &= ~toUTF8;

	x = (flags & toUTF8) ?
		XTextWidth16( XX-> font-> fs, ( XChar2b*) text, len) :
		XTextWidth( XX-> font-> fs, (char*)text, len);
	ovx = gp_get_text_overhangs( self, text, len, flags);

	pt[0].y = pt[2]. y = XX-> font-> font. ascent - 1;
	pt[1].y = pt[3]. y = - XX-> font-> font. descent;
	pt[4].y = 0;
	pt[4].x = x;
	pt[3].x = pt[2]. x = x + ovx. y;
	pt[0].x = pt[1]. x = - ovx. x;

	if ( !XX-> flags. paint_base_line) {
		int i;
		for ( i = 0; i < 4; i++) pt[i]. y += XX-> font-> font. descent;
	}

	if ( PDrawable( self)-> font. direction != 0) {
		int i;
		double s = sin( PDrawable( self)-> font. direction / 57.29577951);
		double c = cos( PDrawable( self)-> font. direction / 57.29577951);
		for ( i = 0; i < 5; i++) {
			double x = pt[i]. x * c - pt[i]. y * s;
			double y = pt[i]. x * s + pt[i]. y * c;
			pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
			pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
		}
	}

	return pt;
}

Point *
apc_gp_get_text_box( Handle self, const char * text, int len, Bool utf8)
{
	Point * ret;

	if ( len > 65535 ) len = 65535;

#ifdef USE_XFT
	if ( X(self)-> font-> xft)
		return prima_xft_get_text_box( self, text, len, utf8);
#endif
	if ( utf8)
		if ( !( text = ( char *) prima_alloc_utf8_to_wchar( text, len))) return 0;
	ret = gp_get_text_box( self, text, len, utf8);
	if ( utf8)
		free(( char*) text);
	return ret;
}

Point *
apc_gp_get_glyphs_box( Handle self, PGlyphsOutRec t)
{
	Point * ret;
	if ( t->len > 65535 ) t->len = 65535;
#ifdef USE_XFT
	if ( X(self)-> font-> xft)
		return prima_xft_get_glyphs_box( self, t);
#endif
	SWAP_BYTES(t->glyphs,t->len);
	ret = gp_get_text_box( self, (char*) t->glyphs, t->len, toUTF8);
	SWAP_BYTES(t->glyphs,t->len);
	return ret;
}

