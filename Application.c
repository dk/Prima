/*-
 * Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
 */

#include "apricot.h"
#include "Timer.h"
#include "Window.h"
#include "Application.h"
#include "Application.inc"

#undef  my
#define inherited CWidget->
#define my  ((( PApplication) self)-> self)->
#define var (( PApplication) self)->

Handle application = nilHandle;

static void Application_HintTimer_Tick( Handle);

extern Bool single_color_notify ( Handle self, Handle child, void * color);
extern Bool font_notify ( Handle self, Handle child, void * font);
extern Bool kill_all( Handle self, Handle child, void * dummy);

void
Application_init( Handle self, HV * profile)
{
   int hintPause = pget_i( hintPause);
   Color hintColor = pget_i( hintColor), hintBackColor = pget_i( hintBackColor);
   SV * hintFont = pget_sv( hintFont);
   char * hintClass      = pget_c( hintClass);
   char * clipboardClass = pget_c( clipboardClass);
   char * printerClass   = pget_c( printerClass);
   if ( application != nilHandle)
      croak( "RTC0010: Attempt to create more than one application instance");

   CDrawable-> init( self, profile);
   list_create( &var widgets, 16, 16);
   list_create( &var modalHorizons, 0, 8);
   application = self;
   if ( !apc_application_create( self))
      croak( "RTC0011: Error creating application");
// Widget init
   SvHV_Font( pget_sv( font), &Font_buffer, "Application::init");
   my set_font( self, Font_buffer);
   SvHV_Font( pget_sv( popupFont), &Font_buffer, "Application::init");
   my set_popup_font( self, Font_buffer);
   {
      AV * av = ( AV *) SvRV( pget_sv( designScale));
      SV ** holder = av_fetch( av, 0, 0);
      if ( holder)
         var designScale. x = SvNV( *holder);
      else
         warn("RTC0012: Array panic on 'designScale'");
      holder = av_fetch( av, 1, 0);
      if ( holder)
         var designScale. y = SvNV( *holder);
      else
         warn("RTC0012: Array panic on 'designScale'");
      pdelete( designScale);
   }
   var text = malloc( 1);
   var text[ 0] = 0;
   opt_set( optModalHorizon);

   {
      HV * profile = newHV();
      static Timer_vmt HintTimerVmt;

      pset_c( name, "Clipboard");
      pset_H( owner, self);
      var clipboard = create_instance( clipboardClass);
      protect_object( var clipboard);
      hv_clear( profile);

      pset_c( name, "Printer");
      pset_H( owner, self);
      var printer = create_instance( printerClass);
      protect_object( var printer);
      hv_clear( profile);

      pset_H( owner, self);
      pset_i( timeout, hintPause);
      pset_c( name, "HintTimer");
      var hintTimer = create_instance( "Prima::Timer");
      protect_object( var hintTimer);
      hv_clear( profile);
      memcpy( &HintTimerVmt, CTimer, sizeof( HintTimerVmt));
      HintTimerVmt. on_tick = Application_HintTimer_Tick;
      (( PTimer) var hintTimer)-> self = &HintTimerVmt;

      pset_H( owner, self);
      pset_i( color, hintColor);
      pset_i( backColor, hintBackColor);
      pset_i( visible, 0);
      pset_i( selectable, 0);
      pset_i( showHint, 0);
      pset_c( name, "HintWidget");
      pset_sv( font, hintFont);
      var hintWidget = create_instance( hintClass);
      protect_object( var hintWidget);
      sv_free(( SV *) profile);
   }
   my set( self, profile);
}

void
Application_done( Handle self)
{
   my close_help( self);
   my first_that( self, kill_all, nil);
   my first_that_component( self, kill_all, nil);
   unprotect_object( var clipboard);
   unprotect_object( var printer);
   unprotect_object( var hintTimer);
   unprotect_object( var hintWidget);
   list_destroy( &var modalHorizons);
   list_destroy( &var widgets);
   free( var helpFile);
   free( var text);
   free( var hint);
   var accelTable = var printer = var clipboard = var hintWidget = var hintTimer = nilHandle;
   var helpFile   = var text    = var hint      = nil;
   apc_application_destroy( self);
   CDrawable-> done( self);
   application = nilHandle;
}

