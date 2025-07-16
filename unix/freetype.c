#include "unix/guts.h"

#ifdef WITH_HARFBUZZ
#include <hb.h>
#include <hb-ft.h>
#endif

#ifdef USE_FONTQUERY

#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H

#define FTdebug(...) if (pguts->debug & DEBUG_FONTS) prima_debug2("ft", __VA_ARGS__)

static FT_Library  ft_library;
static PHash ft_files = NULL; /* fontconfig filenames */

typedef struct {
	int refcnt;
	FT_Face ft_face;
} FaceEntry, *PFaceEntry;

Bool
prima_ft_init( void)
{
	if (FT_Init_FreeType(&ft_library))
		return false;
	ft_files = hash_create();
	return true;
}

static Bool
unload_face( void * f, int keyLen, void * key, void * dummy)
{
	if ( keyLen == sizeof(FT_Face) )
		FT_Done_Face(*((FT_Face*) key));
	return false;
}

void
prima_ft_done( void)
{
	hash_first_that( ft_files, (void*) unload_face, NULL, NULL, NULL);
	hash_destroy( ft_files, true);
	FT_Done_FreeType(ft_library);
}

static void*
build_face_key( char *filename, int index, unsigned int *size)
{
	Byte *key, *p;
	int len = strlen(filename);
	*size = len + sizeof(int) + sizeof(unsigned int);
	if ( !( key = (Byte*) malloc(*size)))
		return NULL;
	p = key;
	memcpy(p, size, sizeof(unsigned int));
	p += sizeof(unsigned int);
	memcpy(p, &index, sizeof(index));
	p += sizeof(int);
	memcpy(p, filename, len);
	return key;
}

FT_Face
prima_ft_lock_face( char *filename, int index)
{
	void *key;
	PFaceEntry fe;
	unsigned int size;

	if ( !( key = build_face_key( filename, index, &size )))
		return NULL;

	fe = (PFaceEntry) hash_fetch( ft_files, key, size);

	if ( fe != NULL ) {
		free(key);
		fe->refcnt++;
		return fe->ft_face;
	}

	if ( !( fe = malloc(sizeof(FaceEntry)))) {
		free(key);
		return NULL;
	}
	if (FT_New_Face (ft_library, filename, index, &fe->ft_face)) {
		free(key);
		free(fe);
		return NULL;
	}
	fe->refcnt = 1;

	hash_store(ft_files, key, size, fe);
	hash_store(ft_files, &(fe->ft_face), sizeof(FT_Face), key);

	return fe->ft_face;
}

/* XXX retain some? */
void
prima_ft_unlock_face( FT_Face face)
{
	void *key;
	PFaceEntry fe;

	if ( !( key = hash_fetch( ft_files, &face, sizeof(face))))
		return;
	if ( !( fe = (PFaceEntry) hash_fetch( ft_files, key, *((unsigned int*)key))))
		return;
	if ( --fe->refcnt <= 0 ) {
		hash_delete( ft_files, key, *((unsigned int*)key), true);
		hash_delete( ft_files, &face, sizeof(face), true);
		FT_Done_Face( face );
	}
}

Bool
prima_ft_combining_supported( FT_Face face )
{
#ifdef WITH_HARFBUZZ
	hb_buffer_t *buf;
	hb_font_t *font;
	hb_glyph_position_t *glyph_pos;
	unsigned int l = 0;
	uint32_t x_tilde[2] = {'x', 0x330};
	Bool ok = 0;

	buf = hb_buffer_create();
	hb_buffer_add_utf32(buf, x_tilde, 2, 0, -1);
	hb_buffer_guess_segment_properties (buf);
	font = hb_ft_font_create(face, NULL);
	hb_shape(font, buf, NULL, 0);
	glyph_pos  = hb_buffer_get_glyph_positions(buf, &l);

	if ( l == 2 && glyph_pos[1].x_advance == 0 )
		ok = 1;

	hb_buffer_destroy(buf);
	hb_font_destroy(font);

	return ok;
#else
	return false;
#endif
}

typedef struct {
	int count, size, last_ptr;
	int *buffer;
} OutlineStorage;

#define STORE_POINT(p) if(p) {\
	storage->buffer[ storage->last_ptr + 1 ]++;\
	storage->buffer[ storage->count++ ] = p->x;\
	storage->buffer[ storage->count++ ] = p->y;\
}

static int
store_command( OutlineStorage * storage, int cmd, const FT_Vector * p1, const FT_Vector * p2, const FT_Vector * p3)
{
	if ( storage-> size == 0 ) {
		storage-> size = 256;
		if ( !( storage-> buffer = malloc(sizeof(int) * storage->size)))
			return 1;

	} else if ( storage-> count + 7 >= storage->size ) {
		int * r;
		storage-> size *= 2;
		if (( r = realloc( storage->buffer, sizeof(int) * storage->size)) == NULL ) {
			warn("Not enough memory");
			free( storage-> buffer );
			storage-> buffer = NULL;
			storage-> count = 0;
			return 1;
		}
		storage-> buffer = r;
	}

	if ( 
		storage-> last_ptr < 0 || storage->buffer[storage->last_ptr] != cmd || 
		cmd != ggoLine
	) {
		storage->last_ptr = storage->count;
		storage->buffer[ storage->count++ ] = cmd;
		storage->buffer[ storage->count++ ] = 0;
	}

	STORE_POINT(p1)
	STORE_POINT(p2)
	STORE_POINT(p3)

	return 0;
}

