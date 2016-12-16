use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 9;

my $a = $::application;

my @sz = $a->size;

$a-> begin_paint;
ok( $a-> get_paint_state, "get_paint_state");

SKIP: {
	skip "xquartz doesn't support this", 2 if $^O eq 'darwin';
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
}

$a-> end_paint;

$a-> visible(0);
ok( $a-> visible && $a-> width == $sz[0] && $a-> height == $sz[1], "width and height");

my $msz = $a->get_monitor_rects;
ok( $msz && ref($msz) eq 'ARRAY' && @$msz > 0, "monitor configuration" );

# test yield
alarm(10);
$::application->yield(0); # clear up accumulated events
my $t = Prima::Timer->new( timeout => 50, onTick => \&set_flag );
$t->start;
my $e = $::application->yield(1);
ok( $e && get_flag, "timer triggers yield return");
$t->stop;

alarm(1);
reset_flag;
$SIG{ALRM} = \&set_flag;
my $p = 0;
$::application->onIdle( sub { $p+=1 } );
$::application->onIdle( sub { $p+=8 } );
$::application->yield(1);
ok( get_flag, "yield without events sleeps, but still is alive");
ok( $p == 9, "idle event"); 

$SIG{ALRM} = 'DEFAULT';
alarm(10);
$::application->close;
$e = $::application->yield(1);
ok(!$e, "yield returns 0 on application.close");
