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

static char * ext[] = {
	"heic",
	"heif",
	NULL, /* <-- maybe avif here, see below */
	NULL
};
static int    bpp[] = { imbpp24, imByte, 0 };

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
	"aux",
	NULL
};

#define MAX_FEATURES 15
static char * features[MAX_FEATURES+1];

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
	features,    /* features  */
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
	pset_c( folder,   "toplevel");
	pset_c( subindex, 0);
	return profile;
}

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	{
		struct heif_context*ctx = heif_context_alloc();
		struct heif_encoder_descriptor *enc[1024];
		int i, n, feat;
		n = heif_context_get_encoder_descriptors(ctx, heif_compression_undefined, NULL,
			(const struct heif_encoder_descriptor**) enc, 1024);
		for ( i = feat = 0; i < n; i++) {
			char buf[2048];
			const char *name, *shrt, *compstr;
			enum heif_compression_format comp;
			int lossy, lossless;

			if ( feat >= MAX_FEATURES) {
				features[MAX_FEATURES] = NULL;
				break;
			}

			comp     = heif_encoder_descriptor_get_compression_format(enc[i]);
			switch ( comp ) {
			case heif_compression_HEVC:
				compstr = "HEVC";
				break;
			case heif_compression_AVC:
				compstr = "AVC";
				ext[2] = "avif";
				break;
			case heif_compression_AV1:
				compstr = "AV1";
				ext[2] = "avif";
				break;
			default:
				continue;
			}

			name     = heif_encoder_descriptor_get_name(enc[i]);
			shrt     = heif_encoder_descriptor_get_id_name(enc[i]);
			lossy    = heif_encoder_descriptor_supports_lossy_compression(enc[i]);
			lossless = heif_encoder_descriptor_supports_lossless_compression(enc[i]);

			snprintf(buf, 2048, "%s/%s%s%s (%s)",
				compstr,
				shrt,
				lossy    ? " lossy"    : "",
				lossless ? " lossless" : "",
				name
			);
			buf[2047] = 0;
			features[feat++] = duplicate_string(buf);
		}
		heif_context_free(ctx);
	}
	return (void*)1;
}

static void
done( PImgCodec codec)
{
	int feat;
	for ( feat = 0; feat < MAX_FEATURES; feat++) {
		if ( features[feat] == NULL ) break;
		free(features[feat]);
	}
}

#define FOLDER_TOPLEVEL  0
#define FOLDER_THUMBNAIL 1

typedef struct _LoadRec {
	struct heif_context* ctx;
	struct heif_error    error;
	heif_item_id*        image_ids;
	int                  primary_index, folder, subindex;
	struct heif_image_handle* subhandle;
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
	.reader_api_version = 1,
	.get_position       = heif_get_position,
	.read               = heif_read,
	.seek               = heif_seek,
	.wait_for_file_size = heif_wait_for_file_size
};

#define SET_ERROR(e) if (1) { strlcpy(fi->errbuf,e,256); goto FAIL; }
#define SET_HEIF_ERROR SET_ERROR(l->error.message)
#define CHECK_HEIF_ERROR if (l->error.code != heif_error_Ok) SET_HEIF_ERROR
#define CALL l->error=

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	dPROFILE;
	HV * profile = fi->extras;
	LoadRec * l;
	int n, n_images;
#define PEEK_SIZE 32
	uint8_t data[PEEK_SIZE];
	enum heif_filetype_result ftype;
	heif_item_id primary_id;

	l = malloc( sizeof( LoadRec));
	if ( !l) return NULL;
	memset( l, 0, sizeof( LoadRec));

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
		fi-> stop = true;
		SET_ERROR("unsupported HEIF/AVIC");
	default:
		return NULL;
	}
