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
	r.type = rgnEmpty;

	inherited-> init( self, profile);

	if ( pexist( rect ) || pexist(ellipse) ) {
		int rect[4];
		if ( pexist(rect)) {
			prima_read_point( pget_sv( rect), rect, 4, "Array panic on 'rect'");
			r. type = rgnRectangle;
		} else {
			prima_read_point( pget_sv( ellipse), rect, 4, "Array panic on 'ellipse'");
			r. type = rgnEllipse;
		}
		r. data. box. x      = rect[0];
		r. data. box. y      = rect[1];
		r. data. box. width  = rect[2];
		r. data. box. height = rect[3];
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
