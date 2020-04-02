#include "apricot.h"
#include "Drawable.h"

#ifdef WITH_FRIBIDI
#include <fribidi/fribidi.h>
#endif
#define MAX_CHARACTERS 8192
extern Bool   use_fribidi;

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


static void*
read_subarray( AV * av, int index, int length_expected, int * plen, const char * caller, const char * subarray )
{
	SV ** holder;
	void * ref;
	size_t length;
	char * letter;
	holder = av_fetch(av, index, 0);

	if (
		!holder ||
		!*holder ||
		!SvOK(*holder)
	) {
		warn("invalid subarray #%d (%s) passed to %s", index, subarray, caller);
		return NULL;
	}

	if ( !prima_array_parse( *holder, &ref, &length, &letter)) {
		warn("invalid subarray #%d (%s) passed to %s: %s", index, subarray, caller, "not a Prima::array");
		return NULL;
	}

	if (*letter != 'S') {
		warn("invalid subarray #%d (%s/%c) passed to %s: %s", index, subarray, *letter, caller, "not a Prima::array of 16-bit integers");
		return NULL;
	}

	if ( length_expected >= 0 && length < length_expected ) {
		warn("invalid subarray #%d (%s) passed to %s: length must be at least %d", index, subarray, caller, length_expected);
		return NULL;
	}
	if ( plen ) *plen = length;
	return ref;
}

static Bool
read_glyphs( PGlyphsOutRec t, SV * text, Bool clusters_required, const char * caller)
{
	int len;
	AV* av;

	bzero(t, sizeof(GlyphsOutRec));
	/* assuming caller checked for SvTYPE( SvRV( text)) == SVt_PVAV */

	av  = (AV*) SvRV(text);

	/* is this just a single glyphstr? */
	if ( SvTIED_mg(( SV*) av, PERL_MAGIC_tied )) {
		void * ref;
		size_t length;
		char * letter;

		if ( clusters_required ) {
			warn("%s requires glyphstr with clusters", caller);
			return false;
		}
	
		if ( !prima_array_parse( text, &ref, &length, &letter) || *letter != 'S') {
			warn("invalid glyphstr passed to %s: %s", caller, "not a Prima::array");
			return false;
		}

		t-> glyphs = ref;
		t-> len    = length;
		return true;
	}

	len = av_len(av) + 1;
	if ( len > 4 ) len = 4; /* we don't need more */
	if ( len < 1 || (len != 2 && len != 4) ) {
		warn("malformed glyphs array in %s", caller);
		return false;
	}

	if ( !( t-> glyphs = read_subarray( av, 0, -1, &t->len, caller, "glyphs")))
		return false;
	if ( t->len == 0 )
		return false;

	switch ( len ) {
	case 4:
		if ( !( t-> positions = read_subarray( av, 3, t->len * 2, NULL, caller, "positions")))
			return false;
		if ( !( t-> advances = read_subarray( av, 2, t->len, NULL, caller, "advances")))
			return false;
	case 2:
		if ( !( t-> clusters = read_subarray( av, 1, t->len, NULL, caller, "clusters")))
			return false;
	}

	return true;
}

Bool
Drawable_text_out( Handle self, SV * text, int x, int y)
{
	Bool ok;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		if ( utf8) dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
		ok = apc_gp_text_out( self, c_text, x, y, dlen, utf8 ? toUTF8 : 0);
		if ( !ok) perl_error();
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!read_glyphs(&t, text, 0, "Drawable::text_out"))
			return false;
		ok = apc_gp_glyphs_out( self, &t, x, y);
		if ( !ok) perl_error();
	} else {
		SV * ret = sv_call_perl(text, "text_out", "<Hii", self, x, y);
		ok = ret && SvTRUE(ret);
	}
	return ok;
}

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
sv2unicode( SV * text, int * size, int * flags)
{
	STRLEN dlen;
	register char * src;
	uint32_t *ret;

	src = SvPV(text, dlen);
	if (prima_is_utf8_sv(text)) {
		*flags |= toUTF8;
		*size = prima_utf8_length(src);
	} else {
		*size = dlen;
	}

	if (!(ret = ( uint32_t*) warn_malloc(sizeof(uint32_t) * (*size))))
		return NULL;

	if (*flags & toUTF8 ) {
		uint32_t *dst = ret;
		while ( dlen > 0) {
			STRLEN charlen;
			*(dst++) = prima_utf8_uvchr(src, dlen, &charlen);
			if ( charlen == 0 ) break;
			src  += charlen;
			dlen -= charlen;
		}
	} else {
		register int i = *size;
		register uint32_t *dst = ret;
		while (i-- > 0) *(dst++) = *(src++);
	}

	return ret;
}

/*
Iterator for individual shaper runs. Each run has same direction
and its clusters are increasing/decreasing monotonically. The latter is
to avoid situations where text A<PDF>B is being sent to shaper as single run AB
which might ligate.
*/

