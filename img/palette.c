#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define map_RGB_gray ((Byte*)std256gray_palette)

Byte     map_stdcolorref    [ 256];
Byte     div51              [ 256];
Byte     div51f             [ 256];
Byte     div17              [ 256];
Byte     mod51              [ 256];
int8_t   mod51f             [ 256];
Byte     mod17mul3          [ 256];
RGBColor cubic_palette      [ 256];
RGBColor cubic_palette8     [   8];
RGBColor cubic_palette16    [  16] =
{
	{0,0,0}, {0,128,128}, {128,0,128}, {0,0,255},
	{128,128,0}, {0,255,0}, {255,0,0}, {85,85,85},
	{170,170,170},{0,255,255},{255,0,255},{128,128,255},
	{255,255,0},{128,255,128},{255,128,128},{255,255,255}
};
RGBColor stdmono_palette    [   2] = {{ 0, 0, 0}, { 0xFF, 0xFF, 0xFF}};
RGBColor std16gray_palette  [  16];
RGBColor std256gray_palette [ 256];
Byte     map_halftone8x8_51 [  64] = {
	0, 38,  9, 47,  2, 40, 11, 50,
	25, 12, 35, 22, 27, 15, 37, 24,
	6, 44,  3, 41,  8, 47,  5, 43,
	31, 19, 28, 15, 34, 21, 31, 18,
	1, 39, 11, 49,  0, 39, 10, 48,
	27, 14, 36, 23, 26, 13, 35, 23,
	7, 46,  4, 43,  7, 45,  3, 42,
	33, 20, 30, 17, 32, 19, 29, 16
};
Byte     map_halftone8x8_64 [  64] = {
	0, 47, 12, 59,  3, 50, 15, 62,
	31, 16, 43, 28, 34, 19, 46, 31,
	8, 55,  4, 51, 11, 58,  7, 54,
	39, 24, 35, 20, 42, 27, 38, 23,
	2, 49, 14, 61,  1, 48, 13, 60,
	33, 18, 45, 30, 32, 17, 44, 29,
	10, 57,  6, 53,  9, 56,  5, 52,
	41, 26, 37, 22, 40, 25, 36, 21
};

void
cm_init_colormap( void)
{
	int i;
	for ( i = 0; i < 256; i++)
	{
		map_RGB_gray[ i * 3] = map_RGB_gray[ i * 3 + 1] = map_RGB_gray[ i * 3 + 2] = i;
		map_stdcolorref[ i] = i;
		div51f[ i] = (float) i / 51.0 + .5;
		mod51f[ i] = (i + 25) % 51 - 25;
		div51[ i] = i / 51;
		div17[ i] = i / 17;
		mod51[ i] = i % 51;
		mod17mul3[ i] = ( i % 17) * 3;
	}
	for ( i = 0; i < 16; i++)
		std16gray_palette[ i]. r = std16gray_palette[ i]. g = std16gray_palette[ i]. b = i * 17;
	{
		int b, g, r;
		for ( b = 0; b < 6; b++) for ( g = 0; g < 6; g++) for ( r = 0; r < 6; r++)
		{
/*       cubic_palette[ b + g * 6 + r * 36] =( RGBColor) { b * 51, g * 51, r *
*       51 }; */
			int idx = b + g * 6 + r * 36;
			cubic_palette[ idx]. b = b * 51;
			cubic_palette[ idx]. g = g * 51;
			cubic_palette[ idx]. r = r * 51;
		}
	}
	{
		int b, g, r;
		for ( b = 0; b < 2; b++) for ( g = 0; g < 2; g++) for ( r = 0; r < 2; r++)
		{
/*       cubic_palette8[ b + g * 2 + r * 4] = ( RGBColor) { b * 255, g * 255,
*       r * 255 }; */
			int idx = b + g * 2 + r * 4;
			cubic_palette8[ idx]. b = b * 255;
			cubic_palette8[ idx]. g = g * 255;
			cubic_palette8[ idx]. r = r * 255;
		}
	}
}

