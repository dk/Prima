# $Id$
print "1..4 create,start,onTick,recreate\n";

my $t = $w-> insert( Timer => timeout => 20 => onTick => \&__dong);
ok( $t);
$t-> start;
ok( $t-> get_active);
ok( &__wait);
$t-> owner( $::application);
$t-> owner( $w);
ok( &__wait);
$t-> destroy;

1;
