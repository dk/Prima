/*

WebP codec with massive borrowings from Google code by
Skal (pascal.massimino@gmail.com) and Urvang (urvang@google.com)

*/
#include "img.h"
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>
#include <webp/mux.h>
#include "Icon.h"
#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

static char * webpext[] = { "webp", NULL };

static int    webpbpp[] = {
	imbpp24,
	0 };

static char * loadOutput[] = {
	"blendMethod",
	"delayTime",
	"disposalMethod",
	"hasAlpha",
	"left",
	"loopCount",
	"background",
	"top",
	"screenWidth",
	"screenHeight",
	NULL
};

static char * mime[] = {
	"image/webp",
	NULL
};

static ImgCodecInfo codec_info = {
	"WebP",
	"Google",
	0, 0,            /* version    */
	webpext,         /* extension  */
	"Web/P Image",   /* file type  */
	"WEBP",          /* short type */
	NULL,            /* features   */
	"",              /* module     */
	"",              /* package    */
	IMG_LOAD_FROM_FILE | IMG_LOAD_FROM_STREAM | IMG_LOAD_MULTIFRAME |
	IMG_SAVE_TO_FILE   | IMG_SAVE_TO_STREAM   | IMG_SAVE_MULTIFRAME,
	webpbpp,         /* save types */
	loadOutput,
	mime
};

static void
my_WebPDataClear(WebPData* webp_data) {
	if (webp_data != NULL) {
		WebPFree((void*)webp_data->bytes);
		WebPDataInit(webp_data);
	}
}


static void *
init( PImgCodecInfo * info, void * param)
{
	int version = WebPGetDecoderVersion();
	codec_info.versionMaj = (version >> 16);
	codec_info.versionMaj = (version >> 8) & 0xff;
	*info = &codec_info;
	return (void*)1;
}

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
	/*
	if background is not RGB, (like clInvalid), use
	whatever it is in the file.
	*/
	pset_f( background, clInvalid);
	return profile;
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
	if ( nread < 0) {
		snprintf( fi-> errbuf, 256, "I/O error:%s", strerror(req_error( fi-> req)));
		return false;
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
		free((void*)l->data.bytes);
		outc("Input file doesn't appear to be WebP format.");
		goto EXIT;
	}

	/* get first info */
	if ((l->dmux = WebPDemux(&l->data)) == NULL) {
		free((void*)l->data.bytes);
		outc("Could not create demuxing object");
		goto EXIT;
	}
	l->canvas_width = WebPDemuxGetI(l->dmux, WEBP_FF_CANVAS_WIDTH);
	l->canvas_height = WebPDemuxGetI(l->dmux, WEBP_FF_CANVAS_HEIGHT);

	if (!WebPDemuxGetFrame(l->dmux, 1, &l->curr_frame)) {
		free((void*)l->data.bytes);
		outc("Could not get first frame");
		goto EXIT;
	}
	fi-> frameCount  = l->curr_frame.num_frames;
	l-> loop_count    = (int)WebPDemuxGetI(l->dmux, WEBP_FF_LOOP_COUNT);
	l-> bg_color      = WebPDemuxGetI(l->dmux, WEBP_FF_BACKGROUND_COLOR);

	if ( fi-> loadExtras) {
		pset_i( screenWidth,  l->canvas_width);
		pset_i( screenHeight, l->canvas_height);
		pset_i( background,   l->bg_color);
		pset_i( loopCount,    l->loop_count);
	}

	return l;

EXIT:
	free(l);
	return NULL;
}

