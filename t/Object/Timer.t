use Test::More tests => 4;

use strict;
use warnings;

use Prima::Test;
use Prima::Application;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $window = create_window();
my $sub_ref = \&Prima::Test::set_flag;
my $t = $window->( Timer =>
                   timeout => 20,
                   onTick => $sub_ref);
ok( $t, "create" );
$t-> start;
ok( $t-> get_active, "start" );
ok( &Prima::Test::wait, "onTick" );
$t-> owner( $::application );
$t-> owner( $window );
ok( &Prima::Test::wait, "recreate" );
$t-> destroy;

done_testing();
