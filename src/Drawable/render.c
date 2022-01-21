#include "apricot.h"
#include "Drawable.h"
#include "Drawable_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/*

Render B-spline

thanks to :

Marcel Steinbeck for tinyspline.c
Thibaut Séguy for bspline.js
Carl-Wilhelm Reinhold de Boor for the rendering algorithm

*/

/* fill default set of knots for curve */
static double *
default_knots( int n_points, int degree, Bool clamped )
{
	double * ret, * ptr;
	int i, n_knots, order;

	order = degree + 1;
	n_knots = n_points + order;
	if ( !( ret = malloc( sizeof(double) * n_knots )))
		return NULL;
	ptr = ret;

	if ( clamped ) {
		int slope = n_knots - order * 2;
		double fac = 1.0 / (n_knots - 2 * degree - 1);
		for ( i = 0; i < order; i++, ptr++) *ptr = 0.0;
		for ( i = 0; i < slope; i++, ptr++) *ptr = fac * (i + 1);
		for ( i = 0; i < order; i++, ptr++) *ptr = 1.0;
	} else {
		for ( i = 0; i < n_knots; i++, ptr++) *ptr = (double) i;
	}

	return ret;
}

/* render single point on a spline with de Boor's algorithm */
static Bool
render_point(
	double t,
	int degree, int n_points, int dimensions, double * v,
	double * knots, int * last_found_knot, Point * result, NPoint * nresult
) {
	double lo, hi;
	int l, i, n_knots = n_points + degree + 1, s, found = false;

	lo = knots[degree];
	hi = knots[n_knots - degree - 1];
	t = t * ( hi - lo ) + lo;

	/* find which pair of knots t belongs to */
	s = ( *last_found_knot < 0 ) ? degree : *last_found_knot;
	for ( ; s < n_points; s++ ) {
		if ( t >= knots[s] && t <= knots[s + 1] ) {
			*last_found_knot = s;
			found = true;
			break;
		}
	}
	if ( !found ) {
		warn("badly formed knot vector: outside curve definition");
		return false;
	}

	for ( l = 1; l <= degree + 1; l++) {
		for ( i = s; i > s - degree - 1 + l; i-- ) {
			int j, ix;
			double dkt, a, a_hat;
			dkt = knots[i + degree + 1 - l] - knots[i];
			if ( dkt == 0.0 ) {
				warn("badly formed knot vector: not increasing");
				return false;
			}
			a = ( t - knots[i] ) / dkt;
			a_hat = 1.0 - a;
			ix = i * 3;
      			/* interpolate each component */
			for ( j = 0; j < dimensions; j++, ix++)
				v[ix] = a_hat * v[ix - 3] + a * v[ix];
		}
	}

	/* convert back to cartesian and return */
	s *= 3;
	if ( dimensions == 3 ) {
		double f;
		f = v[s] / v[s+2];
		if ( result )
			result-> x = ( f < 0 ) ? (f - .5) : (f + .5);
		else
			nresult-> x = f;
		f = v[s+1] / v[s+2];
		if ( result )
			result-> y = ( f < 0 ) ? (f - .5) : (f + .5);
		else
			nresult-> y = f;
	} else if ( result ) {
		result-> x = (v[s] < 0) ? (v[s] - .5) : (v[s] + .5);
		result-> y = (v[s+1] < 0) ? (v[s+1] - .5) : (v[s+1] + .5);
	} else {
		nresult-> x = v[s];
		nresult-> y = v[s+1];
	}

	return true;
}

static int
tangent_detect( Point * a, Point * b)
{
	register int x = a->x - b->x;
	register int y = a->y - b->y;
	if ( x == 0 ) {
		if ( y ==  0) return 0;
		if ( y == -1) return 1;
		if ( y ==  1) return 2;
	} else if ( y == 0 ) {
		if ( x == -1) return 3;
		if ( x ==  1) return 4;
	} else if ( x < 2 && x > -2 && y < 2 && y > -2 )
		return (x * 10) + y;
	return -1;
}

static void
tangent_apply( int tangent,  Point * b)
{
	switch(tangent) {
	case 1:
		b->y++;
		break;
	case 2:
		b->y--;
		break;
	case 3:
		b->x++;
		break;
	case 4:
		b->x--;
		break;
	case 10 - 1:
		b->x--;	
		b->y++;	
		break;
	case 10 + 1:
		b->x--;	
		b->y--;	
		break;
	case - 10 - 1:
		b->x++;	
		b->y++;	
		break;
	case - 10 + 1:
		b->x++;	
		b->y--;	
		break;
	}
}

static Bool
cut_corner( int t2, int t1, Point * p2, Point * p1)
{
	switch (t2 * 10 + t1) {
	/*
	 >xxa    xx is tangent 3
  	    ab>  aa is tangent 1
	         bb is something not 1

        corner is detected when (aa) is exactly 1px high,
	and is cut so that (ab) becomes (b)
	*/
	case 13: case 14: return p2->y + 1 == p1->y;
	case 23: case 24: return p2->y - 1 == p1->y;
	case 31: case 32: return p2->x + 1 == p1->x;
	case 41: case 42: return p2->x - 1 == p1->x;
	}
	return false;
}

