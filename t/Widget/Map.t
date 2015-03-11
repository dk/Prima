use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 6;

my $window = create_window();

my $ww = $window->insert( 'Widget' => origin => [ 10, 10],);
my @xr = $window-> origin;
my @r = $ww-> screen_to_client( $xr[0] + 10, $xr[1] + 10);
is( $r[0], 0, "screen to client" );
is( $r[1], 0, "screen to client" );

@r = $ww-> client_to_screen( 0, 0);
is( $r[0], $xr[0] + 10, "client to screen" );
is( $r[1], $xr[1] + 10, "client to screen" );

@r = $::application-> client_to_screen( 1, 2);
is( $r[0], 1, "application" );
is( $r[1], 2, "application" );

$ww-> destroy;
