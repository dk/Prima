#ifndef _GUTS_H_
#define _GUTS_H_

#ifndef _APRICOT_H_
#include "apricot.h"
#endif

extern Bool dolbug;
extern Bool waitBeforeQuit;

extern Bool window_subsystem_init( void);
extern void window_subsystem_cleanup( void);
extern void window_subsystem_done( void);
extern void build_static_vmt( void *vmt);
extern void kill_zombies( void);
extern void init_image_support( void);
extern void prima_init_image_subsystem( void);
extern void prima_cleanup_image_subsystem( void);

#endif