void
Application_set( Handle self, HV * profile)
{
   pdelete( owner);
   pdelete( syncPaint);
   pdelete( clipOwner);
   pdelete( left);
   pdelete( rigth);
   pdelete( width);
   pdelete( height);
   pdelete( top);
   pdelete( bottom);
   pdelete( size);
   pdelete( origin);
   pdelete( rect);
   pdelete( enabled);
   pdelete( visible);
   pdelete( tabOrder);
   pdelete( tabStop);
   pdelete( growMode);
   pdelete( text);
   pdelete( selectable);
   pdelete( ownerPalette);
   pdelete( palette);
   pdelete( ownerShowHint);
   pdelete( ownerBackColor);
   pdelete( ownerColor);
   pdelete( ownerFont);
   pdelete( centered);
   pdelete( transparent);
   pdelete( clipboardClass);
   pdelete( printerClass);
   pdelete( hintClass);
   pdelete( hintVisible);
   pdelete( buffered);
   pdelete( modalHorizon);
   inherited set( self, profile);
}

void Application_handle_event( Handle self, PEvent event)
{
   switch ( event-> cmd)
   {
      case cmPost:
      if ( event-> gen. H != self)
      {
         ((( PComponent) event-> gen. H)-> self)-> message( event-> gen. H, event);
         event-> cmd = 0;
         if ( var stage > csNormal) return;
      }
      break;
   }
   inherited handle_event ( self, event);
}

void
Application_yield( char * dummy)
{
   apc_application_yield();
}

Bool
Application_begin_paint( Handle self)
{
   Bool ok;
   if ( is_opt( optInDraw)) return false;
   CDrawable-> begin_paint( self);
   if ( !( ok = apc_application_begin_paint( self)))
      CDrawable-> end_paint( self);
   return ok;
}

Bool
Application_begin_paint_info( Handle self)
{
   Bool ok;
   if ( is_opt( optInDraw))     return true;
   if ( is_opt( optInDrawInfo)) return false;
   CDrawable-> begin_paint_info( self);
   if ( !( ok = apc_application_begin_paint_info( self)))
      CDrawable-> end_paint_info( self);
   return ok;
}

void
Application_end_paint( Handle self)
{
   if ( !is_opt( optInDraw)) return;
   apc_application_end_paint( self);
   CDrawable-> end_paint( self);
}

void
Application_end_paint_info( Handle self)
{
   if ( !is_opt( optInDrawInfo)) return;
   apc_application_end_paint_info( self);
   CDrawable-> end_paint_info( self);
}

void Application_bring_to_front( Handle self) {}
void Application_set_focused( Handle self, Bool focused) {}
void Application_show( Handle self) {}
void Application_hide( Handle self) {}
void Application_insert_behind( Handle self, Handle view) {}
void Application_send_to_back( Handle self) {}
Point Application_client_to_screen( Handle self, Point p) { return p; }
Point Application_screen_to_client( Handle self, Point p) { return p; }

SV*
Application_fonts( Handle self, char * name)
{
   int count, i;
   AV * glo = newAV();
   PFont fmtx = apc_fonts( strlen( name) ? name : nil, &count);
   for ( i = 0; i < count; i++) {
      SV * sv      = sv_Font2HV( &fmtx[ i]);
      HV * profile = ( HV*) SvRV( sv);
      pdelete( resolution);
      pdelete( codepage);
      av_push( glo, sv);
   }
   free( fmtx);
   return newRV_noinc(( SV *) glo);
}

Font
Application_get_default_font( char * dummy)
{
   Font font;
   apc_font_default( &font);
   return font;
}

Font
Application_get_message_font( char * dummy)
{
   Font font;
   apc_sys_get_msg_font( &font);
   return font;
}

Font
Application_get_caption_font( char * dummy)
{
   Font font;
   apc_sys_get_caption_font( &font);
   return font;
}


int
Application_get_default_cursor_width( char * dummy)
{
   return apc_sys_get_value( svXCursor);
}