typedef struct {
	int start, i, vis_len;
	Byte rtl;
	uint16_t cluster;
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
	int n, rtl, start = r->i;

	if ( r->i >= r->vis_len ) return 0;

	rtl = t->analysis[r->i];
	for ( n = 0; r->i < r->vis_len + 1; r->i++, n++) {
		if (
			r->i >= r->vis_len ||          /* eol */
			(t-> analysis[r->i] != rtl)    /* rtl != ltr */
		) {
			r->rtl = rtl;
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
run_alloc( PTextShapeRec t, int visual_start, int visual_len, Bool invert_rtl, PTextShapeRec run)
{
	int i, flags = 0;
	bzero(run, sizeof(TextShapeRec));

	run-> text         = t->text + visual_start;
	run-> v2l          = t->v2l + visual_start;
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
	run-> glyphs       = t->glyphs   + t-> n_glyphs;
	run-> clusters     = t->clusters + t-> n_glyphs;
	if ( t-> positions )
		run-> positions = t->positions + t-> n_glyphs * 2;
	if ( t-> advances )
		run-> advances = t->advances + t-> n_glyphs;
}


/* minimal bidi and unicode processing - does not swap RTLs */
static Bool
fallback_reorder(PTextShapeRec t)
{
	int i;
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
	int i, sz, mlen;
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
	/* XXX FRIBIDI_PAR_ON ? */
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

static Bool
shape(Handle self, PTextShapeRec t, PTextShapeFunc shaper, Bool glyph_mapper_only)
{
	Bool ok, reorder_swaps_rtl;
	Byte analysis[MAX_CHARACTERS], *save_analysis;
	uint16_t i, l2v[MAX_CHARACTERS], run_offs, run_len;
	BidiRunRec brr;

#ifdef _DEBUG
	printf("\n%s input: ", (t->flags & toRTL) ? "rtl" : "ltr");
	for (i = 0; i < t->len; i++)
		printf("%x ", t->text[i]);
	printf("\n");
#endif

#ifdef WITH_FRIBIDI
	if ( use_fribidi ) {
		reorder_swaps_rtl = true;
		ok = bidi_reorder(t, glyph_mapper_only);
	}
	else
#endif
	{
		reorder_swaps_rtl = false;
		ok = fallback_reorder(t);
	}
	if (!ok) return false;

#ifdef _DEBUG
	printf("%s output: ", (t->flags & toRTL) ? "rtl" : "ltr");
	for (i = 0; i < t->len; i++)
		printf("%d(%x) ", t->analysis[i], t->text[i]);
	printf("\n");
#endif

	bzero(&l2v, MAX_CHARACTERS);
	for ( i = 0; i < t->len; i++)
		l2v[t->v2l[i]] = i;
	for ( i = 0; i < t->len; i++)
		analysis[i] = t->analysis[l2v[i]];
#ifdef _DEBUG
	printf("v2l: ");
	for (i = 0; i < t->len; i++)
		printf("%d ", t->v2l[i]);
	printf("\n");
	printf("l2v: ");
	for (i = 0; i < t->len; i++)
		printf("%d(%d) ", l2v[i], analysis[i]);
	printf("\n");
#endif
	
	run_offs = 0;
	run_init(&brr, t->len);
	save_analysis = t->analysis;
	t-> analysis = analysis;
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
		ok = shaper( self, &run );
#ifdef _DEBUG
	{
		int i;
		printf("shaper output: ");
		for (i = 0; i < run.n_glyphs; i++) 
			printf("%d(%x) ", glyph_mapper_only ? -1 : run.clusters[i], run.glyphs[i]);
		printf("\n");
	}
#endif
		if (!ok) break;
		for (i = t->n_glyphs; i < t->n_glyphs + run.n_glyphs; i++) {
			int x = glyph_mapper_only ? i - t->n_glyphs : t->clusters[i];
			t->clusters[i] = t->v2l[x + run_offs];
		}
		run_offs += run_len;
#ifdef _DEBUG
	{
		int i;
		printf("clusters copied back: ");
		for (i = t->n_glyphs; i < t->n_glyphs + run.n_glyphs; i++)
			printf("%d ", t->clusters[i]);
		printf("\n");
	}
#endif
		t-> n_glyphs += run.n_glyphs;
	}
	t-> analysis = save_analysis;

	return ok;
}

SV*
Drawable_text_shape( Handle self, SV * text_sv, HV * profile)
{
	dPROFILE;
	gpARGS;
	int i;
	SV * ret = nilSV, 
		*sv_glyphs = nilSV,
		*sv_clusters = nilSV,
		*sv_positions = nilSV,
		*sv_advances = nilSV;
	PTextShapeFunc system_shaper;
	TextShapeRec t;
	int shaper_type;
	Bool skip_if_simple = false, return_zero = false;

	/* forward, if any */
	if ( SvROK(text_sv)) {
		SV * ref = newRV_noinc((SV*) profile);
		hv_clear(profile); /* old gencls bork */
		gpENTER(nilSV);
		ret = sv_call_perl(text_sv, "text_shape", "<HSS", self, text_sv, ref);
		gpLEAVE;
		sv_free(ret);
		return ret;
	}

	bzero(&t, sizeof(t));

	/* asserts */
#ifdef WITH_FRIBIDI
	if ( use_fribidi && sizeof(FriBidiLevel) != 1) {
		warn("sizeof(FriBidiLevel) != 1, fribidi is disabled");
		use_fribidi = false;
	}
#endif

	if ( pexist(rtl) && pget_B(rtl))
		t.flags |= toRTL;
	if ( pexist(positions) && pget_B(positions))
		t.flags |= toPositions;
	if ( pexist(language))
		t.language = pget_c(language);
	if ( pexist(skip_if_simple))
		t.language = pget_c(skip_if_simple);
	hv_clear(profile); /* old gencls bork */

	/* font supports shaping? */
	if (!( system_shaper = apc_gp_get_text_shaper(self, &shaper_type)))
		return newSViv(0);

	if ( skip_if_simple && (shaper_type == SHAPING_EMULATION))
		return newSViv(0);

	/* allocate buffers */
	if (!(t.text = sv2unicode(text_sv, &t.len, &t.flags)))
		goto EXIT;

	if ( t.len == 0 ) {
		free(t.text);
		return newSViv(0);
	}

	if ( t.len > MAX_CHARACTERS ) {
		warn("Drawable.text_shape: text too long, %dK max", MAX_CHARACTERS / 1024);
		t.len = MAX_CHARACTERS;
	}

	if (!(t.analysis = warn_malloc(t.len)))
		goto EXIT;
	if (!(t.v2l = warn_malloc(sizeof(uint16_t) * t.len)))
		goto EXIT;

	/* MSDN, on ScriptShape: A reasonable value is (1.5 * cChars + 16) */
	t.n_glyphs_max = t.len * 2 + 16;
#define ALLOC(id,n) { \
	sv_##id = prima_array_new( t.n_glyphs_max * n * sizeof(uint16_t)); \
	t.id   = (uint16_t*) prima_array_get_storage(sv_##id); \
}

	ALLOC(glyphs,1);
	ALLOC(clusters,1);
	if ( t.flags & toPositions) {
		ALLOC(positions,2);
		ALLOC(advances,1);
	} else {
		sv_positions = sv_advances = nilSV;
		t.positions = t.advances = NULL;
	}
#undef ALLOC

	if ( !shape(self, &t, system_shaper, shaper_type < SHAPING_FULL) ) goto EXIT;

	if ( skip_if_simple ) {
		Bool is_simple = true;
		for ( i = 0; i < t.n_glyphs; i++) {
			if ( i != t.clusters[i] ) {
				is_simple = false;
				break;
			}
		}
		if ( is_simple ) {
			return_zero = true;
			goto EXIT;
		}
	}

	/* encode direction */
	for ( i = 0; i < t.n_glyphs; i++) {
		if ( t.analysis[ t.clusters[i] ] & 1 )
			t.clusters[i] |= toRTL;
	}
	/* add an extra cluster as text length */
	t.clusters[t.n_glyphs] = t.len;

	free(t.v2l );
	free(t.analysis );
	free(t.text);
	t.v2l = NULL;
	t.analysis = NULL;
	t.text = NULL;

#define BIND(x,n,d) { \
	prima_array_truncate(x, (t.n_glyphs * n + d) * sizeof(uint16_t)); \
	x = prima_array_tie( x, sizeof(uint16_t), "S"); \
}
	BIND(sv_glyphs, 1, 0);
	BIND(sv_clusters, 1, 1);
	if ( sv_positions != nilSV ) BIND(sv_positions, 2, 0);
	if ( sv_advances != nilSV ) BIND(sv_advances, 1, 2);
#undef BIND

	return newSVsv(call_perl(self, "new_glyph_obj", "<SSSS",
		sv_glyphs, sv_clusters, sv_advances, sv_positions
	));

EXIT:
	if ( t.text     ) free(t.text     );
	if ( t.v2l      ) free(t.v2l      );
	if ( t.analysis ) free(t.analysis );
	if ( t.clusters ) sv_free(sv_clusters );
	if ( t.glyphs   ) sv_free(sv_glyphs   );
	if ( t.positions) sv_free(sv_positions);
	if ( t.advances ) sv_free(sv_advances );

	return return_zero ? newSViv(0) : nilSV;
}

static int
get_glyphs_width( PGlyphsOutRec t)
{
	int i, ret;
	uint16_t * advances = t->advances;
	for ( i = ret = 0; i < t-> len; i++)
		ret += *(advances++);
	if ( t-> flags & toAddOverhangs ) {
		ret += *(advances++);
		ret += *(advances++);
	}
	return ret;
}

int
Drawable_get_text_width( Handle self, SV * text, int flags)
{
	gpARGS;
	int res;

	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		} else
			flags &= ~toUTF8;
		gpENTER(0);
		res = apc_gp_get_text_width( self, c_text, dlen, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!read_glyphs(&t, text, 0, "Drawable::get_text_width"))
			return false;
		t.flags = flags;
		if (t.advances)
			return get_glyphs_width(&t);
		gpENTER(0);
		res = apc_gp_get_glyphs_width( self, &t);
		gpLEAVE;
	} else {
		SV * ret;
		gpENTER(0);
		ret = sv_call_perl(text, "get_text_width", "<Hi", self, flags);
		gpLEAVE;
		res = (ret && SvOK(ret)) ? SvIV(ret) : 0;
	}

	return res;
}

