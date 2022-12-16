#include "apricot.h"
#include "Icon.h"
#include "Region.h"
#include "img_conv.h"
#include <Icon.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CImage->
#define my  ((( PIcon) self)-> self)
#define var (( PIcon) self)

static void
produce_mask( Handle self)
{
	Byte * area8 = var-> data;
	Byte * dest = var-> mask;
	Byte * src;
	Byte color = 0;
	RGBColor rgbcolor;
	int i, bpp2;
	int line8Size = LINE_SIZE(var->w, 8), areaLineSize = var-> lineSize;
	int bpp = var-> type & imBPP;
	int w = var-> w, h = var-> h;

	if ( var-> w == 0 || var-> h == 0) return;

	if ( var-> autoMasking == amNone) return;

	if ( var-> autoMasking == amMaskColor) {
		rgbcolor. b = var-> maskColor & 0xFF;
		rgbcolor. g = (var-> maskColor >> 8)  & 0xFF;
		rgbcolor. r = (var-> maskColor >> 16) & 0xFF;
		if ( bpp <= 8)
			color = cm_nearest_color( rgbcolor, var-> palSize, var-> palette);
	} else if ( var-> autoMasking == amMaskIndex) {
		if ( bpp > 8) return;
		color = var-> maskIndex;
		bzero( &rgbcolor, sizeof(rgbcolor));
	}

	if ( bpp == imMono) {
		/* mono case simplifies our task */
		int  j = var-> maskSize;
		Byte * mask = var-> mask;
		memcpy ( var-> mask, var-> data, var-> dataSize);
		if ( color == 0) {
			while ( j--) mask[ j] = ~mask[ j];
		}
		var-> palette[color]. r = var-> palette[color]. g = var-> palette[color]. b = 0;
		if ( color > 0)
			var-> type &= ~imGrayScale;
		return;
	}

	/* convert to 8 bit */
	switch ( bpp)
	{
	case im16:
	case im256:
	case imRGB:
		bpp2 = bpp;
		break;
	default:
		bpp2  = im256;
		areaLineSize = line8Size;
		if (!( area8 = allocb( var-> h * line8Size))) return;
		ic_type_convert( self, area8, var-> palette, im256 | ( var-> type & imGrayScale), &var-> palSize, false);
		break;
	}

	if ( var-> autoMasking == amAuto) {  /* calculate transparent color */
		Byte corners [4];
		Byte counts  [4] = {1, 1, 1, 1};
		RGBColor rgbcorners[4];
		int j = areaLineSize, k;

		/* retrieving corner pixels */
		switch ( bpp2) {
		case im16:
			corners[ 0] = area8[ 0] >> 4;
			corners[ 1] = area8[( w - 1) >> 1];
			corners[ 1] = (( w - 1) & 1) ? corners[ 1] & 0x0f : corners[ 1] >> 4;
			corners[ 2] = area8[ j * ( h - 1)] >> 4;
			corners[ 3] = area8[ j * ( h - 1) + (( w - 1) >> 1)];
			corners[ 3] = (( w - 1) & 1) ? corners[ 3] & 0x0f : corners[ 3] >> 4;
			for ( j = 0; j < 4; j++) {
				rgbcorners[j].r = var-> palette[ corners[ j]]. r;
				rgbcorners[j].g = var-> palette[ corners[ j]]. g;
				rgbcorners[j].b = var-> palette[ corners[ j]]. b;
			}
			break;
		case im256:
			corners[ 0] = area8[ 0];
			corners[ 1] = area8[ w - 1];
			corners[ 2] = area8[ j * ( h - 1)];
			corners[ 3] = area8[ j * ( h - 1) + w - 1];
			for ( j = 0; j < 4; j++) {
				rgbcorners[j].r = var-> palette[ corners[ j]]. r;
				rgbcorners[j].g = var-> palette[ corners[ j]]. g;
				rgbcorners[j].b = var-> palette[ corners[ j]]. b;
			}
			break;
		case imRGB:
			rgbcorners[0] = *(PRGBColor)( area8);
			rgbcorners[1] = *(PRGBColor)( area8 + ( w - 1) * 3);
			rgbcorners[2] = *(PRGBColor)( area8 + j * ( h - 1));
			rgbcorners[3] = *(PRGBColor)( area8 + j * ( h - 1) + ( w - 1) * 3);
			for ( j = 0; j < 4; j++) corners[j] = j;
			#define rgbcmp(x,y) ((rgbcorners[x].r == rgbcorners[y].r) &&\
					(rgbcorners[x].g == rgbcorners[y].g) &&\
					(rgbcorners[x].b == rgbcorners[y].b))
			if ( rgbcmp(1,0)) corners[1] = 0;
			if ( rgbcmp(2,0)) corners[2] = 0;
			if ( rgbcmp(3,0)) corners[3] = 0;
			if ( rgbcmp(2,1)) corners[2] = corners[1];
			if ( rgbcmp(3,1)) corners[3] = corners[1];
			if ( rgbcmp(3,2)) corners[3] = corners[2];
			#undef rgbcmp
			break;
		}

		/* preliminary comparison to a transparent color candidate ( hack ) */
		for ( j = 0; j < 4; j++) {
			if (
				(( rgbcorners[j]. b) == 0) &&
				(( rgbcorners[j]. g) == 128) &&
				(( rgbcorners[j]. r) == 128)) {
				color = corners[ j];
				rgbcolor = rgbcorners[ j];
				goto colorFound;
			}
		}

		color = corners[ 3]; /* our wild (and possibly bad) guess */
		rgbcolor = rgbcorners[ 3];

		/* sorting */
		for ( j = 0; j < 3; j++)
			for (k = 0; k < 3; k++)
				if ( corners[ k] < corners[ k + 1]) {
					Byte l = corners[ k];
					corners[ k] = corners[ k + 1];
					corners[ k + 1] = l;
				}
		/* forming maximum's vector */
		i = 0;
		for (j = 0; j < 3; j++) if ( corners[ j + 1] == corners[ j]) counts[ i]++; else i++;
		for (j = 0; j < 3; j++)
			for (k = 0; k < 3; k++)
				if ( counts[ k] < counts[ k + 1]) {
					Byte l = counts[ k];
					counts[ k] = counts[ k + 1];
					counts[ k + 1] = l;
					l = corners[ k];
					corners[ k] = corners[ k + 1];
					corners[ k + 1] = l;
				}
		if (( counts[0] > 2) || (( counts[0] == 2) && ( counts[1] == 1))) {
			color = corners[ 0];
			rgbcolor = rgbcorners[ 0];
		} else {
			int colorsToCompare = ( counts[0] == 2) ? 2 : 4;

			/* compare to that yellowish hue... */
			for ( j = 0; j < colorsToCompare; j++)
				if (( rgbcorners[j]. b < 20) &&
					( rgbcorners[j]. r > 100) &&
					( rgbcorners[j]. r < 150) &&
					( rgbcorners[j]. g > 100) &&
					( rgbcorners[j]. g < 150))
				{
					color = corners[ j];
					rgbcolor = rgbcorners[ j];
					goto colorFound;
				}

			/* compare to MicroSoft Pink */
			for ( j = 0; j < colorsToCompare; j++)
				if (( rgbcorners[j]. g < 20) &&
					( rgbcorners[j]. r > 200) &&
					( rgbcorners[j]. b > 200))
				{
					color = corners[ j];
					rgbcolor = rgbcorners[ j];
					goto colorFound;
				}
		}
colorFound:;
	}

	/* processing transparency */
	memset( var-> mask, 0, var-> maskSize);
	src  = area8;
	for ( i = 0; i < h; i++, dest += var-> maskLine, src += areaLineSize) {
		register int j;
		switch ( bpp2) {
		case im16:
			{
				int max = ( w >> 1) + ( w & 1);
				register int k = 0;
				for ( j = 0; j < max; j++) {
					if ( color == ( src[ j] >> 4))
						dest[ k >> 3] |= 1 << (7 - ( k & 7));
					if ( color == ( src[ j] & 0x0f))
						dest[ k >> 3] |= 1 << (6 - ( k & 7));
					k += 2;
				}
			}
			break;
		case imRGB:
			{
				register PRGBColor r = ( PRGBColor) src;
				for ( j = 0; j < w; j++) {
					if (( r-> r == rgbcolor.r) &&
						( r-> g == rgbcolor.g) &&
						( r-> b == rgbcolor.b))
						dest[ j >> 3] |= 1 << (7 - ( j & 7));
					r++;
				}
			}
			break;
		default:
			for ( j = 0; j < w; j++)
				if ( src[ j] == color)
					dest[ j >> 3] |= 1 << (7 - ( j & 7));
		}
	}

	/* finalize */
	if ( var-> data != area8) free( area8);
	if ( var-> palSize > color && bpp <= im256) {
		var-> palette[ color]. r = var-> palette[ color]. b = var-> palette[ color]. g = 0;
		if ( color > 0)
			var-> type &= ~imGrayScale;
	}
}

