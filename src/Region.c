#include "apricot.h"
#include "Region.h"
#include <Region.inc>

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#define inherited CComponent
#define my  ((( PRegion) self)-> self)
#define var (( PRegion) self)

void
Region_init( Handle self, HV * profile)
{
	dPROFILE;
	RegionRec r;
	char *t;

	r.type = rgnEmpty;

	inherited-> init( self, profile);

	if ( pexist(rect)) {
		t = "rect";
		r.type = rgnRectangle;
	} else if (pexist(box)) {
		t = "box";
		r.type = rgnRectangle;
	} else if (pexist(ellipse)) {
		t = "ellipse";
		r.type = rgnEllipse;
	}
	

	if ( r. type != rgnEmpty ) {
		int rect[4];
		SV ** val = hv_fetch( profile, t, (I32) strlen( t), 0);
		prima_read_point( *val, rect, 4, "Array panic");

		r. data. box. x      = rect[0];
		r. data. box. y      = rect[1];
		if ( strncmp(t, "rect", 4) == 0 ) {
			r. data. box. width  = rect[2] - rect[0];
			r. data. box. height = rect[3] - rect[1];
		} else {
			r. data. box. width  = rect[2];
			r. data. box. height = rect[3];
		}
		if ( r.data.box.width <= 0 || r.data.box.height <= 0 )
			r. type = rgnEmpty;
	}
	if ( !apc_region_create( self, &r))
		croak("Cannot create region");
	CORE_INIT_TRANSIENT(Region);
}

void
Region_done( Handle self)
{
	apc_region_destroy( self);
	inherited-> done( self);
}

Bool
Region_equals( Handle self, Handle other_region)
{
	if ( !other_region) return false;
	if (PRegion(other_region)->stage > csNormal || !kind_of(other_region, CRegion))
		croak("Not a region passed");
	return apc_region_equals( self, other_region );
}

Bool
Region_combine( Handle self, Handle other_region, int rgnop)
{
	if ( !other_region) return false;
	if (PRegion(other_region)->stage > csNormal || !kind_of(other_region, CRegion))
		croak("Not a region passed");
	return apc_region_combine( self, other_region, rgnop );
}

#ifdef __cplusplus
}
#endif
