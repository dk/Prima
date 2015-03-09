use strict;
use warnings;

use Test::More tests => 11;
use Prima::Test;

my $window = create_window();
my $ww = $window-> insert( 'Widget' => origin => [ 10, 10],);

reset_flag();

$ww-> set(
	onEnter   => sub { set_flag(0); },
	onLeave   => sub { set_flag(0); },
	onShow    => sub { set_flag(0); },
	onHide    => sub { set_flag(0); },
	onEnable  => sub { set_flag(0); },
	onDisable => sub { set_flag(0); }
);


$ww-> hide;
ok(get_flag || &wait, "hide" );
is($ww-> visible, 0, "hide" );
reset_flag();

$ww-> show;
ok(get_flag || &wait, "show" );
isnt($ww-> visible, 0, "show" );
reset_flag();

$ww-> enabled(0);
ok(get_flag || &wait, "disable" );
is($ww-> enabled, 0, "disable" );
reset_flag();

$ww-> enabled(1);
ok(get_flag || &wait, "enable" );
isnt( $ww-> enabled, 0, "enable" );
reset_flag();

$ww-> focused(1);
SKIP : {
    if ( $ww-> focused) {
       ok(get_flag || &wait, "enter" );
       reset_flag();
	
       $ww-> focused(0);
       ok( get_flag || &wait, "leave" );
       is( $ww-> focused, 0, "leave" );
       reset_flag();
    } else {
       # WM refuses to refocus
       skip "WM refuses to refocus", 3;
    }
};

$ww-> destroy;
