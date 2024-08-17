=head1 NAME

examples/3d.pl - raycasting demo

=head1 SEE ALSO

This example is a simplified port of
L<https://www.playfuljs.com/a-first-person-engine-in-265-lines> 
by Hunter Loftis

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
my $wall           = create_wall();
my @wall_size      = $wall->size;
my $size           = 32;
my @grid           = map {(0.3 > rand) ? 1 : 0} 1..$size*$size;
my $fov            = $pi * 0.4;
my $resolution     = 320;
my $range          = 14;
my $seconds        = 1;
my $draw_rain      = 1;
my $light          = 0;
my $can_alpha      = $::application->can_draw_alpha;
$wall->data( ~$wall->data) unless $can_alpha;
$wall = $wall->bitmap if $can_alpha;
my ( $width, $height, $spacing, $scale, $cast_cache ) ;

sub create_wall
{
	my $wall = Prima::Image->new(
		type => im::Byte,
		size => [128,128],
	);
	$wall->clear;
	$wall->antialias(1);
	$wall->lineWidth(0.8);
	$wall->matrix->scale(0.4,-0.4)->translate(-23,290);
	my (@o, $path);
	while (<DATA>) {
		chomp;
		my @p = split / /, $_;
		my $c = pop @p;
		if ( $c eq 'C') {
			$path->spline([@o, @p], degree => 3);
			@o = @p[-2,-1];
		} elsif ( $c eq 'M') {
			$path = $wall->new_path;
			$path->moveto(@o = @p);
		} elsif ( $c eq 'L') {
			$path->line(@p);
		} elsif ( $c eq 'F') {
			$path->fill;
			$path = $wall->new_path;
		} elsif ( $c eq 'S') {
			$wall->color(0x404040);
			$path->stroke;
			$path = $wall->new_path;
		} elsif ( $c eq 'B') {
			$wall->color(0);
		} elsif ( $c eq 'W') {
			$wall->color(0xffffff);
		}
	}
	$wall->type(8);
	$wall->colormap( map { $_ & $shade } $wall->colormap);
	return $wall;
}

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
		$nextstep->[OFFSET]   = 1 - $nextstep->[OFFSET] if $shiftx ? ($x < $ox) : ($y > $oy);

		last if $nextstep->[DISTANCE] > $range;
		push @rays, @$nextstep;
	};

	return \@rays;
}

sub draw_sky
{
	my ($canvas) = @_;

	if ( $light > 0 ) {
		my $l = $light - 10 * $seconds;
		$light = ($l < 0) ? 0 : $l;
	} elsif ( rand() * 5 < $seconds ) {
		$light = 2;
	}

	$canvas->new_gradient(
		palette => [map { cl::blend($_,0xffffff,$light) } 0,$shade,0],
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

	my $got_rain;
	for ( my $s = @$rays - RAYSIZE; $s >= 0; $s -= RAYSIZE) {
		my $step         = $s / RAYSIZE;
		my $cos_distance = $cos_angle * $rays->[$s + DISTANCE] || 1;
		my $bottom       = int( $height / 2 * (1 - 1 / $cos_distance) + .5);
		if ( $step == $hit && $cos_distance) {
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

		unless ($got_rain) {
			my $rain_drops   = $step * (rand() ** 3);
			my $rain_height  = ($rain_drops > 0) ? 0.2 * $height / $cos_distance : 0;
			while ( $draw_rain && --$rain_drops > 0 ) {
				my $top = rand() * $height;
				push @$rain, $left, $top, $left, $top+$rain_height;
				$got_rain = 1;
			}
		}
	}
}

sub draw_columns
{
	my $canvas = shift;
	$canvas->color(cl::Black);
	my $rain = Prima::array->new_int;
	for ( my $column = 0; $column < $resolution; $column++) {
		my $angle = $fov * ( $column / $resolution - 0.5 );
		my $ray   = $cast_cache->[$column] //= cast($x, $y, $direction + $angle, $range, $size);
		draw_column($column, $ray, cos($angle), $canvas, $rain);
	}
	$canvas->color($shade | 0x404040);
	$canvas->alpha(42);
	$canvas->bars( $rain );
	$canvas->alpha(255);
}

sub draw_weapon
{
	my ( $canvas ) = @_;
	my $bobx = cos($paces * 2) * $scale * 6;
	my $boby = sin($paces * 4) * $scale * 6;
	my $left = $width * 0.5 + $bobx;
	my $top  = $boby - 50 * $scale;
	$canvas->graphic_context( sub {
		$canvas->antialias(1);
		$canvas->matrix->scale( $scale )->translate( $left, $top );
		$canvas->color((0xcccccc & $shade) | 0x222222);
		$canvas->new_path->spline([90, 0, 40, 80, 0, 190])->spline([0, 200, 50, 90, 100, 0])->fill;
		$canvas->color((0x444444 & $shade) | 0x222222);
		$canvas->new_path->spline([100, 0, 50, 90, 0, 200])->spline([5, 190, 55, 90, 105, 0])->fill;
	});
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
	onMouseMove => sub {
		my ( $self, $mod, $x, $y ) = @_;
		return unless $self->get_mouse_state;
		if ($self->{alarm} && !$self->{simulation}) {
			$self->{alarm}->destroy;
			undef $self->{alarm};
		}
		my $w = $self->width / 2;
		my $h = $self->height / 2;
		rotate( $pi * ($x - $w) / $w * $seconds);
		walk( ($y - $h) / $h * 3 * $seconds);
		$self->{alarm} //= Prima::Utils::alarm(60, sub {
			delete $self->{alarm};
			local $self->{simulation} = 1;
			$self->mouse_move($mod, $x, $y);
		});
	},
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		draw_sky($canvas);
		draw_columns($canvas);
		draw_weapon($canvas);
		my $t = time;
		$canvas->color($shade);
		$seconds = $t - $last_time;
		$canvas->alpha(255);
		$canvas->text_out(sprintf("%.1d fps", 1/$seconds),0,0);
		$last_time = $t;
	},
);

