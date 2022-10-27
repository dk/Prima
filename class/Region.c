#include "apricot.h"
#include "Region.h"
#include "img_conv.h"
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
	char *t = NULL;
	Bool free_image = false, ok;

	r. type = rgnEmpty;

	inherited-> init( self, profile);

	if ( pexist(rect)) {
		t = "rect";
		r. type = rgnRectangle;
	} else if (pexist(box)) {
		t = "box";
		r. type = rgnRectangle;
	} else if (pexist(polygon)) {
		r. type = rgnPolygon;
	} else if (pexist(image)) {
		r. type = rgnImage;
	}

	switch (r. type) {
	case rgnRectangle: {
		SV ** box_entry = hv_fetch( profile, t, (I32) strlen(t), 0);
		if (( r. data. box. boxes = (Box*) prima_read_array(
			*box_entry, "Region::new", 'i',
			4, 1, -1,
			&r. data. box. n_boxes, NULL
		)) == NULL) {
			r. type = rgnEmpty;
			break;
		}
		if ( strncmp(t, "rect", 4) == 0 ) {
			int i;
			Box * box = r. data. box. boxes;
			for ( i = 0; i < r. data. box. n_boxes; i++, box++) {
				box-> width  -= box-> x;
				box-> height -= box-> y;
			}
		}
		break;
	}
	case rgnPolygon:
		if (( r. data. polygon. points = (Point*) prima_read_array(
			pget_sv(polygon), "Region::polygon", 'i',
			2, 2, -1,
			&r. data. polygon. n_points, NULL)
		) == NULL) {
			r. type = rgnEmpty;
			break;
		}
		r. data. polygon. fill_mode = pexist(fillMode) ? pget_i(fillMode) : (fmOverlay | fmWinding);
		break;
	case rgnEmpty:
		break;
	case rgnImage:
		r. data. image = pget_H(image);
		if ( !kind_of( r. data. image, CImage )) {
			warn("Not an image passed");
			r. type = rgnEmpty;
			goto CREATE;
		}
		if (( PImage(r.data.image)->type & imBPP ) != 1 ) {
			r.data.image = CImage(r.data.image)->dup(r.data.image);
			CImage(r.data.image)->set_conversion(r.data.image, ictNone);
			CImage(r.data.image)->set_type(r.data.image, imbpp1 | imGrayScale);
			free_image = true;
		}
	}
CREATE:
	if ( r.type == rgnPolygon ) {
		PBoxRegionRec x;
		if ( !( x = img_region_polygon( r.data.polygon.points, r.data.polygon.n_points, r.data.polygon.fill_mode)))
			ok = false;
		if ( ok ) {
			ok = apc_region_create_boxes(self, x);
			free( x);
		}
	} else {
		ok = apc_region_create( self, &r);
	}
	if ( r. type == rgnPolygon   ) free( r. data. polygon. points );
	if ( r. type == rgnRectangle ) free( r. data. box. boxes );
	if ( free_image ) Object_destroy(r.data.image);
	opt_set( optDirtyRegion);
	CORE_INIT_TRANSIENT(Region);
	if (!ok)
		warn("Cannot create region");
}

Handle
Region_create_from_data( Handle self, PRegionRec data)
{
	Bool ok;
	HV * profile = newHV();
	self = Object_create( "Prima::Region", profile);
	apc_region_destroy(self);
	ok = apc_region_create( self, data);
	opt_set( optDirtyRegion);
	sv_free(( SV *) profile);
	--SvREFCNT( SvRV(var-> mate));
	if (!ok) {
 		warn("Cannot create region");
		return NULL_HANDLE;
	}
	return self;
}

PRegionRec
Region_clone_data( Handle self, PRegionRec data)
{
	int size, extras;
	PRegionRec copy;

	size = sizeof(RegionRec);
	extras = 0;
	switch (data->type) {
	case rgnRectangle:
		extras = data->data.box.n_boxes * sizeof(Box);
		break;
	case rgnPolygon:
		extras = data->data.polygon.n_points * sizeof(Point);
		break;
	}

	size += extras;
	if ( !( copy = malloc(size))) return NULL;
	memcpy(copy, data, sizeof(RegionRec));

	switch (data->type) {
	case rgnRectangle:
		copy->data.box.boxes = (Box*) (((Byte*)copy) + sizeof(RegionRec));
		memcpy( copy->data.box.boxes, data->data.box.boxes, extras);
		break;
	case rgnPolygon:
		copy->data.polygon.points = (Point*) (((Byte*)copy) + sizeof(RegionRec));
		memcpy( copy->data.polygon.points, data->data.polygon.points, extras);
		break;
	}

	return copy;
}

void
Region_done( Handle self)
{
	if ( var->rects != NULL ) {
		free(var->rects);
		var->rects = NULL;
	}
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
Region_offset( Handle self, int dx, int dy)
{
	opt_set( optDirtyRegion);
	return apc_region_offset( self, dx, dy);
}

Bool
Region_combine( Handle self, Handle other_region, int rgnop)
{
	if ( !other_region) return false;
	if (PRegion(other_region)->stage > csNormal || !kind_of(other_region, CRegion))
		croak("Not a region passed");
	opt_set( optDirtyRegion);
	return apc_region_combine( self, other_region, rgnop );
}

SV *
Region_get_handle( Handle self)
{
	char buf[ 256];
	snprintf( buf, 256, PR_HANDLE_FMT, apc_region_get_handle( self));
	return newSVpv( buf, 0);
}

SV *
Region_get_boxes( Handle self)
{
	SV *ret;
	PRegionRec data;

	if (( data = my->update_change(self, false)) == NULL)
		return NULL_SV;
	if (( ret = prima_array_new(data-> data. box. n_boxes * sizeof(Box))) == NULL)
		return NULL_SV;
	memcpy( prima_array_get_storage(ret), data->data.box.boxes, data-> data. box. n_boxes * sizeof(Box));
	return prima_array_tie( ret, sizeof(int), "i");
}

PRegionRec
Region_update_change( Handle self, Bool disown)
{
	if ( is_opt( optDirtyRegion)) {
		if ( var->rects != NULL ) {
			free(var->rects);
			var->rects = NULL;
		}
		opt_clear( optDirtyRegion);
		var->rects = apc_region_copy_rects(self);
	}
	if ( disown && var->rects ) {
		PRegionRec ret = var->rects;
		var->rects = NULL;
		opt_set( optDirtyRegion);
		return ret;
	}
	return var->rects;
}

#ifdef __cplusplus
}
#endif
