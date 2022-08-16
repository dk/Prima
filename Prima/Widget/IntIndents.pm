package Prima::Widget::IntIndents;

use strict;
use warnings;
use Prima;

sub indents
{
	return wantarray ? @{$_[0]-> {indents}} : [@{$_[0]-> {indents}}] unless $#_;
	my ( $self, @indents) = @_;
	@indents = @{$indents[0]} if ( scalar(@indents) == 1) && ( ref($indents[0]) eq 'ARRAY');
	for ( @indents) {
		$_ = 0 if $_ < 0;
	}
	$self-> {indents} = \@indents;
}

sub get_active_area
{
	my @r = ( scalar @_ > 2) ? @_[2,3] : $_[0]-> size;
	my $i = $_[0]-> {indents};
	if ( !defined($_[1]) || $_[1] == 0) {
		# returns inclusive - exclusive
		return $$i[0], $$i[1], $r[0] - $$i[2], $r[1] - $$i[3];
	} elsif ( $_[1] == 1) {
		# returns inclusive - inclusive
		return $$i[0], $$i[1], $r[0] - $$i[2] - 1, $r[1] - $$i[3] - 1;
	} else {
		# returns size
		return $r[0] - $$i[0] - $$i[2], $r[1] - $$i[1] - $$i[3];
	}
}

1;

=head1 NAME

Prima::Widget::IntIndents - indenting support

=head1 DESCRIPTION

Provides the common functionality for the widgets that delegate part of their
surface to the border elements. A list box can be of an example, where its
scroll bars and 3-d borders are such elements.

=head1 Properties

=over

=item indents ARRAY

Contains four integers, specifying the breadth of decoration elements for
each side. The first integer is width of the left element, the second - height
of the lower element, the third - width of the right element, the fourth - height
of the upper element.

The property can accept and return the array either as a four scalars, or as
an anonymous array of four scalars.

=back

=head2 Methods

=over

=item get_active_area [ TYPE = 0, WIDTH, HEIGHT ]

Calculates and returns the extension of the area without the border elements,
or the active area.
The extension are related to the current size of a widget, however, can be
overridden by specifying WIDTH and HEIGHT. TYPE is an integer, indicating
the type of calculation:

=over

=item TYPE = 0

Returns four integers, defining the area in the inclusive-exclusive coordinates.

=item TYPE = 1

Returns four integers, defining the area in the inclusive-inclusive coordinates.

=item TYPE = 2

Returns two integers, the size of the area.

=back

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>

=cut
