/*
		stock.c

		Stock objects - pens, brushes, fonts
*/

#include "win32\win32guts.h"
#ifndef _APRICOT_H_
#include "apricot.h"
#endif
#include "guts.h"
#include <ctype.h>
#include "Window.h"
#include "Image.h"
#include "Printer.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  sys (( PDrawableData)(( PComponent) self)-> sysData)->
#define  dsys( view) (( PDrawableData)(( PComponent) view)-> sysData)->
#define var (( PWidget) self)->
#define HANDLE sys handle
#define DHANDLE(x) dsys(x) handle

// Stylus section

static int
stylus_get_key_size( int type )
{
	switch (type) {
	case DCO_PEN     : return sizeof(RQPen);     break;
	case DCO_BRUSH   : return sizeof(RQBrush);   break;
	case DCO_GP_PEN  : return sizeof(RQGPPen);   break;
	case DCO_GP_BRUSH: return sizeof(RQGPBrush); break;
	default: return 0;
	}
}

/*
void
dump_key(void * key, int size)
{
	Byte *k = (Byte*) key;
	int type = *((int*) key);
	int sz = stylus_get_key_size(type);
	int i;
	char buf[20000], *ptr = buf, xx[256];
	buf[0] = 0;
	if (size > 0) sz = size;
	for ( i = 0; i < sz; i++) {
		snprintf( xx, 256, "%02x ", k[i]);
		strcat(buf, xx);
	}
	warn("[%x] %s\n", type, buf);
}
*/

static PDCObject
stylus_alloc(int type)
{
	int size;
	PDCObject ret;

	size = stylus_get_key_size(type);
	if ( !( ret = (PDCObject) malloc(sizeof(DCObject) + size - 1))) {
		warn("Not enough memory");
		return false;
	}

	bzero(ret, sizeof(DCObject) + size - 1);
	ret-> type    = type;
	ret-> rq_size = size;
	ret-> rq      = &ret-> rq_buf;

	return ret;
}

PDCObject
stylus_fetch( void * key )
{
	PDCObject cached;
	int type = *((int*) key);
	int size = stylus_get_key_size(type);
	if (( cached = (PDCObject) hash_fetch( mgr_styli, key, size)) != NULL )
		return cached;
	if (( cached = stylus_alloc(type)) == NULL)
		return NULL;
	cached-> cached = true;
	if ( hash_count( mgr_styli) > 128)
		stylus_clean();
	memcpy( cached-> rq, key, size );
	hash_store( mgr_styli, key, size, cached);
	return cached;
}

HPEN
stylus_get_pen( DWORD style, DWORD line_width, COLORREF color )
{
	RQPen ss;
	PDCObject dcobj;
	bzero(&ss, sizeof(ss));
	ss.type               = DCO_PEN;
	ss.logpen.lopnStyle   = style;
	ss.logpen.lopnWidth.x = line_width;
	ss.logpen.lopnColor   = color;
	if (( dcobj = stylus_fetch(&ss)) == NULL)
		return 0;
	if ( !dcobj-> handle ) {
		if (( dcobj-> handle = CreatePen( style, line_width, color )) == 0)
			apiErr;
	}
	return (HPEN) dcobj->handle;
}

HBRUSH
stylus_get_solid_brush( COLORREF color )
{
	RQBrush ss;
	PDCObject dcobj;
	bzero(&ss, sizeof(ss));
	ss.type             = DCO_BRUSH;
	ss.logbrush.lbStyle = BS_SOLID;
	ss.logbrush.lbColor = color;
	ss.logbrush.lbHatch = (LONG_PTR) 0;
	if (( dcobj = stylus_fetch(&ss)) == NULL)
		return 0;
	if ( !dcobj-> handle ) {
		if (( dcobj-> handle = CreateSolidBrush( color )) == 0)
			apiErr;
	}

	return (HBRUSH) dcobj->handle;
}

static void
stylus_free( PDCObject res, Bool permanent)
{
	if ( !res || --res-> refcnt > 0)
		return;

	if ( res-> cached ) {
		void *p = NULL;
		if ( !permanent) {
			res-> refcnt = 0;
			return;
		}
		p = hash_delete( mgr_styli, res->rq, res-> rq_size, false);
		if ( p && p != res ) {
			warn("panic: bad stylus hash %p ne %p\n", p, res);
			return;
		}

	}

	switch ( res-> type ) {
	case DCO_PEN:
	case DCO_BRUSH:
		DeleteObject( res-> handle );
		break;
	case DCO_GP_PEN:
		GdipDeletePen((GpPen*) res-> handle);
		break;
	case DCO_GP_BRUSH:
		GdipDeleteBrush((GpBrush*) res-> handle);
		break;
	}

	free(res);
}

static void
stylus_release_current( Handle self, int index, Bool force_deselect )
{
	if ( sys current_dc_obj[index] ) {
		if ( !sys current_dc_obj[index]->cached || force_deselect ) {
			switch ( index ) {
			case DCO_PEN:
				SelectObject( sys ps, sys stock_pen );
				break;
			case DCO_BRUSH:
				SelectObject( sys ps, sys stock_brush );
				break;
			}
		}
		stylus_free( sys current_dc_obj[index], 0);
		sys current_dc_obj[index] = NULL;
	}
}

static void
stylus_replace_current( Handle self, int index, PDCObject new_stylus )
{
	stylus_release_current(self, index, 0);
	sys current_dc_obj[index] = new_stylus;
	new_stylus-> refcnt++;
	if ( index == DCO_PEN || index == DCO_BRUSH )
		SelectObject( sys ps, new_stylus-> handle );
}

void
stylus_release( Handle self )
{
	int i;
	for ( i = 0; i < DCO_COUNT; i++)
		stylus_release_current(self, i, 1);
}

static Bool
stylus_cleaner( PDCObject s, int keyLen, void * key, void * dummy)
{
	if ( s-> refcnt <= 0) stylus_free( s, true);
	return false;
}

void
stylus_clean()
{
	hash_first_that( mgr_styli, stylus_cleaner, NULL, NULL, NULL);
}

Bool
select_pen( Handle self )
{
	RQPen key = sys rq_pen;
	PDCObject ret;

	if ( ! sys ps ) return false;

	key.type = DCO_PEN;
	if ( !key.geometric ) {
		key.style = 0;
		key.line_pattern = NULL;
	}
	if (( ret = stylus_fetch(&key)) == NULL )
		return false;
	if ( ret-> handle )
		goto SUCCESS;

	if ( key.geometric ) {
		LOGBRUSH pb;
		int i, delta;
		PLinePattern pat;

		pb.lbStyle = BS_SOLID;
		pb.lbColor = key.logpen.lopnColor;
		pb.lbHatch = 0;

		delta = (key.logpen.lopnWidth.x > 1) ? key.logpen.lopnWidth.x - 1 : 0;
		pat = key.line_pattern;
		for ( i = 1; i < pat->count; i += 2)
			pat->dots[i] += delta;
		ret-> handle = ExtCreatePen(
			key.style,
			key.logpen.lopnWidth.x, &pb,
			pat->count, pat->ptr
		);
		for ( i = 1; i < pat->count; i += 2)
			pat->dots[i] -= delta;
	} else
		ret-> handle = CreatePenIndirect( &key.logpen);

	if ( !ret-> handle ) {
		apiErr;
		ret-> handle = CreatePen( PS_SOLID, 0, 0);
	}

SUCCESS:
	stylus_replace_current( self, DCO_PEN, ret );
	return true;
}

Bool
select_brush( Handle self)
{
	COLORREF fg;
	RQBrush key;
	PDCObject ret;
	LOGBRUSH b;
	Bool mono_workaround = false;

	/* image brush is not cacheable */
	fg = sys rq_brush.color;
	if ( var fillPatternImage ) {

		if ( PObject(var fillPatternImage)->stage != csNormal )
			return false;

		b.lbHatch = (LONG_PTR) 0;

		if ( PImage(var fillPatternImage)->type == imBW && !dsys(var fillPatternImage)options.aptIcon ) {
			if ( sys fg == sys bg )
				mono_workaround = true;
			else
				b.lbHatch = (LONG_PTR) image_create_mono_pattern_dib(var fillPatternImage,fg,sys rq_brush.back_color);
		} else
			b.lbHatch = (LONG_PTR) image_create_color_pattern_dib(var fillPatternImage);

		if ( b.lbHatch ) {
			HBRUSH h;
			b.lbStyle = BS_DIBPATTERNPT;
			b.lbColor = DIB_RGB_COLORS;
			h = CreateBrushIndirect(&b);
			free((void *) b.lbHatch);
			if ( h != 0 ) {
				if (( ret = stylus_alloc(DCO_BRUSH)) == NULL ) {
					DeleteObject(h);
					return false;
				}
				ret->handle = h;
				goto SUCCESS;
			} else
				apiErr;
		}
	}

	/* fix the key */
	key = sys rq_brush;
	if ( key.logbrush.lbStyle == BS_DIBPATTERNPT)
		key.logbrush.lbHatch = (LONG_PTR)0;
	if ( key.logbrush.lbStyle == BS_DIBPATTERNPT && key.back_color == fg)
		mono_workaround = true;
	if ( mono_workaround ) {
		/* workaround Win32 bug with mono bitmaps -
		if color and backColor are the same, but fill pattern present, backColor
		value is ignored by some unknown, but certainly important reason :) */
		key.logbrush.lbStyle = BS_SOLID;
		key.logbrush.lbColor = fg;
		key.logbrush.lbHatch = 0;
	}

	/* have the brush? */
	key.type = DCO_BRUSH;
	if (( ret = stylus_fetch( &key )) == NULL)
		return false;
	if ( ret-> handle ) {
		goto SUCCESS;
	}

	/* create new one */
	b = key.logbrush;
	if ( key.logbrush.lbStyle == BS_DIBPATTERNPT) {
		int i;
		static DIBMONOBRUSH bmi_hatch = {
			{ sizeof( BITMAPINFOHEADER), 8, 8, 1, 1, BI_RGB, 0, 0, 0, 2, 2},
			{{0,0,0,0}, {0,0,0,0}}
		};
		for ( i = 0; i < 8; i++)
			bmi_hatch.data[i * 4] = key.fill_pattern[i];
		bmi_hatch.colors[0].rgbRed   =  ( key.back_color & 0xFF);
		bmi_hatch.colors[0].rgbGreen = (( key.back_color >> 8) & 0xFF);
		bmi_hatch.colors[0].rgbBlue  = (( key.back_color >> 16) & 0xFF);
		bmi_hatch.colors[1].rgbRed   =  ( fg & 0xFF);
		bmi_hatch.colors[1].rgbGreen = (( fg >> 8) & 0xFF);
		bmi_hatch.colors[1].rgbBlue  = (( fg >> 16) & 0xFF);
		b.lbHatch = (LONG_PTR) &bmi_hatch;
	}

	if ( !( ret-> handle = CreateBrushIndirect(&b))) {
		apiErr;
		ret-> handle = CreateSolidBrush(fg);
	}

SUCCESS:
	stylus_replace_current( self, DCO_BRUSH, ret);
	return true;
}

