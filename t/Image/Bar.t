use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 527;

sub bits { join ':', map { sprintf "%08b", ord } split '', shift }

sub is_bits
{
	my ( $bits_actual, $bits_expected, $name ) = @_;
	my $ok = $bits_actual eq $bits_expected;
	ok( $ok, $name );
	warn "#   " . bits($bits_actual) . " (actual)\n#   " . bits($bits_expected) . " (expected)\n" unless $ok;
	exit unless $ok;
}

my $i = Prima::Image->create(
	width  => 32,
	height => 1,
	type   => im::BW,
	color  => cl::White,
);

my @revbits;
for ( my $i = 0; $i < 256; $i++) {
	my ($r,$x) = (0,$i);
	for (0..7) {
		$r |= 0x100 if $x & 0x80;
		$x <<= 1;
		$r >>= 1;
	}
	push @revbits, $r;
}

for my $w ( 1..31 ) {
	for my $offset ( 0 .. 32 - $w ) {
		my $vec = "\0\0\0\0";
		vec( $vec, $_, 1) = 1 for $offset .. $offset + $w - 1;
		$vec = join('', map { chr $revbits[ord $_] } split '', $vec);
		$i->data("\0\0\0\0");
		$i-> bar( $offset, 0, $w + $offset - 1, 0 );
		is_bits( $i->data, $vec, "imBW/ropCopy $w bits at $offset");
	}
}
