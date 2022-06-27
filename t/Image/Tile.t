use strict;
use warnings;

use Test::More;
use Prima::sys::Test qw(noX11);

sub is_pict
{
	my ( $i, $name, $pict ) = @_;
	my $ok = 1;
	ALL: for ( my $y = 0; $y < $i->height; $y++) {
		for ( my $x = 0; $x < $i->width; $x++) {
			my $actual   = ( $i->pixel($x,$y) > 0) ? 1 : 0;
			my $expected = (substr($pict, ($i->height-$y-1) * $i->width + $x, 1) eq ' ') ? 0 : 1;
			next if $actual == $expected;
			$ok = 0;
			last ALL;
		}
	}
	ok( $ok, $name );
	return 1 if $ok;
	warn "# Actual vs expected:\n";
	for ( my $y = 0; $y < $i->height; $y++) {
		my $actual   = join '', map { ($i->pixel($_,$i->height-$y-1) > 0) ? '*' : ' ' } 0..$i->width-1;
		my $expected = substr($pict, $y * $i->width, $i->width);
		warn "$actual  | $expected\n";
	}
	return 0;
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

my ($mtile,$mdest,$tile,$i);

$mtile = Prima::Image->new( size => [2,2], type => im::BW );
$mtile->clear;
$mtile->pixel(0,0,0);
$mtile->pixel(1,1,0);

$mdest = Prima::Image->new( size => [4,4], type => im::bpp1 );

sub cmp_check_1
{
	my $src_bpp = $tile->type & im::BPP;
	my $dst_bpp = $i->type & im::BPP;


	my %t = @_;
	my $id = delete $t{id};
	$id =~ /^(\w+)/;
	my $aluname = $1;
	my $inverse = delete $t{inv};
	my $j = $i->extract(0,0,2,2)->dup;
	$j->set(
		backColor         => 0x0,
		color             => 0xffffff,
		rop               => rop::CopyPut,
		rop2              => rop::CopyPut,
	);
	$j->clear;
	$j->bar(0,0,1,0);
	$j->set(
		%t,
		fillPatternOffset => [0,0],
		fillPattern       => $tile,
	);
	$j->set(
		backColor         => 0xffffff,
		color             => 0x0
	) if $inverse;
	$j->colormap($j->backColor, $j->color);
	$j->bar(0,0,1,1);

	$j->resample(0, $j->rangeHi, 0, -1) if $j->type == im::Short; # -1::IV is 0xffff::short but 0xffffff::IV is 32767::short

	my $c = $j->dup;
	$c->set(
		color             => 0xffffff,
		backColor         => 0x0,
		rop               => rop::CopyPut,
		rop2              => rop::CopyPut,
	);
	$c->clear;
	$c->bar(0,0,1,0);
	$c->resample(0, 32767, 0, -1) if $c->type == im::Short;
	my $cb = $ops[ $alunames{ $aluname }];
	my $white = ($i->type & im::SignedInt) ? -1 : 0xffffff;
	for my $x (0,1) {
		for my $y (0,1) {
			my $s = $tile->pixel($x,$y) ? 1 : 0;
			my $d = $c->pixel($x,$y)    ? 1 : 0;
			my $z = $cb->(($inverse ? !$s : $s), $d) ? $white : 0;
	#		warn "+$x $y = $z \n" if $s || $t{rop2} == rop::CopyPut;
			$c->pixel($x,$y,$z) if $s || $t{rop2} == rop::CopyPut;
		}
	}
	is($j->pixel(0,0), $c->pixel(0,0), "$src_bpp/$dst_bpp 00 $id");
	is($j->pixel(0,1), $c->pixel(0,1), "$src_bpp/$dst_bpp 01 $id");
	is($j->pixel(1,0), $c->pixel(1,0), "$src_bpp/$dst_bpp 10 $id");
	is($j->pixel(1,1), $c->pixel(1,1), "$src_bpp/$dst_bpp 11 $id");
}

sub test_pat
{
	my $src_bpp = $tile->type & im::BPP;
	my $dst_bpp = $i->type & im::BPP;

	$i->rop2(rop::CopyPut);

	$i->backColor(0);
	$i->color(cl::White);
	$i->clear;
	$i->bar(0,0,3,3);
	is_pict($i, "[$src_bpp/$dst_bpp] 4x0",
		"* * ".
		" * *".
		"* * ".
		" * *"
	);

	$i->clear;
	$i->bar(0,0,2,2);
	is_pict($i, "[$src_bpp/$dst_bpp] 3x0",
		"    ".
		" *  ".
		"* * ".
		" *  "
	);

	$i->clear;
	$i->bar(1,1,3,3);
	is_pict($i, "[$src_bpp/$dst_bpp] 3x1",
		"  * ".
		" * *".
		"  * ".
		"    "
	);

	$i->clear;
	$i->bar(1,1,2,2);
	is_pict($i, "[$src_bpp/$dst_bpp] 2x2",
		"    ".
		" *  ".
		"  * ".
		"    "
	);

	$i->clear;
	$i->fillPatternOffset(1,0);
	$i->bar(0,0,3,3);
	is_pict($i, "[$src_bpp/$dst_bpp] 3x0/1.0",
		" * *".
		"* * ".
		" * *".
		"* * "
	);

	$i->backColor(cl::White);
	$i->color(0);
	$i->clear;
	$i->bar(0,0,2,2);
	is_pict($i, "[$src_bpp/$dst_bpp] inv",
		"****".
		" * *".
		"* **".
		" * *"
	) if $tile->type == im::BW;

	$i->clear;
	$i->rop(rop::NotPut);
	$i->bar(0,0,2,2);
	is_pict($i, "[$src_bpp/$dst_bpp] inv not",
		"****".
		"* **".
		" * *".
		"* **"
	) if $i->type != im::Short && $tile->type == im::BW; # binops on uint16_t are meaningless to be checked arithmetically

	$i->rop(rop::CopyPut);
	$i->clear;
	$i->rop(rop::NotPut);
	$i->backColor(0);
	$i->color(cl::White);
	$i->bar(0,0,2,2);
	$i->backColor(0);
	is_pict($i, "[$src_bpp/$dst_bpp] not",
		"****".
		" * *".
		"* **".
		" * *"
	) if $i->type != im::Short && $tile->type == im::BW; # binops on uint16_t are meaningless to be checked arithmetically
	$i->rop(rop::CopyPut);

	for my $alu (@alu) {
		my $rop = $rop::{$alu}->();

		next if $i->type == im::Short and $alu ne 'CopyPut'; # makes no sense on binops
		cmp_check_1(rop => $rop, rop2 => rop::CopyPut, inv => 0, id => "$alu/CopyPut");

		next if $tile->type != im::BW; # NoOper and coloring is for stippling only
		cmp_check_1(rop => $rop, rop2 => rop::CopyPut, inv => 1, id => "$alu/CopyPut inv");
		cmp_check_1(rop => $rop, rop2 => rop::NoOper , inv => 0, id => "$alu/NoOper");
		cmp_check_1(rop => $rop, rop2 => rop::NoOper , inv => 1, id => "$alu/NoOper inv");
	}

}

for my $src_bpp ( im::BW, 4, 8, 24 ) {
	$tile = $mtile->clone( type => $src_bpp );
	for my $dst_bpp ( 1, 4, 8, 24, im::Short ) {
		my $d = $dst_bpp;
		$d |= im::GrayScale if $d < 24;
		$i = $mdest->clone( type => $d, fillPattern => $tile);
		test_pat();
	}
}

done_testing;
