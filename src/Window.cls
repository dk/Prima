object Prima::Window( Prima::Widget)
{
	Handle   menu;
	Font     menuFont;
	ColorSet menuColor;
	int      modal;
	int      modalResult;
	SV *     effects;

	property int      borderIcons;
	property int	  borderStyle;
	property SV *     effects;
	property Bool     focused;
	property Point    frameOrigin;
	property Point    frameSize;
	property Handle   icon;
	property Bool     mainWindow;
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
	property Bool     transparent;
	property int      windowState;

	c_only   Bool     can_propagate_key();
	method   void     cancel();
	method   void     cleanup();
	method   void     done();
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
	method   void     set_text( SV * text);
	c_only   void     update_sys_handle( HV * profile);
	c_only   Bool     validate_owner( Handle * newOwner, HV * profile);
#	private
	Handle   nextExclModal;
	Handle   prevExclModal;
	Handle   nextSharedModal;
	Handle   prevSharedModal;
	Handle   topSharedModal;
	c_only   void    exec_enter_proc( Bool sharedExec, Handle insertBefore);
	c_only   void    exec_leave_proc();
	c_only   void    cancel_children();
}
