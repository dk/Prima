/*-
 * Copyright (c) 1997-2001 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

/***********************************************************/
/*                                                         */
/*  Interaction with Window Managers                       */
/*                                                         */
/***********************************************************/

#include "unix/guts.h"
#include "Application.h"

/* Generic Window Manager Support */


#define DEFWMDATA PWmGenericData wm = guts. wm_data

static void
wm_generic_create_window_hook( Handle self, ApiHandle w)
{
   DEFWMDATA;
   Atom atoms[ 2];
   XWMHints wmhints;

   wmhints. flags = InputHint | StateHint;
   wmhints. input = false;
   wmhints. initial_state = X(self)-> flags. iconic ? IconicState : NormalState;
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
   free( guts. wm_data);
   guts. wm_data = nil;
}

static Bool
wm_generic_translate_event_hook( Handle self, XClientMessageEvent *xev, PEvent ev)
{
   DEFWMDATA;
   Handle selectee;

   selectee = CApplication(application)->map_focus( application, self);

   if ( xev-> type == ClientMessage && xev-> message_type == wm-> protocols) {
      if ((Atom) xev-> data. l[0] == wm-> deleteWindow) {
         if ( guts. message_boxes) return false;
         if ( self != CApplication(application)-> map_focus( application, self))
            return false;
	 ev-> cmd = cmClose;
	 return true;
      } else if ((Atom) xev-> data. l[0] == wm-> takeFocus) {
         if ( guts. message_boxes) {
            struct MsgDlg * md = guts. message_boxes;
            while ( md) {
               XMapRaised( DISP, md-> w);
               md = md-> next;
            }
            return false;
         }
         guts. lastWMFocus = X_WINDOW;
         if ( selectee != self) XMapRaised( DISP, PWidget(selectee)-> handle);
         XSetInputFocus( DISP, X_WINDOW, RevertToParent, CurrentTime);
         Widget_selected( selectee, true, true);
	 return false;
      }
   }
   
   return false;
}

static Bool
prima_wm_generic( void)
{
   DEFWMDATA;

   if ( !( guts. wm_data = wm = malloc( sizeof( WmGenericData))))
      return false;

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
