#include "img.h"
#include "Icon.h"
#include <jxl/version.h>
#include <jxl/decode.h>

#if JPEGXL_MAJOR_VERSION > 0 || JPEGXL_MINOR_VERSION > 6

#define _DEBUG 0
#define DEBUG if (_DEBUG) warn

static char * jxlext[] = { "jxl", NULL };

static int    jxlbpp[] = {
	imRGB,
	imbpp8 | imGrayScale,
	imShort,
	imFloat,
	0
};
static char * jxlfeat[] = {
	NULL
};

static char * loadOutput[] = {
	/* basic */
	"have_container",
	"bits_per_sample",
	"exponent_bits_per_sample",
	"intensity_target",
	"min_nits",
	"relative_to_max_display",
	"linear_below",
	"uses_original_profile",
	"have_preview",
	"have_animation",
	"orientation",
	"num_color_channels",
	"num_extra_channels",
	"alpha_bits",
	"alpha_exponent_bits",
	"alpha_premultiplied",
	"preview_xsize",
	"preview_ysize",
	"intrinsic_xsize",
	"intrinsic_ysize",

	"boxes",

	/* animation */
	"tps_numerator",
	"tps_denominator",
	"num_loops",
	"have_timecodes",

	/* per-frame */
	"duration",
	"timecode",
	"name",
	"is_last",

	/* for layer */
	"crop_x0",
	"crop_y0",

	/* blend info */
	"blend_mode",
	"blend_source",
	"blend_alpha",
	"blend_clamp",
	NULL
};

static char * mime[] = {
	"image/jxl",
	"image/vnd.jxl",
	NULL
};

static ImgCodecInfo codec_info = {
	"JPEG-XL",
	"libjxl",
	JPEGXL_MAJOR_VERSION, JPEGXL_MINOR_VERSION,		      /* version */
	jxlext,		      /* extension */
	"JPEG-XL Image",      /* file type */
	"JXL",		      /* short type */
	jxlfeat,	      /* features  */
	"",		      /* module */
	"",		      /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_MULTIFRAME | IMG_LOAD_FROM_STREAM |
	/* XXX IMG_SAVE_TO_FILE   | IMG_SAVE_MULTIFRAME | IMG_SAVE_TO_STREAM, */ 0,
	jxlbpp,		      /* save types */
	loadOutput,
	mime
};

static void *
init( PImgCodecInfo * info, void * param)
{
	*info = &codec_info;
	return (void*)1;
}

static HV *
load_defaults( PImgCodec c)
{
	HV * profile = newHV();
	pset_i( coalescing,               1);
	pset_f( desired_intensity_target, 0);
	pset_i( skip_reorientation,       0);
	pset_i( unpremul_alpha,           0);
	pset_i( render_spotcolors,        1);
	return profile;
}

#define SIGSIZE 12
#define BUFSIZE 16384

typedef struct _LoadRec {
	unsigned int bytes_read;
	Bool         box_pending;
	Bool         got_basic_info;
	Bool         rewound;
	Bool         has_alpha;
	Bool         is_icon;
	Bool         eof;
	Bool         frame_construction_pending;
	Bool         coalescing;
	Bool         first_call;
	int          current_frame, type;
	long         saved_position;
	PImgLoadFileInstance fi;

	JxlDecoder  *dec;
	JxlBasicInfo basic_info;

	Byte         buf[BUFSIZE];
	Byte         box[BUFSIZE];
	char         curr_box[5];
	char         prev_box[5];
} LoadRec;

#define outc(x){ strlcpy( fi-> errbuf, x, 256); return false;}
#define outcm(dd){ snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", (int)(dd)); return false;}


#define my_JxlDecoderSubscribeEvents(l,fi)(\
	JxlDecoderSubscribeEvents(l, 0  \
		| JXL_DEC_BASIC_INFO    \
		| JXL_DEC_FULL_IMAGE    \
		| JXL_DEC_FRAME         \
		| (fi->loadExtras ? JXL_DEC_BOX : 0) \
	) == JXL_DEC_SUCCESS)

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l;
	Byte buf[SIGSIZE];
	long pos;

	pos = req_tell(fi->req);
	if ( req_read(fi->req, SIGSIZE, buf) != SIGSIZE )
		return false;

	switch ( JxlSignatureCheck( buf, SIGSIZE )) {
	case JXL_SIG_CODESTREAM:
	case JXL_SIG_CONTAINER:
		break;
	default:
		return false;
	}

	fi-> stop = true;

	if ( !( l = malloc( sizeof( LoadRec))))
		return NULL;
	bzero(l, sizeof(LoadRec));
	l-> current_frame = -1;
	l-> saved_position = pos;

	if ( !( l->dec = JxlDecoderCreate(NULL))) {
		free(l);
		return NULL;
	}

	memcpy( l->buf, buf, SIGSIZE );
	l->bytes_read = SIGSIZE;
	l->fi         = fi;
	l->first_call = true;
	l->box_pending  = 0;

	if ( !my_JxlDecoderSubscribeEvents(l->dec,fi))
		goto FAIL;

	DEBUG("jxl: open\n");

	return l;

