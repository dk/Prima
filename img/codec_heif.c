#include "img.h"
#include "img_conv.h"
#include <stdio.h>
#include <ctype.h>
#include <libheif/heif.h>
#include <libheif/heif_version.h>

#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

static char * ext[] = { "heic", "heif", NULL };
static int    bpp[] = { imbpp24, 0 };

static char * loadOutput[] = {
	"chroma_bits_per_pixel",
	"depth_images",
	"has_alpha",
	"ispe_height",
	"ispe_width",
	"luma_bits_per_pixel",
	"premultiplied_alpha",
	"primary_frame",
	"thumbnails",
	NULL
};

static char * mime[] = {
	"image/heic",
	"image/heic-sequence",
	"image/heif",
	"image/heif-sequence",
	NULL
};

static ImgCodecInfo codec_info = {
	"libheif",
	"https://github.com/strukturag/libheif",
	LIBHEIF_NUMERIC_VERSION >> 24, (LIBHEIF_NUMERIC_VERSION >> 16) & 0xff,    /* version */
	ext,    /* extension */
	"High Efficiency Image File Format",     /* file type */
	"HEIF", /* short type */
	NULL,    /* features  */
	"",     /* module */
	"",     /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_LOAD_MULTIFRAME |
	IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM | IMG_SAVE_MULTIFRAME,
	bpp, /* save types */
	loadOutput,
	mime
};

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( ignore_transformations, 0);
	pset_i( convert_hdr_to_8bit, 0);
	return profile;
}

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	return (void*)1;
}

typedef struct _LoadRec {
	struct heif_context* ctx;
	struct heif_error    error;
	heif_item_id*        image_ids;
	int                  primary_index;
} LoadRec;

static int64_t
heif_get_position(void* userdata)
{
	return req_tell(( PImgIORequest) (userdata));
}

static int
heif_read(void* data, size_t size, void* userdata)
{
	return req_read(( PImgIORequest) (userdata), size, data) < 0;
}

static int
heif_seek(int64_t position, void* userdata)
{
	return req_seek(( PImgIORequest) (userdata), (long) position, SEEK_SET) < 0;
}

static enum heif_reader_grow_status
heif_wait_for_file_size(int64_t target_size, void* userdata)
{
	PImgIORequest req = (PImgIORequest) userdata;
	long orig, curr;

	orig = req_tell(req);
	req_seek(req, 0, SEEK_END);
	curr = req_tell(req);
	req_seek(req, orig, SEEK_SET);
	return (target_size > curr) ?
		heif_reader_grow_status_size_beyond_eof :
		heif_reader_grow_status_size_reached;
}

struct heif_reader reader = {
	.reader_api_version = LIBHEIF_NUMERIC_VERSION,
	.get_position       = heif_get_position,
	.read               = heif_read,
	.seek               = heif_seek,
	.wait_for_file_size = heif_wait_for_file_size
};

#define SET_ERROR(e) strlcpy(fi->errbuf,e,256)
#define SET_HEIF_ERROR SET_ERROR(l->error.message)

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = malloc( sizeof( LoadRec));
	int n, n_images;
#define PEEK_SIZE 32
	uint8_t data[PEEK_SIZE];
	enum heif_filetype_result ftype;
	heif_item_id primary_id;

	if ( !l) return NULL;

	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return NULL;
	if ( req_read( fi-> req, PEEK_SIZE, data) < PEEK_SIZE) return NULL;
	if ( req_seek( fi-> req, 0, SEEK_SET) < 0) return NULL;
	ftype = heif_check_filetype( data, PEEK_SIZE );
	switch ( ftype ) {
	case heif_filetype_yes_supported:
	case heif_filetype_maybe:
		fi-> stop = true;
		break;
	case heif_filetype_yes_unsupported:
		SET_ERROR("unsupported HEIF/AVIC");
		fi-> stop = true;
		return NULL;
	default:
		return NULL;
	}
