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
ok($Prima::Test::dong || &Prima::Test::wait, "hide" );
cmp_ok($ww-> visible, '==', 0, "hide" );
$Prima::Test::dong = 0;

$ww-> show;
ok($Prima::Test::dong || &Prima::Test::wait, "show" );
cmp_ok($ww-> visible, '!=', 0, "show" );
$Prima::Test::dong = 0;

$ww-> enabled(0);
ok($Prima::Test::dong || &Prima::Test::wait, "disable" );
cmp_ok($ww-> enabled, '==', 0, "disable" );
$Prima::Test::dong = 0;

$ww-> enabled(1);
ok($Prima::Test::dong || &Prima::Test::wait, "enable" );
cmp_ok( $ww-> enabled, '!=', 0, "enable" );
$Prima::Test::dong = 0;

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