static void
get_glyphs_box( Handle self, PGlyphsOutRec t, Point * pt)
{
	Bool text_out_baseline;

	t-> flags = 0;

	pt[0].y = pt[2]. y = var-> font. ascent - 1;
	pt[1].y = pt[3]. y = - var-> font. descent;
	pt[4].y = pt[0]. x = pt[1].x = 0;
	pt[3].x = pt[2]. x = pt[4].x = get_glyphs_width(t);

	text_out_baseline = ( my-> textOutBaseline == Drawable_textOutBaseline) ?
		apc_gp_get_text_out_baseline(self) :
		my-> get_textOutBaseline(self);
	if ( !text_out_baseline ) {
		int i = 4, d = var->font. descent;
		while ( i--) pt[i]. y += d;
	}
}

SV *
Drawable_get_text_box( Handle self, SV * text )
{
	gpARGS;
	Point * p;
	AV * av;
	int i, flags = 0;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);

		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		}
		gpENTER( newRV_noinc(( SV *) newAV()));
		p = apc_gp_get_text_box( self, c_text, dlen, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!read_glyphs(&t, text, 0, "Drawable::get_text_box"))
			return false;
		if (t.advances) {
			if (!( p = malloc( sizeof(Point) * 5 )))
				return newRV_noinc(( SV *) newAV());
			get_glyphs_box(self, &t, p);
		} else {
			gpENTER( newRV_noinc(( SV *) newAV()));
			p = apc_gp_get_glyphs_box( self, &t);
			gpLEAVE;
		}
	} else {
		SV * ret;
		gpENTER( newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "get_text_box", "<H", self ));
		gpLEAVE;
		return ret;
	}

	av = newAV();
	if ( p) {
		for ( i = 0; i < 5; i++) {
			av_push( av, newSViv( p[ i]. x));
			av_push( av, newSViv( p[ i]. y));
		};
		free( p);
	}
	return newRV_noinc(( SV *) av);
}

