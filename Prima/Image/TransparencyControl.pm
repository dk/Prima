package Prima::Image::TransparencyControl;

use strict;
use warnings;
use Prima;
use Prima::ImageViewer;
use Prima::Label;
use Prima::Sliders;

use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		index  => 0,
		image  => undef,
		width  => 364,
		height => 158,
		designScale => [ 7, 16],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {imageColors} = 0;
	my %profile = $self-> SUPER::init(@_);

	$self-> insert( qq(Prima::ImageViewer) =>
		origin => [ 10, 40],
		name => 'Panel',
		size => [ 100, 100],
		borderWidth => 1,
		alignment =>  ta::Center,
		valignment => ta::Center,
		delegations => [ 'MouseDown'],
	);
	my $p = $self-> insert( qq(Prima::Widget) =>
		origin => [ 120, 40],
		name => 'Palette',
		size => [ 235, 100],
		delegations => [ 'Paint', 'MouseDown'],
		buffered => 1,
	);
	my $se = $self-> insert( qq(Prima::SpinEdit) =>
		origin => [ 120, 10],
		name => 'Index',
		size => [ 100, 20],
		delegations => [ 'Change'],
	);
	$self-> insert( qq(Prima::Label) =>
		origin => [ 10, 10],
		size => [ 105, 19],
		text => 'Color inde~x',
		focusLink => $se,
	);

	my @sz = $p-> size;
	my $sqd = 20;
	$sz[$_] -= 5 for 0,1;
	while ( $sqd-- > 1) {
		my @d = map { int($sz[$_] / $sqd)} 0, 1;
		last if $d[0] * $d[1] >= 256;
	}
	$p-> {sqd} = $sqd;
	$p-> {columns} = int( $sz[0] / $sqd);
	$p-> width( 4 + $p-> {columns} * $sqd);

	$self-> image( $profile{image});
	$self-> index( $profile{index});
	return %profile;
}

sub image
{
	return $_[0]-> {image} unless $#_;
	my ( $self, $i) = @_;
	$self-> {image} = $i;
	$self-> {imageColors} = scalar ( @{$self-> {image}-> palette}) / 3 if $i;
	$self-> Index-> max( $self-> {imageColors} - 1);
	return unless $self-> enabled;
	$self-> Panel-> image( $self-> {image});
	return unless $i;
	my @szA = $i-> size;
	my @szB = $self-> Panel->get_active_area(2);
	my $xx = $szB[0]/$szA[0];
	my $yy = $szB[1]/$szA[1];
	$self-> Panel-> zoom( $xx < $yy ? $xx : $yy);
}

sub index
{
	return $_[0]-> Index-> value unless $#_;
	my ( $self, $i) = @_;
	my $v = $self-> Index-> value;
	$i = 0 if $i < 0;
	$i = $self-> {imageColors} - 1 if $i >= $self-> {imageColors};
	return if $v == $i;
	$self-> Index-> value( $_[1]);
	$self-> Palette-> repaint;
	$self-> notify(q(Change));
}

sub Index_Change
{
	$_[0]-> index( $_[1]-> value);
	$_[0]-> Palette-> repaint;
}

sub on_enable
{
	my $self = $_[0];
	$_-> enabled( 1) for $self-> widgets;
	$self-> Panel-> image( $self-> {image});
	return unless $self-> {image};
	my @szA = $self-> {image}->size;
	my @szB = $self-> Panel->get_active_area(2);
	my $xx = $szB[0]/$szA[0];
	my $yy = $szB[1]/$szA[1];
	$self-> Panel-> zoom( $xx < $yy ? $xx : $yy);
	$self-> Palette-> repaint;
}

sub on_disable
{
	my $self = $_[0];
	$_-> enabled( 0) for $self-> widgets;
	$self-> Panel-> image( undef);
	$self-> Palette-> repaint;
}

sub Panel_MouseDown
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	( $x, $y) = $self-> screen2point( $x, $y);
	my @sz = $owner-> {image}-> size;
	return if $x < 0 || $x >= $sz[0] || $y < 0 || $y >= $sz[1];
	my $pix = $owner-> {image}-> pixel( $x, $y);
	my $pal = $owner-> {image}-> palette;
	my $i;
	my $c = $owner-> {imageColors};
	my ( $r, $g, $b) = cl::to_rgb($pix);
	for ( $i = 0; $i < $c; $i++) {
		last if $pal->[ $i * 3 + 0] == $b &&
				$pal->[ $i * 3 + 1] == $g &&
				$pal->[ $i * 3 + 2] == $r;
	}
	return if $i == $c;
	$owner-> index( $i);
}

