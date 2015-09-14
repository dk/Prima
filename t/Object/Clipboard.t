use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 11;
my $c = $::application-> Clipboard;
ok( $c && $c-> alive, "alive");

my %rc = map { $_ => 1 } $c-> get_registered_formats;
ok( exists $rc{'Text'} && exists $rc{'Image'}, "predefined formats" );

SKIP: {
$::application->begin_paint;
skip "rdesktop", 8 if $^O =~ /win32/i && $::application->pixel(0,0) == cl::Invalid;
$::application->end_paint;

skip "Cannot acquire clipboard", 9 unless $c->open;
$c->close;

skip "Cannot acquire clipboard", 9 unless $c-> store( "Text", 'jabba dabba du');
skip "Cannot acquire clipboard", 9 unless $c->open;
my $res = $c-> fetch( 'Text');
my %fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Text} && defined $res, "text exists");
is( $res, 'jabba dabba du', "text is correct" );
$c->close;

my $i = Prima::Image-> create( width => 32, height => 32);
skip "Cannot acquire clipboard", 7 unless $c-> store( "Image", $i);
skip "Cannot acquire clipboard", 7 unless $c->open;
$i = $c-> fetch( 'Image');
%fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Image} && defined $i && $i-> alive, "image exists");
is( $i-> width, 32, "image width ok" );
is( $i-> height, 32, "image height ok" );
$i-> destroy if $i;
$c->close;

ok( $c-> register_format("Mumbo-Jumbo"), "register user-defined format");
%rc = map { $_ => 1 } $c-> get_registered_formats;
skip "Cannot acquire clipboard", 3 unless $c-> store( "Mumbo-Jumbo", pack( 'C*', 0,1,2,3,4,5,6,7,8,9));
skip "Cannot acquire clipboard", 3 unless $c->open;
$res = $c-> fetch( "Mumbo-Jumbo");
%fm = map { $_ => 1 } $c-> get_formats;
ok(exists $rc{"Mumbo-Jumbo"} && exists $fm{"Mumbo-Jumbo"} && defined $res, "exists user-defined format" );
is( $res, pack( 'C*', 0,1,2,3,4,5,6,7,8,9), "user-defined format data ok");
$c-> deregister_format("Mumbo-Jumbo");
$c->close;

$c-> clear;
skip "Cannot acquire clipboard", 1 unless $c->open;
my @f = $c-> get_formats;
is( scalar(@f), 0, "clear");
$c->close;
}
