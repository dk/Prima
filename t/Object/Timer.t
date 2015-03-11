use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 4;

my $window = create_window();
my $sub_ref = \&set_flag;
my $t = $window->insert( Timer =>
                   timeout => 20,
                   onTick => $sub_ref);
ok( $t, "create" );
$t-> start;
ok( $t-> get_active, "start" );
ok( &wait, "onTick" );
$t-> owner( $::application );
$t-> owner( $window );
ok( &wait, "recreate" );
$t-> destroy;