Point
Application_get_default_scrollbar_metrics( char * dummy)
{
   Point ret = {
      apc_sys_get_value( svXScrollbar),
      apc_sys_get_value( svYScrollbar)
   };
   return ret;
}

Point
Application_get_default_window_borders( char * dummy, int borderStyle)
{
   Point ret = { 0,0};
   switch ( borderStyle) {
   case bsNone:
      ret.x = svXbsNone;
      ret.y = svYbsNone;
      break;
   case bsSizeable:
      ret.x = svXbsSizeable;
      ret.y = svYbsSizeable;
      break;
   case bsSingle:
      ret.x = svXbsSingle;
      ret.y = svYbsSingle;
      break;
   case bsDialog:
      ret.x = svXbsDialog;
      ret.y = svYbsDialog;
      break;
   default:
      return ret;
   }
   ret. x = apc_sys_get_value( ret. x);
   ret. y = apc_sys_get_value( ret. y);
   return ret;
}

int
Application_get_system_value( char * dummy, int sysValue)
{
   return apc_sys_get_value( sysValue);
}

SV *
Application_get_system_info( char * dummy)
{
   HV * profile = newHV();
   char system   [ 1024];
   char release  [ 1024];
   char vendor   [ 1024];
   char arch     [ 1024];
   char gui_desc [ 1024];
   int  os, gui;

   os  = apc_application_get_os_info( system, sizeof( system),
				      release, sizeof( release),
				      vendor, sizeof( vendor),
				      arch, sizeof( arch));
   gui = apc_application_get_gui_info( gui_desc, sizeof( gui_desc));

   pset_i( apc,            os);
   pset_i( gui,            gui);
   pset_c( system,         system);
   pset_c( release,        release);
   pset_c( vendor,         vendor);
   pset_c( architecture,   arch);
   pset_c( guiDescription, gui_desc);

   return newRV_noinc(( SV *) profile);
}


Handle
Application_get_clipboard( Handle self)
{
   return var clipboard;
}

Handle
Application_get_printer( Handle self)
{
   return var printer;
}

Point
Application_get_pos( Handle self)
{
   Point ret = { 0, 0};
   return ret;
}

Handle
Application_get_widget_from_handle( Handle self, SV * handle)
{
   ApiHandle apiHandle;
   if ( SvIOK( handle))
	   apiHandle = SvUVX( handle);
   else
      apiHandle = sv_2uv( handle);
   return apc_application_get_handle( self, apiHandle);
}

Handle
Application_get_hint_widget( Handle self)
{
   return var hintWidget;
}

char *
Application_get_help_file ( Handle self)
{
   return var helpFile ? var helpFile : "";
}

void
Application_set_help_file( Handle self, char * helpFile)
{
   if ( var stage > csNormal) return;
   if ( var helpFile && ( strcmp( var helpFile, helpFile) == 0)) return;
   free( var helpFile);
   strcpy( var helpFile = malloc( strlen ( helpFile) + 1), helpFile);
   apc_help_set_file( self, helpFile);
}

Handle
Application_get_focused_widget( Handle self)
{
   return apc_widget_get_focused();
}

Handle
Application_get_active_window( Handle self)
{
   return apc_window_get_active();
}

SV *
Application_sys_action( Handle self, char * params)
{
   char * i = apc_system_action( params);
   SV * ret = i ? newSVpv( i, 0) : nilSV;
   free( i);
   return ret;
}

typedef struct _SingleColor
{
   Color color;
   int   index;
} SingleColor, *PSingleColor;


Color
Application_get_color_index( Handle self, int index)
{
   index = (( index < 0) || ( index > ciMaxId)) ? 0 : index;
   switch ( index)
   {
     case ciFore:
        return opt_InPaint ?
           CDrawable-> get_color ( self) : var colors[ index];
     case ciBack:
        return opt_InPaint ?
           CDrawable-> get_back_color ( self) : var colors[ index];
     default:
        return  var colors[ index];
   }
}

void
Application_set_font( Handle self, Font font)
{
   if ( !opt_InPaint) my first_that( self, font_notify, &font);
   apc_font_pick( self, &font, & var font);
   if ( opt_InPaint) apc_gp_set_font ( self, & var font);
}

