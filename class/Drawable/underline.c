#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "private/Drawable.h"

#if 0
#define _DEBUG
#define DEBUG if(1)printf
#else
#define DEBUG if(0)printf
#endif

/*

Renders smart underline based on glyph outlines:

$ perl -I. -Iblib/arch examples/text-render.pl 14.Arial ups --underline
 ▄    ▄  ▄ ▄▄▄    ▄▄▄
 █    █  █▀  ▀█  █   ▀
 █    █  █    █  ▀▀█▄▄
 █   ██  █▄  ▄█  ▄   █
  ▀▀▀ ▀  █ ▀▀▀   ▀▀▀▀
▀▀▀▀▀▀▀▀ █  ▀▀▀▀▀▀▀▀▀▀▀

*/

#ifdef __cplusplus
extern "C" {
#endif

/*
Fill the 2-int array meaning:

(-2,-2) - glyph descent is present but not fetched
(-1,-1) - glyph doesn't intersect the descent line
(A,B)   - glyph intersects the line from A to B
*/

static void
check_intersections( Point a, Point b, int y0, int y1, int *x_margins)
{
	int i;

	DEBUG("? %d.%d - %d.%d x (%d %d)\n", a.x, a.y, b.x, b.y, y0, y1);
	if ( a.y > b.y ) {
		Point s = a;
		a = b;
		b = s;
	}

	if ( a.y > y1 || b.y < y0 ) {
		return;
	} else if ( a.y >= y0 && b.y <= y1 ) {
		int min = ( a.x < b.x ) ? a.x : b.x;
		int max = ( a.x < b.x ) ? b.x : a.x;

		for ( i = 0; i < ((min == max) ? 1 : 2); i++) {
			int x = (i == 0) ? min : max;
			if ( x < 0 ) x = 0;
			if ( x_margins[1] >= 0 ) {
				if ( x_margins[0] < 0 || x_margins[0] > x)
					x_margins[0] = x;
				if ( x_margins[1] < x)
					x_margins[1] = x;
			} else if ( x_margins[0] < 0 )
				x_margins[0] = x_margins[1] = x;
		}
		DEBUG("x> %d %d\n", x_margins[0], x_margins[1]);
	} else {
		for ( i = 0; i < ((y0 == y1) ? 1 : 2); i++) {
			int y = (i == 0) ? y0 : y1;

			if ( a.y < y && b.y >= y ) {
				int x = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
				if ( x < 0 ) x = 0;
				if ( x_margins[1] >= 0 ) {
					if ( x_margins[0] < 0 || x_margins[0] > x)
						x_margins[0] = x;
					if ( x_margins[1] < x)
						x_margins[1] = x;
				} else if ( x_margins[0] < 0 )
					x_margins[0] = x_margins[1] = x;
			}
		}
		DEBUG("y> %d %d\n", x_margins[0], x_margins[1]);
	}
}

static void
fill_glyph_intersection_info(Handle self, unsigned int index, int *descents, UnderlineInfo *u)
{
	int k, count, *buffer = NULL, *p;
	Point anchor = {0,0}, curr_point = {0,0}, next_point;
	Bool close_shape = false, got_anchor = false, got_curr_point = false;

	count = apc_gp_get_glyph_outline( self, index, ggoGlyphIndex, &buffer);

	DEBUG("GLYPH %d\n", index);
	if ( count < 0 || !buffer) {
		descents[0] = descents[1] = -1;
		return;
	}

#define POINT(x) ((x) / 64.0 + .5)
	for ( k = 0, p = buffer; k < count;) {
		int cmd      = *(p++);
		int n_subcmd = *(p++);
		int *p_sub, sub;
		k += n_subcmd * 2 + 2;
		DEBUG("cmd %d\n", n_subcmd);
		switch ( cmd ) {
		case ggoMove:
			anchor.x    = POINT(p[0]);
			anchor.y    = POINT(p[1]);
			if ( got_curr_point )
				check_intersections( curr_point, anchor, u->y[0], u->y[1], descents);
			close_shape = false;
			got_anchor  = true;
			break;
		case ggoLine:
		case ggoConic:
		case ggoCubic:
			next_point.x = POINT(p[0]);
			next_point.y = POINT(p[1]);
			if ( got_curr_point )
				check_intersections( curr_point, next_point, u->y[0], u->y[1], descents);
			for ( sub = 1, p_sub = p + 1; sub < n_subcmd; sub++) {
				curr_point   = next_point;
				next_point.x = POINT(*(p_sub++));
				next_point.y = POINT(*(p_sub++));
				check_intersections( curr_point, next_point, u->y[0], u->y[1], descents);
			}
			break;
		default:
			p += n_subcmd * 2;
			continue;
		}

		p += n_subcmd * 2;
		curr_point.x   = POINT(p[-2]);
		curr_point.y   = POINT(p[-1]);
		got_curr_point = true;
		close_shape    = true;
	}
#undef POINT

	if ( close_shape && got_anchor )
		check_intersections( curr_point, anchor, u->y[0], u->y[1], descents);

	/* if not the whole glyph, extend the X parts by half of the line thickness */
	descents[0]   -= u->extra_margin;
	if ( descents[0] < 0 )
		descents[0] = 0;
	descents[1] += u->extra_margin;

	DEBUG("=> %d %d\n", descents[0], descents[1]);
	if ( buffer )
		free(buffer);
}

void
Drawable_clear_descent_crossing_caches( Handle self)
{
	int i,j;
	PList l = var->glyph_descents;
	if ( !l )
		return;
	for ( i = 1; i < l->count; i += 2 ) {
		int *descents = (int*) (l->items[i]);
		for ( j = 0; j < BASE_RANGE; j++, descents += 2)
			if ( descents[0] >= 0 )
				descents[0] = descents[1] = -2;
	}
	if ( var->underline_info ) {
		free(var->underline_info);
		var->underline_info = NULL;
	}
}

int*
Drawable_call_get_glyph_descents( Handle self, unsigned int from, unsigned int to)
{
	dmARGS;
	int i, j, breakout, len = to - from + 1;
	int *descents;
	PFontABC abc;
	Bool want_dm_leave = false;

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_def( self, from, to, toGlyphs);
		if ( !abc) return NULL;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		dmCHECK(NULL);
		dmENTER(NULL);
		abc = apc_gp_get_font_def( self, from, to, toGlyphs);
		if ( !abc) {
			dmLEAVE;
			return NULL;
		}
		want_dm_leave = true;
	} else {
		SV * sv;
		if ( !( abc = malloc(len * sizeof( FontABC)))) return NULL;
		sv = my-> get_font_def( self, from, to, toGlyphs);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > len * 3) n = len * 3;
			n = ( n / 3) * 3;
			if ( n < len) memset( abc, 0, len * sizeof( FontABC));
			for ( i = 0; i < n; i += 3) {
				SV ** holder = av_fetch( av, i, 0);
				if ( holder) abc[j].a = ( float) SvNV( *holder);
				j++;
			}
		} else
			memset( abc, 0, len * sizeof( FontABC));
		sv_free( sv);
	}


	{
		int ut = (var->font.underlineThickness <= 0) ?
			1 :
			var->font.underlineThickness;
		int up = (var->font.underlinePosition < 0) ?
			var->font.underlinePosition :
			-(var->font.descent - ut/2 - 1);
		breakout = var->font.descent + up + ut;
	}


	if (!( descents = malloc(sizeof(int) * len * 2))) {
		free(abc);
		if ( want_dm_leave ) dmLEAVE;
		return NULL;
	}
	for ( i = j = 0; i < len; i++, j += 2)
		descents[j] = descents[j + 1] = -1;

	/* query glyph outlines, check intersections with the underline */
	for ( i = j = 0; i < len; i++, j += 2) {
		if ( abc[i].a >= breakout )
			continue;

		if ( !self || my-> render_glyph == Drawable_render_glyph) {
			descents[j] = descents[j+1] = -2;
		} else {
			/* XXX do it the old style so far */
			descents[j] = 0;
			descents[j+1] = INT_MAX;
		}
	}

	free(abc);

	if ( want_dm_leave ) dmLEAVE;
	return descents;
}


