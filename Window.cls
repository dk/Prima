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
object Prima::Window( Prima::Widget)
{
   Handle   menu;
   Font     menuFont;
   ColorSet menuColor;
   int      modal;
   int      modalResult;
   method   void     cancel();
   method   void     done();
   method   void     end_modal();
   method   int      execute( Handle insertBefore = nilHandle);
   method   Bool     execute_shared( Handle insertBefore = nilHandle);
   method   int      get_border_icons()              => apc_window_get_border_icons;
   method   int      get_border_style()              => apc_window_get_border_style;
   static   Font     get_default_menu_font( char * dummy = "");
   method   Point    get_frame_pos()                 => apc_widget_get_pos;
   method   Point    get_frame_size()                => apc_widget_get_size;
   method   Handle   get_horizon();
   method   Handle   get_icon();
   method   Handle   get_menu();
   method   Color    get_menu_color( int index);
   method   Font     get_menu_font();
   method   SV *     get_menu_items();
   method   int      get_modal();
   method   Bool     get_modal_horizon();
   method   int      get_modal_result();
   method   Point    get_pos()                       => apc_window_get_client_pos;
   method   Point    get_size()                      => apc_window_get_client_size;
   method   Bool     get_task_listed()               => apc_window_get_task_listed;
   method   int      get_window_state()              => apc_window_get_window_state;
   c_only   void     handle_event ( PEvent event);
   method   void     init( HV * profile);
   method   void     ok();
   c_only   Bool     process_accel( int key);
   import   SV *     profile_default ();
   method   void     set( HV * profile);
   import   void     set_border_icons( int icons);
   import   void     set_border_style( int style);
   method   void     set_icon( Handle icon);
   method   void     set_focused( Bool focused);
   method   void     set_frame_size( int x, int y)   => apc_widget_set_size;
   method   void     set_frame_pos( int x, int y)    => apc_widget_set_pos;
   method   void     set_menu( Handle menu);
   method   void     set_menu_color( Color color, int index);
   method   void     set_menu_font( Font font);
   method   void     set_menu_items( SV * menuItems);
   method   void     set_modal_horizon( Bool modalHorizon);
   method   void     set_modal_result( int modalResult);
   method   void     set_pos( int x, int y)          => apc_window_set_client_pos;
   method   void     set_selected( Bool selected);
   method   void     set_size( int x, int y)         => apc_window_set_client_size;
   import   void     set_task_listed( Bool taskListed);
   method   void     set_text( char * text);
   method   void     set_transparent( Bool transparent);
   method   void     set_window_state( int state)    => apc_window_set_window_state;
   c_only   void     update_sys_handle( HV * profile);
   method   void     update_delegator();
   event    void     on_activate();
   event    void     on_deactivate();
   event    void     on_endmodal();
   event    void     on_execute();
   event    void     on_windowstate( int windowState);
   SV * onActivate;
   SV * onDeactivate;
   SV * onEndModal;
   SV * onExecute;
   SV * onWindowState;
# private
   Handle   nextExclModal;
   Handle   prevExclModal;
   Handle   nextSharedModal;
   Handle   prevSharedModal;
   Handle   topSharedModal;
   c_only   void    exec_enter_proc( Bool sharedExec, Handle insertBefore);
   c_only   void    exec_leave_proc();
   c_only   void    cancel_children();

}
