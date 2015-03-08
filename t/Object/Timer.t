use strict;
use warnings;

use Test::More tests => 4;
use Prima::Test qw(noX11);
use Prima::Application;


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
