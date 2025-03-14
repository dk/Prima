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

Prima::Application - the root of the widget hierarchy

=head1 DESCRIPTION

The Prima::Application class serves as the hierarchy root for the majority of
Prima objects.  All toolkit widgets are ultimately owned by the application
object. There can be only one instance of the Prima::Application class at a time.

=head1 SYNOPSIS

	use Prima qw(Application);
	Prima::MainWindow->new();
	Prima->run;

=head1 USAGE

Prima::Application class and its only instance are treated is a special way
in the toolkit's paradigm. Its only object instance is stored in the

	$::application

scalar, defined in I<Prima.pm> module.  The application instance must be
created whenever a widget, window, or event loop functionality is needed.
Usually the

	use Prima::Application;

or

	use Prima qw(Application);

code is enough, but I<$::application> can also be created and assigned explicitly. The
'use' syntax has an advantage as more resistant to eventual changes in the toolkit
design. It can also be used in conjunction with custom parameters hash like
the new() syntax:

	use Prima::Application name => 'Test application', icon => $icon;

In addition to this functionality, Prima::Application is also a wrapper to a set
of system functions, not directly related to the object classes. This functionality
is generally explained in L<"API">.

=head2 Inherited functionality

Prima::Application is a descendant of Prima::Widget but does not conform
strictly ( in the OO sense ) to any of the built-in classes. It has methods from
both Prima::Widget and Prima::Window, also, the methods inherited from the
Prima::Widget class may work quite differently.  For example, the C<::origin>
property from Prima::Widget is also implemented in Prima::Application, but
always returns (0,0), an expected but not much usable result.  The C<::size>
property, on the contrary, returns the extent of the screen in pixels.  There
are a few properties inherited from Prima::Widget, which return actual but
uninformative results, - C<::origin> is one of those, but there are several
others.  The methods and properties, that are like C<::size> providing
different functionality, are described separately in L<"API">.

=head2 Global functionality

Prima::Application is a wrapper to a set of unrelated functions that do not
belong to other classes.  A notable example, the painting functionality that is
inherited from the Prima::Drawable class, allows drawing on the screen,
possibly overwriting the graphic information created by the other programs.
Although it is still a subject to the begin_paint()/end_paint() brackets, this
functionality does not belong to a single object and is considered global.

=over

=item Painting

As stated above, the Prima::Application class provides an interface to the
on-screen painting. This mode is triggered by the begin_paint()/end_paint()
methods, while the other pair, begin_paint_info()/end_paint_info() triggers the
information mode. This three-state paint functionality is more thoroughly
described in L<Prima::Drawable>.

The painting on the screen surfaces under certain environments (XQuartz,
XWayland) is either silently ignored or results in an error. There,
C<begin_paint> may return a false value (C<begin_paint_info> though always
true).

=item Hints

C<$::application> hosts a special C<Prima::HintWidget> class object, accessible
via C<get_hint_widget()>, but with its color and font functions aliased ( see
C<::hintColor>, C<::hintBackColor>, C<::hintFont> ).

This widget serves as a hint label, floating over other widgets if the mouse pointer
hovers longer than C<::hintPause> milliseconds.

Prima::Application internally manages all of the hint functionality.  The hint widget
itself, however, can be replaced before the application object is created, using
the C<::hintClass> create-only property.

Also see: L<set_hint_action>

=item Printer

The result of the L<get_printer> method points to an automatically created printer
object, responsible for the system printing. Depending on the operating
system, it is either Prima::Printer, if the system provides GUI printing
capabilities, or generic Prima::PS::Printer, the PostScript/PDF document interface.

See L<Prima::Printer> for details.

=item Clipboard

C<$::application> hosts a set of Prima::Clipboard objects created automatically to
reflect the system-provided clipboard IPC functionality. Their number depends
on the system, - under the X11 environment, there are three clipboard objects, and
one under Win32.

There are no specific methods to access these clipboard objects, except bring()
( or the indirect name call ); the clipboard objects are named after the
system clipboard names, which are returned by the
Prima::Clipboard::get_standard_clipboards method.

The default clipboard is named I<Clipboard>, and is accessible via the

	my $clipboard = $::application-> Clipboard;

call.

See L<Prima::Clipboard> for details.