static PFontABC
find_abc_in_list_cache( PList p, int base )
{
	int i;
	for ( i = 0; i < p-> count; i += 2)
		if (( unsigned int) p-> items[ i] == base)
			return ( PFontABC) p-> items[i + 1];
	return NULL;
}

static Bool
fill_abc_list_cache( PList * cache, int base, PFontABC abc)
{
	PList p;
	if ( *cache == NULL )
		*cache = plist_create( 8, 8);
	if (( p = *cache) == NULL)
		return false;
	list_add( p, ( Handle) base);
	list_add( p, ( Handle) abc);
	return true;
}

static PFontABC
call_get_font_abc( Handle self, unsigned int base, int flags)
{
	PFontABC abc;

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, flags);
		if ( !abc) return NULL;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		gpARGS;
		gpENTER(NULL);
		abc = apc_gp_get_font_abc( self, base * 256, base * 256 + 255, flags);
		gpLEAVE;
		if ( !abc) return NULL;
	} else {
		SV * sv;
		if ( !( abc = malloc( 256 * sizeof( FontABC)))) return NULL;
		sv = my-> get_font_abc( self, base * 256, base * 256 + 255, flags);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > 256 * 3) n = 256 * 3;
			n = ( n / 3) * 3;
			if ( n < 256) memset( abc, 0, 256 * sizeof( FontABC));
			for ( i = 0; i < n; i += 3) {
				SV ** holder = av_fetch( av, i, 0);
				if ( holder) abc[j]. a = ( float) SvNV( *holder);
				holder = av_fetch( av, i + 1, 0);
				if ( holder) abc[j]. b = ( float) SvNV( *holder);
				holder = av_fetch( av, i + 2, 0);
				if ( holder) abc[j]. c = ( float) SvNV( *holder);
				j++;
			}
		} else
			memset( abc, 0, 256 * sizeof( FontABC));
		sv_free( sv);
	}

	return abc;
}

static PFontABC
query_abc_range( Handle self, TextWrapRec * t, unsigned int base)
{
	PFontABC abc;

	if ( t-> utf8_text) {
		if ( 
			*(t-> unicode) && 
			(( abc = find_abc_in_list_cache( *(t->unicode), base)) != NULL)
		)
			return abc;
	} else if (*( t-> ascii))
		return *(t-> ascii);

	if ( !( abc = call_get_font_abc(self, base, t-> utf8_text ? toUTF8 : 0)))
		return NULL;

	if ( t-> utf8_text) {
		if ( !fill_abc_list_cache(t->unicode, base, abc)) {
			free( abc);
			return NULL;
		}
	} else
		*(t-> ascii) = abc;

	return abc;
}

static Bool
precalc_abc_buffer( PFontABC src, float * width, PFontABC dest)
{
	int i;
	if ( !dest) return false;
	for ( i = 0; i < 256; i++) {
		width[i] = src[i]. a + src[i]. b + src[i]. c;
		dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
		dest[i]. b = src[i]. b;
		dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
	}
	return true;
}

static Bool
add_wrapped_text(
	TextWrapRec * t, 
	int start, int utfstart, int end, int utfend,
	int tildeIndex, int * tildePos, int * tildeLine,
	char *** array, int * size
) {
	int l = end - start;
	char *c = NULL;

	if (!( t-> options & twReturnChunks)) {
		if ( !( c = allocs( l + 1)))
			return false;
		memcpy( c, t-> text + start, l);
		c[l] = 0;
	}

	if ( tildeIndex >= 0 && tildeIndex >= start && tildeIndex < end) {
		*tildeLine = t-> t_line = t-> count;
		*tildePos = tildeIndex - start;
		if ( tildeIndex == end - 1) t-> t_line++;
	}

	if ( t-> count == *size) {
		char ** n;
		*size *= 2;
		if ( !( n = (char **) realloc( *array, *size * sizeof(char*))))
			return false;
		*array = n;
	}

	if ( t-> options & twReturnChunks) {
		(*array)[ t-> count++] = INT2PTR(char*,utfstart);
		(*array)[ t-> count++] = INT2PTR(char*,utfend - utfstart);
	} else
		(*array)[ t-> count++] = c;

	return true;
}

static int
find_tilde_position( TextWrapRec * t )
{
	int i, tildeIndex = -100;

	for ( i = 0; i < t-> textLen - 1; i++)
		if ( t-> text[ i] == '~') {
			unsigned char c = t-> text[ i + 1];
			if ( c == '~' || c < ' ') {
				i++;
				continue;
			} else {
				tildeIndex = i;
				break;
			}
		}

	return tildeIndex;
}

