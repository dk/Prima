#  Created by Anton Berezin  <tobez@plab.ku.dk>
package main;

package Prima::Application;
use strict;
use warnings;
use Prima::Classes;
use vars qw($uses);

sub import {
	shift;
	return if $^C;
	my %profile = ( name => q(Prima), @_);
	$::application ||= Prima::Application-> create( %profile);
	$uses++;
}

1;

__END__

=pod

=head1 NAME

Prima::Application - root of widget objects hierarchy

=head1 DESCRIPTION

Prima::Application class serves as a hierarchy root for all objects with
child-owner relationship. All toolkit objects, existing with non-null owner
property, belong by their top-level parental relationship to Prima::Application
object. There can be only one instance of Prima::Application class at a time.

=head1 SYNOPSIS

	use Prima;
	use Prima::Application;

or

	use Prima qw(Application);

	Prima::MainWindow-> create();

	run Prima;

=head1 USAGE

Prima::Application class, and its only instance are treated specially
throughout the toolkit. The object instance is contained in

	$::application

scalar, defined in I<Prima.pm> module.  The application instance must be
created whenever widget and window, or event loop functionality is desired.
Usually

	use Prima::Application;

code is enough, but I<$::application> can also be assigned explicitly. The
'use' syntax has advantage as more resistant to eventual changes in the toolkit
design.  It can also be used in conjunction with custom parameters hash, alike
the general create() syntax:

	use Prima::Application name => 'Test application', icon => $icon;

In addition to this functionality Prima::Application is also a wrapper to a set
of system functions, not directly related to object classes. This functionality
is generally explained in L<"API">.

=head2 Inherited functionality

Prima::Application is a descendant of Prima::Widget, but it is designed so
because their functional outliers are closest to each other.
Prima::Application does not strictly conform ( in OO sense ) to any of the
built-in classes. It has methods copied from both Prima::Widget and
Prima::Window at one time, and the inherited Prima::Widget methods and
properties function differently.  For example, C<::origin>, a property from
Prima::Widget, is also implemented in Prima::Application, but returns always
(0,0), an expected but not much usable result.  C<::size>, on the contrary,
returns the extent of the screen in pixels.  There are few properties,
inherited from Prima::Widget, which return actual, but uninformative results, -
C<::origin> is one of those, but same are C<::buffered>, C<::clipOwner>,
C<::enabled>, C<::growMode>, C<::owner> and owner-inheritance properties,
C<::selectable>, C<::shape>, C<::syncPaint>, C<::tabOrder>, C<::tabStop>,
C<::transparent>, C<::visible>.  To this group also belongs C<::modalHorizon>,
Prima::Window class property, but defined for consistency and returning always
1.  Other methods and properties, like C<::size>, that provide different
functionality are described in L<"API">.

=head2 Global functionality

Prima::Application is a wrapper to functionality, that is not related to one or
another class clearly.  A notable example, paint mode, which is derived from
Prima::Drawable class, allows painting on the screen, overwriting the graphic
information created by the other programs. Although being subject to
begin_paint()/end_paint() brackets, this functionality can not be attached to a
class-shared API, an therefore is considered global. All such functionality is
gathered in the Prima::Application class.

These topics enumerated below, related to the global scope, but occupying more
than one method or property - such functions described in L<"API">.

=over

=item Painting

As stated above, Prima::Application provides interface to the on-screen
painting. This mode is triggered by begin_paint()/end_paint() methods pair, and
the other pair, begin_paint_info()/end_paint_info() triggers the information
mode. This three-state paint functionality is more thoroughly described in
L<Prima::Drawable>.

The painting on the screen surfaces under certain environments (XQuartz,
XWayland) is either silently ignored or results in an error. There,
C<begin_paint> will return a false value (C<begin_paint_info> though returns
true).

=item Hint

