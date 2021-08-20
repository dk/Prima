#include "apricot.h"
#include "guts.h"
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

#define CHECK_GP(ret) \
	if ( !is_opt(optSystemDrawable)) { \
		warn("This method is not available because %s is not a system Drawable object. You need to implement your own (ref:%d)", my->className, __LINE__);\
		return ret; \
	}

/*

SECTION 1: FONT MAPPER

font_passive_entries is queried at start and is filled with fonts that can be
used in font substitution. Each contains a Font record and a set of bit
vectors, split by FONTMAPPER_VECTOR_MASK (PassiveFontEntry). Bit vectors are
built on demand.

font_active_entries is a sparse list that contains either NULLs or PList
entries.  Each index is a block for (INDEX >> FONTMAPPER_VECTOR_BASE), and
contains set of FONT IDs, each is font_passive_entries index. It is added to
whenever text_shape needs another font, selecting entries from font_passive_entries,
or filling them by querying ranges

*/

#define FONTMAPPER_VECTOR_BASE 9 /* 512 chars or 64 bytes per vector - make sure it's greater than 256, text_wrap/abc depends on that */
#define FONTMAPPER_VECTOR_MASK ((1 << FONTMAPPER_VECTOR_BASE) - 1)

static List  font_active_entries;
static List  font_passive_entries;
static PHash font_substitutions;

#define STYLE_MASK (fsThin|fsItalic|fsBold)
#define N_STYLES   (1 + STYLE_MASK)
static int   font_mapper_default_id[N_STYLES];

typedef struct {
	Font   font;
	List   vectors;
	Bool   ranges_queried;
	Bool   is_active;
	unsigned int flags, style;
} PassiveFontEntry, *PPassiveFontEntry;

#define PASSIVE_FONT(fid) ((PPassiveFontEntry) font_passive_entries.items[(unsigned int)(fid)])

PFont 
prima_font_mapper_get_font(unsigned int fid)
{
	if ( fid >= font_passive_entries.count ) return NULL;
	return &PASSIVE_FONT(fid)->font;
}

void
prima_init_font_mapper(void)
{
	int i;
	list_create(&font_passive_entries, 256, 256);
	list_create(&font_active_entries,  16,  16);
	for ( i = 0; i < N_STYLES; i++) font_mapper_default_id[i] = -1;
	font_substitutions = prima_hash_create();

	prima_font_mapper_save_font(NULL, 0); /* occupy zero index */
}

static Bool
kill_active_entry( PList fontlist, void * dummy)
{
	if (fontlist) 
		plist_destroy(fontlist);
	return false;
}

static Bool
kill_passive_entry( PPassiveFontEntry entry, void * dummy)
{
	if ( entry-> ranges_queried ) {
		list_delete_all( &entry->vectors, true );
		list_destroy( &entry-> vectors);
	}
	free(entry);
	return false;
}

void
prima_cleanup_font_mapper(void)
{
	list_first_that( &font_active_entries, (void*)kill_active_entry, NULL);
	list_destroy( &font_active_entries);

	list_first_that( &font_passive_entries, (void*)kill_passive_entry, NULL);
	list_destroy( &font_passive_entries);

	hash_destroy( font_substitutions, false);
}

static char *
font_key( const char * name, unsigned int style)
{
	static char buf[2048];
	if ( !name ) return NULL;
	buf[0] = '0' + (style & (fsThin|fsBold|fsNormal));
	strncpy( buf + 1, name, 2046 );
	return buf;
}

PFont
prima_font_mapper_save_font(const char * name, unsigned int style)
{
	PPassiveFontEntry p;
	PFont f;
	char * key;

	key = font_key(name, style);
	if ( name && PTR2IV(hash_fetch(font_substitutions, key, strlen(key))) != 0)
		return NULL;

	if ( !( p = malloc(sizeof(PassiveFontEntry)))) {
		warn("not enough memory\n");
		return NULL;
	}
	bzero(p, sizeof(PassiveFontEntry));
	f = &p->font;
	memset( &f->undef, 0xff, sizeof(f->undef));
	f->undef.encoding = 0; /* needs enforcing */
	if (name) {
		f->undef.name = 0;
		strncpy(f->name, name, 256);
		f->name[255] = 0;
		f->undef.style = 0;
		f->style = style;
	}

	if ( name ) hash_store(
		font_substitutions,
		key, strlen(key),
		INT2PTR(void*, font_passive_entries.count)
	);

	list_add(&font_passive_entries, (Handle)f);

	return f;
}

static void
query_ranges(PPassiveFontEntry pfe)
{
	Font f;
	unsigned long * ranges;
	int i, count = 0, last;

	f = pfe->font;
	f.undef.pitch = 1;
	f.pitch = fpDefault;

	pfe-> ranges_queried = true;
	ranges = apc_gp_get_mapper_ranges(&f, &count, &pfe->flags);
	if ( count <= 0 ) {
		list_create( &pfe->vectors, 0, 1);
		return;
	}

	last = (ranges[count - 1] >> FONTMAPPER_VECTOR_BASE) + 1;
	list_create( &pfe->vectors, last, 1);
	bzero( pfe->vectors.items, last * sizeof(Handle));
	pfe->vectors.count = last;

	for ( i = 0; i < count; i += 2 ) {
		int j;
		int from = ranges[i];
		int to   = ranges[i+1];
		for (j = from; j <= to; j++) {
			Byte * map;
			unsigned int page = j >> FONTMAPPER_VECTOR_BASE, bit = j & FONTMAPPER_VECTOR_MASK;

			if ( !(pfe->flags & MAPPER_FLAGS_COMBINING_SUPPORTED)) {
				if ( j >= 0x300 && j <= 0x36f )
					continue;
			} 

			if (( map = (Byte*) pfe->vectors.items[page]) == NULL ) {
				if (!( map = malloc(1 << (FONTMAPPER_VECTOR_BASE - 3)))) {
					warn("Not enough memory");
					return;
				}
				bzero(map, 1 << (FONTMAPPER_VECTOR_BASE - 3));
				pfe->vectors.items[page] = (Handle) map;
			}
			map[ bit >> 3 ] |= 1 << (bit & 7);
		}
	}
}

static void
add_active_font(int fid)
{
	int page;
	PPassiveFontEntry pfe = PASSIVE_FONT(fid);

	if ( pfe-> is_active ) return;
	pfe-> is_active = true;

	for ( page = 0; page < pfe->vectors.count; page++) {
		if ( !pfe->vectors.items[page] ) continue;

		if ( font_active_entries.count <= page ) {
			while (font_active_entries.count <= page )
				list_add(&font_active_entries, (Handle)NULL);
		}
		if ( font_active_entries.items[page] == NULL_HANDLE )
			font_active_entries.items[page] = (Handle) plist_create(4, 4);
		list_add((PList) font_active_entries.items[page], fid);
	}
}

