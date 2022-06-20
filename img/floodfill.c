#include "img_conv.h"
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	PIcon         i;
	Rect          clip;
	int           y, bpp, bytes;
	Byte   *      color;
	Bool          single_border;
	int           first;
	PList  *      lists;
	PBoxRegionRec new_region;
	int           new_region_size;
} FillSession;

static Bool
fs_get_pixel( FillSession * fs, int x, int y)
{
	Byte * data;


	if ( x < fs-> clip. left || x > fs-> clip. right || y < fs-> clip. bottom || y > fs-> clip. top)
		return false;

	if ( fs-> lists[ y - fs-> first]) {
		PList l = fs-> lists[ y - fs-> first];
		int i;
		for ( i = 0; i < l-> count; i+=2) {
			if (((int) l-> items[i+1] >= x) && ((int)l->items[i] <= x))
				return false;
		}
	}

	data = fs->i->data + fs->i->lineSize * y;

	switch( fs-> bpp) {
	case 1: {
		Byte xz = *(data + (x >> 3));
		Byte v  = ( xz & ( 0x80 >> ( x & 7)) ? 1 : 0);
		return fs-> single_border ?
			( v == *(fs-> color)) : ( v != *(fs-> color));
	}
	case 4: {
		Byte xz = *(data + (x >> 1));
		Byte v  = (x & 1) ? ( xz & 0xF) : ( xz >> 4);
		return fs-> single_border ?
			( v == *(fs-> color)) : ( v != *(fs-> color));
	}
	case 8:
		return fs-> single_border ?
			( *(fs-> color) == *(data + x) ):
			( *(fs-> color) != *(data + x) );
	case 16:
		return fs-> single_border ?
			( *((uint16_t*)(fs-> color)) == *((uint16_t*)data + x) ) :
			( *((uint16_t*)(fs-> color)) != *((uint16_t*)data + x) );
	case 32:
		return fs-> single_border ?
			( *((uint32_t*)(fs-> color)) == *((uint32_t*)data + x) ) :
			( *((uint32_t*)(fs-> color)) != *((uint32_t*)data + x) );
	default: {
		return fs-> single_border ?
			( memcmp(data + x * fs->bytes, fs->color, fs->bytes) == 0) :
			( memcmp(data + x * fs->bytes, fs->color, fs->bytes) != 0);
	}}
}

static void
fs_hline( FillSession * fs, int x1, int y, int x2)
{
	y -= fs-> first;
	if ( fs-> lists[y] == NULL)
		fs-> lists[y] = plist_create( 32, 128);
	list_add( fs-> lists[y], ( Handle) x1);
	list_add( fs-> lists[y], ( Handle) x2);
}

static int
fs_fill( FillSession * fs, int sx, int sy, int d, int pxl, int pxr)
{
	int x, xr = sx;
	while ( sx > fs-> clip. left  && fs_get_pixel( fs, sx - 1, sy)) sx--;
	while ( xr < fs-> clip. right && fs_get_pixel( fs, xr + 1, sy)) xr++;
	fs_hline( fs, sx, sy, xr);

	if ( sy + d >= fs-> clip. bottom && sy + d <= fs-> clip. top) {
		x = sx;
		while ( x <= xr) {
			if ( fs_get_pixel( fs, x, sy + d))
				x = fs_fill( fs, x, sy + d, d, sx, xr);
			x++;
		}
	} 

	if ( sy - d >= fs-> clip. bottom && sy - d <= fs-> clip. top) {
		x = sx;
		while ( x < pxl) {
			if ( fs_get_pixel( fs, x, sy - d))
				x = fs_fill( fs, x, sy - d, -d, sx, xr);
			x++;
		}
		x = pxr;
		while ( x <= xr) {
			if ( fs_get_pixel( fs, x, sy - d))
				x = fs_fill( fs, x, sy - d, -d, sx, xr);
			x++;
		}
	}
	return xr;
}

static Bool
fs_intersect( int x1, int y, int w, int h, FillSession * fs)
{
	PList l;
	Handle * items;
	int i, j, x2;

	x2 = x1 + w - 1;
	for ( i = 0; i < h; i++) 
		if (( l = fs-> lists[y + i - fs->first]) != NULL )
			for ( j = 0, items = l->items; j < l-> count; j+=2) {
				Box * box;
				int left  = (int) (*(items++));
				int right = (int) (*(items++));
				if ( left < x1 )
					left = x1;
				if ( right > x2)
					right = x2;
				if ( left > right )
					continue;
				if ( fs-> new_region-> n_boxes >= fs-> new_region_size ) {
					PBoxRegionRec n;
					fs-> new_region_size *= 2;
					if ( !( n = img_region_alloc( fs-> new_region, fs-> new_region_size)))
						return false;
					fs-> new_region = n;
				}
				box = fs-> new_region-> boxes + fs-> new_region-> n_boxes;
				box-> x      = left;
				box-> y      = y + i;
				box-> width  = right - left + 1;
				box-> height = 1;
				fs-> new_region-> n_boxes++;
			}

	return true;
}

Bool
img_flood_fill( Handle self, int x, int y, ColorPixel color, Bool single_border, PImgPaintContext ctx)
{
	Bool ok = true;
	Box box;
	FillSession fs;
	
	fs.i             = ( PIcon ) self;
	fs.color         = color;
	fs.bpp           = fs.i->type & imBPP;
	fs.bytes         = fs.bpp / 8;
	fs.single_border = single_border;

	if ( ctx-> region ) {
		Box box = img_region_box( ctx-> region );
		fs. clip. left   = box. x;
		fs. clip. bottom = box. y;
		fs. clip. right  = box. x + box. width - 1;
		fs. clip. top    = box. y + box. height - 1;
	} else {
		fs. clip. left   = 0;
		fs. clip. bottom = 0;
		fs. clip. right  = fs.i->w - 1;
		fs. clip. top    = fs.i->h - 1;
	}

	fs. new_region_size = (fs. clip. top - fs. clip. bottom + 1) * 4;
	if ( !( fs. new_region = img_region_alloc(NULL, fs. new_region_size)))
		return false;

	fs. first = fs. clip. bottom;
	if ( !( fs. lists = malloc(( fs. clip. top - fs. clip. bottom + 1) * sizeof( void*)))) {
		free( fs. new_region );
		return false;
	}
	bzero( fs. lists, ( fs. clip. top - fs. clip. bottom + 1) * sizeof( void*));

	if ( fs_get_pixel( &fs, x, y)) {
		fs_fill( &fs, x, y, -1, x, x);
		ok = img_region_foreach( ctx->region,
			0, 0, fs.i->w, fs.i->h,
			(RegionCallbackFunc*)fs_intersect, &fs
		);
	}

	for ( x = 0; x < fs. clip. bottom - fs. clip. top + 1; x++)
		if ( fs. lists[x])
			plist_destroy( fs.lists[x]);
	free( fs. lists);

	if ( ok ) {
		ctx-> region = fs. new_region;
		box = img_region_box( ctx-> region );
		ok = img_bar( self, box.x, box.y, box.width, box.height, ctx);
	}

	free( fs. new_region);

	return ok;
}


#ifdef __cplusplus
}
#endif
