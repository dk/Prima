/*
* System dependent image routines (unix, x11)
*/

#include "unix/guts.h"
#include "Image.h"
#include "Icon.h"
#include "DeviceBitmap.h"

#define REVERT(a)	( XX-> size. y - (a) - 1 )
#define SHIFT(a,b)	{ (a) += XX-> btransform. x; (b) += XX-> btransform. y; }
/* Multiple evaluation macro! */
#define REVERSE_BYTES_32(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))
#define REVERSE_BYTES_16(x) ((((x)&0xff)<<8 ) | (((x)&0xff00)>>8))

#define ByteBits                8
#define ByteMask                0xff
#define ByteValues              256
#define LOWER_BYTE(x)           ((x)&ByteMask)
#define ColorComponentMask      ByteMask
#define LSNibble                0x0f
#define LSNibbleShift           0
#define MSNibble                0xf0
#define MSNibbleShift           4
#define NPalEntries4            16
#define NPalEntries8            256

typedef U8 Pixel8;
typedef unsigned long XPixel;

typedef uint16_t Pixel16;

typedef struct
{
	Pixel16 a;
	Pixel16 b;
} Duplet16;

typedef struct
{
	ColorComponent a0, a1, a2;
} Pixel24;

typedef struct
{
	ColorComponent a0, a1, a2;
	ColorComponent b0, b1, b2;
} Duplet24;

typedef uint32_t Pixel32;

typedef struct
{
	Pixel32 a;
	Pixel32 b;
} Duplet32;


#define get_ximage_data(xim)            ((xim)->data_alias)
#define get_ximage_bytes_per_line(xim)  ((xim)->bytes_per_line_alias)

#ifdef USE_MITSHM
static int
shm_ignore_errors(Display *d, XErrorEvent *ev)
{
	guts.xshmattach_failed = true;
	return 0;
}
#endif

PrimaXImage*
prima_prepare_ximage( int width, int height, int format)
{
	PrimaXImage *i;
	int extra_bytes, depth, pformat, idepth;
	Visual *visual;

	if (width == 0 || height == 0) return false;

	switch(format) {
	case CACHE_BITMAP:
		depth   = 1;
		idepth  = 1;
		visual  = guts.visual.visual;
		pformat = XYBitmap;
		break;
	case CACHE_LAYERED:
		if ( guts.render_supports_argb32 ) {
			depth   = guts. argb_visual.depth;
			idepth  = guts. argb_depth;
			visual  = guts. argb_visual. visual;
			pformat = ZPixmap;
			break;
		}
	case CACHE_PIXMAP:
		idepth  = guts.idepth;
		depth   = guts.depth;
		visual  = guts.visual.visual;
		pformat = ZPixmap;
		break;
	default:
		croak("bad call to prima_prepare_ximage");
	}

	switch ( idepth) {
	case 16:     extra_bytes = 1;        break;
	case 24:     extra_bytes = 5;        break;
	case 32:     extra_bytes = 7;        break;
	default:     extra_bytes = 0;
	}

	i = malloc( sizeof( PrimaXImage));
	if (!i) {
		warn("Not enough memory");
		return NULL;
	}
	bzero( i, sizeof( PrimaXImage));

#ifdef USE_MITSHM
	if ( guts. shared_image_extension && format != CACHE_BITMAP) {
		i-> image = XShmCreateImage(
			DISP, visual, depth, pformat,
			NULL, &i->xmem, width, height
		);
		XCHECKPOINT;
		if ( !i-> image) goto normal_way;
		i-> bytes_per_line_alias = i-> image-> bytes_per_line;
		i-> xmem. shmid = shmget(
			IPC_PRIVATE,
			i-> image-> bytes_per_line * height + extra_bytes,
			IPC_CREAT | 0666
		);
		if ( i-> xmem. shmid < 0) {
			XDestroyImage( i-> image);
			goto normal_way;
		}
		i-> xmem. shmaddr = i-> image-> data = shmat( i-> xmem. shmid, 0, 0);
		if ( i-> xmem. shmaddr == (void*)-1 || i-> xmem. shmaddr == NULL) {
			i-> image-> data = NULL;
			XDestroyImage( i-> image);
			shmctl( i-> xmem. shmid, IPC_RMID, NULL);
			goto normal_way;
		}
		i-> xmem. readOnly = false;
		guts.xshmattach_failed = false;
		XSetErrorHandler(shm_ignore_errors);
		if ( XShmAttach(DISP, &i->xmem) == 0) {
			XCHECKPOINT;
bad_xshm_attach:
			XSetErrorHandler(guts.main_error_handler);
			i-> image-> data = NULL;
			XDestroyImage( i-> image);
			shmdt( i-> xmem. shmaddr);
			shmctl( i-> xmem. shmid, IPC_RMID, NULL);
			goto normal_way;
		}
		XCHECKPOINT;
		XSync(DISP,false);
		XCHECKPOINT;
		if (guts.xshmattach_failed)       goto bad_xshm_attach;
		shmctl( i-> xmem. shmid, IPC_RMID, NULL);
		i-> data_alias = i-> image-> data;
		i-> shm = true;
		return i;
	}
normal_way:
#endif
	i-> bytes_per_line_alias = (( width * idepth + 31) / 32) * 4;
	i-> data_alias = malloc( height * i-> bytes_per_line_alias + extra_bytes);
	if (!i-> data_alias) {
		warn("Not enough memory");
		free(i);
		return NULL;
	}
	i-> image = XCreateImage(
		DISP, visual, depth, pformat,
		0, i-> data_alias,
		width, height, 32, i-> bytes_per_line_alias
	);
	XCHECKPOINT;
	if ( !i-> image) {
		warn("XCreateImage(%d,%d,visual=%x,depth=%d/%d) error", width, height, (int)visual->visualid,depth,idepth);
		free( i-> data_alias);
		free( i);
		return NULL;
	}
	return i;
}

void
prima_XDestroyImage( XImage * i)
{
	if ( i) {
		if ( i-> data) {
			free( i-> data);
			i-> data = NULL;
		}
		((*((i)->f.destroy_image))((i)));
	}
}

Bool
prima_free_ximage( PrimaXImage *i)
{
	if (!i) return true;
#ifdef USE_MITSHM
	if ( i-> shm) {
		XShmDetach( DISP, &i-> xmem);
		i-> image-> data = NULL;
		XDestroyImage( i-> image);
		shmdt( i-> xmem. shmaddr);
		free(i);
		return true;
	}
#endif
	XDestroyImage( i-> image);
	free(i);
	return true;
}

static Bool
destroy_ximage( PrimaXImage *i)
{
	if ( !i) return true;
	if ( i-> ref_cnt > 0) {
		i-> can_free = true;
		return true;
	}
	return prima_free_ximage( i);
}

static Bool
destroy_one_ximage( PrimaXImage *i, int nothing1, void *nothing2, void *nothing3)
{
	prima_free_ximage( i);
	return false;
}

void
prima_gc_ximages( void )
{
	if ( !guts.ximages) return;
	hash_first_that( guts.ximages, (void*)destroy_one_ximage, NULL, NULL, NULL);
}

void
prima_ximage_event( XEvent *eve) /* to be called from apc_event's handle_event */
{
#ifdef USE_MITSHM
	XShmCompletionEvent *ev = (XShmCompletionEvent*)eve;
	PrimaXImage *i;

	if ( eve && eve-> type == guts. shared_image_completion_event) {
		i = hash_fetch( guts.ximages, (void*)&ev->shmseg, sizeof(ev->shmseg));
		if ( i) {
			i-> ref_cnt--;
			if ( i-> ref_cnt <= 0) {
				hash_delete( guts.ximages, (void*)&ev->shmseg, sizeof(ev->shmseg), false);
				if ( i-> can_free)
					prima_free_ximage( i);
			}
		}
	}
#endif
}

#ifdef USE_MITSHM
static int
check_ximage_event( Display * disp, XEvent * ev, XPointer data)
{
	return ev-> type == guts. shared_image_completion_event;
}
#endif

Bool
prima_put_ximage(
	XDrawable win, GC gc, PrimaXImage *i,
	int src_x, int src_y, int dst_x, int dst_y,
	int width, int height
) {
	if ( src_x < 0) {
		width += src_x;
		dst_x -= src_x;
		src_x = 0;
		if ( width <= 0) return false;
	}
#ifdef USE_MITSHM
	if ( i-> shm) {
		XEvent ev;
		if ( src_y + height > i-> image-> height)
			height = i-> image-> height - src_y;
		if ( i-> ref_cnt < 0)
			i-> ref_cnt = 0;
		i-> ref_cnt++;
		if ( i-> ref_cnt == 1)
			hash_store( guts.ximages, &i->xmem.shmseg, sizeof(i->xmem.shmseg), i);
		XShmPutImage( DISP, win, gc, i-> image, src_x, src_y, dst_x, dst_y, width, height, true);
		XFlush(DISP);
		while (XCheckIfEvent( DISP, &ev, check_ximage_event, NULL))
			prima_ximage_event(&ev);
		return true;
	}
#endif
	XPutImage( DISP, win, gc, i-> image, src_x, src_y, dst_x, dst_y, width, height);
	XCHECKPOINT;
	return true;
}


/* image & bitmaps */
Bool
apc_image_create( Handle self)
{
	DEFXX;
	XX-> type.image        = true;
	XX-> type.icon         = !!kind_of(self, CIcon);
	XX-> type.drawable     = true;
	XX-> image_cache. type = CACHE_INVALID;
	XX->size. x            = PImage(self)-> w;
	XX->size. y            = PImage(self)-> h;
	return true;
}

static void
clear_caches( Handle self)
{
	DEFXX;

	prima_palette_free( self, false);
	destroy_ximage( XX-> image_cache. icon);
	destroy_ximage( XX-> image_cache. image);
	XX-> image_cache. icon      = NULL;
	XX-> image_cache. image     = NULL;
}

Bool
apc_image_destroy( Handle self)
{
	clear_caches( self);
	return true;
}

ApiHandle
apc_image_get_handle( Handle self)
{
	return (ApiHandle) X(self)-> gdrawable;
}

Bool
apc_image_begin_paint_info( Handle self)
{
	DEFXX;
	PIcon img = PIcon( self);
	int icon = XX-> type. icon;
	Bool bitmap = (img-> type == imBW) || ( guts. idepth == 1);
	Bool layered = icon && img-> maskType == imbpp8 && guts. argb_visual. visual;
	int depth = layered ? guts. argb_depth : ( bitmap ? 1 : guts. depth );

	if ( !DISP) return false;
	XX-> gdrawable = XCreatePixmap( DISP, guts. root, 1, 1, depth);
	XCHECKPOINT;
	prima_prepare_drawable_for_painting( self, false);
	XX-> size. x = 1;
	XX-> size. y = 1;
	return true;
}

Bool
apc_image_end_paint_info( Handle self)
{
	DEFXX;
	prima_cleanup_drawable_after_painting( self);
	if ( XX-> gdrawable) {
		XFreePixmap( DISP, XX-> gdrawable);
		XCHECKPOINT;
		XX-> gdrawable = 0;
	}
	XX-> size. x = PImage( self)-> w;
	XX-> size. y = PImage( self)-> h;
	return true;
}

Bool
apc_image_update_change( Handle self)
{
	DEFXX;
	PImage img = PImage( self);

	clear_caches( self);

	XX-> size. x = img-> w;
	XX-> size. y = img-> h;
	if ( guts. depth > 1)
		XX-> type.pixmap = (img-> type == imBW) ? 0 : 1;
	else
		XX-> type.pixmap = 0;
	XX-> type.bitmap = !!XX-> type.pixmap;
	if ( XX-> cached_region) {
		XDestroyRegion( XX-> cached_region);
		XX-> cached_region = NULL;
	}
	return true;
}

Bool
apc_dbm_create( Handle self, int type)
{
	int depth;
	DEFXX;

	if ( !DISP) return false;
	if ( guts. idepth == 1) type = dbtBitmap;

	XX-> colormap = guts. defaultColormap;
	XX-> visual   = &guts. visual;

	switch (type) {
	case dbtBitmap:
		XX-> type.bitmap = 1;
		depth = 1;
		break;
	case dbtLayered:
		if ( guts. render_supports_argb32 ) {
			XX-> flags.layered = 1;
			depth = guts. argb_depth;
			XX-> colormap = guts. argbColormap;
			XX-> visual   = &guts. argb_visual;
			break;
		}
	case dbtPixmap:
		XX-> type.pixmap = 1;
		depth = guts.depth;
		break;
	default:
		return false;
	}
	XX-> type.dbm = true;
	XX-> type.drawable = true;
	XX->size. x          = ((PDeviceBitmap)(self))-> w;
	XX->size. y          = ((PDeviceBitmap)(self))-> h;
	if ( XX-> size.x == 0) XX-> size.x = 1;
	if ( XX-> size.y == 0) XX-> size.y = 1;
	XX->gdrawable        = XCreatePixmap( DISP, guts. root, XX->size. x, XX->size. y, depth);
	if (XX-> gdrawable == None) return false;
	XCHECKPOINT;
	prima_prepare_drawable_for_painting( self, false);

	CREATE_ARGB_PICTURE(XX->gdrawable,
		XX->type.bitmap ? 1 : (XF_LAYERED(XX) ? 32 : 0),
		XX->argb_picture);

	return true;
}

Bool
apc_dbm_destroy( Handle self)
{
	DEFXX;
	if ( XX->gdrawable) {
		prima_cleanup_drawable_after_painting( self);
		XFreePixmap( DISP, XX->gdrawable);
		XX-> gdrawable = None;
	}
	return true;
}

ApiHandle
apc_dbm_get_handle( Handle self)
{
	return (ApiHandle) X(self)-> gdrawable;
}

