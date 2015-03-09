use strict;
use warnings;

use Test::More tests => 20;

use Prima::Test;
use Prima::Application;

my $dong2 = 0;

my @mrep;
my @mrep2;
my $wx = Prima::Window-> create(
	size => [ 100, 100],
	onMove => sub { set_flag(0); shift; @mrep = scalar(@mrep) ? ( @mrep[0,1], @_[2,3]) : @_;  },
	name => 'TEST',
);


my $wl = $wx-> insert( 'Prima::Widget' =>
	clipOwner => 0,
	growMode  => 0,
	onMove => sub { $dong2 = 1; set_flag(1); shift; @mrep2 = scalar(@mrep2) ? ( @mrep2[0,1], @_[2,3]) : @_;  },
);

my @or = $wx-> origin;
my @or2 = $wl-> origin;
reset_flag();
$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);

ok(( get_flag && $dong2) || &wait, "onMove message - pass 1" );
my @nor = $wx-> origin;
SKIP : {
    if ( $nor[0] == $or[0] + 1 && $nor[1] == $or[1] + 1) {
        pass("correct movement");
    } elsif ( Prima::Application-> get_system_info->{apc} != apc::Unix) {
        fail("correct movement");
    } else {
        skip "skipping movement", 1;
    }
};
is( $mrep[2], $nor[0], "parameters consistency - pass 1" );
is( $mrep[3], $nor[1], "parameters consistency - pass 1" );
my @d = ( $nor[0] - $or[0], $nor[1] - $or[1]);
ok( scalar(@mrep2), "child move" );
is( $mrep2[0], $or2[0], "child move consistency" );
is( $mrep2[1], $or2[1], "child move consistency" );
is( $mrep2[2], $mrep2[0] + $d[0], "child move consistency" );
is( $mrep2[3], $mrep2[1] + $d[1], "child move consistency" );

$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);
$wx-> size( $wx-> width + 1, $wx-> height + 1);

@or = $wx-> origin;
@or2 = $wl-> origin;
@mrep = @mrep2 = ();

reset_flag();
$wl-> growMode( gm::DontCare);
$wx-> origin( $wx-> left + 2, $wx-> bottom + 2);
ok( get_flag || &wait, "onMove message - pass 2" );
@nor = $wx-> origin;
is( $mrep[0], $or[0], "parameters consistency - pass 2" );
is( $mrep[1], $or[1], "parameters consistency - pass 2" );
is( $mrep[2], $nor[0], "parameters consistency - pass 2" );
is( $mrep[3], $nor[1], "parameters consistency - pass 2" );
@or = $wl-> origin;
is( $or[0], $or2[0], "gmDontCare" );
is( $or[1], $or2[1], "gmDontCare" );

@or = $wl-> origin;
$wl-> owner( create_window() );
@nor = $wl-> origin;
$wl-> owner( $wx);
is( $or[0], $nor[0], "recreate consistency" );
is( $or[1], $nor[1], "recreate consistency" );

$wl-> clipOwner(1);
@or = map { $_ + 10 } $wl-> origin;
$wx->scroll( 10, 10, withChildren => 1); 
@or2 = $wl-> origin;
is( $or[0], $or2[0], "scroll children" );
is( $or[1], $or2[1], "scroll children" );

$wx-> destroy;
