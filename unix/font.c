/***********************************************************/
/*                                                         */
/*  System dependent font routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

/* these are freed right after use */
static char * do_default_font = NULL;
static char * do_caption_font = NULL;
static char * do_msg_font     = NULL;
static char * do_menu_font    = NULL;
static char * do_widget_font  = NULL;
static Bool   do_xft          = true;
static Bool   do_xft_priority = true;
static Bool   do_freetype     = true;
static Bool   do_harfbuzz     = true;

#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding
#define MY_MATRIX (PDrawable(self)->current_state.matrix)

void
prima_fill_default_font( Font * font )
{
	bzero( font, sizeof( Font));
	strcpy( font-> name, "Default");
	font-> size   = 12;
	font-> style  = fsNormal;
	font-> pitch  = fpDefault;
	font-> vector = fvDefault;
	font-> undef. height = font-> undef. width = font-> undef. vector = 1;
}

Bool
prima_font_init_subsystem(void)
{
	guts. font_hash = hash_create();

	(void) do_freetype;
#ifdef USE_FONTQUERY
	prima_fc_init();
	if ( do_freetype)
		do_freetype = prima_ft_init();
#endif
	guts. use_harfbuzz  = do_harfbuzz;
	return true;
}

Bool
prima_font_init_x11( char * error_buf)
{
	if ( !prima_corefont_init(error_buf))
		return false;

	guts. xft_priority     = do_xft_priority;

	(void) do_xft;
#ifdef USE_XFT
	if ( do_xft) prima_xft_init();
#endif

	prima_corefont_pp2font( "fixed", NULL);
	Fdebug("font: init");
	if ( do_default_font) {
		XrmPutStringResource( &guts.db, "Prima.font", do_default_font);
		prima_corefont_pp2font( do_default_font, &guts. default_font);
		free( do_default_font);
		do_default_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "font",
		NULL_HANDLE, frFont, &guts. default_font)) {
		prima_fill_default_font( &guts. default_font);
		prima_font_pick( &guts. default_font, NULL, NULL, 0 );
		guts. default_font. pitch = fpDefault;
		/*
			Although apc_font_pick() does respect $LANG, it does not always picks
			up a font with the correct encoding here, because we use a hard-coded
			string "Default". Whereas users can set an alias for "Default",
			or set the default font via XRDB:Prima.font option, it is not done by
			default, so here we pick such a font that contains the user-specified
			encoding, and has more or less reasonable metrics.
		*/
		if ( guts. locale[0] && (strcmp( guts. locale, guts. default_font. encoding) != 0)) {
			prima_corefont_pick_default_font_with_encoding();
		}
	}
	guts. default_font_ok = 1;
	Fdebug("default font: %d.[w=%d,s=%g].%s.%s", DEBUG_FONT(guts.default_font));
	if ( do_menu_font) {
		XrmPutStringResource( &guts.db, "Prima.menu_font", do_menu_font);
		prima_corefont_pp2font( do_menu_font, &guts. default_menu_font);
		free( do_menu_font);
		do_menu_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "menu_font",
		NULL_HANDLE, frFont, &guts. default_menu_font)) {
		memcpy( &guts. default_menu_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("menu font: %d.[w=%d,s=%g].%s.%s", DEBUG_FONT(guts.default_menu_font));

	if ( do_widget_font) {
		XrmPutStringResource( &guts.db, "Prima.widget_font", do_widget_font);
		prima_corefont_pp2font( do_widget_font, &guts. default_widget_font);
		free( do_widget_font);
		do_widget_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "widget_font",
		NULL_HANDLE, frFont, &guts. default_widget_font)) {
		memcpy( &guts. default_widget_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("widget font: %d.[w=%d,s=%g].%s.%s", DEBUG_FONT(guts.default_widget_font));

	if ( do_msg_font) {
		XrmPutStringResource( &guts.db, "Prima.message_font", do_widget_font);
		prima_corefont_pp2font( do_msg_font, &guts. default_msg_font);
		free( do_msg_font);
		do_msg_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "message_font",
		NULL_HANDLE, frFont, &guts. default_msg_font)) {
		memcpy( &guts. default_msg_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("msg font: %d.[w=%d,s=%g].%s.%s", DEBUG_FONT(guts.default_msg_font));

	if ( do_caption_font) {
		XrmPutStringResource( &guts.db, "Prima.caption_font", do_widget_font);
		prima_corefont_pp2font( do_caption_font, &guts. default_caption_font);
		free( do_caption_font);
		do_caption_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "caption_font",
		NULL_HANDLE, frFont, &guts. default_caption_font)) {
		memcpy( &guts. default_caption_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("caption font: %d.[w=%d,s=%g].%s.%s", DEBUG_FONT(guts.default_caption_font));
#ifdef USE_XFT
	if ( do_xft) prima_xft_init_font_substitution();
#endif

	return true;
}

Bool
prima_font_subsystem_set_option( char * option, char * value)
{
	if ( prima_corefont_set_option( option, value))
		return true;

#ifdef USE_XFT
	if ( strcmp( option, "no-xft") == 0) {
		if ( value) warn("`--no-xft' option has no parameters");
		do_xft = false;
		return true;
	} else
	if ( strcmp( option, "font-priority") == 0) {
		if ( !value) {
			warn("`--font-priority' must be given parameters, either 'core' or 'xft'");
			return false;
		}
		if ( strcmp( value, "core") == 0)
			do_xft_priority = false;
		else if ( strcmp( value, "xft") == 0)
			do_xft_priority = true;
		else
			warn("Invalid value '%s' to `--font-priority' option. Valid are 'core' and 'xft'", value);
		return true;
	} else
#endif
#ifdef WITH_HARFBUZZ
	if ( strcmp( option, "no-harfbuzz") == 0) {
		if ( value) warn("`--no-harfbuzz' option has no parameters");
		do_harfbuzz = false;
		return true;
	} else
#endif
#ifdef USE_FONTQUERY
	if ( strcmp( option, "no-freetype") == 0) {
		if ( value) warn("`--no-freetype' option has no parameters");
		do_freetype = false;
		return true;
	} else
#endif
	if ( strcmp( option, "font") == 0) {
		free( do_default_font);
		do_default_font = duplicate_string( value);
		Mdebug( "set default font: %s", do_default_font);
		return true;
	} else
	if ( strcmp( option, "menu-font") == 0) {
		free( do_menu_font);
		do_menu_font = duplicate_string( value);
		Mdebug( "set menu font: %s", do_menu_font);
		return true;
	} else
	if ( strcmp( option, "widget-font") == 0) {
		free( do_widget_font);
		do_widget_font = duplicate_string( value);
		Mdebug( "set menu font: %s", do_widget_font);
		return true;
	} else
	if ( strcmp( option, "msg-font") == 0) {
		free( do_msg_font);
		do_msg_font = duplicate_string( value);
		Mdebug( "set msg font: %s", do_msg_font);
		return true;
	} else
	if ( strcmp( option, "caption-font") == 0) {
		free( do_caption_font);
		do_caption_font = duplicate_string( value);
		Mdebug( "set caption font: %s", do_caption_font);
		return true;
	}
	return false;
}

static void
free_cached_font( PCachedFont f)
{
	switch (f-> type) {
	case FONTKEY_CORE:
		prima_corefont_free_cached_font(f);
		break;
#ifdef USE_FONTQUERY
	case FONTKEY_FREETYPE:
		prima_fq_free_cached_font(f);
		break;
#endif
	/* XFT is static */
	}
	free( f);
}

static Bool
free_cached_entry( PCachedFont f, int keyLen, void * key, void * dummy)
{
	if ( --f-> ref_cnt > 0 )
		return false;

	free_cached_font(f);
	return false;
}

static Bool
cleanup_cached_entry( PCachedFont f, int keyLen, void * key, void * dummy)
{
	if ( f-> lock_cnt > 0 )
		return false;

	hash_delete(guts.font_hash, key, keyLen, false);
	/* shared hash value by some other key */
	if ( --f-> ref_cnt > 0 )
		return false;

	free_cached_font(f);
	return hash_count(guts.font_hash) < ( MAX_CACHED_FONTS / 2 );
}

void
prima_font_cleanup_subsystem( void)
{
	if ( DISP ) {
		prima_corefont_done();
#ifdef USE_XFT
		if (do_xft) prima_xft_done();
#endif

	}
#ifdef USE_FONTQUERY
	if (do_freetype) prima_ft_done();
	prima_fc_done();
#endif
	if (guts.font_hash) {
		hash_first_that(guts.font_hash, (void*)free_cached_entry, NULL, NULL, NULL);
		hash_destroy(guts.font_hash, false);
		guts.font_hash = NULL;
	}
}

const char *
prima_font_debug_style(int style)
{
	static char buf[256];
	char * p = buf;

	if ( style & fsBold) *p++ = 'B';
	if ( style & fsThin) *p++ = 'T';
	if ( style & fsItalic) *p++ = 'I';
	if ( style & fsUnderlined) *p++ = 'U';
	if ( style & fsStruckOut) *p++ = 'S';
	if ( style & fsOutline) *p++ = 'O';
	if ( style & ~fsMask) *p++ = '+';
	if ( style == 0) *p++ = '0';
	*p++ = 0;

	return buf;
}

void
prima_font_init_try_height( HeightGuessStack * p, int target, int firstMove )
{
	p-> locked = 0;
	p-> sp     = 1;
	p-> target = target;
	p-> xlfd[0] = firstMove;
}

int
prima_font_try_height( HeightGuessStack * p, int height)
{
	int ret = -1;

	if ( p-> locked != 0) return p-> locked;
	if ( p-> sp >= MAX_HGS_SIZE) goto DONT_ADVISE;

	if ( p-> sp > 1) {
		int x0 = p-> xlfd[ p-> sp - 2];
		int x1 = p-> xlfd[ p-> sp - 1];
		int y0 = p-> prima[ p-> sp - 2];
		int y1 = p-> prima[ p-> sp - 1] = height;
		int d0 = y0 - p-> target;
		int d1 = y1 - p-> target;
		if (( d0 < 0 && d1 < 0 && d1 >= d0) || ( d0 > 0 && d1 > 0 && d1 <= d0)) { /* underflow */
			int sp = p-> sp - 2;
			while ( d1 == d0 && sp-- > 0) { /* not even moved */
				x0 = p-> xlfd[ sp];
				y0 = p-> prima[ sp];
				d0 = y0 - p-> target;
			}
			if ( d1 != d0) {
				ret = x0 + ( x1 - x0) * ( p-> target - y0) / ( y1 - y0) + 0.5;
				if ( ret == x1 ) ret += ( d1 < 0) ? 1 : -1;
				if ( ret < 1 ) ret = 1;
			}
		} else if ((( d0 < 0 && d1 > 0) || ( d0 > 0 && d1 < 0)) && ( abs(x0 - x1) > 1)) { /* overflow */
			ret = x0 + ( x1 - x0) * ( p-> target - y0) / ( y1 - y0) + 0.5;
			if ( ret == x0 ) ret += ( d0 < 0) ? 1 : -1; else
			if ( ret == x1 ) ret += ( d1 < 0) ? 1 : -1;
			if ( ret < 1 ) ret = 1;
		}
		/* printf("-- [%d=>%d],[%d=>%d] (%d %d)\n", x0, y0, x1, y1, d0, d1); */
	} else {
		if ( height > 0) /* sp == 0 */
			ret = p-> xlfd[0] * p-> target / height;
		p-> prima[ p-> sp - 1] = height;
	}

	if ( ret > 0) {
		int i;
		for ( i = 0; i < p-> sp; i++)
			if ( p-> xlfd[ i] == ret) { /* advised already? */
				ret = -1;
				break;
			}
	}

	p-> xlfd[ p-> sp] = ret;

	p-> sp++;

DONT_ADVISE:
	if ( ret < 0) { /* select closest match */
		int i, best_i = -1, best_diff = INT_MAX, diff, sp = p-> sp - 1;
		for ( i = 0; i < sp; i++) {
			diff = p-> target - p-> prima[i];
			if ( diff < 0) diff += 1000;
			if ( best_diff > diff) {
				if ( p-> xlfd[i] < 0 ) continue;
				best_diff = diff;
				best_i = i;
			}
		}
		if ( best_i >= 0 && p-> xlfd[best_i] != p-> xlfd[ sp - 1])
			ret = p-> xlfd[best_i];
		p-> locked = -1;
	}

	return ret;
}


PFont
apc_font_default( PFont f)
{
	memcpy( f, &guts. default_font, sizeof( Font));
	return f;
}

int
apc_font_load( Handle self, char* filename)
{
#ifdef USE_FONTQUERY
	return prima_fc_load_font(filename);
#else
	return 0;
#endif
}

static void
dump_font( PFont f)
{
	if ( !DISP) return;
	fprintf( stderr, "*** BEGIN FONT DUMP ***\n");
	fprintf( stderr, "height    : %d%s\n"  , f-> height   , f->undef.height    ? "/undef" : "");
	fprintf( stderr, "width     : %d%s\n"  , f-> width    , f->undef.width     ? "/undef" : "");
	fprintf( stderr, "style     : %d%s\n"  , f-> style    , f->undef.style     ? "/undef" : "");
	fprintf( stderr, "pitch     : %d%s\n"  , f-> pitch    , f->undef.pitch     ? "/undef" : "");
	fprintf( stderr, "direction : %g%s\n"  , f-> direction, f->undef.direction ? "/undef" : "");
	fprintf( stderr, "name      : %s%s\n"  , f-> name     , f->undef.name      ? "/undef" : "");
	fprintf( stderr, "size      : %g%s\n"  , f-> size     , f->undef.size      ? "/undef" : "");
	fprintf( stderr, "*** END FONT DUMP ***\n");
}

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
	return prima_font_pick(
		source, self ? MY_MATRIX : NULL, dest,
		self && is_opt(optInFontQuery) ? FONTKEY_FREETYPE : FONTKEY_DEFAULT
	) != NULL;
}

static void
build_font_key(PFontKey fk, PFont source, PFont save_synthetics, Matrix matrix, int type)
{
	Bool by_size = source->undef.height;
	Font sanitized = *source;

	sanitized.pitch  &= fpMask;
	sanitized.vector &= fvMask;
	sanitized.style  &= fsMask & ~(fsUnderlined|fsOutline|fsStruckOut);

	if ( save_synthetics )
		*save_synthetics = *source;

	bzero(fk, sizeof(FontKey));
	fk->type = type;
	switch (type) {
	case FONTKEY_CORE:
		prima_corefont_build_key( fk, &sanitized, by_size );
		return;
#ifdef USE_XFT
	case FONTKEY_XFT:
		prima_xft_build_key( fk, &sanitized, matrix, by_size);
		return;
#endif
#ifdef USE_FONTQUERY
	case FONTKEY_FREETYPE:
		prima_fq_build_key( fk, &sanitized);
		return;
#endif
	}
}

static void
apply_synthetic_fields(PCachedFont kf, PFont s, PFont d)
{
	d-> style |= s->style & (fsUnderlined|fsOutline|fsStruckOut);
	d-> direction = s->direction;
	switch (kf->type) {
#ifdef USE_FONTQUERY
	case FONTKEY_FREETYPE:
		prima_fq_apply_synthetic_fields(kf, s, d);
		d->size = FONT_SIZE_ROUND(d->size);
		break;
#endif
	}
}

static PCachedFont
match_font( PFontKey fk, PFont font, Matrix matrix)
{
	PCachedFont cf, cf2;
	Font s = *font;
	Bool by_size     = font->undef.height;

	if ( !( cf = malloc(sizeof(CachedFont)))) {
		warn("no memory");
		return NULL;
	}
	bzero(cf, sizeof(CachedFont));
	cf->type = fk->type;

	font-> pitch  &= fpMask;
	font-> vector &= fvMask;
	font-> style  &= fsMask & ~(fsUnderlined|fsOutline|fsStruckOut);

	switch (fk-> type) {
	case FONTKEY_CORE:
		cf2 = prima_corefont_match( font, by_size, cf );
		break;
#ifdef USE_XFT
	case FONTKEY_XFT:
		cf2 = prima_xft_match( font, matrix, by_size, cf );
		break;
#endif
#ifdef USE_FONTQUERY
	case FONTKEY_FREETYPE:
		cf2 = prima_fq_match( font, by_size, cf );
		break;
#endif
	default:
		return NULL;
	}

	if ( !cf2 ) {
		free(cf);
		return NULL;
	} else if ( cf2 == cf ) {
		/* the new matched font needs to fill all fields */
		cf->type = fk->type;
		bzero(&font->undef, sizeof(font->undef));
		cf->font = *font;
	} else
		cf = cf2;

	/* Restore the flags that are either not used by backends, or must be the same anyway.
	This is a bit of hack because we know what flags the backends won't use */
	apply_synthetic_fields(cf, &s, font); 

	return cf;
}

static Bool
store_font( PFont font, Matrix matrix, int type, PCachedFont cf)
{
	FontKey fk;
	build_font_key(&fk, font, NULL, matrix, type);
	if (hash_fetch( guts.font_hash, &fk, sizeof( FontKey)))
		return false;
	cf->ref_cnt++;
	hash_store( guts.font_hash, &fk, sizeof( FontKey), cf);
	return true;
}

static PCachedFont
find_font( int type, PFont font, Matrix matrix)
{
	Font f, s;
	FontKey fk;
	PCachedFont cf;
	Bool want_default_pitch = font->pitch == fpDefault;
	Bool by_size = font->undef.height;

	/* find by font */
	build_font_key(&fk, font, &s, matrix, type);
	if (( cf = hash_fetch( guts. font_hash, &fk, sizeof( FontKey))) != NULL) {
		*font = cf->font;
		apply_synthetic_fields( cf, &s, font );
		return cf;
	}
	if (!( cf = match_font(&fk, font, matrix)))
		return NULL;
	cf->ref_cnt++;

	if ( hash_count(guts.font_hash) > MAX_CACHED_FONTS)
		hash_first_that(guts.font_hash, (void*)cleanup_cached_entry, NULL, NULL, NULL);

	/* cache font */
	hash_store( guts.font_hash, &fk, sizeof( FontKey), cf);

	f = *font;
	if ( by_size ) {
		/* cache by size */
		f.undef.height = 1;
		f.height = 1;
	} else {
		/* cache by height */
		f.undef.size = 1;
		f.size = 1;
	}
	store_font( &f, matrix, type, cf);

	if ( want_default_pitch && font->pitch != fpDefault ) {
		/* cache by default pitch too */
		f = *font;
		f.pitch = fpDefault;
		if ( by_size ) {
			f.undef.height = 1;
			f.height = 1;
		} else {
			f.undef.size = 1;
			f.size = 1;
		}
		store_font( &f, matrix, type, cf);
	}

	return cf;
}

PCachedFont
prima_font_pick( PFont source, Matrix matrix, PFont dest, unsigned int selection )
{
	if ( selection == 0 )
		selection = FONTKEY_DEFAULT;

	if ( dest != NULL )
		Drawable_font_add( NULL_HANDLE, source, dest );
	else
		dest = source;

#ifdef USE_FONTQUERY
	if ( selection & FONTKEY_FREETYPE )
		return find_font( FONTKEY_FREETYPE, dest, matrix );
#endif

#ifdef USE_XFT
	if ( guts. use_xft && (selection & FONTKEY_XFT) ) {
		PCachedFont cf;
		if (( cf = find_font( FONTKEY_XFT, dest, matrix)) != NULL)
			return cf;
	}
#endif

	if ( selection & FONTKEY_CORE)
		return find_font( FONTKEY_CORE, dest, matrix );

	return NULL;
}

PFont
apc_fonts( Handle self, const char *facename, const char * encoding, int *retCount)
{
	PFont fmtx;

#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) )
		return prima_fc_fonts( NULL, facename, encoding, retCount);
#endif

	fmtx = prima_corefont_fonts( self, facename, encoding, retCount);
#ifdef USE_XFT
	if ( guts. use_xft)
		fmtx = prima_fc_fonts( fmtx, facename, encoding, retCount);
#endif
	return fmtx;
}

PHash
apc_font_encodings( Handle self )
{
	PHash hash = hash_create();
	if ( !hash) return NULL;

#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) ) {
		prima_fc_font_encodings( hash);
		return hash;
	}
#endif

#ifdef USE_XFT
	if ( guts. use_xft)
		prima_fc_font_encodings( hash);
#endif
	prima_corefont_encodings(hash);
	return hash;
}

static Bool
set_font( Handle self, int type, PFont font)
{
	DEFXX;
	PCachedFont kf;

	if ( !( kf = find_font( type, font, MY_MATRIX)))
		return false;

	switch (type) {
	case FONTKEY_CORE:
		if ( XX-> flags. paint) {
			XSetFont( DISP, XX-> gc, kf-> id);
			XCHECKPOINT;
		}
		break;
#ifdef USE_XFT
	case FONTKEY_XFT:
		prima_fc_set_font( self, font );
		break;
#endif
#ifdef USE_FONTQUERY
	case FONTKEY_FREETYPE:
		prima_fq_set_font( self, kf );
		break;
#endif
	}

	if ( XX-> font ) XX-> font-> lock_cnt--;
	XX->font = kf;
	kf->lock_cnt++;

	return true;
}

Bool
apc_gp_set_font( Handle self, PFont font)
{
#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) ) {
		if (set_font( self, FONTKEY_FREETYPE, font))
			return true;
		dump_font( font);
		return false;
	}
#endif

#ifdef USE_XFT
	if ( guts. use_xft && set_font( self, FONTKEY_XFT, font))
		return true;
#endif

	if ( set_font(self, FONTKEY_CORE, font))
		return true;
	dump_font( font);
	return false;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
	DEFMM;
	Bool xft_metrics = 0;
	PCachedFont kf = NULL;
	FontKey fk;

	font-> direction = 0; /* skip unnecessary logic */

#ifdef USE_XFT
	if ( guts. use_xft) {
		build_font_key(&fk, font, NULL, NULL, FONTKEY_XFT);
		if (( kf = hash_fetch( guts. font_hash, &fk, sizeof( FontKey))) != NULL)
			xft_metrics = 1;
	}
#endif

	if ( !kf) {
		build_font_key(&fk, font, NULL, NULL, FONTKEY_CORE);
		if (( kf = hash_fetch( guts. font_hash, &fk, sizeof( FontKey))) == NULL ) {
			dump_font( font);
			return false;
		}
	}

	if ( XX->font ) XX->font->lock_cnt--;
	XX-> font = kf;
	kf-> lock_cnt++;

	if ( !xft_metrics) {
		XX-> guillemots = XTextWidth( kf-> fs, ">>", 2);
	} else {
#ifdef USE_XFT
		XX-> guillemots = prima_xft_get_text_width( kf, ">>", 2, toAddOverhangs, NULL, NULL);
#endif
	}
	if ( !XX-> type. popup && X_WINDOW) {
		if (( kf-> font. height + 4) != X(PComponent(self)-> owner)-> menuHeight) {
			prima_window_reset_menu( PComponent(self)-> owner, kf-> font. height + MENU_ITEM_GAP * 2);
			XResizeWindow( DISP, X_WINDOW, XX-> w-> sz.x, XX-> w-> sz.y = kf-> font. height + MENU_ITEM_GAP * 2);
		} else if ( !XX-> paint_pending) {
			XClearArea( DISP, X_WINDOW, 0, 0, XX-> w-> sz.x, XX-> w-> sz.y, true);
			XX-> paint_pending = true;
		}
	}

	return true;
}

int
apc_gp_get_glyph_outline( Handle self, unsigned int index, unsigned int flags, int ** buffer)
{

#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) ) {
		if ( X(self)->font )
			return prima_fq_get_glyph_outline( self, index, flags, buffer);
		return -1;
	}
#endif

#ifdef USE_XFT
	if ( guts.use_xft && X(self)-> font-> xft)
		return prima_xft_get_glyph_outline( self, index, flags, buffer);
#endif
	return -1;
}

