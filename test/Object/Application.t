print "1..3 gp_init,pixel,forbidden actions\n";
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
ok( $bl == 0 && $wh == 0xFFFFFF);

$a-> end_paint;

$a-> visible(0);
ok( $a-> visible && $a-> width == $sz[0] && $a-> height == $sz[1]);

1;
