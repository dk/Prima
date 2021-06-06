use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 4;

my $t;
$t = Prima::Timer->new(
                   onTick => \&set_flag,
		   timeout => 20,
		   owner => undef,
		   onDestroy => \&set_flag,
		);
ok( $t, "create" );
$t-> start;
ok( $t-> get_active, "start" );
reset_flag;
ok( wait_flag, "onTick" );
reset_flag;
$t->stop;
undef $t;
ok( wait_flag, "destroy by refcnt" );
