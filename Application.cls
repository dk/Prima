#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
object Application( Widget)
{
   Handle    clipboard;
   Handle    printer;
   ColorSet  colors;
   Handle    hintTimer;
   Handle    hintWidget;
   char *    helpFile;

   method Bool   begin_paint();
   method Bool   begin_paint_info();
   method void   bring_to_front();
   method Point  client_to_screen( Point p);
   method Bool   close();
   method void   close_help()                                 => apc_help_close;
   method void   done();
   method void   end_paint();
   method void   end_paint_info();
   method SV*    fonts( char * name = "");
   method Handle get_active_window();
   static Font   get_caption_font( char * dummy = "");
   static int    get_default_cursor_width( char * dummy = "");
   static Font   get_default_font( char * dummy = "");
   static Point  get_default_scrollbar_metrics( char * dummy = "");
   static Point  get_default_window_borders( char * dummy = "", int borderStyle = bsSizeable);
   method Handle get_clipboard();
   method Bool   get_insert_mode();
   method char*  get_help_file();
   method Handle get_focused_widget();
   method Handle get_widget_from_handle( SV * handle);
   method Handle get_hint_widget();
   static Font   get_message_font( char * dummy = "");
   method Handle get_printer();
   method Point  get_pos();
   method Point  get_scroll_rate();
   method Point  get_size()  => apc_application_get_size;
   static SV *   get_system_info( char * dummy = "");
   static int    get_system_value( char * dummy = "", int sysValue = 0);
   method void   go( void)   => apc_application_go;
   c_only void   handle_event ( PEvent event);
   method Bool   help_context( long context)   => apc_help_open_topic;
   method void   hide();
   method void   init( HV * profile);
   method void   lock()                        => apc_application_lock;
   method void   insert_behind( Handle view);
   method Handle next();
   method Handle prev();
   method Point  screen_to_client( Point p);
   method void   send_to_back();
   method void   set_insert_mode( Bool insMode);
   method void   set_help_file( char * helpFile);
   c_only void   set_hint_action( Handle view, Bool show, Bool mouseEvent);
   method void   set_hint_visible( Bool visible);
   method void   show();
   method SV*    sys_action( char * params);
   method void   unlock()                      => apc_application_unlock;
   c_only void   update_sys_handle( HV * profile);
   static void   yield( char *dummy = "");

   method Color  get_color_index( int index);
   method Bool   get_enabled();
   method Color  get_hint_back_color();
   method Color  get_hint_color();
   method Font   get_hint_font();
   method int    get_hint_pause();
   method Bool   get_modal_stop();
   method SV *   get_palette();
   method Handle get_parent();
   method Bool   get_selectable();
   method Bool   get_sync_paint();
   method Bool   get_tab_stop();
   method char * get_text();
   method Handle get_view_from_point( Point p) => apc_application_get_view_from_point;
   method Bool   get_visible();

   method void   set( HV * profile);
   method void   set_buffered( Bool buffered);
   method void   set_centered( Bool x, Bool y);
   method void   set_clip_owner( Bool clipOwner);
   method void   set_color_index( Color color, int index);
   method void   set_enabled( Bool enable);
   method void   set_focused( Bool focused);
   method void   set_font( Font font);
   method void   set_help_context( long helpContext);
   method void   set_hint_back_color( Color hintBackColor);
   method void   set_hint_color( Color hintColor);
   method void   set_hint_font( Font hintFont);
   method void   set_hint_pause( int hintPause);
   method void   set_grow_mode( int flags);
   method void   set_modal_horizon( Bool modalHorizon);
   method void   set_owner ( Handle owner);
   method void   set_owner_back_color( Bool ownerBackColor);
   method void   set_owner_color( Bool ownerColor);
   method void   set_owner_font( Bool ownerFont);
   method void   set_owner_show_hint( Bool ownerShowHint);
   method void   set_owner_palette( Bool ownerShowHint);
   method void   set_pos( int x, int y);
   method void   set_palette( SV * palette);
   method void   set_selectable( Bool selectable);
   method void   set_sync_paint( Bool syncPaint);
   method void   set_size( int x, int y);
   method void   set_show_hint( Bool showHint);
   method void   set_text( char * text);
   method void   set_visible( Bool visible);
   method void   set_tab_order( int tabOrder);
   method void   set_tab_stop( Bool tabStop);
   method void   set_transparent( Bool transparent);

   int     hintActive;
   Bool    hintVisible;
   Point   hintMousePos;
   Handle  hintUnder;
   Handle  topExclModal;
   Handle  exclModal;
   Handle  topSharedModal;
   Handle  sharedModal;
   List    modalHorizons;

   c_only  Handle map_focus( Handle from);
   c_only  Handle top_frame( Handle from);
   c_only  Handle popup_modal();
}
