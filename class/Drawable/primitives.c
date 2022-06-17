#include "apricot.h"
#include "Drawable.h"
#include "Drawable_private.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	ret = call_perl_indirect( self, fill ? "fill_aa_primitive" : "stroke_aa_primitive", format, true, false, args);
	va_end( args);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}

#define IS_AA (var->antialias || (var->alpha < 255))
#define TRUNC(x) x=trunc(x)
#define TRUNC2(x,y) {TRUNC(x);TRUNC(y);}
#define TRUNC4(x,y,z,t) {TRUNC(x);TRUNC(y);TRUNC(z);TRUNC(t);}


Bool
Drawable_arc( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	while ( startAngle > endAngle ) endAngle += 360.0;
	return IS_AA ?
		primitive( self, 0, "snnnnnn", "arc", x, y, dX-1, dY-1, startAngle, endAngle) :
		apc_gp_arc(self, x, y, dX, dY, startAngle, endAngle);
}

Bool
Drawable_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnnnn", "chord", x, y, dX-1, dY-1, startAngle, endAngle) :
		apc_gp_chord(self, x, y, dX, dY, startAngle, endAngle);
}

Bool
Drawable_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnn", "ellipse", x, y, dX-1, dY-1) :
		apc_gp_ellipse(self, x, y, dX, dY);
}

Bool
Drawable_bar( Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);

	if ( !var->antialias ) TRUNC4(x1,y1,x2,y2);

	return IS_AA ?
		apc_gp_aa_bar( self, x1, y1, x2, y2) :
		apc_gp_bar(self, x1, y1, x2, y2);
}

Bool
Drawable_bars( Handle self, SV * rects)
{
	int count;
	Rect * p;
	Bool ret = false, do_free;
	CHECK_GP(false);
	if (( p = prima_read_array( rects, "Drawable::bars",
		IS_AA ? 'd' : 'i',
		4, 0, -1, &count, &do_free)) == NULL)
		return false;

	if ( IS_AA ) {
		int i;
		NRect *r;
		for ( i = 0, r = (NRect*)p; i < count; i++, r++) {
			int x1 = r-> left,y1 = r->bottom, x2 = r->right, y2 = r->top;
			if ( !var->antialias ) TRUNC4(x1,y1,x2,y2);
			if ( !apc_gp_aa_bar( self, x1, y1, x2, y2))
				break;
		}
	} else
		ret = apc_gp_bars( self, count, p);
	if ( !ret) perl_error();
	if ( do_free ) free( p);
	return ret;
}

Bool
Drawable_clear( Handle self, double x1, double y1, double x2, double y2)
{
	Bool full;
	CHECK_GP(false);

	full = x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0;
	if ( !var->antialias ) TRUNC4(x1,y1,x2,y2);
	if ( !full && IS_AA) {
		Bool ok;
<<<<<<< HEAD
		if ( !my->graphic_context_push(self)) return false;
		apc_gp_set_color(self, apc_gp_get_back_color(self));
		apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
		ok = apc_gp_aa_bar( self, x1, y1, x2, y2);
		my->graphic_context_pop(self);
=======
		if ( !apc_gp_push(self)) return false;
		apc_gp_set_color(self, apc_gp_get_back_color(self));
		apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
		ok = apc_gp_aa_bar( self, x1, y1, x2, y2);
		apc_gp_pop(self);
>>>>>>> 3e818d51 (apply graphic_context())
		return ok;
	} else return apc_gp_clear(self,
		x1,y1,x2,y2
	);
}

Bool
Drawable_fill_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	if (IS_AA) {
		return primitive( self, 1, "snnnnnn", "chord", x, y, dX, dY, startAngle, endAngle);
	} else return apc_gp_fill_chord(self,
		x, y, dX, dY, startAngle, endAngle
	);
}

Bool
Drawable_fill_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	if (IS_AA) {
		return primitive( self, 1, "snnnn", "ellipse", x, y, dX, dY);
	} else return apc_gp_fill_ellipse(self,
		x, y, dX, dY
	);
}

Bool
Drawable_fill_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	if (IS_AA) {
		return primitive( self, 1, "snnnnnn", "sector", x, y, dX, dY, startAngle, endAngle);
	} else return apc_gp_fill_sector(self,
		x, y, dX, dY, startAngle, endAngle
	);
}

Bool
Drawable_fillpoly(Handle self, SV * points)
{
	int count;
	void *p;
	Bool ret = false;
	Bool do_free = true;
	CHECK_GP(false);

	if (( p = prima_read_array(
		points, "fillpoly",
		IS_AA ? 'd' : 'i',
		2, 2, -1, &count, 
		(var->alpha < 255 && !var->antialias) ? NULL : &do_free
	)) == NULL)
		return false;

	if ( var->alpha < 255 && !var->antialias ) {
		int i;
		NPoint *pp = (NPoint*)p;
		for ( i = 0; i < count; i++, pp++) TRUNC2(pp->x,pp->y);
	}

	ret = IS_AA ?
		apc_gp_aa_fill_poly( self, count, (NPoint*) p) :
		apc_gp_fill_poly( self, count, (Point*) p);
	if ( !ret) perl_error();
	if ( do_free ) free(p);

	return ret;
}

Bool
Drawable_line(Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);
	if (IS_AA)
		return primitive( self, 0, "snnnn", "line", x1, y1, x2, y2);
	else return apc_gp_line(self,
		x1, y1, x2, y2
	);
}

static Bool
read_polypoints( Handle self, SV * points, char * procName, int min, Bool (*procPtr)(Handle,int,Point*))
{
	int count;
	Point * p;
	Bool ret = false;
	Bool do_free;
	if (( p = (Point*) prima_read_array( points, procName, 'i', 2, min, -1, &count, &do_free)) != NULL) {
		ret = procPtr( self, count, p);
		if ( !ret) perl_error();
		if ( do_free ) free(p);
	}
	return ret;
}

Bool
Drawable_lines(Handle self, SV * lines)
{
	CHECK_GP(false);

	if (IS_AA)
		return primitive( self, 0, "sS", "lines", lines);
	else
		return read_polypoints( self, lines, "Drawable::lines", 2, apc_gp_draw_poly2);
}

Bool
Drawable_polyline(Handle self, SV * lines)
{
	CHECK_GP(false);

	if (IS_AA)
		return primitive( self, 0, "sS", "line", lines);
	else
		return read_polypoints( self, lines, "Drawable::polyline", 2, apc_gp_draw_poly);
}

Bool
Drawable_rectangle( Handle self, double x1, double y1, double x2, double y2)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnn", "rectangle", x1,y1,x2,y2) :
		apc_gp_rectangle(self, x1, y1, x2, y2);
}

Bool
Drawable_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	CHECK_GP(false);
	return IS_AA ?
		primitive( self, 0, "snnnnnn", "sector", x, y, dX-1, dY-1, startAngle, endAngle) :
		apc_gp_sector(self, x, y, dX, dY, startAngle, endAngle);
}

#ifdef __cplusplus
}
#endif