static void
remove_active_font(int fid)
{
	int page;
	PPassiveFontEntry pfe = PASSIVE_FONT(fid);

	if ( !pfe-> is_active ) return;

	for ( page = 0; page < pfe->vectors.count; page++) {
		if ( !pfe->vectors.items[page] ) continue;
		if ( font_active_entries.items[page] == NULL_HANDLE ) continue;
		list_delete((PList) font_active_entries.items[page], fid);
	}
}

static Bool
can_substitute(uint32_t c, int pitch, int fid)
{
	Byte * fa;
	unsigned int page, bit;
	PPassiveFontEntry pfe = PASSIVE_FONT(fid);

	if ( !pfe-> ranges_queried )
		query_ranges(pfe);

	if ( 
		pitch != fpDefault && 
		(( pfe->font.undef.pitch || pfe->font.pitch != pitch )) &&
		!( pfe-> flags & MAPPER_FLAGS_SYNTHETIC_PITCH)
	)
		return false;

	page = c >> FONTMAPPER_VECTOR_BASE;
	if ( pfe-> vectors.count <= page ) return false;

	fa = (Byte *) pfe-> vectors.items[ page ];
	if ( !fa ) return false;

	bit  = c & FONTMAPPER_VECTOR_MASK;
	if (( fa[bit >> 3] & (1 << (bit & 7))) == 0) return false;

	if ( !pfe-> is_active ) {
#ifdef _DEBUG
		printf("add polyfont %s for chr(%x)\n", pfe->font.name, c);
#endif
		add_active_font(fid);
	}

	return true;
}

static unsigned int
find_font(uint32_t c, int pitch, int style, uint16_t preferred_font)
{
	unsigned int i, def_style;
	unsigned int page = c >> FONTMAPPER_VECTOR_BASE;

	if ( preferred_font > 0 && can_substitute(c, pitch, preferred_font))
		return preferred_font;

	if ( font_active_entries.count > page && font_active_entries.items[page] ) {
		PList fonts = (PList) font_active_entries.items[page];
		for (i = 0; i < fonts->count; i++) {
			int fid = (int) fonts->items[i];
			if ( style >= 0 ) {
				PPassiveFontEntry pfe = PASSIVE_FONT(fid);
				if ( pfe-> font.style != style ) continue;
			}
			if ( can_substitute(c, pitch, fid))
				return fid;
		}
	}

	def_style = (style >= 0) ? style : 0;
	if ( font_mapper_default_id[def_style] == -1 ) {
		Font font;
		apc_font_default( &font);
		font_mapper_default_id[def_style] = -2;
		for ( i = 1; i < font_passive_entries.count; i++) {
			PPassiveFontEntry pfe = PASSIVE_FONT(i);
			if ( pfe->font.style != def_style ) continue;
			if ( strcmp( font.name, pfe->font.name ) != 0 ) continue;
			font_mapper_default_id[def_style] = i;
			break;
		}
	}

	if ( font_mapper_default_id[def_style] >= 0 && can_substitute(c, pitch, font_mapper_default_id[def_style]))
		return font_mapper_default_id[def_style];

	for ( i = 1; i < font_passive_entries.count; i++)
		if ( can_substitute(c, pitch, i))
			return i;

	if ( pitch == fpFixed ) {
		if ( font_mapper_default_id[def_style] >= 0 && can_substitute(c, pitch, font_mapper_default_id[def_style]))
			return font_mapper_default_id[def_style];
		for ( i = 1; i < font_passive_entries.count; i++) {
			if ( style >= 0 ) {
				PPassiveFontEntry pfe = PASSIVE_FONT(i);
				if ( pfe->font.style != style ) continue;
			}
			if ( can_substitute(c, fpDefault, i))
				return i;
		}
	}

	if ( style >= 0 ) {
		if ( style & fsThin )
			return find_font(c, pitch, style & ~fsThin, preferred_font);
		if ( style & fsBold )
			return find_font(c, pitch, style & ~fsBold, preferred_font);
		if ( style & fsItalic )
			return find_font(c, pitch, style & ~fsItalic, preferred_font);
		return find_font(c, pitch, -1, preferred_font);
	}

#ifdef _DEBUG
	printf("cannot map chr(%x)\n", c);
#endif
	return 0;
}

SV *
Drawable_fontMapperPalette( Handle self, Bool set, int index, SV * sv)
{
	if ( var->  stage > csFrozen) return NULL_SV;
	if ( set) {
		uint16_t fid;
		Font font;
		PPassiveFontEntry pfe;
		char * key;

		SvHV_Font(sv, &font, "Drawable::fontMapperPalette");
		key = font_key(font.name, font.style);
		fid = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));
		if ( fid == 0 ) return NULL_SV;
		pfe = PASSIVE_FONT(fid);

		switch ( index ) {
		case 0: 
			/* delete */
			if ( !pfe-> is_active ) return NULL_SV;
			remove_active_font(fid);
			return newSViv(1);
		case 1:
			/* add */
			if ( pfe-> is_active ) return NULL_SV;
			add_active_font(fid);
			return newSViv(1);
		default:
			warn("Drawable::fontPalette(%d) operation is not defined", index);
			return NULL_SV;
		}
	} else if ( index < 0 ) {
		return newSViv( font_passive_entries.count );
	} else if ( index == 0 ) {
		char * key = font_key(var->font.name, var->font.style);
		index = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));
		return newSViv(index);
	} else {
		PFont f = prima_font_mapper_get_font(index);
		if (!f) return NULL_SV;
		return sv_Font2HV( f );
	}
}

/* SECTION 2: GET TEXT WIDTH / TEXT OUT */

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
	SV ** holder;

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
	if ( len > 5 ) len = 5; /* we don't need more */
	if ( len < 1 || len != 5 ) {
		warn("malformed glyphs array in %s", caller);
		return false;
	}

	if ( !( t-> glyphs = read_subarray( av, 0, -1, &t->len, "S", caller, "glyphs")))
		return false;
	if ( t->len == 0 )
		return true;

	holder = av_fetch(av, 4, 0);
	if ( holder && *holder && !SvOK(*holder))
		goto NO_FONTS;
	if ( !( t-> fonts = read_subarray( av, 4, t->len, NULL, "S", caller, "fonts")))
		return false;