void
Icon_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	my-> set_maskType( self, pget_i( maskType));
	my-> update_change(self); /* instantiate mask memory */

	my-> set_maskColor( self, pget_i( maskColor));
	my-> set_maskIndex( self, pget_i( maskIndex));
	my-> set_autoMasking( self, pget_i( autoMasking));
	my-> set_mask( self, pget_sv( mask));
	CORE_INIT_TRANSIENT(Icon);
}


SV *
Icon_mask( Handle self, Bool set, SV * svmask)
{
	STRLEN maskSize;
	void * mask;
	int am = var-> autoMasking;
	if ( var-> stage > csFrozen) return NULL_SV;
	if ( !set) {
		SV * sv = newSV_type(SVt_PV);
		SvREADONLY_on(sv);
		SvLEN_set(sv, 0); /* So Perl won't free it. */
		SvPV_set(sv, (char*)var-> mask);
		SvCUR_set(sv, var-> maskSize);
		SvPOK_only(sv);
		return sv;
	}
	mask = SvPV( svmask, maskSize);
	if ( is_opt( optInDraw) || maskSize <= 0) return NULL_SV;
	memcpy( var-> mask, mask, (maskSize > (STRLEN)var-> maskSize) ? (STRLEN)var-> maskSize : maskSize);
	var-> autoMasking = amNone;
	my-> update_change( self);
	var-> autoMasking = am;
	return NULL_SV;
}

