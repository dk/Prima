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

static void
prepare_matrix( Handle self, Matrix ctx)
{
	if ( my-> matrix == Drawable_matrix) {
		Matrix *matrix = apc_gp_get_matrix(self);
		if ( matrix )
			memcpy( ctx, matrix, sizeof(Matrix));
		else {
			bzero(ctx, sizeof(Matrix));
			ctx[0] = ctx[3] = 1.0;
		}
	} else
		prima_matrix_read_sv( my->get_matrix( self), ctx );
}

SV *
Image_pixel( Handle self, Bool set, int x, int y, SV * pixel)
{
	Point pt;
	Matrix matrix;

#define BGRto32(pal) ((var->palette[pal].r<<16) | (var->palette[pal].g<<8) | (var->palette[pal].b))
	if (!set) {
		if ( opt_InPaint)
			return inherited pixel(self,false,x,y,pixel);

		prepare_matrix(self, matrix);
		pt = prima_matrix_apply_to_int( matrix, x, y );
		x = pt.x;
		y = pt.y;

		if ((x>=var->w) || (x<0) || (y>=var->h) || (y<0))
			return newSViv( clInvalid);

		if ( var-> type & (imComplexNumber|imTrigComplexNumber)) {
			AV * av = newAV();
			switch ( var-> type) {
			case imComplex:
			case imTrigComplex: {
				float * f = (float*)(var->data + (var->lineSize*y+x*2*sizeof(float)));
				av_push( av, newSVnv( *(f++)));
				av_push( av, newSVnv( *f));
				break;
			}
			case imDComplex:
			case imTrigDComplex: {
				double * f = (double*)(var->data + (var->lineSize*y+x*2*sizeof(double)));
				av_push( av, newSVnv( *(f++)));
				av_push( av, newSVnv( *f));
				break;
			}
			}
			return newRV_noinc(( SV*) av);
		} else if ( var-> type & imRealNumber) {
			switch ( var-> type) {
			case imFloat:
				return newSVnv(*(float*)(var->data + (var->lineSize*y+x*sizeof(float))));
			case imDouble:
				return newSVnv(*(double*)(var->data + (var->lineSize*y+x*sizeof(double))));
			default:
				return NULL_SV;
		}} else
			switch (var->type & imBPP) {
			case imbpp1: {
				Byte p=var->data[var->lineSize*y+(x>>3)];
				p=(p >> (7-(x & 7))) & 1;
				return newSViv(((var->type & imGrayScale) ? (p ? 255 : 0) : BGRto32(p)));
			}
			case imbpp4: {
				Byte p=var->data[var->lineSize*y+(x>>1)];
				p=(x&1) ? p & 0x0f : p>>4;
				return newSViv(((var->type & imGrayScale) ? (p*255L)/15 : BGRto32(p)));
			}
			case imbpp8: {
				Byte p=var->data[var->lineSize*y+x];
				return newSViv(((var->type & imGrayScale) ? p :  BGRto32(p)));
			}
			case imbpp16: {
				return newSViv(*(Short*)(var->data + (var->lineSize*y+x*2)));
			}
			case imbpp24: {
				RGBColor p=*(PRGBColor)(var->data + (var->lineSize*y+x*3));
				return newSViv((p.r<<16) | (p.g<<8) | p.b);
			}
			case imbpp32:
				return newSViv(*(Long*)(var->data + (var->lineSize*y+x*4)));
			default:
				return newSViv(clInvalid);
		}
#undef BGRto32
	} else {
		ColorPixel color;
		int bpp = (var->type & imBPP) / 8;
		if ( is_opt( optInDraw))
			return inherited pixel(self,true,x,y,pixel);

		prepare_matrix(self, matrix);
		pt = prima_matrix_apply_to_int( matrix, x, y );
		x = pt.x;
		y = pt.y;

		if ((x>=var->w) || (x<0) || (y>=var->h) || (y<0))
			return NULL_SV;

		if ( !Image_read_pixel(self, pixel, &color))
			return NULL_SV;

		if ( var-> type & (imComplexNumber|imTrigComplexNumber)) {
			memcpy( var->data + ( var->lineSize * y + x * bpp ), color, bpp);
			return NULL_SV;
		} else if ( var-> type & imRealNumber) {
			memcpy( var->data + ( var->lineSize * y + x * bpp ), color, bpp);
			my->update_change( self);
			return NULL_SV;
		}

		switch (var->type & imBPP) {
		case imbpp1  : {
			int x1   = 7 - ( x & 7 );
			Byte p   = *((Byte*)&color);
			Byte *pd = var->data + (var->lineSize * y + ( x >> 3 ));
			*pd &= ~(1 << x1);
			*pd |= (p << x1);
			break;
		}
		case imbpp4  : {
			Byte p   = *((Byte*)&color);
			Byte *pd = var->data + ( var->lineSize * y + ( x >> 1 ));
			if ( x & 1 ) {
				*pd &= 0xf0;
			} else {
				p <<= 4;
				*pd &= 0x0f;
			}
			*pd |= p;
			break;
		}
		case imbpp8:
			var->data[ var->lineSize * y + x ] = *((Byte*)&color);
			break;
		default:
			memcpy(var->data + (var->lineSize * y + x * bpp), &color, bpp);
		}
		my->update_change( self);
		return NULL_SV;
	}
}


