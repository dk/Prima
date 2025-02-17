#include "img.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include "apricot.h"
#include "Region.h"
#include "Image.h"
#include "private/Image.h"
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

SV *
Image_pixel( Handle self, Bool set, int x, int y, SV * pixel)
{
	Point pt;

#define BGRto32(pal) ((var->palette[pal].r<<16) | (var->palette[pal].g<<8) | (var->palette[pal].b))
	if (!set) {
		if ( is_opt( optInDraw))
			return inherited pixel(self,false,x,y,pixel);

		pt = prima_matrix_apply_to_int( VAR_MATRIX, x, y );
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

		pt = prima_matrix_apply_to_int( VAR_MATRIX, x, y );
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
prepare_fill_context(Handle self, PImgPaintContext ctx)
{
	FillPattern * p = &ctx->pattern;

	bzero(ctx, sizeof(ImgPaintContext));

	ctx-> rop = var-> extraROP;
	if ( ctx->rop >= ropMinPDFunc && ctx->rop <= ropMaxPDFunc) {
		ctx->rop &= ~(0xff << ropSrcAlphaShift);
		ctx->rop |= ropSrcAlpha | ( var-> alpha << ropSrcAlphaShift );
	}
	Image_color2pixel( self, Image_premultiply_color( self, ctx->rop, my->get_color(self)),     ctx->color);
	Image_color2pixel( self, Image_premultiply_color( self, ctx->rop, my->get_backColor(self)), ctx->backColor);

	ctx-> region = var->regionData;
	ctx-> patternOffset = my->get_fillPatternOffset(self);
	ctx-> transparent = my->get_rop2(self) == ropNoOper;

	ctx-> tile = NULL_HANDLE;
	if ( var-> fillPatternImage ) {
		memset( p, 0xff, sizeof(FillPattern));
		if ( PObject(var->fillPatternImage)->stage == csNormal)
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

	ctx-> rop = var-> extraROP;
	if ( ctx->rop >= ropMinPDFunc && ctx->rop <= ropMaxPDFunc ) {
		ctx->rop &= ~(0xff << ropSrcAlphaShift);
		ctx->rop |= ropSrcAlpha | ( var-> alpha << ropSrcAlphaShift );
	}
	Image_color2pixel( self, Image_premultiply_color( self, ctx->rop, my->get_color(self)),     ctx->color);
	Image_color2pixel( self, Image_premultiply_color( self, ctx->rop, my->get_backColor(self)), ctx->backColor);

	ctx->region = var->regionData;
	ctx->transparent = my->get_rop2(self) == ropNoOper;
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

Bool
Image_stroke_primitive( Handle self, char * method, ...)
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
		var->antialias ? "stroke_imgaa_primitive" : "stroke_img_primitive",
		format, true, false, args);
	va_end( args);
	r = ret ? SvTRUE( ret) : false;
	FREETMPS;
	LEAVE;
	return r;
}

static Bool
fill_noaapoly( Handle self, int n_pts, NPoint *pts)
{
	Bool ok = false;
	int mode;
	PRegionRec rgn = NULL;
	ImgPaintContext ctx;
	Point i_pts_buf[1024];
	semistatic_t pi_pts_buf;

	if ( n_pts < 2 )
		return false;

	mode = ( my-> fillMode == Drawable_fillMode) ?
		apc_gp_get_fill_mode(self) :
		my-> get_fillMode(self);
	prepare_fill_context(self, &ctx);

	semistatic_init(&pi_pts_buf, &i_pts_buf, sizeof(Point), 1024);
	if ( !semistatic_expand(&pi_pts_buf, n_pts)) {
		warn("no memory");
		goto EXIT;
	}

	prima_matrix_apply2_to_int(VAR_MATRIX, pts, (Point*)pi_pts_buf.heap, n_pts);
	if ( !( rgn = img_region_polygon( (Point*)pi_pts_buf.heap, n_pts, mode)))
		goto EXIT;

	if ( ctx.region != NULL ) {
		PRegionRec rgn3;
		if (( rgn3 = img_region_combine(rgn, ctx.region, rgnopIntersect)) == NULL)
			goto EXIT;
		free(rgn);
		rgn = rgn3;
#if 0
		{
			int i;
			Box *b = rgn->boxes;
			for ( i = 0; i < rgn->n_boxes; i++, b++) printf("> %d %d %d %d\n", b->x, b->y, b->width, b->height);
		}
#endif
	}

	ctx.region = rgn;
	if ( rgn->n_boxes > 0 ) {
		Box box = img_region_box(ctx.region);
		ok = img_bar( self, box.x, box.y, box.width, box.height, &ctx);
	} else
		ok = true;
EXIT:
	if ( rgn ) free( rgn );
	semistatic_done(&pi_pts_buf);
	return ok;
}

static Bool
fill_aapoly( Handle self, int n_pts, NPoint *pts)
{
	int mode;
	ImgPaintContext ctx;
	prepare_fill_context(self, &ctx);
	mode = ( my-> fillMode == Drawable_fillMode) ?
		apc_gp_get_fill_mode(self) :
		my-> get_fillMode(self);
	if ( ctx.rop == ropDefault || ctx.rop == ropCopyPut )
		ctx.rop = ropSrcOver | ropSrcAlpha | ( var->alpha << ropSrcAlphaShift );
	if ( !prima_matrix_is_identity(VAR_MATRIX))
		prima_matrix_apply2(VAR_MATRIX, pts, pts, n_pts);
	return img_aafill( self, pts, n_pts, mode, &ctx);
}

Bool
Image_fill_poly( Handle self, int n_pts, NPoint *pts)
{
	return var->antialias ?
		fill_aapoly( self, n_pts, pts) :
		fill_noaapoly( self, n_pts, pts);
}

Bool
Image_fill_rect( Handle self, double x1, double y1, double x2, double y2)
{
	NPoint p[4] = {{x1,y1},{x2,y1},{x2,y2},{x1,y2}};
	return Image_fill_poly( self, 4, p);
}

Bool
Image_bar( Handle self, double x1, double y1, double x2, double y2)
{
	Bool ok = false;
	ImgPaintContext ctx;
	NRect nrect = {x1,y1,x2,y2};
	NPoint npoly[4];

	if (opt_InPaint)
		return inherited bar( self, x1, y1, x2, y2);
	else if ( var-> antialias || !prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly))
		ok = Image_fill_rect( self, x1, y1, x2, y2);
	else {
		Rect r;
		prima_array_convert( 4, &nrect, 'd', &r, 'i');
		prepare_fill_context(self, &ctx);
		ok = img_bar( self, r.left, r.bottom, r.right - r.left + 1, r.top - r.bottom + 1, &ctx);
	}
	my-> update_change(self);
	return ok;
}

Bool
Image_bar_alpha(Handle self, int alpha, int x1, int y1, int x2, int y2)
{
	if (opt_InPaint)
		return inherited bar_alpha( self, alpha, x1, y1, x2, y2);
	else
		return false;
}

Bool
Image_bars( Handle self, SV * rects)
{
	ImgPaintContext ctx;
	int i, count;
	Bool ok = true, do_free, got_ctx = false;
	NRect *p, *r;
	NPoint npoly[4];

	if (opt_InPaint)
		return inherited bars( self, rects);

	if (( p = prima_read_array( rects, "Image::bars", 'd', 4, 0, -1, &count, &do_free)) == NULL)
		return false;

	for ( i = 0, r = p; i < count; i++, r++) {
		NRect nrect = *r;
		if ( var-> antialias || !prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly))
			ok = Image_fill_rect( self,
				r->left,
				r->bottom,
				r->right,
				r->top
			);
		else {
			Rect r;
			ImgPaintContext ctx2;
			if ( !got_ctx ) {
				prepare_fill_context(self, &ctx);
				got_ctx = true;
			}
			ctx2 = ctx;
			prima_array_convert( 4, &nrect, 'd', &r, 'i');
			ok = img_bar( self, r.left, r.bottom, r.right - r.left + 1, r.top - r.bottom + 1, &ctx2);
		}
		if ( !ok ) break;
	}

	if ( do_free ) free( p);
	my-> update_change(self);
	return ok;
}

