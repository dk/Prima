#include "img_conv.h"

#ifdef __cplusplus
extern "C" {
#endif


#define var (( PImage) self)

#define dEDIFF_ARGS \
	int * err_buf, err_stride = (width + 2) * 3
#define EDIFF_INIT  \
	if (!(err_buf = malloc(err_stride * sizeof( int) * OMP_MAX_THREADS))) return;\
	memset( err_buf, 0, err_stride * sizeof( int) * OMP_MAX_THREADS);
#define EDIFF_DONE free(err_buf)
#define EDIFF_CONV err_buf + err_stride * OMP_THREAD_NUM

#define BCPARMS      self, dstData, dstPal, dstType, dstPalSize, palSize_only
#define FILL_PALETTE(_pal,_palsize,_maxpalsize,_colorref)\
	fill_palette(self,palSize_only,dstPal,dstPalSize,_pal,_palsize,_maxpalsize,_colorref)

/* Mono */

static void
fill_palette( Handle self, Bool palSize_only, RGBColor * dstPal, int * dstPalSize, 
				RGBColor * fillPalette, int fillPalSize, int maxPalSize, Byte * colorref)
{
	Bool do_colormap = 1;
	if ( palSize_only) { 
		if ( var-> palSize > *dstPalSize) {
			cm_squeeze_palette( var-> palette, var-> palSize, dstPal, *dstPalSize);
		} else if ( *dstPalSize > fillPalSize + var-> palSize) {
			memcpy( dstPal, var-> palette, var-> palSize * sizeof(RGBColor));
			memcpy( dstPal + var-> palSize, fillPalette, fillPalSize * sizeof(RGBColor));
			memset( dstPal + var-> palSize + fillPalSize, 0, (*dstPalSize - fillPalSize - var-> palSize) * sizeof(RGBColor));
			do_colormap = 0;
		} else {
			memcpy( dstPal, var-> palette, var-> palSize * sizeof(RGBColor));
			cm_squeeze_palette( fillPalette, fillPalSize, dstPal + var-> palSize, *dstPalSize - var-> palSize);
			do_colormap = 0;
		}
	} else if ( *dstPalSize != 0) {
		if ( *dstPalSize > maxPalSize) 
			cm_squeeze_palette( dstPal, *dstPalSize, dstPal, *dstPalSize = maxPalSize);
	} else if ( var-> palSize > maxPalSize) {
		cm_squeeze_palette( var-> palette, var-> palSize, dstPal, *dstPalSize = maxPalSize);
	} else {
		memmove( dstPal, var-> palette, (*dstPalSize = var-> palSize) * sizeof(RGBColor));
		do_colormap = 0;
	}
	if ( colorref) {
		if ( do_colormap)
			cm_fill_colorref( var->palette, var-> palSize, dstPal, *dstPalSize, colorref);
		else
			memmove( colorref, map_stdcolorref, 256);
	}
}

BC( mono, mono, None)
{
	int ws, mask;
	dBCARGS;
	BCWARN;

	if ( palSize_only || *dstPalSize == 0) 
		memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof( RGBColor));
	
	if ((
		(var->palette[0].r + var->palette[0].g + var->palette[0].b) >
		(var->palette[1].r + var->palette[1].g + var->palette[1].b)
	) == (
		(dstPal[0].r + dstPal[0].g + dstPal[0].b) >
		(dstPal[1].r + dstPal[1].g + dstPal[1].b) 
	)) {
		if ( dstData != var-> data)
			memcpy( dstData, var-> data, var-> dataSize);
	} else {
		/* preserve off-width zeros */
		ws = width >> 3;
		if ((width & 7) == 0) {
			ws--;
			mask = 0xff;
		} else
			mask = (0xff00 >> (width & 7)) & 0xff;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
		for ( i = 0; i < height; i++) {
			register int j;
			dBCLOOP;
			for ( j = 0; j < ws; j++) dstDataLoop[j] =~ srcDataLoop[j];
			dstDataLoop[ws] = (~srcDataLoop[j]) & mask;
		}
	}
}

BC( mono, mono, Optimized)
{
	dBCARGS;
	U16 * tree;
	Byte * buf;
	dEDIFF_ARGS;
	BCWARN;

	FILL_PALETTE( stdmono_palette, 2, 2, nil);

	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;

	EDIFF_INIT;
	if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_mono_byte( srcDataLoop, buf_t, width); 
		bc_byte_op( buf_t, buf_t, width, tree, var-> palette, dstPal, EDIFF_CONV);
		bc_byte_mono_cr( buf_t, dstDataLoop, width, map_stdcolorref); 
	}
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;
	
