use Test::More;

use lib 't/lib';

BEGIN {
    use_ok( "Prima::Test" );
}
if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $i = Prima::Image-> create(
       width => 20,
       height => 20,
       type => im::Mono,
       palette => [0,0,0,255,0,0],
       conversion => ict::None,
);

# 1,2,3
ok( $i, "create" );
cmp_ok( $i-> type, '==', im::Mono, "type check" );
$i-> begin_paint_info;
cmp_ok( $i-> get_paint_state, '==', ps::Information, "paintInfo" );
$i-> end_paint_info;

# 4
$i-> preserveType(0);
$i-> begin_paint;
$i-> end_paint;
$i-> preserveType(1);
if ( $::application-> get_bpp != 1) {
    ok( im::BPP, "paint type consistency" );
    cmp_ok( $i-> type, '!=', 1, "paint type consistency" );
} else {
	skip();
}
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

cmp_ok( $p[0], '==', 0, "palette" );
cmp_ok( $p[1], '==', 0, "palette" );
cmp_ok( $p[2], '==', 0, "palette" );
cmp_ok( $p[3], '==', 0xFF, "palette" );
cmp_ok( $p[4], '==', 0, "palette"  );
cmp_ok( $p[5], '==', 0, "palette" );

cmp_ok( $i-> pixel( 0,0), '==', 0, "pixel" );
cmp_ok( $i-> pixel( 15,15), '==', 0, "pixel" );
cmp_ok( $i-> pixel( 1,1), '!=', 0, "pixel" );
$i-> begin_paint;
cmp_ok( $i-> get_paint_state, '==', ps::Enabled, "paint" );
$i-> end_paint;

# 8,9,10
cmp_ok( $i-> get_paint_state, '==', ps::Disabled, "get_paint_state()" );
cmp_ok( $i-> type, '==', im::bpp1, "type" );
cmp_ok( $i-> pixel( 0,0), '==', 0, "pixel" );
cmp_ok( $i-> pixel( 15,15), '==', 0, "pixel" );
$i-> size( 160, 160);
# 11, 12, 13
cmp_ok( $i-> pixel( 0,0), '==', 0, "stretch" );
cmp_ok( $i-> pixel( 159,159), '==', 0, "stretch" );
$i-> size( 16, 16);
$i-> pixel( 15, 15, 0xFFFFFF);
cmp_ok( $i-> pixel( 15,15), '!=', 0, "pixel bpp1" );
$i-> size( -16, -16);
cmp_ok( $i-> pixel( 0,0), '!=', 0, "reverse stretch" );
cmp_ok( $i-> pixel( 15,15), '==', 0, "reverse stretch" );

my $j;
# 14, 15
for ( [ im::bpp4, "bpp4" ], [ im::bpp8, "bpp8" ] ) {
       $i-> type( $_->[ 0 ] );
       $i-> palette([0xFF, 0, 0xFF]);
       $i-> pixel( 3, 3, 0xFF00FF);
       $j = $i-> pixel( 3,3);
       $i-> size( -16, -16);
       cmp_ok( $i-> pixel( 12,12), '==', 0xFF00FF, $_->[ 1 ] );
       cmp_ok( $j, '==', 0xFF00FF, $_->[ 1 ]);
}

# 16, 17, 18
for ( [ im::RGB, "RGB"], [ im::Short, "short"], [im::Long, "long"] ) {
       $i-> type( $_->[ 0 ]);
       $i-> pixel( 3, 3, 0x1234);
       $j = $i-> pixel( 3,3);
       $i-> size( -16, -16);
       cmp_ok( $i-> pixel( 12,12), '==', 0x1234, $_->[ 1 ] );
       cmp_ok( $j, '==', 0x1234, $_->[ 1 ]  );
}

# 19
$i-> type( im::Float);
$i-> pixel( 3, 3, 25);
$j = $i-> pixel( 3,3);
$i-> size( -16, -16);
cmp_ok( abs( 25 - $i-> pixel( 12,12)), '<', 1, "float" );
cmp_ok( abs( 25-$j), '<', 1, "float" );

# 20
$i-> type( im::Complex);
$i-> pixel( 3, 3, [ 100, 200]);
$j = $i-> pixel( 3,3);
$i-> size( -16, -16);
cmp_ok( abs( 100 - $i-> pixel( 12,12)->[0]), '< ', 1, "complex" );
cmp_ok( abs( 200 - $j->[1]), '<', 1, "complex" );

# 21
$i-> type( im::Byte);
$i-> size( 2, 2);
$i-> pixel( 0, 0, 193);
$i-> pixel( 0, 1, 1);
$i-> pixel( 1, 1, 63);
$i-> pixel( 1, 0, 7);
$i-> put_image_indirect( $i, 0, 0, -1, -1, 2, 2, 2, 2, rop::NotAnd);
cmp_ok( $i-> pixel( 0, 0), '==', 255, "offline put_image" );
cmp_ok( $i-> pixel( 0, 0), '==', 255, "offline put_image" );
cmp_ok( $i-> pixel( 0, 1), '==', 255, "offline put_image" );
cmp_ok( $i-> pixel( 1, 0), '==', 255, "offline put_image" );
cmp_ok( $i-> pixel( 1, 1), '==', 254, "offline put_image" );

# 22
$i-> type(im::Double);
$i-> pixel(0, 0, 4.9999);
$i-> type(im::Long);
cmp_ok( $i-> pixel(0,0), '==', 5, "integer roundoff" );

$i-> destroy;

done_testing();
