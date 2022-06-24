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

my ($mtile,$mdest,$tile,$i);

$mtile = Prima::Image->new( size => [2,2], type => im::BW );
$mtile->clear;
$mtile->pixel(0,0,0);
$mtile->pixel(1,1,0);

$mdest = Prima::Image->new( size => [4,4], type => im::bpp1 );

sub test_pat
{
	my $src_bpp = $tile->type & im::BPP;
	my $dst_bpp = $i->type & im::BPP;

	$i->rop2(rop::CopyPut);

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
		"****".
		" * *".
		"* **".
		" * *"
	);

	$i->clear;
	$i->bar(1,1,3,3);
	is_pict($i, "[$src_bpp/$dst_bpp] 3x1",
		"* * ".
		"** *".
		"* * ".
		"****"
	);

	$i->clear;
	$i->bar(1,1,2,2);
	is_pict($i, "[$src_bpp/$dst_bpp] 2x2",
		"****".
		"** *".
		"* **".
		"****"
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

	$i->backColor(0);
	$i->color(0xffffff);
	if ( $i->type == im::BW) {
		$i->clear;
		$i->bar(0,0,2,2);
		is_pict($i, "[$src_bpp/$dst_bpp] inv",
			"    ".
			" *  ".
			"* * ".
			" *  "
		);
	}

	$i->clear;
	$i->rop(rop::NotPut);
	$i->bar(0,0,2,2);
	is_pict($i, "[$src_bpp/$dst_bpp] not",
		"    ".
		"* * ".
		" *  ".
		"* * "
	);
	$i->rop(rop::CopyPut);

	$i->backColor(0xffffff);
	$i->color(0);
	$i->clear;
	$i->bar(0,0,3,3);
	$i->rop2(rop::NoOper);
	$i->fillPatternOffset(0,0);
	$i->bar(0,2,3,3);
	is_pict($i, "[$src_bpp/$dst_bpp] transparent",
		"    ".
		"    ".
		" * *".
		"* * "
	);

#	$i->clear;
#	$i->rop2(rop::CopyPut);
#	$i->fillPatternOffset(1,0);
#	$i->bar(0,0,3,3);
#	$i->fillPatternOffset(0,0);
#	$i->rop2(rop::NoOper);
#	$i->color(0xffffff);
#	$i->backColor(0);
#	$i->bar(0,2,3,3);
#	is_pict($i, "[$src_bpp/$dst_bpp] 4x0",
#		"    ".
#		"    ".
#		"* * ".
#		" * *"
#	);
}

for my $src_bpp ( im::BW, 4, 8, 24 ) {
	next unless $src_bpp == im::BW;
	$tile = $mtile->clone( type => $src_bpp );
	for my $dst_bpp ( 1, 4, 8, 24 ) {
		next unless $dst_bpp == 1;
		$i = $mdest->clone( type => $dst_bpp, fillPattern => $tile);
		test_pat();
	}
}

done_testing;
