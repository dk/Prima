#
#  Main widget description
#

local @DNDResp
{
	int action;
	Handle counterpart;
}

object Prima::Widget( Prima::Drawable)
{
#	user variables
	Handle    accelTable;	            # accelerator table
	Handle    currentWidget;	    # current widget pointer
	int       cursorLock;               # cursor lock count
	NPoint    designScale;              # font width and height for view design
	char   *  dndAware;                 # NULL for no, "1" for all formats, other for selected
	char   *  helpContext;              # help context string
	SV     *  hint;                     # hint tag text
	int       lockCount;                # lock count
	Font      popupFont;                # font for popup menu
	ColorSet  popupColor;               # colors for popup menu
	Handle    popupMenu;                # popup menu by default
	int       pointerType;              # pointer type, crXXX
	Point     pos;                      # last position
	int       selectingButtons;         # which mouse buttons can select a view
	Point     sizeMin;                  # default min size
	Point     sizeMax;                  # default max size
	int       tabOrder;                 # tab ordering number
	SV     *  text;                     # caption text
	int       widgetClass;              # predefined system class
	List      widgets;		       # list of child widgets
	Handle *  enum_lists;               # list of currently running enumerators

# 	geometry managers data
#	common:
	int       geometry;                 # geometry manager, gtXXX
	Point     geomSize;                 # requested size, common to all managers
	GeomInfo  geomInfo;                 # managers options
#	growMode:
	int       growMode;                 # growing flags, see gmGrowXXX
	Point     virtualSize;              # fictive size not aware of size limits
#	pack
	Handle    packSlaves;               # first pack slave in linked list
#	place
	Handle    placeSlaves;              # first place slave in linked list

#	properties
	property  SV *   accelItems;
	property  Handle accelTable;
	property  Bool   autoEnableChildren;
	property  Color  backColor;
	property  int    bottom;
	property  Bool   briefKeys;
	property  Bool   buffered;
	property  Bool   clipChildren;
	property  Bool   clipOwner;
	property  Color  color;
	property  Color  colorIndex( int index);
	property  Bool   current;
	property  Handle currentWidget;
	property  Point  cursorPos;
	property  Point  cursorSize;
	property  Bool   cursorVisible;
	property  NPoint designScale;
	property  SV*    dndAware;
	property  Bool   enabled;
	property  int    geometry;
	property  int    geomHeight;
	property  Point  geomSize;
	property  int    geomWidth;
	property  int    growMode;
	property  Bool   firstClick;
	property  Bool   focused;
	property  SV *   helpContext;
	property  int    hintVisible;
	property  int    left;
	property  Bool   layered;
	property  Handle popup;
	property  SV *   popupItems;
	property  Bool   ownerBackColor;
	property  Bool   ownerColor;
	property  Bool   ownerFont;
	property  Bool   ownerHint;
	property  Bool   ownerShowHint;
	property  Bool   ownerPalette;
	property  Point  origin;
	property  SV  *  packInfo;
	property  SV  *  palette;
	property  SV  *  placeInfo;
	property  Point  pointerHotSpot;
	property  Handle pointerIcon;
	property  Point  pointerPos;
	property  int    pointerType;
	property  Color  popupColorIndex( int index);
	property  Bool   packPropagate;
	property  Rect   rect;
	property  int    right;
	property  Bool   scaleChildren;
	property  Bool   selectable;
	property  Bool   selected;
	property  Handle selectedWidget;
	property  int    selectingButtons;
	property  Handle shape;
	property  Bool   showHint;
	property  Point  size;
	property  Point  sizeMax;
	property  Point  sizeMin;
	property  Bool   syncPaint;
	property  int    tabOrder;
	property  Bool   tabStop;
	property  int    top;
	property  Bool   transparent;
	property  Bool   visible;
	property  int    widgetClass;

#	direct call methods
	method   void     attach( Handle objectHandle);
	method   Bool     begin_paint();
	method   Bool     begin_paint_info();
	method   void     bring_to_front();
	method   Bool     can_close();
	c_only   Bool     can_propagate_key();
	method   void     cleanup();
	public   void     client_to_screen();
	method   Bool     close();
	method   void     detach( Handle objectHandle, Bool kill);
	method   void     done();
	method   DNDResp  dnd_start(int dnd_actions = dndCopy, Bool default_pointers = 1);
	method   void     end_paint();
	method   void     end_paint_info();
	method   Bool     exposed()                       => apc_widget_is_exposed;
	static   SV      *fetch_resource( char *className, char *name, char *classRes, char *res, Handle owner, int resType = frString);
	method   Handle   first();
	c_only   Handle   first_that( void * actionProc, void * params);
	c_only   void     handle_event ( PEvent event);
	method   void     hide();
	method   void     hide_cursor();
	import   Bool     help();
	method   void     init( HV * profile);
	method   void     insert_behind( Handle view);
	method   void     invalidate_rect( Rect rect);
	c_only   Bool     is_child( Handle possibleOwner);
	method   Bool     is_surface_layered()                    => apc_widget_surface_is_layered;
	method   void     key_event( int command, int code, int key = kbNoKey, int mod = 0, int repeat = 1, Bool post = 0);
	method   Handle   last();
	method   Bool     lock();
	method   Color    map_color( Color color) => apc_widget_map_color;
	method   void     mouse_event( int command = cmMouseDown, int button = mbLeft, int mod = 0, int x = 0, int y = 0, int nth = 0, Bool post = 0);
	method   Handle   next();
	method   Handle   next_tab( Bool forward = true);
	method   Handle   next_positional( int dx, int dy);
	import   SV *     notification_types();
	method   void     post_message( SV * info1, SV * info2);
	c_only   Bool     process_accel( int key);
	import   SV *     profile_default ();
	method   Handle   prev();
	method   void     repaint();
	method   Bool     responsive()                    => apc_widget_is_responsive;
	public   void     screen_to_client();
	public   int      scroll( int dx, int dy, Rect *confine, Rect *clip, Bool withChildren);
	method   void     send_to_back();
	method   void     set( HV * profile);
	method   void     setup();
	method   void     show();
	method   void     show_cursor();
	method   Bool     showing()                       => apc_widget_is_showing;
	method   Bool     unlock();
	method   void     update_view();
	c_only   void     update_sys_handle( HV * profile);
	c_only   Bool     validate_owner( Handle * newOwner, HV * profile);
	method   Bool     validate_rect( Rect rect)       => apc_widget_validate_rect;

#	properties read methods
	method   Bool     get_capture()                   => apc_widget_is_captured;
	static   Font     get_default_font( char * dummy = "");
	static   Font     get_default_popup_font( char * dummy = "");
	method   Rect     get_invalid_rect()              => apc_widget_get_invalid_rect;
	method   SV   *   get_handle();
	method   SV   *   get_hint();
	method   Bool     get_locked();
	method   int      get_mouse_state()               => apc_pointer_get_state;
	public   void     get_pack_slaves();
	method   Handle   get_parent();
	method   SV   *   get_parent_handle();
	public   void     get_place_slaves();
	static   Point    get_pointer_size( char * dummy = "");
	method   Font     get_popup_font();
	c_only   Handle   get_selectee();
	method   int      get_shift_state()               => apc_kbd_get_state;
	method   Point    get_virtual_size();
	public   void     get_widgets();
	method   SV   *   get_text();

#	properties write methods
	method   Bool     set_capture( Bool capture, Handle confineTo = NULL_HANDLE);
	method   void     set_centered( Bool x, Bool y);
	method   void     set_font( Font font);
	method   void     set_hint( SV * hint);
	method   void     set_popup_font( Font font);
	method   void     set_text( SV * text);

#	other
	method   void     on_paint( SV * canvas);
}
