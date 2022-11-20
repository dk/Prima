#include <apricot.h>
#include <guts.h>

#ifdef HAVE_OPENMP
#include <omp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int
prima_omp_max_threads(void)
{
	return
#ifdef HAVE_OPENMP
		omp_get_max_threads()
#else
		1
#endif
	;
}

int
prima_omp_thread_num(void)
{
	return
#ifdef HAVE_OPENMP
		omp_get_thread_num()
#else
		0
#endif
	;
}

void
prima_omp_set_num_threads(int num)
{
#ifdef HAVE_OPENMP
	omp_set_num_threads(num);
#endif
}

#ifdef __cplusplus
}
#endif