void
cm_reverse_palette( PRGBColor source, PRGBColor dest, int colors)
{
	while( colors--)
	{
		register Byte r = source[0].r;
		register Byte b = source[0].b;
		register Byte g = source[0].g;
		dest[0].r = b;
		dest[0].b = r;
		dest[0].g = g;
		source++;
		dest++;
	}
}

void
cm_squeeze_palette( PRGBColor source, int srcColors, PRGBColor dest, int destColors)
{
	if (( srcColors == 0) || ( destColors == 0)) return;
	if ( srcColors <= destColors)
		memcpy( dest, source, srcColors * sizeof( RGBColor));
	else
	{
		int tolerance = 0;
		int colors    = srcColors;

		PRGBColor buf = allocn( RGBColor, srcColors);
		if (!buf) return;
		memcpy( buf, source, srcColors * sizeof( RGBColor));
		while (1)
		{
			int i;
			int tt2 = tolerance*tolerance;

			for ( i = 0; i < colors - 1; i++)
			{
				register int r = buf[i]. r;
				register int g = buf[i]. g;
				register int b = buf[i]. b;
				int j;
				register PRGBColor next = buf + i + 1;

				for ( j = i + 1; j < colors; j++)
				{
					if (( ( next-> r - r)*( next-> r - r) +
							( next-> g - g)*( next-> g - g) +
							( next-> b - b)*( next-> b - b)) <= tt2)
					{
						buf[ j] = buf[ --colors];
						if ( colors <= destColors) goto Enough;
					}
					next++;
				}
			}
			tolerance += 2;
		}
Enough:
		memcpy( dest, buf, destColors * sizeof( RGBColor));
		free( buf);
	}
}

Byte
cm_nearest_color( RGBColor color, int palSize, PRGBColor palette)
{
	int diff = INT_MAX, cdiff = 0;
	Byte ret = 0;
	while( palSize--)
	{
		int dr=abs( (int)color. r - (int)palette[ palSize]. r),
			dg=abs( (int)color. g - (int)palette[ palSize]. g),
			db=abs( (int)color. b - (int)palette[ palSize]. b);
		cdiff=dr*dr+dg*dg+db*db;
		if ( cdiff < diff)
		{
			ret = palSize;
			diff = cdiff;
			if ( cdiff == 0) break;
		}
	}
	return ret;
}

void
cm_fill_colorref( PRGBColor fromPalette, int fromColorCount, PRGBColor toPalette, int toColorCount, Byte * colorref)
{

	while( fromColorCount--) {
		RGBColor x = fromPalette[fromColorCount]; /* don't optimize this away, register reading reads past the array bounds */
		colorref[ fromColorCount] =
			cm_nearest_color( x, toColorCount, toPalette);
	}
}

void
cm_colorref_4to8( Byte * src16, Byte * dst256 )
{
	int i;
	Byte srcx[16];
	if ( src16 == NULL ) {
		for ( i = 0; i < 16; i++ ) srcx[i] = i;
		src16 = srcx;
	}
	if ( dst256 == src16 ) {
		Byte dstx[256];
		for ( i = 0; i < 256; i++)
			dstx[i] = (src16[ i >> 4 ] << 4) | src16[ i & 15 ];
		memcpy( dst256, dstx, 256 );
	} else {
		for ( i = 0; i < 256; i++)
			dst256[i] = (src16[ i >> 4 ] << 4) | src16[ i & 15 ];
	}
}

