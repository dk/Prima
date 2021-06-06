use strict;
use warnings;

use Test::More;

use Prima::sys::Test;
use Prima::Application;

plan tests => 12;

my @w;
my $window = create_window;
$window-> restore;
$window-> size( 400, 400);
@w = $window-> get_virtual_size;
my @wsave = @w;
is_deeply( \@w, [400,400], "set/get consistency" );
$window-> size( 300, 300);
@w = $window-> size;
my $id = $window-> add_notification( Size => sub {
	my ( $self, $oldx, $oldy, $newx, $newy) = @_;
	set_flag(0) if ( $newx == $wsave[0] && $newy == $wsave[1] && $oldx == $w[0] && $oldy == $w[1]);
});
reset_flag();
$window-> size( 400, 400);
ok( wait_flag, "onSize message" );
$window-> remove_notification( $id);
$window-> size( @w);
my @dw = $window-> size;
is_deeply( \@w, \@dw, "consistency with onSize" );
$window-> visible(0);
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
$window-> visible(1);
is_deeply( \@w, [200,200], "in hidden state" );
$window-> minimize;
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
$window-> restore;
is_deeply( \@w, [200,200], "in minimized state" );
$window-> maximize;
$window-> size( 200, 200);
@w = $window-> get_virtual_size;
$window-> restore;
is_deeply( \@w, [200,200], "in maximized state" );

####################################

my @sz1 = $window-> size;
my $ww = $window-> insert( Widget =>
                           size => [ -2, -2],
                           growMode => gm::Client,
    );
my @a = $ww-> get_virtual_size;
is_deeply( \@a, [-2,-2], "virtual size on create set/get consistency" );
$window-> size( $window-> width + 50, $window-> height + 50);
my @sz2 = $window-> size;
my @ds = ( $sz2[0] - $sz1[0], $sz2[1] - $sz1[1]);
my @a2 = $ww-> get_virtual_size;
$a2[$_] -= $a[$_] for 0..1;
is_deeply( \@a2, \@ds, "growMode" );
$window-> size( 200, 200);
@a = $ww-> get_virtual_size;
is_deeply( \@a, [-2,-2], "virtual size restore consistency" );
my @p = $ww-> origin;
$ww-> owner( $::application );
$ww-> owner( $window );
my @p2 = $ww-> origin;
@a = $ww-> get_virtual_size;
is_deeply( \@a, [-2,-2], "reparent check 1" );
is_deeply( \@p, \@p2, "reparent check 2" );
$window-> size( 300, 300);
$window-> size( 200, 200);
@a = $ww-> get_virtual_size;
is_deeply( \@a, [-2,-2], "virtual size runtime consistency" );
$ww-> destroy;