=item Help subsystem

The toolkit has a built-in help viewer, that understands perl's native POD (
plain old documentation ) format.  Whereas the viewer functionality itself is a
part of the toolkit that resides in the C<Prima::HelpViewer> module, any custom
help viewing module can be assigned. The create-only C<Prima::Application>
properties C<::helpClass> and C<::helpModule> can be used to set these options.

C<Prima::Application> provides two methods for communicating with the help
viewer window: C<open_help()> opens a selected topic in the help window, and
C<close_help()> closes the window.

=item System-dependent information

A complex program will need eventually more information than the toolkit
provides. Knowing the toolkit boundaries in some platforms, the program
may change its behavior accordingly. Both these topics are facilitated by extra
system information returned by Prima::Application methods.  The
C<get_system_value> method returns a system-defined value for each of the
C<sv::XXX> constants, so the program can read the system-specific information.
Another method C<get_system_info> returns the short description of
the system that augments perl's C< $^O > variable.

The C<sys_action> method is a wrapper to system-dependent functionality that is
called in a non-portable way. This method is rarely used in the toolkit,
its usage is discouraged, primarily because its options do not serve the
toolkit design, its syntax is subject to changes, and cannot be relied upon.

=item Exceptions and signals

By default Prima doesn't track exceptions caused by C<die>, C<warn>, and signals.
Currently, it is possible to enable a GUI dialog tracking the C<die> exceptions,
by either operating the boolean C<guiException> property or using the

   use Prima qw(sys::GUIException)

syntax.

If you need to track signals or warnings you may do so by using standard perl
practices. It is though not advisable to call Prima interactive methods
directly inside signal handlers but use a minimal code instead. F.ex. code that
would ask whether the user wants to quit would look like this:

   use Prima qw(Utils MsgBox);
   $SIG{INT} = sub {
      Prima::Utils::post( sub {
          exit if message_box("Got Ctrl+C", "Do you really want to quit?", mb::YesNo) == mb::Yes;
      });
   };

and if you want to treat all warnings as potentially fatal, like this:

   use Prima qw(Utils MsgBox);
   $SIG{__WARN__} = sub {
      my ($warn, $stack) = ($_[0], Carp::longmess);
      Prima::Utils::post( sub {
	  exit if $::application && Prima::MsgBox::signal_dialog("Warning", $warn, $stack) == mb::Abort;
      });
   };

See also: L<Die>, L<Prima::MsgBox/signal_dialog>

=back

=head1 API

=head2 Properties

=over

=item autoClose BOOLEAN

If set to 1, issues C<close()> after the last top-level window is destroyed.
Does not influence anything if set to 0.

This feature is designed to help with generic 'one main window' application
layouts.

Default value: 0

=item guiException BOOLEAN

If set to 1, when a C<die> exception is thrown, displays a system message dialog.
allowing the user to choose the course of action -- to stop, to continue, etc.

Is 0 by default.

Note that the exception is only handled inside the C<Prima::run> and
C<Prima::Dialog::execute> calls; if there is a call to f ex
C<Prima::Window::execute> or a manual event loop run with C<yield>, the signal
dialog will not be shown. One needs to explicitly call C<<
$::application->notify(Die => $@) >> and check the notification result to
decide whether to propagate the exception or not.

The alternative syntax for setting C<guiException> to 1 is the

   use Prima::sys::GUIException;

or

   use Prima qw(sys::GUIException);

statement.

If for some reason an exception is thrown during dialog execution, it will not be handled
by Prima but by the current C< $SIG{__DIE__} > handler.

See also L<Prima::MsgBox/signal_dialog> .

=item icon OBJECT

Holds the icon object associated with the application.  If C<undef>, the
system-provided default icon is assumed.  Prima::Window objects
inherit this application icon by default.

=item insertMode BOOLEAN

The system boolean flag signaling whether text widgets through the system should
insert ( 1 ) or overwrite ( 0 ) text on user input. Not all systems provide the
global state of the flag.

=item helpClass STRING

Specifies the class of the object used as the help viewing package.  The
default value is Prima::HelpViewer.  Run-time changes to the property do not
affect the help subsystem until a call to C<close_help> is made.

=item helpModule STRING

