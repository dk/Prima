package Prima::sys::XQuartz;

use strict;
use warnings;
use Prima;

sub get_fullscreen_image
{
	my $self = shift;
	goto FALLBACK unless $^O eq 'darwin';
	goto FALLBACK unless $self->sys_action("xquartz.local_display");
	my $real_screen_height = $self->sys_action("xquartz.screen_height");
	goto FALLBACK unless $real_screen_height;

	my $grab_mode = $self->sys_action("xquartz.grab_mode");
	$self->sys_action("xquartz.grab_mode native");
	goto FALLBACK unless $self->sys_action("xquartz.grab_mode") eq 'native';
	my $diff    = $real_screen_height - $self->height;
	my $menubar = $self->get_image(0,$self->height - $diff,$self->width,$diff);
	$self->sys_action("xquartz.grab_mode $grab_mode");
	goto FALLBACK unless $menubar;

	my $fullscreen = $self->get_image(0,0,$self->size);
	$fullscreen->scaling(ist::None);
	$fullscreen->height( $real_screen_height );
	$fullscreen->put_image( 0, $self-> height, $menubar );
	return $fullscreen;

FALLBACK:
	return $self->get_image(0,0,$self->size);
}

1;

=pod

=head1 NAME

Prima::sys::XQuartz - MacOSX/XQuartz facilities

=head1 DESCRIPTION

XQuartz emulates the X11 environment with certain limits, namely, it cannot grab bits
from the screen, and it also hides the top-level menu from screen coordinates accessible
for X11 clients. For example, a Mac with 1024x768 resolution will only report
f.ex. 1024x746 size to Prima.  If Prima is compiled with the Cocoa library,
the C<get_fullscreen_image> method circumvents these limitations and returns a shot
of the whole screen, including the application menu.

Note that screen grabbing has to be allowed by the user or the
administrator. To do that, Choose the Apple menu, System Preferences, click
Security & Privacy, then click Privacy.  Click on an icon on the left lower
corner to allow changes.  Then, in the screen recording tab, add XQuartz to the
list of allowed applications. Note that it might not work if you run your
application from a (remote) ssh session - I couldn't find how to 
enable screen grabbing for sshd.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Dialog::FileDialog>

=cut
