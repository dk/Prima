#include "apricot.h"
#include "Drawable.h"
#include "Icon.h"
#include "Region.h"
#include <Drawable.inc>
#include "Drawable_private.h"

#ifdef __cplusplus
extern "C" {
#endif


#undef my
#define inherited CComponent->
#define my  ((( PDrawable) self)-> self)
#define var (( PDrawable) self)

#define gpARGS            Bool inPaint = opt_InPaint
#define gpENTER(fail)     if ( !inPaint) if ( !my-> begin_paint_info( self)) return (fail)
#define gpLEAVE           if ( !inPaint) my-> end_paint_info( self)

#define CHECK_GP(ret) \
	if ( !is_opt(optSystemDrawable)) { \
		warn("This method is not available because %s is not a system Drawable object. You need to implement your own (ref:%d)", my->className, __LINE__);\
		return ret; \
	}

void
Drawable_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited init( self, profile);
	apc_gp_init( self);
	var-> w = var-> h = 0;
	my-> set_alpha        ( self, pget_i ( alpha));
	my-> set_antialias    ( self, pget_B ( antialias));
	my-> set_color        ( self, pget_i ( color));
	my-> set_backColor    ( self, pget_i ( backColor));
	my-> set_fillMode     ( self, pget_i ( fillMode));
	my-> set_fillPattern  ( self, pget_sv( fillPattern));
	my-> set_lineEnd      ( self, pget_i ( lineEnd));
	my-> set_lineJoin     ( self, pget_i ( lineJoin));
	my-> set_linePattern  ( self, pget_sv( linePattern));
	my-> set_lineWidth    ( self, pget_f ( lineWidth));
	my-> set_miterLimit   ( self, pget_i ( miterLimit));
	my-> set_region       ( self, pget_H ( region));
	my-> set_rop          ( self, pget_i ( rop));
	my-> set_rop2         ( self, pget_i ( rop2));
	my-> set_textOpaque   ( self, pget_B ( textOpaque));
	my-> set_textOutBaseline( self, pget_B ( textOutBaseline));
	if ( pexist( translate))
	{
		AV * av;
		Point tr;
		SV ** holder, *sv;

		sv = pget_sv( translate);
		if ( sv && SvOK(sv) && SvROK(sv) && SvTYPE(av = (AV*)SvRV(sv)) == SVt_PVAV && av_len(av) == 1) {
			tr.x = tr.y = 0;
			holder = av_fetch( av, 0, 0);
			if ( holder) tr.x = SvIV( *holder); else warn("Array panic on 'translate'");
			holder = av_fetch( av, 1, 0);
			if ( holder) tr.y = SvIV( *holder); else warn("Array panic on 'translate'");
			my-> set_translate( self, tr);
		} else
			warn("Array panic on 'translate'");

		sv = pget_sv( fillPatternOffset);
		if ( sv && SvOK(sv) && SvROK(sv) && SvTYPE(av = (AV*)SvRV(sv)) == SVt_PVAV && av_len(av) == 1) {
			tr.x = tr.y = 0;
			holder = av_fetch( av, 0, 0);
			if ( holder) tr.x = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
			holder = av_fetch( av, 1, 0);
			if ( holder) tr.y = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
			my-> set_fillPatternOffset( self, tr );
		} else
			warn("Array panic on 'fillPatternOffset'");
	}
	SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::init");
	my-> set_font( self, Font_buffer);
	my-> set_palette( self, pget_sv( palette));
	CORE_INIT_TRANSIENT(Drawable);
}

void
Drawable_done( Handle self)
{
	if ( var-> fillPatternImage ) {
		unprotect_object(var-> fillPatternImage);
		var-> fillPatternImage = NULL_HANDLE;
	}
	clear_font_abc_caches( self);
	apc_gp_done( self);
	inherited done( self);
}

void
Drawable_cleanup( Handle self)
{
	if ( is_opt( optInDrawInfo))
		my-> end_paint_info( self);
	if ( is_opt( optInDraw))
		my-> end_paint( self);
	inherited cleanup( self);
}

