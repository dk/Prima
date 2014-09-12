#
#  Copyright (c) 1997-2003 The Protein Laboratory, University of Copenhagen
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
#  Created by Dmitry Karasik  <dmitry@karasik.eu.org>
#
# $Id$
#
# Initializes Prima in no-X11 environment

package Prima;
use Prima::Config;

push @preload, argv => '--no-x11' if $Prima::Config::Config{platform} eq 'unix';

sub XOpenDisplay
{
	return undef unless $Prima::Config::Config{platform} eq 'unix';

	Prima::options( 'display', $_[0]) if @_;
	return Prima::Application::sys_action( 'Prima::Application', 'XOpenDisplay');
}

1;

__DATA__

=head1 NAME

Prima::noX11 - Use Prima without X11

=head1 SYNOPSIS

   use Prima::noX11;
   use Prima;

   my $error = Prima::XOpenDisplay();
   if ( defined $error) {
        print "not connected to display: $error\n";
   } else {
   	print "connected to display\n";
   }

=head1 DESCRIPTION

Prima will by default connect to X11 server on unix. To use
Prima functionality in modules or programs where this default
behavior is undesired, please follow the guidelines below.

=head2 No connection

In the beginning of a script or a module that is never intended to connect to
X11 display, add this:

  use Prima::noX11;
  use Prima;

It will be possible to connect to X11 server later on manually.

=head2 Manual connect to X11

If connection to X11 is optional, use this code after C<use Prima::noX11>
was invoked:

   my $error = Prima::XOpenDisplay();
   if ( defined $error) {
        print "not connected to display: $error\n";
   } else {
   	print "connected to display\n";
   }


=head2 Checking if GUI functionality is accesiible.

Without X11 connection, no GUI functionality such as screen grabbing will be
accessible. In addition to that functionality, windowing functions will only
become accessible after L<Prima::Application> creates a single instance
C<$::application>.

Therefore, if C<$::application> is defined, then all GUI functions can be
safely used. If, on the contrary, it is not defined, initiate it as this:

        unless ( $::application) {
                my $error = Prima::XOpenDisplay();
                die $error if defined $error;
                require Prima::Application;
                import Prima::Application;
        }

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::X11>

=cut
