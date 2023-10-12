=pod

=head1 NAME

examples/f_fill.pl - gradient fill example

=head1 FEATURES

Demonstrates the usage of graphic context regions.  Note that the C<$i> region
is not created, but is assigned on every onPaint. This tests whether the image
object is able to hold a cached copy of the region.

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
my $path;

if ( $i->font->{vector}) {
	$i = $i->new_path;
	$i->translate( 10, 10 );
	$i->text('PRIMA', fill => 1);
	$path = $i;
	$i = $i->region(fm::Winding);
} else {
	$i-> begin_paint;
	$i-> color( cl::Black);
	$i-> bar(0,0,@is);
	$i-> color( cl::White);
	$i-> text_out( "PRIMA", 0,0);
	$i-> end_paint;
	$i = $i->to_region;
}

my ($g1, $g2);

my $w = Prima::MainWindow-> create(
	size   => [ @is],
	centered => 1,
	buffered => 1,
	palette => [ map { ($_) x 3 } 0..255 ],
	onPaint => sub {
		my ( $self, $canvas) = @_;

		# XXX modern X11 is extremely bad with complex regions, use XRender
		$canvas->antialias(1);

		$canvas->clear;
		$g1->ellipse($is[0]/2 ,$is[1]/2, $is[0], $is[0]);
		$canvas-> region( $i);
		$g2->ellipse($is[0]/2 ,$is[1]/2, $is[0], $is[0]);
		$canvas-> linePattern(lp::Dot);
		$canvas-> lineWidth(6);
		if ( $path ) {
			$path->canvas($canvas);
			$path->stroke;
		}
	},
);

$g1 = $w->new_gradient( palette => [cl::White, cl::Black ] );
$g2 = $w->new_gradient( palette => [cl::Black, cl::White ] );

run Prima;
