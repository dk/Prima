# $Id$
print "1..8 onPaint message,update_view,scroll,query invalid area,invalid area consistency,pixel,clipRect,translate\n";

$dong = 0;
$w-> bring_to_front;
my @rcrect;
my $ww = $w-> insert( Widget => origin => [ 0, 0] => size => [ 8, 8],
syncPaint => 0,
buffered => 0,
cursorSize => [ 30, 30],
cursorVisible => 1,
onPaint => sub {
	$_[0]-> on_paint( $_[1]);
	$dong = 1;
	@rcrect = $_[0]-> clipRect;
});
ok( $dong || &__wait);
$dong = 0;
$ww-> repaint;
$ww-> update_view;
ok( $dong);

$dong = 0;
$ww-> scroll( 2, 2);
$ww-> update_view;
ok( $dong );

$ww-> invalidate_rect( 0, 0, 2, 2);
my @cr = $ww-> get_invalid_rect;
ok( $cr[0] == 0 && $cr[1] == 0 && $cr[2] == 2 && $cr[3] == 2);
$ww-> update_view;
ok( $rcrect[0] == 0 && $rcrect[1] == 0 && $rcrect[2] == 1 && $rcrect[3] == 1);

$ww-> buffered(1);
$ww-> set( onPaint => sub {
	my $x = $_[1];
	$_[0]-> on_paint( $x);
	$x-> pixel( 0,0,cl::White);
	my $white = $x-> pixel(0,0);
	$x-> pixel( 0,0,cl::Black);
	ok( $x-> pixel(0,0) == 0);
	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> clipRect( 2, 2, 3, 3);
	$x-> bar( 1, 1, 2, 2);
	$x-> clipRect( 0, 0, $x-> size);
	ok( $x-> pixel( 2,2) == 0 && $x-> pixel( 1,1) == $white);

	$x-> color( cl::White);
	$x-> bar( 0, 0, 7, 7);
	$x-> color( cl::Black);
	$x-> translate( -1, 1);
	$x-> bar( 2, 2, 3, 3);
	$x-> translate( 0, 0);
	ok( $x-> pixel( 1,4) == 0 &&
		$x-> pixel( 3,2) == $white
	);
});
$ww-> repaint;
$ww-> update_view;

$ww-> destroy;

1;

