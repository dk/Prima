#include "img.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include "apricot.h"
#include "Image.h"
#include "Image_private.h"
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CDrawable->
#define my  ((( PImage) self)-> self)
#define var (( PImage) self)

Handle
Image_convert_to_icon( Handle self, int maskType, SV * mask_fill )
{
	if ( maskType != 1 && maskType != 8 )
		croak("Image.convert_to_icon: maskType must be either 1 or 8");
	return Icon_create_from_image( self, maskType, mask_fill );
}

void
Image_color2pixel( Handle self, Color color, Byte * pixel)
{
	RGBColor rgb;
	rgb.b = color & 0xff;
	rgb.g = ((color)>>8) & 0xff;
	rgb.r = ((color)>>16) & 0xff;

	switch (var->type) {
	case imBW:
		pixel[0] = (int)( (rgb.r + rgb.g + rgb.b) / 768.0 + .5);
		break;
	case imbpp1:
		pixel[0] = cm_nearest_color(rgb,var->palSize,var->palette) & 1;
		break;
	case imbpp4 | imGrayScale :
		pixel[0] = (int)( (rgb.r + rgb.g + rgb.b) / 48.0);
		break;
	case imbpp4  :
		pixel[0] = cm_nearest_color(rgb,var->palSize,var->palette) & 15;
		break;
	case imByte:
		pixel[0] = (int)( (rgb.r + rgb.g + rgb.b) / 3.0);
		break;
	case imbpp8:
		pixel[0] = cm_nearest_color(rgb,var->palSize,var->palette);
		break;
	case imShort :
		if ( color > INT16_MAX ) color = INT16_MAX;
		*((Short*)pixel) = color;
		break;
	case imRGB :
		memcpy( pixel, &rgb, 3);
		break;
	case imLong :
		if ( color > INT32_MAX ) color = INT32_MAX;
		*((Long*)pixel) = color;
		break;
	case imFloat:
		*((float*)pixel) = color;
		break;
	case imDouble:
		*((double*)pixel) = color;
		break;
	case imComplex:
	case imTrigComplex:
		((float*)pixel)[0] = color;
		((float*)pixel)[1] = color;
		break;
	case imDComplex:
	case imTrigDComplex:
		((double*)pixel)[0] = color;
		((double*)pixel)[1] = color;
		break;
	default:
		croak("Not implemented yet");
	}
}

void
Image_create_empty( Handle self, int width, int height, int type)
{
	free( var->data);
	var->w = width;
	var->h = height;
	var->type     = type;
	var->lineSize = LINE_SIZE(var->w, var->type);
	var->dataSize = var->lineSize * var->h;
	var->palSize  = (1 << (var->type & imBPP)) & 0x1ff;
	var->statsCache = 0;
	if ( var->dataSize > 0)
	{
		var->data = allocb( var->dataSize);
		if ( var-> data == NULL) {
			int sz = var-> dataSize;
			my-> make_empty( self);
			croak("Image::create_empty: cannot allocate %d bytes", sz);
		}
		memset( var->data, 0, var->dataSize);
	} else
		var->data = NULL;
	if ( var->type & imGrayScale) switch ( var->type & imBPP)
	{
	case imbpp1:
		memcpy( var->palette, stdmono_palette, sizeof( stdmono_palette));
		break;
	case imbpp4:
		memcpy( var->palette, std16gray_palette, sizeof( std16gray_palette));
		break;
	case imbpp8:
		memcpy( var->palette, std256gray_palette, sizeof( std256gray_palette));
		break;
	}
}


