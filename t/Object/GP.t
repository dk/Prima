use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

plan tests => 119;

my $x = Prima::DeviceBitmap-> create( type => dbt::Bitmap, width => 8, height => 8);
# 1
ok( $x && $x-> get_paint_state, "create");
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);

# 2
my $coordinates = [
    [ 0, 0, 0xFFFFFF ],
    [ 0, 2, 0xFFFFFF ],
    [ 0, 4, 0xFFFFFF ],
    [ 0, 6, 0xFFFFFF ],
    [ 1, 0, 0xFFFFFF ],
    [ 2, 0, 0xFFFFFF ],
    [ 3, 7, 0xFFFFFF ],
    [ 4, 0, 0xFFFFFF ],
    [ 5, 0, 0xFFFFFF ],
    [ 6, 6, 0xFFFFFF ],
    [ 7, 5, 0xFFFFFF ],
    [ 2, 2, 0xFFFFFF ],
    [ 4, 3, 0xFFFFFF ],
    [ 5, 6, 0xFFFFFF ],
    [ 6, 5, 0xFFFFFF ],
    [ 0, 7, 0xFFFFFF ]
];
run_tests( $x, $coordinates, "pixel");

# 3
$coordinates = [
    [ 1, 1, 0 ],
    [ 2, 2, 0 ],
    [ 6, 6, 0 ],
    [ 4, 5, 0 ]
];
for my $coordinate( @$coordinates ) {
    my ($xco, $yco) = @$coordinate;
    $x-> pixel( $xco, $yco, cl::Black);
}
run_tests( $x, $coordinates, "line" );

