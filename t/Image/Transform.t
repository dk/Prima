use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 9;

sub bytes { unpack('H*', shift ) }

sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   bytes(" . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
}

my $i = Prima::Image->create(
	width    => 2,
	height   => 4,
	type     => im::Byte,
	data     => "12345678",
	lineSize => 2,
);

my $j = $i->dup;
$j->rotate(270);
is( $j->width, 4, "rotate(270) width ok");
is( $j->height, 2, "rotate(270) height ok");
is($j->data, "75318642", "rotate(270) data ok");

$j = $i->dup;
$j->rotate(90);
is( $j->width, 4, "rotate(90) width ok");
is( $j->height, 2, "rotate(90) height ok");
is($j->data, "24681357", "rotate(90) data ok");

$j->rotate(180);
is( $j->width, 4, "rotate(180) width ok");
is( $j->height, 2, "rotate(180) height ok");
is($j->data, "75318642", "rotate(180) data ok");
