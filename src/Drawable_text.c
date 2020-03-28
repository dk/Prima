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
		int n_glyphs;
		void * glyphs;
		Bool do_free;
	
		if ( !( glyphs = prima_read_array(
			text, "Drawable::text_out",
			's', 1, 1, -1, &n_glyphs, &do_free)))
			return false;

		ok = apc_gp_text_out( self, (char*)glyphs, x, y, n_glyphs, toGlyphs);
		if ( !ok) perl_error();
		if ( do_free ) free(glyphs);
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
		*flags = toUTF8;
		*size = prima_utf8_length(src);
	} else {
		*flags = 0;
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

/* knows only strong RTL characters and doesn't know about unicode bidi */
static void 
fallback_analysis( uint32_t * text, int len, Byte * analysis)
{
	int i;
	for ( i = 0; i < len; i++) {
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
	}
}

/* initialize clusters */
static void
apply_identity_to_clusters(PTextShapeRec t)
{
	register int i = 0;
	register uint16_t * c = t-> clusters;
	while (i < t->len) *(c++) = i++;
}

static __INLINE__ void
inverse_cluster_chunk(register uint16_t * start, register uint16_t * end)
{
	while ( start < end ) {
		register uint16_t c = *start;
		*(start++) = *end;
		*(end--) = c;
	}
}

/* inverse RTL portions of clusters, 12CAR34 => 12RAC34 */
static void
apply_rtl_to_clusters(PTextShapeRec t)
{
	int i, start;
	Byte rtl = t-> analysis[0];
	for ( i = 1, start = 0; i < t-> len + 1; i++) {
		if ( i < t-> len && (t-> analysis[i] == rtl)) continue;
		if ( rtl & 1 )
			inverse_cluster_chunk(t->clusters + start, t->clusters + i - 1);
		if ( i < t-> len )
			rtl = t->analysis[i];
		start = i;
	}
}

#define FILTER_BIDI_MARKS \
	(c >= 0x200e && c <= 0x200f) || /* dir marks */ \
	(c >= 0x202a && c <= 0x202e) || /* embedding */ \
	(c >= 0x2066 && c <= 0x2069)    /* isolates */


/* remove known bidi marks from clusters, so that 12<PDF>3 becomes 123 */
static int
apply_bidi_to_clusters(PTextShapeRec t)
{
	int i, j;
	for ( i = j = 0; i < t->len; i++) {
		register uint32_t c = t->text[t->clusters[i]];
		if ( FILTER_BIDI_MARKS ) continue;
		t->clusters[j++] = t->clusters[i];
	}

	return j;
}

/*
create sub-record for individual shape run, aliasing it partly onto the
parent record, so that shaper fills glyphs directly there. Clusters though
are needed to addres manually - for glyph_mapper_only shapers these are
unneeded, as they mean char->shape 1:1 mapping anyway. For real shapers
caller needs to allocate a separate cluster buf that will be resolved 
individually by apply_run_results() (see below)
*/

static Bool
alloc_run( PTextShapeRec t, int visual_start, int visual_len, PTextShapeRec run)
{
	int i;
	uint32_t *chars;

	bzero(run, sizeof(TextShapeRec));
	if ( !( run-> text = warn_malloc(sizeof(uint32_t) * visual_len)))
		return false;
	for ( i = 0, chars = run-> text; i < visual_len; i++)
		*(chars++) = t->text[ t->clusters[i + visual_start] ];
	run-> len = visual_len;

	run-> flags        = (t->analysis[t->clusters[visual_start]] & 1) ? toRTL : 0;
	run-> n_glyphs_max = t->n_glyphs_max - t-> n_glyphs;
	run-> glyphs       = t->glyphs   + t-> n_glyphs;
	if ( t-> coords )
		run-> coords = t->coords + t-> n_glyphs * 2;
	if ( t-> advances )
		run-> advances = t->advances + t-> n_glyphs;

	return true;
}

/*

Applies cluster map provided by shaper onto cluster map calculated here
either by us or fribidi:

   orig text           : L1 L2 NONVISUAL R1 R2
   clusters            :  0  1  3  4

   text for shaper.1   > L1 L2
            map        : 0 1
   shaper.1 litages Ls < L12
      returns clusters < 0
	applied back   : 0

   text for shaper.2   > R1 R2 - don't invert, shaper does this
            map        : 3 4
   shaper.2 litages Rs < R21
          run clusters < 0
	applied back   : 3

*/

static void
apply_run_results(
	PTextShapeRec src,
	uint16_t * map,
	PTextShapeRec dst
) {
	int i;
	register uint16_t 
		*dst_c = dst->clusters + dst->n_glyphs,
		*src_c = src->clusters;
	for ( i = 0; i < src->n_glyphs; i++) {
		*(dst_c++) = map[ *(src_c++) ];
		dst->n_glyphs++; /* glyphs are already copied */
	}
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
	int n, rtl,
		start = r->i,
		last_cluster = t->clusters[r->i],
		dir = 0;

	if ( r->i >= r->vis_len ) return 0;

	rtl = t->analysis[t->clusters[r->i]];
	for ( n = 0; r->i < r->vis_len + 1; r->i++, n++) {
		if (
			r->i >= r->vis_len ||                                  /* eol */
			(t-> analysis[t->clusters[r->i]] != rtl) || (    /* rtl != ltr */
				n > 1 &&
				t->clusters[r->i] != last_cluster + dir        /* cluster break */
			)
		) {
			r->rtl = rtl;
			return r->i - start;
		}

		/* determine cluster direction */
		if ( n == 1 ) {
			dir = t->clusters[r->i] - last_cluster;
			if ( dir != 1 && dir != -1 ) {
				r->rtl = rtl;
				return r->i - start;
			}
		}
		last_cluster = t->clusters[r->i];
	}

	return 0;
}

typedef Bool DrawableShaperFunc(Handle self, PTextShapeRec t, PTextShapeFunc shaper);

/* minimal bidi and unicode processing, map only glyphs, no coordinates */
static Bool
shape_map_glyphs(Handle self, PTextShapeRec t, PTextShapeFunc shaper)
{
	Bool ok;
	int n_chars_after_bidi;
	TextShapeRec run;

	fallback_analysis(t->text, t->len, t->analysis);
	apply_identity_to_clusters(t);
	apply_rtl_to_clusters(t);
	n_chars_after_bidi = apply_bidi_to_clusters(t);

	if ( !alloc_run(t, 0, n_chars_after_bidi, &run))
		return false;
	ok = shaper( self, &run );
	t->n_glyphs += run.n_glyphs;

	free(run.text);

	return ok;
}

/* call real shaper with minimal bidi prefiltering */
static Bool
shape_apc(Handle self, PTextShapeRec t, PTextShapeFunc shaper)
{
	Bool ok = false;
	int run_offs, run_len;
	BidiRunRec brr;
	uint16_t *run_clusters;
	TextShapeRec tmp;

	fallback_analysis(t->text, t->len, t->analysis);

	tmp = *t;
	/*
		t.clusters accumulates final clusters,
		tmp.clusters contains clusters after rtl and bidi processing
		run_clusters accepts shaper output:
		t = tmp(run)
	*/
	if ( !( tmp.clusters = malloc( sizeof(uint16_t) * tmp.n_glyphs_max * 2)))
		return false;
	run_clusters = tmp.clusters + tmp.n_glyphs_max;

	apply_identity_to_clusters(&tmp);
	tmp.len = apply_bidi_to_clusters(&tmp);

//#define _DEBUG
#ifdef _DEBUG
	{
		int i;
		printf("tmp: ");
		for (i = 0; i < tmp.len; i++) {
			printf("%d(%x) ", tmp.clusters[i], tmp.text[tmp.clusters[i]]);
		}
		printf("\n");
	}
#endif

	run_offs = 0;
	run_init(&brr, tmp.len);
	while (( run_len = run_next(&tmp, &brr)) > 0) {
		TextShapeRec run;

		if ( !alloc_run(&tmp, run_offs, run_len, &run))
			goto EXIT;

		run.clusters = run_clusters;
#ifdef _DEBUG
	{
		int i;
		printf("shaper input %s %d - %d: ", (run.flags & toRTL) ? "rtl" : "ltr", run_offs, run_offs + run_len - 1);
		for (i = 0; i < run.len; i++) {
			printf("%x ", run.text[i]);
		}
		printf("\n");
	}
#endif
		ok = shaper( self, &run );
#ifdef _DEBUG
	{
		int i;
		printf("shaper output: ");
		for (i = 0; i < run.n_glyphs; i++) {
			printf("%d(%d) ", run.clusters[i], run.glyphs[i]);
		}
		printf("\n");
	}
#endif
		free(run.text);
		if ( !ok ) goto EXIT;

		apply_run_results( &run, tmp.clusters + run_offs, t );
		tmp.n_glyphs += run.n_glyphs;
#ifdef _DEBUG
	{
		int i;
		printf("copied back @ %d: ", t->n_glyphs - run.n_glyphs);
		for (i = t->n_glyphs - run.n_glyphs; i < t->n_glyphs; i++) {
			printf("%d(%d) ", t->clusters[i], t->glyphs[i]);
		}
		printf("\n");
	}
#endif
		run_offs += run_len;
	}

EXIT:
	free( tmp.clusters );
	return ok;
}

/* call glyph mapper with fribidi prefiltering and shaping */
static Bool
shape_bidi_full(Handle self, PTextShapeRec t, PTextShapeFunc shaper)
{
	Bool ok;
	Byte *buf, *ptr;
	int i, sz, mlen, n_visual;
	uint32_t *visual;
	uint16_t *clusters;
	FriBidiFlags _flags = FRIBIDI_FLAGS_DEFAULT | FRIBIDI_FLAGS_ARABIC;
  	FriBidiParType   base_dir;
	FriBidiCharType* types;
	FriBidiArabicProp* ar;
	FriBidiStrIndex* map;
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
	map      = (FriBidiStrIndex*)   (ptr += mlen * sizeof(FriBidiArabicProp));
#if FRIBIDI_INTERFACE_VERSION > 3
	bracket_types = (FriBidiBracketType*) (ptr += mlen * sizeof(FriBidiStrIndex));
#endif
	fribidi_get_bidi_types(t->text, t->len, types);
	fribidi_get_joining_types(t->text, t->len, ar);
	/* XXX FRIBIDI_PAR_ON ? */
	base_dir = ( t->flags & toRTL ) ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR;

#if FRIBIDI_INTERFACE_VERSION > 3
	fribidi_get_bracket_types(t->text, t->len, types, bracket_types);
	fribidi_get_par_embedding_levels_ex(
		types, bracket_types, t->len, &base_dir,
		(FriBidiLevel*)t->analysis);
#else
	fribidi_get_par_embedding_levels(
		types, t->len, &base_dir,
		(FriBidiLevel*)t->analysis);
#endif

	fribidi_get_joining_types(t->text, t->len, ar);
	fribidi_join_arabic(types, t->len, (FriBidiLevel*)t->analysis, ar);
	fribidi_shape(_flags, (FriBidiLevel*)t->analysis, t->len, ar, t->text);
	for ( i = 0; i < t->len; i++)
		map[i] = i;
    	fribidi_reorder_line(_flags, types, t->len, 0, 
		base_dir, (FriBidiLevel*)t->analysis, t->text, map);

	/* from fribidi_remove_bidi_marks: */
	for (
		i = n_visual = 0, visual = t->text, clusters = t->clusters;
		i < t->len;
		i++
	) {
		register uint32_t c = t->text[i];
		if ( FILTER_BIDI_MARKS ) continue;
		*(visual++) = t->text[i];
		n_visual++;
		*(clusters++) = map[i];
	}
	t-> len = n_visual;

	ok = shaper( self, t );
	free(buf);
	return ok;
}

/* call glyph shaper with fribidi prefiltering but without shaping */
static Bool
shape_bidi_and_apc(Handle self, PTextShapeRec t, PTextShapeFunc shaper)
{
	Bool ok;
	Byte *buf, *ptr;
	int i, sz, mlen;
	FriBidiFlags _flags = FRIBIDI_FLAGS_DEFAULT;
  	FriBidiParType   base_dir;
	FriBidiCharType* types;
	FriBidiStrIndex* v2l;
#if FRIBIDI_INTERFACE_VERSION > 3
	FriBidiBracketType* bracket_types;
#endif
	int run_offs, run_len;
	BidiRunRec brr;
	TextShapeRec tmp;
	uint16_t *run_clusters, *l2v;

	mlen = sizeof(void*) * ((t->len / sizeof(void*)) + 1); /* align pointers */
	sz = mlen * (
		sizeof(FriBidiCharType) + 
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
	v2l      = (FriBidiStrIndex*)   (ptr += mlen * sizeof(FriBidiCharType));
#if FRIBIDI_INTERFACE_VERSION > 3
	bracket_types = (FriBidiBracketType*) (ptr += mlen * sizeof(FriBidiStrIndex));
#endif
	fribidi_get_bidi_types(t->text, t->len, types);
	/* XXX FRIBIDI_PAR_ON ? */
	base_dir = ( t->flags & toRTL ) ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR;

#if FRIBIDI_INTERFACE_VERSION > 3
	fribidi_get_bracket_types(t->text, t->len, types, bracket_types);
	fribidi_get_par_embedding_levels_ex(
		types, bracket_types, t->len, &base_dir,
		(FriBidiLevel*)t->analysis);
#else
	fribidi_get_par_embedding_levels(
		types, t->len, &base_dir,
		(FriBidiLevel*)t->analysis);
#endif

#ifdef _DEBUG
	printf("\n%s input: ", (t->flags & toRTL) ? "RTL" : "LTR");
	for ( i = 0; i < t->len; i++) 
		printf("%x ", t->text[i]);
	printf("\n");
#endif

	for ( i = 0; i < t->len; i++)
		v2l[i] = i;
    	fribidi_reorder_line(_flags, types, t->len, 0, 
		base_dir, (FriBidiLevel*)t->analysis, t->text, v2l);
	/*
		t.clusters accumulates final clusters,
		v2l -> tmp.clusters contains clusters after rtl and bidi processing
		run_clusters accepts shaper output:
		t = tmp(run)
	*/
	tmp = *t;
	if ( !( tmp.clusters = malloc( sizeof(uint16_t) * tmp.n_glyphs_max * 4))) {
		free(buf);
		return false;
	}

	run_clusters = tmp.clusters + tmp.n_glyphs_max;
	tmp.analysis = (Byte*)(run_clusters + tmp.n_glyphs_max);
	l2v          = run_clusters + tmp.n_glyphs_max * 2;

	bzero(l2v, t->len);
	for ( i = 0; i < t->len; i++)
		l2v[v2l[i]] = i;

#ifdef _DEBUG
	printf("v2l: ");
	for ( i = 0; i < t->len; i++) 
		printf("%d ", v2l[i]);
	printf("\n");
	printf("l2v: ");
	for ( i = 0; i < t->len; i++) 
		printf("%d ", l2v[i]);
	printf("\n");
	printf("%s visual: ", (t->flags & toRTL) ? "RTL" : "LTR");
	for ( i = 0; i < t->len; i++) 
		printf("%x ", t->text[i]);
	printf("\n");
#endif

	for (
		i = tmp.len = 0, tmp.text = t->text;
		i < t->len;
		i++
	) {
		register uint32_t c = t->text[i];
		if ( FILTER_BIDI_MARKS ) continue;
		tmp.text[tmp.len] = t->text[i];
		tmp.clusters[tmp.len] = tmp.len;
		tmp.analysis[tmp.len] = t->analysis[l2v[i]];
		v2l[tmp.len] = v2l[i];
		tmp.len++;
	}

#ifdef _DEBUG
	printf("analysis: ");
	for ( i = 0; i < t->len; i++) 
		printf("%d ", tmp.analysis[i]);
	printf("\n");
	printf("tmp: ");
	for (i = 0; i < tmp.len; i++) {
		printf("%d(%x)%s ", tmp.clusters[i], tmp.text[tmp.clusters[i]], (tmp.analysis[tmp.clusters[i]] & 1) ? "R" : "L");
	}
#endif

	apply_rtl_to_clusters(&tmp);

#ifdef _DEBUG
	{
		int i;
		printf("after rtl: ");
		for (i = 0; i < tmp.len; i++) {
			printf("%d(%x) ", tmp.clusters[i], tmp.text[tmp.clusters[i]]);
		}
		printf("\n");
	}
#endif

	run_offs = 0;
	run_init(&brr, tmp.len);
	while (( run_len = run_next(&tmp, &brr)) > 0) {
		TextShapeRec run;

		if ( !alloc_run(&tmp, run_offs, run_len, &run)) {
			free(tmp.clusters);
			free(buf);
			return false;
		}
		run.clusters = run_clusters;
#ifdef _DEBUG
	{
		int i;
		printf("shaper input %s %d - %d: ", (run.flags & toRTL) ? "rtl" : "ltr", run_offs, run_offs + run_len - 1);
		for (i = 0; i < run.len; i++) {
			printf("%x ", run.text[i]);
		}
		printf("\n");
	}
#endif
		ok = shaper( self, &run );
#ifdef _DEBUG
	{
		int i;
		printf("shaper output: ");
		for (i = 0; i < run.n_glyphs; i++) {
			printf("%d(%d) ", run.clusters[i], run.glyphs[i]);
		}
		printf("\n");
	}
#endif
		free(run.text);
		if (!ok) break;

		{
			int i;
			register uint16_t 
				*dst_c = t->clusters + t->n_glyphs,
				*src_c = run.clusters;
			for ( i = 0; i < run.n_glyphs; i++) {
				int index = 
					( run.flags & toRTL ) ?
						run_offs + run_len - *(src_c++) - 1 :
						*(src_c++) + run_offs;
#ifdef _DEBUG
				printf("m[%x + %d -> %d] => %x\n", *(src_c-1), run_offs, index, v2l[index]);
#endif
				*(dst_c++) = v2l[index];
				t->n_glyphs++; /* glyphs are already copied */
			}
		}
		tmp.n_glyphs += run.n_glyphs;
#ifdef _DEBUG
	{
		int i;
		printf("copied back @ %d: ", t->n_glyphs - run.n_glyphs);
		for (i = t->n_glyphs - run.n_glyphs; i < t->n_glyphs; i++) {
			printf("%d(%d) ", t->clusters[i], t->glyphs[i]);
		}
		printf("\n");
	}
#endif
		run_offs += run_len;
	}

	free(tmp.clusters);
	free(buf);
	return ok;
}

static DrawableShaperFunc*
find_shaper(Bool glyph_mapper_only)
{
#ifdef WITH_FRIBIDI
	if ( use_fribidi ) {
		if ( glyph_mapper_only ) 
			return shape_bidi_full;
		else
			return shape_bidi_and_apc;
	} else
#endif
		if ( glyph_mapper_only ) 
			return shape_map_glyphs;
		else
			return shape_apc;
}

SV*
Drawable_text_shape( Handle self, SV * text_sv, HV * profile)
{
	dPROFILE;
	gpARGS;
	int i;
	SV * ret = nilSV, *sv_glyphs, *sv_clusters, *sv_coords, *sv_advances;
	PTextShapeFunc system_shaper;
	DrawableShaperFunc* drawable_shaper;
	TextShapeRec t;
	Bool glyph_mapper_only;

	/* forward, if any */
	if ( SvROK(text_sv)) {
		SV * ref = newRV_noinc((SV*) profile);
		gpENTER(nilSV);
		ret = sv_call_perl(text_sv, "text_shape", "<HSS", self, text_sv, ref);
		gpLEAVE;
		sv_free(ref);
		return ret;
	}

	/* asserts */
#ifdef WITH_FRIBIDI
	if ( use_fribidi && sizeof(FriBidiLevel) != 1) {
		warn("sizeof(FriBidiLevel) != 1, fribidi is disabled");
		use_fribidi = false;
	}
#endif

	/* font supports shaping? */
	if (!( system_shaper = apc_gp_text_get_shaper(self, &glyph_mapper_only)))
		return newSViv(0);
	drawable_shaper = find_shaper(glyph_mapper_only);

	/* allocate buffers */
	bzero(&t, sizeof(t));
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

	/* MSDN, on ScriptShape: A reasonable value is (1.5 * cChars + 16) */
	t.n_glyphs_max = t.len * 2 + 16; 
#define ALLOC(id,n) { \
	sv_##id = prima_array_new( t.n_glyphs_max * n * sizeof(uint16_t)); \
	t.id   = (uint16_t*) prima_array_get_storage(sv_##id); \
}
	
	ALLOC(glyphs,1);
	ALLOC(clusters,1);
	if ( glyph_mapper_only ) {
		sv_coords = sv_advances = nilSV;
	} else {
		ALLOC(coords,2);
		ALLOC(advances,1);
	}
#undef ALLOC

	if ( pexist(rtl) && pget_B(rtl)) t.flags |= toRTL;
		
	if ( !drawable_shaper(self, &t, system_shaper) ) goto EXIT;

	/* encode direction */
	for ( i = 0; i < t.n_glyphs; i++) {
		if ( t.analysis[ t.clusters[i] ] & 1 )
			t.clusters[i] |= toRTL;
	}

	free(t.analysis );
	free(t.text);
	t.analysis = NULL;
	t.text = NULL;

#define BIND(x,n) { \
	prima_array_truncate(x, t.n_glyphs * n * sizeof(uint16_t)); \
	x = prima_array_tie( x, sizeof(uint16_t), "S"); \
}
	BIND(sv_glyphs, 1);
	BIND(sv_clusters, 1);
	if ( sv_coords != nilSV ) BIND(sv_coords, 2);
	if ( sv_advances != nilSV ) BIND(sv_advances, 1);
#undef BIND

	return newSVsv(call_perl(self, "new_glyph_obj", "<SSSS",
		sv_glyphs, sv_clusters, sv_advances, sv_coords
	));

EXIT:
	if ( t.text     ) free(t.text     );
	if ( t.analysis ) free(t.analysis );
	if ( t.clusters ) sv_free(sv_clusters );
	if ( t.glyphs   ) sv_free(sv_glyphs   );
	if ( t.coords   ) sv_free(sv_coords   );
	if ( t.advances ) sv_free(sv_advances );

	return nilSV;
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
		int n_glyphs;
		void * glyphs;
		Bool do_free;

		gpENTER(0);
		if ( !( glyphs = prima_read_array(
			text, "Drawable::get_text_width",
			's', 1, 1, -1, &n_glyphs, &do_free))) {
			gpLEAVE;
			return false;
		}

		flags &= ~toUTF8;
		res = apc_gp_get_text_width( self, (char*) glyphs, n_glyphs, flags | toGlyphs );
		gpLEAVE;
		if ( do_free ) free(glyphs);
	} else {
		SV * ret;
		gpENTER(0);
		ret = sv_call_perl(text, "get_text_width", "<Hi", self, flags);
		gpLEAVE;
		res = (ret && SvOK(ret)) ? SvIV(ret) : 0;
	}

	return res;
}

