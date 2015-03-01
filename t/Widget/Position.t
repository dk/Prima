use Test::More;
use Prima::Test qw(noX11 w dong);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $ww = $Prima::Test::w-> insert( Widget =>
	origin    => [ 10, 10],
	growMode  => gm::GrowLoX,
);

cmp_ok( $ww-> left, '==', 10, "create" );
cmp_ok( $ww-> bottom, '==', 10, "create" );

$ww-> origin( 30, 30);

cmp_ok( $ww-> left, '==', 30, "runtime" );
cmp_ok( $ww-> bottom, '==', 30, "runtime" );

$Prima::Test::w-> size( 200, 200);
my @wp = $Prima::Test::w-> size;
$Prima::Test::w-> size( 300, 300);
$wp[0] = $Prima::Test::w-> width - $wp[0];
$wp[1] = $Prima::Test::w-> height- $wp[1];

cmp_ok( $ww-> left, '==', 30 + $wp[0], "horizontal" );
cmp_ok( $ww-> bottom, '==', 30, "horizontal" );
$ww-> growMode( gm::GrowLoY);
$Prima::Test::w-> size( 200, 200);
cmp_ok( $ww-> left, '==', 30 + $wp[0], "vertical" );
cmp_ok( $ww-> bottom, '==', 30 - $wp[1], "vertical" );

$ww-> hide;
$dong = 0;
$ww-> set( onMove => sub { $Prima::Test::dong = 1; });
$ww-> origin(10,10);
cmp_ok( $ww-> left, '==', 10, "hidden" );
cmp_ok( $ww-> bottom, '==', 10, "hidden" );
ok( $Prima::Test::dong || &__wait, "event" );

$ww-> owner( $::application);
$ww-> owner( $Prima::Test::w );
cmp_ok( $ww-> left, '==', 10, "reparent" );
cmp_ok( $ww-> bottom, '==', 10, "reparent" );

$ww-> destroy;

done_testing();