Handle
Image_extract( Handle self, int x, int y, int width, int height)
{
	Handle h;
	PImage i;
	HV * profile;
	unsigned char * data = var->data;
	int ls = var->lineSize;
	int nodata = 0;

	if ( var->w == 0 || var->h == 0) return my->dup( self);
	if ( x < 0) x = 0;
	if ( y < 0) y = 0;
	if ( x >= var->w) x = var->w - 1;
	if ( y >= var->h) y = var->h - 1;
	if ( width  + x > var->w) width  = var->w - x;
	if ( height + y > var->h) height = var->h - y;
	if ( width <= 0 ) {
		warn("Requested image width is less than 1");
		width = 1;
		nodata = 1;
	}
	if ( height <= 0 ) {
		warn("Requested image height is less than 1");
		height = 1;
		nodata = 1;
	}

	profile = newHV();
	pset_H( owner,        var->owner);
	pset_i( width,        width);
	pset_i( height,       height);
	pset_i( type,         var->type);
	pset_i( conversion,   var->conversion);
	pset_i( scaling,      var->scaling);
	pset_i( preserveType, is_opt( optPreserveType));

	h = Object_create( var->self-> className, profile);
	sv_free(( SV *) profile);

	i = ( PImage) h;
	memcpy( i-> palette, var->palette, 768);
	i-> palSize = var-> palSize;
	if (nodata) goto NODATA;

	if (( var->type & imBPP) >= 8) {
		int pixelSize = ( var->type & imBPP) / 8;
		while ( height > 0) {
			height--;
			memcpy( i-> data + height * i-> lineSize,
					data + ( y + height) * ls + pixelSize * x,
					pixelSize * width);
		}
	} else if (( var->type & imBPP) == 4) {
		while ( height > 0) {
			height--;
			bc_nibble_copy( data + ( y + height) * ls, i-> data + height * i-> lineSize, x, width);
		}
	} else if (( var->type & imBPP) == 1) {
		while ( height > 0) {
			height--;
			bc_mono_copy( data + ( y + height) * ls, i-> data + height * i-> lineSize, x, width);
		}
	}
NODATA:
	--SvREFCNT( SvRV( i-> mate));
	return h;
}

void
Image_make_empty( Handle self)
{
	free( var->data);
	free( var->palette);
	var->w = 0;
	var->h = 0;
	var->type     = 0;
	var->palSize  = 0;
	var->lineSize = 0;
	var->dataSize = 0;
	var->data     = NULL;
	var->palette  = NULL;
	my->update_change( self);
}


/*
divide the pixels, by whether they match color or not on two
groups, F and B. Both are converted correspondingly to the settings
of color/backColor and rop/rop2. Possible variations:
rop == rop::NoOper,    pixel value remains the same
rop == rop::CopyPut,   use the color value
rop == rop::Blackness, use black pixel
rop == rop::Whiteness, use white pixel
rop == rop::AndPut   , result is dest & color value
etc...
*/

