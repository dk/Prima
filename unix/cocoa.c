/*********************************/
/*                               */
/*  MacOSX functions             */
/*                               */
/*********************************/

#include "generic/config.h"

#ifdef WITH_COCOA
#import <ApplicationServices/ApplicationServices.h> 

uint32_t*
prima_cocoa_application_get_bitmap( int x, int y, int xLen, int yLen)
{
	CGImageRef      cimg       = NULL;
	CGColorSpaceRef colorspace = NULL;
	CGContextRef    context    = NULL;
	uint32_t       *pixels     = NULL;

	/* prepare source */
	if (!( cimg = CGWindowListCreateImage(
		CGRectMake(x, y, xLen, yLen),
		kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault
	)))
		goto FAIL;
	if ( !( colorspace = CGColorSpaceCreateDeviceRGB())) 
		goto FAIL;
	if ( !( pixels = malloc( xLen * yLen * 4 ))) 
		goto FAIL;
 	if ( !( context = CGBitmapContextCreate(
		pixels, xLen, yLen, 8, xLen * 4, colorspace,
		kCGImageAlphaNoneSkipLast)))
		goto FAIL;
	CGContextDrawImage(context, CGRectMake(0, 0, xLen, yLen), cimg);
	CGColorSpaceRelease(colorspace);
	CFRelease(cimg);

	return pixels;

FAIL:
	if (pixels)     free(pixels);
  	if (colorspace) CGColorSpaceRelease(colorspace);
	if (cimg)       CFRelease(cimg);
	return NULL;
	
}

#endif