/*

cm_study_palette scans a RGB palette and builds a special structure that allows
quick mapping of a RGB triplet into palette index. The structure is organized
as a set of 64-cell tables (U16 array) following each other without any special
order except the 1st table:

Table 0   Table 1
[U16 x 64][U16 x 64] ...

so that it forms one chunk of memory.

Each table can reference up to 64 colors, using a table-specific resolution.
In order to map a RGB value to a palette index, one splits 8-bit channel values
into 2-bit index, which makes for exactly 4*4*4=64 address space, and is
basically a color cube.

Each U16 table entry is either a color
index (in which case it is less than 256 ), or a reference to another table, in
which case there's PAL_REF bit is set, and the table referenced by int value,
or PAL_FREE (unoccupied).

For example, the tableset is formed like this:

[ 50, PAL_REF | 1, 60, ... ]
[ 70, 100, ... ]

and RGB=(0x00,0x00,0x00) is stripped to bit mask 0xC0 (bits 6 and 7), so that
index points in the table 1 to cell #0 (value 50). For RGB=(0x00,0x00,0x50) the
stripped bit mask (0x40>>6) points to the cell #1, which is PAL_REF|1.  That
means that one has to switch to table #1, and use bit mask 0x30 (bits 4 and 5),
and read cell #1 there (value 100).

This process can go deep max 3 tables, where the last table uses bits 0 and 1.
However the procedure never allocates more than 256 color cells, because the
input palette is 256 colors max.

At the end of the process there's no PAL_FREE entries, and each is made to hold
a color index to the closest color represented by the palette. This allows quick
mapping also for colors outside the palette, which is the whole point of this
function.

*/
U16 *
cm_study_palette( RGBColor * palette, int pal_size)
{
	RGBColor * org_palette = palette;
	int i, pal2count = 1, pal2size = 64;
	int sz = CELL_SIZE * pal2size;

	U16 * p = malloc( sz * sizeof( U16));
	if ( !p) return NULL;
	for ( i = 0; i < sz; i++) p[i] = PAL_FREE;

	/* Scan for all palette entries. If the cell is empty, assign the color index to it;
		if there is already an index, promote the cell into PAL_REF and realloc the tableset.
		If there's alreay a PAL_REF, just go down and repeat the procesure on the next level */
	for ( i = 0; i < pal_size; i++, palette++) {
		int table = 0, index =
			((palette-> r >> 6) << 4) +
			((palette-> g >> 6) << 2) +
			(palette-> b >> 6);
		int shift = 4;
		while ( 1) {
			if ( p[table + index] & PAL_FREE) {
				/* PAL_FREE -> color index */
				p[table + index] = i;
				break;
			} else if ( p[table + index] & PAL_REF) {
				/* do down one level */
				table = (p[table + index] & ~PAL_REF) * CELL_SIZE;
				index =
					(((palette-> r >> shift) & 3) << 4) +
					(((palette-> g >> shift) & 3) << 2) +
					((palette-> b >> shift) & 3);
				shift -= 2;
			} else {
				/* color index -> PAL_REF */
				U16 old = p[table + index];
				int sub_index, old_sub_index, new_table;

			REPEAT:
				/* realloc */
				if ( pal2count == pal2size) {
					int j, newsz;
					U16 * n;
					newsz = ( pal2size += 64 ) * CELL_SIZE;
					if ( !(n = malloc( newsz * sizeof( U16)))) {
						free( p);
						return NULL;
					}
					memcpy( n, p, sizeof(U16) * sz);
					for ( j = sz; j < newsz; j++) n[j] = PAL_FREE;
					free( p);
					p = n;
					sz = newsz;
				}

				/* this color value */
				sub_index =
					(((palette-> r >> shift) & 3) << 4) +
					(((palette-> g >> shift) & 3) << 2) +
					((palette-> b >> shift) & 3);
				/* the value that was previously assigned - now both are moved
				to the subtable */
				old_sub_index =
					(((org_palette[old].r >> shift) & 3) << 4) +
					(((org_palette[old].g >> shift) & 3) << 2) +
					((org_palette[old].b >> shift) & 3);
				new_table = pal2count * CELL_SIZE;
				p[table + index] = (pal2count++) | PAL_REF;
				if ( sub_index != old_sub_index) {
					/* finally, assign color index */
					p[ new_table + sub_index]     = i;
					p[ new_table + old_sub_index] = old;
					break;
				} else {
					/* both values have the same 6-bit index on this level, go down to the next table */
					if ( shift > 1) {
						shift -= 2;
						table = new_table;
						index = sub_index;
						goto REPEAT;
					}
					/* just a duplicate */
					p[ table + index] = i;
					break;
				}
			}
		}
	}

	/* do cm_nearest_color for each PAL_FREE cell */
	{
		struct {
			int i;
			int table;
			int r;
			int g;
			int b;
		} stack[4]; /* max depth - each channel is 8 bit, but each color cube uses 2 bits, so 8/2=4 levels max */
		int sp = 0;
		memset( stack, 0, sizeof(stack));
		for ( ; stack[sp].i < 64; stack[sp].i++) {
			if ( p[stack[sp].table + stack[sp].i] & PAL_FREE) {
				RGBColor cell;
				int shift = 6 - sp * 2, delta = 32 >> sp * 2;
				cell. r = stack[sp]. r + delta + ((( stack[sp].i >> 4) & 3) << shift);
				cell. g = stack[sp]. g + delta + ((( stack[sp].i >> 2) & 3) << shift);
				cell. b = stack[sp]. b + delta +  (( stack[sp].i & 3) << shift);
				p[stack[sp].table + stack[sp].i] = cm_nearest_color( cell, pal_size, org_palette);
			} else if ( p[stack[sp].table + stack[sp].i] & PAL_REF) {
				int shift = 6 - sp * 2;
				stack[sp + 1].r = stack[sp]. r + ((( stack[sp].i >> 4) & 3) << shift);
				stack[sp + 1].g = stack[sp]. g + ((( stack[sp].i >> 2) & 3) << shift);
				stack[sp + 1].b = stack[sp]. b + (( stack[sp].i & 3) << shift);
				stack[sp + 1].table = (p[stack[sp].table + stack[sp].i] & ~PAL_REF) * CELL_SIZE;
				stack[++sp].i = -1;
			}
			while ( stack[sp].i == 63 && sp > 0) sp--;
		}
	}
	return p;
}