int
Icon_autoMasking( Handle self, Bool set, int autoMasking)
{
	if ( !set)
		return var-> autoMasking;
	if ( var-> autoMasking == autoMasking) return 0;
	var-> autoMasking = autoMasking;
	if ( is_opt( optInDraw)) return 0;
	my-> update_change( self);
	return 0;
}

/*

1-bit format is a true AND-mask: 0 is alpha=1, 1 is alpha=0
8-bit format has alpha value from 0 to 255.

Note that inter-conversion between these negates the pixel values

*/
Byte*
Icon_convert_mask( Handle self, int type )
{
	int i;
	int srcLine = LINE_SIZE( var-> w, var-> maskType );
	int dstLine = LINE_SIZE( var-> w, type );
	Byte colorref[256], *src = var-> mask, *dst, *ret;
	RGBColor palette[2];

	if ( type == var-> maskType )
		croak("invalid usage of Icon::convert_mask");
	if ( !( ret = malloc( dstLine * var-> h))) {
		warn("Icon::convert_mask: cannot allocate %d bytes", dstLine * var-> h);
		return NULL;
	}
	bzero( ret, dstLine * var-> h);

	switch (type) {
	case imbpp1:
		/* downgrade */
		memset( colorref,     1, 1   );
		memset( colorref + 1, 0, 255 );
		for ( i = 0, dst = ret; i < var->h; i++, src += srcLine, dst += dstLine) {
			memset( dst, 0, dstLine );
			bc_byte_mono_cr( src, dst, var-> w, colorref);
		}
		break;
	case imbpp8:
		/* upgrade */
		memset( &palette[0], 0xff, sizeof(RGBColor));
		memset( &palette[1], 0x00, sizeof(RGBColor));
		for ( i = 0, dst = ret; i < var->h; i++, src += srcLine, dst += dstLine)
			bc_mono_graybyte( src, dst, var-> w, palette);
		break;
	default:
		croak("invalid usage of Icon::convert_mask");
	}
	return ret;
}