static const char* const load_status_messages[VP8_STATUS_NOT_ENOUGH_DATA + 1] = {
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
format_load_error( PImgLoadFileInstance fi, VP8StatusCode status)
{
	if ( status >= VP8_STATUS_OK && status <= VP8_STATUS_NOT_ENOUGH_DATA) {
		snprintf( fi->errbuf, 256, "%s", load_status_messages[status]);
	} else {
		snprintf( fi->errbuf, 256, "error (code=%d)", status);
	}
}

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	dPROFILE;
	LoadRec * l = ( LoadRec *) fi-> instance;
	WebPIterator* curr = &l->curr_frame;
	WebPDecoderConfig* const config = &l->config;
	WebPDecBuffer* const output_buffer = &config->output;
	VP8StatusCode status;
	HV * profile;
	Bool icon;
	Color background;

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

	profile = fi-> profile;

	/* read pixels */
	icon = kind_of( fi-> object, CIcon) && curr->has_alpha;
	output_buffer->colorspace = fi->blending ? MODE_bgrA : MODE_BGRA;
	if (( status = WebPDecode(curr->fragment.bytes, curr->fragment.size, config)) != VP8_STATUS_OK) {
		format_load_error(fi, status);
		goto EXIT;
	}

	/* set blending background */
	background = l->bg_color;
	if ( pexist( background)) {
		if ( kind_of( fi-> object, CIcon) ) {
			strcpy( fi-> errbuf, "Option 'background' cannot be set when loading to an Icon object");
			goto EXIT;
		}
		if ( (pget_i( background) & clSysFlag) == 0)
			background = pget_i( background);
	}
	background &= 0xffffff;


	l->pic = output_buffer;
	CImage( fi-> object)-> create_empty( fi-> object, curr->width, curr->height, imRGB);
	if ( icon ) {
		CIcon(fi->object)-> set_maskType( fi-> object, imbpp8 );
		PIcon(fi-> object)-> autoMasking = amNone;
	}
	EVENT_HEADER_READY(fi);

	{
		Bool    blend  = !icon && fi->blending && (background != 0);
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
			} else if ( blend ) {
				int32_t R = ( background & 0xff         ) << 8;
				int32_t G = ( (background >> 8)  & 0xff ) << 8;
				int32_t B = ( (background >> 16) & 0xff ) << 8;
				while (x--) {
					register int32_t r = (int32_t)(*(s++) << 8);
					register int32_t g = (int32_t)(*(s++) << 8);
					register int32_t b = (int32_t)(*(s++) << 8);
					register Byte a = 255 - *(s++);
					*(d++) = (r + R * a / 255 + 127) >> 8;
					*(d++) = (g + G * a / 255 + 127) >> 8;
					*(d++) = (b + B * a / 255 + 127) >> 8;
				}
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
	if ( l->data.bytes) free((void*)l-> data.bytes);
	if ( l-> dmux)      WebPDemuxDelete(l->dmux);

	free(l);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	/* image */
	pset_i( background, 0);
	pset_i( loopCount, 0);
	/* encoder */
	pset_c( compression, "lossless");
	pset_f( quality, 75.0);
	pset_i( method, 3);
	pset_i( minimize_size, 0);
	pset_i( filter_strength, 0);
	pset_i( kmin, 9);
	pset_i( kmax, 17);
	pset_i( thread_level, 0);
	/* local */
	pset_i( delay, 100);
	return profile;
}

typedef struct _SaveRec {
	WebPData webp_data;
	WebPAnimEncoder* enc;
	WebPConfig config;
	WebPAnimEncoderOptions enc_options;
	WebPPicture frame;
	WebPMux *mux;
	int timestamp, w, h, loop_count;
} SaveRec;

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec *s;
	if (!(s = malloc(sizeof(SaveRec)))) {
		outcm(sizeof(SaveRec));
		return NULL;
	}
	memset( s, 0, sizeof( SaveRec));
	if (
		!WebPConfigInit(&s->config) ||
		!WebPAnimEncoderOptionsInit(&s->enc_options) ||
      		!WebPPictureInit(&s->frame)
	) {
		outc("Version mismatch");
		free(s);
		return NULL;
	}
	WebPDataInit(&s->webp_data);

	return (void*)s;
}

