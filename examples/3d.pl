=head1 NAME

examples/3d.pl - raycasting demo

=head1 SEE ALSO

This example is a simplified port of L<https://www.playfuljs.com/a-first-person-engine-in-265-lines> by Hunter Loftis

=cut

use strict;
use warnings;
use POSIX qw(ceil floor);
use Time::HiRes qw(time);
use Prima qw(Application StdBitmap);

my $shade          = 0xff0000;
my $pi             = atan2(1,0)*2;
my $twopi          = $pi*2;
my ( $x, $y, $direction, $paces ) = (15.3,-1.2,$pi/4,0);
my $wall           = Prima::StdBitmap::image(0);
$wall->type(im::Byte);
$wall->type(8);
$wall->colormap( map { $_ & $shade } $wall->colormap);
my @wall_size      = $wall->size;
my $size           = 32;
my @grid           = map {(0.3 > rand) ? 1 : 0} 1..$size*$size;
my $fov            = $pi * 0.4;
my $resolution     = 320;
my $range          = 14;
my $cast_cache;
my $seconds        = 1;
my $draw_rain      = 1;
my $can_alpha      = $::application->can_draw_alpha;
$wall->data( ~$wall->data) unless $can_alpha;

my ( $width, $height, $spacing, $scale ) ;

sub rotate
{
	$direction = ($direction + $twopi + shift);
	$direction -= $twopi while $direction > $twopi;
	undef $cast_cache;
}

sub walk
{
	my $distance = shift;
	my $dx = cos($direction) * $distance;
	my $dy = sin($direction) * $distance;
	$x += $dx if inside($x + $dx, $y) <= 0;
	$y += $dy if inside($x, $y + $dy) <= 0;
	$paces += $distance;
	undef $cast_cache;
}

sub inside
{
	my ( $x, $y ) = map { floor($_) } @_;
	return ($x < 0 or $x > $size - 1 or $y < 0 or $y > $size - 1) ? -1 : $grid[ $y * $size + $x ];
}

use constant {
	X        => 0,
	Y        => 1,
	HEIGHT	 => 2,
	DISTANCE => 3,
	SHADING  => 4,
	OFFSET	 => 5,
	LENGTH2  => 6,
	RAYSIZE  => 7,
};

sub _rayentry
{
	my %r = @_;
	my @r = (0) x RAYSIZE;
	while ( my ($k,$v) = each %r) {
		$r[$k] = $v;
	}
	return @r;
}

sub cast
{
	my ($ox, $oy, $angle, $range) = @_;
	my $sin    = sin($angle);
	my $cos    = cos($angle);
	my $sincos = $sin / $cos;
	my $cossin = $cos / $sin;

	my @rays = _rayentry(X,$ox,Y,$oy,HEIGHT,-1);

	while (1) {
		my $r = @rays - RAYSIZE;

		my ( @stepx, @stepy );
		if ( $cos != 0 ) {
			my ( $x, $y ) = ($rays[$r + X], $rays[$r + Y]);
			my $dx = ($cos > 0) ? int($x + 1) - $x : ceil($x - 1) - $x;
			my $dy = $dx * $sincos;
			@stepx = _rayentry( X, $x + $dx, Y, $y + $dy, LENGTH2, $dx*$dx + $dy*$dy );
		} else {
			@stepx = _rayentry( LENGTH2, 0 + 'Inf' );
		}

		if ( $sin != 0 ) {
			my ( $x, $y ) = ($rays[$r + Y], $rays[$r + X]);
			my $dx = ($sin > 0) ? int($x + 1) - $x : ceil($x - 1) - $x;
			my $dy = $dx * $cossin;
			@stepy = _rayentry( Y, $x + $dx, X, $y + $dy, LENGTH2, $dx*$dx + $dy*$dy );
		} else {
			@stepy = _rayentry( LENGTH2, 0 + 'Inf' );
		}

		my ( $nextstep, $shiftx, $shifty, $distance, $offset ) = 
			($stepx[LENGTH2] < $stepy[LENGTH2]) ?
				(\@stepx, 1, 0, $rays[$r + DISTANCE], $stepx[Y]) :
				(\@stepy, 0, 1, $rays[$r + DISTANCE], $stepy[X]);

		my ( $x, $y ) = map { floor($_) } (
			$nextstep->[X] - (( $cos < 0 ) ? $shiftx : 0), 
			$nextstep->[Y] - (( $sin < 0 ) ? $shifty : 0)
		);
		$nextstep->[HEIGHT]   = ($x < 0 or $x > $size - 1 or $y < 0 or $y > $size - 1) ? -1 : $y * $size + $x; 
		$nextstep->[DISTANCE] = $distance + sqrt($nextstep->[LENGTH2]);
		$nextstep->[SHADING]  = $shiftx ? ( $cos < 0 ? 2 : 0 ) : ( $sin < 0 ? 2 : 1 );
		$nextstep->[OFFSET]   = $offset - int($offset);
		$nextstep->[OFFSET]   = 1 - $nextstep->[OFFSET] if 
			($stepx[LENGTH2] < $stepy[LENGTH2]) ? ($x < $ox) : ($y > $oy);

		last if $nextstep->[DISTANCE] > $range;
		push @rays, @$nextstep;
	};

	return \@rays;
}

