# $Id$
print "1..7 create,runtime,horizontal,vertical,hidden,event,reparent\n";

my $ww = $w-> insert( Widget =>
	origin    => [ 10, 10],
	growMode  => gm::GrowLoX,
);

ok( $ww-> left == 10 && $ww-> bottom == 10);

$ww-> origin( 30, 30);

ok( $ww-> left == 30 && $ww-> bottom == 30);

$w-> size( 200, 200);
my @wp = $w-> size;
$w-> size( 300, 300);
$wp[0] = $w-> width - $wp[0];
$wp[1] = $w-> height- $wp[1];

ok( $ww-> left == 30 + $wp[0] && $ww-> bottom == 30);
$ww-> growMode( gm::GrowLoY);
$w-> size( 200, 200);
ok( $ww-> left == 30 + $wp[0] && $ww-> bottom == 30 - $wp[1]);

$ww-> hide;
$dong = 0;
$ww-> set( onMove => sub { $dong = 1; });
$ww-> origin(10,10);
ok( $ww-> left == 10 && $ww-> bottom == 10);
ok( $dong || &__wait);

$ww-> owner( $::application);
$ww-> owner( $w);
ok( $ww-> left == 10 && $ww-> bottom == 10);

$ww-> destroy;

1;
