#ifndef _GUTS_H_
#define _GUTS_H_

#ifndef _APRICOT_H_
#include "apricot.h"
#endif

extern Bool loggerDead;
extern Bool waitBeforeQuit;

#define dmopt_set( option)           (((PComponent)self)->delegatedMessages. option = 1)
#define dmopt_clear( option)         (((PComponent)self)->delegatedMessages. option = 0)
#define is_dmopt( option)            (((PComponent)self)->delegatedMessages. option)
#define dmopt_assign( option, value) (((PComponent)self)->delegatedMessages. option = (value)?1:0)

extern Bool window_subsystem_init( void);
extern void window_subsystem_cleanup( void);
extern void window_subsystem_done( void);
extern void build_static_vmt( void *vmt);
extern void kill_zombies( void);
extern void init_image_support( void);

#endif
