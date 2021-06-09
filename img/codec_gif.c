#include <sys/types.h>
#include "img.h"
#include "img_conv.h"
#include "Image.h"
#include "Icon.h"
#include <gif_lib.h>

#ifdef __cplusplus
extern "C" {
#endif

static char * gifext[] = { "gif", NULL };
static char * gifver[] = { "gif87a", "gif89a", NULL };
static int    gifbpp[] = {
	imbpp1, imbpp1 | imGrayScale,
	imbpp4,
	imbpp8, imbpp8 | imGrayScale,
	0
};
static char * loadOutput[] = {
	"screenWidth",
	"screenHeight",
	"screenColorResolution",
	"screenBackGroundColor",
	"screenPalette",
	"delayTime",
	"disposalMethod",
	"userInput",
	"transparentColorIndex",
	"comment",
	"left",
	"top",
	"interlaced",
	"loopCount",
	NULL
};

static char * mime[] = {
	"image/gif",
	NULL
};

static ImgCodecInfo codec_info = {
	"GIFLIB",
	"ftp://prtr-13.ucsc.edu/pub/libungif/",
	1, 0,      /* version */
	gifext,    /* extension */
	"Graphics Interchange Format",     /* file type */
	"GIF", /* short type */
	gifver,    /* features  */
	"Prima::Image::gif",     /* module */
	"Prima::Image::gif",     /* package */
	IMG_LOAD_FROM_FILE | IMG_LOAD_MULTIFRAME | IMG_LOAD_FROM_STREAM |
	IMG_SAVE_TO_FILE | IMG_SAVE_MULTIFRAME | IMG_SAVE_TO_STREAM,
	gifbpp, /* save types */
	loadOutput,
	mime
};

static int img_gif_error_code = 0;

#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR >= 5
#define GIF_ERROR_ARG ,&img_gif_error_code
#define MakeMapObject GifMakeMapObject
#define FreeMapObject GifFreeMapObject

#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || (GIFLIB_MAJOR > 5)
#define GIF_ERROR_ARG_51 ,&img_gif_error_code
#else
#define GIF_ERROR_ARG_51
#endif

static int
GifLastError()
{
	return img_gif_error_code;
}

static int
EGifPutExtensionFirst(GifFileType * GifFile,
							int ExtCode,
							int ExtLen,
							const void * Extension) {
	int r = EGifPutExtensionLeader( GifFile, ExtCode );
	if ( r != GIF_OK ) return r;
	return EGifPutExtensionBlock( GifFile, ExtLen, Extension );
}

static int
EGifPutExtensionLast(GifFileType * GifFile,
							int ExtCode,
							int ExtLen,
							const void * Extension) {
	int r = EGifPutExtensionBlock( GifFile, ExtLen, Extension );
	if ( r != GIF_OK ) return r;
	return EGifPutExtensionTrailer( GifFile);
}

#else
#define GIF_ERROR_ARG
#define GIF_ERROR_ARG_51
#endif
#define GIF_CALL        img_gif_error_code =
#define GIF_CALL_FAILED img_gif_error_code != GIF_OK

static void *
init( ImgCodecInfo ** info, void * param)
{
	*info = &codec_info;

#if defined(GIFLIB_MAJOR)
	codec_info.versionMaj = GIFLIB_MAJOR;
	codec_info.versionMin = GIFLIB_MINOR;
#else
	sscanf( GIF_LIB_VERSION, "%*s %d.%d", &codec_info.versionMaj, &codec_info.versionMin);
	if (codec_info.versionMaj > 4) EGifSetGifVersion( "89a");

#endif

	return (void*)1;
}

typedef struct _LoadRec {
	GifFileType * gft;
	GifRecordType grt;
	int           passed;
	int           transparent;
} LoadRec;

static SV *
make_palette_sv( ColorMapObject * pal)
{
	AV * av = newAV();
	SV * sv = newRV_noinc(( SV *) av);
	if ( pal) {
		int i;
		GifColorType * c = pal-> Colors;
		for ( i = 0; i < pal-> ColorCount; i++) {
			av_push( av, newSViv(( int) c-> Blue));
			av_push( av, newSViv(( int) c-> Green));
			av_push( av, newSViv(( int) c-> Red));
			c++;
		}
	}
	return sv;
}

static void
copy_palette( PImage i, ColorMapObject * pal)
{
	int j, last_non_null = -1, first_null = -1;
	PRGBColor r = i-> palette;
	GifColorType * c;

	if ( !pal) return;
	c = pal-> Colors;
	memset( r, 0, 768);
	i-> palSize = ( pal-> ColorCount > 256) ? 256 : pal-> ColorCount;
	for ( j = 0; j < i-> palSize; j++) {
		r-> r = c-> Red;
		r-> g = c-> Green;
		r-> b = c-> Blue;
		if ( r-> r != 0 || r-> g != 0 || r-> b != 0)
			last_non_null = j;
		else if ( first_null < 0 && r-> r == 0 && r-> g == 0 && r-> b == 0)
			first_null = j;
		c++;
		r++;
	}
	/* optimize palette, since gif palette must be **2 only */
	i-> palSize = last_non_null + 1;
	if ( first_null > last_non_null && i-> palSize < 256) i-> palSize++;
}

static void
format_error( int errorID, char * errbuf, int line)
{
	char * msg = NULL;
	switch ( errorID) {
		case D_GIF_ERR_OPEN_FAILED:
		case E_GIF_ERR_OPEN_FAILED:
			msg = "open failed"; break;
		case D_GIF_ERR_READ_FAILED:
			msg = "read failed"; break;
		case D_GIF_ERR_NOT_GIF_FILE:
			msg = "not a valid GIF file"; break;
		case D_GIF_ERR_NO_SCRN_DSCR:
			msg = "no screen description block in the file"; break;
		case D_GIF_ERR_NO_IMAG_DSCR:
			msg = "no image description block in the file"; break;
		case D_GIF_ERR_NO_COLOR_MAP:
			msg = "no color map in the file"; break;
		case D_GIF_ERR_WRONG_RECORD:
			msg = "wrong record type"; break;
		case D_GIF_ERR_DATA_TOO_BIG:
			msg = "queried data size is too big"; break;
		case D_GIF_ERR_NOT_ENOUGH_MEM:
		case E_GIF_ERR_NOT_ENOUGH_MEM:
			msg = "not enough memory"; break;
		case D_GIF_ERR_CLOSE_FAILED:
		case E_GIF_ERR_CLOSE_FAILED:
			msg = "close failed"; break;
		case D_GIF_ERR_NOT_READABLE:
			msg = "file is not readable"; break;
		case D_GIF_ERR_IMAGE_DEFECT:
			msg = "image defect detected"; break;
		case D_GIF_ERR_EOF_TOO_SOON:
			msg = "unexpected end of file reached"; break;
		case E_GIF_ERR_WRITE_FAILED:
			msg = "write failed"; break;
		case E_GIF_ERR_HAS_SCRN_DSCR:
			msg = "screen descriptor already been set"; break;
		case E_GIF_ERR_HAS_IMAG_DSCR:
			msg = "image descriptor is still active"; break;
		case E_GIF_ERR_NO_COLOR_MAP:
			msg = "no color map specified"; break;
		case E_GIF_ERR_DATA_TOO_BIG:
			msg = "data is too big (exceeds size of the image)"; break;
		case E_GIF_ERR_DISK_IS_FULL:
			msg = "storage capacity exceeded"; break;
		case E_GIF_ERR_NOT_WRITEABLE:
			msg = "file opened not for writing"; break;
	}
	if ( msg) snprintf( errbuf, 256, "%s", msg);
}

static int
my_gif_read( GifFileType * gif, GifByteType * buf, int size)
{
	return req_read( ( PImgIORequest) (gif-> UserData), size, buf);
}

static void *
open_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = malloc( sizeof( LoadRec));
	HV * profile = fi-> fileProperties;

	if ( !l) return NULL;
	memset( l, 0, sizeof( LoadRec));

	GIF_CALL 0;
	if ( !( l-> gft = DGifOpen( fi-> req, my_gif_read GIF_ERROR_ARG))) {
		free( l);
		return NULL;
	}
	fi-> stop = true;

	l-> passed  = -1;
	l-> transparent = -1;

	if ( fi-> loadExtras) {
		pset_i( screenWidth,           l-> gft-> SWidth);
		pset_i( screenHeight,          l-> gft-> SHeight);
		pset_i( screenColorResolution, l-> gft-> SColorResolution);
		pset_i( screenBackGroundColor, l-> gft-> SBackGroundColor);
		pset_sv_noinc( screenPalette,  make_palette_sv( l-> gft-> SColorMap));
	}
	return l;
}

