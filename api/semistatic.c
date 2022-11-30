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
	if (static_size > 0) {
		bzero(stack, (size_t) (elem_size * static_size));
	}
}

int
semistatic_expand( semistatic_t * s, unsigned int desired_elems )
{
	void * n;
	char * p;
	size_t prevbytesize = (size_t) (s->elem_size * s->count);
	size_t newbytesize;

	if ( desired_elems > 0 ) {
		if ( s-> size >= desired_elems )
			return 1;
		s-> size = desired_elems;
	} else {
		s-> size *= 2;
	}

	newbytesize = (size_t) (s->elem_size * s->size);
	if ( s->stack == s->heap ) {
		if (( n = malloc(newbytesize)) == NULL)
			goto FAIL;
		memcpy( n, s->stack, prevbytesize);
	} else {
		if (( n = realloc(s->heap, newbytesize)) == NULL )
			goto FAIL;
	}
	if (newbytesize > prevbytesize) {
		p = (char *) n;
		bzero(p + prevbytesize, newbytesize - prevbytesize);
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
