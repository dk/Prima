use strict;
use warnings;

use Test::More tests => 4;
use Prima::Test;
use Prima::Application;

my $a = $::application;

my @sz = $a->size;

$a-> begin_paint;
ok( $a-> get_paint_state, "get_paint_state is ok");
my $pix = $a-> pixel( 10, 10);

$a-> pixel( 10, 10, 0);
my $bl = $a-> pixel( 10, 10);
$a-> pixel( 10, 10, 0xFFFFFF);
my $wh = $a-> pixel( 10, 10);
$a-> pixel( 10, 10, $pix);
my ( $xr, $xg, $xb) = (( $wh & 0xFF0000) >> 16, ( $wh & 0xFF00) >> 8, $wh & 0xFF);
$wh =  ( $xr + $xg + $xb ) / 3;
ok( $bl == 0 && $wh > 200, "pixel ok");

$a-> end_paint;

$a-> visible(0);
ok( $a-> visible && $a-> width == $sz[0] && $a-> height == $sz[1], "width and height ok");

my $msz = $a->get_monitor_rects;
ok( $msz && ref($msz) eq 'ARRAY' && @$msz > 0, "monitor configuration ok" );
