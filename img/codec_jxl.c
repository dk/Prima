#include "img.h"
#include "img_conv.h"
#include "Icon.h"
#include <jxl/version.h>
#include <jxl/encode.h>
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
	"intensity_target",
	"min_nits",
	"relative_to_max_display",
	"linear_below",
	"uses_original_profile",
	"orientation",
	"alpha_premultiplied",
	"preview_xsize",
	"preview_ysize",
	"intrinsic_xsize",
	"intrinsic_ysize",

	"exif",

	/* animation */
	"tps_numerator",
	"tps_denominator",
	"num_loops",
	"have_timecodes",

	/* per-frame */
	"duration",
	"timecode",
	"name",

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
	"Prima::Image::jxl",  /* module     */
	"Prima::Image::jxl",  /* package    */
	IMG_LOAD_FROM_FILE | IMG_LOAD_MULTIFRAME | IMG_LOAD_FROM_STREAM |
	IMG_SAVE_TO_FILE   | IMG_SAVE_MULTIFRAME | IMG_SAVE_TO_STREAM,
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
	pset_i( keep_orientation,         0);
	pset_i( unpremul_alpha,           0);
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

	if ((l-> has_alpha = b->alpha_bits) > 0 || b->num_color_channels == 3)
		l->type = imRGB;
	else if ( b->exponent_bits_per_sample > 0 )
		l->type = imFloat;
	else if ( b->bits_per_sample > 8)
		l->type = imShort;
	else
		l->type = imByte;

	if ( fi->loadExtras) {
		pset_i( have_container,           b-> have_container           );
		pset_f( intensity_target,         b-> intensity_target         );
		pset_f( min_nits,                 b-> min_nits                 );
		pset_i( relative_to_max_display,  b-> relative_to_max_display  );
		pset_f( linear_below,             b-> linear_below             );
		pset_i( uses_original_profile,    b-> uses_original_profile    );
		pset_i( orientation,              b-> orientation              );
		pset_i( alpha_premultiplied,      b-> alpha_premultiplied      );
		pset_i( intrinsic_xsize,          b-> intrinsic_xsize          );
		pset_i( intrinsic_ysize,          b-> intrinsic_ysize          );

		if ( b->have_preview ) {
			pset_i( preview_xsize,            b-> preview.xsize            );
			pset_i( preview_ysize,            b-> preview.ysize            );
		}

		if ( b->have_animation ) {
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
		bc_rgba_bgr_a((Byte*) pixels,  dst, i->mask + i->maskLine * (i->h - y - 1 ) + x, num_pixels);
	} else if ( i->type == imRGB) {
		bc_rgb_bgr(pixels, dst, num_pixels);
	} else if ( i->type == imShort ) {
		register uint16_t *s = (uint16_t*) pixels;
		register  int16_t *d = ( int16_t*) dst;
		while ( num_pixels-- )
			*(d++) = (int16_t)((long)(*(s++)) + INT16_MIN);
	} else
		memcpy( dst, pixels, (i->type & imBPP) * num_pixels / 8 );
}

