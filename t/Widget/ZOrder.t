use Test::More tests => 5;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $first   = $Prima::Test::w-> insert( 'Widget');
my $second  = $Prima::Test::w-> insert( 'Widget');

is(( $second-> next || 0), $first, "create" );
is(( $first-> prev || 0), $second,  "create" );

$Prima::Test::dong = 0;
$second-> set( onZOrderChanged => sub { $Prima::Test::dong = 1; });
$second-> insert_behind( $first);

is(( $second-> prev || 0), $first, "runtime" );
is(( $first-> next || 0), $second, "runtime" );
ok( get_flag() || &Prima::Test::wait, "event" );

$first-> destroy;
$second-> destroy;

done_testing();
