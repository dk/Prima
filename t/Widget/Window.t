use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 16;

my %id;
my $xw = create_window(
	onActivate    => sub { set_flag; $id{Activate}   = 1;},
	onDeactivate  => sub { set_flag; $id{Deactivate} = 1;},
	onExecute     => sub { set_flag; $id{Execute}    = 1;},
	onWindowState => sub { set_flag; $id{State}      = 1;},
	onClose       => sub { set_flag; $id{Close}      = 1; $_[0]-> clear_event; },
	onShow        => sub { set_flag; $id{Show}       = 1; },
	onHide        => sub { set_flag; $id{Hide}       = 1; },
);

# window managers are heavy today
local $Prima::sys::Test::timeout = 5000;

# make sure the other window is focused
my $window = create_window( onActivate => sub {
	set_flag;
	$id{Activate2} = 1;
});
$window-> focus;
reset_flag;
wait_flag unless $id{Activate2};
SKIP: {
	skip "WM doesn't respect focus requests", 4 if !$id{Activate2} &&
		Prima::Application-> get_system_info->{apc} == apc::Unix;

	reset_flag;
	$xw-> focus;
	ok( wait_flag && $id{Activate}, "onActivate" );
	ok( $xw-> selected, "activate" );
	%id=();

	reset_flag;
	$window-> focus;
	ok( !$xw-> selected, "deactivate" );
	ok( wait_flag && $id{Deactivate}, "onDeactivate" );
}

%id=();
reset_flag;
$xw-> windowState( ws::Maximized);
is( $xw-> windowState, ws::Maximized, "maximize" );
ok( wait_flag && $id{State}, "onWindowState" );
$xw-> windowState( ws::Normal);
is( $xw-> windowState, ws::Normal, "restore from maximized" );
%id=();

reset_flag;
$xw-> windowState( ws::Minimized);
is( $xw-> windowState, ws::Minimized, "restore from minimized" );

reset_flag;
$xw-> windowState( ws::Normal);
is( $xw-> windowState, ws::Normal, "restore max->min->normal" );

$xw-> windowState( ws::Maximized);
$xw-> windowState( ws::Minimized);
$xw-> windowState( ws::Normal);
is( $xw-> windowState, ws::Normal, "user modality" );

%id=();
reset_flag;

my $tt = $xw-> insert( Timer =>
	timeout => 250,
	onTick => sub {
		$_[0]-> stop;
		$xw-> ok;
	}
);
$tt-> start;
my $mr = $xw-> execute;
ok( $id{Execute}, "cmExecute");
ok( get_flag, "execute" );
is( $mr, mb::OK, "execute" );

reset_flag;
%id=();
$xw-> show;
ok( wait_flag && $id{Show} && $xw-> visible, "show" );

reset_flag;
%id=();
$xw-> hide;
ok( wait_flag && $id{Hide} && !$xw-> visible, "hide" );

%id=();
reset_flag;
$xw-> close;
ok( wait_flag && $id{Close} && $xw-> alive, "close" );

$xw-> destroy;
