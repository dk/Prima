#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif



#define var (( PImage) self)

/* Color mappers */
#define BCPARMS      self, dstData, dstPal, dstType, dstPalSize, palSize_only
#define BCSELFGRAY   self, var->data, dstPal, imByte, dstPalSize, palSize_only

static void
ic_Byte_convert( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only, Bool inplace)
{
	int new_data_size = LINE_SIZE(var->w, 8) * var->h;
	Byte * new_data;
	RGBColor dummy_pal[256];
	int dummy_pal_size = 0;

	if ( !inplace) {
		new_data = allocb( new_data_size);
		if ( !new_data) {
			croak("Not enough memory:%d bytes", new_data_size);
			return;
		}
		memset( new_data, 0, new_data_size);
	} else
		new_data = var-> data;
	ic_type_convert( self, new_data, dummy_pal, imByte, &dummy_pal_size, false);
	if ( !inplace) {
		free( var-> data);
		var-> data = new_data;
	}
	var-> type = imByte;
	var-> dataSize = new_data_size;
	var-> lineSize = new_data_size / var-> h;
	memcpy( var-> palette, std256gray_palette, sizeof(std256gray_palette));
	var-> palSize = 256;
	ic_type_convert( self, dstData, dstPal, dstType, dstPalSize, palSize_only);
}

static void
ic_raize_palette( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)
{
	Byte * odata = var-> data;
	int otype = var-> type, dummy_pal_size = 0, oconv = var->conversion;
	var->conversion = ictNone;
	ic_type_convert( self, dstData, var-> palette, dstType, &dummy_pal_size, false);
	var-> palSize = dummy_pal_size;
	var-> type = dstType;
	var-> data = dstData;
	ic_type_convert( self, dstData, dstPal, dstType, dstPalSize, palSize_only);
	var-> data = odata;
	var-> type = otype;
	var-> conversion = oconv;
}

