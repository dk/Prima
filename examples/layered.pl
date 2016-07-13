use strict;
use warnings;

use Prima qw(Application);

my $j = Prima::Image->new(
	width => 100,
	height => 100,
	type => im::Byte,
);
$j->begin_paint;
$j->backColor(cl::Clear);
$j->clear;
$j->gradient_ellipse(50, 50, 100, 100, {palette => [cl::Black, cl::White]});
$j->end_paint;
$j->type(im::Byte);

sub icon
{
	my $color =  shift;
	my $i = Prima::Image->new(
		width => 100,
		height => 100,
		type => im::RGB,
	);
	$i->begin_paint;
	$i->backColor(cl::Clear);
	$i->clear;
	$i->gradient_ellipse(50, 50, 100, 100, {
		palette => [cl::Black, $color],
		spline  => [0.2,0.8],
	});
	$i->end_paint;

	my $k = Prima::Icon->new;
	$k->combine($i, $j);

	my $db = Prima::DeviceBitmap->new(
		width  => 160,
		height => 160,
		type   => dbt::Layered,
	);

	$db->backColor(cl::Clear);
	$db->clear;
	$db->stretch_image( 0, 0, 160, 160, $k);

	return $db;
}

my ( $r, $g, $b ) = map { icon($_) } (cl::LightRed, cl::LightGreen, cl::LightBlue);

my $angle = 0;
my $pi = 3.14159;

my $w = Prima::Widget->new(
	layered  => 1,
	buffered => 1,
	size     => [ 300, 300 ],
	backColor => cl::Black,
	selectable => 1,
	onMouseDown => sub { exit },
	onKeyDown => sub { exit },
	onPaint   => sub {
		my $self = shift;
		$self->clear;
		my ($w, $h) = $self->size;
		my $x = $w * 1 / 2;
		my $y = $h * 1 / 2;

		$self->translate($w/2, $h/2);
		my ( $x0, $y0 ) = ( $w/12, $h/12);
		my $r0 = ($x0 < $y0) ? $x0 : $y0;
		my $a = $angle;
		for my $i ( $r, $g, $b ) {
			my $sin = sin($a) * $r0;
			my $cos = cos($a) * $r0;
			my $xx = $cos - $sin; 
			my $yy = $sin + $cos; 
			$xx -= $x / 2;
			$yy -= $y / 2;
			#$self->stretch_image( $xx, $yy, $x, $y, $i);
			$self->put_image( $xx, $yy, $i);
			$a += $pi * 2 / 3;
		}
	}
);

my ($mx, $my) = (1, 1);

$w->insert( Timer => 
	timeout => 100,
	onTick => sub {
		$angle += 0.1;
		$angle -= $pi*2 if $angle > $pi*2;
		my @p = $w->origin;
		$p[0] += $mx * 5;
		$p[1] += $my * 5;
		$mx = 1 if $p[0] < 0 && $mx < 0;
		$my = 1 if $p[1] < 0 && $my < 0;
		$mx = -1 if $p[0] > $::application->width-$w->width && $mx > 0;
		$my = -1 if $p[1] > $::application->height-$w->height && $my > 0;
		$w->origin(@p);
		$w->repaint;
	},
)->start;


run Prima;