static Bool
parse_global_options( PImgSaveFileInstance fi, HV * profile, SaveRec * s)
{
	dPROFILE;
	PIcon i;

	/* config */
	if ( pexist(compression)) {
		char * compression = pget_c(compression);
		if ( strcmp(compression, "lossless") == 0) {
			s-> config. lossless = 1;
		} else if ( strcmp(compression, "lossy") == 0) {
			s-> config. lossless = 0;
		} else if ( strcmp(compression, "mixed") == 0) {
			s-> config. lossless = 0;
			s-> enc_options. allow_mixed = 1;
		} else {
			outc("'compression' option must be one of: lossless, lossy, mixed");
			return false;
		}
	} else {
		s-> config. lossless = 1;
	}
	if ( s->config.lossless) {
		s-> enc_options. kmin = 9;
		s-> enc_options. kmax = 17;
	} else {
		s-> enc_options. kmin = 3;
		s-> enc_options. kmax = 5;
	}

	if ( pexist(quality)) {
		s->config.quality = pget_f(quality);
		if (s->config.quality < 0.0 || s->config.quality > 100.0) {
			outc("'quality' option must be in range 0 to 100 (small to big)");
			return false;
		}
	} else
		s->config.quality = 75.0;

	if ( pexist(method)) {
		s->config.method = pget_i(method);
		if (s->config.method < 0 || s->config.method > 6) {
			outc("'method' option must be in range 0 to 6 (fast to slow)");
			return false;
		}
	}

	if (pexist(thread_level))
		s->config.thread_level = pget_B(thread_level);

	if ( pexist(filter_strength)) {
		s->config.filter_strength = pget_i(filter_strength);
		if (s->config.filter_strength < 0 || s->config.filter_strength > 100) {
			outc("'filter_strength' option must be in range 0 to 100 (0=off)");
			return false;
		}
	}

	if (!WebPValidateConfig(&s->config)) {
		outc("Invalid configuration");
		return false;
	}

	/* encoder */
	if ( pexist(minimize_size))
		s->enc_options.minimize_size = pget_B(minimize_size);

	if ( pexist(kmin)) {
		s->enc_options.kmin = pget_i(kmin);
		if (s->enc_options.kmin < 1) {
			outc("'kmin' option must be greater than 0");
			return false;
		}
	}
	if ( pexist(kmax)) {
		s->enc_options.kmax = pget_i(kmax);
		if (s->enc_options.kmax < 1) {
			outc("'kmax' option must be greater than 0");
			return false;
		}
	}
	if ( pexist(loopCount)) {
		s-> loop_count = pget_i(loopCount);
		if (s->loop_count < 0) {
			outc("'loopCount' option must be greater or equal to 0");
			return false;
		}
	}

	/* framebuffer */
	if (pexist(background))
		s->enc_options.anim_params.bgcolor = pget_i(background);

	i = ( PIcon) fi-> object;
	s->w = i->w;
	s->h = i->h;
	if (!(s->enc = WebPAnimEncoderNew(s->w, s->h, &s->enc_options))) {
		outc("Failed to create encoder");
		return false;
	}

	return true;
}

static const char* const save_status_messages[-WEBP_MUX_NOT_ENOUGH_DATA + 1] = {
	"Not found",
	"Invalid parameter",
	"Bad data",
	"Memory error",
	"Not enough data"
};

static void
format_save_error( PImgSaveFileInstance fi, const char * function, WebPMuxError status)
{
	if ( status >= WEBP_MUX_NOT_ENOUGH_DATA && status <= WEBP_MUX_NOT_FOUND) {
		snprintf( fi->errbuf, 256, "%s: %s", function, save_status_messages[-status]);
	} else {
		snprintf( fi->errbuf, 256, "%s error %d", function, status);
	}
}

