use Test::More tests => 16;

use strict;
use warnings;

use Prima::Test qw(noX11);
use Prima::Application;

my %id;
my $xw = Prima::Window-> create(
	size => [ 100, 100],
	onActivate    => sub { set_flag(0); $id{Activate}   = 1;},
	onDeactivate  => sub { set_flag(0); $id{Deactivate} = 1;},
	onExecute     => sub { set_flag(0); $id{Execute}    = 1;},
	onWindowState => sub { set_flag(0); $id{State}      = 1;},
	onClose       => sub { set_flag(0); $id{Close}      = 1; $_[0]-> clear_event; },
	onShow        => sub { set_flag(0); $id{Show}       = 1; },
	onHide        => sub { set_flag(0); $id{Hide}       = 1; },
);

my $window = create_window();
$window-> focus;

reset_flag();
$xw-> focus;
ok( $xw-> selected, "activate" );
ok(( get_flag() || &Prima::Test::wait) && $id{Activate}, "onActivate" );
%id=();

reset_flag();
$window-> focus;
ok( !$xw-> selected, "deActivate" );
ok(( get_flag() || &Prima::Test::wait) && $id{Deactivate}, "onDeactivate" );
%id=();

reset_flag();
$xw-> windowState( ws::Maximized);
is( $xw-> windowState, ws::Maximized, "maximize" );
ok(( get_flag() || &Prima::Test::wait) && $id{State}, "onWindowState" );
$xw-> windowState( ws::Normal);
is( $xw-> windowState, ws::Normal, "restore from maximized" );
%id=();

reset_flag();
$xw-> windowState( ws::Minimized);
is( $xw-> windowState, ws::Minimized, "restore from minimized" );

reset_flag();
$xw-> windowState( ws::Normal);
is( $xw-> windowState, ws::Normal, "restore max->min->normal" );

$xw-> windowState( ws::Maximized);
$xw-> windowState( ws::Minimized);
$xw-> windowState( ws::Normal);
is( $xw-> windowState, ws::Normal, "user modality" );

%id=();
reset_flag();
$xw-> insert( Timer =>
              timeout => 250,
              onTick => sub {
                  $_[0]-> stop;
                  $window-> focus;
                  ok( !$window-> selected, "execute" );
                  $xw-> ok;
                  $_[0]-> destroy;
              })-> start;
my $mr = $xw-> execute;
ok( get_flag(), "execute" );
is( $mr, mb::OK, "execute" );

reset_flag();
%id=();
$xw-> show;
ok(( get_flag() || &Prima::Test::wait) && $id{Show} && $xw-> visible, "show" );

reset_flag();
%id=();
$xw-> hide;
ok(( get_flag() || &Prima::Test::wait) && $id{Hide} && !$xw-> visible, "hide" );

%id=();
reset_flag();
$xw-> close;
ok(( get_flag() || &Prima::Test::wait) && $id{Close} && $xw-> alive, "close" );

$xw-> destroy;

done_testing();
