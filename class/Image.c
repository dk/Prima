#include "img.h"
#include "apricot.h"
#include "guts.h"
#include "Image.h"
#include "Image_private.h"
#include "Region.h"
#include "img_conv.h"
#include <Image.inc>

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#define inherited CDrawable->
#define my  ((( PImage) self)-> self)
#define var (( PImage) self)

static void Image_reset_notifications( Handle self);

void
Image_init( Handle self, HV * profile)
{
	dPROFILE;
	var-> updateLock = 0;
	opt_set(optSystemDrawable);
	inherited init( self, profile);
	var-> eventMask1 =
	( query_method( self, "on_headerready", 0) ? IMG_EVENTS_HEADER_READY : 0) |
	( query_method( self, "on_dataready",   0) ? IMG_EVENTS_DATA_READY   : 0);
	Image_reset_notifications( self);
	var->w = pget_i( width);
	var->h = pget_i( height);
	if ( var-> w < 0 ) var-> w = 0;
	if ( var-> h < 0 ) var-> h = 0;
	if ( !iconvtype_supported( var->conversion = pget_i( conversion) )) {
		warn("Invalid conversion: %d\n", var->conversion);
		var->conversion = ictNone;
	}
	var->scaling = pget_i( scaling);
	if ( var->scaling < istNone || var-> scaling > istMax) {
		warn("Invalid scaling: %d\n", var->scaling);
		var-> scaling = istNone;
	}
	if ( !itype_supported( var-> type = pget_i( type)))
		if ( !itype_importable( var-> type, &var-> type, NULL, NULL)) {
			warn( "Image::init: cannot set type %08x", var-> type);
			var-> type = imBW;
		}

	var->lineSize = LINE_SIZE(var->w, var->type);
	var->dataSize = ( var->lineSize) * var->h;
	if ( var-> dataSize > 0) {
		var->data = allocb( var->dataSize);
		memset( var-> data, 0, var-> dataSize);
		if ( var-> data == NULL) {
			my-> make_empty( self);
			croak("Image::init: cannot allocate %d bytes", var-> dataSize);
		}
	} else
		var-> data = NULL;
	var->palette = allocn( RGBColor, 256);
	if ( var-> palette == NULL) {
		free( var-> data);
		var-> data = NULL;
		croak("Image::init: cannot allocate %d bytes", 768);
	}
	if ( !Image_set_extended_data( self, profile))
		my-> set_data( self, pget_sv( data));
	opt_assign( optPreserveType, pget_B( preserveType));
	var->palSize = (1 << (var->type & imBPP)) & 0x1ff;
	if (!( var->type & imGrayScale) &&
		pexist( palette)) { /* palette might be killed by set_extended_data() */
		int ps = apc_img_read_palette( var->palette, pget_sv( palette), true);
		if ( ps) var-> palSize = ps;
	}

	{
		Point set;
		prima_read_point( pget_sv( resolution), (int*)&set, 2, "Array panic on 'resolution'");
		my-> set_resolution( self, set);
	}
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
	apc_image_create( self);
	my->update_change( self);
	CORE_INIT_TRANSIENT(Image);
}

void
Image_handle_event( Handle self, PEvent event)
{
	inherited handle_event ( self, event);
	if ( var-> stage > csNormal) return;
	switch ( event-> cmd) {
	case cmImageHeaderReady:
		my-> notify( self, "<sS", "HeaderReady", sv_2mortal(newRV((SV*) event-> gen. p)));
		break;
	case cmImageDataReady:
		my-> update_change( self);
		my-> notify( self, "<siiii", "DataReady",
			event-> gen. R. left,
			event-> gen. R. bottom,
			event-> gen. R. right - event-> gen. R. left   + 1,
			event-> gen. R. top   - event-> gen. R. bottom + 1);
		break;
	}
}

void
Image_done( Handle self)
{
	if ( var-> regionData ) {
		free(var->regionData);
		var->regionData = NULL;
	}
	if ( var->fillPatternImage ) {
		unprotect_object(var-> fillPatternImage);
		var->fillPatternImage = NULL_HANDLE;
	}
	apc_image_destroy( self);
	my->make_empty( self);
	inherited done( self);
}

