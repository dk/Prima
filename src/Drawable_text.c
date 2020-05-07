#include "apricot.h"
#include "Drawable.h"
#include "Application.h"

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
read_subarray( AV * av, int index, 
	int length_expected, int * plen, char * letter_expected,
	const char * caller, const char * subarray
) {
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

	if (*letter != *letter_expected) {
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
read_glyphs( PGlyphsOutRec t, SV * text, Bool indexes_required, const char * caller)
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

		if ( indexes_required ) {
			warn("%s requires glyphstr with indexes", caller);
			return false;
		}

		if ( !prima_array_parse( text, &ref, &length, &letter) || *letter != 'S') {
			warn("invalid glyphstr passed to %s: %s", caller, "not a Prima::array");
			return false;
		}

		t-> glyphs = ref;
		t-> len    = length;
		t-> text_len = 0;
		return true;
	}

	len = av_len(av) + 1;
	if ( len > 4 ) len = 4; /* we don't need more */
	if ( len < 1 || (len != 2 && len != 4) ) {
		warn("malformed glyphs array in %s", caller);
		return false;
	}

	if ( !( t-> glyphs = read_subarray( av, 0, -1, &t->len, "S", caller, "glyphs")))
		return false;
	if ( t->len == 0 )
		return true;

	switch ( len ) {
	case 4:
		if ( !( t-> positions = read_subarray( av, 3, t->len * 2, NULL, "s", caller, "positions")))
			return false;
		if ( !( t-> advances = read_subarray( av, 2, t->len, NULL, "S", caller, "advances")))
			return false;
	case 2:
		if ( !( t-> indexes = read_subarray( av, 1, t->len + 1, NULL, "S", caller, "indexes")))
			return false;
		t-> text_len = t-> indexes[t->len];
	}

	return true;
}

static int
check_length( int from, int len, int real_len )
{
	if ( len < 0 ) len = real_len;
	if ( from < 0 ) return 0;
	if ( from + len > real_len ) len = real_len - from;
	if ( len <= 0 ) return 0;
	return len;
}

char * 
hop_text(char * start, Bool utf8, int from)
{
	if ( !utf8 ) return start + from;
	while ( from-- ) start = (char*)utf8_hop((U8*)start, 1);
	return start;
}

void 
hop_glyphs(GlyphsOutRec * t, int from, int len)
{
	if ( from == 0 && len == t->len ) return;

	t->len      = len;
	t->glyphs  += from;

	if ( t-> indexes ) {
		int i, max_index = 0, next_index = t->indexes[t->len];
		t->indexes += from;
		for ( i = 0; i <= len; i++ ) {
			int ix = t->indexes[i] & ~toRTL;
			if ( max_index < ix ) max_index = ix;
		}
		for ( i = 0; i <= t->len; i++ ) {
			int ix = t->indexes[i] & ~toRTL;
			if (ix > max_index && ix < next_index) next_index = ix;
		}
		t->text_len = next_index;
	}

	if ( t->advances ) {
		t->advances  += from;
		t->positions += from * 2;
	}
}

Bool
Drawable_text_out( Handle self, SV * text, int x, int y, int from, int len)
{
	Bool ok;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		if ( utf8) dlen = prima_utf8_length(c_text, dlen);
		if ((len = check_length(from,len,dlen)) == 0)
			return true;
		c_text = hop_text(c_text, utf8, from);
		ok = apc_gp_text_out( self, c_text, x, y, len, utf8 ? toUTF8 : 0);
		if ( !ok) perl_error();
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!read_glyphs(&t, text, 0, "Drawable::text_out"))
			return false;
		if (t.len == 0)
			return true;
		if (( len = check_length(from,len,t.len)) == 0)
			return true;
		hop_glyphs(&t, from, len);
		ok = apc_gp_glyphs_out( self, &t, x, y);
		if ( !ok) perl_error();
	} else {
		SV * ret = sv_call_perl(text, "text_out", "<Hiiii", self, x, y, from, len);
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
sv2uint32( SV * text, int * size, int * flags)
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
		while (i-- > 0) *(dst++) = *(src++);
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
	int start, i, vis_len;
	Byte rtl;
	uint16_t index;
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
	run-> glyphs       = t->glyphs  + t-> n_glyphs;
	run-> indexes      = t->indexes + t-> n_glyphs;
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
shape_unicode(Handle self, PTextShapeRec t, PTextShapeFunc shaper,
	Bool glyph_mapper_only, Bool fribidi_arabic_shaping, Bool reorder
)
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
			printf("%d(%x) ", glyph_mapper_only ? -1 : run.indexes[i], run.glyphs[i]);
		printf("\n");
	}
