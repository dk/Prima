#include "apricot.h"
#include "guts.h"
#include "img_conv.h"
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


#define PI 3.14159265358979323846264338327950288419716939937510
#define PI_2 (PI / 2)
#define PI_4 (PI / 4)
#define RAD (180.0 / PI)
#define SQRT_2 1.4142135623731

#define CMD_LINE  (Handle) 0
#define CMD_ARC   (Handle) 1

#define NEW_CMD(cmd)    av_push(path, newSVpv(cmd,0));
#define ADD_SV(sv)      av_push(path, sv)
#define ADD_AV          av_push(path, newRV_noinc((SV*) av))

#define ADD_ARC(x,y,dx,dy,as,ae) { \
	av = newAV();              \
	av_push(av, newSVnv(x));   \
	av_push(av, newSVnv(y));   \
	av_push(av, newSVnv(dx));  \
	av_push(av, newSVnv(dy));  \
	av_push(av, newSVnv(as));  \
	av_push(av, newSVnv(ae));  \
}

static void
collide_commands(void * heap, AV * path, PList up, PList down, Bool integer_precision)
{
	SV * sv;
	int i, j, n, n_up_cmd, m;
	int datum_size;
	char * datum_type;

	datum_size   = integer_precision ? sizeof(int) : sizeof(double);
	datum_type   = integer_precision ? "i" : "d";

/* direct order from up and reverse order from down */
#define ITEM(x)   (( x < n_up_cmd ) ? up->items[x << 1]       : down->items[down->count - ((x - n_up_cmd) << 1) - 2])
#define PARAM(x)  (( x < n_up_cmd ) ? up->items[(x << 1) + 1] : down->items[down->count - ((x - n_up_cmd) << 1) - 1])
	n = (up->count + down->count) / 2;
	n_up_cmd = up->count / 2;
	for ( i = 0; i < n; i++) {
		if ( ITEM(i) == CMD_ARC ) {
			NEW_CMD("arc");
			ADD_SV( newRV_noinc((SV*) PARAM(i)));
			continue;
		}
		for ( j = i, m = 0; j < n; j++ ) {
			if ( ITEM(j) != CMD_LINE ) break;
			m++;
		}
		sv = prima_array_new(datum_size * m * 2);
		if ( integer_precision ) {
			int *dest = (int*) prima_array_get_storage(sv);
			for ( j = i; j < i + m; j++ ) {
				int *src = ((int*)(heap)) + (int) PARAM(j);
				*(dest++) = *(src++);
				*(dest++) = *(src++);
			}
		} else {
			double *dest = (double*) prima_array_get_storage(sv);
			for ( j = i; j < i + m; j++ ) {
				double *src = ((double*)(heap)) + (int) PARAM(j);
				*(dest++) = *(src++);
				*(dest++) = *(src++);
			}
		}

		NEW_CMD("line");
		ADD_SV( prima_array_tie( sv, datum_size, datum_type));
		i += m;
	}
#undef ITEM
#undef PARAM
}

