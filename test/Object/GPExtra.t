# $Id$
print "1..5 linePattern,lineWidth,fillPattern,dithering,rop paint\n";

my $x = Prima::DeviceBitmap-> create( monochrome => 1, width => 8, height => 8);


# 1
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
ok( $bl < 6);

# 2
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> lineWidth( 3);
$x-> line( 3, 4, 5, 4);
$x-> lineWidth( 1);
ok( $x-> pixel( 2, 4) == 0 &&
	$x-> pixel( 5, 3) == 0
);

# 3
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
ok( $bl > 6000 && $bl < 10000 );

# 4
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( 0x808080);
$x-> bar( 0, 0, 7, 7);
$bl = 0;
for ( $i = 0; $i < 8; $i++) {
	$bl++ if $x-> pixel( $i, 4) == 0;
}
ok( $bl > 2 && $bl < 6);

# 5
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> rop( rop::XorPut);
$x-> bar( 0, 0, 1, 1);
$x-> rop( rop::CopyPut);
ok( $x-> pixel( 0, 0) == 0);

$x-> destroy;
1;