#pragma pack(1)
typedef struct _GIFGraphControlExt {
	Byte packedFields;
	Byte delayTimeLo;
	Byte delayTimeHi;
	Byte transparentColorIndex;
} GIFGraphControlExt, *PGIFGraphControlExt;

typedef struct _GIFNetscapeLoopExt {
	Byte subid;
	Byte loopCountLo;
	Byte loopCountHi;
} GIFNetscapeLoopExt, *PGIFNetscapeLoopExt;
#pragma pack()

#define out { format_error( GifLastError(), fi-> errbuf, __LINE__); return false;}
#define outc(x){ strncpy( fi-> errbuf, x, 256); return false;}
#define outcm(dd){ snprintf( fi-> errbuf, 256, "Not enough memory (%d bytes)", dd); return false;}

#define NETSCAPE_EXT_FUNC_CODE 255

static Bool
load_extension( PImgLoadFileInstance fi, int code, Byte * data)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	HV * profile = fi-> frameProperties;

	switch( code) {
	case GRAPHICS_EXT_FUNC_CODE: {
		PGIFGraphControlExt ext = ( PGIFGraphControlExt) (data + 1);
		Byte p = ext-> packedFields;
		if ( fi-> loadExtras) {
			pset_i( delayTime,      ext-> delayTimeLo + ext-> delayTimeHi * 256);
			pset_i( disposalMethod, ( p & 0x1c) >> 2);
			pset_i( userInput,      ( p & 2) ? 1 : 0);
		}
		if ( p & 1) {
			if ( fi-> loadExtras)
				pset_i( transparentColorIndex, ext-> transparentColorIndex);
			l-> transparent = ext-> transparentColorIndex;
		}
		break;
	}

	case COMMENT_EXT_FUNC_CODE: if ( fi-> loadExtras) {
		SV *sv, *mainsv = newSVpv((char*) data + 1, *data);
		pset_sv_noinc( comment, mainsv);
		while ( 1) {
			if (( GIF_CALL DGifGetExtensionNext( l-> gft, &data)) != GIF_OK ) out;
		 if ( data == NULL) break;
			sv = newSVpv((char*) data + 1, *data);
			sv_catsv( mainsv, sv);
			sv_free( sv);
		}
		break;
	}

	case NETSCAPE_EXT_FUNC_CODE:
		if ( *data == 11 && memcmp( data + 1, "NETSCAPE2.0", 11) == 0) {
			LoadRec * l = ( LoadRec *) fi-> instance;
			if ((GIF_CALL DGifGetExtensionNext( l-> gft, &data)) != GIF_OK) out;
				if ( data && *data == 3 && fi-> loadExtras) {
				PGIFNetscapeLoopExt ext = ( PGIFNetscapeLoopExt) ( data + 1);
					pset_i( loopCount, ext-> loopCountLo + 256 * ext-> loopCountHi);
			}
		}
		break;
	}

	while ( data) {
		if (( GIF_CALL DGifGetExtensionNext( l-> gft, &data)) != GIF_OK) out;
	}

	return true;
}