SV *
Drawable_render_spline( SV * obj, SV * points, HV * profile)
{
	dPROFILE;
	NPoint *p, *pp;
	Point *rendered, *storage;
	NPoint *nrendered, *nstorage;
	SV *ret;
	Bool ok, closed, as_integer;
	int i, j, degree, precision, n_points, final_size, k, dim, n_add_points, temp_size,
		tangent, last_tangent;
	double *knots, *weights, t, dt, *weighted, *temp;

	knots = weights = weighted = NULL;
	p = NULL;
	ret = NULL;
	ok = false;

	/* parse input */
	if ( pexist( degree )) {
		degree = pget_i(degree);
		if ( degree < 2 ) {
			warn("degree must be at least 2");
			goto EXIT;
		}
	} else
		degree = 2;

	if ( pexist( precision )) {
		precision = pget_i(precision);
		if ( precision < 2 || precision > 1024 ) {
			warn("precision must be at least 2 and max 1024");
			goto EXIT;
		}
	} else
		precision = 24;

	as_integer = pexist(integer) ? pget_B( integer ) : true;

	p = (NPoint*) prima_read_array( points, "Drawable::render_spline", 'd', 2, degree + 1, -1, &n_points, NULL);
	if ( !p) goto EXIT;

	/* closed curve will need at least one extra point and unclamped default knot set */
	if ( pexist( closed )) {
		SV * sv = pget_sv(closed);
		if ( SvTYPE(sv) == SVt_NULL ) goto DETECT_SHAPE;
		closed = SvTRUE(sv);
	}
	else {
	DETECT_SHAPE:
		closed = p[0].x == p[n_points-1].x && p[0].y == p[n_points-1].y;
	}
	n_add_points = closed ? degree - 1 : 0;
	n_points += n_add_points;

	if ( pexist( knots )) {
		knots = (double*) prima_read_array( pget_sv(knots), "knots", 'd', 1,
			n_points + degree + 1, n_points + degree + 1, NULL, NULL);
		if (!knots) goto EXIT;
	} else
		knots = default_knots(n_points, degree, !closed);

	if ( pexist( weights )) {
		weights = (double*) prima_read_array(pget_sv(weights), "weights", 'd', 1,
			n_points, n_points, NULL, NULL);
		if (!weights) goto EXIT;
		dim = 3;
	} else {
		weights = NULL; /* all ones */
		dim = 2;
	}

	/* allocate result storage */
	precision *= n_points - n_add_points;
	ret = prima_array_new(( precision + 1) * (as_integer ? sizeof(Point) : sizeof(NPoint)) );

	temp_size = sizeof(double) * 3 * n_points;
	if ( !(weighted = malloc( 2 * temp_size ))) {
		warn("not enough memory");
		goto EXIT;
	}

	/* convert to weighted points */
	for ( i = j = 0, pp = p; i < n_points; i++, pp++) {
		register double w = weights ? weights[i] : 1.0;
		weighted[j++] = pp->x * w;
		weighted[j++] = pp->y * w;
		weighted[j++] = w;
		if ( i == n_points - n_add_points - 1 ) pp = p;
	}
	temp = weighted + 3 * n_points;

	/* render */
	final_size = 0;
	if ( as_integer ) {
		nrendered = nstorage = NULL;
		rendered  = storage  = (Point*) prima_array_get_storage(ret);
	} else {
		rendered  = storage  = NULL;
		nrendered = nstorage = (NPoint*) prima_array_get_storage(ret);
	}
	k = -1;
	last_tangent = -1;
	for ( i = 0, t = 0.0, dt = 1.0 / precision; i < precision - 1; i++, t += dt) {
		memcpy( temp, weighted, temp_size);
		if (!render_point(t, degree, n_points, dim, temp, knots, &k, rendered, nrendered))
			goto EXIT;
		if ( as_integer && i > 0 ) {
			/* primitive line detection */
			tangent = tangent_detect( rendered-1, rendered);
			if ( tangent == 0 ) continue;
			if ( final_size > 1 && tangent > 0 && tangent == last_tangent) {
				tangent_apply( tangent, rendered-1);
				continue;
			} else if ( final_size > 1 && cut_corner(last_tangent, tangent, rendered-2, rendered-1)) {
			/* primitive corner detection - convert 4-connectivity into 8- */
				*(rendered-1) = *rendered;
				tangent = -1;
				rendered--;
				final_size--;
				continue;
			} else
				last_tangent = tangent;
		}
		final_size++;
		if ( as_integer )
			rendered++;
		else
			nrendered++;
	}
	memcpy( temp, weighted, temp_size);
	if ( !render_point(1.0, degree, n_points, dim, temp, knots, &k, rendered, nrendered))
		goto EXIT;
	final_size++;
	rendered++;
	nrendered++;

	/* looks good */
	ok = true;
	if ( closed ) {
		final_size++;
		if ( as_integer ) {
			*rendered = storage[0];
			rendered++;
		} else {
			*nrendered = nstorage[0];
			nrendered++;
		}
	}
	if ( final_size == 1 ) {
		final_size = 2;
		if ( as_integer )
			storage[1] = storage[0];
		else
			nstorage[1] = nstorage[0];
	}
	prima_array_truncate( ret, final_size * (as_integer ? sizeof( Point) : sizeof(NPoint)) );

EXIT:
	hv_clear(profile); /* old gencls bork */
	if (weighted) free(weighted);
	if (p)        free(p);
	if (knots)    free(knots);
	if (weights)  free(weights);
	if ( ok ) {
		return prima_array_tie( ret,
			as_integer ? sizeof(int) : sizeof(double),
			as_integer ? "i" : "d"
		);
	} else {
		if (ret)  sv_free(ret);
		return newRV_noinc(( SV *) newAV());
	}
}