NO_FONTS:
	holder = av_fetch(av, 2, 0);
	if ( holder && *holder && !SvOK(*holder))
		goto NO_ADVANCES;
	if ( !( t-> advances = read_subarray( av, 2, t->len, NULL, "S", caller, "advances")))
		return false;
	if ( !( t-> positions = read_subarray( av, 3, t->len * 2, NULL, "s", caller, "positions")))
		return false;

NO_ADVANCES:
	if ( !( t-> indexes = read_subarray( av, 1, t->len + 1, NULL, "S", caller, "indexes")))
		return false;
	t-> text_len = t-> indexes[t->len];

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

	if ( t-> fonts )
		t-> fonts    += from;
}

static Bool
switch_font( Handle self, uint16_t fid)
{
	Font src, dst;
	src = PASSIVE_FONT(fid)->font;
	if ( is_opt(optSystemDrawable) ) {
		dst = var->font;
		src.size = dst.size;
		src.undef.size = 0;
		apc_font_pick( self, &src, &dst);
		if ( strcmp(dst.name, src.name) != 0 )
			return false;
		apc_gp_set_font( self, &dst);
	} else {
		dst = my->get_font(self);
		src.size = dst.size;
		src.undef.size = 0;
		my->set_font(self, src);
	}
	return true;
}