Specifies the perl module loaded indirectly when a help viewing call is made
via the C<open_help> method.  Used when the C<::helpClass> property is
overridden and the new class is contained in a third-party module.  Run-time
changes to the property do not affect the help subsystem until a call to
C<close_help> is made.

=item hintClass STRING

Create-only property.

Specifies the class of the widget used as the hint label.

Default value: Prima::HintWidget

=item hintColor COLOR

The alias to the foreground color property of the hint label widget.

=item hintBackColor COLOR

The alias to the background color property of the hint label widget.

=item hintFont %FONT

The alias to the font property of the hint label widget.

=item hintPause TIMEOUT

Sets the timeout in milliseconds before the hint label is shown
when the mouse pointer hovers over a widget.

=item language STRING

By default contains the user interface language deduced either from the
C<$ENV{LANG}> environment variable (unix) or a system default setting (win32).
When changed, updates the C<textDirection> property.

See also: C<get_system_info>.

=item modalHorizon BOOLEAN

A read-only property. Used as the lowest-level modal horizon.  Always returns
1.

=item palette [ @PALETTE ]

Used only within the paint and information modes.  Selects solid colors in the
system palette, as many as possible.  PALETTE is an array of 8-bit integer
triplets, where each is a red, green, and blue component.

=item printerClass STRING

Create-only property.

Specifies the class of the object used as the printer.  The default value is
system-dependent, but is either C<Prima::Printer> or C<Prima::PS::Printer>.

=item printerModule STRING

Create-only property.

Specifies the perl module loaded indirectly before the printer object of the
C<::printerClass> class is created.  Used when the C<::printerClass> property
is overridden and the new class is contained in a third-party module.

=item pointerVisible BOOLEAN

Manages the system pointer visibility.  If 0, hides the pointer so it is not
visible in all system windows. Therefore this property usage must be considered
with care.

=item size WIDTH, HEIGHT

A read-only property.

Returns two integers, the width and height of the screen.

=item showHint BOOLEAN

If 1, the toolkit is allowed to show the hint label over a widget. If 0, the
display of the hint is forbidden. In addition to the functionality of the
C<::showHint> property in Prima::Widget, Prima::Application::showHint is
another layer of hint visibility control - if it is 0, all hint actions are
disabled, disregarding C<::showHint> value in the widgets.

=item skin SCALAR

The same as L<Prima::Widget/skin>, but is mentioned here because it is possible
to change the whole application skin by changing this property, f ex like this:

   use Prima::Application skin => 'flat';

=item textDirection BOOLEAN

Contains the preferred text direction initially deduced from the preferred
interface language.  If 0 ( default ), the preferred text direction is
left-to-right (LTR), otherwise right-to-left (RTL), f.ex. for Arabic and Hebrew
languages.

The value is used as a default when shaping text and setting widget input
direction.

=item uiScaling FLOAT

The property contains an advisory multiplier factor, useful for UI elements
that have a fixed pixel value, but that would like to be represented in a
useful manner when the display resolution is too high (on modern High-DPI
displays) or too low (on ancient monitors).

By default, it acquires the system display resolution and sets the scaling
factor so that when the DPI is 96 it is 1.0, 192 it is 2.0, etc. The increase
step is 0.25, so that bitmaps may look not that distorted when scaled. However,
when the value is manually set the step is not enforced and any value can be
accepted.

See also: L<Prima/Stress>.

=item wantUnicodeInput BOOLEAN

Selects if the system is allowed to generate key codes in unicode.  Returns the
effective state of the unicode input flag, which cannot be changed if perl or
the operating system does not support UTF8.

If 1, the C<Prima::Clipboard::text> property may return UTF8 text from system
clipboards is available.

Default value: 1

=back

=head2 Events

=over

=item Clipboard $CLIPBOARD, $ACTION, $TARGET

With (the only implemented) C<$ACTION> I<copy>, is called whenever another
application requests clipboard data in the format C<$TARGET>. This notification is
handled internally to optimize image pasting through the clipboard. Since the
clipboard pasting semantics in Prima is such that data must be supplied to the
clipboard in advance, before another application can request it, there is a
problem with which format to use. To avoid encoding an image or other complex
data in all possible formats but do that on demand and in the format the other
application wants, this notification can be used.

Only implemented for X11.

