use strict;
use warnings;

use Test::More;
use Prima::sys::Test qw(noX11);

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

# patterned ROPs
my @alu = qw(
   Blackness
   NotOr
   NotSrcAnd
   NotPut
   NotDestAnd
   Invert
   XorPut
   NotAnd
   AndPut
   NotXor
   NoOper
   NotSrcOr
   CopyPut
   NotDestOr
   OrPut
   Whiteness
);
my %alunames;
for (my $i = 0; $i < @alu; $i++) { $alunames{$alu[$i]} = $i }

my @ops = (
	sub {0},                # Blackness
	sub {!($_[0]|$_[1])},   # NotOr
	sub {!$_[0] & $_[1]},   # NotSrcAnd
	sub {!$_[0]},           # NotPut
	sub {$_[0] & !$_[1]},   # NotDestAnd
	sub {!$_[1]},           # Invert
	sub {$_[0] ^ $_[1]},    # XorPut
	sub {!($_[0] & $_[1])}, # NotAnd
	sub {$_[0] & $_[1]},    # AndPut
	sub {!($_[0] ^ $_[1])}, # NotXor
	sub {$_[1]},            # NoOper
	sub {!$_[0] | $_[1]},   # NotSrcOr
	sub {$_[0]},            # CopyPut
	sub {$_[0] | !$_[1]},   # NotDestOr
	sub {$_[0] | $_[1]},    # OrPut
	sub {1},         # Whiteness
);

sub alu { $ops[$_[0]]->( $_[1], $_[2] ) }
sub aln { $ops[$alunames{$_[0]}]->( $_[1], $_[2]) }
sub mbits($$) { map { ($_[1] >> $_) & 1 } 0..($_[0]-1)}
sub val {
	my $r = 0;
	for ( my $i = 0; $i < @_; $i++) {
		$r |= ($_[$i] & 1) << $i;
	}
	return $r;
}
sub alx {
	my $n = shift;
	my $nn = ($n < 8) ? 8 : $n;
	my @src = mbits $nn, $_[1];
	my @dst = mbits $nn, $_[2];
	my $val = val( map { aln($_[0], $src[$_], $dst[$_] ) } 0..$#src);
	return ($n > 1) ? $val : (($val >= 0x80) ? 0xff : 0);
}

sub h_is
{
	my ( $act, $exp, $name) = @_;
	if ( $act == $exp ) {
		ok(1, $name);
	} else {
		ok(0, $name);
		diag( sprintf("actual(%0x) != expected(%0x)\n", $act, $exp));
	}
}

for my $bpp ( 1, 4, 8, 24) {
for my $alu ( @alu ) {
	my ($src, $dst, $bk) = (0xcccccc, 0xaaaaaa, 0x333333);
	my $p = Prima::Image->new( type => $bpp | (($bpp < 24) ? im::GrayScale : 0), size => [20,20]);
	my $fun = $rop::{$alu}->();


	my $xbpp = $bpp;
	$xbpp = 8 if $xbpp == 4 || $xbpp == 1;

	$p->color($dst);
	$p->backColor(0);
	$p->rop(rop::CopyPut);
	$p->rop2(rop::CopyPut);
	$p->fillPattern(fp::Solid);
	$p->bar(0,0,$p->size);

	$p->color($src);
	$p->backColor($bk);
	$p->rop($fun);
	$p->fillPattern([(0x55,0xAA)x4]);
	$p->bar(0,0,$p->size);
	h_is($p->pixel(0,0), alx($bpp,$alu,$bk,$dst),  "i$bpp($alu).cpy.0=B");
	h_is($p->pixel(0,1), alx($bpp,$alu,$src,$dst), "i$bpp($alu).cpy.1=F");

	$p->color($dst);
	$p->backColor(0);
	$p->rop(rop::CopyPut);
	$p->rop2(rop::CopyPut);
	$p->fillPattern(fp::Solid);
	$p->bar(0,0,$p->size);

	$p->color($src);
	$p->backColor($bk);
	$p->fillPattern([(0x55,0xAA)x4]);
	$p->rop($fun);
	$p->rop2(rop::NoOper);
	$p->bar(0,0,$p->size);

	my $expected = ($dst & ((1 << $xbpp) - 1));
	$expected = ( $expected >= 0x80 ) ? 0xff : 0 if $bpp == 1;
	h_is($p->pixel(0,0), $expected,  "i$bpp($alu).nop.0=S");
	h_is($p->pixel(0,1), alx($bpp,$alu,$src,$dst), "i$bpp($alu).nop.1=F");
}}

sub test_bar_and_line
{
	my ($i, $clone, $msg) = @_;
	my $bpp = $i->type & im::BPP;
	my $j = $i->clone(%$clone);
	$j->bar(0,0,$i->size);
	$j->type(im::BW);
	is_bytes( $j->data, ("\xff"x4).("\x89\x09\x09\x09").("\xff"x4), "bar $msg, bpp=$bpp");

	$j = $i->clone(%$clone);
	$j->line(0,1,32,1);
	$j->type(im::BW);
	is_bytes( $j->data, ("\xff"x4).("\x89\x09\x09\x09").("\xff"x4), "line $msg, bpp=$bpp");
}

my $r = Prima::Region->new( box => [ 1, 1, 30, 1 ]);
for my $type ( im::BW, 4, im::Byte, 24 ) {
	my $bpp = $type & im::BPP;
	my $i = Prima::Icon->new( size => [32, 3], type => $bpp, conversion => ict::None, autoMasking => am::None);
	$i->clear;
	my %clone = (region => $r, fillPattern => [(0xF6) x 8], linePattern => "\4\1\2\1");
	test_bar_and_line($i, { %clone, rop2 => rop::NoOper }, "patshift/transparent" );
	test_bar_and_line($i, { %clone, rop2 => rop::CopyPut }, "patshift/opaque" );

	$clone{rop} = rop::alpha(rop::SrcCopy, 255);
	test_bar_and_line($i, { %clone, rop2 => rop::NoOper }, "patshift/transparent alpha" );
	test_bar_and_line($i, { %clone, rop2 => rop::CopyPut }, "patshift/opaque alpha" );
}
	
done_testing;
