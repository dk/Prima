use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 4;

my $window = create_window;
my $t = $window->insert( Timer =>
                   timeout => 20,
                   onTick => \&set_flag);
ok( $t, "create" );
$t-> start;
ok( $t-> get_active, "start" );
reset_flag;
ok( wait_flag, "onTick" );
$t-> owner( $::application );
$t-> owner( $window );
reset_flag;
ok( wait_flag, "recreate" );
$t-> destroy;