static int interlaceOffs[] = { 0, 4, 2, 1};
static int interlaceStep[] = { 8, 8, 4, 2};

static Bool
load( PImgCodec instance, PImgLoadFileInstance fi)
{
	PImage i = ( PImage) fi-> object;
	LoadRec * l = ( LoadRec *) fi-> instance;
	HV * profile = fi-> frameProperties;
	Bool loop = true;

	/* Reopen file if rewind requested */
	if ( fi-> frame <= l-> passed) {
		DGifCloseFile( l-> gft GIF_ERROR_ARG_51);
		l-> gft = NULL;
		if ( req_seek( fi-> req, 0, SEEK_SET)) {
			snprintf( fi-> errbuf, 256, "Can't rewind GIF stream, seek() error:%s", strerror(req_error( fi-> req)));
			return false;
		}
		GIF_CALL 0;
		if ( !( l-> gft = DGifOpen( fi-> req, my_gif_read GIF_ERROR_ARG))) out;
		l-> passed  = -1;
		l-> transparent = -1;
	}

	while ( loop) {
		GIF_CALL DGifGetRecordType( l-> gft, &l-> grt);
		if ( GIF_CALL_FAILED ) {
			/* handle premature EOF gracefully */
			if ( fi-> frameCount < 0 && l-> passed < fi-> frame)
				fi-> frameCount = l-> passed;
			out;
		}

		switch( l-> grt) {

		case SCREEN_DESC_RECORD_TYPE:
			break;

		case TERMINATE_RECORD_TYPE:
			fi-> frameCount = l-> passed + 1; /* can report how many frames now */
			outc("Frame index out of range");

		case IMAGE_DESC_RECORD_TYPE:
			if (( GIF_CALL DGifGetImageDesc( l-> gft)) != GIF_OK) out;

			if ( ++l-> passed != fi-> frame) { /* skip image block */
				int sz;
				Byte * block = NULL;
				if ((GIF_CALL DGifGetCode( l-> gft, &sz, &block)) != GIF_OK) out;
				while ( block) {
					if (( GIF_CALL DGifGetCodeNext( l-> gft, &block)) != GIF_OK) out;
				}
				break;
			}

			/* loading palette */
			{
				int type = l-> gft-> Image.ColorMap ? l-> gft-> Image.ColorMap-> BitsPerPixel :
							( l-> gft-> SColorMap      ? l-> gft-> SColorMap-> BitsPerPixel : imbpp1);
				if ( type != 1) type = ( type <= 4) ? 4 : 8;
				i-> self-> create_empty(( Handle) i, 1, 1, type);
			}

			if ( l-> gft-> Image.ColorMap != NULL)
				copy_palette( i, l-> gft-> Image. ColorMap);
			else if ( l-> gft-> SColorMap) {
				copy_palette( i, l-> gft-> SColorMap);
				if ( fi-> loadExtras) pset_i( useScreenPalette, 1);
			} else
				i-> palSize = 0;

			if ( fi-> loadExtras) {
				pset_i( interlaced, l-> gft-> Image. Interlace ? 1 : 0);
				pset_i( left,       l-> gft-> Image. Left);
				pset_i( top,        l-> gft-> Image. Top);
			}

			if ( fi-> noImageData) {
				int sz;
				Byte * block = NULL;

				pset_i( width,      l-> gft-> Image. Width);
				pset_i( height,     l-> gft-> Image. Height);

				/* skip block */
				if (( GIF_CALL DGifGetCode( l-> gft, &sz, &block)) != GIF_OK) out;
				while ( block) {
					if (( GIF_CALL DGifGetCodeNext( l-> gft, &block)) != GIF_OK) out;
				}
			} else {
				GifPixelType * data;
				int ls = sizeof( GifPixelType) * l-> gft-> Image. Width, ps = i-> palSize;
				i-> self-> create_empty(( Handle) i,
					l-> gft-> Image. Width, l-> gft-> Image. Height, i-> type);
				i-> palSize = ps;
				data = ( GifPixelType *) malloc( ls * i-> h);
				if ( !data) outcm( ls * i-> h);
				GIF_CALL DGifGetLine( l-> gft, data, i-> w * i-> h);
				if ( GIF_CALL_FAILED ) {
					fi-> wasTruncated = 1;
					if ( fi-> noIncomplete ) {
						free( data);
						out;
					}
					loop = false;
				}

				/* copying & converting data */
				{
					int j, k, pass = 0, idst = 0;
					Byte * src = data, *dst = i-> data;

					for ( j = 0; j < i-> h; j++) {
						dst = i-> data + ( i-> h - 1 - idst) * i-> lineSize;

						for ( k = 0; k < i-> w; k++)
							if ( src[k] >= i-> palSize) i-> palSize = src[k] + 1;

						if ( l-> gft-> Image. Interlace) {
							idst += interlaceStep[ pass];
							if ( idst >= i-> h && pass < 3)
								idst = interlaceOffs[ ++pass];
						} else
							idst++;

						switch( i-> type & imBPP) {
						case imbpp1:
							bc_byte_mono_cr( src, dst, i-> w, map_stdcolorref);   break;
						case imbpp4:
							bc_byte_nibble_cr( src, dst, i-> w, map_stdcolorref); break;
						default:
							memcpy( dst, src, i-> w);
						}
						src += ls;
					}
				}

				free( data);

				/* applying transparent index to Icon */
				if ( kind_of( fi-> object, CIcon) &&
					( l-> transparent >= 0) &&
					( l-> transparent < PIcon( fi-> object)-> palSize)) {
					PIcon( fi-> object)-> maskIndex = l-> transparent;
					PIcon( fi-> object)-> autoMasking = amMaskIndex;
				}
			}

			loop = false;
			break;

		case EXTENSION_RECORD_TYPE:
			{
				int code = -1;
				Byte * data = NULL;
				if (( GIF_CALL DGifGetExtension( l-> gft, &code, &data)) != GIF_OK) out;
				if ( data) {
					if ( 1 + l-> passed != fi-> frame) /* skip this extension block */
						code = -1;
					if ( !load_extension( fi, code, data))
							return false;
				}
			}
			break;
		default:;
		}
	}

	return true;
}