static Bool
finalize_save(PImgSaveFileInstance fi, SaveRec *s)
{
	WebPMuxAnimParams new_params;
	WebPMuxError err;
	ssize_t nwrote;
	Bool need_loop_count;

	if ( !WebPAnimEncoderAdd(s->enc, NULL, s->timestamp, NULL)) {
		outc(WebPAnimEncoderGetError(s->enc));
		return false;
	}
	if ( !WebPAnimEncoderAssemble(s->enc, &s->webp_data)) {
		outc(WebPAnimEncoderGetError(s->enc));
		return false;
	}

	need_loop_count = fi->frameMapSize > 1;

	if ( need_loop_count ) {
		if (!(s->mux = WebPMuxCreate(&s->webp_data, 1))) {
			outc("Could not remux");
			return false;
		}
		if ((err = WebPMuxGetAnimationParams(s->mux, &new_params)) != WEBP_MUX_OK) {
			if ( err == WEBP_MUX_NOT_FOUND ) {
				/* webp thinks that the produced mux is not an animation */
				goto LEAVE_ANIMATION;
			} else {
				my_WebPDataClear(&s->webp_data);
				format_save_error(fi, "WebPMuxGetAnimationParams", err);
				return false;
			}
		}
		my_WebPDataClear(&s->webp_data);
		new_params.loop_count = s->loop_count;
		if ((err = WebPMuxSetAnimationParams(s->mux, &new_params)) != WEBP_MUX_OK) {
			format_save_error(fi, "WebPMuxSetAnimationParams", err);
			return false;
		}
		if ((err = WebPMuxAssemble(s->mux, &s->webp_data)) != WEBP_MUX_OK) {
			format_save_error(fi, "WebPMuxAssemble", err);
			return false;
		}
	LEAVE_ANIMATION:

		WebPMuxDelete(s->mux);
		s->mux = NULL;
	}

	nwrote = req_write(fi->req, s->webp_data.size, (void*)s->webp_data.bytes);
	if ( nwrote != s->webp_data.size) {
		snprintf( fi-> errbuf, 256, "I/O error:%s", strerror(req_error( fi-> req)));
		return false;
	}

	return true;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	SaveRec *s;
	PIcon i;
	HV * profile;
	Bool icon;
	WebPPicture *frame;
	int y, delay;
	Byte *src, *dst, *mask, *mask_buf;
	RGBColor palette[2];

	s = ( SaveRec *) fi-> instance;
	icon = kind_of( fi-> object, CIcon);
	i = ( PIcon) fi-> object;
	profile = fi-> objectExtras;

	/* settings */
	if ( fi-> frame == 0)
		if (!parse_global_options(fi, profile, s))
			return false;

	if ( i->w != s->w || i->h != s->h) {
		snprintf( fi-> errbuf, 256, "frame #%d size is not the same as the first frame's", fi->frame);
		return false;
	}

	/* Force frames with a small or no duration to 100ms to be consistent
        with web browsers and other transcoding tools. This also avoids
        incorrect durations between frames when padding frames are
        discarded.*/
	if ( pexist(delay)) {
		delay = pget_i(delay);
		if (delay < 10) {
			outc("'delay' option must be greater than 10");
			return false;
		}
	} else
		delay = 100;

	/* copy bits */
	frame = &s->frame;
	frame-> width  = i->w;
	frame-> height = i->h;
	frame-> use_argb = 1;
	mask = mask_buf = NULL;
	if ( icon ) {
		if ( i->maskType != imbpp8 ) {
			if (!(mask_buf = malloc( i->w ))) {
				outcm(i->w * i->h);
				return false;
			}
			memset( &palette[0], 0xff, sizeof(RGBColor));
			memset( &palette[1], 0x00, sizeof(RGBColor));
		}
		mask = i->mask + (i->h-1) * i->maskLine;
	}

	if (!WebPPictureAlloc(frame)) {
		free(mask_buf);
		outc("Cannot allocate picture frame");
		return false;
	}
	src = i->data + (i->h-1) * i->lineSize;
	dst = (Byte*) frame->argb;
	for ( y = 0; y < i->h; y++, src -= i->lineSize, dst += frame->argb_stride * 4) {
		register Byte *s = src;
		register Byte *d = dst;
		int x = i-> w;
		if ( icon ) {
			register Byte *m;
			if ( mask_buf ) {
				bc_mono_graybyte( mask, mask_buf, i-> w, palette);
				m = mask_buf;
			} else
				m = mask;
			while (x--) {
				*(d++) = *(s++);
				*(d++) = *(s++);
				*(d++) = *(s++);
				*(d++) = *(m++);
			}
			mask -= i-> maskLine;
		} else {
			while (x--) {
				*(d++) = *(s++);
				*(d++) = *(s++);
				*(d++) = *(s++);
				*(d++) = 0xff;
			}
		}
	}
	if ( mask_buf ) free(mask_buf);

	if ( !WebPAnimEncoderAdd(s->enc, frame, s->timestamp, &s->config)) {
		WebPPictureFree(frame);
		outc(WebPAnimEncoderGetError(s->enc));
		return false;
	}
	WebPPictureFree(frame);
	s->timestamp += delay;

	/* save file */
	if ( fi->frame == fi->frameMapSize - 1) {
		if (!finalize_save(fi, s))
			return false;
	}

	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * s = ( SaveRec *) fi->instance;

	if (s->enc) WebPAnimEncoderDelete(s->enc);
	my_WebPDataClear(&s->webp_data);
	if (s->mux) WebPMuxDelete(s->mux);

	free( s);
}

void
apc_img_codec_webp( void )
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
