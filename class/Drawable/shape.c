#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "Application.h"
#include "Drawable_private.h"

#ifdef WITH_FRIBIDI
#include <fribidi/fribidi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static void*
warn_malloc(ssize_t size)
{
	void * ret;
	if (!(ret = malloc(size))) {
		warn("Drawable.text_shape: not enough memory");
		return NULL;
	}
	return ret;
}

static uint32_t*
sv2uint32( SV * text, unsigned int * size, unsigned int * flags)
{
	STRLEN dlen;
	register char * src;
	uint32_t *ret;

	src = SvPV(text, dlen);
	if (prima_is_utf8_sv(text)) {
		*flags |= toUTF8;
		*size = prima_utf8_length(src, dlen);
	} else {
		*size = dlen;
	}

	if (!(ret = ( uint32_t*) warn_malloc(sizeof(uint32_t) * (*size))))
		return NULL;

	if (*flags & toUTF8 ) {
		uint32_t *dst = ret;
		while ( dlen > 0 && dst - ret < *size) {
			STRLEN charlen;
			UV uv;
			uv = prima_utf8_uvchr(src, dlen, &charlen);
			if ( uv > 0x10FFFF ) uv = 0x10FFFF;
			*(dst++) = uv;
			if ( charlen == 0 ) break;
			src  += charlen;
			dlen -= charlen;
		}
		*size = dst - ret;
	} else {
		register int i = *size;
		register uint32_t *dst = ret;
		while (i-- > 0) *(dst++) = *((unsigned char*) src++);
	}

	return ret;
}

/*
Iterator for individual shaper runs. Each run has same direction
and its indexes are increasing/decreasing monotonically. The latter is
to avoid situations where text A<PDF>B is being sent to shaper as single run AB
which might ligate.
*/

typedef struct {
	unsigned int i, vis_len;
} BidiRunRec, *PBidiRunRec;

static void
run_init(PBidiRunRec r, int visual_length)
{
	r->i       = 0;
	r->vis_len = visual_length;
}

static int
run_next(PTextShapeRec t, PBidiRunRec r)
{
	int rtl, font, start = r->i;

	if ( r->i >= r->vis_len ) return 0;

	rtl  = t->analysis[r->i];
	font = t->fonts ? t->fonts[r->i] : 0;
	for ( ; r->i < r->vis_len + 1; r->i++) {
		if (
			r->i >= r->vis_len ||          /* eol */
			(t-> analysis[r->i] != rtl) || /* rtl != ltr */
			(t->fonts && font != t->fonts[r->i])
		) {
			return r->i - start;
		}
	}

	return 0;
}

#define INVERT(_type,_start,_end)             \
{                                             \
	register _type *s = _start, *e = _end;\
	while ( s < e ) {                     \
		register _type c = *s;        \
		*(s++) = *e;                  \
		*(e--) = c;                   \
	}                                     \
}

static void
run_alloc( PTextShapeRec t, unsigned int visual_start, unsigned int visual_len, Bool invert_rtl, PTextShapeRec run)
{
	unsigned int i, flags = 0;
	bzero(run, sizeof(TextShapeRec));

	run-> text         = t->text + visual_start;
	run-> v2l          = t->v2l + visual_start;
	if ( t-> fonts )
		run-> fonts  = t->fonts + visual_start;
	for ( i = 0; i < visual_len; i++) {
		register uint32_t c = run->text[i];
		if ( 
			(c >= 0x200e && c <= 0x200f) || /* dir marks */
			(c >= 0x202a && c <= 0x202e) || /* embedding */
			(c >= 0x2066 && c <= 0x2069)    /* isolates */
		)
			continue;

		run->text[run->len] = run->text[i];
		run->v2l[run->len] = run->v2l[i];
		run->len++;
	}
	if ( t->analysis[visual_start] & 1) {
		if ( invert_rtl ) {
			INVERT(uint32_t, run->text, run->text + run->len - 1);
			INVERT(uint16_t, run->v2l,  run->v2l + run->len - 1);
		}
		flags = toRTL;
	}
	run-> flags        = ( t-> flags & ~toRTL ) | flags;
	run-> n_glyphs_max = t->n_glyphs_max - t-> n_glyphs;
	run-> glyphs       = t->glyphs  + t-> n_glyphs;
	run-> indexes      = t->indexes + t-> n_glyphs;
	if ( t-> positions )
		run-> positions = t->positions + t-> n_glyphs * 2;
	if ( t-> advances )
		run-> advances  = t->advances + t-> n_glyphs;
}