Byte*
prima_mirror_bits( void)
{
	static Bool initialized = false;
	static Byte bits[256];
	unsigned int i, j;
	int k;

	if (!initialized) {
		for ( i = 0; i < 256; i++) {
			bits[i] = 0;
			j = i;
			for ( k = 0; k < 8; k++) {
				bits[i] <<= 1;
				if ( j & 0x1)
					bits[i] |= 1;
				j >>= 1;
			}
		}
		initialized = true;
	}

	return bits;
}

static void
copy_xybitmap( unsigned char *data, const unsigned char *idata, int w, int h, int ls, int ils, int els)
{
	int y;
	register int x;
	Byte *mirrored_bits;

	if ( els > ils ) els = ils;

	/* XXX: MSB/LSB */
	if ( guts.bit_order == MSBFirst) {
		for ( y = h-1; y >= 0; y--) {
			memcpy( ls*(h-y-1)+data, idata+y*ils, els);
		}
	} else {
		mirrored_bits = prima_mirror_bits();
		for ( y = h-1; y >= 0; y--) {
			register const unsigned char *s = idata+y*ils;
			register unsigned char *t = ls*(h-y-1)+data;
			x = els;
			while (x--)
				*t++ = mirrored_bits[*s++];
		}
	}
}

void
prima_copy_1bit_ximage( unsigned char *data, XImage *i, Bool to_ximage)
{
	if ( to_ximage )
		copy_xybitmap(
			(unsigned char*) i->data, (const unsigned char*) data,
			i->width, i->height,
			i->bytes_per_line, LINE_SIZE(i->width,1), i->bytes_per_line
		);
	else
		copy_xybitmap(
			data, (const unsigned char*) i->data,
			i->width, i->height,
			LINE_SIZE(i->width,1), i->bytes_per_line, EFFECTIVE_LINE_SIZE(i->width,1)
		);
}

void
prima_mirror_bytes( unsigned char *data, int dataSize)
{
	Byte *mirrored_bits = prima_mirror_bits();
	while ( dataSize--) {
		*data = mirrored_bits[*data];
		data++;
	}
}

static Bool
create_cache1_1( Image *img, ImageCache *cache, Bool for_icon)
{
	PrimaXImage *ximage;
	if ( !( ximage = prima_prepare_ximage( img->w, img->h, CACHE_BITMAP)))
		return false;
	prima_copy_1bit_ximage( for_icon ? PIcon(img)->mask : img->data, ximage->image, true);
	if ( for_icon)
		cache-> icon  = ximage;
	else
		cache-> image = ximage;
	return true;
}

static void
create_rgb_to_8_lut( int ncolors, const PRGBColor pal, Pixel8 *lut)
{
	int i;
	for ( i = 0; i < ncolors; i++)
		lut[i] = PALETTE2DEV_RGB( &guts.screen_bits, pal[i]);
}

static void
create_rgb_to_16_lut( int ncolors, const PRGBColor pal, Pixel16 *lut)
{
	int i;
	for ( i = 0; i < ncolors; i++)
		lut[i] = PALETTE2DEV_RGB( &guts.screen_bits, pal[i]);
	if ( guts.machine_byte_order != guts.byte_order)
		for ( i = 0; i < ncolors; i++)
			lut[i] = REVERSE_BYTES_16(lut[i]);
}

static int *
rank_rgb_shifts( void)
{
	static int shift[3];
	static Bool shift_unknown = true;

	if ( shift_unknown) {
		int xchg;
		shift[0] = guts. screen_bits. red_shift;
		shift[1] = guts. screen_bits. green_shift;
		if ( shift[1] < shift[0]) {
			xchg = shift[0];
			shift[0] = shift[1];
			shift[1] = xchg;
		}
		shift[2] = guts. screen_bits. blue_shift;
		if ( shift[2] < shift[0]) {
			xchg = shift[2];
			shift[2] = shift[1];
			shift[1] = shift[0];
			shift[0] = xchg;
		} else if ( shift[2] < shift[1]) {
			xchg = shift[1];
			shift[1] = shift[2];
			shift[2] = xchg;
		}

		shift_unknown = false;
	}

	return shift;
}

static void
create_rgb_to_xpixel_lut( int ncolors, const PRGBColor pal, XPixel *lut)
{
	int i;
	for ( i = 0; i < ncolors; i++)
		lut[i] = PALETTE2DEV_RGB( &guts.screen_bits, pal[i]);
	if ( guts.machine_byte_order != guts.byte_order)
		for ( i = 0; i < ncolors; i++)
			lut[i] = REVERSE_BYTES_32(lut[i]);
}

static Bool
create_cache4_8( Image *img, ImageCache *cache)
{
	static Bool init = false;
	static unsigned char lut1[ NPalEntries8];
	static unsigned char lut2[ NPalEntries8];
	unsigned char *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w, ww = (w >> 1) + (w & 1);
	unsigned i;

	if ( !init) {
		init = true;
		for ( i = 0; i < NPalEntries8; i++) {
			lut1[i] = ((i & MSNibble) >> MSNibbleShift);
			lut2[i] = ((i & LSNibble) >> LSNibbleShift);
		}
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);
	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register unsigned char *d = (unsigned char*)(ls*(h-y-1)+data);
		for ( x = 0; x < ww; x++) {
			*d++ = lut1[line[x]];
			*d++ = lut2[line[x]];
		}
	}
	return true;
}