Bool
stylus_is_geometric( Handle self )
{
	LOGPEN *s = & sys rq_pen.logpen;
	return s-> lopnStyle == PS_USERSTYLE;
}

Bool
stylus_is_complex( Handle self )
{
	int rop;

	if ( sys rq_brush.logbrush.lbStyle == BS_DIBPATTERNPT)
		return true;

	rop = GetROP2( sys ps);
	if (
		( rop != R2_COPYPEN) &&
		( rop != R2_WHITE  ) &&
		( rop != R2_NOP    ) &&
		( rop != R2_BLACK  )
	)
		return true;

	return false;
}

static PDCObject
alloc_gp_image_brush( Handle self)
{
	GpTexture *t;
	PDCObject ret;

	if ( PObject(var fillPatternImage)->stage != csNormal )
		return NULL;

	if (( t = image_create_gp_pattern( self, var fillPatternImage, sys alpha )) == NULL)
		return NULL;

	if (( ret = stylus_alloc(DCO_GP_BRUSH)) == NULL) {
		GdipDeleteBrush((GpBrush*) t);
		return NULL;
	}
	ret->handle = t;

	return ret;
}

Bool
select_gp_brush(Handle self)
{
	int r,g,b;
	COLORREF fg, bg;
	RQGPBrush key;
	PDCObject ret;
	Bool is_solid;
	POINT offset;
	int alpha;

	is_solid = sys rq_brush.logbrush.lbStyle == BS_SOLID;

	if ( !is_solid && var fillPatternImage ) {
		if ( PObject(var fillPatternImage)->stage != csNormal )
			return false;

		if (( ret = alloc_gp_image_brush(self)) != NULL )
			goto SUCCESS;
		else
			is_solid = true;
	}

	memset(&key, 0, sizeof(key));
	key.type = DCO_GP_BRUSH;

	alpha = sys alpha;
	switch ( sys rop ) {
	case ropNotPut:
		fg = ~sys rq_brush.color;
		bg = ~sys rq_brush.back_color;
		break;
	case ropBlackness:
		fg = bg = 0;
		break;
	case ropWhiteness:
		fg = bg = 0xffffff;
		break;
	case ropNoOper:
		fg = bg = alpha = 0;
		break;
	default:
		fg = sys rq_brush.color;
		bg = sys rq_brush.back_color;
	}
	b = (fg >> 16) & 0xff;
	g = (fg & 0xff00) >> 8;
	r = fg & 0xff;
	key.fg = (alpha << 24) | (r << 16) | (g << 8) | b;
	key.opaque = sys rop2 == ropCopyPut;
	if ( !is_solid ) {
		key.opaque = (sys rop2 == ropCopyPut) ? 1 : 0;
		b = (bg >> 16) & 0xff;
		g = (bg & 0xff00) >> 8;
		r =  bg & 0xff;
		key.bg = (sys alpha << 24) | (r << 16) | (g << 8) | b;
		*key.fill_pattern = *sys fill_pattern;
	}

	if ((ret = stylus_fetch(&key)) == NULL)
		return false;

	if ( ret->handle )
		goto SUCCESS;
	if ( is_solid ) {
		GpSolidFill *f;
		GPCALL GdipCreateSolidFill((ARGB)key.fg, &f);
		if ( rc ) goto FAIL;

		ret->handle = f;
	} else {
		GpBitmap * b;
		GpTexture * t;
		uint32_t x, y, fp[64], *fpp, bg;

		bg = key.opaque ? key.bg : 0x00000000;
		for ( y = 0, fpp = fp; y < 8; y++) {
			int yy = (y + 8) % 8;
			Byte src = sys fill_pattern[yy];
			for ( x = 0; x < 8; x++) {
				int xx = x % 8;
				*(fpp++) = (src & ( 1 << xx )) ? key.fg : bg;
			}
		}

		GPCALL GdipCreateBitmapFromScan0( 8, 8, 32, PixelFormat32bppARGB, (BYTE*)fp, &b);
		apiGPErrCheck;
		if ( rc ) goto FAIL;

		GPCALL GdipCreateTexture((GpImage*) b, WrapModeTile, &t);
		apiGPErrCheck;
		GdipDisposeImage((GpImage*) b);
		if ( rc ) goto FAIL;

		ret->handle = t;
	}

SUCCESS:
	GetBrushOrgEx( sys ps, &offset);
	GdipResetTextureTransform((GpTexture*) ret->handle);
	GdipTranslateTextureTransform((GpTexture*) ret->handle,offset.x,offset.y,MatrixOrderPrepend);
	stylus_replace_current( self, DCO_GP_BRUSH, ret );
	return true;

FAIL:
	hash_delete( mgr_styli, &key, sizeof(key), true);
	return false;
}

/* not using GpPens for Prima representation, but for internal painting only */
GpPen*
stylus_gp_get_pen(int line_width, uint32_t color)
{
	RQGPPen key;
	PDCObject cached;

	memset( &key, 0, sizeof(key));
	key.type       = DCO_GP_PEN;
	key.fg         = color;
	key.line_width = line_width;
	if (( cached = stylus_fetch(&key)) == NULL)
		return NULL;

	if ( !cached->handle ) {
		GPCALL GdipCreatePen1(color, line_width, UnitPixel, (GpPen**) &cached->handle);
		apiGPErrCheck;
		if ( rc ) goto FAIL;
	}

	return (GpPen*) cached->handle;

FAIL:
	hash_delete( mgr_styli, &key, sizeof(key), true);
	return NULL;
}

PLinePattern
patres_fetch( unsigned char * pattern, int len)
{
	int i;
	PLinePattern r = ( PLinePattern) hash_fetch( mgr_patterns, pattern, len);
	if ( r)
		return r;

	r = ( PLinePattern) malloc( sizeof( LinePattern) + sizeof( DWORD) * len);
	if ( !r) return &std_hollow_line_pattern;

	r-> count = len;
	r-> ptr   = r-> dots;
	for ( i = 0; i < len; i++) {
		DWORD x = ( DWORD) pattern[ i];
		if ( i & 1)
			x++;
		else
			if ( x > 0) x--;
		r-> dots[i] = x;
	}
	hash_store( mgr_patterns, pattern, len, r);
	return r;
}

UINT
patres_user( unsigned char * pattern, int len)
{
	switch ( len) {
	case 0:
		return PS_NULL;
	case 1:
		return pattern[0] ? PS_SOLID : PS_NULL;
	default:
		return PS_USERSTYLE;
	}
}

// Stylus end

// Font section


#define FONTHASH_SIZE 563
#define FONTIDHASH_SIZE 23

typedef struct _FontHashNode
{
	struct _FontHashNode *next;
	struct _FontHashNode *next2;
	Font key;
	Font value;
} FontHashNode, *PFontHashNode;

typedef struct _FontHash
{
	PFontHashNode buckets[ FONTHASH_SIZE];
} FontHash, *PFontHash;

static FontHash fontHash;
static FontHash fontHashBySize;

static unsigned long
elf_hash( const char *key, int size, unsigned long h)
{
	unsigned long g;
	if ( size >= 0) {
		while ( size) {
			h = ( h << 4) + *key++;
			if (( g = h & 0xF0000000))
				h ^= g >> 24;
			h &= ~g;
			size--;
		}
	} else {
		while ( *key) {
			h = ( h << 4) + *key++;
			if (( g = h & 0xF0000000))
				h ^= g >> 24;
			h &= ~g;
		}
	}
	return h;
}

static unsigned long
elf( Font * font, Bool bySize)
{
	unsigned long seed = 0;
	if ( bySize) {
		seed = elf_hash( (const char*)&font-> width, (char *)(&(font-> name)) - (char *)&(font-> width), seed);
		seed = elf_hash( font-> name, -1, seed);
		seed = elf_hash( font-> encoding, -1, seed);
		seed = elf_hash( (const char*)&font-> size, sizeof( font-> size), seed);
	} else {
		seed = elf_hash( (const char*)&font-> height, (char *)(&(font-> name)) - (char *)&(font-> height), seed);
		seed = elf_hash( font-> name, -1, seed);
		seed = elf_hash( font-> encoding, -1, seed);
	}
	return seed % FONTHASH_SIZE;
}


static PFontHashNode
find_node( const PFont font, Bool bySize)
{
	unsigned long i;
	PFontHashNode node;
	int sz;

	if ( font == NULL) return NULL;

	i = elf( font, bySize);
	if (bySize)
		node = fontHashBySize. buckets[ i];
	else
		node = fontHash. buckets[ i];
	if ( bySize) {
		sz = (char *)(&(font-> name)) - (char *)&(font-> width);
		while ( node != NULL)
		{
			if (( memcmp( &(font-> width), &(node-> key. width), sz) == 0) &&
				( strcmp( font-> name, node-> key. name) == 0 ) &&
				( strcmp( font-> encoding, node-> key. encoding) == 0 ) &&
				(font-> resolution == node-> key. resolution) &&
				(font-> size == node-> key. size))
				return node;
			node = node-> next2;
		}
	} else {
		sz = (char *)(&(font-> name)) - (char *)&(font-> height);
		while ( node != NULL)
		{
			if (( memcmp( font, &(node-> key), sz) == 0) &&
				( strcmp( font-> name, node-> key. name) == 0 )&&
				(font-> resolution == node-> key. resolution) &&
				( strcmp( font-> encoding, node-> key. encoding) == 0 ))
				return node;
			node = node-> next;
		}
	}
	return NULL;
}