#undef PEEK_SIZE

	memset( l, 0, sizeof( LoadRec));
	if ( !( l-> ctx = heif_context_alloc())) {
		SET_ERROR("cannot create context");
		free(l);
		return NULL;
	}

	l->error = heif_context_read_from_reader(l->ctx, &reader, fi->req, NULL);
	if (l->error.code != heif_error_Ok) {
		SET_HEIF_ERROR;
		goto FAIL;
	}

	n_images = heif_context_get_number_of_top_level_images(l->ctx);
	if ( n_images < 1 ) {
		SET_ERROR("cannot get top level images");
		goto FAIL;
	}

	l-> error = heif_context_get_primary_image_ID(l->ctx, &primary_id);
	if (l->error.code != heif_error_Ok) {
		SET_HEIF_ERROR;
		goto FAIL;
	}

	if ( !( l-> image_ids = malloc( sizeof(heif_item_id) * n_images))) {
		SET_ERROR("not enough memory");
		goto FAIL;
	}
	n = heif_context_get_list_of_top_level_image_IDs(l->ctx, l->image_ids, n_images);
	if ( n < n_images ) {
		n_images = n;
		if ( n < 0 ) {
			SET_ERROR("cannot get list of top level images");
			free( l-> image_ids );
			goto FAIL;
		}
	}
	l-> primary_index = -1;
	for ( n = 0; n < n_images; n++)
		if ( primary_id == l->image_ids[n] ) {
			l-> primary_index = n;
			break;
		}

	fi->frameCount = n_images;

	return l;

FAIL:
	heif_context_free(l-> ctx);
	free(l);
	return NULL;
}

static void
read_metadata(const struct heif_image_handle* h, HV * profile)
{
	int i, n;
	heif_item_id* ids;
	AV * av;

	n = heif_image_handle_get_number_of_metadata_blocks(h, NULL);
	if ( n < 1 ) return;

	if ( !( ids = malloc(n * sizeof(heif_item_id))))
		return;

	i = heif_image_handle_get_list_of_metadata_block_IDs(h, NULL, ids, n);
	if ( i > n ) n = i;
	if ( n < 1 ) goto BAILOUT;

	av = newAV();
	pset_sv_noinc( metadata, newRV_noinc((SV*) av));
	for ( i = 0; i < n; i++) {
		char * c;
		size_t sz;
		SV * sv;
		struct heif_error error;
		HV * profile;

		profile = newHV();
		av_push( av, newRV_noinc((SV*)profile));

		c = (char*) heif_image_handle_get_metadata_type(h, ids[i]);
		pset_c(type, c);
		c = (char*) heif_image_handle_get_metadata_content_type(h, ids[i]);
		pset_c(content_type, c);

		sz = heif_image_handle_get_metadata_size(h, ids[i]);
		if ( sz < 0 ) continue;
		sv = prima_array_new(sz);
		error = heif_image_handle_get_metadata(h, ids[i], prima_array_get_storage(sv));
		if (error.code != heif_error_Ok) {
			sv_free(sv);
			continue;
		}
		pset_sv_noinc(content, sv);
	}

BAILOUT:
	free(ids);
}

