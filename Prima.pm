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
$VERSION = '1.58';
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
	$::application-> go if $::application-> alive;
	$::application = undef if $::application and not $::application->alive;
}

sub import
{
	my @module = @_;
	while (@module) {
		my $module = shift @module;
		next if $module eq 'Prima' || $module eq '';
		$module = "Prima::$module" unless $module =~ /^Prima::/;
		local $__import = caller;
		if ( $module) {
			eval "use $module;";
			die $@ if $@;
		}
		$__import = 0;
	}
}

1;

__END__

=pod

=head1 NAME

Prima - a perl graphic toolkit

=head1 SYNOPSIS

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

=head1 DESCRIPTION

The toolkit is combined from two basic set of classes - core and external. The
core classes are coded in C and form a base line for every Prima object 
written in perl. The usage of C is possible together with the toolkit; however,
its full power is revealed in the perl domain. The external classes present 
easily expandable set of widgets, written completely in perl and communicating 
with the system using Prima library calls. 

The core classes form an hierarchy, which is displayed below:

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

The very basic code shown in L<"SYNOPSIS"> is explained here. 
The code creates a window with 'Hello,
world' title and a centered button with the same text. The program
terminates after the button is pressed.

A basic construct for a program written with Prima obviously requires 

	use Prima;

code; however, the effective programming requires usage of the other
modules, for example, C<Prima::Buttons>, which contains set of
button widgets. C<Prima.pm> module can be
invoked with a list of such modules, which makes the construction

	use Prima;
	use Prima::Application;
	use Prima::Buttons;

shorter by using the following scheme:

	use Prima qw(Application Buttons);

Another basic issue is the event loop, which is called by

	run Prima;

sentence and requires a C<Prima::Application> object to be created beforehand.
Invoking C<Prima::Application> standard module is one of the possible ways to 
create an application object. The program usually terminates after the event loop
is finished.

The window is created by invoking 

	Prima::MainWindow->new();

or

	Prima::MainWindow-> create()

code with the additional parameters. Actually, all Prima objects are created by such a
scheme. The class name is passed as the first parameter, and a custom set
of parameters is passed afterwards. These parameters are usually 
represented in a hash syntax, although actually passed as an array.
The hash syntax is preferred for the code readability:

	$new_object = Class->new(
		parameter => value,
		parameter => value,
		...
	);

Here, parameters are the class properties names, and differ from class to
class. Classes often have common properties, primarily due to the
object inheritance.

In the example, the following properties are set :

	Window::text
	Window::size
	Button::text
	Button::centered
	Button::onClick

Property values can be of any type, given that they are scalar. As depicted
here, C<::text> property accepts a string, C<::size> - an anonymous array 
of two integers and C<onClick> - a sub.

onXxxx are special properties that form a class of I<events>, 
which share the C<new>/C<create> syntax, and are additive when 
the regular properties are substitutive (read more in L<Prima::Object>). 
Events are called in the object context when a specific condition occurs. 
The C<onClick> event here, for example, is called when the 
user presses (or otherwise activates) the button.

=head1 API

This section describes miscellaneous methods, registered in C<Prima::>
namespace.

=over

=item message TEXT 

Displays a system message box with TEXT.

=item run

Enters the program event loop. The loop is ended when C<Prima::Application>'s C<destroy>
or C<close> method is called.

=item parse_argv @ARGS

Parses prima options from @ARGS, returns unparsed arguments. 

=back

=head1 OPTIONS

Prima applications do not have a portable set of arguments; it depends on the
particular platform. Run  

	perl -e '$ARGV[0]=q(--help); require Prima'

or any Prima program with C<--help> argument to get the list of supported
arguments. Programmaticaly, setting and obtaining these options can be done
by using C<Prima::options> routine.

In cases where Prima argument parsing conflicts with application options, use
L<Prima::noARGV> to disable automatic parsing; also see L<parse_argv>. 
Alternatively, the construct 

	BEGIN { local @ARGV; require Prima; } 

will also do.

=head1 SEE ALSO

The toolkit documentation is divided by several
subjects, and the information can
be found in the following files:

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

L<Prima::Application> - root of widget objects hierarchy 

L<Prima::Printer> - system printing services 

L<Prima::File> - asynchronous stream I/O

=item Widget library

L<Prima::Buttons> - buttons and button grouping widgets 

L<Prima::Calendar> - calendar widget 

L<Prima::ComboBox> - combo box widget 

L<Prima::DetailedList> - multi-column list viewer with controlling header widget

L<Prima::DetailedOutline> - a multi-column outline viewer with controlling header widget

L<Prima::DockManager> - advanced dockable widgets

L<Prima::Docks> - dockable widgets

L<Prima::Edit> - text editor widget