static void
fill_tilde_properties(Handle self, TextWrapRec * t, int tildeIndex, int tildePos, int tildeOffset)
{
	UV uv;
	int base;
	PFontABC abcc;
	float start, end;

	t-> t_pos  = tildePos + (( t->options & twCollapseTilde ) ? 0 : 1);
	t-> t_char = t-> text + tildeIndex + 1;
	if ( t-> utf8_text) {
		STRLEN len;
		uv = prima_utf8_uvchr_end(t-> t_char, t->text + t-> textLen, &len);
		if ( len == 0 ) return;
	} else
		uv = *(t->t_char);

	abcc = query_abc_range( self, t, base = uv / 256) + (uv & 0xff);
	start = tildeOffset;
	end   = start + abcc-> b - 1.0;
	if ( abcc-> a < 0.0 ) {
		start += abcc->a;
		end += abcc->a;
	}
	t-> t_start = start + .5 * (( start < 0 ) ? -1 : 1);
	t-> t_end   = end   + .5 * (( end   < 0 ) ? -1 : 1);
}

static Bool
expand_tabs( TextWrapRec * t, char ** strings)
{
	int i;

	for ( i = 0; i < t-> count; i++) {
		int tabs = 0, len = 0;
		char *substr = strings[ i], *n;

		while (*substr) {
			if ( *substr == '\t') tabs++;
			substr++;
			len++;
		}
		if ( tabs == 0) continue;
	
		if ( !( n = allocs( len + tabs * t-> tabIndent + 1)))
			return false;

		len = 0;
		substr = strings[ i];
		while ( *substr) {
			if ( *substr == '\t') {
				int j = t-> tabIndent;
				while ( j--) n[ len++] = ' ';
			} else
				n[ len++] = *substr;
			substr++;
		}

		free( strings[ i]);
		n[len] = 0;
		strings[i] = n;
	}

	return true;
}

char **
Drawable_do_text_wrap( Handle self, TextWrapRec * t)
{
	char **ret = NULL;
	int bufsize = 128;

	float width[256];
	FontABC abc[256];
	unsigned int base = 0x10000000;

	int
		start = 0, utf_start = 0,
		split_start = -1, split_end = -1, 
		i, utf_p,
		utf_split = -1;
	float w = 0, inc = 0, initial_overhang = 0;
	Bool wasTab = 0, reassign_w = 1;
	Bool doWidthBreak = t-> width >= 0;
	int tildeIndex = -100, tildeLine = 0, tildePos = 0, tildeOffset = 0;
	int spaceWidth = 0, spaceC = 0, spaceOK = 0;

#define ADD(end, utfend)                                          \
	if ( !add_wrapped_text( t,                                \
		start, utf_start, end, utfend,                    \
		tildeIndex,                                       \
		&tildePos, &tildeLine,                            \
		&ret, &bufsize))                                  \
			return ret;                               \
	start     = end;                                          \
	utf_start = utfend;                                       \
	if (( t-> options & twReturnFirstLineLength) == twReturnFirstLineLength) return ret;\

#define NEW_WORD              \
	split_start = p;      \
	split_end   = i;      \
	utf_split   = utf_p;

#define NEW_LINE              \
	start = i;            \
	utf_start++;          \
	reassign_w = 1

	t-> count = 0;
	if (!( ret = allocn( char*, bufsize))) return NULL;

	/* determining ~ character location */
	if ( t-> options & twCalcMnemonic)
		tildeIndex = find_tilde_position(t);

	/* process UV chars */
	for ( i = 0, utf_p = 0; i < t-> textLen; utf_p++) {
		UV uv;
		float winc;
		int p = i;

		if ( t-> utf8_text) {
			STRLEN len;
			uv = prima_utf8_uvchr_end(t->text + i, t->text + t-> textLen, &len);
			i += len;
			if ( len == 0 )
				break;
		} else
			uv = (( unsigned char *)(t-> text))[i++];

		if ( uv / 256 != base)
			if ( !precalc_abc_buffer( query_abc_range( self, t, base = uv / 256), width, abc))
				return ret;

		if ( reassign_w) {
			w = initial_overhang = abc[ uv & 0xff]. a;
			reassign_w = 0;
		}

		switch ( uv ) {
		case '\t':
			NEW_WORD;
			if (!( t-> options & twCalcTabs))
				goto _default;

			if ( t-> options & twSpaceBreak) {
				ADD( p, utf_p);
				NEW_LINE;
				continue;
			}

			if ( !spaceOK) {
				PFontABC s;
				if ( !( s = query_abc_range( self, t, 0))) return ret;
				spaceOK    = 1;
				spaceC     = (s[' '].c < 0) ? - s[' ']. c : 0;
				spaceWidth = (s[' '].a + s[' '].b + s[' '].c) * t-> tabIndent;
			}
			winc   = spaceWidth;
			inc    = spaceC;
			wasTab = true;
			break;

		case '\n':
		case 0x2028:
		case 0x2029:
			NEW_WORD;
			if (!( t-> options & twNewLineBreak))
				goto _default;
			ADD( p, utf_p);
			NEW_LINE;
			continue;

		case ' ':
			NEW_WORD;
			if (!( t-> options & twSpaceBreak))
				goto _default;
			ADD( p, utf_p);
			NEW_LINE;
			continue;

		case '~':
			if ( p == tildeIndex ) {
				tildeOffset = w - initial_overhang;
				inc = winc = 0;
				break;
			}

		_default: default:
			winc = width[uv & 0xff];
			inc  = abc[uv & 0xff].c;
		}

		/* add next char */
		if ( !doWidthBreak || w + winc + inc <= t-> width) {
			w += winc;
			continue;
		}

		if (
			( p == start) || 
			(( p == start - 1) && ( p - 1 == tildeIndex))
		) {
			/* case when even single char cannot be fit in  */
			if ( t-> options & twBreakSingle) {
				/* do not return anything in this case */
				int j;
				if (!( t-> options & twReturnChunks)) {
					for ( j = 0; j < t-> count; j++) free( ret[ j]);
					ret[0] = duplicate_string("");
				}
				t-> count = 0;
				return ret;
			}

			/* or push this character disregarding the width */
			ADD(i, utf_p + 1);
			reassign_w = 1;
		} else {
			/* normal break condition */
			if ( t-> options & twWordBreak) {
				/* checking if break was at word boundary */
				if ( start <= split_start) {
					ADD(split_start, utf_split);
					i = start = split_end;
					utf_start = utf_split + 1;
					utf_p = utf_split;
					w = 0;
					reassign_w = 1;
					continue;
				} else if ( t-> options & twBreakSingle) {
					/* cannot be split, return nothing */
					int j;
					if (!( t-> options & twReturnChunks)) {
						for ( j = 0; j < t-> count; j++)
							free( ret[ j]);
						ret[0] = duplicate_string("");
					}
					t-> count = 0;
					return ret;
				}
			}

			/* repeat again */
			ADD(p, utf_p);
			i = start = p;
			utf_start = utf_p;
			utf_p--;
			reassign_w = 1;
		}
		w = 0;
	}

	/* adding or skipping last line */
	if ( t-> textLen - start > 0 || t-> count == 0)
		ADD(t-> textLen, t-> utf8_textLen);

	/* remove ~ and fill its location */
	t-> t_start = t-> t_end = C_NUMERIC_UNDEF;
	if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
		if ( t-> options & twCollapseTilde) {
			char * l = ret[tildeLine];
			memmove( l + tildePos, l + tildePos + 1, strlen(l) - tildePos);
		}
		fill_tilde_properties(self, t, tildeIndex, tildePos, tildeOffset);
	}

	if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab)
		expand_tabs( t, ret);

	return ret;
}
#undef ADD

