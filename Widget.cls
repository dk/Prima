#
#  Main widget description
#

object Widget( Drawable)
{
# user variables
   Handle    accelTable;	       # accelerator table
   List      widgets;		       # list of child widgets
   Handle    currentWidget;	       # current widget pointer
   int       cursorLock;               # cursor lock count
   NPoint    designScale;              # font width and height for view design
   SV     *  dragPointer;              # cursor shape when dragging smth over
   int       growMode;                 # growing flags, see gmGrowXXX
   long      helpContext;              # help context
   char   *  hint;                     # hint tag text
   int       lockCount;                # lock count
   Font      popupFont;                # font for popup menu
   ColorSet  popupColor;               # colors for popup menu
   Handle    popupMenu;                # popup menu by default
   SV     *  pointer;                  # cursor shape
   Point     pos;                      # last position
   Point     sizeUnbound;              # fictive size not aware of size limits
   Point     sizeMin;                  # default min size
   Point     sizeMax;                  # default max size
   int       tabOrder;                 # tab ordering number
   char   *  text;                     # caption text
   int       selectingButtons;         # which mouse buttons can select a view
   int       widgetClass;              # predefined system class

# direct call methods
   method   void     attach( Handle objectHandle);
   method   Bool     begin_paint();
   method   Bool     begin_paint_info();
   method   void     bring_to_front();
   method   Bool     can_close();
   method   void     click();
   method   Point    client_to_screen( Point p)       => apc_widget_client_to_screen;
   method   Bool     close();
   c_only   Bool     custom_paint();
   method   void     detach( Handle objectHandle, Bool kill);
   import   void     disable_commands();
   method   void     done();
   import   void     enable_commands();
   method   void     end_paint();
   method   void     end_paint_info();
   c_only   Handle   first_that( void * actionProc, void * params);
   c_only   void     handle_event ( PEvent event);
   method   void     hide();
   method   void     hide_cursor();
   method   Bool     help();
   method   void     init( HV * profile);
   method   void     insert_behind( Handle view);
   method   void     invalidate_rect( Rect rect);
   c_only   Bool     is_child( Handle possibleOwner);
   method   void     key_event( int command, int code, int key = kbNoKey, int mod = 0, int repeat = 1, Bool post = 0);
   method   void     locate       ( Rect r);
   method   void     lock();
   method   void     mouse_event( int command = cmMouseDown, int button = mbLeft, int mod = 0, int x = 0, int y = 0, Bool dblclk = 0, Bool post = 0);
   method   Handle   next();
   import   SV *     notification_types();
   method   void     post_message( SV * info1, SV * info2);
   c_only   Bool     process_accel( int key);
   import   SV *     profile_default ();
   method   Handle   prev();
   method   void     repaint();
   method   Bool     responsive()                    => apc_widget_is_responsive;
   method   Point    screen_to_client(Point p)       => apc_widget_screen_to_client;
   method   void     scroll( int horiz, int vert, Bool scrollChildren = false);
   method   void     scroll_rect( int horiz, int vert, Rect r, Bool scrollChildren = false);
   method   void     send_to_back();
   method   void     set( HV * profile);
   method   void     setup();
   method   void     show();
   method   void     show_cursor();
   method   void     show_hint();
   method   void     unlock();
   method   void     update_view();
   c_only   void     update_sys_handle( HV * profile);
   method   void     update_delegator();
   method   void     validate_rect( Rect rect)       => apc_widget_validate_rect;

# properties read methods
   method   SV *     get_accel_items();
   method   Handle   get_accel_table();
   method   Color    get_back_color();
   method   int      get_bottom();
   method   Bool     get_brief_keys();
   method   Bool     get_buffered();
   method   Bool     get_capture()                   => apc_widget_is_captured;
   method   Bool     get_clip_owner()                => apc_widget_get_clip_owner;
   method   Rect     get_clip_rect();
   method   Color    get_color();
   method   Color    get_color_index( int index);
   method   Bool     get_current();
   method   Handle   get_current_widget();
   method   Point    get_cursor_pos()                => apc_cursor_get_pos;
   method   Point    get_cursor_size()               => apc_cursor_get_size;
   method   Bool     get_cursor_visible()            => apc_cursor_get_visible;
   static   Font     get_default_font( char * dummy = "");
   static   Font     get_default_popup_font( char * dummy = "");
   method   NPoint   get_design_scale();
   method   SV *     get_drag_pointer();
   method   Bool     get_enabled()                   => apc_widget_is_enabled;
   method   int      get_grow_mode();
   method   Bool     get_first_click()               => apc_widget_get_first_click;
   method   Bool     get_focused()                   => apc_widget_is_focused;
   method   Rect     get_invalid_rect()              => apc_widget_get_invalid_rect;
   method   SV   *   get_handle();
   method   char *   get_hint();
   method   Bool     get_hint_visible();
   method   int      get_height();
   method   long     get_help_context();
   method   int      get_left();
   method   int      get_mouse_state()               => apc_pointer_get_state;
   method   Handle   get_popup();
   method   SV *     get_popup_items();
   method   Bool     get_owner_back_color();
   method   Bool     get_owner_color();
   method   Bool     get_owner_font();
   method   Bool     get_owner_hint();
   method   Bool     get_owner_show_hint();
   method   Bool     get_owner_palette();
   method   Handle   get_parent();
   method   SV *     get_pointer();
   method   Handle   get_pointer_icon();
   method   Point    get_pointer_pos();
   method   Point    get_pointer_hot_spot()          => apc_pointer_get_hot_spot;
   static   Point    get_pointer_size( char * dummy = "");
   method   Bool     get_pointer_visible()           => apc_pointer_get_visible;
   method   Color    get_popup_color( int index);
   method   Font     get_popup_font();
   method   Point    get_pos()                       => apc_widget_get_pos;
   method   Rect     get_rect();
   method   int      get_right();
   method   Bool     get_scale_children();
   method   Bool     get_selectable();
   c_only   Handle   get_selectee();
   method   Bool     get_selected();
   method   Handle   get_selected_widget();
   method   int      get_selecting_buttons();
   method   int      get_shift_state()               => apc_kbd_get_state;
   method   Bool     get_show_hint();
   method   Point    get_size()                      => apc_widget_get_size;
   method   Point    get_size_min();
   method   Point    get_size_max();
   method   Bool     get_sync_paint()                => apc_widget_get_sync_paint;
   method   int      get_tab_order();
   method   Bool     get_tab_stop();
   method   char *   get_text();
   method   Bool     get_transparent()               => apc_widget_get_transparent;
   method   int      get_top();
   method   Bool     get_visible()                   => apc_widget_is_visible;
   method   int      get_widget_class();
   public   void     get_widgets();
   method   int      get_width();

# properties write methods
   method   void     set_accel_items( SV * accelItems);
   method   void     set_accel_table( Handle accelTable);
   method   void     set_back_color( Color color);
   method   void     set_bottom( int bottom);
   method   void     set_brief_keys( Bool briefKeys);
   method   void     set_buffered( Bool buffered);
   method   void     set_capture( Bool capture, Handle confineTo = nilHandle);
   method   void     set_centered( Bool x, Bool y);
   import   void     set_clip_owner( Bool clipOwner);
   method   void     set_clip_rect( Rect clipRect);
   method   void     set_current_widget( Handle widget);
   method   void     set_color( Color color);
   method   void     set_color_index( Color color, int index);
   method   void     set_current( Bool current);
   method   void     set_cursor_pos( int x, int y)     => apc_cursor_set_pos;
   method   void     set_cursor_size( int x, int y)    => apc_cursor_set_size;
   method   void     set_cursor_visible( Bool visible) => apc_cursor_set_visible;
   method   void     set_drag_pointer( SV * dragPointer);
   method   void     set_enabled( Bool enable)         => apc_widget_set_enabled;
   method   void     set_first_click( Bool firstClick) => apc_widget_set_first_click;
   method   void     set_focused( Bool focused);
   method   void     set_font( Font font);
   method   void     set_grow_mode( int flags);
   method   void     set_height( int height);
   method   void     set_help_context(long helpContext);
   method   void     set_hint( char * hint);
   method   void     set_hint_visible( Bool visible);
   method   void     set_left( int left);
   method   void     set_owner_back_color( Bool ownerColor);
   method   void     set_owner_color( Bool ownerColor);
   method   void     set_owner_font( Bool ownerFont);
   method   void     set_owner_hint( Bool ownerHint);
   method   void     set_owner_show_hint( Bool ownerShowHint);
   method   void     set_owner_palette( Bool ownerPalette);
   method   void     set_palette( SV * palette);
   method   void     set_pointer( SV * pointer);
   method   void     set_pointer_icon( Handle icon, Point hotSpot);
   method   void     set_pointer_pos( int x, int y);
   method   void     set_pointer_visible( Bool visible) => apc_pointer_set_visible;
   method   void     set_popup( Handle popup);
   method   void     set_popup_color( Color color, int index);
   method   void     set_popup_font( Font font);
   method   void     set_popup_items( SV * popupItems);
   method   void     set_pos( int x, int y)             => apc_widget_set_pos;
   method   void     set_rect( Rect r);
   method   void     set_right( int right);
   method   void     set_scale_children( Bool scaleChildren);
   method   void     set_selectable( Bool selectable);
   method   void     set_selected( Bool selected);
   method   void     set_selected_widget( Handle widget);
   method   void     set_selecting_buttons( int sb);
   method   void     set_show_hint( Bool showHint);
   method   void     set_size( int x, int y)            => apc_widget_set_size;
   method   void     set_size_min( Point min);
   method   void     set_size_max( Point max);
   import   void     set_sync_paint( Bool syncPaint);
   method   void     set_tab_order( int tabOrder);
   method   void     set_tab_stop( Bool tabStop);
   method   void     set_text( char * text);
   method   void     set_top( int top);
   import   void     set_transparent( Bool transparent);
   method   void     set_visible( Bool visible)         => apc_widget_set_visible;
   method   void     set_widget_class( int widgetClass);
   method   void     set_width( int width);

# virtual event handlers
   event    void     on_change();
   event    void     on_click();
   event    void     on_close();
   event    void     on_colorchanged( int colorIndex);
   event    void     on_disable();
   event    void     on_dragdrop( Handle source, int x, int y);
   event    void     on_dragover( Handle source, int x, int y, int state);
   event    void     on_enable();
   event    void     on_enddrag( Handle target, int x, int y);
   event    void     on_enter();
   event    void     on_fontchanged();
   event    void     on_help();
   event    void     on_hide();
   event    void     on_hint( Bool show);
   event    void     on_keydown( int code, int key, int shiftState, int repeat);
   event    void     on_keyup( int code, int key, int shiftState);
   event    void     on_leave();
   event    void     on_menu( Handle menu, char * variable);
   event    void     on_mouseclick( int button, int shiftState, int x, int y, Bool dbl);
   event    void     on_mousedown( int button, int shiftState, int x, int y);
   event    void     on_mouseup( int button, int shiftState, int x, int y);
   event    void     on_mousemove( int shiftState, int x, int y);
   event    void     on_mousewheel( int shiftState, int x, int y, int z);
   event    void     on_mouseenter( int shiftState, int x, int y);
   event    void     on_mouseleave();
   event    void     on_move( Point oldPos, Point newPos);
   method   void     on_paint( Handle canvas);
   event    void     on_popup( Bool mouseDriven, int x, int y);
   event    void     on_setup();
   event    void     on_show();
   event    void     on_size( Point oldSize, Point newSize);
   event    void     on_translateaccel( int code, int key, int shiftState);
   event    void     on_zorderchanged();
# dynamic event handlers
   SV *   onChange;
   SV *   onClick;
   SV *   onClose;
   SV *   onColorChanged;
   SV *   onDisable;
   SV *   onDragDrop;
   SV *   onDragOver;
   SV *   onEnable;
   SV *   onEndDrag;
   SV *   onEnter;
   SV *   onFontChanged;
   SV *   onHelp;
   SV *   onHide;
   SV *   onHint;
   SV *   onKeyDown;
   SV *   onKeyUp;
   SV *   onLeave;
   SV *   onMenu;
   SV *   onMouseClick;
   SV *   onMouseDown;
   SV *   onMouseUp;
   SV *   onMouseMove;
   SV *   onMouseWheel;
   SV *   onMouseEnter;
   SV *   onMouseLeave;
   SV *   onMove;
   SV *   onPaint;
   SV *   onPopup;
   SV *   onSetup;
   SV *   onShow;
   SV *   onSize;
   SV *   onTranslateAccel;
   SV *   onZOrderChanged;
}

