/***********************************************************/
/*                                                         */
/*  System dependent event management (unix, x11)          */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "AbstractMenu.h"
#include "Application.h"
#include "Window.h"
#include "File.h"
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#define SELF_MESSAGE(_eventrec) {\
guts.currentFocusTime=guts.last_time;\
CComponent(self)-> message(self,&_eventrec);\
guts.currentFocusTime=CurrentTime;\
}

void
prima_send_create_event( XWindow win)
{
	XClientMessageEvent ev;

	bzero( &ev, sizeof(ev));
	ev. type = ClientMessage;
	ev. display = DISP;
	ev. window = win;
	ev. message_type = CREATE_EVENT;
	ev. format = 32;
	ev. data. l[0] = 0;
	XSendEvent( DISP, win, false, 0, (XEvent*)&ev);
	XCHECKPOINT;
}

Handle
prima_xw2h( XWindow win)
/*
	tries to map X window to Prima's native handle
*/
{
	Handle self;
	self = (Handle)hash_fetch( guts.windows, (void*)&win, sizeof(win));
	if (!self)
		self = (Handle)hash_fetch( guts.menu_windows, (void*)&win, sizeof(win));
	return self;
}

/* x11 keysym to unicode: from http://cvsweb.xfree86.org/cvsweb/xc/lib/X11/imKStoUCS.c */

static unsigned short const keysym_to_unicode_1a1_1ff[] = {
	        0x0104, 0x02d8, 0x0141, 0x0000, 0x013d, 0x015a, 0x0000, /* 0x01a0-0x01a7 */
	0x0000, 0x0160, 0x015e, 0x0164, 0x0179, 0x0000, 0x017d, 0x017b, /* 0x01a8-0x01af */
	0x0000, 0x0105, 0x02db, 0x0142, 0x0000, 0x013e, 0x015b, 0x02c7, /* 0x01b0-0x01b7 */
	0x0000, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, /* 0x01b8-0x01bf */
	0x0154, 0x0000, 0x0000, 0x0102, 0x0000, 0x0139, 0x0106, 0x0000, /* 0x01c0-0x01c7 */
	0x010c, 0x0000, 0x0118, 0x0000, 0x011a, 0x0000, 0x0000, 0x010e, /* 0x01c8-0x01cf */
	0x0110, 0x0143, 0x0147, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, /* 0x01d0-0x01d7 */
	0x0158, 0x016e, 0x0000, 0x0170, 0x0000, 0x0000, 0x0162, 0x0000, /* 0x01d8-0x01df */
	0x0155, 0x0000, 0x0000, 0x0103, 0x0000, 0x013a, 0x0107, 0x0000, /* 0x01e0-0x01e7 */
	0x010d, 0x0000, 0x0119, 0x0000, 0x011b, 0x0000, 0x0000, 0x010f, /* 0x01e8-0x01ef */
	0x0111, 0x0144, 0x0148, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, /* 0x01f0-0x01f7 */
	0x0159, 0x016f, 0x0000, 0x0171, 0x0000, 0x0000, 0x0163, 0x02d9  /* 0x01f8-0x01ff */
};

static unsigned short const keysym_to_unicode_2a1_2fe[] = {
	        0x0126, 0x0000, 0x0000, 0x0000, 0x0000, 0x0124, 0x0000, /* 0x02a0-0x02a7 */
	0x0000, 0x0130, 0x0000, 0x011e, 0x0134, 0x0000, 0x0000, 0x0000, /* 0x02a8-0x02af */
	0x0000, 0x0127, 0x0000, 0x0000, 0x0000, 0x0000, 0x0125, 0x0000, /* 0x02b0-0x02b7 */
	0x0000, 0x0131, 0x0000, 0x011f, 0x0135, 0x0000, 0x0000, 0x0000, /* 0x02b8-0x02bf */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010a, 0x0108, 0x0000, /* 0x02c0-0x02c7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02c8-0x02cf */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0120, 0x0000, 0x0000, /* 0x02d0-0x02d7 */
	0x011c, 0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x015c, 0x0000, /* 0x02d8-0x02df */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010b, 0x0109, 0x0000, /* 0x02e0-0x02e7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02e8-0x02ef */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0121, 0x0000, 0x0000, /* 0x02f0-0x02f7 */
	0x011d, 0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x015d          /* 0x02f8-0x02ff */
};

static unsigned short const keysym_to_unicode_3a2_3fe[] = {
	                0x0138, 0x0156, 0x0000, 0x0128, 0x013b, 0x0000, /* 0x03a0-0x03a7 */
	0x0000, 0x0000, 0x0112, 0x0122, 0x0166, 0x0000, 0x0000, 0x0000, /* 0x03a8-0x03af */
	0x0000, 0x0000, 0x0000, 0x0157, 0x0000, 0x0129, 0x013c, 0x0000, /* 0x03b0-0x03b7 */
	0x0000, 0x0000, 0x0113, 0x0123, 0x0167, 0x014a, 0x0000, 0x014b, /* 0x03b8-0x03bf */
	0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012e, /* 0x03c0-0x03c7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0116, 0x0000, 0x0000, 0x012a, /* 0x03c8-0x03cf */
	0x0000, 0x0145, 0x014c, 0x0136, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03d0-0x03d7 */
	0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0168, 0x016a, 0x0000, /* 0x03d8-0x03df */
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012f, /* 0x03e0-0x03e7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0117, 0x0000, 0x0000, 0x012b, /* 0x03e8-0x03ef */
	0x0000, 0x0146, 0x014d, 0x0137, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03f0-0x03f7 */
	0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0169, 0x016b          /* 0x03f8-0x03ff */
};

static unsigned short const keysym_to_unicode_4a1_4df[] = {
	        0x3002, 0x3008, 0x3009, 0x3001, 0x30fb, 0x30f2, 0x30a1, /* 0x04a0-0x04a7 */
	0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3, /* 0x04a8-0x04af */
	0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad, /* 0x04b0-0x04b7 */
	0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd, /* 0x04b8-0x04bf */
	0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc, /* 0x04c0-0x04c7 */
	0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, /* 0x04c8-0x04cf */
	0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, /* 0x04d0-0x04d7 */
	0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c  /* 0x04d8-0x04df */
};

static unsigned short const keysym_to_unicode_590_5fe[] = {
	0x06f0, 0x06f1, 0x06f2, 0x06f3, 0x06f4, 0x06f5, 0x06f6, 0x06f7, /* 0x0590-0x0597 */
	0x06f8, 0x06f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x0598-0x059f */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x066a, 0x0670, 0x0679, /* 0x05a0-0x05a7 */

	0x067e, 0x0686, 0x0688, 0x0691, 0x060c, 0x0000, 0x06d4, 0x0000, /* 0x05ac-0x05af */
	0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, /* 0x05b0-0x05b7 */
	0x0668, 0x0669, 0x0000, 0x061b, 0x0000, 0x0000, 0x0000, 0x061f, /* 0x05b8-0x05bf */
	0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, /* 0x05c0-0x05c7 */
	0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, /* 0x05c8-0x05cf */
	0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, /* 0x05d0-0x05d7 */
	0x0638, 0x0639, 0x063a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x05d8-0x05df */
	0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, /* 0x05e0-0x05e7 */
	0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, /* 0x05e8-0x05ef */
	0x0650, 0x0651, 0x0652, 0x0653, 0x0654, 0x0655, 0x0698, 0x06a4, /* 0x05f0-0x05f7 */
	0x06a9, 0x06af, 0x06ba, 0x06be, 0x06cc, 0x06d2, 0x06c1          /* 0x05f8-0x05fe */
};

static unsigned short keysym_to_unicode_680_6ff[] = {
	0x0492, 0x0496, 0x049a, 0x049c, 0x04a2, 0x04ae, 0x04b0, 0x04b2, /* 0x0680-0x0687 */
	0x04b6, 0x04b8, 0x04ba, 0x0000, 0x04d8, 0x04e2, 0x04e8, 0x04ee, /* 0x0688-0x068f */
	0x0493, 0x0497, 0x049b, 0x049d, 0x04a3, 0x04af, 0x04b1, 0x04b3, /* 0x0690-0x0697 */
	0x04b7, 0x04b9, 0x04bb, 0x0000, 0x04d9, 0x04e3, 0x04e9, 0x04ef, /* 0x0698-0x069f */
	0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, /* 0x06a0-0x06a7 */
	0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0491, 0x045e, 0x045f, /* 0x06a8-0x06af */
	0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, /* 0x06b0-0x06b7 */
	0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0490, 0x040e, 0x040f, /* 0x06b8-0x06bf */
	0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, /* 0x06c0-0x06c7 */
	0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, /* 0x06c8-0x06cf */
	0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, /* 0x06d0-0x06d7 */
	0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a, /* 0x06d8-0x06df */
	0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, /* 0x06e0-0x06e7 */
	0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, /* 0x06e8-0x06ef */
	0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, /* 0x06f0-0x06f7 */
	0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a  /* 0x06f8-0x06ff */
};

static unsigned short const keysym_to_unicode_7a1_7f9[] = {
	        0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c, /* 0x07a0-0x07a7 */
	0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015, /* 0x07a8-0x07af */
	0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc, /* 0x07b0-0x07b7 */
	0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07b8-0x07bf */
	0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, /* 0x07c0-0x07c7 */
	0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, /* 0x07c8-0x07cf */
	0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7, /* 0x07d0-0x07d7 */
	0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07d8-0x07df */
	0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, /* 0x07e0-0x07e7 */
	0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, /* 0x07e8-0x07ef */
	0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, /* 0x07f0-0x07f7 */
	0x03c8, 0x03c9                                                  /* 0x07f8-0x07ff */
};

static unsigned short const keysym_to_unicode_8a4_8fe[] = {
	                                0x2320, 0x2321, 0x0000, 0x231c, /* 0x08a0-0x08a7 */
	0x231d, 0x231e, 0x231f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08a8-0x08af */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08b0-0x08b7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222b, /* 0x08b8-0x08bf */
	0x2234, 0x0000, 0x221e, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000, /* 0x08c0-0x08c7 */
	0x2245, 0x2246, 0x0000, 0x0000, 0x0000, 0x0000, 0x22a2, 0x0000, /* 0x08c8-0x08cf */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221a, 0x0000, /* 0x08d0-0x08d7 */
	0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228, /* 0x08d8-0x08df */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e0-0x08e7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e8-0x08ef */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000, /* 0x08f0-0x08f7 */
	0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193          /* 0x08f8-0x08ff */
};

static unsigned short const keysym_to_unicode_9df_9f8[] = {
	                                                        0x2422, /* 0x09d8-0x09df */
	0x2666, 0x25a6, 0x2409, 0x240c, 0x240d, 0x240a, 0x0000, 0x0000, /* 0x09e0-0x09e7 */
	0x240a, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x2500, /* 0x09e8-0x09ef */
	0x0000, 0x0000, 0x0000, 0x0000, 0x251c, 0x2524, 0x2534, 0x252c, /* 0x09f0-0x09f7 */
	0x2502                                                          /* 0x09f8-0x09ff */
};

static unsigned short const keysym_to_unicode_aa1_afe[] = {
	        0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009, /* 0x0aa0-0x0aa7 */
	0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025, /* 0x0aa8-0x0aaf */
	0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a, /* 0x0ab0-0x0ab7 */
	0x2105, 0x0000, 0x0000, 0x2012, 0x2039, 0x2024, 0x203a, 0x0000, /* 0x0ab8-0x0abf */
	0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000, /* 0x0ac0-0x0ac7 */
	0x0000, 0x2122, 0x2120, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25ad, /* 0x0ac8-0x0acf */
	0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033, /* 0x0ad0-0x0ad7 */
	0x0000, 0x271d, 0x0000, 0x220e, 0x25c2, 0x2023, 0x25cf, 0x25ac, /* 0x0ad8-0x0adf */
	0x25e6, 0x25ab, 0x25ae, 0x25b5, 0x25bf, 0x2606, 0x2022, 0x25aa, /* 0x0ae0-0x0ae7 */
	0x25b4, 0x25be, 0x261a, 0x261b, 0x2663, 0x2666, 0x2665, 0x0000, /* 0x0ae8-0x0aef */
	0x2720, 0x2020, 0x2021, 0x2713, 0x2612, 0x266f, 0x266d, 0x2642, /* 0x0af0-0x0af7 */
	0x2640, 0x2121, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e          /* 0x0af8-0x0aff */
};

