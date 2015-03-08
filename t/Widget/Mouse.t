use strict;
use warnings;

use Test::More tests => 10;
use Prima::Test qw(noX11);

reset_flag();
my @keydata = ();
my $window = create_window();
my $c = $window-> insert( Widget =>
       onCreate  => \&set_flag,
       onDestroy => \&set_flag,
       onMouseDown  => sub { set_flag(0); push( @keydata, [@_]); },
       onMouseUp    => sub { set_flag(0); },
       onMouseMove  => sub { set_flag(0); },
       onMouseClick => sub { set_flag(0); push( @keydata, [@_]);},
);

$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok( get_flag && scalar @keydata, "send");
@keydata = ();

my $w;
$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 1);
$w = &wait;
@keydata = grep { scalar @$_ == 5 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok($w && scalar @keydata, "post" );

reset_flag();
$c-> mouse_event( cm::MouseUp, mb::Left, 0, 1, 2, 0, 0);
ok( get_flag, "mouse up" );

reset_flag();
$c-> mouse_event( cm::MouseMove, mb::Left, 0, 1, 2, 0, 0);
ok( get_flag, "mouse move" );

reset_flag();
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 0 } @keydata;
ok( get_flag && scalar @keydata, "click" );

reset_flag();
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 1, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 1 } @keydata;
ok( get_flag && scalar @keydata, "doubleclick" );


my @ppx = $c-> pointerPos;
$c-> capture(1);
$c-> focus;
ok( $c-> capture, "capture" );

reset_flag();
$c-> pointerPos( 10, 10);
my @pp = $c-> pointerPos;
is( $pp[0], 10, "positioning" );
is( $pp[1], 10, "positioning" );
$c-> pointerPos( 11, 11);
ok( get_flag || &wait, "simulated movement" );

$c-> pointerPos( @ppx);
$c-> capture(0);
$c-> destroy;
