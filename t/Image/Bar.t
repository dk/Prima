use strict;
use warnings;

use Test::More;
use Prima::Test qw(noX11);
use Prima::Application;

plan tests => 1133;

sub bits  { join ':', map { sprintf "%08b", ord } split '', shift }
sub bytes { unpack('H*', shift ) }

sub is_bits
{
	my ( $bits_actual, $bits_expected, $name ) = @_;
	my $ok = $bits_actual eq $bits_expected;
	ok( $ok, $name );
	warn "#   " . bits($bits_actual) . " (actual)\n#   " . bits($bits_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
}

sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   " . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
}

# 1 bit

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
		$i->rop(rop::CopyPut);
		$i-> bar( $offset, 0, $w + $offset - 1, 0 );
		is_bits( $i->data, $vec, "imbpp1/ropCopy $w bits at $offset");

		$vec = "\xaa\xaa\xaa\xaa";
		vec( $vec, $_, 1) = !vec( $vec, $_, 1) for $offset .. $offset + $w - 1;
		$vec = join('', map { chr $revbits[ord $_] } split '', $vec);
		$i->data("\x55\x55\x55\x55");
		$i->rop(rop::Invert);
		$i-> bar( $offset, 0, $w + $offset - 1, 0 );
		is_bits( $i->data, $vec, "imbpp1/ropInvert $w bits at $offset");
	}
}

# 1-bit color 
$i-> set(
	colormap => [ 0xff0000, 0x00ff00 ],
	type     => im::bpp1,
	data     => "\xff\xff\xff\xff",
	rop      => rop::CopyPut,
	color    => 0xff0000,
);
$i->bar(0,0,31,0);
is_bits( $i->data, "\0\0\0\0", "imbpp1 color find");

# 4-bit 
$i->set(
	type     => im::bpp4,
	color    => 0x00ff00,
	width    => 8,
	data     => "\0\0\0\0",
);
$i->bar(0,0,7,0);
is_bits( $i->data, "\x11\x11\x11\x11", "imbpp4 color find");

$i->set(
	type     => im::bpp4 | im::GrayScale,
	color    => 0x808080,
);
$i->bar(0,0,7,0);
is_bits( $i->data, "\x88\x88\x88\x88", "imbpp4 gray color find");

$i->color(cl::White);
for my $w ( 1..7 ) {
	for my $offset ( 0 .. 8 - $w ) {
		my $vec = "\0\0\0\0";
		vec( $vec, $_, 4) = 15 for $offset .. $offset + $w - 1;
		$vec = join('', map { chr $revbits[ord $_] } split '', $vec);
		$i->data("\0\0\0\0");
		$i->rop(rop::CopyPut);
		$i->bar( $offset, 0, $w + $offset - 1, 0 );
		is_bytes( $i->data, $vec, "imbpp4/ropCopy $w nibbles at $offset");

		$vec = "\xaa\xaa\xaa\xaa";
		vec( $vec, $_, 4) = ~vec( $vec, $_, 4) for $offset .. $offset + $w - 1;
		$vec = join('', map { chr $revbits[ord $_] } split '', $vec);
		$i->data("\x55\x55\x55\x55");
		$i->rop(rop::Invert);
		$i-> bar( $offset, 0, $w + $offset - 1, 0 );
		is_bytes( $i->data, $vec, "imbpp4/ropInvert $w nibbles at $offset");
	}
}

# 8-bit
$i->set(
	width    => 4,
	type     => im::bpp8,
	colormap => [0..255],
	color    => 0x12,
	rop      => rop::CopyPut,
);
$i->bar(0,0,7,0);
is_bytes( $i->data, "\x12\x12\x12\x12", "imbpp8 color find");

$i->set(
	type     => im::Byte,
	color    => 0x808080,
);
$i->bar(0,0,3,0);
is_bytes( $i->data, "\x80\x80\x80\x80", "imByte color find");

# rgb

$i->set(
	type     => im::RGB,
	color    => 0x563412,
);
$i->bar(0,0,3,0);
is_bytes( $i->data, "\x12\x34\x56" x 4, "imRGB ropCopy");

$i->color( 0xf0f0f0);
$i->rop( rop::OrPut);
$i->bar(0,0,3,0);
is_bytes( $i->data, "\xf2\xf4\xf6" x 4, "imRGB ropOrPut");

# short, long
$i->set(
	rop      => rop::CopyPut,
	type     => im::Short,
	color    => 1023,
);
$i->bar(0,0,3,0);
is( $i->pixel(0,0), 1023, "imShort");
$i->set(
	type     => im::Long,
	color    => 0x123456,
);
$i->bar(0,0,3,0);
is( $i->pixel(0,0), 0x123456, "imLong");