Bool
Drawable_begin_paint( Handle self)
{
	if ( var-> stage > csFrozen) return false;
	if ( is_opt( optInDrawInfo)) my-> end_paint_info( self);
	opt_set( optInDraw);
	return true;
}

void
Drawable_end_paint( Handle self)
{
	clear_font_abc_caches( self);
	opt_clear( optInDraw);
}

Bool
Drawable_begin_paint_info( Handle self)
{
	if ( var-> stage > csFrozen) return false;
	if ( is_opt( optInDraw))     return true;
	if ( is_opt( optInDrawInfo)) return false;
	opt_set( optInDrawInfo);
	return true;
}

void
Drawable_end_paint_info( Handle self)
{
	clear_font_abc_caches( self);
	opt_clear( optInDrawInfo);
}

void
Drawable_set( Handle self, HV * profile)
{
	dPROFILE;
	if ( pexist( font))
	{
		SvHV_Font( pget_sv( font), &Font_buffer, "Drawable::set");
		my-> set_font( self, Font_buffer);
		pdelete( font);
	}
	if ( pexist( translate))
	{
		AV * av = ( AV *) SvRV( pget_sv( translate));
		Point tr = {0,0};
		SV ** holder = av_fetch( av, 0, 0);
		if ( holder) tr.x = SvIV( *holder); else warn("Array panic on 'translate'");
		holder = av_fetch( av, 1, 0);
		if ( holder) tr.y = SvIV( *holder); else warn("Array panic on 'translate'");
		my-> set_translate( self, tr);
		pdelete( translate);
	}
	if ( pexist( width) && pexist( height)) {
		Point size;
		size. x = pget_i( width);
		size. y = pget_i( height);
		my-> set_size( self, size);
		pdelete( width);
		pdelete( height);
	}
	if ( pexist( fillPatternOffset))
	{
		AV * av = ( AV *) SvRV( pget_sv( fillPatternOffset));
		Point fpo = {0,0};
		SV ** holder = av_fetch( av, 0, 0);
		if ( holder) fpo.x = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
		holder = av_fetch( av, 1, 0);
		if ( holder) fpo.y = SvIV( *holder); else warn("Array panic on 'fillPatternOffset'");
		my-> set_fillPatternOffset( self, fpo);
		pdelete( fillPatternOffset);
	}
	inherited set( self, profile);
}

Bool
Drawable_graphic_context_push(Handle self)
{
	if (!opt_InPaint) return false;
	return apc_gp_push(self);
}

Bool
Drawable_graphic_context_pop(Handle self)
{
	Bool ok;
	if (!opt_InPaint) return false;
	ok = apc_gp_pop(self);
	if ( var-> fillPatternImage && PObject(var-> fillPatternImage)->stage != csNormal) {
		unprotect_object(var-> fillPatternImage);
		var-> fillPatternImage = NULL_HANDLE;
	}
	return ok;
}

int
Drawable_get_bpp( Handle self)
{
	gpARGS;
	int ret;
	CHECK_GP(0);
	gpENTER(0);
	ret = apc_gp_get_bpp( self);
	gpLEAVE;
	return ret;
}

int
Drawable_get_paint_state( Handle self)
{
	if ( is_opt( optInDraw))
		return psEnabled;
	else if ( is_opt( optInDrawInfo))
		return psInformation;
	else
		return psDisabled;
}

Color
Drawable_get_nearest_color( Handle self, Color color)
{
	gpARGS;
	CHECK_GP(0);
	gpENTER(clInvalid);
	color = apc_gp_get_nearest_color( self, color);
	gpLEAVE;
	return color;
}

