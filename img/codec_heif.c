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
	"is_primary",
	"ispe_height",
	"ispe_width",
	"luma_bits_per_pixel",
	"premultiplied_alpha",
	"thumbnails",
	"aux",
	"metadata",
	"thumbnail_of",
	NULL
};

#define MAX_ENCODERS 32
#define MAX_FEATURES (MAX_ENCODERS*2)
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
	"Prima::Image::heif",     /* module */
	"Prima::Image::heif",     /* package */
	0,
	bpp, /* save types */
	loadOutput,
	mime
};

static char default_plugin[256] = "";

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( ignore_transformations, 0);
	pset_i( convert_hdr_to_8bit, 0);
	return profile;
}

#if LIBHEIF_NUMERIC_VERSION >= ((1<<24) | (15<<16) | (0<<8) | 0)
#define HAS_V1_15_API
#endif

static int
get_encoder_descriptors(
	struct heif_context*_ctx,
	enum heif_compression_format format_filter,
	const char* name_filter,
	const struct heif_encoder_descriptor** out_encoders,
	int count
) {
	int n;
	struct heif_context*ctx = _ctx ? _ctx : heif_context_alloc();
#ifdef HAS_V1_15_API
	n = heif_get_encoder_descriptors(format_filter, name_filter, out_encoders, count);
#else
	n = heif_context_get_encoder_descriptors(ctx, format_filter, name_filter, out_encoders, count);
#endif
	if (!_ctx) heif_context_free(ctx);
	return n;
}

static char*
describe_compression(int v, char *shrt)
{
	static char buf[4], *compstr;

	if (
		(strstr(shrt, "jpeg") != NULL) ||
		(strstr(shrt, "png") != NULL) ||
		(strcmp(shrt, "mask") == 0)
	)
		return NULL;

	switch (v) {
	case heif_compression_undefined:
		compstr = "any";
		break;
	case heif_compression_HEVC:
		compstr = "HEVC";
		break;
	case heif_compression_AVC:
		compstr = "AVC";
		break;
	case heif_compression_AV1:
		compstr = "AV1";
		break;
	default:
		if ( strcmp( shrt, "dav1d" ) == 0 )
			compstr = "AV1";
		else if (
			(strcmp( shrt, "ffmpeg" ) == 0) ||
			(strcmp( shrt, "libde265" ) == 0) 
		)
			compstr = "HEVC";
		else
			snprintf(compstr = buf, sizeof(buf), "%d", v);
	}

	return compstr;
}

