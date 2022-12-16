#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

void
prima_matrix_apply( Matrix *matrix, double *x, double *y)
{
	double xx = (*matrix)[0] * (*x) + (*matrix)[2] * (*y) + (*matrix)[4];
	double yy = (*matrix)[1] * (*x) + (*matrix)[3] * (*y) + (*matrix)[5];
	*x = xx;
	*y = yy;
}

Point
prima_matrix_apply_to_int( Matrix *matrix, double x, double y)
{
	Point p;
	p.x = floor( (*matrix)[0] * x + (*matrix)[2] * y + (*matrix)[4] + .5 );
	p.y = floor( (*matrix)[1] * x + (*matrix)[3] * y + (*matrix)[5] + .5 );
	return p;
}

void
prima_matrix_apply_int_to_int( Matrix *matrix, int *x, int *y)
{
	Point p;
	p.x = floor( (*matrix)[0] * *x + (*matrix)[2] * *y + (*matrix)[4] + .5 );
	p.y = floor( (*matrix)[1] * *x + (*matrix)[3] * *y + (*matrix)[5] + .5 );
	*x = p.x;
	*y = p.y;
}

void
prima_matrix_apply2( Matrix *matrix, NPoint *src, NPoint *dst, int n)
{
	int i;
	for ( i = 0; i < n; i++) {
		register double xx = (*matrix)[0] * (*src).x + (*matrix)[2] * (*src).y + (*matrix)[4];
		register double yy = (*matrix)[1] * (*src).x + (*matrix)[3] * (*src).y + (*matrix)[5];
		(*dst).x = xx;
		(*dst).y = yy;
		src++;
		dst++;
	}
}

void
prima_matrix_apply2_to_int( Matrix *matrix, NPoint *src, Point *dst, int n)
{
	int i;
	for ( i = 0; i < n; i++) {
		register double xx = (*matrix)[0] * (*src).x + (*matrix)[2] * (*src).y + (*matrix)[4];
		register double yy = (*matrix)[1] * (*src).x + (*matrix)[3] * (*src).y + (*matrix)[5];
		(*dst).x = floor( xx + .5 );
		(*dst).y = floor( yy + .5 );
		src++;
		dst++;
	}
}

void
prima_matrix_apply2_int_to_int( Matrix *matrix, Point *src, Point *dst, int n)
{
	int i;
	for ( i = 0; i < n; i++) {
		register double xx = (*matrix)[0] * (*src).x + (*matrix)[2] * (*src).y + (*matrix)[4];
		register double yy = (*matrix)[1] * (*src).x + (*matrix)[3] * (*src).y + (*matrix)[5];
		(*dst).x = floor( xx + .5 );
		(*dst).y = floor( yy + .5 );
		src++;
		dst++;
	}
}

void
prima_matrix_set_identity( Matrix *m)
{
	bzero(*m, sizeof(Matrix));
	(*m)[0] = (*m)[3] = 1.0;
}

Bool
prima_matrix_is_identity( Matrix *m)
{
	Matrix identity = {1.0,0.0,0.0,1.0,0.0,0.0};
	return memcmp(m, identity, sizeof(Matrix)) == 0;
}

Bool
prima_matrix_is_translated_only( Matrix *m)
{
	double translate_only[4] = {1.0,0.0,0.0,1.0};
	return memcmp(m, translate_only, sizeof(translate_only)) == 0;
}


Point*
prima_matrix_transform_to_int( Matrix *matrix, NPoint *src, Bool src_is_modifiable, int n_points)
{
	unsigned char mbuf[256];
	semistatic_t local_points;
	NPoint *transformed_src;
	Bool free_local_points = false;
	int i;
	Point *ptr, *result;

	transformed_src = src;
	if ( !prima_matrix_is_identity(matrix)) {
		if ( !src_is_modifiable ) {
			semistatic_init(&local_points, &mbuf, sizeof(NPoint), 256);
			if ( !semistatic_expand(&local_points, n_points)) {
				warn("Not enough memory");
				return NULL;
			}
			memcpy(local_points.heap, src, sizeof(NPoint) * n_points);
			free_local_points = true;
			transformed_src = (NPoint*)local_points.heap;
		}
		prima_matrix_apply2( matrix, transformed_src, transformed_src, n_points);
	}

	if ( !( result = malloc(sizeof(Point) * n_points ))) {
		if (free_local_points)
			semistatic_done(&local_points);
		warn("Not enough memory");
		return NULL;
	}

	for ( i = 0, ptr = result; i < n_points; i++, ptr++, transformed_src++) {
		(*ptr).x = floor( (*transformed_src).x + .5 );
		(*ptr).y = floor( (*transformed_src).y + .5 );
	}

	if (free_local_points)
		semistatic_done(&local_points);
	return result;
}

Bool
prima_matrix_is_square_rectangular( Matrix *matrix, NRect *src_dest_rect, NPoint *dest_polygon)
{
	NPoint *p = dest_polygon;
	p[0].x = p[3].x = src_dest_rect->left;
	p[0].y = p[1].y = src_dest_rect->bottom;
	p[1].x = p[2].x = src_dest_rect->right;
	p[2].y = p[3].y = src_dest_rect->top;
	prima_matrix_apply2( matrix, p, p, 4);
	if (p[0].x == p[3].x && p[0].y == p[1].y && p[1].x == p[2].x && p[2].y == p[3].y) {
		src_dest_rect->left   = p[0].x;
		src_dest_rect->bottom = p[0].y;
		src_dest_rect->right  = p[1].x;
		src_dest_rect->top    = p[2].y;
	} else if (p[0].x == p[1].x && p[0].y == p[3].y && p[3].x == p[2].x && p[1].y == p[2].y) {
		src_dest_rect->left   = p[0].x;
		src_dest_rect->bottom = p[0].y;
		src_dest_rect->right  = p[2].x;
		src_dest_rect->top    = p[1].y;
	} else {
		return false;
	}

	if ( src_dest_rect->left > src_dest_rect->right ) {
		double x = src_dest_rect->left;
		src_dest_rect->left = src_dest_rect->right;
		src_dest_rect->right = x;
	}

	if ( src_dest_rect->bottom > src_dest_rect->top ) {
		double y = src_dest_rect->bottom;
		src_dest_rect->bottom = src_dest_rect->top;
		src_dest_rect->top = y;
	}

	return true;
}

void
prima_matrix_multiply( Matrix *m1, Matrix *m2, Matrix *result)
{
	(*result)[0] = (*m1)[0] * (*m2)[0] + (*m1)[1] * (*m2)[2];
	(*result)[1] = (*m1)[0] * (*m2)[1] + (*m1)[1] * (*m2)[3];
	(*result)[2] = (*m1)[2] * (*m2)[0] + (*m1)[3] * (*m2)[2];
	(*result)[3] = (*m1)[2] * (*m2)[1] + (*m1)[3] * (*m2)[3];
	(*result)[4] = (*m1)[4] * (*m2)[0] + (*m1)[5] * (*m2)[2] + (*m2)[4];
	(*result)[5] = (*m1)[4] * (*m2)[1] + (*m1)[5] * (*m2)[3] + (*m2)[5];
}



#ifdef __cplusplus
}
#endif