static Bool
create_cache4_16( Image *img, ImageCache *cache)
{
	Duplet16 lut[ NPalEntries8];
	Pixel16 lut1[ NPalEntries4];
	unsigned char *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;
	unsigned i;

	create_rgb_to_16_lut( NPalEntries4, img-> palette, lut1);
	for ( i = 0; i < NPalEntries8; i++) {
		lut[i]. a = lut1[(i & MSNibble) >> MSNibbleShift];
		lut[i]. b = lut1[(i & LSNibble) >> LSNibbleShift];
	}
	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);
	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Duplet16 *d = (Duplet16*)(ls*(h-y-1)+data);
		for ( x = 0; x < (w+1)/2; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache4_24( Image *img, ImageCache *cache)
{
	Duplet24 lut[ NPalEntries8];
	XPixel lut1[ NPalEntries4];
	unsigned char *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;
	unsigned i;
	int *shift = rank_rgb_shifts();

	create_rgb_to_xpixel_lut( NPalEntries4, img-> palette, lut1);
	for ( i = 0; i < NPalEntries8; i++) {
		lut[i]. a0 = (ColorComponent)((lut1[(i & MSNibble) >> MSNibbleShift] >> shift[0]) & ColorComponentMask);
		lut[i]. a1 = (ColorComponent)((lut1[(i & MSNibble) >> MSNibbleShift] >> shift[1]) & ColorComponentMask);
		lut[i]. a2 = (ColorComponent)((lut1[(i & MSNibble) >> MSNibbleShift] >> shift[2]) & ColorComponentMask);
		lut[i]. b0 = (ColorComponent)((lut1[(i & LSNibble) >> LSNibbleShift] >> shift[0]) & ColorComponentMask);
		lut[i]. b1 = (ColorComponent)((lut1[(i & LSNibble) >> LSNibbleShift] >> shift[1]) & ColorComponentMask);
		lut[i]. b2 = (ColorComponent)((lut1[(i & LSNibble) >> LSNibbleShift] >> shift[2]) & ColorComponentMask);
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Duplet24 *d = (Duplet24 *)(ls*(h-y-1)+data);
		for ( x = 0; x < (w+1)/2; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache4_32( Image *img, ImageCache *cache)
{
	Duplet32 lut[ NPalEntries8];
	XPixel lut1[ NPalEntries4];
	unsigned char *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;
	unsigned i;

	create_rgb_to_xpixel_lut( NPalEntries4, img-> palette, lut1);
	for ( i = 0; i < NPalEntries8; i++) {
		lut[i]. a = lut1[(i & MSNibble) >> MSNibbleShift];
		lut[i]. b = lut1[(i & LSNibble) >> LSNibbleShift];
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Duplet32 *d = (Duplet32 *)(ls*(h-y-1)+data);
		for ( x = 0; x < (w+1)/2; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache_equal( Image *img, ImageCache *cache)
{
	unsigned char *data;
	int y, ls, lls, h = img-> h;
	cache->image = prima_prepare_ximage( img-> w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);
	lls = (ls > img-> lineSize) ? img-> lineSize : ls;

	for ( y = h-1; y >= 0; y--)
		memcpy( data + ls * (h - y - 1), img-> data + y*img-> lineSize,  lls);
	return true;
}

static Bool
create_cache8_8_tc( Image *img, ImageCache *cache)
{
	Pixel8 lut[ NPalEntries8];
	Pixel8 *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;

	create_rgb_to_8_lut( img-> palSize, img-> palette, lut);

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Pixel8 *d = (Pixel8*)(ls*(h-y-1)+(unsigned char *)data);
		for ( x = 0; x < w; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache8_16( Image *img, ImageCache *cache)
{
	Pixel16 lut[ NPalEntries8];
	Pixel16 *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;

	create_rgb_to_16_lut( img-> palSize, img-> palette, lut);

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Pixel16 *d = (Pixel16*)(ls*(h-y-1)+(unsigned char *)data);
		for ( x = 0; x < w; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache8_24( Image *img, ImageCache *cache)
{
	Pixel24 lut[ NPalEntries8];
	XPixel lut1[ NPalEntries8];
	Pixel24 *data;
	int i;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;
	int *shift = rank_rgb_shifts();

	create_rgb_to_xpixel_lut( img-> palSize, img-> palette, lut1);
	for ( i = 0; i < NPalEntries8; i++) {
		lut[i]. a0 = (ColorComponent)((lut1[i] >> shift[0]) & ColorComponentMask);
		lut[i]. a1 = (ColorComponent)((lut1[i] >> shift[1]) & ColorComponentMask);
		lut[i]. a2 = (ColorComponent)((lut1[i] >> shift[2]) & ColorComponentMask);
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Pixel24 *d = (Pixel24*)(ls*(h-y-1)+(unsigned char *)data);
		for ( x = 0; x < w; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache8_32( Image *img, ImageCache *cache)
{
	XPixel lut[ NPalEntries8];
	Pixel32 *data;
	int x, y;
	int ls;
	int h = img-> h, w = img-> w;

	create_rgb_to_xpixel_lut( img-> palSize, img-> palette, lut);

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register unsigned char *line = img-> data + y*img-> lineSize;
		register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
		for ( x = 0; x < w; x++) {
			*d++ = lut[line[x]];
		}
	}
	return true;
}

static Bool
create_cache24_16( Image *img, ImageCache *cache)
{
	static Pixel16 lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8];
	static Bool initialize = true;
	U16 *data;
	int x, y;
	int i;
	RGBColor pal[NPalEntries8];
	int ls;
	int h = img-> h, w = img-> w;

	if ( initialize) {
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = i; pal[i]. g = 0; pal[i]. b = 0;
		}
		create_rgb_to_16_lut( NPalEntries8, pal, lur);
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = 0; pal[i]. g = i; pal[i]. b = 0;
		}
		create_rgb_to_16_lut( NPalEntries8, pal, lug);
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = 0; pal[i]. g = 0; pal[i]. b = i;
		}
		create_rgb_to_16_lut( NPalEntries8, pal, lub);
		initialize = false;
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register Pixel24 *line = (Pixel24*)(img-> data + y*img-> lineSize);
		register Pixel16 *d = (Pixel16*)(ls*(h-y-1)+(unsigned char *)data);
		for ( x = 0; x < w; x++) {
			*d++ = lub[line->a0] | lug[line->a1] | lur[line->a2];
			line++;
		}
	}
	return true;
}

static Bool
create_cache24_32( Image *img, ImageCache *cache)
{
	static XPixel lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8];
	static Bool initialize = true;
	RGBColor pal[NPalEntries8];
	Pixel32 *data;
	int x, y;
	int i, ls;
	int h = img-> h, w = img-> w;

	if ( initialize) {
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = i;
			pal[i]. g = 0;
			pal[i]. b = 0;
		}
		create_rgb_to_xpixel_lut( NPalEntries8, pal, lur);
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = 0;
			pal[i]. g = i;
			pal[i]. b = 0;
		}
		create_rgb_to_xpixel_lut( NPalEntries8, pal, lug);
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = 0;
			pal[i]. g = 0;
			pal[i]. b = i;
		}
		create_rgb_to_xpixel_lut( NPalEntries8, pal, lub);
		initialize = false;
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_PIXMAP);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	for ( y = h-1; y >= 0; y--) {
		register Pixel24 *line = (Pixel24*)(img-> data + y*img-> lineSize);
		register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
		for ( x = 0; x < w; x++) {
			*d++ = lub[line->a0] | lug[line->a1] | lur[line->a2];
			line++;
		}
	}
	return true;
}

static Bool
create_cache1( Image* img, ImageCache *cache, int bpp)
{
	return create_cache1_1( img, cache, false);
}

static Bool
create_cache4( Image* img, ImageCache *cache, int bpp)
{
	switch (bpp) {
	case  8:     return create_cache4_8( img, cache);
	case 16:     return create_cache4_16( img, cache);
	case 24:     return create_cache4_24( img, cache);
	case 32:     return create_cache4_32( img, cache);
	default:     warn( "UAI_011: unsupported image conversion: %d => %d", 4, bpp);
	}
	return false;
}

static Bool
create_cache8( Image* img, ImageCache *cache, int bpp)
{
	switch (bpp) {
	case 8:
		return ( guts. visualClass == TrueColor || guts. visualClass == DirectColor) ?
			create_cache8_8_tc( img, cache) :
			create_cache_equal( img, cache);
	case 16: return create_cache8_16( img, cache);
	case 24: return create_cache8_24( img, cache);
	case 32: return create_cache8_32( img, cache);
	default: warn( "UAI_012: unsupported image conversion: %d => %d", 8, bpp);
	}
	return false;
}

static Bool
create_cache24( Image* img, ImageCache *cache, int bpp)
{
	switch (bpp) {
	case 16: return create_cache24_16( img, cache); break;
	case 32: return create_cache24_32( img, cache); break;
	default: warn( "UAI_013: unsupported image conversion: %d => %d", 24, bpp);
	}
	return false;
}

static void
cache_remap_8( Image*img, ImageCache* cache)
{
	int sz = img-> h * cache-> image-> bytes_per_line_alias;
	Byte * p = cache-> image-> data_alias;
	while ( sz--) {
		*p = guts. mappingPlace[ *p];
		p++;
	}
}

static void
cache_remap_4( Image*img, ImageCache* cache)
{
	int sz = img-> h * cache-> image-> bytes_per_line_alias;
	Byte * p = cache-> image-> data_alias;
	while ( sz--) {
		*p =
			guts. mappingPlace[(*p) & 0xf] |
		(guts. mappingPlace[((*p) & 0xf0) >> 4] << 4);
		p++;
	}
}

static void
cache_remap_1( Image*img, ImageCache* cache)
{
	int sz = img-> h * cache-> image-> bytes_per_line_alias;
	Byte * p = cache-> image-> data_alias;
	if ( guts. mappingPlace[0] == guts. mappingPlace[1])
		memset( p, (guts. mappingPlace[0] == 0) ? 0 : 0xff, sz);
	else if ( guts. mappingPlace[0] != 0)
		while ( sz--) {
			*p = ~(*p);
			p++;
		}
}

static void
create_rgb_to_argb_xpixel_lut( int ncolors, const PRGBColor pal, XPixel *lut)
{
	int i;
	for ( i = 0; i < ncolors; i++)
		lut[i] = PALETTE2DEV_RGB( &guts.argb_bits, pal[i]);
	if ( guts.machine_byte_order != guts.byte_order)
		for ( i = 0; i < ncolors; i++)
			lut[i] = REVERSE_BYTES_32(lut[i]);
}

static void
create_rgb_to_alpha_xpixel_lut( int ncolors, const Byte * alpha, XPixel *lut)
{
	int i;
	for ( i = 0; i < ncolors; i++)
		lut[i] = ((alpha[i] << guts. argb_bits. alpha_range) >> 8) << guts. argb_bits. alpha_shift;
	if ( guts.machine_byte_order != guts.byte_order)
		for ( i = 0; i < ncolors; i++)
			lut[i] = REVERSE_BYTES_32(lut[i]);
}

static Bool
create_argb_cache(PIcon img, ImageCache * cache, int type, int alpha_channel)
{
	static XPixel lur[NPalEntries8], lub[NPalEntries8], lug[NPalEntries8], lua[NPalEntries8];
	static Bool initialize = true;
	RGBColor pal[NPalEntries8];
	Byte alpha[NPalEntries8];
	Pixel32 *data;
	int x, y;
	int i, ls;
	int h = img-> h, w = img-> w;

	if ( initialize) {
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = i;
			pal[i]. g = 0;
			pal[i]. b = 0;
		}
		create_rgb_to_argb_xpixel_lut( NPalEntries8, pal, lur);
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = 0;
			pal[i]. g = i;
			pal[i]. b = 0;
		}
		create_rgb_to_argb_xpixel_lut( NPalEntries8, pal, lug);
		for ( i = 0; i < NPalEntries8; i++) {
			pal[i]. r = 0;
			pal[i]. g = 0;
			pal[i]. b = i;
		}
		create_rgb_to_argb_xpixel_lut( NPalEntries8, pal, lub);
		for ( i = 0; i < NPalEntries8; i++)
			alpha[i] = i;
		create_rgb_to_alpha_xpixel_lut( NPalEntries8, alpha, lua);
		initialize = false;
	}

	cache->image = prima_prepare_ximage( w, h, CACHE_LAYERED);
	if ( !cache->image) return false;
	ls = get_ximage_bytes_per_line( cache->image);
	data = get_ximage_data( cache->image);

	switch (type) {
	case CACHE_LAYERED_ALPHA:
		for ( y = h-1; y >= 0; y--) {
			register Pixel24 *line = (Pixel24*)(img-> data + y*img-> lineSize);
			register Pixel8  *mask = (Pixel8 *)(img-> mask + y*img-> maskLine);
			register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
			for ( x = 0; x < w; x++) {
				*(d++) = lub[line->a0] | lug[line->a1] | lur[line->a2] | lua[*(mask++)];
				line++;
			}
		}
		break;
	case CACHE_LAYERED: {
		XPixel alpha = lua[alpha_channel];
		for ( y = h-1; y >= 0; y--) {
			register Pixel24 *line = (Pixel24*)(img-> data + y*img-> lineSize);
			register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
			for ( x = 0; x < w; x++) {
				*d++ = lub[line->a0] | lug[line->a1] | lur[line->a2] | alpha;
				line++;
			}
		}}
		break;
	case CACHE_A8:
		for ( y = h-1; y >= 0; y--) {
			register Pixel8  *line = (Pixel8*)(img-> data + y*img-> lineSize);
			register Pixel32 *d = (Pixel32*)(ls*(h-y-1)+(unsigned char *)data);
			for ( x = 0; x < w; x++)
				*(d++) = lua[*(line++)];
		}
		break;
	default:
		croak("bad call to create_argb_cache");
	}
	return true;
}

static ImageCache*
create_image_cache( PImage img, int type, int alpha_mul, int alpha_channel)
{
	PDrawableSysData IMG = X((Handle)img);
	int target_bpp;
	ImageCache *cache    = &X((Handle)img)-> image_cache;
	Bool ret;
	Handle dup = NULL_HANDLE;
	PImage pass = img;

	/* common validity checks */
	if ( img-> w == 0 || img-> h == 0) return NULL;
	if ( img-> palette == NULL) {
		warn( "UAI_014: image has no palette");
		return NULL;
	}

	/* test if types are applicable */
	switch ( type) {
	case CACHE_A8:
	case CACHE_LAYERED:
	case CACHE_LAYERED_ALPHA:
		if ( !guts. render_supports_argb32 ) {
			warn("panic: no argb support");
			return NULL;
		}
		break;
	case CACHE_PIXMAP:
		if ( guts. idepth == 1) type = CACHE_BITMAP;
		break;
	case CACHE_LOW_RES:
		if ( !guts. dynamicColors) type = CACHE_PIXMAP;
		if ( guts. idepth == 1) type = CACHE_BITMAP;
		break;
	}

	/* find Prima image depth */
	switch (type) {
	case CACHE_BITMAP:
		target_bpp = 1;
		break;
	case CACHE_A8:
	case CACHE_LAYERED:
	case CACHE_LAYERED_ALPHA:
		target_bpp = guts. argb_depth;
		break;
	default:
		target_bpp = guts. idepth;
	}

	/* create icon cache, if any */
	if ( XT_IS_ICON(IMG) && type != CACHE_LAYERED_ALPHA) {
		if ( cache-> icon == NULL) {
			if ( PIcon(img)->maskType == imbpp8) {
				if ( !dup) {
					if (!(dup = img-> self-> dup(( Handle) img)))
						return NULL;
				}
				CIcon(dup)->set_maskType(dup, imbpp1);
				pass = ( PImage) dup;
			}

			if ( !create_cache1_1(pass, cache, true))
				return NULL;
		}
	} else
		cache-> icon = NULL;

	if ( cache-> image != NULL) {
		if ( type != CACHE_LAYERED )
			alpha_channel = 0;
		if (
			cache-> type == type &&
			cache->alpha_mul == alpha_mul &&
			cache->alpha_channel == alpha_channel
		) return cache;
		destroy_ximage( cache-> image);
		cache-> image = NULL;
	}


	/* convert from funky image types */
	if (( img-> type & ( imRealNumber | imComplexNumber | imTrigComplexNumber)) ||
		( img-> type == imLong || img-> type == imShort)) {
		if ( !dup) {
			if (!(dup = img-> self-> dup(( Handle) img)))
				return NULL;
		}
		pass = ( PImage) dup;
		pass-> self->resample(( Handle) pass,
			pass-> self->stats(( Handle) pass, false, isRangeLo, 0),
			pass-> self->stats(( Handle) pass, false, isRangeHi, 0),
			0, 255
		);
		pass-> self-> set_type(( Handle) pass, imByte);
	}

	/* treat ARGB separately, and leave */
	if ( type == CACHE_LAYERED || type == CACHE_LAYERED_ALPHA ) {
		Bool ok;
		PIcon i = (PIcon) pass;

		/* premultiply */
		if ( alpha_mul != 255 ) {
			if ( !dup)
				if (!(dup = img-> self-> dup(( Handle) img)))
					return NULL;
		}
		if ( i->type != imRGB ) {
			if ( !dup)
				if (!(dup = img-> self-> dup(( Handle) i)))
					return NULL;
			i = (PIcon) dup;
			i-> self-> set_type(dup, imRGB);
		}

		if ( XT_IS_ICON(IMG) && type == CACHE_LAYERED_ALPHA && i->maskType != imbpp8 ) {
			if ( !dup)
				if (!(dup = i-> self-> dup((Handle) i)))
					return NULL;
			i = (PIcon) dup;
			i-> self-> set_maskType(dup, imbpp8);
		}

		if ( alpha_mul != 255 ) {
			img_premultiply_alpha_constant( dup, alpha_mul);
			if ( XT_IS_ICON(IMG)) {
				Image dummy;
				img_fill_dummy( &dummy, img->w, img->h, imByte, PIcon(dup)->mask, std256gray_palette);
				img_premultiply_alpha_constant( (Handle) &dummy, alpha_mul);
			}
		}
		ok = create_argb_cache(i, cache,
			(XT_IS_ICON(IMG) && type == CACHE_LAYERED_ALPHA) ? CACHE_LAYERED_ALPHA : CACHE_LAYERED,
			alpha_channel
		);
		if ( dup) Object_destroy(dup);
		if ( !ok ) return NULL;

		cache-> type = type;
		cache-> alpha_mul = alpha_mul;
		cache-> alpha_channel = alpha_channel;
		return cache;
	}

	cache->alpha_mul = alpha_mul;
	cache->alpha_channel = alpha_channel;
	if ( type == CACHE_A8 ) {
		Bool ok;
		PImage i = (PImage) pass;
		if ( i->type != imByte ) {
			if ( !dup)
				if (!(dup = img-> self-> dup(( Handle) i)))
					return NULL;
			i = (PImage) dup;
			i-> self-> set_type(dup, imByte);
		}
		ok = create_argb_cache((PIcon) i, cache, CACHE_A8, 0);
		if ( dup) Object_destroy(dup);
		if ( !ok ) return NULL;

		cache-> type = type;
		return cache;
	}

	/*
		apply as much of system palette colors as possible to new image,
		if we're working on 1-8 bit displays. CACHE_LOW_RES on displays with
		dynamic colors goes only after conservative strategy, using only
		immutable colors to be copied to clipboard, icon, etc.
	*/
	if ( target_bpp <= 8 && img-> type != imBW) {
		int bpp, colors = 0;
		RGBColor palbuf[256], *palptr = NULL;
		if ( !dup) {
			if (!(dup = img-> self-> dup(( Handle) img)))
				return NULL;
		}
		pass = ( PImage) dup;
		if ( target_bpp <= 1) bpp = imbpp1; else
		if ( target_bpp <= 4) bpp = imbpp4; else bpp = imbpp8;

		if ( guts. palSize > 0 && target_bpp > 1) {
			int i, maxRank = RANK_FREE;
			if ( type == CACHE_LOW_RES) maxRank = RANK_LOCKED;
			for ( i = 0; i < guts. palSize; i++) {
				if ( guts. palette[i]. rank <= maxRank) continue;
				palbuf[colors]. r = guts. palette[i]. r;
				palbuf[colors]. g = guts. palette[i]. g;
				palbuf[colors]. b = guts. palette[i]. b;
				colors++;
				if ( colors > 255) break;
			}
			palptr = palbuf;
		}
		pass-> self-> reset( dup, bpp, palptr, colors);
	}

	/* convert image bits */
	switch ( pass-> type & imBPP) {
	case 1:   ret = create_cache1( pass, cache, target_bpp); break;
	case 4:   ret = create_cache4( pass, cache, target_bpp); break;
	case 8:   ret = create_cache8( pass, cache, target_bpp); break;
	case 24:  ret = create_cache24(pass, cache, target_bpp); break;
	default:
		warn( "UAI_015: unsupported image type");
		return NULL;
	}
	if ( !ret) {
		if ( dup) Object_destroy(dup);
		return NULL;
	}

	/* on paletted displays, acquire actual color indexes, and
		remap pixels to match them */
	if (( guts. palSize > 0) && (( pass-> type & imBPP) != 24)) {
		int i, maxRank = RANK_FREE;
		Byte * p = X((Handle)img)-> palette;

		if ( type == CACHE_LOW_RES) /* don't use dynamic colors */
			maxRank = RANK_LOCKED;

		for ( i = 0; i < pass-> palSize; i++) {
			int j = guts. mappingPlace[i] = prima_color_find( NULL_HANDLE,
				RGB_COMPOSITE(
				pass-> palette[i].r,
				pass-> palette[i].g,
				pass-> palette[i].b
				), -1, NULL, maxRank);

			if ( p && ( prima_lpal_get( p, j) == RANK_FREE))
				prima_color_add_ref(( Handle) img, j, RANK_LOCKED);
		}
		for ( i = pass-> palSize; i < 256; i++) guts. mappingPlace[i] = 0;

		switch(target_bpp){
		case 8: if ((pass-> type & imBPP) != 1) cache_remap_8( img, cache); break;
		case 4: if ((pass-> type & imBPP) != 1) cache_remap_4( img, cache); break;
		case 1: cache_remap_1( img, cache); break;
		default: warn("UAI_019: palette is not supported");
		}
	} else if ( target_bpp == 1 ) {
		RGBColor * p = pass->palette;
		guts. mappingPlace[0] = (p[0].r + p[0].g + p[0].b > 382) ? 0xff : 0;
		guts. mappingPlace[1] = (p[1].r + p[1].g + p[1].b > 382) ? 0xff : 0;
		cache_remap_1( img, cache);
	}

	if ( dup) Object_destroy(dup);
	cache-> type = type;
	return cache;
}

ImageCache*
prima_image_cache( PImage img, int type, int alpha_mul, int alpha_channel)
{
	ImageCache *cache = &X((Handle)img)-> image_cache;

	if ( cache->type != CACHE_LAYERED )
		alpha_channel = 0;

	if (
		cache-> image         != NULL          &&
		cache-> type          == type          &&
		cache-> alpha_mul     == alpha_mul     &&
		cache-> alpha_channel == alpha_channel
	)
		return cache;

	return create_image_cache(img, type, alpha_mul, alpha_channel);
}

Bool
prima_create_icon_pixmaps( Handle self, Pixmap *xor, Pixmap *and)
{
	Pixmap p1, p2;
	PIcon icon = PIcon(self);
	ImageCache *cache;
	GC gc;
	XGCValues gcv;

	cache = prima_image_cache((PImage)icon, CACHE_BITMAP, 255, 0);
	if ( !cache) return false;
	p1 = XCreatePixmap( DISP, guts. root, icon-> w, icon-> h, 1);
	p2 = XCreatePixmap( DISP, guts. root, icon-> w, icon-> h, 1);
	XCHECKPOINT;
	if ( p1 == None || p2 == None) {
		if (p1 != None) XFreePixmap( DISP, p1);
		if (p2 != None) XFreePixmap( DISP, p2);
		return false;
	}
	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, p1, GCGraphicsExposures, &gcv);
	XSetForeground( DISP, gc, 0);
	XSetBackground( DISP, gc, 1);
	prima_put_ximage( p2, gc, cache->icon, 0, 0, 0, 0, icon-> w, icon-> h);
	XSetForeground( DISP, gc, 1);
	XSetBackground( DISP, gc, 0);
	prima_put_ximage( p1, gc, cache->image, 0, 0, 0, 0, icon-> w, icon-> h);
	XFreeGC( DISP, gc);
	*xor = p1;
	*and = p2;
	return true;
}

typedef struct {
	int src_x;
	int src_y;
	int w;
	int h;
	int dst_x;
	int dst_y;
	int rop;
	int old_rop;

#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	int dst_w;
	int dst_h;
	int ofs_x;
	int ofs_y;
	XTransform *transform;
	Bool scaling_only;
#endif
} PutImageRequest;

#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
static XTransform render_identity_transform = { .matrix = {
	{XDoubleToFixed(1.0), XDoubleToFixed(0.0), XDoubleToFixed(0.0)},
	{XDoubleToFixed(0.0), XDoubleToFixed(1.0), XDoubleToFixed(0.0)},
	{XDoubleToFixed(0.0), XDoubleToFixed(0.0), XDoubleToFixed(1.0)}
}};
#endif

/* 1-bit images with palettes need their colors mapped to the target drawable */
void
query_1bit_colors(Handle self, Handle image, unsigned long *fore, unsigned long *back)
{
	PImage img = (PImage) image;

	if ( XT_IS_WIDGET(X(self))) {
		if ( guts. palSize > 0) {
			*fore = prima_color_find( self,
				RGB_COMPOSITE( img-> palette[1].r, img-> palette[1].g, img-> palette[1].b),
				-1, NULL, RANK_NORMAL);
			*back = prima_color_find( self,
				RGB_COMPOSITE( img-> palette[0].r, img-> palette[0].g, img-> palette[0].b),
				-1, NULL, RANK_NORMAL);
		} else {
			*fore = PALETTE2DEV_RGB( &guts.screen_bits, img->palette[1]);
			*back = PALETTE2DEV_RGB( &guts.screen_bits, img->palette[0]);
		}
	} else {
		RGBColor * p = img->palette;
		*fore = prima_allocate_color( self, ARGB(p[1].r, p[1].g, p[1].b), NULL);
		*back = prima_allocate_color( self, ARGB(p[0].r, p[0].g, p[0].b), NULL);
	}
}

static void
rop_apply_colors(Handle self, PutImageRequest * req)
{
	DEFXX;
	/*
	Special case with current foreground and background colors for 1-bit bitmaps/pixmaps, see also
	L<pod/Prima/Drawable.pod | Monochrome bitmaps>.

	Raster ops can be identified by a fingerprint.  For example, Or's is 14
	and Noop's is 10:

        0 | 0 =    0                      0 | 0 =    0
        0 | 1 =   1                       0 | 1 =   1
        1 | 0 =  1                        1 | 0 =  0
        1 | 1 = 1                         1 | 1 = 1
        ---     ----                      ---     ----
                1110 = 14                         1010 = 10

	when this special case uses not actual 0s and 1s, but bit values of
	foreground and background color instead, the resulting operation can
	still be expressed in rops, but these needs to be adjusted. Let's
	consider a case where both colors are 0, and rop = OrPut:

        0 | 0 =    0
        0 | 1 =   1
        0 | 0 =  0
        0 | 1 = 1
        ---     ----
                1010 = 10

	this means that in these conditions, Or (as well as Xor and AndInverted) becomes Noop.

	*/
	unsigned long fore = XX-> fore.primary & 1;
	unsigned long back = XX-> back.primary & 1;
	if ( fore == 0 && back == 0 ) {
		switch( req->rop) {
			case GXand:
			case GXandReverse:
			case GXclear:
			case GXcopy:          req->rop = GXclear;         break;
			case GXequiv:
			case GXinvert:
			case GXnor:
			case GXorReverse:     req->rop = GXinvert;        break;
			case GXandInverted:
			case GXnoop:
			case GXor:
			case GXxor:           req->rop = GXnoop;          break;
			case GXnand:
			case GXcopyInverted:
			case GXorInverted:
			case GXset:           req->rop = GXset;           break;
		}
	} else if ( fore == 1 && back == 0 ) {
		switch( req->rop) {
			case GXand:           req->rop = GXandInverted;   break;
			case GXandInverted:   req->rop = GXand;           break;
			case GXandReverse:    req->rop = GXnor;           break;
			case GXclear:         req->rop = GXclear;         break;
			case GXcopy:          req->rop = GXcopyInverted;  break;
			case GXcopyInverted:  req->rop = GXcopy;          break;
			case GXequiv:         req->rop = GXxor;           break;
			case GXinvert:        req->rop = GXinvert;        break;
			case GXnand:          req->rop = GXorReverse;     break;
			case GXnoop:          req->rop = GXnoop;          break;
			case GXnor:           req->rop = GXandReverse;    break;
			case GXor:            req->rop = GXorInverted;    break;
			case GXorInverted:    req->rop = GXor;            break;
			case GXorReverse:     req->rop = GXnand;          break;
			case GXset:           req->rop = GXset;           break;
			case GXxor:           req->rop = GXequiv;         break;
		}
	} else if ( fore == 1 && back == 1 ) {
		switch( req->rop) {
			case GXand:
			case GXorInverted:
			case GXequiv:
			case GXnoop:          req->rop = GXnoop;          break;
			case GXandInverted:
			case GXclear:
			case GXcopyInverted:
			case GXnor:           req->rop = GXclear;         break;
			case GXinvert:
			case GXnand:
			case GXandReverse:
			case GXxor:           req->rop = GXinvert;        break;
			case GXor:
			case GXorReverse:
			case GXset:
			case GXcopy:          req->rop = GXset;           break;
		}
	}
}

#define SET_ROP(x) if ( req->old_rop != x) XSetFunction( DISP, XX-> gc, req->old_rop = x)
typedef Bool PutImageFunc( Handle self, Handle image, PutImageRequest * req);

static Bool
img_put_copy_area( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	PDrawableSysData YY = X(image);

	XCHECKPOINT;
	SET_ROP(req->rop);

	XCopyArea(
		DISP, YY-> gdrawable, XX-> gdrawable, XX-> gc,
		req->src_x, req->src_y,
		req->w, req->h,
		req->dst_x, req->dst_y
	);

	XCHECKPOINT;
	XFLUSH;

	return true;
}

static Bool
img_put_ximage( Handle self, PrimaXImage * image, PutImageRequest * req)
{
	DEFXX;
	SET_ROP(req->rop);
	return prima_put_ximage(
		XX-> gdrawable, XX-> gc, image,
		req->src_x, req->src_y,
		req->dst_x, req->dst_y,
		req->w, req->h
	);
}

static Handle
img_get_image( Pixmap pixmap, PutImageRequest * req)
{
	XImage *i;
	Handle obj;
	Bool ok;

	XCHECKPOINT;
	if ( !( i = XGetImage( DISP, pixmap,
		req->src_x, req->src_y, req->w, req->h, AllPlanes, ZPixmap)))
		return NULL_HANDLE;

	obj = ( Handle) create_object("Prima::Image", "");
	CImage( obj)-> create_empty( obj, req->w, req->h, guts. qdepth);
	ok = prima_query_image( obj, i);
	XDestroyImage( i);
	if ( !ok ) {
		Object_destroy( obj );
		return NULL_HANDLE;
	}
	return obj;
}

static Bool
img_put_icon_mask( Handle self, PrimaXImage * icon, PutImageRequest * req)
{
	Bool ret;
	DEFXX;
	XSetForeground( DISP, XX-> gc, 0xFFFFFFFF);
	XSetBackground( DISP, XX-> gc, 0x00000000);
	XX-> flags. brush_fore = 0;
	XX-> flags. brush_back = 0;

	req->rop = GXand;
	XCHECKPOINT;
	ret = img_put_ximage( self, icon, req);
	req->rop = GXxor;
	return ret;
}

static Bool
img_put_bitmap_on_bitmap( Handle self, Handle image, PutImageRequest * req)
{
	PDrawableSysData YY = X(image);

	if ( XT_IS_DBM(YY) && XT_IS_BITMAP(YY))
		rop_apply_colors(self, req);

	return img_put_copy_area( self, image, req);
}

static Bool
img_put_image_on_bitmap( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	ImageCache *cache;
	PImage img = (PImage) image;
	PDrawableSysData YY = X(image);

	if (!(cache = prima_image_cache(img, CACHE_BITMAP, 255, 0)))
		return false;

	if ( XT_IS_ICON(YY) && !img_put_icon_mask( self, cache->icon, req))
		return false;

	XSetForeground( DISP, XX-> gc, 1);
	XSetBackground( DISP, XX-> gc, 0);
	XX-> flags. brush_fore = XX-> flags. brush_back = 0;

	return img_put_ximage( self, cache->image, req);
}

static Bool
img_put_pixmap_on_bitmap( Handle self, Handle image, PutImageRequest * req)
{
	Bool ret;
	Handle obj;

	if (!( obj = img_get_image( X(image)-> gdrawable, req )))
		return false;

	CImage( obj)-> set_type( obj, imBW);
	req->src_x = req->src_y = 0;
	ret = img_put_image_on_bitmap( self, obj, req);
	Object_destroy( obj);

	return ret;
}

static Bool
img_put_argb_on_bitmap( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	ImageCache *cache;
	int rop = req->rop;

	PImage img = (PImage) image;

	if (!(cache = prima_image_cache(img, CACHE_BITMAP, 255, 0)))
		return false;

	if ( !img_put_icon_mask( self, cache->icon, req))
		return false;

	req-> rop = ( rop == ropSrcCopy ) ? GXcopy : GXor;
	XSetForeground( DISP, XX-> gc, 1);
	XSetBackground( DISP, XX-> gc, 0);
	XX-> flags. brush_fore = XX-> flags. brush_back = 0;

	return img_put_ximage( self, cache->image, req);
}

Bool
prima_query_argb_rect( Handle self, Pixmap px, int x, int y, int w, int h);

static Bool
img_put_layered_on_bitmap( Handle self, Handle image, PutImageRequest * req)
{
	Handle obj;
	Bool ok;

	obj = ( Handle) create_object("Prima::Icon", "");
	ok = prima_query_argb_rect( obj, X(image)-> gdrawable, req-> src_x, req-> src_y, req-> w, req-> h);
	if ( !ok ) {
		Object_destroy( obj );
		return false;
	}

	req->src_x = req->src_y = 0;
	ok = img_put_argb_on_bitmap( self, obj, req );
	Object_destroy( obj);
	return ok;
}

static Bool
img_put_bitmap_on_pixmap( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	PDrawableSysData YY = X(image);

	/* XCopyPlane uses 0s for background and 1s for foreground */
	if ( XT_IS_BITMAP(YY)) {
		if ( XT_IS_DBM(YY)) {
			XSetBackground( DISP, XX-> gc, XX-> fore. primary);
			XSetForeground( DISP, XX-> gc, XX-> back. primary);
		} else {
			/* imBW in paint - no palettes, no colors, just plain black & white */
			if ( XF_LAYERED(XX)) {
				XSetForeground( DISP, XX-> gc, 0xFFFFFF);
				XSetBackground( DISP, XX-> gc, 0x000000);
			} else {
				XSetForeground( DISP, XX-> gc, guts. monochromeMap[1]);
				XSetBackground( DISP, XX-> gc, guts. monochromeMap[0]);
			}
		}
		XX->flags.brush_fore = XX->flags.brush_back = 0;
	}

	SET_ROP(req->rop);
	XCHECKPOINT;

	XCopyPlane(
		DISP, YY-> gdrawable, XX-> gdrawable, XX-> gc,
		req->src_x, req->src_y,
		req->w, req->h,
		req->dst_x, req->dst_y, 1
	);

	XCHECKPOINT;
	XFLUSH;

	return true;
}

static Bool
img_put_image_on_pixmap( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	ImageCache *cache;
	PImage img = (PImage) image;
	PDrawableSysData YY = X(image);

	if (!(cache = prima_image_cache(img, CACHE_PIXMAP, 255, 0)))
		return false;

	if ( XT_IS_ICON(YY) && !img_put_icon_mask( self, cache->icon, req))
		return false;

	if (( img->type & imBPP ) == 1) {
		unsigned long fore, back;
		query_1bit_colors(self, image, &fore, &back);
		XSetForeground( DISP, XX-> gc, fore);
		XSetBackground( DISP, XX-> gc, back);
		XX->flags.brush_fore = XX->flags.brush_back = 0;
	}

	return img_put_ximage( self, cache->image, req);
}


#define RENDER_APPLY_TRANSFORM(picture) \
	if ( req-> transform ) \
		XRenderSetPictureTransform( DISP, picture, req->transform)

#define RENDER_RESTORE_TRANSFORM(picture) \
	if ( req-> transform ) \
		XRenderSetPictureTransform( DISP, picture, &render_identity_transform)

#define ROP_SRC_OR_COPY  (req-> rop == ropSrcCopy) ? PictOpSrc : PictOpOver
#define RENDER_COMPOSITE( ofs, rop, picture )                    \
	XRenderComposite(                                        \
		DISP, rop,                                       \
		picture, 0, XX-> argb_picture,                   \
		req->ofs##_x, req->ofs##_y, 0, 0,                \
		req->dst_x, req->dst_y, req->dst_w, req->dst_h   \
	)

#define RENDER_FREE(picture) XRenderFreePicture( DISP, picture)

static Bool
img_put_layered_on_pixmap( Handle self, Handle image, PutImageRequest * req)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	RENDER_COMPOSITE( src, ROP_SRC_OR_COPY, X(image)->argb_picture);
	XRENDER_SYNC_NEEDED;
	return true;
#else
	return false;
#endif
}

static Bool
img_put_image_on_widget( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	ImageCache *cache;
	PImage img = (PImage) image;
	PDrawableSysData YY = X(image);

	if (!(cache = prima_image_cache(img, CACHE_PIXMAP, 255, 0)))
		return false;

	if ( XT_IS_ICON(YY) && !img_put_icon_mask( self, cache->icon, req))
		return false;

	if (( img->type & imBPP ) == 1) {
		unsigned long fore, back;
		query_1bit_colors(self, image, &fore, &back);
		XSetBackground( DISP, XX-> gc, back);
		XSetForeground( DISP, XX-> gc, fore);
		XX->flags.brush_back = XX->flags.brush_fore = 0;
	}

	if ( guts. dynamicColors) {
		int i;
		for ( i = 0; i < guts. palSize; i++)
			if (( wlpal_get( image, i) == RANK_FREE) &&
				( wlpal_get( self,  i) != RANK_FREE))
				prima_color_add_ref( self, i, RANK_LOCKED);
	}

	return img_put_ximage( self, cache->image, req);
}

static Bool
img_put_image_on_layered( Handle self, Handle image, PutImageRequest * req)
{
	ImageCache *cache;
	PDrawableSysData YY = X(image);
	if (!(cache = prima_image_cache((PImage) image, CACHE_LAYERED, 255, 0)))
		return false;
	if ( XT_IS_ICON(YY) && !img_put_icon_mask( self, cache->icon, req))
		return false;
	return img_put_ximage( self, cache->image, req);
}

static Bool
img_put_pixmap_on_layered( Handle self, Handle image, PutImageRequest * req)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	int render_rop = PictOpMinimum - 1;

	switch ( req-> rop ) {
		case GXcopy:  render_rop = PictOpSrc;   break;
		case GXclear: render_rop = PictOpClear; break;
		case GXnoop:  render_rop = PictOpDst;   break;
	}

	if ( render_rop >= PictOpMinimum ) {
		/* cheap on-server blit */
		RENDER_COMPOSITE( src, render_rop, X(image)->argb_picture);
		XRENDER_SYNC_NEEDED;
		return true;
	} else {
		/* expensive bit-transfer and blit with rop */
		Handle obj;
		Bool ret;
		if (!( obj = img_get_image( X(image)-> gdrawable, req )))
			return false;
		req-> src_x = req-> src_y = 0;
		ret = img_put_image_on_layered( self, obj, req );
		Object_destroy( obj);
		return ret;
	}
#else
	return false;
#endif
}