static void *
init( PImgCodecInfo * info, void * param)
{
	int feat = 0;
	char hevc_plugin[256] = "";
	PHash encoders = prima_hash_create();
	*info = &codec_info;
	{
		struct heif_encoder_descriptor *enc[1024];
		int i, n;

		n = get_encoder_descriptors(NULL, heif_compression_undefined, NULL,
			(const struct heif_encoder_descriptor**) enc, 1024);
		for ( i = 0; i < n; i++) {
			char buf[2048], *compstr;
			const char *name, *shrt;
			enum heif_compression_format comp;
			int lossy, lossless;

			if ( feat >= MAX_FEATURES) {
				features[MAX_FEATURES] = NULL;
				break;
			}

			comp     = heif_encoder_descriptor_get_compression_format(enc[i]);
			name     = heif_encoder_descriptor_get_name(enc[i]);
			shrt     = heif_encoder_descriptor_get_id_name(enc[i]);
			lossy    = heif_encoder_descriptor_supports_lossy_compression(enc[i]);
			lossless = heif_encoder_descriptor_supports_lossless_compression(enc[i]);

			if ( !( compstr = describe_compression(comp, (char*) shrt)))
				continue;

			switch ( comp ) {
			case heif_compression_AVC:
				ext[2] = "avif";
				break;
			case heif_compression_AV1:
				ext[2] = "avif";
				break;
			default:
			}

			if ( strcmp(compstr, "HEVC") == 0)
				strlcpy( hevc_plugin, shrt, sizeof(hevc_plugin));

			snprintf(buf, 2048, "encoder %s %s%s%s (%s)",
				shrt,
				compstr,
				lossy    ? " lossy"    : "",
				lossless ? " lossless" : "",
				name
			);
			buf[2047] = 0;
			features[feat++] = duplicate_string(buf);

			{
				intptr_t v = (intptr_t) comp;
				prima_hash_store(encoders, shrt, strlen(shrt), (void*)v);
			}
		}
		if ( n > 0 )
			codec_info.IOFlags |= IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_LOAD_MULTIFRAME;
	}

#if defined(HAS_V1_15_API)
	{
		struct heif_decoder_descriptor *dec[1024];
		int i, n;

		n = heif_get_decoder_descriptors(heif_compression_undefined,
			(const struct heif_decoder_descriptor**) dec, 1024);
		for ( i = 0; i < n; i++) {
			char buf[2048];
			const char *name, *shrt, *compstr;
			intptr_t v;

			if ( feat >= MAX_FEATURES) {
				features[MAX_FEATURES] = NULL;
				break;
			}

			name     = heif_decoder_descriptor_get_name(dec[i]);
			shrt     = heif_decoder_descriptor_get_id_name(dec[i]);

			v = (intptr_t) prima_hash_fetch(encoders, shrt, strlen(shrt));
			if ( !( compstr = describe_compression(v, (char*) shrt)))
				continue;

			snprintf(buf, 2048, "decoder %s %s (%s)", shrt, compstr, name);
			buf[2047] = 0;
			features[feat++] = duplicate_string(buf);

			if (
				v && (
					default_plugin[0] == 0 || (
						hevc_plugin[0] != 0 &&
						( strcmp( hevc_plugin, shrt ) == 0)
					)
				)
			) {
				strlcpy( default_plugin, shrt, sizeof(default_plugin));
			}
		}
		if ( n > 0 )
			codec_info.IOFlags |= IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM | IMG_SAVE_MULTIFRAME;
	}
#else
	{
		int i;
		char * compstr;
		enum heif_compression_format comp[3] = {
			heif_compression_HEVC,
			heif_compression_AVC,
			heif_compression_AV1
		};
		for ( i = 0; i < 3; i++) {
			switch ( comp[i] ) {
			case heif_compression_HEVC:
				compstr = "HEVC";
				break;
			case heif_compression_AVC:
				compstr = "AVC";
				break;
			case heif_compression_AV1:
				compstr = "AV1";
				break;
			default:
				continue;
			}

			/* default plugin is always NULL because we cannot say whether
			something a random encoder creates can be read by a random decoder */
			if ( heif_have_decoder_for_format(comp[i])) {
				char buf[2048];
				snprintf(buf, 2048, "decoder ? %s ()", compstr);
				buf[2047] = 0;
				codec_info.IOFlags |= IMG_SAVE_TO_FILE | IMG_SAVE_TO_STREAM | IMG_SAVE_MULTIFRAME;

				if ( feat >= MAX_FEATURES) {
					features[MAX_FEATURES] = NULL;
					break;
				} else
					features[feat++] = duplicate_string(buf);
			}
		}
	}
#endif

	prima_hash_destroy( encoders, false );

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

typedef struct {
	heif_item_id *items;
	int           count, size, curr;
	heif_item_id _buf[1];
} ItemList;

typedef struct _LoadRec {
	struct heif_context* ctx;
	struct heif_error    error;
	int                  primary_index;
	ItemList             *toplevel, *thumbnails;
	int                  *toplevel_index;
	struct heif_image_handle *curr_toplevel_handle;
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

#define SET_ERROR(e) if (1) { \
	snprintf(fi->errbuf, 256, "%s (at %s line %d)", e, __FILE__, __LINE__);\
	goto FAIL; }
#define SET_HEIF_ERROR SET_ERROR(l->error.message)
#define CHECK_HEIF_ERROR if (l->error.code != heif_error_Ok) SET_HEIF_ERROR
#define CALL l->error=

static Bool
item_list_alloc(ItemList** list, int n)
{
	ItemList* p = *list;
	if ( p ) {
		if ( p-> size < n ) {
			int sz = p-> size;
			while (sz < n) sz *= 2;
			if ( !( p = realloc( p, (sz - 1) * sizeof(heif_item_id) + sizeof(ItemList))))
				return false;
			p-> size = sz;
		}
	} else {
		if ( !( p = malloc((n - 1) * sizeof(heif_item_id) + sizeof(ItemList))))
			return false;
		p-> size = n;
		p-> curr = 0;
	}
	p-> count = n;
	p-> items = p-> _buf;
	*list = p;
	return true;
}

/*
this codec throws all found images as frame numbers.
currently only thumbnails, but if i get my hands on heics with depth images and the like, I'll add these here as well.
if f ex we have a 2-frame heic where each frame has two thumbnails, Prima treats such file as this:

frame 0 -> toplevel #0
frame 1 -> toplevel #0's thumbnail #0
frame 2 -> toplevel #0's thumbnail #1
frame 3 -> toplevel #1
frame 4 -> toplevel #1's thumbnail #0
frame 5 -> toplevel #1's thumbnail #1

thumbnail extras have .thumbnail_of field set

*/
static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
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

	if ( !( l-> ctx = heif_context_alloc()))
		SET_ERROR("cannot create context");

#if LIBHEIF_NUMERIC_VERSION > 0x10e0100
	/* https://github.com/strukturag/libheif/pull/737 - multi-threading crashes sometimes */
	heif_context_set_max_decoding_threads(l->ctx, 0);
#endif

	CALL heif_context_read_from_reader(l->ctx, &reader, fi->req, NULL);
	CHECK_HEIF_ERROR;

	n_images = heif_context_get_number_of_top_level_images(l->ctx);
	if ( n_images < 1 ) SET_ERROR("cannot get top level images");

	CALL heif_context_get_primary_image_ID(l->ctx, &primary_id);
	CHECK_HEIF_ERROR;

	if ( !item_list_alloc(&l->toplevel, n_images))
		SET_ERROR("not enough memory");
	l->toplevel->curr = -1;

	if ( !( l->toplevel_index = malloc((n_images + 1) * sizeof(int))))
		SET_ERROR("not enough memory");
	l->toplevel_index[0] = 0;
	for ( n = 1; n <= n_images; n++) l->toplevel_index[n] = -1;

	if ( !item_list_alloc(&l->thumbnails, 16))
		SET_ERROR("not enough memory");
	l->thumbnails->count = 0;

	n = heif_context_get_list_of_top_level_image_IDs(l->ctx, l->toplevel->items, n_images);
	if ( n < n_images ) {
		n_images = n;
		if ( n < 0 ) SET_ERROR("cannot get list of top level images");
	}
	l-> primary_index = -1;
	for ( n = 0; n < n_images; n++)
		if ( primary_id == l->toplevel->items[n] ) {
			l-> primary_index = n;
			break;
		}

	if ( fi-> wantFrames ) {
		struct heif_image_handle *h;
		int index = 0;
		fi-> frameCount = n_images;
		for ( n = 0; n < n_images; n++) {
			int nn;
			CALL heif_context_get_image_handle(l-> ctx, l->toplevel->items[n], &h);
			CHECK_HEIF_ERROR;
			nn = heif_image_handle_get_number_of_thumbnails(h);
			l->toplevel_index[n] = index;
			index += 1 + nn;
			fi-> frameCount += nn;
			heif_image_handle_release(h);
		}
		l->toplevel_index[n_images] = index;
	}

	return l;

FAIL:
	if ( l-> toplevel_index) free(l-> toplevel_index);
	if ( l-> toplevel   ) free(l-> toplevel);
	if ( l-> thumbnails ) free(l-> thumbnails);
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
set_toplevel_handle( PImgLoadFileInstance fi, int toplevel)
{
	int n;
	struct heif_image_handle* h = NULL;
	LoadRec * l  = ( LoadRec *) fi-> instance;

	if ( toplevel == l->toplevel->curr && l->curr_toplevel_handle)
		return true;

	if ( l->curr_toplevel_handle ) {
		heif_image_handle_release(l->curr_toplevel_handle);
		l->curr_toplevel_handle = NULL;
	}
	CALL heif_context_get_image_handle(l-> ctx, l->toplevel->items[toplevel], &h);
	CHECK_HEIF_ERROR;
	l->toplevel->curr = toplevel;
	l->curr_toplevel_handle = h;

	n = heif_image_handle_get_number_of_thumbnails(h);
	if ( !item_list_alloc(&l->thumbnails, n))
		SET_ERROR("not enough memory");
	bzero(l->thumbnails->items, sizeof(heif_item_id) * n);
	heif_image_handle_get_list_of_thumbnail_IDs(h, l->thumbnails->items, l->thumbnails->count);

	return true;

FAIL:
	return false;
}

static struct heif_image_handle*
load_thumbnail( PImgLoadFileInstance fi, int toplevel)
{
	LoadRec * l  = ( LoadRec *) fi-> instance;
	struct heif_image_handle* h = NULL;

	if ( !set_toplevel_handle(fi, toplevel)) return false;

	CALL heif_image_handle_get_thumbnail(l->curr_toplevel_handle,
		l-> thumbnails->items[fi->frame - l->toplevel_index[toplevel] - 1],
		&h);
	CHECK_HEIF_ERROR;
FAIL:
	return h;
}

static struct heif_image_handle*
seek_image_handle( PImgLoadFileInstance fi)
{
	int i;
	struct heif_image_handle* h = NULL;

	if ( fi->frame < 0 ) return NULL;
	LoadRec * l  = ( LoadRec *) fi-> instance;

	for ( i = 0; i < l-> toplevel-> count; i++) {
		if ( l->toplevel_index[i+1] < 0 ) {
			if ( !set_toplevel_handle(fi, i)) return false;
			l->toplevel_index[i+1] = l->toplevel_index[i] + 1 + l->thumbnails->count;
		}

		if ( fi->frame == l->toplevel_index[i]) {
			if ( !set_toplevel_handle(fi, i)) return false;
			return l->curr_toplevel_handle;
		} else if ( fi->frame < l->toplevel_index[i] ) {
			if ( !( h = load_thumbnail(fi, i - 1))) return false;
			break;
		} else if ( i + 1 == l-> toplevel->count && fi->frame > l->toplevel_index[i] ) {
			if ( !( h = load_thumbnail(fi, i))) return false;
			break;
		}
	}

	if ( fi->frameCount < 0 && l->toplevel_index[l->toplevel->count] >= 0 ) 
		fi->frameCount = l->toplevel_index[l->toplevel->count];

	return h;
}

Bool
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

	if ( !( h = seek_image_handle(fi)))
		goto FAIL;

	/* read basics */
	bit_depth = heif_image_handle_get_luma_bits_per_pixel(h);
	if ( bit_depth < 0 ) SET_ERROR("undefined bit depth");
	width  = heif_image_handle_get_width(h);
	height = heif_image_handle_get_height(h);
	if ( fi-> loadExtras) {
		int n;
		if ( heif_image_handle_is_premultiplied_alpha(h))
			pset_i( premultiplied_alpha, 1);
		pset_i( chroma_bits_per_pixel,  heif_image_handle_get_chroma_bits_per_pixel(h));
		pset_i( luma_bits_per_pixel,    bit_depth);
		pset_i( ispe_width,             heif_image_handle_get_ispe_width(h));
		pset_i( ispe_height,            heif_image_handle_get_ispe_height(h));
		if (heif_image_handle_has_alpha_channel(h))
			pset_i( has_alpha,      1);
		if ( h == l->curr_toplevel_handle ) {
			n = heif_image_handle_get_number_of_depth_images(h);
			if (n) pset_i( depth_images, n);
			n = heif_image_handle_get_number_of_thumbnails(h);
			if (n) pset_i( thumbnails, n);
			n = heif_image_handle_get_number_of_auxiliary_images(h,
				LIBHEIF_AUX_IMAGE_FILTER_OMIT_ALPHA|
				LIBHEIF_AUX_IMAGE_FILTER_OMIT_DEPTH
			);
			if (n) pset_i( aux, n);
			if ( l->primary_index == l->toplevel->curr)
				pset_i(is_primary, 1);
		} else {
			pset_i(thumbnail_of, l->toplevel_index[l->toplevel->curr]);
		}

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
	if ( !ret && fi->frameCount < 0 )
		fi->frameCount = l->toplevel->count; /* failure, not a EOF */
	if (himg) heif_image_release(himg);
	if (hdo) heif_decoding_options_free(hdo);
	if ( h && h != l->curr_toplevel_handle )
		heif_image_handle_release(h);
	return ret;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	if ( l-> curr_toplevel_handle) heif_image_handle_release(l-> curr_toplevel_handle);
	if ( l-> ctx)        heif_context_free( l-> ctx);
	if ( l-> toplevel_index) free(l-> toplevel_index);
	if ( l-> toplevel)   free( l-> toplevel );
	if ( l-> thumbnails) free(l-> thumbnails);
	free( l);
}

typedef struct _SaveRec {
	struct heif_context* ctx;
	struct heif_error    error;
	struct heif_image_handle** handles;
	struct heif_image_handle*  handlebuf[1];
} SaveRec;

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	struct heif_encoder* encoder = NULL;
	struct heif_context* ctx;

	pset_c(encoder, default_plugin);
	pset_i(is_primary, 0);
	pset_c(quality, "50"); /* x265.quality and aom.quality default values are 50 */
	pset_i(premultiplied_alpha, 0);
	pset_i(thumbnail_of,    -1);
	pset_sv(metadata,  newRV_noinc((SV*) newHV()));

	if (( ctx = heif_context_alloc()) != NULL) {
		struct heif_encoder_descriptor *enc[1024];
		int i, n;
		n = get_encoder_descriptors(ctx, heif_compression_undefined, NULL,
			(const struct heif_encoder_descriptor**) enc, 1024);
		for ( i = 0; i < n; i++) {
			const char * shrt = heif_encoder_descriptor_get_id_name(enc[i]);
			if (heif_context_get_encoder(ctx, enc[i], &encoder).code == heif_error_Ok) {
				char buf[2048];
				const struct heif_encoder_parameter*const* list = heif_encoder_list_parameters(encoder);
				while ( list && *list) {
					HV *hv = newHV();
					const char* name = heif_encoder_parameter_get_name(*list);
					const char* svtype = "";
					enum heif_encoder_parameter_type type = heif_encoder_parameter_get_type(*list);
					switch (type) {
					case heif_encoder_parameter_type_integer: {
						HV *profile = hv;
						int v, have_min = 0, min = 0, max = 0;
						int have_max = 0, n = 0, *ints = NULL;
						svtype = "int";
						if ( heif_encoder_get_parameter_integer(encoder, name, &v).code == heif_error_Ok)
							pset_i(default, v);
						if ( heif_encoder_parameter_get_valid_integer_values(*list,
							&have_min, &have_max, &min, &max, &n, (const int**) &ints
							).code == heif_error_Ok) {
							if ( have_min ) pset_i(min, min);
							if ( have_max ) pset_i(max, max);
							if ( ints && n > 0 ) {
								AV * av = newAV();
								for (v = 0; v < n; v++)
									av_push(av, newSViv(ints[v]));
								pset_sv(values, newRV_noinc((SV*)av));
							}
						}
						break;
					}
					case heif_encoder_parameter_type_boolean: {
						int v;
						HV *profile = hv;
						svtype = "bool";
						if ( heif_encoder_get_parameter_boolean(encoder, name, &v).code == heif_error_Ok)
							pset_i(default, v);
						break;
					}
					case heif_encoder_parameter_type_string: {
						char v[2048], **strs;
						HV *profile = hv;
						svtype = "str";
						if ( heif_encoder_get_parameter_string(encoder, name, v, sizeof(v)).code == heif_error_Ok) 
							pset_c(default, v);
						if (
							heif_encoder_parameter_get_valid_string_values(
								*list,
								(const char* const**) &strs
							).code == heif_error_Ok
						) {
							AV * av = newAV();
							while (strs && *strs) {
								av_push(av, newSVpv(*strs, 0));
								strs++;
							}
							pset_sv(values, newRV_noinc((SV*)av));
						}
						break;
					}
					default:
						sv_free((SV*) hv);
						list++;
						continue;
					}
					snprintf(buf, 2048, "%s.%s", shrt, name);
					(void)hv_store( hv, "type", 4, newSVpv(svtype, 0), 0);
					(void)hv_store( profile, buf, (I32) strlen(buf), newRV_noinc((SV*)hv), 0);
					list++;
				}
				heif_encoder_release(encoder);
			}
		}
		heif_context_free(ctx);
	}

	return profile;
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec *l;
	int sz;

	sz = sizeof(SaveRec) + (fi->n_frames * sizeof(struct heif_image_handle*));
	if (!(l = malloc(sz))) return NULL;
	memset( l, 0, sz);
	l-> handles = l->handlebuf;

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
encode_thumbnail( PImgSaveFileInstance fi, HV * profile)
{
	dPROFILE;
	SaveRec * l  = ( SaveRec *) fi-> instance;
	int thumbnail_of = -1;

	if ( !pexist(thumbnail_of))
		return true;

	thumbnail_of = pget_i(thumbnail_of);
	if ( thumbnail_of < 0 || thumbnail_of >= fi->n_frames )
		SET_ERROR("thumbnail_of must be an integer from 0 to the last frame");
	if ( thumbnail_of == fi->frame)
		SET_ERROR("thumbnail_of cannot refer to itself");
	if ( !l->handles[thumbnail_of])
		SET_ERROR("master image is not encoded yet");
	CALL heif_context_assign_thumbnail(l->ctx, l->handles[thumbnail_of], l->handles[fi->frame]);
	CHECK_HEIF_ERROR;
	return true;

FAIL:
	return false;
}

static Bool
encode_metadata( PImgSaveFileInstance fi, HV * profile)
{
	dPROFILE;
	SaveRec * l  = ( SaveRec *) fi-> instance;
	SV *sv, *content;
	AV *av;
	void * data;
	STRLEN size;
	char * item_type, * content_type;
	int i, len;

	if ( !pexist(metadata))
		return true;
	sv = pget_sv(metadata);

	if ( !SvOK(sv) || !SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV)
		SET_ERROR("'metadata' is not an array");
	av = (AV*) SvRV(sv);
	len = av_len(av) + 1;
	for ( i = 0; i < len; i++) {
		SV **ssv;
		ssv = av_fetch( av, i, 0 );
		if ( !ssv || !(sv = *ssv) || !SvOK(sv) || !SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVHV)
			SET_ERROR("metadata[index] is not a hash");

		profile = ( HV*) SvRV( sv);

		/*
		XMP:  mime=application/rdf+xml
		EXIF: (uint32_t)offset + data, Exif=
		*/

		if ( pexist(type))
			item_type = pget_c(type);
		else
			SET_ERROR("metadata.type is missing");

		if ( pexist(content_type))
			content_type = pget_c(content_type);
		else
			content_type = NULL;

		if ( pexist(content))
			content = pget_sv(content);
		else
			SET_ERROR("metadata.content is missing");
		data = SvPV(content, size);

		CALL heif_context_add_generic_metadata(
			l->ctx, l->handles[fi->frame],
			data, (int)size,
			(const char*)item_type, (const char*)content_type
		);
		CHECK_HEIF_ERROR;
	}

	return true;

FAIL:
	return false;
}

static Bool
apply_encoder_options( PImgSaveFileInstance fi, char * plugin, struct heif_encoder** encoder)
{
	SV **tmp;
	HV * profile = fi-> extras;
	SaveRec * l = ( SaveRec *) fi-> instance;
	const struct heif_encoder_parameter*const* list;
	const char * shrt = NULL;
	struct heif_encoder_descriptor* descr;

	if (get_encoder_descriptors(
		l->ctx, heif_compression_undefined, plugin,
		(const struct heif_encoder_descriptor**) &descr, 1
	) <= 0 )
		SET_ERROR("cannot find an encoder");
	if ( !( shrt = heif_encoder_descriptor_get_id_name(descr)))
		SET_ERROR("cannot find encoder descriptor");
	CALL heif_context_get_encoder(l->ctx, descr, encoder);
	CHECK_HEIF_ERROR;

	list = heif_encoder_list_parameters(*encoder);
	while ( list && *list) {
		char buf[128];
		const char* name = heif_encoder_parameter_get_name(*list);
		enum heif_encoder_parameter_type type = heif_encoder_parameter_get_type(*list);
		SV* sv;

		snprintf(buf, 128, "%s.%s", shrt, name);
		if ( !hv_exists( profile, buf, (I32) strlen( buf)))
			goto NEXT;
		if (( tmp = hv_fetch( profile, buf, (I32) strlen( buf), 0)) == NULL)
			goto NEXT;
		sv = *tmp;

		switch (type) {
		case heif_encoder_parameter_type_integer:
			CALL heif_encoder_set_parameter_integer(*encoder, name, SvIV(sv));
			break;
		case heif_encoder_parameter_type_boolean:
			CALL heif_encoder_set_parameter_boolean(*encoder, name, SvIV(sv));
			break;
		case heif_encoder_parameter_type_string:
			CALL heif_encoder_set_parameter_string(*encoder, name, SvPV_nolen(sv));
			break;
		}
		if (l->error.code != heif_error_Ok) {
			snprintf(fi->errbuf, 256, "%s (%s)", l->error.message, buf);
			goto FAIL;
		}
	NEXT:
		list++;
	}

	return true;

FAIL:
	return false;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	SaveRec * l = ( SaveRec *) fi-> instance;
	struct heif_encoder* encoder = NULL;
	HV * profile = fi-> extras;
	struct heif_image* himg = NULL;
	PIcon i = ( PIcon) fi-> object;
	Bool icon;
	int y, dst_stride, alpha_stride = 0;
	Byte *src, *dst, *alpha = NULL, *mask8 = NULL;
	struct heif_encoding_options* options = NULL;
	enum heif_colorspace colorspace;
	enum heif_chroma chroma;
	char *plugin;

	if ( pexist( encoder )) {
		plugin = pget_c(encoder);
		if ( plugin[0] == 0 )
			plugin = NULL;
	} else if ( default_plugin[0] != 0 )
		plugin = default_plugin;
	else
		plugin = NULL;
	if ( !apply_encoder_options(fi, plugin, &encoder))
		goto FAIL;

	if ( pexist(quality)) {
		char * c = pget_c(quality);
		if ( strcmp(c, "lossless") == 0) {
			CALL heif_encoder_set_lossless(encoder, 1);
			CHECK_HEIF_ERROR;
		} else {
			char * err = NULL;
			int quality = strtol(c, &err, 10);
			if ( *err || quality < 0 || quality > 100 )
				SET_ERROR("quality must be set either to an integer between 0 and 100 or to 'lossless'");
			CALL heif_encoder_set_lossy_quality(encoder, quality);
			CHECK_HEIF_ERROR;
		}
	}


	options = heif_encoding_options_alloc();

	icon = kind_of( fi-> object, CIcon);
	if ( icon ) {
		if ( i-> maskType != 8 ) {
			if ( !( mask8 = i-> self-> convert_mask((Handle) i, 8)))
				SET_ERROR("not enough memory");
		}
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

	if ( pexist(premultiplied_alpha) && pget_i(premultiplied_alpha))
		heif_image_set_premultiplied_alpha(himg, 1);

	if ( icon ) {
		alpha        = mask8 ? mask8 : i-> mask;
		alpha_stride = mask8 ? LINE_SIZE(i->w, 8) : i-> maskLine;
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
	if ( icon && i->type == imByte ) {
		CALL heif_image_add_plane(himg, heif_channel_Alpha, i->w, i->h, 8);
		CHECK_HEIF_ERROR;
		dst = (Byte*) heif_image_get_plane(himg, heif_channel_Y, &dst_stride);
		for (
			y = 0, dst += (i->h - 1) * dst_stride;
			y < i->h;
			y++, alpha += alpha_stride, dst -= dst_stride
		) {
			memcpy( dst, src, i-> w);
		}
	}

	CALL heif_context_encode_image(l->ctx, himg, encoder, options, &l->handles[fi->frame]);
	CHECK_HEIF_ERROR;

	heif_encoding_options_free(options);
	heif_encoder_release(encoder);
	if ( mask8) free(mask8);
	encoder = NULL;
	options = NULL;
	mask8   = NULL;

	if ( !encode_thumbnail(fi, profile))
		goto FAIL;
	if ( !encode_metadata(fi, profile))
		goto FAIL;
	if ( pexist(is_primary) && pget_B(is_primary)) {
		CALL heif_context_set_primary_image(l->ctx, l->handles[fi->frame]);
		CHECK_HEIF_ERROR;
	}

	if ( fi-> frame == fi-> n_frames - 1 ) {
		CALL heif_context_write(l->ctx, &writer, fi->req);
		CHECK_HEIF_ERROR;
	}
	return true;

FAIL:
	if ( mask8)    free(mask8);
	if ( options ) heif_encoding_options_free(options);
	if ( encoder ) heif_encoder_release(encoder);
	return false;

}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * l = ( SaveRec *) fi-> instance;
	int i;

	for ( i = 0; i < fi-> n_frames; i++) {
		if ( l->handles[i] )
			heif_image_handle_release(l->handles[i]);
	}
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