/* minimal bidi and unicode processing - does not swap RTLs */
static Bool
fallback_reorder(PTextShapeRec t)
{
	unsigned int i;
	register uint32_t* text = t->text;
	register Byte *analysis = t->analysis;
	register uint16_t* v2l  = t->v2l;
	for ( i = 0; i < t->len; i++) {
		register uint32_t c = *(text++);
		*(analysis++) = (
			(c == 0x590) ||
			(c == 0x5be) ||
			(c == 0x5c0) ||
			(c == 0x5c3) ||
			(c == 0x5c6) ||
			( c >= 0x5c8 && c <= 0x5ff) ||
			(c == 0x608) ||
			(c == 0x60b) ||
			(c == 0x60d) ||
			( c >= 0x61b && c <= 0x64a) ||
			( c >= 0x66d && c <= 0x66f) ||
			( c >= 0x671 && c <= 0x6d5) ||
			( c >= 0x6e5 && c <= 0x6e6) ||
			( c >= 0x6ee && c <= 0x6ef) ||
			( c >= 0x6fa && c <= 0x710) ||
			( c >= 0x712 && c <= 0x72f) ||
			( c >= 0x74b && c <= 0x7a5) ||
			( c >= 0x7b1 && c <= 0x7ea) ||
			( c >= 0x7f4 && c <= 0x7f5) ||
			( c >= 0x7fa && c <= 0x815) ||
			(c == 0x81a) ||
			(c == 0x824) ||
			(c == 0x828) ||
			( c >= 0x82e && c <= 0x858) ||
			( c >= 0x85c && c <= 0x8d3) ||
			(c == 0x200f) ||
			(c == 0xfb1d) ||
			( c >= 0xfb1f && c <= 0xfb28) ||
			( c >= 0xfb2a && c <= 0xfd3d) ||
			( c >= 0xfd40 && c <= 0xfdcf) ||
			( c >= 0xfdf0 && c <= 0xfdfc) ||
			( c >= 0xfdfe && c <= 0xfdff) ||
			( c >= 0xfe70 && c <= 0xfefe) ||
			( c >= 0x10800 && c <= 0x1091e) ||
			( c >= 0x10920 && c <= 0x10a00) ||
			(c == 0x10a04) ||
			( c >= 0x10a07 && c <= 0x10a0b) ||
			( c >= 0x10a10 && c <= 0x10a37) ||
			( c >= 0x10a3b && c <= 0x10a3e) ||
			( c >= 0x10a40 && c <= 0x10ae4) ||
			( c >= 0x10ae7 && c <= 0x10b38) ||
			( c >= 0x10b40 && c <= 0x10e5f) ||
			( c >= 0x10e7f && c <= 0x10fff) ||
			( c >= 0x1e800 && c <= 0x1e8cf) ||
			( c >= 0x1e8d7 && c <= 0x1e943) ||
			( c >= 0x1e94b && c <= 0x1eeef) ||
			( c >= 0x1eef2 && c <= 0x1efff)
		) ? 1 : 0; 
		*(v2l++) = i;
	}
	return true;
}