Bool
Image_read_pixel( Handle self, SV * pixel, ColorPixel *output )
{
	Color color;
	RGBColor rgb;

	if ( var-> type & (imComplexNumber|imTrigComplexNumber)) {
		if ( !SvROK( pixel) || ( SvTYPE( SvRV( pixel)) != SVt_PVAV)) {
			switch ( var-> type) {
			case imComplex:
			case imTrigComplex:
				*((float*)(output)) = SvNV(pixel);
				return true;
			case imDComplex:
			case imTrigDComplex:
				*((double*)(output)) = SvNV(pixel);
				return true;
			default:
				return false;
			}
		} else {
			AV * av = (AV *) SvRV( pixel);
			SV **sv[2];
			sv[0] = av_fetch( av, 0, 0);
			sv[1] = av_fetch( av, 1, 0);

			switch ( var-> type) {
			case imComplex:
			case imTrigComplex:
				((float*)(output))[0] = sv[0] ? SvNV(*(sv[0])) : 0.0;
				((float*)(output))[1] = sv[1] ? SvNV(*(sv[1])) : 0.0;
				return true;
			case imDComplex:
			case imTrigDComplex:
				((double*)(output))[0] = sv[0] ? SvNV(*(sv[0])) : 0.0;
				((double*)(output))[1] = sv[1] ? SvNV(*(sv[1])) : 0.0;
				return true;
			default:
				return false;
			}
		}
	} else if ( var-> type & imRealNumber) {
		switch ( var-> type) {
		case imFloat:
			*((float*)(output)) = SvNV(pixel);
			return true;
		case imDouble:
			*((double*)(output)) = SvNV(pixel);
			return true;
		default:
			return false;
		}
	}

#define LONGtoBGR(lv,clr)   (\
	(clr).b =  ( lv )         & 0xff, \
	(clr).g = (( lv ) >>  8 ) & 0xff, \
	(clr).r = (( lv ) >> 16 ) & 0xff, \
	(clr)) \

	color = SvIV( pixel);
	switch (var->type & imBPP) {
	case imbpp1 :
		*((Byte*)(output)) = ((var->type & imGrayScale) ?
			color / 255 :
			cm_nearest_color( LONGtoBGR(color,rgb), var->palSize, var->palette))
		& 1;
		return true;
	case imbpp4  :
		*((Byte*)(output)) = (var->type & imGrayScale) ?
			( color * 15 ) / 255 :
			cm_nearest_color( LONGtoBGR(color,rgb), var->palSize, var->palette) ;
		return true;
	case imbpp8:
		*((Byte*)(output)) = (var->type & imGrayScale) ?
			color :
			cm_nearest_color( LONGtoBGR(color,rgb), var->palSize, var->palette);
		return true;
	case imbpp16 : {
		int32_t color = SvIV( pixel);
		if ( color > INT16_MAX ) color = INT16_MAX;
		if ( color < INT16_MIN ) color = INT16_MIN;
		*((Short*)(output)) = color;
		return true;
	}
	case imbpp24 :
		(void) LONGtoBGR(color,rgb);
		memcpy(output, &rgb, sizeof(RGBColor));
		return true;
	case imbpp32 : {
		IV color = SvIV(pixel);
		if ( color > INT32_MAX ) color = INT32_MAX;
		if ( color < INT32_MIN ) color = INT32_MIN;
		*((Long*)(output)) = color;
		return true;
	}
	default:
		return false;
	}
#undef LONGtoBGR
}