Bool
add_font_to_hash( const PFont key, const PFont font, Bool addSizeEntry)
{
	PFontHashNode node;
	unsigned long i;

	node = ( PFontHashNode) malloc( sizeof( FontHashNode));
	if ( node == NULL) return false;
	memcpy( &(node-> key), key, sizeof( Font));
	memcpy( &(node-> value), font, sizeof( Font));
	i = elf(key, 0);
	node-> next = fontHash. buckets[ i];
	fontHash. buckets[ i] = node;
	if ( addSizeEntry) {
		i = elf( key, 1);
		node-> next2 = fontHashBySize. buckets[ i];
		fontHashBySize. buckets[ i] = node;
	}
	return true;
}

Bool
get_font_from_hash( PFont font, Bool bySize)
{
	PFontHashNode node = find_node( font, bySize);
	if ( node == NULL) return false;
	*font = node-> value;
	return true;
}

Bool
create_font_hash( void)
{
	memset ( &fontHash, 0, sizeof( FontHash));
	memset ( &fontHashBySize, 0, sizeof( FontHash));
	return true;
}

Bool
destroy_font_hash( void)
{
	PFontHashNode node, node2;
	int i;
	for ( i = 0; i < FONTHASH_SIZE; i++)
	{
		node = fontHash. buckets[ i];
		while ( node)
		{
			node2 = node;
			node = node-> next;
			free( node2);
		}
	}
	create_font_hash();     // just in case...
	return true;
}


static Bool _ft_cleaner( PDCFont f, int keyLen, void * key, void * dummy) {
	if ( f-> refcnt <= 0) font_free( f, true);
	return false;
}

static int
build_dcfont_key( Font * src, unsigned char * key)
{
	int sz = FONTSTRUCSIZE;
	char * p;
	memcpy( key, src, FONTSTRUCSIZE);
	key += FONTSTRUCSIZE;
	p = src-> name;
	while ( *p) {
		*(key++) = *(p++);
		sz++;
	}
	*(key++) = 0;
	sz++;
	p = src-> encoding;
	while ( *p) {
		*(key++) = *(p++);
		sz++;
	}
	*(key++) = 0;
	sz++;
	return sz;
}

PDCFont
font_alloc( Font * data)
{
	char key[sizeof(Font)];
	int keyLen = build_dcfont_key( data, (unsigned char*)key);
	PDCFont ret = ( PDCFont) hash_fetch( mgr_fonts, key, keyLen);

	if ( ret == NULL) {
		LOGFONTW logfont;
		PFont   f;

		if ( hash_count( mgr_fonts) > 128)
			font_clean();

		ret = ( PDCFont) malloc( sizeof( DCFont));
		if ( !ret) return NULL;

		memcpy( f = &ret-> font, data, sizeof( Font));
		ret-> refcnt = 0;
		font_font2logfont( f, &logfont);
		if ( !( ret-> hfont  = CreateFontIndirectW( &logfont))) {
			LOGFONTW lf;
			apiErr;
			memset( &lf, 0, sizeof( lf));
			ret-> hfont = CreateFontIndirectW( &lf);
		}
		keyLen = build_dcfont_key( &ret-> font, (unsigned char*)key);
		hash_store( mgr_fonts, key, keyLen, ret);
	}
	ret-> refcnt++;
	return ret;
}

void
font_free( PDCFont res, Bool permanent)
{
	int keyLen;
	char key[sizeof(Font)];

	if ( !res || --res-> refcnt > 0) return;
	if ( !permanent) {
		res-> refcnt = 0;
		return;
	}
	if ( res-> hfont) {
		DeleteObject( res-> hfont);
		res-> hfont = NULL;
	}
	keyLen = build_dcfont_key( &res-> font, (unsigned char*)key);
	hash_delete( mgr_fonts, key, keyLen, true);
}

void
font_change( Handle self, Font * font)
{
	PDCFont p;
	PDCFont newP;
	if ( is_apt( aptDCChangeLock)) return;

	sys alpha_arena_font_changed = true;
	p    = sys dc_font;
	newP = ( sys dc_font = font_alloc( font));
	if ( sys alpha_arena_stock_font ) {
		SelectObject( sys alpha_arena_dc, sys alpha_arena_stock_font );
		sys alpha_arena_font_changed = true;
	}
	font_free( p, false);
	if ( sys ps)
		SelectObject( sys ps, newP-> hfont);
}

void
font_clean()
{
	hash_first_that( mgr_fonts, _ft_cleaner, NULL, NULL, NULL);
}

static char* encodings[] = {
	"Western",
	"Windows",
	"Symbol",
	"Shift-JIS",
	"Hangeul",
	"GB 2312",
	"Chinese Big 5",
	"OEM DOS",
	"Johab",
	"Hebrew",
	"Arabic",
	"Greek",
	"Turkish",
	"Vietnamese",
	"Thai",
	"Eastern Europe",
	"Cyrillic",
	"Mac",
	"Baltic",
	NULL
};

static Handle ctx_CHARSET2index[] = {
	ANSI_CHARSET        , 0,
	DEFAULT_CHARSET     , 1,
	SYMBOL_CHARSET      , 2,
	SHIFTJIS_CHARSET    , 3,
	HANGEUL_CHARSET     , 4,
	GB2312_CHARSET      , 5,
	CHINESEBIG5_CHARSET , 6,
	OEM_CHARSET         , 7,
#ifdef JOHAB_CHARSET
	JOHAB_CHARSET       , 8,
	HEBREW_CHARSET      , 9,
	ARABIC_CHARSET      , 10,
	GREEK_CHARSET       , 11,
	TURKISH_CHARSET     , 12,
	VIETNAMESE_CHARSET  , 13,
	THAI_CHARSET        , 14,
	EASTEUROPE_CHARSET  , 15,
	RUSSIAN_CHARSET     , 16,
	MAC_CHARSET         , 17,
	BALTIC_CHARSET      , 18,
#endif
	endCtx
};

#define MASK_CODEPAGE  0x00FF
#define MASK_FAMILY    0xFF00
#define FF_MASK        0x00F0

static char *
font_charset2encoding( int charset)
{
#define SYNCPLEN 7
	static char buf[ SYNCPLEN * 256], initialized = false;
	char *str;
	int index = ctx_remap_end( charset, ctx_CHARSET2index, true);
	if ( index == endCtx) {
		charset &= 255;
		if ( !initialized) {
			int i;
			for ( i = 0; i < 256; i++) sprintf( buf + i * SYNCPLEN, "CP-%d", i);
			initialized = true;
		}
		str = buf + charset * SYNCPLEN;
	} else
		str = encodings[ index];
	return str;
}

static int
font_encoding2charset( const char * encoding)
{
	int index = 0;
	char ** e = encodings;

	if ( !encoding || encoding[0] == 0) return DEFAULT_CHARSET;

	while ( *e) {
		if ( strcmp( *e, encoding) == 0)
			return ctx_remap_def( index, ctx_CHARSET2index, false, DEFAULT_CHARSET);
		index++;
		e++;
	}
	if ( strncmp( encoding, "CP-", 3) == 0) {
		int i = atoi( encoding + 3);
		if ( i <= 0 || i > 256) i = DEFAULT_CHARSET;
		return i;
	}
	return DEFAULT_CHARSET;
}

static Bool
utf8_flag_strncpy( char * dst, const WCHAR * src, unsigned int maxlen)
{
	int i;
	Bool is_utf8 = false;
	for ( i = 0; i < maxlen && src[i] > 0; i++) {
		if ( src[i] > 0x7f ) {
			is_utf8 = true;
			break;
		}
	}
	WideCharToMultiByte(CP_UTF8, 0, src, -1, (LPSTR)dst, 256, NULL, false);
	return is_utf8;
}

int CALLBACK
fep_register_mapper_fonts( ENUMLOGFONTEXW FAR *e, NEWTEXTMETRICEXW FAR *t, DWORD type, LPARAM _es)
{
	char name[LF_FACESIZE + 1];
	Bool name_is_utf8;
	unsigned int i, styles[4], pitch;
	if (type & RASTER_FONTTYPE) return 1;

	if (e-> elfLogFont.lfFaceName[0] == '@') return 1; /* vertical font */

	name_is_utf8 = utf8_flag_strncpy( name, e-> elfLogFont.lfFaceName, LF_FACESIZE);
	pitch =
		((( e-> elfLogFont.lfPitchAndFamily & 3) == DEFAULT_PITCH ) ? fpDefault :
		((( e-> elfLogFont.lfPitchAndFamily & 3) == VARIABLE_PITCH) ? fpVariable : fpFixed));

	styles[0] = fsNormal;
	styles[1] = fsBold;
	styles[2] = fsItalic;
	styles[3] = fsBold | fsItalic;
	for ( i = 0; i < 4; i++) {
		PFont f;

		if ((f = prima_font_mapper_save_font(name, styles[i])) == NULL)
			continue;

		f->undef.pitch    = 0;
		f->pitch          = pitch;
		f->undef.vector   = 0;
		f->vector         = fvOutline;
		f->is_utf8.name   = name_is_utf8;
		f->is_utf8.family = utf8_flag_strncpy( f-> family, e-> elfFullName, LF_FULLFACESIZE);
	}
	return 1;
}

