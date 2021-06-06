use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

reset_flag;
my @keydata = ();
my $window = create_window();
my $c = $window-> insert( Widget =>
       onCreate  => \&set_flag,
       onDestroy => \&set_flag,
       onMouseDown  => sub { set_flag; push( @keydata, [@_]); },
       onMouseUp    => sub { set_flag; },
       onMouseMove  => sub { set_flag; },
       onMouseClick => sub { set_flag; push( @keydata, [@_]);},
);

plan skip_all => "can't capture" unless $c->capture(1);
$c->capture(0);

# XP does some random bad magic with mouse movements
# if there's a top-level window on top, or start menu,
# or shutdown prompt ... anyway, we're not testing for robustness here,
# but for proper implementation under normal circumstances
if ( $^O =~ /win32/i) {
	my $info = $::application->get_system_info;
	plan skip_all => "XP does random bad things to me" if $info->{release} < 6;
}

plan tests => 10;

$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok( get_flag && scalar @keydata, "send");
@keydata = ();

my $w;
$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 1);
reset_flag;
$w = wait_flag;
@keydata = grep { scalar @$_ == 5 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok($w && scalar @keydata, "post" );

reset_flag();
$c-> mouse_event( cm::MouseUp, mb::Left, 0, 1, 2, 0, 0);
ok( get_flag, "mouse up" );

reset_flag;
$c-> mouse_event( cm::MouseMove, mb::Left, 0, 1, 2, 0, 0);
ok( get_flag, "mouse move" );

reset_flag;
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 0 } @keydata;
ok( get_flag && scalar @keydata, "click" );

reset_flag;
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 1, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 1 } @keydata;
ok( get_flag && scalar @keydata, "doubleclick" );

my @ppx = $c-> pointerPos;

SKIP: {
	$c-> capture(1) or skip "Can't capture", 1;
	$c-> focus;
	ok( $c-> capture, "capture" );
}

SKIP: {
	$::application->begin_paint;
	skip "rdesktop", 3 if $^O =~ /win32/i && $::application->pixel(0,0) == cl::Invalid;
	$::application->end_paint;

	reset_flag;
	$c-> pointerPos( 10, 10);
	my @pp = $c-> pointerPos;
	is( $pp[0], 10, "positioning" );
	is( $pp[1], 10, "positioning" );
	$c-> pointerPos( 11, 11);
	ok( wait_flag, "simulated movement" );

	$c-> pointerPos( @ppx);
	$c-> capture(0);
}
$c-> destroy;
