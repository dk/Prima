#
#  Created by Anton Berezin  <tobez@plab.ku.dk>
#
package Prima;

use strict;
use warnings;
require DynaLoader;
use vars qw($VERSION @ISA $__import @preload $pid);
@ISA = qw(DynaLoader);
sub dl_load_flags { 0x00 }
$VERSION = '1.71';
$pid = $$;
bootstrap Prima $VERSION;
unless ( UNIVERSAL::can('Prima', 'init')) {
	$::application = 0;
	return 0;
}
$::application = undef;
require Prima::Const;
require Prima::Classes;

sub parse_argv
{
	my %options = Prima::options();
	my @ret;
	for ( my $i = 0; $i < @_; $i++) {
		if ( $_[$i] =~ m/^--(?:([^\=]+)\=)?(.*)$/) {
			my ( $option, $value) = ( defined( $1) ? ( $1, $2) : ( $2, undef));
			last unless defined($option);
			if ( $option eq 'help') {
				my @options = Prima::options();
				printf "   --%-10s - %s\n", shift @options, shift @options
					while @options;
				exit(0);
			}
			next unless exists $options{$option};
			Prima::options( $option, $value);
		} else {
			push @ret, $_[$i];
		}
	}
	return @ret;
}

{
	my ( $i, $skip_argv, @argv);
	for ( $i = 0; $i < @preload; $i++) {
		if ( $preload[$i] eq 'argv') {
			push @argv, $preload[++$i];
		} elsif ( $preload[$i] eq 'noargv') {
			$skip_argv++;	
		}
	}
	parse_argv( @argv) if @argv;
	@ARGV = parse_argv( @ARGV) if @ARGV and not $skip_argv;
}

Prima::init($VERSION) unless $^C;

sub CLONE { $pid = 0 }

sub END
{
	&Prima::cleanup() if $pid == $$ && UNIVERSAL::can('Prima', 'cleanup');
}

sub run
{
	die "Prima was not properly initialized\n" unless $::application;
	while ( 1 ) {
		my $stack;
		my $got_exception;
		{
			local $SIG{__DIE__} = sub {
				$got_exception = 1;
				$stack //= Carp::longmess();
				die @_;
			};
			eval {
				$::application-> go if $::application-> alive;
				$got_exception = 0;
			};
		}
		last unless defined $@ && $got_exception;
		next if $::application->alive && !$::application->notify('Die', "$@", $stack);
		die $@;
	}
	$::application-> destroy if $::application && $::application-> alive;
	$::application = undef if $::application and not $::application->alive;
}

sub import
{
	my @module = @_;
	while (@module) {
		my $module = shift @module;
		next if $module eq 'Prima' || $module eq '';
		$module = "Prima::$module" unless $module =~ /^Prima::/;
		next unless $module;
		local $__import = caller;
		eval "use $module;";
		die $@ if $@;
		if ( $module->isa('Exporter') && $module->can('export')) {
			no strict 'refs';
			if ( exists ${$module.'::'}{'EXPORT'} && (my $ok = \@{"${module}::EXPORT"})) {
				$module->export( $__import, @$ok) if @$ok;
			}
		}
		$__import = 0;
	}
}

1;

__END__

=pod

=head1 NAME

Prima - a Perl graphic toolkit

=head1 SYNOPSIS

=for podview <img src="Prima/hello-world.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/hello-world.gif">

	use Prima qw(Application Buttons);

	Prima::MainWindow->new(
		text     => 'Hello world!',
		size     => [ 200, 200],
	)-> insert( Button =>
		centered => 1,
		text     => 'Hello world!',
		onClick  => sub { $::application-> close },
	);

	run Prima;

See more screenshots at L<http://prima.eu.org/big-picture>.

=head1 DESCRIPTION