void
Application_set_color_index( Handle self, Color color, int index)
{
   SingleColor s = { color, index};
   if ( var stage > csNormal) return;
   if (( index < 0) || ( index > ciMaxId)) return;
   if ( !opt_InPaint) my first_that( self, single_color_notify, &s);
   if ( opt_InPaint) switch ( index)
   {
      case ciFore:
         CDrawable-> set_color ( self, color);
         break;
      case ciBack:
         CDrawable-> set_back_color ( self, color);
         break;
    }
   var colors[ index] = color;
}

Bool
Application_close( Handle self)
{
   if ( var stage > csNormal) return true;
   return my can_close( self) ? ( apc_application_close( self), true) : false;
}

Bool
Application_get_insert_mode( Handle self)
{
   return apc_sys_get_insert_mode();
}

void
Application_set_insert_mode( Handle self, Bool insMode)
{
   apc_sys_set_insert_mode( insMode);
}

Handle
Application_get_parent( Handle self)
{
   return nilHandle;
}

Point
Application_get_scroll_rate( Handle self)
{
   Point ret = {
      apc_sys_get_value( svAutoScrollFirst),
      apc_sys_get_value( svAutoScrollNext)
   };
   return ret;
}

static void hshow( Handle self)
{
   PWidget_vmt hintUnder = CWidget( var hintUnder);
   char * text = hintUnder-> get_hint( var hintUnder);
   Point size  = hintUnder-> get_size( var hintUnder);
   Point s = my get_size( self);
   Point fin = {0,0};
   Point pos = hintUnder-> client_to_screen( var hintUnder, fin);
   Point mouse = my get_pointer_pos( self);
   Point hintSize;
   PWidget_vmt hintWidget = CWidget( var hintWidget);

   hintWidget-> set_text( var hintWidget, text);
   hintSize = hintWidget-> get_size( var hintWidget);

   fin. x = mouse. x - 16;
   fin. y = pos. y - hintSize. y - 1;
   if ( fin. y > mouse. y - hintSize. y - 32) fin. y = mouse. y - hintSize. y - 32;

   if ( fin. x + hintSize. x >= s. x) fin. x = pos. x - hintSize. x;
   if ( fin. x < 0) fin. x = 0;
   if ( fin. y + hintSize. y >= s. y) fin. y = pos. y - hintSize. y;
   if ( fin. y < 0) fin. y = pos. y + size. y + 1;
   if ( fin. y < 0) fin. y = 0;

   hintWidget-> set_pos( var hintWidget, fin. x, fin. y);
   hintWidget-> show( var hintWidget);
   hintWidget-> bring_to_front( var hintWidget);
}

void
Application_HintTimer_Tick( Handle timer)
{
   Handle self = application;
   Point pos = my get_pointer_pos( self);
   ((( PTimer) timer)-> self)-> stop( timer);
   if ( var hintActive == 1)
   {
      Event ev = {cmHint};
      if ( pos. x != var hintMousePos. x || pos. y != var hintMousePos. y) return;
      if ( !var hintUnder || (( PObject) var hintUnder)-> stage != csNormal) return;
      ev. gen. B = true;
      ev. gen. H = var hintUnder;
      var hintVisible = 1;
      if (( PWidget( var hintUnder)-> stage == csNormal) &&
          ( CWidget( var hintUnder)-> message( var hintUnder, &ev)))
          hshow( self);
   } else if ( var hintActive == -1)
      var hintActive = 0;
}

