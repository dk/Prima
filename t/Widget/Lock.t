use Test::More tests => 3;
use Prima::Test qw(noX11);

use strict;
use warnings;

reset_flag();
my $window = create_window();
$window->lock;
my $c = $window-> insert( Widget =>
                          onPaint => sub { set_flag(0); }
    );
$c-> update_view;
ok( !get_flag(), "child" );

reset_flag();
$c-> repaint;
$window-> unlock;
$c-> update_view;
ok(get_flag(), "child unlock" );

reset_flag();
$c-> lock;
$c-> repaint;
$c-> update_view;
ok( !get_flag(), "lock consistency" );
$c-> unlock;
$c-> destroy;

done_testing();
