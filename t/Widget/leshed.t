use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

plan tests => 11;

my $window = create_window;
my $ww = $window-> insert( 'Widget' => origin => [ 10, 10],);

reset_flag();

$ww-> set(
	onEnter   => \&set_flag,
	onLeave   => \&set_flag,
	onShow    => \&set_flag,
	onHide    => \&set_flag,
	onEnable  => \&set_flag,
	onDisable => \&set_flag
);


$ww-> hide;
ok(wait_flag, "hide" );
is($ww-> visible, 0, "hide" );
reset_flag;

$ww-> show;
ok(wait_flag, "show" );
isnt($ww-> visible, 0, "show" );
reset_flag;

$ww-> enabled(0);
ok(wait_flag, "disable" );
is($ww-> enabled, 0, "disable" );
reset_flag;

$ww-> enabled(1);
ok(wait_flag, "enable" );
isnt( $ww-> enabled, 0, "enable" );
reset_flag;

$ww-> focused(1);
SKIP : {
    if ( $ww-> focused) {
       ok(wait_flag, "enter" );
       reset_flag;

       $ww-> focused(0);
       ok( wait_flag, "leave" );
       is( $ww-> focused, 0, "leave" );
       reset_flag;
    } else {
       # WM refuses to refocus
       skip "WM refuses to refocus", 3;
    }
};

$ww-> destroy;
