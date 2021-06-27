/*********************************/
/*                               */
/*  MacOSX functions             */
/*                               */
/*********************************/

#include <sys/param.h>
#include <sys/stat.h>
#include "generic/config.h"

#ifdef WITH_COCOA
#import <ApplicationServices/ApplicationServices.h> 

extern char *
duplicate_string( const char *);

static int
get_screen_height(void)
{
#define MAX_DISPLAYS 32
	static int screen_height = -1;
	uint32_t          display_count = 0;
	CGDirectDisplayID displays[MAX_DISPLAYS];

	if ( screen_height >= 0 ) return screen_height;

	if ( CGGetOnlineDisplayList(MAX_DISPLAYS, displays, &display_count) == kCGErrorSuccess) {
		int i;
		CGRect extents = CGRectMake(0,0,1,1);
		for ( i = 0; i < display_count; i++)
			extents = CGRectUnion(extents, CGDisplayBounds(displays[i]));
		screen_height = extents.size.height + .5;
	} else {
		screen_height = 0;
	}
	return screen_height;
}

static int grab_mode_native = 0;

uint32_t*
prima_cocoa_application_get_bitmap( int x, int y, int xLen, int yLen, int yMax)
{
	CGImageRef      cimg       = NULL;
	CGColorSpaceRef colorspace = NULL;
	CGContextRef    context    = NULL;
	uint32_t       *pixels     = NULL;
	int screen_height;

	if ( !grab_mode_native ) {
		screen_height = get_screen_height();
		if ( screen_height > 0 ) y += screen_height - yMax;
	}

	/* prepare source */
	CGDisplayHideCursor(kCGDirectMainDisplay);
	if (!( cimg = CGWindowListCreateImage(
		CGRectMake(x, y, xLen, yLen),
		kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault
	))) {
		CGDisplayShowCursor(kCGDirectMainDisplay);
		goto FAIL;
	}
	CGDisplayShowCursor(kCGDirectMainDisplay);

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

int
prima_cocoa_is_x11_local(void)
{
	struct stat s;
	char * display_str = getenv("DISPLAY");
	if ( !display_str ) return false;
	if ((stat( display_str, &s) < 0) || !S_ISSOCK(s.st_mode))  /* not a socket */
		return false;
	return true;
}

char *
prima_cocoa_system_action( char * params)
{
	if ( strncmp( params, "screen_height", strlen("screen_height")) == 0) {
		char buf[16];
		snprintf( buf, 16, "%d", get_screen_height());
		return duplicate_string(buf);
	} else if ( strncmp( params, "grab_mode", strlen("grab_mode")) == 0) {
		params += strlen("grab_mode");
		while ( *params == ' ' ) params++;
		if ( !*params )
			return duplicate_string( grab_mode_native ? "native" : "emulated" );
		if ( strncmp(params, "native", strlen("native")) == 0) {
			grab_mode_native = 1;
		} else if ( strncmp(params, "emulated", strlen("emulated")) == 0) {
			grab_mode_native = 0;
		} else {
			fprintf(stderr, "bad grab_mode\n");
		}
		return NULL;
	} else if ( strncmp( params, "local_display", strlen("local_display")) == 0) {
		return prima_cocoa_is_x11_local() ? duplicate_string("1") : NULL;
	} else {
		return NULL;
	}
}
#endif