unsigned long *
apc_gp_get_mapper_ranges(PFont font, int * count, unsigned int * flags)
{
#ifdef USE_FONTQUERY
	/* exactly same as the xft version buf libXft caching is more efficient */
	if ( !DISP )
		return prima_fq_mapper_query_ranges(font, count, flags);
#endif
#ifdef USE_XFT
	if ( do_xft )
		return prima_xft_mapper_query_ranges(font, count, flags);
#endif
	*count = 0;
	*flags = 0;
	return NULL;
}

Byte*
apc_font_get_glyph_bitmap( Handle self, uint16_t index, unsigned int flags, PPoint offset, PPoint size, int *advance)
{
#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) ) {
		if ( X(self)->font)
			return prima_fq_get_glyph_bitmap(self, index, flags, offset, size, advance);
		return NULL;
	}
#endif

#ifdef USE_XFT
	if ( X(self)-> font-> xft)
		return prima_xft_get_glyph_bitmap(self, index, flags, offset, size, advance);
#endif

	return prima_corefont_get_glyph_bitmap(self, index, flags & ggoMonochrome, offset, size, advance);
}

Bool
apc_gp_set_text_matrix( Handle self, Matrix matrix)
{
	PFont f = & PDrawable(self)->font;
	if (f->undef.height &&f->undef.size) /* too early */
		return true;
	return apc_gp_set_font( self, f);
}

