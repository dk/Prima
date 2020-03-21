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

#define dG_EVAL_ARGS SV * errSave = nil
#define OPEN_G_EVAL \
errSave = SvTRUE( GvSV( PL_errgv)) ? newSVsv( GvSV( PL_errgv)) : nil;\
sv_setsv( GvSV( PL_errgv), nilSV)
#define CLOSE_G_EVAL \
if ( errSave) sv_catsv( GvSV( PL_errgv), errSave);\
if ( errSave) sv_free( errSave)

extern long   apcError;
extern List   postDestroys;
extern int    recursiveCall;
extern PHash  primaObjects;
extern SV *   eventHook;

#define CORE_INIT_TRANSIENT(cls) ((PObject)self)->transient_class = (void*)C##cls

extern Bool window_subsystem_init( char * error_buf);
extern Bool window_subsystem_set_option( char * option, char * value);
extern Bool window_subsystem_get_options( int * argc, char *** argv);
extern void window_subsystem_cleanup( void);
extern void window_subsystem_done( void);
extern void prima_init_image_subsystem( void);
extern void prima_cleanup_image_subsystem( void);

/* kernel exports */
extern XS( Component_set_notification_FROMPERL);

extern PRGBColor prima_read_palette( int * palSize, SV * palette);
extern Bool prima_read_point( SV *rvav, int * pt, int number, char * error);
extern Bool prima_accel_notify ( Handle group, Handle self, PEvent event);
extern Bool prima_font_notify ( Handle self, Handle child, void * font);
extern Bool prima_find_accel( Handle self, Handle item, int * key);
extern Bool prima_single_color_notify ( Handle self, Handle child, void * color);
extern Bool prima_kill_all_objects( Handle self, Handle child, void * dummy);

#ifdef __cplusplus
}
#endif


#endif
