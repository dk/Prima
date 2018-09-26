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
);

my @filters;
for ( sort keys %ict:: ) { 
	next if /^(AUTOLOAD|Constant)$/i;
	push @filters, [ $_, &{$ict::{$_}}() ];
}

plan tests => @filters * @types * @types;

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
		}			
	}
}
