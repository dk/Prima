package Prima::sys::XQuartz;

use strict;
use warnings;

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

=head1 NAME

Prima::sys::XQuartz - MacOSX/XQuartz facilities

=head1 DESCRIPTION

XQuartz emulates X environment with certain limits, namely it cannot grab bits
from the screen, and it hides top-level menu from screen coordinates accessible
for X clients. For example, a Mac with 1024x768 resolution will only report
f.ex. 1024x746 size to Prima.  If Prima is compiled with Cocoa library,
C<get_fullscreen_image> method circumvents these limitations and returns shot
of the whole screen.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Dialog::FileDialog>

=cut