void
Image_map( Handle self, Color color)
{
	Byte * d, b[2];
	RGBColor c;
	int   type = var-> type, height = var-> h, i, ls;
	int   rop[2];
	RGBColor r[2];
	int bc = 0;
	ImagePreserveTypeRec p;

	if ( var-> data == NULL) return;

	rop[0] = my-> get_rop( self);
	rop[1] = my-> get_rop2( self);
	if ( rop[0] == ropNoOper && rop[1] == ropNoOper) return;

	for ( i = 0; i < 2; i++) {
		int not = 0;

		switch( rop[i]) {
		case ropBlackness:
			r[i]. r = r[i]. g = r[i]. b = 0;
			rop[i] = ropCopyPut;
			break;
		case ropWhiteness:
			r[i]. r = r[i]. g = r[i]. b = 0xff;
			rop[i] = ropCopyPut;
			break;
		case ropNoOper:
			r[i]. r = r[i]. g = r[i]. b = 0;
			break;
		default: {
			Color c = i ? my-> get_backColor( self) : my-> get_color( self);
			r[i]. r = ( c >> 16) & 0xff;
			r[i]. g = ( c >> 8) & 0xff;
			r[i]. b = c & 0xff;
		}}

		if (( type & imBPP) <= 8) {
			b[i] = cm_nearest_color( r[i], var-> palSize, var-> palette);
		}

		switch ( rop[i]) {
		case ropNotPut:
			rop[i] = ropCopyPut; not = 1; break;
		case ropNotSrcXor: /* same as ropNotDestXor and ropNotXor */
			rop[i] = ropXorPut; not = 1; break;
		case ropNotSrcAnd:
			rop[i] = ropAndPut; not = 1; break;
		case ropNotSrcOr:
			rop[i] = ropOrPut; not = 1; break;
		}

		if ( not) {
			r[i]. r = ~ r[i]. r;
			r[i]. g = ~ r[i]. g;
			r[i]. b = ~ r[i]. b;
			b[i]    = ~ b[i];
		}
	}

	c. r = ( color >> 16) & 0xff;
	c. g = ( color >> 8) & 0xff;
	c. b = color & 0xff;
	if (( type & imBPP) <= 8) {
		Color cc;
		bc = cm_nearest_color( c, var-> palSize, var-> palette);
		cc = ARGB( var->palette[bc].r, var->palette[bc].g, var->palette[bc].b);
		if ( cc != color) bc = 0xffff; /* no exact color found */
	}

	my-> begin_preserve_type(self, &p);
	if (
		(( type & imBPP) < 8) ||
		(
			( type != imRGB) &&
			( type != (imRGB | imGrayScale))
		)
		) {
		if ( type & imGrayScale)
			my-> set_type( self, imbpp8 | imGrayScale);
		else
			my-> set_type( self, imbpp8);
	}

	d = ( Byte * ) var-> data;
	ls = var-> lineSize;

	while ( height--) {
		if (( type & imBPP) == 24) {
			PRGBColor data = ( PRGBColor) d;
			for ( i = 0; i < var-> w; i++) {
				int z = ( data-> r == c.r && data-> g == c.g && data-> b == c.b) ? 0 : 1;
				switch( rop[z]) {
				case ropAndPut:
					data-> r &= r[z]. r; data-> g &= r[z]. g; data-> b &= r[z]. b; break;
				case ropXorPut:
					data-> r ^= r[z]. r; data-> g ^= r[z]. g; data-> b ^= r[z]. b; break;
				case ropOrPut:
					data-> r |= r[z]. r; data-> g |= r[z]. g; data-> b |= r[z]. b; break;
				case ropNotDestAnd:
					data-> r = ( ~data-> r) & r[z].r; data-> g = ( ~data-> g) & r[z].g; data-> b = ( ~data-> b) & r[z].b; break;
				case ropNotDestOr:
					data-> r = ( ~data-> r) | r[z].r; data-> g = ( ~data-> g) | r[z].g; data-> b = ( ~data-> b) | r[z].b; break;
				case ropNotAnd:
					data-> r = ~(data-> r & r[z].r); data-> g = ~(data-> g & r[z].g); data-> b = ~(data-> b & r[z].b); break;
				case ropNotOr:
					data-> r = ~(data-> r | r[z].r); data-> g = ~(data-> g | r[z].g); data-> b = ~(data-> b | r[z].b); break;
				case ropNoOper:
					break;
				case ropInvert:
					data-> r = ~data-> r; data-> g = ~data-> g; data-> b = ~data-> b; break;
				default:
					data-> r = r[z]. r; data-> g = r[z]. g; data-> b = r[z]. b;
				}
				data++;
			}
			d += ls;
		} else {
			Byte * data = d;
			for ( i = 0; i < var-> w; i++) {
				int z = ( *data == bc) ? 0 : 1;
				switch( rop[z]) {
				case ropAndPut:
					*data &= b[z]; break;
				case ropXorPut:
					*data ^= b[z]; break;
				case ropOrPut:
					*data |= b[z]; break;
				case ropNotDestAnd:
					*data = (~(*data)) & b[z]; break;
				case ropNotDestOr:
					*data = (~(*data)) | b[z]; break;
				case ropNotAnd:
					*data = ~(*data & b[z]); break;
				case ropNotOr:
					*data = ~(*data | b[z]); break;
				case ropNoOper:
					break;
				case ropInvert:
					*data = ~(*data); break;
				default:
					*data = b[z]; break;
				}
				data++;
			}
			d += ls;
		}
	}

	my-> end_preserve_type(self, &p);
}