$::application hosts a special Prima::HintWidget class object, accessible via
C<get_hint_widget()>, but with color and font functions aliased (
C<::hintColor>, C<::hintBackColor>, C<::hintFont> ).

This widget serves as a hint label, floating over widgets if the mouse pointer
hovers longer than C<::hintPause> milliseconds.

Prima::Application internally manages all hint functionality.  The hint widget
itself, however, can be replaced before application object is created, using
C<::hintClass> create-only property.

=item Printer

Result of L<get_printer> method points to an automatically created printer
object, responsible for the system-driven printing. Depending on the operating
system, it is either Prima::Printer, if the system provides GUI printing
capabilities, or generic Prima::PS::Printer, the PostScript document interface.

See L<Prima::Printer> for details.

=item Clipboard

$::application hosts set of Prima::Clipboard objects, created automatically to
reflect the system-provided clipboard IPC functionality. Their number depends
on the system, - under X11 environment there is three clipboard objects, and
only one under Win32.

These are no methods to access these clipboard objects, except fetch() ( or,
the indirect name calling ) - the clipboard objects are named after the system
clipboard names, which are returned by
Prima::Clipboard::get_standard_clipboards.

The default clipboard is named I<Clipboard>, and is accessible via

	my $clipboard = $::application-> Clipboard;

code.

See L<Prima::Clipboard> for details.

=item Help subsystem

The toolkit has a built-in help viewer, that understands perl's native POD (
plain old documentation ) format.  Whereas the viewer functionality itself is
part of the toolkit, and resides in C<Prima::HelpViewer> module, any custom
help viewing module can be assigned. Create-only C<Prima::Application>
properties C<::helpClass> and C<::helpModule> can be used to set these options.

C<Prima::Application> provides two methods for communicating with the help
viewer window: C<open_help()> opens a selected topic in the help window, and
C<close_help()> closes the window.

=item System-dependent information

A complex program will need eventually more information than the toolkit
provides. Or, knowing the toolkit boundaries in some platforms, the program
changes its behavior accordingly. Both these topics are facilitated by extra
system information, returned by Prima::Application methods.
C<get_system_value> returns a system value for one of C<sv::XXX> constants, so
the program can read the system-specific information. As well as
C<get_system_info> method, that returns the short description of the system, it
is the portable call.  To the contrary, C<sys_action> method is a wrapper to
system-dependent functionality, called in non-portable way. This method is
never used within the toolkit, and its usage is discouraged, primarily because
its options do not serve the toolkit design, are subject to changes and cannot
be relied upon.

=back

=head1 API

=head2 Properties

=over

=item autoClose BOOLEAN

If set to 1, issues C<close()> after the last top-level window is destroyed.
Does not influence anything if set to 0.

This feature is designed to help with general 'one main window' application
layouts.

Default value: 0

=item icon OBJECT

Holds the icon object, associated with the application.  If C<undef>, a
system-provided default icon is assumed.  Prima::Window object instances
inherit the application icon by default.

=item insertMode BOOLEAN

A system boolean flag, showing whether text widgets through the system should
insert ( 1 ) or overwrite ( 0 ) text on user input. Not all systems provide the
global state of the flag.

=item helpClass STRING

Specifies a class of object, used as a help viewing package.  The default value
is Prima::HelpViewer.

Run-time changes to the property do not affect the help subsystem until
C<close_help> call is made.

=item helpModule STRING

Specifies a perl module, loaded indirectly when a help viewing call is made via
C<open_help>.  Used when C<::helpClass> property is overridden and the new
class is contained in a third-party module.

Run-time changes to the property do not affect the help subsystem until
C<close_help> call is made.

=item hintClass STRING

Create-only property.

Specifies a class of widget, used as the hint label.

Default value: Prima::HintWidget

=item hintColor COLOR

An alias to foreground color property for the hint label widget.

=item hintBackColor COLOR

An alias to background color property for the hint label widget.

=item hintFont %FONT

An alias to font property for the hint label widget.

=item hintPause TIMEOUT

