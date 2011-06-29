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

object Prima::Window( Prima::Widget)
{
   Handle   menu;
   Font     menuFont;
   ColorSet menuColor;
   int      modal;
   int      modalResult;

   property int      borderIcons;
   property int	     borderStyle;
   property Bool     focused;
   property Point    frameOrigin;
   property Point    frameSize;
   property Handle   icon;
   property Handle   menu;
   property Color    menuColorIndex( int index);
   property SV *     menuItems;
   property Bool     modalHorizon;
   property int      modalResult;
   property Bool     onTop;
   property Point    origin;
   property Bool     ownerIcon;
   property Rect     rect;
   property Bool     selected;
   property Point    size;
   property Bool     taskListed;
   property SV *     text;
   property Bool     transparent;
   property int      windowState;

   method   void     cancel();
   method   void     cleanup();
   method   void     end_modal();
   method   int      execute( Handle insertBefore = nilHandle);
   method   Bool     execute_shared( Handle insertBefore = nilHandle);
   method   SV   *   get_client_handle();
   static   Font     get_default_menu_font( char * dummy = "");
   method   Handle   get_horizon();
   method   Font     get_menu_font();
   method   int      get_modal();
   method   Handle   get_modal_window( int modalFlag = mtExclusive, Bool next = true);
   c_only   void     handle_event ( PEvent event);
   method   void     init( HV * profile);
   import   SV *     notification_types();
   method   void     ok();
   c_only   Bool     process_accel( int key);
   import   SV *     profile_default ();
   method   void     set( HV * profile);
   method   void     set_menu_font( Font font);
   c_only   void     update_sys_handle( HV * profile);
   c_only   Bool     validate_owner( Handle * newOwner, HV * profile);
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