static Bool
integral_rotate( Handle self, int degrees)
{
	Byte * new_data = NULL;
	int new_line_size = 0;

	if (( var-> type & imBPP) < 8) {
		Bool ok;
		int type = var->type;
		my->set_type( self, imbpp8 );
		ok = integral_rotate( self, degrees );
		if ( is_opt( optPreserveType)) {
			int conv = var-> conversion;
			my-> set_conversion( self, ictNone);
			my-> set_type( self, type);
			my-> set_conversion( self, conv);
		}
		return ok;
	}

	switch (degrees) {
	case 90:
	case 270:
		new_line_size = LINE_SIZE( var-> h , var->type);
		if (( new_data = allocb( new_line_size * var->w )) == NULL ) {
			warn("Image::rotate: cannot allocate %d bytes", new_line_size * var->w);
			return false;
		}
		break;
	case 180:
		if (( new_data = allocb( var->dataSize )) == NULL ) {
			warn("Image::rotate: cannot allocate %d bytes", var->dataSize );
			return false;
		}
		break;
	default:
		croak("'degrees' must be 90,180,or 270");
	}

	img_integral_rotate( self, new_data, new_line_size, degrees );
	if ( degrees != 180 ) {
		int h = var->h;
		var->h = var->w;
		var->w = h;
		var->lineSize = new_line_size;
		var->dataSize = new_line_size * var->h;
	}
	free( var->data);
	var->data = new_data;

	my-> update_change(self);
	return true;
}

static Bool
generic_rotate( Handle self, float degrees, ColorPixel fill)
{
	Image i;
	int desired_type = var->type;
	NPoint delta = {0,0};

	if (( desired_type & imBPP) <= 8) 
		desired_type = (desired_type & imGrayScale) ? imByte : imRGB;

	if (var->type != desired_type) {
		Bool ok;
		int type = var->type;
		my->set_type( self, desired_type );
		ok = generic_rotate( self, degrees, fill );
		if ( is_opt( optPreserveType)) {
			int conv = var-> conversion;
			my-> set_conversion( self, ictNone);
			my-> set_type( self, type);
			my-> set_conversion( self, conv);
		}
		return ok;
	}

	if (!img_generic_rotate( self, degrees, &i, fill, delta, NULL))
		return false;

	free( var->data);

	var->h        = i. h;
	var->w        = i. w;
	var->lineSize = i. lineSize;
	var->dataSize = i. dataSize;
	var->data     = i. data;

	my-> update_change(self);
	return true;
}

void
Image_mirror( Handle self, Bool vertically)
{
	if (!vertically && ( var-> type & imBPP) < 8) {
		int type = var->type;
		my->set_type( self, imbpp8 );
		my->mirror( self, vertically );
		if ( is_opt( optPreserveType)) {
			int conv = var-> conversion;
			my-> set_conversion( self, ictNone);
			my-> set_type( self, type);
			my-> set_conversion( self, conv );
		}
		return;
	}

	img_mirror( self, vertically );
	my-> update_change(self);
}

void
Image_premultiply_alpha( Handle self, SV * alpha)
{
	ImagePreserveTypeRec p;

	my-> begin_preserve_type(self, &p);
	if ( var-> type & imGrayScale ) {
		if ( var-> type != imByte )
			my-> set_type( self, imByte );
	} else {
		if ( var-> type != imRGB )
			my-> set_type( self, imRGB );
	}

	if ( SvROK( alpha )) {
		Handle a = gimme_the_mate( alpha), dup = NULL_HANDLE;
		if ( !a || !kind_of( a, CImage) || PImage(a)-> w != var-> w || PImage(a)-> h != var-> h )
			croak( "Illegal object reference passed to Prima::Image::%s", "premultiply_alpha");
		if ( PImage(a)->type != imByte)
			a = dup = CImage(a)->dup(a);
		img_premultiply_alpha_map( self, a);
		if (dup)
			Object_destroy(dup);
	} else
		img_premultiply_alpha_constant( self, SvIV( alpha ));

	my-> end_preserve_type(self, &p);
}

void
Image_resample( Handle self, double srcLo, double srcHi, double dstLo, double dstHi)
{
#define RSPARMS self, var->data, var->type, srcLo, srcHi, dstLo, dstHi
	switch ( var->type)
	{
		case imByte:   rs_Byte_Byte     ( RSPARMS); break;
		case imShort:  rs_Short_Short   ( RSPARMS); break;
		case imLong:   rs_Long_Long     ( RSPARMS); break;
		case imFloat:  rs_float_float   ( RSPARMS); break;
		case imDouble: rs_double_double ( RSPARMS); break;
		default: return;
	}
	my->update_change( self);
}