static Bool
widen_line(AV * path, NPolyPolyline *poly, DrawablePaintState *state, Bool integer_precision)
{
	/* widen the line */
	semistatic_t lines;
	Byte line_storage[16384];
	double lw2 = state-> line_width / 2;
	NPolyPolyline* p;
	List up, down; /* store either full av-formed commands or pointers to line_storage here */
	int n_points, datum_size;
	char *datum_type;
	Bool ok = false;

	datum_size  = integer_precision ? sizeof(int) : sizeof(double);
	datum_type  = integer_precision ? "i" : "d";

	p = poly;
	n_points = 0;
	while (p) {
		n_points += p->n_points;
		p = p->next;
	}
	list_create( &up,   n_points, n_points );
	list_create( &down, n_points, n_points );

	semistatic_init(&lines, &line_storage, datum_size, sizeof(line_storage) / datum_size);

	p = poly;
	while (p) {
		Bool closed, no_line_ends;
		int i, last, firstsign = 0;
		NPoint firstin = p->points[0], firstout = p->points[0]; /* no need, just to hush warnings */

		if ( p-> n_points == 0 ) goto NEXT;
		no_line_ends = state->line_width < 2.5 && integer_precision;
		closed =
			(p->points[0].x == p->points[p->n_points-1].x) &&
			(p->points[0].y == p->points[p->n_points-1].y);
		last = p->n_points - (closed ? 2 : 1);


#define TEMP_ADD_VAL(v)                                                            \
	if ( integer_precision ) {                                                 \
		semistatic_push(lines, int, floor(v + .5));                        \
	} else {                                                                   \
		semistatic_push(lines, double, v);                                 \
	}

#define TEMP_ADD_POINT(list,x,y) {                                                 \
	if ( list_add(list,CMD_LINE) < 0)              goto FAIL;                  \
	if ( !semistatic_expand(&lines,lines.count+2)) goto FAIL;                  \
	if ( list_add(list, (Handle)lines.count) < 0)  goto FAIL;                  \
	TEMP_ADD_VAL(x);                                                           \
	TEMP_ADD_VAL(y);                                                           \
}

#define TEMP_ADD_ARC(list,x,y,d,as,ae) {                                           \
	AV *av;                                                                    \
	if ( list_add(list,CMD_ARC) < 0) goto FAIL;                                \
	ADD_ARC(x,y,d,d,as,ae);                                                    \
	if ( list_add(list,(Handle) av) < 0) {                                     \
		av_undef(av);                                                      \
		goto FAIL;                                                         \
	}                                                                          \
}

		if ( last == 0 ) {
			/* single pixel line, draw a rectangle or ellipse */
			NPoint f = p->points[0];
			if ( state->line_end == leSquare || no_line_ends ) {
				SV * sv;
				double rect[8] = {
					f.x - lw2, f.y - lw2,
					f.x - lw2, f.y + lw2,
					f.x + lw2, f.y + lw2,
					f.x + lw2, f.y - lw2
				};
				sv = prima_array_new(datum_size * 8);
				prima_array_convert( 8, rect, 'd', prima_array_get_storage(sv), *datum_type);

				NEW_CMD("line");
				ADD_SV( prima_array_tie( sv, datum_size, datum_type));
			} else if ( state->line_end == leRound ) {
				AV * av;
				NEW_CMD("arc");
				ADD_ARC(f.x, f.y, state->line_width, state->line_width, 0.0, 360.0);
				ADD_AV;
			}
			goto NEXT;
		}

		for ( i = 0; i <= last; i++) {
			/* end points */
			if ( !closed && ( i == 0 || i == last )) {
				NPoint o = p->points[i], a = p->points[i ? last - 1 : 1];
				double theta = atan2( a.y - o.y, a.x - o.x );
				if ( state-> line_end == leFlat || no_line_ends ) {
					double s = sin(theta + PI_2), c = cos(theta + PI_2);
					TEMP_ADD_POINT( &up, o.x + lw2 * c, o.y + lw2 * s);
					TEMP_ADD_POINT( &up, o.x - lw2 * c, o.y - lw2 * s);
				} else if ( state->line_end == leSquare ) {
					TEMP_ADD_POINT( &up, o.x - SQRT_2 * cos(theta - PI_4), o.y - SQRT_2 * sin(theta - PI_4));
					TEMP_ADD_POINT( &up, o.x - SQRT_2 * cos(theta + PI_4), o.y - SQRT_2 * sin(theta + PI_4));
				} else {
					TEMP_ADD_ARC( &up,
						o.x, o.y, state->line_width,
						RAD * (theta + PI_2), RAD * (theta + 3 * PI_2)
					);
				}
			} else {
				/* normal points */
				int prev, next, sign, lj;
				NPoint o, a, b, da, db, d1, d2;
				double theta, alpha;
				List *in, *out;
				if ( i > 0 && i < last ) {
					prev = i - 1;
					next = i + 1;
				} else if ( i == 0 ) {
					prev = last;
					next = i + 1;
				} else {
					prev = i - 1;
					next = 0;
				}
				o = p->points[i];
				a = p->points[prev];
				b = p->points[next];
				da.x = o.x - a.x;
				da.y = o.y - a.y;
				db.x = b.x - o.x;
				db.y = b.y - o.y;
				theta = atan2( da.y, da.x );
				alpha = atan2( db.y, db.x ) - theta;
				alpha += PI * (( alpha > 0 ) ? -1 : 1);
				/* XXX if ( alpha == 0.0 ) continue */
				sign  = (alpha > 0) ? -1 : 1;
				if ( alpha > 0 ) {
					in  = &up;
					out = &down;
				} else {
					in  = &down;
					out = &up;
				}
				d1.x = sign * lw2 * cos(theta + PI_2);
				d1.y = sign * lw2 * sin(theta + PI_2);
				d2.x = sign * lw2 * cos(theta + alpha + PI_2);
				d2.y = sign * lw2 * sin(theta + alpha + PI_2);

				lj = state-> line_join;
#define DMIN 3
				if (
					lj != ljMiter &&
					fabs(da.x) < DMIN &&
					fabs(da.y) < DMIN &&
					fabs(db.x) < DMIN &&
					fabs(db.y) < DMIN
				)
					lj = ljMiter;
				if ( lj == ljMiter && (
					alpha == 0.0 ||
					( state-> miter_limit < fabs( 1.0 / sin( alpha / 2 )))
				))
					lj = ljBevel;
#undef DMIN
				if ( i == 0 ) {
					firstin.x = o.x + d1.x;
					firstin.y = o.y + d1.y;
					firstsign = sign;
				}
				if ( da.x == 0.0 && da.y == 0.0 ) goto NEXT;
				if ( db.x == 0.0 && db.y == 0.0 ) goto NEXT;
				TEMP_ADD_POINT( in, o.x + d1.x, o.y + d1.y );
				TEMP_ADD_POINT( in, o.x - d2.x, o.y - d2.y );
				if ( lj == ljMiter ) {
					double miter_width = fabs(lw2 / cos(PI_2 - fabs(alpha) / 2));
					NPoint apex = {
						o.x + miter_width * cos(theta + alpha / 2),
						o.y + miter_width * sin(theta + alpha / 2)
					};
					TEMP_ADD_POINT(out, apex.x, apex.y);
					if ( i == 0 ) firstout = apex;
				} else if ( lj == ljBevel || no_line_ends ) {
					if ( i == 0 ) {
						firstout.x = o.x - d1.x;
						firstout.y = o.y - d1.y;
					}
					TEMP_ADD_POINT( out, o.x - d1.x, o.y - d1.y );
					TEMP_ADD_POINT( out, o.x + d2.x, o.y + d2.y );
				} else {
					NPoint arc;
					if ( i == 0 ) {
						firstout.x = o.x - d1.x;
						firstout.y = o.y - d1.y;
					}
					if ( alpha > 0 ) {
						arc.x = theta + alpha - PI_2;
						arc.y = theta + PI_2;
					} else {
						arc.x = theta - PI_2;
						arc.y = theta + alpha - PI_2;
					}
					TEMP_ADD_ARC(out, o.x, o.y, state->line_width, RAD * arc.x, RAD * arc.y );
				}

				if ( i == last ) {
					if ( sign != firstsign ) {
						NPoint f = firstin;
						firstin = firstout;
						firstout = f;
					}
					TEMP_ADD_POINT(in,  firstin.x, firstin.y);
					TEMP_ADD_POINT(out, firstout.x,firstout.y);
				}
			}
		}

	NEXT:
		collide_commands(lines.heap, path, &up, &down, integer_precision);
		NEW_CMD("open");
		up.count = down.count = 0;
		p = p->next;
	}
	ok = true;