int
Icon_maskType( Handle self, Bool set, int type)
{
	if ( !set)
		return var-> maskType;

	type &= ~imGrayScale;
	if ( var-> maskType == type) return 0;
	switch ( type ) {
	case imbpp1:
	case imbpp8:
		if ( var-> mask ) {
			Byte * new_mask = Icon_convert_mask(self, type);
			free( var-> mask );
			var-> mask = new_mask;
			var-> maskLine = LINE_SIZE( var-> w, type );
			var-> maskSize = var-> maskLine * var-> h;
		}
		break;
	default:
		croak("mask type must be either im::bpp1 or im::bpp8");
	}

	var-> maskType = type;
	return 1;
}

Color
Icon_maskColor( Handle self, Bool set, Color color)
{
	if ( !set)
		return var-> maskColor;
	if ( var-> maskColor == color) return 0;
	var-> maskColor = color;
	if ( is_opt( optInDraw)) return 0;
	if ( var-> autoMasking == amMaskColor)
		my-> update_change( self);
	return clInvalid;
}

int
Icon_maskIndex( Handle self, Bool set, int index)
{
	if ( !set)
		return var-> maskIndex;
	var-> maskIndex = index;
	if ( is_opt( optInDraw)) return 0;
	if ( var-> autoMasking == amMaskIndex)
		my-> update_change( self);
	return -1;
}

SV *
Icon_maskPixel( Handle self, Bool set, int x, int y, SV * pixel)
{
	Point pt;

	if (!set) {
		if ( opt_InPaint)
			return inherited pixel(self,false,x,y,pixel);

		pt = prima_matrix_apply_to_int( var->current_state.matrix, x, y );
		x = pt.x;
		y = pt.y;

		if (x >= var->w || x < 0 || y >= var->h || y < 0)
			return newSViv(clInvalid);

		switch (var->maskType) {
		case imbpp1: {
			Byte p = var->mask[ var->maskLine * y + ( x>>3 )];
			p = (p >> (7 - (x & 7))) & 1;
			return newSViv(p);
		}
		case imbpp8: {
			Byte p = var->mask[ var->lineSize * y + x];
			return newSViv(p);
		}
		default:
			return newSViv(clInvalid);
		}
	} else {
		IV color;
		if ( is_opt( optInDraw))
			return inherited pixel(self,true,x,y,pixel);

		pt = prima_matrix_apply_to_int( var->current_state.matrix, x, y );
		x = pt.x;
		y = pt.y;


		if ( x >= var->w || x < 0 || y >= var->h || y < 0)
			return NULL_SV;

		color = SvIV( pixel);
		if ( color < 0 ) color = 0;
		if ( color > 255 ) color = 255;
		switch (var->maskType) {
		case imbpp1 : {
			int x1 = 7 - ( x & 7 );
			Byte p = (color > 0) ? 1 : 0;
			Byte *pd = var->mask + (var->maskLine * y + ( x >> 3));
			*pd &= ~(1 << x1);
			*pd |= p << x1;
			break;
		}
		case imbpp8:
			var->mask[var->maskLine * y + x] = color;
			break;
		default:
			return NULL_SV;
		}
		my->update_change( self);
		return NULL_SV;
	}
}