static Bool
img_render_argb_on_pixmap_or_widget( Handle self, Handle image, PutImageRequest * req)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	ImageCache *cache;
	Pixmap pixmap;
	GC gc;
	XGCValues gcv;
	Bool ret = false;
	Picture picture;

	if (!(cache = prima_image_cache((PImage) image, CACHE_LAYERED_ALPHA, 255, 0)))
		return false;

	pixmap = XCreatePixmap( DISP, guts.root, req->w, req->h, guts. argb_visual. depth);
	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, pixmap, GCGraphicsExposures, &gcv);

	SET_ROP(GXcopy);
	if ( !( prima_put_ximage(
		pixmap, gc, cache->image,
		req->src_x, req->src_y, 0, 0,
		req->w, req->h
	))) goto FAIL;

	picture = prima_render_create_picture(pixmap, 32);
	RENDER_APPLY_TRANSFORM(picture);
	RENDER_COMPOSITE( ofs, ROP_SRC_OR_COPY, picture);
	RENDER_FREE(picture);
	XRENDER_SYNC_NEEDED;
	ret = true;

FAIL:
	XFreeGC( DISP, gc);
	XFreePixmap( DISP, pixmap );
	return ret;
#else
	return false;
#endif
}

static Bool
img_put_a8_on_layered( Handle self, Handle image, PutImageRequest * req)
{
	DEFXX;
	Bool ok;
	ImageCache *cache;
	if (!(cache = prima_image_cache((PImage) image, CACHE_A8, 255, 0)))
		return false;
	XSetPlaneMask( DISP, XX-> gc, guts. argb_bits. alpha_mask);
	req->rop = GXcopy;
	ok = img_put_ximage( self, cache->image, req);
	XSetPlaneMask( DISP, XX-> gc, AllPlanes);
	return ok;
}