Prima is a classic 2D GUI toolkit that works under Windows and X11
environments. The toolkit features a rich widget library, extensive 2D
graphic support, PDF generation, modern Unicode text input and output, and
supports a wide set of image formats.  Additionally, the RAD-style Visual
Builder and POD viewer are included. The toolkit can interoperate with other
popular event loop libraries.

=head1 CLASS HIERARCHY

The toolkit is built with a combination of two basic sets of classes - core and
external. The core classes are coded in C and form a baseline for every Prima
object written in Perl. The usage of C is possible together with the toolkit;
however, its full power is revealed in the Perl domain. The external classes
present an easily expandable set of widgets, written entirely in Perl and
communicating with the system using Prima library calls.

The core classes form a hierarchy, which is displayed below:

	Prima::Object
		Prima::Component
			Prima::AbstractMenu
				Prima::AccelTable
				Prima::Menu
				Prima::Popup
			Prima::Clipboard
			Prima::Drawable
				Prima::DeviceBitmap
				Prima::Printer
				Prima::Image
					Prima::Icon
			Prima::File
			Prima::Region
			Prima::Timer
			Prima::Widget
				Prima::Application
				Prima::Window

The external classes are derived from these; the list of widget classes
can be found below in L</SEE ALSO>.

=head1 BASIC PROGRAM

The very basic code shown in L<"SYNOPSIS"> is explained here.  The code creates
a window with a 'Hello, world' title and a button with the same text.
The program terminates after the button is pressed.

A basic construct for a program written with Prima requires 

	use Prima;

code; however, effective programming requires the usage of the other modules, for
example, C<Prima::Buttons>, which contains various button widgets. The
C<Prima.pm> module can be invoked together with a list of such modules, which makes the
construction

	use Prima;
	use Prima::Application;
	use Prima::Buttons;

shorter by using the following scheme:

	use Prima qw(Application Buttons);

Another basic issue is the event loop, which is called by

	run Prima;

code and requires a C<Prima::Application> object to be created beforehand.
Invoking the C<Prima::Application> standard module is one of the possible ways
to create an application object. The program usually terminates after the event
loop is finished.

The main window is created by invoking

	Prima::MainWindow->new();

or

	Prima::MainWindow->create()

code with additional parameters. All Prima objects are created by the same
scheme; the class name is passed as the first parameter, and a custom set
of parameters is passed afterward:

	$new_object = Class->new(
		parameter => value,
		parameter => value,
		...
	);

Here, parameters are the class property names, and they differ from class to
class. Classes often have common properties, primarily due to object
inheritance.

In the example, the following properties are used:

	Window::text
	Window::size
	Button::text
	Button::centered
	Button::onClick

Property values can be of any scalar type. For example, the C<::text> property
accepts a string, C<::size> - an anonymous array of two integers, and
C<onClick> - a sub.

onXxxx are special properties that describe I<events> that can be used together
with the C<new>/C<create> syntax, and are additive when the regular properties
are substitutive (read more in L<Prima::Object>).  Events are called in the
object context when a specific condition occurs.  The C<onClick> event here,
for example, is called when the user presses (or otherwise activates) the
button.

=head1 API

This section describes miscellaneous methods, registered in the C<Prima::>
namespace.

=over

=item message TEXT

Displays a system message box with TEXT.

=item open_file, save_file

When the C<Prima::Dialog::FileDialog> module is loaded, these shortcut methods
are registered in the C<Prima::> namespace as an alternative to the same
methods in the module's namespace. The methods execute standard file open
and save dialogs, correspondingly.

See L<Prima::Dialog::FileDialog> for more.

=item run

Enters the program event loop. The loop is ended when C<Prima::Application>'s C<destroy>
or C<close> method is called.

=item parse_argv @ARGS

Parses Prima options from @ARGS, returns unparsed arguments.

=back

=head1 OPTIONS

Prima applications do not have a portable set of arguments; it depends on the
particular platform. Run

	perl -e '$ARGV[0]=q(--help); require Prima'