/* none of the APL keysyms match the Unicode characters */

static unsigned short const keysym_to_unicode_cdf_cfa[] = {
	                                                        0x2017, /* 0x0cd8-0x0cdf */
	0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, /* 0x0ce0-0x0ce7 */
	0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, /* 0x0ce8-0x0cef */
	0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, /* 0x0cf0-0x0cf7 */
	0x05e8, 0x05e9, 0x05ea                                          /* 0x0cf8-0x0cff */
};

static unsigned short const keysym_to_unicode_da1_df9[] = {
	        0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, /* 0x0da0-0x0da7 */
	0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, /* 0x0da8-0x0daf */
	0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, /* 0x0db0-0x0db7 */
	0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, /* 0x0db8-0x0dbf */
	0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, /* 0x0dc0-0x0dc7 */
	0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, /* 0x0dc8-0x0dcf */
	0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, /* 0x0dd0-0x0dd7 */
	0x0e38, 0x0e39, 0x0e3a, 0x0000, 0x0000, 0x0000, 0x0e3e, 0x0e3f, /* 0x0dd8-0x0ddf */
	0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, /* 0x0de0-0x0de7 */
	0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0000, 0x0000, /* 0x0de8-0x0def */
	0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, /* 0x0df0-0x0df7 */
	0x0e58, 0x0e59                                                  /* 0x0df8-0x0dff */
};

static unsigned short const keysym_to_unicode_ea0_eff[] = {
	0x0000, 0x1101, 0x1101, 0x11aa, 0x1102, 0x11ac, 0x11ad, 0x1103, /* 0x0ea0-0x0ea7 */
	0x1104, 0x1105, 0x11b0, 0x11b1, 0x11b2, 0x11b3, 0x11b4, 0x11b5, /* 0x0ea8-0x0eaf */
	0x11b6, 0x1106, 0x1107, 0x1108, 0x11b9, 0x1109, 0x110a, 0x110b, /* 0x0eb0-0x0eb7 */
	0x110c, 0x110d, 0x110e, 0x110f, 0x1110, 0x1111, 0x1112, 0x1161, /* 0x0eb8-0x0ebf */
	0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167, 0x1168, 0x1169, /* 0x0ec0-0x0ec7 */
	0x116a, 0x116b, 0x116c, 0x116d, 0x116e, 0x116f, 0x1170, 0x1171, /* 0x0ec8-0x0ecf */
	0x1172, 0x1173, 0x1174, 0x1175, 0x11a8, 0x11a9, 0x11aa, 0x11ab, /* 0x0ed0-0x0ed7 */
	0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3, /* 0x0ed8-0x0edf */
	0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb, /* 0x0ee0-0x0ee7 */
	0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x0000, /* 0x0ee8-0x0eef */
	0x0000, 0x0000, 0x1140, 0x0000, 0x0000, 0x1159, 0x119e, 0x0000, /* 0x0ef0-0x0ef7 */
	0x11eb, 0x0000, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9, /* 0x0ef8-0x0eff */
};

static unsigned short keysym_to_unicode_12a1_12fe[] = {
	        0x1e02, 0x1e03, 0x0000, 0x0000, 0x0000, 0x1e0a, 0x0000, /* 0x12a0-0x12a7 */
	0x1e80, 0x0000, 0x1e82, 0x1e0b, 0x1ef2, 0x0000, 0x0000, 0x0000, /* 0x12a8-0x12af */
	0x1e1e, 0x1e1f, 0x0000, 0x0000, 0x1e40, 0x1e41, 0x0000, 0x1e56, /* 0x12b0-0x12b7 */
	0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61, /* 0x12b8-0x12bf */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c0-0x12c7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c8-0x12cf */
	0x0174, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6a, /* 0x12d0-0x12d7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0176, 0x0000, /* 0x12d8-0x12df */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e0-0x12e7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e8-0x12ef */
	0x0175, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6b, /* 0x12f0-0x12f7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0177          /* 0x12f0-0x12ff */
};

static unsigned short const keysym_to_unicode_13bc_13be[] = {
	0x0152, 0x0153, 0x0178          /* 0x13b8-0x13bf */
};

static unsigned short keysym_to_unicode_14a1_14ff[] = {
	        0x2741, 0x00a7, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab, /* 0x14a0-0x14a7 */
	0x2014, 0x002e, 0x055d, 0x002c, 0x2013, 0x058a, 0x2026, 0x055c, /* 0x14a8-0x14af */
	0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, /* 0x14b0-0x14b7 */
	0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, /* 0x14b8-0x14bf */
	0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b, /* 0x14c0-0x14c7 */
	0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f, /* 0x14c8-0x14cf */
	0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, /* 0x14d0-0x14d7 */
	0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, /* 0x14d8-0x14df */
	0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b, /* 0x14e0-0x14e7 */
	0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f, /* 0x14e8-0x14ef */
	0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583, /* 0x14f0-0x14f7 */
	0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x2019, 0x0027, /* 0x14f8-0x14ff */
};

static unsigned short keysym_to_unicode_15d0_15f6[] = {
	0x10d0, 0x10d1, 0x10d2, 0x10d3, 0x10d4, 0x10d5, 0x10d6, 0x10d7, /* 0x15d0-0x15d7 */
	0x10d8, 0x10d9, 0x10da, 0x10db, 0x10dc, 0x10dd, 0x10de, 0x10df, /* 0x15d8-0x15df */
	0x10e0, 0x10e1, 0x10e2, 0x10e3, 0x10e4, 0x10e5, 0x10e6, 0x10e7, /* 0x15e0-0x15e7 */
	0x10e8, 0x10e9, 0x10ea, 0x10eb, 0x10ec, 0x10ed, 0x10ee, 0x10ef, /* 0x15e8-0x15ef */
	0x10f0, 0x10f1, 0x10f2, 0x10f3, 0x10f4, 0x10f5, 0x10f6          /* 0x15f0-0x15f7 */
};

static unsigned short keysym_to_unicode_16a0_16f6[] = {
	0x0000, 0x0000, 0xf0a2, 0x1e8a, 0x0000, 0xf0a5, 0x012c, 0xf0a7, /* 0x16a0-0x16a7 */
	0xf0a8, 0x01b5, 0x01e6, 0x0000, 0x0000, 0x0000, 0x0000, 0x019f, /* 0x16a8-0x16af */
	0x0000, 0x0000, 0xf0b2, 0x1e8b, 0x01d1, 0xf0b5, 0x012d, 0xf0b7, /* 0x16b0-0x16b7 */
	0xf0b8, 0x01b6, 0x01e7, 0x0000, 0x0000, 0x01d2, 0x0000, 0x0275, /* 0x16b8-0x16bf */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x018f, 0x0000, /* 0x16c0-0x16c7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16c8-0x16cf */
	0x0000, 0x1e36, 0xf0d2, 0xf0d3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d0-0x16d7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d8-0x16df */
	0x0000, 0x1e37, 0xf0e2, 0xf0e3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e0-0x16e7 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e8-0x16ef */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0259          /* 0x16f0-0x16f6 */
};

static unsigned short const keysym_to_unicode_1e9f_1eff[] = {
	                                                        0x0303,
	0x1ea0, 0x1ea1, 0x1ea2, 0x1ea3, 0x1ea4, 0x1ea5, 0x1ea6, 0x1ea7, /* 0x1ea0-0x1ea7 */
	0x1ea8, 0x1ea9, 0x1eaa, 0x1eab, 0x1eac, 0x1ead, 0x1eae, 0x1eaf, /* 0x1ea8-0x1eaf */
	0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3, 0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7, /* 0x1eb0-0x1eb7 */
	0x1eb8, 0x1eb9, 0x1eba, 0x1ebb, 0x1ebc, 0x1ebd, 0x1ebe, 0x1ebf, /* 0x1eb8-0x1ebf */
	0x1ec0, 0x1ec1, 0x1ec2, 0x1ec3, 0x1ec4, 0x1ec5, 0x1ec6, 0x1ec7, /* 0x1ec0-0x1ec7 */
	0x1ec8, 0x1ec9, 0x1eca, 0x1ecb, 0x1ecc, 0x1ecd, 0x1ece, 0x1ecf, /* 0x1ec8-0x1ecf */
	0x1ed0, 0x1ed1, 0x1ed2, 0x1ed3, 0x1ed4, 0x1ed5, 0x1ed6, 0x1ed7, /* 0x1ed0-0x1ed7 */
	0x1ed8, 0x1ed9, 0x1eda, 0x1edb, 0x1edc, 0x1edd, 0x1ede, 0x1edf, /* 0x1ed8-0x1edf */
	0x1ee0, 0x1ee1, 0x1ee2, 0x1ee3, 0x1ee4, 0x1ee5, 0x1ee6, 0x1ee7, /* 0x1ee0-0x1ee7 */
	0x1ee8, 0x1ee9, 0x1eea, 0x1eeb, 0x1eec, 0x1eed, 0x1eee, 0x1eef, /* 0x1ee8-0x1eef */
	0x1ef0, 0x1ef1, 0x0300, 0x0301, 0x1ef4, 0x1ef5, 0x1ef6, 0x1ef7, /* 0x1ef0-0x1ef7 */
	0x1ef8, 0x1ef9, 0x01a0, 0x01a1, 0x01af, 0x01b0, 0x0309, 0x0323  /* 0x1ef8-0x1eff */
};

static unsigned short const keysym_to_unicode_20a0_20ac[] = {
	0x20a0, 0x20a1, 0x20a2, 0x20a3, 0x20a4, 0x20a5, 0x20a6, 0x20a7, /* 0x20a0-0x20a7 */
	0x20a8, 0x20a9, 0x20aa, 0x20ab, 0x20ac                          /* 0x20a8-0x20af */
};

static unsigned short const keysym_to_unicode_ff00_ff1f[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xff00-0xff07 */
	0x0008, 0x0009, 0x000a, 0x0000, 0x0000, 0x000d, 0x0000, 0x0000, /* 0xff08-0xff0f */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xff10-0xff17 */
	0x0000, 0x0000, 0x0000, 0x001b, 0x0000, 0x0000, 0x0000, 0x0000  /* 0xff18-0xff1f */
};

static unsigned short const keysym_to_unicode_ff80_ffbb[] = {
	0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xff80-0xff87 */
	0x0000, 0x0009, 0x0000, 0x0000, 0x0000, 0x000D, 0x0000, 0x0000, /* 0xff88-0xff8f */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xff90-0xff97 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xff98-0xff9f */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xffa0-0xffa7 */
	0x0000, 0x0000, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, /* 0xffa8-0xffaf */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, /* 0xffb0-0xffb7 */
	0x0038, 0x0039, 0x0000, 0x003d                                  /* 0xffb8-0xffbb */
};