void
register_mapper_fonts(void)
{
	HDC dc;
	LOGFONTW elf;

	/* MS Shell Dlg is a virtual font, not reported by enum */
	prima_font_mapper_save_font(guts.window_font.name, 0);

	if ( !( dc = dc_alloc()))
		return;
	memset( &elf, 0, sizeof( elf));
	elf. lfCharSet = DEFAULT_CHARSET;
	EnumFontFamiliesExW( dc, &elf, (FONTENUMPROCW) fep_register_mapper_fonts, 0, 0);
	dc_free();
}

void
reset_system_fonts(void)
{
	memset( &guts. window_font, 0, sizeof( Font));
	strcpy( guts. window_font. name, DEFAULT_WIDGET_FONT);
	guts. window_font. size  = DEFAULT_WIDGET_FONT_SIZE;
	guts. window_font. undef. width = guts. window_font. undef. height = guts. window_font. undef. vector = 1;
	apc_font_pick( NULL_HANDLE, &guts. window_font, &guts. window_font);

	guts. ncmData. cbSize = sizeof( NONCLIENTMETRICSW);
	if ( !SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICSW),
		( PVOID) &guts. ncmData, 0)) apiErr;
	font_logfont2font( &guts.ncmData.lfMenuFont,    &guts.menu_font, &guts.display_resolution);
	font_logfont2font( &guts.ncmData.lfMessageFont, &guts.msg_font,  &guts.display_resolution);
	font_logfont2font( &guts.ncmData.lfCaptionFont, &guts.cap_font,  &guts.display_resolution);
}

void
font_logfont2font( LOGFONTW * lf, Font * f, Point * res)
{
	TEXTMETRICW tm;
	HDC dc = dc_alloc();
	HFONT hf;

	if ( !dc) return;
	hf = SelectObject( dc, CreateFontIndirectW( lf));

	GetTextMetricsW( dc, &tm);
	DeleteObject( SelectObject( dc, hf));
	dc_free();

	if ( !res) res = &guts. display_resolution;
	bzero( f, sizeof(Font));
	f-> height              = tm. tmHeight;
	f-> size                = ( f-> height - tm. tmInternalLeading) * 72.0 / res-> y + 0.5;
	f-> width               = lf-> lfWidth;
	f-> direction           = (double)(lf-> lfEscapement) / 10.0;
	f-> vector              = (( tm. tmPitchAndFamily & ( TMPF_VECTOR | TMPF_TRUETYPE)) ? fvOutline : fvBitmap);
	f-> style               = 0 |
		( lf-> lfItalic     ? fsItalic     : 0) |
		( lf-> lfUnderline  ? fsUnderlined : 0) |
		( lf-> lfStrikeOut  ? fsStruckOut  : 0) |
		(( lf-> lfWeight >= 700) ? fsBold   : 0);
	f-> pitch               = ((( lf-> lfPitchAndFamily & 3) == DEFAULT_PITCH) ? fpDefault :
		((( lf-> lfPitchAndFamily & 3) == VARIABLE_PITCH) ? fpVariable : fpFixed));
	strcpy( f-> encoding, font_charset2encoding( lf-> lfCharSet));
	f-> is_utf8.name = utf8_flag_strncpy( f->name, lf->lfFaceName, LF_FACESIZE);
}

void
font_font2logfont( Font * f, LOGFONTW * lf)
{
	lf-> lfHeight           = f-> height;
	lf-> lfWidth            = f-> width;
	lf-> lfEscapement       = f-> direction * 10;
	lf-> lfOrientation      = f-> direction * 10;
	lf-> lfWeight           = ( f-> style & fsBold)       ? 800 : 400;
	lf-> lfItalic           = ( f-> style & fsItalic)     ? 1 : 0;
	lf-> lfUnderline        = 0;
	lf-> lfStrikeOut        = 0;
	lf-> lfOutPrecision     = f-> vector ? OUT_TT_PRECIS : OUT_RASTER_PRECIS;
	lf-> lfClipPrecision    = CLIP_DEFAULT_PRECIS;
	lf-> lfQuality          = PROOF_QUALITY;
	lf-> lfPitchAndFamily   = FF_DONTCARE;
	lf-> lfCharSet          = font_encoding2charset( f-> encoding);
	MultiByteToWideChar(CP_UTF8, 0, f->name, -1, lf->lfFaceName, LF_FACESIZE);
}

void
font_textmetric2font( TEXTMETRICW * tm, Font * fm, Bool readonly)
{
	if ( !readonly) {
		fm-> size            = ( tm-> tmHeight - tm-> tmInternalLeading) * 72.0 / guts. display_resolution.y + 0.5;
		fm-> width           = tm-> tmAveCharWidth;
		fm-> height          = tm-> tmHeight;
		fm-> style           = 0 |
			( tm-> tmItalic     ? fsItalic     : 0) |
			( tm-> tmUnderlined ? fsUnderlined : 0) |
			( tm-> tmStruckOut  ? fsStruckOut  : 0) |
			(( tm-> tmWeight >= 700) ? fsBold   : 0)
			;
	}
	fm-> pitch  = (( tm-> tmPitchAndFamily & TMPF_FIXED_PITCH) ? fpVariable : fpFixed);
	fm-> vector = (( tm-> tmPitchAndFamily & ( TMPF_VECTOR | TMPF_TRUETYPE)) ? true : false);
	fm-> weight                 = tm-> tmWeight / 100;
	fm-> ascent                 = tm-> tmAscent;
	fm-> descent                = tm-> tmDescent;
	fm-> maximalWidth           = tm-> tmMaxCharWidth;
	fm-> internalLeading        = tm-> tmInternalLeading;
	fm-> externalLeading        = tm-> tmExternalLeading;
	fm-> xDeviceRes             = tm-> tmDigitizedAspectX;
	fm-> yDeviceRes             = tm-> tmDigitizedAspectY;
	fm-> firstChar              = tm-> tmFirstChar;
	fm-> lastChar               = tm-> tmLastChar;
	fm-> breakChar              = tm-> tmBreakChar;
	fm-> defaultChar            = tm-> tmDefaultChar;
	strcpy( fm-> encoding, font_charset2encoding( tm-> tmCharSet));
}

void
font_pp2font( char * presParam, Font * f)
{
	int i;
	char * p = strchr( presParam, '.');

	memset( f, 0, sizeof( Font));
	if ( p) {
		f-> size = atoi( presParam);
		p++;
	} else
		f-> size = 10;

	strlcpy( f-> name, p, 255);
	p = f-> name;
	f-> style = 0;
	f-> pitch = fpDefault;
	for ( i = strlen( p) - 1; i >= 0; i--)  {
		if ( p[ i] == '.')  {
			if ( stricmp( "Italic",    &p[ i + 1]) == 0) f-> style |= fsItalic;
			if ( stricmp( "Bold",      &p[ i + 1]) == 0) f-> style |= fsBold;
			if ( stricmp( "Underscore",&p[ i + 1]) == 0) f-> style |= fsUnderlined;
			if ( stricmp( "StrikeOut", &p[ i + 1]) == 0) f-> style |= fsStruckOut;
			p[ i] = 0;
		}
	}
	f-> undef. width = f-> undef. height = f-> undef. vector = 1;
	f-> direction = 0;
}

PFont
apc_font_default( PFont font)
{
	*font = guts. window_font;
	font-> pitch = fpDefault;
	font-> vector = fvDefault;
	return font;
}

int
apc_font_load( Handle self, char* filename)
{
	int ret = AddFontResource(( LPCTSTR) filename );
	if (ret) hash_store( mgr_myfonts, filename, strlen(filename), (void*)NULL);
	return ret;
}

static Bool recursiveFF         = 0;
static Bool recursiveFFEncoding = 0;
static Bool recursiveFFPitch    = 0;

typedef struct _FEnumStruc
{
	int           count;
	int           passedCount;
	int           resValue;
	int           heiValue;
	int           widValue;
	int           fType;
	Bool          useWidth;
	Bool          wantOutline;
	Bool          useVector;
	Bool          usePitch;
	Bool          forceSize;
	Bool          vecId;
	Bool          matchILead;
	PFont         font;
	TEXTMETRICW   tm;
	LOGFONTW      lf;
	Point         res;
	char          name[ LF_FACESIZE];
	char          family[ LF_FULLFACESIZE];
	Bool          is_utf8_name;
	Bool          is_utf8_family;
} FEnumStruc, *PFEnumStruc;

