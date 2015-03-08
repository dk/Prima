use Test::More tests => 16;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

$Prima::Test::dong = 0;
$Prima::Test::w-> bring_to_front;
my @rcrect;
my $ww = $Prima::Test::w-> insert( Widget => origin => [ 0, 0] => size => [ 8, 8],
                                   syncPaint => 0,
                                   buffered => 0,
                                   cursorSize => [ 30, 30],
                                   cursorVisible => 1,
                                   onPaint => sub {
                                       $_[0]-> on_paint( $_[1]);
                                       $Prima::Test::dong = 1;
                                       @rcrect = $_[0]-> clipRect;
                                   });
ok( $Prima::Test::dong || &Prima::Test::wait, "onPaint message" );
$Prima::Test::dong = 0;
$ww-> repaint;
$ww-> update_view;
ok( $Prima::Test::dong, "update_view" );

$Prima::Test::dong = 0;
$ww-> scroll( 2, 2);
$ww-> update_view;
ok( $Prima::Test::dong, "scroll" );

$ww-> invalidate_rect( 0, 0, 2, 2);
my @cr = $ww-> get_invalid_rect;
cmp_ok( $cr[0], '==', 0, "query invalid area" );
cmp_ok( $cr[1], '==', 0, "query invalid area" );
cmp_ok( $cr[2], '==', 2, "query invalid area" );
cmp_ok( $cr[3], '==', 2, "query invalid area" );
$ww-> update_view;
cmp_ok( $rcrect[0], '==', 0, "invalid area consistency" );
cmp_ok( $rcrect[1], '==', 0, "invalid area consistency" );
cmp_ok( $rcrect[2], '==', 1, "invalid area consistency" );
cmp_ok( $rcrect[3], '==', 1, "invalid area consistency" );

$ww-> buffered(1);
$ww-> set( onPaint => sub {
	my $x = $_[1];
	$_[0]-> on_paint( $x);
	$x-> pixel( 0,0,cl::White);
	my $white = $x-> pixel(0,0);
	$x-> pixel( 0,0,cl::Black);
	cmp_ok( $x-> pixel(0,0), '==', 0, "pixel" );
	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> clipRect( 2, 2, 3, 3);
	$x-> bar( 1, 1, 2, 2);
	$x-> clipRect( 0, 0, $x-> size);
	cmp_ok( $x-> pixel( 2,2), '==', 0, "clipRect" );
    cmp_ok( $x-> pixel( 1,1), '==', $white, "clipRect" );

	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> translate( -1, 1);
	$x-> bar( 2, 2, 3, 3);
	$x-> translate( 0, 0);
	cmp_ok( $x-> pixel( 1,4), '==', 0, "translate" );
	cmp_ok( $x-> pixel( 3,2), '==', $white, "translate" );
});
$ww-> repaint;
$ww-> update_view;

$ww-> destroy;

done_testing();
