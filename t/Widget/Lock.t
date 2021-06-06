use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

plan tests => 3;

reset_flag;
my $window = create_window();
$window->lock;
my $c = $window-> insert( Widget => onPaint => \&set_flag );
$c-> update_view;
ok( !get_flag, "child" );

reset_flag;
$c-> repaint;
$window-> unlock;
$c-> update_view;
ok(get_flag, "child unlock" );

reset_flag;
$c-> lock;
$c-> repaint;
$c-> update_view;
ok( !get_flag, "lock consistency" );
$c-> unlock;
$c-> destroy;