SV *
Drawable_get_physical_palette( Handle self)
{
	gpARGS;
	int i, nCol;
	AV * av = newAV();
	PRGBColor r;

	CHECK_GP(NULL_SV);
	gpENTER(newRV_noinc(( SV *) av));
	r = apc_gp_get_physical_palette( self, &nCol);
	gpLEAVE;

	for ( i = 0; i < nCol; i++) {
		av_push( av, newSViv( r[ i].b));
		av_push( av, newSViv( r[ i].g));
		av_push( av, newSViv( r[ i].r));
	}
	free( r);
	return newRV_noinc(( SV *) av);
}

SV *
Drawable_get_handle( Handle self)
{
	char buf[ 256];
	CHECK_GP(NULL_SV);
	snprintf( buf, 256, PR_HANDLE_FMT, apc_gp_get_handle( self));
	return newSVpv( buf, 0);
}

int
Drawable_height( Handle self, Bool set, int height)
{
	Point p = my-> get_size( self);
	if ( !set)
		return p. y;
	p. y = height;
	my-> set_size( self, p);
	return height;
}

Point
Drawable_resolution( Handle self, Bool set, Point resolution)
{
	CHECK_GP(resolution);
	if ( set)
		croak("Attempt to write read-only property %s", "Drawable::resolution");
	return apc_gp_get_resolution( self);
}

Point
Drawable_size ( Handle self, Bool set, Point size)
{
	if ( set)
		croak("Attempt to write read-only property %s", "Drawable::size");
	size. x = var-> w;
	size. y = var-> h;
	return size;
}

int
Drawable_width( Handle self, Bool set, int width)
{
	Point p = my-> get_size( self);
	if ( !set)
		return p. x;
	p. x = width;
	my-> set_size( self, p);
	return width;
}

