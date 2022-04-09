# contains:
#   CheckList

package Prima::ExtLists;
use strict;
use warnings;
use Prima qw(Lists StdBitmap);

package Prima::CheckList;
use vars qw(@ISA);
@ISA = qw(Prima::ListBox);

my (@images, @imgSize);

Prima::Application::add_startup_notification( sub {
	@images = (
		Prima::StdBitmap::image(sbmp::CheckBoxUnchecked),
		Prima::StdBitmap::image(sbmp::CheckBoxChecked),
	);
	@imgSize = (0,0);
	@imgSize = $images[0]-> size if $images[0];
});

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		vector => '',
	);
	@$def{keys %prf} = values %prf;
	return $def;
}


sub init
{
	my $self = shift;
	$self-> {vector} = 0;
	my %profile = $self-> SUPER::init(@_);
	$self-> {fontHeight} = $self-> font-> height;
	$self-> recalc_icons;
	$self-> vector( $profile{vector});
	return %profile;
}

sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> get_text_width( $self-> {items}-> [$index]) + $imgSize[0] + 4;
}

sub recalc_icons
{
	return unless $imgSize[0];
	my $self = $_[0];
	my $hei = $self-> font-> height + 2;
	$hei = $imgSize[1] + 2 if $hei < $imgSize[1] + 2;
	$self-> itemHeight( $hei);
}

sub on_fontchanged
{
	my $self = shift;
	$self-> recalc_icons;
	$self-> {fontHeight} = $self-> font-> height;
	$self-> SUPER::on_fontchanged(@_);
}


sub draw_items
{
	shift-> std_draw_text_items( @_);
}

sub draw_text_items
{
	my ( $self, $canvas, $first, $last, $step, $x, $y, $textShift, $clipRect) = @_;
	my ( $i, $j);
	for ( $i = $first, $j = 1; $i <= $last; $i += $step, $j++) {
		next if $self-> {widths}-> [$i] + $self-> {offset} + $x + 1 < $clipRect-> [0];
		$canvas-> text_shape_out( $self-> {items}-> [$i], $x + 2 + $imgSize[0],
			$y + $textShift - $j * $self-> {itemHeight} + 1);
		$canvas-> put_image( $x + 1,
			$y + int(( $self-> {itemHeight} - $imgSize[1]) / 2) -
				$j * $self-> {itemHeight} + 1,
			$images[ vec($self-> {vector}, $i, 1)],
		);
	}
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	$self-> SUPER::on_mouseclick( $btn, $mod, $x, $y);
	return if $btn != mb::Left;
	my $foc = $self-> focusedItem;
	return if $foc < 0;
	my $f = vec( $self-> {vector}, $foc, 1) ? 0 : 1;
	vec( $self-> {vector}, $foc, 1) = $f;
	$self-> notify(q(Change), $foc, $f);
	$self-> invalidate_rect( $self-> item2rect( $foc));
}

sub on_click
{
	my $self = $_[0];
	my $foc = $self-> focusedItem;
	return if $foc < 0;
	my $f = vec( $self-> {vector}, $foc, 1) ? 0 : 1;
	vec( $self-> {vector}, $foc, 1) = $f;
	$self-> notify(q(Change), $foc, $f);
	$self-> invalidate_rect( $self-> item2rect( $foc));
}


sub vector
{
	my $self = $_[0];
	return $self-> {vector} unless $#_;
	$self-> {vector} = $_[1];
	$self-> repaint;
}

sub clear_all_buttons
{
	my $self = $_[0];
	my $c = int($self-> {count} / 32) + (($self-> {count} % 32) ? 1 : 0);
	vec( $self-> {vector}, $c, 32) = 0 while $c--;
	$self-> notify(q(Change), -1, 1);
	$self-> repaint;
};

sub set_all_buttons
{
	my $self = $_[0];
	my $c = int($self-> {count} / 32) + (($self-> {count} % 32) ? 1 : 0);
	vec( $self-> {vector}, $c, 32) = 0xffffffff while $c--;
	$self-> notify(q(Change), -1, 0);
	$self-> repaint;
};

sub button
{
	my ( $self, $index, $state) = @_;
	return 0 if $index < 0 || $index >= $self-> {count};
	my $current = vec( $self-> {vector}, $index, 1);
	return $current unless defined $state;
	$state = ( $state < 0) ? !$current : ( $state ? 1 : 0);
	return $current if $current == $state;
	vec( $self-> {vector}, $index, 1) = $state;
	$self-> notify(q(Change), $index, $state);
	$self-> redraw_items( $index);
	return $state;
}

#sub on_change
#{
#	my ( $self, $index, $state) = @_;
#}

1;

=pod

=head1 NAME

Prima::ExtLists - extended functionality for list boxes

=head1 SYNOPSIS

	use Prima qw(ExtLists Application);

	my $vec = '';
	vec( $vec, 0, 8) = 0x55;
	Prima::CheckList-> new(
		items  => [1..10],
		vector => $vec,
	);
	run Prima;

=for podview <img src="extlist.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/extlist.gif">

=head1 DESCRIPTION

The module is intended to be a collection of list boxes with
particular enhancements. Currently the only package is contained
is C<Prima::CheckList> class.

=head1 Prima::CheckList

Provides a list box, where each item is equipped with a check box.
The check box state can interactively be toggled by the enter key;
also the list box reacts differently by click and double click.

=head2 Properties

=over

=item button INDEX, STATE

Runtime only. Sets INDEXth button STATE to 0 or 1.
If STATE is -1, the button state is toggled.

Returns the new state of the button.

=item vector VEC

VEC is a vector scalar, where each bit corresponds to the check state
of each list box item.

See also: L<perlfunc/vec>.

=back

=head2 Methods

=over

=item clear_all_buttons

Sets all buttons to state 0

=item set_all_buttons

Sets all buttons to state 1

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Lists>, F<examples/extlist.pl>

=cut