FAIL:  
	ic_mono_mono_ictNone(BCPARMS);
}

BC( mono, nibble, None)
{
	dBCARGS;
	BCWARN;
	FILL_PALETTE( stdmono_palette, 2, 16, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_mono_nibble_cr( BCCONVLOOP, colorref);
	}
}

BC( mono, byte, None)
{
	dBCARGS;
	BCWARN;
	FILL_PALETTE( stdmono_palette, 2, 256, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_mono_byte_cr( BCCONVLOOP, colorref);
	}
}

BC( mono, graybyte, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_mono_graybyte( BCCONVLOOP, var->palette);
	}
}

BC( mono, rgb, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_mono_rgb( BCCONVLOOP, var->palette);
	}
}

/* Nibble */

BC( nibble, mono, None)
{
	dBCARGS;
	BCWARN;
	FILL_PALETTE( stdmono_palette, 2, 2, colorref);
	cm_fill_colorref( var->palette, var-> palSize, dstPal, *dstPalSize, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_mono_cr( BCCONVLOOP, colorref);
	}
}

BC( nibble, mono, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_mono_ht( BCCONVLOOP, var->palette, i);
	}
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( nibble, mono, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_mono_ed( BCCONVLOOP, var->palette, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( nibble, mono, Optimized)
{
	dBCARGS;
	U16 * tree;
	Byte * buf;
	dEDIFF_ARGS;
	BCWARN;

	FILL_PALETTE( stdmono_palette, 2, 2, nil);
	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;
	EDIFF_INIT;
	if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_nibble_byte( srcDataLoop, buf_t, width);
		bc_byte_op( buf_t, buf_t, width, tree, var-> palette, dstPal, EDIFF_CONV);
		bc_byte_mono_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;
	
FAIL:  
	ic_nibble_mono_ictErrorDiffusion(BCPARMS);
} 

BC( nibble, nibble, None)
{
	dBCARGS;
	int w = (width >> 1) + (width & 1);
	BCWARN;
	FILL_PALETTE( cubic_palette16, 16, 16, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		register int j;
		dBCLOOP;
		for ( j = 0; j < w; j++)
			dstDataLoop[j] = (colorref[srcDataLoop[j] >> 4] << 4) | colorref[srcDataLoop[j] & 0xf]; 
	}
}

BC( nibble, nibble, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_nibble_ht( BCCONVLOOP, var->palette, i);
	}
	memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( nibble, nibble, ErrorDiffusion)
{
	dBCARGS;
	BCWARN;
	dEDIFF_ARGS;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_nibble_ed( BCCONVLOOP, var-> palette, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( nibble, nibble, Optimized)
{
	dBCARGS;
	U16 * tree;
	Byte * buf;
	dEDIFF_ARGS;
	BCWARN;

	FILL_PALETTE( cubic_palette16, 16, 16, nil);

	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;

	EDIFF_INIT;
	if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_nibble_byte( srcDataLoop, buf_t, width); 
		bc_byte_op( buf_t, buf_t, width, tree, var-> palette, dstPal, EDIFF_CONV);
		bc_byte_nibble_cr( buf_t, dstDataLoop, width, map_stdcolorref); 
	}
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;

FAIL:
	ic_nibble_nibble_ictNone(BCPARMS);
}

BC( nibble, byte, None)
{
	dBCARGS;
	BCWARN;
	FILL_PALETTE( cubic_palette, 216, 256, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_byte_cr( BCCONVLOOP, colorref);
	}
}

BC( nibble, graybyte, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_graybyte( BCCONVLOOP, var->palette);
	}
}

BC( nibble, rgb, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_nibble_rgb( BCCONVLOOP, var->palette);
	}
}

/* Byte */
BC( byte, mono, None)
{
	dBCARGS;
	BCWARN;
	FILL_PALETTE( stdmono_palette, 2, 2, colorref);
	cm_fill_colorref( var->palette, var-> palSize, dstPal, *dstPalSize, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_mono_cr( BCCONVLOOP, colorref);
	}
}

BC( byte, mono, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_mono_ht( BCCONVLOOP, var->palette, i);
	}
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( byte, mono, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_mono_ed( BCCONVLOOP, var->palette, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( byte, mono, Optimized)
{
	dBCARGS;
	U16 * tree;
	Byte * buf;
	dEDIFF_ARGS;
	BCWARN;

	FILL_PALETTE( stdmono_palette, 2, 2, nil);
	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;
	EDIFF_INIT;
	if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_byte_op( srcDataLoop, buf_t, width, tree, var-> palette, dstPal, EDIFF_CONV);
		bc_byte_mono_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;

FAIL:
	ic_byte_mono_ictErrorDiffusion(BCPARMS);
}

BC( byte, nibble, None)
{
	dBCARGS;
	BCWARN;
	FILL_PALETTE( cubic_palette16, 16, 16, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_nibble_cr( BCCONVLOOP, colorref);
	}
}

BC( byte, nibble, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_nibble_ht( BCCONVLOOP, var->palette, i);
	}
	memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( byte, nibble, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_nibble_ed( BCCONVLOOP, var->palette, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( byte, nibble, Optimized)
{
	dBCARGS;
	U16 * tree;
	Byte * buf, hist[256];
	RGBColor new_palette[256];
	int j, new_pal_size = 0;
	dEDIFF_ARGS;
	BCWARN;

	if ( *dstPalSize == 0 || palSize_only) {
		int lim = palSize_only ? *dstPalSize : 16;
		memset( hist, 0, sizeof( hist));
		for ( i = 0; i < height; i++) {
			Byte * d = srcData + srcLine * i;
			for ( j = 0; j < width; j++, d++) 
				if ( hist[*d] == 0) {
					hist[*d] = 1;
					new_palette[new_pal_size++] = var-> palette[*d];
					if ( new_pal_size == 256) goto END_HIST_LOOP;
				}
		}
	END_HIST_LOOP:
		if ( new_pal_size > lim) {
			cm_squeeze_palette( new_palette, new_pal_size, new_palette, lim);
			new_pal_size = lim;
		}
	} else
		memcpy( new_palette, dstPal, ( new_pal_size = *dstPalSize) * sizeof(RGBColor));

	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;
	EDIFF_INIT;
	if (!( tree = cm_study_palette( new_palette, new_pal_size))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_byte_op( srcDataLoop, buf_t, width, tree, var-> palette, new_palette, EDIFF_CONV);
		bc_byte_nibble_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	memcpy( dstPal, new_palette, new_pal_size * sizeof(RGBColor));
	*dstPalSize = new_pal_size;
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;

FAIL:
	ic_byte_nibble_ictErrorDiffusion(BCPARMS);
}

BC( byte, byte, None)
{
	dBCARGS;
	int j;
	BCWARN;
	FILL_PALETTE( cubic_palette, 216, 256, colorref);
	for ( i = 0; i < height; i++, srcData += srcLine, dstData += dstLine) {
		for ( j = 0; j < width; j++)
			dstData[j] = colorref[srcData[j]];
	}
}

BC( byte, byte, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_byte_ht( BCCONVLOOP, var->palette, i);
	}
	memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( byte, byte, ErrorDiffusion)
{
	dBCARGS;
	BCWARN;
	dEDIFF_ARGS;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_byte_ed( BCCONVLOOP, var-> palette, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( byte, byte, Optimized)
{
	dBCARGS;
	U16 * tree;
	dEDIFF_ARGS;
	BCWARN;

	FILL_PALETTE( cubic_palette, 216, 256, nil);

	EDIFF_INIT;
	if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
		EDIFF_DONE;
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_op( srcDataLoop, dstDataLoop, width, tree, var-> palette, dstPal, EDIFF_CONV);
	}
	free( tree);
	EDIFF_DONE;
	return;

FAIL:
	ic_byte_byte_ictNone(BCPARMS);
}

BC( byte, graybyte, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_graybyte( BCCONVLOOP, var->palette);
	}
}

BC( byte, rgb, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_rgb( BCCONVLOOP, var->palette);
	}
}

/* Graybyte */
BC( graybyte, mono, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_graybyte_mono_ht( BCCONVLOOP, i);
	}
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( graybyte, mono, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_byte_mono_ed( BCCONVLOOP, std256gray_palette, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( graybyte, nibble, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_graybyte_nibble_ht( BCCONVLOOP, i);
	}
	memcpy( dstPal, std16gray_palette, sizeof( std16gray_palette));
	*dstPalSize = 16;
}

BC( graybyte, nibble, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_graybyte_nibble_ed( BCCONVLOOP, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, std16gray_palette, sizeof( std16gray_palette));
	*dstPalSize = 16;
}

BC( graybyte, rgb, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_graybyte_rgb( BCCONVLOOP);
	}
}

/* RGB */
BC( rgb, mono, None)
{
	dBCARGS;
	BCWARN;
	Byte * convBuf;

	if ( !( convBuf = allocb( width * OMP_MAX_THREADS))) return;
	cm_fill_colorref(( PRGBColor) map_RGB_gray, 256, stdmono_palette, 2, colorref);
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++)
	{
		dBCLOOP;
		Byte * buf_t = convBuf + width * OMP_THREAD_NUM;
		bc_rgb_graybyte( srcDataLoop, buf_t, width);
		bc_byte_mono_cr( buf_t, dstDataLoop, width, colorref);
	}
	free( convBuf);
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( rgb, mono, Posterization)
{
	dBCARGS;
	BCWARN;
	RGBColor new_palette[2], *curr_palette;
	int new_pal_size;
	U16 * tree;
	Byte * buf;

	new_pal_size = *dstPalSize;
	if ( palSize_only ) {
		if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size))
			goto FAIL;
		curr_palette = new_palette;
	} else {
		curr_palette = dstPal;
	}
	if ( !( buf = malloc( width * OMP_MAX_THREADS)))
		goto FAIL;
	if (!( tree = cm_study_palette( curr_palette, new_pal_size))) {
		free(buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_rgb_byte_nop(( RGBColor *) srcDataLoop, buf_t, width, tree, dstPal);
		bc_byte_mono_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	free( tree);
	free( buf);
	*dstPalSize = new_pal_size;
	if ( palSize_only)
		memcpy( dstPal, new_palette, new_pal_size * 3);
	return;

FAIL:
	ic_rgb_mono_ictNone(BCPARMS);
}

BC( rgb, mono, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_mono_ht( BCCONVLOOP, i);
	}
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( rgb, mono, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_mono_ed( BCCONVLOOP, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, stdmono_palette, (*dstPalSize = 2) * sizeof(RGBColor));
}

BC( rgb, mono, Optimized)
{
	dBCARGS;
	Byte * buf;
	U16 * tree;
	dEDIFF_ARGS;
	BCWARN;

	if ( palSize_only) goto FAIL;
	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;
	EDIFF_INIT;
	if (!( tree = cm_study_palette( dstPal, *dstPalSize))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_rgb_byte_op(( RGBColor *) srcDataLoop, buf_t, width, tree, dstPal, EDIFF_CONV);
		bc_byte_mono_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;

FAIL:
	ic_rgb_mono_ictErrorDiffusion(BCPARMS);
}

BC( rgb, nibble, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_nibble(BCCONVLOOP);
	}
	memcpy( dstPal, cubic_palette16, sizeof( cubic_palette16));
	*dstPalSize = 16;
}

BC( rgb, nibble, Posterization)
{
	dBCARGS;
	BCWARN;
	RGBColor new_palette[16], *curr_palette;
	int new_pal_size;
	U16 * tree;
	Byte * buf;

	new_pal_size = *dstPalSize;
	if ( palSize_only ) {
		if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size))
			goto FAIL;
		curr_palette = new_palette;
	} else {
		curr_palette = dstPal;
	}
	if ( !( buf = malloc( width * OMP_MAX_THREADS)))
		goto FAIL;
	if (!( tree = cm_study_palette( curr_palette, new_pal_size))) {
		free(buf);
		goto FAIL;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_rgb_byte_nop(( RGBColor *) srcDataLoop, buf_t, width, tree, dstPal);
		bc_byte_nibble_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	free( tree);
	free( buf);
	*dstPalSize = new_pal_size;
	if ( palSize_only)
		memcpy( dstPal, new_palette, new_pal_size * 3);
	return;

FAIL:
	ic_rgb_nibble_ictNone( BCPARMS );
}

BC( rgb, nibble, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_nibble_ht( BCCONVLOOP, i);
	}
	memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( rgb, nibble, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_nibble_ed( BCCONVLOOP, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, cubic_palette8, (*dstPalSize = 8) * sizeof(RGBColor));
}

BC( rgb, nibble, Optimized)
{
	dBCARGS;
	U16 * tree;
	Byte * buf;
	RGBColor new_palette[16];
	int new_pal_size = 16;
	dEDIFF_ARGS;
	BCWARN;

	if ( *dstPalSize == 0 || palSize_only) {
		if ( palSize_only) new_pal_size = *dstPalSize;
		if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size)) 
			goto FAIL;
	} else
		memcpy( new_palette, dstPal, ( new_pal_size = *dstPalSize) * sizeof(RGBColor));

	if ( !( buf = malloc( width * OMP_MAX_THREADS))) goto FAIL;
	EDIFF_INIT;
	if (!( tree = cm_study_palette( new_palette, new_pal_size))) {
		EDIFF_DONE;
		free( buf);
		goto FAIL;
	}
	memcpy( dstPal, new_palette, new_pal_size * 3);
	*dstPalSize = new_pal_size;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		Byte * buf_t = buf + width * OMP_THREAD_NUM;
		bc_rgb_byte_op(( RGBColor *) srcDataLoop, buf_t, width, tree, dstPal, EDIFF_CONV);
		bc_byte_nibble_cr( buf_t, dstDataLoop, width, map_stdcolorref);
	}
	free( tree);
	free( buf);
	EDIFF_DONE;
	return;

FAIL:
	ic_rgb_nibble_ictErrorDiffusion(BCPARMS);
}

BC( rgb, byte, None)
{
	dBCARGS;
	BCWARN;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_byte( BCCONVLOOP);
	}
	memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( rgb, byte, Posterization)
{
	dBCARGS;
	RGBColor new_palette[256], *curr_palette;
	int new_pal_size;
	U16 * tree;

	BCWARN;
	new_pal_size = *dstPalSize;
	if ( palSize_only ) {
		if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size))
			goto FAIL;
		curr_palette = new_palette;
	} else {
		curr_palette = dstPal;
	}
	if (!( tree = cm_study_palette( curr_palette, new_pal_size)))
		goto FAIL;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_byte_nop(( RGBColor *) srcDataLoop, dstDataLoop, width, tree, dstPal);
	}
	free( tree);
	*dstPalSize = new_pal_size;
	if ( palSize_only)
		memcpy( dstPal, new_palette, new_pal_size * 3);

