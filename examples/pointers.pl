=pod

=head1 NAME

examples/pointers.pl - Prima mouse pointer example

=head1 FEATURES

Demonstrates the usage of Prima mouse pointer functionality.
Note the custom pointer creation and its dynamic change ( the "User" button ).

=cut

use strict;
use warnings;
use Prima qw( StdBitmap Buttons Application);

package UserInit;

my $ph = Prima::Application-> get_system_value(sv::YPointer);
my $sc = $::application-> uiScaling;
$ph = $::application->font->height * 2 if $ph < $::application->font->height * 2;
my $w = Prima::MainWindow-> create(
	size    => [ 350 * $sc, 20 + ($ph+8)*13],
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

my @mapset = map {
	my ($x,$a) = $ptr-> split;
	my $j = Prima::Icon-> create;
	$x-> begin_paint;
	$x-> text_out( $_, 3, 3);
	$x-> end_paint;
	$a-> begin_paint;
	$a-> text_out( $_, 3, 3);
	$a-> end_paint;
	$j-> combine( $x, $a);
	$j;
} 1..4;
my $mapsetID = 0;

my $b = $w-> insert( SpeedButton =>
	left    => $sc * ( 10 + (($i-1) % 2)*170 ),
	width   => 160 * $sc,
	height  => $ph+4,
	bottom  => $w-> height - int(($i+1)/2) * ($ph+8) - 10,
	pointer => $ptr,
	image   => $ptr,
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
