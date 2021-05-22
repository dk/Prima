use strict;
use warnings;

use Test::More;
use Prima::Test qw(noX11);

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
	['complex', im::Complex],
	['dcomplex', im::DComplex],
# trigs are same as complex here
);

my @inttypes = (
	['bpp1', im::BW],
	['bpp4', im::bpp4],
	['bpp8', im::bpp8],
	['rgb', im::RGB],
	['int16', im::Short],
	['int32', im::Long],
);

my @filters;
for ( keys %ist:: ) {
	next if /^(AUTOLOAD|Constant)$/i;
	push @filters, [ $_, &{$ist::{$_}}() ];
}

sub bytes { unpack('H*', shift ) }
sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   " . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
	return $ok;
}

for ( @types ) {
	my ( $typename, $type ) = @$_;
	my $i = Prima::Image->create(
		width => 32,
		height => 32,
		type => im::Byte,
		conversion => ict::None,
		preserveType => 1,
		color => 0x0,
		backColor => 0xffffff,
		rop2 => rop::CopyPut,
	);
	$i->fillPattern([(7) x 3, (0) x 5]);
	$i->bar(0,0,32,32);
	my @stats1 = map { $i->stats($_) } (is::RangeLo, is::RangeHi, is::Variance, is::StdDev);
	my @max    = ( 256, 256, 256 * 256, 256 );
	$i->type($type);
	for ( @filters ) {
		my ( $filtername, $filter ) = @$_;
		my $j = $i-> clone( scaling => $filter );
		$j-> size( 64, 64 );
		$j-> size( 32, 32 );
		$j-> type(im::Byte);
		my @stats2 = map { $j->stats($_) } (is::RangeLo, is::RangeHi, is::Variance, is::StdDev);
		my @err;
		for ( my $k = 0; $k < @stats1; $k++) {
			my $err = int(abs(($stats1[$k] - $stats2[$k]) / $max[$k]) * 100);
			$err = 100 if $err > 100;
			push @err, $err;
		}
		my $lim = ( $filter > ist::OR ) ? 10 : 0;
		my $nonzero = grep { $_ > $lim } @err;
		ok(!$nonzero, "$typename $filtername");
		diag( "accumulated errors: @err" ) if $nonzero;
	}
}

for ( @inttypes ) {
	my ( $typename, $type ) = @$_;
	my $i = Prima::Image->create(
		width => 8,
		height => 8,
		type => $type,
		color => 0x0,
		backColor => 0xffffff,
		rop2 => rop::CopyPut,
	);
	$i->fillPattern([(0x11,0x11,0x44,0x44)x2]);
	$i->bar(0,0,7,7);
	$i->scaling(ist::AND);
	$i->size(4,4);
	$i->type(im::Byte);
	is_bytes($i->data,
		"\xff\x00\xff\x00".
		"\x00\xff\x00\xff".
		"\xff\x00\xff\x00".
		"\x00\xff\x00\xff",
		"$typename ist::AND");

	$i->size(8,8);
	$i->fillPattern([(0xee,0xee,0xbb,0xbb)x2]);
	$i->bar(0,0,7,7);
	$i->scaling(ist::OR);
	$i->size(4,4);
	$i->type(im::Byte);
	is_bytes($i->data,
		"\x00\xff\x00\xff".
		"\xff\x00\xff\x00".
		"\x00\xff\x00\xff".
		"\xff\x00\xff\x00",
		"$typename ist::OR");
}

done_testing;
