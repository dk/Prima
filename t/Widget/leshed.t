use Test::More tests => 11;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $ww = $Prima::Test::w-> insert( 'Widget' => origin => [ 10, 10],);

$Prima::Test::dong = 0;

$ww-> set(
	onEnter   => sub { $Prima::Test::dong = 1; },
	onLeave   => sub { $Prima::Test::dong = 1; },
	onShow    => sub { $Prima::Test::dong = 1; },
	onHide    => sub { $Prima::Test::dong = 1; },
	onEnable  => sub { $Prima::Test::dong = 1; },
	onDisable => sub { $Prima::Test::dong = 1; }
);


$ww-> hide;
ok(get_flag() || &Prima::Test::wait, "hide" );
is($ww-> visible, 0, "hide" );
reset_flag();

$ww-> show;
ok(get_flag() || &Prima::Test::wait, "show" );
isnt($ww-> visible, 0, "show" );
reset_flag();

$ww-> enabled(0);
ok(get_flag() || &Prima::Test::wait, "disable" );
is($ww-> enabled, 0, "disable" );
reset_flag();

$ww-> enabled(1);
ok(get_flag() || &Prima::Test::wait, "enable" );
isnt( $ww-> enabled, 0, "enable" );
reset_flag();

$ww-> focused(1);
if ( $ww-> focused) {
	ok($Prima::Test::dong || &Prima::Test::wait, "enter" );
	$Prima::Test::dong = 0;
	
	$ww-> focused(0);
	ok( $Prima::Test::dong || &Prima::Test::wait, "leave" );
    cmp_ok( $ww-> focused, '==', 0, "leave" );
	$Prima::Test::dong = 0;
} else {
	# WM refuses to refocus
	skip;
}

$ww-> destroy;

done_testing();