L<Prima::ExtLists> - listbox with checkboxes

L<Prima::FrameSet> - frameset widget class

L<Prima::Grids> - grid widgets

L<Prima::Header> - a multi-tabbed header widget

L<Prima::HelpViewer> - the built-in POD file browser

L<Prima::Image::TransparencyControl> - standard dialog for transparent color index selection

L<Prima::ImageViewer> - bitmap viewer

L<Prima::InputLine> - input line widget

L<Prima::KeySelector> - key combination widget and routines

L<Prima::Menus> - menu widgets

L<Prima::Label> - static text widget 

L<Prima::Lists> - user-selectable item list widgets

L<Prima::MDI> - top-level windows emulation classes

L<Prima::Notebooks> - multipage widgets

L<Prima::Outlines> - tree view widgets

L<Prima::PodView> - POD browser widget

L<Prima::ScrollBar> - scroll bars

L<Prima::ScrollWidget> - scrollable generic document widget

L<Prima::Sliders> - sliding bars, spin buttons and input lines, dial widget etc.

L<Prima::Spinner> - spinner animation

L<Prima::StartupWindow> - a simplistic startup banner window

L<Prima::TextView> - rich text browser widget

L<Prima::Themes> - widget themes manager

=item Standard dialogs

L<Prima::Dialog::ColorDialog> - color selection facilities

L<Prima::Dialog::FindDialog> - find and replace dialogs

L<Prima::Dialog::FileDialog> - file system related widgets and dialogs

L<Prima::Dialog::FontDialog> - font dialog

L<Prima::Dialog::ImageDialog> - image file open and save dialogs

L<Prima::Image::TransparencyControl> - transparent color index selection

L<Prima::MsgBox> - message and input dialog boxes

L<Prima::Dialog::PrintDialog> - standard printer setup dialog

=item Drawing helpers

L<Prima::Drawable::CurvedText> - fit text to path

L<Prima::Drawable::Glyphs> - bi-directional text input and complex scripts output

L<Prima::Drawable::Gradient> - gradient fills for primitives

L<Prima::Drawable::Markup> - Allow markup in widgets

L<Prima::Drawable::Path> - stroke and fill complex paths

L<Prima::Drawable::Subcanvas> - paint a hierarchy of widgets to any drawable

L<Prima::Drawable::TextBlock> - rich text representation

=item Visual Builder

L<VB> - Visual Builder for the Prima toolkit

L<Prima::VB::VBLoader> - Visual Builder file loader

L<prima-cfgmaint> - configuration tool for Visual Builder

L<Prima::VB::CfgMaint> - maintains visual builder widget palette configuration

=item PostScript printer interface

L<Prima::PS::Drawable> - PostScript interface to C<Prima::Drawable>

L<Prima::PS::Printer> - PostScript interface to C<Prima::Printer>

=item C interface to the toolkit

L<Prima::internals> - Internal architecture

L<Prima::codecs>    - Step-by-step image codec creation

L<prima-gencls>     - C<prima-gencls>, a class compiler tool.

=item Miscellaneous

L<Prima::faq> - frequently asked questions 

L<Prima::Const> - predefined toolkit constants 

L<Prima::EventHook> - event filtering

L<Prima::Image::Animate> - animate gif and webp files

L<Prima::IniFile> - support of Windows-like initialization files

L<Prima::IntUtils> - internal functions

L<Prima::StdBitmap> - shared access to the standard toolkit bitmaps

L<Prima::Stress> - stress test module

L<Prima::Tie> - tie widget properties to scalars or arrays

L<Prima::Utils> - miscellaneous routines

L<Prima::Widgets> - miscellaneous widget classes

=item System-specific modules and documentation

L<Prima::gp-problems> - Graphic subsystem portability issues

L<Prima::X11> - usage guide for X11 environment

L<Prima::sys::gtk::FileDialog> - GTK file system dialogs

L<Prima::sys::win32::FileDialog> - Windows file system dialogs

L<Prima::sys::XQuartz> - MacOSX/XQuartz facilities

=item Class information

The Prima manual pages often provide information for more than one Prima class.
To quickly find out the manual page of a desired class, as well as display the
inheritance information, use C<prima-class> command. The command can produce output in
text and pod formats; the latter feature is used by the standard Prima documentation
viewer C<podview> ( see File/Run/prima-class ).

=back

=head1 COPYRIGHT

Copyright 1997-2003 The Protein Laboratory, University of Copenhagen. All
rights reserved.

Copyright 2004-2020 Dmitry Karasik. All rights reserved. 

This program is distributed under the BSD License.

=head1 AUTHORS

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>,
Anton Berezin E<lt>tobez@tobez.orgE<gt>,
Vadim Belman E<lt>voland@lflat.orgE<gt>,

=cut