static void
close_load( PImgCodec instance, PImgLoadFileInstance fi)
{
	LoadRec * l = ( LoadRec *) fi-> instance;
	if ( l-> gft) DGifCloseFile( l-> gft GIF_ERROR_ARG_51);
	free( l);
}

static HV *
save_defaults( PImgCodec c)
{
	HV * profile = newHV();
	AV * av = newAV();
	pset_i( screenWidth,  -1);
	pset_i( screenHeight, -1);
	pset_i( screenBackGroundColor, 0);

		av_push( av, newSViv(0));
		av_push( av, newSViv(0));
		av_push( av, newSViv(0));
		av_push( av, newSViv(0xff));
		av_push( av, newSViv(0xff));
		av_push( av, newSViv(0xff));
	pset_sv_noinc( screenPalette, newRV_noinc(( SV *) av));

	pset_i( delayTime,      1);
	pset_i( disposalMethod, 0);
	pset_i( userInput,      0);
	pset_i( transparentColorIndex, 0);
	pset_i( loopCount,      1);

	pset_c( comment, "");

	pset_i( left, 0);
	pset_i( top,  0);
	pset_i( interlaced, 0);

	return profile;
}

static int
my_gif_write( GifFileType * gif, const GifByteType * buf, int size)
{
	return req_write( (( PImgIORequest) (gif-> UserData)), size, ( void*) buf);
}

