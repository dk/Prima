=pod 

=head1 NAME

examples/dwm.pl - win32 DWM blur demo

=cut

use strict;
use warnings;
use Prima qw(Application);

die "Your system doesn't support DWM blur - sorry. You'll need Window 7 or above\n"
	if $::application->get_system_value(sv::DWM) <= 0;

my $angle = 0;

use constant PI => 4.0 * atan2 1, 1;
use constant D2R => PI / 180;
use constant Cos_120 => cos(D2R*(-120));
use constant Sin_120 => sin(D2R*(-120));

sub dwm_reset
{
	my $win  = shift;
	my @sz = $win->size;
	my $i = Prima::Image->create( size => \@sz, type => im::BW );

	$i-> begin_paint;
	$i-> clear;

	my ($w, $h) = @sz;
	my $sz = ($w > $h ? $h : $w) * 0.8 * 0.5;
	my ($x, $y) = (25*$sz/34,$sz);
	my ($fx, $fy) = ($x,$y);
	($w,$h) = ($w/2,$h/2);
	my $cos = cos(D2R*$angle);
	my $sin = sin(D2R*$angle);
	my @lines = ();
	($x,$y) = ($fx, $fy);
	push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
	$x = Cos_120*$fx - Sin_120*$fy;
	$y = Sin_120*$fx + Cos_120*$fy;
	push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
	($x, $y) = ($x+sqrt(3)*$y,0);
	push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
	$i->fillpoly(\@lines);
		
	$i->end_paint;

	$win->effects({ dwm_blur => {
		enable  => 1,
		mask    => $i,
	}});
}

my $w = new Prima::MainWindow(
	text => "Hello, world!",
	backColor => cl::Black,
	onSize => sub { dwm_reset(shift) },
	onPaint => sub {
		my $w = shift;
		my $W = $w->width / 4;
		my $H = $w->height / 4;
		$w->clear;
		$w->color(cl::Yellow);
		$w->bar( $W, $H, $w->width - $W, $w->height - $H);
	},
);

$w-> insert( Timer =>
	timeout => 50,
	onTick => sub { 
		$angle += 10;
		$angle -= 360 if $angle >= 360;
		dwm_reset($w);
	},   
) -> start;

run Prima;
