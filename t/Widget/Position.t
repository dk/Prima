use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

plan tests => 13;

my $window = create_window;

my $ww = $window-> insert( Widget =>
	origin    => [ 10, 10],
	growMode  => gm::GrowLoX,
);

is( $ww-> left, 10, "create" );
is( $ww-> bottom, 10, "create" );

$ww-> origin( 30, 30);

is( $ww-> left, 30, "runtime" );
is( $ww-> bottom, 30, "runtime" );

$window-> size( 200, 200);
my @wp = $window-> size;
$window-> size( 300, 300);
$wp[0] = $window-> width - $wp[0];
$wp[1] = $window-> height- $wp[1];

is( $ww-> left, 30 + $wp[0], "horizontal" );
is( $ww-> bottom, 30, "horizontal" );
$ww-> growMode( gm::GrowLoY);
$window-> size( 200, 200);
is( $ww-> left, 30 + $wp[0], "vertical" );
is( $ww-> bottom, 30 - $wp[1], "vertical" );

$ww-> hide;
reset_flag();
$ww-> set( onMove => \&set_flag );
$ww-> origin(10,10);
is( $ww-> left, 10, "hidden" );
is( $ww-> bottom, 10, "hidden" );
ok( wait_flag, "event" );

$ww-> owner( $::application);
$ww-> owner( $window );
is( $ww-> left, 10, "reparent" );
is( $ww-> bottom, 10, "reparent" );

$ww-> destroy;