static SV*
string_wrap( Handle self,SV * text, int width, int options, int tabIndent)
{
	gpARGS;
	TextWrapRec t;
	char** c;
	int i;
	AV * av;
	STRLEN tlen;

	t. text      = SvPV( text, tlen);
	t. utf8_text = prima_is_utf8_sv( text);
	if ( t. utf8_text) {
		t. utf8_textLen = prima_utf8_length( t. text);
		t. textLen = utf8_hop(( U8*) t. text, t. utf8_textLen) - (U8*) t. text;
	} else {
		t. utf8_textLen = t. textLen = tlen;
	}
	t. width     = ( width < 0) ? 0 : width;
	t. tabIndent = ( tabIndent < 0) ? 0 : tabIndent;
	t. options   = options;
	t. ascii     = &var-> font_abc_ascii;
	t. unicode   = &var-> font_abc_unicode;
	t. t_char    = NULL;

	gpENTER(
		(( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) ?
			newSViv(0) : newRV_noinc(( SV *) newAV())
	);
	c = Drawable_do_text_wrap( self, &t);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		IV rlen = 0;
		if ( c) {
			if ( t. count > 0) rlen = PTR2IV(c[1]);
			free( c);
		}
		return newSViv( rlen);
	}

	if ( !c) return nilSV;

	av = newAV();
	for ( i = 0; i < t. count; i++) {
		SV * sv = (t. options & twReturnChunks) ?
			newSViv( PTR2IV(c[i])) :
			newSVpv( c[ i], 0);
		if ( !(t. options & twReturnChunks)) {
			if ( t. utf8_text) SvUTF8_on( sv);
			free( c[i]);
		}
		av_push( av, sv);
	}

	free( c);

	if  ( t. options & ( twCalcMnemonic | twCollapseTilde)) {
		HV * profile = newHV();
		SV * sv_char;
		if ( t. t_char) {
			STRLEN len = t. utf8_text ? utf8_hop(( U8*) t. t_char, 1) - ( U8*) t. t_char : 1;
			sv_char = newSVpv( t. t_char, len);
			if ( t. utf8_text) SvUTF8_on( sv_char);
			if ( t. t_start != C_NUMERIC_UNDEF) pset_i( tildeStart, t. t_start);
			if ( t. t_end   != C_NUMERIC_UNDEF) pset_i( tildeEnd,   t. t_end);
			if ( t. t_line  != C_NUMERIC_UNDEF) pset_i( tildeLine,  t. t_line);
			if ( t. t_pos   != C_NUMERIC_UNDEF) pset_i( tildePos,   t. t_pos);
		} else {
			sv_char = newSVsv( nilSV);
			pset_sv( tildeStart, nilSV);
			pset_sv( tildeEnd,   nilSV);
			pset_sv( tildeLine,  nilSV);
			pset_sv( tildePos,   nilSV);
		}
		pset_sv_noinc( tildeChar, sv_char);
		av_push( av, newRV_noinc(( SV *) profile));
	}

	return newRV_noinc(( SV *) av);
}

typedef struct {
	uint16_t * glyphs;   /* glyphset to be wrapped */
	uint16_t * clusters; /* for visual ordering; also, won't break within a cluster */
	int        n_glyphs; /* glyphset length in words */
	int        width;    /* width to wrap with */
	int        options;  /* twXXX constants */
	int        count;    /* count of lines returned */
	PList    * cache;

	Byte     * glyphs_in_cluster;
	Byte     * chars_in_cluster;
	int      * l2v;
} GlyphWrapRec;