static unsigned int
KeySymToUcs4(KeySym keysym)
{
	/* 'Unicode keysym' */
	if ((keysym & 0xff000000) == 0x01000000)
		return (keysym & 0x00ffffff);

	if (keysym > 0 && keysym < 0x100)
		return keysym;
	else if (keysym > 0x1a0 && keysym < 0x200)
		return keysym_to_unicode_1a1_1ff[keysym - 0x1a1];
	else if (keysym > 0x2a0 && keysym < 0x2ff)
		return keysym_to_unicode_2a1_2fe[keysym - 0x2a1];
	else if (keysym > 0x3a1 && keysym < 0x3ff)
		return keysym_to_unicode_3a2_3fe[keysym - 0x3a2];
	else if (keysym > 0x4a0 && keysym < 0x4e0)
		return keysym_to_unicode_4a1_4df[keysym - 0x4a1];
	else if (keysym > 0x589 && keysym < 0x5ff)
		return keysym_to_unicode_590_5fe[keysym - 0x590];
	else if (keysym > 0x67f && keysym < 0x700)
		return keysym_to_unicode_680_6ff[keysym - 0x680];
	else if (keysym > 0x7a0 && keysym < 0x7fa)
		return keysym_to_unicode_7a1_7f9[keysym - 0x7a1];
	else if (keysym > 0x8a3 && keysym < 0x8ff)
		return keysym_to_unicode_8a4_8fe[keysym - 0x8a4];
	else if (keysym > 0x9de && keysym < 0x9f9)
		return keysym_to_unicode_9df_9f8[keysym - 0x9df];
	else if (keysym > 0xaa0 && keysym < 0xaff)
		return keysym_to_unicode_aa1_afe[keysym - 0xaa1];
	else if (keysym > 0xcde && keysym < 0xcfb)
		return keysym_to_unicode_cdf_cfa[keysym - 0xcdf];
	else if (keysym > 0xda0 && keysym < 0xdfa)
		return keysym_to_unicode_da1_df9[keysym - 0xda1];
	else if (keysym > 0xe9f && keysym < 0xf00)
		return keysym_to_unicode_ea0_eff[keysym - 0xea0];
	else if (keysym > 0x12a0 && keysym < 0x12ff)
		return keysym_to_unicode_12a1_12fe[keysym - 0x12a1];
	else if (keysym > 0x13bb && keysym < 0x13bf)
		return keysym_to_unicode_13bc_13be[keysym - 0x13bc];
	else if (keysym > 0x14a0 && keysym < 0x1500)
		return keysym_to_unicode_14a1_14ff[keysym - 0x14a1];
	else if (keysym > 0x15cf && keysym < 0x15f7)
		return keysym_to_unicode_15d0_15f6[keysym - 0x15d0];
	else if (keysym > 0x169f && keysym < 0x16f7)
		return keysym_to_unicode_16a0_16f6[keysym - 0x16a0];
	else if (keysym > 0x1e9e && keysym < 0x1f00)
		return keysym_to_unicode_1e9f_1eff[keysym - 0x1e9f];
	else if (keysym > 0x209f && keysym < 0x20ad)
		return keysym_to_unicode_20a0_20ac[keysym - 0x20a0];
		/* added by dk */
	else if (keysym > 0xfeff && keysym < 0xff20)
		return keysym_to_unicode_ff00_ff1f[keysym - 0xff00];
	else if (keysym > 0xff80 && keysym < 0xffbc)
		return keysym_to_unicode_ff80_ffbb[keysym - 0xff80];
	else
		return 0;
}
/* end unicode map */

#define UNICODE_HEX_CTRL           0x0001
#define UNICODE_HEX_SHIFT          0x0002
#define UNICODE_HEX_U              0x0004
#define UNICODE_HEX_ENTER          0x0008
#define UNICODE_HEX_KEYMASK        0x000F
#define UNICODE_HEX_PRESS          0x0010
#define UNICODE_HEX_RELEASE        0x0020
#define UNICODE_HEX_PRESSMASK      0x0030
#define UNICODE_HEX_METHOD_WANTED  0x0100
#define UNICODE_HEX_ENTER_WANTED   0x0200
#define UNICODE_HEX_RELEASE_WANTED 0x0400
#define UNICODE_HEX_WANT_MASK      0x0700

static void
feed_unicode_character(Handle self, PEvent ev)
{
	char *x;
	int r;

	r = strtol( guts.unicode_hex_input_buffer, &x, 16);
	guts.unicode_hex_input_flags = 0;
	*guts.unicode_hex_input_buffer = 0;

	if (*x) return;
	if ( r > 0x10ffff ) return;
	
	ev-> cmd = cmKeyDown;
	ev-> key.mod  = kmUnicode;
	ev-> key.key  = kbNoKey;
	ev-> key.code = r;
}

U32
prima_keysym_to_keycode( KeySym keysym, XKeyEvent *ev, U8 character )
{
	U32 keycode;

	switch (keysym) {
	/* virtual keys-modifiers */
	case XK_Shift_L:      keycode = kbShiftL;     break;
	case XK_Shift_R:      keycode = kbShiftR;     break;
	case XK_Control_L:    keycode = kbCtrlL;      break;
	case XK_Control_R:    keycode = kbCtrlR;      break;
	case XK_Alt_L:        keycode = kbAltL;       break;
	case XK_Alt_R:        keycode = kbAltR;       break;
	case XK_Meta_L:       keycode = kbMetaL;      break;
	case XK_Meta_R:       keycode = kbMetaR;      break;
	case XK_Super_L:      keycode = kbSuperL;     break;
	case XK_Super_R:      keycode = kbSuperR;     break;
	case XK_Hyper_L:      keycode = kbHyperL;     break;
	case XK_Hyper_R:      keycode = kbHyperR;     break;
	case XK_Caps_Lock:    keycode = kbCapsLock;   break;
	case XK_Num_Lock:     keycode = kbNumLock;    break;
	case XK_Scroll_Lock:  keycode = kbScrollLock; break;
	case XK_Shift_Lock:   keycode = kbShiftLock;  break;
	/* virtual keys with charcode */
	case XK_BackSpace:    keycode = kbBackspace;  break;
	case XK_Tab:          keycode = kbTab;        break;
	case XK_KP_Tab:       keycode = kbKPTab;      break;
	case XK_Linefeed:     keycode = kbLinefeed;   break;
	case XK_Return:       keycode = kbEnter;      break;
	case XK_KP_Enter:     keycode = kbKPEnter;    break;
	case XK_Escape:       keycode = kbEscape;     break;
	case XK_KP_Space:     keycode = kbKPSpace;    break;
	case XK_KP_Equal:     keycode = kbKPEqual;    break;
	case XK_KP_Multiply:  keycode = kbKPMultiply; break;
	case XK_KP_Add:       keycode = kbKPAdd;      break;
	case XK_KP_Separator: keycode = kbKPSeparator;break;
	case XK_KP_Subtract:  keycode = kbKPSubtract; break;
	case XK_KP_Decimal:   keycode = kbKPDecimal;  break;
	case XK_KP_Divide:    keycode = kbKPDivide;   break;
	case XK_KP_0:         keycode = kbKP0;        break;
	case XK_KP_1:         keycode = kbKP1;        break;
	case XK_KP_2:         keycode = kbKP2;        break;
	case XK_KP_3:         keycode = kbKP3;        break;
	case XK_KP_4:         keycode = kbKP4;        break;
	case XK_KP_5:         keycode = kbKP5;        break;
	case XK_KP_6:         keycode = kbKP6;        break;
	case XK_KP_7:         keycode = kbKP7;        break;
	case XK_KP_8:         keycode = kbKP8;        break;
	case XK_KP_9:         keycode = kbKP9;        break;
	/* Other virtual keys */
	case XK_Clear:        keycode = kbClear;      break;
	case XK_Pause:        keycode = kbPause;      break;
	case XK_Sys_Req:      keycode = kbSysReq;     break;
	case XK_Delete:       keycode = kbDelete;     break;
	case XK_KP_Delete:    keycode = kbKPDelete;   break;
	case XK_Home:         keycode = kbHome;       break;
	case XK_KP_Home:      keycode = kbKPHome;     break;
	case XK_Left:         keycode = kbLeft;       break;
	case XK_KP_Left:      keycode = kbKPLeft;     break;
	case XK_Up:           keycode = kbUp;         break;
	case XK_KP_Up:        keycode = kbKPUp;       break;
	case XK_Right:        keycode = kbRight;      break;
	case XK_KP_Right:     keycode = kbKPRight;    break;
	case XK_Down:         keycode = kbDown;       break;
	case XK_KP_Down:      keycode = kbKPDown;     break;
	case XK_Prior:        keycode = kbPrior;      break;
	case XK_KP_Prior:     keycode = kbKPPrior;    break;
	case XK_Next:         keycode = kbNext;       break;
	case XK_KP_Next:      keycode = kbKPNext;     break;
	case XK_End:          keycode = kbEnd;        break;
	case XK_KP_End:       keycode = kbKPEnd;      break;
	case XK_Begin:        keycode = kbBegin;      break;
	case XK_KP_Begin:     keycode = kbKPBegin;    break;
	case XK_Select:       keycode = kbSelect;     break;
	case XK_Print:        keycode = kbPrint;      break;
	case XK_Execute:      keycode = kbExecute;    break;
	case XK_Insert:       keycode = kbInsert;     break;
	case XK_KP_Insert:    keycode = kbKPInsert;   break;
	case XK_Undo:         keycode = kbUndo;       break;
	case XK_Redo:         keycode = kbRedo;       break;
	case XK_Menu:         keycode = kbMenu;       break;
	case XK_Find:         keycode = kbFind;       break;
	case XK_Cancel:       keycode = kbCancel;     break;
	case XK_Help:         keycode = kbHelp;       break;
	case XK_Break:        keycode = kbBreak;      break;
	case XK_ISO_Left_Tab: keycode = kbBackTab;    break;
	/* Virtual function keys */
	case XK_F1:           keycode = kbF1;         break;
	case XK_KP_F1:        keycode = kbKPF1;       break;
	case XK_F2:           keycode = kbF2;         break;
	case XK_KP_F2:        keycode = kbKPF2;       break;
	case XK_F3:           keycode = kbF3;         break;
	case XK_KP_F3:        keycode = kbKPF3;       break;
	case XK_F4:           keycode = kbF4;         break;
	case XK_KP_F4:        keycode = kbKPF4;       break;
	case XK_F5:           keycode = kbF5;         break;
	case XK_F6:           keycode = kbF6;         break;
	case XK_F7:           keycode = kbF7;         break;
	case XK_F8:           keycode = kbF8;         break;
	case XK_F9:           keycode = kbF9;         break;
	case XK_F10:          keycode = kbF10;        break;
	case XK_F11:          keycode = kbF11;        break;
	case XK_F12:          keycode = kbF12;        break;
	case XK_F13:          keycode = kbF13;        break;
	case XK_F14:          keycode = kbF14;        break;
	case XK_F15:          keycode = kbF15;        break;
	case XK_F16:          keycode = kbF16;        break;
	case XK_F17:          keycode = kbF17;        break;
	case XK_F18:          keycode = kbF18;        break;
	case XK_F19:          keycode = kbF19;        break;
	case XK_F20:          keycode = kbF20;        break;
	case XK_F21:          keycode = kbF21;        break;
	case XK_F22:          keycode = kbF22;        break;
	case XK_F23:          keycode = kbF23;        break;
	case XK_F24:          keycode = kbF24;        break;
	case XK_F25:          keycode = kbF25;        break;
	case XK_F26:          keycode = kbF26;        break;
	case XK_F27:          keycode = kbF27;        break;
	case XK_F28:          keycode = kbF28;        break;
	case XK_F29:          keycode = kbF29;        break;
	case XK_F30:          keycode = kbF30;        break;
	case XK_space:        keycode = kbSpace;      break;
	default:              keycode = kbNoKey;
	}

	if (( keycode == kbTab || keycode == kbKPTab) && ( ev-> state & ShiftMask))
		keycode = kbBackTab;

	if ( keycode == kbNoKey) {
		KeySym keysym2;
		if ( keysym <= 0x0000007f && !isalpha(keysym & 0x000000ff))
			keycode = keysym & 0x000000ff;
		else if (
			( ev-> state & ControlMask) && keysym > 0x0000007f
			&& (keysym2 = XLookupKeysym( ev, 0)) <= 0x0000007f
			&& isalpha(keysym2 & 0x0000007f)
		)
			keycode = toupper(keysym2 & 0x0000007f) - '@';
		else if ( character != 0)
			keycode = character;
		else if ( keysym < 0xFD00)
			keycode = keysym & 0x000000ff;
	}

	if (( keycode & kbCodeCharMask) == kbCodeCharMask)
		keycode |= (keycode >> 8) & 0xFF;

	return keycode;
}