=item CopyImage $CLIPBOARD, $IMAGE

The notification stores C<$IMAGE> in the clipboard.

=item CopyText $CLIPBOARD, $TEXT

The notification stores C<$TEXT> in the clipboard.

=item Die $@, $STACK

Called when an exception occurs inside the event loop C<Prima::run>.  By
default, consults the C<guiException> property, and if it is set, displays the
system message dialog allowing the user to decide what to do next.

=item Idle

Called when the event loop handled all pending events, and
is about to sleep waiting for more.

=item PasteImage $CLIPBOARD, $$IMAGE_REF

The notification queries C<$CLIPBOARD> for image content and stores in
C<$$IMAGE_REF>. The default action is that the C<'Image'> format is queried.
On unix, encoded formats C<'image/bmp'>, C<'image/png'> etc are queried if the
default C<'Image'> is not found.

The C<PasteImage> mechanism can read images from the clipboard in the GTK
environment.

=item PasteText $CLIPBOARD, $$TEXT_REF

The notification queries C<$CLIPBOARD> for text content and stores it in the
C<$$TEXT_REF> scalar. Its default action is that only the C<'Text'> format is
queried if C<wantUnicodeInput> is unset. Otherwise, the C<'UTF8'> format is
queried first.

The C<PasteText> mechanism is devised to ease defining text unicode/ascii
conversion between clipboard and standard widgets, in a unified way.

=back

=head2 Methods

=over

=item add_startup_notification @CALLBACK

CALLBACK is an array of anonymous subs, which are all executed when the
Prima::Application object is created. If the application object is already
created during the call, CALLBACKs are called immediately.

Useful for initialization of add-on packages.

=item begin_paint

Enters the enabled ( active paint ) state, and returns the success flag.  Once the
object is in the enabled state, painting and drawing methods can perform drawing
operations on the whole screen.

=item begin_paint_info

Enters the information state, and returns the success flag.  The object information
state is the same as the enabled state ( see C<begin_paint()>), except that painting
and drawing methods are not permitted to change the screen.

=item close

Issues a system termination call, resulting in calling the C<close> method for
all top-level windows. The call can be interrupted by the latter, and
effectively canceled. If not canceled stops the application event loop.

=item close_help

Closes the help viewer window.

=item end_paint

Quits the enabled state and returns the application object to the normal state.

=item end_paint_info

Quits the information state and returns the application object to the normal state.

=item font_encodings

Returns an array of encodings represented by strings, that are recognized by
the system and available for at least one font. Each system provides different
sets of encoding strings; the font encodings are not portable.

=item fonts NAME = '', ENCODING = ''

Returns a hash of font hashes ( see L<Prima::Drawable/Fonts> ) describing fonts
of NAME font family and of ENCODING text encoding. If NAME is '' or C<undef>,
returns one font hash for each of the font families that match the ENCODING
string. If ENCODING is '' or C<undef>, no encoding match is performed.  If
ENCODING is not valid ( not present in the C<font_encodings> result), it is
treated as if it was '' or C<undef>.

In the special case when both NAME and ENCODING are '' or C<undef>, each font
metric hash contains the element C<encodings>, which points to an array of the
font encodings, available for the fonts of the NAME font family.

=item get_active_window

Returns the object reference to the currently active window, if any, that
belongs to the program. If no such window exists, C<undef> is returned.

The exact definition of 'active window' is system-dependent, but it is
generally believed that an active window is the one that has a keyboard focus
on one of its children widgets.

=item get_caption_font

Returns the title font that the system uses to draw top-level window captions.
The method can be called with a class string instead of an object instance.

=item get_default_cursor_width

Returns the width of the system cursor in pixels.  The method can be called
with a class string instead of an object instance.

=item get_default_font

Returns the default system font.  The method can be called with a class string
instead of an object instance.

=item get_default_scrollbar_metrics

Returns dimensions of the system scrollbars - width of the standard vertical
scrollbar and height of the standard horizon scrollbar.  The method can be
called with a class string instead of an object instance.

=item get_dnd_clipboard

Returns the predefined special clipboard used as a proxy for drag-and-drop
interactions.

See also: C<Widget/Drag and drop>, C<Clipboard/is_dnd>.

=item get_default_window_borders BORDER_STYLE = bs::Sizeable

