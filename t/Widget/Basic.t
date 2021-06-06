use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 5;

my $sub_ref = \&set_flag;
reset_flag;
my $window = create_window;
my @xpm = (0,0);
my $c = $window-> insert( Widget =>
                     onCreate => $sub_ref,
                     onDestroy => $sub_ref,
                     onPostMessage => sub { set_flag; @xpm = ($_[1],$_[2])}
    );
ok($c, "create" );
ok(get_flag, "onCreate" );
$c-> post_message("abcd", [1..200]);
$c-> owner( $::application);
$c-> owner( $window );
reset_flag;
ok(wait_flag, "onPostMessage" );
ok($xpm[0] eq 'abcd' && @{$xpm[1]} == 200, , "onPostMessage" );
reset_flag;
$c-> destroy;
ok(get_flag, "onDestroy" );
