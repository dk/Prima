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
#  Created by Anton Berezin  <tobez@plab.ku.dk>
#
#  $Id$

package Prima;

use strict;
require DynaLoader;
use vars qw($VERSION @ISA $__import);
@ISA = qw(DynaLoader);
sub dl_load_flags { 0x00 }
$VERSION = '1.07';
bootstrap Prima $VERSION;
unless ( UNIVERSAL::can('Prima', 'init')) {
   $::application = 0;
   return 0;
}
$::application = undef;
require Prima::Const;
require Prima::Classes;
init Prima $VERSION;

sub END
{
   &Prima::cleanup() if UNIVERSAL::can('Prima', 'cleanup');
}

sub run
{
   die "Prima was not properly initialized\n" unless $::application;
   $::application-> go if $::application-> alive;
   $::application = undef if $::application and not $::application->alive;
}

sub find_image
{
   my $mod = @_ > 1 ? shift : 'Prima';
   my $name = shift;
   $name =~ s!::!/!g;
   $mod =~ s!::!/!g;
   for (@INC) {
      return "$_/$mod/$name" if -f "$_/$mod/$name" && -r _;
   }
   return undef;
}

sub import
{
   my @module = @_;
   while (@module) {
      my $module = shift @module;
      my %parameters = ();
      %parameters = %{shift @module} if @module && ref($module[0]) eq 'HASH';
      next if $module eq 'Prima' || $module eq '';
      $module = "Prima::$module" unless $module =~ /^Prima::/;
      $__import = caller;
      if ( $module) {
         eval "use $module \%parameters;";
         die $@ if $@;
      }
      $__import = 0;
   }
}

# returns a preferred path for the toolkit configuration files,
# or, if a filename given, returns the name appended to the path
# and proofs that the path exists
sub path
{
   my $path;
   if ( exists $ENV{HOME}) {
      $path = "$ENV{HOME}/.prima";
   } elsif ( $^O =~ /win32/ && exists $ENV{WINDIR}) {
      $path = "$ENV{WINDIR}/.prima";
   } else {
      $path = "/.prima";
   }

   if ( $_[0]) {
      unless ( -d $path) {
         eval "use File::Path"; die "$@\n" if $@;
         File::Path::mkpath( $path);
      }
      $path .= "/$_[0]";
   }

   return $path;
}

1;

__END__

=pod

=head1 NAME

Prima - a perl graphic toolkit