void
Application_set_hint_action( Handle self, Handle view, Bool show, Bool byMouse)
{
   if ( show && !is_opt( optShowHint)) return;
   if ( show)
   {
      var hintMousePos = my get_pointer_pos( self);
      var hintUnder = view;
      if ( var hintActive == -1)
      {
         Event ev = {cmHint};
         ev. gen. B = true;
         ev. gen. H = view;
         ((( PTimer) var hintTimer)-> self)-> stop( var hintTimer);
         var hintVisible = 1;
         if (( PWidget( view)-> stage == csNormal) &&
             ( CWidget( view)-> message( view, &ev)))
             hshow( self);
      } else {
         if ( !byMouse && var hintActive == 1) return;
         CTimer( var hintTimer)-> start( var hintTimer);
      }
      var hintActive = 1;
   } else {
      int oldHA = var hintActive;
      int oldHV = var hintVisible;
      if ( oldHA != -1)
         ((( PTimer) var hintTimer)-> self)-> stop( var hintTimer);
      if ( var hintVisible)
      {
         Event ev = {cmHint};
         ev. gen. B = false;
         ev. gen. H = view;
         var hintVisible = 0;
         if (( PWidget( view)-> stage != csNormal) ||
              ( CWidget( view)-> message( view, &ev)))
            CWidget( var hintWidget)-> hide( var hintWidget);
      }
      if ( oldHA != -1) var hintActive = 0;
      if ( byMouse && oldHV) {
         var hintActive = -1;
         CTimer( var hintTimer)-> start( var hintTimer);
      }
   }
}

void
Application_set_hint_color( Handle self, Color hintColor)
{
   CWidget( var hintWidget)-> set_color( var hintWidget, hintColor);
}

void
Application_set_hint_back_color( Handle self, Color hintBackColor)
{
   CWidget( var hintWidget)-> set_back_color( var hintWidget, hintBackColor);
}

void
Application_set_hint_font( Handle self, Font hintFont)
{
   CWidget( var hintWidget)-> set_font( var hintWidget, hintFont);
}


void
Application_set_hint_pause( Handle self, int hintPause)
{
   CTimer( var hintTimer)-> set_timeout( var hintTimer, hintPause);
}

Color
Application_get_hint_color( Handle self)
{
   return CWidget( var hintWidget)-> get_color( var hintWidget);
}

Color
Application_get_hint_back_color( Handle self)
{
   return CWidget( var hintWidget)-> get_back_color( var hintWidget);
}

Font
Application_get_hint_font( Handle self)
{
   return CWidget( var hintWidget)-> get_font( var hintWidget);
}

int
Application_get_hint_pause( Handle self)
{
   return CTimer( var hintTimer)-> get_timeout( var hintTimer);
}

void
Application_set_palette( Handle self, SV * palette)
{
   CDrawable-> set_palette( self, palette);
}

void
Application_set_show_hint( Handle self, Bool showHint)
{
   opt_assign( optShowHint, showHint);
}

Handle Application_next( Handle self) { return self;}
Handle Application_prev( Handle self) { return self;}

SV *
Application_get_palette( Handle self)
{
   return CDrawable-> get_palette( self);
}

Handle
Application_top_frame( Handle self, Handle from)
{
   while ( from) {
      if ( kind_of( from, CWindow) &&
           (( PWidget(from)-> owner == application) || !CWidget(from)-> get_clip_owner(from))
         )
         return from;
      from = PWidget( from)-> owner;
   }
   return application;
}

Handle
Application_get_image( Handle self, int x, int y, int xLen, int yLen)
{
   HV * profile;
   Handle i;
   Bool ret;
   if ( var stage > csNormal) return nilHandle;
   if ( xLen <= 0 || yLen <= 0) return nilHandle;

   profile = newHV();
   i = Object_create( "Prima::Icon", profile);
   sv_free(( SV *) profile);
   ret = apc_application_get_bitmap( self, i, x, y, xLen, yLen);
   --SvREFCNT( SvRV((( PAnyObject) i)-> mate));
   return ret ? i : nilHandle;
}


Handle
Application_map_focus( Handle self, Handle from)
{
   Handle topFrame = my top_frame( self, from);
   Handle topShared;

   if ( var topExclModal)
      return ( topFrame == var topExclModal) ? from : var topExclModal;

   if ( !var topSharedModal && var modalHorizons. count == 0)
      return from; // return from if no shared modals active

  if ( topFrame == self) {
      if ( !var topSharedModal) return from;
      topShared = var topSharedModal;
   } else {
      Handle horizon =
         ( !CWindow( topFrame)-> get_modal_horizon( topFrame)) ?
         CWindow( topFrame)-> get_horizon( topFrame) : topFrame;
      if ( horizon == self)
         topShared = var topSharedModal;
      else
         topShared = PWindow( horizon)-> topSharedModal;
   }

   return ( !topShared || ( topShared == topFrame)) ? from : topShared;
}