static Bool
img_put_argb_on_layered( Handle self, Handle image, PutImageRequest * req)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	ImageCache *cache;
	Pixmap pixmap;
	GC gc;
	XGCValues gcv;
	Bool ret = false;
	Picture picture;

	if (!(cache = prima_image_cache((PImage) image, CACHE_LAYERED_ALPHA, 255, 0)))
		return false;

	pixmap = XCreatePixmap( DISP, guts.root, req->w, req->h, guts. argb_visual. depth);
	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, pixmap, GCGraphicsExposures, &gcv);

	SET_ROP(GXcopy);
	if ( !( prima_put_ximage(
		pixmap, gc, cache->image,
		req->src_x, req->src_y, 0, 0,
		req->w, req->h
	))) goto FAIL;

	picture = prima_render_create_picture(pixmap, 32);
	RENDER_APPLY_TRANSFORM(picture);
	RENDER_COMPOSITE( ofs, ROP_SRC_OR_COPY, picture);
	RENDER_FREE(picture);
	XRENDER_SYNC_NEEDED;
	ret = true;

FAIL:
	XFreeGC( DISP, gc);
	XFreePixmap( DISP, pixmap );
	return ret;
#else
	return false;
#endif
}

#define SRC_BITMAP       0
#define SRC_PIXMAP       1
#define SRC_IMAGE        2
#define SRC_A8           3
#define SRC_ARGB         4
#define SRC_LAYERED      5
#define SRC_MAX          5
#define SRC_NUM          SRC_MAX+1

PutImageFunc (*img_put_on_bitmap[SRC_NUM]) = {
	img_put_bitmap_on_bitmap,
	img_put_pixmap_on_bitmap,
	img_put_image_on_bitmap,
	img_put_image_on_bitmap,
	img_put_argb_on_bitmap,
	img_put_layered_on_bitmap
};

PutImageFunc (*img_put_on_pixmap[SRC_NUM]) = {
	img_put_bitmap_on_pixmap,
	img_put_copy_area,
	img_put_image_on_pixmap,
	img_put_image_on_pixmap,
	img_put_image_on_pixmap,
	img_put_layered_on_pixmap
};

PutImageFunc (*img_put_on_widget[SRC_NUM]) = {
	img_put_bitmap_on_pixmap,
	img_put_copy_area,
	img_put_image_on_widget,
	img_put_image_on_widget,
	img_put_image_on_widget, 
	img_put_layered_on_pixmap
};

PutImageFunc (*img_put_on_layered[SRC_NUM]) = {
	img_put_bitmap_on_pixmap,
	img_put_pixmap_on_layered,
	img_put_image_on_layered,
	img_put_a8_on_layered,
	img_put_argb_on_layered,
	img_put_layered_on_pixmap
};

static Bool
img_render_image_on_picture( Handle self, Handle image, PutImageRequest * req, Bool on_layered)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	PImage img = (PImage) image;
	ImageCache *cache;
	Pixmap pixmap;
	GC gc;
	XGCValues gcv;
	Bool ret = false;
	Picture picture;

	/* request alpha_channel=255 because PictOpOver emulates SrcCopy here */
	if (!(cache = prima_image_cache((PImage) image,
		on_layered ? CACHE_LAYERED : CACHE_PIXMAP,
		255, 255)))
		return false;

	pixmap = XCreatePixmap( DISP, guts.root, req->w, req->h,
		on_layered ? guts.argb_visual.depth : guts.visual.depth
	);
	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, pixmap, GCGraphicsExposures, &gcv);
	if (( img->type & imBPP ) == 1) {
		unsigned long fore, back;
		query_1bit_colors(self, image, &fore, &back);
		XSetForeground( DISP, gc, fore);
		XSetBackground( DISP, gc, back);
	}

	SET_ROP(GXcopy);
	if ( !( prima_put_ximage(
		pixmap, gc, cache->image,
		req->src_x, req->src_y, 0, 0,
		req->w, req->h
	))) goto FAIL;

	picture = prima_render_create_picture(pixmap, on_layered ? 32 : 0);
	RENDER_APPLY_TRANSFORM(picture);
	RENDER_COMPOSITE( ofs, PictOpOver, picture);
	RENDER_FREE(picture);
	XRENDER_SYNC_NEEDED;
	ret = true;

FAIL:
	XFreeGC( DISP, gc);
	XFreePixmap( DISP, pixmap );
	return ret;
#else
	return false;
#endif
}

static Bool
img_render_image_on_pixmap( Handle self, Handle image, PutImageRequest * req)
{
	if ( XT_IS_ICON(X(image)))
		return img_put_image_on_pixmap(self, image, req);
	return img_render_image_on_picture( self, image, req, false);
}

static Bool
img_render_image_on_widget( Handle self, Handle image, PutImageRequest * req)
{
	if ( XT_IS_ICON(X(image)))
		return img_put_image_on_widget(self, image, req);
	return img_render_image_on_picture( self, image, req, false);
}

static Bool
img_render_image_on_layered( Handle self, Handle image, PutImageRequest * req)
{
	if ( XT_IS_ICON(X(image)))
		return img_put_image_on_layered(self, image, req);
	return img_render_image_on_picture( self, image, req, true);
}

static Bool
img_render_bitmap_on_picture( Handle self, Handle image, PutImageRequest * req)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	PDrawableSysData YY = X(image);
	Pixmap pixmap       = (Pixmap) 0;
	GC gc               = (GC) 0;
	Bool use_layered_surface;
	XGCValues gcv;
	Picture picture;

	/* only needed to match dst color format */
	use_layered_surface = XT_IS_DBM(YY) ? XF_LAYERED(XX) : 0;

	pixmap = XCreatePixmap( DISP, guts.root, req->w, req->h,
		use_layered_surface ? guts.argb_visual.depth : guts.visual.depth
	);

	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, pixmap, GCGraphicsExposures, &gcv);
	if ( XT_IS_DBM(YY)) {
		XSetBackground( DISP, gc, XX-> fore.primary);
		XSetForeground( DISP, gc, XX-> back.primary);
	} else {
		/* imBW in paint - no palettes, no colors, just plain black & white */
		if ( XF_LAYERED(XX)) {
			XSetForeground( DISP, gc, 0xFFFFFF);
			XSetBackground( DISP, gc, 0x000000);
		} else {
			XSetForeground( DISP, gc, guts.monochromeMap[1]);
			XSetBackground( DISP, gc, guts.monochromeMap[0]);
		}
	}
	XCopyPlane( DISP, YY->gdrawable, pixmap, gc,
		req-> src_x, req-> src_y,
		req-> w, req-> h,
		0, 0, 1
	);

	picture = prima_render_create_picture(pixmap, use_layered_surface ? 32 : 0);
	RENDER_APPLY_TRANSFORM(picture);
	RENDER_COMPOSITE( ofs, PictOpSrc, picture);
	RENDER_FREE(picture);
	XFreeGC( DISP, gc);
	XFreePixmap( DISP, pixmap );
	XRENDER_SYNC_NEEDED;
	return true;
#else
	return false;
#endif
}

