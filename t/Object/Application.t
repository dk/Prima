use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 12;

my $a = $::application;

my @sz = $a->size;

# Test screen grabbing
SKIP: {
	if ($^O eq 'darwin') {
		skip "not compiled with cocoa", 3 unless $a->get_system_info->{guiDescription} != /bCocoa/;
	} elsif ( ($ENV{XDG_SESSION_TYPE} // 'x11') ne 'x11') {
		skip "not compiled with gtk", 3 unless $a->get_system_info->{gui} == gui::GTK;
	}

	my $w = $a->insert(Widget =>
		rect => [0,0,5,5],
		color => cl::White,
		backColor => cl::Black,
		fillPattern => fp::SimpleDots,
	);
	$w->show;
	$w->bring_to_front;

	my $i = $a->get_image(1,1,2,1);
	ok( $i && $i->width == 2 && $i->height == 1, "some bitmap grabbing succeeded");
	skip "no bitmap", 1 unless $i;
	$i->type(im::BW);
	my ( $a, $b ) = ( $i->pixel(0,0), $i->pixel(1,0) );
	($a,$b) = ($b,$a) if $b < $a;
	is($a, 0, "one pixel is black");
	is($b, 255, "another is white");
}



SKIP: {
	skip "system doesn't allow direct access to screen", 3 unless $a-> begin_paint;
	ok( $a-> get_paint_state, "get_paint_state");
	my $pix = $a-> pixel( 10, 10);
	skip "rdesktop", 2 if $^O =~ /win32/i && $pix == cl::Invalid;

	$a-> pixel( 10, 10, 0);
	my $bl = $a-> pixel( 10, 10);
	$a-> pixel( 10, 10, 0xFFFFFF);
	my $wh = $a-> pixel( 10, 10);
	$a-> pixel( 10, 10, $pix);
	my ( $xr, $xg, $xb) = (( $wh & 0xFF0000) >> 16, ( $wh & 0xFF00) >> 8, $wh & 0xFF);
	$wh =  ( $xr + $xg + $xb ) / 3;
	is( $bl, 0, "black pixel");
	cmp_ok( $wh, '>', 200, "white pixel");
	$a-> end_paint;
}


$a-> visible(0);
ok( $a-> visible && $a-> width == $sz[0] && $a-> height == $sz[1], "width and height");

my $msz = $a->get_monitor_rects;
ok( $msz && ref($msz) eq 'ARRAY' && @$msz > 0, "monitor configuration" );

# test yield
alarm(10);
$::application->yield(0); # clear up accumulated events
my $t = Prima::Timer->new( timeout => 50, onTick => \&set_flag );
$t->start;
my $e = 1;
$e &= $::application->yield(1) while !get_flag;
ok( $e && get_flag, "timer triggers yield return");
$t->stop;

reset_flag;
$SIG{ALRM} = \&set_flag;
alarm(1);
my $p = 0;
$::application->onIdle( sub { $p|=1 } );
$::application->onIdle( sub { $p|=2 } );
my $time = time + 2;
while ( 1) {
	$::application->yield(1);
	last if $time < time or get_flag;
}
ok( get_flag, "yield without events sleeps, but still is alive");
ok( $p == 3, "idle event");

$SIG{ALRM} = 'DEFAULT';
alarm(10);
$::application->close;
$e = $::application->yield(1);
ok(!$e, "yield returns 0 on application.close");