FAIL:
	if ( !ok ) {
		int i;
		for ( i = 0; i < up.count; i+=2)
			if (up.items[i] == CMD_ARC)
				av_undef((AV*) up.items[i+1]);
		for ( i = 0; i < down.count; i+=2)
			if (down.items[i] == CMD_ARC)
				av_undef((AV*) down.items[i+1]);
	}
	semistatic_done(&lines);
	list_destroy( &down );
	list_destroy( &up );
	return ok;
}

#undef NEW_CMD
#undef ADD_SV
#undef ADD_ARC
#undef TEMP_ADD_VAL
#undef TEMP_ADD_POINT
#undef TEMP_ADD_ARC

/* produce a closed shape so eventual fill() won't try to autoclose it */
static SV*
render_line2fill(NPoint *buffer, int count, Bool as_integer)
{
	SV *sv;
	void * storage;

	sv = prima_array_new(count * 4 * (as_integer ? sizeof(int) : sizeof(double)));
	storage = prima_array_get_storage(sv);
	prima_array_convert(count * 2, buffer, 'd', storage, as_integer ? 'i' : 'd');

#define REVCPY(type)                                                  \
	type *src = storage, *dst = ((type*)storage) + count * 4 - 1; \
	while (src < dst) {                                           \
		register type x = *(src++);                           \
		*(dst--) = *(src++);                                  \
		*(dst--) = x;                                         \
	}
	if ( as_integer ) {
		REVCPY(int);
	} else {
		REVCPY(double);
	}
#undef REVCPY

	return prima_array_tie( sv,
		as_integer ? sizeof(int) : sizeof(double),
		as_integer ? "i" : "d");
}