static void
handle_key_event( Handle self, XKeyEvent *ev, Event *e, KeySym * sym, Bool release)
{
	/* if e-> cmd is unset on exit, the event will not be passed to the system-independent part */
	char str_buf[ 256];
	KeySym keysym;
	U32 keycode;
	int str_len;

	if (
		!release &&
		guts.use_xim &&
		P_APPLICATION-> wantUnicodeInput &&
		(ev->state & (ShiftMask|ControlMask|Mod1Mask)) == 0
	) {
		if ( prima_xim_handle_key_press(self, ev, e, sym))
			return;
	}

	str_len = XLookupString( ev, str_buf, 256, &keysym, NULL);
	*sym = keysym;
	Edebug( "event: keysym: %08lx/%08lx 0: %08lx, 1: %08lx, 2: %08lx, 3: %08lx\n", keysym,
		KeySymToUcs4( keysym),
		XLookupKeysym(ev,0),
		XLookupKeysym(ev,1),
		XLookupKeysym(ev,2),
		XLookupKeysym(ev,3)
	);
	keycode = prima_keysym_to_keycode( keysym, ev, ( str_len == 1 ) ? str_buf[0] : 0 );

	e-> cmd = release ? cmKeyUp : cmKeyDown;
	e-> key. key = keycode & kbCodeMask;
	if ( !e-> key. key) e-> key. key = kbNoKey;
	e-> key. mod = keycode & kbModMask;
	e-> key. repeat = 1;
	/* ShiftMask LockMask ControlMask Mod1Mask Mod2Mask Mod3Mask Mod4Mask Mod5Mask */
	if ( ev-> state & ShiftMask)    e-> key. mod |= kmShift;
	if ( ev-> state & ControlMask)  e-> key. mod |= kmCtrl;
	if ( ev-> state & Mod1Mask)     e-> key. mod |= kmAlt;
	if ( P_APPLICATION-> wantUnicodeInput) {
		e-> key. mod  |= kmUnicode;
		e-> key. code = KeySymToUcs4( keysym);
		if (( ev-> state & ControlMask) && isalpha( e-> key. code))
			e-> key. code = toupper( e-> key. code) - '@';
	} else {
		e-> key. code = keycode & kbCharMask;
	}

	/* do not generate event */
	if ( !e->key.code && keycode == kbNoKey )
		return;

	/* unicode hex treatment */
	switch(e->key.key) {
	case kbShiftL:
	case kbShiftR:
	case kbCtrlL:
	case kbCtrlR:
		if (( e->key.mod & kmAlt) == 0) {
			int flag = ( e->key.key == kbShiftL || e->key.key == kbShiftR) ?
				UNICODE_HEX_SHIFT : UNICODE_HEX_CTRL;
			if ( e-> key.cmd == cmKeyDown ) {
				if (( guts.unicode_hex_input_flags & UNICODE_HEX_WANT_MASK) == UNICODE_HEX_ENTER_WANTED)
					goto _default;
				guts.unicode_hex_input_flags |= flag;
			} else if ( guts.unicode_hex_input_flags ) {
				guts.unicode_hex_input_flags &= ~flag;
				switch ( guts.unicode_hex_input_flags) {
				case UNICODE_HEX_RELEASE_WANTED:
					feed_unicode_character(self, e);
					break;
				case UNICODE_HEX_METHOD_WANTED:
					guts.unicode_hex_input_flags = UNICODE_HEX_ENTER_WANTED;
					break;
				}
			}
			break;
		}
		goto _default;
	
	case kbNoKey: if (guts.unicode_hex_input_flags) {
		int code = XLookupKeysym( ev, 0);
		switch ( code ) {
		case 'u':
		case 'U':
			if ((guts.unicode_hex_input_flags & UNICODE_HEX_WANT_MASK) == 0) {
				int keys = (guts.unicode_hex_input_flags & UNICODE_HEX_KEYMASK);
				if ( e-> key.cmd == cmKeyDown ) {
					if (keys != (UNICODE_HEX_CTRL|UNICODE_HEX_SHIFT))
						goto _default;
					guts.unicode_hex_input_flags |= UNICODE_HEX_U;
				} else {
					if (keys != (UNICODE_HEX_CTRL|UNICODE_HEX_SHIFT|UNICODE_HEX_U))
						goto _default;
					guts.unicode_hex_input_flags &= ~UNICODE_HEX_U;
					guts.unicode_hex_input_flags |= UNICODE_HEX_METHOD_WANTED;
				}
				break;
			}
			goto _default;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9': case 'a': case 'A':
		case 'b': case 'B': case 'c': case 'C':
		case 'd': case 'D': case 'e': case 'E':
		case 'f': case 'F':
			if ((guts.unicode_hex_input_flags & UNICODE_HEX_WANT_MASK) != 0) {
				int len;
				if ( e-> key.cmd != cmKeyDown ) break;
				if ((guts.unicode_hex_input_flags & UNICODE_HEX_WANT_MASK) == UNICODE_HEX_METHOD_WANTED) {
					guts.unicode_hex_input_flags &= ~UNICODE_HEX_WANT_MASK;
					guts.unicode_hex_input_flags |= UNICODE_HEX_RELEASE_WANTED;
				}
				len = strlen(guts.unicode_hex_input_buffer);
				if ( len >= MAX_UNICODE_HEX_LENGTH )
					goto _default;
				guts.unicode_hex_input_buffer[len++] = tolower(code);
				guts.unicode_hex_input_buffer[len++] = 0;
				break;
			}
			goto _default;
		default:
			goto _default;
		}
		break;
	}
	case kbEnter:
		if ((guts.unicode_hex_input_flags & UNICODE_HEX_WANT_MASK) == UNICODE_HEX_ENTER_WANTED) {
			if ( e-> key.cmd != cmKeyUp ) break;
			feed_unicode_character(self, e);
		}

	_default:
	default:
		guts.unicode_hex_input_flags = 0;
	}

	if ( guts.unicode_hex_input_flags & UNICODE_HEX_WANT_MASK )
		e-> cmd = 0;
	/* printf("%x %s\n", guts.unicode_hex_input_flags, guts.unicode_hex_input_buffer); */
}

static Bool
input_disabled( PDrawableSysData XX, Bool ignore_horizon)
{
	Handle horizon = prima_guts.application;

	if ( guts. message_boxes) return true;
	if ( guts. modal_count > 0 && !ignore_horizon) {
		horizon = C_APPLICATION-> map_focus( prima_guts.application, XX-> self);
		if ( XX-> self == horizon) return !XF_ENABLED(XX);
	}
	while (XX->self && XX-> self != horizon && XX-> self != prima_guts.application) {
		if (!XF_ENABLED(XX)) return true;
		XX = X(PWidget(XX->self)->owner);
	}
	return XX->self && XX-> self != horizon;
}

Bool
prima_no_input( PDrawableSysData XX, Bool ignore_horizon, Bool beep)
{
	if ( input_disabled(XX, ignore_horizon)) {
		if ( beep) {
			apc_beep( mbWarning);
		}
		return true;
	}
	return false;
}

static void
syntetic_mouse_move( void)
{
	XMotionEvent e, last;
	e. root = guts. root;
	last. window = e. window = None;
	while ( 1) {
		Bool ret;
		last = e;
		ret = XQueryPointer( DISP, e.root, &e. root, &e. window, &e.x_root,
					&e.y_root, &e.x, &e.y, &e. state);
		XCHECKPOINT;
		if ( !ret || e. window == None) {
			e. window = last. window;
			break;
		}
		e. root = e. window;
	}
	if ( prima_xw2h( e. window)) {
		e. type        = MotionNotify;
		e. send_event  = true;
		e. display     = DISP;
		e. subwindow   = e. window;
		e. root        = guts. root;
		e. time        = guts. last_time;
		e. same_screen = true;
		e. is_hint     = false;
		XSendEvent( DISP, e. window, false, 0, ( XEvent*) &e);
	}
}

typedef struct _WMSyncData
{
	Point   origin;
	Point   size;
	XWindow above;
	Bool    mapped;
	Bool    allow_cmSize;
} WMSyncData;

static void
wm_sync_data_from_event( Handle self, WMSyncData * wmsd, XConfigureEvent * cev, Bool mapped)
{
	wmsd-> above     = cev-> above;
	wmsd-> size. x   = cev-> width;
	wmsd-> size. y   = cev-> height;

	if ( X(self)-> real_parent) { /* trust no one */
		XWindow dummy;
		XTranslateCoordinates( DISP, X_WINDOW, guts. root,
			0, 0, &cev-> x, &cev-> y, &dummy);
	}
	wmsd-> origin. x  = cev-> x;
	wmsd-> origin. y  = X(X(self)-> owner)-> size. y - wmsd-> size. y - cev-> y;
	wmsd-> mapped  = mapped;
}

static void
open_wm_sync_data( Handle self, WMSyncData * wmsd)
{
	DEFXX;
	wmsd-> size. x = XX-> size. x;
	wmsd-> size. y = XX-> size. y + XX-> menuHeight;
	wmsd-> origin  = PWidget( self)-> pos;
	wmsd-> above   = XX-> above;
	wmsd-> mapped  = XX-> flags. mapped ? true : false;
	wmsd-> allow_cmSize = false;
}

static Bool
process_wm_sync_data( Handle self, WMSyncData * wmsd)
{
	DEFXX;
	Event e;
	Bool size_changed = false;
	Point old_size = XX-> size, old_pos = XX-> origin;

	if ( wmsd-> origin. x != PWidget(self)-> pos. x || wmsd-> origin. y != PWidget(self)-> pos. y) {
		Edebug("event: GOT move to %d %d / %d %d\n", wmsd-> origin.x, wmsd-> origin.y, PWidget(self)->pos. x, PWidget(self)->pos. y);
		bzero( &e, sizeof( Event));
		e. cmd      = cmMove;
		e. gen. P   = XX-> origin = wmsd-> origin;
		e. gen. source = self;
		SELF_MESSAGE(e);
		if ( PObject( self)-> stage == csDead) return false;
	}

	if ( wmsd-> allow_cmSize &&
		( wmsd-> size. x != XX-> size. x || wmsd-> size. y != XX-> size. y + XX-> menuHeight)) {
		XX-> size. x = wmsd-> size. x;
		XX-> size. y = wmsd-> size. y - XX-> menuHeight;
		PWidget( self)-> virtualSize = XX-> size;
		Edebug("event: got size to %d %d\n", XX-> size.x, XX-> size.y);
		prima_send_cmSize( self, old_size);
		if ( PObject( self)-> stage == csDead) return false;
		size_changed = true;
	}

	if ( wmsd-> above != XX-> above) {
		XX-> above = wmsd-> above;
		bzero( &e, sizeof( Event));
		e. cmd = cmZOrderChanged;
		SELF_MESSAGE(e);
		if ( PObject( self)-> stage == csDead) return false;
	}

	if ( size_changed && XX-> flags. want_visible && !guts. net_wm_maximization && !XX-> flags. fullscreen) {
		int qx = guts. displaySize.x * 4 / 5, qy = guts. displaySize.y * 4 / 5;
		bzero( &e, sizeof( Event));
		if ( !XX-> flags. zoomed) {
			if ( XX-> size. x > qx && XX-> size. y > qy) {
				e. cmd = cmWindowState;
				e. gen. i = wsMaximized;
				XX-> zoomRect.left = old_pos.x;
				XX-> zoomRect.bottom = old_pos.y;
				XX-> zoomRect.right = old_size.x;
				XX-> zoomRect.top = old_size.y;
				XX-> flags. zoomed = 1;
			}
		} else {
			if ( old_size.x > XX-> size.x && old_size.y > XX-> size.y) {
				e. cmd = cmWindowState;
				e. gen. i = wsNormal;
				XX-> flags. zoomed = 0;
			} else {
				XX-> zoomRect.left = XX-> origin.x;
				XX-> zoomRect.bottom = XX-> origin.y;
				XX-> zoomRect.right = XX-> size.x;
				XX-> zoomRect.top = XX-> size.y;
			}
		}
		if ( e. cmd) SELF_MESSAGE(e);
		if ( PObject( self)-> stage == csDead) return false;
	}

	if ( !XX-> flags. mapped && wmsd-> mapped) {
		Event f;
		bzero( &e, sizeof( Event));
		bzero( &f, sizeof( Event));
		if ( XX-> type. window && XX-> flags. iconic) {
			f. cmd = cmWindowState;
			f. gen. i = XX-> flags. zoomed ? wsMaximized : wsNormal;
			f. gen. source = self;
			XX-> flags. iconic = 0;
		}
		if ( XX-> flags. withdrawn)
			XX-> flags. withdrawn = 0;
		XX-> flags. mapped = 1;
		e. cmd = cmShow;
		SELF_MESSAGE(e);
		if ( PObject( self)-> stage == csDead) return false;
		if ( f. cmd) {
			SELF_MESSAGE(f);
			if ( PObject( self)-> stage == csDead) return false;
		}
	} else if ( XX-> flags. mapped && !wmsd-> mapped) {
		Event f;
		bzero( &e, sizeof( Event));
		bzero( &f, sizeof( Event));
		if ( !XX-> flags. iconic && XX-> type. window) {
			f. cmd = cmWindowState;
			f. gen. i = wsMinimized;
			f. gen. source = self;
			XX-> flags. iconic = 1;
		}
		e. cmd = cmHide;
		XX-> flags. mapped = 0;
		SELF_MESSAGE(e);
		if ( PObject( self)-> stage == csDead) return false;
		if ( f. cmd) {
			SELF_MESSAGE(f);
			if ( PObject( self)-> stage == csDead) return false;
		}
	}
	return true;
}