static void
prepare_fill_context(Handle self, Point translate, PImgPaintContext ctx)
{
	FillPattern * p = &ctx->pattern;

	bzero(ctx, sizeof(ImgPaintContext));
	Image_color2pixel( self, my->get_color(self), ctx->color);
	Image_color2pixel( self, my->get_backColor(self), ctx->backColor);

	ctx-> rop = var-> extraROP;
	if ( var->alpha < 255 ) {
		ctx-> rop &= ~(0xff << ropSrcAlphaShift);
		ctx-> rop |= ropSrcAlpha | ( var-> alpha << ropSrcAlphaShift );
	}
	ctx-> region = var->regionData ? &var->regionData-> data. box : NULL;
	ctx-> patternOffset = my->get_fillPatternOffset(self);
	ctx-> transparent = my->get_rop2(self) == ropNoOper;

	ctx-> tile = NULL_HANDLE;
	if ( var-> fillPatternImage ) {
		memset( p, 0xff, sizeof(FillPattern));
		ctx-> tile = var-> fillPatternImage;
	} else if ( my-> fillPattern == Drawable_fillPattern) {
		FillPattern * fp = apc_gp_get_fill_pattern( self);
		if ( fp )
			memcpy( p, fp, sizeof(FillPattern));
		else 
			memset( p, 0xff, sizeof(FillPattern));
	} else {
		AV * av;
		SV * fp;
		fp = my->get_fillPattern( self);
		if ( fp && SvOK(fp) && SvROK(fp) && SvTYPE(av = (AV*)SvRV(fp)) == SVt_PVAV && av_len(av) == sizeof(FillPattern) - 1) {
			int i;
			for ( i = 0; i < 8; i++) {
				SV ** sv = av_fetch( av, i, 0);
				(*p)[i] = (sv && *sv && SvOK(*sv)) ? SvIV(*sv) : 0;
			}
		} else {
			warn("Bad array returned by .fillPattern");
			memset( p, 0xff, sizeof(FillPattern));
		}
	}
}

static void
prepare_line_context( Handle self, unsigned char * lp, ImgPaintContext * ctx)
{
	bzero(ctx, sizeof(ImgPaintContext));
	Image_color2pixel( self, my->get_color(self), ctx->color);
	Image_color2pixel( self, my->get_backColor(self), ctx->backColor);
	ctx-> rop = var-> extraROP;
	if ( var->alpha < 255 ) {
		ctx-> rop &= ~(0xff << ropSrcAlphaShift);
		ctx-> rop |= ropSrcAlpha | ( var-> alpha << ropSrcAlphaShift );
	}
	ctx->region = var->regionData ? &var->regionData-> data. box : NULL;
	ctx->transparent = my->get_rop2(self) == ropNoOper;
	ctx->translate = my->get_translate(self);
	if ( my-> linePattern == Drawable_linePattern) {
		int lplen;
		lplen = apc_gp_get_line_pattern( self, lp);
		lp[lplen] = 0;
	} else {
		SV * sv = my->get_linePattern(self);
		if ( sv && SvOK(sv)) {
			STRLEN lplen;
			char * lpsv = SvPV(sv, lplen);
			if ( lplen > 255 ) lplen = 255;
			memcpy(lp, lpsv, lplen);
			lp[lplen] = 0;
		} else 
			strcpy((char*) lp, (const char*) lpSolid);
	}
	ctx->linePattern = lp;
}


static Bool
primitive( Handle self, Bool fill, char * method, ...)
{
	Bool r;
	SV * ret;
	char format[256];
	va_list args;
	va_start( args, method);
	ENTER;
	SAVETMPS;
	strcpy(format, "<");
	strncat(format, method, 255);
	ret = call_perl_indirect( self, 
		var->antialias ?
			(fill ? "fill_imgaa_primitive" : "stroke_imgaa_primitive") :
			(fill ? "fill_img_primitive"   : "stroke_img_primitive"),
		format, true, false, args);
	va_end( args);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}