static int
sort_palette( const void * a, const void * b)
{
	register unsigned int A =
		((PRGBColor)a)->r +
		((PRGBColor)a)->g +
		((PRGBColor)a)->b
	;
	register unsigned int B =
		((PRGBColor)b)->r +
		((PRGBColor)b)->g +
		((PRGBColor)b)->b
	;
	if ( A < B)
		return -1;
	else if ( A > B)
		return 1;
	else
		return 0;
}

void
cm_sort_palette( RGBColor * palette, int size)
{
	qsort( palette, size, sizeof(RGBColor), sort_palette);
}

void
cm_reduce_palette4( Byte * srcData, int srcLine, int width, int height, RGBColor * srcPalette, int srcPalSize, RGBColor * dstPalette, int * dstPalSize)
{
	Byte hist[16];
	int i, j, tail, w;

	w = width / 2;
	tail = width % 2;

	memset( hist, 0, sizeof( hist));
	*dstPalSize = 0;
	for ( i = 0; i < height; i++) {
		Byte * d = srcData;
		srcData += srcLine;
		for ( j = 0; j < w; j++, d++) {
			Byte c = *d >> 4;
			if ( hist[c] == 0) {
				hist[c] = 1;
				dstPalette[(*dstPalSize)++] = srcPalette[c];
				if ( *dstPalSize >= srcPalSize) return;
			}
			c = *d & 0x0f;
			if ( hist[c] == 0) {
				hist[c] = 1;
				dstPalette[(*dstPalSize)++] = srcPalette[c];
				if ( *dstPalSize >= srcPalSize) return;
			}
		}
		if (tail) {
			Byte c = *d >> 4;
			if ( hist[c] == 0) {
				hist[c] = 1;
				dstPalette[(*dstPalSize)++] = srcPalette[c];
				if ( *dstPalSize >= srcPalSize) return;
			}
		}
	}
}

void
cm_reduce_palette8( Byte * srcData, int srcLine, int width, int height, RGBColor * srcPalette, int srcPalSize, RGBColor * dstPalette, int * dstPalSize)
{
	Byte hist[256];
	int i, j;

	memset( hist, 0, sizeof( hist));
	*dstPalSize = 0;
	for ( i = 0; i < height; i++) {
		Byte * d = srcData;
		srcData += srcLine;
		for ( j = 0; j < width; j++, d++) {
			if ( hist[*d] == 0) {
				hist[*d] = 1;
				dstPalette[(*dstPalSize)++] = srcPalette[*d];
				if ( *dstPalSize >= srcPalSize) return;
			}
		}
	}
}