void
Image_reset( Handle self, int new_type, RGBColor * palette, int palSize)
{
	Bool want_palette;
	RGBColor new_palette[256];
	Byte * new_data = NULL;
	int new_pal_size = 0, new_line_size, new_data_size, want_only_palette_colors = 0;

	if ( var->stage > csFrozen) return;
	if (
		palSize   == var->palSize &&
		var->type == new_type     &&
		((palSize > 0 && palette != NULL) ?
			( memcmp( var->palette, palette, palSize * 3) == 0) :
			true
		)
	)
		return;

	want_palette = (!( new_type & imGrayScale)) && ( new_type != imRGB) && (palSize > 0);

	if ( want_palette) {
		new_pal_size = palSize;
		if ( new_pal_size == 0) want_palette = false;
		if ( new_pal_size > ( 1 << ( new_type & imBPP)))
			new_pal_size = 1 << ( new_type & imBPP);
		if ( new_pal_size > 256)
			new_pal_size = 256;
		if ( palette != NULL)
			memcpy( new_palette, palette, new_pal_size * 3);
		else
			want_only_palette_colors = 1;
	}
	if (
		!want_palette && (
			((var->type == (imbpp8|imGrayScale)) && (new_type == imbpp8)) ||
			((var->type == (imbpp4|imGrayScale)) && (new_type == imbpp4)) ||
			((var->type == (imbpp1|imGrayScale)) && (new_type == imbpp1))
		)
	) {
		var->type = new_type;
		return;
	}
	if (( var->conversion & ictpMask) == ictpCubic)
		want_palette = true;
	if (
		var-> type == new_type && (
			(
				new_type != imbpp8 &&
				new_type != imbpp4 &&
				new_type != imbpp1
			) ||
			!want_palette
		)
	)
		return;

	new_line_size = LINE_SIZE(var->w, new_type);
	new_data_size = new_line_size * var-> h;
	if ( new_data_size > 0) {
		if ( !( new_data = allocb( new_data_size))) {
			my-> make_empty( self);
			croak("Image::reset: cannot allocate %d bytes", new_data_size);
		}
		memset( new_data, 0, new_data_size);
		if ( new_pal_size != 1)
			ic_type_convert( self, new_data, new_palette, new_type,
					&new_pal_size, want_only_palette_colors);
	}
	if ( new_pal_size > 0) {
		var-> palSize = new_pal_size;
		memcpy( var-> palette, new_palette, new_pal_size * 3);
	}
	free( var-> data);
	var-> type     = new_type;
	var-> data     = new_data;
	var-> lineSize = new_line_size;
	var-> dataSize = new_data_size;
	my-> update_change( self);
}

Bool
Image_rotate( Handle self, double degrees, SV * svfill)
{
	degrees = fmod(degrees, 360.0);
	if ( degrees < 0 ) degrees += 360.0;

	if ( degrees == 0.0 )
		return true;

	if ( degrees == 90.0 || degrees == 180.0 || degrees == 270.0 )
		return integral_rotate(self, (int)degrees);
	else {
		ColorPixel px;
		bzero(&px, sizeof(px));
		if (svfill != NULL_SV)
			Image_read_pixel(self, svfill, &px);
		return generic_rotate(self, degrees, px);
	}
}

void
Image_stretch( Handle self, int width, int height)
{
	Byte * newData = NULL;
	int lineSize, newType;
	ImagePreserveTypeRec p;

	if ( var->stage > csFrozen) return;
	if ( width  >  65535) width  =  65535;
	if ( height >  65535) height =  65535;
	if ( width  < -65535) width  = -65535;
	if ( height < -65535) height = -65535;
	if (( width == var->w) && ( height == var->h)) return;
	if ( width == 0 || height == 0)
	{
		my->create_empty( self, 0, 0, var->type);
		return;
	}

	my->begin_preserve_type(self, &p);
	newType = ic_stretch_suggest_type( var-> type, var-> scaling );
	if ( newType != var->type)
		my->set_type(self, newType);

	lineSize = LINE_SIZE( abs( width) , var->type);
	newData = allocb( lineSize * abs( height));
	if ( newData == NULL)
		croak("Image::stretch: cannot allocate %d bytes", lineSize * abs( height));
	memset( newData, 0, lineSize * abs( height));
	if ( var-> data) {
		char error[256];
		if (!ic_stretch( var-> type, var-> data, var-> w, var-> h, newData, width, height, var->scaling, error)) {
			free( var->data);
			my-> make_empty( self);
			croak("%s", error);
		}
	}

	free( var->data);
	var->data = newData;
	var->lineSize = lineSize;
	var->dataSize = lineSize * abs( height);
	var->w = abs( width);
	var->h = abs( height);

	my->end_preserve_type(self, &p);
	my->update_change( self);
}