#undef PEEK_SIZE

	l-> folder   = FOLDER_TOPLEVEL;
	if (pexist(folder)) {
		char * c = pget_c(folder);
		if ( strcmp(c, "toplevel") == 0)
			l-> folder = FOLDER_TOPLEVEL;
		else if ( strcmp(c, "thumbnails") == 0)
			l-> folder = FOLDER_THUMBNAIL;
		else
			SET_ERROR("unknown folder, must be one of: toplevel, thumbnails");
	}
	if (pexist(subindex)) {
		l-> subindex = pget_i(subindex);
		if ( l-> subindex < 0 ) SET_ERROR("bad subindex");
	}

	if ( !( l-> ctx = heif_context_alloc()))
		SET_ERROR("cannot create context");

	CALL heif_context_read_from_reader(l->ctx, &reader, fi->req, NULL);
	CHECK_HEIF_ERROR;

	n_images = heif_context_get_number_of_top_level_images(l->ctx);
	if ( n_images < 1 ) SET_ERROR("cannot get top level images");

	CALL heif_context_get_primary_image_ID(l->ctx, &primary_id);
	CHECK_HEIF_ERROR;

	if ( !( l-> image_ids = malloc( sizeof(heif_item_id) * n_images))) 
		SET_ERROR("not enough memory");

	n = heif_context_get_list_of_top_level_image_IDs(l->ctx, l->image_ids, n_images);
	if ( n < n_images ) {
		n_images = n;
		if ( n < 0 ) SET_ERROR("cannot get list of top level images");
	}
	l-> primary_index = -1;
	for ( n = 0; n < n_images; n++)
		if ( primary_id == l->image_ids[n] ) {
			l-> primary_index = n;
			break;
		}

	if ( l-> folder != FOLDER_TOPLEVEL ) {
		int subindex;
		if ( l-> subindex >= n_images ) SET_ERROR("subindex out of frame range");
		subindex = l->image_ids[l->subindex];
		free(l->image_ids);
		l->image_ids = NULL;
		CALL heif_context_get_image_handle(l-> ctx, subindex, &l->subhandle);
		CHECK_HEIF_ERROR;

		switch ( l-> folder ) {
		case FOLDER_THUMBNAIL:
			n_images = heif_image_handle_get_number_of_thumbnails(l->subhandle);
			break;
		default:
			SET_ERROR("panic: internal error");
		}
		if ( !( l-> image_ids = malloc( sizeof(heif_item_id) * n_images))) 
			SET_ERROR("not enough memory");
		switch ( l-> folder ) {
		case FOLDER_THUMBNAIL:
			n = heif_image_handle_get_list_of_thumbnail_IDs(l->subhandle, l->image_ids, n_images);
			break;
		default:
			SET_ERROR("panic: internal error");
		}
		if ( n < n_images ) {
			n_images = n;
			if ( n < 0 ) {
				snprintf(fi->errbuf, 256, "cannot get list of %s images", pget_c(folder));
				goto FAIL;
			}
		}
	}

	fi->frameCount = n_images;

	return l;

FAIL:
	if ( l->subhandle  ) heif_image_handle_release(l->subhandle);
	if ( l-> image_ids ) free(l-> image_ids);
	if ( l->ctx) heif_context_free(l-> ctx);
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

	switch (l->folder) {
	case FOLDER_TOPLEVEL:
		CALL heif_context_get_image_handle(l-> ctx, l-> image_ids[fi->frame], &h);
		break;
	case FOLDER_THUMBNAIL:
		CALL heif_image_handle_get_thumbnail(l->subhandle, l-> image_ids[fi->frame], &h);
		break;
	}
	CHECK_HEIF_ERROR;

	/* read basics */
	bit_depth = heif_image_handle_get_luma_bits_per_pixel(h);
	if ( bit_depth < 0 ) SET_ERROR("undefined bit depth");
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
		n = heif_image_handle_get_number_of_auxiliary_images(h,
			LIBHEIF_AUX_IMAGE_FILTER_OMIT_ALPHA|
			LIBHEIF_AUX_IMAGE_FILTER_OMIT_DEPTH
		);
		if (n) pset_i( aux,      n);

	}
	icon = kind_of( fi-> object, CIcon);
	if ( icon && !heif_image_handle_has_alpha_channel(h))
		icon = false;

	/* check load options */
	if ( !( hdo = heif_decoding_options_alloc()))
		SET_ERROR("not enough memory");
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
	CALL heif_decode_image(h, &himg,
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
			bc_rgba_bgr_a( src, dst, alpha, width);
			alpha += alpha_stride;
		} else
			cm_reverse_palette(( PRGBColor) src, (PRGBColor) dst, width);
	}

SUCCESS:
	ret = true;
FAIL:
	if (himg) heif_image_release(himg);
	if (hdo) heif_decoding_options_free(hdo);
	if (h) heif_image_handle_release(h);
	return ret;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	if ( l->subhandle ) heif_image_handle_release(l->subhandle);
	if ( l-> ctx)       heif_context_free( l-> ctx);
	if ( l-> image_ids) free( l-> image_ids );
	free( l);
}

typedef struct _SaveRec {
	struct heif_context* ctx;
	struct heif_error    error;
} SaveRec;

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_c(encoder, "HEVC");
	pset_c(quality, "75");
	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec *l;
	if (!(l = malloc(sizeof(SaveRec)))) return NULL;
	memset( l, 0, sizeof( SaveRec));

	if ( !( l-> ctx = heif_context_alloc()))
		SET_ERROR("cannot create context");

	return (void*)l;

FAIL:
	if ( l->ctx) heif_context_free(l-> ctx);
	free(l);
	return NULL;
}

static struct heif_error
heif_write(struct heif_context* ctx, const void* data, size_t size, void* userdata)
{
	struct heif_error err;
	if ( req_write(( PImgIORequest) (userdata), size, (void*)data) >= 0) {
		err.code    = heif_error_Ok;
		err.subcode = heif_suberror_Unspecified;
		err.message = "Ok";
	} else {
		err.code    = heif_error_Encoding_error;
		err.subcode = heif_suberror_Cannot_write_output_data;
		err.message = "write error";
	}
	return err;
}