Bool
Drawable_text_out( Handle self, SV * text, int x, int y, int from, int len)
{
	Bool ok;
	if ( !opt_InPaint) return false;

	if ( !SvROK( text )) {
		STRLEN dlen;
		char * c_text = SvPV( text, dlen);
		Bool   utf8 = prima_is_utf8_sv( text);
		CHECK_GP(false);
		if ( utf8) dlen = prima_utf8_length(c_text, dlen);
		if ((len = check_length(from,len,dlen)) == 0)
			return true;
		c_text = hop_text(c_text, utf8, from);
		ok = apc_gp_text_out( self, c_text, x, y, len, utf8 ? toUTF8 : 0);
		if ( !ok) perl_error();
	} else if ( SvTYPE( SvRV( text)) == SVt_PVAV) {
		GlyphsOutRec t;
		CHECK_GP(false);
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
		free(abc);
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
		CHECK_GP(0);
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
		CHECK_GP(0);
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
		CHECK_GP(NULL_SV);

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
		CHECK_GP(NULL_SV);
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

/* SECTION 3: SHAPING */

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
	int i, vis_len;
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
	int rtl, font, start = r->i;

	if ( r->i >= r->vis_len ) return 0;

	rtl  = t->analysis[r->i];
	if ( t-> fonts ) font = t->fonts[r->i];
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
run_alloc( PTextShapeRec t, int visual_start, int visual_len, Bool invert_rtl, PTextShapeRec run)
{
	int i, flags = 0;
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

static void
analyze_fonts( Handle self, PTextShapeRec t, uint16_t * fonts)
{
	int i;
	uint32_t *text = t-> text;
	int pitch      = (t->flags >> toPitch) & fpMask;
	char * key     = font_key(var->font.name, var->font.style);
	uint16_t fid   = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));

	bzero(fonts, t->len * sizeof(uint16_t));

	for ( i = 0; i < t->len; i++) {
		unsigned int nfid = find_font(*(text++), pitch, var->font.style, fid);
		if ( nfid != fid ) fonts[i] = nfid;
	}

	/* make sure that all combiners are same font as base glyph */
	text = t->text;
	for ( i = 0; i < t->len; ) {
		int j, base, non_base_font = 0, need_align = 0;
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
	Byte analysis[MAX_CHARACTERS], *save_analysis;
	uint16_t fonts[MAX_CHARACTERS], *save_fonts;
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
				if ( switch_font(self, run.fonts[0])) {
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

/* SECTION 4: TEXT WRAP */

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
		CHECK_GP(NULL);
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
fill_font_ranges( Handle self )
{
	if ( Drawable_get_font_ranges == my->get_font_ranges ) {
		CHECK_GP(false);
		if ( !var-> font_abc_glyphs_ranges ) {
			if ( !( var-> font_abc_glyphs_ranges = apc_gp_get_font_ranges(self, &var->font_abc_glyphs_n_ranges)))
				return false;
		}
	} else {
		SV * sv;
		void * array;
		Bool do_free;
		sv = my-> get_font_ranges( self);
		array = prima_read_array( sv, "get_font_ranges", 'i', 1, -1, -1, &var->font_abc_glyphs_n_ranges, &do_free);
		if ( !array ) {
			sv_free(sv);
			return false;
		}
		if ( do_free ) {
			var-> font_abc_glyphs_ranges = array;
		} else {
			int size = var->font_abc_glyphs_n_ranges * sizeof(int);
			if ( !( var-> font_abc_glyphs_ranges = malloc(size))) {
				warn("Not enough memory");
				sv_free(sv);
				return false;
			}
			memcpy( var-> font_abc_glyphs_ranges, array, size );
			free(array);
		}
		sv_free(sv);
	}
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
	if ( t->fonts) {
		/* different fonts case */
		Byte * fa;
		PassiveFontEntry *pfe;
		int i, font_changed = 0;
		uint32_t from, to;
		unsigned int page;
		char * key;
		Byte used_fonts[MAX_CHARACTERS / 8], filled_entries[256 / 8];

		from = base * 256;
		to   = from + 255;
		page = from >> FONTMAPPER_VECTOR_BASE;
		bzero(used_fonts, sizeof(used_fonts));
		bzero(filled_entries, sizeof(filled_entries));
		used_fonts[0] = 0x01; /* fid = 0 */
		key = font_key(var->font.name, var->font.style);
		i = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));
		if ( i > 0 ) {
			/* copy ranges from subst table */
			pfe = PASSIVE_FONT(i);
			if ( !pfe-> ranges_queried )
				query_ranges(pfe);
			if ( pfe-> vectors.count <= page ) goto NO_FONT_ABC; /* should be there, or some error */
			/* page covers the 256 range in whole */
			fa = (Byte *) pfe-> vectors.items[ page ];
			if ( fa ) {
				i = from & FONTMAPPER_VECTOR_MASK;
				memcpy( filled_entries, fa + i, 256 / 8);
			}
		} else {
			/* query the range and fill the cache */
			unsigned long * ranges;
			if ( !fill_font_ranges(self))
				goto NO_FONT_ABC;
			ranges = var-> font_abc_glyphs_ranges;
			for ( i = 0; i < var->font_abc_glyphs_n_ranges; i += 2, ranges += 2 ) {
				int j;
				if ( ranges[0] > to || ranges[1] < from )
					continue;
				for ( j = ranges[0]; j <= ranges[1]; j++) {
					if ( j >= from && j <= to )
						filled_entries[(j - from) >> 3] |= 1 << ((j - from) & 7);
				}
			}
		}

		for ( i = 0; i < t->n_glyphs; i++) {
			PFontABC abc2;
			uint16_t fid = t->fonts[i];
			uint32_t uv;
			if ( used_fonts[fid >> 3] & ( 1 << (fid & 7)))
				continue;
			used_fonts[fid >> 3] |= 1 << (fid & 7);

			pfe = PASSIVE_FONT(fid);
			if ( !switch_font(self, fid))
				continue;
			font_changed = 1;

			if ( !pfe-> ranges_queried )
				query_ranges(pfe);
			if ( pfe-> vectors.count <= page )
				continue;

			if ( !( abc2 = call_get_font_abc( self, from, to, toGlyphs)))
				continue;

			fa = (Byte *) pfe-> vectors.items[ page ];
			if ( !fa ) continue;
			for ( uv = from; uv <= to; uv++) {
				unsigned int bit = uv & FONTMAPPER_VECTOR_MASK;
				if (( fa[bit >> 3] & (1 << (bit & 7))) == 0) continue;
				if ((filled_entries[(uv - from) >> 3] & (1 << ((uv - from) & 7))) != 0) continue;
				filled_entries[(uv - from) >> 3] |= 1 << ((uv - from) & 7);
				abc[uv - from] = abc2[uv - from];
			}
		}
		if ( font_changed ) {
			if ( Drawable_set_font == my->set_font && is_opt(optSystemDrawable))
				apc_gp_set_font( self, &var->font);
			else
				my->set_font(self, var->font);
		}
	}
NO_FONT_ABC:

	if ( !fill_abc_list_cache(t->cache, base, abc)) {
		free( abc);
		return NULL;
	}

	return abc;
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
fill_tilde_properties(Handle self, TextWrapRec * t, int tildeIndex, int tildePos, int tildeCharPos, int tildeOffset)
{
	UV uv;
	int base;
	PFontABC abcc;
	float start, end;

	t-> t_bytepos = tildePos;
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
	end   = start + ((abcc->a < 0) ? 0 : abcc->a) + abcc-> b + ((abcc->c < 0) ? 0 : abcc->c) - 1.0;
	if ( abcc-> a < 0.0 ) {
		start += abcc->a;
		end += abcc->a;
	}
	t-> t_start = start + .5 * (( start < 0 ) ? -1 : 1);
	t-> t_end   = end   + .5 * (( end   < 0 ) ? -1 : 1);
}

static void
text_init_wrap_rec( Handle self, SV * text, int width, int options, int tabIndent, int from, int len, TextWrapRec * t)
{
	STRLEN tlen;

	t-> text      = SvPV( text, tlen);
	t-> utf8_text = prima_is_utf8_sv( text);
	if ( t-> utf8_text) {
		t-> utf8_textLen = prima_utf8_length( t-> text, tlen);
		if (( t-> utf8_textLen = check_length(from, len, t-> utf8_textLen)) == 0)
			from = 0;
		t-> text = hop_text(t->text, true, from);
		t-> textLen = utf8_hop(( U8*) t-> text, t-> utf8_textLen) - (U8*) t-> text;
	} else {
		if ((tlen = check_length(from, len, tlen)) == 0)
			from = 0;
		t-> text = hop_text(t->text, false, from);
		t-> utf8_textLen = t-> textLen = tlen;
	}

	t-> width     = width;
	t-> tabIndent = ( tabIndent < 0) ? 0 : tabIndent;
	t-> options   = options;
	t-> ascii     = &var-> font_abc_ascii;
	t-> unicode   = &var-> font_abc_unicode;
	t-> t_char    = NULL;
	t-> t_start   = C_NUMERIC_UNDEF;
	t-> t_end     = C_NUMERIC_UNDEF;
	t-> t_line    = C_NUMERIC_UNDEF;
	t-> t_pos     = C_NUMERIC_UNDEF;
	t-> t_bytepos = C_NUMERIC_UNDEF;
	t-> count     = 0;
}

static SV*
mnemonic2sv(TextWrapRec * t)
{
	HV * profile = newHV();
	if ( t-> t_char) {
		STRLEN len = t-> utf8_text ? utf8_hop(( U8*) t-> t_char, 1) - ( U8*) t-> t_char : 1;
		SV * sv_char = newSVpv( t-> t_char, len);
		pset_sv_noinc( tildeChar, sv_char);
		if ( t->utf8_text) SvUTF8_on( sv_char);
		if ( t->t_start != C_NUMERIC_UNDEF) pset_i( tildeStart, t->t_start);
		if ( t->t_end   != C_NUMERIC_UNDEF) pset_i( tildeEnd,   t->t_end);
		if ( t->t_line  != C_NUMERIC_UNDEF) pset_i( tildeLine,  t->t_line);
		if ( t->t_pos   != C_NUMERIC_UNDEF) pset_i( tildePos,   t->t_pos);
	}
	return newRV_noinc(( SV *) profile);
}

static SV*
first_line2sv(int * c, int count )
{
	int rlen = ( c && count > 0) ? c[3] : 0;
	return newSViv( rlen);
}

static SV*
chunks2sv(Handle self, int from, int * c, int count)
{
	int i;
	AV * av = newAV();

	for ( i = 0; i < count; i += 4) {
		av_push( av, newSViv(from + c[i+2]) );
		av_push( av, newSViv(c[i+3]) );
	}

	return (SV*)av;
}

static SV*
textout2sv(Handle self, int * c, TextWrapRec * t)
{
	int i, line;
	char buf[256];
	semistatic_t pbuf;
	AV * av = newAV();

	if ( t-> options & twExpandTabs )
		semistatic_init(&pbuf, &buf, 1, 256);

	for ( i = line = 0; i < t-> count; i += 4, line++) {
		SV * sv;
		if ( t-> options & twExpandTabs ) {
			int j, nt, len = c[i+1], sz;
			char *src, *dst;
			for ( j = nt = 0, src = t->text + c[i]; j < len; j++, src++ )
				if ( *src == '\t' )
					nt++;
			if ( nt == 0 ) goto AS_IS;

			sz = len + nt * (t->tabIndent - 1) + 1;
			if ( !semistatic_expand(&pbuf, sz)) {
				warn("Not enough memory");
				sv_free((SV*) av);
				return NULL_SV;
			}
			for ( j = 0, src = t->text + c[i], dst = (char*)pbuf.heap; j < len; j++, src++ ) {
				if ( *src == '\t') {
					int k;
					for ( k = 0; k < t->tabIndent; k++ )
						*(dst++) = ' ';
				}
				else
					*(dst++) = *src;
			}
			sv = newSVpv((char*) pbuf.heap, sz - 1 );
		} else {
		AS_IS:
			sv = newSVpv( t->text + c[i], c[i+1]);
		}
		if (( t-> options & twCollapseTilde) && ( line == t-> t_line) && t-> t_char) {
			STRLEN tlen;
			char * pv = SvPV( sv, tlen );
			memmove( pv + t-> t_bytepos, pv + t-> t_bytepos + 1, tlen - t-> t_bytepos - 1);
			pv[tlen] = 0;
			SvCUR_set(sv, tlen-1);
			SvPOK_only(sv);
		}

		if ( t->utf8_text) SvUTF8_on( sv);
		av_push( av, sv );
	}

	if ( t-> options & twExpandTabs )
		semistatic_done(&pbuf);

	return (SV*)av;
}

typedef struct {
	int * storage;
	unsigned int bufsize, base, options;
	int width, limit, utf8_limit;
	float widths[256];
	FontABC abcs[256];
	struct {
		int start, utf8_start;
		int end;
		int split_start, utf8_split_start;
		int split_end;
		int p, utf8_p;
	} curr, prev;
	Bool do_width_break, first_only;
	int tilde_index, tilde_line, tilde_pos, tilde_char_pos, tilde_offset;
} WrapRec;

static Bool
wrap_init( WrapRec * w, TextWrapRec * tw, GlyphWrapRec * gw)
{
	bzero( w, sizeof(WrapRec));
	w-> width            = tw ? tw->width        : gw->width;
	w-> options          = tw ? tw->options      : gw->options;
	w-> limit            = tw ? tw->textLen      : gw->n_glyphs;
	w-> utf8_limit       = tw ? tw->utf8_textLen : gw->n_glyphs;

	w-> do_width_break   = w->width >= 0;
	w-> first_only       = (w->options & twReturnFirstLineLength) == twReturnFirstLineLength;

	w-> base             = 0x10000000;

	w-> curr.split_start = w-> curr.split_end = w-> curr.utf8_split_start = -1;
	w-> prev             = w-> curr;

	w-> bufsize          = 128;
	if (!( w-> storage = allocn( int, w-> bufsize))) return false;

	w-> tilde_index      = -100;

	return true;
}

static Bool
wrap_add_entry( WrapRec * w, TextWrapRec * tw, GlyphWrapRec * gw, int end, int utf_end )
{
	int *count = tw ? &tw->count : &gw->count;
	if ( *count == w-> bufsize) {
		int * n;
		if ( !( n = (int *)realloc( w->storage, sizeof(unsigned int) * (w->bufsize *= 2))))
			return false;
		w->storage = n;
	}
#ifdef _DEBUG
	printf("add %d - %d\n", w-> curr. utf8_start, end);
#endif

	if (
		tw && 
		w-> tilde_index >= 0 &&
		w-> tilde_index >= w-> curr.start &&
		w-> tilde_index < end
	) {
		char
			*line     = tw-> text + w-> curr.start,
			*tilde_at = tw->text + w-> tilde_index;
		w-> tilde_char_pos = 0;
		while ( line < tilde_at ) {
			line = (char*)utf8_hop((U8*)line, 1);
			w-> tilde_char_pos++;
		}
		w-> tilde_line = tw-> t_line = *count / 4;
		w-> tilde_pos  = w->tilde_index - w->curr.start;
		if ( w-> tilde_index == end - 1) tw-> t_line++;
	}

	w-> storage[(*count)++] = w-> curr.start;
	w-> storage[(*count)++] = end - w-> curr.start;
	w-> storage[(*count)++] = w-> curr.utf8_start;
	w-> storage[(*count)++] = utf_end - w-> curr.utf8_start;
	if ( tw && gw ) gw-> count = tw-> count;

	w-> curr.start      = end;
	w-> curr.utf8_start = utf_end;

	return !w-> first_only;
}

#define wrap_new_word(w,len)                      \
	w.curr.split_start      = w.curr.p;       \
	w.curr.split_end        = w.curr.p + len; \
	w.curr.utf8_split_start = w.curr.utf8_p

#define wrap_fetch_uvchr(w, tw, len)              \
	((len = 1) && tw->utf8_text) ?            \
		prima_utf8_uvchr_end(             \
			tw->text + w.curr.p,      \
			tw->text + tw-> textLen,  \
			&len                      \
		) :                               \
		((unsigned char*)(tw-> text))[w.curr.p]

#define wrap_step_ptr(w,len)                      \
	w.curr.p += len;                          \
	w.curr.utf8_p++

static Bool
wrap_load_glyphs_abc(uint32_t uv, WrapRec * w, Handle self, GlyphWrapRec *g)
{
	PFontABC labc;
	if ( uv / 256 == w-> base)
		return true;
	w-> base = uv / 256;
	if ( !(labc = query_abc_range_glyphs( self, g, w->base)))
		return false;
	if ( g-> advances )
		precalc_ac_buffer(labc, w->abcs);
	else
		precalc_abc_buffer(labc, w->widths, w->abcs);
	return true;
}

/* a very quick check, if possible, if glyphstr fits */

static SV*
glyphs_fit_quickcheck(Handle self, SV * glyphs, int width, int options, TextWrapRec *tw, GlyphsOutRec *g)
{
	AV * av;
	if (!(
		(g->len == 0) ||
		(g->advances && ( width >= get_glyphs_width(self, g, true)))
	))
		return NULL;

	if (( options & twReturnFirstLineLength) == twReturnFirstLineLength)
		return newSViv(tw ? tw-> utf8_textLen : g->len);

	av = newAV();
	if ( options & twReturnChunks) {
		av_push( av, newSViv(0));
		av_push( av, newSViv(tw ? tw-> utf8_textLen : g->len));
	} else if ( !tw || options & twReturnGlyphs ) {
		av_push( av, newSVsv(sv_call_perl(glyphs, "clone", "<S", glyphs)));
	} else {
		SV * sv = newSVpv( tw-> text, tw-> textLen );
		if ( tw->utf8_text) SvUTF8_on( sv);
		av_push( av, sv );
	}
	return newRV_noinc(( SV *) av);
}

static void
glyph_init_wrap_rec( Handle self, int width, int options, int offset, GlyphsOutRec *g, GlyphWrapRec *t)
{
	t->offset    = offset;
	t->n_glyphs  = g->len;
	t->glyphs    = g->glyphs;
	t->indexes   = g->indexes;
	t->advances  = g->advances;
	t->positions = g->positions;
	t->fonts     = g->fonts;
	t->width     = width;
	t->text_len  = g->text_len;
	t->options   = options;
	t->cache     = &var-> font_abc_glyphs;
	t->count     = 0;
}

static SV*
glyphout2sv(Handle self, int * c, GlyphsOutRec *g, TextWrapRec *tw, GlyphWrapRec *gw, uint16_t* log2vis)
{
#define STATIC_BUF_SIZE 1024
	int i, line;
	AV * av;
	int j,
		mul[5] = { 1, 1, 1, 2, 1 },
		extras[5] = {0, 1, 0, 0, 0},
		got_tab = 0;
	uint16_t *payload[5] = { g->glyphs, g->indexes, g->advances, (uint16_t*)g->positions, g->fonts };
	uint16_t buf1[STATIC_BUF_SIZE];
	uint32_t buf2[STATIC_BUF_SIZE];
	semistatic_t sbuf, tbuf;

	av = newAV();

	semistatic_init(&sbuf, &buf1, sizeof(uint16_t), STATIC_BUF_SIZE);

	if ( tw != NULL && (tw->options & twExpandTabs)) {
		int k, l;
		char *text;
		semistatic_init(&tbuf, &buf2, sizeof(uint32_t), STATIC_BUF_SIZE);
		for (
			k = 0, l = 0, text = tw->text;
			k < tw-> utf8_textLen;
			k++, l++
		) {
			uint32_t uv;
			STRLEN len;
			if (tw->utf8_text) {
				uv = prima_utf8_uvchr_end(text, tw->text + tw-> textLen, &len);
				if ( len < 1 ) break;
				text += len;
			} else {
				uv = *(text++);
			}
			if ( !semistatic_push(tbuf,uint32_t,uv))
				goto FAIL;
			if ( uv == '\t')
				got_tab = 1;
		}
	}

	for ( i = 2, line = 0; i < gw->count; i += 4, line++) {
		SV *sv_payload[5];
		uint16_t *dest[5];
		int first_char = c[i] + gw->offset, last_char = first_char + c[i + 1];

		sbuf.count = 0;

		if ( tw && gw->indexes) {
			/* copy subset by text and use indexes */
			for ( j = 0; j < g->len; j++) {
				int ix = g->indexes[j] & ~toRTL;
				if ( ix < first_char || ix >= last_char ) continue;
				if (
					tw &&
					( tw-> options & twCollapseTilde) &&
					tw-> t_char &&
					line == tw-> t_line &&
					ix == tw-> t_pos
				)
					continue;
				if ( !semistatic_push(sbuf,uint16_t,j))
					goto FAIL;
			}
		} else {
			/* copy as is */
			for ( j = first_char; j < last_char; j++)
				if ( !semistatic_push(sbuf,uint16_t,j))
					goto FAIL;
		}

		for ( j = 0; j < 5; j++) {
			SV * sv;
			uint16_t *dst, k;
			if ( payload[j] == NULL ) {
				sv_payload[j] = NULL_SV;
				continue;
			}
			sv  = prima_array_new(sizeof(uint16_t) * (sbuf.count * mul[j] + extras[j]));
			dest[j] = dst = (uint16_t*)prima_array_get_storage(sv);
			if ( mul[j] == 1 ) {
				for ( k = 0; k < sbuf.count; k++)
					*(dst++) = payload[j][semistatic_at(sbuf,uint16_t,k)];
				if ( j == 1 )
					*dst = payload[j][g->len];
			} else {
				for ( k = 0; k < sbuf.count; k++) {
					int ix = semistatic_at(sbuf,uint16_t,k) * 2;
					*(dst++) = payload[j][ix];
					*(dst++) = payload[j][ix+1];
				}
			}

			if ( j == 2 && got_tab ) {
				uint16_t *advances = dest[2], *indexes = dest[1];
				for ( k = 0; k < sbuf.count; k++)
					if ( semistatic_at(tbuf, uint32_t, indexes[k] & ~toRTL) == '\t' )
						advances[ k ] *= tw-> tabIndent;
			}
			sv_payload[j] = sv_2mortal(prima_array_tie( sv, sizeof(uint16_t), (j == 3) ? "s" : "S"));
		}

		av_push( av, newSVsv(
			call_perl(self, "new_glyph_obj", "<SSSSS",
				sv_payload[0],
				sv_payload[1],
				sv_payload[2],
				sv_payload[3],
				sv_payload[4]
			)
		));
	}

	semistatic_done(&sbuf);
	return (SV*)av;
#undef STATIC_BUF_SIZE

FAIL:
	semistatic_done(&sbuf);
	sv_free((SV*)av);
	return NULL_SV;
}

static uint16_t *
fill_log2vis(GlyphsOutRec *g, int from)
{
	int i;
	uint16_t *l2v, *ix = g->indexes, last = 0;
	if (( l2v = malloc( sizeof(uint16_t) * g->text_len)) == NULL)
		return NULL;
	if ( ix ) {
		memset(l2v, 0xff, sizeof(uint16_t) * g->text_len);
		for ( i = 0; i < g->len; i++) {
			int v = ix[i] & ~toRTL;
			if ( l2v[v] > i ) l2v[v] = i;
		}
		for ( i = 0; i < g->text_len; i++) {
			if ( l2v[i] != 0xffff )
				last = l2v[i];
			else
				l2v[i] = last;
		}
	} else {
		for ( i = 0; i < g->text_len; i++)
			l2v[i] = i;
	}

	return l2v;
}

int *
Drawable_do_text_wrap( Handle self, TextWrapRec * tw, GlyphWrapRec * gw, uint16_t * log2vis)
{
	WrapRec wr;
	float w = 0, initial_overhang = 0;
	Bool reassign_w = 0;
	int space_width = -1, space_c = 0;

	if ( !wrap_init(&wr, tw, gw))
		return NULL;

	/* determining ~ character location */
	if ( wr.options & twCalcMnemonic)
		wr.tilde_index = find_tilde_position(tw);

#define ADD(ptr) \
	if ( !wrap_add_entry( &wr, tw, gw, wr.curr.ptr, wr.curr.utf8_##ptr))  \
		return wr.storage

#define LOAD_ABC(x) \
	if ( !wrap_load_glyphs_abc(x, &wr, self, gw)) \
		return wr.storage

#define RETURN_EMPTY if (1) {             \
	if ( gw ) gw-> count = 0;         \
	if ( tw ) tw-> count = 0;         \
	return wr.storage;                \
}

	while ( wr.curr.p < wr.limit ) {
		float dw, c;
		unsigned int j, nc, ng, wmul = 1;
		STRLEN len = 1;
		uint32_t uv, uv0, last_uv = 0;
		uint16_t index;

		wr.prev = wr.curr;

		/* nc: codepoints in the cluster */
		if ( log2vis ) {
			unsigned int v, cmp;
			for (
				nc = 1, v = wr.curr.utf8_p + 1, index = log2vis[wr.curr.utf8_p];
				v < tw->utf8_textLen;
				v++
			) {
				if ( log2vis[v] != index ) break;
				nc++;
			}
			for (
				ng = 1,
					v = log2vis[wr.curr.utf8_p] + 1,
					cmp = gw->indexes[log2vis[wr.curr.utf8_p]] & ~toRTL;
				v < gw->n_glyphs;
				v++
			) {
				if (( gw->indexes[v] & ~toRTL ) != cmp ) break;
				ng++;
			}
		} else {
			ng    = 1;
			nc    = 1;
			index = wr.curr.utf8_p;
		}

		uv = tw ? wrap_fetch_uvchr(wr,tw,len) : 0;
		if ( !tw || nc > 1 ) goto NON_BREAKER;

		if ( len < 1 ) break;

		switch ( uv ) {
		case '\n':
		case 0x2028:
		case 0x2029:
			wrap_new_word(wr,len);
			if (!( wr.options & twNewLineBreak))
				goto NON_BREAKER;
			break;

		case ' ':
			wrap_new_word(wr,len);
			if (!( wr.options & twSpaceBreak))
				goto NON_BREAKER;
			break;
		case '\t':
			wrap_new_word(wr,len);
			if ( wr.options & twCalcTabs)
				wmul = tw->tabIndent;
			if (!( wr.options & twSpaceBreak))
				goto NON_BREAKER;
			if ( space_width < 0 ) {
				PFontABC s;
				if ( !( s = query_abc_range( self, tw, 0)))
					return wr.storage;
				space_c     = (s[' '].c < 0) ? - s[' ']. c : 0;
				space_width = (s[' '].a + s[' '].b + s[' '].c) * tw-> tabIndent;
			}
			dw   = space_width;
			c    = space_c;
			goto PREDEFINED_WIDTH;
		case '~':
			if ( wr.curr.p == wr.tilde_index ) {
				wr.tilde_offset = w - initial_overhang;
				dw = c = 0;
				goto PREDEFINED_WIDTH;
			}
			goto NON_BREAKER;
		default:
			goto NON_BREAKER;
		}
		ADD(p);
		wrap_step_ptr(wr, len);
		wr.curr.start      = wr.curr.p;
		wr.curr.utf8_start = wr.curr.utf8_p;
		reassign_w = 1;
		continue;
	NON_BREAKER:

		/* calculate widths */
		dw = c = 0;
		if ( gw ) {
			for ( j = 0, uv = uv0 = 0; j < ng; j++) {
				last_uv = uv;
				uv = gw->glyphs[index + j];
				if ( j == 0 ) uv0 = uv;
				if (!gw-> advances || reassign_w) /* do not query ABC unnecessarily if advances are there */
					LOAD_ABC(last_uv);
				dw += (gw->advances ? gw->advances[index + j] : wr.widths[uv & 0xff]) * wmul;
				if ( j == nc - 1 && !gw-> advances)
					c = wr.abcs[uv & 0xff].c;
				if ( reassign_w) {
					w = initial_overhang = wr.abcs[uv & 0xff].a;
					reassign_w = 0;
				}
			}
		} else {
			if ( uv / 256 != wr.base)
				if ( !precalc_abc_buffer( query_abc_range( self, tw, wr.base = uv / 256), wr.widths, wr.abcs))
					return wr.storage;
			uv0 = uv;
			dw  = wr.widths[uv & 0xff];
			c   = wr.abcs[uv & 0xff].c;

		}
		if ( reassign_w) {
			w = initial_overhang = wr.abcs[uv0 & 0xff].a;
			reassign_w = 0;
		}
	PREDEFINED_WIDTH:

		/* advance text pointers */
		if ( tw ) {
			for ( j = 0; j < nc; j++) {
				wrap_fetch_uvchr(wr,tw,len);
				if ( len < 1 ) break;
				wrap_step_ptr(wr, len);
			}
		} else {
			wrap_step_ptr(wr, 1);
		}

#ifdef _DEBUG
		printf("i:%d/%d nc:%d ng:%d w:%f dw:%f c:%f index:%d uv:%x\n",  wr.curr.p, wr.curr.utf8_p, nc, ng, w, dw, c, index, uv0);
#endif
		if ( !wr.do_width_break || (w + dw + c <= wr.width)) {
			w += dw;
			continue;
		}

		if ( gw && gw-> advances && wr.prev.p > wr.curr.start ) {
			/* this glyph is clearly out of bounds, but it could be that the previous was too.

			The reason behind this complication is that fetching every glyphs A/C metrics under libXft,
			on probably under win32, requires the whole glyph to be fetched. This hiccups if the string
			or font size are so unfortunate that glyphs are being discarded often. But since C is used only
			to check whether the last glyph hangs over the limit or not, we don't query C until necessary.
			The complication is that we need to step back if the previous glyph's C was big enough to make it
			not fit either.

			The effect can be seen when selecting with mouse chinese text in podview in Prima/Drawable/Glyphs -
			when each glyph is queried, it might take several seconds for each redraw.
			*/
			LOAD_ABC(last_uv);
			if ( w + wr.abcs[last_uv & 0xff].c > wr.width ) /* ... and it is */
				wr.curr = wr.prev;
		}

		if ( wr.prev.p == wr.curr.start) {
			/* case when even single char cannot be fit in  */
			if ( wr.options & twBreakSingle) RETURN_EMPTY;
			/* or push this character disregarding the width */
			ADD(p);
			reassign_w = 1;
		} else {
			/* normal break condition */
			if ( wr.options & twWordBreak) {
				/* checking if break was at word boundary */
				if ( wr.curr.start <= wr.curr.split_start) {
					ADD(split_start);
					wr.curr.p      = wr.curr.start      = wr.curr.split_end;
					wr.curr.utf8_p = wr.curr.utf8_start = wr.curr.utf8_split_start + 1;
					w = 0;
					reassign_w = 1;
					continue;
				} else if ( wr.options & twBreakSingle) {
					/* cannot be split */
					RETURN_EMPTY;
				}
			}

			/* repeat again */
			wr.curr = wr.prev;
			ADD(p);
			reassign_w = 1;
		}
		w = 0;
	}

	/* adding or skipping last line */
	if (
		wr.limit - wr.curr.start > 0 ||
		( tw && tw->count == 0) ||
		( gw && gw->count == 0)
	) {
		wr.curr.p      = wr.limit;
		wr.curr.utf8_p = wr.utf8_limit;
		ADD(p);
	}

	/* fill ~ location */
	if (tw && wr.tilde_index >= 0 && !(wr.options & twReturnChunks))
		fill_tilde_properties(self, tw, wr.tilde_index, wr.tilde_pos, wr.tilde_char_pos, wr.tilde_offset);

	return wr.storage;
}
#undef ADD
#undef LOAD_ABC
#undef RETURN_EMPTY

static SV*
string_wrap( Handle self,SV * text, int width, int options, int tabIndent, int from, int len)
{
	gpARGS;
	TextWrapRec t;
	int * c;
	SV *ret;

	if ( options & twReturnGlyphs ) {
		warn("Drawable.text_wrap only can use tw::ReturnGlyphs if glyphs are supplied");
		options &= ~twReturnGlyphs;
	}

	text_init_wrap_rec( self, text, width, options, tabIndent, from, len, &t);
	gpENTER(NULL_SV);
	c = my->do_text_wrap( self, &t, NULL, NULL);
	gpLEAVE;
	t.t_pos += from;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength)
		ret = first_line2sv(c, t.count);
	else if ( !c)
		return NULL_SV;
	else if ( options & twReturnChunks ) {
		SV * sv = chunks2sv(self, from, c, t.count);
		ret = ( sv == NULL_SV ) ? NULL_SV : newRV_noinc(sv);
	} else {
		SV * av = textout2sv(self, c, &t);
		if ( av != NULL_SV ) {
			if  (t.options & ( twCalcMnemonic | twCollapseTilde))
				av_push((AV*) av, mnemonic2sv(&t));
			ret = newRV_noinc(av);
		} else
			ret = av;
	}
	free( c);

	return ret;
}


static SV*
glyphs_wrap( Handle self, SV * text, int width, int options, int from, int len)
{
	gpARGS;
	GlyphWrapRec t;
	int * c;
	GlyphsOutRec g;
	SV *qt, *ret;

	if (!read_glyphs(&g, text, 1, "Drawable::text_wrap"))
		return NULL_SV;
	if ((len = check_length(from, len, g.len)) == 0)
		from = 0;
	hop_glyphs(&g, from, len);
	if (( qt = glyphs_fit_quickcheck(self, text, width, options, NULL, &g)) != NULL)
		return qt;
	glyph_init_wrap_rec( self, width, options, 0, &g, &t);
	if (options & (twExpandTabs|twCollapseTilde|twCalcMnemonic|twCalcTabs|twWordBreak))
		warn("Drawable::text_wrap(glyphs) does not accept tw::ExpandTabs,tw::CollapseTilde,tw::CalcMnemonic,tw::CalcTabs,tw::WordBreak");

	gpENTER(NULL_SV);
	c = my->do_text_wrap( self, NULL, &t, NULL);
	gpLEAVE;

	if (( t. options & twReturnFirstLineLength) == twReturnFirstLineLength)
		ret = first_line2sv(c, t.count);
	else if ( !c)
		return NULL_SV;
	else if ( options & twReturnChunks ) {
		SV * sv = chunks2sv(self, from, c, t.count);
		ret = (sv == NULL_SV) ? NULL_SV : newRV_noinc(sv);
	} else {
		SV * sv = glyphout2sv(self, c, &g, NULL, &t, NULL);
		ret = (sv == NULL_SV) ? NULL_SV : newRV_noinc(sv);
	}
	free( c);

	return ret;
}

static SV*
string_glyphs_wrap( Handle self, SV * text, int width, int options, int tabIndent, int from, int len, SV * glyphs)
{
	gpARGS;
	SV *qt, *ret, *av = NULL;
	GlyphsOutRec g;
	TextWrapRec tw;
	GlyphWrapRec gw;
	int *c;
	void *subglyphs = NULL;
	uint16_t *log2vis = NULL;

	if ( !SvROK(glyphs) || SvTYPE( SvRV(glyphs)) != SVt_PVAV ) {
		warn("Drawable::text_wrap: not a glyph array passed");
		return NULL_SV;
	}
	if (!read_glyphs(&g, glyphs, 1, "Drawable::text_wrap"))
		return NULL_SV;
	text_init_wrap_rec( self, text, width, options, tabIndent, 0, -1, &tw);
	if ( g.text_len != tw.utf8_textLen) {
		warn("Drawable::text_wrap: text and glyphstr don't match");
		return NULL_SV;
	}
	if ( from != 0 || len != -1 )
		text_init_wrap_rec( self, text, width, options, tabIndent, from, len, &tw);

	if (
		from == 0 && len == -1 &&
		!( options & (twCalcTabs|twExpandTabs|twSpaceBreak|twNewLineBreak|twCalcMnemonic|twCollapseTilde))
	) {
		if (( qt = glyphs_fit_quickcheck(self, glyphs, width, options, &tw, &g)) != NULL)
			return qt;
	}

	glyph_init_wrap_rec( self, width, options, from, &g, &gw);
	if ( g.indexes ) {
		/* log2vis needs to address the whole string */
		if ( !( log2vis = fill_log2vis(&g, from))) {
			warn("not enough memory");
			return NULL_SV;
		}
	}

	gpENTER(NULL_SV);
	c = my->do_text_wrap( self, &tw, &gw, log2vis + from);
	gpLEAVE;
	tw.t_pos += from;

	if (( options & twReturnFirstLineLength) == twReturnFirstLineLength) {
		ret = first_line2sv(c, gw.count);
	} else if ( !c ) {
		ret = NULL_SV;
	} else if ( options & twReturnGlyphs ) {
		av = glyphout2sv(self, c, &g, &tw, &gw, log2vis + from );
		ret = ( av == NULL_SV ) ? NULL_SV : newRV_noinc(av);
	} else if ( options & twReturnChunks ) {
		SV * sv = chunks2sv(self, from, c, gw.count);
		ret = ( sv == NULL_SV ) ? NULL_SV : newRV_noinc(sv);
	} else {
		av = textout2sv(self, c, &tw);
		ret = ( av == NULL_SV ) ? NULL_SV : newRV_noinc(av);
	}

	if  (
		(tw.options & ( twCalcMnemonic | twCollapseTilde)) &&
		av &&
		SvTYPE(av) == SVt_PVAV
	)
		av_push((AV*) av, mnemonic2sv(&tw));

	if ( subglyphs ) free(subglyphs);
	if ( log2vis) free(log2vis);
	free( c);

	return ret;
}

SV*
Drawable_text_wrap( Handle self, SV * text, int width, int options, int tabIndent, int from, int len, SV * glyphs)
{
	if ( width < 0 ) width = INT_MAX;
	if ( SvTYPE(glyphs) != SVt_NULL ) {
		return string_glyphs_wrap(self, text, width, options, tabIndent, from, len, glyphs);
	} else if ( !SvROK( text )) {
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
