=pod

=head1 NAME

examples/gradient.pl - gradients demo

=head1 FEATURES

Demonstrates use of C<Prima::Drawable::new_gradient>

=cut

use strict;
use warnings;
use subs qw(reset);
use Prima qw(Application Dialog::ColorDialog Label Buttons Sliders);

my $w = Prima::MainWindow->new(
	size => [ 400, 300 ],
	text => 'Gradient',
	designScale => [ 7, 16 ],
);

my $panel = $w-> insert( Widget =>
	maxSize=> [undef, 150],
	pack   => { fill => 'x' },
);

my $vertical = $panel-> insert( CheckBox =>
	pack   => { side => 'left', padx => 20 },
	text   => '~Vertical',
	onClick => \&repaint,
);

my $splined = $panel-> insert( CheckBox =>
	pack   => { side => 'left', padx => 20 },
	text   => '~Splines',
	checked => 1,
	onClick => \&repaint,
);

my $dithered = $panel-> insert( CheckBox =>
	pack   => { side => 'left', padx => 20 },
	text   => '~Dither',
	checked => 0,
	onClick => \&repaint,
);

$panel-> insert( Button =>
	text   => '~Reset',
	pack   => { side => 'left', padx => 20 },
	onClick => \&reset,
);

my $c1 = $panel-> insert( ColorComboBox =>
	name   => 'C1',
	pack   => { side => 'right',  },
	value  => cl::White,
	onChange => \&repaint,
);

$panel-> insert( Label =>
	text  => '~To',
	focusLink => 'C1',
	pack   => { side => 'right', },
);

sub breadth ();

my @colors;
my @offsets;
my $add_color;
$add_color = $panel-> insert( AltSpinButton =>
	pack   => { side => 'right', padx => 20 },
	onIncrement => sub {
		my ( $self, $increment ) = @_;
		if ( $increment > 0 ) {
			return if @colors > 10;
			splice(@colors, -1, 0, $panel-> insert( ColorComboBox =>
				pack   => { side => 'right', padx => 20, after => $add_color },
				value  => cl::Black,
				onChange => \&repaint,
			));
			push @offsets, ( @offsets ? ((breadth - $offsets[-1]) / 2) : (breadth * 0.3) );
		} elsif ( @colors > 2 ) {
			my ($c) = splice(@colors, -2, 1);
			$c->destroy if $c;
			pop @offsets if $c;
		}
		repaint();
	}
);

my $c2 = $panel-> insert( ColorComboBox =>
	pack   => { side => 'right', padx => 20 },
	value  => cl::Black,
	name   => 'C2',
	onChange => \&repaint,
);

$panel-> insert( Label =>
	text  => '~From',
	focusLink => 'C2',
	pack   => { side => 'right', },
);