unsigned long *
apc_gp_get_font_ranges( Handle self, int * count)
{
	DEFXX;
	unsigned long * ret = NULL;
	XFontStruct * fs;

#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) )
		return prima_fq_get_font_ranges( self, count);
#endif

#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_get_font_ranges( self, count);
#endif
	fs = XX-> font-> fs;
	*count = (fs-> max_byte1 - fs-> min_byte1 + 1) * 2;
	if (( ret = malloc( sizeof( unsigned long) * ( *count)))) {
		int i;
		for ( i = fs-> min_byte1; i <= fs-> max_byte1; i++) {
			ret[(i - fs-> min_byte1) * 2 + 0] = i * 256 + fs-> min_char_or_byte2;
			ret[(i - fs-> min_byte1) * 2 + 1] = i * 256 + fs-> max_char_or_byte2;
		}
	}
	return ret;
}

char *
apc_gp_get_font_languages( Handle self)
{
	DEFXX;
	char * ret;
#ifdef USE_FONTQUERY
	if ( is_opt(optInFontQuery) )
		return prima_fq_get_font_languages( self);
#endif
#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_get_font_languages(self);
#endif
	if ( XX-> font-> flags.funky )
		return NULL;
	if ( !( ret = malloc(4)))
		return NULL;
	memcpy(ret, "en\0\0", 4);
	return ret;
}


Bool
apc_font_begin_query( Handle self )
{
#ifdef USE_FONTQUERY
	if ( !do_freetype)
		return false;
	return prima_fq_begin_query(self);
#endif
	return false;
}

void
apc_font_end_query( Handle self )
{
	DEFXX;
	if (XX->font)
		XX->font->lock_cnt--;
	XX->font = NULL;
}


