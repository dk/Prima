package Prima::Drawable::Wave;

use strict;
use warnings;
use Prima;

sub check
{
	my ($vertices, $amplitude, $wavelength) = @_;

	$wavelength //= (defined $amplitude) ? $amplitude * 4 : 16;
	$amplitude	//= $wavelength / 4;
	Carp::croak "wavelength must be > 0"		  unless $wavelength > 0;
	Carp::croak "amplitude must be >= 0"		  unless $amplitude >= 0;
	Carp::croak "need at least two vertices"	  unless @$vertices >= 4;
	Carp::croak "vertices must contain x,y pairs" if @$vertices % 2;

	return ($amplitude, $wavelength);
}

sub square
{
	my ($vertices, $amplitude, $wavelength) = @_;

	($amplitude, $wavelength) = check($vertices, $amplitude, $wavelength);
	my $half_wave  = $wavelength / 2;
	my $quart_wave = $wavelength / 4;

	my $points = Prima::array->new_double;
	push @$points, @$vertices[0,1];

	# process baselines
	for (my $v = 0; $v < @$vertices - 2; $v += 2) {
		my ($x0,$y0,$x1,$y1) = @$vertices[$v..$v+3];

		my $dx = $x1 - $x0;
		my $dy = $y1 - $y0;
		my $length = sqrt($dx * $dx + $dy * $dy) or next;

		my $tx = $dx / $length;
		my $ty = $dy / $length;
		my $matrix = Prima::array->new_cmatrix(
			$tx,  $ty,
			-$ty, $tx,
			$x0,  $y0
		);

		# process full waves
		my $full_waves = int($length / $wavelength);

		push @$points, $x0, $y0;
		for my $wave (0 .. $full_waves - 1) {
			my $s = $wave * $wavelength;
			Prima::array::append($points, Prima::Drawable->render_polyline([
				$s              , +$amplitude,
				$s + $half_wave , +$amplitude,
				$s + $half_wave , -$amplitude,
				$s + $wavelength, -$amplitude,
			], matrix => $matrix));
		}

		# any quarter-waves left?
		my @ptail;
		my $ofs  = $wavelength * $full_waves;
		my $tail = $length - $ofs;
		if ( $tail > $quart_wave ) {
			$tail -= $quart_wave;
			push @ptail, $ofs              , +$amplitude;
		}
		if ( $tail > $quart_wave ) {
			$tail -= $quart_wave;
			push @ptail, $ofs + $half_wave , +$amplitude;
		}
		if ( $tail > $quart_wave ) {
			push @ptail, $ofs + $half_wave , -$amplitude;
		}

		Prima::array::append($points, Prima::Drawable->render_polyline(\@ptail, matrix => $matrix))
			if @ptail;
	}

	Prima::array::deduplicate($points, 2, 4);

	return $points;
}

sub triangle
{
	my ($vertices, $amplitude, $wavelength) = @_;

	($amplitude, $wavelength) = check($vertices, $amplitude, $wavelength);
	my $half_wave  = $wavelength / 2;
	my $quart_wave = $wavelength / 4;

	my $points = Prima::array->new_double;
	push @$points, @$vertices[0,1];

	# process baselines
	for (my $v = 0; $v < @$vertices - 2; $v += 2) {
		my ($x0,$y0,$x1,$y1) = @$vertices[$v..$v+3];

		my $dx = $x1 - $x0;
		my $dy = $y1 - $y0;
		my $length = sqrt($dx * $dx + $dy * $dy) or next;

		my $tx = $dx / $length;
		my $ty = $dy / $length;
		my $matrix = Prima::array->new_cmatrix(
			$tx,  $ty,
			-$ty, $tx,
			$x0,  $y0
		);

		# process full waves
		my $full_waves = int($length / $wavelength);

		push @$points, $x0, $y0;
		for my $wave (0 .. $full_waves - 1) {
			my $s = $wave * $wavelength;
			Prima::array::append($points, Prima::Drawable->render_polyline([
				$s + $quart_wave , +$amplitude,
				$s + $half_wave + $quart_wave, -$amplitude,
			], matrix => $matrix));
		}

		# any quarter-waves left?
		my @ptail;
		my $ofs  = $wavelength * $full_waves;
		my $tail = $length - $ofs;
		if ( $tail > $quart_wave ) {
			$tail -= $quart_wave;
			push @ptail, $ofs + $quart_wave , +$amplitude;
		}
		if ( $tail > $half_wave) {
			push @ptail, $ofs + $half_wave + $quart_wave, -$amplitude;
		} elsif ( $tail > $quart_wave ) {
			push @ptail, $ofs + $half_wave , 0;
		}

		Prima::array::append($points, Prima::Drawable->render_polyline(\@ptail, matrix => $matrix))
			if @ptail;
	}

	Prima::array::deduplicate($points, 2, 4);

	return $points;
}

sub wave
{
	my ($vertices, $amplitude, $wavelength, $precision) = @_;
	return Prima::Drawable->render_spline(
		square($vertices, $amplitude, $wavelength),
		($precision ? (precision => $precision) : ())
	);
}

sub line
{
	my ( $effect, $vertices, @opt ) = @_;
	if ( $effect eq 'square') {
		return square($vertices, @opt);
	} elsif ( $effect eq 'wave') {
		return wave($vertices, @opt);
	} elsif ( $effect eq 'triangle') {
		return triangle($vertices, @opt);
	} else {
		Carp::carp "no such effect: $effect";
		return;
	}
}

sub underline
{
	my ( $canvas, $text, $x, $y, %opt ) = @_;

	my $up = abs($canvas->font->underlinePosition);
	$up = 0 if $up < 0;
	$y += $up;
	my @p = ($x,$y,$x + $canvas->get_text_width($text),$y);
	my $amplitude = $up / 2;
	$amplitude = 2 if $amplitude < 2;

	my $effect = $opt{effect} // 'wave';
	my $p = line($effect, \@p, $amplitude, undef, $opt{precision});
	$canvas->polyline($p);
}

1;

__END__

=pod

=head1 NAME

Prima::Drawable::Wave - plot wavy lines

=head1 DESCRIPTION

.

=head1 SYNOPSIS

  use Prima qw(Application Drawable::Wave);

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable::Path>

=cut

