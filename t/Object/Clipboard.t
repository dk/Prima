use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 11;
my $c = $::application-> Clipboard;
ok( $c && $c-> alive, "alive");

my %rc = map { $_ => 1 } $c-> get_registered_formats;
ok( exists $rc{'Text'} && exists $rc{'Image'}, "predefined formats" );

SKIP: {
$::application->begin_paint;
skip "rdesktop", 9 if $^O =~ /win32/i && $::application->pixel(0,0) == cl::Invalid;
$::application->end_paint;

sub try(&$)
{
	my $sub  = shift;
	my $skip = shift;
	for ( 1..2 ) {
		return 1 if $sub->();
		sleep(1);
	}
	my $msg = "Cannot acquire clipboard: " . (Prima::Utils::last_error() // 'unknown error');
	skip $msg, $skip;
}

try { $c->open } 9;
$c->close;

try { $c-> store( "Text", 'jabba dabba du') } 9;
try { $c->open } 9;
my $res = $c-> fetch( 'Text');
unless ( defined $res ) {
	my $msg = "Cannot fetch data from clipboard: " . (Prima::Utils::last_error() // 'unknown error');
	skip $msg, 9;
}
my %fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Text} && defined $res, "text exists");
is( $res, 'jabba dabba du', "text is correct" );
$c->close;

my $i = Prima::Image-> create( width => 32, height => 32);
try { $c-> store( "Image", $i) } 7;
try { $c->open } 7;
$i = $c-> fetch( 'Image');
%fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Image} && defined $i && $i-> alive, "image exists");
skip "no image", 6 unless $i;
is( $i-> width, 32, "image width ok" );
is( $i-> height, 32, "image height ok" );
$i-> destroy if $i;
$c->close;

SKIP: {
if ( $^O =~ /win32/i) {
	my $info = $::application->get_system_info;
	skip "XP does bad things to me", 3 if $info->{release} < 6;
}
ok( $c-> register_format("Mumbo-Jumbo"), "register user-defined format");
%rc = map { $_ => 1 } $c-> get_registered_formats;
try { $c-> store( "Mumbo-Jumbo", pack( 'C*', 0,1,2,3,4,5,6,7,8,9)) } 3;
try { $c->open } 3;
$res = $c-> fetch( "Mumbo-Jumbo");
%fm = map { $_ => 1 } $c-> get_formats;
ok(exists $rc{"Mumbo-Jumbo"} && exists $fm{"Mumbo-Jumbo"} && defined $res, "exists user-defined format" );
is( $res, pack( 'C*', 0,1,2,3,4,5,6,7,8,9), "user-defined format data ok");
$c-> deregister_format("Mumbo-Jumbo");
$c->close;
}

$c-> clear;
try { $c->open } 1;
my @f = $c-> get_formats;
is( scalar(@f), 0, "clear");
$c->close;
}
