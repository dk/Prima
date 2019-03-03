#include "img.h"
#include <webp/decode.h>
#include <webp/demux.h>
#include "Icon.h"

#ifdef __cplusplus
extern "C" {
#endif

static char * webpext[] = { "webp", NULL };

static int    webpbpp[] = {
	imbpp24,
	0 };

static char * loadOutput[] = {
	"hasAlpha",
	"screenWidth",
	"screenHeight",
	"screenBackGroundColor",
	"delayTime",
	"disposalMethod",
	"blendMethod",
	"left",
	"top",
	"loopCount",
	NULL
};

static ImgCodecInfo codec_info = {
	"WebP",
	"Google",
	0, 0,    /* version */
	webpext,    /* extension */
	"Web/P Image",     /* file type */
	"WEBP", /* short type */
	NULL,    /* features  */
	"",     /* module */
	"",     /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_LOAD_MULTIFRAME,
	webpbpp, /* save types */
	loadOutput
};

static void *
init( PImgCodecInfo * info, void * param)
{
	int version = WebPGetDecoderVersion();
	codec_info.versionMaj = (version >> 16);
	codec_info.versionMaj = (version >> 8) & 0xff;
	*info = &codec_info;
	return (void*)1;
}

#define outcm(dd) snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", (int)dd)
#define outc(x)   strncpy( fi-> errbuf, x, 256)


typedef struct _LoadRec {
	int decoding_error;
	int canvas_width, canvas_height;
	int loop_count;
	uint32_t bg_color;

	WebPData data;
	WebPDecoderConfig config;
	WebPDecBuffer* pic;
	WebPDemuxer* dmux;
	WebPIterator curr_frame;
} LoadRec;

static Bool
read_data( PImgLoadFileInstance fi, uint8_t ** data, size_t * data_size, uint8_t * preread, size_t preread_size)
{
	size_t buf_size;
	ssize_t nread;

	buf_size = 16384;
	*data_size = preread_size;

	if ((*data = malloc(buf_size)) == NULL) {
		outcm(buf_size);
		return false;
	}
	memcpy( *data, preread, preread_size);
	while (( nread = req_read(fi->req, buf_size - *data_size, *data + *data_size)) > 0) {
		uint8_t * new_data;
		*data_size += nread;
		buf_size *= 2;
		if ((new_data = realloc( *data, buf_size )) == NULL) {
			free(*data);
			outcm(buf_size);
			return false;
		}
		*data = new_data;
	}
	if ( *data_size == 0 ) {
		outc("Empty input");
		return false;
	}

	return true;
}

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	Byte buf[4];
	HV * profile = fi-> fileProperties;
	long pos;

	/* is it webp at all (grossly) ? */
	pos = req_tell(fi->req);
	if (
		(req_read( fi-> req, 4, buf) < 0) ||
		(memcmp( "RIFF", buf, 4) != 0)
	) {
		req_seek( fi-> req, pos, SEEK_SET);
		return false;
	}
	fi-> stop = true;

	/* init instance and decoder */
	if (( l = malloc( sizeof( LoadRec))) == NULL)
		return NULL;

	memset( l, 0, sizeof( LoadRec));
	fi-> instance = l;

	if (!WebPInitDecoderConfig(&l->config)) {
		outc("Library version mismatch");
		goto EXIT;
	}

	/* read all data */
	{
		uint8_t * data;
		size_t data_size;
		if ( !read_data(fi, &data, &data_size, buf, 4))
			goto EXIT;
		l-> data.bytes = data;
		l-> data.size  = data_size;
	}

	/* now, is it webp? */
	if (!WebPGetInfo(l->data.bytes, l->data.size, NULL, NULL)) {
		WebPDataClear(&l->data);
		outc("Input file doesn't appear to be WebP format.");
		goto EXIT;
	}

	/* get first info */
	if ((l->dmux = WebPDemux(&l->data)) == NULL) {
		WebPDataClear(&l->data);
		outc("Could not create demuxing object");
		goto EXIT;
	}
	l->canvas_width = WebPDemuxGetI(l->dmux, WEBP_FF_CANVAS_WIDTH);
	l->canvas_height = WebPDemuxGetI(l->dmux, WEBP_FF_CANVAS_HEIGHT);

	if (!WebPDemuxGetFrame(l->dmux, 1, &l->curr_frame)) {
		WebPDataClear(&l->data);
		outc("Could not get first frame");
		goto EXIT;
	}
	fi-> frameCount  = l->curr_frame.num_frames;
	l-> loop_count    = (int)WebPDemuxGetI(l->dmux, WEBP_FF_LOOP_COUNT);
	l-> bg_color      = WebPDemuxGetI(l->dmux, WEBP_FF_BACKGROUND_COLOR);

	if ( fi-> loadExtras) {
		pset_i( screenWidth,           l->canvas_width);
		pset_i( screenHeight,          l->canvas_height);
		pset_i( screenBackGroundColor, l->bg_color);
		pset_i( loopCount,             l->loop_count);
	}

	return l;

EXIT:
	free(l);
	return NULL;
}

