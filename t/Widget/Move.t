use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 10;

$Prima::sys::Test::timeout *= 4;

my $dong1 = 0;
my $dong2 = 0;

my @p_rep;
my @c_rep;
my $p = create_window(
	onMove => sub {
		$dong1 = 1;
		set_flag;
		shift;
		@p_rep = scalar(@p_rep) ? ( @p_rep[0,1], @_[2,3]) : @_;
	},
	name => 'TEST',
);


my $c = $p-> insert( 'Prima::Widget' =>
	clipOwner => 0,
	growMode  => 0,
	onMove => sub {
		$dong2 = 1;
		set_flag; shift;
		@c_rep = scalar(@c_rep) ? ( @c_rep[0,1], @_[2,3]) : @_;
	},
);

my @p_or = $p-> origin;
my @c_or = $c-> origin;
reset_flag;
$p-> origin( $p-> left + 1, $p-> bottom + 1);

ok(( $dong1 && $dong2) || wait_flag, "onMove message - pass 1" );
my @nor = $p-> origin;
SKIP : {
    if ( $nor[0] == $p_or[0] + 1 && $nor[1] == $p_or[1] + 1) {
        pass("correct movement");
    } elsif ( Prima::Application-> get_system_info->{apc} != apc::Unix) {
        fail("correct movement");
    } else {
        skip "WM doesn't respect moving request", 1;
    }
};
is_deeply( [$p_rep[2],$p_rep[3]], \@nor, "parameters consistency - pass 1" );
my @d = ( $nor[0] - $p_or[0], $nor[1] - $p_or[1]);
ok( scalar(@c_rep), "c_rep move" );
is_deeply( \@c_rep, [@c_or, $c_rep[0] + $d[0], $c_rep[0] + $d[1]], "child move consistency" );

$p-> origin( $p-> left + 1, $p-> bottom + 1);
$p-> size( $p-> width + 1, $p-> height + 1);

@p_or = $p-> origin;
@c_or = $c-> origin;
@p_rep = @c_rep = ();

reset_flag;
$c-> growMode( gm::DontCare);
$p-> origin( $p-> left + 2, $p-> bottom + 2);
ok( wait_flag, "onMove message - pass 2" );
@nor = $p-> origin;
is_deeply( \@p_rep, [@p_or,@nor], "parameters consistency - pass 2" );
@p_or = $c-> origin;
is_deeply( \@p_or, \@c_or, "gmDontCare" );

@p_or = $c-> origin;
$c-> owner( create_window() );
@nor = $c-> origin;
$c-> owner( $p);
is_deeply( \@p_or, \@nor, "recreate consistency" );

$c-> clipOwner(1);
@p_or = map { $_ + 10 } $c-> origin;
$p->scroll( 10, 10, withChildren => 1);
@c_or = $c-> origin;
is_deeply( \@p_or, \@c_or, "scroll childrren" );

$p-> destroy;
