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

#define CHECK_GP(ret) \
	if ( !is_opt(optSystemDrawable)) { \
		warn("This method is not available because %s is not a system Drawable object. You need to implement your own (ref:%d)", my->className, __LINE__);\
		return ret; \
	}

#define MAX_CHARACTERS 8192

#define FONTMAPPER_VECTOR_BASE 9 /* 512 chars or 64 bytes per vector - make sure it's greater than 256, text_wrap/abc depends on that */
#define FONTMAPPER_VECTOR_MASK ((1 << FONTMAPPER_VECTOR_BASE) - 1)

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

void
Drawable_clear_font_abc_caches( Handle self);

PFontABC
Drawable_call_get_font_abc( Handle self, unsigned int from, unsigned int to, int flags);

int
Drawable_check_length( int from, int len, int real_len );

unsigned int
Drawable_find_font(uint32_t c, int pitch, int style, uint16_t preferred_font);

char *
Drawable_font_key( const char * name, unsigned int style);

int
Drawable_get_glyphs_width( Handle self, PGlyphsOutRec t, Bool add_overhangs);

char *
Drawable_hop_text(char * start, Bool utf8, int from);

void 
Drawable_hop_glyphs(GlyphsOutRec * t, int from, int len);

void
Drawable_query_ranges(PPassiveFontEntry pfe);

Bool
Drawable_read_glyphs( PGlyphsOutRec t, SV * text, Bool indexes_required, const char * caller);

Bool
Drawable_switch_font( Handle self, uint16_t fid);

Bool
Drawable_read_line_ends(SV *lineEnd, DrawablePaintState *state);

void
Drawable_line_end_refcnt( DrawablePaintState *gs, int delta);

#ifdef __cplusplus
}
#endif
#endif