SV *
Drawable_render_polyline( SV * obj, SV * points, HV * profile)
{
	dPROFILE;
	int count;
	Bool free_input = false, free_buffer = false, as_integer = false;
	double *input = NULL, *buffer = NULL, box[4];
	SV * ret;
	void * storage;

	if (( input = (double*) prima_read_array( points, "render_polyline", 'd', 2, 1, -1, &count, &free_input)) == NULL)
		goto FAIL;

	if ( pexist(integer)) as_integer = pget_B(integer);

	if ( pexist(matrix) ) {
		int i;
		double *src, *dst, *cmatrix;
		if (( cmatrix = (double*) prima_read_array(
			pget_sv(matrix),
			"render_polyline.matrix", 'd', 1, 6, 6, NULL, NULL)
		) == NULL) 
			goto FAIL;
		if ( !( buffer = malloc(sizeof(double) * 2 * count))) {
			free(cmatrix);
			warn("Not enough memory");
			goto FAIL;
		}
		free_buffer = true;

		for ( i = 0, src = input, dst = buffer; i < count; i++) {
			double x,y;
			x = *(src++);
			y = *(src++);
			*(dst++) = cmatrix[0] * x + cmatrix[2] * y + cmatrix[4];
			*(dst++) = cmatrix[1] * x + cmatrix[3] * y + cmatrix[5];
		}
		free(cmatrix);
	} else {
		buffer = input;
		free_buffer = false;
	}

	if ( pexist(box) && pget_B(box)) {
		int i;
		double *src;
		box[0] = box[2] = buffer[0];
		box[1] = box[3] = buffer[1];
		for ( i = 1, src = buffer + 2; i < count; i++) {
			double x,y;
			x = *(src++);
			y = *(src++);
			if ( box[0] > x ) box[0] = x;
			if ( box[1] > y ) box[1] = y;
			if ( box[2] < x ) box[2] = x;
			if ( box[3] < y ) box[3] = y;
		}
		box[2] -= box[0] - 1;
		box[3] -= box[1] - 1;
		if ( as_integer ) {
			box[0] = floor(box[0]);
			box[1] = floor(box[1]);
		}
		if ( free_buffer ) free(buffer);
		free_buffer = false;
		buffer = box;
		count  = 2;
	}

	ret = prima_array_new(count * 2 * (as_integer ? sizeof(int) : sizeof(double)));
	storage = prima_array_get_storage(ret);
	if ( as_integer ) {
		int i, *dst;
		double *src;
		for ( i = 0, src = buffer, dst = (int*)storage; i < count; i++) {
			register double x;
			x = *(src++);
			*(dst++) = x + ((x < 0) ? -.5 : +.5);
			x = *(src++);
			*(dst++) = x + ((x < 0) ? -.5 : +.5);
		}
	} else
		memcpy(storage, buffer, count * 2 * sizeof(double));

	if ( free_buffer ) free( buffer );
	if ( free_input ) free(input);
	hv_clear(profile); /* old gencls bork */

	return prima_array_tie( ret,
		as_integer ? sizeof(int) : sizeof(double),
		as_integer ? "i" : "d");

FAIL:
	if ( free_buffer ) free( buffer );
	if ( free_input ) free(input);
	hv_clear(profile); /* old gencls bork */
	return NULL_SV;
}

SV *
Drawable_render_glyph( Handle self, int index, HV * profile)
{
	int flags, *buffer, count;
	SV * ret;
	dPROFILE;
	gpARGS;
	CHECK_GP(NULL_SV);
	gpENTER(NULL_SV);

	flags = ggoUseHints;
	if ( pexist(glyph)   && pget_B(glyph))   flags |= ggoGlyphIndex;
	if ( pexist(hints)   && !pget_B(hints))  flags &= ~ggoUseHints;
	if ( pexist(unicode) && pget_B(unicode)) flags |= ggoUnicode;
	count = apc_gp_get_glyph_outline( self, index, flags, &buffer);
	hv_clear(profile); /* old gencls bork */
	gpLEAVE;

	if ( count < 0 ) return NULL_SV;
	ret = prima_array_new(sizeof(int) * count);
	memcpy( prima_array_get_storage(ret), buffer, sizeof(int) * count);
	if ( buffer ) free( buffer );
	return prima_array_tie( ret, sizeof(int), "i");
}


#ifdef __cplusplus
}
#endif