Selects the timeout in milliseconds before the hint label is shown
when the mouse pointer hovers over a widget.

=item modalHorizon BOOLEAN

A read-only property. Used as a landmark for
the lowest-level modal horizon.
Always returns 1.

=item palette [ @PALETTE ]

Used only within paint and information modes.  Selects solid colors in a system
palette, as many as possible.  PALETTE is an array of integer triplets, where
each is red, green, and blue component, with intensity range from 0 to 255.

=item printerClass STRING

Create-only property.

Specifies a class of object, used as a printer.  The default value is
system-dependent, but is either C<Prima::Printer> or C<Prima::PS::Printer>.

=item printerModule STRING

Create-only property.

Specifies a perl module, loaded indirectly before a printer object of
C<::printerClass> class is created.  Used when C<::printerClass> property is
overridden and the new class is contained in a third-party module.

=item pointerVisible BOOLEAN

Governs the system pointer visibility.  If 0, hides the pointer so it is not
visible in all system windows. Therefore this property usage must be considered
with care.

=item size WIDTH, HEIGHT

A read-only property.

Returns two integers, width and height of the screen.

=item showHint BOOLEAN

If 1, the toolkit is allowed to show the hint label over a widget. If 0, the
display of the hint is forbidden. In addition to functionality of C<::showHint>
property in Prima::Widget, Prima::Application::showHint is another layer of
hint visibility control - if it is 0, all hint actions are disabled,
disregarding C<::showHint> value in widgets.

=item uiScaling FLOAT

The property contains an advisory multiplier factor, useful for UI elements
that have a fixed pixel value, but that would like to be represented in a
useful manner when the display resolution is too high (on modern High-DPI
displays) or too low (on ancient monitors).

By default, it acquires the system display resolution, and sets the scaling
factor so that when the DPI is 96 it is 1.0, 192 it is 2.0, etc. The increase
step is 0.25, so that bitmaps may look not that distorted. However, when the
value is manually set, there is no such step, any value can be set.

See also: L<Prima/Stress>.

=item wantUnicodeInput BOOLEAN

Selects if the system is allowed to generate key codes in unicode.  Returns the
effective state of the unicode input flag, which cannot be changed if perl or
operating system do not support UTF8.

If 1, C<Prima::Clipboard::text> property may return UTF8 text from system
clipboards is available.

Default value: 0

=back

=head2 Events

=over

=item CopyImage $CLIPBOARD, $IMAGE

The notification stores C<$IMAGE> in clipboard.

=item CopyText $CLIPBOARD, $TEXT

The notification stores C<$TEXT> in clipboard.

=item Idle

Called when the event loop handled all pending events, and
is about to sleep waiting for more.

=item PasteImage $CLIPBOARD, $$IMAGE_REF

The notification queries C<$CLIPBOARD> for image content and stores in
C<$$IMAGE_REF>. Default action is that C<'Image'> format is queried.  On unix,
encoded formats C<'image/bmp'>, C<'image/png'> etc are queried if the default
C<'Image'> is not found.

The C<PasteImage> mechanism is devised to read images from clipboard in GTK
environment.

=item PasteText $CLIPBOARD, $$TEXT_REF

The notification queries C<$CLIPBOARD> for text content and stores in
C<$$TEXT_REF>. Default action is that C<'Text'> format is queried if
C<wantUnicodeInput> is unset. Otherwise, C<'UTF8'> format is queried
beforehand.

The C<PasteText> mechanism is devised to ease defining text unicode/ascii
conversion between clipboard and standard widgets, in a standard way.

=back

=head2 Methods

=over

=item add_startup_notification @CALLBACK

CALLBACK is an array of anonymous subs, which is executed when
Prima::Application object is created. If the application object is already
created during the call, CALLBACKs called immediately.

Useful for add-on packages initialization.

=item begin_paint