#ifdef WITH_FRIBIDI
/* fribidi unicode processing - swaps RTLs */
static Bool
bidi_reorder(PTextShapeRec t, Bool arabic_shaping)
{
	Byte *buf, *ptr;
	unsigned int i, sz, mlen;
	FriBidiFlags flags = FRIBIDI_FLAGS_DEFAULT;
  	FriBidiParType   base_dir;
	FriBidiCharType* types;
	FriBidiArabicProp* ar;
	FriBidiStrIndex* v2l;
#if FRIBIDI_INTERFACE_VERSION > 3
	FriBidiBracketType* bracket_types;
#endif

	mlen = sizeof(void*) * ((t->len / sizeof(void*)) + 1); /* align pointers */
	sz = mlen * (
		sizeof(FriBidiCharType) + 
		sizeof(FriBidiArabicProp) +
		sizeof(FriBidiStrIndex) +
#if FRIBIDI_INTERFACE_VERSION > 3
		sizeof(FriBidiBracketType) +
#endif
		0
	);
	if ( !( buf = warn_malloc(sz)))
		return false;
	bzero(buf, sz);

	types    = (FriBidiCharType*)   (ptr = buf);
	ar       = (FriBidiArabicProp*) (ptr += mlen * sizeof(FriBidiCharType));
	v2l      = (FriBidiStrIndex*)   (ptr += mlen * sizeof(FriBidiArabicProp));
#if FRIBIDI_INTERFACE_VERSION > 3
	bracket_types = (FriBidiBracketType*) (ptr += mlen * sizeof(FriBidiStrIndex));
#endif
	fribidi_get_bidi_types(t->text, t->len, types);
	base_dir = ( t->flags & toRTL ) ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR;

#if FRIBIDI_INTERFACE_VERSION > 3
	fribidi_get_bracket_types(t->text, t->len, types, bracket_types);
	if ( !fribidi_get_par_embedding_levels_ex(
		types, bracket_types, t->len, &base_dir,
		(FriBidiLevel*)t->analysis)) goto FAIL;
#else
	if ( !fribidi_get_par_embedding_levels(
		types, t->len, &base_dir,
		(FriBidiLevel*)t->analysis)) goto FAIL;
#endif
	if ( arabic_shaping ) {
		flags |= FRIBIDI_FLAGS_ARABIC;
		fribidi_get_joining_types(t->text, t->len, ar);
		fribidi_join_arabic(types, t->len, (FriBidiLevel*)t->analysis, ar);
		fribidi_shape(flags, (FriBidiLevel*)t->analysis, t->len, ar, t->text);
	}

	for ( i = 0; i < t->len; i++)
		v2l[i] = i;
    	if ( !fribidi_reorder_line(flags, types, t->len, 0,
		base_dir, (FriBidiLevel*)t->analysis, t->text, v2l))
		goto FAIL;
	for ( i = 0; i < t->len; i++)
		t->v2l[i] = v2l[i];
	free( buf );

	return true;

FAIL:
	free( buf );
	return false;
}

#endif

static void
analyze_fonts( Handle self, PTextShapeRec t, uint16_t * fonts)
{
	unsigned int i;
	uint32_t *text = t-> text;
	int pitch      = (t->flags >> toPitch) & fpMask;
	char * key     = Drawable_font_key(var->font.name, var->font.style);
	uint16_t fid   = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));

	bzero(fonts, t->len * sizeof(uint16_t));

	for ( i = 0; i < t->len; i++) {
		unsigned int nfid = Drawable_find_font(*(text++), pitch, var->font.style, fid);
		if ( nfid != fid ) fonts[i] = nfid;
	}

	/* make sure that all combiners are same font as base glyph */
	text = t->text;
	for ( i = 0; i < t->len; ) {
		unsigned int j, base, non_base_font = 0, need_align = 0;
		if ( text[i] >= 0x300 && text[i] <= 0x36f ) {
			i++;
			continue;
		}

		base = i++;
		for ( ; i < t->len; ) {
			if ( text[i] < 0x300 || text[i] > 0x36f )
				break;
			if ( fonts[i] != fonts[base] ) {
				need_align = 1;
				non_base_font = fonts[i];
				break;
			}
			i++;
		}
		if (!need_align) continue;

		/* don't test all combiners for all fonts, such as f.ex. base glyph is Arial, ` is Courier, and ~ is Times.
		   Wild guess is that if ` is supported, others should be too */
		for ( j = base; j <= i; j++)
			fonts[j] = non_base_font;

	}
}