or any Prima program with a C<--help> argument to get the list of supported
arguments. Programmatically, setting and obtaining these options can be done
by using the C<Prima::options> routine.

In cases where the Prima argument parsing conflicts with the application
options, use L<Prima::noARGV> to disable the automatic parsing; also see
L<parse_argv>.  Alternatively, the construct

	BEGIN { local @ARGV; require Prima; }

will also do.

=head1 SEE ALSO

The toolkit documentation is divided into several sections; the individual
manuals can be found in the following files:

=over

=item Tutorials

L<Prima::tutorial> - introductory tutorial

=item Core toolkit classes

L<Prima::Object> - basic object concepts, properties, events

L<Prima::Classes> - binder module for the core classes

L<Prima::Drawable> - 2-D graphic interface

L<Prima::Region> - generic shape for clipping and hit testing

L<Prima::Image>  - bitmap routines

L<Prima::image-load> - image subsystem and file operations

L<Prima::Widget> - window management

=over 2

=item *

L<Prima::Widget::pack> - Tk::pack geometry manager

=item *

L<Prima::Widget::place> - Tk::place geometry manager

=back

L<Prima::Window> - top-level window management

L<Prima::Clipboard> - GUI interprocess data exchange

L<Prima::Menu> - pull-down and pop-up menu objects

L<Prima::Timer> - programmable periodical events

L<Prima::Application> - the root of widget objects hierarchy

L<Prima::Printer> - system printing services

L<Prima::File> - asynchronous stream I/O

=item Widget library

L<Prima::Buttons> - buttons, checkboxes, radios

L<Prima::Calendar> - calendar widget

L<Prima::ComboBox> - combo box widget

L<Prima::DetailedList> - multi-column list viewer with controlling header widget

L<Prima::DetailedOutline> - multi-column outline viewer with controlling header widget

L<Prima::DockManager> - advanced dockable widgets

L<Prima::Docks> - dockable widgets

L<Prima::Edit> - text editor

L<Prima::ExtLists> - listbox with checkboxes

L<Prima::FrameSet> - frameset widget

L<Prima::Grids> - grid widgets

L<Prima::HelpViewer> - the built-in POD browser

L<Prima::ImageViewer> - image viewer

L<Prima::InputLine> - input line widget

L<Prima::KeySelector> - key combination widget and routines

L<Prima::Menus> - menu widgets

L<Prima::Label> - static text widget

L<Prima::Lists> - list widgets

L<Prima::MDI> - top-level window emulation

L<Prima::Notebooks> - multipage widgets

L<Prima::Outlines> - tree view widgets

L<Prima::PodView> - POD browser widget

L<Prima::ScrollBar> - scroll bars

L<Prima::Sliders> - sliding bars, spin buttons, dial widgets, etc.

L<Prima::Spinner> - spinner animation widget

L<Prima::TextView> - rich text browser widget

L<Prima::Widget::Date> - date picker widget

L<Prima::Widget::Time> - time input widget

=item Standard dialogs

L<Prima::Dialog::ColorDialog> - color selection facilities

L<Prima::Dialog::FindDialog> - find and replace dialogs

L<Prima::Dialog::FileDialog> - file system-related widgets and dialogs

L<Prima::Dialog::FontDialog> - font dialog

L<Prima::Dialog::ImageDialog> - image file open and save dialogs

L<Prima::Image::TransparencyControl> - transparent color index selection

L<Prima::MsgBox> - message and input dialog boxes

L<Prima::Dialog::PrintDialog> - standard printer setup dialog

=item Drawing helpers

L<Prima::Drawable::Antialias> - plot antialiased shapes

L<Prima::Drawable::CurvedText> - fit text to path

L<Prima::Drawable::Glyphs> - bi-directional text input and complex scripts output

L<Prima::Drawable::Gradient> - gradient fills for primitives

L<Prima::Drawable::Markup> - allow markup in widgets

