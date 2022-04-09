#ifndef _GUTS_H_
#define _GUTS_H_

#ifndef _APRICOT_H_
#include "apricot.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define dPUB_ARGS    int rc = recursiveCall
#define PUB_CHECK    rc = recursiveCall

#define dG_EVAL_ARGS SV * errSave = NULL
#define OPEN_G_EVAL \
errSave = SvTRUE( GvSV( PL_errgv)) ? newSVsv( GvSV( PL_errgv)) : NULL;\
sv_setsv( GvSV( PL_errgv), NULL_SV)
#define CLOSE_G_EVAL \
if ( errSave) sv_catsv( GvSV( PL_errgv), errSave);\
if ( errSave) sv_free( errSave)

typedef struct {
	Handle     application;
	List       post_destroys;
	int        recursive_call;
	PHash      objects;
	SV *       event_hook;
	Bool       use_fribidi;
	int        use_libthai;
	List       static_hashes;
	int        init_ok;
	PHash      vmt_hash;
	List       static_objects;
	PAnyObject kill_chain;
	PAnyObject ghost_chain;
	Bool       app_is_dead;
} PrimaGuts, *PPrimaGuts;

#define P_APPLICATION PApplication(prima_guts.application)
#define C_APPLICATION CApplication(prima_guts.application)

extern PrimaGuts prima_guts;

extern PPrimaGuts prima_api_guts(void);

#define CORE_INIT_TRANSIENT(cls) ((PObject)self)->transient_class = (void*)C##cls

extern Bool window_subsystem_init( char * error_buf);
extern Bool window_subsystem_set_option( char * option, char * value);
extern Bool window_subsystem_get_options( int * argc, char *** argv);
extern void window_subsystem_cleanup( void);
extern void window_subsystem_done( void);
extern void prima_init_image_subsystem( void);
extern void prima_cleanup_image_subsystem( void);

/* kernel exports */
XS( Prima_options);
XS( Prima_dl_export);
XS( Prima_message_FROMPERL);
XS( create_from_Perl);
XS( destroy_from_Perl);
XS( Object_alive_FROMPERL);
XS( Component_set_notification_FROMPERL);
XS( Component_event_hook_FROMPERL);
XS(Utils_getdir_FROMPERL);
XS(Utils_stat_FROMPERL);
XS(Utils_closedir_FROMPERL);

extern PRGBColor prima_read_palette( int * palSize, SV * palette);
extern Bool prima_read_point( SV *rvav, int * pt, int number, char * error);
extern Bool prima_accel_notify ( Handle group, Handle self, PEvent event);
extern Bool prima_font_notify ( Handle self, Handle child, void * font);
extern Bool prima_find_accel( Handle self, Handle item, int * key);
extern Bool prima_single_color_notify ( Handle self, Handle child, void * color);

extern void  prima_init_font_mapper(void);
extern void  prima_cleanup_font_mapper(void);
extern PFont prima_font_mapper_save_font(const char * name, unsigned int style);
extern PFont prima_font_mapper_get_font(unsigned int fid );

#define pfmaGetCount      0
#define pfmaIsActive      1
#define pfmaPassivate     2
#define pfmaActivate      3
#define pfmaIsEnabled     4
#define pfmaEnable        5
#define pfmaDisable       6
#define pfmaGetIndex      7
extern int   prima_font_mapper_action(int action, PFont font);

extern void
prima_register_notifications( PVMT vmt);

/* OpenMP support */
extern int  prima_omp_max_threads(void);
extern int  prima_omp_thread_num(void);
extern void prima_omp_set_num_threads(int num);

#ifdef HAVE_OPENMP
#define OMP_MAX_THREADS prima_omp_max_threads()
#define OMP_THREAD_NUM prima_omp_thread_num()
#else
#define OMP_MAX_THREADS 1
#define OMP_THREAD_NUM  0
#endif

typedef struct {
	void *stack, *heap;
	unsigned int elem_size, count, size;
} semistatic_t;

extern void
semistatic_init( semistatic_t * s, void * stack, unsigned int elem_size, unsigned int static_size);

extern int
semistatic_expand( semistatic_t * s, unsigned int desired_elems);

extern void
semistatic_done( semistatic_t * s);

#define semistatic_at(s,type,i) (((type*)s.heap)[i])

#define semistatic_push(s,type,v) \
	((( s.count >= s.size ) ? semistatic_expand(&s,-1) : 1) && \
		(((((type*)s.heap)[s.count++])=v) || 1))


#ifdef __cplusplus
}
#endif


#endif
