use strict;
use warnings;

use Test::More;
use Prima::Test;

plan tests => 112;

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
my @aluref = 0..15;
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
	sub {1},                # Whiteness
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

sub test_alu
{
	for ( my $i = 0; $i < @alu; $i++) {
		my $res = 0;
		for ( @table) {
			my ($s, $d, $p) = @$_;
			$src-> pixel( 0, 0, $s);
			$dst-> pixel( 0, 0, $d);
			$dst-> put_image( 0, 0, $src, $rop::{$alu[$i]}->());
			$res |= ( $dst-> pixel(0, 0) & 1) << $p;
		}
		is( $res, $aluref[$i], $test_name.$alu[$i] );
	}
}

sub fill_colored_alu
{
	my ( $fore, $back ) = @_;
	my $tf = $fore ? cl::Set : cl::Clear;
	my $tb = $back ? cl::Set : cl::Clear;
	@table = (
		[$tf,cl::Clear,0],
		[$tf,cl::Set,  1],
		[$tb,cl::Clear,2],
		[$tb,cl::Set,  3],
	);
	for ( my $i = 0; $i < @alu; $i++) {
		my $op = $ops[$i];
		my $val = 0;
		for (@table) {
			my ( $src, $dst, $shift ) = @$_;
			$src = ( $src == cl::Set ) ? 1 : 0;
			$dst = ( $dst == cl::Set ) ? 1 : 0;
			$val |= $op->($src, $dst) << $shift;
		}
		$aluref[$i] = $val;
	}
}

test_alu;

$src-> begin_paint;
$dst-> begin_paint;
$test_name = "gui ";
test_alu;
$src-> end_paint;
$dst-> end_paint;

my $src_old = $src;
my $dst_old = $dst;
$src = $src->bitmap;
$dst = $dst->bitmap;
$test_name = "bitmap (fc=1,bc=0) ";
$src->set(color => cl::Set, backColor => cl::Clear);
fill_colored_alu(1,0);
test_alu;

$test_name = "bitmap (fc=0,bc=0) ";
$src->set(color => cl::Clear, backColor => cl::Clear);
fill_colored_alu(0,0);
test_alu;

$test_name = "bitmap (fc=1,bc=1) ";
$src->set(color => cl::Set, backColor => cl::Set);
fill_colored_alu(1,1);
test_alu;

$test_name = "bitmap (fc=0,bc=1) ";
$src->set(color => cl::Clear, backColor => cl::Set);
fill_colored_alu(0,1); # this restores colors back to default
test_alu;

$src = $src_old;
$dst = $dst_old;

$dst-> end_paint;
$src = $src_old;

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