void
ic_type_convert( Handle self, Byte * dstData, PRGBColor dstPal, int dstType, int * dstPalSize, Bool palSize_only)
{
	int srcType = var->type;
	int orgDstType = dstType;

	/* remove redundant combinations */
	switch( srcType)
	{
		case imBW:
		case im16  + imGrayScale:
		case imRGB + imGrayScale:
			srcType &=~ imGrayScale;
			break;
	}

	switch( dstType)
	{
		case imBW:
		case im16  + imGrayScale:
		case imRGB + imGrayScale:
			dstType &=~ imGrayScale;
			break;
	}

	/* fill grayscale palette, if any */
	if ( orgDstType & imGrayScale) {
		/* XXX check ictpUnoptimized on dst grayscales */
		switch( orgDstType & imBPP) {
		case imbpp1:
			memcpy( dstPal, stdmono_palette, sizeof( stdmono_palette));
			*dstPalSize = 2;
			break;
		case imbpp4:
			memcpy( dstPal, std16gray_palette, sizeof( std16gray_palette));
			*dstPalSize = 16;
			break;
		case imbpp8:
			memcpy( dstPal, std256gray_palette, sizeof( std256gray_palette));
			*dstPalSize = 256;
			break;
		}
	} else if (
		(( var->conversion & ictpMask ) == ictpCubic) &&
		(( orgDstType & imBPP) <= (srcType & imBPP)) &&
		(*dstPalSize == 0 || palSize_only)
	) {
		palSize_only = false;
		switch( orgDstType & imBPP) {
		case imbpp1:
			memcpy( dstPal, stdmono_palette, sizeof( stdmono_palette));
			*dstPalSize = 2;
			break;
		case imbpp4:
			memcpy( dstPal, cubic_palette16, sizeof( cubic_palette16));
			*dstPalSize = 16;
			break;
		case imbpp8:
			memcpy( dstPal, cubic_palette, sizeof( cubic_palette));
			*dstPalSize = 216;
			break;
		}
	}

	/* no palette conversion, same type, - out */
	if (
		srcType == dstType && (
			*dstPalSize == 0 || (
				(srcType != imbpp1) &&
				(srcType != imbpp4) &&
				(srcType != imbpp8)
			)
		)
	) {
		memcpy( dstData, var->data, var->dataSize);
		if (( orgDstType & imGrayScale) == 0) {
			memcpy( dstPal, var->palette, var->palSize * 3);
			*dstPalSize = var-> palSize;
		}
		return;
	}

#define CASE_CONVERSION(src,dst,ictc) \
	case ict##ictc: \
		ic_##src##_##dst##_ict##ictc(BCPARMS); \
		break
#define CASE_COLOR_BYTE_CONVERT \
	case imMono:\
	case im16:\
	case im256:\
	case imRGB:\
		ic_Byte_convert( BCPARMS, true);\
		break
#define CASE_NUMERIC_BYTE_CONVERT \
	case imShort: \
	case imLong: \
	case imFloat: \
	case imDouble: \
	case imComplex: \
	case imDComplex: \
		ic_Byte_convert( BCPARMS, false); \
		break;

	switch( srcType)
	{
		case imMono: switch( dstType)
		{
			case imMono:
				switch ( var->conversion)
				{
					case ictPosterization:
						if ( palSize_only || *dstPalSize == 0) {
							if ( dstData != var->data)    memcpy( dstData, var->data, var->dataSize);
							if ( dstPal  != var->palette) memcpy( dstPal, var->palette, var->palSize * 3);
							*dstPalSize = var-> palSize;
							return;
						}
					case ictNone:
					case ictOrdered:
					case ictErrorDiffusion:
						ic_mono_mono_ictNone(BCPARMS);
						break;
					CASE_CONVERSION(mono,mono,Optimized);
				}
				break;
			case im16:
				if ( *dstPalSize > 0)
					ic_raize_palette(BCPARMS);
				else
					ic_mono_nibble_ictNone(BCPARMS);
				break;
			case im256:
				if ( *dstPalSize > 0)
					ic_raize_palette(BCPARMS);
				else
					ic_mono_byte_ictNone(BCPARMS);
				break;
			case imByte:
				ic_mono_graybyte_ictNone(BCPARMS);
				break;
			case imRGB:
				ic_mono_rgb_ictNone(BCPARMS);
				break;
			CASE_NUMERIC_BYTE_CONVERT;
		}
		break; /* imMono */

		case im16: switch( dstType)
		{
			case imMono:
				switch ( var->conversion)
				{
					case ictPosterization:
						ic_nibble_mono_ictNone(BCPARMS);
						break;
					CASE_CONVERSION(nibble, mono, None);
					CASE_CONVERSION(nibble, mono, Optimized);
					CASE_CONVERSION(nibble, mono, Ordered);
					CASE_CONVERSION(nibble, mono, ErrorDiffusion);
				}
				break;
			case im16:
				switch ( var->conversion)
				{
					CASE_CONVERSION(nibble, nibble, None);
					CASE_CONVERSION(nibble, nibble, Posterization);
					CASE_CONVERSION(nibble, nibble, Optimized);
					CASE_CONVERSION(nibble, nibble, Ordered);
					CASE_CONVERSION(nibble, nibble, ErrorDiffusion);
				}
				break;
			case im256:
				if ( *dstPalSize > 0)
					ic_raize_palette(BCPARMS);
				else
					ic_nibble_byte_ictNone(BCPARMS);
				break;
			case imByte:
				ic_nibble_graybyte_ictNone(BCPARMS);
				break;
			case imRGB:
				ic_nibble_rgb_ictNone(BCPARMS);
				break;
			CASE_NUMERIC_BYTE_CONVERT;
		}
		break; /* im16 */

		case im256: switch( dstType)
		{
			case imMono:
				switch ( var->conversion)
				{
					case ictPosterization:
						ic_byte_mono_ictNone(BCPARMS);
						break;
					CASE_CONVERSION(byte, mono, None);
					CASE_CONVERSION(byte, mono, Optimized);
					CASE_CONVERSION(byte, mono, Ordered);
					CASE_CONVERSION(byte, mono, ErrorDiffusion);
				}
				break;
			case im16:
				switch ( var->conversion)
				{
					CASE_CONVERSION(byte, nibble, None);
					CASE_CONVERSION(byte, nibble, Posterization);
					CASE_CONVERSION(byte, nibble, Optimized);
					CASE_CONVERSION(byte, nibble, Ordered);
					CASE_CONVERSION(byte, nibble, ErrorDiffusion);
				}
				break;
			case im256:
				switch ( var->conversion)
				{
					CASE_CONVERSION(byte, byte, None);
					CASE_CONVERSION(byte, byte, Posterization);
					CASE_CONVERSION(byte, byte, Optimized);
					CASE_CONVERSION(byte, byte, Ordered);
					CASE_CONVERSION(byte, byte, ErrorDiffusion);
				}
				break;
			case imByte:
				ic_byte_graybyte_ictNone(BCPARMS);
				break;
			case imRGB:
				ic_byte_rgb_ictNone(BCPARMS);
				break;
			CASE_NUMERIC_BYTE_CONVERT;
		}
		break; /* im256 */

		case imByte: switch( dstType)
		{
			case imMono:
				switch ( var->conversion)
				{
					case ictPosterization:
						ic_byte_mono_ictNone(BCPARMS);
						break;
					CASE_CONVERSION(byte, mono, None);
					CASE_CONVERSION(byte, mono, Optimized);
					CASE_CONVERSION(graybyte, mono, Ordered);
					CASE_CONVERSION(graybyte, mono, ErrorDiffusion);
				}
				break;
			case im16:
				switch ( var->conversion)
				{
					CASE_CONVERSION(byte, nibble, None);
					CASE_CONVERSION(byte, nibble, Posterization);
					CASE_CONVERSION(byte, nibble, Optimized);
					CASE_CONVERSION(graybyte, nibble, Ordered);
					CASE_CONVERSION(graybyte, nibble, ErrorDiffusion);
				}
				break;
			case im256:
				switch ( var->conversion)
				{
					CASE_CONVERSION(byte, byte, None);
					CASE_CONVERSION(byte, byte, Posterization);
					CASE_CONVERSION(byte, byte, Optimized);
					CASE_CONVERSION(byte, byte, Ordered);
					CASE_CONVERSION(byte, byte, ErrorDiffusion);
				}
				break;
			case imRGB:
				ic_graybyte_rgb_ictNone(BCPARMS);
				break;
			case imShort:
				ic_Byte_Short( BCPARMS);
				break;
			case imLong:
				ic_Byte_Long( BCPARMS);
				break;
			case imFloat:
				ic_Byte_float( BCPARMS);
				break;
			case imDouble:
				ic_Byte_double( BCPARMS);
				break;
			case imComplex:
				ic_Byte_float_complex(BCPARMS);
				break;
			case imDComplex:ic_Byte_double_complex(BCPARMS);
				break;
		}
		break; /* imByte */

		case imShort:  switch ( dstType)
		{
			CASE_COLOR_BYTE_CONVERT;
			case imByte   : ic_Short_Byte( BCPARMS);   break;
			case imLong   : ic_Short_Long( BCPARMS);   break;
			case imFloat  : ic_Short_float( BCPARMS);  break;
			case imDouble : ic_Short_double( BCPARMS); break;
			case imComplex: ic_Short_float_complex(BCPARMS); break;
			case imDComplex: ic_Short_double_complex(BCPARMS); break;
		}
		break;
		/* imShort */

		case imLong:  switch ( dstType)
		{
			CASE_COLOR_BYTE_CONVERT;
			case imByte   : ic_Long_Byte( BCPARMS);   break;
			case imShort  : ic_Long_Short( BCPARMS);  break;
			case imFloat  : ic_Long_float( BCPARMS);  break;
			case imDouble : ic_Long_double( BCPARMS); break;
			case imComplex: ic_Long_float_complex(BCPARMS); break;
			case imDComplex: ic_Long_double_complex(BCPARMS); break;
		}
		break;
		/* imLong */

		case imFloat:  switch ( dstType)
		{
			CASE_COLOR_BYTE_CONVERT;
			case imByte   : ic_float_Byte( BCPARMS);   break;
			case imShort  : ic_float_Short( BCPARMS);  break;
			case imLong   : ic_float_Long( BCPARMS);   break;
			case imDouble : ic_float_double( BCPARMS); break;
			case imComplex: ic_float_float_complex(BCPARMS); break;
			case imDComplex: ic_float_double_complex(BCPARMS); break;
		}
		break;
		/* imFloat */


		case imDouble:  switch ( dstType)
		{
			CASE_COLOR_BYTE_CONVERT;
			case imByte   : ic_double_Byte( BCPARMS);   break;
			case imShort  : ic_double_Short( BCPARMS);  break;
			case imLong   : ic_double_Long( BCPARMS);   break;
			case imFloat  : ic_double_float( BCPARMS);  break;
			case imComplex: ic_double_float_complex(BCPARMS); break;
			case imDComplex: ic_double_double_complex(BCPARMS); break;
		}
		break;
		/* imDouble */

		case imRGB: switch( dstType)
		{
			case imMono:
				switch ( var->conversion)
				{
					CASE_CONVERSION(rgb, mono, None);
					CASE_CONVERSION(rgb, mono, Posterization);
					CASE_CONVERSION(rgb, mono, Optimized);
					CASE_CONVERSION(rgb, mono, Ordered);
					CASE_CONVERSION(rgb, mono, ErrorDiffusion);
				}
				break;
			case im16:
				switch ( var->conversion)
				{
					CASE_CONVERSION(rgb, nibble, None);
					CASE_CONVERSION(rgb, nibble, Posterization);
					CASE_CONVERSION(rgb, nibble, Optimized);
					CASE_CONVERSION(rgb, nibble, Ordered);
					CASE_CONVERSION(rgb, nibble, ErrorDiffusion);
				}
				break;
			case im256:
				switch ( var->conversion)
				{
					CASE_CONVERSION(rgb, byte, None);
					CASE_CONVERSION(rgb, byte, Posterization);
					CASE_CONVERSION(rgb, byte, Optimized);
					CASE_CONVERSION(rgb, byte, Ordered);
					CASE_CONVERSION(rgb, byte, ErrorDiffusion);
					break;
				}
				break;
			case imByte:
				ic_rgb_graybyte_ictNone(BCPARMS);
				break;
			CASE_NUMERIC_BYTE_CONVERT;
		}
		break; /* imRGB */

		case imComplex: switch( dstType) {
			CASE_COLOR_BYTE_CONVERT;
			case imByte:    ic_float_complex_Byte(BCPARMS); break;
			case imShort:   ic_float_complex_Short(BCPARMS); break;
			case imLong:    ic_float_complex_Long(BCPARMS); break;
			case imDouble:  ic_float_complex_double(BCPARMS); break;
			case imFloat:   ic_float_complex_float( BCPARMS); break;
		}
		break;
		/* imComplex */

		case imDComplex: switch( dstType) {
			CASE_COLOR_BYTE_CONVERT;
			case imByte:    ic_double_complex_Byte(BCPARMS); break;
			case imShort:   ic_double_complex_Short(BCPARMS); break;
			case imLong:    ic_double_complex_Long(BCPARMS); break;
			case imDouble:  ic_double_complex_double(BCPARMS); break;
			case imFloat:   ic_double_complex_float( BCPARMS); break;
		}
		break;
		/* imDComplex */
	}
}

static int imTypes[] = {
	imbpp1, imbpp1|imGrayScale,
	imbpp4, imbpp4|imGrayScale,
	imbpp8, imbpp8|imGrayScale,
	imRGB,
	imShort, imLong, imFloat, imDouble,
	imComplex, imDComplex, imTrigComplex, imTrigDComplex,
	-1
};

Bool
itype_supported( int type)
{
	int i = 0;
	while( imTypes[i] != type && imTypes[i] != -1) i++;
	return imTypes[i] != -1;
}

static int imConversions[] = {
	ictNone,
	ictPosterization,
	ictOrdered,
	ictErrorDiffusion,
	ictOptimized,
	-1
};

Bool
iconvtype_supported( int conv)
{
	int i = 0;
	while( imConversions[i] != conv && imConversions[i] != -1) i++;
	return imConversions[i] != -1;
}

void
init_image_support(void)
{
	cm_init_colormap();
}

#ifdef __cplusplus
}
#endif