/*

cm_optimized_palette scans a RGB image and builds a palette that best represents colors found in the image.

*/

#define MAP1_SIDE  32
#define MAP1_SHIFT  3
#define MAP1_SIDE3 (MAP1_SIDE*MAP1_SIDE*MAP1_SIDE)
#define MAP2_SIDE   8
#define MAP2_SHIFT  3
#define MAP2_MASK   7
#define MAP2_ITEM_SIZE  64
#define MAP2_ITEM_SHIFT 6

Bool
cm_optimized_palette( Byte * data, int lineSize, int width, int height, RGBColor * palette, int * max_pal_size)
{
	int i, j, sz, count, side = MAP1_SIDE, shift = MAP1_SHIFT, force_squeeze = 0, map0index = 0;
	int countB, countL, map2scale = 0;
	Byte * map, * map2;
	RGBColor * big_pal;

	if ( !( map = malloc( MAP1_SIDE3))) return false;

REPEAT_CALC:
	count = 0;
	memset( map, 0, MAP1_SIDE3);

	/* calculate colors with resolution 1 / 512 */
	for ( i = 0; i < height; i++) {
		RGBColor * p = ( RGBColor*)( data + i * lineSize );
		for ( j = 0; j < width; j++, p++) {
			int index = (p-> r >> shift) * side * side +
							(p-> g >> shift) * side +
							(p-> b >> shift);
			if ( map[index] == 0) {
				map[index] = 1;
				count++;
			}
		}
	}

	j = 0;
	sz = side * side * side;
	/* if too many colors, extract only max_pal_size and return */
	if ( count > *max_pal_size) {
		if (( count > 512) && ( side > 8) && !force_squeeze) {
			side >>= 1;
			shift++;
			goto REPEAT_CALC;
		}
		NO_MEMORY:
		{
			int delta = (1 << shift) - 1;
			RGBColor * big_pal = malloc( count * sizeof(RGBColor));
			if ( !big_pal) {
				free( map);
				return false;
			}
			for ( i = 0; i < sz; i++)
				if ( map[i]) {
					big_pal[j]. r = (i / ( side * side)) << shift;
					big_pal[j]. g = ((i / side) % side ) << shift;
					big_pal[j]. b = (i % side) << shift;
					if ( big_pal[j]. r > 127) big_pal[j]. r += delta;
					if ( big_pal[j]. g > 127) big_pal[j]. g += delta;
					if ( big_pal[j]. b > 127) big_pal[j]. b += delta;
					j++;
				}
			cm_squeeze_palette( big_pal, j, palette, *max_pal_size);
			cm_sort_palette( palette, *max_pal_size);
			free( big_pal);
			free( map);
			return true;
		}
	}

	/* scale was lowered due to many colors, but now it is too few colors... */
	if ( side != MAP1_SIDE) {
		force_squeeze = 1;
		side <<= 1;
		shift--;
		goto REPEAT_CALC;
	}


	/* stage 2 - full color calc */
	if (!( map2 = malloc( MAP2_ITEM_SIZE * count))) goto NO_MEMORY;
	memset( map2, 0, MAP2_ITEM_SIZE * count);

	/* calculate colors with full resolution */
	sz = MAP1_SIDE * MAP1_SIDE * MAP1_SIDE;
	count = 0;
	for ( i = 0; i < sz; i++)
		if ( map[i]) {
			if ( count == 0) map0index = i;
			map[i] = count++;
		}
	count = 0;

	for ( i = 0; i < height; i++) {
		RGBColor * p = ( RGBColor*)( data + i * lineSize );
		for ( j = 0; j < width; j++, p++) {
			int index1 = (p-> r >> MAP1_SHIFT) * MAP1_SIDE * MAP1_SIDE +
							(p-> g >> MAP1_SHIFT) * MAP1_SIDE +
							(p-> b >> MAP1_SHIFT);
			int index2 = (p-> r & MAP2_MASK) * MAP2_SIDE * MAP2_SIDE +
							(p-> g & MAP2_MASK) * MAP2_SIDE +
							(p-> b & MAP2_MASK);
			index1 = (map[index1] << MAP2_ITEM_SHIFT) + (index2 >> 3);
			if (( map2[index1] & (1 << (index2 & 7))) == 0) {
				map2[index1] |= (1 << (index2 & 7));
				count++;
			}
		}
	}

	/* Too many colors - if indeed too many, resample with /8 and /64 scale.
		Even /64 scale though can result in up to 4K colors */
	countB = countL = 0;
	if ( count > *max_pal_size) {
		if ( count > *max_pal_size * 2) {
			for ( i = 0; i < sz; i++)
				if ( i == map0index || map[i] != 0) {
					Byte * k = map2 + map[i] * MAP2_ITEM_SIZE;
					U32 * l = ( U32 *) k;
					for ( j = 0; j < MAP2_ITEM_SIZE / 4; j+=2)
						if ( l[j] || l[j+1]) countL++;
					for ( j = 0; j < MAP2_ITEM_SIZE; j++)
						if ( k[j]) countB++;
				}
			if ( countB > *max_pal_size * 2) {
				count = countL;
				map2scale = 2;
			} else {
				count = countB;
				map2scale = 1;
			}
		}
	}

	/* collect final palette */
	if ( count > *max_pal_size) {
		if ( !( big_pal = malloc( count * sizeof(RGBColor)))) {
			free( map);
			free( map2);
			return false;
		}
	} else
		big_pal = palette;

	count = 0;
	for ( i = 0; i < sz; i++)
		if ( i == map0index || map[i] != 0) {
			int r = (i / ( MAP1_SIDE * MAP1_SIDE)) << MAP1_SHIFT;
			int g = ((i / MAP1_SIDE) % MAP1_SIDE ) << MAP1_SHIFT;
			int b = (i % MAP1_SIDE) << MAP1_SHIFT;
			Byte * k = map2 + map[i] * MAP2_ITEM_SIZE;
			switch ( map2scale) {
			case 0: /* 1/1  */
				for ( j = 0; j < 512; j++) {
					if ( k[j >> 3] & ( 1 << ( j & 7))) {
						big_pal[count]. r = r + j / ( MAP2_SIDE * MAP2_SIDE);
						big_pal[count]. g = g + (j / MAP2_SIDE) % MAP2_SIDE;
						big_pal[count]. b = b + j % MAP2_SIDE;
						count++;
					}
				}
				break;
			case 1: /* 1/8  */
				for ( j = 0; j < 64; j++) {
					if ( k[j]) {
						big_pal[count]. r = r + ((j / ( 4 * 4)) << 1);
						big_pal[count]. g = g + (((j / 4) % 4) << 1);
						big_pal[count]. b = b + ((j % 4) << 1);
						if ( big_pal[count]. r > 127) big_pal[count]. r ++;
						if ( big_pal[count]. g > 127) big_pal[count]. g ++;
						if ( big_pal[count]. b > 127) big_pal[count]. b ++;
						count++;
					}
				}
				break;
			case 2: /* 1/64  */
				for ( j = 0; j < 8; j++) {
					if ( *(( U32*) k) || *((( U32*) k) + 1)) {
						big_pal[count]. r = r + ((j / ( 2 * 2)) << 2);
						big_pal[count]. g = g + (((j / 2) % 2) << 2);
						big_pal[count]. b = b + ((j % 2) << 2);
						if ( big_pal[count]. r > 127) big_pal[count]. r += 3;
						if ( big_pal[count]. g > 127) big_pal[count]. g += 3;
						if ( big_pal[count]. b > 127) big_pal[count]. b += 3;
						count++;
					}
					k += 8;
				}
				break;
			}
		}
	if ( big_pal != palette) {
		cm_squeeze_palette( big_pal, count, palette, *max_pal_size);
		count = *max_pal_size;
		free( big_pal);
	}
	free( map);
	free( map2);
	*max_pal_size = count;
	cm_sort_palette( palette, count);
	return true;
}

#ifdef __cplusplus
}
#endif
