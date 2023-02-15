use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

## image fill patterns:
# mono with mostly 0s
my $fp0m = Prima::Image->new( type => im::BW, size => [2,2] );
$fp0m->pixel(1,1,0xffffff);
# mono with mostly 1s
my $fp1m = Prima::Image->new( type => im::BW, size => [2,2] );
$fp1m->pixel(0,0,0xffffff);
$fp1m->pixel(0,1,0xffffff);
$fp1m->pixel(1,0,0xffffff);
# their colored clones
my $fp0a = $fp0m->clone( type => 1 );
my $fp1a = $fp1m->clone( type => 1 );
my $fp0c = $fp0m->clone( type => 4 );
my $fp1c = $fp1m->clone( type => 4 );


my $x = Prima::DeviceBitmap-> create( type => dbt::Bitmap, width => 8, height => 8);

$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> linePattern( lp::Dot);
$x-> line( 0, 4, 7, 4);
$x-> linePattern( lp::Solid);
my $bl = 0;
my $i;
for ( $i = 0; $i < 8; $i++) {
       $bl++ if $x-> pixel( $i, 4) == 0;
}
cmp_ok( $bl, '<', 6, "linePattern");

$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> lineWidth( 4);
$x-> line( 3, 4, 5, 4);
$x-> lineWidth( 1);
is( $x-> pixel( 2, 4), 0, "lineWidth");
is( $x-> pixel( 5, 3), 0, "lineWidth");

$x->rop2(rop::CopyPut);
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> backColor( cl::White);
$x-> fillPattern( fp::SimpleDots);
$x-> bar( 0, 0, 7, 7);
$bl = $x-> image;
$bl-> type( im::Byte);
my $bl1 = $bl->data;
SKIP: {
	skip "bad graphics driver", 2 unless
		unpack('H*', $bl1) eq (('ff00' x 4).('00ff' x 4))x4 or
		unpack('H*', $bl1) eq (('00ff' x 4).('ff00' x 4))x4;

$x-> fillPattern( fp::SimpleDots);
$x-> fillPatternOffset(1,0);
$x-> bar( 0, 0, 7, 7);
$bl = $x-> image;
$bl->type(im::Byte);
my $bl2 = $bl->data;
isnt( $bl1, $bl2, 'fillPatternOffset not same');

$x-> fillPatternOffset(2,2);
$x-> bar( 0, 0, 7, 7);
$bl = $x-> image;
$bl->type(im::Byte);
$bl2 = $bl->data;
is( $bl1, $bl2, 'fillPatternOffset same');

$x-> clear;
$x-> fillPattern([ 0x00, 0x80, 0x40, 0x20, 0x10, 0x8, 0x04, 0x02]);
$x-> fillPatternOffset(0,0);
$x-> bar( 0, 0, 7, 7);
$bl = $x->image;
$bl->type(im::Byte);
is( $bl->pixel(0,0), 255, 'endiness pat.#1');
is( $bl->pixel(0,1), 0, 'endiness pat.#2');
is( $bl->pixel(1,2), 0, 'endiness pat.#3');
is( $bl->pixel(3,3), 255, 'endiness pat.#4');
is( $bl->pixel(4,4), 255, 'endiness pat.#5');
is( $bl->pixel(5,6), 0, 'endiness pat.#6');
is( $bl->pixel(6,7), 0, 'endiness pat.#7');
is( $bl->pixel(7,7), 255, 'endiness pat.#8');

$x->clear;
$x->fillPattern($bl);
$x-> bar( 0, 0, 7, 7);
$bl = $x->image;
$bl->type(im::Byte);
is( $bl->pixel(0,0), 255, 'endiness tile.#1');
is( $bl->pixel(0,1), 0, 'endiness tile.#2');
is( $bl->pixel(1,2), 0, 'endiness tile.#3');
is( $bl->pixel(3,3), 255, 'endiness tile.#4');
is( $bl->pixel(4,4), 255, 'endiness tile.#5');
is( $bl->pixel(5,6), 0, 'endiness tile.#6');
is( $bl->pixel(6,7), 0, 'endiness tile.#7');
is( $bl->pixel(7,7), 255, 'endiness tile.#8');
}

$x-> fillPattern( fp::Solid);
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( 0x808080);
$x-> bar( 0, 0, 7, 7);
$bl = 0;
for ( $i = 0; $i < 8; $i++) {
       $bl++ if $x-> pixel( $i, 4) == 0;
}
SKIP: {
skip "bad graphics driver", 2 if $bl == 8;
cmp_ok( $bl, '>', 2, "dithering" );
cmp_ok( $bl, '<', 6, "dithering" );
}

