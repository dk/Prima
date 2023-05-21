package Prima::Widget::ListBoxUtils;

use strict;
use warnings;

sub draw_item_background
{
	my ( $self, $canvas, $left, $bottom, $right, $top, $prelight, $back_color ) = @_;
	if ( $prelight ) {
		$back_color //= $canvas-> backColor;
		my $c = $self-> color;
		if ( $self->skin eq 'flat') {
			$canvas->color($self->prelight_color($back_color));
			$canvas->bar( $left, $bottom, $right, $top);
		} else {
			$canvas-> new_gradient(
				spline   => [ 0.75, 0.25 ],
				palette  => [ $self->prelight_color($back_color), $back_color ],
			)-> bar( $left, $bottom, $right, $top, 0 );
		}
		$self-> color($c);
	} else {
		$canvas-> backColor($back_color) if $back_color;
		$canvas-> clear( $left, $bottom, $right, $top);
	}
}

1;

=head1 NAME

Prima::Widget::ListBoxUtils - common paint routine for listboxes

=head1 DESCRIPTION

To be used by list-like widgets

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Lists>

=cut