SV *
Drawable_get_text_box( Handle self, SV * text, int flags )
{
	gpARGS;
	Point * p;
	AV * av;
	int i;
	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);

		if ( prima_is_utf8_sv( text)) {
			dlen = utf8_length(( U8*) c_text, ( U8*) c_text + dlen);
			flags |= toUTF8;
		} else
			flags &= ~toUTF8;

		gpENTER( newRV_noinc(( SV *) newAV()));
		p = apc_gp_get_text_box( self, c_text, dlen, flags);
		gpLEAVE;
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		int n_glyphs;
		void * glyphs;
		Bool do_free;
	
		gpENTER( newRV_noinc(( SV *) newAV()));
		if ( !( glyphs = prima_read_array(
			text, "Drawable::get_text_box",
			's', 1, 1, -1, &n_glyphs, &do_free))) {
			gpLEAVE;
			return false;
		}

		flags &= ~toUTF8;
		p = apc_gp_get_text_box( self, (char*) glyphs, n_glyphs, flags | toGlyphs );
		gpLEAVE;

		if ( do_free ) free(glyphs);
	} else {
		SV * ret;
		gpENTER( newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "get_text_box", "<H", self));
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
	int tildeIndex, int * tildePos, int * tildeLPos, int * tildeLine,
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
		*tildePos = *tildeLPos = tildeIndex - start;
		if ( tildeIndex == end - 1) {
			t-> t_line++;
			tildeLPos = 0;
		}
	}

	if ( t-> count == *size) {
		char ** n;
		if ( !( n = (char **) realloc( *array, *size *= 2)))
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
fill_tilde_properties(Handle self, TextWrapRec * t, int tildePos, int tildeOffset)
{
	UV uv;
	int base;
	PFontABC abcc;
	float start, end;

	t-> t_char = t-> text + tildePos + 1;
	if ( t-> utf8_text) {
		STRLEN len;
		prima_utf8_uvchr_end(t-> t_char, t->text + t-> textLen, &len);
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
	int tildeIndex = -100, tildeLPos = 0, tildeLine = 0, tildePos = 0, tildeOffset = 0;
	int spaceWidth = 0, spaceC = 0, spaceOK = 0;

#define ADD(end, utfend)                                          \
	if ( !add_wrapped_text( t,                                \
		start, utf_start, end, utfend,                    \
		tildeIndex,                                       \
		&tildePos, &tildeLPos, &tildeLine,                \
		&ret, &bufsize))                                    \
			return ret;                               \
	start     = end;                                          \
	utf_start = utfend;                                       \
	if ( t-> options & twReturnFirstLineLength) return ret;

#define ADVANCE               \
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
			ADVANCE;	
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
			ADVANCE;	
			if (!( t-> options & twNewLineBreak))
				goto _default;
			ADD( p, utf_p);
			NEW_LINE;
			continue;

		case ' ':
			ADVANCE;	
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
		}
		w = 0;
	}

	/* adding or skipping last line */
	if ( t-> textLen - start > 0 || t-> count == 0)
		ADD(t-> textLen, t-> utf8_textLen);

	/* remove ~ and fill its location */
	t-> t_start = t-> t_end = t-> t_line = C_NUMERIC_UNDEF;
	if ( tildeIndex >= 0 && !(t-> options & twReturnChunks)) {
		if ( t-> options & twCollapseTilde) {
			char * l = ret[tildeLine];
			memmove( l + tildePos, l + tildePos + 1, strlen(l) - tildePos);
		}
		fill_tilde_properties(self, t, tildePos, tildeOffset);
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

	gpENTER( (options & twReturnFirstLineLength) ? newSViv(0) : newRV_noinc(( SV *) newAV()));
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
		} else {
			sv_char = newSVsv( nilSV);
			pset_sv( tildeStart, nilSV);
			pset_sv( tildeEnd,   nilSV);
			pset_sv( tildeLine,  nilSV);
		}
		pset_sv_noinc( tildeChar, sv_char);
		av_push( av, newRV_noinc(( SV *) profile));
	}

	return newRV_noinc(( SV *) av);
}