static void
Image_reset_sv( Handle self, int new_type, SV * palette, Bool triplets)
{
	int colors;
	RGBColor pal_buf[256], *pal_ptr;
	if ( !palette || palette == NULL_SV) {
		pal_ptr = NULL;
		colors  = 0;
	} else if ( SvROK( palette) && ( SvTYPE( SvRV( palette)) == SVt_PVAV)) {
		colors = apc_img_read_palette( pal_ptr = pal_buf, palette, triplets);
	} else {
		pal_ptr = NULL;
		colors  = SvIV( palette);
	}
	my-> reset( self, new_type, pal_ptr, colors);
}

void
Image_set( Handle self, HV * profile)
{
	dPROFILE;
	if ( pexist( conversion))
	{
		my-> set_conversion( self, pget_i( conversion));
		pdelete( conversion);
	}
	if ( pexist( scaling))
	{
		my->set_scaling( self, pget_i( scaling));
		pdelete( scaling);
	}

	if ( Image_set_extended_data( self, profile))
		pdelete( data);

	if ( pexist( type))
	{
		int newType = pget_i( type);
		if ( !itype_supported( newType))
			warn("Invalid image type requested (%08x) in Image::set_type", newType);
		else
			if ( !opt_InPaint) {
				SV * palette;
				Bool triplets;
				if ( pexist( palette)) {
					palette  = pget_sv(palette);
					triplets = true;
				} else if ( pexist( colormap)) {
					palette  = pget_sv(colormap);
					triplets = false;
				} else {
					palette = NULL_SV;
					triplets = false;
				}
				Image_reset_sv( self, newType, palette, triplets);
			}
		pdelete( colormap);
		pdelete( palette);
		pdelete( type);
	}

	if ( pexist( size))
	{
		int set[2];
		prima_read_point( pget_sv( size), set, 2, "Array panic on 'size'");
		my-> stretch( self, set[0], set[1]);
		pdelete( size);
	}

	if ( pexist( resolution))
	{
		Point set;
		prima_read_point( pget_sv( resolution), (int*)&set, 2, "Array panic on 'resolution'");
		my-> set_resolution( self, set);
		pdelete( resolution);
	}

	inherited set ( self, profile);
}