static Bool
wm_event( Handle self, XEvent *xev, PEvent ev)
{

	switch ( xev-> xany. type) {
	case ClientMessage:
		if ( xev-> xclient. message_type == WM_PROTOCOLS) {
			if ((Atom) xev-> xclient. data. l[0] == WM_DELETE_WINDOW) {
				if ( guts. message_boxes) return false;
				if ( self != C_APPLICATION-> map_focus( prima_guts.application, self))
					return false;
				ev-> cmd = cmClose;
				return true;
			} else if ((Atom) xev-> xclient. data. l[0] == WM_TAKE_FOCUS) {
				Handle selectee;
				if ( guts. message_boxes) {
					struct MsgDlg * md = guts. message_boxes;
					while ( md) {
						if ( md-> w) XMapRaised( DISP, md-> w);
						md = md-> next;
					}
					return false;
				}

				selectee = C_APPLICATION-> map_focus( prima_guts.application, self);

				/* under modal window? */

				if ( selectee && selectee != self)
					XMapRaised( DISP, PWidget(selectee)-> handle);

				if ( !guts. currentMenu) {
		       			if ( selectee) {
						int rev;
						XWindow focus = None;
						Handle selectee2 = Widget_get_selectee( selectee);
						if ( selectee2) {
							XGetInputFocus( DISP, &focus, &rev);
							/* protection against openbox who fires WM_TAKE_FOCUS no matter what */
							if ( selectee2 && focus != None && focus == PWidget(selectee2)-> handle)
								return false;
						}
					}
					guts. currentFocusTime = xev-> xclient. data. l[1];
					/* Refuse to take focus unless there are no modal windows above */
					if ( !selectee || selectee == self)
						XSetInputFocus( DISP, X_WINDOW, RevertToParent, xev-> xclient. data. l[1]);
					if ( selectee)
						Widget_selected( selectee, true, true);
					guts. currentFocusTime = CurrentTime;
				}
				return false;
			}
		}
		break;
	case PropertyNotify:
		if (
			xev-> xproperty. atom == NET_WM_STATE &&
			xev-> xproperty. state == PropertyNewValue &&
			!X(self)-> flags. fullscreen &&
			X(self)-> flags. mapped
		) {
			DEFXX;
			ev-> cmd = cmWindowState;
			ev-> gen. source = self;
			if ( prima_wm_net_state_read_maximization( xev-> xproperty. window, NET_WM_STATE)) {
				if ( !XX-> flags. zoomed) {
					ev-> gen. i = wsMaximized;
					XX-> flags. zoomed = 1;
				} else
					ev-> cmd = 0;
			} else {
				if ( XX-> flags. zoomed) {
					ev-> gen. i = wsNormal;
					XX-> flags. zoomed = 0;
				} else if ( XX-> flags. iconic ) {
					ev-> gen. i = wsNormal;
					XX-> flags. iconic = 0;
				} else
					ev-> cmd = 0;
			}
		}
		break;
	}

	return false;
}

static int
compress_configure_events( XEvent *ev)
/*
	This reads as many ConfigureNotify events so these do not
	trash us down. This can happen when the WM allows dynamic
	top-level windows resizing, and large amount of ConfigureNotify
	events are sent as the user resizes the window.

	Xlib has many functions to peek into event queue, but all
	either block or XFlush() implicitly, which isn't a good
	idea - so the code uses XNextEvent/XPutBackEvent calls.
*/
{
	Handle self;
	XEvent * set, *x;
	XWindow win = ev-> xconfigure. window;
	int i, skipped = 0, read_events = 0,
		queued_events = XEventsQueued( DISP, QueuedAlready);

	if (
		( queued_events <= 0) ||
		!( set = malloc( sizeof( XEvent) * queued_events))
	) return 0;

	for ( i = 0, x = set; i < queued_events; i++, x++) {
		XNextEvent( DISP, x);
		read_events++;

		switch ( x-> type) {
		case KeyPress:       case KeyRelease:
		case ButtonPress:    case ButtonRelease:
		case MotionNotify:   case DestroyNotify:
		case ReparentNotify: case PropertyNotify:
		case FocusIn:        case FocusOut:
		case EnterNotify:    case LeaveNotify:
			goto STOP;
		case MapNotify:
		case UnmapNotify:
			self = prima_xw2h( x-> xconfigure. window);
			/* stop only after top-level events */
			if ( self && XT_IS_WINDOW(X(self)) && x-> xconfigure. window == X_WINDOW)
				goto STOP;
			break;
		case ClientMessage:
			if ( x-> xclient. message_type == WM_PROTOCOLS &&
				(Atom) x-> xclient. data. l[0] == WM_DELETE_WINDOW)
				goto STOP;
			break;
		case ConfigureNotify:
			/* check if it's Widget event, which is anyway not passed to perl */
			self = prima_xw2h( x-> xconfigure. window);
			if ( self && XT_IS_WINDOW(X(self)) && x-> xconfigure. window == X_WINDOW) {
				/* other top-level's configure? */
				if ( x-> xconfigure. window != win) goto STOP;
				/* or, discard the previous ConfigureNotify */
				ev-> type = 0;
				ev = x;
				skipped++;
			}
			break;
		}
	}
STOP:;

	for ( i = read_events - 1, x = set + read_events - 1; i >= 0; i--, x--)
		if ( x-> type != 0)
			XPutBackEvent( DISP, x);
	free( set);
	return skipped;
}

static char * xevdefs[] = { "0", "1"
,"KeyPress" ,"KeyRelease" ,"ButtonPress" ,"ButtonRelease" ,"MotionNotify" ,"EnterNotify"
,"LeaveNotify" ,"FocusIn" ,"FocusOut" ,"KeymapNotify" ,"Expose" ,"GraphicsExpose"
,"NoExpose" ,"VisibilityNotify" ,"CreateNotify" ,"DestroyNotify" ,"UnmapNotify"
,"MapNotify" ,"MapRequest" ,"ReparentNotify" ,"ConfigureNotify" ,"ConfigureRequest"
,"GravityNotify" ,"ResizeRequest" ,"CirculateNotify" ,"CirculateRequest" ,"PropertyNotify"
,"SelectionClear" ,"SelectionRequest" ,"SelectionNotify" ,"ColormapNotify" ,"ClientMessage"
,"MappingNotify"};