static Bool
add_wrapped_glyphs( GlyphWrapRec * t, unsigned int start, unsigned int end, unsigned int ** array, int * size)
{
	if ( t-> count == *size) {
		unsigned int * n;
		if ( !( n = (unsigned int *)realloc( *array, sizeof(unsigned int) * (*size *= 2))))
			return false;
		*array = n;
	}

	(*array)[t-> count++] = start;
	(*array)[t-> count++] = end - start;

	return true;
}

static PFontABC
query_abc_range_glyphs( Handle self, GlyphWrapRec * t, unsigned int base)
{
	PFontABC abc;

	if ( 
		*(t-> cache) && 
		(( abc = find_abc_in_list_cache( *(t->cache), base)) != NULL)
	)
		return abc;

	if ( !( abc = call_get_font_abc(self, base, toGlyphs)))
		return NULL;

	if ( !fill_abc_list_cache(t->cache, base, abc)) {
		free( abc);
		return NULL;
	}

	return abc;
}

/* returns character (not glyph!) mappings */
static unsigned int *
Drawable_do_glyphs_wrap( Handle self, GlyphWrapRec * t)
{
	unsigned int *ret = NULL;
	int size = 128;

	float width[256];
	FontABC abc[256];
	unsigned int base = 0x10000000;

	unsigned int start = 0, i, j;
	uint16_t max_cluster, min_cluster, text_length;
	float w = 0.0, inc = 0, initial_overhang = 0;
	Bool reassign_w = 1;
	Bool doWidthBreak = t-> width >= 0;

#define ADD(end) if (1) {                                        \
	if ( !add_wrapped_glyphs( t, start, end, &ret, &size))   \
		return ret;                                      \
	start = end;                                             \
	reassign_w = 1;                                          \
	if (( t-> options & twReturnFirstLineLength) == twReturnFirstLineLength) return ret;\
}

	t-> count = 0;
	if (!( ret = allocn( unsigned int, size))) return NULL;

	text_length = t->clusters[t->n_glyphs];
	min_cluster = max_cluster = t->clusters[0] & ~toRTL;
	for ( i = 0; i < t->n_glyphs; i++) {
		uint16_t c = t->clusters[i] & ~toRTL;
		if ( max_cluster < c ) max_cluster = c;
		if ( min_cluster > c ) min_cluster = c;
	}

	for ( i = min_cluster; i <= max_cluster; i++) {
		t->l2v[i] = -1;
		t->glyphs_in_cluster[i] = 0;
		t->chars_in_cluster[i] = 0;
	}
	for ( i = 0; i < t->n_glyphs; i++) {
		uint16_t c = t->clusters[i] & ~toRTL;
		if ( t->l2v[c] < 0 ) t->l2v[c] = i;
	}

	for ( i = j = min_cluster; i < max_cluster; i++) {
		if ( t->l2v[i] >= 0 ) j = i;
		t->chars_in_cluster[j]++;
	}
	t->chars_in_cluster[max_cluster] = text_length - max_cluster;

#ifdef _DEBUG
	printf("clusters:");
	for ( i = 0; i < t->n_glyphs; i++) {
		uint16_t c = t->clusters[i] & ~toRTL;
		printf(" %d", c);
	}
	printf("\n");
	printf("l2v(%d-%d):", min_cluster, max_cluster);
	for ( i = min_cluster; i <= max_cluster; i++) 
		printf(" %d", t->l2v[i]);
	printf("\n");
	printf("chars/cluster:");
	for ( i = min_cluster; i <= max_cluster; i++) 
		printf(" %d", t->chars_in_cluster[i]);
	printf("\n");
#endif

	/* process glyphs */
	for ( i = min_cluster; i <= max_cluster; ) {
		uint16_t cluster;
		float winc;
		int j, ng, p, v;

		v = t->l2v[i];

		/* ng: glyphs in the cluster */
		for ( ng = 0, cluster = t->clusters[v]; v < t->n_glyphs; v++) {
			if ( t->clusters[v] != cluster ) break;
			ng++;
		}
		t->glyphs_in_cluster[i] = ng;
		v -= ng;

		winc = 0;
		for ( j = 0; j < ng; j++) {
			uint16_t uv = t->glyphs[v + j];
			if ( uv / 256 != base)
				if ( !precalc_abc_buffer( query_abc_range_glyphs( self, t, base = uv / 256), width, abc)) {
					return ret;
				}
			if ( reassign_w) {
				w = initial_overhang = abc[ uv & 0xff]. a;
				reassign_w = 0;
			}
			winc += width[ uv & 0xff];
			if ( j == ng - 1 ) inc = abc[ uv & 0xff]. c;
		}

#ifdef _DEBUG
		printf("l:%d v:%d: ng:%d nc:%d cluster:%d\n", i, v, ng, t->chars_in_cluster[i], cluster & ~toRTL);
#endif
		p = i;
		i += t->chars_in_cluster[i];

		if ( !doWidthBreak || (w + winc + inc <= t-> width)) {
			w += winc;
			continue;
		}

		if ( p == start ) {
			/* case when even single char cannot be fit in  */
			if ( t-> options & twBreakSingle) {
				/* do not return anything in this case */
				t-> count = 0;
				return ret;
			}
			/* or push this character disregarding the width */
			ADD(i);
		} else {
			/* normal break condition */
			ADD(p);
			i = start = p;
		}

		w = 0;
	}

	/* adding or skipping last line */
	if ( text_length - start > 0 || t-> count == 0)
		ADD(text_length);

	return ret;
}
#undef ADD