int CALLBACK
fep( ENUMLOGFONTEXW FAR *e, NEWTEXTMETRICEXW FAR *t, DWORD type, LPARAM _es)
{
	int ret = 1, copy = 0;
	long hei, res;
	PFEnumStruc es = (PFEnumStruc) _es;
	Font * font = es-> font;

	es-> passedCount++;

	if ( es-> usePitch)
	{
		int fpitch = ( t-> ntmTm. tmPitchAndFamily & TMPF_FIXED_PITCH) ? fpVariable : fpFixed;
		if ( fpitch != font-> pitch)
			return 1; // so this font cannot be selected due pitch pickup failure
	}

	if ( font-> vector != fvDefault) {
		int fvector = (( t-> ntmTm. tmPitchAndFamily & ( TMPF_VECTOR | TMPF_TRUETYPE)) ? fvOutline : fvBitmap);
		if ( fvector != font-> vector)
			return 1; // so this font cannot be selected due quality pickup failure
	}

	if ( type & TRUETYPE_FONTTYPE) {
		copy = 1;
		es-> vecId = 1;
		ret = 0; // enough; no further enumeration requred, since Win renders TrueType quite good
					// to not mix these with raster fonts.
		goto EXIT;
	} else if ( !( type & RASTER_FONTTYPE)) {
		copy = 1;
		ret  = 1; // it's vector font; but keep enumeration in case we'll get better match
		es-> vecId = 1;
		goto EXIT;
	}

	// bitmapped fonts:
	// determining best match for primary measure - size or height
	if ( es-> forceSize) {
		long xs  = ( t-> ntmTm. tmHeight - ( es-> matchILead ? t-> ntmTm. tmInternalLeading : 0))
			* 72.0 / es-> res. y + 0.5;
		hei = ( xs - font-> size) * ( xs - font-> size);
	} else {
		long dv = t-> ntmTm. tmHeight - font-> height - ( es-> matchILead ? 0 : t-> ntmTm. tmInternalLeading);
		hei = dv * dv;
	}

	// resolution difference
	res = ( t-> ntmTm. tmDigitizedAspectY - es-> res. y) * ( t-> ntmTm. tmDigitizedAspectY - es-> res. y);

	if ( hei < es-> heiValue) {
		// current height is closer than before...
		es-> heiValue = hei;
		es-> resValue = res;
		if ( es-> useWidth)
			es-> widValue = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
		copy = 1;
	} else if ( es-> useWidth && ( hei == es-> heiValue)) {
		// height is same close, but width is requested and close that before
		long wid = ( e-> elfLogFont. lfWidth - font-> width) * ( e-> elfLogFont. lfWidth - font-> width);
		if ( wid < es-> widValue) {
			es-> widValue = wid;
			es-> resValue = res;
			copy = 1;
		} else if (( wid == es-> widValue) && ( res < es-> resValue)) {
			// height and width are same close, but resolution is closer
			es-> resValue = res;
			copy = 1;
		}
	} else if (( res < es-> resValue) && ( hei == es-> heiValue)) {
		// height is same close, but resolution is closer
		es-> resValue = res;
		copy = 1;
	}

EXIT:

	if ( copy) {
		es-> count++;
		es-> fType = type;
		memcpy( &es-> tm, &t-> ntmTm, sizeof( TEXTMETRICW));
		es->is_utf8_name   = utf8_flag_strncpy( es-> family, e-> elfFullName, LF_FULLFACESIZE);
		memcpy( &es-> lf, &e-> elfLogFont, sizeof( LOGFONT));
		es->is_utf8_family = utf8_flag_strncpy( es-> name, e-> elfLogFont.lfFaceName, LF_FACESIZE);
		wcsncpy( es-> lf.lfFaceName, e-> elfLogFont.lfFaceName, LF_FACESIZE);
	}

	return ret;
}

static int
font_font2gp( PFont font, Point res, Bool forceSize, HDC dc);

static void
font_logfont2textmetric( HDC dc, LOGFONTW * lf, TEXTMETRICW * tm)
{
	HFONT hf = SelectObject( dc, CreateFontIndirectW( lf));
	GetTextMetricsW( dc, tm);
	DeleteObject( SelectObject( dc, hf));
}

int
font_font2gp_internal( PFont font, Point res, Bool forceSize, HDC theDC)
{
#define out( retVal)  if ( !theDC) dc_free(); return retVal
	FEnumStruc es;
	HDC  dc            = theDC ? theDC : dc_alloc();
	Bool useNameSubplacing = false;
	LOGFONTW elf;

	if ( !dc) return fvBitmap;

	memset( &es, 0, sizeof( es));
	es. resValue       = es. heiValue = es. widValue = INT_MAX;
	es. useWidth       = font-> width != 0;
	es. useVector      = font->vector != fvDefault;
	es. wantOutline    = (font-> vector == fvDefault && fabs(font-> direction) > 0.0001) || font->vector == fvOutline;
	es. usePitch       = font-> pitch != fpDefault;
	es. res            = res;
	es. forceSize      = forceSize;
	es. font           = font;
	es. matchILead     = forceSize ? ( font-> size >= 0) : ( font-> height >= 0);

	useNameSubplacing = es. usePitch || es. useVector;
	if ( font-> height < 0) font-> height *= -1;
	if ( font-> size   < 0) font-> size   *= -1;

	MultiByteToWideChar(CP_UTF8, 0, font->name, -1, elf.lfFaceName, LF_FACESIZE);
	elf. lfPitchAndFamily = 0;
	elf. lfCharSet = font_encoding2charset( font-> encoding);
	EnumFontFamiliesExW( dc, &elf, (FONTENUMPROCW) fep, ( LPARAM) &es, 0);

	// check encoding match
	if (( es. passedCount == 0) && ( elf. lfCharSet != DEFAULT_CHARSET) && ( recursiveFFEncoding == 0)) {
		int r;
		font-> name[0] = 0; // any name
		recursiveFFEncoding++;
		r = font_font2gp_internal( font, res, forceSize, dc);
		recursiveFFEncoding--;
		out( r);
	}

	// checking matched font, if available
	if ( es. count > 0) {
		if ( es. wantOutline) {
			if ( !es. vecId) useNameSubplacing = 1; // try other font if bitmap wouldn't fit
			es. resValue = es. heiValue = INT_MAX;  // cancel bitmap font selection
		}

		// special synthesized GDI font case
		if (
			es. useWidth && ( es. widValue > 0) &&
			( es. heiValue == 0) &&
			( es. fType & RASTER_FONTTYPE) &&
			( font-> style & fsBold)
		) {
				int r;
				Font xf = *font;
				xf. width--;
				xf. style = 0; // any style could affect tmOverhang
				r = font_font2gp( &xf, res, forceSize, dc);
				if (( r == fvBitmap) && ( xf. width < font-> width)) {
					LOGFONTW lpf;
					TEXTMETRICW tm;
					font_font2logfont( &xf, &lpf);
					lpf. lfWeight = 700;
					font_logfont2textmetric( dc, &lpf, &tm);
					if ( xf. width + tm. tmOverhang == font-> width) {
						font_textmetric2font( &tm, font, true);
						font-> direction = 0;
						font-> size   = xf. size;
						font-> height = xf. height;
						font-> maximalWidth += tm. tmOverhang;
						out( fvBitmap);
					}
				}
			}

		// have resolution ok? so using raster font mtx
		if (
			( es. heiValue < 2) &&
			( es. fType & RASTER_FONTTYPE) &&
			( !es. useWidth  || (( es. widValue == 0) && ( es. heiValue == 0)))
			)
		{
			font-> height   = es. tm. tmHeight;
			// if synthesized embolding added, we have to reflect that...
			// ( 'cos it increments B-extent of a char cell).
			if ( font-> style & fsBold) {
				LOGFONTW lpf = es. lf;
				TEXTMETRICW tm;
				lpf. lfWeight = 700; // ignore italics, it changes tmOverhang also
				font_logfont2textmetric( dc, &lpf, &tm);
				es. tm. tmMaxCharWidth += tm. tmOverhang;
				es. lf. lfWidth        += tm. tmOverhang;
			}
			font_textmetric2font( &es. tm, font, true);
			font-> direction = 0;
			strlcpy( font-> family, es. family, LF_FULLFACESIZE);
			font-> is_utf8.family = es.is_utf8_family;
			font-> size     = ( es. tm. tmHeight - es. tm. tmInternalLeading) * 72.0 / res.y + 0.5;
			font-> width    = es. lf. lfWidth;
			out( fvBitmap);
		}

		// or vector font - for any purpose?
		// if so, it could guaranteed that font-> height == tmHeight
		if ( es. vecId) {
			LOGFONTW lpf = es. lf;
			TEXTMETRICW tm;

			// since proportional computation of small items as InternalLeading
			// gives incorrect result, querying the only valid source - GetTextMetrics
			if ( forceSize) {
				lpf. lfHeight = font-> size * res. y / 72.0 + 0.5;
				if ( es. matchILead) lpf. lfHeight *= -1;
			} else
				lpf. lfHeight = es. matchILead ? font-> height : -font-> height;

			lpf. lfWidth = es. useWidth ? font-> width : 0;

			font_logfont2textmetric( dc, &lpf, &tm);
			if ( forceSize)
				font-> height = tm. tmHeight;
			else
				font-> size = ( tm. tmHeight - tm. tmInternalLeading) * 72.0 / res. y + 0.5;

			if ( !es. useWidth)
				font-> width = tm. tmAveCharWidth;

			font_textmetric2font( &tm, font, true);
			strlcpy( font-> family, es. family, LF_FULLFACESIZE);
			font-> is_utf8.family = es.is_utf8_family;
			out( fvOutline);
		}
	}

	// if strict match not found, use subplacing
	if ( useNameSubplacing && recursiveFFPitch == 0)
	{
		int ret;
		int ogp  = font-> pitch;

		if ( es. usePitch && es. useVector ) {
			// is this too much? try first without vector
			font-> vector = fvDefault;
			ret = font_font2gp( font, res, forceSize, dc);
			out(ret);
		}

		// setting some other( maybe other) font name
		if ( es. usePitch) {
			switch ( font->pitch ) {
			case fpFixed:
				strcpy( font-> name, guts. default_fixed_font);
				break;
			case fpVariable:
				strcpy( font-> name, guts. default_variable_font);
				break;
			}
			font-> pitch = fpDefault;
		}

		if ( es. useVector )
			font-> vector = fvDefault;

		recursiveFFPitch++;
		ret = font_font2gp( font, res, forceSize, dc);
		// if that alternative match succeeded with name subplaced again, skip
		// that result and use DEFAULT_SYSTEM_FONT match
		if (( ogp == fpFixed) && ( strcmp( font-> name, guts. default_fixed_font) != 0)) {
			strcpy( font-> name, DEFAULT_SYSTEM_FONT);
			font-> pitch = fpDefault;
			ret = font_font2gp( font, res, forceSize, dc);
		}
		recursiveFFPitch--;
		out( ret);
	}

	// font not found, so use general representation for height and width
	strcpy( font-> name, guts. default_system_font);
	if ( recursiveFF == 0)
	{
		// trying to catch default font with correct values
		int r;
		recursiveFF++;
		r = font_font2gp( font, res, forceSize, dc);
		recursiveFF--;
		out( r);
	} else {
		int r;
		// if not succeeded, to avoid recursive call use "wild guess".
		// This could be achieved if system does not have "System" font
		*font = guts. window_font;
		font-> pitch = fpDefault;
		recursiveFF++;
		r = ( recursiveFF < 3) ? font_font2gp( font, res, forceSize, dc) : fvBitmap;
		recursiveFF--;
		out( r);
	}
	return fvBitmap;
#undef out
}

