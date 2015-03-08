use Test::More tests => 3;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

$Prima::Test::dong = 0;
$Prima::Test::w->lock;
my $c = $Prima::Test::w-> insert( Widget =>
	onPaint => sub { $Prima::Test::dong = 1; }
);
$c-> update_view;
ok( !$Prima::Test::dong, "child" );

$dong = 0;
$c-> repaint;
$Prima::Test::w-> unlock;
$c-> update_view;
ok($Prima::Test::dong, "child unlock" );

$Prima::Test::dong = 0;
$c-> lock;
$c-> repaint;
$c-> update_view;
ok( !$Prima::Test::dong, "lock consistency" );
$c-> unlock;
$c-> destroy;

done_testing();