#endif
		if (!ok) break;
		for (i = t->n_glyphs; i < t->n_glyphs + run.n_glyphs; i++) {
			int x = glyph_mapper_only ? i - t->n_glyphs : t->indexes[i];
			t->indexes[i] = t->v2l[x + run_offs];
		}
		run_offs += run_len;
#ifdef _DEBUG
	{
		int i;
		printf("indexes copied back: ");
		for (i = t->n_glyphs; i < t->n_glyphs + run.n_glyphs; i++)
			printf("%d ", t->indexes[i]);
		printf("\n");
	}
#endif
		t-> n_glyphs += run.n_glyphs;
	}
	t-> analysis = save_analysis;

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

Bool
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
	SV * ret = nilSV, 
		*sv_glyphs = nilSV,
		*sv_indexes = nilSV,
		*sv_positions = nilSV,
		*sv_advances = nilSV;
	PTextShapeFunc system_shaper;
	TextShapeRec t;
	int shaper_type, level = tsDefault;
	Bool skip_if_simple = false, return_zero = false, force_advances = false, reorder = true;
	Bool gp_enter;

	/* forward, if any */
	if ( SvROK(text_sv)) {
		SV * ref = newRV((SV*) profile);
		gpENTER(nilSV);
		ret = sv_call_perl(text_sv, "text_shape", "<HSS", self, text_sv, ref);
		gpLEAVE;
		hv_clear(profile); /* old gencls bork */
		sv_free(ref);
		return newSVsv(ret);
	}

	bzero(&t, sizeof(t));

	/* asserts */
#ifdef WITH_FRIBIDI
	if ( use_fribidi && sizeof(FriBidiLevel) != 1) {
		warn("sizeof(FriBidiLevel) != 1, fribidi is disabled");
		use_fribidi = false;
	}
#endif

	if ( pexist(rtl) ?
		pget_B(rtl) :
		(application ? PApplication(application)->textDirection : lang_is_rtl()
	))
		t.flags |= toRTL;
	if ( pexist(language))
		t.language = pget_c(language);
	if ( pexist(skip_if_simple))
		skip_if_simple = pget_B(skip_if_simple);
	if ( pexist(advances))
		force_advances = pget_B(advances);
	if ( pexist(reorder))
		reorder = pget_B(reorder);
	if ( pexist(level)) {
		level = pget_i(level);
		if ( level < tsNone || level > tsBytes ) level = tsFull;
	}
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
		gpENTER(nilSV);
		if (!( system_shaper = apc_gp_get_text_shaper(self, &shaper_type))) {
			return_zero = true;
			goto EXIT;
		}
	}

	if ( skip_if_simple && (shaper_type == tsNone)) {
		return_zero = true;
		goto EXIT;
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

	if ( t.len > MAX_CHARACTERS ) {
		warn("Drawable.text_shape: text too long, %dK max", MAX_CHARACTERS / 1024);
		t.len = MAX_CHARACTERS;
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
		sv_positions = sv_advances = nilSV;
		t.positions = NULL;
		t.advances = NULL;
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
		if ( is_simple ) {
			return_zero = true;
			goto EXIT;
		}
	}

	gpLEAVE;

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

#define BIND(x,n,d,letter) { \
	prima_array_truncate(x, (t.n_glyphs * n + d) * sizeof(uint16_t)); \
	x = sv_2mortal(prima_array_tie( x, sizeof(uint16_t), letter)); \
}
	BIND(sv_glyphs, 1, 0, "S");
	BIND(sv_indexes, 1, 1, "S");
	if ( sv_positions != nilSV ) BIND(sv_positions, 2, 0, "s");
	if ( sv_advances != nilSV ) BIND(sv_advances, 1, 0, "S");
