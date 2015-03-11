use strict;
use warnings;

use Test::More;
use Prima::Test;

plan tests => 10;

reset_flag;
my $window = create_window;
$window-> bring_to_front;
my @rcrect;
my $ww = $window-> insert( Widget => origin => [ 0, 0] => size => [ 8, 8],
                           syncPaint => 0,
                           buffered => 0,
                           cursorSize => [ 30, 30],
                           cursorVisible => 1,
                           onPaint => sub {
                               $_[0]-> on_paint( $_[1]);
                               set_flag;
                               @rcrect = $_[0]-> clipRect;
                           });
ok( wait_flag, "onPaint message" );
reset_flag;
$ww-> repaint;
$ww-> update_view;
ok( get_flag, "update_view" );

reset_flag;
$ww-> scroll( 2, 2);
$ww-> update_view;
ok( get_flag, "scroll" );

$ww-> invalidate_rect( 0, 0, 2, 2);
my @cr = $ww-> get_invalid_rect;
is_deeply( \@cr, [0,0,2,2], "query invalid area" );
$ww-> update_view;
is_deeply( \@rcrect, [0,0,1,1], "invalid area consistency" );

$ww-> buffered(1);
$ww-> set( onPaint => sub {
	my $x = $_[1];
	$_[0]-> on_paint( $x);
	$x-> pixel( 0,0,cl::White);
	my $white = $x-> pixel(0,0);
	$x-> pixel( 0,0,cl::Black);
	is( $x-> pixel(0,0), 0, "pixel" );
	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> clipRect( 2, 2, 3, 3);
	$x-> bar( 1, 1, 2, 2);
	$x-> clipRect( 0, 0, $x-> size);
	is( $x-> pixel( 2,2), 0, "clipRect" );
	is( $x-> pixel( 1,1), $white, "clipRect" );

	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> translate( -1, 1);
	$x-> bar( 2, 2, 3, 3);
	$x-> translate( 0, 0);
	is( $x-> pixel( 1,4), 0, "translate" );
	is( $x-> pixel( 3,2), $white, "translate" );
});
$ww-> repaint;
$ww-> update_view;

$ww-> destroy;