FAIL:
	ic_rgb_byte_ictNone( BCPARMS );
}

BC( rgb, byte, Ordered)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_byte_ht( BCCONVLOOP, i);
	}
	memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( rgb, byte, ErrorDiffusion)
{
	dBCARGS;
	dEDIFF_ARGS;
	BCWARN;
	EDIFF_INIT;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_byte_ed( BCCONVLOOP, EDIFF_CONV);
	}
	EDIFF_DONE;
	memcpy( dstPal, cubic_palette, (*dstPalSize = 216) * sizeof(RGBColor));
}

BC( rgb, byte, Optimized)
{
	dBCARGS;
	U16 * tree;
	RGBColor new_palette[768];
	int new_pal_size = 256;
	dEDIFF_ARGS;
	BCWARN;
	if ( *dstPalSize == 0 || palSize_only) {
		if ( palSize_only) new_pal_size = *dstPalSize;
		if ( !cm_optimized_palette( srcData, srcLine, width, height, new_palette, &new_pal_size)) 
			goto FAIL;
	} else
		memcpy( new_palette, dstPal, ( new_pal_size = *dstPalSize) * sizeof(RGBColor));

	EDIFF_INIT;
	if (!( tree = cm_study_palette( new_palette, new_pal_size))) {
		EDIFF_DONE;
		goto FAIL;
	}
	memcpy( dstPal, new_palette, new_pal_size * 3);
	*dstPalSize = new_pal_size;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_byte_op(( RGBColor *) srcDataLoop, dstDataLoop, width, tree, dstPal, EDIFF_CONV);
	}
	free( tree);
	EDIFF_DONE;
	return;

FAIL:
	ic_rgb_byte_ictErrorDiffusion(BCPARMS);
}

BC( rgb, graybyte, None)
{
	dBCARGS;
	BCWARN;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
	for ( i = 0; i < height; i++) {
		dBCLOOP;
		bc_rgb_graybyte( BCCONVLOOP);
	}
}

#ifdef __cplusplus
}
#endif