void
Icon_update_change( Handle self)
{
	if ( var-> updateLock > 0 ) return;

	inherited update_change( self);

	if ( var-> maskType == 0) return; /* inside inherited init */

	if ( var-> autoMasking == amNone) {
		int maskLine = LINE_SIZE( var-> w, var-> maskType );
		int maskSize = maskLine * var-> h;
		if ( maskLine != var-> maskLine || maskSize != var-> maskSize) {
			free( var-> mask);
			var-> maskLine = maskLine;
			if (!( var-> mask = allocb( var-> maskSize = maskSize)) && maskSize > 0) {
				my-> make_empty( self);
				warn("Not enough memory: %d bytes", maskSize);
			} else
				memset( var-> mask, 0, maskSize);
		}
		return;
	}

	free( var-> mask);
	if ( var-> data)
	{
		int oldtype = var-> maskType;
		var-> maskType = imbpp1;
		var-> maskLine = LINE_SIZE( var-> w, var-> maskType );
		var-> maskSize = var-> maskLine * var-> h;
		if ( !( var-> mask = allocb( var-> maskSize)) && var-> maskSize > 0) {
			my-> make_empty( self);
			warn("Not enough memory: %d bytes", var-> maskSize);
			return;
		}
		bzero( var-> mask, var-> maskSize );
		produce_mask( self);
		if ( oldtype != imbpp1 )
			my-> set_maskType( self, oldtype);
	}
	else
		var-> mask = NULL;
}

void
Icon_stretch( Handle self, int width, int height)
{
	Byte * newMask = NULL;
	int lineSize, oldW = var-> w, oldH = var-> h, am = var-> autoMasking;
	if ( var->stage > csFrozen) return;
	if ( width  >  65535) width  =	65535;
	if ( height >  65535) height =	65535;
	if ( width  < -65535) width  = -65535;
	if ( height < -65535) height = -65535;
	if (( width == var->w) && ( height == var->h)) return;
	if ( width == 0 || height == 0)
	{
		my->create_empty( self, 0, 0, var->type);
		return;
	}

	if ( var-> mask && var-> maskType == imbpp1 && var-> scaling > istBox ) {
		/* upgrade to bpp8 */
		my-> set_maskType( self, imbpp8 );
	}


	lineSize = LINE_SIZE( abs( width), var-> maskType );
	newMask  = allocb( lineSize * abs( height));
	if ( newMask == NULL && lineSize > 0) {
		my-> make_empty( self);
		croak("Icon::stretch: cannot allocate %d bytes", lineSize * abs( height));
	}
	var-> autoMasking = amNone;
	if ( var-> mask) {
		char error[256];
		if ( !ic_stretch( var->maskType | imGrayScale, var-> mask, oldW, oldH, newMask, width, height, var->scaling, error)) {
			free(newMask);
			my-> make_empty( self);
			croak("%s", error);
		}
	}
	inherited stretch( self, width, height);
	free( var-> mask);


	var->mask = newMask;
	var->maskLine = lineSize;
	var->maskSize = lineSize * abs( height);
	inherited stretch( self, width, height);
	var-> autoMasking = am;
}

void
Icon_create_empty( Handle self, int width, int height, int type)
{
	my-> create_empty_icon( self, width, height, type, imbpp1);
}

void
Icon_create_empty_icon( Handle self, int width, int height, int type, int maskType)
{
	inherited create_empty( self, width, height, type);
	free( var-> mask);
	if ( var-> data)
	{
		var-> maskType = maskType;
		var-> maskLine = LINE_SIZE( var-> w, var-> maskType );
		var-> maskSize = var-> maskLine * var-> h;
		if ( !( var-> mask = allocb( var-> maskSize)) && var-> maskSize > 0) {
			my-> make_empty( self);
			warn("Not enough memory: %d bytes", var-> maskSize);
			return;
		}
		memset( var-> mask, 0, var-> maskSize);
	}
	else {
		var-> mask = NULL;
		var-> maskLine = 0;
		var-> maskSize = 0;
	}
}