static int
ftoutline_line(const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoLine, to, NULL, NULL);
}

static int
ftoutline_move(const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoMove, to, NULL, NULL);
}

static int
ftoutline_conic(const FT_Vector* control, const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoConic, control, to, NULL);
}

static int
ftoutline_cubic(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
	return store_command((OutlineStorage*)user, ggoCubic, control1, control2, to);
}

int
prima_ft_get_glyph_outline( FT_Face face, FT_UInt ft_index, FT_Int32 ft_flags, Bool want_color, int ** buffer)
{
	FT_Outline_Funcs funcs = {
		ftoutline_move,
		ftoutline_line,
		ftoutline_conic,
		ftoutline_cubic,
		0, 0
	};
	OutlineStorage storage = { 0, 0, -1, NULL };

#ifdef FT_COLOR_H
	if (want_color && FT_HAS_COLOR(face)) {
		FT_Color *palette = NULL;
		FT_UInt glyph, color;
		FT_LayerIterator i = { 0, 0, NULL };

		FT_Palette_Select( face, 0, &palette );
		while ( FT_Get_Color_Glyph_Layer( face, ft_index, &glyph, &color, &i)) {
			if (
				(FT_Load_Glyph (face, glyph, ft_flags) == 0) &&
				(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
			) {
				FT_Vector layer_color; /* it's signed long, should hold 24-bit signed RGB fine */
				layer_color.x = ( color == 0xFFFF || palette == NULL ) ? -1 : (
					( palette[color].red   << 16)|
					( palette[color].green << 8)|
					( palette[color].blue  )
				);
				store_command(&storage, ggoSetColor, &layer_color, NULL, NULL);
				FT_Outline_Decompose( &face->glyph->outline, &funcs, (void*)&storage);
			}
		}
		*buffer = storage.buffer;
		return storage.count;
	}
#endif

	if (
		(FT_Load_Glyph (face, ft_index, ft_flags) == 0) &&
		(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	)
		FT_Outline_Decompose( &face->glyph->outline, &funcs, (void*)&storage);

	*buffer = storage.buffer;
	return storage.count;
}

Byte*
prima_ft_get_glyph_bitmap( FT_Face face, FT_UInt index, FT_Int32 flags, PPoint offset, PPoint size, int *advance, int *bpp)
{
	Byte *ret = NULL;
	FT_Bitmap *b;
	int src_stride;
	Byte *src;

	if ( FT_Load_Glyph (face, index, flags) != 0 )
		return NULL;

	if ( !( b = &face->glyph->bitmap))
		return NULL;

	src        = b->buffer;
	src_stride = abs(b->pitch);

#define LINE_SIZE(width,type) (((( width ) * (( type ) & imBPP) + 31) / 32) * 4)
	if ( b-> pixel_mode == FT_PIXEL_MODE_BGRA ) {
		int y, dst_stride = LINE_SIZE(b->width, 24 ), mask_stride = LINE_SIZE(b->width, 8);
		if ( ( ret = malloc( (dst_stride + mask_stride) * b->rows )) != NULL ) {
			Byte *dst = ret, *mask = dst + dst_stride * b->rows;
			if ( b->pitch > 0 ) {
				dst += dst_stride * (b->rows - 1);
				dst_stride = -dst_stride;
				mask += mask_stride * (b->rows - 1);
				mask_stride = -mask_stride;
			}

			for (
				y = 0;
				y < b->rows;
				y++, dst += dst_stride, mask += mask_stride
			) {
				register unsigned int n = b->width;
				register Byte *d = dst, *m = mask;
				while (n--) {
					*(d++) = *(src++);
					*(d++) = *(src++);
					*(d++) = *(src++);
					*(m++) = *(src++);
				}
			}
		}
		*bpp = 32;
	} else {
		int dst_stride  = LINE_SIZE(b->width, (flags & FT_LOAD_MONOCHROME) ? 1 : 8);
		int bytes       = (src_stride > dst_stride) ? dst_stride : src_stride;
		if (( ret = malloc( b->rows * dst_stride )) != NULL) {
			int i;
			Byte *dst = ret;
			if ( b->pitch > 0 ) {
				dst += dst_stride * (b->rows - 1);
				dst_stride = -dst_stride;
			}
			for ( i = 0; i < b->rows; i++, src += src_stride, dst += dst_stride)
				memcpy( dst, src, bytes);
		}
		*bpp = (flags & FT_LOAD_MONOCHROME) ? 1 : 8;
	}

	if ( ret ) {
		offset->x = face->glyph->bitmap_left;
		offset->y = face->glyph->bitmap_top - b->rows;
		size->x   = b->width;
		size->y   = b->rows;
		if ( advance ) {
			FT_Fixed a = face->glyph->linearHoriAdvance;
			*advance = (a >> 16) + (((a & 0xffff) > 0x7fff) ? 1 : 0);
		}
	}
#undef LINE_SIZE

	return ret;
}

Bool
prima_ft_text_shaper_harfbuzz( FT_Face face, PTextShapeRec r)
{
#ifdef WITH_HARFBUZZ
	Bool ret = true;
	int i, j;
	hb_buffer_t *buf;
	hb_font_t *font;
	hb_glyph_info_t *glyph_info;
	hb_glyph_position_t *glyph_pos;

	buf = hb_buffer_create();
	hb_buffer_add_utf32(buf, r->text, r->len, 0, -1);

#if HB_VERSION_MAJOR >= 1
#if HB_VERSION_ATLEAST(1,0,3)
	hb_buffer_set_cluster_level(buf, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
#endif
#endif
	hb_buffer_set_direction(buf, (r->flags & toRTL) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
/*
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_script (buf, hb_script_from_string ("Arab", -1));
*/
	if ( r-> language != NULL )
		hb_buffer_set_language(buf, hb_language_from_string(r->language, -1));
	hb_buffer_guess_segment_properties (buf);

	font = hb_ft_font_create(face, NULL);

	hb_shape(font, buf, NULL, 0);

	glyph_info = hb_buffer_get_glyph_infos(buf, &r->n_glyphs);
	glyph_pos  = hb_buffer_get_glyph_positions(buf, &r->n_glyphs);

	for (i = j = 0; i < r->n_glyphs; i++) {
		uint32_t c = glyph_info[i].cluster;
		if ( c > r-> len ) {
			/* something bad happened? */
			warn("harfbuzz shaping assertion failed: got cluster=%d for strlen=%d", c, r->len);
			guts. use_harfbuzz = false;
			ret = false;
			break;
		}
                r->indexes[i] = c;
                r->glyphs[i]   = glyph_info[i].codepoint;
		if ( glyph_pos ) {
			r->advances[i]    = floor(glyph_pos[i].x_advance / 64.0 + .5);
			r->positions[j++] = floor(glyph_pos[i].x_offset  / 64.0 + .5);
			r->positions[j++] = floor(glyph_pos[i].y_offset  / 64.0 + .5);
		}
	}

	hb_buffer_destroy(buf);
	hb_font_destroy(font);

	return ret;
#else
	return false;
#endif
}

void
prima_ft_detail_tt_font( FT_Face face, PFont font, float mul)
{
	/* get TTF and OTF metrics so these match win32 ones more or less */
	TT_OS2 *os2;
	TT_HoriHeader *hori;

	if ( ( hori = (TT_HoriHeader*) FT_Get_Sfnt_Table(face, ft_sfnt_hhea))) {
		font->externalLeading = hori->Line_Gap * mul + .5;
		FTdebug("set external leading: %d", font->externalLeading);
	} else {
		font->externalLeading = (face-> bbox.yMax - face->bbox.yMin - face-> height) * mul + .5;
	}
	if ( font->externalLeading < 0 )
		font->externalLeading = 0;

	if ( font->pitch == fpFixed)
		font->width = font->maximalWidth;
	else if ( ( os2  = (TT_OS2*) FT_Get_Sfnt_Table(face, ft_sfnt_os2 ))) {
		if ( os2-> xAvgCharWidth > 0 ) {
			font->width = os2->xAvgCharWidth * mul + .5;
			FTdebug("set width: %d", font->width);
		} else
			goto APPROXIMATE;
	} else {
		FcChar32 c;
		int num, sum;
	APPROXIMATE:
		for ( c = 63, num = sum = 0; c < 126; c += 4 ) {
			FT_UInt ix;
			if (( ix = FcFreeTypeCharIndex( face, c)) == 0)
				continue;
			if ( FT_Load_Glyph( face, ix, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_NO_BITMAP))
				continue;
			sum += FT266_to_short(face->glyph->metrics.width);
			num++;
		}
		if ( num > 10 ) {
			font->width = ((float) sum / num) + .5;
			FTdebug("approximated width: %d", font->width);
		} else
			font->width = font->maximalWidth;
	}
	if ( font->width <= 0 )
		font->width = 1;

	if ( face->underline_position != 0 && face->underline_position != 0) {
		font->underlinePosition  = -face->underline_position * mul + .5;
		font->underlineThickness = face->underline_thickness * mul + .5;
	} else {
		font->underlinePosition  = -font-> descent;
		font->underlineThickness = (font->height > 16) ? font->height / 16 : 1;
	}
}

Bool
prima_ft_font_has_color( FT_Face face)
{
#ifdef FT_HAS_COLOR
	return FT_HAS_COLOR(face);
#else
	return false;
#endif
}

#endif

