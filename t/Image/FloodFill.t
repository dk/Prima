
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

for my $bpp ( 1, 4, 8, 24 ) {
	my $i = Prima::Image->create(
		width     => 5,
		height    => 5,
		type      => $bpp,
		color     => cl::White,
		backColor => cl::Black,
	);

	$i->clear;
	$i->line(0,0,4,4);
	$i->flood_fill(4,2,cl::White,0);
	is_pict($i, "bpp $bpp: white",
		"    *".
		"   **".
		"  ***".
		" ****".
		"*****"
	);

	$i->clear;
	$i->line(0,4,4,0);
	$i->flood_fill(4,2,cl::Black,1);
	is_pict($i, "bpp $bpp: non-black",
		"*****".
		" ****".
		"  ***".
		"   **".
		"    *"
	);

	$i->clear;
	$i->line(0,0,4,4);
	$i->clipRect(1,1,3,3);
	$i->flood_fill(3,2,cl::White,0);
	is_pict($i, "bpp $bpp: with clipping",
		"    *".
		"   * ".
		"  ** ".
		" *** ".
		"*    "
	);

	$i->clipRect(0,0,4,4);
	$i->clear;
	$i->line(0,0,4,4);
	$i->clipRect(1,1,3,3);
	$i->flood_fill(4,4,cl::White,0);
	is_pict($i, "bpp $bpp: outside",
		"    *".
		"   * ".
		"  *  ".
		" *   ".
		"*    "
	);
}

done_testing;
