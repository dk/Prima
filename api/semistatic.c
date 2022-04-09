#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

void
semistatic_init( semistatic_t * s, void * stack, unsigned int elem_size, unsigned int static_size)
{
	bzero(s, sizeof(semistatic_t));
	s->stack = s->heap = stack;
	s->elem_size = elem_size;
	s->size      = static_size;
}

int
semistatic_expand( semistatic_t * s, unsigned int desired_elems )
{
	void * n;

	if ( desired_elems > 0 ) {
		if ( s-> size >= desired_elems )
			return 1;
		s-> size = desired_elems;
	} else {
		s-> size *= 2;
	}

	if ( s->stack == s->heap ) {
		if (( n = malloc(s->elem_size * s->size)) == NULL)
			goto FAIL;
		memcpy( n, s->stack, s->elem_size * s->count);
	} else {
		if (( n = realloc(s->heap, s->elem_size * s->size)) == NULL )
			goto FAIL;
	}
	s-> heap = n;
	return 1;
FAIL:
	warn("not enough memory");
	return 0;
}

void
semistatic_done( semistatic_t * s)
{
	if ( s->stack != s->heap )
		free(s->heap);
}

#ifdef __cplusplus
}
#endif
