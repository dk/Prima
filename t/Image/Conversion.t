use strict;
use warnings;

use Test::More;
use Prima::sys::Test qw(noX11);

my @types = (
	['bpp1', im::Mono],
	['bpp1 gray', im::BW],
	['bpp4', im::bpp4],
	['bpp4 gray', im::bpp4|im::GrayScale],
	['bpp8', im::bpp8],
	['bpp8 gray', im::Byte],
	['rgb', im::RGB],
	['int16', im::Short],
	['int32', im::Long],
	['float', im::Float],
	['double', im::Double],
);

my @filters;
for ( sort keys %ict:: ) {
	next if /^(AUTOLOAD|Constant)$/i;
	push @filters, [ $_, &{$ict::{$_}}() ];
}

sub bytes { unpack('H*', shift ) }
sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   " . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
}

my $i = Prima::Image->create(
	width  => 2,
	height => 2,
	type   => im::Byte,
	data   => "\x{00}\x{ff}**\x{ff}\x{00}**",
);
$i-> size( 32, 32 );

# generic conversion
for ( @types ) {
	my ( $typename, $type ) = @$_;
	my $j = $i-> clone( type => $type, conversion => ict::None);
	for ( @types ) {
		my ( $typename2, $type2 ) = @$_;
		for ( @filters ) {
			my ( $filtername, $filter ) = @$_;
			my $k = $j-> clone( type => $type2, conversion => $filter);
			$k-> set( type => im::Byte, conversion => ict::None);
			is_bytes( $i->data, $k-> data, "$typename -> $typename2 $filtername");

			$k = $j-> clone( type => $type2, conversion => $filter, palette => 2); # will be reduced automatically
			$k-> set( type => im::Byte, conversion => ict::None);
			is_bytes( $i->data, $k-> data, "$typename -> $typename2 $filtername with reduced palette colors");

			$k = $j-> clone( type => $type2, conversion => $filter, palette => [0,0,0,255,255,255]); # will be reduced automatically
			$k-> set( type => im::Byte, conversion => ict::None);
			is_bytes( $i->data, $k-> data, "$typename -> $typename2 $filtername with predefined palette");
		}
	}
}

$i = Prima::Image->create(
	width   => 4,
	height  => 4,
	type    => im::bpp8,
	data    => "\0\1\2\3\4\5\6\7\x{8}\x{9}\x{a}\x{b}\x{c}\x{d}\x{e}\x{f}",
	colormap => [ map { $_*17 } 0..15 ],
);

# check palette stability during color reduction
for ( @types ) {
	my ( $typename, $type ) = @$_;
	is_deeply( [$i->clone(type => $type)-> clone( type => im::BW )->colormap], [0, 0xffffff], "color $typename->BW");
	if ( $type & im::GrayScale ) {
		is_deeply( [$i->clone(type => $type)-> clone( type => im::Mono )->colormap], [0, 0xffffff], "color $typename->mono is gray");
		if (( $type & im::BPP ) > 1 ) {
			my @cm = $i->clone(type => $type)-> clone( type => im::bpp4, palette => 2 )->colormap;
			# 0xff/3(blue)=0x55, but 0x55+/-0x11 is good too, color tree is unstable on unexact matches as palette size changes
			$cm[1] = 0x555555 if $cm[1] == 0x666666 || $cm[1] == 0x444444;
			is_deeply( \@cm, [0, 0x555555], "color $typename->nibble is gray");
			if (( $type & im::BPP ) > 4 ) {
				@cm = $i->clone(type => $type)-> clone( type => im::bpp8, palette => 2 )->colormap;
				$cm[1] = 0x555555 if $cm[1] == 0x666666 || $cm[1] == 0x444444;
				is_deeply( \@cm, [0, 0x555555], "color $typename->byte is gray");
			}
		}
	} else {
		is_deeply( [$i->clone(type => $type)-> clone( type => im::Mono )->colormap], [0, 0x0000ff], "color $typename->mono is blue");
		if (( $type & im::BPP ) > 1 ) {
			is_deeply( [$i->clone(type => $type)-> clone( type => im::bpp4, palette => 2 )->colormap], [0, 0x0000ff], "color $typename->nibble is blue");
			if (( $type & im::BPP ) > 4 ) {
				is_deeply( [$i->clone(type => $type)-> clone( type => im::bpp8, palette => 2 )->colormap], [0, 0x0000ff], "color $typename->byte is blue");
			}
		}
	}
}

# check dithering capacity to sustain image statistics (grayscale, easy)
$i = Prima::Image->create(
	width   => 16,
	height  => 16,
	type    => im::Byte,
	data    => join '', map { chr } 0..255,
);
$i->size(128,128);

for my $src_type (im::RGB, im::Byte, im::bpp4|im::GrayScale, im::BW) {
	my $src = $i->clone(type => $src_type);
	for (
		['bpp1', im::Mono],
		['bpp1 gray', im::BW],
		['bpp4', im::bpp4],
		['bpp4 gray', im::bpp4|im::GrayScale],
		['bpp8', im::bpp8],
		['bpp8 gray', im::Byte]
	) {
		my ($typename, $type) = @$_;
		next if ($src_type & im::BPP) < ($type & im::BPP);
		for (@filters) {
			my ( $filtername, $filter ) = @$_;
			my %extras;
			my $src_type = $src->type & im::BPP;
			if (($type & im::BPP) == $src_type) { # noop otheriwse
				$extras{palette} = 1 << $src_type;
				$extras{palette}-- if $extras{palette} > 2;
			}
			my $j = $src->clone(type => $type, conversion => $filter, %extras)->clone(type => im::Byte);
			for (qw(rangeHi rangeLo)) {
				is( $j->$_(), $i->$_(), "dithering $src_type->$typename with $filtername, $_ ok");
			}
			my $diff = abs($i->mean - $j->mean);
			# src  dst err
			# 0 -> 0   0
			# 1 -> 0   1
			# 2 -> 0   2
			# 3 -> 0   3
			# 4 -> 1   0
			# etc      7*64 per 256 pixels = 1.75 per pixel
			cmp_ok( $diff, '<', 1.75, "dithering $src_type->$typename with $filtername, mean ok");
		}
	}
}

done_testing;
