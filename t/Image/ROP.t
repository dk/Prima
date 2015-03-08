use Test::More tests => 48;
use Prima::Test qw(noX11);

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

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

my $src = Prima::Image-> create(
	width        => 1,
	height       => 1,
	type         => im::BW,
	conversion   => ict::None,
	preserveType => 1,
);
my $dst = $src-> dup;

my @table = (
	[0,0,0],
	[0,0xffffff,1],
	[0xffffff,0,2],
	[0xffffff,0xffffff,3],
);

my $test_name = "image ";

AGAIN:
for ( my $i = 0; $i < @alu; $i++) {
	my $res = 0;
	for ( @table) {
		my ($s, $d, $p) = @$_;
		$src-> pixel( 0, 0, $s);
		$dst-> pixel( 0, 0, $d);
		$dst-> put_image( 0, 0, $src, $rop::{$alu[$i]}->());
		$res |= ( $dst-> pixel(0, 0) & 1) << $p;
	}
	is( $res, $i, $test_name.$alu[$i] );
}

unless ( $src-> get_paint_state) {
	$src-> begin_paint;
	$dst-> begin_paint;
       $test_name = "gui ";
	goto AGAIN;
} else {
	$src-> end_paint;
	$dst-> end_paint;
}

# third stage, with map
$src-> type( im::Byte);
$dst-> type( im::Byte);
for ( my $i = 0; $i < @alu; $i++) {
	my $res = 0;
	for ( @table) {
		my ($s, $d, $p) = @$_;
		$src-> pixel( 0, 0, $d);
		my $g = $src-> pixel(0,0);
		$src-> set(
			color     => $s,
			backColor => $s,
			rop       => $rop::{$alu[$i]}->(),
			rop2      => $rop::{$alu[$i]}->(),
		);
		$src-> map( 0);
		$res |= ( $src-> pixel(0, 0) & 1) << $p;
	}
	is( $res, $i, "map ". $alu[ $i] );
}

done_testing();