static Bool
img_render_picture_on_pixmap( Handle self, Handle image, PutImageRequest * req, Bool from_layered)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
 	PDrawableSysData YY = X(image);
	PImage img          = (PImage) image;
	Pixmap pixmap       = (Pixmap) 0;
	GC gc               = (GC) 0;
	Bool quick          = false;
	XGCValues gcv;
	Picture picture;

	if ( req->w != img->w || req->h != img->h) {
		/* When a matrix is skewed, it is not possible to
		render a strict parallelogram because the compositor will peek
		into neighboring pixels, and will plot them where the dst
		pixels should be left intact. That's why we need to extract a
		rectangular sub-pixmap for these cases - but rectangular cases
		are just fine.
		The QUICK case should be valid for all 90-based rotations and mirrorings as well,
		but for now it is only scaled matrices
		*/
		if ( req-> scaling_only ) {
			req-> ofs_x = req-> src_x / XFixedToDouble(req-> transform-> matrix[0][0]);
			req-> ofs_y = req-> src_y / XFixedToDouble(req-> transform-> matrix[1][1]);
			goto QUICK;
		}

		pixmap = XCreatePixmap( DISP, guts.root, req->w, req->h,
			from_layered ? guts.argb_visual.depth : guts.visual.depth
		);
		gcv. graphics_exposures = false;
		gc = XCreateGC( DISP, pixmap, GCGraphicsExposures, &gcv);
		XCopyArea( DISP, YY->gdrawable, pixmap, gc,
			req-> src_x, req-> src_y,
			req-> w, req-> h,
			0, 0
		);

		picture = prima_render_create_picture(pixmap, from_layered ? 32 : 0);
	} else {
	QUICK:
		picture = YY->argb_picture;
		quick   = true;
	}

	RENDER_APPLY_TRANSFORM(picture);
	RENDER_COMPOSITE( ofs, ROP_SRC_OR_COPY, picture);

	if ( !quick ) {
		RENDER_FREE(picture);
		XFreeGC( DISP, gc);
		XFreePixmap( DISP, pixmap );
	} else
		RENDER_RESTORE_TRANSFORM(picture);

	XRENDER_SYNC_NEEDED;
	return true;
#else
	return false;
#endif
}

static Bool
img_render_layered_on_pixmap( Handle self, Handle image, PutImageRequest * req)
{
	return img_render_picture_on_pixmap(self, image, req, true);
}

static Bool
img_render_pixmap_or_widget_on_pixmap_or_widget( Handle self, Handle image, PutImageRequest * req)
{
	req-> rop = ropCopyPut;
	return img_render_picture_on_pixmap(self, image, req, false);
}

PutImageFunc (*img_render_nullset[SRC_NUM]) = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

PutImageFunc (*img_render_on_bitmap[SRC_NUM]) = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

PutImageFunc (*img_render_on_pixmap[SRC_NUM]) = {
	img_render_bitmap_on_picture,
	img_render_pixmap_or_widget_on_pixmap_or_widget,
	img_render_image_on_pixmap,
	NULL,
	img_render_argb_on_pixmap_or_widget,
	img_render_layered_on_pixmap
};

PutImageFunc (*img_render_on_widget[SRC_NUM]) = {
	img_render_bitmap_on_picture,
	img_render_pixmap_or_widget_on_pixmap_or_widget,
	img_render_image_on_widget,
	NULL,
	img_render_argb_on_pixmap_or_widget,
	img_render_layered_on_pixmap
};

PutImageFunc (*img_render_on_layered[SRC_NUM]) = {
	img_render_bitmap_on_picture,
	img_render_pixmap_or_widget_on_pixmap_or_widget,
	img_render_image_on_layered,
	NULL,
	img_put_argb_on_layered,
	img_render_layered_on_pixmap
};

static int
get_image_src_format( Handle self, Handle image, int * rop )
{
	DEFXX;
	PDrawableSysData YY = X(image);
	int src = -1;

	if ( XT_IS_DBM(YY)) {
		if (XT_IS_BITMAP(YY) || ( XT_IS_PIXMAP(YY) && guts.depth==1))
			src = SRC_BITMAP;
		else if ( XF_LAYERED(YY))
			src = SRC_LAYERED;
		else if ( XT_IS_PIXMAP(YY))
			src = SRC_PIXMAP;
	} else if ( XT_IS_IMAGE(YY)) {
		if ( XF_IN_PAINT(YY)) {
			if ( XT_IS_BITMAP(YY) || ( XT_IS_PIXMAP(YY) && guts.depth==1))
				src = SRC_BITMAP;
			else if ( XF_LAYERED(YY))
				src = SRC_LAYERED;
			else if ( XT_IS_PIXMAP(YY))
				src = SRC_PIXMAP;
		} else if ( XT_IS_ICON(YY) && PIcon(image)->maskType == imbpp8) {
			src = SRC_ARGB;
		} else {
			src = SRC_IMAGE;
			if (XF_LAYERED(XX) && !XT_IS_ICON(YY) && (PImage(image)->type & imGrayScale) && *rop == ropAlphaCopy ) {
				src = SRC_A8;
				*rop = ropCopyPut;
			}
		}
	}

	return src;
}

static PutImageFunc**
get_image_dst_format( Handle self, int rop, int src_type, Bool use_xrender )
{
	DEFXX;

	if ( use_xrender ) {
		if ( !guts.render_extension || !guts.render_matrix_enabled )
			return img_render_nullset;
		if ( !guts.render_supports_argb32 && src_type == SRC_ARGB )
			return img_render_nullset;
		/* xrender cannot rops */
		if ( src_type != SRC_LAYERED && src_type != SRC_ARGB && rop != ropCopyPut )
			return img_render_nullset;
	}

	if (XT_IS_BITMAP(XX) || (( XT_IS_PIXMAP(XX) || XT_IS_APPLICATION(XX)) && guts.depth==1))
		return use_xrender ? img_render_on_bitmap : img_put_on_bitmap;
	else if ( XF_LAYERED(XX))
		return use_xrender ? img_render_on_layered : img_put_on_layered;
	else if ( XT_IS_PIXMAP(XX) || XT_IS_APPLICATION(XX))
		return use_xrender ? img_render_on_pixmap :  img_put_on_pixmap;
	else if ( XT_IS_WIDGET(XX))
		return use_xrender ? img_render_on_widget :  img_put_on_widget;

	return NULL;
}

Bool
apc_gp_put_image( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xLen, int yLen, int rop)
{
	DEFXX;
	PImage img = PImage( image);
	PutImageRequest req;
	PutImageFunc ** dst = NULL;
	Bool ok;
	XGCValues gcv;
	int src;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	if ( xFrom >= img-> w || yFrom >= img-> h) return false;
	if ( xFrom + xLen > img-> w) xLen = img-> w - xFrom;
	if ( yFrom + yLen > img-> h) yLen = img-> h - yFrom;
	if ( xLen <= 0 || yLen <= 0) return false;

	SHIFT( x, y);
	bzero( &req, sizeof(req));
	req.src_x = xFrom;
	req.src_y = img->h - yFrom - yLen;
	req.dst_x = x;
	req.dst_y = XX->size. y - y - yLen;
	req.w     = xLen;
	req.h     = yLen;
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	req.dst_w = req.w;
	req.dst_h = req.h;
#endif

	src = get_image_src_format(self, image, &rop);
	if ( rop > ropNoOper ) return false;
	if ( src < 0 ) {
		warn("cannot guess image type");
		return false;
	}
	if (!(dst = get_image_dst_format(self, rop, src, true))) {
		warn("cannot guess surface type");
		return false;
	}
	if ( !dst[src] ) {
		if (!( dst = get_image_dst_format(self, rop, src, false))) {
			warn("cannot guess surface type");
			return false;
		}
		if ( !dst[src] ) {
			warn("cannot guess image type");
			return false;
		}
	}

	if ( !XGetGCValues(DISP, XX->gc, GCFunction, &gcv))
		warn("cannot query XGCValues");

	req.old_rop = gcv.function;
	req.rop     = (src == SRC_LAYERED || src == SRC_ARGB) ? rop : prima_rop_map( rop);

	ok = (*dst[src])(self, image, &req);

	if ( gcv.function != req. old_rop)
		XSetFunction( DISP, XX->gc, gcv. function);

	return ok;
}

Bool
apc_image_begin_paint( Handle self)
{
	DEFXX;
	PIcon img = PIcon( self);
	int icon = XX-> type. icon;
	Bool bitmap = (img-> type  == imBW) || ( guts. idepth == 1);
	Bool layered = icon && img-> maskType == imbpp8 && guts. argb_visual. visual;
	int depth = layered ? guts. argb_depth : ( bitmap ? 1 : guts. depth );

	if ( !DISP) return false;
	if (img-> w == 0 || img-> h == 0) return false;

	XX-> gdrawable = XCreatePixmap( DISP, guts. root, img-> w, img-> h, depth);

	XX-> type.pixmap = !bitmap;
	XX-> type.bitmap = !!bitmap;
	XX-> flags.layered = layered;
	XX-> visual      = &guts. visual;
	XX-> colormap    = guts. defaultColormap;
	if ( XF_LAYERED(XX)) {
		XX-> visual    = &guts. argb_visual;
		XX-> colormap  = guts. argbColormap;
	}
	CREATE_ARGB_PICTURE(XX->gdrawable,
		bitmap ? 1 : (XF_LAYERED(XX) ? 32 : 0),
		XX->argb_picture);
	XCHECKPOINT;
	XX-> type. icon = 0;
	prima_prepare_drawable_for_painting( self, false);
	XX-> type. icon = icon;
	PObject( self)-> options. optInDraw = 0;
	XX->flags. paint = 0;

	if (is_opt(optReadonlyPaint)) {
		XX-> flags. brush_fore = 0;
		guts.xrender_pen_dirty = true;
		XSetForeground( DISP, XX->gc, 0);
		XFillRectangle( DISP, XX->gdrawable, XX->gc, 0, 0, img-> w, img-> h);
	} else {
		PutImageRequest req;
		PutImageFunc ** dst = layered ? img_put_on_layered : ( bitmap ? img_put_on_bitmap : img_put_on_pixmap );
		bzero(&req, sizeof(req));
		req.w   = img-> w;
		req.h   = img-> h;
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
		req.dst_w = req.w;
		req.dst_h = req.h;
#endif
		req.rop = layered ? ropSrcCopy : GXcopy;
		req.old_rop = XX-> gcv. function;
		(*dst[layered ? SRC_ARGB : SRC_IMAGE])(self, self, &req);
		/*                                     ^^^^^ ^^^^    :-)))  */
		if ( req. old_rop != XX-> gcv. function)
			XSetFunction( DISP, XX-> gc, XX-> gcv. function);
	}

	PObject( self)-> options. optInDraw = 1;
	XX->flags. paint = 1;
	return true;
}

static void
convert_8_to_24( XImage *i, PImage img, RGBABitDescription * bits)
{
	int y, x, h, w;
	Pixel8 *d;
	register Pixel24 *line;

	/*
		Compensate less than 8-bit true-color memory layout depth converted into
		real 8 bit, a bit slower but more error-prone in general sense. Although
		Prima::gp-problems advises not to check against 0xffffff as white, since
		white is 0xf8f8f8 on 15-bit displays for example, it is not practical to
		use this check fro example when a RGB image is to be converted into a
		low-palette image with RGB(0xff,0xff,0xff) expected and desirable palette
		slot value.
	*/

	int rmax = 0xff & ( 0xff << ( 8 - bits-> red_range));
	int gmax = 0xff & ( 0xff << ( 8 - bits-> green_range));
	int bmax = 0xff & ( 0xff << ( 8 - bits-> blue_range));
	if ( rmax == 0 ) rmax = 0xff;
	if ( gmax == 0 ) gmax = 0xff;
	if ( bmax == 0 ) bmax = 0xff;

	h = img-> h; w = img-> w;
	for ( y = 0; y < h; y++) {
		d = (Pixel8 *)(i-> data + (h-y-1)*i-> bytes_per_line);
		line = (Pixel24*)(img-> data + y*img-> lineSize);
		for ( x = 0; x < w; x++) {
			line-> a0 = (((*d & bits-> blue_mask)  >> bits-> blue_shift) << 8)  >> bits->  blue_range;
			line-> a1 = (((*d & bits-> green_mask) >> bits-> green_shift) << 8) >> bits->  green_range;
			line-> a2 = (((*d & bits-> red_mask)   >> bits-> red_shift) << 8)   >> bits->  red_range;
			if ( line-> a0 == bmax) line-> a0 = 0xff;
			if ( line-> a1 == gmax) line-> a1 = 0xff;
			if ( line-> a2 == rmax) line-> a2 = 0xff;
			line++; d++;
		}
	}
}
static void
convert_16_to_24( XImage *i, PImage img, RGBABitDescription * bits)
{
	int y, x, h, w;
	Pixel16 *d;
	register Pixel24 *line;

	/*
		Compensate less than 8-bit true-color memory layout depth converted into
		real 8 bit, a bit slower but more error-prone in general sense. Although
		Prima::gp-problems advises not to check against 0xffffff as white, since
		white is 0xf8f8f8 on 15-bit displays for example, it is not practical to
		use this check fro example when a RGB image is to be converted into a
		low-palette image with RGB(0xff,0xff,0xff) expected and desirable palette
		slot value.
	*/

	int rmax = 0xff & ( 0xff << ( 8 - bits-> red_range));
	int gmax = 0xff & ( 0xff << ( 8 - bits-> green_range));
	int bmax = 0xff & ( 0xff << ( 8 - bits-> blue_range));
	if ( rmax == 0 ) rmax = 0xff;
	if ( gmax == 0 ) gmax = 0xff;
	if ( bmax == 0 ) bmax = 0xff;

	h = img-> h; w = img-> w;
	for ( y = 0; y < h; y++) {
		d = (Pixel16 *)(i-> data + (h-y-1)*i-> bytes_per_line);
		line = (Pixel24*)(img-> data + y*img-> lineSize);
		if ( guts.machine_byte_order != guts.byte_order) {
			for ( x = 0; x < w; x++) {
				register Pixel16 dd = REVERSE_BYTES_16(*d);
				line-> a0 = (((dd & bits-> blue_mask)  >> bits-> blue_shift) << 8)  >> bits->  blue_range;
				line-> a1 = (((dd & bits-> green_mask) >> bits-> green_shift) << 8) >> bits->  green_range;
				line-> a2 = (((dd & bits-> red_mask)   >> bits-> red_shift) << 8)   >> bits->  red_range;
				if ( line-> a0 == bmax) line-> a0 = 0xff;
				if ( line-> a1 == gmax) line-> a1 = 0xff;
				if ( line-> a2 == rmax) line-> a2 = 0xff;
				line++; d++;
			}
		} else {
			for ( x = 0; x < w; x++) {
				line-> a0 = (((*d & bits-> blue_mask)  >> bits-> blue_shift) << 8)  >> bits->  blue_range;
				line-> a1 = (((*d & bits-> green_mask) >> bits-> green_shift) << 8) >> bits->  green_range;
				line-> a2 = (((*d & bits-> red_mask)   >> bits-> red_shift) << 8)   >> bits->  red_range;
				if ( line-> a0 == bmax) line-> a0 = 0xff;
				if ( line-> a1 == gmax) line-> a1 = 0xff;
				if ( line-> a2 == rmax) line-> a2 = 0xff;
				line++; d++;
			}
		}
	}
}

