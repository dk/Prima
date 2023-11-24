#include "apricot.h"
#include "DeviceBitmap.h"
#include <DeviceBitmap.inc>

#ifdef __cplusplus
extern "C" {
#endif


#undef  my
#define inherited CDrawable->
#define my  ((( PDeviceBitmap) self)-> self)
#define var (( PDeviceBitmap) self)

void
DeviceBitmap_init( Handle self, HV * profile)
{
	dPROFILE;
	opt_set(optSystemDrawable);
	inherited init( self, profile);
	var-> w = pget_i( width);
	var-> h = pget_i( height);
	var-> type = pget_i( type);
	if ( !apc_dbm_create( self, var-> type))
		croak("Cannot create device bitmap");
	inherited begin_paint( self);
	opt_set( optInDraw);
	CORE_INIT_TRANSIENT(DeviceBitmap);
}

void
DeviceBitmap_done( Handle self)
{
	apc_dbm_destroy( self);
	inherited done( self);
}

Bool DeviceBitmap_begin_paint      ( Handle self) { return true;}
Bool DeviceBitmap_begin_paint_info ( Handle self) { return true;}
void DeviceBitmap_end_paint        ( Handle self) { return;}

int
DeviceBitmap_type( Handle self, Bool set, int type)
{
	if ( set)
		croak("Attempt to write read-only property %s", "DeviceBitmap::type");
	return var-> type;
}

static Handle xdup( Handle self, Bool icon)
{
	Handle h;
	PDrawable i;
	HV * profile = newHV();
	Point s;
	int rop;

	pset_H( owner,        var-> owner);
	pset_i( width,        var-> w);
	pset_i( height,       var-> h);
	if ( var-> type == dbtLayered) {
		pset_i( type,      imRGB);
		if ( icon ) {
			pset_i( maskType,  imbpp8);
			pset_i( autoMasking, 0);
		}
		rop = ropSrcCopy;
	} else {
		pset_i( type,      (var-> type == dbtBitmap) ? imBW : imRGB);
		rop = ropCopyPut;
	}

	h = Object_create( icon ? "Prima::Icon" : "Prima::Image", profile);
	sv_free(( SV *) profile);
	i = ( PDrawable) h;
	s = i-> self-> get_size( h);
	i-> options.optReadonlyPaint = 1;
	i-> self-> begin_paint( h);
	i-> options.optReadonlyPaint = 0;
	i-> self-> put_image_indirect( h, self, 0, 0, 0, 0, s.x, s.y, s.x, s.y, rop);
	i-> self-> end_paint( h);
	--SvREFCNT( SvRV( i-> mate));
	return h;
}

Handle DeviceBitmap_image( Handle self) { return xdup( self, false); }
Handle DeviceBitmap_icon( Handle self) { return xdup( self, true); }

int
DeviceBitmap_effective_rop( Handle self, int rop)
{
	if ( rop == ropDefault )
		rop = (var->type == dbtLayered) ? ropSrcCopy : ropCopyPut;
	return rop;
}

SV *
DeviceBitmap_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_dbm_get_handle( self));
	return newSVpv( buf, 0);
}

int
DeviceBitmap_get_paint_state( Handle self)
{
	return psEnabled;
}

int
DeviceBitmap_maskPixel( Handle self, Bool set, int x, int y, int pixel)
{
	Point pt;

	if ( var->type != dbtLayered )
		return 0;

	pt = prima_matrix_apply_to_int( var->current_state.matrix, x, y );
	x = pt.x;
	y = pt.y;
	if (x >= var->w || x < 0 || y >= var->h || y < 0)
		return clInvalid;

	if (set) {
		if ( pixel < 0 ) pixel = 0;
		if ( pixel > 255 ) pixel = 255;
		return apc_gp_set_mask_pixel( self, x, y, pixel );
	} else {
		return apc_gp_get_mask_pixel( self, x, y );
	}
}

#ifdef __cplusplus
}
#endif
