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
		dither  => 0,
		%opt
	}, $class;
}

sub clone
{
	my ( $self, %opt ) = @_;
	return ref($self)->new( undef, %$self, %opt );
}

sub canvas  { shift->{canvas}  }
sub palette { shift->{palette} }
sub dither  { shift->{dither}  }

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
	return $start_color, 1 if $breadth == 1;

	my @delta = map { ( $end[$_] - $start[$_] ) / ($breadth - 1) } 0..2;

	my $last_color;
	my $color;
	if ( $self->{dither}) {
		$color = $last_color = [ $start_color, $start_color, 0 ];
	} else {
		$color = $last_color = $start_color;
	}

	my $width      = 0;
	my @ret;

	for ( my $i = 0; $i < $breadth; $i++) {
		my @c;
		my $j = $function ? $function->( $offset + $i ) - $offset : $i;
		if ( $self->{dither}) {
			for ( 0..2 ) {
				my $c = $start[$_] + $j * $delta[$_];
				$c = 255 if $c > 255;
				$c = 0   if $c < 0;
				push @c, int( $c * 64 + .5 ) / 64;
			}
			my $color = \@c;
			if ( join('.', @$last_color) ne join('.', @$color )) {
				my ($c1, $c2, $fp);
				if ( 3 == grep { $_ == int } @$last_color) {
					$c1 = $c2 = ( $last_color->[0] << 16 ) | ( $last_color->[1] << 8 ) | $last_color->[2];
					$fp = 0;
				} else {
					my ($d1,$d2,@l,@r) = (0,0);
					for ( @$last_color ) {
						my $l = int($_);
						my $r = $l + (($l != $_ && $_ < 255) ? 1 : 0);
						push @l, $l;
						push @r, $r;
						$d1 += ($_ - $l) * ($_ - $l);
						$d2 += ($r - $l) * ($r - $l);
					}
					if ( $d2 > 0 ) {
						$fp = int(sqrt($d1 / $d2) * 64 );
						$c1 = ( $l[0] << 16 ) | ( $l[1] << 8 ) | $l[2];
						$c2 = ( $r[0] << 16 ) | ( $r[1] << 8 ) | $r[2];
					} else {
						$c1 = $c2 = ( $l[0] << 16 ) | ( $l[1] << 8 ) | $l[2];
						$fp = 0;
					}
				}
				my $new_stripe = 1;
				if ( @ret ) {
					my $p = $ret[-2];
					if ( $p->[0] == $c1 && $p->[1] == $c2 && $p->[2] == $fp) {
						$ret[-1] += $width;
						$new_stripe = 0;
					}
				}
				push @ret, [$c1, $c2, $fp], $width if $new_stripe;
				$last_color = $color;
				$width = 0;
			}
		} else {
			for ( 0..2 ) {
				my $c = $start[$_] + $j * $delta[$_];
				$c[$_] = int($c + .5);
				$c[$_] = 0xff if $c[$_] > 0xff;
				$c[$_] = 0    if $c[$_] < 0;
			}
			$color = ( $c[0] << 16 ) | ( $c[1] << 8 ) | $c[2];
			if ( $last_color != $color ) {
				push @ret, $last_color, $width;
				$last_color = $color;
				$width = 0;
			}
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

my @map_halftone8x8_64 = (
	0, 47, 12, 59,  3, 50, 15, 62,
	31, 16, 43, 28, 34, 19, 46, 31,
	8, 55,  4, 51, 11, 58,  7, 54,
	39, 24, 35, 20, 42, 27, 38, 23,
	2, 49, 14, 61,  1, 48, 13, 60,
	33, 18, 45, 30, 32, 17, 44, 29,
	10, 57,  6, 53,  9, 56,  5, 52,
	41, 26, 37, 22, 40, 25, 36, 21
);

sub fp
{
	my $fp = shift;
	return fp::Solid unless $fp;

	my @p;
	for ( my $i = 0; $i < 64; $i += 8) {
		push @p, 0;
		$p[-1] |= $_ for map { 1 << $_ } grep { $map_halftone8x8_64[$i + $_] > $fp } 0..7;
	}
	return \@p;
}

sub init_brush
{
	my $self = shift;
	return 0 unless $self->{canvas}->graphic_context_push;
	if ( $self->{dither}) {
		$self->{canvas}->rop2(rop::CopyPut);
		$self->{canvas}->fillMode(fm::Winding);
	} else {
		$self->{canvas}->fillPattern(fp::Solid);
	}
	return 1;
}

sub apply_brush
{
	my ( $self, $stripe) = @_;
	if ( $self->{dither}) {
		$self->{canvas}->set(
			color             => $stripe->[0],
			backColor         => $stripe->[1],
			fillPattern       => fp($stripe->[2]),
		);
	} else {
		$self->{canvas}->color($stripe);
	}
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
	return unless $self->init_brush;

	for ( my $i = 0; $i < @$stripes; $i+=2) {
		$bar[$ptr2] = $bar[$ptr1] + $stripes->[$i+1] - 1;
		$self->apply_brush($stripes->[$i]);
		$canvas->bar( @bar );
		$bar[$ptr1] = $bar[$ptr2] + 1;
		last if $bar[$ptr1] > $max;
	}
	if ( $bar[$ptr1] <= $max ) {
		$bar[$ptr2] = $max;
		$canvas->bar(@bar);
	}
	$canvas->graphic_context_pop;
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
	return unless $self->init_brush;
	for ( my $i = 0; $i < @$stripes; $i+=2) {
		$self->apply_brush($stripes->[$i]);
		$canvas->fill_ellipse( $x, $y, $mx * $diameter, $my * $diameter );
		$diameter -= $stripes->[$i+1];
	}
	$canvas->graphic_context_pop;
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
	return unless $self->init_brush;
	for ( my $i = 0; $i < @$stripes - 2; $i+=2) {
		$self->apply_brush($stripes->[$i]);
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
		$self->apply_brush($stripes->[-2]);
		$canvas->fill_sector( $x, $y, $dx, $dy, $start, $end);
	}
	$canvas->graphic_context_pop;
}

1;

=pod

=head1 NAME

Prima::Drawable::Gradient - gradient fills for primitives

=head1 DESCRIPTION

Prima offers simple gradient services to draw gradually changing colors.  A
gradient is made by setting at least two colors and optionally a set of points
that, when projected, generate the transition curve between the colors.

The module augments the C<Prima::Drawable> drawing functionality by
adding the C<new_gradient> function.

=head1 SYNOPSIS

	$canvas-> new_gradient(
		palette => [ cl::White, cl::Blue, cl::White ],
	)-> sector(50,50,100,100,0,360);

=for podview <img src="Prima/gradient.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/gradient.gif">

=head1 API

=head2 Methods

=over

=item clone %OPTIONS

Creates a new gradient object with %OPTIONS replaced.

=item new $CANVAS, %OPTIONS

Creates a new gradient object. The following %OPTIONS are available:

=over

=item dither BOOLEAN = 0

When set, applies not only gradient colors but also different fill patterns
to create an even smoother transition effect between adjacent colors.
Works significantly slower.

=item palette @COLORS

Each color is a C<cl::> value. The gradient is calculated as a polyline where
each of its vertices corresponds to a certain blend between two adjacent colors
in the palette. F.ex. the simplest palette going from C<cl::White> to
C<cl::Black> over a transition line C<0..1> (default), produces a pure white color at
the start and a pure black color at the end, with all available shades of gray
in between.

=item poly @VERTICES

A set of 2-integer polyline vertices where the first integer is a coordinate (x,
y, or whatever is required by the drawing primitive) between 0 and 1, and the
second is the color blend value between 0 and 1.

Default: ((0,0),(1,1))

=item spline \@VERTICES, %OPTIONS

Serving the same purpose as the C<poly> option but the vertices are projected first
to a B-spline curve using L<render_spline> and C<%OPTIONS>. The resulting
polyline is treated as C<poly>.

=item vertical BOOLEAN

Only used in the L<bar> primitive, to set the gradient direction.

=item widgetClass INTEGER

Points to the widget class to resolve generic colors like C<cl::Back>
that may differ between widget classes.

=back

See also: L<bar>, L<stripes> .

=item bar X1, Y1, X2, Y2, VERTICAL = 0

Draws a filled rectangle with (X1,Y1) - (X2,Y2) extents

Context used: fillPattern, rop, rop2

=item colors BREADTH

Returns a list of gradient colors for each step from 1 to BREADTH.  When
C<dither> is set, each color is an array of three items, - the two adjacent
colors and an integer value between 0 and 63 that reflects the amount of
blending needed between the colors.

=item ellipse X, Y, DIAM_X, DIAM_Y

Draws a filled ellipse with the center in (X,Y) and diameters (DIAM_X,DIAM_Y)

Context used: fillPattern, rop, rop2

=item sector X, Y, DIAM_X, DIAM_Y, START_ANGLE, END_ANGLE

Draws a filled sector with the center in (X,Y) and diameters (DIAM_X,DIAM_Y) from START_ANGLE to END_ANGLE

Context used: fillPattern, rop, rop2

=item stripes BREADTH

Returns an array consisting of integer pairs, where the first one is a color
value, and the second is the breadth of the color strip.  L<bar> uses this
information to draw a gradient fill, where each color strip is drawn with its
own color. Can be used for implementing other gradient-aware primitives (see
F<examples/f_fill.pl> )

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable>, F<examples/f_fill.pl>, F<examples/gradient.pl>

=cut