FAIL:
	JxlDecoderDestroy(l->dec);
	free(l);
	return NULL;
}

static Bool
handle_basic_info( LoadRec *l, PImgLoadFileInstance fi)
{
	HV * profile = fi->fileProperties;
	JxlBasicInfo *b = &l->basic_info;

	if ( l->got_basic_info )
		return true;

	if ( JxlDecoderGetBasicInfo(l->dec, b) != JXL_DEC_SUCCESS)
		outc("cannot get basic info");
	l->got_basic_info = true;

	if ((l-> has_alpha = b->alpha_bits) > 0 )
		l->type = imRGB;
	else if ( b->exponent_bits_per_sample > 0 )
		l->type = imFloat;
	else if ( b->bits_per_sample > 8)
		l->type = imShort;
	else
		l->type = imByte;

	if ( fi->loadExtras) {
		pset_i( have_container,           b-> have_container           );
		pset_i( bits_per_sample,          b-> bits_per_sample          );
		pset_i( exponent_bits_per_sample, b-> exponent_bits_per_sample );
		pset_f( intensity_target,         b-> intensity_target         );
		pset_f( min_nits,                 b-> min_nits                 );
		pset_i( relative_to_max_display,  b-> relative_to_max_display  );
		pset_f( linear_below,             b-> linear_below             );
		pset_i( uses_original_profile,    b-> uses_original_profile    );
		pset_i( orientation,              b-> orientation              );
		pset_i( num_color_channels,       b-> num_color_channels       );
		pset_i( num_extra_channels,       b-> num_extra_channels       );
		pset_i( alpha_bits,               b-> alpha_bits               );
		pset_i( alpha_exponent_bits,      b-> alpha_exponent_bits      );
		pset_i( alpha_premultiplied,      b-> alpha_premultiplied      );
		pset_i( intrinsic_xsize,          b-> intrinsic_xsize          );
		pset_i( intrinsic_ysize,          b-> intrinsic_ysize          );

		if ( b->have_preview ) {
			pset_i( have_preview,             b-> have_preview             );
			pset_i( preview_xsize,            b-> preview.xsize            );
			pset_i( preview_ysize,            b-> preview.ysize            );
		}

		if ( b->have_animation ) {
			pset_i( have_animation,           b-> have_animation           );
			pset_i( tps_numerator,            b-> animation.tps_numerator  );
			pset_i( tps_denominator,          b-> animation.tps_denominator);
			pset_i( num_loops,                b-> animation.num_loops      );
			pset_i( have_timecodes,           b-> animation.have_timecodes );
		}
	}

	return true;
}

static void
callback(void *opaque, size_t x, size_t y, size_t num_pixels, const void *pixels)
{
	LoadRec *l              = (LoadRec*) opaque;
	PImgLoadFileInstance fi = l->fi;
	PIcon i                 = (PIcon) fi-> object;
	register Byte *dst      = i->data + i->lineSize * ( i->h - y - 1 ) + (i->type & imBPP) / 8 * x;

	if ( l->has_alpha && l->is_icon ) {
		register Byte *src  = (Byte*) pixels;
		register Byte *mask = i->mask + i->maskLine * (i->h - y - 1 ) + x;
		while ( num_pixels-- ) {
			*(dst++)  = *(src++);
			*(dst++)  = *(src++);
			*(dst++)  = *(src++);
			*(mask++) = *(src++);
		}
	} else
		memcpy( dst, pixels, (i->type & imBPP) * num_pixels / 8 );
}

