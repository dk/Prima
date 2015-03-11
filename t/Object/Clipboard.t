use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 10;
my $c = $::application-> Clipboard;
ok( $c && $c-> alive, "alive");

my %rc = map { $_ => 1 } $c-> get_registered_formats;
ok( exists $rc{'Text'} && exists $rc{'Image'}, "predefined formats" );

$c-> store( "Text", 'jabba dabba du');
my $res = $c-> fetch( 'Text');
my %fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Text} && defined $res, "text");
is( $res, 'jabba dabba du', "text" );

my $i = Prima::Image-> create( width => 32, height => 32);
$c-> store( "Image", $i);
$i = $c-> fetch( 'Image');
%fm = map { $_ => 1 } $c-> get_formats;
ok( exists $fm{Image} && defined $i && $i-> alive, "image");
is( $i-> width, 32, "image" );
is( $i-> height, 32, "image" );
$i-> destroy if $i;

$c-> register_format("Mumbo-Jumbo");
%rc = map { $_ => 1 } $c-> get_registered_formats;
$c-> store( "Mumbo-Jumbo", pack( 'C*', 0,1,2,3,4,5,6,7,8,9));
$res = $c-> fetch( "Mumbo-Jumbo");
%fm = map { $_ => 1 } $c-> get_formats;
ok(exists $rc{"Mumbo-Jumbo"} && exists $fm{"Mumbo-Jumbo"} && defined $res, "user-defined format" );
is( $res, pack( 'C*', 0,1,2,3,4,5,6,7,8,9), "user-defined format");
$c-> deregister_format("Mumbo-Jumbo");

$c-> clear;
my @f = $c-> get_formats;
is( scalar(@f), 0, "clear");