Handle
Icon_dup( Handle self)
{
	Handle h = inherited dup( self);
	PIcon  i = ( PIcon) h;

	if ( var-> maskType != imbpp1 ) {
		Byte * p;
		if ( !(p = realloc( i-> mask, var-> maskSize ))) {
			warn("Icon::dup: cannot allocate %d bytes", var->maskSize);
			Object_destroy(h);
			return NULL_HANDLE;
		}
		i-> mask = p;
	}

	i-> autoMasking = var-> autoMasking;
	i-> maskType	= var-> maskType;
	i-> maskColor	= var-> maskColor;
	i-> maskIndex	= var-> maskIndex;
	i-> maskSize	= var-> maskSize;
	i-> maskLine	= var-> maskLine;

	memcpy( i-> mask, var-> mask, var-> maskSize);
	return h;
}

IconHandle
Icon_split( Handle self)
{
	IconHandle ret = {0,0};
	PImage i;
	HV * profile = newHV();
	char* className = var-> self-> className;

	pset_H( owner,	      var-> owner);
	pset_i( width,	      var-> w);
	pset_i( height,       var-> h);
	pset_i( type,	      var->maskType|imGrayScale);
	pset_i( conversion,   var->conversion);
	pset_i( scaling,      var->scaling);
	pset_i( preserveType, is_opt( optPreserveType));

	ret. andMask = Object_create( "Prima::Image", profile);
	sv_free(( SV *) profile);
	i = ( PImage) ret. andMask;
	memcpy( i-> data, var-> mask, var-> maskSize);
	i-> self-> update_change(( Handle) i);

	var-> self-> className = inherited className;
	ret. xorMask	     = inherited dup( self);
	(void) hv_delete(( HV*)SvRV( PImage(ret. xorMask)-> mate), "extras", 6, G_DISCARD);
	var-> self-> className = className;

	--SvREFCNT( SvRV( i-> mate));
	return ret;
}

void
Icon_combine( Handle self, Handle xorMask, Handle andMask)
{
	Bool killAM = 0;
	int maskType;

	if ( !kind_of( xorMask, CImage) || !kind_of( andMask, CImage))
		return;

	var-> autoMasking = amNone;
	maskType = PImage( andMask)-> type & imBPP;
	if ( maskType != imbpp1 && maskType != imbpp8) {
		killAM = 1;
		andMask = CImage( andMask)-> dup( andMask);
		CImage( andMask)-> set_type( andMask, imbpp1);
		maskType = imbpp1;
	}

	my-> create_empty_icon( self, PImage( xorMask)-> w, PImage( xorMask)-> h, PImage( xorMask)-> type, maskType);

	if ( var-> w != PImage( andMask)-> w || var-> h != PImage( andMask)-> h) {
		if ( !killAM) {
			killAM = 1;
			andMask = CImage( andMask)-> dup( andMask);
		}
		CImage( andMask)-> set_size( andMask, my-> get_size( self));
	}

	memcpy( var-> data, PImage( xorMask)-> data, var-> dataSize);
	memcpy( var-> mask, PImage( andMask)-> data, var-> maskSize);
	memcpy( var-> palette, PImage( xorMask)-> palette, 768);
	var-> palSize = PImage( xorMask)-> palSize;

	if ( killAM) Object_destroy( andMask);

	my-> update_change( self);
}

void
Icon_set( Handle self, HV * profile)
{
	dPROFILE;

	if (pexist( maskType)) {
		int maskType = pget_i(maskType);
		if ( maskType == var-> maskType ) pdelete( maskType );
	}

	if ( pexist( maskType) && pexist( mask ))
	{
		free( var-> mask );
		var-> mask = NULL;
		my-> set_maskType( self, pget_i( maskType));
		my-> set_mask( self, pget_sv( mask));
		pdelete( maskType);
		pdelete( mask);
	}

	inherited set ( self, profile);
}