$w->insert(Timer => onTick => sub { $w->repaint }, timeout => 5 )->start;

run Prima;

__DATA__;
B
301 574 M
301 574 300 571 297 568 296 565 C
296 565 287 565 290 568 283 572 C
283 572 284 569 287 558 283 558 C
283 558 279 558 278 560 275 564 C
275 564 273 566 270 569 267 571 C
267 571 269 562 271 553 261 563 C
261 563 253 570 252 575 246 576 C
246 576 246 572 245 567 243 564 C
227 575 L
243 564 225 577 225 578 221 579 C
221 579 222 573 223 576 223 566 C
223 566 223 563 212 552 208 562 C
208 562 207 564 208 563 206 566 C
206 566 204 570 197 575 195 580 C
195 580 198 579 198 579 200 577 C
205 574 L
200 577 208 571 208 569 212 566 C
212 566 222 571 214 575 214 586 C
214 586 225 586 231 574 238 574 C
238 574 243 594 256 575 260 570 C
260 570 260 576 258 577 258 584 C
258 584 266 583 269 570 277 568 C
277 568 277 575 275 578 274 585 C
274 585 282 584 282 575 290 573 C
290 573 296 576 293 577 299 580 C
299 580 303 579 306 577 308 575 C
308 575 308 575 304 583 308 583 C
308 583 313 583 315 580 318 578 C
318 578 320 576 323 573 325 572 C
325 572 326 579 330 584 339 575 C
339 575 341 573 343 572 343 569 C
343 569 336 570 340 574 332 574 C
332 574 331 570 331 569 331 564 C
331 564 324 564 322 573 314 575 C
314 575 320 565 332 559 333 554 C
333 554 327 550 317 564 306 571 C
306 571 303 573 305 573 301 574 C
F
B
170 540 M
173 540 L
170 540 175 545 176 540 174 552 C
174 552 174 554 174 555 173 558 C
171 565 L
173 558 173 582 169 576 167 582 C
167 582 169 582 171 582 170 587 C
170 587 170 590 170 589 169 590 C
169 590 170 592 171 594 171 597 C
171 597 174 596 173 596 175 595 C
175 595 179 587 176 592 176 586 C
176 586 176 582 177 579 181 581 C
181 581 183 577 187 578 195 573 C
195 573 207 566 203 552 197 544 C
193 540 L
197 544 192 539 193 540 191 539 C
191 539 185 534 187 537 185 534 C
185 534 176 534 175 530 170 540 C
F
W
178 540 M
178 540 186 544 194 551 194 561 C
194 561 194 570 185 572 178 576 C
178 576 175 561 188 552 181 544 C
181 544 178 540 180 542 178 540 C
178 540 L
F
B
239 549 M
239 549 239 555 249 553 249 541 C
249 541 244 541 239 546 239 549 C
F
177 635 M
177 635 166 612 154 551 96 558 C
96 558 5 570 241 499 356 499 C
356 499 239 493 157 500 123 509 C
123 509 106 517 148 522 177 635 C
S
