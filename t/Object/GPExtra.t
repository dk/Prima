use strict;
use warnings;

use Test::More tests => 8;
use Prima::Test;

my $x = Prima::DeviceBitmap-> create( monochrome => 1, width => 8, height => 8);

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
$x-> lineWidth( 3);
$x-> line( 3, 4, 5, 4);
$x-> lineWidth( 1);
is( $x-> pixel( 2, 4), 0, "lineWidth");
is( $x-> pixel( 5, 3), 0, "lineWidth");

$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> backColor( cl::White);
$x-> fillPattern( fp::SimpleDots);
$x-> bar( 0, 0, 7, 7);
$x-> fillPattern( fp::Solid);
$bl = $x-> image;
$bl-> type( im::Byte);
$bl = $bl-> sum;
cmp_ok( $bl, '>', 6000, "fillPattern" );
cmp_ok( $bl, '<', 10000, "fillPattern" );

$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( 0x808080);
$x-> bar( 0, 0, 7, 7);
$bl = 0;
for ( $i = 0; $i < 8; $i++) {
       $bl++ if $x-> pixel( $i, 4) == 0;
}
cmp_ok( $bl, '>', 2, "dithering" );
cmp_ok( $bl, '<', 6, "dithering" );

$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> rop( rop::XorPut);
$x-> bar( 0, 0, 1, 1);
$x-> rop( rop::CopyPut);
is( $x-> pixel( 0, 0), 0, "rob paint" );

$x-> destroy;
