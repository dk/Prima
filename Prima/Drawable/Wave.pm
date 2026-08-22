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

sub process
{
	my ($vertices, $amplitude, $wavelength, $cb) = @_;

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
		push @$points, $x0, $y0;
		my $pp = $cb->($length);
		$pp = Prima::Drawable->render_polyline($pp, matrix => $matrix) if $pp;
		Prima::array::append($points, $pp) if $pp;
	}

	Prima::array::deduplicate($points, 2, 4);

	return $points;
}

sub square
{
	my ( $vertices, $amplitude, $wavelength) = @_;
	($amplitude, $wavelength) = check($vertices, $amplitude, $wavelength);
	my $half_wave  = $wavelength / 2;
	my $quart_wave = $wavelength / 4;

	return process( $vertices, $wavelength, $amplitude, sub {
		my ($length) = @_;
		my @ret;

		# body
		my $full_waves = int($length / $wavelength);
		for my $wave (0 .. $full_waves - 1) {
			my $s = $wave * $wavelength;
			push @ret,
				$s              , +$amplitude,
				$s + $half_wave , +$amplitude,
				$s + $half_wave , -$amplitude,
				$s + $wavelength, -$amplitude,
			;
		}

		# tail
		my $ofs  = $wavelength * $full_waves;
		my $tail = $length - $ofs;
		if ( $tail > $quart_wave ) {
			$tail -= $quart_wave;
			push @ret, $ofs              , +$amplitude;
		}
		if ( $tail > $quart_wave ) {
			$tail -= $quart_wave;
			push @ret, $ofs + $half_wave , +$amplitude;
		}
		if ( $tail > $quart_wave ) {
			push @ret, $ofs + $half_wave , -$amplitude;
		}

		return \@ret;
	});
}

sub triangle
{
	my ( $vertices, $amplitude, $wavelength) = @_;
	($amplitude, $wavelength) = check($vertices, $amplitude, $wavelength);
	my $half_wave  = $wavelength / 2;
	my $quart_wave = $wavelength / 4;
	return process( $vertices, $wavelength, $amplitude, sub {
		my ($length) = @_;
		my @ret;

		# body
		my $full_waves = int($length / $wavelength);
		for my $wave (0 .. $full_waves - 1) {
			my $s = $wave * $wavelength;
			push @ret,
				$s + $quart_wave, +$amplitude,
				$s + $half_wave + $quart_wave, -$amplitude;
		}

		# tail
		my $ofs  = $wavelength * $full_waves;
		my $tail = $length - $ofs;
		if ( $tail > $quart_wave ) {
			$tail -= $quart_wave;
			push @ret, $ofs + $quart_wave , +$amplitude;
		}
		if ( $tail > $half_wave) {
			push @ret, $ofs + $half_wave + $quart_wave, -$amplitude;
		} elsif ( $tail > $quart_wave ) {
			push @ret, $ofs + $half_wave , 0;
		}

		return \@ret;
	});
}

sub spline
{
	my ($vertices, $amplitude, $wavelength, $precision) = @_;
	return Prima::Drawable->render_spline(
		square($vertices, $amplitude, $wavelength),
		($precision ? (precision => $precision) : ())
	);
}

sub polyline
{
	my ( $effect, $vertices, @opt ) = @_;
	if ( $effect eq 'square') {
		return square($vertices, @opt);
	} elsif ( $effect eq 'spline') {
		return spline($vertices, @opt);
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

	my $effect = $opt{effect} // 'spline';
	my $p = polyline($effect, \@p, $amplitude, undef, $opt{precision});
	$canvas->polyline($p);
}

1;

__END__

=pod

=head1 NAME

Prima::Drawable::Wave - plot wavy lines

=head1 DESCRIPTION

Collection of routines for generating and plotting wavy lines.
Can render triangle, square, and spline effects.

=head1 SYNOPSIS

  use Prima::Drawable::Wave;

  $canvas->antialias(1);
  my $points = Prima::Drawable::Wave::square(
     [20, 100, 100, 60, 180, 120, 270, 80],
     4, 20,
  );
  $canvas->lineWidth(.5);
  $canvas->spline( $canvas-> render_spline($points) );

  $canvas->font->size(20);
  $canvas->text_out('Hello world', 70, 80);
  $canvas->lineWidth($canvas->font->underlineThickness);
  $canvas->alpha(128);
  Prima::Drawable::Wave::underline($canvas, 'Hello world', 70, 80, effect => 'triangle');

=for podview <img src="Prima/wave.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/wave.gif">

=head1 API

=head2 underline CANVAS, TEXT, X, Y, %OPT

Given TEXT, draws a way line in the X and Y coordinates.
Uses C<$OPT{effect}> or C<'wave'> as the line effect, and C<$OPT{precision}> for eventual spline precision.

Can only draw a single wavy underline; check L<Prima::Drawable/draw_underline>
for wavy effects with smart underline that respect font glyphs that cross the
descent line.

=head2 polyline EFFECT, VERTICES, AMPLITUDE, WAVENELNGTH, @OPTIONS

Converts VERTICES to another set of vertices for generation of a wavy line.
The result can directly be passed to the C<Prima::Drawable::polyline> method.

Optional AMPLITUDE and WAVENELNGTH manage the effect settings.

Eventual C<@OPTIONS> are passed to the effect function, see below.

=head2 square VERTICES, AMPLITUDE, WAVENELNGTH

Creates a square wave line

=head2 triangle VERTICES, AMPLITUDE, WAVENELNGTH

Creates a triangle wave line

=head2 spline VERTICES, AMPLITUDE, WAVENELNGTH, PRECISION

Creates a spline wave line

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable::Path>

=cut

