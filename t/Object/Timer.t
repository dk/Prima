use Test::More tests => 4;

use strict;
use warnings;

use Prima::Test;
use Prima::Application;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $t = $Prima::Test::w-> insert( Timer => timeout => 20 => onTick => \&Prima::Test::set_dong);
ok( $t, "create" );
$t-> start;
ok( $t-> get_active, "start" );
ok( &Prima::Test::wait, "onTick" );
$t-> owner( $::application);
$t-> owner( $Prima::Test::w );
ok( &Prima::Test::wait, "recreate" );
$t-> destroy;

done_testing();
