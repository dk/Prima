# $Id$
print "1..10 onMove message - pass 1,correct movement,parameters consistency - pass 1,child move,child move consistency,onMove message - pass 2,parameters consistency - pass 2,gmDontCare,recreate consistency,scroll children\n";

my $dong2 = 0;

my @mrep;
my @mrep2;
my $wx = Prima::Window-> create(
	size => [ 100, 100],
	onMove => sub { $dong = 1; shift; @mrep = scalar(@mrep) ? ( @mrep[0,1], @_[2,3]) : @_;  },
	name => 'TEST',
);


my $wl = $wx-> insert( 'Prima::Widget' =>
	clipOwner => 0,
	growMode  => 0,
	onMove => sub { $dong2 = 1; __end_wait(); shift; @mrep2 = scalar(@mrep2) ? ( @mrep2[0,1], @_[2,3]) : @_;  },
);

my @or = $wx-> origin;
my @or2 = $wl-> origin;
$dong = 0;
$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);

ok(( $dong && $dong2) || &__wait);
my @nor = $wx-> origin;
if ( $nor[0] == $or[0] + 1 && $nor[1] == $or[1] + 1) {
	ok(1);
} elsif ( Prima::Application-> get_system_info->{apc} != apc::Unix) {
	ok(0);
} else {
	skip;
}
ok( $mrep[2] == $nor[0] && $mrep[3] == $nor[1]);
my @d = ( $nor[0] - $or[0], $nor[1] - $or[1]);
ok( scalar(@mrep2));
ok( $mrep2[0] == $or2[0] && $mrep2[1] == $or2[1] && $mrep2[2] == $mrep2[0] + $d[0] && $mrep2[3] == $mrep2[1] + $d[1]);

$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);
$wx-> size( $wx-> width + 1, $wx-> height + 1);

@or = $wx-> origin;
@or2 = $wl-> origin;
@mrep = @mrep2 = ();

$dong = 0;
$wl-> growMode( gm::DontCare);
$wx-> origin( $wx-> left + 2, $wx-> bottom + 2);
ok( $dong || &__wait);
@nor = $wx-> origin;
ok( $mrep[0] == $or[0] && $mrep[1] == $or[1] && $mrep[2] == $nor[0] && $mrep[3] == $nor[1]);
@or = $wl-> origin;
ok( $or[0] == $or2[0] && $or[1] == $or2[1]);

@or = $wl-> origin;
$wl-> owner( $w);
@nor = $wl-> origin;
$wl-> owner( $wx);
ok( $or[0] == $nor[0] && $or[1] == $nor[1]);

$wl-> clipOwner(1);
@or = map { $_ + 10 } $wl-> origin;
$wx->scroll( 10, 10, withChildren => 1); 
@or2 = $wl-> origin;
ok( $or[0] == $or2[0] && $or[1] == $or2[1]);

$wx-> destroy;

1;
