use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

plan tests => 5;

my $window = create_window;
my $first   = $window-> insert( 'Widget');
my $second  = $window-> insert( 'Widget');

is(( $second-> next || 0), $first, "create" );
is(( $first-> prev || 0), $second,  "create" );

reset_flag;
$second-> set( onZOrderChanged => sub { set_flag(0); });
$second-> insert_behind( $first);

is(( $second-> prev || 0), $first, "runtime" );
is(( $first-> next || 0), $second, "runtime" );
ok( wait_flag, "event" );

$first-> destroy;
$second-> destroy;
