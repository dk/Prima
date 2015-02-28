# $Id$
print "1..4 gp_init,pixel,forbidden actions,monitor sizes\n";
my $a = $::application;

my @sz = $a-> size;

$a-> begin_paint;
ok( $a-> get_paint_state);
my $pix = $a-> pixel( 10, 10);

$a-> pixel( 10, 10, 0);
my $bl = $a-> pixel( 10, 10);
$a-> pixel( 10, 10, 0xFFFFFF);
my $wh = $a-> pixel( 10, 10);
$a-> pixel( 10, 10, $pix);
my ( $xr, $xg, $xb) = (( $wh & 0xFF0000) >> 16, ( $wh & 0xFF00) >> 8, $wh & 0xFF);
$wh =  ( $xr + $xg + $xb ) / 3;
ok( $bl == 0 && $wh > 200);

$a-> end_paint;

$a-> visible(0);
ok( $a-> visible && $a-> width == $sz[0] && $a-> height == $sz[1]);

my $msz = $a->get_monitor_rects;
ok( $msz && ref($msz) eq 'ARRAY' && @$msz > 0 );

1;