Point
Image_resolution( Handle self, Bool set, Point resolution)
{
	if ( !set)
		return var-> resolution;
	if ( resolution. x <= 0 || resolution. y <= 0)
		resolution = apc_gp_get_resolution( prima_guts.application);
	var-> resolution = resolution;
	return resolution;
}

int
Image_scaling( Handle self, Bool set, int scaling)
{
	if ( !set)
		return var->scaling;
	if ( scaling < istNone || scaling > istMax ) {
		warn("Invalid scaling: %d", scaling);
		return false;
	}
	var->scaling = scaling;
	return false;
}

Point
Image_size( Handle self, Bool set, Point size)
{
	if ( !set)
		return inherited size( self, set, size);
	my-> stretch( self, size.x, size.y);
	return size;
}


Bool
Image_can_draw_alpha( Handle self)
{
	if ( is_opt( optInDrawInfo) )
		return false;
	else if ( is_opt( optInDraw))
		return apc_gp_can_draw_alpha(self);
	else
		return var->type == imByte || var->type == imRGB;
}

SV *
Image_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_image_get_handle( self));
	return newSVpv( buf, 0);
}

Color
Image_get_nearest_color( Handle self, Color color)
{
	Byte pal;
	RGBColor rgb, *pcolor;

	if ( is_opt( optInDrawInfo) || is_opt( optInDraw))
		return inherited get_nearest_color( self, color);

	switch ( var-> type & imCategory) {
	case imColor:
		if (( var-> type & imBPP) > 8)
			return color;
		rgb. b = color         & 0xFF;
		rgb. g = (color >> 8)  & 0xFF;
		rgb. r = (color >> 16) & 0xFF;
		break;
	case imGrayScale:
		rgb. r = rgb. g = rgb. b = (
			(color & 0xFF) +
			((color >> 8)  & 0xFF) +
			((color >> 16) & 0xFF)
		) / 3;
		break;
	default:
		return clInvalid; /* what else? */
	}

	pal    = cm_nearest_color( rgb, var-> palSize, var-> palette);
	pcolor = var->palette + pal;
	return ARGB( pcolor-> r, pcolor-> g, pcolor-> b);
}

SV *
Image_data( Handle self, Bool set, SV * svdata)
{
	void *data;
	STRLEN dataSize;

	if ( var->stage > csFrozen) return NULL_SV;

	if ( !set) {
		SV * sv = newSV_type(SVt_PV);
		SvREADONLY_on(sv);
		SvLEN_set(sv, 0); /* So Perl won't free it. */
		SvPV_set(sv, (char*)var-> data);
		SvCUR_set(sv, var-> dataSize);
		SvPOK_only(sv);
		return sv;
	}

	data = SvPV( svdata, dataSize);
	if ( is_opt( optInDraw) || dataSize <= 0) return NULL_SV;

	memcpy( var->data, data, (dataSize > (STRLEN)var->dataSize) ? (STRLEN)var->dataSize : dataSize);
	my-> update_change( self);
	return NULL_SV;
}

int
Image_lineSize( Handle self, Bool set, int dummy)
{
	if ( set)
		croak("Image::lineSize: attempt to write read-only property");

	return var-> lineSize;
}

int
Image_type( Handle self, Bool set, int type)
{
	HV * profile;
	if ( !set)
		return var->type;
	profile = newHV();
	pset_i( type, type);
	my-> set( self, profile);
	sv_free(( SV *) profile);
	return NULL_HANDLE;
}

int
Image_get_bpp( Handle self)
{
	return var->type & imBPP;
}

Bool
Image_begin_paint( Handle self)
{
	Bool ok;
	if ( var-> regionData ) {
		free(var->regionData);
		var->regionData = NULL;
	}
	if ( !inherited begin_paint( self))
		return false;
	if ( !( ok = apc_image_begin_paint( self))) {
		inherited end_paint( self);
		perl_error();
	}
	if (ok)
		apc_gp_set_antialias( self, var->antialias );
	return ok;
}