Bool
Image_clear(Handle self, double x1, double y1, double x2, double y2)
{
	Bool ok;
	Bool full;
	NPoint npoly[4];
	NRect nrect = {x1,y1,x2,y2};

	full = x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0;
	if (opt_InPaint)
		return inherited clear( self, x1, y1, x2, y2);
	else if (
		!full &&
		(var->antialias || !prima_matrix_is_square_rectangular( VAR_MATRIX, &nrect, npoly))
	) {
		if ( !my->graphic_context_push(self)) return false;
		apc_gp_set_color(self, apc_gp_get_back_color(self));
		apc_gp_set_fill_pattern(self, fillPatterns[fpSolid]);
		ok = Image_fill_rect( self, x1, y1, x2, y2);
		my->graphic_context_pop(self);
	} else {
		Rect r;
		ImgPaintContext ctx;
		if ( full ) {
			r.left  = r.bottom = 0;
			r.right = var-> w - 1;
			r.top   = var-> h - 1;
		} else
			prima_array_convert( 4, &nrect, 'd', &r, 'i');

		bzero(&ctx, sizeof(ctx));
		Image_color2pixel( self, my->get_backColor(self), ctx.color);
		*ctx.backColor      = *ctx.color;
		ctx.rop             = my->get_rop(self);
		ctx.region          = var->regionData;
		ctx.patternOffset.x = ctx.patternOffset.y = 0;
		ctx.transparent     = false;
		memset( ctx.pattern, 0xff, sizeof(ctx.pattern));
		ok = img_bar( self, r.left, r.bottom, r.right - r.left + 1, r.top - r.bottom + 1, &ctx);
	}
	my-> update_change(self);
	return ok;
}