static void *
open_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	GifFileType * g;

	GIF_CALL 0;
	if ( !( g = EGifOpen( fi-> req, my_gif_write GIF_ERROR_ARG)))
		return NULL;

	return g;
}

static ColorMapObject *
make_colormap( PRGBColor r, int sz)
{
	int j, psz;
	ColorMapObject * ret;
	GifColorType   * c;

	if ( sz <= 2)  psz = 2;  else
	if ( sz <= 4)  psz = 4;  else
	if ( sz <= 8)  psz = 8;  else
	if ( sz <= 16) psz = 16; else
	if ( sz <= 32) psz = 32; else
	if ( sz <= 64) psz = 64; else
	if ( sz <= 128) psz = 128; else
		psz = 256;
	ret = MakeMapObject( psz, NULL);
	if ( !ret) return NULL;
	c = ret-> Colors;
	for ( j = 0; j < sz; j++) {
		c-> Red   = r-> r;
		c-> Green = r-> g;
		c-> Blue  = r-> b;
		c++;
		r++;
	}
	for ( j = sz; j < psz; j++) {
		c-> Red = c-> Green = c-> Blue = 0;
		c++;
	}
	return ret;
}


static Bool
save( PImgCodec instance, PImgSaveFileInstance fi)
{
	dPROFILE;
	GifFileType * g = ( GifFileType *) fi-> instance;
	PImage i = ( PImage) fi-> object;
	HV * profile = fi-> objectExtras;

	if ( fi-> frame == 0) {
		/* put screen description */
		int w = i-> w, h = i-> h, bg = 0, ps = i-> palSize;
		RGBColor * r = i-> palette;
		RGBColor rgbc[ 256];
		ColorMapObject * c;

		if ( pexist( screenWidth))           w  = pget_i( screenWidth);
		if ( pexist( screenHeight))          h  = pget_i( screenHeight);
		if ( pexist( screenBackGroundColor)) bg = pget_i( screenBackGroundColor);
		if ( pexist( screenPalette))
			ps = apc_img_read_palette( r = rgbc, pget_sv( screenPalette), true);
		c = make_colormap( r, ps);
		if ( !c) outcm( ps * 3);
		if ( w < 0) w = i-> w;
		if ( h < 0) h = i-> h;
		GIF_CALL EGifPutScreenDesc( g, w, h, c-> BitsPerPixel, bg, c);
		if (GIF_CALL_FAILED) {
			FreeMapObject( c);
			out;
		}
		FreeMapObject( c);
	}

	/* writing extras */

	/* comments  */
	if ( pexist( comment))
		EGifPutComment( g, pget_c( comment));

	/* graphics control block */
	if ( pexist( delayTime)      ||
		pexist( disposalMethod) ||
		pexist( userInput)      ||
		pexist( transparentColorIndex)) {

		GIFGraphControlExt ext = { 0, 0, 0};
		int disposalMethod = 0, userInput = 0, transparent = 0;

		if ( pexist( delayTime))  {
			int dt = pget_i( delayTime);
			ext. delayTimeLo = dt % 256;
			ext. delayTimeHi = dt / 256;
		}
		if ( pexist( disposalMethod)) disposalMethod = pget_i( disposalMethod);
		if ( pexist( userInput))      userInput      = pget_i( userInput);
		if ( pexist( transparentColorIndex)) {
			int t = pget_i( transparentColorIndex);
			if ( t >= 0) {
				transparent = 1;
				ext. transparentColorIndex = t;
			}
		}
		if ( disposalMethod < 0 || disposalMethod > 3) outc("disposalMethod not in 0..3");
		ext. packedFields =
			( transparent    ? 1 : 0) |
			( userInput      ? 2 : 0) |
			(( disposalMethod & 7) << 2);
		if (( GIF_CALL EGifPutExtension( g, GRAPHICS_EXT_FUNC_CODE,
			sizeof( GIFGraphControlExt), &ext)) != GIF_OK) out;
	}

	/* loop count */
	if ( pexist( loopCount)) {
		int lc = pget_i( loopCount);
		GIFNetscapeLoopExt ext = { 1, lc % 256, lc / 256};
		if (( GIF_CALL EGifPutExtensionFirst( g, NETSCAPE_EXT_FUNC_CODE,
			11, "NETSCAPE2.0")) != GIF_OK) out;
		if (( GIF_CALL EGifPutExtensionLast( g, 0,
			sizeof( GIFNetscapeLoopExt), &ext)) != GIF_OK) out;
	}

	/* writing image */
	{
		ColorMapObject * c = make_colormap( i-> palette, i-> palSize);
		int ox = 0, oy = 0, interlaced = 0;

		GifPixelType * data;
		int ls = sizeof( GifPixelType) * i-> w;
		int j, pass = 0, idst = 0;
		Byte * src, * dst;

		if ( !c) outcm( i-> palSize * 3);
		if ( pexist( left)) ox = pget_i( left);
		if ( pexist( top )) oy = pget_i( top);
		if ( pexist( interlaced )) interlaced = pget_i( interlaced);

		GIF_CALL EGifPutImageDesc( g, ox, oy, i-> w, i-> h, interlaced, c);
		if ( GIF_CALL_FAILED ) {
			FreeMapObject( c);
			out;
		}
		FreeMapObject( c);

		data = malloc( ls * i-> h);
		if ( !data) outcm( ls * i-> h);
		dst = data;

		/* copying & converting data */
		for ( j = 0; j < i-> h; j++) {
			src = i-> data + ( i-> h - 1 - idst) * i-> lineSize;
			if ( interlaced) {
				idst += interlaceStep[ pass];
				if ( idst >= i-> h && pass < 3)
					idst = interlaceOffs[ ++pass];
			} else
				idst++;

			switch( i-> type & imBPP) {
			case imbpp1:
				bc_mono_byte( src, dst, i-> w); break;
			case imbpp4:
				bc_nibble_byte( src, dst, i-> w); break;
			default:
				memcpy( dst, src, i-> w);
			}
			dst += ls;
		}
		GIF_CALL EGifPutLine( g, data, i-> w * i-> h);
		if ( GIF_CALL_FAILED ) {
			free( data);
			out;
		}
		free( data);
	}
	return true;
}


static void
close_save( PImgCodec instance, PImgSaveFileInstance fi)
{
	EGifCloseFile(( GifFileType *) fi-> instance GIF_ERROR_ARG_51);
}

void
apc_img_codec_gif( void )
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
	apc_img_register( &vmt, NULL);
}

#undef out
#undef outc

#ifdef __cplusplus
}
#endif