static int
font_font2gp( PFont font, Point res, Bool forceSize, HDC dc)
{
	Font key;
	Bool addSizeEntry;
	font-> resolution = res. y * 0x10000 + res. x;
	if ( get_font_from_hash( font, forceSize))
		return font->vector;
	memcpy( &key, font, sizeof( Font));
	font_font2gp_internal( font, res, forceSize, dc);
	font-> resolution = res. y * 0x10000 + res. x;
	if ( forceSize) {
		key. height = font-> height;
		addSizeEntry = true;
	} else {
		key. size  = font-> size;
		addSizeEntry = (font->vector > fvBitmap) ? (( key. height == key. width)  || ( key. width == 0)) : true;
	}
	add_font_to_hash( &key, font, addSizeEntry);
	return font->vector;
}

Bool
apc_font_pick( Handle self, PFont source, PFont dest)
{
	if ( self) objCheck false;
	font_font2gp( dest, apc_gp_get_resolution(self),
		Drawable_font_add( self, source, dest), self ? sys ps : 0);
	return true;
}

typedef struct {
	List  lst;
	PHash hash;
} Fep2;

int CALLBACK
fep2( ENUMLOGFONTEXW FAR *e, NEWTEXTMETRICEXW FAR *t, DWORD type, LPARAM _f)
{
	PFont fm;
	char name[256];
	Fep2 *f = (Fep2*) _f;
	Bool name_is_utf8;

	if ( e-> elfLogFont.lfFaceName[0] == '@') return 1; /* skip vertical fonts */

	name_is_utf8 = utf8_flag_strncpy( name, e-> elfLogFont.lfFaceName, LF_FACESIZE);

	if ( f-> hash) { /* gross-family enumeration */
		fm = hash_fetch( f-> hash, name, strlen( name));
		if ( fm) {
			char ** enc = (char**) fm-> encoding;
			unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
			if ( *shift + 2 < 256 / sizeof(char*)) {
				*(enc + ++(*shift)) = font_charset2encoding( e-> elfLogFont. lfCharSet);
			}
			return 1;
		}
	}
	fm = ( PFont) malloc( sizeof( Font));
	if ( !fm) return 1;
	memset( fm, 0, sizeof(Font));
	font_textmetric2font(( TEXTMETRICW*) &t-> ntmTm, fm, false);
	if ( f-> hash) { /* multi-encoding format */
		char ** enc = (char**) fm-> encoding;
		unsigned char * shift = (unsigned char*)enc + sizeof(char *) - 1;
		memset( fm-> encoding, 0, 256);
		*(enc + ++(*shift)) = font_charset2encoding( e-> elfLogFont. lfCharSet); /* lfCharSet is A/W-safe */
		hash_store( f-> hash, name, strlen( name), fm);
	}
	fm-> direction = fm-> resolution = 0;
	fm-> is_utf8.name = name_is_utf8;
	strcpy( fm-> name, name);
	fm-> is_utf8.family = utf8_flag_strncpy( fm-> family, e-> elfFullName, LF_FULLFACESIZE);
	list_add( &f-> lst, ( Handle) fm);
	return 1;
}

PFont
apc_fonts( Handle self, const char* facename, const char *encoding, int * retCount)
{
	PFont fmtx = NULL;
	int  i;
	HDC  dc;
	Fep2 f;
	Bool hasdc = 0;
	LOGFONTW elf;
	apcErrClear;

	*retCount = 0;
	if ( self == NULL_HANDLE || self == prima_guts.application) {
		if ( !( dc = dc_alloc())) return NULL;
	}
	else if ( kind_of( self, CPrinter)) {
		if ( !is_opt( optInDraw) && !is_opt( optInDrawInfo)) {
			hasdc = 1;
			CPrinter( self)-> begin_paint_info( self);
		}
		dc = sys ps;
	} else
		return NULL;

	f. hash = NULL;
	if ( !facename && !encoding)
		if ( !( f. hash = hash_create()))
			return NULL;
	list_create( &f. lst, 256, 256);
	memset( &elf, 0, sizeof( elf));
	MultiByteToWideChar(CP_UTF8, 0, facename ? facename : "", -1, elf.lfFaceName, LF_FACESIZE);
	elf. lfCharSet = font_encoding2charset( encoding);
	EnumFontFamiliesExW( dc, &elf, (FONTENUMPROCW) fep2, ( LPARAM) &f, 0);
	if ( f. hash) {
		hash_destroy( f. hash, false);
		f. hash = NULL;
	}

	if ( self == NULL_HANDLE || self == prima_guts.application)
		dc_free();
	else if ( hasdc)
		CPrinter( self)-> end_paint_info( self);

	if ( f. lst. count == 0) goto Nothing;
	fmtx = ( PFont) malloc( f. lst. count * sizeof( Font));
	if ( !fmtx) return NULL;

	*retCount = f. lst. count;
	for ( i = 0; i < f. lst. count; i++)
		memcpy( &fmtx[ i], ( void *) f. lst. items[ i], sizeof( Font));
	list_delete_all( &f. lst, true);
Nothing:
	list_destroy( &f. lst);
	return fmtx;
}


int CALLBACK
fep3( ENUMLOGFONTEXW FAR *e, NEWTEXTMETRICW FAR *t, DWORD type, LPARAM _lst)
{
	PHash lst = (PHash) _lst;
	char * str = font_charset2encoding( e-> elfLogFont. lfCharSet);
	hash_store( lst, str, strlen( str), (void*)1);
	return 1;
}

PHash
apc_font_encodings( Handle self )
{
	HDC  dc;
	PHash lst;
	Bool hasdc = 0;
	LOGFONTW elf;
	apcErrClear;

	if ( self == NULL_HANDLE || self == prima_guts.application) {
		if ( !( dc = dc_alloc())) return NULL;
	}
	else if ( kind_of( self, CPrinter)) {
		if ( !is_opt( optInDraw) && !is_opt( optInDrawInfo)) {
			hasdc = 1;
			CPrinter( self)-> begin_paint_info( self);
		}
		dc = sys ps;
	} else
		return NULL;

	lst = hash_create();
	memset( &elf, 0, sizeof( elf));
	elf. lfCharSet = DEFAULT_CHARSET;
	EnumFontFamiliesExW( dc, &elf, (FONTENUMPROCW) fep3, ( LPARAM) lst, 0);

	if ( self == NULL_HANDLE || self == prima_guts.application)
		dc_free();
	else if ( hasdc)
		CPrinter( self)-> end_paint_info( self);

	return lst;
}


// Font end
// Colors section


#define stdDisabled  COLOR_GRAYTEXT        ,  COLOR_BTNFACE
#define stdHilite    COLOR_HIGHLIGHTTEXT   ,  COLOR_HIGHLIGHT
#define std3d        COLOR_BTNHIGHLIGHT    ,  COLOR_BTNSHADOW

static Handle buttonScheme[] = {
	COLOR_BTNTEXT,   COLOR_BTNFACE,
	COLOR_BTNTEXT,   COLOR_BTNFACE,
	COLOR_GRAYTEXT,  COLOR_BTNFACE,
	std3d
};
static Handle sliderScheme[] = {
	COLOR_WINDOWTEXT,       COLOR_SCROLLBAR,
	COLOR_WINDOWTEXT,       COLOR_SCROLLBAR,
	stdDisabled,
	std3d
};

static Handle dialogScheme[] = {
	COLOR_WINDOWTEXT, COLOR_BTNFACE,
	COLOR_CAPTIONTEXT, COLOR_ACTIVECAPTION,
	COLOR_INACTIVECAPTIONTEXT, COLOR_INACTIVECAPTION,
	std3d
};
static Handle staticScheme[] = {
	COLOR_WINDOWTEXT, COLOR_BTNFACE,
	COLOR_WINDOWTEXT, COLOR_BTNFACE,
	stdDisabled,
	std3d
};
static Handle editScheme[] = {
	COLOR_WINDOWTEXT, COLOR_WINDOW,
	stdHilite,
	stdDisabled,
	std3d
};
static Handle menuScheme[] = {
	COLOR_MENUTEXT, COLOR_MENU,
	stdHilite,
	stdDisabled,
	std3d
};

static Handle scrollScheme[] = {
	COLOR_WINDOWTEXT,    COLOR_BTNFACE,
	stdHilite,
	stdDisabled,
	std3d
};

static Handle windowScheme[] = {
	COLOR_WINDOWTEXT,  COLOR_BTNFACE,
	COLOR_CAPTIONTEXT, COLOR_ACTIVECAPTION,
	COLOR_INACTIVECAPTIONTEXT, COLOR_INACTIVECAPTION,
	std3d
};
static Handle customScheme[] = {
	COLOR_WINDOWTEXT, COLOR_BTNFACE,
	stdHilite,
	stdDisabled,
	std3d
};

static Handle ctx_wc2SCHEME[] =
{
	wcButton    , ( Handle) &buttonScheme,
	wcCheckBox  , ( Handle) &buttonScheme,
	wcRadio     , ( Handle) &buttonScheme,
	wcDialog    , ( Handle) &dialogScheme,
	wcSlider    , ( Handle) &sliderScheme,
	wcLabel     , ( Handle) &staticScheme,
	wcInputLine , ( Handle) &editScheme,
	wcEdit      , ( Handle) &editScheme,
	wcListBox   , ( Handle) &editScheme,
	wcCombo     , ( Handle) &editScheme,
	wcMenu      , ( Handle) &menuScheme,
	wcPopup     , ( Handle) &menuScheme,
	wcScrollBar , ( Handle) &scrollScheme,
	wcWindow    , ( Handle) &windowScheme,
	wcWidget    , ( Handle) &customScheme,
	endCtx
};


long
remap_color( long clr, Bool toSystem)
{
	if ( toSystem && ( clr & clSysFlag)) {
		long c = clr;
		Handle * scheme = ( Handle *) ctx_remap_def( clr & wcMask, ctx_wc2SCHEME, true, ( Handle) &customScheme);
		if (( clr = ( clr & ~wcMask)) > clMaxSysColor) clr = clMaxSysColor;
		if ( clr == clSet || clr == clInvalid) return 0xFFFFFF;
		if ( clr == clClear) return 0;
		c = GetSysColor( scheme[( clr & clSysMask) - 1]);
		return c;
	} else {
		PRGBColor cp = ( PRGBColor) &clr;
		unsigned char sw = cp-> r;
		cp-> r = cp-> b;
		cp-> b = sw;
		return clr;
	}
}