static void
convert_32_to_24( XImage *i, PImage img, RGBABitDescription * bits)
{
	int y, x, h, w;
	Pixel32 *d, dd;
	Pixel24 *line;

	h = img-> h; w = img-> w;
	if ( guts.machine_byte_order != guts.byte_order) {
		for ( y = 0; y < h; y++) {
			d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
			line = (Pixel24*)(img-> data + y*img-> lineSize);
			for ( x = 0; x < w; x++) {
				dd = REVERSE_BYTES_32(*d);
				line-> a0 = (((dd & bits-> blue_mask)  >> bits-> blue_shift) << 8)  >> bits-> blue_range;
				line-> a1 = (((dd & bits-> green_mask) >> bits-> green_shift) << 8) >> bits-> green_range;
				line-> a2 = (((dd & bits-> red_mask)   >> bits-> red_shift) << 8)   >> bits-> red_range;
				d++; line++;
			}
		}
	} else {
		for ( y = 0; y < h; y++) {
			d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
			line = (Pixel24*)(img-> data + y*img-> lineSize);
			for ( x = 0; x < w; x++) {
				line-> a0 = (((*d & bits-> blue_mask)  >> bits-> blue_shift) << 8)  >> bits-> blue_range;
				line-> a1 = (((*d & bits-> green_mask) >> bits-> green_shift) << 8) >> bits-> green_range;
				line-> a2 = (((*d & bits-> red_mask)   >> bits-> red_shift) << 8)   >> bits-> red_range;
				d++; line++;
			}
		}
	}
}

static void
convert_equal_paletted( XImage *i, PImage img)
{
	int y, h;
	Pixel8 *d, *line;
	XColor xc[256];

	h = img-> h;
	d = ( Pixel8*)(i-> data + (h-1) * i-> bytes_per_line);
	line = (Pixel8*)img-> data;
	bzero( line, img-> dataSize);
	for ( y = 0; y < h; y++) {
		memcpy( line, d, img-> w);
		d -= i-> bytes_per_line;
		line += img-> lineSize;
	}
	for ( y = 0; y < 256; y++) guts. mappingPlace[y] = -1;
	for ( y = 0; y < img-> dataSize; y++)
		guts. mappingPlace[ img-> data[y]] = 0;
	for ( y = 0; y < guts. palSize; y++) xc[y]. pixel = y;
	XQueryColors( DISP, guts. defaultColormap, xc, guts. palSize);

	img-> palSize = 0;
	for ( y = 0; y < 256; y++)
		if ( guts. mappingPlace[y] == 0) {
			img-> palette[img-> palSize]. r = xc[y].red/256;
			img-> palette[img-> palSize]. g = xc[y].green/256;
			img-> palette[img-> palSize]. b = xc[y].blue/256;
			guts. mappingPlace[y] = img-> palSize++;
		}

	for ( y = 0; y < img-> dataSize; y++)
		img-> data[y] = guts. mappingPlace[ img-> data[y]];
}

Bool
prima_query_image( Handle self, XImage * i)
{
	PImage img = PImage( self);
	int target_depth = ( img-> type == imBW) ? 1 : guts. qdepth;

	if (( img-> type & imBPP) != target_depth)
		CImage( self)-> create_empty( self, img-> w, img-> h, target_depth);

	X(self)-> size. x = img-> w;
	X(self)-> size. y = img-> h;

	if ( target_depth == 1) {
		prima_copy_1bit_ximage( img->data, i, false);
	} else {
	switch ( guts. idepth) {
	case 8:
		switch ( target_depth) {
		case 4:
			CImage( self)-> create_empty( self, img-> w, img-> h, 8);
		case 8:
			convert_equal_paletted( i, img);
			break;
		default: goto slurp_image_unsupported_depth;
		}
		break;
	case 16:
		switch ( target_depth) {
		case 24:
			convert_16_to_24( i, img, &guts. screen_bits);
			break;
		default: goto slurp_image_unsupported_depth;
		}
		break;
	case 32:
		switch ( target_depth) {
		case 24:
			convert_32_to_24( i, img, &guts. screen_bits);
			break;
		default: goto slurp_image_unsupported_depth;
		}
		break;
slurp_image_unsupported_depth:
	default:
		warn("UAI_023: unsupported backing image conversion from %d to %d\n", guts.idepth, target_depth);
		return false;
	}
	}
	return true;
}

Bool
prima_std_query_image( Handle self, Pixmap px)
{
	XImage * i;
	Bool mono = PImage(self)-> type == imBW || guts. depth == 1;
	Bool ret;
	if (!( i = XGetImage( DISP, px, 0, 0,
		PImage(self)-> w, PImage( self)-> h,
		mono ? 1 : AllPlanes, mono ? XYPixmap : ZPixmap)))
		return false;
	XCHECKPOINT;
	ret = prima_query_image( self, i);
	XDestroyImage( i);
	return ret;
}

static void
convert_32_to_mask( XImage *i, PIcon img)
{
	int y, x, h, w;
	Pixel32 *d, dd;
	Pixel8 *line;
	int max = 0xff & ( 0xff << ( 8 - guts. argb_bits. alpha_range));
	if ( max == 0 ) max = 0xff;

	h = img-> h; w = img-> w;
	if ( guts.machine_byte_order != guts.byte_order) {
		for ( y = 0; y < h; y++) {
			d = (Pixel32 *)(i-> data + (h-y-1)*i-> bytes_per_line);
			line = (Pixel8*)(img-> mask + y*img-> maskLine);
			for ( x = 0; x < w; x++) {
				dd = REVERSE_BYTES_32(*d);
				*line = (((dd & guts. argb_bits. alpha_mask) >> guts. argb_bits. alpha_shift) << 8) >> guts. argb_bits. alpha_range;
				if ( *line == max) *line = 0xff;
				d++; line++;
			}
		}
	} else {
		for ( y = 0; y < h; y++) {
			d = (Pixel32*)(i-> data + (h-y-1)*i-> bytes_per_line);
			line = (Pixel8*)(img-> mask + y*img-> maskLine);
			for ( x = 0; x < w; x++) {
				*line = (((*d & guts. argb_bits. alpha_mask) >> guts. argb_bits. alpha_shift) << 8) >> guts. argb_bits. alpha_range;
				if ( *line == max) *line = 0xff;
				d++; line++;
			}
		}
	}
}

static void
convert_16_to_mask( XImage *i, PIcon img)
{
	int y, x, h, w;
	Pixel16 *d, dd;
	Pixel8 *line;
	int max = 0xff & ( 0xff << ( 8 - guts. argb_bits. alpha_range));
	if ( max == 0 ) max = 0xff;

	h = img-> h; w = img-> w;
	if ( guts.machine_byte_order != guts.byte_order) {
		for ( y = 0; y < h; y++) {
			d = (Pixel16 *)(i-> data + (h-y-1)*i-> bytes_per_line);
			line = (Pixel8*)(img-> mask + y*img-> maskLine);
			for ( x = 0; x < w; x++) {
				dd = REVERSE_BYTES_16(*d);
				*line = (((dd & guts. argb_bits. alpha_mask) >> guts. argb_bits. alpha_shift) << 8) >> guts. argb_bits. alpha_range;
				if ( *line == max) *line = 0xff;
				d++; line++;
			}
		}
	} else {
		for ( y = 0; y < h; y++) {
			d = (Pixel16*)(i-> data + (h-y-1)*i-> bytes_per_line);
			line = (Pixel8*)(img-> mask + y*img-> maskLine);
			for ( x = 0; x < w; x++) {
				*line = (((*d & guts. argb_bits. alpha_mask) >> guts. argb_bits. alpha_shift) << 8) >> guts. argb_bits. alpha_range;
				if ( *line == max) *line = 0xff;
				d++; line++;
			}
		}
	}
}

static void
convert_8_to_mask( XImage *i, PIcon img)
{
	int y, x, h, w;
	Pixel8 *d;
	Pixel8 *line;
	int max = 0xff & ( 0xff << ( 8 - guts. argb_bits. alpha_range));
	if ( max == 0 ) max = 0xff;

	h = img-> h; w = img-> w;
	for ( y = 0; y < h; y++) {
		d = (Pixel8*)(i-> data + (h-y-1)*i-> bytes_per_line);
		line = (Pixel8*)(img-> mask + y*img-> maskLine);
		for ( x = 0; x < w; x++) {
			*line = (((*d & guts. argb_bits. alpha_mask) >> guts. argb_bits. alpha_shift) << 8) >> guts. argb_bits. alpha_range;
			if ( *line == max) *line = 0xff;
			d++; line++;
		}
	}
}

Bool
prima_query_argb_rect( Handle self, Pixmap px, int x, int y, int w, int h)
{
	XImage * i;
	PIcon img = (PIcon) self;

	if (!( i = XGetImage( DISP, px, x, y, w, h,
		AllPlanes, ZPixmap)))
		return false;
	XCHECKPOINT;

	if (( img-> type & imBPP) != 24 || img->maskType != imbpp8)
		CIcon( self)-> create_empty_icon( self, w, h, imRGB, imbpp8);

	switch ( guts. argb_depth) {
	case 8:
		convert_8_to_24( i, (PImage)img, &guts. argb_bits);
		convert_8_to_mask( i, img);
		break;
	case 16:
		convert_16_to_24( i, (PImage)img, &guts. argb_bits);
		convert_16_to_mask( i, img);
		break;
	case 32:
		convert_32_to_24( i, (PImage)img, &guts. argb_bits);
		convert_32_to_mask( i, img);
		break;
	default:
		warn("UAI_023: unsupported backing image conversion from %d to %d\n", guts. argb_depth, guts. qdepth);
		return false;
	}

	XDestroyImage( i);
	return true;
}

Bool
prima_query_argb_image( Handle self, Pixmap px)
{
	return prima_query_argb_rect( self, px, 0, 0, PImage(self)-> w, PImage( self)-> h);
}

Pixmap
prima_std_pixmap( Handle self, int type)
{
	Pixmap px;
	XGCValues gcv;
	GC gc;
	PImage img = ( PImage) self;
	unsigned long fore, back;

	ImageCache * xi = prima_image_cache(( PImage) self, type, 255, 0);
	if ( !xi) return NULL_HANDLE;

	px = XCreatePixmap( DISP, guts. root, img-> w, img-> h,
		( type == CACHE_BITMAP) ? 1 : guts. depth);
	if ( !px) return NULL_HANDLE;

	gcv. graphics_exposures = false;
	gc = XCreateGC( DISP, guts. root, GCGraphicsExposures, &gcv);
	query_1bit_colors(self, self, &fore, &back);
	XSetForeground( DISP, gc, fore);
	XSetBackground( DISP, gc, back);
	prima_put_ximage( px, gc, xi->image, 0, 0, 0, 0, img-> w, img-> h);
	XFreeGC( DISP, gc);
	return px;
}

Bool
apc_image_end_paint( Handle self)
{
	DEFXX;
	if ( XF_LAYERED(XX))
		prima_query_argb_image( self, XX-> gdrawable);
	else
		prima_std_query_image( self, XX-> gdrawable);
	prima_cleanup_drawable_after_painting( self);
	if ( XX-> gdrawable) {
		XFreePixmap( DISP, XX-> gdrawable);
		XCHECKPOINT;
		XX-> gdrawable = 0;
	}
	clear_caches( self);
	return true;
}

static NRect
pt4_extents( NPoint *pt)
{
	int i;
	double x1, x2, y1, y2;
	NRect r;
	x1 = x2 = pt[0].x;
	y1 = y2 = pt[0].y;
	for ( i = 1; i < 4; i++) {
		if ( x1 > pt[i].x ) x1 = pt[i].x;
		if ( y1 > pt[i].y ) y1 = pt[i].y;
		if ( x2 < pt[i].x ) x2 = pt[i].x;
		if ( y2 < pt[i].y ) y2 = pt[i].y;
	}
	r.left   = x1;
	r.bottom = y1;
	r.right  = x2;
	r.top    = y2;
	return r;
}

