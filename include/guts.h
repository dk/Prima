/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* $Id$ */
#ifndef _GUTS_H_
#define _GUTS_H_

#ifndef _APRICOT_H_
#include "apricot.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern Bool dolbug;
extern Bool waitBeforeQuit;

#define dPUB_ARGS    int rc = recursiveCall
#define PUB_CHECK    rc = recursiveCall
#define DOLBUG       if(dolbug)debug_write

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
extern void build_static_vmt( void *vmt);
extern void kill_zombies( void);
extern void prima_init_image_subsystem( void);
extern void prima_cleanup_image_subsystem( void);

extern Handle  gimme_the_real_mate( SV *perlObject);

/* kernel exports */
extern XS( Component_set_notification_FROMPERL);

extern PRGBColor read_palette( int * palSize, SV * palette);
extern Bool prima_read_point( SV *rvav, int * pt, int number, char * error);
extern Bool accel_notify ( Handle group, Handle self, PEvent event);
extern Bool font_notify ( Handle self, Handle child, void * font);
extern Bool find_accel( Handle self, Handle item, int * key);
extern Bool single_color_notify ( Handle self, Handle child, void * color);
extern Bool kill_all( Handle self, Handle child, void * dummy);

#ifdef __cplusplus
}
#endif


#endif
