use strict;
use warnings;

use Test::More tests => 23;

use Prima::Test;
use Prima::Application;

my @w;
my $window = create_window();
$window-> restore;
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
my @wsave = @w;
is( $w[0], 200, "set/get consistency" );
is( $w[1], 200, "set/get consistency" );
$window-> size( 300, 300);
@w = $window-> size;
my $id = $window-> add_notification( Size => sub {
	my ( $self, $oldx, $oldy, $newx, $newy) = @_;
	set_flag(0) if ( $newx == $wsave[0] && $newy == $wsave[1] && $oldx == $w[0] && $oldy == $w[1]);
                                     });
reset_flag();
$window-> size( 200, 200);
ok( get_flag || &wait, "onSize message" );
$window-> remove_notification( $id);
$window-> size( @w);
my @dw = $window-> size;
is( $w[0], $dw[0], "consistency with onSize" );
is( $w[1], $dw[1], "consistency with onSize" );
$window-> visible(0);
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
$window-> visible(1);
is( $w[0], 200, "in hidden state" );
is( $w[1], 200, "in hidden state" );
$window-> minimize;
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
$window-> restore;
is( $w[0], 200, "in minimized state" );
is( $w[1], 200, "in minimized state" );
$window-> maximize;
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
$window-> restore;
is( $w[0], 200, "in maximized state" );
is( $w[1], 200, "in maximized state" );

####################################

my @sz1 = $window-> size;
my $ww = $window-> insert( Widget =>
                           size => [ -2, -2],
                           growMode => gm::Client,
    );
my @a = $ww-> get_virtual_size;
is( $a[0], -2, "virtual size on create set/get consistency" );
is( $a[1], -2, "virtual size on create set/get consistency" );
$window-> size( $window-> width + 50, $window-> height + 50);
my @sz2 = $window-> size;
my @ds = ( $sz2[0] - $sz1[0], $sz2[1] - $sz1[1]);
my @a2 = $ww-> get_virtual_size;
$a2[$_] -= $a[$_] for 0..1;
is( $a2[0], $ds[0], "growMode" );
is( $a2[1], $ds[1], "growMode" );
$window-> size( 200, 200);
@a = $ww-> get_virtual_size;
is( $a[0], -2, "virtual size restore consistency" );
is( $a[1], -2, "virtual size restore consistency" );
my @p = $ww-> origin;
$ww-> owner( $::application );
$ww-> owner( $window );
my @p2 = $ww-> origin;
@a = $ww-> get_virtual_size;
is( $a[0], -2, "reparent check" );
is( $a[1], -2, "reparent check" );
is( $p[0], $p2[0], "reparent check" );
is( $p[1], $p2[1], "reparent check" );
$window-> size( 300, 300);
$window-> size( 200, 200);
@a = $ww-> get_virtual_size;
is( $a[0], -2, "virtual size runtime consistency" );
is( $a[1], -2, "virtual size runtime consistency" );
$ww-> destroy;