Handle
Icon_extract( Handle self, int x, int y, int width, int height)
{
	int nodata = 0;
	Handle h = inherited extract( self, x, y, width, height);
	PIcon i = (PIcon) h;
	unsigned char * mask = var->mask;
	int ls = var->maskLine;

	if ( var->w == 0 || var->h == 0) return h;
	if ( x < 0) x = 0;
	if ( y < 0) y = 0;
	if ( x >= var->w) x = var->w - 1;
	if ( y >= var->h) y = var->h - 1;
	if ( width  + x > var->w) width  = var->w - x;
	if ( height + y > var->h) height = var->h - y;
	if ( width <= 0 ) {
		width = 1;
		nodata = 1;
	}
	if ( height <= 0 ) {
		height = 1;
		nodata = 1;
	}
	if ( nodata ) return h;

	CIcon(h)->set_autoMasking(h, amNone);
	CIcon(h)->set_maskType(h, var-> maskType);
	CIcon(h)->set_maskColor(h, var-> maskColor);

	if ( var->maskType == imbpp8)
		while ( height-- > 0) {
			memcpy( i-> mask + height * i-> maskLine, mask + ( y + height) * ls + x, width);
	} else {
		while ( height-- > 0)
			bc_mono_copy( mask + ( y + height) * ls, i-> mask + height * i-> maskLine, x, width);
	}
	return h;
}

void
Icon_make_empty( Handle self)
{
	inherited make_empty(self);
	free( var->mask);
	var->mask = NULL;
}

Handle
Icon_bitmap( Handle self)
{
	Handle h;
	Point s;
	HV * profile;

	if ( !apc_sys_get_value(svLayeredWidgets))
		return inherited bitmap(self);

	profile = newHV();
	pset_H( owner,	      var->owner);
	pset_i( width,	      var->w);
	pset_i( height,       var->h);
	pset_sv_noinc( palette,     my->get_palette( self));
	pset_i( type,	      dbtLayered);
	h = Object_create( "Prima::DeviceBitmap", profile);
	sv_free(( SV *) profile);
	s = CDrawable( h)-> get_size( h);
	CDrawable( h)-> put_image_indirect( h, self, 0, 0, 0, 0, s.x, s.y, s.x, s.y, ropSrcCopy);
	--SvREFCNT( SvRV( PDrawable( h)-> mate));

	return h;
}

void
Icon_premultiply_alpha( Handle self, SV * alpha)
{
	if ( !alpha || ( SvTYPE( alpha ) == SVt_NULL)) {
		int type = var-> maskType;
		Image dummy;
		/* multiply with self */
		if ( var-> maskType != imbpp8 )
			my-> set_maskType( self, imbpp8 );

		img_fill_dummy( &dummy, var-> w, var-> h, imByte, var-> mask, std256gray_palette);
		img_premultiply_alpha_map( self, (Handle) &dummy);

		if ( is_opt( optPreserveType) && var-> maskType != imbpp8 )
			my-> set_maskType( self, type );
	} else
		inherited premultiply_alpha( self, alpha );
}

