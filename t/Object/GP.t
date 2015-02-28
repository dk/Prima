# $Id$
print "1..21 create,pixel,pixel,line,lines,polyline,rectangle,ellipse,arc,bar,fillpoly,fill_ellipse,fill_chord,flood_fill,clipRect,translate,put_image,stretch_image,xor_put,text_out,dbm(put_image)\n";

my $x = Prima::DeviceBitmap-> create( monochrome => 1, width => 8, height => 8);
# 1
ok( $x && $x-> get_paint_state);
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);

# 2
ok( $x-> pixel( 0,0) == 0xFFFFFF &&
	$x-> pixel( 0,2) == 0xFFFFFF &&
	$x-> pixel( 0,4) == 0xFFFFFF &&
	$x-> pixel( 0,6) == 0xFFFFFF &&
	$x-> pixel( 1,0) == 0xFFFFFF &&
	$x-> pixel( 2,0) == 0xFFFFFF &&
	$x-> pixel( 3,7) == 0xFFFFFF &&
	$x-> pixel( 4,0) == 0xFFFFFF &&
	$x-> pixel( 5,0) == 0xFFFFFF &&
	$x-> pixel( 6,6) == 0xFFFFFF &&
	$x-> pixel( 7,5) == 0xFFFFFF &&
	$x-> pixel( 2,2) == 0xFFFFFF &&
	$x-> pixel( 4,3) == 0xFFFFFF &&
	$x-> pixel( 5,6) == 0xFFFFFF &&
	$x-> pixel( 6,5) == 0xFFFFFF &&
	$x-> pixel( 0,7) == 0xFFFFFF
);

# 3
$x-> pixel( 1,1,cl::Black);
$x-> pixel( 2,2,cl::Black);
$x-> pixel( 6,6,cl::Black);
$x-> pixel( 4,5,cl::Black);
ok( $x-> pixel( 1,1) == 0 &&
	$x-> pixel( 2,2) == 0 &&
	$x-> pixel( 6,6) == 0 &&
	$x-> pixel( 4,5) == 0
);

# 4
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> line( 1, 1, 6, 6);
ok( $x-> pixel( 1,1) == 0 &&
	$x-> pixel( 6,6) == 0 &&
	$x-> pixel( 0,0) == 0xFFFFFF &&
	$x-> pixel( 7,7) == 0xFFFFFF
);

# 5
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> lines([2,1,6,1,4,6,3,3]);
ok( $x-> pixel( 2,1) == 0 &&
	$x-> pixel( 6,1) == 0 &&
	$x-> pixel( 4,6) == 0 &&
	$x-> pixel( 3,3) == 0 &&
	$x-> pixel( 1,1) == 0xFFFFFF &&
	$x-> pixel( 1,0) == 0xFFFFFF &&
	$x-> pixel( 7,1) == 0xFFFFFF &&
	$x-> pixel( 4,7) == 0xFFFFFF &&
	$x-> pixel( 7,0) == 0xFFFFFF
);

# 6
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> polyline([2,1,6,1,4,6]);
ok( $x-> pixel( 2,1) == 0 &&
	$x-> pixel( 6,1) == 0 &&
	$x-> pixel( 4,6) == 0 &&
	$x-> pixel( 1,1) == 0xFFFFFF &&
	$x-> pixel( 1,0) == 0xFFFFFF &&
	$x-> pixel( 7,1) == 0xFFFFFF &&
	$x-> pixel( 4,7) == 0xFFFFFF &&
	$x-> pixel( 7,0) == 0xFFFFFF
);

# 7
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> rectangle( 1, 1, 3, 3);
ok( $x-> pixel( 1,1) == 0 &&
	$x-> pixel( 3,3) == 0 &&
	$x-> pixel( 2,2) == 0xFFFFFF &&
	$x-> pixel( 0,0) == 0xFFFFFF &&
	$x-> pixel( 4,4) == 0xFFFFFF
);

# 8
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> ellipse( 2, 2, 3, 3);
ok( $x-> pixel( 1,2) == 0 &&
	$x-> pixel( 2,1) == 0 &&
	$x-> pixel( 2,3) == 0 &&
	$x-> pixel( 3,2) == 0 &&
	$x-> pixel( 2,2) == 0xFFFFFF &&
	$x-> pixel( 1,0) == 0xFFFFFF &&
	$x-> pixel( 0,1) == 0xFFFFFF &&
	$x-> pixel( 4,3) == 0xFFFFFF &&
	$x-> pixel( 3,4) == 0xFFFFFF
);

# 9
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> arc( 4, 4, 7, 7, 30, 210);
ok( $x-> pixel( 1,4) == 0 &&
	$x-> pixel( 4,7) == 0 &&
	$x-> pixel( 7,4) == 0xFFFFFF &&
	$x-> pixel( 4,1) == 0xFFFFFF
);



# 10
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> bar( 3, 3, 1, 1);
ok( $x-> pixel( 1,1) == 0 &&
	$x-> pixel( 3,3) == 0 &&
	$x-> pixel( 2,2) == 0 &&
	$x-> pixel( 0,0) == 0xFFFFFF &&
	$x-> pixel( 4,4) == 0xFFFFFF
);

