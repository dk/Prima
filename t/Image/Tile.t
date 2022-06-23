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

my $tile;
my $dst;

$tile = Prima::Image->new( size => [2,2], type => im::BW );
$tile->clear;
$tile->pixel(0,0,0);
$tile->pixel(1,1,0);

$dst  = Prima::Image->new( size => [4,4], type => im::bpp1 );
$dst->clear;
$dst->rop2(rop::CopyPut);
$dst->fillPattern($tile);
$dst->bar(0,0,2,2);
ok(1);

done_testing;
