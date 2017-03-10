#include "apricot.h"
#include "Region.h"
#include "Image.h"
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
	Bool free_image = false, ok;

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
	} else if (pexist(polygon)) {
		r.type = rgnPolygon;
	} else if (pexist(image)) {
		r.type = rgnImage;
	}

	switch (r.type) {
	case rgnRectangle:
	case rgnEllipse: {
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
		break;
	}
	case rgnPolygon:
		if (( r. data. polygon. points = (Point*) read_array( 
			pget_sv(polygon), "Region::polygon", true, 
			2, 2, -1, 
			&r. data. polygon. n_points)
		) == NULL)
			croak("Bad polygon data");
		r. data. polygon. winding = pexist(winding) ? pget_B(winding) : 0;
		break;
	case rgnEmpty:
		break;
	case rgnImage:
		r. data. image = pget_H(image);
		if ( !kind_of( r. data. image, CImage ))
			croak("Not an image passed");
		if (( PImage(r.data.image)->type & imBPP ) != 1 ) {
			r.data.image = CImage(r.data.image)->dup(r.data.image);
			CImage(r.data.image)->set_type(r.data.image, imbpp1 | imGrayScale);
			free_image = true;
		}
	}
	ok = apc_region_create( self, &r);
	if ( r. type == rgnPolygon ) free( r. data. polygon. points );
	if ( free_image ) Object_destroy(r.data.image);
	CORE_INIT_TRANSIENT(Region);
	if (!ok)
		croak("Cannot create region");
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