static Bool
handle_new_frame( LoadRec *l, PImgLoadFileInstance fi)
{
	JxlFrameHeader h;
	HV * profile = fi-> frameProperties;
	int type;

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
	type = l->is_icon ? imRGB : l->type;
	if ( fi-> noImageData) {
		pset_i( width,  l->basic_info.xsize);
		pset_i( height, l->basic_info.ysize);
		CImage(fi-> object)-> create_empty( fi-> object, 1, 1, type);
	} else {
		CImage(fi-> object)-> create_empty( fi-> object, l->basic_info.xsize, l->basic_info.ysize, type);
	}

	if ( l->has_alpha && l->is_icon ) {
		PIcon i = ( PIcon) fi-> object;
		i-> autoMasking = amNone;
		i-> self-> set_maskType((Handle) i, imbpp8 );
	}

	if ( !fi-> noImageData) {
		JxlPixelFormat f;
		switch (type) {
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

/* handle Exif only, skip all other boxes */
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
		dPROFILE;
		size_t n;
		HV * profile;
		SV * content;

		n = JxlDecoderReleaseBoxBuffer(l->dec);
		l->box_pending = false;

		if ( strcmp(box, "Exif") == 0) {
			profile = fi->fileProperties;
			if ( pexist(exif)) {
				STRLEN d = BUFSIZE - n;
				content = pget_sv(exif);
				sv_catpvn(content, (char*) l->box, d);
			} else {
				content = newSVpv((char*) l->box, BUFSIZE - n);
				pset_sv(exif, content);
			}
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
potentially_incomplete( LoadRec *l, PImgLoadFileInstance fi)
{
	JxlDecoderFlushImage(l->dec);
	l-> frame_construction_pending = false;
	fi-> wasTruncated = 1;
	DEBUG("jxl: leave on incomplete image\n");
	return !fi->noIncomplete;
}

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
		if (pexist(keep_orientation))
			JxlDecoderSetKeepOrientation(l->dec, pget_B(keep_orientation));
		if (pexist(unpremul_alpha))
			JxlDecoderSetUnpremultiplyAlpha(l->dec, pget_B(unpremul_alpha));
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
				if ( l->frame_construction_pending )
					return potentially_incomplete(l,fi);
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
		default:;
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
		default:;
		}
	}

	if ( l->frame_construction_pending )
		return potentially_incomplete(l,fi);

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
	pset_i ( alpha_premultiplied , 1);
	pset_i ( duration            , 1);
	pset_i ( effort              , 7);
	pset_sv( exif                , NULL_SV);
	pset_f ( frame_distance      , 1.0);
	pset_i ( lossless            , 0);
	pset_i ( use_container       , 0);
	return profile;
}

typedef struct _SaveRec {
	Bool                     icon;
	Bool                     first_frame;
	int                      type_expected;
	Point                    size_expected;
	JxlEncoder              *enc;
	JxlEncoderFrameSettings* frame_settings;
} SaveRec;

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * s;

	if ( !( s = malloc( sizeof( SaveRec))))
		return NULL;

	memset( s, 0, sizeof( SaveRec));

	if (!(s->enc = JxlEncoderCreate(NULL))) {
		free(s);
		return NULL;
	}
	s->frame_settings = JxlEncoderFrameSettingsCreate(s->enc, NULL);
	s->first_frame = true;

	return s;
}

static Bool
init_encoder(SaveRec *s, PImgSaveFileInstance fi)
{
	dPROFILE;
	HV *profile = fi->extras;
	PIcon i = (PIcon) fi->object;
	Bool is_gray = true;
	JxlBasicInfo basic_info;
	JxlColorEncoding color_encoding;

	if ( pexist(use_container) && pget_B(use_container))
		JxlEncoderUseContainer(s->enc, JXL_TRUE);

	JxlEncoderInitBasicInfo(&basic_info);
	basic_info.xsize                 = i->w;
	basic_info.ysize                 = i->h;
	basic_info.num_extra_channels    = 0;
	basic_info.num_color_channels    = 1;
	basic_info.bits_per_sample       = 8;
	basic_info.uses_original_profile = JXL_TRUE;
	basic_info.have_animation        = fi->n_frames > 1;

	s->icon            = kind_of( fi-> object, CIcon);
	s->type_expected   = i->type;
	s->size_expected.x = i->w;
	s->size_expected.y = i->h;

	if ( s->icon ) {
		is_gray = false;
		basic_info.alpha_bits         = 8;
		basic_info.num_color_channels = 3;
		basic_info.num_extra_channels = 1;
	} else switch (i->type) {
		case imRGB:
			is_gray = false;
			basic_info.num_color_channels = 3;
			break;
		case imByte:
			break;
		case imShort:
			basic_info.bits_per_sample = 16;
			break;
		case imFloat:
			basic_info.bits_per_sample = sizeof(float) * 8;
			basic_info.exponent_bits_per_sample = 8;
			break;
		default:
			outc("cannot encode this type");
	}
	if ( JxlEncoderSetBasicInfo(s->enc, &basic_info) != JXL_ENC_SUCCESS)
		outc("encoder error");

	if ( i->type == imFloat )
		JxlColorEncodingSetToLinearSRGB(&color_encoding, true);
	else
		JxlColorEncodingSetToSRGB(&color_encoding, is_gray);
	if ( JxlEncoderSetColorEncoding(s->enc, &color_encoding) != JXL_ENC_SUCCESS)
		outc("encoder error");

	if (pexist(exif)) {
		SV * content = pget_sv(exif);
		if ( SvOK(content)) {
			STRLEN l;
			char * c;
			c = SvPV(content, l);
			JxlEncoderUseBoxes(s->enc);
			if ( JxlEncoderAddBox( s->enc, "Exif", (uint8_t*)c, l, JXL_FALSE) != JXL_ENC_SUCCESS)
				outc("exif encode error");
			JxlEncoderCloseBoxes(s->enc);
		}
	}

	return true;
}