Returns width and height of standard system window border decorations for one
of the C<bs::XXX> constants.  The method can be called with a class string
instead of an object instance.

=item get_focused_widget

Returns object reference to the currently focused widget, if any, that belongs
to the program. If no such widget exists, C<undef> is returned.

=item get_fullscreen_image

Syntax sugar for grabbing the whole screen as in

   $::application->get_image( 0, 0, $::application->size)

(MacOSX/XQuartz note: get_image() does not grab all screen bits, but
C<get_fullscreen_image> does if Prima is compiled with the Cocoa library).

=item get_hint_widget

Returns the hint label widget, attached automatically to the Prima::Application
object during startup. The widget is of the C<::hintClass> class,
Prima::HintWidget by default.

=item get_image X_OFFSET, Y_OFFSET, WIDTH, HEIGHT

Returns Prima::Image object with WIDTH and HEIGHT dimensions filled with
graphic content of the screen, copied from X_OFFSET and Y_OFFSET coordinates.
If WIDTH and HEIGHT extend beyond the screen dimensions, they are adjusted.  If
the offsets are outside the screen boundaries, or WIDTH and HEIGHT are zero or
negative, C<undef> is returned.

Note: When running on MacOSX under XQuartz, the latter does not give access to
the whole screen, so the function will not be able to grab the top-level menu
bar.  This problem is addressed in the C<get_fullscreen_image> method.

=item get_indents

Returns 4 integers that correspond to extensions of eventual desktop
decorations that the windowing system may present on the left, bottom, right,
and top edges of the screen. For example, for win32 this reports the size of
the part of the screen that the windows taskbar may occupy, if any.

=item get_printer

Returns the printer object attached automatically to the Prima::Application
object. The object is an instance of the C<::printerClass> class.

=item get_message_font

Returns the font the system uses to draw the message text.  The method can be
called with a class string instead of an object instance.

=item get_modal_window MODALITY_TYPE = mt::Exclusive, TOPMOST = 1

Returns the modal window that resides on an end of the modality chain.
MODALITY_TYPE selects the chain, and can be either C<mt::Exclusive> or
C<mt::Shared>. TOPMOST is a boolean flag selecting the lookup direction: if it
is 1, the 'topmost' window is returned, if 0, the 'lower-most' one ( in a
simple case when window A is made modal (executed) after modal window B, the A
window is the 'topmost' one ).

If the chain is empty C<undef> is returned. In case the chain consists of just
one window, the TOPMOST value is irrelevant.

=item get_monitor_rects

Returns set of rectangles in the format of (X,Y,WIDTH,HEIGHT) identifying monitor
layouts.

=item get_scroll_rate

Returns two integer values of two system-specific scrolling timeouts. The first
is the initial timeout that is applied when the user drags the mouse from a
scrollable widget ( a text field, for example ), and the widget is about to
scroll, but the actual scroll is performed after the timeout has expired. The
second value is the repetitive timeout, - if the dragging condition did not
change, the scrolling performs automatically after this timeout. The timeout
values are in milliseconds.

=item get_system_info

Returns a hash with the following keys containing information about the system:

=over

=item apc

One of the C<apc::XXX> constants reporting the platform the program is running on.
Currently, the list of the supported platforms is one of these two:

	apc::Win32
	apc::Unix

=item gui

One of the C<gui::XXX> constants reporting the graphic user interface used in
the system:

	gui::Default
	gui::Windows
	gui::XLib
	gui::GTK

=item guiDescription

Description of the graphic user interface returned as an arbitrary string.

=item guiLanguage

The preferred language of the interface returned as an ISO 639 code.

=item system

An arbitrary string representing the operating system software.

=item release

An arbitrary string, contains the OS version information.

=item vendor

The OS vendor string

=item architecture

The machine architecture string

=back

The method can be called with a class string instead of an object instance.

=item get_system_value

