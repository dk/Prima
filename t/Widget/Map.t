use Test::More;
use Prima::Test qw(noX11 w dong set_dong);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $ww = $Prima::Test::w->insert( 'Widget' => origin => [ 10, 10],);
my @xr = $Prima::Test::w-> origin;
my @r = $ww-> screen_to_client( $xr[0] + 10, $xr[1] + 10);
cmp_ok( $r[0], '==', 0, "screen to client" );
cmp_ok( $r[1], '==', 0, "screen to client" );

@r = $ww-> client_to_screen( 0, 0);
cmp_ok( $r[0], '==', $xr[0] + 10, "client to screen" );
cmp_ok( $r[1], '==', $xr[1] + 10, "client to screen" );

@r = $::application-> client_to_screen( 1, 2);
cmp_ok( $r[0], '==', 1, "application" );
cmp_ok( $r[1], '==', 2, "application" );

$ww-> destroy;

done_testing();