typedef struct {
	uint16_t * glyphs;   /* glyphset to be wrapped */
	int        n_glyphs; /* glyphset length in words */
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

static unsigned int *
Drawable_do_glyphs_wrap( Handle self, GlyphWrapRec * t)
{
	unsigned int *ret = NULL;
	int size = 128;

	float width[256];
	FontABC abc[256];
	unsigned int base = 0x10000000;

	unsigned int start = 0, i;
	float w = 0, inc = 0, initial_overhang = 0;
	Bool reassign_w = 1;
	Bool doWidthBreak = t-> width >= 0;

#define ADD(end)                                                 \
	if ( !add_wrapped_glyphs( t, start, end, &ret, &size))   \
		return ret;                                      \
	start = end;                                             \
	if (t-> options & twReturnFirstLineLength)               \
		return ret;

	t-> count = 0;
	if (!( ret = allocn( unsigned int, size))) return NULL;

	/* process glyphs */
	for ( i = 0; i < t-> n_glyphs; i++) {
		uint16_t uv;
		float winc;
		int p = i;

		uv = t-> glyphs[i];
		if ( uv / 256 != base)
			if ( !precalc_abc_buffer( query_abc_range_glyphs( self, t, base = uv / 256), width, abc))
				return ret;
		if ( reassign_w) {
			w = initial_overhang = abc[ uv & 0xff]. a;
			reassign_w = 0;
		}
		winc = width[ uv & 0xff];
		inc  = abc[ uv & 0xff]. c;

		if ( doWidthBreak && w + winc + inc > t-> width) {
			w += winc;
			continue;
		}

		if (( p == start) || ( p == start - 1)) {
			/* case when even single char cannot be fit in  */
			if ( t-> options & twBreakSingle) {
				/* do not return anything in this case */
				t-> count = 0;
				return ret;
			}
			/* or push this character disregarding the width */
			ADD(i);
		} else { /* normal break condition */
			ADD(p);
			i = start = p;
		}

		w = 0;
	}

	/* adding or skipping last line */
	if ( t-> n_glyphs - start > 0 || t-> count == 0)
		ADD( t-> n_glyphs);

	return ret;
}
#undef ADD

static SV*
glyphs_wrap( Handle self, SV * text, int width, int options)
{
	gpARGS;
	GlyphWrapRec t;
	AV * av;
	int i;
	unsigned int * c;
	Bool do_free;

	t. width     = ( width < 0) ? 0 : width;
	t. options   = options;
	t. cache     = &var-> font_abc_glyphs;

	gpENTER((options & twReturnFirstLineLength) ? newSViv(0) : newRV_noinc(( SV *) newAV()));

	if ( !( t.glyphs = prima_read_array(
		text, "Drawable::text_wrap",
		's', 1, 1, -1, &t.n_glyphs, &do_free))) {
		gpLEAVE;
		return nilSV;
	}

	c = Drawable_do_glyphs_wrap( self, &t);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		IV rlen = 0;
		if ( c) {
			if ( t. count > 0) rlen = c[1];
			free( c);
		}
		if ( do_free ) free( t. glyphs );
		return newSViv( rlen);
	}

	if ( !c) {
		if ( do_free ) free( t. glyphs );
		return nilSV;
	}

	av = newAV();
	if ( options & twReturnChunks) {
		for ( i = 0; i < t.count; i++)
			av_push( av, newSViv( c[i]));
	} else {
		for ( i = 0; i < t. count; i += 2) {
			SV * sv;
			unsigned int offset = c[i], size = c[i + 1] * 2;
			sv = prima_array_new(size);
			memcpy( prima_array_get_storage(sv), t.glyphs + offset, size);
			av_push( av, prima_array_tie( sv, 2, "S"));
		}
	}
	free( c);

	if ( do_free ) free( t. glyphs );
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
		gpENTER((options & twReturnFirstLineLength) ? newSViv(0) : newRV_noinc(( SV *) newAV()));
		ret = newSVsv(sv_call_perl(text, "text_wrap", "<Hiii", self, width, options, tabIndent));
		gpLEAVE;
		return ret;
	}
}

#ifdef __cplusplus
}
#endif
