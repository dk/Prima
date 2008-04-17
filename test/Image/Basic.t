# $Id$
print "1..22 create,type check,paintInfo,paint type consistency,palette,pixel,paint,get_paint_state(),type,pixel,stretch,pixel bpp1,reverse stretch,bpp4,bpp8,RGB,short,long,float,complex,offline put_image,integer roundoff\n";

my $i = Prima::Image-> create(
	width => 20,
	height => 20,
	type => im::Mono,
	palette => [0,0,0,255,0,0],
	conversion => ict::None,
);

# 1,2,3
ok( $i);
ok( $i-> type == im::Mono);
$i-> begin_paint_info;
ok( $i-> get_paint_state == 2);
$i-> end_paint_info;

# 4
$i-> preserveType(0);
$i-> begin_paint;
$i-> end_paint;
$i-> preserveType(1);
if ( $::application-> get_bpp != 1) {
	ok(( im::BPP & $i-> type) != 1);
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
ok( $p[0] == 0 && $p[1] == 0 && $p[2] == 0 && $p[3] == 0xFF && $p[4] == 0 && $p[5] == 0);
ok(  $i-> pixel( 0,0) == 0 && $i-> pixel( 15,15) == 0 && $i-> pixel( 1,1) != 0);
$i-> begin_paint;
ok( $i-> get_paint_state == 1);
$i-> end_paint;

# 8,9,10
ok( $i-> get_paint_state == 0);
ok( $i-> type == im::bpp1);
ok( $i-> pixel( 0,0) == 0 && $i-> pixel( 15,15) == 0);
$i-> size( 160, 160);
# 11, 12, 13
ok( $i-> pixel( 0,0) == 0 && $i-> pixel( 159,159) == 0);
$i-> size( 16, 16);
$i-> pixel( 15, 15, 0xFFFFFF);
ok( $i-> pixel( 15,15) != 0);
$i-> size( -16, -16);
ok( $i-> pixel( 0,0) != 0 && $i-> pixel( 15,15) == 0);

my $j;
# 14, 15
for ( im::bpp4, im::bpp8) {
	$i-> type( $_);
	$i-> palette([0xFF, 0, 0xFF]);
	$i-> pixel( 3, 3, 0xFF00FF);
	$j = $i-> pixel( 3,3);
	$i-> size( -16, -16);
	ok( $i-> pixel( 12,12) == 0xFF00FF && $j == 0xFF00FF);
}

# 16, 17, 18
for ( im::RGB, im::Short, im::Long) {
	$i-> type( $_);
	$i-> pixel( 3, 3, 0x1234);
	$j = $i-> pixel( 3,3);
	$i-> size( -16, -16);
	ok( $i-> pixel( 12,12) == 0x1234 && $j == 0x1234);
}

# 19
$i-> type( im::Float);
$i-> pixel( 3, 3, 25);
$j = $i-> pixel( 3,3);
$i-> size( -16, -16);
ok( abs( 25 - $i-> pixel( 12,12)) < 1 && abs( 25-$j) < 1);

# 20
$i-> type( im::Complex);
$i-> pixel( 3, 3, [ 100, 200]);
$j = $i-> pixel( 3,3);
$i-> size( -16, -16);
ok( abs( 100 - $i-> pixel( 12,12)->[0]) < 1 && abs( 200 - $j->[1]) < 1);

# 21
$i-> type( im::Byte);
$i-> size( 2, 2);
$i-> pixel( 0, 0, 193);
$i-> pixel( 0, 1, 1);
$i-> pixel( 1, 1, 63);
$i-> pixel( 1, 0, 7);
$i-> put_image_indirect( $i, 0, 0, -1, -1, 2, 2, 2, 2, rop::NotAnd);
ok( 
	($i-> pixel( 0, 0) == 255) &&
	($i-> pixel( 0, 1) == 255) &&
	($i-> pixel( 1, 0) == 255) &&
	($i-> pixel( 1, 1) == 254) 
);

# 22
$i-> type(im::Double);
$i-> pixel(0, 0, 4.9999);
$i-> type(im::Long);
ok( $i-> pixel(0,0) == 5);

$i-> destroy;

1;