static Bool
handle_new_frame( LoadRec *l, PImgLoadFileInstance fi)
{
	JxlFrameHeader h;
	HV * profile = fi-> frameProperties;

	l->current_frame++;
	if ( l->current_frame != fi->frame ) {
		if ( l->frame_construction_pending )
			outc("new frame came while previous not finished");
		return true;
	}

	if ( !l->got_basic_info )
		outc("new frame but no basic info");
	l->frame_construction_pending = true;

	l->is_icon = kind_of( fi-> object, CIcon);
	if ( fi-> noImageData) {
		pset_i( width,  l->basic_info.xsize);
		pset_i( height, l->basic_info.ysize);
		CImage(fi-> object)-> create_empty( fi-> object, 1, 1, l->type);
	} else {
		CImage(fi-> object)-> create_empty( fi-> object, l->basic_info.xsize, l->basic_info.ysize, l->type);
	}

	if ( l->has_alpha && l->is_icon ) {
		PIcon i = ( PIcon) fi-> object;
		i-> autoMasking = amNone;
		i-> self-> set_maskType((Handle) i, imbpp8 );
	}

	if ( !fi-> noImageData) {
		JxlPixelFormat f;
		switch (l->type) {
		case imRGB:
			f.num_channels = 3;
			if ( l->has_alpha && l->is_icon)
				f.num_channels++;
			f.data_type = JXL_TYPE_UINT8;
			break;
		case imByte:
			f.num_channels = 1;
			f.data_type = JXL_TYPE_UINT8;
			break;
		case imShort:
			f.num_channels = 1;
			f.data_type = JXL_TYPE_UINT16;
			break;
		case imFloat:
			f.num_channels = 1;
			f.data_type = JXL_TYPE_FLOAT;
			break;
		default:
			outc("internal error, bad type");
		}

		f.align = 0;
		f.endianness = JXL_NATIVE_ENDIAN;
		JxlDecoderSetImageOutCallback(l->dec, &f, callback, l);
	}

	if (
		fi->loadExtras &&
		(JxlDecoderGetFrameHeader(l->dec, &h) == JXL_DEC_SUCCESS)
	) {
		pset_i( is_last,  h.is_last );
		if ( l->basic_info.have_animation) {
			pset_i( duration, h.duration );
			if ( l->basic_info.animation.have_timecodes)
				pset_i( timecode, h.timecode );
		}
		if ( h.name_length > 0 ) {
			char *name;
			if (( name = malloc(h.name_length+1)) != NULL ) {
				if ( JxlDecoderGetFrameName(l->dec, name, h.name_length+1) == JXL_DEC_SUCCESS)
					pset_c(name, name);
			}
			free(name);
		}

		if ( !l->coalescing ) {
			if ( h.layer_info.have_crop ) {
				pset_i( crop_x0,       h.layer_info.crop_x0  );
				pset_i( crop_y0,       h.layer_info.crop_y0  );
			}

			pset_i( blend_source, h.layer_info.blend_info.source);
			pset_i( blend_alpha,  h.layer_info.blend_info.alpha );
			pset_i( blend_clamp,  h.layer_info.blend_info.clamp );

			switch (h.layer_info.blend_info.blendmode) {
			case JXL_BLEND_REPLACE : pset_c(blend_mode, "replace"); break;
			case JXL_BLEND_ADD     : pset_c(blend_mode, "add"    ); break;
			case JXL_BLEND_BLEND   : pset_c(blend_mode, "blend"  ); break;
			case JXL_BLEND_MULADD  : pset_c(blend_mode, "muladd" ); break;
			case JXL_BLEND_MUL     : pset_c(blend_mode, "mul"    ); break;
			default:                 pset_c(blend_mode, "unknown");
			}

		}
	}

	EVENT_HEADER_READY(fi);

	return true;
}

static Bool
handle_box( LoadRec *l, PImgLoadFileInstance fi, Bool new_box, Bool final)
{
	char *box;

	/* this is based on an assumption that all boxes come prior to the 1st frame */
	if ( l->rewound )
		return true;

	if ( new_box ) {
		int i;
		size_t s;
		JxlBoxType box_type;
		if ( JxlDecoderGetBoxType(l->dec, box_type, JXL_TRUE) != JXL_DEC_SUCCESS)
			return false;
		if ( JxlDecoderGetBoxSizeRaw(l->dec, &s) != JXL_DEC_SUCCESS)
			return false;
		memcpy(l->prev_box, l->curr_box, 5);
		memcpy(l->curr_box, box_type, 4);
		for ( i = 3; i >= 0; i--) {
			if ( l->curr_box[i] != ' ' ) break;
			l->curr_box[i] = 0;
		}
		l->curr_box[4] = 0;
		box = l->prev_box;
		DEBUG( "jxl: box %s, %d bytes\n", l->curr_box, (int)s - 8);
	} else
		box = l->curr_box;

	if ( l->box_pending ) {
		size_t n;
		HV * profile;
		SV * content;

		n = JxlDecoderReleaseBoxBuffer(l->dec);
		l->box_pending = false;

		profile = fi->fileProperties;
		if ( pexist(boxes)) {
			dPROFILE;
			SV * boxes = pget_sv(boxes);
			profile = (HV*) SvRV(boxes);
		} else {
			HV * boxes = newHV();
			pset_sv(boxes, newRV_noinc((SV*)boxes));
			profile = boxes;
		}

		if ( hv_exists(profile, box, strlen(box))) {
			SV **content;
			STRLEN d = BUFSIZE - n;
			content = hv_fetch(profile, box, strlen(box), 0);
			if ( !content )
				outc("internal error");
			sv_catpvn(*content, (char*) l->box, d);
		} else {
			content = newSVpv((char*) l->box, BUFSIZE - n);
			hv_store(profile, box, strlen(box), content, 0);
		}

	} else if ( !new_box )
		outc("internal error: need more box output");

	if ( !final ) {
		if ( JxlDecoderSetBoxBuffer(l->dec, l->box, BUFSIZE) != JXL_DEC_SUCCESS)
			return false;
		l->box_pending = true;
	}

	return true;
}