Color
apc_lookup_color( const char * colorName)
{
	char buf[ 256];
	char *b;
	int len;

#define xcmp( name, stlen, retval)  if (( len == stlen) && ( strcmp( name, buf) == 0)) return retval

	strlcpy( buf, colorName, 255);
	len = strlen( buf);
	for ( b = buf; *b; b++) *b = tolower(*b);

	switch( buf[0]) {
	case 'a':
		xcmp( "aqua", 4, 0x00FFFF);
		xcmp( "azure", 5, ARGB(240,255,255));
		break;
	case 'b':
		xcmp( "black", 5, 0x000000);
		xcmp( "blanchedalmond", 14, ARGB( 255,235,205));
		xcmp( "blue", 4, 0x000080);
		xcmp( "brown", 5, 0x808000);
		xcmp( "beige", 5, ARGB(245,245,220));
		break;
	case 'c':
		xcmp( "cyan", 4, 0x008080);
		xcmp( "chocolate", 9, ARGB(210,105,30));
		break;
	case 'd':
		xcmp( "darkgray", 8, 0x404040);
		break;
	case 'e':
		break;
	case 'f':
		xcmp( "fuchsia", 7, 0xFF00FF);
		break;
	case 'g':
		xcmp( "green", 5, 0x008000);
		xcmp( "gray", 4, 0x808080);
		xcmp( "gray80", 6, ARGB(204,204,204));
		xcmp( "gold", 4, ARGB(255,215,0));
		break;
	case 'h':
		xcmp( "hotpink", 7, ARGB(255,105,180));
		break;
	case 'i':
		xcmp( "ivory", 5, ARGB(255,255,240));
		break;
	case 'j':
		break;
	case 'k':
		xcmp( "khaki", 5, ARGB(240,230,140));
		break;
	case 'l':
		xcmp( "lime", 4, 0x00FF00);
		xcmp( "lightgray", 9, 0xC0C0C0);
		xcmp( "lightblue", 9, 0x0000FF);
		xcmp( "lightgreen", 10, 0x00FF00);
		xcmp( "lightcyan", 9, 0x00FFFF);
		xcmp( "lightmagenta", 12, 0xFF00FF);
		xcmp( "lightred", 8, 0xFF0000);
		xcmp( "lemon", 5, ARGB(255,250,205));
		break;
	case 'm':
		xcmp( "maroon", 6, 0x800000);
		xcmp( "magenta", 7, 0x800080);
		break;
	case 'n':
		xcmp( "navy", 4, 0x000080);
		break;
	case 'o':
		xcmp( "olive", 5, 0x808000);
		xcmp( "orange", 6, ARGB(255,165,0));
		break;
	case 'p':
		xcmp( "purple", 6, 0x800080);
		xcmp( "peach", 5, ARGB(255,218,185));
		xcmp( "peru", 4, ARGB(205,133,63));
		xcmp( "pink", 4, ARGB(255,192,203));
		xcmp( "plum", 4, ARGB(221,160,221));
		break;
	case 'q':
		break;
	case 'r':
		xcmp( "red", 3, 0x800000);
		xcmp( "royalblue", 9, ARGB(65,105,225));
		break;
	case 's':
		xcmp( "silver", 6, 0xC0C0C0);
		xcmp( "sienna", 6, ARGB(160,82,45));
		break;
	case 't':
		xcmp( "teal", 4, 0x008080);
		xcmp( "turquoise", 9, ARGB(64,224,208));
		xcmp( "tan", 3, ARGB(210,180,140));
		xcmp( "tomato", 6, ARGB(255,99,71));
		break;
	case 'u':
		break;
	case 'w':
		xcmp( "white", 5, 0xFFFFFF);
		xcmp( "wheat", 5, ARGB(245,222,179));
		break;
	case 'v':
		xcmp( "violet", 6, ARGB(238,130,238));
		break;
	case 'x':
		break;
	case 'y':
		xcmp( "yellow", 6, 0xFFFF00);
		break;
	case 'z':
		break;
	}

#undef xcmp

	return clInvalid;
}

// Colors end
// Miscellaneous

static HDC cachedScreenDC = NULL;
static int cachedScreenRC = 0;
static HDC cachedCompatDC = NULL;
static int cachedCompatRC = 0;


HDC dc_alloc()
{
	if ( cachedScreenRC++ == 0) {
		if ( !( cachedScreenDC = GetDC( 0)))
			apiErr;
	}
	return cachedScreenDC;
}

void dc_free()
{
	if ( --cachedScreenRC <= 0)
		ReleaseDC( 0, cachedScreenDC);
	if ( cachedScreenRC < 0)
		cachedScreenRC = 0;
}

HDC dc_compat_alloc( HDC compatDC)
{
	if ( cachedCompatRC++ == 0) {
		if ( !( cachedCompatDC = CreateCompatibleDC( compatDC)))
			apiErr;
	}
	return cachedCompatDC;
}

void dc_compat_free()
{
	if ( --cachedCompatRC <= 0)
		DeleteDC( cachedCompatDC);
	if ( cachedCompatRC < 0)
		cachedCompatRC = 0;
}

void
hwnd_enter_paint( Handle self)
{
	Point res;
	HDC save_ps;

	save_ps = sys ps;
	sys ps = 0;
	apc_gp_push(self, NULL, NULL, 0);
	sys ps = save_ps;

	apt_clear(aptWantWorldTransform);
	apt_clear(aptUsedWorldTransform);
	apt_clear(aptCachedWorldTransform);

	SetGraphicsMode( sys ps, GM_ADVANCED);
	sys stock_pen   = GetCurrentObject( sys ps, OBJ_PEN);
	sys stock_brush = GetCurrentObject( sys ps, OBJ_BRUSH);
	sys stock_font  = GetCurrentObject( sys ps, OBJ_FONT);
	if ( !sys stock_palette)
		sys stock_palette = GetCurrentObject( sys ps, OBJ_PAL);
	font_free( sys dc_font, false);
	stylus_release(self);
	sys dc_font   = NULL;
	sys stylus_flags    = 0;
	apt_set( aptDCChangeLock);
	sys bpp = GetDeviceCaps( sys ps, BITSPIXEL);
	if ( is_apt( aptWinPS) && self != prima_guts.application) {
		apc_gp_set_color( self, sys view_colors[ ciFore]);
		apc_gp_set_back_color( self, sys view_colors[ ciBack]);
	} else {
		apc_gp_set_color( self, remap_color(sys fg,false));
		apc_gp_set_back_color( self, remap_color(sys bg,false));
	}

	apc_gp_set_alpha( self, sys alpha);
	apc_gp_set_antialias( self, is_apt( aptGDIPlus));
	apc_gp_set_text_opaque( self, is_apt( aptTextOpaque));
	apc_gp_set_text_out_baseline( self, is_apt( aptTextOutBaseline));
	apc_gp_set_fill_mode( self, sys fill_mode);
	apc_gp_set_fill_pattern_offset( self, sys fill_pattern_offset);
	apc_gp_set_line_pattern( self,
		( Byte*)(( sys line_pattern_len > sizeof(sys line_pattern)) ? sys line_pattern : ( Byte*)&sys line_pattern),
		sys line_pattern_len);
	apc_gp_set_rop( self, sys rop);
	apc_gp_set_rop2( self, sys rop2);
	if ( var fillPatternImage ) {
		if ( PObject(var fillPatternImage)->stage == csNormal )
			apc_gp_set_fill_image( self, var fillPatternImage);
	} else
		apc_gp_set_fill_pattern( self, sys fill_pattern);

	apt_clear( aptDCChangeLock);
	sys stylus_flags = 0;
	apc_gp_set_font( self, &var font);
	res = apc_gp_get_resolution(self);
	var font. resolution = res. y * 0x10000 + res. x;
	SetStretchBltMode( sys ps, COLORONCOLOR);
}

static void
gc_stack_free( Handle self, PPaintState state)
{
	if ( state-> fill_image )
		unprotect_object( state-> fill_image );
	if ( state-> in_paint) {
		int i;
		for ( i = 0; i < DCO_COUNT; i++)
			stylus_free(state->paint.dc_obj[i], 0);
	}
	if ( state-> user_destructor )
		state-> user_destructor(self, state->user_data, state->user_data_size, state->in_paint);
	free(state);
}

static Bool
gc_stack_free_paints_only( Handle item, void * p)
{
	PPaintState state = ( PPaintState ) item;
	if ( !state->in_paint) return true;
	gc_stack_free((Handle) p, state);
	return false;
}

static Bool
gc_stack_free_all( Handle item, void * p)
{
	gc_stack_free((Handle) p, ( PPaintState ) item);
	return false;
}

void
cleanup_gc_stack(Handle self, Bool all)
{
	if ( !sys gc_stack ) return;

	if ( all ) {
		list_first_that(sys gc_stack, &gc_stack_free_all, (void*) self);
		plist_destroy(sys gc_stack);
		sys gc_stack = NULL;
	} else
		list_grep(sys gc_stack, &gc_stack_free_paints_only, (void*) self);
}

void
hwnd_leave_paint( Handle self)
{
	HDC save_ps;
	cleanup_gc_stack(self, 0);
	if ( sys graphics) {
		GdipDeleteGraphics(sys graphics);
		sys graphics = NULL;
	}
	SelectObject( sys ps,  sys stock_pen);
	SelectObject( sys ps,  sys stock_brush);
	SelectObject( sys ps,  sys stock_font);
	SelectPalette( sys ps, sys stock_palette, 0);
	sys stock_pen = NULL;
	sys stock_brush = NULL;
	sys stock_font = NULL;
	sys stock_palette = NULL;
	stylus_release(self);
	sys bpp = 0;
	select_world_transform(self, false);

	save_ps = sys ps;
	sys ps = 0;
	apc_gp_pop(self, NULL);

	sys ps = save_ps;
}