static Bool
shape_unicode(Handle self, PTextShapeRec t, PTextShapeFunc shaper,
	Bool glyph_mapper_only, Bool fribidi_arabic_shaping, Bool reorder
) {
	Bool ok, reorder_swaps_rtl, font_changed = false;
	Byte analysis_buf[MAX_CHARACTERS], *save_analysis, *analysis;
	uint16_t fonts_buf[MAX_CHARACTERS], *save_fonts, *fonts;
	uint16_t l2v_buf[MAX_CHARACTERS], *l2v;
	unsigned int i, run_offs, run_len;
	semistatic_t p_analysis, p_fonts, p_l2v;
	BidiRunRec brr;

#ifdef _DEBUG
	printf("\n%s input: ", (t->flags & toRTL) ? "rtl" : "ltr");
	for (i = 0; i < t->len; i++)
		printf("%x ", t->text[i]);
	printf("\n");
#endif

#ifdef WITH_FRIBIDI
	if ( prima_guts.use_fribidi ) {
		reorder_swaps_rtl = true;
		ok = bidi_reorder(t, fribidi_arabic_shaping);
	}
	else
#endif
	{
		reorder_swaps_rtl = false;
		ok = fallback_reorder(t);
	}
	if (!ok) return false;

	if ( !reorder )
		reorder_swaps_rtl = !reorder_swaps_rtl;

#ifdef _DEBUG
	printf("%s output: ", (t->flags & toRTL) ? "rtl" : "ltr");
	for (i = 0; i < t->len; i++)
		printf("%d(%x) ", t->analysis[i], t->text[i]);
	printf("\n");
#endif

	semistatic_init( &p_analysis, &analysis_buf, sizeof(Byte),     MAX_CHARACTERS);
	semistatic_init( &p_fonts,    &fonts_buf,    sizeof(uint16_t), MAX_CHARACTERS);
	semistatic_init( &p_l2v,      &l2v_buf,      sizeof(uint16_t), MAX_CHARACTERS);

	if ( !semistatic_expand( &p_analysis, t->len ))
		return false;
	if ( !semistatic_expand( &p_l2v, t->len )) {
		semistatic_done( &p_analysis);
		return false;
	}
	if ( t->fonts && !semistatic_expand( &p_fonts, t->len )) {
		semistatic_done( &p_l2v);
		semistatic_done( &p_analysis);
		return false;
	}

	analysis = (Byte*)     p_analysis.heap;
	fonts    = (uint16_t*) p_fonts.heap;
	l2v      = (uint16_t*) p_l2v.heap;

	for ( i = 0; i < t->len; i++)
		l2v[t->v2l[i]] = i;
	for ( i = 0; i < t->len; i++)
		analysis[i] = t->analysis[l2v[i]];
	if ( t-> fonts )
		analyze_fonts(self, t, fonts);

#ifdef _DEBUG
	printf("v2l: ");
	for (i = 0; i < t->len; i++)
		printf("%d ", t->v2l[i]);
	printf("\n");
	printf("l2v: ");
	for (i = 0; i < t->len; i++)
		printf("%d(%d) ", l2v[i], analysis[i]);
	printf("\n");
	if ( t-> fonts ) {
		printf("fonts: ");
		for (i = 0; i < t->len; i++)
			printf("%d ", fonts[i]);
		printf("\n");
	}
#endif

	run_offs = 0;
	run_init(&brr, t->len);
	save_analysis = t->analysis;
	t-> analysis = analysis;
	save_fonts = t->fonts;
	if ( t->fonts ) t-> fonts = fonts;
	while (( run_len = run_next(t, &brr)) > 0) {
		TextShapeRec run;
		run_alloc(t, run_offs, run_len, glyph_mapper_only ^ reorder_swaps_rtl, &run);
		if (run.len == 0) {
			run_offs += run_len;
			continue;
		}
#ifdef _DEBUG
	{
		int i;
		printf("shaper input %s %d - %d: ", (run.flags & toRTL) ? "rtl" : "ltr", run_offs, run_offs + run.len - 1);
		for (i = 0; i < run.len; i++) printf("%x ", run.text[i]);
		printf("\n");
	}
#endif
		if ( t-> fonts && ( run.fonts[0] != 0 || font_changed )) {
			if ( run.fonts[0] == 0 ) {
				apc_gp_set_font( self, &var->font);
			} else {
				if ( Drawable_switch_font(self, run.fonts[0])) {
#ifdef _DEBUG
					printf("%d: set font #%d\n", run_offs, run.fonts[0]);
#endif
					font_changed = true;
				}
#ifdef _DEBUG
				else {
					printf("%d: failed to set font #%d\n", run_offs, run.fonts[0]);
				}
#endif
			}
		}
		ok = shaper( self, &run );
#ifdef _DEBUG
	{
		int i;
		printf("shaper output: ");
		for (i = 0; i < run.n_glyphs; i++) 
			printf("%d(%x) ", glyph_mapper_only ? -1 : run.indexes[i], run.glyphs[i]);
		printf("\n");
	}
#endif
		if (!ok) break;
		for (i = t->n_glyphs; i < t->n_glyphs + run.n_glyphs; i++) {
			int x = glyph_mapper_only ? i - t->n_glyphs : t->indexes[i];
			t->indexes[i] = t->v2l[x + run_offs];
			if ( t-> fonts ) save_fonts[i] = run.fonts[0];
		}
		run_offs += run_len;
#ifdef _DEBUG
	{
		int i;
		printf("indexes copied back: ");
		if ( t-> fonts) printf("(font=%d) ", save_fonts[t->n_glyphs]);
		for (i = t->n_glyphs; i < t->n_glyphs + run.n_glyphs; i++)
			printf("%d ", t->indexes[i]);
		printf("\n");
	}
#endif
		t-> n_glyphs += run.n_glyphs;
	}
	t-> analysis = save_analysis;
	t-> fonts = save_fonts;

	semistatic_done( &p_analysis);
	semistatic_done( &p_fonts);
	semistatic_done( &p_l2v);

	if ( font_changed )
		apc_gp_set_font( self, &var->font);

	return ok;
}