#undef BIND

	return newSVsv(call_perl(self, "new_glyph_obj", "<SSSS",
		sv_glyphs, sv_indexes, sv_advances, sv_positions
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

	return return_zero ? newSViv(0) : nilSV;
}

static PFontABC
call_get_font_abc( Handle self, unsigned int from, unsigned int to, int flags);

static int
get_glyphs_width( Handle self, PGlyphsOutRec t, Bool add_overhangs)
{
	int i, ret;
	uint16_t * advances = t->advances;

	for ( i = ret = 0; i < t-> len; i++)
		ret += *(advances++);

	if ( add_overhangs ) {
		PFontABC abc;
		uint16_t glyph1 = t->glyphs[0], glyph2 = t->glyphs[t->len - 1];

		abc = call_get_font_abc( self, glyph1, glyph1, toGlyphs);
		if ( !abc ) return ret;
		ret += ( abc->a < 0 ) ? (-abc->a + .5) : 0;

		if ( glyph1 != glyph2 ) {
			free(abc);
			abc = call_get_font_abc( self, glyph2, glyph2, toGlyphs);
			if ( !abc ) return ret;
		}
		ret += ( abc->c < 0 ) ? (-abc->c + .5) : 0;
	}

	return ret;
}

int
Drawable_get_text_width( Handle self, SV * text, int flags, int from, int len)
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
		if (( len = check_length(from,len,dlen)) == 0)
			return 0;
		c_text = hop_text(c_text, flags & toUTF8, from);
		gpENTER(0);
		res = apc_gp_get_text_width( self, c_text, len, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!read_glyphs(&t, text, 0, "Drawable::get_text_width"))
			return false;
		if (t.len == 0)
			return true;
		if (( len = check_length(from,len,t.len)) == 0)
			return 0;
		hop_glyphs(&t, from, len);
		if (t.advances)
			return get_glyphs_width(self, &t, flags & toAddOverhangs);
		gpENTER(0);
		res = apc_gp_get_glyphs_width( self, &t);
		gpLEAVE;
	} else {
		SV * ret;
		gpENTER(0);
		ret = sv_call_perl(text, "get_text_width", "<Hiii", self, flags, from, len);
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
	pt[3].x = pt[2]. x = pt[4].x = get_glyphs_width(self, t, false);

	text_out_baseline = ( my-> textOutBaseline == Drawable_textOutBaseline) ?
		apc_gp_get_text_out_baseline(self) :
		my-> get_textOutBaseline(self);
	if ( !text_out_baseline ) {
		int i = 4, d = var->font. descent;
		while ( i--) pt[i]. y += d;
	}

	if ( var-> font. direction != 0) {
		int i;
#define GRAD 57.29577951
		float s = sin( var-> font. direction / GRAD);
		float c = cos( var-> font. direction / GRAD);
#undef GRAD
		for ( i = 0; i < 5; i++) {
			float x = pt[i]. x * c - pt[i]. y * s;
			float y = pt[i]. x * s + pt[i]. y * c;
			pt[i]. x = x + (( x > 0) ? 0.5 : -0.5);
			pt[i]. y = y + (( y > 0) ? 0.5 : -0.5);
		}
	}
}

