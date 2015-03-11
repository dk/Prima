use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 10;

my $dong1 = 0;
my $dong2 = 0;

my @mrep;
my @mrep2;
my $wx = Prima::Window-> create(
	size => [ 100, 100],
	onMove => sub { $dong1 = 1; set_flag; shift; @mrep = scalar(@mrep) ? ( @mrep[0,1], @_[2,3]) : @_;  },
	name => 'TEST',
);


my $wl = $wx-> insert( 'Prima::Widget' =>
	clipOwner => 0,
	growMode  => 0,
	onMove => sub { $dong2 = 1; set_flag; shift; @mrep2 = scalar(@mrep2) ? ( @mrep2[0,1], @_[2,3]) : @_;  },
);

my @or = $wx-> origin;
my @or2 = $wl-> origin;
reset_flag;
$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);

ok(( $dong1 && $dong2) || wait_flag, "onMove message - pass 1" );
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
is_deeply( [$mrep[2],$mrep[3]], \@nor, "parameters consistency - pass 1" );
my @d = ( $nor[0] - $or[0], $nor[1] - $or[1]);
ok( scalar(@mrep2), "child move" );
is_deeply( \@mrep2, [@or2, $mrep2[0] + $d[1], $mrep2[0] + $d[1]], "child move consistency" );

$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);
$wx-> size( $wx-> width + 1, $wx-> height + 1);

@or = $wx-> origin;
@or2 = $wl-> origin;
@mrep = @mrep2 = ();

reset_flag;
$wl-> growMode( gm::DontCare);
$wx-> origin( $wx-> left + 2, $wx-> bottom + 2);
ok( wait_flag, "onMove message - pass 2" );
@nor = $wx-> origin;
is_deeply( \@mrep, [@or,@nor], "parameters consistency - pass 2" );
@or = $wl-> origin;
is_deeply( \@or, \@or2, "gmDontCare" );

@or = $wl-> origin;
$wl-> owner( create_window() );
@nor = $wl-> origin;
$wl-> owner( $wx);
is_deeply( \@or, \@nor, "recreate consistency" );

$wl-> clipOwner(1);
@or = map { $_ + 10 } $wl-> origin;
$wx->scroll( 10, 10, withChildren => 1); 
@or2 = $wl-> origin;
is_deeply( \@or, \@or2, "scroll children" );

$wx-> destroy;
