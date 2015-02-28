# $Id$
print "1..3 create,runtime,event\n";

my $first   = $w-> insert( 'Widget');
my $second  = $w-> insert( 'Widget');

ok(( $second-> next || 0) == $first && ( $first-> prev || 0) == $second);

$dong = 0;
$second-> set( onZOrderChanged => sub { $dong = 1; });
$second-> insert_behind( $first);

ok(( $second-> prev || 0) == $first && ( $first-> next || 0) == $second);
ok( $dong || &__wait);

$first-> destroy;
$second-> destroy;

1;