struct heif_writer writer = {
	.writer_api_version = 1,
	.write              = heif_write
};

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	SaveRec * l = ( SaveRec *) fi-> instance;
	struct heif_encoder* encoder = NULL;
	HV * profile = fi-> objectExtras;
	enum heif_compression_format compression;
	struct heif_image* himg = NULL;
	PIcon i = ( PIcon) fi-> object;
	Bool icon;
	int y, dst_stride, alpha_stride = 0;
	Byte *src, *dst, *alpha = NULL;
	struct heif_encoding_options* options = NULL;
	enum heif_colorspace colorspace;
	enum heif_chroma chroma;

	compression = heif_compression_HEVC;
	if ( pexist(encoder)) {
		char * c = pget_c(encoder);
		if ( strcmp(c, "HEVC") == 0)
			compression = heif_compression_HEVC;
		else if ( strcmp(c, "AVC") == 0)
			compression = heif_compression_AVC;
		else if ( strcmp(c, "AV1") == 0)
			compression = heif_compression_AV1;
		else
			SET_ERROR("bad encoder, must be one of: HEVC, AVC, AV1");
	}
	if ( !heif_have_decoder_for_format(compression))
		SET_ERROR("encoder is not available");
	CALL heif_context_get_encoder_for_format(l->ctx, compression, &encoder);
	CHECK_HEIF_ERROR;

	if ( pexist(quality)) {
		char * c = pget_c(quality);
		if ( strcmp(c, "lossless") == 0) {
			CALL heif_encoder_set_lossless(encoder, 1);
		} else {
			char * err = NULL;
			int quality = strtol(c, &err, 10);
			if ( *err || quality < 0 || quality > 100 )
				SET_ERROR("quality must be set either to an integer between 0 and 100 or to 'lossless'");
			CALL heif_encoder_set_lossy_quality(encoder, quality);
		}
	} else
		CALL heif_encoder_set_lossy_quality(encoder, 75);
	CHECK_HEIF_ERROR;

	options = heif_encoding_options_alloc();


	icon = kind_of( fi-> object, CIcon);
	if ( icon ) {
		if ( i->type == imByte ) {
			colorspace = heif_colorspace_monochrome;
			chroma     = heif_chroma_monochrome;
		} else {
			colorspace = heif_colorspace_RGB;
			chroma     = heif_chroma_interleaved_RGBA;
		}
	} else {
		if ( i->type == imByte ) {
			colorspace = heif_colorspace_monochrome;
			chroma     = heif_chroma_monochrome;
		} else {
			colorspace = heif_colorspace_RGB;
			chroma     = heif_chroma_interleaved_RGB;
		}
	}
	CALL heif_image_create(i-> w, i-> h, colorspace, chroma, &himg);
	CHECK_HEIF_ERROR;

	if ( i->type == imByte ) {
		CALL heif_image_add_plane(himg, heif_channel_Y, i->w, i->h, 8);
		CHECK_HEIF_ERROR;
		dst = (Byte*) heif_image_get_plane(himg, heif_channel_Y, &dst_stride);
	} else {
		CALL heif_image_add_plane(himg, heif_channel_interleaved, i->w, i->h, 8);
		CHECK_HEIF_ERROR;
		dst = (Byte*) heif_image_get_plane(himg, heif_channel_interleaved, &dst_stride);
	}

	if ( icon ) {
		alpha        = i-> mask;
		alpha_stride = i-> maskLine;
	}
	for (
		y = 0, dst += (i->h - 1) * dst_stride, src = i->data;
		y < i->h;
		y++, src += i->lineSize, dst -= dst_stride
	) {
		if ( icon ) {
			if ( i->type == imByte ) {
				memcpy( dst, src, i-> w);
			} else {
				bc_bgr_a_rgba( src, alpha, dst, i->w);
				alpha += alpha_stride;
			}
		} else {
			if ( i->type == imByte ) {
				memcpy( dst, src, i-> w);
			} else {
				cm_reverse_palette(( PRGBColor) src, (PRGBColor) dst, i->w);
			}
		}
	}

	struct heif_image_handle* handle;
	CALL heif_context_encode_image(l->ctx, himg, encoder, options, &handle);
	CHECK_HEIF_ERROR;

	heif_encoding_options_free(options);
	heif_image_handle_release(handle);
	heif_encoder_release(encoder);
	encoder = NULL;
	options = NULL;

	if ( fi-> frame == fi-> frameMapSize - 1 ) {
		CALL heif_context_write(l->ctx, &writer, fi->req);
		CHECK_HEIF_ERROR;
	}
	return true;

FAIL:
	if ( options ) heif_encoding_options_free(options);
	if ( encoder ) heif_encoder_release(encoder);
	return false;

}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * l = ( SaveRec *) fi-> instance;
	if ( l-> ctx) heif_context_free( l-> ctx);
	free(l);
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
	vmt. done          = done;
	apc_img_register( &vmt, NULL);
}

#ifdef __cplusplus
}
#endif
