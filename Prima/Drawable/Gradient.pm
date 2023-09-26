package Prima::Drawable::Gradient;

use strict;
use warnings;
use Prima;

sub new
{
	my ( $class, $canvas, %opt) = @_;
	bless {
		canvas  => $canvas,
		palette => [ cl::White, cl::Black ],
		%opt
	}, $class;
}

sub clone
{
	my ( $self, %opt ) = @_;
	return ref($self)->new( undef, %$self, %opt );
}

sub canvas  { shift->{canvas} }
sub palette { shift->{palette} }

sub polyline_to_points
{
	my ($self, $p) = @_;
	my @map;
	for ( my $i = 0; $i < @$p - 2; $i+=2) {
		my ($x1,$y1,$x2,$y2) = @$p[$i..$i+3];
		$x1 = 0 if $x1 < 0;
		my $dx = $x2 - $x1;
		if ( $dx > 0 ) {
			my $dy = ($y2 - $y1) / $dx;
			my $y = $y1;
			for ( my $x = int($x1); $x <= int($x2); $x++) {
				$map[$x] = $y;
				$y += $dy;
			}
		} else {
			$map[int($x1)] = $y1;
		}
	}
	return \@map;
}

sub stripes
{
	my ( $self, $breadth) = @_;

	my ($offsets, $points);

	unless ($points = $self->{points}) {
		my @spline = (0,0);
		if ( my $s = $self->{spline} ) {
			push @spline, map { $_ * $breadth } @$s;
		}
		if ( my $s = $self->{poly} ) {
			push @spline, map { $_ * $breadth } @$s;
		}
		push @spline, $breadth, $breadth;
		my $polyline = ( @spline > 4 && $self->{spline} ) ? Prima::Drawable-> render_spline( \@spline ) : \@spline;
		$points = $self-> polyline_to_points($polyline);
	}


	unless ( $offsets = $self->{offsets}) {
		my @o;
		my $n = scalar(@{$self->{palette}}) - 1;
		my $d = 0;
		for ( my $i = 0; $i < $n; $i++) {
			$d += 1/$n;
			push @o, $d;
		}
		push @o, 1;
		$offsets = \@o;
	}

	return $self-> calculate(
		$self->{palette},
		[ map { $_ * $breadth } @$offsets ],
		sub { $points->[shift] }
	);
}

sub colors
{
	my ( $self, $breadth) = @_;

	my $stripes = $self->stripes($breadth);
	my @colors;
	for ( my $i = 0; $i < @$stripes; $i+=2 ) {
		push @colors, $stripes->[$i] for 1 .. $stripes->[$i+1];
	}
	return @colors;
}

sub map_color
{
	my ( $self, $color ) = @_;
	return $color unless $color & cl::SysFlag;
	$color |=
		$self->{widgetClass} //
		(( $self->{canvas} && $self->{canvas}->isa('Prima::Widget')) ? $self->{canvas}->widgetClass : undef ) //
		wc::Undef
		unless $color & wc::Mask;
	return $::application->map_color($color);
}

sub calculate_single
{
	my ( $self, $breadth, $start_color, $end_color, $function, $offset ) = @_;

	return if $breadth <= 0;

	$offset //= 0;
	$start_color = $self-> map_color( $start_color);
	$end_color   = $self-> map_color( $end_color);
	my @start = cl::to_rgb($start_color);
	my @end   = cl::to_rgb($end_color);
	my @color = @start;
	return $start_color, 1 if $breadth == 1;

	my @delta = map { ( $end[$_] - $start[$_] ) / ($breadth - 1) } 0..2;

	my $last_color = $start_color;
	my $color      = $start_color;
	my $width      = 0;
	my @ret;
	for ( my $i = 0; $i < $breadth; $i++) {
		my @c;
		my $j = $function ? $function->( $offset + $i ) - $offset : $i;
		for ( 0..2 ) {
			$color[$_] = $start[$_] + $j * $delta[$_] for 0..2;
			$c[$_] = int($color[$_] + .5);
			$c[$_] = 0xff if $c[$_] > 0xff;
			$c[$_] = 0    if $c[$_] < 0;
		}
		$color = ( $c[0] << 16 ) | ( $c[1] << 8 ) | $c[2];
		if ( $last_color != $color ) {
			push @ret, $last_color, $width;
			$last_color = $color;
			$width = 0;
		}

		$width++;
	}

	return @ret, $color, $width;
}

sub calculate
{
	my ( $self, $palette, $offsets, $function ) = @_;
	my @ret;
	my $last_offset = 0;
	$offsets = [ $offsets ] unless ref $offsets;
	for ( my $i = 0; $i < @$offsets; $i++) {
		my $breadth = $offsets->[$i] - $last_offset;
		push @ret, $self-> calculate_single( $breadth, $palette->[$i], $palette->[$i+1], $function, $last_offset);
		$last_offset = $offsets->[$i];
	}
	return \@ret;
}

sub bar
{
	my ( $self, $x1, $y1, $x2, $y2, $vertical ) = @_;

	$_ = int($_) for $x1, $y1, $x2, $y2;

	($x1,$x2)=($x2,$x1) if $x1 > $x2;
	($y1,$y2)=($y2,$y1) if $y1 > $y2;

	$vertical //= $self->{vertical};
	my $stripes = $self-> stripes(
		$vertical ?
			$x2 - $x1 + 1 :
			$y2 - $y1 + 1
	);

	my @bar        = ($x1,$y1,$x2,$y2);
	my ($ptr1,$ptr2) = $vertical ? (0,2) : (1,3);
	my $max          = $bar[$ptr2];
	my $canvas       = $self->canvas;
	for ( my $i = 0; $i < @$stripes; $i+=2) {
		$bar[$ptr2] = $bar[$ptr1] + $stripes->[$i+1] - 1;
		$canvas->color( $stripes->[$i]);
		$canvas->bar( @bar );
		$bar[$ptr1] = $bar[$ptr2] + 1;
		last if $bar[$ptr1] > $max;
	}
	if ( $bar[$ptr1] <= $max ) {
		$bar[$ptr2] = $max;
		$canvas->bar(@bar);
	}
}