$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> rop( rop::XorPut);
$x-> bar( 0, 0, 1, 1);
$x-> rop( rop::CopyPut);
is( $x-> pixel( 0, 0), 0, "rob paint" );
$x-> destroy;


my $subtest;
sub check
{
	my ($test, $sum, $fp, %opt) = @_;
	$x->set(%opt, fillPattern => $fp);
	$x->bar(0,0,7,7);
	my $xsum = $x->image->extract(0,0,2,2)->clone(type => im::Byte)->sum / 255;
	$xsum = int($xsum * 10 + .5) / 10;
	is( $xsum, $sum, "$test on $subtest");
}


my $can_argb = $::application->get_system_value(sv::LayeredWidgets);
for my $aa ( 0, 1 ) {
for my $subtype ( dbt::Pixmap, dbt::Layered ) {
	if ( $subtype == dbt::Bitmap ) {
		$subtest = 'bitmap';
	} elsif ( $subtype == dbt::Pixmap ) {
		$subtest = 'pixmap';
	} else {
		$subtest = 'layered';
		unless ( $can_argb ) {
			diag "skipped layered\n";
			next;
		}
	}
	$subtest .= '.aa' if $aa;

	$x = Prima::DeviceBitmap-> create( type => $subtype, width => 8, height => 8, antialias => 0);
	$x->rop2(rop::CopyPut);
	check( "fp0m WB", 1, $fp0m, color => cl::White, backColor => cl::Black );
	check( "fp0m BW", 3, $fp0m, color => cl::Black, backColor => cl::White );
	check( "fp1m WB", 3, $fp1m, color => cl::White, backColor => cl::Black );
	check( "fp1m WB", 3, $fp1m, color => cl::White, backColor => cl::Black );
	check( "fpXm WW", 4, $fp0m, color => cl::White, backColor => cl::White );
	check( "fpXm BB", 0, $fp1m, color => cl::Black, backColor => cl::Black );

	$x->rop2(rop::NoOper);
	$x->backColor(cl::White);
	$x->clear;
	check( "fpXm BT", 1, $fp1m, color => cl::Black, backColor => cl::Black );
	$x->rop2(rop::CopyPut);

	check( "fp0a", 1, $fp0a, color => cl::White, backColor => cl::Black );
	check( "fp1a", 3, $fp1a, color => cl::White, backColor => cl::Black );
	check( "fpXa", 3, $fp1a, color => cl::White, backColor => cl::White );
	check( "fp0c", 1, $fp0c, color => cl::White, backColor => cl::Black );
	check( "fp1c", 3, $fp1c, color => cl::White, backColor => cl::Black );
	check( "fpXc", 3, $fp1c, color => cl::White, backColor => cl::White );

	$x->rop2(rop::NoOper);
	$x->backColor(cl::White);
	$x->clear;
	check("fp0f", 4, fp::Empty,         color => cl::Black, backColor => cl::White);
	check("fp8f", 0, fp::Solid,         color => cl::Black, backColor => cl::White);
	$x->clear;
	check("fp2f", 2, [(0x55,0xAA) x 4], color => cl::Black, backColor => cl::White);
}}

if ( $can_argb  ) {
	my $mask1x0 = Prima::Image->new( size => [2,2], type => im::BW, data => "\1" x 8);
	my $mask8x8 = Prima::Image->new( size => [2,2], type => im::Byte, data => "\x80" x 8);
	my $mask8xf = Prima::Image->new( size => [2,2], type => im::Byte, data => "\xff" x 8);

	$x = Prima::DeviceBitmap-> create( type => dbt::Layered, width => 8, height => 8, antialias => 1);
	$x->clear;
	check( "fpi0m", 1, Prima::Icon->create_combined( $fp0m, $mask1x0 ), color => cl::White, backColor => cl::White);
	$x->backColor(cl::White);
	$x->clear;
	check( "fpi8c", 3.5, Prima::Icon->create_combined( $fp1c, $mask8x8 ));
	$x->clear;
	check( "fpifc", 3, Prima::Icon->create_combined( $fp1c, $mask8xf ));

}

my $ii = Prima::Image->new( type => im::BW, size => [8,8]);
$ii->clear;
$ii->rop2(rop::CopyPut);
$ii->fillPattern(fp::LtSlash);
$ii->bar(0,0,7,7);
my $jj = $ii->data;
$ii->begin_paint;
$ii->end_paint;
is($ii->data, $jj, "begin/end paint preserves data");

done_testing;
