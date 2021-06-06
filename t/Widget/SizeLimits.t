use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 12;

my $window = create_window;
my $ww = $window-> insert( Widget =>
	origin => [ 0, 0],
	sizeMin => [ 10, 10],
	sizeMax => [ 200, 200],
);
$ww-> size( 100, 100);
my @sz = $ww-> size;
cmp_ok( $sz[0], '>=', 10, "create" );
cmp_ok( $sz[1], '>=', 10, "create" );
cmp_ok( $sz[0], '<=', 200, "create" );
cmp_ok( $sz[1], '<=', 200, "create");
$ww-> size( 1, 1);
@sz = $ww-> size;
cmp_ok( $sz[0], '>=', 10, "runtime sizeMin" );
cmp_ok( $sz[1], '>=', 10, "runtime sizeMin" );
cmp_ok( $sz[0], '<=', 200, "runtime sizeMin" );
cmp_ok( $sz[1], '<=', 200, "runtime sizeMin" );
$ww-> owner( $::application);
$ww-> owner( $window );
$ww-> size( 1000, 1000);
cmp_ok( $sz[0], '>=', 10, "reparent sizeMax" );
cmp_ok( $sz[1], '>=', 10, "reparent sizeMax" );
cmp_ok( $sz[0], '<=', 200, "reparent sizeMax" );
cmp_ok( $sz[1], '<=', 200, "reparent sizeMax" );

$ww-> destroy;