#define CHECK_HEIF_ERROR \
	if (l->error.code != heif_error_Ok) { \
		SET_HEIF_ERROR; \
		goto BAILOUT; \
	}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	HV * profile = fi-> frameProperties;
	PImage i     = ( PImage) fi-> object;
	LoadRec * l  = ( LoadRec *) fi-> instance;

	struct heif_image_handle* h = NULL;
	struct heif_decoding_options* hdo = NULL;
	struct heif_image* himg = NULL;

	Bool ret = false, icon;
	int y, bit_depth, width, height, src_stride, alpha_stride = 0;
	Byte *src, *dst, *alpha = NULL;

	l-> error = heif_context_get_image_handle(l-> ctx, l-> image_ids[fi->frame], &h);
	CHECK_HEIF_ERROR;

	/* read basics */
	bit_depth = heif_image_handle_get_luma_bits_per_pixel(h);
	if ( bit_depth < 0 ) {
		SET_ERROR("undefined bit depth");
		goto BAILOUT;
	}
	width  = heif_image_handle_get_width(h);
	height = heif_image_handle_get_height(h);
	if ( fi-> loadExtras) {
		int n;
		if ( l-> primary_index >= 0)
			pset_i( primary_index,  l-> primary_index);
		if ( heif_image_handle_is_premultiplied_alpha(h))
			pset_i( premultiplied_alpha, 1);
		pset_i( chroma_bits_per_pixel,  heif_image_handle_get_chroma_bits_per_pixel(h));
		pset_i( luma_bits_per_pixel,    bit_depth);
		pset_i( ispe_width,             heif_image_handle_get_ispe_width(h));
		pset_i( ispe_height,            heif_image_handle_get_ispe_height(h));
		if (heif_image_handle_has_alpha_channel(h))
			pset_i( has_alpha,      1);
		n = heif_image_handle_get_number_of_depth_images(h);
		if (n) pset_i( depth_images,    n);
		n = heif_image_handle_get_number_of_thumbnails(h);
		if (n) pset_i( thumbnails,      n);
	}
	icon = kind_of( fi-> object, CIcon);
	if ( icon && !heif_image_handle_has_alpha_channel(h))
		icon = false;

	/* check load options */
	if ( !( hdo = heif_decoding_options_alloc())) {
		SET_ERROR("not enough memory");
		goto BAILOUT;
	}
	{
		dPROFILE;
		HV * profile = fi->profile;
		if ( pexist(ignore_transformations))
			hdo->ignore_transformations = pget_i(ignore_transformations);
		if ( pexist(convert_hdr_to_8bit))
			hdo->convert_hdr_to_8bit = pget_i(convert_hdr_to_8bit);
	}
	if ( fi-> loadExtras)
		read_metadata(h, profile);

	if ( fi-> noImageData) {
		pset_i( width,  width);
		pset_i( height, height);
		goto SUCCESS;
	}
	i-> self-> create_empty(( Handle) i, width, height, imbpp24);
	if ( icon ) {
		PIcon(i)->self-> set_autoMasking((Handle) i, false);
		PIcon(i)->self-> set_maskType((Handle) i, imbpp8);
	}

	/* load */
	l->error = heif_decode_image(h, &himg,
		heif_colorspace_RGB,
		icon ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
		hdo);
	CHECK_HEIF_ERROR;

	src = (Byte*) heif_image_get_plane_readonly(himg, heif_channel_interleaved, &src_stride);
	if ( icon ) {
		alpha        = PIcon(i)-> mask;
		alpha_stride = PIcon(i)-> maskLine;
	}
	for (
		y = 0, src += (height - 1) * src_stride, dst = i->data;
		y < height;
		y++, src -= src_stride, dst += i->lineSize
	) {
		if ( icon ) {
			register Byte *s = src;
			register Byte *d = dst;
			register Byte *a = alpha;
			register int   x = width;
			x = width;
			while (x--) {
				register Byte r,g;
				r = *(s++);
				g = *(s++);
				*(d++) = *(s++);
				*(d++) = g;
				*(d++) = r;
				*(a++) = *(s++);
			}
			alpha += alpha_stride;
		} else
			cm_reverse_palette(( PRGBColor) src, (PRGBColor) dst, width);
	}

SUCCESS:
	ret = true;
BAILOUT:
	if (himg) heif_image_release(himg);
	if (hdo) heif_decoding_options_free(hdo);
	if (h) heif_image_handle_release(h);
	return ret;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	if ( l-> ctx)       heif_context_free( l-> ctx);
	if ( l-> image_ids) free( l-> image_ids );
	free( l);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return (void*)1;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	return false;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
}

void
apc_img_codec_heif( void )
{
	struct ImgCodecVMT vmt;
	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
	vmt. load_defaults = load_defaults;
	vmt. init          = init;
	vmt. open_load     = open_load;
	vmt. load          = load;
	vmt. close_load    = close_load;
	vmt. save_defaults = save_defaults;
	vmt. open_save     = open_save;
	vmt. save          = save;
	vmt. close_save    = close_save;
	apc_img_register( &vmt, NULL);
}

#ifdef __cplusplus
}
#endif
