use strict;
use warnings;

use Test::More;
use Prima::sys::Test qw(noX11);

my ( $tile, $i, $j, $z );

my %resample;
my @resample = ('#','*','=','+','-',':','.',' ');
$resample{$_} = scalar keys %resample for @resample;

sub is_pict
{
	my ( $i, $name, $pict ) = @_;
	$i = $i->clone(type => im::Byte);
	$i->resample(0,255,0,7);
	my $ok = 1;
	ALL: for ( my $y = 0; $y < $i->height; $y++) {
		for ( my $x = 0; $x < $i->width; $x++) {
			my $actual   = $i->pixel($x,$y);
			my $expected = substr($pict, ($i->height-$y-1) * $i->width + $x, 1);
			$expected = $resample{$expected};
			next if $actual == $expected;
			$ok = 0;
			last ALL;
		}
	}
	ok( $ok, $name );
	return 1 if $ok;
	warn "# Actual vs expected:\n";
	for ( my $y = 0; $y < $i->height; $y++) {
		my $actual   = join '', map { my $p = $i->pixel($_,$i->height-$y-1); $resample[$p] // '?' } 0..$i->width-1;
		my $expected = substr($pict, $y * $i->width, $i->width);
		warn "# [$actual][$expected]\n";
	}
	return 0;
}

sub check
{
	my ( $i, $name, $chk, %opt) = @_;
	$j = $i->clone(antialias => 1, %opt);
	$j->fillpoly([0,0,0,2,2,2,2,0]);
	my $typename = ( $j->type == im::Byte) ? 'gray' : 'color';
	is_pict($j, "$typename: $name", $chk);
}

sub check_icon
{
	my ( $i, $name, $chk, %opt) = @_;
	$j = $i->clone(antialias => 1, %opt);
	$j->fillpoly([0,0,0,2,2,2,2,0]);
	my ( $xx, $xa ) = $j->split;
	my $typename = ( $j->type == im::Byte) ? 'gray icon' : 'color icon';
	is_pict($xa, "$typename: $name", $chk);
}

sub check_both
{
	my ( $i, $name, $chk1, $chk2, %opt) = @_;
	$j = $i->clone(antialias => 1, %opt);
	$j->fillpoly([0,0,0,2,2,2,2,0]);
	my ( $xx, $xa ) = $j->split;
	my $typename = ( $j->type == im::Byte) ? 'gray' : 'color';
	is_pict($xx, "$typename icon data: $name", $chk1);
	is_pict($xa, "$typename icon mask: $name", $chk2);
}

sub test_target
{
	my $type = shift;
	my $typename = ( $type == im::Byte) ? 'gray' : 'color';
	$i = Prima::Image->create(
		size      => [4,4],
		type      => $type,
	);
	$i->clear;
	$i->color(0x808080);
	$i->bar(0,0,3,1);
	is_pict($i, "$typename: empty",
		"    ".
		"    ".
		"++++".
		"++++"
	);

	$i->color(0);

	$j = $i->clone( antialias => 1 );
	$j->fillpoly([1,1,3,1,3,3,1,3]);
	is_pict($j, "$typename: solid",
		"    ".
		" ## ".
		"+##+".
		"++++"
	);
	$i = Prima::Image->create(
		size      => [2,2],
		type      => $type,
	);
	$i->clear;
	$i->color(0x808080);
	$i->bar(0,0,1,0);
	is_pict($i, "$typename: base", "  ++");

	my %fill = (
		fillPattern => [(0xAA,0x55)x4],
		color       => 0,
		backColor   => 0xffffff,
	);
	check( $i, "solid.a=0xff"   , "####", color => 0);
	check( $i, "solid.a=0x80"   , "++**", alpha => 128, color => 0);

	my $xa = Prima::Image->new( type => im::Byte, size => [$i->size] );
	$xa->clear;
	$xa->color(0x808080);
	$xa->bar(0,0,1,0);
	$z = Prima::Icon->create_combined($i, $xa);
	is_pict( $i, "mask", "  ++");

	check_icon( $z, "solid.a=0xff.1"   , "    ", color => 0);
	check_icon( $z, "solid.a=0xff.2"   , "    ", color => 0xffffff);
	check_icon( $z, "solid.sa=0x80"    , "::++", color => 0, rop => rop::alpha(rop::SrcOver, 0x80, 0xff));
	check_icon( $z, "solid.da=0x80"    , "    ", color => 0, rop => rop::alpha(rop::SrcOver, 0xff, 0x80));

	check_both( $z, "pat.transparent", " ##+", "   +", alpha => 255, %fill, rop2 => rop::NoOper);
	check_both( $z, "pat.semitransp" , " +*+", " :++", alpha => 128, %fill, rop2 => rop::NoOper);
	check_both( $z, "pat.fulltransp" , "  ++", "  ++", alpha =>   0, %fill, rop2 => rop::NoOper);
	check_both( $z, "pat.offset-x"   , "# +#", "  + ", alpha => 255, %fill, rop2 => rop::NoOper, fillPatternOffset => [1,0]);
	check_both( $z, "pat.offset-y"   , "# +#", "  + ", alpha => 255, %fill, rop2 => rop::NoOper, fillPatternOffset => [0,1]);
	check_both( $z, "pat.offset-xy"  , " ##+", "   +", alpha => 255, %fill, rop2 => rop::NoOper, fillPatternOffset => [3,3]);
	check_both( $z, "pat.opaque"     , " ## ", "    ", alpha => 255, %fill, rop2 => rop::CopyPut);
	check_both( $z, "pat.semiopaque" , " +*:", "::++", alpha => 128, %fill, rop2 => rop::CopyPut);
	check_both( $z, "pat.noopaque"   , "  ++", "  ++", alpha =>   0, %fill, rop2 => rop::CopyPut);

	my $fp = Prima::Image->new(
		type        => im::BW,
		fillPattern => [(0xAA,0x55)x4],
		size        => [8,8],
		rop2        => rop::CopyPut,
	);
	$fp->bar(0,0,8,8);
	$fill{fillPattern} = $fp;
	check_both( $z, "fp1.transparent", " ##+", "   +", alpha => 255, %fill, rop2 => rop::NoOper);
	check_both( $z, "fp1.semitransp" , " +*+", " :++", alpha => 128, %fill, rop2 => rop::NoOper);
	check_both( $z, "fp1.fulltransp" , "  ++", "  ++", alpha =>   0, %fill, rop2 => rop::NoOper);
	check_both( $z, "fp1.offset-x"   , "# +#", "  + ", alpha => 255, %fill, rop2 => rop::NoOper, fillPatternOffset => [1,0]);
	check_both( $z, "fp1.offset-y"   , "# +#", "  + ", alpha => 255, %fill, rop2 => rop::NoOper, fillPatternOffset => [0,1]);
	check_both( $z, "fp1.offset-xy"  , " ##+", "   +", alpha => 255, %fill, rop2 => rop::NoOper, fillPatternOffset => [3,3]);
	check_both( $z, "fp1.opaque"     , " ## ", "    ", alpha => 255, %fill, rop2 => rop::CopyPut);
	check_both( $z, "fp1.semiopaque" , " +*:", "::++", alpha => 128, %fill, rop2 => rop::CopyPut);
	check_both( $z, "fp1.noopaque"   , "  ++", "  ++", alpha =>   0, %fill, rop2 => rop::CopyPut);

	$fp->type($type);
	$fp->pixel(0,0,0x808080);
	check_both( $z, "fpX.opaque"     , " #+ ", "    ", alpha => 255, %fill);
	check_both( $z, "fpX.semiopaque" , " ++:", "::++", alpha => 128, %fill);
	check_both( $z, "fpX.noopaque"   , "  ++", "  ++", alpha =>   0, %fill);

	my $fi = Prima::Image->new(
		type        => im::BW,
		size        => [8,8],
	);
	$fi->clear;
	$fi->bar(0,0,1,0);
	$fill{fillPattern} = Prima::Icon->create_combined( $fp, $fi );
	check_both( $z, "fpX+1.opaque"     , "  + ", "    ", alpha => 255, %fill);
	check_both( $z, "fpX+1.semiopaque" , "  +:", "  ++", alpha => 128, %fill);
	check_both( $z, "fpX+1.noopaque"   , "  ++", "  ++", alpha =>   0, %fill);

	$fi->type(im::Byte);
	$fi->data(~$fi->data); # so tests stay the same
	$fi->pixel(1,0,0x808080);
	$fill{fillPattern} = Prima::Icon->create_combined( $fp, $fi );
	check_both( $z, "fpX+8.opaque"     , "  +:", "   +", alpha => 255, %fill);
	check_both( $z, "fpX+8.semiopaque" , "  +-", "  :+", alpha => 128, %fill);
	check_both( $z, "fpX+8.noopaque"   , "  ++", "  ++", alpha =>   0, %fill);
}

sub test_region
{
	$i = Prima::Image->create(
		size      => [6,6],
		type      => im::Byte,
	);
	$i->region( Prima::Region->new( box => [
		-5,-2,10,2,
		-5,-1,10,2,
		-5,2,10,1,
		1,4,1,1,
		3,4,10,3,
	]));
	$i->color(0xffffff);
	$i->antialias(1);
	$i->fillpoly([0,0,0,6,6,6,6,0]);
	is_pict($i, "region",
		"###   ".
		"# #   ".
		"######".
		"     #".
		"######".
		"     #"
	);
}

for my $type ( im::Byte, im::RGB ) {
	test_target($type);
}

test_region;

done_testing;