# 4
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> line( 1, 1, 6, 6);
$coordinates = [
    [ 1, 1, 0 ],
    [ 6, 6, 0 ],
    [ 0, 0, 0xFFFFFF ],
    [ 7, 7, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "line" );

# 5
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> lines([2,1,6,1,4,6,3,3]);
$coordinates = [
    [ 2, 1, 0 ],
    [ 6, 1, 0 ],
    [ 4, 6, 0 ],
    [ 3, 3, 0 ],
    [ 1, 1, 0xFFFFFF ],
    [ 1, 0, 0xFFFFFF ],
    [ 7, 1, 0xFFFFFF ],
    [ 4, 7, 0xFFFFFF ],
    [ 7, 0, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "lines");

# 6
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> polyline([2,1,6,1,4,6]);
$coordinates = [
    [ 2, 1, 0 ],
    [ 6, 1, 0 ],
    [ 4, 6, 0 ],
    [ 1, 1, 0xFFFFFF ],
    [ 1, 0, 0xFFFFFF ],
    [ 7, 1, 0xFFFFFF ],
    [ 4, 7, 0xFFFFFF ],
    [ 7, 0, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "polyline");

# 7
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> rectangle( 1, 1, 3, 3);
$coordinates = [
    [ 1, 1, 0 ],
    [ 3, 3, 0 ],
    [ 2, 2, 0xFFFFFF ],
    [ 0, 0, 0xFFFFFF ],
    [ 4, 4, 0xFFFFFF ]
];
run_tests( $x, $coordinates, "rectangle");

# 8
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> ellipse( 2, 2, 3, 3);
$coordinates = [
    [ 1, 2, 0 ],
    [ 2, 1, 0 ],
    [ 2, 3, 0 ],
    [ 3, 2, 0 ],
    [ 2, 2, 0xFFFFFF ],
    [ 1, 0, 0xFFFFFF ],
    [ 0, 1, 0xFFFFFF ],
    [ 4, 3, 0xFFFFFF ],
    [ 3, 4, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "ellipse");

# 9
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> arc( 4, 4, 7, 7, 30, 210);
$coordinates = [
    [ 1, 4, 0 ],
    [ 4, 7, 0 ],
    [ 7, 4, 0xFFFFFF ],
    [ 4, 1, 0xFFFFFF ]
];
run_tests( $x, $coordinates, "arc");


# 10
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> bar( 3, 3, 1, 1);
$coordinates = [
    [ 1, 1, 0 ],
    [ 3, 3, 0 ],
    [ 2, 2, 0 ],
    [ 0, 0, 0xFFFFFF ],
    [ 0, 4, 0xFFFFFF ]
];
run_tests( $x, $coordinates, "bar");

# 11
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> fillpoly([2,1,6,1,4,6]);
$coordinates = [
    [ 2, 1, 0 ],
    [ 6, 1, 0 ],
    [ 4, 6, 0 ],
    [ 4, 4, 0 ],
    [ 1, 1, 0xFFFFFF ],
    [ 1, 0, 0xFFFFFF ],
    [ 7, 1, 0xFFFFFF ],
    [ 4, 7, 0xFFFFFF ],
    [ 7, 0, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "fillpoly");


# 12
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> fill_ellipse( 2, 2, 3, 3);
$coordinates = [
    [ 1, 2, 0 ],
    [ 2, 1, 0 ],
    [ 2, 3, 0 ],
    [ 3, 2, 0 ],
    [ 2, 2, 0 ],
    [ 1, 0, 0xFFFFFF ],
    [ 0, 1, 0xFFFFFF ],
    [ 4, 3, 0xFFFFFF ],
    [ 3, 4, 0xFFFFFF ],
    ];
run_tests( $x, $coordinates, "fill_ellipse");


# 13
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> fill_chord( 4, 4, 7, 7, 30, 210);
$coordinates = [
    [ 1, 4, 0 ],
    [ 4, 7, 0 ],
    [ 4, 4, 0 ],
    [ 7, 4, 0xFFFFFF ],
    [ 4, 1, 0xFFFFFF ]
];
run_tests( $x, $coordinates, "fill_chord");

# 14
$x-> color( cl::White);
$x-> flood_fill( 1, 4, cl::Black);
$coordinates = [
    [ 1, 4, 0xFFFFFF ],
    [ 4, 7, 0xFFFFFF ],
    [ 4, 4, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "flood_fill");

# 15
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> clipRect( 2, 2, 3, 3);
$x-> bar( 1, 1, 2, 2);
$x-> clipRect( 0, 0, $x-> size);
$coordinates = [
    [ 2, 2, 0 ],
    [ 1, 1, 0xFFFFFF ]
    ];
run_tests( $x, $coordinates, "clipRect");

# 16
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> color( cl::Black);
$x-> translate( -1, 1);
$x-> bar( 2, 2, 3, 3);
$x-> translate( 0, 0);
$coordinates = [
    [ 1, 4, 0 ],
    [ 3, 2, 0xFFFFFF ],
    ];
run_tests( $x, $coordinates, "translate" );

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
$coordinates = [
    [ 1, 2, 0 ],
    [ 2, 1, 0 ],
    [ 2, 3, 0 ],
    [ 3, 2, 0 ],
    [ 2, 2, 0 ],
    [ 1, 1, 0xFFFFFF ],
    [ 3, 3, 0xFFFFFF ]
];
run_tests( $x, $coordinates, "put_image");

# 18
$x-> color( cl::White);
$x-> bar( 0, 0, 7, 7);
$x-> stretch_image( 0, 0, 16, 16, $image);
$coordinates = [
    [ 2, 4, 0 ],
    [ 4, 2, 0 ],
    [ 4, 6, 0 ],
    [ 6, 4, 0 ],
    [ 4, 4, 0 ],
    [ 2, 2, 0xFFFFFF ],
    [ 6, 6, 0xFFFFFF ],
    ];
run_tests( $x, $coordinates, "stretch_image");

# 19
$x-> put_image_indirect( $image, 0, 0, 0, 0, 16, 16, 8, 8, rop::XorPut);
$coordinates = [
    [ 0, 0, 0 ],
    [ 1, 1, 0 ],
    [ 2, 2, 0 ],
    [ 3, 3, 0 ],
    [ 4, 4, 0 ],
    [ 5, 5, 0 ],
    [ 6, 6, 0 ],
    [ 7, 7, 0 ],
];
run_tests( $x, $coordinates, "xor_put");
$image-> destroy;

# 20
sub text_out_test
{
	my $x = shift;
	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> font-> height( 8);
	$x-> text_out( "xyz", 0, 0);
	my $xi = $x-> image;
	$xi->type(im::BW);
	my ( $i, $j);
	my ( $wh, $bl) = ( 0, 0);
	for ( $i = 0; $i < 8; $i++) {
		for ( $j = 0; $j < 8; $j++) {
			$xi-> pixel( $i,$j) == 0 ?
				$bl++ : $wh++;
		}
	}
	return $bl;
}

my $bl = text_out_test($x);
if ( $bl == 0 ) {
	# did we hit this bug? https://gitlab.freedesktop.org/xorg/xserver/issues/87
	my $x = Prima::DeviceBitmap-> create( type => dbt::Pixmap, width => 8, height => 8);
	$bl = text_out_test($x);
}

	
cmp_ok( $bl, '>', 5, "text_out");

# 21
my $y = Prima::DeviceBitmap-> create( type => dbt::Bitmap, width => 2, height => 2);
$y-> clear;
$y-> pixel( 0, 0, cl::Black);
$y-> translate( 1, 1);
$x-> color( cl::White);
$x-> bar(0,0,8,8);
$x-> set( color => cl::Black, backColor => cl::White);
$x-> put_image( 0, 0, $y);
$y-> destroy;
is( $x-> pixel( 0, 0), 0, 'dbm(put_image)');

$x-> destroy;

sub run_tests {
    my ($x, $coordinates, $name) =  @_;
    for my $coordinate( @$coordinates ) {
        my ($xco, $yco, $expected) = @$coordinate;
        is( $x->pixel( $xco, $yco ), $expected, "$name ($xco, $yco)" );
    }
}
