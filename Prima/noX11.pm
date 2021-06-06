# Initializes Prima in no-X11 environment

package Prima::noX11; # for metacpan
package
  Prima;
use strict;
use warnings;
no warnings 'once';
use Prima::Config;

push @Prima::preload, argv => '--no-x11' if $Prima::Config::Config{platform} eq 'unix';

sub XOpenDisplay
{
	return undef unless $Prima::Config::Config{platform} eq 'unix';

	Prima::options( 'display', $_[0]) if @_;
	return Prima::Application::sys_action( 'Prima::Application', 'XOpenDisplay');
}

1;

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

=cut