Enters the enabled ( active paint ) state, returns success flag.  Once the
object is in enabled state, painting and drawing methods can perform write
operations on the whole screen.

=item begin_paint_info

Enters the information state, returns success flag.  The object information
state is same as enabled state ( see C<begin_paint()>), except that painting
and drawing methods are not permitted to change the screen.

=item close

Issues a system termination call, resulting in calling C<close> for all
top-level windows. The call can be interrupted by these, and thus canceled. If
not canceled, stops the application event loop.

=item close_help

Closes the help viewer window.

=item end_paint

Quits the enabled state and returns application object to the normal state.

=item end_paint_info

Quits the information state and returns application object to the normal state.

=item font_encodings

Returns array of encodings, represented by strings, that are recognized by the
system and available for at least one font. Each system provides different sets
of encoding strings; the font encodings are not portable.

=item fonts NAME = '', ENCODING = ''

Returns hash of font hashes ( see L<Prima::Drawable/Fonts> ) describing fonts
of NAME font family and of ENCODING. If NAME is '' or C<undef>, returns one
fonts hash for each of the font families that match the ENCODING string. If
ENCODING is '' or C<undef>, no encoding match is performed.  If ENCODING is not
valid ( not present in C<font_encodings> result), it is treated as if it was ''
or C<undef>.

In the special case, when both NAME and ENCODING are '' or C<undef>, each font
metric hash contains element C<encodings>, that points to array of the font
encodings, available for the fonts of NAME font family.

=item get_active_window

Returns object reference to a currently active window, if any, that belongs to
the program. If no such window exists, C<undef> is returned.

The exact definition of 'active window' is system-dependent, but it is
generally believed that an active window is the one that has keyboard focus on
one of its children widgets.

=item get_caption_font

Returns a title font, that the system uses to draw top-level window captions.
The method can be called with a class string instead of an object instance.

=item get_default_cursor_width

Returns width of the system cursor in pixels.  The method can be called with a
class string instead of an object instance.

=item get_default_font

Returns the default system font.  The method can be called with a class string
instead of an object instance.

=item get_default_scrollbar_metrics

Returns dimensions of the system scrollbars - width of the standard vertical
scrollbar and height of the standard horizon scrollbar.  The method can be
called with a class string instead of an object instance.

=item get_dnd_clipboard

Returns the predefined special clipboard used as a proxy for drag and drop
interactions.

See also: C<Widget/Drag and drop>, C<Clipboard/is_dnd>.

=item get_default_window_borders BORDER_STYLE = bs::Sizeable

Returns width and height of standard system window border decorations for one
of C<bs::XXX> constants.  The method can be called with a class string instead
of an object instance.

=item get_focused_widget

Returns object reference to a currently focused widget, if any, that belongs to
the program. If no such widget exists, C<undef> is returned.

=item get_fullscreen_image

Syntax sugar for grabbing whole screen as in

   $::application->get_image( 0, 0, $::application->size)

(MacOSX/XQuartz: get_image() does not grab all screen bits, but
C<get_fullscreen_image> does (given Prima is compiled with Cocoa library)).

=item get_hint_widget

Returns the hint label widget, attached automatically to Prima::Application
object during startup. The widget is of C<::hintClass> class, Prima::HintWidget
by default.


=item get_image X_OFFSET, Y_OFFSET, WIDTH, HEIGHT

Returns Prima::Image object with WIDTH and HEIGHT dimensions filled with
graphic content of the screen, copied from X_OFFSET and Y_OFFSET coordinates.
If WIDTH and HEIGHT extend beyond the screen dimensions, they are adjusted.  If
the offsets are outside screen boundaries, or WIDTH and HEIGHT are zero or
negative, C<undef> is returned.

Note: When running on MacOSX under XQuartz, the latter does not give access to
the whole screen, so the function will not be able to grab top-level menu bar.
This problem is addressed in C<get_fullscreen_image>.

=item get_indents

