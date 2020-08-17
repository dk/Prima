use strict;
use warnings;

use Test::More;
use Prima::Test;

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

# test alpha
sub img($)
{
   Prima::Image->create( width => 4, height => 1, type => im::Byte, data => shift);
}

sub is_bits
{
   my ( $x, $y, $t ) = @_;
   if ( $x eq $y ) {
      pass($t);
   } else {
      fail($t);
      warn unpack('H*', $x) , ' != ', unpack('H*', $y);
   }
}

# test constant alpha blending
$src = img("\1\x0f\xf0\xff");
$dst = img("\0\1\2\3");
$dst->put_image( 0,0,$src, rop::blend(0));
is_bits( $dst->data, "\0\1\2\3", "alpha 0");

$dst = img("\0\1\2\3");
$dst->put_image( 0,0,$src, rop::blend(255));
is_bits( $dst->data, $src->data, "alpha 1");

$src = img("\0\3\6\x9");
$dst = img("\xf0\xf0\xf0\xf0");
$dst->put_image( 0,0,$src, rop::blend(85));
is_bits( $dst->data, "\xa0\xa1\xa2\xa3", "alpha 1/3");

$src = img("\0\3\6\x9");
$dst = img("\xf0\xf0\xf0\xf0");
$dst->put_image_indirect( $src, 2,0,0,0, 2,1,2,1, rop::blend(85));
is_bits( $dst->data, "\xf0\xf0\xa0\xa1", "alpha 1/3 with src shift");

$dst = Prima::Icon->new(
	width  => 4,
	height => 1,
	maskType => 8,
	mask     => "1234",
);
$src = img("5678");
$dst->put_image( 0,0, $src, rop::AlphaCopy);
is_bits( $dst->mask, "5678", "ropAlphaCopy");

$dst->alpha(ord("9"), 1, 0, 2, 0);
is_bits( $dst->mask, "5998", "alpha(x)");

SKIP: {
	skip "no argb support", 1 unless Prima::Application->get_system_value(sv::LayeredWidgets);
	$dst->begin_paint;
	$dst->alpha(ord("0"), 1, 0, 2, 0);
	$dst->end_paint;
	is_bits( $dst->mask, "5008", "alpha(x) in paint");
}

$dst = Prima::Image->new(
	width  => 4,
	height => 1,
	type   => im::Byte,
	backColor => 0x808080,
	color     => 0x202020,
);
$dst->clear;
$dst->put_image( 0,0,$dst->dup, rop::ConstantColor | rop::SrcOver | rop::Premultiply );
is_bits( $dst->data, "PPPP", "rop::ConstantColor ( 0x80 + 0x20 ) / 2 = 0x50");

# test porter-duff
sub pd_color
{
	my ( $rop, $s, $as, $d, $ad ) = @_;

	my $dst;

	   if ( $rop == rop::SrcOver ) {  $dst = $s + ($d * (255 - $as)) / 255.0               }
	elsif ( $rop == rop::DstOver ) {  $dst = $s * (255 - $ad) / 255.0 + $d                 }
	elsif ( $rop == rop::DstCopy ) {  $dst = $d                                            }
	elsif ( $rop == rop::Clear   ) {  $dst = 0                                             }
	elsif ( $rop == rop::SrcIn   ) {  $dst = $s * $ad / 255.0                              }
	elsif ( $rop == rop::DstIn   ) {  $dst = $d * $as / 255.0                              }
	elsif ( $rop == rop::SrcOut  ) {  $dst = $s * (255 - $ad) / 255.0                      }
	elsif ( $rop == rop::DstOut  ) {  $dst = $d * (255 - $as) / 255.0                      }
	elsif ( $rop == rop::SrcAtop ) {  $dst = ($s * $ad + $d * (255 - $as)) / 255.0         }
	elsif ( $rop == rop::DstAtop ) {  $dst = ($s * (255 - $ad) + $d * $as) / 255.0         }
	elsif ( $rop == rop::Xor     ) {  $dst = ($s * (255 - $ad) + $d * (255 - $as)) / 255.0 }
	elsif ( $rop == rop::SrcCopy ) {  $dst = $s                                            }
	else  { $dst = -1 }

	$dst += .5;
	if ( $dst > 255 ) { $dst = 255 };
	return int( $dst );
}