Bool
Image_begin_paint_info( Handle self)
{
	Bool ok;
	if ( is_opt( optInDraw))     return true;
	if ( var-> regionData ) {
		free(var->regionData);
		var->regionData = NULL;
	}
	if ( !inherited begin_paint_info( self))
		return false;
	if ( !( ok = apc_image_begin_paint_info( self))) {
		inherited end_paint_info( self);
		perl_error();
	}
	if (ok)
		apc_gp_set_antialias( self, var->antialias );
	return ok;
}

void
Image_end_paint( Handle self)
{
	int oldType = var->type;
	if ( !is_opt( optInDraw)) return;
	apc_image_end_paint( self);
	inherited end_paint( self);
	if ( is_opt( optPreserveType) && var->type != oldType) {
		my->reset( self, oldType, NULL, 0);
		return;
	}

	switch( var->type) {
	case imbpp1:
		if ( var-> palSize == 2 && memcmp( var->palette, stdmono_palette, sizeof( stdmono_palette)) == 0)
			var->type |= imGrayScale;
		break;
	case imbpp4:
		if ( var-> palSize == 16 && memcmp( var->palette, std16gray_palette, sizeof( std16gray_palette)) == 0)
			var->type |= imGrayScale;
		break;
	case imbpp8:
		if ( var-> palSize == 256 && memcmp( var->palette, std256gray_palette, sizeof( std256gray_palette)) == 0)
			var->type |= imGrayScale;
		break;
	}
	my->update_change( self);
}

void
Image_end_paint_info( Handle self)
{
	if ( !is_opt( optInDrawInfo)) return;
	apc_image_end_paint_info( self);
	inherited end_paint_info( self);
}

void
Image_update_change( Handle self)
{
	if ( var-> updateLock ) return;
	if ( var-> stage <= csNormal) apc_image_update_change( self);
	var->statsCache = 0;
}

typedef struct _PaintState
{
	DrawablePaintState parent_data;
	Bool antialias;
	int alpha, rop;
	PRegionRec region;
} PaintState, *PPaintState;

static void
gc_destroy( Handle self, void * user_data, unsigned int user_data_size, Bool in_paint)
{
	PPaintState state = ( PPaintState ) user_data;
	if ( state-> region ) free( state-> region );
}

Bool
Image_graphic_context_push(Handle self)
{
	PaintState state;

	if (opt_InPaint) return inherited graphic_context_push(self);

	state.parent_data = var-> current_state;
	state.alpha       = var-> alpha;
	state.antialias   = var-> antialias;
	state.rop         = var-> extraROP;
	state.region      = var-> regionData ? Region_clone_data(NULL_HANDLE, var->regionData) : NULL;

	return apc_gp_push(self, gc_destroy, &state, sizeof(state));
}

Bool
Image_graphic_context_pop(Handle self)
{
	PaintState state;
	if (opt_InPaint) return inherited graphic_context_pop(self);

	if (!apc_gp_pop( self, &state)) return false;

	var-> current_state = state.parent_data;
	var-> alpha         = state.alpha;
	var-> antialias     = state.antialias;
	var-> extraROP      = state.rop;
	if ( var-> regionData ) free( var-> regionData );
	var-> regionData = state.region;

	return true;
}

SV *
Image_palette( Handle self, Bool set, SV * palette)
{
	if ( var->stage > csFrozen) return NULL_SV;
	if ( set) {
		int ps;
		if ( var->type & imGrayScale) return NULL_SV;
		if ( !var->palette)           return NULL_SV;
		ps = apc_img_read_palette( var->palette, palette, true);

		if ( ps)
			var-> palSize = ps;
		else
			warn("Invalid array reference passed to Image::palette");
		my-> update_change( self);
	} else {
		int i;
		AV * av = newAV();
		int colors = ( 1 << ( var->type & imBPP)) & 0x1ff;
		Byte * pal = ( Byte*) var->palette;
		if (( var->type & imGrayScale) && (( var->type & imBPP) > imbpp8)) colors = 256;
		if ( var-> palSize < colors) colors = var-> palSize;
		for ( i = 0; i < colors*3; i++) av_push( av, newSViv( pal[ i]));
		return newRV_noinc(( SV *) av);
	}
	return NULL_SV;
}