void
prima_handle_event( XEvent *ev, XEvent *next_event)
{
	XWindow win;
	Handle self;
	Bool was_sent;
	Event e, secondary;
	PDrawableSysData selfxx;
	XButtonEvent *bev;
	KeySym keysym = 0;
	int cmd;

	XCHECKPOINT;
	if ( ev-> type == guts. shared_image_completion_event) {
		prima_ximage_event( ev);
		return;
	}

	if ( guts. message_boxes) {
		struct MsgDlg * md = guts. message_boxes;
		XWindow win = ev-> xany. window;
		while ( md) {
			if ( md-> w == win) {
				prima_msgdlg_event( ev, md);
				return;
			}
			md = md-> next;
		}
	}

	if ( prima_guts.app_is_dead)
		return;

	bzero( &e, sizeof( e));
	bzero( &secondary, sizeof( secondary));

	/* Get a window, including special cases */
	switch ( ev-> type) {
	case ConfigureNotify:
	case -ConfigureNotify:
		win = ev-> xconfigure. window;
		break;
	case ReparentNotify:
		win = ev-> xreparent. window;
		break;
	case MapNotify:
		win = ev-> xmap. window;
		break;
	case UnmapNotify:
		win = ev-> xunmap. window;
		break;
	case DestroyNotify:
		if ( guts. clipboard_xfers &&
			hash_fetch( guts. clipboard_xfers, &ev-> xdestroywindow. window, sizeof( XWindow))) {
			prima_handle_selection_event( ev, ev-> xproperty. window, NULL_HANDLE);
			return;
		}
		goto DEFAULT;
	case PropertyNotify:
		guts. last_time = ev-> xproperty. time;
		if ( guts. clipboard_xfers) {
			Handle value;
			ClipboardXferKey key;
			CLIPBOARD_XFER_KEY( key, ev-> xproperty. window, ev-> xproperty. atom);
			value = ( Handle) hash_fetch( guts. clipboard_xfers, key, sizeof( key));
			if ( value) {
				prima_handle_selection_event( ev, ev-> xproperty. window, value);
				return;
			}
		}
		goto DEFAULT;
	DEFAULT:
	default:
		win = ev-> xany. window;
	}

	/* possibly skip this event */
	if ( next_event) {
		if (next_event-> type == ev-> type
			&& ev-> type == MotionNotify
			&& win == next_event-> xany. window) {
			guts. skipped_events++;
			return;
		} else if ( ev-> type == KeyRelease
			&& next_event-> type == KeyPress
			&& ev-> xkey. time == next_event-> xkey. time
			&& ev-> xkey. display == next_event-> xkey. display
			&& ev-> xkey. window == next_event-> xkey. window
			&& ev-> xkey. root == next_event-> xkey. root
			&& ev-> xkey. subwindow == next_event-> xkey. subwindow
			&& ev-> xkey. x == next_event-> xkey. x
			&& ev-> xkey. y == next_event-> xkey. y
			&& ev-> xkey. state == next_event-> xkey. state
			&& ev-> xkey. keycode == next_event-> xkey. keycode) {
			guts. skipped_events++;
			return;
		}
	}

	if ( win == guts. root && guts. grab_redirect)
		win = guts. grab_redirect;

	self = prima_xw2h( win);
	if ( ev-> type > 0)
		Edebug("event: %d:%s of %s\n", ev-> type,
				((ev-> type >= LASTEvent) ? "?" : xevdefs[ev-> type]),
				self ? PWidget(self)-> name : "(NULL)");

	if (!self)
		return;
	if ( XT_IS_MENU(X(self))) {
		prima_handle_menu_event( ev, win, self);
		return;
	}
	e. gen. source = self;
	secondary. gen. source = self;
	XX = X(self);

	was_sent = ev-> xany. send_event;

	switch ( ev-> type) {
	case KeyPress:
		if (guts.xdnds_widget) {
			char str_buf[ 256];
			KeySym keysym = 0;
			XLookupString( &ev-> xkey, str_buf, 256, &keysym, NULL);
			if ( keysym == XK_Escape ) {
				guts. xdnds_escape_key = true;
				return;
			}
		}
	case KeyRelease: {
		guts. last_time = ev-> xkey. time;
		if ( !ev-> xkey. send_event && self != guts. focused && guts. focused) {
			/* bypass pointer-driven input */
			Handle newself = self;
			while ( PComponent(newself)-> owner && newself != guts. focused)
				newself = PComponent(newself)-> owner;
			if ( newself == guts. focused) XX = X(self = newself);
		}
		if (prima_no_input(XX, false, ev-> type == KeyPress)) return;
		handle_key_event( self, &ev-> xkey, &e, &keysym, ev-> type == KeyRelease);
		break;
	}
	case ButtonPress: {
		guts. last_time = ev-> xbutton. time;
		if ( guts. currentMenu) prima_end_menu();
		if (prima_no_input(XX, false, true)) return;
		if ( guts. grab_widget != NULL_HANDLE && self != guts. grab_widget) {
			XWindow rx;
			XTranslateCoordinates( DISP, XX-> client, PWidget(guts. grab_widget)-> handle,
				ev-> xbutton.x, ev-> xbutton.y,
				&ev-> xbutton.x, &ev-> xbutton.y, &rx);
			self = guts. grab_widget;
			XX = X(self);
		}
		bev = &ev-> xbutton;
		e. cmd = cmMouseDown;
	ButtonEvent:
		bev-> x -= XX-> origin. x - XX-> ackOrigin. x;
		bev-> y += XX-> origin. y - XX-> ackOrigin. y;
		if ( ev-> xany. window == guts. root && guts. grab_redirect) {
			bev-> x -= guts. grab_translate_mouse. x;
			bev-> y -= guts. grab_translate_mouse. y;
		}
		switch (bev-> button) {
		case Button1:
			e. pos. button = mb1;
			break;
		case Button2:
			e. pos. button = mb2;
			break;
		case Button3:
			e. pos. button = mb3;
			break;
		case Button4:
			e. pos. button = mb4;
			break;
		case Button5:
			e. pos. button = mb5;
			break;
		case Button6:
			e. pos. button = mb6;
			break;
		case Button7:
			e. pos. button = mb7;
			break;
		case Button8:
			e. pos. button = mb8;
			break;
		case Button9:
			e. pos. button = mb9;
			break;
		case Button10:
			e. pos. button = mb10;
			break;
		case Button11:
			e. pos. button = mb11;
			break;
		case Button12:
			e. pos. button = mb12;
			break;
		case Button13:
			e. pos. button = mb13;
			break;
		case Button14:
			e. pos. button = mb14;
			break;
		case Button15:
			e. pos. button = mb15;
			break;
		case Button16:
			e. pos. button = mb16;
			break;
		default:
			return;
		}
		e. pos. where. x = bev-> x;
		e. pos. where. y = XX-> size. y - bev-> y - 1;
		if ( bev-> state & ShiftMask)     e.pos.mod |= kmShift;
		if ( bev-> state & ControlMask)   e.pos.mod |= kmCtrl;
		if ( bev-> state & Mod1Mask)      e.pos.mod |= kmAlt;
		if ( bev-> state & Button1Mask)   e.pos.mod |= mb1;
		if ( bev-> state & Button2Mask)   e.pos.mod |= mb2;
		if ( bev-> state & Button3Mask)   e.pos.mod |= mb3;
		if ( bev-> state & Button4Mask)   e.pos.mod |= mb4;
		if ( bev-> state & Button5Mask)   e.pos.mod |= mb5;
		if ( bev-> state & Button6Mask)   e.pos.mod |= mb6;
		if ( bev-> state & Button7Mask)   e.pos.mod |= mb7;

		if ( e. cmd == cmMouseDown &&
			guts.last_button_event.type == cmMouseUp &&
			bev-> window == guts.last_button_event.window &&
			bev-> button == guts.last_button_event.button &&
			bev-> button != guts. mouse_wheel_up &&
			bev-> button != guts. mouse_wheel_down &&
			bev-> time - guts.last_button_event.time <= guts.click_time_frame
		) {
			e. cmd = cmMouseClick;
			e. pos. nth = ++guts.last_mouseclick_number;
		}

		if (
			(e. cmd == cmMouseDown || e. cmd == cmMouseUp) &&
			(
				( guts. mouse_wheel_up   != 0 && bev-> button == guts. mouse_wheel_up) ||
				( guts. mouse_wheel_down != 0 && bev-> button == guts. mouse_wheel_down)
			)
		) {
			if ( e. cmd == cmMouseDown ) {
				e. cmd = cmMouseWheel;
				e. pos. button = bev-> button == guts. mouse_wheel_up ? WHEEL_DELTA : -WHEEL_DELTA;
			} else {
				e. cmd = 0;
			}
		} else if ( e.cmd == cmMouseUp &&
			guts.last_button_event.type == cmMouseDown &&
			bev-> window == guts.last_button_event.window &&
			bev-> button == guts.last_button_event.button &&
			bev-> time - guts.last_button_event.time <= guts.click_time_frame) {
			secondary. cmd = cmMouseClick;
			secondary. pos. where. x = e. pos. where. x;
			secondary. pos. where. y = e. pos. where. y;
			secondary. pos. mod = e. pos. mod;
			secondary. pos. button = e. pos. button;
			memcpy( &guts.last_click, bev, sizeof(guts.last_click));
			guts. last_mouseclick_number = 1;
			if ( e. pos. button == mbRight) {
				Event ev;
				bzero( &ev, sizeof(ev));
				ev. cmd = cmPopup;
				ev. gen. B = true;
				ev. gen. P. x = e. pos. where. x;
				ev. gen. P. y = e. pos. where. y;
				apc_message( self, &ev, true);
				if ( PObject( self)-> stage == csDead) return;
			}
		}
		memcpy( &guts.last_button_event, bev, sizeof(*bev));
		guts. last_button_event.type = e.cmd;

		if ( e. cmd == cmMouseDown) {
			Handle x = prima_find_root_parent(self);
			Handle f = prima_find_root_parent(guts. focused ? guts. focused : prima_guts.application);
			if ( XX-> flags. first_click) {
				if ( ! is_opt( optSelectable)) {
					if ( x && x != f && X(x)-> type. window)
						XSetInputFocus( DISP, PWidget(x)-> handle, RevertToParent, bev-> time);
				}
			} else {
				if ( x != f) {
					e. cmd = 0;
					if (P_APPLICATION-> hintUnder == self)
						CWidget(self)-> set_hintVisible( self, 0);
					if (( PWidget(self)-> options. optSelectable) && ( PWidget(self)-> selectingButtons & e. pos. button))
						apc_widget_set_focused( self);
				}
			}
		}
		break;
	}
	case ButtonRelease: {
		guts. last_time = ev-> xbutton. time;
		if (prima_no_input(XX, false, false)) return;
		if ( guts. grab_widget != NULL_HANDLE && self != guts. grab_widget) {
			XWindow rx;
			XTranslateCoordinates( DISP, XX-> client, PWidget(guts. grab_widget)-> handle,
				ev-> xbutton.x, ev-> xbutton.y,
				&ev-> xbutton.x, &ev-> xbutton.y, &rx);
			self = guts. grab_widget;
			XX = X(self);
		}
		bev = &ev-> xbutton;
		e. cmd = cmMouseUp;
		goto ButtonEvent;
	}
	case MotionNotify: {
		guts. last_time = ev-> xmotion. time;
		if ( guts. grab_widget != NULL_HANDLE && self != guts. grab_widget) {
			XWindow rx;
			XTranslateCoordinates( DISP, XX-> client, PWidget(guts. grab_widget)-> handle,
				ev-> xmotion.x, ev-> xmotion.y,
				&ev-> xmotion.x, &ev-> xmotion.y, &rx);
			self = guts. grab_widget;
			XX = X(self);
		}
		ev-> xmotion. x -= XX-> origin. x - XX-> ackOrigin. x;
		ev-> xmotion. y += XX-> origin. y - XX-> ackOrigin. y;
		e. cmd = cmMouseMove;
		if ( ev-> xany. window == guts. root && guts. grab_redirect) {
			ev-> xmotion. x -= guts. grab_translate_mouse. x;
			ev-> xmotion. y -= guts. grab_translate_mouse. y;
		}
		e. pos. where. x = ev-> xmotion. x;
		e. pos. where. y = XX-> size. y - ev-> xmotion. y - 1;
		if ( ev-> xmotion. state & ShiftMask)     e.pos.mod |= kmShift;
		if ( ev-> xmotion. state & ControlMask)   e.pos.mod |= kmCtrl;
		if ( ev-> xmotion. state & Mod1Mask)      e.pos.mod |= kmAlt;
		if ( ev-> xmotion. state & Button1Mask)   e.pos.mod |= mb1;
		if ( ev-> xmotion. state & Button2Mask)   e.pos.mod |= mb2;
		if ( ev-> xmotion. state & Button3Mask)   e.pos.mod |= mb3;
		if ( ev-> xmotion. state & Button4Mask)   e.pos.mod |= mb4;
		if ( ev-> xmotion. state & Button5Mask)   e.pos.mod |= mb5;
		if ( ev-> xmotion. state & Button6Mask)   e.pos.mod |= mb6;
		if ( ev-> xmotion. state & Button7Mask)   e.pos.mod |= mb7;
		break;
	}
	case EnterNotify: {
		if (( guts. pointer_invisible_count == 0) && XX-> flags. pointer_obscured) {
			XX-> flags. pointer_obscured = 0;
			XDefineCursor( DISP, XX-> udrawable, prima_get_cursor(self));
		} else if (( guts. pointer_invisible_count < 0) && !XX-> flags. pointer_obscured) {
			XX-> flags. pointer_obscured = 1;
			XDefineCursor( DISP, XX-> udrawable, guts. null_pointer);
		}
		guts. last_time = ev-> xcrossing. time;
		e. cmd = cmMouseEnter;
	CrossingEvent:
		if ( ev-> xcrossing. subwindow != None) return;
		e. pos. where. x = ev-> xcrossing. x;
		e. pos. where. y = XX-> size. y - ev-> xcrossing. y - 1;
		break;
	}
	case LeaveNotify: {
		guts. last_time = ev-> xcrossing. time;
		e. cmd = cmMouseLeave;
		goto CrossingEvent;
	}
	case FocusIn: {
		Handle frame = self;
		switch ( ev-> xfocus. detail) {
		case NotifyVirtual:
		case NotifyPointer:
		case NotifyPointerRoot:
		case NotifyDetailNone:
			return;
		case NotifyNonlinearVirtual:
			if (!XT_IS_WINDOW(XX)) return;
			break;
		}

		if ( guts. message_boxes) {
			struct MsgDlg * md = guts. message_boxes;
			while ( md) {
				XSetInputFocus( DISP, md-> w, RevertToNone, CurrentTime);
				md = md-> next;
			}
			return;
		}

		syntetic_mouse_move(); /* XXX - simulated MouseMove event for compatibility reasons */

		if (!XT_IS_WINDOW(XX))
			frame = C_APPLICATION-> top_frame( prima_guts.application, self);
		if ( C_APPLICATION-> map_focus( prima_guts.application, frame) != frame) {
			C_APPLICATION-> popup_modal( prima_guts.application);
			break;
		}

		if ( XT_IS_WINDOW(XX)) {
			e. cmd = cmActivate;
			SELF_MESSAGE(e);
			if (( PObject( self)-> stage == csDead) ||
				( ev-> xfocus. detail == NotifyNonlinearVirtual)) return;
		}

		if ( guts. focused) prima_no_cursor( guts. focused);
		guts. focused = self;
		prima_update_cursor( guts. focused);
		e. cmd = cmReceiveFocus;
		if ( guts.use_xim )
			prima_xim_focus_in(self);
		break;
	}
	case FocusOut: {
		switch ( ev-> xfocus. detail) {
		case NotifyVirtual:
		case NotifyPointer:
		case NotifyPointerRoot:
		case NotifyDetailNone:
			return;
		case NotifyNonlinearVirtual:
			if (!XT_IS_WINDOW(XX)) return;
			break;
		}

		if ( XT_IS_WINDOW(XX)) {
			e. cmd = cmDeactivate;
			SELF_MESSAGE(e);
			if (( PObject( self)-> stage == csDead) ||
				( ev-> xfocus. detail == NotifyNonlinearVirtual)) return;
		}

		if ( guts. focused) prima_no_cursor( guts. focused);
		if ( self == guts. focused) guts. focused = NULL_HANDLE;
		e. cmd = cmReleaseFocus;
		if ( guts.use_xim )
			prima_xim_focus_out();
		break;
	}
	case KeymapNotify: {
		break;
	}
	case GraphicsExpose:
	case Expose: {
		XRectangle r;
		if ( !was_sent) {
			r. x = ev-> xexpose. x;
			r. y = ev-> xexpose. y;
			r. width = ev-> xexpose. width;
			r. height = ev-> xexpose. height;
			if ( !XX-> invalid_region)
				XX-> invalid_region = XCreateRegion();
			XUnionRectWithRegion( &r, XX-> invalid_region, XX-> invalid_region);
		}

		if ( ev-> xexpose. count == 0 && !XX-> flags. paint_pending) {
			TAILQ_INSERT_TAIL( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = true;
		}

		process_transparents(self);
		return;
	}
	case NoExpose: {
		break;
	}
	case VisibilityNotify: {
		XX-> flags. exposed = ( ev-> xvisibility. state != VisibilityFullyObscured);
		break;
	}
	case CreateNotify: {
		break;
	}
	case DestroyNotify: {
		break;
	}
	case UnmapNotify: {
		if ( XT_IS_WINDOW(XX) && win != XX-> client) {
			WMSyncData wmsd;
			open_wm_sync_data( self, &wmsd);
			wmsd. mapped = false;
			process_wm_sync_data( self, &wmsd);
		}
		if ( !XT_IS_WINDOW(XX) || win != XX-> client) {
			XX-> flags. mapped = false;
		}
		break;
	}
	case MapNotify: {
		if ( XT_IS_WINDOW(XX) && win != XX-> client) {
			WMSyncData wmsd;
			open_wm_sync_data( self, &wmsd);
			wmsd. mapped = true;
			process_wm_sync_data( self, &wmsd);
		}
		if ( !XT_IS_WINDOW(XX) || win != XX-> client) {
			XX-> flags. mapped = true;
		}
		break;
	}
	case MapRequest: {
		break;
	}

	case ReparentNotify: {
			XWindow p = ev-> xreparent. parent;
			if ( !XX-> type. window) return;
			XX-> real_parent = ( p == guts. root) ? NULL_HANDLE : p;
			if ( XX-> real_parent) {
				XWindow dummy;
				XTranslateCoordinates( DISP, X_WINDOW, XX-> real_parent,
					0, 0, &XX-> decorationSize.x, &XX-> decorationSize.y, &dummy);
			} else
				XX-> decorationSize. x = XX-> decorationSize. y = 0;
		}
		return;

	case -ConfigureNotify:
		if ( XT_IS_WINDOW(XX)) {
			if ( win != XX-> client) {
				int nh;
				WMSyncData wmsd;
				Bool size_changed =
					XX-> ackFrameSize. x != ev-> xconfigure. width ||
					XX-> ackFrameSize. y != ev-> xconfigure. height;
				wmsd. allow_cmSize = true;
				XX-> ackOrigin. x = ev-> xconfigure. x;
				XX-> ackOrigin. y = X(X(self)-> owner)-> size. y - ev-> xconfigure. height - ev-> xconfigure. y;
				XX-> ackFrameSize. x   = ev-> xconfigure. width;
				XX-> ackFrameSize. y   = ev-> xconfigure. height;
				nh = ev-> xconfigure. height - XX-> menuHeight;
				if ( nh < 1 ) nh = 1;
				XMoveResizeWindow( DISP, XX-> client, 0, XX-> menuHeight, ev-> xconfigure. width, nh);
				if ( PWindow( self)-> menu) {
					if ( size_changed) {
						M(PWindow( self)-> menu)-> paint_pending = true;
						XResizeWindow( DISP, PComponent(PWindow( self)-> menu)-> handle,
							ev-> xconfigure. width, XX-> menuHeight);
					}
					M(PWindow( self)-> menu)-> w-> pos. x = ev-> xconfigure. x;
					M(PWindow( self)-> menu)-> w-> pos. y = ev-> xconfigure. y;
					prima_end_menu();
				}
				wm_sync_data_from_event( self, &wmsd, &ev-> xconfigure, XX-> flags. mapped);
				process_wm_sync_data( self, &wmsd);
			} else {
				XX-> ackSize. x  = ev-> xconfigure. width;
				XX-> ackSize. y  = ev-> xconfigure. height;
			}
		} else {
			XX-> ackSize. x   = ev-> xconfigure. width;
			XX-> ackSize. y   = ev-> xconfigure. height;
			XX-> ackOrigin. x = ev-> xconfigure. x;
			XX-> ackOrigin. y = X(X(self)-> owner)-> size. y - ev-> xconfigure. height - ev-> xconfigure. y;
		}
		return;
	case ConfigureNotify:
		if ( XT_IS_WINDOW(XX)) {
			if ( win != XX-> client) {
				int nh;
				WMSyncData wmsd;
				ConfigureEventPair *n1, *n2;
				Bool match = false, size_changed;

				if ( compress_configure_events( ev)) return;

				size_changed =
					XX-> ackFrameSize. x != ev-> xconfigure. width ||
					XX-> ackFrameSize. y != ev-> xconfigure. height;
				nh = ev-> xconfigure. height - XX-> menuHeight;
				if ( nh < 1 ) nh = 1;
				XMoveResizeWindow( DISP, XX-> client, 0, XX-> menuHeight, ev-> xconfigure. width, nh);

				wmsd. allow_cmSize = true;
				wm_sync_data_from_event( self, &wmsd, &ev-> xconfigure, XX-> flags. mapped);
				XX-> ackOrigin. x = wmsd. origin. x;
				XX-> ackOrigin. y = wmsd. origin. y;
				XX-> ackFrameSize. x = ev-> xconfigure. width;
				XX-> ackFrameSize. y = ev-> xconfigure. height;

				if (( n1 = TAILQ_FIRST( &XX-> configure_pairs))) {
					if ( n1-> w == ev-> xconfigure. width &&
						n1-> h == ev-> xconfigure. height) {
						n1-> match = true;
						wmsd. size. x  = XX-> size.x;
						wmsd. size. y = XX-> size.y + XX-> menuHeight;
						match = true;
					} else if ( n1-> match && (n2 = TAILQ_NEXT( n1, link)) &&
						n2-> w == ev-> xconfigure. width &&
						n2-> h == ev-> xconfigure. height) {
						n2-> match = true;
						wmsd. size. x  = XX-> size.x;
						wmsd. size. y = XX-> size.y + XX-> menuHeight;
						TAILQ_REMOVE( &XX-> configure_pairs, n1, link);
						free( n1);
						match = true;
					}

					if ( !match) {
						while ( n1 != NULL) {
							n2 = TAILQ_NEXT(n1, link);
							free( n1);
							n1 = n2;
						}
						TAILQ_INIT( &XX-> configure_pairs);
					}
				}

				Edebug("event: configure: %d --> %d %d\n", ev-> xany.serial, ev-> xconfigure. width, ev-> xconfigure. height);
				XX-> flags. configured = 1;
				process_wm_sync_data( self, &wmsd);

				if ( PWindow( self)-> menu) {
					if ( size_changed) {
						XEvent e;
						Handle menu = PWindow( self)-> menu;
						M(PWindow( self)-> menu)-> paint_pending = true;
						XResizeWindow( DISP, PComponent(PWindow( self)-> menu)-> handle,
							ev-> xconfigure. width, XX-> menuHeight);
						e. type = ConfigureNotify;
						e. xconfigure. width  = ev-> xconfigure. width;
						e. xconfigure. height = XX-> menuHeight;
						prima_handle_menu_event( &e, PAbstractMenu(menu)-> handle, menu);
					}
					M(PWindow( self)-> menu)-> w-> pos. x = ev-> xconfigure. x;
					M(PWindow( self)-> menu)-> w-> pos. y = ev-> xconfigure. y;
				}
				prima_end_menu();
			} else {
				XX-> ackSize. x = ev-> xconfigure. width;
				XX-> ackSize. y = ev-> xconfigure. height;
			}
		} else {
			XX-> ackOrigin. x = ev-> xconfigure. x;
			XX-> ackOrigin. y = X(X(self)-> owner)-> ackSize. y - ev-> xconfigure. height - ev-> xconfigure. y;
			XX-> ackSize. x   = ev-> xconfigure. width;
			XX-> ackSize. y   = ev-> xconfigure. height;
			XX-> flags. configured = 1;
		}
		return;
	case ConfigureRequest: {
		break;
	}
	case GravityNotify: {
		break;
	}
	case ResizeRequest: {
		break;
	}
	case CirculateNotify: {
		break;
	}
	case CirculateRequest: {
		break;
	}
	case PropertyNotify: {
		wm_event( self, ev, &e);
		break;
	}
	case SelectionClear:
	case SelectionRequest: {
		prima_handle_selection_event( ev, win, self);
		break;
	}
	case SelectionNotify: {
		guts. last_time = ev-> xselection. time;
		break;
	}
	case ColormapNotify: {
		break;
	}
	case ClientMessage: {
		if ( ev-> xclient. message_type == CREATE_EVENT) {
			e. cmd = cmSetup;
		} else {
			wm_event( self, ev, &e);
			prima_handle_dnd_event( self, ev);
		}
		break;
	}
	case MappingNotify: {
		XRefreshKeyboardMapping( &ev-> xmapping);
		break;
	}
	}

	if ( e. cmd) {
		guts. handled_events++;
		cmd = e. cmd;
		SELF_MESSAGE(e);
		if ( PObject( self)-> stage == csDead ) return;
		if ( e. cmd) {
			switch ( cmd) {
			case cmClose:
				if ( XX-> type. window && PWindow(self)-> modal)
					CWindow(self)-> cancel( self);
				else
					Object_destroy( self);
				break;
			case cmKeyDown:
				if ( prima_handle_menu_shortcuts( self, ev, keysym) < 0) return;
				break;
			}
		}
		if ( secondary. cmd) {
			SELF_MESSAGE(secondary);
			if ( PObject( self)-> stage == csDead) return;
		}
	} else {
		/* Unhandled event, do nothing */
		guts. unhandled_events++;
	}
}

#define DEAD_BEEF 0xDEADBEEF

static int
copy_events( Handle self, PList events, WMSyncData * w, int eventType)
{
	int ret = 0;
	int queued_events = XEventsQueued( DISP, QueuedAlready);
	if ( queued_events <= 0) return 0;
	while ( queued_events--) {
		XEvent * x = malloc( sizeof( XEvent));
		if ( !x) {
			list_delete_all( events, true);
			plist_destroy( events);
			return -1;
		}
		XNextEvent( DISP, x);

		switch ( x-> type) {
		case ReparentNotify:
			if ( X(self)-> type. window && ( x-> xreparent. window == PWidget(self)-> handle)) {
				X(self)-> real_parent = ( x-> xreparent. parent == guts. root) ?
					NULL_HANDLE : x-> xreparent. parent;
				x-> type = DEAD_BEEF;
				if ( X(self)-> real_parent) {
					XWindow dummy;
					XTranslateCoordinates( DISP, X_WINDOW, X(self)-> real_parent,
						0, 0, &X(self)-> decorationSize.x, &X(self)-> decorationSize.y, &dummy);
				} else
					X(self)-> decorationSize. x = X(self)-> decorationSize. y = 0;
				}
			break;
		}

		{
			Bool ok = false;
			switch ( x-> type) {
			case ConfigureNotify:
				if ( x-> xconfigure. window == PWidget(self)-> handle) {
					wm_sync_data_from_event( self, w, &x-> xconfigure, w-> mapped);
					Edebug("event: configure copy %d %d\n", x-> xconfigure. width, x-> xconfigure. height);
					ok = true;
				}
				break;
			case UnmapNotify:
				if ( x-> xmap. window == PWidget(self)-> handle) {
					w-> mapped = false;
					ok = true;
				}
				break;
			case MapNotify:
				if ( x-> xmap. window == PWidget(self)-> handle) {
					w-> mapped = true;
					ok = true;
				}
				break;
			}
			if ( ok) {
				if ( x-> type == eventType) ret++;
				x-> type = -x-> type;
			}
		}
		if ( x-> type != DEAD_BEEF)
			list_add( events, ( Handle) x);
		else
			free( x);
	}
	return ret;
}

void
prima_wm_sync( Handle self, int eventType)
{
	int r;
	long diff, delay, evx;
	fd_set zero_r, zero_w, read;
	struct timeval start_time, timeout;
	PList events;
	WMSyncData wmsd;

	open_wm_sync_data( self, &wmsd);

	Edebug("event: enter syncer for %d. current size: %d %d\n", eventType, X(self)-> size.x, X(self)-> size.y);
	gettimeofday( &start_time, NULL);

	/* browse & copy queued events */
	evx = XEventsQueued( DISP, QueuedAlready);
	if ( !( events = plist_create( evx + 32, 32)))
		return;
	r = copy_events( self, events, &wmsd, eventType);
	if ( r < 0) return;
	Edebug("event: copied %ld events %s\n", evx, r ? "GOT CONF!" : "");

	/* measuring round-trip time */
	XSync( DISP, false);
	gettimeofday( &timeout, NULL);
	delay = 2 * (( timeout. tv_sec - start_time. tv_sec) * 1000 +
					( timeout. tv_usec - start_time. tv_usec) / 1000) + guts. wm_event_timeout;
	Edebug("event: sync took %ld.%03ld sec\n", timeout. tv_sec - start_time. tv_sec, (timeout. tv_usec - start_time. tv_usec) / 1000);
	if ( guts. is_xwayland) delay *= 2; /* because of extra roundtrip between xwayland and wayland */

	/* got response already? happens if no wm present or  */
	/* sometimes if wm is local to server */
	evx = XEventsQueued( DISP, QueuedAlready);
	r = copy_events( self, events, &wmsd, eventType);
	if ( r < 0) return;
	Edebug("event: pass 1, copied %ld events %s\n", evx, r ? "GOT CONF!" : "");
	if ( delay < 50) delay = 50; /* wait 50 ms just in case */
	/* waiting for ConfigureNotify or timeout */
	Edebug("event: enter cycle, size: %d %d\n", wmsd.size.x, wmsd.size.y);
	start_time = timeout;
	while ( 1) {
		gettimeofday( &timeout, NULL);
		diff = ( timeout. tv_sec - start_time. tv_sec) * 1000 +
				( timeout. tv_usec - start_time. tv_usec) / 1000;
		if ( delay <= diff)
			break;
		timeout. tv_sec  = ( delay - diff) / 1000;
		timeout. tv_usec = (( delay - diff) % 1000) * 1000;
		Edebug("event: want timeout:%g\n", (double)( delay - diff) / 1000);
		FD_ZERO( &zero_r);
		FD_ZERO( &zero_w);
		FD_ZERO( &read);
		FD_SET( guts.connection, &read);
		r = select( guts.connection+1, &read, &zero_r, &zero_w, &timeout);
		if ( r < 0) {
			warn("server connection error");
			return;
		}
		if ( r == 0) {
			Edebug("event: timeout\n");
			break;
		}
		if (( evx = XEventsQueued( DISP, QueuedAfterFlush)) <= 0) {
			/* just like tcl/perl tk do, to avoid an infinite loop */
			RETSIGTYPE oldHandler = signal( SIGPIPE, SIG_IGN);
			XNoOp( DISP);
			XFlush( DISP);
			(void) signal( SIGPIPE, oldHandler);
		}

		/* copying new events */
		r = copy_events( self, events, &wmsd, eventType);
		if ( r < 0) return;
		Edebug("event: copied %ld events %s\n", evx, r ? "GOT CONF!" : "");
		if ( r > 0) break; /* has come ConfigureNotify */
	}
	Edebug("event:exit cycle\n");

	/* put events back */
	Edebug("event: put back %d events\n", events-> count);
	for ( r = events-> count - 1; r >= 0; r--) {
		XPutBackEvent( DISP, ( XEvent*) events-> items[ r]);
		free(( void*) events-> items[ r]);
	}
	plist_destroy( events);
	evx = XEventsQueued( DISP, QueuedAlready);

	Edebug("event: exit syncer, size: %d %d\n", wmsd.size.x, wmsd.size.y);
	process_wm_sync_data( self, &wmsd);
	X(self)-> flags. configured = 1;
}


static Bool
purge_invalid_watchers( Handle self, void *dummy)
{
	((PFile)self)->self->is_active(self,true);
	return false;
}

static int
perform_pending_paints( void)
{
	PDrawableSysData selfxx;
	int i, events = 0;
	List list;

	if ( !prima_guts.application ) return 0;

	list_create(&list, 256, 1024);
	for ( XX = TAILQ_FIRST( &guts.paintq); XX != NULL; ) {
		PDrawableSysData next = TAILQ_NEXT( XX, paintq_link);
		if ( XX-> flags. paint_pending && (guts. appLock == 0) &&
			(PWidget( XX->self)-> stage == csNormal)) {
			TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = false;
			list_add( &list, (Handle) XX->self);
			list_add( &list, (Handle) XX);
			protect_object(XX->self);
		}
 		XX = next;
        }

	for ( i = 0; i < list.count; i+=2) {
		Handle self;

		self = list_at(&list, i);
		if ( PWidget( self)-> stage != csNormal)
			goto NEXT;

		selfxx = (PDrawableSysData) list_at(&list, i+1);
		if ( XX-> flags. paint_pending) {
			TAILQ_REMOVE( &guts.paintq, XX, paintq_link);
			XX-> flags. paint_pending = false;
		}
		prima_simple_message( self, cmPaint, false);
		events++;

		if ( (PWidget( self)-> stage != csNormal)) goto NEXT;

		/* handle the case where this widget is locked */
		if (XX->invalid_region) {
			XDestroyRegion(XX->invalid_region);
			XX->invalid_region = NULL;
		}

	NEXT:
		unprotect_object(self);
	}
	list_destroy(&list);

	return events;
}

static int
send_pending_events( void)
{
	PendingEvent *pe, *next;
	int stage, events = 0;

	if ( !prima_guts.application ) return 0;

	for ( pe = TAILQ_FIRST( &guts.peventq); pe != NULL; ) {
		next  = TAILQ_NEXT( pe, peventq_link);
		stage = PComponent( pe->recipient)-> stage;
		if ( stage != csConstructing) { /* or not yet there */
			TAILQ_REMOVE( &guts.peventq, pe, peventq_link);
			unprotect_object( pe-> recipient );
		}
		if ( stage == csNormal) {
			apc_message( pe-> recipient, &pe-> event, false);
			events++;
		}
		if ( stage != csConstructing) {
			free( pe);
		}
		pe = next;
	}

	return events;
}

static int
send_queued_x_events(int careOfApplication)
{
	int events = 0, queued_events;
	XEvent ev, next_event;
	struct timeval t1, t2;
	gettimeofday( &t1, NULL );

	if ( !prima_guts.application && careOfApplication ) return 0;

	if (( queued_events = XEventsQueued( DISP, QueuedAlready)) <= 0)
		return 0;

	XNextEvent( DISP, &ev);
	XCHECKPOINT;
	if ( guts.use_xim && XFilterEvent(&ev, None))
		ev.type = 0;
	queued_events--;

	while ( queued_events > 0) {
		if (!prima_guts.application && careOfApplication) return false;
		if ( events % 100 ) {
			gettimeofday( &t2, NULL );
			if (( (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec ) > 10000)
				break; /* 10 ms is a good slice */
		}
		XNextEvent( DISP, &next_event);
		XCHECKPOINT;
		if ( !guts.use_xim || !XFilterEvent(&next_event, None))
			prima_handle_event( &ev, &next_event);
		events++;
		queued_events = XEventsQueued( DISP, QueuedAlready);
		memcpy( &ev, &next_event, sizeof( XEvent));
	}
	if (!prima_guts.application && careOfApplication) return events;
	prima_handle_event( &ev, NULL);
	events++;
	return events;
}

static int
process_timers(void)
{
	int events = 0;
	struct timeval t;
	PTimerSysData timer;

	gettimeofday( &t, NULL);
	while (1) {
		if ( !guts. oldest) return 0;

		if ( guts. oldest-> when. tv_sec > t. tv_sec || (
			guts. oldest-> when. tv_sec == t. tv_sec &&
			guts. oldest-> when. tv_usec > t. tv_usec
		))
			return 0;

		timer = guts. oldest;
		apc_timer_start( timer-> who);
		if ( timer-> who == CURSOR_TIMER) {
			prima_cursor_tick();
		} else if ( timer-> who == MENU_TIMER) {
			apc_timer_stop( MENU_TIMER);
			if ( guts. currentMenu) {
				XEvent ev;
				ev. type = MenuTimerMessage;
				prima_handle_menu_event( &ev, M(guts. currentMenu)-> w-> w, guts. currentMenu);
				events++;
			}
		} else if ( timer-> who == MENU_UNFOCUS_TIMER) {
			prima_end_menu();
		} else {
			prima_simple_message( timer-> who, cmTimer, false);
			events++;
		}
	}
	return events;
}

static int
process_file_events(Bool * x_events_pending, struct timeval * t)
{
	int i, r, events = 0, files = 0;
	fd_set read_set, write_set, excpt_set;
	struct {
		Handle file;
		Bool r;
		Bool w;
		Bool e;
	} queue[FD_SETSIZE], *cur;

	if ( x_events_pending )
		*x_events_pending = 0;
	if ( !prima_guts.application ) return 0;

	read_set  = guts.read_set;
	write_set = guts.write_set;
	excpt_set = guts.excpt_set;
	r = select( guts.max_fd+1, &read_set, &write_set, &excpt_set, t);
	if ( r == 0 ) return 0;
	if ( r < 0 ) {
		list_first_that( guts.files, (void*)purge_invalid_watchers, NULL);
		return 0;
	}

	if ( x_events_pending )
		*x_events_pending = FD_ISSET( guts.connection, &read_set);

	for ( i = 0; i < guts. files->count; i++) {
		Bool r = false, w = false, e = false;
		PFile f = (PFile)list_at( guts. files, i);
		if ( FD_ISSET( f->fd, &read_set) && (f->eventMask & feRead))
			r = true;
		if ( FD_ISSET( f->fd, &write_set) && (f->eventMask & feWrite))
			w = true;
		if ( FD_ISSET( f->fd, &excpt_set) && (f->eventMask & feException))
			e = true;
		if ( r || w || e ) {
			if ( files >= FD_SETSIZE - 1 ) break;
			cur = queue + files++;
			cur-> file = (Handle) f;
			cur-> r = r;
			cur-> w = w;
			cur-> e = e;
			protect_object((Handle) f);
		}
	}

	for ( i = 0, cur = queue; i < files; i++, cur++) {
		if ( cur-> r ) {
			prima_simple_message( cur-> file, cmFileRead, false);
			events++;
		}
		if ( cur-> w ) {
			prima_simple_message( cur-> file, cmFileWrite, false);
			events++;
		}
		if ( cur-> e ) {
			prima_simple_message( cur-> file, cmFileException, false);
			events++;
		}
		unprotect_object( cur-> file );
	}

	return events;
}

static void
x_flush(void)
{
	if ( XEventsQueued( DISP, QueuedAfterFlush) <= 0) {
		/* just like tcl/perl tk do, to avoid an infinite loop */
		RETSIGTYPE oldHandler = signal( SIGPIPE, SIG_IGN);
		XNoOp( DISP);
		XFlush( DISP);
		(void) signal( SIGPIPE, oldHandler);
	}
}

static struct timeval *
select_timeout(struct timeval * timeout)
{
	if ( guts. application_stop_signal ) {
		timeout-> tv_sec = 0;
		timeout-> tv_usec = 0;
		return timeout;
	}

	if ( !guts. oldest) return NULL;

	gettimeofday( timeout, NULL);
	if ( guts. oldest-> when. tv_sec < timeout-> tv_sec) {
		timeout-> tv_sec = 0;
		timeout-> tv_usec = 0;
	} else {
		timeout-> tv_sec = guts. oldest-> when. tv_sec - timeout-> tv_sec;
		if ( guts. oldest-> when. tv_usec < timeout-> tv_usec) {
			if ( timeout-> tv_sec == 0) {
				timeout-> tv_sec = 0;
				timeout-> tv_usec = 0;
			} else {
				timeout-> tv_sec--;
				timeout-> tv_usec = 1000000 - (timeout-> tv_usec - guts. oldest-> when. tv_usec);
			}
		} else {
			timeout-> tv_usec = guts. oldest-> when. tv_usec - timeout-> tv_usec;
		}
	}

	return timeout;
}

static int
handle_queued_events( Bool careOfApplication )
{
	int events = 0;

	events += send_queued_x_events(careOfApplication);
	events += perform_pending_paints();
	events += send_pending_events();
	events += process_timers();

	return events;
}

Bool
prima_one_loop_round( int wait, Bool careOfApplication)
{
	struct timeval timeout;
	Bool x_events_pending;
	struct timeval t;

	if ( guts. applicationClose) return false;
	prima_kill_zombies();

	/* handle queued events */
	while ( 1 ) {
		int events;
		events = handle_queued_events(careOfApplication);
		t. tv_sec = 0;
		t. tv_usec = 0;
		events += process_file_events(&x_events_pending, &t);
		if ( x_events_pending && ( prima_guts.application || !careOfApplication) ) {
			x_flush();
			events += handle_queued_events(careOfApplication);
		}
		if ( wait == WAIT_NEVER || ( events > 0 && wait == WAIT_IF_NONE))
			return true;
		if ( !prima_guts.application || guts. applicationClose || guts. application_stop_signal)
			return false;
		if ( events == 0 )
			break;
	}

	/* wait for events */
	prima_simple_message( prima_guts.application, cmIdle, false);
	process_file_events(&x_events_pending, select_timeout(&timeout));
	if ( x_events_pending && ( prima_guts.application || !careOfApplication) )
		x_flush();
	handle_queued_events(careOfApplication);
	exception_dispatch_pending_signals();

	return prima_guts.application != NULL_HANDLE;
}

Bool
prima_simple_message( Handle self, int cmd, Bool is_post)
{
	Event e;

	bzero( &e, sizeof(e));
	e. cmd = cmd;
	e. gen. source = self;
	return apc_message( self, &e, is_post);
}

Bool
apc_message( Handle self, PEvent e, Bool is_post)
{
	PendingEvent *pe;

	if ( is_post) {
		if (!( pe = alloc1(PendingEvent))) return false;
		memcpy( &pe->event, e, sizeof(pe->event));
		pe-> recipient = self;
		protect_object(self);
		TAILQ_INSERT_TAIL( &guts.peventq, pe, peventq_link);
	} else {
		guts. total_events++;
		CComponent(self)->message( self, e);
		if ( PObject( self)-> stage == csDead) return false;
	}
	return true;
}

