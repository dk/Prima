#include "unix/guts.h"

/***********************************************************/
/*                                                         */
/*  Prima project                                          */
/*                                                         */
/*  Interaction with Window Managers                       */
/*                                                         */
/*  Copyright (C) 1997,1998 The Protein Laboratory,        */
/*  University of Copenhagen                               */
/*                                                         */
/***********************************************************/

/* Generic Window Manager Support */

typedef struct _WmGenericData {
   Atom deleteWindow;
   Atom protocols;
   Atom takeFocus;
} WmGenericData, *PWmGenericData;

#define DEFWMDATA PWmGenericData wm = guts. wmData

static void
wm_generic_create_window_hook( Handle self, ApiHandle w)
{
   DEFWMDATA;
   Atom atoms[ 2];
   XWMHints wmhints;

   wmhints. flags = InputHint;
   wmhints. input = false;
   XSetWMHints( DISP, X_WINDOW, &wmhints);
   XCHECKPOINT;

   atoms[ 0] = wm-> deleteWindow;
   atoms[ 1] = wm-> takeFocus;
   XSetWMProtocols( DISP, w, atoms, 2);
   XCHECKPOINT;
}

static void
wm_generic_cleanup_hook( void)
{
   free( guts. wmData);
   guts. wmData = nil;
}

static Bool
wm_generic_translate_event_hook( Handle self, XClientMessageEvent *xev, PEvent ev)
{
   DEFWMDATA;

   if ( xev-> type == ClientMessage && xev-> message_type == wm-> protocols) {
      if ((Atom) xev-> data. l[0] == wm-> deleteWindow) {
	 ev-> cmd = cmClose;
	 return true;
      } else if ((Atom) xev-> data. l[0] == wm-> takeFocus) {
	 Handle toSelect = CWidget( self)-> get_selectee( self);
	 XWindow s = toSelect ? PWidget(toSelect)-> handle : PWidget(self)-> handle;
	 fprintf( stderr, "~~~~~~~~~~~~~~ Whoa there! Got take focus!\n");
	 XSetInputFocus( DISP, s, RevertToParent, CurrentTime);
	 XCHECKPOINT;
	 return false;
      }
   }
   
   return false;
}

static Bool
prima_wm_generic( void)
{
   DEFWMDATA;

   guts. wmData = wm = malloc( sizeof( WmGenericData));

   wm-> deleteWindow = XInternAtom( DISP, "WM_DELETE_WINDOW", 1);
   wm-> takeFocus = XInternAtom( DISP, "WM_TAKE_FOCUS", 1);
   wm-> protocols = XInternAtom( DISP, "WM_PROTOCOLS", 1);

   guts. wm_create_window = wm_generic_create_window_hook;
   guts. wm_cleanup = wm_generic_cleanup_hook;
   guts. wm_translate_event = (void*)wm_generic_translate_event_hook;

   return true;
}

/* XXX TWM-specific support */

/* XXX OLWM-specific support */

/* XXX MWM-specific support */

/* XXX FVWM-specific support */

/* XXX FVWM-2-specific support */

/* XXX ICEWM-specific support */

/* XXX WM2-specific support */

/* XXX WMX-specific support */

/* XXX KWM-specific support */

/* XXX OpenStep-specific support */

/* XXX WindowMaker-specific support */

/* XXX other window managers specific support */

/********************************/

#define PRIMA_WM_SUPPORT
#include "unix/guts.h"			/* once again... */

void
prima_wm_init( void)
{
   int i;

   for ( i = 0; i < sizeof(registered_window_managers) / sizeof(prima_wm_hook); i++)
      if ( registered_window_managers[ i]())
	 break;

   /* XXX error checking? */
}