/*
Routine sets image data almost as Image::set_data, but taking into
account 'lineSize', 'type', and 'reverse' fields. To be called from bunch routines,
line ::init or ::set. Returns true if relevant fields were found and
data extracted and set, and false if user data should be set through ::set_data.
Image itself may undergo conversion during the routine; in that case 'palette'
property may be used also. All these fields, if used, or meant to be used but
erroneously set, will be deleted regardless of routine success.
*/
Bool
Image_set_extended_data( Handle self, HV * profile)
{
	dPROFILE;
	void *data, *proc;
	STRLEN dataSize;
	int lineSize = 0, newType = var-> type, fixType, oldType = -1;
	Bool pexistType, pexistLine, pexistReverse, supp, reverse = false;

	if ( !pexist( data)) {
		if ( pexist( lineSize)) {
			warn( "Image: lineSize supplied without data property.");
			pdelete( lineSize);
		}
		return false;
	}

	data = SvPV( pget_sv( data), dataSize);

	/* parameters check */
	pexistType = pexist( type) && ( newType = pget_i( type)) != var-> type;
	pexistLine = pexist( lineSize) && ( lineSize = pget_i( lineSize)) != var-> lineSize;
	pexistReverse = pexist( reverse) && ( reverse = pget_B( reverse));

	pdelete( lineSize);
	pdelete( type);
	pdelete( reverse);

	if ( !pexistLine && !pexistType && !pexistReverse) return false;

	if ( is_opt( optInDraw) || dataSize <= 0)
		goto GOOD_RETURN;

	/* determine line size, if any */
	if ( pexistLine) {
		if ( lineSize <= 0) {
			warn( "Image::set_data: invalid lineSize:%d passed", lineSize);
			goto GOOD_RETURN;
		}
		if ( !pexistType) { /* plain repadding */
			ibc_repad(( Byte*) data, var-> data, lineSize, var-> lineSize, dataSize, var-> dataSize, 1, 1, NULL, reverse);
			my-> update_change( self);
			goto GOOD_RETURN;
		}
	}

	/* pre-fetch auto conversion, if set in same clause */
	if ( pexist( preserveType))
		opt_assign( optPreserveType, pget_B( preserveType));
	if ( is_opt( optPreserveType))
		oldType = var-> type;

	/* getting closest type */
	if (( supp = itype_supported( newType))) {
		fixType = newType;
		proc    = NULL;
	} else if ( !itype_importable( newType, &fixType, &proc, NULL)) {
		warn( "Image::set_data: invalid image type %08x", newType);
		goto GOOD_RETURN;
	}

	/* fixing image and maybe palette - for known type it's same code as in ::set, */
	/* but here's no sense calling it, just doing what we need. */
	if ( fixType != var-> type || pexist( palette) || pexist( colormap)) {
		SV * palette;
		Bool triplets;
		if ( pexist( palette)) {
			palette = pget_sv( palette);
			triplets = true;
		} else if ( pexist( colormap)) {
			palette = pget_sv( colormap);
			triplets = false;
		} else {
			palette = NULL_SV;
			triplets = false;
		}
		Image_reset_sv( self, fixType, palette, triplets);
		pdelete( palette);
		pdelete( colormap);
	}

	/* copying user data */
	if ( supp && lineSize == 0 && !reverse)
		/* same code as in ::set_data */
		memcpy( var->data, data, (dataSize > (STRLEN)var->dataSize) ? (STRLEN)var->dataSize : dataSize);
	else {
		/* if no explicit lineSize set, assuming x4 padding */
		if ( lineSize == 0)
			lineSize = LINE_SIZE( var-> w , newType);
		/* copying using repadding routine */
		ibc_repad(( Byte*) data, var-> data, lineSize, var-> lineSize, dataSize, var-> dataSize,
				( newType & imBPP) / 8, ( var-> type & imBPP) / 8, proc, reverse
		);
	}
	my-> update_change( self);
	/* if want to keep original type, restoring */
	if ( is_opt( optPreserveType))
		my-> set_type( self, oldType);

GOOD_RETURN:
	pdelete(data);
	return true;
}


double
Image_stats( Handle self, Bool set, int index, double value)
{
	if ( index < 0 || index > isMaxIndex) return 0;
	if ( set) {
		var-> stats[ index] = value;
		var-> statsCache |= 1 << index;
		return 0;
	} else {
#define gather_stats(TYP) if ( var->data) {                   \
	TYP *src = (TYP*)var->data, *stop, *s;                \
	maxv = minv = *src;                                   \
	for ( y = 0; y < var->h; y++) {                       \
		s = src;  stop = s + var->w;                  \
		while (s != stop) {                           \
			v = (double)*s;                       \
			sum += v;                             \
			sum2 += v*v;                          \
			if ( minv > v) minv = v;              \
			if ( maxv < v) maxv = v;              \
			s++;                                  \
		}                                             \
		src = (TYP*)(((Byte *)src) + var->lineSize);  \
	}                                                     \
}
		int y;
		double sum = 0.0, sum2 = 0.0, minv = 0.0, maxv = 0.0, v;

		if ( var->statsCache & ( 1 << index)) return var->stats[ index];
		/* calculate image stats */
		switch ( var->type) {
			case imByte:    gather_stats(uint8_t); break;
			case imShort:   gather_stats(int16_t); break;
			case imLong:    gather_stats(int32_t); break;
			case imFloat:   gather_stats(float);   break;
			case imDouble:  gather_stats(double);  break;
			default:        return 0;
		}
		if ( var->w * var->h > 0)
		{
			var->stats[ isSum] = sum;
			var->stats[ isSum2] = sum2;
			sum /= var->w * var->h;
			sum2 /= var->w * var->h;
			sum2 = sum2 - sum*sum;
			var->stats[ isMean] = sum;
			var->stats[ isVariance] = sum2;
			var->stats[ isStdDev] = sqrt(sum2);
			var->stats[ isRangeLo] = minv;
			var->stats[ isRangeHi] = maxv;
		} else {
			for ( y = 0; y <= isMaxIndex; y++) var->stats[ y] = 0;
		}
		var->statsCache = (1 << (isMaxIndex + 1)) - 1;
	}
	return var->stats[ index];
}