static int*
query_descent_range( Handle self, int base)
{
	int* ret;

	ret = (int*) Drawable_find_in_cache( var->glyph_descents, base);
	if ( ret == NULL) {
		ret = Drawable_call_get_glyph_descents( self, BASE_FROM(base), BASE_TO(base));
		if ( ret == NULL)
			return NULL;
		if ( !Drawable_fill_cache(&(var->glyph_descents), base, ret)) {
			free(ret);
			return NULL;
		}
	}

	return ret;
}

static UnderlineInfo*
get_underline_info( Handle self)
{
	UnderlineInfo *u;

	if ( var->underline_info)
		return var->underline_info;

	if ( ( var->underline_info = malloc(sizeof(UnderlineInfo))))
		u = var->underline_info;
	else
		u = &UnderlineInfo_buffer;

	u->thickness = (var->font.underlineThickness <= 0) ?
		1 :
		var->font.underlineThickness;

	u->position = (var->font.underlinePosition < 0) ?
		-var->font.underlinePosition :
		(var->font.descent - 1);

	if ( u->thickness > 1 ) {
		int i,j;
		for (i = 0; i < 2; i++) {
			NRect box;
			j = ( i == 0 ) ? leiArrowTail : leiArrowHead;
			j   = Drawable_resolve_line_end_index(&var->current_state, j);
			box = Drawable_line_end_box( &var->current_state, j);
			u->half_widths[i] = box.right * u->thickness;
		}
		u->half_widths[1] *= -1.0;
	} else
		u->half_widths[0] = u->half_widths[1] = 0.0;

	u->extra_margin = ( u->thickness > 2 ) ? u->thickness / 2 : 1;

	u->y[0] = u->position - u->extra_margin;
	u->y[1] = u->position + u->extra_margin + u->thickness - 1;

	if ( u->y[0] >= 0 ) u->y[0] = -1;
	if ( u->y[1] >= 0 ) u->y[1] = -1;

	DEBUG("UnderlineInfo: d=%d up=%d ut=%d y=%d %d\n", var->font.descent, u->position, u->thickness, u->y[0], u->y[0]);

	return u;
}

