use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

plan tests => 8;

reset_flag();
my @xpm = (0,0);
my $sub_ref = \&set_flag;

my $window = create_window;
my $c = $window-> insert( Component =>
                          onCreate  => $sub_ref,
                          onDestroy => $sub_ref,
                          onPostMessage => sub { set_flag; @xpm = ($_[1],$_[2])},
                          name => 'gumbo jumbo',
    );
# 1
ok($c, "create" );
ok(get_flag, "onCreate" );
is($c-> name, 'gumbo jumbo', "name" );
$c-> post_message("abcd", [1..200]);
$c-> owner( $::application);
$c-> owner( $window );
# 4
reset_flag;
ok(wait_flag, "onPostMessage" );
is($xpm[0], 'abcd', "onPostMessage" );
is( @{$xpm[1]}, 200, "onPostMessage" );
reset_flag;
$c-> destroy;
ok(get_flag, "onDestroy" );
# 7
reset_flag;
Prima::Drawable-> create( onDestroy => \&set_flag );
ok( get_flag, "garbage collection" );
