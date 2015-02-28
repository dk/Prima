# $Id$
print "1..11 set/get consinstency,onSize message,consistency with onSize,in hidden state,in minimized state,in maximized state,".
	"virtual size on create set/get consistency,growMode,virtual size restore consistency,reparent check,virtual size runtime consistency\n";

my @w;
$w-> restore;
$w-> size( 200, 200);
@w = $w-> get_virtual_size;
my @wsave = @w;
ok($w[0] == 200 && $w[1] == 200);
$w-> size( 300, 300);
@w = $w-> size;
my $id = $w-> add_notification( Size => sub {
	my ( $self, $oldx, $oldy, $newx, $newy) = @_;
	$dong = 1 if ( $newx == $wsave[0] && $newy == $wsave[1] && $oldx == $w[0] && $oldy == $w[1]);
});
$dong = 0;
$w-> size( 200, 200);
ok( $dong || &__wait);
$w-> remove_notification( $id);
$w-> size( @w);
my @dw = $w-> size;
ok($w[0] == $dw[0] && $w[1] == $dw[1]);
$w-> visible(0);
$w-> size( 200, 200);
@w = $w-> get_virtual_size;
$w-> visible(1);
ok($w[0] == 200 && $w[1] == 200);
$w-> minimize;
$w-> size( 200, 200);
@w = $w-> get_virtual_size;
$w-> restore;
ok($w[0] == 200 && $w[1] == 200);
$w-> maximize;
$w-> size( 200, 200);
@w = $w-> get_virtual_size;
$w-> restore;
ok($w[0] == 200 && $w[1] == 200);

####################################

my @sz1 = $w-> size;
my $ww = $w-> insert( Widget =>
size => [ -2, -2],
growMode => gm::Client,
);
my @a = $ww-> get_virtual_size;
ok( $a[0] == -2 && $a[1] == -2);
$w-> size( $w-> width + 50, $w-> height + 50);
my @sz2 = $w-> size;
my @ds = ( $sz2[0] - $sz1[0], $sz2[1] - $sz1[1]);
my @a2 = $ww-> get_virtual_size;
$a2[$_] -= $a[$_] for 0..1;
ok( $a2[0] == $ds[0] && $a2[1] == $ds[1]);
$w-> size( 200, 200);
@a = $ww-> get_virtual_size;
ok( $a[0] == -2 && $a[1] == -2);
my @p = $ww-> origin;
$ww-> owner( $::application);
$ww-> owner( $w);
my @p2 = $ww-> origin;
@a = $ww-> get_virtual_size;
ok( $a[0] == -2 && $a[1] == -2 && $p[0] == $p2[0] && $p[1] == $p2[1]);
$w-> size( 300, 300);
$w-> size( 200, 200);
@a = $ww-> get_virtual_size;
ok( $a[0] == -2 && $a[1] == -2);
$ww-> destroy;

1;