L<Prima::Drawable::Metafile> - graphics recorder

L<Prima::Drawable::Path> - stroke and fill complex paths

L<Prima::Drawable::Subcanvas> - paint a hierarchy of widgets to any drawable

L<Prima::Drawable::TextBlock> - rich text representation

=item Visual Builder

L<VB> - Visual Builder for the Prima toolkit

L<Prima::VB::VBLoader> - Visual Builder file loader

L<prima-cfgmaint> - configuration tool for Visual Builder

L<Prima::VB::CfgMaint> - configures the widget palette in the Visual Builder

=item PostScript printer interface

L<Prima::PS::PostScript> - PostScript interface to C<Prima::Drawable>

L<Prima::PS::PDF> - PDF interface to C<Prima::Drawable>

L<Prima::PS::Printer> - PostScript and PDF interfaces to C<Prima::Printer>

=item Widget helpers

L<Prima::Widget::BidiInput> - heuristics for i18n input

L<Prima::Widget::Fader> - fading- in/out functions

L<Prima::Widget::GroupScroller> - optional automatic scroll bars

L<Prima::Widget::Header> - multi-column header widget

L<Prima::Widget::IntIndents> - indenting support

L<Prima::Widget::Link> - links embedded in widgets

L<Prima::Widget::ListBoxUtils> - common paint routine for listboxes

L<Prima::Widget::MouseScroller> - auto-repeating mouse events

L<Prima::Widget::Panel> - simple panel widget

L<Prima::Widget::RubberBand> - dynamic rubberbands

L<Prima::Widget::ScrollWidget> - scrollable generic document widget

L<Prima::Widget::StartupWindow> - a simplistic startup banner window

L<Prima::Widget::UndoActions> - undo and redo the content of editable widgets

=item C interface to the toolkit

L<Prima::internals> - Internal architecture

L<Prima::codecs>    - Step-by-step image codec creation

L<prima-gencls>     - C<prima-gencls>, a class compiler tool.

=item Miscellaneous

L<Prima::faq> - frequently asked questions

L<Prima::Const> - predefined toolkit constants

L<Prima::EventHook> - event filtering

L<Prima::Image::Animate> - animate gif and webp files

L<Prima::Image::base64> - hardcoded image files

L<Prima::Image::Loader> - per-frame image loading and saving

L<Prima::IniFile> - support of Windows-like initialization files

L<Prima::StdBitmap> - shared access to the standard bitmaps

L<Prima::Stress> - stress test module

L<Prima::Themes> - widget themes manager

L<Prima::Tie> - tie widget properties to scalars and arrays

L<Prima::types> - builtin types

L<Prima::Utils> - miscellaneous routines

=item System-specific modules and documentation

L<Prima::gp-problems> - Graphic subsystem portability issues

L<Prima::X11> - usage guide for X11 environment

L<Prima::sys::gtk::FileDialog> - GTK file system dialogs

L<Prima::sys::win32::FileDialog> - Windows file system dialogs

L<Prima::sys::XQuartz> - MacOSX/XQuartz facilities

L<Prima::sys::FS> - unicode-aware core file functions

=item Class information

The Prima manual pages often provide information for more than one Prima class.
To quickly find out the manual page of a desired class, as well as display the
inheritance information, use the C<prima-class> command. The command can
produce output in the text and pod formats; the latter feature is used by the
standard Prima documentation viewer C<podview> ( see File/Run/prima-class ).

=back

=head1 COPYRIGHT

Copyright 1997-2003 The Protein Laboratory, University of Copenhagen. All
rights reserved.

Copyright 2004-2023 Dmitry Karasik. All rights reserved.

This program is distributed under the BSD License.

=head1 AUTHORS

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>,
Anton Berezin E<lt>tobez@tobez.orgE<gt>,
Vadim Belman E<lt>voland@lflat.orgE<gt>,

=cut