Bool
Image_matrix_transform( Handle self, Matrix matrix, ColorPixel fill, Point *aperture)
{
	Image i;
	int desired_type = var->type;

	if (( desired_type & imBPP) <= 8)
		desired_type = (desired_type & imGrayScale) ? imByte : imRGB;

	if (var->type != desired_type) {
		Bool ok;
		int type = var->type;
		my->set_type( self, desired_type );
		ok = my->matrix_transform( self, matrix, fill, aperture );
		if ( is_opt( optPreserveType)) {
			int conv = var-> conversion;
			my-> set_conversion( self, ictNone);
			my-> set_type( self, type);
			my-> set_conversion( self, conv);
		}
		return ok;
	}

	if (!img_2d_transform( self, matrix, fill, &i, aperture ))
		return false;

	if ( i.data != NULL ) {
		free( var->data);

		var->h        = i. h;
		var->w        = i. w;
		var->lineSize = i. lineSize;
		var->dataSize = i. dataSize;
		var->data     = i. data;

		my-> update_change(self);
	}

	return true;
}

Bool
Image_transform( Handle self, HV * profile )
{
	dPROFILE;
	Matrix matrix;
	ColorPixel fill;

	if ( !pexist(matrix)) {
		warn("'matrix' is required");
		goto FAIL;
	}

	{
		int i;
		double *cmatrix;
		if (( cmatrix = (double*) prima_read_array(
			pget_sv(matrix),
			"transform.matrix", 'd', 1, 6, 6, NULL, NULL)
		) == NULL)
			goto FAIL;

		for ( i = 0; i < 6; i++)
			matrix[i] = cmatrix[i];
		free(cmatrix);
	}

	bzero(fill, sizeof(fill));
	if ( pexist(fill))
		Image_read_pixel( self, pget_sv(fill), &fill );

	hv_clear(profile);
	return my-> matrix_transform( self, matrix, fill, NULL );

FAIL:
	hv_clear(profile);
	return false;
}

Color
Image_premultiply_color( Handle self, int rop, Color color)
{
	int alpha;

	if ( (rop & ropPorterDuffMask) != ropBlend)
		return color;

	alpha = var->alpha;
	if ( rop & ropSrcAlpha)
		alpha = alpha * ((rop >> ropSrcAlphaShift) & 0xff) / 255;
	if ( alpha == 255 )
		return color;

	if ( var->type & imGrayScale )
		return color * alpha / 255;

	{
		int c[3] = {
			( color & 0xff         ) * alpha / 255,
			( ((color)>>8) & 0xff  ) * alpha / 255,
			( ((color)>>16) & 0xff ) * alpha / 255
		};
		color =
			(c[0] & 0xff) |
			((c[1] & 0xff) << 8) |
			((c[2] & 0xff) << 16)
			;
	}

	return color;
}

void
Image_begin_preserve_type( Handle self, PImagePreserveTypeRec save)
{
	if ( !is_opt(optPreserveType)) {
		save-> enabled = false;
		return;
	}
	save->enabled = true;
	save->type    = var->type;
	save->colors  = var->palSize;
	memcpy( save->palette, var->palette, sizeof(save->palette) );
}

void
Image_end_preserve_type( Handle self, PImagePreserveTypeRec save)
{
	if ( save->enabled )
		my->reset( self, save->type, save->palette, save->colors);
}

#ifdef __cplusplus
}
#endif