sub Palette_Paint
{
	my ( $owner, $self, $canvas) = @_;
	my @sz = $self-> size;
	my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> rect3d( 0, 0, $sz[0]-1, $sz[1]-1, 1, reverse(@c3d), $self-> backColor);
	return unless $owner-> {image};
	my $c = $owner-> {imageColors};
	my $p = $owner-> {image}-> palette;
	my $x = 2;
	my $s = $self-> {sqd};
	my $y = $sz[1] - 2 - $s;
	my $i;
	my $e = $self-> enabled;
	my $cl = $self-> {columns};
	my $ci = 0;
	my $se = $owner-> index;
	$se = -1 unless $owner-> enabled;
	my $bwo = ( $s > 7) ? 1 : 0;
	for ( $i = 0; $i < $c; $i++) {
		$canvas-> rect3d( $x, $y, $x + $s - 1, $y + $s - 1,
		$bwo + (($se == $i) ? 1 : 0),
			( $se == $i) ? reverse(@c3d) : @c3d,
			$e ? ( $p->[$i*3] + $p-> [$i*3+1] * 256 + $p-> [$i*3+2] * 65536) : ()
		);
		$x += $s;
		$x = 2, $y -= $s, $ci = 0 if ++$ci == $cl;
	}
}

sub Palette_MouseDown
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	my @sz = $self-> size;
	my $c = $owner-> {imageColors};
	$self-> clear_event;
	$x = int(( $x - 2) / $self-> {sqd});
	$y = int(( $sz[1] - $y - 3) / $self-> {sqd});
	return if $x >= $self-> {columns};
	return if $y * $self-> {columns} + $x >= $c;
	$owner-> index( $y * $self-> {columns} + $x);
}

package Prima::Image::BasicTransparencyDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		width    => 480,
		height   => 206,
		centered => 1,
		designScale => [ 7, 16],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> insert( qq(Prima::CheckBox) =>
		origin => [ 3, 167],
		name => 'Transparent',
		size => [ 133, 36],
		text => '~Transparent',
		delegations => ['Check'],
	);
	$self-> insert( qq(Prima::Image::TransparencyControl) =>
		origin => [ 3, 6],
		size => [ 364, 158],
		text => '',
		name => 'TC',
	);
	$self-> insert( qq(Prima::Button) =>
		origin => [ 379, 165],
		name => 'OK',
		size => [ 96, 36],
		text => '~OK',
		default => 1,
		modalResult => mb::OK,
		delegations => ['Click'],
	);
	$self-> insert( qq(Prima::Button) =>
		origin => [ 379, 120],
		size => [ 96, 36],
		text => 'Cancel',
		modalResult => mb::Cancel,
	);
	return %profile;
}


sub transparent
{
	my $self = $_[0];
	$self-> Transparent-> checked( $_[1]);
	$self-> TC-> enabled( $_[1]);
}

sub Transparent_Check
{
	my ( $self, $tr) = @_;
	$self-> transparent( $tr-> checked);
}

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	return unless $image;
	$self-> transparent( $image-> {extras}-> {transparentColorIndex} ? 1 : 0);
	$self-> TC-> image( $image);
	$self-> TC-> index( exists( $image-> {extras}-> {transparentColorIndex}) ?
		$image-> {extras}-> {transparentColorIndex} : 0);
}

sub OK_Click
{
	my $self = $_[0];
	if ( $self-> Transparent-> checked) {
		$self-> {image}-> {extras}-> {transparentColorIndex} = $self-> TC-> index;
	} else {
		delete $self-> {image}-> {extras}-> {transparentColorIndex};
	}
	delete $self-> {image};
	$self-> TC-> image( undef);
}

1;

=pod

=head1 NAME

Prima::Image::TransparencyControl - standard dialog
for transparent color index selection.

=head1 DESCRIPTION

The module contains two classes - C<Prima::Image::BasicTransparencyDialog>
and C<Prima::Image::TransparencyControl>. The former provides a dialog,
used by image codec-specific save options dialogs to select a transparent
color index when saving an image to a file. C<Prima::Image::TransparencyControl>
is a widget class that displays the image palette and allow color rather than
index selection.

=head1 Prima::Image::TransparencyControl

=head2 Properties

=over

=item index INTEGER

Selects the palette index.

=item image IMAGE

Selects image which palette is displayed, and the color
index can be selected from.

=back

=head2 Events

=over

=item Change

Triggered when the user changes C<index> property.

=back

=head1 Prima::Image::BasicTransparencyDialog

=head2 Methods

=over

=item transparent BOOLEAN

If 1, the transparent selection widgets are enabled, and the
user can select the palette index. If 0, the widgets are
disabled; the image file is saved with no transparent color index.

The property can be toggled interactively by a checkbox.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Dialog::ImageDialog>.

=cut