my $aperture = 12;
my $capture;
my $prelight;
my @points;
my $gradient = $w->insert( Widget =>
	name   => 'Gradient',
	pack   => { fill => 'both', expand => 1 },
	buffered => 1,
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		my ( $w, $h ) = $self-> size;
		my $v = $vertical->checked;
		my $b = breadth;
		my @xpoints = @points;
		for ( my $i = 0; $i < @xpoints; $i+=2) {
			$xpoints[$i]   /= $w;
			$xpoints[$i+1] /= $h;
		}
		$canvas->new_gradient(
			vertical => $v,
			dither   => $dithered-> checked,
			palette  => [map { $_-> value } @colors ],
			offsets  => [ map { $_ / $b } @offsets, $b ],
			( $splined->checked ? 'spline' : 'poly') => \@xpoints,
		)->bar( 0, 0, $w, $h);

		my $i;
		$canvas->lineWidth(2);
		my $c = $prelight // $capture // -1;
		for ( $i = 0; $i < @points; $i+=2) {
			$canvas-> color(($i == $c) ? cl::White : cl::Black);
			$canvas-> fill_ellipse(
				$points[$i], $points[$i+1],
				$aperture, $aperture
			);
			$canvas-> color(($i == $c) ? cl::Black : cl::White);
			$canvas-> fill_ellipse(
				$points[$i], $points[$i+1],
				$aperture/2, $aperture/2
			);
		}

		my $triangle = $v ?
			[ $aperture, 0, 0, -$aperture, -$aperture, 0, $aperture, 0 ] :
			[ 0, $aperture, $aperture, 0, 0, -$aperture, 0, $aperture  ];
		$c = -1 * ( $prelight // $capture // 1 ) - 1;
		for ( $i = 0; $i < @offsets; $i++) {
			my $offset = $offsets[$i];
			$canvas->translate($v ? ( $offset, $self-> height ) : ( 0, $offset ));

			$canvas->color(($i == $c) ? cl::White : cl::Black);
			$canvas->lineWidth(3);
			$canvas->polyline($triangle);
			$canvas->lineWidth(1);

			$canvas->color(($i == $c) ? cl::Black : cl::White);
			$canvas->fillpoly($triangle);
		}
		$canvas->translate(0,0);

		$canvas->color(cl::Black);
		$canvas->lineWidth(3);
		$canvas->line(0,0,$w,$h);
		$canvas->color(cl::White);
		$canvas->linePattern(lp::Dot);
		$canvas->lineWidth(1);
		$canvas->line(0,0,$w,$h);

		$canvas->color(cl::Black);
		$canvas->lineWidth(3);
		my $method = $splined->checked ? 'spline' : 'polyline';
		$method = 'polyline' unless @points;
		@xpoints = ( 0, 0, @points, $w, $h);
		$canvas-> $method( \@xpoints);
		$canvas->color(cl::White);
		$canvas->lineWidth(1);
		$canvas->linePattern(lp::Solid);
		$canvas-> $method( \@xpoints);
	},
	onSize   => sub {
		my ( $self, $ox, $oy, $x, $y) = @_;
		my $i;
		for ( $i = 0; $i < @points; $i+=2) {
			$points[$i] = $x   if $points[$i] > $x;
			$points[$i+1] = $y if $points[$i+1] > $y;
		}
		my $b = $vertical->checked ? $x : $y;
		for ( $i = 0; $i < @offsets; $i++) {
			$offsets[$i] = $b  if $offsets[$i] > $b;
		}
	},
	onMouseDown => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		my $i;
		$capture = undef;
		for ( $i = 0; $i < @points; $i+=2) {
			if ( $points[$i] > $x - $aperture && $points[$i] < $x + $aperture &&
				$points[$i+1] > $y - $aperture && $points[$i+1] < $y + $aperture) {
				if ( $btn == mb::Left ) {
					$capture = $i;
				} elsif ( $btn == mb::Right ) {
					splice(@points, $i, 2);
				}
				$self->repaint;
				return;
			}
		}

		for ( $i = 0; $i < @offsets; $i++) {
			my ($ax, $ay) = $vertical->checked ? ( $offsets[$i], $self->height - $aperture/2 ) : ( $aperture/2, $offsets[$i]);
			if ( $ax > $x - $aperture/2 && $ax < $x + $aperture/2 &&
				$ay > $y - $aperture/2 && $ay < $y + $aperture/2) {
				if ( $btn == mb::Left ) {
					$capture = -$i - 1;
				} elsif ( $btn == mb::Right && @colors > 2) {
					$_-> destroy for splice(@colors, 1 + $i, 1);
					splice(@offsets, $i, 1);
				}
				$self->repaint;
				return;
			}
		}
	},
	onMouseClick => sub {
		my ( $self, $mod, $btn, $x, $y, $dbl) = @_;
		return unless $dbl;
		return if @points > 10;
		push @points, $x, $y;
		$self->repaint;
	},
	onMouseUp => sub {
		undef $capture;
		shift->repaint;
	},
	onMouseMove => sub {
		my ( $self, $btn, $x, $y) = @_;
		if (defined $capture && $capture >= 0 ) {
			my @bounds = $self->size;
			$points[$capture] = $x if $x >= 0 && $x < $bounds[0];
			$points[$capture+1] = $y if $y >= 0 && $y < $bounds[1];
			$self->repaint;
		} elsif (defined $capture && $capture < 0 ) {
			my @bounds = $self->size;
			my $i = -$capture - 1;
			if ( $vertical->checked ) {
				$offsets[$i] = $x if $x >= 0 && $x < $bounds[0];
			} else {
				$offsets[$i] = $y if $y >= 0 && $y < $bounds[1];
			}
			$self->repaint;
		} else {
			my $i;
			my $p;
			for ( $i = 0; $i < @points; $i+=2) {
				if ( $points[$i] > $x - $aperture && $points[$i] < $x + $aperture &&
					$points[$i+1] > $y - $aperture && $points[$i+1] < $y + $aperture) {
					$p = $i;
					goto FOUND;
				}
			}
			for ( $i = 0; $i < @offsets; $i++) {
				my ($ax, $ay) = $vertical->checked ? ( $offsets[$i], $self->height - $aperture/2 ) : ( $aperture/2, $offsets[$i]);
				if ( $ax > $x - $aperture/2 && $ax < $x + $aperture/2 &&
					$ay > $y - $aperture/2 && $ay < $y + $aperture/2) {
					$p = -$i - 1;
					last;
				}
			}
		FOUND:
			if (( $p // -20 ) != ($prelight // -20)) {
				$prelight = $p;
				$self->repaint;
			}
		}
	},
	onMouseLeave => sub {
		if ( $prelight ) {
			undef $prelight;
			repaint();
		}
	},
);


sub repaint { $gradient->repaint }
sub reset
{
	@points = map { $w->width * $_, $w->height * $_ } (0.33, 0.66);
	$c1->value(cl::Black);
	$c2->value(cl::White);
	if (@colors > 2) {
		$_-> destroy for @colors[1..$#colors-1];
	}
	@colors  = ( $c1, $c2 );
	@offsets = ( );
	repaint;
}

reset();

sub breadth()
{
	$vertical->checked ? $gradient->width : $gradient->height
}

run Prima;
