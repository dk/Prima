# $Id$
print "1..2 size,indents";
my @sz = $::application-> size;
ok( $sz[0] > 256 and $sz[1] > 256);

my @i = $::application-> get_indents;
ok(($i[0] + $i[2] < $sz[0]) and ( $i[1] + $i[3] < $sz[1]));

1;

