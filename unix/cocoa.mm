/*********************************/
/*                               */
/*  MacOSX functions             */
/*                               */
/*********************************/

#include <sys/param.h>
#include <sys/stat.h>
#include "generic/config.h"

#ifdef WITH_COCOA

#import <Foundation/Foundation.h>
#import <ApplicationServices/ApplicationServices.h>

#ifdef __cplusplus
extern "C" {

static int grab_mode_native = 0;

extern void
prima_if_debug( const char filter, const char *format, ...);
#define Mdebug(...) prima_if_debug('M',__VA_ARGS__)

extern char *
duplicate_string( const char *);

static int
get_screen_height(void);

static uint32_t*
get_bitmap( int x, int y, int xLen, int yLen, int yMax);

extern uint32_t*
prima_cocoa_application_get_bitmap( int x, int y, int xLen, int yLen, int yMax)
{
	if ( !grab_mode_native ) {
		int screen_height = get_screen_height();
		if ( screen_height > 0 ) y += screen_height - yMax;
	}
	return get_bitmap( x, y, xLen, yLen, yMax);
}

extern int
prima_cocoa_is_x11_local(void)
{
	struct stat s;
	char * display_str = getenv("DISPLAY");
	if ( !display_str ) return false;
	if ((stat( display_str, &s) < 0) || !S_ISSOCK(s.st_mode))  /* not a socket */
		return false;
	return true;
}

extern char *
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
#ifdef __cplusplus
}
#endif


#if defined (MAC_OS_VERSION_14_4) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_14_4
#	define SCREEN_CAPTURE_KIT
#endif

#ifdef SCREEN_CAPTURE_KIT
#	import <ScreenCaptureKit/ScreenCaptureKit.h>
#endif

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

static uint32_t*
get_pixels( int xLen, int yLen, CGImageRef cimg)
{
	uint32_t  *pixels          = NULL;
	CGColorSpaceRef colorspace = NULL;
	CGContextRef    context    = NULL;
	if ( !( colorspace = CGColorSpaceCreateDeviceRGB())) 
		goto FAIL;
	if ( !( pixels = (uint32_t*) malloc( xLen * yLen * 4 ))) 
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
	return NULL;
}


static uint32_t*
get_bitmap( int x, int y, int xLen, int yLen, int yMax)
{
#ifdef SCREEN_CAPTURE_KIT
	__block uint32_t  *pixels     = NULL;
	dispatch_group_t group = dispatch_group_create();

	dispatch_group_enter(group);

        [SCShareableContent getCurrentProcessShareableContentWithCompletionHandler:^(
		SCShareableContent* shareable_content, NSError* error
	) {
		if ( error ) {
			Mdebug("mac: SCShareableContent error: %s", [error.localizedDescription UTF8String]);
			dispatch_group_leave(group);
			return;
		}
		if ( shareable_content.displays.count == 0 ) {
			Mdebug("mac: no shareable displays");
			dispatch_group_leave(group);
			return;
		}
		Mdebug("mac: got displays:     %d", (int)shareable_content.displays.count);
		Mdebug("mac: got windows:      %d", (int)shareable_content.windows.count);
		Mdebug("mac: got applications: %d", (int)shareable_content.applications.count);

		SCStreamConfiguration* config = [[[SCStreamConfiguration alloc] init] autorelease];
		config.colorSpaceName               = kCGColorSpaceSRGB;
		config.showsCursor                  = NO;
		config.ignoreShadowsSingleWindow    = YES;
		config.captureResolution            = SCCaptureResolutionBest;
		config.ignoreGlobalClipSingleWindow = YES;
		config.includeChildWindows          = YES;
		config.width                        = (size_t) xLen;
		config.height                       = (size_t) yLen;
		config.sourceRect                   = CGRectMake(x, y, xLen, yLen);

		NSArray<SCWindow*> *nsarray = @[];
		SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:shareable_content.displays[0] excludingWindows:nsarray];

		[SCScreenshotManager captureImageWithFilter:filter configuration:config completionHandler:^(
			CGImageRef cimg, NSError* error
		) {
			if ( error )
				Mdebug("mac: SCScreenshotManager error: %s", [error.localizedDescription UTF8String]);
			else
				pixels = get_pixels(xLen, yLen, cimg);
			dispatch_group_leave(group);
		}];

	}];

	if ( dispatch_group_wait(group, dispatch_time( DISPATCH_TIME_NOW,  1 * NSEC_PER_SEC))) /* wait 1 sec */
		Mdebug("mac: screenshot timeout");

	return pixels;
#else
	uint32_t       *pixels     = NULL;
	CGImageRef      cimg       = NULL;

	/* prepare source */
	CGDisplayHideCursor(kCGDirectMainDisplay);
	if (!( cimg = CGWindowListCreateImage(
		CGRectMake(x, y, xLen, yLen),
		kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault
	))) {
		CGDisplayShowCursor(kCGDirectMainDisplay);
		return NULL;
	}
	CGDisplayShowCursor(kCGDirectMainDisplay);
	pixels = get_pixels(xLen, yLen, cimg);
	CFRelease(cimg);
	return pixels;
#endif
}


#endif