# 11
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> fillpoly([2,1,6,1,4,6]);
ok( $x-> pixel( 2,1) == 0 &&
	$x-> pixel( 6,1) == 0 &&
	$x-> pixel( 4,6) == 0 &&
	$x-> pixel( 4,4) == 0 &&
	$x-> pixel( 1,1) == 0xFFFFFF &&
	$x-> pixel( 1,0) == 0xFFFFFF &&
	$x-> pixel( 7,1) == 0xFFFFFF &&
	$x-> pixel( 4,7) == 0xFFFFFF &&
	$x-> pixel( 7,0) == 0xFFFFFF
);

# 12
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> fill_ellipse( 2, 2, 3, 3);
ok( $x-> pixel( 1,2) == 0 &&
	$x-> pixel( 2,1) == 0 &&
	$x-> pixel( 2,3) == 0 &&
	$x-> pixel( 3,2) == 0 &&
	$x-> pixel( 2,2) == 0 &&
	$x-> pixel( 1,0) == 0xFFFFFF &&
	$x-> pixel( 0,1) == 0xFFFFFF &&
	$x-> pixel( 4,3) == 0xFFFFFF &&
	$x-> pixel( 3,4) == 0xFFFFFF
);


# 13
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> fill_chord( 4, 4, 7, 7, 30, 210);
ok( $x-> pixel( 1,4) == 0 &&
	$x-> pixel( 4,7) == 0 &&
	$x-> pixel( 4,4) == 0 &&
	$x-> pixel( 7,4) == 0xFFFFFF &&
	$x-> pixel( 4,1) == 0xFFFFFF
);

# 14
$x-> color( cl::White);
$x-> flood_fill( 1, 4, cl::Black);
ok( $x-> pixel( 1,4) == 0xFFFFFF &&
	$x-> pixel( 4,7) == 0xFFFFFF &&
	$x-> pixel( 4,4) == 0xFFFFFF
);

# 15
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> clipRect( 2, 2, 3, 3);
$x-> bar( 1, 1, 2, 2);
$x-> clipRect( 0, 0, $x-> size);
ok( $x-> pixel( 2,2) == 0 &&
	$x-> pixel( 1,1) == 0xFFFFFF
);

# 16
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> translate( -1, 1);
$x-> bar( 2, 2, 3, 3);
$x-> translate( 0, 0);
ok( $x-> pixel( 1,4) == 0 &&
	$x-> pixel( 3,2) == 0xFFFFFF
);

# 17
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> pixel( 1,2,cl::Black);
$x-> pixel( 2,1,cl::Black);
$x-> pixel( 2,3,cl::Black);
$x-> pixel( 3,2,cl::Black);
$x-> pixel( 2,2,cl::Black);
my $image = $x-> image;
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> put_image( 0, 0, $image);

ok( $x-> pixel( 1,2) == 0 &&
	$x-> pixel( 2,1) == 0 &&
	$x-> pixel( 2,3) == 0 &&
	$x-> pixel( 3,2) == 0 &&
	$x-> pixel( 2,2) == 0 &&
	$x-> pixel( 1,1) == 0xFFFFFF &&
	$x-> pixel( 3,3) == 0xFFFFFF
);

# 18
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> stretch_image( 0, 0, 16, 16, $image);
ok( $x-> pixel( 2,4) == 0 &&
	$x-> pixel( 4,2) == 0 &&
	$x-> pixel( 4,6) == 0 &&
	$x-> pixel( 6,4) == 0 &&
	$x-> pixel( 4,4) == 0 &&
	$x-> pixel( 2,2) == 0xFFFFFF &&
	$x-> pixel( 6,6) == 0xFFFFFF
);

# 19
$x-> put_image_indirect( $image, 0, 0, 0, 0, 16, 16, 8, 8, rop::XorPut);
ok( $x-> pixel( 0,0) == 0 &&
	$x-> pixel( 1,1) == 0 &&
	$x-> pixel( 2,2) == 0 &&
	$x-> pixel( 3,3) == 0 &&
	$x-> pixel( 4,4) == 0 &&
	$x-> pixel( 5,5) == 0 &&
	$x-> pixel( 6,6) == 0 &&
	$x-> pixel( 7,7) == 0
);

$image-> destroy;

# 20
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> font-> height( 8);
$x-> text_out( "xyz", 0, 0);
my ( $i, $j);
my ( $wh, $bl) = ( 0, 0);
for ( $i = 0; $i < 8; $i++) {
	for ( $j = 0; $j < 8; $j++) {
		$x-> pixel( $i,$j) == 0 ?
			$bl++ : $wh++;
	}
}
ok( $bl > 5);

# 21
my $y = Prima::DeviceBitmap-> create( monochrome => 1, width => 2, height => 2);
$y-> clear;
$y-> pixel( 0, 0, cl::Black);
$y-> translate( 1, 1);
$x-> color( cl::White);
$x-> bar(0,0,8,8);
$x-> put_image( 0, 0, $y);
$y-> destroy;
ok( $x-> pixel( 0, 0) == 0);

$x-> destroy;

1;
