use Test::More tests => 13;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $ww = $Prima::Test::w-> insert( Widget =>
	origin    => [ 10, 10],
	growMode  => gm::GrowLoX,
);

is( $ww-> left, 10, "create" );
is( $ww-> bottom, 10, "create" );

$ww-> origin( 30, 30);

is( $ww-> left, 30, "runtime" );
is( $ww-> bottom, 30, "runtime" );

$Prima::Test::w-> size( 200, 200);
my @wp = $Prima::Test::w-> size;
$Prima::Test::w-> size( 300, 300);
$wp[0] = $Prima::Test::w-> width - $wp[0];
$wp[1] = $Prima::Test::w-> height- $wp[1];

is( $ww-> left, 30 + $wp[0], "horizontal" );
is( $ww-> bottom, 30, "horizontal" );
$ww-> growMode( gm::GrowLoY);
$window-> size( 200, 200);
is( $ww-> left, 30 + $wp[0], "vertical" );
is( $ww-> bottom, 30 - $wp[1], "vertical" );

$ww-> hide;
$dong = 0;
$ww-> set( onMove => sub { $Prima::Test::dong = 1; });
$ww-> origin(10,10);
is( $ww-> left, 10, "hidden" );
is( $ww-> bottom, 10, "hidden" );
ok( get_flag() || &__wait, "event" );

$ww-> owner( $::application);
$ww-> owner( $window );
is( $ww-> left, 10, "reparent" );
is( $ww-> bottom, 10, "reparent" );

$ww-> destroy;

done_testing();