Bool
Drawable_put_image_indirect( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
	Bool ok;
	CHECK_GP(false);
	if ( image == NULL_HANDLE) return false;
	if ( !(PObject(image)-> options.optSystemDrawable)) {
		warn("This method is not available on this class because it is not a system Drawable object. You need to implement your own");
		return false;
	}
	if ( xLen == xDestLen && yLen == yDestLen)
		ok = apc_gp_put_image( self, image, x, y, xFrom, yFrom, xLen, yLen, rop);
	else
		ok = apc_gp_stretch_image( self, image, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
	if ( !ok) perl_error();
	return ok;
}

PRGBColor
prima_read_palette( int * palSize, SV * palette)
{
	AV * av;
	int i, count;
	Byte * buf;

	if ( !SvROK( palette) || ( SvTYPE( SvRV( palette)) != SVt_PVAV)) {
		*palSize = 0;
		return NULL;
	}
	av = (AV *) SvRV( palette);
	count = av_len( av) + 1;
	*palSize = count / 3;
	count = *palSize * 3;
	if ( count == 0) return NULL;

	if ( !( buf = allocb( count))) return NULL;

	for ( i = 0; i < count; i++)
	{
		SV **itemHolder = av_fetch( av, i, 0);
		if ( itemHolder == NULL)
			return ( PRGBColor) buf;
		buf[ i] = SvIV( *itemHolder);
	}

	return ( PRGBColor) buf;
}

/* Properties */

int
Drawable_alpha( Handle self, Bool set, int alpha)
{
	if (!set) return apc_gp_get_alpha( self);
	if ( alpha < 0 ) alpha = 0;
	if ( alpha > 255 ) alpha = 255;
	apc_gp_set_alpha( self, alpha);
	return var->alpha = apc_gp_get_alpha(self);
}

Bool
Drawable_antialias( Handle self, Bool set, Bool aa)
{
	if (set) apc_gp_set_antialias( self, aa );
	return var->antialias = apc_gp_get_antialias( self );
}

Color
Drawable_backColor( Handle self, Bool set, Color color)
{
	if (!set) return apc_gp_get_back_color( self);
	apc_gp_set_back_color( self, color);
	return color;
}

Color
Drawable_color( Handle self, Bool set, Color color)
{
	if (!set) return apc_gp_get_color( self);
	apc_gp_set_color( self, color);
	return color;
}

Rect
Drawable_clipRect( Handle self, Bool set, Rect clipRect)
{
	if ( !set)
		return apc_gp_get_clip_rect( self);
	apc_gp_set_clip_rect( self, clipRect);
	return clipRect;
}

int
Drawable_fillMode( Handle self, Bool set, int fillMode)
{
	if (!set) return apc_gp_get_fill_mode( self);
	apc_gp_set_fill_mode( self, fillMode);
	return fillMode;
}

SV *
Drawable_fillPattern( Handle self, Bool set, SV * svpattern)
{
	int i;
	if ( !set) {
		AV * av;
		FillPattern * fp;
		if ( var-> fillPatternImage )
			return newSVsv( PObject(var->fillPatternImage)->mate );

		if ( !( fp = apc_gp_get_fill_pattern( self))) return NULL_SV;
		av = newAV();
		for ( i = 0; i < 8; i++) av_push( av, newSViv(( int) (*fp)[i]));
		return newRV_noinc(( SV *) av);
	} else {
		if ( var->fillPatternImage ) {
			unprotect_object(var-> fillPatternImage);
			var->fillPatternImage = NULL_HANDLE;
		}
		if ( SvROK( svpattern) && ( SvTYPE( SvRV( svpattern)) == SVt_PVAV)) {
			FillPattern fp;
			AV * av = ( AV *) SvRV( svpattern);
			if ( av_len( av) != 7) {
				warn("Illegal fillPattern passed to Drawable::fillPattern");
				return NULL_SV;
			}
			for ( i = 0; i < 8; i++) {
				SV ** holder = av_fetch( av, i, 0);
				if ( !holder) {
					warn("Array panic on Drawable::fillPattern");
					return NULL_SV;
				}
				fp[ i] = SvIV( *holder);
			}
			apc_gp_set_fill_pattern( self, fp);
		} else if ( SvROK( svpattern) && ( SvTYPE( SvRV( svpattern)) == SVt_PVHV)) {
			Handle h = gimme_the_mate(svpattern);
			if ( h && kind_of(h, CImage) && h != self && PObject(h)->stage == csNormal) {
				protect_object(var-> fillPatternImage = h);
			} else {
				warn("Drawable::fillPattern: object passed is not a Prima::Image descendant or is invalid");
				return NULL_SV;
			}
			if ( opt_InPaint)
				apc_gp_set_fill_image( self, h);
		} else {
			int id = SvIV( svpattern);
			if (( id < 0) || ( id > fpMaxId)) {
				warn("fillPattern index out of range passed to Drawable::fillPattern");
				return NULL_SV;
			}
			apc_gp_set_fill_pattern( self, fillPatterns[ id]);
		}
	}
	return NULL_SV;
}

Point
Drawable_fillPatternOffset( Handle self, Bool set, Point fpo)
{
	if (!set) return apc_gp_get_fill_pattern_offset( self);
	fpo. x %= 8;
	fpo. y %= 8;
	apc_gp_set_fill_pattern_offset( self, fpo);
	return fpo;
}


int
Drawable_lineEnd( Handle self, Bool set, int lineEnd)
{
	if (!set) return apc_gp_get_line_end( self);
	apc_gp_set_line_end( self, lineEnd);
	return lineEnd;
}

int
Drawable_lineJoin( Handle self, Bool set, int lineJoin)
{
	if (!set) return apc_gp_get_line_join( self);
	apc_gp_set_line_join( self, lineJoin);
	return lineJoin;
}

SV *
Drawable_linePattern( Handle self, Bool set, SV * pattern)
{
	if ( set) {
		STRLEN len;
		unsigned char *pat = ( unsigned char *) SvPV( pattern, len);
		if ( len > 255) len = 255;
		apc_gp_set_line_pattern( self, pat, len);
	} else {
		unsigned char ret[ 256];
		int len = apc_gp_get_line_pattern( self, ret);
		return newSVpvn((char*) ret, len);
	}
	return NULL_SV;
}


double
Drawable_lineWidth( Handle self, Bool set, double lineWidth)
{
	if (!set) return apc_gp_get_line_width( self);
	if ( lineWidth < 0.0 ) lineWidth = 0.0;
	apc_gp_set_line_width( self, lineWidth);
	return lineWidth;
}

double
Drawable_miterLimit( Handle self, Bool set, double miterLimit)
{
	if (!set) return apc_gp_get_miter_limit( self);
	apc_gp_set_miter_limit( self, miterLimit);
	return miterLimit;
}

SV *
Drawable_palette( Handle self, Bool set, SV * palette)
{
	int colors;
	if ( var-> stage > csFrozen) return NULL_SV;
	colors = var-> palSize;
	if ( set) {
		free( var-> palette);
		var-> palette = prima_read_palette( &var-> palSize, palette);
		if ( colors == 0 && var-> palSize == 0) return NULL_SV; /* do not bother apc */
		apc_gp_set_palette( self);
	} else {
		AV * av = newAV();
		int i;
		Byte * pal = ( Byte*) var-> palette;
		for ( i = 0; i < colors * 3; i++) av_push( av, newSViv( pal[ i]));
		return newRV_noinc(( SV *) av);
	}
	return NULL_SV;
}

SV *
Drawable_pixel( Handle self, Bool set, int x, int y, SV * color)
{
	CHECK_GP(0);
	if (!set)
		return newSViv( apc_gp_get_pixel( self, x, y));
	apc_gp_set_pixel( self, x, y, SvIV( color));
	return NULL_SV;
}

Handle
Drawable_region( Handle self, Bool set, Handle mask)
{
	if ( var-> stage > csFrozen) return NULL_HANDLE;
	if ( !is_opt(optSystemDrawable))
		return NULL_HANDLE;

	if ( set) {
		if ( mask && kind_of( mask, CRegion)) {
			apc_gp_set_region( self, mask);
			return NULL_HANDLE;
		}

		if ( mask && !kind_of( mask, CImage)) {
			warn("Illegal object reference passed to Drawable::region");
			return NULL_HANDLE;
		}

		if ( mask ) {
			Handle region;
			HV * profile = newHV();

			pset_H( image, mask );
			region = Object_create("Prima::Region", profile);
			sv_free(( SV *) profile);

			apc_gp_set_region(self, region);
			Object_destroy(region);

		} else
			apc_gp_set_region(self, NULL_HANDLE);

	} else if ( apc_gp_get_region( self, NULL_HANDLE)) {
		HV * profile = newHV();
		Handle i = Object_create( "Prima::Region", profile);
		sv_free(( SV *) profile);
		apc_gp_get_region( self, i);
		--SvREFCNT( SvRV((( PAnyObject) i)-> mate));
		return i;
	}

	return NULL_HANDLE;
}

int
Drawable_rop( Handle self, Bool set, int rop)
{
	if (!set) return apc_gp_get_rop( self);
	apc_gp_set_rop( self, rop);
	return rop;
}

int
Drawable_rop2( Handle self, Bool set, int rop2)
{
	if (!set) return apc_gp_get_rop2( self);
	apc_gp_set_rop2( self, rop2);
	return rop2;
}

Bool
Drawable_textOpaque( Handle self, Bool set, Bool opaque)
{
	if (!set) return apc_gp_get_text_opaque( self);
	apc_gp_set_text_opaque( self, opaque);
	return opaque;
}

Bool
Drawable_textOutBaseline( Handle self, Bool set, Bool textOutBaseline)
{
	if (!set) return apc_gp_get_text_out_baseline( self);
	apc_gp_set_text_out_baseline( self, textOutBaseline);
	return textOutBaseline;
}

Point
Drawable_translate( Handle self, Bool set, Point translate)
{
	if (!set) return apc_gp_get_transform( self);
	apc_gp_set_transform( self, translate. x, translate. y);
	return translate;
}

#ifdef __cplusplus
}
#endif