SV *
Drawable_get_text_box( Handle self, SV * text, int from, int len )
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
		if ((len = check_length(from,len,dlen)) == 0)
			return newRV_noinc(( SV *) newAV());
		c_text = hop_text(c_text, flags & toUTF8, from);
		gpENTER( newRV_noinc(( SV *) newAV()));
		p = apc_gp_get_text_box( self, c_text, len, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		if (!read_glyphs(&t, text, 0, "Drawable::get_text_box"))
			return false;
		if (( len = check_length(from,len,t.len)) == 0)
			return newRV_noinc(( SV *) newAV());
		hop_glyphs(&t, from, len);
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
		ret = newSVsv(sv_call_perl(text, "get_text_box", "<Hii", self, from, len ));
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
call_get_font_abc( Handle self, unsigned int from, unsigned int to, int flags)
{
	PFontABC abc;

	/* query ABC information */
	if ( !self) {
		abc = apc_gp_get_font_abc( self, from, to, flags);
		if ( !abc) return NULL;
	} else if ( my-> get_font_abc == Drawable_get_font_abc) {
		gpARGS;
		gpENTER(NULL);
		abc = apc_gp_get_font_abc( self, from, to, flags);
		gpLEAVE;
		if ( !abc) return NULL;
	} else {
		SV * sv;
		int len = to - from + 1;
		if ( !( abc = malloc(len * sizeof( FontABC)))) return NULL;
		sv = my-> get_font_abc( self, from, to, flags);
		if ( SvOK( sv) && SvROK( sv) && SvTYPE( SvRV( sv)) == SVt_PVAV) {
			AV * av = ( AV*) SvRV( sv);
			int i, j = 0, n = av_len( av) + 1;
			if ( n > len * 3) n = len * 3;
			n = ( n / 3) * 3;
			if ( n < len) memset( abc, 0, len * sizeof( FontABC));
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
			memset( abc, 0, len * sizeof( FontABC));
		sv_free( sv);
	}

	return abc;
}

static PFontABC
call_get_font_abc_base( Handle self, unsigned int base, int flags)
{
	return call_get_font_abc( self, base * 256, base * 256 + 255, flags);
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

	if ( !( abc = call_get_font_abc_base(self, base, t-> utf8_text ? toUTF8 : 0)))
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
precalc_ac_buffer( PFontABC src, PFontABC dest)
{
	int i;
	if ( !dest) return false;
	for ( i = 0; i < 256; i++) {
		dest[i]. a = ( src[i]. a < 0) ? - src[i]. a : 0;
		dest[i]. c = ( src[i]. c < 0) ? - src[i]. c : 0;
	}
	return true;
}

static Bool
add_wrapped_text(
	TextWrapRec * t, 
	int start, int utfstart, int end, int utfend,
	int tildeIndex, int * tildePos, int * tildeCharPos, int * tildeLine,
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
		char * line = t-> text + start, *tilde_at = t->text + tildeIndex;
		*tildeCharPos = 0;
		while ( line < tilde_at ) {
			line = (char*)utf8_hop((U8*)line, 1);
			(*tildeCharPos)++;
		}
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

	for ( i = 0; i < t-> textLen - 1; i++) {
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
	}

	return tildeIndex;
}

static void
fill_tilde_properties(Handle self, TextWrapRec * t, int tildeIndex, int tildeCharPos, int tildeOffset)
{
	UV uv;
	int base;
	PFontABC abcc;
	float start, end;

	t-> t_pos  = tildeCharPos + (( t->options & twCollapseTilde ) ? 0 : 1);
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
	int tildeIndex = -100, tildeLine = 0, tildePos = 0, tildeCharPos = 0, tildeOffset = 0;
	int spaceWidth = 0, spaceC = 0, spaceOK = 0;

#define ADD(end, utfend)                                          \
	if ( !add_wrapped_text( t,                                \
		start, utf_start, end, utfend,                    \
		tildeIndex,                                       \
		&tildePos, &tildeCharPos, &tildeLine,             \
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
	if ( t-> textLen - start > 0 || t-> count == 0) {
		ADD(t-> textLen, t-> utf8_textLen);
	}

	/* remove ~ and fill its location */
	t-> t_start = t-> t_end = C_NUMERIC_UNDEF;
	if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
		if ( t-> options & twCollapseTilde) {
			char * l = ret[tildeLine];
			memmove( l + tildePos, l + tildePos + 1, strlen(l) - tildePos);
		}
		fill_tilde_properties(self, t, tildeIndex, tildeCharPos, tildeOffset);
	}

	if (( t-> options & twExpandTabs) && !(t-> options & twReturnChunks) && wasTab)
		expand_tabs( t, ret);

	return ret;
}
#undef ADD

static SV*
string_wrap( Handle self,SV * text, int width, int options, int tabIndent, int from, int len)
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
		t. utf8_textLen = prima_utf8_length( t. text, tlen);
		if (( t. utf8_textLen = check_length(from, len, t. utf8_textLen)) == 0)
			from = 0;
		t. text = hop_text(t.text, true, from);
		t. textLen = utf8_hop(( U8*) t. text, t. utf8_textLen) - (U8*) t. text;
	} else {
		if ((tlen = check_length(from, len, tlen)) == 0)
			from = 0;
		t. text = hop_text(t.text, false, from);
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
		if ( t. t_char) {
			STRLEN len = t. utf8_text ? utf8_hop(( U8*) t. t_char, 1) - ( U8*) t. t_char : 1;
			SV * sv_char = newSVpv( t. t_char, len);
			pset_sv_noinc( tildeChar, sv_char);
			if ( t.utf8_text) SvUTF8_on( sv_char);
			if ( t.t_start != C_NUMERIC_UNDEF) pset_i( tildeStart, t.t_start);
			if ( t.t_end   != C_NUMERIC_UNDEF) pset_i( tildeEnd,   t.t_end);
			if ( t.t_line  != C_NUMERIC_UNDEF) pset_i( tildeLine,  t.t_line);
			if ( t.t_pos   != C_NUMERIC_UNDEF) pset_i( tildePos,   t.t_pos);
		}
		av_push( av, newRV_noinc(( SV *) profile));
	}

	return newRV_noinc(( SV *) av);
}

typedef struct {
	uint16_t * glyphs;   /* glyphset to be wrapped */
	uint16_t * indexes;  /* for visual ordering; also, won't break within a cluster */
	uint16_t * advances;
	int16_t  * positions;
	int        n_glyphs; /* glyphset length in words */
	int        text_len; /* original index[-1] */
	int        width;    /* width to wrap with */
	int        options;  /* twXXX constants */
	int        count;    /* count of lines returned */
	PList    * cache;
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
#ifdef _DEBUG
	printf("add %d - %d\n", start, end);
#endif

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

	if ( !( abc = call_get_font_abc_base(self, base, toGlyphs)))
		return NULL;

	if ( !fill_abc_list_cache(t->cache, base, abc)) {
		free( abc);
		return NULL;
	}

	return abc;
}

static unsigned int *
Drawable_do_glyphs_wrap( Handle self, GlyphWrapRec * t)
{
	unsigned int *ret = NULL;
	int size = 128;
	float width[256];
	FontABC abc[256];
	unsigned int base = 0x10000000;

	unsigned int start, i;
	float w = 0.0, initial_overhang = 0;
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

#ifdef _DEBUG
		printf("rest(%d)\n", t->n_glyphs);
#endif

	/* process glyphs */
	for ( i = start = 0; i < t-> n_glyphs; ) {
		uint16_t index;
		float winc, inc = 0;
		int j, ng, p, v;

		/* ng: glyphs in the cluster */
		for ( ng = 0, v = i, index = t->indexes[v]; v < t->n_glyphs; v++) {
			if ( t->indexes[v] != index ) break;
			ng++;
		}

		winc = 0;
		for ( j = 0; j < ng; j++) {
			uint16_t uv = t->glyphs[i + j];
			if ( uv / 256 != base) {
				PFontABC labc;
				if ( !(labc = query_abc_range_glyphs( self, t, base = uv / 256)))
					return ret;
				if ( t-> advances )
					precalc_ac_buffer(labc, abc);
				else
					precalc_abc_buffer(labc, width, abc);
			}

			if ( reassign_w) {
				w = initial_overhang = abc[ uv & 0xff]. a;
				reassign_w = 0;
			}
			if ( t-> advances ) {
				winc += t->advances[i + j];
			} else {
				winc += width[ uv & 0xff];
			}
			if ( j == ng - 1 ) {
				inc = abc[ uv & 0xff]. c;
				if ( t-> advances ) 
					inc += t->positions[(i + j) * 2];
			}
		}


#ifdef _DEBUG
		printf("i:%d ng:%d inc:%f w:%f winc:%f\n", i, ng, inc, w, winc);
#endif
		p = i;
		i += ng;

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
	if ( t->n_glyphs - start > 0 || t-> count == 0) {
		ADD(t->n_glyphs);
	}

	return ret;
}
#undef ADD

static SV*
glyphs_wrap( Handle self, SV * text, int width, int options, int from, int len)
{
	gpARGS;
	GlyphWrapRec t;
	AV * av;
	int i;
	unsigned int * c;
	GlyphsOutRec g;

	if (!read_glyphs(&g, text, 1, "Drawable::text_wrap"))
		return nilSV;
	if ((len = check_length(from, len, g.len)) == 0)
		from = 0;
	hop_glyphs(&g, from, len);

	/* a very quick check, if possible, if glyphstr fits */
	if (
		(g.len == 0) ||
		(g.advances && ( width >= get_glyphs_width(self, &g, true)))
	) {
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
	t.indexes   = g.indexes;
	t.advances  = g.advances;
	t.positions = g.positions;
	t.width     = ( width < 0) ? 0 : width;
	t.text_len  = g.text_len;
	t.options   = options;
	t.cache     = &var-> font_abc_glyphs;

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
		int j,
			mul[4] = { 1, 1, 1, 2 },
			extras[4] = {0, 1, 0, 0};
		uint16_t *payload[4] = { g.glyphs, g.indexes, g.advances, (uint16_t*)g.positions };
		for ( i = 0; i < t.count; i += 2) {
			SV *sv_payload[4];
			int offset = c[i], size = c[i + 1];
			for ( j = 0; j < 4; j++) {
				SV * sv;
				uint16_t *dst, *src, l;
				if ( payload[j] == NULL ) {
					sv_payload[j] = nilSV;
					continue;
				}

				sv  = prima_array_new(sizeof(uint16_t) * (size * mul[j] + extras[j]));
				dst = (uint16_t*)prima_array_get_storage(sv);
				l = size * mul[j];
				src = payload[j] + offset * mul[j];
				while (l-- > 0) *(dst++) = *(src++);
				if (j == 1) {/* add fake sub-text length */
					int i, diff = g.indexes[g.len], 
						char_offset = g.indexes[offset + size - 1] & ~toRTL;
					for ( i = 0; i < g.len; i++) {
						int co = g.indexes[i] & ~toRTL;
						if ( co > char_offset && diff > co - char_offset  ) 
							diff = co - char_offset;
					}
					*dst = diff;
				}
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
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent, int from, int len)
{
	if ( !SvROK( text )) {
		return string_wrap(self, text, width, options, tabIndent, from, len);
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		return glyphs_wrap(self, text, width, options, from, len);
	} else {
		SV * ret;
		gpARGS;
		gpENTER(
			(( options & twReturnFirstLineLength) == twReturnFirstLineLength) ?
				newSViv(0) : newRV_noinc(( SV *) newAV())
		);
		ret = newSVsv(sv_call_perl(text, "text_wrap", "<Hiiiii", self, width, options, tabIndent, from, len));
		gpLEAVE;
		return ret;
	}
}

#ifdef __cplusplus
}
#endif