sub pd_alpha
{
	my ( $rop, $as, $ad ) = @_;
	return pd_color( $rop, $as, $as, $ad, $ad);
}

sub test_rop
{
	my ($name) = @_;
	my $rop = $rop::{$name}->();
	my @q = (0, 85, 170, 255);

	for my $bytes ( 1, 3 ) {
 		my $subname = ($bytes * 8) . ' bits ';

		my $src = Prima::Icon->new(
			width    => 4,
			height   => 5,
			data     => join('', map { chr } @q, @q, @q, @q, @q),
			mask     => join('', map { chr } @q, @q, @q, @q, @q),
			type     => im::Byte,
			maskType => im::Byte,
		);

		my $dst = Prima::Icon->new(
			width    => 4,
			height   => 5,
			data     => join('', map { chr } @q, reverse(@q), @q, reverse(@q), @q),
			mask     => join('', map { chr } @q, reverse(@q), @q, reverse(@q), @q),
			type     => im::Byte,
			maskType => im::Byte,
		);

		if ( $bytes == 3 ) {
			$src->type(im::RGB);
			$dst->type(im::RGB);
		}

		$dst->put_image_indirect($src,0,0,0,0,4,2,4,2,$rop);
		$dst->put_image_indirect($src,0,2,0,2,4,1,4,1,rop::alpha( $rop, 128 ));
		$dst->put_image_indirect($src,0,3,0,3,4,1,4,1,rop::alpha( $rop, undef, 128 ));
		$dst->put_image_indirect($src,0,4,0,4,4,1,4,1,$rop | rop::Premultiply);

		my ( $cc, $aa ) = $dst->split;
		$cc->type(im::Byte);
		$aa->type(im::Byte);

		for ( my $i = 0; $i < @q; $i++) {
			my $q = $q[$i];
			my $c = pd_color( $rop, $q, $q, $q, $q );
			my $a = pd_alpha( $rop, $q, $q );
			my $pc = $cc->pixel($i, 0);
			my $pa = $aa->pixel($i, 0);
			is( $pc, $c, "C (($q/$q) $name ($q/$q)) = $c $subname");
			is( $pa, $a, "A (($q/$q) $name ($q/$q)) = $a $subname");

			my $q2 = 255 - $q[$i];
			$c = pd_color( $rop, $q, $q, $q2, $q2 );
			$a = pd_alpha( $rop, $q, $q2 );
			$pc = $cc->pixel($i, 1);
			$pa = $aa->pixel($i, 1);
			is( $pc, $c, "C (($q/$q) $name ($q2/$q2)) = $c $subname");
			is( $pa, $a, "A (($q/$q) $name ($q2/$q2)) = $a $subname");
			
			my $q_half = int( $q / 2 + .5);
			$c = pd_color( $rop, $q, $q_half, $q, $q);
			$a = pd_alpha( $rop, $q_half, $q);
			$pc = $cc->pixel($i, 2);
			$pa = $aa->pixel($i, 2);
			is( $pc, $c, "C (($q/{$q/2}) $name ($q/$q)) = $c $subname");
			is( $pa, $a, "A (($q/{$q/2}) $name ($q/$q)) = $a $subname");
			
			my $q2_half = int( $q2 / 2 + .5);
			$c = pd_color( $rop, $q, $q, $q2, $q2_half);
			$a = pd_alpha( $rop, $q, $q2_half);
			$pc = $cc->pixel($i, 3);
			$pa = $aa->pixel($i, 3);
			is( $pc, $c, "C (($q/$q) $name ($q/{$q/2})) = $c $subname");
			is( $pa, $a, "A (($q/$q) $name ($q/{$q/2})) = $a $subname");

			my $qm = int($q * $q / 255 + .5);
			$c = pd_color( $rop, $qm, $q, $q, $q );
			$a = pd_alpha( $rop, $q, $q );
			$pc = $cc->pixel($i, 4);
			$pa = $aa->pixel($i, 4);
			is( $pc, $c, "C*(($q/$q) $name ($q/$q)) = $c $subname");
			is( $pa, $a, "A*(($q/$q) $name ($q/$q)) = $a $subname");
		}
	}
}

test_rop( $_ ) for qw(
	SrcOver DstOver DstCopy Clear  SrcIn  DstIn
	SrcOut DstOut SrcAtop DstAtop Xor    SrcCopy
);

done_testing;
