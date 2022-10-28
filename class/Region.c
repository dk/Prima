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

static Box*
rgn_rect( HV * profile, Bool is_box, int * n_boxes )
{
	char *t;
	SV ** box_entry;
	Box *boxes;

	t  = is_box ? "box" : "rect";
	box_entry = hv_fetch( profile, t, (I32) strlen(t), 0);
	if (( boxes = (Box*) prima_read_array(
		*box_entry, "Region::new", 'i',
		4, 1, -1,
		n_boxes, NULL
	)) == NULL) {
		*n_boxes = 0;
		return NULL;
	}

	if ( !is_box ) {
		int i;
		Box * box = boxes;
		for ( i = 0; i < *n_boxes; i++, box++) {
			box-> width  -= box-> x;
			box-> height -= box-> y;
		}
	}

	return boxes;
}

static PRegionRec
rgn_polygon( HV * profile )
{
	dPROFILE;
	Bool do_free;
	int count, fill_mode;
	PRegionRec rgn;
	Point *points;

	if (( points = (Point*) prima_read_array(
		pget_sv(polygon), "Region::polygon", 'i',
		2, 2, -1,
		&count, &do_free)
	) == NULL)
		return NULL;

	fill_mode = pexist(fillMode) ? pget_i(fillMode) : (fmOverlay | fmWinding);

	rgn = img_region_polygon( points, count, fill_mode );
	if ( do_free ) free( points );

	return rgn;
}

static PRegionRec
rgn_image( HV * profile )
{
	dPROFILE;
	Handle mask;
	PRegionRec rgn;
	Bool free_image = false;

	mask = pget_H(image);
	if ( !kind_of( mask, CImage )) {
		warn("Not an image passed");
		return NULL;
	}
	if (( PImage(mask)->type & imBPP ) != 1 ) {
		mask = CImage(mask)->dup(mask);
		CImage(mask)->set_conversion(mask, ictNone);
		CImage(mask)->set_type(mask, imbpp1 | imGrayScale);
		free_image = true;
	}

	rgn = img_region_mask( mask );
	if ( free_image ) Object_destroy(mask);

	return rgn;
}

void
Region_init( Handle self, HV * profile)
{
	Bool ok;
	RegionRec r, *pr = &r;

	inherited-> init( self, profile);

	if ( pexist(rect)) {
		r.boxes = rgn_rect(profile, 0, &r.n_boxes);
	} else if (pexist(box)) {
		r.boxes = rgn_rect(profile, 1, &r.n_boxes);
	} else if (pexist(polygon)) {
		pr = rgn_polygon(profile);
	} else if (pexist(image)) {
		pr = rgn_image(profile);
	} else {
		r.n_boxes = 0;
		r.boxes   = NULL;
	}

	ok = apc_region_create(self, pr);
	if ( pr != &r && pr != NULL )
		free(pr);

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
	PRegionRec copy;

	if ( !( copy = img_region_new( data->n_boxes )))
		return NULL;
	copy-> n_boxes = data-> n_boxes;
	memcpy( copy-> boxes, data-> boxes, data-> n_boxes * sizeof(Box) );

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
	if (( ret = prima_array_new(data-> n_boxes * sizeof(Box))) == NULL)
		return NULL_SV;
	memcpy( prima_array_get_storage(ret), data->boxes, data->n_boxes * sizeof(Box));
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
