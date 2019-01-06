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

my @filters;
for ( keys %ist:: ) {
	next if /^(AUTOLOAD|Constant)$/i;
	push @filters, [ $_, &{$ist::{$_}}() ];
}

plan tests => @filters * @types;

sub bytes { unpack('H*', shift ) }
sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   " . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
}

for ( @types ) {
	my ( $typename, $type ) = @$_;
	my $i = Prima::Image->create(
		width => 32,
		height => 32,
		type => $type,
		conversion => ict::None,
		preserveType => 1,
	);
	for ( @filters ) {
		my ( $filtername, $filter ) = @$_;
		my $j = $i-> clone( scaling => $filter );
		$j-> size( 64, 64 );
		$j-> size( 32, 32 );
		is_bytes( $i->data, $j-> data, "$typename $filtername");
	}
}