static NPoint*
render_underline(Handle self, int x, int y, GlyphsOutRec *t, int * n_points)
{
	dmARGS;
	int x0, y0, i, base;
	FontABC *abc = NULL, last_abc = {0,0,0};
	int *descents = NULL;
	Bool ok = true;
	Bool line_is_on;
	NPoint c, *ret;
	SaveFont savefont;
	UnderlineInfo *underline;

	x0 = x;
	y0 = y;
	x = y = 0;

	if ( var-> font. direction != 0)
		c = my->trig_cache(self);
	else
		c.x = c.y = 0;

	{
		Bool text_out_baseline;
		text_out_baseline = ( my-> textOutBaseline == Drawable_textOutBaseline) ?
			apc_gp_get_text_out_baseline(self) :
			my-> get_textOutBaseline(self);
		if ( !text_out_baseline )
			y += var->font.descent;

		underline = get_underline_info(self);
		y -= underline->position + ((underline->thickness > 1) ? (underline->thickness / 2) : 0);
	}

	*n_points = 0;
	dmCHECK(NULL);
	dmENTER(NULL);
	if (( ret = malloc(sizeof(NPoint) * 6 * t->len)) == NULL) {
		dmLEAVE;
		return NULL;
	}

	line_is_on = false;
	base = -1;

#define ADD_POINT(hw,use_last_c) {                                             \
	float xx = x - hw;                                                     \
	float this_a = ( abc[o].a < 0 ) ? -abc[o].a   : 0;                     \
	float last_c = ((use_last_c) && (last_abc.c < 0)) ? -last_abc.c : 0;   \
	xx -= this_a + last_c;                                                 \
	xx += underline->half_widths[hw];                                      \
	ret[ *n_points    ].x = xx;                                            \
	ret[ (*n_points)++].y = y;                                             \
	DEBUG("x=%d-%d a=%g c=%g hw=%g = %g\n",x,hw,this_a,last_c,underline->half_widths[hw],xx);\
}
#define LAST_X ret[(*n_points) - 1].x
#define CHECK_POINT                                                            \
	if ( *n_points > 1 && LAST_X - ret[(*n_points)-2].x <= underline->thickness ) {          \
		DEBUG("skip (%g-%g <= %d)\n", LAST_X,ret[(*n_points)-2].x,underline->thickness); \
		*n_points -= 2;                                                \
	}

	if ( t-> fonts )
		my->save_font(self, &savefont);
	for ( i = 0; i < t->len; i++) {
		int w;
		int o = GID2OFFSET(t->glyphs[i]), from, to;
		int b = FIDGID2BASE(t->fonts ? t->fonts[i] : 0, t->glyphs[i]);
		if ( base != b) {
			base = b;
			if ( t->fonts )
				my-> switch_font(self, &savefont, t->fonts[i]);
			abc      = Drawable_query_abc_glyph_range( self, base);
			descents = query_descent_range  ( self, base);
			if (abc == NULL || descents == NULL) {
				ok = false;
				break;
			}
		}

		from = descents[o * 2];
		if ( from == -2 ) {
			fill_glyph_intersection_info(self, t->glyphs[i], descents + o * 2, underline);
			from = descents[o * 2];
		}

		to   = descents[o * 2 + 1];
		w    = abc[o].a + abc[o].b + abc[o].c + .5;
		if ( to > w ) to = w;
		DEBUG("C %d (%g %g %g = %d) => %d %d\n", t->glyphs[i], abc[o].a, abc[o].b, abc[o].c, w, from, to);

		if ( from < 0 ) {
			if ( !line_is_on ) {
				/* start the line */
				ADD_POINT(0,1)
				CHECK_POINT;
				line_is_on = true;
			}
			/* else continue the existing line */
		} else if ( from == 0 ) {
			/* line wanted after a skip */
			if ( line_is_on ) {
				ADD_POINT(1,1)
				CHECK_POINT;
				DEBUG("a0: %g\n", LAST_X);
			} else
				line_is_on = true;
			ADD_POINT(0,0)
			LAST_X += to + 1;
			DEBUG("b: %g\n", LAST_X);
			CHECK_POINT;
		} else if ( to + 1 >= w ) {
			/* line wanted but not until the end */
			if ( line_is_on ) {
				line_is_on = false;
			} else {
				ADD_POINT(0,1)
				DEBUG("d0: %g\n", LAST_X);
				CHECK_POINT;
			}
			ADD_POINT(1,1)
			LAST_X += from - 1;
			CHECK_POINT;
			DEBUG("d1: %g\n", LAST_X);
		} else {
			/* glyph descent breaks the line */
			if ( !line_is_on ) {
				ADD_POINT(0,1)
				DEBUG("f0: %g\n", LAST_X);
				CHECK_POINT;
				line_is_on = true;
			}

			ADD_POINT(1,1);
			LAST_X += from - 1;
			DEBUG("f1: %g\n", LAST_X);
			CHECK_POINT;

			ADD_POINT(0,0)
			LAST_X += to + 1;
			DEBUG("f2: %g\n", LAST_X);
			CHECK_POINT;
		}

		x += w;
		last_abc = abc[o];
	}
	if ( t-> fonts )
		my->restore_font(self, &savefont);
	dmLEAVE;

	if ( !ok ) {
		free(ret);
		ret = NULL;
		*n_points = 0;
	}

	if ((( *n_points ) % 2) == 1 ) {
		int o = t->glyphs[t->len - 1] & 0xff;
		DEBUG("FINO\n");
		ADD_POINT(1,*n_points > 1)
		CHECK_POINT;
	}

