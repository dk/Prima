use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 39;

my $i = Prima::Image-> create(
       width => 20,
       height => 20,
       type => im::Mono,
       palette => [0,0,0,255,0,0],
       conversion => ict::None,
);

# 1,2,3
ok( $i, "create" );
is( $i-> type, im::Mono, "type check" );
$i-> begin_paint_info;
is( $i-> get_paint_state, ps::Information, "paintInfo" );
$i-> end_paint_info;

# 4
$i-> preserveType(0);
$i-> begin_paint;
$i-> end_paint;
$i-> preserveType(1);
SKIP: {
    if ( $::application->get_bpp != 1 ) {
        ok( im::BPP, "paint type consistency" );
        isnt( $i-> type, 1, "paint type consistency" );
    } else {
        skip( "skipping paint type consistency tests", 2);
    }
};

$i-> type( im::bpp1);
$i-> palette( [0,0,0,255,0,0]);
$i-> data(
    "\x00\x00\x00\x00\x7f\xfe\x00\x00\@\x02\x00\x00_\xfa\x00\x00P\x0a\x00\x00".
    "W\xea\x00\x00T\*\x00\x00U\xaa\x00\x00U\xaa\x00\x00T\*\x00\x00".
    "W\xea\x00\x00P\x0a\x00\x00_\xfa\x00\x00\@\x02\x00\x00\x7f\xfe\x00\x00".
    "\x00\x00\x00\x00"
);
# 5,6,7
my @p = @{$i-> palette};

is_deeply( \@p, [0,0,0,0xff,0,0], "palette" );

is( $i-> pixel( 0,0), 0, "pixel 1" );
is( $i-> pixel( 15,15), 0, "pixel 2" );
isnt( $i-> pixel( 1,1), 0, "pixel 3" );
$i-> begin_paint;
is( $i-> get_paint_state, ps::Enabled, "paint" );
$i-> end_paint;

# 8,9,10
is( $i-> get_paint_state, ps::Disabled, "get_paint_state()" );
is( $i-> type, im::bpp1, "type" );
is( $i-> pixel( 0,0), 0, "pixel 4" );
is( $i-> pixel( 15,15), 0, "pixel 5" );
$i-> size( 160, 160);
# 11, 12, 13
is( $i-> pixel( 0,0), 0, "stretch 1" );
is( $i-> pixel( 159,159), 0, "stretch 2" );
$i-> size( 16, 16);
$i-> pixel( 15, 15, 0xFFFFFF);
isnt( $i-> pixel( 15,15), 0, "pixel bpp1" );
$i-> size( -16, -16);
isnt( $i-> pixel( 0,0), 0, "reverse stretch 1" );
is( $i-> pixel( 15,15), 0, "reverse stretch 2" );

my $j;
# 14, 15
for ( [ im::bpp4, "bpp4" ], [ im::bpp8, "bpp8" ] ) {
       $i-> type( $_->[ 0 ] );
       $i-> palette([0xFF, 0, 0xFF]);
       $i-> pixel( 3, 3, 0xFF00FF);
       $j = $i-> pixel( 3,3);
       $i-> size( -16, -16);
       is( $i-> pixel( 12,12), 0xFF00FF, $_->[ 1 ] . ' 1' );
       is( $j, 0xFF00FF, $_->[ 1 ] . ' 2');
}

# 16, 17, 18
for ( [ im::RGB, "RGB"], [ im::Short, "short"], [im::Long, "long"] ) {
       $i-> type( $_->[ 0 ]);
       $i-> pixel( 3, 3, 0x1234);
       $j = $i-> pixel( 3,3);
       $i-> size( -16, -16);
       is( $i-> pixel( 12,12), 0x1234, $_->[ 1 ] . ' 1');
       is( $j, 0x1234, $_->[ 1 ] . ' 2');
}

# 19
$i-> type( im::Float);
$i-> pixel( 3, 3, 25);
$j = $i-> pixel( 3,3);
$i-> size( -16, -16);
cmp_ok( abs( 25 - $i-> pixel( 12,12)), '<', 1, "float 1" );
cmp_ok( abs( 25-$j), '<', 1, "float 2" );

# 20
$i-> type( im::Complex);
$i-> pixel( 3, 3, [ 100, 200]);
$j = $i-> pixel( 3,3);
$i-> size( -16, -16);
cmp_ok( abs( 100 - $i-> pixel( 12,12)->[0]), '< ', 1, "complex 1" );
cmp_ok( abs( 200 - $j->[1]), '<', 1, "complex 2" );

# 21
$i-> type( im::Byte);
$i-> size( 2, 2);
$i-> pixel( 0, 0, 193);
$i-> pixel( 0, 1, 1);
$i-> pixel( 1, 1, 63);
$i-> pixel( 1, 0, 7);
$i-> put_image_indirect( $i, 0, 0, -1, -1, 2, 2, 2, 2, rop::NotAnd);
is( $i-> pixel( 0, 0), 255, "offline put_image 1" );
is( $i-> pixel( 0, 0), 255, "offline put_image 2" );
is( $i-> pixel( 0, 1), 255, "offline put_image 3" );
is( $i-> pixel( 1, 0), 255, "offline put_image 4" );
is( $i-> pixel( 1, 1), 254, "offline put_image 5" );

# 22
$i-> type(im::Double);
$i-> pixel(0, 0, 4.9999);
$i-> type(im::Long);
is( $i-> pixel(0,0), 5, "integer roundoff" );

$i-> destroy;
