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
stroke( Handle self, char * method, ...)
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
	ret = call_perl_indirect( self, "stroke_primitive", format, true, false, args);
	va_end( args);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}


Bool
Drawable_bar( Handle self, double x1, double y1, double x2, double y2)
{
	NRect nrect = {x1,y1,x2,y2};
	NPoint npoly[4];
	gpCHECK(false);

	if ( prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly)) {
		Rect r;
		if ( !var->antialias ) FLOOR4(nrect.left,nrect.top,nrect.right,nrect.bottom);
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

	gpCHECK(false);
	NRect nrect = {0.0,0.0,1.0,1.0};
	NPoint npoly[4];

	if ( !IS_AA && prima_matrix_is_identity( VAR_MATRIX )) {
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

	is_rect = prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly);

	if ( is_rect ) {
		int i;
		NRect *r;
		for ( i = 0, r = p; i < count; i++, r++)
			prima_matrix_is_square_rectangular( VAR_MATRIX, r, npoly);

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
			prima_matrix_is_square_rectangular( VAR_MATRIX, r, npoly);
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
	gpCHECK(false);

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0 ) {
		nrect.left   = x1 = 0;
		nrect.bottom = y1 = 0;
		nrect.right  = x2 = var-> w - 1;
		nrect.top    = y2 = var-> h - 1;
		is_pure_rect = true;
	} else {
		if ( prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly)) {
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
Drawable_fillpoly(Handle self, SV * points)
{
	Bool do_free = true;
	int count;
	void *p;
	Bool ret = false;
	gpCHECK(false);

	if ( prima_matrix_is_identity(VAR_MATRIX)) {
		if ( !IS_AA ) {
			/* fast integer case */
			if (( p = prima_read_array(
				points, "fillpoly", 'i',
				2, 2, -1, &count,
				&do_free
			)) == NULL)
				return false;
			ret = apc_gp_fill_poly( self, count, (Point*) p);
			goto EXIT;
		} else if ( var->antialias ) {
			/* fast double case */
			if (( p = prima_read_array(
				points, "fillpoly", 'd',
				2, 2, -1, &count,
				&do_free
			)) == NULL)
				return false;
			ret = apc_gp_aa_fill_poly( self, count, (NPoint*) p);
			goto EXIT;
		}
	}

	/* generic case */
	if (( p = prima_read_array(
		points, "fillpoly",
		IS_AA ? 'd' : 'i',
		2, 2, -1, &count,
		NULL
	)) == NULL)
		return false;

	if ( IS_AA ) {
		prima_matrix_apply2( VAR_MATRIX, p, p, count);
		if ( !var->antialias )
			FLOORN(p, count*2);
		ret = apc_gp_aa_fill_poly( self, count, (NPoint*) p);
	} else {
		prima_matrix_apply2_int_to_int( VAR_MATRIX, p, p, count);
		ret = apc_gp_fill_poly( self, count, (Point*) p);
	}

EXIT:
	if ( !ret) perl_error();
	if ( do_free) free(p);
	return ret;
}

Bool
Drawable_line(Handle self, double x1, double y1, double x2, double y2)
{
	gpCHECK(false);
	if (EMULATED_LINE)
		return stroke( self, "snnnn", "line", x1, y1, x2, y2);

	prima_matrix_apply( VAR_MATRIX, &x1, &y1);
	prima_matrix_apply( VAR_MATRIX, &x2, &y2);

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

	want_double    = !prima_matrix_is_translated_only( VAR_MATRIX );
	want_new_array = want_double ? true : !prima_matrix_is_identity( VAR_MATRIX );

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
		prima_matrix_apply2_to_int( VAR_MATRIX, (NPoint*)p, np, count);
		free(p);
		p = np;
	} else if ( want_new_array ) {
		prima_matrix_apply2_int_to_int( VAR_MATRIX, p, p, count);
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
	gpCHECK(false);

	if (EMULATED_LINE)
		return stroke( self, "sS", "lines", lines);
	else
		return read_polypoints( self, lines, "Drawable::lines", 2, apc_gp_draw_poly2);
}

Bool
Drawable_polyline(Handle self, SV * lines)
{
	gpCHECK(false);
	if (EMULATED_LINE)
		return stroke( self, "sS", "line", lines);
	else
		return read_polypoints( self, lines, "Drawable::polyline", 2, apc_gp_draw_poly);
}

Bool
Drawable_rectangle( Handle self, double x1, double y1, double x2, double y2)
{
	NPoint npoly[4];
	NRect nrect = {x1,y1,x2,y2};

	gpCHECK(false);
	if ( EMULATED_LINE )
		return stroke( self, "snnnn", "rectangle", x1,y1,x2,y2);

	if ( prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly)) {
		Rect r;
		prima_array_convert( 4, &nrect, 'd', &r, 'i');
		return apc_gp_rectangle(self, r.left, r.bottom, r.right, r.top);
	} else {
		Point poly[5];
		prima_array_convert( 8, &npoly, 'd', &poly, 'i');
		poly[4] = poly[0];
		return apc_gp_draw_poly( self, 5, poly);
	}
}

#ifdef __cplusplus
}
#endif
