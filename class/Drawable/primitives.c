#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "Drawable_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IS_AA (var->antialias || (var->alpha < 255))
#define EMULATED_LINE (var->antialias || (var->alpha < 255) || var->current_state.line_width > 0.0)
#define FLOOR(x) x=floor(x + .5)
#define FLOOR2(x,y) {FLOOR(x);FLOOR(y);}
#define FLOOR4(x,y,z,t) {FLOOR(x);FLOOR(y);FLOOR(z);FLOOR(t);}
#define FLOORN(x,n) {int i,m = n; double *src = (double*)x; for (i = 0; i < m; i++,src++) FLOOR(*src); }


static Bool
primitive( Handle self, Bool fill, char * method, ...)
{
	Bool r;
	SV * ret;
	char format[256];
	va_list args;
	va_start( args, method);
	ENTER;
	SAVETMPS;
	strcpy(format, "<");
	strncat(format, method, 255);
	ret = call_perl_indirect( self, fill ? "fill_primitive" : "stroke_primitive", format, true, false, args);
	va_end( args);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}


Bool
Drawable_arc( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	while ( startAngle > endAngle ) endAngle += 360.0;
	return primitive( self, 0, "snnnnnn", "arc", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Drawable_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return primitive( self, 0, "snnnnnn", "chord", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Drawable_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	CHECK_GP(false);
	return primitive( self, 0, "snnnn", "ellipse", x, y, dX-1, dY-1);
}

Bool
Drawable_bar( Handle self, double x1, double y1, double x2, double y2)
{
	NRect nrect = {x1,y1,x2,y2};
	NPoint npoly[4];
	CHECK_GP(false);

	if ( prima_matrix_is_square_rectangular( var->current_state.matrix, &nrect, npoly)) {
		Rect r;
		if ( !var->antialias ) FLOOR4(x1,y1,x2,y2);
		if (IS_AA)
			return apc_gp_aa_bars( self, 1, &nrect);
		prima_array_convert( 4, &nrect, 'd', &r, 'i');
		return apc_gp_bars(self, 1, &r);
	}

	if ( !var->antialias )
		FLOORN(npoly, 8);

	if ( IS_AA ) {
		return apc_gp_aa_fill_poly( self, 4, npoly);
	} else {
		Point poly[4];
		prima_array_convert( 8, &npoly, 'd', &poly, 'i');
		return apc_gp_fill_poly( self, 4, poly);
	}
}

Bool
Drawable_bars( Handle self, SV * rects)
{
	int count;
	NRect * p;
	Bool ok, is_rect;

	CHECK_GP(false);
	NRect nrect = {0.0,0.0,1.0,1.0};
	NPoint npoly[4];

	if ( !IS_AA && prima_matrix_is_identity( var->current_state.matrix )) {
		Bool do_free;
		Rect *p;
		if (( p = prima_read_array( rects, "Drawable::bars",
			'i', 4, 0, -1, &count, &do_free)) == NULL)
			return false;
		ok = apc_gp_bars( self, count, p);
		if ( do_free ) free(p);
		return ok;
	}

	if (( p = prima_read_array( rects, "Drawable::bars",
		'd', 4, 0, -1, &count, NULL)) == NULL)
		return false;

	is_rect = prima_matrix_is_square_rectangular( var->current_state.matrix, &nrect, npoly);

	if ( is_rect ) {
		int i;
		NRect *r;
		for ( i = 0, r = p; i < count; i++, r++)
			prima_matrix_is_square_rectangular( var->current_state.matrix, r, npoly);

		if ( IS_AA ) {
			if ( !var->antialias ) FLOORN( p, count * 4 );
			ok = apc_gp_aa_bars( self, count, p);
		} else {
			Rect *pp;
			if ( !( pp = prima_array_convert( count * 4, p, 'd', NULL, 'i'))) {
				free(p);
				return false;
			}
			ok = apc_gp_bars( self, count, pp);
			free(pp);
		}
	} else {
		int i;
		NRect *r;
		for ( i = 0, r = (NRect*) p; i < count; i++, r++) {
			prima_matrix_is_square_rectangular( var->current_state.matrix, r, npoly);
			if ( IS_AA ) {
				if ( !var->antialias )
					FLOORN(npoly, 8);
				if ( !apc_gp_aa_fill_poly( self, 4, npoly))
					break;
			} else {
				Point poly[4];
				prima_array_convert( count * 8, npoly, 'd', poly, 'i');
				if (!apc_gp_fill_poly( self, 4, poly))
					break;
			}
		}
		ok = true;
	}

	if ( !ok) perl_error();
	free( p);
	return ok;
}

Bool
Drawable_clear( Handle self, double x1, double y1, double x2, double y2)
{
	Bool ok;
	Bool is_pure_rect = false;
	NRect nrect = {x1,y1,x2,y2};
	NPoint npoly[4];
	CHECK_GP(false);

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0 ) {
		nrect.left   = x1 = 0;
		nrect.bottom = y1 = 0;
		nrect.right  = x2 = var-> w - 1;
		nrect.top    = y2 = var-> h - 1;
		is_pure_rect = true;
	} else {
		if ( prima_matrix_is_square_rectangular( var->current_state.matrix, &nrect, npoly)) {
			FLOOR4(nrect.left,nrect.bottom,nrect.right,nrect.top);
			is_pure_rect = true;
		} else if ( !var-> antialias )
			FLOORN( npoly, 8 );
	}

	if ( !var->antialias && is_pure_rect )
		return apc_gp_clear(self, x1, y1, x2, y2);

	if ( !my->graphic_context_push(self)) return false;
	apc_gp_set_alpha(self, 255);
	apc_gp_set_color(self, apc_gp_get_back_color(self));
	apc_gp_set_rop(self, ropCopyPut);
	apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
	if ( is_pure_rect )
		ok = apc_gp_aa_bars( self, 1, &nrect);
	else if ( var-> antialias )
		ok = apc_gp_aa_fill_poly( self, 4, npoly);
	else {
		Point poly[4];
		prima_array_convert( 8, npoly, 'd', poly, 'i');
		ok = apc_gp_fill_poly( self, 4, poly);
	}
	my->graphic_context_pop(self);
	return ok;
}

Bool
Drawable_fill_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return primitive( self, 1, "snnnnnn", "chord", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Drawable_fill_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	CHECK_GP(false);
	return primitive( self, 1, "snnnn", "ellipse", x, y, dX-1, dY-1);
}

Bool
Drawable_fill_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return primitive( self, 1, "snnnnnn", "sector", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Drawable_fillpoly(Handle self, SV * points)
{
	Bool do_free = true;
	int count;
	void *p;
	Bool ret = false;
	CHECK_GP(false);

	if ( !IS_AA && prima_matrix_is_translated_only(var-> current_state.matrix)) {
		Bool is_identity = prima_matrix_is_identity(var->current_state.matrix);
		/* fast integer case */
		if (( p = prima_read_array(
			points, "fillpoly", 'i',
			2, 2, -1, &count,
			is_identity ? &do_free : NULL
		)) == NULL)
			return false;
		if ( !is_identity ) {
			int i;
			Point *pt, t;
			t.x = floor(var->current_state.matrix[4] + .5);
			t.y = floor(var->current_state.matrix[5] + .5);
			for ( i = 0, pt = p; i < count; i++, pt++) {
				pt->x += t.x;
				pt->y += t.y;
			}
		}
		ret = apc_gp_fill_poly( self, count, (Point*) p);
		goto EXIT;
	} else if (
		var-> alpha == 255 && var->antialias &&
		prima_matrix_is_identity( var->current_state.matrix)
	) {
		/* fast double case */
		if (( p = prima_read_array(
			points, "fillpoly", 'd',
			2, 2, -1, &count,
			&do_free
		)) == NULL)
			return false;
		ret = apc_gp_aa_fill_poly( self, count, (NPoint*) p);
		goto EXIT;
	} else {
		/* generic case */
		if (( p = prima_read_array(
			points, "fillpoly",
			IS_AA ? 'd' : 'i',
			2, 2, -1, &count,
			NULL
		)) == NULL)
			return false;
		prima_matrix_apply2( var->current_state.matrix, p, p, count);

		if ( IS_AA ) {
			if ( !var->antialias )
				FLOORN(p, count*2);
			ret = apc_gp_aa_fill_poly( self, count, (NPoint*) p);
		} else {
			Point *pp;
			if ( !( pp = prima_array_convert( count * 2, p, 'd', NULL, 'i')))
				goto EXIT;
			ret = apc_gp_fill_poly( self, count, (Point*) pp);
		}
	}

EXIT:
	if ( !ret) perl_error();
	if ( do_free) free(p);
	return ret;
}

Bool
Drawable_line(Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);
	if (EMULATED_LINE)
		return primitive( self, 0, "snnnn", "line", x1, y1, x2, y2);

	prima_matrix_apply( var->current_state.matrix, &x1, &y1);
	prima_matrix_apply( var->current_state.matrix, &x2, &y2);

	return apc_gp_line(self,
		x1, y1, x2, y2
	);
}

static Bool
read_polypoints( Handle self, SV * points, char * procName, int min, Bool (*procPtr)(Handle,int,Point*))
{
	int count;
	void *p;
	Bool ret = false;
	Bool do_free = true, want_new_array, want_double;

	want_double    = !prima_matrix_is_translated_only( var->current_state.matrix );
	want_new_array = want_double ? true : !prima_matrix_is_identity( var->current_state.matrix );

	if (( p = prima_read_array( points, procName,
		want_double ? 'd' : 'i',
		2, min, -1, &count,
		want_new_array ? NULL : &do_free)) == NULL
	)
		return false;

	if ( want_double ) {
		Point *np;
		if ( !( np = malloc( count * sizeof(Point))))
			goto EXIT;
		prima_matrix_apply2_to_int( var->current_state.matrix, (NPoint*)p, np, count);
		free(p);
		p = np;
	} else if ( want_new_array ) {
		int i;
		Point t, *pt;
		t.x = floor( var->current_state.matrix[4] + .5);
		t.y = floor( var->current_state.matrix[5] + .5);
		for ( i = 0, pt = (Point*) p; i < count; i++, pt++) {
			pt->x += t.x;
			pt->y += t.y;
		}
	}

	ret = procPtr( self, count, (Point*) p);
	if ( !ret) perl_error();
EXIT:
	if ( do_free ) free(p);
	return ret;
}

Bool
Drawable_lines(Handle self, SV * lines)
{
	CHECK_GP(false);

	if (EMULATED_LINE)
		return primitive( self, 0, "sS", "lines", lines);
	else
		return read_polypoints( self, lines, "Drawable::lines", 2, apc_gp_draw_poly2);
}

Bool
Drawable_polyline(Handle self, SV * lines)
{
	CHECK_GP(false);
	if (EMULATED_LINE)
		return primitive( self, 0, "sS", "line", lines);
	else
		return read_polypoints( self, lines, "Drawable::polyline", 2, apc_gp_draw_poly);
}

Bool
Drawable_rectangle( Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);
	if ( EMULATED_LINE )
		return primitive( self, 0, "snnnn", "rectangle", x1,y1,x2,y2);

	prima_matrix_apply( var->current_state.matrix, &x1, &y1);
	prima_matrix_apply( var->current_state.matrix, &x2, &y2);

	return apc_gp_rectangle(self, x1, y1, x2, y2);
}

Bool
Drawable_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return primitive( self, 0, "snnnnnn", "sector", x, y, dX-1, dY-1, startAngle, endAngle);
}

#ifdef __cplusplus
}
#endif
