#ifndef _GUTS_H_
#define _GUTS_H_

#ifndef _APRICOT_H_
#include "apricot.h"
#endif

extern Bool loggerDead;
extern Bool waitBeforeQuit;

extern Bool window_subsystem_init( void);
extern void window_subsystem_cleanup( void);
extern void window_subsystem_done( void);
extern void build_static_vmt( void *vmt);
extern void kill_zombies( void);
extern void init_image_support( void);

#endif
