#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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

BEGIN {
    if ( $^O =~ /freebsd/i) {
	( my $ver = `/usr/bin/uname -r`) =~ s/^(\d+\.\d+).*$/$1/;
	if ( $ver >= 3.4) {
	    eval "sub dl_load_flags { 0x01 }";
	}
    }
}

$VERSION = '1.04';
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
      eval "use $module \%parameters;" if $module;
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

The toolkit contains two basic set of classes - built-in and external. The
built-in set consists of a fixed small amount of classes, coded in C. These
classes form a baseline for every Prima object written in perl. Although
usage of C is possible with the toolkit, its full power is revealed in perl
programming. The external classes are the expandable set of widgets,
written completely in perl and communicating with the system using Prima
interfaces. The built-in classes can be arrayed in the tree-like hierarchy
scheme:


    Object
        Component
                AbstractMenu
                        AccelTable
                        Menu
                        Popup
                Clipboard
                Drawable
                        DeviceBitmap
                        Printer
                        Image
                                Icon
                File
                Timer
                Widget
                        Application
                        Window

The description of these classes can be found by adding Prima:: prefix to
the request - for example, the I<Drawable> manpage is located 
under I<Prima> etc. These classes are declared in Prima::Classes module.

The number of external classes is much bigger, and the whole tree is not
depicted here, primarily because of its extent. Some of these classes used
as an illustration to the toolkit usage although.

=head1 BASIC PROGRAM

The very basic code shown in L<"SYNOPSIS"> is explained here. 
The code creates a window with a title 'Hello,
world' with a button in the center and the same text. The program
terminates after the button is hit.

A basic construct for a program written with Prima obviously requires 

  use Prima;

code. However, the effective programming requires usage of the other
modules, for example, I<Prima::Buttons>, which contains different kinds of button widgets. Prima.pm module can be
invoked with a list of such modules, what makes the construction

  use Prima;
  use Prima::Application;
  use Prima::Buttons;
  
shorter by using the following scheme:

  use Prima qw(Application Buttons);

Another basic issue is the event loop, which is called by

   run Prima;

sentence and requires a Prima::Application object to be created before.
Invoking I<Prima::Application> standard module is one of the possible ways to create the application
object. The program usually terminates after quitting the event loop.

The window is created by invoking 

  Prima::Window-> create()

code with the additional parameters. Actually, all Prima objects are created by such a
scheme. The class name is passed as the first parameter, and a custom set
of parameters is passed aftrewards. These parameters are usually are
represented in a syntax of a hash, although actually they are an array.
Nevertheless, the hash syntax helps both readability and understanding of
the code. The syntax is:


   $new_object = Class-> create(
     parameter => value,
     parameter => value,
     ...
   );

The parameter names are the class properties names, and differ from class to
class, although many classes share properties, primarily because of the
object inheritance.

In the example, the following properties are set forth :

 Window::text
 Window::size
 Button::text
 Button::centered
 Button::onClick

The properties can be of any type, given that they are scalar. As depicted
here, C<::text> property accepts a string, C<::size> - an anonymous array 
of two integers and C<onClick> - a sub.

In short, onXxxx properties form a class of I<events>, 
which although share the C<create> syntax but are not
substitutive but additive ( read more in L<Prima::Object> ). 
Events are called in the object context when a specific condition occurs. 
The C<onClick> event here, for example, is called when the 
user presses ( or otherwise activates ) the button.

=head1 SEE ALSO

The toolkit documentation is divided by several
subjects, and the information can
be found in the following files.

B<Perl programming>:

- L<Prima::Object> - Basic object concepts, properties, events.

- L<Prima::Drawable> - 2-D graphic interface 

- L<Prima::Image>  - Bitmap routines 

- L<Prima::image-load> - Image subsystem and file operations

- L<Prima::Widget> - Window management 

- L<Prima::Window> - Window and Dialog - top-level window management

- L<Prima::Menu> - pull-down and pop-up menu objects

- L<Prima::Timer> - programmable periodical events 

- L<Prima::Application> - root of widget objects hierarchy 

- L<Prima::Printer> - system printing services 

- L<Prima::File> - asynchronous stream I/O

B<C programming>

- L<Prima::internals> - Internal architecture

- L<Prima::codecs>    - Step-by-step image codec creation

- L<gencls>           - gencls, a class compiler tool.

B<miscellaneous>:

- L<Prima::gp-problems> - Graphic subsystem portability issues

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
