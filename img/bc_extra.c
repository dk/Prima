#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct valid_image_type {
	int type;
	int newtype;
	void *from_proc;
	void *to_proc;
} valid_image_types[] =
{
	{ imbpp24 | imColor | imfmtBGR,      imRGB, (void*)cm_reverse_palette, (void*)cm_reverse_palette},
	{ imbpp32 | imColor | imfmtRGBI,     imRGB, (void*)bc_rgbi_rgb, (void*)bc_rgb_rgbi },
	{ imbpp32 | imColor | imfmtIRGB,     imRGB, (void*)bc_irgb_rgb, (void*)bc_rgb_irgb },
	{ imbpp32 | imColor | imfmtBGRI,     imRGB, (void*)bc_bgri_rgb, (void*)bc_rgb_bgri },
	{ imbpp32 | imColor | imfmtIBGR,     imRGB, (void*)bc_ibgr_rgb, (void*)bc_rgb_ibgr },
};

Bool
itype_importable( int type, int *newtype, void **from_proc, void **to_proc)
{
	int i;

	for (i=0; i < sizeof(valid_image_types)/sizeof(struct valid_image_type); i++)
		if ( valid_image_types[i]. type == type) {
			if (newtype) *newtype =        valid_image_types[i]. newtype;
			if (from_proc) *from_proc =    valid_image_types[i]. from_proc;
			if (to_proc) *to_proc =        valid_image_types[i]. to_proc;
			return true;
		}
	return false;
}


static void
memcpy_bitconvproc( Byte * src, Byte * dest, int count)
{
	memcpy( dest, src, count);
}


void
ibc_repad( Byte * source, Byte * dest, int srcLineSize, int dstLineSize, int srcDataSize, int dstDataSize, int srcBpp, int dstBpp, void * convProc, Bool reverse)
{
	int sb  = srcLineSize / srcBpp;
	int db  = dstLineSize / dstBpp;
	int bsc = sb > db ? db : sb;
	int sh  = srcDataSize / srcLineSize;
	int dh  = dstDataSize / dstLineSize;
	int  h  = sh > dh ? dh : sh;

	if ( convProc == NULL) {
		convProc = (void*)memcpy_bitconvproc;
		srcBpp = dstBpp = 1;
		sb = srcLineSize;
		db = dstLineSize;
		bsc = sb > db ? db : sb;
	}

	if ( reverse) {
		dest += dstLineSize * ( h - 1);
		for ( ; h > 0; h--, source += srcLineSize, dest -= dstLineSize)
			(( PSimpleConvProc) convProc)( source, dest, bsc);
	} else {
		for ( ; h > 0; h--, source += srcLineSize, dest += dstLineSize)
			(( PSimpleConvProc) convProc)( source, dest, bsc);
	}

	sb = srcDataSize % srcLineSize / srcBpp;
	db = dstDataSize % dstLineSize / dstBpp;
	(( PSimpleConvProc) convProc)( source, dest, sb > db ? db : sb);
}

void
bc_rgb_rgbi( register Byte * source, register Byte * dest, register int count)
{
	dest   += count * 4 - 1;
	source += count * 3 - 1;
	while ( count--)
	{
		*dest-- = 0;
		*dest-- = *source--;
		*dest-- = *source--;
		*dest-- = *source--;
	}
}

void
bc_rgbi_rgb( register Byte * source, register Byte * dest, register int count)
{
	while ( count--)
	{
		*dest++ = *source++;
		*dest++ = *source++;
		*dest++ = *source++;
		source++;
	}
}


void
bc_rgb_irgb( register Byte * source, register Byte * dest, register int count)
{
	dest   += count * 4 - 1;
	source += count * 3 - 1;
	while ( count--)
	{
		*dest-- = *source--;
		*dest-- = *source--;
		*dest-- = *source--;
		*dest-- = 0;
	}
}

void
bc_irgb_rgb( register Byte * source, register Byte * dest, register int count)
{
	while ( count--)
	{
		source++;
		*dest++ = *source++;
		*dest++ = *source++;
		*dest++ = *source++;
	}
}

void
bc_bgri_rgb( register Byte * source, register Byte * dest, register int count)
{
	while ( count--)
	{
		register Byte a, b;
		a = *source++;
		b = *source++;
		*dest++ = *source++;
		*dest++ = b;
		*dest++ = a;
		source++;
	}
}

void
bc_rgb_bgri( register Byte * source, register Byte * dest, register int count)
{
	dest   += count * 4 - 1;
	source += count * 3 - 1;
	while ( count--)
	{
		register Byte a = *source--;
		register Byte b = *source--;
		*dest--   = 0;
		*dest-- = *source--;
		*dest-- = b;
		*dest-- = a;
	}
}

void
bc_ibgr_rgb( register Byte * source, register Byte * dest, register int count)
{
	while ( count--)
	{
		register Byte a, b;
		source++;
		a = *source++;
		b = *source++;
		*dest++ = *source++;
		*dest++ = b;
		*dest++ = a;
	}
}

void
bc_rgb_ibgr( register Byte * source, register Byte * dest, register int count)
{
	dest   += count * 4 - 1;
	source += count * 3 - 1;
	while ( count--)
	{
		register Byte a = *source--;
		register Byte b = *source--;
		*dest-- = *source--;
		*dest-- = b;
		*dest-- = a;
		*dest-- = 0;
	}
}

void bc_rgba_rgb_a( register Byte * rgba_source, register Byte * rgb_dest, register Byte * a_dest, register int count)
{
	while (count--) {
		*rgb_dest++ = *rgba_source++;
		*rgb_dest++ = *rgba_source++;
		*rgb_dest++ = *rgba_source++;
		*a_dest++ = *rgba_source++;
	}
}

void bc_rgba_bgr_a( register Byte * rgba_source, register Byte * bgr_dest, register Byte * a_dest,    register int count)
{
	while (count--) {
		register Byte r, g;
		r = *rgba_source++;
		g = *rgba_source++;
		*bgr_dest++ = *rgba_source++;
		*bgr_dest++ = g;
		*bgr_dest++ = r;
		*a_dest++ = *rgba_source++;
	}
}

void bc_rgb_a_rgba( register Byte * rgb_source,  register Byte * a_source, register Byte * rgba_dest, register int count)
{
	while (count--) {
		*rgba_dest++ = *rgb_source++;
		*rgba_dest++ = *rgb_source++;
		*rgba_dest++ = *rgb_source++;
		*rgba_dest++ = *a_source++;
	}
}

void bc_bgr_a_rgba( register Byte * bgr_source,  register Byte * a_source, register Byte * rgba_dest, register int count)
{
	while (count--) {
		register Byte r, g;
		r = *bgr_source++;
		g = *bgr_source++;
		*rgba_dest++ = *bgr_source++;
		*rgba_dest++ = g;
		*rgba_dest++ = r;
		*rgba_dest++ = *a_source++;
	}
}


#ifdef __cplusplus
}
#endif