Returns 4 integers that corresponds to extensions of eventual desktop
decorations that the windowing system may present on the left, bottom, right,
and top edges of the screen. For example, for win32 this reports the size of
the part of the scraan that windows taskbar may occupies, if any.

=item get_printer

Returns the printer object, attached automatically to Prima::Application
object. The object is of C<::printerClass> class.

=item get_message_font

Returns the font the system uses to draw the message text.  The method can be
called with a class string instead of an object instance.

=item get_modal_window MODALITY_TYPE = mt::Exclusive, TOPMOST = 1

Returns the modal window, that resides on an end of a modality chain.
MODALITY_TYPE selects the chain, and can be either C<mt::Exclusive> or
C<mt::Shared>. TOPMOST is a boolean flag, selecting the lookup direction; if it
is 1, the 'topmost' window is returned, if 0, the 'lowest' one ( in a simple
case when window A is made modal (executed) after modal window B, the A window
is the 'topmost' one ).

If a chain is empty C<undef> is returned. In case when a chain consists of just
one window, TOPMOST value is apparently irrelevant.

=item get_monitor_rects

Returns set of rects in format [X,Y,WIDTH,HEIGHT] identifying monitor
configurations. Currently works under X11 only.

=item get_scroll_rate

Returns two integer values of two system-specific scrolling timeouts. The first
is the initial timeout, that is applied when the user drags the mouse from a
scrollable widget ( a text field, for example ), and the widget is about to
scroll, but the actual scroll is performed after the timeout is expired. The
second is the repetitive timeout, - if the dragging condition did not change,
the scrolling performs automatically after this timeout. The timeout values are
in milliseconds.

=item get_system_info

Returns a hash with information about the system.  The hash result contains the
following keys:

=over

=item apc

One of C<apc::XXX> constants, reflecting the platform.
Currently, the list of the supported platforms is:

	apc::Win32
	apc::Unix

=item gui

One of C<gui::XXX> constants, reflecting the graphic
user interface used in the system:

	gui::Default
	gui::PM
	gui::Windows
	gui::XLib
	gui::GTK

=item guiDescription

Description of graphic user interface,
returned as an arbitrary string.

=item system

An arbitrary string, representing the operating
system software.

=item release

An arbitrary string, reflecting the OS version
information.

=item vendor

The OS vendor string

=item architecture

The machine architecture string

=back

The method can be called with a class string instead of an object instance.

=item get_system_value

Returns the system integer value, associated with one
of C<sv::XXX> constants. The constants are:

	sv::YMenu            - height of menu bar in top-level windows
	sv::YTitleBar        - height of title bar in top-level windows
	sv::XIcon            - width and height of main icon dimensions,
	sv::YIcon              acceptable by the system
	sv::XSmallIcon       - width and height of alternate icon dimensions,
	sv::YSmallIcon         acceptable by the system
	sv::XPointer         - width and height of mouse pointer icon
	sv::YPointer           acceptable by the system
	sv::XScrollbar       - width of the default vertical scrollbar
	sv::YScrollbar       - height of the default horizontal scrollbar
								( see get_default_scrollbar_metrics() )
	sv::XCursor          - width of the system cursor
								( see get_default_cursor_width() )
	sv::AutoScrollFirst  - the initial and the repetitive
	sv::AutoScrollNext     scroll timeouts
								( see get_scroll_rate() )
	sv::InsertMode       - the system insert mode
								( see insertMode )
	sv::XbsNone          - widths and heights of the top-level window
	sv::YbsNone            decorations, correspondingly, with borderStyle
	sv::XbsSizeable        bs::None, bs::Sizeable, bs::Single, and
	sv::YbsSizeable        bs::Dialog.
	sv::XbsSingle          ( see get_default_window_borders() )
	sv::YbsSingle
	sv::XbsDialog
	sv::YbsDialog
	sv::MousePresent     - 1 if the mouse is present, 0 otherwise
	sv::MouseButtons     - number of the mouse buttons
	sv::WheelPresent     - 1 if the mouse wheel is present, 0 otherwise
	sv::SubmenuDelay     - timeout ( in ms ) before a sub-menu shows on
								an implicit selection
	sv::FullDrag         - 1 if the top-level windows are dragged dynamically,
								0 - with marquee mode
	sv::DblClickDelay    - mouse double-click timeout in milliseconds
	sv::ShapeExtension   - 1 if Prima::Widget::shape functionality is supported,
								0 otherwise
	sv::ColorPointer     - 1 if system accepts color pointer icons.
	sv::CanUTF8_Input    - 1 if system can generate key codes in unicode
	sv::CanUTF8_Output   - 1 if system can output utf8 text
	sv::CompositeDisplay - 1 if system uses double-buffering and alpha composition for the desktop,
	                       0 if it doesn't, -1 if unknown
	sv::LayeredWidgets   - 1 if system supports layering
	sv::DWM              - 1 if system supports DWM API
	sv::FixedPointerSize - 0 if system doesn't support arbitrary sized pointers and will resize custom icons to the system size