#if _DEBUG
static const char *
debug_status(int x)
{
	static char buf[16];
	switch (x) {
	case JXL_DEC_SUCCESS                  : return "JXL_DEC_SUCCESS";
	case JXL_DEC_ERROR                    : return "JXL_DEC_ERROR";
	case JXL_DEC_NEED_MORE_INPUT          : return "JXL_DEC_NEED_MORE_INPUT";
	case JXL_DEC_NEED_IMAGE_OUT_BUFFER    : return "JXL_DEC_NEED_IMAGE_OUT_BUFFER";
  	case JXL_DEC_NEED_PREVIEW_OUT_BUFFER  : return "JXL_DEC_NEED_PREVIEW_OUT_BUFFER";
	case JXL_DEC_JPEG_NEED_MORE_OUTPUT    : return "JXL_DEC_JPEG_NEED_MORE_OUTPUT";
	case JXL_DEC_BOX_NEED_MORE_OUTPUT     : return "JXL_DEC_BOX_NEED_MORE_OUTPUT";
	case JXL_DEC_BASIC_INFO               : return "JXL_DEC_BASIC_INFO";
	case JXL_DEC_COLOR_ENCODING           : return "JXL_DEC_COLOR_ENCODING";
	case JXL_DEC_PREVIEW_IMAGE            : return "JXL_DEC_PREVIEW_IMAGE";
	case JXL_DEC_FRAME                    : return "JXL_DEC_FRAME";
	case JXL_DEC_FULL_IMAGE               : return "JXL_DEC_FULL_IMAGE";
	case JXL_DEC_JPEG_RECONSTRUCTION      : return "JXL_DEC_JPEG_RECONSTRUCTION";
	case JXL_DEC_BOX                      : return "JXL_DEC_BOX";
	case JXL_DEC_FRAME_PROGRESSION        : return "JXL_DEC_FRAME_PROGRESSION";
	default:
		snprintf(buf, sizeof(buf), "%x", x);
		return buf;
	}
}
#else
#define debug_status(x) ""
#endif

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;

	DEBUG("jxl: load %d\n", fi->frame);

	if ( l-> first_call ) {
		dPROFILE;
		HV * profile = fi->profile;
		l-> first_call = false;
		l-> coalescing = pexist(coalescing) ? pget_i(coalescing) : 1;
		JxlDecoderSetCoalescing(l->dec, l->coalescing);

		if ( pexist(desired_intensity_target))
			JxlDecoderSetDesiredIntensityTarget(l->dec, pget_f(desired_intensity_target));
		if (pexist(skip_reorientation))
			JxlDecoderSetKeepOrientation(l->dec, pget_B(skip_reorientation));
		if (pexist(unpremul_alpha))
			JxlDecoderSetUnpremultiplyAlpha(l->dec, pget_B(unpremul_alpha));
		if (pexist(render_spotcolors))
			JxlDecoderSetRenderSpotcolors(l->dec, pget_B(render_spotcolors));
		JxlDecoderSetDecompressBoxes(l->dec, true);
	}


	if ( fi->frame <= l->current_frame ) {
		JxlDecoderRewind(l->dec);
		if ( ! my_JxlDecoderSubscribeEvents(l->dec,fi))
			return false;
		req_seek( fi->req, 0, l->saved_position);
		l->eof           = false;
		l->bytes_read    = 0;
		l->current_frame = -1;
		l->frame_construction_pending = false;
		l->rewound       = 1;
	DEBUG("jxl: rewind\n");
	}

	while ( !l->eof || l->bytes_read > 0 ) {
		size_t n;
		JxlDecoderStatus s;

		if ( l->bytes_read < BUFSIZE) {
			n = l->eof ? 0 : req_read(fi->req, BUFSIZE - l->bytes_read, l->buf + l->bytes_read);
			if ( n < 0 || ( n == 0 && l->bytes_read == 0 )) {
				DEBUG("jxl: leave on eof\n");
				return false;
			}
			if ( n == 0 )
				l->eof = true;
			l->bytes_read += n;
		}

AGAIN:
		if ( JxlDecoderSetInput(l->dec, l->buf, l->bytes_read) != JXL_DEC_SUCCESS)
			outc("Internal error");
		s = JxlDecoderProcessInput(l->dec);
		n = JxlDecoderReleaseInput(l->dec);
		DEBUG("jxl: in(%d bytes) -> out(%d bytes%s), status=%s, frame %d\n",
			l->bytes_read, (int)n, l->eof ? " eof" : "", debug_status(s), l->current_frame);

		/* these states can be triggered without reading a single byte */
		switch (s) {
		case JXL_DEC_ERROR:
			outc("decoder error");
		case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
			JxlDecoderSkipCurrentFrame(l->dec);
			if ( l->frame_construction_pending && fi-> noImageData ) {
				if ( l->box_pending && !handle_box(l, fi, true, true))
					return false;
				l->frame_construction_pending = false;
				DEBUG("jxl: leave no pixels needed\n");
				return true;
			}
			goto AGAIN;
		case JXL_DEC_SUCCESS:
			if ( fi->frameCount < 0 )
				fi->frameCount = l->current_frame + 1; /* can report frame count now */
			break;
		case JXL_DEC_BOX:
			if ( !handle_box(l, fi, true, false))
				return false;
			break;
		case JXL_DEC_BOX_NEED_MORE_OUTPUT:
			if ( !handle_box(l, fi, false, false))
				return false;
			break;
		default:
		}

		/* so checking if the input is stalled only/maybe after handling these states */
		if ( n == l->bytes_read && s != JXL_DEC_BOX && s != JXL_DEC_BOX_NEED_MORE_OUTPUT )
			outc("internal error, decoder stuck");
		if ( n > 0 )
			memmove( l->buf, l->buf + l->bytes_read - n, n);
		l->bytes_read = n;

		if ( l-> eof && l->bytes_read == 0 && fi->frameCount < 0 )
			fi->frameCount = l->current_frame + 1; /* can report frame count now */

		switch (s) {
		case JXL_DEC_NEED_MORE_INPUT:
			break;
		case JXL_DEC_BASIC_INFO:
			if ( !handle_basic_info(l, fi))
				return false;
			break;
		case JXL_DEC_FRAME:
			if ( !handle_new_frame(l, fi))
				return false;
			break;
		case JXL_DEC_FULL_IMAGE:
			if ( l->frame_construction_pending ) {
				JxlDecoderFlushImage(l->dec);
				l->frame_construction_pending = false;
				if ( l->box_pending && !handle_box(l, fi, true, true))
					return false;
				DEBUG("jxl: leave on image ready\n");
				return true;
			}
			break;
		default:
		}
	}

	if ( l->frame_construction_pending ) {
		JxlDecoderFlushImage(l->dec);
		l-> frame_construction_pending = false;
		fi-> wasTruncated = 1;
		DEBUG("jxl: leave on incomplete image\n");
		return !fi->noIncomplete;
	}

	outc("decoder error");
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	JxlDecoderDestroy(l->dec);
	DEBUG("jxl: close\n");
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
	return (void*)0;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
//	dPROFILE;
//	HV * profile = fi-> extras;
//	PIcon i = ( PIcon) fi-> object;
//	Bool icon = kind_of( fi-> object, CIcon);

	return false;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
}

void
apc_img_codec_jxl( void )
{
	struct ImgCodecVMT vmt;

	memcpy( &vmt, &CNullImgCodecVMT, sizeof( CNullImgCodecVMT));
	vmt. init          = init;
	vmt. load_defaults = load_defaults;
	vmt. open_load     = open_load;
	vmt. load          = load;
	vmt. close_load    = close_load;
	vmt. save_defaults = save_defaults;
	vmt. open_save     = open_save;
	vmt. save          = save;
	vmt. close_save    = close_save;
	apc_img_register( &vmt, NULL);
}

#else

void apc_img_codec_jxl( void ) {}

#endif

#ifdef __cplusplus
}
#endif
