#ifndef Drawable_private_H_
#define Drawable_private_H_

#ifdef __cplusplus
extern "C" {
#endif

#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)
#define var (( PDrawable) self)

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER(fail)     if ( !inPaint) if ( !my-> begin_paint_info( self)) return (fail)
#define gpLEAVE           if ( !inPaint) my-> end_paint_info( self)
#define gpCHECK(ret)      if ( !is_opt(optSystemDrawable)) { \
	warn("This method is not available because %s is not a system Drawable object. You need to implement your own (ref:%d)", my->className, __LINE__);\
	return ret; \
}

#define dmARGS            Bool inPaint = my->assert_drawing_mode(self, admStatus )
#define dmENTER(fail)     if ( !inPaint) if ( !my-> assert_drawing_mode( self, admEnter)) return (fail)
#define dmLEAVE           if ( !inPaint) my-> assert_drawing_mode( self, admLeave)
#define dmCHECK(ret)      if ( !my-> assert_drawing_mode( self, admAllowed )) { \
	warn("This method is not available because %s is not a system Drawable object. You need to implement your own (ref:%d)", my->className, __LINE__);\
	return ret; \
}

#define MAX_CHARACTERS 8192

#define FONTMAPPER_VECTOR_BASE 9 /* 512 chars or 64 bytes per vector - make sure it's greater than 256, text_wrap/abc depends on that */
#define FONTMAPPER_VECTOR_MASK ((1 << FONTMAPPER_VECTOR_BASE) - 1)

#define VAR_MATRIX var->current_state.matrix

typedef struct {
	Font   font;
	List   vectors;
	Bool   ranges_queried;
	Bool   is_active;
	Bool   is_enabled;
	unsigned int flags, style;
} PassiveFontEntry, *PPassiveFontEntry;

#define PASSIVE_FONT(fid) ((PPassiveFontEntry) font_passive_entries.items[(unsigned int)(fid)])

extern List  font_active_entries;
extern List  font_passive_entries;
extern PHash font_substitutions;

#define STYLE_MASK (fsThin|fsItalic|fsBold)
#define N_STYLES   (1 + STYLE_MASK)
extern int   font_mapper_default_id[N_STYLES];

#define FIDGID2BASE(fid,gid) (((fid) << 8) + ((gid) >> 8))
#define GID2OFFSET(gid)      ((gid) & 0xff)
#define BASE_FROM(base)      ((base & 0xff) * 256)
#define BASE_TO(base)        (BASE_FROM(base) + 255)
#define BASE_RANGE           256

void*
Drawable_find_in_cache( PList p, int base );

Bool
Drawable_fill_cache( PList * cache, int base, void *entry);

void
Drawable_clear_font_abc_caches( Handle self);

void
Drawable_clear_descent_crossing_caches( Handle self);

PFontABC
Drawable_query_abc_glyph_range( Handle self, int base);

PFontABC
Drawable_call_get_font_abc( Handle self, unsigned int from, unsigned int to, int flags);

int*
Drawable_call_get_glyph_descents( Handle self, unsigned int from, unsigned int to);

int
Drawable_check_length( int from, int len, int real_len );

unsigned int
Drawable_find_font(uint32_t c, int pitch, int style, uint16_t preferred_font);

char *
Drawable_font_key( const char * name, unsigned int style);

int
Drawable_get_glyphs_width( Handle self, PGlyphsOutRec t, Bool add_overhangs);

void
Drawable_get_glyphs_box( Handle self, PGlyphsOutRec t, Point * pt);

char *
Drawable_hop_text(char * start, Bool utf8, int from);

void 
Drawable_hop_glyphs(GlyphsOutRec * t, int from, int len);

void
Drawable_query_ranges(PPassiveFontEntry pfe);

Bool
Drawable_read_glyphs( PGlyphsOutRec t, SV * text, Bool indexes_required, const char * caller);

void
Drawable_restore_font_internal( Handle self, SaveFont *f, Bool quick);

Bool
Drawable_switch_font_internal( Handle self, SaveFont *f, uint16_t fid, Bool quick);

Bool
Drawable_read_line_ends(SV *lineEnd, DrawablePaintState *state);

void
Drawable_line_end_refcnt( DrawablePaintState *gs, int delta);

int
Drawable_resolve_line_end_index( DrawablePaintState *gs, int index);

NRect
Drawable_line_end_box( DrawablePaintState *gs, int index);

#ifdef __cplusplus
}
#endif
#endif
