print "1..3 create,start,onTick\n";

my $t = $w-> insert( Timer => timeout => 20 => onTick => \&__dong);
ok( $t);
$t-> start;
ok( $t-> get_active);
$t-> owner( $::application);
$t-> owner( $w);
ok( &__wait);
$t-> destroy;

1;