int
Image_conversion( Handle self, Bool set, int conversion)
{
	if ( !set)
		return var-> conversion;
	if ( !iconvtype_supported(conversion))
		return var-> conversion;
	return var-> conversion = conversion;
}

Bool
Image_preserveType( Handle self, Bool set, Bool preserveType)
{
	if ( !set)
		return is_opt( optPreserveType);
	opt_assign( optPreserveType, preserveType);
	return false;
}

Handle
Image_bitmap( Handle self)
{
	Handle h;
	Point s;
	HV * profile = newHV();

	pset_H( owner,        var->owner);
	pset_i( width,        var->w);
	pset_i( height,       var->h);
	pset_sv_noinc( palette,     my->get_palette( self));
	pset_i( type,        (var-> type == imBW) ? dbtBitmap : dbtPixmap);
	h = Object_create( "Prima::DeviceBitmap", profile);
	sv_free(( SV *) profile);
	s = CDrawable( h)-> get_size( h);
	CDrawable( h)-> put_image_indirect( h, self, 0, 0, 0, 0, s.x, s.y, s.x, s.y, ropCopyPut);
	--SvREFCNT( SvRV( PDrawable( h)-> mate));
	return h;
}

Handle
Image_dup( Handle self)
{
	Handle h;
	PImage i;
	HV * profile = newHV();

	pset_H( owner,        var->owner);
	pset_i( width,        var->w);
	pset_i( height,       var->h);
	pset_i( type,         var->type);
	pset_i( conversion,   var->conversion);
	pset_i( scaling,      var->scaling);
	pset_i( preserveType, is_opt( optPreserveType));

	h = Object_create( var->self-> className, profile);
	sv_free(( SV *) profile);
	i = ( PImage) h;
	memcpy( i-> palette, var->palette, 768);
	i-> palSize = var-> palSize;
	if ( i-> type != var->type)
		croak("Image::dup consistency failed");
	else
		memcpy( i-> data, var->data, var->dataSize);
	memcpy( i-> stats, var->stats, sizeof( var->stats));
	i-> statsCache = var->statsCache;

	if ( var->mate && hv_exists(( HV*)SvRV( var-> mate), "extras", 6)) {
		SV ** sv = hv_fetch(( HV*)SvRV( var-> mate), "extras", 6, 0);
		if ( sv && SvOK( *sv) && SvROK( *sv) && SvTYPE( SvRV( *sv)) == SVt_PVHV)
			(void) hv_store(( HV*)SvRV( i-> mate), "extras", 6, newSVsv( *sv), 0);
	}

	--SvREFCNT( SvRV( i-> mate));
	return h;
}

Bool
Image_put_image_indirect( Handle self, Handle image, int x, int y, int xFrom, int yFrom, int xDestLen, int yDestLen, int xLen, int yLen, int rop)
{
	Bool ret;
	Byte * color = NULL, colorbuf[ MAX_SIZEOF_PIXEL ];
	Matrix * matrix = &var->current_state.matrix;

	if ( is_opt( optInDrawInfo)) return false;
	if ( image == NULL_HANDLE) return false;
	if ( is_opt( optInDraw))
		return inherited put_image_indirect( self, image, x, y, xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop);
	if ( !kind_of( image, CImage)) return false;

	if ( rop & ropConstantColor ) {
		bzero( colorbuf, MAX_SIZEOF_PIXEL );
		Image_color2pixel( self, my->get_color(self), colorbuf );
		color = colorbuf;
	}

	ret = img_put( self, image, x + (*matrix)[4], y + (*matrix)[5], xFrom, yFrom, xDestLen, yDestLen, xLen, yLen, rop,
		var->regionData ? &var->regionData-> data. box : NULL, color);
	my-> update_change( self);
	return ret;
}

UV
Image_add_notification( Handle self, char * name, SV * subroutine, Handle referer, int index)
{
	UV id = inherited add_notification( self, name, subroutine, referer, index);
	if ( id != 0) Image_reset_notifications( self);
	return id;
}