Bool
Image_arc( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	if ( opt_InPaint) return inherited arc(self, x, y, dX, dY, startAngle, endAngle);
	while ( startAngle > endAngle ) endAngle += 360.0;
	return primitive( self, 0, "snnnnnn", "arc", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Image_bar( Handle self, double x1, double y1, double x2, double y2)
{
	Point t;
	Bool ok;
	int _x1, _y1, _x2, _y2;
	ImgPaintContext ctx;
	Matrix matrix;
	NRect nrect = {x1,y1,x2,y2};
	NPoint npoly[4];

	if (opt_InPaint)
		return inherited bar( self, x1, y1, x2, y2);
	else if ( var-> antialias ) {
		ok = primitive( self, 1, "snnnn", "rectangle", x1, y1, x2, y2);
		my-> update_change(self);
		return ok;
	}

	prepare_matrix(self, matrix);
	if ( prima_matrix_is_square_rectangular( matrix, &nrect, npoly)) {
		_x1 = floor(nrect.left   + .5);
		_y1 = floor(nrect.bottom + .5);
		_x2 = floor(nrect.right  + .5);
		_y2 = floor(nrect.top    + .5);
	} else {
		SV *sv, *points;
		Matrix identity = {1.0,0.0,0.0,1.0,0.0,0.0};

		sv = prima_array_new(sizeof(npoly));
		memcpy( prima_array_get_storage(sv), npoly, sizeof(npoly));
		points = prima_array_tie( sv, sizeof(double), "d");
		apc_gp_set_matrix(self, identity);
		ok = primitive( self, 1, "sS", "line", points );
		apc_gp_set_matrix(self, matrix);
		sv_free(points);
		my-> update_change(self);
		return ok;
	}

	t = my->get_translate(self);
	_x1 += t.x;
	_y1 += t.y;

	prepare_fill_context(self, t, &ctx);
	ok = img_bar( self, _x1, _y1, _x2 - _x1 + 1, _y2 - _y1 + 1, &ctx);
	my-> update_change(self);
	return ok;
}

Bool
Image_bars( Handle self, SV * rects)
{
	Point t;
	ImgPaintContext ctx;
	int i, count;
	Bool ok = true, do_free;
	NRect *p, *r;
	if (opt_InPaint)
		return inherited bars( self, rects);
	else if ( var-> antialias ) {
		if (( p = prima_read_array( rects, "Image::bars", 'd', 4, 0, -1, &count, &do_free)) == NULL)
			return false;
		for ( i = 0, r = p; i < count; i++, r++) {
			ok = primitive( self, 1, "snnnn", "rectangle",
				r->left,
				r->bottom,
				r->right - r->left,
				r->top - r->bottom
			);
			if ( !ok ) break;
		}
		if ( do_free ) free( p);
	} else {
		Matrix matrix;
		SV *sv, *points = NULL;
		void *storage = NULL;
		Matrix identity = {1.0,0.0,0.0,1.0,0.0,0.0};
		Rect irect;

		if (( p = prima_read_array( rects, "Image::bars", 'd', 4, 0, -1, &count, &do_free)) == NULL)
			return false;
		t = my->get_translate(self);
		prepare_fill_context(self, t, &ctx);
		prepare_matrix(self, matrix);

		for ( i = 0, r = p; i < count; i++, r++) {
			ImgPaintContext ctx2 = ctx;
			NRect nrect = *r;
			NPoint npoly[4];
			if ( prima_matrix_is_square_rectangular( matrix, &nrect, npoly)) {
				irect.left  	 = floor(nrect.left   + .5);
				irect.bottom	 = floor(nrect.bottom + .5);
				irect.right 	 = floor(nrect.right  + .5);
				irect.top   	 = floor(nrect.top    + .5);
			} else {
				if ( points == NULL ) {
					sv = prima_array_new(sizeof(npoly));
					storage = prima_array_get_storage(sv);
					points = prima_array_tie( sv, sizeof(double), "d");
					apc_gp_set_matrix(self, identity);
				}
				memcpy( storage, npoly, sizeof(npoly));
				ok &= primitive( self, 1, "sS", "line", points );
				continue;
			}

			if ( !( ok &= img_bar( self,
				irect.left,
				irect.bottom,
				irect.right - irect.left   + 1,
				irect.top   - irect.bottom + 1,
				&ctx2))) break;
		}
		if ( do_free ) free( p);

		if ( points != NULL ) {
			apc_gp_set_matrix(self, matrix);
			sv_free(points);
		}
	}
	my-> update_change(self);
	return ok;
}

Bool
Image_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	if ( opt_InPaint) return inherited chord(self, x, y, dX, dY, startAngle, endAngle);
	return primitive( self, 0, "snnnnnn", "chord", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Image_clear(Handle self, double x1, double y1, double x2, double y2)
{
	Point t;
	Bool ok;
	ImgPaintContext ctx;
	int _x1, _y1, _x2, _y2;
	Bool full;

	full = x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0;
	if (opt_InPaint)
		return inherited clear( self, x1, y1, x2, y2);
	else if ( !full && var->antialias ) {
		Bool ok;
		if ( !my->graphic_context_push(self)) return false;
		apc_gp_set_color(self, apc_gp_get_back_color(self));
		apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
		ok = primitive( self, 1, "snnnn", "rectangle", x1, y1, x2, y2);
		my->graphic_context_pop(self);
		return ok;
	} else {
		NPoint npoly[4];
		Bool is_pure_rect = false;

		if ( full ) {
			_x1 = 0;
			_y1 = 0;
			_x2 = var-> w - 1;
			_y2 = var-> h - 1;
			is_pure_rect = true;
		} else {
			Matrix matrix;
			NRect nrect = {x1,y1,x2,y2};
			prepare_matrix(self, matrix);
			if ( prima_matrix_is_square_rectangular( matrix, &nrect, npoly)) {
				_x1 = floor(nrect.left   + .5);
				_y1 = floor(nrect.bottom + .5);
				_x2 = floor(nrect.right  + .5);
				_y2 = floor(nrect.top    + .5);
				is_pure_rect = true;
			}
		}

		if ( is_pure_rect ) {
			bzero(&ctx, sizeof(ctx));
			t = my->get_translate(self);
			_x1 += t.x;
			_y1 += t.y;
			Image_color2pixel( self, my->get_backColor(self), ctx.color);
			*ctx.backColor = *ctx.color;
			ctx.rop = my->get_rop(self);
			ctx.region = var->regionData ? &var->regionData-> data. box : NULL;
			memset( ctx.pattern, 0xff, sizeof(ctx.pattern));
			ctx.patternOffset.x = ctx.patternOffset.y = 0;
			ctx.patternOffset.x -= t.x;
			ctx.patternOffset.y -= t.y;
			ctx.transparent = false;

			ok = img_bar( self, _x1, _y1, _x2 - _x1 + 1, _y2 - _y1 + 1, &ctx);
		} else {
			SV *sv, *points;
			Matrix identity = {1.0,0.0,0.0,1.0,0.0,0.0};

			if ( !my->graphic_context_push(self)) return false;

			sv = prima_array_new(sizeof(npoly));
			memcpy( prima_array_get_storage(sv), npoly, sizeof(npoly));
			points = prima_array_tie( sv, sizeof(double), "d");
			apc_gp_set_matrix(self, identity);
			apc_gp_set_color(self, apc_gp_get_back_color(self));
			apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
			ok = primitive( self, 1, "sS", "line", points );
			sv_free(points);
			my-> graphic_context_pop(self);
			my-> update_change(self);
			return ok;
		}

	}
	my-> update_change(self);
	return ok;
}

Bool
Image_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	if ( opt_InPaint) return inherited ellipse(self, x, y, dX, dY);
	return primitive( self, 0, "snnnn", "ellipse", x, y, dX-1, dY-1);
}

Bool
Image_fill_chord( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	if ( opt_InPaint) return inherited fill_chord(self, x, y, dX, dY, startAngle, endAngle);
	return primitive( self, 1, "snnnnnn", "chord", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Image_fill_ellipse( Handle self, double x, double y,  double dX, double dY)
{
	if ( opt_InPaint) return inherited fill_ellipse(self, x, y, dX, dY);
	return primitive( self, 1, "snnnn", "ellipse", x, y, dX-1, dY-1);
}

Bool
Image_fillpoly( Handle self, SV * points)
{
	if ( opt_InPaint) return inherited fillpoly(self, points);
	return primitive( self, 1, "sS", "line", points );
}

Bool
Image_fill_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	if ( opt_InPaint) return inherited fill_sector(self, x, y, dX, dY, startAngle, endAngle);
	return primitive( self, 1, "snnnnnn", "sector", x, y, dX-1, dY-1, startAngle, endAngle);
}

Bool
Image_flood_fill( Handle self, int x, int y, Color color, Bool singleBorder)
{
	Point t, pp;
	Bool ok;
	ImgPaintContext ctx;
	ColorPixel px;
	Matrix matrix;
	if (opt_InPaint)
		return inherited flood_fill(self, x, y, color, singleBorder);

	t = my->get_translate(self);
	x += t.x;
	y += t.y;

	prepare_matrix(self, matrix);
	pp = prima_matrix_apply_to_int( matrix, x, y );
	prepare_fill_context(self, t, &ctx);
	Image_color2pixel( self, color, (Byte*)&px);
	ok = img_flood_fill( self, pp.x, pp.y, px, singleBorder, &ctx); 
	my-> update_change(self);
	return ok;
}

Bool
Image_line(Handle self, double x1, double y1, double x2, double y2)
{
	if ( opt_InPaint) {
		return inherited line(self, x1, y1, x2, y2);
	} else if ( !var->antialias && (int)(my->get_lineWidth(self) + .5) == 0) {
		ImgPaintContext ctx;
		Matrix matrix;
		unsigned char lp[256];
		Point poly[2];

		prepare_matrix(self, matrix);
		poly[0] = prima_matrix_apply_to_int( matrix, x1, y1);
		poly[1] = prima_matrix_apply_to_int( matrix, x2, y2);

		prepare_line_context( self, lp, &ctx);
		return img_polyline(self, 2, poly, &ctx);
	} else {
		return primitive( self, 0, "snnnn", "line", x1, y1, x2, y2);
	}
}

Bool
Image_lines( Handle self, SV * points)
{
	if ( opt_InPaint) {
		return inherited lines(self, points);
	} else if ( !var->antialias && (int)(my->get_lineWidth(self) + .5) == 0) {
		NPoint *lines, *p;
		int i, count;
		Bool ok = true, do_free;
		ImgPaintContext ctx, ctx2;
		unsigned char lp[256];
		Matrix matrix;

		if (( lines = prima_read_array( points, "Image::lines", 'd', 4, 0, -1, &count, &do_free)) == NULL)
			return false;
		prepare_line_context( self, lp, &ctx);
		prepare_matrix(self, matrix);
		for (i = 0, p = lines; i < count; i++, p += 2) {
			Point segment[2];
			ctx2 = ctx;
			prima_matrix_apply2_to_int( matrix, p, segment, 2 );
			if ( !( ok &= img_polyline(self, 2, segment, &ctx2))) break;
		}
		if (do_free) free(lines);
		return ok;
	} else {
		return primitive( self, 0, "sS", "lines", points );
	}
}

Bool
Image_polyline( Handle self, SV * points)
{
	if ( opt_InPaint) {
		return inherited polyline(self, points);
	} else if ( !var->antialias && (int)(my->get_lineWidth(self) + .5) == 0) {
		Point *lines;
		NPoint *raw_points;
		int count;
		Bool ok = false, free_raw_points;
		ImgPaintContext ctx;
		Matrix matrix;
		unsigned char lp[256];

		if (( raw_points = prima_read_array( points, "Image::polyline", 'd', 2, 2, -1, &count, &free_raw_points)) == NULL)
			return false;
		prepare_matrix(self, matrix);
		if (( lines = prima_matrix_transform_to_int( matrix, raw_points, free_raw_points, count)) == NULL )
			goto FAIL;
		prepare_line_context( self, lp, &ctx);
		ok = img_polyline(self, count, lines, &ctx);

	FAIL:
		if ( free_raw_points )
			free(raw_points);
		free( lines );
		return ok;
	} else {
		return primitive( self, 0, "sS", "line", points );
	}
}

Bool
Image_rectangle(Handle self, double x1, double y1, double x2, double y2)
{
	if ( opt_InPaint) {
		return inherited rectangle(self, x1, y1, x2, y2);
	} else if ( !var->antialias && (int)(my->get_lineWidth(self) + .5) == 0) {
		ImgPaintContext ctx;
		unsigned char lp[256];
		Matrix matrix;
		NPoint src[5] = { {x1,y1}, {x2,y1}, {x2,y2}, {x1,y2}, {x1,y1} };
		Point dst[5];

		prepare_matrix(self, matrix);
		prima_matrix_apply2_to_int(matrix, src, dst, 5);
		prepare_line_context( self, lp, &ctx);
		return img_polyline(self, 5, dst, &ctx);
	} else {
		return primitive( self, 0, "snnnn", "rectangle", x1, y1, x2, y2);
	}
}

Bool
Image_sector( Handle self, double x, double y, double dX, double dY, double startAngle, double endAngle)
{
	if ( opt_InPaint) return inherited sector(self, x, y, dX, dY, startAngle, endAngle);
	return primitive( self, 0, "snnnnnn", "sector", x, y, dX-1, dY-1, startAngle, endAngle);
}


#ifdef __cplusplus
}
#endif
