/***********************************************************/
/*                                                         */
/*  System dependent font routines (unix, x11)             */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"

/* these are freed right after use */
static char * do_default_font = NULL;
static char * do_caption_font = NULL;
static char * do_msg_font = NULL;
static char * do_menu_font = NULL;
static char * do_widget_font = NULL;
static Bool   do_xft = true;
static Bool   do_xft_no_antialias = false;
static Bool   do_xft_priority = true;
static Bool   do_harfbuzz = true;

#define DEBUG_FONT(font) font.height,font.width,font.size,font.name,font.encoding

void
prima_fill_default_font( Font * font )
{
	bzero( font, sizeof( Font));
	strcpy( font-> name, "Default");
	font-> size = 12;
	font-> style = fsNormal;
	font-> pitch = fpDefault;
	font-> vector = fvDefault;
	font-> undef. height = font-> undef. width = font-> undef. vector = 1;
}

Bool
prima_init_font_subsystem( char * error_buf)
{
	if ( !prima_corefont_init(error_buf))
		return false;

	guts. xft_no_antialias = do_xft_no_antialias;
	guts. xft_priority     = do_xft_priority;
#ifdef USE_XFT
	if ( do_xft) prima_xft_init();
#endif
	guts. use_harfbuzz     = do_xft && do_harfbuzz;

	prima_corefont_pp2font( "fixed", NULL);
	Fdebug("font: init\n");
	if ( do_default_font) {
		XrmPutStringResource( &guts.db, "Prima.font", do_default_font);
		prima_corefont_pp2font( do_default_font, &guts. default_font);
		free( do_default_font);
		do_default_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "font",
		NULL_HANDLE, frFont, &guts. default_font)) {
		prima_fill_default_font( &guts. default_font);
		apc_font_pick( prima_guts.application, &guts. default_font, &guts. default_font);
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
	Fdebug("default font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_font));
	if ( do_menu_font) {
		XrmPutStringResource( &guts.db, "Prima.menu_font", do_menu_font);
		prima_corefont_pp2font( do_menu_font, &guts. default_menu_font);
		free( do_menu_font);
		do_menu_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "menu_font",
		NULL_HANDLE, frFont, &guts. default_menu_font)) {
		memcpy( &guts. default_menu_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("menu font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_menu_font));

	if ( do_widget_font) {
		XrmPutStringResource( &guts.db, "Prima.widget_font", do_widget_font);
		prima_corefont_pp2font( do_widget_font, &guts. default_widget_font);
		free( do_widget_font);
		do_widget_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "widget_font",
		NULL_HANDLE, frFont, &guts. default_widget_font)) {
		memcpy( &guts. default_widget_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("widget font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_widget_font));

	if ( do_msg_font) {
		XrmPutStringResource( &guts.db, "Prima.message_font", do_widget_font);
		prima_corefont_pp2font( do_msg_font, &guts. default_msg_font);
		free( do_msg_font);
		do_msg_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "message_font",
		NULL_HANDLE, frFont, &guts. default_msg_font)) {
		memcpy( &guts. default_msg_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("msg font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_msg_font));

	if ( do_caption_font) {
		XrmPutStringResource( &guts.db, "Prima.caption_font", do_widget_font);
		prima_corefont_pp2font( do_caption_font, &guts. default_caption_font);
		free( do_caption_font);
		do_caption_font = NULL;
	} else if ( !apc_fetch_resource( "Prima", "", "Font", "caption_font",
		NULL_HANDLE, frFont, &guts. default_caption_font)) {
		memcpy( &guts. default_caption_font, &guts. default_font, sizeof( Font));
	}
	Fdebug("caption font: %d.[w=%d,s=%d].%s.%s\n", DEBUG_FONT(guts.default_caption_font));
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
	if ( strcmp( option, "no-aa") == 0) {
		if ( value) warn("`--no-antialias' option has no parameters");
		do_xft_no_antialias = true;
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
	if ( strcmp( option, "font") == 0) {
		free( do_default_font);
		do_default_font = duplicate_string( value);
		Mdebug( "set default font: %s\n", do_default_font);
		return true;
	} else
	if ( strcmp( option, "menu-font") == 0) {
		free( do_menu_font);
		do_menu_font = duplicate_string( value);
		Mdebug( "set menu font: %s\n", do_menu_font);
		return true;
	} else
	if ( strcmp( option, "widget-font") == 0) {
		free( do_widget_font);
		do_widget_font = duplicate_string( value);
		Mdebug( "set menu font: %s\n", do_widget_font);
		return true;
	} else
	if ( strcmp( option, "msg-font") == 0) {
		free( do_msg_font);
		do_msg_font = duplicate_string( value);
		Mdebug( "set msg font: %s\n", do_msg_font);
		return true;
	} else
	if ( strcmp( option, "caption-font") == 0) {
		free( do_caption_font);
		do_caption_font = duplicate_string( value);
		Mdebug( "set caption font: %s\n", do_caption_font);
		return true;
	}
	return false;
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

PFont
apc_font_default( PFont f)
{
	memcpy( f, &guts. default_font, sizeof( Font));
	return f;
}

int
apc_font_load( Handle self, char* filename)
{
#ifdef USE_XFT
	return prima_xft_load_font(filename);
#else
	return 0;
#endif
}

static void
dump_font( PFont f)
{
	if ( !DISP) return;
	fprintf( stderr, "*** BEGIN FONT DUMP ***\n");
	fprintf( stderr, "height: %d\n", f-> height);
	fprintf( stderr, "width: %d\n", f-> width);
	fprintf( stderr, "style: %d\n", f-> style);
	fprintf( stderr, "pitch: %d\n", f-> pitch);
	fprintf( stderr, "direction: %g\n", f-> direction);
	fprintf( stderr, "name: %s\n", f-> name);
	fprintf( stderr, "family: %s\n", f-> family);
	fprintf( stderr, "size: %d\n", f-> size);
	fprintf( stderr, "*** END FONT DUMP ***\n");
}

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
#ifdef USE_XFT
	if ( guts. use_xft) {
		if ( prima_xft_font_pick( self, source, dest, NULL, NULL, NULL))
			return true;
	}
#endif
	return prima_corefont_pick( self, source, dest);
}


static PFont
spec_fonts( int *retCount)
{
	int i, count = guts. n_fonts;
	PFontInfo info = guts. font_info;
	PFont fmtx = NULL;
	List list;
	PHash hash = NULL;

	list_create( &list, 256, 256);

	*retCount = 0;


	if ( !( hash = hash_create())) {
		list_destroy( &list);
		return NULL;
	}

	/* collect font info */
	for ( i = 0; i < count; i++) {
		int len;
		PFont fm;
		if ( info[ i]. flags. disabled) continue;

		len = strlen( info[ i].font.name);

		fm = hash_fetch( hash, info[ i].font.name, len);
		if ( fm) {
			char ** enc = (char**) fm-> encoding;
			unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
			if ( *shift + 2 < 256 / sizeof(char*)) {
				int j, exists = 0;
				for ( j = 1; j <= *shift; j++) {
					if ( strcmp( enc[j], info[i].xname + info[i].info_offset) == 0) {
						exists = 1;
						break;
					}
				}
				if ( exists) continue;
				*(enc + ++(*shift)) = info[i].xname + info[i].info_offset;
			}
			continue;
		}

		if ( !( fm = ( PFont) malloc( sizeof( Font)))) {
			if ( hash) hash_destroy( hash, false);
			list_delete_all( &list, true);
			list_destroy( &list);
			return NULL;
		}

		*fm = info[i]. font;

		{ /* multi-encoding format */
			char ** enc = (char**) fm-> encoding;
			unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
			memset( fm-> encoding, 0, 256);
			*(enc + ++(*shift)) = info[i].xname + info[i].info_offset;
			hash_store( hash, info[ i].font.name, strlen( info[ i].font.name), fm);
		}

		list_add( &list, ( Handle) fm);
	}

	if ( hash) hash_destroy( hash, false);

	if ( list. count == 0) goto Nothing;
	fmtx = ( PFont) malloc( list. count * sizeof( Font));
	if ( !fmtx) {
		list_delete_all( &list, true);
		list_destroy( &list);
		return NULL;
	}

	*retCount = list. count;
		for ( i = 0; i < list. count; i++)
			memcpy( fmtx + i, ( void *) list. items[ i], sizeof( Font));
	list_delete_all( &list, true);

Nothing:
	list_destroy( &list);

#ifdef USE_XFT
	if ( guts. use_xft)
		fmtx = prima_xft_fonts( fmtx, NULL, NULL, retCount);
#endif

	return fmtx;
}

PFont
apc_fonts( Handle self, const char *facename, const char * encoding, int *retCount)
{
	int i, count = guts. n_fonts;
	PFontInfo info = guts. font_info;
	PFontInfo * table;
	int n_table;
	PFont fmtx;

	if ( !facename && !encoding) return spec_fonts( retCount);

	*retCount = 0;
	n_table = 0;

	/* stage 1 - browse through names and validate records */
	table = malloc( count * sizeof( PFontInfo));
	if ( !table && count > 0) return NULL;

	if ( facename == NULL) {
		PHash hash = hash_create();
		for ( i = 0; i < count; i++) {
			int len;
			if ( info[ i]. flags. disabled) continue;
			len = strlen( info[ i].font.name);
			if ( hash_fetch( hash, info[ i].font.name, len) ||
				strcmp( info[ i].xname + info[ i].info_offset, encoding) != 0)
				continue;
			hash_store( hash, info[ i].font.name, len, (void*)1);
			table[ n_table++] = info + i;
		}
		hash_destroy( hash, false);
		*retCount = n_table;
	} else {
		for ( i = 0; i < count; i++) {
			if ( info[ i]. flags. disabled) continue;
			if (
				( stricmp( info[ i].font.name, facename) == 0) &&
				(
					!encoding ||
					( strcmp( info[ i].xname + info[ i].info_offset, encoding) == 0)
				)
			) {
				table[ n_table++] = info + i;
			}
		}
		*retCount = n_table;
	}

	fmtx = malloc( n_table * sizeof( Font));
	bzero( fmtx, n_table * sizeof( Font));
	if ( !fmtx && n_table > 0) {
		*retCount = 0;
		free( table);
		return NULL;
	}
	for ( i = 0; i < n_table; i++) {
		fmtx[i] = table[i]-> font;
	}
	free( table);

#ifdef USE_XFT
	if ( guts. use_xft)
		fmtx = prima_xft_fonts( fmtx, facename, encoding, retCount);
#endif

	return fmtx;
}

PHash
apc_font_encodings( Handle self )
{
	PHash hash = hash_create();
	if ( !hash) return NULL;

#ifdef USE_XFT
	if ( guts. use_xft)
		prima_xft_font_encodings( hash);
#endif
	prima_corefont_encodings(hash);
	return hash;
}

Bool
apc_gp_set_font( Handle self, PFont font)
{
	DEFXX;
	Bool reload;
	PCachedFont kf;

#ifdef USE_XFT
	if ( guts. use_xft && prima_xft_set_font( self, font)) return true;
#endif

	kf = prima_corefont_find_known_font( font, false, false);
	if ( !kf || !kf-> id) {
		dump_font( font);
		if ( DISP) warn( "internal error (kf:%p)", kf); /* the font was not cached, can't be */
		return false;
	}

	reload = XX-> font != kf && XX-> font != NULL;

	if ( reload) {
		kf-> refCnt++;
		if ( XX-> font && ( --XX-> font-> refCnt <= 0)) {
			prima_free_rotated_entry( XX-> font);
			XX-> font-> refCnt = 0;
		}
	}

	XX-> font = kf;

	if ( XX-> flags. paint) {
		XX-> flags. reload_font = reload;
		XSetFont( DISP, XX-> gc, XX-> font-> id);
		XCHECKPOINT;
	}

	return true;
}

Bool
apc_menu_set_font( Handle self, PFont font)
{
	DEFMM;
	Bool xft_metrics = 0;
	PCachedFont kf = NULL;

	font-> direction = 0; /* skip unnecessary logic */

#ifdef USE_XFT
	if ( guts. use_xft) {
		kf = prima_xft_get_cache( font, NULL);
		if ( kf) xft_metrics = 1;
	}
#endif

	if ( !kf) {
		kf = prima_corefont_find_known_font( font, false, false);
		if ( !kf || !kf-> id) {
			dump_font( font);
			warn( "internal error (kf:%p)", kf); /* the font was not cached, can't be */
			return false;
		}
	}

	XX-> font = kf;
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
#ifdef USE_XFT
	if ( guts.use_xft && X(self)-> font-> xft)
		return prima_xft_get_glyph_outline( self, index, flags, buffer);
#endif
	return -1;
}

unsigned long *
apc_gp_get_mapper_ranges(PFont font, int * count, unsigned int * flags)
{
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
	DEFXX;
#ifdef USE_XFT
	if ( XX-> font-> xft)
		return prima_xft_get_glyph_bitmap(self, index, flags, offset, size, advance);
#endif
	return prima_corefont_get_glyph_bitmap(self, index, flags & ggoMonochrome, offset, size, advance);
}



Bool
apc_gp_set_text_matrix( Handle self, Matrix matrix)
{
#ifdef USE_XFT
	DEFXX;
	Bool old_matrix_used  = XX->flags.matrix_used;
	XX->flags.matrix_used = !prima_matrix_is_translated_only(matrix);
	if ( do_xft && (old_matrix_used || XX->flags.matrix_used )) {
		Font f = PDrawable(self)->font;
		return prima_xft_set_font( self, &f);
	}
#endif
	return true;
}

Bool
apc_font_begin_query( Handle self )
{
	if ( !DISP )
		return false;
	return true;
}

void
apc_font_end_query( Handle self )
{
}

