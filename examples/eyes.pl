=pod

=head1 NAME

examples/eyes.pl - an port of the I<xeyes> program

=head1 FEATURES

A well-known xeyes program re-written in Prima toolkit.  Demostrates the usage
of non-rectangular widget shapes and how to check if the system supports it.

Note the 'hide menu' feature - it's activation (^M) tests a correct
implementation of Prima shape-extension interface.

=cut

use strict;
use warnings;
use Prima;
use Prima::Application name => 'Eyes';


my $eye  = 0.45;
my $ball = 0.06;

my $revcolors = 0;
my $canshape  = $::application-> get_system_value( sv::ShapeExtension);

sub reshape
{
	my $x = $_[0];
	my @sz = $x-> size;
	my $nope = $sz[0] < 5 || $sz[1] < 5;
	for (0,1) {
	$sz[$_] = 5 if $sz[$_] < 5;
	}
	my $i = Prima::Image-> create(
		width  => $sz[0],
		height => $sz[1],
		type   => im::BW,
	);
	$i-> begin_paint;
	$i-> color( cl::White);
	$i-> backColor( cl::Black);
	$i-> clear;
	my $minSz = ( $sz[0] < $sz[1]) ? $sz[0] : $sz[1];
	my @eye = ( $sz[0] * $eye, $sz[1] * $eye * 2);
	$i-> lineWidth(( $minSz < 220) ? $minSz / 20 : 11);
	$i-> ellipse( $sz[0] * 0.25, $sz[1]/2, @eye);
	$i-> fill_ellipse( $sz[0]*0.25, $sz[1]/2, @eye);
	$i-> ellipse( $sz[0]*0.75, $sz[1]/2, @eye);
	$i-> fill_ellipse( $sz[0]*0.75, $sz[1]/2, @eye);
	$i-> end_paint;
	$x-> shape( $i) unless $nope;
	return $i;
}

my $m;

my $x = Prima::MainWindow-> create(
	visible  => 0,
	buffered  => 1,
	color     => cl::Black,
	backColor => cl::White,
	menuItems => [
		['~Options' => [
			["~Reverse colors" => sub {
				my ( $self, $mit) = @_;
				$revcolors = $revcolors ? 0 : 1;
				$self-> menu-> text( $mit,
					$revcolors ? "~Normal colors" : "~Reverse colors");
				$self-> color( $revcolors ? cl::White : cl::Black);
				$self-> backColor( $revcolors ? cl::Black : cl::White);
			}],
			['~Remove menu' => 'Ctrl+M' => '^M' => sub {
				if ( $_[0]-> menu) {
					$m = $_[0]-> menu;
					$_[0]-> menu-> selected(0);
				} else {
					$m-> selected(1);
				}
			}],
			[],
			["E~xit" => 'Alt+X' => '@X' => sub { $::application-> close }],
		]],
	],
	size     => [ 200, 300],
	name     => 'Eyes',
	onSize   => sub {
		reshape( $_[0]) if $canshape;
	},
	onPaint  => sub {
		my ( $self, $canvas) = @_;
		my @sz = $self-> size;
		$canvas-> clear;
		my $minSz = ( $sz[0] < $sz[1]) ? $sz[0] : $sz[1];
		$canvas-> lineWidth(( $minSz < 220) ? $minSz / 20 : 11);
		my @cc = ( $sz[0]* 0.25, $sz[1]/2);
		my @eye = ( $sz[0] * $eye, $sz[1] * $eye * 2);
		my @pp = $self-> pointerPos;
		for ( 0..1) {
			$canvas-> translate( @cc);
			$canvas-> ellipse( 0, 0, @eye);
			my @dd = ( $pp[0] - $cc[0], $pp[1] - $cc[1]);
			my $angle = atan2( $dd[1], $dd[0]);
			my ( $sin, $cos) = ( sin($angle), cos( $angle));
			my $h = sqrt(
				($eye[1]*$cos) * ($eye[1]*$cos) +
				($eye[0]*$sin) * ($eye[0]*$sin)
			);
			my @da = ( $eye[0] * $eye[1] * $cos / $h, $eye[0] * $eye[1] * $sin / $h);
			my $dp = sqrt( $dd[0] * $dd[0] + $dd[1] * $dd[1]);
			my $db = sqrt( $da[0] * $da[0] + $da[1] * $da[1]) * 0.36;
			my @e = ( $db < $dp) ? ( $db * $cos, $db * $sin) : @dd;
			$canvas-> fill_ellipse( @e, $sz[0]* $ball, $sz[1]* $ball * 2);
			$cc[0] += $sz[0] / 2;
		}
	},
);

$x-> icon( reshape( $x));

my @pp = $x-> pointerPos;

$x-> insert( Timer =>
	timeout => 100,
	onTick => sub {
		my @pxp = $x-> pointerPos;
		return if $pxp[0] == $pp[0] && $pxp[1] == $pp[1];
		$x-> repaint;
		@pp = @pxp;
	})-> start;
$x-> show;
$x-> select;

run Prima;
