#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
# $Id$

object Prima::Application( Prima::Widget)
{
   ColorSet  colors;
   Handle    hintTimer;
   Handle    hintWidget;
   Handle    icon;
   Bool      autoClose;

   property  Bool   autoClose;
   property  Bool   buffered;
   property  Bool   clipOwner;
   property  Color  colorIndex( int index);
   property  Bool   enabled;
   property  Bool   focused;
   property  Handle icon;
   property  Bool   insertMode;
   property  int    growMode;
   property  Color  hintColor;
   property  Color  hintBackColor;
   property  int    hintPause;
   property  Bool   modalHorizon;
   property  SV  *  palette;
   property  Bool   pointerVisible;
   property  Point  origin;
   property  Handle owner;
   property  Bool   ownerBackColor;
   property  Bool   ownerColor;
   property  Bool   ownerFont;
   property  Bool   ownerShowHint;
   property  Bool   ownerPalette;
   property  Bool   selectable;
   property  Handle shape;
   property  Point  size;
   property  Bool   showHint;
   property  Bool   syncPaint;
   property  int    tabOrder;
   property  Bool   tabStop;
   property  Bool   transparent;
   property  SV *   text;
   property  Bool   visible;
   property  Bool   wantUnicodeInput;

   method Bool   begin_paint();
   method Bool   begin_paint_info();
   method void   bring_to_front();
   method void   cleanup();
   method Bool   close();
   method void   done();
   method void   detach( Handle objectHandle, Bool kill);
   method void   end_paint();
   method void   end_paint_info();
   method SV*    fonts( char * name = "", char * encoding = "");
   method SV*    font_encodings( char * encoding = "");
   method Handle get_active_window();
   static Font   get_caption_font( char * dummy = "");
   static int    get_default_cursor_width( char * dummy = "");
   static Font   get_default_font( char * dummy = "");
   static Point  get_default_scrollbar_metrics( char * dummy = "");
   static Point  get_default_window_borders( char * dummy = "", int borderStyle = bsSizeable);
   method Handle get_focused_widget();
   method Handle get_widget_from_handle( SV * handle);
   method Handle get_hint_widget();
   method Rect   get_indents() => apc_application_get_indents;
   static Font   get_message_font( char * dummy = "");
   method Handle get_modal_window( int modalFlag = mtExclusive, Bool topMost = true);
   method Point  get_scroll_rate();
   static SV *   get_system_info( char * dummy = "");
   static int    get_system_value( char * dummy = "", int sysValue = 0);
   method Bool   go( void)   => apc_application_go;
   c_only void   handle_event ( PEvent event);
   method void   hide();
   method void   init( HV * profile);
   method Bool   lock()                        => apc_application_lock;
   method void   insert_behind( Handle widget);
   method Handle next();
   import SV *   notification_types();
   method Handle prev();
   method void   send_to_back();
   import void   setup();
   c_only void   set_hint_action( Handle widget, Bool show, Bool mouseEvent);
   method void   show();
   static void   sync( char *dummy = "");
   static SV*    sys_action( char * dymmy = "", char * params = "");
   method Bool   unlock()                      => apc_application_unlock;
   c_only void   update_sys_handle( HV * profile);
   c_only Bool   validate_owner( Handle * newOwner, HV * profile);
   static void   yield( char *dummy = "");

   method Bool   get_capture();
   method Font   get_hint_font();
   method Handle get_image( int x, int y, int xLen, int yLen);
   method Handle get_parent();
   method Handle get_widget_from_point( Point p) => apc_application_get_widget_from_point;

   method void   set( HV * profile);
   method void   set_capture( Bool capture, Handle confineTo);
   method void   set_centered( Bool x, Bool y);
   method void   set_font( Font font);
   method void   set_hint_font( Font hintFont);

   int     hintActive;
   Bool    hintVisible;
   Handle  hintUnder;
   Handle  topExclModal;
   Handle  exclModal;
   Handle  topSharedModal;
   Handle  sharedModal;
   List    modalHorizons;
   Bool    wantUnicodeInput;

   c_only  Handle map_focus( Handle from);
   c_only  Handle top_frame( Handle from);
   c_only  Handle popup_modal();
}
