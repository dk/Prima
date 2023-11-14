#include "apricot.h"
#include "guts.h"
#include "Drawable.h"
#include "Drawable_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/*

FONT MAPPER

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


List  font_active_entries;
List  font_passive_entries;
PHash font_substitutions;
int   font_mapper_default_id[N_STYLES];

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

char *
Drawable_font_key( const char * name, unsigned int style)
{
	static char buf[2048];
	if ( !name ) return NULL;
	buf[0] = '0' + (style & STYLE_MASK);
	strlcpy( buf + 1, name, 2046 );
	return buf;
}

PFont
prima_font_mapper_save_font(const char * name, unsigned int style)
{
	PPassiveFontEntry p;
	PFont f;
	char * key;

	key = Drawable_font_key(name, style);
	if ( name && PTR2IV(hash_fetch(font_substitutions, key, strlen(key))) != 0)
		return NULL;

	if ( !( p = malloc(sizeof(PassiveFontEntry)))) {
		warn("not enough memory\n");
		return NULL;
	}
	bzero(p, sizeof(PassiveFontEntry));
	p->is_enabled = true;
	f = &p->font;
	memset( &f->undef, 0xff, sizeof(f->undef));
	f->undef.encoding = 0; /* needs enforcing */
	if (name) {
		f->undef.name = 0;
		strlcpy(f->name, name, 256);
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

void
Drawable_query_ranges(PPassiveFontEntry pfe)
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

	if ( !pfe-> is_enabled)
		return false;

	if ( !pfe-> ranges_queried )
		Drawable_query_ranges(pfe);

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

unsigned int
Drawable_find_font(uint32_t c, int pitch, int style, uint16_t preferred_font)
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

	def_style = (style >= 0) ? (style & STYLE_MASK) : 0;
	if ( font_mapper_default_id[def_style] == -1 ) {
		Font font;
		char *key;
		uint16_t fid;
		apc_font_default( &font);
		font_mapper_default_id[def_style] = -2;
		key = Drawable_font_key(font.name, def_style);
		fid = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));
		if ( fid > 0 ) 
			font_mapper_default_id[def_style] = fid;
	}

	if ( font_mapper_default_id[def_style] >= 0 && can_substitute(c, pitch, font_mapper_default_id[def_style])) 
		return font_mapper_default_id[def_style];

	for ( i = 1; i < font_passive_entries.count; i++) {
		PPassiveFontEntry pfe = PASSIVE_FONT(i);
		if ( !pfe-> is_enabled)
			continue;
		if ( style >= 0 && pfe-> font.style != style )
			continue;
		if ( can_substitute(c, pitch, i))
			return i;
	}

	if ( pitch == fpFixed ) {
		if ( font_mapper_default_id[def_style] >= 0 && can_substitute(c, pitch, font_mapper_default_id[def_style]))
			return font_mapper_default_id[def_style];
		for ( i = 1; i < font_passive_entries.count; i++) {
			PPassiveFontEntry pfe = PASSIVE_FONT(i);
			if ( !pfe-> is_enabled)
				continue;
			if ( style >= 0 && pfe-> font.style != style )
				continue;
			if ( can_substitute(c, fpDefault, i))
				return i;
		}
	}

	if ( style >= 0 ) {
		if ( style & fsThin )
			return Drawable_find_font(c, pitch, style & ~fsThin, preferred_font);
		if ( style & fsBold )
			return Drawable_find_font(c, pitch, style & ~fsBold, preferred_font);
		if ( style & fsItalic )
			return Drawable_find_font(c, pitch, style & ~fsItalic, preferred_font);
		return Drawable_find_font(c, pitch, -1, preferred_font);
	}

#ifdef _DEBUG
	printf("cannot map chr(%x)\n", c);
#endif
	return 0;
}

int
prima_font_mapper_action(int action, PFont font)
{
	uint16_t i, fid;
	char * key;
	PPassiveFontEntry pfe;

	switch (action) {
	case pfmaIsActive:
	case pfmaActivate:
	case pfmaPassivate:
	case pfmaIsEnabled:
	case pfmaEnable:
	case pfmaDisable:
	case pfmaGetIndex:
		key = Drawable_font_key(font->name, font->style);
		fid = PTR2IV(hash_fetch(font_substitutions, key, strlen(key)));
		if ( fid == 0 ) return -1;
		pfe = PASSIVE_FONT(fid);
		break;
	case pfmaGetCount:
		return font_passive_entries.count;
		break;
	default:
		return -1;
	}

	switch (action) {
	case pfmaIsActive:
		return pfe-> is_active;
	case pfmaPassivate:
		if ( !pfe-> is_active ) return 0;
		remove_active_font(fid);
		return 1;
	case pfmaActivate:
		if ( pfe-> is_active || !pfe-> is_enabled ) return 0;
		add_active_font(fid);
		return 1;
	case pfmaIsEnabled:
		return pfe-> is_enabled;
	case pfmaEnable:
		if ( pfe-> is_enabled ) return 0;
		pfe-> is_enabled = 1;
		return 1;
	case pfmaDisable:
		if ( !pfe-> is_enabled ) return 0;
		if ( pfe-> is_active ) remove_active_font(fid);
		pfe-> is_enabled = 0;
		for ( i = 0; i < N_STYLES; i++)
			if ( font_mapper_default_id[i] == fid )
				font_mapper_default_id[i] = -1;
		return 1;
	case pfmaGetIndex:
		return fid;
	}

	return -1;
}

Bool
Drawable_switch_font( Handle self, uint16_t fid)
{
	Font src, dst;
	src = PASSIVE_FONT(fid)->font;
	if ( is_opt(optSystemDrawable) || is_opt(optInFontQuery) ) {
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


#ifdef __cplusplus
}
#endif
