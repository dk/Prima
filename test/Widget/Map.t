# $Id$
print "1..3 screen to client,client to screen,application";

my $ww = $w-> insert( 'Widget' => origin => [ 10, 10],);
my @xr = $w-> origin;
my @r = $ww-> screen_to_client( $xr[0] + 10, $xr[1] + 10);
ok( $r[0] == 0 && $r[1] == 0);

@r = $ww-> client_to_screen( 0, 0);
ok( $r[0] == $xr[0] + 10 && $r[1] == $xr[1] + 10);

@r = $::application-> client_to_screen( 1, 2);
ok( $r[0] == 1 && $r[1] == 2);

$ww-> destroy;

1;
