=pod 

=head1 NAME

examples/f_fill.pl - A gradient fill example

=head1 FEATURES

Demonstrates the usage of graphic context regions.
Note that the $i region is not created, but is assigned
on every onPaint. Tests whether image object is able
to hold a cached region copy.

=cut

use strict;
use warnings;
use Prima qw(Application);

my $i = Prima::Image-> create(
	preserveType => 1,
	type => im::BW,
	font => { size => 100, style => fs::Bold|fs::Italic },
);
$i-> begin_paint_info;
my $textx = $i-> get_text_width( "PRIMA");
my $texty = $i-> font-> height;
$i-> end_paint_info;
$i-> size( $textx + 20, $texty + 20);

my @is = $i-> size;
$i-> begin_paint;
$i-> color( cl::Black);
$i-> bar(0,0,@is);
$i-> color( cl::White);
$i-> text_out( "PRIMA", 0,0);
$i-> end_paint;


my $w = Prima::MainWindow-> create(
	size   => [ @is],
	centered => 1,
	buffered => 1,
	palette => [ map { ($_) x 3 } 0..255 ],
	onPaint => sub {
	my ( $self, $canvas) = @_;
		$canvas->clear;
		$canvas->gradient_ellipse($is[0]/2 ,$is[1]/2, $is[0], $is[0], {palette => [cl::White, cl::Black ]});
		$canvas-> region( $i);
		$canvas->gradient_ellipse($is[0]/2 ,$is[1]/2, $is[0], $is[0], {palette => [cl::Black, cl::White]});
	},
);

run Prima;
