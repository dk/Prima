use strict;
use warnings;
use subs qw(reset);
use Prima qw(Application ColorDialog Label Buttons);

my $w = Prima::MainWindow->new(
	size => [ 400, 300 ],
	text => 'Gradient',
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
		my $spline = $canvas-> render_spline( [ 0, 0, @points, $self->size ]);
		$spline = [ 0, 0, $self->size ] unless @points;
		my $v      = $vertical->checked;
		my $points = polyline2points($spline, $v, $self-> size);
		my $gradient = $self-> calculate_gradient( $v ? $self->width : $self->height, $c1->value, $c2->value, sub { $points->[shift] });
		$canvas->gradient_bar( 0,0,$self->size, $v, $gradient);

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

		$canvas->color(cl::Black);
		$canvas->lineWidth(3);
		$canvas->line(0,0,$self->size);
		$canvas->color(cl::White);
		$canvas->linePattern(lp::Dot);
		$canvas->lineWidth(1);
		$canvas->line(0,0,$self->size);
		
		$canvas->color(cl::Black);
		$canvas->lineWidth(3);
		$canvas-> polyline( $spline);
		$canvas->color(cl::White);
		$canvas->lineWidth(1);
		$canvas->linePattern(lp::Solid);
		$canvas-> polyline( $spline);
	},
	onSize   => sub {
		my ( $self, $ox, $oy, $x, $y) = @_;
		my $i;
		for ( $i = 0; $i < @points; $i+=2) { 
			$points[$i] = $x   if $points[$i] > $x;
			$points[$i+1] = $y if $points[$i+1] > $y; 
		}
	},
	onMouseDown => sub {
		my ( $self, $mod, $btn, $x, $y) = @_;
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
				last;
			}
		}
	},
	onMouseClick => sub {
		my ( $self, $mod, $btn, $x, $y, $dbl) = @_;
		return unless $dbl;
		push @points, $x, $y;
		$self->repaint;
	},
	onMouseUp => sub {
		undef $capture;
		shift->repaint;
	},
	onMouseMove => sub {
		my ( $self, $btn, $x, $y) = @_;
		if (defined $capture) {
			my @bounds = $self->size;
			$points[$capture] = $x if $x >= 0 && $x < $bounds[0];
			$points[$capture+1] = $y if $y >= 0 && $y < $bounds[1];
			$self->repaint;
		} else {
			my $i;
			my $p;
			for ( $i = 0; $i < @points; $i+=2) {
				if ( $points[$i] > $x - $aperture && $points[$i] < $x + $aperture && 
					$points[$i+1] > $y - $aperture && $points[$i+1] < $y + $aperture) {
					$p = $i;
					last;
				}
			}
			if (( $p // -1 ) != ($prelight // -1)) {
				$prelight = $p;
				$self->repaint;
			}
		}
	},
);


sub repaint { $gradient->repaint }
sub reset
{
	@points = map { $w->width * $_, $w->height * $_ } (0.33, 0.66);
	$c1->value(cl::Black);
	$c2->value(cl::White);
	repaint;
}

reset();

sub polyline2points
{
	my ($p, $vertical, $width, $height) = @_;
	my @map;
	($width, $height) = ($height, $width) unless $vertical;
	for ( my $i = 0; $i < @$p - 2; $i+=2) {
		my ($x1,$y1,$x2,$y2) = @$p[$i..$i+3];
		($x1, $y1, $x2, $y2) = ($y1, $x1, $y2, $x2) unless $vertical;
		my $dx = $x2 - $x1;
		if ( $dx > 0 ) {
			my $dy = ($y2 - $y1) / $dx;
			my $y = $y1;
			for ( my $x = $x1; $x < $x2; $x++) {
				$map[$x] = $y;
				$y += $dy;
			}
		} else {
			$map[$x1] = $y1;
		}
	}
	$_ = $_ / $height * $width for @map;
	return \@map;
}

run Prima;