=head1 SYNOPSIS

  use Prima qw(Application Buttons);

  Prima::Window-> create(
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
written in perl. The usage of C is possible together with the toolkit, however 
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

code. However, the effective programming requires usage of the other
modules, for example, C<Prima::Buttons>, which contains set of
button widgets. C<Prima.pm> module can be
invoked with a list of such modules, what makes the construction

  use Prima;
  use Prima::Application;
  use Prima::Buttons;
  
shorter by using the following scheme:

  use Prima qw(Application Buttons);

Another basic issue is the event loop, which is called by

   run Prima;

sentence and requires a C<Prima::Application> object to be created before.
Invoking C<Prima::Application> standard module is one of the possible ways to 
create an application object. The program usually terminates after the event loop
is finished.

The window is created by invoking 

  Prima::Window-> create()

code with the additional parameters. Actually, all Prima objects are created by such a
scheme. The class name is passed as the first parameter, and a custom set
of parameters is passed afterwards. These parameters are usually 
represented in a hash syntax, although actually passed as an array.
The hash syntax is preferred for the code readability:

   $new_object = Class-> create(
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
which share the C<create> syntax, and are additive when 
the regular properties are substitutive ( read more in L<Prima::Object> ). 
Events are called in the object context when a specific condition occurs. 
The C<onClick> event here, for example, is called when the 
user presses ( or otherwise activates ) the button.

=head1 API

This section describes miscellaneous methods, registered in C<Prima::>
namespace.

=over

=item find_image PATH

Converts PATH from perl module notation into a file path, and
searches for the file in C<@INC> paths set. If a file is
found, its full filename is returned; otherwise C<undef> is
returned.

=item message TEXT 

Displays a system message box with TEXT.

=item path [ FILE ]

If called with no parameters, returns path to a directory,
usually F<~/.prima>, that can be used to contain the user settings
of a toolkit module or a program. If FILE is specified, appends
it to the path and returns the full file name. In the latter case 
the path is automatically created by C<File::Path::mkpath> unless
already exists.

=item run

Enters the program event loop. The loop is ended when C<Prima::Application>'s C<destroy>
or C<close> method is called.

=back

=head1 SEE ALSO

The toolkit documentation is divided by several
subjects, and the information can
be found in the following files:

=over

=item Core toolkit classes

L<Prima::Object> - basic object concepts, properties, events

L<Prima::Classes> - binder module for the core classes 

L<Prima::Drawable> - 2-D graphic interface 

L<Prima::Image>  - bitmap routines 

L<Prima::image-load> - image subsystem and file operations

L<Prima::Widget> - window management 

L<Prima::Window> - top-level window management

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

L<Prima::DockManager> - advanced dockable widgets

L<Prima::Docks> - dockable widgets

L<Prima::Edit> - text editor widget

L<Prima::ExtLists> - listbox with checkboxes

L<Prima::FrameSet> - frameset widget class

L<Prima::Header> - a multi-tabbed header widget

L<Prima::HelpViewer> - the built-in POD file browser

L<Prima::Image::TransparencyControl> - standard dialog for transparent color index selection

L<Prima::ImageViewer> - bitmap viewer

L<Prima::InputLine> - input line widget

L<Prima::KeySelector> - key combination widget and routines

L<Prima::Label> - static text widget 

L<Prima::Lists> - user-selectable item list widgets

L<Prima::MDI> - top-level windows emulation classes

L<Prima::Notebooks> - multipage widgets

L<Prima::Outlines> - tree view widgets

L<Prima::PodView> - POD browser widget

L<Prima::ScrollBar> - scroll bars

L<Prima::ScrollWidget> - scrollable generic document widget

L<Prima::Sliders> - sliding bars, spin buttons and input lines, dial widget etc.

L<Prima::StartupWindow> - a simplistic startup banner window

L<Prima::TextView> - rich text browser widget

=item Standard dialogs

L<Prima::ColorDialog> - color selection facilities

L<Prima::EditDialog> - find and replace dialogs

L<Prima::FileDialog> - file system related widgets and dialogs 

L<Prima::FontDialog> - font dialog

L<Prima::ImageDialog> - image file open and save dialogs

L<Prima::MsgBox> - message and input dialog boxes

L<Prima::PrintDialog> - standard printer setup dialog

L<Prima::StdDlg> - wrapper module to the toolkit standard dialogs

=item Visual Builder

L<VB> - Visual Builder for the Prima toolkit

L<Prima::VB::VBLoader> - Visual Builder file loader

L<cfgmaint> - configuration tool for Visual Builder

L<Prima::VB::CfgMaint> - maintains visual builder widget palette configuration

=item PostScript printer interface

L<Prima::PS::Drawable> - PostScript interface to C<Prima::Drawable>

L<Prima::PS::Encodings> - latin-based encodings

L<Prima::PS::Fonts> - PostScript device fonts metrics

L<Prima::PS::Printer> - PostScript interface to C<Prima::Printer>

=item C interface to the toolkit

L<Prima::internals> - Internal architecture

L<Prima::codecs>    - Step-by-step image codec creation

L<gencls>           - C<gencls>, a class compiler tool.

L<Prima::Make> - module for automated Makefile creation 

=item Miscellaneous

L<Prima::Const> - predefined toolkit constants 

L<Prima::IniFile> - support of Windows-like initialization files

L<Prima::IntUtils> - internal functions

L<Prima::StdBitmap> - shared access to the standard toolkit bitmaps

L<Prima::Stress> - stress test module

L<Prima::Utils> - miscellanneous routines

L<Prima::Widgets> - miscellaneous widget classes

L<Prima::gp-problems> - Graphic subsystem portability issues

=back

=head1 COPYRIGHT

Copyright 1997, 2002 The Protein Laboratory, University of Copenhagen. All
rights reserved.

This library is free software; you can redistribute it and/or modify it
under the same terms as Perl itself.

=head1 AUTHORS

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>,
Anton Berezin E<lt>tobez@tobez.orgE<gt>,
Vadim Belman E<lt>voland@lflat.orgE<gt>,

=cut