void
Image_remove_notification( Handle self, UV id)
{
	inherited remove_notification( self, id);
	Image_reset_notifications( self);
}

static void
Image_reset_notifications( Handle self)
{
	int i;
	PList  list;
	void * ret[ 2];
	int    cmd[ 2] = { IMG_EVENTS_HEADER_READY, IMG_EVENTS_DATA_READY };
	var-> eventMask2 = var-> eventMask1;
	if ( var-> eventIDs == NULL) return;

	ret[0] = hash_fetch( var-> eventIDs, "HeaderReady", 11);
	ret[1] = hash_fetch( var-> eventIDs, "DataReady",   9);

	for ( i = 0; i < 2; i++) {
		if ( ret[i] == NULL) continue;
		list = var-> events + PTR2IV( ret[i]) - 1;
		if ( list-> count > 0) var-> eventMask2 |= cmd[ i];
	}
}

Bool
Image_antialias( Handle self, Bool set, Bool aa)
{
	if ( is_opt(optInDraw) || is_opt(optInDrawInfo))
		return inherited antialias(self,set,aa);

	if (set) {
		if ( aa && !my->can_draw_alpha(self))
			aa = false;
		var-> antialias = aa;
	}
	return var->antialias;
}

Rect
Image_clipRect( Handle self, Bool set, Rect r)
{
	if ( is_opt(optInDraw) || is_opt(optInDrawInfo))
		return inherited clipRect(self,set,r);

	if ( var-> stage > csFrozen) return r;

	if ( set) {
		PRegionRec reg;
		if ( var-> regionData ) {
			free(var->regionData);
			var->regionData = NULL;
		}
		if ((reg = malloc(sizeof(RegionRec) + sizeof(Box))) != NULL) {
			Box *box;
			reg->type = rgnRectangle;
			reg-> data. box. n_boxes = 1;
			box = reg-> data. box. boxes = (Box*) (((Byte*)reg) + sizeof(RegionRec));
			box-> x = r.left;
			box-> y = r.bottom;
			box-> width  = r.right - r.left + 1;
			box-> height = r.top - r.bottom + 1;
			var->regionData = reg;
		}
	} else if ( var-> regionData ) {
		Box box   = img_region_box( &var->regionData->data.box);
		r.left    = box.x;
		r.bottom  = box.y;
		r.right   = box.x + box.width  - 1;
		r.top     = box.y + box.height - 1;
	} else {
		r.left    = r.bottom  = 0;
		r.right   = var-> w - 1;
		r.top     = var-> h - 1;
	}

	return r;
}

Handle
Image_region( Handle self, Bool set, Handle mask)
{
	if ( is_opt(optInDraw) || is_opt(optInDrawInfo))
		return inherited region(self,set,mask);

	if ( var-> stage > csFrozen) return NULL_HANDLE;

	if ( set) {
		if ( var-> regionData ) {
			free(var->regionData);
			var->regionData = NULL;
		}

		if ( mask && kind_of( mask, CRegion)) {
			var->regionData = CRegion(mask)->update_change(mask, true);
			return NULL_HANDLE;
		}

		if ( mask && !kind_of( mask, CImage)) {
			warn("Illegal object reference passed to Image::region");
			return NULL_HANDLE;
		}

		if ( mask ) {
			Handle region;
			HV * profile = newHV();
			pset_H( image, mask );
			region = Object_create("Prima::Region", profile);
			sv_free(( SV *) profile);
			var->regionData = CRegion(region)->update_change(region, true);
			Object_destroy(region);
		}

	} else if ( var-> regionData )
		return Region_create_from_data( NULL_HANDLE, var->regionData);

	return NULL_HANDLE;
}

int
Image_rop( Handle self, Bool set, int rop)
{
	if (!set) return var-> extraROP;
	if ( rop < 0 ) rop = 0;
	var-> extraROP = rop;
	if ( rop > ropNoOper ) rop = ropNoOper;
	apc_gp_set_rop( self, rop);
	return var-> extraROP;
}

#ifdef __cplusplus
}
#endif