The method can be called with a class string instead of an object instance.

=item get_widget_from_handle HANDLE

HANDLE is an integer value of a toolkit widget. It is usually passed to the
program by other IPC means, so it returns the associated widget.  If no widget
is associated with HANDLE, C<undef> is returned.

=item get_widget_from_point X_OFFSET, Y_OFFSET

Returns the widget that occupies screen area under (X_OFFSET,Y_OFFSET)
coordinates. If no toolkit widget are found, C<undef> is returned.

=item go

The main event loop. Called by

run Prima;

standard code. Returns when the program is about to terminate, or if the
exception was signaled. In the latter case, the loop can be safely re-started.

=item lock

Effectively blocks the graphic output for all widgets.  The output can be
restored with C<unlock()>.

=item load_font FONTNAME

Registers font resource in system-specific format. The resource is freed after
prgram ends.

Notes for win32: To add a font whose information comes from several resource
files, point FONTNAME to a string with the file names separated by a C<|> - for
example, C< abcxxxxx.pfm | abcxxxxx.pfb >.

Notes for unix: available only when Prima is compiled with fontconfig and Xft .

Returns number of font resources added.

=item open_help TOPIC

Opens the help viewer window with TOPIC string in
link POD format ( see L<perlpod> ) - the string is treated
as "manpage/section", where 'manpage' is the file with POD
content and 'section' is the topic inside the manpage.

=item sync

Synchronizes all pending requests where there are any. Is an effective
C<XSync(false)> on X11, and is a no-op otherwise.

=item sys_action CALL

CALL is an arbitrary string of the system service name and the parameters to
it.  This functionality is non-portable, and its usage should be avoided.  The
system services provided are not documented and subject to change. The actual
services can be looked in the toolkit source code under I<apc_system_action>
tag.

=item unlock

Unblocks the graphic output for all widgets, previously locked with C<lock()>.

=item yield $wait_for_event=0

An event dispatcher, called from within the event loop.
If the event loop can be schematized, then in

	while ( application not closed ) {
		yield
	}

draft yield() is the only function, called repeatedly within the event loop.
yield(0) call shouldn't be used to organize event loops, but it can be employed
to process stacked system events explicitly, to increase responsiveness of a
program, for example, inside a long calculation cycle.

yield(1) though is adapted exactly for external implementation of event loops;
it does exactly the same as yeild(0), but if there are no events, it sleeps
until there comes at least one, processes it, and then returns. The return
value is 0 if the application doesn't need more event processins, because of
shutting down.  The corresponding code will be

	while ( yield(1)) {
	    ...
	}

but in turn, this call cannot be used for UI responsiveness inside tight cycles.

The method can be called with a class string instead of an object instance;
however, the $::application object must be initialized.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Object>, L<Prima::Widget>, L<Prima::Window>

=cut
