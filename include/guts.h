#ifndef _GUTS_H_
#define _GUTS_H_

#ifndef _APRICOT_H_
#include "apricot.h"
#endif

extern Bool dolbug;
extern Bool waitBeforeQuit;

#define dPUB_ARGS    int rc = recursiveCall
#define PUB_CHECK    rc = recursiveCall

extern long   apcError;
extern List   postDestroys;
extern int    recursiveCall;
extern PHash  primaObjects;

extern Bool window_subsystem_init( void);
extern void window_subsystem_cleanup( void);
extern void window_subsystem_done( void);
extern void build_static_vmt( void *vmt);
extern void kill_zombies( void);
extern void init_image_support( void);
extern void prima_init_image_subsystem( void);
extern void prima_cleanup_image_subsystem( void);

extern Handle  gimme_the_real_mate( SV *perlObject);

/* kernel exports */
extern XS( Component_set_notification_FROMPERL);

extern PRGBColor read_palette( int * palSize, SV * palette);
extern Bool read_point( AV * av, int * pt, int number, char * error);
extern Bool accel_notify ( Handle group, Handle self, PEvent event);
extern Bool font_notify ( Handle self, Handle child, void * font);
extern Bool find_accel( Handle self, Handle item, int * key);
extern Bool single_color_notify ( Handle self, Handle child, void * color);
extern Bool kill_all( Handle self, Handle child, void * dummy);


#endif