static Bool
process_output( SaveRec *s, PImgSaveFileInstance fi)
{
	while ( 1 ) {
		Byte buffer[BUFSIZE], *next_out = buffer;
		size_t avail_out = BUFSIZE;
		switch ( JxlEncoderProcessOutput(s->enc, &next_out, &avail_out)) {
		case JXL_ENC_SUCCESS:
			if ( req_write(fi->req, next_out - buffer, buffer) != next_out - buffer)
				outc("write error");
			return true;
		case JXL_ENC_NEED_MORE_OUTPUT:
			if ( req_write(fi->req, next_out - buffer, buffer) != next_out - buffer)
				outc("write error");
			continue;
		default:
			outc("frame output error");
		}
	}
	return false;
}

static Bool
save_bits( PIcon i, SaveRec *s, PImgSaveFileInstance fi )
{
	int n;
	JxlEncoderStatus es;
	Byte *dup = NULL;
	Byte *src = i->data + i->lineSize * (i->h - 1), *dst;

	JxlPixelFormat pixel_format = {
		.num_channels = 1,
		.data_type    = JXL_TYPE_UINT8,
		.endianness   = JXL_NATIVE_ENDIAN,
		.align        = i->lineSize
	};

	if ( s->icon ) {
		pixel_format.num_channels = 3;
	} else switch (i->type) {
		case imRGB:
			pixel_format.num_channels = 3;
			break;
		case imByte:
			break;
		case imShort:
			pixel_format.data_type = JXL_TYPE_UINT16;
			break;
		case imFloat:
			pixel_format.data_type = JXL_TYPE_FLOAT;
			if (
				(i->self->stats((Handle)i, 0, isRangeLo, 0) < 0.0) ||
				(i->self->stats((Handle)i, 0, isRangeHi, 0) > 1.0)
			)
				outc("nominal range for float images must be (0,1)");
			break;
		default:
			outc("cannot encode this type");
	}

	if ( !( dup = dst = malloc(i->dataSize)))
		outcm(i->dataSize);

	for (n = 0; n < i->h; n++, src -= i->lineSize, dst += i->lineSize) {
		switch ( i->type ) {
		case imShort: {
			unsigned int a = i->lineSize / 2;
			register int16_t  *s = ( int16_t*) src;
			register uint16_t *d = (uint16_t*) dst;
			while (a--)
				*(d++) = (uint16_t)((long)(*(s++)) - INT16_MIN);
			break;
		}
		case imRGB:
			bc_rgb_bgr(src, dst, i->w);
			break;
		default:
			memcpy(dst, src, i->lineSize);
		}
	}

	es = JxlEncoderAddImageFrame(s->frame_settings, &pixel_format, dup, i->dataSize);
	free( dup );

	if ( es != JXL_ENC_SUCCESS)
		outc("frame encode error");

	return true;
}