Bool
put_transformed(Handle self, Handle image, int x, int y, int rop, Matrix matrix)
{
	ColorPixel fill;
	PImage img = (PImage) image;
	PDrawableSysData YY = X(image);
	NRect r;
	NPoint pt[4];

	memset(&fill, 0x0, sizeof(fill));
	r.left   = 0.0;
	r.bottom = 0.0;
	r.right  = (double) img->w;
	r.top    = (double) img->h;

	prima_matrix_is_square_rectangular( matrix, &r, pt);
	r = pt4_extents(pt);
	x += floor(r.left);
	y += floor(r.bottom);


	if ( XT_IS_ICON(YY)) {
		img->self->set_preserveType(image, 0);
		img->self->matrix_transform(image, matrix, fill);
		if ( !guts.render_supports_argb32 )
			CIcon(img)->set_maskType( self, imbpp1 );
		return apc_gp_put_image( self, image, x, y, 0, 0, img->w, img-> h, ropXorPut);
	} else {
		Handle ok;
		Handle icon = img->self->convert_to_icon(image, imbpp8, NULL);
		CIcon(icon)->matrix_transform(icon, matrix, fill);
		if ( !guts.render_supports_argb32 )
			CIcon(icon)->set_maskType( icon, imbpp1 );
		ok = apc_gp_put_image( self, icon, x, y, 0, 0, PIcon(icon)->w, PIcon(icon)->h, rop);
		Object_destroy(icon);
		return ok;
	}
}

static Bool
apc_gp_stretch_image_x11( Handle self, Handle image,
	int dst_x, int dst_y, int src_x, int src_y,
	int dst_w, int dst_h, int src_w, int src_h,
	int rop, Bool use_matrix)
{
	DEFXX;
	PDrawableSysData YY = X(image);
	PImage img = (PImage) image;
	int src;
	Handle obj;
	Bool ok;

	if ( PObject( self)-> options. optInDrawInfo) return false;
	if ( !XF_IN_PAINT(XX)) return false;

	src = get_image_src_format(self, image, &rop);
	if ( rop > ropNoOper ) return false;
	if ( src < 0 ) return false;

	XRENDER_SYNC;

	/* query xserver bits */
	if ( src == SRC_BITMAP || src == SRC_PIXMAP ) {
		XImage *i;

		if ( !( i = XGetImage( DISP, YY-> gdrawable,
				src_x, img-> h - src_y - src_h, src_w, src_h,
				AllPlanes, (src == SRC_BITMAP) ? XYPixmap : ZPixmap)))
			return false;

		if ( XT_IS_ICON(YY)) {
			int height = src_w;
			PIcon isrc = (PIcon) image, idst;
			obj = ( Handle) create_object("Prima::Icon", "");
			idst = (PIcon) obj;
			CIcon( obj)-> create_empty_icon( obj, src_w, src_h,
				(src == SRC_BITMAP) ? imBW : guts. qdepth,
				isrc-> maskType
			);
			if ( isrc->maskType == imbpp8) {
				while ( height-- > 0)
					memcpy(
						idst-> mask + height * idst-> maskLine,
						isrc-> mask + ( src_y + height) * isrc-> maskLine + src_x, src_w);
			} else {
				while ( height-- > 0)
					bc_mono_copy(
						isrc->mask + ( src_y + height) * isrc->maskLine,
						idst-> mask + height * idst-> maskLine, src_x, src_w);
			}
		} else {
			obj = ( Handle) create_object("Prima::Image", "");
			CIcon( obj)-> create_empty( obj, src_w, src_h,
				(src == SRC_BITMAP) ? imBW : guts. qdepth
			);
		}
		if (!prima_query_image( obj, i)) {
			XDestroyImage( i);
			Object_destroy( obj);
			return false;
		}
		XDestroyImage( i);
		if ( src == SRC_BITMAP && !XT_IS_IMAGE(YY)) {
			PImage o = (PImage) obj;
			o->type = imbpp1;
			o->palette[0].r = XX->fore. color & 0xff;
			o->palette[0].g = (XX->fore. color >> 8) & 0xff;
			o->palette[0].b = (XX->fore. color >> 16) & 0xff;
			o->palette[1].r = XX->back. color & 0xff;
			o->palette[1].g = (XX->back. color >> 8) & 0xff;
			o->palette[1].b = (XX->back. color >> 16) & 0xff;
		}
		ok = apc_gp_stretch_image( self, obj, dst_x, dst_y, 0, 0, dst_w, dst_h, src_w, src_h, rop, use_matrix);
	} else if ( src == SRC_LAYERED ) {
		obj = ( Handle) create_object("Prima::Icon", "");
		ok = prima_query_argb_rect( obj, X(image)-> gdrawable, src_x, PDrawable(image)-> h - src_h - src_y, src_w, src_h);
		if ( !ok ) {
			Object_destroy( obj );
			return false;
		}
		ok = apc_gp_stretch_image( self, obj, dst_x, dst_y, 0, 0, dst_w, dst_h, src_w, src_h, rop, use_matrix);
	} else if ( use_matrix || img->w != dst_w || img->h != dst_h || src_x != 0 || src_y != 0) {
		/* extract local bits */
		obj = CImage(image)->extract( image, src_x, src_y, src_w, src_h );
		if ( !obj ) return false;
		if ( use_matrix ) {
			Matrix m1, m2, m3;
			prima_matrix_set_identity(m1);
			m1[0] = (double) dst_w / img->w;
			m1[3] = (double) dst_h / img->h;
			COPY_MATRIX_WITHOUT_TRANSLATION( PDrawable(self)->current_state.matrix, m2);
			prima_matrix_multiply(m1, m2, m3);
			ok = put_transformed( self, obj, dst_x, dst_y, rop, m3);
		} else {
			CImage(obj)-> stretch( obj, dst_w, dst_h );
			ok  = apc_gp_put_image( self, obj, dst_x, dst_y, 0, 0, dst_w, dst_h, rop);
		}
	} else {
		return apc_gp_put_image( self, image, dst_x, dst_y, 0, 0, dst_w, dst_h, rop);
	}

	Object_destroy( obj );
	return ok;
}

static Bool
apc_gp_stretch_image_xrender( Handle self, Handle image, PutImageFunc* func,
	int dst_x, int dst_y, int src_x, int src_y,
	int dst_w, int dst_h, int src_w, int src_h,
	int rop, Bool use_matrix)
{
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
	DEFXX;
	int dx, dy;
	Matrix m1;
	XTransform xt;
	NRect r;
	NPoint pt[4];
	double det;
	PImage img = (PImage) image;
	PutImageRequest req = {
		.src_x   = src_x,
		.src_y   = img->h - src_y - src_h,
		.w       = src_w,
		.h       = src_h,
		.rop     = rop,
	};

	if ( src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0 ) return false;
	SHIFT( dst_x, dst_y);
	prima_matrix_set_identity(m1);
	m1[0] = (double) dst_w / src_w;
	m1[3] = (double) dst_h / src_h;
	if ( use_matrix ) {
		Matrix m2, m3;
		COPY_MATRIX_WITHOUT_TRANSLATION( PDrawable(self)->current_state.matrix, m2);
		prima_matrix_multiply(m1, m2, m3);
		COPY_MATRIX(m3, m1);
	}

	bzero( &xt, sizeof(xt) );
	det = m1[0] * m1[3] - m1[1] * m1[2];
	if ( det == 0.0 ) return false;
	xt.matrix[0][0] = XDoubleToFixed( m1[3] / det );
	xt.matrix[0][1] = XDoubleToFixed( m1[2] / det );
	xt.matrix[1][0] = XDoubleToFixed( m1[1] / det );
	xt.matrix[1][1] = XDoubleToFixed( m1[0] / det);
	xt.matrix[2][2] = XDoubleToFixed( 1.0 );

	req.scaling_only = m1[1] == 0.0 && m1[2] == 0.0;
	req.transform = &xt;
	r.left     = 0.0;
	r.bottom   = 0.0;
	r.right    = (double) src_w;
	r.top      = (double) src_h;
	prima_matrix_is_square_rectangular( m1, &r, pt);
	r          = pt4_extents(pt);
	dx         = floor(pt[3].x + .5);
	req.dst_x  = (dx < 0) ? dx : 0.0;
	req.ofs_x  = (dx < 0) ? 0.0 : -dx;
	dy         = floor(pt[1].y + .5);
	req.dst_y  = (dy < 0) ? dy : 0.0;
	req.ofs_y  = (dy < 0) ? 0.0 : -dy;
	req.dst_w  = ceil(r.right) - req.dst_x;
	req.dst_h  = ceil(r.top) - req.dst_y;
	req.dst_x += dst_x;
	req.dst_y += dst_y;
	req.dst_y  = XX->size.y - req.dst_y - req.dst_h;
	return func(self, image, &req);
#else
	return false;
#endif
}

Bool
apc_gp_stretch_image( Handle self, Handle image,
	int dst_x, int dst_y, int src_x, int src_y,
	int dst_w, int dst_h, int src_w, int src_h,
	int rop, Bool use_matrix)
{
	int src;
	PutImageFunc **dst;
	PImage img = (PImage) image;

	if ( src_h < 0) {
		src_h = -src_h;
		dst_h = -dst_h;
	}
	if ( src_w < 0) {
		src_w = -src_w;
		dst_w = -dst_w;
	}
	if ( abs(src_x) >= img-> w) return false;
	if ( abs(src_y) >= img-> h) return false;
	if ( src_w == 0 || src_h == 0) return false;
	if ( src_x < 0) {
		dst_x -= src_x * dst_w / src_w;
		dst_w += src_x * dst_w / src_w;
		src_w += src_x;
		src_x = 0;
	}
	if ( src_y < 0) {
		dst_y -= src_y * dst_h / src_h;
		dst_h += src_y * dst_h / src_h;
		src_h += src_y;
		src_y = 0;
	}
	if ( src_x + src_w > img-> w) {
		dst_w = (img-> w - src_x) * dst_w / src_w;
		src_w = img-> w - src_x;
	}
	if ( src_y + src_h > img-> h) {
		dst_h = (img-> h - src_y) * dst_h / src_h;
		src_h = img-> h - src_y;
	}
	if ( src_w <= 0 || src_h <= 0) return false;

	if ( !guts.render_extension)
		goto FALLBACK;

	src = get_image_src_format(self, image, &rop);
	if ( rop > ropNoOper ) return false;
	if ( src < 0 ) return false;

	if (!(dst = get_image_dst_format(self, rop, src, true))) {
		warn("cannot guess surface type");
		return false;
	}
	if ( !dst[src] )
		goto FALLBACK;

	switch ( rop ) {
		case ropNoOper:
			return true;
		case ropCopyPut:
			break;
		case ropSrcCopy:
			if ( X(image)->flags.layered)
				break;
		default:
			goto FALLBACK;
	}

	return apc_gp_stretch_image_xrender(self, image, dst[src],
		dst_x, dst_y, src_x, src_y,
		dst_w, dst_h, src_w, src_h,
		rop, use_matrix);

FALLBACK:
	return apc_gp_stretch_image_x11(self,image,
		dst_x, dst_y, src_x, src_y,
		dst_w, dst_h, src_w, src_h,
		rop, use_matrix);
}

Bool
apc_application_get_bitmap( Handle self, Handle image, int x, int y, int xLen, int yLen)
{
	DEFXX;
	Bool inPaint = opt_InPaint, ret = false;
	XImage * i;
	XErrorEvent xr;

	if ( !image || PObject(image)-> stage == csDead) return false;

	XFLUSH;

	/* rect validation - questionable but without it the request may be fatal ( by BadMatch) */
	if ( x < 0) x = 0;
	if ( y < 0) y = 0;
	if ( x + xLen > XX-> size. x) xLen = XX-> size. x - x;
	if ( y + yLen > XX-> size. y) yLen = XX-> size. y - y;
	if ( xLen <= 0 || yLen <= 0) return false;

#ifdef WITH_COCOA
	if ( guts. use_quartz && prima_cocoa_is_x11_local()) {
		uint32_t *pixels;
		if ( PImage(image)->type != imRGB)
			CImage( image)-> create_empty( image, xLen, yLen, imRGB);
		if (( pixels = prima_cocoa_application_get_bitmap(
			x, XX->size.y - y - yLen, xLen, yLen, XX->size.y
		))) {
			int y;
			Byte *src = (Byte*) (pixels + xLen * (yLen - 1));
			Byte *dst = PImage(image)->data;
			for ( y = 0; y < yLen; y++, src -= xLen * 4, dst += PImage(image)->lineSize)
				bc_bgri_rgb(src, dst, xLen);
			free(pixels);
			return true;
		}
	}
#endif

	CImage( image)-> create_empty( image, xLen, yLen, guts. qdepth);

	if ( !inPaint) {
		if ( !apc_application_begin_paint( self)) goto PREFAIL;
	}

	prima_save_xerror_event( &xr);
	if ( guts. idepth == 1)
		i = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y - y - yLen, xLen, yLen, 1, XYPixmap);
	else
		i = XGetImage( DISP, XX-> gdrawable, x, XX-> size.y - y - yLen, xLen, yLen, AllPlanes, ZPixmap);
	XCHECKPOINT;

	if ( i) {
		if ( !( ret = prima_query_image( image, i)))
			warn("UAI_017: unsupported depths combination");
		XDestroyImage( i);
	}

	if ( !inPaint) apc_application_end_paint( self);
	if (ret) bzero( &xr, sizeof(xr));
	prima_restore_xerror_event( &xr);

PREFAIL:
#ifdef WITH_GTK
	if ( !ret && guts. use_gtk )
		ret = prima_gtk_application_get_bitmap( self, image, x, y, xLen, yLen);
#endif

	return ret;
}