static void
fill_ac( Handle self, GlyphWrapRec * t, uint16_t glyph1, uint16_t glyph2, uint16_t * advances)
{
	PFontABC abc;
	abc = query_abc_range_glyphs( self, t, glyph1 / 256);
	*(advances++) = ( abc[glyph1 & 0xff].a < 0 ) ? (-abc[glyph1 & 0xff].a + .5) : 0;
	if ( glyph1 != glyph2 )
		abc = query_abc_range_glyphs( self, t, glyph2 / 256);
	*(advances++) = ( abc[glyph1 & 0xff].c < 0 ) ? (-abc[glyph1 & 0xff].c + .5) : 0;
}

static SV*
glyphs_wrap( Handle self, SV * text, int width, int options)
{
	gpARGS;
	GlyphWrapRec t;
	AV * av;
	int i;
	unsigned int * c;
	GlyphsOutRec g;
	int l2v[MAX_CHARACTERS];
	Byte glyphs_in_cluster[MAX_CHARACTERS];
	Byte chars_in_cluster[MAX_CHARACTERS];

	if (!read_glyphs(&g, text, 1, "Drawable::text_wrap"))
		return nilSV;

	/* a very quick check, if possible, if glyphstr fits */
	if ( g.advances && ( width >= get_glyphs_width(&g))) {
		AV * av;
		if (( options & twReturnFirstLineLength) == twReturnFirstLineLength)
			return newSViv(g.len);
		av = newAV();
		if ( options & twReturnChunks) {
			av_push( av, newSViv(0));
			av_push( av, newSViv(g.len));
		} else {
			av_push( av, newSVsv(sv_call_perl(text, "clone", "<S", text)));
		}
		return newRV_noinc(( SV *) av);
	}

	t.n_glyphs  = g.len;
	t.glyphs    = g.glyphs;
	t.clusters  = g.clusters;
	t.width     = ( width < 0) ? 0 : width;
	t.options   = options;
	t.cache     = &var-> font_abc_glyphs;
	t.l2v       = l2v;
	t.glyphs_in_cluster = glyphs_in_cluster;
	t.chars_in_cluster  = chars_in_cluster;

	gpENTER(
		(( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) ?
			newSViv(0) : newRV_noinc(( SV *) newAV())
	);
	c = Drawable_do_glyphs_wrap( self, &t);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		IV rlen = 0;
		if ( c) {
			if ( t. count > 0) rlen = c[1];
			free( c);
		}
		return newSViv( rlen);
	}

	if ( !c)
		return nilSV;

	av = newAV();
	if ( options & twReturnChunks) {
		for ( i = 0; i < t.count; i++)
			av_push( av, newSViv( c[i]));
	} else {
		int j, k,
			mul[4] = { 1, 1, 1, 2 },
			extras[4] = {0, 1, 2, 0};
		uint16_t *payload[4] = { g.glyphs, g.clusters, g.advances, g.positions };
		for ( i = 0; i < t.count; i += 2) {
			SV *sv_payload[4];
			uint16_t * glyphs_dst = NULL;
			unsigned int
				glyph_size,
				char_offset = c[i], char_size = c[i + 1];
			if ( char_size == 0 ) {
				av_push( av, nilSV);
				continue;
			}

			glyph_size = 0;
			for ( k = char_offset, glyph_size = 0; k < char_offset + char_size; k++)
				glyph_size += glyphs_in_cluster[k];

			for ( j = 0; j < 4; j++) {
				SV * sv;
				uint16_t *dst;
				if ( payload[j] == NULL ) {
					sv_payload[j] = nilSV;
					continue;
				}

				sv  = prima_array_new(sizeof(uint16_t) * (glyph_size * mul[j] + extras[j]));
				dst = (uint16_t*)prima_array_get_storage(sv);
				if ( j == 0 ) glyphs_dst = dst;
				for ( k = char_offset; k < char_offset + char_size; k++) {
					if ( l2v[k] >= 0 ) {
						int l = glyphs_in_cluster[k] * mul[j];
						uint16_t * src = payload[j] + l2v[k] * mul[j];
						while (l-- > 0) *(dst++) = *(src++);
					}
				}
				if (j == 1) /* add fake sub-text length */
					*dst = char_size;
				else if (j == 2) /* add A and C advances */
					fill_ac(self, &t, glyphs_dst[0],glyphs_dst[glyph_size-1],dst);
				sv_payload[j] = prima_array_tie( sv, sizeof(uint16_t), "S");
			}
			av_push( av, newSVsv(
				call_perl(self, "new_glyph_obj", "<SSSS",
					sv_payload[0],
					sv_payload[1],
					sv_payload[2],
					sv_payload[3]
				)
			));
		}
	}
	free( c);

	return newRV_noinc(( SV *) av);
}

SV*
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent)
{

	if ( !SvROK( text )) {
		return string_wrap(self, text, width, options, tabIndent);
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		return glyphs_wrap(self, text, width, options);
	} else {
		SV * ret;
		gpARGS;
		gpENTER(
			(( options & twReturnFirstLineLength) == twReturnFirstLineLength) ?
				newSViv(0) : newRV_noinc(( SV *) newAV())
		);
		ret = newSVsv(sv_call_perl(text, "text_wrap", "<Hiii", self, width, options, tabIndent));
		gpLEAVE;
		return ret;
	}
}

#ifdef __cplusplus
}
#endif