Returns the system integer value, associated with one
of the C<sv::XXX> constants. The constants are:

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
	sv::ColorPointer     - 1 if the system accepts color pointer icons.
	sv::CanUTF8_Input    - 1 if the system can generate key codes in unicode
	sv::CanUTF8_Output   - 1 if the system can output utf8 text
	sv::CompositeDisplay - 1 if the system uses double-buffering and alpha composition for the desktop,
	                       0 if it doesn't, -1 if unknown
	sv::LayeredWidgets   - 1 if the system supports layering
	sv::FixedPointerSize - 0 if the system doesn't support arbitrarily sized pointers and will resize custom icons to the system size
	sv::MenuCheckSize    - width and height of default menu check icon
	sv::FriBidi          - 1 if Prima is compiled with libfribidi and full bidi unicode support is available
	sv::Antialias        - 1 if the system supports antialiasing and alpha layer for primitives
	sv::LibThai          - 1 if Prima is compiled with libthai

The method can be called with a class string instead of an object instance.

=item get_widget_from_handle HANDLE

HANDLE is an integer value of a toolkit widget handle as used in the underlying
GUI level, for example, it is a HWND value on win32. It is usually passed to
the program by other IPC means, so that the method can return the associated
widget.  If no widget is associated with HANDLE, C<undef> is returned.

=item get_widget_from_point X_OFFSET, Y_OFFSET

Returns the widget that occupies the screen area under (X_OFFSET,Y_OFFSET)
coordinates. If no toolkit widgets are found, C<undef> is returned.

=item go

The main event loop. Called by the

   Prima->run;

standard code. Returns when the program is about to terminate, if C<stop> was
called, or if the exception was signaled. In the latter two cases, the loop can
be safely restarted.

=item lock

Effectively blocks the graphic output for all widgets.  The output can be
restored with C<unlock()>.

=item load_font FONTNAME

Registers a font resource in the system-specific format. The resource is freed
after the program ends.

Notes for win32: To add a font whose information comes from several resource
files, point FONTNAME to a string with the file names separated by a C<|> - for
example, C< abcxxxxx.pfm | abcxxxxx.pfb >.

Notes for unix: available only when Prima is compiled with fontconfig and Xft .

Returns the number of the font resources added.

=item open_help TOPIC

Opens the help viewer window with TOPIC string in the link POD format ( see
L<perlpod> ) - the string is treated as "manpage/section", where 'manpage' is
the file with POD content and 'section' is the topic inside the manpage.

Alternatively can handle the syntax in the form of C< file://path|section >
where C<path> is the file with the pod content and C<section> is an optional
pod section within the file.

=item set_hint_action WIDGET, SHOW, IS_MOUSE_EVENT, @AROUND

Special method to execute an immediate show or hide action on the hint widget,
without waiting for either timeout or mouse moved by the user. The boolean SHOW
flag signals whether the hint should be hdden or shown, IS_MOUSE_EVENT signals
if Prima should also start the internal timer that pops up a hint if it was
previously invisible.  The special @AROUND rect is a 4-integer array that
signals that the hint should be positioned in an adjacent fashion next to one
of the rect sides.

=item stop

Stops the event loop. The loop can be started again.

=item sync

Synchronizes all pending requests where there are any. Is an effective
C<XSync(false)> on X11, and is a no-op otherwise.

=item sys_action CALL

CALL is an arbitrary string of the system service name and the parameters to
it.  This functionality is non-portable, and its usage should be avoided.  The
system services provided are not documented and are subject to change. The
actual services can be looked at in the toolkit source code under the
I<apc_system_action> tag.

=item unlock

Unlocks the graphic output for all widgets, previously locked with C<lock()>.

=item yield $wait_for_event=0

An event dispatcher, called from within the event loop.
If the event loop can be schematized, then in this code

	while ( application not closed ) {
		yield
	}

yield() is the only function called repeatedly inside the
loop.  The yield(0) call shouldn't be used to organize event loops, but it can
be employed to process stacked system events explicitly, to increase the
responsiveness of a program, for example, inside a long calculation cycle.

yield(1) though is adapted exactly for external implementation of event loops;
it does the same as yield(0), but if there are no events it sleeps
until there comes at least one, processes it, and then returns. The return
value is 0 if the application doesn't need more event processing, because of
shutting down.  The corresponding code will be

	while ( yield(1)) {
	    ...
	}

but in turn, this call cannot be used for increasing UI responsiveness inside
tight calculation loops.

The method can be called with a class string instead of an object instance;
however, the $::application object must be initialized.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Object>, L<Prima::Widget>, L<Prima::Window>

=cut