Bool
Image_fillpoly( Handle self, SV * points)
{
	if ( opt_InPaint)
		return inherited fillpoly(self, points);
	else {
		int n;
		Bool ok;
		NPoint *p;
		if ((( p = (NPoint*) prima_read_array( points, "fillpoly", 'd', 2, 2, -1, &n, NULL))) == NULL)
			return false;
		ok = Image_fill_poly( self, n, p);
		free(p);
		return ok;
	}
}

Bool
Image_flood_fill( Handle self, int x, int y, Color color, Bool singleBorder)
{
	Point pp;
	Bool ok;
	ImgPaintContext ctx;
	ColorPixel px;
	if (opt_InPaint)
		return inherited flood_fill(self, x, y, color, singleBorder);

	pp = prima_matrix_apply_to_int( VAR_MATRIX, x, y );
	prepare_fill_context(self, &ctx);
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
		unsigned char lp[256];
		Point poly[2];

		poly[0] = prima_matrix_apply_to_int( VAR_MATRIX, x1, y1);
		poly[1] = prima_matrix_apply_to_int( VAR_MATRIX, x2, y2);

		prepare_line_context( self, lp, &ctx);
		return img_polyline(self, 2, poly, &ctx);
	} else {
		return Image_stroke_primitive( self, "snnnn", "line", x1, y1, x2, y2);
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

		if (( lines = prima_read_array( points, "Image::lines", 'd', 4, 0, -1, &count, &do_free)) == NULL)
			return false;
		prepare_line_context( self, lp, &ctx);
		for (i = 0, p = lines; i < count; i++, p += 2) {
			Point segment[2];
			ctx2 = ctx;
			prima_matrix_apply2_to_int( VAR_MATRIX, p, segment, 2 );
			if ( !( ok &= img_polyline(self, 2, segment, &ctx2))) break;
		}
		if (do_free) free(lines);
		return ok;
	} else {
		return Image_stroke_primitive( self, "sS", "lines", points );
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
		unsigned char lp[256];

		if (( raw_points = prima_read_array( points, "Image::polyline", 'd', 2, 2, -1, &count, &free_raw_points)) == NULL)
			return false;
		if (( lines = prima_matrix_transform_to_int( VAR_MATRIX, raw_points, free_raw_points, count)) == NULL )
			goto FAIL;
		prepare_line_context( self, lp, &ctx);
		ok = img_polyline(self, count, lines, &ctx);

	FAIL:
		if ( free_raw_points )
			free(raw_points);
		free( lines );
		return ok;
	} else {
		return Image_stroke_primitive( self, "sS", "line", points );
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
		NPoint src[5] = { {x1,y1}, {x2,y1}, {x2,y2}, {x1,y2}, {x1,y1} };
		Point dst[5];

		prima_matrix_apply2_to_int(VAR_MATRIX, src, dst, 5);
		prepare_line_context( self, lp, &ctx);
		return img_polyline(self, 5, dst, &ctx);
	} else {
		return Image_stroke_primitive( self, "snnnn", "rectangle", x1, y1, x2, y2);
	}
}

#ifdef __cplusplus
}
#endif