sub ellipse
{
	my ( $self, $x, $y, $dx, $dy ) = @_;
	return if $dx <= 0 || $dy <= 0;

	$_ = int($_) for $x, $y, $dx, $dy;
	my $diameter = ($dx > $dy) ? $dx : $dy;
	my $mx = $dx / $diameter;
	my $my = $dy / $diameter;
	my $stripes = $self-> stripes( $diameter);
	my $canvas  = $self->canvas;
	for ( my $i = 0; $i < @$stripes; $i+=2) {
		$canvas->color( $stripes->[$i]);
		$canvas->fill_ellipse( $x, $y, $mx * $diameter, $my * $diameter );
		$diameter -= $stripes->[$i+1];
	}
}

sub sector
{
	my ( $self, $x, $y, $dx, $dy, $start, $end ) = @_;
	my $angle = $end - $start;
	$angle += 360 while $angle < 0;
	$angle %= 720;
	$angle -= 360 if $angle > 360;
	my $min_angle = 1.0 / 64; # x11 limitation

	my $max = ($dx < $dy) ? $dy : $dx;
	my $df  = $max * 3.14 / 360;
	my $arclen = int($df * $angle + .5);
	my $stripes = $self-> stripes( $arclen );
	my $accum = 0;
	my $canvas = $self->canvas;
	for ( my $i = 0; $i < @$stripes - 2; $i+=2) {
		$canvas->color( $stripes->[$i]);
		my $d = $stripes->[$i+1] / $df;
		if ( $accum + $d < $min_angle ) {
			$accum += $d;
			next;
		}
		$d += $accum;
		$accum = 0;
		$canvas->fill_sector( $x, $y, $dx, $dy, $start, $start + $d + $min_angle);
		$start += $d;
	}
	if ( @$stripes ) {
		$canvas->color( $stripes->[-2]);
		$canvas->fill_sector( $x, $y, $dx, $dy, $start, $end);
	}
}

1;

=pod

=head1 NAME

Prima::Drawable::Gradient - gradient fills for primitives

=head1 DESCRIPTION

Prima offers primitive gradient services to draw gradually changing colors.
A gradient is requested by setting of at least two colors and optionally
a set of quadratic spline points that, when, projected, generate the transition curve
between the colors.

The module augments the C<Prima::Drawable> drawing functionality by
adding C<new_gradient> function.

=head1 SYNOPSIS

	$canvas-> new_gradient(
		palette => [ cl::White, cl::Blue, cl::White ],
	)-> sector(50,50,100,100,0,360);

=for podview <img src="Prima/gradient.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/gradient.gif">

=head1 API

=head2 Methods

=over

=item new $CANVAS, %OPTIONS

Here are %OPTIONS understood in the gradient request:

=over

=item clone %OPTIONS

Creates a new gradient object with %OPTIONS replaced.

=item widgetClass INTEGER

Points to a widget class to resolve generic colors like C<cl::Back>,
that may differ from widget class to widget class.

=item palette @COLORS

Each color is a C<cl::> value. The gradient is calculated as polyline where
each its vertex corresponds to a certain blend between two neighbouring colors
in the palette. F.ex. the simplest palette going from C<cl::White> to
C<cl::Black> over a polyline C<0..1> (default), produces pure white color at
the start and pure black color at the end, filling all available shades of gray
in between, and changing monotonically.

=item poly @VERTICES

Set of 2-integer polyline vertexes where the first integer is a coordinate (x,
y, or whatever required by the drawing primitive) between 0 and 1, and the
second is the color blend value between 0 and 1.

Default: ((0,0),(1,1))

=item spline \@VERTICES, %OPTIONS

Serving same purpose as C<poly> but vertexes are projected first to a B-spline
curve using L<render_spline> and C<%OPTIONS>. The resulting polyline is treated
as C<poly>.

=item vertical BOOLEAN

Only used in L<bar>, to set gradient direction.

=back

See also: L<bar>, L<stripes> .

=item bar X1, Y1, X2, Y2, VERTICAL = 0

Draws a filled rectangle within (X1,Y1) - (X2,Y2) extents

Context used: fillPattern, rop, rop2

=item colors BREADTH

Returns a list of gradient colors for each step from 1 to BREADTH.

=item ellipse X, Y, DIAM_X, DIAM_Y

Draws a filled ellipse with the center in (X,Y) and diameters (DIAM_X,DIAM_Y)

Context used: fillPattern, rop, rop2

=item sector X, Y, DIAM_X, DIAM_Y, START_ANGLE, END_ANGLE

Draws a filled sector with the center in (X,Y) and diameters (DIAM_X,DIAM_Y) from START_ANGLE to END_ANGLE

Context used: fillPattern, rop, rop2

=item stripes BREADTH

Returns an array consisting of integer pairs, where the first one is
a color value, and the second is the breadth of the color strip.
L<bar> uses this information to draw a gradient fill, where
each color strip is drawn with its own color. Can be used for implementing
other gradient-aware primitives (see F<examples/f_fill.pl> )

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable>, F<examples/f_fill.pl>, F<examples/gradient.pl>

=cut