static SV *
render_wide_line( NPoint *points, unsigned int n_points, DrawablePaintState *state, unsigned char * line_pattern, Bool integer_precision )
{
	AV* path;
	NPolyPolyline* poly = NULL, static_poly;
	Bool ok = false;

	if (
		state-> line_width < 0.0 ||
		state-> miter_limit < 0.0 ||
		state-> line_end < 0 || state-> line_end > leMax ||
		state-> line_join < 0 || state-> line_join > ljMax
	)
		return NULL;

	if (
		(strcmp((char*) line_pattern, (char*) lpSolid) == 0) &&
		integer_precision && state->line_width <= 1.5
	)
		return NULL;

	if ( !( poly = img_polyline2patterns(points, n_points, state->line_width, line_pattern, integer_precision))) {
		poly = &static_poly;
		poly-> next     = NULL;
		poly-> prev     = NULL;
		poly-> n_points = n_points;
		poly-> points   = points;
	}

	path = newAV();

	if (integer_precision && state->line_width <= 1.5) {
		/* no line widening, return as is */
		NPolyPolyline* p = poly;
		while (p) {
			av_push( path, newSVpv("line", 0));
			av_push( path, render_line2fill( p->points, p->n_points, integer_precision));
			av_push( path, newSVpv("open", 0));
			p = p->next;
		}
		ok = true;
	} else
		ok = widen_line( path, poly, state, integer_precision );

	if ( poly != &static_poly ) 
		while (poly) {
			NPolyPolyline* p = poly->next;
			free(poly);
			poly = p;
		}

	if ( ok ) return newRV_noinc((SV*) path);

	av_undef( path );
	return NULL;
}

SV *
Drawable_render_polyline( SV * obj, SV * points, HV * profile)
{
	dPROFILE;
	int count;
	Bool free_input = false, free_buffer = false, as_integer = false;
	double *input = NULL, *buffer = NULL, box[4];
	SV * ret = NULL_SV;
	void * storage;

	if (( input = (double*) prima_read_array( points, "render_polyline", 'd', 2, 1, -1, &count, &free_input)) == NULL)
		goto EXIT;

	if ( pexist(integer)) as_integer = pget_B(integer);

	if ( pexist(matrix) ) {
		double *cmatrix;
		if (( cmatrix = (double*) prima_read_array(
			pget_sv(matrix),
			"render_polyline.matrix", 'd', 1, 6, 6, NULL, NULL)
		) == NULL) 
			goto EXIT;
		if ( !( buffer = malloc(sizeof(double) * 2 * count))) {
			free(cmatrix);
			warn("Not enough memory");
			goto EXIT;
		}
		free_buffer = true;
		prima_matrix_apply2( cmatrix, (NPoint*)input, (NPoint*)buffer, count);
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
	} else if (pexist(path) && pget_B(path)) {
		Handle self;
		DrawablePaintState state;
		unsigned char * line_pattern;

		self = gimme_the_mate(obj);
		if ( self ) {
			state = var->current_state;
		} else {
			state.line_width   = 1.0;
			state.miter_limit  = 10.0;
			state.line_end     = leSquare;
			state.line_join    = ljMiter;
		}
		line_pattern = lpSolid; 

		if ( pexist(lineWidth))
			state.line_width = pget_f(lineWidth);
		if ( pexist(miterLimit))
			state.miter_limit = pget_f(miterLimit);
		if ( pexist(lineEnd))
			state.line_end = pget_i(lineEnd);
		if ( pexist(lineJoin))
			state.line_join = pget_i(lineJoin);
		if ( pexist(linePattern))
			line_pattern = (unsigned char*) pget_c(linePattern);
		else if (self != NULL_HANDLE) {
			SV * lp = my->get_linePattern(self);
			if ( lp && lp != NULL_SV) line_pattern = (unsigned char*) SvPV_nolen(lp);
		}

		ret = render_wide_line(( NPoint*) buffer, count, &state, line_pattern, as_integer );
		if ( ret == NULL ) {
			AV * av = newAV();
			av_push(av, newSVpv("line", 0));
			av_push(av, render_line2fill((NPoint*) buffer, count, as_integer));
			ret = newRV_noinc((SV*) av);
		}
		goto EXIT;
	}

	ret = prima_array_new(count * 2 * (as_integer ? sizeof(int) : sizeof(double)));
	storage = prima_array_get_storage(ret);
	prima_array_convert(count * 2, buffer, 'd', storage, as_integer ? 'i' : 'd');

	if ( free_buffer ) free( buffer );
	if ( free_input ) free(input);
	hv_clear(profile); /* old gencls bork */

	return prima_array_tie( ret,
		as_integer ? sizeof(int) : sizeof(double),
		as_integer ? "i" : "d");

EXIT:
	if ( free_buffer ) free( buffer );
	if ( free_input ) free(input);
	hv_clear(profile); /* old gencls bork */
	return ret;
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
