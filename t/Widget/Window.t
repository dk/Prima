use Test::More;
use Prima::Test qw(noX11 wait dong w);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my %id;
my $xw = Prima::Window-> create(
	size => [ 100, 100],
	onActivate    => sub { $Prima::Test::dong = 1; $id{Activate}   = 1;},
	onDeactivate  => sub { $Prima::Test::dong = 1; $id{Deactivate} = 1;},
	onExecute     => sub { $Prima::Test::dong = 1; $id{Execute}    = 1;},
	onWindowState => sub { $Prima::Test::dong = 1; $id{State}      = 1;},
	onClose       => sub { $Prima::Test::dong = 1; $id{Close}      = 1; $_[0]-> clear_event; },
	onShow        => sub { $Prima::Test::dong = 1; $id{Show}       = 1; },
	onHide        => sub { $Prima::Test::dong = 1; $id{Hide}       = 1; },
);

$Prima::Test::w-> focus;

$Prima::Test::dong = 0;
$xw-> focus;
ok( $xw-> selected, "activate" );
ok(( $Prima::Test::dong || &Prima::Test::wait) && $id{Activate}, "onActivate" );
%id=();

$Prima::Test::dong = 0;
$Prima::Test::w-> focus;
ok( !$xw-> selected, "deActivate" );
ok(( $Prima::Test::dong || &Prima::Test::wait) && $id{Deactivate}, "onDeactivate" );
%id=();

$Prima::Test::dong = 0;
$xw-> windowState( ws::Maximized);
cmp_ok( $xw-> windowState, '==', ws::Maximized, "maximize" );
ok(( $Prima::Test::dong || &Prima::Test::wait) && $id{State}, "onWindowState" );
$xw-> windowState( ws::Normal);
cmp_ok( $xw-> windowState, '==', ws::Normal, "restore from maximized" );
%id=();

$Prima::Test::dong = 0;
$xw-> windowState( ws::Minimized);
cmp_ok( $xw-> windowState, '==', ws::Minimized, "restore from minimized" );

$Prima::Test::dong = 0;
$xw-> windowState( ws::Normal);
cmp_ok( $xw-> windowState, '==', ws::Normal, "restore max->min->normal" );

$xw-> windowState( ws::Maximized);
$xw-> windowState( ws::Minimized);
$xw-> windowState( ws::Normal);
cmp_ok( $xw-> windowState, '==', ws::Normal, "user modality" );

%id=();
$Prima::Test::dong = 0;
$xw-> insert( Timer =>
              timeout => 250,
              onTick => sub {
                  $_[0]-> stop;
                  $Prima::Test::w-> focus;
                  ok( !$Prima::Test::w-> selected, "execute" );
                  $xw-> ok;
                  $_[0]-> destroy;
              })-> start;
my $mr = $xw-> execute;
ok( $Prima::Test::dong, "execute" );
cmp_ok( $mr, "==", mb::OK, "execute" );

$Prima::Test::dong = 0;
%id=();
$xw-> show;
ok(( $Prima::Test::dong || &Prima::Test::wait) && $id{Show} && $xw-> visible, "show" );

$Prima::Test::dong = 0;
%id=();
$xw-> hide;
ok(( $Prima::Test::dong || &Prima::Test::wait) && $id{Hide} && !$xw-> visible, "hide" );

%id=();
$Prima::Test::dong = 0;
$xw-> close;
ok(( $Prima::Test::dong || &Prima::Test::wait) && $id{Close} && $xw-> alive, "close" );

$xw-> destroy;

done_testing();