sub draw_sky
{
	my ($canvas) = @_;
	$canvas->new_gradient(
		palette => [0,$shade,0],
		poly    => [0,0,0.5,0.3,1,1],
	)->bar(0,0,$canvas->size);
}

sub draw_column
{
	my ($column, $rays, $cos_angle, $canvas, $rain ) = @_;
	my $left  = int( $column * $spacing );
	my $width = ceil( $spacing );
	my $hit   = HEIGHT;
	$hit     += RAYSIZE while $hit < @$rays && ( $rays->[$hit] < 0 || $grid[$rays->[$hit]] == 0 );
	$hit      = ($hit - HEIGHT)/RAYSIZE;

	for ( my $s = @$rays - RAYSIZE; $s >= 0; $s -= RAYSIZE) {
		my $step         = $s / RAYSIZE;
		my $cos_distance = $cos_angle * $rays->[$s + DISTANCE] || 1;
		my $bottom       = int( $height / 2 * (1 - 1 / $cos_distance) + .5);
		next unless $step == $hit && $cos_distance;

		my $texturex = int( $wall_size[0] * $rays->[$s + OFFSET]);
		my $wproj_height = int( $height * $grid[$rays->[$s + HEIGHT]] / $cos_distance + .5);
		$canvas->put_image_indirect( $wall,
			$left,     $bottom,
			$texturex, 0,
			$width,    $wproj_height,
			1,         $wall_size[1],
			rop::CopyPut);

		last unless $can_alpha;
		my $alpha = ($rays->[$s + DISTANCE] + $rays->[$s + SHADING]) / 5;
		$alpha = 0 if $alpha < 0;
		$canvas->alpha($alpha * 255);
		$canvas->bar( $left, $bottom, $left + $width - 1, $bottom + $wproj_height - 1);
		last;
	}

}

sub draw_columns
{
	my $canvas = shift;
	$canvas->color(cl::Black);
	for ( my $column = 0; $column < $resolution; $column++) {
		my $angle = $fov * ( $column / $resolution - 0.5 );
		my $ray   = $cast_cache->[$column] //= cast($x, $y, $direction + $angle, $range, $size);
		draw_column($column, $ray, cos($angle), $canvas);
	}
	$canvas->alpha(255);
}

my $last_time = time;
my $w = Prima::MainWindow->new(
	text => 'raycaster',
	buffered => 1,
	onSize	 => sub {
		my ( $self, $ox, $oy, $x, $y ) = @_;
		$width = $x;
		$height = $y;
		$spacing = $width / $resolution;
		$scale	= ( $width + $height ) / 1200;
	},
	onKeyDown => sub {
		my ( $self, $code, $key, $mod ) = @_;
		if ( $key == kb::Left  ) { rotate(-$pi*$seconds);    }
		elsif ( $key == kb::Right ) { rotate($pi*$seconds);  }
		elsif ( $key == kb::Up	  ) { walk(3*$seconds);	     }
		elsif ( $key == kb::Down  ) { walk(-3*$seconds);     }
	},
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		draw_sky($canvas);
		draw_columns($canvas);
		my $t = time;
		$canvas->color($shade);
		$seconds = $t - $last_time;
		$canvas->alpha(255);
		$canvas->text_out(sprintf("%.1d fps", 1/$seconds),0,0);
		$last_time = $t;
	},
);


$w->insert(Timer => 
	onTick => sub { $w->repaint },
	timeout => 5,
)->start;

run Prima;
