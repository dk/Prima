use Test::More tests => 8;
use Prima::Test qw(noX11);

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $x = Prima::DeviceBitmap-> create( monochrome => 1, width => 8, height => 8);

for ( qw( height width size direction)) {
       my $fx = $x-> font-> $_();
       $x-> font( $_ => $x-> font-> $_() * 3 + 12);
       my $fx2 = $x-> font-> $_();
       if ( $fx2 == $fx) {
           skip;
       } else {
           $x-> font( $_ => $fx);
           is( $fx, $x-> font-> $_(), "$_");
       };
}

my $fx = $x-> font-> pitch;
my $newfx = ( $fx == fp::Fixed) ? fp::Variable : fp::Fixed;
$x-> font( pitch => $newfx);
my $fx2 = $x-> font-> pitch;
$x-> font( pitch => $fx);
is( $x-> font-> pitch, $fx, "pitch");
is( $fx2, $newfx, "pitch");

$fx = $x-> font-> style;
$newfx = ~$fx;
$x-> font( style => $newfx);
$fx2 = $x-> font-> style;
if ( $fx2 == $fx) {
       skip;
} else {
       $x-> font( style => $fx);
       cmp_ok( $fx, '==', $x-> font-> style, "style");
}

$x-> font-> height( 16);
my $w = $x-> width;
cmp_ok( scalar @{$x-> text_wrap( "Ein zwei drei fir funf sechs seben acht neun zehn", $w * 5)}, '>', 4, "text wrap");

$x-> destroy;

done_testing();