static Bool
bidi_only_shaper( Handle self, PTextShapeRec r)
{
        r-> n_glyphs = r->len;
	bzero(r->glyphs, sizeof(uint16_t) * r->len);
	if ( r-> advances ) {
		bzero(r->advances, r->len * sizeof(uint16_t));
		bzero(r->positions, r->len * 2 * sizeof(int16_t));
	}
	return true;
}

static Bool
lang_is_rtl(void)
{
	static int cached = -1;
	SV * app, *sub, *ret;

	if ( cached >= 0 ) return cached;
	app = newSVpv("Prima::Application", 0);
	if ( !( sub = (SV*) sv_query_method( app, "lang_is_rtl", 0))) {
		sv_free(app);
		return cached = false;
	}
	ret = cv_call_perl( app, sub, "<");
	sv_free(app);
	return cached = (ret && SvOK(ret)) ? SvBOOL(ret) : 0;
}

SV*
Drawable_text_shape( Handle self, SV * text_sv, HV * profile)
{
	dPROFILE;
	gpARGS;
	int i;
	SV * ret = NULL_SV, 
		*sv_glyphs = NULL_SV,
		*sv_indexes = NULL_SV,
		*sv_positions = NULL_SV,
		*sv_advances = NULL_SV,
		*sv_fonts = NULL_SV;
	PTextShapeFunc system_shaper;
	TextShapeRec t;
	int shaper_type, level = tsDefault;
	Bool skip_if_simple = false, return_zero = false, force_advances = false, 
		reorder = true, polyfont = true;
	Bool gp_enter;

	/* forward, if any */
	if ( SvROK(text_sv)) {
		SV * ref = newRV((SV*) profile);
		gpENTER(NULL_SV);
		ret = sv_call_perl(text_sv, "text_shape", "<HS", self, ref);
		gpLEAVE;
		hv_clear(profile); /* old gencls bork */
		sv_free(ref);
		return newSVsv(ret);
	}
	CHECK_GP(NULL_SV);
	bzero(&t, sizeof(t));

	/* asserts */
#ifdef WITH_FRIBIDI
	if ( prima_guts.use_fribidi && sizeof(FriBidiLevel) != 1) {
		warn("sizeof(FriBidiLevel) != 1, fribidi is disabled");
		prima_guts.use_fribidi = false;
	}
#endif

	if ( pexist(rtl) ?
		pget_B(rtl) :
		(prima_guts.application ? P_APPLICATION->textDirection : lang_is_rtl()
	))
		t.flags |= toRTL;
	if ( pexist(language))
		t.language = pget_c(language);
	if ( pexist(skip_if_simple))
		skip_if_simple = pget_B(skip_if_simple);
	if ( pexist(advances))
		force_advances = pget_B(advances);
	if ( pexist(reorder)) {
		reorder = pget_B(reorder);
	}
	if ( !reorder )
		t.flags &= ~toRTL;
	if ( pexist(level)) {
		level = pget_i(level);
		if ( level < tsNone || level > tsBytes ) level = tsFull;
	}
	if ( pexist(polyfont))
		polyfont = pget_B(polyfont);
	if ( level == tsBytes || level == tsNone )
		polyfont = false;
	if ( pexist(pitch)) {
		int pitch = pget_i(pitch);
		if ( pitch == fpVariable || pitch == fpFixed )
			t.flags |= pitch << toPitch;
	} else if ( var-> font. pitch == fpFixed )
		t.flags |= fpFixed << toPitch;

	hv_clear(profile); /* old gencls bork */

	/* font supports shaping? */
	if ( level == tsNone ) {
		shaper_type    = tsNone;
		force_advances = false;
		gp_enter       = false;
		system_shaper  = bidi_only_shaper;
	} else {
		shaper_type    = level;
		gp_enter       = true;
		gpENTER(NULL_SV);
		if (!( system_shaper = apc_gp_get_text_shaper(self, &shaper_type))) {
			return_zero = true;
			goto EXIT;
		}
	}

	if ( shaper_type == tsFull ) {
		force_advances = false;
		skip_if_simple = false; /* kerning may affect advances */ 
	}

	/* allocate buffers */
	if (!(t.text = sv2uint32(text_sv, &t.len, &t.flags)))
		goto EXIT;
	if ( level == tsBytes ) {
		int i;
		uint32_t *c = t.text;
		for ( i = 0; i < t.len; i++, c++)
			if ( *c > 255 ) *c = 255;
	}

	if ( t.len == 0 ) {
		return_zero = true;
		goto EXIT;
	}

	if ( level != tsBytes ) {
		if (!(t.analysis = warn_malloc(t.len)))
			goto EXIT;
		if (!(t.v2l = warn_malloc(sizeof(uint16_t) * t.len)))
			goto EXIT;
	}

	/* MSDN, on ScriptShape: A reasonable value is (1.5 * cChars + 16) */
	t.n_glyphs_max = t.len * 2 + 16;
#define ALLOC(id,n,type) { \
	sv_##id = prima_array_new( t.n_glyphs_max * n * sizeof(uint16_t)); \
	t.id   = (type*) prima_array_get_storage(sv_##id); \
}

	ALLOC(glyphs,1,uint16_t);
	ALLOC(indexes,1,uint16_t);
	if ( shaper_type == tsFull || force_advances) {
		ALLOC(positions,2,int16_t);
		ALLOC(advances,1,uint16_t);
	} else {
		sv_positions = sv_advances = NULL_SV;
		t.positions = NULL;
		t.advances = NULL;
	}
	if ( polyfont ) {
		ALLOC(fonts,1,uint16_t);
		bzero(t.fonts, sizeof(uint16_t) * t.n_glyphs_max);
	} else {
		sv_fonts = NULL_SV;
		t.fonts = NULL;
	}
#undef ALLOC

	if ( level == tsBytes ) {
		int i;
		uint16_t * indexes;
		if ( !system_shaper(self, &t))
			goto EXIT;
		for ( i = 0, indexes = t.indexes; i < t.n_glyphs; i++)
			*(indexes++) = i;
	} else {
		if ( !shape_unicode(self, &t,
			system_shaper, shaper_type < tsFull,
			(level < tsFull) ? false : (shaper_type < tsFull), reorder)
		) goto EXIT;
	}

	if ( skip_if_simple ) {
		Bool is_simple = true;
		for ( i = 0; i < t.n_glyphs; i++) {
			if ( i != t.indexes[i] ) {
				is_simple = false;
				break;
			}
		}
		if ( is_simple && !(t.flags & toUTF8))
			for ( i = 0; i < t.len; i++) {
				if (t.text[i] > 0x7f) {
					is_simple = false;
					break;
				}
			}
		if ( is_simple ) {
			return_zero = true;
			goto EXIT;
		}
	}

	if (gp_enter) gpLEAVE;

	/* encode direction */
	if ( level != tsBytes ) {
		for ( i = 0; i < t.n_glyphs; i++) {
			if ( t.analysis[ t.indexes[i] ] & 1 )
				t.indexes[i] |= toRTL;
		}
	}
	/* add an extra index as text length */
	t.indexes[t.n_glyphs] = t.len;

	free(t.v2l );
	free(t.analysis );
	free(t.text);
	t.v2l = NULL;
	t.analysis = NULL;
	t.text = NULL;

	if (sv_fonts != NULL_SV) {
		int i, non_zero = 0;
		for ( i = 0; i < t.n_glyphs; i++) {
			if ( t.fonts[i] != 0 ) {
				non_zero = 1;
				break;
			}
		}
		if (!non_zero) {
			sv_free(sv_fonts);
			sv_fonts = NULL_SV;
		}
	}

#define BIND(x,n,d,letter) { \
	prima_array_truncate(x, (t.n_glyphs * n + d) * sizeof(uint16_t)); \
	x = sv_2mortal(prima_array_tie( x, sizeof(uint16_t), letter)); \
}
	BIND(sv_glyphs, 1, 0, "S");
	BIND(sv_indexes, 1, 1, "S");
	if ( sv_positions != NULL_SV ) BIND(sv_positions, 2, 0, "s");
	if ( sv_advances  != NULL_SV ) BIND(sv_advances,  1, 0, "S");
	if ( sv_fonts     != NULL_SV ) BIND(sv_fonts,     1, 0, "S");
#undef BIND

	return newSVsv(call_perl(self, "new_glyph_obj", "<SSSSS",
		sv_glyphs, sv_indexes, sv_advances, sv_positions, sv_fonts
	));

EXIT:
	if (gp_enter) gpLEAVE;
	if ( t.text     ) free(t.text     );
	if ( t.v2l      ) free(t.v2l      );
	if ( t.analysis ) free(t.analysis );
	if ( t.indexes  ) sv_free(sv_indexes );
	if ( t.glyphs   ) sv_free(sv_glyphs   );
	if ( t.positions) sv_free(sv_positions);
	if ( t.advances ) sv_free(sv_advances );

	return return_zero ? newSViv(0) : NULL_SV;
}

#ifdef __cplusplus
}
#endif
