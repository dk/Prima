# $Id$
print "1..3 create,runtime sizeMin,reparent sizeMax\n";

my $ww = $w-> insert( Widget =>
	origin => [ 0, 0],
	sizeMin => [ 10, 10],
	sizeMax => [ 200, 200],
);
$ww-> size( 100, 100);
my @sz = $ww-> size;
ok( $sz[0] >= 10 && $sz[1] >= 10 && $sz[0] <= 200 && $sz[1] <= 200);
$ww-> size( 1, 1);
@sz = $ww-> size;
ok( $sz[0] >= 10 && $sz[1] >= 10 && $sz[0] <= 200 && $sz[1] <= 200);
$ww-> owner( $::application);
$ww-> owner( $w);
$ww-> size( 1000, 1000);
ok( $sz[0] >= 10 && $sz[1] >= 10 && $sz[0] <= 200 && $sz[1] <= 200);

$ww-> destroy;

1;

