use Test::More tests => 10;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

$Prima::Test::dong = 0;
my @keydata = ();
my $c = $Prima::Test::w-> insert( Widget =>
	onCreate  => \&Prima::Test::set_dong,
	onDestroy => \&Prima::Test::set_dong,
	onMouseDown  => sub { $Prima::Test::dong = 1; push( @keydata, [@_]); },
	onMouseUp    => sub { $Prima::Test::dong = 1; },
	onMouseMove  => sub { $Prima::Test::dong = 1; },
	onMouseClick => sub { $Prima::Test::dong = 1; push( @keydata, [@_]);},
);

$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok( $Prima::Test::dong && scalar @keydata, "send");
@keydata = ();

my $w;
$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 1);
$w = &Prima::Test::wait;
@keydata = grep { scalar @$_ == 5 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok($w && scalar @keydata, "post" );

$Prima::Test::dong = 0;
$c-> mouse_event( cm::MouseUp, mb::Left, 0, 1, 2, 0, 0);
ok( $Prima::Test::dong, "mouse up" );

$Prima::Test::dong = 0;
$c-> mouse_event( cm::MouseMove, mb::Left, 0, 1, 2, 0, 0);
ok( $Prima::Test::dong, "mouse move" );

$Prima::Test::dong = 0;
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 0 } @keydata;
ok( $Prima::Test::dong && scalar @keydata, "click" );

$Prima::Test::dong = 0;
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 1, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 1 } @keydata;
ok( $Prima::Test::dong && scalar @keydata, "doubleclick" );


my @ppx = $c-> pointerPos;
$c-> capture(1);
$c-> focus;
ok( $c-> capture, "capture" );

$Prima::Test::dong = 0;
$c-> pointerPos( 10, 10);
my @pp = $c-> pointerPos;
is( $pp[0], 10, "positioning" );
is( $pp[1], 10, "positioning" );
$c-> pointerPos( 11, 11);
ok( $Prima::Test::dong || &Prima::Test::__wait, "simulated movement" );

$c-> pointerPos( @ppx);
$c-> capture(0);
$c-> destroy;

done_testing();
