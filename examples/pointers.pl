=pod

=head1 NAME

examples/pointers.pl - mouse pointer example

=head1 FEATURES

Demonstrates the usage of the mouse pointer functionality.
Note the custom pointer creation and its dynamic change ( the "User" button ).

=cut

use strict;
use warnings;
use Prima qw( StdBitmap Buttons Application);

package UserInit;

my $ph = Prima::Application-> get_system_value(sv::YPointer);
my $sc = $::application-> uiScaling;
$ph = $::application->font->height * 2 if $ph < $::application->font->height * 2;
my $w = Prima::MainWindow-> new(
	size    => [ 350 * $sc, 20 + ($ph+8)*15],
	left    => 200,
	text    => 'Pointers',
);

my %grep_out = (
	'BEGIN' => 1,
	'END' => 1,
	'AUTOLOAD' => 1,
	'constant' => 1
);

my @a = sort { &{$cr::{$a}}() <=> &{$cr::{$b}}()} grep { !exists $grep_out{$_}} keys %cr::;
shift @a;
$::application-> pointerVisible(0);
my $i = 1;

for my $c ( @a[0..$#a-1])
{
	my $p = eval("cr::$c");
	$w-> pointer( $p);
	my $b = $w-> insert( Button =>
		flat => 1,
		left    => (10 + (($i-1) % 2)*170) * $sc,
		width   => 160 * $sc,
		height  => $ph + 4,
		bottom  => $w-> height - int(($i+1)/2) * ($ph+8) - 10,
		pointer => $p,
		name    => $c,
		image   => $w-> pointerIcon,
		onClick => sub { $::application-> pointer( $_[0]-> pointer); },
	);
	$i++;
};

my $ptr = Prima::StdBitmap::icon( sbmp::DriveCDROM, argb => 0, copy => 1);
my $color_pointer = $::application->get_system_value(sv::ColorPointer);

my @mapset = map {
	my ($x,$r) = $ptr-> split;
	my $j = Prima::Icon-> new;
	$x-> begin_paint;
	$x-> text_out( $_, 3, 3);
	$x-> end_paint;
	$r-> begin_paint;
	$r-> text_out( $_, 3, 3);
	$r-> end_paint;
	if ( $color_pointer ) { # add alpha
		$r-> type(im::Byte);
		$r-> rop2(rop::CopyPut);
		$r->color(0xc0c0c0);
		$r->backColor(0);
		$r->map(0);
	}
	$j-> combine( $x, $r);
	$j;
} 1..4;
my $mapsetID = 0;

my $b = $w-> insert( SpeedButton =>
	left    => $sc * ( 10 + (($i-1) % 2)*170 ),
	width   => 160 * $sc,
	height  => $ph+4,
	bottom  => $w-> height - int(($i+1)/2) * ($ph+8) - 10,
	pointer => $mapset[-1],
	image   => $mapset[-1],
	text => $a[-1],
	flat => 1,
	onClick => sub {
		$::application-> pointer( $_[0]-> pointer);
	},
);

$b-> insert( Timer =>
	timeout => 1250,
	onTick  => sub {
		$b-> pointerIcon( $mapset[$mapsetID]);
		$b-> image( $mapset[$mapsetID]);
		$mapsetID++;
		$mapsetID = 0 if $mapsetID >= @mapset;
	},
)-> start;

$w-> pointer( cr::Default);
$::application-> pointerVisible(1);

run Prima;
