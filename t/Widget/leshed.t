# $Id$
# Leave, Enter, Show, Hide, Enable, Disable
print "1..6 hide,show,disable,enable,enter,leave\n";

my $ww = $w-> insert( 'Widget' => origin => [ 10, 10],);

$dong = 0;

$ww-> set(
	onEnter   => sub { $dong = 1; },
	onLeave   => sub { $dong = 1; },
	onShow    => sub { $dong = 1; },
	onHide    => sub { $dong = 1; },
	onEnable  => sub { $dong = 1; },
	onDisable => sub { $dong = 1; }
);


$ww-> hide;
ok(($dong || &__wait) && ($ww-> visible == 0));
$dong = 0;

$ww-> show;
ok(($dong || &__wait) && ($ww-> visible != 0));
$dong = 0;

$ww-> enabled(0);
ok(($dong || &__wait) && ($ww-> enabled == 0));
$dong = 0;

$ww-> enabled(1);
ok(($dong || &__wait) && ($ww-> enabled != 0));
$dong = 0;

$ww-> focused(1);
if ( $ww-> focused) {
	ok(($dong || &__wait));
	$dong = 0;
	
	$ww-> focused(0);
	ok(($dong || &__wait) && ($ww-> focused == 0));
	$dong = 0;
} else {
	# WM refuses to refocus
	skip;
	skip;
}

$ww-> destroy;


1;


