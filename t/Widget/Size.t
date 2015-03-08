use Test::More tests => 23;

use strict;
use warnings;

use Prima::Test;
use Prima::Application;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my @w;
$Prima::Test::w-> restore;
$Prima::Test::w-> size( 200, 200);
@w = $Prima::Test::w-> get_virtual_size;
my @wsave = @w;
cmp_ok( $w[0], '==', 200, "set/get consistency" );
cmp_ok( $w[1], '==', 200, "set/get consistency" );
$Prima::Test::w-> size( 300, 300);
@w = $Prima::Test::w-> size;
my $id = $Prima::Test::w-> add_notification( Size => sub {
	my ( $self, $oldx, $oldy, $newx, $newy) = @_;
	$Prima::Test::dong = 1 if ( $newx == $wsave[0] && $newy == $wsave[1] && $oldx == $w[0] && $oldy == $w[1]);
});
$Prima::Test::dong = 0;
$Prima::Test::w-> size( 200, 200);
ok( $Prima::Test::dong || &Prima::Test::wait, "onSize message" );
$Prima::Test::w-> remove_notification( $id);
$Prima::Test::w-> size( @w);
my @dw = $Prima::Test::w-> size;
cmp_ok( $w[0], '==', $dw[0], "consistency with onSize" );
cmp_ok( $w[1], '==', $dw[1], "consistency with onSize" );
$Prima::Test::w-> visible(0);
$Prima::Test::w-> size( 200, 200);
@w = $Prima::Test::w-> get_virtual_size;
$Prima::Test::w-> visible(1);
cmp_ok( $w[0], '==', 200, "in hidden state" );
cmp_ok( $w[1], '==', 200, "in hidden state" );
$Prima::Test::w-> minimize;
$Prima::Test::w-> size( 200, 200);
@w = $Prima::Test::w-> get_virtual_size;
$Prima::Test::w-> restore;
cmp_ok( $w[0], '==', 200, "in minimized state" );
cmp_ok( $w[1], '==', 200, "in minimized state" );
$Prima::Test::w-> maximize;
$Prima::Test::w-> size( 200, 200);
@w = $Prima::Test::w-> get_virtual_size;
$Prima::Test::w-> restore;
cmp_ok( $w[0], '==', 200, "in maximized state" );
cmp_ok( $w[1], '==', 200, "in maximized state" );

####################################

my @sz1 = $Prima::Test::w-> size;
my $ww = $Prima::Test::w-> insert( Widget =>
                                   size => [ -2, -2],
                                   growMode => gm::Client,
    );
my @a = $ww-> get_virtual_size;
cmp_ok( $a[0], '==', -2, "virtual size on create set/get consistency" );
cmp_ok( $a[1], '==', -2, "virtual size on create set/get consistency" );
$Prima::Test::w-> size( $Prima::Test::w-> width + 50, $Prima::Test::w-> height + 50);
my @sz2 = $Prima::Test::w-> size;
my @ds = ( $sz2[0] - $sz1[0], $sz2[1] - $sz1[1]);
my @a2 = $ww-> get_virtual_size;
$a2[$_] -= $a[$_] for 0..1;
cmp_ok( $a2[0], '==', $ds[0], "growMode" );
cmp_ok( $a2[1], '==', $ds[1], "growMode" );
$Prima::Test::w-> size( 200, 200);
@a = $ww-> get_virtual_size;
cmp_ok( $a[0], '==', -2, "virtual size restore consistency" );
cmp_ok( $a[1], '==', -2, "virtual size restore consistency" );
my @p = $ww-> origin;
$ww-> owner( $::application);
$ww-> owner( $Prima::Test::w);
my @p2 = $ww-> origin;
@a = $ww-> get_virtual_size;
cmp_ok( $a[0], '==', -2, "reparent check" );
cmp_ok( $a[1], '==', -2, "reparent check" );
cmp_ok( $p[0], '==', $p2[0], "reparent check" );
cmp_ok( $p[1], '==', $p2[1], "reparent check" );
$Prima::Test::w-> size( 300, 300);
$Prima::Test::w-> size( 200, 200);
@a = $ww-> get_virtual_size;
cmp_ok( $a[0], '==', -2, "virtual size runtime consistency" );
cmp_ok( $a[1], '==', -2, "virtual size runtime consistency" );
$ww-> destroy;

done_testing()