static const char* const status_messages[VP8_STATUS_NOT_ENOUGH_DATA + 1] = {
	"OK",
	"Out of memory",
	"Invalid parameter",
	"Bitstream error",
	"Unsupported feature",
	"Suspended",
	"Aborted",
	"Not enough data"
};

static void
format_error( PImgLoadFileInstance fi, VP8StatusCode status)
{
	if ( status >= VP8_STATUS_OK && status <= VP8_STATUS_NOT_ENOUGH_DATA) {
		snprintf( fi->errbuf, 256, "status:%d (%s)", status, status_messages[status]);
	} else {
		snprintf( fi->errbuf, 256, "status:%d", status);
	}
}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	WebPIterator* curr = &l->curr_frame;
	WebPDecoderConfig* const config = &l->config;
	WebPDecBuffer* const output_buffer = &config->output;
	VP8StatusCode status;
	HV * profile;
	Bool icon;

	/* position to frame */
	if ( !WebPDemuxGetFrame(l->dmux, fi->frame + 1, curr)) { /* 0th is last */
		outc("Decoding error");
		return false;
	}

	/* read info */
	profile = fi-> frameProperties;
	if ( fi-> loadExtras) {
		pset_i( hasAlpha,       curr->has_alpha);
		pset_i( left,           curr->x_offset);
		pset_i( top,            curr->y_offset);
		pset_i( delayTime,      curr->duration);
		pset_c( disposalMethod,
			(curr->dispose_method == WEBP_MUX_DISPOSE_NONE)       ? "none" :
			(curr->dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) ? "background" :
			                                                        "unknown"
		);
		pset_c( blendMethod,
			(curr->blend_method   == WEBP_MUX_BLEND)              ? "blend" :
			(curr->blend_method   == WEBP_MUX_NO_BLEND)           ? "no_blend" :
			                                                        "unknown"
		);
	}

	if ( fi-> noImageData) {
		CImage( fi-> object)-> create_empty( fi-> object, 1, 1, imRGB);
		pset_i( width,  curr->width);
		pset_i( height, curr->height);
		return true;
	}

	/* read pixels */
	icon = kind_of( fi-> object, CIcon) && curr->has_alpha;
	output_buffer->colorspace = fi->blending ? MODE_bgrA : MODE_BGRA;
	if (( status = WebPDecode(curr->fragment.bytes, curr->fragment.size, config)) != VP8_STATUS_OK) {
		format_error(fi, status);
		goto EXIT;
	}

	l->pic = output_buffer;
	CImage( fi-> object)-> create_empty( fi-> object, curr->width, curr->height, imRGB);
	if ( icon ) {
		CIcon(fi->object)-> set_maskType( fi-> object, imbpp8 );
		PIcon(fi-> object)-> autoMasking = amNone;
	}
	EVENT_HEADER_READY(fi);

	{
		PIcon   i      = PIcon(fi->object);
		int     y      = curr->height,
			stride = l->pic->u.RGBA.stride;
		Byte    *src   = l->pic->u.RGBA.rgba,
			*dst   = i->data + i->lineSize * (y - 1),
			*mask  = icon ? i->mask + i->maskLine * (y - 1) : NULL;

		while(y--) {
			register Byte * s = src;
			register Byte * d = dst;
			register int    x = curr->width;
			if ( icon ) {
				register Byte * m = mask;
				while (x--) {
					*(d++) = *(s++);
					*(d++) = *(s++);
					*(d++) = *(s++);
					*(m++) = *(s++);
				}
				mask -= i->maskLine;
			} else {
				while (x--) {
					*(d++) = *(s++);
					*(d++) = *(s++);
					*(d++) = *(s++);
					s++;
				}
			}
			src  += stride;
			dst  -= i->lineSize;
		}
	}
	WebPFreeDecBuffer(l->pic);
	l->pic = NULL;

	EVENT_TOPDOWN_SCANLINES_FINISHED(fi);
	return true;

EXIT:
	WebPFreeDecBuffer(l->pic);
	l->pic = NULL;
	return false;

}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;

	WebPDemuxReleaseIterator(&l->curr_frame);

	if ( l->pic)        WebPFreeDecBuffer((WebPDecBuffer*)l->pic);
	if ( l->data.bytes) WebPDataClear(&l->data);
	if ( l-> dmux)      WebPDemuxDelete(l->dmux);

	free(l);
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
	return NULL;
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
apc_img_codec_webp( void )
{
	struct ImgCodecVMT vmt;
	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
	vmt. init          = init;
	vmt. open_load     = open_load;
	vmt. load          = load;
	vmt. close_load    = close_load;
	vmt. save_defaults = save_defaults;
	vmt. open_save     = open_save;
	vmt. save          = save;
	vmt. close_save    = close_save;
	apc_img_register( &vmt, nil);
}

#ifdef __cplusplus
}
#endif
