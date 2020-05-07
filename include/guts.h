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
extern Bool   use_fribidi;

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

/*

font_passive_entries is queried at start and is filled with fonts that can be
used in font substitution. Each contains a Font record and a set of bit
vectors, split by FONTMAPPER_VECTOR_MASK (PassiveFontEntry). Bit vectors are
built on demand.

font_map_indexes is a plain array of integers, each is a font_passive_entries index.
The array grows either internally when font mapper needs it, or explicitly by
Application.fontPalette. text_shape().fonts uses indexes of font_map_indexes,
and text_out(glyph) where glyph has fonts, uses font_map_indexes to get to 
font_passive_entries.font and select the actual fonts.

font_active_entries is a sparse list that contains either NULLs or PList
entries.  Each index is a block for (INDEX >> FONTMAPPER_VECTOR_BASE), and
contains set of FONT IDs, each is font_passive_entries index. It is added to
whenever text_shape needs another font, selecting entries from font_passive_entries,
or filling them by querying ranges

*/

#define FONTMAPPER_VECTOR_BASE 9 /* 512 chars or 64 bytes per vector */
#define FONTMAPPER_VECTOR_MASK ((1 << FONTMAPPER_VECTOR_BASE) - 1)

typedef struct {
	Font   font;
	List   vectors;
	Handle handle; 
	Bool   ranges_queried;
} PassiveFontEntry, *PPassiveFontEntry;

extern List font_map_indexes;
extern List font_active_entries;
extern List font_passive_entries;
extern int font_mapper_default_id;

#define PASSIVE_FONT(fid) ((PPassiveFontEntry) font_passive_entries.items[(unsigned int)(fid)])

extern void   prima_init_font_mapper(void);
extern void   prima_cleanup_font_mapper(void);
extern PFont  prima_font_mapper_add_passive_font(void);
extern int    prima_font_mapper_find_font(uint32_t c, int pitch, Handle * handle);

#ifdef __cplusplus
}
#endif


#endif