Bool
Icon_bar_alpha( Handle self, int alpha, int x1, int y1, int x2, int y2)
{
	Image dummy;
	ImgPaintContext ctx;
	Bool free_rgn = false;
	PRegionRec rgn = var->regionData;

	if (opt_InPaint)
		return apc_gp_alpha( self, alpha, x1, y1, x2, y2);

	if ( x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0 ) {
		x1 = 0;
		y1 = 0;
		x2 = var-> w - 1;
		y2 = var-> h - 1;
	} else {
		NRect nrect = {x1,y1,x2,y2};
		NPoint npoly[4];

		if ( prima_matrix_is_square_rectangular( var->current_state.matrix, &nrect, npoly)) {
			x1 = floor(nrect.left   + .5);
			y1 = floor(nrect.bottom + .5);
			x2 = floor(nrect.right  + .5);
			y2 = floor(nrect.top    + .5);
		} else {
			Handle rgn1;
			PRegionRec rgndata;
			int i;
			Point poly[4];

			prima_matrix_apply2_to_int( var->current_state.matrix, npoly, poly, 4 );
			x1 = x2 = poly[0].x;
			y1 = y2 = poly[0].y;
			for ( i = 1; i < 4; i++) {
				if ( x1 > poly[i].x ) x1 = poly[i].x;
				if ( y1 > poly[i].y ) y1 = poly[i].y;
				if ( x2 < poly[i].x ) x2 = poly[i].x;
				if ( y2 < poly[i].y ) y2 = poly[i].y;
			}
			rgndata = img_region_polygon( poly, 4, fmWinding | fmOverlay );
			rgn1 = Region_create_from_data( NULL_HANDLE, rgndata );
			free( rgndata );

			if ( var-> regionData ) {
				Handle rgn2 = Region_create_from_data( NULL_HANDLE, var->regionData );
				Region_combine(rgn1, rgn2, rgnopUnion);
				Object_destroy(rgn2);
			}
			rgn = Region_update_change(rgn1, true);
			Object_destroy(rgn1);
		}
	}

	img_fill_dummy( &dummy, var-> w, var-> h, var-> maskType | imGrayScale, var-> mask, std256gray_palette);

	bzero(&ctx, sizeof(ctx));
	memset( ctx.pattern, 0xff, sizeof(ctx.pattern));
	ctx.patternOffset.x = ctx.patternOffset.y = 0;
	ctx.transparent     = false;
	ctx. color[0]       = alpha & 0xff;
	ctx. rop            = ropCopyPut;
	ctx. region         = rgn;
	img_bar((Handle) &dummy, x1, y1, x2 - x1 + 1, y2 - y1 + 1, &ctx);

	if ( free_rgn ) free(rgn);

	return true;
}

Bool
Icon_rotate( Handle self, double degrees, SV * svfill)
{
	Bool ok;
	Image dummy;
	int autoMasking = var->autoMasking, maskType = var->maskType;
	var->autoMasking = amNone;

	var->updateLock++;

	my->set_maskType(self, imbpp8);

	img_fill_dummy( &dummy, var->w, var->h, imByte, var->mask, NULL);
	dummy.scaling = var->scaling;
	dummy.mate    = var->mate;

	ok = inherited rotate(self, degrees, NULL_SV);
	if ( ok ) {
		ok = Image_rotate((Handle) &dummy, degrees, NULL_SV);
		if ( ok ) {
			var-> mask     = dummy.data;
			var-> maskLine = dummy. lineSize;
			var-> maskSize = dummy. dataSize;
			if ( var->w != dummy.w || var->h != dummy.h)
				croak("panic: icon object inconsistent after rotation");
		}
	}

	if (maskType != imbpp8 && is_opt( optPreserveType))
		my-> set_maskType( self, maskType);
	var->updateLock--;
	my->update_change(self);
	var->autoMasking = autoMasking;
	return ok;
}

Bool
Icon_transform( Handle self, HV * profile )
{
	dPROFILE;
	Bool ok;
	Image dummy;
	int autoMasking = var->autoMasking, maskType = var->maskType;
	SV * matrix = NULL;
	var->autoMasking = amNone;

	var->updateLock++;

	my->set_maskType(self, imbpp8);

	img_fill_dummy( &dummy, var->w, var->h, imByte, var->mask, NULL);
	dummy.scaling = var->scaling;
	dummy.mate    = var->mate;

	pdelete( fill );

	if ( pexist(matrix)) {
		matrix = pget_sv(matrix);
		++SvREFCNT(matrix);
	}

	ok = inherited transform(self, profile);
	if ( ok ) {
		pset_sv( matrix, matrix );
		ok = Image_transform((Handle) &dummy, profile );
		hv_clear(profile);
		if ( ok ) {
			var-> mask     = dummy.data;
			var-> maskLine = dummy. lineSize;
			var-> maskSize = dummy. dataSize;
			if ( var->w != dummy.w || var->h != dummy.h)
				croak("panic: icon object inconsistent after 2d transform");
		}
	}
	if ( matrix )
		--SvREFCNT(matrix);

	if (maskType != imbpp8 && is_opt( optPreserveType))
		my-> set_maskType( self, maskType);
	var->updateLock--;
	my->update_change(self);
	var->autoMasking = autoMasking;
	return ok;
}

#ifdef __cplusplus
}
#endif