#undef ADD_POINT
#undef LAST_X
#undef CHECK_POINT

	if ( *n_points > 0 ) {
		NPoint *p = ret;
		DEBUG("Y: %g X: ", p->y);
		for ( i = 0; i < *n_points; i++, p++) {
			DEBUG("%g ", p->x);
			if ( var-> font. direction != 0) {
				NPoint n = *p;
				p->x = n.x * c.y - n.y * c.x + x0;
				p->y = n.x * c.x + n.y * c.y + y0;
			} else {
				p-> x += x0;
				p-> y += y0;
			}
		}
	}

	return ret;
}

SV *
Drawable_render_underline(Handle self, SV * glyphs, int x, int y)
{
	AV * av;
	NPoint *np, *npi;
	int i, n_points;
	GlyphsOutRec t;
	if (!Drawable_read_glyphs(&t, glyphs, 0, "Drawable::render_underline"))
		return NULL_SV;
	np = render_underline(self, x, y, &t, &n_points);
	av = newAV();
	for ( i = 0, npi = np; i < n_points; i++, npi++) {
		if ( var-> antialias ) {
			av_push(av, newSVnv(npi->x));
			av_push(av, newSVnv(npi->y));
		} else {
			int z;
			z = (npi->x > 0) ? (npi->x + .5) : (npi->x - 0.5);
			av_push(av, newSViv(z));
			z = (npi->y > 0) ? (npi->y + .5) : (npi->y - 0.5);
			av_push(av, newSViv(z));
		}
	}
	free(np);
	return newRV_noinc(( SV *) av);
}

#ifdef __cplusplus
}
#endif