static Bool
save_alpha( PIcon i, SaveRec *s, PImgSaveFileInstance fi)
{
	dPROFILE;
	int n;
	RGBColor palette[2];
	HV * profile = fi-> extras;
	Byte *mask8  = NULL;
	JxlEncoderStatus es;
	Byte *src = i->mask + i->maskLine * (i->h - 1), *dst;

	JxlExtraChannelInfo c = {
		.type                = JXL_CHANNEL_ALPHA,
		.bits_per_sample     = 8,
		.name_length         = 0,
		.alpha_premultiplied = 1
	};
	JxlPixelFormat p = {
		.num_channels        = 1,
		.data_type           = JXL_TYPE_UINT8,
		.endianness          = JXL_NATIVE_ENDIAN,
		.align               = ((i->w * 8 + 31) / 32) * 4
	};

	if (pexist(alpha_premultiplied))
		c.alpha_premultiplied = pget_B(alpha_premultiplied);

	if ( JxlEncoderSetExtraChannelInfo(s->enc, 0, &c) != JXL_ENC_SUCCESS)
		outc("alpha encode error");

	if ( !( mask8 = dst = malloc(i->h * p.align)))
		outc("not enough memory");
	if ( i->maskType == 1 ) {
		memset( &palette[0], 0xff, sizeof(RGBColor));
		memset( &palette[1], 0x00, sizeof(RGBColor));
	}

	for (n = 0; n < i->h; n++, src -= i->maskLine, dst += p.align) {
		if ( i->maskType == 1 )
			bc_mono_graybyte( src, dst, i-> w, palette);
		else
			memcpy(dst, src, i->maskLine);
	}

	es = JxlEncoderSetExtraChannelBuffer(
		s->frame_settings, &p,
		mask8, p.align * i->h, 0
	);
	free(mask8);
	if ( es != JXL_ENC_SUCCESS)
		outc("alpha frame encode error");

	return true;
}


static Bool
process_frame( SaveRec *s, PImgSaveFileInstance fi)
{
	dPROFILE;
	HV * profile = fi-> extras;
	PIcon i     = PIcon(fi->object);

	if ( s->type_expected != i->type )
		outc("images must be of the same type");
	if ( s->size_expected.x != i->w || s->size_expected.y != i->h )
		outc("images must be of the same size");
	if ( s->icon && !kind_of(fi->object, CIcon))
		outc("all images must be icons");

	if (fi->n_frames > 1) {
		JxlFrameHeader ff = {
			.duration    = 1,
			.timecode    = 0,
			.name_length = 0,
			.is_last     = ( fi->frame == fi->n_frames - 1)
		};
		if ( pexist(duration))
			ff.duration = pget_i(duration);
		JxlEncoderSetFrameHeader(s->frame_settings, &ff);
	}

	if (pexist(lossless))
		JxlEncoderSetFrameLossless(s->frame_settings, pget_B(lossless));
	if (pexist(frame_distance)) {
		float i = pget_f(frame_distance);
		if ( i < 0.0 || i > 25.0 )
			outc("valid frame_distance values are between 0 and 25");
		JxlEncoderSetFrameDistance(s->frame_settings, i);
	}
	if (pexist(effort)) {
		int i = pget_i(effort);
		if ( i < 1 || i > 10 )
			outc("valid effort values are between 1 and 10");
		JxlEncoderFrameSettingsSetOption(s->frame_settings, JXL_ENC_FRAME_SETTING_EFFORT, i);
	}

	if ( !save_bits(i, s, fi))
		return false;

	if ( s->icon && !save_alpha(i, s, fi))
		return false;

	return true;
}

static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * s = ( SaveRec *) fi-> instance;

	if ( s->first_frame ) {
		if ( !init_encoder(s,fi))
			return false;
		s->first_frame = false;
	}

	if ( !process_frame(s,fi))
		return false;

	if ( fi->frame == fi->n_frames - 1 ) {
		JxlEncoderCloseInput(s->enc);
		if ( !process_output(s,fi))
			return false;
	}

	return true;
}

static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	SaveRec * s = ( SaveRec *) fi-> instance;
	JxlEncoderDestroy(s->enc);
	free( s);
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