Handle
Application_popup_modal( Handle self)
{
   Handle ha = apc_window_get_active();
   Handle xTop, ret = nilHandle;


#define popupWin                                      \
STMT_START {							                     \
   PWindow_vmt top = CWindow( xTop);			         \
   if ( !top-> get_visible( xTop))			          	\
      top-> set_visible( xTop, 1);              		\
   if ( top-> get_window_state( xTop) == wsMinimized)	\
      top-> set_window_state( xTop, wsNormal);	      \
   top-> set_selected( xTop, 1);          				\
   ret = xTop;                                        \
} STMT_END


   if ( var topExclModal) {
   // checking exclusive modal chain
      xTop = ( !ha || ( PWindow(ha)->modal == 0)) ? var exclModal : ha;
      while ( xTop) {
         if ( PWindow(xTop)-> nextExclModal) {
            CWindow(xTop)-> bring_to_front( xTop);
            xTop = PWindow(xTop)-> nextExclModal;
         } else {
            popupWin;
            break;
         }
      }
   } else {
      if ( !var topSharedModal && var modalHorizons. count == 0)
         return nilHandle; // return from if no shared modals active
      // checking shared modal chains
      if ( ha) {
         xTop = ( PWindow(ha)->modal == 0) ? CWindow(ha)->get_horizon(ha) : ha;
         if ( xTop == application) xTop = var sharedModal;
      } else
         xTop = var sharedModal ? var sharedModal : var modalHorizons. items[ 0];

      while ( xTop) {
         if ( PWindow(xTop)-> nextSharedModal) {
            CWindow(xTop)-> bring_to_front( xTop);
            xTop = PWindow(xTop)-> nextSharedModal;
         } else {
            popupWin;
            break;
         }
      }
   }

   return ret;
}


void Application_update_sys_handle( Handle self, HV * profile) {}

char * Application_get_text( Handle self) { return ""; }
Bool Application_get_enabled( Handle self) { return true; }
Bool Application_get_tab_stop( Handle self) { return false; }
Bool Application_get_selectable( Handle self) { return false; }
Handle Application_get_shape( Handle self) { return nilHandle; }
Bool Application_get_sync_paint( Handle self) { return false; }
Bool Application_get_visible( Handle self) { return true; }
Bool Application_get_modal_stop( Handle self) { return true; }

void Application_set_help_context( Handle self, long context)
{
   if ( context == hmpOwner) context = hmpNone;
   inherited set_help_context( self, context);
}

void Application_set_text( Handle self, char * text) {}
void Application_set_buffered( Handle self, Bool buffered) {}
void Application_set_centered( Handle self, Bool x, Bool y) {}
void Application_set_enabled( Handle self, Bool enable) {}
void Application_set_visible( Handle self, Bool visible) {}
void Application_set_grow_mode( Handle self, int flags) {}
void Application_set_hint_visible( Handle self, Bool visible) {}
void Application_set_modal_horizon( Handle self, Bool modalHorizon) {}
void Application_set_owner( Handle self, Handle owner) {}
void Application_set_owner_color( Handle self, Bool ownerColor) {}
void Application_set_owner_back_color( Handle self, Bool ownerBackColor) {}
void Application_set_owner_font( Handle self, Bool ownerFont) {}
void Application_set_owner_show_hint( Handle self, Bool ownerShowHint) {}
void Application_set_owner_palette( Handle self, Bool ownerPalette) {}
void Application_set_pos( Handle self, int x, int y) {}
void Application_set_size( Handle self, int x, int y) {}
void Application_set_selectable( Handle self, Bool selectable) {}
void Application_set_shape( Handle self, Handle mask) {}
void Application_set_sync_paint( Handle self, Bool syncPaint) {}
void Application_set_clip_owner( Handle self, Bool clipOwner) {}
void Application_set_tab_order( Handle self, int tabOrder) {}
void Application_set_tab_stop( Handle self, Bool tabStop) {}
void Application_set_transparent( Handle self, Bool transparent) {}