Bool
hwnd_repaint_layered( Handle self, Bool now )
{
	Event ev;
	Handle top;

	top = hwnd_layered_top_level( self );
	if ( top && top != self && dsys(top) options. aptLayered )
		return hwnd_repaint_layered(top, now);

	if ( !is_apt( aptLayered)) return false;

	if ( !now && !is_apt( aptSyncPaint) ) {
		if ( !is_apt( aptRepaintPending )) {
			apt_set( aptRepaintPending );
			PostMessage(( HWND) var handle, WM_REPAINT_LAYERED, 0, 0);
		}
		return false;
	}

	apt_clear( aptRepaintPending );
	ev. cmd = cmPaint;
	CWidget(self)-> message( self, &ev);

	return true;
}

Handle
hwnd_top_level( Handle self)
{
	while ( self) {
		if ( sys class_name == WC_FRAME) return self;
		self = var owner;
	}
	return NULL_HANDLE;
}

Handle
hwnd_frame_top_level( Handle self)
{
	while ( self && ( self != prima_guts.application)) {
		if (( sys class_name == WC_FRAME) ||
			( !is_apt( aptClipOwner) && ( var owner != prima_guts.application))) return self;
		self = var owner;
	}
	return NULL_HANDLE;
}

Handle
hwnd_layered_top_level( Handle self)
{
	while ( self && ( self != prima_guts.application)) {
		if (( sys class_name == WC_FRAME) ||
			(!is_apt( aptClipOwner) || (var owner == prima_guts.application))) return self;
		self = var owner;
	}
	return NULL_HANDLE;
}

typedef struct _ModData
{
	BYTE  ks  [ 256];
	BYTE  kss [ 3];
	BYTE *gks;
} ModData;


BYTE *
mod_select( int mod)
{
	ModData * ks;

	ks = ( ModData*) malloc( sizeof( ModData));
	if ( !ks) return NULL;

	GetKeyboardState( ks-> ks);
	ks-> kss[ 0]   = ks-> ks[ VK_MENU];
	ks-> kss[ 1]   = ks-> ks[ VK_CONTROL];
	ks-> kss[ 2]   = ks-> ks[ VK_SHIFT];
	ks-> ks[ VK_MENU   ] = ( mod & kmAlt  ) ? 0x80 : 0;
	ks-> ks[ VK_CONTROL] = ( mod & kmCtrl ) ? 0x80 : 0;
	ks-> ks[ VK_SHIFT  ] = ( mod & kmShift) ? 0x80 : 0;
	SetKeyboardState( ks-> ks);
	ks-> gks = guts. current_key_state;
	guts. current_key_state = ks-> ks;
	return ( BYTE*) ks;
}

void
mod_free( BYTE * modState)
{
	ModData * ks = ( ModData*) modState;
	if ( !ks) return;

	ks-> ks[ VK_MENU   ] = ks-> kss[ 0];
	ks-> ks[ VK_CONTROL] = ks-> kss[ 1];
	ks-> ks[ VK_SHIFT  ] = ks-> kss[ 2];
	SetKeyboardState( ks-> ks);
	guts. current_key_state = ks-> gks;
	free( ks);
}

typedef struct _SzList
{
	List l;
	int  sz;
	PRGBColor p;
} SzList, *PSzList;

static Bool
pal_count( Handle window, Handle self, PSzList l)
{
	if ( !is_apt( aptClipOwner) && ( window != prima_guts.application))
		return false;
	if ( var palSize > 0) {
		list_add( &l->l, self);
		l->sz += var palSize;
	}
	if ( var widgets. count > 0)
		CWidget( self)-> first_that( self, pal_count, l);
	return false;
}

static Bool
pal_collect( Handle self, PSzList l)
{
	memcpy( l-> p, var palette, var palSize * sizeof( RGBColor));
	l-> p  += var palSize;
	return false;
}

static Bool
repaint_all( Handle owner, Handle self, void * dummy)
{
	objCheck false;
	if ( !is_apt( aptClipOwner))
		return false;
	if ( !is_apt( aptTransparent)) {
		if ( !InvalidateRect(( HWND) var handle, NULL, false)) apiErr;
		if ( is_apt( aptSyncPaint) && !apcUpdateWindow(( HWND) var handle)) apiErr;
		objCheck false;
		var self-> first_that( self, repaint_all, NULL);
	}
	process_transparents( self);
	return false;
}

void
hwnd_repaint( Handle self)
{
	objCheck;
	if ( !InvalidateRect (( HWND) var handle, NULL, false)) apiErr;
	if ( is_apt( aptSyncPaint) && !apcUpdateWindow(( HWND) var handle)) apiErr;
	objCheck;
	var self-> first_that( self, repaint_all, NULL);
	process_transparents( self);
}

Bool
palette_change( Handle self)
{
	SzList l;
	PRGBColor p;
	PRGBColor d;
	int nColors = ( 1 << (
		guts. display_bm_info. bmiHeader. biBitCount *
		guts. display_bm_info. bmiHeader. biPlanes
	)) & 0x1FF;
	int i;
	HPALETTE pal;
	XLOGPALETTE xlp = {0x300};
	HDC dc;

	if ( nColors == 0)
		return false;

	self = hwnd_frame_top_level( self);
	if ( self == NULL_HANDLE) return false;

	list_create( &l.l, 32, 32);
	l. sz = 0;
	if ( var palSize > 0) {
		list_add( &l.l, self);
		l.sz += var palSize;
	}
	if ( var widgets. count > 0)
		CWidget( self)-> first_that( self, pal_count, &l);

	if ( l. l. count == 0) {
		list_destroy( &l.l);
		hwnd_repaint( self);
		return false;
	}


	xlp. palNumEntries = l. sz;
	p = ( PRGBColor) malloc( sizeof( RGBColor) * l. sz);
	if ( !p) {
		list_destroy( &l.l);
		hwnd_repaint( self);
		return false;
	}

	d = ( PRGBColor) malloc( sizeof( RGBColor) * nColors);
	if ( !d) {
		free( p);
		list_destroy( &l.l);
		hwnd_repaint( self);
		return false;
	}

	l. p = p;
	list_first_that( &l.l, pal_collect, &l);
	cm_squeeze_palette( p, xlp. palNumEntries, d, nColors);
	xlp. palNumEntries = nColors;

	for ( i = 0; i < nColors; i++) {
		xlp. palPalEntry[ i]. peRed    = d[ i]. r;
		xlp. palPalEntry[ i]. peGreen  = d[ i]. g;
		xlp. palPalEntry[ i]. peBlue   = d[ i]. b;
		xlp. palPalEntry[ i]. peFlags  = 0;
	}

	free( d);
	free( p);

	pal = CreatePalette(( LOGPALETTE *) &xlp);

	dc  = GetDC( HANDLE);
	pal  = SelectPalette( dc, pal, 0);
	RealizePalette( dc);
	DeleteObject( SelectPalette( dc, pal, 0));
	ReleaseDC( HANDLE, dc);

	hwnd_repaint( self);

	list_destroy( &l.l);
	return true;
}

int
palette_match_color( XLOGPALETTE * lp, long clr, int * diff_factor)
{
	int diff = 0x10000, cdiff = 0, ret = 0, nCol = lp-> palNumEntries;
	RGBColor color;

	if ( nCol == 0) {
		if ( diff_factor) *diff_factor = 0;
		return clr;
	}

	color. r = clr & 0xFF;
	color. g = ( clr >> 8)  & 0xFF;
	color. b = ( clr >> 16) & 0xFF;

	while( nCol--)
	{
		int dr=abs((int)color. r - (int)lp-> palPalEntry[ nCol]. peRed),
			dg=abs((int)color. g - (int)lp-> palPalEntry[ nCol]. peGreen),
			db=abs((int)color. b - (int)lp-> palPalEntry[ nCol]. peBlue);
		cdiff=dr*dr+dg*dg+db*db;
		if ( cdiff < diff) {
			ret  = nCol;
			diff = cdiff;
			if ( cdiff == 0) break;
		}
	}

	if ( diff_factor) *diff_factor = cdiff;
	return ret;
}


long
palette_match( Handle self, long clr)
{
	XLOGPALETTE lp;
	int cdiff;
	RGBColor color;

	lp. palNumEntries = GetPaletteEntries( sys pal, 0, 256, lp. palPalEntry);

	if ( lp. palNumEntries == 0) {
		apiErr;
		return clr;
	}

	color. r = clr & 0xFF;
	color. g = ( clr >> 8)  & 0xFF;
	color. b = ( clr >> 16) & 0xFF;

	palette_match_color( &lp, clr, &cdiff);

	if ( cdiff >= COLOR_TOLERANCE)
		return clr;

	lp. palNumEntries = GetSystemPaletteEntries( sys ps, 0, 256, lp. palPalEntry);

	palette_match_color( &lp, clr, &cdiff);

	if ( cdiff >= COLOR_TOLERANCE)
		return clr;

	return PALETTERGB( color.r, color.g, color.b);
}

int
arc_completion( double * angleStart, double * angleEnd, int * needFigure)
{
	int max;
	long diff = ((long)( fabs( *angleEnd - *angleStart) * 1000 + 0.5));

	if ( diff == 0) {
		*needFigure = false;
		return 0;
	}
	diff /= 1000;

	while ( *angleStart > *angleEnd)
		*angleEnd += 360;

	while ( *angleStart < 0) {
		*angleStart += 360;
		*angleEnd   += 360;
	}

	while ( *angleStart >= 360) {
		*angleStart -= 360;
		*angleEnd   -= 360;
	}

	while ( *angleEnd >= *angleStart + 360)
		*angleEnd -= 360;

	if ( diff < 360) {
		*needFigure = true;
		return 0;
	}

	max = (int)(diff / 360);
	*needFigure = ( max * 360) != diff;
	return ( max % 2) ? 1 : 2;
}


#ifdef __cplusplus
}
#endif
